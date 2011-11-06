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
 * watershedtest.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
l_int32      i, j, w, h, empty;
l_uint32     redval, greenval;
l_float32    f;
L_WSHED     *wshed;
PIX         *pixs, *pixc, *pixd;
PIX         *pixt0, *pixt1, *pixt2, *pixt3, *pixt4, *pixt5;
PIX         *pixt6, *pixt7, *pixt8;
PIXA        *pixac;
PTA         *pta;
static char  mainName[] = "watershedtest";

    if (argc != 1)
	exit(ERROR_INT(" Syntax:  watershedtest", mainName, 1));

    pixac = pixaCreate(0);

    pixs = pixCreate(500, 500, 8);
    pixGetDimensions(pixs, &w, &h, NULL);
    for (i = 0; i < 500; i++) {
        for (j = 0; j < 500; j++) {
#if 1
            f = 128.0 + 26.3 * sin(0.0438 * (l_float32)i);
            f += 33.4 * cos(0.0712 * (l_float32)i);
            f += 18.6 * sin(0.0561 * (l_float32)j);
            f += 23.6 * cos(0.0327 * (l_float32)j);
#else
            f = 128.0 + 26.3 * sin(0.0238 * (l_float32)i);
            f += 33.4 * cos(0.0312 * (l_float32)i);
            f += 18.6 * sin(0.0261 * (l_float32)j);
            f += 23.6 * cos(0.0207 * (l_float32)j);
#endif
            pixSetPixel(pixs, j, i, (l_int32)f);
        }
    }
    pixSaveTiled(pixs, pixac, 1, 1, 10, 32);
    pixWrite("/tmp/junkpattern.png", pixs, IFF_PNG);
    startTimer();
    pixLocalExtrema(pixs, 0, 0, &pixt1, &pixt2);
    fprintf(stderr, "Time for extrema: %7.3f\n", stopTimer());
    pixSetOrClearBorder(pixt1, 2, 2, 2, 2, PIX_CLR);
    composeRGBPixel(255, 0, 0, &redval);
    composeRGBPixel(0, 255, 0, &greenval);
    pixc = pixConvertTo32(pixs);
    pixPaintThroughMask(pixc, pixt2, 0, 0, greenval);
    pixPaintThroughMask(pixc, pixt1, 0, 0, redval);
    pixSaveTiled(pixc, pixac, 1, 0, 10, 32);
    pixWrite("/tmp/junkpixc.png", pixc, IFF_PNG);
    pixSaveTiled(pixt1, pixac, 1, 0, 10, 32);
    pta = pixSelectMinInConnComp(pixs, pixt1, NULL);
/*    ptaWriteStream(stderr, pta, 1); */
    pixt3 = pixGenerateFromPta(pta, w, h);
    pixSaveTiled(pixt3, pixac, 1, 1, 10, 32);

    pixt4 = pixConvertTo32(pixs);
    pixPaintThroughMask(pixt4, pixt3, 0, 0, greenval);
    pixSaveTiled(pixt4, pixac, 1, 0, 10, 32);
    pixt5 = pixRemoveSeededComponents(NULL, pixt3, pixt1, 8, 2);
    pixSaveTiled(pixt5, pixac, 1, 0, 10, 32);
    pixZero(pixt5, &empty);
    fprintf(stderr, "Is empty?  %d\n", empty);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);

    wshed = wshedCreate(pixs, pixt3, 10, 0);
    startTimer();
    wshedApply(wshed);
    fprintf(stderr, "Time for wshed: %7.3f\n", stopTimer());
    pixt6 = pixaDisplayRandomCmap(wshed->pixad, w, h);
    pixSaveTiled(pixt6, pixac, 1, 1, 10, 32);
    numaWriteStream(stderr, wshed->nalevels);
    pixt7 = wshedRenderFill(wshed);
    pixSaveTiled(pixt7, pixac, 1, 0, 10, 32);
    pixt8 = wshedRenderColors(wshed);
    pixSaveTiled(pixt8, pixac, 1, 0, 10, 32);
    wshedDestroy(&wshed);

    pixd = pixaDisplay(pixac, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/junkwshed.png", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixaDestroy(&pixac);

    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt6);
    pixDestroy(&pixt7);
    pixDestroy(&pixt8);
    pixDestroy(&pixs);
    pixDestroy(&pixc);
    ptaDestroy(&pta);
    return 0;
}

