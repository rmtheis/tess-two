/*
 * Copyright (C) 2011 Google Inc.
 * Copyright (C) 2011 Robert Theis
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

package com.googlecode.tesseract.android;

import android.graphics.Bitmap;
import android.graphics.Rect;
import android.support.annotation.IntDef;
import android.support.annotation.WorkerThread;

import com.googlecode.leptonica.android.Pix;
import com.googlecode.leptonica.android.Pixa;
import com.googlecode.leptonica.android.ReadFile;

import java.io.File;
import java.lang.annotation.Retention;

import static java.lang.annotation.RetentionPolicy.SOURCE;

/**
 * Java interface for the Tesseract OCR engine. Does not implement all available
 * JNI methods, but does implement enough to be useful. Comments are adapted
 * from original Tesseract source.
 * 
 * @author alanv@google.com (Alan Viverette)
 */
@SuppressWarnings({"unused", "WeakerAccess"})
public class TessBaseAPI {
    /**
     * Used by the native implementation of the class.
     */
    private long mNativeData;

    static {
        System.loadLibrary("jpgt");
        System.loadLibrary("pngt");
        System.loadLibrary("lept");
        System.loadLibrary("tess");

        nativeClassInit();
    }

    /** Page segmentation mode. */
    public static final class PageSegMode {
        @Retention(SOURCE)
        @IntDef({PSM_OSD_ONLY, PSM_AUTO_OSD, PSM_AUTO_ONLY, PSM_AUTO, PSM_SINGLE_COLUMN,
                PSM_SINGLE_BLOCK_VERT_TEXT, PSM_SINGLE_BLOCK, PSM_SINGLE_LINE, PSM_SINGLE_WORD,
        PSM_CIRCLE_WORD, PSM_SINGLE_CHAR, PSM_SPARSE_TEXT, PSM_SPARSE_TEXT_OSD, PSM_RAW_LINE})
        public @interface Mode {}

        /** Orientation and script detection only. */
        public static final int PSM_OSD_ONLY = 0;

        /** Automatic page segmentation with orientation and script detection. (OSD) */
        public static final int PSM_AUTO_OSD = 1;

        /** Fully automatic page segmentation, but no OSD, or OCR. */
        public static final int PSM_AUTO_ONLY = 2;

        /** Fully automatic page segmentation, but no OSD. */
        public static final int PSM_AUTO = 3;

        /** Assume a single column of text of variable sizes. */
        public static final int PSM_SINGLE_COLUMN = 4;

        /** Assume a single uniform block of vertically aligned text. */
        public static final int PSM_SINGLE_BLOCK_VERT_TEXT = 5;

        /** Assume a single uniform block of text. (Default.) */
        public static final int PSM_SINGLE_BLOCK = 6;

        /** Treat the image as a single text line. */
        public static final int PSM_SINGLE_LINE = 7;

        /** Treat the image as a single word. */
        public static final int PSM_SINGLE_WORD = 8;

        /** Treat the image as a single word in a circle. */
        public static final int PSM_CIRCLE_WORD = 9;

        /** Treat the image as a single character. */
        public static final int PSM_SINGLE_CHAR = 10;

        /** Find as much text as possible in no particular order. */
        public static final int PSM_SPARSE_TEXT = 11;

        /** Sparse text with orientation and script detection. */
        public static final int PSM_SPARSE_TEXT_OSD = 12;

        /** Treat the image as a single text line, bypassing hacks that are Tesseract-specific. */
        public static final int PSM_RAW_LINE = 13;
    }

    /** Whitelist of characters to recognize. */
    public static final String VAR_CHAR_WHITELIST = "tessedit_char_whitelist";

    /** Blacklist of characters to not recognize. */
    public static final String VAR_CHAR_BLACKLIST = "tessedit_char_blacklist";

    /** Save blob choices allowing us to get alternative results. */
    public static final String VAR_SAVE_BLOB_CHOICES = "save_blob_choices";

