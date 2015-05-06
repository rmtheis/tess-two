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

import android.os.Handler;
import android.os.Looper;
import android.os.Message;

import com.googlecode.eyesfree.env.Stopwatch;

import java.util.Vector;

/**
 * This is the thread where the actual work gets done. Uses the Android
 * Looper/Handler paradigm to receive frames from the UI thread.
 *
 * Modified by Alan Viverette from Andrew Harp's original source.
 *
 * @author Andrew Harp
 * @author alanv@google.com (Alan Viverette)
 */
public class ProcessingThread extends Thread {
    private static final int STOP_CODE = -1;

    private final Vector<FrameProcessor> previewProcessors;

    private Handler processingHandler;

    private volatile boolean isProcessing;

    private final Stopwatch stopwatch;

    private final String name;

    private final FrameLooper previewLooper;

    private final int delay;

    private final int priority;

    public ProcessingThread(
            final FrameLooper previewLooper, final int level, final int delay, final int priority) {
        this.previewLooper = previewLooper;
        this.delay = delay;
        this.priority = priority;

        this.isProcessing = false;
        this.stopwatch = new Stopwatch();
        this.previewProcessors = new Vector<FrameProcessor>();
        this.name = "FrameProcessingThread" + level;

        stopwatch.start();

        setName(name);
    }

    protected void kill() {
        processingHandler.sendEmptyMessage(STOP_CODE);
        processingHandler = null;
    }

    protected void sendFrame(final TimestampedFrame previewFrame) {
        final Message message = new Message();
        message.obj = previewFrame;
        // TODO(mrcasey): Figure out when this is null... it doesn't seem like
        // it should be.
        if (processingHandler != null) {
            processingHandler.sendMessage(message);
        }
    }

    protected boolean isReady() {
        return !isProcessing && stopwatch.getElapsedMilliseconds() >= delay;
    }

    protected void preprocess(final TimestampedFrame previewFrame) {
        previewFrame.threadStart();

        isProcessing = true;
        for (final FrameProcessor processor : previewProcessors) {
            synchronized (processor) {
                processor.preprocessFrame(previewFrame);
            }
        }
    }

    /**
     * Processes the frame in a the processing thread.
     */
    protected void processFrame(final TimestampedFrame frame) {
        stopwatch.reset();

        final Vector<FrameProcessor> processors = getProcessors();

        // This is where the frame handling happens.
        for (final FrameProcessor processor : processors) {
            synchronized (processor) {
                if (!processor.isInitialized()) {
                    // Initialize any processors that have been added lately.
                    processor.init(frame.getSize());
                }

                processor.processFrame(frame);
            }
        }

        frame.threadDone();
        previewLooper.doneProcessing(frame);
        isProcessing = false;
    }

    @Override
    public void run() {
        setPriority(priority);
        Looper.prepare();
        processingHandler = new Handler() {
            @Override
            public void handleMessage(final Message msg) {
                // Stop the loop if stopLoop has been called (and cleared
                // processingHandler).
                if (processingHandler == null) {
                    final Vector<FrameProcessor> processors = getProcessors();
                    for (final FrameProcessor processor : processors) {
                        synchronized (processor) {
                            processor.shutdown();
                        }
                    }
                    Looper.myLooper().quit();
                    return;
                }

                processFrame((TimestampedFrame) msg.obj);
            }
        };
        Looper.loop();
    }

    public void addProcessor(final FrameProcessor handler) {
        previewProcessors.add(handler);
    }

    public Vector<FrameProcessor> getProcessors() {
        return new Vector<FrameProcessor>(previewProcessors);
    }
}
