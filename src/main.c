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

/* main.c - firmware for Ultima 8 drive corrector based on Covington's ALCOR */

/* NOTE: compiled with hi-tech C pro V9.80 */

#include <htc.h>

#include "freq.h"

#define _XTAL_FREQ 64000000

#if defined(_18F14K22)
__CONFIG (1, FOSC_IRC & PLLEN_ON);  /* system clock is HFOSC*4 */
__CONFIG (2, BOREN_OFF & WDTEN_OFF);
__CONFIG (3, HFOFST_OFF & MCLRE_OFF);
__CONFIG (4, LVP_OFF);
#else
#error code assumes 18F14K22 chip.
#endif

#define I2C_ADDR        8

#define ADC_SOUTH       3
#define ADC_WEST        2
#define ADC_EAST        8
#define SW_NORTH        PORTAbits.RA5

#define SQWAVE          PORTBbits.RB7
#define FOCOUT          PORTBbits.RB5

#define LED             PORTCbits.RC7
#define PWM             PORTCbits.RC5
#define PHASE1          PORTCbits.RC4
#define PHASE2          PORTCbits.RC3
#define FOCIN           PORTCbits.RC2
#define GONORTH         PORTCbits.RC1
#define GOSOUTH         PORTCbits.RC0

static volatile char buttons = 0;  /* bitmask of depressed handbox buttons */
static volatile char ibuttons = 0; /* i2c version of above */
#define BUTTON_NORTH    0x01
#define BUTTON_SOUTH    0x02
#define BUTTON_EAST     0x04
#define BUTTON_WEST     0x08
#define BUTTON_FOCIN    0x10
#define BUTTON_FOCOUT   0x20
#define BUTTON_LAMP     0x40

static volatile char output_inhibit = 1;

static volatile char ac_freq_now = FREQ_EAST;
static volatile char ac_freq_targ = FREQ_SIDEREAL;

void
ac_set_freq (char freq)
{
    if (freq <= FREQ_WEST)
        ac_freq_targ = freq;
}
char 
ac_get_freq (void)
{
    return ac_freq_now;
}

#define FUDGE_COUNT 500 /* FIXME shouldn't need this */

inline void
ac_interrupt (void)
{
    static char state = 0;
    static int startup_delay = 0;
    char inhibit = 0;

    if (startup_delay < 1000)
        startup_delay++;
    else
        output_inhibit = 0;
    if (output_inhibit || ac_freq_now == FREQ_EAST)
        inhibit = 1;
    switch (state) {
        case 0:
            if (!inhibit) {
                SQWAVE = 1;
                NOP ();
                PHASE1 = 1;
            }
            state = 1;
            break;
        case 1:
            if (!inhibit)
                PHASE1 = 0;
            state = 2;
            break;
        case 2:
            if (!inhibit) {
                SQWAVE = 0;
                NOP ();
                PHASE2 = 1;
            }
            state = 3;
            break;
        case 3:
            if (!inhibit)
                PHASE2 = 0;
            state = 0;
            /* Cycle completed - change timer count? */
            if (ac_freq_now < ac_freq_targ)
                ac_freq_now++;
            else if (ac_freq_now > ac_freq_targ)
                ac_freq_now--;
            break;
    }
    TMR0 = freq[ac_freq_now] + FUDGE_COUNT;
}
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

typedef enum {REG_BUTTONS=0} i2c_reg_t;
inline void
reg_set (unsigned char regnum, unsigned char val)
{
    switch (regnum) {
        case REG_BUTTONS:
            ibuttons = val;
            break;
    }
}
inline unsigned char
reg_get (unsigned char regnum)
{
    unsigned char val = 0;

    switch (regnum) {
        case REG_BUTTONS:
            val = buttons;
            break;
    }
    return val;
}

/* Simple protocol based on 1-byte registers:
 * To write a register, master writes 2 <regnum> <value>
 * To read a register, master writes 1 <regnum>, then reads <value>.
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
               reg_set (regnum, i2c_read ());
            } else {
               (void)i2c_read();
            }
            count++;
            break;
        case 3: /* read 1st byte */
        case 4: /* read 2..Nth byte */
            i2c_write (reg_get (regnum));
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
    if (TMR0IE && TMR0IF) {
        ac_interrupt ();
        TMR0IF = 0;
    }
    if (SSPIE && SSPIF) {
        i2c_interrupt ();
        SSPIF = 0;
    }
}

typedef enum { ADC_OFF, ADC_MID, ADC_ON } adc_t;
/* voltages 0-5V scaled to 10b ADC resolution */
#define SCALED_2V  409
#define SCALED_4V  819
adc_t
adc_read (char chs)
{
    int result;
    adc_t res;

    ADCON0bits.CHS = chs;               /* select input channel */
    ADCON0bits.GO_DONE = 1;             /* start conversion */
    while (ADCON0bits.GO_DONE)          /* wait for result */
        NOP ();
    result = (((int)ADRESH)<<8) | ADRESL;
    return (result < SCALED_2V ? ADC_OFF :
            result > SCALED_4V ? ADC_ON : ADC_MID);
}

#define DEBOUNCE_THRESH 10
typedef enum { DB_SETTLING, DB_ON, DB_OFF} db_t;
db_t
port_read_debounce (int *dbcount, char val)
{
    db_t res = DB_SETTLING;

    if (val) {
        if (*dbcount == DEBOUNCE_THRESH)
            res = DB_ON;
        else
            (*dbcount)++;
    } else {
        if (*dbcount == 0)
            res = DB_OFF;
        else
            (*dbcount)--;
    }
    return res;
}

