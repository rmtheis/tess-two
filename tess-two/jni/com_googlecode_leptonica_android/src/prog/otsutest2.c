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
 *   otsutest2.c
 *
 *   This demonstrates the usefulness of the modified version of Otsu
 *   for thresholding an image that doesn't have a well-defined
 *   background color.
 *
 *   Standard Otsu binarization is done with scorefract = 0.0, which
 *   returns the threshold at the maximum value of the score.  However.
 *   this value is up on the shoulder of the background, and its
 *   use causes some of the dark background to be binarized as foreground.
 *
 *   Using the modified Otsu with scorefract = 0.1 returns a threshold
 *   at the lowest value of this histogram such that the score
 *   is at least 0.9 times the maximum value of the score.  This allows
 *   the threshold to be taken in the histogram minimum between
 *   the fg and bg peaks, producing a much cleaner binarization.
 */

#ifndef  _WIN32
#include <unistd.h>
#else
#include <windows.h>   /* for Sleep() */
#endif  /* _WIN32 */

#include "allheaders.h"


int main(int    argc,
         char **argv)
{
char       textstr[256];
l_int32    i, thresh, fgval, bgval;
l_float32  scorefract;
L_BMF     *bmf;
PIX       *pixs, *pixb, *pixb2, *pixb3, *pixg, *pixp, *pixt1, *pixt2;
PIXA      *pixa;

    pixs = pixRead("1555.007.jpg");
    pixg = pixConvertTo8(pixs, 0);
    bmf = bmfCreate("fonts", 8);
    for (i = 0; i < 3; i++) {
        pixa = pixaCreate(3);
        scorefract = 0.1 * i;
        pixOtsuAdaptiveThreshold(pixg, 2000, 2000, 0, 0, scorefract,
                                 NULL, &pixb);
        pixSaveTiledOutline(pixb, pixa, 0.5, 1, 20, 2, 32);
        pixSplitDistributionFgBg(pixg, scorefract, 1, &thresh, &fgval, &bgval, 1);
        fprintf(stderr, "thresh = %d, fgval = %d, bgval = %d\n", thresh, fgval,
                 bgval);

        /* Give gnuplot time to write out the plot */
#ifndef  _WIN32
    sleep(1);
#else
    Sleep(1000);
#endif  /* _WIN32 */

        pixp = pixRead("/tmp/histplot.png");
        pixSaveTiled(pixp, pixa, 1.0, 0, 20, 1);
        pixt1 = pixaDisplay(pixa, 0, 0);
        snprintf(textstr, sizeof(textstr),
             "Scorefract = %3.1f ........... Thresh = %d", scorefract, thresh);
        pixt2 = pixAddSingleTextblock(pixt1, bmf, textstr, 0x00ff0000,
                                      L_ADD_BELOW, NULL);
        pixDisplay(pixt2, 100, 100);
        snprintf(textstr, sizeof(textstr), "/tmp/otsu.%d.png", i);
        pixWrite(textstr, pixt2, IFF_PNG);
        pixDestroy(&pixb);
        pixDestroy(&pixp);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixaDestroy(&pixa);
    }

    pixa = pixaCreate(2);
    for (i = 0; i < 2; i++) {
        scorefract = 0.1 * i;
        pixOtsuAdaptiveThreshold(pixg, 300, 300, 0, 0, scorefract,
                                 NULL, &pixb);
        pixb2 = pixAddBlackOrWhiteBorder(pixb, 2, 2, 2, 2, L_GET_BLACK_VAL);
        snprintf(textstr, sizeof(textstr),
             "Scorefract = %3.1f", scorefract);
        pixb3 = pixAddSingleTextblock(pixb2, bmf, textstr, 1,
                                      L_ADD_BELOW, NULL);
        pixSaveTiled(pixb3, pixa, 2, (i + 1) % 1, 20, 32);
        pixDestroy(&pixb);
        pixDestroy(&pixb2);
    }
    pixb = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/otsu-tiled.jpg", pixb, IFF_PNG);
    pixDestroy(&pixb);
    pixaDestroy(&pixa);

    bmfDestroy(&bmf);
    pixDestroy(&pixs);
    pixDestroy(&pixg);
    return 0;
}
