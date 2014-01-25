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
 *   boxfunc2.c
 *
 *      Boxa/Box transform (shift, scale) and orthogonal rotation
 *           BOXA            *boxaTransform()
 *           BOX             *boxTransform()
 *           BOXA            *boxaTransformOrdered()
 *           BOX             *boxTransformOrdered()
 *           BOXA            *boxaRotateOrth()
 *           BOX             *boxRotateOrth()
 *
 *      Boxa sort
 *           BOXA            *boxaSort()
 *           BOXA            *boxaBinSort()
 *           BOXA            *boxaSortByIndex()
 *           BOXAA           *boxaSort2d()
 *           BOXAA           *boxaSort2dByIndex()
 *
 *      Boxa statistics
 *           BOX             *boxaGetRankSize()
 *           BOX             *boxaGetMedian()
 *
 *      Boxa array extraction
 *           l_int32          boxaExtractAsNuma()
 *           l_int32          boxaExtractAsPta()
 *
 *      Other Boxaa functions
 *           l_int32          boxaaGetExtent()
 *           BOXA            *boxaaFlattenToBoxa()
 *           BOXA            *boxaaFlattenAligned()
 *           BOXAA           *boxaEncapsulateAligned()
 *           l_int32          boxaaAlignBox()
 */

#include <math.h>
#include "allheaders.h"

    /* For more than this number of c.c. in a binarized image of
     * semi-perimeter (w + h) about 5000 or less, the O(n) binsort
     * is faster than the O(nlogn) shellsort.  */
static const l_int32   MIN_COMPS_FOR_BIN_SORT = 200;


/*---------------------------------------------------------------------*
 *      Boxa/Box transform (shift, scale) and orthogonal rotation      *
 *---------------------------------------------------------------------*/
/*!
 *  boxaTransform()
 *
 *      Input:  boxa
 *              shiftx, shifty
 *              scalex, scaley
 *      Return: boxad, or null on error
 *
 *  Notes:
 *      (1) This is a very simple function that first shifts, then scales.
 */
BOXA *
boxaTransform(BOXA      *boxas,
              l_int32    shiftx,
              l_int32    shifty,
              l_float32  scalex,
              l_float32  scaley)
{
l_int32  i, n;
BOX     *boxs, *boxd;
BOXA    *boxad;

    PROCNAME("boxaTransform");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    n = boxaGetCount(boxas);
    if ((boxad = boxaCreate(n)) == NULL)
        return (BOXA *)ERROR_PTR("boxad not made", procName, NULL);
    for (i = 0; i < n; i++) {
        if ((boxs = boxaGetBox(boxas, i, L_CLONE)) == NULL)
            return (BOXA *)ERROR_PTR("boxs not found", procName, NULL);
        boxd = boxTransform(boxs, shiftx, shifty, scalex, scaley);
        boxDestroy(&boxs);
        boxaAddBox(boxad, boxd, L_INSERT);
    }

    return boxad;
}


/*!
 *  boxTransform()
 *
 *      Input:  box
 *              shiftx, shifty
 *              scalex, scaley
 *      Return: boxd, or null on error
 *
 *  Notes:
 *      (1) This is a very simple function that first shifts, then scales.
 *      (2) If the box is invalid, a new invalid box is returned.
 */
BOX *
boxTransform(BOX       *box,
             l_int32    shiftx,
             l_int32    shifty,
             l_float32  scalex,
             l_float32  scaley)
{
    PROCNAME("boxTransform");

    if (!box)
        return (BOX *)ERROR_PTR("box not defined", procName, NULL);
    if (box->w <= 0 || box->h <= 0)
        return boxCreate(0, 0, 0, 0);
    else
        return boxCreate((l_int32)(scalex * (box->x + shiftx) + 0.5),
                         (l_int32)(scaley * (box->y + shifty) + 0.5),
                         (l_int32)(L_MAX(1.0, scalex * box->w + 0.5)),
                         (l_int32)(L_MAX(1.0, scaley * box->h + 0.5)));
}


/*!
 *  boxaTransformOrdered()
 *
 *      Input:  boxa
 *              shiftx, shifty
 *              scalex, scaley
 *              xcen, ycen (center of rotation)
 *              angle (in radians; clockwise is positive)
 *              order (one of 6 combinations: L_TR_SC_RO, ...)
 *      Return: boxd, or null on error
 *
 *  Notes:
 *      (1) This allows a sequence of linear transforms on each box.
 *          the transforms are from the affine set, composed of
 *          shift, scaling and rotation, and the order of the
 *          transforms is specified.
 *      (2) Although these operations appear to be on an infinite
 *          2D plane, in practice the region of interest is clipped
 *          to a finite image.  The center of rotation is usually taken
 *          with respect to the image (either the UL corner or the
 *          center).  A translation can have two very different effects:
 *            (a) Moves the boxes across the fixed image region.
 *            (b) Moves the image origin, causing a change in the image
 *                region and an opposite effective translation of the boxes.
 *          This function should only be used for (a), where the image
 *          region is fixed on translation.  If the image region is
 *          changed by the translation, use instead the functions
 *          in affinecompose.c, where the image region and rotation
 *          center can be computed from the actual clipping due to
 *          translation of the image origin.
 *      (3) See boxTransformOrdered() for usage and implementation details.
 */
BOXA *
boxaTransformOrdered(BOXA      *boxas,
                     l_int32    shiftx,
                     l_int32    shifty,
                     l_float32  scalex,
                     l_float32  scaley,
                     l_int32    xcen,
                     l_int32    ycen,
                     l_float32  angle,
                     l_int32    order)
{
l_int32  i, n;
BOX     *boxs, *boxd;
BOXA    *boxad;

    PROCNAME("boxaTransformOrdered");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    n = boxaGetCount(boxas);
    if ((boxad = boxaCreate(n)) == NULL)
        return (BOXA *)ERROR_PTR("boxad not made", procName, NULL);
    for (i = 0; i < n; i++) {
        if ((boxs = boxaGetBox(boxas, i, L_CLONE)) == NULL)
            return (BOXA *)ERROR_PTR("boxs not found", procName, NULL);
        boxd = boxTransformOrdered(boxs, shiftx, shifty, scalex, scaley,
                                   xcen, ycen, angle, order);
        boxDestroy(&boxs);
        boxaAddBox(boxad, boxd, L_INSERT);
    }

    return boxad;
}


