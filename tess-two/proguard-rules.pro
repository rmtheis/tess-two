#
# Adapted from example file:
# android-sdk/tools/proguard/examples/library.pro
#
# This ProGuard configuration file illustrates how to process a program
# library, such that it remains usable as a library.
#

# Don't obfuscate field and method names.

-dontobfuscate

# Preserve all annotations.

-keepattributes *Annotation*

# Preserve all public classes, and their public and protected fields and
# methods.

-keep public class * {
    public protected *;
}

# Preserve all .class method names.

-keepclassmembernames class * {
    java.lang.Class class$(java.lang.String);
    java.lang.Class class$(java.lang.String, boolean);
}

# Preserve all native method names and the names of their classes.

-keepclasseswithmembernames class * {
    native <methods>;
}

# Preserve the special static methods that are required in all enumeration
# classes.

-keepclassmembers class * extends java.lang.Enum {
    public static **[] values();
    public static ** valueOf(java.lang.String);
}

# Explicitly preserve all serialization members. The Serializable interface
# is only a marker interface, so it wouldn't save them.
# You can comment this out if your library doesn't use serialization.
# If your code contains serializable classes that have to be backward
# compatible, please refer to the manual.

-keepclassmembers class * implements java.io.Serializable {
    static final long serialVersionUID;
    static final java.io.ObjectStreamField[] serialPersistentFields;
    private void writeObject(java.io.ObjectOutputStream);
    private void readObject(java.io.ObjectInputStream);
    java.lang.Object writeReplace();
    java.lang.Object readResolve();
}

# Your library may contain more items that need to be preserved;
# typically classes that are dynamically created using Class.forName:

# -keep public class mypackage.MyClass
# -keep public interface mypackage.MyInterface
# -keep public class * implements mypackage.MyInterface

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