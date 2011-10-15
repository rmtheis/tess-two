REAL_LOCAL_PATH := $(call my-dir)
LOCAL_PATH :=

include $(CLEAR_VARS)

LOCAL_MODULE := liblept

# leptonica (minus freetype)

BLACKLIST_SRC_FILES := \
  %endiantest.c \
  %freetype.c \
  %xtractprotos.c

LOCAL_SRC_FILES := \
  $(filter-out $(BLACKLIST_SRC_FILES),$(wildcard $(LEPTONICA_PATH)/src/*.c))

LOCAL_CFLAGS := \
  -DHAVE_CONFIG_H

LOCAL_C_INCLUDES := \
  $(LIBJPEG_PATH)

LOCAL_LDLIBS := \
  -lz

# missing stdio functions

ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SRC_FILES += \
  $(REAL_LOCAL_PATH)/stdio/open_memstream.c \
  $(REAL_LOCAL_PATH)/stdio/fopencookie.c \
  $(REAL_LOCAL_PATH)/stdio/fmemopen.c
LOCAL_C_INCLUDES += \
  $(REAL_LOCAL_PATH)/stdio
endif

# jni

LOCAL_SRC_FILES += \
  $(REAL_LOCAL_PATH)/box.cpp \
  $(REAL_LOCAL_PATH)/pix.cpp \
  $(REAL_LOCAL_PATH)/pixa.cpp \
  $(REAL_LOCAL_PATH)/utilities.cpp \
  $(REAL_LOCAL_PATH)/readfile.cpp \
  $(REAL_LOCAL_PATH)/writefile.cpp \
  $(REAL_LOCAL_PATH)/jni.cpp
  
LOCAL_C_INCLUDES += \
  $(REAL_LOCAL_PATH) \
  $(LEPTONICA_PATH)/src

LOCAL_LDLIBS += \
  -ljnigraphics \
  -llog

# common

LOCAL_SHARED_LIBRARIES:= libjpeg
LOCAL_PRELINK_MODULE:= false

include $(BUILD_SHARED_LIBRARY)
