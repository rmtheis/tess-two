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

package com.googlecode.leptonica.android;

import android.graphics.Rect;

import java.io.File;
import java.util.ArrayList;
import java.util.Iterator;

/**
 * Java representation of a native PIXA object. This object contains multiple
 * PIX objects and their associated bounding BOX objects.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class Pixa implements Iterable<Pix> {
    static {
        System.loadLibrary("lept");
    }

    /** A pointer to the native PIXA object. This is used internally by native code. */
    final int mNativePixa;

    /** The specified width of this Pixa. */
    final int mWidth;

    /** The specified height of this Pixa. */
    final int mHeight;

    private boolean mRecycled;

    /**
     * Creates a new Pixa with the specified minimum capacity. The Pixa will
     * expand automatically as new Pix are added.
     *
     * @param size The minimum capacity of this Pixa.
     * @return a new Pixa or <code>null</code> on error
     */
    public static Pixa createPixa(int size) {
        return createPixa(size, 0, 0);
    }

    /**
     * Creates a new Pixa with the specified minimum capacity. The Pixa will
     * expand automatically as new Pix are added.
     * <p>
     * If non-zero, the specified width and height will be used to specify the
     * bounds of output images. *
     *
     * @param size The minimum capacity of this Pixa.
     * @param width (Optional) The width of this Pixa, use 0 for default.
     * @param height (Optional) The height of this Pixa, use 0 for default.
     * @return a new Pixa or <code>null</code> on error
     */
    public static Pixa createPixa(int size, int width, int height) {
        int nativePixa = nativeCreate(size);

        if (nativePixa == 0) {
            throw new OutOfMemoryError();
        }

        return new Pixa(nativePixa, width, height);
    }

    /**
     * Creates a wrapper for the specified native Pixa pointer.
     *
     * @param nativePixa Native pointer to a PIXA object.
     * @param width The width of the PIXA.
     * @param height The height of the PIXA.
     */
    public Pixa(int nativePixa, int width, int height) {
        mNativePixa = nativePixa;
        mWidth = width;
        mHeight = height;
        mRecycled = false;
    }

    /**
     * Returns a pointer to the native PIXA object. This is used by native code.
     *
     * @return a pointer to the native PIXA object
     */
    public int getNativePixa() {
        return mNativePixa;
    }

    /**
     * Creates a shallow copy of this Pixa. Contained Pix are cloned, and the
     * resulting Pixa may be recycled separately from the original.
     *
     * @return a shallow copy of this Pixa
     */
    public Pixa copy() {
        int nativePixa = nativeCopy(mNativePixa);

        if (nativePixa == 0) {
            throw new OutOfMemoryError();
        }

        return new Pixa(nativePixa, mWidth, mHeight);
    }

    /**
     * Sorts this Pixa using the specified field and order. See
     * Constants.L_SORT_BY_* and Constants.L_SORT_INCREASING or
     * Constants.L_SORT_DECREASING.
     *
     * @param field The field to sort by. See Constants.L_SORT_BY_*.
     * @param order The order in which to sort. Must be either
     *            Constants.L_SORT_INCREASING or Constants.L_SORT_DECREASING.
     * @return a sorted copy of this Pixa
     */
    public Pixa sort(int field, int order) {
        int nativePixa = nativeSort(mNativePixa, field, order);

        if (nativePixa == 0) {
            throw new OutOfMemoryError();
        }

        return new Pixa(nativePixa, mWidth, mHeight);
    }

    /**
     * Returns the number of elements in this Pixa.
     *
     * @return the number of elements in this Pixa
     */
    public int size() {
        return nativeGetCount(mNativePixa);
    }

    /**
     * Recycles this Pixa and frees natively allocated memory. You may not
     * access or modify the Pixa after calling this method.
     * <p>
     * Any Pix obtained from this Pixa or copies of this Pixa will still be
     * accessible until they are explicitly recycled or finalized by the garbage
     * collector.
     */
    public synchronized void recycle() {
        if (!mRecycled) {
            nativeDestroy(mNativePixa);

            mRecycled = true;
        }
    }

    @Override
    protected void finalize() throws Throwable {
        recycle();

        super.finalize();
    }

    /**
     * Merges the contents of another Pixa into this one.
     *
     * @param otherPixa
     * @return <code>true</code> on success
     */
    public boolean join(Pixa otherPixa) {
        return nativeJoin(mNativePixa, otherPixa.mNativePixa);
    }

    /**
     * Adds a Pix to this Pixa.
     *
     * @param pix The Pix to add.
     * @param mode The mode in which to add this Pix, typically
     *            Constants.L_CLONE.
     */
    public void addPix(Pix pix, int mode) {
        nativeAddPix(mNativePixa, pix.mNativePix, mode);
    }

    /**
     * Adds a Box to this Pixa.
     *
     * @param box The Box to add.
     * @param mode The mode in which to add this Box, typically
     *            Constants.L_CLONE.
     */
    public void addBox(Box box, int mode) {
        nativeAddBox(mNativePixa, box.mNativeBox, mode);
    }

    /**
     * Adds a Pix and associated Box to this Pixa.
     *
     * @param pix The Pix to add.
     * @param box The Box to add.
     * @param mode The mode in which to add this Pix and Box, typically
     *            Constants.L_CLONE.
     */
    public void add(Pix pix, Box box, int mode) {
        nativeAdd(mNativePixa, pix.mNativePix, box.mNativeBox, mode);
    }

    /**
     * Returns the Box at the specified index, or <code>null</code> on error.
     *
     * @param index The index of the Box to return.
     * @return the Box at the specified index, or <code>null</code> on error
     */
    public Box getBox(int index) {
        int nativeBox = nativeGetBox(mNativePixa, index);

        if (nativeBox == 0) {
            return null;
        }

        return new Box(nativeBox);
    }

    /**
     * Returns the Pix at the specified index, or <code>null</code> on error.
     *
     * @param index The index of the Pix to return.
     * @return the Pix at the specified index, or <code>null</code> on error
     */
    public Pix getPix(int index) {
        int nativePix = nativeGetPix(mNativePixa, index);

        if (nativePix == 0) {
            return null;
        }

        return new Pix(nativePix);
    }

    /**
     * Returns the width of this Pixa, or 0 if one was not set when it was
     * created.
     *
     * @return the width of this Pixa, or 0 if one was not set when it was
     *         created
     */
    public int getWidth() {
        return mWidth;
    }

    /**
     * Returns the height of this Pixa, or 0 if one was not set when it was
     * created.
     *
     * @return the height of this Pixa, or 0 if one was not set when it was
     *         created
     */
    public int getHeight() {
        return mHeight;
    }

    /**
     * Returns a bounding Rect for this Pixa, which may be (0,0,0,0) if width
     * and height were not specified on creation.
     *
     * @return a bounding Rect for this Pixa
     */
    public Rect getRect() {
        return new Rect(0, 0, mWidth, mHeight);
    }

    /**
     * Returns a bounding Rect for the Box at the specified index.
     *
     * @param index The index of the Box to get the bounding Rect of.
     * @return a bounding Rect for the Box at the specified index
     */
    public Rect getBoxRect(int index) {
        int[] dimensions = getBoxGeometry(index);

        if (dimensions == null) {
            return null;
        }

        int x = dimensions[Box.INDEX_X];
        int y = dimensions[Box.INDEX_Y];
        int w = dimensions[Box.INDEX_W];
        int h = dimensions[Box.INDEX_H];

        Rect bound = new Rect(x, y, x + w, y + h);

        return bound;
    }

    /**
     * Returns a geometry array for the Box at the specified index. See
     * Box.INDEX_* for indices.
     *
     * @param index The index of the Box to get the geometry of.
     * @return a bounding Rect for the Box at the specified index
     */
    public int[] getBoxGeometry(int index) {
        int[] dimensions = new int[4];

        if (getBoxGeometry(index, dimensions)) {
            return dimensions;
        }

        return null;
    }

    /**
     * Fills an array with the geometry of the Box at the specified index. See
     * Box.INDEX_* for indices.
     *
     * @param index The index of the Box to get the geometry of.
     * @param dimensions The array to fill with Box geometry. Must be at least 4
     *            elements.
     * @return <code>true</code> on success
     */
    public boolean getBoxGeometry(int index, int[] dimensions) {
        return nativeGetBoxGeometry(mNativePixa, index, dimensions);
    }

    /**
     * Returns an ArrayList of Box bounding Rects.
     *
     * @return an ArrayList of Box bounding Rects
     */
    public ArrayList<Rect> getBoxRects() {
        final int pixaCount = nativeGetCount(mNativePixa);
        final int[] buffer = new int[4];
        final ArrayList<Rect> rects = new ArrayList<Rect>(pixaCount);

        for (int i = 0; i < pixaCount; i++) {
            getBoxGeometry(i, buffer);

            final int x = buffer[Box.INDEX_X];
            final int y = buffer[Box.INDEX_Y];
            final Rect bound = new Rect(x, y, x + buffer[Box.INDEX_W], y + buffer[Box.INDEX_H]);

            rects.add(bound);
        }

        return rects;
    }

    /**
     * Replaces the Pix and Box at the specified index with the specified Pix
     * and Box, both of which may be recycled after calling this method.
     *
     * @param index The index of the Pix to replace.
     * @param pix The Pix to replace the existing Pix.
     * @param box The Box to replace the existing Box.
     */
    public void replacePix(int index, Pix pix, Box box) {
        nativeReplacePix(mNativePixa, index, pix.mNativePix, box.mNativeBox);
    }

    /**
     * Merges the Pix at the specified indices and removes the Pix at the second
     * index.
     *
     * @param indexA The index of the first Pix.
     * @param indexB The index of the second Pix, which will be removed after
     *            merging.
     */
    public void mergeAndReplacePix(int indexA, int indexB) {
        nativeMergeAndReplacePix(mNativePixa, indexA, indexB);
    }

    /**
     * Writes the components of this Pix to a bitmap-formatted file using a
     * random color map.
     *
     * @param file The file to write to.
     * @return <code>true</code> on success
     */
    public boolean writeToFileRandomCmap(File file) {
        return nativeWriteToFileRandomCmap(mNativePixa, file.getAbsolutePath(), mWidth, mHeight);
    }

    public Iterator<Pix> iterator() {
        return new PixIterator();
    }

    private class PixIterator implements Iterator<Pix> {
        private int mIndex;

        private PixIterator() {
            mIndex = 0;
        }

        public boolean hasNext() {
            final int size = size();
            return (size > 0 && mIndex < size);
        }

        public Pix next() {
            return getPix(mIndex++);
        }

        public void remove() {
            throw new UnsupportedOperationException();
        }
    }

    // ***************
    // * NATIVE CODE *
    // ***************

    private static native int nativeCreate(int size);

    private static native int nativeCopy(int nativePixa);

    private static native int nativeSort(int nativePixa, int field, int order);

    private static native boolean nativeJoin(int nativePixa, int otherPixa);

    private static native int nativeGetCount(int nativePixa);

    private static native void nativeDestroy(int nativePixa);

    private static native void nativeAddPix(int nativePixa, int nativePix, int mode);

    private static native void nativeAddBox(int nativePixa, int nativeBox, int mode);

    private static native void nativeAdd(int nativePixa, int nativePix, int nativeBox, int mode);

    private static native boolean nativeWriteToFileRandomCmap(
            int nativePixa, String fileName, int width, int height);

    private static native void nativeReplacePix(
            int nativePixa, int index, int nativePix, int nativeBox);

    private static native void nativeMergeAndReplacePix(int nativePixa, int indexA, int indexB);

    private static native int nativeGetBox(int nativePix, int index);

    private static native int nativeGetPix(int nativePix, int index);

    private static native boolean nativeGetBoxGeometry(int nativePixa, int index, int[] dimensions);
}
