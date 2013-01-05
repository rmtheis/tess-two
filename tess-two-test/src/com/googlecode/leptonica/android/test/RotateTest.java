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

import com.googlecode.leptonica.android.Pix;
import com.googlecode.leptonica.android.ReadFile;
import com.googlecode.leptonica.android.Rotate;
import com.googlecode.leptonica.android.WriteFile;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.test.suitebuilder.annotation.SmallTest;

import junit.framework.TestCase;

public class RotateTest extends TestCase {
    @SmallTest
    public void testRotate() {
        Bitmap bmp = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bmp);
        Paint paint = new Paint();

        // Paint the background white
        canvas.drawColor(Color.WHITE);

        // Paint a black circle in the center
        paint.setColor(Color.BLACK);
        paint.setStyle(Style.FILL);
        canvas.drawCircle(50, 50, 10, paint);

        Pix pixs = ReadFile.readBitmap(bmp);
        Pix pixd = Rotate.rotate(pixs, 180);
        pixs.recycle();

        Bitmap rotated = WriteFile.writeBitmap(pixd);
        pixd.recycle();

        float match = TestUtils.compareBitmaps(bmp, rotated);
        bmp.recycle();
        rotated.recycle();

        assertTrue("Bitmaps match", (match > 0.99f));
    }

    @SmallTest
    public void testRotateResize() {
        Bitmap bmp = Bitmap.createBitmap(100, 10, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bmp);
        Paint paint = new Paint();

        // Paint the background white
        canvas.drawColor(Color.BLACK);

        // Paint a black circle in the center
        paint.setColor(Color.BLACK);
        paint.setStyle(Style.FILL);
        canvas.drawCircle(50, 50, 10, paint);

        Pix pixs = ReadFile.readBitmap(bmp);
        Pix pixd = Rotate.rotate(pixs, 180);
        pixs.recycle();

        assertTrue("Rotated width is 100", (pixd.getWidth() == 100));
        pixd.recycle();
    }
}