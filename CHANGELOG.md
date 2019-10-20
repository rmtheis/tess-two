# Change Log

## [9.1.0](https://github.com/rmtheis/tess-two/tree/9.1.0) (2019-10-19)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/9.0.0...9.1.0)

**Fixed bugs:**

- WriteFile.writeBytes altering the source image endiannes and producing artefacts [\#228](https://github.com/rmtheis/tess-two/issues/228)
- Crashing reported on 64-bit devices [\#197](https://github.com/rmtheis/tess-two/issues/197)
- testPixaReplacePix crashes in native code [\#159](https://github.com/rmtheis/tess-two/issues/159)
- ReadFile.readBitmap alters image colors [\#87](https://github.com/rmtheis/tess-two/issues/87)

**Closed issues:**

- read Jpeg and PNG produce wrong alpha in 32bpp pix [\#264](https://github.com/rmtheis/tess-two/issues/264)
- Native crush when 'vert'.tessdata is used [\#263](https://github.com/rmtheis/tess-two/issues/263)
- In versions higher than 5.4.1 TessBaseAPI.ProgressNotifier never called [\#262](https://github.com/rmtheis/tess-two/issues/262)
- Decoding is slow when multiple languages are used [\#261](https://github.com/rmtheis/tess-two/issues/261)
- Getting different results when using tesseract on mobile vs on PC using Python. [\#260](https://github.com/rmtheis/tess-two/issues/260)
-  Illegible words recognition in Persian lang   [\#259](https://github.com/rmtheis/tess-two/issues/259)
- Could not initialize Tesseract API with language=ces! [\#258](https://github.com/rmtheis/tess-two/issues/258)
- Can't lanch [\#257](https://github.com/rmtheis/tess-two/issues/257)
- Tess4 initialize crash [\#255](https://github.com/rmtheis/tess-two/issues/255)
- tess4 on android studio [\#254](https://github.com/rmtheis/tess-two/issues/254)
- How I can init the tesseract to work only with digits? [\#253](https://github.com/rmtheis/tess-two/issues/253)
- java.lang.NoSuchFieldError: no "I" field "mNativeData" in class "Lcom/googlecode/tesseract/android/TessBaseAPI;" or its superclasses [\#252](https://github.com/rmtheis/tess-two/issues/252)
- java.lang.NoSuchFieldError: no "I" field "mNativeData" in class "Lcom/googlecode/tesseract/android/TessBaseAPI;" or its superclasses [\#251](https://github.com/rmtheis/tess-two/issues/251)
- Arabic trained-data produce 20% accuracy  [\#250](https://github.com/rmtheis/tess-two/issues/250)
- import tess two error [\#249](https://github.com/rmtheis/tess-two/issues/249)
- error in Android 7.0 [\#248](https://github.com/rmtheis/tess-two/issues/248)
- How to build tess-two without JNI? [\#247](https://github.com/rmtheis/tess-two/issues/247)
- Initializing TessBaseAPI crashes app [\#246](https://github.com/rmtheis/tess-two/issues/246)
- OCR number [\#245](https://github.com/rmtheis/tess-two/issues/245)
- compile binary only [\#244](https://github.com/rmtheis/tess-two/issues/244)
- Skip tesseract's default image preprocessing \(Otsu\) [\#243](https://github.com/rmtheis/tess-two/issues/243)
- How to add OPTITypewriter-Special font [\#242](https://github.com/rmtheis/tess-two/issues/242)
- Special requirements for Hindi and Arabic OCR [\#239](https://github.com/rmtheis/tess-two/issues/239)

**Merged pull requests:**

- fix Java tests for leptronica [\#265](https://github.com/rmtheis/tess-two/pull/265) ([alexcohn](https://github.com/alexcohn))
- Update to support latest Android Studio [\#256](https://github.com/rmtheis/tess-two/pull/256) ([Robyer](https://github.com/Robyer))

## [9.0.0](https://github.com/rmtheis/tess-two/tree/9.0.0) (2018-04-20)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/8.0.0...9.0.0)

**Change:**

- This version fixes and adds back in the 64-bit ABIs, and removes the deprecated armv5 and mips/mips64 ABIs.

**Implemented enhancements:**

- Improve developer support for Hindi/Arabic [\#240](https://github.com/rmtheis/tess-two/issues/240)

**Fixed bugs:**

- Crashing reported on 64-bit devices [\#197](https://github.com/rmtheis/tess-two/issues/197)

**Merged pull requests:**

- Updated repositories. Changed compile to implementation. [\#233](https://github.com/rmtheis/tess-two/pull/233) ([mauriciotogneri](https://github.com/mauriciotogneri))
- Updated version of SDK, build tools and support libraries [\#231](https://github.com/rmtheis/tess-two/pull/231) ([mauriciotogneri](https://github.com/mauriciotogneri))

## [8.0.0](https://github.com/rmtheis/tess-two/tree/8.0.0) (2017-08-13)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/7.0.0...8.0.0)

**Change:**

- This version removes the 64-bit ABIs from the default build in order to avoid crashes due to [\#197](https://github.com/rmtheis/tess-two/issues/197)

**Closed issues:**

- Build failure with Android Studio 3 and NDK ver. 15.x [\#215](https://github.com/rmtheis/tess-two/issues/215)

**Merged pull requests:**

- clone tag 3.04.00 from tessdata [\#217](https://github.com/rmtheis/tess-two/pull/217) ([ivankolev](https://github.com/ivankolev))
- See issue \#215 [\#216](https://github.com/rmtheis/tess-two/pull/216) ([ivankolev](https://github.com/ivankolev))

## [7.0.0](https://github.com/rmtheis/tess-two/tree/7.0.0) (2017-06-12)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/6.3.0...7.0.0)

**API-breaking change:**

- ResultIterator getChoicesAndConfidence\(\) is now getSymbolChoicesAndConfidence\(\)

**Fixed bugs:**

- ResultIterator crashes with certain character combinations [\#126](https://github.com/rmtheis/tess-two/issues/126)
- ResultIterator returns single Character as Alternatives for Words [\#119](https://github.com/rmtheis/tess-two/issues/119)
- beginDocument\(\) returns false even when successful [\#103](https://github.com/rmtheis/tess-two/issues/103)

**Closed issues:**

- Hardcoded path in TessBaseAPITest.java [\#208](https://github.com/rmtheis/tess-two/issues/208)

**Merged pull requests:**

- Add missing PageSegMode constant [\#209](https://github.com/rmtheis/tess-two/pull/209) ([Robyer](https://github.com/Robyer))
- Fix return value of BeginDocument \(fixes \#103\) [\#207](https://github.com/rmtheis/tess-two/pull/207) ([Robyer](https://github.com/Robyer))
- Improve ResultIterator [\#206](https://github.com/rmtheis/tess-two/pull/206) ([Robyer](https://github.com/Robyer))
- Update gradle plugin [\#205](https://github.com/rmtheis/tess-two/pull/205) ([Robyer](https://github.com/Robyer))
- Fix javadocs errors and warnings [\#202](https://github.com/rmtheis/tess-two/pull/202) ([Robyer](https://github.com/Robyer))

## [6.3.0](https://github.com/rmtheis/tess-two/tree/6.2.0) (2017-04-06)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/6.2.0...6.3.0)

**Updates:**

- Tesseract code updated to version 3.05.00

[Tesseract change log](https://github.com/tesseract-ocr/tesseract/blob/cf0b378577e7ed0c75bfaf97cae7e35d7634cf4d/ChangeLog#L22)

## [6.2.0](https://github.com/rmtheis/tess-two/tree/6.2.0) (2017-02-04)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/6.1.1...6.2.0)

**Updates:**

- Leptonica 1.74.1
- Libpng 1.6.25

**Fixed bugs:**

- Stop\(\) does not work with GetUTF8Text\(\) [\#185](https://github.com/rmtheis/tess-two/issues/185)
- TessBaseAPI stop\(\) sometimes causes SIGSEGV [\#97](https://github.com/rmtheis/tess-two/issues/97)

**Merged pull requests:**

- Issue 185 [\#186](https://github.com/rmtheis/tess-two/pull/186) ([Xyresic](https://github.com/Xyresic))

## [6.1.1](https://github.com/rmtheis/tess-two/tree/6.1.1) (2016-11-27)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/6.1.0...6.1.1)

**Updates:**

- Changed ProGuard settings
 
## [6.1.0](https://github.com/rmtheis/tess-two/tree/6.1.0) (2016-11-22)
[Full Changelog](https://github.com/rmtheis/tess-two/compare/6.0.4...6.1.0)

**Updates:**

- Tesseract code updated, version 3.05.00dev

**Closed issues:**
 
- Tests doesn't work on devices without sdcard [\#171](https://github.com/rmtheis/tess-two/issues/171)
- no field with name='mNativeData' signature='J' in class Lcom/googlecode/tesseract/android/TessBaseAPI; [\#166](https://github.com/rmtheis/tess-two/issues/166)
 
**Merged pull requests:**
 
- Pass native pointer as parameter [\#172](https://github.com/rmtheis/tess-two/pull/172) ([jereksel](https://github.com/jereksel))

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

- Progress values no longer available when using getUTF8Text. Use getHOCRText instead.
- Android 2.2 (API level 8) no longer supported
- Eyes-two project refactored. Code similar to what was removed is available from the [Google Mobile Vision API](https://developers.google.com/vision/).
- Tess-two deprecated methods removed

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

