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

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

jlong Java_com_googlecode_leptonica_android_Pixa_nativeCreate(JNIEnv *env, jclass clazz, jint size) {
  PIXA *pixa = pixaCreate((l_int32) size);

  return (jlong) pixa;
}

jlong Java_com_googlecode_leptonica_android_Pixa_nativeCopy(JNIEnv *env, jclass clazz,
                                                            jlong nativePixa) {
  PIXA *pixas = (PIXA *) nativePixa;
  PIXA *pixad = pixaCopy(pixas, L_CLONE);

  return (jlong) pixad;
}

jlong Java_com_googlecode_leptonica_android_Pixa_nativeSort(JNIEnv *env, jclass clazz,
                                                            jlong nativePixa, jint field, jint order) {
  PIXA *pixas = (PIXA *) nativePixa;
  PIXA *pixad = pixaSort(pixas, field, order, NULL, L_CLONE);

  return (jlong) pixad;
}

void Java_com_googlecode_leptonica_android_Pixa_nativeDestroy(JNIEnv *env, jclass clazz,
                                                              jlong nativePixa) {
  PIXA *pixa = (PIXA *) nativePixa;

  pixaDestroy(&pixa);
}

jboolean Java_com_googlecode_leptonica_android_Pixa_nativeJoin(JNIEnv *env, jclass clazz,
                                                               jlong nativePixa, jlong otherPixa) {
  PIXA *pixa = (PIXA *) nativePixa;
  PIXA *pixas = (PIXA *) otherPixa;

  if (pixaJoin(pixa, pixas, 0, 0)) {
    return JNI_FALSE;
  }

  return JNI_TRUE;
}

jint Java_com_googlecode_leptonica_android_Pixa_nativeGetCount(JNIEnv *env, jclass clazz,
                                                               jlong nativePixa) {
  PIXA *pixa = (PIXA *) nativePixa;

  return (jint) pixaGetCount(pixa);
}

void Java_com_googlecode_leptonica_android_Pixa_nativeAddPix(JNIEnv *env, jclass clazz,
                                                             jlong nativePixa, jlong nativePix,
                                                             jint mode) {
  PIXA *pixa = (PIXA *) nativePixa;
  PIX *pix = (PIX *) nativePix;

  pixaAddPix(pixa, pix, mode);
}

void Java_com_googlecode_leptonica_android_Pixa_nativeAddBox(JNIEnv *env, jclass clazz,
                                                             jlong nativePixa, jlong nativeBox,
                                                             jint mode) {
  PIXA *pixa = (PIXA *) nativePixa;
  BOX *box = (BOX *) nativeBox;

  pixaAddBox(pixa, box, mode);
}

void Java_com_googlecode_leptonica_android_Pixa_nativeAdd(JNIEnv *env, jclass clazz,
                                                          jlong nativePixa, jlong nativePix,
                                                          jlong nativeBox, jint mode) {
  PIXA *pixa = (PIXA *) nativePixa;
  PIX *pix = (PIX *) nativePix;
  BOX *box = (BOX *) nativeBox;

  pixaAddPix(pixa, pix, mode);
  pixaAddBox(pixa, box, mode);
}

void Java_com_googlecode_leptonica_android_Pixa_nativeReplacePix(JNIEnv *env, jclass clazz,
                                                                 jlong nativePixa, jint index,
                                                                 jlong nativePix, jlong nativeBox) {
  PIXA *pixa = (PIXA *) nativePixa;
  PIX *pix = (PIX *) nativePix;
  BOX *box = (BOX *) nativeBox;

  pixaReplacePix(pixa, index, pix, box);
}

