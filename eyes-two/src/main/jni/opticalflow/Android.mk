LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libopticalflow
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := -Wall \
                -DHAVE_MALLOC_H \
                -DHAVE_PTHREAD \
                -finline-functions \
                -frename-registers \
                -ffast-math \
                -s \
                -fomit-frame-pointer

LOCAL_SRC_FILES := optical_flow-jni.cpp \
                   optical_flow.cpp \
                   feature_detector.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../common

ifeq ($(LOG_TIME),true)
  LOCAL_CFLAGS += -DLOG_TIME
endif

ifeq ($(SANITY_CHECKS),true)
  LOCAL_CFLAGS += -DSANITY_CHECKS
endif

ifeq ($(VERBOSE_LOGGING),true)
  LOCAL_CFLAGS += -DVERBOSE_LOGGING
endif

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_CFLAGS += -DHAVE_ARMEABI_V7A=1 -mfloat-abi=softfp -mfpu=neon
    LOCAL_C_INCLUDES += $(NDK_ROOT)/sources/cpufeatures
    LOCAL_STATIC_LIBRARIES += cpufeatures
endif

LOCAL_LDLIBS := -llog

LOCAL_STATIC_LIBRARIES += common

include $(BUILD_SHARED_LIBRARY)


include $(NDK_ROOT)/sources/cpufeatures/Android.mk
