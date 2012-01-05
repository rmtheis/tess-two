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

import android.app.Activity;
import android.graphics.Rect;
import android.graphics.YuvImage;
import android.hardware.Camera;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;

import com.googlecode.eyesfree.ocr.client.Intents;
import com.googlecode.eyesfree.opticflow.FrameProducer.Frame;
import com.googlecode.eyesfree.opticflow.FrameProducer.FrameReceiver;

import java.io.IOException;
import java.io.OutputStream;

/**
 * @author alanv@google.com (Alan Viverette)
 */
public abstract class CaptureActivity extends CameraActivity {
    private static final String TAG = "CaptureActivity";

    private static final int DEFAULT_JPEG_QUALITY = 85;

    private Uri mSaveUri;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mSaveUri = getIntent().getParcelableExtra(Intents.Detect.EXTRA_OUTPUT);
    }

    protected void takePreview() {
        getCameraManager().requestFrame(frameCallback);
    }

    protected void takePicture() {
        getCameraManager().requestPicture(pictureCallback);
    }

    private void writePreview(byte[] yuvData, int format, int width, int height) {
        Log.i(TAG, "Writing preview to " + mSaveUri.toString());

        int result = Activity.RESULT_CANCELED;

        try {
            Rect rectangle = new Rect(0, 0, width, height);

            OutputStream stream = getContentResolver().openOutputStream(mSaveUri);

            YuvImage yuvImage = new YuvImage(yuvData, format, width, height, null);
            yuvImage.compressToJpeg(rectangle, DEFAULT_JPEG_QUALITY, stream);

            stream.close();

            result = Activity.RESULT_OK;
        } catch (IOException e) {
            e.printStackTrace();
        }

        setResult(result);
        finish();
    }

    private void writePicture(byte[] jpegData) {
        Log.i(TAG, "Writing picture to " + mSaveUri.toString());

        int result = Activity.RESULT_CANCELED;

        try {
            OutputStream stream = getContentResolver().openOutputStream(mSaveUri);

            stream.write(jpegData);
            stream.close();

            result = Activity.RESULT_OK;
        } catch (IOException e) {
            e.printStackTrace();
        }

        setResult(result);
        finish();
    }

    private final FrameReceiver frameCallback = new FrameReceiver() {
        @Override
        public void onFrameReceived(Frame frame) {
            writePreview(frame.data, frame.format, frame.width, frame.height);

            frame.recycle();
        }
    };

    private final Camera.PictureCallback pictureCallback = new Camera.PictureCallback() {
        @Override
        public void onPictureTaken(byte[] data, Camera camera) {
            writePicture(data);
        }
    };
}
