///////////////////////////////////////////////////////////////////////
// File:        tesseractclass.h
// Description: The Tesseract class. It holds/owns everything needed
//              to run Tesseract on a single language, and also a set of
//              sub-Tesseracts to run sub-languages. For thread safety, *every*
//              global variable goes in here, directly, or indirectly.
//              This makes it safe to run multiple Tesseracts in different
//              threads in parallel, and keeps the different language
//              instances separate.
// Author:      Ray Smith
// Created:     Fri Mar 07 08:17:01 PST 2008
//
// (C) Copyright 2008, Google Inc.
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

#ifndef TESSERACT_CCMAIN_TESSERACTCLASS_H__
#define TESSERACT_CCMAIN_TESSERACTCLASS_H__

#include "allheaders.h"
#include "control.h"
#include "docqual.h"
#include "devanagari_processing.h"
#include "genericvector.h"
#include "params.h"
#include "ocrclass.h"
#include "textord.h"
#include "wordrec.h"

class BLOB_CHOICE_LIST_CLIST;
class BLOCK_LIST;
class CharSamp;
struct OSResults;
class PAGE_RES;
class PAGE_RES_IT;
struct Pix;
class ROW;
class SVMenuNode;
class TBOX;
class TO_BLOCK_LIST;
class WERD;
class WERD_CHOICE;
class WERD_RES;


// Top-level class for all tesseract global instance data.
// This class either holds or points to all data used by an instance
// of Tesseract, including the memory allocator. When this is
// complete, Tesseract will be thread-safe. UNTIL THEN, IT IS NOT!
//
// NOTE to developers: Do not create cyclic dependencies through this class!
// The directory dependency tree must remain a tree! The keep this clean,
// lower-level code (eg in ccutil, the bottom level) must never need to
// know about the content of a higher-level directory.
// The following scheme will grant the easiest access to lower-level
// global members without creating a cyclic dependency:
//
// Class Hierarchy (^ = inheritance):
//
//             CCUtil (ccutil/ccutil.h)
//                         ^      Members include: UNICHARSET
//            CUtil (cutil/cutil_class.h)
//                         ^       Members include: TBLOB*, TEXTBLOCK*
//           CCStruct (ccstruct/ccstruct.h)
//                         ^       Members include: Image
//           Classify (classify/classify.h)
//                         ^       Members include: Dict
//             WordRec (wordrec/wordrec.h)
//                         ^       Members include: WERD*, DENORM*
//        Tesseract (ccmain/tesseractclass.h)
//                                 Members include: Pix*, CubeRecoContext*,
//                                 TesseractCubeCombiner*
//
// Other important classes:
//
//  TessBaseAPI (api/baseapi.h)
//                                 Members include: BLOCK_LIST*, PAGE_RES*,
//                                 Tesseract*, ImageThresholder*
//  Dict (dict/dict.h)
//                                 Members include: Image* (private)
//
// NOTE: that each level contains members that correspond to global
// data that is defined (and used) at that level, not necessarily where
// the type is defined so for instance:
// BOOL_VAR_H(textord_show_blobs, false, "Display unsorted blobs");
// goes inside the Textord class, not the cc_util class.

namespace tesseract {

class ColumnFinder;
class CubeLineObject;
class CubeObject;
class CubeRecoContext;
class EquationDetect;
class Tesseract;
class TesseractCubeCombiner;

// A collection of various variables for statistics and debugging.
struct TesseractStats {
  TesseractStats()
    : adaption_word_number(0),
      doc_blob_quality(0),
      doc_outline_errs(0),
      doc_char_quality(0),
      good_char_count(0),
      doc_good_char_quality(0),
      word_count(0),
      dict_words(0),
      tilde_crunch_written(false),
      last_char_was_newline(true),
      last_char_was_tilde(false),
      write_results_empty_block(true) {}

  inT32 adaption_word_number;
  inT16 doc_blob_quality;
  inT16 doc_outline_errs;
  inT16 doc_char_quality;
  inT16 good_char_count;
  inT16 doc_good_char_quality;
  inT32 word_count;  // count of word in the document
  inT32 dict_words;  // number of dicitionary words in the document
  STRING dump_words_str;  // accumulator used by dump_words()
  // Flags used by write_results()
  bool tilde_crunch_written;
  bool last_char_was_newline;
  bool last_char_was_tilde;
  bool write_results_empty_block;
};

// Struct to hold all the pointers to relevant data for processing a word.
struct WordData {
  WordData() : word(NULL), row(NULL), block(NULL), prev_word(NULL) {}
  explicit WordData(const PAGE_RES_IT& page_res_it)
    : word(page_res_it.word()), row(page_res_it.row()->row),
      block(page_res_it.block()->block), prev_word(NULL) {}
  WordData(BLOCK* block_in, ROW* row_in, WERD_RES* word_res)
    : word(word_res), row(row_in), block(block_in), prev_word(NULL) {}

  WERD_RES* word;
  ROW* row;
  BLOCK* block;
  WordData* prev_word;
  PointerVector<WERD_RES> lang_words;
};

// Definition of a Tesseract WordRecognizer. The WordData provides the context
// of row/block, in_word holds an initialized, possibly pre-classified word,
// that the recognizer may or may not consume (but if so it sets *in_word=NULL)
// and produces one or more output words in out_words, which may be the
// consumed in_word, or may be generated independently.
// This api allows both a conventional tesseract classifier to work, or a
// line-level classifier that generates multiple words from a merged input.
typedef void (Tesseract::*WordRecognizer)(const WordData& word_data,
                                          WERD_RES** in_word,
                                          PointerVector<WERD_RES>* out_words);

class Tesseract : public Wordrec {
 public:
  Tesseract();
  ~Tesseract();

  // Clear as much used memory as possible without resetting the adaptive
  // classifier or losing any other classifier data.
  void Clear();
  // Clear all memory of adaption for this and all subclassifiers.
  void ResetAdaptiveClassifier();
  // Clear the document dictionary for this and all subclassifiers.
  void ResetDocumentDictionary();

  // Set the equation detector.
  void SetEquationDetect(EquationDetect* detector);

  // Simple accessors.
  const FCOORD& reskew() const {
    return reskew_;
  }
  // Destroy any existing pix and return a pointer to the pointer.
  Pix** mutable_pix_binary() {
    Clear();
    return &pix_binary_;
  }
  Pix* pix_binary() const {
    return pix_binary_;
  }
  Pix* pix_grey() const {
    return pix_grey_;
  }
  void set_pix_grey(Pix* grey_pix) {
    pixDestroy(&pix_grey_);
    pix_grey_ = grey_pix;
  }
  // Returns a pointer to a Pix representing the best available image of the
  // page. The image will be 8-bit grey if the input was grey or color. Note
  // that in grey 0 is black and 255 is white. If the input was binary, then
  // the returned Pix will be binary. Note that here black is 1 and white is 0.
  // To tell the difference pixGetDepth() will return 8 or 1.
  // In either case, the return value is a borrowed Pix, and should not be
  // deleted or pixDestroyed.
  Pix* BestPix() const {
    return pix_grey_ != NULL ? pix_grey_ : pix_binary_;
  }
  void set_pix_thresholds(Pix* thresholds) {
    pixDestroy(&pix_thresholds_);
    pix_thresholds_ = thresholds;
  }
  int source_resolution() const {
    return source_resolution_;
  }
  void set_source_resolution(int ppi) {
    source_resolution_ = ppi;
  }
  int ImageWidth() const {
    return pixGetWidth(pix_binary_);
  }
  int ImageHeight() const {
    return pixGetHeight(pix_binary_);
  }
  Pix* scaled_color() const {
    return scaled_color_;
  }
  int scaled_factor() const {
    return scaled_factor_;
  }
  void SetScaledColor(int factor, Pix* color) {
    scaled_factor_ = factor;
    scaled_color_ = color;
  }
  const Textord& textord() const {
    return textord_;
  }
  Textord* mutable_textord() {
    return &textord_;
  }

