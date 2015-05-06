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
 * Image bit-depth conversion methods.
 * 
 * @author alanv@google.com (Alan Viverette)
 */
public class Convert {
    static {
        System.loadLibrary("lept");
    }
    
    /**
     * Converts an image of any bit depth to 8-bit grayscale.
     *
     * @param pixs Source pix of any bit-depth.
     * @return a new Pix image or <code>null</code> on error
     */
    public static Pix convertTo8(Pix pixs) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");

        long nativePix = nativeConvertTo8(pixs.mNativePix);

        if (nativePix == 0)
            throw new RuntimeException("Failed to natively convert pix");

        return new Pix(nativePix);
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native long nativeConvertTo8(long nativePix);
}
