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
 *      Other boxaa functions
 *           l_int32          boxaaGetExtent()
 *           BOXA            *boxaaFlattenToBoxa()
 *           l_int32          boxaaAlignBox()
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allheaders.h"

    /* For more than this number of c.c. in a binarized image of
     * semi-perimeter (w + h) about 5000 or less, the O(n) binsort
     * is faster than the O(nlogn) shellsort.  */
static const l_int32   MIN_COMPS_FOR_BIN_SORT = 500;


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
        if (angle == 0.0)
            boxd = boxCreate(tx, ty, tw, th);
        else {
            xdif = tx + 0.5 * tw - xcent;
            ydif = ty + 0.5 * th - ycent;
            rw = L_ABS(tw * cosa) + L_ABS(th * sina);
            rh = L_ABS(th * cosa) + L_ABS(tw * sina);
            rx = xcent + xdif * cosa - ydif * sina - 0.5 * rw;
            ry = ycent + ydif * cosa + xdif * sina - 0.5 * rh;
            boxd = boxCreate((l_int32)rx, (l_int32)ry, (l_int32)rw,
                             (l_int32)rh);
        }
    }
    else if (order == L_SC_TR_RO) {
        tx = (l_int32)(scalex * bx + shiftx + 0.5);
        ty = (l_int32)(scaley * by + shifty + 0.5);
        tw = (l_int32)(L_MAX(1.0, scalex * bw + 0.5));
        th = (l_int32)(L_MAX(1.0, scaley * bh + 0.5));
        xcent = (l_int32)(scalex * xcen + 0.5);
        ycent = (l_int32)(scaley * ycen + 0.5);
        if (angle == 0.0)
            boxd = boxCreate(tx, ty, tw, th);
        else {
            xdif = tx + 0.5 * tw - xcent;
            ydif = ty + 0.5 * th - ycent;
            rw = L_ABS(tw * cosa) + L_ABS(th * sina);
            rh = L_ABS(th * cosa) + L_ABS(tw * sina);
            rx = xcent + xdif * cosa - ydif * sina - 0.5 * rw;
            ry = ycent + ydif * cosa + xdif * sina - 0.5 * rh;
            boxd = boxCreate((l_int32)rx, (l_int32)ry, (l_int32)rw,
                             (l_int32)rh);
        }
    }
    else if (order == L_RO_TR_SC) {
        if (angle == 0.0) {
            rx = bx;
            ry = by;
            rw = bw;
            rh = bh;
        }
        else {
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
    }
    else if (order == L_RO_SC_TR) {
        if (angle == 0.0) {
            rx = bx;
            ry = by;
            rw = bw;
            rh = bh;
        }
        else {
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
    }
    else if (order == L_TR_RO_SC) {
        tx = bx + shiftx;
        ty = by + shifty;
        if (angle == 0.0) {
            rx = tx;
            ry = ty;
            rw = bw;
            rh = bh;
        }
        else {
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
    }
    else {  /* order == L_SC_RO_TR) */
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
        }
        else {
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
    if (rotation == 0)
        return boxaCopy(boxas, L_COPY);
    if (rotation < 1 || rotation > 3)
        return (BOXA *)ERROR_PTR("rotation not in {0,1,2,3}", procName, NULL);

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
    if (rotation == 0)
        return boxCopy(box);
    if (rotation < 1 || rotation > 3)
        return (BOX *)ERROR_PTR("rotation not in {0,1,2,3}", procName, NULL);
    boxGetGeometry(box, &bx, &by, &bw, &bh);
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
 *              sorttype (L_SORT_BY_X, L_SORT_BY_Y, L_SORT_BY_WIDTH,
 *                        L_SORT_BY_HEIGHT, L_SORT_BY_MIN_DIMENSION,
 *                        L_SORT_BY_MAX_DIMENSION, L_SORT_BY_PERIMETER,
 *                        L_SORT_BY_AREA, L_SORT_BY_ASPECT_RATIO)
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
            L_WARNING("invalid sort type", procName);
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
            L_WARNING("invalid sort type", procName);
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
 *      Return: boxaa (2d sorted version of boxa), or null on error
 *
 *  Notes:
 *      (1) The final result is a sort where the 'fast scan' direction is
 *          left to right, and the 'slow scan' direction is from top
 *          to bottom.  Each boxa in the boxaa represents a sorted set
 *          of boxes from left to right.
 *      (2) Two passes are used to aggregate the boxas, which can corresond
 *          to characters or words in a line of text.  In pass 1, only
 *          taller components, which correspond to xheight or larger,
 *          are permitted to start a new boxa, whereas in pass 2,
 *          the remaining vertically-challenged components are allowed
 *          to join an existing boxa or start a new one.
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
BOXA    *boxa, *boxae, *boxan, *boxat1, *boxat2, *boxav, *boxavs;
BOXAA   *baa, *baad;
NUMA    *naindex, *nae, *nan, *nah, *nav, *nat1, *nat2, *nad;
NUMAA   *naa, *naad;

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
        }
        else {
            n = boxaaGetCount(baa);
            boxaaAlignBox(baa, box, delta1, &index);
            if (index < n) {  /* append to an existing boxa */
                boxaaAddBox(baa, index, box, L_INSERT);
            }
            else {  /* doesn't align, need new boxa */
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

        /* Second pass: feed in small height boxes;
         * TODO: this correctly, using local y position! */
    ne = boxaGetCount(boxae);
    for (i = 0; i < ne; i++) {
        box = boxaGetBox(boxae, i, L_CLONE);
        n = boxaaGetCount(baa);
        boxaaAlignBox(baa, box, delta2, &index);
        if (index < n) {  /* append to an existing boxa */
            boxaaAddBox(baa, index, box, L_INSERT);
        }
        else {  /* doesn't align, need new boxa */
            boxan = boxaCreate(0);
            boxaAddBox(boxan, box, L_INSERT);
            boxaaAddBoxa(baa, boxan, L_INSERT);
            nan = numaCreate(0);
            numaaAddNuma(naa, nan, L_INSERT);
        }
        numaGetIValue(nae, i, &ival);  /* location in original boxas */
        numaaAddNumber(naa, index, ival);
    }

        /* Sort each boxa in the boxaa */
    m = boxaaGetCount(baa);
    for (i = 0; i < m; i++) {
        boxat1 = boxaaGetBoxa(baa, i, L_CLONE);
        boxat2 = boxaSort(boxat1, L_SORT_BY_X, L_SORT_INCREASING, &nah);
        boxaaReplaceBoxa(baa, i, boxat2);
        nat1 = numaaGetNuma(naa, i, L_CLONE);
        nat2 = numaSortByIndex(nat1, nah);
        numaaReplaceNuma(naa, i, nat2);
        boxaDestroy(&boxat1);
        numaDestroy(&nat1);
        numaDestroy(&nah);
    }

        /* Sort boxa vertically within boxaa, using the first box
         * in each boxa. */
    m = boxaaGetCount(baa);
    boxav = boxaCreate(m);  /* holds first box in each boxa in baa */
    naad = numaaCreate(m);
    if (pnaad)
        *pnaad = naad;
    baad = boxaaCreate(m);
    for (i = 0; i < m; i++) {
        boxat1 = boxaaGetBoxa(baa, i, L_CLONE);
        box = boxaGetBox(boxat1, 0, L_CLONE); 
        boxaAddBox(boxav, box, L_INSERT);
        boxaDestroy(&boxat1);
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
 *      (2) The four box parameters are sorted independently.  To assure
 *          that the resulting box size is sorted in increasing order:
 *             - x and y are sorted in decreasing order
 *             - w and h are sorted in increasing order
 */
BOX *
boxaGetRankSize(BOXA      *boxa,
                l_float32  fract)
{
l_int32    i, n, x, y, w, h;
l_float32  xval, yval, wval, hval;
NUMA      *nax, *nay, *naw, *nah;
BOX       *box;

    PROCNAME("boxaGetRankSize");

    if (!boxa)
        return (BOX *)ERROR_PTR("boxa not defined", procName, NULL);
    if (fract < 0.0 || fract > 1.0)
        return (BOX *)ERROR_PTR("fract not in [0.0 ... 1.0]", procName, NULL);
    if ((n = boxaGetCount(boxa)) == 0)
        return (BOX *)ERROR_PTR("boxa is empty", procName, NULL);

    nax = numaCreate(n);
    nay = numaCreate(n);
    naw = numaCreate(n);
    nah = numaCreate(n);
    for (i = 0; i < n; i++) {
        boxaGetBoxGeometry(boxa, i, &x, &y, &w, &h);
        if (w == 0 || h == 0) continue;
        numaAddNumber(nax, x);
        numaAddNumber(nay, y);
        numaAddNumber(naw, w);
        numaAddNumber(nah, h);
    }
    numaGetRankValue(nax, 1.0 - fract, &xval);
    numaGetRankValue(nay, 1.0 - fract, &yval);
    numaGetRankValue(naw, fract, &wval);
    numaGetRankValue(nah, fract, &hval);
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
 *      Input:  boxaa
 *              &w  (<optional return> width)
 *              &h  (<optional return> height)
 *              &box (<optional return>, minimum box containing all boxa
 *                    in boxaa)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The returned w and h are the minimum size image
 *          that would contain all boxes untranslated.
 */
l_int32
boxaaGetExtent(BOXAA    *boxaa,
               l_int32  *pw,
               l_int32  *ph,
               BOX     **pbox)
{
l_int32  i, j, n, m, x, y, w, h, xmax, ymax, xmin, ymin;
BOXA    *boxa;

    PROCNAME("boxaaGetExtent");

    if (!pw && !ph && !pbox)
        return ERROR_INT("no ptrs defined", procName, 1);
    if (pbox) *pbox = NULL;
    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (!boxaa)
        return ERROR_INT("boxaa not defined", procName, 1);

    n = boxaaGetCount(boxaa);
    if (n == 0)
        return ERROR_INT("no boxa in boxaa", procName, 1);

    xmax = ymax = 0;
    xmin = ymin = 100000000;
    for (i = 0; i < n; i++) {
        boxa = boxaaGetBoxa(boxaa, i, L_CLONE);
        m = boxaGetCount(boxa);
        for (j = 0; j < m; j++) {
            boxaGetBoxGeometry(boxa, j, &x, &y, &w, &h);
            xmin = L_MIN(xmin, x);
            ymin = L_MIN(ymin, y);
            xmax = L_MAX(xmax, x + w);
            ymax = L_MAX(ymax, y + h);
        }
    }
    if (pw) *pw = xmax;
    if (ph) *ph = ymax;
    if (pbox)
      *pbox = boxCreate(xmin, ymin, xmax - xmin, ymax - ymin);

    return 0;
}


/*!
 *  boxaaFlattenToBoxa()
 *
 *      Input:  boxaa
 *              &naindex  (<optional return> the boxa index in the boxaa)
 *              copyflag  (L_COPY or L_CLONE)
 *      Return: boxa, or null on error
 *
 *  Notes:
 *      (1) This 'flattens' the boxaa to a boxa, taking the boxes in
 *          order in the first boxa, then the second, etc.
 *      (2) If &naindex is defined, we generate a Numa that gives, for
 *          each box in the boxaa, the index of the boxa to which it belongs.
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
        for (j = 0; j < m; j++) {
            box = boxaGetBox(boxat, j, copyflag);
            boxaAddBox(boxa, box, L_INSERT);
            if (pnaindex)
                numaAddNumber(naindex, i);  /* save 'row' number */
        }
        boxaDestroy(&boxat);
    }

    return boxa;
}


/*!
 *  boxaaAlignBox()
 * 
 *      Input:  boxaa
 *              box (to be aligned with the last of one of the boxa
 *                   in boxaa, if possible)
 *              delta (amount by which consecutive components can miss
 *                     in overlap and still be included in the array)
 *              &index (of boxa with best overlap, or if none match,
 *                      this is the index of the next boxa to be generated)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is not greedy; it finds the boxa whose last box has
 *          the biggest overlap with the input box.
 */
l_int32
boxaaAlignBox(BOXAA    *baa,
              BOX      *box,
              l_int32   delta,
              l_int32  *pindex)
{
l_int32  i, n, m, y, yt, h, ht, ovlp, maxovlp, maxindex;
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
            L_WARNING("no boxes in boxa", procName);
            continue;
        }
        boxaGetBoxGeometry(boxa, m - 1, NULL, &yt, NULL, &ht);  /* last one */
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


