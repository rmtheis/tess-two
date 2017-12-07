///////////////////////////////////////////////////////////////////////
// File:        unicharcompress.h
// Description: Unicode re-encoding using a sequence of smaller numbers in
//              place of a single large code for CJK, similarly for Indic,
//              and dissection of ligatures for other scripts.
// Author:      Ray Smith
// Created:     Wed Mar 04 14:45:01 PST 2015
//
// (C) Copyright 2015, Google Inc.
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

#ifndef TESSERACT_CCUTIL_UNICHARCOMPRESS_H_
#define TESSERACT_CCUTIL_UNICHARCOMPRESS_H_

#include <unordered_map>

#include "serialis.h"
#include "strngs.h"
#include "unicharset.h"

namespace tesseract {

// Trivial class to hold the code for a recoded unichar-id.
class RecodedCharID {
 public:
  // The maximum length of a code.
  static const int kMaxCodeLen = 9;

  RecodedCharID() : self_normalized_(0), length_(0) {
    memset(code_, 0, sizeof(code_));
  }
  void Truncate(int length) { length_ = length; }
  // Sets the code value at the given index in the code.
  void Set(int index, int value) {
    code_[index] = value;
    if (length_ <= index) length_ = index + 1;
  }
  // Shorthand for setting codes of length 3, as all Hangul and Han codes are
  // length 3.
  void Set3(int code0, int code1, int code2) {
    length_ = 3;
    code_[0] = code0;
    code_[1] = code1;
    code_[2] = code2;
  }
  // Accessors
  bool self_normalized() const { return self_normalized_ != 0; }
  void set_self_normalized(bool value) { self_normalized_ = value; }
  int length() const { return length_; }
  int operator()(int index) const { return code_[index]; }

  // Writes to the given file. Returns false in case of error.
  bool Serialize(TFile* fp) const {
    if (fp->FWrite(&self_normalized_, sizeof(self_normalized_), 1) != 1)
      return false;
    if (fp->FWrite(&length_, sizeof(length_), 1) != 1) return false;
    if (fp->FWrite(code_, sizeof(code_[0]), length_) != length_) return false;
    return true;
  }
  // Reads from the given file. Returns false in case of error.
  // If swap is true, assumes a big/little-endian swap is needed.
  bool DeSerialize(bool swap, TFile* fp) {
    if (fp->FRead(&self_normalized_, sizeof(self_normalized_), 1) != 1)
      return false;
    if (fp->FRead(&length_, sizeof(length_), 1) != 1) return false;
    if (swap) ReverseN(&length_, sizeof(length_));
    if (fp->FRead(code_, sizeof(code_[0]), length_) != length_) return false;
    if (swap) {
      for (int i = 0; i < length_; ++i) {
        ReverseN(&code_[i], sizeof(code_[i]));
      }
    }
    return true;
  }
  bool operator==(const RecodedCharID& other) const {
    if (length_ != other.length_) return false;
    for (int i = 0; i < length_; ++i) {
      if (code_[i] != other.code_[i]) return false;
    }
    return true;
  }
  // Hash functor for RecodedCharID.
  struct RecodedCharIDHash {
    size_t operator()(const RecodedCharID& code) const {
      size_t result = 0;
      for (int i = 0; i < code.length_; ++i) {
        result ^= code(i) << (7 * i);
      }
      return result;
    }
  };

 private:
  // True if this code is self-normalizing, ie is the master entry for indices
  // that map to the same code. Has boolean value, but inT8 for serialization.
  inT8 self_normalized_;
  // The number of elements in use in code_;
  inT32 length_;
  // The re-encoded form of the unichar-id to which this RecodedCharID relates.
  inT32 code_[kMaxCodeLen];
};

// Class holds a "compression" of a unicharset to simplify the learning problem
// for a neural-network-based classifier.
// Objectives:
// 1 (CJK): Ids of a unicharset with a large number of classes are expressed as
//          a sequence of 3 codes with much fewer values.
//          This is achieved using the Jamo coding for Hangul and the Unicode
//          Radical-Stroke-index for Han.
// 2 (Indic): Instead of thousands of codes with one for each grapheme, re-code
//            as the unicode sequence (but coded in a more compact space).
// 3 (the rest): Eliminate multi-path problems with ligatures and fold confusing
//               and not significantly distinct shapes (quotes) togther, ie
//               represent the fi ligature as the f-i pair, and fold u+2019 and
//               friends all onto ascii single '
// 4 The null character and mapping to target activations:
//    To save horizontal coding space, the compressed codes are generally mapped
//    to target network activations without intervening null characters, BUT
//    in the case of ligatures, such as ff, null characters have to be included
//    so existence of repeated codes is detected at codebook-building time, and
//    null characters are embedded directly into the codes, so the rest of the
//    system doesn't need to worry about the problem (much). There is still an
//    effect on the range of ways in which the target activations can be
//    generated.
//
// The computed code values are compact (no unused values), and, for CJK,
// unique (each code position uses a disjoint set of values from each other code
// position). For non-CJK, the same code value CAN be used in multiple
// positions, eg the ff ligature is converted to <f> <nullchar> <f>, where <f>
// is the same code as is used for the single f.
// NOTE that an intended consequence of using the normalized text from the
// unicharset is that the fancy quotes all map to a single code, so round-trip
// conversion doesn't work for all unichar-ids.
class UnicharCompress {
 public:
  UnicharCompress();
  UnicharCompress(const UnicharCompress& src);
  ~UnicharCompress();
  UnicharCompress& operator=(const UnicharCompress& src);

