LOCAL_PATH := $(call my-dir)
TESSERACT_PATH := $(LOCAL_PATH)/com_googlecode_tesseract_android/src
LEPTONICA_PATH := $(LOCAL_PATH)/com_googlecode_leptonica_android/src
LIBJPEG_PATH := $(LOCAL_PATH)/libjpeg
LIBPNG_PATH := $(LOCAL_PATH)/libpng

# Just build the Android.mk files in the subdirs
include $(call all-subdir-makefiles)
