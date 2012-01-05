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

package com.googlecode.eyesfree.opticflow;

/**
 * Wrapper for native image blur detection code. Modified by Alan Viverette from
 * Xiaotao Duan's original source.
 *
 * @author Xiaotao Duan
 * @author alanv@google.com (Alan Viverette)
 */
public class ImageBlur {

    /**
     * Tests if a given image is blurred or not.
     *
     * @param input An array of input pixels in YUV420SP format.
     * @param width The width of the input image.
     * @param height The height of the input image.
     * @return true when input image is blurred.
     */
    public static native boolean isBlurred(byte[] input, int width, int height);

    /**
     * Computes signature of a given image.
     *
     * @param input An array of input pixels in YUV420SP format.
     * @param width The width of the input image.
     * @param height The height of the input image.
     * @param signatureBuffer A buffer for output signature. If it's null or not
     *            in the right size, this buffer will be ignored and not used.
     *            This is used to avoid GC.
     * @return Signature of input image. If signatureBuffer is valid,
     *         signatureBuffer will be returned. Otherwise a new array will be
     *         returned and can be used as signature buffer in next function
     *         call.
     */
    public static native int[] computeSignature(
            byte[] input, int width, int height, int[] signatureBuffer);

    /**
     * Computes how similar of two given images represented by their signatures.
     *
     * @return An integer from 0 to 100 is returned indicating how much
     *         percentage of signature2 is different from signature1.
     */
    public static native int diffSignature(int[] signature1, int[] signature2);

    static {
        System.loadLibrary("imageutils");
    }
}
