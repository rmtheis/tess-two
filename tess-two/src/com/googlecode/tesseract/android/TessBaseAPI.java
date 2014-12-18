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
import android.util.Log;

import com.googlecode.leptonica.android.Pixa;
import com.googlecode.leptonica.android.Pix;
import com.googlecode.leptonica.android.ReadFile;

import java.io.File;

/**
 * Java interface for the Tesseract OCR engine. Does not implement all available
 * JNI methods, but does implement enough to be useful. Comments are adapted
 * from original Tesseract source.
 * 
 * @author alanv@google.com (Alan Viverette)
 */
public class TessBaseAPI {
    /**
     * Used by the native implementation of the class.
     */
    private long mNativeData;

    static {
        System.loadLibrary("lept");
        System.loadLibrary("tess");

        nativeClassInit();
    }

    public static final class PageSegMode {
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

    	/** Number of enum entries. */
    	public static final int PSM_COUNT = 13;
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

    /** Run Tesseract only - fastest */
    public static final int OEM_TESSERACT_ONLY = 0;
    
    /** Run Cube only - better accuracy, but slower */
    public static final int OEM_CUBE_ONLY = 1;
    
    /** Run both and combine results - best accuracy */
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
    };

    private ProgressNotifier progressNotifier;

    /**
     * Interface that may be implemented by calling object in order to receive 
     * progress callbacks during OCR.
     */
    public interface ProgressNotifier {
        void onProgressValues(ProgressValues progressValues);
    }

    /**
     * Represents values indicating recognition progress and status.
     */
    public class ProgressValues {
        private int percent;
        private int boundingBoxLeft;
        private int boundingBoxRight;
        private int boundingBoxTop;
        private int boundingBoxBottom;

        public ProgressValues(int percent, int left, int right, int top, int bottom) {
            this.percent = percent;
            this.boundingBoxLeft = left;
            this.boundingBoxRight = right;
            this.boundingBoxTop = top;
            this.boundingBoxBottom = bottom;
        }

        public int getPercent() {
            return percent;
        }

        public int getBoundingBoxLeft() {
            return boundingBoxLeft;
        }

        public int getBoundingBoxRight() {
            return boundingBoxRight;
        }

        public int getBoundingBoxTop() {
            return boundingBoxTop;
        }

        public int getBoundingBoxBottom() {
            return boundingBoxBottom;
        }
    }

    /**
     * Constructs an instance of TessBaseAPI.
     */
    public TessBaseAPI() {
        nativeConstruct();
    }

    /**
     * Constructs an instance of TessBaseAPI with a callback method for
     * receiving progress updates during OCR.
     *
     * @param progressNotifier Callback to receive progress notifications
     */
    public TessBaseAPI(ProgressNotifier progressNotifier) {
        this.progressNotifier = progressNotifier;
        nativeConstruct();
    }

