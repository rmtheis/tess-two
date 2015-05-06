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

import android.graphics.Bitmap;
import android.graphics.Rect;

/**
 * @author alanv@google.com (Alan Viverette)
 */
public class Result {
    private final Bitmap mBitmap;
    private final Rect mRect;

    public Result(final Bitmap bitmap, final Rect rect) {
        mBitmap = bitmap;
        mRect = rect;
    }

    public Bitmap getBitmap() {
        return mBitmap;
    }

    public Rect getBoundingBox() {
        return mRect;
    }

    public boolean hasBoundingBox() {
        return (mRect != null);
    }
}
