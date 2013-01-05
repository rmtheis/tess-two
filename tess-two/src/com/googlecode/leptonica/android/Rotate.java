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

package com.googlecode.leptonica.android;

/**
 * @author alanv@google.com (Alan Viverette)
 */
public class Rotate {
    static {
        System.loadLibrary("lept");
    }

    // Rotation default

    /** Default rotation quality is high. */
    public static final boolean ROTATE_QUALITY = true;

    /**
     * Performs rotation using the default parameters.
     *
     * @param pixs The source pix.
     * @param degrees The number of degrees to rotate; clockwise is positive.
     * @return the rotated source image
     */
    public static Pix rotate(Pix pixs, float degrees) {
        return rotate(pixs, degrees, false);
    }

    /**
     * Performs rotation with resizing using the default parameters.
     *
     * @param pixs The source pix.
     * @param degrees The number of degrees to rotate; clockwise is positive.
     * @param quality Whether to use high-quality rotation.
     * @return the rotated source image
     */
    public static Pix rotate(Pix pixs, float degrees, boolean quality) {
        return rotate(pixs, degrees, quality, true);
    }
    
    /**
     * Performs basic image rotation about the center.
     * <p>
     * Notes:
     * <ol>
     * <li>Rotation is about the center of the image.
     * <li>For very small rotations, just return a clone.
     * <li>Rotation brings either white or black pixels in from outside the
     * image.
     * <li>Above 20 degrees, if rotation by shear is requested, we rotate by
     * sampling.
     * <li>Colormaps are removed for rotation by area map and shear.
     * <li>The dest can be expanded so that no image pixels are lost. To invoke
     * expansion, input the original width and height. For repeated rotation,
     * use of the original width and height allows the expansion to stop at the
     * maximum required size, which is a square with side = sqrt(w*w + h*h).
     * </ol>
     *
     * @param pixs The source pix.
     * @param degrees The number of degrees to rotate; clockwise is positive.
     * @param quality Whether to use high-quality rotation.
     * @param Whether to expand the output so that no pixels are lost.
     *         <strong>Note:</strong> 1bpp images are always resized when
     *         quality is {@code true}.
     * @return the rotated source image
     */
    public static Pix rotate(Pix pixs, float degrees, boolean quality, boolean resize) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");

        int nativePix = nativeRotate(pixs.mNativePix, degrees, quality, resize);

        if (nativePix == 0)
            return null;

        return new Pix(nativePix);
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native int nativeRotate(int nativePix, float degrees, boolean quality,
    		boolean resize);
}
