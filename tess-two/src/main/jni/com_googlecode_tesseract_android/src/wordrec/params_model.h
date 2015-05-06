///////////////////////////////////////////////////////////////////////
// File:        params_model.h
// Description: Trained feature serialization for language parameter training.
// Author:      David Eger
// Created:     Mon Jun 11 11:26:42 PDT 2012
//
// (C) Copyright 2011, Google Inc.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
///////////////////////////////////////////////////////////////////////

#ifndef TESSERACT_WORDREC_PARAMS_MODEL_H_
#define TESSERACT_WORDREC_PARAMS_MODEL_H_

#include "params_training_featdef.h"
#include "ratngs.h"
#include "strngs.h"

namespace tesseract {

// Represents the learned weights for a given language.
class ParamsModel {
 public:
  // Enum for expressing OCR pass.
  enum PassEnum {
    PTRAIN_PASS1,
    PTRAIN_PASS2,

    PTRAIN_NUM_PASSES
  };

  ParamsModel() : pass_(PTRAIN_PASS1) {}
  ParamsModel(const char *lang, const GenericVector<float> &weights) :
    lang_(lang), pass_(PTRAIN_PASS1) { weights_vec_[pass_] = weights; }
  inline bool Initialized() {
    return weights_vec_[pass_].size() == PTRAIN_NUM_FEATURE_TYPES;
  }
  // Prints out feature weights.
  void Print();
  // Clears weights for all passes.
  void Clear() {
    for (int p = 0; p < PTRAIN_NUM_PASSES; ++p) weights_vec_[p].clear();
  }
  // Copies the weights of the given params model.
  void Copy(const ParamsModel &other_model);
  // Applies params model weights to the given features.
  // Assumes that features is an array of size PTRAIN_NUM_FEATURE_TYPES.
  float ComputeCost(const float features[]) const;
  bool Equivalent(const ParamsModel &that) const;

  // Returns true on success.
  bool SaveToFile(const char *full_path) const;

  // Returns true on success.
  bool LoadFromFile(const char *lang, const char *full_path);
  bool LoadFromFp(const char *lang, FILE *fp, inT64 end_offset);

  const GenericVector<float>& weights() const {
    return weights_vec_[pass_];
  }
  const GenericVector<float>& weights_for_pass(PassEnum pass) const {
    return weights_vec_[pass];
  }
  void SetPass(PassEnum pass) { pass_ = pass; }

 private:
  bool ParseLine(char *line, char **key, float *val);

  STRING lang_;
  // Set to the current pass type and used to determine which set of weights
  // should be used for ComputeCost() and other functions.
  PassEnum pass_;
  // Several sets of weights for various OCR passes (e.g. pass1 with adaption,
  // pass2 without adaption, etc).
  GenericVector<float> weights_vec_[PTRAIN_NUM_PASSES];
};

}  // namespace tesseract

#endif  // TESSERACT_WORDREC_PARAMS_MODEL_H_

