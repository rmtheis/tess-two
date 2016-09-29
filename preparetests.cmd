git clone https://github.com/tesseract-ocr/tessdata.git

adb shell rm /mnt/sdcard/testAddPageToDocument.pdf
adb shell rm /mnt/sdcard/testCreate.pdf

adb uninstall com.googlecode.tesseract.android.test

adb shell mkdir /mnt/sdcard/tesseract
adb shell mkdir /mnt/sdcard/tesseract/tessdata

adb push -p tessdata/eng.cube.bigrams /mnt/sdcard/tesseract/tessdata
adb push -p tessdata/eng.cube.fold /mnt/sdcard/tesseract/tessdata
adb push -p tessdata/eng.cube.lm /mnt/sdcard/tesseract/tessdata
adb push -p tessdata/eng.cube.nn /mnt/sdcard/tesseract/tessdata
adb push -p tessdata/eng.cube.params /mnt/sdcard/tesseract/tessdata
adb push -p tessdata/eng.cube.size /mnt/sdcard/tesseract/tessdata
adb push -p tessdata/eng.cube.word-freq /mnt/sdcard/tesseract/tessdata
adb push -p tessdata/eng.tesseract_cube.nn /mnt/sdcard/tesseract/tessdata
adb push -p tessdata/eng.traineddata /mnt/sdcard/tesseract/tessdata
adb push -p tess-two/jni/com_googlecode_tesseract_android/src/tessdata/pdf.ttf /mnt/sdcard/tesseract/tessdata
