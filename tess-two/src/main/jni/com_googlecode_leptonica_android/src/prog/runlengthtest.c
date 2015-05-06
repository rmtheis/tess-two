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
 * runlengthtest.c
 *
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
PIX         *pixs, *pixh, *pixv, *pix, *pixd;
char        *filein, *fileout;
static char  mainName[] = "runlengthtest";

    if (argc != 3)
        return ERROR_INT(" Syntax:  runlengthtest filein fileout", mainName, 1);

    filein = argv[1];
    fileout = argv[2];

    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);

    startTimer();
    pixh = pixRunlengthTransform(pixs, 0, L_HORIZONTAL_RUNS, 8);
    pixv = pixRunlengthTransform(pixs, 0, L_VERTICAL_RUNS, 8);
    pix = pixMinOrMax(NULL, pixh, pixv, L_CHOOSE_MAX);
    pixd = pixMaxDynamicRange(pix, L_LINEAR_SCALE);
    fprintf(stderr, "Total time: %7.3f sec\n", stopTimer());
    pixDisplay(pixh, 0, 0);
    pixDisplay(pixv, 400, 0);
    pixDisplay(pix, 800, 0);
    pixDisplay(pixd, 1200, 0);
    pixWrite("/tmp/junkpixh.png", pixh, IFF_PNG);
    pixWrite("/tmp/junkpixv.png", pixv, IFF_PNG);
    pixWrite("/tmp/junkpix.png", pix, IFF_PNG);
    pixWrite(fileout, pixd, IFF_PNG);

    pixDestroy(&pixs);
    pixDestroy(&pixh);
    pixDestroy(&pixv);
    pixDestroy(&pix);
    pixDestroy(&pixd);
    return 0;
}