  bool right_to_left() const {
    return right_to_left_;
  }
  int num_sub_langs() const {
    return sub_langs_.size();
  }
  Tesseract* get_sub_lang(int index) const {
    return sub_langs_[index];
  }
  // Returns true if any language uses Tesseract (as opposed to cube).
  bool AnyTessLang() const {
    if (tessedit_ocr_engine_mode != OEM_CUBE_ONLY) return true;
    for (int i = 0; i < sub_langs_.size(); ++i) {
      if (sub_langs_[i]->tessedit_ocr_engine_mode != OEM_CUBE_ONLY)
        return true;
    }
    return false;
  }

  void SetBlackAndWhitelist();

  // Perform steps to prepare underlying binary image/other data structures for
  // page segmentation. Uses the strategy specified in the global variable
  // pageseg_devanagari_split_strategy for perform splitting while preparing for
  // page segmentation.
  void PrepareForPageseg();

  // Perform steps to prepare underlying binary image/other data structures for
  // Tesseract OCR. The current segmentation is required by this method.
  // Uses the strategy specified in the global variable
  // ocr_devanagari_split_strategy for performing splitting while preparing for
  // Tesseract ocr.
  void PrepareForTessOCR(BLOCK_LIST* block_list,
                         Tesseract* osd_tess, OSResults* osr);

  int SegmentPage(const STRING* input_file, BLOCK_LIST* blocks,
                  Tesseract* osd_tess, OSResults* osr);
  void SetupWordScripts(BLOCK_LIST* blocks);
  int AutoPageSeg(PageSegMode pageseg_mode,
                  BLOCK_LIST* blocks, TO_BLOCK_LIST* to_blocks,
                  Tesseract* osd_tess, OSResults* osr);
  ColumnFinder* SetupPageSegAndDetectOrientation(
      bool single_column, bool osd, bool only_osd,
      BLOCK_LIST* blocks, Tesseract* osd_tess, OSResults* osr,
      TO_BLOCK_LIST* to_blocks, Pix** photo_mask_pix, Pix** music_mask_pix);
  // par_control.cpp
  void PrerecAllWordsPar(const GenericVector<WordData>& words);

  //// control.h /////////////////////////////////////////////////////////
  bool ProcessTargetWord(const TBOX& word_box, const TBOX& target_word_box,
                         const char* word_config, int pass);
  // Sets up the words ready for whichever engine is to be run
  void SetupAllWordsPassN(int pass_n,
                          const TBOX* target_word_box,
                          const char* word_config,
                          PAGE_RES* page_res,
                          GenericVector<WordData>* words);
  // Sets up the single word ready for whichever engine is to be run.
  void SetupWordPassN(int pass_n, WordData* word);
  // Runs word recognition on all the words.
  bool RecogAllWordsPassN(int pass_n, ETEXT_DESC* monitor,
                          PAGE_RES_IT* pr_it,
                          GenericVector<WordData>* words);
  bool recog_all_words(PAGE_RES* page_res,
                       ETEXT_DESC* monitor,
                       const TBOX* target_word_box,
                       const char* word_config,
                       int dopasses);
  void rejection_passes(PAGE_RES* page_res,
                        ETEXT_DESC* monitor,
                        const TBOX* target_word_box,
                        const char* word_config);
  void bigram_correction_pass(PAGE_RES *page_res);
  void blamer_pass(PAGE_RES* page_res);
  // Sets script positions and detects smallcaps on all output words.
  void script_pos_pass(PAGE_RES* page_res);
  // Helper to recognize the word using the given (language-specific) tesseract.
  // Returns positive if this recognizer found more new best words than the
  // number kept from best_words.
  int RetryWithLanguage(const WordData& word_data,
                        WordRecognizer recognizer,
                        WERD_RES** in_word,
                        PointerVector<WERD_RES>* best_words);
  void classify_word_and_language(WordRecognizer recognizer,
                                  PAGE_RES_IT* pr_it,
                                  WordData* word_data);
  void classify_word_pass1(const WordData& word_data,
                           WERD_RES** in_word,
                           PointerVector<WERD_RES>* out_words);
  void recog_pseudo_word(PAGE_RES* page_res,  // blocks to check
                         TBOX &selection_box);

  void fix_rep_char(PAGE_RES_IT* page_res_it);

  ACCEPTABLE_WERD_TYPE acceptable_word_string(const UNICHARSET& char_set,
                                              const char *s,
                                              const char *lengths);
  void match_word_pass_n(int pass_n, WERD_RES *word, ROW *row, BLOCK* block);
  void classify_word_pass2(const WordData& word_data,
                           WERD_RES** in_word,
                           PointerVector<WERD_RES>* out_words);
  void ReportXhtFixResult(bool accept_new_word, float new_x_ht,
                          WERD_RES* word, WERD_RES* new_word);
  bool RunOldFixXht(WERD_RES *word, BLOCK* block, ROW *row);
  bool TrainedXheightFix(WERD_RES *word, BLOCK* block, ROW *row);
  BOOL8 recog_interactive(PAGE_RES_IT* pr_it);

  // Set fonts of this word.
  void set_word_fonts(WERD_RES *word);
  void font_recognition_pass(PAGE_RES* page_res);
  void dictionary_correction_pass(PAGE_RES* page_res);
  BOOL8 check_debug_pt(WERD_RES *word, int location);

  //// superscript.cpp ////////////////////////////////////////////////////
  bool SubAndSuperscriptFix(WERD_RES *word_res);
  void GetSubAndSuperscriptCandidates(const WERD_RES *word,
                                      int *num_rebuilt_leading,
                                      ScriptPos *leading_pos,
                                      float *leading_certainty,
                                      int *num_rebuilt_trailing,
                                      ScriptPos *trailing_pos,
                                      float *trailing_certainty,
                                      float *avg_certainty,
                                      float *unlikely_threshold);
  WERD_RES *TrySuperscriptSplits(int num_chopped_leading,
                                 float leading_certainty,
                                 ScriptPos leading_pos,
                                 int num_chopped_trailing,
                                 float trailing_certainty,
                                 ScriptPos trailing_pos,
                                 WERD_RES *word,
                                 bool *is_good,
                                 int *retry_leading,
                                 int *retry_trailing);
  bool BelievableSuperscript(bool debug,
                             const WERD_RES &word,
                             float certainty_threshold,
                             int *left_ok,
                             int *right_ok) const;

