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

package com.googlecode.tesseract.android.test;

import android.test.suitebuilder.TestSuiteBuilder;

import com.googlecode.leptonica.android.test.ReadFileTest;

import junit.framework.Test;
import junit.framework.TestSuite;

/**
 * To run all suites found in this apk:
 * $ adb shell am instrument -w \
 *   com.googlecode.tesseract.android.test/android.test.InstrumentationTestRunner
 *
 * To run just this suite from the command line:
 * $ adb shell am instrument -w \
 *   -e class com.googlecode.tesseract.android.test.AllTests \
 *   com.googlecode.tesseract.android.test/android.test.InstrumentationTestRunner
 *
 * To run an individual test case, e.g. {@link ReadFileTest}:
 * $ adb shell am instrument -w \
 *   -e class com.googlecode.leptonica.android.test.ReadFileTest \
 *   com.googlecode.tesseract.android.test/android.test.InstrumentationTestRunner
 *
 * To run an individual test, e.g. {@link ReadFileTest#testReadBitmap_640x480()}:
 * $ adb shell am instrument -w \
 *   -e class com.googlecode.leptonica.android.test.ReadFileTest#testReadBitmap \
 *   com.googlecode.tesseract.android.test/android.test.InstrumentationTestRunner
 */
public class AllTests extends TestSuite {
    public static Test suite() {
        return new TestSuiteBuilder(AllTests.class)
                .includeAllPackagesUnderHere()
                .build();
    }
}
