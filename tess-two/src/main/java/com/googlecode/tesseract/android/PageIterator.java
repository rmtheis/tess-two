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
    private final long mNativePageIterator;

    /* package */PageIterator(long nativePageIterator) {
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

    /**
     * Get bounding box: x, y, w, h
     * 
     * ============= Accessing data ==============.
     * Coordinate system:
     * Integer coordinates are at the cracks between the pixels.
     * The top-left corner of the top-left pixel in the image is at (0,0).
     * The bottom-right corner of the bottom-right pixel in the image is at
     * (width, height).
     * Every bounding box goes from the top-left of the top-left contained
     * pixel to the bottom-right of the bottom-right contained pixel, so
     * the bounding box of the single top-left pixel in the image is:
     * (0,0)->(1,1).
     * If an image rectangle has been set in the API, then returned coordinates
     * relate to the original (full) image, rather than the rectangle.
     *
     * Returns the bounding rectangle of the current object at the given level.
     * See comment on coordinate system above.
     * The returned bounding box may clip foreground pixels from a grey image.
     * 
     * @param level the page iterator level. See {@link PageIteratorLevel}.
     * @return the bounding rectangle of the current object at the given level
     */
    public int[] getBoundingBox(int level){
    	return nativeBoundingBox(mNativePageIterator, level);
    }
    
    private static native void nativeBegin(long nativeIterator);
    private static native boolean nativeNext(long nativeIterator, int level);
    private static native int[] nativeBoundingBox(long nativeIterator, int level);
}
