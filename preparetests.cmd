adb shell rm /mnt/sdcard/testAddPageToDocument.pdf
adb shell rm /mnt/sdcard/testCreate.pdf

adb uninstall com.googlecode.tesseract.android.test

powershell -Command "Invoke-WebRequest https://tesseract-ocr.googlecode.com/files/tesseract-ocr-3.02.eng.tar.gz -OutFile tesseract-ocr-3.02.eng.tar.gz"

"C:\Program Files\7-Zip\7z.exe" e tesseract-ocr-3.02.eng.tar.gz -o"."
"C:\Program Files\7-Zip\7z.exe" x tesseract-ocr-3.02.eng.tar -aoa
del tesseract-ocr-3.02.eng.tar

adb shell mkdir /mnt/sdcard/tesseract
adb shell mkdir /mnt/sdcard/tesseract/tessdata

adb push -p tesseract-ocr/tessdata/eng.cube.bigrams /mnt/sdcard/tesseract/tessdata
adb push -p tesseract-ocr/tessdata/eng.cube.fold /mnt/sdcard/tesseract/tessdata
adb push -p tesseract-ocr/tessdata/eng.cube.lm /mnt/sdcard/tesseract/tessdata
adb push -p tesseract-ocr/tessdata/eng.cube.nn /mnt/sdcard/tesseract/tessdata
adb push -p tesseract-ocr/tessdata/eng.cube.params /mnt/sdcard/tesseract/tessdata
adb push -p tesseract-ocr/tessdata/eng.cube.size /mnt/sdcard/tesseract/tessdata
adb push -p tesseract-ocr/tessdata/eng.cube.word-freq /mnt/sdcard/tesseract/tessdata
adb push -p tesseract-ocr/tessdata/eng.tesseract_cube.nn /mnt/sdcard/tesseract/tessdata
adb push -p tesseract-ocr/tessdata/eng.traineddata /mnt/sdcard/tesseract/tessdata
adb push -p tess-two/jni/com_googlecode_tesseract_android/src/tessdata/pdf.ttf /mnt/sdcard/tesseract/tessdata
