/*
 * Copyright 2015 Robert Theis
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

package com.googlecode.tesseract.android;

/**
 * Java representation of a native Tesseract PDF renderer
 */
public class TessPdfRenderer {

    /**
     * Used by the native implementation of the class.
     */
    private final long mNativePdfRenderer;

    static {
        System.loadLibrary("jpgt");
        System.loadLibrary("pngt");
        System.loadLibrary("lept");
        System.loadLibrary("tess");
    }

    private boolean mRecycled;

    /**
     * Constructs an instance of a Tesseract PDF renderer.
     * 
     * When the instance of TessPdfRenderer is no longer needed, its 
     * {@link #recycle} method must be invoked to dispose of it.
     * 
     * @param baseApi API instance to use for performing OCR 
     * @param outputPath Full path to write the resulting PDF to, not
     *         including the ".pdf" extension 
     */
    public TessPdfRenderer(TessBaseAPI baseApi, String outputPath) {        
        this.mNativePdfRenderer = nativeCreate(baseApi.getNativeData(), outputPath);
        mRecycled = false;
    }

    /**
     * @return A pointer to the native TessPdfRenderer object.
     */
    public long getNativePdfRenderer() {
        if (mRecycled)
            throw new IllegalStateException();

        return mNativePdfRenderer;
    }

    /**
     * Releases resources and frees any memory associated with this 
     * TessPdfRenderer object. Must be called on object destruction.
     */
    public void recycle() {
        nativeRecycle(mNativePdfRenderer);
        mRecycled = true;
    }

    private static native long nativeCreate(long tessBaseAPINativeData, String outputPath);

    private static native void nativeRecycle(long nativePointer);

}