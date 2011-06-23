/* timer.c - calculate timer count values */

#include <stdio.h>
#include <math.h>

#define _XTAL_FREQ 4000000
#define PRESCALER 64

#define ALCOR_LUNAR_50 48.912
#define ALCOR_LUNAR_60 58.696

double
intfreq (int n)
{
    return (double)_XTAL_FREQ / (4*PRESCALER*((255-n)+1));
}

double
motorfreq (int n)
{
    return intfreq(n)/4;
}

int
main (int argc, char *argv[])
{
	int i, j;
	double targ[] = {
		0.25*60, /* 0 */
		0.25*60,
		0.5*60,
		0.75*60,
		ALCOR_LUNAR_60, /* 4 */
                60, /* 5 */
		1.25*60,
		1.5*60,
		1.75*60,
		2*60, /* 9 */
		ALCOR_LUNAR_50, /* 50Hz */
		50,  /* 50 Hz */
	 };
	int val[12] = { 0,0,0,0,0,0,0,0,0,0,0 };

	for (i = 0; i < 256; i++) {
		for (j = 0; j < 12; j++) {
			double d = fabs (motorfreq(val[j]) - targ[j]);

			if (d > fabs (motorfreq(i) - targ[j]))
				val[j] = i;
		}
	}

	printf("speed\ttarget\t\tcount\terror\n");
	for (j = 0; j < 12; j++)
		printf ("%d:\t%f\t%d\t%+f\t%s\n", j, targ[j], val[j],
			motorfreq(val[j]) - targ[j],
			j == 4 ? "60hz lunar" :
			j == 5 ? "60hz sidereal" :
			j == 10 ? "50hz lunar" :
			j == 11 ? "50hz sidereal" : "");
}

