#!/bin/sh

adb shell rm /mnt/sdcard/testAddPageToDocument.pdf
adb shell rm /mnt/sdcard/testCreate.pdf

adb uninstall com.googlecode.tesseract.android.test

if [ ! -f tesseract-ocr-3.02.eng.tar.gz ]; then
    wget https://tesseract-ocr.googlecode.com/files/tesseract-ocr-3.02.eng.tar.gz
fi
tar xvfz tesseract-ocr-3.02.eng.tar.gz
adb shell mkdir /mnt/sdcard/tesseract
adb shell mkdir /mnt/sdcard/tesseract/tessdata
 
for f in tesseract-ocr/tessdata/eng.*; do
    TRAINEDDATA_FILENAME="/mnt/sdcard/tesseract/tessdata/`basename $f`"

    if [ `adb shell "if [ -f $TRAINEDDATA_FILENAME ]; then echo 1; fi"` ]; then
        echo "$TRAINEDDATA_FILENAME already present on device"
        continue
    fi 
    
    echo "Copying $f to device...";
    adb push -p $f /mnt/sdcard/tesseract/tessdata
done

adb push -p tess-two/jni/com_googlecode_tesseract_android/src/tessdata/pdf.ttf /mnt/sdcard/tesseract/tessdata

# adb shell am instrument -w com.googlecode.tesseract.android.test/android.test.InstrumentationTestRunner
