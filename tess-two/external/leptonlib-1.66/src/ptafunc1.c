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
 *   ptafunc1.c
 *
 *      Pta and Ptaa rearrangements
 *           PTA      *ptaSubsample()
 *           l_int32   ptaJoin()
 *           PTA      *ptaReverse()
 *           PTA      *ptaCyclicPerm()
 *           PTA      *ptaSort()
 *           PTA      *ptaRemoveDuplicates()
 *           PTAA     *ptaaSortByIndex()
 *
 *      Geometric
 *           BOX      *ptaGetBoundingRegion()
 *           l_int32  *ptaGetRange()
 *           PTA      *ptaGetInsideBox()
 *           PTA      *pixFindCornerPixels()
 *           l_int32   ptaContainsPt()
 *           l_int32   ptaTestIntersection()
 *           PTA      *ptaTransform()
 *
 *      Least Squares Fit
 *           l_int32   ptaGetLinearLSF()
 *           l_int32   ptaGetQuadraticLSF()
 *           l_int32   ptaGetCubicLSF()
 *           l_int32   ptaGetQuarticLSF()
 *           l_int32   applyLinearFit()
 *           l_int32   applyQuadraticFit()
 *           l_int32   applyCubicFit()
 *           l_int32   applyQuarticFit()
 *
 *      Interconversions with Pix
 *           l_int32   pixPlotAlongPta()
 *           PTA      *ptaGetPixelsFromPix()
 *           PIX      *pixGenerateFromPta()
 *           PTA      *ptaGetBoundaryPixels()
 *           PTAA     *ptaaGetBoundaryPixels()
 *
 *      Display Pta and Ptaa
 *           PIX      *pixDisplayPta()
 *           PIX      *pixDisplayPtaa()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

    /* Default spreading factor for hashing pts in a plane */
static const l_int32  DEFAULT_SPREADING_FACTOR = 7500;


/*---------------------------------------------------------------------*
 *                           Pta rearrangements                        *
 *---------------------------------------------------------------------*/
/*!
 *  ptaSubsample()
 *
 *      Input:  ptas
 *              subfactor (subsample factor, >= 1)
 *      Return: ptad (evenly sampled pt values from ptas, or null on error
 */
PTA *
ptaSubsample(PTA     *ptas,
             l_int32  subfactor)
{
l_int32    n, i;
l_float32  x, y;
PTA       *ptad;

    PROCNAME("pixSubsample");

    if (!ptas)
        return (PTA *)ERROR_PTR("ptas not defined", procName, NULL);
    if (subfactor < 1)
        return (PTA *)ERROR_PTR("subfactor < 1", procName, NULL);

    ptad = ptaCreate(0);
    n = ptaGetCount(ptas);
    for (i = 0; i < n; i++) {
        if (i % subfactor != 0) continue;
        ptaGetPt(ptas, i, &x, &y);
        ptaAddPt(ptad, x, y);
    }

    return ptad;
}


/*!
 *  ptaJoin()
 *
 *      Input:  ptad  (dest pta; add to this one)
 *              ptas  (source pta; add from this one)
 *              istart  (starting index in ptas)
 *              iend  (ending index in ptas; use 0 to cat all)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) istart < 0 is taken to mean 'read from the start' (istart = 0)
 *      (2) iend <= 0 means 'read to the end'
 */
l_int32
ptaJoin(PTA     *ptad,
        PTA     *ptas,
        l_int32  istart,
        l_int32  iend)
{
l_int32  ns, i, x, y;

    PROCNAME("ptaJoin");

    if (!ptad)
        return ERROR_INT("ptad not defined", procName, 1);
    if (!ptas)
        return ERROR_INT("ptas not defined", procName, 1);
    ns = ptaGetCount(ptas);
    if (istart < 0)
        istart = 0;
    if (istart >= ns)
        return ERROR_INT("istart out of bounds", procName, 1);
    if (iend <= 0)
        iend = ns - 1;
    if (iend >= ns)
        return ERROR_INT("iend out of bounds", procName, 1);
    if (istart > iend)
        return ERROR_INT("istart > iend; no pts", procName, 1);

    for (i = istart; i <= iend; i++) {
        ptaGetIPt(ptas, i, &x, &y);
        ptaAddPt(ptad, x, y);
    }

    return 0;
}


/*!
 *  ptaReverse()
 *
 *      Input:  ptas
 *              type  (0 for float values; 1 for integer values)
 *      Return: ptad (reversed pta), or null on error
 */
PTA  *
ptaReverse(PTA     *ptas,
           l_int32  type)
{
l_int32    n, i, ix, iy;
l_float32  x, y;
PTA       *ptad;

    PROCNAME("ptaReverse");

    if (!ptas)
        return (PTA *)ERROR_PTR("ptas not defined", procName, NULL);

    n = ptaGetCount(ptas);
    if ((ptad = ptaCreate(n)) == NULL)
        return (PTA *)ERROR_PTR("ptad not made", procName, NULL);
    for (i = n - 1; i >= 0; i--) {
        if (type == 0) {
            ptaGetPt(ptas, i, &x, &y);
            ptaAddPt(ptad, x, y);
        }
        else {  /* type == 1 */
            ptaGetIPt(ptas, i, &ix, &iy);
            ptaAddPt(ptad, ix, iy);
        }
    }

    return ptad;
}


/*!
 *  ptaCyclicPerm()
 *
 *      Input:  ptas
 *              xs, ys  (start point; must be in ptas)
 *      Return: ptad (cyclic permutation, starting and ending at (xs, ys),
 *              or null on error
 *
 *  Notes:
 *      (1) Check to insure that (a) ptas is a closed path where
 *          the first and last points are identical, and (b) the
 *          resulting pta also starts and ends on the same point
 *          (which in this case is (xs, ys).
 */
PTA  *
ptaCyclicPerm(PTA     *ptas,
              l_int32  xs,
              l_int32  ys)
{
l_int32  n, i, x, y, j, index, state;
l_int32  x1, y1, x2, y2;
PTA     *ptad;

    PROCNAME("ptaCyclicPerm");

    if (!ptas)
        return (PTA *)ERROR_PTR("ptas not defined", procName, NULL);

    n = ptaGetCount(ptas);

        /* Verify input data */
    ptaGetIPt(ptas, 0, &x1, &y1);
    ptaGetIPt(ptas, n - 1, &x2, &y2);
    if (x1 != x2 || y1 != y2)
        return (PTA *)ERROR_PTR("start and end pts not same", procName, NULL);
    state = L_NOT_FOUND;
    for (i = 0; i < n; i++) {
        ptaGetIPt(ptas, i, &x, &y);
        if (x == xs && y == ys) {
            state = L_FOUND;
            break;
        }
    }
    if (state == L_NOT_FOUND)
        return (PTA *)ERROR_PTR("start pt not in ptas", procName, NULL);

    if ((ptad = ptaCreate(n)) == NULL)
        return (PTA *)ERROR_PTR("ptad not made", procName, NULL);
    for (j = 0; j < n - 1; j++) {
        if (i + j < n - 1)
            index = i + j;
        else
            index = (i + j + 1) % n;
        ptaGetIPt(ptas, index, &x, &y);
        ptaAddPt(ptad, x, y);
    }
    ptaAddPt(ptad, xs, ys);

    return ptad;
}


