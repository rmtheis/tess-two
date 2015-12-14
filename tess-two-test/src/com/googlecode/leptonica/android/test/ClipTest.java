/*
 * Copyright 2014 Robert Theis
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

import junit.framework.TestCase;
import android.test.suitebuilder.annotation.SmallTest;

import com.googlecode.leptonica.android.Box;
import com.googlecode.leptonica.android.Clip;
import com.googlecode.leptonica.android.Pix;

public class ClipTest extends TestCase {
    @SmallTest
    public void testClipRectangle() {
        final int newWidth = 100;
        final int newHeight = 75;

        Pix pix = new Pix(640, 480, 8);
        Box clippingBox = new Box(50, 50, newWidth, newHeight);
        Pix clippedPix = Clip.clipRectangle(pix, clippingBox);
        clippingBox.recycle();

        assertNotNull(clippedPix);

        // The clipped pix should not have the same native pointer.
        assertNotSame(pix.getNativePix(), clippedPix.getNativePix());

        // The clipped pix should have the correct size.
        assertTrue("Clipped pix has incorrect width.", clippedPix.getWidth() == newWidth);
        assertTrue("Clipped pix has incorrect height.", clippedPix.getHeight() == newHeight);

        // We should be able to recycle both Pix.
        pix.recycle();
        clippedPix.recycle();
    }
}
