///////////////////////////////////////////////////////////////////////
// File:        plumbing.cpp
// Description: Base class for networks that organize other networks
//              eg series or parallel.
// Author:      Ray Smith
// Created:     Mon May 12 08:17:34 PST 2014
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

#include "plumbing.h"

namespace tesseract {

// ni_ and no_ will be set by AddToStack.
Plumbing::Plumbing(const STRING& name)
  : Network(NT_PARALLEL, name, 0, 0) {
}

Plumbing::~Plumbing() {
}

// Suspends/Enables training by setting the training_ flag. Serialize and
// DeSerialize only operate on the run-time data if state is false.
void Plumbing::SetEnableTraining(TrainingState state) {
  Network::SetEnableTraining(state);
  for (int i = 0; i < stack_.size(); ++i)
    stack_[i]->SetEnableTraining(state);
}

// Sets flags that control the action of the network. See NetworkFlags enum
// for bit values.
void Plumbing::SetNetworkFlags(uinT32 flags) {
  Network::SetNetworkFlags(flags);
  for (int i = 0; i < stack_.size(); ++i)
    stack_[i]->SetNetworkFlags(flags);
}

// Sets up the network for training. Initializes weights using weights of
// scale `range` picked according to the random number generator `randomizer`.
// Note that randomizer is a borrowed pointer that should outlive the network
// and should not be deleted by any of the networks.
// Returns the number of weights initialized.
int Plumbing::InitWeights(float range, TRand* randomizer) {
  num_weights_ = 0;
  for (int i = 0; i < stack_.size(); ++i)
    num_weights_ += stack_[i]->InitWeights(range, randomizer);
  return num_weights_;
}

// Converts a float network to an int network.
void Plumbing::ConvertToInt() {
  for (int i = 0; i < stack_.size(); ++i)
    stack_[i]->ConvertToInt();
}

// Provides a pointer to a TRand for any networks that care to use it.
// Note that randomizer is a borrowed pointer that should outlive the network
// and should not be deleted by any of the networks.
void Plumbing::SetRandomizer(TRand* randomizer) {
  for (int i = 0; i < stack_.size(); ++i)
    stack_[i]->SetRandomizer(randomizer);
}

// Adds the given network to the stack.
void Plumbing::AddToStack(Network* network) {
  if (stack_.empty()) {
    ni_ = network->NumInputs();
    no_ = network->NumOutputs();
  } else if (type_ == NT_SERIES) {
    // ni is input of first, no output of last, others match output to input.
    ASSERT_HOST(no_ == network->NumInputs());
    no_ = network->NumOutputs();
  } else {
    // All parallel types. Output is sum of outputs, inputs all match.
    ASSERT_HOST(ni_ == network->NumInputs());
    no_ += network->NumOutputs();
  }
  stack_.push_back(network);
}

// Sets needs_to_backprop_ to needs_backprop and calls on sub-network
// according to needs_backprop || any weights in this network.
bool Plumbing::SetupNeedsBackprop(bool needs_backprop) {
  if (IsTraining()) {
    needs_to_backprop_ = needs_backprop;
    bool retval = needs_backprop;
    for (int i = 0; i < stack_.size(); ++i) {
      if (stack_[i]->SetupNeedsBackprop(needs_backprop)) retval = true;
    }
    return retval;
  }
  // Frozen networks don't do backprop.
  needs_to_backprop_ = false;
  return false;
}

// Returns an integer reduction factor that the network applies to the
// time sequence. Assumes that any 2-d is already eliminated. Used for
// scaling bounding boxes of truth data.
// WARNING: if GlobalMinimax is used to vary the scale, this will return
// the last used scale factor. Call it before any forward, and it will return
// the minimum scale factor of the paths through the GlobalMinimax.
int Plumbing::XScaleFactor() const {
  return stack_[0]->XScaleFactor();
}

// Provides the (minimum) x scale factor to the network (of interest only to
// input units) so they can determine how to scale bounding boxes.
void Plumbing::CacheXScaleFactor(int factor) {
  for (int i = 0; i < stack_.size(); ++i) {
    stack_[i]->CacheXScaleFactor(factor);
  }
}

// Provides debug output on the weights.
void Plumbing::DebugWeights() {
  for (int i = 0; i < stack_.size(); ++i)
    stack_[i]->DebugWeights();
}

// Returns a set of strings representing the layer-ids of all layers below.
void Plumbing::EnumerateLayers(const STRING* prefix,
                               GenericVector<STRING>* layers) const {
  for (int i = 0; i < stack_.size(); ++i) {
    STRING layer_name;
    if (prefix) layer_name = *prefix;
    layer_name.add_str_int(":", i);
    if (stack_[i]->IsPlumbingType()) {
      Plumbing* plumbing = reinterpret_cast<Plumbing*>(stack_[i]);
      plumbing->EnumerateLayers(&layer_name, layers);
    } else {
      layers->push_back(layer_name);
    }
  }
}

// Returns a pointer to the network layer corresponding to the given id.
Network* Plumbing::GetLayer(const char* id) const {
  char* next_id;
  int index = strtol(id, &next_id, 10);
  if (index < 0 || index >= stack_.size()) return NULL;
  if (stack_[index]->IsPlumbingType()) {
    Plumbing* plumbing = reinterpret_cast<Plumbing*>(stack_[index]);
    ASSERT_HOST(*next_id == ':');
    return plumbing->GetLayer(next_id + 1);
  }
  return stack_[index];
}

// Returns a pointer to the learning rate for the given layer id.
float* Plumbing::LayerLearningRatePtr(const char* id) const {
  char* next_id;
  int index = strtol(id, &next_id, 10);
  if (index < 0 || index >= stack_.size()) return NULL;
  if (stack_[index]->IsPlumbingType()) {
    Plumbing* plumbing = reinterpret_cast<Plumbing*>(stack_[index]);
    ASSERT_HOST(*next_id == ':');
    return plumbing->LayerLearningRatePtr(next_id + 1);
  }
  if (index < 0 || index >= learning_rates_.size()) return NULL;
  return &learning_rates_[index];
}

// Writes to the given file. Returns false in case of error.
bool Plumbing::Serialize(TFile* fp) const {
  if (!Network::Serialize(fp)) return false;
  inT32 size = stack_.size();
  // Can't use PointerVector::Serialize here as we need a special DeSerialize.
  if (fp->FWrite(&size, sizeof(size), 1) != 1) return false;
  for (int i = 0; i < size; ++i)
    if (!stack_[i]->Serialize(fp)) return false;
  if ((network_flags_ & NF_LAYER_SPECIFIC_LR) &&
      !learning_rates_.Serialize(fp)) {
    return false;
  }
  return true;
}

// Reads from the given file. Returns false in case of error.
// If swap is true, assumes a big/little-endian swap is needed.
bool Plumbing::DeSerialize(bool swap, TFile* fp) {
  stack_.truncate(0);
  no_ = 0;  // We will be modifying this as we AddToStack.
  inT32 size;
  if (fp->FRead(&size, sizeof(size), 1) != 1) return false;
  for (int i = 0; i < size; ++i) {
    Network* network = CreateFromFile(swap, fp);
    if (network == NULL) return false;
    AddToStack(network);
  }
  if ((network_flags_ & NF_LAYER_SPECIFIC_LR) &&
      !learning_rates_.DeSerialize(swap, fp)) {
    return false;
  }
  return true;
}

// Updates the weights using the given learning rate and momentum.
// num_samples is the quotient to be used in the adagrad computation iff
// use_ada_grad_ is true.
void Plumbing::Update(float learning_rate, float momentum, int num_samples) {
  for (int i = 0; i < stack_.size(); ++i) {
    if (network_flags_ & NF_LAYER_SPECIFIC_LR) {
      if (i < learning_rates_.size())
        learning_rate = learning_rates_[i];
      else
        learning_rates_.push_back(learning_rate);
    }
    if (stack_[i]->IsTraining()) {
      stack_[i]->Update(learning_rate, momentum, num_samples);
    }
  }
}

// Sums the products of weight updates in *this and other, splitting into
// positive (same direction) in *same and negative (different direction) in
// *changed.
void Plumbing::CountAlternators(const Network& other, double* same,
                                double* changed) const {
  ASSERT_HOST(other.type() == type_);
  const Plumbing* plumbing = reinterpret_cast<const Plumbing*>(&other);
  ASSERT_HOST(plumbing->stack_.size() == stack_.size());
  for (int i = 0; i < stack_.size(); ++i)
    stack_[i]->CountAlternators(*plumbing->stack_[i], same, changed);
}

}  // namespace tesseract.

