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

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.util.AttributeSet;
import android.view.View;

/**
 * Draws all debug info for FrameProcessors. Modified by Alan Viverette from
 * Andrew Harp's original source.
 *
 * @author Andrew Harp
 * @author alanv@google.com (Alan Viverette)
 */
public class DebugView extends View {
    private FrameLooper callback;

    public static boolean isVisible;

    public DebugView(final Context context, final AttributeSet attrs) {
        super(context, attrs);

        isVisible = (getVisibility() == View.VISIBLE);
    }

    @Override
    public synchronized void onDraw(final Canvas canvas) {
        canvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);

        if (callback != null) {
            callback.drawDebug(canvas);
        }
    }

    public synchronized void setCallback(final FrameLooper previewLooper) {
        callback = previewLooper;
    }

    public void toggleVisibility() {
        post(toggleVisible);
    }

    private final Runnable toggleVisible = new Runnable() {
        @Override
        public void run() {
            isVisible = !isVisible;

            setVisibility(isVisible ? View.VISIBLE : View.GONE);
        }
    };
}
