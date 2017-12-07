///////////////////////////////////////////////////////////////////////
// File:        fullyconnected.h
// Description: Simple feed-forward layer with various non-linearities.
// Author:      Ray Smith
// Created:     Wed Feb 26 14:46:06 PST 2014
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

#ifndef TESSERACT_LSTM_FULLYCONNECTED_H_
#define TESSERACT_LSTM_FULLYCONNECTED_H_

#include "network.h"
#include "networkscratch.h"

namespace tesseract {

// C++ Implementation of the Softmax (output) class from lstm.py.
class FullyConnected : public Network {
 public:
  FullyConnected(const STRING& name, int ni, int no, NetworkType type);
  virtual ~FullyConnected();

  // Returns the shape output from the network given an input shape (which may
  // be partially unknown ie zero).
  virtual StaticShape OutputShape(const StaticShape& input_shape) const;

  virtual STRING spec() const {
    STRING spec;
    if (type_ == NT_TANH)
      spec.add_str_int("Ft", no_);
    else if (type_ == NT_LOGISTIC)
      spec.add_str_int("Fs", no_);
    else if (type_ == NT_RELU)
      spec.add_str_int("Fr", no_);
    else if (type_ == NT_LINEAR)
      spec.add_str_int("Fl", no_);
    else if (type_ == NT_POSCLIP)
      spec.add_str_int("Fp", no_);
    else if (type_ == NT_SYMCLIP)
      spec.add_str_int("Fs", no_);
    else if (type_ == NT_SOFTMAX)
      spec.add_str_int("Fc", no_);
    else
      spec.add_str_int("Fm", no_);
    return spec;
  }

  // Changes the type to the given type. Used to commute a softmax to a
  // non-output type for adding on other networks.
  void ChangeType(NetworkType type) {
    type_ = type;
  }

  // Suspends/Enables training by setting the training_ flag. Serialize and
  // DeSerialize only operate on the run-time data if state is false.
  virtual void SetEnableTraining(TrainingState state);

  // Sets up the network for training. Initializes weights using weights of
  // scale `range` picked according to the random number generator `randomizer`.
  virtual int InitWeights(float range, TRand* randomizer);

  // Converts a float network to an int network.
  virtual void ConvertToInt();

  // Provides debug output on the weights.
  virtual void DebugWeights();

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
  // Components of Forward so FullyConnected can be reused inside LSTM.
  void SetupForward(const NetworkIO& input,
                    const TransposedArray* input_transpose);
  void ForwardTimeStep(const double* d_input, const inT8* i_input, int t,
                       double* output_line);

  // Runs backward propagation of errors on the deltas line.
  // See Network for a detailed discussion of the arguments.
  virtual bool Backward(bool debug, const NetworkIO& fwd_deltas,
                        NetworkScratch* scratch,
                        NetworkIO* back_deltas);
  // Components of Backward so FullyConnected can be reused inside LSTM.
  void BackwardTimeStep(const NetworkIO& fwd_deltas, int t, double* curr_errors,
                        TransposedArray* errors_t, double* backprop);
  void FinishBackward(const TransposedArray& errors_t);

  // Updates the weights using the given learning rate and momentum.
  // num_samples is the quotient to be used in the adagrad computation iff
  // use_ada_grad_ is true.
  virtual void Update(float learning_rate, float momentum, int num_samples);
  // Sums the products of weight updates in *this and other, splitting into
  // positive (same direction) in *same and negative (different direction) in
  // *changed.
  virtual void CountAlternators(const Network& other, double* same,
                                double* changed) const;

 protected:
  // Weight arrays of size [no, ni + 1].
  WeightMatrix weights_;
  // Transposed copy of input used during training of size [ni, width].
  TransposedArray source_t_;
  // Pointer to transposed input stored elsewhere. If not null, this is used
  // in preference to calculating the transpose and storing it in source_t_.
  const TransposedArray* external_source_;
  // Activations from forward pass of size [width, no].
  NetworkIO acts_;
  // Memory of the integer mode input to forward as softmax always outputs
  // float, so the information is otherwise lost.
  bool int_mode_;
};

}  // namespace tesseract.



#endif  // TESSERACT_LSTM_FULLYCONNECTED_H_