  // The 1st Hangul unicode.
  static const int kFirstHangul = 0xac00;
  // The number of Hangul unicodes.
  static const int kNumHangul = 11172;
  // The number of Jamos for each of the 3 parts of a Hangul character, being
  // the Leading consonant, Vowel and Trailing consonant.
  static const int kLCount = 19;
  static const int kVCount = 21;
  static const int kTCount = 28;

  // Computes the encoding for the given unicharset. It is a requirement that
  // the file training/langdata/radical-stroke.txt have been read into the
  // input string radical_stroke_table.
  // Returns false if the encoding cannot be constructed.
  bool ComputeEncoding(const UNICHARSET& unicharset, int null_id,
                       STRING* radical_stroke_table);
  // Sets up an encoder that doesn't change the unichars at all, so it just
  // passes them through unchanged.
  void SetupPassThrough(const UNICHARSET& unicharset);
  // Sets up an encoder directly using the given encoding vector, which maps
  // unichar_ids to the given codes.
  void SetupDirect(const GenericVector<RecodedCharID>& codes);

  // Returns the number of different values that can be used in a code, ie
  // 1 + the maximum value that will ever be used by an RecodedCharID code in
  // any position in its array.
  int code_range() const { return code_range_; }

  // Encodes a single unichar_id. Returns the length of the code, (or zero if
  // invalid input), and the encoding itself in code.
  int EncodeUnichar(int unichar_id, RecodedCharID* code) const;
  // Decodes code, returning the original unichar-id, or
  // INVALID_UNICHAR_ID if the input is invalid. Note that this is not a perfect
  // inverse of EncodeUnichar, since the unichar-id of U+2019 (curly single
  // quote), for example, will have the same encoding as the unichar-id of
  // U+0027 (ascii '). The foldings are obtained from the input unicharset,
  // which in turn obtains them from NormalizeUTF8String in normstrngs.cpp,
  // and include NFKC normalization plus others like quote and dash folding.
  int DecodeUnichar(const RecodedCharID& code) const;
  // Returns true if the given code is a valid start or single code.
  bool IsValidFirstCode(int code) const { return is_valid_start_[code]; }
  // Returns a list of valid non-final next codes for a given prefix code,
  // which may be empty.
  const GenericVector<int>* GetNextCodes(const RecodedCharID& code) const {
    auto it = next_codes_.find(code);
    return it == next_codes_.end() ? NULL : it->second;
  }
  // Returns a list of valid final codes for a given prefix code, which may
  // be empty.
  const GenericVector<int>* GetFinalCodes(const RecodedCharID& code) const {
    auto it = final_codes_.find(code);
    return it == final_codes_.end() ? NULL : it->second;
  }

  // Writes to the given file. Returns false in case of error.
  bool Serialize(TFile* fp) const;
  // Reads from the given file. Returns false in case of error.
  // If swap is true, assumes a big/little-endian swap is needed.
  bool DeSerialize(bool swap, TFile* fp);

  // Returns a STRING containing a text file that describes the encoding thus:
  // <index>[,<index>]*<tab><UTF8-str><newline>
  // In words, a comma-separated list of one or more indices, followed by a tab
  // and the UTF-8 string that the code represents per line. Most simple scripts
  // will encode a single index to a UTF8-string, but Chinese, Japanese, Korean
  // and the Indic scripts will contain a many-to-many mapping.
  // See the class comment above for details.
  STRING GetEncodingAsString(const UNICHARSET& unicharset) const;

  // Helper decomposes a Hangul unicode to 3 parts, leading, vowel, trailing.
  // Note that the returned values are 0-based indices, NOT unicode Jamo.
  // Returns false if the input is not in the Hangul unicode range.
  static bool DecomposeHangul(int unicode, int* leading, int* vowel,
                              int* trailing);

 private:
  // Renumbers codes to eliminate unused values.
  void DefragmentCodeValues(int encoded_null);
  // Computes the value of code_range_ from the encoder_.
  void ComputeCodeRange();
  // Initializes the decoding hash_map from the encoder_ array.
  void SetupDecoder();
  // Frees allocated memory.
  void Cleanup();

  // The encoder that maps a unichar-id to a sequence of small codes.
  // encoder_ is the only part that is serialized. The rest is computed on load.
  GenericVector<RecodedCharID> encoder_;
  // Decoder converts the output of encoder back to a unichar-id.
  std::unordered_map<RecodedCharID, int, RecodedCharID::RecodedCharIDHash>
      decoder_;
  // True if the index is a valid single or start code.
  GenericVector<bool> is_valid_start_;
  // Maps a prefix code to a list of valid next codes.
  // The map owns the vectors.
  std::unordered_map<RecodedCharID, GenericVectorEqEq<int>*,
                     RecodedCharID::RecodedCharIDHash>
      next_codes_;
  // Maps a prefix code to a list of valid final codes.
  // The map owns the vectors.
  std::unordered_map<RecodedCharID, GenericVectorEqEq<int>*,
                     RecodedCharID::RecodedCharIDHash>
      final_codes_;
  // Max of any value in encoder_ + 1.
  int code_range_;
};

}  // namespace tesseract.

#endif  // TESSERACT_CCUTIL_UNICHARCOMPRESS_H_