void Java_com_googlecode_leptonica_android_Pixa_nativeMergeAndReplacePix(JNIEnv *env, jclass clazz,
                                                                         jlong nativePixa,
                                                                         jint indexA, jint indexB) {
  PIXA *pixa = (PIXA *) nativePixa;

  l_int32 op;
  l_int32 x, y, w, h;
  l_int32 dx, dy, dw, dh;
  PIX *pixs, *pixd;
  BOX *boxA, *boxB, *boxd;

  boxA = pixaGetBox(pixa, indexA, L_CLONE);
  boxB = pixaGetBox(pixa, indexB, L_CLONE);
  boxd = boxBoundingRegion(boxA, boxB);

  boxGetGeometry(boxd, &x, &y, &w, &h);
  pixd = pixCreate(w, h, 1);

  op = PIX_SRC | PIX_DST;

  pixs = pixaGetPix(pixa, indexA, L_CLONE);
  boxGetGeometry(boxA, &dx, &dy, &dw, &dh);
  pixRasterop(pixd, dx - x, dy - y, dw, dh, op, pixs, 0, 0);
  pixDestroy(&pixs);
  boxDestroy(&boxA);

  pixs = pixaGetPix(pixa, indexB, L_CLONE);
  boxGetGeometry(boxB, &dx, &dy, &dw, &dh);
  pixRasterop(pixd, dx - x, dy - y, dw, dh, op, pixs, 0, 0);
  pixDestroy(&pixs);
  boxDestroy(&boxB);

  pixaReplacePix(pixa, indexA, pixd, boxd);

}

jboolean Java_com_googlecode_leptonica_android_Pixa_nativeWriteToFileRandomCmap(JNIEnv *env,
                                                                                jclass clazz,
                                                                                jlong nativePixa,
                                                                                jstring fileName,
                                                                                jint width,
                                                                                jint height) {
  PIX *pixtemp;
  PIXA *pixa = (PIXA *) nativePixa;

  const char *c_fileName = env->GetStringUTFChars(fileName, NULL);
  if (c_fileName == NULL) {
    LOGE("could not extract fileName string!");
    return JNI_FALSE;
  }

  if (pixaGetCount(pixa) > 0) {
    pixtemp = pixaDisplayRandomCmap(pixa, (l_int32) width, (l_int32) height);
  } else {
    pixtemp = pixCreate((l_int32) width, (l_int32) height, 1);
  }

  pixWrite(c_fileName, pixtemp, IFF_BMP);
  pixDestroy(&pixtemp);

  env->ReleaseStringUTFChars(fileName, c_fileName);

  return JNI_TRUE;
}

jint Java_com_googlecode_leptonica_android_Pixa_nativeGetPix(JNIEnv *env, jclass clazz,
                                                             jlong nativePixa, jint index) {
  PIXA *pixa = (PIXA *) nativePixa;
  PIX *pix = pixaGetPix(pixa, (l_int32) index, L_CLONE);

  return (jlong) pix;
}

jlong Java_com_googlecode_leptonica_android_Pixa_nativeGetBox(JNIEnv *env, jclass clazz,
                                                              jlong nativePixa, jint index) {
  PIXA *pixa = (PIXA *) nativePixa;
  BOX *box = pixaGetBox(pixa, (l_int32) index, L_CLONE);

  return (jlong) box;
}

jboolean Java_com_googlecode_leptonica_android_Pixa_nativeGetBoxGeometry(JNIEnv *env, jclass clazz,
                                                                         jlong nativePixa,
                                                                         jint index,
                                                                         jintArray dimensions) {
  PIXA *pixa = (PIXA *) nativePixa;
  jint *dimensionArray = env->GetIntArrayElements(dimensions, NULL);
  l_int32 x, y, w, h;

  if (pixaGetBoxGeometry(pixa, (l_int32) index, &x, &y, &w, &h)) {
    return JNI_FALSE;
  }

  dimensionArray[0] = x;
  dimensionArray[1] = y;
  dimensionArray[2] = w;
  dimensionArray[3] = h;

  env->ReleaseIntArrayElements(dimensions, dimensionArray, 0);

  return JNI_TRUE;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */
