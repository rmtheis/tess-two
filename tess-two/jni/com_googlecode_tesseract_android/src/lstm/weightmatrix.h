///////////////////////////////////////////////////////////////////////
// File:        weightmatrix.h
// Description: Hides distinction between float/int implementations.
// Author:      Ray Smith
// Created:     Tue Jun 17 09:05:39 PST 2014
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

#ifndef TESSERACT_LSTM_WEIGHTMATRIX_H_
#define TESSERACT_LSTM_WEIGHTMATRIX_H_

#include "genericvector.h"
#include "matrix.h"
#include "tprintf.h"

namespace tesseract {

// Convenience instantiation of GENERIC_2D_ARRAY<double> with additional
// operations to write a strided vector, so the transposed form of the input
// is memory-contiguous.
class TransposedArray : public GENERIC_2D_ARRAY<double> {
 public:
  // Copies the whole input transposed, converted to double, into *this.
  void Transpose(const GENERIC_2D_ARRAY<double>& input);
  // Writes a vector of data representing a timestep (gradients or sources).
  // The data is assumed to be of size1 in size (the strided dimension).
  void WriteStrided(int t, const float* data) {
    int size1 = dim1();
    for (int i = 0; i < size1; ++i) put(i, t, data[i]);
  }
  void WriteStrided(int t, const double* data) {
    int size1 = dim1();
    for (int i = 0; i < size1; ++i) put(i, t, data[i]);
  }
  // Prints the first and last num elements of the un-transposed array.
  void PrintUnTransposed(int num) {
    int num_features = dim1();
    int width = dim2();
    for (int y = 0; y < num_features; ++y) {
      for (int t = 0; t < width; ++t) {
        if (num == 0 || t < num || t + num >= width) {
          tprintf(" %g", (*this)(y, t));
        }
      }
      tprintf("\n");
    }
  }
};  // class TransposedArray

// Generic weight matrix for network layers. Can store the matrix as either
// an array of floats or inT8. Provides functions to compute the forward and
// backward steps with the matrix and updates to the weights.
class WeightMatrix {
 public:
  WeightMatrix() : int_mode_(false), use_ada_grad_(false) {}
  // Sets up the network for training. Initializes weights using weights of
  // scale `range` picked according to the random number generator `randomizer`.
  // Note the order is outputs, inputs, as this is the order of indices to
  // the matrix, so the adjacent elements are multiplied by the input during
  // a forward operation.
  int InitWeightsFloat(int no, int ni, bool ada_grad, float weight_range,
                       TRand* randomizer);

  // Converts a float network to an int network. Each set of input weights that
  // corresponds to a single output weight is converted independently:
  // Compute the max absolute value of the weight set.
  // Scale so the max absolute value becomes MAX_INT8.
  // Round to integer.
  // Store a multiplicative scale factor (as a float) that will reproduce
  //   the original value, subject to rounding errors.
  void ConvertToInt();

  // Accessors.
  bool is_int_mode() const {
    return int_mode_;
  }
  int NumOutputs() const { return int_mode_ ? wi_.dim1() : wf_.dim1(); }
  // Provides one set of weights. Only used by peep weight maxpool.
  const double* GetWeights(int index) const { return wf_[index]; }
  // Provides access to the deltas (dw_).
  double GetDW(int i, int j) const { return dw_(i, j); }

  // Allocates any needed memory for running Backward, and zeroes the deltas,
  // thus eliminating any existing momentum.
  void InitBackward(bool ada_grad);

  // Writes to the given file. Returns false in case of error.
  bool Serialize(bool training, TFile* fp) const;
  // Reads from the given file. Returns false in case of error.
  // If swap is true, assumes a big/little-endian swap is needed.
  bool DeSerialize(bool training, bool swap, TFile* fp);
  // As DeSerialize, but reads an old (float) format WeightMatrix for
  // backward compatibility.
  bool DeSerializeOld(bool training, bool swap, TFile* fp);