/*!
 *  boxTransformOrdered()
 *
 *      Input:  boxs
 *              shiftx, shifty
 *              scalex, scaley
 *              xcen, ycen (center of rotation)
 *              angle (in radians; clockwise is positive)
 *              order (one of 6 combinations: L_TR_SC_RO, ...)
 *      Return: boxd, or null on error
 *
 *  Notes:
 *      (1) This allows a sequence of linear transforms, composed of
 *          shift, scaling and rotation, where the order of the
 *          transforms is specified.
 *      (2) The rotation is taken about a point specified by (xcen, ycen).
 *          Let the components of the vector from the center of rotation
 *          to the box center be (xdif, ydif):
 *            xdif = (bx + 0.5 * bw) - xcen
 *            ydif = (by + 0.5 * bh) - ycen
 *          Then the box center after rotation has new components:
 *            bxcen = xcen + xdif * cosa + ydif * sina
 *            bycen = ycen + ydif * cosa - xdif * sina
 *          where cosa and sina are the cos and sin of the angle,
 *          and the enclosing box for the rotated box has size:
 *            rw = |bw * cosa| + |bh * sina|
 *            rh = |bh * cosa| + |bw * sina|
 *          where bw and bh are the unrotated width and height.
 *          Then the box UL corner (rx, ry) is
 *            rx = bxcen - 0.5 * rw
 *            ry = bycen - 0.5 * rh
 *      (3) The center of rotation specified by args @xcen and @ycen
 *          is the point BEFORE any translation or scaling.  If the
 *          rotation is not the first operation, this function finds
 *          the actual center at the time of rotation.  It does this
 *          by making the following assumptions:
 *             (1) Any scaling is with respect to the UL corner, so
 *                 that the center location scales accordingly.
 *             (2) A translation does not affect the center of
 *                 the image; it just moves the boxes.
 *          We always use assumption (1).  However, assumption (2)
 *          will be incorrect if the apparent translation is due
 *          to a clipping operation that, in effect, moves the
 *          origin of the image.  In that case, you should NOT use
 *          these simple functions.  Instead, use the functions
 *          in affinecompose.c, where the rotation center can be
 *          computed from the actual clipping due to translation
 *          of the image origin.
 */
BOX *
boxTransformOrdered(BOX       *boxs,
                    l_int32    shiftx,
                    l_int32    shifty,
                    l_float32  scalex,
                    l_float32  scaley,
                    l_int32    xcen,
                    l_int32    ycen,
                    l_float32  angle,
                    l_int32    order)
{
l_int32    bx, by, bw, bh, tx, ty, tw, th;
l_int32    xcent, ycent;  /* transformed center of rotation due to scaling */
l_float32  sina, cosa, xdif, ydif, rx, ry, rw, rh;
BOX       *boxd;

    PROCNAME("boxTransformOrdered");

    if (!boxs)
        return (BOX *)ERROR_PTR("boxs not defined", procName, NULL);
    if (order != L_TR_SC_RO && order != L_SC_RO_TR && order != L_RO_TR_SC &&
        order != L_TR_RO_SC && order != L_RO_SC_TR && order != L_SC_TR_RO)
        return (BOX *)ERROR_PTR("order invalid", procName, NULL);

    boxGetGeometry(boxs, &bx, &by, &bw, &bh);
    if (bw <= 0 || bh <= 0)  /* invalid */
        return boxCreate(0, 0, 0, 0);
    if (angle != 0.0) {
        sina = sin(angle);
        cosa = cos(angle);
    }

    if (order == L_TR_SC_RO) {
        tx = (l_int32)(scalex * (bx + shiftx) + 0.5);
        ty = (l_int32)(scaley * (by + shifty) + 0.5);
        tw = (l_int32)(L_MAX(1.0, scalex * bw + 0.5));
        th = (l_int32)(L_MAX(1.0, scaley * bh + 0.5));
        xcent = (l_int32)(scalex * xcen + 0.5);
        ycent = (l_int32)(scaley * ycen + 0.5);
        if (angle == 0.0) {
            boxd = boxCreate(tx, ty, tw, th);
        } else {
            xdif = tx + 0.5 * tw - xcent;
            ydif = ty + 0.5 * th - ycent;
            rw = L_ABS(tw * cosa) + L_ABS(th * sina);
            rh = L_ABS(th * cosa) + L_ABS(tw * sina);
            rx = xcent + xdif * cosa - ydif * sina - 0.5 * rw;
            ry = ycent + ydif * cosa + xdif * sina - 0.5 * rh;
            boxd = boxCreate((l_int32)rx, (l_int32)ry, (l_int32)rw,
                             (l_int32)rh);
        }
    } else if (order == L_SC_TR_RO) {
        tx = (l_int32)(scalex * bx + shiftx + 0.5);
        ty = (l_int32)(scaley * by + shifty + 0.5);
        tw = (l_int32)(L_MAX(1.0, scalex * bw + 0.5));
        th = (l_int32)(L_MAX(1.0, scaley * bh + 0.5));
        xcent = (l_int32)(scalex * xcen + 0.5);
        ycent = (l_int32)(scaley * ycen + 0.5);
        if (angle == 0.0) {
            boxd = boxCreate(tx, ty, tw, th);
        } else {
            xdif = tx + 0.5 * tw - xcent;
            ydif = ty + 0.5 * th - ycent;
            rw = L_ABS(tw * cosa) + L_ABS(th * sina);
            rh = L_ABS(th * cosa) + L_ABS(tw * sina);
            rx = xcent + xdif * cosa - ydif * sina - 0.5 * rw;
            ry = ycent + ydif * cosa + xdif * sina - 0.5 * rh;
            boxd = boxCreate((l_int32)rx, (l_int32)ry, (l_int32)rw,
                             (l_int32)rh);
        }
    } else if (order == L_RO_TR_SC) {
        if (angle == 0.0) {
            rx = bx;
            ry = by;
            rw = bw;
            rh = bh;
        } else {
            xdif = bx + 0.5 * bw - xcen;
            ydif = by + 0.5 * bh - ycen;
            rw = L_ABS(bw * cosa) + L_ABS(bh * sina);
            rh = L_ABS(bh * cosa) + L_ABS(bw * sina);
            rx = xcen + xdif * cosa - ydif * sina - 0.5 * rw;
            ry = ycen + ydif * cosa + xdif * sina - 0.5 * rh;
        }
        tx = (l_int32)(scalex * (rx + shiftx) + 0.5);
        ty = (l_int32)(scaley * (ry + shifty) + 0.5);
        tw = (l_int32)(L_MAX(1.0, scalex * rw + 0.5));
        th = (l_int32)(L_MAX(1.0, scaley * rh + 0.5));
        boxd = boxCreate(tx, ty, tw, th);
    } else if (order == L_RO_SC_TR) {
        if (angle == 0.0) {
            rx = bx;
            ry = by;
            rw = bw;
            rh = bh;
        } else {
            xdif = bx + 0.5 * bw - xcen;
            ydif = by + 0.5 * bh - ycen;
            rw = L_ABS(bw * cosa) + L_ABS(bh * sina);
            rh = L_ABS(bh * cosa) + L_ABS(bw * sina);
            rx = xcen + xdif * cosa - ydif * sina - 0.5 * rw;
            ry = ycen + ydif * cosa + xdif * sina - 0.5 * rh;
        }
        tx = (l_int32)(scalex * rx + shiftx + 0.5);
        ty = (l_int32)(scaley * ry + shifty + 0.5);
        tw = (l_int32)(L_MAX(1.0, scalex * rw + 0.5));
        th = (l_int32)(L_MAX(1.0, scaley * rh + 0.5));
        boxd = boxCreate(tx, ty, tw, th);
    } else if (order == L_TR_RO_SC) {
        tx = bx + shiftx;
        ty = by + shifty;
        if (angle == 0.0) {
            rx = tx;
            ry = ty;
            rw = bw;
            rh = bh;
        } else {
            xdif = tx + 0.5 * bw - xcen;
            ydif = ty + 0.5 * bh - ycen;
            rw = L_ABS(bw * cosa) + L_ABS(bh * sina);
            rh = L_ABS(bh * cosa) + L_ABS(bw * sina);
            rx = xcen + xdif * cosa - ydif * sina - 0.5 * rw;
            ry = ycen + ydif * cosa + xdif * sina - 0.5 * rh;
        }
        tx = (l_int32)(scalex * rx + 0.5);
        ty = (l_int32)(scaley * ry + 0.5);
        tw = (l_int32)(L_MAX(1.0, scalex * rw + 0.5));
        th = (l_int32)(L_MAX(1.0, scaley * rh + 0.5));
        boxd = boxCreate(tx, ty, tw, th);
    } else {  /* order == L_SC_RO_TR) */
        tx = (l_int32)(scalex * bx + 0.5);
        ty = (l_int32)(scaley * by + 0.5);
        tw = (l_int32)(L_MAX(1.0, scalex * bw + 0.5));
        th = (l_int32)(L_MAX(1.0, scaley * bh + 0.5));
        xcent = (l_int32)(scalex * xcen + 0.5);
        ycent = (l_int32)(scaley * ycen + 0.5);
        if (angle == 0.0) {
            rx = tx;
            ry = ty;
            rw = tw;
            rh = th;
        } else {
            xdif = tx + 0.5 * tw - xcent;
            ydif = ty + 0.5 * th - ycent;
            rw = L_ABS(tw * cosa) + L_ABS(th * sina);
            rh = L_ABS(th * cosa) + L_ABS(tw * sina);
            rx = xcent + xdif * cosa - ydif * sina - 0.5 * rw;
            ry = ycent + ydif * cosa + xdif * sina - 0.5 * rh;
        }
        tx = (l_int32)(rx + shiftx + 0.5);
        ty = (l_int32)(ry + shifty + 0.5);
        tw = (l_int32)(rw + 0.5);
        th = (l_int32)(rh + 0.5);
        boxd = boxCreate(tx, ty, tw, th);
    }

    return boxd;
}


