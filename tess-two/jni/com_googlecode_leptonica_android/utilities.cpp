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

/***************
 * AdaptiveMap *
 ***************/

jlong Java_com_googlecode_leptonica_android_AdaptiveMap_nativeBackgroundNormMorph(JNIEnv *env,
                                                                                  jclass clazz,
                                                                                  jlong nativePix,
                                                                                  jint reduction,
                                                                                  jint size,
                                                                                  jint bgval) {
  // Normalizes the background of each element in pixa.

  PIX *pixs = (PIX *) nativePix;
  PIX *pixd = pixBackgroundNormMorph(pixs, NULL, (l_int32) reduction, (l_int32) size,
                                     (l_int32) bgval);

  return (jlong) pixd;
}

jlong Java_com_googlecode_leptonica_android_AdaptiveMap_nativePixContrastNorm(JNIEnv *env,
                                                                              jclass clazz,
                                                                              jlong nativePix,
                                                                              jint sizeX,
                                                                              jint sizeY,
                                                                              jint minDiff,
                                                                              jint smoothX,
                                                                              jint smoothY) {

  PIX *pixs = (PIX *) nativePix;
  PIX *pixd = pixContrastNorm(NULL, pixs, (l_int32) sizeX, (l_int32) sizeY,
                                     (l_int32) minDiff, (l_int32) smoothX, (l_int32) smoothY);

  return (jlong) pixd;
}

/************
 * Binarize *
 ************/

jlong Java_com_googlecode_leptonica_android_Binarize_nativeOtsuAdaptiveThreshold(JNIEnv *env,
                                                                                 jclass clazz,
                                                                                 jlong nativePix,
                                                                                 jint sizeX,
                                                                                 jint sizeY,
                                                                                 jint smoothX,
                                                                                 jint smoothY,
                                                                                 jfloat scoreFract) {

  PIX *pixs = (PIX *) nativePix;
  PIX *pixd;

  if (pixOtsuAdaptiveThreshold(pixs, (l_int32) sizeX, (l_int32) sizeY, (l_int32) smoothX,
                               (l_int32) smoothY, (l_float32) scoreFract, NULL, &pixd)) {
    return (jlong) 0;
  }

  return (jlong) pixd;
}

jlong Java_com_googlecode_leptonica_android_Binarize_nativeSauvolaBinarizeTiled(JNIEnv *env,
                                                                                jclass clazz,
                                                                                jlong nativePix,
                                                                                jint whsize,
                                                                                jfloat factor,
                                                                                jint nx,
                                                                                jint ny) {

  PIX *pixs = (PIX *) nativePix;
  PIX *pixd;

  if (pixSauvolaBinarizeTiled(pixs, (l_int32) whsize, (l_float32) factor, (l_int32) nx,
                               (l_int32) ny, NULL, &pixd)) {
    return (jlong) 0;
  }

  return (jlong) pixd;
}

/********
 * Clip *
 ********/

jlong Java_com_googlecode_leptonica_android_Clip_nativeClipRectangle(JNIEnv *env, jclass clazz,
                                                                     jlong nativePix, jlong nativeBox) {

  PIX *pixs = (PIX *) nativePix;
  BOX *box = (BOX *) nativeBox;
  PIX *pixd;
  pixd = pixClipRectangle(pixs,box,NULL);
  return (jlong) pixd;
}

/***********
 * Convert *
 ***********/

jlong Java_com_googlecode_leptonica_android_Convert_nativeConvertTo8(JNIEnv *env, jclass clazz,
                                                                     jlong nativePix) {
  PIX *pixs = (PIX *) nativePix;
  PIX *pixd = pixConvertTo8(pixs, FALSE);

  return (jlong) pixd;
}

/********
 * Edge *
 ********/

jlong Java_com_googlecode_leptonica_android_Edge_nativePixSobelEdgeFilter(JNIEnv *env,
                                                                          jclass clazz,
                                                                          jlong nativePix,
                                                                          jint orientFlag) {
  PIX *pixs = (PIX *) nativePix;
  PIX *pixd = pixSobelEdgeFilter(pixs, (l_int32) orientFlag);

  return (jlong) pixd;
}

/***********
 * Enhance *
 ***********/

jlong Java_com_googlecode_leptonica_android_Enhance_nativeUnsharpMasking(JNIEnv *env, jclass clazz,
                                                                         jlong nativePix,
                                                                         jint halfwidth,
                                                                         jfloat fract) {
  PIX *pixs = (PIX *) nativePix;
  PIX *pixd = pixUnsharpMasking(pixs, (l_int32) halfwidth, (l_float32) fract);

  return (jlong) pixd;
}

/*************
 * GrayQuant *
 *************/

jlong Java_com_googlecode_leptonica_android_GrayQuant_nativePixThresholdToBinary(JNIEnv *env, jclass clazz,
                                                                                 jlong nativePix, jint thresh) {
  PIX *pixs = (PIX *) nativePix;
  PIX *pixd = pixThresholdToBinary(pixs, (l_int32) thresh);

  return (jlong) pixd;
}

/**********
 * JpegIO *
 **********/

jbyteArray Java_com_googlecode_leptonica_android_JpegIO_nativeCompressToJpeg(JNIEnv *env,
                                                                             jclass clazz,
                                                                             jlong nativePix,
                                                                             jint quality,
                                                                             jboolean progressive) {
  PIX *pix = (PIX *) nativePix;

  l_uint8 *data;
  size_t size;

  if (pixWriteMemJpeg(&data, &size, pix, (l_int32) quality, progressive == JNI_TRUE ? 1 : 0)) {
    LOGE("Failed to write JPEG data");

    return NULL;
  }

  // TODO Can we just use the byte array directly?
  jbyteArray array = env->NewByteArray(size);
  env->SetByteArrayRegion(array, 0, size, (jbyte *) data);

  free(data);

  return array;
}

/************
 * MorphApp *
 ************/

jlong Java_com_googlecode_leptonica_android_MorphApp_nativePixTophat(JNIEnv *env, jclass clazz,
                                                                     jlong nativePix, jint hsize,
                                                                     jint vsize, jint type) {
  PIX *pixs = (PIX *) nativePix;
  PIX *pixd = pixTophat(pixs, (l_int32) hsize, (l_int32) vsize, (l_int32) type);

  return (jlong) pixd;
}

jlong Java_com_googlecode_leptonica_android_MorphApp_nativePixFastTophat(JNIEnv *env, jclass clazz,
                                                                         jlong nativePix, jint xsize,
                                                                         jint ysize, jint type) {
  PIX *pixs = (PIX *) nativePix;
  PIX *pixd = pixFastTophat(pixs, (l_int32) xsize, (l_int32) ysize, (l_int32) type);

  return (jlong) pixd;
}

/*********
 * Scale *
 *********/

jlong Java_com_googlecode_leptonica_android_Scale_nativeScaleGeneral(JNIEnv *env, jclass clazz,
                                                                     jlong nativePix, jfloat scaleX,
                                                                     jfloat scaleY, jfloat sharpfract, jint sharpwidth) {
  PIX *pixs = (PIX *) nativePix;
  PIX *pixd = pixScaleGeneral(pixs, (l_float32) scaleX, (l_float32) scaleY,(l_float32) sharpfract, (l_int32) sharpwidth);
  return (jlong) pixd;
}

jlong Java_com_googlecode_leptonica_android_Scale_nativeScale(JNIEnv *env, jclass clazz,
                                                              jlong nativePix, jfloat scaleX,
                                                              jfloat scaleY) {
  PIX *pixs = (PIX *) nativePix;
  PIX *pixd = pixScale(pixs, (l_float32) scaleX, (l_float32) scaleY);

  return (jlong) pixd;
}

/********
 * Skew *
 ********/

jfloat Java_com_googlecode_leptonica_android_Skew_nativeFindSkew(JNIEnv *env, jclass clazz,
                                                                 jlong nativePix, jfloat sweepRange,
                                                                 jfloat sweepDelta,
                                                                 jint sweepReduction,
                                                                 jint searchReduction,
                                                                 jfloat searchMinDelta) {
  PIX *pixs = (PIX *) nativePix;

  l_float32 angle, conf;

  if (!pixFindSkewSweepAndSearch(pixs, &angle, &conf, (l_int32) sweepReduction,
                                 (l_int32) searchReduction, (l_float32) sweepRange,
                                 (l_int32) sweepDelta, (l_float32) searchMinDelta)) {
    if (conf <= 0) {
      return (jfloat) 0;
    }

    return (jfloat) angle;
  }

  return (jfloat) 0;
}

/**********
 * Rotate *
 **********/

jlong Java_com_googlecode_leptonica_android_Rotate_nativeRotate(JNIEnv *env, jclass clazz,
                                                                jlong nativePix, jfloat degrees,
                                                                jboolean quality, jboolean resize) {
  PIX *pixd;
  PIX *pixs = (PIX *) nativePix;

  l_float32 deg2rad = 3.1415926535 / 180.0;
  l_float32 radians = degrees * deg2rad;
  l_int32 w, h, bpp, type;

  pixGetDimensions(pixs, &w, &h, &bpp);

  if (bpp == 1 && quality == JNI_TRUE) {
    pixd = pixRotateBinaryNice(pixs, radians, L_BRING_IN_WHITE);
  } else {
    type = quality == JNI_TRUE ? L_ROTATE_AREA_MAP : L_ROTATE_SAMPLING;
    w = (resize == JNI_TRUE) ? w : 0;
    h = (resize == JNI_TRUE) ? h : 0;
    pixd = pixRotate(pixs, radians, type, L_BRING_IN_WHITE, w, h);
  }

  return (jlong) pixd;
}

jlong Java_com_googlecode_leptonica_android_Rotate_nativeRotateOrth(JNIEnv *env, jclass clazz,
                                                                    jlong nativePix, jint quads) {

  PIX *pixs = (PIX *) nativePix;
  PIX *pixd;
  pixd = pixRotateOrth(pixs,(int)quads);
  return (jlong) pixd;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */
