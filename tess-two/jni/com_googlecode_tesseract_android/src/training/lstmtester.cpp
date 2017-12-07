///////////////////////////////////////////////////////////////////////
// File:        lstmtester.cpp
// Description: Top-level line evaluation class for LSTM-based networks.
// Author:      Ray Smith
// Created:     Wed Nov 23 11:18:06 PST 2016
//
// (C) Copyright 2016, Google Inc.
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

#include "lstmtester.h"
#include "genericvector.h"

namespace tesseract {

LSTMTester::LSTMTester(inT64 max_memory)
    : test_data_(max_memory), total_pages_(0), async_running_(false) {}

// Loads a set of lstmf files that were created using the lstm.train config to
// tesseract into memory ready for testing. Returns false if nothing was
// loaded. The arg is a filename of a file that lists the filenames.
bool LSTMTester::LoadAllEvalData(const STRING& filenames_file) {
  GenericVector<STRING> filenames;
  if (!LoadFileLinesToStrings(filenames_file, &filenames)) {
    tprintf("Failed to load list of eval filenames from %s\n",
            filenames_file.string());
    return false;
  }
  return LoadAllEvalData(filenames);
}

// Loads a set of lstmf files that were created using the lstm.train config to
// tesseract into memory ready for testing. Returns false if nothing was
// loaded.
bool LSTMTester::LoadAllEvalData(const GenericVector<STRING>& filenames) {
  test_data_.Clear();
  bool result =
      test_data_.LoadDocuments(filenames, "eng", CS_SEQUENTIAL, nullptr);
  total_pages_ = test_data_.TotalPages();
  return result;
}

// Runs an evaluation asynchronously on the stored data and returns a string
// describing the results of the previous test.
STRING LSTMTester::RunEvalAsync(int iteration, const double* training_errors,
                                const GenericVector<char>& model_data,
                                int training_stage) {
  STRING result;
  if (total_pages_ == 0) {
    result.add_str_int("No test data at iteration", iteration);
    return result;
  }
  if (!LockIfNotRunning()) {
    result.add_str_int("Previous test incomplete, skipping test at iteration",
                       iteration);
    return result;
  }
  // Save the args.
  STRING prev_result = test_result_;
  test_result_ = "";
  if (training_errors != nullptr) {
    test_iteration_ = iteration;
    test_training_errors_ = training_errors;
    test_model_data_ = model_data;
    test_training_stage_ = training_stage;
    SVSync::StartThread(&LSTMTester::ThreadFunc, this);
  } else {
    UnlockRunning();
  }
  return prev_result;
}

// Runs an evaluation synchronously on the stored data and returns a string
// describing the results.
STRING LSTMTester::RunEvalSync(int iteration, const double* training_errors,
                               const GenericVector<char>& model_data,
                               int training_stage) {
  LSTMTrainer trainer;
  if (!trainer.ReadTrainingDump(model_data, &trainer)) {
    return "Deserialize failed";
  }
  int eval_iteration = 0;
  double char_error = 0.0;
  double word_error = 0.0;
  int error_count = 0;
  while (error_count < total_pages_) {
    const ImageData* trainingdata = test_data_.GetPageBySerial(eval_iteration);
    trainer.SetIteration(++eval_iteration);
    NetworkIO fwd_outputs, targets;
    if (trainer.PrepareForBackward(trainingdata, &fwd_outputs, &targets) !=
        UNENCODABLE) {
      char_error += trainer.NewSingleError(tesseract::ET_CHAR_ERROR);
      word_error += trainer.NewSingleError(tesseract::ET_WORD_RECERR);
      ++error_count;
    }
  }
  char_error *= 100.0 / total_pages_;
  word_error *= 100.0 / total_pages_;
  STRING result;
  result.add_str_int("At iteration ", iteration);
  result.add_str_int(", stage ", training_stage);
  result.add_str_double(", Eval Char error rate=", char_error);
  result.add_str_double(", Word error rate=", word_error);
  return result;
}

// Static helper thread function for RunEvalAsync, with a specific signature
// required by SVSync::StartThread. Actually a member function pretending to
// be static, its arg is a this pointer that it will cast back to LSTMTester*
// to call RunEvalSync using the stored args that RunEvalAsync saves in *this.
// LockIfNotRunning must have returned true before calling ThreadFunc, and
// it will call UnlockRunning to release the lock after RunEvalSync completes.
/* static */
void* LSTMTester::ThreadFunc(void* lstmtester_void) {
  LSTMTester* lstmtester = reinterpret_cast<LSTMTester*>(lstmtester_void);
  lstmtester->test_result_ = lstmtester->RunEvalSync(
      lstmtester->test_iteration_, lstmtester->test_training_errors_,
      lstmtester->test_model_data_, lstmtester->test_training_stage_);
  lstmtester->UnlockRunning();
  return lstmtester_void;
}

// Returns true if there is currently nothing running, and takes the lock
// if there is nothing running.
bool LSTMTester::LockIfNotRunning() {
  SVAutoLock lock(&running_mutex_);
  if (async_running_) return false;
  async_running_ = true;
  return true;
}

// Releases the running lock.
void LSTMTester::UnlockRunning() {
  SVAutoLock lock(&running_mutex_);
  async_running_ = false;
}

}  // namespace tesseract
