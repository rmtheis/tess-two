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
 * edgetest.c
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
l_int32      i, w, h, d;
l_float32    time;
PIX         *pixs, *pixf, *pixd;
PIXA        *pixa;
char        *filein, *fileout;
static char  mainName[] = "edgetest";

    if (argc != 3)
        return ERROR_INT(" Syntax:  edgetest filein fileout", mainName, 1);

    filein = argv[1];
    fileout = argv[2];

    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pix not made", mainName, 1);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8)
        return ERROR_INT("pix not 8 bpp", mainName, 1);

        /* Speed: about 12 Mpix/GHz/sec */
    startTimer();
    pixf = pixSobelEdgeFilter(pixs, L_HORIZONTAL_EDGES);
    pixd = pixThresholdToBinary(pixf, 60);
    pixInvert(pixd, pixd);
    time = stopTimer();
    fprintf(stderr, "Time =  %7.3f sec\n", time);
    fprintf(stderr, "MPix/sec: %7.3f\n", 0.000001 * w * h / time);
    pixDisplay(pixs, 0, 0);
    pixInvert(pixf, pixf);
    pixDisplay(pixf, 480, 0);
    pixDisplay(pixd, 960, 0);
    pixWrite(fileout, pixf, IFF_PNG);
    pixDestroy(&pixd);

        /* Threshold at different values */
    pixInvert(pixf, pixf);
    for (i = 10; i <= 120; i += 10) {
        pixd = pixThresholdToBinary(pixf, i);
        pixInvert(pixd, pixd);
        pixDisplayWrite(pixd, 1);
        pixDestroy(&pixd);
    }
    pixDestroy(&pixf);

        /* Display tiled */
    pixa = pixaReadFiles("/tmp/display", "file");
    pixd = pixaDisplayTiledAndScaled(pixa, 8, 400, 3, 0, 25, 2);
    pixWrite("/tmp/junktiles.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixDestroy(&pixs);
    return 0;
}