    /** String value used to assign a boolean variable to true. */
    public static final String VAR_TRUE = "T";

    /** String value used to assign a boolean variable to false. */
    public static final String VAR_FALSE = "F";

    @Retention(SOURCE)
    @IntDef({OEM_TESSERACT_ONLY, OEM_CUBE_ONLY, OEM_TESSERACT_CUBE_COMBINED, OEM_DEFAULT})
    public @interface OcrEngineMode {}

    /** Run Tesseract only - fastest */
    public static final int OEM_TESSERACT_ONLY = 0;

    /** Run Cube only - better accuracy, but slower */
    @Deprecated
    public static final int OEM_CUBE_ONLY = 1;

    /** Run both and combine results - best accuracy */
    @Deprecated
    public static final int OEM_TESSERACT_CUBE_COMBINED = 2;

    /** Default OCR engine mode. */
    public static final int OEM_DEFAULT = 3;

    /**
     * Elements of the page hierarchy, used in {@link ResultIterator} to provide
     * functions that operate on each level without having to have 5x as many
     * functions.
     * <p>
     * NOTE: At present {@link #RIL_PARA} and {@link #RIL_BLOCK} are equivalent
     * as there is no paragraph internally yet.
     */
    public static final class PageIteratorLevel {
        @Retention(SOURCE)
        @IntDef({RIL_BLOCK, RIL_PARA, RIL_TEXTLINE, RIL_WORD, RIL_SYMBOL})
        public @interface Level {}

        /** Block of text/image/separator line. */
        public static final int RIL_BLOCK = 0;

        /** Paragraph within a block. */
        public static final int RIL_PARA = 1;

        /** Line within a paragraph. */
        public static final int RIL_TEXTLINE = 2;

        /** Word within a text line. */
        public static final int RIL_WORD = 3;

        /** Symbol/character within a word. */
        public static final int RIL_SYMBOL = 4;
    }

    private ProgressNotifier progressNotifier;

    private boolean mRecycled;

    /**
     * Interface that may be implemented by calling object in order to receive 
     * progress callbacks during OCR.
     *
     * Progress callbacks are available when {@link #getHOCRText(int)} is used.
     */
    public interface ProgressNotifier {
        void onProgressValues(ProgressValues progressValues);
    }

    /**
     * Represents values indicating recognition progress and status.
     */
    public class ProgressValues {
        private final int percent;
        private final Rect wordRect;
        private final Rect textRect;

        public ProgressValues(int percent, Rect wordRect, Rect textRect) {
            this.percent = percent;
            this.wordRect = wordRect;
            this.textRect = textRect;
        }

        /**
         * Return word recognition progress.
         * 
         * @return a value between 0 and 100
         */
        public int getPercent() {
            return percent;
        }

        /**
         * Return the bounds of the word currently being recognized.
         * 
         * The returned bounding box is in the Android coordinate system,
         * which has the origin in the top left.
         * 
         * @return an {@link android.graphics.Rect} bounding box
         */
        public Rect getCurrentWordRect() {
            return wordRect;
        }

        /**
         * Return the bounds of the current recognition region. May match the 
         * bounds of the entire image or a sub-rectangle of the entire image.
         * 
         * The returned bounding box is in the Android coordinate system,
         * which has the origin in the top left.
         * 
         * @return an {@link android.graphics.Rect} bounding box
         */
        public Rect getCurrentRect() {
            return textRect;
        }
    }

    /**
     * Constructs an instance of TessBaseAPI.
     * <p>
     * When the instance of TessBaseAPI is no longer needed, its {@link #end}
     * method must be invoked to dispose of it.
     */
    public TessBaseAPI() {
        mNativeData = nativeConstruct();
        if (mNativeData == 0) {
            throw new RuntimeException("Can't create TessBaseApi object");
        }
        mRecycled = false;
    }

