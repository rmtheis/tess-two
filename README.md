# tess-two

A fork of Tesseract Tools for Android 
([tesseract-android-tools][tesseract-android-tools]) that adds some
additional functions. Tesseract Tools for Android is a set of Android APIs and
build files for the [Tesseract OCR][tesseract-ocr] and [Leptonica][leptonica] 
image processing libraries.

This project works with:

 - Tesseract 3.05
 - Leptonica 1.74.1
 - libjpeg 9b
 - libpng 1.6.25
 
The source code for these dependencies is included within the
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

* Android 2.3 or higher
* A v3.04 [trained data file][tessdata] for a language. Data files must be 
copied to the Android device in a subdirectory named `tessdata`.

## Usage

To use tess-two from your app, edit your app module's `build.gradle` file to add 
tess-two as an external dependency:

	dependencies {
	    implementation 'com.rmtheis:tess-two:9.0.0'
	}
	
[Javadoc][javadoc] is available.

## Building

If you want to modify the tess-two code, or you want to use the eyes-two module,
you may build the project yourself locally. See [BUILDING.md](BUILDING.md).


## Versions

Release points are tagged with [version numbers][semantic-versioning]. A change 
to the major version number indicates an API change making that version 
incompatible with previous versions.

The [change log](CHANGELOG.md) shows what's new in each version.

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

## See Also

The Google Mobile Vision API team has made available an OCR capability that is
provided to developers through the Google Play Services library: The
[Mobile Vision API][mobile-vision-api] offers a simple API for OCR that
currently works for Latin-based characters.

[tesseract-android-tools]: https://github.com/alanv/tesseract-android-tools
[tesseract-ocr]: https://github.com/tesseract-ocr/tesseract
[leptonica]: https://github.com/DanBloomberg/leptonica
[eyes-free]: https://github.com/rmtheis/eyes-free
[tessdata]: https://github.com/tesseract-ocr/tessdata/tree/3.04.00
[javadoc]: https://rmtheis.github.io/tess-two/javadoc/index.html
[semantic-versioning]: http://semver.org
[stackoverflow]: https://stackoverflow.com/
[mobile-vision-api]: https://developers.google.com/vision/
