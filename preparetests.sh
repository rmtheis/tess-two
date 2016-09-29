#!/bin/sh

git clone https://github.com/tesseract-ocr/tessdata.git

adb shell rm /sdcard/testAddPageToDocument.pdf
adb shell rm /sdcard/testCreate.pdf

adb uninstall com.googlecode.tesseract.android.test

adb shell mkdir /sdcard/tesseract
adb shell mkdir /sdcard/tesseract/tessdata
 
for f in tessdata/eng.*; do
    TRAINEDDATA_FILENAME="/sdcard/tesseract/tessdata/`basename $f`"

    if [ `adb shell "if [ -f $TRAINEDDATA_FILENAME ]; then echo 1; fi"` ]; then
        echo "$TRAINEDDATA_FILENAME already present on device"
        continue
    fi 
    
    echo "Copying $f to device...";
    adb push -p $f /sdcard/tesseract/tessdata
done

adb push -p tess-two/jni/com_googlecode_tesseract_android/src/tessdata/pdf.ttf /sdcard/tesseract/tessdata

# adb shell am instrument -w com.googlecode.tesseract.android.test/android.test.InstrumentationTestRunner
