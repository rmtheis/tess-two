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

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.speech.tts.TextToSpeech;
import android.util.AttributeSet;
import android.view.GestureDetector;
import android.view.GestureDetector.SimpleOnGestureListener;
import android.view.MotionEvent;
import android.view.View;

import com.googlecode.eyesfree.ocr.R;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.Vector;

/**
 * @author alanv@google.com (Alan Viverette)
 */
public class VoiceGestureView extends View {
    private static final String TAG = "VoiceGestureView";
    private static final String PACKAGE = "com.googlecode.eyesfree.ocr";
    private static final String EARCON_CLICK = "[click]";
    private static final String EARCON_LOUD_BEEP = "[long_beep]";
    private static final String EARCON_DOUBLE_BEEP = "[double_beep]";

    private GestureDetector mDetector;
    private Paint mPaint;
    private HashMap<String, String> mParams;
    private TextToSpeech mTts;

    private LinkedList<String> mOldUtterances;
    private LinkedList<String> mNewUtterances;

    private String mCurrentUtterance;

    private boolean mTtsReady;
    private boolean mManualMode;

    public VoiceGestureView(Context context) {
        super(context);

        init();
    }

    public VoiceGestureView(Context context, AttributeSet attrs) {
        super(context, attrs);

        init();
    }

    public VoiceGestureView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        init();
    }

    private void init() {
        Context context = getContext();

        mDetector = new GestureDetector(gestureListener);

        mPaint = new Paint();

        mParams = new HashMap<String, String>();
        mParams.put(TextToSpeech.Engine.KEY_PARAM_UTTERANCE_ID, TAG);

        mTts = new TextToSpeech(context, ttsInitListener);

        mOldUtterances = new LinkedList<String>();
        mNewUtterances = new LinkedList<String>();
        mCurrentUtterance = null;

        mManualMode = false;
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();

        mTts.shutdown();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        Vector<String> lines = new Vector<String>();
        lines.add("Queued: " + mNewUtterances.size());
        lines.add("Speaking: " + (mCurrentUtterance != null));
        lines.add("Spoken: " + mOldUtterances.size());

        final Paint p = new Paint();
        final int kLargeTextSize = 20;
        final int kSmallTextSize = 16;
        final int kTextBufferSize = 4;

        // TODO(andrewharp): Don't hardcode this, figure out the text
        // length.
        final int shadedWidth = 200;

        // Each block has one large header line followed by a buffer, then N
        // smaller lines each followed by a buffer, and then an additional
        // buffer.
        final int shadedHeight = kLargeTextSize + kTextBufferSize
                + (kSmallTextSize + kTextBufferSize) * lines.size() + kTextBufferSize;

        int startingYPos = 0;

        p.setColor(Color.BLACK);
        p.setAlpha(100);

        int yPos = startingYPos;
        int xPos = 0;

        canvas.drawRect(new Rect(xPos, yPos, xPos + shadedWidth, yPos + shadedHeight), p);

        // Header line.
        p.setAlpha(255);

        p.setAntiAlias(true);
        p.setColor(Color.CYAN);
        p.setTextSize(kLargeTextSize);
        yPos += kLargeTextSize + kTextBufferSize;
        canvas.drawText(TAG, xPos, yPos, p);

        mPaint.setColor(Color.WHITE);
        mPaint.setTextSize(kSmallTextSize);
        for (final String line : lines) {
            yPos += kSmallTextSize + kTextBufferSize;
            canvas.drawText(line, xPos, yPos, mPaint);
        }
    }

    public void shutdown() {
        mTts.shutdown();
    }

    /**
     * Adds an utterance to the queue. If the queue is empty and no utterance is
     * currently being spoken, plays the utterance immediately.
     *
     * @param utterance
     */
    public void addUtterance(String utterance) {
        synchronized (this) {
            mNewUtterances.addLast(utterance);

            // If the current utterance is null, advance to the one we added.
            if (mCurrentUtterance == null && !mManualMode) {
                changeUtterance(1);
            }
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        if (mDetector.onTouchEvent(e))
            return true;

        return true;
    }

    private boolean onSingleTap() {
        if (mManualMode) {
            mManualMode = false;
            return changeUtterance(1);
        } else {
            mManualMode = true;
            return changeUtterance(0);
        }
    }

    private boolean onVerticalSwipe(float delta) {
        mManualMode = true;

        if (delta > 0) {
            return changeUtterance(1);
        } else {
            return changeUtterance(-1);
        }
    }

    private boolean changeUtterance(int direction) {
        boolean changed;

        synchronized (this) {
            if (!mTtsReady) {
                return false;
            }

            LinkedList<String> src;
            LinkedList<String> dst;

            if (direction < 0) {
                src = mOldUtterances;
                dst = mNewUtterances;
            } else if (direction > 0){
                src = mNewUtterances;
                dst = mOldUtterances;
            } else {
                src = null;
                dst = mOldUtterances;
            }

            if (mCurrentUtterance != null) {
                dst.addFirst(mCurrentUtterance);
            }

            if (src != null && !src.isEmpty()) {
                mCurrentUtterance = src.removeFirst();
                mTts.speak(mCurrentUtterance, TextToSpeech.QUEUE_FLUSH, null);
                mTts.speak(EARCON_CLICK, TextToSpeech.QUEUE_ADD, mParams);

                changed = true;
            } else {
                mCurrentUtterance = null;
                mTts.speak(EARCON_LOUD_BEEP, TextToSpeech.QUEUE_FLUSH, null);

                changed = false;
                mManualMode = false;
            }
        }

        postInvalidate();

        return changed;
    }

    private static final float X_TOLERANCE = 0.25f;

    private final SimpleOnGestureListener gestureListener = new SimpleOnGestureListener() {
        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
            int width = getWidth();
            int height = getHeight();

            float dX = (e2.getX() - e1.getX()) / width;
            float dY = (e2.getY() - e1.getY()) / height;

            if (Math.abs(dX) > X_TOLERANCE) {
                return onVerticalSwipe(dX);
            }

            return false;
        }

        @Override
        public boolean onSingleTapUp(MotionEvent e) {
            return onSingleTap();
        }
    };

    private final TextToSpeech.OnInitListener ttsInitListener = new TextToSpeech.OnInitListener() {
        @Override
        public void onInit(int status) {
            synchronized (VoiceGestureView.this) {
                mTtsReady = true;

                mTts.setOnUtteranceCompletedListener(utteranceListener);
                mTts.addSpeech(EARCON_CLICK, PACKAGE, R.raw.click);
                mTts.addSpeech(EARCON_LOUD_BEEP, PACKAGE, R.raw.loud_beep);
                mTts.addSpeech(EARCON_DOUBLE_BEEP, PACKAGE, R.raw.double_beep);
            }
        }
    };

    private final TextToSpeech.OnUtteranceCompletedListener utteranceListener =
            new TextToSpeech.OnUtteranceCompletedListener() {
                @Override
                public void onUtteranceCompleted(String utteranceId) {
                    // When we finish an utterance, immediately move to the next
                    // one.
                    if (!mManualMode) {
                        changeUtterance(1);
                    }
                }
            };
}
