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
 * rotateorth_reg.c
 *
 *    Regression test for all rotateorth functions
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define   BINARY_IMAGE        "test1.png"
#define   GRAYSCALE_IMAGE     "test8.jpg"
#define   FOUR_BPP_IMAGE      "weasel4.8g.png"
#define   COLORMAP_IMAGE      "dreyfus8.png"
#define   RGB_IMAGE           "marge.jpg"

void RotateOrthTest(PIX *pix, l_int32 *pcount, L_REGPARAMS *rp);


main(int    argc,
     char **argv)
{
l_int32       count, success, display;
FILE         *fp;
PIX          *pixs;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &fp, &display, &success, &rp))
              return 1;

    count = 0;
    fprintf(stderr, "\nTest binary image:\n");
    pixs = pixRead(BINARY_IMAGE);
    RotateOrthTest(pixs, &count, rp);
    pixDestroy(&pixs);
    fprintf(stderr, "\nTest 4 bpp colormapped image:\n");
    pixs = pixRead(FOUR_BPP_IMAGE);
    RotateOrthTest(pixs, &count, rp);
    pixDestroy(&pixs);
    fprintf(stderr, "\nTest grayscale image:\n");
    pixs = pixRead(GRAYSCALE_IMAGE);
    RotateOrthTest(pixs, &count, rp);
    pixDestroy(&pixs);
    fprintf(stderr, "\nTest colormap image:\n");
    pixs = pixRead(COLORMAP_IMAGE);
    RotateOrthTest(pixs, &count, rp);
    pixDestroy(&pixs);
    fprintf(stderr, "\nTest rgb image:\n");
    pixs = pixRead(RGB_IMAGE);
    RotateOrthTest(pixs, &count, rp);
    pixDestroy(&pixs);

    regTestCleanup(argc, argv, fp, success, rp);
    return 0;
}


void
RotateOrthTest(PIX          *pixs,
               l_int32      *pcount,
               L_REGPARAMS  *rp)
{
l_int32   zero, count;
PIX      *pixt, *pixd;
PIXCMAP  *cmap;

    cmap = pixGetColormap(pixs);

	/* Test 4 successive 90 degree rotations */
    pixt = pixRotate90(pixs, 1);
    pixd = pixRotate90(pixt, 1);
    pixDestroy(&pixt);
    pixt = pixRotate90(pixd, 1);
    pixDestroy(&pixd);
    pixd = pixRotate90(pixt, 1);
    pixDestroy(&pixt);
    regTestComparePix(rp->fp, rp->argv, pixs, pixd, (*pcount)++, &rp->success);
    if (!cmap) {
        pixXor(pixd, pixd, pixs);
        pixZero(pixd, &zero);
        if (zero)
            fprintf(stderr, "OK.  Four 90-degree rotations gives I\n");
        else {
             pixCountPixels(pixd, &count, NULL);
             fprintf(stderr, "Failure for four 90-degree rots; count = %d\n",
                     count);
        }
    }
    pixDestroy(&pixd);

	/* Test 2 successive 180 degree rotations */
    pixt = pixRotate180(NULL, pixs);
    pixRotate180(pixt, pixt);
    regTestComparePix(rp->fp, rp->argv, pixs, pixt, (*pcount)++, &rp->success);
    if (!cmap) {
        pixXor(pixt, pixt, pixs);
        pixZero(pixt, &zero);
        if (zero)
            fprintf(stderr, "OK.  Two 180-degree rotations gives I\n");
        else {
            pixCountPixels(pixt, &count, NULL);
            fprintf(stderr, "Failure for two 180-degree rots; count = %d\n",
                    count);
        }
    }
    pixDestroy(&pixt);

	/* Test 2 successive LR flips */
    pixt = pixFlipLR(NULL, pixs);
    pixFlipLR(pixt, pixt);
    regTestComparePix(rp->fp, rp->argv, pixs, pixt, (*pcount)++, &rp->success);
    if (!cmap) {
        pixXor(pixt, pixt, pixs);
        pixZero(pixt, &zero);
        if (zero)
            fprintf(stderr, "OK.  Two LR flips gives I\n");
        else {
            pixCountPixels(pixt, &count, NULL);
            fprintf(stderr, "Failure for two LR flips; count = %d\n", count);
        }
    }
    pixDestroy(&pixt);

	/* Test 2 successive TB flips */
    pixt = pixFlipTB(NULL, pixs);
    pixFlipTB(pixt, pixt);
    regTestComparePix(rp->fp, rp->argv, pixs, pixt, (*pcount)++, &rp->success);
    if (!cmap) {
        pixXor(pixt, pixt, pixs);
        pixZero(pixt, &zero);
        if (zero)
            fprintf(stderr, "OK.  Two TB flips gives I\n");
        else {
            pixCountPixels(pixt, &count, NULL);
            fprintf(stderr, "Failure for two TB flips; count = %d\n", count);
        }
    }
    pixDestroy(&pixt);
    return;
}