  //// cube_control.cpp ///////////////////////////////////////////////////
  bool init_cube_objects(bool load_combiner,
                         TessdataManager *tessdata_manager);
  // Iterates through tesseract's results and calls cube on each word,
  // combining the results with the existing tesseract result.
  void run_cube_combiner(PAGE_RES *page_res);
  // Recognizes a single word using (only) cube. Compatible with
  // Tesseract's classify_word_pass1/classify_word_pass2.
  void cube_word_pass1(BLOCK* block, ROW *row, WERD_RES *word);
  // Cube recognizer to recognize a single word as with classify_word_pass1
  // but also returns the cube object in case the combiner is needed.
  CubeObject* cube_recognize_word(BLOCK* block, WERD_RES* word);
  // Combines the cube and tesseract results for a single word, leaving the
  // result in tess_word.
  void cube_combine_word(CubeObject* cube_obj, WERD_RES* cube_word,
                        WERD_RES* tess_word);
  // Call cube on the current word, and write the result to word.
  // Sets up a fake result  and returns false if something goes wrong.
  bool cube_recognize(CubeObject *cube_obj, BLOCK* block, WERD_RES *word);
  void fill_werd_res(const BoxWord& cube_box_word,
                     const char* cube_best_str,
                     WERD_RES* tess_werd_res);
  bool extract_cube_state(CubeObject* cube_obj, int* num_chars,
                          Boxa** char_boxes, CharSamp*** char_samples);
  bool create_cube_box_word(Boxa *char_boxes, int num_chars,
                            TBOX word_box, BoxWord* box_word);
  //// output.h //////////////////////////////////////////////////////////

  void output_pass(PAGE_RES_IT &page_res_it, const TBOX *target_word_box);
  void write_results(PAGE_RES_IT &page_res_it,  // full info
                     char newline_type,         // type of newline
                     BOOL8 force_eol            // override tilde crunch?
                    );
  void set_unlv_suspects(WERD_RES *word);
  UNICHAR_ID get_rep_char(WERD_RES *word);  // what char is repeated?
  BOOL8 acceptable_number_string(const char *s,
                                 const char *lengths);
  inT16 count_alphanums(const WERD_CHOICE &word);
  inT16 count_alphas(const WERD_CHOICE &word);
  //// tessedit.h ////////////////////////////////////////////////////////
  void read_config_file(const char *filename, SetParamConstraint constraint);
  // Initialize for potentially a set of languages defined by the language
  // string and recursively any additional languages required by any language
  // traineddata file (via tessedit_load_sublangs in its config) that is loaded.
  // See init_tesseract_internal for args.
  int init_tesseract(const char *arg0,
                     const char *textbase,
                     const char *language,
                     OcrEngineMode oem,
                     char **configs,
                     int configs_size,
                     const GenericVector<STRING> *vars_vec,
                     const GenericVector<STRING> *vars_values,
                     bool set_only_init_params);
  int init_tesseract(const char *datapath,
                     const char *language,
                     OcrEngineMode oem) {
    return init_tesseract(datapath, NULL, language, oem,
                          NULL, 0, NULL, NULL, false);
  }
  // Common initialization for a single language.
  // arg0 is the datapath for the tessdata directory, which could be the
  // path of the tessdata directory with no trailing /, or (if tessdata
  // lives in the same directory as the executable, the path of the executable,
  // hence the name arg0.
  // textbase is an optional output file basename (used only for training)
  // language is the language code to load.
  // oem controls which engine(s) will operate on the image
  // configs (argv) is an array of config filenames to load variables from.
  // May be NULL.
  // configs_size (argc) is the number of elements in configs.
  // vars_vec is an optional vector of variables to set.
  // vars_values is an optional corresponding vector of values for the variables
  // in vars_vec.
  // If set_only_init_params is true, then only the initialization variables
  // will be set.
  int init_tesseract_internal(const char *arg0,
                              const char *textbase,
                              const char *language,
                              OcrEngineMode oem,
                              char **configs,
                              int configs_size,
                              const GenericVector<STRING> *vars_vec,
                              const GenericVector<STRING> *vars_values,
                              bool set_only_init_params);

  // Set the universal_id member of each font to be unique among all
  // instances of the same font loaded.
  void SetupUniversalFontIds();

  int init_tesseract_lm(const char *arg0,
                        const char *textbase,
                        const char *language);

  void recognize_page(STRING& image_name);
  void end_tesseract();

  bool init_tesseract_lang_data(const char *arg0,
                                const char *textbase,
                                const char *language,
                                OcrEngineMode oem,
                                char **configs,
                                int configs_size,
                                const GenericVector<STRING> *vars_vec,
                                const GenericVector<STRING> *vars_values,
                                bool set_only_init_params);

  void ParseLanguageString(const char* lang_str,
                           GenericVector<STRING>* to_load,
                           GenericVector<STRING>* not_to_load);

  //// pgedit.h //////////////////////////////////////////////////////////
  SVMenuNode *build_menu_new();
  #ifndef GRAPHICS_DISABLED
  void pgeditor_main(int width, int height, PAGE_RES* page_res);
  #endif  // GRAPHICS_DISABLED
  void process_image_event( // action in image win
                           const SVEvent &event);
  BOOL8 process_cmd_win_event(                 // UI command semantics
                              inT32 cmd_event,  // which menu item?
                              char *new_value   // any prompt data
                             );
  void debug_word(PAGE_RES* page_res, const TBOX &selection_box);
  void do_re_display(
      BOOL8 (tesseract::Tesseract::*word_painter)(PAGE_RES_IT* pr_it));
  BOOL8 word_display(PAGE_RES_IT* pr_it);
  BOOL8 word_bln_display(PAGE_RES_IT* pr_it);
  BOOL8 word_blank_and_set_display(PAGE_RES_IT* pr_its);
  BOOL8 word_set_display(PAGE_RES_IT* pr_it);
  // #ifndef GRAPHICS_DISABLED
  BOOL8 word_dumper(PAGE_RES_IT* pr_it);
  // #endif  // GRAPHICS_DISABLED
  void blob_feature_display(PAGE_RES* page_res, const TBOX& selection_box);
  //// reject.h //////////////////////////////////////////////////////////
  // make rej map for word
  void make_reject_map(WERD_RES *word, ROW *row, inT16 pass);
  BOOL8 one_ell_conflict(WERD_RES *word_res, BOOL8 update_map);
  inT16 first_alphanum_index(const char *word,
                             const char *word_lengths);
  inT16 first_alphanum_offset(const char *word,
                              const char *word_lengths);
  inT16 alpha_count(const char *word,
                    const char *word_lengths);
  BOOL8 word_contains_non_1_digit(const char *word,
                                  const char *word_lengths);
  void dont_allow_1Il(WERD_RES *word);
  inT16 count_alphanums(  //how many alphanums
                        WERD_RES *word);
  void flip_0O(WERD_RES *word);
  BOOL8 non_0_digit(const UNICHARSET& ch_set, UNICHAR_ID unichar_id);
  BOOL8 non_O_upper(const UNICHARSET& ch_set, UNICHAR_ID unichar_id);
  BOOL8 repeated_nonalphanum_wd(WERD_RES *word, ROW *row);
  void nn_match_word(  //Match a word
                     WERD_RES *word,
                     ROW *row);
  void nn_recover_rejects(WERD_RES *word, ROW *row);
  void set_done(  //set done flag
                WERD_RES *word,
                inT16 pass);
  inT16 safe_dict_word(const WERD_RES *werd_res);  // is best_choice in dict?
  void flip_hyphens(WERD_RES *word);
  void reject_I_1_L(WERD_RES *word);
  void reject_edge_blobs(WERD_RES *word);
  void reject_mostly_rejects(WERD_RES *word);
  //// adaptions.h ///////////////////////////////////////////////////////
  BOOL8 word_adaptable(  //should we adapt?
                       WERD_RES *word,
                       uinT16 mode);

