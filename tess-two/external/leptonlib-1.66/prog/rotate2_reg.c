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

static const l_float32  ANGLE1 = 3.14159265 / 30.;
static const l_float32  ANGLE2 = 3.14159265 / 7.;

void rotateTest(PIX *pixs, const char *filename, l_int32 reduction);



main(int    argc,
     char **argv)
{
PIX         *pixs, *pixd;
static char  mainName[] = "rotate2_reg";

    if (argc != 1)
	exit(ERROR_INT(" Syntax:  rotate2_reg", mainName, 1));

    fprintf(stderr, "Test binary image:\n");
    pixs = pixRead(BINARY_IMAGE);
    rotateTest(pixs, "/tmp/junk1bpp.png", 2);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 2 bpp cmapped image with filled cmap:\n");
    pixs = pixRead(TWO_BPP_IMAGE);
    rotateTest(pixs, "/tmp/junk2bpp.png", 1);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 4 bpp cmapped image with unfilled cmap:\n");
    pixs = pixRead(FOUR_BPP_IMAGE1);
    rotateTest(pixs, "/tmp/junk4bpp1.png", 1);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 4 bpp cmapped image with filled cmap:\n");
    pixs = pixRead(FOUR_BPP_IMAGE2);
    rotateTest(pixs, "/tmp/junk4bpp2.png", 1);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 8 bpp grayscale image:\n");
    pixs = pixRead(EIGHT_BPP_IMAGE);
    rotateTest(pixs, "/tmp/junk8bpp.png", 1);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 8 bpp grayscale cmap image:\n");
    pixs = pixRead(EIGHT_BPP_CMAP_IMAGE1);
    rotateTest(pixs, "/tmp/junk8bppcmapgray.png", 1);
    pixDestroy(&pixs);

    fprintf(stderr, "Test 8 bpp color cmap image:\n");
    pixs = pixRead(EIGHT_BPP_CMAP_IMAGE2);
    pixd = pixOctreeColorQuant(pixs, 200, 0);
    rotateTest(pixs, "/tmp/junk8bppcmapcolor.png", 2);
    pixDestroy(&pixs);
    pixDestroy(&pixd);

    fprintf(stderr, "Test rgb image:\n");
    pixs = pixRead(RGB_IMAGE);
    rotateTest(pixs, "/tmp/junkrgb.png", 2);
    pixDestroy(&pixs);

    return 0;
}


void
rotateTest(PIX          *pixs,
           const char  *filename,
           l_int32      reduction)
{
l_int32   w, h;
PIX      *pixt1, *pixt2, *pixt3, *pixd;
PIXA     *pixa;

    PROCNAME("rotateTest");

    pixa = pixaCreate(0);
    pixGetDimensions(pixs, &w, &h, NULL);
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

    pixt1 = pixRotate(pixs, ANGLE2, L_ROTATE_SAMPLING, L_BRING_IN_WHITE, w, h);
    pixSaveTiled(pixt1, pixa, reduction, 1, 20, 0);
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
    pixDisplay(pixd, 100, 100);
    pixWrite(filename, pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    return;
}


