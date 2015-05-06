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
#include "pageiterator.h"
#include "allheaders.h"
#include "helpers.h"
#include "pageres.h"
#include "tesseractclass.h"

using namespace tesseract;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void Java_com_googlecode_tesseract_android_PageIterator_nativeBegin(JNIEnv *env, jclass clazz,
    jlong nativePageIterator) {
  ((PageIterator *) nativePageIterator)->Begin();
}

jboolean Java_com_googlecode_tesseract_android_PageIterator_nativeNext(JNIEnv *env, jclass clazz,
    jlong nativePageIterator, jint level) {
  PageIterator *pageIterator = (PageIterator *) nativePageIterator;
  PageIteratorLevel enumLevel = (PageIteratorLevel) level;

  return pageIterator->Next(enumLevel) ? JNI_TRUE : JNI_FALSE;
}

jintArray Java_com_googlecode_tesseract_android_PageIterator_nativeBoundingBox(JNIEnv *env, jclass clazz,
    jlong nativePageIterator, jint level) {
  int size = 4;
  jintArray result = env->NewIntArray(size);

  LOG_ASSERT((result != NULL), "Could not create Java bounding box array!");
  
  PageIterator *pageIterator = (PageIterator *) nativePageIterator;
  PageIteratorLevel enumLevel = (PageIteratorLevel) level;
  int x1, y1, x2, y2;
  pageIterator->BoundingBox(enumLevel, &x1, &y1, &x2, &y2);
  
  // fill a temp structure to use to populate the java int array
  jint fill[4];
  fill[0] = x1;
  fill[1] = y1;
  fill[2] = x2;
  fill[3] = y2;
 
  env->SetIntArrayRegion(result, 0, size, fill);
  return result;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */
