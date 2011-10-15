/*
 * Copyright 2010, Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

jint Java_com_googlecode_leptonica_android_Pix_nativeCreatePix(JNIEnv *env, jclass clazz,
                                                                jint w, jint h, jint d) {
  LOGV(__FUNCTION__);

  PIX *pix = pixCreate((l_int32) w, (l_int32) h, (l_int32) d);

  return (jint) pix;
}

jint Java_com_googlecode_leptonica_android_Pix_nativeCreateFromData(JNIEnv *env, jclass clazz,
                                                                     jbyteArray data, jint w,
                                                                     jint h, jint d) {
  LOGV(__FUNCTION__);

  PIX *pix = pixCreateNoInit((l_int32) w, (l_int32) h, (l_int32) d);

  jbyte *data_buffer = env->GetByteArrayElements(data, NULL);
  l_uint8 *byte_buffer = (l_uint8 *) data_buffer;

  size_t size = 4 * pixGetWpl(pix) * pixGetHeight(pix);
  memcpy(pixGetData(pix), byte_buffer, size);

  env->ReleaseByteArrayElements(data, data_buffer, JNI_ABORT);

  return (jint) pix;
}

jboolean Java_com_googlecode_leptonica_android_Pix_nativeGetData(JNIEnv *env, jclass clazz,
                                                                  jint nativePix, jbyteArray data) {
  LOGV(__FUNCTION__);

  PIX *pix = (PIX *) nativePix;

  jbyte *data_buffer = env->GetByteArrayElements(data, NULL);
  l_uint8 *byte_buffer = (l_uint8 *) data_buffer;

  size_t size = 4 * pixGetWpl(pix) * pixGetHeight(pix);
  memcpy(byte_buffer, pixGetData(pix), size);

  env->ReleaseByteArrayElements(data, data_buffer, 0);

  return JNI_TRUE;
}

jint Java_com_googlecode_leptonica_android_Pix_nativeGetDataSize(JNIEnv *env, jclass clazz,
                                                                  jint nativePix) {
  LOGV(__FUNCTION__);

  PIX *pix = (PIX *) nativePix;

  size_t size = 4 * pixGetWpl(pix) * pixGetHeight(pix);

  return (jint) size;
}

jint Java_com_googlecode_leptonica_android_Pix_nativeClone(JNIEnv *env, jclass clazz,
                                                           jint nativePix) {
  LOGV(__FUNCTION__);

  PIX *pixs = (PIX *) nativePix;
  PIX *pixd = pixClone(pixs);

  return (jint) pixd;
}

jint Java_com_googlecode_leptonica_android_Pix_nativeCopy(JNIEnv *env, jclass clazz,
                                                           jint nativePix) {
  LOGV(__FUNCTION__);

  PIX *pixs = (PIX *) nativePix;
  PIX *pixd = pixCopy(NULL, pixs);

  return (jint) pixd;
}

jboolean Java_com_googlecode_leptonica_android_Pix_nativeInvert(JNIEnv *env, jclass clazz,
                                                                 jint nativePix) {
  LOGV(__FUNCTION__);

  PIX *pixs = (PIX *) nativePix;

  if (pixInvert(pixs, pixs)) {
    return JNI_FALSE;
  }

  return JNI_TRUE;
}

void Java_com_googlecode_leptonica_android_Pix_nativeDestroy(JNIEnv *env, jclass clazz,
                                                              jint nativePix) {
  LOGV(__FUNCTION__);

  PIX *pix = (PIX *) nativePix;

  pixDestroy(&pix);
}

jboolean Java_com_googlecode_leptonica_android_Pix_nativeGetDimensions(JNIEnv *env, jclass clazz,
                                                                        jint nativePix,
                                                                        jintArray dimensions) {
  LOGV(__FUNCTION__);

  PIX *pix = (PIX *) nativePix;
  jint *dimensionArray = env->GetIntArrayElements(dimensions, NULL);
  l_int32 w, h, d;

  if (pixGetDimensions(pix, &w, &h, &d)) {
    return JNI_FALSE;
  }

  dimensionArray[0] = w;
  dimensionArray[1] = h;
  dimensionArray[2] = d;

  env->ReleaseIntArrayElements(dimensions, dimensionArray, 0);

  return JNI_TRUE;
}

jint Java_com_googlecode_leptonica_android_Pix_nativeGetWidth(JNIEnv *env, jclass clazz,
                                                               jint nativePix) {
  PIX *pix = (PIX *) nativePix;

  return (jint) pixGetWidth(pix);
}

jint Java_com_googlecode_leptonica_android_Pix_nativeGetHeight(JNIEnv *env, jclass clazz,
                                                                jint nativePix) {
  PIX *pix = (PIX *) nativePix;

  return (jint) pixGetHeight(pix);
}

jint Java_com_googlecode_leptonica_android_Pix_nativeGetDepth(JNIEnv *env, jclass clazz,
                                                               jint nativePix) {
  PIX *pix = (PIX *) nativePix;

  return (jint) pixGetDepth(pix);
}

void Java_com_googlecode_leptonica_android_Pix_nativeSetPixel(JNIEnv *env, jclass clazz,
                                                               jint nativePix, jint xCoord,
                                                               jint yCoord, jint argbColor) {
  PIX *pix = (PIX *) nativePix;
  l_int32 d = pixGetDepth(pix);
  l_int32 x = (l_int32) xCoord;
  l_int32 y = (l_int32) yCoord;

  // These shift values are based on RGBA_8888
  l_uint8 r = (argbColor >> SK_R32_SHIFT) & 0xFF;
  l_uint8 g = (argbColor >> SK_G32_SHIFT) & 0xFF;
  l_uint8 b = (argbColor >> SK_B32_SHIFT) & 0xFF;
  l_uint8 a = (argbColor >> SK_A32_SHIFT) & 0xFF;
  l_uint8 gray = ((r + g + b) / 3) & 0xFF;

  l_uint32 color;

  switch (d) {
    case 1: // 1-bit binary
      color = gray > 128 ? 1 : 0;
      break;
    case 2: // 2-bit grayscale
      color = gray >> 6;
      break;
    case 4: // 4-bit grayscale
      color = gray >> 4;
      break;
    case 8: // 8-bit grayscale
      color = gray;
      break;
    case 24: // 24-bit RGB
      SET_DATA_BYTE(&color, COLOR_RED, r);
      SET_DATA_BYTE(&color, COLOR_GREEN, g);
      SET_DATA_BYTE(&color, COLOR_BLUE, b);
      break;
    case 32: // 32-bit ARGB
      SET_DATA_BYTE(&color, COLOR_RED, r);
      SET_DATA_BYTE(&color, COLOR_GREEN, g);
      SET_DATA_BYTE(&color, COLOR_BLUE, b);
      SET_DATA_BYTE(&color, L_ALPHA_CHANNEL, a);
      break;
    default: // unsupported
      LOGE("Not a supported color depth: %d", d);
      color = 0;
      break;
  }

  pixSetPixel(pix, x, y, color);
}

jint Java_com_googlecode_leptonica_android_Pix_nativeGetPixel(JNIEnv *env, jclass clazz,
                                                               jint nativePix, jint xCoord,
                                                               jint yCoord) {
  PIX *pix = (PIX *) nativePix;
  l_int32 d = pixGetDepth(pix);
  l_int32 x = (l_int32) xCoord;
  l_int32 y = (l_int32) yCoord;
  l_uint32 pixel;
  l_uint32 color;
  l_uint8 a, r, g, b;

  pixGetPixel(pix, x, y, &pixel);

  switch (d) {
    case 1: // 1-bit binary
      a = 0xFF;
      r = g = b = (pixel == 0 ? 0x00 : 0xFF);
      break;
    case 2: // 2-bit grayscale
      a = 0xFF;
      r = g = b = (pixel << 6 | pixel << 4 | pixel);
      break;
    case 4: // 4-bit grayscale
      a = 0xFF;
      r = g = b = (pixel << 4 | pixel);
      break;
    case 8: // 8-bit grayscale
      a = 0xFF;
      r = g = b = pixel;
      break;
    case 24: // 24-bit RGB
      a = 0xFF;
      r = (pixel >> L_RED_SHIFT) & 0xFF;
      g = (pixel >> L_GREEN_SHIFT) & 0xFF;
      b = (pixel >> L_BLUE_SHIFT) & 0xFF;
      break;
    case 32: // 32-bit RGBA
      r = (pixel >> L_RED_SHIFT) & 0xFF;
      g = (pixel >> L_GREEN_SHIFT) & 0xFF;
      b = (pixel >> L_BLUE_SHIFT) & 0xFF;
      a = (pixel >> L_ALPHA_SHIFT) & 0xFF;
      break;
    default: // Not supported
      LOGE("Not a supported color depth: %d", d);
      a = r = g = b = 0x00;
      break;
  }

  color = a << SK_A32_SHIFT;
  color |= r << SK_R32_SHIFT;
  color |= g << SK_G32_SHIFT;
  color |= b << SK_B32_SHIFT;

  return (jint) color;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */
