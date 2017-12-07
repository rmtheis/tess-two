///////////////////////////////////////////////////////////////////////
// File:        input.h
// Description: Input layer class for neural network implementations.
// Author:      Ray Smith
// Created:     Thu Mar 13 08:56:26 PDT 2014
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

#ifndef TESSERACT_LSTM_INPUT_H_
#define TESSERACT_LSTM_INPUT_H_

#include "network.h"

class ScrollView;

namespace tesseract {

class Input : public Network {
 public:
  Input(const STRING& name, int ni, int no);
  Input(const STRING& name, const StaticShape& shape);
  virtual ~Input();

  virtual STRING spec() const {
    STRING spec;
    spec.add_str_int("", shape_.batch());
    spec.add_str_int(",", shape_.height());
    spec.add_str_int(",", shape_.width());
    spec.add_str_int(",", shape_.depth());
    return spec;
  }

  // Returns the required shape input to the network.
  virtual StaticShape InputShape() const { return shape_; }
  // Returns the shape output from the network given an input shape (which may
  // be partially unknown ie zero).
  virtual StaticShape OutputShape(const StaticShape& input_shape) const {
    return shape_;
  }
  // Writes to the given file. Returns false in case of error.
  // Should be overridden by subclasses, but called by their Serialize.
  virtual bool Serialize(TFile* fp) const;
  // Reads from the given file. Returns false in case of error.
  // If swap is true, assumes a big/little-endian swap is needed.
  // Should be overridden by subclasses, but NOT called by their DeSerialize.
  virtual bool DeSerialize(bool swap, TFile* fp);

  // Returns an integer reduction factor that the network applies to the
  // time sequence. Assumes that any 2-d is already eliminated. Used for
  // scaling bounding boxes of truth data.
  // WARNING: if GlobalMinimax is used to vary the scale, this will return
  // the last used scale factor. Call it before any forward, and it will return
  // the minimum scale factor of the paths through the GlobalMinimax.
  virtual int XScaleFactor() const;

  // Provides the (minimum) x scale factor to the network (of interest only to
  // input units) so they can determine how to scale bounding boxes.
  virtual void CacheXScaleFactor(int factor);

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
  // Creates and returns a Pix of appropriate size for the network from the
  // image_data. If non-null, *image_scale returns the image scale factor used.
  // Returns nullptr on error.
  /* static */
  static Pix* PrepareLSTMInputs(const ImageData& image_data,
                                const Network* network, int min_width,
                                TRand* randomizer, float* image_scale);
  // Converts the given pix to a NetworkIO of height and depth appropriate to
  // the given StaticShape:
  // If depth == 3, convert to 24 bit color, otherwise normalized grey.
  // Scale to target height, if the shape's height is > 1, or its depth if the
  // height == 1. If height == 0 then no scaling.
  // NOTE: It isn't safe for multiple threads to call this on the same pix.
  static void PreparePixInput(const StaticShape& shape, const Pix* pix,
                              TRand* randomizer, NetworkIO* input);

 private:
  // Input shape determines how images are dealt with.
  StaticShape shape_;
  // Cached total network x scale factor for scaling bounding boxes.
  int cached_x_scale_;
};

}  // namespace tesseract.

#endif  // TESSERACT_LSTM_INPUT_H_

