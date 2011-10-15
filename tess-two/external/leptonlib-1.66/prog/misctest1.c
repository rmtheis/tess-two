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
 *   misctest1.c
 */


#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define   SHOW    0

main(int    argc,
     char **argv)
{
l_int32      w, h;
BOXA        *boxa;
PIX         *pixs, *pixt1, *pixt2, *pixg, *pixb, *pixd, *pixc;
PIX         *pixm, *pixm2, *pixd2, *pixs2;
PIXA        *pixa, *pixac;
PIXCMAP     *cmap, *cmapg;
static char  mainName[] = "misctest1";

    pixac = pixaCreate(0);

        /* Combine two grayscale images using a mask */
    pixd = pixRead("feyn.tif");
    pixs = pixRead("rabi.png");
    pixm = pixRead("pageseg2-seed.png");
    pixd2 = pixScaleToGray2(pixd);
    pixs2 = pixScaleToGray2(pixs);
    pixSaveTiled(pixd2, pixac, 2, 1, 40, 32);
    pixSaveTiled(pixs2, pixac, 2, 0, 40, 0);
    pixSaveTiled(pixm, pixac, 2, 0, 40, 0);
    pixCombineMaskedGeneral(pixd2, pixs2, pixm, 100, 100);
    pixSaveTiled(pixd2, pixac, 2, 1, 40, 0);
    pixDisplayWithTitle(pixd2, 100, 100, NULL, SHOW);
    pixDestroy(&pixd2);
    pixDestroy(&pixs2);

        /* Combine two binary images using a mask */
    pixm2 = pixExpandBinaryReplicate(pixm, 2);
    pixt1 = pixCopy(NULL, pixd);
    pixCombineMaskedGeneral(pixd, pixs, pixm2, 200, 200);
    pixSaveTiled(pixd, pixac, 4, 0, 40, 0);
    pixDisplayWithTitle(pixd, 700, 100, NULL, SHOW);
    pixCombineMasked(pixt1, pixs, pixm2);
    pixSaveTiled(pixt1, pixac, 4, 0, 40, 0);
    pixDestroy(&pixd);
    pixDestroy(&pixt1);
    pixDestroy(&pixs);
    pixDestroy(&pixm);
    pixDestroy(&pixm2);

        /* Do a restricted seedfill */
    pixs = pixRead("pageseg2-seed.png");
    pixm = pixRead("pageseg2-mask.png");
    pixd = pixSeedfillBinaryRestricted(NULL, pixs, pixm, 8, 50, 175);
    pixSaveTiled(pixs, pixac, 2, 1, 40, 0);
    pixSaveTiled(pixm, pixac, 2, 0, 40, 0);
    pixSaveTiled(pixd, pixac, 2, 0, 40, 0);
    pixDestroy(&pixs);
    pixDestroy(&pixm);
    pixDestroy(&pixd);

        /* Colorize a grayscale image */
    pixs = pixRead("lucasta.150.jpg");
    pixGetDimensions(pixs, &w, &h, NULL);
    pixb = pixThresholdToBinary(pixs, 128);
    boxa = pixConnComp(pixb, &pixa, 8);
    pixSaveTiled(pixs, pixac, 1, 1, 40, 0);
    cmap = pixcmapGrayToColor(0x6f90c0);
    pixSetColormap(pixs, cmap);
    pixSaveTiled(pixs, pixac, 1, 0, 40, 0);
    pixc = pixaDisplayRandomCmap(pixa, w, h);
    pixcmapResetColor(pixGetColormap(pixc), 0, 255, 255, 255);
    pixSaveTiled(pixc, pixac, 1, 0, 40, 0);
    pixDestroy(&pixs);
    pixDestroy(&pixb);
    pixDestroy(&pixc);
    boxaDestroy(&boxa);
    pixaDestroy(&pixa);

        /* Convert color to gray */
    pixs = pixRead("weasel4.16c.png");
    pixSaveTiled(pixs, pixac, 1, 1, 20, 0);
    pixc = pixConvertTo32(pixs);
    pixt1 = pixConvertRGBToGray(pixc, 3., 7., 5.);
    pixSaveTiled(pixt1, pixac, 1, 0, 20, 0);
    pixt2 = pixConvertRGBToGrayFast(pixc);
    pixSaveTiled(pixt2, pixac, 1, 0, 20, 0);
    pixg = pixCopy(NULL, pixs);
    cmap = pixGetColormap(pixs);
    cmapg = pixcmapColorToGray(cmap, 4., 6., 3.);
    pixSetColormap(pixg, cmapg);
    pixSaveTiled(pixg, pixac, 1, 0, 20, 0);
    pixDestroy(&pixs);
    pixDestroy(&pixc);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixg);

    pixd = pixaDisplay(pixac, 0, 0);
    pixDisplayWithTitle(pixd, 100, 100, NULL, 1);
    pixWrite("junkmisc1.png", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixaDestroy(&pixac);

    return 0;
}


