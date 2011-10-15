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
 *  binarize.c
 *                     
 *  ===================================================================
 *  Image binarization algorithms are found in:
 *    grayquant.c:   standard, simple, general grayscale quantization
 *    adaptmap.c:    local adaptive; mostly gray-to-gray in preparation
 *                   for binarization
 *    binarize.c:    special binarization methods, locally adaptive.
 *  ===================================================================
 *
 *      Adaptive Otsu-based thresholding
 *          l_int32    pixOtsuAdaptiveThreshold()       8 bpp
 *
 *      Otsu thresholding on adaptive background normalization
 *          PIX       *pixOtsuThreshOnBackgroundNorm()  8 bpp
 *
 *      Masking and Otsu estimate on adaptive background normalization
 *          PIX       *pixMaskedThreshOnBackgroundNorm()  8 bpp
 *
 *      Sauvola local thresholding
 *          l_int32    pixSauvolaBinarizeTiled()
 *          l_int32    pixSauvolaBinarize()
 *          PIX       *pixSauvolaGetThreshold()
 *          PIX       *pixApplyLocalThreshold();
 *
 *  Notes:
 *      (1) pixOtsuAdaptiveThreshold() computes a global threshold over each
 *          tile and performs the threshold operation, resulting in a
 *          binary image for each tile.  These are stitched into the
 *          final result.
 *      (2) pixOtsuThreshOnBackgroundNorm() and
 *          pixMaskedThreshOnBackgroundNorm() are binarization functions
 *          that use background normalization with other techniques.
 *      (3) Sauvola binarization computes a local threshold based on
 *          the local average and square average.  It takes two constants:
 *          the window size for the measurment at each pixel and a
 *          parameter that determines the amount of normalized local
 *          standard deviation to subtract from the local average value.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allheaders.h"


/*------------------------------------------------------------------*
 *                 Adaptive Otsu-based thresholding                 *
 *------------------------------------------------------------------*/
/*!
 *  pixOtsuAdaptiveThreshold()
 *
 *      Input:  pixs (8 bpp)
 *              sx, sy (desired tile dimensions; actual size may vary)
 *              smoothx, smoothy (half-width of convolution kernel applied to
 *                                threshold array: use 0 for no smoothing)
 *              scorefract (fraction of the max Otsu score; typ. 0.1;
 *                          use 0.0 for standard Otsu)
 *              &pixth (<optional return> array of threshold values
 *                      found for each tile)
 *              &pixd (<optional return> thresholded input pixs, based on
 *                     the threshold array)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The Otsu method finds a single global threshold for an image.
 *          This function allows a locally adapted threshold to be
 *          found for each tile into which the image is broken up.
 *      (2) The array of threshold values, one for each tile, constitutes
 *          a highly downscaled image.  This array is optionally
 *          smoothed using a convolution.  The full width and height of the
 *          convolution kernel are (2 * @smoothx + 1) and (2 * @smoothy + 1).
 *      (3) The minimum tile dimension allowed is 16.  If such small
 *          tiles are used, it is recommended to use smoothing, because
 *          without smoothing, each small tile determines the splitting
 *          threshold independently.  A tile that is entirely in the
 *          image bg will then hallucinate fg, resulting in a very noisy
 *          binarization.  The smoothing should be large enough that no
 *          tile is only influenced by one type (fg or bg) of pixels,
 *          because it will force a split of its pixels.
 *      (4) To get a single global threshold for the entire image, use
 *          input values of @sx and @sy that are larger than the image.
 *          For this situation, the smoothing parameters are ignored.
 *      (5) The threshold values partition the image pixels into two classes:
 *          one whose values are less than the threshold and another
 *          whose values are greater than or equal to the threshold.
 *          This is the same use of 'threshold' as in pixThresholdToBinary().
 *      (6) The scorefract is the fraction of the maximum Otsu score, which
 *          is used to determine the range over which the histogram minimum
 *          is searched.  See numaSplitDistribution() for details on the
 *          underlying method of choosing a threshold.
 *      (7) This uses enables a modified version of the Otsu criterion for
 *          splitting the distribution of pixels in each tile into a
 *          fg and bg part.  The modification consists of searching for
 *          a minimum in the histogram over a range of pixel values where
 *          the Otsu score is within a defined fraction, @scorefract,
 *          of the max score.  To get the original Otsu algorithm, set
 *          @scorefract == 0.
 */
