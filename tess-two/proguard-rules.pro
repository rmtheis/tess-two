# Preserve a method that is called from native code.

-keep class com.googlecode.tesseract.android.TessBaseAPI {
    protected void onProgressValues(int, int, int, int, int, int, int, int, int);
}