  //// tfacepp.cpp ///////////////////////////////////////////////////////
  void recog_word_recursive(WERD_RES* word);
  void recog_word(WERD_RES *word);
  void split_and_recog_word(WERD_RES* word);
  void split_word(WERD_RES *word,
                  int split_pt,
                  WERD_RES **right_piece,
                  BlamerBundle **orig_blamer_bundle) const;
  void join_words(WERD_RES *word,
                  WERD_RES *word2,
                  BlamerBundle *orig_bb) const;
  //// fixspace.cpp ///////////////////////////////////////////////////////
  BOOL8 digit_or_numeric_punct(WERD_RES *word, int char_position);
  inT16 eval_word_spacing(WERD_RES_LIST &word_res_list);
  void match_current_words(WERD_RES_LIST &words, ROW *row, BLOCK* block);
  inT16 fp_eval_word_spacing(WERD_RES_LIST &word_res_list);
  void fix_noisy_space_list(WERD_RES_LIST &best_perm, ROW *row, BLOCK* block);
  void fix_fuzzy_space_list(WERD_RES_LIST &best_perm, ROW *row, BLOCK* block);
  void fix_sp_fp_word(WERD_RES_IT &word_res_it, ROW *row, BLOCK* block);
  void fix_fuzzy_spaces(                      //find fuzzy words
                        ETEXT_DESC *monitor,  //progress monitor
                        inT32 word_count,     //count of words in doc
                        PAGE_RES *page_res);
  void dump_words(WERD_RES_LIST &perm, inT16 score,
                  inT16 mode, BOOL8 improved);
  BOOL8 fixspace_thinks_word_done(WERD_RES *word);
  inT16 worst_noise_blob(WERD_RES *word_res, float *worst_noise_score);
  float blob_noise_score(TBLOB *blob);
  void break_noisiest_blob_word(WERD_RES_LIST &words);
  //// docqual.cpp ////////////////////////////////////////////////////////
  GARBAGE_LEVEL garbage_word(WERD_RES *word, BOOL8 ok_dict_word);
  BOOL8 potential_word_crunch(WERD_RES *word,
                              GARBAGE_LEVEL garbage_level,
                              BOOL8 ok_dict_word);
  void tilde_crunch(PAGE_RES_IT &page_res_it);
  void unrej_good_quality_words(  //unreject potential
                                PAGE_RES_IT &page_res_it);
  void doc_and_block_rejection(  //reject big chunks
                               PAGE_RES_IT &page_res_it,
                               BOOL8 good_quality_doc);
  void quality_based_rejection(PAGE_RES_IT &page_res_it,
                               BOOL8 good_quality_doc);
  void convert_bad_unlv_chs(WERD_RES *word_res);
  void tilde_delete(PAGE_RES_IT &page_res_it);
  inT16 word_blob_quality(WERD_RES *word, ROW *row);
  void word_char_quality(WERD_RES *word, ROW *row, inT16 *match_count,
                         inT16 *accepted_match_count);
  void unrej_good_chs(WERD_RES *word, ROW *row);
  inT16 count_outline_errs(char c, inT16 outline_count);
  inT16 word_outline_errs(WERD_RES *word);
  BOOL8 terrible_word_crunch(WERD_RES *word, GARBAGE_LEVEL garbage_level);
  CRUNCH_MODE word_deletable(WERD_RES *word, inT16 &delete_mode);
  inT16 failure_count(WERD_RES *word);
  BOOL8 noise_outlines(TWERD *word);
  //// pagewalk.cpp ///////////////////////////////////////////////////////
  void
  process_selected_words (
      PAGE_RES* page_res, // blocks to check
      //function to call
      TBOX & selection_box,
      BOOL8 (tesseract::Tesseract::*word_processor)(PAGE_RES_IT* pr_it));
  //// tessbox.cpp ///////////////////////////////////////////////////////
  void tess_add_doc_word(                          //test acceptability
                         WERD_CHOICE *word_choice  //after context
                        );
  void tess_segment_pass_n(int pass_n, WERD_RES *word);
  bool tess_acceptable_word(WERD_RES *word);

  //// applybox.cpp //////////////////////////////////////////////////////
  // Applies the box file based on the image name fname, and resegments
  // the words in the block_list (page), with:
  // blob-mode: one blob per line in the box file, words as input.
  // word/line-mode: one blob per space-delimited unit after the #, and one word
  // per line in the box file. (See comment above for box file format.)
  // If find_segmentation is true, (word/line mode) then the classifier is used
  // to re-segment words/lines to match the space-delimited truth string for
  // each box. In this case, the input box may be for a word or even a whole
  // text line, and the output words will contain multiple blobs corresponding
  // to the space-delimited input string.
  // With find_segmentation false, no classifier is needed, but the chopper
  // can still be used to correctly segment touching characters with the help
  // of the input boxes.
  // In the returned PAGE_RES, the WERD_RES are setup as they would be returned
  // from normal classification, ie. with a word, chopped_word, rebuild_word,
  // seam_array, denorm, box_word, and best_state, but NO best_choice or
  // raw_choice, as they would require a UNICHARSET, which we aim to avoid.
  // Instead, the correct_text member of WERD_RES is set, and this may be later
  // converted to a best_choice using CorrectClassifyWords. CorrectClassifyWords
  // is not required before calling ApplyBoxTraining.
  PAGE_RES* ApplyBoxes(const STRING& fname, bool find_segmentation,
                       BLOCK_LIST *block_list);

  // Any row xheight that is significantly different from the median is set
  // to the median.
  void PreenXHeights(BLOCK_LIST *block_list);

