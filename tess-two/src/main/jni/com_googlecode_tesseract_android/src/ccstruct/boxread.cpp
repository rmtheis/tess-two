/**********************************************************************
 * File:        boxread.cpp
 * Description: Read data from a box file.
 * Author:      Ray Smith
 * Created:     Fri Aug 24 17:47:23 PDT 2007
 *
 * (C) Copyright 2007, Google Inc.
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 ** http://www.apache.org/licenses/LICENSE-2.0
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 *
 **********************************************************************/

#include "boxread.h"
#include <string.h>

#include "fileerr.h"
#include "rect.h"
#include "strngs.h"
#include "tprintf.h"
#include "unichar.h"

// Special char code used to identify multi-blob labels.
static const char* kMultiBlobLabelCode = "WordStr";

// Open the boxfile based on the given image filename.
FILE* OpenBoxFile(const STRING& fname) {
  STRING filename = BoxFileName(fname);
  FILE* box_file = NULL;
  if (!(box_file = fopen(filename.string(), "rb"))) {
    CANTOPENFILE.error("read_next_box", TESSEXIT,
                       "Cant open box file %s",
                       filename.string());
  }
  return box_file;
}

// Reads all boxes from the given filename.
// Reads a specific target_page number if >= 0, or all pages otherwise.
// Skips blanks if skip_blanks is true.
// The UTF-8 label of the box is put in texts, and the full box definition as
// a string is put in box_texts, with the corresponding page number in pages.
// Each of the output vectors is optional (may be NULL).
// Returns false if no boxes are found.
bool ReadAllBoxes(int target_page, bool skip_blanks, const STRING& filename,
                  GenericVector<TBOX>* boxes,
                  GenericVector<STRING>* texts,
                  GenericVector<STRING>* box_texts,
                  GenericVector<int>* pages) {
  GenericVector<char> box_data;
  if (!tesseract::LoadDataFromFile(BoxFileName(filename), &box_data))
    return false;
  return ReadMemBoxes(target_page, skip_blanks, &box_data[0], boxes, texts,
                      box_texts, pages);
}

// Reads all boxes from the string. Otherwise, as ReadAllBoxes.
bool ReadMemBoxes(int target_page, bool skip_blanks, const char* box_data,
                  GenericVector<TBOX>* boxes,
                  GenericVector<STRING>* texts,
                  GenericVector<STRING>* box_texts,
                  GenericVector<int>* pages) {
  STRING box_str(box_data);
  GenericVector<STRING> lines;
  box_str.split('\n', &lines);
  if (lines.empty()) return false;
  int num_boxes = 0;
  for (int i = 0; i < lines.size(); ++i) {
    int page = 0;
    STRING utf8_str;
    TBOX box;
    if (!ParseBoxFileStr(lines[i].string(), &page, &utf8_str, &box)) {
      continue;
    }
    if (skip_blanks && utf8_str == " ") continue;
    if (target_page >= 0 && page != target_page) continue;
    if (boxes != NULL) boxes->push_back(box);
    if (texts != NULL) texts->push_back(utf8_str);
    if (box_texts != NULL) {
      STRING full_text;
      MakeBoxFileStr(utf8_str.string(), box, target_page, &full_text);
      box_texts->push_back(full_text);
    }
    if (pages != NULL) pages->push_back(page);
    ++num_boxes;
  }
  return num_boxes > 0;
}

// Returns the box file name corresponding to the given image_filename.
STRING BoxFileName(const STRING& image_filename) {
  STRING box_filename = image_filename;
  const char *lastdot = strrchr(box_filename.string(), '.');
  if (lastdot != NULL)
    box_filename.truncate_at(lastdot - box_filename.string());

  box_filename += ".box";
  return box_filename;
}

// TODO(rays) convert all uses of ReadNextBox to use the new ReadAllBoxes.
// Box files are used ONLY DURING TRAINING, but by both processes of
// creating tr files with tesseract, and unicharset_extractor.
// ReadNextBox factors out the code to interpret a line of a box
// file so that applybox and unicharset_extractor interpret the same way.
// This function returns the next valid box file utf8 string and coords
// and returns true, or false on eof (and closes the file).
// It ignores the utf8 file signature ByteOrderMark (U+FEFF=EF BB BF), checks
// for valid utf-8 and allows space or tab between fields.
// utf8_str is set with the unichar string, and bounding box with the box.
// If there are page numbers in the file, it reads them all.
bool ReadNextBox(int *line_number, FILE* box_file,
                 STRING* utf8_str, TBOX* bounding_box) {
  return ReadNextBox(-1, line_number, box_file, utf8_str, bounding_box);
}

