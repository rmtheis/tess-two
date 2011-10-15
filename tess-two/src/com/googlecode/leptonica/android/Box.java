/*
 * Copyright (C) 2010 Google Inc.
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

package com.googlecode.leptonica.android;

/**
 * Wrapper for Leptonica's native BOX.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class Box {
    static {
        System.loadLibrary("lept");
    }

    /** The index of the X coordinate within the geometry array. */
    public static final int INDEX_X = 0;

    /** The index of the Y coordinate within the geometry array. */
    public static final int INDEX_Y = 1;

    /** The index of the width within the geometry array. */
    public static final int INDEX_W = 2;

    /** The index of the height within the geometry array. */
    public static final int INDEX_H = 3;

    /**
     * A pointer to the native Box object. This is used internally by native
     * code.
     */
    final int mNativeBox;

    private boolean mRecycled = false;

    /**
     * Creates a new Box wrapper for the specified native BOX.
     *
     * @param nativeBox A pointer to the native BOX.
     */
    Box(int nativeBox) {
        mNativeBox = nativeBox;
        mRecycled = false;
    }

    /**
     * Creates a box with the specified geometry. All dimensions should be
     * non-negative and specified in pixels.
     *
     * @param x X-coordinate of the top-left corner of the box.
     * @param y Y-coordinate of the top-left corner of the box.
     * @param w Width of the box.
     * @param h Height of the box.
     */
    public Box(int x, int y, int w, int h) {
        if (x < 0 || y < 0 || w < 0 || h < 0) {
            throw new IllegalArgumentException("All box dimensions must be non-negative");
        }
        
        int nativeBox = nativeCreate(x, y, w, h);

        if (nativeBox == 0) {
            throw new OutOfMemoryError();
        }
        
        mNativeBox = nativeBox;
        mRecycled = false;
    }
    
    /**
     * Returns the box's x-coordinate in pixels.
     * 
     * @return The box's x-coordinate in pixels.
     */
    public int getX() {
        return nativeGetX(mNativeBox);
    }

    /**
     * Returns the box's y-coordinate in pixels.
     * 
     * @return The box's y-coordinate in pixels.
     */
    public int getY() {
        return nativeGetY(mNativeBox);
    }

    /**
     * Returns the box's width in pixels.
     * 
     * @return The box's width in pixels.
     */
    public int getWidth() {
        return nativeGetWidth(mNativeBox);
    }

    /**
     * Returns the box's height in pixels.
     * 
     * @return The box's height in pixels.
     */
    public int getHeight() {
        return nativeGetHeight(mNativeBox);
    }

    /**
     * Returns an array containing the coordinates of this box. See INDEX_*
     * constants for indices.
     *
     * @return an array of box oordinates
     */
    public int[] getGeometry() {
        int[] geometry = new int[4];

        if (getGeometry(geometry)) {
            return geometry;
        }

        return null;
    }

    /**
     * Fills an array containing the coordinates of this box. See INDEX_*
     * constants for indices.
     *
     * @param geometry A 4+ element integer array to fill with coordinates.
     * @return <code>true</code> on success
     */
    public boolean getGeometry(int[] geometry) {
        if (geometry.length < 4) {
            throw new IllegalArgumentException("Geometry array must be at least 4 elements long");
        }

        return nativeGetGeometry(mNativeBox, geometry);
    }

    /**
     * Releases resources and frees any memory associated with this Box.
     */
    public void recycle() {
        if (!mRecycled) {
            nativeDestroy(mNativeBox);

            mRecycled = true;
        }
    }

    @Override
    protected void finalize() throws Throwable {
        recycle();

        super.finalize();
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native int nativeCreate(int x, int y, int w, int h);
    private static native int nativeGetX(int nativeBox);
    private static native int nativeGetY(int nativeBox);
    private static native int nativeGetWidth(int nativeBox);
    private static native int nativeGetHeight(int nativeBox);
    private static native void nativeDestroy(int nativeBox);
    private static native boolean nativeGetGeometry(int nativeBox, int[] geometry);
}
