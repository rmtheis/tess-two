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

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.test.suitebuilder.annotation.SmallTest;

import com.googlecode.leptonica.android.JpegIO;
import com.googlecode.leptonica.android.Pix;
import com.googlecode.leptonica.android.ReadFile;

import junit.framework.TestCase;

public class JpegIOTest extends TestCase {
    @SmallTest
    public void testCompressToJpeg() {
        testCompressToJpeg(640, 480, 85, true);
        testCompressToJpeg(640, 480, 85, false);
    }
    
    private void testCompressToJpeg(int width, int height, int quality, boolean progressive) {
        Bitmap bmps = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        Pix pixs = ReadFile.readBitmap(bmps);
        byte[] encodedBytes = JpegIO.compressToJpeg(pixs, quality, progressive);
        Bitmap bmpd = BitmapFactory.decodeByteArray(encodedBytes, 0, encodedBytes.length);
        
        assertEquals(bmps.getWidth(), bmpd.getWidth());
        assertEquals(bmps.getHeight(), bmpd.getHeight());
        
        bmps.recycle();
        pixs.recycle();
        encodedBytes = null;
        bmpd.recycle();
    }
}
