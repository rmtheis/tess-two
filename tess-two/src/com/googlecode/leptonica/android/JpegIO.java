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
import android.graphics.Bitmap.CompressFormat;
import android.os.Build;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

/**
 * JPEG input and output methods.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class JpegIO {
    static {
        System.loadLibrary("lept");
    }

    /** Default quality is 85%, which is reasonably good. */
    public static final int DEFAULT_QUALITY = 85;

    /** Progressive encoding is disabled by default to increase compatibility. */
    public static final boolean DEFAULT_PROGRESSIVE = false;

    /**
     * Returns a compressed JPEG byte representation of this Pix using default
     * parameters.
     *
     * @param pixs
     * @return a compressed JPEG byte array representation of the Pix
     */
    public static byte[] compressToJpeg(Pix pixs) {
        return compressToJpeg(pixs, DEFAULT_QUALITY, DEFAULT_PROGRESSIVE);
    }

    /**
     * Returns a compressed JPEG byte representation of this Pix.
     *
     * @param pixs A source pix image.
     * @param quality The quality of the compressed image. Valid range is 0-100.
     * @param progressive Whether to use progressive compression.
     * @return a compressed JPEG byte array representation of the Pix
     */
    public static byte[] compressToJpeg(Pix pixs, int quality, boolean progressive) {
        if (pixs == null)
            throw new IllegalArgumentException("Source pix must be non-null");
        if (quality < 0 || quality > 100)
            throw new IllegalArgumentException("Quality must be between 0 and 100 (inclusive)");

        // TODO Fix support for JPEG library in Android 2.2 & lower
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.FROYO) {
            return nativeCompressToJpeg(pixs.mNativePix, quality, progressive);
        } else {
            final ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
            final Bitmap bmp = WriteFile.writeBitmap(pixs);
            bmp.compress(CompressFormat.JPEG, quality, byteStream);
            bmp.recycle();

            final byte[] encodedData = byteStream.toByteArray();

            try {
                byteStream.close();
            } catch (IOException e) {
                e.printStackTrace();
            }

            return encodedData;
        }
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native byte[] nativeCompressToJpeg(
            int nativePix, int quality, boolean progressive);
}
