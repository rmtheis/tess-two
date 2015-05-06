///////////////////////////////////////////////////////////////////////
// File:        paramsd.cpp
// Description: Tesseract parameter editor
// Author:      Joern Wanke
// Created:     Wed Jul 18 10:05:01 PDT 2007
//
// (C) Copyright 2007, Google Inc.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
///////////////////////////////////////////////////////////////////////
//
// Tesseract parameter editor is used to edit all the parameters used
// within tesseract from the ui.
#ifndef GRAPHICS_DISABLED
#ifndef VARABLED_H
#define VARABLED_H

#include "elst.h"
#include "scrollview.h"
#include "params.h"
#include "tesseractclass.h"

class SVMenuNode;

// A list of all possible parameter types used.
enum ParamType {
  VT_INTEGER,
  VT_BOOLEAN,
  VT_STRING,
  VT_DOUBLE
};

// A rather hackish helper structure which can take any kind of parameter input
// (defined by ParamType) and do a couple of common operations on them, like
// comparisond or getting its value. It is used in the context of the
// ParamsEditor as a bridge from the internal tesseract parameters to the
// ones displayed by the ScrollView server.
class ParamContent : public ELIST_LINK {
 public:
  // Compare two VC objects by their name.
  static int Compare(const void* v1, const void* v2);

  // Gets a VC object identified by its ID.
  static ParamContent* GetParamContentById(int id);

  // Constructors for the various ParamTypes.
  ParamContent() {
  }
  explicit ParamContent(tesseract::StringParam* it);
  explicit ParamContent(tesseract::IntParam* it);
  explicit ParamContent(tesseract::BoolParam* it);
  explicit ParamContent(tesseract::DoubleParam* it);


  // Getters and Setters.
  void SetValue(const char* val);
  STRING GetValue() const;
  const char* GetName() const;
  const char* GetDescription() const;

  int GetId() { return my_id_; }
  bool HasChanged() { return changed_; }

 private:
  // The unique ID of this VC object.
  int my_id_;
  // Whether the parameter was changed_ and thus needs to be rewritten.
  bool changed_;
  // The actual ParamType of this VC object.
  ParamType param_type_;

  tesseract::StringParam* sIt;
  tesseract::IntParam* iIt;
  tesseract::BoolParam* bIt;
  tesseract::DoubleParam* dIt;
};

ELISTIZEH(ParamContent)

// The parameters editor enables the user to edit all the parameters used within
// tesseract. It can be invoked on its own, but is supposed to be invoked by
// the program editor.
class ParamsEditor : public SVEventHandler {
 public:
  // Integrate the parameters editor as popupmenu into the existing scrollview
  // window (usually the pg editor). If sv == null, create a new empty
  // empty window and attach the parameter editor to that window (ugly).
  explicit ParamsEditor(tesseract::Tesseract*, ScrollView* sv = NULL);

  // Event listener. Waits for SVET_POPUP events and processes them.
  void Notify(const SVEvent* sve);

 private:
  // Gets the up to the first 3 prefixes from s (split by _).
  // For example, tesseract_foo_bar will be split into tesseract,foo and bar.
  void GetPrefixes(const char* s, STRING* level_one,
                   STRING* level_two, STRING* level_three);

  // Gets the first n words (split by _) and puts them in t.
  // For example, tesseract_foo_bar with N=2 will yield tesseract_foo_.
  void GetFirstWords(const char *s,  // source string
                     int n,          // number of words
                     char *t);       // target string

  // Find all editable parameters used within tesseract and create a
  // SVMenuNode tree from it.
  SVMenuNode *BuildListOfAllLeaves(tesseract::Tesseract *tess);

  // Write all (changed_) parameters to a config file.
  void WriteParams(char* filename, bool changes_only);

  ScrollView* sv_window_;
};

#endif
#endif