/*!
 *  boxaRotateOrth()
 *
 *      Input:  boxa
 *              w, h (of image in which the boxa is embedded)
 *              rotation (0 = noop, 1 = 90 deg, 2 = 180 deg, 3 = 270 deg;
 *                        all rotations are clockwise)
 *      Return: boxad, or null on error
 *
 *  Notes:
 *      (1) See boxRotateOrth() for details.
 */
BOXA *
boxaRotateOrth(BOXA    *boxas,
               l_int32  w,
               l_int32  h,
               l_int32  rotation)
{
l_int32  i, n;
BOX     *boxs, *boxd;
BOXA    *boxad;

    PROCNAME("boxaRotateOrth");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (rotation < 0 || rotation > 3)
        return (BOXA *)ERROR_PTR("rotation not in {0,1,2,3}", procName, NULL);
    if (rotation == 0)
        return boxaCopy(boxas, L_COPY);

    n = boxaGetCount(boxas);
    if ((boxad = boxaCreate(n)) == NULL)
        return (BOXA *)ERROR_PTR("boxad not made", procName, NULL);
    for (i = 0; i < n; i++) {
        if ((boxs = boxaGetBox(boxas, i, L_CLONE)) == NULL)
            return (BOXA *)ERROR_PTR("boxs not found", procName, NULL);
        boxd = boxRotateOrth(boxs, w, h, rotation);
        boxDestroy(&boxs);
        boxaAddBox(boxad, boxd, L_INSERT);
    }

    return boxad;
}


/*!
 *  boxRotateOrth()
 *
 *      Input:  box
 *              w, h (of image in which the box is embedded)
 *              rotation (0 = noop, 1 = 90 deg, 2 = 180 deg, 3 = 270 deg;
 *                        all rotations are clockwise)
 *      Return: boxd, or null on error
 *
 *  Notes:
 *      (1) Rotate the image with the embedded box by the specified amount.
 *      (2) After rotation, the rotated box is always measured with
 *          respect to the UL corner of the image.
 */
BOX *
boxRotateOrth(BOX     *box,
              l_int32  w,
              l_int32  h,
              l_int32  rotation)
{
l_int32  bx, by, bw, bh, xdist, ydist;

    PROCNAME("boxRotateOrth");

    if (!box)
        return (BOX *)ERROR_PTR("box not defined", procName, NULL);
    if (rotation < 0 || rotation > 3)
        return (BOX *)ERROR_PTR("rotation not in {0,1,2,3}", procName, NULL);
    if (rotation == 0)
        return boxCopy(box);

    boxGetGeometry(box, &bx, &by, &bw, &bh);
    if (bw <= 0 || bh <= 0)  /* invalid */
        return boxCreate(0, 0, 0, 0);
    ydist = h - by - bh;  /* below box */
    xdist = w - bx - bw;  /* to right of box */
    if (rotation == 1)   /* 90 deg cw */
        return boxCreate(ydist, bx, bh, bw);
    else if (rotation == 2)  /* 180 deg cw */
        return boxCreate(xdist, ydist, bw, bh);
    else  /*  rotation == 3, 270 deg cw */
        return boxCreate(by, xdist, bh, bw);
}


/*---------------------------------------------------------------------*
 *                              Boxa sort                              *
 *---------------------------------------------------------------------*/
/*!
 *  boxaSort()
 *
 *      Input:  boxa
 *              sorttype (L_SORT_BY_X, L_SORT_BY_Y,
 *                        L_SORT_BY_RIGHT, L_SORT_BY_BOT,
 *                        L_SORT_BY_WIDTH, L_SORT_BY_HEIGHT,
 *                        L_SORT_BY_MIN_DIMENSION, L_SORT_BY_MAX_DIMENSION,
 *                        L_SORT_BY_PERIMETER, L_SORT_BY_AREA,
 *                        L_SORT_BY_ASPECT_RATIO)
 *              sortorder  (L_SORT_INCREASING, L_SORT_DECREASING)
 *              &naindex (<optional return> index of sorted order into
 *                        original array)
 *      Return: boxad (sorted version of boxas), or null on error
 */
BOXA *
boxaSort(BOXA    *boxas,
         l_int32  sorttype,
         l_int32  sortorder,
         NUMA   **pnaindex)
{
l_int32    i, n, x, y, w, h, size;
BOXA      *boxad;
NUMA      *na, *naindex;

    PROCNAME("boxaSort");

    if (pnaindex) *pnaindex = NULL;
    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (sorttype != L_SORT_BY_X && sorttype != L_SORT_BY_Y &&
        sorttype != L_SORT_BY_RIGHT && sorttype != L_SORT_BY_BOT &&
        sorttype != L_SORT_BY_WIDTH && sorttype != L_SORT_BY_HEIGHT &&
        sorttype != L_SORT_BY_MIN_DIMENSION &&
        sorttype != L_SORT_BY_MAX_DIMENSION &&
        sorttype != L_SORT_BY_PERIMETER &&
        sorttype != L_SORT_BY_AREA &&
        sorttype != L_SORT_BY_ASPECT_RATIO)
        return (BOXA *)ERROR_PTR("invalid sort type", procName, NULL);
    if (sortorder != L_SORT_INCREASING && sortorder != L_SORT_DECREASING)
        return (BOXA *)ERROR_PTR("invalid sort order", procName, NULL);

        /* Use O(n) binsort if possible */
    n = boxaGetCount(boxas);
    if (n > MIN_COMPS_FOR_BIN_SORT &&
        ((sorttype == L_SORT_BY_X) || (sorttype == L_SORT_BY_Y) ||
         (sorttype == L_SORT_BY_WIDTH) || (sorttype == L_SORT_BY_HEIGHT) ||
         (sorttype == L_SORT_BY_PERIMETER)))
        return boxaBinSort(boxas, sorttype, sortorder, pnaindex);

        /* Build up numa of specific data */
    if ((na = numaCreate(n)) == NULL)
        return (BOXA *)ERROR_PTR("na not made", procName, NULL);
    for (i = 0; i < n; i++) {
        boxaGetBoxGeometry(boxas, i, &x, &y, &w, &h);
        switch (sorttype)
        {
        case L_SORT_BY_X:
            numaAddNumber(na, x);
            break;
        case L_SORT_BY_Y:
            numaAddNumber(na, y);
            break;
        case L_SORT_BY_RIGHT:
            numaAddNumber(na, x + w - 1);
            break;
        case L_SORT_BY_BOT:
            numaAddNumber(na, y + h - 1);
            break;
        case L_SORT_BY_WIDTH:
            numaAddNumber(na, w);
            break;
        case L_SORT_BY_HEIGHT:
            numaAddNumber(na, h);
            break;
        case L_SORT_BY_MIN_DIMENSION:
            size = L_MIN(w, h);
            numaAddNumber(na, size);
            break;
        case L_SORT_BY_MAX_DIMENSION:
            size = L_MAX(w, h);
            numaAddNumber(na, size);
            break;
        case L_SORT_BY_PERIMETER:
            size = w + h;
            numaAddNumber(na, size);
            break;
        case L_SORT_BY_AREA:
            size = w * h;
            numaAddNumber(na, size);
            break;
        case L_SORT_BY_ASPECT_RATIO:
            numaAddNumber(na, (l_float32)w / (l_float32)h);
            break;
        default:
            L_WARNING("invalid sort type\n", procName);
        }
    }

        /* Get the sort index for data array */
    if ((naindex = numaGetSortIndex(na, sortorder)) == NULL)
        return (BOXA *)ERROR_PTR("naindex not made", procName, NULL);

        /* Build up sorted boxa using sort index */
    boxad = boxaSortByIndex(boxas, naindex);

    if (pnaindex)
        *pnaindex = naindex;
    else
        numaDestroy(&naindex);
    numaDestroy(&na);
    return boxad;
}


/*!
 *  boxaBinSort()
 *
 *      Input:  boxa
 *              sorttype (L_SORT_BY_X, L_SORT_BY_Y, L_SORT_BY_WIDTH,
 *                        L_SORT_BY_HEIGHT, L_SORT_BY_PERIMETER)
 *              sortorder  (L_SORT_INCREASING, L_SORT_DECREASING)
 *              &naindex (<optional return> index of sorted order into
 *                        original array)
 *      Return: boxad (sorted version of boxas), or null on error
 *
 *  Notes:
 *      (1) For a large number of boxes (say, greater than 1000), this
 *          O(n) binsort is much faster than the O(nlogn) shellsort.
 *          For 5000 components, this is over 20x faster than boxaSort().
 *      (2) Consequently, boxaSort() calls this function if it will
 *          likely go much faster.
 */
BOXA *
boxaBinSort(BOXA    *boxas,
            l_int32  sorttype,
            l_int32  sortorder,
            NUMA   **pnaindex)
{
l_int32  i, n, x, y, w, h;
BOXA    *boxad;
NUMA    *na, *naindex;

    PROCNAME("boxaBinSort");

    if (pnaindex) *pnaindex = NULL;
    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (sorttype != L_SORT_BY_X && sorttype != L_SORT_BY_Y &&
        sorttype != L_SORT_BY_WIDTH && sorttype != L_SORT_BY_HEIGHT &&
        sorttype != L_SORT_BY_PERIMETER)
        return (BOXA *)ERROR_PTR("invalid sort type", procName, NULL);
    if (sortorder != L_SORT_INCREASING && sortorder != L_SORT_DECREASING)
        return (BOXA *)ERROR_PTR("invalid sort order", procName, NULL);

        /* Generate Numa of appropriate box dimensions */
    n = boxaGetCount(boxas);
    if ((na = numaCreate(n)) == NULL)
        return (BOXA *)ERROR_PTR("na not made", procName, NULL);
    for (i = 0; i < n; i++) {
        boxaGetBoxGeometry(boxas, i, &x, &y, &w, &h);
        switch (sorttype)
        {
        case L_SORT_BY_X:
            numaAddNumber(na, x);
            break;
        case L_SORT_BY_Y:
            numaAddNumber(na, y);
            break;
        case L_SORT_BY_WIDTH:
            numaAddNumber(na, w);
            break;
        case L_SORT_BY_HEIGHT:
            numaAddNumber(na, h);
            break;
        case L_SORT_BY_PERIMETER:
            numaAddNumber(na, w + h);
            break;
        default:
            L_WARNING("invalid sort type\n", procName);
        }
    }

        /* Get the sort index for data array */
    if ((naindex = numaGetBinSortIndex(na, sortorder)) == NULL)
        return (BOXA *)ERROR_PTR("naindex not made", procName, NULL);

        /* Build up sorted boxa using the sort index */
    boxad = boxaSortByIndex(boxas, naindex);

    if (pnaindex)
        *pnaindex = naindex;
    else
        numaDestroy(&naindex);
    numaDestroy(&na);
    return boxad;
}


/*!
 *  boxaSortByIndex()
 *
 *      Input:  boxas
 *              naindex (na that maps from the new boxa to the input boxa)
 *      Return: boxad (sorted), or null on error
 */
BOXA *
boxaSortByIndex(BOXA  *boxas,
                NUMA  *naindex)
{
l_int32  i, n, index;
BOX     *box;
BOXA    *boxad;

    PROCNAME("boxaSortByIndex");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (!naindex)
        return (BOXA *)ERROR_PTR("naindex not defined", procName, NULL);

    n = boxaGetCount(boxas);
    boxad = boxaCreate(n);
    for (i = 0; i < n; i++) {
        numaGetIValue(naindex, i, &index);
        box = boxaGetBox(boxas, index, L_COPY);
        boxaAddBox(boxad, box, L_INSERT);
    }

    return boxad;
}


