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
 *  blend.c
 *
 *      Blending two images that are not colormapped
 *           PIX             *pixBlend()
 *           PIX             *pixBlendMask()
 *           PIX             *pixBlendGray()
 *           PIX             *pixBlendColor()
 *           PIX             *pixBlendColorByChannel()
 *           PIX             *pixBlendGrayAdapt()
 *           static l_int32   blendComponents()
 *           PIX             *pixFadeWithGray()
 *           PIX             *pixBlendHardLight()
 *           static l_int32   blendHardLightComponents()
 *
 *      Blending two colormapped images
 *           l_int32          pixBlendCmap()
 *
 *      Blending two images using a third (alpha mask)
 *           PIX             *pixBlendWithGrayMask()
 *
 *      Coloring "gray" pixels
 *           l_int32          pixColorGray()
 *
 *      Adjusting one or more colors to a target color
 *           PIX             *pixSnapColor()
 *           PIX             *pixSnapColorCmap()
 *
 *      Mapping colors based on a source/target pair
 *           PIX             *pixLinearMapToTargetColor()
 *           l_uint32         pixelLinearMapToTargetColor()
 *
 *      Fractional shift of RGB towards black or white
 *           l_uint32         pixelFractionalShift()
 *
 *  In blending operations a new pix is produced where typically
 *  a subset of pixels in src1 are changed by the set of pixels
 *  in src2, when src2 is located in a given position relative
 *  to src1.  This is similar to rasterop, except that the
 *  blending operations we allow are more complex, and typically
 *  result in dest pixels that are a linear combination of two
 *  pixels, such as src1 and its inverse.  I find it convenient
 *  to think of src2 as the "blender" (the one that takes the action)
 *  and src1 as the "blendee" (the one that changes).
 *
 *  Blending works best when src1 is 8 or 32 bpp.  We also allow
 *  src1 to be colormapped, but the colormap is removed before blending,
 *  so if src1 is colormapped, we can't allow in-place blending.
 *
 *  Because src2 is typically smaller than src1, we can implement by
 *  clipping src2 to src1 and then transforming some of the dest
 *  pixels that are under the support of src2.  In practice, we
 *  do the clipping in the inner pixel loop.  For grayscale and
 *  color src2, we also allow a simple form of transparency, where
 *  pixels of a particular value in src2 are transparent; for those pixels,
 *  no blending is done.
 *
 *  The blending functions are categorized by the depth of src2,
 *  the blender, and not that of src1, the blendee.
 *
 *   - If src2 is 1 bpp, we can do one of three things:
 *     (1) L_BLEND_WITH_INVERSE: Blend a given fraction of src1 with its
 *         inverse color for those pixels in src2 that are fg (ON),
 *         and leave the dest pixels unchanged for pixels in src2 that
 *         are bg (OFF).
 *     (2) L_BLEND_TO_WHITE: Fade the src1 pixels toward white by a
 *         given fraction for those pixels in src2 that are fg (ON),
 *         and leave the dest pixels unchanged for pixels in src2 that
 *         are bg (OFF).
 *     (3) L_BLEND_TO_BLACK: Fade the src1 pixels toward black by a
 *         given fraction for those pixels in src2 that are fg (ON),
 *         and leave the dest pixels unchanged for pixels in src2 that
 *         are bg (OFF).
 *     The blending function is pixBlendMask().
 *
 *   - If src2 is 8 bpp grayscale, we can do one of two things
 *     (but see pixFadeWithGray() below):
 *     (1) L_BLEND_GRAY: If src1 is 8 bpp, mix the two values, using
 *         a fraction of src2 and (1 - fraction) of src1.
 *         If src1 is 32 bpp (rgb), mix the fraction of src2 with
 *         each of the color components in src1.
 *     (2) L_BLEND_GRAY_WITH_INVERSE: Use the grayscale value in src2
 *         to determine how much of the inverse of a src1 pixel is
 *         to be combined with the pixel value.  The input fraction
 *         further acts to scale the change in the src1 pixel.
 *     The blending function is pixBlendGray().
 *
 *   - If src2 is color, we blend a given fraction of src2 with
 *     src1.  If src1 is 8 bpp, the resulting image is 32 bpp.
 *     The blending function is pixBlendColor().
 *
 *   - For all three blending functions -- pixBlendMask(), pixBlendGray()
 *     and pixBlendColor() -- you can apply the blender to the blendee
 *     either in-place or generating a new pix.  For the in-place
 *     operation, this requires that the depth of the resulting pix
 *     must equal that of the input pixs1.
 *
 *   - We remove colormaps from src1 and src2 before blending.
 *     Any quantization would have to be done after blending.
 *
 *  We include another function, pixFadeWithGray(), that blends
 *  a gray or color src1 with a gray src2.  It does one of these things:
 *     (1) L_BLEND_TO_WHITE: Fade the src1 pixels toward white by
 *         a number times the value in src2.
 *     (2) L_BLEND_TO_BLACK: Fade the src1 pixels toward black by
 *         a number times the value in src2.
 *
 *  Also included is a generalization of the so-called "hard light"
 *  blending: pixBlendHardLight().  We generalize by allowing a fraction < 1.0
 *  of the blender to be admixed with the blendee.  The standard function
 *  does full mixing.
 */


#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static l_int32 blendComponents(l_int32 a, l_int32 b, l_float32 fract);
static l_int32 blendHardLightComponents(l_int32 a, l_int32 b, l_float32 fract);



/*-------------------------------------------------------------*
 *                         Pixel blending                      *
 *-------------------------------------------------------------*/
/*!
 *  pixBlend()
 *
 *      Input:  pixs1 (blendee)
 *              pixs2 (blender; typ. smaller)
 *              x,y  (origin (UL corner) of pixs2 relative to
 *                    the origin of pixs1; can be < 0)
 *              fract (blending fraction)
 *      Return: pixd (blended image), or null on error
 *
 *  Notes:
 *      (1) This is a simple top-level interface.  For more flexibility,
 *          call directly into pixBlendMask(), etc.
 */
PIX *
pixBlend(PIX       *pixs1,
         PIX       *pixs2,
         l_int32    x,
         l_int32    y,
         l_float32  fract)
{
l_int32    w1, h1, d1, d2;
BOX       *box;
PIX       *pixc, *pixt, *pixd;

    PROCNAME("pixBlend");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, NULL);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, NULL);

        /* check relative depths */
    d1 = pixGetDepth(pixs1);
    d2 = pixGetDepth(pixs2);
    if (d1 == 1 && d2 > 1)
        return (PIX *)ERROR_PTR("mixing gray or color with 1 bpp",
                                procName, NULL);

        /* remove colormap from pixs2 if necessary */
    pixt = pixRemoveColormap(pixs2, REMOVE_CMAP_BASED_ON_SRC);
    d2 = pixGetDepth(pixt);

        /* Check if pixs2 is clipped by its position with respect
         * to pixs1; if so, clip it and redefine x and y if necessary.
         * This actually isn't necessary, as the specific blending
         * functions do the clipping directly in the pixel loop
         * over pixs2, but it's included here to show how it can
         * easily be done on pixs2 first. */
    pixGetDimensions(pixs1, &w1, &h1, NULL);
    box = boxCreate(-x, -y, w1, h1);  /* box of pixs1 relative to pixs2 */
    pixc = pixClipRectangle(pixt, box, NULL);
    boxDestroy(&box);
    if (!pixc) {
        L_WARNING("box doesn't overlap pix", procName);
        return NULL;
    }
    x = L_MAX(0, x);
    y = L_MAX(0, y);

    if (d2 == 1)
        pixd = pixBlendMask(NULL, pixs1, pixc, x, y, fract,
                            L_BLEND_WITH_INVERSE);
    else if (d2 == 8)
        pixd = pixBlendGray(NULL, pixs1, pixc, x, y, fract,
                            L_BLEND_GRAY, 0, 0);
    else  /* d2 == 32 */
        pixd = pixBlendColor(NULL, pixs1, pixc, x, y, fract, 0, 0);

    pixDestroy(&pixc);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixBlendMask()
 *
 *      Input:  pixd (<optional>; either NULL or equal to pixs1 for in-place)
 *              pixs1 (blendee; depth > 1)
 *              pixs2 (blender; typ. smaller in size than pixs1)
 *              x,y  (origin (UL corner) of pixs2 relative to
 *                    the origin of pixs1; can be < 0)
 *              fract (blending fraction)
 *              type (L_BLEND_WITH_INVERSE, L_BLEND_TO_WHITE, L_BLEND_TO_BLACK)
 *      Return: pixd if OK; pixs1 on error
 *
 *  Notes:
 *      (1) pixs2 must be 1 bpp
 *      (2) Clipping of pixs2 to pixs1 is done in the inner pixel loop.
 *      (3) If pixs1 has a colormap, it is removed.
 *      (4) For inplace operation, call it this way:
 *            pixBlendMask(pixs1, pixs1, pixs2, ...)
 *      (5) For generating a new pixd:
 *            pixd = pixBlendMask(NULL, pixs1, pixs2, ...)
 *      (6) Only call in-place if pixs1 does not have a colormap.
 */