  // Builds a PAGE_RES from the block_list in the way required for ApplyBoxes:
  // All fuzzy spaces are removed, and all the words are maximally chopped.
  PAGE_RES* SetupApplyBoxes(const GenericVector<TBOX>& boxes,
                            BLOCK_LIST *block_list);
  // Tests the chopper by exhaustively running chop_one_blob.
  // The word_res will contain filled chopped_word, seam_array, denorm,
  // box_word and best_state for the maximally chopped word.
  void MaximallyChopWord(const GenericVector<TBOX>& boxes,
                         BLOCK* block, ROW* row, WERD_RES* word_res);
  // Gather consecutive blobs that match the given box into the best_state
  // and corresponding correct_text.
  // Fights over which box owns which blobs are settled by pre-chopping and
  // applying the blobs to box or next_box with the least non-overlap.
  // Returns false if the box was in error, which can only be caused by
  // failing to find an appropriate blob for a box.
  // This means that occasionally, blobs may be incorrectly segmented if the
  // chopper fails to find a suitable chop point.
  bool ResegmentCharBox(PAGE_RES* page_res, const TBOX *prev_box,
                        const TBOX& box, const TBOX& next_box,
                        const char* correct_text);
  // Consume all source blobs that strongly overlap the given box,
  // putting them into a new word, with the correct_text label.
  // Fights over which box owns which blobs are settled by
  // applying the blobs to box or next_box with the least non-overlap.
  // Returns false if the box was in error, which can only be caused by
  // failing to find an overlapping blob for a box.
  bool ResegmentWordBox(BLOCK_LIST *block_list,
                        const TBOX& box, const TBOX& next_box,
                        const char* correct_text);
  // Resegments the words by running the classifier in an attempt to find the
  // correct segmentation that produces the required string.
  void ReSegmentByClassification(PAGE_RES* page_res);
  // Converts the space-delimited string of utf8 text to a vector of UNICHAR_ID.
  // Returns false if an invalid UNICHAR_ID is encountered.
  bool ConvertStringToUnichars(const char* utf8,
                               GenericVector<UNICHAR_ID>* class_ids);
  // Resegments the word to achieve the target_text from the classifier.
  // Returns false if the re-segmentation fails.
  // Uses brute-force combination of upto kMaxGroupSize adjacent blobs, and
  // applies a full search on the classifier results to find the best classified
  // segmentation. As a compromise to obtain better recall, 1-1 ambigiguity
  // substitutions ARE used.
  bool FindSegmentation(const GenericVector<UNICHAR_ID>& target_text,
                        WERD_RES* word_res);
  // Recursive helper to find a match to the target_text (from text_index
  // position) in the choices (from choices_pos position).
  // Choices is an array of GenericVectors, of length choices_length, with each
  // element representing a starting position in the word, and the
  // GenericVector holding classification results for a sequence of consecutive
  // blobs, with index 0 being a single blob, index 1 being 2 blobs etc.
  void SearchForText(const GenericVector<BLOB_CHOICE_LIST*>* choices,
                     int choices_pos, int choices_length,
                     const GenericVector<UNICHAR_ID>& target_text,
                     int text_index,
                     float rating, GenericVector<int>* segmentation,
                     float* best_rating, GenericVector<int>* best_segmentation);
  // Counts up the labelled words and the blobs within.
  // Deletes all unused or emptied words, counting the unused ones.
  // Resets W_BOL and W_EOL flags correctly.
  // Builds the rebuild_word and rebuilds the box_word.
  void TidyUp(PAGE_RES* page_res);
  // Logs a bad box by line in the box file and box coords.
  void ReportFailedBox(int boxfile_lineno, TBOX box, const char *box_ch,
                       const char *err_msg);
  // Creates a fake best_choice entry in each WERD_RES with the correct text.
  void CorrectClassifyWords(PAGE_RES* page_res);
  // Call LearnWord to extract features for labelled blobs within each word.
  // Features are written to the given filename.
  void ApplyBoxTraining(const STRING& filename, PAGE_RES* page_res);

