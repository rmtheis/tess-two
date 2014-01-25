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
 *   boxfunc1.c
 *
 *      Box geometry
 *           l_int32   boxContains()
 *           l_int32   boxIntersects()
 *           BOXA     *boxaContainedInBox()
 *           BOXA     *boxaIntersectsBox()
 *           BOXA     *boxaClipToBox()
 *           BOXA     *boxaCombineOverlaps()
 *           BOX      *boxOverlapRegion()
 *           BOX      *boxBoundingRegion()
 *           l_int32   boxOverlapFraction()
 *           l_int32   boxOverlapArea()
 *           BOXA     *boxaHandleOverlaps()
 *           l_int32   boxSeparationDistance()
 *           l_int32   boxContainsPt()
 *           BOX      *boxaGetNearestToPt()
 *           l_int32   boxIntersectByLine()
 *           l_int32   boxGetCenter()
 *           BOX      *boxClipToRectangle()
 *           l_int32   boxClipToRectangleParams()
 *           BOX      *boxRelocateOneSide()
 *           BOX      *boxAdjustSides()
 *           BOXA     *boxaSetSide()
 *           BOXA     *boxaAdjustWidthToTarget()
 *           BOXA     *boxaAdjustHeightToTarget()
 *           l_int32   boxEqual()
 *           l_int32   boxaEqual()
 *           l_int32   boxSimilar()
 *           l_int32   boxaSimilar()
 *
 *      Boxa combine and split
 *           l_int32   boxaJoin()
 *           l_int32   boxaaJoin()
 *           l_int32   boxaSplitEvenOdd()
 *           BOXA     *boxaMergeEvenOdd()
 */

#include "allheaders.h"


/*---------------------------------------------------------------------*
 *                             Box geometry                            *
 *---------------------------------------------------------------------*/
/*!
 *  boxContains()
 *
 *      Input:  box1, box2
 *              &result (<return> 1 if box2 is entirely contained within
 *                       box1, and 0 otherwise)
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxContains(BOX     *box1,
            BOX     *box2,
            l_int32 *presult)
{
    PROCNAME("boxContains");

    if (!box1 || !box2)
        return ERROR_INT("box1 and box2 not both defined", procName, 1);

    if ((box1->x <= box2->x) &&
        (box1->y <= box2->y) &&
        (box1->x + box1->w >= box2->x + box2->w) &&
        (box1->y + box1->h >= box2->y + box2->h))
        *presult = 1;
    else
        *presult = 0;
    return 0;
}


/*!
 *  boxIntersects()
 *
 *      Input:  box1, box2
 *              &result (<return> 1 if any part of box2 is contained
 *                      in box1, and 0 otherwise)
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxIntersects(BOX      *box1,
              BOX      *box2,
              l_int32  *presult)
{
l_int32  left1, left2, top1, top2, right1, right2, bot1, bot2;

    PROCNAME("boxIntersects");

    if (!box1 || !box2)
        return ERROR_INT("box1 and box2 not both defined", procName, 1);

    left1 = box1->x;
    left2 = box2->x;
    top1 = box1->y;
    top2 = box2->y;
    right1 = box1->x + box1->w - 1;
    bot1 = box1->y + box1->h - 1;
    right2 = box2->x + box2->w - 1;
    bot2 = box2->y + box2->h - 1;
    if ((bot2 >= top1) && (bot1 >= top2) &&
         (right1 >= left2) && (right2 >= left1))
        *presult = 1;
    else
        *presult = 0;
    return 0;
}


/*!
 *  boxaContainedInBox()
 *
 *      Input:  boxas
 *              box (for containment)
 *      Return: boxad (boxa with all boxes in boxas that are
 *                     entirely contained in box), or null on error
 *
 *  Notes:
 *      (1) All boxes in boxa that are entirely outside box are removed.
 */
BOXA *
boxaContainedInBox(BOXA  *boxas,
                   BOX   *box)
{
l_int32  i, n, val;
BOX     *boxt;
BOXA    *boxad;

    PROCNAME("boxaContainedInBox");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (!box)
        return (BOXA *)ERROR_PTR("box not defined", procName, NULL);
    if ((n = boxaGetCount(boxas)) == 0)
        return boxaCreate(1);  /* empty */

    boxad = boxaCreate(0);
    for (i = 0; i < n; i++) {
        boxt = boxaGetBox(boxas, i, L_CLONE);
        boxContains(box, boxt, &val);
        if (val == 1)
            boxaAddBox(boxad, boxt, L_COPY);
        boxDestroy(&boxt);  /* destroy the clone */
    }

    return boxad;
}


/*!
 *  boxaIntersectsBox()
 *
 *      Input:  boxas
 *              box (for intersecting)
 *      Return  boxad (boxa with all boxes in boxas that intersect box),
 *                     or null on error
 *
 *  Notes:
 *      (1) All boxes in boxa that intersect with box (i.e., are completely
 *          or partially contained in box) are retained.
 */
BOXA *
boxaIntersectsBox(BOXA  *boxas,
                  BOX   *box)
{
l_int32  i, n, val;
BOX     *boxt;
BOXA    *boxad;

    PROCNAME("boxaIntersectsBox");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (!box)
        return (BOXA *)ERROR_PTR("box not defined", procName, NULL);
    if ((n = boxaGetCount(boxas)) == 0)
        return boxaCreate(1);  /* empty */

    boxad = boxaCreate(0);
    for (i = 0; i < n; i++) {
        boxt = boxaGetBox(boxas, i, L_CLONE);
        boxIntersects(box, boxt, &val);
        if (val == 1)
            boxaAddBox(boxad, boxt, L_COPY);
        boxDestroy(&boxt);  /* destroy the clone */
    }

    return boxad;
}


/*!
 *  boxaClipToBox()
 *
 *      Input:  boxas
 *              box (for clipping)
 *      Return  boxad (boxa with boxes in boxas clipped to box),
 *                     or null on error
 *
 *  Notes:
 *      (1) All boxes in boxa not intersecting with box are removed, and
 *          the remaining boxes are clipped to box.
 */
BOXA *
boxaClipToBox(BOXA  *boxas,
              BOX   *box)
{
l_int32  i, n;
BOX     *boxt, *boxo;
BOXA    *boxad;

    PROCNAME("boxaClipToBox");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (!box)
        return (BOXA *)ERROR_PTR("box not defined", procName, NULL);
    if ((n = boxaGetCount(boxas)) == 0)
        return boxaCreate(1);  /* empty */

    boxad = boxaCreate(0);
    for (i = 0; i < n; i++) {
        boxt = boxaGetBox(boxas, i, L_CLONE);
        if ((boxo = boxOverlapRegion(box, boxt)) != NULL)
            boxaAddBox(boxad, boxo, L_INSERT);
        boxDestroy(&boxt);
    }

    return boxad;
}


