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
 * Image scaling methods.
 * 
 * @author alanv@google.com (Alan Viverette)
 */
public class Scale {
    static {
        System.loadLibrary("pngt");
        System.loadLibrary("lept");
    }

    public enum ScaleType {
        /* Scale in X and Y independently, so that src matches dst exactly. */
        FILL,

        /*
         * Compute a scale that will maintain the original src aspect ratio, but
         * will also ensure that src fits entirely inside dst. May shrink or
         * expand src to fit dst.
         */
        FIT,

        /*
         * Compute a scale that will maintain the original src aspect ratio, but
         * will also ensure that src fits entirely inside dst. May shrink src to
         * fit dst, but will not expand it.
         */
        FIT_SHRINK,
    }

    /**
     * Scales the Pix to a specified width and height using a specified scaling
     * type (fill, stretch, etc.). Returns a scaled image or a clone of the Pix
     * if no scaling is required.
     *
     * @param pixs
     * @param width
     * @param height
     * @param type
     * @return a scaled image or a clone of the Pix if no scaling is required
     */
    public static Pix scaleToSize(Pix pixs, int width, int height, ScaleType type) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");

        int pixWidth = pixs.getWidth();
        int pixHeight = pixs.getHeight();

        float scaleX = width / (float) pixWidth;
        float scaleY = height / (float) pixHeight;

        switch (type) {
            case FILL:
                // Retains default scaleX and scaleY values
                break;
            case FIT:
                scaleX = Math.min(scaleX, scaleY);
                scaleY = scaleX;
                break;
            case FIT_SHRINK:
                scaleX = Math.min(1.0f, Math.min(scaleX, scaleY));
                scaleY = scaleX;
                break;
        }

        return scale(pixs, scaleX, scaleY);
    }

    /**
     * Scales the Pix to specified scale. If no scaling is required, returns a
     * clone of the source Pix.
     *
     * @param pixs the source Pix
     * @param scale dimension scaling factor
     * @return a Pix scaled according to the supplied factors
     */
    public static Pix scale(Pix pixs, float scale) {
        return scale(pixs, scale, scale);
    }

    /**
     * Scales the Pix to the specified scale without sharpening.
     *
     * @param pixs the source Pix (1, 2, 4, 8, 16 and 32 bpp)
     * @param scale scaling factor for both X and Y
     * @return a Pix scaled while maintaining its aspect ratio
     */
    public static Pix scaleWithoutSharpening(Pix pixs, float scale) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");
        if (scale <= 0.0f)
            throw new IllegalArgumentException("Scaling factor must be positive");

        return new Pix(nativeScaleGeneral(pixs.getNativePix(), scale, scale,
                0f, 0));
    }

    /**
     * Scales the Pix to specified x and y scale. If no scaling is required,
     * returns a clone of the source Pix.
     *
     * @param pixs the source Pix
     * @param scaleX x-dimension (width) scaling factor
     * @param scaleY y-dimension (height) scaling factor
     * @return a Pix scaled according to the supplied factors
     */
    public static Pix scale(Pix pixs, float scaleX, float scaleY) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");
        if (scaleX <= 0.0f)
            throw new IllegalArgumentException("X scaling factor must be positive");
        if (scaleY <= 0.0f)
            throw new IllegalArgumentException("Y scaling factor must be positive");

        long nativePix = nativeScale(pixs.getNativePix(), scaleX, scaleY);

        if (nativePix == 0)
            throw new RuntimeException("Failed to natively scale pix");

        return new Pix(nativePix);
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native long nativeScale(long nativePix, float scaleX, float scaleY);
    private static native long nativeScaleGeneral(long nativePix, float scaleX, float scaleY, float sharpfract, int sharpwidth);

}
