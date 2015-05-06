/******************************************************************
 * File:        output.cpp  (Formerly output.c)
 * Description: Output pass
 * Author:					Phil Cheatle
 * Created:					Thu Aug  4 10:56:08 BST 1994
 *
 * (C) Copyright 1994, Hewlett-Packard Ltd.
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

#ifdef _MSC_VER
#pragma warning(disable:4244)  // Conversion warnings
#endif

#include <string.h>
#include <ctype.h>
#ifdef __UNIX__
#include          <assert.h>
#include          <unistd.h>
#include          <errno.h>
#endif
#include "helpers.h"
#include "tessvars.h"
#include "control.h"
#include "reject.h"
#include "docqual.h"
#include "output.h"
#include "globals.h"
#include "tesseractclass.h"

#define EPAPER_EXT      ".ep"
#define PAGE_YSIZE      3508
#define CTRL_INSET      '\024'   //dc4=text inset
#define CTRL_FONT       '\016'   //so=font change
#define CTRL_DEFAULT      '\017' //si=default font
#define CTRL_SHIFT      '\022'   //dc2=x shift
#define CTRL_TAB        '\011'   //tab
#define CTRL_NEWLINE      '\012' //newline
#define CTRL_HARDLINE   '\015'   //cr

/**********************************************************************
 * pixels_to_pts
 *
 * Convert an integer number of pixels to the nearest integer
 * number of points.
 **********************************************************************/

inT32 pixels_to_pts(               //convert coords
                    inT32 pixels,
                    inT32 pix_res  //resolution
                   ) {
  float pts;                     //converted value

  pts = pixels * 72.0 / pix_res;
  return (inT32) (pts + 0.5);    //round it
}