// As ReadNextBox above, but get a specific page number. (0-based)
// Use -1 to read any page number. Files without page number all
// read as if they are page 0.
bool ReadNextBox(int target_page, int *line_number, FILE* box_file,
                 STRING* utf8_str, TBOX* bounding_box) {
  int page = 0;
  char buff[kBoxReadBufSize];   // boxfile read buffer
  char *buffptr = buff;

  while (fgets(buff, sizeof(buff) - 1, box_file)) {
    (*line_number)++;

    buffptr = buff;
    const unsigned char *ubuf = reinterpret_cast<const unsigned char*>(buffptr);
    if (ubuf[0] == 0xef && ubuf[1] == 0xbb && ubuf[2] == 0xbf)
      buffptr += 3;  // Skip unicode file designation.
    // Check for blank lines in box file
    if (*buffptr == '\n' || *buffptr == '\0') continue;
    // Skip blank boxes.
    if (*buffptr == ' ' || *buffptr == '\t') continue;
    if (*buffptr != '\0') {
      if (!ParseBoxFileStr(buffptr, &page, utf8_str, bounding_box)) {
        tprintf("Box file format error on line %i; ignored\n", *line_number);
        continue;
      }
      if (target_page >= 0 && target_page != page)
        continue;  // Not on the appropriate page.
      return true;  // Successfully read a box.
    }
  }
  fclose(box_file);
  return false;  // EOF
}

// Parses the given box file string into a page_number, utf8_str, and
// bounding_box. Returns true on a successful parse.
// The box file is assumed to contain box definitions, one per line, of the
// following format for blob-level boxes:
//   <UTF8 str> <left> <bottom> <right> <top> <page id>
// and for word/line-level boxes:
//   WordStr <left> <bottom> <right> <top> <page id> #<space-delimited word str>
// See applyybox.cpp for more information.
bool ParseBoxFileStr(const char* boxfile_str, int* page_number,
                     STRING* utf8_str, TBOX* bounding_box) {
  *bounding_box = TBOX();       // Initialize it to empty.
  *utf8_str = "";
  char uch[kBoxReadBufSize];
  const char *buffptr = boxfile_str;
  // Read the unichar without messing up on Tibetan.
  // According to issue 253 the utf-8 surrogates 85 and A0 are treated
  // as whitespace by sscanf, so it is more reliable to just find
  // ascii space and tab.
  int uch_len = 0;
  // Skip unicode file designation, if present.
  const unsigned char *ubuf = reinterpret_cast<const unsigned char*>(buffptr);
  if (ubuf[0] == 0xef && ubuf[1] == 0xbb && ubuf[2] == 0xbf)
      buffptr += 3;
  // Allow a single blank as the UTF-8 string. Check for empty string and
  // then blindly eat the first character.
  if (*buffptr == '\0') return false;
  do {
    uch[uch_len++] = *buffptr++;
  } while (*buffptr != '\0' && *buffptr != ' ' && *buffptr != '\t' &&
           uch_len < kBoxReadBufSize - 1);
  uch[uch_len] = '\0';
  if (*buffptr != '\0') ++buffptr;
  int x_min, y_min, x_max, y_max;
  *page_number = 0;
  int count = sscanf(buffptr, "%d %d %d %d %d",
                 &x_min, &y_min, &x_max, &y_max, page_number);
  if (count != 5 && count != 4) {
    tprintf("Bad box coordinates in boxfile string! %s\n", ubuf);
    return false;
  }
  // Test for long space-delimited string label.
  if (strcmp(uch, kMultiBlobLabelCode) == 0 &&
      (buffptr = strchr(buffptr, '#')) != NULL) {
    strncpy(uch, buffptr + 1, kBoxReadBufSize - 1);
    uch[kBoxReadBufSize - 1] = '\0';  // Prevent buffer overrun.
    chomp_string(uch);
    uch_len = strlen(uch);
  }
  // Validate UTF8 by making unichars with it.
  int used = 0;
  while (used < uch_len) {
    UNICHAR ch(uch + used, uch_len - used);
    int new_used = ch.utf8_len();
    if (new_used == 0) {
      tprintf("Bad UTF-8 str %s starts with 0x%02x at col %d\n",
              uch + used, uch[used], used + 1);
      return false;
    }
    used += new_used;
  }
  *utf8_str = uch;
  if (x_min > x_max) Swap(&x_min, &x_max);
  if (y_min > y_max) Swap(&y_min, &y_max);
  bounding_box->set_to_given_coords(x_min, y_min, x_max, y_max);
  return true;  // Successfully read a box.
}

// Creates a box file string from a unichar string, TBOX and page number.
void MakeBoxFileStr(const char* unichar_str, const TBOX& box, int page_num,
                    STRING* box_str) {
  *box_str = unichar_str;
  box_str->add_str_int(" ", box.left());
  box_str->add_str_int(" ", box.bottom());
  box_str->add_str_int(" ", box.right());
  box_str->add_str_int(" ", box.top());
  box_str->add_str_int(" ", page_num);
}