PIX *
pixBlendMask(PIX       *pixd,
             PIX       *pixs1,
             PIX       *pixs2,
             l_int32    x,
             l_int32    y,
             l_float32  fract,
             l_int32    type)
{
l_int32    i, j, d, wc, hc, w, h, wplc;
l_int32    val, rval, gval, bval;
l_uint32   pixval;
l_uint32  *linec, *datac;
PIX       *pixc, *pixt1, *pixt2;

    PROCNAME("pixBlendMask");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, pixd);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, pixd);
    if (pixGetDepth(pixs1) == 1)
        return (PIX *)ERROR_PTR("pixs1 is 1 bpp", procName, pixd);
    if (pixGetDepth(pixs2) != 1)
        return (PIX *)ERROR_PTR("pixs2 not 1 bpp", procName, pixd);
    if (pixd == pixs1 && pixGetColormap(pixs1))
        return (PIX *)ERROR_PTR("inplace; pixs1 has colormap", procName, pixd);
    if (pixd && (pixd != pixs1))
        return (PIX *)ERROR_PTR("pixd must be NULL or pixs1", procName, pixd);
    if (fract < 0.0 || fract > 1.0) {
        L_WARNING("fract must be in [0.0, 1.0]; setting to 0.5", procName);
        fract = 0.5;
    }
    if (type != L_BLEND_WITH_INVERSE && type != L_BLEND_TO_WHITE &&
        type != L_BLEND_TO_BLACK) {
        L_WARNING("invalid blend type; setting to L_BLEND_WITH_INVERSE",
                  procName);
        type = L_BLEND_WITH_INVERSE;
    }


        /* If pixd != NULL, we know that it is equal to pixs1 and
         * that pixs1 does not have a colormap, so that an in-place operation
         * can be done.  Otherwise, remove colormap from pixs1 if
         * it exists and unpack to at least 8 bpp if necessary,
         * to do the blending on a new pix. */
    if (!pixd) {
        pixt1 = pixRemoveColormap(pixs1, REMOVE_CMAP_BASED_ON_SRC);
        if (pixGetDepth(pixt1) < 8)
            pixt2 = pixConvertTo8(pixt1, FALSE);
        else
            pixt2 = pixClone(pixt1);
        pixd = pixCopy(NULL, pixt2);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }

    pixGetDimensions(pixd, &w, &h, &d);  /* d must be either 8 or 32 bpp */
    pixc = pixClone(pixs2);
    wc = pixGetWidth(pixc);
    hc = pixGetHeight(pixc);
    datac = pixGetData(pixc);
    wplc = pixGetWpl(pixc);

        /* Check limits for src1, in case clipping was not done. */
    switch (type)
    {
    case L_BLEND_WITH_INVERSE:
            /*
             * The core logic for this blending is:
             *      p -->  (1 - f) * p + f * (1 - p)
             * where p is a normalized value: p = pixval / 255.
             * Thus,
             *      p -->  p + f * (1 - 2 * p)
             */
        for (i = 0; i < hc; i++) {
            if (i + y < 0  || i + y >= h) continue;
            linec = datac + i * wplc;
            for (j = 0; j < wc; j++) {
                if (j + x < 0  || j + x >= w) continue;
                bval = GET_DATA_BIT(linec, j);
                if (bval) {
                    switch (d)
                    {
                    case 8:
                        pixGetPixel(pixd, x + j, y + i, &pixval);
                        val = (l_int32)(pixval + fract * (255 - 2 * pixval));
                        pixSetPixel(pixd, x + j, y + i, val);
                        break;
                    case 32:
                        pixGetPixel(pixd, x + j, y + i, &pixval);
                        extractRGBValues(pixval, &rval, &gval, &bval);
                        rval = (l_int32)(rval + fract * (255 - 2 * rval));
                        gval = (l_int32)(gval + fract * (255 - 2 * gval));
                        bval = (l_int32)(bval + fract * (255 - 2 * bval));
                        composeRGBPixel(rval, gval, bval, &pixval);
                        pixSetPixel(pixd, x + j, y + i, pixval);
                        break;
                    default:
                        L_WARNING("d neither 8 nor 32 bpp; no blend", procName);
                    }
                }
            }
        }
        break;
    case L_BLEND_TO_WHITE:
        for (i = 0; i < hc; i++) {
            if (i + y < 0  || i + y >= h) continue;
            linec = datac + i * wplc;
            for (j = 0; j < wc; j++) {
                if (j + x < 0  || j + x >= w) continue;
                bval = GET_DATA_BIT(linec, j);
                if (bval) {
                    switch (d)
                    {
                    case 8:
                        pixGetPixel(pixd, x + j, y + i, &pixval);
                        val = (l_int32)(pixval + fract * (255 - pixval));
                        pixSetPixel(pixd, x + j, y + i, val);
                        break;
                    case 32:
                        pixGetPixel(pixd, x + j, y + i, &pixval);
                        extractRGBValues(pixval, &rval, &gval, &bval);
                        rval = (l_int32)(rval + fract * (255 - rval));
                        gval = (l_int32)(gval + fract * (255 - gval));
                        bval = (l_int32)(bval + fract * (255 - bval));
                        composeRGBPixel(rval, gval, bval, &pixval);
                        pixSetPixel(pixd, x + j, y + i, pixval);
                        break;
                    default:
                        L_WARNING("d neither 8 nor 32 bpp; no blend", procName);
                    }
                }
            }
        }
        break;
    case L_BLEND_TO_BLACK:
        for (i = 0; i < hc; i++) {
            if (i + y < 0  || i + y >= h) continue;
            linec = datac + i * wplc;
            for (j = 0; j < wc; j++) {
                if (j + x < 0  || j + x >= w) continue;
                bval = GET_DATA_BIT(linec, j);
                if (bval) {
                    switch (d)
                    {
                    case 8:
                        pixGetPixel(pixd, x + j, y + i, &pixval);
                        val = (l_int32)((1. - fract) * pixval);
                        pixSetPixel(pixd, x + j, y + i, val);
                        break;
                    case 32:
                        pixGetPixel(pixd, x + j, y + i, &pixval);
                        extractRGBValues(pixval, &rval, &gval, &bval);
                        rval = (l_int32)((1. - fract) * rval);
                        gval = (l_int32)((1. - fract) * gval);
                        bval = (l_int32)((1. - fract) * bval);
                        composeRGBPixel(rval, gval, bval, &pixval);
                        pixSetPixel(pixd, x + j, y + i, pixval);
                        break;
                    default:
                        L_WARNING("d neither 8 nor 32 bpp; no blend", procName);
                    }
                }
            }
        }
        break;
    default:
        L_WARNING("invalid binary mask blend type", procName);
        break;
    }

    pixDestroy(&pixc);
    return pixd;
}


/*!
 *  pixBlendGray()
 *
 *      Input:  pixd (<optional>; either NULL or equal to pixs1 for in-place)
 *              pixs1 (blendee; depth > 1)
 *              pixs2 (blender, 8 bpp; typ. smaller in size than pixs1)
 *              x,y  (origin (UL corner) of pixs2 relative to
 *                    the origin of pixs1; can be < 0)
 *              fract (blending fraction)
 *              type (L_BLEND_GRAY, L_BLEND_GRAY_WITH_INVERSE)
 *              transparent (1 to use transparency; 0 otherwise)
 *              transpix (pixel grayval in pixs2 that is to be transparent)
 *      Return: pixd if OK; pixs1 on error
 *
 *  Notes:
 *      (1) pixs2 must be 8 bpp, and have no colormap.
 *      (2) Clipping of pixs2 to pixs1 is done in the inner pixel loop.
 *      (3) If pixs1 has a colormap, it is removed.
 *      (4) If pixs1 has depth < 8, it is unpacked to generate a 8 bpp pix.
 *      (5) For inplace operation, call it this way:
 *            pixBlendGray(pixs1, pixs1, pixs2, ...)
 *      (6) For generating a new pixd:
 *            pixd = pixBlendGray(NULL, pixs1, pixs2, ...)
 *      (7) Only call in-place if pixs1 does not have a colormap;
 *          otherwise it is an error.
 *      (8) If transparent = 0, the blending fraction (fract) is
 *          applied equally to all pixels.
 *      (9) If transparent = 1, all pixels of value transpix (typically
 *          either 0 or 0xff) in pixs2 are transparent in the blend.
 *      (10) After processing pixs1, it is either 8 bpp or 32 bpp:
 *          - if 8 bpp, the fraction of pixs2 is mixed with pixs1.
 *          - if 32 bpp, each component of pixs1 is mixed with
 *            the same fraction of pixs2.
 *      (11) For L_BLEND_GRAY_WITH_INVERSE, the white values of the blendee
 *           (cval == 255 in the code below) result in a delta of 0.
 *           Thus, these pixels are intrinsically transparent!
 *           The "pivot" value of the src, at which no blending occurs, is
 *           128.  Compare with the adaptive pivot in pixBlendGrayAdapt().
 */
PIX *
pixBlendGray(PIX       *pixd,
             PIX       *pixs1,
             PIX       *pixs2,
             l_int32    x,
             l_int32    y,
             l_float32  fract,
             l_int32    type,
             l_int32    transparent,
             l_uint32   transpix)
{
l_int32    i, j, d, wc, hc, w, h, wplc, wpld, delta;
l_int32    ival, irval, igval, ibval, cval, dval;
l_uint32   val32;
l_uint32  *linec, *lined, *datac, *datad;
PIX       *pixc, *pixt1, *pixt2;

    PROCNAME("pixBlendGray");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, pixd);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, pixd);
    if (pixGetDepth(pixs1) == 1)
        return (PIX *)ERROR_PTR("pixs1 is 1 bpp", procName, pixd);
    if (pixGetDepth(pixs2) != 8)
        return (PIX *)ERROR_PTR("pixs2 not 8 bpp", procName, pixd);
    if (pixGetColormap(pixs2))
        return (PIX *)ERROR_PTR("pixs2 has a colormap", procName, pixd);
    if (pixd == pixs1 && pixGetColormap(pixs1))
        return (PIX *)ERROR_PTR("can't do in-place with cmap", procName, pixd);
    if (pixd && (pixd != pixs1))
        return (PIX *)ERROR_PTR("pixd must be NULL or pixs1", procName, pixd);
    if (fract < 0.0 || fract > 1.0) {
        L_WARNING("fract must be in [0.0, 1.0]; setting to 0.5", procName);
        fract = 0.5;
    }
    if (type != L_BLEND_GRAY && type != L_BLEND_GRAY_WITH_INVERSE) {
        L_WARNING("invalid blend type; setting to L_BLEND_GRAY", procName);
        type = L_BLEND_GRAY;
    }

        /* If pixd != NULL, we know that it is equal to pixs1 and
         * that pixs1 does not have a colormap, so that an in-place operation
         * can be done.  Otherwise, remove colormap from pixs1 if
         * it exists and unpack to at least 8 bpp if necessary,
         * to do the blending on a new pix. */
    if (!pixd) {
        pixt1 = pixRemoveColormap(pixs1, REMOVE_CMAP_BASED_ON_SRC);
        if (pixGetDepth(pixt1) < 8)
            pixt2 = pixConvertTo8(pixt1, FALSE);
        else
            pixt2 = pixClone(pixt1);
        pixd = pixCopy(NULL, pixt2);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }

    pixGetDimensions(pixd, &w, &h, &d);  /* 8 or 32 bpp */
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);
    pixc = pixClone(pixs2);
    pixGetDimensions(pixc, &wc, &hc, NULL);
    datac = pixGetData(pixc);
    wplc = pixGetWpl(pixc);

        /* check limits for src1, in case clipping was not done */
    if (type == L_BLEND_GRAY) {
        for (i = 0; i < hc; i++) {
            if (i + y < 0  || i + y >= h) continue;
            linec = datac + i * wplc;
            lined = datad + (i + y) * wpld;
            switch (d)
            {
            case 8:
                for (j = 0; j < wc; j++) {
                    if (j + x < 0  || j + x >= w) continue;
                    cval = GET_DATA_BYTE(linec, j);
                    if (transparent == 0 ||
                        (transparent != 0 && cval != transpix)) {
                        dval = GET_DATA_BYTE(lined, j + x);
                        ival = (l_int32)((1. - fract) * dval + fract * cval);
                        SET_DATA_BYTE(lined, j + x, ival);
                    }
                }
                break;
            case 32:
                for (j = 0; j < wc; j++) {
                    if (j + x < 0  || j + x >= w) continue;
                    cval = GET_DATA_BYTE(linec, j);
                    if (transparent == 0 ||
                        (transparent != 0 && cval != transpix)) {
                        val32 = *(lined + j + x);
                        extractRGBValues(val32, &irval, &igval, &ibval);
                        irval = (l_int32)((1. - fract) * irval + fract * cval);
                        igval = (l_int32)((1. - fract) * igval + fract * cval);
                        ibval = (l_int32)((1. - fract) * ibval + fract * cval);
                        composeRGBPixel(irval, igval, ibval, &val32);
                        *(lined + j + x) = val32;
                    }
                }
                break;
            default:
                break;   /* shouldn't happen */
            }
        }
    }
    else {  /* L_BLEND_GRAY_WITH_INVERSE */
        for (i = 0; i < hc; i++) {
            if (i + y < 0  || i + y >= h) continue;
            linec = datac + i * wplc;
            lined = datad + (i + y) * wpld;
            switch (d)
            {
            case 8:
                /*
                 * For 8 bpp, the dest pix is shifted by a signed amount
                 * proportional to the distance from 128 (the pivot value),
                 * and to the darkness of src2.  If the dest is darker
                 * than 128, it becomes lighter, and v.v.
                 * The basic logic is:
                 *     d  -->  d + f * (0.5 - d) * (1 - c)
                 * where d and c are normalized pixel values for src1 and
                 * src2, respectively, with normalization to 255.
                 */
                for (j = 0; j < wc; j++) {
                    if (j + x < 0  || j + x >= w) continue;
                    cval = GET_DATA_BYTE(linec, j);
                    if (transparent == 0 ||
                        (transparent != 0 && cval != transpix)) {
                        ival = GET_DATA_BYTE(lined, j + x);
                        delta = (128 - ival) * (255 - cval) / 256;
                        ival += (l_int32)(fract * delta + 0.5);
                        SET_DATA_BYTE(lined, j + x, ival);
                    }
                }
                break;
            case 32:
                /* Each component is shifted by the same formula for 8 bpp */
                for (j = 0; j < wc; j++) {
                    if (j + x < 0  || j + x >= w) continue;
                    cval = GET_DATA_BYTE(linec, j);
                    if (transparent == 0 ||
                        (transparent != 0 && cval != transpix)) {
                        val32 = *(lined + j + x);
                        extractRGBValues(val32, &irval, &igval, &ibval);
                        delta = (128 - irval) * (255 - cval) / 256;
                        irval += (l_int32)(fract * delta + 0.5);
                        delta = (128 - igval) * (255 - cval) / 256;
                        igval += (l_int32)(fract * delta + 0.5);
                        delta = (128 - ibval) * (255 - cval) / 256;
                        ibval += (l_int32)(fract * delta + 0.5);
                        composeRGBPixel(irval, igval, ibval, &val32);
                        *(lined + j + x) = val32;
                    }
                }
                break;
            default:
                break;   /* shouldn't happen */
            }
        }
    }

    pixDestroy(&pixc);
    return pixd;
}


/*!
 *  pixBlendColor()
 *
 *      Input:  pixd (<optional>; either NULL or equal to pixs1 for in-place)
 *              pixs1 (blendee; depth > 1)
 *              pixs2 (blender, 32 bpp; typ. smaller in size than pixs1)
 *              x,y  (origin (UL corner) of pixs2 relative to
 *                    the origin of pixs1)
 *              fract (blending fraction)
 *              transparent (1 to use transparency; 0 otherwise)
 *              transpix (pixel color in pixs2 that is to be transparent)
 *      Return: pixd if OK; pixs1 on error
 *
 *  Notes:
 *      (1) pixs2 must be 32 bpp, and have no colormap.
 *      (2) Clipping of pixs2 to pixs1 is done in the inner pixel loop.
 *      (3) If pixs1 has a colormap, it is removed to generate a 32 bpp pix.
 *      (4) If pixs1 has depth < 32, it is unpacked to generate a 32 bpp pix.
 *      (5) For inplace operation, call it this way:
 *            pixBlendColor(pixs1, pixs1, pixs2, ...)
 *      (6) For generating a new pixd:
 *            pixd = pixBlendColor(NULL, pixs1, pixs2, ...)
 *      (7) Only call in-place if pixs1 is 32 bpp; otherwise it is an error.
 *      (8) If transparent = 0, the blending fraction (fract) is
 *          applied equally to all pixels.
 *      (9) If transparent = 1, all pixels of value transpix (typically
 *          either 0 or 0xffffff00) in pixs2 are transparent in the blend.
 */
PIX *
pixBlendColor(PIX       *pixd,
              PIX       *pixs1,
              PIX       *pixs2,
              l_int32    x,
              l_int32    y,
              l_float32  fract,
              l_int32    transparent,
              l_uint32   transpix)
{
l_int32    i, j, wc, hc, w, h, wplc, wpld;
l_int32    rval, gval, bval, rcval, gcval, bcval;
l_uint32   cval32, val32;
l_uint32  *linec, *lined, *datac, *datad;
PIX       *pixc, *pixt1, *pixt2;

    PROCNAME("pixBlendColor");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, pixd);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, pixd);
    if (pixGetDepth(pixs1) == 1)
        return (PIX *)ERROR_PTR("pixs1 is 1 bpp", procName, pixd);
    if (pixGetDepth(pixs2) != 32)
        return (PIX *)ERROR_PTR("pixs2 not 32 bpp", procName, pixd);
    if (pixd == pixs1 && pixGetDepth(pixs1) != 32)
        return (PIX *)ERROR_PTR("inplace; pixs1 not 32 bpp", procName, pixd);
    if (pixd && (pixd != pixs1))
        return (PIX *)ERROR_PTR("pixd must be NULL or pixs1", procName, pixd);
    if (fract < 0.0 || fract > 1.0) {
        L_WARNING("fract must be in [0.0, 1.0]; setting to 0.5", procName);
        fract = 0.5;
    }

        /* If pixd != NULL, we know that it is equal to pixs1 and
         * that pixs1 is 32 bpp rgb, so that an in-place operation
         * can be done.  Otherwise, remove colormap from pixs1 if
         * it exists and unpack to 32 bpp if necessary, to do the
         * blending on a new 32 bpp Pix. */
    if (!pixd) {
        pixt1 = pixRemoveColormap(pixs1, REMOVE_CMAP_TO_FULL_COLOR);
        if (pixGetDepth(pixt1) < 32)
            pixt2 = pixConvertTo32(pixt1);
        else
            pixt2 = pixClone(pixt1);
        pixd = pixCopy(NULL, pixt2);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }

    pixGetDimensions(pixd, &w, &h, NULL);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);
    pixc = pixClone(pixs2);
    pixGetDimensions(pixc, &wc, &hc, NULL);
    datac = pixGetData(pixc);
    wplc = pixGetWpl(pixc);

        /* check limits for src1, in case clipping was not done */
    for (i = 0; i < hc; i++) {
        if (i + y < 0  || i + y >= h) continue;
        linec = datac + i * wplc;
        lined = datad + (i + y) * wpld;
        for (j = 0; j < wc; j++) {
            if (j + x < 0  || j + x >= w) continue;
            cval32 = *(linec + j);
            if (transparent == 0 ||
                (transparent != 0 &&
                     ((cval32 & 0xffffff00) != (transpix & 0xffffff00)))) {
                val32 = *(lined + j + x);
                extractRGBValues(cval32, &rcval, &gcval, &bcval);
                extractRGBValues(val32, &rval, &gval, &bval);
                rval = (l_int32)((1. - fract) * rval + fract * rcval);
                gval = (l_int32)((1. - fract) * gval + fract * gcval);
                bval = (l_int32)((1. - fract) * bval + fract * bcval);
                composeRGBPixel(rval, gval, bval, &val32);
                *(lined + j + x) = val32;
            }
        }
    }

    pixDestroy(&pixc);
    return pixd;
}


/*
 *  pixBlendColorByChannel()
 *
 *  This is an extended version of pixBlendColor.  All parameters have the
 *  same meaning except it takes one mixing fraction per channel, and the
 *  mixing fraction may be < 0 or > 1, in which case, the min or max of two
 *  images are taken.  More specifically,
 *
 *   for a = pixs1[i], b = pixs2[i]:
 *       frac < 0.0 --> min(a, b)
 *       frac > 1.0 --> max(a, b)
 *       else --> (1-frac)*a + frac*b
 *       frac == 0 --> a
 *       frac == 1 --> b
 *
 * Notes:
 *     (1) See usage notes in pixBlendColor()
 *     (2) pixBlendColor() would be equivalent to
 *           pixBlendColorChannel(..., fract, fract, fract, ...);
 *         at a small cost of efficiency.
 */
PIX *
pixBlendColorByChannel(PIX       *pixd,
                       PIX       *pixs1,
                       PIX       *pixs2,
                       l_int32    x,
                       l_int32    y,
                       l_float32  rfract,
                       l_float32  gfract,
                       l_float32  bfract,
                       l_int32    transparent,
                       l_uint32   transpix)
{
l_int32    i, j, wc, hc, w, h, wplc, wpld;
l_int32    rval, gval, bval, rcval, gcval, bcval;
l_uint32   cval32, val32;
l_uint32  *linec, *lined, *datac, *datad;
PIX       *pixc, *pixt1, *pixt2;

    PROCNAME("pixBlendColorByChannel");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, pixd);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, pixd);
    if (pixGetDepth(pixs1) == 1)
        return (PIX *)ERROR_PTR("pixs1 is 1 bpp", procName, pixd);
    if (pixGetDepth(pixs2) != 32)
        return (PIX *)ERROR_PTR("pixs2 not 32 bpp", procName, pixd);
    if (pixd == pixs1 && pixGetDepth(pixs1) != 32)
        return (PIX *)ERROR_PTR("inplace; pixs1 not 32 bpp", procName, pixd);
    if (pixd && (pixd != pixs1))
        return (PIX *)ERROR_PTR("pixd must be NULL or pixs1", procName, pixd);

        /* If pixd != NULL, we know that it is equal to pixs1 and
         * that pixs1 is 32 bpp rgb, so that an in-place operation
         * can be done.  Otherwise, remove colormap from pixs1 if
         * it exists and unpack to 32 bpp if necessary, to do the
         * blending on a new 32 bpp Pix. */
    if (!pixd) {
        pixt1 = pixRemoveColormap(pixs1, REMOVE_CMAP_TO_FULL_COLOR);
        if (pixGetDepth(pixt1) < 32)
            pixt2 = pixConvertTo32(pixt1);
        else
            pixt2 = pixClone(pixt1);
        pixd = pixCopy(NULL, pixt2);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }

    pixGetDimensions(pixd, &w, &h, NULL);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);
    pixc = pixClone(pixs2);
    pixGetDimensions(pixc, &wc, &hc, NULL);
    datac = pixGetData(pixc);
    wplc = pixGetWpl(pixc);

        /* Check limits for src1, in case clipping was not done */
    for (i = 0; i < hc; i++) {
        if (i + y < 0  || i + y >= h) continue;
        linec = datac + i * wplc;
        lined = datad + (i + y) * wpld;
        for (j = 0; j < wc; j++) {
            if (j + x < 0  || j + x >= w) continue;
            cval32 = *(linec + j);
            if (transparent == 0 ||
                (transparent != 0 &&
                     ((cval32 & 0xffffff00) != (transpix & 0xffffff00)))) {
                val32 = *(lined + j + x);
                extractRGBValues(cval32, &rcval, &gcval, &bcval);
                extractRGBValues(val32, &rval, &gval, &bval);
                rval = blendComponents(rval, rcval, rfract);
                gval = blendComponents(gval, gcval, gfract);
                bval = blendComponents(bval, bcval, bfract);
                composeRGBPixel(rval, gval, bval, &val32);
                *(lined + j + x) = val32;
            }
        }
    }

    pixDestroy(&pixc);
    return pixd;
}


static l_int32
blendComponents(l_int32    a,
                l_int32    b,
                l_float32  fract)
{
    if (fract < 0.)
        return ((a < b) ? a : b);
    if (fract > 1.)
        return ((a > b) ? a : b);
    return (l_int32)((1. - fract) * a + fract * b);
}


/*!
 *  pixBlendGrayAdapt()
 *
 *      Input:  pixd (<optional>; either NULL or equal to pixs1 for in-place)
 *              pixs1 (blendee; depth > 1)
 *              pixs2 (blender, 8 bpp; typ. smaller in size than pixs1)
 *              x,y  (origin (UL corner) of pixs2 relative to
 *                    the origin of pixs1; can be < 0)
 *              fract (blending fraction)
 *              shift (>= 0 but <= 128: shift of zero blend value from
 *                     median source; use -1 for default value; )
 *      Return: pixd if OK; pixs1 on error
 *
 *  Notes:
 *      (1) pixs2 must be 8 bpp, and have no colormap.
 *      (2) Clipping of pixs2 to pixs1 is done in the inner pixel loop.
 *      (3) If pixs1 has a colormap, it is removed.
 *      (4) If pixs1 has depth < 8, it is unpacked to generate a 8 bpp pix.
 *      (5) For inplace operation, call it this way:
 *            pixBlendGray(pixs1, pixs1, pixs2, ...)
 *          For generating a new pixd:
 *            pixd = pixBlendGray(NULL, pixs1, pixs2, ...)
 *          Only call in-place if pixs1 does not have a colormap;
 *          otherwise it is an error.
 *      (6) This does a blend with inverse.  Whereas in pixGlendGray(), the
 *          zero blend point is where the blendee pixel is 128, here
 *          the zero blend point is found adaptively, with respect to the
 *          median of the blendee region.  If the median is < 128,
 *          the zero blend point is found from
 *              median + shift.
 *          Otherwise, if the median >= 128, the zero blend point is
 *              median - shift.
 *          The purpose of shifting the zero blend point away from the
 *          median is to prevent a situation in pixBlendGray() where
 *          the median is 128 and the blender is not visible.
 *          The default value of shift is 64.
 *      (7) After processing pixs1, it is either 8 bpp or 32 bpp:
 *          - if 8 bpp, the fraction of pixs2 is mixed with pixs1.
 *          - if 32 bpp, each component of pixs1 is mixed with
 *            the same fraction of pixs2.
 *      (8) The darker the blender, the more it mixes with the blendee.
 *          A blender value of 0 has maximum mixing; a value of 255
 *          has no mixing and hence is transparent.
 */
PIX *
pixBlendGrayAdapt(PIX       *pixd,
                  PIX       *pixs1,
                  PIX       *pixs2,
                  l_int32    x,
                  l_int32    y,
                  l_float32  fract,
                  l_int32    shift)
{
l_int32    i, j, d, wc, hc, w, h, wplc, wpld, delta, overlap;
l_int32    rval, gval, bval, cval, dval, mval, median, pivot;
l_uint32   val32;
l_uint32  *linec, *lined, *datac, *datad;
l_float32  fmedian, factor;
BOX       *box, *boxt;
PIX       *pixc, *pixt1, *pixt2;

    PROCNAME("pixBlendGrayAdapt");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, pixd);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, pixd);
    if (pixGetDepth(pixs1) == 1)
        return (PIX *)ERROR_PTR("pixs1 is 1 bpp", procName, pixd);
    if (pixGetDepth(pixs2) != 8)
        return (PIX *)ERROR_PTR("pixs2 not 8 bpp", procName, pixd);
    if (pixGetColormap(pixs2))
        return (PIX *)ERROR_PTR("pixs2 has a colormap", procName, pixd);
    if (pixd == pixs1 && pixGetColormap(pixs1))
        return (PIX *)ERROR_PTR("can't do in-place with cmap", procName, pixd);
    if (pixd && (pixd != pixs1))
        return (PIX *)ERROR_PTR("pixd must be NULL or pixs1", procName, pixd);
    if (fract < 0.0 || fract > 1.0) {
        L_WARNING("fract must be in [0.0, 1.0]; setting to 0.5", procName);
        fract = 0.5;
    }
    if (shift == -1) shift = 64;   /* default value */
    if (shift < 0 || shift > 127) {
        L_WARNING("invalid shift; setting to 64", procName);
        shift = 64;
    }

        /* Test for overlap */
    pixGetDimensions(pixs1, &w, &h, NULL);
    pixGetDimensions(pixs2, &wc, &hc, NULL);
    box = boxCreate(x, y, wc, hc);
    boxt = boxCreate(0, 0, w, h);
    boxIntersects(box, boxt, &overlap);
    boxDestroy(&boxt);
    if (!overlap) {
        boxDestroy(&box);
        return (PIX *)ERROR_PTR("no image overlap", procName, pixd);
    }

        /* If pixd != NULL, we know that it is equal to pixs1 and
         * that pixs1 does not have a colormap, so that an in-place operation
         * can be done.  Otherwise, remove colormap from pixs1 if
         * it exists and unpack to at least 8 bpp if necessary,
         * to do the blending on a new pix. */
    if (!pixd) {
        pixt1 = pixRemoveColormap(pixs1, REMOVE_CMAP_BASED_ON_SRC);
        if (pixGetDepth(pixt1) < 8)
            pixt2 = pixConvertTo8(pixt1, FALSE);
        else
            pixt2 = pixClone(pixt1);
        pixd = pixCopy(NULL, pixt2);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }

        /* Get the median value in the region of blending */
    pixt1 = pixClipRectangle(pixd, box, NULL);
    pixt2 = pixConvertTo8(pixt1, 0);
    pixGetRankValueMasked(pixt2, NULL, 0, 0, 1, 0.5, &fmedian, NULL);
    median = (l_int32)(fmedian + 0.5);
    if (median < 128)
        pivot = median + shift;
    else
        pivot = median - shift;
/*    L_INFO_INT2("median = %d, pivot = %d", procName, median, pivot); */
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    boxDestroy(&box);

        /* Process over src2; clip to src1. */
    d = pixGetDepth(pixd);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);
    pixc = pixClone(pixs2);
    datac = pixGetData(pixc);
    wplc = pixGetWpl(pixc);
    for (i = 0; i < hc; i++) {
        if (i + y < 0  || i + y >= h) continue;
        linec = datac + i * wplc;
        lined = datad + (i + y) * wpld;
        switch (d)
        {
        case 8:
                /*
                 * For 8 bpp, the dest pix is shifted by an amount
                 * proportional to the distance from the pivot value,
                 * and to the darkness of src2.  In no situation will it
                 * pass the pivot value in intensity.
                 * The basic logic is:
                 *     d  -->  d + f * (np - d) * (1 - c)
                 * where np, d and c are normalized pixel values for
                 * the pivot, src1 and src2, respectively, with normalization
                 * to 255.
                 */
            for (j = 0; j < wc; j++) {
                if (j + x < 0  || j + x >= w) continue;
                dval = GET_DATA_BYTE(lined, j + x);
                cval = GET_DATA_BYTE(linec, j);
                delta = (pivot - dval) * (255 - cval) / 256;
                dval += (l_int32)(fract * delta + 0.5);
                SET_DATA_BYTE(lined, j + x, dval);
            }
            break;
        case 32:
                /*
                 * For 32 bpp, the dest pix is shifted by an amount
                 * proportional to the max component distance from the
                 * pivot value, and to the darkness of src2.  Each component
                 * is shifted by the same fraction, either up or down,
                 * depending on the shift direction (which is toward the
                 * pivot).   The basic logic for the red component is:
                 *     r  -->  r + f * (np - m) * (1 - c) * (r / m)
                 * where np, r, m and c are normalized pixel values for
                 * the pivot, the r component of src1, the max component
                 * of src1, and src2, respectively, again with normalization
                 * to 255.  Likewise for the green and blue components.
                 */
            for (j = 0; j < wc; j++) {
                if (j + x < 0  || j + x >= w) continue;
                cval = GET_DATA_BYTE(linec, j);
                val32 = *(lined + j + x);
                extractRGBValues(val32, &rval, &gval, &bval);
                mval = L_MAX(rval, gval);
                mval = L_MAX(mval, bval);
                mval = L_MAX(mval, 1);
                delta = (pivot - mval) * (255 - cval) / 256;
                factor = fract * delta / mval;
                rval += (l_int32)(factor * rval + 0.5);
                gval += (l_int32)(factor * gval + 0.5);
                bval += (l_int32)(factor * bval + 0.5);
                composeRGBPixel(rval, gval, bval, &val32);
                *(lined + j + x) = val32;
            }
            break;
        default:
            break;   /* shouldn't happen */
        }
    }

    pixDestroy(&pixc);
    return pixd;
}


/*!
 *  pixFadeWithGray()
 *
 *      Input:  pixs (colormapped or 8 bpp or 32 bpp)
 *              pixb (8 bpp blender)
 *              factor (multiplicative factor to apply to blender value)
 *              type (L_BLEND_TO_WHITE, L_BLEND_TO_BLACK)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This function combines two pix aligned to the UL corner; they
 *          need not be the same size.
 *      (2) Each pixel in pixb is multiplied by 'factor' divided by 255, and
 *          clipped to the range [0 ... 1].  This gives the fade fraction
 *          to be appied to pixs.  Fade either to white (L_BLEND_TO_WHITE)
 *          or to black (L_BLEND_TO_BLACK).
 */
PIX *
pixFadeWithGray(PIX       *pixs,
                PIX       *pixb,
                l_float32  factor,
                l_int32    type)
{
l_int32    i, j, w, h, d, wb, hb, db, wd, hd, wplb, wpld;
l_int32    valb, vald, nvald, rval, gval, bval, nrval, ngval, nbval;
l_float32  nfactor, fract;
l_uint32   val32, nval32;
l_uint32  *lined, *datad, *lineb, *datab;
PIX       *pixd;
PIXCMAP   *cmap;

    PROCNAME("pixFadeWithGray");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!pixb)
        return (PIX *)ERROR_PTR("pixb not defined", procName, NULL);
    cmap = pixGetColormap(pixs);
    d = pixGetDepth(pixs);
    if (d < 8 && !cmap)
        return (PIX *)ERROR_PTR("pixs not cmapped and < 8bpp", procName, NULL);
    pixGetDimensions(pixb, &wb, &hb, &db);
    if (db != 8)
        return (PIX *)ERROR_PTR("pixb not 8bpp", procName, NULL);
    if (type != L_BLEND_TO_WHITE && type != L_BLEND_TO_BLACK)
        return (PIX *)ERROR_PTR("invalid fade type", procName, NULL);

    if (cmap)
        pixd = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    else
        pixd = pixCopy(NULL, pixs);
    pixGetDimensions(pixd, &wd, &hd, &d);
    w = L_MIN(wb, wd);
    h = L_MIN(hb, hd);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    datab = pixGetData(pixb);
    wplb = pixGetWpl(pixb);

    nfactor = factor / 255.;
    for (i = 0; i < h; i++) {
        lineb = datab + i * wplb;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            valb = GET_DATA_BYTE(lineb, j);
            fract = nfactor * (l_float32)valb;
            fract = L_MIN(fract, 1.0);
            if (d == 8) {
                vald = GET_DATA_BYTE(lined, j);
                if (type == L_BLEND_TO_WHITE)
                    nvald = vald + (l_int32)(fract * (255. - (l_float32)vald));
                else  /* L_BLEND_TO_BLACK */
                    nvald = vald - (l_int32)(fract * (l_float32)vald);
                SET_DATA_BYTE(lined, j, nvald);
            }
            else {  /* d == 32 */
                val32 = lined[j];
                extractRGBValues(val32, &rval, &gval, &bval);
                if (type == L_BLEND_TO_WHITE) {
                    nrval = rval + (l_int32)(fract * (255. - (l_float32)rval));
                    ngval = gval + (l_int32)(fract * (255. - (l_float32)gval));
                    nbval = bval + (l_int32)(fract * (255. - (l_float32)bval));
                }
                else {
                    nrval = rval - (l_int32)(fract * (l_float32)rval);
                    ngval = gval - (l_int32)(fract * (l_float32)gval);
                    nbval = bval - (l_int32)(fract * (l_float32)bval);
                }
                composeRGBPixel(nrval, ngval, nbval, &nval32);
                lined[j] = nval32;
            }
        }
    }

    return pixd;
}


/*
 *  pixBlendHardLight()
 *
 *      Input:  pixd (<optional>; either NULL or equal to pixs1 for in-place)
 *              pixs1 (blendee; depth > 1, may be cmapped)
 *              pixs2 (blender, 8 or 32 bpp; may be colormapped;
 *                     typ. smaller in size than pixs1)
 *              x,y  (origin (UL corner) of pixs2 relative to
 *                    the origin of pixs1)
 *              fract (blending fraction, or 'opacity factor')
 *      Return: pixd if OK; pixs1 on error
 *
 *  Notes:
 *      (1) pixs2 must be 8 or 32 bpp; either may have a colormap.
 *      (2) Clipping of pixs2 to pixs1 is done in the inner pixel loop.
 *      (3) Only call in-place if pixs1 is not colormapped.
 *      (4) If pixs1 has a colormap, it is removed to generate either an
 *          8 or 32 bpp pix, depending on the colormap.
 *      (5) For inplace operation, call it this way:
 *            pixBlendHardLight(pixs1, pixs1, pixs2, ...)
 *      (6) For generating a new pixd:
 *            pixd = pixBlendHardLight(NULL, pixs1, pixs2, ...)
 *      (7) This is a generalization of the usual hard light blending,
 *          where fract == 1.0.
 *      (8) When the opacity factor fract = 1.0, this implements "overlay"
 *          blending, by swapping pixs1 and pixs2.
 *      (9) See, e.g.:
 *           http://www.pegtop.net/delphi/articles/blendmodes/hardlight.htm
 *           http://www.digitalartform.com/imageArithmetic.htm
 *      (10) This function was built by Paco Galanes.
 */
PIX *
pixBlendHardLight(PIX       *pixd,
                  PIX       *pixs1,
                  PIX       *pixs2,
                  l_int32    x,
                  l_int32    y,
                  l_float32  fract)
{
l_int32    i, j, w, h, d, wc, hc, dc, wplc, wpld;
l_int32    cval, dval, rcval, gcval, bcval, rdval, gdval, bdval;
l_uint32   cval32, dval32;
l_uint32  *linec, *lined, *datac, *datad;
PIX       *pixc, *pixt;

    PROCNAME("pixBlendHardLight");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, pixd);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, pixd);
    pixGetDimensions(pixs1, &w, &h, &d);
    pixGetDimensions(pixs2, &wc, &hc, &dc);
    if (d == 1)
        return (PIX *)ERROR_PTR("pixs1 is 1 bpp", procName, pixd);
    if (dc != 8 && dc != 32)
        return (PIX *)ERROR_PTR("pixs2 not 8 or 32 bpp", procName, pixd);
    if (pixd && (pixd != pixs1))
        return (PIX *)ERROR_PTR("inplace and pixd != pixs1", procName, pixd);
    if (pixd == pixs1 && pixGetColormap(pixs1))
        return (PIX *)ERROR_PTR("inplace and pixs1 cmapped", procName, pixd);
    if (pixd && d != 8 && d != 32)
        return (PIX *)ERROR_PTR("inplace and not 8 or 32 bpp", procName, pixd);

    if (fract < 0.0 || fract > 1.0) {
        L_WARNING("fract must be in [0.0, 1.0]; setting to 0.5", procName);
        fract = 0.5;
    }

        /* If pixs2 has a colormap, remove it */
    if (pixGetColormap(pixs2))
        pixc = pixRemoveColormap(pixs2, REMOVE_CMAP_BASED_ON_SRC);
    else
        pixc = pixClone(pixs2);
    dc = pixGetDepth(pixc);

        /* There are 4 cases:
         *    * pixs1 has or doesn't have a colormap
         *    * pixc is either 8 or 32 bpp
         * In all situations, if pixs has a colormap it must be removed,
         * and pixd must have a depth that is equal to or greater than pixc. */
    if (dc == 32) {
        if (pixGetColormap(pixs1))  /* pixd == NULL */
            pixd = pixRemoveColormap(pixs1, REMOVE_CMAP_TO_FULL_COLOR);
        else {
            if (!pixd)
                pixd = pixConvertTo32(pixs1);
            else {
                pixt = pixConvertTo32(pixs1);
                pixCopy(pixd, pixt);
                pixDestroy(&pixt);
            }
        }
        d = 32;
    } else {  /* dc == 8 */
        if (pixGetColormap(pixs1))   /* pixd == NULL */
            pixd = pixRemoveColormap(pixs1, REMOVE_CMAP_BASED_ON_SRC);
        else
            pixd = pixCopy(pixd, pixs1);
        d = pixGetDepth(pixd);
    }

    if (!(d == 8 && dc == 8) &&   /* 3 cases only */
        !(d == 32 && dc == 8) &&
        !(d == 32 && dc == 32)) {
        pixDestroy(&pixc);
        return (PIX *)ERROR_PTR("bad! -- invalid depth combo!", procName, pixd);
    }

    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);
    datac = pixGetData(pixc);
    wplc = pixGetWpl(pixc);
    for (i = 0; i < hc; i++) {
        if (i + y < 0  || i + y >= h) continue;
        linec = datac + i * wplc;
        lined = datad + (i + y) * wpld;
        for (j = 0; j < wc; j++) {
            if (j + x < 0  || j + x >= w) continue;
            if (d == 8 && dc == 8) {
                dval = GET_DATA_BYTE(lined, x + j);
                cval = GET_DATA_BYTE(linec, j);
                dval = blendHardLightComponents(dval, cval, fract);
                SET_DATA_BYTE(lined, x + j, dval);
            } else if (d == 32 && dc == 8) {
                dval32 = *(lined + x + j);
                extractRGBValues(dval32, &rdval, &gdval, &bdval);
                cval = GET_DATA_BYTE(linec, j);
                rdval = blendHardLightComponents(rdval, cval, fract);
                gdval = blendHardLightComponents(gdval, cval, fract);
                bdval = blendHardLightComponents(bdval, cval, fract);
                composeRGBPixel(rdval, gdval, bdval, &dval32);
                *(lined + x + j) = dval32;
            } else if (d == 32 && dc == 32) {
                dval32 = *(lined + x + j);
                extractRGBValues(dval32, &rdval, &gdval, &bdval);
                cval32 = *(linec + j);
                extractRGBValues(cval32, &rcval, &gcval, &bcval);
                rdval = blendHardLightComponents(rdval, rcval, fract);
                gdval = blendHardLightComponents(gdval, gcval, fract);
                bdval = blendHardLightComponents(bdval, bcval, fract);
                composeRGBPixel(rdval, gdval, bdval, &dval32);
                *(lined + x + j) = dval32;
            }
        }
    }

    pixDestroy(&pixc);
    return pixd;
}


/*
 *  blendHardLightComponents()
 *      Input:  a (8 bpp blendee component)
 *              b (8 bpp blender component)
 *              fract (fraction of blending; use 1.0 for usual definition)
 *      Return: blended 8 bpp component
 */
static l_int32 blendHardLightComponents(l_int32    a,
                                        l_int32    b,
                                        l_float32  fract)
{
    if (b < 0x80) {
        b = 0x80 - (l_int32)(fract * (0x80 - b));
        return (a * b) >> 7;
    } else {
        b = 0x80 + (l_int32)(fract * (b - 0x80));
        return  0xff - (((0xff - b) * (0xff - a)) >> 7);
    }
}


/*-------------------------------------------------------------*
 *               Blending two colormapped images               *
 *-------------------------------------------------------------*/
/*!
 *  pixBlendCmap()
 *
 *      Input:  pixs (2, 4 or 8 bpp, with colormap)
 *              pixb (colormapped blender)
 *              x, y (UL corner of blender relative to pixs)
 *              sindex (colormap index of pixels in pixs to be changed)
 *      Return: 0 if OK, 1 on error
 *
 *  Note:
 *      (1) This function combines two colormaps, and replaces the pixels
 *          in pixs that have a specified color value with those in pixb.
 *      (2) sindex must be in the existing colormap; otherwise an
 *          error is returned.  In use, sindex will typically be the index
 *          for white (255, 255, 255).
 *      (3) Blender colors that already exist in the colormap are used;
 *          others are added.  If any blender colors cannot be
 *          stored in the colormap, an error is returned.
 *      (4) In the implementation, a mapping is generated from each
 *          original blender colormap index to the corresponding index
 *          in the expanded colormap for pixs.  Then for each pixel in
 *          pixs with value sindex, and which is covered by a blender pixel,
 *          the new index corresponding to the blender pixel is substituted
 *          for sindex.
 */
l_int32
pixBlendCmap(PIX     *pixs,
             PIX     *pixb,
             l_int32  x,
             l_int32  y,
             l_int32  sindex)
{
l_int32    rval, gval, bval;
l_int32    i, j, w, h, d, ncb, wb, hb, wpls;
l_int32    index, val, nadded;
l_int32    lut[256];
l_uint32   pval;
l_uint32  *lines, *datas;
PIXCMAP   *cmaps, *cmapb, *cmapsc;

    PROCNAME("pixBlendCmap");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixb)
        return ERROR_INT("pixb not defined", procName, 1);
    if ((cmaps = pixGetColormap(pixs)) == NULL)
        return ERROR_INT("no colormap in pixs", procName, 1);
    if ((cmapb = pixGetColormap(pixb)) == NULL)
        return ERROR_INT("no colormap in pixb", procName, 1);
    ncb = pixcmapGetCount(cmapb);

        /* Make a copy of cmaps; we'll add to this if necessary
         * and substitute at the end if we found there was enough room
         * to hold all the new colors. */
    cmapsc = pixcmapCopy(cmaps);

    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 2 && d != 4 && d != 8)
        return ERROR_INT("depth not in {2,4,8}", procName, 1);

        /* Add new colors if necessary; get mapping array between
         * cmaps and cmapb. */
    for (i = 0, nadded = 0; i < ncb; i++) {
        pixcmapGetColor(cmapb, i, &rval, &gval, &bval);
        if (pixcmapGetIndex(cmapsc, rval, gval, bval, &index)) { /* not found */
            if (pixcmapAddColor(cmapsc, rval, gval, bval)) {
                pixcmapDestroy(&cmapsc);
                return ERROR_INT("not enough room in cmaps", procName, 1);
            }
            lut[i] = pixcmapGetCount(cmapsc) - 1;
            nadded++;
        }
        else
            lut[i] = index;
    }

        /* Replace cmaps if colors have been added. */
    if (nadded == 0)
        pixcmapDestroy(&cmapsc);
    else
        pixSetColormap(pixs, cmapsc);

        /* Replace each pixel value sindex by mapped colormap index when
         * a blender pixel in pixbc overlays it. */
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    pixGetDimensions(pixb, &wb, &hb, NULL);
    for (i = 0; i < hb; i++) {
        if (i + y < 0  || i + y >= h) continue;
        lines = datas + (y + i) * wpls;
        for (j = 0; j < wb; j++) {
            if (j + x < 0  || j + x >= w) continue;
            switch (d) {
            case 2:
                val = GET_DATA_DIBIT(lines, x + j);
                if (val == sindex) {
                    pixGetPixel(pixb, j, i, &pval);
                    SET_DATA_DIBIT(lines, x + j, lut[pval]);
                }
                break;
            case 4:
                val = GET_DATA_QBIT(lines, x + j);
                if (val == sindex) {
                    pixGetPixel(pixb, j, i, &pval);
                    SET_DATA_QBIT(lines, x + j, lut[pval]);
                }
                break;
            case 8:
                val = GET_DATA_BYTE(lines, x + j);
                if (val == sindex) {
                    pixGetPixel(pixb, j, i, &pval);
                    SET_DATA_BYTE(lines, x + j, lut[pval]);
                }
                break;
            default:
                return ERROR_INT("depth not in {2,4,8}", procName, 1);
            }
        }
    }

    return 0;
}


/*---------------------------------------------------------------------*
 *                  Blending two images using a third                  *
 *---------------------------------------------------------------------*/
/*!
 *  pixBlendWithGrayMask()
 *
 *      Input:  pixs1 (8 bpp gray, rgb or colormapped)
 *              pixs2 (8 bpp gray, rgb or colormapped)
 *              pixg (8 bpp gray, for transparency of pixs2; can be null)
 *              x, y (UL corner of pixg with respect to pixs1)
 *      Return: pixd (blended image), or null on error
 *
 *  Notes:
 *      (1) The result is 8 bpp grayscale if both pixs1 and pixs2 are
 *          8 bpp gray.  Otherwise, the result is 32 bpp rgb.
 *      (2) pixg is an 8 bpp transparency image, where 0 is transparent
 *          and 255 is opaque.  It determines the transparency of pixs2
 *          when applied over pixs1.  It can be null if pixs2 is rgb,
 *          in which case we use the alpha component of pixs2.
 *      (3) If pixg exists, both it and pixs2 must be the same size,
 *          and they are applied with both their UL corners at the
 *          location (x, y) in pixs1.
 *      (4) The pixels in pixd are a combination of those in pixs1
 *          and pixs2, where the amount from pixs2 is proportional to
 *          the value of the pixel (p) in pixg, and the amount from pixs1
 *          is proportional to (255 - p).  Thus pixg is a transparency
 *          image (usually called an alpha blender) where each pixel
 *          can be associated with a pixel in pixs2, and determines
 *          the amount of the pixs2 pixel in the final result.
 *          For example, if pixg is all 0, pixs2 is transparent and
 *          the result in pixd is simply pixs1.
 *      (5) A typical use is for the pixs2/pixg combination to be
 *          a small watermark that is applied to pixs1.
 */
PIX *
pixBlendWithGrayMask(PIX     *pixs1,
                     PIX     *pixs2,
                     PIX     *pixg,
                     l_int32  x,
                     l_int32  y)
{
l_int32    w1, h1, d1, w2, h2, d2, wg, hg, wmin, hmin, wpld, wpls, wplg;
l_int32    i, j, val, dval, sval;
l_int32    drval, dgval, dbval, srval, sgval, sbval;
l_uint32   dval32, sval32;
l_uint32  *datad, *datas, *datag, *lined, *lines, *lineg;
l_float32  fract;
PIX       *pixr1, *pixr2, *pix1, *pix2, *pixalpha, *pixd;

    PROCNAME("pixBlendWithGrayMask");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, NULL);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, NULL);
    pixGetDimensions(pixs1, &w1, &h1, &d1);
    pixGetDimensions(pixs2, &w2, &h2, &d2);
    if (d1 == 1 || d2 == 1)
        return (PIX *)ERROR_PTR("pixs1 or pixs2 is 1 bpp", procName, NULL);
    if (pixg) {
        if (pixGetDepth(pixg) != 8)
            return (PIX *)ERROR_PTR("pixg not 8 bpp", procName, NULL);
        pixGetDimensions(pixg, &wg, &hg, NULL);
        wmin = L_MIN(w2, wg);
        hmin = L_MIN(h2, hg);
        pixalpha = pixClone(pixg);
    }
    else {  /* use the alpha component of pixs2 */
        if (d2 != 32)
            return (PIX *)ERROR_PTR("no alpha; pixs2 not rgba", procName, NULL);
        wmin = w2;
        hmin = h2;
        pixalpha = pixGetRGBComponent(pixs2, L_ALPHA_CHANNEL);
    }

        /* Remove colormaps if they exist */
    if (pixGetColormap(pixs1))
        pixr1 = pixRemoveColormap(pixs1, REMOVE_CMAP_BASED_ON_SRC);
    else
        pixr1 = pixClone(pixs1);
    if (pixGetColormap(pixs2))
        pixr2 = pixRemoveColormap(pixs2, REMOVE_CMAP_BASED_ON_SRC);
    else
        pixr2 = pixClone(pixs2);

        /* Regularize to the same depth if necessary */
    d1 = pixGetDepth(pixr1);
    d2 = pixGetDepth(pixr2);
    if (d1 == 32) {  /* convert d2 to rgb if necessary */
        pix1 = pixClone(pixr1);
        if (d2 != 32)
            pix2 = pixConvertTo32(pixr2);
        else
            pix2 = pixClone(pixr2);
    }
    else if (d2 == 32) {   /* and d1 != 32; convert to 32 */
        pix2 = pixClone(pixr2);
        pix1 = pixConvertTo32(pixr1);
    }
    else {  /* both are 8 bpp or less */
        pix1 = pixConvertTo8(pixr1, FALSE);
        pix2 = pixConvertTo8(pixr2, FALSE);
    }
    pixDestroy(&pixr1);
    pixDestroy(&pixr2);

        /* Sanity check */
    d1 = pixGetDepth(pix1);
    d2 = pixGetDepth(pix2);
    if (d1 != d2) {
        pixDestroy(&pix1);
        pixDestroy(&pix2);
        return (PIX *)ERROR_PTR("depths not regularized! bad!", procName, NULL);
    }

        /* Start off with a copy of pix1.  Then only change
         * pixels that will be blended with pix2. */
    pixd = pixCopy(NULL, pix1);
    pixDestroy(&pix1);

        /*
         * Let the normalized pixel value of pixg be f = pixval / 255,
         * and the pixel values of pixs1 and pixs2 be p1 and p2, rsp.
         * Then the blended value is:
         *      p = (1.0 - f) * p1 + f * p2
         * Blending is done component-wise if rgb.
         *
         * Scan over pixs2 and pixg, doing clipping on pixs1 where necessary.
         */
    datad = pixGetData(pixd);
    datas = pixGetData(pix2);
    datag = pixGetData(pixalpha);
    wpld = pixGetWpl(pixd);
    wpls = pixGetWpl(pix2);
    wplg = pixGetWpl(pixalpha);
    for (i = 0; i < hmin; i++) {
        if (i + y < 0  || i + y >= h1) continue;
        lined = datad + (i + y) * wpld;
        lines = datas + i * wpls;
        lineg = datag + i * wplg;
        for (j = 0; j < wmin; j++) {
            if (j + x < 0  || j + x >= w1) continue;
            val = GET_DATA_BYTE(lineg, j);
            if (val == 0) continue;  /* pix2 is transparent */
            fract = (l_float32)val / 255.;
            switch (d1)
            {
            case 8:
                dval = GET_DATA_BYTE(lined, j + x);
                sval = GET_DATA_BYTE(lines, j);
                dval = (l_int32)((1.0 - fract) * dval + fract * sval);
                SET_DATA_BYTE(lined, j + x, dval);
                break;
            case 32:
                dval32 = *(lined + j + x);
                sval32 = *(lines + j);
                extractRGBValues(dval32, &drval, &dgval, &dbval);
                extractRGBValues(sval32, &srval, &sgval, &sbval);
                drval = (l_int32)((1.0 - fract) * drval + fract * srval);
                dgval = (l_int32)((1.0 - fract) * dgval + fract * sgval);
                dbval = (l_int32)((1.0 - fract) * dbval + fract * sbval);
                composeRGBPixel(drval, dgval, dbval, &dval32);
                *(lined + j + x) = dval32;
                break;
            default:
                return (PIX *)ERROR_PTR("impossible error", procName, NULL);
            }
        }
    }

    pixDestroy(&pixalpha);
    pixDestroy(&pix2);
    return pixd;
}


/*---------------------------------------------------------------------*
 *                        Coloring "gray" pixels                       *
 *---------------------------------------------------------------------*/
/*!
 *  pixColorGray()
 *
 *      Input:  pixs (8 bpp gray, rgb or colormapped image)
 *              box (<optional> region in which to apply color; can be NULL)
 *              type (L_PAINT_LIGHT, L_PAINT_DARK)
 *              thresh (average value below/above which pixel is unchanged)
 *              rval, gval, bval (new color to paint)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This is an in-place operation; pixs is modified.
 *          If pixs is colormapped, the operation will add colors to the
 *          colormap.  Otherwise, pixs will be converted to 32 bpp rgb if
 *          it is initially 8 bpp gray.
 *      (2) If type == L_PAINT_LIGHT, it colorizes non-black pixels,
 *          preserving antialiasing.
 *          If type == L_PAINT_DARK, it colorizes non-white pixels,
 *          preserving antialiasing.
 *      (3) If box is NULL, applies function to the entire image; otherwise,
 *          clips the operation to the intersection of the box and pix.
 *      (4) If colormapped, calls pixColorGrayCmap(), which applies the
 *          coloring algorithm only to pixels that are strictly gray.
 *      (5) For RGB, determines a "gray" value by averaging; then uses this
 *          value, plus the input rgb target, to generate the output
 *          pixel values.
 *      (6) thresh is only used for rgb; it is ignored for colormapped pix.
 *          If type == L_PAINT_LIGHT, use thresh = 0 if all pixels are to
 *          be colored (black pixels will be unaltered).
 *          In situations where there are a lot of black pixels,
 *          setting thresh > 0 will make the function considerably
 *          more efficient without affecting the final result.
 *          If type == L_PAINT_DARK, use thresh = 255 if all pixels
 *          are to be colored (white pixels will be unaltered).
 *          In situations where there are a lot of white pixels,
 *          setting thresh < 255 will make the function considerably
 *          more efficient without affecting the final result.
 */
l_int32
pixColorGray(PIX     *pixs,
             BOX     *box,
             l_int32  type,
             l_int32  thresh,
             l_int32  rval,
             l_int32  gval,
             l_int32  bval)
{
l_int32    i, j, w, h, d, wpl, x1, x2, y1, y2, bw, bh;
l_int32    nrval, ngval, nbval, aveval;
l_float32  factor;
l_uint32   val32;
l_uint32  *line, *data;
PIX       *pixt;
PIXCMAP   *cmap;

    PROCNAME("pixColorGray");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (type != L_PAINT_LIGHT && type != L_PAINT_DARK)
        return ERROR_INT("invalid type", procName, 1);

    cmap = pixGetColormap(pixs);
    pixGetDimensions(pixs, &w, &h, &d);
    if (!cmap && d != 8 && d != 32)
        return ERROR_INT("pixs not cmapped, 8 bpp or rgb", procName, 1);
    if (cmap)
        return pixColorGrayCmap(pixs, box, type, rval, gval, bval);

        /* rgb or 8 bpp gray image; check the thresh */
    if (type == L_PAINT_LIGHT) {  /* thresh should be low */
        if (thresh >= 255)
            return ERROR_INT("thresh must be < 255; else this is a no-op",
                             procName, 1);
        if (thresh > 127)
            L_WARNING("threshold set very high", procName);
    }
    else {  /* type == L_PAINT_DARK; thresh should be high */
        if (thresh <= 0)
            return ERROR_INT("thresh must be > 0; else this is a no-op",
                             procName, 1);
        if (thresh < 128)
            L_WARNING("threshold set very low", procName);
    }

    if (d == 8) {
        pixt = pixConvertTo32(pixs);
        pixTransferAllData(pixs, &pixt, 1, 0);
    }

    if (!box) {
        x1 = y1 = 0;
        x2 = w;
        y2 = h;
    }
    else {
        boxGetGeometry(box, &x1, &y1, &bw, &bh);
        x2 = x1 + bw - 1;
        y2 = y1 + bh - 1;
    }

    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    factor = 1. / 255.;
    for (i = y1; i <= y2; i++) {
        if (i < 0 || i >= h)
            continue;
        line = data + i * wpl;
        for (j = x1; j <= x2; j++) {
            if (j < 0 || j >= w)
                continue;
            val32 = *(line + j);
            aveval = ((val32 >> 24) + ((val32 >> 16) & 0xff) +
                      ((val32 >> 8) & 0xff)) / 3;
            if (type == L_PAINT_LIGHT) {
                if (aveval < thresh)  /* skip sufficiently dark pixels */
                    continue;
                nrval = (l_int32)(rval * aveval * factor);
                ngval = (l_int32)(gval * aveval * factor);
                nbval = (l_int32)(bval * aveval * factor);
            }
            else {  /* type == L_PAINT_DARK */
                if (aveval > thresh)  /* skip sufficiently light pixels */
                    continue;
                nrval = rval + (l_int32)((255. - rval) * aveval * factor);
                ngval = gval + (l_int32)((255. - gval) * aveval * factor);
                nbval = bval + (l_int32)((255. - bval) * aveval * factor);
            }
            composeRGBPixel(nrval, ngval, nbval, &val32);
            *(line + j) = val32;
        }
    }

    return 0;
}



/*------------------------------------------------------------------*
 *            Adjusting one or more colors to a target color        *
 *------------------------------------------------------------------*/
/*!
 *  pixSnapColor()
 *
 *      Input:  pixd (<optional>; either NULL or equal to pixs for in-place)
 *              pixs (colormapped or 8 bpp gray or 32 bpp rgb)
 *              srcval (color center to be selected for change: 0xrrggbb00)
 *              dstval (target color for pixels: 0xrrggbb00)
 *              diff (max absolute difference, applied to all components)
 *      Return: pixd (with all pixels within diff of pixval set to pixval),
 *                    or pixd on error
 *
 *  Notes:
 *      (1) For inplace operation, call it this way:
 *           pixSnapColor(pixs, pixs, ... )
 *      (2) For generating a new pixd:
 *           pixd = pixSnapColor(NULL, pixs, ...)
 *      (3) If pixs has a colormap, it is handled by pixSnapColorCmap().
 *      (4) All pixels within 'diff' of 'srcval', componentwise,
 *          will be changed to 'dstval'.
 */
PIX *
pixSnapColor(PIX      *pixd,
             PIX      *pixs,
             l_uint32  srcval,
             l_uint32  dstval,
             l_int32   diff)
{
l_int32    val, sval, dval;
l_int32    rval, gval, bval, rsval, gsval, bsval;
l_int32    i, j, w, h, d, wpl;
l_uint32   pixel;
l_uint32  *line, *data;

    PROCNAME("pixSnapColor");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixd && (pixd != pixs))
        return (PIX *)ERROR_PTR("pixd not null or == pixs", procName, pixd);

    if (pixGetColormap(pixs))
        return pixSnapColorCmap(pixd, pixs, srcval, dstval, diff);

        /* pixs does not have a colormap; it must be 8 bpp gray or
         * 32 bpp rgb. */
    if (pixGetDepth(pixs) < 8)
        return (PIX *)ERROR_PTR("pixs is < 8 bpp", procName, pixd);

        /* Do the work on pixd */
    if (!pixd)
        pixd = pixCopy(NULL, pixs);

    pixGetDimensions(pixd, &w, &h, &d);
    data = pixGetData(pixd);
    wpl = pixGetWpl(pixd);
    if (d == 8) {
        sval = srcval & 0xff;
        dval = dstval & 0xff;
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            for (j = 0; j < w; j++) {
                val = GET_DATA_BYTE(line, j);
                if (L_ABS(val - sval) <= diff)
                    SET_DATA_BYTE(line, j, dval);
            }
        }
    }
    else {  /* d == 32 */
        extractRGBValues(srcval, &rsval, &gsval, &bsval);
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            for (j = 0; j < w; j++) {
                pixel = *(line + j);
                extractRGBValues(pixel, &rval, &gval, &bval);
                if ((L_ABS(rval - rsval) <= diff) &&
                    (L_ABS(gval - gsval) <= diff) &&
                    (L_ABS(bval - bsval) <= diff))
                    *(line + j) = dstval;  /* replace */
            }
        }
    }

    return pixd;
}


/*!
 *  pixSnapColorCmap()
 *
 *      Input:  pixd (<optional>; either NULL or equal to pixs for in-place)
 *              pixs (colormapped)
 *              srcval (color center to be selected for change: 0xrrggbb00)
 *              dstval (target color for pixels: 0xrrggbb00)
 *              diff (max absolute difference, applied to all components)
 *      Return: pixd (with all pixels within diff of srcval set to dstval),
 *                    or pixd on error
 *
 *  Notes:
 *      (1) For inplace operation, call it this way:
 *           pixSnapCcmap(pixs, pixs, ... )
 *      (2) For generating a new pixd:
 *           pixd = pixSnapCmap(NULL, pixs, ...)
 *      (3) pixs must have a colormap.
 *      (4) All colors within 'diff' of 'srcval', componentwise,
 *          will be changed to 'dstval'.
 */
