///////////////////////////////////////////////////////////////////////
// File:        simddetect.h
// Description: Architecture detector.
// Author:      Stefan Weil (based on code from Ray Smith)
//
// (C) Copyright 2014, Google Inc.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
///////////////////////////////////////////////////////////////////////

#include "platform.h"

// Architecture detector. Add code here to detect any other architectures for
// SIMD-based faster dot product functions. Intended to be a single static
// object, but it does no real harm to have more than one.
class SIMDDetect {
 public:
  // Returns true if AVX is available on this system.
  static inline bool IsAVXAvailable() {
    return detector.avx_available_;
  }
  // Returns true if SSE4.1 is available on this system.
  static inline bool IsSSEAvailable() {
    return detector.sse_available_;
  }

 private:
  // Constructor, must set all static member variables.
  SIMDDetect();

 private:
  // Singleton.
  static SIMDDetect detector;
  // If true, then AVX has been detected.
  static TESS_API bool avx_available_;
  // If true, then SSe4.1 has been detected.
  static TESS_API bool sse_available_;
};
