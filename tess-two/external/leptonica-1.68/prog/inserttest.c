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
 * inserttest.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
l_int32     i, n;
l_float32   pi, angle, val;
BOX        *box;
BOXA       *boxa, *boxa1, *boxa2;
NUMA       *na1, *na2;
PIX        *pix, *pix1, *pix2, *pix3, *pixd;
PIXA       *pixa1, *pixa2, *pixa3, *pixa4;
static char     mainName[] = "inserttest";

#if 1
    pi = 3.1415926535;
    na1 = numaCreate(500);
    for (i = 0; i < 500; i++) {
        angle = 0.02293 * i * pi;
        val = (l_float32)sin(angle);
        numaAddNumber(na1, val);
    }
    numaWrite("/tmp/junknuma1", na1);
    na2 = numaCopy(na1);
    n = numaGetCount(na2);
    for (i = 0; i < n; i++) {
      numaGetFValue(na2, i, &val);
      numaRemoveNumber(na2, i);
      numaInsertNumber(na2, i, val);
    }
    numaWrite("/tmp/junknuma2", na2);
    numaDestroy(&na1);
    numaDestroy(&na2);
#endif

#if 1
    pix1 = pixRead("feyn.tif");
    box = boxCreate(1138, 1666, 1070, 380);
    pix2 = pixClipRectangle(pix1, box, NULL);
    boxDestroy(&box);
    boxa1 = pixConnComp(pix2, NULL, 8);
    boxaWrite("/tmp/junkboxa1", boxa1);
    boxa2 = boxaCopy(boxa1, L_COPY);
    n = boxaGetCount(boxa2);
    for (i = 0; i < n; i++) {
      box = boxaGetBox(boxa2, i, L_COPY);
      boxaRemoveBox(boxa2, i);
      boxaInsertBox(boxa2, i, box);
    }
    boxaWrite("/tmp/junkboxa2", boxa2);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
#endif

#if 1
    pix1 = pixRead("feyn.tif");
    box = boxCreate(1138, 1666, 1070, 380);
    pix2 = pixClipRectangle(pix1, box, NULL);
    boxDestroy(&box);
    boxa = pixConnComp(pix2, &pixa1, 8);
    boxaDestroy(&boxa);
    pixaWrite("/tmp/junkpixa1", pixa1);

    pixa2 = pixaCopy(pixa1, L_COPY);
    n = pixaGetCount(pixa2);
        /* Remove and insert each one */
    for (i = 0; i < n; i++) {
      pix = pixaGetPix(pixa2, i, L_COPY);
      box = pixaGetBox(pixa2, i, L_COPY);
      pixaRemovePix(pixa2, i);
      pixaInsertPix(pixa2, i, pix, box);
    }
    pixaWrite("/tmp/junkpixa2", pixa2);

        /* Move the last to the beginning; do it n times */
    pixa3 = pixaCopy(pixa2, L_COPY);
    for (i = 0; i < n; i++) {
      pix = pixaGetPix(pixa3, n - 1, L_CLONE);
      box = pixaGetBox(pixa3, n - 1, L_CLONE);
      pixaInsertPix(pixa3, 0, pix, box);
      pixaRemovePix(pixa3, n);
    }
    pixaWrite("/tmp/junkpixa3", pixa3);

        /* Move the first one to the end; do it n times */
    pixa4 = pixaCopy(pixa3, L_COPY);
    for (i = 0; i < n; i++) {
      pix = pixaGetPix(pixa4, 0, L_CLONE);
      box = pixaGetBox(pixa4, 0, L_CLONE);
      pixaInsertPix(pixa4, n, pix, box);  /* make sure insert works at end */
      pixaRemovePix(pixa4, 0);
    }
    pixaWrite("/tmp/junkpixa4", pixa4);

    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixaDestroy(&pixa1);
    pixaDestroy(&pixa2);
    pixaDestroy(&pixa3);
    pixaDestroy(&pixa4);
#endif

    return 0;
}
