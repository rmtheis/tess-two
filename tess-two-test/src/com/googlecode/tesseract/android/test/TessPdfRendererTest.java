/*
 * Copyright 2015 Robert Theis
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

import java.io.File;
import java.io.IOException;

import junit.framework.TestCase;

import android.annotation.SuppressLint;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.Paint.Style;
import android.test.suitebuilder.annotation.SmallTest;

import com.googlecode.leptonica.android.Pix;
import com.googlecode.leptonica.android.ReadFile;
import com.googlecode.leptonica.android.WriteFile;
import com.googlecode.tesseract.android.TessBaseAPI;
import com.googlecode.tesseract.android.TessPdfRenderer;

public class TessPdfRendererTest extends TestCase {

    @SuppressLint("SdCardPath")
    private final static String OUTPUT_PATH = "/sdcard/";

    @SmallTest
    public void testCreate() {
        // Attempt to initialize the API.
        final TessBaseAPI baseApi = new TessBaseAPI();
        boolean success = baseApi.init(TessBaseAPITest.TESSBASE_PATH,
                TessBaseAPITest.DEFAULT_LANGUAGE);
        assertTrue(success);

        String pdfBasename = "testCreate";
        
        // Attempt to create a TessPdfRenderer instance.
        TessPdfRenderer pdfRenderer = new TessPdfRenderer(baseApi, OUTPUT_PATH
                + pdfBasename);

        pdfRenderer.recycle();
        baseApi.end();
    }

    @SmallTest
    public void testAddPageToDocument() throws IOException {
        // Attempt to initialize the API.
        final TessBaseAPI baseApi = new TessBaseAPI();
        boolean success = baseApi.init(TessBaseAPITest.TESSBASE_PATH,
                TessBaseAPITest.DEFAULT_LANGUAGE);
        assertTrue(success);

        String pdfBasename = "testAddPageToDocument";

        // Attempt to create a TessPdfRenderer instance.
        TessPdfRenderer pdfRenderer = new TessPdfRenderer(baseApi, OUTPUT_PATH
                + pdfBasename);

        // Start the PDF writing process.
        boolean beginSuccess = baseApi.beginDocument(pdfRenderer, "title");
        assertTrue(beginSuccess);

        // Add a page to the PDF.
        final Pix pixOne = getTextImage("page one", 640, 480);
        final File fileOne = File.createTempFile("testPageOne", ".png");
        WriteFile.writeImpliedFormat(pixOne, fileOne);
        boolean addedPageOne = baseApi.addPageToDocument(pixOne,
                fileOne.getAbsolutePath(), pdfRenderer);
        assertTrue(addedPageOne);

        // Add a second page.
        final Pix pixTwo = getTextImage("page two", 640, 480);
        final File fileTwo = File.createTempFile("testPageTwo", ".png");
        WriteFile.writeImpliedFormat(pixTwo, fileTwo);
        boolean addedPageTwo = baseApi.addPageToDocument(pixTwo,
                fileTwo.getAbsolutePath(), pdfRenderer);
        assertTrue(addedPageTwo);

        // Finish writing to the PDF document.
        boolean endSuccess = baseApi.endDocument(pdfRenderer);
        assertTrue(endSuccess);

        // Ensure that a PDF file was created.
        File pdf = new File(OUTPUT_PATH + pdfBasename + ".pdf");
        assertTrue(pdf.isFile());
        assertTrue(pdf.length() > 0);

        pdfRenderer.recycle();
        baseApi.end();
        pixOne.recycle();
        pixTwo.recycle();
    }

    private static Pix getTextImage(String text, int width, int height) {
        final Bitmap bmp = Bitmap.createBitmap(width, height, 
                Bitmap.Config.ARGB_8888);
        final Paint paint = new Paint();
        final Canvas canvas = new Canvas(bmp);

        canvas.drawColor(Color.WHITE);

        paint.setColor(Color.BLACK);
        paint.setStyle(Style.FILL);
        paint.setAntiAlias(true);
        paint.setTextAlign(Align.CENTER);
        paint.setTextSize(24.0f);
        canvas.drawText(text, width / 2, height / 2, paint);

        return ReadFile.readBitmap(bmp);
    }

}