/*!
 *  ptaSort()
 *
 *      Input:  ptas
 *              sorttype (L_SORT_BY_X, L_SORT_BY_Y)
 *              sortorder  (L_SORT_INCREASING, L_SORT_DECREASING)
 *              &naindex (<optional return> index of sorted order into
 *                        original array)
 *      Return: ptad (sorted version of ptas), or null on error
 */
PTA *
ptaSort(PTA     *ptas,
        l_int32  sorttype,
        l_int32  sortorder,
        NUMA   **pnaindex)
{
l_int32    i, index, n;
l_float32  x, y;
PTA       *ptad;
NUMA      *na, *naindex;

    PROCNAME("ptaSort");

    if (!ptas)
        return (PTA *)ERROR_PTR("ptas not defined", procName, NULL);
    if (sorttype != L_SORT_BY_X && sorttype != L_SORT_BY_Y)
        return (PTA *)ERROR_PTR("invalid sort type", procName, NULL);
    if (sortorder != L_SORT_INCREASING && sortorder != L_SORT_DECREASING)
        return (PTA *)ERROR_PTR("invalid sort order", procName, NULL);

        /* Build up numa of specific data */
    n = ptaGetCount(ptas);
    if ((na = numaCreate(n)) == NULL)
        return (PTA *)ERROR_PTR("na not made", procName, NULL);
    for (i = 0; i < n; i++) {
        ptaGetPt(ptas, i, &x, &y);
        if (sorttype == L_SORT_BY_X)
            numaAddNumber(na, x);
        else
            numaAddNumber(na, y);
    }

        /* Get the sort index for data array */
    if ((naindex = numaGetSortIndex(na, sortorder)) == NULL)
        return (PTA *)ERROR_PTR("naindex not made", procName, NULL);

        /* Build up sorted pta using sort index */
    if ((ptad = ptaCreate(n)) == NULL)
        return (PTA *)ERROR_PTR("ptad not made", procName, NULL);
    for (i = 0; i < n; i++) {
        numaGetIValue(naindex, i, &index);
        ptaGetPt(ptas, index, &x, &y);
        ptaAddPt(ptad, x, y);
    }

    if (pnaindex)
        *pnaindex = naindex;
    else
        numaDestroy(&naindex);
    numaDestroy(&na);
    return ptad;
}


/*!
 *  ptaRemoveDuplicates()
 *
 *      Input:  ptas (assumed to be integer values)
 *              factor (should be larger than the largest point value;
 *                      use 0 for default)
 *      Return: ptad (with duplicates removed), or null on error
 */
PTA *
ptaRemoveDuplicates(PTA      *ptas,
                    l_uint32  factor)
{
l_int32    nsize, i, j, k, index, n, nvals;
l_int32    x, y, xk, yk;
l_int32   *ia;
PTA       *ptad;
NUMA      *na;
NUMAHASH  *nahash;

    PROCNAME("ptaRemoveDuplicates");

    if (!ptas)
        return (PTA *)ERROR_PTR("ptas not defined", procName, NULL);
    if (factor == 0)
        factor = DEFAULT_SPREADING_FACTOR;

        /* Build up numaHash of indices, hashed by a key that is
         * a large linear combination of x and y values designed to
         * randomize the key. */
    nsize = 5507;  /* buckets in hash table; prime */
    nahash = numaHashCreate(nsize, 2);
    n = ptaGetCount(ptas);
    for (i = 0; i < n; i++) {
        ptaGetIPt(ptas, i, &x, &y);
        numaHashAdd(nahash, factor * x + y, (l_float32)i);
    }

    if ((ptad = ptaCreate(n)) == NULL)
        return (PTA *)ERROR_PTR("ptad not made", procName, NULL);
    for (i = 0; i < nsize; i++) {
        na = numaHashGetNuma(nahash, i);
        if (!na) continue;

        nvals = numaGetCount(na);
            /* If more than 1 pt, compare exhaustively with double loop;
             * otherwise, just enter it. */
        if (nvals > 1) {
            if ((ia = (l_int32 *)CALLOC(nvals, sizeof(l_int32))) == NULL)
                return (PTA *)ERROR_PTR("ia not made", procName, NULL);
            for (j = 0; j < nvals; j++) {
                if (ia[j] == 1) continue;
                numaGetIValue(na, j, &index);
                ptaGetIPt(ptas, index, &x, &y);
                ptaAddPt(ptad, x, y);
                for (k = j + 1; k < nvals; k++) {
                    if (ia[k] == 1) continue;
                    numaGetIValue(na, k, &index);
                    ptaGetIPt(ptas, index, &xk, &yk);
                    if (x == xk && y == yk)  /* duplicate */
                        ia[k] = 1;
                }
            }
            FREE(ia);
        }
        else {
            numaGetIValue(na, 0, &index);
            ptaGetIPt(ptas, index, &x, &y);
            ptaAddPt(ptad, x, y);
        }
        numaDestroy(&na);  /* the clone */
    }

    numaHashDestroy(&nahash);
    return ptad;
}


/*!
 *  ptaaSortByIndex()
 *
 *      Input:  ptaas
 *              naindex (na that maps from the new ptaa to the input ptaa)
 *      Return: ptaad (sorted), or null on error
 */
PTAA *
ptaaSortByIndex(PTAA  *ptaas,
                NUMA  *naindex)
{
l_int32  i, n, index;
PTA     *pta;
PTAA    *ptaad;

    PROCNAME("ptaaSortByIndex");

    if (!ptaas)
        return (PTAA *)ERROR_PTR("ptaas not defined", procName, NULL);
    if (!naindex)
        return (PTAA *)ERROR_PTR("naindex not defined", procName, NULL);

    n = ptaaGetCount(ptaas);
    if (numaGetCount(naindex) != n)
        return (PTAA *)ERROR_PTR("numa and ptaa sizes differ", procName, NULL);
    ptaad = ptaaCreate(n);
    for (i = 0; i < n; i++) {
        numaGetIValue(naindex, i, &index);
        pta = ptaaGetPta(ptaas, index, L_COPY);
        ptaaAddPta(ptaad, pta, L_INSERT);
    }

    return ptaad;
}



/*---------------------------------------------------------------------*
 *                               Geometric                             *
 *---------------------------------------------------------------------*/
/*!
 *  ptaGetBoundingRegion()
 *
 *      Input:  pta
 *      Return: box, or null on error
 *
 *  Notes:
 *      (1) This is used when the pta represents a set of points in
 *          a two-dimensional image.  It returns the box of minimum
 *          size containing the pts in the pta.
 */
