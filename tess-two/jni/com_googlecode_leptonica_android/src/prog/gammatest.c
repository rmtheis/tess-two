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
 * gammatest.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allheaders.h"

#define  NPLOTS      5

#define  MINVAL      30
#define  MAXVAL      210

main(int    argc,
     char **argv)
{
char        *filein, *fileout;
char         bigbuf[512];
l_int32      iplot;
l_float32    gam;
l_float64    gamma[NPLOTS] = {.5, 1.0, 1.5, 2.0, 2.5};
GPLOT       *gplot;
NUMA        *na, *nax;
PIX         *pixs, *pixd;
static char  mainName[] = "gammatest";

    if (argc != 4)
	exit(ERROR_INT(" Syntax:  gammatest filein gam fileout", mainName, 1));

    filein = argv[1];
    gam = atof(argv[2]);
    fileout = argv[3];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
	    
#if 1
    startTimer();
    pixGammaTRC(pixs, pixs, gam, MINVAL, MAXVAL);
    fprintf(stderr, "Time for gamma: %7.3f sec\n", stopTimer());
    pixWrite(fileout, pixs, IFF_JFIF_JPEG);
    pixDestroy(&pixs);
#endif

#if 0
    startTimer();
    pixd = pixGammaTRC(NULL, pixs, gam, MINVAL, MAXVAL);
    fprintf(stderr, "Time for gamma: %7.3f sec\n", stopTimer());
    pixWrite(fileout, pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixs);
    pixDestroy(&pixd);
#endif

    na = numaGammaTRC(gam, MINVAL, MAXVAL);
    gplotSimple1(na, GPLOT_X11, "/tmp/junkroot", "gamma trc");
    numaDestroy(&na);

#if 1     /* plot gamma TRC maps */
    gplot = gplotCreate("/tmp/junkmap", GPLOT_X11,
                        "Mapping function for gamma correction",
		       	"value in", "value out");
    nax = numaMakeSequence(0.0, 1.0, 256);
    for (iplot = 0; iplot < NPLOTS; iplot++) {
        na = numaGammaTRC(gamma[iplot], 30, 215);
	sprintf(bigbuf, "gamma = %3.1f", gamma[iplot]);
        gplotAddPlot(gplot, nax, na, GPLOT_LINES, bigbuf);
	numaDestroy(&na);
    }
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&nax);
#endif

    exit(0);
}



