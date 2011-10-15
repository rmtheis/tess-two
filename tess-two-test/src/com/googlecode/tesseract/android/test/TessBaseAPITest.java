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

package com.googlecode.tesseract.android.test;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.Paint.Style;
import android.graphics.Rect;
import android.test.suitebuilder.annotation.SmallTest;

import com.googlecode.tesseract.android.TessBaseAPI;

import junit.framework.TestCase;

import java.io.File;

public class TessBaseAPITest extends TestCase {
    private static final String TESSBASE_PATH = "/mnt/sdcard/tesseract/";
    private static final String DEFAULT_LANGUAGE = "eng";
    private static final String EXPECTED_FILE = TESSBASE_PATH + "tessdata/" + DEFAULT_LANGUAGE
            + ".traineddata";

    @SmallTest
    public void testInit() {
        // First, make sure the eng.traineddata file exists.
        assertTrue("Make sure that you've copied " + DEFAULT_LANGUAGE + ".traineddata to "
                + EXPECTED_FILE, new File(EXPECTED_FILE).exists());

        // Attempt to initialize the API.
        final TessBaseAPI baseApi = new TessBaseAPI();
        baseApi.init(TESSBASE_PATH, DEFAULT_LANGUAGE);

        // Attempt to shut down the API.
        baseApi.end();
    }

    @SmallTest
    public void testSetImage() {
        // First, make sure the eng.traineddata file exists.
        assertTrue("Make sure that you've copied " + DEFAULT_LANGUAGE + ".traineddata to "
                + EXPECTED_FILE, new File(EXPECTED_FILE).exists());

        // Attempt to initialize the API.
        final TessBaseAPI baseApi = new TessBaseAPI();
        baseApi.init(TESSBASE_PATH, DEFAULT_LANGUAGE);

        // Set the image to a Bitmap.
        final Bitmap bmp = Bitmap.createBitmap(640, 480, Bitmap.Config.ARGB_8888);
        baseApi.setImage(bmp);

        // Attempt to shut down the API.
        baseApi.end();
    }

    @SmallTest
    public void testGetUTF8Text() {
        // First, make sure the eng.traineddata file exists.
        assertTrue("Make sure that you've copied " + DEFAULT_LANGUAGE + ".traineddata to "
                + EXPECTED_FILE, new File(EXPECTED_FILE).exists());

        final String inputText = "hello";

        // Attempt to initialize the API.
        final TessBaseAPI baseApi = new TessBaseAPI();
        baseApi.init(TESSBASE_PATH, DEFAULT_LANGUAGE);
        baseApi.setPageSegMode(TessBaseAPI.PSM_SINGLE_LINE);

        // Set the image to a Bitmap containing "hello".
        final Bitmap bmp = Bitmap.createBitmap(640, 480, Bitmap.Config.ARGB_8888);
        final Paint paint = new Paint();
        final Canvas canvas = new Canvas(bmp);

        paint.setColor(Color.WHITE);
        paint.setStyle(Style.FILL);
        canvas.drawRect(new Rect(0, 0, 640, 480), paint);

        paint.setColor(Color.BLACK);
        paint.setStyle(Style.FILL);
        paint.setAntiAlias(true);
        paint.setTextAlign(Align.CENTER);
        paint.setTextSize(24.0f);
        canvas.drawText(inputText, 320, 240, paint);

        baseApi.setImage(bmp);

        // Ensure that the result is correct.
        final String outputText = baseApi.getUTF8Text();
        assertTrue("\"" + outputText + "\" != \"" + inputText + "\"", inputText.equals(outputText));

        // Attempt to shut down the API.
        baseApi.end();
        bmp.recycle();
    }
}
