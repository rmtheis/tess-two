LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := time_log.cpp

LOCAL_CFLAGS := -Wall \
                -DHAVE_MALLOC_H \
                -DHAVE_PTHREAD \
                -finline-functions \
                -frename-registers \
                -ffast-math \
                -s \
                -fomit-frame-pointer

ifeq ($(LOG_TIME),true)
  LOCAL_CFLAGS += -DLOG_TIME
endif

LOCAL_MODULE := common

include $(BUILD_STATIC_LIBRARY)
