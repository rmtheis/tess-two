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

// Author: Andrew Harp

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include "types.h"
#include "optical_flow_utils.h"
#include "time_log.h"
#include "image.h"
#include "optical_flow.h"

namespace flow {

#ifdef __cplusplus
extern "C" {
#endif

  JNIEXPORT
  void
  JNICALL
  Java_com_googlecode_eyesfree_opticflow_OpticalFlow_initNative(
      JNIEnv* env,
      jobject thiz,
      jint width,
      jint height,
      jint downsample_factor);

  JNIEXPORT
  void
  JNICALL
  Java_com_googlecode_eyesfree_opticflow_OpticalFlow_addFrameNative(
      JNIEnv* env,
      jobject thiz,
      jbyteArray photo_data,
      jlong timestamp);

  JNIEXPORT
  void
  JNICALL
  Java_com_googlecode_eyesfree_opticflow_OpticalFlow_computeFeaturesNative(
      JNIEnv* env,
      jobject thiz,
      jboolean cached_ok);

  JNIEXPORT
  void
  JNICALL
  Java_com_googlecode_eyesfree_opticflow_OpticalFlow_computeFlowNative(
      JNIEnv* env,
      jobject thiz);

  JNIEXPORT
  void
  JNICALL
  Java_com_googlecode_eyesfree_opticflow_OpticalFlow_printInfoNative(
      JNIEnv* env,
      jobject thiz);

  JNIEXPORT
  void
  JNICALL
  Java_com_googlecode_eyesfree_opticflow_OpticalFlow_addInterestRegionNative(
      JNIEnv* env,
      jobject thiz,
      jint num_x, jint num_y,
      jfloat left, jfloat top, jfloat right, jfloat bottom);

  JNIEXPORT
  jfloatArray
  JNICALL
  Java_com_googlecode_eyesfree_opticflow_OpticalFlow_getFeaturesNative(
      JNIEnv* env,
      jobject thiz,
      jboolean only_found_);

  JNIEXPORT
  void
  JNICALL
  Java_com_googlecode_eyesfree_opticflow_OpticalFlow_getAccumulatedDeltaNative(
      JNIEnv* env,
      jobject thiz,
      jlong timestamp,
      jfloat position_x,
      jfloat position_y,
      jfloat radius,
      jfloatArray delta);

  JNIEXPORT
  void
  JNICALL
  Java_com_googlecode_eyesfree_opticflow_OpticalFlow_resetNative(
      JNIEnv* env,
      jobject thiz);

#ifdef __cplusplus
}
#endif

OpticalFlow* optical_flow = NULL;


JNIEXPORT
void
JNICALL
Java_com_googlecode_eyesfree_opticflow_OpticalFlow_initNative(
    JNIEnv* env,
    jobject thiz,
    jint width,
    jint height,
    jint downsample_factor) {
  SAFE_DELETE(optical_flow);

  LOGI("Initializing optical flow. %dx%d, %d",
       width, height, downsample_factor);
  optical_flow = new OpticalFlow(width, height, downsample_factor);
}


JNIEXPORT
void
JNICALL
Java_com_googlecode_eyesfree_opticflow_OpticalFlow_addFrameNative(
    JNIEnv* env,
    jobject thiz,
    jbyteArray photo_data,
    jlong timestamp) {
  CHECK(optical_flow != NULL, "Optical flow not initialized!");

  resetTimeLog();
  timeLog("Starting optical flow");

  // Copy image into currFrame.
  jboolean iCopied = JNI_FALSE;
  jbyte* pixels = env->GetByteArrayElements(photo_data, &iCopied);

  timeLog("Got elements");

  // Add the frame to the optical flow object.
  optical_flow->nextFrame(reinterpret_cast<uint8*>(pixels), timestamp);

  env->ReleaseByteArrayElements(photo_data, pixels, JNI_ABORT);
  timeLog("Released elements");
}


JNIEXPORT
void
JNICALL
Java_com_googlecode_eyesfree_opticflow_OpticalFlow_computeFeaturesNative(
    JNIEnv* env,
    jobject thiz,
    jboolean cached_ok) {
  CHECK(optical_flow != NULL, "Optical flow not initialized!");

  optical_flow->computeFeatures(cached_ok);
}


JNIEXPORT
void
JNICALL
Java_com_googlecode_eyesfree_opticflow_OpticalFlow_computeFlowNative(
    JNIEnv* env,
    jobject thiz) {
  CHECK(optical_flow != NULL, "Optical flow not initialized!");

  optical_flow->computeFlow();
}


JNIEXPORT
void
JNICALL
Java_com_googlecode_eyesfree_opticflow_OpticalFlow_printInfoNative(
    JNIEnv* env,
    jobject thiz) {
  CHECK(optical_flow != NULL, "Optical flow not initialized!");

  printTimeLog();
  optical_flow->printInfo();
}


JNIEXPORT
void
JNICALL
Java_com_googlecode_eyesfree_opticflow_OpticalFlow_addInterestRegionNative(
    JNIEnv* env,
    jobject thiz,
    jint num_x, jint num_y,
    jfloat left, jfloat top, jfloat right, jfloat bottom) {
  CHECK(optical_flow != NULL, "Optical flow not initialized!");

  optical_flow->addInterestRegion(num_x, num_y, left, top, right, bottom);
  timeLog("Added interest region.");
}


JNIEXPORT
jfloatArray
JNICALL
Java_com_googlecode_eyesfree_opticflow_OpticalFlow_getFeaturesNative(
    JNIEnv* env,
    jobject thiz,
    jboolean only_found) {
  CHECK(optical_flow != NULL, "Optical flow not initialized!");

  jfloat feature_arr[MAX_FEATURES * FEATURE_STEP];

  const int32 number_of_features =
      optical_flow->getFeatures(only_found, feature_arr);

  // Create and return the array that will be passed back to Java.
  jfloatArray features = env->NewFloatArray(number_of_features * FEATURE_STEP);
  if (features == NULL) {
    LOGE("null array!");
    return NULL;
  }
  env->SetFloatArrayRegion(
      features, 0, number_of_features * FEATURE_STEP, feature_arr);

  return features;
}


JNIEXPORT
void
JNICALL
Java_com_googlecode_eyesfree_opticflow_OpticalFlow_getAccumulatedDeltaNative(
    JNIEnv* env,
    jobject thiz,
    jlong timestamp,
    jfloat position_x,
    jfloat position_y,
    jfloat radius,
    jfloatArray delta) {
  CHECK(optical_flow != NULL, "Optical flow not initialized!");

  const Point2D query_position(position_x, position_y);
  const Point2D query_delta =
      optical_flow->getAccumulatedDelta(query_position, radius, timestamp);
  const jfloat point_arr[] = { query_delta.x, query_delta.y };

  env->SetFloatArrayRegion(delta, 0, 2, point_arr);
}


JNIEXPORT
void
JNICALL
Java_com_googlecode_eyesfree_opticflow_OpticalFlow_resetNative(
    JNIEnv* env, jobject thiz) {
  LOGI("Cleaning up optical flow.");
  SAFE_DELETE(optical_flow);
}

}  // namespace flow
