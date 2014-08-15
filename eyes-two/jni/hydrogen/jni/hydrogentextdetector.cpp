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
#include "hydrogentextdetector.h"

#define DEBUG_MODE false

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

jlong Java_com_googlecode_eyesfree_textdetect_HydrogenTextDetector_nativeConstructor(
    JNIEnv *env,
    jclass clazz,
    jlong nativePtr) {
  if (DEBUG_MODE) LOGV(__FUNCTION__);

  HydrogenTextDetector *ptr = new HydrogenTextDetector();

  return (jlong) ptr;
}

void Java_com_googlecode_eyesfree_textdetect_HydrogenTextDetector_nativeDestructor(
    JNIEnv *env,
    jclass clazz,
    jlong nativePtr) {
  if (DEBUG_MODE) LOGV(__FUNCTION__);

  HydrogenTextDetector *ptr = (HydrogenTextDetector *) nativePtr;

  delete ptr;
}

bool getBoolField(JNIEnv *env, jclass &clazz, jobject &obj, const char *field) {
  jfieldID fieldID = env->GetFieldID(clazz, field, "Z");

  return (env->GetBooleanField(obj, fieldID) == JNI_TRUE);
}

int getIntField(JNIEnv *env, jclass &clazz, jobject &obj, const char *field) {
  jfieldID fieldID = env->GetFieldID(clazz, field, "I");

  return (int) env->GetIntField(obj, fieldID);
}

float getFloatField(JNIEnv *env, jclass &clazz, jobject &obj, const char *field) {
  jfieldID fieldID = env->GetFieldID(clazz, field, "F");

  return (float) env->GetFloatField(obj, fieldID);
}

void getStringField(JNIEnv *env, jclass &clazz, jobject &obj, const char *field,
                    char *dst) {
  jfieldID fieldID = env->GetFieldID(clazz, field, "Ljava/lang/String;");
  jstring str = (jstring) env->GetObjectField(obj, fieldID);

  if (str != NULL) {
    jsize len = L_MIN(env->GetStringLength(str), 254);

    env->GetStringUTFRegion(str, 0, len, dst);

    dst[len + 1] = 0;
  } else {
    dst[0] = 0;
  }
}

void Java_com_googlecode_eyesfree_textdetect_HydrogenTextDetector_nativeSetParameters(
    JNIEnv *env,
    jclass clazz,
    jlong nativePtr,
    jobject params) {
  if (DEBUG_MODE) LOGV(__FUNCTION__);

  HydrogenTextDetector *ptr = (HydrogenTextDetector *) nativePtr;
  HydrogenTextDetector::TextDetectorParameters *myParams = ptr->GetMutableParameters();

  jclass paramClass = env->GetObjectClass(params);

  getStringField(env, paramClass, params, "out_dir", myParams->out_dir);

  myParams->debug = getBoolField(env, paramClass, params, "debug");
  myParams->edge_tile_x = getIntField(env, paramClass, params, "edge_tile_x");
  myParams->edge_tile_y = getIntField(env, paramClass, params, "edge_tile_y");
  myParams->edge_thresh = getIntField(env, paramClass, params, "edge_thresh");
  myParams->edge_avg_thresh = getIntField(env, paramClass, params, "edge_avg_thresh");

  myParams->skew_enabled = getBoolField(env, paramClass, params, "skew_enabled");
  myParams->skew_min_angle = getFloatField(env, paramClass, params, "skew_min_angle");
  myParams->skew_sweep_range = getFloatField(env, paramClass, params, "skew_sweep_range");
  myParams->skew_sweep_delta = getFloatField(env, paramClass, params, "skew_sweep_delta");
  myParams->skew_sweep_reduction = getIntField(env, paramClass, params, "skew_sweep_reduction");
  myParams->skew_search_reduction = getIntField(env, paramClass, params, "skew_search_reduction");
  myParams->skew_search_min_delta = getFloatField(env, paramClass, params, "skew_search_min_delta");

  myParams->single_min_aspect = getFloatField(env, paramClass, params, "single_min_aspect");
  myParams->single_max_aspect = getFloatField(env, paramClass, params, "single_max_aspect");
  myParams->single_min_area = getIntField(env, paramClass, params, "single_min_area");
  myParams->single_min_density = getFloatField(env, paramClass, params, "single_min_density");

  myParams->pair_h_ratio = getFloatField(env, paramClass, params, "pair_h_ratio");
  myParams->pair_d_ratio = getFloatField(env, paramClass, params, "pair_d_ratio");
  myParams->pair_h_dist_ratio = getFloatField(env, paramClass, params, "pair_h_dist_ratio");
  myParams->pair_v_dist_ratio = getFloatField(env, paramClass, params, "pair_v_dist_ratio");
  myParams->pair_h_shared = getFloatField(env, paramClass, params, "pair_h_shared");

  myParams->cluster_width_spacing = getIntField(env, paramClass, params, "cluster_width_spacing");
  myParams->cluster_shared_edge = getFloatField(env, paramClass, params, "cluster_shared_edge");
  myParams->cluster_h_ratio = getFloatField(env, paramClass, params, "cluster_h_ratio");

  myParams->cluster_min_blobs = getIntField(env, paramClass, params, "cluster_min_blobs");
  myParams->cluster_min_aspect = getFloatField(env, paramClass, params, "cluster_min_aspect");
  myParams->cluster_min_fdr = getFloatField(env, paramClass, params, "cluster_min_fdr");
  myParams->cluster_min_edge = getIntField(env, paramClass, params, "cluster_min_edge");
  myParams->cluster_min_edge_avg = getIntField(env, paramClass, params, "cluster_min_edge_avg");
}

