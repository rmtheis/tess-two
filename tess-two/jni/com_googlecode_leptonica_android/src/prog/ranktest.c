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
 * ranktest.c
 *
 *     Tests rank filters on 8 and 32 bpp images.
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
l_int32      i, wf, hf, w, h, d, same;
l_float32    rank, time;
PIX         *pixs, *pixd, *pixt1, *pixt2, *pixt3, *pixt4;
PIXA        *pixa;
char        *filein, *fileout;
static char  mainName[] = "ranktest";

    if (argc != 6)
        return ERROR_INT(" Syntax:  ranktest filein wf hf rank fileout",
                         mainName, 1);

    filein = argv[1];
    wf = atoi(argv[2]);
    hf = atoi(argv[3]);
    rank = atof(argv[4]);
    fileout = argv[5];

    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pix not made", mainName, 1);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8 && d != 32)
        return ERROR_INT("pix neither 8 nor 32 bpp", mainName, 1);

    startTimer();
    pixd = pixRankFilter(pixs, wf, hf, rank);
    time = stopTimer();
    fprintf(stderr, "Time =  %7.3f sec\n", time);
    fprintf(stderr, "MPix/sec: %7.3f\n", 0.000001 * w * h / time);
    pixDisplay(pixs, 0, 0);
    pixDisplay(pixd, 600, 0);
    pixWrite(fileout, pixd, IFF_PNG);
    pixDestroy(&pixd);

        /* Get results for different rank values */
    for (i = 0; i <= 10; i++) {
        pixd = pixRankFilter(pixs, wf, hf, 0.1 * i);
        pixDisplayWrite(pixd, 1);
        pixDestroy(&pixd);
    }

        /* Make the dimensions odd to compare with dilation & erosion */
    if (wf % 2 == 0) wf++;
    if (hf % 2 == 0) hf++;

        /* Get results for dilation and erosion */
    if (d == 8) {
        pixt1 = pixDilateGray(pixs, wf, hf);
        pixt2 = pixErodeGray(pixs, wf, hf);
    } else {
        pixt1 = pixColorMorph(pixs, L_MORPH_DILATE, wf, hf);
        pixt2 = pixColorMorph(pixs, L_MORPH_ERODE, wf, hf);
    }
    pixDisplayWrite(pixt1, 1);  /* dilation */

        /* Get results using the rank filter for rank = 0.0 and 1.0.
         * Don't use 0.0 or 1.0, because those are dispatched
         * automatically to erosion and dilation! */
    pixt3 = pixRankFilter(pixs, wf, hf, 0.0001);
    pixt4 = pixRankFilter(pixs, wf, hf, 0.9999);

        /* Compare */
    pixEqual(pixt1, pixt4, &same);
    if (same)
        fprintf(stderr, "Correct: dilation results same as rank 1.0\n");
    else
        fprintf(stderr, "Error: dilation results differ from rank 1.0\n");
    pixEqual(pixt2, pixt3, &same);
    if (same)
        fprintf(stderr, "Correct: erosion results same as rank 0.0\n");
    else
        fprintf(stderr, "Error: erosion results differ from rank 0.0\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

        /* Display tiled */
    pixa = pixaReadFiles("/tmp/display", "file");
    pixd = pixaDisplayTiledAndScaled(pixa, d, 400, 3, 0, 25, 2);
    pixWrite("/tmp/junktiles.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixDestroy(&pixs);
    return 0;
}

