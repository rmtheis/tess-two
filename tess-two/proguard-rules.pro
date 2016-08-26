-dontobfuscate

-keep class com.googlecode.leptonica.android.Box {
    private long mNativeBox;
}
-keep class com.googlecode.leptonica.android.Boxa {
    private long mNativeBoxa;
}
-keep class com.googlecode.leptonica.android.Pix {
    private long mNativePix;
}
-keep class com.googlecode.leptonica.android.Pixa {
    private long mNativePixa;
}
-keep class com.googlecode.tesseract.android.TessBaseAPI {
    private long mNativeData;
    protected void onProgressValues(int, int, int, int, int, int, int, int, int);
}
-keep class com.googlecode.tesseract.android.PageIterator {
    private long mNativePageIterator;
}
-keep class com.googlecode.tesseract.android.TessPdfRenderer {
    private long mNativePdfRenderer;
}
-keep class com.googlecode.tesseract.android.ResultIterator {
    private long mNativeResultIterator;
}
