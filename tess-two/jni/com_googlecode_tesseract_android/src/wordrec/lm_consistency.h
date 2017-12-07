///////////////////////////////////////////////////////////////////////
// File:        lm_consistency.h
// Description: Struct for recording consistency of the paths  representing
//              OCR hypotheses.
// Author:      Rika Antonova
// Created:     Mon Jun 20 11:26:43 PST 2012
//
// (C) Copyright 2012, Google Inc.
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
////////////////////////////////////////////////////////////////////////

#ifndef TESSERACT_WORDREC_LM_CONSISTENCY_H_
#define TESSERACT_WORDREC_LM_CONSISTENCY_H_

#include "dawg.h"
#include "dict.h"
#include "host.h"
#include "ratngs.h"

namespace tesseract {

static const char * const XHeightConsistencyEnumName[] = {
    "XH_GOOD",
    "XH_SUBNORMAL",
    "XH_INCONSISTENT",
};

// Struct for keeping track of the consistency of the path.
struct LMConsistencyInfo {
  enum ChartypeEnum { CT_NONE, CT_ALPHA, CT_DIGIT, CT_OTHER};

  // How much do characters have to be shifted away from normal parameters
  // before we say they're not normal?
  static const int kShiftThresh = 1;

  // How much shifting from subscript to superscript and back
  // before we declare shenanigans?
  static const int kMaxEntropy = 1;

  // Script positions - order important for entropy calculation.
  static const int kSUB = 0, kNORM = 1, kSUP = 2;
  static const int kNumPos = 3;

  explicit LMConsistencyInfo(const LMConsistencyInfo* parent_info) {
    if (parent_info == NULL) {
      // Initialize from scratch.
      num_alphas = 0;
      num_digits = 0;
      num_punc = 0;
      num_other = 0;
      chartype = CT_NONE;
      punc_ref = NO_EDGE;
      invalid_punc = false;
      num_non_first_upper = 0;
      num_lower = 0;
      script_id = 0;
      inconsistent_script = false;
      num_inconsistent_spaces = 0;
      inconsistent_font = false;
      // Initialize XHeight stats.
      for (int i = 0; i < kNumPos; i++) {
        xht_count[i] = 0;
        xht_count_punc[i] = 0;
        xht_lo[i] = 0;
        xht_hi[i] = 256;  // kBlnCellHeight
      }
      xht_sp = -1;  // This invalid value indicates that there was no parent.
      xpos_entropy = 0;
      xht_decision = XH_GOOD;
    } else {
      // Copy parent info
      *this = *parent_info;
    }
  }
  inline int NumInconsistentPunc() const {
    return invalid_punc ? num_punc : 0;
  }
  inline int NumInconsistentCase() const {
    return (num_non_first_upper > num_lower) ? num_lower : num_non_first_upper;
  }
  inline int NumInconsistentChartype() const {
    return (NumInconsistentPunc() + num_other +
        ((num_alphas > num_digits) ? num_digits : num_alphas));
  }
  inline bool Consistent() const {
    return (NumInconsistentPunc() == 0 && NumInconsistentCase() == 0 &&
            NumInconsistentChartype() == 0 && !inconsistent_script &&
            !inconsistent_font && !InconsistentXHeight());
  }
  inline int  NumInconsistentSpaces() const {
    return num_inconsistent_spaces;
  }
  inline int InconsistentXHeight() const {
    return xht_decision == XH_INCONSISTENT;
  }
  void ComputeXheightConsistency(const BLOB_CHOICE *b, bool is_punc);
  float BodyMinXHeight() const {
    if (InconsistentXHeight())
      return 0.0f;
    return xht_lo[kNORM];
  }
  float BodyMaxXHeight() const {
    if (InconsistentXHeight())
      return static_cast<float>(MAX_INT16);
    return xht_hi[kNORM];
  }

  int num_alphas;
  int num_digits;
  int num_punc;
  int num_other;
  ChartypeEnum chartype;
  EDGE_REF punc_ref;
  bool invalid_punc;
  int num_non_first_upper;
  int num_lower;
  int script_id;
  bool inconsistent_script;
  int num_inconsistent_spaces;
  bool inconsistent_font;
  // Metrics clumped by position.
  float xht_lo[kNumPos];
  float xht_hi[kNumPos];
  inT16 xht_count[kNumPos];
  inT16 xht_count_punc[kNumPos];
  inT16 xht_sp;
  inT16 xpos_entropy;
  XHeightConsistencyEnum xht_decision;
};

}  // namespace tesseract

#endif  // TESSERACT_WORDREC_LM_CONSISTENCY_H_
