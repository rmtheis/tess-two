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
 * blend2_reg.c
 *
 *   Regression test for this function:
 *       pixBlendWithGrayMask()
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
l_int32  i, j, w1, h1, w2, h2, w, h, same;
BOX     *box1, *box2;
PIX     *pixs, *pixs1, *pixs2, *pix1, *pix2;
PIX     *pixg, *pixg1, *pixg2, *pixc2, *pixbl, *pixd;
PIXA    *pixa;

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
    box1 = boxCreate(0, 0, w, h);
    box2 = boxCreate(0, 300, 660, 500);
    pix1 = pixClipRectangle(pixs1, box1, NULL);
    pix2 = pixClipRectangle(pixs2, box2, NULL);
    pixDestroy(&pixs1);
    pixDestroy(&pixs2);
    boxDestroy(&box1);
    boxDestroy(&box2);

        /* --- Blend 2 rgb images --- */
    pixa = pixaCreate(0);
    pixSaveTiled(pixg, pixa, 1.0, 1, 40, 32);
    pixd = pixBlendWithGrayMask(pix1, pix2, pixg, 50, 50);
    pixSaveTiled(pix1, pixa, 1.0, 1, 40, 32);
    pixSaveTiled(pix2, pixa, 1.0, 0, 40, 32);
    pixSaveTiled(pixd, pixa, 1.0, 0, 40, 32);
    pixDestroy(&pixd);

        /* --- Blend 2 grayscale images --- */
    pixg1 = pixConvertRGBToLuminance(pix1);
    pixg2 = pixConvertRGBToLuminance(pix2);
    pixd = pixBlendWithGrayMask(pixg1, pixg2, pixg, 50, 50);
    pixSaveTiled(pixg1, pixa, 1.0, 1, 40, 32);
    pixSaveTiled(pixg2, pixa, 1.0, 0, 40, 32);
    pixSaveTiled(pixd, pixa, 1.0, 0, 40, 32);
    pixDestroy(&pixg1);
    pixDestroy(&pixg2);
    pixDestroy(&pixd);

        /* --- Blend a colormap image and an rgb image --- */
    pixc2 = pixFixedOctcubeQuantGenRGB(pix2, 2);
    pixd = pixBlendWithGrayMask(pix1, pixc2, pixg, 50, 50);
    pixSaveTiled(pix1, pixa, 1.0, 1, 40, 32);
    pixSaveTiled(pixc2, pixa, 1.0, 0, 40, 32);
    pixSaveTiled(pixd, pixa, 1.0, 0, 40, 32);
    pixDestroy(&pixc2);
    pixDestroy(&pixd);

        /* --- Blend a colormap image and a grayscale image --- */
    pixg1 = pixConvertRGBToLuminance(pix1);
    pixc2 = pixFixedOctcubeQuantGenRGB(pix2, 2);
    pixd = pixBlendWithGrayMask(pixg1, pixc2, pixg, 50, 50);
    pixSaveTiled(pixg1, pixa, 1.0, 1, 40, 32);
    pixSaveTiled(pixc2, pixa, 1.0, 0, 40, 32);
    pixSaveTiled(pixd, pixa, 1.0, 0, 40, 32);
    pixDestroy(&pixd);
    pixd = pixBlendWithGrayMask(pixg1, pixc2, pixg, -100, -100);
    pixSaveTiled(pixg1, pixa, 1.0, 1, 40, 32);
    pixSaveTiled(pixc2, pixa, 1.0, 0, 40, 32);
    pixSaveTiled(pixd, pixa, 1.0, 0, 40, 32);
    pixDestroy(&pixd);
    pixDestroy(&pixg1);
    pixDestroy(&pixc2);

        /* --- Test png read/write with alpha channel --- */
        /* First make pixs1, using pixg as the alpha channel */
    pixs = pixRead("fish24.jpg");
    box1 = boxCreate(0, 300, 660, 500);
    pixs1 = pixClipRectangle(pixs, box1, NULL);
    pixSaveTiled(pixs1, pixa, 1.0, 1, 40, 32);
    pixSetRGBComponent(pixs1, pixg, L_ALPHA_CHANNEL);
        /* To see the alpha channel, blend with a black image */
    pixbl = pixCreate(660, 500, 32);
    pixd = pixBlendWithGrayMask(pixbl, pixs1, NULL, 0, 0);
    pixSaveTiled(pixd, pixa, 1.0, 0, 40, 32);
    pixDestroy(&pixd);
        /* Write out the RGBA image and read it back */
    pixWrite("/tmp/junkpixs1.png", pixs1, IFF_PNG);
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
    pixSaveTiled(pixd, pixa, 1.0, 0, 40, 32);
    pixDestroy(&pixd);
        /* Blend with a white image */
    pixSetAll(pixbl);
    pixd = pixBlendWithGrayMask(pixbl, pixs2, NULL, 0, 0);
    pixSaveTiled(pixd, pixa, 1.0, 0, 40, 32);
    pixDestroy(&pixd);
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
