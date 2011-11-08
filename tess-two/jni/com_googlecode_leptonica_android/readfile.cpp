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
#include "string.h"
#include "android/bitmap.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/************
 * ReadFile *
 ************/

jint Java_com_googlecode_leptonica_android_ReadFile_nativeReadMem(JNIEnv *env, jclass clazz,
                                                                  jbyteArray image, jint length) {
  LOGV(__FUNCTION__);

  jbyte *image_buffer = env->GetByteArrayElements(image, NULL);
  int buffer_length = env->GetArrayLength(image);

  PIX *pix = pixReadMem((const l_uint8 *) image_buffer, buffer_length);

  env->ReleaseByteArrayElements(image, image_buffer, JNI_ABORT);

  return (jint) pix;
}

jint Java_com_googlecode_leptonica_android_ReadFile_nativeReadBytes8(JNIEnv *env, jclass clazz,
                                                                     jbyteArray data, jint w,
                                                                     jint h) {
  LOGV(__FUNCTION__);

  PIX *pix = pixCreateNoInit((l_int32) w, (l_int32) h, 8);
  l_uint8 **lineptrs = pixSetupByteProcessing(pix, NULL, NULL);
  jbyte *data_buffer = env->GetByteArrayElements(data, NULL);
  l_uint8 *byte_buffer = (l_uint8 *) data_buffer;

  for (int i = 0; i < h; i++) {
    memcpy(lineptrs[i], (byte_buffer + (i * w)), w);
  }

  env->ReleaseByteArrayElements(data, data_buffer, JNI_ABORT);
  pixCleanupByteProcessing(pix, lineptrs);

  l_int32 d;

  pixGetDimensions(pix, &w, &h, &d);

  LOGE("Created image width w=%d, h=%d, d=%d", w, h, d);

  return (jint) pix;
}

jboolean Java_com_googlecode_leptonica_android_ReadFile_nativeReplaceBytes8(JNIEnv *env,
                                                                            jclass clazz,
                                                                            jint nativePix,
                                                                            jbyteArray data,
                                                                            jint srcw, jint srch) {
  LOGV(__FUNCTION__);

  PIX *pix = (PIX *) nativePix;
  l_int32 w, h, d;

  pixGetDimensions(pix, &w, &h, &d);

  if (d != 8 || (l_int32) srcw != w || (l_int32) srch != h) {
    LOGE("Failed to replace bytes at w=%d, h=%d, d=%d with w=%d, h=%d", w, h, d, srcw, srch);

    return JNI_FALSE;
  }

  l_uint8 **lineptrs = pixSetupByteProcessing(pix, NULL, NULL);
  jbyte *data_buffer = env->GetByteArrayElements(data, NULL);
  l_uint8 *byte_buffer = (l_uint8 *) data_buffer;

  for (int i = 0; i < h; i++) {
    memcpy(lineptrs[i], (byte_buffer + (i * w)), w);
  }

  env->ReleaseByteArrayElements(data, data_buffer, JNI_ABORT);
  pixCleanupByteProcessing(pix, lineptrs);

  return JNI_TRUE;
}

jint Java_com_googlecode_leptonica_android_ReadFile_nativeReadFiles(JNIEnv *env, jclass clazz,
                                                                    jstring dirName, jstring prefix) {
  LOGV(__FUNCTION__);

  PIXA *pixad = NULL;

  const char *c_dirName = env->GetStringUTFChars(dirName, NULL);
  if (c_dirName == NULL) {
    LOGE("could not extract dirName string!");
    return NULL;
  }

  const char *c_prefix = env->GetStringUTFChars(prefix, NULL);
  if (c_prefix == NULL) {
    LOGE("could not extract prefix string!");
    return NULL;
  }

  pixad = pixaReadFiles(c_dirName, c_prefix);

  env->ReleaseStringUTFChars(dirName, c_dirName);
  env->ReleaseStringUTFChars(prefix, c_prefix);

  return (jint) pixad;
}

jint Java_com_googlecode_leptonica_android_ReadFile_nativeReadFile(JNIEnv *env, jclass clazz,
                                                                   jstring fileName) {
  LOGV(__FUNCTION__);

  PIX *pixd = NULL;

  const char *c_fileName = env->GetStringUTFChars(fileName, NULL);
  if (c_fileName == NULL) {
    LOGE("could not extract fileName string!");
    return NULL;
  }

  pixd = pixRead(c_fileName);

  env->ReleaseStringUTFChars(fileName, c_fileName);

  return (jint) pixd;
}

jint Java_com_googlecode_leptonica_android_ReadFile_nativeReadBitmap(JNIEnv *env,
    jclass clazz,
    jobject bitmap) {
  //LOGV(__FUNCTION__);

  l_int32 w, h, d;
  AndroidBitmapInfo info;
  void* pixels;
  int ret;

  if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
    LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
    return JNI_FALSE;
  }

  if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
    LOGE("Bitmap format is not RGBA_8888 !");
    return JNI_FALSE;
  }

  if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
    LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
    return JNI_FALSE;
  }

  PIX *pixd = pixCreate(info.width, info.height, 8);

  l_uint32 *src = (l_uint32 *) pixels;
  l_int32 srcWpl = (info.stride / 4);
  l_uint32 *dst = pixGetData(pixd);
  l_int32 dstWpl = pixGetWpl(pixd);
  l_uint8 a, r, g, b, pixel8;

  for (int y = 0; y < info.height; y++) {
    l_uint32 *dst_line = dst + (y * dstWpl);
    l_uint32 *src_line = src + (y * srcWpl);

    for (int x = 0; x < info.width; x++) {
      // Get pixel from RGBA_8888
      r = *src_line >> SK_R32_SHIFT;
      g = *src_line >> SK_G32_SHIFT;
      b = *src_line >> SK_B32_SHIFT;
      a = *src_line >> SK_A32_SHIFT;
      pixel8 = (l_uint8) ((r + g + b) / 3);

      // Set pixel to LUMA_8
      SET_DATA_BYTE(dst_line, x, pixel8);

      // Move to the next pixel
      src_line++;
    }
  }

  AndroidBitmap_unlockPixels(env, bitmap);

  return (jint) pixd;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */
