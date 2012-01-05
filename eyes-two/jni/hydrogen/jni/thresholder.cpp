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
#include "thresholder.h"
#include "utilities.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

jint Java_com_googlecode_eyesfree_textdetect_Thresholder_nativeSobelEdgeThreshold(JNIEnv *env,
                                                                                  jclass clazz,
                                                                                  jint nativePix,
                                                                                  jint threshold) {
  LOGV(__FUNCTION__);

  PIX *pixs = (PIX *) nativePix;
  PIX *pixd = pixThreshedSobelEdgeFilter(pixs, (l_int32) threshold);

  return (jint) pixd;
}

jint Java_com_googlecode_eyesfree_textdetect_Thresholder_nativeEdgeAdaptiveThreshold(
                                                                                     JNIEnv *env,
                                                                                     jclass clazz,
                                                                                     jint nativePix,
                                                                                     jint tileX,
                                                                                     jint tileY,
                                                                                     jint threshold,
                                                                                     jint average) {
  LOGV(__FUNCTION__);

  PIX *pixs = (PIX *) nativePix;
  PIX *pixd;

  if (pixEdgeAdaptiveThreshold(pixs, &pixd, (l_int32) tileX, (l_int32) tileY, (l_int32) threshold,
                               (l_int32) average)) {
    return (jint) 0;
  }

  return (jint) pixd;
}

jint Java_com_googlecode_eyesfree_textdetect_Thresholder_nativeFisherAdaptiveThreshold(
                                                                                       JNIEnv *env,
                                                                                       jclass clazz,
                                                                                       jint nativePix,
                                                                                       jint tileX,
                                                                                       jint tileY,
                                                                                       jfloat scoreFract,
                                                                                       jfloat thresh) {
  LOGV(__FUNCTION__);

  PIX *pixs = (PIX *) nativePix;
  PIX *pixd;

  if (pixFisherAdaptiveThreshold(pixs, &pixd, (l_int32) tileX, (l_int32) tileY,
                                 (l_float32) scoreFract, (l_float32) thresh)) {
    return (jint) 0;
  }

  return (jint) pixd;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */
