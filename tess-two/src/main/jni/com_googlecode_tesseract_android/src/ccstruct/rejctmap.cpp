/**********************************************************************
 * File:        rejctmap.cpp  (Formerly rejmap.c)
 * Description: REJ and REJMAP class functions.
 * Author:		Phil Cheatle
 * Created:		Thu Jun  9 13:46:38 BST 1994
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

#include          "host.h"
#include          "rejctmap.h"
#include          "params.h"

BOOL8 REJ::perm_rejected() {  //Is char perm reject?
  return (flag (R_TESS_FAILURE) ||
    flag (R_SMALL_XHT) ||
    flag (R_EDGE_CHAR) ||
    flag (R_1IL_CONFLICT) ||
    flag (R_POSTNN_1IL) ||
    flag (R_REJ_CBLOB) ||
    flag (R_BAD_REPETITION) || flag (R_MM_REJECT));
}


BOOL8 REJ::rej_before_nn_accept() {
  return flag (R_POOR_MATCH) ||
    flag (R_NOT_TESS_ACCEPTED) ||
    flag (R_CONTAINS_BLANKS) || flag (R_BAD_PERMUTER);
}


BOOL8 REJ::rej_between_nn_and_mm() {
  return flag (R_HYPHEN) ||
    flag (R_DUBIOUS) ||
    flag (R_NO_ALPHANUMS) || flag (R_MOSTLY_REJ) || flag (R_XHT_FIXUP);
}


BOOL8 REJ::rej_between_mm_and_quality_accept() {
  return flag (R_BAD_QUALITY);
}


BOOL8 REJ::rej_between_quality_and_minimal_rej_accept() {
  return flag (R_DOC_REJ) ||
    flag (R_BLOCK_REJ) || flag (R_ROW_REJ) || flag (R_UNLV_REJ);
}


BOOL8 REJ::rej_before_mm_accept() {
  return rej_between_nn_and_mm () ||
    (rej_before_nn_accept () &&
    !flag (R_NN_ACCEPT) && !flag (R_HYPHEN_ACCEPT));
}


BOOL8 REJ::rej_before_quality_accept() {
  return rej_between_mm_and_quality_accept () ||
    (!flag (R_MM_ACCEPT) && rej_before_mm_accept ());
}


BOOL8 REJ::rejected() {  //Is char rejected?
  if (flag (R_MINIMAL_REJ_ACCEPT))
    return FALSE;
  else
    return (perm_rejected () ||
      rej_between_quality_and_minimal_rej_accept () ||
      (!flag (R_QUALITY_ACCEPT) && rej_before_quality_accept ()));
}


BOOL8 REJ::accept_if_good_quality() {  //potential rej?
  return (rejected () &&
    !perm_rejected () &&
    flag (R_BAD_PERMUTER) &&
    !flag (R_POOR_MATCH) &&
    !flag (R_NOT_TESS_ACCEPTED) &&
    !flag (R_CONTAINS_BLANKS) &&
    (!rej_between_nn_and_mm () &&
     !rej_between_mm_and_quality_accept () &&
     !rej_between_quality_and_minimal_rej_accept ()));
}


void REJ::setrej_tess_failure() {  //Tess generated blank
  set_flag(R_TESS_FAILURE);
}


void REJ::setrej_small_xht() {  //Small xht char/wd
  set_flag(R_SMALL_XHT);
}


void REJ::setrej_edge_char() {  //Close to image edge
  set_flag(R_EDGE_CHAR);
}


void REJ::setrej_1Il_conflict() {  //Initial reject map
  set_flag(R_1IL_CONFLICT);
}


void REJ::setrej_postNN_1Il() {  //1Il after NN
  set_flag(R_POSTNN_1IL);
}


void REJ::setrej_rej_cblob() {  //Insert duff blob
  set_flag(R_REJ_CBLOB);
}


void REJ::setrej_mm_reject() {  //Matrix matcher
  set_flag(R_MM_REJECT);
}


void REJ::setrej_bad_repetition() {  //Odd repeated char
  set_flag(R_BAD_REPETITION);
}


void REJ::setrej_poor_match() {  //Failed Rays heuristic
  set_flag(R_POOR_MATCH);
}


void REJ::setrej_not_tess_accepted() {
                                 //TEMP reject_word
  set_flag(R_NOT_TESS_ACCEPTED);
}


void REJ::setrej_contains_blanks() {
                                 //TEMP reject_word
  set_flag(R_CONTAINS_BLANKS);
}


void REJ::setrej_bad_permuter() {  //POTENTIAL reject_word
  set_flag(R_BAD_PERMUTER);
}


void REJ::setrej_hyphen() {  //PostNN dubious hyphen or .
  set_flag(R_HYPHEN);
}


void REJ::setrej_dubious() {  //PostNN dubious limit
  set_flag(R_DUBIOUS);
}


void REJ::setrej_no_alphanums() {  //TEMP reject_word
  set_flag(R_NO_ALPHANUMS);
}


void REJ::setrej_mostly_rej() {  //TEMP reject_word
  set_flag(R_MOSTLY_REJ);
}


void REJ::setrej_xht_fixup() {  //xht fixup
  set_flag(R_XHT_FIXUP);
}


void REJ::setrej_bad_quality() {  //TEMP reject_word
  set_flag(R_BAD_QUALITY);
}


void REJ::setrej_doc_rej() {  //TEMP reject_word
  set_flag(R_DOC_REJ);
}


void REJ::setrej_block_rej() {  //TEMP reject_word
  set_flag(R_BLOCK_REJ);
}


void REJ::setrej_row_rej() {  //TEMP reject_word
  set_flag(R_ROW_REJ);
}


void REJ::setrej_unlv_rej() {  //TEMP reject_word
  set_flag(R_UNLV_REJ);
}


void REJ::setrej_hyphen_accept() {  //NN Flipped a char
  set_flag(R_HYPHEN_ACCEPT);
}


void REJ::setrej_nn_accept() {  //NN Flipped a char
  set_flag(R_NN_ACCEPT);
}


void REJ::setrej_mm_accept() {  //Matrix matcher
  set_flag(R_MM_ACCEPT);
}


void REJ::setrej_quality_accept() {  //Quality flip a char
  set_flag(R_QUALITY_ACCEPT);
}


void REJ::setrej_minimal_rej_accept() {
                                 //Accept all except blank
  set_flag(R_MINIMAL_REJ_ACCEPT);
}


void REJ::full_print(FILE *fp) {
  fprintf (fp, "R_TESS_FAILURE: %s\n", flag (R_TESS_FAILURE) ? "T" : "F");
  fprintf (fp, "R_SMALL_XHT: %s\n", flag (R_SMALL_XHT) ? "T" : "F");
  fprintf (fp, "R_EDGE_CHAR: %s\n", flag (R_EDGE_CHAR) ? "T" : "F");
  fprintf (fp, "R_1IL_CONFLICT: %s\n", flag (R_1IL_CONFLICT) ? "T" : "F");
  fprintf (fp, "R_POSTNN_1IL: %s\n", flag (R_POSTNN_1IL) ? "T" : "F");
  fprintf (fp, "R_REJ_CBLOB: %s\n", flag (R_REJ_CBLOB) ? "T" : "F");
  fprintf (fp, "R_MM_REJECT: %s\n", flag (R_MM_REJECT) ? "T" : "F");
  fprintf (fp, "R_BAD_REPETITION: %s\n", flag (R_BAD_REPETITION) ? "T" : "F");
  fprintf (fp, "R_POOR_MATCH: %s\n", flag (R_POOR_MATCH) ? "T" : "F");
  fprintf (fp, "R_NOT_TESS_ACCEPTED: %s\n",
    flag (R_NOT_TESS_ACCEPTED) ? "T" : "F");
  fprintf (fp, "R_CONTAINS_BLANKS: %s\n",
    flag (R_CONTAINS_BLANKS) ? "T" : "F");
  fprintf (fp, "R_BAD_PERMUTER: %s\n", flag (R_BAD_PERMUTER) ? "T" : "F");
  fprintf (fp, "R_HYPHEN: %s\n", flag (R_HYPHEN) ? "T" : "F");
  fprintf (fp, "R_DUBIOUS: %s\n", flag (R_DUBIOUS) ? "T" : "F");
  fprintf (fp, "R_NO_ALPHANUMS: %s\n", flag (R_NO_ALPHANUMS) ? "T" : "F");
  fprintf (fp, "R_MOSTLY_REJ: %s\n", flag (R_MOSTLY_REJ) ? "T" : "F");
  fprintf (fp, "R_XHT_FIXUP: %s\n", flag (R_XHT_FIXUP) ? "T" : "F");
  fprintf (fp, "R_BAD_QUALITY: %s\n", flag (R_BAD_QUALITY) ? "T" : "F");
  fprintf (fp, "R_DOC_REJ: %s\n", flag (R_DOC_REJ) ? "T" : "F");
  fprintf (fp, "R_BLOCK_REJ: %s\n", flag (R_BLOCK_REJ) ? "T" : "F");
  fprintf (fp, "R_ROW_REJ: %s\n", flag (R_ROW_REJ) ? "T" : "F");
  fprintf (fp, "R_UNLV_REJ: %s\n", flag (R_UNLV_REJ) ? "T" : "F");
  fprintf (fp, "R_HYPHEN_ACCEPT: %s\n", flag (R_HYPHEN_ACCEPT) ? "T" : "F");
  fprintf (fp, "R_NN_ACCEPT: %s\n", flag (R_NN_ACCEPT) ? "T" : "F");
  fprintf (fp, "R_MM_ACCEPT: %s\n", flag (R_MM_ACCEPT) ? "T" : "F");
  fprintf (fp, "R_QUALITY_ACCEPT: %s\n", flag (R_QUALITY_ACCEPT) ? "T" : "F");
  fprintf (fp, "R_MINIMAL_REJ_ACCEPT: %s\n",
    flag (R_MINIMAL_REJ_ACCEPT) ? "T" : "F");
}


//The REJMAP class has been hacked to use alloc_struct instead of new [].
//This is to reduce memory fragmentation only as it is rather kludgy.
//alloc_struct by-passes the call to the contsructor of REJ on each
//array element. Although the constructor is empty, the BITS16 members
//do have a constructor which sets all the flags to 0. The memset
//replaces this functionality.

REJMAP::REJMAP(  //classwise copy
               const REJMAP &source) {
  REJ *to;
  REJ *from = source.ptr;
  int i;

  len = source.length ();

  if (len > 0) {
    ptr = (REJ *) alloc_struct (len * sizeof (REJ), "REJ");
    to = ptr;
    for (i = 0; i < len; i++) {
      *to = *from;
      to++;
      from++;
    }
  }
  else
    ptr = NULL;
}


REJMAP & REJMAP::operator= (     //assign REJMAP
const REJMAP & source            //from this
) {
  REJ *
    to;
  REJ *
    from = source.ptr;
  int
    i;

  initialise (source.len);
  to = ptr;
  for (i = 0; i < len; i++) {
    *to = *from;
    to++;
    from++;
  }
  return *this;
}


void REJMAP::initialise(  //Redefine map
                        inT16 length) {
  if (ptr != NULL)
    free_struct (ptr, len * sizeof (REJ), "REJ");
  len = length;
  if (len > 0)
    ptr = (REJ *) memset (alloc_struct (len * sizeof (REJ), "REJ"),
      0, len * sizeof (REJ));
  else
    ptr = NULL;
}


inT16 REJMAP::accept_count() {  //How many accepted?
  int i;
  inT16 count = 0;

  for (i = 0; i < len; i++) {
    if (ptr[i].accepted ())
      count++;
  }
  return count;
}


BOOL8 REJMAP::recoverable_rejects() {  //Any non perm rejs?
  int i;

  for (i = 0; i < len; i++) {
    if (ptr[i].recoverable ())
      return TRUE;
  }
  return FALSE;
}


BOOL8 REJMAP::quality_recoverable_rejects() {  //Any potential rejs?
  int i;

  for (i = 0; i < len; i++) {
    if (ptr[i].accept_if_good_quality ())
      return TRUE;
  }
  return FALSE;
}


void REJMAP::remove_pos(           //Cut out an element
                        inT16 pos  //element to remove
                       ) {
  REJ *new_ptr;                  //new, smaller map
  int i;

  ASSERT_HOST (pos >= 0);
  ASSERT_HOST (pos < len);
  ASSERT_HOST (len > 0);

  len--;
  if (len > 0)
    new_ptr = (REJ *) memset (alloc_struct (len * sizeof (REJ), "REJ"),
      0, len * sizeof (REJ));
  else
    new_ptr = NULL;

  for (i = 0; i < pos; i++)
    new_ptr[i] = ptr[i];         //copy pre pos

  for (; pos < len; pos++)
    new_ptr[pos] = ptr[pos + 1]; //copy post pos

                                 //delete old map
  free_struct (ptr, (len + 1) * sizeof (REJ), "REJ");
  ptr = new_ptr;
}


void REJMAP::print(FILE *fp) {
  int i;
  char buff[512];

  for (i = 0; i < len; i++) {
    buff[i] = ptr[i].display_char ();
  }
  buff[i] = '\0';
  fprintf (fp, "\"%s\"", buff);
}


void REJMAP::full_print(FILE *fp) {
  int i;

  for (i = 0; i < len; i++) {
    ptr[i].full_print (fp);
    fprintf (fp, "\n");
  }
}


void REJMAP::rej_word_small_xht() {  //Reject whole word
  int i;

  for (i = 0; i < len; i++) {
    ptr[i].setrej_small_xht ();
  }
}


void REJMAP::rej_word_tess_failure() {  //Reject whole word
  int i;

  for (i = 0; i < len; i++) {
    ptr[i].setrej_tess_failure ();
  }
}


void REJMAP::rej_word_not_tess_accepted() {  //Reject whole word
  int i;

  for (i = 0; i < len; i++) {
    if (ptr[i].accepted()) ptr[i].setrej_not_tess_accepted();
  }
}


void REJMAP::rej_word_contains_blanks() {  //Reject whole word
  int i;

  for (i = 0; i < len; i++) {
    if (ptr[i].accepted()) ptr[i].setrej_contains_blanks();
  }
}


void REJMAP::rej_word_bad_permuter() {  //Reject whole word
  int i;

  for (i = 0; i < len; i++) {
    if (ptr[i].accepted()) ptr[i].setrej_bad_permuter ();
  }
}


void REJMAP::rej_word_xht_fixup() {  //Reject whole word
  int i;

  for (i = 0; i < len; i++) {
    if (ptr[i].accepted()) ptr[i].setrej_xht_fixup();
  }
}


void REJMAP::rej_word_no_alphanums() {  //Reject whole word
  int i;

  for (i = 0; i < len; i++) {
    if (ptr[i].accepted()) ptr[i].setrej_no_alphanums();
  }
}


void REJMAP::rej_word_mostly_rej() {  //Reject whole word
  int i;

  for (i = 0; i < len; i++) {
    if (ptr[i].accepted()) ptr[i].setrej_mostly_rej();
  }
}


void REJMAP::rej_word_bad_quality() {  //Reject whole word
  int i;

  for (i = 0; i < len; i++) {
    if (ptr[i].accepted()) ptr[i].setrej_bad_quality();
  }
}


void REJMAP::rej_word_doc_rej() {  //Reject whole word
  int i;

  for (i = 0; i < len; i++) {
    if (ptr[i].accepted()) ptr[i].setrej_doc_rej();
  }
}


void REJMAP::rej_word_block_rej() {  //Reject whole word
  int i;

  for (i = 0; i < len; i++) {
    if (ptr[i].accepted()) ptr[i].setrej_block_rej();
  }
}


void REJMAP::rej_word_row_rej() {  //Reject whole word
  int i;

  for (i = 0; i < len; i++) {
    if (ptr[i].accepted()) ptr[i].setrej_row_rej();
  }
}
