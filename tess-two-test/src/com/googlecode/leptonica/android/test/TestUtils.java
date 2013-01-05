/*
 * Copyright (C) 2012 Google Inc.
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

package com.googlecode.leptonica.android.test;

import android.graphics.Bitmap;

/**
 * Utility methods for running Leptonica unit tests.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class TestUtils {
    public static float compareBitmaps(Bitmap a, Bitmap b) {
        int found = 0;

        for (int y = 0; y < a.getHeight(); y++) {
            for (int x = 0; x < a.getWidth(); x++) {
                if (a.getPixel(x, y) == a.getPixel(x, y)) {
                    found++;
                }
            }
        }

        return found / (float)(a.getWidth() * a.getHeight());
    }
}