BOX *
ptaGetBoundingRegion(PTA  *pta)
{
l_int32  n, i, x, y, minx, maxx, miny, maxy;

    PROCNAME("ptaGetBoundingRegion");

    if (!pta)
        return (BOX *)ERROR_PTR("pta not defined", procName, NULL);

    minx = 10000000;
    miny = 10000000;
    maxx = -10000000;
    maxy = -10000000;
    n = ptaGetCount(pta);
    for (i = 0; i < n; i++) {
        ptaGetIPt(pta, i, &x, &y);
        if (x < minx) minx = x;
        if (x > maxx) maxx = x;
        if (y < miny) miny = y;
        if (y > maxy) maxy = y;
    }

    return boxCreate(minx, miny, maxx - minx + 1, maxy - miny + 1);
}


/*!
 *  ptaGetRange()
 *
 *      Input:  pta
 *              &minx (<optional return> min value of x)
 *              &maxx (<optional return> max value of x)
 *              &miny (<optional return> min value of y)
 *              &maxy (<optional return> max value of y)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) We can use pts to represent pairs of floating values, that
 *          are not necessarily tied to a two-dimension region.  For
 *          example, the pts can represent a general function y(x).
 */
l_int32
ptaGetRange(PTA        *pta,
            l_float32  *pminx,
            l_float32  *pmaxx,
            l_float32  *pminy,
            l_float32  *pmaxy)
{
l_int32    n, i;
l_float32  x, y, minx, maxx, miny, maxy;

    PROCNAME("ptaGetRange");

    if (!pminx && !pmaxx && !pminy && !pmaxy)
        return ERROR_INT("no output requested", procName, 1);
    if (pminx) *pminx = 0;
    if (pmaxx) *pmaxx = 0;
    if (pminy) *pminy = 0;
    if (pmaxy) *pmaxy = 0;
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    if ((n = ptaGetCount(pta)) == 0)
        return ERROR_INT("no points in pta", procName, 1);

    ptaGetPt(pta, 0, &x, &y);
    minx = x;
    maxx = x;
    miny = y;
    maxy = y;
    for (i = 1; i < n; i++) {
        ptaGetPt(pta, i, &x, &y);
        if (x < minx) minx = x;
        if (x > maxx) maxx = x;
        if (y < miny) miny = y;
        if (y > maxy) maxy = y;
    }
    if (pminx) *pminx = minx;
    if (pmaxx) *pmaxx = maxx;
    if (pminy) *pminy = miny;
    if (pmaxy) *pmaxy = maxy;
    return 0;
}


/*!
 *  ptaGetInsideBox()
 *
 *      Input:  ptas (input pts)
 *              box
 *      Return: ptad (of pts in ptas that are inside the box), or null on error
 */
PTA *
ptaGetInsideBox(PTA  *ptas,
                BOX  *box)
{
PTA       *ptad;
l_int32    n, i, contains;
l_float32  x, y;

    PROCNAME("ptaGetInsideBox");

    if (!ptas)
        return (PTA *)ERROR_PTR("ptas not defined", procName, NULL);
    if (!box)
        return (PTA *)ERROR_PTR("box not defined", procName, NULL);

    n = ptaGetCount(ptas);
    ptad = ptaCreate(0);
    for (i = 0; i < n; i++) {
        ptaGetPt(ptas, i, &x, &y);
        boxContainsPt(box, x, y, &contains);
        if (contains)
            ptaAddPt(ptad, x, y);
    }

    return ptad;
}


/*!
 *  pixFindCornerPixels()
 *
 *      Input:  pixs (1 bpp)
 *      Return: pta, or null on error
 *
 *  Notes:
 *      (1) Finds the 4 corner-most pixels, as defined by a search
 *          inward from each corner, using a 45 degree line.
 */
PTA *
pixFindCornerPixels(PIX  *pixs)
{
l_int32    i, j, x, y, w, h, wpl, mindim, found;
l_uint32  *data, *line;
PTA       *pta;

    PROCNAME("pixFindCornerPixels");

    if (!pixs)
        return (PTA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PTA *)ERROR_PTR("pixs not 1 bpp", procName, NULL);

    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);
    mindim = L_MIN(w, h);
    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);

    if ((pta = ptaCreate(4)) == NULL)
        return (PTA *)ERROR_PTR("pta not made", procName, NULL);

    for (found = FALSE, i = 0; i < mindim; i++) {
        for (j = 0; j <= i; j++) {
            y = i - j;
            line = data + y * wpl;
            if (GET_DATA_BIT(line, j)) {
                ptaAddPt(pta, j, y);
                found = TRUE;
                break;
            }
        }
        if (found == TRUE)
            break;
    }

    for (found = FALSE, i = 0; i < mindim; i++) {
        for (j = 0; j <= i; j++) {
            y = i - j;
            line = data + y * wpl;
            x = w - 1 - j;
            if (GET_DATA_BIT(line, x)) {
                ptaAddPt(pta, x, y);
                found = TRUE;
                break;
            }
        }
        if (found == TRUE)
            break;
    }

    for (found = FALSE, i = 0; i < mindim; i++) {
        for (j = 0; j <= i; j++) {
            y = h - 1 - i + j;
            line = data + y * wpl;
            if (GET_DATA_BIT(line, j)) {
                ptaAddPt(pta, j, y);
                found = TRUE;
                break;
            }
        }
        if (found == TRUE)
            break;
    }

    for (found = FALSE, i = 0; i < mindim; i++) {
        for (j = 0; j <= i; j++) {
            y = h - 1 - i + j;
            line = data + y * wpl;
            x = w - 1 - j;
            if (GET_DATA_BIT(line, x)) {
                ptaAddPt(pta, x, y);
                found = TRUE;
                break;
            }
        }
        if (found == TRUE)
            break;
    }

    return pta;
}


/*!
 *  ptaContainsPt()
 *
 *      Input:  pta
 *              x, y  (point)
 *      Return: 1 if contained, 0 otherwise or on error
 */
l_int32
ptaContainsPt(PTA     *pta,
              l_int32  x,
              l_int32  y)
{
l_int32  i, n, ix, iy;

    PROCNAME("ptaContainsPt");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 0);

    n = ptaGetCount(pta);
    for (i = 0; i < n; i++) {
        ptaGetIPt(pta, i, &ix, &iy);
        if (x == ix && y == iy)
            return 1;
    }
    return 0;
}


/*!
 *  ptaTestIntersection()
 *
 *      Input:  pta1, pta2
 *      Return: bval which is 1 if they have any elements in common;
 *              0 otherwise or on error.
 */
l_int32
ptaTestIntersection(PTA  *pta1,
                    PTA  *pta2)
{
l_int32  i, j, n1, n2, x1, y1, x2, y2;

    PROCNAME("ptaTestIntersection");

    if (!pta1)
        return ERROR_INT("pta1 not defined", procName, 0);
    if (!pta2)
        return ERROR_INT("pta2 not defined", procName, 0);

    n1 = ptaGetCount(pta1);
    n2 = ptaGetCount(pta2);
    for (i = 0; i < n1; i++) {
        ptaGetIPt(pta1, i, &x1, &y1);
        for (j = 0; j < n2; j++) {
            ptaGetIPt(pta2, i, &x2, &y2);
            if (x1 == x2 && y1 == y2)
                return 1;
        }
    }

    return 0;
}


