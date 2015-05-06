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
import com.googlecode.eyesfree.textdetect.HydrogenTextDetector;
import com.googlecode.eyesfree.textdetect.HydrogenTextDetector.Parameters;
import com.googlecode.leptonica.android.Pix;
import com.googlecode.leptonica.android.Pixa;

/**
 * Frame processor that runs text detection.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class TextDetectionProcessor extends FrameProcessor {
    private HydrogenTextDetector mHydrogen;

    public TextDetectionProcessor() {
        mHydrogen = new HydrogenTextDetector();

        // We're relaxing the default parameters a little here...
        Parameters params = mHydrogen.getParameters();
        params.edge_thresh = 32;
        params.edge_avg_thresh = 1;
        params.cluster_min_blobs = 3;
        params.skew_enabled = true;
        mHydrogen.setParameters(params);
    }

    @Override
    protected synchronized void onInit(final Size size) {
        int width = size.width;
        int height = size.height;

        // TODO(alanv): Implement an image buffer throughout Hydrogen.
        mHydrogen.setSize(width, height);
    }

    @Override
    protected void onProcessFrame(TimestampedFrame frame) {
        if (frame.isBlurred() || frame.takenWhileFocusing()) {
            return;
        }

        Pix pixs = frame.getPixData();

        mHydrogen.setSourceImage(pixs);
        pixs.recycle();

        mHydrogen.detectText();
        Pixa pixa = mHydrogen.getTextAreas();
        float[] conf = mHydrogen.getTextConfs();
        float angle = mHydrogen.getSkewAngle();
        frame.setDetectedText(pixa, conf, angle);
        pixa.recycle();

        // TODO(alanv): This won't be necessary when we start using a buffer.
        mHydrogen.clear();
    }
}
