/*
 * Copyright 2014 Robert Theis
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
 * Extract rectangular regions.
 */
public class Clip {
    static {
        System.loadLibrary("jpgt");
        System.loadLibrary("pngt");
        System.loadLibrary("lept");
    }

    /**
     * Extract a region from a Pix.
     * <p>
     * Notes:
     * <p>
     * This should be simple, but there are choices to be made. The box is
     * defined relative to the pix coordinates.  However, if the box is not
     * contained within the pix, we have two choices:
     * <p>     (1) clip the box to the pix
     * <p>     (2) make a new pix equal to the full box dimensions,
     *             but let rasterop do the clipping and positioning
     *             of the src with respect to the dest
     * <p>
     * Choice (2) immediately brings up the problem of what pixel values
     * to use that were not taken from the src.  For example, on a grayscale
     * image, do you want the pixels not taken from the src to be black
     * or white or something else?  To implement choice 2, one needs to
     * specify the color of these extra pixels.
     * <p>
     * So we adopt (1), and clip the box first, if necessary,
     * before making the dest pix and doing the rasterop.  But there
     * is another issue to consider.  If you want to paste the
     * clipped pix back into pixs, it must be properly aligned, and
     * it is necessary to use the clipped box for alignment.
     *
     * @param source Source pix
     * @param box Requested clipping region
     * @return clipped pix, or null if rectangle doesn't intersect source pix
     */
    public static Pix clipRectangle(Pix source, Box box) {
        long result = nativeClipRectangle(source.getNativePix(),
                box.getNativeBox());
        if (result != 0) {
            return new Pix(result);
        }
        return null;
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native long nativeClipRectangle(long nativePix, long nativeBox);
}