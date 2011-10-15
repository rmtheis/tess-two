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
 * adaptmaptest.c
 *
 *   Generates adaptive mappings in both gray and color, testing
 *   individual parts.
 *
 *   e.g., use with wet-day.jpg
 */

#include <stdio.h>
#include <stdlib.h>
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


main(int    argc,
     char **argv)
{
l_int32      w, h, d;
PIX         *pixs, *pixc, *pixg, *pixgm, *pixm, *pixmi, *pixd, *pixd2;
PIX         *pixmr, *pixmg, *pixmb, *pixmri, *pixmgi, *pixmbi;
PIX         *pixim;
PIXA        *pixa;
char        *filein;
static char  mainName[] = "adaptmaptest";

    if (argc != 2)
	exit(ERROR_INT(" Syntax:  adaptmaptest filein", mainName, 1));

    filein = argv[1];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8 && d != 32)
        exit(ERROR_INT("pix not 8 or 32 bpp", mainName, 1));
    pixDisplayWrite(NULL, -1);
    pixa = pixaCreate(0);
    pixSaveTiled(pixs, pixa, 1, 1, 20, 32);
    pixDisplayWrite(pixs, 1);

    if (d == 32) {
        pixc = pixClone(pixs);
        pixg = pixConvertRGBToGray(pixs, 0.33, 0.34, 0.33);
    } else {
        pixc = pixConvertTo32(pixs);
        pixg = pixClone(pixs);
    }
    pixSaveTiled(pixg, pixa, 1, 0, 20, 32);
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
    pixWrite("/tmp/junkpixgm1.png", pixgm, IFF_PNG);
    pixSaveTiled(pixgm, pixa, 1, 1, 20, 32);
    pixDisplayWrite(pixgm, 1);

    startTimer();
    pixmi = pixGetInvBackgroundMap(pixgm, BGVAL, SMOOTH_X, SMOOTH_Y);
    fprintf(stderr, "time for gray inv map generation: %7.3f\n", stopTimer());
    pixWrite("/tmp/junkpixmi1.png", pixmi, IFF_PNG);
    pixSaveTiled(pixmi, pixa, 1, 0, 20, 32);
    pixDisplayWrite(pixmi, 1);

    startTimer();
    pixd = pixApplyInvBackgroundGrayMap(pixg, pixmi, SIZE_X, SIZE_Y);
    fprintf(stderr, "time to apply gray inv map: %7.3f\n", stopTimer());
    pixWrite("/tmp/junkpixd1.jpg", pixd, IFF_JFIF_JPEG);
    pixSaveTiled(pixd, pixa, 1, 0, 20, 32);
    pixDisplayWrite(pixd, 1);

    pixd2 = pixGammaTRCMasked(NULL, pixd, pixim, 1.0, 0, 190);
    pixInvert(pixim, pixim);
    pixGammaTRCMasked(pixd2, pixd2, pixim, 1.0, 60, 190);
    pixWrite("/tmp/junkpixo1.jpg", pixd2, IFF_JFIF_JPEG);
    pixSaveTiled(pixd2, pixa, 1, 0, 20, 32);
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
    pixWrite("/tmp/junkpixmr.png", pixmr, IFF_PNG);
    pixWrite("/tmp/junkpixmg.png", pixmg, IFF_PNG);
    pixWrite("/tmp/junkpixmb.png", pixmb, IFF_PNG);

    startTimer();
    pixmri = pixGetInvBackgroundMap(pixmr, BGVAL, SMOOTH_X, SMOOTH_Y);
    pixmgi = pixGetInvBackgroundMap(pixmg, BGVAL, SMOOTH_X, SMOOTH_Y);
    pixmbi = pixGetInvBackgroundMap(pixmb, BGVAL, SMOOTH_X, SMOOTH_Y);
    fprintf(stderr, "time for color inv map generation: %7.3f\n", stopTimer());
    pixWrite("/tmp/junkpixmri.png", pixmri, IFF_PNG);
    pixWrite("/tmp/junkpixmgi.png", pixmgi, IFF_PNG);
    pixWrite("/tmp/junkpixmbi.png", pixmbi, IFF_PNG);

    startTimer();
    pixd = pixApplyInvBackgroundRGBMap(pixc, pixmri, pixmgi, pixmbi,
                                       SIZE_X, SIZE_Y);
    fprintf(stderr, "time to apply color inv maps: %7.3f\n", stopTimer());
    pixWrite("/tmp/junkpixd2.jpg", pixd, IFF_JFIF_JPEG);
    pixSaveTiled(pixd, pixa, 1, 1, 20, 32);
    pixDisplayWrite(pixd, 1);

    pixd2 = pixGammaTRCMasked(NULL, pixd, pixim, 1.0, 0, 190);
    pixInvert(pixim, pixim);
    pixGammaTRCMasked(pixd2, pixd2, pixim, 1.0, 60, 190);
    pixWrite("/tmp/junkpixo2.jpg", pixd2, IFF_JFIF_JPEG);
    pixSaveTiled(pixd2, pixa, 1, 0, 20, 32);
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
    pixWrite("/tmp/junkpixd3.jpg", pixd, IFF_JFIF_JPEG);
    pixSaveTiled(pixd, pixa, 1, 1, 20, 32);
    pixDisplayWrite(pixd, 1);

    pixd2 = pixGammaTRCMasked(NULL, pixd, pixim, 1.0, 0, 190);
    pixInvert(pixim, pixim);
    pixGammaTRCMasked(pixd2, pixd2, pixim, 1.0, 60, 190);
    pixWrite("/tmp/junkpixo3.jpg", pixd2, IFF_JFIF_JPEG);
    pixSaveTiled(pixd2, pixa, 1, 0, 20, 32);
    pixDisplayWrite(pixd2, 1);

    pixDestroy(&pixd);
    pixDestroy(&pixd2);
    pixDestroy(&pixim);
#endif
    
        /* Display results */
    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/junkadapt.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixDisplayMultiple("/tmp/junk_write_display*");

    pixDestroy(&pixs);
    pixDestroy(&pixg);
    pixDestroy(&pixc);
    exit(0);
}

