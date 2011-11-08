/*****************************************************************************\
 *  Copyright (C) 2011 Jim Garlick
 *  
 *  This file is part of ultima8drivecorrector, replacement base electronics
 *  for the Celestron Ultima 8 telescope.  For details, see
 *  <http://code.google.com/p/ultima8drivecorrector>.
 *  
 *  ultima8drivecorrector is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  ultima8drivecorrector is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *  Public License *  for more details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with ultima8drivecorrector; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

/* aux.c - firmware for Ultima 8 ra/dec/focus encoders and focus motor driver */

/* NOTE: compiled with hi-tech C pro V9.80 */

#include <htc.h>
#include "i2c_slave.h"

#define _XTAL_FREQ 64000000

#if defined(_18F14K22)
__CONFIG (1, FOSC_IRC & PLLEN_ON);  /* system clock is HFOSC*4 */
__CONFIG (2, BOREN_OFF & WDTEN_OFF);
__CONFIG (3, HFOFST_OFF & MCLRE_OFF);
__CONFIG (4, LVP_OFF);
#else
#error code assumes 18F14K22 chip.
#endif

#define I2C_ADDR        0x0a

#define FOCOUT          PORTCbits.RC1
#define FOCIN           PORTCbits.RC2
#define PWM             PORTCbits.RC5

#define DEC_A           PORTAbits.RA5
#define DEC_B           PORTAbits.RA4

#define RA_A            PORTAbits.RA3
#define RA_B            PORTBbits.RB7

#define FOC_A           PORTBbits.RB5
#define FOC_B           PORTAbits.RA2

static char ibuttons = 0; /* i2c "buttons" */
#define BUTTON_FOCIN    0x10
#define BUTTON_FOCOUT   0x20

static UINT16 enc_ra = 0;
static UINT16 enc_dec = 0;
static UINT16 enc_focus = 0;

typedef enum {
    REG_BUTTONS=0,
    REG_RA_ENC=1,
    REG_DEC_ENC=2,
    REG_FOCUS_ENC=2,
} i2c_reg_t;

void
reg_set (unsigned char regnum, unsigned char val, regbyte_t sel)
{
    switch (regnum) {
        case REG_BUTTONS:
            if (sel == REG_LSB)
                ibuttons = val;
            break;
        case REG_RA_ENC:
            if (sel == REG_LSB)
                enc_ra = val;
            else
                enc_ra |= (UINT16)val << 8;
            break;
        case REG_DEC_ENC:
            if (sel == REG_LSB)
                enc_dec = val;
            else
                enc_dec |= (UINT16)val << 8;
            break;
        case REG_FOCUS_ENC:
            if (sel == REG_LSB)
                enc_focus = val;
            else
                enc_focus |= (UINT16)val << 8;
            break;
    }
}

unsigned char
reg_get (unsigned char regnum, regbyte_t sel)
{
    unsigned char val = 0;

    switch (regnum) {
        case REG_BUTTONS:
            if (sel == REG_LSB)
                val = ibuttons;
            break;
        case REG_RA_ENC:
            if (sel == REG_LSB)
                val = (unsigned char)(enc_ra & 0xff);
            else
                val = (unsigned char)((enc_ra >> 8) & 0xff);
            break;
        case REG_DEC_ENC:
            if (sel == REG_LSB)
                val = (unsigned char)(enc_dec & 0xff);
            else
                val = (unsigned char)((enc_dec >> 8) & 0xff);
            break;
        case REG_FOCUS_ENC:
            if (sel == REG_LSB)
                val = (unsigned char)(enc_focus & 0xff);
            else
                val = (unsigned char)((enc_focus >> 8) & 0xff);
            break;
    }
    return val;
}

void interrupt
isr (void)
{
    if (SSPIE && SSPIF) {
        i2c_interrupt ();
        SSPIF = 0;
    }
}

typedef enum { FOC_OFF, FOC_IN, FOC_OUT} focus_t;

void
dc_set_focus (focus_t want)
{
    static focus_t cur = FOC_OFF;

    if (cur == want)
        return;
    switch (want) {
        case FOC_OFF:
            FOCIN = 0;
            NOP ();
            FOCOUT = 0;
            break;
        case FOC_IN:
            FOCIN = 1;
            NOP ();
            FOCOUT = 0;
            break;
        case FOC_OUT:
            FOCIN = 0;
            NOP ();
            FOCOUT = 1;
            break;
    }
    cur = want;
}

void
action (void)
{
    unsigned char b = ibuttons;

    if ((b & BUTTON_FOCIN))
        dc_set_focus (FOC_IN);
    else if (b & BUTTON_FOCOUT)
        dc_set_focus (FOC_OUT);
    else
        dc_set_focus (FOC_OFF);
}

void
main(void)
{
    OSCCONbits.IRCF = 7;        /* system clock HFOSC 16 MHz (x 4 with PLL) */

    ANSEL = 0;                  /* disable all ADC inputs */
    ANSELH = 0;
    TRISA = 0;                  /* disable all digital inputs */
    TRISB = 0;
    TRISC = 0;
    WPUA = 0;                   /* disable all pullups */
    WPUB = 0;
    IOCA = 0;                   /* disable all interrupt on change bits */
    IOCB = 0; 


    /* I2C config
     */
    i2c_init (I2C_ADDR);

    /* Digital input config
     */
    RABPU = 0;                  /* enable weak pullups feature */
    TRISAbits.RA5 = 1;          /* RA5 is input */
    WPUAbits.WPUA5 = 1;         /* pullup on RA5 */
    TRISAbits.RA4 = 1;          /* RA4 is input */
    WPUAbits.WPUA4 = 1;         /* pullup on RA4 */
    //TRISAbits.RA3 = 1;        /* RA3 is input (cannot be anything else) */
    WPUAbits.WPUA3 = 1;         /* pullup on RA3 */
    TRISBbits.RB7 = 1;          /* RB7 is input */
    WPUBbits.WPUB7 = 1;         /* pullup on RB7 */
    TRISBbits.RB5 = 1;          /* RB5 is input */
    WPUBbits.WPUB5 = 1;         /* pullup on RB5 */
    TRISAbits.RA2 = 1;          /* RA2 is input */
    WPUAbits.WPUA2 = 1;         /* pullup on RA2 */
    /* enable IOC */
    /* clear IOC interrupt flag */
    /* enable IOC interrupt */

    /* DC motor config
     */
    PWM = 1;                    /* TODO: enable PWM - for now set high */
    FOCIN = 0;
    FOCOUT = 0;

    PEIE = 1;                   /* enable peripheral interrupts */
    GIE = 1;                    /* enable global interrupts */
    for (;;) {
        action ();
        __delay_ms(1);
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
