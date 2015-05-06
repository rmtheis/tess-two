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
 *  adaptmap.c
 *
 *  ===================================================================
 *  Image binarization algorithms are found in:
 *     grayquant.c:   standard, simple, general grayscale quantization
 *     adaptmap.c:    local adaptive; mostly gray-to-gray in preparation
 *                    for binarization
 *     binarize.c:    special binarization methods, locally adaptive.
 *  ===================================================================
 *
 *      Adaptive background normalization (top-level functions)
 *          PIX       *pixBackgroundNormSimple()     8 and 32 bpp
 *          PIX       *pixBackgroundNorm()           8 and 32 bpp
 *          PIX       *pixBackgroundNormMorph()      8 and 32 bpp
 *
 *      Arrays of inverted background values for normalization (16 bpp)
 *          l_int32    pixBackgroundNormGrayArray()   8 bpp input
 *          l_int32    pixBackgroundNormRGBArrays()   32 bpp input
 *          l_int32    pixBackgroundNormGrayArrayMorph()   8 bpp input
 *          l_int32    pixBackgroundNormRGBArraysMorph()   32 bpp input
 *
 *      Measurement of local background
 *          l_int32    pixGetBackgroundGrayMap()        8 bpp
 *          l_int32    pixGetBackgroundRGBMap()         32 bpp
 *          l_int32    pixGetBackgroundGrayMapMorph()   8 bpp
 *          l_int32    pixGetBackgroundRGBMapMorph()    32 bpp
 *          l_int32    pixFillMapHoles()
 *          PIX       *pixExtendByReplication()         8 bpp
 *          l_int32    pixSmoothConnectedRegions()      8 bpp
 *
 *      Measurement of local foreground
 *          l_int32    pixGetForegroundGrayMap()        8 bpp
 *
 *      Generate inverted background map for each component
 *          PIX       *pixGetInvBackgroundMap()   16 bpp
 *
 *      Apply inverse background map to image
 *          PIX       *pixApplyInvBackgroundGrayMap()   8 bpp
 *          PIX       *pixApplyInvBackgroundRGBMap()    32 bpp
 *
 *      Apply variable map
 *          PIX       *pixApplyVariableGrayMap()        8 bpp
 *
 *      Non-adaptive (global) mapping
 *          PIX       *pixGlobalNormRGB()               32 bpp or cmapped
 *          PIX       *pixGlobalNormNoSatRGB()          32 bpp
 *
 *      Adaptive threshold spread normalization
 *          l_int32    pixThresholdSpreadNorm()         8 bpp
 *
 *      Adaptive background normalization (flexible adaptaption)
 *          PIX       *pixBackgroundNormFlex()          8 bpp
 *
 *      Adaptive contrast normalization
 *          PIX             *pixContrastNorm()          8 bpp
 *          l_int32          pixMinMaxTiles()
 *          l_int32          pixSetLowContrast()
 *          PIX             *pixLinearTRCTiled()
 *          static l_int32  *iaaGetLinearTRC()
 *
 *  Background normalization is done by generating a reduced map (or set
 *  of maps) representing the estimated background value of the
 *  input image, and using this to shift the pixel values so that
 *  this background value is set to some constant value.
 *
 *  Specifically, normalization has 3 steps:
 *    (1) Generate a background map at a reduced scale.
 *    (2) Make the array of inverted background values by inverting
 *        the map.  The result is an array of local multiplicative factors.
 *    (3) Apply this inverse background map to the image
 *
 *  The inverse background arrays can be generated in two different ways here:
 *    (1) Remove the 'foreground' pixels and average over the remaining
 *        pixels in each tile.  Propagate values into tiles where
 *        values have not been assigned, either because there was not
 *        enough background in the tile or because the tile is covered
 *        by a foreground region described by an image mask.
 *        After the background map is made, the inverse map is generated by
 *        smoothing over some number of adjacent tiles
 *        (block convolution) and then inverting.
 *    (2) Remove the foreground pixels using a morphological closing
 *        on a subsampled version of the image.  Propagate values
 *        into pixels covered by an optional image mask.  Invert the
 *        background map without preconditioning by convolutional smoothing.
 *
 *  Note: Several of these functions make an implicit assumption about RGB
 *        component ordering.
 *
 *  Other methods for adaptively normalizing the image are also given here.
 *
 *  (1) pixThresholdSpreadNorm() computes a local threshold over the image
 *      and normalizes the input pixel values so that this computed threshold
 *      is a constant across the entire image.
 *
 *  (2) pixContrastNorm() computes and applies a local TRC so that the
 *      local dynamic range is expanded to the full 8 bits, where the
 *      darkest pixels are mapped to 0 and the lightest to 255.  This is
 *      useful for improving the appearance of pages with very light
 *      foreground or very dark background, and where the local TRC
 *      function doesn't change rapidly with position.
 */

#include "allheaders.h"

    /* Default input parameters for pixBackgroundNormSimple()
     * Note:
     *    (1) mincount must never exceed the tile area (width * height)
     *    (2) bgval must be sufficiently below 255 to avoid accidental
     *        saturation; otherwise it should be large to avoid
     *        shrinking the dynamic range
     *    (3) results should otherwise not be sensitive to these values
     */
static const l_int32  DEFAULT_TILE_WIDTH = 10;
static const l_int32  DEFAULT_TILE_HEIGHT = 15;
static const l_int32  DEFAULT_FG_THRESHOLD = 60;
static const l_int32  DEFAULT_MIN_COUNT = 40;
static const l_int32  DEFAULT_BG_VAL = 200;
static const l_int32  DEFAULT_X_SMOOTH_SIZE = 2;
static const l_int32  DEFAULT_Y_SMOOTH_SIZE = 1;

static l_int32 *iaaGetLinearTRC(l_int32 **iaa, l_int32 diff);

#ifndef  NO_CONSOLE_IO
#define  DEBUG_GLOBAL    0
#endif  /* ~NO_CONSOLE_IO */


/*------------------------------------------------------------------*
 *                Adaptive background normalization                 *
 *------------------------------------------------------------------*/
/*!
 *  pixBackgroundNormSimple()
 *
 *      Input:  pixs (8 bpp grayscale or 32 bpp rgb)
 *              pixim (<optional> 1 bpp 'image' mask; can be null)
 *              pixg (<optional> 8 bpp grayscale version; can be null)
 *      Return: pixd (8 bpp or 32 bpp rgb), or null on error
 *
 *  Notes:
 *    (1) This is a simplified interface to pixBackgroundNorm(),
 *        where seven parameters are defaulted.
 *    (2) The input image is either grayscale or rgb.
 *    (3) See pixBackgroundNorm() for usage and function.
 */
PIX *
pixBackgroundNormSimple(PIX  *pixs,
                        PIX  *pixim,
                        PIX  *pixg)
{
    return pixBackgroundNorm(pixs, pixim, pixg,
                             DEFAULT_TILE_WIDTH, DEFAULT_TILE_HEIGHT,
                             DEFAULT_FG_THRESHOLD, DEFAULT_MIN_COUNT,
                             DEFAULT_BG_VAL, DEFAULT_X_SMOOTH_SIZE,
                             DEFAULT_Y_SMOOTH_SIZE);
}


/*!
 *  pixBackgroundNorm()
 *
 *      Input:  pixs (8 bpp grayscale or 32 bpp rgb)
 *              pixim (<optional> 1 bpp 'image' mask; can be null)
 *              pixg (<optional> 8 bpp grayscale version; can be null)
 *              sx, sy (tile size in pixels)
 *              thresh (threshold for determining foreground)
 *              mincount (min threshold on counts in a tile)
 *              bgval (target bg val; typ. > 128)
 *              smoothx (half-width of block convolution kernel width)
 *              smoothy (half-width of block convolution kernel height)
 *      Return: pixd (8 bpp or 32 bpp rgb), or null on error
 *
 *  Notes:
 *    (1) This is a top-level interface for normalizing the image intensity
 *        by mapping the image so that the background is near the input
 *        value 'bgval'.
 *    (2) The input image is either grayscale or rgb.
 *    (3) For each component in the input image, the background value
 *        in each tile is estimated using the values in the tile that
 *        are not part of the foreground, where the foreground is
 *        determined by the input 'thresh' argument.
 *    (4) An optional binary mask can be specified, with the foreground
 *        pixels typically over image regions.  The resulting background
 *        map values will be determined by surrounding pixels that are
 *        not under the mask foreground.  The origin (0,0) of this mask
 *        is assumed to be aligned with the origin of the input image.
 *        This binary mask must not fully cover pixs, because then there
 *        will be no pixels in the input image available to compute
 *        the background.
 *    (5) An optional grayscale version of the input pixs can be supplied.
 *        The only reason to do this is if the input is RGB and this
 *        grayscale version can be used elsewhere.  If the input is RGB
 *        and this is not supplied, it is made internally using only
 *        the green component, and destroyed after use.
 *    (6) The dimensions of the pixel tile (sx, sy) give the amount by
 *        by which the map is reduced in size from the input image.
 *    (7) The threshold is used to binarize the input image, in order to
 *        locate the foreground components.  If this is set too low,
 *        some actual foreground may be used to determine the maps;
 *        if set too high, there may not be enough background
 *        to determine the map values accurately.  Typically, it's
 *        better to err by setting the threshold too high.
 *    (8) A 'mincount' threshold is a minimum count of pixels in a
 *        tile for which a background reading is made, in order for that
 *        pixel in the map to be valid.  This number should perhaps be
 *        at least 1/3 the size of the tile.
 *    (9) A 'bgval' target background value for the normalized image.  This
 *        should be at least 128.  If set too close to 255, some
 *        clipping will occur in the result.
 *    (10) Two factors, 'smoothx' and 'smoothy', are input for smoothing
 *        the map.  Each low-pass filter kernel dimension is
 *        is 2 * (smoothing factor) + 1, so a
 *        value of 0 means no smoothing. A value of 1 or 2 is recommended.
 */
