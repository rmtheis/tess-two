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

// Author: Xiaotao Duan

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "time_log.h"
#include "similar.h"

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jintArray JNICALL
Java_com_googlecode_eyesfree_opticflow_ImageBlur_computeSignature(
    JNIEnv* env, jclass clazz, jbyteArray input, jint width, jint height,
    jintArray signatureBuffer);

JNIEXPORT jint JNICALL
Java_com_googlecode_eyesfree_opticflow_ImageBlur_diffSignature(
    JNIEnv* env, jclass clazz, jintArray signature1, jintArray signature2);
#ifdef __cplusplus
}
#endif

JNIEXPORT jintArray JNICALL
Java_com_googlecode_eyesfree_opticflow_ImageBlur_computeSignature(
    JNIEnv* env, jclass clazz, jbyteArray input, jint width, jint height,
    jintArray signatureBuffer) {
  jboolean inputCopy = JNI_FALSE;
  jbyte* const i = env->GetByteArrayElements(input, &inputCopy);

  int sig_len = 0;

  resetTimeLog();
  uint32_t* sig = ComputeSignature(reinterpret_cast<uint8*>(i),
      width, height, &sig_len);
  timeLog("Finished image signature computation");
  printTimeLog();

  env->ReleaseByteArrayElements(input, i, JNI_ABORT);

  jintArray ret = signatureBuffer;
  if (ret == NULL || env->GetArrayLength(ret) != sig_len) {
    ret = env->NewIntArray(sig_len);
  }
  jint* body = env->GetIntArrayElements(ret, 0);
  for (int i = 0; i < sig_len; ++i) {
    body[i] = sig[i];
  }
  env->ReleaseIntArrayElements(ret, body, 0);
  return ret;
}

JNIEXPORT jint JNICALL
Java_com_googlecode_eyesfree_opticflow_ImageBlur_diffSignature(
    JNIEnv* env, jclass clazz, jintArray signature1, jintArray signature2) {
  jint* sig1 = env->GetIntArrayElements(signature1, 0);
  jint* sig2 = env->GetIntArrayElements(signature2, 0);

  int size = env->GetArrayLength(signature1);

  int diff = Diff(sig1, sig2, size);

  env->ReleaseIntArrayElements(signature1, sig1, 0);
  env->ReleaseIntArrayElements(signature2, sig2, 0);

  return diff;
}
