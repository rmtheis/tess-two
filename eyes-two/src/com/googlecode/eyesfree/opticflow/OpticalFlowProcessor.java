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

package com.googlecode.eyesfree.opticflow;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PointF;
import android.graphics.Typeface;
import android.os.SystemClock;

import com.googlecode.eyesfree.env.Size;

import java.util.Vector;

/**
 * Frame processor that tracks optical flow.
 *
 * Modified by Alan Viverette from Andrew Harp's original source.
 *
 * @author Andrew Harp
 * @author alanv@google.com (Alan Viverette)
 */
public class OpticalFlowProcessor extends FrameProcessor {
    private static final boolean DRAW_TEXT = false;

    // How many history points to keep track of and draw.
    private static final int MAX_HISTORY_SIZE = 30;

    // The reduction factor in image dimensions for optical flow processing.
    private static final int DOWNSAMPLE_FACTOR = 4;

    private int frameWidth;

    private int frameHeight;

    private final Paint p;

    private final Vector<PointF> history;

    private FrameChange lastFeatures;

    private long lastTimestamp;

    private OpticalFlow mOpticalFlow;

    public OpticalFlowProcessor() {
        history = new Vector<PointF>(MAX_HISTORY_SIZE);

        p = new Paint();
        p.setAntiAlias(false);
        p.setTypeface(Typeface.SERIF);

        mOpticalFlow = new OpticalFlow();
    }

    public static class Feature {
        public final float x;

        public final float y;

        public final float score;

        public final int type;

        public Feature(final float x, final float y) {
            this.x = x;
            this.y = y;
            this.score = 0;
            this.type = -1;
        }

        public Feature(final float x, final float y, final float score, final int type) {
            this.x = x;
            this.y = y;
            this.score = score;
            this.type = type;
        }

        Feature delta(final Feature other) {
            return new Feature(this.x - other.x, this.y - other.y);
        }
    }

    public static class PointChange {
        public final Feature featureA;

        public final Feature featureB;

        Feature pointDelta;

        public PointChange(final float x1, final float y1, final float x2, final float y2,
                final float score, final int type) {
            featureA = new Feature(x1, y1, score, type);
            featureB = new Feature(x2, y2);
        }

        public Feature getDelta() {
            if (pointDelta == null) {
                pointDelta = featureB.delta(featureA);
            }
            return pointDelta;
        }
    }

    /**
     * A class that records a timestamped frame translation delta for optical
     * flow.
     */
    public static class FrameChange {
        public final Vector<PointChange> pointDeltas;

        private final float minScore;

        private final float maxScore;

        public FrameChange(final float[] framePoints) {
            final int featureStep = 7;

            float minScore = 100.0f;
            float maxScore = -100.0f;

            pointDeltas = new Vector<PointChange>(framePoints.length / featureStep);
            float totalChangeX = 0.0f;
            float totalChangeY = 0.0f;
            for (int i = 0; i < framePoints.length; i += featureStep) {
                final float x1 = framePoints[i + 0];
                final float y1 = framePoints[i + 1];

                final float x2 = framePoints[i + 3];
                final float y2 = framePoints[i + 4];
                final float score = framePoints[i + 5];
                final int type = (int) framePoints[i + 6];

                minScore = Math.min(minScore, score);
                maxScore = Math.max(maxScore, score);

                final PointChange pointDelta = new PointChange(x1, y1, x2, y2, score, type);
                final Feature change = pointDelta.getDelta();

                totalChangeX += change.x;
                totalChangeY += change.y;

                pointDeltas.add(pointDelta);
            }

            this.minScore = minScore;
            this.maxScore = maxScore;
        }
    }

    @Override
    protected synchronized void onPreprocess(final TimestampedFrame frame) {
        mOpticalFlow.setImage(frame.getRawData(), frame.getTimestamp());
    }

