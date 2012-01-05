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

import com.googlecode.eyesfree.env.Size;
import com.googlecode.eyesfree.opticflow.TextTrackerProcessor.TrackedRect;

import java.util.LinkedList;
import java.util.Vector;

/**
 * Frame processor that queues OCR jobs.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class OcrProcessor extends FrameProcessor {
    /** Text area tracker. */
    private final TextTrackerProcessor mTracker;

    /** Queue containing text areas to be OCR'ed. */
    private final OcrQueue mOcrQueue;

    /** Callback for OCR completion events. */
    private Listener mListener;

    /**
     * Constructs a new OCR processor.
     *
     * @param tessdata The path containing the {@code tessdata} directory.
     * @param language The language pack to use. Defaults to "eng" if not
     *            available.
     * @param tracker A text area tracker.
     */
    public OcrProcessor(String tessdata, String language, TextTrackerProcessor tracker) {
        mTracker = tracker;
        mOcrQueue = new OcrQueue(tessdata, language);
        mOcrQueue.setListener(queueListener);
    }

    public void setListener(Listener listener) {
        mListener = listener;
    }

    @Override
    protected void onInit(Size size) {
        mOcrQueue.init();
    }

    @Override
    protected void onProcessFrame(TimestampedFrame frame) {
        LinkedList<TrackedRect> add = mTracker.getOcrAdd();
        LinkedList<TrackedRect> remove = mTracker.getOcrRemove();

        mOcrQueue.addAll(add);
        mOcrQueue.removeAll(remove);
    }

    @Override
    protected Vector<String> getDebugText() {
        Vector<String> debugText = new Vector<String>();
        debugText.add("Queued: " + mOcrQueue.size());
        return debugText;
    }

    public interface Listener {
        public void onResult(String result, int[] confs);
    }

    private final OcrQueue.Listener queueListener = new OcrQueue.Listener() {
        @Override
        public void onResult(String result, int[] confs) {
            if (mListener != null) {
                mListener.onResult(result, confs);
            }
        }
    };
}
