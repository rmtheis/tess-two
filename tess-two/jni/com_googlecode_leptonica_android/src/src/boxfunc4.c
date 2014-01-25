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
 *   boxfunc4.c
 *
 *      Boxa and Boxaa range selection
 *           BOXA     *boxaSelectRange()
 *           BOXAA    *boxaaSelectRange()
 *
 *      Boxa size selection
 *           BOXA     *boxaSelectBySize()
 *           NUMA     *boxaMakeSizeIndicator()
 *           BOXA     *boxaSelectByArea()
 *           NUMA     *boxaMakeAreaIndicator()
 *           BOXA     *boxaSelectWithIndicator()
 *
 *      Boxa permutation
 *           BOXA     *boxaPermutePseudorandom()
 *           BOXA     *boxaPermuteRandom()
 *           l_int32   boxaSwapBoxes()
 *
 *      Boxa conversions
 *           PTA      *boxaConvertToPta()
 *           BOXA     *ptaConvertToBoxa()
 *
 *      Boxa sequence fitting
 *           BOXA     *boxaSmoothSequence()
 *           BOXA     *boxaLinearFit()
 *           BOXA     *boxaConstrainSize()
 *           BOXA     *boxaReconcileEvenOddHeight()
 *           l_int32   boxaPlotSides()    [for debugging]
 *
 *      Miscellaneous boxa functions
 *           l_int32   boxaGetExtent()
 *           l_int32   boxaGetCoverage()
 *           l_int32   boxaaSizeRange()
 *           l_int32   boxaSizeRange()
 *           l_int32   boxaLocationRange()
 *           l_int32   boxaGetArea()
 *           PIX      *boxaDisplayTiled()
 */

#include "allheaders.h"


/*---------------------------------------------------------------------*
 *                     Boxa and boxaa range selection                  *
 *---------------------------------------------------------------------*/
/*!
 *  boxaSelectRange()
 *
 *      Input:  boxas
 *              first (use 0 to select from the beginning)
 *              last (use 0 to select to the end)
 *              copyflag (L_COPY, L_CLONE)
 *      Return: boxad, or null on error
 *
 *  Notes:
 *      (1) The copyflag specifies what we do with each box from boxas.
 *          Specifically, L_CLONE inserts a clone into boxad of each
 *          selected box from boxas.
 */
BOXA *
boxaSelectRange(BOXA    *boxas,
                l_int32  first,
                l_int32  last,
                l_int32  copyflag)
{
l_int32  n, nbox, i;
BOX     *box;
BOXA    *boxad;

    PROCNAME("boxaSelectRange");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (copyflag != L_COPY && copyflag != L_CLONE)
        return (BOXA *)ERROR_PTR("invalid copyflag", procName, NULL);
    n = boxaGetCount(boxas);
    first = L_MAX(0, first);
    if (last <= 0) last = n - 1;
    if (first >= n)
        return (BOXA *)ERROR_PTR("invalid first", procName, NULL);
    if (first > last)
        return (BOXA *)ERROR_PTR("first > last", procName, NULL);

    nbox = last - first + 1;
    boxad = boxaCreate(nbox);
    for (i = first; i <= last; i++) {
        box = boxaGetBox(boxas, i, copyflag);
        boxaAddBox(boxad, box, L_INSERT);
    }
    return boxad;
}


/*!
 *  boxaaSelectRange()
 *
 *      Input:  baas
 *              first (use 0 to select from the beginning)
 *              last (use 0 to select to the end)
 *              copyflag (L_COPY, L_CLONE)
 *      Return: baad, or null on error
 *
 *  Notes:
 *      (1) The copyflag specifies what we do with each boxa from baas.
 *          Specifically, L_CLONE inserts a clone into baad of each
 *          selected boxa from baas.
 */
BOXAA *
boxaaSelectRange(BOXAA   *baas,
                 l_int32  first,
                 l_int32  last,
                 l_int32  copyflag)
{
l_int32  n, nboxa, i;
BOXA    *boxa;
BOXAA   *baad;

    PROCNAME("boxaaSelectRange");

    if (!baas)
        return (BOXAA *)ERROR_PTR("baas not defined", procName, NULL);
    if (copyflag != L_COPY && copyflag != L_CLONE)
        return (BOXAA *)ERROR_PTR("invalid copyflag", procName, NULL);
    n = boxaaGetCount(baas);
    first = L_MAX(0, first);
    if (last <= 0) last = n - 1;
    if (first >= n)
        return (BOXAA *)ERROR_PTR("invalid first", procName, NULL);
    if (first > last)
        return (BOXAA *)ERROR_PTR("first > last", procName, NULL);

    nboxa = last - first + 1;
    baad = boxaaCreate(nboxa);
    for (i = first; i <= last; i++) {
        boxa = boxaaGetBoxa(baas, i, copyflag);
        boxaaAddBoxa(baad, boxa, L_INSERT);
    }
    return baad;
}


/*---------------------------------------------------------------------*
 *                          Boxa size selection                        *
 *---------------------------------------------------------------------*/
/*!
 *  boxaSelectBySize()
 *
 *      Input:  boxas
 *              width, height (threshold dimensions)
 *              type (L_SELECT_WIDTH, L_SELECT_HEIGHT,
 *                    L_SELECT_IF_EITHER, L_SELECT_IF_BOTH)
 *              relation (L_SELECT_IF_LT, L_SELECT_IF_GT,
 *                        L_SELECT_IF_LTE, L_SELECT_IF_GTE)
 *              &changed (<optional return> 1 if changed; 0 if clone returned)
 *      Return: boxad (filtered set), or null on error
 *
 *  Notes:
 *      (1) The args specify constraints on the size of the
 *          components that are kept.
 *      (2) Uses box clones in the new boxa.
 *      (3) If the selection type is L_SELECT_WIDTH, the input
 *          height is ignored, and v.v.
 *      (4) To keep small components, use relation = L_SELECT_IF_LT or
 *          L_SELECT_IF_LTE.
 *          To keep large components, use relation = L_SELECT_IF_GT or
 *          L_SELECT_IF_GTE.
 */
BOXA *
boxaSelectBySize(BOXA     *boxas,
                 l_int32   width,
                 l_int32   height,
                 l_int32   type,
                 l_int32   relation,
                 l_int32  *pchanged)
{
BOXA  *boxad;
NUMA  *na;

    PROCNAME("boxaSelectBySize");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (type != L_SELECT_WIDTH && type != L_SELECT_HEIGHT &&
        type != L_SELECT_IF_EITHER && type != L_SELECT_IF_BOTH)
        return (BOXA *)ERROR_PTR("invalid type", procName, NULL);
    if (relation != L_SELECT_IF_LT && relation != L_SELECT_IF_GT &&
        relation != L_SELECT_IF_LTE && relation != L_SELECT_IF_GTE)
        return (BOXA *)ERROR_PTR("invalid relation", procName, NULL);
    if (pchanged) *pchanged = FALSE;

        /* Compute the indicator array for saving components */
    na = boxaMakeSizeIndicator(boxas, width, height, type, relation);

        /* Filter to get output */
    boxad = boxaSelectWithIndicator(boxas, na, pchanged);

    numaDestroy(&na);
    return boxad;
}


/*!
 *  boxaMakeSizeIndicator()
 *
 *      Input:  boxa
 *              width, height (threshold dimensions)
 *              type (L_SELECT_WIDTH, L_SELECT_HEIGHT,
 *                    L_SELECT_IF_EITHER, L_SELECT_IF_BOTH)
 *              relation (L_SELECT_IF_LT, L_SELECT_IF_GT,
 *                        L_SELECT_IF_LTE, L_SELECT_IF_GTE)
 *      Return: na (indicator array), or null on error
 *
 *  Notes:
 *      (1) The args specify constraints on the size of the
 *          components that are kept.
 *      (2) If the selection type is L_SELECT_WIDTH, the input
 *          height is ignored, and v.v.
 *      (3) To keep small components, use relation = L_SELECT_IF_LT or
 *          L_SELECT_IF_LTE.
 *          To keep large components, use relation = L_SELECT_IF_GT or
 *          L_SELECT_IF_GTE.
 */
NUMA *
boxaMakeSizeIndicator(BOXA     *boxa,
                      l_int32   width,
                      l_int32   height,
                      l_int32   type,
                      l_int32   relation)
{
l_int32  i, n, w, h, ival;
NUMA    *na;

    PROCNAME("boxaMakeSizeIndicator");

    if (!boxa)
        return (NUMA *)ERROR_PTR("boxa not defined", procName, NULL);
    if (type != L_SELECT_WIDTH && type != L_SELECT_HEIGHT &&
        type != L_SELECT_IF_EITHER && type != L_SELECT_IF_BOTH)
        return (NUMA *)ERROR_PTR("invalid type", procName, NULL);
    if (relation != L_SELECT_IF_LT && relation != L_SELECT_IF_GT &&
        relation != L_SELECT_IF_LTE && relation != L_SELECT_IF_GTE)
        return (NUMA *)ERROR_PTR("invalid relation", procName, NULL);

    n = boxaGetCount(boxa);
    na = numaCreate(n);
    for (i = 0; i < n; i++) {
        ival = 0;
        boxaGetBoxGeometry(boxa, i, NULL, NULL, &w, &h);
        switch (type)
        {
        case L_SELECT_WIDTH:
            if ((relation == L_SELECT_IF_LT && w < width) ||
                (relation == L_SELECT_IF_GT && w > width) ||
                (relation == L_SELECT_IF_LTE && w <= width) ||
                (relation == L_SELECT_IF_GTE && w >= width))
                ival = 1;
            break;
        case L_SELECT_HEIGHT:
            if ((relation == L_SELECT_IF_LT && h < height) ||
                (relation == L_SELECT_IF_GT && h > height) ||
                (relation == L_SELECT_IF_LTE && h <= height) ||
                (relation == L_SELECT_IF_GTE && h >= height))
                ival = 1;
            break;
        case L_SELECT_IF_EITHER:
            if (((relation == L_SELECT_IF_LT) && (w < width || h < height)) ||
                ((relation == L_SELECT_IF_GT) && (w > width || h > height)) ||
               ((relation == L_SELECT_IF_LTE) && (w <= width || h <= height)) ||
                ((relation == L_SELECT_IF_GTE) && (w >= width || h >= height)))
                    ival = 1;
            break;
        case L_SELECT_IF_BOTH:
            if (((relation == L_SELECT_IF_LT) && (w < width && h < height)) ||
                ((relation == L_SELECT_IF_GT) && (w > width && h > height)) ||
               ((relation == L_SELECT_IF_LTE) && (w <= width && h <= height)) ||
                ((relation == L_SELECT_IF_GTE) && (w >= width && h >= height)))
                    ival = 1;
            break;
        default:
            L_WARNING("can't get here!\n", procName);
            break;
        }
        numaAddNumber(na, ival);
    }

    return na;
}


/*!
 *  boxaSelectByArea()
 *
 *      Input:  boxas
 *              area (threshold value of width * height)
 *              relation (L_SELECT_IF_LT, L_SELECT_IF_GT,
 *                        L_SELECT_IF_LTE, L_SELECT_IF_GTE)
 *              &changed (<optional return> 1 if changed; 0 if clone returned)
 *      Return: boxad (filtered set), or null on error
 *
 *  Notes:
 *      (1) Uses box clones in the new boxa.
 *      (2) To keep small components, use relation = L_SELECT_IF_LT or
 *          L_SELECT_IF_LTE.
 *          To keep large components, use relation = L_SELECT_IF_GT or
 *          L_SELECT_IF_GTE.
 */
BOXA *
boxaSelectByArea(BOXA     *boxas,
                 l_int32   area,
                 l_int32   relation,
                 l_int32  *pchanged)
{
BOXA  *boxad;
NUMA  *na;

    PROCNAME("boxaSelectByArea");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (relation != L_SELECT_IF_LT && relation != L_SELECT_IF_GT &&
        relation != L_SELECT_IF_LTE && relation != L_SELECT_IF_GTE)
        return (BOXA *)ERROR_PTR("invalid relation", procName, NULL);
    if (pchanged) *pchanged = FALSE;

        /* Compute the indicator array for saving components */
    na = boxaMakeAreaIndicator(boxas, area, relation);

        /* Filter to get output */
    boxad = boxaSelectWithIndicator(boxas, na, pchanged);

    numaDestroy(&na);
    return boxad;
}


/*!
 *  boxaMakeAreaIndicator()
 *
 *      Input:  boxa
 *              area (threshold value of width * height)
 *              relation (L_SELECT_IF_LT, L_SELECT_IF_GT,
 *                        L_SELECT_IF_LTE, L_SELECT_IF_GTE)
 *      Return: na (indicator array), or null on error
 *
 *  Notes:
 *      (1) To keep small components, use relation = L_SELECT_IF_LT or
 *          L_SELECT_IF_LTE.
 *          To keep large components, use relation = L_SELECT_IF_GT or
 *          L_SELECT_IF_GTE.
 */
NUMA *
boxaMakeAreaIndicator(BOXA     *boxa,
                      l_int32   area,
                      l_int32   relation)
{
l_int32  i, n, w, h, ival;
NUMA    *na;

    PROCNAME("boxaMakeAreaIndicator");

    if (!boxa)
        return (NUMA *)ERROR_PTR("boxa not defined", procName, NULL);
    if (relation != L_SELECT_IF_LT && relation != L_SELECT_IF_GT &&
        relation != L_SELECT_IF_LTE && relation != L_SELECT_IF_GTE)
        return (NUMA *)ERROR_PTR("invalid relation", procName, NULL);

    n = boxaGetCount(boxa);
    na = numaCreate(n);
    for (i = 0; i < n; i++) {
        ival = 0;
        boxaGetBoxGeometry(boxa, i, NULL, NULL, &w, &h);

        if ((relation == L_SELECT_IF_LT && w * h < area) ||
            (relation == L_SELECT_IF_GT && w * h > area) ||
            (relation == L_SELECT_IF_LTE && w * h <= area) ||
            (relation == L_SELECT_IF_GTE && w * h >= area))
            ival = 1;
        numaAddNumber(na, ival);
    }

    return na;
}


