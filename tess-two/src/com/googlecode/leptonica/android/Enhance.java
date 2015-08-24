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
 * Image sharpening methods.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class Enhance {
    static {
        System.loadLibrary("pngt");
        System.loadLibrary("lept");
    }

    // Unsharp masking constants
    
    public final static int DEFAULT_UNSHARP_HALFWIDTH = 1;
    
    public final static float DEFAULT_UNSHARP_FRACTION = 0.3f;
    
    /**
     * Performs unsharp masking (edge enhancement) using default values.
     * 
     * @see #unsharpMasking(Pix, int, float)
     * 
     * @param pixs Source image
     * @return an edge-enhanced Pix image or copy if no enhancement requested
     */
    public static Pix unsharpMasking(Pix pixs) {
        return unsharpMasking(pixs, DEFAULT_UNSHARP_HALFWIDTH, 
                DEFAULT_UNSHARP_FRACTION);
    }
    
    /**
     * Performs unsharp masking (edge enhancement).
     * <p>
     * Notes:
     * <ul>
     * <li>We use symmetric smoothing filters of odd dimension, typically use
     * sizes of 3, 5, 7, etc. The <code>halfwidth</code> parameter for these is
     * (size - 1)/2; i.e., 1, 2, 3, etc.</li>
     * <li>The <code>fract</code> parameter is typically taken in the range: 0.2
     * &lt; <code>fract</code> &lt; 0.7</li>
     * </ul>
     *
     * @param halfwidth The half-width of the smoothing filter.
     * @param fraction The fraction of edge to be added back into the source
     *            image.
     * @return an edge-enhanced Pix image or copy if no enhancement requested
     */
    public static Pix unsharpMasking(Pix pixs, int halfwidth, float fraction) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");

        long nativePix = nativeUnsharpMasking(pixs.getNativePix(), halfwidth, 
                fraction);

        if (nativePix == 0) {
            throw new OutOfMemoryError();
        }

        return new Pix(nativePix);
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native long nativeUnsharpMasking(long nativePix, int halfwidth, float fract);
}