l_int32
pixOtsuAdaptiveThreshold(PIX       *pixs,
                         l_int32    sx,
                         l_int32    sy,
                         l_int32    smoothx,
                         l_int32    smoothy,
                         l_float32  scorefract,
                         PIX      **ppixth,
                         PIX      **ppixd)
{
l_int32     w, h, nx, ny, i, j, thresh;
l_uint32    val;
PIX        *pixt, *pixb, *pixthresh, *pixth, *pixd;
PIXTILING  *pt;

    PROCNAME("pixOtsuAdaptiveThreshold");

    if (!ppixth && !ppixd)
        return ERROR_INT("neither &pixth nor &pixd defined", procName, 1);
    if (ppixth) *ppixth = NULL;
    if (ppixd) *ppixd = NULL;
    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (sx < 16 || sy < 16)
        return ERROR_INT("sx and sy must be >= 16", procName, 1);

        /* Compute the threshold array for the tiles */
    pixGetDimensions(pixs, &w, &h, NULL);
    nx = L_MAX(1, w / sx);
    ny = L_MAX(1, h / sy);
    smoothx = L_MIN(smoothx, (nx - 1) / 2);
    smoothy = L_MIN(smoothy, (ny - 1) / 2);
    pt = pixTilingCreate(pixs, nx, ny, 0, 0, 0, 0);
    pixthresh = pixCreate(nx, ny, 8);
    for (i = 0; i < ny; i++) {
        for (j = 0; j < nx; j++) {
            pixt = pixTilingGetTile(pt, i, j);
            pixSplitDistributionFgBg(pixt, scorefract, 1, &thresh,
                                     NULL, NULL, 0);
            pixSetPixel(pixthresh, j, i, thresh);  /* see note (4) */
            pixDestroy(&pixt);
        }
    }

        /* Optionally smooth the threshold array */
    if (smoothx > 0 || smoothy > 0)
        pixth = pixBlockconv(pixthresh, smoothx, smoothy);
    else
        pixth = pixClone(pixthresh);
    pixDestroy(&pixthresh);

        /* Optionally apply the threshold array to binarize pixs */
    if (ppixd) {
        pixd = pixCreate(w, h, 1);
        for (i = 0; i < ny; i++) {
            for (j = 0; j < nx; j++) {
                pixt = pixTilingGetTile(pt, i, j);
                pixGetPixel(pixth, j, i, &val);
                pixb = pixThresholdToBinary(pixt, val);
                pixTilingPaintTile(pixd, i, j, pixb, pt);
                pixDestroy(&pixt);
                pixDestroy(&pixb);
            }
        }
        *ppixd = pixd;
    }

    if (ppixth)
        *ppixth = pixth;
    else
        pixDestroy(&pixth);

    pixTilingDestroy(&pt);
    return 0;
}


/*------------------------------------------------------------------*
 *      Otsu thresholding on adaptive background normalization      *
 *------------------------------------------------------------------*/
/*!
 *  pixOtsuThreshOnBackgroundNorm()
 *
 *      Input:  pixs (8 bpp grayscale; not colormapped)
 *              pixim (<optional> 1 bpp 'image' mask; can be null)
 *              sx, sy (tile size in pixels)
 *              thresh (threshold for determining foreground)
 *              mincount (min threshold on counts in a tile)
 *              bgval (target bg val; typ. > 128)
 *              smoothx (half-width of block convolution kernel width)
 *              smoothy (half-width of block convolution kernel height)
 *              scorefract (fraction of the max Otsu score; typ. 0.1)
 *              &thresh (<optional return> threshold value that was
 *                       used on the normalized image)
 *      Return: pixd (1 bpp thresholded image), or null on error
 *
 *  Notes:
 *      (1) This does background normalization followed by Otsu
 *          thresholding.  Otsu binarization attempts to split the
 *          image into two roughly equal sets of pixels, and it does
 *          a very poor job when there are large amounts of dark
 *          background.  By doing a background normalization first,
 *          to get the background near 255, we remove this problem.
 *          Then we use a modified Otsu to estimate the best global
 *          threshold on the normalized image.
 *      (2) See pixBackgroundNorm() for meaning and typical values
 *          of input parameters.  For a start, you can try:
 *            sx, sy = 10, 15
 *            thresh = 100
 *            mincount = 50
 *            bgval = 255
 *            smoothx, smoothy = 2
 */