/*!
 *  boxaSelectWithIndicator()
 *
 *      Input:  boxas
 *              na (indicator numa)
 *              &changed (<optional return> 1 if changed; 0 if clone returned)
 *      Return: boxad, or null on error
 *
 *  Notes:
 *      (1) Returns a boxa clone if no components are removed.
 *      (2) Uses box clones in the new boxa.
 *      (3) The indicator numa has values 0 (ignore) and 1 (accept).
 */
BOXA *
boxaSelectWithIndicator(BOXA     *boxas,
                        NUMA     *na,
                        l_int32  *pchanged)
{
l_int32  i, n, ival, nsave;
BOX     *box;
BOXA    *boxad;

    PROCNAME("boxaSelectWithIndicator");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (!na)
        return (BOXA *)ERROR_PTR("na not defined", procName, NULL);

    nsave = 0;
    n = numaGetCount(na);
    for (i = 0; i < n; i++) {
        numaGetIValue(na, i, &ival);
        if (ival == 1) nsave++;
    }

    if (nsave == n) {
        if (pchanged) *pchanged = FALSE;
        return boxaCopy(boxas, L_CLONE);
    }
    if (pchanged) *pchanged = TRUE;
    boxad = boxaCreate(nsave);
    for (i = 0; i < n; i++) {
        numaGetIValue(na, i, &ival);
        if (ival == 0) continue;
        box = boxaGetBox(boxas, i, L_CLONE);
        boxaAddBox(boxad, box, L_INSERT);
    }

    return boxad;
}


/*---------------------------------------------------------------------*
 *                           Boxa Permutation                          *
 *---------------------------------------------------------------------*/
/*!
 *  boxaPermutePseudorandom()
 *
 *      Input:  boxas (input boxa)
 *      Return: boxad (with boxes permuted), or null on error
 *
 *  Notes:
 *      (1) This does a pseudorandom in-place permutation of the boxes.
 *      (2) The result is guaranteed not to have any boxes in their
 *          original position, but it is not very random.  If you
 *          need randomness, use boxaPermuteRandom().
 */
BOXA *
boxaPermutePseudorandom(BOXA  *boxas)
{
l_int32  n;
NUMA    *na;
BOXA    *boxad;

    PROCNAME("boxaPermutePseudorandom");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxa not defined", procName, NULL);

    n = boxaGetCount(boxas);
    na = numaPseudorandomSequence(n, 0);
    boxad = boxaSortByIndex(boxas, na);
    numaDestroy(&na);
    return boxad;
}


/*!
 *  boxaPermuteRandom()
 *
 *      Input:  boxad (<optional> can be null or equal to boxas)
 *              boxas (input boxa)
 *      Return: boxad (with boxes permuted), or null on error
 *
 *  Notes:
 *      (1) If boxad is null, make a copy of boxas and permute the copy.
 *          Otherwise, boxad must be equal to boxas, and the operation
 *          is done in-place.
 *      (2) This does a random in-place permutation of the boxes,
 *          by swapping each box in turn with a random box.  The
 *          result is almost guaranteed not to have any boxes in their
 *          original position.
 *      (3) MSVC rand() has MAX_RAND = 2^15 - 1, so it will not do
 *          a proper permutation is the number of boxes exceeds this.
 */
BOXA *
boxaPermuteRandom(BOXA  *boxad,
                  BOXA  *boxas)
{
l_int32  i, n, index;

    PROCNAME("boxaPermuteRandom");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxa not defined", procName, NULL);
    if (boxad && (boxad != boxas))
        return (BOXA *)ERROR_PTR("boxad defined but in-place", procName, NULL);

    if (!boxad)
        boxad = boxaCopy(boxas, L_COPY);
    n = boxaGetCount(boxad);
    index = (l_uint32)rand() % n;
    index = L_MAX(1, index);
    boxaSwapBoxes(boxad, 0, index);
    for (i = 1; i < n; i++) {
        index = (l_uint32)rand() % n;
        if (index == i) index--;
        boxaSwapBoxes(boxad, i, index);
    }

    return boxad;
}


/*!
 *  boxaSwapBoxes()
 *
 *      Input:  boxa
 *              i, j (two indices of boxes, that are to be swapped)
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxaSwapBoxes(BOXA    *boxa,
              l_int32  i,
              l_int32  j)
{
l_int32  n;
BOX     *box;

    PROCNAME("boxaSwapBoxes");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    n = boxaGetCount(boxa);
    if (i < 0 || i >= n)
        return ERROR_INT("i invalid", procName, 1);
    if (j < 0 || j >= n)
        return ERROR_INT("j invalid", procName, 1);
    if (i == j)
        return ERROR_INT("i == j", procName, 1);

    box = boxa->box[i];
    boxa->box[i] = boxa->box[j];
    boxa->box[j] = box;
    return 0;
}


/*---------------------------------------------------------------------*
 *                           Boxa Conversions                          *
 *---------------------------------------------------------------------*/
/*!
 *  boxaConvertToPta()
 *
 *      Input:  boxa
 *              ncorners (2 or 4 for the representation of each box)
 *      Return: pta (with @ncorners points for each box in the boxa),
 *                   or null on error
 *
 *  Notes:
 *      (1) If ncorners == 2, we select the UL and LR corners.
 *          Otherwise we save all 4 corners in this order: UL, UR, LL, LR.
 */
PTA *
boxaConvertToPta(BOXA    *boxa,
                 l_int32  ncorners)
{
l_int32  i, n, x, y, w, h;
PTA     *pta;

    PROCNAME("boxaConvertToPta");

    if (!boxa)
        return (PTA *)ERROR_PTR("boxa not defined", procName, NULL);
    if (ncorners != 2 && ncorners != 4)
        return (PTA *)ERROR_PTR("ncorners not 2 or 4", procName, NULL);

    n = boxaGetCount(boxa);
    if ((pta = ptaCreate(n)) == NULL)
        return (PTA *)ERROR_PTR("pta not made", procName, NULL);
    for (i = 0; i < n; i++) {
        boxaGetBoxGeometry(boxa, i, &x, &y, &w, &h);
        ptaAddPt(pta, x, y);
        if (ncorners == 2) {
            ptaAddPt(pta, x + w - 1, y + h - 1);
        } else {
            ptaAddPt(pta, x + w - 1, y);
            ptaAddPt(pta, x, y + h - 1);
            ptaAddPt(pta, x + w - 1, y + h - 1);
        }
    }

    return pta;
}


/*!
 *  ptaConvertToBoxa()
 *
 *      Input:  pta
 *              ncorners (2 or 4 for the representation of each box)
 *      Return: boxa (with one box for each 2 or 4 points in the pta),
 *                    or null on error
 *
 *  Notes:
 *      (1) For 2 corners, the order of the 2 points is UL, LR.
 *          For 4 corners, the order of points is UL, UR, LL, LR.
 *      (2) Each derived box is the minimum szie containing all corners.
 */
