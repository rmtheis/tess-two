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

import android.hardware.Camera;
import android.hardware.Camera.AutoFocusCallback;

import com.googlecode.eyesfree.env.Size;
import com.googlecode.eyesfree.env.Stopwatch;
import com.googlecode.eyesfree.ocr.intent.CameraManager;

import java.util.Vector;

/**
 * An {@link FrameProcessor} to detect image blurriness for each preview frame.
 * Modified by Alan Viverette from Xiaotao Duan's original source.
 *
 * @author Xiaotao Duan
 * @author alanv@google.com (Alan Viverette)
 */
public class ImageBlurProcessor extends FrameProcessor implements AutoFocusCallback {
    /**
     * Minimum period to wait between consecutive focus requests.
     */
    private static final long MIN_TIME_BETWEEN_FOCUS_REQUESTS_MS = 1500;

    /**
     * Period during which frames must be consistently blurry to trigger a focus
     * request.
     */
    private static final long FOCUS_DELAY_MS = 300;

    private static final int MIN_DIFF_PERCENT = 10;

    private final Stopwatch lastFocusTimer = new Stopwatch();

    private final Stopwatch consecutiveBlurTimer = new Stopwatch();

    private volatile int movingBits = 0;

    private final CameraManager cameraManager;

    private final Vector<String> debugText = new Vector<String>();

    private boolean lastFrameBlurred = false;

    private boolean focusing = false;

    private volatile boolean justFocused = false;

    private boolean hasFocusedWithSameImage = false;

    private int lastDiffPercent = 0;

    private long lastBlurDuration;

    private int[] focusedSignature;

    private int[] currFrameSignature;

    private boolean runSinceLastTime = false;

    private boolean movedSinceLastFocus = false;

    public ImageBlurProcessor(final CameraManager cameraManager) {
        this.cameraManager = cameraManager;
    }

    @Override
    protected void onInit(final Size size) {
        // We want to start timing as early as possible for our inter-focus
        // delays.
        lastFocusTimer.reset();
        lastFocusTimer.start();

        // Make sure this is zeroed and not running.
        consecutiveBlurTimer.stop();
        consecutiveBlurTimer.reset();

        justFocused = false;
        focusing = false;
        lastFrameBlurred = false;
        lastDiffPercent = 0;

        focusedSignature = null;
    }

    @Override
    protected void onStart() {
        onInit(null);
    }

    public void requestFocus() {
        focusing = true;
        cameraManager.requestFocus(this);
    }

    @Override
    public void onAutoFocus(final boolean success, Camera camera) {
        lastFocusTimer.reset();
        lastFocusTimer.start();
        movedSinceLastFocus = false;
        // Make sure we don't focus on the very next frame we get, just in case.
        consecutiveBlurTimer.stop();
        consecutiveBlurTimer.reset();
        justFocused = true;
        focusing = false;
    }

    /**
     * Tells blurriness of last preview frame processed by this processor.
     * Caller may use the return value to decide whether a focus action is
     * necessary at this moment.
     */
    public boolean isLastFrameBlurred() {
        return lastFrameBlurred;
    }

    @Override
    protected void onProcessFrame(final TimestampedFrame frame) {
        runSinceLastTime = true;

        frame.setTakenWhileFocusing(focusing);

        // No activity if we're moving or focusing.
        if (focusing) {
            return;
        }
        if (movingBits > 0) {
            // If we move, we will focus without considering
            // MIN_TIME_BETWEEN_FOCUS_REQUESTS_MS
            // after we stop.
            movedSinceLastFocus = true;
            return;
        }

        // TODO(xiaotao): Remove time evaluation and debugText.
        final long start = System.currentTimeMillis();
        // First check to see if the current image is blurred.
        lastFrameBlurred = ImageBlur.isBlurred(frame.getRawData(), frame.getWidth(), frame.getHeight());
        final long end = System.currentTimeMillis();
        lastBlurDuration = end - start;

        frame.setBlurred(lastFrameBlurred);

        // Aborts if camera does not support focus
        // if (!cameraManager.isFocusSupported()) {
        // return;
        // }

        // Make sure the blur timer is running if the frame was blurred,
        // otherwise reset it.
        if (lastFrameBlurred) {
            if (!consecutiveBlurTimer.isRunning()) {
                consecutiveBlurTimer.start();
            }
        } else {
            consecutiveBlurTimer.stop();
            consecutiveBlurTimer.reset();
        }

        // If we've gone more than FOCUS_DELAY_MS frames worth with a blurry
        // image, see if the image has changed enough, and if so, focus.
        if (consecutiveBlurTimer.getElapsedMilliseconds() > FOCUS_DELAY_MS) {
            // See if device has been moved or enough time has gone by since the
            // last focus attempt.
            if (movedSinceLastFocus) {
                focusedSignature = null;
                hasFocusedWithSameImage = false;
                requestFocus();
            } else if (lastFocusTimer.getElapsedMilliseconds() > MIN_TIME_BETWEEN_FOCUS_REQUESTS_MS) {
                // Only request a focus if we've changed significantly since the
                // last focus finished.
                currFrameSignature = ImageBlur.computeSignature(frame.getRawData(), frame.getWidth(), frame.getHeight(),
                        currFrameSignature);

                // This logic computes the image difference from the last
                // post-focus event frame, if it exists.
                if (focusedSignature != null) {
                    lastDiffPercent = ImageBlur.diffSignature(currFrameSignature, focusedSignature);
                } else {
                    // If we've never focused before we're not going to have a
                    // previous signature to compare to, so force the focus
                    // event.
                    lastDiffPercent = 100;
                }

                // If the difference is above a certain threshold, focus. If
                // not, it's probably the case that the environment is not
                // amenable to focusing anyway, so lets just wait till there is
                // a difference.
                if (lastDiffPercent > MIN_DIFF_PERCENT) {
                    hasFocusedWithSameImage = false;
                    requestFocus();
                } else if (!hasFocusedWithSameImage) {
                    // If the image is blurry but it looks the same as last
                    // time, try focusing once more.
                    hasFocusedWithSameImage = true;
                    requestFocus();
                }
            }
        } else if (justFocused) {
            // Take a snapshot of the image signature for reference when decided
            // whether to refocus or not.
            focusedSignature = ImageBlur.computeSignature(frame.getRawData(), frame.getWidth(), frame.getHeight(),
                    focusedSignature);
            justFocused = false;
        }
    }

    @Override
    protected Vector<String> getDebugText() {
        if (runSinceLastTime) {
            debugText.clear();
            debugText.add((lastFrameBlurred ? "blurred" : "focused") + ": " + lastBlurDuration
                    + " ms(s)");
            debugText.add("moving: " + movingBits);
            debugText.add("lastDiffPercent: " + lastDiffPercent);
            runSinceLastTime = false;
        }
        return debugText;
    }
}
