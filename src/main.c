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
#define SW_NORTH        PORTAbits.RA5
#define SW_SOUTH        PORTAbits.RA4   /* read via AN3 */
#define SW_EAST         PORTAbits.RA3
#define SW_WEST         PORTAbits.RA2   /* read via AN2 */
#define PORTA_INPUTS    0b00111100
#define PORTA_PULLUPS   0b00101000      /* external p/u on RA2, RA4 */

#define SQWAVE          PORTBbits.RB7
#define FOCOUT          PORTBbits.RB5

#define LED             PORTCbits.RC7
#define PWM             PORTCbits.RC5
#define PHASE1          PORTCbits.RC4
#define PHASE2          PORTCbits.RC3
#define FOCIN           PORTCbits.RC2
#define GONORTH         PORTCbits.RC1
#define GOSOUTH         PORTCbits.RC0

/* generated with ./genfreq 8 (CLOCK_FREQ=64000000) */
int freq60[] = {31693, 31693, 48359, 53915, 56508, 56693, 58359, 59470, 60264, 60859};
int freq50[] = {31693, 31693, 48359, 53915, 54804, 55026, 58359, 59470, 60264, 60859};
#define FUDGE_COUNT 500 /* FIXME shouldn't need this */

#define PRESCALER       2 /* 1:8 */

#define FREQ_EAST       0 /* special:  turn off motor, but leave timer on */
#define FREQ_LUNAR      4
#define FREQ_SIDEREAL   5
#define FREQ_WEST       9

#if (MOTOR_HZ == 60)
#define freq freq60
#elif (MOTOR_HZ == 50)
#define freq freq50
#else
#error unsupported MOTOR_HZ
#endif

static char adj_ra = 0;
static char adj_dec = 0;
static char adj_focus = 0;

static char output_inhibit = 1;

static char ac_freq_now = FREQ_EAST;
static char ac_freq_targ = FREQ_SIDEREAL;

void
ac_set_freq (int freq)
{
    ac_freq_targ = freq;
    if (ac_freq_targ != FREQ_SIDEREAL)
        adj_ra = 1;
    else
        adj_ra = 0;
}

void
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

/* See AN734 for I2C slave state descriptions
 */
void
i2c_interrupt (void)
{
    static unsigned char data[16] = { 42, 43, 44 };
    static unsigned char count = 0;
    unsigned char dummy;

    // State 1: I2C write operation, last byte was an address byte
    // SSPSTAT bits: S = 1, D_A = 0, R_W = 0, BF = 1
    if (SSPSTATbits.S == 1 && SSPSTATbits.D_A == 0
                           && SSPSTATbits.R_W == 0
                           && SSPSTATbits.BF == 1) {
        count = 0;
        dummy = SSPBUF; /* read address and discard */

    // State 2: I2C write operation, last byte was a data byte
    // SSPSTAT bits: S = 1, D_A = 1, R_W = 0, BF = 1
    } else if (SSPSTATbits.S == 1 && SSPSTATbits.D_A == 1
                                  && SSPSTATbits.R_W == 0
                                  && SSPSTATbits.BF == 1) {
        if (count < sizeof(data)/sizeof(data[0]))
            data[count++] = SSPBUF;

    // State 3: I2C read operation, last byte was an address byte
    // SSPSTAT bits: S = 1, D_A = 0, R_W = 1
    } else if (SSPSTATbits.S == 1 && SSPSTATbits.D_A == 0
                                  && SSPSTATbits.R_W == 1) {
        count = 0;
        SSPBUF = data[count++];
        SSPCON1bits.CKP = 1;

    // State 4: I2C read operation, last byte was a data byte
    // SSPSTAT bits: S = 1, D_A = 1, R_W = 1, BF = 0
    } else if (SSPSTATbits.S == 1 && SSPSTATbits.D_A == 1
                                  && SSPSTATbits.R_W == 1
                                  && SSPSTATbits.BF == 0) {
       
        if (count < sizeof(data)/sizeof(data[0])) {
            SSPBUF = data[count++];
            SSPCON1bits.CKP = 1;
        }
    
    // State 5: Slave I2C logic reset by NACK from master
    // SSPSTAT bits: S = 1, D_A = 1, BF = 0, CKP = 1
    } else if (SSPSTATbits.S == 1 && SSPSTATbits.D_A == 1
                                  && SSPSTATbits.BF == 0
                                  && SSPCON1bits.CKP == 1) {
        count = 0;
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

#define DEBOUNCE_THRESH 100
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

typedef enum { DEC_OFF, DEC_NORTH, DEC_SOUTH } dec_t;
void
dc_set_dec (dec_t want)
{
    static dec_t cur = DEC_OFF;

    if (cur == want)
        return;
    switch (want) {
        case DEC_OFF:
            GONORTH = 0;
            NOP ();
            GOSOUTH = 0;
            adj_dec = 0;
            break;
        case DEC_NORTH:
            GONORTH = 1;
            NOP ();
            GOSOUTH = 0;
            adj_dec = 1;
            break;
        case DEC_SOUTH:
            GONORTH = 0;
            NOP ();
            GOSOUTH = 1;
            adj_dec = 1;
            break;
    }
    cur = want;
}

typedef enum { FOC_OFF, FOC_PLUS, FOC_MINUS} focus_t;

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
            adj_focus = 0;
            break;
        case FOC_PLUS:
            FOCIN = 1;
            NOP ();
            FOCOUT = 0;
            adj_focus = 1;
            break;
        case FOC_MINUS:
            FOCIN = 0;
            NOP ();
            FOCOUT = 1;
            adj_focus = 1;
            break;
    }
    cur = want;
}