    /**
     * Constructs an instance of TessBaseAPI with a callback method for
     * receiving progress updates during OCR.
     * <p>
     * When the instance of TessBaseAPI is no longer needed, its {@link #end}
     * method must be invoked to dispose of it.
     * 
     * @param progressNotifier Callback to receive progress notifications
     */
    public TessBaseAPI(ProgressNotifier progressNotifier) {
        this();
        this.progressNotifier = progressNotifier;
    }

    /**
     * Initializes the Tesseract engine with a specified language model. Returns
     * <code>true</code> on success.
     * <p>
     * Instances are now mostly thread-safe and totally independent, but some
     * global parameters remain. Basically it is safe to use multiple
     * TessBaseAPIs in different threads in parallel, UNLESS you use SetVariable
     * on some of the Params in classify and textord. If you do, then the effect
     * will be to change it for all your instances.
     * <p>
     * The datapath must be the name of the parent directory of tessdata and
     * must end in / . Any name after the last / will be stripped. The language
     * is (usually) an ISO 639-3 string or <code>null</code> will default to eng.
     * It is entirely safe (and eventually will be efficient too) to call Init
     * multiple times on the same instance to change language, or just to reset
     * the classifier.
     * <p>
     * The language may be a string of the form {@code [~]<lang>[+[~]<lang>]*} indicating
     * that multiple languages are to be loaded. Eg hin+eng will load Hindi and
     * English. Languages may specify internally that they want to be loaded
     * with one or more other languages, so the ~ sign is available to override
     * that. Eg if hin were set to load eng by default, then hin+~eng would force
     * loading only hin. The number of loaded languages is limited only by
     * memory, with the caveat that loading additional languages will impact
     * both speed and accuracy, as there is more work to do to decide on the
     * applicable language, and there is more chance of hallucinating incorrect
     * words.
     * <p>
     * <b>WARNING:</b> On changing languages, all Tesseract parameters are reset
     * back to their default values. (Which may vary between languages.)
     * <p>
     * If you have a rare need to set a Variable that controls initialization
     * for a second call to Init you should explicitly call End() and then use
     * SetVariable before Init. This is only a very rare use case, since there
     * are very few uses that require any parameters to be set before Init.
     *
     * @param datapath the parent directory of tessdata ending in a forward
     *            slash
     * @param language an ISO 639-3 string representing the language(s)
     * @return <code>true</code> on success
     */
    public boolean init(String datapath, String language) {
        return init(datapath, language, OEM_DEFAULT);
    }

    /**
     * Initializes the Tesseract engine with the specified language model(s). Returns
     * <code>true</code> on success.
     *
     * @see #init(String, String)
     *
     * @param datapath the parent directory of tessdata ending in a forward
     *            slash
     * @param language an ISO 639-3 string representing the language(s)
     * @param ocrEngineMode the OCR engine mode to be set
     * @return <code>true</code> on success
     */
    public boolean init(String datapath, String language, @OcrEngineMode int ocrEngineMode) {
        if (datapath == null)
            throw new IllegalArgumentException("Data path must not be null!");
        if (!datapath.endsWith(File.separator))
            datapath += File.separator;

        File datapathFile = new File(datapath);
        if (!datapathFile.exists())
            throw new IllegalArgumentException("Data path does not exist!");

        File tessdata = new File(datapath + "tessdata");
        if (!tessdata.exists() || !tessdata.isDirectory())
            throw new IllegalArgumentException("Data path must contain subfolder tessdata!");

        //noinspection deprecation
        if (ocrEngineMode != OEM_CUBE_ONLY) {
            for (String languageCode : language.split("\\+")) {
                if (!languageCode.startsWith("~")) {
                    File datafile = new File(tessdata + File.separator + 
                            languageCode + ".traineddata");
                    if (!datafile.exists())
                        throw new IllegalArgumentException("Data file not found at " + datafile);

                    // Catch some common problematic initialization cases.
                    if (languageCode.equals("ara") || (languageCode.equals("hin") &&
                            ocrEngineMode == OEM_DEFAULT)) {
                        boolean sampleCubeFileExists = new File(tessdata +
                                File.separator + languageCode + ".cube.params").exists();
                        if (!sampleCubeFileExists) {
                            throw new IllegalArgumentException("Cube data files not found." +
                                    " See https://github.com/rmtheis/tess-two/issues/239");
                        }
                    }
                }
            }
        }

        boolean success = nativeInitOem(mNativeData, datapath, language, ocrEngineMode);

        if (success) {
            mRecycled = false;
        }

        return success;
    }

    /**
     * Returns the languages string used in the last valid initialization.
     * If the last initialization specified "deu+hin" then that will be
     * returned. If hin loaded eng automatically as well, then that will
     * not be included in this list.
     * 
     * @return the last-used language code
     */
    public String getInitLanguagesAsString() {
        if (mRecycled)
            throw new IllegalStateException();

        return nativeGetInitLanguagesAsString(mNativeData);
    }

    /**
     * Frees up recognition results and any stored image data, without actually
     * freeing any recognition data that would be time-consuming to reload.
     * Afterwards, you must call SetImage or SetRectangle before doing any
     * Recognize or Get* operation.
     */
    public void clear() {
        if (mRecycled)
            throw new IllegalStateException();

        nativeClear(mNativeData);
    }

    /**
     * Closes down tesseract and free up all memory. End() is equivalent to
     * destructing and reconstructing your TessBaseAPI.
     * <p>
     * Once End() has been used, none of the other API functions may be used
     * other than Init and anything declared above it in the class definition.
     */
    public void end() {
        if (!mRecycled) {
            nativeEnd(mNativeData);

            mRecycled = true;
        }
    }

    /**
     * Set the value of an internal "parameter."
     * <p>
     * Supply the name of the parameter and the value as a string, just as
     * you would in a config file.
     * <p>
     * Returns false if the name lookup failed.
     * <p>
     * Eg <code>setVariable("tessedit_char_blacklist", "xyz");</code> to 
     * ignore x, y and z.
     * 
     * Or <code>setVariable("classify_bln_numeric_mode", "1");</code> to set
     * numeric-only mode.
     * <p>
     * setVariable may be used before init, but settings will revert to
     * defaults on end().
     * <p>
     * Note: Must be called after init(). Only works for non-init variables.
     * 
     * @param var name of the variable
     * @param value value to set
     * @return false if the name lookup failed
     */
    public boolean setVariable(String var, String value) {
        if (mRecycled)
            throw new IllegalStateException();

        return nativeSetVariable(mNativeData, var, value);
    }

    /**
     * Return the current page segmentation mode.
     *
     * @return value of the current page segmentation mode
     */
    public @PageSegMode.Mode int getPageSegMode() {
        if (mRecycled)
            throw new IllegalStateException();

        return nativeGetPageSegMode(mNativeData);
    }

    /**
     * Sets the page segmentation mode. Defaults to 
     * {@link PageSegMode#PSM_SINGLE_BLOCK}. This controls how much processing
     * the OCR engine will perform before recognizing text.
     * <p>
     * The mode can also be modified by readConfigFile or 
     * setVariable("tessedit_pageseg_mode", mode as string).
     *
     * @param mode the {@link PageSegMode} to set
     */
    public void setPageSegMode(@PageSegMode.Mode int mode) {
        if (mRecycled)
            throw new IllegalStateException();

        nativeSetPageSegMode(mNativeData, mode);
    }

    /**
     * Sets debug mode. This controls how much information is displayed in the
     * log during recognition.
     *
     * @param enabled <code>true</code> to enable debugging mode
     */
    public void setDebug(boolean enabled) {
        if (mRecycled)
            throw new IllegalStateException();

        nativeSetDebug(mNativeData, enabled);
    }