/*!
 *  boxaSort2d()
 *
 *      Input:  boxas
 *              &naa (<optional return> numaa with sorted indices
 *                    whose values are the indices of the input array)
 *              delta1 (min overlap that permits aggregation of a box
 *                      onto a boxa of horizontally-aligned boxes; pass 1)
 *              delta2 (min overlap that permits aggregation of a box
 *                      onto a boxa of horizontally-aligned boxes; pass 2)
 *              minh1 (components less than this height either join an
 *                     existing boxa or are set aside for pass 2)
 *      Return: baa (2d sorted version of boxa), or null on error
 *
 *  Notes:
 *      (1) The final result is a sort where the 'fast scan' direction is
 *          left to right, and the 'slow scan' direction is from top
 *          to bottom.  Each boxa in the baa represents a sorted set
 *          of boxes from left to right.
 *      (2) Three passes are used to aggregate the boxas, which can correspond
 *          to characters or words in a line of text.  In pass 1, only
 *          taller components, which correspond to xheight or larger,
 *          are permitted to start a new boxa.  In pass 2, the remaining
 *          vertically-challenged components are allowed to join an
 *          existing boxa or start a new one.  In pass 3, boxa whose extent
 *          is overlapping are joined.  After that, the boxes in each
 *          boxa are sorted horizontally, and finally the boxa are
 *          sorted vertically.
 *      (3) If delta1 < 0, the first pass allows aggregation when
 *          boxes in the same boxa do not overlap vertically.
 *          The distance by which they can miss and still be aggregated
 *          is the absolute value |delta1|.   Similar for delta2 on
 *          the second pass.
 *      (4) On the first pass, any component of height less than minh1
 *          cannot start a new boxa; it's put aside for later insertion.
 *      (5) On the second pass, any small component that doesn't align
 *          with an existing boxa can start a new one.
 *      (6) This can be used to identify lines of text from
 *          character or word bounding boxes.
 */
BOXAA *
boxaSort2d(BOXA    *boxas,
           NUMAA  **pnaad,
           l_int32  delta1,
           l_int32  delta2,
           l_int32  minh1)
{
l_int32  i, index, h, nt, ne, n, m, ival;
BOX     *box;
BOXA    *boxa, *boxae, *boxan, *boxa1, *boxa2, *boxa3, *boxav, *boxavs;
BOXAA   *baa, *baa1, *baad;
NUMA    *naindex, *nae, *nan, *nah, *nav, *na1, *na2, *nad, *namap;
NUMAA   *naa, *naa1, *naad;

    PROCNAME("boxaSort2d");

    if (pnaad) *pnaad = NULL;
    if (!boxas)
        return (BOXAA *)ERROR_PTR("boxas not defined", procName, NULL);

        /* Sort from left to right */
    if ((boxa = boxaSort(boxas, L_SORT_BY_X, L_SORT_INCREASING, &naindex))
                    == NULL)
        return (BOXAA *)ERROR_PTR("boxa not made", procName, NULL);

        /* First pass: assign taller boxes to boxa by row */
    nt = boxaGetCount(boxa);
    baa = boxaaCreate(0);
    naa = numaaCreate(0);
    boxae = boxaCreate(0);  /* save small height boxes here */
    nae = numaCreate(0);  /* keep track of small height boxes */
    for (i = 0; i < nt; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        boxGetGeometry(box, NULL, NULL, NULL, &h);
        if (h < minh1) {  /* save for 2nd pass */
            boxaAddBox(boxae, box, L_INSERT);
            numaAddNumber(nae, i);
        } else {
            n = boxaaGetCount(baa);
            boxaaAlignBox(baa, box, delta1, &index);
            if (index < n) {  /* append to an existing boxa */
                boxaaAddBox(baa, index, box, L_INSERT);
            } else {  /* doesn't align, need new boxa */
                boxan = boxaCreate(0);
                boxaAddBox(boxan, box, L_INSERT);
                boxaaAddBoxa(baa, boxan, L_INSERT);
                nan = numaCreate(0);
                numaaAddNuma(naa, nan, L_INSERT);
            }
            numaGetIValue(naindex, i, &ival);
            numaaAddNumber(naa, index, ival);
        }
    }
    boxaDestroy(&boxa);
    numaDestroy(&naindex);

        /* Second pass: feed in small height boxes */
    ne = boxaGetCount(boxae);
    for (i = 0; i < ne; i++) {
        box = boxaGetBox(boxae, i, L_CLONE);
        n = boxaaGetCount(baa);
        boxaaAlignBox(baa, box, delta2, &index);
        if (index < n) {  /* append to an existing boxa */
            boxaaAddBox(baa, index, box, L_INSERT);
        } else {  /* doesn't align, need new boxa */
            boxan = boxaCreate(0);
            boxaAddBox(boxan, box, L_INSERT);
            boxaaAddBoxa(baa, boxan, L_INSERT);
            nan = numaCreate(0);
            numaaAddNuma(naa, nan, L_INSERT);
        }
        numaGetIValue(nae, i, &ival);  /* location in original boxas */
        numaaAddNumber(naa, index, ival);
    }

        /* Third pass: merge some boxa whose extent is overlapping.
         * Think of these boxa as text lines, where the bounding boxes
         * of the text lines can overlap, but likely won't have
         * a huge overlap.
         * First do a greedy find of pairs of overlapping boxa, where
         * the two boxa overlap by at least 50% of the smaller, and
         * the smaller is not more than half the area of the larger.
         * For such pairs, call the larger one the primary boxa.  The
         * boxes in the smaller one are appended to those in the primary
         * in pass 3a, and the primaries are extracted in pass 3b.
         * In this way, all boxes in the original baa are saved. */
    n = boxaaGetCount(baa);
    boxaaGetExtent(baa, NULL, NULL, NULL, &boxa3);
    boxa1 = boxaHandleOverlaps(boxa3, L_REMOVE_SMALL, 1000, 0.5, 0.5, &namap);
    boxaDestroy(&boxa1);
    boxaDestroy(&boxa3);
    for (i = 0; i < n; i++) {  /* Pass 3a: join selected copies of boxa */
        numaGetIValue(namap, i, &ival);
        if (ival >= 0) {  /* join current to primary boxa[ival] */
            boxa1 = boxaaGetBoxa(baa, i, L_COPY);
            boxa2 = boxaaGetBoxa(baa, ival, L_CLONE);
            boxaJoin(boxa2, boxa1, 0, -1);
            boxaDestroy(&boxa2);
            boxaDestroy(&boxa1);
            na1 = numaaGetNuma(naa, i, L_COPY);
            na2 = numaaGetNuma(naa, ival, L_CLONE);
            numaJoin(na2, na1, 0, -1);
            numaDestroy(&na1);
            numaDestroy(&na2);
        }
    }
    baa1 = boxaaCreate(n);
    naa1 = numaaCreate(n);
    for (i = 0; i < n; i++) {  /* Pass 3b: save primary boxa */
        numaGetIValue(namap, i, &ival);
        if (ival == -1) {
            boxa1 = boxaaGetBoxa(baa, i, L_CLONE);
            boxaaAddBoxa(baa1, boxa1, L_INSERT);
            na1 = numaaGetNuma(naa, i, L_CLONE);
            numaaAddNuma(naa1, na1, L_INSERT);
        }
    }
    numaDestroy(&namap);
    boxaaDestroy(&baa);
    baa = baa1;
    numaaDestroy(&naa);
    naa = naa1;

        /* Sort the boxes in each boxa horizontally */
    m = boxaaGetCount(baa);
    for (i = 0; i < m; i++) {
        boxa1 = boxaaGetBoxa(baa, i, L_CLONE);
        boxa2 = boxaSort(boxa1, L_SORT_BY_X, L_SORT_INCREASING, &nah);
        boxaaReplaceBoxa(baa, i, boxa2);
        na1 = numaaGetNuma(naa, i, L_CLONE);
        na2 = numaSortByIndex(na1, nah);
        numaaReplaceNuma(naa, i, na2);
        boxaDestroy(&boxa1);
        numaDestroy(&na1);
        numaDestroy(&nah);
    }

        /* Sort the boxa vertically within boxaa, using the first box
         * in each boxa. */
    m = boxaaGetCount(baa);
    boxav = boxaCreate(m);  /* holds first box in each boxa in baa */
    naad = numaaCreate(m);
    if (pnaad)
        *pnaad = naad;
    baad = boxaaCreate(m);
    for (i = 0; i < m; i++) {
        boxa1 = boxaaGetBoxa(baa, i, L_CLONE);
        box = boxaGetBox(boxa1, 0, L_CLONE);
        boxaAddBox(boxav, box, L_INSERT);
        boxaDestroy(&boxa1);
    }
    boxavs = boxaSort(boxav, L_SORT_BY_Y, L_SORT_INCREASING, &nav);
    for (i = 0; i < m; i++) {
        numaGetIValue(nav, i, &index);
        boxa = boxaaGetBoxa(baa, index, L_CLONE);
        boxaaAddBoxa(baad, boxa, L_INSERT);
        nad = numaaGetNuma(naa, index, L_CLONE);
        numaaAddNuma(naad, nad, L_INSERT);
    }


/*    fprintf(stderr, "box count = %d, numaa count = %d\n", nt,
            numaaGetNumberCount(naad)); */

    boxaaDestroy(&baa);
    boxaDestroy(&boxav);
    boxaDestroy(&boxavs);
    boxaDestroy(&boxae);
    numaDestroy(&nav);
    numaDestroy(&nae);
    numaaDestroy(&naa);
    if (!pnaad)
        numaaDestroy(&naad);

    return baad;
}


