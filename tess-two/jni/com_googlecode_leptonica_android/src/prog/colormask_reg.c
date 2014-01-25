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
 *   colormask_reg.c
 *
 *   This tests the ability to identify regions in HSV color space
 *   by analyzing the HS histogram and building masks that cover
 *   peaks in HS.
 */

#include <math.h>
#ifndef  _WIN32
#include <unistd.h>
#else
#include <windows.h>   /* for Sleep() */
#endif  /* _WIN32 */
#include "allheaders.h"


int main(int    argc,
         char **argv)
{
l_int32       i, j, x, y, rval, gval, bval;
l_uint32      pixel;
l_float32     frval, fgval, fbval;
NUMA         *nahue, *nasat, *napk;
PIX          *pixs, *pixhsv, *pixh, *pixg, *pixf, *pixd;
PIX          *pixr, *pixt1, *pixt2, *pixt3;
PIXA         *pixa, *pixapk;
PTA          *ptapk;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

        /* Make a graded frame color */
    pixs = pixCreate(650, 900, 32);
    for (i = 0; i < 900; i++) {
        rval = 40 + i / 30;
        for (j = 0; j < 650; j++) {
            gval = 255 - j / 30;
            bval = 70 + j / 30;
            composeRGBPixel(rval, gval, bval, &pixel);
            pixSetPixel(pixs, j, i, pixel);
        }
    }

        /* Place an image inside the frame and convert to HSV */
    pixt1 = pixRead("1555-3.jpg");
    pixt2 = pixScale(pixt1, 0.5, 0.5);
    pixRasterop(pixs, 100, 100, 2000, 2000, PIX_SRC, pixt2, 0, 0);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDisplayWithTitle(pixs, 400, 0, "Input image", rp->display);
    pixa = pixaCreate(0);
    pixhsv = pixConvertRGBToHSV(NULL, pixs);

        /* Work in the HS projection of HSV */
    pixh = pixMakeHistoHS(pixhsv, 5, &nahue, &nasat);
    pixg = pixMaxDynamicRange(pixh, L_LOG_SCALE);
    pixf = pixConvertGrayToFalseColor(pixg, 1.0);
    regTestWritePixAndCheck(rp, pixf, IFF_PNG);   /* 0 */
    pixDisplayWithTitle(pixf, 100, 0, "False color HS histo", rp->display);
    pixaAddPix(pixa, pixs, L_COPY);
    pixaAddPix(pixa, pixhsv, L_INSERT);
    pixaAddPix(pixa, pixg, L_INSERT);
    pixaAddPix(pixa, pixf, L_INSERT);
    gplotSimple1(nahue, GPLOT_PNG, "/tmp/junkhue", "Histogram of hue values");
#ifndef  _WIN32
    sleep(1);
#else
    Sleep(1000);
#endif  /* _WIN32 */
    pixt3 = pixRead("/tmp/junkhue.png");
    regTestWritePixAndCheck(rp, pixt3, IFF_PNG);  /* 1 */
    pixDisplayWithTitle(pixt3, 100, 300, "Histo of hue", rp->display);
    pixaAddPix(pixa, pixt3, L_INSERT);
    gplotSimple1(nasat, GPLOT_PNG, "/tmp/junksat",
                 "Histogram of saturation values");
#ifndef  _WIN32
    sleep(1);
#else
    Sleep(1000);
#endif  /* _WIN32 */
    pixt3 = pixRead("/tmp/junksat.png");
    regTestWritePixAndCheck(rp, pixt3, IFF_PNG);  /* 2 */
    pixDisplayWithTitle(pixt3, 100, 800, "Histo of saturation", rp->display);
    pixaAddPix(pixa, pixt3, L_INSERT);
    pixd = pixaDisplayTiledAndScaled(pixa, 32, 270, 7, 0, 30, 3);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);  /* 3 */
    pixDisplayWithTitle(pixd, 0, 400, "Hue and Saturation Mosaic", rp->display);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    numaDestroy(&nahue);
    numaDestroy(&nasat);

        /* Find all the peaks */
    pixFindHistoPeaksHSV(pixh, L_HS_HISTO, 20, 20, 6, 2.0,
                         &ptapk, &napk, &pixapk);
    numaWriteStream(stderr, napk);
    ptaWriteStream(stderr, ptapk, 1);
    pixd = pixaDisplayTiledInRows(pixapk, 32, 1400, 1.0, 0, 30, 2);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);  /* 4 */
    pixDisplayWithTitle(pixd, 0, 550, "Peaks in HS", rp->display);
    pixDestroy(&pixh);
    pixDestroy(&pixd);
    pixaDestroy(&pixapk);

        /* Make masks for each of the peaks */
    pixa = pixaCreate(0);
    pixr = pixScaleBySampling(pixs, 0.4, 0.4);
    for (i = 0; i < 6; i++) {
        ptaGetIPt(ptapk, i, &x, &y);
        pixt1 = pixMakeRangeMaskHS(pixr, y, 20, x, 20, L_INCLUDE_REGION);
        pixaAddPix(pixa, pixt1, L_INSERT);
        pixGetAverageMaskedRGB(pixr, pixt1, 0, 0, 1, L_MEAN_ABSVAL,
                               &frval, &fgval, &fbval);
        composeRGBPixel((l_int32)frval, (l_int32)fgval, (l_int32)fbval,
                        &pixel);
        pixt2 = pixCreateTemplate(pixr);
        pixSetAll(pixt2);
        pixPaintThroughMask(pixt2, pixt1, 0, 0, pixel);
        pixaAddPix(pixa, pixt2, L_INSERT);
        pixt3 = pixCreateTemplate(pixr);
        pixSetAllArbitrary(pixt3, pixel);
        pixaAddPix(pixa, pixt3, L_INSERT);
    }
    pixd = pixaDisplayTiledAndScaled(pixa, 32, 225, 3, 0, 30, 3);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);  /* 5 */
    pixDisplayWithTitle(pixd, 600, 0, "Masks over peaks", rp->display);
    pixDestroy(&pixs);
    pixDestroy(&pixr);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    ptaDestroy(&ptapk);
    numaDestroy(&napk);

    return regTestCleanup(rp);
}
