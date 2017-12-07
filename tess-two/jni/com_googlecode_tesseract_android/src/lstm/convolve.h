///////////////////////////////////////////////////////////////////////
// File:        convolve.h
// Description: Convolutional layer that stacks the inputs over its rectangle
//              and pulls in random data to fill out-of-input inputs.
//              Output is therefore same size as its input, but deeper.
// Author:      Ray Smith
// Created:     Tue Mar 18 16:45:34 PST 2014
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

#ifndef TESSERACT_LSTM_CONVOLVE_H_
#define TESSERACT_LSTM_CONVOLVE_H_

#include "genericvector.h"
#include "matrix.h"
#include "network.h"

namespace tesseract {

// Makes each time-step deeper by stacking inputs over its rectangle. Does not
// affect the size of its input. Achieves this by bringing in random values in
// out-of-input areas.
class Convolve : public Network {
 public:
  // The area of convolution is 2*half_x + 1 by 2*half_y + 1, forcing it to
  // always be odd, so the center is the current pixel.
  Convolve(const STRING& name, int ni, int half_x, int half_y);
  virtual ~Convolve();

  virtual STRING spec() const {
    STRING spec;
    spec.add_str_int("C", half_x_ * 2 + 1);
    spec.add_str_int(",", half_y_ * 2 + 1);
    return spec;
  }

  // Writes to the given file. Returns false in case of error.
  virtual bool Serialize(TFile* fp) const;
  // Reads from the given file. Returns false in case of error.
  // If swap is true, assumes a big/little-endian swap is needed.
  virtual bool DeSerialize(bool swap, TFile* fp);

  // Runs forward propagation of activations on the input line.
  // See Network for a detailed discussion of the arguments.
  virtual void Forward(bool debug, const NetworkIO& input,
                       const TransposedArray* input_transpose,
                       NetworkScratch* scratch, NetworkIO* output);

  // Runs backward propagation of errors on the deltas line.
  // See Network for a detailed discussion of the arguments.
  virtual bool Backward(bool debug, const NetworkIO& fwd_deltas,
                        NetworkScratch* scratch,
                        NetworkIO* back_deltas);

 protected:
  // Serialized data.
  inT32 half_x_;
  inT32 half_y_;
};

}  // namespace tesseract.


#endif  // TESSERACT_LSTM_SUBSAMPLE_H_
