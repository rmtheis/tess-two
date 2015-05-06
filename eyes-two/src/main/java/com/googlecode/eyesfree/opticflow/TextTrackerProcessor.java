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
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.Paint.Style;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.SystemClock;
import android.util.Log;

import com.googlecode.eyesfree.env.Size;
import com.googlecode.leptonica.android.Pix;
import com.googlecode.leptonica.android.Pixa;

import java.util.LinkedList;
import java.util.ListIterator;
import java.util.Vector;

/**
 * Frame processor that tracks positioning and visibility of text areas.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class TextTrackerProcessor extends FrameProcessor {
    private static final String TAG = "OcrProcessor";

    /**
     * The minimum amount of overlap allowed between an existing text area
     * (after optical flow considerations) and a potential match.
     */
    private static final float MIN_OVERLAP = 0.50f;

    /**
     * The maximum amount of normalized error in aspect ratio allowed between an
     * existing text area and a potential match.
     */
    private static final float MAX_ASPECT_ERROR = 0.10f;

    /**
     * Minimum amount of time is milliseconds that a text area must persist in
     * order for it to be OCR'ed.
     */
    private static final long MIN_PRESENCE = 500;

    /**
     * Maximum amount of time in milliseconds that a text area may be absent in
     * order for it to remain in the OCR queue.
     */
    private static final long MAX_ABSENCE = 1500;

    /** Native optical flow tracker. */
    private final OpticalFlow mOpticalFlow;

    /** List of tracked text areas. */
    private final LinkedList<TrackedRect> mTrackedRects;

    /** List of new OCR candidates. */
    private LinkedList<TrackedRect> mOcrAdd;

    /** List of text areas to be removed from the OCR queue. */
    private LinkedList<TrackedRect> mOcrRemove;

    private RectF mBounds;

    /** Text detection listener. */
    private Listener mListener;

    /**
     * @param opticalFlow
     */
    public TextTrackerProcessor(OpticalFlow opticalFlow) {
        mOpticalFlow = opticalFlow;
        mTrackedRects = new LinkedList<TrackedRect>();
        mOcrAdd = new LinkedList<TrackedRect>();
        mOcrRemove = new LinkedList<TrackedRect>();
    }

    @Override
    public void onInit(Size size) {
        mBounds = new RectF(0, 0, size.width, size.height);
    }

    public void setListener(Listener listener) {
        mListener = listener;
    }

    @Override
    protected void onProcessFrame(TimestampedFrame frame) {
        if (frame.isBlurred() || frame.takenWhileFocusing()) {
            return;
        }

        Pixa pixa = frame.getDetectedText();
        float[] conf = frame.getTextConfidences();
        float angle = frame.getAngle();

        if (pixa == null)
            return;

        processResults(pixa, conf, angle);

        pixa.recycle();
        frame.recycleDetectedText();

        if (mListener != null) {
            mListener.onTextDetected(mBounds, mTrackedRects);
        }
    }

    @Override
    protected void onDrawDebug(final Canvas canvas) {
        Paint paint = new Paint();
        long current = SystemClock.uptimeMillis();

        for (TrackedRect trackedRect : mTrackedRects) {
            trackedRect.onDrawDebug(canvas, current);
        }
    }

    @Override
    protected Vector<String> getDebugText() {
        Vector<String> debugText = new Vector<String>();
        debugText.add("Tracking: " + mTrackedRects.size());
        return debugText;
    }

    private void processResults(Pixa textAreas, float[] textConfs, float angle) {
        LinkedList<TrackedRect> unmatchedRects = new LinkedList<TrackedRect>();
        LinkedList<TrackedRect> newRects = new LinkedList<TrackedRect>();

        long timestamp = SystemClock.uptimeMillis();

        updateTrackedRects(timestamp);
        matchExistingRects(textAreas, textConfs, angle, timestamp);
    }

    private void updateTrackedRects(long timestamp) {
        if (mOpticalFlow == null) {
            // No optical flow detection!
            return;
        }

        for (TrackedRect tracked : mTrackedRects) {
            PointF delta = mOpticalFlow.getAccumulatedDelta(tracked.timestamp,
                    tracked.rect.centerX(), tracked.rect.centerY(), tracked.radius());

            tracked.rect.offset(delta.x, delta.y);
            tracked.timestamp = timestamp;
        }

        Log.i(TAG, "Updated " + mTrackedRects.size() + " tracked rects");
    }

    /**
     * Attempts to match the text areas in textAreas with the currently tracked
     * rectangles.
     *
     * @param textAreas A Pixa containing the detected text areas.
     * @param textConfs An array of text confidences corresponding to the text
     *            areas.
     * @param timestamp The current system uptime in milliseconds.
     */
    private void matchExistingRects(Pixa textAreas, float[] textConfs, float angle, long timestamp) {
        int count = textConfs.length;
        boolean[] matchFlags = new boolean[count];

        ListIterator<TrackedRect> iterator = mTrackedRects.listIterator();

        // Matching algorithm runs in O(n*m) time, but we probably won't have
        // n*m > 100.
        while (iterator.hasNext()) {
            TrackedRect rect = iterator.next();

            int matchIndex = findBestMatch(rect, textAreas, matchFlags);

            if (matchIndex >= 0) {
                matchFlags[matchIndex] = true;

                boolean enqueue = onRectMatched(rect, textAreas, matchIndex, angle, timestamp);

                if (enqueue) {
                    rect.firstTimestamp = -1;
                    mOcrAdd.add(rect);
                }
            } else {
                boolean remove = onRectUnmatched(rect, timestamp);

                if (remove) {
                    iterator.remove();
                    mOcrRemove.add(rect);
                }
            }
        }

        // Go back through the list of matched Pix and add the unmatched ones to
        // the list of new tracked rects.
        for (int i = 0; i < count; i++) {
            if (matchFlags[i]) {
                continue;
            }

            Pix pix = textAreas.getPix(i);
            float quality = textConfs[i];
            Rect rect = textAreas.getBoxRect(i);

            TrackedRect newRect = new TrackedRect(pix, angle, quality, rect, timestamp);

            onRectDiscovered(newRect);
        }
    }

    /**
     * Searches textAreas for the text area most similar to rect.
     *
     * @param rect The tracked rect to match.
     * @param textAreas The Pixa containing potential matches.
     * @param matchFlags A boolean array marking matched Pix within textAreas.
     * @return Returns the index of the best match.
     */
    private int findBestMatch(TrackedRect rect, Pixa textAreas, boolean[] matchFlags) {
        float maxSimilarity = 0.0f;
        float rectAspect = rect.aspect();
        int size = textAreas.size();
        int maxIndex = -1;

        for (int i = 0; i < size; i++) {
            // Don't check if this Pix has already been claimed. Technically we
            // should check every pairing and minimize a cost function, but this
            // is easier.
            if (matchFlags[i]) {
                continue;
            }

            Rect boxRect = textAreas.getBoxRect(i);
            float overlap = rect.getOverlap(boxRect);

            // TODO(alanv): Ideally the OpticalFlow tracker will ensure that
            // identical rects overlap, but we can't count on it for dense text.
            // Remove this (and optical flow?) once we have a better way to
            // compute visual similarity.
            if (overlap < MIN_OVERLAP) {
                // Log.e(TAG, i + " failed with overlap=" + overlap);
                // continue;
            }

            float boxAspect = (boxRect.width() / (float) boxRect.height());
            float aspectError = Math.abs(boxAspect - rectAspect) / Math.max(boxAspect, rectAspect);

            // Aspect ratio should be constant even after zoom; however, it
            // results in split clusters appearing as two entirely new clusters.
            // This might not be so bad.
            if (aspectError > MAX_ASPECT_ERROR) {
                continue;
            }

            float similarity = overlap / (aspectError + 1) + (1 - aspectError);

            if (similarity > maxSimilarity) {
                maxIndex = i;
                maxSimilarity = similarity;
            }
        }

        return maxIndex;
    }

    /**
     * @param rect
     * @param textAreas
     * @param matchIndex
     * @param timestamp
     * @return true if rect needs to be added to queue
     */
    private boolean onRectMatched(TrackedRect rect, Pixa textAreas, int matchIndex, float angle,
            long timestamp) {
        Rect newRect = textAreas.getBoxRect(matchIndex);

        rect.missingTimestamp = -1;
        rect.timestamp = timestamp;
        rect.rect = new RectF(newRect);
        rect.rotation.setRotate(angle, newRect.exactCenterX(), newRect.exactCenterY());

        long presentSince = rect.firstTimestamp;

        if (presentSince < 0) {
            // We've already marked this rect as present and queued it for OCR.
            // TODO(alanv): If we've queued the rect but not finished OCR,
            // update the rect's Pix with a higher quality Pix (if one is
            // available).

            return false;
        }

        long presence = timestamp - presentSince;

        if (presence < MIN_PRESENCE) {
            return false;
        }

        return true;
    }

    /**
     * @param rect
     * @param timestamp
     * @return true if rect needs to be removed
     */
    private boolean onRectUnmatched(TrackedRect rect, long timestamp) {
        long missingSince = rect.missingTimestamp;

        if (missingSince < 0) {
            rect.missingTimestamp = timestamp;

            return false;
        }

        long absence = timestamp - missingSince;

        if (absence < MAX_ABSENCE) {
            return false;
        }

        return true;
    }

    /**
     * @param newRect
     */
    private void onRectDiscovered(TrackedRect newRect) {
        mTrackedRects.add(newRect);
    }

    public LinkedList<TrackedRect> getOcrAdd() {
        LinkedList<TrackedRect> temp = mOcrAdd;
        mOcrAdd = new LinkedList<TrackedRect>();

        return temp;
    }

    public LinkedList<TrackedRect> getOcrRemove() {
        LinkedList<TrackedRect> temp = mOcrRemove;
        mOcrRemove = new LinkedList<TrackedRect>();

        return temp;
    }

    /**
     * A huge mess containing everything the app needs to know about a tracked
     * text area. TODO(alanv): Refactor this.
     *
     * @author alanv@google.com (Alan Viverette)
     */
    public static class TrackedRect {
        public Pix pix;

        public float quality;

        public RectF rect;

        public String text;

        public Matrix rotation;

        public long firstTimestamp;

        public long timestamp;

        public long missingTimestamp;

        private Paint paint;

        public boolean queued;

        public TrackedRect(Pix pix, float quality, float angle, Rect rect, long timestamp) {
            this.pix = pix;
            this.quality = quality;
            this.rect = new RectF(rect);
            this.text = null;
            this.firstTimestamp = timestamp;
            this.timestamp = timestamp;
            this.missingTimestamp = -1;

            rotation = new Matrix();
            rotation.setRotate(angle, rect.exactCenterX(), rect.exactCenterY());

            paint = new Paint();
            paint.setTextAlign(Align.CENTER);
        }

        public float radius() {
            return (rect.width() + rect.height()) / 4;
        }

        public float aspect() {
            return (rect.width() / rect.height());
        }

        public float getOverlap(Rect other) {
            RectF otherF = new RectF(other);
            RectF isect = new RectF();

            if (isect.setIntersect(otherF, rect)) {
                float areaA = rect.width() * rect.height();
                float areaB = otherF.width() * otherF.height();
                float maxArea = Math.max(areaA, areaB);

                float isectArea = isect.width() * isect.height();

                return isectArea / maxArea;
            }

            return 0;
        }

        public void onDrawDebug(final Canvas canvas, final long timestamp) {
            int color = Color.BLACK;
            long alpha = 0xFF;

            // Apply rotation matrix.
            int saveCount = canvas.save();
            canvas.concat(rotation);

            if (missingTimestamp >= 0) {
                color = Color.YELLOW;
                long missing = timestamp - missingTimestamp;
                alpha = alpha * Math.max(0, (MAX_ABSENCE - missing)) / MAX_ABSENCE;
                color = ((int) alpha << 24) | (0xFFFFFF & color);
            } else if (firstTimestamp >= 0) {
                color = Color.RED;
                long present = timestamp - firstTimestamp;
                alpha = alpha * Math.max(0, present) / MIN_PRESENCE;
            } else {
                color = Color.GREEN;
            }

            color = ((int) alpha << 24) | (0xFFFFFF & color);

            if (text == null || text.length() > 0) {
                paint.setColor(color);
                canvas.drawRect(rect, paint);
            }

            if (text != null) {
                float cx = rect.left + rect.width() / 2.0f;
                float cy = rect.top + rect.height() * (2.0f / 3.0f);

                paint.setColor(Color.BLACK);
                paint.setStyle(Style.FILL);
                paint.setTextSize(2.0f * rect.height() / 3.0f);

                canvas.drawText(text, cx, cy, paint);
            }

            // Restore previous matrix.
            canvas.restoreToCount(saveCount);
        }
    }

    public interface Listener {
        public void onTextDetected(RectF bounds, LinkedList<TrackedRect> trackedRects);
    }
}
