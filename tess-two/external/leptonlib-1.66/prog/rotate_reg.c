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
 *   rotate_reg.c
 *
 *    Regression test for rotation by shear and area mapping.
 *    Displays many images to the screen.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define   BINARY_IMAGE        "test1.png"
#define   GRAYSCALE_IMAGE     "test8.jpg"
#define   FOUR_BPP_IMAGE      "weasel4.16g.png"
#define   COLORMAP_IMAGE      "dreyfus8.png"
#define   RGB_IMAGE           "marge.jpg"

static const l_int32    MODSIZE = 5;  /* set to 5 for display */

static const l_float32  ANGLE1 = 3.14159265 / 12.;
static const l_float32  ANGLE2 = 3.14159265 / 120.;
static const l_int32    NTIMES = 24;

void rotateTest(PIX *pixs, PIXA *pixa, l_int32 reduction);


main(int    argc,
     char **argv)
{
PIX         *pixs, *pixd;
PIXA        *pixa;
static char  mainName[] = "rotate_reg";

    if (argc != 1)
	exit(ERROR_INT(" Syntax:  rotate_reg", mainName, 1));

    pixa = pixaCreate(0);
    fprintf(stderr, "Test binary image:\n");
    pixs = pixRead(BINARY_IMAGE);
    rotateTest(pixs, pixa, 1);
    pixDestroy(&pixs);
    fprintf(stderr, "Test 4 bpp colormapped image:\n");
    pixs = pixRead(FOUR_BPP_IMAGE);
    rotateTest(pixs, pixa, 1);
    pixDestroy(&pixs);
    fprintf(stderr, "Test grayscale image:\n");
    pixs = pixRead(GRAYSCALE_IMAGE);
    rotateTest(pixs, pixa, 1);
    pixDestroy(&pixs);
    fprintf(stderr, "Test colormap image:\n");
    pixs = pixRead(COLORMAP_IMAGE);
    rotateTest(pixs, pixa, 1);
    pixDestroy(&pixs);
    fprintf(stderr, "Test rgb image:\n");
    pixs = pixRead(RGB_IMAGE);
    rotateTest(pixs, pixa, 1);
    pixDestroy(&pixs);

    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("junkrotate.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    return 0;
}


void
rotateTest(PIX     *pixs,
           PIXA    *pixa,
           l_int32  reduction)
{
l_int32   w, h, d, i;
PIX      *pixt, *pixd1, *pixd2, *pixd3, *pixd4;
PIXCMAP  *cmap;

    PROCNAME("rotateTest");

    pixd3 = pixd4 = NULL;
    pixGetDimensions(pixs, &w, &h, &d);
    cmap = pixGetColormap(pixs);
    pixd1 = pixRotate(pixs, ANGLE1, L_ROTATE_SHEAR, L_BRING_IN_WHITE, w, h);
    for (i = 1; i < NTIMES; i++) {
        if ((i % MODSIZE) == 0) {
            if (i == MODSIZE)
                pixSaveTiled(pixd1, pixa, reduction, 1, 20, 32);
            else
                pixSaveTiled(pixd1, pixa, reduction, 0, 20, 32);
        }
        pixt = pixRotate(pixd1, ANGLE1, L_ROTATE_SHEAR,
                         L_BRING_IN_WHITE, w, h);
        pixDestroy(&pixd1);
        pixd1 = pixt;
    }

    pixd2 = pixRotate(pixs, ANGLE1, L_ROTATE_AREA_MAP, L_BRING_IN_WHITE, w, h);
    for (i = 1; i < NTIMES; i++) {
        if ((i % MODSIZE) == 0) {
            if (i == MODSIZE)
                pixSaveTiled(pixd2, pixa, reduction, 1, 20, 32);
            else
                pixSaveTiled(pixd2, pixa, reduction, 0, 20, 32);
        }
        pixt = pixRotate(pixd2, ANGLE1, L_ROTATE_AREA_MAP,
                         L_BRING_IN_WHITE, w, h);
        pixDestroy(&pixd2);
        pixd2 = pixt;
    }

    pixd3 = pixRotateAMCorner(pixs, ANGLE2, L_BRING_IN_WHITE);
    for (i = 1; i < NTIMES; i++) {
        if ((i % MODSIZE) == 0) {
            if (i == MODSIZE)
                pixSaveTiled(pixd3, pixa, reduction, 1, 20, 32);
            else
                pixSaveTiled(pixd3, pixa, reduction, 0, 20, 32);
        }
        pixt = pixRotateAMCorner(pixd3, ANGLE2, L_BRING_IN_WHITE);
        pixDestroy(&pixd3);
        pixd3 = pixt;
    }

    if (d == 32) {
        pixd4 = pixRotateAMColorFast(pixs, ANGLE1, 0xb0ffb000);
        for (i = 1; i < NTIMES; i++) {
            if ((i % MODSIZE) == 0) {
                if (i == MODSIZE)
                    pixSaveTiled(pixd4, pixa, reduction, 1, 20, 32);
                else
                    pixSaveTiled(pixd4, pixa, reduction, 0, 20, 32);
            }
            pixt = pixRotateAMColorFast(pixd4, ANGLE1, 0xb0ffb000);
            pixDestroy(&pixd4);
            pixd4 = pixt;
        }
    }

    if (d == 1) {
        pixWrite("junkbin1.png", pixd1, IFF_PNG);
        pixWrite("junkbin2.png", pixd2, IFF_PNG);
        pixWrite("junkbin3.png", pixd3, IFF_PNG);
    } else if (d == 4) {
        pixWrite("junk4bpp1.png", pixd1, IFF_PNG);
        pixWrite("junk4bpp2.png", pixd2, IFF_PNG);
        pixWrite("junk4bpp3.png", pixd3, IFF_PNG);
    } else if (d == 8 && cmap) {
        pixWrite("junkcmap1.png", pixd1, IFF_PNG);
        pixWrite("junkcmap2.png", pixd2, IFF_PNG);
        pixWrite("junkcmap3.png", pixd3, IFF_PNG);
    } else if (d == 8 && !cmap) {
        pixWrite("junkgray1.jpg", pixd1, IFF_JFIF_JPEG);
        pixWrite("junkgray2.jpg", pixd2, IFF_JFIF_JPEG);
        pixWrite("junkgray3.jpg", pixd3, IFF_JFIF_JPEG);
    } else if (d == 32) {
        pixWrite("junkrgb1.jpg", pixd1, IFF_JFIF_JPEG);
        pixWrite("junkrgb2.jpg", pixd2, IFF_JFIF_JPEG);
        pixWrite("junkrgb3.jpg", pixd3, IFF_JFIF_JPEG);
        pixWrite("junkrgb4.jpg", pixd4, IFF_JFIF_JPEG);
    }

    pixDestroy(&pixd1);
    pixDestroy(&pixd2);
    pixDestroy(&pixd3);
    pixDestroy(&pixd4);
    return;
}


