///////////////////////////////////////////////////////////////////////
// File:        lstmtrainer.h
// Description: Top-level line trainer class for LSTM-based networks.
// Author:      Ray Smith
// Created:     Fri May 03 09:07:06 PST 2013
//
// (C) Copyright 2013, Google Inc.
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

#ifndef TESSERACT_LSTM_LSTMTRAINER_H_
#define TESSERACT_LSTM_LSTMTRAINER_H_

#include "imagedata.h"
#include "lstmrecognizer.h"
#include "rect.h"
#include "tesscallback.h"

namespace tesseract {

class LSTM;
class LSTMTrainer;
class Parallel;
class Reversed;
class Softmax;
class Series;

// Enum for the types of errors that are counted.
enum ErrorTypes {
  ET_RMS,          // RMS activation error.
  ET_DELTA,        // Number of big errors in deltas.
  ET_WORD_RECERR,  // Output text string word recall error.
  ET_CHAR_ERROR,   // Output text string total char error.
  ET_SKIP_RATIO,   // Fraction of samples skipped.
  ET_COUNT         // For array sizing.
};

// Enum for the trainability_ flags.
enum Trainability {
  TRAINABLE,         // Non-zero delta error.
  PERFECT,           // Zero delta error.
  UNENCODABLE,       // Not trainable due to coding/alignment trouble.
  HI_PRECISION_ERR,  // Hi confidence disagreement.
  NOT_BOXED,         // Early in training and has no character boxes.
};

// Enum to define the amount of data to get serialized.
enum SerializeAmount {
  LIGHT,            // Minimal data for remote training.
  NO_BEST_TRAINER,  // Save an empty vector in place of best_trainer_.
  FULL,             // All data including best_trainer_.
};

// Enum to indicate how the sub_trainer_ training went.
enum SubTrainerResult {
  STR_NONE,     // Did nothing as not good enough.
  STR_UPDATED,  // Subtrainer was updated, but didn't replace *this.
  STR_REPLACED  // Subtrainer replaced *this.
};

class LSTMTrainer;
// Function to restore the trainer state from a given checkpoint.
// Returns false on failure.
typedef TessResultCallback2<bool, const GenericVector<char>&, LSTMTrainer*>*
    CheckPointReader;
// Function to save a checkpoint of the current trainer state.
// Returns false on failure. SerializeAmount determines the amount of the
// trainer to serialize, typically used for saving the best state.
typedef TessResultCallback3<bool, SerializeAmount, const LSTMTrainer*,
                            GenericVector<char>*>* CheckPointWriter;
// Function to compute and record error rates on some external test set(s).
// Args are: iteration, mean errors, model, training stage.
// Returns a STRING containing logging information about the tests.
typedef TessResultCallback4<STRING, int, const double*,
                            const GenericVector<char>&, int>* TestCallback;

// Trainer class for LSTM networks. Most of the effort is in creating the
// ideal target outputs from the transcription. A box file is used if it is
// available, otherwise estimates of the char widths from the unicharset are
// used to guide a DP search for the best fit to the transcription.
class LSTMTrainer : public LSTMRecognizer {
 public:
  LSTMTrainer();
  // Callbacks may be null, in which case defaults are used.
  LSTMTrainer(FileReader file_reader, FileWriter file_writer,
              CheckPointReader checkpoint_reader,
              CheckPointWriter checkpoint_writer,
              const char* model_base, const char* checkpoint_name,
              int debug_interval, inT64 max_memory);
  virtual ~LSTMTrainer();

  // Tries to deserialize a trainer from the given file and silently returns
  // false in case of failure.
  bool TryLoadingCheckpoint(const char* filename);

  // Initializes the character set encode/decode mechanism.
  // train_flags control training behavior according to the TrainingFlags
  // enum, including character set encoding.
  // script_dir is required for TF_COMPRESS_UNICHARSET, and, if provided,
  // fully initializes the unicharset from the universal unicharsets.
  // Note: Call before InitNetwork!
  void InitCharSet(const UNICHARSET& unicharset, const STRING& script_dir,
                   int train_flags);
  // Initializes the character set encode/decode mechanism directly from a
  // previously setup UNICHARSET and UnicharCompress.
  // ctc_mode controls how the truth text is mapped to the network targets.
  // Note: Call before InitNetwork!
  void InitCharSet(const UNICHARSET& unicharset,
                   const UnicharCompress& recoder);