/*!
 *  ptaTransform()
 *
 *      Input:  pta
 *              shiftx, shifty
 *              scalex, scaley
 *      Return: pta, or null on error
 *
 *  Notes:
 *      (1) Shift first, then scale.
 */
PTA *
ptaTransform(PTA       *ptas,
             l_int32    shiftx,
             l_int32    shifty,
             l_float32  scalex,
             l_float32  scaley)
{
l_int32  n, i, x, y;
PTA     *ptad;

    PROCNAME("ptaTransform");

    if (!ptas)
        return (PTA *)ERROR_PTR("ptas not defined", procName, NULL);
    n = ptaGetCount(ptas);
    ptad = ptaCreate(n);
    for (i = 0; i < n; i++) {
        ptaGetIPt(ptas, i, &x, &y);
        x = (l_int32)(scalex * (x + shiftx) + 0.5);
        y = (l_int32)(scaley * (y + shifty) + 0.5);
        ptaAddPt(ptad, x, y);
    }

    return ptad;
}



/*---------------------------------------------------------------------*
 *                            Least Squares Fit                        *
 *---------------------------------------------------------------------*/
/*!
 *  ptaGetLinearLSF()
 *
 *      Input:  pta
 *              &a  (<optional return> slope a of least square fit: y = ax + b)
 *              &b  (<optional return> intercept b of least square fit)
 *              &nafit (<optional return> numa of least square fit)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) At least one of: &a and &b must not be null.
 *      (2) If both &a and &b are defined, this returns a and b that minimize:
 *
 *              sum (yi - axi -b)^2
 *               i
 *
 *          The method is simple: differentiate this expression w/rt a and b,
 *          and solve the resulting two equations for a and b in terms of
 *          various sums over the input data (xi, yi).
 *      (3) We also allow two special cases, where either a = 0 or b = 0:
 *           (a) If &a is given and &b = null, find the linear LSF that
 *               goes through the origin (b = 0).
 *           (b) If &b is given and &a = null, find the linear LSF with
 *               zero slope (a = 0).
 *      (4) If @nafit is defined, this returns an array of fitted values,
 *          corresponding to the two implicit Numa arrays (nax and nay) in pta.
 *          Thus, just as you can plot the data in pta as nay vs. nax,
 *          you can plot the linear least square fit as nafit vs. nax.
 */
l_int32
ptaGetLinearLSF(PTA        *pta,
                l_float32  *pa,
                l_float32  *pb,
                NUMA      **pnafit)
{
l_int32     n, i;
l_float32   factor, sx, sy, sxx, sxy, val;
l_float32  *xa, *ya;

    PROCNAME("ptaGetLinearLSF");

    if (!pa && !pb)
        return ERROR_INT("&a and/or &b not defined", procName, 1);
    if (pa) *pa = 0.0;
    if (pb) *pb = 0.0;
    if (pnafit) *pnafit = NULL;
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

    if ((n = ptaGetCount(pta)) < 2)
        return ERROR_INT("less than 2 pts not found", procName, 1);
    xa = pta->x;  /* not a copy */
    ya = pta->y;  /* not a copy */

    sx = sy = sxx = sxy = 0.;
    if (pa && pb) {
        for (i = 0; i < n; i++) {
            sx += xa[i];
            sy += ya[i];
            sxx += xa[i] * xa[i];
            sxy += xa[i] * ya[i];
        }
        factor = n * sxx - sx * sx;
        if (factor == 0.0)
            return ERROR_INT("no solution found", procName, 1);
        factor = 1. / factor;

        *pa = factor * ((l_float32)n * sxy - sx * sy);
        *pb = factor * (sxx * sy - sx * sxy);
    }
    else if (pa) {  /* line through origin */
        for (i = 0; i < n; i++) {
            sxx += xa[i] * xa[i];
            sxy += xa[i] * ya[i];
        }
        if (sxx == 0.0)
            return ERROR_INT("no solution found", procName, 1);
        *pa = sxy / sxx;
    }
    else {  /* a = 0; horizontal line */
        for (i = 0; i < n; i++)
            sy += ya[i];
        *pb = sy / (l_float32)n;
    }

    if (pnafit) {
        *pnafit = numaCreate(n);
        for (i = 0; i < n; i++) {
            val = (*pa) * xa[i] + *pb;
            numaAddNumber(*pnafit, val);
        }
    }

    return 0;
}


/*!
 *  ptaGetQuadraticLSF()
 *
 *      Input:  pta
 *              &a  (<optional return> coeff a of LSF: y = ax^2 + bx + c)
 *              &b  (<optional return> coeff b of LSF: y = ax^2 + bx + c)
 *              &c  (<optional return> coeff c of LSF: y = ax^2 + bx + c)
 *              &nafit (<optional return> numa of least square fit)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This does a quadratic least square fit to the set of points
 *          in @pta.  That is, it finds coefficients a, b and c that minimize:
 *
 *              sum (yi - a*xi*xi -b*xi -c)^2
 *               i
 *
 *          The method is simple: differentiate this expression w/rt
 *          a, b and c, and solve the resulting three equations for these
 *          coefficients in terms of various sums over the input data (xi, yi).
 *          The three equations are in the form:
 *             f[0][0]a + f[0][1]b + f[0][2]c = g[0]
 *             f[1][0]a + f[1][1]b + f[1][2]c = g[1]
 *             f[2][0]a + f[2][1]b + f[2][2]c = g[2]
 *      (2) If @nafit is defined, this returns an array of fitted values,
 *          corresponding to the two implicit Numa arrays (nax and nay) in pta.
 *          Thus, just as you can plot the data in pta as nay vs. nax,
 *          you can plot the linear least square fit as nafit vs. nax.
 */
l_int32
ptaGetQuadraticLSF(PTA        *pta,
                   l_float32  *pa,
                   l_float32  *pb,
                   l_float32  *pc,
                   NUMA      **pnafit)
{
l_int32     n, i, ret;
l_float32   x, y, sx, sy, sx2, sx3, sx4, sxy, sx2y;
l_float32  *xa, *ya;
l_float32  *f[3];
l_float32   g[3];
NUMA       *nafit;

    PROCNAME("ptaGetQuadraticLSF");

    if (!pa && !pb && !pc && !pnafit)
        return ERROR_INT("no output requested", procName, 1);
    if (pa) *pa = 0.0;
    if (pb) *pb = 0.0;
    if (pc) *pc = 0.0;
    if (pnafit) *pnafit = NULL;
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

    if ((n = ptaGetCount(pta)) < 3)
        return ERROR_INT("less than 3 pts not found", procName, 1);
    xa = pta->x;  /* not a copy */
    ya = pta->y;  /* not a copy */

    sx = sy = sx2 = sx3 = sx4 = sxy = sx2y = 0.;
    for (i = 0; i < n; i++) {
        x = xa[i];
        y = ya[i];
        sx += x;
        sy += y;
        sx2 += x * x;
        sx3 += x * x * x;
        sx4 += x * x * x * x;
        sxy += x * y;
        sx2y += x * x * y;
    }

    for (i = 0; i < 3; i++)
        f[i] = (l_float32 *)CALLOC(3, sizeof(l_float32));
    f[0][0] = sx4;
    f[0][1] = sx3;
    f[0][2] = sx2;
    f[1][0] = sx3;
    f[1][1] = sx2;
    f[1][2] = sx;
    f[2][0] = sx2;
    f[2][1] = sx;
    f[2][2] = n;
    g[0] = sx2y;
    g[1] = sxy;
    g[2] = sy;

        /* Solve for the unknowns, also putting f-inverse into f */
    ret = gaussjordan(f, g, 3);
    for (i = 0; i < 3; i++)
        FREE(f[i]);
    if (ret)
        return ERROR_INT("quadratic solution failed", procName, 1);

    if (pa) *pa = g[0];
    if (pb) *pb = g[1];
    if (pc) *pc = g[2];
    if (pnafit) {
        nafit = numaCreate(n);
        *pnafit = nafit;
        for (i = 0; i < n; i++) {
            x = xa[i];
            y = g[0] * x * x + g[1] * x + g[2];
            numaAddNumber(nafit, y);
        }
    }

    return 0;
}


