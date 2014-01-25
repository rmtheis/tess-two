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
 * colorspacetest.c
 *
 *   This allows the functions in colorspace_reg to be applied
 *   to any arbitrary image.  Otherwise, it is essentially the same.
 *
 *   We test:
 *     * conversions between HSV and both RGB and colormapped images
 *     * global linear color mapping and extraction of color magnitude
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
char         label[512];
l_int32      rval, gval, bval, w, h, i, j, rwhite, gwhite, bwhite, count;
l_uint32     pixel;
GPLOT       *gplot1, *gplot2;
NUMA        *naseq, *na;
NUMAA       *naa1, *naa2;
PIX         *pixs, *pixt, *pixt0, *pixt1, *pixt2;
PIX         *pixr, *pixg, *pixb;
PIXA        *pixa;
PIXCMAP     *cmap;
static char  mainName[] = "colorspacetest";

    if (argc != 2)
        return ERROR_INT(" Syntax:  colorspacetest filein", mainName, 1);

    if ((pixs = pixRead(argv[1])) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);

        /* Generate colors by sampling hue with max sat and value.
         * This was used to make the color strip 19-colors.png.  */
    pixa = pixaCreate(19);
    for (i = 0; i < 19; i++) {
        convertHSVToRGB((240 * i / 18), 255, 255, &rval, &gval, &bval);
        composeRGBPixel(rval, gval, bval, &pixel);
        pixt1 = pixCreate(50, 100, 32);
        pixSetAllArbitrary(pixt1, pixel);
        pixaAddPix(pixa, pixt1, L_INSERT);
    }
    pixt2 = pixaDisplayTiledInRows(pixa, 32, 1100, 1.0, 0, 0, 0);
    pixDisplayWrite(pixt2, 1);
    pixDestroy(&pixt2);
    pixaDestroy(&pixa);

        /* Colorspace conversion in rgb */
    pixDisplayWrite(pixs, 1);
    pixt = pixConvertRGBToHSV(NULL, pixs);
    pixDisplayWrite(pixt, 1);
    pixConvertHSVToRGB(pixt, pixt);
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);

        /* Colorspace conversion on a colormap */
    pixt = pixOctreeQuantNumColors(pixs, 25, 0);
    pixDisplayWrite(pixt, 1);
    cmap = pixGetColormap(pixt);
    pixcmapWriteStream(stderr, cmap);
    pixcmapConvertRGBToHSV(cmap);
    pixcmapWriteStream(stderr, cmap);
    pixDisplayWrite(pixt, 1);
    pixcmapConvertHSVToRGB(cmap);
    pixcmapWriteStream(stderr, cmap);
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);

        /* Color content extraction */
    pixColorContent(pixs, 0, 0, 0, 0, &pixr, &pixg, &pixb);
    pixDisplayWrite(pixr, 1);
    pixDisplayWrite(pixg, 1);
    pixDisplayWrite(pixb, 1);
    pixDestroy(&pixr);
    pixDestroy(&pixg);
    pixDestroy(&pixb);

        /* Color content measurement */
    pixa = pixaCreate(20);
    naseq = numaMakeSequence(100, 5, 20);
    naa1 = numaaCreate(6);
    naa2 = numaaCreate(6);
    for (i = 0; i < 6; i++) {
        na = numaCreate(20);
        numaaAddNuma(naa1, na, L_COPY);
        numaaAddNuma(naa2, na, L_INSERT);
    }
    pixGetDimensions(pixs, &w, &h, NULL);
    for (i = 0; i < 20; i++) {
        rwhite = 100 + 5 * i;
        gwhite = 200 - 5 * i;
        bwhite = 150;
        pixt0 = pixGlobalNormRGB(NULL, pixs, rwhite, gwhite, bwhite, 255);
        pixaAddPix(pixa, pixt0, L_INSERT);
        pixt1 = pixColorMagnitude(pixs, rwhite, gwhite, bwhite,
                                  L_MAX_DIFF_FROM_AVERAGE_2);
        for (j = 0; j < 6; j++) {
            pixt2 = pixThresholdToBinary(pixt1, 30 + 10 * j);
            pixInvert(pixt2, pixt2);
            pixCountPixels(pixt2, &count, NULL);
            na = numaaGetNuma(naa1, j, L_CLONE);
            numaAddNumber(na, (l_float32)count / (l_float32)(w * h));
            numaDestroy(&na);
            pixDestroy(&pixt2);
        }
        pixDestroy(&pixt1);
        pixt1 = pixColorMagnitude(pixs, rwhite, gwhite, bwhite,
                                  L_MAX_MIN_DIFF_FROM_2);
        for (j = 0; j < 6; j++) {
            pixt2 = pixThresholdToBinary(pixt1, 30 + 10 * j);
            pixInvert(pixt2, pixt2);
            pixCountPixels(pixt2, &count, NULL);
            na = numaaGetNuma(naa2, j, L_CLONE);
            numaAddNumber(na, (l_float32)count / (l_float32)(w * h));
            numaDestroy(&na);
            pixDestroy(&pixt2);
        }
        pixDestroy(&pixt1);
    }
    gplot1 = gplotCreate("/tmp/junkplot1", GPLOT_X11,
                         "Fraction with given color (diff from average)",
                         "white point space for red", "amount of color");
    gplot2 = gplotCreate("/tmp/junkplot2", GPLOT_X11,
                         "Fraction with given color (min diff)",
                         "white point space for red", "amount of color");
    for (j = 0; j < 6; j++) {
        na = numaaGetNuma(naa1, j, L_CLONE);
        sprintf(label, "thresh %d", 30 + 10 * j);
        gplotAddPlot(gplot1, naseq, na, GPLOT_LINES, label);
        numaDestroy(&na);
        na = numaaGetNuma(naa2, j, L_CLONE);
        gplotAddPlot(gplot2, naseq, na, GPLOT_LINES, label);
        numaDestroy(&na);
    }
    gplotMakeOutput(gplot1);
    gplotMakeOutput(gplot2);
    gplotDestroy(&gplot1);
    gplotDestroy(&gplot2);
    pixt1 = pixaDisplayTiledAndScaled(pixa, 32, 250, 4, 0, 10, 2);
    pixWrite("/tmp/junkcolormag", pixt1, IFF_PNG);
    pixDisplayWithTitle(pixt1, 0, 100, "Color magnitude", 1);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);
    numaDestroy(&naseq);
    numaaDestroy(&naa1);
    numaaDestroy(&naa2);

    pixDisplayMultiple("/tmp/display/file*");

    pixDestroy(&pixs);
    return 0;
}