    @Override
    protected synchronized void onProcessFrame(final TimestampedFrame frame) {
        mOpticalFlow.computeOpticalFlow();

        if (DebugView.isVisible) {
            updateHistory();
        }
    }

    public OpticalFlow getOpticalFlow() {
        return mOpticalFlow;
    }

    @Override
    protected synchronized void onInit(final Size size) {
        this.frameWidth = size.width;
        this.frameHeight = size.height;

        mOpticalFlow.initialize(frameWidth, frameHeight, DOWNSAMPLE_FACTOR);
    }

    @Override
    protected synchronized void onShutdown() {
        mOpticalFlow = null;
    }

    private void drawHistory(final Canvas canvas) {
        drawHistoryPoint(canvas, canvas.getWidth() / 2, canvas.getHeight() / 2);
    }

    private void drawHistoryPoint(final Canvas canvas, final float startX, final float startY) {
        // Draw the center circle.
        p.setColor(Color.GREEN);
        canvas.drawCircle(startX, startY, 3.0f, p);

        p.setColor(Color.RED);
        p.setStrokeWidth(2.0f);

        float x1 = startX;
        float y1 = startY;

        // Iterate through in backwards order.
        synchronized (history) {
            final int numPoints = history.size();
            for (int featureNum = numPoints - 1; featureNum >= 0; --featureNum) {
                final PointF delta = history.get(featureNum);

                final float x2 = x1 + delta.x;
                final float y2 = y1 + delta.y;

                canvas.drawLine(x1, y1, x2, y2, p);

                x1 = x2;
                y1 = y2;
            }
        }
    }

    private static int floatToChar(final float value) {
        return Math.max(0, Math.min((int) (value * 255.999f), 255));
    }

    private void drawFeatures(final Canvas canvas) {
        if (lastFeatures == null) {
            return;
        }
        final int featureSize = 3;

        final float minScore = lastFeatures.minScore;
        final float maxScore = lastFeatures.maxScore;

        for (final PointChange feature : lastFeatures.pointDeltas) {
            final int r = floatToChar((feature.featureA.score - minScore) / (maxScore - minScore));
            final int b = floatToChar(
                    1.0f - (feature.featureA.score - minScore) / (maxScore - minScore));

            final int color = 0xFF000000 | (r << 16) | b;
            p.setColor(color);

            canvas.drawRect(feature.featureB.x - featureSize, feature.featureB.y - featureSize,
                    feature.featureB.x + featureSize, feature.featureB.y + featureSize, p);

            p.setColor(Color.CYAN);
            canvas.drawLine(feature.featureB.x, feature.featureB.y, feature.featureA.x,
                    feature.featureA.y, p);

            if (DRAW_TEXT) {
                p.setColor(Color.WHITE);
                canvas.drawText(feature.featureA.type + ": " + feature.featureA.score,
                        feature.featureA.x, feature.featureA.y, p);
            }
        }
    }

    private void updateHistory() {
        final float[] features = mOpticalFlow.getFeatures(true);

        lastFeatures = new FrameChange(features);

        final long timestamp = SystemClock.uptimeMillis();

        final PointF delta = mOpticalFlow.getAccumulatedDelta(
                lastTimestamp, frameWidth / 2, frameHeight / 2, 100);
        lastTimestamp = timestamp;

        synchronized (history) {
            history.add(delta);

            while (history.size() > MAX_HISTORY_SIZE) {
                history.remove(0);
            }
        }
    }

    @Override
    protected synchronized void onDrawDebug(final Canvas canvas) {
        drawHistory(canvas);
        drawFeatures(canvas);
    }

    @Override
    protected Vector<String> getDebugText() {
        final Vector<String> lines = new Vector<String>();

        if (lastFeatures != null) {
            lines.add("Num features " + lastFeatures.pointDeltas.size());
            lines.add("Min score: " + lastFeatures.minScore);
            lines.add("Max score: " + lastFeatures.maxScore);
        }

        return lines;
    }
}