/*!
 *  ptaGetCubicLSF()
 *
 *      Input:  pta
 *              &a  (<optional return> coeff a of LSF: y = ax^3 + bx^2 + cx + d)
 *              &b  (<optional return> coeff b of LSF)
 *              &c  (<optional return> coeff c of LSF)
 *              &d  (<optional return> coeff d of LSF)
 *              &nafit (<optional return> numa of least square fit)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This does a cubic least square fit to the set of points
 *          in @pta.  That is, it finds coefficients a, b, c and d
 *          that minimize:
 *
 *              sum (yi - a*xi*xi*xi -b*xi*xi -c*xi - d)^2
 *               i
 *
 *          Differentiate this expression w/rt a, b, c and d, and solve
 *          the resulting four equations for these coefficients in
 *          terms of various sums over the input data (xi, yi).
 *          The four equations are in the form:
 *             f[0][0]a + f[0][1]b + f[0][2]c + f[0][3] = g[0]
 *             f[1][0]a + f[1][1]b + f[1][2]c + f[1][3] = g[1]
 *             f[2][0]a + f[2][1]b + f[2][2]c + f[2][3] = g[2]
 *             f[3][0]a + f[3][1]b + f[3][2]c + f[3][3] = g[3]
 *      (2) If @nafit is defined, this returns an array of fitted values,
 *          corresponding to the two implicit Numa arrays (nax and nay) in pta.
 *          Thus, just as you can plot the data in pta as nay vs. nax,
 *          you can plot the linear least square fit as nafit vs. nax.
 */
l_int32
ptaGetCubicLSF(PTA        *pta,
               l_float32  *pa,
               l_float32  *pb,
               l_float32  *pc,
               l_float32  *pd,
               NUMA      **pnafit)
{
l_int32     n, i, ret;
l_float32   x, y, sx, sy, sx2, sx3, sx4, sx5, sx6, sxy, sx2y, sx3y;
l_float32  *xa, *ya;
l_float32  *f[4];
l_float32   g[4];
NUMA       *nafit;

    PROCNAME("ptaGetCubicLSF");

    if (!pa && !pb && !pc && !pd && !pnafit)
        return ERROR_INT("no output requested", procName, 1);
    if (pa) *pa = 0.0;
    if (pb) *pb = 0.0;
    if (pc) *pc = 0.0;
    if (pd) *pd = 0.0;
    if (pnafit) *pnafit = NULL;
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

    if ((n = ptaGetCount(pta)) < 4)
        return ERROR_INT("less than 4 pts not found", procName, 1);
    xa = pta->x;  /* not a copy */
    ya = pta->y;  /* not a copy */

    sx = sy = sx2 = sx3 = sx4 = sx5 = sx6 = sxy = sx2y = sx3y = 0.;
    for (i = 0; i < n; i++) {
        x = xa[i];
        y = ya[i];
        sx += x;
        sy += y;
        sx2 += x * x;
        sx3 += x * x * x;
        sx4 += x * x * x * x;
        sx5 += x * x * x * x * x;
        sx6 += x * x * x * x * x * x;
        sxy += x * y;
        sx2y += x * x * y;
        sx3y += x * x * x * y;
    }

    for (i = 0; i < 4; i++)
        f[i] = (l_float32 *)CALLOC(4, sizeof(l_float32));
    f[0][0] = sx6;
    f[0][1] = sx5;
    f[0][2] = sx4;
    f[0][3] = sx3;
    f[1][0] = sx5;
    f[1][1] = sx4;
    f[1][2] = sx3;
    f[1][3] = sx2;
    f[2][0] = sx4;
    f[2][1] = sx3;
    f[2][2] = sx2;
    f[2][3] = sx;
    f[3][0] = sx3;
    f[3][1] = sx2;
    f[3][2] = sx;
    f[3][3] = n;
    g[0] = sx3y;
    g[1] = sx2y;
    g[2] = sxy;
    g[3] = sy;

        /* Solve for the unknowns, also putting f-inverse into f */
    ret = gaussjordan(f, g, 4);
    for (i = 0; i < 4; i++)
        FREE(f[i]);
    if (ret)
        return ERROR_INT("cubic solution failed", procName, 1);

    if (pa) *pa = g[0];
    if (pb) *pb = g[1];
    if (pc) *pc = g[2];
    if (pd) *pd = g[3];
    if (pnafit) {
        nafit = numaCreate(n);
        *pnafit = nafit;
        for (i = 0; i < n; i++) {
            x = xa[i];
            y = g[0] * x * x * x + g[1] * x * x + g[2] * x + g[3];
            numaAddNumber(nafit, y);
        }
    }

    return 0;
}


/*!
 *  ptaGetQuarticLSF()
 *
 *      Input:  pta
 *              &a  (<optional return> coeff a of LSF:
 *                        y = ax^4 + bx^3 + cx^2 + dx + e)
 *              &b  (<optional return> coeff b of LSF)
 *              &c  (<optional return> coeff c of LSF)
 *              &d  (<optional return> coeff d of LSF)
 *              &e  (<optional return> coeff e of LSF)
 *              &nafit (<optional return> numa of least square fit)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This does a quartic least square fit to the set of points
 *          in @pta.  That is, it finds coefficients a, b, c, d and 3
 *          that minimize:
 *
 *              sum (yi - a*xi*xi*xi*xi -b*xi*xi*xi -c*xi*xi - d*xi - e)^2
 *               i
 *
 *          Differentiate this expression w/rt a, b, c, d and e, and solve
 *          the resulting five equations for these coefficients in
 *          terms of various sums over the input data (xi, yi).
 *          The five equations are in the form:
 *             f[0][0]a + f[0][1]b + f[0][2]c + f[0][3] + f[0][4] = g[0]
 *             f[1][0]a + f[1][1]b + f[1][2]c + f[1][3] + f[1][4] = g[1]
 *             f[2][0]a + f[2][1]b + f[2][2]c + f[2][3] + f[2][4] = g[2]
 *             f[3][0]a + f[3][1]b + f[3][2]c + f[3][3] + f[3][4] = g[3]
 *             f[4][0]a + f[4][1]b + f[4][2]c + f[4][3] + f[4][4] = g[4]
 *      (2) If @nafit is defined, this returns an array of fitted values,
 *          corresponding to the two implicit Numa arrays (nax and nay) in pta.
 *          Thus, just as you can plot the data in pta as nay vs. nax,
 *          you can plot the linear least square fit as nafit vs. nax.
 */
