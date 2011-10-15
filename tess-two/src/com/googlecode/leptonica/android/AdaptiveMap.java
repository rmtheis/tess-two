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
        System.loadLibrary("lept");
    }

    // Background normalization constants

    /** Image reduction value; possible values are 1, 2, 4, 8 */
    private final static int NORM_REDUCTION = 16;

    /** Desired tile size; actual size may vary */
    private final static int NORM_SIZE = 3;

    /** Background brightness value; values over 200 may result in clipping */
    private final static int NORM_BG_VALUE = 200;

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

        int nativePix = nativeBackgroundNormMorph(
                pixs.mNativePix, normReduction, normSize, normBgValue);

        if (nativePix == 0)
            throw new RuntimeException("Failed to normalize image background");

        return new Pix(nativePix);
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native int nativeBackgroundNormMorph(
            int nativePix, int reduction, int size, int bgval);
}