PIX *
pixBackgroundNorm(PIX     *pixs,
                  PIX     *pixim,
                  PIX     *pixg,
                  l_int32  sx,
                  l_int32  sy,
                  l_int32  thresh,
                  l_int32  mincount,
                  l_int32  bgval,
                  l_int32  smoothx,
                  l_int32  smoothy)
{
l_int32  d, allfg;
PIX     *pixm, *pixmi, *pixd;
PIX     *pixmr, *pixmg, *pixmb, *pixmri, *pixmgi, *pixmbi;

    PROCNAME("pixBackgroundNorm");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    if (d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pixs not 8 or 32 bpp", procName, NULL);
    if (sx < 4 || sy < 4)
        return (PIX *)ERROR_PTR("sx and sy must be >= 4", procName, NULL);
    if (mincount > sx * sy) {
        L_WARNING("mincount too large for tile size\n", procName);
        mincount = (sx * sy) / 3;
    }

        /* If pixim exists, verify that it is not all foreground. */
    if (pixim) {
        pixInvert(pixim, pixim);
        pixZero(pixim, &allfg);
        pixInvert(pixim, pixim);
        if (allfg)
            return (PIX *)ERROR_PTR("pixim all foreground", procName, NULL);
    }

    pixd = NULL;
    if (d == 8) {
        pixm = NULL;
        pixGetBackgroundGrayMap(pixs, pixim, sx, sy, thresh, mincount, &pixm);
        if (!pixm) {
            L_WARNING("map not made; return a copy of the source\n", procName);
            return pixCopy(NULL, pixs);
        }

        pixmi = pixGetInvBackgroundMap(pixm, bgval, smoothx, smoothy);
        if (!pixmi)
            ERROR_PTR("pixmi not made", procName, NULL);
        else
            pixd = pixApplyInvBackgroundGrayMap(pixs, pixmi, sx, sy);

        pixDestroy(&pixm);
        pixDestroy(&pixmi);
    }
    else {
        pixmr = pixmg = pixmb = NULL;
        pixGetBackgroundRGBMap(pixs, pixim, pixg, sx, sy, thresh,
                               mincount, &pixmr, &pixmg, &pixmb);
        if (!pixmr || !pixmg || !pixmb) {
            pixDestroy(&pixmr);
            pixDestroy(&pixmg);
            pixDestroy(&pixmb);
            L_WARNING("map not made; return a copy of the source\n", procName);
            return pixCopy(NULL, pixs);
        }

        pixmri = pixGetInvBackgroundMap(pixmr, bgval, smoothx, smoothy);
        pixmgi = pixGetInvBackgroundMap(pixmg, bgval, smoothx, smoothy);
        pixmbi = pixGetInvBackgroundMap(pixmb, bgval, smoothx, smoothy);
        if (!pixmri || !pixmgi || !pixmbi)
            ERROR_PTR("not all pixm*i are made", procName, NULL);
        else
            pixd = pixApplyInvBackgroundRGBMap(pixs, pixmri, pixmgi, pixmbi,
                                               sx, sy);

        pixDestroy(&pixmr);
        pixDestroy(&pixmg);
        pixDestroy(&pixmb);
        pixDestroy(&pixmri);
        pixDestroy(&pixmgi);
        pixDestroy(&pixmbi);
    }

    if (!pixd)
        ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    return pixd;
}


/*!
 *  pixBackgroundNormMorph()
 *
 *      Input:  pixs (8 bpp grayscale or 32 bpp rgb)
 *              pixim (<optional> 1 bpp 'image' mask; can be null)
 *              reduction (at which morph closings are done; between 2 and 16)
 *              size (of square Sel for the closing; use an odd number)
 *              bgval (target bg val; typ. > 128)
 *      Return: pixd (8 bpp), or null on error
 *
 *  Notes:
 *    (1) This is a top-level interface for normalizing the image intensity
 *        by mapping the image so that the background is near the input
 *        value 'bgval'.
 *    (2) The input image is either grayscale or rgb.
 *    (3) For each component in the input image, the background value
 *        is estimated using a grayscale closing; hence the 'Morph'
 *        in the function name.
 *    (4) An optional binary mask can be specified, with the foreground
 *        pixels typically over image regions.  The resulting background
 *        map values will be determined by surrounding pixels that are
 *        not under the mask foreground.  The origin (0,0) of this mask
 *        is assumed to be aligned with the origin of the input image.
 *        This binary mask must not fully cover pixs, because then there
 *        will be no pixels in the input image available to compute
 *        the background.
 *    (5) The map is computed at reduced size (given by 'reduction')
 *        from the input pixs and optional pixim.  At this scale,
 *        pixs is closed to remove the background, using a square Sel
 *        of odd dimension.  The product of reduction * size should be
 *        large enough to remove most of the text foreground.
 *    (6) No convolutional smoothing needs to be done on the map before
 *        inverting it.
 *    (7) A 'bgval' target background value for the normalized image.  This
 *        should be at least 128.  If set too close to 255, some
 *        clipping will occur in the result.
 */
PIX *
pixBackgroundNormMorph(PIX     *pixs,
                       PIX     *pixim,
                       l_int32  reduction,
                       l_int32  size,
                       l_int32  bgval)
{
l_int32    d, allfg;
PIX       *pixm, *pixmi, *pixd;
PIX       *pixmr, *pixmg, *pixmb, *pixmri, *pixmgi, *pixmbi;

    PROCNAME("pixBackgroundNormMorph");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    if (d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pixs not 8 or 32 bpp", procName, NULL);
    if (reduction < 2 || reduction > 16)
        return (PIX *)ERROR_PTR("reduction must be between 2 and 16",
                                procName, NULL);

        /* If pixim exists, verify that it is not all foreground. */
    if (pixim) {
        pixInvert(pixim, pixim);
        pixZero(pixim, &allfg);
        pixInvert(pixim, pixim);
        if (allfg)
            return (PIX *)ERROR_PTR("pixim all foreground", procName, NULL);
    }

    pixd = NULL;
    if (d == 8) {
        pixGetBackgroundGrayMapMorph(pixs, pixim, reduction, size, &pixm);
        if (!pixm)
            return (PIX *)ERROR_PTR("pixm not made", procName, NULL);
        pixmi = pixGetInvBackgroundMap(pixm, bgval, 0, 0);
        if (!pixmi)
            ERROR_PTR("pixmi not made", procName, NULL);
        else
            pixd = pixApplyInvBackgroundGrayMap(pixs, pixmi,
                                                reduction, reduction);
        pixDestroy(&pixm);
        pixDestroy(&pixmi);
    }
    else {  /* d == 32 */
        pixmr = pixmg = pixmb = NULL;
        pixGetBackgroundRGBMapMorph(pixs, pixim, reduction, size,
                                    &pixmr, &pixmg, &pixmb);
        if (!pixmr || !pixmg || !pixmb) {
            pixDestroy(&pixmr);
            pixDestroy(&pixmg);
            pixDestroy(&pixmb);
            return (PIX *)ERROR_PTR("not all pixm*", procName, NULL);
        }

        pixmri = pixGetInvBackgroundMap(pixmr, bgval, 0, 0);
        pixmgi = pixGetInvBackgroundMap(pixmg, bgval, 0, 0);
        pixmbi = pixGetInvBackgroundMap(pixmb, bgval, 0, 0);
        if (!pixmri || !pixmgi || !pixmbi)
            ERROR_PTR("not all pixm*i are made", procName, NULL);
        else
            pixd = pixApplyInvBackgroundRGBMap(pixs, pixmri, pixmgi, pixmbi,
                                               reduction, reduction);

        pixDestroy(&pixmr);
        pixDestroy(&pixmg);
        pixDestroy(&pixmb);
        pixDestroy(&pixmri);
        pixDestroy(&pixmgi);
        pixDestroy(&pixmbi);
    }

    if (!pixd)
        ERROR_PTR("pixd not made", procName, NULL);
    return pixd;
}


/*-------------------------------------------------------------------------*
 *      Arrays of inverted background values for normalization             *
 *-------------------------------------------------------------------------*
 *  Notes for these four functions:                                        *
 *      (1) They are useful if you need to save the actual mapping array.  *
 *      (2) They could be used in the top-level functions but are          *
 *          not because their use makes those functions less clear.        *
 *      (3) Each component in the input pixs generates a 16 bpp pix array. *
 *-------------------------------------------------------------------------*/
/*!
 *  pixBackgroundNormGrayArray()
 *
 *      Input:  pixs (8 bpp grayscale)
 *              pixim (<optional> 1 bpp 'image' mask; can be null)
 *              sx, sy (tile size in pixels)
 *              thresh (threshold for determining foreground)
 *              mincount (min threshold on counts in a tile)
 *              bgval (target bg val; typ. > 128)
 *              smoothx (half-width of block convolution kernel width)
 *              smoothy (half-width of block convolution kernel height)
 *              &pixd (<return> 16 bpp array of inverted background value)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *    (1) See notes in pixBackgroundNorm().
 *    (2) This returns a 16 bpp pix that can be used by
 *        pixApplyInvBackgroundGrayMap() to generate a normalized version
 *        of the input pixs.
 */
l_int32
pixBackgroundNormGrayArray(PIX     *pixs,
                           PIX     *pixim,
                           l_int32  sx,
                           l_int32  sy,
                           l_int32  thresh,
                           l_int32  mincount,
                           l_int32  bgval,
                           l_int32  smoothx,
                           l_int32  smoothy,
                           PIX    **ppixd)
{
l_int32  allfg;
PIX     *pixm;

    PROCNAME("pixBackgroundNormGrayArray");

    if (!ppixd)
        return ERROR_INT("&pixd not defined", procName, 1);
    *ppixd = NULL;
    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (pixGetColormap(pixs))
        return ERROR_INT("pixs is colormapped", procName, 1);
    if (pixim && pixGetDepth(pixim) != 1)
        return ERROR_INT("pixim not 1 bpp", procName, 1);
    if (sx < 4 || sy < 4)
        return ERROR_INT("sx and sy must be >= 4", procName, 1);
    if (mincount > sx * sy) {
        L_WARNING("mincount too large for tile size\n", procName);
        mincount = (sx * sy) / 3;
    }

        /* If pixim exists, verify that it is not all foreground. */
    if (pixim) {
        pixInvert(pixim, pixim);
        pixZero(pixim, &allfg);
        pixInvert(pixim, pixim);
        if (allfg)
            return ERROR_INT("pixim all foreground", procName, 1);
    }

    pixGetBackgroundGrayMap(pixs, pixim, sx, sy, thresh, mincount, &pixm);
    if (!pixm)
        return ERROR_INT("pixm not made", procName, 1);
    *ppixd = pixGetInvBackgroundMap(pixm, bgval, smoothx, smoothy);
    pixCopyResolution(*ppixd, pixs);
    pixDestroy(&pixm);
    return 0;
}


/*!
 *  pixBackgroundNormRGBArrays()
 *
 *      Input:  pixs (32 bpp rgb)
 *              pixim (<optional> 1 bpp 'image' mask; can be null)
 *              pixg (<optional> 8 bpp grayscale version; can be null)
 *              sx, sy (tile size in pixels)
 *              thresh (threshold for determining foreground)
 *              mincount (min threshold on counts in a tile)
 *              bgval (target bg val; typ. > 128)
 *              smoothx (half-width of block convolution kernel width)
 *              smoothy (half-width of block convolution kernel height)
 *              &pixr (<return> 16 bpp array of inverted R background value)
 *              &pixg (<return> 16 bpp array of inverted G background value)
 *              &pixb (<return> 16 bpp array of inverted B background value)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *    (1) See notes in pixBackgroundNorm().
 *    (2) This returns a set of three 16 bpp pix that can be used by
 *        pixApplyInvBackgroundGrayMap() to generate a normalized version
 *        of each component of the input pixs.
 */
l_int32
pixBackgroundNormRGBArrays(PIX     *pixs,
                           PIX     *pixim,
                           PIX     *pixg,
                           l_int32  sx,
                           l_int32  sy,
                           l_int32  thresh,
                           l_int32  mincount,
                           l_int32  bgval,
                           l_int32  smoothx,
                           l_int32  smoothy,
                           PIX    **ppixr,
                           PIX    **ppixg,
                           PIX    **ppixb)
{
l_int32  allfg;
PIX     *pixmr, *pixmg, *pixmb;

    PROCNAME("pixBackgroundNormRGBArrays");

    if (!ppixr || !ppixg || !ppixb)
        return ERROR_INT("&pixr, &pixg, &pixb not all defined", procName, 1);
    *ppixr = *ppixg = *ppixb = NULL;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetDepth(pixs) != 32)
        return ERROR_INT("pixs not 32 bpp", procName, 1);
    if (pixim && pixGetDepth(pixim) != 1)
        return ERROR_INT("pixim not 1 bpp", procName, 1);
    if (sx < 4 || sy < 4)
        return ERROR_INT("sx and sy must be >= 4", procName, 1);
    if (mincount > sx * sy) {
        L_WARNING("mincount too large for tile size\n", procName);
        mincount = (sx * sy) / 3;
    }

        /* If pixim exists, verify that it is not all foreground. */
    if (pixim) {
        pixInvert(pixim, pixim);
        pixZero(pixim, &allfg);
        pixInvert(pixim, pixim);
        if (allfg)
            return ERROR_INT("pixim all foreground", procName, 1);
    }

    pixGetBackgroundRGBMap(pixs, pixim, pixg, sx, sy, thresh, mincount,
                           &pixmr, &pixmg, &pixmb);
    if (!pixmr || !pixmg || !pixmb) {
        pixDestroy(&pixmr);
        pixDestroy(&pixmg);
        pixDestroy(&pixmb);
        return ERROR_INT("not all pixm* made", procName, 1);
    }

    *ppixr = pixGetInvBackgroundMap(pixmr, bgval, smoothx, smoothy);
    *ppixg = pixGetInvBackgroundMap(pixmg, bgval, smoothx, smoothy);
    *ppixb = pixGetInvBackgroundMap(pixmb, bgval, smoothx, smoothy);
    pixDestroy(&pixmr);
    pixDestroy(&pixmg);
    pixDestroy(&pixmb);
    return 0;
}


/*!
 *  pixBackgroundNormGrayArrayMorph()
 *
 *      Input:  pixs (8 bpp grayscale)
 *              pixim (<optional> 1 bpp 'image' mask; can be null)
 *              reduction (at which morph closings are done; between 2 and 16)
 *              size (of square Sel for the closing; use an odd number)
 *              bgval (target bg val; typ. > 128)
 *              &pixd (<return> 16 bpp array of inverted background value)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *    (1) See notes in pixBackgroundNormMorph().
 *    (2) This returns a 16 bpp pix that can be used by
 *        pixApplyInvBackgroundGrayMap() to generate a normalized version
 *        of the input pixs.
 */
l_int32
pixBackgroundNormGrayArrayMorph(PIX     *pixs,
                                PIX     *pixim,
                                l_int32  reduction,
                                l_int32  size,
                                l_int32  bgval,
                                PIX    **ppixd)
{
l_int32  allfg;
PIX     *pixm;

    PROCNAME("pixBackgroundNormGrayArrayMorph");

    if (!ppixd)
        return ERROR_INT("&pixd not defined", procName, 1);
    *ppixd = NULL;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not 8 bpp", procName, 1);
    if (pixim && pixGetDepth(pixim) != 1)
        return ERROR_INT("pixim not 1 bpp", procName, 1);
    if (reduction < 2 || reduction > 16)
        return ERROR_INT("reduction must be between 2 and 16", procName, 1);

        /* If pixim exists, verify that it is not all foreground. */
    if (pixim) {
        pixInvert(pixim, pixim);
        pixZero(pixim, &allfg);
        pixInvert(pixim, pixim);
        if (allfg)
            return ERROR_INT("pixim all foreground", procName, 1);
    }

    pixGetBackgroundGrayMapMorph(pixs, pixim, reduction, size, &pixm);
    if (!pixm)
        return ERROR_INT("pixm not made", procName, 1);
    *ppixd = pixGetInvBackgroundMap(pixm, bgval, 0, 0);
    pixCopyResolution(*ppixd, pixs);
    pixDestroy(&pixm);
    return 0;
}


/*!
 *  pixBackgroundNormRGBArraysMorph()
 *
 *      Input:  pixs (32 bpp rgb)
 *              pixim (<optional> 1 bpp 'image' mask; can be null)
 *              reduction (at which morph closings are done; between 2 and 16)
 *              size (of square Sel for the closing; use an odd number)
 *              bgval (target bg val; typ. > 128)
 *              &pixr (<return> 16 bpp array of inverted R background value)
 *              &pixg (<return> 16 bpp array of inverted G background value)
 *              &pixb (<return> 16 bpp array of inverted B background value)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *    (1) See notes in pixBackgroundNormMorph().
 *    (2) This returns a set of three 16 bpp pix that can be used by
 *        pixApplyInvBackgroundGrayMap() to generate a normalized version
 *        of each component of the input pixs.
 */
l_int32
pixBackgroundNormRGBArraysMorph(PIX     *pixs,
                                PIX     *pixim,
                                l_int32  reduction,
                                l_int32  size,
                                l_int32  bgval,
                                PIX    **ppixr,
                                PIX    **ppixg,
                                PIX    **ppixb)
{
l_int32  allfg;
PIX     *pixmr, *pixmg, *pixmb;

    PROCNAME("pixBackgroundNormRGBArraysMorph");

    if (!ppixr || !ppixg || !ppixb)
        return ERROR_INT("&pixr, &pixg, &pixb not all defined", procName, 1);
    *ppixr = *ppixg = *ppixb = NULL;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetDepth(pixs) != 32)
        return ERROR_INT("pixs not 32 bpp", procName, 1);
    if (pixim && pixGetDepth(pixim) != 1)
        return ERROR_INT("pixim not 1 bpp", procName, 1);
    if (reduction < 2 || reduction > 16)
        return ERROR_INT("reduction must be between 2 and 16", procName, 1);

        /* If pixim exists, verify that it is not all foreground. */
    if (pixim) {
        pixInvert(pixim, pixim);
        pixZero(pixim, &allfg);
        pixInvert(pixim, pixim);
        if (allfg)
            return ERROR_INT("pixim all foreground", procName, 1);
    }

    pixGetBackgroundRGBMapMorph(pixs, pixim, reduction, size,
                                &pixmr, &pixmg, &pixmb);
    if (!pixmr || !pixmg || !pixmb) {
        pixDestroy(&pixmr);
        pixDestroy(&pixmg);
        pixDestroy(&pixmb);
        return ERROR_INT("not all pixm* made", procName, 1);
    }

    *ppixr = pixGetInvBackgroundMap(pixmr, bgval, 0, 0);
    *ppixg = pixGetInvBackgroundMap(pixmg, bgval, 0, 0);
    *ppixb = pixGetInvBackgroundMap(pixmb, bgval, 0, 0);
    pixDestroy(&pixmr);
    pixDestroy(&pixmg);
    pixDestroy(&pixmb);
    return 0;
}


/*------------------------------------------------------------------*
 *                 Measurement of local background                  *
 *------------------------------------------------------------------*/
/*!
 *  pixGetBackgroundGrayMap()
 *
 *      Input:  pixs (8 bpp grayscale; not cmapped)
 *              pixim (<optional> 1 bpp 'image' mask; can be null; it
 *                     should not have all foreground pixels)
 *              sx, sy (tile size in pixels)
 *              thresh (threshold for determining foreground)
 *              mincount (min threshold on counts in a tile)
 *              &pixd (<return> 8 bpp grayscale map)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The background is measured in regions that don't have
 *          images.  It is then propagated into the image regions,
 *          and finally smoothed in each image region.
 */
l_int32
pixGetBackgroundGrayMap(PIX     *pixs,
                        PIX     *pixim,
                        l_int32  sx,
                        l_int32  sy,
                        l_int32  thresh,
                        l_int32  mincount,
                        PIX    **ppixd)
{
l_int32    w, h, wd, hd, wim, him, wpls, wplim, wpld, wplf;
l_int32    xim, yim, delx, nx, ny, i, j, k, m;
l_int32    count, sum, val8;
l_int32    empty, fgpixels;
l_uint32  *datas, *dataim, *datad, *dataf, *lines, *lineim, *lined, *linef;
l_float32  scalex, scaley;
PIX       *pixd, *piximi, *pixb, *pixf, *pixims;

    PROCNAME("pixGetBackgroundGrayMap");

    if (!ppixd)
        return ERROR_INT("&pixd not defined", procName, 1);
    *ppixd = NULL;
    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (pixGetColormap(pixs))
        return ERROR_INT("pixs is colormapped", procName, 1);
    if (pixim && pixGetDepth(pixim) != 1)
        return ERROR_INT("pixim not 1 bpp", procName, 1);
    if (sx < 4 || sy < 4)
        return ERROR_INT("sx and sy must be >= 4", procName, 1);
    if (mincount > sx * sy) {
        L_WARNING("mincount too large for tile size\n", procName);
        mincount = (sx * sy) / 3;
    }

        /* Evaluate the 'image' mask, pixim, and make sure
         * it is not all fg. */
    fgpixels = 0;  /* boolean for existence of fg pixels in the image mask. */
    if (pixim) {
        piximi = pixInvert(NULL, pixim);  /* set non-'image' pixels to 1 */
        pixZero(piximi, &empty);
        pixDestroy(&piximi);
        if (empty)
            return ERROR_INT("pixim all fg; no background", procName, 1);
        pixZero(pixim, &empty);
        if (!empty)  /* there are fg pixels in pixim */
            fgpixels = 1;
    }

        /* Generate the foreground mask, pixf, which is at
         * full resolution.  These pixels will be ignored when
         * computing the background values. */
    pixb = pixThresholdToBinary(pixs, thresh);
    pixf = pixMorphSequence(pixb, "d7.1 + d1.7", 0);
    pixDestroy(&pixb);


    /* ------------- Set up the output map pixd --------------- */
        /* Generate pixd, which is reduced by the factors (sx, sy). */
    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);
    wd = (w + sx - 1) / sx;
    hd = (h + sy - 1) / sy;
    pixd = pixCreate(wd, hd, 8);

        /* Note: we only compute map values in tiles that are complete.
         * In general, tiles at right and bottom edges will not be
         * complete, and we must fill them in later. */
    nx = w / sx;
    ny = h / sy;
    wpls = pixGetWpl(pixs);
    datas = pixGetData(pixs);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);
    wplf = pixGetWpl(pixf);
    dataf = pixGetData(pixf);
    for (i = 0; i < ny; i++) {
        lines = datas + sy * i * wpls;
        linef = dataf + sy * i * wplf;
        lined = datad + i * wpld;
        for (j = 0; j < nx; j++) {
            delx = j * sx;
            sum = 0;
            count = 0;
            for (k = 0; k < sy; k++) {
                for (m = 0; m < sx; m++) {
                    if (GET_DATA_BIT(linef + k * wplf, delx + m) == 0) {
                        sum += GET_DATA_BYTE(lines + k * wpls, delx + m);
                        count++;
                    }
                }
            }
            if (count >= mincount) {
                val8 = sum / count;
                SET_DATA_BYTE(lined, j, val8);
            }
        }
    }
    pixDestroy(&pixf);

        /* If there is an optional mask with fg pixels, erase the previous
         * calculation for the corresponding map pixels, setting the
         * map values to 0.   Then, when all the map holes are filled,
         * these erased pixels will be set by the surrounding map values.
         *
         * The calculation here is relatively efficient: for each pixel
         * in pixd (which corresponds to a tile of mask pixels in pixim)
         * we look only at the pixel in pixim that is at the center
         * of the tile.  If the mask pixel is ON, we reset the map
         * pixel in pixd to 0, so that it can later be filled in. */
    pixims = NULL;
    if (pixim && fgpixels) {
        wim = pixGetWidth(pixim);
        him = pixGetHeight(pixim);
        dataim = pixGetData(pixim);
        wplim = pixGetWpl(pixim);
        for (i = 0; i < ny; i++) {
            yim = i * sy + sy / 2;
            if (yim >= him)
                break;
            lineim = dataim + yim * wplim;
            for (j = 0; j < nx; j++) {
                xim = j * sx + sx / 2;
                if (xim >= wim)
                    break;
                if (GET_DATA_BIT(lineim, xim))
                    pixSetPixel(pixd, j, i, 0);
            }
        }
    }

        /* Fill all the holes in the map. */
    if (pixFillMapHoles(pixd, nx, ny, L_FILL_BLACK)) {
        pixDestroy(&pixd);
        L_WARNING("can't make the map\n", procName);
        return 1;
    }

        /* Finally, for each connected region corresponding to the
         * 'image' mask, reset all pixels to their average value.
         * Each of these components represents an image (or part of one)
         * in the input, and this smooths the background values
         * in each of these regions. */
    if (pixim && fgpixels) {
        scalex = 1. / (l_float32)sx;
        scaley = 1. / (l_float32)sy;
        pixims = pixScaleBySampling(pixim, scalex, scaley);
        pixSmoothConnectedRegions(pixd, pixims, 2);
        pixDestroy(&pixims);
    }

    *ppixd = pixd;
    pixCopyResolution(*ppixd, pixs);
    return 0;
}


/*!
 *  pixGetBackgroundRGBMap()
 *
 *      Input:  pixs (32 bpp rgb)
 *              pixim (<optional> 1 bpp 'image' mask; can be null; it
 *                     should not have all foreground pixels)
 *              pixg (<optional> 8 bpp grayscale version; can be null)
 *              sx, sy (tile size in pixels)
 *              thresh (threshold for determining foreground)
 *              mincount (min threshold on counts in a tile)
 *              &pixmr, &pixmg, &pixmb (<return> rgb maps)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If pixg, which is a grayscale version of pixs, is provided,
 *          use this internally to generate the foreground mask.
 *          Otherwise, a grayscale version of pixs will be generated
 *          from the green component only, used, and destroyed.
 */
l_int32
pixGetBackgroundRGBMap(PIX     *pixs,
                       PIX     *pixim,
                       PIX     *pixg,
                       l_int32  sx,
                       l_int32  sy,
                       l_int32  thresh,
                       l_int32  mincount,
                       PIX    **ppixmr,
                       PIX    **ppixmg,
                       PIX    **ppixmb)
{
l_int32    w, h, wm, hm, wim, him, wpls, wplim, wplf;
l_int32    xim, yim, delx, nx, ny, i, j, k, m;
l_int32    count, rsum, gsum, bsum, rval, gval, bval;
l_int32    empty, fgpixels;
l_uint32   pixel;
l_uint32  *datas, *dataim, *dataf, *lines, *lineim, *linef;
l_float32  scalex, scaley;
PIX       *piximi, *pixgc, *pixb, *pixf, *pixims;
PIX       *pixmr, *pixmg, *pixmb;

    PROCNAME("pixGetBackgroundRGBMap");

    if (!ppixmr || !ppixmg || !ppixmb)
        return ERROR_INT("&pixm* not all defined", procName, 1);
    *ppixmr = *ppixmg = *ppixmb = NULL;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetDepth(pixs) != 32)
        return ERROR_INT("pixs not 32 bpp", procName, 1);
    if (pixim && pixGetDepth(pixim) != 1)
        return ERROR_INT("pixim not 1 bpp", procName, 1);
    if (sx < 4 || sy < 4)
        return ERROR_INT("sx and sy must be >= 4", procName, 1);
    if (mincount > sx * sy) {
        L_WARNING("mincount too large for tile size\n", procName);
        mincount = (sx * sy) / 3;
    }

        /* Evaluate the mask pixim and make sure it is not all foreground */
    fgpixels = 0;  /* boolean for existence of fg mask pixels */
    if (pixim) {
        piximi = pixInvert(NULL, pixim);  /* set non-'image' pixels to 1 */
        pixZero(piximi, &empty);
        pixDestroy(&piximi);
        if (empty)
            return ERROR_INT("pixim all fg; no background", procName, 1);
        pixZero(pixim, &empty);
        if (!empty)  /* there are fg pixels in pixim */
            fgpixels = 1;
    }

        /* Generate the foreground mask.  These pixels will be
         * ignored when computing the background values. */
    if (pixg)  /* use the input grayscale version if it is provided */
        pixgc = pixClone(pixg);
    else
        pixgc = pixConvertRGBToGrayFast(pixs);
    pixb = pixThresholdToBinary(pixgc, thresh);
    pixf = pixMorphSequence(pixb, "d7.1 + d1.7", 0);
    pixDestroy(&pixgc);
    pixDestroy(&pixb);

        /* Generate the output mask images */
    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);
    wm = (w + sx - 1) / sx;
    hm = (h + sy - 1) / sy;
    pixmr = pixCreate(wm, hm, 8);
    pixmg = pixCreate(wm, hm, 8);
    pixmb = pixCreate(wm, hm, 8);

    /* ------------- Set up the mapping images --------------- */
        /* Note: we only compute map values in tiles that are complete.
         * In general, tiles at right and bottom edges will not be
         * complete, and we must fill them in later. */
    nx = w / sx;
    ny = h / sy;
    wpls = pixGetWpl(pixs);
    datas = pixGetData(pixs);
    wplf = pixGetWpl(pixf);
    dataf = pixGetData(pixf);
    for (i = 0; i < ny; i++) {
        lines = datas + sy * i * wpls;
        linef = dataf + sy * i * wplf;
        for (j = 0; j < nx; j++) {
            delx = j * sx;
            rsum = gsum = bsum = 0;
            count = 0;
            for (k = 0; k < sy; k++) {
                for (m = 0; m < sx; m++) {
                    if (GET_DATA_BIT(linef + k * wplf, delx + m) == 0) {
                        pixel = *(lines + k * wpls + delx + m);
                        rsum += (pixel >> 24);
                        gsum += ((pixel >> 16) & 0xff);
                        bsum += ((pixel >> 8) & 0xff);
                        count++;
                    }
                }
            }
            if (count >= mincount) {
                rval = rsum / count;
                gval = gsum / count;
                bval = bsum / count;
                pixSetPixel(pixmr, j, i, rval);
                pixSetPixel(pixmg, j, i, gval);
                pixSetPixel(pixmb, j, i, bval);
            }
        }
    }
    pixDestroy(&pixf);

        /* If there is an optional mask with fg pixels, erase the previous
         * calculation for the corresponding map pixels, setting the
         * map values in each of the 3 color maps to 0.   Then, when
         * all the map holes are filled, these erased pixels will
         * be set by the surrounding map values. */
    if (pixim) {
        wim = pixGetWidth(pixim);
        him = pixGetHeight(pixim);
        dataim = pixGetData(pixim);
        wplim = pixGetWpl(pixim);
        for (i = 0; i < ny; i++) {
            yim = i * sy + sy / 2;
            if (yim >= him)
                break;
            lineim = dataim + yim * wplim;
            for (j = 0; j < nx; j++) {
                xim = j * sx + sx / 2;
                if (xim >= wim)
                    break;
                if (GET_DATA_BIT(lineim, xim)) {
                    pixSetPixel(pixmr, j, i, 0);
                    pixSetPixel(pixmg, j, i, 0);
                    pixSetPixel(pixmb, j, i, 0);
                }
            }
        }
    }

    /* ----------------- Now fill in the holes ----------------------- */
    if (pixFillMapHoles(pixmr, nx, ny, L_FILL_BLACK) ||
        pixFillMapHoles(pixmg, nx, ny, L_FILL_BLACK) ||
        pixFillMapHoles(pixmb, nx, ny, L_FILL_BLACK)) {
        pixDestroy(&pixmr);
        pixDestroy(&pixmg);
        pixDestroy(&pixmb);
        L_WARNING("can't make the maps\n", procName);
        return 1;
    }

        /* Finally, for each connected region corresponding to the
         * fg mask, reset all pixels to their average value. */
    if (pixim && fgpixels) {
        scalex = 1. / (l_float32)sx;
        scaley = 1. / (l_float32)sy;
        pixims = pixScaleBySampling(pixim, scalex, scaley);
        pixSmoothConnectedRegions(pixmr, pixims, 2);
        pixSmoothConnectedRegions(pixmg, pixims, 2);
        pixSmoothConnectedRegions(pixmb, pixims, 2);
        pixDestroy(&pixims);
    }

    *ppixmr = pixmr;
    *ppixmg = pixmg;
    *ppixmb = pixmb;
    return 0;
}