  // Initializes the trainer with a network_spec in the network description
  // net_flags control network behavior according to the NetworkFlags enum.
  // There isn't really much difference between them - only where the effects
  // are implemented.
  // For other args see NetworkBuilder::InitNetwork.
  // Note: Be sure to call InitCharSet before InitNetwork!
  bool InitNetwork(const STRING& network_spec, int append_index, int net_flags,
                   float weight_range, float learning_rate, float momentum);
  // Initializes a trainer from a serialized TFNetworkModel proto.
  // Returns the global step of TensorFlow graph or 0 if failed.
  // Building a compatible TF graph: See tfnetwork.proto.
  int InitTensorFlowNetwork(const std::string& tf_proto);
  // Resets all the iteration counters for fine tuning or training a head,
  // where we want the error reporting to reset.
  void InitIterations();

  // Accessors.
  double ActivationError() const {
    return error_rates_[ET_DELTA];
  }
  double CharError() const { return error_rates_[ET_CHAR_ERROR]; }
  const double* error_rates() const {
    return error_rates_;
  }
  double best_error_rate() const {
    return best_error_rate_;
  }
  int best_iteration() const {
    return best_iteration_;
  }
  int learning_iteration() const { return learning_iteration_; }
  int improvement_steps() const { return improvement_steps_; }
  void set_perfect_delay(int delay) { perfect_delay_ = delay; }
  const GenericVector<char>& best_trainer() const { return best_trainer_; }
  // Returns the error that was just calculated by PrepareForBackward.
  double NewSingleError(ErrorTypes type) const {
    return error_buffers_[type][training_iteration() % kRollingBufferSize_];
  }
  // Returns the error that was just calculated by TrainOnLine. Since
  // TrainOnLine rolls the error buffers, this is one further back than
  // NewSingleError.
  double LastSingleError(ErrorTypes type) const {
    return error_buffers_[type]
                         [(training_iteration() + kRollingBufferSize_ - 1) %
                          kRollingBufferSize_];
  }
  const DocumentCache& training_data() const {
    return training_data_;
  }
  DocumentCache* mutable_training_data() { return &training_data_; }

  // If the training sample is usable, grid searches for the optimal
  // dict_ratio/cert_offset, and returns the results in a string of space-
  // separated triplets of ratio,offset=worderr.
  Trainability GridSearchDictParams(
      const ImageData* trainingdata, int iteration, double min_dict_ratio,
      double dict_ratio_step, double max_dict_ratio, double min_cert_offset,
      double cert_offset_step, double max_cert_offset, STRING* results);

  void SetSerializeMode(SerializeAmount serialize_amount) const {
    serialize_amount_ = serialize_amount;
  }

  // Provides output on the distribution of weight values.
  void DebugNetwork();

  // Loads a set of lstmf files that were created using the lstm.train config to
  // tesseract into memory ready for training. Returns false if nothing was
  // loaded.
  bool LoadAllTrainingData(const GenericVector<STRING>& filenames);

  // Keeps track of best and locally worst error rate, using internally computed
  // values. See MaintainCheckpointsSpecific for more detail.
  bool MaintainCheckpoints(TestCallback tester, STRING* log_msg);
  // Keeps track of best and locally worst error_rate (whatever it is) and
  // launches tests using rec_model, when a new min or max is reached.
  // Writes checkpoints using train_model at appropriate times and builds and
  // returns a log message to indicate progress. Returns false if nothing
  // interesting happened.
  bool MaintainCheckpointsSpecific(int iteration,
                                   const GenericVector<char>* train_model,
                                   const GenericVector<char>* rec_model,
                                   TestCallback tester, STRING* log_msg);
  // Builds a string containing a progress message with current error rates.
  void PrepareLogMsg(STRING* log_msg) const;
  // Appends <intro_str> iteration learning_iteration()/training_iteration()/
  // sample_iteration() to the log_msg.
  void LogIterations(const char* intro_str, STRING* log_msg) const;

  // TODO(rays) Add curriculum learning.
  // Returns true and increments the training_stage_ if the error rate has just
  // passed through the given threshold for the first time.
  bool TransitionTrainingStage(float error_threshold);
  // Returns the current training stage.
  int CurrentTrainingStage() const { return training_stage_; }

  // Writes to the given file. Returns false in case of error.
  virtual bool Serialize(TFile* fp) const;
  // Reads from the given file. Returns false in case of error.
  // If swap is true, assumes a big/little-endian swap is needed.
  virtual bool DeSerialize(bool swap, TFile* fp);

  // De-serializes the saved best_trainer_ into sub_trainer_, and adjusts the
  // learning rates (by scaling reduction, or layer specific, according to
  // NF_LAYER_SPECIFIC_LR).
  void StartSubtrainer(STRING* log_msg);
  // While the sub_trainer_ is behind the current training iteration and its
  // training error is at least kSubTrainerMarginFraction better than the
  // current training error, trains the sub_trainer_, and returns STR_UPDATED if
  // it did anything. If it catches up, and has a better error rate than the
  // current best, as well as a margin over the current error rate, then the
  // trainer in *this is replaced with sub_trainer_, and STR_REPLACED is
  // returned. STR_NONE is returned if the subtrainer wasn't good enough to
  // receive any training iterations.
  SubTrainerResult UpdateSubtrainer(STRING* log_msg);
  // Reduces network learning rates, either for everything, or for layers
  // independently, according to NF_LAYER_SPECIFIC_LR.
  void ReduceLearningRates(LSTMTrainer* samples_trainer, STRING* log_msg);
  // Considers reducing the learning rate independently for each layer down by
  // factor(<1), or leaving it the same, by double-training the given number of
  // samples and minimizing the amount of changing of sign of weight updates.
  // Even if it looks like all weights should remain the same, an adjustment
  // will be made to guarantee a different result when reverting to an old best.
  // Returns the number of layer learning rates that were reduced.
  int ReduceLayerLearningRates(double factor, int num_samples,
                               LSTMTrainer* samples_trainer);

  // Converts the string to integer class labels, with appropriate null_char_s
  // in between if not in SimpleTextOutput mode. Returns false on failure.
  bool EncodeString(const STRING& str, GenericVector<int>* labels) const {
    return EncodeString(str, GetUnicharset(), IsRecoding() ? &recoder_ : NULL,
                        SimpleTextOutput(), null_char_, labels);
  }
  // Static version operates on supplied unicharset, encoder, simple_text.
  static bool EncodeString(const STRING& str, const UNICHARSET& unicharset,
                           const UnicharCompress* recoder, bool simple_text,
                           int null_char, GenericVector<int>* labels);

  // Converts the network to int if not already.
  void ConvertToInt() {
    if ((training_flags_ & TF_INT_MODE) == 0) {
      network_->ConvertToInt();
      training_flags_ |= TF_INT_MODE;
    }
  }

  // Performs forward-backward on the given trainingdata.
  // Returns the sample that was used or NULL if the next sample was deemed
  // unusable. samples_trainer could be this or an alternative trainer that
  // holds the training samples.
  const ImageData* TrainOnLine(LSTMTrainer* samples_trainer, bool batch) {
    int sample_index = sample_iteration();
    const ImageData* image =
        samples_trainer->training_data_.GetPageBySerial(sample_index);
    if (image != NULL) {
      Trainability trainable = TrainOnLine(image, batch);
      if (trainable == UNENCODABLE || trainable == NOT_BOXED) {
        return NULL;  // Sample was unusable.
      }
    } else {
      ++sample_iteration_;
    }
    return image;
  }
  Trainability TrainOnLine(const ImageData* trainingdata, bool batch);

  // Prepares the ground truth, runs forward, and prepares the targets.
  // Returns a Trainability enum to indicate the suitability of the sample.
  Trainability PrepareForBackward(const ImageData* trainingdata,
                                  NetworkIO* fwd_outputs, NetworkIO* targets);

