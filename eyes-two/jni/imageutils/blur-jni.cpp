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
#include "blur.h"

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jboolean JNICALL
Java_com_googlecode_eyesfree_opticflow_ImageBlur_isBlurred(
    JNIEnv* env, jclass clazz, jbyteArray input, jint width, jint height);

#ifdef __cplusplus
}
#endif

JNIEXPORT jboolean JNICALL
Java_com_googlecode_eyesfree_opticflow_ImageBlur_isBlurred(
    JNIEnv* env, jclass clazz, jbyteArray input, jint width, jint height) {
  jboolean inputCopy = JNI_FALSE;
  jbyte* const i = env->GetByteArrayElements(input, &inputCopy);

  float blur = 0;
  float extent = 0;

  resetTimeLog();
  int blurred = IsBlurred(reinterpret_cast<uint8*>(i),
                          width, height, &blur, &extent);
  timeLog("Finished image blur detection");
  printTimeLog();

  env->ReleaseByteArrayElements(input, i, JNI_ABORT);

  return blurred ? JNI_TRUE : JNI_FALSE;
}
