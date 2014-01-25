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
 *  overlap_reg.c
 *
 *    Tests the function that combines boxes that overlap into
 *    their bounding regions.
 */

#include "allheaders.h"

    /* Determines maximum size of boxes */
static const l_float32  maxsize[] = {5.0, 10.0, 15.0, 20.0, 25.0, 26.0, 27.0};


int main(int    argc,
         char **argv)
{
l_int32       i, k, x, y, w, h;
BOX          *box;
BOXA         *boxa1, *boxa2;
PIX          *pix1, *pix2, *pixd;
PIXA         *pixa;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
	return 1;

    for (k = 0; k < 7; k++) {
    srand(45617);
        pixa = pixaCreate(2);
        boxa1 = boxaCreate(0);
        for (i = 0; i < 500; i++) {
            x = (l_int32)(600.0 * (l_float64)rand() / (l_float64)RAND_MAX);
            y = (l_int32)(600.0 * (l_float64)rand() / (l_float64)RAND_MAX);
            w = (l_int32)
              (1.0 + maxsize[k] * (l_float64)rand() / (l_float64)RAND_MAX);
            h = (l_int32)
              (1.0 + maxsize[k] * (l_float64)rand() / (l_float64)RAND_MAX);
            box = boxCreate(x, y, w, h);
            boxaAddBox(boxa1, box, L_INSERT);
        }

        pix1 = pixCreate(660, 660, 1);
        pixRenderBoxa(pix1, boxa1, 1, L_SET_PIXELS);
        pixaAddPix(pixa, pix1, L_INSERT);
        boxa2 = boxaCombineOverlaps(boxa1);
        pix2 = pixCreate(660, 660, 1);
        pixRenderBoxa(pix2, boxa2, 1, L_SET_PIXELS);
        pixaAddPix(pixa, pix2, L_INSERT);

        pixd = pixaDisplayTiledInRows(pixa, 1, 1500, 1.0, 0, 50, 2);
        pixDisplayWithTitle(pixd, 100, 100 + 100 * k, NULL, rp->display);
        regTestWritePixAndCheck(rp, pixd, IFF_PNG);
        fprintf(stderr, "%d: n_init = %d, n_final = %d\n",
                k, boxaGetCount(boxa1), boxaGetCount(boxa2));
        pixDestroy(&pixd);
        boxaDestroy(&boxa1);
        boxaDestroy(&boxa2);
        pixaDestroy(&pixa);
    }

    return regTestCleanup(rp);
}
