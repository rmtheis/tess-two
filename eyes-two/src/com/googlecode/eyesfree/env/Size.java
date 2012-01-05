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

import android.hardware.Camera;
import android.text.TextUtils;

/**
 * Code stolen shamelessly from Camera.Size. This class is what Camera.Size
 * should have been but never was: independent of a Camera object. Unlike
 * Camera.Size, this class does not hold an unused reference to a Camera. It is
 * also comparable to allow sorting a list of sizes.
 *
 * @author Sharvil Nanavati
 */
public class Size implements Comparable<Size> {
    public final int width;
    public final int height;

    public Size(final int width, final int height) {
        this.width = width;
        this.height = height;
    }

    public Size(final Camera.Size s) {
        this.width = s.width;
        this.height = s.height;
    }

    public static Size parseFromString(String sizeString) {
        if (TextUtils.isEmpty(sizeString)) {
            return null;
        }

        sizeString = sizeString.trim();

        // The expected format is "<width>x<height>".
        final String[] components = sizeString.split("x");
        if (components.length == 2) {
            try {
                final int width = Integer.parseInt(components[0]);
                final int height = Integer.parseInt(components[1]);
                return new Size(width, height);
            } catch (final NumberFormatException e) {
                return null;
            }
        } else {
            return null;
        }
    }

    @Override
    public int compareTo(final Size other) {
        return width * height - other.width * other.height;
    }

    @Override
    public boolean equals(final Object other) {
        if (other == null) {
            return false;
        }

        if (!(other instanceof Size)) {
            return false;
        }

        final Size otherSize = (Size) other;
        return (width == otherSize.width && height == otherSize.height);
    }

    @Override
    public int hashCode() {
        return width * 32713 + height;
    }

    @Override
    public String toString() {
        return dimensionsAsString(width, height);
    }

    public static final String dimensionsAsString(final int width, final int height) {
        return width + "x" + height;
    }
}
