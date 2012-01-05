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
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager.LayoutParams;

import com.googlecode.eyesfree.ocr.R;

import java.io.IOException;

/**
 * A basic activity capable of rendering a camera preview as its background.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public abstract class CameraActivity extends Activity {
    /** Used for controlling the camera. */
    private CameraManager mCameraManager;

    /** Whether the preview is ready for drawing. */
    private boolean mHasSurface;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mCameraManager = new CameraManager(this);
        mHasSurface = false;

        getWindow().addFlags(LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    @Override
    protected void onResume() {
        super.onResume();

        SurfaceView surfaceView = (SurfaceView) findViewById(R.id.preview);
        SurfaceHolder surfaceHolder = surfaceView.getHolder();

        if (mHasSurface) {
            initializeCamera(surfaceHolder);
        } else {
            surfaceHolder.addCallback(surfaceHolderCallback);
            surfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        }
    }

    @Override
    protected void onPause() {
        synchronized (mCameraManager) {
            mHasSurface = false;
            mCameraManager.closeDriver();
        }

        super.onPause();
    }

    /**
     * @return the camera manager
     */
    protected CameraManager getCameraManager() {
        return mCameraManager;
    }

    /**
     * Initializes the camera by opening the surface holder, starting preview,
     * and starting focusing.
     *
     * @param surfaceHolder
     */
    private void initializeCamera(SurfaceHolder surfaceHolder) {
        try {
            mCameraManager.openDriver(surfaceHolder);
            mCameraManager.startPreview();

            onCameraStarted();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private final SurfaceHolder.Callback surfaceHolderCallback = new SurfaceHolder.Callback() {
        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            if (!mHasSurface) {
                mHasSurface = true;

                initializeCamera(holder);
            }
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            mHasSurface = false;
        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            // Do nothing
        }
    };

    protected abstract void onCameraStarted();
}
