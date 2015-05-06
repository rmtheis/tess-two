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

import android.os.AsyncTask;
import android.util.Log;

import com.googlecode.eyesfree.opticflow.TextTrackerProcessor.TrackedRect;
import com.googlecode.tesseract.android.TessBaseAPI;

import java.util.Collection;
import java.util.LinkedList;

/**
 * Runs OCR jobs on an availability basis.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class OcrQueue {
    private static final String TAG = "OcrQueue";
    private static final String DEFAULT_WHITELIST = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789/:=.@,!-'%()$&?*";

    private final TessBaseAPI mOcrAPI;
    private final LinkedList<TrackedRect> mRectQueue;
    private final String mTessdata;
    private final String mLanguage;

    private Listener mListener;

    /** Whether init() has been called yet. */
    private boolean mInitialized;

    /**
     * Constructs a new recognition queue.
     *
     * @param tessdata The path containing the {@code tessdata} directory.
     * @param language The language pack to use. Defaults to "eng" if not
     *            available.
     */
    public OcrQueue(String tessdata, String language) {
        mTessdata = tessdata;
        mLanguage = language;
        mRectQueue = new LinkedList<TrackedRect>();
        mOcrAPI = new TessBaseAPI();
    }

    /**
     * Sets a listener to receive recognition results.
     *
     * @param listener The listener that will receive recognition results.
     */
    public void setListener(Listener listener) {
        mListener = listener;
    }

    /**
     * Initializes the OCR API for the specified language pack.
     */
    public void init() {
        boolean success = mOcrAPI.init(mTessdata, mLanguage);

        if (success) {
            mOcrAPI.setPageSegMode(TessBaseAPI.PageSegMode.PSM_SINGLE_LINE);
            mOcrAPI.setVariable(TessBaseAPI.VAR_CHAR_WHITELIST, DEFAULT_WHITELIST);

            mInitialized = true;
        }
    }

    /**
     * Adds a collection of tracked rects to the queue.
     *
     * @param rects The collection of tracked rects to queue for recognition.
     */
    public void addAll(Collection<? extends TrackedRect> rects) {
        boolean initialized = false;
        boolean wasEmpty = false;

        synchronized (mRectQueue) {
            initialized = mInitialized;
            wasEmpty = mRectQueue.isEmpty();
            mRectQueue.addAll(rects);
        }

        if (initialized && wasEmpty) {
            next();
        }
    }

    /**
     * Adds a tracked rect to the queue.
     *
     * @param rect The tracked rect to queue for recognition.
     */
    public void add(TrackedRect rect) {
        boolean initialized = false;
        boolean wasEmpty = false;

        synchronized (mRectQueue) {
            initialized = mInitialized;
            wasEmpty = mRectQueue.isEmpty();
            mRectQueue.addLast(rect);
        }

        if (initialized && wasEmpty) {
            next();
        }
    }

    /**
     * Removes a collection of tracked rects from the queue.
     *
     * @param rects The collection of tracked rects to remove from the recognition queue.
     */
    public void removeAll(Collection<? extends TrackedRect> rects) {
        synchronized (mRectQueue) {
            mRectQueue.removeAll(rects);
        }
    }

    /**
     * Adds a tracked rect to the queue.
     *
     * @param rect The tracked rect to remove from the recognition queue.
     */
    public void remove(TrackedRect rect) {
        synchronized (mRectQueue) {
            mRectQueue.remove(rect);
        }
    }

    /**
     * Runs the next queue item, if one is available.
     */
    private void next() {
        TrackedRect rect = null;
        boolean initialized = false;

        synchronized (mRectQueue) {
            initialized = mInitialized;

            if (!mRectQueue.isEmpty()) {
                rect = mRectQueue.getFirst();
            }
        }

        if (initialized && rect != null) {
            RecognizeTask task = new RecognizeTask();
            task.execute(rect);
        }
    }

    /**
     * @return the size of the recognition queue
     */
    public int size() {
        int size = 0;

        synchronized (mRectQueue) {
            size = mRectQueue.size();
        }

        return size;
    }

    private class RecognizeTask extends AsyncTask<TrackedRect, Void, RecognitionResult> {
        @Override
        protected RecognitionResult doInBackground(TrackedRect... rects) {
            if (rects.length <= 0 || isCancelled()) {
                return null;
            }

            TrackedRect rect = rects[0];

            Log.i(TAG, "Recognizing");

            mOcrAPI.setImage(rect.pix);
            String utf8 = mOcrAPI.getUTF8Text();
            int[] confs = mOcrAPI.wordConfidences();

            rect.text = utf8;

            return new RecognitionResult(utf8, confs);
        }

        @Override
        protected void onCancelled() {
            // TODO(alanv): Cancel current native recognition task.
        }

        @Override
        protected void onPostExecute(RecognitionResult result) {
            Log.i(TAG, "Recognized " + result.utf8);

            synchronized (mRectQueue) {
                if (!mRectQueue.isEmpty()) {
                    mRectQueue.removeFirst();
                    next();
                }
            }

            if (mListener != null) {
                mListener.onResult(result.utf8, result.confs);
            }
        }
    }

    public interface Listener {
        public void onResult(String result, int[] confs);
    }

    private class RecognitionResult {
        public final String utf8;
        public final int[] confs;

        public RecognitionResult(String utf8, int[] confs) {
            this.utf8 = utf8;
            this.confs = confs;
        }
    }
}