l_int32
ptaGetQuarticLSF(PTA        *pta,
                 l_float32  *pa,
                 l_float32  *pb,
                 l_float32  *pc,
                 l_float32  *pd,
                 l_float32  *pe,
                 NUMA      **pnafit)
{
l_int32     n, i, ret;
l_float32   x, y, sx, sy, sx2, sx3, sx4, sx5, sx6, sx7, sx8;
l_float32   sxy, sx2y, sx3y, sx4y;
l_float32  *xa, *ya;
l_float32  *f[5];
l_float32   g[5];
NUMA       *nafit;

    PROCNAME("ptaGetQuarticLSF");

    if (!pa && !pb && !pc && !pd && !pe && !pnafit)
        return ERROR_INT("no output requested", procName, 1);
    if (pa) *pa = 0.0;
    if (pb) *pb = 0.0;
    if (pc) *pc = 0.0;
    if (pd) *pd = 0.0;
    if (pe) *pe = 0.0;
    if (pnafit) *pnafit = NULL;
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

    if ((n = ptaGetCount(pta)) < 5)
        return ERROR_INT("less than 5 pts not found", procName, 1);
    xa = pta->x;  /* not a copy */
    ya = pta->y;  /* not a copy */

    sx = sy = sx2 = sx3 = sx4 = sx5 = sx6 = sx7 = sx8 = 0;
    sxy = sx2y = sx3y = sx4y = 0.;
    for (i = 0; i < n; i++) {
        x = xa[i];
        y = ya[i];
        sx += x;
        sy += y;
        sx2 += x * x;
        sx3 += x * x * x;
        sx4 += x * x * x * x;
        sx5 += x * x * x * x * x;
        sx6 += x * x * x * x * x * x;
        sx7 += x * x * x * x * x * x * x;
        sx8 += x * x * x * x * x * x * x * x;
        sxy += x * y;
        sx2y += x * x * y;
        sx3y += x * x * x * y;
        sx4y += x * x * x * x * y;
    }

    for (i = 0; i < 5; i++)
        f[i] = (l_float32 *)CALLOC(5, sizeof(l_float32));
    f[0][0] = sx8;
    f[0][1] = sx7;
    f[0][2] = sx6;
    f[0][3] = sx5;
    f[0][4] = sx4;
    f[1][0] = sx7;
    f[1][1] = sx6;
    f[1][2] = sx5;
    f[1][3] = sx4;
    f[1][4] = sx3;
    f[2][0] = sx6;
    f[2][1] = sx5;
    f[2][2] = sx4;
    f[2][3] = sx3;
    f[2][4] = sx2;
    f[3][0] = sx5;
    f[3][1] = sx4;
    f[3][2] = sx3;
    f[3][3] = sx2;
    f[3][4] = sx;
    f[4][0] = sx4;
    f[4][1] = sx3;
    f[4][2] = sx2;
    f[4][3] = sx;
    f[4][4] = n;
    g[0] = sx4y;
    g[1] = sx3y;
    g[2] = sx2y;
    g[3] = sxy;
    g[4] = sy;

        /* Solve for the unknowns, also putting f-inverse into f */
    ret = gaussjordan(f, g, 5);
    for (i = 0; i < 5; i++)
        FREE(f[i]);
    if (ret)
        return ERROR_INT("quartic solution failed", procName, 1);

    if (pa) *pa = g[0];
    if (pb) *pb = g[1];
    if (pc) *pc = g[2];
    if (pd) *pd = g[3];
    if (pe) *pe = g[4];
    if (pnafit) {
        nafit = numaCreate(n);
        *pnafit = nafit;
        for (i = 0; i < n; i++) {
            x = xa[i];
            y = g[0] * x * x * x * x + g[1] * x * x * x + g[2] * x * x
                 + g[3] * x + g[4];
            numaAddNumber(nafit, y);
        }
    }

    return 0;
}


/*!
 *  applyLinearFit()
 *
 *      Input: a, b (linear fit coefficients)
 *             x
 *             &y (<return> y = a * x + b)
 *      Return: 0 if OK, 1 on error
 */
l_int32
applyLinearFit(l_float32   a,
                  l_float32   b,
                  l_float32   x,
                  l_float32  *py)
{
    PROCNAME("applyLinearFit");

    if (!py)
        return ERROR_INT("&y not defined", procName, 1);

    *py = a * x + b;
    return 0;
}


/*!
 *  applyQuadraticFit()
 *
 *      Input: a, b, c (quadratic fit coefficients)
 *             x
 *             &y (<return> y = a * x^2 + b * x + c)
 *      Return: 0 if OK, 1 on error
 */
l_int32
applyQuadraticFit(l_float32   a,
                  l_float32   b,
                  l_float32   c,
                  l_float32   x,
                  l_float32  *py)
{
    PROCNAME("applyQuadraticFit");

    if (!py)
        return ERROR_INT("&y not defined", procName, 1);

    *py = a * x * x + b * x + c;
    return 0;
}


/*!
 *  applyCubicFit()
 *
 *      Input: a, b, c, d (cubic fit coefficients)
 *             x
 *             &y (<return> y = a * x^3 + b * x^2  + c * x + d)
 *      Return: 0 if OK, 1 on error
 */
l_int32
applyCubicFit(l_float32   a,
              l_float32   b,
              l_float32   c,
              l_float32   d,
              l_float32   x,
              l_float32  *py)
{
    PROCNAME("applyCubicFit");

    if (!py)
        return ERROR_INT("&y not defined", procName, 1);

    *py = a * x * x * x + b * x * x + c * x + d;
    return 0;
}


/*!
 *  applyQuarticFit()
 *
 *      Input: a, b, c, d, e (quartic fit coefficients)
 *             x
 *             &y (<return> y = a * x^4 + b * x^3  + c * x^2 + d * x + e)
 *      Return: 0 if OK, 1 on error
 */
l_int32
applyQuarticFit(l_float32   a,
                l_float32   b,
                l_float32   c,
                l_float32   d,
                l_float32   e,
                l_float32   x,
                l_float32  *py)
{
l_float32  x2;

    PROCNAME("applyQuarticFit");

    if (!py)
        return ERROR_INT("&y not defined", procName, 1);

    x2 = x * x;
    *py = a * x2 * x2 + b * x2 * x + c * x2 + d * x + e;
    return 0;
}


/*---------------------------------------------------------------------*
 *                        Interconversions with Pix                    *
 *---------------------------------------------------------------------*/
