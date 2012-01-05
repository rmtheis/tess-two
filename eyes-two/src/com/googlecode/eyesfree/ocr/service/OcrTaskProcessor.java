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

package com.googlecode.eyesfree.ocr.service;

import android.graphics.Rect;
import android.os.AsyncTask;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;

import com.googlecode.eyesfree.ocr.client.Ocr;
import com.googlecode.eyesfree.ocr.client.Ocr.Parameters;
import com.googlecode.eyesfree.ocr.client.OcrResult;
import com.googlecode.eyesfree.textdetect.HydrogenTextDetector;
import com.googlecode.leptonica.android.Constants;
import com.googlecode.leptonica.android.Convert;
import com.googlecode.leptonica.android.Pix;
import com.googlecode.leptonica.android.Pixa;
import com.googlecode.leptonica.android.ReadFile;
import com.googlecode.leptonica.android.Rotate;
import com.googlecode.leptonica.android.Scale;
import com.googlecode.leptonica.android.WriteFile;
import com.googlecode.tesseract.android.TessBaseAPI;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedList;
import java.util.ListIterator;

/**
 * @author alanv@google.com (Alan Viverette)
 */
public class OcrTaskProcessor {
    private static final String TAG = "OcrTaskProcessor";

    /** The wrapper for the native Tesseract instance. */
    private final TessBaseAPI mTessBaseAPI;

    /** The wrapper for the native Hydrogen instance. */
    private final HydrogenTextDetector mTextDetector;

    /** List of queued tasks with the current task at the front. */
    private final LinkedList<OcrTask> mTaskQueue;

    /** The path containing the <code>tessdata</code> directory. */
    private final File mDatapath;

    private final Handler mHandler;

    /** Object that receives recognition results. */
    private OcrTaskListener mListener;

    private AsyncOcrTask mCurrentTask;

    /**
     * Creates a new OCR task processor using the given data path.
     *
     * @param datapath A path to a directory containing the <code>tessdata
     *            </code> directory.
     */
    public OcrTaskProcessor(File datapath) {
        mDatapath = datapath;

        mHandler = new Handler();
        mTessBaseAPI = new TessBaseAPI();
        mTextDetector = new HydrogenTextDetector();
        mTaskQueue = new LinkedList<OcrTask>();
    }

    /**
     * Sets the object that receives recognition results.
     *
     * @param listener The object that receives recognition results.
     */
    public void setListener(OcrTaskListener listener) {
        mListener = listener;
    }

    /**
     * Cancels a job, stopping it if it is currently running or removing it from
     * the job queue if it is not.
     *
     * @param pid The ID of the requesting process.
     * @param token The task ID of the OCR job to cancel.
     */
    public boolean cancel(int pid, long token) {
        synchronized (mTaskQueue) {
            if (mCurrentTask != null && mCurrentTask.mPid == pid && mCurrentTask.mToken == token) {
                mCurrentTask.requestStop();
                return true;
            }

            final ListIterator<OcrTask> it = mTaskQueue.listIterator();

            while (it.hasNext()) {
                final OcrTask task = it.next();

                if (task.pid == pid && task.token == token) {
                    it.remove();
                    return true;
                }
            }
        }

        return false;
    }

    /**
     * Cancels all jobs for a given process ID.
     *
     * @param pid The ID of the requesting process.
     */
    public boolean cancelAll(int pid) {
        boolean removed = false;

        synchronized (mTaskQueue) {
            if (mCurrentTask != null && mCurrentTask.mPid == pid) {
                mCurrentTask.requestStop();
            }

            final ListIterator<OcrTask> it = mTaskQueue.listIterator();

            while (it.hasNext()) {
                final OcrTask task = it.next();

                if (task.pid == pid) {
                    it.remove();

                    removed = true;
                }
            }
        }

        return removed;
    }

    /**
     * Cancels all jobs.
     */
    public void abort() {
        synchronized (mTaskQueue) {
            mTaskQueue.clear();
        }
    }

    /**
     * Cancels pending jobs and releases resources. No jobs should be queued
     * after calling this method.
     */
    public void shutdown() {
        abort();

        mTessBaseAPI.end();
    }

