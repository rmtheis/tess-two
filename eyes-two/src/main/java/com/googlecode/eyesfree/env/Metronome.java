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

package com.googlecode.eyesfree.env;

import android.content.Context;
import android.media.AudioManager;
import android.media.SoundPool;
import android.os.SystemClock;

/**
 * A metronome that uses a provided sound resource as its ticking noise.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class Metronome {
    private static final int DEFAULT_QUALITY = 0;

    private static final int DEFAULT_PRIORITY = 1;

    private static final int NO_LOOP = 0;

    private Thread mThread;

    private SoundPool mSoundPool;

    private int mSoundId;

    private int mStreamId;

    private long mDelayMillis;

    private float mLeftVolume;

    private float mRightVolume;

    private float mPitch;

    private boolean mAlive;

    private boolean[] mPattern;

    private int mPatternIndex;

    public Metronome(Context context, int resId) {
        mSoundPool = new SoundPool(1, AudioManager.STREAM_MUSIC, DEFAULT_QUALITY);
        mSoundId = mSoundPool.load(context, resId, DEFAULT_PRIORITY);

        mStreamId = -1;
        mDelayMillis = 1000;
        mLeftVolume = 0.5f;
        mRightVolume = 0.5f;
        mPitch = 1.0f;

        mPattern = new boolean[] { true };
        mPatternIndex = 0;
    }

    /**
     * Starts metronome playback. Does nothing if called again before stop().
     */
    public synchronized void start() {
        if (mThread != null) {
            return;
        }

        mAlive = true;

        mThread = new Thread() {
            @Override
            public void run() {
                runAsync();
            }
        };
        mThread.start();
    }

    private void runAsync() {
        long timeElapsed = 0;
        long timeLeft;
        long time = SystemClock.elapsedRealtime();

        while (mAlive) {
            timeLeft = mDelayMillis - timeElapsed;

            if (timeLeft > 0) {
                try {
                    Thread.sleep(timeLeft);
                } catch (InterruptedException e) {
                    // Do nothing
                }
            }

            timeElapsed += (SystemClock.elapsedRealtime() - time);
            time = SystemClock.elapsedRealtime();

            if (timeElapsed >= mDelayMillis) {
                timeElapsed = 0;

                int streamId = -1;

                // Play the sound only if we're supposed to play at this pattern
                // index
                if (mPattern[mPatternIndex]) {
                    streamId = mSoundPool.play(
                            mSoundId, mLeftVolume, mRightVolume, DEFAULT_PRIORITY, NO_LOOP, mPitch);
                }

                if (mStreamId >= 0) {
                    mSoundPool.stop(mStreamId);
                }

                mStreamId = streamId;

                // Move to next pattern index, looping around if necessary
                mPatternIndex++;
                if (mPatternIndex >= mPattern.length) {
                    mPatternIndex %= mPattern.length;
                }
            }
        }
    }

    /**
     * Stops metronome playback. You may call start() after this to resume
     * playback. Does nothing if called again before start().
     */
    public synchronized void stop() {
        mAlive = false;

        if (mThread != null) {
            mThread.interrupt();
            mThread = null;
        }
    }

    /**
     * Sets the delay in milliseconds between metronome clicks.
     *
     * @param delayMillis
     */
    public void setDelay(long delayMillis) {
        if (delayMillis <= 10) {
            delayMillis = 10;
        }

        mDelayMillis = delayMillis;

        if (mThread != null) {
            mThread.interrupt();
        }
    }

    /**
     * Sets the volume (within the range of 0.0 to 1.0) of the left and right
     * channels.
     *
     * @param leftChannel
     * @param rightChannel
     */
    public void setVolume(float leftChannel, float rightChannel) {
        if (leftChannel < 0.0f) {
            leftChannel = 0.0f;
        } else if (leftChannel > 1.0f) {
            leftChannel = 1.0f;
        }

        if (rightChannel < 0.0f) {
            rightChannel = 0.0f;
        } else if (rightChannel > 1.0f) {
            rightChannel = 1.0f;
        }

        mLeftVolume = leftChannel;
        mRightVolume = rightChannel;
    }

    /**
     * Sets the relative pitch of the metronome by changing the playback rate. A
     * pitch of 1.0 is normal, while a pitch of 2.0 plays twice as quickly and a
     * pitch of 0.5 plays half as quickly.
     *
     * @param pitch
     */
    public void setPitch(float pitch) {
        if (pitch < 0.001f) {
            pitch = 0.001f;
        }

        mPitch = pitch;
    }

    public void setPattern(boolean[] pattern) {
        mPattern = pattern;
        mPatternIndex = 0;
    }
}