    /**
     * Restricts recognition to a sub-rectangle of the image. Call after
     * SetImage. Each SetRectangle clears the recognition results so multiple
     * rectangles can be recognized with the same image.
     *
     * @param rect the bounding rectangle
     */
    public void setRectangle(Rect rect) {
        if (mRecycled)
            throw new IllegalStateException();

        setRectangle(rect.left, rect.top, rect.width(), rect.height());
    }

    /**
     * Restricts recognition to a sub-rectangle of the image. Call after
     * SetImage. Each SetRectangle clears the recognition results so multiple
     * rectangles can be recognized with the same image.
     *
     * @param left the left bound
     * @param top the right bound
     * @param width the width of the bounding box
     * @param height the height of the bounding box
     */
    public void setRectangle(int left, int top, int width, int height) {
        if (mRecycled)
            throw new IllegalStateException();

        nativeSetRectangle(mNativeData, left, top, width, height);
    }

    /**
     * Provides an image for Tesseract to recognize. Copies the image buffer.
     * The source image may be destroyed immediately after SetImage is called.
     * SetImage clears all recognition results, and sets the rectangle to the
     * full image, so it may be followed immediately by a GetUTF8Text, and it
     * will automatically perform recognition.
     *
     * @param file absolute path to the image file
     */
    @WorkerThread
    public void setImage(File file) {
        if (mRecycled)
            throw new IllegalStateException();

        Pix image = ReadFile.readFile(file);

        if (image == null) {
            throw new RuntimeException("Failed to read image file");
        }

        nativeSetImagePix(mNativeData, image.getNativePix());

        image.recycle();
    }

    /**
     * Provides an image for Tesseract to recognize. Copies the image buffer.
     * The source image may be destroyed immediately after SetImage is called.
     * SetImage clears all recognition results, and sets the rectangle to the
     * full image, so it may be followed immediately by a GetUTF8Text, and it
     * will automatically perform recognition.
     *
     * @param bmp bitmap representation of the image
     */
    @WorkerThread
    public void setImage(Bitmap bmp) {
        if (mRecycled)
            throw new IllegalStateException();

        Pix image = ReadFile.readBitmap(bmp);

        if (image == null) {
            throw new RuntimeException("Failed to read bitmap");
        }

        nativeSetImagePix(mNativeData, image.getNativePix());
        
        image.recycle();
    }

    /**
     * Provides a Leptonica pix format image for Tesseract to recognize. Clones
     * the pix object. The source image may be destroyed immediately after
     * SetImage is called, but its contents may not be modified.
     *
     * @param image Leptonica pix representation of the image
     */
    @WorkerThread
    public void setImage(Pix image) {
        if (mRecycled)
            throw new IllegalStateException();

        nativeSetImagePix(mNativeData, image.getNativePix());
    }

    /**
     * Provides an image for Tesseract to recognize. Copies the image buffer.
     * The source image may be destroyed immediately after SetImage is called.
     * SetImage clears all recognition results, and sets the rectangle to the
     * full image, so it may be followed immediately by a GetUTF8Text, and it
     * will automatically perform recognition.
     *
     * @param imagedata byte representation of the image
     * @param width image width
     * @param height image height
     * @param bpp bytes per pixel
     * @param bpl bytes per line
     */
    @WorkerThread
    public void setImage(byte[] imagedata, int width, int height, int bpp, int bpl) {
        if (mRecycled)
            throw new IllegalStateException();

        nativeSetImageBytes(mNativeData, imagedata, width, height, bpp, bpl);
    }

    /**
     * The recognized text is returned as a String which is coded as UTF8.
     * This is a blocking operation that will not work with {@link #stop()}.
     * Call {@link #getHOCRText(int)} before calling this function to
     * interrupt a recognition task with {@link #stop()}
     *
     * @return the recognized text
     */
    @WorkerThread
    public String getUTF8Text() {
        if (mRecycled)
            throw new IllegalStateException();

        // Trim because the text will have extra line breaks at the end
        String text = nativeGetUTF8Text(mNativeData);

        return text != null ? text.trim() : null;
    }

