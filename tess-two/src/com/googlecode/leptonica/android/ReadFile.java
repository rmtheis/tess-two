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
import android.graphics.BitmapFactory;
import android.os.Build;

import java.io.File;

/**
 * Image input and output methods.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class ReadFile {
    static {
        System.loadLibrary("lept");
    }

    /**
     * Creates a 32bpp Pix object from encoded data. Supported formats are BMP
     * and JPEG.
     *
     * @param encodedData JPEG or BMP encoded byte data.
     * @return a 32bpp Pix object
     */
    public static Pix readMem(byte[] encodedData) {
        if (encodedData == null)
            throw new IllegalArgumentException("Image data byte array must be non-null");

        // TODO Fix support for JPEG library in Android 2.2 & lower
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.FROYO) {
            final int nativePix = nativeReadMem(encodedData, encodedData.length);

            if (nativePix == 0)
                throw new RuntimeException("Failed to read pix from memory");

            return new Pix(nativePix);
        } else {
            final BitmapFactory.Options opts = new BitmapFactory.Options();
            opts.inPreferredConfig = Bitmap.Config.ARGB_8888;

            final Bitmap bmp = BitmapFactory.decodeByteArray(encodedData, 0, encodedData.length,
                    opts);
            final Pix pix = readBitmap(bmp);

            bmp.recycle();

            return pix;
        }
    }

    /**
     * Creates an 8bpp Pix object from raw 8bpp grayscale pixels.
     *
     * @param pixelData 8bpp grayscale pixel data.
     * @param width The width of the input image.
     * @param height The height of the input image.
     * @return an 8bpp Pix object
     */
    public static Pix readBytes8(byte[] pixelData, int width, int height) {
        if (pixelData == null)
            throw new IllegalArgumentException("Byte array must be non-null");
        if (width <= 0)
            throw new IllegalArgumentException("Image width must be greater than 0");
        if (height <= 0)
            throw new IllegalArgumentException("Image height must be greater than 0");
        if (pixelData.length < width * height)
            throw new IllegalArgumentException("Array length does not match dimensions");

        int nativePix = nativeReadBytes8(pixelData, width, height);

        if (nativePix == 0)
            throw new RuntimeException("Failed to read pix from memory");

        return new Pix(nativePix);
    }

    /**
     * Replaces the bytes in an 8bpp Pix object with raw grayscale 8bpp pixels.
     * Width and height be identical to the input Pix.
     *
     * @param pixs The Pix whose bytes will be replaced.
     * @param pixelData 8bpp grayscale pixel data.
     * @param width The width of the input image.
     * @param height The height of the input image.
     * @return an 8bpp Pix object
     */
    public static boolean replaceBytes8(Pix pixs, byte[] pixelData, int width, int height) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");
        if (pixelData == null)
            throw new IllegalArgumentException("Byte array must be non-null");
        if (width <= 0)
            throw new IllegalArgumentException("Image width must be greater than 0");
        if (height <= 0)
            throw new IllegalArgumentException("Image height must be greater than 0");
        if (pixelData.length < width * height)
            throw new IllegalArgumentException("Array length does not match dimensions");
        if (pixs.getWidth() != width)
            throw new IllegalArgumentException("Source pix width does not match image width");
        if (pixs.getHeight() != height)
            throw new IllegalArgumentException("Source pix width does not match image width");

        return nativeReplaceBytes8(pixs.mNativePix, pixelData, width, height);
    }

    /**
     * Creates a Pixa object from encoded files in a directory. Supported
     * formats are BMP and JPEG.
     *
     * @param dir The directory containing the files.
     * @param prefix The prefix of the files to load into a Pixa.
     * @return a Pixa object containing one Pix for each file
     */
    public static Pixa readFiles(File dir, String prefix) {
        if (dir == null)
            throw new IllegalArgumentException("Directory must be non-null");
        if (!dir.exists())
            throw new IllegalArgumentException("Directory does not exist");
        if (!dir.canRead())
            throw new IllegalArgumentException("Cannot read directory");

        // TODO Fix support for JPEG library in Android 2.2 & lower
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.FROYO) {
            int nativePixa = nativeReadFiles(dir.getAbsolutePath(), prefix);

            if (nativePixa == 0)
                throw new RuntimeException("Failed to read pixs from files");

            // TODO Get bounding box from Pixa

            return new Pixa(nativePixa, -1, -1);
        } else {
            throw new RuntimeException("readFiles() is only available in SDK >= 10");
        }
    }

    /**
     * Creates a Pix object from encoded file data. Supported formats are BMP
     * and JPEG.
     *
     * @param file The JPEG or BMP-encoded file to read in as a Pix.
     * @return a Pix object
     */
    public static Pix readFile(File file) {
        if (file == null)
            throw new IllegalArgumentException("File must be non-null");
        if (!file.exists())
            throw new IllegalArgumentException("File does not exist");
        if (!file.canRead())
            throw new IllegalArgumentException("Cannot read file");

        // TODO Fix support for JPEG library in Android 2.2 & lower
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.FROYO) {
            final int nativePix = nativeReadFile(file.getAbsolutePath());

            if (nativePix == 0)
                throw new RuntimeException("Failed to read pix from file");

            return new Pix(nativePix);
        } else {
            final BitmapFactory.Options opts = new BitmapFactory.Options();
            opts.inPreferredConfig = Bitmap.Config.ARGB_8888;

            final Bitmap bmp = BitmapFactory.decodeFile(file.getAbsolutePath(), opts);
            final Pix pix = readBitmap(bmp);

            bmp.recycle();

            return pix;
        }
    }

    /**
     * Creates a Pix object from Bitmap data. Currently supports only
     * ARGB_8888-formatted bitmaps.
     *
     * @param bmp The Bitmap object to convert to a Pix.
     * @return a Pix object
     */
    public static Pix readBitmap(Bitmap bmp) {
        if (bmp == null)
            throw new IllegalArgumentException("Bitmap must be non-null");
        if (bmp.getConfig() != Bitmap.Config.ARGB_8888)
            throw new IllegalArgumentException("Bitmap config must be ARGB_8888");

        int nativePix = nativeReadBitmap(bmp);

        if (nativePix == 0)
            throw new RuntimeException("Failed to read pix from bitmap");

        return new Pix(nativePix);
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native int nativeReadMem(byte[] data, int size);

    private static native int nativeReadBytes8(byte[] data, int w, int h);

    private static native boolean nativeReplaceBytes8(int nativePix, byte[] data, int w, int h);

    private static native int nativeReadFiles(String dirname, String prefix);

    private static native int nativeReadFile(String filename);

    private static native int nativeReadBitmap(Bitmap bitmap);
}
