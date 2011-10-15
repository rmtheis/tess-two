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

#include <stdio.h>
#include <stdlib.h>

#ifndef  _WIN32
#include <unistd.h>
#else
    /* Need declaration of Sleep() defined in WinBase.h, but must
     * include Windows.h to avoid errors  */
#include <Windows.h>
#endif  /* _WIN32 */

#include "allheaders.h"


main(int    argc,
     char **argv)
{
char       textstr[256];
l_int32    i, thresh, fgval, bgval;
l_float32  scorefract;
L_BMF     *bmf;
PIX       *pixs, *pixb, *pixb2, *pixb3, *pixg, *pixp, *pixt1, *pixt2;
PIXA      *pixa;

    pixs = pixRead("1555-7.jpg");
    pixg = pixConvertTo8(pixs, 0);
    bmf = bmfCreate("fonts", 8);
    for (i = 0; i < 3; i++) {
        pixa = pixaCreate(3);
        scorefract = 0.1 * i;
        pixOtsuAdaptiveThreshold(pixg, 2000, 2000, 0, 0, scorefract,
                                 NULL, &pixb);
        pixSaveTiledOutline(pixb, pixa, 2, 1, 20, 2, 32);
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
        pixSaveTiled(pixp, pixa, 1, 0, 20, 1);
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
        pixb2 = pixAddBlackBorder(pixb, 2);
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

