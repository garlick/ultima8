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

/* NOTE: compiled with hi-tech C pro V9.66 */

#include <htc.h>

#if defined(_18F14K22)
#if (CLOCK_FREQ == 64000000)
__CONFIG (1, FOSC_IRC & PLLEN_ON);  /* system clock is HFOSC*4 */
#elif (CLOCK_FREQ == 16000000)
__CONFIG (1, FOSC_IRC);
#endif
__CONFIG (2, BOREN_OFF & WDTEN_OFF);
__CONFIG (3, HFOFST_OFF);
#else
#error Config bits may need attention for non-18F14K22 chip.
#endif

#define SW_NORTH	RA5
#define SW_EAST		RA4
#define PORTA_INPUTS	0b00110000
#define PORTA_PULLUPS	0b00110000

#define SQWAVE		RB7
#define FOCOUT		RB5
#define PORTB_INPUTS	0b00000000
#define PORTB_PULLUPS	0b00000000

#define SW_SOUTH	RC7 // FIXME use AN9 to see focus+
#define SW_WEST		RC6 // FIXME use AN8 to see focus-
#define PWM		RC5 // FIXME had to PWM module once that works
#define PHASE1		RC4
#define PHASE2		RC3
#define FOCIN		RC2
#define GONORTH		RC1
#define GOSOUTH		RC0
#define PORTC_INPUTS	0b11000000

/* generated with ./genfreq 8 (CLOCK_FREQ=64000000) */
int freq60[] = {31693, 31693, 48359, 53915, 56508, 56693, 58359, 59470, 60264, 60859};
int freq50[] = {31693, 31693, 48359, 53915, 54804, 55026, 58359, 59470, 60264, 60859};
#define FUDGE_COUNT 500 /* FIXME shouldn't need this */

#if (CLOCK_FREQ == 64000000)
#define PRESCALER	2 /* 1:8 */
#define IRCF_VAL	7
#elif (CLOCK_FREQ == 16000000)
#define PRESCALER	0 /* 1:2 */
#define IRCF_VAL	7
#else
#error unsupported CLOCK_FREQ
#endif

#define FREQ_EAST	0 /* special:  turn off motor, but leave timer on */
#define FREQ_LUNAR	4
#define FREQ_SIDEREAL	5
#define FREQ_WEST	9

#if (MOTOR_HZ == 60)
#define freq freq60
#elif (MOTOR_HZ == 50)
#define freq freq50
#else
#error unsupported MOTOR_HZ
#endif

char freqnow = FREQ_EAST;
char freqtarg = FREQ_SIDEREAL;

/* FIXME: should be configurable via I2C */
char swapew = SWAPEW;
char swapns = SWAPNS;
char lunar = 0;

void interrupt
isr (void)
{
    static char state = 0;

    if (TMR0IE && TMR0IF) {
	switch (state) {
            case 0:
                if (freqnow != FREQ_EAST) {
                    SQWAVE = 1;
                    PHASE1 = 1;
                }
                state = 1;
                break;
            case 1:
                if (freqnow != FREQ_EAST)
                    PHASE1 = 0;
                state = 2;
                break;
            case 2:
                if (freqnow != FREQ_EAST) {
                    SQWAVE = 0;
                    PHASE2 = 1;
                }
                state = 3;
                break;
            case 3:
                if (freqnow != FREQ_EAST)
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

void
main(void)
{
    /* Configure HFOSC for desired system clock frequency.
     */
    OSCCONbits.IRCF = IRCF_VAL;
    //OSCTUNEbits.TUN = 0x1f;

    /* Port configuration
     */
    TRISA = PORTA_INPUTS;
    WPUA = PORTA_PULLUPS;
    TRISB = PORTB_INPUTS;
    WPUB = PORTB_PULLUPS;
    TRISC = PORTC_INPUTS;

    PHASE1 = 0;
    PHASE2 = 0;
    SQWAVE = 0;
    PWM = 1;

    /* Timer 0 configuration
     */ 
    T0CONbits.T0PS = PRESCALER;	/* set prescaler value */
    PSA = 0;			/* assign prescaler */
    T0CS = 0;			/* use instr cycle clock (CLOCK_FREQ/4) */
    T08BIT = 0;			/* 16 bit mode */
    TMR0IE = 1;         	/* enable timer0 interrupt */
    PEIE = 1;			/* enable peripheral interrupts */
    GIE = 1;			/* enable global interrupts */
    TMR0 = freq[freqnow]; 	/* load count */
    TMR0ON = 1;			/* start timer */

    /* Poll for switch closures (active low).
     */
    for (;;) {
        /* RA */
        if (!SW_EAST == 0 && SW_WEST)
            freqtarg = swapew ? FREQ_WEST : FREQ_EAST;
        else if (!SW_WEST && SW_EAST)
            freqtarg = swapew ? FREQ_EAST : FREQ_WEST;
        else
            freqtarg = lunar ? FREQ_LUNAR : FREQ_SIDEREAL;

        /* DEC */
        if (!SW_NORTH && SW_SOUTH) {
            if (swapns) {
                GONORTH = 0;
                GOSOUTH = 1;
            } else {
                GONORTH = 1;
                GOSOUTH = 0;
            }
        } else if (!SW_SOUTH && SW_NORTH) {
            if (swapns) {
                GONORTH = 1;
                GOSOUTH = 0;
            } else {
                GONORTH = 0;
                GOSOUTH = 1;
            } 
        } else {
            GONORTH = 0;
            GOSOUTH = 0;
        }
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
