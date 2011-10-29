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

#define ADC_SOUTH       3
#define ADC_WEST        2
#define SW_NORTH        PORTAbits.RA5
#define SW_SOUTH        PORTAbits.RA4   /* read via AN3 */
#define SW_EAST         PORTAbits.RA3
#define SW_WEST         PORTAbits.RA2   /* read via AN2 */
#define PORTA_INPUTS    0b00111100
#define PORTA_PULLUPS   0b00101000      /* external p/u on RA2, RA4 */
#define PORTA_IOC       0

#define SQWAVE          PORTBbits.RB7
#define FOCOUT          PORTBbits.RB5
#define PORTB_INPUTS    0b00000000
#define PORTB_PULLUPS   0b00000000
#define PORTB_IOC       0

#define LED             PORTCbits.RC7
#define PWM             PORTCbits.RC5
#define PHASE1          PORTCbits.RC4
#define PHASE2          PORTCbits.RC3
#define FOCIN           PORTCbits.RC2
#define GONORTH         PORTCbits.RC1
#define GOSOUTH         PORTCbits.RC0
#define PORTC_INPUTS    0b00000000

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

static char freqnow = FREQ_EAST;
static char freqtarg = FREQ_SIDEREAL;
static char adj_ra = 0;
static char adj_dec = 0;
static char adj_focus = 0;
static char output_inhibit = 1;

/* Interrupt is driven by TMR0 at variable rate.  N.B. Although FREQ_EAST
 * is 0Hz, we still run timer, but with AC output inhibited.
 */
void interrupt
isr (void)
{
    static char state = 0;
    static int startup_delay = 0;
    char inhibit = 0;

    if (TMR0IE && TMR0IF) {
        if (startup_delay < 1000)
            startup_delay++;
        else
            output_inhibit = 0;
        if (output_inhibit || freqnow == FREQ_EAST)
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
                if (freqnow < freqtarg)
                    freqnow++;
                else if (freqnow > freqtarg)
                    freqnow--;
                break;
        }
        TMR0 = freq[freqnow] + FUDGE_COUNT;
        TMR0IF = 0;
    }
}

typedef enum { ADC_OFF, ADC_MID, ADC_ON } adc_t;

/* voltages 0-5V scaled to 10b ADC resolution */
#define SCALED_2V  409
#define SCALED_4V  819

adc_t
read_button_adc (char chs)
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
read_button_debounce (int *dbcount, char val)
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
set_dec_motor (dec_t want)
{
    static dec_t cur = DEC_OFF;

    if (cur == want || output_inhibit)
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
set_focus_motor (focus_t want)
{
    static focus_t cur = FOC_OFF;

    if (cur == want || output_inhibit)
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

    n = read_button_debounce (&ncount, !SW_NORTH);      /* dec+ */
    e = read_button_debounce (&ecount, !SW_EAST);       /* ra+ */
    s = read_button_adc (ADC_SOUTH);                    /* focus+/dec- */
    w = read_button_adc (ADC_WEST);                     /* focus-/ra- */

    if (s == ADC_MID && w != ADC_MID)
        set_focus_motor (FOC_MINUS);
    else if (s != ADC_MID && w == ADC_MID)
        set_focus_motor (FOC_PLUS);
    else if (s != ADC_MID && w != ADC_MID)
        set_focus_motor (FOC_OFF);

    if (n == DB_ON && s == ADC_ON)
        set_dec_motor (DEC_NORTH);
    else if (n == DB_OFF && s == ADC_OFF)
        set_dec_motor (DEC_SOUTH);
    else if (n == DB_OFF && s != ADC_OFF)
        set_dec_motor (DEC_OFF);

    if (e == DB_ON && w == ADC_ON) {
        freqtarg = FREQ_EAST;
        adj_ra = 1;
    } else if (e == DB_OFF && w == ADC_OFF) {
        freqtarg = FREQ_WEST;
        adj_ra = 1;
    } else if (e == DB_OFF && w != ADC_OFF) {
        freqtarg = FREQ_SIDEREAL;
        adj_ra = 0;
    }
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
    /* Configure HFOSC for desired system clock frequency.
     */
    OSCCONbits.IRCF = 7;
    //OSCTUNEbits.TUN = 0x1f;

    /* Port configuration
     */
    ANSEL = 0;                  /* disable all analog inputs */
    ANSELH = 0;                 /*  high byte too */
    RABPU = 0;                  /* enable weak pullups on PORTA and B */

    TRISA = PORTA_INPUTS;
    WPUA = PORTA_PULLUPS;       /* enable specific weak pullups */
    IOCA = PORTA_IOC;           /* interrupt on change */
    ANSELbits.ANS2 = 1;         /* RA2(AN2) and RA4(AN3) will be analog */
    ANSELbits.ANS3 = 1;
  
    TRISB = PORTB_INPUTS;
    WPUB = PORTB_PULLUPS;
    IOCB = PORTB_IOC;

    TRISC = PORTC_INPUTS;

    PHASE1 = 0;
    PHASE2 = 0;
    SQWAVE = 0;
    PWM = 1; /* TODO: enable PWM - for now set high */
    FOCIN = 0;
    FOCOUT = 0;
    GONORTH = 0;
    GOSOUTH = 0;
    port_led_set (LED_OFF);

    /* Configure ADC
     */
    ADCON2bits.ADCS = 6;            /* conv clock = Fosc/64 */
    ADCON1bits.PVCFG = 0;           /* ref to Vdd */
    ADCON1bits.NVCFG = 0;           /* ref to Vss */
    ADCON2bits.ADFM = 1;            /* right justified output fmt */
    ADCON2bits.ACQT = 5;            /* 12 Tad time */
    ADCON0bits.ADON = 1;            /* enable adc */

    /* Timer 0 configuration
     */ 
    T0CONbits.T0PS = PRESCALER;     /* set prescaler value */
    PSA = 0;                        /* assign prescaler */
    T0CS = 0;                       /* use instr cycle clock (CLOCK_FREQ/4) */
    T08BIT = 0;                     /* 16 bit mode */
    TMR0IE = 1;                     /* enable timer0 interrupt */
    PEIE = 1;                       /* enable peripheral interrupts */
    GIE = 1;                        /* enable global interrupts */
    TMR0 = freq[freqnow];           /* load count */
    TMR0ON = 1;                     /* start timer */

    for (;;) {
        poll_buttons ();
        indicate ();
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