BOXA *
ptaConvertToBoxa(PTA     *pta,
                 l_int32  ncorners)
{
l_int32  i, n, nbox, x1, y1, x2, y2, x3, y3, x4, y4, x, y, xmax, ymax;
BOX     *box;
BOXA    *boxa;

    PROCNAME("ptaConvertToBoxa");

    if (!pta)
        return (BOXA *)ERROR_PTR("pta not defined", procName, NULL);
    if (ncorners != 2 && ncorners != 4)
        return (BOXA *)ERROR_PTR("ncorners not 2 or 4", procName, NULL);
    n = ptaGetCount(pta);
    if (n % ncorners != 0)
        return (BOXA *)ERROR_PTR("size % ncorners != 0", procName, NULL);
    nbox = n / ncorners;
    if ((boxa = boxaCreate(nbox)) == NULL)
        return (BOXA *)ERROR_PTR("boxa not made", procName, NULL);
    for (i = 0; i < n; i += ncorners) {
        ptaGetIPt(pta, i, &x1, &y1);
        ptaGetIPt(pta, i + 1, &x2, &y2);
        if (ncorners == 2) {
            box = boxCreate(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
            boxaAddBox(boxa, box, L_INSERT);
            continue;
        }
        ptaGetIPt(pta, i + 2, &x3, &y3);
        ptaGetIPt(pta, i + 3, &x4, &y4);
        x = L_MIN(x1, x3);
        y = L_MIN(y1, y2);
        xmax = L_MAX(x2, x4);
        ymax = L_MAX(y3, y4);
        box = boxCreate(x, y, xmax - x + 1, ymax - y + 1);
        boxaAddBox(boxa, box, L_INSERT);
    }

    return boxa;
}


/*---------------------------------------------------------------------*
 *                        Boxa sequence fitting                        *
 *---------------------------------------------------------------------*/
/*!
 *  boxaSmoothSequence()
 *
 *      Input:  boxas (source boxa)
 *              factor (reject outliers with error greater than this
 *                      number of median errors; typically ~3)
 *              max_error (maximum difference in pixels between fitted
 *                         and original location to allow using the
 *                         original value instead of the fitted value)
 *              debug (1 for debug output)
 *      Return: boxad (fitted boxa), or null on error
 *
 *  Notes:
 *      (1) This does linear fitting separately to the sequences of
 *          even and odd boxes.  It is assumed that in both the even and
 *          odd sets, the box edges vary slowly and linearly across each set.
 */
BOXA *
boxaSmoothSequence(BOXA      *boxas,
                   l_float32  factor,
                   l_int32    max_error,
                   l_int32    debug)
{
l_int32  n;
BOXA    *boxae, *boxao, *boxalfe, *boxalfo, *boxad;

    PROCNAME("boxaSmoothSequence");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if ((n = boxaGetCount(boxas)) < 4) {
        L_WARNING("need at least 4 boxes; returning copy\n", procName);
        return boxaCopy(boxas, L_COPY);
    }

    boxaSplitEvenOdd(boxas, 1, &boxae, &boxao);
    if (debug) {
        boxaWrite("/tmp/boxae.ba", boxae);
        boxaWrite("/tmp/boxao.ba", boxao);
    }

    boxalfe = boxaLinearFit(boxae, factor, max_error, debug);
    boxalfo = boxaLinearFit(boxao, factor, max_error, debug);
    if (debug) {
        boxaWrite("/tmp/boxalfe.ba", boxalfe);
        boxaWrite("/tmp/boxalfo.ba", boxalfo);
    }

    boxad = boxaMergeEvenOdd(boxalfe, boxalfo, 1);
    boxaDestroy(&boxae);
    boxaDestroy(&boxao);
    boxaDestroy(&boxalfe);
    boxaDestroy(&boxalfo);
    return boxad;
}


/*!
 *  boxaLinearFit()
 *
 *      Input:  boxas (source boxa)
 *              factor (reject outliers with error greater than this
 *                      number of median errors; typically ~3)
 *              max_error (maximum difference in pixels between fitted
 *                         and original location to allow using the
 *                         original value instead of the fitted value)
 *              debug (1 for debug output)
 *      Return: boxad (fitted boxa), or null on error
 *
 *  Notes:
 *      (1) Suppose you have a boxa where the box edges are expected
 *          to vary slowly and linearly across the set.  These could
 *          be, for example, noisy measurements of similar regions
 *          on successive scanned pages.
 *      (2) Method: there are 2 basic steps:
 *          (a) Find outliers, separately based on the deviation
 *              from the median of the width and height of the box.
 *              After the width- and height-based outliers are removed,
 *              do a linear LSF for each of the four sides.  Use
 *              @factor to specify tolerance to outliers; use a very large
 *              value of @factor to avoid rejecting points.
 *          (b) Using the LSF of (a), make the final determination of
 *              the four edge locations.  See (3) for details.
 *      (3) The parameter @max_error makes the input values somewhat sticky.
 *          Use the fitted values only when the difference between input
 *          and fitted value is greater than @max_error.  Two special cases:
 *          (a) set @max_error == 0 to use only fitted values in boxad.
 *          (b) set @max_error == 10000 to ignore all fitted values; then
 *              boxad will be the same as boxas.
 *      (4) Invalid input boxes are not used in computation of the LSF,
 *          and the output boxes are found from the LSF.
 *      (5) To enforce additional constraints on the size of each box,
 *          follow this operation with boxaConstrainSize(), taking boxad
 *          as input.
 */
BOXA *
boxaLinearFit(BOXA      *boxas,
              l_float32  factor,
              l_int32    max_error,
              l_int32    debug)
{
l_int32    n, i, w, h, left, top, right, bot, lval, tval, rval, bval;
l_int32    lnew, tnew, rnew, bnew, wnew, hnew, rejectlr, rejecttb;
l_float32  al, bl, at, bt, ar, br, ab, bb;  /* LSF coefficients */
l_float32  medw, medh, medvarw, medvarh;
BOX       *box, *boxempty;
BOXA      *boxalr, *boxatb, *boxad;
NUMA      *naw, *nah;
PTA       *ptal, *ptat, *ptar, *ptab;

    PROCNAME("boxaLinearFit");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if ((n = boxaGetCount(boxas)) < 2)
        return (BOXA *)ERROR_PTR("need at least 2 boxes", procName, NULL);

        /* Remove outliers based on width and height.
         * First find the median width and the median variation from
         * the median width.  Ditto for the height. */
    boxaExtractAsNuma(boxas, NULL, NULL, &naw, &nah, 0);
    numaGetMedianVariation(naw, &medw, &medvarw);
    numaGetMedianVariation(nah, &medh, &medvarh);
    numaDestroy(&naw);
    numaDestroy(&nah);

    if (debug) {
        fprintf(stderr, "medw = %7.3f, medvarw = %7.3f\n", medw, medvarw);
        fprintf(stderr, "medh = %7.3f, medvarh = %7.3f\n", medh, medvarh);
    }

        /* To fit the left and right sides, only use boxes whose
         * width is within (factor * medvarw) of the median width.
         * Ditto for the top and bottom sides.  Add empty boxes
         * in as placeholders so that the index remains the same
         * as in boxas. */
    boxalr = boxaCreate(n);
    boxatb = boxaCreate(n);
    boxempty = boxCreate(0, 0, 0, 0);  /* placeholders */
    rejectlr = rejecttb = 0;
    for (i = 0; i < n; i++) {
        if ((box = boxaGetValidBox(boxas, i, L_CLONE)) == NULL) {
            boxaAddBox(boxalr, boxempty, L_COPY);
            boxaAddBox(boxatb, boxempty, L_COPY);
            continue;
        }
        boxGetGeometry(box, NULL, NULL, &w, &h);
        if (L_ABS(w - medw) <= factor * medvarw) {
            boxaAddBox(boxalr, box, L_COPY);
        } else {
            rejectlr++;
            boxaAddBox(boxalr, boxempty, L_COPY);
        }
        if (L_ABS(h - medh) <= factor * medvarh) {
            boxaAddBox(boxatb, box, L_COPY);
        } else {
            rejecttb++;
            boxaAddBox(boxatb, boxempty, L_COPY);
        }
        boxDestroy(&box);
    }
    boxDestroy(&boxempty);
    if (boxaGetCount(boxalr) < 2 || boxaGetCount(boxatb) < 2) {
        boxaDestroy(&boxalr);
        boxaDestroy(&boxatb);
        return (BOXA *)ERROR_PTR("need at least 2 valid boxes", procName, NULL);
    }

    if (debug) {
        L_INFO("# lr reject = %d, # tb reject = %d\n", procName,
               rejectlr, rejecttb);
        boxaWrite("/tmp/boxalr.ba", boxalr);
        boxaWrite("/tmp/boxatb.ba", boxatb);
    }

        /* Extract the valid left and right box sides, along with the box
         * index, from boxalr.  This only extracts pts corresponding to
         * valid boxes.  Ditto: top and bottom sides from boxatb. */
    boxaExtractAsPta(boxalr, &ptal, NULL, &ptar, NULL, 0);
    boxaExtractAsPta(boxatb, NULL, &ptat, NULL, &ptab, 0);
    boxaDestroy(&boxalr);
    boxaDestroy(&boxatb);

    if (debug) {
        ptaWrite("/tmp/ptal.pta", ptal, 1);
        ptaWrite("/tmp/ptar.pta", ptar, 1);
        ptaWrite("/tmp/ptat.pta", ptat, 1);
        ptaWrite("/tmp/ptab.pta", ptab, 1);
    }

        /* A linear LSF fit to the points that are width and
         * height validated should work.  So, e.g., we don't need to use
         *    ptaNoisyLinearLSF(ptal, factor, NULL, &al, &bl, NULL, NULL); */
    ptaGetLinearLSF(ptal, &al, &bl, NULL);
    ptaGetLinearLSF(ptat, &at, &bt, NULL);
    ptaGetLinearLSF(ptar, &ar, &br, NULL);
    ptaGetLinearLSF(ptab, &ab, &bb, NULL);

        /* Use the LSF smoothed values when the error is large. */
    boxad = boxaCreate(n);
    for (i = 0; i < n; i++) {
        lval = (l_int32)(al * i + bl + 0.5);
        tval = (l_int32)(at * i + bt + 0.5);
        rval = (l_int32)(ar * i + br + 0.5);
        bval = (l_int32)(ab * i + bb + 0.5);
        box = boxaGetValidBox(boxas, i, L_CLONE);
        if (box) {
            boxGetGeometry(box, &left, &top, &w, &h);
            boxDestroy(&box);
        } else {  /* use something big to force using the LSF value */
            left = 100000000;
            top = 100000000;
            w = 100000000;
            h = 100000000;
        }
        right = left + w - 1;
        bot = top + h - 1;
        lnew = (L_ABS(lval - left) <= max_error) ? left : lval;
        tnew = (L_ABS(tval - top) <= max_error) ? top : tval;
        rnew = (L_ABS(rval - right) <= max_error) ? right : rval;
        bnew = (L_ABS(bval - bot) <= max_error) ? bot : bval;
        wnew = rnew - lnew + 1;
        hnew = bnew - tnew + 1;
        box = boxCreate(lnew, tnew, wnew, hnew);
        boxaAddBox(boxad, box, L_INSERT);
    }
    if (debug)
        boxaPlotSides(boxad, NULL, NULL, NULL, NULL, NULL, GPLOT_X11);

    ptaDestroy(&ptal);
    ptaDestroy(&ptat);
    ptaDestroy(&ptar);
    ptaDestroy(&ptab);
    return boxad;
}


/*!
 *  boxaConstrainSize()
 *
 *      Input:  boxas
 *              width (force width of all boxes to this size;
 *                     input 0 to use the median width)
 *              widthflag (L_ADJUST_SKIP, L_ADJUST_LEFT, L_ADJUST_RIGHT,
 *                         or L_ADJUST_LEFT_AND_RIGHT)
 *              height (force height of all boxes to this size;
 *                      input 0 to use the median height)
 *              heightflag (L_ADJUST_SKIP, L_ADJUST_TOP, L_ADJUST_BOT,
 *                          or L_ADJUST_TOP_AND_BOT)
 *      Return: boxad (adjusted so all boxes are the same size)
 *
 *  Notes:
 *      (1) Forces either width or height (or both) of every box in
 *          the boxa to a specified size, by moving the indicated sides.
 *      (2) All input boxes should be valid.  Median values will be
 *          used with invalid boxes.
 *      (3) Typical input might be the output of boxaLinearFit(),
 *          where each side has been fit.
 *      (4) Unlike boxaAdjustWidthToTarget() and boxaAdjustHeightToTarget(),
 *          this is not dependent on a difference threshold to change the size.
 */
BOXA *
boxaConstrainSize(BOXA    *boxas,
                  l_int32  width,
                  l_int32  widthflag,
                  l_int32  height,
                  l_int32  heightflag)
{
l_int32  n, i, w, h, delw, delh, del_left, del_right, del_top, del_bot;
BOX     *medbox, *boxs, *boxd;
BOXA    *boxad;

    PROCNAME("boxaConstrainSize");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);

        /* Use median value if requested */
    medbox = boxaGetMedian(boxas);
    if (width == 0 || height == 0) {
        boxGetGeometry(medbox, NULL, NULL, &w, &h);
        if (width == 0)
            width = w;
        if (height == 0)
            height = h;
    }

    n = boxaGetCount(boxas);
    boxad = boxaCreate(n);
    for (i = 0; i < n; i++) {
        boxs = boxaGetValidBox(boxas, i, L_CLONE);
        if (!boxs) {
            L_ERROR("invalid box %d; using median\n", procName, i);
            boxs = boxCopy(medbox);
        }
        boxGetGeometry(boxs, NULL, NULL, &w, &h);
        delw = width - w;
        delh = height - h;
        del_left = del_right = del_top = del_bot = 0;
        if (widthflag == L_ADJUST_LEFT) {
            del_left = -delw;
        } else if (widthflag == L_ADJUST_RIGHT) {
            del_right = delw;
        } else {
            del_left = -delw / 2;
            del_right = delw / 2 + L_SIGN(delw) * (delw & 1);
        }
        if (heightflag == L_ADJUST_TOP) {
            del_top = -delh;
        } else if (heightflag == L_ADJUST_BOT) {
            del_bot = delh;
        } else {
            del_top = -delh / 2;
            del_bot = delh / 2 + L_SIGN(delh) * (delh & 1);
        }
        boxd = boxAdjustSides(NULL, boxs, del_left, del_right,
                              del_top, del_bot);
        boxaAddBox(boxad, boxd, L_INSERT);
        boxDestroy(&boxs);
    }

    boxDestroy(&medbox);
    return boxad;
}


