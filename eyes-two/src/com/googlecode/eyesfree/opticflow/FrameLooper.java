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
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.Log;

import com.googlecode.eyesfree.env.Size;
import com.googlecode.eyesfree.env.Stopwatch;
import com.googlecode.eyesfree.opticflow.FrameProducer.Frame;
import com.googlecode.eyesfree.opticflow.FrameProducer.FrameReceiver;

import java.util.ArrayList;
import java.util.Vector;

/**
 * Handles looping of the preview frames. FrameProcessors may be registered, and
 * each will get access to the TimestampedFrame buffer wrapper in sequence.
 * FrameProcessors may access data that came before them, so care must be taken
 * that they are added in the right order. Modified by Alan Viverette from
 * Andrew Harp's original source.
 *
 * @author Andrew Harp
 * @author alanv@google.com (Alan Viverette)
 */
public class FrameLooper implements FrameReceiver {
    private static final String TAG = "FrameLooper";

    private final FrameProducer frameProducer;

    private final DebugView debugView;

    private final Stopwatch stopwatch;

    // The thread that does the actual frame processing. Also acts as a flag for
    // whether the processing loop is active or not.
    private final ProcessingThread processingThreads[];

    // The number of preview frames that have been received from the camera.
    private int numPreviewFrames;

    private boolean firstRun;

    private boolean running;

    private Size size;

    ArrayList<FrameProcessor> allPreviewProcessors;

    public FrameLooper(
            final FrameProducer cameraManager, final DebugView debugView, final int[] delays) {
        this.frameProducer = cameraManager;
        this.debugView = debugView;

        this.stopwatch = new Stopwatch();

        this.firstRun = true;
        this.running = false;

        int width = frameProducer.getFrameWidth();
        int height = frameProducer.getFrameHeight();
        size = new Size(width, height);

        this.processingThreads = new ProcessingThread[delays.length];

        for (int level = 0; level < delays.length; ++level) {
            // The last processing thread should have priority
            // Thread.MIN_PRIORITY, every thread before it should have
            // incrementally more priority.
            final int priority = Thread.MIN_PRIORITY + delays.length - (level + 1);
            processingThreads[level] = new ProcessingThread(this, level, delays[level], priority);
        }
    }

    public void addPreviewProcessor(final FrameProcessor handler, final int level) {
        synchronized (processingThreads[level]) {
            processingThreads[level].addProcessor(handler);
            allPreviewProcessors = null;
        }
    }

    /**
     * @return A copy of the current processor list that is safe to modify.
     */
    public ArrayList<FrameProcessor> getAllProcessors() {
        if (allPreviewProcessors == null) {
            allPreviewProcessors = new ArrayList<FrameProcessor>();

            for (final ProcessingThread thread : processingThreads) {
                allPreviewProcessors.addAll(thread.getProcessors());
            }
        }
        return allPreviewProcessors;
    }

    final ArrayList<ProcessingThread> readyThreads = new ArrayList<ProcessingThread>();

    @Override
    public synchronized void onFrameReceived(final Frame frame) {
        ++numPreviewFrames;

        // Create a new TimestampedFrame to wrap the raw byte[].
        final TimestampedFrame previewFrame = new TimestampedFrame(frame);

        // TODO(alanv): Why does this run on the main thread?!
        processingThreads[0].preprocess(previewFrame);

        // readyThreads is a list of all threads that are done processing the
        // last
        // iteration.
        readyThreads.clear();
        for (int i = 1; i < processingThreads.length; ++i) {
            final ProcessingThread thread = processingThreads[i];
            if (thread.isReady()) {
                readyThreads.add(thread);
                thread.preprocess(previewFrame);
            }
        }

        // This is synchronized so that stopLoop is guaranteed to be called (if
        // desired) before preview frame processing finishes.
        processingThreads[0].processFrame(previewFrame);
        if (running) {
            frameProducer.requestFrame(this);
        }

        if (DebugView.isVisible) {
            debugView.invalidate();
        }

        for (final ProcessingThread thread : readyThreads) {
            thread.sendFrame(previewFrame);
        }
    }