    /**
     * Returns the (average) confidence value between 0 and 100.
     *
     * @return confidence value
     */
    public int meanConfidence() {
        if (mRecycled)
            throw new IllegalStateException();

        return nativeMeanConfidence(mNativeData);
    }

    /**
     * Returns all word confidences (between 0 and 100) in an array.
     * <p>
     * The number of confidences should correspond to the number of 
     * space-delimited words in GetUTF8Text().
     *
     * @return an array of word confidences
     */
    public int[] wordConfidences() {
        if (mRecycled)
            throw new IllegalStateException();

        int[] conf = nativeWordConfidences(mNativeData);

        // We shouldn't return null confidences
        if (conf == null)
            conf = new int[0];

        return conf;
    }

    /**
     * Get a copy of the internal thresholded image from Tesseract.
     * <p>
     * Caller takes ownership of the Pix and must recycle() it.
     * May be called any time after setImage.
     * 
     * @return Pix containing the thresholded image
     */
    public Pix getThresholdedImage() {
        if (mRecycled)
            throw new IllegalStateException();

        return new Pix(nativeGetThresholdedImage(mNativeData));
    }

    /**
     * Returns the result of page layout analysis as a Pixa, in reading order.
     * <p>
     * Can be called before or after Recognize.
     * 
     * @return Pixa contaning page layout bounding boxes
     */
    public Pixa getRegions() {
        if (mRecycled)
            throw new IllegalStateException();

        return new Pixa(nativeGetRegions(mNativeData), 0, 0);
    }

    /**
     * Returns the textlines as a Pixa. Textlines are extracted from the 
     * thresholded image.
     * <p>
     * Can be called before or after Recognize. Block IDs are not returned.
     * Paragraph IDs are not returned.
     * 
     * @return Pixa containing textlines
     */
    public Pixa getTextlines() {
        if (mRecycled)
            throw new IllegalStateException();

        return new Pixa(nativeGetTextlines(mNativeData), 0, 0);
    }

    /**
     * Get textlines and strips of image regions as a Pixa, in reading order.
     * <p>
     * Enables downstream handling of non-rectangular regions. Can be called
     * before or after Recognize. Block IDs are not returned.
     * 
     * @return Pixa containing strips
     */
    public Pixa getStrips() {
        if (mRecycled)
            throw new IllegalStateException();

        return new Pixa(nativeGetStrips(mNativeData), 0, 0);
    }    

    /**
     * Get the words as a Pixa, in reading order.
     * <p>
     * Can be called before or after Recognize.
     * 
     * @return Pixa containing word bounding boxes 
     */
    public Pixa getWords() {
        if (mRecycled)
            throw new IllegalStateException();

        return new Pixa(nativeGetWords(mNativeData), 0, 0);
    }

    /**
     * Gets the individual connected (text) components (created after pages 
     * segmentation step, but before recognition) as a Pixa, in reading order.
     * <p>
     * Can be called before or after Recognize. Note: the caller is 
     * responsible for calling recycle() on the returned Pixa.
     * 
     * @return Pixa containing connected components bounding boxes 
     */
    public Pixa getConnectedComponents() {
        if (mRecycled)
            throw new IllegalStateException();

        return new Pixa(nativeGetConnectedComponents(mNativeData), 0, 0);
    }

    /**
     * Get a reading-order iterator to the results of LayoutAnalysis and/or
     * Recognize. The returned iterator must be deleted after use.
     * 
     * @return iterator to the results of LayoutAnalysis and/or Recognize
     */
    public ResultIterator getResultIterator() {
        if (mRecycled)
            throw new IllegalStateException();

        long nativeResultIterator = nativeGetResultIterator(mNativeData);

        if (nativeResultIterator == 0) {
            return null;
        }

        return new ResultIterator(nativeResultIterator);
    }

