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
 * Composite image processing operations.
 */
public class MorphApp {
    static {
        System.loadLibrary("lept");
    }

    // Morphological tophat flags

    public static final int L_TOPHAT_WHITE = 0;
    public static final int L_TOPHAT_BLACK = 1;

    /**
     * Performs a tophat transform.
     * <p>
     * Notes:
     * <ol>
     * <li> Sel is a brick with all elements being hits
     * <li> If hsize = vsize = 1, returns an image with all 0 data.
     * <li> The L_TOPHAT_WHITE flag emphasizes small bright regions, whereas
     * the L_TOPHAT_BLACK flag emphasizes small dark regions. The L_TOPHAT_WHITE
     * tophat can be accomplished by doing a L_TOPHAT_BLACK tophat on the 
     * inverse, or v.v.
     * </ol>
     *     
     * @param pixs Source pix (8bpp)
     * @param hsize (of Sel; must be odd; origin implicitly in center)
     * @param vsize (ditto)
     * @param type L_TOPHAT_WHITE: image - opening or L_TOPHAT_BLACK: closing - image
     * @return a new Pix image
     */
    public static Pix pixTophat(Pix pixs, int hsize, int vsize, int type) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");
        if (pixs.getDepth() != 8)
            throw new IllegalArgumentException("Source pix depth must be 8bpp");
        if (hsize < 1 || vsize < 1)
            throw new IllegalArgumentException("hsize or vsize < 1");
        if (type < 0 || type > 1)
            throw new IllegalArgumentException("Type must be L_TOPHAT_BLACK or L_TOPHAT_WHITE");

        long nativePix = nativePixTophat(pixs.mNativePix, hsize, vsize, type);

        if (nativePix == 0)
            throw new RuntimeException("Failed to perform Tophat on image");

        return new Pix(nativePix); 
    }

    /**
     * Performs a tophat-like operation.
     * <p>
     * Notes:
     * <ol>
     * <li> Don't be fooled. This is NOT a tophat.  It is a tophat-like
     * operation, where the result is similar to what you'd get if you used an 
     * erosion instead of an opening, or a dilation instead of a closing.
     * 
     * <li> Instead of opening or closing at full resolution, it does a fast
     * downscale/minmax operation, then a quick small smoothing at low res, a 
     * replicative expansion of the "background" to full res, and finally a 
     * removal of the background level from the input image.  The smoothing 
     * step may not be important.
     * 
     * <li> It does not remove noise as well as a tophat, but it is 5 to 10 
     * times faster. If you need the preciseness of the tophat, don't use this.
     * <li> The L_TOPHAT_WHITE flag emphasizes small bright regions, whereas 
     * the L_TOPHAT_BLACK flag emphasizes small dark regions.
     * </ol>
     *
     * @param pixs Source pix (8bpp)
     * @param xsize width of max/min op, smoothing; any integer >= 1
     * @param ysize height of max/min op, smoothing; any integer >= 1
     * @param type L_TOPHAT_WHITE: image - min, or L_TOPHAT_BLACK: max - image
     * @return a new Pix image
     */
    public static Pix pixFastTophat(Pix pixs, int xsize, int ysize, int type) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");
        if (pixs.getDepth() != 8)
            throw new IllegalArgumentException("Source pix depth must be 8bpp");
        if (xsize < 1 || ysize < 1)
            throw new IllegalArgumentException("size < 1");
        if (type < 0 || type > 1)
            throw new IllegalArgumentException("Type must be L_TOPHAT_BLACK or L_TOPHAT_WHITE");

        long nativePix = nativePixFastTophat(pixs.mNativePix, xsize, ysize, type);

        if (nativePix == 0)
            throw new RuntimeException("Failed to perform pixFastTophat on image");

        return new Pix(nativePix); 
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native long nativePixTophat(long nativePix, int hsize, int vsize, int type);

    private static native long nativePixFastTophat(long nativePix, int xsize, int ysize, int type);
}
