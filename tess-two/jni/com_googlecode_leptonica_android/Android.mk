LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := liblept

# leptonica (minus freetype)

BLACKLIST_SRC_FILES := \
  %endiantest.c \
  %freetype.c \
  %xtractprotos.c

LEPTONICA_SRC_FILES := \
  $(subst $(LOCAL_PATH)/,,$(wildcard $(LEPTONICA_PATH)/src/*.c))

LOCAL_SRC_FILES := \
  $(filter-out $(BLACKLIST_SRC_FILES),$(LEPTONICA_SRC_FILES))

LOCAL_CFLAGS := \
  -DHAVE_CONFIG_H

LOCAL_LDLIBS := \
  -lz

# jni

LOCAL_SRC_FILES += \
  box.cpp \
  boxa.cpp \
  pix.cpp \
  pixa.cpp \
  utilities.cpp \
  readfile.cpp \
  writefile.cpp \
  jni.cpp
  
LOCAL_C_INCLUDES += \
  $(LOCAL_PATH) \
  $(LEPTONICA_PATH)/src

LOCAL_LDLIBS += \
  -ljnigraphics \
  -llog

# common

LOCAL_PRELINK_MODULE:= false

include $(BUILD_SHARED_LIBRARY)
