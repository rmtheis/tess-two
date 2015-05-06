/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -
 -  Redistribution and use in source and binary forms, with or without
 -  modification, are permitted provided that the following conditions
 -  are met:
 -  1. Redistributions of source code must retain the above copyright
 -     notice, this list of conditions and the following disclaimer.
 -  2. Redistributions in binary form must reproduce the above
 -     copyright notice, this list of conditions and the following
 -     disclaimer in the documentation and/or other materials
 -     provided with the distribution.
 -
 -  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 -  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 -  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 -  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 -  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 -  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 -  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 -  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 -  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 -  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 -  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================*/

/*
 * locminmax_reg.c
 *
 *    Note: you can remove all minima that are touching the border, using:
 *             pix3 = pixRemoveBorderConnComps(pix1, 8);
 *             pixPaintThroughMask(pixd, pix3, 0, 0, redval);
 */

#include <math.h>
#include "allheaders.h"

int main(int    argc,
         char **argv)
{
l_int32      i, j;
l_float32    f;
l_uint32     redval, greenval;
PIX         *pixs, *pixd, *pix0, *pix1, *pix2;
static char  mainName[] = "locminmax_reg";

    if (argc != 1)
        return ERROR_INT("syntax: locminmax_reg", mainName, 1);

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
/*    pixSelectedLocalExtrema(pixs, 1, &pix1, &pix2); */
    pixLocalExtrema(pixs, 0, 0, &pix1, &pix2);
    fprintf(stderr, "Time for extrema: %7.3f\n", stopTimer());
    composeRGBPixel(255, 0, 0, &redval);
    composeRGBPixel(0, 255, 0, &greenval);
    pixd = pixConvertTo32(pixs);
    pixPaintThroughMask(pixd, pix2, 0, 0, greenval);
    pixPaintThroughMask(pixd, pix1, 0, 0, redval);
    pixDisplay(pixd, 510, 0);
    pixWrite("/tmp/junkpixd.png", pixd, IFF_PNG);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pixs);
    pixDestroy(&pixd);

    pix0 = pixRead("karen8.jpg");
    pixs = pixBlockconv(pix0, 10, 10);
    pixDisplay(pixs, 0, 400);
    pixWrite("/tmp/junkconv.png", pixs, IFF_PNG);
    startTimer();
/*    pixSelectedLocalExtrema(pixs, 1, &pix1, &pix2); */
    pixLocalExtrema(pixs, 50, 100, &pix1, &pix2);
    fprintf(stderr, "Time for extrema: %7.3f\n", stopTimer());
    composeRGBPixel(255, 0, 0, &redval);
    composeRGBPixel(0, 255, 0, &greenval);
    pixd = pixConvertTo32(pixs);
    pixPaintThroughMask(pixd, pix2, 0, 0, greenval);
    pixPaintThroughMask(pixd, pix1, 0, 0, redval);
    pixDisplay(pixd, 350, 400);
    pixWrite("/tmp/junkpixd2.png", pixd, IFF_PNG);
    pixDestroy(&pix0);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pixs);
    pixDestroy(&pixd);
    return 0;
}
