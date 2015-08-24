/*
 * Copyright (C) 2014 Robert Theis
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
 * Edge detection.
 */
public class Edge {
    static {
        System.loadLibrary("pngt");
        System.loadLibrary("lept");
    }

    // Edge orientation flags

    /** Filters for horizontal edges */
    public static final int L_HORIZONTAL_EDGES = 0;

    /** Filters for vertical edges */
    public static final int L_VERTICAL_EDGES = 1;

    /** Filters for all edges */
    public static final int L_ALL_EDGES = 2;

    /**
     * Performs a Sobel edge detecting filter.
     * <p>
     * To use both the vertical and horizontal filters, set the orientation
     * flag to L_ALL_EDGES; this sums the abs. value of their outputs,
     * clipped to 255.
     * <p>
     * Notes:
     * <ol>
     * <li> Invert pixd to see larger gradients as darker (grayscale).
     * <li> To generate a binary image of the edges, threshold the result
     * using pixThresholdToBinary().  If the high edge values are to be fg (1),
     * invert after running pixThresholdToBinary().
     * <li> Label the pixels as follows:
     * <p>
     * <p>         1    4    7
     * <p>         2    5    8
     * <p>         3    6    9
     * <p>
     * Read the data incrementally across the image and unroll the loop.
     * <li> This runs at about 45 Mpix/sec on a 3 GHz processor.
     * </ol>
     * 
     * @param pixs Source pix (8 bpp; no colormap)
     * @param orientFlag Edge orientation flag (L_HORIZONTAL_EDGES, 
     *        L_VERTICAL_EDGES, L_ALL_EDGES)
     * @return a new Pix image (8bpp, edges are brighter), or null on error
     */
    public static Pix pixSobelEdgeFilter(Pix pixs, int orientFlag) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");
        if (pixs.getDepth() != 8)
            throw new IllegalArgumentException("Source pix depth must be 8bpp");
        if (orientFlag < 0 || orientFlag > 2)
            throw new IllegalArgumentException("Invalid orientation flag");

        long nativePix = nativePixSobelEdgeFilter(pixs.getNativePix(), 
                orientFlag);

        if (nativePix == 0)
            throw new RuntimeException("Failed to perform Sobel edge filter on image");

        return new Pix(nativePix);
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native long nativePixSobelEdgeFilter(long nativePix, int orientFlag);
}
