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
 * rank_reg.c
 *
 *   Tests grayscale rank functions:
 *      (1) pixRankFilterGray()
 *      (2) pixScaleGrayMinMax()
 *      (3) pixScaleGrayRankCascade()
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const l_int32   SIZE = 20;


main(int    argc,
     char **argv)
{
l_int32       i, j, w, h, same;
l_float32     t, t1, t2;
GPLOT        *gplot;
NUMA         *nax, *nay1, *nay2;
PIX          *pixs, *pixd, *pixt1, *pixt2, *pixt3, *pixt4;
PIXA         *pixa;
static char   mainName[] = "rank_reg";

    if (argc != 1)
	exit(ERROR_INT(" Syntax: rank_reg", mainName, 1));

    if ((pixs = pixRead("lucasta.150.jpg")) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
    pixGetDimensions(pixs, &w, &h, NULL);

    startTimer();
    pixd = pixRankFilterGray(pixs, 15, 15, 0.4);
    t = stopTimer();
    fprintf(stderr, "Time =  %7.3f sec\n", t);
    fprintf(stderr, "MPix/sec: %7.3f\n", 0.000001 * w * h / t);
    pixDisplay(pixs, 0, 200);
    pixDisplay(pixd, 600, 200);
    pixWrite("/tmp/junkfilter.png", pixd, IFF_PNG);
    pixDestroy(&pixd);

        /* Get results for dilation */
    startTimer();
    pixt1 = pixDilateGray(pixs, 15, 15);
    t = stopTimer();
    fprintf(stderr, "Dilation time =  %7.3f sec\n", t);

        /* Get results for erosion */
    pixt2 = pixErodeGray(pixs, 15, 15);

        /* Get results using the rank filter for rank = 0.0 and 1.0.
         * Don't use 0.0 or 1.0, because those are dispatched
         * automatically to erosion and dilation! */
    pixt3 = pixRankFilterGray(pixs, 15, 15, 0.0001);
    pixt4 = pixRankFilterGray(pixs, 15, 15, 0.9999);

        /* Compare */
    pixEqual(pixt1, pixt4, &same);
    if (same)
        fprintf(stderr, "Correct: dilation results same as rank 1.0\n");
    else
        fprintf(stderr, "Error: dilation results differ from rank 1.0\n");
    pixEqual(pixt2, pixt3, &same);
    if (same)
        fprintf(stderr, "Correct: erosion results same as rank 0.0\n");
    else
        fprintf(stderr, "Error: erosion results differ from rank 0.0\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

    fprintf(stderr, "\n----------------------------------------\n");
    fprintf(stderr, "The next part takes about 30 seconds\n");
    fprintf(stderr, "----------------------------------------\n\n");

    nax = numaMakeSequence(1, 1, SIZE);
    nay1 = numaCreate(SIZE);
    nay2 = numaCreate(SIZE);
    gplot = gplotCreate("/tmp/junkroot", GPLOT_X11, "sec/MPix vs filter size",
		        "size", "time");
    for (i = 1; i <= SIZE; i++) {
        t1 = t2 = 0.0;
        for (j = 0; j < 5; j++) {
            startTimer();
            pixt1 = pixRankFilterGray(pixs, i, SIZE + 1, 0.5);
	    t1 += stopTimer();
	    pixDestroy(&pixt1);
            startTimer();
            pixt1 = pixRankFilterGray(pixs, SIZE + 1, i, 0.5);
	    t2 += stopTimer();
	    if (j == 0)
                pixDisplayWrite(pixt1, 1);
	    pixDestroy(&pixt1);
        }
	numaAddNumber(nay1, 1000000. * t1 / (5. * w * h));
	numaAddNumber(nay2, 1000000. * t2 / (5. * w * h));
    }
    gplotAddPlot(gplot, nax, nay1, GPLOT_LINES, "vertical");
    gplotAddPlot(gplot, nax, nay2, GPLOT_LINES, "horizontal");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);

        /* Display tiled */
    pixa = pixaReadFiles("/tmp", "junk_write_display");
    pixd = pixaDisplayTiledAndScaled(pixa, 8, 250, 5, 0, 25, 2);
    pixWrite("/tmp/junktiles.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    pixDestroy(&pixs);

    pixDisplayWrite(NULL, -1);  /* clear out */

    pixs = pixRead("test8.jpg");
    for (i = 1; i <= 4; i++) {
        pixt1 = pixScaleGrayRank2(pixs, i);
        pixDisplay(pixt1, 300 * (i - 1), 100);
        pixDestroy(&pixt1);
    }
    pixDestroy(&pixs);

    pixs = pixRead("test24.jpg");
    pixt1 = pixConvertRGBToLuminance(pixs);
    pixt2 = pixScale(pixt1, 1.5, 1.5);
    for (i = 1; i <= 4; i++) {
        for (j = 1; j <= 4; j++) {
            pixt3 = pixScaleGrayRankCascade(pixt2, i, j, 0, 0);
            pixDisplayWrite(pixt3, 1);
	    pixDestroy(&pixt3);
	}
    }
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixs);
    pixDisplayMultiple("/tmp/junk_write_display*");
    return 0;
}


