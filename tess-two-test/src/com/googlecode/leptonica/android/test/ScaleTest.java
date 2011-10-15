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
import android.test.suitebuilder.annotation.SmallTest;

import com.googlecode.leptonica.android.Pix;
import com.googlecode.leptonica.android.ReadFile;
import com.googlecode.leptonica.android.Scale;

import junit.framework.TestCase;

public class ScaleTest extends TestCase {
    @SmallTest
    public void testScale() {
        testScale(640, 480, 1.0f, 1.0f);
        testScale(640, 480, 0.5f, 0.25f);
    }
    
    private void testScale(int inputWidth, int inputHeight, float scaleX, float scaleY) {
        Bitmap bmp = Bitmap.createBitmap(inputWidth, inputHeight, Bitmap.Config.ARGB_8888);
        Pix pixs = ReadFile.readBitmap(bmp);
        Pix pixd = Scale.scale(pixs, scaleX, scaleY);
        
        assertEquals((int) (inputWidth * scaleX), pixd.getWidth());
        assertEquals((int) (inputHeight * scaleY), pixd.getHeight());
        
        bmp.recycle();
        pixs.recycle();
        pixd.recycle();
    }
}
