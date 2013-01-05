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

public class PageIterator {
    static {
        System.loadLibrary("lept");
        System.loadLibrary("tess");
    }

    /** Pointer to native page iterator. */
    private final int mNativePageIterator;

    /* package */PageIterator(int nativePageIterator) {
        mNativePageIterator = nativePageIterator;
    }

    /**
     * Resets the iterator to point to the start of the page.
     */
    public void begin() {
        nativeBegin(mNativePageIterator);
    }

    /**
     * Moves to the start of the next object at the given level in the page
     * hierarchy, and returns false if the end of the page was reached.
     * <p>
     * NOTE that {@link PageIteratorLevel#RIL_SYMBOL} will skip non-text blocks,
     * but all other {@link PageIteratorLevel} level values will visit each
     * non-text block once. Think of non text blocks as containing a single
     * para, with a single line, with a single imaginary word.
     * <p>
     * Calls to {@link #next} with different levels may be freely intermixed.
     * <p>
     * This function iterates words in right-to-left scripts correctly, if the
     * appropriate language has been loaded into Tesseract.
     *
     * @param level the page iterator level. See {@link PageIteratorLevel}.
     * @return {@code false} if the end of the page was reached, {@code true}
     *         otherwise.
     */
    public boolean next(int level) {
        return nativeNext(mNativePageIterator, level);
    }

    private static native void nativeBegin(int nativeIterator);
    private static native boolean nativeNext(int nativeIterator, int level);
}
