///////////////////////////////////////////////////////////////////////
// File:        reconfig.h
// Description: Network layer that reconfigures the scaling vs feature
//              depth.
// Author:      Ray Smith
// Created:     Wed Feb 26 15:37:42 PST 2014
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
#ifndef TESSERACT_LSTM_RECONFIG_H_
#define TESSERACT_LSTM_RECONFIG_H_


#include "genericvector.h"
#include "matrix.h"
#include "network.h"

namespace tesseract {

// Reconfigures (Shrinks) the inputs by concatenating an x_scale by y_scale tile
// of inputs together, producing a single, deeper output per tile.
// Note that fractional parts are truncated for efficiency, so make sure the
// input stride is a multiple of the y_scale factor!
class Reconfig : public Network {
 public:
  Reconfig(const STRING& name, int ni, int x_scale, int y_scale);
  virtual ~Reconfig();

  // Returns the shape output from the network given an input shape (which may
  // be partially unknown ie zero).
  virtual StaticShape OutputShape(const StaticShape& input_shape) const;

  virtual STRING spec() const {
    STRING spec;
    spec.add_str_int("S", y_scale_);
    spec.add_str_int(",", x_scale_);
    return spec;
  }

  // Returns an integer reduction factor that the network applies to the
  // time sequence. Assumes that any 2-d is already eliminated. Used for
  // scaling bounding boxes of truth data.
  // WARNING: if GlobalMinimax is used to vary the scale, this will return
  // the last used scale factor. Call it before any forward, and it will return
  // the minimum scale factor of the paths through the GlobalMinimax.
  virtual int XScaleFactor() const;

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
  // Non-serialized data used to store parameters between forward and back.
  StrideMap back_map_;
  // Serialized data.
  inT32 x_scale_;
  inT32 y_scale_;
};

}  // namespace tesseract.


#endif  // TESSERACT_LSTM_SUBSAMPLE_H_
