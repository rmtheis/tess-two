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
 *  rotateam.c
 *
 *     Grayscale and color rotation for area mapping (== interpolation)
 *
 *         Rotation about the image center
 *                  PIX     *pixRotateAM()
 *                  PIX     *pixRotateAMColor()
 *                  PIX     *pixRotateAMGray()
 *
 *         Rotation about the UL corner of the image
 *                  PIX     *pixRotateAMCorner()
 *                  PIX     *pixRotateAMColorCorner()
 *                  PIX     *pixRotateAMGrayCorner()
 *
 *         Faster color rotation about the image center
 *                  PIX     *pixRotateAMColorFast()
 *
 *     Rotations are measured in radians; clockwise is positive.
 *
 *     The basic area mapping grayscale rotation works on 8 bpp images.
 *     For color, the same method is applied to each color separately.
 *     This can be done in two ways: (1) as here, computing each dest
 *     rgb pixel from the appropriate four src rgb pixels, or (2) separating
 *     the color image into three 8 bpp images, rotate each of these,
 *     and then combine the result.  Method (1) is about 2.5x faster.
 *     We have also implemented a fast approximation for color area-mapping
 *     rotation (pixRotateAMColorFast()), which is about 25% faster
 *     than the standard color rotator.  If you need the extra speed,
 *     use it.
 *
 *     Area mapping works as follows.  For each dest
 *     pixel you find the 4 source pixels that it partially
 *     covers.  You then compute the dest pixel value as
 *     the area-weighted average of those 4 source pixels.
 *     We make two simplifying approximations:
 *
 *       -  For simplicity, compute the areas as if the dest
 *          pixel were translated but not rotated.
 *
 *       -  Compute area overlaps on a discrete sub-pixel grid.
 *          Because we are using 8 bpp images with 256 levels,
 *          it is convenient to break each pixel into a
 *          16x16 sub-pixel grid, and count the number of
 *          overlapped sub-pixels.
 *
 *     It is interesting to note that the digital filter that
 *     implements the area mapping algorithm for rotation
 *     is identical to the digital filter used for linear
 *     interpolation when arbitrarily scaling grayscale images.
 *
 *     The advantage of area mapping over pixel sampling
 *     in grayscale rotation is that the former naturally
 *     blurs sharp edges ("anti-aliasing"), so that stair-step
 *     artifacts are not introduced.  The disadvantage is that
 *     it is significantly slower.
 *
 *     But it is still pretty fast.  With standard 3 GHz hardware,
 *     the anti-aliased (area-mapped) color rotation speed is
 *     about 15 million pixels/sec.
 *
 *     The function pixRotateAMColorFast() is about 10-20% faster
 *     than pixRotateAMColor().  The quality is slightly worse,
 *     and if you make many successive small rotations, with a
 *     total angle of 360 degrees, it has been noted that the
 *     center wanders -- it seems to be doing a 1 pixel translation
 *     in addition to the rotation.
 */

#include <stdio.h>
#include <string.h>
#include "allheaders.h"

static const l_float32  VERY_SMALL_ANGLE = 0.001;  /* radians; ~0.06 degrees */


/*------------------------------------------------------------------*
 *                     Rotation about the center                    *
 *------------------------------------------------------------------*/
/*!
 *  pixRotateAM()
 *
 *      Input:  pixs (2, 4, 8 bpp gray or colormapped, or 32 bpp RGB)
 *              angle (radians; clockwise is positive)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Rotates about image center.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) Brings in either black or white pixels from the boundary.
 */
PIX *
pixRotateAM(PIX       *pixs,
            l_float32  angle,
            l_int32    incolor)
{
l_int32   d;
l_uint32  fillval;
PIX      *pixt1, *pixt2, *pixd;

    PROCNAME("pixRotateAM");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) == 1)
        return (PIX *)ERROR_PTR("pixs is 1 bpp", procName, NULL);

    if (L_ABS(angle) < VERY_SMALL_ANGLE)
        return pixClone(pixs);

        /* Remove cmap if it exists, and unpack to 8 bpp if necessary */
    pixt1 = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    d = pixGetDepth(pixt1);
    if (d < 8)
        pixt2 = pixConvertTo8(pixt1, FALSE);
    else
        pixt2 = pixClone(pixt1);
    d = pixGetDepth(pixt2);

        /* Compute actual incoming color */
    fillval = 0;
    if (incolor == L_BRING_IN_WHITE) {
        if (d == 8)
            fillval = 255;
        else  /* d == 32 */
            fillval = 0xffffff00;
    }

    if (d == 8)
        pixd = pixRotateAMGray(pixt2, angle, fillval);
    else   /* d == 32 */
        pixd = pixRotateAMColor(pixt2, angle, fillval);

    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    return pixd;
}


/*!
 *  pixRotateAMColor()
 *
 *      Input:  pixs (32 bpp)
 *              angle (radians; clockwise is positive)
 *              colorval (e.g., 0 to bring in BLACK, 0xffffff00 for WHITE)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Rotates about image center.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) Specify the color to be brought in from outside the image.
 */
PIX *
pixRotateAMColor(PIX       *pixs,
                 l_float32  angle,
                 l_uint32   colorval)
{
l_int32    w, h, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixRotateAMColor");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs must be 32 bpp", procName, NULL);

    if (L_ABS(angle) < VERY_SMALL_ANGLE)
        return pixClone(pixs);

    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    pixd = pixCreateTemplate(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    rotateAMColorLow(datad, w, h, wpld, datas, wpls, angle, colorval);

    return pixd;
}