typedef enum { LED_OFF, LED_ON } led_t;
void
port_led_set (led_t val)
{
    LED = (val == LED_OFF ? 1 : 0);
}
void
port_led_toggle (void)
{
    LED = !LED;
}

typedef enum { DEC_OFF, DEC_NORTH, DEC_SOUTH } dec_t;
void
dc_set_dec (dec_t want)
{
    static dec_t cur = DEC_OFF;

    if (cur == want || output_inhibit)
        return;
    switch (want) {
        case DEC_OFF:
            GONORTH = 0;
            NOP ();
            GOSOUTH = 0;
            break;
        case DEC_NORTH:
            GONORTH = 1;
            NOP ();
            GOSOUTH = 0;
            break;
        case DEC_SOUTH:
            GONORTH = 0;
            NOP ();
            GOSOUTH = 1;
            break;
    }
    cur = want;
}

typedef enum { FOC_OFF, FOC_IN, FOC_OUT} focus_t;

void
dc_set_focus (focus_t want)
{
    static focus_t cur = FOC_OFF;

    if (cur == want || output_inhibit)
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
poll_buttons (void)
{
    static int ncount = 0;
    static int ecount = 0;
    char b = 0;

    switch (port_read_debounce (&ncount, !SW_NORTH)) {
        case DB_ON:
            b |= BUTTON_NORTH;
            break;
    }
    switch (adc_read (ADC_EAST)) {
        case ADC_MID:
            b |= BUTTON_LAMP;
            break;
        case ADC_OFF:
            b |= BUTTON_EAST;
            break;
    }
    switch (adc_read (ADC_SOUTH)) {
        case ADC_MID:
            b |= BUTTON_FOCIN;
            break;
        case ADC_OFF:
            b |= BUTTON_SOUTH;
            break;
    }
    switch (adc_read (ADC_WEST)) {
        case ADC_MID:
            b |= BUTTON_FOCOUT;
            break;
        case ADC_OFF:
            b |= BUTTON_WEST;
            break;
    }
    buttons = b;
}

void
action (void)
{
    unsigned char b = buttons | ibuttons;

    if ((b & BUTTON_FOCIN))
        dc_set_focus (FOC_IN);
    else if (b & BUTTON_FOCOUT)
        dc_set_focus (FOC_OUT);
    else
        dc_set_focus (FOC_OFF);

    if (b & BUTTON_NORTH)
        dc_set_dec (DEC_NORTH);
    else if (b & BUTTON_SOUTH)
        dc_set_dec (DEC_SOUTH);
    else
        dc_set_dec (DEC_OFF);

    if (b & BUTTON_EAST)
        ac_set_freq (FREQ_EAST);
    else if (b & BUTTON_WEST)
        ac_set_freq (FREQ_WEST);
    else
        ac_set_freq (FREQ_SIDEREAL);
}

void
indicate(void)
{
    if (output_inhibit) {
        static int counter = 0;
        if (++counter == 500) {
            port_led_toggle ();
            counter = 0;
        }
    } else { 
        if (buttons | ibuttons)
            port_led_set (LED_ON);
        else
            port_led_set (LED_OFF); 
    }
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

    /* ADC input config
     */
    TRISAbits.RA2 = 1;          /* AN3 */
    TRISAbits.RA4 = 1;          /* AN2 */ 
    TRISCbits.RC6 = 1;          /* AN8 */
    ANSELbits.ANS2 = 1;         /* RA2(AN2) will be analog */
    ANSELbits.ANS3 = 1;         /* RA4(AN3) will be analog */
    ANSELHbits.ANS8 = 1;        /* RC6(AN8) will be analog */
    ADCON2bits.ADCS = 6;        /* conv clock = Fosc/64 */
    ADCON1bits.PVCFG = 0;       /* ref to Vdd */
    ADCON1bits.NVCFG = 0;       /* ref to Vss */
    ADCON2bits.ADFM = 1;        /* right justified output fmt */
    ADCON2bits.ACQT = 5;        /* 12 Tad time */
    ADCON0bits.ADON = 1;        /* enable adc */

    /* DC motor config
     */
    PWM = 1;                    /* TODO: enable PWM - for now set high */
    FOCIN = 0;
    FOCOUT = 0;
    GONORTH = 0;
    GOSOUTH = 0;

    /* AC motor config
     */ 
    PHASE1 = 0;
    PHASE2 = 0;
    SQWAVE = 0;
    T0CONbits.T0PS = 2;         /* set prescaler to 1:8 (see genfreq.c) */
    PSA = 0;                    /* assign prescaler */
    T0CS = 0;                   /* use instr cycle clock (CLOCK_FREQ/4) */
    T08BIT = 0;                 /* 16 bit mode */
    TMR0IE = 1;                 /* enable timer0 interrupt */
    TMR0 = freq[ac_freq_now];   /* load count */

    PEIE = 1;                   /* enable peripheral interrupts */
    GIE = 1;                    /* enable global interrupts */
    TMR0ON = 1;                 /* start timer0 */
    for (;;) {
        poll_buttons ();
        action ();
        indicate ();
        __delay_ms(1);
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
