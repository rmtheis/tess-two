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

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Rect;
import android.os.Bundle;
import android.view.Display;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.googlecode.eyesfree.ocr.R;
import com.googlecode.eyesfree.ocr.client.Intents;
import com.googlecode.eyesfree.ocr.client.Ocr;

import java.io.File;

/**
 * This activity runs text recognition and displays bounding box results. If the
 * OCR service fails or is missing, this activity will return null.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public abstract class BaseRecognizeActivity extends Activity {
    private TextRectsView mOverlayView;

    private ProgressBar mProgress;

    private Ocr.Parameters mParams;

    private File mFile;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.recognize);

        mProgress = (ProgressBar) findViewById(R.id.progress);
        mProgress.setIndeterminate(true);

        mOverlayView = (TextRectsView) findViewById(R.id.overlay);

        findViewById(R.id.cancel_processing).setOnClickListener(onClickListener);

        // Load extras from intent
        Intent intent = getIntent();
        mParams = intent.getParcelableExtra(Intents.Recognize.EXTRA_PARAMETERS);
        mFile = new File(intent.getStringExtra(Intents.Recognize.EXTRA_INPUT));

        // Load background image in a separate thread
        Thread setBackground = new Thread() {
            @Override
            public void run() {
                setBackgroundAsync();
            }
        };
        setBackground.start();
    }

    protected void addOverlayRect(String text, Rect rect) {
        mOverlayView.addRect(text, rect);
    }

    protected Ocr.Parameters getParameters() {
        return mParams;
    }

    protected File getFile() {
        return mFile;
    }

    protected abstract void initializeOcrEngine();

    private static Bitmap getScaledBitmap(File file, int width) {
        String path = file.getAbsolutePath();

        BitmapFactory.Options sizeOpts = new BitmapFactory.Options();
        sizeOpts.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(path, sizeOpts);

        BitmapFactory.Options opts = new BitmapFactory.Options();
        opts.inSampleSize = sizeOpts.outWidth / width;

        Bitmap bitmap = BitmapFactory.decodeFile(path, opts);

        return bitmap;
    }

    private void setBackgroundAsync() {
        WindowManager manager = (WindowManager) getSystemService(Context.WINDOW_SERVICE);
        Display display = manager.getDefaultDisplay();

        final Bitmap scaled = getScaledBitmap(mFile, display.getWidth());

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mOverlayView.setImageBitmap(scaled);

                // Since we have a very limited amount of memory, we don't want
                // this to run until we're done loading the background bitmap.
                initializeOcrEngine();
            }
        });
    }

    protected void updateStatus(String status) {
        TextView txtStatus = (TextView) findViewById(R.id.progress_status);
        txtStatus.setText(status);
        txtStatus.postInvalidate();

        // TODO Notify TTS that the status has updated?
    }

    protected void updateProgress(int current, int max) {
        ProgressBar progress = (ProgressBar) findViewById(R.id.progress);
        TextView txtPercent = (TextView) findViewById(R.id.progress_percent);
        TextView txtNumber = (TextView) findViewById(R.id.progress_number);

        if (current < 0 || max <= 0) {
            progress.setIndeterminate(true);
            txtPercent.setVisibility(View.INVISIBLE);
            txtNumber.setVisibility(View.INVISIBLE);
            return;
        } else if (progress.isIndeterminate()) {
            progress.setIndeterminate(false);
            txtPercent.setVisibility(View.VISIBLE);
            txtNumber.setVisibility(View.VISIBLE);
        }

        int intPercent = 100 * current / max;

        String strPercent = getString(R.string.percent, intPercent);
        String strNumber = getString(R.string.ratio, current, max);

        progress.setMax(max);
        progress.setProgress(current);
        txtPercent.setText(strPercent);
        txtNumber.setText(strNumber);
    }

    private final OnClickListener onClickListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            if (v.getId() == R.id.cancel_processing) {
				onBackPressed();
			}
        }
    };
}
