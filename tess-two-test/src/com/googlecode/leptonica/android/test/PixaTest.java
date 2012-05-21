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

package com.googlecode.leptonica.android.test;

import android.test.suitebuilder.annotation.SmallTest;

import com.googlecode.leptonica.android.Box;
import com.googlecode.leptonica.android.Constants;
import com.googlecode.leptonica.android.Pix;
import com.googlecode.leptonica.android.Pixa;

import junit.framework.TestCase;

public class PixaTest extends TestCase {
    @SmallTest
    public void testPixaCreate() {
        internalTestPixaCreate(0, 0, 0);
        internalTestPixaCreate(1, 0, 0);
        internalTestPixaCreate(0, 640, 480);
        internalTestPixaCreate(5, 640, 480);
    }

    private void internalTestPixaCreate(int initialCapacity, int width, int height) {
        Pixa pixa = Pixa.createPixa(initialCapacity, width, height);

        // Make sure the dimensions were set correctly.
        assertEquals(0, pixa.size());
        assertEquals(width, pixa.getWidth());
        assertEquals(height, pixa.getHeight());

        // Fill the Pixa to capacity.
        for (int i = 0; i < initialCapacity; i++) {
            addBlockToPixa(pixa, 0, 0, 640, 480, 8);
        }

        // Make sure the size is reflected correctly.
        assertEquals(initialCapacity, pixa.size());

        // Make sure we can recycle the Pixa.
        pixa.recycle();
    }

    @SmallTest
    public void testPixaCopy() {
        internalTestPixaCopy(0, 0, 0);
        internalTestPixaCopy(5, 0, 0);
        internalTestPixaCopy(0, 640, 480);
        internalTestPixaCopy(5, 640, 480);
    }

    private void internalTestPixaCopy(int initialCapacity, int width, int height) {
        Pixa pixa = Pixa.createPixa(initialCapacity, width, height);

        // Fill the Pixa to capacity.
        for (int i = 0; i < initialCapacity; i++) {
            addBlockToPixa(pixa, 0, 0, 640, 640, 8);
        }

        // Create a shallow copy of the Pixa.
        Pixa pixaCopy = pixa.copy();

        // Add a new Pix to the copy.
        addBlockToPixa(pixaCopy, 0, 0, 640, 640, 8);

        // Ensure that both copies changed size.
        assertEquals(initialCapacity + 1, pixaCopy.size());
        assertEquals(pixaCopy.size(), pixa.size());

        // Finally, we should be able to recycle both Pixa.
        pixa.recycle();
        pixaCopy.recycle();
    }

    @SmallTest
    public void testPixaSort() {
        Pixa pixa = Pixa.createPixa(0);

        // Add contained Pix in arbitrary order.
        addBlockToPixa(pixa, 0, 0, 640, 640, 8);
        addBlockToPixa(pixa, 160, 160, 64, 64, 8);
        addBlockToPixa(pixa, 32, 32, 320, 320, 8);
        addBlockToPixa(pixa, 64, 64, 160, 160, 8);
        addBlockToPixa(pixa, 320, 320, 32, 32, 8);

        // Sort by increasing height.
        Pixa pixaSorted = pixa.sort(Constants.L_SORT_BY_HEIGHT, Constants.L_SORT_INCREASING);

        // Ensure sort was successful.
        int[] currentDimensions = new int[4];
        int previousHeight = -1;

        for (int i = 0; i < pixa.size(); i++) {
            assertTrue(pixaSorted.getBoxGeometry(i, currentDimensions));
            int currentHeight = currentDimensions[Box.INDEX_H];
            assertTrue(currentHeight > previousHeight);
            previousHeight = currentHeight;
        }

        pixa.recycle();
        pixaSorted.recycle();
    }

    @SmallTest
    public void testPixaJoin() {
        internalTestPixaJoin(0, 0);
        internalTestPixaJoin(1, 0);
        internalTestPixaJoin(0, 1);
        internalTestPixaJoin(1, 1);
    }

    private void internalTestPixaJoin(int sizeA, int sizeB) {
        Pixa pixaA = Pixa.createPixa(0);
        Pixa pixaB = Pixa.createPixa(0);

        // Populate both Pixa.
        for (int i = 0; i < sizeA; i++) {
            addBlockToPixa(pixaA, 0, 0, 640, 640, 8);
        }

        for (int i = 0; i < sizeB; i++) {
            addBlockToPixa(pixaB, 0, 0, 640, 640, 8);
        }

        // Join pixaB into pixaA.
        pixaA.join(pixaB);

        // Ensure the join was successful.
        assertEquals(pixaA.size(), sizeA + sizeB);
        assertEquals(pixaB.size(), sizeB);

        pixaA.recycle();
        pixaB.recycle();
    }

    @SmallTest
    public void textPixaReplacePix() {
        Pixa pixa = Pixa.createPixa(0, 640, 480);

        // Populate the Pixa.
        addBlockToPixa(pixa, 0, 0, 640, 480, 8);

        Pix pix = new Pix(320, 240, 8);
        Box box = new Box(320, 240, 320, 240);

        // Replace the existing Pix.
        pixa.replacePix(0, pix, box);

        // Ensure the replacement was successful.
        Pix returnedPix = pixa.getPix(0);
        Box returnedBox = pixa.getBox(0);

        assertEquals(pix.getWidth(), returnedPix.getWidth());
        assertEquals(pix.getHeight(), returnedPix.getHeight());
        assertEquals(pix.getDepth(), returnedPix.getDepth());

        assertEquals(box.getX(), returnedBox.getX());
        assertEquals(box.getY(), returnedBox.getY());
        assertEquals(box.getWidth(), returnedBox.getWidth());
        assertEquals(box.getHeight(), returnedBox.getHeight());

        pix.recycle();
        box.recycle();
        returnedPix.recycle();
        returnedBox.recycle();
        pixa.recycle();
    }

    @SmallTest
    public void textPixaMergeAndReplacePix() {
        Pixa pixa = Pixa.createPixa(0, 640, 480);

        // Populate the Pixa.
        addBlockToPixa(pixa, 0, 0, 320, 240, 8);
        addBlockToPixa(pixa, 320, 240, 320, 240, 8);

        // Merge both Pix, removing the second Pix.
        pixa.mergeAndReplacePix(0, 1);

        // Ensure the merge was successful.
        Pix pix = pixa.getPix(0);
        Box box = pixa.getBox(0);

        assertEquals(pixa.size(), 1);

        assertEquals(pix.getWidth(), 640);
        assertEquals(pix.getHeight(), 480);
        assertEquals(pix.getDepth(), 8);

        assertEquals(box.getX(), 0);
        assertEquals(box.getY(), 0);
        assertEquals(box.getWidth(), 640);
        assertEquals(box.getHeight(), 480);

        pix.recycle();
        box.recycle();
        pixa.recycle();
    }

    /**
     * Adds a block to the specified Pixa.
     *
     * @param pixa
     * @param x X-coordinate of the top-left corner of the block.
     * @param y Y-coordinate of the top-left corner of the block.
     * @param width Width of the block.
     * @param height Height of the block.
     * @param depth Bit-depth of the block.
     */
    private static void addBlockToPixa(Pixa pixa, int x, int y, int width, int height, int depth) {
        final Pix pix = new Pix(width, height, depth);
        final Box box = new Box(x, y, width, height);

        pixa.add(pix, box, Constants.L_COPY);

        pix.recycle();
        box.recycle();
    }
}
