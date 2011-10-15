// Copyright 2010 Google Inc. All Rights Reserved.

package com.googlecode.leptonica.android;

/**
 * @author alanv@google.com (Your Name Here)
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
     * @return the rotated source image
     */
    public static Pix rotate(Pix pixs, float degrees, boolean quality) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");

        int nativePix = nativeRotate(pixs.mNativePix, degrees, quality);

        if (nativePix == 0)
            return null;

        return new Pix(nativePix);
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native int nativeRotate(int nativePix, float degrees, boolean quality);
}