/*!
 *  pixPlotAlongPta()
 *
 *      Input: pixs (any depth)
 *             pta (set of points on which to plot)
 *             outformat (GPLOT_PNG, GPLOT_PS, GPLOT_EPS, GPLOT_X11,
 *                        GPLOT_LATEX)
 *             title (<optional> for plot; can be null)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) We remove any existing colormap and clip the pta to the input pixs.
 *      (2) This is a debugging function, and does not remove temporary
 *          plotting files that it generates.
 *      (3) If the image is RGB, three separate plots are generated.
 */
l_int32
pixPlotAlongPta(PIX         *pixs,
                PTA         *pta,
                l_int32      outformat,
                const char  *title)
{
char            buffer[128];
char           *rtitle, *gtitle, *btitle;
static l_int32  count = 0;  /* require separate temp files for each call */
l_int32         i, x, y, d, w, h, npts, rval, gval, bval;
l_uint32        val;
NUMA           *na, *nar, *nag, *nab;
PIX            *pixt;

    PROCNAME("pixPlotAlongPta");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    if (outformat != GPLOT_PNG && outformat != GPLOT_PS &&
        outformat != GPLOT_EPS && outformat != GPLOT_X11 &&
        outformat != GPLOT_LATEX) {
        L_WARNING("outformat invalid; using GPLOT_PNG", procName);
        outformat = GPLOT_PNG;
    }

    pixt = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    d = pixGetDepth(pixt);
    w = pixGetWidth(pixt);
    h = pixGetHeight(pixt);
    npts = ptaGetCount(pta);
    if (d == 32) {
        nar = numaCreate(npts);
        nag = numaCreate(npts);
        nab = numaCreate(npts);
        for (i = 0; i < npts; i++) {
            ptaGetIPt(pta, i, &x, &y);
            if (x < 0 || x >= w)
                continue;
            if (y < 0 || y >= h)
                continue;
            pixGetPixel(pixt, x, y, &val);
            rval = GET_DATA_BYTE(&val, COLOR_RED);
            gval = GET_DATA_BYTE(&val, COLOR_GREEN);
            bval = GET_DATA_BYTE(&val, COLOR_BLUE);
            numaAddNumber(nar, rval);
            numaAddNumber(nag, gval);
            numaAddNumber(nab, bval);
        }

        sprintf(buffer, "junkplot.%d", count++);
        rtitle = stringJoin("Red: ", title);
        gplotSimple1(nar, outformat, buffer, rtitle);
        sprintf(buffer, "junkplot.%d", count++);
        gtitle = stringJoin("Green: ", title);
        gplotSimple1(nag, outformat, buffer, gtitle);
        sprintf(buffer, "junkplot.%d", count++);
        btitle = stringJoin("Blue: ", title);
        gplotSimple1(nab, outformat, buffer, btitle);
        numaDestroy(&nar);
        numaDestroy(&nag);
        numaDestroy(&nab);
        FREE(rtitle);
        FREE(gtitle);
        FREE(btitle);
    }
    else {
        na = numaCreate(npts);
        for (i = 0; i < npts; i++) {
            ptaGetIPt(pta, i, &x, &y);
            if (x < 0 || x >= w)
                continue;
            if (y < 0 || y >= h)
                continue;
            pixGetPixel(pixt, x, y, &val);
            numaAddNumber(na, (l_float32)val);
        }

        sprintf(buffer, "junkplot.%d", count++);
        gplotSimple1(na, outformat, buffer, title);
        numaDestroy(&na);
    }
    pixDestroy(&pixt);
    return 0;
}


/*!
 *  ptaGetPixelsFromPix()
 *
 *      Input:  pixs (1 bpp)
 *              box (<optional> can be null)
 *      Return: pta, or null on error
 *
 *  Notes:
 *      (1) Generates a pta of fg pixels in the pix, within the box.
 *          If box == NULL, it uses the entire pix.
 */
