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

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.List;

import junit.framework.TestCase;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.Paint.Style;
import android.graphics.Rect;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.Pair;

import com.googlecode.leptonica.android.Pix;
import com.googlecode.leptonica.android.Pixa;
import com.googlecode.tesseract.android.ResultIterator;
import com.googlecode.tesseract.android.TessBaseAPI;
import com.googlecode.tesseract.android.TessBaseAPI.PageIteratorLevel;

public class TessBaseAPITest extends TestCase {
    private static final String TESSBASE_PATH = "/mnt/sdcard/tesseract/";
    private static final String DEFAULT_LANGUAGE = "eng";
    private static final String EXPECTED_FILE = TESSBASE_PATH + "tessdata/" + DEFAULT_LANGUAGE
            + ".traineddata";
    private static final int DEFAULT_PAGE_SEG_MODE = TessBaseAPI.PageSegMode.PSM_SINGLE_BLOCK;

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
    public void testClear() {
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

        // Ensure that the getUTF8Text() operation fails after clear() is called.
        baseApi.clear();
        String text = baseApi.getUTF8Text();

        assertNull("Received non-null result after clear().", text);

        // Attempt to shut down the API.
        baseApi.end();
        bmp.recycle();
    }

    @SmallTest
    public void testGetInitLanguagesAsString() {
        // First, make sure the eng.traineddata file exists.
        assertTrue("Make sure that you've copied " + DEFAULT_LANGUAGE + ".traineddata to "
                + EXPECTED_FILE, new File(EXPECTED_FILE).exists());

        // Attempt to initialize the API.
        final TessBaseAPI baseApi = new TessBaseAPI();
        baseApi.init(TESSBASE_PATH, DEFAULT_LANGUAGE);

        // Check the last-used language code.
        String lang = baseApi.getInitLanguagesAsString();
        assertEquals("Got incorrect init languages value.", lang, DEFAULT_LANGUAGE);

        // Attempt to shut down the API.
        baseApi.end();
    }

    @SmallTest
    public void testGetThresholdedImage() {
        // First, make sure the eng.traineddata file exists.
        assertTrue("Make sure that you've copied " + DEFAULT_LANGUAGE + ".traineddata to "
                + EXPECTED_FILE, new File(EXPECTED_FILE).exists());

        // Attempt to initialize the API.
        final TessBaseAPI baseApi = new TessBaseAPI();
        baseApi.init(TESSBASE_PATH, DEFAULT_LANGUAGE);

        // Set the image to a Bitmap.
        final Bitmap bmp = Bitmap.createBitmap(640, 480, Bitmap.Config.ARGB_8888);
        baseApi.setImage(bmp);

        // Check the size of the thresholded image.
        Pix pixd = baseApi.getThresholdedImage();
        assertNotNull("Thresholded image is null.", pixd);
        assertEquals(bmp.getWidth(), pixd.getWidth());
        assertEquals(bmp.getHeight(), pixd.getHeight());

        // Attempt to shut down the API.
        baseApi.end();
        bmp.recycle();
        pixd.recycle();
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

        // Ensure getTextlines() works.
        final Pixa textlines = baseApi.getTextlines();
        assertEquals("Found incorrect number of textlines.", textlines.size(), 1);

        // Ensure getStrips() works.
        final Pixa strips = baseApi.getStrips();
        assertEquals("Found incorrect number of strips.", strips.size(), 1);

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
        Rect lastBoundingRect;
        int count = 0;
        iterator.begin();
        do {
            lastUTF8Text = iterator.getUTF8Text(PageIteratorLevel.RIL_WORD);
            lastConfidence = iterator.confidence(PageIteratorLevel.RIL_WORD);
            lastBoundingBox = iterator.getBoundingBox(PageIteratorLevel.RIL_WORD);
            lastBoundingRect = iterator.getBoundingRect(PageIteratorLevel.RIL_WORD);
            count++;
        } while (iterator.next(PageIteratorLevel.RIL_WORD));

        assertEquals("Found incorrect number of results.", count, 1);
        assertEquals("Found an incorrect result.", lastUTF8Text, outputText);
        assertTrue("Result was not high-confidence.", lastConfidence > 80);
        assertTrue("Result bounding box not found.", lastBoundingBox[2] > 0 && lastBoundingBox[3] > 0);

        boolean validBoundingRect =  lastBoundingRect.left < lastBoundingRect.right 
                && lastBoundingRect.top < lastBoundingRect.bottom;
        assertTrue("Result bounding box Rect is incorrect.", validBoundingRect);

        // Attempt to shut down the API.
        baseApi.end();
        bmp.recycle();
    }

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
    public void testInit_ocrEngineMode() {
        // First, make sure the eng.traineddata file exists.
        assertTrue("Make sure that you've copied " + DEFAULT_LANGUAGE + ".traineddata to "
                + EXPECTED_FILE, new File(EXPECTED_FILE).exists());

        // Attempt to initialize the API.
        final TessBaseAPI baseApi = new TessBaseAPI();
        boolean result = baseApi.init(TESSBASE_PATH, DEFAULT_LANGUAGE, 
                TessBaseAPI.OEM_TESSERACT_ONLY);

        assertTrue("Init was unsuccessful.", result);

        // Attempt to shut down the API.
        baseApi.end();
    }

    @SmallTest
    public void testSetImage_bitmap() {
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
        bmp.recycle();
    }

