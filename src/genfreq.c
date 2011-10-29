/* genfreq.c - calculate timer count values */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

#define ALCOR_LUNAR_50 48.912
#define ALCOR_LUNAR_60 58.696

#define CLOCK_FREQ 64000000

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
	double prescaler;
	double countfreq;

	if (argc != 2) {
		fprintf (stderr, "Usage: genfreq prescaler\n");
		exit (1);
	}
	prescaler = strtod (argv[1], NULL);
	if (prescaler != 2 && prescaler != 4 && prescaler != 8 &&
	    prescaler != 16 && prescaler != 32 && prescaler != 64 &&
	    prescaler != 128 && prescaler != 256) {
		fprintf (stderr, "genfreq: prescaler must be 2,4,8,16,32,64,128,256\n");
		exit (1);
	}
        countfreq = (double)CLOCK_FREQ/(4*prescaler);

	for (i = 0; i < 256*256; i++) {
		for (j = 0; j < 12; j++) {
			double d = fabs (mfreq(val[j], countfreq) - targ[j]);

			if (d > fabs (mfreq(i, countfreq) - targ[j]))
				val[j] = i;
		}
	}

	printf("speed\ttarget\t\tcount\terror\n");
	for (j = 0; j < 12; j++)
		printf ("%d:\t%f\t%d\t%+f\t%s\n", j, targ[j], val[j],
			mfreq(val[j], countfreq) - targ[j],
			j == 4 ? "60hz lunar" :
			j == 5 ? "60hz sidereal" :
			j == 10 ? "50hz lunar" :
			j == 11 ? "50hz sidereal" : "");

	printf ("/* generated with ./genfreq %.0f (CLOCK_FREQ=%d) */\n",
		prescaler, CLOCK_FREQ);
	printf ("int freq60[] = {");
	for (j = 0; j < 10; j++)
		printf ("%d%s", val[j], j < 9 ? ", " : "};\n");

	printf ("int freq50[] = {");
	for (j = 0; j < 4; j++)
		printf ("%d, ", val[j]);
	printf ("%d, ", val[10]);
	printf ("%d, ", val[11]);
	for (j = 6; j < 10; j++)
		printf ("%d%s", val[j], j < 9 ? ", " : "};\n");
	printf ("#define PRESCALER\t%d /* 1:%.0f */\n",
		prescaler == 2 ? 0 :
		prescaler == 4 ? 1 :
		prescaler == 8 ? 2 :
		prescaler == 16 ? 3 :
		prescaler == 32 ? 4 :
		prescaler == 64 ? 5 :
		prescaler == 128 ? 6 : 7,
		prescaler);
}