/*!
 *  pixRotateAMGray()
 *
 *      Input:  pixs (8 bpp)
 *              angle (radians; clockwise is positive)
 *              grayval (0 to bring in BLACK, 255 for WHITE)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Rotates about image center.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) Specify the grayvalue to be brought in from outside the image.
 */
PIX *
pixRotateAMGray(PIX       *pixs,
                l_float32  angle,
                l_uint8    grayval)
{
l_int32    w, h, wpls, wpld;
l_uint32  *datas, *datad;
PIX        *pixd;

    PROCNAME("pixRotateAMGray");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs must be 8 bpp", procName, NULL);

    if (L_ABS(angle) < VERY_SMALL_ANGLE)
        return pixClone(pixs);

    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    pixd = pixCreateTemplate(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    rotateAMGrayLow(datad, w, h, wpld, datas, wpls, angle, grayval);

    return pixd;
}


/*------------------------------------------------------------------*
 *                    Rotation about the UL corner                  *
 *------------------------------------------------------------------*/
/*!
 *  pixRotateAMCorner()
 *
 *      Input:  pixs (1, 2, 4, 8 bpp gray or colormapped, or 32 bpp RGB)
 *              angle (radians; clockwise is positive)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Rotates about the UL corner of the image.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) Brings in either black or white pixels from the boundary.
 */
PIX *
pixRotateAMCorner(PIX       *pixs,
                  l_float32  angle,
                  l_int32    incolor)
{
l_int32   d;
l_uint32  fillval;
PIX      *pixt1, *pixt2, *pixd;

    PROCNAME("pixRotateAMCorner");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    if (L_ABS(angle) < VERY_SMALL_ANGLE)
        return pixClone(pixs);

        /* Remove cmap if it exists, and unpack to 8 bpp if necessary */
    pixt1 = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    d = pixGetDepth(pixt1);
    if (d < 8)
        pixt2 = pixConvertTo8(pixt1, FALSE);
    else
        pixt2 = pixClone(pixt1);
    d = pixGetDepth(pixt2);

        /* Compute actual incoming color */
    fillval = 0;
    if (incolor == L_BRING_IN_WHITE) {
        if (d == 8)
            fillval = 255;
        else  /* d == 32 */
            fillval = 0xffffff00;
    }

    if (d == 8)
        pixd = pixRotateAMGrayCorner(pixt2, angle, fillval);
    else   /* d == 32 */
        pixd = pixRotateAMColorCorner(pixt2, angle, fillval);

    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    return pixd;
}


/*!
 *  pixRotateAMColorCorner()
 *
 *      Input:  pixs
 *              angle (radians; clockwise is positive)
 *              colorval (e.g., 0 to bring in BLACK, 0xffffff00 for WHITE)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Rotates the image about the UL corner.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) Specify the color to be brought in from outside the image.
 */
PIX *
pixRotateAMColorCorner(PIX       *pixs,
                       l_float32  angle,
                       l_uint32   fillval)
{
l_int32    w, h, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixRotateAMColorCorner");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs must be 32 bpp", procName, NULL);

    if (L_ABS(angle) < VERY_SMALL_ANGLE)
        return pixClone(pixs);

    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    pixd = pixCreateTemplate(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    rotateAMColorCornerLow(datad, w, h, wpld, datas, wpls, angle, fillval);

    return pixd;
}


/*!
 *  pixRotateAMGrayCorner()
 *
 *      Input:  pixs
 *              angle (radians; clockwise is positive)
 *              grayval (0 to bring in BLACK, 255 for WHITE)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Rotates the image about the UL corner.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) Specify the grayvalue to be brought in from outside the image.
 */
PIX *
pixRotateAMGrayCorner(PIX       *pixs,
                      l_float32  angle,
                      l_uint8    grayval)
{
l_int32    w, h, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixRotateAMGrayCorner");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs must be 8 bpp", procName, NULL);

    if (L_ABS(angle) < VERY_SMALL_ANGLE)
        return pixClone(pixs);

    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    pixd = pixCreateTemplate(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    rotateAMGrayCornerLow(datad, w, h, wpld, datas, wpls, angle, grayval);

    return pixd;
}


/*------------------------------------------------------------------*
 *                    Fast rotation about the center                *
 *------------------------------------------------------------------*/
/*!
 *  pixRotateAMColorFast()
 *
 *      Input:  pixs
 *              angle (radians; clockwise is positive)
 *              colorval (e.g., 0 to bring in BLACK, 0xffffff00 for WHITE)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This rotates a color image about the image center.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) It uses area mapping, dividing each pixel into
 *          16 subpixels.
 *      (4) It is about 10% to 20% faster than the more accurate linear
 *          interpolation function pixRotateAMColor(),
 *          which uses 256 subpixels.
 *
 *  *** Warning: implicit assumption about RGB component ordering ***
 */
PIX *
pixRotateAMColorFast(PIX       *pixs,
                     l_float32  angle,
                     l_uint32   colorval)
{
l_int32    w, h, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixRotateAMColorFast");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs must be 32 bpp", procName, NULL);

    if (L_ABS(angle) < VERY_SMALL_ANGLE)
        return pixClone(pixs);

    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    pixd = pixCreateTemplate(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    rotateAMColorFastLow(datad, w, h, wpld, datas, wpls, angle, colorval);

    return pixd;
}

