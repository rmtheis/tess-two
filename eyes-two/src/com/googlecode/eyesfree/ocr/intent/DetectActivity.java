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

package com.googlecode.eyesfree.ocr.intent;

import android.graphics.RectF;
import android.os.Bundle;
import android.view.View;

import com.googlecode.eyesfree.ocr.R;
import com.googlecode.eyesfree.opticflow.DebugView;
import com.googlecode.eyesfree.opticflow.FrameLooper;
import com.googlecode.eyesfree.opticflow.ImageBlurProcessor;
import com.googlecode.eyesfree.opticflow.OpticalFlowProcessor;
import com.googlecode.eyesfree.opticflow.TextDetectionProcessor;
import com.googlecode.eyesfree.opticflow.TextTrackerProcessor;
import com.googlecode.eyesfree.opticflow.TextTrackerProcessor.TrackedRect;

import java.util.ArrayList;
import java.util.LinkedList;

/**
 * Analyzes the camera preview and provides feedback on whether there is text on
 * the screen. Useful for helping users with visual impairments decide whether
 * something is worth performing OCR on.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class DetectActivity extends CaptureActivity {
    private FrameLooper mPreviewLooper;
    private DebugView mDebugView;
    private NoisyDetector mNoisyDetector;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.detect);

        mNoisyDetector = new NoisyDetector(this);

        mDebugView = (DebugView) findViewById(R.id.debug_view);

        findViewById(R.id.preview).setOnClickListener(clickListener);
    }

    /*
     * private void processIntent(Intent settings) { Display display =
     * getWindowManager().getDefaultDisplay(); int width =
     * settings.getIntExtra(Intents.Detect.WIDTH, -1); int height =
     * settings.getIntExtra(Intents.Detect.HEIGHT, -1); String flashMode =
     * settings.getStringExtra(Intents.Detect.FLASH_MODE); boolean flashlight =
     * settings.getBooleanExtra( Intents.Detect.FLASHLIGHT, DEFAULT_FLASHLIGHT);
     * mCameraManager.setPictureSize(width, height);
     * mCameraManager.setFlashlight(flashlight);
     * mCameraManager.setFlashMode(flashMode); }
     */

    @Override
    protected void onResume() {
        super.onResume();

        mNoisyDetector.start();

        if (mPreviewLooper != null) {
            mPreviewLooper.startLoop();
        }
    }

    @Override
    protected void onPause() {
        mNoisyDetector.pause();

        if (mPreviewLooper != null) {
            mPreviewLooper.stopLoop();
        }

        super.onPause();
    }

    @Override
    protected void onCameraStarted() {
        initializeContinuous();
    }

    private void initializeContinuous() {
        CameraManager cameraManager = getCameraManager();

        final int[] delayMillis = {
                0, 10, 100, 1000
        };

        mPreviewLooper = new FrameLooper(cameraManager, mDebugView, delayMillis);
        mDebugView.setCallback(mPreviewLooper);

        final OpticalFlowProcessor opticalFlow = new OpticalFlowProcessor();
        mPreviewLooper.addPreviewProcessor(opticalFlow, 1);

        final TextDetectionProcessor textDetect = new TextDetectionProcessor();
        mPreviewLooper.addPreviewProcessor(textDetect, 2);

        final TextTrackerProcessor textTracker = new TextTrackerProcessor(
                opticalFlow.getOpticalFlow());
        mPreviewLooper.addPreviewProcessor(textTracker, 2);

        final ImageBlurProcessor imageBlur = new ImageBlurProcessor(cameraManager);
        mPreviewLooper.addPreviewProcessor(imageBlur, 3);

        // This is a workaround for an issue where the previewLooper isn't
        // running properly when the activity is restarted.
        // TODO(mrcasey): Figure out why this seems to fix the issue.
        if (mPreviewLooper.isRunning()) {
            mPreviewLooper.stopLoop();
        }

        textTracker.setListener(textListener);

        mPreviewLooper.initAllProcessors();
        mPreviewLooper.startLoop();
    }

    private final TextTrackerProcessor.Listener textListener = new TextTrackerProcessor.Listener() {
        @Override
        public void onTextDetected(RectF bounds, LinkedList<TrackedRect> trackedRects) {
            ArrayList<RectF> trackedBounds = new ArrayList<RectF>(trackedRects.size());

            for (TrackedRect rect : trackedRects) {
                trackedBounds.add(rect.rect);
            }

            mNoisyDetector.onTextDetected(bounds, trackedBounds);
        }
    };

    private final View.OnClickListener clickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if (v.getId() == R.id.preview) {
				mNoisyDetector.pause();
				mPreviewLooper.stopLoop();
				takePreview();
			}
        }
    };
}
