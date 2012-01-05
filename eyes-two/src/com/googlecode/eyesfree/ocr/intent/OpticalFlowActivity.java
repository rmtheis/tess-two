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

import android.os.Bundle;

import com.googlecode.eyesfree.ocr.R;
import com.googlecode.eyesfree.opticflow.DebugView;
import com.googlecode.eyesfree.opticflow.FrameLooper;
import com.googlecode.eyesfree.opticflow.ImageBlurProcessor;
import com.googlecode.eyesfree.opticflow.OcrProcessor;
import com.googlecode.eyesfree.opticflow.OpticalFlowProcessor;
import com.googlecode.eyesfree.opticflow.TextDetectionProcessor;
import com.googlecode.eyesfree.opticflow.TextTrackerProcessor;

import java.io.File;

/**
 * Computes and displays likely text areas and live OCR results.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class OpticalFlowActivity extends CameraActivity {
    private static final String DEFAULT_LANGUAGE = "eng";

    private String mLanguage;
    private File mDatapath;
    private VoiceGestureView mVoiceView;
    private DebugView mDebugView;
    private FrameLooper mPreviewLooper;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.optical_flow);

        mDebugView = (DebugView) findViewById(R.id.debug_view);
        mVoiceView = (VoiceGestureView) findViewById(R.id.gesture_view);

        final String lang = getString(R.string.lang_pref);
        mLanguage = getPreferences(MODE_PRIVATE).getString(lang, DEFAULT_LANGUAGE);
        mDatapath = getExternalFilesDir(null);
    }

    @Override
    protected void onStop() {
        mPreviewLooper.stopLoop();
        mVoiceView.shutdown();

        super.onStop();
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

        final OcrProcessor ocr = new OcrProcessor(mDatapath.toString(), mLanguage, textTracker);
        mPreviewLooper.addPreviewProcessor(ocr, 2);

        final ImageBlurProcessor imageBlur = new ImageBlurProcessor(cameraManager);
        mPreviewLooper.addPreviewProcessor(imageBlur, 3);

        // This is a workaround for an issue where the previewLooper isn't
        // running properly when the activity is restarted.
        // TODO(mrcasey): Figure out why this seems to fix the issue.
        if (mPreviewLooper.isRunning()) {
            mPreviewLooper.stopLoop();
        }

        ocr.setListener(ocrListener);

        mPreviewLooper.initAllProcessors();
        mPreviewLooper.startLoop();
    }

    private final OcrProcessor.Listener ocrListener = new OcrProcessor.Listener() {
        @Override
        public void onResult(String result, int[] confs) {
            if (result != null && result.length() > 0 && confs.length > 0) {
                int avgConf = 0;
                int minConf = confs[0];
                for (int conf : confs) {
                    avgConf += conf;
                    minConf = Math.min(minConf, conf);
                }
                avgConf /= confs.length;

                if (avgConf > 70 && minConf > 60) {
                    mVoiceView.addUtterance(result);
                }
            }
        }
    };
}
