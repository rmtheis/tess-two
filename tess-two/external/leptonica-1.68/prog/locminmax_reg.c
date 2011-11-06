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
 * locminmax_reg.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
l_int32      i, j;
l_float32    f;
l_uint32     redval, greenval;
PIX         *pixs, *pixd, *pixt0, *pixt1, *pixt2, *pixt3;
static char  mainName[] = "locminmax_reg";

    if (argc != 1)
        exit(ERROR_INT("syntax: locminmax_reg", mainName, 1));

    pixs = pixCreate(500, 500, 8);
    for (i = 0; i < 500; i++) {
        for (j = 0; j < 500; j++) {
            f = 128.0 + 26.3 * sin(0.0438 * (l_float32)i);
            f += 33.4 * cos(0.0712 * (l_float32)i);
            f += 18.6 * sin(0.0561 * (l_float32)j);
            f += 23.6 * cos(0.0327 * (l_float32)j);
            pixSetPixel(pixs, j, i, (l_int32)f);
        }
    }
    pixDisplay(pixs, 0, 0);
    pixWrite("/tmp/junkpattern.png", pixs, IFF_PNG);

    startTimer();
/*    pixSelectedLocalExtrema(pixs, 1, &pixt1, &pixt2); */
    pixLocalExtrema(pixs, 0, 0, &pixt1, &pixt2);
    fprintf(stderr, "Time for extrema: %7.3f\n", stopTimer());
    composeRGBPixel(255, 0, 0, &redval);
    composeRGBPixel(0, 255, 0, &greenval);
    pixd = pixConvertTo32(pixs);
    pixPaintThroughMask(pixd, pixt2, 0, 0, greenval);
    pixPaintThroughMask(pixd, pixt1, 0, 0, redval);
    pixDisplay(pixd, 510, 0);
    pixWrite("/tmp/junkpixd.png", pixd, IFF_PNG);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixs);
    pixDestroy(&pixd);

    pixt0 = pixRead("karen8.jpg");
    pixs = pixBlockconv(pixt0, 10, 10);
    pixDisplay(pixs, 0, 400);
    pixWrite("/tmp/junkconv.png", pixs, IFF_PNG);
    startTimer();
/*    pixSelectedLocalExtrema(pixs, 1, &pixt1, &pixt2); */
    pixLocalExtrema(pixs, 50, 100, &pixt1, &pixt2);
    fprintf(stderr, "Time for extrema: %7.3f\n", stopTimer());
    composeRGBPixel(255, 0, 0, &redval);
    composeRGBPixel(0, 255, 0, &greenval);
    pixd = pixConvertTo32(pixs);
    pixPaintThroughMask(pixd, pixt2, 0, 0, greenval);
    pixPaintThroughMask(pixd, pixt1, 0, 0, redval);
    pixDisplay(pixd, 350, 400);
    pixWrite("/tmp/junkpixd2.png", pixd, IFF_PNG);
    pixDestroy(&pixt0);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixs);
    pixDestroy(&pixd);

    return 0;
}

