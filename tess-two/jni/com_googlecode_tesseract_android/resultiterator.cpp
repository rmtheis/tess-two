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

#include <stdio.h>
#include "common.h"
#include "resultiterator.h"
#include "allheaders.h"
#include "pageres.h"
#include "tesseractclass.h"

using namespace tesseract;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

jstring Java_com_googlecode_tesseract_android_ResultIterator_nativeGetUTF8Text(JNIEnv *env,
    jclass clazz, jint nativeResultIterator, jint level) {
  ResultIterator *resultIterator = (ResultIterator *) nativeResultIterator;
  PageIteratorLevel enumLevel = (PageIteratorLevel) level;

  char *text = resultIterator->GetUTF8Text(enumLevel);
  jstring result = env->NewStringUTF(text);

  free(text);

  return result;
}

jfloat Java_com_googlecode_tesseract_android_ResultIterator_nativeConfidence(JNIEnv *env,
    jclass clazz, jint nativeResultIterator, jint level) {
  ResultIterator *resultIterator = (ResultIterator *) nativeResultIterator;
  PageIteratorLevel enumLevel = (PageIteratorLevel) level;

  return (jfloat) resultIterator->Confidence(enumLevel);
}

jboolean Java_com_googlecode_tesseract_android_ResultIterator_nativeNext(JNIEnv *env, jclass clazz,
    jint nativeResultIterator, jint level) {
  ResultIterator *resultIterator = (ResultIterator *) nativeResultIterator;
  PageIteratorLevel enumLevel = (PageIteratorLevel) level;

  return resultIterator->Next(enumLevel) ? JNI_TRUE : JNI_FALSE;
}

void Java_com_googlecode_tesseract_android_ResultIterator_nativeDelete(JNIEnv *env, jclass clazz,
    jint nativeResultIterator) {
  ResultIterator *resultIterator = (ResultIterator *) nativeResultIterator;
  if (resultIterator != 0) {
    delete resultIterator;
  }
  return JNI_TRUE;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */
