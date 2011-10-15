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
import android.graphics.Bitmap.CompressFormat;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Rect;
import android.test.suitebuilder.annotation.SmallTest;

import com.googlecode.leptonica.android.Pix;
import com.googlecode.leptonica.android.ReadFile;

import junit.framework.TestCase;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * @author alanv@google.com (Your Name Here)
 */
public class ReadFileTest extends TestCase {
    @SmallTest
    public void testReadBitmap() {
        testReadBitmap(1, 1, Bitmap.Config.ARGB_8888);
        testReadBitmap(640, 480, Bitmap.Config.ARGB_8888);
    }

    private void testReadBitmap(int width, int height, Bitmap.Config format) {
        Bitmap bmp = Bitmap.createBitmap(width, height, format);
        Canvas canvas = new Canvas(bmp);
        Paint paint = new Paint();

        if (width > 1 && height > 1) {
            // Paint the left half white
            paint.setColor(Color.WHITE);
            paint.setStyle(Style.FILL);
            canvas.drawRect(new Rect(0, 0, width / 2 , height - 1), paint);

            // Paint the right half black
            paint.setColor(Color.BLACK);
            paint.setStyle(Style.FILL);
            canvas.drawRect(new Rect(width / 2, 0, width - 1, height - 1), paint);
        }

        Pix pix = ReadFile.readBitmap(bmp);

        assertEquals(bmp.getWidth(), pix.getWidth());
        assertEquals(bmp.getHeight(), pix.getHeight());

        if (width > 1 && height > 1) {
            // Make sure the colors were preserved.
            assertEquals(Color.WHITE, pix.getPixel(0, 0));
            assertEquals(Color.BLACK, pix.getPixel(width - 1, height - 1));
        }

        bmp.recycle();
        pix.recycle();
    }

    @SmallTest
    public void testReadFile() throws IOException {
        File file = File.createTempFile("testReadFile", "jpg");
        FileOutputStream fileStream = new FileOutputStream(file);
        Bitmap bmp = Bitmap.createBitmap(640, 480, Bitmap.Config.RGB_565);
        bmp.compress(CompressFormat.JPEG, 85, fileStream);
        Pix pix = ReadFile.readFile(file);

        assertEquals(bmp.getWidth(), pix.getWidth());
        assertEquals(bmp.getHeight(), pix.getHeight());

        fileStream.close();
        bmp.recycle();
        pix.recycle();
    }

    @SmallTest
    public void testReadMem() throws IOException {
        ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
        Bitmap bmp = Bitmap.createBitmap(640, 480, Bitmap.Config.RGB_565);
        bmp.compress(CompressFormat.JPEG, 85, byteStream);
        byte[] encodedData = byteStream.toByteArray();
        Pix pix = ReadFile.readMem(encodedData);

        assertEquals(bmp.getWidth(), pix.getWidth());
        assertEquals(bmp.getHeight(), pix.getHeight());

        // TODO(alanv): Need some way to test content, ex. Pix.getPixel(int, int)

        byteStream.close();
        bmp.recycle();
        encodedData = null;
        pix.recycle();
    }
}
