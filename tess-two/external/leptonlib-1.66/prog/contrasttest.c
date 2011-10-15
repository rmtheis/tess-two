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
 * contrasttest.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allheaders.h"

#define   NPLOTS       5

main(int    argc,
     char **argv)
{
char        *filein, *fileout;
char         bigbuf[512];
l_int32      iplot;
l_float32    factor;    /* scaled width of atan curve */
l_float32    fact[NPLOTS] = {.2, 0.4, 0.6, 0.8, 1.0};
GPLOT       *gplot;
NUMA        *na, *nax;
PIX         *pixs, *pixd;
static char  mainName[] = "contrasttest";

    if (argc != 4)
	exit(ERROR_INT(" Syntax:  contrasttest filein factor fileout",
	    mainName, 1));

    filein = argv[1];
    factor = atof(argv[2]);
    fileout = argv[3];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
	    
#if 0
    startTimer();
    pixContrastTRC(pixs, pixs, factor);
    fprintf(stderr, "Time for contrast: %7.3f sec\n", stopTimer());
    pixWrite(fileout, pixs, IFF_JFIF_JPEG);
    pixDestroy(&pixs);
#endif

#if 0
    startTimer();
    pixd = pixContrastTRC(NULL, pixs, factor);
    fprintf(stderr, "Time for contrast: %7.3f sec\n", stopTimer());
    pixWrite(fileout, pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixs);
    pixDestroy(&pixd);
#endif

    na = numaContrastTRC(factor);
    gplotSimple1(na, GPLOT_X11, "junkroot", "contrast trc");
    numaDestroy(&na);

#if 1     /* plot contrast TRC maps */
    nax = numaMakeSequence(0.0, 1.0, 256);
    gplot = gplotCreate("junkmap", GPLOT_X11,
        "Atan mapping function for contrast enhancement",
        "value in", "value out");
    for (iplot = 0; iplot < NPLOTS; iplot++) {
	na = numaContrastTRC(fact[iplot]);
	sprintf(bigbuf, "factor = %3.1f", fact[iplot]);
        gplotAddPlot(gplot, nax, na, GPLOT_LINES, bigbuf);
	numaDestroy(&na);
    }
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&nax);
#endif

    exit(0);
}