    /**
     * Make a HTML-formatted string with hOCR markup from the internal data
     * structures. Interruptible by {@link #stop()}.
     * 
     * @param page is 0-based but will appear in the output as 1-based. 
     * @return HTML-formatted string with hOCR markup
     */
    @WorkerThread
    public String getHOCRText(int page){
        if (mRecycled)
            throw new IllegalStateException();

        return nativeGetHOCRText(mNativeData, page);
    }

    /**
     * Set the name of the input file. Needed for training and reading a UNLV
     * zone file.
     * 
     * @param name input file name
     */
    public void setInputName(String name){
        if (mRecycled)
            throw new IllegalStateException();

        nativeSetInputName(mNativeData, name);
    } 

    /**
     * Set the name of the bonus output files. Needed only for debugging.
     * 
     * @param name output file name
     */
    public void setOutputName(String name){
        if (mRecycled)
            throw new IllegalStateException();

        nativeSetOutputName(mNativeData, name);
    } 

    /**
     * Read a "config" file containing a set of variable, value pairs.
     * <p>
     * Searches the standard places: <i>tessdata/configs, tessdata/tessconfigs</i>.
     * Note: only non-init params will be set.
     * 
     * @param filename the configuration filename, without the path
     */
    public void readConfigFile(String filename) {
        if (mRecycled)
            throw new IllegalStateException();

        nativeReadConfigFile(mNativeData, filename);
    }

    /**
     * The recognized text is returned as coded in the same format as a UTF8 
     * box file used in training.
     * <p>
     * Constructs coordinates in the original image - not just the rectangle.
     * 
     * @param page a 0-based page index that will appear in the box file.
     * @return the recognized text
     */
    public String getBoxText(int page){
        if (mRecycled)
            throw new IllegalStateException();

        return nativeGetBoxText(mNativeData, page);
    }

    /**
     * Returns the version identifier as a string.
     *
     * @return the version identifier
     */
    public String getVersion() {
        return nativeGetVersion(mNativeData);
    }

    /**
     * Cancel recognition started by {@link #getHOCRText(int)}.
     */
    public void stop() {
        if (mRecycled)
            throw new IllegalStateException();

        nativeStop(mNativeData);
    }

    /**
     * Called from native code to update progress of ongoing recognition passes.
     *
     * @param percent Percent complete
     * @param left Left bound of word bounding box
     * @param right Right bound of word bounding box
     * @param top Top bound of word bounding box
     * @param bottom Bottom bound of word bounding box
     * @param textLeft Left bound of text bounding box
     * @param textRight Right bound of text bounding box
     * @param textTop Top bound of text bounding box
     * @param textBottom Bottom bound of text bounding box
     */
    protected void onProgressValues(final int percent, final int left,
            final int right, final int top, final int bottom,
            final int textLeft, final int textRight, final int textTop, final int textBottom) {

        if (progressNotifier != null) {
            Rect wordRect = new Rect(left, textTop - top, right, textTop - bottom);
            Rect textRect = new Rect(textLeft, textBottom, textRight, textTop);

            ProgressValues pv = new ProgressValues(percent, wordRect, textRect);
            progressNotifier.onProgressValues(pv);
        }
    }

    /**
     * Starts a new document. This clears the contents of the output data.
     * 
     * Caller is responsible for escaping the provided title.
     *
     * @param tessPdfRenderer the renderer instance to use
     * @param title a title to be used in the document metadata
     * @return {@code true} on success. {@code false} on failure
     */
    public boolean beginDocument(TessPdfRenderer tessPdfRenderer, String title) {
        return nativeBeginDocument(tessPdfRenderer.getNativePdfRenderer(),
                title);
    }