PIX *
pixOtsuThreshOnBackgroundNorm(PIX       *pixs,
                              PIX       *pixim,
                              l_int32    sx,
                              l_int32    sy,
                              l_int32    thresh,
                              l_int32    mincount,
                              l_int32    bgval,
                              l_int32    smoothx,
                              l_int32    smoothy,
                              l_float32  scorefract,
                              l_int32   *pthresh)
{
l_int32   w, h;
l_uint32  val;
PIX      *pixn, *pixt, *pixd;

    PROCNAME("pixOtsuThreshOnBackgroundNorm");

    if (pthresh) *pthresh = 0;
    if (!pixs || pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs is colormapped", procName, NULL);
    if (sx < 4 || sy < 4)
        return (PIX *)ERROR_PTR("sx and sy must be >= 4", procName, NULL);
    if (mincount > sx * sy) {
        L_WARNING("mincount too large for tile size", procName);
        mincount = (sx * sy) / 3;
    }

    pixn = pixBackgroundNorm(pixs, pixim, NULL, sx, sy, thresh,
                             mincount, bgval, smoothx, smoothy);
    if (!pixn)
        return (PIX *)ERROR_PTR("pixn not made", procName, NULL);

        /* Just use 1 tile for a global threshold, which is stored
         * as a single pixel in pixt. */
    pixGetDimensions(pixn, &w, &h, NULL);
    pixOtsuAdaptiveThreshold(pixn, w, h, 0, 0, scorefract, &pixt, &pixd);
    pixDestroy(&pixn);

    if (pixt && pthresh) {
        pixGetPixel(pixt, 0, 0, &val);
        *pthresh = val;
    }
    pixDestroy(&pixt);

    if (!pixd)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    else
        return pixd;
}
    


/*----------------------------------------------------------------------*
 *    Masking and Otsu estimate on adaptive background normalization    *
 *----------------------------------------------------------------------*/
/*!
 *  pixMaskedThreshOnBackgroundNorm()
 *
 *      Input:  pixs (8 bpp grayscale; not colormapped)
 *              pixim (<optional> 1 bpp 'image' mask; can be null)
 *              sx, sy (tile size in pixels)
 *              thresh (threshold for determining foreground)
 *              mincount (min threshold on counts in a tile)
 *              smoothx (half-width of block convolution kernel width)
 *              smoothy (half-width of block convolution kernel height)
 *              scorefract (fraction of the max Otsu score; typ. ~ 0.1)
 *              &thresh (<optional return> threshold value that was
 *                       used on the normalized image)
 *      Return: pixd (1 bpp thresholded image), or null on error
 *
 *  Notes:
 *      (1) This begins with a standard background normalization.
 *          Additionally, there is a flexible background norm, that
 *          will adapt to a rapidly varying background, and this
 *          puts white pixels in the background near regions with
 *          significant foreground.  The white pixels are turned into
 *          a 1 bpp selection mask by binarization followed by dilation.
 *          Otsu thresholding is performed on the input image to get an
 *          estimate of the threshold in the non-mask regions.
 *          The background normalized image is thresholded with two
 *          different values, and the result is combined using
 *          the selection mask.
 *      (2) Note that the numbers 255 (for bgval target) and 190 (for
 *          thresholding on pixn) are tied together, and explicitly
 *          defined in this function.
 *      (3) See pixBackgroundNorm() for meaning and typical values
 *          of input parameters.  For a start, you can try:
 *            sx, sy = 10, 15
 *            thresh = 100
 *            mincount = 50
 *            smoothx, smoothy = 2
 */
PIX *
pixMaskedThreshOnBackgroundNorm(PIX       *pixs,
                                PIX       *pixim,
                                l_int32    sx,
                                l_int32    sy,
                                l_int32    thresh,
                                l_int32    mincount,
                                l_int32    smoothx,
                                l_int32    smoothy,
                                l_float32  scorefract,
                                l_int32   *pthresh)
{
l_int32   w, h;
l_uint32  val;
PIX      *pixn, *pixm, *pixd, *pixt1, *pixt2, *pixt3, *pixt4;

    PROCNAME("pixMaskedThreshOnBackgroundNorm");

    if (pthresh) *pthresh = 0;
    if (!pixs || pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs is colormapped", procName, NULL);
    if (sx < 4 || sy < 4)
        return (PIX *)ERROR_PTR("sx and sy must be >= 4", procName, NULL);
    if (mincount > sx * sy) {
        L_WARNING("mincount too large for tile size", procName);
        mincount = (sx * sy) / 3;
    }

        /* Standard background normalization */
    pixn = pixBackgroundNorm(pixs, pixim, NULL, sx, sy, thresh,
                             mincount, 255, smoothx, smoothy);
    if (!pixn)
        return (PIX *)ERROR_PTR("pixn not made", procName, NULL);

        /* Special background normalization for adaptation to quickly
         * varying background.  Threshold on the very light parts,
         * which tend to be near significant edges, and dilate to
         * form a mask over regions that are typically text.  The
         * dilation size is chosen to cover the text completely,
         * except for very thick fonts. */
    pixt1 = pixBackgroundNormFlex(pixs, 7, 7, 1, 1, 20);
    pixt2 = pixThresholdToBinary(pixt1, 240);
    pixInvert(pixt2, pixt2);
    pixm = pixMorphSequence(pixt2, "d21.21", 0);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

        /* Use Otsu to get a global threshold estimate for the image,
         * which is stored as a single pixel in pixt3. */
    pixGetDimensions(pixs, &w, &h, NULL);
    pixOtsuAdaptiveThreshold(pixs, w, h, 0, 0, scorefract, &pixt3, NULL);
    if (pixt3 && pthresh) {
        pixGetPixel(pixt3, 0, 0, &val);
        *pthresh = val;
    }
    pixDestroy(&pixt3);

        /* Threshold the background normalized images differentially,
         * using a high value correlated with the background normalization
         * for the part of the image under the mask (i.e., near the
         * darker, thicker foreground), and a value that depends on the Otsu
         * threshold for the rest of the image.  This gives a solid
         * (high) thresholding for the foreground parts of the image,
         * while allowing the background and light foreground to be
         * reasonably well cleaned using a threshold adapted to the
         * input image. */
    pixd = pixThresholdToBinary(pixn, val + 30);  /* for bg and light fg */
    pixt4 = pixThresholdToBinary(pixn, 190);  /* for heavier fg */
    pixCombineMasked(pixd, pixt4, pixm);
    pixDestroy(&pixt4);
    pixDestroy(&pixm);
    pixDestroy(&pixn);

    if (!pixd)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    else
        return pixd;
}
    

/*----------------------------------------------------------------------*
 *                           Sauvola binarization                       *
 *----------------------------------------------------------------------*/
/*!
 *  pixSauvolaBinarizeTiled()
 *
 *      Input:  pixs (8 bpp grayscale, not colormapped)
 *              whsize (window half-width for measuring local statistics)
 *              factor (factor for reducing threshold due to variance; >= 0)
 *              nx, ny (subdivision into tiles; >= 1)
 *              &pixth (<optional return> Sauvola threshold values)
 *              &pixd (<optional return> thresholded image)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The window width and height are 2 * @whsize + 1.  The minimum
 *          value for @whsize is 2; typically it is >= 7..
 *      (2) For nx == ny == 1, this defaults to pixSauvolaBinarize().
 *      (3) Why a tiled version?
 *          (a) Because the mean value accumulator is a uint32, overflow
 *              can occur for an image with more than 16M pixels.
 *          (b) The mean value accumulator array for 16M pixels is 64 MB.
 *              The mean square accumulator array for 16M pixels is 128 MB.
 *              Using tiles reduces the size of these arrays.
 *          (c) Each tile can be processed independently, in parallel,
 *              on a multicore processor.
 *      (4) The Sauvola threshold is determined from the formula:
 *              t = m * (1 - k * (1 - s / 128))
 *          See pixSauvolaBinarize() for details.
 */
l_int32
pixSauvolaBinarizeTiled(PIX       *pixs,
                        l_int32    whsize,
                        l_float32  factor,
                        l_int32    nx,
                        l_int32    ny,
                        PIX      **ppixth,
                        PIX      **ppixd)
{
l_int32     i, j, w, h, xrat, yrat;
PIX        *pixth, *pixd, *tileth, *tiled, *pixt;
PIX       **ptileth, **ptiled;
PIXTILING  *pt;

    PROCNAME("pixSauvolaBinarizeTiled");
    
    if (!ppixth && !ppixd)
        return ERROR_INT("no outputs", procName, 1);
    if (ppixth) *ppixth = NULL;
    if (ppixd) *ppixd = NULL;
    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs undefined or not 8 bpp", procName, 1);
    if (pixGetColormap(pixs))
        return ERROR_INT("pixs is cmapped", procName, 1);
    pixGetDimensions(pixs, &w, &h, NULL);
    if (whsize < 2)
        return ERROR_INT("whsize must be >= 2", procName, 1);
    if (w < 2 * whsize + 3 || h < 2 * whsize + 3)
        return ERROR_INT("whsize too large for image", procName, 1);
    if (factor < 0.0)
        return ERROR_INT("factor must be >= 0", procName, 1);

    if (nx <= 1 && ny <= 1)
        return pixSauvolaBinarize(pixs, whsize, factor, 1, NULL, NULL,
                                  ppixth, ppixd);

        /* Test to see if the tiles are too small.  The required
         * condition is that the tile dimensions must be at least
         * (whsize + 2) x (whsize + 2).  */
    xrat = w / nx;
    yrat = h / ny;
    if (xrat < whsize + 2) {
        nx = w / (whsize + 2);
        L_WARNING_INT("tile width too small; nx reduced to %d", procName, nx);
    }
    if (yrat < whsize + 2) {
        ny = h / (whsize + 2);
        L_WARNING_INT("tile height too small; ny reduced to %d", procName, ny);
    }
    if (nx <= 1 && ny <= 1)
        return pixSauvolaBinarize(pixs, whsize, factor, 1, NULL, NULL,
                                  ppixth, ppixd);

        /* We can use pixtiling for painting both outputs, if requested */
    if (ppixth) {
        pixth = pixCreateNoInit(w, h, 8);
        *ppixth = pixth;
    }
    if (ppixd) {
        pixd = pixCreateNoInit(w, h, 1);
        *ppixd = pixd;
    }
    pt = pixTilingCreate(pixs, nx, ny, 0, 0, whsize + 1, whsize + 1);
    pixTilingNoStripOnPaint(pt);  /* pixSauvolaBinarize() does the stripping */

    for (i = 0; i < ny; i++) {
        for (j = 0; j < nx; j++) {
            pixt = pixTilingGetTile(pt, i, j);
            ptileth = (ppixth) ? &tileth : NULL;
            ptiled = (ppixd) ? &tiled : NULL;
            pixSauvolaBinarize(pixt, whsize, factor, 0, NULL, NULL,
                               ptileth, ptiled);
            if (ppixth) {  /* do not strip */
                pixTilingPaintTile(pixth, i, j, tileth, pt);
                pixDestroy(&tileth);
            }
            if (ppixd) {
                pixTilingPaintTile(pixd, i, j, tiled, pt);
                pixDestroy(&tiled);
            }
            pixDestroy(&pixt);
        }
    }

    pixTilingDestroy(&pt);
    return 0;
}


/*!
 *  pixSauvolaBinarize()
 *
 *      Input:  pixs (8 bpp grayscale; not colormapped)
 *              whsize (window half-width for measuring local statistics)
 *              factor (factor for reducing threshold due to variance; >= 0)
 *              addborder (1 to add border of width (@whsize + 1) on all sides)
 *              &pixm (<optional return> local mean values)
 *              &pixsd (<optional return> local standard deviation values)
 *              &pixth (<optional return> threshold values)
 *              &pixd (<optional return> thresholded image)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The window width and height are 2 * @whsize + 1.  The minimum
 *          value for @whsize is 2; typically it is >= 7..
 *      (2) The local statistics, measured over the window, are the
 *          average and standard deviation.
 *      (3) The measurements of the mean and standard deviation are
 *          performed inside a border of (@whsize + 1) pixels.  If pixs does
 *          not have these added border pixels, use @addborder = 1 to add
 *          it here; otherwise use @addborder = 0.
 *      (4) The Sauvola threshold is determined from the formula:
 *            t = m * (1 - k * (1 - s / 128))
 *          where:
 *            t = local threshold
 *            m = local mean
 *            k = @factor (>= 0)   [ typ. 0.35 ]
 *            s = local standard deviation, which is maximized at
 *                127.5 when half the samples are 0 and half are 255.
 *      (5) The basic idea of Niblack and Sauvola binarization is that
 *          the local threshold should be less than the median value,
 *          and the larger the variance, the closer to the median
 *          it should be chosen.  Typical values for k are between
 *          0.2 and 0.5.
 */
l_int32
pixSauvolaBinarize(PIX       *pixs,
                   l_int32    whsize,
                   l_float32  factor,
                   l_int32    addborder,
                   PIX      **ppixm,
                   PIX      **ppixsd,
                   PIX      **ppixth,
                   PIX      **ppixd)
{
l_int32  w, h;
PIX     *pixg, *pixsc, *pixm, *pixms, *pixth, *pixd;

    PROCNAME("pixSauvolaBinarize");

    
    if (!ppixm && !ppixsd && !ppixth && !ppixd)
        return ERROR_INT("no outputs", procName, 1);
    if (ppixm) *ppixm = NULL;
    if (ppixsd) *ppixsd = NULL;
    if (ppixth) *ppixth = NULL;
    if (ppixd) *ppixd = NULL;
    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs undefined or not 8 bpp", procName, 1);
    if (pixGetColormap(pixs))
        return ERROR_INT("pixs is cmapped", procName, 1);
    pixGetDimensions(pixs, &w, &h, NULL);
    if (whsize < 2)
        return ERROR_INT("whsize must be >= 2", procName, 1);
    if (w < 2 * whsize + 3 || h < 2 * whsize + 3)
        return ERROR_INT("whsize too large for image", procName, 1);
    if (factor < 0.0)
        return ERROR_INT("factor must be >= 0", procName, 1);

    if (addborder) {
        pixg = pixAddMirroredBorder(pixs, whsize + 1, whsize + 1,
                                    whsize + 1, whsize + 1);
        pixsc = pixClone(pixs);
    }
    else {
        pixg = pixClone(pixs);
        pixsc = pixRemoveBorder(pixs, whsize + 1);
    }
    if (!pixg || !pixsc)
        return ERROR_INT("pixg and pixsc not made", procName, 1);

        /* All these functions strip off the border pixels. */
    if (ppixm || ppixth || ppixd)
        pixm = pixWindowedMean(pixg, whsize, whsize, 1);
    if (ppixsd || ppixth || ppixd)
        pixms = pixWindowedMeanSquare(pixg, whsize);
    if (ppixth || ppixd)
        pixth = pixSauvolaGetThreshold(pixm, pixms, factor, ppixsd);
    if (ppixd)
        pixd = pixApplyLocalThreshold(pixsc, pixth, 1);

    if (ppixm)
        *ppixm = pixm;
    else
        pixDestroy(&pixm);
    pixDestroy(&pixms);
    if (ppixth)
        *ppixth = pixth;
    else
        pixDestroy(&pixth);
    if (ppixd)
        *ppixd = pixd;
    else
        pixDestroy(&pixd);
    pixDestroy(&pixg);
    pixDestroy(&pixsc);
    return 0;
}


/*!
 *  pixSauvolaGetThreshold()
 *
 *      Input:  pixm (8 bpp grayscale; not colormapped)
 *              pixms (32 bpp)
 *              factor (factor for reducing threshold due to variance; >= 0)
 *              &pixsd (<optional return> local standard deviation)
 *      Return: pixd (8 bpp, sauvola threshold values), or null on error
 *
 *  Notes:
 *      (1) The Sauvola threshold is determined from the formula:
 *            t = m * (1 - k * (1 - s / 128))
 *          where:
 *            t = local threshold
 *            m = local mean
 *            k = @factor (>= 0)   [ typ. 0.35 ]
 *            s = local standard deviation, which is maximized at
 *                127.5 when half the samples are 0 and half are 255.
 *      (2) See pixSauvolaBinarize() for other details.
 *      (3) Important definitions and relations for computing averages:
 *            v == pixel value
 *            E(p) == expected value of p == average of p over some pixel set
 *            S(v) == square of v == v * v
 *            mv == E(v) == expected pixel value == mean value
 *            ms == E(S(v)) == expected square of pixel values
 *               == mean square value
 *            var == variance == expected square of deviation from mean
 *                == E(S(v - mv)) = E(S(v) - 2 * S(v * mv) + S(mv))
 *                                = E(S(v)) - S(mv)
 *                                = ms - mv * mv
 *            s == standard deviation = sqrt(var)
 *          So for evaluating the standard deviation in the Sauvola
 *          threshold, we take
 *            s = sqrt(ms - mv * mv)
 */
PIX *
pixSauvolaGetThreshold(PIX       *pixm,
                       PIX       *pixms,
                       l_float32  factor,
                       PIX      **ppixsd)
{
l_int32     i, j, w, h, tabsize, wplm, wplms, wplsd, wpld, usetab;
l_int32     mv, ms, var, thresh;
l_uint32   *datam, *datams, *datasd, *datad;
l_uint32   *linem, *linems, *linesd, *lined;
l_float32   sd;
l_float32  *tab;  /* of 2^16 square roots */
PIX        *pixsd, *pixd;

    PROCNAME("pixSauvolaGetThreshold");
    
    if (ppixsd) *ppixsd = NULL;
    if (!pixm || pixGetDepth(pixm) != 8)
        return (PIX *)ERROR_PTR("pixm undefined or not 8 bpp", procName, NULL);
    if (pixGetColormap(pixm))
        return (PIX *)ERROR_PTR("pixm is colormapped", procName, NULL);
    if (!pixms || pixGetDepth(pixms) != 32)
        return (PIX *)ERROR_PTR("pixms undefined or not 32 bpp",
                                procName, NULL);
    if (factor < 0.0)
        return (PIX *)ERROR_PTR("factor must be >= 0", procName, NULL);

        /* Only make a table of 2^16 square roots if there
         * are enough pixels to justify it. */
    pixGetDimensions(pixm, &w, &h, NULL);
    usetab = (w * h > 100000) ? 1 : 0;
    if (usetab) {
        tabsize = 1 << 16;
        tab = (l_float32 *)CALLOC(tabsize, sizeof(l_float32));
        for (i = 0; i < tabsize; i++)
            tab[i] = (l_float32)sqrt((l_float64)i);
    }

    pixd = pixCreate(w, h, 8);
    if (ppixsd) {
        pixsd = pixCreate(w, h, 8);
        *ppixsd = pixsd;
    }
    datam = pixGetData(pixm);
    datams = pixGetData(pixms);
    if (ppixsd) datasd = pixGetData(pixsd);
    datad = pixGetData(pixd);
    wplm = pixGetWpl(pixm);
    wplms = pixGetWpl(pixms);
    if (ppixsd) wplsd = pixGetWpl(pixsd);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        linem = datam + i * wplm;
        linems = datams + i * wplms;
        if (ppixsd) linesd = datasd + i * wplsd;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            mv = GET_DATA_BYTE(linem, j);
            ms = linems[j];
            var = ms - mv * mv;
            if (usetab)
                sd = tab[var];
            else
                sd = (l_float32)sqrt(var);
            if (ppixsd) SET_DATA_BYTE(linesd, j, (l_int32)sd);
            thresh = (l_int32)(mv * (1.0 - factor * (1.0 - sd / 128.)));
            SET_DATA_BYTE(lined, j, thresh);
        }
    }

    if (usetab) FREE(tab);
    return pixd;
}