/*!
 *  boxaCombineOverlaps()
 *
 *      Input:  boxas
 *      Return: boxad (where each set of boxes in boxas that overlap are
 *                     combined into a single bounding box in boxad), or
 *                     null on error.
 *
 *  Notes:
 *      (1) If there are no overlapping boxes, it simply returns a copy
 *          of @boxas.
 *      (2) The alternative method of painting each rectanle and finding
 *          the 4-connected components gives the wrong result, because
 *          two non-overlapping rectangles, when rendered, can still
 *          be 4-connected, and hence they will be joined.
 *      (3) A bad case is to have n boxes, none of which overlap.
 *          Then you have one iteration with O(n^2) compares.  This
 *          is still faster than painting each rectangle and finding
 *          the connected components, even for thousands of rectangles.
 */
BOXA *
boxaCombineOverlaps(BOXA  *boxas)
{
l_int32  i, j, n1, n2, inter, interfound, niters;
BOX     *box1, *box2, *box3;
BOXA    *boxat1, *boxat2;

    PROCNAME("boxaCombineOverlaps");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);

    boxat1 = boxaCopy(boxas, L_COPY);
    n1 = boxaGetCount(boxat1);
    niters = 0;
/*    fprintf(stderr, "%d iters: %d boxes\n", niters, n1); */
    while (1) {  /* loop until no change from previous iteration */
        niters++;
        boxat2 = boxaCreate(n1);
        for (i = 0; i < n1; i++) {
            box1 = boxaGetBox(boxat1, i, L_COPY);
            if (i == 0) {
                boxaAddBox(boxat2, box1, L_INSERT);
                continue;
            }
            n2 = boxaGetCount(boxat2);
                /* Now test box1 against all boxes already put in boxat2.
                 * If it is found to intersect with an existing box,
                 * replace that box by the union of the two boxes,
                 * and break to the outer loop.  If no overlap is
                 * found, add box1 to boxat2. */
            interfound = FALSE;
            for (j = 0; j < n2; j++) {
                box2 = boxaGetBox(boxat2, j, L_CLONE);
                boxIntersects(box1, box2, &inter);
                if (inter == 1) {
                    box3 = boxBoundingRegion(box1, box2);
                    boxaReplaceBox(boxat2, j, box3);
                    boxDestroy(&box1);
                    boxDestroy(&box2);
                    interfound = TRUE;
                    break;
                }
                boxDestroy(&box2);
            }
            if (interfound == FALSE)
                boxaAddBox(boxat2, box1, L_INSERT);
        }

        n2 = boxaGetCount(boxat2);
/*        fprintf(stderr, "%d iters: %d boxes\n", niters, n2); */
        if (n2 == n1)  /* we're done */
            break;

        n1 = n2;
        boxaDestroy(&boxat1);
        boxat1 = boxat2;
    }
    boxaDestroy(&boxat1);
    return boxat2;
}


/*!
 *  boxOverlapRegion()
 *
 *      Input:  box1, box2 (two boxes)
 *      Return: box (of overlap region between input boxes),
 *              or null if no overlap or on error
 */
BOX *
boxOverlapRegion(BOX  *box1,
                 BOX  *box2)
{
l_int32  x, y, w, h, left1, left2, top1, top2, right1, right2, bot1, bot2;

    PROCNAME("boxOverlapRegion");

    if (!box1)
        return (BOX *)ERROR_PTR("box1 not defined", procName, NULL);
    if (!box2)
        return (BOX *)ERROR_PTR("box2 not defined", procName, NULL);

    left1 = box1->x;
    left2 = box2->x;
    top1 = box1->y;
    top2 = box2->y;
    right1 = box1->x + box1->w - 1;
    bot1 = box1->y + box1->h - 1;
    right2 = box2->x + box2->w - 1;
    bot2 = box2->y + box2->h - 1;
    if ((bot2 < top1) || (bot1 < top2) ||
         (right1 < left2) || (right2 < left1))
        return NULL;

    x = (left1 > left2) ? left1 : left2;
    y = (top1 > top2) ? top1 : top2;
    w = L_MIN(right1 - x + 1, right2 - x + 1);
    h = L_MIN(bot1 - y + 1, bot2 - y + 1);
    return boxCreate(x, y, w, h);
}


/*!
 *  boxBoundingRegion()
 *
 *      Input:  box1, box2 (two boxes)
 *      Return: box (of bounding region containing the input boxes),
 *              or null on error
 */
BOX *
boxBoundingRegion(BOX  *box1,
                  BOX  *box2)
{
l_int32  left, top, right1, right2, right, bot1, bot2, bot;

    PROCNAME("boxBoundingRegion");

    if (!box1)
        return (BOX *)ERROR_PTR("box1 not defined", procName, NULL);
    if (!box2)
        return (BOX *)ERROR_PTR("box2 not defined", procName, NULL);

    left = L_MIN(box1->x, box2->x);
    top = L_MIN(box1->y, box2->y);
    right1 = box1->x + box1->w - 1;
    right2 = box2->x + box2->w - 1;
    right = L_MAX(right1, right2);
    bot1 = box1->y + box1->h - 1;
    bot2 = box2->y + box2->h - 1;
    bot = L_MAX(bot1, bot2);
    return boxCreate(left, top, right - left + 1, bot - top + 1);
}


/*!
 *  boxOverlapFraction()
 *
 *      Input:  box1, box2 (two boxes)
 *              &fract (<return> the fraction of box2 overlapped by box1)
 *      Return: 0 if OK, 1 on error.
 *
 *  Notes:
 *      (1) The result depends on the order of the input boxes,
 *          because the overlap is taken as a fraction of box2.
 */
l_int32
boxOverlapFraction(BOX        *box1,
                   BOX        *box2,
                   l_float32  *pfract)
{
l_int32  w2, h2, w, h;
BOX     *boxo;

    PROCNAME("boxOverlapFraction");

    if (!pfract)
        return ERROR_INT("&fract not defined", procName, 1);
    *pfract = 0.0;
    if (!box1)
        return ERROR_INT("box1 not defined", procName, 1);
    if (!box2)
        return ERROR_INT("box2 not defined", procName, 1);

    if ((boxo = boxOverlapRegion(box1, box2)) == NULL)  /* no overlap */
        return 0;

    boxGetGeometry(box2, NULL, NULL, &w2, &h2);
    boxGetGeometry(boxo, NULL, NULL, &w, &h);
    *pfract = (l_float32)(w * h) / (l_float32)(w2 * h2);
    boxDestroy(&boxo);
    return 0;
}


/*!
 *  boxOverlapArea()
 *
 *      Input:  box1, box2 (two boxes)
 *              &area (<return> the number of pixels in the overlap)
 *      Return: 0 if OK, 1 on error.
 */
l_int32
boxOverlapArea(BOX      *box1,
               BOX      *box2,
               l_int32  *parea)
{
l_int32  w, h;
BOX     *box;

    PROCNAME("boxOverlapArea");

    if (!parea)
        return ERROR_INT("&area not defined", procName, 1);
    *parea = 0;
    if (!box1)
        return ERROR_INT("box1 not defined", procName, 1);
    if (!box2)
        return ERROR_INT("box2 not defined", procName, 1);

    if ((box = boxOverlapRegion(box1, box2)) == NULL)  /* no overlap */
        return 0;

    boxGetGeometry(box, NULL, NULL, &w, &h);
    *parea = w * h;
    boxDestroy(&box);
    return 0;
}


