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

import junit.framework.TestCase;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.Paint.Style;
import android.test.suitebuilder.annotation.SmallTest;

import com.googlecode.leptonica.android.Convert;
import com.googlecode.leptonica.android.GrayQuant;
import com.googlecode.leptonica.android.Pix;
import com.googlecode.leptonica.android.ReadFile;
import com.googlecode.leptonica.android.Skew;

public class SkewTest extends TestCase {

    private static final String SENTENCE = "The quick brown fox jumps over the lazy dog.";

    @SmallTest
    public void testFindSkew() {
        testFindSkew(SENTENCE, 640, 480, -15.0f);
        testFindSkew(SENTENCE, 640, 480, 0.0f);
        testFindSkew(SENTENCE, 640, 480, 15.0f);
    }

    private void testFindSkew(String text, int width, int height, float skew) {
        Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        Paint paint = new Paint();
        Canvas canvas = new Canvas(bmp);

        paint.setColor(Color.BLACK);
        paint.setStyle(Style.FILL);
        paint.setAntiAlias(true);
        paint.setTextAlign(Align.CENTER);
        paint.setTextSize(32.0f);

        canvas.drawColor(Color.WHITE);
        canvas.rotate(skew, width / 2, height / 2);
        canvas.drawText(text, width / 2, height / 2 , paint);

        Pix pixs = ReadFile.readBitmap(bmp);

        assertNotNull(pixs);

        Pix pixd;
        //noinspection ConstantConditions
        if (pixs.getDepth() != 4 || pixs.getDepth() != 8) {
            Pix pix8 = Convert.convertTo8(pixs);
            pixd = GrayQuant.pixThresholdToBinary(pix8, 1);
            pix8.recycle();
        } else {
            pixd = GrayQuant.pixThresholdToBinary(pixs, 1);
        }

        float measuredSkew = -Skew.findSkew(pixd);
        float tol = 1f;
        boolean isInRange = skew - tol < measuredSkew && measuredSkew < skew + tol;
        assertTrue("Skew has incorrect value.", isInRange);

        pixs.recycle();
        pixd.recycle();
        bmp.recycle();
    }
}