/*!
 *  boxaReconcileEvenOddHeight()
 *
 *      Input:  boxas (containing at least 3 valid boxes in even and odd)
 *              sides (L_ADJUST_TOP, L_ADJUST_BOT, L_ADJUST_TOP_AND_BOT)
 *              delh (threshold on median height difference)
 *              op (L_ADJUST_CHOOSE_MIN, L_ADJUST_CHOOSE_MAX)
 *              factor (> 0.0, typically near 1.0)
 *      Return: boxad (adjusted)
 *
 *  Notes:
 *      (1) The basic idea is to reconcile differences in box height
 *          in the even and odd boxes, by moving the top and/or bottom
 *          edges in the even and odd boxes.  Choose the edge or edges
 *          to be moved, whether to adjust the boxes with the min
 *          or the max of the medians, and the threshold on the median
 *          difference between even and odd box heights for the operations
 *          to take place.  The same threshold is also used to
 *          determine if each individual box edge is to be adjusted.
 *      (2) Boxes are conditionally reset with either the same top (y)
 *          value or the same bottom value, or both.  The value is
 *          determined by the greater or lesser of the medians of the
 *          even and odd boxes, with the choice depending on the value
 *          of @op, which selects for either min or max median height.
 *          If the median difference between even and odd boxes is
 *          greater than @dely, then any individual box edge that differs
 *          from the selected median by more than @dely is set to
 *          the selected median times a factor typically near 1.0.
 *      (3) Note that if selecting for minimum height, you will choose
 *          the largest y-value for the top and the smallest y-value for
 *          the bottom of the box.
 *      (4) Typical input might be the output of boxaSmoothSequence(),
 *          where even and odd boxa have been independently regulated.
 *      (5) Require at least 3 valid even boxes and 3 valid odd boxes.
 *          Median values will be used for invalid boxes.
 */
