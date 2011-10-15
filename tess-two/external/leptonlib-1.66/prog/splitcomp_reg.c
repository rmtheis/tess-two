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
 * splitcomp_reg.c
 *
 *   Regression test for splittings of a single component and for an image
 *   composed of several components, using different components and
 *   parameters.  Note that:
 *      (1) All coverings must cover the fg of the mask.
 *      (2) The first set of parameters is small and generates
 *          a proper tiling, covering ONLY the mask fg.
 *      (3) The tilings generated on 90 degree rotated components
 *          are identical (rotated) to those on un-rotated components.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
l_int32  i, j, w, h, display, success;
l_int32  minsum[5] =    { 2, 40, 50, 50, 70};
l_int32  skipdist[5] =  { 5,  5, 10, 10, 30};
l_int32  delta[5] =     { 2, 10, 10, 25, 40};
l_int32  maxbg[5] =     {10, 15, 10, 20, 40};
BOX     *box1, *box2, *box3, *box4;
BOXA    *boxa;
FILE    *fp;
PIX     *pixs, *pixc, *pixt, *pixd, *pix32;
PIXA    *pixas, *pixad;

    if (regTestSetup(argc, argv, &fp, &display, &success, NULL))
              return 1;

        /* Generate and save 1 bpp masks */
    pixas = pixaCreate(0);
    pixs = pixCreate(300, 250, 1);
    pixSetAll(pixs);
    box1 = boxCreate(50, 0, 140, 25);
    box2 = boxCreate(120, 100, 100, 25);
    box3 = boxCreate(75, 170, 80, 20);
    box4 = boxCreate(150, 80, 25, 70);

    pixClearInRect(pixs, box1);
    pixaAddPix(pixas, pixs, L_COPY);
    pixt = pixRotateOrth(pixs, 1);
    pixaAddPix(pixas, pixt, L_INSERT);

    pixClearInRect(pixs, box2);
    pixaAddPix(pixas, pixs, L_COPY);
    pixt = pixRotateOrth(pixs, 1);
    pixaAddPix(pixas, pixt, L_INSERT);

    pixClearInRect(pixs, box3);
    pixaAddPix(pixas, pixs, L_COPY);
    pixt = pixRotateOrth(pixs, 1);
    pixaAddPix(pixas, pixt, L_INSERT);

    pixClearInRect(pixs, box4);
    pixaAddPix(pixas, pixs, L_COPY);
    pixt = pixRotateOrth(pixs, 1);
    pixaAddPix(pixas, pixt, L_INSERT);

    boxDestroy(&box1);
    boxDestroy(&box2);
    boxDestroy(&box3);
    boxDestroy(&box4);
    pixDestroy(&pixs);

        /* Do 5 splittings on each of the 8 masks */
    pixad = pixaCreate(0);
    for (j = 0; j < 8; j++) {
        pixt = pixaGetPix(pixas, j, L_CLONE);
        pixGetDimensions(pixt, &w, &h, NULL);
        pix32 = pixCreate(w, h, 32);
        pixSetAll(pix32);
        pixPaintThroughMask(pix32, pixt, 0, 0, 0xc0c0c000);
        pixSaveTiled(pix32, pixad, 1, 1, 30, 32);
        for (i = 0; i < 5; i++) {
            pixc = pixCopy(NULL, pix32);
            boxa = pixSplitComponentIntoBoxa(pixt, NULL, minsum[i], skipdist[i],
                                             delta[i], maxbg[i], 0, 1);
/*            boxaWriteStream(stderr, boxa); */
            pixd = pixBlendBoxaRandom(pixc, boxa, 0.4);
            pixRenderBoxaArb(pixd, boxa, 2, 255, 0, 0);
            pixSaveTiled(pixd, pixad, 1, 0, 30, 32);
            pixDestroy(&pixd);
            pixDestroy(&pixc);
            boxaDestroy(&boxa);
        }
        pixDestroy(&pixt);
        pixDestroy(&pix32);
    }

        /* Display results */
    pixd = pixaDisplay(pixad, 0, 0);
    pixWrite("/tmp/split.0.png", pixd, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/split.0.png", 0, &success);
    pixDisplayWithTitle(pixd, 100, 100, NULL, display);
    pixDestroy(&pixd);
    pixaDestroy(&pixad);

        /* Put the 8 masks all together, and split 5 ways */
    pixad = pixaCreate(0);
    pixs = pixaDisplayOnLattice(pixas, 325, 325);
    pixGetDimensions(pixs, &w, &h, NULL);
    pix32 = pixCreate(w, h, 32);
    pixSetAll(pix32);
    pixPaintThroughMask(pix32, pixs, 0, 0, 0xc0c0c000);
    pixSaveTiled(pix32, pixad, 1, 1, 30, 32);
    for (i = 0; i < 5; i++) {
        pixc = pixCopy(NULL, pix32);
        boxa = pixSplitIntoBoxa(pixs, minsum[i], skipdist[i],
                                delta[i], maxbg[i], 0, 1);
/*        boxaWriteStream(stderr, boxa); */
        pixd = pixBlendBoxaRandom(pixc, boxa, 0.4);
        pixRenderBoxaArb(pixd, boxa, 2, 255, 0, 0);
        pixSaveTiled(pixd, pixad, 1, 0, 30, 32);
        pixDestroy(&pixd);
        pixDestroy(&pixc);
        boxaDestroy(&boxa);
    }
    pixDestroy(&pix32);
    pixDestroy(&pixs);

        /* Display results */
    pixd = pixaDisplay(pixad, 0, 0);
    pixWrite("/tmp/split.1.png", pixd, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/split.1.png", 1, &success);
    pixDisplayWithTitle(pixd, 600, 100, NULL, display);
    pixDestroy(&pixd);
    pixaDestroy(&pixad);

    pixaDestroy(&pixas);
    regTestCleanup(argc, argv, fp, success, NULL);
    return 0;
}


