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
 * comparepages.c
 *
 *    This compares text pages using the location of word bounding boxes.
 *    The goal is to get a fast and robust determination for whether
 *    two pages are the same.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

main(int    argc,
     char **argv)
{
l_int32      w, h, n, same;
BOXA        *boxa1, *boxa2;
NUMA        *nai1, *nai2;
NUMAA       *naa1, *naa1r, *naa2;
PIX         *pixs, *pixt, *pixb1, *pixb2;
static char  mainName[] = "comparepages";

    pixs = pixRead("lucasta-47.jpg");
    pixb1 = pixConvertTo1(pixs, 128);
    pixGetWordBoxesInTextlines(pixb1, 1, 10, 10, 500, 50, &boxa1, &nai1);
    pixt = pixDrawBoxaRandom(pixs, boxa1, 2);
    pixDisplay(pixt, 100, 100);
    pixWrite("junkpixt", pixt, IFF_PNG);
    naa1 = boxaExtractSortedPattern(boxa1, nai1);
    numaaWrite("junknaa1", naa1);
    naa1r = numaaRead("junknaa1");
    numaaWrite("junknaa1r", naa1r);
    n = numaaGetCount(naa1);
    fprintf(stderr, "Number of textlines = %d\n", n);
    pixDisplay(pixb1, 300, 0);

        /* Translate */
    pixb2 = pixCreateTemplate(pixb1);
    pixGetDimensions(pixb1, &w, &h, NULL);
    pixRasterop(pixb2, 148, 133, w, h, PIX_SRC, pixb1, 0, 0);
    pixDisplay(pixb2, 600, 0);
    pixGetWordBoxesInTextlines(pixb2, 1, 10, 10, 500, 50, &boxa2, &nai2);
    naa2 = boxaExtractSortedPattern(boxa2, nai2);
    numaaCompareImagesByBoxes(naa1, naa2, 5, 10, 150, 150, 20, 20, &same, 1);
    fprintf(stderr, "Translation.  same?: %d\n\n", same);
    boxaDestroy(&boxa2);
    numaDestroy(&nai2);
    pixDestroy(&pixb2);
    numaaDestroy(&naa2);

        /* Aligned part is below h/3 */
    pixb2 = pixCreateTemplate(pixb1);
    pixGetDimensions(pixb1, &w, &h, NULL);
    pixRasterop(pixb2, 0, 0, w, h / 3, PIX_SRC, pixb1, 0, 2 * h / 3);
    pixRasterop(pixb2, 0, h / 3, w, 2 * h / 3, PIX_SRC, pixb1, 0, h / 3);
    pixDisplay(pixb2, 900, 0);
    pixGetWordBoxesInTextlines(pixb2, 1, 10, 10, 500, 50, &boxa2, &nai2);
    naa2 = boxaExtractSortedPattern(boxa2, nai2);
    numaaCompareImagesByBoxes(naa1, naa2, 5, 10, 150, 150, 20, 20, &same, 1);
    fprintf(stderr, "Aligned part below h/3.  same?: %d\n\n", same);
    boxaDestroy(&boxa2);
    numaDestroy(&nai2);
    pixDestroy(&pixb2);
    numaaDestroy(&naa2);

        /* Top and bottom switched; no aligned parts */
    pixb2 = pixCreateTemplate(pixb1);
    pixGetDimensions(pixb1, &w, &h, NULL);
    pixRasterop(pixb2, 0, 0, w, h / 3, PIX_SRC, pixb1, 0, 2 * h / 3);
    pixRasterop(pixb2, 0, h / 3, w, 2 * h / 3, PIX_SRC, pixb1, 0, 0);
    pixDisplay(pixb2, 1200, 0);
    pixGetWordBoxesInTextlines(pixb2, 1, 10, 10, 500, 50, &boxa2, &nai2);
    naa2 = boxaExtractSortedPattern(boxa2, nai2);
    numaaCompareImagesByBoxes(naa1, naa2, 5, 10, 150, 150, 20, 20, &same, 1);
    fprintf(stderr, "Top/Bot switched; no alignment.  Same?: %d\n", same);
    boxaDestroy(&boxa2);
    numaDestroy(&nai2);
    pixDestroy(&pixb2);
    numaaDestroy(&naa2);

    boxaDestroy(&boxa1);
    numaDestroy(&nai1);
    pixDestroy(&pixs);
    pixDestroy(&pixb1);
    pixDestroy(&pixt);
    numaaDestroy(&naa1);
    numaaDestroy(&naa1r);
    return 0;
}
