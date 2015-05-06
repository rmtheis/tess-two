// Copyright 2009 Google Inc. All Rights Reserved.
// Author: andrewharp@google.com (Andrew Harp)

#ifndef JAVA_COM_GOOGLE_ANDROID_APPS_UNVEIL_JNI_COMMON_UTILS_H_
#define JAVA_COM_GOOGLE_ANDROID_APPS_UNVEIL_JNI_COMMON_UTILS_H_

#include <android/log.h>
#include <stdlib.h>

#ifdef HAVE_ARMEABI_V7A
#include <cpu-features.h>
#include <arm_neon.h>
#endif

#include <math.h>
#include "types.h"

#define SAFE_DELETE(pointer) {\
  if ((pointer) != NULL) {\
    LOGV("Safe deleting pointer: %s", #pointer);\
    delete (pointer);\
    (pointer) = NULL;\
  } else {\
    LOGV("Pointer already null: %s", #pointer);\
  }\
}

#ifdef VERBOSE_LOGGING
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#else
#define LOGV(...) {}
#endif

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOG_TAG "goggles"

#ifdef SANITY_CHECKS
#define CHECK(condition, ...) {\
  if (!(condition)) {\
    LOGE("CHECK FAILED: (%s) @ %s:%u\n", #condition, __FILE__, __LINE__);\
    LOGE(__VA_ARGS__);\
  }\
}
#else
#define CHECK(...) {}
#endif


#ifdef HAVE_ARMEABI_V7A
// Runtime check for NEON support.  Only call on devices that support at least
// armeabi-v7a.
inline bool supportsNeon() {
  return (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0;
}
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) > (b)) ? (b) : (a))
#endif

template<typename T>
inline static T square(const T a) {
  return a * a;
}

template<typename T>
inline static T clip(const T a, const T floor, const T ceil) {
  return min(ceil, max(a, floor));
}

template<typename T>
inline static int32 floor(const T a) {
  return static_cast<int32>(a);
}

template<typename T>
inline static int32 ceil(const T a) {
  return floor(a) + 1;
}

template<typename T>
inline static bool inRange(const T a, const T min, const T max) {
  return (a >= min) && (a <= max);
}

template<typename T>
inline static int32 round(const float a) {
  return (a - (float) floor(a) > 0.5f) ? ceil(a) : floor(a);
}

template<typename T>
inline static void swap(T* const a, T* const b) {
  // Cache out the VALUE of what's at a.
  T tmp = *a;
  *a = *b;

  *b = tmp;
}

#endif // JAVA_COM_GOOGLE_ANDROID_APPS_UNVEIL_JNI_COMMON_UTILS_H_
