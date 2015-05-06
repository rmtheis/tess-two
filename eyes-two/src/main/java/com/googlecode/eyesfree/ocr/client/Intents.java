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

package com.googlecode.eyesfree.ocr.client;

import android.content.Intent;

/**
 * This class enumerates the Intents made available to application developers
 * through the OCR Service.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public final class Intents {
    private Intents() {
        // This class is not instantiable.
    }

    public static final class Actions {
        /**
         * This action is broadcast when the list of installed languages has
         * been updated.
         */
        public static final String LANGUAGES_UPDATED =
                "com.googlecode.eyesfree.ocr.action.LANGUAGES_UPDATED";

        private Actions() {
            // This class is not instantiable.
        }
    }

    public static final class Service {
        /**
         * Use this to bind to the OCR service. Typically this will only be used
         * by the Ocr object.
         */
        public static final String ACTION = "com.googlecode.eyesfree.ocr.intent.SERVICE";
        public static final String CATEGORY = Intent.CATEGORY_DEFAULT;

        private Service() {
            // This class is not instantiable.
        }
    }

    public static final class Languages {
        /**
         * Use this to bind to the OCR service. Typically this will only be used
         * by the Ocr object.
         */
        public static final String ACTION = "com.googlecode.eyesfree.ocr.intent.LANGUAGES";

        private Languages() {
            // This class is not instantiable.
        }
    }

    protected static class BaseCapture {
        /**
         * The desired picture width as an integer. Use Intent.putExtra(WIDTH,
         * width) where width is the desired picture width. If set, you must
         * also set the picture height.
         */
        public static final String EXTRA_WIDTH = "width";

        /**
         * The desired picture height as an integer. Use Intent.putExtra(HEIGHT,
         * height) where height is the desired picture height. If set, you must
         * also set the picture width.
         */
        public static final String EXTRA_HEIGHT = "height";

        /**
         * Whether the camera light should be used when previewing and taking a
         * picture. Use Intent.putExtra(FLASHLIGHT, flashlight) where flashlight
         * is a boolean value.
         */
        public static final String EXTRA_FLASHLIGHT = "flashlight";

        /**
         * Whether the camera flash should be used when taking a picture. Use
         * Intent.putExtra(FLASH_MODE, flashMode) where flashMode is a constant
         * from Camera.Parameters.FLASH_
         */
        public static final String EXTRA_FLASH_MODE = "flash_mode";

        /**
         * The name of the Intent-extra used to indicate a content resolver Uri
         * to be used to store the requested image.
         */
        public static final String EXTRA_OUTPUT = "output";
    }

    public static final class Detect extends BaseCapture {
        /**
         * Send this intent to open the continuous text detection screen.
         */
        public static final String ACTION = "com.googlecode.eyesfree.ocr.intent.DETECT";

        /**
         * The name of the Intent-extra used to return the list of text area
         * boundaries detected in the requested image.
         */
        public static final String EXTRA_OUTPUT_BOUNDS = "output_bounds";

        private Detect() {
            // This class is not instantiable.
        }
    }

    protected static class BaseRecognize {
        /**
         * The name of the Intent-extra used to indicate an absolute file system
         * path from which to read the image to be recognized.
         */
        public static final String EXTRA_INPUT = "input";

        /**
         * The name of the Intent-extra used to indicate the recognition
         * parameters to be used for recognition.
         */
        public static final String EXTRA_PARAMETERS = "parameters";

        /**
         * The name of the Intent-extra used to indicate the results of
         * recognition as an ArrayList of Result objects.
         */
        public static final String EXTRA_RESULTS = "results";

        private BaseRecognize() {
            // This class is not instantiable.
        }
    }

    public static final class Recognize extends BaseRecognize {
        /**
         * Send this intent to process an OCR configuration object and receive
         * OCR results in return.
         */
        public static final String ACTION = "com.googlecode.eyesfree.ocr.intent.RECOGNIZE";

        private Recognize() {
            // This class is not instantiable.
        }
    }
}