PTA *
ptaGetPixelsFromPix(PIX  *pixs,
                    BOX  *box)
{
l_int32    i, j, w, h, wpl, xstart, xend, ystart, yend, bw, bh;
l_uint32  *data, *line;
PTA       *pta;

    PROCNAME("ptaGetPixelsFromPix");

    if (!pixs || (pixGetDepth(pixs) != 1))
        return (PTA *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    xstart = ystart = 0;
    xend = w - 1;
    yend = h - 1;
    if (box) {
        boxGetGeometry(box, &xstart, &ystart, &bw, &bh);
        xend = xstart + bw - 1;
        yend = ystart + bh - 1;
    }

    if ((pta = ptaCreate(0)) == NULL)
        return (PTA *)ERROR_PTR("pta not made", procName, NULL);
    for (i = ystart; i <= yend; i++) {
        line = data + i * wpl;
        for (j = xstart; j <= xend; j++) {
            if (GET_DATA_BIT(line, j))
                ptaAddPt(pta, j, i);
        }
    }

    return pta;
}


/*!
 *  pixGenerateFromPta()
 *
 *      Input:  pta
 *              w, h (of pix)
 *      Return: pix (1 bpp), or null on error
 *
 *  Notes:
 *      (1) Points are rounded to nearest ints.
 *      (2) Any points outside (w,h) are silently discarded.
 *      (3) Output 1 bpp pix has values 1 for each point in the pta.
 */
PIX *
pixGenerateFromPta(PTA     *pta,
                   l_int32  w,
                   l_int32  h)
{
l_int32  n, i, x, y;
PIX     *pix;

    PROCNAME("pixGenerateFromPta");

    if (!pta)
        return (PIX *)ERROR_PTR("pta not defined", procName, NULL);

    if ((pix = pixCreate(w, h, 1)) == NULL)
        return (PIX *)ERROR_PTR("pix not made", procName, NULL);
    n = ptaGetCount(pta);
    for (i = 0; i < n; i++) {
        ptaGetIPt(pta, i, &x, &y);
        if (x < 0 || x >= w || y < 0 || y >= h)
            continue;
        pixSetPixel(pix, x, y, 1);
    }

    return pix;
}


/*!
 *  ptaGetBoundaryPixels()
 *
 *      Input:  pixs (1 bpp)
 *              type (L_BOUNDARY_FG, L_BOUNDARY_BG)
 *      Return: pta, or null on error
 *
 *  Notes:
 *      (1) This generates a pta of either fg or bg boundary pixels.
 */
PTA *
ptaGetBoundaryPixels(PIX     *pixs,
                     l_int32  type)
{
PIX  *pixt;
PTA  *pta;

    PROCNAME("ptaGetBoundaryPixels");

    if (!pixs || (pixGetDepth(pixs) != 1))
        return (PTA *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (type != L_BOUNDARY_FG && type != L_BOUNDARY_BG)
        return (PTA *)ERROR_PTR("invalid type", procName, NULL);

    if (type == L_BOUNDARY_FG)
        pixt = pixMorphSequence(pixs, "e3.3", 0);
    else
        pixt = pixMorphSequence(pixs, "d3.3", 0);
    pixXor(pixt, pixt, pixs);
    pta = ptaGetPixelsFromPix(pixt, NULL);

    pixDestroy(&pixt);
    return pta;
}


/*!
 *  ptaaGetBoundaryPixels()
 *
 *      Input:  pixs (1 bpp)
 *              type (L_BOUNDARY_FG, L_BOUNDARY_BG)
 *              connectivity (4 or 8)
 *              &boxa (<optional return> bounding boxes of the c.c.)
 *              &pixa (<optional return> pixa of the c.c.)
 *      Return: ptaa, or null on error
 *
 *  Notes:
 *      (1) This generates a ptaa of either fg or bg boundary pixels,
 *          where each pta has the boundary pixels for a connected
 *          component.
 *      (2) We can't simply find all the boundary pixels and then select
 *          those within the bounding box of each component, because
 *          bounding boxes can overlap.  It is necessary to extract and
 *          dilate or erode each component separately.  Note also that
 *          special handling is required for bg pixels when the
 *          component touches the pix boundary.
 */
PTAA *
ptaaGetBoundaryPixels(PIX     *pixs,
                      l_int32  type,
                      l_int32  connectivity,
                      BOXA   **pboxa,
                      PIXA   **ppixa)
{
l_int32  i, n, w, h, x, y, bw, bh, left, right, top, bot;
BOXA    *boxa;
PIX     *pixt1, *pixt2;
PIXA    *pixa;
PTA     *pta1, *pta2;
PTAA    *ptaa;

    PROCNAME("ptaaGetBoundaryPixels");

    if (pboxa) *pboxa = NULL;
    if (ppixa) *ppixa = NULL;
    if (!pixs || (pixGetDepth(pixs) != 1))
        return (PTAA *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (type != L_BOUNDARY_FG && type != L_BOUNDARY_BG)
        return (PTAA *)ERROR_PTR("invalid type", procName, NULL);
    if (connectivity != 4 && connectivity != 8)
        return (PTAA *)ERROR_PTR("connectivity not 4 or 8", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    boxa = pixConnComp(pixs, &pixa, connectivity);
    n = boxaGetCount(boxa);
    ptaa = ptaaCreate(0);
    for (i = 0; i < n; i++) {
        pixt1 = pixaGetPix(pixa, i, L_CLONE);
        boxaGetBoxGeometry(boxa, i, &x, &y, &bw, &bh);
        left = right = top = bot = 0;
        if (type == L_BOUNDARY_BG) {
            if (x > 0) left = 1;
            if (y > 0) top = 1;
            if (x + bw < w) right = 1;
            if (y + bh < h) bot = 1;
            pixt2 = pixAddBorderGeneral(pixt1, left, right, top, bot, 0);
        }
        else
            pixt2 = pixClone(pixt1);
        pta1 = ptaGetBoundaryPixels(pixt2, type);
        pta2 = ptaTransform(pta1, x - left, y - top, 1.0, 1.0);
        ptaaAddPta(ptaa, pta2, L_INSERT);
        ptaDestroy(&pta1);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }

    if (pboxa)
        *pboxa = boxa;
    else
        boxaDestroy(&boxa);
    if (ppixa)
        *ppixa = pixa;
    else
        pixaDestroy(&pixa);
    return ptaa;
}


/*---------------------------------------------------------------------*
 *                          Display Pta and Ptaa                       *
 *---------------------------------------------------------------------*/
/*!
 *  pixDisplayPta()
 *
 *      Input:  pixd (can be same as pixs or null; 32 bpp if in-place)
 *              pixs (1, 2, 4, 8, 16 or 32 bpp)
 *              pta (of path to be plotted)
 *      Return: pixd (32 bpp RGB version of pixs, with path in green),
 *              or null on error
 *
 *  Notes:
 *      (1) To write on an existing pixs, pixs must be 32 bpp and
 *          call with pixd == pixs:
 *             pixDisplayPta(pixs, pixs, pta);
 *          To write on a new pix, set pixd == NULL and call:
 *             pixd = pixDisplayPta(NULL, pixs, pta);
 */
PIX *
pixDisplayPta(PIX  *pixd,
              PIX  *pixs,
              PTA  *pta)
{
l_int32   i, n, x, y;
l_uint32  rpixel, gpixel, bpixel;

    PROCNAME("pixDisplayPta");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!pta)
        return (PIX *)ERROR_PTR("pta not defined", procName, NULL);
    if (pixd && (pixd != pixs || pixGetDepth(pixd) != 32))
        return (PIX *)ERROR_PTR("invalid pixd", procName, NULL);

    if (!pixd)
        pixd = pixConvertTo32(pixs);
    composeRGBPixel(255, 0, 0, &rpixel);  /* start point */
    composeRGBPixel(0, 255, 0, &gpixel);
    composeRGBPixel(0, 0, 255, &bpixel);  /* end point */

    n = ptaGetCount(pta);
    for (i = 0; i < n; i++) {
        ptaGetIPt(pta, i, &x, &y);
        if (i == 0)
            pixSetPixel(pixd, x, y, rpixel);
        else if (i < n - 1)
            pixSetPixel(pixd, x, y, gpixel);
        else
            pixSetPixel(pixd, x, y, bpixel);
    }

    return pixd;
}


/*!
 *  pixDisplayPtaa()
 *
 *      Input:  pixs (1, 2, 4, 8, 16 or 32 bpp)
 *              ptaa (array of paths to be plotted)
 *      Return: pixd (32 bpp RGB version of pixs, with paths plotted
 *                    in different colors), or null on error
 */
PIX *
pixDisplayPtaa(PIX   *pixs,
               PTAA  *ptaa)
{
l_int32    i, j, npta, npt, x, y, rv, gv, bv;
l_uint32  *pixela;
NUMA      *na1, *na2, *na3;
PIX       *pixd;
PTA       *pta;

    PROCNAME("pixDisplayPtaa");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!ptaa)
        return (PIX *)ERROR_PTR("ptaa not defined", procName, NULL);
    npta = ptaaGetCount(ptaa);
    if (npta == 0)
        return (PIX *)ERROR_PTR("no pta", procName, NULL);

    if ((pixd = pixConvertTo32(pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

        /* Make a colormap for the paths */
    if ((pixela = (l_uint32 *)CALLOC(npta, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("calloc fail for pixela", procName, NULL);
    na1 = numaPseudorandomSequence(256, 14657);
    na2 = numaPseudorandomSequence(256, 34631);
    na3 = numaPseudorandomSequence(256, 54617);
    for (i = 0; i < npta; i++) {
        numaGetIValue(na1, i % 256, &rv);
        numaGetIValue(na2, i % 256, &gv);
        numaGetIValue(na3, i % 256, &bv);
        composeRGBPixel(rv, gv, bv, &pixela[i]);
    }
    numaDestroy(&na1);
    numaDestroy(&na2);
    numaDestroy(&na3);

    for (i = 0; i < npta; i++) {
        pta = ptaaGetPta(ptaa, i, L_CLONE);
        npt = ptaGetCount(pta);
        for (j = 0; j < npt; j++) {
            ptaGetIPt(pta, j, &x, &y);
            pixSetPixel(pixd, x, y, pixela[i]);
        }
        ptaDestroy(&pta);
    }

    FREE(pixela);
    return pixd;
}