/*!
 *  boxaHandleOverlaps()
 *
 *      Input:  boxas
 *              op (L_COMBINE, L_REMOVE_SMALL)
 *              range (> 0, forward distance over which overlaps are checked)
 *              min_overlap (minimum fraction of smaller box required for
 *                           overlap to count; 0.0 to ignore)
 *              max_ratio (maximum fraction of small/large areas for
 *                         overlap to count; 1.0 to ignore)
 *              &namap (<optional return> combining map)
 *      Return: boxad, or null on error.
 *
 *  Notes:
 *      (1) For all n(n-1)/2 box pairings, if two boxes overlap, either:
 *          (a) op == L_COMBINE: get the bounding region for the two,
 *              replace the larger with the bounding region, and remove
 *              the smaller of the two, or
 *          (b) op == L_REMOVE_SMALL: just remove the smaller.
 *      (2) If boxas is 2D sorted, range can be small, but if it is
 *          not spatially sorted, range should be large to allow all
 *          pairwise comparisons to be made.
 *      (3) The @min_overlap parameter allows ignoring small overlaps.
 *          If @min_overlap == 1.0, only boxes fully contained in larger
 *          boxes can be considered for removal; if @min_overlap == 0.0,
 *          this constraint is ignored.
 *      (4) The @max_ratio parameter allows ignoring overlaps between
 *          boxes that are not too different in size.  If @max_ratio == 0.0,
 *          no boxes can be removed; if @max_ratio == 1.0, this constraint
 *          is ignored.
 */
BOXA *
boxaHandleOverlaps(BOXA    *boxas,
                   l_int32  op,
                   l_int32  range,
                   l_float32  min_overlap,
                   l_float32  max_ratio,
                   NUMA   **pnamap)
{
l_int32    i, j, n, w, h, area1, area2, val;
l_int32    overlap_area;
l_float32  overlap_ratio, area_ratio;
BOX       *box1, *box2, *box3;
BOXA      *boxat, *boxad;
NUMA      *namap;

    PROCNAME("boxaHandleOverlaps");

    if (pnamap) *pnamap = NULL;
    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (op != L_COMBINE && op != L_REMOVE_SMALL)
        return (BOXA *)ERROR_PTR("invalid op", procName, NULL);

    n = boxaGetCount(boxas);
    if (n == 0)
        return boxaCreate(1);  /* empty */
    if (range == 0) {
        L_WARNING("range is 0\n", procName);
        return boxaCopy(boxas, L_COPY);
    }

        /* Identify smaller boxes in overlap pairs, and mark to eliminate. */
    namap = numaMakeConstant(-1, n);
    for (i = 0; i < n; i++) {
        box1 = boxaGetBox(boxas, i, L_CLONE);
        boxGetGeometry(box1, NULL, NULL, &w, &h);
        area1 = w * h;
        if (area1 == 0) {
            boxDestroy(&box1);
            continue;
        }
        for (j = i + 1; j < i + 1 + range && j < n; j++) {
            box2 = boxaGetBox(boxas, j, L_CLONE);
            boxOverlapArea(box1, box2, &overlap_area);
            if (overlap_area > 0) {
                boxGetGeometry(box2, NULL, NULL, &w, &h);
                area2 = w * h;
                if (area2 == 0) {
                    /* do nothing */
                } else if (area1 >= area2) {
                    overlap_ratio = (l_float32)overlap_area / area2;
                    area_ratio = (l_float32)area2 / (l_float32)area1;
                    if (overlap_ratio >= min_overlap &&
                        area_ratio <= max_ratio) {
                        numaSetValue(namap, j, i);
                    }
                } else {
                    overlap_ratio = overlap_area / area1;
                    area_ratio = (l_float32)area1 / (l_float32)area2;
                    if (overlap_ratio >= min_overlap &&
                        area_ratio <= max_ratio) {
                        numaSetValue(namap, i, j);
                    }
                }
            }
            boxDestroy(&box2);
        }
        boxDestroy(&box1);
    }

    boxat = boxaCopy(boxas, L_COPY);
    if (op == L_COMBINE) {
            /* Resize the larger of the pair to the bounding region */
        for (i = 0; i < n; i++) {
            numaGetIValue(namap, i, &val);
            if (val >= 0) {
                box1 = boxaGetBox(boxas, i, L_CLONE);  /* smaller */
                box2 = boxaGetBox(boxas, val, L_CLONE);  /* larger */
                box3 = boxBoundingRegion(box1, box2);
                boxaReplaceBox(boxat, val, box3);
                boxDestroy(&box1);
                boxDestroy(&box2);
            }
        }
    }

        /* Remove the smaller of the pairs */
    boxad = boxaCreate(n);
    for (i = 0; i < n; i++) {
        numaGetIValue(namap, i, &val);
        if (val == -1) {
            box1 = boxaGetBox(boxat, i, L_COPY);
            boxaAddBox(boxad, box1, L_INSERT);
        }
    }
    boxaDestroy(&boxat);
    if (pnamap)
        *pnamap = namap;
    else
        numaDestroy(&namap);
    return boxad;
}


/*!
 *  boxSeparationDistance()
 *
 *      Input:  box1, box2 (two boxes, in any order)
 *              &h_sep (<optional return> horizontal separation)
 *              &v_sep (<optional return> vertical separation)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This measures horizontal and vertical separation of the
 *          two boxes.  If the boxes are touching but have no pixels
 *          in common, the separation is 0.  If the boxes overlap by
 *          a distance d, the returned separation is -d.
 */
l_int32
boxSeparationDistance(BOX      *box1,
                      BOX      *box2,
                      l_int32  *ph_sep,
                      l_int32  *pv_sep)
{
l_int32  left1, left2, top1, top2, right1, right2, bot1, bot2;

    PROCNAME("boxSeparationDistance");

    if (!ph_sep && !pv_sep)
        return ERROR_INT("nothing to do", procName, 1);
    if (ph_sep) *ph_sep = 0;
    if (pv_sep) *pv_sep = 0;
    if (!box1 || !box2)
        return ERROR_INT("box1 and box2 not both defined", procName, 1);

    if (ph_sep) {
        left1 = box1->x;
        left2 = box2->x;
        right1 = box1->x + box1->w;  /* 1 pixel to right of the box */
        right2 = box2->x + box2->w;
        if (left2 >= left1)
            *ph_sep = left2 - right1;
        else
            *ph_sep = left1 - right2;
    }
    if (pv_sep) {
        top1 = box1->y;
        top2 = box2->y;
        bot1 = box1->y + box1->h;  /* 1 pixel below the box */
        bot2 = box2->y + box2->h;
        if (top2 >= top1)
            *pv_sep = top2 - bot1;
        else
            *pv_sep = top1 - bot2;
    }
    return 0;
}


/*!
 *  boxContainsPt()
 *
 *      Input:  box
 *              x, y (a point)
 *              &contains (<return> 1 if box contains point; 0 otherwise)
 *      Return: 0 if OK, 1 on error.
 */
l_int32
boxContainsPt(BOX       *box,
              l_float32  x,
              l_float32  y,
              l_int32   *pcontains)
{
l_int32  bx, by, bw, bh;

    PROCNAME("boxContainsPt");

    if (!pcontains)
        return ERROR_INT("&contains not defined", procName, 1);
    *pcontains = 0;
    if (!box)
        return ERROR_INT("&box not defined", procName, 1);
    boxGetGeometry(box, &bx, &by, &bw, &bh);
    if (x >= bx && x < bx + bw && y >= by && y < by + bh)
        *pcontains = 1;
    return 0;
}


/*!
 *  boxaGetNearestToPt()
 *
 *      Input:  boxa
 *              x, y  (point)
 *      Return  box (box with centroid closest to the given point [x,y]),
 *              or NULL if no boxes in boxa)
 *
 *  Notes:
 *      (1) Uses euclidean distance between centroid and point.
 */
BOX *
boxaGetNearestToPt(BOXA    *boxa,
                   l_int32  x,
                   l_int32  y)
{
l_int32    i, n, minindex;
l_float32  delx, dely, dist, mindist, cx, cy;
BOX       *box;

    PROCNAME("boxaGetNearestToPt");

    if (!boxa)
        return (BOX *)ERROR_PTR("boxa not defined", procName, NULL);
    if ((n = boxaGetCount(boxa)) == 0)
        return (BOX *)ERROR_PTR("n = 0", procName, NULL);

    mindist = 1000000000.;
    minindex = 0;
    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        boxGetCenter(box, &cx, &cy);
        delx = (l_float32)(cx - x);
        dely = (l_float32)(cy - y);
        dist = delx * delx + dely * dely;
        if (dist < mindist) {
            minindex = i;
            mindist = dist;
        }
        boxDestroy(&box);
    }

    return boxaGetBox(boxa, minindex, L_COPY);
}


/*!
 *  boxGetCenter()
 *
 *      Input:  box
 *              &cx, &cy (<return> location of center of box)
 *      Return  0 if OK, 1 on error
 */
l_int32
boxGetCenter(BOX        *box,
             l_float32  *pcx,
             l_float32  *pcy)
{
l_int32  x, y, w, h;

    PROCNAME("boxGetCenter");

    if (!pcx || !pcy)
        return ERROR_INT("&cx, &cy not both defined", procName, 1);
    *pcx = *pcy = 0;
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    boxGetGeometry(box, &x, &y, &w, &h);
    *pcx = (l_float32)(x + 0.5 * w);
    *pcy = (l_float32)(y + 0.5 * h);

    return 0;
}


/*!
 *  boxIntersectByLine()
 *
 *      Input:  box
 *              x, y (point that line goes through)
 *              slope (of line)
 *              (&x1, &y1) (<return> 1st point of intersection with box)
 *              (&x2, &y2) (<return> 2nd point of intersection with box)
 *              &n (<return> number of points of intersection)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If the intersection is at only one point (a corner), the
 *          coordinates are returned in (x1, y1).
 *      (2) Represent a vertical line by one with a large but finite slope.
 */
l_int32
boxIntersectByLine(BOX       *box,
                   l_int32    x,
                   l_int32    y,
                   l_float32  slope,
                   l_int32   *px1,
                   l_int32   *py1,
                   l_int32   *px2,
                   l_int32   *py2,
                   l_int32   *pn)
{
l_int32    bx, by, bw, bh, xp, yp, xt, yt, i, n;
l_float32  invslope;
PTA       *pta;

    PROCNAME("boxIntersectByLine");

    if (!px1 || !py1 || !px2 || !py2)
        return ERROR_INT("&x1, &y1, &x2, &y2 not all defined", procName, 1);
    *px1 = *py1 = *px2 = *py2 = 0;
    if (!pn)
        return ERROR_INT("&n not defined", procName, 1);
    *pn = 0;
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    boxGetGeometry(box, &bx, &by, &bw, &bh);

    if (slope == 0.0) {
        if (y >= by && y < by + bh) {
            *py1 = *py2 = y;
            *px1 = bx;
            *px2 = bx + bw - 1;
        }
        return 0;
    }

    if (slope > 1000000.0) {
        if (x >= bx && x < bx + bw) {
            *px1 = *px2 = x;
            *py1 = by;
            *py2 = by + bh - 1;
        }
        return 0;
    }

        /* Intersection with top and bottom lines of box */
    pta = ptaCreate(2);
    invslope = 1.0 / slope;
    xp = (l_int32)(x + invslope * (y - by));
    if (xp >= bx && xp < bx + bw)
        ptaAddPt(pta, xp, by);
    xp = (l_int32)(x + invslope * (y - by - bh + 1));
    if (xp >= bx && xp < bx + bw)
        ptaAddPt(pta, xp, by + bh - 1);

        /* Intersection with left and right lines of box */
    yp = (l_int32)(y + slope * (x - bx));
    if (yp >= by && yp < by + bh)
        ptaAddPt(pta, bx, yp);
    yp = (l_int32)(y + slope * (x - bx - bw + 1));
    if (yp >= by && yp < by + bh)
        ptaAddPt(pta, bx + bw - 1, yp);

        /* There is a maximum of 2 unique points; remove duplicates.  */
    n = ptaGetCount(pta);
    if (n > 0) {
        ptaGetIPt(pta, 0, px1, py1);  /* accept the first one */
	*pn = 1;
    }
    for (i = 1; i < n; i++) {
        ptaGetIPt(pta, i, &xt, &yt);
        if ((*px1 != xt) || (*py1 != yt)) {
            *px2 = xt;
            *py2 = yt;
            *pn = 2;
            break;
        }
    }

    ptaDestroy(&pta);
    return 0;
}


/*!
 *  boxClipToRectangle()
 *
 *      Input:  box
 *              wi, hi (rectangle representing image)
 *      Return: part of box within given rectangle, or NULL on error
 *              or if box is entirely outside the rectangle
 *
 *  Notes:
 *      (1) This can be used to clip a rectangle to an image.
 *          The clipping rectangle is assumed to have a UL corner at (0, 0),
 *          and a LR corner at (wi - 1, hi - 1).
 */
BOX *
boxClipToRectangle(BOX     *box,
                   l_int32  wi,
                   l_int32  hi)
{
BOX  *boxd;

    PROCNAME("boxClipToRectangle");

    if (!box)
        return (BOX *)ERROR_PTR("box not defined", procName, NULL);
    if (box->x >= wi || box->y >= hi ||
        box->x + box->w <= 0 || box->y + box->h <= 0)
        return (BOX *)ERROR_PTR("box outside rectangle", procName, NULL);

    boxd = boxCopy(box);
    if (boxd->x < 0) {
        boxd->w += boxd->x;
        boxd->x = 0;
    }
    if (boxd->y < 0) {
        boxd->h += boxd->y;
        boxd->y = 0;
    }
    if (boxd->x + boxd->w > wi)
        boxd->w = wi - boxd->x;
    if (boxd->y + boxd->h > hi)
        boxd->h = hi - boxd->y;
    return boxd;
}


