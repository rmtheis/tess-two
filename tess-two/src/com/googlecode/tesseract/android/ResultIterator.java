/*
 * Copyright (C) 2012 Google Inc.
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

import com.googlecode.tesseract.android.TessBaseAPI.PageIteratorLevel;

/**
 * Java interface for the ResultIterator. Does not implement all available JNI
 * methods, but does implement enough to be useful. Comments are adapted from
 * original Tesseract source.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class ResultIterator extends PageIterator {
    static {
        System.loadLibrary("lept");
        System.loadLibrary("tess");
    }

    /** Pointer to native result iterator. */
    private final int mNativeResultIterator;

    /* package */ResultIterator(int nativeResultIterator) {
        super(nativeResultIterator);

        mNativeResultIterator = nativeResultIterator;
    }

    /**
     * Returns the text string for the current object at the given level.
     *
     * @param level the page iterator level. See {@link PageIteratorLevel}.
     * @return the text string for the current object at the given level.
     */
    public String getUTF8Text(int level) {
        return nativeGetUTF8Text(mNativeResultIterator, level);
    }

    /**
     * Returns the mean confidence of the current object at the given level. The
     * number should be interpreted as a percent probability (0-100).
     *
     * @param level the page iterator level. See {@link PageIteratorLevel}.
     * @return the mean confidence of the current object at the given level.
     */
    public float confidence(int level) {
        return nativeConfidence(mNativeResultIterator, level);
    }

    private static native String nativeGetUTF8Text(int nativeResultIterator, int level);
    private static native float nativeConfidence(int nativeResultIterator, int level);
}
