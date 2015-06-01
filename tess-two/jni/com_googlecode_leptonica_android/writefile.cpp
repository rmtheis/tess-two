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
                                                                       jlong nativePix,
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

jboolean Java_com_googlecode_leptonica_android_WriteFile_nativeWriteImpliedFormat(JNIEnv *env,
                                                                                  jclass clazz,
                                                                                  jlong nativePix,
                                                                                  jstring fileName) {
  PIX *pixs = (PIX *) nativePix;

  const char *c_fileName = env->GetStringUTFChars(fileName, NULL);
  if (c_fileName == NULL) {
    LOGE("could not extract fileName string!");
    return JNI_FALSE;
  }

  jboolean result = JNI_TRUE;

  if (pixWriteImpliedFormat(c_fileName, pixs, 0, JNI_FALSE)) {
    LOGE("could not write pix data to %s", c_fileName);
    result = JNI_FALSE;
  }

  env->ReleaseStringUTFChars(fileName, c_fileName);

  return result;
}

jboolean Java_com_googlecode_leptonica_android_WriteFile_nativeWriteBitmap(JNIEnv *env,
                                                                           jclass clazz,
                                                                           jlong nativePix,
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

  LOGI("Writing 32bpp RGBA bitmap (w=%d, h=%d, stride=%d) from %dbpp Pix (wpl=%d)", info.width,
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
