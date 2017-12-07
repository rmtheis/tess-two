///////////////////////////////////////////////////////////////////////
// File:        weightmatrix.h
// Description: Hides distinction between float/int implementations.
// Author:      Ray Smith
// Created:     Tue Jun 17 11:46:20 PST 2014
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

#include "weightmatrix.h"

#include "dotproductavx.h"
#include "dotproductsse.h"
#include "simddetect.h"
#include "statistc.h"
#include "tprintf.h"

namespace tesseract {

// Copies the whole input transposed, converted to double, into *this.
void TransposedArray::Transpose(const GENERIC_2D_ARRAY<double>& input) {
  int width = input.dim1();
  int num_features = input.dim2();
  ResizeNoInit(num_features, width);
  for (int t = 0; t < width; ++t) WriteStrided(t, input[t]);
}

// Sets up the network for training. Initializes weights using weights of
// scale `range` picked according to the random number generator `randomizer`.
int WeightMatrix::InitWeightsFloat(int no, int ni, bool ada_grad,
                                   float weight_range, TRand* randomizer) {
  int_mode_ = false;
  wf_.Resize(no, ni, 0.0);
  if (randomizer != NULL) {
    for (int i = 0; i < no; ++i) {
      for (int j = 0; j < ni; ++j) {
        wf_[i][j] = randomizer->SignedRand(weight_range);
      }
    }
  }
  InitBackward(ada_grad);
  return ni * no;
}

// Converts a float network to an int network. Each set of input weights that
// corresponds to a single output weight is converted independently:
// Compute the max absolute value of the weight set.
// Scale so the max absolute value becomes MAX_INT8.
// Round to integer.
// Store a multiplicative scale factor (as a double) that will reproduce
//   the original value, subject to rounding errors.
void WeightMatrix::ConvertToInt() {
  wi_.ResizeNoInit(wf_.dim1(), wf_.dim2());
  scales_.init_to_size(wi_.dim1(), 0.0);
  int dim2 = wi_.dim2();
  for (int t = 0; t < wi_.dim1(); ++t) {
    double* f_line = wf_[t];
    inT8* i_line = wi_[t];
    double max_abs = 0.0;
    for (int f = 0; f < dim2; ++f) {
      double abs_val = fabs(f_line[f]);
      if (abs_val > max_abs) max_abs = abs_val;
    }
    double scale = max_abs / MAX_INT8;
    scales_[t] = scale;
    if (scale == 0.0) scale = 1.0;
    for (int f = 0; f < dim2; ++f) {
      i_line[f] = IntCastRounded(f_line[f] / scale);
    }
  }
  wf_.Resize(1, 1, 0.0);
  int_mode_ = true;
}

// Allocates any needed memory for running Backward, and zeroes the deltas,
// thus eliminating any existing momentum.
void WeightMatrix::InitBackward(bool ada_grad) {
  int no = int_mode_ ? wi_.dim1() : wf_.dim1();
  int ni = int_mode_ ? wi_.dim2() : wf_.dim2();
  use_ada_grad_ = ada_grad;
  dw_.Resize(no, ni, 0.0);
  updates_.Resize(no, ni, 0.0);
  wf_t_.Transpose(wf_);
  if (use_ada_grad_) dw_sq_sum_.Resize(no, ni, 0.0);
}

// Flag on mode to indicate that this weightmatrix uses inT8.
const int kInt8Flag = 1;
// Flag on mode to indicate that this weightmatrix uses ada grad.
const int kAdaGradFlag = 4;
// Flag on mode to indicate that this weightmatrix uses double. Set
// independently of kInt8Flag as even in int mode the scales can
// be float or double.
const int kDoubleFlag = 128;

// Writes to the given file. Returns false in case of error.
bool WeightMatrix::Serialize(bool training, TFile* fp) const {
  // For backward compatibility, add kDoubleFlag to mode to indicate the doubles
  // format, without errs, so we can detect and read old format weight matrices.
  uinT8 mode = (int_mode_ ? kInt8Flag : 0) |
               (use_ada_grad_ ? kAdaGradFlag : 0) | kDoubleFlag;
  if (fp->FWrite(&mode, sizeof(mode), 1) != 1) return false;
  if (int_mode_) {
    if (!wi_.Serialize(fp)) return false;
    if (!scales_.Serialize(fp)) return false;
  } else {
    if (!wf_.Serialize(fp)) return false;
    if (training && !updates_.Serialize(fp)) return false;
    if (training && use_ada_grad_ && !dw_sq_sum_.Serialize(fp)) return false;
  }
  return true;
}

// Reads from the given file. Returns false in case of error.
// If swap is true, assumes a big/little-endian swap is needed.
bool WeightMatrix::DeSerialize(bool training, bool swap, TFile* fp) {
  uinT8 mode = 0;
  if (fp->FRead(&mode, sizeof(mode), 1) != 1) return false;
  int_mode_ = (mode & kInt8Flag) != 0;
  use_ada_grad_ = (mode & kAdaGradFlag) != 0;
  if ((mode & kDoubleFlag) == 0) return DeSerializeOld(training, swap, fp);
  if (int_mode_) {
    if (!wi_.DeSerialize(swap, fp)) return false;
    if (!scales_.DeSerialize(swap, fp)) return false;
  } else {
    if (!wf_.DeSerialize(swap, fp)) return false;
    if (training) {
      InitBackward(use_ada_grad_);
      if (!updates_.DeSerialize(swap, fp)) return false;
      if (use_ada_grad_ && !dw_sq_sum_.DeSerialize(swap, fp)) return false;
    }
  }
  return true;
}

// As DeSerialize, but reads an old (float) format WeightMatrix for
// backward compatibility.
bool WeightMatrix::DeSerializeOld(bool training, bool swap, TFile* fp) {
  GENERIC_2D_ARRAY<float> float_array;
  if (int_mode_) {
    if (!wi_.DeSerialize(swap, fp)) return false;
    GenericVector<float> old_scales;
    if (!old_scales.DeSerialize(swap, fp)) return false;
    scales_.init_to_size(old_scales.size(), 0.0);
    for (int i = 0; i < old_scales.size(); ++i) scales_[i] = old_scales[i];
  } else {
    if (!float_array.DeSerialize(swap, fp)) return false;
    FloatToDouble(float_array, &wf_);
  }
  if (training) {
    InitBackward(use_ada_grad_);
    if (!float_array.DeSerialize(swap, fp)) return false;
    FloatToDouble(float_array, &updates_);
    // Errs was only used in int training, which is now dead.
    if (!float_array.DeSerialize(swap, fp)) return false;
  }
  return true;
}

// Computes matrix.vector v = Wu.
// u is of size W.dim2() - 1 and the output v is of size W.dim1().
// u is imagined to have an extra element at the end with value 1, to
// implement the bias, but it doesn't actually have it.
// Asserts that the call matches what we have.
void WeightMatrix::MatrixDotVector(const double* u, double* v) const {
  ASSERT_HOST(!int_mode_);
  MatrixDotVectorInternal(wf_, true, false, u, v);
}

void WeightMatrix::MatrixDotVector(const inT8* u, double* v) const {
  ASSERT_HOST(int_mode_);
  int num_out = wi_.dim1();
  int num_in = wi_.dim2() - 1;
  for (int i = 0; i < num_out; ++i) {
    const inT8* Wi = wi_[i];
    int total = 0;
    if (SIMDDetect::IsSSEAvailable()) {
      total = IntDotProductSSE(u, Wi, num_in);
    } else {
      for (int j = 0; j < num_in; ++j) total += Wi[j] * u[j];
    }
    // Add in the bias and correct for integer values.
    v[i] = (static_cast<double>(total) / MAX_INT8 + Wi[num_in]) * scales_[i];
  }
}

// MatrixDotVector for peep weights, MultiplyAccumulate adds the
// component-wise products of *this[0] and v to inout.
void WeightMatrix::MultiplyAccumulate(const double* v, double* inout) {
  ASSERT_HOST(!int_mode_);
  ASSERT_HOST(wf_.dim1() == 1);
  int n = wf_.dim2();
  const double* u = wf_[0];
  for (int i = 0; i < n; ++i) {
    inout[i] += u[i] * v[i];
  }
}

// Computes vector.matrix v = uW.
// u is of size W.dim1() and the output v is of size W.dim2() - 1.
// The last result is discarded, as v is assumed to have an imaginary
// last value of 1, as with MatrixDotVector.
void WeightMatrix::VectorDotMatrix(const double* u, double* v) const {
  ASSERT_HOST(!int_mode_);
  MatrixDotVectorInternal(wf_t_, false, true, u, v);
}

// Fills dw_[i][j] with the dot product u[i][] . v[j][], using elements from
// u and v. In terms of the neural network, u is the gradients and v is the
// inputs.
// Note that (matching MatrixDotVector) v[last][] is missing, presumed 1.0.
// Runs parallel if requested. Note that u and v must be transposed.
void WeightMatrix::SumOuterTransposed(const TransposedArray& u,
                                      const TransposedArray& v,
                                      bool in_parallel) {
  ASSERT_HOST(!int_mode_);
  int num_outputs = dw_.dim1();
  ASSERT_HOST(u.dim1() == num_outputs);
  ASSERT_HOST(u.dim2() == v.dim2());
  int num_inputs = dw_.dim2() - 1;
  int num_samples = u.dim2();
  // v is missing the last element in dim1.
  ASSERT_HOST(v.dim1() == num_inputs);
#ifdef _OPENMP
#pragma omp parallel for num_threads(4) if (in_parallel)
#endif
  for (int i = 0; i < num_outputs; ++i) {
    double* dwi = dw_[i];
    const double* ui = u[i];
    for (int j = 0; j < num_inputs; ++j) {
      dwi[j] = DotProduct(ui, v[j], num_samples);
    }
    // The last element of v is missing, presumed 1.0f.
    double total = 0.0;
    for (int k = 0; k < num_samples; ++k) total += ui[k];
    dwi[num_inputs] = total;
  }
}

// Updates the weights using the given learning rate and momentum.
// num_samples is the quotient to be used in the adagrad computation iff
// use_ada_grad_ is true.
void WeightMatrix::Update(double learning_rate, double momentum,
                          int num_samples) {
  ASSERT_HOST(!int_mode_);
  if (use_ada_grad_ && num_samples > 0) {
    dw_sq_sum_.SumSquares(dw_);
    dw_.AdaGradScaling(dw_sq_sum_, num_samples);
  }
  dw_ *= learning_rate;
  updates_ += dw_;
  if (momentum > 0.0) wf_ += updates_;
  if (momentum >= 0.0) updates_ *= momentum;
  wf_t_.Transpose(wf_);
}

// Adds the dw_ in other to the dw_ is *this.
void WeightMatrix::AddDeltas(const WeightMatrix& other) {
  ASSERT_HOST(dw_.dim1() == other.dw_.dim1());
  ASSERT_HOST(dw_.dim2() == other.dw_.dim2());
  dw_ += other.dw_;
}

// Sums the products of weight updates in *this and other, splitting into
// positive (same direction) in *same and negative (different direction) in
// *changed.
void WeightMatrix::CountAlternators(const WeightMatrix& other, double* same,
                                    double* changed) const {
  int num_outputs = updates_.dim1();
  int num_inputs = updates_.dim2();
  ASSERT_HOST(num_outputs == other.updates_.dim1());
  ASSERT_HOST(num_inputs == other.updates_.dim2());
  for (int i = 0; i < num_outputs; ++i) {
    const double* this_i = updates_[i];
    const double* other_i = other.updates_[i];
    for (int j = 0; j < num_inputs; ++j) {
      double product = this_i[j] * other_i[j];
      if (product < 0.0)
        *changed -= product;
      else
        *same += product;
    }
  }
}

// Helper computes an integer histogram bucket for a weight and adds it
// to the histogram.
const int kHistogramBuckets = 16;
static void HistogramWeight(double weight, STATS* histogram) {
  int bucket = kHistogramBuckets - 1;
  if (weight != 0.0) {
    double logval = -log(fabs(weight)/log(2));
    bucket = ClipToRange(IntCastRounded(logval), 0, kHistogramBuckets - 1);
  }
  histogram->add(bucket, 1);
}

void WeightMatrix::Debug2D(const char* msg) {
  STATS histogram(0, kHistogramBuckets);
  if (int_mode_) {
    for (int i = 0; i < wi_.dim1(); ++i) {
      for (int j = 0; j < wi_.dim2(); ++j) {
        HistogramWeight(wi_[i][j] * scales_[i], &histogram);
      }
    }
  } else {
    for (int i = 0; i < wf_.dim1(); ++i) {
      for (int j = 0; j < wf_.dim2(); ++j) {
        HistogramWeight(wf_[i][j], &histogram);
      }
    }
  }
  tprintf("%s\n", msg);
  histogram.print();
}

// Computes and returns the dot product of the two n-vectors u and v.
/* static */
double WeightMatrix::DotProduct(const double* u, const double* v, int n) {
  // Note: because the order of addition is different among the 3 DotProduct
  // functions, the results can (and do) vary slightly (although they agree
  // to within about 4e-15). This produces different results when running
  // training, despite all random inputs being precisely equal.
  // To get consistent results, use just one of these DotProduct functions.
  // On a test multi-layer network, serial is 57% slower than sse, and avx
  // is about 8% faster than sse. This suggests that the time is memory
  // bandwidth constrained and could benefit from holding the reused vector
  // in AVX registers.
  if (SIMDDetect::IsAVXAvailable()) return DotProductAVX(u, v, n);
  if (SIMDDetect::IsSSEAvailable()) return DotProductSSE(u, v, n);
  double total = 0.0;
  for (int k = 0; k < n; ++k) total += u[k] * v[k];
  return total;
}

// Utility function converts an array of float to the corresponding array
// of double.
/* static */
void WeightMatrix::FloatToDouble(const GENERIC_2D_ARRAY<float>& wf,
                                 GENERIC_2D_ARRAY<double>* wd) {
  int dim1 = wf.dim1();
  int dim2 = wf.dim2();
  wd->ResizeNoInit(dim1, dim2);
  for (int i = 0; i < dim1; ++i) {
    const float* wfi = wf[i];
    double* wdi = (*wd)[i];
    for (int j = 0; j < dim2; ++j) wdi[j] = static_cast<double>(wfi[j]);
  }
}

// Computes matrix.vector v = Wu.
// u is of size W.dim2() - add_bias_fwd and the output v is of size
// W.dim1() - skip_bias_back.
// If add_bias_fwd, u is imagined to have an extra element at the end with value
// 1, to implement the bias, weight.
// If skip_bias_back, we are actullay performing the backwards product on a
// transposed matrix, so we need to drop the v output corresponding to the last
// element in dim1.
void WeightMatrix::MatrixDotVectorInternal(const GENERIC_2D_ARRAY<double>& w,
                                           bool add_bias_fwd,
                                           bool skip_bias_back, const double* u,
                                           double* v) {
  int num_results = w.dim1() - skip_bias_back;
  int extent = w.dim2() - add_bias_fwd;
  for (int i = 0; i < num_results; ++i) {
    const double* wi = w[i];
    double total = DotProduct(wi, u, extent);
    if (add_bias_fwd) total += wi[extent];  // The bias value.
    v[i] = total;
  }
}

}  // namespace tesseract.
