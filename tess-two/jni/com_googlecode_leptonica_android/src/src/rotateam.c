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


/*!
 * \file rotateam.c
 * <pre>
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
 *       ~  For simplicity, compute the areas as if the dest
 *          pixel were translated but not rotated.
 *
 *       ~  Compute area overlaps on a discrete sub-pixel grid.
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
 * </pre>
 */

#include <string.h>
#include "allheaders.h"

static const l_float32  MIN_ANGLE_TO_ROTATE = 0.001;  /* radians; ~0.06 deg */


/*------------------------------------------------------------------*
 *                     Rotation about the center                    *
 *------------------------------------------------------------------*/
/*!
 * \brief   pixRotateAM()
 *
 * \param[in]    pixs 2, 4, 8 bpp gray or colormapped, or 32 bpp RGB
 * \param[in]    angle radians; clockwise is positive
 * \param[in]    incolor L_BRING_IN_WHITE, L_BRING_IN_BLACK
 * \return  pixd, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Rotates about image center.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) Brings in either black or white pixels from the boundary.
 * </pre>
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

    if (L_ABS(angle) < MIN_ANGLE_TO_ROTATE)
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
 * \brief   pixRotateAMColor()
 *
 * \param[in]    pixs 32 bpp
 * \param[in]    angle radians; clockwise is positive
 * \param[in]    colorval e.g., 0 to bring in BLACK, 0xffffff00 for WHITE
 * \return  pixd, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Rotates about image center.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) Specify the color to be brought in from outside the image.
 * </pre>
 */
PIX *
pixRotateAMColor(PIX       *pixs,
                 l_float32  angle,
                 l_uint32   colorval)
{
l_int32    w, h, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pix1, *pix2, *pixd;

    PROCNAME("pixRotateAMColor");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs must be 32 bpp", procName, NULL);

    if (L_ABS(angle) < MIN_ANGLE_TO_ROTATE)
        return pixClone(pixs);

    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    pixd = pixCreateTemplate(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    rotateAMColorLow(datad, w, h, wpld, datas, wpls, angle, colorval);
    if (pixGetSpp(pixs) == 4) {
        pix1 = pixGetRGBComponent(pixs, L_ALPHA_CHANNEL);
        pix2 = pixRotateAMGray(pix1, angle, 255);  /* bring in opaque */
        pixSetRGBComponent(pixd, pix2, L_ALPHA_CHANNEL);
        pixDestroy(&pix1);
        pixDestroy(&pix2);
    }

    return pixd;
}


/*!
 * \brief   pixRotateAMGray()
 *
 * \param[in]    pixs 8 bpp
 * \param[in]    angle radians; clockwise is positive
 * \param[in]    grayval 0 to bring in BLACK, 255 for WHITE
 * \return  pixd, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Rotates about image center.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) Specify the grayvalue to be brought in from outside the image.
 * </pre>
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

    if (L_ABS(angle) < MIN_ANGLE_TO_ROTATE)
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
 * \brief   pixRotateAMCorner()
 *
 * \param[in]    pixs 1, 2, 4, 8 bpp gray or colormapped, or 32 bpp RGB
 * \param[in]    angle radians; clockwise is positive
 * \param[in]    incolor L_BRING_IN_WHITE, L_BRING_IN_BLACK
 * \return  pixd, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Rotates about the UL corner of the image.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) Brings in either black or white pixels from the boundary.
 * </pre>
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

    if (L_ABS(angle) < MIN_ANGLE_TO_ROTATE)
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
 * \brief   pixRotateAMColorCorner()
 *
 * \param[in]    pixs
 * \param[in]    angle radians; clockwise is positive
 * \param[in]    fillval e.g., 0 to bring in BLACK, 0xffffff00 for WHITE
 * \return  pixd, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Rotates the image about the UL corner.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) Specify the color to be brought in from outside the image.
 * </pre>
 */
PIX *
pixRotateAMColorCorner(PIX       *pixs,
                       l_float32  angle,
                       l_uint32   fillval)
{
l_int32    w, h, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pix1, *pix2, *pixd;

    PROCNAME("pixRotateAMColorCorner");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs must be 32 bpp", procName, NULL);

    if (L_ABS(angle) < MIN_ANGLE_TO_ROTATE)
        return pixClone(pixs);

    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    pixd = pixCreateTemplate(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    rotateAMColorCornerLow(datad, w, h, wpld, datas, wpls, angle, fillval);
    if (pixGetSpp(pixs) == 4) {
        pix1 = pixGetRGBComponent(pixs, L_ALPHA_CHANNEL);
        pix2 = pixRotateAMGrayCorner(pix1, angle, 255);  /* bring in opaque */
        pixSetRGBComponent(pixd, pix2, L_ALPHA_CHANNEL);
        pixDestroy(&pix1);
        pixDestroy(&pix2);
    }

    return pixd;
}


/*!
 * \brief   pixRotateAMGrayCorner()
 *
 * \param[in]    pixs
 * \param[in]    angle radians; clockwise is positive
 * \param[in]    grayval 0 to bring in BLACK, 255 for WHITE
 * \return  pixd, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Rotates the image about the UL corner.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) Specify the grayvalue to be brought in from outside the image.
 * </pre>
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

    if (L_ABS(angle) < MIN_ANGLE_TO_ROTATE)
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
 * \brief   pixRotateAMColorFast()
 *
 * \param[in]    pixs
 * \param[in]    angle radians; clockwise is positive
 * \param[in]    colorval e.g., 0 to bring in BLACK, 0xffffff00 for WHITE
 * \return  pixd, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This rotates a color image about the image center.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) It uses area mapping, dividing each pixel into
 *          16 subpixels.
 *      (4) It is about 10% to 20% faster than the more accurate linear
 *          interpolation function pixRotateAMColor(),
 *          which uses 256 subpixels.
 *      (5) For some reason it shifts the image center.
 *          No attempt is made to rotate the alpha component.
 *
 *  *** Warning: implicit assumption about RGB component ordering ***
 * </pre>
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

    if (L_ABS(angle) < MIN_ANGLE_TO_ROTATE)
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