    /**
     * Starts a new document with no title.
     * 
     * @param tessPdfRenderer the renderer instance to use
     * @return {@code true} on success. {@code false} on failure
     * @see #beginDocument(TessPdfRenderer, String)
     */
    public boolean beginDocument(TessPdfRenderer tessPdfRenderer) {
        return nativeBeginDocument(tessPdfRenderer.getNativePdfRenderer(), "");
    }

    /**
     * Finishes the document and finalizes the output data.
     * Invalid if beginDocument not yet called.
     *
     * @param tessPdfRenderer the renderer instance to use
     * @return {@code true} on success. {@code false} on failure
     */
    public boolean endDocument(TessPdfRenderer tessPdfRenderer) {
        return nativeEndDocument(tessPdfRenderer.getNativePdfRenderer());
    }

    /**
     * Adds the given data to the opened document (if any).
     * 
     * @param imageToProcess image to be used for OCR
     * @param imageToWrite path to image to be written into resulting document
     * @param tessPdfRenderer the renderer instance to use
     *
     * @return {@code true} on success. {@code false} on failure
     */
    public boolean addPageToDocument(Pix imageToProcess, String imageToWrite,
            TessPdfRenderer tessPdfRenderer) {
        return nativeAddPageToDocument(mNativeData, imageToProcess.getNativePix(),
                imageToWrite, tessPdfRenderer.getNativePdfRenderer());
    }

    /*package*/ long getNativeData() {
        return mNativeData;
    }

    // ******************
    // * Native methods *
    // ******************

    /**
     * Initializes static native data. Must be called on object load.
     */
    private static native void nativeClassInit();

    /**
     * Initializes native data. Must be called on object construction.
     */
    private native long nativeConstruct();

    /**
     * Calls End() and finalizes native data. Must be called on object 
     * destruction.
     */
    private native void nativeEnd(long mNativeData);

    private native boolean nativeInit(long mNativeData, String datapath, String language);

    private native boolean nativeInitOem(long mNativeData, String datapath, String language, int mode);

    private native String nativeGetInitLanguagesAsString(long mNativeData);

    private native void nativeClear(long mNativeData);

    private native void nativeSetImageBytes(
            long mNativeData,   byte[] imagedata, int width, int height, int bpp, int bpl);

    private native void nativeSetImagePix(long mNativeData, long nativePix);

    private native void nativeSetRectangle(long mNativeData, int left, int top, int width, int height);

    private native String nativeGetUTF8Text(long mNativeData);

    private native int nativeMeanConfidence(long mNativeData);

    private native int[] nativeWordConfidences(long mNativeData);

    private native boolean nativeSetVariable(long mNativeData, String var, String value);

    private native void nativeSetDebug(long mNativeData, boolean debug);

    @PageSegMode.Mode
    private native int nativeGetPageSegMode(long mNativeData);

    private native void nativeSetPageSegMode(long mNativeData, int mode);

    private native long nativeGetThresholdedImage(long mNativeData);

    private native long nativeGetRegions(long mNativeData);

    private native long nativeGetTextlines(long mNativeData);

    private native long nativeGetStrips(long mNativeData);

    private native long nativeGetWords(long mNativeData);

    private native long nativeGetConnectedComponents(long mNativeData);

    private native long nativeGetResultIterator(long mNativeData);

    private native String nativeGetBoxText(long mNativeData, int page_number);

    private native String nativeGetHOCRText(long mNativeData, int page_number);

    private native void nativeSetInputName(long mNativeData, String name);

    private native void nativeSetOutputName(long mNativeData, String name);

    private native void nativeReadConfigFile(long mNativeData, String fileName);

    private native String nativeGetVersion(long mNativeData);

    private native void nativeStop(long mNativeData);

    private native boolean nativeBeginDocument(long rendererPointer, String title);

    private native boolean nativeEndDocument(long rendererPointer);

    private native boolean nativeAddPageToDocument(long mNativeData, long nativePix, String imagePath, long rendererPointer);
}
