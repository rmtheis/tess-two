/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -  This software is distributed in the hope that it will be
 -  useful, but with NO WARRANTY OF ANY KIND.
 -  No author or distributor accepts responsibility to anyone for the
 -  consequences of using this software, or for whether it serves any
 -  particular purpose or works at all, unless he or she says so in
 -  writing.  Everyone is granted permission to copy, modify and
 -  redistribute this source code, for commercial or non-commercial
 -  purposes, with the following restrictions: (1) the origin of this
 -  source code must not be misrepresented; (2) modified versions must
 -  be plainly marked as such; and (3) this notice may not be removed
 -  or altered from any source or modified source distribution.
 *====================================================================*/


/*
 * plottest.c
 *
 *	      plottest
 *
 *     This tests the gplot library functions that generate
 *     the plot commands and data required for input to gnuplot.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "allheaders.h"

    /* for GPLOT_STYLE, use one of the following set:
     *    GPLOT_LINES
     *    GPLOT_POINTS
     *    GPLOT_IMPULSE
     *    GPLOT_LINESPOINTS
     *    GPLOT_DOTS */
#define  GPLOT_STYLE   GPLOT_LINES

    /* for GPLOT_OUTPUT use one of the following set:
     *    GPLOT_PNG
     *    GPLOT_PS
     *    GPLOT_EPS
     *    GPLOT_X11
     *    GPLOT_LATEX
     */
#define  GPLOT_OUTPUT   GPLOT_X11


main(int    argc,
     char **argv)
{
char        *str1, *str2;
l_int32      i, nbytes1, nbytes2;
l_float32    x, y1, y2, pi;
GPLOT       *gplot1, *gplot2, *gplot3;
NUMA        *nax, *nay1, *nay2;
static char  mainName[] = "plottest";

    if (argc != 1)
	exit(ERROR_INT(" Syntax:  plottest", mainName, 1));

	/* Generate plot data */
    nax = numaCreate(0);
    nay1 = numaCreate(0);
    nay2 = numaCreate(0);
    pi = 3.1415926535;
    for (i = 0; i < 180; i++) {
	x = (pi / 180.) * i;
	y1 = (l_float32)sin(2.4 * x);
	y2 = (l_float32)cos(2.4 * x);
	numaAddNumber(nax, x);
	numaAddNumber(nay1, y1);
	numaAddNumber(nay2, y2);
    }

	/* Show the plot */
    gplot1 = gplotCreate("/tmp/junkplotroot1", GPLOT_OUTPUT, "Example plots",
			 "theta", "f(theta)");
    gplotAddPlot(gplot1, nax, nay1, GPLOT_STYLE, "sin (2.4 * theta)");
    gplotAddPlot(gplot1, nax, nay2, GPLOT_STYLE, "cos (2.4 * theta)");
    gplotMakeOutput(gplot1);

        /* Also save the plot to png */
    gplot1->outformat = GPLOT_PNG;
    gplot1->outname = stringNew("/tmp/junkplotroot1.png");
    gplotMakeOutput(gplot1);

        /* Test gplot serialization */
    gplotWrite("/tmp/junkgplot1.plt", gplot1);
    if ((gplot2 = gplotRead("/tmp/junkgplot1.plt")) == NULL)
        exit(ERROR_INT("gplotRead failure!", mainName, 1));
    gplotWrite("/tmp/junkgplot2.plt", gplot2);

        /* Are the two written gplot files the same? */
    str1 = (char *)arrayRead("/tmp/junkgplot1.plt", &nbytes1);
    str2 = (char *)arrayRead("/tmp/junkgplot2.plt", &nbytes2);
    if (nbytes1 != nbytes2)
        fprintf(stderr, "Error: nbytes1 = %d, nbytes2 = %d\n",
                nbytes1, nbytes2);
    else
        fprintf(stderr, "Correct: nbytes1 = nbytes2 = %d\n", nbytes1);
    if (strcmp(str1, str2))
        fprintf(stderr, "Error: str1 != str2\n");
    else
        fprintf(stderr, "Correct: str1 == str2\n");
    numaDestroy(&nax);
    numaDestroy(&nay1);
    numaDestroy(&nay2);
    gplotDestroy(&gplot1);
    gplotDestroy(&gplot2);
    FREE(str1);
    FREE(str2);

        /* Read from file and regenerate the plot */
    gplot3 = gplotRead("/tmp/junkgplot2.plt");
    gplot3->outformat = GPLOT_X11;
    gplotMakeOutput(gplot3);
    gplotDestroy(&gplot3);
    return 0;
}

