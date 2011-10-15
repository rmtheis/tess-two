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

import android.graphics.Color;
import android.test.suitebuilder.annotation.SmallTest;

import com.googlecode.leptonica.android.Convert;
import com.googlecode.leptonica.android.Pix;

import junit.framework.TestCase;

public class ConvertTest extends TestCase {
    @SmallTest
    public void testConvertTo8() {
        Pix pixs = new Pix(640, 480, 32);
        pixs.setPixel(0, 0, Color.RED);
        pixs.setPixel(1, 0, Color.GREEN);
        pixs.setPixel(2, 0, Color.BLUE);
        pixs.setPixel(3, 0, Color.WHITE);
        pixs.setPixel(4, 0, Color.BLACK);
        
        Pix pixd = Convert.convertTo8(pixs);
        
        assertNotSame(Color.RED, pixd.getPixel(0, 0));
        assertNotSame(Color.GREEN, pixd.getPixel(1, 0));
        assertNotSame(Color.BLUE, pixd.getPixel(2, 0));
        assertEquals(Color.WHITE, pixd.getPixel(3, 0));
        assertEquals(Color.BLACK, pixd.getPixel(4, 0));
        
        pixs.recycle();
        pixd.recycle();
    }
}
