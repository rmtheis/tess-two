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
 * pagesegtest1.c
 *
 *   Use on, e.g.:   feyn.tif, witten.tif,
 *                   pageseg1.tif, pageseg2.tif, pageseg3.tif, pageseg4.tif
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
PIX         *pixs, *pixhm, *pixtm, *pixtb, *pixd;
PIXA        *pixa;
char        *filein;
static char  mainName[] = "pagesegtest1";

    if (argc != 2)
        return ERROR_INT(" Syntax:  pagesegtest1 filein", mainName, 1);

    filein = argv[1];
    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);

    pixGetRegionsBinary(pixs, &pixhm, &pixtm, &pixtb, 1);
    pixDestroy(&pixhm);
    pixDestroy(&pixtm);
    pixDestroy(&pixtb);
    pixDestroy(&pixs);

        /* Display intermediate images in a single image */
    pixa = pixaReadFiles("/tmp", "junk_write");
    pixd = pixaDisplayTiledAndScaled(pixa, 32, 400, 4, 0, 20, 3);
    pixWrite("/tmp/segstuff.png", pixd, IFF_PNG);
    pixaDestroy(&pixa);
    pixDestroy(&pixd);
    return 0;
}