BOXA *
boxaReconcileEvenOddHeight(BOXA      *boxas,
                           l_int32    sides,
                           l_int32    delh,
                           l_int32    op,
                           l_float32  factor)
{
l_int32  n, ne, no, he, ho, hmed, doeven;
BOX     *boxe, *boxo;
BOXA    *boxae, *boxao, *boxa1e, *boxa1o, *boxad;

    PROCNAME("boxaReconcileEvenOddHeight");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (sides != L_ADJUST_TOP && sides != L_ADJUST_BOT &&
        sides != L_ADJUST_TOP_AND_BOT) {
        L_WARNING("no action requested; returning copy\n", procName);
        return boxaCopy(boxas, L_COPY);
    }
    if ((n = boxaGetValidCount(boxas)) < 6) {
        L_WARNING("need at least 6 valid boxes; returning copy\n", procName);
        return boxaCopy(boxas, L_COPY);
    }
    if (factor <= 0.0) {
        L_WARNING("invalid factor; setting to 1.0\n", procName);
        factor = 1.0;
    }

        /* Require at least 3 valid boxes of both types */
    boxaSplitEvenOdd(boxas, 0, &boxae, &boxao);
    if (boxaGetValidCount(boxae) < 3 || boxaGetValidCount(boxao) < 3) {
        boxaDestroy(&boxae);
        boxaDestroy(&boxao);
        return boxaCopy(boxas, L_COPY);
    }
    ne = boxaGetCount(boxae);
    no = boxaGetCount(boxao);

        /* Get the median heights for each set */
    boxa1e = boxaSort(boxae, L_SORT_BY_HEIGHT, L_SORT_INCREASING, NULL);
    boxa1o = boxaSort(boxao, L_SORT_BY_HEIGHT, L_SORT_INCREASING, NULL);
    boxe = boxaGetBox(boxa1e, ne / 2, L_COPY);  /* median ht even boxes */
    boxo = boxaGetBox(boxa1o, no / 2, L_COPY);  /* median ht odd boxes */
    boxGetGeometry(boxe, NULL, NULL, NULL, &he);
    boxGetGeometry(boxo, NULL, NULL, NULL, &ho);
    boxaDestroy(&boxa1e);
    boxaDestroy(&boxa1o);
    boxDestroy(&boxe);
    boxDestroy(&boxo);
    L_INFO("median he = %d, median ho = %d\n", procName, he, ho);

        /* If the difference in median height reaches the threshold @delh,
         * only adjust the side(s) of one of the sets.  If we choose
         * the minimum median height as the target, allow the target
         * to be scaled by a factor, typically near 1.0, of the
         * minimum median height.  And similarly if the target is
         * the maximum median height. */
    if (L_ABS(he - ho) > delh) {
        if (op == L_ADJUST_CHOOSE_MIN) {
            doeven = (ho < he) ? TRUE : FALSE;
            hmed = (l_int32)(factor * L_MIN(he, ho));
            hmed = L_MIN(hmed, L_MAX(he, ho));  /* don't make it bigger! */
        } else {  /* max height */
            doeven = (ho > he) ? TRUE : FALSE;
            hmed = (l_int32)(factor * L_MAX(he, ho));
            hmed = L_MAX(hmed, L_MIN(he, ho));  /* don't make it smaller! */
        }
        if (doeven) boxaAdjustHeightToTarget(boxae, boxae, sides, hmed, delh);
        if (!doeven) boxaAdjustHeightToTarget(boxao, boxao, sides, hmed, delh);
    }

    boxad = boxaMergeEvenOdd(boxae, boxao, 0);
    boxaDestroy(&boxae);
    boxaDestroy(&boxao);
    return boxad;
}


/*!
 *  boxaPlotSides()
 *
 *      Input:  boxas (source boxa)
 *              plotname (<optional>, can be NULL)
 *              &nal (<optional return> na of left sides)
 *              &nat (<optional return> na of top sides)
 *              &nar (<optional return> na of right sides)
 *              &nab (<optional return> na of bottom sides)
 *              outformat (GPLOT_NONE for no output; GPLOT_PNG for png, etc)
 *               ut
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is a debugging function to show the progression of
 *          the four sides in the boxes.  There must be at least 2 boxes.
 *      (2) One of three conditions holds:
 *          (a) only the even indices have valid boxes
 *          (b) only the odd indices have valid boxes
 *          (c) all indices have valid boxes
 *          This condition is determined by looking at the first 2 boxes.
 *      (3) The plotfiles are put in /tmp, and are named either with
 *          @plotname or, if NULL, a default name.
 */
