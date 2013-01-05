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
 *   rotate2_reg.c
 *
 *    Regression test for rotation by shear, sampling and area mapping.
 *    Displays results from all the various types of rotations.
 */

#include "allheaders.h"

#define   BINARY_IMAGE              "test1.png"
#define   TWO_BPP_IMAGE             "weasel2.4c.png"
#define   FOUR_BPP_IMAGE1           "weasel4.11c.png"
#define   FOUR_BPP_IMAGE2           "weasel4.16g.png"
#define   EIGHT_BPP_IMAGE           "test8.jpg"
#define   EIGHT_BPP_CMAP_IMAGE1     "dreyfus8.png"
#define   EIGHT_BPP_CMAP_IMAGE2     "test24.jpg"
#define   RGB_IMAGE                 "marge.jpg"

static const l_float32  ANGLE1 = 3.14159265 / 30.;
static const l_float32  ANGLE2 = 3.14159265 / 7.;

void RotateTest(PIX *pixs, l_int32 reduction, L_REGPARAMS *rp);


main(int    argc,
     char **argv)
{
PIX          *pixs, *pixd;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

    fprintf(stderr, "Test binary image:\n");
    pixs = pixRead(BINARY_IMAGE);
    RotateTest(pixs, 1, rp);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 2 bpp cmapped image with filled cmap:\n");
    pixs = pixRead(TWO_BPP_IMAGE);
    RotateTest(pixs, 1, rp);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 4 bpp cmapped image with unfilled cmap:\n");
    pixs = pixRead(FOUR_BPP_IMAGE1);
    RotateTest(pixs, 1, rp);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 4 bpp cmapped image with filled cmap:\n");
    pixs = pixRead(FOUR_BPP_IMAGE2);
    RotateTest(pixs, 1, rp);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 8 bpp grayscale image:\n");
    pixs = pixRead(EIGHT_BPP_IMAGE);
    RotateTest(pixs, 1, rp);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 8 bpp grayscale cmap image:\n");
    pixs = pixRead(EIGHT_BPP_CMAP_IMAGE1);
    RotateTest(pixs, 1, rp);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 8 bpp color cmap image:\n");
    pixs = pixRead(EIGHT_BPP_CMAP_IMAGE2);
    pixd = pixOctreeColorQuant(pixs, 200, 0);
    RotateTest(pixd, 2, rp);
    pixDestroy(&pixs);
    pixDestroy(&pixd);

    fprintf(stderr, "Test rgb image:\n");
    pixs = pixRead(RGB_IMAGE);
    RotateTest(pixs, 4, rp);
    pixDestroy(&pixs);

    return regTestCleanup(rp);
}


void
RotateTest(PIX          *pixs,
           l_int32       reduction,
           L_REGPARAMS  *rp)
{
l_int32   w, h, d, outformat;
PIX      *pixt1, *pixt2, *pixt3, *pixd;
PIXA     *pixa;

    pixGetDimensions(pixs, &w, &h, &d);
    outformat = (d == 8 || d == 32) ? IFF_JFIF_JPEG : IFF_PNG;

    pixa = pixaCreate(0);
    pixt1 = pixRotate(pixs, ANGLE1, L_ROTATE_SHEAR, L_BRING_IN_WHITE, w, h);
    pixSaveTiled(pixt1, pixa, reduction, 1, 20, 32);
    pixt2 = pixRotate(pixs, ANGLE1, L_ROTATE_SHEAR, L_BRING_IN_BLACK, w, h);
    pixSaveTiled(pixt2, pixa, reduction, 0, 20, 0);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixt1 = pixRotate(pixs, ANGLE1, L_ROTATE_SHEAR, L_BRING_IN_WHITE, 0, 0);
    pixSaveTiled(pixt1, pixa, reduction, 1, 20, 0);
    pixt2 = pixRotate(pixs, ANGLE1, L_ROTATE_SHEAR, L_BRING_IN_BLACK, 0, 0);
    pixSaveTiled(pixt2, pixa, reduction, 0, 20, 0);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixt1 = pixRotate(pixs, ANGLE2, L_ROTATE_SHEAR, L_BRING_IN_WHITE, w, h);
    pixSaveTiled(pixt1, pixa, reduction, 1, 20, 0);
    pixt2 = pixRotate(pixs, ANGLE2, L_ROTATE_SHEAR, L_BRING_IN_BLACK, w, h);
    pixSaveTiled(pixt2, pixa, reduction, 0, 20, 0);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixt1 = pixRotate(pixs, ANGLE2, L_ROTATE_SHEAR, L_BRING_IN_WHITE, 0, 0);
    pixSaveTiled(pixt1, pixa, reduction, 1, 20, 0);
    pixt2 = pixRotate(pixs, ANGLE2, L_ROTATE_SHEAR, L_BRING_IN_BLACK, 0, 0);
    pixSaveTiled(pixt2, pixa, reduction, 0, 20, 0);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixd = pixaDisplay(pixa, 0, 0);
    regTestWritePixAndCheck(rp, pixd, outformat);
    pixDisplayWithTitle(pixd, 100, 100, NULL, rp->display);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixa = pixaCreate(0);
    pixt1 = pixRotate(pixs, ANGLE2, L_ROTATE_SAMPLING, L_BRING_IN_WHITE, w, h);
    pixSaveTiled(pixt1, pixa, reduction, 1, 20, 32);
    pixt2 = pixRotate(pixs, ANGLE2, L_ROTATE_SAMPLING, L_BRING_IN_BLACK, w, h);
    pixSaveTiled(pixt2, pixa, reduction, 0, 20, 0);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixt1 = pixRotate(pixs, ANGLE2, L_ROTATE_SAMPLING, L_BRING_IN_WHITE, 0, 0);
    pixSaveTiled(pixt1, pixa, reduction, 1, 20, 0);
    pixt2 = pixRotate(pixs, ANGLE2, L_ROTATE_SAMPLING, L_BRING_IN_BLACK, 0, 0);
    pixSaveTiled(pixt2, pixa, reduction, 0, 20, 0);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

    if (pixGetDepth(pixs) == 1)
        pixt1 = pixScaleToGray2(pixs);
    else
        pixt1 = pixClone(pixs);
    pixt2 = pixRotate(pixt1, ANGLE2, L_ROTATE_AREA_MAP, L_BRING_IN_WHITE, w, h);
    pixSaveTiled(pixt2, pixa, reduction, 1, 20, 0);
    pixt3 = pixRotate(pixt1, ANGLE2, L_ROTATE_AREA_MAP, L_BRING_IN_BLACK, w, h);
    pixSaveTiled(pixt3, pixa, reduction, 0, 20, 0);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixt2 = pixRotate(pixt1, ANGLE2, L_ROTATE_AREA_MAP, L_BRING_IN_WHITE, 0, 0);
    pixSaveTiled(pixt2, pixa, reduction, 1, 20, 0);
    pixt3 = pixRotate(pixt1, ANGLE2, L_ROTATE_AREA_MAP, L_BRING_IN_BLACK, 0, 0);
    pixSaveTiled(pixt3, pixa, reduction, 0, 20, 0);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt1);
    pixd = pixaDisplay(pixa, 0, 0);
    regTestWritePixAndCheck(rp, pixd, outformat);
    pixDisplayWithTitle(pixd, 100, 100, NULL, rp->display);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    return;
}
