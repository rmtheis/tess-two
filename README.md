#tess-two
* * *

A fork of Tesseract Tools for Android ([tesseract-android-tools](http://code.google.com/p/tesseract-android-tools/)) that adds some 
additional functions. Tesseract Tools for Android is a set of Android APIs and
build files for the [Tesseract OCR](https://code.google.com/p/tesseract-ocr/) and [Leptonica](http://www.leptonica.com/) image processing libraries.

This project works with Tesseract pre-release v3.04 and Leptonica v1.71. The required source code for Tesseract and
Leptonica is included within the `tess-two/src/main/jni` folder.

The `tess-two` subdirectory contains tools for compiling the Tesseract and Leptonica
libraries for use on the Android platform. It contains an Android
[library project](http://developer.android.com/tools/projects/projects-eclipse.html#ReferencingLibraryProject)
that provides a Java API for accessing natively-compiled Tesseract and Leptonica APIs.

## Requires

* Android 2.2 or higher
* A v3.02 [trained data file](https://code.google.com/p/tesseract-ocr/downloads/list) for a language. Data files must be extracted to a subdirectory named `tessdata`.

eyes-two
========

The `eyes-two` subdirectory contains a second, separate library project with additional image 
processing code copied from the [eyes-free project](http://code.google.com/p/eyes-free/) without 
modifications. It includes native functions for text detection, blurriness detection, optical flow 
detection, and thresholding. Building eyes-two is not necessary for using the Tesseract or Leptonica APIs.

While I haven't tested all the Eyes-two code, I've bundled it in this project alongside tess-two for
convenience due to its dependency on Leptonica. 

Build
=====

This project is set up to build on Android SDK Tools r22.3+ and Android NDK r10+. The build works on Linux, Mac OS X, and Windows 7/8. See [Issues](https://github.com/rmtheis/tess-two/issues) for reported build issues.

On 64-bit Ubuntu, you may need to install the `ia32-libs` 32-bit compatibility library.

To build native libs for `tess-two`, run the following commands in the terminal:

    git clone -b gradle https://github.com/dschuermann/tess-two.git tess-two
    cd tess-two/tess-two/src/main
    ndk-build
    cd ../../..
    ./gradlew build  # this builds the example project
    
Not tested
==========    
To build native libs for `eyes-two`, additionally run the following:

    cd eyes-two/src/main
    ndk-build

* `tess-two-test`

Maven
=====

While this project does not require Maven (and this project has not yet been registered in a Maven central repository), it can be [integrated into a local Maven repository for convenience](http://www.jameselsey.co.uk/blogs/techblog/tesseract-ocr-on-android-is-easier-if-you-maven-ise-it-works-on-windows-too/).

License
=======

This project is licensed under the [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0.html).

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

