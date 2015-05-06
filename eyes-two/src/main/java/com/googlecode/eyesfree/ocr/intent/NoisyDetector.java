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
import android.graphics.RectF;
import android.media.AudioManager;
import android.media.SoundPool;

import com.googlecode.eyesfree.env.Metronome;
import com.googlecode.eyesfree.ocr.R;

import java.util.ArrayList;

/**
 * @author alanv@google.com (Alan Viverette)
 */
public class NoisyDetector {
    /* Minimum height of a text item in pixels */
    private static final int MIN_HEIGHT = 30;

    /* Percent error allowed for height threshold */
    private static final float HEIGHT_ERROR = 0.4f;

    private boolean mPaused;

    private Metronome mMetronome;

    private SoundPool mSoundPool;

    private int mBeepId;

    public NoisyDetector(Context context) {
        mPaused = false;

        mSoundPool = new SoundPool(1, AudioManager.STREAM_MUSIC, 0);
        mBeepId = mSoundPool.load(context, R.raw.loud_beep, 1);

        mMetronome = new Metronome(context, R.raw.click);
    }

    public void start() {
        mPaused = false;

        mMetronome.start();
    }

    public void pause() {
        mPaused = true;

        mMetronome.stop();
    }

    public void onTextDetected(RectF pixaBound, ArrayList<RectF> pixBounds) {
        if (mPaused)
            return;

        // Compute whether this is likely a text block
        int likelyText = 0;
        int likelySmallText = 0;

        for (int i = 0; i < pixBounds.size() && likelyText < 4; i++) {
            if (pixBounds.get(i) == null)
                break;

            if (BoundsClassifier.isLikelyTextRect(pixBounds, i)) {
                // Lines must be at least MIN_HEIGHT pixels high
                if (pixBounds.get(i).height() < MIN_HEIGHT) {
                    // Allow a tolerance of HEIGHT_ERROR percent around
                    // MIN_HEIGHT
                    if (BoundsClassifier.computeError(pixBounds.get(i).height(), MIN_HEIGHT)
                            < HEIGHT_ERROR) {
                        likelyText++;
                    }
                    likelySmallText++;
                } else {
                    likelyText++;
                }
            }
        }

        if (mPaused)
            return;

        if (pixBounds.size() > 0) {
            float pixaArea = pixaBound.width() * pixaBound.height();
            float pixaCenterX = pixaBound.centerX();

            float textAreaL = 0.0f;
            float textAreaR = 0.0f;

            for (RectF textBounds : pixBounds) {
                float textArea = textBounds.width() * textBounds.height();
                float textCenterX = textBounds.centerX();

                float offsetRatio = (pixaCenterX - textCenterX) / textBounds.width();
                float leftPortion = Math.min(1.0f, Math.max(0.0f, 0.5f - offsetRatio));
                float rightPortion = 1.0f - leftPortion;

                textAreaL += leftPortion * textArea;
                textAreaR += rightPortion * textArea;
            }

            float volumeL = Math.min(1.0f, 5.0f * textAreaL / pixaArea);
            float volumeR = Math.min(1.0f, 5.0f * textAreaR / pixaArea);
            long delay = (long) (300 / Math.log(1 + pixBounds.size()));

            // Make clicking noises according to how much area is text
            mMetronome.setVolume(volumeL, volumeR);
            mMetronome.setDelay(delay);
        } else {
            mMetronome.setDelay(3000);
        }

        if (mPaused)
            return;

        // Vibrate if we're looking at text
        if (likelySmallText > likelyText && likelySmallText > 4) {
            mSoundPool.play(mBeepId, 0.3f, 0.3f, 1, 0, 1.0f);
        } else if (likelyText >= 4) {
            mSoundPool.play(mBeepId, 1.0f, 1.0f, 1, 0, 1.0f);
        }
    }

    /**
     * @author alanv@google.com (Alan Viverette)
     */
    private static class BoundsClassifier {
        /** Minimum number of matching lines. */
        private static final int MIN_PAIRS = 1;

        /** Minimum width/height ratio. */
        private static final int MIN_WRATIO = 10;

        /** Top offset in height units. */
        private static final float TOP_ERROR = 2.0f;

        /** Left offset in height units. */
        private static final float LEFT_ERROR = 1.0f;


        public static boolean isLikelyTextRect(ArrayList<RectF> results, int position) {
            RectF bounds = results.get(position);

            // Lines must be at least MIN_WRATIO times as wide as they are tall
            if (bounds.width() / bounds.height() < MIN_WRATIO) {
                return false;
            }

            int pairs = 0;

            for (int i = 0; i < results.size() && pairs < MIN_PAIRS; i++) {
                if (i == position)
                    continue;
                if (results.get(i) == null)
                    break;

                RectF otherBounds = results.get(i);

                // Pairs must be within HEIGHT_ERROR similar height
                float hError = computeError(bounds.height(), otherBounds.height());
                if (hError > HEIGHT_ERROR)
                    continue;

                // Pairs must have similar left margins within LEFT_ERROR heights
                float xDist = Math.abs(bounds.left - otherBounds.left);
                if (xDist > LEFT_ERROR * bounds.height())
                    continue;

                // Pairs must be within SPACING_ERROR heights apart
                float yDist = Math.abs(bounds.top - otherBounds.top);
                if (yDist > TOP_ERROR * bounds.height())
                    continue;

                pairs++;
            }

            return pairs >= MIN_PAIRS;
        }

        public static float computeError(float expected, float experimental) {
            return Math.abs((experimental - expected) / expected);
        }
    }
}
