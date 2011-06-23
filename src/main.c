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

#include <htc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(_16F84A)
__CONFIG (FOSC_XT & WDTE_OFF & PWRTE_ON);
#else
#error Config bits and other stuff may need attention for non-16F84A chip.
#endif

/* outputs - active high */
#define GONORTH         RA0	/* dec motor N */
#define GOSOUTH		RA1	/* dec motor S */
#define PHASE1		RA2	/* ac motor */
#define PHASE2		RA3	/* ac motor */
#define SQWAVE		RA4	/* square wave (test point) */
#define PORTA_INPUTS    0b00000000

/* inputs - active low */
#define SW_NORTH	RB0	/* correct +north */
#define SW_SOUTH	RB1	/* correct +south */
#define SW_EAST		RB2	/* correct +east */
#define SW_WEST		RB3	/* correct +west */
#define SW_LUNAR	RB4	/* select lunar rate */
#define SW_50HZ		RB5     /* select 50hz */
#define SW_SWAPNS	RB6     /* swap SW_NORTH and SW_SOUTH */
#define SW_SWAPEW	RB7	/* swap SW_EAST and SW_WEST */
#define PORTB_INPUTS    0b11111111

/* Timer counts determined programmatically with ../util/timer.c,
 * which presumes 4mhz clock and 1:64 prescaler.
 */
char freq60[] = { 0, 0, 126, 169, 189, 191, 204, 213, 219, 223};
char freq50[] = { 0, 0, 126, 169, 176, 178, 204, 213, 219, 223};

#define FREQ_EAST	0 /* special:  turn off motor, but leave timer on */
#define FREQ_LUNAR	4
#define FREQ_SIDEREAL	5
#define FREQ_WEST	9

char freqnow = FREQ_EAST;
char freqtarg = FREQ_SIDEREAL;

void interrupt
isr (void)
{
    static char state = 0;

    if (T0IF) {
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
                /* Cycle completed, reload timer with different count?
                 */
		if (freqnow < freqtarg) {
                    freqnow++;
		    TMR0 = !SW_50HZ ? freq50[freqnow] : freq60[freqnow];
		} else if (freqnow > freqtarg) {
                    freqnow--;
		    TMR0 = !SW_50HZ ? freq50[freqnow] : freq60[freqnow];
                }
                break;
        }
        T0IF = 0;
    }
}

void
main(void)
{
    /* Timer configuration
     */ 
    PSA = 0;		/* enable prescaler */
    OPTION_REGbits.PS = 5;/* select 64:1 */
    T0CS = 0;		/* select internal clock */
    TMR0 = freq60[freqnow];

    /* Port configuration
     */
    TRISA = PORTA_INPUTS;
    TRISB = PORTB_INPUTS;
    nRBPU = 0;		/* use internal pull-ups */
    PHASE1 = 0;
    PHASE2 = 0;
    SQWAVE = 0;

    /* Interrupt configuration
     */
    T0IE = 1;		/* enable interrupt on T0 overflow */
    INTE = 0;		/* disable external interrupt */
    GIE = 1;            /* enable global interrupts */

    /* Poll for switch closures (active low).
     */
    for (;;) {
        /* RA */
        if (!SW_EAST == 0 && SW_WEST)
            freqtarg = !SW_SWAPEW ? FREQ_WEST : FREQ_EAST;
        else if (!SW_WEST && SW_EAST)
            freqtarg = !SW_SWAPEW ? FREQ_EAST : FREQ_WEST;
        else
            freqtarg = !SW_LUNAR ? FREQ_LUNAR : FREQ_SIDEREAL;

        /* DEC */
        if (!SW_NORTH && SW_SOUTH) {
            if (!SW_SWAPNS) {
                GONORTH = 0;
                GOSOUTH = 1;
            } else {
                GONORTH = 1;
                GOSOUTH = 0;
            }
        } else if (!SW_SOUTH && SW_NORTH) {
            if (!SW_SWAPNS) {
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
