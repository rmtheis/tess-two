# tess-two

A fork of Tesseract Tools for Android 
([tesseract-android-tools][tesseract-android-tools]) that adds some
additional functions. Tesseract Tools for Android is a set of Android APIs and
build files for the [Tesseract OCR][tesseract-ocr] and [Leptonica][leptonica] 
image processing libraries.

This project works with Tesseract pre-release v3.04 and Leptonica v1.72. The 
required source code for Tesseract and Leptonica is included within the 
`tess-two/jni` folder.

The `tess-two` subdirectory contains tools for compiling the Tesseract and 
Leptonica libraries for use on the Android platform. It contains an Android 
[library project][library-project] that provides a Java API for accessing 
natively-compiled Tesseract and Leptonica APIs.

The `eyes-two` subdirectory contains a second, separate library project with 
additional image processing code copied from the [eyes-free project][eyes-free].
It includes native functions for text detection, blur detection, optical flow 
detection, and thresholding. Building eyes-two is not necessary for using the 
Tesseract or Leptonica APIs.

The `tess-two-test` subdirectory contains Android JUnit tests.

## Pre-requisites

* Android 2.2 or higher
* A v3.02 [trained data file][tessdata] for a language. Data files must be 
extracted to the Android device in a subdirectory named `tessdata`.

## Versions

Release points are tagged with [version numbers][semantic-versioning]. A change 
to the major version number indicates an API change making that version 
incompatible with previous versions.

## Building

This project is set up to build on Android SDK Tools r22.3+ and Android NDK 
r10d+. The build works on Linux, Mac OS X, and Windows 7/8. See [Issues][issues]
for reported build issues.

On 64-bit Ubuntu, you may need to install the `ia32-libs` 32-bit compatibility 
library.

To build the latest tess-two code, run the following commands in the terminal:

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

After building, the tess-two and eyes-two projects can be imported into Eclipse 
using File->Import->Existing Projects into Workspace.

## Gradle build

This project can also be built using Gradle, tested and installed into a (local) Maven repository.

### Prerequisites 

Building the project through Gradle required the NDK to be installed through the Android SDK manager. The build system will look for the NDK in $ANDROID_HOME/ndk-bundle/ 

A file local.properties needs to be created with the location of your android SDK, the contents of this file should be:

    sdk.dir=<path to Android SDK>

### Building

The project can be built using the following command: 

    ./gradlew build

This produces a release and debug  AAR file in tess-two/build/outputs/aar/

### Installing in Maven

The group and artifact ID's are:

Release version:
groupId: com.googlecode.tesseract.android
artifactId: tess-two
version: 5.2.0-SNAPSHOT
packaging: aar

Debug version
groupId: com.googlecode.tesseract.android
artifactId: tess-two-debug
version: 5.2.0-SNAPSHOT
packaging: aar


Both the debug and release builds can be installed as artifacts in a Maven repository of your choosing.

To install the debug build into your local repository run the following command:

    ./gradlew publishDebugPublicationToMavenLocal 

To install the release build into your local repository run the following command:

    ./gradlew publishReleasePublicationToMavenLocal

To install the build into a remote repository the following configuration items need to be added to your ~/.gradle/gradle.properties file

    mavenSnapshots=<url>
    mavenSnapshotsUsername=<username>
    mavenSnapshotsPassword=<password>
    mavenReleases=<url>
    mavenReleasesUsername=<username>
    mavenReleasesPassword=<password>

To install into a maven repository, run one of the following commands:

Debug/Release | Snapshots/Releases | Command
------------------------------------------------------
Debug         | Snapshot           | ./gradlew publishDebugPublicationToTess-two-snapshotsRepository
Debug         | Releases           | ./gradlew publishDebugPublicationToTess-two-releasesRepository
Release       | Snapshot           | ./gradlew publishReleasePublicationToTess-two-snapshotsRepository
Release       | Releases           | ./gradlew publishReleasePublicationToTess-two-releasesRepository

## Depending on tess-two from Gradle

To use tess-two as a dependency in your Android Gradle project make sure the following lines are in your gradle build files:

app/build.gradle:
    dependencies { 
        compile 'com.googlecode.tesseract.android:tess-two:5.2.0-SNAPSHOT'  
        // other dependencies
    }

build.gradle:
    allprojects {
        repositories {
            mavenLocal()
            // other repo's
        }
    }



## Maven

While this project does not require Maven (and this project has not yet been 
registered in a Maven central repository), it can be 
[integrated into a local Maven repository for convenience][maven].

## ProGuard
If you're using ProGuard for code shrinking and obfuscation, add the following
to your app's ProGuard config to retain fields used for sharing data with native
code:
```proguard
# tess-two
-keep class com.googlecode.leptonica.android.Box {
    private long mNativeBox;
}
-keep class com.googlecode.leptonica.android.Boxa {
    private long mNativeBoxa;
}
-keep class com.googlecode.leptonica.android.Pix {
    private long mNativePix;
}
-keep class com.googlecode.leptonica.android.Pixa {
    private long mNativePixa;
}
-keep class com.googlecode.tesseract.android.TessBaseAPI {
    private long mNativeData;
}
-keep class com.googlecode.tesseract.android.PageIterator {
    private long mNativePageIterator;
}
-keep class com.googlecode.tesseract.android.ResultIterator {
    private long mNativeResultIterator;
}
```

```proguard
# eyes-two
-keep class com.googlecode.eyesfree.textdetect.HydrogenTextDetector {
    private long mNative;
}
```

## Support

* Stack Overflow: http://stackoverflow.com/questions/tagged/tess-two
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
[library-project]: https://developer.android.com/tools/projects/projects-eclipse.html#ReferencingLibraryProject
[eyes-free]: https://code.google.com/p/eyes-free/
[tessdata]: https://github.com/tesseract-ocr/tessdata
[semantic-versioning]: http://semver.org
[issues]: https://github.com/rmtheis/tess-two/issues
[maven]: http://www.jameselsey.co.uk/blogs/techblog/tesseract-ocr-on-android-is-easier-if-you-maven-ise-it-works-on-windows-too/
[stackoverflow]: https://stackoverflow.com/
