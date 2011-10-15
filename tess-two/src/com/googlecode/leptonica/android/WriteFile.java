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

import android.graphics.Bitmap;
import android.os.Build;

import java.io.File;

/**
 * @author alanv@google.com (Alan Viverette)
 */
public class WriteFile {
    static {
        System.loadLibrary("lept");
    }

    /* Default JPEG quality */
    public static final int DEFAULT_QUALITY = 85;

    /* Default JPEG progressive encoding */
    public static final boolean DEFAULT_PROGRESSIVE = true;

    /**
     * Write an 8bpp Pix to a flat byte array.
     *
     * @param pixs The 8bpp source image.
     * @return a byte array where each byte represents a single 8-bit pixel
     */
    public static byte[] writeBytes8(Pix pixs) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");

        int size = pixs.getWidth() * pixs.getHeight();

        if (pixs.getDepth() != 8) {
            Pix pix8 = Convert.convertTo8(pixs);
            pixs.recycle();
            pixs = pix8;
        }

        byte[] data = new byte[size];

        writeBytes8(pixs, data);

        return data;
    }

    /**
     * Write an 8bpp Pix to a flat byte array.
     *
     * @param pixs The 8bpp source image.
     * @param data A byte array large enough to hold the pixels of pixs.
     * @return the number of bytes written to data
     */
    public static int writeBytes8(Pix pixs, byte[] data) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");

        int size = pixs.getWidth() * pixs.getHeight();

        if (data.length < size)
            throw new IllegalArgumentException("Data array must be large enough to hold image bytes");

        int bytesWritten = nativeWriteBytes8(pixs.mNativePix, data);

        return bytesWritten;
    }

    /**
     * Writes all the images in a Pixa array to individual files using the
     * specified format. The output file extension will be determined by the
     * format.
     * <p>
     * Output file names will take the format <path>/<prefix><index>.<extension>
     *
     * @param pixas The source Pixa image array.
     * @param path The output directory.
     * @param prefix The prefix to give output files.
     * @param format The format to use for output files.
     * @return <code>true</code> on success
     */
    public static boolean writeFiles(Pixa pixas, File path, String prefix, int format) {
        if (pixas == null)
            throw new IllegalArgumentException("Source pixa must be non-null");
        if (path == null)
            throw new IllegalArgumentException("Destination path non-null");
        if (prefix == null)
            throw new IllegalArgumentException("Filename prefix must be non-null");

        String rootname = new File(path, prefix).getAbsolutePath();

        // TODO Fix support for JPEG library in Android 2.2 & lower
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.FROYO) {
            return nativeWriteFiles(pixas.mNativePixa, rootname, format);
        } else {
            throw new RuntimeException("writeFiles() is only available in SDK >= 10");
        }
    }

    /**
     * Write a Pix to a byte array using the specified encoding from
     * Constants.IFF_*.
     *
     * @param pixs The source image.
     * @param format A format from Constants.IFF_*.
     * @return a byte array containing encoded bytes
     */
    public static byte[] writeMem(Pix pixs, int format) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");

        return nativeWriteMem(pixs.mNativePix, format);
    }

    /**
     * Writes a Pix to file using the file extension as the output format;
     * supported formats are .jpg or .jpeg for JPEG and .bmp for bitmap.
     * <p>
     * Uses default quality and progressive encoding settings.
     *
     * @param pixs Source image.
     * @param file The file to write.
     * @return <code>true</code> on success
     */
    public static boolean writeImpliedFormat(Pix pixs, File file) {
        return writeImpliedFormat(pixs, file, DEFAULT_QUALITY, DEFAULT_PROGRESSIVE);
    }

    /**
     * Writes a Pix to file using the file extension as the output format;
     * supported formats are .jpg or .jpeg for JPEG and .bmp for bitmap.
     * <p>
     * Notes:
     * <ol>
     * <li>This determines the output format from the filename extension.
     * <li>The last two args are ignored except for requests for jpeg files.
     * <li>The jpeg default quality is 75.
     * </ol>
     *
     * @param pixs Source image.
     * @param file The file to write.
     * @param quality (Only for lossy formats) Quality between 1 - 100, 0 for
     *            default.
     * @param progressive (Only for JPEG) Whether to encode as progressive.
     * @return <code>true</code> on success
     */
    public static boolean writeImpliedFormat(
            Pix pixs, File file, int quality, boolean progressive) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");
        if (file == null)
            throw new IllegalArgumentException("File must be non-null");

        return nativeWriteImpliedFormat(
                pixs.mNativePix, file.getAbsolutePath(), quality, progressive);
    }

    /**
     * Writes a Pix to an Android Bitmap object. The output Bitmap will always
     * be in ARGB_8888 format, but the input Pixs may be any bit-depth.
     *
     * @param pixs The source image.
     * @return a Bitmap containing a copy of the source image, or <code>null
     *         </code> on failure
     */
    public static Bitmap writeBitmap(Pix pixs) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");

        int[] dimensions = pixs.getDimensions();
        int width = dimensions[Pix.INDEX_W];
        int height = dimensions[Pix.INDEX_H];

        Bitmap.Config config = Bitmap.Config.ARGB_8888;
        Bitmap bitmap = Bitmap.createBitmap(width, height, config);

        if (nativeWriteBitmap(pixs.mNativePix, bitmap)) {
            return bitmap;
        }

        bitmap.recycle();

        return null;
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native int nativeWriteBytes8(int nativePix, byte[] data);

    private static native boolean nativeWriteFiles(int nativePix, String rootname, int format);

    private static native byte[] nativeWriteMem(int nativePix, int format);

    private static native boolean nativeWriteImpliedFormat(
            int nativePix, String fileName, int quality, boolean progressive);

    private static native boolean nativeWriteBitmap(int nativePix, Bitmap bitmap);
}
