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

public class GrayQuant {
    static {
        System.loadLibrary("lept");
    }

    /**
     * Perform simple (pixelwise) binarization with fixed threshold
     * <p>
     * Notes:
     * <ol>
     * <li> If the source pixel is less than the threshold value, the dest will 
     * be 1; otherwise, it will be 0
     * </ol>
     *
     * @param pixs Source pix (4 or 8 bpp)
     * @param thresh Threshold value
     * @return a new Pix image, 1 bpp
     */
    public static Pix pixThresholdToBinary(Pix pixs, int thresh) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");
        int depth = pixs.getDepth();
        if (depth != 4 && depth != 8)
            throw new IllegalArgumentException("Source pix depth must be 4 or 8 bpp");
        if (depth == 4 && thresh > 16)
            throw new IllegalArgumentException("4 bpp thresh not in {0-16}");
        if (depth == 8 && thresh > 256)
            throw new IllegalArgumentException("8 bpp thresh not in {0-256}");

        long nativePix = nativePixThresholdToBinary(pixs.mNativePix, thresh);

        if (nativePix == 0)
            throw new RuntimeException("Failed to perform binarization");

        return new Pix(nativePix);         
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native long nativePixThresholdToBinary(long nativePix, int thresh);
}