  // Writes the trainer to memory, so that the current training state can be
  // restored.
  bool SaveTrainingDump(SerializeAmount serialize_amount,
                        const LSTMTrainer* trainer,
                        GenericVector<char>* data) const;

  // Reads previously saved trainer from memory.
  bool ReadTrainingDump(const GenericVector<char>& data, LSTMTrainer* trainer);
  bool ReadSizedTrainingDump(const char* data, int size);

  // Sets up the data for MaintainCheckpoints from a light ReadTrainingDump.
  void SetupCheckpointInfo();

  // Writes the recognizer to memory, so that it can be used for testing later.
  void SaveRecognitionDump(GenericVector<char>* data) const;

  // Reads and returns a previously saved recognizer from memory.
  static LSTMRecognizer* ReadRecognitionDump(const GenericVector<char>& data);

  // Writes current best model to a file, unless it has already been written.
  bool SaveBestModel(FileWriter writer) const;

  // Returns a suitable filename for a training dump, based on the model_base_,
  // the iteration and the error rates.
  STRING DumpFilename() const;

  // Fills the whole error buffer of the given type with the given value.
  void FillErrorBuffer(double new_error, ErrorTypes type);

 protected:
  // Factored sub-constructor sets up reasonable default values.
  void EmptyConstructor();

  // Sets the unicharset properties using the given script_dir as a source of
  // script unicharsets. If the flag TF_COMPRESS_UNICHARSET is true, also sets
  // up the recoder_ to simplify the unicharset.
  void SetUnicharsetProperties(const STRING& script_dir);

  // Outputs the string and periodically displays the given network inputs
  // as an image in the given window, and the corresponding labels at the
  // corresponding x_starts.
  // Returns false if the truth string is empty.
  bool DebugLSTMTraining(const NetworkIO& inputs,
                         const ImageData& trainingdata,
                         const NetworkIO& fwd_outputs,
                         const GenericVector<int>& truth_labels,
                         const NetworkIO& outputs);
  // Displays the network targets as line a line graph.
  void DisplayTargets(const NetworkIO& targets, const char* window_name,
                      ScrollView** window);

  // Builds a no-compromises target where the first positions should be the
  // truth labels and the rest is padded with the null_char_.
  bool ComputeTextTargets(const NetworkIO& outputs,
                          const GenericVector<int>& truth_labels,
                          NetworkIO* targets);

  // Builds a target using standard CTC. truth_labels should be pre-padded with
  // nulls wherever desired. They don't have to be between all labels.
  // outputs is input-output, as it gets clipped to minimum probability.
  bool ComputeCTCTargets(const GenericVector<int>& truth_labels,
                         NetworkIO* outputs, NetworkIO* targets);

  // Computes network errors, and stores the results in the rolling buffers,
  // along with the supplied text_error.
  // Returns the delta error of the current sample (not running average.)
  double ComputeErrorRates(const NetworkIO& deltas, double char_error,
                           double word_error);

  // Computes the network activation RMS error rate.
  double ComputeRMSError(const NetworkIO& deltas);

  // Computes network activation winner error rate. (Number of values that are
  // in error by >= 0.5 divided by number of time-steps.) More closely related
  // to final character error than RMS, but still directly calculable from
  // just the deltas. Because of the binary nature of the targets, zero winner
  // error is a sufficient but not necessary condition for zero char error.
  double ComputeWinnerError(const NetworkIO& deltas);

  // Computes a very simple bag of chars char error rate.
  double ComputeCharError(const GenericVector<int>& truth_str,
                          const GenericVector<int>& ocr_str);
  // Computes a very simple bag of words word recall error rate.
  // NOTE that this is destructive on both input strings.
  double ComputeWordError(STRING* truth_str, STRING* ocr_str);

  // Updates the error buffer and corresponding mean of the given type with
  // the new_error.
  void UpdateErrorBuffer(double new_error, ErrorTypes type);

  // Rolls error buffers and reports the current means.
  void RollErrorBuffers();

