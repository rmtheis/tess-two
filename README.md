# tess-two

A fork of Tesseract Tools for Android 
([tesseract-android-tools][tesseract-android-tools]) that adds some
additional functions. Tesseract Tools for Android is a set of Android APIs and
build files for the [Tesseract OCR][tesseract-ocr] and [Leptonica][leptonica] 
image processing libraries.

This project works with Tesseract v3.05.00dev and Leptonica v1.73. The 
required source code for Tesseract and Leptonica is included within the 
`tess-two/jni` folder.

The `tess-two` module contains tools for compiling the Tesseract and Leptonica
libraries for use on the Android platform. It provides a Java API for accessing 
natively-compiled Tesseract and Leptonica APIs.

The `eyes-two` module contains additional image processing code copied from the
[eyes-free project][eyes-free]. It includes native functions for text detection,
blur detection, optical flow detection, and thresholding. Eyes-two is not needed
for using the Tesseract or Leptonica APIs.

The `tess-two-test` module contains instrumented unit tests for tess-two.

## Pre-requisites

* Android 2.2 or higher
* A v3.02+ [trained data file][tessdata] for a language. Data files must be 
extracted to the Android device in a subdirectory named `tessdata`.

## Usage

To use tess-two from your app, edit your app module's `build.gradle` file to add 
tess-two as an external dependency:

	dependencies {
	    compile 'com.rmtheis:tess-two:5.4.1'
	}
	
[Javadoc][javadoc] is available.

## Building

If you want to modify the tess-two code, or you want to use the eyes-two module,
you may build the project yourself locally. See [BUILDING.md](BUILDING.md).


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
[javadoc]: https://rmtheis.github.io/tess-two/javadoc/index.html
[semantic-versioning]: http://semver.org
[stackoverflow]: https://stackoverflow.com/
