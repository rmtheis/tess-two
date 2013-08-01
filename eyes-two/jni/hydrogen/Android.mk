ifneq ($(TARGET_SIMULATOR),true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libhydrogen

LOCAL_SRC_FILES += \
  src/clusterer.cpp \
  src/hydrogentextdetector.cpp \
  src/thresholder.cpp \
  src/utilities.cpp \
  src/validator.cpp

LOCAL_SRC_FILES += \
  jni/hydrogentextdetector.cpp \
  jni/thresholder.cpp \
  jni/jni.cpp

LOCAL_C_INCLUDES += \
  $(LOCAL_PATH)/src \
  $(LOCAL_PATH)/include/leptonica

LOCAL_LDLIBS += \
  -llog

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

LOCAL_DISABLE_FORMAT_STRING_CHECKS := true

TARGET_PREBUILT_SHARED_LIBRARIES += \
  $(PREBUILT_PATH)/liblept.so

include $(BUILD_SHARED_LIBRARY)

endif #TARGET_SIMULATOR
