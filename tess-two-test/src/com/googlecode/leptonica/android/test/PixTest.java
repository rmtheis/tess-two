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

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import junit.framework.TestCase;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.Color;
import android.test.suitebuilder.annotation.SmallTest;

import com.googlecode.leptonica.android.Pix;
import com.googlecode.leptonica.android.ReadFile;

public class PixTest extends TestCase {
    @SmallTest
    public void testGetData() throws IOException {
        File file = File.createTempFile("testGetData", ".jpg");
        FileOutputStream fileStream = new FileOutputStream(file);
        Bitmap bmp = Bitmap.createBitmap(640, 480, Bitmap.Config.RGB_565);
        bmp.compress(CompressFormat.JPEG, 85, fileStream);
        Pix pix = ReadFile.readFile(file);
        assertNotNull(pix);
        byte[] pixData = pix.getData();
        assertNotNull(pixData);

        bmp.recycle();
        pix.recycle();
    }

    @SmallTest
    public void testGetDimensions() {
        int width = 640;
        int height = 480;
        int depth = 32;
        Pix pix = new Pix(width, height, depth);

        int[] dimens = pix.getDimensions();

        assertEquals("Incorrect width value found.", width,
                dimens[Pix.INDEX_W]);
        assertEquals("Incorrect height value found.", height,
                dimens[Pix.INDEX_H]);
        assertEquals("Incorrect bit-depth value found.", depth,
                dimens[Pix.INDEX_D]);
    }

    @SmallTest
    public void testPixClone() {
        Pix pix = new Pix(640, 480, 32);
        Pix pixCopy = pix.clone();

        // The clone should not have the same native pointer.
        assertNotSame(pix.getNativePix(), pixCopy.getNativePix());

        // The clone should share the same backing data.
        pix.setPixel(0, 0, Color.RED);
        assertEquals(Color.RED, pixCopy.getPixel(0, 0));

        // Finally, we should be able to recycle both Pix.
        pix.recycle();
        pixCopy.recycle();
    }

    @SmallTest
    public void testPixCreate() {
        testPixCreate(1, 1, 1);
        testPixCreate(640, 480, 32);
    }

    private void testPixCreate(int w, int h, int d) {
        Pix pix = new Pix(w, h, d);

        // Make sure the dimensions were set correctly.
        assertEquals(w, pix.getWidth());
        assertEquals(h, pix.getHeight());
        assertEquals(d, pix.getDepth());

        // Make sure we can recycle the Pix.
        pix.recycle();
    }

    @SmallTest
    public void testPixPixelOps() {
        Pix pix = new Pix(640, 480, 32);

        // Set various pixel colors.
        pix.setPixel(0, 0, Color.RED);
        pix.setPixel(1, 0, Color.BLUE);
        pix.setPixel(2, 0, Color.GREEN);
        pix.setPixel(3, 0, Color.BLACK);
        pix.setPixel(4, 0, Color.WHITE);

        // Make sure the pixel was set and retrieved correctly.
        assertEquals(Color.RED, pix.getPixel(0, 0));
        assertEquals(Color.BLUE, pix.getPixel(1, 0));
        assertEquals(Color.GREEN, pix.getPixel(2, 0));
        assertEquals(Color.BLACK, pix.getPixel(3, 0));
        assertEquals(Color.WHITE, pix.getPixel(4, 0));

        pix.recycle();
    }
}
