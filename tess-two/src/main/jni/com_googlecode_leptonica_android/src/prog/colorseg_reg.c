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
 * colorseg_reg.c
 *
 *   This explores the space of the four parameters input for color
 *   segmentation.  Of the four, only two strongly determine the
 *   output result:
 *      maxdist (the maximum distance between pixels that get
 *               clustered: 20 is very small, 180 is very large)
 *      selsize (responsible for smoothing the result: 0 is no
 *               smoothing (fine texture), 8 is large smoothing)
 *
 *   For large selsize (>~ 6), large regions get the same color,
 *   and there are few colors in the final result.
 *
 *   The other two parameters, maxcolors and finalcolors, can be
 *   set small (~4) or large (~20).  When set large, @maxdist will
 *   be most influential in determining the actual number of colors.
 */

#include "allheaders.h"

static const l_int32  MaxColors[] = {4, 8, 16};
static const l_int32  FinalColors[] = {4, 8, 16};

int main(int    argc,
         char **argv)
{
char         namebuf[256];
l_int32      i, j, k, maxdist, maxcolors, selsize, finalcolors;
PIX         *pixs, *pixt, *pixd;
PIXA        *pixa;
static char  mainName[] = "colorseg_reg";

    if (argc != 1)
        return ERROR_INT("Syntax: colorseg_reg", mainName, 1);

    pixs = pixRead("tetons.jpg");
    for (k = 0; k < 3; k++) {
        maxcolors = MaxColors[k];
        finalcolors = FinalColors[k];
        pixa = pixaCreate(0);
        pixSaveTiled(pixs, pixa, 1.0, 1, 15, 32);
        for (i = 1; i <= 9; i++) {
            maxdist = 20 * i;
            for (j = 0; j <= 6; j++) {
                selsize = j;
                pixt = pixColorSegment(pixs, maxdist, maxcolors, selsize,
                                       finalcolors);
                pixSaveTiled(pixt, pixa, 1.0, j == 0 ? 1 : 0, 15, 32);
                pixDestroy(&pixt);
            }
        }

        pixd = pixaDisplay(pixa, 0, 0);
        pixDisplay(pixd, 100, 100);
        sprintf(namebuf, "/tmp/junkcolorseg%d.jpg", k);
        pixWrite(namebuf, pixd, IFF_JFIF_JPEG);
        pixDestroy(&pixd);
        pixaDestroy(&pixa);
    }

    pixDestroy(&pixs);
    return 0;
}


