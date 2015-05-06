LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libtess

# tesseract (minus executable)

BLACKLIST_SRC_FILES := \
  %api/tesseractmain.cpp \
  %ccstruct/imagedata.cpp \
  %viewer/svpaint.cpp

TESSERACT_SRC_FILES := \
  $(wildcard $(TESSERACT_PATH)/api/*.cpp) \
  $(wildcard $(TESSERACT_PATH)/ccmain/*.cpp) \
  $(wildcard $(TESSERACT_PATH)/ccstruct/*.cpp) \
  $(wildcard $(TESSERACT_PATH)/ccutil/*.cpp) \
  $(wildcard $(TESSERACT_PATH)/classify/*.cpp) \
  $(wildcard $(TESSERACT_PATH)/cube/*.cpp) \
  $(wildcard $(TESSERACT_PATH)/cutil/*.cpp) \
  $(wildcard $(TESSERACT_PATH)/dict/*.cpp) \
  $(wildcard $(TESSERACT_PATH)/opencl/*.cpp) \
  $(wildcard $(TESSERACT_PATH)/neural_networks/runtime/*.cpp) \
  $(wildcard $(TESSERACT_PATH)/textord/*.cpp) \
  $(wildcard $(TESSERACT_PATH)/viewer/*.cpp) \
  $(wildcard $(TESSERACT_PATH)/wordrec/*.cpp)

LOCAL_SRC_FILES := \
  $(filter-out $(BLACKLIST_SRC_FILES),$(subst $(LOCAL_PATH)/,,$(TESSERACT_SRC_FILES)))

LOCAL_C_INCLUDES := \
  $(TESSERACT_PATH)/api \
  $(TESSERACT_PATH)/ccmain \
  $(TESSERACT_PATH)/ccstruct \
  $(TESSERACT_PATH)/ccutil \
  $(TESSERACT_PATH)/classify \
  $(TESSERACT_PATH)/cube \
  $(TESSERACT_PATH)/cutil \
  $(TESSERACT_PATH)/dict \
  $(TESSERACT_PATH)/opencl \
  $(TESSERACT_PATH)/neural_networks/runtime \
  $(TESSERACT_PATH)/textord \
  $(TESSERACT_PATH)/viewer \
  $(TESSERACT_PATH)/wordrec \
  $(LEPTONICA_PATH)/src

LOCAL_CFLAGS := \
  -DHAVE_LIBLEPT \
  -DGRAPHICS_DISABLED \
  --std=c++11 \
  -DUSE_STD_NAMESPACE \
  -D'VERSION="Android"' \
  -include ctype.h \
  -include unistd.h \
  -fpermissive \
  -Wno-deprecated \
  -D_GLIBCXX_PERMIT_BACKWARD_HASH   # fix for android-ndk-r8e/sources/cxx-stl/gnu-libstdc++/4.6/include/ext/hash_map:61:30: fatal error: backward_warning.h: No such file or directory

# jni

LOCAL_SRC_FILES += \
  pageiterator.cpp \
  resultiterator.cpp \
  tessbaseapi.cpp

LOCAL_C_INCLUDES += \
  $(LOCAL_PATH)

LOCAL_LDLIBS += \
  -ljnigraphics \
  -llog

# common

LOCAL_PRELINK_MODULE := false
LOCAL_SHARED_LIBRARIES := liblept
LOCAL_DISABLE_FORMAT_STRING_CHECKS := true

include $(BUILD_SHARED_LIBRARY)