/*!
 *  pixGetBackgroundGrayMapMorph()
 *
 *      Input:  pixs (8 bpp grayscale; not cmapped)
 *              pixim (<optional> 1 bpp 'image' mask; can be null; it
 *                     should not have all foreground pixels)
 *              reduction (factor at which closing is performed)
 *              size (of square Sel for the closing; use an odd number)
 *              &pixm (<return> grayscale map)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixGetBackgroundGrayMapMorph(PIX     *pixs,
                             PIX     *pixim,
                             l_int32  reduction,
                             l_int32  size,
                             PIX    **ppixm)
{
l_int32    nx, ny, empty, fgpixels;
l_float32  scale;
PIX       *pixm, *pixt1, *pixt2, *pixt3, *pixims;

    PROCNAME("pixGetBackgroundGrayMapMorph");

    if (!ppixm)
        return ERROR_INT("&pixm not defined", procName, 1);
    *ppixm = NULL;
    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (pixGetColormap(pixs))
        return ERROR_INT("pixs is colormapped", procName, 1);
    if (pixim && pixGetDepth(pixim) != 1)
        return ERROR_INT("pixim not 1 bpp", procName, 1);

        /* Evaluate the mask pixim and make sure it is not all foreground. */
    fgpixels = 0;  /* boolean for existence of fg mask pixels */
    if (pixim) {
        pixInvert(pixim, pixim);  /* set background pixels to 1 */
        pixZero(pixim, &empty);
        if (empty)
            return ERROR_INT("pixim all fg; no background", procName, 1);
        pixInvert(pixim, pixim);  /* revert to original mask */
        pixZero(pixim, &empty);
        if (!empty)  /* there are fg pixels in pixim */
            fgpixels = 1;
    }

        /* Downscale as requested and do the closing to get the background. */
    scale = 1. / (l_float32)reduction;
    pixt1 = pixScaleBySampling(pixs, scale, scale);
    pixt2 = pixCloseGray(pixt1, size, size);
    pixt3 = pixExtendByReplication(pixt2, 1, 1);

        /* Downscale the image mask, if any, and remove it from the
         * background.  These pixels will be filled in (twice). */
    pixims = NULL;
    if (pixim) {
        pixims = pixScale(pixim, scale, scale);
        pixm = pixConvertTo8(pixims, FALSE);
        pixAnd(pixm, pixm, pixt3);
    }
    else
        pixm = pixClone(pixt3);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);

        /* Fill all the holes in the map. */
    nx = pixGetWidth(pixs) / reduction;
    ny = pixGetHeight(pixs) / reduction;
    if (pixFillMapHoles(pixm, nx, ny, L_FILL_BLACK)) {
        pixDestroy(&pixm);
        L_WARNING("can't make the map\n", procName);
        return 1;
    }

        /* Finally, for each connected region corresponding to the
         * fg mask, reset all pixels to their average value. */
    if (pixim && fgpixels) {
        pixSmoothConnectedRegions(pixm, pixims, 2);
        pixDestroy(&pixims);
    }

    *ppixm = pixm;
    pixCopyResolution(*ppixm, pixs);
    return 0;
}


/*!
 *  pixGetBackgroundRGBMapMorph()
 *
 *      Input:  pixs (32 bpp rgb)
 *              pixim (<optional> 1 bpp 'image' mask; can be null; it
 *                     should not have all foreground pixels)
 *              reduction (factor at which closing is performed)
 *              size (of square Sel for the closing; use an odd number)
 *              &pixmr (<return> red component map)
 *              &pixmg (<return> green component map)
 *              &pixmb (<return> blue component map)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixGetBackgroundRGBMapMorph(PIX     *pixs,
                            PIX     *pixim,
                            l_int32  reduction,
                            l_int32  size,
                            PIX    **ppixmr,
                            PIX    **ppixmg,
                            PIX    **ppixmb)
{
l_int32    nx, ny, empty, fgpixels;
l_float32  scale;
PIX       *pixm, *pixmr, *pixmg, *pixmb, *pixt1, *pixt2, *pixt3, *pixims;

    PROCNAME("pixGetBackgroundRGBMapMorph");

    if (!ppixmr || !ppixmg || !ppixmb)
        return ERROR_INT("&pixm* not all defined", procName, 1);
    *ppixmr = *ppixmg = *ppixmb = NULL;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetDepth(pixs) != 32)
        return ERROR_INT("pixs not 32 bpp", procName, 1);
    if (pixim && pixGetDepth(pixim) != 1)
        return ERROR_INT("pixim not 1 bpp", procName, 1);

        /* Generate an 8 bpp version of the image mask, if it exists */
    scale = 1. / (l_float32)reduction;
    pixm = NULL;
    if (pixim) {
        pixims = pixScale(pixim, scale, scale);
        pixm = pixConvertTo8(pixims, FALSE);
    }

        /* Evaluate the mask pixim and make sure it is not all foreground. */
    fgpixels = 0;  /* boolean for existence of fg mask pixels */
    if (pixim) {
        pixInvert(pixim, pixim);  /* set background pixels to 1 */
        pixZero(pixim, &empty);
        if (empty)
            return ERROR_INT("pixim all fg; no background", procName, 1);
        pixInvert(pixim, pixim);  /* revert to original mask */
        pixZero(pixim, &empty);
        if (!empty)  /* there are fg pixels in pixim */
            fgpixels = 1;
    }

        /* Downscale as requested and do the closing to get the background.
         * Then remove the image mask pixels from the background.  They
         * will be filled in (twice) later.  Do this for all 3 components. */
    pixt1 = pixScaleRGBToGrayFast(pixs, reduction, COLOR_RED);
    pixt2 = pixCloseGray(pixt1, size, size);
    pixt3 = pixExtendByReplication(pixt2, 1, 1);
    if (pixim)
        pixmr = pixAnd(NULL, pixm, pixt3);
    else
        pixmr = pixClone(pixt3);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);

    pixt1 = pixScaleRGBToGrayFast(pixs, reduction, COLOR_GREEN);
    pixt2 = pixCloseGray(pixt1, size, size);
    pixt3 = pixExtendByReplication(pixt2, 1, 1);
    if (pixim)
        pixmg = pixAnd(NULL, pixm, pixt3);
    else
        pixmg = pixClone(pixt3);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);

    pixt1 = pixScaleRGBToGrayFast(pixs, reduction, COLOR_BLUE);
    pixt2 = pixCloseGray(pixt1, size, size);
    pixt3 = pixExtendByReplication(pixt2, 1, 1);
    if (pixim)
        pixmb = pixAnd(NULL, pixm, pixt3);
    else
        pixmb = pixClone(pixt3);
    pixDestroy(&pixm);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);

        /* Fill all the holes in the three maps. */
    nx = pixGetWidth(pixs) / reduction;
    ny = pixGetHeight(pixs) / reduction;
    if (pixFillMapHoles(pixmr, nx, ny, L_FILL_BLACK) ||
        pixFillMapHoles(pixmg, nx, ny, L_FILL_BLACK) ||
        pixFillMapHoles(pixmb, nx, ny, L_FILL_BLACK)) {
        pixDestroy(&pixmr);
        pixDestroy(&pixmg);
        pixDestroy(&pixmb);
        L_WARNING("can't make the maps\n", procName);
        return 1;
    }

        /* Finally, for each connected region corresponding to the
         * fg mask in each component, reset all pixels to their
         * average value. */
    if (pixim && fgpixels) {
        pixSmoothConnectedRegions(pixmr, pixims, 2);
        pixSmoothConnectedRegions(pixmg, pixims, 2);
        pixSmoothConnectedRegions(pixmb, pixims, 2);
        pixDestroy(&pixims);
    }

    *ppixmr = pixmr;
    *ppixmg = pixmg;
    *ppixmb = pixmb;
    return 0;
}


/*!
 *  pixFillMapHoles()
 *
 *      Input:  pix (8 bpp; a map, with one pixel for each tile in
 *              a larger image)
 *              nx (number of horizontal pixel tiles that are entirely
 *                  covered with pixels in the original source image)
 *              ny (ditto for the number of vertical pixel tiles)
 *              filltype (L_FILL_WHITE or L_FILL_BLACK)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is an in-place operation on pix (the map).  pix is
 *          typically a low-resolution version of some other image
 *          from which it was derived, where each pixel in pix
 *          corresponds to a rectangular tile (say, m x n) of pixels
 *          in the larger image.  All we need to know about the larger
 *          image is whether or not the rightmost column and bottommost
 *          row of pixels in pix correspond to tiles that are
 *          only partially covered by pixels in the larger image.
 *      (2) Typically, some number of pixels in the input map are
 *          not known, and their values must be determined by near
 *          pixels that are known.  These unknown pixels are the 'holes'.
 *          They can take on only two values, 0 and 255, and the
 *          instruction about which to fill is given by the filltype flag.
 *      (3) The "holes" can come from two sources.  The first is when there
 *          are not enough foreground or background pixels in a tile;
 *          the second is when a tile is at least partially covered
 *          by an image mask.  If we're filling holes in a fg mask,
 *          the holes are initialized to black (0) and use L_FILL_BLACK.
 *          For filling holes in a bg mask, initialize the holes to
 *          white (255) and use L_FILL_WHITE.
 *      (4) If w is the map width, nx = w or nx = w - 1; ditto for h and ny.
 */