  //// fixxht.cpp ///////////////////////////////////////////////////////
  // Returns the number of misfit blob tops in this word.
  int CountMisfitTops(WERD_RES *word_res);
  // Returns a new x-height in pixels (original image coords) that is
  // maximally compatible with the result in word_res.
  // Returns 0.0f if no x-height is found that is better than the current
  // estimate.
  float ComputeCompatibleXheight(WERD_RES *word_res);
  //// Data members ///////////////////////////////////////////////////////
  // TODO(ocr-team): Find and remove obsolete parameters.
  BOOL_VAR_H(tessedit_resegment_from_boxes, false,
             "Take segmentation and labeling from box file");
  BOOL_VAR_H(tessedit_resegment_from_line_boxes, false,
              "Conversion of word/line box file to char box file");
  BOOL_VAR_H(tessedit_train_from_boxes, false,
             "Generate training data from boxed chars");
  BOOL_VAR_H(tessedit_make_boxes_from_boxes, false,
             "Generate more boxes from boxed chars");
  BOOL_VAR_H(tessedit_dump_pageseg_images, false,
             "Dump intermediate images made during page segmentation");
  INT_VAR_H(tessedit_pageseg_mode, PSM_SINGLE_BLOCK,
            "Page seg mode: 0=osd only, 1=auto+osd, 2=auto, 3=col, 4=block,"
            " 5=line, 6=word, 7=char"
            " (Values from PageSegMode enum in publictypes.h)");
  INT_VAR_H(tessedit_ocr_engine_mode, tesseract::OEM_TESSERACT_ONLY,
            "Which OCR engine(s) to run (Tesseract, Cube, both). Defaults"
            " to loading and running only Tesseract (no Cube, no combiner)."
            " (Values from OcrEngineMode enum in tesseractclass.h)");
  STRING_VAR_H(tessedit_char_blacklist, "",
               "Blacklist of chars not to recognize");
  STRING_VAR_H(tessedit_char_whitelist, "",
               "Whitelist of chars to recognize");
  STRING_VAR_H(tessedit_char_unblacklist, "",
               "List of chars to override tessedit_char_blacklist");
  BOOL_VAR_H(tessedit_ambigs_training, false,
             "Perform training for ambiguities");
  INT_VAR_H(pageseg_devanagari_split_strategy,
            tesseract::ShiroRekhaSplitter::NO_SPLIT,
            "Whether to use the top-line splitting process for Devanagari "
            "documents while performing page-segmentation.");
  INT_VAR_H(ocr_devanagari_split_strategy,
            tesseract::ShiroRekhaSplitter::NO_SPLIT,
            "Whether to use the top-line splitting process for Devanagari "
            "documents while performing ocr.");
  STRING_VAR_H(tessedit_write_params_to_file, "",
               "Write all parameters to the given file.");
  BOOL_VAR_H(tessedit_adaption_debug, false,
             "Generate and print debug information for adaption");
  INT_VAR_H(bidi_debug, 0, "Debug level for BiDi");
  INT_VAR_H(applybox_debug, 1, "Debug level");
  INT_VAR_H(applybox_page, 0, "Page number to apply boxes from");
  STRING_VAR_H(applybox_exposure_pattern, ".exp",
               "Exposure value follows this pattern in the image"
               " filename. The name of the image files are expected"
               " to be in the form [lang].[fontname].exp[num].tif");
  BOOL_VAR_H(applybox_learn_chars_and_char_frags_mode, false,
             "Learn both character fragments (as is done in the"
             " special low exposure mode) as well as unfragmented"
             " characters.");
  BOOL_VAR_H(applybox_learn_ngrams_mode, false,
             "Each bounding box is assumed to contain ngrams. Only"
             " learn the ngrams whose outlines overlap horizontally.");
  BOOL_VAR_H(tessedit_display_outwords, false, "Draw output words");
  BOOL_VAR_H(tessedit_dump_choices, false, "Dump char choices");
  BOOL_VAR_H(tessedit_timing_debug, false, "Print timing stats");
  BOOL_VAR_H(tessedit_fix_fuzzy_spaces, true,
             "Try to improve fuzzy spaces");
  BOOL_VAR_H(tessedit_unrej_any_wd, false,
             "Dont bother with word plausibility");
  BOOL_VAR_H(tessedit_fix_hyphens, true, "Crunch double hyphens?");
  BOOL_VAR_H(tessedit_redo_xheight, true, "Check/Correct x-height");
  BOOL_VAR_H(tessedit_enable_doc_dict, true,
             "Add words to the document dictionary");
  BOOL_VAR_H(tessedit_debug_fonts, false, "Output font info per char");
  BOOL_VAR_H(tessedit_debug_block_rejection, false, "Block and Row stats");
  BOOL_VAR_H(tessedit_enable_bigram_correction, true,
             "Enable correction based on the word bigram dictionary.");
  BOOL_VAR_H(tessedit_enable_dict_correction, false,
             "Enable single word correction based on the dictionary.");
  INT_VAR_H(tessedit_bigram_debug, 0, "Amount of debug output for bigram "
            "correction.");
  INT_VAR_H(debug_x_ht_level, 0, "Reestimate debug");
  BOOL_VAR_H(debug_acceptable_wds, false, "Dump word pass/fail chk");
  STRING_VAR_H(chs_leading_punct, "('`\"", "Leading punctuation");
  STRING_VAR_H(chs_trailing_punct1, ").,;:?!", "1st Trailing punctuation");
  STRING_VAR_H(chs_trailing_punct2, ")'`\"", "2nd Trailing punctuation");
  double_VAR_H(quality_rej_pc, 0.08, "good_quality_doc lte rejection limit");
  double_VAR_H(quality_blob_pc, 0.0, "good_quality_doc gte good blobs limit");
  double_VAR_H(quality_outline_pc, 1.0,
               "good_quality_doc lte outline error limit");
  double_VAR_H(quality_char_pc, 0.95, "good_quality_doc gte good char limit");
  INT_VAR_H(quality_min_initial_alphas_reqd, 2, "alphas in a good word");
  INT_VAR_H(tessedit_tess_adaption_mode, 0x27,
            "Adaptation decision algorithm for tess");
  BOOL_VAR_H(tessedit_minimal_rej_pass1, false,
             "Do minimal rejection on pass 1 output");
  BOOL_VAR_H(tessedit_test_adaption, false, "Test adaption criteria");
  BOOL_VAR_H(tessedit_matcher_log, false, "Log matcher activity");
  INT_VAR_H(tessedit_test_adaption_mode, 3,
            "Adaptation decision algorithm for tess");
  BOOL_VAR_H(test_pt, false, "Test for point");
  double_VAR_H(test_pt_x, 99999.99, "xcoord");
  double_VAR_H(test_pt_y, 99999.99, "ycoord");
  INT_VAR_H(paragraph_debug_level, 0, "Print paragraph debug info.");
  BOOL_VAR_H(paragraph_text_based, true,
             "Run paragraph detection on the post-text-recognition "
             "(more accurate)");
  INT_VAR_H(cube_debug_level, 1, "Print cube debug info.");
  STRING_VAR_H(outlines_odd, "%| ", "Non standard number of outlines");
  STRING_VAR_H(outlines_2, "ij!?%\":;", "Non standard number of outlines");
  BOOL_VAR_H(docqual_excuse_outline_errs, false,
             "Allow outline errs in unrejection?");
  BOOL_VAR_H(tessedit_good_quality_unrej, true,
             "Reduce rejection on good docs");
  BOOL_VAR_H(tessedit_use_reject_spaces, true, "Reject spaces?");
  double_VAR_H(tessedit_reject_doc_percent, 65.00,
               "%rej allowed before rej whole doc");
  double_VAR_H(tessedit_reject_block_percent, 45.00,
               "%rej allowed before rej whole block");
  double_VAR_H(tessedit_reject_row_percent, 40.00,
               "%rej allowed before rej whole row");
  double_VAR_H(tessedit_whole_wd_rej_row_percent, 70.00,
               "Number of row rejects in whole word rejects"
               "which prevents whole row rejection");
  BOOL_VAR_H(tessedit_preserve_blk_rej_perfect_wds, true,
             "Only rej partially rejected words in block rejection");
  BOOL_VAR_H(tessedit_preserve_row_rej_perfect_wds, true,
             "Only rej partially rejected words in row rejection");
  BOOL_VAR_H(tessedit_dont_blkrej_good_wds, false,
             "Use word segmentation quality metric");
  BOOL_VAR_H(tessedit_dont_rowrej_good_wds, false,
             "Use word segmentation quality metric");
  INT_VAR_H(tessedit_preserve_min_wd_len, 2,
            "Only preserve wds longer than this");
  BOOL_VAR_H(tessedit_row_rej_good_docs, true,
             "Apply row rejection to good docs");
  double_VAR_H(tessedit_good_doc_still_rowrej_wd, 1.1,
               "rej good doc wd if more than this fraction rejected");
  BOOL_VAR_H(tessedit_reject_bad_qual_wds, true,
             "Reject all bad quality wds");
  BOOL_VAR_H(tessedit_debug_doc_rejection, false, "Page stats");
  BOOL_VAR_H(tessedit_debug_quality_metrics, false,
             "Output data to debug file");
  BOOL_VAR_H(bland_unrej, false, "unrej potential with no chekcs");
  double_VAR_H(quality_rowrej_pc, 1.1,
               "good_quality_doc gte good char limit");
  BOOL_VAR_H(unlv_tilde_crunching, true,
             "Mark v.bad words for tilde crunch");
  BOOL_VAR_H(hocr_font_info, false,
             "Add font info to hocr output");
  BOOL_VAR_H(crunch_early_merge_tess_fails, true, "Before word crunch?");
  BOOL_VAR_H(crunch_early_convert_bad_unlv_chs, false, "Take out ~^ early?");
  double_VAR_H(crunch_terrible_rating, 80.0, "crunch rating lt this");
  BOOL_VAR_H(crunch_terrible_garbage, true, "As it says");
  double_VAR_H(crunch_poor_garbage_cert, -9.0,
               "crunch garbage cert lt this");
  double_VAR_H(crunch_poor_garbage_rate, 60, "crunch garbage rating lt this");
  double_VAR_H(crunch_pot_poor_rate, 40, "POTENTIAL crunch rating lt this");
  double_VAR_H(crunch_pot_poor_cert, -8.0, "POTENTIAL crunch cert lt this");
  BOOL_VAR_H(crunch_pot_garbage, true, "POTENTIAL crunch garbage");
  double_VAR_H(crunch_del_rating, 60, "POTENTIAL crunch rating lt this");
  double_VAR_H(crunch_del_cert, -10.0, "POTENTIAL crunch cert lt this");
  double_VAR_H(crunch_del_min_ht, 0.7, "Del if word ht lt xht x this");
  double_VAR_H(crunch_del_max_ht, 3.0, "Del if word ht gt xht x this");
  double_VAR_H(crunch_del_min_width, 3.0, "Del if word width lt xht x this");
  double_VAR_H(crunch_del_high_word, 1.5,
               "Del if word gt xht x this above bl");
  double_VAR_H(crunch_del_low_word, 0.5, "Del if word gt xht x this below bl");
  double_VAR_H(crunch_small_outlines_size, 0.6, "Small if lt xht x this");
  INT_VAR_H(crunch_rating_max, 10, "For adj length in rating per ch");
  INT_VAR_H(crunch_pot_indicators, 1, "How many potential indicators needed");
  BOOL_VAR_H(crunch_leave_ok_strings, true, "Dont touch sensible strings");
  BOOL_VAR_H(crunch_accept_ok, true, "Use acceptability in okstring");
  BOOL_VAR_H(crunch_leave_accept_strings, false,
             "Dont pot crunch sensible strings");
  BOOL_VAR_H(crunch_include_numerals, false, "Fiddle alpha figures");
  INT_VAR_H(crunch_leave_lc_strings, 4,
            "Dont crunch words with long lower case strings");
  INT_VAR_H(crunch_leave_uc_strings, 4,
            "Dont crunch words with long lower case strings");
  INT_VAR_H(crunch_long_repetitions, 3, "Crunch words with long repetitions");
  INT_VAR_H(crunch_debug, 0, "As it says");
  INT_VAR_H(fixsp_non_noise_limit, 1,
            "How many non-noise blbs either side?");
  double_VAR_H(fixsp_small_outlines_size, 0.28, "Small if lt xht x this");
  BOOL_VAR_H(tessedit_prefer_joined_punct, false, "Reward punctation joins");
  INT_VAR_H(fixsp_done_mode, 1, "What constitues done for spacing");
  INT_VAR_H(debug_fix_space_level, 0, "Contextual fixspace debug");
  STRING_VAR_H(numeric_punctuation, ".,",
               "Punct. chs expected WITHIN numbers");
  INT_VAR_H(x_ht_acceptance_tolerance, 8,
            "Max allowed deviation of blob top outside of font data");
  INT_VAR_H(x_ht_min_change, 8, "Min change in xht before actually trying it");
  INT_VAR_H(superscript_debug, 0, "Debug level for sub & superscript fixer");
  double_VAR_H(superscript_worse_certainty, 2.0, "How many times worse "
               "certainty does a superscript position glyph need to be for us "
               "to try classifying it as a char with a different baseline?");
  double_VAR_H(superscript_bettered_certainty, 0.97, "What reduction in "
               "badness do we think sufficient to choose a superscript over "
               "what we'd thought.  For example, a value of 0.6 means we want "
               "to reduce badness of certainty by 40%");
  double_VAR_H(superscript_scaledown_ratio, 0.4,
               "A superscript scaled down more than this is unbelievably "
               "small.  For example, 0.3 means we expect the font size to "
               "be no smaller than 30% of the text line font size.");
  double_VAR_H(subscript_max_y_top, 0.5,
               "Maximum top of a character measured as a multiple of x-height "
               "above the baseline for us to reconsider whether it's a "
               "subscript.");
  double_VAR_H(superscript_min_y_bottom, 0.3,
              "Minimum bottom of a character measured as a multiple of "
              "x-height above the baseline for us to reconsider whether it's "
              "a superscript.");
  BOOL_VAR_H(tessedit_write_block_separators, false,
             "Write block separators in output");
  BOOL_VAR_H(tessedit_write_rep_codes, false,
             "Write repetition char code");
  BOOL_VAR_H(tessedit_write_unlv, false, "Write .unlv output file");
  BOOL_VAR_H(tessedit_create_txt, true, "Write .txt output file");
  BOOL_VAR_H(tessedit_create_hocr, false, "Write .html hOCR output file");
  BOOL_VAR_H(tessedit_create_pdf, false, "Write .pdf output file");
  STRING_VAR_H(unrecognised_char, "|",
               "Output char for unidentified blobs");
  INT_VAR_H(suspect_level, 99, "Suspect marker level");
  INT_VAR_H(suspect_space_level, 100,
            "Min suspect level for rejecting spaces");
  INT_VAR_H(suspect_short_words, 2,
            "Dont Suspect dict wds longer than this");
  BOOL_VAR_H(suspect_constrain_1Il, false, "UNLV keep 1Il chars rejected");
  double_VAR_H(suspect_rating_per_ch, 999.9, "Dont touch bad rating limit");
  double_VAR_H(suspect_accept_rating, -999.9, "Accept good rating limit");
  BOOL_VAR_H(tessedit_minimal_rejection, false, "Only reject tess failures");
  BOOL_VAR_H(tessedit_zero_rejection, false, "Dont reject ANYTHING");
  BOOL_VAR_H(tessedit_word_for_word, false,
             "Make output have exactly one word per WERD");
  BOOL_VAR_H(tessedit_zero_kelvin_rejection, false,
             "Dont reject ANYTHING AT ALL");
  BOOL_VAR_H(tessedit_consistent_reps, true, "Force all rep chars the same");
  INT_VAR_H(tessedit_reject_mode, 0, "Rejection algorithm");
  BOOL_VAR_H(tessedit_rejection_debug, false, "Adaption debug");
  BOOL_VAR_H(tessedit_flip_0O, true, "Contextual 0O O0 flips");
  double_VAR_H(tessedit_lower_flip_hyphen, 1.5,
               "Aspect ratio dot/hyphen test");
  double_VAR_H(tessedit_upper_flip_hyphen, 1.8,
               "Aspect ratio dot/hyphen test");
  BOOL_VAR_H(rej_trust_doc_dawg, false, "Use DOC dawg in 11l conf. detector");
  BOOL_VAR_H(rej_1Il_use_dict_word, false, "Use dictword test");
  BOOL_VAR_H(rej_1Il_trust_permuter_type, true, "Dont double check");
  BOOL_VAR_H(rej_use_tess_accepted, true, "Individual rejection control");
  BOOL_VAR_H(rej_use_tess_blanks, true, "Individual rejection control");
  BOOL_VAR_H(rej_use_good_perm, true, "Individual rejection control");
  BOOL_VAR_H(rej_use_sensible_wd, false, "Extend permuter check");
  BOOL_VAR_H(rej_alphas_in_number_perm, false, "Extend permuter check");
  double_VAR_H(rej_whole_of_mostly_reject_word_fract, 0.85, "if >this fract");
  INT_VAR_H(tessedit_image_border, 2, "Rej blbs near image edge limit");
  STRING_VAR_H(ok_repeated_ch_non_alphanum_wds, "-?*\075",
               "Allow NN to unrej");
  STRING_VAR_H(conflict_set_I_l_1, "Il1[]", "Il1 conflict set");
  INT_VAR_H(min_sane_x_ht_pixels, 8, "Reject any x-ht lt or eq than this");
  BOOL_VAR_H(tessedit_create_boxfile, false, "Output text with boxes");
  INT_VAR_H(tessedit_page_number, -1,
            "-1 -> All pages, else specifc page to process");
  BOOL_VAR_H(tessedit_write_images, false, "Capture the image from the IPE");
  BOOL_VAR_H(interactive_display_mode, false, "Run interactively?");
  STRING_VAR_H(file_type, ".tif", "Filename extension");
  BOOL_VAR_H(tessedit_override_permuter, true, "According to dict_word");
  INT_VAR_H(tessdata_manager_debug_level, 0,
            "Debug level for TessdataManager functions.");
  STRING_VAR_H(tessedit_load_sublangs, "",
               "List of languages to load with this one");
  BOOL_VAR_H(tessedit_use_primary_params_model, false,
             "In multilingual mode use params model of the primary language");
  // Min acceptable orientation margin (difference in scores between top and 2nd
  // choice in OSResults::orientations) to believe the page orientation.
  double_VAR_H(min_orientation_margin, 7.0,
               "Min acceptable orientation margin");
  BOOL_VAR_H(textord_tabfind_show_vlines, false, "Debug line finding");
  BOOL_VAR_H(textord_use_cjk_fp_model, FALSE, "Use CJK fixed pitch model");
  BOOL_VAR_H(poly_allow_detailed_fx, false,
             "Allow feature extractors to see the original outline");
  BOOL_VAR_H(tessedit_init_config_only, false,
             "Only initialize with the config file. Useful if the instance is "
             "not going to be used for OCR but say only for layout analysis.");
  BOOL_VAR_H(textord_equation_detect, false, "Turn on equation detector");
  BOOL_VAR_H(textord_tabfind_vertical_text, true, "Enable vertical detection");
  BOOL_VAR_H(textord_tabfind_force_vertical_text, false,
             "Force using vertical text page mode");
  double_VAR_H(textord_tabfind_vertical_text_ratio, 0.5,
               "Fraction of textlines deemed vertical to use vertical page "
               "mode");
  double_VAR_H(textord_tabfind_aligned_gap_fraction, 0.75,
               "Fraction of height used as a minimum gap for aligned blobs.");
  INT_VAR_H(tessedit_parallelize, 0, "Run in parallel where possible");