/*!
 *  boxClipToRectangleParams()
 *
 *      Input:  box (<optional> requested box; can be null)
 *              w, h (clipping box size; typ. the size of an image)
 *              &xstart (<return>)
 *              &ystart (<return>)
 *              &xend (<return> one pixel beyond clipping box)
 *              &yend (<return> one pixel beyond clipping box)
 *              &bw (<optional return> clipped width)
 *              &bh (<optional return> clipped height)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The return value should be checked.  If it is 1, the
 *          returned parameter values are bogus.
 *      (2) This simplifies the selection of pixel locations within
 *          a given rectangle:
 *             for (i = ystart; i < yend; i++ {
 *                 ...
 *                 for (j = xstart; j < xend; j++ {
 *                     ....
 */
l_int32
boxClipToRectangleParams(BOX      *box,
                         l_int32   w,
                         l_int32   h,
                         l_int32  *pxstart,
                         l_int32  *pystart,
                         l_int32  *pxend,
                         l_int32  *pyend,
                         l_int32  *pbw,
                         l_int32  *pbh)
{
l_int32  bw, bh;
BOX     *boxc;

    PROCNAME("boxClipToRectangleParams");

    if (!pxstart || !pystart || !pxend || !pyend)
        return ERROR_INT("invalid ptr input", procName, 1);
    *pxstart = *pystart = 0;
    *pxend = w;
    *pyend = h;
    if (pbw) *pbw = w;
    if (pbh) *pbh = h;
    if (!box) return 0;

    if ((boxc = boxClipToRectangle(box, w, h)) == NULL)
        return ERROR_INT("box outside image", procName, 1);
    boxGetGeometry(boxc, pxstart, pystart, &bw, &bh);
    boxDestroy(&boxc);

    if (pbw) *pbw = bw;
    if (pbh) *pbh = bh;
    if (bw == 0 || bh == 0)
        return ERROR_INT("invalid clipping box", procName, 1);
    *pxend = *pxstart + bw;  /* 1 past the end */
    *pyend = *pystart + bh;  /* 1 past the end */
    return 0;
}


/*!
 *  boxRelocateOneSide()
 *
 *      Input:  boxd (<optional>; this can be null, equal to boxs,
 *                    or different from boxs);
 *              boxs (starting box; to have one side relocated)
 *              loc (new location of the side that is changing)
 *              sideflag (L_FROM_LEFT, etc., indicating the side that moves)
 *      Return: boxd, or null on error or if the computed boxd has
 *              width or height <= 0.
 *
 *  Notes:
 *      (1) Set boxd == NULL to get new box; boxd == boxs for in-place;
 *          or otherwise to resize existing boxd.
 *      (2) For usage, suggest one of these:
 *               boxd = boxRelocateOneSide(NULL, boxs, ...);   // new
 *               boxRelocateOneSide(boxs, boxs, ...);          // in-place
 *               boxRelocateOneSide(boxd, boxs, ...);          // other
 */
BOX *
boxRelocateOneSide(BOX     *boxd,
                   BOX     *boxs,
                   l_int32  loc,
                   l_int32  sideflag)
{
l_int32  x, y, w, h;

    PROCNAME("boxRelocateOneSide");

    if (!boxs)
        return (BOX *)ERROR_PTR("boxs not defined", procName, NULL);
    if (!boxd)
        boxd = boxCopy(boxs);

    boxGetGeometry(boxs, &x, &y, &w, &h);
    if (sideflag == L_FROM_LEFT)
        boxSetGeometry(boxd, loc, -1, w + x - loc, -1);
    else if (sideflag == L_FROM_RIGHT)
        boxSetGeometry(boxd, -1, -1, loc - x + 1, -1);
    else if (sideflag == L_FROM_TOP)
        boxSetGeometry(boxd, -1, loc, -1, h + y - loc);
    else if (sideflag == L_FROM_BOT)
        boxSetGeometry(boxd, -1, -1, -1, loc - y + 1);
    return boxd;
}


/*!
 *  boxAdjustSides()
 *
 *      Input:  boxd  (<optional>; this can be null, equal to boxs,
 *                     or different from boxs)
 *              boxs  (starting box; to have sides adjusted)
 *              delleft, delright, deltop, delbot (changes in location of
 *                                                 each side)
 *      Return: boxd, or null on error or if the computed boxd has
 *              width or height <= 0.
 *
 *  Notes:
 *      (1) Set boxd == NULL to get new box; boxd == boxs for in-place;
 *          or otherwise to resize existing boxd.
 *      (2) For usage, suggest one of these:
 *               boxd = boxAdjustSides(NULL, boxs, ...);   // new
 *               boxAdjustSides(boxs, boxs, ...);          // in-place
 *               boxAdjustSides(boxd, boxs, ...);          // other
 *      (1) New box dimensions are cropped at left and top to x >= 0 and y >= 0.
 *      (2) For example, to expand in-place by 20 pixels on each side, use
 *             boxAdjustSides(box, box, -20, 20, -20, 20);
 */
BOX *
boxAdjustSides(BOX     *boxd,
               BOX     *boxs,
               l_int32  delleft,
               l_int32  delright,
               l_int32  deltop,
               l_int32  delbot)
{
l_int32  x, y, w, h, xl, xr, yt, yb, wnew, hnew;

    PROCNAME("boxAdjustSides");

    if (!boxs)
        return (BOX *)ERROR_PTR("boxs not defined", procName, NULL);

    boxGetGeometry(boxs, &x, &y, &w, &h);
    xl = L_MAX(0, x + delleft);
    yt = L_MAX(0, y + deltop);
    xr = x + w + delright;  /* one pixel beyond right edge */
    yb = y + h + delbot;    /* one pixel below bottom edge */
    wnew = xr - xl;
    hnew = yb - yt;

    if (wnew < 1 || hnew < 1)
        return (BOX *)ERROR_PTR("boxd has 0 area", procName, NULL);
    if (!boxd)
        return boxCreate(xl, yt, wnew, hnew);

    boxSetGeometry(boxd, xl, yt, wnew, hnew);
    return boxd;
}


/*!
 *  boxaSetSide()
 *
 *      Input:  boxad (use null to get a new one; same as boxas for in-place)
 *              boxas
 *              side (L_SET_LEFT, L_SET_RIGHT, L_SET_TOP, L_SET_BOT)
 *              val (location to set for given side, for each box)
 *              thresh (min abs difference to cause resetting to @val)
 *      Return: boxad, or null on error
 *
 *  Notes:
 *      (1) Sets the given side of each box.  Use boxad == NULL for a new
 *          boxa, and boxad == boxas for in-place.
 *      (2) Use one of these:
 *               boxad = boxaSetSide(NULL, boxas, ...);   // new
 *               boxaSetSide(boxas, boxas, ...);  // in-place
 */