  // Given that error_rate is either a new min or max, updates the best/worst
  // error rates, and record of progress.
  STRING UpdateErrorGraph(int iteration, double error_rate,
                          const GenericVector<char>& model_data,
                          TestCallback tester);

 protected:
  // Alignment display window.
  ScrollView* align_win_;
  // CTC target display window.
  ScrollView* target_win_;
  // CTC output display window.
  ScrollView* ctc_win_;
  // Reconstructed image window.
  ScrollView* recon_win_;
  // How often to display a debug image.
  int debug_interval_;
  // Iteration at which the last checkpoint was dumped.
  int checkpoint_iteration_;
  // Basename of files to save best models to.
  STRING model_base_;
  // Checkpoint filename.
  STRING checkpoint_name_;
  // Training data.
  DocumentCache training_data_;
  // A hack to serialize less data for batch training and record file version.
  mutable SerializeAmount serialize_amount_;
  // Name to use when saving best_trainer_.
  STRING best_model_name_;
  // Number of available training stages.
  int num_training_stages_;
  // Checkpointing callbacks.
  FileReader file_reader_;
  FileWriter file_writer_;
  // TODO(rays) These are pointers, and must be deleted. Switch to unique_ptr
  // when we can commit to c++11.
  CheckPointReader checkpoint_reader_;
  CheckPointWriter checkpoint_writer_;

  // ===Serialized data to ensure that a restart produces the same results.===
  // These members are only serialized when serialize_amount_ != LIGHT.
  // Best error rate so far.
  double best_error_rate_;
  // Snapshot of all error rates at best_iteration_.
  double best_error_rates_[ET_COUNT];
  // Iteration of best_error_rate_.
  int best_iteration_;
  // Worst error rate since best_error_rate_.
  double worst_error_rate_;
  // Snapshot of all error rates at worst_iteration_.
  double worst_error_rates_[ET_COUNT];
  // Iteration of worst_error_rate_.
  int worst_iteration_;
  // Iteration at which the process will be thought stalled.
  int stall_iteration_;
  // Saved recognition models for computing test error for graph points.
  GenericVector<char> best_model_data_;
  GenericVector<char> worst_model_data_;
  // Saved trainer for reverting back to last known best.
  GenericVector<char> best_trainer_;
  // A subsidiary trainer running with a different learning rate until either
  // *this or sub_trainer_ hits a new best.
  LSTMTrainer* sub_trainer_;
  // Error rate at which last best model was dumped.
  float error_rate_of_last_saved_best_;
  // Current stage of training.
  int training_stage_;
  // History of best error rate against iteration. Used for computing the
  // number of steps to each 2% improvement.
  GenericVector<double> best_error_history_;
  GenericVector<int> best_error_iterations_;
  // Number of iterations since the best_error_rate_ was 2% more than it is now.
  int improvement_steps_;
  // Number of iterations that yielded a non-zero delta error and thus provided
  // significant learning. learning_iteration_ <= training_iteration_.
  // learning_iteration_ is used to measure rate of learning progress.
  int learning_iteration_;
  // Saved value of sample_iteration_ before looking for the the next sample.
  int prev_sample_iteration_;
  // How often to include a PERFECT training sample in backprop.
  // A PERFECT training sample is used if the current
  // training_iteration_ > last_perfect_training_iteration_ + perfect_delay_,
  // so with perfect_delay_ == 0, all samples are used, and with
  // perfect_delay_ == 4, at most 1 in 5 samples will be perfect.
  int perfect_delay_;
  // Value of training_iteration_ at which the last PERFECT training sample
  // was used in back prop.
  int last_perfect_training_iteration_;
  // Rolling buffers storing recent training errors are indexed by
  // training_iteration % kRollingBufferSize_.
  static const int kRollingBufferSize_ = 1000;
  GenericVector<double> error_buffers_[ET_COUNT];
  // Rounded mean percent trailing training errors in the buffers.
  double error_rates_[ET_COUNT];    // RMS training error.
};

}  // namespace tesseract.

#endif  // TESSERACT_LSTM_LSTMTRAINER_H_
