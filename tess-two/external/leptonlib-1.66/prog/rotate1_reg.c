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
 *   rotate1_reg.c
 *
 *    Regression test for rotation by shear and area mapping.
 *    Displays results when images are rotated sequentially multiple times.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define   BINARY_IMAGE              "test1.png"
#define   TWO_BPP_IMAGE             "weasel2.4c.png"
#define   FOUR_BPP_IMAGE1           "weasel4.11c.png"
#define   FOUR_BPP_IMAGE2           "weasel4.16g.png"
#define   EIGHT_BPP_IMAGE           "test8.jpg"
#define   EIGHT_BPP_CMAP_IMAGE1     "dreyfus8.png"
#define   EIGHT_BPP_CMAP_IMAGE2     "test24.jpg"
#define   RGB_IMAGE                 "marge.jpg"

static const l_int32    MODSIZE = 11;  /* set to 11 for display */

static const l_float32  ANGLE1 = 3.14159265 / 12.;
static const l_float32  ANGLE2 = 3.14159265 / 120.;
static const l_int32    NTIMES = 24;

static void RotateTest(PIX *pixs, l_int32 reduction, l_int32 *pcount,
                       L_REGPARAMS *rp);


l_int32 main(int    argc,
             char **argv)
{
l_int32       count, display, success;
FILE         *fp;
PIX          *pixs, *pixd;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &fp, &display, &success, &rp))
              return 1;

    count = 0;
    fprintf(stderr, "Test binary image:\n");
    pixs = pixRead(BINARY_IMAGE);
    RotateTest(pixs, 1, &count, rp);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 2 bpp cmapped image with filled cmap:\n");
    pixs = pixRead(TWO_BPP_IMAGE);
    RotateTest(pixs, 1, &count, rp);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 4 bpp cmapped image with unfilled cmap:\n");
    pixs = pixRead(FOUR_BPP_IMAGE1);
    RotateTest(pixs, 1, &count, rp);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 4 bpp cmapped image with filled cmap:\n");
    pixs = pixRead(FOUR_BPP_IMAGE2);
    RotateTest(pixs, 1, &count, rp);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 8 bpp grayscale image:\n");
    pixs = pixRead(EIGHT_BPP_IMAGE);
    RotateTest(pixs, 1, &count, rp);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 8 bpp grayscale cmap image:\n");
    pixs = pixRead(EIGHT_BPP_CMAP_IMAGE1);
    RotateTest(pixs, 1, &count, rp);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 8 bpp color cmap image:\n");
    pixs = pixRead(EIGHT_BPP_CMAP_IMAGE2);
    pixd = pixOctreeColorQuant(pixs, 200, 0);
    RotateTest(pixs, 4, &count, rp);
    pixDestroy(&pixs);
    pixDestroy(&pixd);

    fprintf(stderr, "Test rgb image:\n");
    pixs = pixRead(RGB_IMAGE);
    RotateTest(pixs, 1, &count, rp);
    pixDestroy(&pixs);

    regTestCleanup(argc, argv, fp, success, rp);
    return 0;
}


static void
RotateTest(PIX          *pixs,
           l_int32       reduction,
           l_int32      *pcount,
           L_REGPARAMS  *rp)
{
l_int32   w, h, d, i, outformat;
PIX      *pixt, *pixd;
PIXA     *pixa;
PIXCMAP  *cmap;

    pixa = pixaCreate(0);
    pixGetDimensions(pixs, &w, &h, &d);
    outformat = (d == 8 || d == 32) ? IFF_JFIF_JPEG : IFF_PNG;
    cmap = pixGetColormap(pixs);
    pixd = pixRotate(pixs, ANGLE1, L_ROTATE_SHEAR, L_BRING_IN_WHITE, w, h);
    for (i = 1; i < NTIMES; i++) {
        if ((i % MODSIZE) == 0) {
            if (i == MODSIZE) {
                pixSaveTiled(pixd, pixa, reduction, 1, 20, 32);
                regTestWritePixAndCheck(pixd, outformat, pcount, rp);
            } else {
                pixSaveTiled(pixd, pixa, reduction, 0, 20, 32);
                regTestWritePixAndCheck(pixd, outformat, pcount, rp);
            }
        }
        pixt = pixRotate(pixd, ANGLE1, L_ROTATE_SHEAR,
                         L_BRING_IN_WHITE, w, h);
        pixDestroy(&pixd);
        pixd = pixt;
    }
    pixDestroy(&pixd);

    pixd = pixRotate(pixs, ANGLE1, L_ROTATE_SAMPLING, L_BRING_IN_WHITE, w, h);
    for (i = 1; i < NTIMES; i++) {
        if ((i % MODSIZE) == 0) {
            if (i == MODSIZE) {
                pixSaveTiled(pixd, pixa, reduction, 1, 20, 32);
                regTestWritePixAndCheck(pixd, outformat, pcount, rp);
            } else {
                pixSaveTiled(pixd, pixa, reduction, 0, 20, 32);
                regTestWritePixAndCheck(pixd, outformat, pcount, rp);
            }
        }
        pixt = pixRotate(pixd, ANGLE1, L_ROTATE_SAMPLING,
                         L_BRING_IN_WHITE, w, h);
        pixDestroy(&pixd);
        pixd = pixt;
    }
    pixDestroy(&pixd);

    pixd = pixRotate(pixs, ANGLE1, L_ROTATE_AREA_MAP, L_BRING_IN_WHITE, w, h);
    for (i = 1; i < NTIMES; i++) {
        if ((i % MODSIZE) == 0) {
            if (i == MODSIZE) {
                pixSaveTiled(pixd, pixa, reduction, 1, 20, 32);
                regTestWritePixAndCheck(pixd, outformat, pcount, rp);
            } else {
                pixSaveTiled(pixd, pixa, reduction, 0, 20, 32);
                regTestWritePixAndCheck(pixd, outformat, pcount, rp);
            }
        }
        pixt = pixRotate(pixd, ANGLE1, L_ROTATE_AREA_MAP,
                         L_BRING_IN_WHITE, w, h);
        pixDestroy(&pixd);
        pixd = pixt;
    }
    pixDestroy(&pixd);

    pixd = pixRotateAMCorner(pixs, ANGLE2, L_BRING_IN_WHITE);
    for (i = 1; i < NTIMES; i++) {
        if ((i % MODSIZE) == 0) {
            if (i == MODSIZE) {
                pixSaveTiled(pixd, pixa, reduction, 1, 20, 32);
                regTestWritePixAndCheck(pixd, outformat, pcount, rp);
            } else {
                pixSaveTiled(pixd, pixa, reduction, 0, 20, 32);
                regTestWritePixAndCheck(pixd, outformat, pcount, rp);
            }
        }
        pixt = pixRotateAMCorner(pixd, ANGLE2, L_BRING_IN_WHITE);
        pixDestroy(&pixd);
        pixd = pixt;
    }
    pixDestroy(&pixd);

    if (d == 32) {
        pixd = pixRotateAMColorFast(pixs, ANGLE1, 0xb0ffb000);
        for (i = 1; i < NTIMES; i++) {
            if ((i % MODSIZE) == 0) {
                if (i == MODSIZE) {
                    pixSaveTiled(pixd, pixa, reduction, 1, 20, 32);
                    regTestWritePixAndCheck(pixd, outformat, pcount, rp);
                } else {
                    pixSaveTiled(pixd, pixa, reduction, 0, 20, 32);
                    regTestWritePixAndCheck(pixd, outformat, pcount, rp);
                }
            }
            pixt = pixRotateAMColorFast(pixd, ANGLE1, 0xb0ffb000);
            pixDestroy(&pixd);
            pixd = pixt;
        }
    }
    pixDestroy(&pixd);

    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplayWithTitle(pixd, 100, 100, NULL, rp->display);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    return;
}


