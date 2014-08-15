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

package com.googlecode.eyesfree.textdetect;

import android.os.Environment;

import com.googlecode.leptonica.android.Pix;
import com.googlecode.leptonica.android.Pixa;

/**
 * @author alanv@google.com (Alan Viverette)
 */
public class HydrogenTextDetector {
    private final long mNative;

    static {
        System.loadLibrary("lept");
        System.loadLibrary("hydrogen");
    }

    private Parameters mParams;

    public HydrogenTextDetector() {
        mNative = nativeConstructor();

        mParams = new Parameters();
        setParameters(mParams);
    }

    public void setSize(int width, int height) {
        // TODO(alanv): Set up native buffers
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            nativeDestructor(mNative);
        } finally {
            super.finalize();
        }
    }

    public void setParameters(Parameters params) {
        mParams = params;

        nativeSetParameters(mNative, mParams);
    }

    public Parameters getParameters() {
        return mParams;
    }

    public Pixa getTextAreas() {
        long nativePixa = nativeGetTextAreas(mNative);

        if (nativePixa == 0) {
            return null;
        }

        int width = nativeGetSourceWidth(mNative);
        int height = nativeGetSourceHeight(mNative);

        return new Pixa(nativePixa, width, height);
    }

    public float getSkewAngle() {
        return nativeGetSkewAngle(mNative);
    }

    public float[] getTextConfs() {
        return nativeGetTextConfs(mNative);
    }

    public Pix getSourceImage() {
        long nativePix = nativeGetSourceImage(mNative);

        if (nativePix == 0) {
            return null;
        }

        return new Pix(nativePix);
    }

    /**
     * Sets the text detection source image to be a clone of the supplied source
     * image. The supplied image may be recycled after calling this method.
     *
     * @param pixs The source image on which to perform text detection.
     */
    public void setSourceImage(Pix pixs) {
        nativeSetSourceImage(mNative, pixs.getNativePix());
    }

    public void detectText() {
        nativeDetectText(mNative);
    }

    public void clear() {
        nativeClear(mNative);
    }

    // ******************
    // * PUBLIC CLASSES *
    // ******************

    public class Parameters {
        public boolean debug;

        public String out_dir;

        // Edge-based thresholding
        public int edge_tile_x;

        public int edge_tile_y;

        public int edge_thresh;

        public int edge_avg_thresh;

        // Skew angle correction
        public boolean skew_enabled;

        public float skew_min_angle;

        public float skew_sweep_range;

        public float skew_sweep_delta;

        public int skew_sweep_reduction;

        public int skew_search_reduction;

        public float skew_search_min_delta;

        // Singleton filter
        public float single_min_aspect;

        public float single_max_aspect;

        public int single_min_area;

        public float single_min_density;

        // Quick pair filter
        public float pair_h_ratio;

        public float pair_d_ratio;

        public float pair_h_dist_ratio;

        public float pair_v_dist_ratio;

        public float pair_h_shared;

        // Cluster pair filter
        public int cluster_width_spacing;

        public float cluster_shared_edge;

        public float cluster_h_ratio;

        // Finalized cluster filter
        public int cluster_min_blobs;

        public float cluster_min_aspect;

        public float cluster_min_fdr;

        public int cluster_min_edge;

        public int cluster_min_edge_avg;

        public Parameters() {
            debug = false;
            out_dir = Environment.getExternalStorageDirectory().toString();

            // Edge-based thresholding
            edge_tile_x = 32;
            edge_tile_y = 64;
            edge_thresh = 64;
            edge_avg_thresh = 4;

            // Skew angle correction
            skew_enabled = true;
            skew_min_angle = 1.0f;
            skew_sweep_range = 30.0f;
            skew_sweep_delta = 5.0f;
            skew_sweep_reduction = 8;
            skew_search_reduction = 4;
            skew_search_min_delta = 0.01f;

            // Singleton filter
            single_min_aspect = 0.1f;
            single_max_aspect = 4.0f;
            single_min_area = 4;
            single_min_density = 0.2f;

            // Quick pair filter
            pair_h_ratio = 1.0f;
            pair_d_ratio = 1.5f;
            pair_h_dist_ratio = 2.0f;
            pair_v_dist_ratio = 0.25f;
            pair_h_shared = 0.25f;

            // Cluster pair filter
            cluster_width_spacing = 2;
            cluster_shared_edge = 0.5f;
            cluster_h_ratio = 1.0f;

            // Finalized cluster filter
            cluster_min_blobs = 5;
            cluster_min_aspect = 2;
            cluster_min_fdr = 2.5f;
            cluster_min_edge = 32;
            cluster_min_edge_avg = 1;
        }
    }

    // ******************
    // * NATIVE METHODS *
    // ******************

    private static native long nativeConstructor();

    private static native void nativeDestructor(long nativePtr);

    private static native void nativeSetParameters(long nativePtr, Parameters params);

    private static native long nativeGetTextAreas(long nativePtr);

    private static native float nativeGetSkewAngle(long nativePtr);

    private static native int nativeGetSourceWidth(long nativePtr);

    private static native int nativeGetSourceHeight(long nativePtr);

    private static native float[] nativeGetTextConfs(long nativePtr);

    private static native long nativeGetSourceImage(long nativePtr);

    private static native int nativeSetSourceImage(long nativePtr, long nativePix);

    private static native void nativeDetectText(long nativePtr);

    private static native void nativeClear(long nativePtr);
}
