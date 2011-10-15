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
 *  rotate.c
 *
 *     General rotation about image center
 *              PIX     *pixRotate()
 *              PIX     *pixEmbedForRotation()
 *
 *     General rotation by sampling
 *              PIX     *pixRotateBySampling()
 *
 *     Nice (slow) rotation of 1 bpp image
 *              PIX     *pixRotateBinaryNice()
 *
 *     Rotation including alpha (blend) component and gamma transform
 *              PIX     *pixRotateWithAlpha()
 *              PIX     *pixRotateGammaXform()
 *
 *     Rotations are measured in radians; clockwise is positive.
 *
 *     The general rotation pixRotate() does the best job for
 *     rotating about the image center.  For 1 bpp, it uses shear;
 *     for others, it uses either shear or area mapping.
 *     If requested, it expands the output image so that no pixels are lost
 *     in the rotation, and this can be done on multiple successive shears
 *     without expanding beyond the maximum necessary size.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allheaders.h"

extern l_float32  AlphaMaskBorderVals[2];
static const l_float32  VERY_SMALL_ANGLE = 0.001;  /* radians; ~0.06 degrees */


/*------------------------------------------------------------------*
 *                  General rotation about the center               *
 *------------------------------------------------------------------*/
/*!
 *  pixRotate()
 *
 *      Input:  pixs (1, 2, 4, 8, 32 bpp rgb)
 *              angle (radians; clockwise is positive)
 *              type (L_ROTATE_AREA_MAP, L_ROTATE_SHEAR, L_ROTATE_SAMPLING)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK)
 *              width (original width; use 0 to avoid embedding)
 *              height (original height; use 0 to avoid embedding)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Rotation is about the center of the image.
 *      (2) For very small rotations, just return a clone.
 *      (3) Rotation brings either white or black pixels in
 *          from outside the image.
 *      (4) Above 20 degrees, if rotation by shear is requested, we rotate
 *          by sampling.
 *      (5) Colormaps are removed for rotation by area map and shear.
 *      (6) The dest can be expanded so that no image pixels
 *          are lost.  To invoke expansion, input the original
 *          width and height.  For repeated rotation, use of the
 *          original width and height allows the expansion to
 *          stop at the maximum required size, which is a square 
 *          with side = sqrt(w*w + h*h).
 *
 *  *** Warning: implicit assumption about RGB component ordering ***
 */
PIX *
pixRotate(PIX       *pixs,
          l_float32  angle,
          l_int32    type,
          l_int32    incolor,
          l_int32    width,
          l_int32    height)
{
l_int32    w, h, d;
l_uint32   fillval;
PIX       *pixt1, *pixt2, *pixt3, *pixd;
PIXCMAP   *cmap;

    PROCNAME("pixRotate");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (type != L_ROTATE_SHEAR && type != L_ROTATE_AREA_MAP &&
        type != L_ROTATE_SAMPLING)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return (PIX *)ERROR_PTR("invalid incolor", procName, NULL);

    if (L_ABS(angle) < VERY_SMALL_ANGLE)
        return pixClone(pixs);

        /* Don't rotate by shear more than 20 degrees */
    if (L_ABS(angle) > 0.35 && type == L_ROTATE_SHEAR) {
        L_WARNING("large angle; rotating by sampling", procName);
        type = L_ROTATE_SAMPLING;
    }

        /* If 1 bpp and area map is requested, rotate by sampling */
    d = pixGetDepth(pixs);
    if (d == 1 && type == L_ROTATE_AREA_MAP) {
        L_WARNING("1 bpp; rotating by sampling", procName);
        type = L_ROTATE_SAMPLING;
    }

        /* Remove colormap if we're rotating by area mapping. */
    cmap = pixGetColormap(pixs);
    if (cmap && type == L_ROTATE_AREA_MAP)
        pixt1 = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    else
        pixt1 = pixClone(pixs);
    cmap = pixGetColormap(pixt1);

        /* Otherwise, if there is a colormap and we're not embedding,
         * add white color if it doesn't exist. */
    if (cmap && width == 0) {  /* no embedding; generate @incolor */
        if (incolor == L_BRING_IN_BLACK)
            pixcmapAddBlackOrWhite(cmap, 0, NULL);
        else  /* L_BRING_IN_WHITE */
            pixcmapAddBlackOrWhite(cmap, 1, NULL);
    }

        /* Request to embed in a larger image; do if necessary */
    pixt2 = pixEmbedForRotation(pixt1, angle, incolor, width, height);

        /* Area mapping requires 8 or 32 bpp.
         * If 1 bpp, default to sampling. */
    d = pixGetDepth(pixt2);
    if (type == L_ROTATE_AREA_MAP && d < 8)
        pixt3 = pixConvertTo8(pixt2, FALSE);
    else
        pixt3 = pixClone(pixt2);

        /* Rotate by shear or area mapping */
    pixGetDimensions(pixt3, &w, &h, &d);
    if (type == L_ROTATE_SHEAR)
        pixd = pixRotateShearCenter(pixt3, angle, incolor);
    else if (type == L_ROTATE_SAMPLING)
        pixd = pixRotateBySampling(pixt3, w / 2, h / 2, angle, incolor);
    else {  /* rotate by area mapping */
        fillval = 0;
        if (incolor == L_BRING_IN_WHITE) {
            if (d == 8)
                fillval = 255;
            else  /* d == 32 */
                fillval = 0xffffff00;
        }
        if (d == 8)
            pixd = pixRotateAMGray(pixt3, angle, fillval);
        else   /* d == 32 */
            pixd = pixRotateAMColor(pixt3, angle, fillval);
    }

    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    return pixd;
}


/*!
 *  pixEmbedForRotation()
 *
 *      Input:  pixs (1, 2, 4, 8, 32 bpp rgb)
 *              angle (radians; clockwise is positive)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK)
 *              width (original width; use 0 to avoid embedding)
 *              height (original height; use 0 to avoid embedding)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) For very small rotations, just return a clone.
 *      (2) Generate larger image to embed pixs if necessary, and
 *          place in the center.
 *      (3) Rotation brings either white or black pixels in
 *          from outside the image.  For colormapped images where
 *          there is no white or black, a new color is added if
 *          possible for these pixels; otherwise, either the
 *          lightest or darkest color is used.  In most cases,
 *          the colormap will be removed prior to rotation.
 *      (4) The dest is to be expanded so that no image pixels
 *          are lost after rotation.  Input of the original width
 *          and height allows the expansion to stop at the maximum
 *          required size, which is a square with side equal to
 *          sqrt(w*w + h*h).
 *      (5) For an arbitrary angle, the expansion can be found by
 *          considering the UL and UR corners.  As the image is
 *          rotated, these move in an arc centered at the center of
 *          the image.  Normalize to a unit circle by dividing by half
 *          the image diagonal.  After a rotation of T radians, the UL
 *          and UR corners are at points T radians along the unit
 *          circle.  Compute the x and y coordinates of both these
 *          points and take the max of absolute values; these represent
 *          the half width and half height of the containing rectangle.
 *          The arithmetic is done using formulas for sin(a+b) and cos(a+b),
 *          where b = T.  For the UR corner, sin(a) = h/d and cos(a) = w/d.
 *          For the UL corner, replace a by (pi - a), and you have
 *          sin(pi - a) = h/d, cos(pi - a) = -w/d.  The equations
 *          given below follow directly.
 */
