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

package com.googlecode.eyesfree.ocr.intent;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

import com.googlecode.eyesfree.ocr.client.Intents;
import com.googlecode.eyesfree.ocr.client.Ocr;
import com.googlecode.eyesfree.ocr.client.OcrResult;

import java.util.ArrayList;
import java.util.List;

/**
 * This activity runs text recognition and displays bounding box results. If the
 * OCR service fails or is missing, this activity will return null.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class RecognizeActivity extends BaseRecognizeActivity {
    private static final String TAG = "RecognizeActivity";

    private Ocr mOcr;

    private Ocr.Job mJob;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mOcr = null;
        mJob = null;
    }

    @Override
    public void onDestroy() {
        if (mJob != null) {
            mJob.cancel();
        }

        if (mOcr != null) {
            mOcr.release();
        }

        super.onDestroy();
    }

    @Override
    protected void initializeOcrEngine() {
        Ocr.InitCallback onInit = new Ocr.InitCallback() {
            @Override
            public void onInitialized(int status) {
                processConfig();
            }
        };

        mOcr = new Ocr(this, onInit);
        mOcr.setResultCallback(onResult);
        mOcr.setCompletionCallback(onCompleted);
    }

    private void processConfig() {
        mOcr.setParameters(getParameters());

        mJob = mOcr.enqueue(getFile());

        if (mJob == null) {
            Log.e(TAG, "Text recognition call failed");

            onCompleted.onCompleted(null);
        }
    }

    protected void processResults(List<OcrResult> results) {
        ArrayList<OcrResult> arrayResults = new ArrayList<OcrResult>(results);

        Intent result = new Intent();
        result.putExtra(Intents.Recognize.EXTRA_RESULTS, arrayResults);
        setResult(RESULT_OK, result);

        finish();
    }

    /**
     * Draw the bounding box of a single result.
     *
     * @param result
     */
    protected void processResult(OcrResult result) {
        addOverlayRect(result.getString().trim(), result.getBounds());
    }

    private final Ocr.CompletionCallback onCompleted = new Ocr.CompletionCallback() {
        @Override
        public void onCompleted(List<OcrResult> results) {
            processResults(results);
        }
    };

    private final Ocr.ResultCallback onResult = new Ocr.ResultCallback() {
        @Override
        public void onResult(OcrResult result) {
            processResult(result);
        }
    };
}
