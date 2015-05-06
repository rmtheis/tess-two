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
import android.graphics.Matrix;
import android.graphics.Matrix.ScaleToFit;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.Paint.Style;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.widget.ImageView;

import java.util.LinkedList;

/**
 * Displays colored bounding boxes scaled to screen resolution.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class TextRectsView extends ImageView {
    private Matrix mMatrix;

    private Paint mPaint;

    private LinkedList<TextRect> mRects;

    private int mWidth;
    private int mHeight;
    private int mSrcWidth;
    private int mSrcHeight;

    public class TextRect {
        String text;
        Rect rect;
        int color;

        public TextRect(String text, Rect rect) {
            this.text = text;
            this.rect = rect;
            this.color = (int) (Math.random() * Integer.MAX_VALUE) | 0xFF000000;
        }
    }

    public TextRectsView(Context context, AttributeSet attrs) {
        super(context, attrs);

        mRects = new LinkedList<TextRect>();
        mPaint = new Paint();
        mPaint.setTextAlign(Align.CENTER);
        mPaint.setAntiAlias(true);
    }

    public LinkedList<TextRect> getRects() {
        return new LinkedList<TextRect>(mRects);
    }

    public void clearRects() {
        mRects.clear();
    }

    public void addRect(String text, Rect rect) {
        if (text != null && rect != null) {
            TextRect entry = new TextRect(text, rect);
            mRects.add(entry);
        }
    }

    protected Matrix getScalingMatrix() {
        return mMatrix;
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);

        mWidth = w;
        mHeight = h;

        updateScaling();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        Drawable drawable = getDrawable();

        if (drawable != null) {
            mSrcWidth = drawable.getIntrinsicWidth();
            mSrcHeight = drawable.getIntrinsicHeight();
        }

        updateScaling();
    }

    protected void updateScaling() {
        if (mSrcWidth <= 0 || mSrcHeight <= 0 || mWidth <= 0 || mHeight <= 0) {
            return;
        }

        RectF src = new RectF(0, 0, mSrcWidth, mSrcHeight);
        RectF dst = new RectF(0, 0, mWidth, mHeight);

        mMatrix = new Matrix();
        mMatrix.setRectToRect(src, dst, ScaleToFit.CENTER);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        if (mMatrix == null) {
            return;
        }

        int saveCount = canvas.save();

        canvas.setMatrix(mMatrix);

        for (TextRect entry : mRects) {
            mPaint.setColor(0xDDFFFFFF);
            mPaint.setStyle(Style.FILL);
            canvas.drawRect(entry.rect, mPaint);

            mPaint.setColor(entry.color);
            mPaint.setStyle(Style.STROKE);
            mPaint.setStrokeWidth(3.0f);
            canvas.drawRect(entry.rect, mPaint);

            float w = entry.rect.width();
            float h = entry.rect.height();
            float textSize = 2.0f * h / 3.0f;
            float cx = entry.rect.left + w / 2.0f;
            float cy = entry.rect.top + textSize;

            mPaint.setColor(Color.BLACK);
            mPaint.setStyle(Style.FILL);
            mPaint.setTextSize(textSize);
            mPaint.setTextAlign(Align.CENTER);
            canvas.drawText(entry.text, cx, cy, mPaint);
        }

        canvas.restoreToCount(saveCount);
    }
}