namespace tesseract {
void Tesseract::output_pass(  //Tess output pass //send to api
                            PAGE_RES_IT &page_res_it,
                            const TBOX *target_word_box) {
  BLOCK_RES *block_of_last_word;
  BOOL8 force_eol;               //During output
  BLOCK *nextblock;              //block of next word
  WERD *nextword;                //next word

  page_res_it.restart_page ();
  block_of_last_word = NULL;
  while (page_res_it.word () != NULL) {
    check_debug_pt (page_res_it.word (), 120);

	if (target_word_box)
	{

		TBOX current_word_box=page_res_it.word ()->word->bounding_box();
		FCOORD center_pt((current_word_box.right()+current_word_box.left())/2,(current_word_box.bottom()+current_word_box.top())/2);
		if (!target_word_box->contains(center_pt))
		{
			page_res_it.forward ();
			continue;
		}

	}
    if (tessedit_write_block_separators &&
    block_of_last_word != page_res_it.block ()) {
      block_of_last_word = page_res_it.block ();
    }

    force_eol = (tessedit_write_block_separators &&
      (page_res_it.block () != page_res_it.next_block ())) ||
      (page_res_it.next_word () == NULL);

    if (page_res_it.next_word () != NULL)
      nextword = page_res_it.next_word ()->word;
    else
      nextword = NULL;
    if (page_res_it.next_block () != NULL)
      nextblock = page_res_it.next_block ()->block;
    else
      nextblock = NULL;
                                 //regardless of tilde crunching
    write_results(page_res_it,
                  determine_newline_type(page_res_it.word()->word,
                                         page_res_it.block()->block,
                                         nextword, nextblock), force_eol);
    page_res_it.forward();
  }
}


/*************************************************************************
 * write_results()
 *
 * All recognition and rejection has now been done. Generate the following:
 *   .txt file     - giving the final best choices with NO highlighting
 *   .raw file     - giving the tesseract top choice output for each word
 *   .map file     - showing how the .txt file has been rejected in the .ep file
 *   epchoice list - a list of one element per word, containing the text for the
 *                   epaper. Reject strings are inserted.
 *   inset list    - a list of bounding boxes of reject insets - indexed by the
 *                   reject strings in the epchoice text.
 *************************************************************************/
void Tesseract::write_results(PAGE_RES_IT &page_res_it,
                              char newline_type,  // type of newline
                              BOOL8 force_eol) {  // override tilde crunch?
  WERD_RES *word = page_res_it.word();
  const UNICHARSET &uchset = *word->uch_set;
  int i;
  BOOL8 need_reject = FALSE;
  UNICHAR_ID space = uchset.unichar_to_id(" ");

  if ((word->unlv_crunch_mode != CR_NONE ||
       word->best_choice->length() == 0) &&
      !tessedit_zero_kelvin_rejection && !tessedit_word_for_word) {
    if ((word->unlv_crunch_mode != CR_DELETE) &&
        (!stats_.tilde_crunch_written ||
         ((word->unlv_crunch_mode == CR_KEEP_SPACE) &&
          (word->word->space () > 0) &&
          !word->word->flag (W_FUZZY_NON) &&
          !word->word->flag (W_FUZZY_SP)))) {
      if (!word->word->flag (W_BOL) &&
          (word->word->space () > 0) &&
          !word->word->flag (W_FUZZY_NON) &&
          !word->word->flag (W_FUZZY_SP)) {
        stats_.last_char_was_tilde = false;
      }
      need_reject = TRUE;
    }
    if ((need_reject && !stats_.last_char_was_tilde) ||
        (force_eol && stats_.write_results_empty_block)) {
      /* Write a reject char - mark as rejected unless zero_rejection mode */
      stats_.last_char_was_tilde = TRUE;
      stats_.tilde_crunch_written = true;
      stats_.last_char_was_newline = false;
      stats_.write_results_empty_block = false;
    }

    if ((word->word->flag (W_EOL) && !stats_.last_char_was_newline) || force_eol) {
      stats_.tilde_crunch_written = false;
      stats_.last_char_was_newline = true;
      stats_.last_char_was_tilde = false;
    }

    if (force_eol)
      stats_.write_results_empty_block = true;
    return;
  }

  /* NORMAL PROCESSING of non tilde crunched words */

  stats_.tilde_crunch_written = false;
  if (newline_type)
    stats_.last_char_was_newline = true;
  else
    stats_.last_char_was_newline = false;
  stats_.write_results_empty_block = force_eol;  // about to write a real word

  if (unlv_tilde_crunching &&
      stats_.last_char_was_tilde &&
      (word->word->space() == 0) &&
      !(word->word->flag(W_REP_CHAR) && tessedit_write_rep_codes) &&
      (word->best_choice->unichar_id(0) == space)) {
    /* Prevent adjacent tilde across words - we know that adjacent tildes within
       words have been removed */
    word->MergeAdjacentBlobs(0);
  }
  if (newline_type ||
    (word->word->flag (W_REP_CHAR) && tessedit_write_rep_codes))
    stats_.last_char_was_tilde = false;
  else {
    if (word->reject_map.length () > 0) {
      if (word->best_choice->unichar_id(word->reject_map.length() - 1) == space)
        stats_.last_char_was_tilde = true;
      else
        stats_.last_char_was_tilde = false;
    }
    else if (word->word->space () > 0)
      stats_.last_char_was_tilde = false;
    /* else it is unchanged as there are no output chars */
  }

  ASSERT_HOST (word->best_choice->length() == word->reject_map.length());

  set_unlv_suspects(word);
  check_debug_pt (word, 120);
  if (tessedit_rejection_debug) {
    tprintf ("Dict word: \"%s\": %d\n",
             word->best_choice->debug_string().string(),
             dict_word(*(word->best_choice)));
  }
  if (!word->word->flag(W_REP_CHAR) || !tessedit_write_rep_codes) {
    if (tessedit_zero_rejection) {
      /* OVERRIDE ALL REJECTION MECHANISMS - ONLY REJECT TESS FAILURES */
      for (i = 0; i < word->best_choice->length(); ++i) {
        if (word->reject_map[i].rejected())
          word->reject_map[i].setrej_minimal_rej_accept();
      }
    }
    if (tessedit_minimal_rejection) {
      /* OVERRIDE ALL REJECTION MECHANISMS - ONLY REJECT TESS FAILURES */
      for (i = 0; i < word->best_choice->length(); ++i) {
        if ((word->best_choice->unichar_id(i) != space) &&
            word->reject_map[i].rejected())
          word->reject_map[i].setrej_minimal_rej_accept();
      }
    }
  }
}
}  // namespace tesseract

