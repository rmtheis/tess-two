/*===================================================================*
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
 * convolvetest.c
 *
 */

#include "allheaders.h"

static const char  *kel1str = " 20    50   80  50   20 "
                              " 50   100  140  100  50 "
                              " 90   160  200  160  90 "
                              " 50   100  140  100  50 "
                              " 20    50   80   50  20 ";

static const char  *kel2str = " -20  -50  -80  -50  -20 "
                              " -50   50   80   50  -50 "
                              " -90   90  200   90  -90 "
                              " -50   50   80   50  -50 "
                              " -20  -50  -80  -50  -20 ";

static const char  *kel3xstr = " -70  40  100   40  -70 ";
static const char  *kel3ystr = "  20 -70   40  100   40  -70  20 ";

#define  NTIMES   100
#define  ALL   1

int main(int    argc,
         char **argv)
{
l_int32      i, j, wc, hc, d, bias;
L_KERNEL    *kel1, *kel2, *kel3x, *kel3y;
PIX         *pix, *pixs, *pixg, *pixacc, *pixd, *pixt;
char        *filein, *fileout;
static char  mainName[] = "convolvetest";

    if (argc != 5)
        return ERROR_INT(" Syntax:  convolvetest filein wc hc fileout",
                         mainName, 1);

    filein = argv[1];
    wc = atoi(argv[2]);
    hc = atoi(argv[3]);
    fileout = argv[4];
    if ((pix = pixRead(filein)) == NULL)
        return ERROR_INT("pix not made", mainName, 1);
    d = pixGetDepth(pix);
    if (d != 1 && d != 8 && d != 32)
        pixs = pixConvertTo8(pix, 0);
    else
        pixs = pixClone(pix);
    pixDestroy(&pix);
    d = pixGetDepth(pixs);

    if (d == 8 && (ALL || 0)) {
            /* Measure speed */
        pixacc = pixBlockconvAccum(pixs);
        for (i = 0; i < NTIMES; i++) {
            pixd = pixBlockconvGray(pixs, pixacc, wc, hc);
            if ((i+1) % 10 == 0)
                fprintf(stderr, "%d iters\n", i + 1);
            pixDestroy(&pixd);
        }
        pixd = pixBlockconvGray(pixs, pixacc, wc, hc);
        pixWrite(fileout, pixd, IFF_JFIF_JPEG);
        pixDestroy(&pixacc);
    }
    if (d == 8 && (ALL || 0)) {
            /* Test pixBlockconvGray() */
        pixacc = pixBlockconvAccum(pixs);
        pixd = pixBlockconvGray(pixs, pixacc, wc, hc);
        pixWrite(fileout, pixd, IFF_JFIF_JPEG);
        pixDestroy(&pixacc);
    }
    if (ALL || 0) {
            /* Test pixBlockconv() */
        pixd = pixBlockconv(pixs, wc, hc);
        pixWrite(fileout, pixd, IFF_JFIF_JPEG);
    }
    if (d == 1 && (ALL || 0)) {
            /* Test pixBlockrank() */
        pixacc = pixBlockconvAccum(pixs);
        pixd = pixBlockrank(pixs, pixacc, wc, hc, 0.5);
        pixWrite(fileout, pixd, IFF_TIFF_G4);
        pixDestroy(&pixacc);
    }
    if (d == 1 && (ALL || 0)) {
            /* Test pixBlocksum() */
        pixacc = pixBlockconvAccum(pixs);
        pixd = pixBlocksum(pixs, pixacc, wc, hc);
        pixInvert(pixd, pixd);
        pixWrite(fileout, pixd, IFF_JFIF_JPEG);
        pixDestroy(&pixacc);
    }
    if (ALL || 0) {
            /* Test pixCensusTransform() */
        d = pixGetDepth(pixs);
        if (d == 32)
            pixt = pixConvertRGBToLuminance(pixs);
        else
            pixt = pixClone(pixs);
        pixacc = pixBlockconvAccum(pixt);
        pixd = pixCensusTransform(pixt, wc, NULL);
        pixDestroy(&pixt);
        pixDestroy(&pixacc);
        pixWrite(fileout, pixd, IFF_PNG);
    }
    if (ALL || 0) {
            /* Test generic convolution with kel1 */
        lept_mkdir("lept");
        if (pixGetDepth(pixs) == 32)
            pixg = pixScaleRGBToGrayFast(pixs, 2, COLOR_GREEN);
        else
            pixg = pixScale(pixs, 0.5, 0.5);
        pixDisplay(pixg, 0, 600);
        kel1 = kernelCreateFromString(5, 5, 2, 2, kel1str);
        pixd = pixConvolve(pixg, kel1, 8, 1);
        pixDisplay(pixd, 700, 0);
        pixWrite("/tmp/lept/convol_d4.bmp", pixd, IFF_BMP);
        pixDestroy(&pixd);
        kernelDestroy(&kel1);

            /* Test convolution with flat rectangular kel */
        kel2 = kernelCreate(11, 11);
        kernelSetOrigin(kel2, 5, 5);
        for (i = 0; i < 11; i++) {
            for (j = 0; j < 11; j++)
                kernelSetElement(kel2, i, j, 1);
        }
        startTimer();
        pixd = pixConvolve(pixg, kel2, 8, 1);
        fprintf(stderr, "Generic convolution: %7.3f sec\n", stopTimer());
        pixDisplay(pixd, 1200, 0);
        pixWrite("/tmp/lept/convol_d5.bmp", pixd, IFF_BMP);
        startTimer();
        pixt = pixBlockconv(pixg, 5, 5);
        fprintf(stderr, "Block convolution: %7.3f sec\n", stopTimer());
        pixDisplay(pixd, 1200, 600);
        pixWrite("/tmp/lept/convol_d6.bmp", pixt, IFF_BMP);
        pixCompareGray(pixd, pixt, L_COMPARE_ABS_DIFF, GPLOT_X11, NULL,
                       NULL, NULL, NULL);
        pixDestroy(&pixg);
        pixDestroy(&pixt);
        kernelDestroy(&kel2);
    }
    if (ALL || 0) {
            /* Test bias convolution with kel2 */
        if (pixGetDepth(pixs) == 32)
            pixg = pixScaleRGBToGrayFast(pixs, 2, COLOR_GREEN);
        else
            pixg = pixScale(pixs, 0.5, 0.5);
        pixDisplay(pixg, 0, 600);
        kel2 = kernelCreateFromString(5, 5, 2, 2, kel2str);
        pixd = pixConvolveWithBias(pixg, kel2, NULL, TRUE, &bias);
        pixDisplay(pixd, 700, 0);
        fprintf(stderr, "bias = %d\n", bias);
        pixWrite("/tmp/lept/convol_d6.png", pixd, IFF_PNG);
        pixDestroy(&pixg);
        kernelDestroy(&kel2);
        pixDestroy(&pixd);
    }
    if (ALL || 1) {
            /* Test separable bias convolution with kel3x, kel3y */
        if (pixGetDepth(pixs) == 32)
            pixg = pixScaleRGBToGrayFast(pixs, 2, COLOR_GREEN);
        else
            pixg = pixScale(pixs, 0.5, 0.5);
        pixDisplay(pixg, 0, 600);
        kel3x = kernelCreateFromString(1, 5, 0, 2, kel3xstr);
        kel3y = kernelCreateFromString(7, 1, 3, 0, kel3ystr);
        pixd = pixConvolveWithBias(pixg, kel3x, kel3y, TRUE, &bias);
        pixDisplay(pixd, 700, 0);
        fprintf(stderr, "bias = %d\n", bias);
        pixWrite("/tmp/lept/convol_d7.png", pixd, IFF_PNG);
        pixDestroy(&pixg);
        kernelDestroy(&kel3x);
        kernelDestroy(&kel3y);
        pixDestroy(&pixd);
    }

    pixDestroy(&pixs);
    return 0;
}

