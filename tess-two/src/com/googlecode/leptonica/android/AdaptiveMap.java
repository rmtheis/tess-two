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
 * Image adaptive mapping methods.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class AdaptiveMap {
    static {
        System.loadLibrary("pngt");
        System.loadLibrary("lept");
    }

    // Background normalization constants

    /** Image reduction value; possible values are 1, 2, 4, 8 */
    private final static int NORM_REDUCTION = 16;

    /** Desired tile size; actual size may vary */
    private final static int NORM_SIZE = 3;

    /** Background brightness value; values over 200 may result in clipping */
    private final static int NORM_BG_VALUE = 200;

    // Adaptive contrast normalization constants

    public final static int DEFAULT_TILE_WIDTH = 10;

    public final static int DEFAULT_TILE_HEIGHT = 15;

    public final static int DEFAULT_MIN_COUNT = 40;

    public final static int DEFAULT_X_SMOOTH_SIZE = 2;

    public final static int DEFAULT_Y_SMOOTH_SIZE = 1;

    /**
     * Normalizes an image's background using default parameters.
     *
     * @param pixs A source pix image.
     * @return the source pix image with a normalized background
     */
    public static Pix backgroundNormMorph(Pix pixs) {
        return backgroundNormMorph(pixs, NORM_REDUCTION, NORM_SIZE, NORM_BG_VALUE);
    }

    /**
     * Normalizes an image's background to a specified value.
     * <p>
     * Notes:
     * <ol>
     * <li>This is a top-level interface for normalizing the image intensity by
     * mapping the image so that the background is near the input value 'bgval'.
     * <li>The input image is either grayscale or rgb.
     * <li>For each component in the input image, the background value is
     * estimated using a grayscale closing; hence the 'Morph' in the function
     * name.
     * <li>An optional binary mask can be specified, with the foreground pixels
     * typically over image regions. The resulting background map values will be
     * determined by surrounding pixels that are not under the mask foreground.
     * The origin (0,0) of this mask is assumed to be aligned with the origin of
     * the input image. This binary mask must not fully cover pixs, because then
     * there will be no pixels in the input image available to compute the
     * background.
     * <li>The map is computed at reduced size (given by 'reduction') from the
     * input pixs and optional pixim. At this scale, pixs is closed to remove
     * the background, using a square Sel of odd dimension. The product of
     * reduction * size should be large enough to remove most of the text
     * foreground.
     * <li>No convolutional smoothing needs to be done on the map before
     * inverting it.
     * <li>A 'bgval' target background value for the normalized image. This
     * should be at least 128. If set too close to 255, some clipping will occur
     * in the result.
     * </ol>
     *
     * @param pixs A source pix image.
     * @param normReduction Reduction at which morphological closings are done.
     * @param normSize Size of square Sel for the closing.
     * @param normBgValue Target background value.
     * @return the source pix image with a normalized background
     */
    public static Pix backgroundNormMorph(
            Pix pixs, int normReduction, int normSize, int normBgValue) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");

        long nativePix = nativeBackgroundNormMorph(
                pixs.getNativePix(), normReduction, normSize, normBgValue);

        if (nativePix == 0)
            throw new RuntimeException("Failed to normalize image background");

        return new Pix(nativePix);
    }

    /**
     * Adaptively attempts to expand the contrast to the full dynamic range in 
     * each tile using default parameters.
     *
     * @see #pixContrastNorm(Pix, int, int, int, int, int)
     * 
     * @param pixs A source pix image
     * @return a new image with expanded contrast range
     */
    public static Pix pixContrastNorm(Pix pixs) {
        return pixContrastNorm(pixs, DEFAULT_TILE_WIDTH, DEFAULT_TILE_HEIGHT,
                DEFAULT_MIN_COUNT, DEFAULT_X_SMOOTH_SIZE, DEFAULT_Y_SMOOTH_SIZE);
    }

    /**
     * Adaptively attempts to expand the contrast to the full dynamic range in 
     * each tile.
     * <p>
     * Notes:
     * <ol>
     * <li>If the contrast in a tile is smaller than minDiff, it uses the min 
     * and max pixel values from neighboring tiles.  It also can use
     * convolution to smooth the min and max values from neighboring tiles.  
     * After all that processing, it is possible that the actual pixel values 
     * in the tile are outside the computed [min ... max] range for local 
     * contrast normalization. Such pixels are taken to be at either 0 (if 
     * below the min) or 255 (if above the max).
     * <li>sizeX and sizeY give the tile size; they are typically at least 20.
     * <li>minDiff is used to eliminate results for tiles where it is likely 
     * that either fg or bg is missing.  A value around 50 or more is 
     * reasonable.
     * <li>The full width and height of the convolution kernel are (2 * smoothx
     * + 1) and (2 * smoothy + 1).  Some smoothing is typically useful, and we 
     * limit the smoothing half-widths to the range from 0 to 8. Use 0 for no 
     * smoothing.
     * <li>A linear TRC (gamma = 1.0) is applied to increase the contrast in 
     * each tile. The result can subsequently be globally corrected, by 
     * applying pixGammaTRC() with arbitrary values of gamma and the 0 and 255 
     * points of the mapping.
     * </ol>
     *
     * @param pixs A source pix image
     * @param sizeX Tile width
     * @param sizeY Tile height
     * @param minDiff Minimum difference to accept as valid
     * @param smoothX Half-width of convolution kernel applied to min and max
     * arrays
     * @param smoothY Half-height of convolution kernel applied to min and max
     * arrays
     */
    public static Pix pixContrastNorm(
            Pix pixs, int sizeX, int sizeY, int minDiff, int smoothX, int smoothY) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");

        long nativePix = nativePixContrastNorm(
                pixs.getNativePix(), sizeX, sizeY, minDiff, smoothX, smoothY);

        if (nativePix == 0)
            throw new RuntimeException("Failed to normalize image contrast");

        return new Pix(nativePix);
    }    

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native long nativeBackgroundNormMorph(
            long nativePix, int reduction, int size, int bgval);

    private static native long nativePixContrastNorm(
            long nativePix, int sizeX, int sizeY, int minDiff, int smoothX, int smoothY);
}
