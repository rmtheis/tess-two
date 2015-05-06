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
 * adaptmaptest.c
 *
 *   Generates adaptive mappings in both gray and color, testing
 *   individual parts.
 *
 *   e.g., use with wet-day.jpg
 */

#include "allheaders.h"

#define  SIZE_X        10
#define  SIZE_Y        30
#define  BINTHRESH     50
#define  MINCOUNT      30

#define  BGVAL         200
#define  SMOOTH_X      2
#define  SMOOTH_Y      1

   /* Location of image region in wet-day.jpg */
#define  XS     151
#define  YS     225
#define  WS     913
#define  HS     1285


int main(int    argc,
         char **argv)
{
l_int32      w, h, d;
PIX         *pixs, *pixc, *pixg, *pixgm, *pixmi, *pixd, *pixd2;
PIX         *pixmr, *pixmg, *pixmb, *pixmri, *pixmgi, *pixmbi;
PIX         *pixim;
PIXA        *pixa;
char        *filein;
static char  mainName[] = "adaptmaptest";

    if (argc != 2)
        return ERROR_INT(" Syntax:  adaptmaptest filein", mainName, 1);

    filein = argv[1];
    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pix not made", mainName, 1);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8 && d != 32)
        return ERROR_INT("pix not 8 or 32 bpp", mainName, 1);
    pixDisplayWrite(NULL, -1);
    pixa = pixaCreate(0);
    pixSaveTiled(pixs, pixa, 1.0, 1, 20, 32);
    pixDisplayWrite(pixs, 1);

    if (d == 32) {
        pixc = pixClone(pixs);
        pixg = pixConvertRGBToGray(pixs, 0.33, 0.34, 0.33);
    } else {
        pixc = pixConvertTo32(pixs);
        pixg = pixClone(pixs);
    }
    pixSaveTiled(pixg, pixa, 1.0, 0, 20, 32);
    pixDisplayWrite(pixg, 1);

#if 1
        /* Process in grayscale */
    startTimer();
    pixim = NULL;
    pixim = pixCreate(w, h, 1);
    pixRasterop(pixim, XS, YS, WS, HS, PIX_SET, NULL, 0, 0);
    pixGetBackgroundGrayMap(pixg, pixim, SIZE_X, SIZE_Y,
                            BINTHRESH, MINCOUNT, &pixgm);
    fprintf(stderr, "time for gray adaptmap gen: %7.3f\n", stopTimer());
    pixWrite("/tmp/pixgm1.png", pixgm, IFF_PNG);
    pixSaveTiled(pixgm, pixa, 1.0, 1, 20, 32);
    pixDisplayWrite(pixgm, 1);

    startTimer();
    pixmi = pixGetInvBackgroundMap(pixgm, BGVAL, SMOOTH_X, SMOOTH_Y);
    fprintf(stderr, "time for gray inv map generation: %7.3f\n", stopTimer());
    pixWrite("/tmp/pixmi1.png", pixmi, IFF_PNG);
    pixSaveTiled(pixmi, pixa, 1.0, 0, 20, 32);
    pixDisplayWrite(pixmi, 1);

    startTimer();
    pixd = pixApplyInvBackgroundGrayMap(pixg, pixmi, SIZE_X, SIZE_Y);
    fprintf(stderr, "time to apply gray inv map: %7.3f\n", stopTimer());
    pixWrite("/tmp/pixd1.jpg", pixd, IFF_JFIF_JPEG);
    pixSaveTiled(pixd, pixa, 1.0, 0, 20, 32);
    pixDisplayWrite(pixd, 1);

    pixd2 = pixGammaTRCMasked(NULL, pixd, pixim, 1.0, 0, 190);
    pixInvert(pixim, pixim);
    pixGammaTRCMasked(pixd2, pixd2, pixim, 1.0, 60, 190);
    pixWrite("/tmp/pixo1.jpg", pixd2, IFF_JFIF_JPEG);
    pixSaveTiled(pixd2, pixa, 1.0, 0, 20, 32);
    pixDisplayWrite(pixd2, 1);
    pixDestroy(&pixim);
    pixDestroy(&pixgm);
    pixDestroy(&pixmi);
    pixDestroy(&pixd);
    pixDestroy(&pixd2);
#endif

#if 1
        /* Process in color */
    startTimer();
    pixmr = pixmg = pixmb = NULL;
    pixim = pixCreate(w, h, 1);
    pixRasterop(pixim, XS, YS, WS, HS, PIX_SET, NULL, 0, 0);
    pixGetBackgroundRGBMap(pixc, pixim, NULL, SIZE_X, SIZE_Y,
                           BINTHRESH, MINCOUNT,
                           &pixmr, &pixmg, &pixmb);
    fprintf(stderr, "time for color adaptmap gen: %7.3f\n", stopTimer());
    pixWrite("/tmp/pixmr.png", pixmr, IFF_PNG);
    pixWrite("/tmp/pixmg.png", pixmg, IFF_PNG);
    pixWrite("/tmp/pixmb.png", pixmb, IFF_PNG);

    startTimer();
    pixmri = pixGetInvBackgroundMap(pixmr, BGVAL, SMOOTH_X, SMOOTH_Y);
    pixmgi = pixGetInvBackgroundMap(pixmg, BGVAL, SMOOTH_X, SMOOTH_Y);
    pixmbi = pixGetInvBackgroundMap(pixmb, BGVAL, SMOOTH_X, SMOOTH_Y);
    fprintf(stderr, "time for color inv map generation: %7.3f\n", stopTimer());
    pixWrite("/tmp/pixmri.png", pixmri, IFF_PNG);
    pixWrite("/tmp/pixmgi.png", pixmgi, IFF_PNG);
    pixWrite("/tmp/pixmbi.png", pixmbi, IFF_PNG);

    startTimer();
    pixd = pixApplyInvBackgroundRGBMap(pixc, pixmri, pixmgi, pixmbi,
                                       SIZE_X, SIZE_Y);
    fprintf(stderr, "time to apply color inv maps: %7.3f\n", stopTimer());
    pixWrite("/tmp/pixd2.jpg", pixd, IFF_JFIF_JPEG);
    pixSaveTiled(pixd, pixa, 1.0, 1, 20, 32);
    pixDisplayWrite(pixd, 1);

    pixd2 = pixGammaTRCMasked(NULL, pixd, pixim, 1.0, 0, 190);
    pixInvert(pixim, pixim);
    pixGammaTRCMasked(pixd2, pixd2, pixim, 1.0, 60, 190);
    pixWrite("/tmp/pixo2.jpg", pixd2, IFF_JFIF_JPEG);
    pixSaveTiled(pixd2, pixa, 1.0, 0, 20, 32);
    pixDisplayWrite(pixd2, 1);
    pixDestroy(&pixmr);
    pixDestroy(&pixmg);
    pixDestroy(&pixmb);
    pixDestroy(&pixmri);
    pixDestroy(&pixmgi);
    pixDestroy(&pixmbi);
    pixDestroy(&pixim);
    pixDestroy(&pixd);
    pixDestroy(&pixd2);
#endif

#if 1
        /* Process in either gray or color, depending on the source */
    startTimer();
    pixim = pixCreate(w, h, 1);
    pixRasterop(pixim, XS, YS, WS, HS, PIX_SET, NULL, 0, 0);
/*    pixd = pixBackgroundNorm(pixs, pixim, NULL,SIZE_X, SIZE_Y,
                               BINTHRESH, MINCOUNT,
                               BGVAL, SMOOTH_X, SMOOTH_Y); */
    pixd = pixBackgroundNorm(pixs, pixim, NULL, 5, 10, BINTHRESH, 20,
                             BGVAL, SMOOTH_X, SMOOTH_Y);
    fprintf(stderr, "time for bg normalization: %7.3f\n", stopTimer());
    pixWrite("/tmp/pixd3.jpg", pixd, IFF_JFIF_JPEG);
    pixSaveTiled(pixd, pixa, 1.0, 1, 20, 32);
    pixDisplayWrite(pixd, 1);

    pixd2 = pixGammaTRCMasked(NULL, pixd, pixim, 1.0, 0, 190);
    pixInvert(pixim, pixim);
    pixGammaTRCMasked(pixd2, pixd2, pixim, 1.0, 60, 190);
    pixWrite("/tmp/pixo3.jpg", pixd2, IFF_JFIF_JPEG);
    pixSaveTiled(pixd2, pixa, 1.0, 0, 20, 32);
    pixDisplayWrite(pixd2, 1);

    pixDestroy(&pixd);
    pixDestroy(&pixd2);
    pixDestroy(&pixim);
#endif

        /* Display results */
    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/adapt.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixDisplayMultiple("/tmp/display/file*");

    pixDestroy(&pixs);
    pixDestroy(&pixg);
    pixDestroy(&pixc);
    return 0;
}

