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
 * blend2_reg.c
 *
 *   Regression test for this function:
 *       pixBlendWithGrayMask()
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
l_int32      i, j, w1, h1, w2, h2, w, h, same;
BOX         *box1, *box2;
PIX         *pixs, *pixs1, *pixs2, *pix1, *pix2;
PIX         *pixg, *pixg1, *pixg2, *pixc2, *pixbl, *pixd;
PIXA        *pixa;
static char  mainName[] = "blend2_reg";

        /* --- Set up the 8 bpp blending image --- */
    pixg = pixCreate(660, 500, 8);
    for (i = 0; i < 500; i++)
        for (j = 0; j < 660; j++)
            pixSetPixel(pixg, j, i, (l_int32)(0.775 * j) % 256);

        /* --- Set up the initial color images to be blended together --- */
    pixs1 = pixRead("wyom.jpg");
    pixs2 = pixRead("fish24.jpg");
    pixGetDimensions(pixs1, &w1, &h1, NULL);
    pixGetDimensions(pixs2, &w2, &h2, NULL);
    h = L_MIN(h1, h2);
    w = L_MIN(w1, w2);
    box1 = boxCreate(0, 0, w1, h1);
    box2 = boxCreate(0, 300, 660, 500);
    pix1 = pixClipRectangle(pixs1, box1, NULL);
    pix2 = pixClipRectangle(pixs2, box2, NULL);
    pixDestroy(&pixs1);
    pixDestroy(&pixs2);
    boxDestroy(&box1);
    boxDestroy(&box2);

        /* --- Blend 2 rgb images --- */
    pixa = pixaCreate(0);
    pixSaveTiled(pixg, pixa, 1, 1, 40, 32);
    pixd = pixBlendWithGrayMask(pix1, pix2, pixg, 50, 50);
    pixSaveTiled(pix1, pixa, 1, 1, 40, 32);
    pixSaveTiled(pix2, pixa, 1, 0, 40, 32);
    pixSaveTiled(pixd, pixa, 1, 0, 40, 32);
    pixDestroy(&pixd);

        /* --- Blend 2 grayscale images --- */
    pixg1 = pixConvertRGBToLuminance(pix1);
    pixg2 = pixConvertRGBToLuminance(pix2);
    pixd = pixBlendWithGrayMask(pixg1, pixg2, pixg, 50, 50);
    pixSaveTiled(pixg1, pixa, 1, 1, 40, 32);
    pixSaveTiled(pixg2, pixa, 1, 0, 40, 32);
    pixSaveTiled(pixd, pixa, 1, 0, 40, 32);
    pixDestroy(&pixg1);
    pixDestroy(&pixg2);
    pixDestroy(&pixd);

        /* --- Blend a colormap image and an rgb image --- */
    pixc2 = pixFixedOctcubeQuantGenRGB(pix2, 2);
    pixd = pixBlendWithGrayMask(pix1, pixc2, pixg, 50, 50);
    pixSaveTiled(pix1, pixa, 1, 1, 40, 32);
    pixSaveTiled(pixc2, pixa, 1, 0, 40, 32);
    pixSaveTiled(pixd, pixa, 1, 0, 40, 32);
    pixDestroy(&pixc2);
    pixDestroy(&pixd);

        /* --- Blend a colormap image and a grayscale image --- */
    pixg1 = pixConvertRGBToLuminance(pix1);
    pixc2 = pixFixedOctcubeQuantGenRGB(pix2, 2);
    pixd = pixBlendWithGrayMask(pixg1, pixc2, pixg, 50, 50);
    pixSaveTiled(pixg1, pixa, 1, 1, 40, 32);
    pixSaveTiled(pixc2, pixa, 1, 0, 40, 32);
    pixSaveTiled(pixd, pixa, 1, 0, 40, 32);
    pixDestroy(&pixd);
    pixd = pixBlendWithGrayMask(pixg1, pixc2, pixg, -100, -100);
    pixSaveTiled(pixg1, pixa, 1, 1, 40, 32);
    pixSaveTiled(pixc2, pixa, 1, 0, 40, 32);
    pixSaveTiled(pixd, pixa, 1, 0, 40, 32);
    pixDestroy(&pixd);
    pixDestroy(&pixg1);
    pixDestroy(&pixc2);

        /* --- Test png read/write with alpha channel --- */
        /* First make pixs1, using pixg as the alpha channel */
    pixs = pixRead("fish24.jpg");
    box1 = boxCreate(0, 300, 660, 500);
    pixs1 = pixClipRectangle(pixs, box1, NULL);
    pixSaveTiled(pixs1, pixa, 1, 1, 40, 32);
    pixSetRGBComponent(pixs1, pixg, L_ALPHA_CHANNEL);
        /* To see the alpha channel, blend with a black image */
    pixbl = pixCreate(660, 500, 32);
    pixd = pixBlendWithGrayMask(pixbl, pixs1, NULL, 0, 0);
    pixSaveTiled(pixd, pixa, 1, 0, 40, 32);
    pixDestroy(&pixd);
        /* Write out the RGBA image and read it back */
    l_pngSetWriteAlpha(1);
    pixWrite("/tmp/junkpixs1.png", pixs1, IFF_PNG);
    l_pngSetStripAlpha(0);
    pixs2 = pixRead("/tmp/junkpixs1.png");
        /* Make sure that the alpha channel image hasn't changed */
    pixg2 = pixGetRGBComponent(pixs2, L_ALPHA_CHANNEL);
    pixEqual(pixg, pixg2, &same);
    if (same)
        fprintf(stderr, "PNG with alpha read/write OK\n");
    else
        fprintf(stderr, "PNG with alpha read/write failed\n");
        /* Blend again with a black image */
    pixd = pixBlendWithGrayMask(pixbl, pixs2, NULL, 0, 0);
    pixSaveTiled(pixd, pixa, 1, 0, 40, 32);
    pixDestroy(&pixd);
        /* Blend with a white image */
    pixSetAll(pixbl);
    pixd = pixBlendWithGrayMask(pixbl, pixs2, NULL, 0, 0);
    pixSaveTiled(pixd, pixa, 1, 0, 40, 32);
    pixDestroy(&pixd);
    l_pngSetWriteAlpha(0);  /* reset to default */
    l_pngSetStripAlpha(1);  /* reset to default */
    pixDestroy(&pixbl);
    pixDestroy(&pixs);
    pixDestroy(&pixs1);
    pixDestroy(&pixs2);
    pixDestroy(&pixg2);
    boxDestroy(&box1);

        /* --- Display results --- */
    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/junkblend2.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixDestroy(&pixg);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    return 0;
}

