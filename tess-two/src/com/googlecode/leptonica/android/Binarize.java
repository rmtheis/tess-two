/*
 * Copyright (C) 2010 Google Inc.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package com.googlecode.leptonica.android;

/**
 * Image binarization methods.
 * 
 * @author alanv@google.com (Alan Viverette)
 */
public class Binarize {
    static {
        System.loadLibrary("lept");
    }

    // Otsu thresholding constants

    /** Desired tile X dimension; actual size may vary */
    public final static int OTSU_SIZE_X = 32;

    /** Desired tile Y dimension; actual size may vary */
    public final static int OTSU_SIZE_Y = 32;

    /** Desired X smoothing value */
    public final static int OTSU_SMOOTH_X = 2;

    /** Desired Y smoothing value */
    public final static int OTSU_SMOOTH_Y = 2;

    /** Fraction of the max Otsu score, typically 0.1 */
    public final static float OTSU_SCORE_FRACTION = 0.1f;

    /**
     * Performs locally-adaptive Otsu threshold binarization with default
     * parameters.
     *
     * @param pixs An 8 bpp PIX source image.
     * @return A 1 bpp thresholded PIX image.
     */
    public static Pix otsuAdaptiveThreshold(Pix pixs) {
        return otsuAdaptiveThreshold(
                pixs, OTSU_SIZE_X, OTSU_SIZE_Y, OTSU_SMOOTH_X, OTSU_SMOOTH_Y, OTSU_SCORE_FRACTION);
    }

    /**
     * Performs locally-adaptive Otsu threshold binarization.
     * <p>
     * Notes:
     * <ol>
     * <li>The Otsu method finds a single global threshold for an image. This
     * function allows a locally adapted threshold to be found for each tile
     * into which the image is broken up.
     * <li>The array of threshold values, one for each tile, constitutes a
     * highly downscaled image. This array is optionally smoothed using a
     * convolution. The full width and height of the convolution kernel are (2 *
     * smoothX + 1) and (2 * smoothY + 1).
     * <li>The minimum tile dimension allowed is 16. If such small tiles are
     * used, it is recommended to use smoothing, because without smoothing, each
     * small tile determines the splitting threshold independently. A tile that
     * is entirely in the image bg will then hallucinate fg, resulting in a very
     * noisy binarization. The smoothing should be large enough that no tile is
     * only influenced by one type (fg or bg) of pixels, because it will force a
     * split of its pixels.
     * <li>To get a single global threshold for the entire image, use input
     * values of sizeX and sizeY that are larger than the image. For this
     * situation, the smoothing parameters are ignored.
     * <li>The threshold values partition the image pixels into two classes: one
     * whose values are less than the threshold and another whose values are
     * greater than or equal to the threshold. This is the same use of
     * 'threshold' as in pixThresholdToBinary().
     * <li>The scorefract is the fraction of the maximum Otsu score, which is
     * used to determine the range over which the histogram minimum is searched.
     * See numaSplitDistribution() for details on the underlying method of
     * choosing a threshold.
     * <li>This uses enables a modified version of the Otsu criterion for
     * splitting the distribution of pixels in each tile into a fg and bg part.
     * The modification consists of searching for a minimum in the histogram
     * over a range of pixel values where the Otsu score is within a defined
     * fraction, scoreFraction, of the max score. To get the original Otsu
     * algorithm, set scoreFraction == 0.
     * </ol>
     *
     * @param pixs An 8 bpp PIX source image.
     * @param sizeX Desired tile X dimension; actual size may vary.
     * @param sizeY Desired tile Y dimension; actual size may vary.
     * @param smoothX Half-width of convolution kernel applied to threshold
     *            array: use 0 for no smoothing.
     * @param smoothY Half-height of convolution kernel applied to threshold
     *            array: use 0 for no smoothing.
     * @param scoreFraction Fraction of the max Otsu score; typ. 0.1 (use 0.0
     *            for standard Otsu).
     * @return A 1 bpp thresholded PIX image.
     */
    public static Pix otsuAdaptiveThreshold(
            Pix pixs, int sizeX, int sizeY, int smoothX, int smoothY, float scoreFraction) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");
        if (pixs.getDepth() != 8)
            throw new IllegalArgumentException("Source pix depth must be 8bpp");

        long nativePix = nativeOtsuAdaptiveThreshold(
                pixs.mNativePix, sizeX, sizeY, smoothX, smoothY, scoreFraction);

        if (nativePix == 0)
            throw new RuntimeException("Failed to perform Otsu adaptive threshold on image");

        return new Pix(nativePix);
    }

    /**
     * Performs Sauvola binarization.
     * <p>
     * Notes:
     * <ol>
     * <li> The window width and height are 2 * whsize + 1.  The minimum
     * value for whsize is 2; typically it is >= 7.
     * <li> For nx == ny == 1, this defaults to pixSauvolaBinarize().
     * <li> Why a tiled version?
     * (a) Because the mean value accumulator is a uint32, overflow
     * can occur for an image with more than 16M pixels.
     * (b) The mean value accumulator array for 16M pixels is 64 MB.
     * The mean square accumulator array for 16M pixels is 128 MB.
     * Using tiles reduces the size of these arrays.
     * (c) Each tile can be processed independently, in parallel,
     * on a multicore processor.
     * <li> The Sauvola threshold is determined from the formula:
     *   t = m * (1 - k * (1 - s / 128))
     * where:
     *   t = local threshold
     *   m = local mean
     *   k = @factor (>= 0)   [ typ. 0.35 ]
     *   s = local standard deviation, which is maximized at
     *       127.5 when half the samples are 0 and half are 255.
     * <li> The basic idea of Niblack and Sauvola binarization is that
     * the local threshold should be less than the median value, and the larger
     * the variance, the closer to the median it should be chosen.  Typical 
     * values for k are between 0.2 and 0.5.
     * </ol>
     *   
     * @param pixs An 8 bpp PIX source image.
     * @param whsize Window half-width for measuring local statistics
     * @param factor Factor for reducing threshold due to variance; >= 0
     * @param nx Subdivision into tiles; >= 1
     * @param ny Subdivision into tiles; >= 1
     * @return A 1 bpp thresholded PIX image.
     */
    public static Pix sauvolaBinarizeTiled(Pix pixs, int whsize, float factor, int nx, int ny) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");
        if (pixs.getDepth() != 8)
            throw new IllegalArgumentException("Source pix depth must be 8bpp");
        
        long nativePix = nativeSauvolaBinarizeTiled(pixs.mNativePix, whsize, factor, nx, ny);
        
        if (nativePix == 0)
            throw new RuntimeException("Failed to perform Otsu adaptive threshold on image");

        return new Pix(nativePix);        
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native long nativeOtsuAdaptiveThreshold(
            long nativePix, int sizeX, int sizeY, int smoothX, int smoothY, float scoreFract);

    private static native long nativeSauvolaBinarizeTiled(
            long nativePix, int whsize, float factor, int nx, int ny);
}
