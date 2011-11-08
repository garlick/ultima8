/* genfreq.c - calculate timer count values */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

#if (MOTOR_HZ == 60)
#define LUNAR_HZ       58.696
#define SIDEREAL_HZ    60.0
#elif (MOTOR_HZ == 50)
#define LUNAR_HZ       48.912
#define SIDEREAL_HZ    50.0
#endif

#define _XTAL_FREQ 64000000
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
		0.15*SIDEREAL_HZ,
		0.20*SIDEREAL_HZ,
		0.25*SIDEREAL_HZ,
		0.30*SIDEREAL_HZ,
		0.35*SIDEREAL_HZ,
		0.40*SIDEREAL_HZ,
		0.45*SIDEREAL_HZ,
		0.50*SIDEREAL_HZ,
		0.55*SIDEREAL_HZ,
		0.60*SIDEREAL_HZ,
		0.65*SIDEREAL_HZ,
		0.70*SIDEREAL_HZ,
		0.75*SIDEREAL_HZ,
		0.80*SIDEREAL_HZ,
		0.85*SIDEREAL_HZ,
		0.90*SIDEREAL_HZ,
		0.95*SIDEREAL_HZ,
		LUNAR_HZ,
                SIDEREAL_HZ,
		1.05*SIDEREAL_HZ,
		1.10*SIDEREAL_HZ,
		1.15*SIDEREAL_HZ,
		1.20*SIDEREAL_HZ,
		1.25*SIDEREAL_HZ,
		1.30*SIDEREAL_HZ,
		1.35*SIDEREAL_HZ,
		1.40*SIDEREAL_HZ,
		1.45*SIDEREAL_HZ,
		1.50*SIDEREAL_HZ,
		1.55*SIDEREAL_HZ,
		1.60*SIDEREAL_HZ,
		1.65*SIDEREAL_HZ,
		1.70*SIDEREAL_HZ,
		1.75*SIDEREAL_HZ,
		1.80*SIDEREAL_HZ,
		1.85*SIDEREAL_HZ,
		1.90*SIDEREAL_HZ,
		1.95*SIDEREAL_HZ,
		2.0*SIDEREAL_HZ,
	 };
	int val[sizeof(targ)/sizeof(targ[0])];
	double prescaler = PRESCALER_RATIO;
	double countfreq = (double)_XTAL_FREQ/(4*prescaler);

	for (j = 0; j < sizeof(targ)/sizeof(targ[0]); j++)
		val[j] = 0;
	for (i = 0; i < 256*256; i++) {
		for (j = 0; j < sizeof(targ)/sizeof(targ[0]); j++) {
			double d = fabs (mfreq(val[j], countfreq) - targ[j]);

			if (d > fabs (mfreq(i, countfreq) - targ[j]))
				val[j] = i;
		}
	}

	printf ("/* generated with genfreq - DO NOT EDIT */\n");
	printf ("UINT16 freq[] = {\n");
	printf("/*\tcount\t   target(Hz)\terror */\n");
	for (j = 0; j < sizeof(targ)/sizeof(targ[0]); j++)
		printf ("/*%d*/\t%d,\t/* %f\t%+f */%s\n", j, val[j],
			targ[j], mfreq(val[j], countfreq) - targ[j],
			targ[j] == LUNAR_HZ ? " /* lunar rate */" :
			targ[j] == SIDEREAL_HZ ? " /* sidereal rate */" : "");
	printf ("};\n");
	printf ("#define FREQ_EAST	0\n");
	for (j = 0; j < sizeof(targ)/sizeof(targ[0]); j++) {
       		if (targ[j] == LUNAR_HZ)
			printf ("#define FREQ_LUNAR	%d\n", j);
		else if (targ[j] == SIDEREAL_HZ)
			printf ("#define FREQ_SIDEREAL	%d\n", j);
	}
	printf ("#define FREQ_WEST	%d\n", j - 1);
	exit (0);
}