/**********************************************************************
 * determine_newline_type
 *
 * Find whether we have a wrapping or hard newline.
 * Return FALSE if not at end of line.
 **********************************************************************/

char determine_newline_type(                   //test line ends
                            WERD *word,        //word to do
                            BLOCK *block,      //current block
                            WERD *next_word,   //next word
                            BLOCK *next_block  //block of next word
                           ) {
  inT16 end_gap;                 //to right edge
  inT16 width;                   //of next word
  TBOX word_box;                  //bounding
  TBOX next_box;                  //next word
  TBOX block_box;                 //block bounding

  if (!word->flag (W_EOL))
    return FALSE;                //not end of line
  if (next_word == NULL || next_block == NULL || block != next_block)
    return CTRL_NEWLINE;
  if (next_word->space () > 0)
    return CTRL_HARDLINE;        //it is tabbed
  word_box = word->bounding_box ();
  next_box = next_word->bounding_box ();
  block_box = block->bounding_box ();
                                 //gap to eol
  end_gap = block_box.right () - word_box.right ();
  end_gap -= (inT32) block->space ();
  width = next_box.right () - next_box.left ();
  //      tprintf("end_gap=%d-%d=%d, width=%d-%d=%d, nl=%d\n",
  //              block_box.right(),word_box.right(),end_gap,
  //              next_box.right(),next_box.left(),width,
  //              end_gap>width ? CTRL_HARDLINE : CTRL_NEWLINE);
  return end_gap > width ? CTRL_HARDLINE : CTRL_NEWLINE;
}

/*************************************************************************
 * get_rep_char()
 * Return the first accepted character from the repetition string. This is the
 * character which is repeated - as determined earlier by fix_rep_char()
 *************************************************************************/