  // The following parameters were deprecated and removed from their original
  // locations. The parameters are temporarily kept here to give Tesseract
  // users a chance to updated their [lang].traineddata and config files
  // without introducing failures during Tesseract initialization.
  // TODO(ocr-team): remove these parameters from the code once we are
  // reasonably sure that Tesseract users have updated their data files.
  //
  // BEGIN DEPRECATED PARAMETERS
  BOOL_VAR_H(textord_tabfind_vertical_horizontal_mix, true,
             "find horizontal lines such as headers in vertical page mode");
  INT_VAR_H(tessedit_ok_mode, 5, "Acceptance decision algorithm");
  BOOL_VAR_H(load_fixed_length_dawgs, true,  "Load fixed length"
             " dawgs (e.g. for non-space delimited languages)");
  INT_VAR_H(segment_debug, 0, "Debug the whole segmentation process");
  BOOL_VAR_H(permute_debug, 0, "char permutation debug");
  double_VAR_H(bestrate_pruning_factor, 2.0, "Multiplying factor of"
               " current best rate to prune other hypotheses");
  BOOL_VAR_H(permute_script_word, 0,
             "Turn on word script consistency permuter");
  BOOL_VAR_H(segment_segcost_rating, 0,
             "incorporate segmentation cost in word rating?");
  double_VAR_H(segment_reward_script, 0.95,
               "Score multipler for script consistency within a word. "
               "Being a 'reward' factor, it should be <= 1. "
               "Smaller value implies bigger reward.");
  BOOL_VAR_H(permute_fixed_length_dawg, 0,
             "Turn on fixed-length phrasebook search permuter");
  BOOL_VAR_H(permute_chartype_word, 0,
             "Turn on character type (property) consistency permuter");
  double_VAR_H(segment_reward_chartype, 0.97,
               "Score multipler for char type consistency within a word. ");
  double_VAR_H(segment_reward_ngram_best_choice, 0.99,
               "Score multipler for ngram permuter's best choice"
               " (only used in the Han script path).");
  BOOL_VAR_H(ngram_permuter_activated, false,
             "Activate character-level n-gram-based permuter");
  BOOL_VAR_H(permute_only_top, false, "Run only the top choice permuter");
  INT_VAR_H(language_model_fixed_length_choices_depth, 3,
            "Depth of blob choice lists to explore"
            " when fixed length dawgs are on");
  BOOL_VAR_H(use_new_state_cost, FALSE,
             "use new state cost heuristics for segmentation state evaluation");
  double_VAR_H(heuristic_segcost_rating_base, 1.25,
               "base factor for adding segmentation cost into word rating."
               "It's a multiplying factor, the larger the value above 1, "
               "the bigger the effect of segmentation cost.");
  double_VAR_H(heuristic_weight_rating, 1,
               "weight associated with char rating in combined cost of state");
  double_VAR_H(heuristic_weight_width, 1000.0,
               "weight associated with width evidence in combined cost of"
               " state");
  double_VAR_H(heuristic_weight_seamcut, 0,
               "weight associated with seam cut in combined cost of state");
  double_VAR_H(heuristic_max_char_wh_ratio, 2.0,
               "max char width-to-height ratio allowed in segmentation");
  BOOL_VAR_H(enable_new_segsearch, false,
             "Enable new segmentation search path.");
  double_VAR_H(segsearch_max_fixed_pitch_char_wh_ratio, 2.0,
               "Maximum character width-to-height ratio for"
               "fixed pitch fonts");
  // END DEPRECATED PARAMETERS