jlong Java_com_googlecode_eyesfree_textdetect_HydrogenTextDetector_nativeGetTextAreas(
    JNIEnv *env,
    jclass clazz,
    jlong nativePtr) {
  if (DEBUG_MODE) LOGV(__FUNCTION__);

  HydrogenTextDetector *ptr = (HydrogenTextDetector *) nativePtr;

  PIXA *textAreas = ptr->GetTextAreas();

  return (jlong) textAreas;
}

jfloat Java_com_googlecode_eyesfree_textdetect_HydrogenTextDetector_nativeGetSkewAngle(
    JNIEnv *env,
    jclass clazz,
    jlong nativePtr) {
  if (DEBUG_MODE) LOGV(__FUNCTION__);

  HydrogenTextDetector *ptr = (HydrogenTextDetector *) nativePtr;

  l_float32 skew_angle = ptr->GetSkewAngle();

  return (jfloat) skew_angle;
}

jint Java_com_googlecode_eyesfree_textdetect_HydrogenTextDetector_nativeGetSourceWidth(
    JNIEnv *env,
    jclass clazz,
    jlong nativePtr) {
  if (DEBUG_MODE) LOGV(__FUNCTION__);

  HydrogenTextDetector *ptr = (HydrogenTextDetector *) nativePtr;
  PIX *pix = ptr->GetSourceImage();
  jint h = pixGetWidth(pix);

  pixDestroy(&pix);

  return (jint) h;
}

jint Java_com_googlecode_eyesfree_textdetect_HydrogenTextDetector_nativeGetSourceHeight(
    JNIEnv *env,
    jclass clazz,
    jlong nativePtr) {
  //LOGV(__FUNCTION__);

  HydrogenTextDetector *ptr = (HydrogenTextDetector *) nativePtr;
  PIX *pix = ptr->GetSourceImage();
  jint w = pixGetHeight(pix);

  pixDestroy(&pix);

  return (jint) w;
}

jfloatArray Java_com_googlecode_eyesfree_textdetect_HydrogenTextDetector_nativeGetTextConfs(
    JNIEnv *env,
    jclass clazz,
    jlong nativePtr) {
  if (DEBUG_MODE) LOGV(__FUNCTION__);

  HydrogenTextDetector *ptr = (HydrogenTextDetector *) nativePtr;
  NUMA *confs = ptr->GetTextConfs();
  l_int32 count = numaGetCount(confs);
  jfloatArray ret = env->NewFloatArray(count);
  l_float32 nval;
  jfloat jval;

  if (ret != NULL) {
    for (int i = 0; i < count; i++) {
      numaGetFValue(confs, i, &nval);
      jval = (jfloat) nval;
      env->SetFloatArrayRegion(ret, i, 1, &jval);
    }
  }

  numaDestroy(&confs);

  return ret;
}

jlong Java_com_googlecode_eyesfree_textdetect_HydrogenTextDetector_nativeGetSourceImage(
    JNIEnv *env,
    jclass clazz,
    jlong nativePtr) {
  if (DEBUG_MODE) LOGV(__FUNCTION__);

  HydrogenTextDetector *ptr = (HydrogenTextDetector *) nativePtr;

  return (jlong) ptr->GetSourceImage();
}

void Java_com_googlecode_eyesfree_textdetect_HydrogenTextDetector_nativeSetSourceImage(
    JNIEnv *env,
    jclass clazz,
    jlong nativePtr,
    jlong nativePix) {
  if (DEBUG_MODE) LOGV(__FUNCTION__);

  HydrogenTextDetector *ptr = (HydrogenTextDetector *) nativePtr;
  PIX *pix = (PIX *) nativePix;

  ptr->SetSourceImage(pix);
}

void Java_com_googlecode_eyesfree_textdetect_HydrogenTextDetector_nativeDetectText(
    JNIEnv *env,
    jclass clazz,
    jlong nativePtr) {
  if (DEBUG_MODE) LOGV(__FUNCTION__);

  HydrogenTextDetector *ptr = (HydrogenTextDetector *) nativePtr;

  ptr->DetectText();
}

void Java_com_googlecode_eyesfree_textdetect_HydrogenTextDetector_nativeClear(
    JNIEnv *env,
    jclass clazz,
    jlong nativePtr) {
  if (DEBUG_MODE) LOGV(__FUNCTION__);

  HydrogenTextDetector *ptr = (HydrogenTextDetector *) nativePtr;

  ptr->Clear();
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#undef DEBUG_MODE
