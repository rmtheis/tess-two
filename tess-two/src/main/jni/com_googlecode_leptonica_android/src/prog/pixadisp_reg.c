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
 *   pixadisp_reg.c
 *
 *     Exercises various pixaDisplay*() functions.
 *
 *     Syntax:  pixadisp_reg
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
l_int32      ws, hs, ncols;
BOX         *box;
BOXA        *boxa;
PIX         *pixs, *pixc, *pix32, *pixt, *pixd;
PIXA        *pixat, *pixas, *pixac;
static char  mainName[] = "pixadisp_reg";

    if (argc != 1)
        return ERROR_INT(" Syntax: pixadisp_reg", mainName, 1);

    if ((pixs = pixRead("feyn.tif")) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);
    box = boxCreate(683, 799, 970, 479);
    pixc = pixClipRectangle(pixs, box, NULL);
    boxDestroy(&box);
    pixDisplayWrite(pixc, 1);
    if ((pix32 = pixRead("marge.jpg")) == NULL)
        return ERROR_INT("pix32 not made", mainName, 1);

        /* Generate pixas from pixs and pixac from pixc */
    boxa = pixConnComp(pixs, &pixat, 8);
    pixas = pixaSelectBySize(pixat, 60, 60, L_SELECT_IF_BOTH,
                             L_SELECT_IF_LTE, NULL);
    pixaDestroy(&pixat);
    boxaDestroy(&boxa);
    boxa = pixConnComp(pixc, &pixac, 8);
    boxaDestroy(&boxa);

        /* pixaDisplay() */
    pixGetDimensions(pixs, &ws, &hs, NULL);
    pixd = pixaDisplay(pixas, ws, hs);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);

        /* pixaDisplayRandomCmap() */
    pixd = pixaDisplayRandomCmap(pixas, ws, hs);  /* black bg */
    pixDisplayWrite(pixd, 1);
    pixcmapResetColor(pixGetColormap(pixd), 0, 255, 255, 255);  /* white bg */
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);

        /* pixaDisplayOnLattice() */
    pixd = pixaDisplayOnLattice(pixac, 50, 50, &ncols, &boxa);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);
    fprintf(stderr, "Number of columns = %d; number of boxes: %d\n",
            ncols, boxaGetCount(boxa));
    boxaDestroy(&boxa);

        /* pixaDisplayUnsplit() */
    pixat = pixaSplitPix(pix32, 5, 7, 10, 0x0000ff00);
    pixd = pixaDisplayUnsplit(pixat, 5, 7, 10, 0x00ff0000);
    pixDisplayWrite(pixd, 1);
    pixaDestroy(&pixat);
    pixDestroy(&pixd);

        /* pixaDisplayTiled() */
    pixd = pixaDisplayTiled(pixac, 1000, 0, 10);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);

        /* pixaDisplayTiledInRows() */
    pixd = pixaDisplayTiledInRows(pixac, 1, 1000, 1.0, 0, 10, 2);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);


        /* pixaDisplayTiledAndScaled() */
    pixd = pixaDisplayTiledAndScaled(pixac, 1, 25, 20, 0, 5, 0);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);

    pixat = pixaCreate(10);
    pixd = pixRankFilter(pix32, 8, 8, 0.5);
    pixaAddPix(pixat, pixd, L_INSERT);
    pixt = pixScale(pix32, 0.5, 0.5);
    pixd = pixRankFilter(pixt, 8, 8, 0.5);
    pixaAddPix(pixat, pixd, L_INSERT);
    pixDestroy(&pixt);
    pixt = pixScale(pix32, 0.25, 0.25);
    pixd = pixRankFilter(pixt, 8, 8, 0.5);
    pixaAddPix(pixat, pixd, L_INSERT);
    pixDestroy(&pixt);
    pixd = pixaDisplayTiledAndScaled(pixat, 32, 500, 1, 0, 25, 0);
    pixDisplayWrite(pixd, 1);
    pixaDestroy(&pixat);
    pixDestroy(&pixd);

    pixaDestroy(&pixas);
    pixaDestroy(&pixac);
    pixDestroy(&pixs);
    pixDestroy(&pixc);
    pixDestroy(&pix32);

    pixDisplayMultiple("/tmp/display/file*");
    return 0;
}
