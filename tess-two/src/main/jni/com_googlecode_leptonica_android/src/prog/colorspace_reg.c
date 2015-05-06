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
 * colorspace_reg.c
 *
 *    Tests:
 *       - conversions between HSV and both RGB and colormapped images.
 *       - global linear color mapping and extraction of color magnitude
 */

#ifndef  _WIN32
#include <unistd.h>
#else
#include <windows.h>   /* for Sleep() */
#endif  /* _WIN32 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
char          label[512];
l_int32       rval, gval, bval, w, h, i, j, rwhite, gwhite, bwhite, count;
l_uint32      pixel;
GPLOT        *gplot1, *gplot2;
NUMA         *naseq, *na;
NUMAA        *naa1, *naa2;
PIX          *pixs, *pixt, *pixt0, *pixt1, *pixt2;
PIX          *pixr, *pixg, *pixb;  /* for color content extraction */
PIXA         *pixa, *pixat;
PIXCMAP      *cmap;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

        /* Generate a pdf of results when called with display */
    pixa = pixaCreate(0);

        /* Generate colors by sampling hue with max sat and value.
         * This image has been saved as 19-colors.png.  */
    pixat = pixaCreate(19);
    for (i = 0; i < 19; i++) {
        convertHSVToRGB((240 * i / 18), 255, 255, &rval, &gval, &bval);
        composeRGBPixel(rval, gval, bval, &pixel);
        pixt1 = pixCreate(50, 100, 32);
        pixSetAllArbitrary(pixt1, pixel);
        pixaAddPix(pixat, pixt1, L_INSERT);
    }
    pixt2 = pixaDisplayTiledInRows(pixat, 32, 1100, 1.0, 0, 0, 0);
    regTestWritePixAndCheck(rp, pixt2, IFF_PNG);  /* 0 */
    pixaAddPix(pixa, pixt2, L_INSERT);
    pixaDestroy(&pixat);

        /* Colorspace conversion in rgb */
    pixs = pixRead("wyom.jpg");
    pixaAddPix(pixa, pixs, L_INSERT);
    pixt = pixConvertRGBToHSV(NULL, pixs);
    regTestWritePixAndCheck(rp, pixt, IFF_JFIF_JPEG);  /* 1 */
    pixaAddPix(pixa, pixt, L_COPY);
    pixConvertHSVToRGB(pixt, pixt);
    regTestWritePixAndCheck(rp, pixt, IFF_JFIF_JPEG);  /* 2 */
    pixaAddPix(pixa, pixt, L_INSERT);

        /* Colorspace conversion on a colormap */
    pixt = pixOctreeQuantNumColors(pixs, 25, 0);
    regTestWritePixAndCheck(rp, pixt, IFF_JFIF_JPEG);  /* 3 */
    pixaAddPix(pixa, pixt, L_COPY);
    cmap = pixGetColormap(pixt);
    if (rp->display) pixcmapWriteStream(stderr, cmap);
    pixcmapConvertRGBToHSV(cmap);
    if (rp->display) pixcmapWriteStream(stderr, cmap);
    regTestWritePixAndCheck(rp, pixt, IFF_JFIF_JPEG);  /* 4 */
    pixaAddPix(pixa, pixt, L_COPY);
    pixcmapConvertHSVToRGB(cmap);
    if (rp->display) pixcmapWriteStream(stderr, cmap);
    regTestWritePixAndCheck(rp, pixt, IFF_JFIF_JPEG);  /* 5 */
    pixaAddPix(pixa, pixt, L_INSERT);

        /* Color content extraction */
    pixColorContent(pixs, 0, 0, 0, 0, &pixr, &pixg, &pixb);
    regTestWritePixAndCheck(rp, pixr, IFF_JFIF_JPEG);  /* 6 */
    pixaAddPix(pixa, pixr, L_INSERT);
    regTestWritePixAndCheck(rp, pixg, IFF_JFIF_JPEG);  /* 7 */
    pixaAddPix(pixa, pixg, L_INSERT);
    regTestWritePixAndCheck(rp, pixb, IFF_JFIF_JPEG);  /* 8 */
    pixaAddPix(pixa, pixb, L_INSERT);

        /* Color content measurement.  This tests the global
         * mapping of (r,g,b) --> (white), for 20 different
         * values of (r,g,b).   For each mappings, we compute
         * the color magnitude and threshold it at six values.
         * For each of those six thresholds, we plot the
         * fraction of pixels that exceeds the threshold
         * color magnitude, where the red value (mapped to
         * white) goes between 100 and 195.  */
    pixat = pixaCreate(20);
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
        pixaAddPix(pixat, pixt0, L_INSERT);
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
    gplot1 = gplotCreate("/tmp/regout/colorspace.10", GPLOT_PNG,
                         "Fraction with given color (diff from average)",
                         "white point space for red", "amount of color");
    gplot2 = gplotCreate("/tmp/regout/colorspace.11", GPLOT_PNG,
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
    pixt1 = pixaDisplayTiledAndScaled(pixat, 32, 250, 4, 0, 10, 2);
    regTestWritePixAndCheck(rp, pixt1, IFF_JFIF_JPEG);  /* 9 */
    pixaAddPix(pixa, pixt1, L_INSERT);
    pixDisplayWithTitle(pixt1, 0, 100, "Color magnitude", rp->display);
    pixaDestroy(&pixat);
    numaDestroy(&naseq);
    numaaDestroy(&naa1);
    numaaDestroy(&naa2);

        /* Give gnuplot time to write out the files */
#ifndef  _WIN32
    sleep(1);
#else
    Sleep(1000);
#endif  /* _WIN32 */

        /* Save as golden files, or check against them */
    regTestCheckFile(rp, "/tmp/regout/colorspace.10.png");  /* 10 */
    regTestCheckFile(rp, "/tmp/regout/colorspace.11.png");  /* 11 */

    if (rp->display) {
        pixt = pixRead("/tmp/regout/colorspace.10.png");
        pixaAddPix(pixa, pixt, L_INSERT);
        pixt = pixRead("/tmp/regout/colorspace.11.png");
        pixaAddPix(pixa, pixt, L_INSERT);
        pixaConvertToPdf(pixa, 0, 1.0, 0, 0, "colorspace tests",
                         "/tmp/regout/colorspace.pdf");
        L_INFO("Output pdf: /tmp/regout/colorspace.pdf\n", rp->testname);
    }
    pixaDestroy(&pixa);

    return regTestCleanup(rp);
}
