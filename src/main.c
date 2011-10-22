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
#if (CLOCK_FREQ == 64000000)
__CONFIG (1, FOSC_IRC & PLLEN_ON);  /* system clock is HFOSC*4 */
#elif (CLOCK_FREQ == 16000000)
__CONFIG (1, FOSC_IRC);
#endif
__CONFIG (2, BOREN_OFF & WDTEN_OFF);
__CONFIG (3, HFOFST_OFF & MCLRE_OFF);
__CONFIG (4, LVP_OFF);
#else
#error Config bits may need attention for non-18F14K22 chip.
#endif

#define SW_NORTH        PORTAbits.RA5
#define SW_SOUTH        PORTAbits.RA4   // FIXME: use ADC for focus button
#define SW_EAST         PORTAbits.RA3
#define SW_WEST         PORTAbits.RA2   // FIXME: use ADC for focus button
#define PORTA_INPUTS    0b00111100
#define PORTA_PULLUPS   0b00101000      // external p/u on RA2, RA4
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

#if (CLOCK_FREQ == 64000000)
#define PRESCALER       2 /* 1:8 */
#define IRCF_VAL        7
#elif (CLOCK_FREQ == 16000000)
#define PRESCALER       0 /* 1:2 */
#define IRCF_VAL        7
#else
#error unsupported CLOCK_FREQ
#endif

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
static char decadj = 0;
static char focusadj = 0;

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
        if (startup_delay < 500) {
            inhibit = 1;
            startup_delay++;
        }
        if (freqnow == FREQ_EAST)
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

#define DEBOUNCE_THRESH 100
typedef enum { DB_SETTLING, DB_ON, DB_OFF} db_t;

db_t
poll_button_debounce (char *dbcount, char val)
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

void
poll_buttons_ra (void)
{
    static char ecount = 0;
    static char wcount = 0;
    db_t e, w;

    e = poll_button_debounce (&ecount, !SW_EAST);
    w = poll_button_debounce (&wcount, !SW_WEST);
    if (e == DB_SETTLING || w == DB_SETTLING)
        return;
    if (e == DB_ON && w == DB_OFF)
        freqtarg = FREQ_EAST;
    else if (w == DB_ON && e == DB_OFF)
        freqtarg = FREQ_WEST;
    else
        freqtarg = FREQ_SIDEREAL;
}

typedef enum { DEC_OFF, DEC_NORTH, DEC_SOUTH } dec_t;

void
set_dec_motor (dec_t want)
{
    static dec_t cur = DEC_OFF;

    if (cur == want)
        return;
    switch (want) {
        case DEC_OFF:
            GONORTH = 0;
            NOP ();
            GOSOUTH = 0;
            decadj = 0;
            break;
        case DEC_NORTH:
            GONORTH = 1;
            NOP ();
            GOSOUTH = 0;
            decadj = 1;
            break;
        case DEC_SOUTH:
            GONORTH = 0;
            NOP ();
            GOSOUTH = 1;
            decadj = 1;
            break;
    }
    cur = want;
}

void
poll_buttons_dec (void)
{
    static char ncount = 0;
    static char scount = 0;
    db_t n, s;

    n = poll_button_debounce (&ncount, !SW_NORTH);
    s = poll_button_debounce (&scount, !SW_SOUTH);
    if (n == DB_SETTLING || s == DB_SETTLING)
        return;
    if (n == DB_ON && s == DB_OFF)
        set_dec_motor (DEC_NORTH);
    else if (s == DB_ON && n == DB_OFF)
        set_dec_motor (DEC_SOUTH);
    else {
        set_dec_motor (DEC_OFF);
    }
}

void
indicate(void)
{
    if (freqnow != FREQ_SIDEREAL || decadj || focusadj)
        LED = 0;
    else
        LED = 1;
}

void
main(void)
{
    /* Configure HFOSC for desired system clock frequency.
     */
    OSCCONbits.IRCF = IRCF_VAL;
    //OSCTUNEbits.TUN = 0x1f;

    /* Port configuration
     */
    ANSEL = 0;                  /* disable all analog inputs */
    ANSELH = 0;                 /*  high byte too */
    RABPU = 0;                  /* enable weak pullups on PORTA and B */
    //RABIE = 1;                /* enable interrupt on change PORTA and B */

    TRISA = PORTA_INPUTS;
    WPUA = PORTA_PULLUPS;       /* enable specific weak pullups */
    IOCA = PORTA_IOC;           /* interrupt on change */

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
    LED = 1; /* LED off */

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
        poll_buttons_ra ();
        poll_buttons_dec ();
        indicate ();
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
