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

static unsigned int enc_ra = 0;
static unsigned int enc_dec = 0;
static unsigned int enc_focus = 0;

inline unsigned char
i2c_read (void)
{
    return SSPBUF;
}
inline void
i2c_write (unsigned char c)
{
    SSPCON1bits.CKP = 0;
    SSPBUF = c;
    SSPCON1bits.CKP = 1;
    (void)i2c_read();
}
/* See AN734b for 18F series I2C slave state descriptions
 */
inline unsigned char
i2c_getstate (void)
{
    unsigned char state = 0;

    // State 1: I2C write operation, last byte was an address byte
    // SSPSTAT bits: S = 1, D_A = 0, R_W = 0, BF = 1
    if (SSPSTATbits.S == 1 && SSPSTATbits.D_A == 0
                           && SSPSTATbits.R_W == 0
                           && SSPSTATbits.BF == 1) {
        state = 1;
    }
    // State 2: I2C write operation, last byte was a data byte
    // SSPSTAT bits: S = 1, D_A = 1, R_W = 0, BF = 1
    if (SSPSTATbits.S == 1 && SSPSTATbits.D_A == 1
                                  && SSPSTATbits.R_W == 0
                                  && SSPSTATbits.BF == 1) {
        state = 2;
    }
    // State 3: I2C read operation, last byte was an address byte
    // SSPSTAT bits: S = 1, D_A = 0, R_W = 1
    if (SSPSTATbits.S == 1 && SSPSTATbits.D_A == 0
                           && SSPSTATbits.R_W == 1) {

        state = 3;
    }
    // State 4: I2C read operation, last byte was a data byte
    // SSPSTAT bits: S = 1, D_A = 1, R_W = 1, BF = 0
    if (SSPSTATbits.S == 1 && SSPSTATbits.D_A == 1
                           && SSPSTATbits.R_W == 1
                           && SSPSTATbits.BF == 0) {

        state = 4;
    
    }
    // State 5: Slave I2C logic reset by NACK from master
    // SSPSTAT bits: S = 1, D_A = 1, BF = 0, CKP = 1
    if (SSPSTATbits.S == 1 && SSPSTATbits.D_A == 1
                           && SSPSTATbits.BF == 0
                           && SSPCON1bits.CKP == 1) {
        state = 5;
    }
    return state;
}

typedef enum {
    REG_BUTTONS=0,
    REG_RA_ENC=1,
    REG_DEC_ENC=2,
    REG_FOCUS_ENC=2,
} i2c_reg_t;

typedef enum {
    REG_LSB = 0,
    REG_MSB = 1,
} i2c_bytesel_t;

inline void
reg_set (unsigned char regnum, unsigned char val, i2c_bytesel_t sel)
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
                enc_ra |= (unsigned int)val << 8;
            break;
        case REG_DEC_ENC:
            if (sel == REG_LSB)
                enc_dec = val;
            else
                enc_dec |= (unsigned int)val << 8;
            break;
        case REG_FOCUS_ENC:
            if (sel == REG_LSB)
                enc_focus = val;
            else
                enc_focus |= (unsigned int)val << 8;
            break;
    }
}

inline unsigned char
reg_get (unsigned char regnum, i2c_bytesel_t sel)
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

/* Simple protocol based on 1- and 2-byte registers:
 * To write a register, master writes 2 <regnum> <lsb> [<msb>]
 * To read a register, master writes 1 <regnum>, then reads <lsb> [<msb>].
 * After a read or a write, another read re-samples the same <regnum>.
 */
typedef enum {I2C_CMD_READ=1, I2C_CMD_WRITE=2} i2c_cmd_t;
inline void
i2c_interrupt (void)
{
    static unsigned char count = 0;
    static unsigned char cmd = I2C_CMD_READ;
    static unsigned char regnum = 0;

    switch (i2c_getstate ()) {
        case 1: /* write addr */
            (void)i2c_read();
            count = 0;
            break;
        case 2: /* write 1..Nth byte */
            if (count == 0) {
               cmd = i2c_read ();
            } else if (count == 1) {
               regnum = i2c_read ();
            } else if (count == 2 && cmd == I2C_CMD_WRITE) {
               reg_set (regnum, i2c_read (), REG_LSB);
            } else if (count == 3 && cmd == I2C_CMD_WRITE) {
               reg_set (regnum, i2c_read (), REG_MSB);
            } else {
               (void)i2c_read();
            }
            count++;
            break;
        case 3: /* read 1st byte */
            count = 0;
            i2c_write (reg_get (regnum, REG_LSB));
            count++;
            break;
        case 4: /* read 2..Nth byte */
            if (count == 1)
                i2c_write (reg_get (regnum, REG_MSB));
            else
                i2c_write (0);
            count++;
            break;
        case 5:  /* NAK */
            count = 0;
            (void)i2c_read();
            break;
        default:
            break;
    }

    if (SSPCON1bits.SSPOV) {
        SSPCON1bits.SSPOV = 0;
        (void)i2c_read();
    }
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
    TRISBbits.RB4 = 1;          /* SDA */
    TRISBbits.RB6 = 1;          /* SCL */
    SSPSTAT = 0;
    SSPCON1 = 0;
    SSPCON1bits.SSPM = 6;       /* I2C slave mode, 7-bit address */
    SSPCON1bits.CKP = 1;        /* release clock */
    SSPCON2 = 0;
    SSPCON2bits.GCEN = 1;       /* disable general call */
    SSPCON2bits.SEN = 0;        /* disable clock stretching */
    SSPADD = I2C_ADDR << 1;     /* set I2C slave address */
    SSPCON1bits.SSPEN = 1;      /* enable MSSP peripheral */
    (void)i2c_read();
    SSPCON1bits.SSPOV = 0;      /* clear buffer overflow */
    SSPCON2bits.RCEN = 1;       /* receive enable */
    SSPIF = 0;                  /* clear SSP interrupt flag */
    SSPIE = 1;                  /* enable SSP interrupt */

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