    /**
     * Called by the GC to clean up the native data that we set up when we
     * construct the object.
     * 
     * Altered from original version to avoid a crash-causing bug in OCR Test application.
     */
    @Override
    protected void finalize() throws Throwable {
      // TODO Find out why finalize() is getting called when we change languages, even though
      // we're still using the object. Is bypassing nativeFinalize() OK if we still call
      // baseApi.end() in the Activity's onDestroy()?

      try {
        Log.d("TessBaseAPI.java", "finalize(): NOT calling nativeFinalize() due to premature garbage collection");
        //nativeFinalize();
      } finally {
        Log.d("TessBaseAPI.java", "finalize(): calling super.finalize()");
        super.finalize();
      }
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
	 * The language may be a string of the form [~]<lang>[+[~]<lang>]* indicating
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
     * @param language (optional) an ISO 639-3 string representing the language(s)
     * @return <code>true</code> on success
     */
    public boolean init(String datapath, String language) {
        if (datapath == null)
            throw new IllegalArgumentException("Data path must not be null!");
        if (!datapath.endsWith(File.separator))
            datapath += File.separator;

        File tessdata = new File(datapath + "tessdata");
        if (!tessdata.exists() || !tessdata.isDirectory())
            throw new IllegalArgumentException("Data path must contain subfolder tessdata!");

        return nativeInit(datapath, language);
    }

    /**
     * Initializes the Tesseract engine with the specified language model(s). Returns
     * <code>true</code> on success.
     *
     * @param datapath the parent directory of tessdata ending in a forward
     *            slash
     * @param language (optional) an ISO 639-3 string representing the language(s)
     * @param ocrEngineMode the OCR engine mode to be set
     * @return <code>true</code> on success
     */
    public boolean init(String datapath, String language, int ocrEngineMode) {
        if (datapath == null)
            throw new IllegalArgumentException("Data path must not be null!");
        if (!datapath.endsWith(File.separator))
            datapath += File.separator;

        File tessdata = new File(datapath + "tessdata");
        if (!tessdata.exists() || !tessdata.isDirectory())
            throw new IllegalArgumentException("Data path must contain subfolder tessdata!");

        return nativeInitOem(datapath, language, ocrEngineMode);	
    }

    /**
     * Returns the languages string used in the last valid initialization.
     * If the last initialization specified "deu+hin" then that will be
     * returned. If hin loaded eng automatically as well, then that will
     * not be included in this list. To find the languages actually
     * loaded use GetLoadedLanguagesAsVector.
     * 
     * @return the last-used language code
     */
    public String getInitLanguagesAsString() {
    	return nativeGetInitLanguagesAsString();
    }
    
    /**
     * Frees up recognition results and any stored image data, without actually
     * freeing any recognition data that would be time-consuming to reload.
     * Afterwards, you must call SetImage or SetRectangle before doing any
     * Recognize or Get* operation.
     */
    public void clear() {
        nativeClear();
    }

    /**
     * Closes down tesseract and free up all memory. End() is equivalent to
     * destructing and reconstructing your TessBaseAPI.
     * <p>
     * Once End() has been used, none of the other API functions may be used
     * other than Init and anything declared above it in the class definition.
     */
    public void end() {
        nativeEnd();
    }

    /**
     * Set the value of an internal "variable" (of either old or new types).
     * Supply the name of the variable and the value as a string, just as you
     * would in a config file.
     * <p>
     * Example: <code>setVariable(VAR_TESSEDIT_CHAR_BLACKLIST, "xyz"); to ignore x, y and z. * setVariable(VAR_BLN_NUMERICMODE, "1"); to set numeric-only mode. * </code>
     * <p>
     * setVariable() may be used before open(), but settings will revert to
     * defaults on close().
     *
     * @param var name of the variable
     * @param value value to set
     * @return false if the name lookup failed
     */
    public boolean setVariable(String var, String value) {
        return nativeSetVariable(var, value);
    }

    /**
     * Return the current page segmentation mode.
     *
     * @return value of the current page segmentation mode
     */
    public int getPageSegMode() {
        return nativeGetPageSegMode();
    }

    /**
     * Sets the page segmentation mode. This controls how much processing the
     * OCR engine will perform before recognizing text.
     *
     * @param mode the page segmentation mode to set
     */
    public void setPageSegMode(int mode) {
        nativeSetPageSegMode(mode);
    }

    /**
     * Sets debug mode. This controls how much information is displayed in the
     * log during recognition.
     *
     * @param enabled <code>true</code> to enable debugging mode
     */
    public void setDebug(boolean enabled) {
        nativeSetDebug(enabled);
    }

    /**
     * Restricts recognition to a sub-rectangle of the image. Call after
     * SetImage. Each SetRectangle clears the recogntion results so multiple
     * rectangles can be recognized with the same image.
     *
     * @param rect the bounding rectangle
     */
    public void setRectangle(Rect rect) {
        setRectangle(rect.left, rect.top, rect.width(), rect.height());
    }

    /**
     * Restricts recognition to a sub-rectangle of the image. Call after
     * SetImage. Each SetRectangle clears the recogntion results so multiple
     * rectangles can be recognized with the same image.
     *
     * @param left the left bound
     * @param top the right bound
     * @param width the width of the bounding box
     * @param height the height of the bounding box
     */
    public void setRectangle(int left, int top, int width, int height) {
        nativeSetRectangle(left, top, width, height);
    }

    /**
     * Provides an image for Tesseract to recognize.
     *
     * @param file absolute path to the image file
     */
    public void setImage(File file) {
        Pix image = ReadFile.readFile(file);

        if (image == null) {
            throw new RuntimeException("Failed to read image file");
        }

        nativeSetImagePix(image.getNativePix());
    }

    /**
     * Provides an image for Tesseract to recognize. Does not copy the image
     * buffer. The source image must persist until after Recognize or
     * GetUTF8Chars is called.
     *
     * @param bmp bitmap representation of the image
     */
    public void setImage(Bitmap bmp) {
        Pix image = ReadFile.readBitmap(bmp);

        if (image == null) {
            throw new RuntimeException("Failed to read bitmap");
        }

        nativeSetImagePix(image.getNativePix());
    }

    /**
     * Provides a Leptonica pix format image for Tesseract to recognize. Clones
     * the pix object. The source image may be destroyed immediately after
     * SetImage is called, but its contents may not be modified.
     *
     * @param image Leptonica pix representation of the image
     */
    public void setImage(Pix image) {
        nativeSetImagePix(image.getNativePix());
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
    public void setImage(byte[] imagedata, int width, int height, int bpp, int bpl) {
        nativeSetImageBytes(imagedata, width, height, bpp, bpl);
    }

    /**
     * The recognized text is returned as a String which is coded as UTF8.
     *
     * @return the recognized text
     */
    public String getUTF8Text() {
        // Trim because the text will have extra line breaks at the end
        String text = nativeGetUTF8Text();

        return text.trim();
    }

    /**
     * Returns the mean confidence of text recognition.
     *
     * @return the mean confidence
     */
    public int meanConfidence() {
        return nativeMeanConfidence();
    }

    /**
     * Returns all word confidences (between 0 and 100) in an array. The number
     * of confidences should correspond to the number of space-delimited words
     * in GetUTF8Text().
     *
     * @return an array of word confidences (between 0 and 100) for each
     *         space-delimited word returned by GetUTF8Text()
     */
    public int[] wordConfidences() {
        int[] conf = nativeWordConfidences();

        // We shouldn't return null confidences
        if (conf == null)
            conf = new int[0];

        return conf;
    }

    /**
     * Return a copy of the internal thresholded image from Tesseract.
     * Only available after setImage.
     * 
     * @return Pix containing the thresholded image
     */
    public Pix getThresholdedImage() {
        return new Pix(nativeGetThresholdedImage());
    }
    
    /**
     * Returns the result of page layout analysis as a Pixa, in reading order.
     * 
     * @return Pixa contaning page layout bounding boxes
     */
    public Pixa getRegions() {
        return new Pixa(nativeGetRegions(), 0, 0);
    }
    
    /**
     * Returns the textlines as a Pixa.
     * 
     * Block IDs are not returned.
     * 
     * @return Pixa containing textlines
     */
    public Pixa getTextlines() {
        return new Pixa(nativeGetTextlines(), 0, 0);
    }
    
    /**
     * Returns the strips as a Pixa.
     * 
     * Block IDs are not returned.
     * 
     * @return Pixa containing strips
     */
    public Pixa getStrips() {
        return new Pixa(nativeGetStrips(), 0, 0);
    }    
    
    /**
     * Returns the word bounding boxes as a Pixa, in reading order.
     * 
     * @return Pixa containing word bounding boxes 
     */
    public Pixa getWords() {
        return new Pixa(nativeGetWords(), 0, 0);
    }

    /**
     * Gets the individual connected (text) components (created after pages 
     * segmentation step, but before recognition) as a Pixa, in reading order.
     * Can be called before or after Recognize.
     * 
     * @return Pixa containing connected components bounding boxes 
     */
    public Pixa getConnectedComponents() {
        return new Pixa(nativeGetConnectedComponents(), 0, 0);
    }

    /**
     * Returns an iterator allowing you to iterate over the top result for each recognized word or symbol.
     * 
     * @return ResultIterator iterate over the words
     */
    public ResultIterator getResultIterator() {
        long nativeResultIterator = nativeGetResultIterator();

        if (nativeResultIterator == 0) {
            return null;
        }

        return new ResultIterator(nativeResultIterator);
    }
    
    /**
     * Make a HTML-formatted string with hOCR markup from the internal data
     * structures.  
     * 
     * @param page is 0-based but will appear in the output as 1-based. 
     * @return HTML-formatted string with hOCR markup
     */
    public String getHOCRText(int page){
        return nativeGetHOCRText(page);
    }
    
    /**
     * Set the name of the input file. Needed only for training and
     * reading a UNLV zone file.
     * 
     * @param name input file name
     */
    public void setInputName(String name){
        nativeSetInputName(name);
    } 
    
    /**
     * Set the name of the output files. 
     * Needed only for debugging. 
     * @param name output file name
     */
    public void setOutputName(String name){
        nativeSetOutputName(name);
    } 
    
    /**
     * Read a "config" file containing a set of variable, value pairs.
     * Searches the standard places: <i>tessdata/configs, tessdata/tessconfigs</i>.
     * 
     * @param filename the configuration filename, without path
     */
    public void ReadConfigFile(String filename){
        nativeReadConfigFile(filename);
    }
    
    /**
     * The recognized text is returned as coded in the same format as a UTF8 
     * box file used in training.
     * 
     * @param page is a 0-based page index that will appear in the box file.
     */
    public String getBoxText(int page){
        return nativeGetBoxText(page);
    }

    /**
     * Cancel any recognition in progress.
     */
    public void stop() {
        nativeStop();
    }

    /**
     * Called from native code to update progress of ongoing recognition passes.
     *
     * @param percent Percent complete
     * @param left Left bound of word bounding box
     * @param right Right bound of word bounding box
     * @param top Top bound of word bounding box
     * @param bottom Bottom bound of word bounding box
     */
    private void onProgressValues(final int percent, final int left,
            final int right, final int top, final int bottom) {

        if (progressNotifier != null) {
            ProgressValues pv = new ProgressValues(percent, left, right, top, bottom);
            progressNotifier.onProgressValues(pv);
        }
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
    private native void nativeConstruct();

    /**
     * Finalizes native data. Must be called on object destruction.
     */
    private native void nativeFinalize();

    private native boolean nativeInit(String datapath, String language);
    
    private native boolean nativeInitOem(String datapath, String language, int mode);

    private native String nativeGetInitLanguagesAsString();
    
    private native void nativeClear();

    private native void nativeEnd();

    private native void nativeSetImageBytes(
            byte[] imagedata, int width, int height, int bpp, int bpl);

    private native void nativeSetImagePix(long nativePix);

    private native void nativeSetRectangle(int left, int top, int width, int height);

    private native String nativeGetUTF8Text();

    private native int nativeMeanConfidence();

    private native int[] nativeWordConfidences();

    private native boolean nativeSetVariable(String var, String value);

    private native void nativeSetDebug(boolean debug);

    private native int nativeGetPageSegMode();

    private native void nativeSetPageSegMode(int mode);
    
    private native long nativeGetThresholdedImage();
    
    private native long nativeGetRegions();

    private native long nativeGetTextlines();

    private native long nativeGetStrips();

    private native long nativeGetWords();

    private native long nativeGetConnectedComponents();

    private native long nativeGetResultIterator();
    
    private native String nativeGetBoxText(int page_number);
    
    private native String nativeGetHOCRText(int page_number);
    
    private native void nativeSetInputName(String name);
    
    private native void nativeSetOutputName(String name);
    
    private native void nativeReadConfigFile(String fileName);

    private native int nativeStop();
}
