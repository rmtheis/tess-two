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
 * grayfill_reg.c
 *
 */

#include "allheaders.h"

void PixTestEqual(PIX *pixs1, PIX *pixs2, PIX *pixm, l_int32 set,
                  l_int32 connectivity);


int main(int    argc,
         char **argv)
{
l_int32  i, j;
PIX     *pixm, *pixmi, *pixs1, *pixs1_8;
PIX     *pixs2, *pixs2_8, *pixs3, *pixs3_8;
PIX     *pixb1, *pixb2, *pixb3, *pixmin, *pixd;
PIXA    *pixac;

    pixac = pixaCreate(0);

        /* Mask */
    pixm = pixCreate(200, 200, 8);
    for (i = 0; i < 200; i++)
        for (j = 0; j < 200; j++)
            pixSetPixel(pixm, j, i, 20 + L_ABS((100 - i) * (100 - j)) / 50);
    pixmi = pixInvert(NULL, pixm);

        /* Seed1 */
    pixs1 = pixCreate(200, 200, 8);
    for (i = 99; i <= 101; i++)
        for (j = 99; j <= 101; j++)
            pixSetPixel(pixs1, j, i, 50 - i/100 - j/100);
    pixs1_8 = pixCopy(NULL, pixs1);

        /* Seed2 */
    pixs2 = pixCreate(200, 200, 8);
    for (i = 99; i <= 101; i++)
        for (j = 99; j <= 101; j++)
            pixSetPixel(pixs2, j, i, 205 - i/100 - j/100);
    pixs2_8 = pixCopy(NULL, pixs2);

        /* Inverse grayscale fill */
    pixSaveTiled(pixm, pixac, 1.0, 1, 10, 8);
    pixSaveTiled(pixs1, pixac, 1.0, 0, 10, 0);
    pixSeedfillGrayInv(pixs1, pixm, 4);
    pixSeedfillGrayInv(pixs1_8, pixm, 8);
    pixSaveTiled(pixs1, pixac, 1.0, 0, 10, 0);
    pixSaveTiled(pixs1_8, pixac, 1.0, 0, 10, 0);
    pixb1 = pixThresholdToBinary(pixs1, 20);
    pixSaveTiled(pixb1, pixac, 1.0, 0, 10, 0);
    pixCombineMasked(pixs1, pixm, pixb1);
    pixSaveTiled(pixs1, pixac, 1.0, 0, 10, 0);
    pixDestroy(&pixs1);
    pixDestroy(&pixs1_8);
    pixDestroy(&pixb1);

        /* Standard grayscale fill */
    pixSaveTiled(pixmi, pixac, 1.0, 1, 10, 0);
    pixSaveTiled(pixs2, pixac, 1.0, 0, 10, 0);
    pixSeedfillGray(pixs2, pixmi, 4);
    pixSeedfillGray(pixs2_8, pixmi, 8);
    pixSaveTiled(pixs2, pixac, 1.0, 0, 10, 0);
    pixSaveTiled(pixs2_8, pixac, 1.0, 0, 10, 0);
    pixb2 = pixThresholdToBinary(pixs2, 205);
    pixSaveTiled(pixb2, pixac, 1.0, 0, 10, 0);
    pixDestroy(&pixs2);
    pixDestroy(&pixs2_8);
    pixDestroy(&pixb2);

        /* Basin fill from minima as seed */
    pixSaveTiled(pixm, pixac, 1.0, 1, 10, 8);
    pixLocalExtrema(pixm, 0, 0, &pixmin, NULL);
    pixSaveTiled(pixmin, pixac, 1.0, 0, 10, 0);
    pixs3 = pixSeedfillGrayBasin(pixmin, pixm, 30, 4);
    pixs3_8 = pixSeedfillGrayBasin(pixmin, pixm, 30, 8);
    pixSaveTiled(pixs3, pixac, 1.0, 0, 10, 0);
    pixSaveTiled(pixs3_8, pixac, 1.0, 0, 10, 0);
    pixb3 = pixThresholdToBinary(pixs3, 60);
    pixSaveTiled(pixb3, pixac, 1.0, 0, 10, 0);
    pixDestroy(&pixs3);
    pixDestroy(&pixs3_8);
    pixDestroy(&pixb3);

    pixd = pixaDisplay(pixac, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/junkfill.png", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixaDestroy(&pixac);

        /* Compare hybrid and iterative gray seedfills */
    pixs1 = pixCopy(NULL, pixm);
    pixs2 = pixCopy(NULL, pixm);
    pixAddConstantGray(pixs1, -30);
    pixAddConstantGray(pixs2, 60);

    PixTestEqual(pixs1, pixs2, pixm, 1, 4);
    PixTestEqual(pixs1, pixs2, pixm, 2, 8);
    PixTestEqual(pixs2, pixs1, pixm, 3, 4);
    PixTestEqual(pixs2, pixs1, pixm, 4, 8);
    pixDestroy(&pixs1);
    pixDestroy(&pixs2);

    pixDestroy(&pixm);
    pixDestroy(&pixmi);
    pixDestroy(&pixmin);
    return 0;
}


void
PixTestEqual(PIX     *pixs1,
             PIX     *pixs2,
             PIX     *pixm,
             l_int32  set,
             l_int32  connectivity)
{
l_int32  same;
PIX     *pixc11, *pixc12, *pixc21, *pixc22, *pixmi;

    pixmi = pixInvert(NULL, pixm);
    pixc11 = pixCopy(NULL, pixs1);
    pixc12 = pixCopy(NULL, pixs1);
    pixc21 = pixCopy(NULL, pixs2);
    pixc22 = pixCopy(NULL, pixs2);

        /* Test inverse seed filling */
    pixSeedfillGrayInv(pixc11, pixm, connectivity);
    pixSeedfillGrayInvSimple(pixc12, pixm, connectivity);
    pixEqual(pixc11, pixc12, &same);
    if (same)
        fprintf(stderr, "\nSuccess for inv set %d\n", set);
    else
        fprintf(stderr, "\nFailure for inv set %d\n", set);

        /* Test seed filling */
    pixSeedfillGray(pixc21, pixm, connectivity);
    pixSeedfillGraySimple(pixc22, pixm, connectivity);
    pixEqual(pixc21, pixc22, &same);
    if (same)
        fprintf(stderr, "Success for set %d\n", set);
    else
        fprintf(stderr, "Failure for set %d\n", set);

       /* Display the filling results */
/*    pixDisplay(pixc11, 220 * (set - 1), 100);
    pixDisplay(pixc21, 220 * (set - 1), 320); */

    pixDestroy(&pixmi);
    pixDestroy(&pixc11);
    pixDestroy(&pixc12);
    pixDestroy(&pixc21);
    pixDestroy(&pixc22);
    return;
}