    /**
     * Enqueues a new byte array for processing.
     *
     * @param pid
     * @param data
     * @param params
     * @return The task ID of the enqueued job.
     */
    public long enqueueData(int pid, byte[] data, Parameters params) {
        return enqueueTask(new OcrTask(pid, data, params));
    }

    /**
     * Enqueues a new file for processing.
     *
     * @param pid
     * @param file
     * @param params
     * @return The task ID of the enqueued job.
     */
    public long enqueueFile(int pid, File file, Parameters params) {
        return enqueueTask(new OcrTask(pid, file, params));
    }

    private long enqueueTask(OcrTask task) {
        boolean wasEmpty = false;

        synchronized (mTaskQueue) {
            wasEmpty = mTaskQueue.isEmpty();
            mTaskQueue.addLast(task);
        }

        if (wasEmpty) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    nextTask();
                }
            });
        }

        return task.token;
    }

    private void nextTask() {
        synchronized (mTaskQueue) {
            mCurrentTask = new AsyncOcrTask();
            mCurrentTask.execute(mTaskQueue.getFirst());
        }
    }

    private void finishedTask() {
        boolean isEmpty = false;

        synchronized (mTaskQueue) {
            mCurrentTask = null;
            mTaskQueue.removeFirst();

            isEmpty = mTaskQueue.isEmpty();
        }

        if (!isEmpty) {
            nextTask();
        }
    }

    private class AsyncOcrTask extends AsyncTask<OcrTask, OcrResult, ArrayList<OcrResult>> {
        private int mPid;
        private int mToken;
        private boolean mStopRequested = false;

        @Override
        protected ArrayList<OcrResult> doInBackground(OcrTask... tasks) {
            if (tasks.length == 0 || tasks[0] == null || isStopRequested()) {
                return null;
            }

            final OcrTask task = tasks[0];
            final File file = task.file;
            final byte[] data = task.data;
            final Pix pix = file == null ? ReadFile.readMem(data) : ReadFile.readFile(file);

            mPid = task.pid;
            mToken = task.token;

            // Using arrays because Java can't pass primitives by reference or
            // return tuples, but we need to get the scale and angle values.
            final float[] scale = new float[] {
                1.0f
            };
            final float[] angle = new float[] {
                0.0f
            };

            final Pixa pixa = processPix(task, pix, scale, angle);
            prepareOcrLibrary(task);
            final ArrayList<OcrResult> results = recognizeBitmap(task, pixa, scale[0], angle[0]);
            cleanupOcrLibrary();

            return results;
        }

        @Override
        protected void onProgressUpdate(OcrResult... values) {
            if (mListener == null || values.length == 0 || values[0] == null) {
                return;
            }

            final OcrResult result = values[0];

            mListener.onResult(mPid, mToken, result);
        }

        @Override
        protected void onPostExecute(ArrayList<OcrResult> results) {
            if (mListener == null) {
                return;
            }

            mListener.onCompleted(mPid, mToken, results);

            finishedTask();
        }

        public void requestStop() {
            mStopRequested = true;
        }

        public boolean isStopRequested() {
            return mStopRequested;
        }

        /**
         * Opens the configured language and sets variables. We're setting up
         * Tesseract for this specific job, so we should be the only process
         * accessing it.
         */
        private void prepareOcrLibrary(OcrTask task) {
            final Parameters params = task.params;
            final String language = params.getLanguage();
            final boolean debug = params.getFlag(Ocr.Parameters.FLAG_DEBUG_MODE);
            final boolean detectText = params.getFlag(Ocr.Parameters.FLAG_DETECT_TEXT);
            final int pageSegMode = detectText ? Ocr.Parameters.PSM_SINGLE_LINE : params
                    .getPageSegMode();

            mTessBaseAPI.init(mDatapath.getAbsolutePath() + "/", language);
            mTessBaseAPI.setDebug(debug);
            mTessBaseAPI.setPageSegMode(pageSegMode);

            for (String key : params.getVariableKeys()) {
                String value = params.getVariable(key);
                mTessBaseAPI.setVariable(key, value);
            }
        }

        /**
         * Loads the image, runs any processing options, and runs OCR on all
         * bounding rectangles. Finally, releases the image.
         *
         * @param pixa A {@link Pixa} containing the images to recognize.
         * @param scale The scaling factor for the input {@link Pix}.
         * @return A list of {@link OcrResult}s, one for each {@link Pix}.
         */
        private ArrayList<OcrResult> recognizeBitmap(OcrTask task, Pixa pixa, float scale,
                float angle) {
            if (isStopRequested() || pixa == null)
                return null;

            final Parameters params = task.params;
            final boolean spellcheck = params.getFlag(Ocr.Parameters.FLAG_SPELLCHECK);

            int numSamples = Math.min(3, pixa.size());
            boolean needsRotation = false;
            float avgConfidence = 0.0f;
            ArrayList<OcrResult> results = new ArrayList<OcrResult>(pixa.size());

            // Run first three results to get average confidence
            for (int i = 0; !isStopRequested() & i < numSamples; i++) {
                OcrResult result = getOcrResult(pixa, scale, angle, i, spellcheck, false);
                avgConfidence += result.getAverageConfidence() / numSamples;
                results.add(result);
            }

            // If the preliminary results don't look good (avg. conf < 75), then
            // try
            // again and rotate them all 180 degrees
            if (avgConfidence < 75.0f) {
                Log.e(TAG, "First " + numSamples + " results don't look so hot (avgConfidence="
                        + avgConfidence + ")");

                float flipAvgConfidence = 0.0f;
                ArrayList<OcrResult> flipResults = new ArrayList<OcrResult>(3);

                for (int i = 0; !isStopRequested() & i < numSamples; i++) {
                    OcrResult result = getOcrResult(pixa, scale, angle, i, spellcheck, true);
                    flipAvgConfidence += result.getAverageConfidence() / numSamples;
                    flipResults.add(result);
                }

                // If the rotated set is better, rotate all future images and
                // reverse the results when done
                if (flipAvgConfidence > avgConfidence) {
                    Log.e(TAG, "First " + numSamples
                            + " results look upside-down, flipping all subsequent images");

                    results = flipResults;
                    needsRotation = true;
                }
            }

            Log.e(TAG, "Found " + results.size() + " text areas in first " + numSamples
                    + " samples");

            // Run callbacks on whichever mode we're using
            for (OcrResult result : results) {
                onProgressUpdate(result);
            }

            // Expand whichever result list we've decided to use
            results.ensureCapacity(pixa.size());

            // Since we're already OCR'ed numSamples, continue from there...
            int i = numSamples;
            for (; !isStopRequested() && i < pixa.size(); i++) {
                OcrResult result = getOcrResult(pixa, scale, angle, i, spellcheck, needsRotation);

                // Don't bother returning empty results!
                if (result.getString().length() > 0) {
                    results.add(result);

                    onProgressUpdate(result);
                }
            }

            // If we had to rotate the samples, we need to reverse the list as
            // well
            if (needsRotation) {
                Collections.reverse(results);
            }

            pixa.recycle();

            return results;
        }

        private OcrResult getOcrResult(Pixa pixa, float scale, float angle, int index,
                boolean spellcheck, boolean rotate) {
            Pix pix = pixa.getPix(index);

            // We've already decided the image is rotated 180 degrees
            // TODO(alanv): Write a native function to in-place rotate
            if (rotate) {
                angle += 180.0f;

                Pix temp = Rotate.rotate(pix, 180.0f, false);
                pix.recycle();
                pix = temp;
            }

            mTessBaseAPI.setImage(pix);
            String string = mTessBaseAPI.getUTF8Text();
            int[] confidences = mTessBaseAPI.wordConfidences();
            mTessBaseAPI.clear();

            pix.recycle();

            // Scale the bounding rectangle back up
            Rect bound = pixa.getBoxRect(index);
            scaleRect(bound, scale);

            // TODO(alanv): Send the text angle with the result
            return new OcrResult(bound, string, confidences, angle);
        }

        /**
         * Scales a rectangle in-place using the specified scaling factor.
         *
         * @param rect The rectangle to scale in-place.
         * @param scale The factor by which to scale.
         */
        private void scaleRect(Rect rect, float scale) {
            rect.top /= scale;
            rect.bottom /= scale;
            rect.left /= scale;
            rect.right /= scale;
        }

        /** We'd rather not process anything larger than 720p. */
        private static final int MAX_IMAGE_AREA = 1280 * 720;

        private Pixa processPix(OcrTask task, Pix curr, float[] scale, float[] angle) {
            final Parameters params = task.params;
            final File outputDir = task.outputDir;
            final boolean debug = params.getFlag(Ocr.Parameters.FLAG_DEBUG_MODE);

            long prev;
            long time = System.currentTimeMillis();

            // Transfer parameters to text detector
            HydrogenTextDetector.Parameters hydrogenParams = mTextDetector.getParameters();
            hydrogenParams.debug = debug;
            hydrogenParams.skew_enabled = params.getFlag(Ocr.Parameters.FLAG_ALIGN_TEXT);
            mTextDetector.setParameters(hydrogenParams);

            Pix temp = null;

            int w = curr.getWidth();
            int h = curr.getHeight();

            if (outputDir != null && debug) {
                WriteFile.writeImpliedFormat(curr, new File(outputDir, time + "_0_input.jpg"));
            }

            if (isStopRequested())
                return null;

            // Convert to 8bpp first if necessary
            if (curr.getDepth() != 8) {
                temp = Convert.convertTo8(curr);
                curr.recycle();
                curr = temp;

                if (outputDir != null && debug) {
                    WriteFile.writeImpliedFormat(curr, new File(outputDir, time + "_1_8bpp.jpg"));
                }
            }

            // Shrink if necessary
            int[] dimensions = curr.getDimensions();
            int area = dimensions[0] * dimensions[1];
            if (area > MAX_IMAGE_AREA) {
                scale[0] = MAX_IMAGE_AREA / (float) area;
                Log.i(TAG, "Scaling input image to a factor of " + scale[0]);
                temp = Scale.scale(curr, scale[0]);
                curr.recycle();
                curr = temp;

                if (outputDir != null && debug) {
                    WriteFile.writeImpliedFormat(curr, new File(outputDir, time + "_2_scaled.jpg"));
                }
            }

            // Run text detection (thresholding, alignment, etc.)
            mTextDetector.setSourceImage(curr);
            mTextDetector.detectText();
            curr.recycle();

            // Get alignment angle
            angle[0] = mTextDetector.getSkewAngle();

            // Sort by increasing Y-value so that we read results in order
            Pixa unsorted = mTextDetector.getTextAreas();
            Pixa pixa = unsorted.sort(Constants.L_SORT_BY_Y, Constants.L_SORT_INCREASING);
            unsorted.recycle();

            if (outputDir != null && debug) {
                pixa.writeToFileRandomCmap(new File(outputDir, time + "_5_detected.jpg"));
            }

            return pixa;
        }

        private void cleanupOcrLibrary() {
            mTessBaseAPI.clear();
        }
    }

    private class OcrTask {
        /* package */final int pid;
        /* package */final int token;
        /* package */final File file;
        /* package */final byte[] data;
        /* package */final Ocr.Parameters params;
        /* package */final File outputDir;

        public OcrTask(int pid, File file, Ocr.Parameters params) {
            this(pid, file, null, params);
        }

        public OcrTask(int pid, byte[] data, Ocr.Parameters params) {
            this(pid, null, data, params);
        }

        private OcrTask(int pid, File file, byte[] data, Ocr.Parameters params) {
            this.pid = pid;
            this.token = hashCode();
            this.file = file;
            this.data = data;
            this.params = params;
            this.outputDir = Environment.getExternalStorageDirectory();
        }
    }

    public interface OcrTaskListener {
        public void onResult(int pid, long token, OcrResult result);

        public void onCompleted(int pid, long token, ArrayList<OcrResult> results);
    }
}
