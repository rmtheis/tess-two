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

import android.graphics.PointF;

/**
 * Interface to native optical flow library.
 *
 * Modified by Alan Viverette from Andrew Harp's original source.
 *
 * @author Andrew Harp
 * @author alanv@google.com (Alan Viverette)
 */
public class OpticalFlow {
    static {
        System.loadLibrary("lept");
        System.loadLibrary("opticalflow");
    }

    @Override
    protected void finalize() {
        resetNative();
    }

    public void initialize(int width, int height, int downsampleFactor) {
        initNative(width, height, downsampleFactor);
    }

    public void setImage(byte[] data, long timestamp) {
        addFrameNative(data, timestamp);
    }

    public void computeOpticalFlow() {
        computeFeaturesNative(true);
        computeFlowNative();
        printInfoNative();
    }

    public float[] getFeatures(boolean onlyReturnCorrespondingFeatures) {
        return getFeaturesNative(onlyReturnCorrespondingFeatures);
    }

    public PointF getAccumulatedDelta(
            long timestamp, float positionX, float positionY, float radius) {
        float[] delta = new float[2];

        getAccumulatedDeltaNative(timestamp, positionX, positionY, radius, delta);

        return new PointF(delta[0], delta[1]);
    }

    public void addInterestRegion(int numX, int numY, int left, int top, int right, int bottom) {
        addInterestRegionNative(numX, numY, left, top, right, bottom);
    }

    /*********************** NATIVE METHODS *************************************/

    private native void initNative(int width, int height, int downsampleFactor);

    private native void addFrameNative(byte[] data, long timeStamp);

    private native void computeFeaturesNative(boolean cachedOk);

    private native void computeFlowNative();

    private native void printInfoNative();

    private native void getAccumulatedDeltaNative(
            long timestamp, float positionX, float positionY, float radius, float[] delta);

    private native void addInterestRegionNative(
            int numX, int numY, float left, float top, float right, float bottom);

    private native float[] getFeaturesNative(boolean onlyReturnCorrespondingFeatures);

    private native void resetNative();
}
