#tess-two
* * *

A fork of Tesseract Tools for Android ([tesseract-android-tools](http://code.google.com/p/tesseract-android-tools/)) that adds some 
additional functions. Tesseract Tools for Android is a set of Android APIs and
build files for the [Tesseract OCR](https://code.google.com/p/tesseract-ocr/) and [Leptonica](http://www.leptonica.com/) image processing libraries.

This project works with Tesseract v3.03. The required source code for Tesseract 3.03 and
Leptonica 1.70 is included within the `tess-two/jni` folder.

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
detection, and thresholding. Building eyes-two is not necessary for using the Tesseract API.

While I haven't tested all the Eyes-two code, I've bundled it in this project alongside tess-two for
convenience due to its dependency on Leptonica. 

Build
=====

This project is set up to build on Android SDK Tools r22.3+ and Android NDK r9c+. The build works on Linux, Mac OS X, and Windows 7/8. See [Issues](https://github.com/rmtheis/tess-two/issues) for reported build issues.

On 64-bit Ubuntu, you may need to install the `ia32-libs` 32-bit compatibility library.

To build tess-two, run the following commands in the terminal:

    git clone git://github.com/rmtheis/tess-two tess
    cd tess
    cd tess-two
    ndk-build
    android update project --path .
    ant release

To build eyes-two, additionally run the following:

    cd ..
    cd eyes-two
    ndk-build
    android update project --path .
    ant release

After building, the tess-two and eyes-two projects can be imported into Eclipse using 
File->Import->Existing Projects into Workspace.

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