    @SmallTest
    public void testSetImage_file() throws IOException {
        // First, make sure the eng.traineddata file exists.
        assertTrue("Make sure that you've copied " + DEFAULT_LANGUAGE + ".traineddata to "
                + EXPECTED_FILE, new File(EXPECTED_FILE).exists());

        // Attempt to initialize the API.
        final TessBaseAPI baseApi = new TessBaseAPI();
        baseApi.init(TESSBASE_PATH, DEFAULT_LANGUAGE);

        // Create an image file.
        File file = File.createTempFile("testSetImage", ".bmp");
        FileOutputStream fileStream = new FileOutputStream(file);

        Bitmap bmp = Bitmap.createBitmap(640, 480, Bitmap.Config.ARGB_8888);
        bmp.compress(CompressFormat.JPEG, 85, fileStream);

        // Set the image to a File.
        baseApi.setImage(file);

        // Attempt to shut down the API.
        baseApi.end();
        bmp.recycle();
    }

    @SmallTest
    public void testSetImage_pix() throws IOException {
        // First, make sure the eng.traineddata file exists.
        assertTrue("Make sure that you've copied " + DEFAULT_LANGUAGE + ".traineddata to "
                + EXPECTED_FILE, new File(EXPECTED_FILE).exists());

        // Attempt to initialize the API.
        final TessBaseAPI baseApi = new TessBaseAPI();
        baseApi.init(TESSBASE_PATH, DEFAULT_LANGUAGE);

        // Set the image to a Pix.
        Pix pix = new Pix(640, 480, 32);
        baseApi.setImage(pix);

        // Attempt to shut down the API.
        baseApi.end();
        pix.recycle();
    }

    @SmallTest
    public void testSetPageSegMode() {
        // Attempt to initialize the API.
        final TessBaseAPI baseApi = new TessBaseAPI();
        baseApi.init(TESSBASE_PATH, DEFAULT_LANGUAGE);

        // Check the default page segmentation mode.
        assertEquals("Found unexpected default page segmentation mode.", 
                baseApi.getPageSegMode(), DEFAULT_PAGE_SEG_MODE);

        // Ensure that the page segmentation mode can be changed.
        final int newPageSegMode = TessBaseAPI.PageSegMode.PSM_SINGLE_CHAR;
        baseApi.setPageSegMode(newPageSegMode);
        assertEquals("Found unexpected page segmentation mode.", 
                baseApi.getPageSegMode(), newPageSegMode);

        // Attempt to shut down the API.
        baseApi.end();
    }

    @SmallTest
    public void testSetRectangle() {
        // First, make sure the eng.traineddata file exists.
        assertTrue("Make sure that you've copied " + DEFAULT_LANGUAGE + ".traineddata to "
                + EXPECTED_FILE, new File(EXPECTED_FILE).exists());

        // Attempt to initialize the API.
        final TessBaseAPI baseApi = new TessBaseAPI();
        baseApi.init(TESSBASE_PATH, DEFAULT_LANGUAGE);
        baseApi.setPageSegMode(TessBaseAPI.PageSegMode.PSM_SINGLE_CHAR);

        final int width = 640;
        final int height = 480;
        final Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        final Paint paint = new Paint();
        final Canvas canvas = new Canvas(bmp);

        canvas.drawColor(Color.WHITE);

        paint.setColor(Color.BLACK);
        paint.setStyle(Style.FILL);
        paint.setAntiAlias(true);
        paint.setTextAlign(Align.CENTER);
        paint.setTextSize(32.0f);

        // Draw separate text on the left and right halves of the image.
        final String leftInput = "A";
        final String rightInput  = "B";
        canvas.drawText(leftInput, width / 4, height / 2, paint);
        canvas.drawText(rightInput, width * 3 / 4, height / 2, paint);

        baseApi.setVariable(TessBaseAPI.VAR_CHAR_WHITELIST, leftInput + rightInput);
        baseApi.setImage(bmp);

        // Ensure the result is correct for a rectangle on the left half of the image.
        Rect left = new Rect(0, 0, width / 2, height);
        baseApi.setRectangle(left);
        String leftResult = baseApi.getUTF8Text();
        assertEquals("Found incorrect text.", leftInput, leftResult);

        // Ensure the result is correct for a rectangle on the right half of the image.
        Rect right = new Rect(width / 2, 0, width, height);
        baseApi.setRectangle(right);
        String rightResult = baseApi.getUTF8Text();
        assertEquals("Found incorrect text.", rightInput, rightResult);

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
    public void testWordConfidences() {
        // First, make sure the eng.traineddata file exists.
        assertTrue("Make sure that you've copied " + DEFAULT_LANGUAGE + ".traineddata to "
                + EXPECTED_FILE, new File(EXPECTED_FILE).exists());

        final String inputText = "one two three";
        final Bitmap bmp = getTextImage(inputText, 640, 480);

        // Attempt to initialize the API.
        final TessBaseAPI baseApi = new TessBaseAPI();
        baseApi.init(TESSBASE_PATH, DEFAULT_LANGUAGE);
        baseApi.setPageSegMode(TessBaseAPI.PageSegMode.PSM_SINGLE_BLOCK);

        baseApi.setImage(bmp);
        String text = baseApi.getUTF8Text();

        assertNotNull("Recognized text is null.", text);

        // Ensure that a mean confidence value is returned.
        int conf = baseApi.meanConfidence();
        boolean validConf = conf > 0 && conf <= 100;
        assertTrue("Mean confidence value is incorrect.", validConf);

        // Ensure that word confidence values are returned.
        int numWords = text.split("\\s+").length;
        int[] wordConf = baseApi.wordConfidences();
        assertEquals("Found the wrong number of word confidence values.", numWords, wordConf.length);
        for (int i = 0; i < wordConf.length; i++) {
            boolean valid = 0 <= wordConf[i] && wordConf[i] <= 100;
            assertTrue("Found an invalid word confidence value.", valid);
        }

        // Attempt to shut down the API.
        baseApi.end();
        bmp.recycle();
    }
}