void
poll_buttons (void)
{
    static int ncount = 0;
    static int ecount = 0;
    db_t n, e;
    adc_t s, w;

    n = port_read_debounce (&ncount, !SW_NORTH);      /* dec+ */
    e = port_read_debounce (&ecount, !SW_EAST);       /* ra+ */
    s = adc_read (ADC_SOUTH);                    /* focus+/dec- */
    w = adc_read (ADC_WEST);                     /* focus-/ra- */

    if (s == ADC_MID && w != ADC_MID)
        dc_set_focus (FOC_MINUS);
    else if (s != ADC_MID && w == ADC_MID)
        dc_set_focus (FOC_PLUS);
    else if (s != ADC_MID && w != ADC_MID)
        dc_set_focus (FOC_OFF);

    if (n == DB_ON && s == ADC_ON)
        dc_set_dec (DEC_NORTH);
    else if (n == DB_OFF && s == ADC_OFF)
        dc_set_dec (DEC_SOUTH);
    else if (n == DB_OFF && s != ADC_OFF)
        dc_set_dec (DEC_OFF);

    if (e == DB_ON && w == ADC_ON)
        ac_set_freq (FREQ_EAST);
    else if (e == DB_OFF && w == ADC_OFF)
        ac_set_freq (FREQ_WEST);
    else if (e == DB_OFF && w != ADC_OFF)
        ac_set_freq (FREQ_SIDEREAL);
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

void
indicate(void)
{
    if (output_inhibit) {
        static int counter = 0;
        if (++counter == 5000) {
            port_led_toggle ();
            counter = 0;
        }
    } else { 
        if (adj_dec || adj_focus || adj_ra)
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
    SSPCON1bits.SSPEN = 1;      /* enable MSSP peripheral */
    SSPCON1bits.SSPM = 6;       /* I2C slave mode, 7-bit address */
    SSPADD = I2C_ADDR << 1;     /* set address */
    SSPIE = 1;                  /* enable SSP interrupt */

    /* Digital input config
     */
    //TRISAbits.RA3 = 1;        /* RA3 is input (the only option) */
    RABPU = 0;                  /* enable weak pullups feature */
    WPUAbits.WPUA3 = 1;         /* pullup on RA3 */
    TRISAbits.RA5 = 1;          /* RA5 is input */
    WPUAbits.WPUA5 = 1;         /* pullup on RA5 */

    /* ADC input config
     */
    TRISAbits.RA2 = 1;          /* AN3 */
    TRISAbits.RA4 = 1;          /* AN2 */ 
    ANSELbits.ANS2 = 1;         /* RA2(AN2) and RA4(AN3) will be analog */
    ANSELbits.ANS3 = 1;
    ADCON2bits.ADCS = 6;        /* conv clock = Fosc/64 */
    ADCON1bits.PVCFG = 0;       /* ref to Vdd */
    ADCON1bits.NVCFG = 0;       /* ref to Vss */
    ADCON2bits.ADFM = 1;        /* right justified output fmt */
    ADCON2bits.ACQT = 5;        /* 12 Tad time */
    ADCON0bits.ADON = 1;        /* enable adc */

    /* DC motor config
     */
    PWM = 1; /* TODO: enable PWM - for now set high */
    FOCIN = 0;
    FOCOUT = 0;
    GONORTH = 0;
    GOSOUTH = 0;

    /* AC motor config
     */ 
    PHASE1 = 0;
    PHASE2 = 0;
    SQWAVE = 0;
    T0CONbits.T0PS = PRESCALER;     /* set prescaler value */
    PSA = 0;                        /* assign prescaler */
    T0CS = 0;                       /* use instr cycle clock (CLOCK_FREQ/4) */
    T08BIT = 0;                     /* 16 bit mode */
    TMR0IE = 1;                     /* enable timer0 interrupt */
    TMR0 = freq[ac_freq_now];       /* load count */

    PEIE = 1;                       /* enable peripheral interrupts */
    GIE = 1;                        /* enable global interrupts */
    TMR0ON = 1;                     /* start timer0 */
    for (;;) {
        poll_buttons ();
        indicate ();
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