BOXA *
boxaSetSide(BOXA    *boxad,
            BOXA    *boxas,
            l_int32  side,
            l_int32  val,
            l_int32  thresh)
{
l_int32  x, y, w, h, n, i, diff;
BOX     *box;

    PROCNAME("boxaSetSide");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (boxad && (boxas != boxad))
        return (BOXA *)ERROR_PTR("not in-place", procName, NULL);
    if (side != L_SET_LEFT && side != L_SET_RIGHT &&
        side != L_SET_TOP && side != L_SET_BOT)
        return (BOXA *)ERROR_PTR("invalid side", procName, NULL);
    if (val < 0)
        return (BOXA *)ERROR_PTR("val < 0", procName, NULL);

    if (!boxad)
        boxad = boxaCopy(boxas, L_COPY);
    n = boxaGetCount(boxad);
    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxad, i, L_CLONE);
        boxGetGeometry(box, &x, &y, &w, &h);
        if (side == L_SET_LEFT) {
            diff = x - val;
            if (L_ABS(diff) >= thresh)
                boxSetGeometry(box, val, y, w + diff, h);
        } else if (side == L_SET_RIGHT) {
            diff = x + w -1 - val;
            if (L_ABS(diff) >= thresh)
                boxSetGeometry(box, x, y, val - x + 1, h);
        } else if (side == L_SET_TOP) {
            diff = y - val;
            if (L_ABS(diff) >= thresh)
                boxSetGeometry(box, x, val, w, h + diff);
        } else { /* side == L_SET_BOT */
            diff = y + h - 1 - val;
            if (L_ABS(diff) >= thresh)
                boxSetGeometry(box, x, y, w, val - y + 1);
        }
        boxDestroy(&box);
    }

    return boxad;
}


/*!
 *  boxaAdjustWidthToTarget()
 *
 *      Input:  boxad (use null to get a new one; same as boxas for in-place)
 *              boxas
 *              sides (L_ADJUST_LEFT, L_ADJUST_RIGHT, L_ADJUST_LEFTL_AND_RIGHT)
 *              target (target width if differs by more than thresh)
 *              thresh (min abs difference in width to cause adjustment)
 *      Return: boxad, or null on error
 *
 *  Notes:
 *      (1) Conditionally adjusts the width of each box, by moving
 *          the indicated edges (left and/or right) if the width differs
 *          by @thresh or more from @target.
 *      (2) Use boxad == NULL for a new boxa, and boxad == boxas for in-place.
 *          Use one of these:
 *               boxad = boxaAdjustWidthToTarget(NULL, boxas, ...);   // new
 *               boxaAdjustWidthToTarget(boxas, boxas, ...);  // in-place
 */
BOXA *
boxaAdjustWidthToTarget(BOXA    *boxad,
                        BOXA    *boxas,
                        l_int32  sides,
                        l_int32  target,
                        l_int32  thresh)
{
l_int32  x, y, w, h, n, i, diff;
BOX     *box;

    PROCNAME("boxaAdjustWidthToTarget");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (boxad && (boxas != boxad))
        return (BOXA *)ERROR_PTR("not in-place", procName, NULL);
    if (sides != L_ADJUST_LEFT && sides != L_ADJUST_RIGHT &&
        sides != L_ADJUST_LEFT_AND_RIGHT)
        return (BOXA *)ERROR_PTR("invalid sides", procName, NULL);
    if (target < 1)
        return (BOXA *)ERROR_PTR("target < 1", procName, NULL);

    if (!boxad)
        boxad = boxaCopy(boxas, L_COPY);
    n = boxaGetCount(boxad);
    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxad, i, L_CLONE);
        boxGetGeometry(box, &x, &y, &w, &h);
        diff = w - target;
        if (sides == L_ADJUST_LEFT) {
            if (L_ABS(diff) >= thresh)
                boxSetGeometry(box, L_MAX(0, x + diff), y, target, h);
        } else if (sides == L_ADJUST_RIGHT) {
            if (L_ABS(diff) >= thresh)
                boxSetGeometry(box, x, y, target, h);
        } else { /* sides == L_ADJUST_LEFT_AND_RIGHT */
            if (L_ABS(diff) >= thresh)
                boxSetGeometry(box, L_MAX(0, x + diff/2), y, target, h);
        }
        boxDestroy(&box);
    }

    return boxad;
}


/*!
 *  boxaAdjustHeightToTarget()
 *
 *      Input:  boxad (use null to get a new one)
 *              boxas
 *              sides (L_ADJUST_TOP, L_ADJUST_BOT, L_ADJUST_TOP_AND_BOT)
 *              target (target height if differs by more than thresh)
 *              thresh (min abs difference in height to cause adjustment)
 *      Return: boxad, or null on error
 *
 *  Notes:
 *      (1) Conditionally adjusts the height of each box, by moving
 *          the indicated edges (top and/or bot) if the height differs
 *          by @thresh or more from @target.
 *      (2) Use boxad == NULL for a new boxa, and boxad == boxas for in-place.
 *          Use one of these:
 *               boxad = boxaAdjustHeightToTarget(NULL, boxas, ...);   // new
 *               boxaAdjustHeightToTarget(boxas, boxas, ...);  // in-place
 */
BOXA *
boxaAdjustHeightToTarget(BOXA    *boxad,
                         BOXA    *boxas,
                        l_int32  sides,
                        l_int32  target,
                        l_int32  thresh)
{
l_int32  x, y, w, h, n, i, diff;
BOX     *box;

    PROCNAME("boxaAdjustHeightToTarget");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (boxad && (boxas != boxad))
        return (BOXA *)ERROR_PTR("not in-place", procName, NULL);
    if (sides != L_ADJUST_TOP && sides != L_ADJUST_BOT &&
        sides != L_ADJUST_TOP_AND_BOT)
        return (BOXA *)ERROR_PTR("invalid sides", procName, NULL);
    if (target < 1)
        return (BOXA *)ERROR_PTR("target < 1", procName, NULL);

    if (!boxad)
        boxad = boxaCopy(boxas, L_COPY);
    n = boxaGetCount(boxad);
    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxad, i, L_CLONE);
        boxGetGeometry(box, &x, &y, &w, &h);
        diff = h - target;
        if (sides == L_ADJUST_TOP) {
            if (L_ABS(diff) >= thresh)
                boxSetGeometry(box, x, L_MAX(0, y + diff), w, target);
        } else if (sides == L_ADJUST_BOT) {
            if (L_ABS(diff) >= thresh)
                boxSetGeometry(box, x, y, w, target);
        } else { /* sides == L_ADJUST_TOP_AND_BOT */
            if (L_ABS(diff) >= thresh)
                boxSetGeometry(box, x, L_MAX(0, y + diff/2), w, target);
        }
        boxDestroy(&box);
    }

    return boxad;
}


/*!
 *  boxEqual()
 *
 *      Input:  box1
 *              box2
 *              &same (<return> 1 if equal; 0 otherwise)
 *      Return  0 if OK, 1 on error
 */
