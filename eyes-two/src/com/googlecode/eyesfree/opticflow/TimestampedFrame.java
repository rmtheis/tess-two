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

import android.util.Log;

import com.googlecode.eyesfree.env.Size;
import com.googlecode.eyesfree.opticflow.FrameProducer.Frame;
import com.googlecode.leptonica.android.Pix;
import com.googlecode.leptonica.android.Pixa;
import com.googlecode.leptonica.android.ReadFile;

/**
 * Class for interfacing efficiently with image data, and keeping track of all
 * records associated with a frame.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class TimestampedFrame {
    private static final String TAG = "TimestampedFrame";

    private int threadsLeft;

    // For caching PIX created from rawFrameData.
    private Pix cachedPix;

    // Whether this frame is thought to be blurred. May be null.
    private Boolean isBlurred;

    // Whether this frame was created while focus was occurring. May be null.
    private Boolean takenWhileFocusing;

    // The results of text detection.
    private Pixa detectedText;

    // The FDR value of detected text areas.
    private float[] textConfidences;

    // The estimated angle for text present in this frame.
    // TODO(alanv): Make this a per-Pix setting?
    private float angle;

    private final Frame originalFrame;

    // TODO(andrewharp): Create pool of TimestampedFrames so that used frames
    // can be recycled.
    protected TimestampedFrame(final Frame originalFrame) {
        this.originalFrame = originalFrame;
    }

    public long getTimestamp() {
        return originalFrame.timestamp;
    }

    public Size getSize() {
        return new Size(originalFrame.width, originalFrame.height);
    }

    public int getWidth() {
        return originalFrame.width;
    }

    public int getHeight() {
        return originalFrame.height;
    }

    public void setDetectedText(Pixa detectedText, float[] textAreaQuality, float angle) {
        if (detectedText == null) {
            throw new IllegalArgumentException("Detected text must be non-null");
        }

        this.detectedText = detectedText.copy();
        this.textConfidences = textAreaQuality;
        this.angle = angle;
    }

    public Pixa getDetectedText() {
        if (detectedText != null) {
            return detectedText.copy();
        } else {
            return null;
        }
    }

    public float[] getTextConfidences() {
        return textConfidences.clone();
    }

    public float getAngle() {
        return angle;
    }

    public void recycleDetectedText() {
        if (detectedText != null) {
            detectedText.recycle();
            detectedText = null;
        }
    }

    /**
     * @return A Pix containing the data for a PIX representation of this frame.
     */
    public synchronized Pix getPixData() {
        if (cachedPix == null) {
            cachedPix = ReadFile.readBytes8(
                    originalFrame.data, originalFrame.width, originalFrame.height);
        }

        return cachedPix.clone();
    }

    /**
     * @return Whether or not rawFrameData is null.
     */
    protected synchronized boolean hasRawData() {
        return originalFrame.data != null;
    }

    /**
     * @return The raw frame data. Raw data may have already been released, in
     *         which case an error is logged.
     */
    public synchronized byte[] getRawData() {
        if (!hasRawData()) {
            Log.e(TAG, "Frame data for frame is no longer available.");
        }
        return originalFrame.data;
    }

    /**
     * Raw frame data is expensive to keep around, so we need to provide a way
     * to remove it from this frame after the smaller JPEG is created.
     *
     * @return The byte[] this frame was holding.
     */
    protected synchronized byte[] clearRawData() {
        final byte[] tmpData = getRawData(); // So we get the implicit check.
        originalFrame.recycle();
        // Unblock any threads that are wait()ing in releaesBitmap(true).
        notify();
        return tmpData;
    }

    public boolean isBlurred() {
        if (isBlurred == null) {
            Log.w(TAG, "isBlurred() called without value having been set!");

            // If focusing we can assume it's blurred, otherwise default to
            // unblurred.
            return takenWhileFocusing();
        }

        return isBlurred;
    }

    public void setBlurred(final boolean blurred) {
        if (isBlurred != null) {
            Log.w(TAG, "Blurred already set!");
        }

        isBlurred = blurred;
    }

    public void setTakenWhileFocusing(final boolean takenWhileFocusing) {
        this.takenWhileFocusing = takenWhileFocusing;
    }

    public boolean takenWhileFocusing() {
        if (takenWhileFocusing == null) {
            return false;
        }
        return takenWhileFocusing.booleanValue();
    }

    /**
     * Used by a ProcessingThread to signify that it's done processing this
     * frame.
     */
    public void threadDone() {
        --threadsLeft;
        if (threadsLeft < 0) {
            Log.w(TAG, "Negative number of threads remaining.");
        }
    }

    /**
     * @return true iff all threads have finished processing this frame.
     */
    public boolean allThreadsDone() {
        return threadsLeft == 0;
    }

    public void threadStart() {
        ++threadsLeft;
    }
}