    public synchronized void doneProcessing(final TimestampedFrame frame) {
        if (frame.allThreadsDone()) {
            frame.clearRawData();
        }
    }

    /**
     * Starts the loop for a finite number of frames.
     */
    public synchronized void startLoop() {
        stopwatch.start();
        running = true;

        numPreviewFrames = 0;

        Log.d(TAG, "Starting frame loop.");

        if (firstRun) {
            for (int i = 1; i < processingThreads.length; ++i) {
                processingThreads[i].start();
            }
        }
        firstRun = false;

        startAllProcessors();

        frameProducer.requestFrame(this);
    }

    /**
     * Stops the loop at its earliest possible convenience.
     */
    public synchronized void stopLoop() {
        if (running == false) {
            Log.w(TAG, "Stopping a FrameLooper that was already stoppped.");
            return;
        }

        // Reset the state of preview requests.
        frameProducer.requestFrame(null);

        stopAllProcessors();
        running = false;
    }

    /**
     * Guaranteed to be called at least once before the processors processes
     * anything and every time the preview size changes.
     */
    public void initAllProcessors() {
        int width = frameProducer.getFrameWidth();
        int height = frameProducer.getFrameHeight();
        size = new Size(width, height);

        for (final FrameProcessor processor : getAllProcessors()) {
            processor.init(size);
        }
    }

    private void startAllProcessors() {
        for (final FrameProcessor processor : getAllProcessors()) {
            processor.start();
        }
    }

    // TODO(xiaotao, andrewharp): Make sure that no frame slips through for
    // processing after onStop() is called on processor not running in the UI
    // thread
    private void stopAllProcessors() {
        for (final FrameProcessor processor : getAllProcessors()) {
            processor.stop();
        }
    }

    public void drawDebug(final Canvas canvas) {
        final ArrayList<FrameProcessor> processors = getAllProcessors();

        for (final FrameProcessor processor : processors) {
            synchronized (processor) {
                processor.drawDebug(canvas);
            }
        }

        // Draw the debug text in the bottom left.
        final Paint p = new Paint();

        final int xPos = 0;
        int startingYPos = canvas.getHeight();

        final int kLargeTextSize = 20;
        final int kSmallTextSize = 16;
        final int kTextBufferSize = 4;

        for (final FrameProcessor processor : processors) {
            Vector<String> lines;
            synchronized (processor) {
                lines = processor.getDebugText();
            }

            // TODO(andrewharp): Don't hardcode this, figure out the text
            // length.
            final int shadedWidth = 200;

            // Each block has one large header line followed by a buffer, then N
            // smaller lines each followed by a buffer, and then an additional
            // buffer.
            final int shadedHeight = kLargeTextSize + kTextBufferSize
                    + (kSmallTextSize + kTextBufferSize) * lines.size() + kTextBufferSize;

            startingYPos -= shadedHeight;

            p.setColor(Color.BLACK);
            p.setAlpha(100);

            int yPos = startingYPos;
            canvas.drawRect(new Rect(xPos, yPos, xPos + shadedWidth, yPos + shadedHeight), p);

            // Header line.
            p.setAlpha(255);

            p.setAntiAlias(true);
            p.setColor(Color.CYAN);
            p.setTextSize(kLargeTextSize);
            yPos += kLargeTextSize + kTextBufferSize;
            canvas.drawText(processor.getName(), xPos, yPos, p);

            // Debug lines.
            p.setColor(Color.WHITE);
            p.setTextSize(kSmallTextSize);
            for (final String line : lines) {
                yPos += kSmallTextSize + kTextBufferSize;
                canvas.drawText(line, xPos, yPos, p);
            }

            yPos += kTextBufferSize;
        }
    }

    public boolean isRunning() {
        return running;
    }
}