  //// ambigsrecog.cpp /////////////////////////////////////////////////////////
  FILE *init_recog_training(const STRING &fname);
  void recog_training_segmented(const STRING &fname,
                                PAGE_RES *page_res,
                                volatile ETEXT_DESC *monitor,
                                FILE *output_file);
  void ambigs_classify_and_output(const char *label,
                                  PAGE_RES_IT* pr_it,
                                  FILE *output_file);

  inline CubeRecoContext *GetCubeRecoContext() { return cube_cntxt_; }

 private:
  // The filename of a backup config file. If not null, then we currently
  // have a temporary debug config file loaded, and backup_config_file_
  // will be loaded, and set to null when debug is complete.
  const char* backup_config_file_;
  // The filename of a config file to read when processing a debug word.
  STRING word_config_;
  // Image used for input to layout analysis and tesseract recognition.
  // May be modified by the ShiroRekhaSplitter to eliminate the top-line.
  Pix* pix_binary_;
  // Unmodified image used for input to cube. Always valid.
  Pix* cube_binary_;
  // Grey-level input image if the input was not binary, otherwise NULL.
  Pix* pix_grey_;
  // Thresholds that were used to generate the thresholded image from grey.
  Pix* pix_thresholds_;
  // Input image resolution after any scaling. The resolution is not well
  // transmitted by operations on Pix, so we keep an independent record here.
  int source_resolution_;
  // The shiro-rekha splitter object which is used to split top-lines in
  // Devanagari words to provide a better word and grapheme segmentation.
  ShiroRekhaSplitter splitter_;
  // Page segmentation/layout
  Textord textord_;
  // True if the primary language uses right_to_left reading order.
  bool right_to_left_;
  Pix* scaled_color_;
  int scaled_factor_;
  FCOORD deskew_;
  FCOORD reskew_;
  TesseractStats stats_;
  // Sub-languages to be tried in addition to this.
  GenericVector<Tesseract*> sub_langs_;
  // Most recently used Tesseract out of this and sub_langs_. The default
  // language for the next word.
  Tesseract* most_recently_used_;
  // The size of the font table, ie max possible font id + 1.
  int font_table_size_;
  // Cube objects.
  CubeRecoContext* cube_cntxt_;
  TesseractCubeCombiner *tess_cube_combiner_;
  // Equation detector. Note: this pointer is NOT owned by the class.
  EquationDetect* equ_detect_;
};

}  // namespace tesseract


#endif  // TESSERACT_CCMAIN_TESSERACTCLASS_H__
