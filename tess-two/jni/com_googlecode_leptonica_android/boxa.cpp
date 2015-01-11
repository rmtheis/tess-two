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

void Java_com_googlecode_leptonica_android_Boxa_nativeDestroy(JNIEnv *env, jclass clazz, jlong nativeBoxa) {
  BOXA *boxa = (BOXA *) nativeBoxa;
  boxaDestroy(&boxa);
}


jboolean Java_com_googlecode_leptonica_android_Boxa_nativeGetGeometry(JNIEnv *env, jclass clazz,
                                                                     jlong nativeBoxa,
                                                                     jint index,
                                                                     jintArray dimensions) {
  BOXA *boxa = (BOXA *) nativeBoxa;
  jint *dimensionArray = env->GetIntArrayElements(dimensions, NULL);
  l_int32 x, y, w, h;

  if (boxaGetBoxGeometry(boxa,index,  &x, &y, &w, &h)) {
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
