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

import android.content.Context;
import android.graphics.ImageFormat;
import android.graphics.Point;
import android.hardware.Camera;
import android.hardware.Camera.Size;
import android.os.SystemClock;
import android.util.Log;
import android.view.Display;
import android.view.OrientationEventListener;
import android.view.SurfaceHolder;
import android.view.WindowManager;

import com.googlecode.eyesfree.opticflow.FrameProducer;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.LinkedList;
import java.util.List;

/**
 * This object wraps the Camera service object and expects to be the only one
 * talking to it. The implementation encapsulates the steps needed to take
 * preview-sized images, which are used for both preview and decoding. Based on
 * code from ZXing licensed under Apache License, Version 2.0.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class CameraManager implements FrameProducer {
    private static final String TAG = "CameraManager";

    private static final int PREVIEW_FORMAT = ImageFormat.NV21;

    private static final int PICTURE_FORMAT = ImageFormat.JPEG;

    private static enum CameraState {
        NOT_READY, IDLE, FOCUSING, FOCUSED
    }

    private Camera mCamera;

    private final WeakReference<Context> mContext;

    private final OrientationEventListener mOrientationListener;

    private Point mScreenResolution;

    private boolean mInitialized;

    private boolean mPreviewing;

    private CameraState mState;

    private boolean mSupportsFocus;

    private int mPictureWidth;

    private int mPictureHeight;

    private int mOrientation;

    private String mFlashMode;

    private boolean mFlashlight;

    private int mLastOrientation;

    private Camera.AutoFocusCallback mAutoFocusCallback;

    private Camera.PictureCallback mTakePictureCallback;

    private LinkedList<FrameReceiver> mFrameRequesters;

    private LinkedList<byte[]> mPreviewBuffers;

    private Size mPreviewSize;

    private int mPreviewFormat;

    private int mPreviewBufferSize;

    public CameraManager(Context context) {
        mContext = new WeakReference<Context>(context);
        mCamera = null;
        mInitialized = false;
        mPreviewing = false;
        mLastOrientation = 0;
        mFlashMode = Camera.Parameters.FLASH_MODE_AUTO;
        mFlashlight = false;
        mState = CameraState.NOT_READY;

        mFrameRequesters = new LinkedList<FrameReceiver>();
        mPreviewBuffers = new LinkedList<byte[]>();

        // Create orientation listenter. This should be done first because it
        // takes some time to get first orientation.
        mOrientationListener = new OrientationEventListener(context) {
            @Override
            public void onOrientationChanged(int orientation) {
                // We keep the last known orientation. So if the user
                // first orient the camera then point the camera to
                if (orientation != ORIENTATION_UNKNOWN) {
                    orientation += 90;
                }

                orientation = ((orientation / 90) * 90) % 360;

                if (orientation != mLastOrientation) {
                    mLastOrientation = orientation;
                }
            }
        };
        mOrientationListener.enable();
    }

    public void openDriver(SurfaceHolder holder) throws IOException {
        if (mCamera != null) {
            return;
        }

        mCamera = Camera.open();
        mCamera.setDisplayOrientation(mOrientation);
        mCamera.setPreviewDisplay(holder);

        if (!mInitialized) {
            mInitialized = true;
            getScreenResolution();
        }

        setPreviewParameters();

        Camera.Parameters params = mCamera.getParameters();

        String focusMode = params.getFocusMode();

        mSupportsFocus = !Camera.Parameters.FOCUS_MODE_FIXED.equals(focusMode);

        mState = CameraState.IDLE;
    }

    public void closeDriver() {
        if (mCamera != null) {
            mCamera.release();
            mCamera = null;
            mPreviewing = false;
            mState = CameraState.NOT_READY;
        }

        // TODO(alanv): Need to give all requesters null frames
        mFrameRequesters.clear();
        mAutoFocusCallback = null;
        mTakePictureCallback = null;
    }

    public void startPreview() {
        if (mCamera != null && !mPreviewing) {
            mCamera.startPreview();
            mPreviewing = true;
        }
    }

    public void stopPreview() {
        if (mCamera != null && mPreviewing) {
            mCamera.setPreviewCallback(null);
            mCamera.stopPreview();
            mPreviewing = false;
        }
    }

    public boolean isFocusSupported() {
        return mSupportsFocus;
    }

    @Override
    public int getFrameWidth() {
        return mPreviewSize.width;
    }

    @Override
    public int getFrameHeight() {
        return mPreviewSize.height;
    }

    /**
     * Requests a single preview frame from the camera. If the provided callback
     * is <code>null</code> then pending frame requests will be canceled.
     *
     * @param callback
     */
    @Override
    public void requestFrame(FrameReceiver callback) {
        if (mCamera == null) {
            return;
        }

        if (callback == null) {
            synchronized (mPreviewBuffers) {
                mPreviewBuffers.clear();
                // mCamera.setPreviewCallback(null);
            }

            return;
        }

        synchronized (this) {
            mFrameRequesters.addLast(callback);
        }

        byte[] buffer = null;

        synchronized (mPreviewBuffers) {
            if (mPreviewBuffers.isEmpty()) {
                buffer = new byte[mPreviewBufferSize];
            } else {
                buffer = mPreviewBuffers.remove();
            }
        }

        mCamera.addCallbackBuffer(buffer);
    }

    /**
     * Returns a preview buffer to the stack of available buffers.
     *
     * @param buffer
     */
    public void releaseData(byte[] buffer) {
        synchronized (mPreviewBuffers) {
            mPreviewBuffers.add(buffer);
        }
    }

    public void requestFocus(Camera.AutoFocusCallback callback) {
        if (mCamera == null) {
            return;
        }

        if (callback == null) {
            Log.e(TAG, "Focus callback was null, cancelling autofocus...");
            mCamera.cancelAutoFocus();
        }

        synchronized (this) {
            if (mState == CameraState.FOCUSED || mState == CameraState.FOCUSING) {
                return;
            }

            mState = CameraState.FOCUSING;
        }

        mAutoFocusCallback = callback;
        mCamera.autoFocus(autoFocusCallback);
    }

    public void requestPicture(Camera.PictureCallback callback) {
        if (mCamera == null || !mPreviewing) {
            return;
        }

        Camera.Parameters parameters = mCamera.getParameters();
        parameters.setPictureFormat(PICTURE_FORMAT);
        parameters.setRotation(mLastOrientation);

        List<Size> supported = parameters.getSupportedPictureSizes();

        if (mPictureWidth > 0 && mPictureHeight > 0) {
            int height = -1;
            int width = -1;

            for (Size size : supported) {
                if (size.height >= mPictureHeight && (size.height < height || height < 0)) {
                    width = size.width;
                    height = size.height;
                }
            }

            parameters.setPictureSize(width, height);
        } else {
            Size size = supported.get(supported.size() - 1);

            int width = size.width;
            int height = size.height;

            parameters.setPictureSize(width, height);
        }

        // Flashlight setting overrides flash mode setting
        if (mFlashlight) {
            parameters.setFlashMode(Camera.Parameters.FLASH_MODE_TORCH);
        } else if (mFlashMode != null) {
            parameters.setFlashMode(mFlashMode);
        }

        mTakePictureCallback = callback;

        // Must clear preview callback to take pictures
        mCamera.setPreviewCallback(null);
        mCamera.setParameters(parameters);
        mCamera.takePicture(shutterCallback, rawCallback, pictureCallback);
    }

    /**
     * Preview frames are delivered here, which we pass on to the registered
     * handler. Make sure to clear the handler so it will only receive one
     * message.
     */
    private final Camera.PreviewCallback frameCallback = new Camera.PreviewCallback() {
        @Override
        public void onPreviewFrame(final byte[] data, final Camera camera) {
            // TODO(alanv): Can't call camera.getParameters() because it causes
            // a crash!
            // final Camera.Parameters params = camera.getParameters();
            final int format = mPreviewFormat;
            final Size size = mPreviewSize;

            FrameReceiver callback;

            synchronized (this) {
                if (mFrameRequesters.isEmpty()) {
                    Log.e(TAG, "Preview callback was null, discarding results");
                    return;
                } else {
                    callback = mFrameRequesters.removeFirst();
                }
            }

            long timestamp = SystemClock.uptimeMillis();

            Frame frame = new Frame(data, size.width, size.height, format, timestamp) {
                @Override
                public void recycle() {
                    synchronized (mPreviewBuffers) {
                        mPreviewBuffers.add(data);
                    }
                }
            };

            callback.onFrameReceived(frame);
        }
    };

    private final Camera.ShutterCallback shutterCallback = new Camera.ShutterCallback() {
        @Override
        public void onShutter() {
            // Do nothing.
        }
    };

    private final Camera.PictureCallback rawCallback = new Camera.PictureCallback() {
        @Override
        public void onPictureTaken(byte[] data, Camera camera) {
            // Do nothing.
        }
    };

    private final Camera.PictureCallback pictureCallback = new Camera.PictureCallback() {
        @Override
        public void onPictureTaken(final byte[] data, final Camera camera) {
            // Return jpeg data to callback.
            mTakePictureCallback.onPictureTaken(data, camera);
        }
    };

    private final Camera.AutoFocusCallback autoFocusCallback = new Camera.AutoFocusCallback() {
        @Override
        public void onAutoFocus(final boolean success, final Camera camera) {
            synchronized (this) {
                mState = CameraState.FOCUSED;

                // Camera.Parameters parameters = mCamera.getParameters();
                // parameters.setPreviewFormat(PREVIEW_FORMAT);
                // parameters.setRotation(mLastOrientation);
                // mCamera.setParameters(parameters);

                mAutoFocusCallback.onAutoFocus(success, camera);
                mAutoFocusCallback = null;

                mState = CameraState.IDLE;
            }
        }
    };

    /**
     * Sets the camera up to take preview images which are used for both preview
     * and decoding. We're counting on the default YUV420 semi-planar data. If
     * that changes in the future, we'll need to specify it explicitly
     * witrivateh setPreviewFormat().
     */
    private void setPreviewParameters() {
        Camera.Parameters parameters = mCamera.getParameters();
        parameters.setPreviewFormat(PREVIEW_FORMAT);

        List<Size> supported = parameters.getSupportedPreviewSizes();
        Size selectedSize = mCamera.new Size(0, 0);

        for (Size size : supported) {
            if (size.width * size.height > selectedSize.width * selectedSize.height
                    && size.height <= 480) {
                selectedSize = size;
            }
        }

        if (mFlashlight) {
            parameters.setFlashMode(Camera.Parameters.FLASH_MODE_TORCH);
        } else {
            parameters.setFlashMode(Camera.Parameters.FLASH_MODE_OFF);
        }

        parameters.setPreviewSize(selectedSize.width, selectedSize.height);

        int bitsPerPixel = ImageFormat.getBitsPerPixel(PREVIEW_FORMAT);
        mPreviewBufferSize = selectedSize.width * selectedSize.height * bitsPerPixel / 8;
        mPreviewSize = selectedSize;
        mPreviewFormat = PREVIEW_FORMAT;

        mCamera.setPreviewCallbackWithBuffer(frameCallback);
        mCamera.setParameters(parameters);
    }

    public Size getPreviewSize() {
        Camera.Parameters parameters = mCamera.getParameters();
        Size previewSize = parameters.getPreviewSize();

        return previewSize;
    }

    public void setPictureSize(int width, int height) {
        mPictureWidth = width;
        mPictureHeight = height;
    }

    /**
     * Sets the camera flash mode. Uses Camera.Parameters.FLASH_ constants.
     *
     * @param mode
     */
    public void setFlashMode(String mode) {
        mFlashMode = mode;
    }

    /**
     * Turns the camera light on or off. Takes effect immediately.
     *
     * @param light
     */
    public void setFlashlight(boolean light) {
        mFlashlight = light;

        if (mCamera != null) {
            Camera.Parameters parameters = mCamera.getParameters();

            if (mFlashlight) {
                parameters.setFlashMode(Camera.Parameters.FLASH_MODE_TORCH);
            } else {
                parameters.setFlashMode(Camera.Parameters.FLASH_MODE_OFF);
            }

            mCamera.setParameters(parameters);
        }
    }

    private Point getScreenResolution() {
        if (mScreenResolution == null) {
            WindowManager manager = (WindowManager) mContext.get().getSystemService(
                    Context.WINDOW_SERVICE);
            Display display = manager.getDefaultDisplay();
            mScreenResolution = new Point(display.getWidth(), display.getHeight());
        }

        return mScreenResolution;
    }
}