l_int32
boxEqual(BOX      *box1,
         BOX      *box2,
         l_int32  *psame)
{
    PROCNAME("boxEqual");

    if (!psame)
        return ERROR_INT("&same not defined", procName, 1);
    *psame = 0;
    if (!box1 || !box2)
        return ERROR_INT("box1 and box2 not both defined", procName, 1);
    if (box1->x == box2->x && box1->y == box2->y &&
        box1->w == box2->w && box1->h == box2->h)
        *psame = 1;
    return 0;
}


/*!
 *  boxaEqual()
 *
 *      Input:  boxa1
 *              boxa2
 *              maxdist
 *              &naindex (<optional return> index array of correspondences
 *              &same (<return> 1 if equal; 0 otherwise)
 *      Return  0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The two boxa are the "same" if they contain the same
 *          boxes and each box is within @maxdist of its counterpart
 *          in their positions within the boxa.  This allows for
 *          small rearrangements.  Use 0 for maxdist if the boxa
 *          must be identical.
 *      (2) This applies only to geometry and ordering; refcounts
 *          are not considered.
 *      (3) @maxdist allows some latitude in the ordering of the boxes.
 *          For the boxa to be the "same", corresponding boxes must
 *          be within @maxdist of each other.  Note that for large
 *          @maxdist, we should use a hash function for efficiency.
 *      (4) naindex[i] gives the position of the box in boxa2 that
 *          corresponds to box i in boxa1.  It is only returned if the
 *          boxa are equal.
 */
l_int32
boxaEqual(BOXA     *boxa1,
          BOXA     *boxa2,
          l_int32   maxdist,
          NUMA    **pnaindex,
          l_int32  *psame)
{
l_int32   i, j, n, jstart, jend, found, samebox;
l_int32  *countarray;
BOX      *box1, *box2;
NUMA     *na;

    PROCNAME("boxaEqual");

    if (pnaindex) *pnaindex = NULL;
    if (!psame)
        return ERROR_INT("&same not defined", procName, 1);
    *psame = 0;
    if (!boxa1 || !boxa2)
        return ERROR_INT("boxa1 and boxa2 not both defined", procName, 1);
    n = boxaGetCount(boxa1);
    if (n != boxaGetCount(boxa2))
        return 0;

    countarray = (l_int32 *)CALLOC(n, sizeof(l_int32));
    na = numaMakeConstant(0.0, n);

    for (i = 0; i < n; i++) {
        box1 = boxaGetBox(boxa1, i, L_CLONE);
        jstart = L_MAX(0, i - maxdist);
        jend = L_MIN(n-1, i + maxdist);
        found = FALSE;
        for (j = jstart; j <= jend; j++) {
            box2 = boxaGetBox(boxa2, j, L_CLONE);
            boxEqual(box1, box2, &samebox);
            if (samebox && countarray[j] == 0) {
                countarray[j] = 1;
                numaReplaceNumber(na, i, j);
                found = TRUE;
                boxDestroy(&box2);
                break;
            }
            boxDestroy(&box2);
        }
        boxDestroy(&box1);
        if (!found) {
            numaDestroy(&na);
            FREE(countarray);
            return 0;
        }
    }

    *psame = 1;
    if (pnaindex)
        *pnaindex = na;
    else
        numaDestroy(&na);
    FREE(countarray);
    return 0;
}


/*!
 *  boxSimilar()
 *
 *      Input:  box1
 *              box2
 *              leftdiff, rightdiff, topdiff, botdiff
 *              &similar (<return> 1 if similar; 0 otherwise)
 *      Return  0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The values of leftdiff (etc) are the maximum allowed deviations
 *          between the locations of the left (etc) sides.  If any side
 *          pairs differ by more than this amount, the boxes are not similar.
 */
l_int32
boxSimilar(BOX      *box1,
           BOX      *box2,
           l_int32   leftdiff,
           l_int32   rightdiff,
           l_int32   topdiff,
           l_int32   botdiff,
           l_int32  *psimilar)
{
l_int32  loc1, loc2;

    PROCNAME("boxSimilar");

    if (!psimilar)
        return ERROR_INT("&similar not defined", procName, 1);
    *psimilar = 0;
    if (!box1 || !box2)
        return ERROR_INT("box1 and box2 not both defined", procName, 1);

    boxGetSideLocation(box1, L_GET_LEFT, &loc1);
    boxGetSideLocation(box2, L_GET_LEFT, &loc2);
    if (L_ABS(loc1 - loc2) > leftdiff)
        return 0;
    boxGetSideLocation(box1, L_GET_RIGHT, &loc1);
    boxGetSideLocation(box2, L_GET_RIGHT, &loc2);
    if (L_ABS(loc1 - loc2) > rightdiff)
        return 0;
    boxGetSideLocation(box1, L_GET_TOP, &loc1);
    boxGetSideLocation(box2, L_GET_TOP, &loc2);
    if (L_ABS(loc1 - loc2) > topdiff)
        return 0;
    boxGetSideLocation(box1, L_GET_BOT, &loc1);
    boxGetSideLocation(box2, L_GET_BOT, &loc2);
    if (L_ABS(loc1 - loc2) > botdiff)
        return 0;

    *psimilar = 1;
    return 0;
}


/*!
 *  boxaSimilar()
 *
 *      Input:  boxa1
 *              boxa2
 *              leftdiff, rightdiff, topdiff, botdiff
 *              debugflag (output details of non-similar boxes)
 *              &similar (<return> 1 if similar; 0 otherwise)
 *      Return  0 if OK, 1 on error
 *
 *  Notes:
 *      (1) See boxSimilar() for parameter usage.
 *      (2) Corresponding boxes are taken in order in the two boxa.
 *      (3) With debugflag == 1, boxes continue to be tested after failure.
 */
l_int32
boxaSimilar(BOXA     *boxa1,
            BOXA     *boxa2,
            l_int32   leftdiff,
            l_int32   rightdiff,
            l_int32   topdiff,
            l_int32   botdiff,
            l_int32   debugflag,
            l_int32  *psimilar)
{
l_int32  i, n, match, mismatch;
BOX     *box1, *box2;

    PROCNAME("boxaSimilar");

    if (!psimilar)
        return ERROR_INT("&similar not defined", procName, 1);
    *psimilar = 0;
    if (!boxa1 || !boxa2)
        return ERROR_INT("boxa1 and boxa2 not both defined", procName, 1);
    n = boxaGetCount(boxa1);
    if (n != boxaGetCount(boxa2)) return 0;

    mismatch = FALSE;
    for (i = 0; i < n; i++) {
        box1 = boxaGetBox(boxa1, i, L_CLONE);
        box2 = boxaGetBox(boxa2, i, L_CLONE);
        boxSimilar(box1, box2, leftdiff, rightdiff, topdiff, botdiff,
                   &match);
        boxDestroy(&box1);
        boxDestroy(&box2);
        if (!match) {
            mismatch = TRUE;
            if (debugflag)
                fprintf(stderr, "box %d not similar\n", i);
            else
                return 0;
        }
    }

    if (!mismatch) *psimilar = 1;
    return 0;
}


