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

import com.googlecode.leptonica.android.Pixa;
import com.googlecode.tesseract.android.ResultIterator;
import com.googlecode.tesseract.android.TessBaseAPI;
import com.googlecode.tesseract.android.TessBaseAPI.PageIteratorLevel;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.Paint.Style;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.Log;
import android.util.Pair;

import java.io.File;
import java.util.List;

import junit.framework.TestCase;

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
    public void testSetPageSegMode() {
        // Attempt to initialize the API.
        final TessBaseAPI baseApi = new TessBaseAPI();
        baseApi.init(TESSBASE_PATH, DEFAULT_LANGUAGE);

        // Check the default page segmentation mode.
        final int defaultPageSegMode = TessBaseAPI.PageSegMode.PSM_SINGLE_BLOCK;
        assertEquals("Found unexpected default page segmentation mode.", 
                baseApi.getPageSegMode(), defaultPageSegMode);

        // Ensure that the page segmentation mode can be changed.
        final int newPageSegMode = TessBaseAPI.PageSegMode.PSM_SINGLE_CHAR;
        baseApi.setPageSegMode(newPageSegMode);
        assertEquals("Found unexpected page segmentation mode.", 
                baseApi.getPageSegMode(), newPageSegMode);

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

    private static Bitmap getTextImage(String text, int width, int height) {
        final Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        final Paint paint = new Paint();
        final Canvas canvas = new Canvas(bmp);

        canvas.drawColor(Color.WHITE);

        paint.setColor(Color.BLACK);
        paint.setStyle(Style.FILL);
        paint.setAntiAlias(true);
        paint.setTextAlign(Align.CENTER);
        paint.setTextSize(24.0f);
        canvas.drawText(text, width / 2, height / 2, paint);

        return bmp;
    }

    @SmallTest
    public void testGetUTF8Text() {
        // First, make sure the eng.traineddata file exists.
        assertTrue("Make sure that you've copied " + DEFAULT_LANGUAGE + ".traineddata to "
                + EXPECTED_FILE, new File(EXPECTED_FILE).exists());

        final String inputText = "hello";
        final Bitmap bmp = getTextImage(inputText, 640, 480);

        // Attempt to initialize the API.
        final TessBaseAPI baseApi = new TessBaseAPI();
        baseApi.init(TESSBASE_PATH, DEFAULT_LANGUAGE);
        baseApi.setPageSegMode(TessBaseAPI.PageSegMode.PSM_SINGLE_LINE);
        baseApi.setImage(bmp);

        // Ensure that the result is correct.
        final String outputText = baseApi.getUTF8Text();
        assertEquals("\"" + outputText + "\" != \"" + inputText + "\"", inputText, outputText);

        // Ensure that getHOCRText() produced a result.
        final String hOcr = baseApi.getHOCRText(0);
        assertNotNull("HOCR result not found.", hOcr);
        
        // Ensure getRegions() works.
        final Pixa regions = baseApi.getRegions();
        assertEquals("Found incorrect number of regions.", regions.size(), 1);

        // Ensure getWords() works.
        final Pixa words = baseApi.getWords();
        assertEquals("Found incorrect number of words.", words.size(), 1);

        // Ensure getConnectedComponents() works.
        final Pixa connectedComponents = baseApi.getConnectedComponents();
        assertTrue("Connected components not found.", connectedComponents.size() > 0);

        // Iterate through the results.
        final ResultIterator iterator = baseApi.getResultIterator();
        String lastUTF8Text;
        float lastConfidence;
        int[] lastBoundingBox;
        int count = 0;
        iterator.begin();
        do {
            lastUTF8Text = iterator.getUTF8Text(PageIteratorLevel.RIL_WORD);
            lastConfidence = iterator.confidence(PageIteratorLevel.RIL_WORD);
            lastBoundingBox = iterator.getBoundingBox(PageIteratorLevel.RIL_WORD);
            count++;
        } while (iterator.next(PageIteratorLevel.RIL_WORD));

        assertEquals("Found incorrect number of results.", count, 1);
        assertEquals("Found an incorrect result.", lastUTF8Text, outputText);
        assertTrue("Result was not high-confidence.", lastConfidence > 80);
        assertTrue("Result bounding box not found.", lastBoundingBox[2] > 0 && lastBoundingBox[3] > 0);

        // Attempt to shut down the API.
        baseApi.end();
        bmp.recycle();
    }

    @SmallTest
    public void testSetVariable() {
        final String inputText = "hello";
        final Bitmap bmp = getTextImage(inputText, 640, 480);

        // Attempt to initialize the API.
        final TessBaseAPI baseApi = new TessBaseAPI();
        baseApi.init(TESSBASE_PATH, DEFAULT_LANGUAGE);
        baseApi.setPageSegMode(TessBaseAPI.PageSegMode.PSM_SINGLE_LINE);

        // Ensure that setting the blacklist variable works.
        final String blacklistedCharacter = inputText.substring(1, 2);
        baseApi.setVariable(TessBaseAPI.VAR_CHAR_BLACKLIST, blacklistedCharacter);
        baseApi.setImage(bmp);
        final String outputText = baseApi.getUTF8Text();
        assertFalse("Found a blacklisted character.", outputText.contains(blacklistedCharacter));

        // Attempt to shut down the API.
        baseApi.end();
        bmp.recycle();
    }

    @SmallTest
    public void testChoiceIterator() {
        final String inputText = "hello";
        final Bitmap bmp = TessBaseAPITest.getTextImage(inputText, 640, 480);

        // Attempt to initialize the API.
        final TessBaseAPI baseApi = new TessBaseAPI();
        baseApi.init(TessBaseAPITest.TESSBASE_PATH, TessBaseAPITest.DEFAULT_LANGUAGE);
        baseApi.setPageSegMode(TessBaseAPI.PageSegMode.PSM_SINGLE_LINE);
        baseApi.setVariable(TessBaseAPI.VAR_SAVE_BLOB_CHOICES, TessBaseAPI.VAR_TRUE);

        // Ensure that text is recognized.
        baseApi.setImage(bmp);
        String recognizedText = baseApi.getUTF8Text();
        assertTrue("No recognized text found.", recognizedText != null && !recognizedText.equals(""));

        // Iterate through the results.
        ResultIterator iterator = baseApi.getResultIterator();
        List<Pair<String, Double>> choicesAndConfidences;
        iterator.begin();
        do {
            choicesAndConfidences = iterator.getChoicesAndConfidence(PageIteratorLevel.RIL_SYMBOL);
            assertNotNull("Invalid result.", choicesAndConfidences);

            for (Pair<String, Double> choiceAndConfidence : choicesAndConfidences) {
                String choice = choiceAndConfidence.first;
                Double conf = choiceAndConfidence.second;
                assertTrue("No choice value found.", choice != null && !choice.equals(""));
                assertTrue("Found an incorrect confidence value.", conf >= 0 && conf <= 100);
            }
        } while (iterator.next(PageIteratorLevel.RIL_SYMBOL));

        assertNotNull("No ChoiceIterator values found.", choicesAndConfidences);

        // Attempt to shut down the API.
        baseApi.end();
        bmp.recycle();
    }
}