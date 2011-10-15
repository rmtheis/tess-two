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

        int nativePix = nativeOtsuAdaptiveThreshold(
                pixs.mNativePix, sizeX, sizeY, smoothX, smoothY, scoreFraction);

        if (nativePix == 0)
            throw new RuntimeException("Failed to perform Otsu adaptive threshold on image");

        return new Pix(nativePix);
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native int nativeOtsuAdaptiveThreshold(
            int nativePix, int sizeX, int sizeY, int smoothX, int smoothY, float scoreFract);
}
