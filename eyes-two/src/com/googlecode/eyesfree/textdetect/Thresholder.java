/*
 * Copyright (C) 2011 Google Inc.
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

package com.googlecode.eyesfree.textdetect;

import com.googlecode.leptonica.android.Pix;

/**
 * @author alanv@google.com (Alan Viverette)
 */
public class Thresholder {
    static {
        System.loadLibrary("lept");
        System.loadLibrary("hydrogen");
    }

    /* Threshold under which pixels will be pulled low */
    public final static int SOBEL_THRESH = 64;

    public static Pix sobelEdgeThreshold(Pix pixs) {
        return sobelEdgeThreshold(pixs, SOBEL_THRESH);
    }

    public static Pix sobelEdgeThreshold(Pix pixs, int thresh) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");
        if (pixs.getDepth() != 8)
            throw new IllegalArgumentException("Source pix depth must be 8bpp");
        if (thresh >= 255 || thresh < 0)
            throw new IllegalArgumentException("Threshold must be in the range 0 <= thresh < 255");

        long nativePix = nativeSobelEdgeThreshold(pixs.getNativePix(), thresh);

        if (nativePix == 0)
            throw new RuntimeException("Failed to run Sobel edge threshold on Pix");

        return new Pix(nativePix);
    }

    /* Desired tile X dimension; actual size may vary */
    public static final int EDGE_TILE_X = 32;

    /* Desired tile Y dimension; actual size may vary */
    public static final int EDGE_TILE_Y = 64;

    /* Threshold for maximum edge, typially 32 */
    public static final int EDGE_THRESH = 32;

    /* Threshold for average edge, typially 1 */
    public static final int EDGE_AVERAGE = 1;

    /**
     * Returns a version of the image thresholded using Fisher's discriminant.
     *
     * @return a thresholded image or <code>null</code> on error
     */

    public static Pix edgeAdaptiveThreshold(Pix pixs) {
        return edgeAdaptiveThreshold(pixs, EDGE_TILE_X, EDGE_TILE_Y, EDGE_THRESH, EDGE_AVERAGE);
    }

    public static Pix edgeAdaptiveThreshold(
            Pix pixs, int tileX, int tileY, int threshold, int average) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");
        if (pixs.getDepth() != 8)
            throw new IllegalArgumentException("Source pix depth must be 8bpp");
        if (tileX < 8)
            throw new IllegalArgumentException("Tile width must be at least 8 pixels");
        if (tileY < 8)
            throw new IllegalArgumentException("Tile height must be at least 8 pixels");

        long nativePix = nativeEdgeAdaptiveThreshold(
                pixs.getNativePix(), tileX, tileY, threshold, average);

        if (nativePix == 0)
            throw new RuntimeException("Failed to run Fisher adaptive threshold on Pix");

        return new Pix(nativePix);
    }

    /* Desired tile X dimension; actual size may vary */
    public static final int FDR_TILE_X = 48;

    /* Desired tile Y dimension; actual size may vary */
    public static final int FDR_TILE_Y = 48;

    /* Fraction of the max Otsu score, typically 0.01 */
    public static final float FDR_SCORE_FRACT = 0.0f;

    /* Threshold for Fisher's Discriminant Rate, typially 3.5 */
    public static final float FDR_THRESH = 2.5f;

    /**
     * Returns a version of the image thresholded using Fisher's discriminant.
     *
     * @return a thresholded image or <code>null</code> on error
     */
    public static Pix fisherAdaptiveThreshold(Pix pixs) {
        return fisherAdaptiveThreshold(pixs, 30, 20);
    }

    public static Pix fisherAdaptiveThreshold(Pix pixs, int numTilesX, int numTilesY) {
        int tileX = pixs.getWidth() / numTilesX;
        int tileY = pixs.getHeight() / numTilesY;

        return fisherAdaptiveThreshold(pixs, tileX, tileY, FDR_SCORE_FRACT, FDR_THRESH);
    }

    public static Pix fisherAdaptiveThreshold(
            Pix pixs, int tileX, int tileY, float scoreFract, float thresh) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");
        if (pixs.getDepth() != 8)
            throw new IllegalArgumentException("Source pix depth must be 8bpp");
        if (tileX < 8)
            throw new IllegalArgumentException("Tile width must be at least 8 pixels");
        if (tileY < 8)
            throw new IllegalArgumentException("Tile height must be at least 8 pixels");

        long nativePix = nativeFisherAdaptiveThreshold(
                pixs.getNativePix(), tileX, tileY, scoreFract, thresh);

        if (nativePix == 0)
            throw new RuntimeException("Failed to run Fisher adaptive threshold on Pix");

        return new Pix(nativePix);
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native long nativeFisherAdaptiveThreshold(
            long nativePix, int tileX, int tileY, float scoreFract, float thresh);

    private static native long nativeEdgeAdaptiveThreshold(
            long nativePix, int tileX, int tileY, int threshold, int average);

    private static native long nativeSobelEdgeThreshold(long nativePix, int threshold);
}
