#tess-two
* * *

A fork of Tesseract Tools for Android ([tesseract-android-tools](http://code.google.com/p/tesseract-android-tools/)) that adds some 
additional functions. Tesseract Tools for Android is a set of Android APIs and
build files for the Tesseract OCR and Leptonica image processing libraries.

This project works with Tesseract v3.01. Source code for Tesseract 3.01 and
the other dependencies is included in the tess-two/external folder.

This API adds the following methods on top of tesseract-android-tools r6 to
enable retrieving bounding boxes for words and characters recognized using OCR:

* TessBaseAPI::GetRegions()
* TessBaseAPI::GetTextlines()
* TessBaseAPI::GetWords()
* TessBaseAPI::GetCharacters()

Note: GetTextlines(), GetWords() and GetCharacters() work well, but I have not gotten good 
results from Tesseract when calling GetRegions().


Quickstart
==========




Build
=====

_(Adapted from the tesseract-android-tools README)_

This project contains tools for compiling the Tesseract, Leptonica, and JPEG
libraries for use on the Android platform. It contains an Eclipse Android
[library project](http://developer.android.com/guide/developing/projects/projects-eclipse.html#SettingUpLibraryProject) 
that provides a Java API for accessing natively-compiled Tesseract and Leptonica APIs.

This project is set up to build on Android SDK Tools r14/r15.

To build this project, run the following commands in the terminal:

    cd <project-directory>/tess-two
    export TESSERACT_PATH=${PWD}/external/tesseract-3.01
    export LEPTONICA_PATH=${PWD}/external/leptonica-1.68
    export LIBJPEG_PATH=${PWD}/external/libjpeg
    ndk-build
    android update project --path .
    ant compile

You may also want to edit jni/Android.mk to reflect the correct library source
directories and avoid running "export" every time you run ndk-build.

Test
====




License
=======

tess-two is licensed under the [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0.html)

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

 
This project contains other third party software in the "external" folder, with separate license agreements:

* Tesseract 3.01 (Modified to add TessBaseAPI::GetCharacters())
* Leptonica 1.68 (Unmodified)
* LibJPEG 6b (Unmodified)