l_int32
boxaPlotSides(BOXA        *boxa,
              const char  *plotname,
              NUMA       **pnal,
              NUMA       **pnat,
              NUMA       **pnar,
              NUMA       **pnab,
              l_int32      outformat)
{
char            buf[128];
static l_int32  plotid = 0;
l_int32         n, i, w, h, left, top, right, bot;
l_float32       startx, delx;
BOX            *box, *boxe, *boxo;
GPLOT          *gplot;
NUMA           *nal, *nat, *nar, *nab;

    PROCNAME("boxaPlotSides");

    if (pnal) *pnal = NULL;
    if (pnat) *pnat = NULL;
    if (pnar) *pnar = NULL;
    if (pnab) *pnab = NULL;
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if ((n = boxaGetCount(boxa)) < 2)
        return ERROR_INT("less than 2 boxes", procName, 1);

        /* Determine which condition holds for valid boxes */
    delx = 1;
    boxe = boxaGetValidBox(boxa, 0, L_CLONE);
    boxo = boxaGetValidBox(boxa, 1, L_CLONE);
    if (!boxe) {
        startx = 1;
        delx = 2;
    } else if (!boxo) {
        startx = 0;
        delx = 2;
    }
    boxDestroy(&boxe);
    boxDestroy(&boxo);

        /* Build the numas for each side */
    nal = numaCreate(n);
    nat = numaCreate(n);
    nar = numaCreate(n);
    nab = numaCreate(n);
    if (delx == 2) {
        numaSetParameters(nal, startx, delx);
        numaSetParameters(nat, startx, delx);
        numaSetParameters(nar, startx, delx);
        numaSetParameters(nab, startx, delx);
    }

    for (i = 0; i < n; i++) {
        if ((box = boxaGetValidBox(boxa, i, L_CLONE)) == NULL)
            continue;
        boxGetGeometry(box, &left, &top, &w, &h);
        right = left + w - 1;
        bot = top + h - 1;
        numaAddNumber(nal, left);
        numaAddNumber(nat, top);
        numaAddNumber(nar, right);
        numaAddNumber(nab, bot);
        boxDestroy(&box);
    }

        /* Plot them */
    if (outformat < 0 || outformat > GPLOT_LATEX) {
        L_ERROR("invalid gplot format\n", procName);
        outformat = 0;
    }

    if (outformat > 0) {
        if (plotname)
            snprintf(buf, sizeof(buf), "/tmp/%s", plotname);
        else
            snprintf(buf, sizeof(buf), "/tmp/boxsides.%d", plotid++);
        gplot = gplotCreate(buf, outformat, "Box sides vs. box index",
                            "box index", "box location");
        gplotAddPlot(gplot, NULL, nal, GPLOT_LINES, "left side");
        gplotAddPlot(gplot, NULL, nat, GPLOT_LINES, "top side");
        gplotAddPlot(gplot, NULL, nar, GPLOT_LINES, "right side");
        gplotAddPlot(gplot, NULL, nab, GPLOT_LINES, "bottom side");
        gplotMakeOutput(gplot);
        gplotDestroy(&gplot);
    }

    if (pnal)
        *pnal = nal;
    else
        numaDestroy(&nal);
    if (pnat)
        *pnat = nat;
    else
        numaDestroy(&nat);
    if (pnar)
        *pnar = nar;
    else
        numaDestroy(&nar);
    if (pnab)
        *pnab = nab;
    else
        numaDestroy(&nab);
    return 0;
}


/*---------------------------------------------------------------------*
 *                    Miscellaneous Boxa functions                     *
 *---------------------------------------------------------------------*/
/*!
 *  boxaGetExtent()
 *
 *      Input:  boxa
 *              &w  (<optional return> width)
 *              &h  (<optional return> height)
 *              &box (<optional return>, minimum box containing all boxes
 *                    in boxa)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The returned w and h are the minimum size image
 *          that would contain all boxes untranslated.
 *      (2) If there are no valid boxes, returned w and h are 0 and
 *          all parameters in the returned box are 0.  This
 *          is not an error, because an empty boxa is valid and
 *          boxaGetExtent() is required for serialization.
 */
l_int32
boxaGetExtent(BOXA     *boxa,
              l_int32  *pw,
              l_int32  *ph,
              BOX     **pbox)
{
l_int32  i, n, x, y, w, h, xmax, ymax, xmin, ymin, found;

    PROCNAME("boxaGetExtent");

    if (!pw && !ph && !pbox)
        return ERROR_INT("no ptrs defined", procName, 1);
    if (pbox) *pbox = NULL;
    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    n = boxaGetCount(boxa);
    xmax = ymax = 0;
    xmin = ymin = 100000000;
    found = FALSE;
    for (i = 0; i < n; i++) {
        boxaGetBoxGeometry(boxa, i, &x, &y, &w, &h);
        if (w <= 0 || h <= 0)
            continue;
        found = TRUE;
        xmin = L_MIN(xmin, x);
        ymin = L_MIN(ymin, y);
        xmax = L_MAX(xmax, x + w);
        ymax = L_MAX(ymax, y + h);
    }
    if (found == FALSE)  /* no valid boxes in boxa */
        xmin = ymin = 0;
    if (pw) *pw = xmax;
    if (ph) *ph = ymax;
    if (pbox)
      *pbox = boxCreate(xmin, ymin, xmax - xmin, ymax - ymin);

    return 0;
}


/*!
 *  boxaGetCoverage()
 *
 *      Input:  boxa
 *              wc, hc (dimensions of overall clipping rectangle with UL
 *                      corner at (0, 0) that is covered by the boxes.
 *              exactflag (1 for guaranteeing an exact result; 0 for getting
 *                         an exact result only if the boxes do not overlap)
 *              &fract (<return> sum of box area as fraction of w * h)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The boxes in boxa are clipped to the input rectangle.
 *      (2) * When @exactflag == 1, we generate a 1 bpp pix of size
 *            wc x hc, paint all the boxes black, and count the fg pixels.
 *            This can take 1 msec on a large page with many boxes.
 *          * When @exactflag == 0, we clip each box to the wc x hc region
 *            and sum the resulting areas.  This is faster.
 *          * The results are the same when none of the boxes overlap
 *            within the wc x hc region.
 */
l_int32
boxaGetCoverage(BOXA       *boxa,
                l_int32     wc,
                l_int32     hc,
                l_int32     exactflag,
                l_float32  *pfract)
{
l_int32  i, n, x, y, w, h, sum;
BOX     *box, *boxc;
PIX     *pixt;

    PROCNAME("boxaGetCoverage");

    if (!pfract)
        return ERROR_INT("&fract not defined", procName, 1);
    *pfract = 0.0;
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    n = boxaGetCount(boxa);
    if (n == 0)
        return ERROR_INT("no boxes in boxa", procName, 1);

    if (exactflag == 0) {  /* quick and dirty */
        sum = 0;
        for (i = 0; i < n; i++) {
            box = boxaGetBox(boxa, i, L_CLONE);
            if ((boxc = boxClipToRectangle(box, wc, hc)) != NULL) {
                boxGetGeometry(boxc, NULL, NULL, &w, &h);
                sum += w * h;
                boxDestroy(&boxc);
            }
            boxDestroy(&box);
        }
    } else {  /* slower and exact */
        pixt = pixCreate(wc, hc, 1);
        for (i = 0; i < n; i++) {
            box = boxaGetBox(boxa, i, L_CLONE);
            boxGetGeometry(box, &x, &y, &w, &h);
            pixRasterop(pixt, x, y, w, h, PIX_SET, NULL, 0, 0);
            boxDestroy(&box);
        }
        pixCountPixels(pixt, &sum, NULL);
        pixDestroy(&pixt);
    }

    *pfract = (l_float32)sum / (l_float32)(wc * hc);
    return 0;
}


