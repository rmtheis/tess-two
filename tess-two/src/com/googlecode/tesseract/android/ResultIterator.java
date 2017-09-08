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

import java.util.ArrayList;
import java.util.List;

import android.util.Log;
import android.util.Pair;

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
        System.loadLibrary("jpgt");
        System.loadLibrary("pngt");
        System.loadLibrary("lept");
        System.loadLibrary("tess");
    }

    /** Pointer to native result iterator. */
    private final long mNativeResultIterator;

    /* package */ResultIterator(long nativeResultIterator) {
        super(nativeResultIterator);

        mNativeResultIterator = nativeResultIterator;
    }

    /**
     * Returns the text string for the current object at the given level.
     *
     * @param level the page iterator level. See {@link PageIteratorLevel}.
     * @return the text string for the current object at the given level.
     */
    public String getUTF8Text(@PageIteratorLevel.Level int level) {
        return nativeGetUTF8Text(mNativeResultIterator, level);
    }

    /**
     * Returns the mean confidence of the current object at the given level. The
     * number should be interpreted as a percent probability (0-100).
     *
     * @param level the page iterator level. See {@link PageIteratorLevel}.
     * @return the mean confidence of the current object at the given level.
     */
    public float confidence(@PageIteratorLevel.Level int level) {
        return nativeConfidence(mNativeResultIterator, level);
    }

    /**
     * Returns true if the iterator is at the start of an object at the given
     * level. Possible uses include determining if a call to Next(RIL_WORD)
     * moved to the start of a RIL_PARA.
     *
     * @param level the page iterator level. See {@link PageIteratorLevel}.
     * @return {@code true} if iterator points to the start of an object at the given level.
     */
    public boolean isAtBeginningOf(@PageIteratorLevel.Level int level) {
        return nativeIsAtBeginningOf(mNativeResultIterator, level);
    }

    /**
     * Returns whether the iterator is positioned at the last element in a
     * given level. (e.g. the last word in a line, the last line in a block)
     *
     * @param level the page iterator level. See {@link PageIteratorLevel}.
     * @param element the page iterator level. See {@link PageIteratorLevel}.
     * @return {@code true} if iterator points to the last element in a given level.
     */
    public boolean isAtFinalElement(@PageIteratorLevel.Level int level,
                                    @PageIteratorLevel.Level int element) {
        return nativeIsAtFinalElement(mNativeResultIterator, level, element);
    }

    /**
     * Returns all possible matching text strings and their confidence level 
     * for the current object.
     * <p>
     * The default matching text is blank (""). 
     * The default confidence level is zero (0.0) 
     *
     * @return A list of pairs with the UTF symbol and the confidence
     */
    public List<Pair<String, Double>> getSymbolChoicesAndConfidence() {
        // Get the native choices
        String[] nativeChoices = nativeGetSymbolChoices(mNativeResultIterator);

        // Create the output list
        ArrayList<Pair<String, Double>> pairedResults = new ArrayList<Pair<String, Double>>();

        for (String nativeChoice : nativeChoices) {
            // The string and the confidence level are separated by a '|'
            int separatorPosition = nativeChoice.lastIndexOf('|');

            // Create a pair with the choices
            String utfString;
            Double confidenceLevel = (double) 0;
            if (separatorPosition > 0) {

                // If the string contains a '|' separate the UTF string and the confidence level
                utfString = nativeChoice.substring(0, separatorPosition);
                try {
                    confidenceLevel = Double.parseDouble(nativeChoice.substring(separatorPosition + 1));
                } catch (NumberFormatException e) {
                    Log.e("ResultIterator","Invalid confidence level for " + nativeChoice);
                }
            } else {
                // If the string contains no '|' then save the full native result as the utfString
                utfString = nativeChoice;
            }

            // Add the UTF string to the results
            pairedResults.add(new Pair<String, Double> (utfString, confidenceLevel));
        }

        return pairedResults;
    }

    /**
     * Deletes the iterator after use
     */
    public void delete() {
        nativeDelete(mNativeResultIterator);
    }
    
    private static native String[] nativeGetSymbolChoices(long nativeResultIterator);

    private static native String nativeGetUTF8Text(long nativeResultIterator, int level);
    private static native float nativeConfidence(long nativeResultIterator, int level);
    private static native boolean nativeIsAtBeginningOf(long nativeResultIterator, int level);
    private static native boolean nativeIsAtFinalElement(long nativeResultIterator, int level, int element);
    private static native void nativeDelete(long nativeResultIterator);
}
