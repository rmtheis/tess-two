for /f %%i in ('adb shell echo $EXTERNAL_STORAGE') do set DIR=%%i

git clone -b 3.04.00 https://github.com/tesseract-ocr/tessdata.git

adb shell rm %DIR%/testAddPageToDocument.pdf
adb shell rm %DIR%/testCreate.pdf

adb uninstall com.googlecode.tesseract.android.test

adb shell mkdir %DIR%/tessdata

adb push -p tessdata/eng.cube.bigrams %DIR%/tessdata
adb push -p tessdata/eng.cube.fold %DIR%/tessdata
adb push -p tessdata/eng.cube.lm %DIR%/tessdata
adb push -p tessdata/eng.cube.nn %DIR%/tessdata
adb push -p tessdata/eng.cube.params %DIR%/tessdata
adb push -p tessdata/eng.cube.size %DIR%/tessdata
adb push -p tessdata/eng.cube.word-freq %DIR%/tessdata
adb push -p tessdata/eng.tesseract_cube.nn %DIR%/tessdata
adb push -p tessdata/eng.traineddata %DIR%/tessdata
adb push -p tess-two/jni/com_googlecode_tesseract_android/src/tessdata/pdf.ttf %DIR%/tessdata
