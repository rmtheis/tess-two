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

package com.googlecode.eyesfree.ocr.client;

import android.graphics.Rect;
import android.os.Parcel;
import android.os.Parcelable;

/**
 * This class represents the result of text recognition. It includes the text
 * itself as well as the word confidence values.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class OcrResult implements Parcelable {
    /** The text area boundary. */
    private final Rect mBounds;

    /** The character contents of this text area. */
    private final String mString;

    /** The word confidences of this text area. */
    private final int[] mConfidences;

    private final float mAngle;

    /**
     * The average word confidence of this text area. Initially -1 and lazily
     * generated when getAverageConfidence() is called.
     */
    private float mAvgConfidence;

    /**
     * Creates an OCR result.
     *
     * @param result The result of text recognition.
     * @param confidences The array of word confidences.
     */
    public OcrResult(Rect bounds, String result, int[] confidences, float angle) {
        mBounds = bounds;
        mString = result;
        mConfidences = confidences;
        mAngle = angle;
        mAvgConfidence = -1;
    }

    /**
     * Returns the bounding box containing the text result as a Rect.
     *
     * @return The bounding box containing the text result.
     */
    public Rect getBounds() {
        return mBounds;
    }

    /**
     * Returns the result of text recognition as a String.
     *
     * @return The result of text recognition.
     */
    public String getString() {
        return mString;
    }

    /**
     * Returns the array of word confidences. Each entry corresponds to a single
     * space-delimited word in the result string.
     *
     * @return The array of word confidences.
     */
    public int[] getConfidences() {
        return mConfidences;
    }

    /**
     * Returns the angle of this text. Rotation is relative to the center of the
     * source image.
     *
     * @return The source image center-relative angle of this text.
     */
    public float getAngle() {
        return mAngle;
    }

    /**
     * Returns the average confidence of the words in this result.
     *
     * @return The average confidence of the words in this result.
     */
    public float getAverageConfidence() {
        if (mAvgConfidence >= 0) {
            return mAvgConfidence;
        } else if (mConfidences == null) {
            return 0.0f;
        }

        mAvgConfidence = 0.0f;

        for (int i = 0; i < mConfidences.length; i++) {
            mAvgConfidence += mConfidences[i];
        }

        mAvgConfidence /= mConfidences.length;

        return mAvgConfidence;
    }

    /************************
     * Parcelable functions *
     ************************/

    private OcrResult(Parcel src) {
        mString = src.readString();
        mConfidences = src.createIntArray();
        mBounds = src.readParcelable(null);
        mAngle = src.readFloat();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(mString);
        dest.writeIntArray(mConfidences);
        dest.writeParcelable(mBounds, Parcelable.PARCELABLE_WRITE_RETURN_VALUE);
        dest.writeFloat(mAngle);
    }

    public static final Parcelable.Creator<OcrResult> CREATOR = new Parcelable.Creator<OcrResult>() {
        @Override
        public OcrResult createFromParcel(Parcel in) {
            return new OcrResult(in);
        }

        @Override
        public OcrResult[] newArray(int size) {
            return new OcrResult[size];
        }
    };

}
