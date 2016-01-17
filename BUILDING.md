# Building

This project may be built manually as an alternative to including the pre-built
AAR as an external dependency in your app project. To start the build, import
the root directory of this project into Android Studio as an existing Android
Studio project, or follow the instructions below to build on the command line.

##_Android Studio and Gradle_

The Gradle build uses the gradle-stable plugin and the "bundled" Android NDK to
build the Tesseract and Leptonica native C/C++ code through a call to
`ndk-build` in `build.gradle`. After building, the AAR file that is generated
may be [imported][aar-import] into your app project as a dependency on a local
binary package.

Type the following commands in a terminal window to build the project from the 
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

### Testing

_On Mac/Linux:_

    ./preparetests.sh
    ./gradlew connectedAndroidTest

_On Windows:_

    preparetests.cmd
    gradlew connectedAndroidTest

Some tests will fail due to existing issues.

## _Eclipse and Ant_

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

[aar-import]:http://stackoverflow.com/a/28816265/667810
