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
    jclass clazz, jlong nativeResultIterator, jint level) {
  ResultIterator *resultIterator = (ResultIterator *) nativeResultIterator;
  PageIteratorLevel enumLevel = (PageIteratorLevel) level;

  char *text = resultIterator->GetUTF8Text(enumLevel);
  jstring result = env->NewStringUTF(text);

  free(text);

  return result;
}

jfloat Java_com_googlecode_tesseract_android_ResultIterator_nativeConfidence(JNIEnv *env,
    jclass clazz, jlong nativeResultIterator, jint level) {
  ResultIterator *resultIterator = (ResultIterator *) nativeResultIterator;
  PageIteratorLevel enumLevel = (PageIteratorLevel) level;

  return (jfloat) resultIterator->Confidence(enumLevel);
}

jobjectArray Java_com_googlecode_tesseract_android_ResultIterator_nativeGetSymbolChoices(JNIEnv *env,
    jclass clazz, jlong nativeResultIterator) {

  // Get the actual result iterator (as C object)
  ResultIterator *resultIterator = (ResultIterator *) nativeResultIterator;

  // Create a choice iterator to determine the number of alternatives
  tesseract::ChoiceIterator ci(*resultIterator);
  int numberOfAlternatives = 0;
  do {
    numberOfAlternatives++;
  } while (ci.Next());

  // Create a string array to hold the choices
  jobjectArray ret = (jobjectArray) env->NewObjectArray(numberOfAlternatives, env->FindClass("java/lang/String"), env->NewStringUTF(""));

  // Save each result to the output array
  int i = 0;
  tesseract::ChoiceIterator cb(*resultIterator);
  do {
    // Create the string output
    const char *utfText = cb.GetUTF8Text();

    // Add each string to the object array elements
    char newString[strlen(utfText) + 7];
    sprintf(newString, "%s|%.2f", utfText, cb.Confidence());
    env->SetObjectArrayElement(ret, i, env->NewStringUTF(newString));

    // Move to the next element in the list
    i++;
  } while(cb.Next());

  // Return the string array
  return ret;
}

jboolean Java_com_googlecode_tesseract_android_ResultIterator_nativeIsAtBeginningOf(JNIEnv *env,
    jclass clazz, jlong nativeResultIterator, jint level) {
  ResultIterator *resultIterator = (ResultIterator *) nativeResultIterator;
  PageIteratorLevel enumLevel = (PageIteratorLevel) level;

  return (jboolean) (resultIterator->IsAtBeginningOf(enumLevel) ? JNI_TRUE : JNI_FALSE);
}

jboolean Java_com_googlecode_tesseract_android_ResultIterator_nativeIsAtFinalElement(JNIEnv *env,
    jclass clazz, jlong nativeResultIterator, jint level, jint element) {
  ResultIterator *resultIterator = (ResultIterator *) nativeResultIterator;
  PageIteratorLevel enumLevel = (PageIteratorLevel) level;
  PageIteratorLevel enumElement = (PageIteratorLevel) element;

  return (jboolean) (resultIterator->IsAtFinalElement(enumLevel, enumElement) ? JNI_TRUE : JNI_FALSE);
}

void Java_com_googlecode_tesseract_android_ResultIterator_nativeDelete(JNIEnv *env, jclass clazz,
    jlong nativeResultIterator) {
  ResultIterator *resultIterator = (ResultIterator *) nativeResultIterator;
  if (resultIterator != 0) {
    delete resultIterator;
  }
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */
