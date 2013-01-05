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
 *  compfilter_reg.c
 *
 *     Tests filters that select components based on size, etc.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static void count_pieces(PIX  *pix, l_int32 nexp);
static void count_pieces2(BOXA *boxa, l_int32 nexp);
static l_int32 count_ones(NUMA  *na, l_int32 nexp, l_int32 index,
                          const char *name);

static const l_float32 edges[13] = {0.0, 0.2, 0.3, 0.35, 0.4, 0.45, 0.5,
                                    0.55, 0.6, 0.7, 0.8, 0.9, 1.0};

#if 1   /* for feyn.tif */
static const l_int32 band[12] = {1, 11, 48, 264, 574, 704, 908, 786, 466,
                                 157, 156, 230};
static const l_int32 total[12] = {1, 12, 60, 324, 898, 1602, 2510, 3296,
                                  3762, 3919, 4075, 4305};
#else   /* for rabi.png */
static const l_int32 band[12] = {24, 295, 490, 817, 1768, 962, 8171,
                                 63, 81, 51, 137, 8619};
static const l_int32 total[12] = {24, 319, 809, 1626, 3394, 4356, 12527,
                                  12590, 12671, 12722, 12859, 21478};
#endif


main(int    argc,
     char **argv)
{
l_int32      w, h, n, i, sum, sumi, empty;
BOX         *box1, *box2, *box3, *box4;
BOXA        *boxa, *boxat;
NUMA        *na1, *na2, *na3, *na4, *na5;
NUMA        *na2i, *na3i, *na4i, *nat, *naw, *nah;
PIX         *pixs, *pixc, *pixt, *pixt2, *pixd, *pixcount;
PIXA        *pixas, *pixad, *pixac;
static char  mainName[] = "compfilter_reg";

    pixDisplayWrite(NULL, -1);

        /* Draw 4 filled boxes of different sizes */
    pixs = pixCreate(200, 200, 1);
    box1 = boxCreate(10, 10, 20, 30);
    box2 = boxCreate(50, 10, 40, 20);
    box3 = boxCreate(110, 10, 35, 5);
    box4 = boxCreate(160, 10, 5, 15);
    boxa = boxaCreate(4);
    boxaAddBox(boxa, box1, L_INSERT);
    boxaAddBox(boxa, box2, L_INSERT);
    boxaAddBox(boxa, box3, L_INSERT);
    boxaAddBox(boxa, box4, L_INSERT);
    pixRenderBox(pixs, box1, 1, L_SET_PIXELS);
    pixRenderBox(pixs, box2, 1, L_SET_PIXELS);
    pixRenderBox(pixs, box3, 1, L_SET_PIXELS);
    pixRenderBox(pixs, box4, 1, L_SET_PIXELS);
    pixt = pixFillClosedBorders(pixs, 4);
    pixDisplayWrite(pixt, 1);
    pixt2 = pixCreateTemplate(pixs);
    pixRenderHashBox(pixt2, box1, 6, 4, L_POS_SLOPE_LINE, 1, L_SET_PIXELS);
    pixRenderHashBox(pixt2, box2, 7, 2, L_POS_SLOPE_LINE, 1, L_SET_PIXELS);
    pixRenderHashBox(pixt2, box3, 4, 2, L_VERTICAL_LINE, 1, L_SET_PIXELS);
    pixRenderHashBox(pixt2, box4, 3, 1, L_HORIZONTAL_LINE, 1, L_SET_PIXELS);
    pixDisplayWrite(pixt2, 1);

        /* Exercise the parameters */
    pixd = pixSelectBySize(pixt, 0, 22, 8, L_SELECT_HEIGHT,
                           L_SELECT_IF_GT, NULL);
    count_pieces(pixd, 1);
    pixd = pixSelectBySize(pixt, 0, 30, 8, L_SELECT_HEIGHT,
                           L_SELECT_IF_LT, NULL);
    count_pieces(pixd, 3);
    pixd = pixSelectBySize(pixt, 0, 5, 8, L_SELECT_HEIGHT,
                           L_SELECT_IF_GT, NULL);
    count_pieces(pixd, 3);
    pixd = pixSelectBySize(pixt, 0, 6, 8, L_SELECT_HEIGHT,
                           L_SELECT_IF_LT, NULL);
    count_pieces(pixd, 1);
    pixd = pixSelectBySize(pixt, 20, 0, 8, L_SELECT_WIDTH,
                           L_SELECT_IF_GT, NULL);
    count_pieces(pixd, 2);
    pixd = pixSelectBySize(pixt, 31, 0, 8, L_SELECT_WIDTH,
                           L_SELECT_IF_LT, NULL);
    count_pieces(pixd, 2);
    pixd = pixSelectBySize(pixt, 21, 10, 8, L_SELECT_IF_EITHER,
                           L_SELECT_IF_LT, NULL);
    count_pieces(pixd, 3);
    pixd = pixSelectBySize(pixt, 20, 30, 8, L_SELECT_IF_EITHER,
                           L_SELECT_IF_GT, NULL);
    count_pieces(pixd, 2);
    pixd = pixSelectBySize(pixt, 22, 32, 8, L_SELECT_IF_BOTH,
                           L_SELECT_IF_LT, NULL);
    count_pieces(pixd, 2);
    pixd = pixSelectBySize(pixt, 6, 32, 8, L_SELECT_IF_BOTH,
                           L_SELECT_IF_LT, NULL);
    count_pieces(pixd, 1);
    pixd = pixSelectBySize(pixt, 5, 25, 8, L_SELECT_IF_BOTH,
                           L_SELECT_IF_GT, NULL);
    count_pieces(pixd, 1);
    pixd = pixSelectBySize(pixt, 25, 5, 8, L_SELECT_IF_BOTH,
                           L_SELECT_IF_GT, NULL);
    count_pieces(pixd, 1);

    pixd = pixSelectByAreaPerimRatio(pixt, 1.7, 8, L_SELECT_IF_LT, NULL);
    count_pieces(pixd, 2);
    pixd = pixSelectByAreaPerimRatio(pixt, 5.5, 8, L_SELECT_IF_LT, NULL);
    count_pieces(pixd, 3);
    pixd = pixSelectByAreaPerimRatio(pixt, 1.5, 8, L_SELECT_IF_GTE, NULL);
    count_pieces(pixd, 2);
    pixd = pixSelectByAreaPerimRatio(pixt, 13.0/12.0, 8, L_SELECT_IF_GT, NULL);
    count_pieces(pixd, 3);

    pixd = pixSelectByAreaFraction(pixt2, 0.3, 8, L_SELECT_IF_LT, NULL);
    count_pieces(pixd, 0);
    pixd = pixSelectByAreaFraction(pixt2, 0.9, 8, L_SELECT_IF_LT, NULL);
    count_pieces(pixd, 4);
    pixd = pixSelectByAreaFraction(pixt2, 0.5, 8, L_SELECT_IF_GTE, NULL);
    count_pieces(pixd, 3);
    pixd = pixSelectByAreaFraction(pixt2, 0.7, 8, L_SELECT_IF_GT, NULL);
    count_pieces(pixd, 2);

    boxat = boxaSelectBySize(boxa, 21, 10, L_SELECT_IF_EITHER,
                             L_SELECT_IF_LT, NULL);
    count_pieces2(boxat, 3);
    boxat = boxaSelectBySize(boxa, 22, 32, L_SELECT_IF_BOTH,
                             L_SELECT_IF_LT, NULL);
    count_pieces2(boxat, 2);

    boxaDestroy(&boxa);
    pixDestroy(&pixt);
    pixDestroy(&pixt2);
    pixDestroy(&pixs);

        /* Here's the most general method for selecting components.
         * We do it for area fraction, but any combination of
         * size, area/perimeter ratio and area fraction can be used. */
    pixs = pixRead("feyn.tif");
/*    pixs = pixRead("rabi.png"); */
    pixc = pixCopy(NULL, pixs);  /* subtract bands from this */
    pixt = pixCreateTemplate(pixs);  /* add bands to this */
    pixGetDimensions(pixs, &w, &h, NULL);
    boxa = pixConnComp(pixs, &pixas, 8);
    n = boxaGetCount(boxa);
    fprintf(stderr, "total: %d\n", n);
    na1 = pixaFindAreaFraction(pixas);
    nat = numaCreate(0);
    numaSetCount(nat, n);  /* initialize to all 0 */
    sum = sumi = 0;
    pixac = pixaCreate(0);
    for (i = 0; i < 12; i++) {
            /* Compute within the intervals using an intersection. */
        na2 = numaMakeThresholdIndicator(na1, edges[i], L_SELECT_IF_GTE);
        if (i != 11)
            na3 = numaMakeThresholdIndicator(na1, edges[i + 1], L_SELECT_IF_LT);
        else
            na3 = numaMakeThresholdIndicator(na1, edges[i + 1],
                                             L_SELECT_IF_LTE);
        na4 = numaLogicalOp(NULL, na2, na3, L_INTERSECTION);
        sum += count_ones(na4, 0, 0, NULL);

            /* Compute outside the intervals using a union, and invert */
        na2i = numaMakeThresholdIndicator(na1, edges[i], L_SELECT_IF_LT);
        if (i != 11)
            na3i = numaMakeThresholdIndicator(na1, edges[i + 1],
                                              L_SELECT_IF_GTE);
        else
            na3i = numaMakeThresholdIndicator(na1, edges[i + 1],
                                              L_SELECT_IF_GT);
        na4i = numaLogicalOp(NULL, na3i, na2i, L_UNION);
        numaInvert(na4i, na4i);
        sumi += count_ones(na4i, 0, 0, NULL);

            /* Compare the two methods */
        if (sum == sumi)
            fprintf(stderr, "\nCorrect: sum = sumi = %d\n", sum);
        else
            fprintf(stderr, "\nWRONG: sum = %d, sumi = %d\n", sum, sumi);

            /* Reconstruct the image, band by band. */
        numaLogicalOp(nat, nat, na4, L_UNION);
        pixad = pixaSelectWithIndicator(pixas, na4, NULL);
        pixd = pixaDisplay(pixad, w, h);
        pixOr(pixt, pixt, pixd);  /* add them in */
        pixcount = pixCopy(NULL, pixt);  /* destroyed by count_pieces */
        count_ones(na4, band[i], i, "band");
        count_pieces(pixd, band[i]);
        count_ones(nat, total[i], i, "total");
        count_pieces(pixcount, total[i]);
        pixaDestroy(&pixad);

            /* Remove band successively from full image */
        pixRemoveWithIndicator(pixc, pixas, na4);
        pixSaveTiled(pixc, pixac, 4, 1 - i % 2, 25, 8);

        numaDestroy(&na2);
        numaDestroy(&na3);
        numaDestroy(&na4);
        numaDestroy(&na2i);
        numaDestroy(&na3i);
        numaDestroy(&na4i);
    }

        /* Did we remove all components from pixc? */
    pixZero(pixc, &empty);
    if (!empty)
        fprintf(stderr, "\nWRONG: not all pixels removed from pixc\n");

    pixDestroy(&pixs);
    pixDestroy(&pixc);
    pixDestroy(&pixt);
    boxaDestroy(&boxa);
    pixaDestroy(&pixas);
    numaDestroy(&na1);
    numaDestroy(&nat);

        /* One last extraction.  Get all components that have either
         * a height of at least 50 or a width of between 30 and 35,
         * and also do not have a large area/perimeter ratio. */
    pixs = pixRead("feyn.tif"); 
    boxa = pixConnComp(pixs, &pixas, 8);
    n = boxaGetCount(boxa);
    pixaFindDimensions(pixas, &naw, &nah);
    na1 = pixaFindAreaPerimRatio(pixas);
    na2 = numaMakeThresholdIndicator(nah, 50, L_SELECT_IF_GTE);
    na3 = numaMakeThresholdIndicator(naw, 30, L_SELECT_IF_GTE);
    na4 = numaMakeThresholdIndicator(naw, 35, L_SELECT_IF_LTE);
    na5 = numaMakeThresholdIndicator(na1, 2.5, L_SELECT_IF_LTE);
    numaLogicalOp(na3, na3, na4, L_INTERSECTION);
    numaLogicalOp(na2, na2, na3, L_UNION);
    numaLogicalOp(na2, na2, na5, L_INTERSECTION);
    numaInvert(na2, na2);  /* get components to be removed */
    pixRemoveWithIndicator(pixs, pixas, na2);
    pixSaveTiled(pixs, pixac, 4, 1, 25, 8);
    pixDestroy(&pixs);
    boxaDestroy(&boxa);
    pixaDestroy(&pixas);
    numaDestroy(&naw);
    numaDestroy(&nah);
    numaDestroy(&na1);
    numaDestroy(&na2);
    numaDestroy(&na3);
    numaDestroy(&na4);
    numaDestroy(&na5);


    pixDisplayMultiple("/tmp/junk_write_display*");

    pixd = pixaDisplay(pixac, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/junkcomp.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixac);

    return 0;
}


void count_pieces(PIX  *pix, l_int32 nexp)
{
l_int32  n;
BOXA    *boxa;

    pixDisplayWrite(pix, 1);
    boxa = pixConnComp(pix, NULL, 8);
    n = boxaGetCount(boxa);
    if (n == nexp)
        fprintf(stderr, "Correct: Num. comps: %d\n", n);
    else
        fprintf(stderr, "WRONG!: Num. comps: %d\n", n);
    boxaDestroy(&boxa);
    pixDestroy(&pix);
}

void count_pieces2(BOXA  *boxa, l_int32 nexp)
{
l_int32  n;

    n = boxaGetCount(boxa);
    if (n == nexp)
        fprintf(stderr, "Correct: Num. boxes: %d\n", n);
    else
        fprintf(stderr, "WRONG!: Num. boxes: %d\n", n);
    boxaDestroy(&boxa);
}

l_int32 count_ones(NUMA  *na, l_int32 nexp, l_int32 index, const char *name)
{
l_int32  i, n, val, sum;

    n = numaGetCount(na);
    sum = 0;
    for (i = 0; i < n; i++) {
        numaGetIValue(na, i, &val);
        if (val == 1) sum++;
    }
    if (!name) return sum;
    if (nexp == sum)
        fprintf(stderr, "Correct: %s[%d]: num. ones: %d\n", name, index, sum);
    else
        fprintf(stderr, "WRONG!: %s[%d]: num. ones: %d\n", name, index, sum);
    return 0;
}


