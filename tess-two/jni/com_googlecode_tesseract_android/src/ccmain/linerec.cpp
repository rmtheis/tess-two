///////////////////////////////////////////////////////////////////////
// File:        linerec.cpp
// Description: Top-level line-based recognition module for Tesseract.
// Author:      Ray Smith
// Created:     Thu May 02 09:47:06 PST 2013
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

#include "tesseractclass.h"

#include "allheaders.h"
#include "boxread.h"
#include "imagedata.h"
#ifndef ANDROID_BUILD
#include "lstmrecognizer.h"
#include "recodebeam.h"
#endif
#include "ndminx.h"
#include "pageres.h"
#include "tprintf.h"

namespace tesseract {

// Arbitarary penalty for non-dictionary words.
// TODO(rays) How to learn this?
const float kNonDictionaryPenalty = 5.0f;
// Scale factor to make certainty more comparable to Tesseract.
const float kCertaintyScale = 7.0f;
// Worst acceptable certainty for a dictionary word.
const float kWorstDictCertainty = -25.0f;

// Generates training data for training a line recognizer, eg LSTM.
// Breaks the page into lines, according to the boxes, and writes them to a
// serialized DocumentData based on output_basename.
void Tesseract::TrainLineRecognizer(const STRING& input_imagename,
                                    const STRING& output_basename,
                                    BLOCK_LIST *block_list) {
  STRING lstmf_name = output_basename + ".lstmf";
  DocumentData images(lstmf_name);
  if (applybox_page > 0) {
    // Load existing document for the previous pages.
    if (!images.LoadDocument(lstmf_name.string(), "eng", 0, 0, NULL)) {
      tprintf("Failed to read training data from %s!\n", lstmf_name.string());
      return;
    }
  }
  GenericVector<TBOX> boxes;
  GenericVector<STRING> texts;
  // Get the boxes for this page, if there are any.
  if (!ReadAllBoxes(applybox_page, false, input_imagename, &boxes, &texts, NULL,
                    NULL) ||
      boxes.empty()) {
    tprintf("Failed to read boxes from %s\n", input_imagename.string());
    return;
  }
  TrainFromBoxes(boxes, texts, block_list, &images);
  images.Shuffle();
  if (!images.SaveDocument(lstmf_name.string(), NULL)) {
    tprintf("Failed to write training data to %s!\n", lstmf_name.string());
  }
}

// Generates training data for training a line recognizer, eg LSTM.
// Breaks the boxes into lines, normalizes them, converts to ImageData and
// appends them to the given training_data.
void Tesseract::TrainFromBoxes(const GenericVector<TBOX>& boxes,
                               const GenericVector<STRING>& texts,
                               BLOCK_LIST *block_list,
                               DocumentData* training_data) {
  int box_count = boxes.size();
  // Process all the text lines in this page, as defined by the boxes.
  int end_box = 0;
  // Don't let \t, which marks newlines in the box file, get into the line
  // content, as that makes the line unusable in training.
  while (end_box < texts.size() && texts[end_box] == "\t") ++end_box;
  for (int start_box = end_box; start_box < box_count; start_box = end_box) {
    // Find the textline of boxes starting at start and their bounding box.
    TBOX line_box = boxes[start_box];
    STRING line_str = texts[start_box];
    for (end_box = start_box + 1; end_box < box_count && texts[end_box] != "\t";
         ++end_box) {
      line_box += boxes[end_box];
      line_str += texts[end_box];
    }
    // Find the most overlapping block.
    BLOCK* best_block = NULL;
    int best_overlap = 0;
    BLOCK_IT b_it(block_list);
    for (b_it.mark_cycle_pt(); !b_it.cycled_list(); b_it.forward()) {
      BLOCK* block = b_it.data();
      if (block->poly_block() != NULL && !block->poly_block()->IsText())
        continue;  // Not a text block.
      TBOX block_box = block->bounding_box();
      block_box.rotate(block->re_rotation());
      if (block_box.major_overlap(line_box)) {
        TBOX overlap_box = line_box.intersection(block_box);
        if (overlap_box.area() > best_overlap) {
          best_overlap = overlap_box.area();
          best_block = block;
        }
      }
    }
    ImageData* imagedata = NULL;
    if (best_block == NULL) {
      tprintf("No block overlapping textline: %s\n", line_str.string());
    } else {
      imagedata = GetLineData(line_box, boxes, texts, start_box, end_box,
                              *best_block);
    }
    if (imagedata != NULL)
      training_data->AddPageToDocument(imagedata);
    // Don't let \t, which marks newlines in the box file, get into the line
    // content, as that makes the line unusable in training.
    while (end_box < texts.size() && texts[end_box] == "\t") ++end_box;
  }
}

// Returns an Imagedata containing the image of the given box,
// and ground truth boxes/truth text if available in the input.
// The image is not normalized in any way.
ImageData* Tesseract::GetLineData(const TBOX& line_box,
                                  const GenericVector<TBOX>& boxes,
                                  const GenericVector<STRING>& texts,
                                  int start_box, int end_box,
                                  const BLOCK& block) {
  TBOX revised_box;
  ImageData* image_data = GetRectImage(line_box, block, kImagePadding,
                                       &revised_box);
  if (image_data == NULL) return NULL;
  image_data->set_page_number(applybox_page);
  // Copy the boxes and shift them so they are relative to the image.
  FCOORD block_rotation(block.re_rotation().x(), -block.re_rotation().y());
  ICOORD shift = -revised_box.botleft();
  GenericVector<TBOX> line_boxes;
  GenericVector<STRING> line_texts;
  for (int b = start_box; b < end_box; ++b) {
    TBOX box = boxes[b];
    box.rotate(block_rotation);
    box.move(shift);
    line_boxes.push_back(box);
    line_texts.push_back(texts[b]);
  }
  GenericVector<int> page_numbers;
  page_numbers.init_to_size(line_boxes.size(), applybox_page);
  image_data->AddBoxes(line_boxes, line_texts, page_numbers);
  return image_data;
}

// Helper gets the image of a rectangle, using the block.re_rotation() if
// needed to get to the image, and rotating the result back to horizontal
// layout. (CJK characters will be on their left sides) The vertical text flag
// is set in the returned ImageData if the text was originally vertical, which
// can be used to invoke a different CJK recognition engine. The revised_box
// is also returned to enable calculation of output bounding boxes.
ImageData* Tesseract::GetRectImage(const TBOX& box, const BLOCK& block,
                                   int padding, TBOX* revised_box) const {
  TBOX wbox = box;
  wbox.pad(padding, padding);
  *revised_box = wbox;
  // Number of clockwise 90 degree rotations needed to get back to tesseract
  // coords from the clipped image.
  int num_rotations = 0;
  if (block.re_rotation().y() > 0.0f)
    num_rotations = 1;
  else if (block.re_rotation().x() < 0.0f)
    num_rotations = 2;
  else if (block.re_rotation().y() < 0.0f)
    num_rotations = 3;
  // Handle two cases automatically: 1 the box came from the block, 2 the box
  // came from a box file, and refers to the image, which the block may not.
  if (block.bounding_box().major_overlap(*revised_box))
    revised_box->rotate(block.re_rotation());
  // Now revised_box always refers to the image.
  // BestPix is never colormapped, but may be of any depth.
  Pix* pix = BestPix();
  int width = pixGetWidth(pix);
  int height = pixGetHeight(pix);
  TBOX image_box(0, 0, width, height);
  // Clip to image bounds;
  *revised_box &= image_box;
  if (revised_box->null_box()) return NULL;
  Box* clip_box = boxCreate(revised_box->left(), height - revised_box->top(),
                            revised_box->width(), revised_box->height());
  Pix* box_pix = pixClipRectangle(pix, clip_box, NULL);
  if (box_pix == NULL) return NULL;
  boxDestroy(&clip_box);
  if (num_rotations > 0) {
    Pix* rot_pix = pixRotateOrth(box_pix, num_rotations);
    pixDestroy(&box_pix);
    box_pix = rot_pix;
  }
  // Convert sub-8-bit images to 8 bit.
  int depth = pixGetDepth(box_pix);
  if (depth < 8) {
    Pix* grey;
    grey = pixConvertTo8(box_pix, false);
    pixDestroy(&box_pix);
    box_pix = grey;
  }
  bool vertical_text = false;
  if (num_rotations > 0) {
    // Rotated the clipped revised box back to internal coordinates.
    FCOORD rotation(block.re_rotation().x(), -block.re_rotation().y());
    revised_box->rotate(rotation);
    if (num_rotations != 2)
      vertical_text = true;
  }
  return new ImageData(vertical_text, box_pix);
}

#ifndef ANDROID_BUILD
// Recognizes a word or group of words, converting to WERD_RES in *words.
// Analogous to classify_word_pass1, but can handle a group of words as well.
void Tesseract::LSTMRecognizeWord(const BLOCK& block, ROW *row, WERD_RES *word,
                                  PointerVector<WERD_RES>* words) {
  TBOX word_box = word->word->bounding_box();
  // Get the word image - no frills.
  if (tessedit_pageseg_mode == PSM_SINGLE_WORD ||
      tessedit_pageseg_mode == PSM_RAW_LINE) {
    // In single word mode, use the whole image without any other row/word
    // interpretation.
    word_box = TBOX(0, 0, ImageWidth(), ImageHeight());
  } else {
    float baseline = row->base_line((word_box.left() + word_box.right()) / 2);
    if (baseline + row->descenders() < word_box.bottom())
      word_box.set_bottom(baseline + row->descenders());
    if (baseline + row->x_height() + row->ascenders() > word_box.top())
      word_box.set_top(baseline + row->x_height() + row->ascenders());
  }
  ImageData* im_data = GetRectImage(word_box, block, kImagePadding, &word_box);
  if (im_data == NULL) return;
  lstm_recognizer_->RecognizeLine(*im_data, true, classify_debug_level > 0,
                                  kWorstDictCertainty / kCertaintyScale,
                                  lstm_use_matrix, &unicharset, word_box, 2.0,
                                  false, words);
  delete im_data;
  SearchWords(words);
}

// Apply segmentation search to the given set of words, within the constraints
// of the existing ratings matrix. If there is already a best_choice on a word
// leaves it untouched and just sets the done/accepted etc flags.
void Tesseract::SearchWords(PointerVector<WERD_RES>* words) {
  // Run the segmentation search on the network outputs and make a BoxWord
  // for each of the output words.
  // If we drop a word as junk, then there is always a space in front of the
  // next.
  const Dict* stopper_dict = lstm_recognizer_->GetDict();
  if (stopper_dict == nullptr) stopper_dict = &getDict();
  bool any_nonspace_delimited = false;
  for (int w = 0; w < words->size(); ++w) {
    WERD_RES* word = (*words)[w];
    if (word->best_choice != nullptr &&
        word->best_choice->ContainsAnyNonSpaceDelimited()) {
      any_nonspace_delimited = true;
      break;
    }
  }
  for (int w = 0; w < words->size(); ++w) {
    WERD_RES* word = (*words)[w];
    if (word->best_choice == NULL) {
      // If we are using the beam search, the unicharset had better match!
      word->SetupWordScript(unicharset);
      WordSearch(word);
    } else if (word->best_choice->unicharset() == &unicharset &&
               !lstm_recognizer_->IsRecoding()) {
      // We set up the word without using the dictionary, so set the permuter
      // now, but we can only do it because the unicharsets match.
      word->best_choice->set_permuter(
          getDict().valid_word(*word->best_choice, true));
    }
    if (word->best_choice == NULL) {
      // It is a dud.
      word->SetupFake(lstm_recognizer_->GetUnicharset());
    } else {
      // Set the best state.
      for (int i = 0; i < word->best_choice->length(); ++i) {
        int length = word->best_choice->state(i);
        word->best_state.push_back(length);
      }
      word->reject_map.initialise(word->best_choice->length());
      word->tess_failed = false;
      word->tess_accepted = true;
      word->tess_would_adapt = false;
      word->done = true;
      word->tesseract = this;
      float word_certainty = MIN(word->space_certainty,
                                 word->best_choice->certainty());
      word_certainty *= kCertaintyScale;
      // Arbitrary ding factor for non-dictionary words.
      if (!lstm_recognizer_->IsRecoding() &&
          !Dict::valid_word_permuter(word->best_choice->permuter(), true))
        word_certainty -= kNonDictionaryPenalty;
      if (getDict().stopper_debug_level >= 1) {
        tprintf("Best choice certainty=%g, space=%g, scaled=%g, final=%g\n",
                word->best_choice->certainty(), word->space_certainty,
                MIN(word->space_certainty, word->best_choice->certainty()) *
                    kCertaintyScale,
                word_certainty);
        word->best_choice->print();
      }
      word->best_choice->set_certainty(word_certainty);
      // Discard words that are impossibly bad, but allow a bit more for
      // dictionary words, and keep bad words in non-space-delimited langs.
      if (word_certainty >= RecodeBeamSearch::kMinCertainty ||
          any_nonspace_delimited ||
          (word_certainty >= kWorstDictCertainty &&
           Dict::valid_word_permuter(word->best_choice->permuter(), true))) {
        word->tess_accepted = stopper_dict->AcceptableResult(word);
      } else {
        if (getDict().stopper_debug_level >= 1) {
          tprintf("Deleting word with certainty %g\n", word_certainty);
          word->best_choice->print();
        }
        // It is a dud.
        word->SetupFake(lstm_recognizer_->GetUnicharset());
      }
    }
  }
}
#endif  // ANDROID_BUILD

}  // namespace tesseract.
