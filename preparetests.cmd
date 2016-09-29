git clone https://github.com/tesseract-ocr/tessdata.git

adb shell rm /sdcard/testAddPageToDocument.pdf
adb shell rm /sdcard/testCreate.pdf

adb uninstall com.googlecode.tesseract.android.test

adb shell mkdir /sdcard/tesseract
adb shell mkdir /sdcard/tesseract/tessdata

adb push -p tessdata/eng.cube.bigrams /sdcard/tesseract/tessdata
adb push -p tessdata/eng.cube.fold /sdcard/tesseract/tessdata
adb push -p tessdata/eng.cube.lm /sdcard/tesseract/tessdata
adb push -p tessdata/eng.cube.nn /sdcard/tesseract/tessdata
adb push -p tessdata/eng.cube.params /sdcard/tesseract/tessdata
adb push -p tessdata/eng.cube.size /sdcard/tesseract/tessdata
adb push -p tessdata/eng.cube.word-freq /sdcard/tesseract/tessdata
adb push -p tessdata/eng.tesseract_cube.nn /sdcard/tesseract/tessdata
adb push -p tessdata/eng.traineddata /sdcard/tesseract/tessdata
adb push -p tess-two/jni/com_googlecode_tesseract_android/src/tessdata/pdf.ttf /sdcard/tesseract/tessdata
