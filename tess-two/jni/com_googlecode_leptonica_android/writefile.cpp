/*
 * Copyright 2011, Google Inc.
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
#include <android/bitmap.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*************
 * WriteFile *
 *************/

jint Java_com_googlecode_leptonica_android_WriteFile_nativeWriteBytes8(JNIEnv *env, jclass clazz,
                                                                       jint nativePix,
                                                                       jbyteArray data) {
  l_int32 w, h, d;
  PIX *pix = (PIX *) nativePix;
  pixGetDimensions(pix, &w, &h, &d);

  l_uint8 **lineptrs = pixSetupByteProcessing(pix, NULL, NULL);
  jbyte *data_buffer = env->GetByteArrayElements(data, NULL);
  l_uint8 *byte_buffer = (l_uint8 *) data_buffer;

  for (int i = 0; i < h; i++) {
    memcpy((byte_buffer + (i * w)), lineptrs[i], w);
  }

  env->ReleaseByteArrayElements(data, data_buffer, 0);
  pixCleanupByteProcessing(pix, lineptrs);

  return (jint)(w * h);
}

jboolean Java_com_googlecode_leptonica_android_WriteFile_nativeWriteFiles(JNIEnv *env,
                                                                          jclass clazz,
                                                                          jint nativePixa,
                                                                          jstring rootName,
                                                                          jint format) {
  PIXA *pixas = (PIXA *) nativePixa;

  const char *c_rootName = env->GetStringUTFChars(rootName, NULL);
  if (c_rootName == NULL) {
    LOGE("could not extract rootName string!");
    return JNI_FALSE;
  }

  jboolean result = JNI_TRUE;

  if (pixaWriteFiles(c_rootName, pixas, (l_uint32) format)) {
    LOGE("could not write pixa data to %s", c_rootName);
    result = JNI_FALSE;
  }

  env->ReleaseStringUTFChars(rootName, c_rootName);

  return result;
}

jbyteArray Java_com_googlecode_leptonica_android_WriteFile_nativeWriteMem(JNIEnv *env,
                                                                          jclass clazz,
                                                                          jint nativePix,
                                                                          jint format) {
  PIX *pixs = (PIX *) nativePix;

  l_uint8 *data;
  size_t size;

  if (pixWriteMem(&data, &size, pixs, (l_uint32) format)) {
    LOGE("Failed to write pix data");
    return NULL;
  }

  // TODO Can we just use the byte array directly?
  jbyteArray array = env->NewByteArray(size);
  env->SetByteArrayRegion(array, 0, size, (jbyte *) data);

  free(data);

  return array;
}

jboolean Java_com_googlecode_leptonica_android_WriteFile_nativeWriteImpliedFormat(
                                                                                  JNIEnv *env,
                                                                                  jclass clazz,
                                                                                  jint nativePix,
                                                                                  jstring fileName,
                                                                                  jint quality,
                                                                                  jboolean progressive) {
  PIX *pixs = (PIX *) nativePix;

  const char *c_fileName = env->GetStringUTFChars(fileName, NULL);
  if (c_fileName == NULL) {
    LOGE("could not extract fileName string!");
    return JNI_FALSE;
  }

  jboolean result = JNI_TRUE;

  if (pixWriteImpliedFormat(c_fileName, pixs, (l_int32) quality, (progressive == JNI_TRUE))) {
    LOGE("could not write pix data to %s", c_fileName);
    result = JNI_FALSE;
  }

  env->ReleaseStringUTFChars(fileName, c_fileName);

  return result;
}

jboolean Java_com_googlecode_leptonica_android_WriteFile_nativeWriteBitmap(JNIEnv *env,
                                                                           jclass clazz,
                                                                           jint nativePix,
                                                                           jobject bitmap) {
  PIX *pixs = (PIX *) nativePix;

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

  pixGetDimensions(pixs, &w, &h, &d);

  if (w != info.width || h != info.height) {
    LOGE("Bitmap width and height do not match Pix dimensions!");
    return JNI_FALSE;
  }

  if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
    LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
    return JNI_FALSE;
  }

  pixEndianByteSwap(pixs);

  l_uint8 *dst = (l_uint8 *) pixels;
  l_uint8 *src = (l_uint8 *) pixGetData(pixs);
  l_int32 dstBpl = info.stride;
  l_int32 srcBpl = 4 * pixGetWpl(pixs);

  LOGE("Writing 32bpp RGBA bitmap (w=%d, h=%d, stride=%d) from %dbpp Pix (wpl=%d)", info.width,
       info.height, info.stride, d, pixGetWpl(pixs));

  for (int dy = 0; dy < info.height; dy++) {
    l_uint8 *dstx = dst;
    l_uint8 *srcx = src;

    if (d == 32) {
      memcpy(dst, src, 4 * info.width);
    } else if (d == 8) {
      for (int dw = 0; dw < info.width; dw++) {
        dstx[0] = dstx[1] = dstx[2] = srcx[0];
        dstx[3] = 0xFF;

        dstx += 4;
        srcx += 1;
      }
    } else if (d == 1) {
      for (int dw = 0; dw < info.width; dw++) {
        dstx[0] = dstx[1] = dstx[2] = (1 << (7 - (dw & 7)) & srcx[0]) ? 0x00 : 0xFF;
        dstx[3] = 0xFF;

        dstx += 4;
        srcx += ((dw % 8) == 7) ? 1 : 0;
      }
    }

    dst += dstBpl;
    src += srcBpl;
  }

  AndroidBitmap_unlockPixels(env, bitmap);

  return JNI_TRUE;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */
