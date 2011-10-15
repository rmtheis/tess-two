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
 *  overlap_reg.c
 *
 *    Tests the function that combines boxes that overlap into
 *    their bounding regions.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

    /* Determines maximum size of boxes */
static const l_float32  maxsize[] = {5.0, 10.0, 15.0, 20.0, 25.0, 26.0, 27.0};


main(int    argc,
     char **argv)
{
l_int32       i, k, x, y, w, h, count, success, display;
BOX          *box;
BOXA         *boxa1, *boxa2;
FILE         *fp;
PIX          *pix1, *pix2, *pixd;
PIXA         *pixa;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &fp, &display, &success, &rp))
	return 1;

    count = 0;
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
        pixDisplayWithTitle(pixd, 100, 100 + 100 * k, NULL, display);
        regTestWritePixAndCheck(pixd, IFF_PNG, &count, rp);
        fprintf(stderr, "%d: n_init = %d, n_final = %d\n",
                k, boxaGetCount(boxa1), boxaGetCount(boxa2));
        pixDestroy(&pixd);
        boxaDestroy(&boxa1);
        boxaDestroy(&boxa2);
        pixaDestroy(&pixa);
    }

    regTestCleanup(argc, argv, fp, success, rp);
    return 0;
}


