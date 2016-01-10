# tess-two

A fork of Tesseract Tools for Android 
([tesseract-android-tools][tesseract-android-tools]) that adds some
additional functions. Tesseract Tools for Android is a set of Android APIs and
build files for the [Tesseract OCR][tesseract-ocr] and [Leptonica][leptonica] 
image processing libraries.

This project works with Tesseract v3.04.00 and Leptonica v1.72. The 
required source code for Tesseract and Leptonica is included within the 
`tess-two/jni` folder.

The `tess-two` module contains tools for compiling the Tesseract and Leptonica
libraries for use on the Android platform. It provides a Java API for accessing 
natively-compiled Tesseract and Leptonica APIs.

The `eyes-two` module contains additional image processing code copied from the
[eyes-free project][eyes-free]. It includes native functions for text detection,
blur detection, optical flow detection, and thresholding. Eyes-two is not needed
for using the Tesseract or Leptonica APIs.

The `tess-two-test` subdirectory contains Android JUnit tests.

## Pre-requisites

* Android 2.2 or higher
* A v3.02+ [trained data file][tessdata] for a language. Data files must be 
extracted to the Android device in a subdirectory named `tessdata`.

## Usage

To use tess-two from your app, edit your app's `build.gradle` file to add 
tess-two as an external dependency:

	dependencies {
	    compile 'com.rmtheis:tess-two:5.4.0'
	}

## Building

If you want to modify the tess-two code, or you want to use the eyes-two module,
you may build the project yourself and include it in your app.

**_Android Studio and Gradle_**

The Gradle build uses the gradle-stable plugin and the "bundled" Android NDK
through a call to `ndk-build` in `build.gradle`. After building, the AAR file
that's generated may be [imported][aar-import] into your app project as a
dependency on a local binary package.

Type the following commands in the terminal to build the project from the 
command line:

_On Mac/Linux:_
	
    export ANDROID_HOME=/path/to/your/android-sdk
    git clone git://github.com/rmtheis/tess-two tess
    cd tess
    android update project --path tess-two
    cp tess-two/local.properties .
    ./gradlew assemble
		
_On Windows:_
		
    set ANDROID_HOME=C:\\path\\to\\your\\android-sdk
    git clone git://github.com/rmtheis/tess-two tess
    cd tess
    android update project --path tess-two
    copy tess-two\local.properties .
    gradlew assemble

**_Eclipse and Ant_**

Versions up to 5.4.0 may be built as a library project using the standalone
Android NDK and imported into Eclipse using File->Import->Existing Projects into
Workspace.

On 64-bit Ubuntu, you may need to install the `ia32-libs` 32-bit compatibility 
library.

To build tess-two, run the following commands in the terminal:

    git clone git://github.com/rmtheis/tess-two tess
    cd tess
    git checkout tags/5.4.0
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

If you're using ProGuard for code shrinking and obfuscation, manually add the 
ProGuard keep options from the `proguard-rules.pro` file to your app's ProGuard
config in order to retain fields and methods used by native code.

## Versions

Release points are tagged with [version numbers][semantic-versioning]. A change 
to the major version number indicates an API change making that version 
incompatible with previous versions.

## Support

* Stack Overflow: https://stackoverflow.com/questions/tagged/tess-two
* tesseract-ocr: https://groups.google.com/forum/#!forum/tesseract-ocr

If you've found an error in this project, please file an issue.

Patches are encouraged, and may be submitted by forking this project and 
submitting a pull request through GitHub. 

## License

    Copyright 2011 Robert Theis

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.


[tesseract-android-tools]: https://github.com/alanv/tesseract-android-tools
[tesseract-ocr]: https://github.com/tesseract-ocr/tesseract
[leptonica]: http://www.leptonica.com/
[eyes-free]: https://code.google.com/p/eyes-free/
[tessdata]: https://github.com/tesseract-ocr/tessdata
[aar-import]:http://stackoverflow.com/a/28816265/667810
[semantic-versioning]: http://semver.org
[stackoverflow]: https://stackoverflow.com/