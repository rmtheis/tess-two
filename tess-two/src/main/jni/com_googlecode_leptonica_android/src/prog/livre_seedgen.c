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
 * livre_seedgen.c
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
l_int32      i;
PIX         *pixs, *pixt1, *pixt2, *pixt3, *pixd;
PIXA        *pixa;

    pixs = pixRead("pageseg2.tif");

    startTimer();
    for (i = 0; i < 100; i++) {
        pixt1 = pixReduceRankBinaryCascade(pixs, 1, 4, 4, 3);
        pixDestroy(&pixt1);
    }
    fprintf(stderr, "Time: %8.4f sec\n", stopTimer() / 100.);

        /* 4 2x rank reductions (levels 1, 4, 4, 3), followed by 5x5 opening */
    pixDisplayWrite(NULL, -1);
    pixDisplayWriteFormat(pixs, 4, IFF_PNG);
    pixt1 = pixReduceRankBinaryCascade(pixs, 1, 4, 0, 0);
    pixDisplayWriteFormat(pixt1, 1, IFF_PNG);
    pixt2 = pixReduceRankBinaryCascade(pixt1, 4, 3, 0, 0);
    pixDisplayWriteFormat(pixt2, 1, IFF_PNG);
    pixOpenBrick(pixt2, pixt2, 5, 5);
    pixt3 = pixExpandBinaryReplicate(pixt2, 2);
    pixDisplayWriteFormat(pixt3, 1, IFF_PNG);

        /* Generate the output image */
    pixa = pixaReadFiles("/tmp/display", "file");
    pixd = pixaDisplayTiledAndScaled(pixa, 8, 250, 4, 0, 25, 2);
    pixWrite("/tmp/seedgen.png", pixd, IFF_PNG);

    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    return 0;
}

