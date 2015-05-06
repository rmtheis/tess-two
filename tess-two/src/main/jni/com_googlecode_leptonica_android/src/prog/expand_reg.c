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
 * expand_reg.c
 *
 */

#include "allheaders.h"

#define  BINARY_IMAGE             "test1.png"
#define  TWO_BPP_IMAGE_NO_CMAP    "weasel2.4g.png"
#define  TWO_BPP_IMAGE_CMAP       "weasel2.4c.png"
#define  FOUR_BPP_IMAGE_NO_CMAP   "weasel4.16g.png"
#define  FOUR_BPP_IMAGE_CMAP      "weasel4.16c.png"
#define  EIGHT_BPP_IMAGE_NO_CMAP  "weasel8.149g.png"
#define  EIGHT_BPP_IMAGE_CMAP     "weasel8.240c.png"
#define  RGB_IMAGE                "marge.jpg"


int main(int    argc,
         char **argv)
{
l_int32      i, w, h, same;
char         filename[][64] = {BINARY_IMAGE,
                               TWO_BPP_IMAGE_NO_CMAP, TWO_BPP_IMAGE_CMAP,
                               FOUR_BPP_IMAGE_NO_CMAP, FOUR_BPP_IMAGE_CMAP,
                               EIGHT_BPP_IMAGE_NO_CMAP, EIGHT_BPP_IMAGE_CMAP,
                               RGB_IMAGE};
BOX         *box;
PIX         *pix, *pixs, *pixt, *pixt1, *pixt2, *pixt3, *pixt4, *pixt5, *pixd;
static char  mainName[] = "expand_reg";

    if (argc != 1)
        return ERROR_INT(" Syntax:  expand_reg", mainName, 1);

    pixDisplayWrite(NULL, -1);
    for (i = 0; i < 8; i++) {
        pixs = pixRead(filename[i]);
        pixt = pixExpandReplicate(pixs, 2);
        pixDisplayWrite(pixt, 1);
        pixDestroy(&pixt);
        pixt = pixExpandReplicate(pixs, 3);
        pixDisplayWrite(pixt, 1);
        pixDestroy(&pixt);

        if (i == 4) {
            pixt = pixScale(pixs, 3.0, 3.0);
            pixWrite("/tmp/junkpixt.png", pixt, IFF_PNG);
            pixDestroy(&pixt);
        }
        pixDestroy(&pixs);
    }

    pix = pixRead("test1.png");
    pixGetDimensions(pix, &w, &h, NULL);
    for (i = 1; i <= 15; i++) {
        box = boxCreate(13 * i, 13 * i, w - 13 * i, h - 13 * i);
        pixs = pixClipRectangle(pix, box, NULL);
        pixt = pixExpandReplicate(pixs, 3);
        pixDisplayWrite(pixt, 1);
        boxDestroy(&box);
        pixDestroy(&pixt);
        pixDestroy(&pixs);
    }
    pixDestroy(&pix);

    pixs = pixRead("speckle.png");
        /* Test 2x expansion of 1 bpp */
    pixt = pixExpandBinaryPower2(pixs, 2);
    pixDisplayWrite(pixt, 1);
    pixd = pixReduceRankBinary2(pixt, 4, NULL);
    pixEqual(pixs, pixd, &same);
    if (!same)
        fprintf(stderr, "Error in 2x 1bpp expansion\n");
    pixDestroy(&pixt);
    pixDestroy(&pixd);
        /* Test 2x expansion of 2 bpp */
    pixt1 = pixConvert1To2(NULL, pixs, 3, 0);
    pixt2 = pixExpandReplicate(pixt1, 2);
    pixDisplayWrite(pixt2, 1);
    pixt3 = pixConvertTo8(pixt2, FALSE);
    pixt4 = pixThresholdToBinary(pixt3, 250);
    pixd = pixReduceRankBinary2(pixt4, 4, NULL);
    pixEqual(pixs, pixd, &same);
    if (!same)
        fprintf(stderr, "Error in 2x 2bpp expansion\n");
    pixt5 = pixExpandBinaryPower2(pixd, 2);
    pixDisplayWrite(pixt5, 1);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);
    pixDestroy(&pixd);
        /* Test 4x expansion of 4 bpp */
    pixt1 = pixConvert1To4(NULL, pixs, 15, 0);
    pixt2 = pixExpandReplicate(pixt1, 4);
    pixDisplayWrite(pixt2, 2);
    pixt3 = pixConvertTo8(pixt2, FALSE);
    pixt4 = pixThresholdToBinary(pixt3, 250);
    pixDisplayWrite(pixt4, 2);
    pixd = pixReduceRankBinaryCascade(pixt4, 4, 4, 0, 0);
    pixEqual(pixs, pixd, &same);
    if (!same)
        fprintf(stderr, "Error in 4x 4bpp expansion\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixd);
        /* Test 8x expansion of 8 bpp */
    pixt1 = pixConvertTo8(pixs, FALSE);
    pixt2 = pixExpandReplicate(pixt1, 8);
    pixDisplayWrite(pixt2, 4);
    pixt3 = pixThresholdToBinary(pixt2, 250);
    pixDisplayWrite(pixt3, 4);
    pixd = pixReduceRankBinaryCascade(pixt3, 4, 4, 4, 0);
    pixEqual(pixs, pixd, &same);
    if (!same)
        fprintf(stderr, "Error in 4x 4bpp expansion\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixd);
    pixDestroy(&pixs);

    pixDisplayMultiple("/tmp/display/file*");
    return 0;
}