  // Computes matrix.vector v = Wu.
  // u is of size W.dim2() - 1 and the output v is of size W.dim1().
  // u is imagined to have an extra element at the end with value 1, to
  // implement the bias, but it doesn't actually have it.
  // Asserts that the call matches what we have.
  void MatrixDotVector(const double* u, double* v) const;
  void MatrixDotVector(const inT8* u, double* v) const;
  // MatrixDotVector for peep weights, MultiplyAccumulate adds the
  // component-wise products of *this[0] and v to inout.
  void MultiplyAccumulate(const double* v, double* inout);
  // Computes vector.matrix v = uW.
  // u is of size W.dim1() and the output v is of size W.dim2() - 1.
  // The last result is discarded, as v is assumed to have an imaginary
  // last value of 1, as with MatrixDotVector.
  void VectorDotMatrix(const double* u, double* v) const;
  // Fills dw_[i][j] with the dot product u[i][] . v[j][], using elements
  // from u and v, starting with u[i][offset] and v[j][offset].
  // Note that (matching MatrixDotVector) v[last][] is missing, presumed 1.0.
  // Runs parallel if requested. Note that inputs must be transposed.
  void SumOuterTransposed(const TransposedArray& u, const TransposedArray& v,
                          bool parallel);
  // Updates the weights using the given learning rate and momentum.
  // num_samples is the quotient to be used in the adagrad computation iff
  // use_ada_grad_ is true.
  void Update(double learning_rate, double momentum, int num_samples);
  // Adds the dw_ in other to the dw_ is *this.
  void AddDeltas(const WeightMatrix& other);
  // Sums the products of weight updates in *this and other, splitting into
  // positive (same direction) in *same and negative (different direction) in
  // *changed.
  void CountAlternators(const WeightMatrix& other, double* same,
                        double* changed) const;

  void Debug2D(const char* msg);

  // Computes and returns the dot product of the two n-vectors u and v.
  static double DotProduct(const double* u, const double* v, int n);
  // Utility function converts an array of float to the corresponding array
  // of double.
  static void FloatToDouble(const GENERIC_2D_ARRAY<float>& wf,
                            GENERIC_2D_ARRAY<double>* wd);

 private:
  // Computes matrix.vector v = Wu.
  // u is of size starts.back()+extents.back() and the output v is of size
  // starts.size().
  // The weight matrix w, is of size starts.size()xMAX(extents)+add_bias_fwd.
  // If add_bias_fwd, an extra element at the end of w[i] is the bias weight
  // and is added to v[i].
  static void MatrixDotVectorInternal(const GENERIC_2D_ARRAY<double>& w,
                                      bool add_bias_fwd, bool skip_bias_back,
                                      const double* u, double* v);

 private:
  // Choice between float and 8 bit int implementations.
  GENERIC_2D_ARRAY<double> wf_;
  GENERIC_2D_ARRAY<inT8> wi_;
  // Transposed copy of wf_, used only for Backward, and set with each Update.
  TransposedArray wf_t_;
  // Which of wf_ and wi_ are we actually using.
  bool int_mode_;
  // True if we are running adagrad in this weight matrix.
  bool use_ada_grad_;
  // If we are using wi_, then scales_ is a factor to restore the row product
  // with a vector to the correct range.
  GenericVector<double> scales_;
  // Weight deltas. dw_ is the new delta, and updates_ the momentum-decaying
  // amount to be added to wf_/wi_.
  GENERIC_2D_ARRAY<double> dw_;
  GENERIC_2D_ARRAY<double> updates_;
  // Iff use_ada_grad_, the sum of squares of dw_. The number of samples is
  // given to Update(). Serialized iff use_ada_grad_.
  GENERIC_2D_ARRAY<double> dw_sq_sum_;
};

}  // namespace tesseract.

#endif  // TESSERACT_LSTM_WEIGHTMATRIX_H_
