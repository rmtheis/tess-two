# Building

This project may be built manually as an alternative to including the pre-built
AAR as an external dependency in your app project. To start the build, import
the root directory of this project into Android Studio as an existing Android
Studio project, or follow the instructions below to build on the command line.

## _Building with Android Studio_

The Gradle build uses the gradle-stable plugin and the Android NDK to
build the Tesseract and Leptonica native C/C++ code through a call to
`ndk-build` in `build.gradle`. In Android Studio, use 

Build -> Rebuild Project

to build or rebuild the project.

Note: When building from Android Studio, you may need to set the path to your
NDK installation in the Project Structure dialog (File->Project Structure).

## _Building on the Command Line_

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

# Importing

After building, the code that is generated may be imported into your app
project in Android Studio as a module using

File -> New -> Import Module -> `tess-two` folder

and then adding the dependency to your app module build.gradle:

        dependencies {
            implementation project(':tess-two')
        }

# Testing

_On Mac/Linux:_

    ./preparetests.sh
    ./gradlew connectedAndroidTest

_On Windows:_

    preparetests.cmd
    gradlew connectedAndroidTest

# Removing

If you want to remove your app's dependency on the tess-two module, reverse
the import process by removing the module using the Project Structure dialog
(File->Project Structure), manually deleting the tess-two subfolder from your
app project folder, and removing the tess-two reference from your app module
build.gradle.