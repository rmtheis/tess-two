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

import android.graphics.Canvas;

import com.googlecode.eyesfree.env.Size;
import com.googlecode.eyesfree.env.Stopwatch;

import java.util.Vector;

/**
 * Abstract class which defines base functionality of a preview frame processor,
 * to be called by PreviewLooper. Frame processors are chained together and may
 * use the output of preceeding FrameProcessors. Uses the Template Method design
 * pattern to delegate functionality to implementing classes. Modified by Alan
 * Viverette from Andrew Harp's original source.
 *
 * @author Andrew Harp
 * @author alanv@google.com (Alan Viverette)
 */
public abstract class FrameProcessor {
    private Stopwatch timer;

    private boolean initialized;

    private boolean firstFrame;

    /**
     * @return The amount of time this processor wishes to wait until it sees
     *         the next frame.
     */
    protected final long timeTillNextFrameMillis() {
        // No processor should pass up its first frame.
        if (firstFrame) {
            return 0;
        }

        // Frame processors have the ability to adjust the result of this method
        // by
        // modifying what is returned from getTimeBetweenFramesMillis().
        return getTimeBetweenFramesMillis() - timer.getElapsedMilliseconds();
    }

    /**
     * Overridable method controlling the rate at which this processor can
     * consume frames.
     *
     * @return The amount of time the frame processor wants to wait between
     *         successive calls to processFrame.
     */
    protected long getTimeBetweenFramesMillis() {
        // Greedy by default; always process available frames.
        return 0;
    }

    /**
     * Handles per-processor initialization. Guaranteed to be called at least
     * once after the camera size is known and before looping starts. May be
     * called multiple times. Calls onInit() on child-classes for customized
     * implementation.
     *
     * @param size
     */
    protected final synchronized void init(final Size size) {
        timer = new Stopwatch();
        timer.start();

        firstFrame = true;
        initialized = true;

        onInit(size);
    }

    /**
     * @return whether or not this processor has received at least one call to
     *         init().
     */
    protected final boolean isInitialized() {
        return initialized;
    }

    /**
     * Frees up any resources consumed by this processor. Calls onShutdown() to
     * handle customized cleanup.
     */
    protected final synchronized void shutdown() {
        timer = null;
        initialized = false;

        onShutdown();
    }

    /**
     * Every time processing thread is started, this method is guaranteed to be
     * called.
     */
    protected final synchronized void start() {
        onStart();
    }

    /**
     * Every time processing thread is stopped, this method is guaranteed to be
     * called.
     */
    protected final synchronized void stop() {
        onStop();
    }

    /**
     * Processes an incoming frame. Delegates actual processing to child-class.
     *
     * @param frame the frame to process.
     */
    protected final synchronized void processFrame(final TimestampedFrame frame) {
        timer.reset();
        firstFrame = false;

        onProcessFrame(frame);
    }

    /**
     * Overridable method for processors to define their own custom
     * initialization logic.
     *
     * @param size
     */
    protected void onInit(final Size size) {
    }

    /**
     * Overridable method for processors to define their own custom shutdown
     * logic.
     */
    protected void onShutdown() {
    }

    /**
     * Overridable method for processors to define their own custom logic when
     * they are started. This method is called every time the processing thread
     * is started and a sequence of frames will arrive at the processor.
     * onInit() is guaranteed to be called before this method.
     */
    protected void onStart() {
    }

    /**
     * Overridable method for processors to define their own custom logic when
     * they are stopped. This method is called every time the processing thread
     * is stopped and frame processor will not receive frames continuously.
     * onStart() may be called later when the frame processing thread is
     * restarted. Different from onShutdown() which is called when a processor
     * is destroyed, this method may be called several times.
     */
    protected void onStop() {
    }

    /**
     * The one method that FrameProcessors must implement themselves to do their
     * customized frame processing.
     *
     * @param frame the frame to process.
     */
    protected abstract void onProcessFrame(final TimestampedFrame frame);

    /**
     * Do all debug drawing here.
     *
     * @param canvas
     */
    protected final void drawDebug(Canvas canvas) {
        onDrawDebug(canvas);
    }

    protected void onDrawDebug(Canvas canvas) {
    }

    protected String getName() {
        return this.getClass().getSimpleName();
    }

    /**
     * @return A vector of strings to show, one per line, for debug purposes.
     */
    protected Vector<String> getDebugText() {
        return new Vector<String>();
    }

    protected final void preprocessFrame(TimestampedFrame frame) {
        onPreprocess(frame);
    }

    protected void onPreprocess(TimestampedFrame frame) {
    }
}