/*!
 *  boxaSort2dByIndex()
 *
 *      Input:  boxas
 *              naa (numaa that maps from the new baa to the input boxa)
 *      Return: baa (sorted boxaa), or null on error
 */
BOXAA *
boxaSort2dByIndex(BOXA   *boxas,
                  NUMAA  *naa)
{
l_int32  ntot, boxtot, i, j, n, nn, index;
BOX     *box;
BOXA    *boxa;
BOXAA   *baa;
NUMA    *na;

    PROCNAME("boxaSort2dByIndex");

    if (!boxas)
        return (BOXAA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (!naa)
        return (BOXAA *)ERROR_PTR("naindex not defined", procName, NULL);

        /* Check counts */
    ntot = numaaGetNumberCount(naa);
    boxtot = boxaGetCount(boxas);
    if (ntot != boxtot)
        return (BOXAA *)ERROR_PTR("element count mismatch", procName, NULL);

    n = numaaGetCount(naa);
    baa = boxaaCreate(n);
    for (i = 0; i < n; i++) {
        na = numaaGetNuma(naa, i, L_CLONE);
        nn = numaGetCount(na);
        boxa = boxaCreate(nn);
        for (j = 0; j < nn; j++) {
            numaGetIValue(na, i, &index);
            box = boxaGetBox(boxas, index, L_COPY);
            boxaAddBox(boxa, box, L_INSERT);
        }
        boxaaAddBoxa(baa, boxa, L_INSERT);
        numaDestroy(&na);
    }

    return baa;
}


/*---------------------------------------------------------------------*
 *                        Boxa array extraction                        *
 *---------------------------------------------------------------------*/
/*!
 *  boxaExtractAsNuma()
 *
 *      Input:  boxa
 *              &nax (<optional return> array of x locations)
 *              &nay (<optional return> array of y locations)
 *              &naw (<optional return> array of w locations)
 *              &nah (<optional return> array of h locations)
 *              keepinvalid (1 to keep invalid boxes; 0 to remove them)
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxaExtractAsNuma(BOXA    *boxa,
                  NUMA   **pnax,
                  NUMA   **pnay,
                  NUMA   **pnaw,
                  NUMA   **pnah,
                  l_int32  keepinvalid)
{
l_int32  i, n, x, y, w, h;

    PROCNAME("boxaExtractAsNuma");

    if (!pnax && !pnay && !pnaw && !pnah)
        return ERROR_INT("no output requested", procName, 1);
    if (pnax) *pnax = NULL;
    if (pnay) *pnay = NULL;
    if (pnaw) *pnaw = NULL;
    if (pnah) *pnah = NULL;
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (!keepinvalid && boxaGetValidCount(boxa) == 0)
        return ERROR_INT("no valid boxes", procName, 1);

    n = boxaGetCount(boxa);
    if (pnax) *pnax = numaCreate(n);
    if (pnay) *pnay = numaCreate(n);
    if (pnaw) *pnaw = numaCreate(n);
    if (pnah) *pnah = numaCreate(n);
    for (i = 0; i < n; i++) {
        boxaGetBoxGeometry(boxa, i, &x, &y, &w, &h);
        if (!keepinvalid && (w <= 0 || h <= 0))
            continue;
        if (pnax) numaAddNumber(*pnax, x);
        if (pnay) numaAddNumber(*pnay, y);
        if (pnaw) numaAddNumber(*pnaw, w);
        if (pnah) numaAddNumber(*pnah, h);
    }

    return 0;
}


/*!
 *  boxaExtractAsPta()
 *
 *      Input:  boxa
 *              &ptal (<optional return> array of left locations vs. index)
 *              &ptat (<optional return> array of top locations vs. index)
 *              &ptar (<optional return> array of right locations vs. index)
 *              &ptab (<optional return> array of bottom locations vs. index)
 *              keepinvalid (1 to keep invalid boxes; 0 to remove them)
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxaExtractAsPta(BOXA    *boxa,
                 PTA    **pptal,
                 PTA    **pptat,
                 PTA    **pptar,
                 PTA    **pptab,
                 l_int32  keepinvalid)
{
l_int32  i, n, left, top, right, bot, w, h;

    PROCNAME("boxaExtractAsPta");

    if (!pptal && !pptar && !pptat && !pptab)
        return ERROR_INT("no output requested", procName, 1);
    if (pptal) *pptal = NULL;
    if (pptat) *pptat = NULL;
    if (pptar) *pptar = NULL;
    if (pptab) *pptab = NULL;
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (!keepinvalid && boxaGetValidCount(boxa) == 0)
        return ERROR_INT("no valid boxes", procName, 1);

    n = boxaGetCount(boxa);
    if (pptal) *pptal = ptaCreate(n);
    if (pptat) *pptat = ptaCreate(n);
    if (pptar) *pptar = ptaCreate(n);
    if (pptab) *pptab = ptaCreate(n);
    for (i = 0; i < n; i++) {
        boxaGetBoxGeometry(boxa, i, &left, &top, &w, &h);
        if (!keepinvalid && (w <= 0 || h <= 0))
            continue;
        right = left + w - 1;
        bot = top + h - 1;
        if (pptal) ptaAddPt(*pptal, i, left);
        if (pptat) ptaAddPt(*pptat, i, top);
        if (pptar) ptaAddPt(*pptar, i, right);
        if (pptab) ptaAddPt(*pptab, i, bot);
    }

    return 0;
}


/*---------------------------------------------------------------------*
 *                            Boxa statistics                          *
 *---------------------------------------------------------------------*/
/*!
 *  boxaGetRankSize()
 *
 *      Input:  boxa
 *              fract (use 0.0 for smallest, 1.0 for largest)
 *      Return: box (with rank values for x, y, w, h), or null on error
 *              or if the boxa is empty (has no valid boxes)
 *
 *  Notes:
 *      (1) This function does not assume that all boxes in the boxa are valid
 *      (2) The four box parameters are sorted independently.
 *          For rank order, the width and height are sorted in increasing
 *          order.  But what does it mean to sort x and y in "rank order"?
 *          If the boxes are of comparable size and somewhat
 *          aligned (e.g., from multiple images), it makes some sense
 *          to give a "rank order" for x and y by sorting them in
 *          decreasing order.  But in general, the interpretation of a rank
 *          order on x and y is highly application dependent.  In summary:
 *             - x and y are sorted in decreasing order
 *             - w and h are sorted in increasing order
 */
BOX *
boxaGetRankSize(BOXA      *boxa,
                l_float32  fract)
{
l_float32  xval, yval, wval, hval;
NUMA      *nax, *nay, *naw, *nah;
BOX       *box;

    PROCNAME("boxaGetRankSize");

    if (!boxa)
        return (BOX *)ERROR_PTR("boxa not defined", procName, NULL);
    if (fract < 0.0 || fract > 1.0)
        return (BOX *)ERROR_PTR("fract not in [0.0 ... 1.0]", procName, NULL);
    if (boxaGetValidCount(boxa) == 0)
        return (BOX *)ERROR_PTR("no valid boxes in boxa", procName, NULL);

    boxaExtractAsNuma(boxa, &nax, &nay, &naw, &nah, 0);  /* valid boxes only */

    numaGetRankValue(nax, 1.0 - fract, NULL, 1, &xval);
    numaGetRankValue(nay, 1.0 - fract, NULL, 1, &yval);
    numaGetRankValue(naw, fract, NULL, 1, &wval);
    numaGetRankValue(nah, fract, NULL, 1, &hval);
    box = boxCreate((l_int32)xval, (l_int32)yval, (l_int32)wval, (l_int32)hval);

    numaDestroy(&nax);
    numaDestroy(&nay);
    numaDestroy(&naw);
    numaDestroy(&nah);
    return box;
}


/*!
 *  boxaGetMedian()
 *
 *      Input:  boxa
 *      Return: box (with median values for x, y, w, h), or null on error
 *              or if the boxa is empty.
 *
 *  Notes:
 *      (1) See boxaGetRankSize()
 */
BOX *
boxaGetMedian(BOXA  *boxa)
{
    PROCNAME("boxaGetMedian");

    if (!boxa)
        return (BOX *)ERROR_PTR("boxa not defined", procName, NULL);
    if (boxaGetCount(boxa) == 0)
        return (BOX *)ERROR_PTR("boxa is empty", procName, NULL);

    return boxaGetRankSize(boxa, 0.5);
}


/*---------------------------------------------------------------------*
 *                        Other Boxaa functions                        *
 *---------------------------------------------------------------------*/
/*!
 *  boxaaGetExtent()
 *
 *      Input:  baa
 *              &w  (<optional return> width)
 *              &h  (<optional return> height)
 *              &box (<optional return>, minimum box containing all boxa
 *                    in boxaa)
 *              &boxa (<optional return>, boxa containing all boxes in each
 *                     boxa in the boxaa)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The returned w and h are the minimum size image
 *          that would contain all boxes untranslated.
 *      (2) Each box in the returned boxa is the minimum box required to
 *          hold all the boxes in the respective boxa of baa.
 *      (3) If there are no valid boxes in a boxa, the box corresponding
 *          to its extent has all fields set to 0 (an invalid box).
 */
l_int32
boxaaGetExtent(BOXAA    *baa,
               l_int32  *pw,
               l_int32  *ph,
               BOX     **pbox,
               BOXA    **pboxa)
{
l_int32  i, n, x, y, w, h, xmax, ymax, xmin, ymin, found;
BOX     *box1;
BOXA    *boxa, *boxa1;

    PROCNAME("boxaaGetExtent");

    if (!pw && !ph && !pbox && !pboxa)
        return ERROR_INT("no ptrs defined", procName, 1);
    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (pbox) *pbox = NULL;
    if (pboxa) *pboxa = NULL;
    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);

    n = boxaaGetCount(baa);
    if (n == 0)
        return ERROR_INT("no boxa in baa", procName, 1);

    boxa = boxaCreate(n);
    xmax = ymax = 0;
    xmin = ymin = 100000000;
    found = FALSE;
    for (i = 0; i < n; i++) {
        boxa1 = boxaaGetBoxa(baa, i, L_CLONE);
        boxaGetExtent(boxa1, NULL, NULL, &box1);
        boxaDestroy(&boxa1);
        boxGetGeometry(box1, &x, &y, &w, &h);
        if (w > 0 && h > 0) {  /* a valid extent box */
            found = TRUE;  /* found at least one valid extent box */
            xmin = L_MIN(xmin, x);
            ymin = L_MIN(ymin, y);
            xmax = L_MAX(xmax, x + w);
            ymax = L_MAX(ymax, y + h);
        }
        boxaAddBox(boxa, box1, L_INSERT);
    }
    if (found == FALSE)  /* no valid extent boxes */
        xmin = ymin = 0;

    if (pw) *pw = xmax;
    if (ph) *ph = ymax;
    if (pbox)
        *pbox = boxCreate(xmin, ymin, xmax - xmin, ymax - ymin);
    if (pboxa)
        *pboxa = boxa;
    else
        boxaDestroy(&boxa);
    return 0;
}


/*!
 *  boxaaFlattenToBoxa()
 *
 *      Input:  baa
 *              &naindex  (<optional return> the boxa index in the baa)
 *              copyflag  (L_COPY or L_CLONE)
 *      Return: boxa, or null on error
 *
 *  Notes:
 *      (1) This 'flattens' the baa to a boxa, taking the boxes in
 *          order in the first boxa, then the second, etc.
 *      (2) If a boxa is empty, we generate an invalid, placeholder box
 *          of zero size.  This is useful when converting from a baa
 *          where each boxa has either 0 or 1 boxes, and it is necessary
 *          to maintain a 1:1 correspondence between the initial
 *          boxa array and the resulting box array.
 *      (3) If &naindex is defined, we generate a Numa that gives, for
 *          each box in the baa, the index of the boxa to which it belongs.
 */
BOXA *
boxaaFlattenToBoxa(BOXAA   *baa,
                   NUMA   **pnaindex,
                   l_int32  copyflag)
{
l_int32  i, j, m, n;
BOXA    *boxa, *boxat;
BOX     *box;
NUMA    *naindex;

    PROCNAME("boxaaFlattenToBoxa");

    if (pnaindex) *pnaindex = NULL;
    if (!baa)
        return (BOXA *)ERROR_PTR("baa not defined", procName, NULL);
    if (copyflag != L_COPY && copyflag != L_CLONE)
        return (BOXA *)ERROR_PTR("invalid copyflag", procName, NULL);
    if (pnaindex) {
        naindex = numaCreate(0);
        *pnaindex = naindex;
    }

    n = boxaaGetCount(baa);
    boxa = boxaCreate(n);
    for (i = 0; i < n; i++) {
        boxat = boxaaGetBoxa(baa, i, L_CLONE);
        m = boxaGetCount(boxat);
        if (m == 0) {  /* placeholder box */
            box = boxCreate(0, 0, 0, 0);
            boxaAddBox(boxa, box, L_INSERT);
            if (pnaindex)
                numaAddNumber(naindex, i);  /* save 'row' number */
        } else {
            for (j = 0; j < m; j++) {
                box = boxaGetBox(boxat, j, copyflag);
                boxaAddBox(boxa, box, L_INSERT);
                if (pnaindex)
                    numaAddNumber(naindex, i);  /* save 'row' number */
            }
        }
        boxaDestroy(&boxat);
    }

    return boxa;
}