l_int32
pixFillMapHoles(PIX     *pix,
                l_int32  nx,
                l_int32  ny,
                l_int32  filltype)
{
l_int32   w, h, y, nmiss, goodcol, i, j, found, ival, valtest;
l_uint32  val, lastval;
NUMA     *na;  /* indicates if there is any data in the column */
PIX      *pixt;

    PROCNAME("pixFillMapHoles");

    if (!pix || pixGetDepth(pix) != 8)
        return ERROR_INT("pix not defined or not 8 bpp", procName, 1);
    if (pixGetColormap(pix))
        return ERROR_INT("pix is colormapped", procName, 1);

    /* ------------- Fill holes in the mapping image columns ----------- */
    pixGetDimensions(pix, &w, &h, NULL);
    na = numaCreate(0);  /* holds flag for which columns have data */
    nmiss = 0;
    valtest = (filltype == L_FILL_WHITE) ? 255 : 0;
    for (j = 0; j < nx; j++) {  /* do it by columns */
        found = FALSE;
        for (i = 0; i < ny; i++) {
            pixGetPixel(pix, j, i, &val);
            if (val != valtest) {
                y = i;
                found = TRUE;
                break;
            }
        }
        if (found == FALSE) {
            numaAddNumber(na, 0);  /* no data in the column */
            nmiss++;
        }
        else {
            numaAddNumber(na, 1);  /* data in the column */
            for (i = y - 1; i >= 0; i--)  /* replicate upwards to top */
                pixSetPixel(pix, j, i, val);
            pixGetPixel(pix, j, 0, &lastval);
            for (i = 1; i < h; i++) {  /* set going down to bottom */
                pixGetPixel(pix, j, i, &val);
                if (val == valtest)
                    pixSetPixel(pix, j, i, lastval);
                else
                    lastval = val;
            }
        }
    }
    numaAddNumber(na, 0);  /* last column */

    if (nmiss == nx) {  /* no data in any column! */
        numaDestroy(&na);
        L_WARNING("no bg found; no data in any column\n", procName);
        return 1;
    }

    /* ---------- Fill in missing columns by replication ----------- */
    if (nmiss > 0) {  /* replicate columns */
        pixt = pixCopy(NULL, pix);
            /* Find the first good column */
        goodcol = 0;
        for (j = 0; j < w; j++) {
            numaGetIValue(na, j, &ival);
            if (ival == 1) {
                goodcol = j;
                break;
            }
        }
        if (goodcol > 0) {  /* copy cols backward */
            for (j = goodcol - 1; j >= 0; j--) {
                pixRasterop(pix, j, 0, 1, h, PIX_SRC, pixt, j + 1, 0);
                pixRasterop(pixt, j, 0, 1, h, PIX_SRC, pix, j, 0);
            }
        }
        for (j = goodcol + 1; j < w; j++) {   /* copy cols forward */
            numaGetIValue(na, j, &ival);
            if (ival == 0) {
                    /* Copy the column to the left of j */
                pixRasterop(pix, j, 0, 1, h, PIX_SRC, pixt, j - 1, 0);
                pixRasterop(pixt, j, 0, 1, h, PIX_SRC, pix, j, 0);
            }
        }
        pixDestroy(&pixt);
    }
    if (w > nx) {  /* replicate the last column */
        for (i = 0; i < h; i++) {
            pixGetPixel(pix, w - 2, i, &val);
            pixSetPixel(pix, w - 1, i, val);
        }
    }

    numaDestroy(&na);
    return 0;
}


/*!
 *  pixExtendByReplication()
 *
 *      Input:  pixs (8 bpp)
 *              addw (number of extra pixels horizontally to add)
 *              addh (number of extra pixels vertically to add)
 *      Return: pixd (extended with replicated pixel values), or null on error
 *
 *  Notes:
 *      (1) The pixel values are extended to the left and down, as required.
 */
PIX *
pixExtendByReplication(PIX     *pixs,
                       l_int32  addw,
                       l_int32  addh)
{
l_int32   w, h, i, j;
l_uint32  val;
PIX      *pixd;

    PROCNAME("pixExtendByReplication");

    if (!pixs || pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);

    if (addw == 0 && addh == 0)
        return pixCopy(NULL, pixs);

    pixGetDimensions(pixs, &w, &h, NULL);
    if ((pixd = pixCreate(w + addw, h + addh, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixRasterop(pixd, 0, 0, w, h, PIX_SRC, pixs, 0, 0);

    if (addw > 0) {
        for (i = 0; i < h; i++) {
            pixGetPixel(pixd, w - 1, i, &val);
            for (j = 0; j < addw; j++)
                pixSetPixel(pixd, w + j, i, val);
        }
    }

    if (addh > 0) {
        for (j = 0; j < w + addw; j++) {
            pixGetPixel(pixd, j, h - 1, &val);
            for (i = 0; i < addh; i++)
                pixSetPixel(pixd, j, h + i, val);
        }
    }

    return pixd;
}


/*!
 *  pixSmoothConnectedRegions()
 *
 *      Input:  pixs (8 bpp grayscale; no colormap)
 *              pixm (<optional> 1 bpp; if null, this is a no-op)
 *              factor (subsampling factor for getting average; >= 1)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The pixels in pixs corresponding to those in each
 *          8-connected region in the mask are set to the average value.
 *      (2) This is required for adaptive mapping to avoid the
 *          generation of stripes in the background map, due to
 *          variations in the pixel values near the edges of mask regions.
 *      (3) This function is optimized for background smoothing, where
 *          there are a relatively small number of components.  It will
 *          be inefficient if used where there are many small components.
 */
l_int32
pixSmoothConnectedRegions(PIX     *pixs,
                          PIX     *pixm,
                          l_int32  factor)
{
l_int32    empty, i, n, x, y;
l_float32  aveval;
BOXA      *boxa;
PIX       *pixmc;
PIXA      *pixa;

    PROCNAME("pixSmoothConnectedRegions");

    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (pixGetColormap(pixs))
        return ERROR_INT("pixs has colormap", procName, 1);
    if (!pixm) {
        L_INFO("pixm not defined\n", procName);
        return 0;
    }
    if (pixGetDepth(pixm) != 1)
        return ERROR_INT("pixm not 1 bpp", procName, 1);
    pixZero(pixm, &empty);
    if (empty) {
        L_INFO("pixm has no fg pixels; nothing to do\n", procName);
        return 0;
    }

    boxa = pixConnComp(pixm, &pixa, 8);
    n = boxaGetCount(boxa);
    for (i = 0; i < n; i++) {
        if ((pixmc = pixaGetPix(pixa, i, L_CLONE)) == NULL) {
            L_WARNING("missing pixmc!\n", procName);
            continue;
        }
        boxaGetBoxGeometry(boxa, i, &x, &y, NULL, NULL);
        pixGetAverageMasked(pixs, pixmc, x, y, factor, L_MEAN_ABSVAL, &aveval);
        pixPaintThroughMask(pixs, pixmc, x, y, (l_int32)aveval);
        pixDestroy(&pixmc);
    }

    boxaDestroy(&boxa);
    pixaDestroy(&pixa);
    return 0;
}


/*------------------------------------------------------------------*
 *                 Measurement of local foreground                  *
 *------------------------------------------------------------------*/
#if 0    /* Not working properly: do not use */

/*!
 *  pixGetForegroundGrayMap()
 *
 *      Input:  pixs (8 bpp)
 *              pixim (<optional> 1 bpp 'image' mask; can be null)
 *              sx, sy (src tile size, in pixels)
 *              thresh (threshold for determining foreground)
 *              &pixd (<return> 8 bpp grayscale map)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Each (sx, sy) tile of pixs gets mapped to one pixel in pixd.
 *      (2) pixd is the estimate of the fg (darkest) value within each tile.
 *      (3) All pixels in pixd that are in 'image' regions, as specified
 *          by pixim, are given the background value 0.
 *      (4) For pixels in pixd that can't directly be given a fg value,
 *          the value is inferred by propagating from neighboring pixels.
 *      (5) In practice, pixd can be used to normalize the fg, and
 *          it can be done after background normalization.
 *      (6) The overall procedure is:
 *            - reduce 2x by sampling
 *            - paint all 'image' pixels white, so that they don't
 *              participate in the Min reduction
 *            - do a further (sx, sy) Min reduction -- think of
 *              it as a large opening followed by subsampling by the
 *              reduction factors
 *            - threshold the result to identify fg, and set the
 *              bg pixels to 255 (these are 'holes')
 *            - fill holes by propagation from fg values
 *            - replicatively expand by 2x, arriving at the final
 *              resolution of pixd
 *            - smooth with a 17x17 kernel
 *            - paint the 'image' regions black
 */
l_int32
pixGetForegroundGrayMap(PIX     *pixs,
                        PIX     *pixim,
                        l_int32  sx,
                        l_int32  sy,
                        l_int32  thresh,
                        PIX    **ppixd)
{
l_int32  w, h, d, wd, hd;
l_int32  empty, fgpixels;
PIX     *pixd, *piximi, *pixim2, *pixims, *pixs2, *pixb, *pixt1, *pixt2, *pixt3;

    PROCNAME("pixGetForegroundGrayMap");

    if (!ppixd)
        return ERROR_INT("&pixd not defined", procName, 1);
    *ppixd = NULL;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8)
        return ERROR_INT("pixs not 8 bpp", procName, 1);
    if (pixim && pixGetDepth(pixim) != 1)
        return ERROR_INT("pixim not 1 bpp", procName, 1);
    if (sx < 2 || sy < 2)
        return ERROR_INT("sx and sy must be >= 2", procName, 1);

        /* Generate pixd, which is reduced by the factors (sx, sy). */
    wd = (w + sx - 1) / sx;
    hd = (h + sy - 1) / sy;
    pixd = pixCreate(wd, hd, 8);
    *ppixd = pixd;

        /* Evaluate the 'image' mask, pixim.  If it is all fg,
         * the output pixd has all pixels with value 0. */
    fgpixels = 0;  /* boolean for existence of fg pixels in the image mask. */
    if (pixim) {
        piximi = pixInvert(NULL, pixim);  /* set non-image pixels to 1 */
        pixZero(piximi, &empty);
        pixDestroy(&piximi);
        if (empty)  /* all 'image'; return with all pixels set to 0 */
            return 0;
        pixZero(pixim, &empty);
        if (!empty)  /* there are fg pixels in pixim */
            fgpixels = 1;
    }

        /* 2x subsampling; paint white through 'image' mask. */
    pixs2 = pixScaleBySampling(pixs, 0.5, 0.5);
    if (pixim && fgpixels) {
        pixim2 = pixReduceBinary2(pixim, NULL);
        pixPaintThroughMask(pixs2, pixim2, 0, 0, 255);
        pixDestroy(&pixim2);
    }

        /* Min (erosion) downscaling; total reduction (4 sx, 4 sy). */
    pixt1 = pixScaleGrayMinMax(pixs2, sx, sy, L_CHOOSE_MIN);

/*    pixDisplay(pixt1, 300, 200); */

        /* Threshold to identify fg; paint bg pixels to white. */
    pixb = pixThresholdToBinary(pixt1, thresh);  /* fg pixels */
    pixInvert(pixb, pixb);
    pixPaintThroughMask(pixt1, pixb, 0, 0, 255);
    pixDestroy(&pixb);

        /* Replicative expansion by 2x to (sx, sy). */
    pixt2 = pixExpandReplicate(pixt1, 2);

/*    pixDisplay(pixt2, 500, 200); */

        /* Fill holes in the fg by propagation */
    pixFillMapHoles(pixt2, w / sx, h / sy, L_FILL_WHITE);

/*    pixDisplay(pixt2, 700, 200); */

        /* Smooth with 17x17 kernel. */
    pixt3 = pixBlockconv(pixt2, 8, 8);
    pixRasterop(pixd, 0, 0, wd, hd, PIX_SRC, pixt3, 0, 0);

        /* Paint the image parts black. */
    pixims = pixScaleBySampling(pixim, 1. / sx, 1. / sy);
    pixPaintThroughMask(pixd, pixims, 0, 0, 0);

    pixDestroy(&pixs2);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    return 0;
}
#endif   /* Not working properly: do not use */


/*------------------------------------------------------------------*
 *                  Generate inverted background map                *
 *------------------------------------------------------------------*/
/*!
 *  pixGetInvBackgroundMap()
 *
 *      Input:  pixs (8 bpp grayscale; no colormap)
 *              bgval (target bg val; typ. > 128)
 *              smoothx (half-width of block convolution kernel width)
 *              smoothy (half-width of block convolution kernel height)
 *      Return: pixd (16 bpp), or null on error
 *
 *  Note:
 *     - bgval should typically be > 120 and < 240
 *     - pixd is a normalization image; the original image is
 *       multiplied by pixd and the result is divided by 256.
 */
PIX *
pixGetInvBackgroundMap(PIX     *pixs,
                       l_int32  bgval,
                       l_int32  smoothx,
                       l_int32  smoothy)
{
l_int32    w, h, wplsm, wpld, i, j;
l_int32    val, val16;
l_uint32  *datasm, *datad, *linesm, *lined;
PIX       *pixsm, *pixd;

    PROCNAME("pixGetInvBackgroundMap");

    if (!pixs || pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs has colormap", procName, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);
    if (w < 5 || h < 5)
        return (PIX *)ERROR_PTR("w and h must be >= 5", procName, NULL);

        /* smooth the map image */
    pixsm = pixBlockconv(pixs, smoothx, smoothy);
    datasm = pixGetData(pixsm);
    wplsm = pixGetWpl(pixsm);

        /* invert the map image, scaling up to preserve dynamic range */
    pixd = pixCreate(w, h, 16);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        linesm = datasm + i * wplsm;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            val = GET_DATA_BYTE(linesm, j);
            if (val > 0)
                val16 = (256 * bgval) / val;
            else {  /* shouldn't happen */
                L_WARNING("smoothed bg has 0 pixel!\n", procName);
                val16 = bgval / 2;
            }
            SET_DATA_TWO_BYTES(lined, j, val16);
        }
    }

    pixDestroy(&pixsm);
    return pixd;
}


/*------------------------------------------------------------------*
 *                    Apply background map to image                 *
 *------------------------------------------------------------------*/
/*!
 *  pixApplyInvBackgroundGrayMap()
 *
 *      Input:  pixs (8 bpp grayscale; no colormap)
 *              pixm (16 bpp, inverse background map)
 *              sx (tile width in pixels)
 *              sy (tile height in pixels)
 *      Return: pixd (8 bpp), or null on error
 */
PIX *
pixApplyInvBackgroundGrayMap(PIX     *pixs,
                             PIX     *pixm,
                             l_int32  sx,
                             l_int32  sy)
{
l_int32    w, h, wm, hm, wpls, wpld, i, j, k, m, xoff, yoff;
l_int32    vals, vald;
l_uint32   val16;
l_uint32  *datas, *datad, *lines, *lined, *flines, *flined;
PIX       *pixd;

    PROCNAME("pixApplyInvBackgroundGrayMap");

    if (!pixs || pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs has colormap", procName, NULL);
    if (!pixm || pixGetDepth(pixm) != 16)
        return (PIX *)ERROR_PTR("pixm undefined or not 16 bpp", procName, NULL);
    if (sx == 0 || sy == 0)
        return (PIX *)ERROR_PTR("invalid sx and/or sy", procName, NULL);

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    pixGetDimensions(pixs, &w, &h, NULL);
    pixGetDimensions(pixm, &wm, &hm, NULL);
    pixd = pixCreateTemplate(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < hm; i++) {
        lines = datas + sy * i * wpls;
        lined = datad + sy * i * wpld;
        yoff = sy * i;
        for (j = 0; j < wm; j++) {
            pixGetPixel(pixm, j, i, &val16);
            xoff = sx * j;
            for (k = 0; k < sy && yoff + k < h; k++) {
                flines = lines + k * wpls;
                flined = lined + k * wpld;
                for (m = 0; m < sx && xoff + m < w; m++) {
                    vals = GET_DATA_BYTE(flines, xoff + m);
                    vald = (vals * val16) / 256;
                    vald = L_MIN(vald, 255);
                    SET_DATA_BYTE(flined, xoff + m, vald);
                }
            }
        }
    }

    return pixd;
}


/*!
 *  pixApplyInvBackgroundRGBMap()
 *
 *      Input:  pixs (32 bpp rbg)
 *              pixmr (16 bpp, red inverse background map)
 *              pixmg (16 bpp, green inverse background map)
 *              pixmb (16 bpp, blue inverse background map)
 *              sx (tile width in pixels)
 *              sy (tile height in pixels)
 *      Return: pixd (32 bpp rbg), or null on error
 */
PIX *
pixApplyInvBackgroundRGBMap(PIX     *pixs,
                            PIX     *pixmr,
                            PIX     *pixmg,
                            PIX     *pixmb,
                            l_int32  sx,
                            l_int32  sy)
{
l_int32    w, h, wm, hm, wpls, wpld, i, j, k, m, xoff, yoff;
l_int32    rvald, gvald, bvald;
l_uint32   vals;
l_uint32   rval16, gval16, bval16;
l_uint32  *datas, *datad, *lines, *lined, *flines, *flined;
PIX       *pixd;

    PROCNAME("pixApplyInvBackgroundRGBMap");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs not 32 bpp", procName, NULL);
    if (!pixmr || !pixmg || !pixmb)
        return (PIX *)ERROR_PTR("pix maps not all defined", procName, NULL);
    if (pixGetDepth(pixmr) != 16 || pixGetDepth(pixmg) != 16 ||
        pixGetDepth(pixmb) != 16)
        return (PIX *)ERROR_PTR("pix maps not all 16 bpp", procName, NULL);
    if (sx == 0 || sy == 0)
        return (PIX *)ERROR_PTR("invalid sx and/or sy", procName, NULL);

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);
    wm = pixGetWidth(pixmr);
    hm = pixGetHeight(pixmr);
    pixd = pixCreateTemplate(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < hm; i++) {
        lines = datas + sy * i * wpls;
        lined = datad + sy * i * wpld;
        yoff = sy * i;
        for (j = 0; j < wm; j++) {
            pixGetPixel(pixmr, j, i, &rval16);
            pixGetPixel(pixmg, j, i, &gval16);
            pixGetPixel(pixmb, j, i, &bval16);
            xoff = sx * j;
            for (k = 0; k < sy && yoff + k < h; k++) {
                flines = lines + k * wpls;
                flined = lined + k * wpld;
                for (m = 0; m < sx && xoff + m < w; m++) {
                    vals = *(flines + xoff + m);
                    rvald = ((vals >> 24) * rval16) / 256;
                    rvald = L_MIN(rvald, 255);
                    gvald = (((vals >> 16) & 0xff) * gval16) / 256;
                    gvald = L_MIN(gvald, 255);
                    bvald = (((vals >> 8) & 0xff) * bval16) / 256;
                    bvald = L_MIN(bvald, 255);
                    composeRGBPixel(rvald, gvald, bvald, flined + xoff + m);
                }
            }
        }
    }

    return pixd;
}


/*------------------------------------------------------------------*
 *                         Apply variable map                       *
 *------------------------------------------------------------------*/
/*!
 *  pixApplyVariableGrayMap()
 *
 *      Input:  pixs (8 bpp)
 *              pixg (8 bpp, variable map)
 *              target (typ. 128 for threshold)
 *      Return: pixd (8 bpp), or null on error
 *
 *  Notes:
 *      (1) Suppose you have an image that you want to transform based
 *          on some photometric measurement at each point, such as the
 *          threshold value for binarization.  Representing the photometric
 *          measurement as an image pixg, you can threshold in input image
 *          using pixVarThresholdToBinary().  Alternatively, you can map
 *          the input image pointwise so that the threshold over the
 *          entire image becomes a constant, such as 128.  For example,
 *          if a pixel in pixg is 150 and the target is 128, the
 *          corresponding pixel in pixs is mapped linearly to a value
 *          (128/150) of the input value.  If the resulting mapped image
 *          pixd were then thresholded at 128, you would obtain the
 *          same result as a direct binarization using pixg with
 *          pixVarThresholdToBinary().
 *      (2) The sizes of pixs and pixg must be equal.
 */
PIX *
pixApplyVariableGrayMap(PIX     *pixs,
                        PIX     *pixg,
                        l_int32  target)
{
l_int32    i, j, w, h, d, wpls, wplg, wpld, vals, valg, vald;
l_uint8   *lut;
l_uint32  *datas, *datag, *datad, *lines, *lineg, *lined;
l_float32  fval;
PIX       *pixd;

    PROCNAME("pixApplyVariableGrayMap");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!pixg)
        return (PIX *)ERROR_PTR("pixg not defined", procName, NULL);
    if (!pixSizesEqual(pixs, pixg))
        return (PIX *)ERROR_PTR("pix sizes not equal", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8)
        return (PIX *)ERROR_PTR("depth not 8 bpp", procName, NULL);

        /* Generate a LUT for the mapping if the image is large enough
         * to warrant the overhead.  The LUT is of size 2^16.  For the
         * index to the table, get the MSB from pixs and the LSB from pixg.
         * Note: this LUT is bigger than the typical 32K L1 cache, so
         * we expect cache misses.  L2 latencies are about 5ns.  But
         * division is slooooow.  For large images, this function is about
         * 4x faster when using the LUT.  C'est la vie.  */
    lut = NULL;
    if (w * h > 100000) {  /* more pixels than 2^16 */
        if ((lut = (l_uint8 *)CALLOC(0x10000, sizeof(l_uint8))) == NULL)
            return (PIX *)ERROR_PTR("lut not made", procName, NULL);
        for (i = 0; i < 256; i++) {
            for (j = 0; j < 256; j++) {
                fval = (l_float32)(i * target) / (j + 0.5);
                lut[(i << 8) + j] = L_MIN(255, (l_int32)(fval + 0.5));
            }
        }
    }

    pixd = pixCreateNoInit(w, h, 8);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datag = pixGetData(pixg);
    wplg = pixGetWpl(pixg);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lineg = datag + i * wplg;
        lined = datad + i * wpld;
        if (lut) {
            for (j = 0; j < w; j++) {
                vals = GET_DATA_BYTE(lines, j);
                valg = GET_DATA_BYTE(lineg, j);
                vald = lut[(vals << 8) + valg];
                SET_DATA_BYTE(lined, j, vald);
            }
        }
        else {
            for (j = 0; j < w; j++) {
                vals = GET_DATA_BYTE(lines, j);
                valg = GET_DATA_BYTE(lineg, j);
                fval = (l_float32)(vals * target) / (valg + 0.5);
                vald = L_MIN(255, (l_int32)(fval + 0.5));
                SET_DATA_BYTE(lined, j, vald);
            }
        }
    }

    if (lut) FREE(lut);
    return pixd;
}


/*------------------------------------------------------------------*
 *                  Non-adaptive (global) mapping                   *
 *------------------------------------------------------------------*/
/*!
 *  pixGlobalNormRGB()
 *
 *      Input:  pixd (<optional> null, existing or equal to pixs)
 *              pixs (32 bpp rgb, or colormapped)
 *              rval, gval, bval (pixel values in pixs that are
 *                                linearly mapped to mapval)
 *              mapval (use 255 for mapping to white)
 *      Return: pixd (32 bpp rgb or colormapped), or null on error
 *
 *  Notes:
 *    (1) The value of pixd determines if the results are written to a
 *        new pix (use NULL), in-place to pixs (use pixs), or to some
 *        other existing pix.
 *    (2) This does a global normalization of an image where the
 *        r,g,b color components are not balanced.  Thus, white in pixs is
 *        represented by a set of r,g,b values that are not all 255.
 *    (3) The input values (rval, gval, bval) should be chosen to
 *        represent the gray color (mapval, mapval, mapval) in src.
 *        Thus, this function will map (rval, gval, bval) to that gray color.
 *    (4) Typically, mapval = 255, so that (rval, gval, bval)
 *        corresponds to the white point of src.  In that case, these
 *        parameters should be chosen so that few pixels have higher values.
 *    (5) In all cases, we do a linear TRC separately on each of the
 *        components, saturating at 255.
 *    (6) If the input pix is 8 bpp without a colormap, you can get
 *        this functionality with mapval = 255 by calling:
 *            pixGammaTRC(pixd, pixs, 1.0, 0, bgval);
 *        where bgval is the value you want to be mapped to 255.
 *        Or more generally, if you want bgval to be mapped to mapval:
 *            pixGammaTRC(pixd, pixs, 1.0, 0, 255 * bgval / mapval);
 */