PIX *
pixEmbedForRotation(PIX       *pixs,
                    l_float32  angle,
                    l_int32    incolor,
                    l_int32    width,
                    l_int32    height)
{
l_int32    w, h, d, w1, h1, w2, h2, maxside, wnew, hnew, xoff, yoff;
l_float64  sina, cosa, fw, fh;
PIX       *pixd;

    PROCNAME("pixEmbedForRotation");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return (PIX *)ERROR_PTR("invalid incolor", procName, NULL);
    if (L_ABS(angle) < VERY_SMALL_ANGLE)
        return pixClone(pixs);

        /* Test if big enough to hold any rotation of the original image */
    pixGetDimensions(pixs, &w, &h, &d);
    maxside = (l_int32)(sqrt((l_float64)(width * width) +
                             (l_float64)(height * height)) + 0.5);
    if (w >= maxside && h >= maxside)  /* big enough */
        return pixClone(pixs);
    
        /* Find the new sizes required to hold the image after rotation */
    cosa = cos(angle);
    sina = sin(angle);
    fw = (l_float64)w;
    fh = (l_float64)h;
    w1 = (l_int32)L_ABS(fw * cosa - fh * sina);
    w2 = (l_int32)L_ABS(-fw * cosa - fh * sina);
    h1 = (l_int32)L_ABS(fw * sina + fh * cosa);
    h2 = (l_int32)L_ABS(-fw * sina + fh * cosa);
    wnew = L_MAX(w1, w2);
    hnew = L_MAX(h1, h2);

    if ((pixd = pixCreate(wnew, hnew, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixCopyColormap(pixd, pixs);
    pixCopyText(pixd, pixs);
    xoff = (wnew - w) / 2;
    yoff = (hnew - h) / 2;

        /* Set background to color to be rotated in */
    pixSetBlackOrWhite(pixd, incolor);

    pixRasterop(pixd, xoff, yoff, w, h, PIX_SRC, pixs, 0, 0);
    return pixd;
}


/*------------------------------------------------------------------*
 *                    General rotation by sampling                  *
 *------------------------------------------------------------------*/
/*!
 *  pixRotateBySampling()
 *
 *      Input:  pixs (1, 2, 4, 8, 16, 32 bpp rgb; can be cmapped)
 *              xcen (x value of center of rotation)
 *              ycen (y value of center of rotation)
 *              angle (radians; clockwise is positive)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) For very small rotations, just return a clone.
 *      (2) Rotation brings either white or black pixels in
 *          from outside the image.
 *      (3) Colormaps are retained.
 */
PIX *
pixRotateBySampling(PIX       *pixs,
                    l_int32    xcen,
                    l_int32    ycen,
                    l_float32  angle,
                    l_int32    incolor)
{
l_int32    w, h, d, i, j, x, y, xdif, ydif, wm1, hm1, wpld;
l_uint32   val;
l_float32  sina, cosa;
l_uint32  *datad, *lined;
void     **lines;
PIX       *pixd;

    PROCNAME("pixRotateBySampling");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return (PIX *)ERROR_PTR("invalid incolor", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("invalid depth", procName, NULL);

    if (L_ABS(angle) < VERY_SMALL_ANGLE)
        return pixClone(pixs);

    if ((pixd = pixCreateTemplateNoInit(pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixSetBlackOrWhite(pixd, incolor);

    sina = sin(angle);
    cosa = cos(angle);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    wm1 = w - 1;
    hm1 = h - 1;
    lines = pixGetLinePtrs(pixs, NULL);

        /* Treat 1 bpp case specially */
    if (d == 1) {
        for (i = 0; i < h; i++) {  /* scan over pixd */
            lined = datad + i * wpld;
            ydif = ycen - i;
            for (j = 0; j < w; j++) {
                xdif = xcen - j;
                x = xcen + (l_int32)(-xdif * cosa - ydif * sina);
                if (x < 0 || x > wm1) continue;
                y = ycen + (l_int32)(-ydif * cosa + xdif * sina);
                if (y < 0 || y > hm1) continue;
                if (incolor == L_BRING_IN_WHITE) {
                    if (GET_DATA_BIT(lines[y], x))
                        SET_DATA_BIT(lined, j);
                }
                else {
                    if (!GET_DATA_BIT(lines[y], x))
                        CLEAR_DATA_BIT(lined, j);
                }
            }
        }
        FREE(lines);
        return pixd;
    }

    for (i = 0; i < h; i++) {  /* scan over pixd */
        lined = datad + i * wpld;
        ydif = ycen - i;
        for (j = 0; j < w; j++) {
            xdif = xcen - j;
            x = xcen + (l_int32)(-xdif * cosa - ydif * sina);
            if (x < 0 || x > wm1) continue;
            y = ycen + (l_int32)(-ydif * cosa + xdif * sina);
            if (y < 0 || y > hm1) continue;
            switch (d)
            {
            case 8:
                val = GET_DATA_BYTE(lines[y], x);
                SET_DATA_BYTE(lined, j, val);
                break;
            case 32:
                val = GET_DATA_FOUR_BYTES(lines[y], x);
                SET_DATA_FOUR_BYTES(lined, j, val);
                break;
            case 2:
                val = GET_DATA_DIBIT(lines[y], x);
                SET_DATA_DIBIT(lined, j, val);
                break;
            case 4:
                val = GET_DATA_QBIT(lines[y], x);
                SET_DATA_QBIT(lined, j, val);
                break;
            case 16:
                val = GET_DATA_TWO_BYTES(lines[y], x);
                SET_DATA_TWO_BYTES(lined, j, val);
                break;
            default:
                return (PIX *)ERROR_PTR("invalid depth", procName, NULL);
            }
        }
    }

    FREE(lines);
    return pixd;
}


/*------------------------------------------------------------------*
 *                 Nice (slow) rotation of 1 bpp image              *
 *------------------------------------------------------------------*/
/*!
 *  pixRotateBinaryNice()
 *
 *      Input:  pixs (1 bpp)
 *              angle (radians; clockwise is positive; about the center)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) For very small rotations, just return a clone.
 *      (2) This does a computationally expensive rotation of 1 bpp images.
 *          The fastest rotators (using shears or subsampling) leave
 *          visible horizontal and vertical shear lines across which
 *          the image shear changes by one pixel.  To ameliorate the
 *          visual effect one can introduce random dithering.  One
 *          way to do this in a not-too-random fashion is given here.
 *          We convert to 8 bpp, do a very small blur, rotate using
 *          linear interpolation (same as area mapping), do a
 *          small amount of sharpening to compensate for the initial
 *          blur, and threshold back to binary.  The shear lines
 *          are magically removed.
 *      (3) This operation is about 5x slower than rotation by sampling.
 */
PIX *
pixRotateBinaryNice(PIX       *pixs,
                    l_float32  angle,
                    l_int32    incolor)
{
PIX  *pixt1, *pixt2, *pixt3, *pixt4, *pixd;

    PROCNAME("pixRotateBinaryNice");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return (PIX *)ERROR_PTR("invalid incolor", procName, NULL);

    pixt1 = pixConvertTo8(pixs, 0);
    pixt2 = pixBlockconv(pixt1, 1, 1);  /* smallest blur allowed */
    pixt3 = pixRotateAM(pixt2, angle, incolor);
    pixt4 = pixUnsharpMasking(pixt3, 1, 1.0);  /* sharpen a bit */
    pixd = pixThresholdToBinary(pixt4, 128);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    return pixd;
}


/*------------------------------------------------------------------*
 *             Rotation including alpha (blend) component           *
 *------------------------------------------------------------------*/
/*!
 *  pixRotateWithAlpha()
 *
 *      Input:  pixs (32 bpp rgb)
 *              angle (radians; clockwise is positive)
 *              pixg (<optional> 8 bpp, can be null)
 *              fract (between 0.0 and 1.0, with 0.0 fully transparent
 *                     and 1.0 fully opaque)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) The alpha channel is transformed separately from pixs,
 *          and aligns with it, being fully transparent outside the
 *          boundary of the transformed pixs.  For pixels that are fully
 *          transparent, a blending function like pixBlendWithGrayMask()
 *          will give zero weight to corresponding pixels in pixs.
 *      (2) Rotation is about the center of the image; for very small
 *          rotations, just return a clone.  The dest is automatically
 *          expanded so that no image pixels are lost.
 *      (3) Rotation is by area mapping.  It doesn't matter what
 *          color is brought in because the alpha channel will
 *          be transparent (black) there.
 *      (4) If pixg is NULL, it is generated as an alpha layer that is
 *          partially opaque, using @fract.  Otherwise, it is cropped
 *          to pixs if required and @fract is ignored.  The alpha
 *          channel in pixs is never used.
 *      (4) Colormaps are removed.
 *      (5) The default setting for the border values in the alpha channel
 *          is 0 (transparent) for the outermost ring of pixels and
 *          (0.5 * fract * 255) for the second ring.  When blended over
 *          a second image, this
 *          (a) shrinks the visible image to make a clean overlap edge
 *              with an image below, and
 *          (b) softens the edges by weakening the aliasing there.
 *          Use l_setAlphaMaskBorder() to change these values.
 *
 *  *** Warning: implicit assumption about RGB component ordering ***
 */
PIX *
pixRotateWithAlpha(PIX       *pixs,
                   l_float32  angle,
                   PIX       *pixg,
                   l_float32  fract)
{
l_int32  ws, hs, d;
PIX     *pixd, *pixg2, *pixgr;

    PROCNAME("pixRotateWithAlpha");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &ws, &hs, &d);
    if (d != 32 && pixGetColormap(pixs) == NULL)
        return (PIX *)ERROR_PTR("pixs not cmapped or 32 bpp", procName, NULL);
    if (pixg && pixGetDepth(pixg) != 8) {
        L_WARNING("pixg not 8 bpp; using @fract transparent alpha", procName);
        pixg = NULL;
    }
    if (!pixg && (fract < 0.0 || fract > 1.0)) {
        L_WARNING("invalid fract; using 1.0 (fully transparent)", procName);
        fract = 1.0;
    }
    if (!pixg && fract == 0.0)
        L_WARNING("fully opaque alpha; image cannot be blended", procName);

        /* Do separate rotation of rgb channels of pixs and of pixg */
    pixd = pixRotate(pixs, angle, L_ROTATE_AREA_MAP, L_BRING_IN_WHITE, ws, hs);
    if (!pixg) {
        pixg2 = pixCreate(ws, hs, 8);
        if (fract == 1.0)
            pixSetAll(pixg2);
        else
            pixSetAllArbitrary(pixg2, (l_int32)(255.0 * fract));
    }
    else
        pixg2 = pixResizeToMatch(pixg, NULL, ws, hs);
    if (ws > 10 && hs > 10) {  /* see note 8 */
        pixSetBorderRingVal(pixg2, 1,
                            (l_int32)(255.0 * fract * AlphaMaskBorderVals[0]));
        pixSetBorderRingVal(pixg2, 2,
                            (l_int32)(255.0 * fract * AlphaMaskBorderVals[1]));
    }
    pixgr = pixRotate(pixg2, angle, L_ROTATE_AREA_MAP,
                      L_BRING_IN_BLACK, ws, hs);
    pixSetRGBComponent(pixd, pixgr, L_ALPHA_CHANNEL);

    pixDestroy(&pixg2);
    pixDestroy(&pixgr);
    return pixd;
}


/*!
 *  pixRotateGammaXform()
 *
 *      Input:  pixs (32 bpp rgb)
 *              gamma (gamma correction; must be > 0.0)
 *              angle (radians; clockwise is positive)
 *              fract (between 0.0 and 1.0, with 1.0 fully transparent)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This wraps a gamma/inverse-gamma photometric transform
 *          around pixRotateWithAlpha().
 *      (2) For usage, see notes in pixRotateWithAlpha() and
 *          pixGammaTRCWithAlpha().
 *      (3) The basic idea of a gamma/inverse-gamma transform is
 *          to remove gamma correction before rotating and restore
 *          it afterward.  The effects can be subtle, but important for
 *          some applications.  For example, using gamma > 1.0 will
 *          cause the dark areas to become somewhat lighter and slightly
 *          reduce aliasing effects when blending using the alpha channel.
 */
PIX *
pixRotateGammaXform(PIX       *pixs,
                    l_float32  gamma,
                    l_float32  angle,
                    l_float32  fract)
{
PIX  *pixg, *pixd;

    PROCNAME("pixRotateGammaXform");

    if (!pixs || (pixGetDepth(pixs) != 32))
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);
    if (fract == 0.0)
        L_WARNING("fully opaque alpha; image cannot be blended", procName);
    if (gamma <= 0.0)  {
        L_WARNING("gamma must be > 0.0; setting to 1.0", procName);
        gamma = 1.0;
    }

    pixg = pixGammaTRCWithAlpha(NULL, pixs, 1.0 / gamma, 0, 255);
    pixd = pixRotateWithAlpha(pixg, angle, NULL, fract);
    pixGammaTRCWithAlpha(pixd, pixd, gamma, 0, 255);
    pixDestroy(&pixg);
    return pixd;
}

