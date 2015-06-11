#!/bin/sh
wget http://tesseract-ocr.googlecode.com/files/eng.traineddata.gz
gunzip eng.traineddata.gz
adb shell mkdir /mnt/sdcard/tesseract
adb shell mkdir /mnt/sdcard/tesseract/tessdata
adb push eng.traineddata /mnt/sdcard/tesseract/tessdata
adb shell am instrument -w com.googlecode.tesseract.android.test/android.test.InstrumentationTestRunner
