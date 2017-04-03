# Building

This project may be built manually as an alternative to including the pre-built
AAR as an external dependency in your app project. To start the build, import
the root directory of this project into Android Studio as an existing Android
Studio project, or follow the instructions below to build on the command line.

Note: When building from Android Studio, you may need to set the path to your
NDK installation in the Project Structure dialog (File->Project Structure).

## _Android Studio and Gradle_

The Gradle build uses the gradle-stable plugin and the Android NDK to
build the Tesseract and Leptonica native C/C++ code through a call to
`ndk-build` in `build.gradle`. After building, the AAR file that is generated
may be [imported][aar-import] into your app project as a dependency on a local
binary package.

To build the project from the command line:

_On Mac/Linux:_

Edit your local.properties file to include the path to your NDK directory:

    ndk.dir=/path/to/your/android-ndk

Run the following commands:

    export ANDROID_HOME=/path/to/your/android-sdk
    git clone git://github.com/rmtheis/tess-two tess
    cd tess
    ./gradlew assemble
		
_On Windows:_

Edit your local.properties file to include the path to your NDK directory:

    ndk.dir=C\:\\path\\to\\your\\android-ndk

Run the following commands:

    set ANDROID_HOME=C:\\path\\to\\your\\android-sdk
    git clone git://github.com/rmtheis/tess-two tess
    cd tess
    gradlew assemble

### Testing

_On Mac/Linux:_

    ./preparetests.sh
    ./gradlew connectedAndroidTest

_On Windows:_

    preparetests.cmd
    gradlew connectedAndroidTest

Note that some tests will fail due to existing issues.

[aar-import]:http://stackoverflow.com/a/28816265/667810
