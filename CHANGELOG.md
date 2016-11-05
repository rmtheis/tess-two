# Change Log

## [Unreleased](https://github.com/rmtheis/tess-two/tree/HEAD)

[Full Changelog](https://github.com/rmtheis/tess-two/compare/6.0.4...HEAD)

## [6.0.4](https://github.com/rmtheis/tess-two/tree/6.0.4) (2016-08-21)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/6.0.3...6.0.4)

**Updates:**

- Changed ProGuard settings

**Closed issues:**

- android studio2.1.2 not build [\#168](https://github.com/rmtheis/tess-two/issues/168)
- no field with name='mNativeData' signature='J' in class Lcom/googlecode/tesseract/android/TessBaseAPI; [\#166](https://github.com/rmtheis/tess-two/issues/166)

## [6.0.3](https://github.com/rmtheis/tess-two/tree/6.0.3) (2016-07-16)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/6.0.2...6.0.3)


**Closed issues:**

- UnsatisfiedLinkError; thrown while initializing Lcom/googlecode/tesseract/android/TessBaseAPI 'libjpgt.so' and 'liblept.so' w/ compile 'com.rmtheis:tess-two:6.0.0' [\#150](https://github.com/rmtheis/tess-two/issues/150)

## [6.0.2](https://github.com/rmtheis/tess-two/tree/6.0.2) (2016-06-20)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/6.0.1...6.0.2)

**Closed issues:**

- Building project error on Windows - javadoc failed [\#148](https://github.com/rmtheis/tess-two/issues/148)

**Merged pull requests:**

- Refactor nativeMergeAndReplacePix. Addresses \#132. [\#157](https://github.com/rmtheis/tess-two/pull/157) ([megabytefisher](https://github.com/megabytefisher))
- Fix memory leak in setImage\(Bitmap bmp\) [\#154](https://github.com/rmtheis/tess-two/pull/154) ([megabytefisher](https://github.com/megabytefisher))

## [6.0.1](https://github.com/rmtheis/tess-two/tree/6.0.1) (2016-06-09)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/6.0.0...6.0.1)

**Bug fix:**

 - Fixed nativeGetPix for 64-bit devices

## [6.0.0](https://github.com/rmtheis/tess-two/tree/6.0.0) (2016-05-16)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/5.4.1...6.0.0)

**API-breaking changes:** 

- Progress values no longer available when using GetHOCRText. Use GetUTF8Text instead.
- Android 2.2 (API level 8) no longer supported
- Eyes-two project refactored

**Updates:**

- Tesseract 3.05.00dev (tesseract-ocr/tesseract@add1ed1)
- Leptonica 1.73
- Libpng 1.6.20 (android/platform_external_libpng@2789184)

**Implemented enhancements:**

- GetUTF8Text\(\) monitor and multi-platform projects [\#116](https://github.com/rmtheis/tess-two/issues/116)
- Update eyes-two structure [\#95](https://github.com/rmtheis/tess-two/issues/95)
- Expand image I/O format support [\#94](https://github.com/rmtheis/tess-two/issues/94)

**Fixed bugs:**

- NDK r11 clang build error: undefined reference to isnanf/\_\_isinff [\#138](https://github.com/rmtheis/tess-two/issues/138)
- init\(\) crashes when using OEM\_TESSERACT\_CUBE\_COMBINED for Arabic [\#12](https://github.com/rmtheis/tess-two/issues/12)


## [5.4.1](https://github.com/rmtheis/tess-two/tree/5.4.1) (2016-01-17)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/5.4.0...5.4.1)

**Updates:**

- Added libjpeg 9b

**Closed issues:**

- Pdf renderer isn't working with jpg input images [\#122](https://github.com/rmtheis/tess-two/issues/122)

## [5.4.0](https://github.com/rmtheis/tess-two/tree/5.4.0) (2016-01-10)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/5.3.0...5.4.0)

**Updates:**

- Added Gradle build
- Released on Bintray

**Closed issues:**

- Add Maven support [\#53](https://github.com/rmtheis/tess-two/issues/53)

**Merged pull requests:**

- Gradle build [\#99](https://github.com/rmtheis/tess-two/pull/99) ([Aaargh20318](https://github.com/Aaargh20318))


## [5.3.0](https://github.com/rmtheis/tess-two/tree/5.3.0) (2015-10-30)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/5.2.0...5.3.0)

**Implemented enhancements:**

- Added TessPdfRenderer for PDF output [\#46](https://github.com/rmtheis/tess-two/issues/46)
- Added libpng 1.6.10 (android/platform_external_libpng@37f83eb)

**Closed issues:**

- java.lang.UnsatisfiedLinkError: dlopen failed: cannot locate symbol "png\_set\_longjmp\_fn" referenced by "liblept.so"... [\#106](https://github.com/rmtheis/tess-two/issues/106)
- "'libpng.so' not found" message on some 4.x versions of Android [\#105](https://github.com/rmtheis/tess-two/issues/105)
- libpng fails to build on ARMv8 [\#102](https://github.com/rmtheis/tess-two/issues/102)

**Merged pull requests:**

- fix 64-bit ARMv8 build [\#124](https://github.com/rmtheis/tess-two/pull/124) ([panzerfahrer](https://github.com/panzerfahrer))
- Bugfix for progress notifier [\#115](https://github.com/rmtheis/tess-two/pull/115) ([FDIM](https://github.com/FDIM))

## [5.2.0](https://github.com/rmtheis/tess-two/tree/5.2.0) (2015-07-21)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/5.1.0...5.2.0)

**Updates:**

- Added support for using multiple training data files/languages
- Tesseract 3.04 (tesseract-ocr/tesseract@e8b6d6f)
- Leptonica 1.72

**Fixed bugs:**

- Fix finalizers [\#88](https://github.com/rmtheis/tess-two/issues/88)
- WriteFile.writeImpliedFormat fails for jpegs [\#86](https://github.com/rmtheis/tess-two/issues/86)
- WriteFile don't work as expected [\#25](https://github.com/rmtheis/tess-two/issues/25)

**Closed issues:**

- clang "atomicity.h:49: error: undefined reference to '\_\_atomic\_fetch\_add\_4''" for armeabi [\#81](https://github.com/rmtheis/tess-two/issues/81)
- Android \(ART\) crash with error JNI DETECTED ERROR IN APPLICATION: jarray is an invalid stack indirect reference table or invalid reference [\#78](https://github.com/rmtheis/tess-two/issues/78)

## [5.1.0](https://github.com/rmtheis/tess-two/tree/5.1.0) (2015-03-16)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/5.0.0...5.1.0)

 **Updates:**
 
 - Added 64-bit ABI support
 - Tesseract 3.04 (tesseract-ocr/tesseract@239f350)

**Added to Tesseract API:**
 
 - ProgressNotifier for getting OCR percent complete values
 - ChoiceIterator support for getting per-symbol alternatives
 - GetConnectedComponents
  
**Added to Leptonica API:**

 - Edge.pixSobelEdgeFilter
 - GrayQuant.pixThresholdToBinary
 - Pix.clipRectangle
 - Pix.pixFastTophat
 - Pix.pixTophat
 - Pix.rotateOrth
 - Pix.scaleWithoutSharpening

## [5.0.0](https://github.com/rmtheis/tess-two/tree/5.0.0) (2014-08-13)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/4.0.0...5.0.0)

**Updates:**

- Tesseract 3.03
- Leptonica 1.71

## [4.0.0](https://github.com/rmtheis/tess-two/tree/4.0.0) (2014-02-17)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/3.0.0...4.0.0)

**Updates:**

- Tesseract 3.03
- Leptonica 1.70

**Merged pull requests:**

- Add some useful functions [\#40](https://github.com/rmtheis/tess-two/pull/40) ([ductranit](https://github.com/ductranit))

## [3.0.0](https://github.com/rmtheis/tess-two/tree/3.0.0) (2013-01-22)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/2.0.0...3.0.0)

**Updates:**

- Tesseract 3.03
- Leptonica 1.69

## [2.0.0](https://github.com/rmtheis/tess-two/tree/2.0.0) (2012-05-31)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/1.0.0...2.0.0)

**Updates:**

- Eyes-free project added
- Tesseract 3.02

## [1.0.0](https://github.com/rmtheis/tess-two/tree/1.0.0) (2011-11-06)

- Initial release, branched from tesseract-android-tools project
- Tesseract 3.01
- Leptonica 1.68


\* *This change log was generated by [github_changelog_generator](https://github.com/skywinder/Github-Changelog-Generator)*

