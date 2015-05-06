#android-ocr
* * *

An experimental app for Android that performs optical character recognition (OCR) on images captured using the device camera.

Runs the Tesseract OCR engine using a fork of Tesseract Tools for Android.

Most of the code making up the core structure of this project has been adapted from the ZXing Barcode Scanner. Along with Tesseract-OCR and Tesseract Tools for Android (tesseract-android-tools), several open source projects have been used in this project, including leptonica, google-api-translate-java, microsoft-translator-java-api, and jtar.

## Requires

* Building [tess-two](https://github.com/rmtheis/tess-two), to act as the OCR engine, by following its [build instructions](https://github.com/rmtheis/tess-two#build).
* A Windows Azure Marketplace Client ID and Client Secret (for translation) - [Documentation](http://msdn.microsoft.com/en-us/library/hh454950.aspx)
* A Google Translate API key (for translation) - [Documentation](https://code.google.com/apis/console/?api=translate)

Installing the APK
==================

The APK is available for download to an Android device from Android Market [here](https://market.android.com/details?id=edu.sfsu.cs.orange.ocr).

License
=======

This project is licensed under the [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0.html)

    /*
     * Copyright 2011 Robert Theis
     *
     * Licensed under the Apache License, Version 2.0 (the "License");
     * you may not use this file except in compliance with the License.
     * You may obtain a copy of the License at
     *
     *      http://www.apache.org/licenses/LICENSE-2.0
     *
     * Unless required by applicable law or agreed to in writing, software
     * distributed under the License is distributed on an "AS IS" BASIS,
     * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     * See the License for the specific language governing permissions and
     * limitations under the License.
     */

One of the jar files in the android/libs directory (google-api-translate-java-0.98-mod2.jar) is licensed under the [GNU Lesser GPL](http://www.gnu.org/licenses/lgpl.html).
