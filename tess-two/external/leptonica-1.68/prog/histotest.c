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
 * histotest.c
 *
 *    Makes histograms of grayscale and color pixels
 *    from a pix.  For RGB color, this uses
 *    rgb --> octcube indexing.
 *
 *       histotest filein sigbits
 *
 *    where the number of octcubes is 8^(sigbits)
 *
 *    For gray, sigbits is ignored.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
char        *filein;
l_int32      d, sigbits;
GPLOT       *gplot;
NUMA        *na;
PIX         *pixs, *pixd;
static char  mainName[] = "histotest";

    if (argc != 3)
	exit(ERROR_INT(" Syntax:  histotest filein sigbits", mainName, 1));

    filein = argv[1];
    sigbits = atoi(argv[2]);

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
    d = pixGetDepth(pixs);
    if (d != 8 && d != 32)
	exit(ERROR_INT("depth not 8 or 32 bpp", mainName, 1));
	    
    if (d == 32) {
	startTimer();
	if ((na = pixOctcubeHistogram(pixs, sigbits, NULL)) == NULL)
	    exit(ERROR_INT("na not made", mainName, 1));
	fprintf(stderr, "histo time = %7.3f sec\n", stopTimer());
	gplot = gplotCreate("/tmp/junkrootc", GPLOT_X11,
		"color histogram with octcube indexing",
		"octcube index", "number of pixels in cube");
	gplotAddPlot(gplot, NULL, na, GPLOT_LINES, "input pix");
	gplotMakeOutput(gplot);
	gplotDestroy(&gplot);
    }
    else {
	if ((na = pixGetGrayHistogram(pixs, 1)) == NULL)
	    exit(ERROR_INT("na not made", mainName, 1));
	numaWrite("/tmp/junkna", na);
	gplot = gplotCreate("/tmp/junkrootg", GPLOT_X11, "grayscale histogram",
                            "gray value", "number of pixels");
	gplotSetScaling(gplot, GPLOT_LOG_SCALE_Y);
	gplotAddPlot(gplot, NULL, na, GPLOT_LINES, "input pix");
	gplotMakeOutput(gplot);
	gplotDestroy(&gplot);
    }

    pixDestroy(&pixs);
    numaDestroy(&na);
    exit(0);
}