PIX *
pixSnapColorCmap(PIX      *pixd,
                 PIX      *pixs,
                 l_uint32  srcval,
                 l_uint32  dstval,
                 l_int32   diff)
{
l_int32    i, ncolors, index, found;
l_int32    rval, gval, bval, rsval, gsval, bsval, rdval, gdval, bdval;
l_int32   *tab;
PIX       *pixm;
PIXCMAP   *cmap;

    PROCNAME("pixSnapColorCmap");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (!pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("cmap not found", procName, pixd);
    if (pixd && (pixd != pixs))
        return (PIX *)ERROR_PTR("pixd not null or == pixs", procName, pixd);

    if (!pixd)
        pixd = pixCopy(NULL, pixs);

        /* If no free colors, look for one close to the target
         * that can be commandeered. */
    cmap = pixGetColormap(pixd);
    ncolors = pixcmapGetCount(cmap);
    extractRGBValues(srcval, &rsval, &gsval, &bsval);
    extractRGBValues(dstval, &rdval, &gdval, &bdval);
    found = FALSE;
    if (pixcmapGetFreeCount(cmap) == 0) {
        for (i = 0; i < ncolors; i++) {
            pixcmapGetColor(cmap, i, &rval, &gval, &bval);
            if ((L_ABS(rval - rsval) <= diff) &&
                (L_ABS(gval - gsval) <= diff) &&
                (L_ABS(bval - bsval) <= diff)) {
                index = i;
                pixcmapResetColor(cmap, index, rdval, gdval, bdval);
                found = TRUE;
                break;
            }
        }
    }
    else {  /* just add the new color */
        pixcmapAddColor(cmap, rdval, gdval, bdval);
        ncolors = pixcmapGetCount(cmap);
        index = ncolors - 1;  /* index of new destination color */
        found = TRUE;
    }

    if (!found) {
        L_INFO("nothing to do", procName);
        return pixd;
    }

        /* For each color in cmap that is close enough to srcval,
         * set the tab value to 1.  Then generate a 1 bpp mask with
         * fg pixels for every pixel in pixd that is close enough
         * to srcval (i.e., has value 1 in tab). */
    if ((tab = (l_int32 *)CALLOC(256, sizeof(l_int32))) == NULL)
        return (PIX *)ERROR_PTR("tab not made", procName, pixd);
    for (i = 0; i < ncolors; i++) {
        pixcmapGetColor(cmap, i, &rval, &gval, &bval);
        if ((L_ABS(rval - rsval) <= diff) &&
            (L_ABS(gval - gsval) <= diff) &&
            (L_ABS(bval - bsval) <= diff))
            tab[i] = 1;
    }
    pixm = pixMakeMaskFromLUT(pixd, tab);
    FREE(tab);

        /* Use the binary mask to set all selected pixels to
         * the dest color index. */
    pixSetMasked(pixd, pixm, dstval);
    pixDestroy(&pixm);

        /* Remove all unused colors from the colormap. */
    pixRemoveUnusedColors(pixd);

    return pixd;
}


/*------------------------------------------------------------------*
 *           Mapping colors based on a source/target pair           *
 *------------------------------------------------------------------*/
/*!
 *  pixLinearMapToTargetColor()
 *
 *      Input:  pixd (<optional>; either NULL or equal to pixs for in-place)
 *              pixs (32 bpp rgb)
 *              srcval (source color: 0xrrggbb00)
 *              dstval (target color: 0xrrggbb00)
 *      Return: pixd (with all pixels mapped based on the srcval/destval
 *                    mapping), or pixd on error
 *
 *  Notes:
 *      (1) For each component (r, b, g) separately, this does a piecewise
 *          linear mapping of the colors in pixs to colors in pixd.
 *          If rs and rd are the red src and dest components in @srcval and
 *          @dstval, then the range [0 ... rs] in pixs is mapped to
 *          [0 ... rd] in pixd.  Likewise, the range [rs ... 255] in pixs
 *          is mapped to [rd ... 255] in pixd.  And similarly for green
 *          and blue.
 *      (2) The mapping will in general change the hue of the pixels.
 *          However, if the src and dst targets are related by
 *          a transformation given by pixelFractionalShift(), the hue
 *          is invariant.
 *      (3) For inplace operation, call it this way:
 *            pixLinearMapToTargetColor(pixs, pixs, ... )
 *      (4) For generating a new pixd:
 *            pixd = pixLinearMapToTargetColor(NULL, pixs, ...)
 */
PIX *
pixLinearMapToTargetColor(PIX      *pixd,
                          PIX      *pixs,
                          l_uint32  srcval,
                          l_uint32  dstval)
{
l_int32    i, j, w, h, wpl;
l_int32    rval, gval, bval, rsval, gsval, bsval, rdval, gdval, bdval;
l_int32   *rtab, *gtab, *btab;
l_uint32   pixel;
l_uint32  *line, *data;

    PROCNAME("pixLinearMapToTargetColor");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixd && (pixd != pixs))
        return (PIX *)ERROR_PTR("pixd not null or == pixs", procName, pixd);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs is not 32 bpp", procName, pixd);

        /* Do the work on pixd */
    if (!pixd)
        pixd = pixCopy(NULL, pixs);

    extractRGBValues(srcval, &rsval, &gsval, &bsval);
    extractRGBValues(dstval, &rdval, &gdval, &bdval);
    rsval = L_MIN(254, L_MAX(1, rsval));
    gsval = L_MIN(254, L_MAX(1, gsval));
    bsval = L_MIN(254, L_MAX(1, bsval));
    rtab = (l_int32 *)CALLOC(256, sizeof(l_int32));
    gtab = (l_int32 *)CALLOC(256, sizeof(l_int32));
    btab = (l_int32 *)CALLOC(256, sizeof(l_int32));
    for (i = 0; i < 256; i++) {
        if (i <= rsval)
            rtab[i] = (i * rdval) / rsval;
        else
            rtab[i] = rdval + ((255 - rdval) * (i - rsval)) / (255 - rsval);
        if (i <= gsval)
            gtab[i] = (i * gdval) / gsval;
        else
            gtab[i] = gdval + ((255 - gdval) * (i - gsval)) / (255 - gsval);
        if (i <= bsval)
            btab[i] = (i * bdval) / bsval;
        else
            btab[i] = bdval + ((255 - bdval) * (i - bsval)) / (255 - bsval);
    }
    pixGetDimensions(pixd, &w, &h, NULL);
    data = pixGetData(pixd);
    wpl = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < w; j++) {
            pixel = line[j];
            extractRGBValues(pixel, &rval, &gval, &bval);
            composeRGBPixel(rtab[rval], gtab[gval], btab[bval], &pixel);
            line[j] = pixel;
        }
    }

    FREE(rtab);
    FREE(gtab);
    FREE(btab);
    return pixd;
}


/*!
 *  pixelLinearMapToTargetColor()
 *
 *      Input:  scolor (rgb source color: 0xrrggbb00)
 *              srcmap (source mapping color: 0xrrggbb00)
 *              dstmap (target mapping color: 0xrrggbb00)
 *              &pdcolor (<return> rgb dest color: 0xrrggbb00)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This does this does a piecewise linear mapping of each
 *          component of @scolor to @dcolor, based on the relation
 *          between the components of @srcmap and @dstmap.  It is the
 *          same transformation, performed on a single color, as mapped
 *          on every pixel in a pix by pixLinearMapToTargetColor().
 *      (2) For each component, if the sval is larger than the smap,
 *          the dval will be pushed up from dmap towards white.
 *          Otherwise, dval will be pushed down from dmap towards black.
 *          This is because you can visualize the transformation as
 *          a linear stretching where smap moves to dmap, and everything
 *          else follows linearly with 0 and 255 fixed.
 *      (3) The mapping will in general change the hue of @scolor.
 *          However, if the @srcmap and @dstmap targets are related by
 *          a transformation given by pixelFractionalShift(), the hue
 *          will be invariant.
 */
l_int32
pixelLinearMapToTargetColor(l_uint32   scolor,
                            l_uint32   srcmap,
                            l_uint32   dstmap,
                            l_uint32  *pdcolor)
{
l_int32    srval, sgval, sbval, drval, dgval, dbval;
l_int32    srmap, sgmap, sbmap, drmap, dgmap, dbmap;

    PROCNAME("pixelLinearMapToTargetColor");

    if (!pdcolor)
        return ERROR_INT("&dcolor not defined", procName, 1);
    *pdcolor = 0;

    extractRGBValues(scolor, &srval, &sgval, &sbval);
    extractRGBValues(srcmap, &srmap, &sgmap, &sbmap);
    extractRGBValues(dstmap, &drmap, &dgmap, &dbmap);
    srmap = L_MIN(254, L_MAX(1, srmap));
    sgmap = L_MIN(254, L_MAX(1, sgmap));
    sbmap = L_MIN(254, L_MAX(1, sbmap));

    if (srval < srmap)
        drval = (srval * drmap) / srmap;
    else
        drval = drmap + ((255 - drmap) * (srval - srmap)) / (255 - srmap);
    if (sgval < sgmap)
        dgval = (sgval * dgmap) / sgmap;
    else
        dgval = dgmap + ((255 - dgmap) * (sgval - sgmap)) / (255 - sgmap);
    if (sbval < sbmap)
        dbval = (sbval * dbmap) / sbmap;
    else
        dbval = dbmap + ((255 - dbmap) * (sbval - sbmap)) / (255 - sbmap);

    composeRGBPixel(drval, dgval, dbval, pdcolor);
    return 0;
}


/*------------------------------------------------------------------*
 *          Fractional shift of RGB towards black or white          *
 *------------------------------------------------------------------*/
/*!
 *  pixelFractionalShift()
 *
 *      Input:  rval, gval, bval
 *              fraction (negative toward black; positive toward white)
 *              &ppixel (<return> rgb value)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This transformation leaves the hue invariant, while changing
 *          the saturation and intensity.  It can be used for that
 *          purpose in pixLinearMapToTargetColor().
 *      (2) @fraction is in the range [-1 .... +1].  If @fraction < 0,
 *          saturation is increased and brightness is reduced.  The
 *          opposite results if @fraction > 0.  If @fraction == -1,
 *          the resulting pixel is black; @fraction == 1 results in white.
 */
l_int32
pixelFractionalShift(l_int32    rval,
                     l_int32    gval,
                     l_int32    bval,
                     l_float32  fraction,
                     l_uint32  *ppixel)
{
l_int32    nrval, ngval, nbval;

    PROCNAME("pixelFractionalShift");

    if (!ppixel)
        return ERROR_INT("&pixel defined", procName, 1);
    if (fraction < -1.0 || fraction > 1.0)
        return ERROR_INT("fraction not in [-1 ... +1]", procName, 1);

    nrval = (fraction < 0) ? (l_int32)((1.0 + fraction) * rval + 0.5) :
            rval + (l_int32)(fraction * (255 - rval) + 0.5);
    ngval = (fraction < 0) ? (l_int32)((1.0 + fraction) * gval + 0.5) :
            gval + (l_int32)(fraction * (255 - gval) + 0.5);
    nbval = (fraction < 0) ? (l_int32)((1.0 + fraction) * bval + 0.5) :
            bval + (l_int32)(fraction * (255 - bval) + 0.5);
    composeRGBPixel(nrval, ngval, nbval, ppixel);
    return 0;
}



