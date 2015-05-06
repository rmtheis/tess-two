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
 *  adaptnorm_reg.c
 *
 *    Image normalization for two extreme cases:
 *       * variable and low contrast
 *       * good contrast but fast varying background
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
l_int32      w, h;
l_float32    mps;
PIX         *pixs, *pixt, *pixmin, *pixd;
PIX         *pixt1, *pixt2, *pixt3, *pixt4, *pixt5, *pixt6;
PIX         *pixt7, *pixt8, *pixt9, *pixt10, *pixt11, *pixt12;
PIX         *pixt13, *pixt14, *pixt15, *pixt16;
PIXA        *pixac;
static char  mainName[] = "adaptnorm_reg";

    /* ---------------------------------------------------------- *
     *     Normalize by adaptively expanding the dynamic range    *
     * ---------------------------------------------------------- */
    pixac = pixaCreate(0);
    pixs = pixRead("lighttext.jpg");
    pixGetDimensions(pixs, &w, &h, NULL);
    pixSaveTiled(pixs, pixac, 1.0, 1, 20, 8);
    startTimer();
    pixt1 = pixContrastNorm(NULL, pixs, 10, 10, 40, 2, 2);
    mps = 0.000001 * w * h / stopTimer();
    fprintf(stderr, "Time: Contrast norm: %7.3f Mpix/sec\n", mps);
    pixSaveTiled(pixt1, pixac, 1.0, 1, 40, 8);
    pixWrite("/tmp/pixt1.png", pixt1, IFF_PNG);

         /* Apply a gamma to clean up the remaining background */
    pixt2 = pixGammaTRC(NULL, pixt1, 1.5, 50, 235);
    pixSaveTiled(pixt2, pixac, 1.0, 0, 40, 8);
    pixWrite("/tmp/pixt2.png", pixt2, IFF_PNG);

         /* Here are two possible output display images; a dithered
          * 2 bpp image and a 7 level thresholded 4 bpp image */
    pixt3 = pixDitherTo2bpp(pixt2, 1);
    pixSaveTiled(pixt3, pixac, 1.0, 0, 40, 8);
    pixWrite("/tmp/pixt3.png", pixt3, IFF_PNG);
    pixt4 = pixThresholdTo4bpp(pixt2, 7, 1);
    pixSaveTiled(pixt4, pixac, 1.0, 0, 40, 8);
    pixWrite("/tmp/pixt4.png", pixt4, IFF_PNG);

         /* Binary image produced from 8 bpp normalized ones,
          * before and after the gamma correction. */
    pixt5 = pixThresholdToBinary(pixt1, 180);
    pixSaveTiled(pixt5, pixac, 1.0, 1, 40, 8);
    pixWrite("/tmp/pixt5.png", pixt5, IFF_PNG);
    pixt6 = pixThresholdToBinary(pixt2, 200);
    pixSaveTiled(pixt6, pixac, 1.0, 0, 40, 8);
    pixWrite("/tmp/pixt6.png", pixt6, IFF_PNG);

    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);
    pixDestroy(&pixt6);

    pixd = pixaDisplay(pixac, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/norm.png", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixaDestroy(&pixac);


    /* ---------------------------------------------------------- *
     *          Normalize for rapidly varying background          *
     * ---------------------------------------------------------- */
    pixac = pixaCreate(0);
    pixs = pixRead("w91frag.jpg");
    pixGetDimensions(pixs, &w, &h, NULL);
    pixSaveTiled(pixs, pixac, 1.0, 1, 20, 8);
    startTimer();
    pixt7 = pixBackgroundNormFlex(pixs, 7, 7, 1, 1, 10);
    mps = 0.000001 * w * h / stopTimer();
    fprintf(stderr, "Time: Flexible bg norm: %7.3f Mpix/sec\n", mps);
    pixSaveTiled(pixt7, pixac, 1.0, 0, 40, 8);
    pixWrite("/tmp/pixt7.png", pixt7, IFF_PNG);

        /* Now do it again in several steps */
    pixt8 = pixScaleSmooth(pixs, 1./7., 1./7.);
    pixt = pixScale(pixt8, 7.0, 7.0);
    pixSaveTiled(pixt, pixac, 1.0, 1, 20, 8);
    pixDestroy(&pixt);
    pixLocalExtrema(pixt8, 0, 0, &pixmin, NULL);  /* 1's at minima */
    pixt9 = pixExpandBinaryReplicate(pixmin, 7);
    pixSaveTiled(pixt9, pixac, 1.0, 0, 20, 8);
    pixt10 = pixSeedfillGrayBasin(pixmin, pixt8, 10, 4);
    pixt11 = pixExtendByReplication(pixt10, 1, 1);
    pixt12 = pixGetInvBackgroundMap(pixt11, 200, 1, 1);  /* smoothing incl. */
    pixt13 = pixApplyInvBackgroundGrayMap(pixs, pixt12, 7, 7);
    pixSaveTiled(pixt13, pixac, 1.0, 0, 20, 8);

        /* Process the result for gray and binary output */
    pixt14 = pixGammaTRCMasked(NULL, pixt7, NULL, 1.0, 100, 175);
    pixSaveTiled(pixt14, pixac, 1.0, 1, 20, 8);
    pixt15 = pixThresholdTo4bpp(pixt14, 10, 1);
    pixSaveTiled(pixt15, pixac, 1.0, 0, 20, 8);
    pixt16 = pixThresholdToBinary(pixt14, 190);
    pixSaveTiled(pixt16, pixac, 1.0, 0, 20, 8);

    pixDestroy(&pixs);
    pixDestroy(&pixt7);
    pixDestroy(&pixmin);
    pixDestroy(&pixt8);
    pixDestroy(&pixt9);
    pixDestroy(&pixt10);
    pixDestroy(&pixt11);
    pixDestroy(&pixt12);
    pixDestroy(&pixt13);
    pixDestroy(&pixt14);
    pixDestroy(&pixt15);
    pixDestroy(&pixt16);

    pixd = pixaDisplay(pixac, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/flex.png", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixaDestroy(&pixac);
    return 0;
}


