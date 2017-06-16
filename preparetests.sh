#!/bin/sh

DIR="`adb shell echo \\$EXTERNAL_STORAGE`"

git clone -b 3.04.00 https://github.com/tesseract-ocr/tessdata.git

adb shell rm $DIR/testAddPageToDocument.pdf
adb shell rm $DIR/testCreate.pdf

adb uninstall com.googlecode.tesseract.android.test

adb shell mkdir $DIR/tessdata
 
for f in tessdata/eng.*; do
    TRAINEDDATA_FILENAME="$DIR/tessdata/`basename $f`"

    if [ `adb shell "if [ -f $TRAINEDDATA_FILENAME ]; then echo 1; fi"` ]; then
        echo "$TRAINEDDATA_FILENAME already present on device"
        continue
    fi 
    
    echo "Copying $f to device...";
    adb push -p $f $DIR/tessdata
done

adb push -p tess-two/jni/com_googlecode_tesseract_android/src/tessdata/pdf.ttf $DIR/tessdata

# adb shell am instrument -w com.googlecode.tesseract.android.test/android.test.InstrumentationTestRunner
