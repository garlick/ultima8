/* genfreq.c - calculate timer count values */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

#define ALCOR_LUNAR_50 48.912
#define ALCOR_LUNAR_60 58.696

#define CLOCK_FREQ 64000000
#define PRESCALER_RATIO 8

double
mfreq (int count, double countfreq)
{
    double intrfreq = countfreq/(255*255 - count + 1);

    return intrfreq/4;
}

int
main (int argc, char *argv[])
{
	int i, j;
	double targ[] = {
		0.25*60,	/* 0 */
		0.25*60,
		0.5*60,
		0.75*60,
		ALCOR_LUNAR_60,	/* 4 lunar 60hz */
                60,		/* 5 sidereal 60hz */
		1.25*60,
		1.5*60,
		1.75*60,
		2*60,		/* 9 */
		ALCOR_LUNAR_50,	/* 10 lunar 50hz */
		50,		/* 11 sidereal 50hz */
	 };
	int val[12] = { 0,0,0,0,0,0,0,0,0,0,0 };
	double prescaler = PRESCALER_RATIO;
	double countfreq = (double)CLOCK_FREQ/(4*prescaler);

	for (i = 0; i < 256*256; i++) {
		for (j = 0; j < 12; j++) {
			double d = fabs (mfreq(val[j], countfreq) - targ[j]);

			if (d > fabs (mfreq(i, countfreq) - targ[j]))
				val[j] = i;
		}
	}

	printf ("/*\n");
	printf("speed\ttarget\t\tcount\terror\n");
	for (j = 0; j < 12; j++)
		printf ("%d:\t%f\t%d\t%+f\t%s\n", j, targ[j], val[j],
			mfreq(val[j], countfreq) - targ[j],
			j == 4 ? "60hz lunar" :
			j == 5 ? "60hz sidereal" :
			j == 10 ? "50hz lunar" :
			j == 11 ? "50hz sidereal" : "");
	printf ("*/\n\n");

	printf ("/* generated with genfreq - DO NOT EDIT */\n",
		prescaler, CLOCK_FREQ);
#if (MOTOR_HZ == 60)
	printf ("int freq[] = {");
	for (j = 0; j < 10; j++)
		printf ("%d%s", val[j], j < 9 ? ", " : "};\n");
#elif (MOTOR_HZ == 50)
	printf ("int freq[] = {");
	for (j = 0; j < 4; j++)
		printf ("%d, ", val[j]);
	printf ("%d, ", val[10]);
	printf ("%d, ", val[11]);
	for (j = 6; j < 10; j++)
		printf ("%d%s", val[j], j < 9 ? ", " : "};\n");
#else
#error unsupported MOTOR_HZ
#endif
	exit (0);
}
