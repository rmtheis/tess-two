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

import android.graphics.PixelFormat;

import com.googlecode.leptonica.android.Convert;
import com.googlecode.leptonica.android.Pix;
import com.googlecode.leptonica.android.WriteFile;

/**
 * Interface for an object that produces image frames on a request basis.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public interface FrameProducer {
    public int getFrameWidth();

    public int getFrameHeight();

    public void requestFrame(FrameReceiver listener);

    public interface FrameReceiver {
        public void onFrameReceived(Frame frame);
    }

    public static class Frame {
        public final byte[] data;

        public final int width;

        public final int height;

        public final int format;

        public final long timestamp;

        public Frame(Pix pix, long timestamp) {
            if (pix.getDepth() != 8) {
                Pix pix8 = Convert.convertTo8(pix);
                this.data = WriteFile.writeBytes8(pix);
                pix8.recycle();
            } else {
                this.data = WriteFile.writeBytes8(pix);
            }

            this.width = pix.getWidth();
            this.height = pix.getHeight();
            this.format = PixelFormat.L_8;
            this.timestamp = timestamp;
        }

        public Frame(byte[] data, int width, int height, int type, long timestamp) {
            this.data = data;
            this.width = width;
            this.height = height;
            this.format = type;
            this.timestamp = timestamp;
        }

        public void recycle() {
            // Does nothing in this implementation.
        }
    }
}