namespace tesseract {
UNICHAR_ID Tesseract::get_rep_char(WERD_RES *word) {  // what char is repeated?
  int i;
  for (i = 0; ((i < word->reject_map.length()) &&
               (word->reject_map[i].rejected())); ++i);

  if (i < word->reject_map.length()) {
    return word->best_choice->unichar_id(i);
  } else {
    return word->uch_set->unichar_to_id(unrecognised_char.string());
  }
}

/*************************************************************************
 * SUSPECT LEVELS
 *
 * 0 - dont reject ANYTHING
 * 1,2 - partial rejection
 * 3 - BEST
 *
 * NOTE: to reject JUST tess failures in the .map file set suspect_level 3 and
 * tessedit_minimal_rejection.
 *************************************************************************/
void Tesseract::set_unlv_suspects(WERD_RES *word_res) {
  int len = word_res->reject_map.length();
  const WERD_CHOICE &word = *(word_res->best_choice);
  const UNICHARSET &uchset = *word.unicharset();
  int i;
  float rating_per_ch;

  if (suspect_level == 0) {
    for (i = 0; i < len; i++) {
      if (word_res->reject_map[i].rejected())
        word_res->reject_map[i].setrej_minimal_rej_accept();
    }
    return;
  }

  if (suspect_level >= 3)
    return;                      //Use defaults

  /* NOW FOR LEVELS 1 and 2 Find some stuff to unreject*/

  if (safe_dict_word(word_res) &&
      (count_alphas(word) > suspect_short_words)) {
    /* Unreject alphas in dictionary words */
    for (i = 0; i < len; ++i) {
      if (word_res->reject_map[i].rejected() &&
          uchset.get_isalpha(word.unichar_id(i)))
        word_res->reject_map[i].setrej_minimal_rej_accept();
    }
  }

  rating_per_ch = word.rating() / word_res->reject_map.length();

  if (rating_per_ch >= suspect_rating_per_ch)
    return;                      //Dont touch bad ratings

  if ((word_res->tess_accepted) || (rating_per_ch < suspect_accept_rating)) {
    /* Unreject any Tess Acceptable word - but NOT tess reject chs*/
    for (i = 0; i < len; ++i) {
      if (word_res->reject_map[i].rejected() &&
          (!uchset.eq(word.unichar_id(i), " ")))
        word_res->reject_map[i].setrej_minimal_rej_accept();
    }
  }

  for (i = 0; i < len; i++) {
    if (word_res->reject_map[i].rejected()) {
      if (word_res->reject_map[i].flag(R_DOC_REJ))
        word_res->reject_map[i].setrej_minimal_rej_accept();
      if (word_res->reject_map[i].flag(R_BLOCK_REJ))
        word_res->reject_map[i].setrej_minimal_rej_accept();
      if (word_res->reject_map[i].flag(R_ROW_REJ))
        word_res->reject_map[i].setrej_minimal_rej_accept();
    }
  }

  if (suspect_level == 2)
    return;

  if (!suspect_constrain_1Il ||
      (word_res->reject_map.length() <= suspect_short_words)) {
    for (i = 0; i < len; i++) {
      if (word_res->reject_map[i].rejected()) {
        if ((word_res->reject_map[i].flag(R_1IL_CONFLICT) ||
          word_res->reject_map[i].flag(R_POSTNN_1IL)))
          word_res->reject_map[i].setrej_minimal_rej_accept();

        if (!suspect_constrain_1Il &&
          word_res->reject_map[i].flag(R_MM_REJECT))
          word_res->reject_map[i].setrej_minimal_rej_accept();
      }
    }
  }

  if (acceptable_word_string(*word_res->uch_set,
                             word.unichar_string().string(),
                             word.unichar_lengths().string()) !=
                                 AC_UNACCEPTABLE ||
      acceptable_number_string(word.unichar_string().string(),
                               word.unichar_lengths().string())) {
    if (word_res->reject_map.length() > suspect_short_words) {
      for (i = 0; i < len; i++) {
        if (word_res->reject_map[i].rejected() &&
          (!word_res->reject_map[i].perm_rejected() ||
           word_res->reject_map[i].flag (R_1IL_CONFLICT) ||
           word_res->reject_map[i].flag (R_POSTNN_1IL) ||
           word_res->reject_map[i].flag (R_MM_REJECT))) {
          word_res->reject_map[i].setrej_minimal_rej_accept();
        }
      }
    }
  }
}

inT16 Tesseract::count_alphas(const WERD_CHOICE &word) {
  int count = 0;
  for (int i = 0; i < word.length(); ++i) {
    if (word.unicharset()->get_isalpha(word.unichar_id(i)))
      count++;
  }
  return count;
}


inT16 Tesseract::count_alphanums(const WERD_CHOICE &word) {
  int count = 0;
  for (int i = 0; i < word.length(); ++i) {
    if (word.unicharset()->get_isalpha(word.unichar_id(i)) ||
        word.unicharset()->get_isdigit(word.unichar_id(i)))
      count++;
  }
  return count;
}


BOOL8 Tesseract::acceptable_number_string(const char *s,
                                          const char *lengths) {
  BOOL8 prev_digit = FALSE;

  if (*lengths == 1 && *s == '(')
    s++;

  if (*lengths == 1 &&
      ((*s == '$') || (*s == '.') || (*s == '+') || (*s == '-')))
    s++;

  for (; *s != '\0'; s += *(lengths++)) {
    if (unicharset.get_isdigit(s, *lengths))
      prev_digit = TRUE;
    else if (prev_digit &&
             (*lengths == 1 && ((*s == '.') || (*s == ',') || (*s == '-'))))
      prev_digit = FALSE;
    else if (prev_digit && *lengths == 1 &&
             (*(s + *lengths) == '\0') && ((*s == '%') || (*s == ')')))
      return TRUE;
    else if (prev_digit &&
             *lengths == 1 && (*s == '%') &&
             (*(lengths + 1) == 1 && *(s + *lengths) == ')') &&
             (*(s + *lengths + *(lengths + 1)) == '\0'))
      return TRUE;
    else
      return FALSE;
  }
  return TRUE;
}
}  // namespace tesseract