/*!
 *  boxaaFlattenAligned()
 *
 *      Input:  baa
 *              num (number extracted from each)
 *              fillerbox (<optional> that fills if necessary)
 *              copyflag  (L_COPY or L_CLONE)
 *      Return: boxa, or null on error
 *
 *  Notes:
 *      (1) This 'flattens' the baa to a boxa, taking the first @num
 *          boxes from each boxa.
 *      (2) In each boxa, if there are less than @num boxes, we preserve
 *          the alignment between the input baa and the output boxa
 *          by inserting one or more fillerbox(es) or, if @fillerbox == NULL,
 *          one or more invalid placeholder boxes.
 */
BOXA *
boxaaFlattenAligned(BOXAA   *baa,
                    l_int32  num,
                    BOX     *fillerbox,
                    l_int32  copyflag)
{
l_int32  i, j, m, n, mval, nshort;
BOXA    *boxat, *boxad;
BOX     *box;

    PROCNAME("boxaaFlattenAligned");

    if (!baa)
        return (BOXA *)ERROR_PTR("baa not defined", procName, NULL);
    if (copyflag != L_COPY && copyflag != L_CLONE)
        return (BOXA *)ERROR_PTR("invalid copyflag", procName, NULL);

    n = boxaaGetCount(baa);
    boxad = boxaCreate(n);
    for (i = 0; i < n; i++) {
        boxat = boxaaGetBoxa(baa, i, L_CLONE);
        m = boxaGetCount(boxat);
        mval = L_MIN(m, num);
        nshort = num - mval;
        for (j = 0; j < mval; j++) {  /* take the first @num if possible */
            box = boxaGetBox(boxat, j, copyflag);
            boxaAddBox(boxad, box, L_INSERT);
        }
        for (j = 0; j < nshort; j++) {  /* add fillers if necessary */
            if (fillerbox) {
                boxaAddBox(boxad, fillerbox, L_COPY);
            } else {
                box = boxCreate(0, 0, 0, 0);  /* invalid placeholder box */
                boxaAddBox(boxad, box, L_INSERT);
            }
        }
        boxaDestroy(&boxat);
    }

    return boxad;
}


/*!
 *  boxaEncapsulateAligned()
 *
 *      Input:  boxa
 *              num (number put into each boxa in the baa)
 *              copyflag  (L_COPY or L_CLONE)
 *      Return: baa, or null on error
 *
 *  Notes:
 *      (1) This puts @num boxes from the input @boxa into each of a
 *          set of boxa within an output baa.
 *      (2) This assumes that the boxes in @boxa are in sets of @num each.
 */
BOXAA *
boxaEncapsulateAligned(BOXA    *boxa,
                       l_int32  num,
                       l_int32  copyflag)
{
l_int32  i, j, n, nbaa, index;
BOX     *box;
BOXA    *boxat;
BOXAA   *baa;

    PROCNAME("boxaEncapsulateAligned");

    if (!boxa)
        return (BOXAA *)ERROR_PTR("boxa not defined", procName, NULL);
    if (copyflag != L_COPY && copyflag != L_CLONE)
        return (BOXAA *)ERROR_PTR("invalid copyflag", procName, NULL);

    n = boxaGetCount(boxa);
    nbaa = n / num;
    if (num * nbaa != n)
        L_ERROR("inconsistent alignment: num doesn't divide n\n", procName);
    baa = boxaaCreate(nbaa);
    for (i = 0, index = 0; i < nbaa; i++) {
        boxat = boxaCreate(num);
        for (j = 0; j < num; j++, index++) {
            box = boxaGetBox(boxa, index, copyflag);
            boxaAddBox(boxat, box, L_INSERT);
        }
        boxaaAddBoxa(baa, boxat, L_INSERT);
    }

    return baa;
}


/*!
 *  boxaaAlignBox()
 *
 *      Input:  baa
 *              box (to be aligned with the bext boxa in the baa, if possible)
 *              delta (amount by which consecutive components can miss
 *                     in overlap and still be included in the array)
 *              &index (of boxa with best overlap, or if none match,
 *                      this is the index of the next boxa to be generated)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is not greedy.  It finds the boxa whose vertical
 *          extent has the closest overlap with the input box.
 */
l_int32
boxaaAlignBox(BOXAA    *baa,
              BOX      *box,
              l_int32   delta,
              l_int32  *pindex)
{
l_int32  i, n, m, y, yt, h, ht, ovlp, maxovlp, maxindex;
BOX     *boxt;
BOXA    *boxa;

    PROCNAME("boxaaAlignBox");

    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if (!pindex)
        return ERROR_INT("&index not defined", procName, 1);

    n = boxaaGetCount(baa);
    boxGetGeometry(box, NULL, &y, NULL, &h);
    maxovlp = -10000000;
    for (i = 0; i < n; i++) {
        boxa = boxaaGetBoxa(baa, i, L_CLONE);
        if ((m = boxaGetCount(boxa)) == 0) {
            L_WARNING("no boxes in boxa\n", procName);
            continue;
        }
        boxaGetExtent(boxa, NULL, NULL, &boxt);
        boxGetGeometry(boxt, NULL, &yt, NULL, &ht);
        boxDestroy(&boxt);
        boxaDestroy(&boxa);

            /* Overlap < 0 means the components do not overlap vertically */
        if (yt >= y)
            ovlp = y + h - 1 - yt;
        else
            ovlp = yt + ht - 1 - y;
        if (ovlp > maxovlp) {
            maxovlp = ovlp;
            maxindex = i;
        }
    }

    if (maxovlp + delta >= 0)
        *pindex = maxindex;
    else
        *pindex = n;
    return 0;
}