/*!
 *  boxaaSizeRange()
 *
 *      Input:  baa
 *              &minw, &minh, &maxw, &maxh (<optional return> range of
 *                                          dimensions of all boxes)
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxaaSizeRange(BOXAA    *baa,
               l_int32  *pminw,
               l_int32  *pminh,
               l_int32  *pmaxw,
               l_int32  *pmaxh)
{
l_int32  minw, minh, maxw, maxh, minbw, minbh, maxbw, maxbh, i, n;
BOXA    *boxa;

    PROCNAME("boxaaSizeRange");

    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);
    if (!pminw && !pmaxw && !pminh && !pmaxh)
        return ERROR_INT("no data can be returned", procName, 1);

    minw = minh = 100000000;
    maxw = maxh = 0;
    n = boxaaGetCount(baa);
    for (i = 0; i < n; i++) {
        boxa = boxaaGetBoxa(baa, i, L_CLONE);
        boxaSizeRange(boxa, &minbw, &minbh, &maxbw, &maxbh);
        if (minbw < minw)
            minw = minbw;
        if (minbh < minh)
            minh = minbh;
        if (maxbw > maxw)
            maxw = maxbw;
        if (maxbh > maxh)
            maxh = maxbh;
    }

    if (pminw) *pminw = minw;
    if (pminh) *pminh = minh;
    if (pmaxw) *pmaxw = maxw;
    if (pmaxh) *pmaxh = maxh;
    return 0;
}


/*!
 *  boxaSizeRange()
 *
 *      Input:  boxa
 *              &minw, &minh, &maxw, &maxh (<optional return> range of
 *                                          dimensions of box in the array)
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxaSizeRange(BOXA     *boxa,
              l_int32  *pminw,
              l_int32  *pminh,
              l_int32  *pmaxw,
              l_int32  *pmaxh)
{
l_int32  minw, minh, maxw, maxh, i, n, w, h;

    PROCNAME("boxaSizeRange");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (!pminw && !pmaxw && !pminh && !pmaxh)
        return ERROR_INT("no data can be returned", procName, 1);

    minw = minh = 100000000;
    maxw = maxh = 0;
    n = boxaGetCount(boxa);
    for (i = 0; i < n; i++) {
        boxaGetBoxGeometry(boxa, i, NULL, NULL, &w, &h);
        if (w < minw)
            minw = w;
        if (h < minh)
            minh = h;
        if (w > maxw)
            maxw = w;
        if (h > maxh)
            maxh = h;
    }

    if (pminw) *pminw = minw;
    if (pminh) *pminh = minh;
    if (pmaxw) *pmaxw = maxw;
    if (pmaxh) *pmaxh = maxh;
    return 0;
}


/*!
 *  boxaLocationRange()
 *
 *      Input:  boxa
 *              &minx, &miny, &maxx, &maxy (<optional return> range of
 *                                          UL corner positions)
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxaLocationRange(BOXA     *boxa,
                  l_int32  *pminx,
                  l_int32  *pminy,
                  l_int32  *pmaxx,
                  l_int32  *pmaxy)
{
l_int32  minx, miny, maxx, maxy, i, n, x, y;

    PROCNAME("boxaLocationRange");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (!pminx && !pminy && !pmaxx && !pmaxy)
        return ERROR_INT("no data can be returned", procName, 1);

    minx = miny = 100000000;
    maxx = maxy = 0;
    n = boxaGetCount(boxa);
    for (i = 0; i < n; i++) {
        boxaGetBoxGeometry(boxa, i, &x, &y, NULL, NULL);
        if (x < minx)
            minx = x;
        if (y < miny)
            miny = y;
        if (x > maxx)
            maxx = x;
        if (y > maxy)
            maxy = y;
    }

    if (pminx) *pminx = minx;
    if (pminy) *pminy = miny;
    if (pmaxx) *pmaxx = maxx;
    if (pmaxy) *pmaxy = maxy;

    return 0;
}


/*!
 *  boxaGetArea()
 *
 *      Input:  boxa
 *              &area (<return> total area of all boxes)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Measures the total area of the boxes, without regard to overlaps.
 */
l_int32
boxaGetArea(BOXA     *boxa,
            l_int32  *parea)
{
l_int32  i, n, w, h;

    PROCNAME("boxaGetArea");

    if (!parea)
        return ERROR_INT("&area not defined", procName, 1);
    *parea = 0;
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    n = boxaGetCount(boxa);
    for (i = 0; i < n; i++) {
        boxaGetBoxGeometry(boxa, i, NULL, NULL, &w, &h);
        *parea += w * h;
    }
    return 0;
}


/*!
 *  boxaDisplayTiled()
 *
 *      Input:  boxa
 *              pixa (<optional> background for each box)
 *              maxwidth (of output image)
 *              linewidth (width of box outlines, before scaling)
 *              scalefactor (applied to every box; use 1.0 for no scaling)
 *              background (0 for white, 1 for black; this is the color
 *                          of the spacing between the images)
 *              spacing  (between images, and on outside)
 *              border (width of black border added to each image;
 *                      use 0 for no border)
 *              fontdir (<optional> can be NULL; use to number the boxes)
 *      Return: pixd (of tiled images of boxes), or null on error
 *
 *  Notes:
 *      (1) Displays each box separately in a tiled 32 bpp image.
 *      (2) If pixa is defined, it must have the same count as the boxa,
 *          and it will be a background over with each box is rendered.
 *          If pixa is not defined, the boxes will be rendered over
 *          blank images of identical size.
 *      (3) See pixaDisplayTiledInRows() for other parameters.
 */
PIX *
boxaDisplayTiled(BOXA        *boxa,
                 PIXA        *pixa,
                 l_int32      maxwidth,
                 l_int32      linewidth,
                 l_float32    scalefactor,
                 l_int32      background,
                 l_int32      spacing,
                 l_int32      border,
                 const char  *fontdir)
{
char     buf[32];
l_int32  i, n, npix, w, h;
L_BMF   *bmf;
BOX     *box;
PIX     *pix1, *pix2, *pixd;
PIXA    *pixat;

    PROCNAME("boxaDisplayTiled");

    if (!boxa)
        return (PIX *)ERROR_PTR("boxa not defined", procName, NULL);
    n = boxaGetCount(boxa);
    if (pixa) {
        npix = pixaGetCount(pixa);
        if (n != npix)
            return (PIX *)ERROR_PTR("boxa and pixa counts differ",
                                    procName, NULL);
    }

    bmf = NULL;
    if (fontdir) {
        if ((bmf = bmfCreate(fontdir, 6)) == NULL) {
            L_ERROR("can't find fonts; skipping them\n", procName);
            fontdir = NULL;
        }
    }

    pixat = pixaCreate(n);
    boxaGetExtent(boxa, &w, &h, NULL);
    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        if (!pixa) {
            pix1 = pixCreate(w, h, 32);
            pixSetAll(pix1);
        } else {
            pix1 = pixaGetPix(pixa, i, L_COPY);
        }
        if (fontdir) {
            pixSetBorderVal(pix1, 0, 0, 0, 2, 0x0000ff00);
            snprintf(buf, sizeof(buf), "%d", i);
            pix2 = pixAddSingleTextblock(pix1, bmf, buf, 0x00ff0000,
                                         L_ADD_BELOW, NULL);
        } else {
            pix2 = pixClone(pix1);
        }
        pixDestroy(&pix1);
        pixRenderBoxArb(pix2, box, linewidth, 255, 0, 0);
        pixaAddPix(pixat, pix2, L_INSERT);
        boxDestroy(&box);
    }
    bmfDestroy(&bmf);

    pixd = pixaDisplayTiledInRows(pixat, 32, maxwidth, scalefactor, background,
                                  spacing, border);
    pixaDestroy(&pixat);
    return pixd;
}

