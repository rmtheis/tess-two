LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := imageutils$(LIB_SUFFIX)
LOCAL_SRC_FILES := blur-jni.cpp \
		               similar-jni.cpp \
                   blur.cpp \
		               similar.cpp

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  LOCAL_CFLAGS += -DHAVE_ARMEABI_V7A=1 -mfloat-abi=softfp -mfpu=neon
  LOCAL_C_INCLUDES += $(NDK_ROOT)/sources/cpufeatures
  LOCAL_STATIC_LIBRARIES += cpufeatures
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../common

LOCAL_STATIC_LIBRARIES += common

include $(BUILD_SHARED_LIBRARY)