PIX *
pixGlobalNormRGB(PIX     *pixd,
                 PIX     *pixs,
                 l_int32  rval,
                 l_int32  gval,
                 l_int32  bval,
                 l_int32  mapval)
{
l_int32    w, h, d, i, j, ncolors, rv, gv, bv, wpl;
l_int32   *rarray, *garray, *barray;
l_uint32  *data, *line;
NUMA      *nar, *nag, *nab;
PIXCMAP   *cmap;

    PROCNAME("pixGlobalNormRGB");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    cmap = pixGetColormap(pixs);
    pixGetDimensions(pixs, &w, &h, &d);
    if (!cmap && d != 32)
        return (PIX *)ERROR_PTR("pixs not cmapped or 32 bpp", procName, NULL);
    if (mapval <= 0) {
        L_WARNING("mapval must be > 0; setting to 255\n", procName);
        mapval = 255;
    }

        /* Prepare pixd to be a copy of pixs */
    if ((pixd = pixCopy(pixd, pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

        /* Generate the TRC maps for each component.  Make sure the
         * upper range for each color is greater than zero. */
    nar = numaGammaTRC(1.0, 0, L_MAX(1, 255 * rval / mapval));
    nag = numaGammaTRC(1.0, 0, L_MAX(1, 255 * gval / mapval));
    nab = numaGammaTRC(1.0, 0, L_MAX(1, 255 * bval / mapval));
    if (!nar || !nag || !nab)
        return (PIX *)ERROR_PTR("trc maps not all made", procName, pixd);

        /* Extract copies of the internal arrays */
    rarray = numaGetIArray(nar);
    garray = numaGetIArray(nag);
    barray = numaGetIArray(nab);
    if (!rarray || !garray || !barray)
        return (PIX *)ERROR_PTR("*arrays not all made", procName, pixd);

    if (cmap) {
        ncolors = pixcmapGetCount(cmap);
        for (i = 0; i < ncolors; i++) {
            pixcmapGetColor(cmap, i, &rv, &gv, &bv);
            pixcmapResetColor(cmap, i, rarray[rv], garray[gv], barray[bv]);
        }
    }
    else {
        data = pixGetData(pixd);
        wpl = pixGetWpl(pixd);
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            for (j = 0; j < w; j++) {
                extractRGBValues(line[j], &rv, &gv, &bv);
                composeRGBPixel(rarray[rv], garray[gv], barray[bv], line + j);
            }
        }
    }

    numaDestroy(&nar);
    numaDestroy(&nag);
    numaDestroy(&nab);
    FREE(rarray);
    FREE(garray);
    FREE(barray);
    return pixd;
}


/*!
 *  pixGlobalNormNoSatRGB()
 *
 *      Input:  pixd (<optional> null, existing or equal to pixs)
 *              pixs (32 bpp rgb)
 *              rval, gval, bval (pixel values in pixs that are
 *                                linearly mapped to mapval; but see below)
 *              factor (subsampling factor; integer >= 1)
 *              rank (between 0.0 and 1.0; typ. use a value near 1.0)
 *      Return: pixd (32 bpp rgb), or null on error
 *
 *  Notes:
 *    (1) This is a version of pixGlobalNormRGB(), where the output
 *        intensity is scaled back so that a controlled fraction of
 *        pixel components is allowed to saturate.  See comments in
 *        pixGlobalNormRGB().
 *    (2) The value of pixd determines if the results are written to a
 *        new pix (use NULL), in-place to pixs (use pixs), or to some
 *        other existing pix.
 *    (3) This does a global normalization of an image where the
 *        r,g,b color components are not balanced.  Thus, white in pixs is
 *        represented by a set of r,g,b values that are not all 255.
 *    (4) The input values (rval, gval, bval) can be chosen to be the
 *        color that, after normalization, becomes white background.
 *        For images that are mostly background, the closer these values
 *        are to the median component values, the closer the resulting
 *        background will be to gray, becoming white at the brightest places.
 *    (5) The mapval used in pixGlobalNormRGB() is computed here to
 *        avoid saturation of any component in the image (save for a
 *        fraction of the pixels given by the input rank value).
 */
PIX *
pixGlobalNormNoSatRGB(PIX       *pixd,
                      PIX       *pixs,
                      l_int32    rval,
                      l_int32    gval,
                      l_int32    bval,
                      l_int32    factor,
                      l_float32  rank)
{
l_int32    mapval;
l_float32  rankrval, rankgval, rankbval;
l_float32  rfract, gfract, bfract, maxfract;

    PROCNAME("pixGlobalNormNoSatRGB");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs not 32 bpp", procName, NULL);
    if (factor < 1)
        return (PIX *)ERROR_PTR("sampling factor < 1", procName, NULL);
    if (rank < 0.0 || rank > 1.0)
        return (PIX *)ERROR_PTR("rank not in [0.0 ... 1.0]", procName, NULL);
    if (rval <= 0 || gval <= 0 || bval <= 0)
        return (PIX *)ERROR_PTR("invalid estim. color values", procName, NULL);

        /* The max value for each component may be larger than the
         * input estimated background value.  In that case, mapping
         * for those pixels would saturate.  To prevent saturation,
         * we compute the fraction for each component by which we
         * would oversaturate.  Then take the max of these, and
         * reduce, uniformly over all components, the output intensity
         * by this value.  Then no component will saturate.
         * In practice, if rank < 1.0, a fraction of pixels
         * may have a component saturate.  By keeping rank close to 1.0,
         * that fraction can be made arbitrarily small. */
    pixGetRankValueMaskedRGB(pixs, NULL, 0, 0, factor, rank, &rankrval,
                             &rankgval, &rankbval);
    rfract = rankrval / (l_float32)rval;
    gfract = rankgval / (l_float32)gval;
    bfract = rankbval / (l_float32)bval;
    maxfract = L_MAX(rfract, gfract);
    maxfract = L_MAX(maxfract, bfract);
#if  DEBUG_GLOBAL
    fprintf(stderr, "rankrval = %7.2f, rankgval = %7.2f, rankbval = %7.2f\n",
            rankrval, rankgval, rankbval);
    fprintf(stderr, "rfract = %7.4f, gfract = %7.4f, bfract = %7.4f\n",
            rfract, gfract, bfract);
#endif  /* DEBUG_GLOBAL */

    mapval = (l_int32)(255. / maxfract);
    pixd = pixGlobalNormRGB(pixd, pixs, rval, gval, bval, mapval);
    return pixd;
}


/*------------------------------------------------------------------*
 *              Adaptive threshold spread normalization             *
 *------------------------------------------------------------------*/
/*!
 *  pixThresholdSpreadNorm()
 *
 *      Input:  pixs (8 bpp grayscale; not colormapped)
 *              filtertype (L_SOBEL_EDGE or L_TWO_SIDED_EDGE);
 *              edgethresh (threshold on magnitude of edge filter; typ 10-20)
 *              smoothx, smoothy (half-width of convolution kernel applied to
 *                                spread threshold: use 0 for no smoothing)
 *              gamma (gamma correction; typ. about 0.7)
 *              minval  (input value that gives 0 for output; typ. -25)
 *              maxval  (input value that gives 255 for output; typ. 255)
 *              targetthresh (target threshold for normalization)
 *              &pixth (<optional return> computed local threshold value)
 *              &pixb (<optional return> thresholded normalized image)
 *              &pixd (<optional return> normalized image)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The basis of this approach is the use of seed spreading
 *          on a (possibly) sparse set of estimates for the local threshold.
 *          The resulting dense estimates are smoothed by convolution
 *          and used to either threshold the input image or normalize it
 *          with a local transformation that linearly maps the pixels so
 *          that the local threshold estimate becomes constant over the
 *          resulting image.  This approach is one of several that
 *          have been suggested (and implemented) by Ray Smith.
 *      (2) You can use either the Sobel or TwoSided edge filters.
 *          The results appear to be similar, using typical values
 *          of edgethresh in the rang 10-20.
 *      (3) To skip the trc enhancement, use gamma = 1.0, minval = 0
 *          and maxval = 255.
 *      (4) For the normalized image pixd, each pixel is linearly mapped
 *          in such a way that the local threshold is equal to targetthresh.
 *      (5) The full width and height of the convolution kernel
 *          are (2 * smoothx + 1) and (2 * smoothy + 1).
 *      (6) This function can be used with the pixtiling utility if the
 *          images are too large.  See pixOtsuAdaptiveThreshold() for
 *          an example of this.
 */
l_int32
pixThresholdSpreadNorm(PIX       *pixs,
                       l_int32    filtertype,
                       l_int32    edgethresh,
                       l_int32    smoothx,
                       l_int32    smoothy,
                       l_float32  gamma,
                       l_int32    minval,
                       l_int32    maxval,
                       l_int32    targetthresh,
                       PIX      **ppixth,
                       PIX      **ppixb,
                       PIX      **ppixd)
{
PIX     *pixe, *pixet, *pixsd, *pixg1, *pixg2, *pixth;

    PROCNAME("pixThresholdSpreadNorm");

    if (ppixth) *ppixth = NULL;
    if (ppixb) *ppixb = NULL;
    if (ppixd) *ppixd = NULL;
    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (pixGetColormap(pixs))
        return ERROR_INT("pixs is colormapped", procName, 1);
    if (!ppixth && !ppixb && !ppixd)
        return ERROR_INT("no output requested", procName, 1);
    if (filtertype != L_SOBEL_EDGE && filtertype != L_TWO_SIDED_EDGE)
        return ERROR_INT("invalid filter type", procName, 1);

        /* Get the thresholded edge pixels.  These are the ones
         * that have values in pixs near the local optimal fg/bg threshold. */
    if (filtertype == L_SOBEL_EDGE)
        pixe = pixSobelEdgeFilter(pixs, L_VERTICAL_EDGES);
    else  /* L_TWO_SIDED_EDGE */
        pixe = pixTwoSidedEdgeFilter(pixs, L_VERTICAL_EDGES);
    pixet = pixThresholdToBinary(pixe, edgethresh);
    pixInvert(pixet, pixet);

        /* Build a seed image whose only nonzero values are those
         * values of pixs corresponding to pixels in the fg of pixet. */
    pixsd = pixCreateTemplate(pixs);
    pixCombineMasked(pixsd, pixs, pixet);

        /* Spread the seed and optionally smooth to reduce noise */
    pixg1 = pixSeedspread(pixsd, 4);
    pixg2 = pixBlockconv(pixg1, smoothx, smoothy);

        /* Optionally do a gamma enhancement */
    pixth = pixGammaTRC(NULL, pixg2, gamma, minval, maxval);

        /* Do the mapping and thresholding */
    if (ppixd) {
        *ppixd = pixApplyVariableGrayMap(pixs, pixth, targetthresh);
        if (ppixb)
            *ppixb = pixThresholdToBinary(*ppixd, targetthresh);
    }
    else if (ppixb)
        *ppixb = pixVarThresholdToBinary(pixs, pixth);

    if (ppixth)
        *ppixth = pixth;
    else
        pixDestroy(&pixth);

    pixDestroy(&pixe);
    pixDestroy(&pixet);
    pixDestroy(&pixsd);
    pixDestroy(&pixg1);
    pixDestroy(&pixg2);
    return 0;
}


/*------------------------------------------------------------------*
 *      Adaptive background normalization (flexible adaptaption)    *
 *------------------------------------------------------------------*/
/*!
 *  pixBackgroundNormFlex()
 *
 *      Input:  pixs (8 bpp grayscale; not colormapped)
 *              sx, sy (desired tile dimensions; actual size may vary; use
 *                      values between 3 and 10)
 *              smoothx, smoothy (half-width of convolution kernel applied to
 *                                threshold array: use values between 1 and 3)
 *              delta (difference parameter in basin filling; use 0
 *                     to skip)
 *      Return: pixd (8 bpp, background-normalized), or null on error)
 *
 *  Notes:
 *      (1) This does adaptation flexibly to a quickly varying background.
 *          For that reason, all input parameters should be small.
 *      (2) sx and sy give the tile size; they should be in [5 - 7].
 *      (3) The full width and height of the convolution kernel
 *          are (2 * smoothx + 1) and (2 * smoothy + 1).  They
 *          should be in [1 - 2].
 *      (4) Basin filling is used to fill the large fg regions.  The
 *          parameter @delta measures the height that the black
 *          background is raised from the local minima.  By raising
 *          the background, it is possible to threshold the large
 *          fg regions to foreground.  If @delta is too large,
 *          bg regions will be lifted, causing thickening of
 *          the fg regions.  Use 0 to skip.
 */
PIX *
pixBackgroundNormFlex(PIX     *pixs,
                      l_int32  sx,
                      l_int32  sy,
                      l_int32  smoothx,
                      l_int32  smoothy,
                      l_int32  delta)
{
l_float32  scalex, scaley;
PIX       *pixt, *pixsd, *pixmin, *pixbg, *pixbgi, *pixd;

    PROCNAME("pixBackgroundNormFlex");

    if (!pixs || pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs is colormapped", procName, NULL);
    if (sx < 3 || sy < 3)
        return (PIX *)ERROR_PTR("sx and/or sy less than 3", procName, NULL);
    if (sx > 10 || sy > 10)
        return (PIX *)ERROR_PTR("sx and/or sy exceed 10", procName, NULL);
    if (smoothx < 1 || smoothy < 1)
        return (PIX *)ERROR_PTR("smooth params less than 1", procName, NULL);
    if (smoothx > 3 || smoothy > 3)
        return (PIX *)ERROR_PTR("smooth params exceed 3", procName, NULL);

        /* Generate the bg estimate using smoothed average with subsampling */
    scalex = 1. / (l_float32)sx;
    scaley = 1. / (l_float32)sy;
    pixt = pixScaleSmooth(pixs, scalex, scaley);

        /* Do basin filling on the bg estimate if requested */
    if (delta <= 0)
        pixsd = pixClone(pixt);
    else {
        pixLocalExtrema(pixt, 0, 0, &pixmin, NULL);
        pixsd = pixSeedfillGrayBasin(pixmin, pixt, delta, 4);
        pixDestroy(&pixmin);
    }
    pixbg = pixExtendByReplication(pixsd, 1, 1);

        /* Map the bg to 200 */
    pixbgi = pixGetInvBackgroundMap(pixbg, 200, smoothx, smoothy);
    pixd = pixApplyInvBackgroundGrayMap(pixs, pixbgi, sx, sy);

    pixDestroy(&pixt);
    pixDestroy(&pixsd);
    pixDestroy(&pixbg);
    pixDestroy(&pixbgi);
    return pixd;
}


/*------------------------------------------------------------------*
 *                    Adaptive contrast normalization               *
 *------------------------------------------------------------------*/
/*!
 *  pixContrastNorm()
 *
 *      Input:  pixd (<optional> 8 bpp; null or equal to pixs)
 *              pixs (8 bpp grayscale; not colormapped)
 *              sx, sy (tile dimensions)
 *              mindiff (minimum difference to accept as valid)
 *              smoothx, smoothy (half-width of convolution kernel applied to
 *                                min and max arrays: use 0 for no smoothing)
 *      Return: pixd always
 *
 *  Notes:
 *      (1) This function adaptively attempts to expand the contrast
 *          to the full dynamic range in each tile.  If the contrast in
 *          a tile is smaller than @mindiff, it uses the min and max
 *          pixel values from neighboring tiles.  It also can use
 *          convolution to smooth the min and max values from
 *          neighboring tiles.  After all that processing, it is
 *          possible that the actual pixel values in the tile are outside
 *          the computed [min ... max] range for local contrast
 *          normalization.  Such pixels are taken to be at either 0
 *          (if below the min) or 255 (if above the max).
 *      (2) pixd can be equal to pixs (in-place operation) or
 *          null (makes a new pixd).
 *      (3) sx and sy give the tile size; they are typically at least 20.
 *      (4) mindiff is used to eliminate results for tiles where it is
 *          likely that either fg or bg is missing.  A value around 50
 *          or more is reasonable.
 *      (5) The full width and height of the convolution kernel
 *          are (2 * smoothx + 1) and (2 * smoothy + 1).  Some smoothing
 *          is typically useful, and we limit the smoothing half-widths
 *          to the range from 0 to 8.
 *      (6) A linear TRC (gamma = 1.0) is applied to increase the contrast
 *          in each tile.  The result can subsequently be globally corrected,
 *          by applying pixGammaTRC() with arbitrary values of gamma
 *          and the 0 and 255 points of the mapping.
 */
PIX *
pixContrastNorm(PIX       *pixd,
                PIX       *pixs,
                l_int32    sx,
                l_int32    sy,
                l_int32    mindiff,
                l_int32    smoothx,
                l_int32    smoothy)
{
PIX  *pixmin, *pixmax;

    PROCNAME("pixContrastNorm");

    if (!pixs || pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs undefined or not 8 bpp", procName, pixd);
    if (pixd && pixd != pixs)
        return (PIX *)ERROR_PTR("pixd not null or == pixs", procName, pixd);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs is colormapped", procName, pixd);
    if (sx < 5 || sy < 5)
        return (PIX *)ERROR_PTR("sx and/or sy less than 5", procName, pixd);
    if (smoothx < 0 || smoothy < 0)
        return (PIX *)ERROR_PTR("smooth params less than 0", procName, pixd);
    if (smoothx > 8 || smoothy > 8)
        return (PIX *)ERROR_PTR("smooth params exceed 8", procName, pixd);

        /* Get the min and max pixel values in each tile, and represent
         * each value as a pixel in pixmin and pixmax, respectively. */
    pixMinMaxTiles(pixs, sx, sy, mindiff, smoothx, smoothy, &pixmin, &pixmax);

        /* For each tile, do a linear expansion of the dynamic range
         * of pixels so that the min value is mapped to 0 and the
         * max value is mapped to 255.  */
    pixd = pixLinearTRCTiled(pixd, pixs, sx, sy, pixmin, pixmax);

    pixDestroy(&pixmin);
    pixDestroy(&pixmax);
    return pixd;
}


/*!
 *  pixMinMaxTiles()
 *
 *      Input:  pixs (8 bpp grayscale; not colormapped)
 *              sx, sy (tile dimensions)
 *              mindiff (minimum difference to accept as valid)
 *              smoothx, smoothy (half-width of convolution kernel applied to
 *                                min and max arrays: use 0 for no smoothing)
 *              &pixmin (<return> tiled minima)
 *              &pixmax (<return> tiled maxima)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This computes filtered and smoothed values for the min and
 *          max pixel values in each tile of the image.
 *      (2) See pixContrastNorm() for usage.
 */
l_int32
pixMinMaxTiles(PIX     *pixs,
               l_int32  sx,
               l_int32  sy,
               l_int32  mindiff,
               l_int32  smoothx,
               l_int32  smoothy,
               PIX    **ppixmin,
               PIX    **ppixmax)
{
l_int32  w, h;
PIX     *pixmin1, *pixmax1, *pixmin2, *pixmax2;

    PROCNAME("pixMinMaxTiles");

    if (!ppixmin || !ppixmax)
        return ERROR_INT("&pixmin or &pixmax undefined", procName, 1);
    *ppixmin = *ppixmax = NULL;
    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs undefined or not 8 bpp", procName, 1);
    if (pixGetColormap(pixs))
        return ERROR_INT("pixs is colormapped", procName, 1);
    if (sx < 5 || sy < 5)
        return ERROR_INT("sx and/or sy less than 3", procName, 1);
    if (smoothx < 0 || smoothy < 0)
        return ERROR_INT("smooth params less than 0", procName, 1);
    if (smoothx > 5 || smoothy > 5)
        return ERROR_INT("smooth params exceed 5", procName, 1);

        /* Get the min and max values in each tile */
    pixmin1 = pixScaleGrayMinMax(pixs, sx, sy, L_CHOOSE_MIN);
    pixmax1 = pixScaleGrayMinMax(pixs, sx, sy, L_CHOOSE_MAX);

    pixmin2 = pixExtendByReplication(pixmin1, 1, 1);
    pixmax2 = pixExtendByReplication(pixmax1, 1, 1);
    pixDestroy(&pixmin1);
    pixDestroy(&pixmax1);

        /* Make sure no value is 0 */
    pixAddConstantGray(pixmin2, 1);
    pixAddConstantGray(pixmax2, 1);

        /* Generate holes where the contrast is too small */
    pixSetLowContrast(pixmin2, pixmax2, mindiff);

        /* Fill the holes (0 values) */
    pixGetDimensions(pixmin2, &w, &h, NULL);
    pixFillMapHoles(pixmin2, w, h, L_FILL_BLACK);
    pixFillMapHoles(pixmax2, w, h, L_FILL_BLACK);

        /* Smooth if requested */
    if (smoothx > 0 || smoothy > 0) {
        smoothx = L_MIN(smoothx, (w - 1) / 2);
        smoothy = L_MIN(smoothy, (h - 1) / 2);
        *ppixmin = pixBlockconv(pixmin2, smoothx, smoothy);
        *ppixmax = pixBlockconv(pixmax2, smoothx, smoothy);
    }
    else {
        *ppixmin = pixClone(pixmin2);
        *ppixmax = pixClone(pixmax2);
    }
    pixDestroy(&pixmin2);
    pixDestroy(&pixmax2);

    return 0;
}


/*!
 *  pixSetLowContrast()
 *
 *      Input:  pixs1 (8 bpp)
 *              pixs2 (8 bpp)
 *              mindiff (minimum difference to accept as valid)
 *      Return: 0 if OK; 1 if no pixel diffs are large enough, or on error
 *
 *  Notes:
 *      (1) This compares corresponding pixels in pixs1 and pixs2.
 *          When they differ by less than @mindiff, set the pixel
 *          values to 0 in each.  Each pixel typically represents a tile
 *          in a larger image, and a very small difference between
 *          the min and max in the tile indicates that the min and max
 *          values are not to be trusted.
 *      (2) If contrast (pixel difference) detection is expected to fail,
 *          caller should check return value.
 */
l_int32
pixSetLowContrast(PIX     *pixs1,
                  PIX     *pixs2,
                  l_int32  mindiff)
{
l_int32    i, j, w, h, d, wpl, val1, val2, found;
l_uint32  *data1, *data2, *line1, *line2;

    PROCNAME("pixSetLowContrast");

    if (!pixs1 || !pixs2)
        return ERROR_INT("pixs1 and pixs2 not both defined", procName, 1);
    if (pixSizesEqual(pixs1, pixs2) == 0)
        return ERROR_INT("pixs1 and pixs2 not equal size", procName, 1);
    pixGetDimensions(pixs1, &w, &h, &d);
    if (d != 8)
        return ERROR_INT("depth not 8 bpp", procName, 1);
    if (mindiff > 254) return 0;

    data1 = pixGetData(pixs1);
    data2 = pixGetData(pixs2);
    wpl = pixGetWpl(pixs1);
    found = 0;  /* init to not finding any diffs >= mindiff */
    for (i = 0; i < h; i++) {
        line1 = data1 + i * wpl;
        line2 = data2 + i * wpl;
        for (j = 0; j < w; j++) {
            val1 = GET_DATA_BYTE(line1, j);
            val2 = GET_DATA_BYTE(line2, j);
            if (L_ABS(val1 - val2) >= mindiff) {
                found = 1;
                break;
            }
        }
        if (found) break;
    }
    if (!found) {
        L_WARNING("no pixel pair diffs as large as mindiff\n", procName);
        pixClearAll(pixs1);
        pixClearAll(pixs2);
        return 1;
    }

    for (i = 0; i < h; i++) {
        line1 = data1 + i * wpl;
        line2 = data2 + i * wpl;
        for (j = 0; j < w; j++) {
            val1 = GET_DATA_BYTE(line1, j);
            val2 = GET_DATA_BYTE(line2, j);
            if (L_ABS(val1 - val2) < mindiff) {
                SET_DATA_BYTE(line1, j, 0);
                SET_DATA_BYTE(line2, j, 0);
            }
        }
    }

    return 0;
}


/*!
 *  pixLinearTRCTiled()
 *
 *      Input:  pixd (<optional> 8 bpp)
 *              pixs (8 bpp, not colormapped)
 *              sx, sy (tile dimensions)
 *              pixmin (pix of min values in tiles)
 *              pixmax (pix of max values in tiles)
 *      Return: pixd always
 *
 *  Notes:
 *      (1) pixd can be equal to pixs (in-place operation) or
 *          null (makes a new pixd).
 *      (2) sx and sy give the tile size; they are typically at least 20.
 *      (3) pixmin and pixmax are generated by pixMinMaxTiles()
 *      (4) For each tile, this does a linear expansion of the dynamic
 *          range so that the min value in the tile becomes 0 and the
 *          max value in the tile becomes 255.
 *      (5) The LUTs that do the mapping are generated as needed
 *          and stored for reuse in an integer array within the ptr array iaa[].
 */
PIX *
pixLinearTRCTiled(PIX       *pixd,
                  PIX       *pixs,
                  l_int32    sx,
                  l_int32    sy,
                  PIX       *pixmin,
                  PIX       *pixmax)
{
l_int32    i, j, k, m, w, h, wt, ht, wpl, wplt, xoff, yoff;
l_int32    minval, maxval, val, sval;
l_int32   *ia;
l_int32  **iaa;
l_uint32  *data, *datamin, *datamax, *line, *tline, *linemin, *linemax;

    PROCNAME("pixLinearTRCTiled");

    if (!pixs || pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs undefined or not 8 bpp", procName, pixd);
    if (pixd && pixd != pixs)
        return (PIX *)ERROR_PTR("pixd not null or == pixs", procName, pixd);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs is colormapped", procName, pixd);
    if (!pixmin || !pixmax)
        return (PIX *)ERROR_PTR("pixmin & pixmax not defined", procName, pixd);
    if (sx < 5 || sy < 5)
        return (PIX *)ERROR_PTR("sx and/or sy less than 5", procName, pixd);

    pixd = pixCopy(pixd, pixs);
    iaa = (l_int32 **)CALLOC(256, sizeof(l_int32 *));
    pixGetDimensions(pixd, &w, &h, NULL);

    data = pixGetData(pixd);
    wpl = pixGetWpl(pixd);
    datamin = pixGetData(pixmin);
    datamax = pixGetData(pixmax);
    wplt = pixGetWpl(pixmin);
    pixGetDimensions(pixmin, &wt, &ht, NULL);
    for (i = 0; i < ht; i++) {
        line = data + sy * i * wpl;
        linemin = datamin + i * wplt;
        linemax = datamax + i * wplt;
        yoff = sy * i;
        for (j = 0; j < wt; j++) {
            xoff = sx * j;
            minval = GET_DATA_BYTE(linemin, j);
            maxval = GET_DATA_BYTE(linemax, j);
            if (maxval == minval) {  /* this is bad */
/*                fprintf(stderr, "should't happen! i,j = %d,%d, minval = %d\n",
                        i, j, minval); */
                continue;
            }
            ia = iaaGetLinearTRC(iaa, maxval - minval);
            for (k = 0; k < sy && yoff + k < h; k++) {
                tline = line + k * wpl;
                for (m = 0; m < sx && xoff + m < w; m++) {
                    val = GET_DATA_BYTE(tline, xoff + m);
                    sval = val - minval;
                    sval = L_MAX(0, sval);
                    SET_DATA_BYTE(tline, xoff + m, ia[sval]);
                }
            }
        }
    }

    for (i = 0; i < 256; i++)
        if (iaa[i]) FREE(iaa[i]);
    FREE(iaa);
    return pixd;
}


/*!
 *  iaaGetLinearTRC()
 *
 *      Input:  iaa (bare array of ptrs to l_int32)
 *              diff (between min and max pixel values that are
 *                    to be mapped to 0 and 255)
 *      Return: ia (LUT with input (val - minval) and output a
 *                  value between 0 and 255)
 */
static l_int32 *
iaaGetLinearTRC(l_int32  **iaa,
                l_int32    diff)
{
l_int32    i;
l_int32   *ia;
l_float32  factor;

    PROCNAME("iaaGetLinearTRC");

    if (!iaa)
        return (l_int32 *)ERROR_PTR("iaa not defined", procName, NULL);

    if (iaa[diff] != NULL)  /* already have it */
       return iaa[diff];

    if ((ia = (l_int32 *)CALLOC(256, sizeof(l_int32))) == NULL)
        return (l_int32 *)ERROR_PTR("ia not made", procName, NULL);
    iaa[diff] = ia;
    if (diff == 0) {  /* shouldn't happen */
        for (i = 0; i < 256; i++)
            ia[i] = 128;
    }
    else {
        factor = 255. / (l_float32)diff;
        for (i = 0; i < diff + 1; i++)
            ia[i] = (l_int32)(factor * i + 0.5);
        for (i = diff + 1; i < 256; i++)
            ia[i] = 255;
    }

    return ia;
}