/*!
 *  pixApplyLocalThreshold()
 *
 *      Input:  pixs (8 bpp grayscale; not colormapped)
 *              pixth (8 bpp array of local thresholds)
 *              redfactor ( ... )
 *      Return: pixd (1 bpp, thresholded image), or null on error
 */
PIX *
pixApplyLocalThreshold(PIX     *pixs,
                       PIX     *pixth,
                       l_int32  redfactor)
{
l_int32    i, j, w, h, wpls, wplt, wpld, vals, valt;
l_uint32  *datas, *datat, *datad, *lines, *linet, *lined;
PIX       *pixd;

    PROCNAME("pixApplyLocalThreshold");
    
    if (!pixs || pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs is colormapped", procName, NULL);
    if (!pixth || pixGetDepth(pixth) != 8)
        return (PIX *)ERROR_PTR("pixth undefined or not 8 bpp", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    pixd = pixCreate(w, h, 1);
    datas = pixGetData(pixs);
    datat = pixGetData(pixth);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wplt = pixGetWpl(pixth);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        linet = datat + i * wplt;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            vals = GET_DATA_BYTE(lines, j);
            valt = GET_DATA_BYTE(linet, j);
            if (vals < valt)
                SET_DATA_BIT(lined, j);
        }
    }

    return pixd;
}