/*----------------------------------------------------------------------*
 *                      Boxa combine and split                          *
 *----------------------------------------------------------------------*/
/*!
 *  boxaJoin()
 *
 *      Input:  boxad  (dest boxa; add to this one)
 *              boxas  (source boxa; add from this one)
 *              istart  (starting index in boxas)
 *              iend  (ending index in boxas; use -1 to cat all)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This appends a clone of each indicated box in boxas to boxad
 *      (2) istart < 0 is taken to mean 'read from the start' (istart = 0)
 *      (3) iend < 0 means 'read to the end'
 *      (4) if boxas == NULL or has no boxes, this is a no-op.
 */
l_int32
boxaJoin(BOXA    *boxad,
         BOXA    *boxas,
         l_int32  istart,
         l_int32  iend)
{
l_int32  n, i;
BOX     *box;

    PROCNAME("boxaJoin");

    if (!boxad)
        return ERROR_INT("boxad not defined", procName, 1);
    if (!boxas || ((n = boxaGetCount(boxas)) == 0))
        return 0;

    if (istart < 0)
        istart = 0;
    if (iend < 0 || iend >= n)
        iend = n - 1;
    if (istart > iend)
        return ERROR_INT("istart > iend; nothing to add", procName, 1);

    for (i = istart; i <= iend; i++) {
        box = boxaGetBox(boxas, i, L_CLONE);
        boxaAddBox(boxad, box, L_INSERT);
    }

    return 0;
}


/*!
 *  boxaaJoin()
 *
 *      Input:  baad  (dest boxaa; add to this one)
 *              baas  (source boxaa; add from this one)
 *              istart  (starting index in baas)
 *              iend  (ending index in baas; use -1 to cat all)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This appends a clone of each indicated boxa in baas to baad
 *      (2) istart < 0 is taken to mean 'read from the start' (istart = 0)
 *      (3) iend < 0 means 'read to the end'
 *      (4) if baas == NULL, this is a no-op.
 */
l_int32
boxaaJoin(BOXAA   *baad,
          BOXAA   *baas,
          l_int32  istart,
          l_int32  iend)
{
l_int32  n, i;
BOXA    *boxa;

    PROCNAME("boxaaJoin");

    if (!baad)
        return ERROR_INT("baad not defined", procName, 1);
    if (!baas)
        return 0;

    if (istart < 0)
        istart = 0;
    n = boxaaGetCount(baas);
    if (iend < 0 || iend >= n)
        iend = n - 1;
    if (istart > iend)
        return ERROR_INT("istart > iend; nothing to add", procName, 1);

    for (i = istart; i <= iend; i++) {
        boxa = boxaaGetBoxa(baas, i, L_CLONE);
        boxaaAddBoxa(baad, boxa, L_INSERT);
    }

    return 0;
}


/*!
 *  boxaSplitEvenOdd()
 *
 *      Input:  boxa
 *              fillflag (1 to put invalid boxes in place; 0 to omit)
 *              &boxae, &boxao (<return> save even and odd boxes in their
 *                 separate boxa, setting the other type to invalid boxes.)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If @fillflag == 1, boxae has copies of the even boxes
 *          in their original location, and nvalid boxes are placed
 *          in the odd array locations.  And v.v.
 *      (2) If @fillflag == 0, boxae has only copies of the even boxes.
 */
l_int32
boxaSplitEvenOdd(BOXA    *boxa,
                 l_int32  fillflag,
                 BOXA   **pboxae,
                 BOXA   **pboxao)
{
l_int32  i, n;
BOX     *box, *boxt;

    PROCNAME("boxaSplitEvenOdd");

    if (!pboxae || !pboxao)
        return ERROR_INT("&boxae and &boxao not defined", procName, 1);
    *pboxae = *pboxao = NULL;
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    n = boxaGetCount(boxa);
    *pboxae = boxaCreate(n);
    *pboxao = boxaCreate(n);
    if (fillflag == 0) {
            /* don't fill with invalid boxes; end up with half-size boxa */
        for (i = 0; i < n; i++) {
            box = boxaGetBox(boxa, i, L_COPY);
            if ((i & 1) == 0)
                boxaAddBox(*pboxae, box, L_INSERT);
            else
                boxaAddBox(*pboxao, box, L_INSERT);
        }
    } else {
        for (i = 0; i < n; i++) {
            box = boxaGetBox(boxa, i, L_COPY);
            boxt = boxCreate(0, 0, 0, 0);  /* empty placeholder */
            if ((i & 1) == 0) {
                boxaAddBox(*pboxae, box, L_INSERT);
                boxaAddBox(*pboxao, boxt, L_INSERT);
            } else {
                boxaAddBox(*pboxae, boxt, L_INSERT);
                boxaAddBox(*pboxao, box, L_INSERT);
            }
        }
    }
    return 0;
}


/*!
 *  boxaMergeEvenOdd()
 *
 *      Input:  boxae (boxes to go in even positions in merged boxa)
 *              boxao (boxes to go in odd positions in merged boxa)
 *              fillflag (1 if there are invalid boxes in placeholders)
 *      Return: boxad (merged), or null on error
 *
 *  Notes:
 *      (1) This is essentially the inverse of boxaSplitEvenOdd().
 *          Typically, boxae and boxao were generated by boxaSplitEvenOdd(),
 *          and the value of @fillflag needs to be the same in both calls.
 *      (2) If @fillflag == 1, both boxae and boxao are of the same size;
 *          otherwise boxae may have one more box than boxao.
 */
BOXA *
boxaMergeEvenOdd(BOXA    *boxae,
                 BOXA    *boxao,
                 l_int32  fillflag)
{
l_int32  i, n, ne, no;
BOX     *box;
BOXA    *boxad;

    PROCNAME("boxaMergeEvenOdd");

    if (!boxae || !boxao)
        return (BOXA *)ERROR_PTR("boxae and boxao not defined", procName, NULL);
    ne = boxaGetCount(boxae);
    no = boxaGetCount(boxao);
    if (ne < no || ne > no + 1)
        return (BOXA *)ERROR_PTR("boxa sizes invalid", procName, NULL);

    boxad = boxaCreate(ne);
    if (fillflag == 0) {  /* both are approx. half-sized; all valid boxes */
        n = ne + no;
        for (i = 0; i < n; i++) {
            if ((i & 1) == 0)
                box = boxaGetBox(boxae, i / 2, L_COPY);
            else
                box = boxaGetBox(boxao, i / 2, L_COPY);
            boxaAddBox(boxad, box, L_INSERT);
        }
    } else {  /* both are full size and have invalid placeholders */
        for (i = 0; i < ne; i++) {
            if ((i & 1) == 0)
                box = boxaGetBox(boxae, i, L_COPY);
            else
                box = boxaGetBox(boxao, i, L_COPY);
            boxaAddBox(boxad, box, L_INSERT);
        }
    }
    return boxad;
}

