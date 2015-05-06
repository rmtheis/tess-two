/**********************************************************************
 * File:        elst.c  (Formerly elist.c)
 * Description: Embedded list handling code which is not in the include file.
 * Author:      Phil Cheatle
 * Created:     Fri Jan 04 13:55:49 GMT 1991
 *
 * (C) Copyright 1991, Hewlett-Packard Ltd.
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

#include <stdlib.h>
#include "elst.h"

/***********************************************************************
 *  MEMBER FUNCTIONS OF CLASS: ELIST
 *  ================================
 **********************************************************************/

/***********************************************************************
 *							ELIST::internal_clear
 *
 *  Used by the destructor and the "clear" member function of derived list
 *  classes to destroy all the elements on the list.
 *  The calling function passes a "zapper" function which can be called to
 *  delete each element of the list, regardless of its derived type.  This
 *  technique permits a generic clear function to destroy elements of
 *  different derived types correctly, without requiring virtual functions and
 *  the consequential memory overhead.
 **********************************************************************/

void
ELIST::internal_clear (          //destroy all links
void (*zapper) (ELIST_LINK *)) {
                                 //ptr to zapper functn
  ELIST_LINK *ptr;
  ELIST_LINK *next;

  #ifndef NDEBUG
  if (!this)
    NULL_OBJECT.error ("ELIST::internal_clear", ABORT, NULL);
  #endif

  if (!empty ()) {
    ptr = last->next;            //set to first
    last->next = NULL;           //break circle
    last = NULL;                 //set list empty
    while (ptr) {
      next = ptr->next;
      zapper(ptr);
      ptr = next;
    }
  }
}

/***********************************************************************
 *							ELIST::assign_to_sublist
 *
 *  The list is set to a sublist of another list.  "This" list must be empty
 *  before this function is invoked.  The two iterators passed must refer to
 *  the same list, different from "this" one.  The sublist removed is the
 *  inclusive list from start_it's current position to end_it's current
 *  position.  If this range passes over the end of the source list then the
 *  source list has its end set to the previous element of start_it.  The
 *  extracted sublist is unaffected by the end point of the source list, its
 *  end point is always the end_it position.
 **********************************************************************/

void ELIST::assign_to_sublist(                           //to this list
                              ELIST_ITERATOR *start_it,  //from list start
                              ELIST_ITERATOR *end_it) {  //from list end
  const ERRCODE LIST_NOT_EMPTY =
    "Destination list must be empty before extracting a sublist";

  #ifndef NDEBUG
  if (!this)
    NULL_OBJECT.error ("ELIST::assign_to_sublist", ABORT, NULL);
  #endif

  if (!empty ())
    LIST_NOT_EMPTY.error ("ELIST.assign_to_sublist", ABORT, NULL);

  last = start_it->extract_sublist (end_it);
}


/***********************************************************************
 *							ELIST::length
 *
 *  Return count of elements on list
 **********************************************************************/

inT32 ELIST::length() const {  // count elements
  ELIST_ITERATOR it(const_cast<ELIST*>(this));
  inT32 count = 0;

  #ifndef NDEBUG
  if (!this)
    NULL_OBJECT.error ("ELIST::length", ABORT, NULL);
  #endif

  for (it.mark_cycle_pt (); !it.cycled_list (); it.forward ())
    count++;
  return count;
}


/***********************************************************************
 *							ELIST::sort
 *
 *  Sort elements on list
 *  NB If you dont like the const declarations in the comparator, coerce yours:
 *   ( int (*)(const void *, const void *)
 **********************************************************************/

void
ELIST::sort (                    //sort elements
int comparator (                 //comparison routine
const void *, const void *)) {
  ELIST_ITERATOR it(this);
  inT32 count;
  ELIST_LINK **base;             //ptr array to sort
  ELIST_LINK **current;
  inT32 i;

  #ifndef NDEBUG
  if (!this)
    NULL_OBJECT.error ("ELIST::sort", ABORT, NULL);
  #endif

  /* Allocate an array of pointers, one per list element */
  count = length ();
  base = (ELIST_LINK **) malloc (count * sizeof (ELIST_LINK *));

  /* Extract all elements, putting the pointers in the array */
  current = base;
  for (it.mark_cycle_pt (); !it.cycled_list (); it.forward ()) {
    *current = it.extract ();
    current++;
  }

  /* Sort the pointer array */
  qsort ((char *) base, count, sizeof (*base), comparator);

  /* Rebuild the list from the sorted pointers */
  current = base;
  for (i = 0; i < count; i++) {
    it.add_to_end (*current);
    current++;
  }
  free(base);
}

// Assuming list has been sorted already, insert new_link to
// keep the list sorted according to the same comparison function.
// Comparision function is the same as used by sort, i.e. uses double
// indirection. Time is O(1) to add to beginning or end.
// Time is linear to add pre-sorted items to an empty list.
// If unique is set to true and comparator() returns 0 (an entry with the
// same information as the one contained in new_link is already in the
// list) - new_link is not added to the list and the function returns the
// pointer to the identical entry that already exists in the list
// (otherwise the function returns new_link).
ELIST_LINK *ELIST::add_sorted_and_find(
    int comparator(const void*, const void*),
    bool unique, ELIST_LINK* new_link) {
  // Check for adding at the end.
  if (last == NULL || comparator(&last, &new_link) < 0) {
    if (last == NULL) {
      new_link->next = new_link;
    } else {
      new_link->next = last->next;
      last->next = new_link;
    }
    last = new_link;
  } else {
    // Need to use an iterator.
    ELIST_ITERATOR it(this);
    for (it.mark_cycle_pt(); !it.cycled_list(); it.forward()) {
      ELIST_LINK* link = it.data();
      int compare = comparator(&link, &new_link);
      if (compare > 0) {
        break;
      } else if (unique && compare == 0) {
        return link;
      }
    }
    if (it.cycled_list())
      it.add_to_end(new_link);
    else
      it.add_before_then_move(new_link);
  }
  return new_link;
}

/***********************************************************************
 *  MEMBER FUNCTIONS OF CLASS: ELIST_ITERATOR
 *  =========================================
 **********************************************************************/

/***********************************************************************
 *							ELIST_ITERATOR::forward
 *
 *  Move the iterator to the next element of the list.
 *  REMEMBER: ALL LISTS ARE CIRCULAR.
 **********************************************************************/

ELIST_LINK *ELIST_ITERATOR::forward() {
  #ifndef NDEBUG
  if (!this)
    NULL_OBJECT.error ("ELIST_ITERATOR::forward", ABORT, NULL);
  if (!list)
    NO_LIST.error ("ELIST_ITERATOR::forward", ABORT, NULL);
  #endif
  if (list->empty ())
    return NULL;

  if (current) {                 //not removed so
                                 //set previous
    prev = current;
    started_cycling = TRUE;
    // In case next is deleted by another iterator, get next from current.
    current = current->next;
  } else {
    if (ex_current_was_cycle_pt)
      cycle_pt = next;
    current = next;
  }
  next = current->next;

  #ifndef NDEBUG
  if (!current)
    NULL_DATA.error ("ELIST_ITERATOR::forward", ABORT, NULL);
  if (!next)
    NULL_NEXT.error ("ELIST_ITERATOR::forward", ABORT,
                     "This is: %p  Current is: %p", this, current);
  #endif
  return current;
}


/***********************************************************************
 *							ELIST_ITERATOR::data_relative
 *
 *  Return the data pointer to the element "offset" elements from current.
 *  "offset" must not be less than -1.
 *  (This function can't be INLINEd because it contains a loop)
 **********************************************************************/

ELIST_LINK *ELIST_ITERATOR::data_relative(                //get data + or - ...
                                          inT8 offset) {  //offset from current
  ELIST_LINK *ptr;

  #ifndef NDEBUG
  if (!this)
    NULL_OBJECT.error ("ELIST_ITERATOR::data_relative", ABORT, NULL);
  if (!list)
    NO_LIST.error ("ELIST_ITERATOR::data_relative", ABORT, NULL);
  if (list->empty ())
    EMPTY_LIST.error ("ELIST_ITERATOR::data_relative", ABORT, NULL);
  if (offset < -1)
    BAD_PARAMETER.error ("ELIST_ITERATOR::data_relative", ABORT,
      "offset < -l");
  #endif

  if (offset == -1)
    ptr = prev;
  else
    for (ptr = current ? current : prev; offset-- > 0; ptr = ptr->next);

  #ifndef NDEBUG
  if (!ptr)
    NULL_DATA.error ("ELIST_ITERATOR::data_relative", ABORT, NULL);
  #endif

  return ptr;
}


/***********************************************************************
 *							ELIST_ITERATOR::move_to_last()
 *
 *  Move current so that it is set to the end of the list.
 *  Return data just in case anyone wants it.
 *  (This function can't be INLINEd because it contains a loop)
 **********************************************************************/

ELIST_LINK *ELIST_ITERATOR::move_to_last() {
  #ifndef NDEBUG
  if (!this)
    NULL_OBJECT.error ("ELIST_ITERATOR::move_to_last", ABORT, NULL);
  if (!list)
    NO_LIST.error ("ELIST_ITERATOR::move_to_last", ABORT, NULL);
  #endif

  while (current != list->last)
    forward();

  return current;
}


/***********************************************************************
 *							ELIST_ITERATOR::exchange()
 *
 *  Given another iterator, whose current element is a different element on
 *  the same list list OR an element of another list, exchange the two current
 *  elements.  On return, each iterator points to the element which was the
 *  other iterators current on entry.
 *  (This function hasn't been in-lined because its a bit big!)
 **********************************************************************/

void ELIST_ITERATOR::exchange(                             //positions of 2 links
                              ELIST_ITERATOR *other_it) {  //other iterator
  const ERRCODE DONT_EXCHANGE_DELETED =
    "Can't exchange deleted elements of lists";

  ELIST_LINK *old_current;

  #ifndef NDEBUG
  if (!this)
    NULL_OBJECT.error ("ELIST_ITERATOR::exchange", ABORT, NULL);
  if (!list)
    NO_LIST.error ("ELIST_ITERATOR::exchange", ABORT, NULL);
  if (!other_it)
    BAD_PARAMETER.error ("ELIST_ITERATOR::exchange", ABORT, "other_it NULL");
  if (!(other_it->list))
    NO_LIST.error ("ELIST_ITERATOR::exchange", ABORT, "other_it");
  #endif

  /* Do nothing if either list is empty or if both iterators reference the same
  link */

  if ((list->empty ()) ||
    (other_it->list->empty ()) || (current == other_it->current))
    return;

  /* Error if either current element is deleted */

  if (!current || !other_it->current)
    DONT_EXCHANGE_DELETED.error ("ELIST_ITERATOR.exchange", ABORT, NULL);

  /* Now handle the 4 cases: doubleton list; non-doubleton adjacent elements
  (other before this); non-doubleton adjacent elements (this before other);
  non-adjacent elements. */

                                 //adjacent links
  if ((next == other_it->current) ||
  (other_it->next == current)) {
                                 //doubleton list
    if ((next == other_it->current) &&
    (other_it->next == current)) {
      prev = next = current;
      other_it->prev = other_it->next = other_it->current;
    }
    else {                       //non-doubleton with
                                 //adjacent links
                                 //other before this
      if (other_it->next == current) {
        other_it->prev->next = current;
        other_it->current->next = next;
        current->next = other_it->current;
        other_it->next = other_it->current;
        prev = current;
      }
      else {                     //this before other
        prev->next = other_it->current;
        current->next = other_it->next;
        other_it->current->next = current;
        next = current;
        other_it->prev = other_it->current;
      }
    }
  }
  else {                         //no overlap
    prev->next = other_it->current;
    current->next = other_it->next;
    other_it->prev->next = current;
    other_it->current->next = next;
  }

  /* update end of list pointer when necessary (remember that the 2 iterators
    may iterate over different lists!) */

  if (list->last == current)
    list->last = other_it->current;
  if (other_it->list->last == other_it->current)
    other_it->list->last = current;

  if (current == cycle_pt)
    cycle_pt = other_it->cycle_pt;
  if (other_it->current == other_it->cycle_pt)
    other_it->cycle_pt = cycle_pt;

  /* The actual exchange - in all cases*/

  old_current = current;
  current = other_it->current;
  other_it->current = old_current;
}


/***********************************************************************
 *							ELIST_ITERATOR::extract_sublist()
 *
 *  This is a private member, used only by ELIST::assign_to_sublist.
 *  Given another iterator for the same list, extract the links from THIS to
 *  OTHER inclusive, link them into a new circular list, and return a
 *  pointer to the last element.
 *  (Can't inline this function because it contains a loop)
 **********************************************************************/

ELIST_LINK *ELIST_ITERATOR::extract_sublist(                             //from this current
                                            ELIST_ITERATOR *other_it) {  //to other current
  #ifndef NDEBUG
  const ERRCODE BAD_EXTRACTION_PTS =
    "Can't extract sublist from points on different lists";
  const ERRCODE DONT_EXTRACT_DELETED =
    "Can't extract a sublist marked by deleted points";
  #endif
  const ERRCODE BAD_SUBLIST = "Can't find sublist end point in original list";

  ELIST_ITERATOR temp_it = *this;
  ELIST_LINK *end_of_new_list;

  #ifndef NDEBUG
  if (!this)
    NULL_OBJECT.error ("ELIST_ITERATOR::extract_sublist", ABORT, NULL);
  if (!other_it)
    BAD_PARAMETER.error ("ELIST_ITERATOR::extract_sublist", ABORT,
      "other_it NULL");
  if (!list)
    NO_LIST.error ("ELIST_ITERATOR::extract_sublist", ABORT, NULL);
  if (list != other_it->list)
    BAD_EXTRACTION_PTS.error ("ELIST_ITERATOR.extract_sublist", ABORT, NULL);
  if (list->empty ())
    EMPTY_LIST.error ("ELIST_ITERATOR::extract_sublist", ABORT, NULL);

  if (!current || !other_it->current)
    DONT_EXTRACT_DELETED.error ("ELIST_ITERATOR.extract_sublist", ABORT,
      NULL);
  #endif

  ex_current_was_last = other_it->ex_current_was_last = FALSE;
  ex_current_was_cycle_pt = FALSE;
  other_it->ex_current_was_cycle_pt = FALSE;

  temp_it.mark_cycle_pt ();
  do {                           //walk sublist
    if (temp_it.cycled_list ())  //cant find end pt
      BAD_SUBLIST.error ("ELIST_ITERATOR.extract_sublist", ABORT, NULL);

    if (temp_it.at_last ()) {
      list->last = prev;
      ex_current_was_last = other_it->ex_current_was_last = TRUE;
    }

    if (temp_it.current == cycle_pt)
      ex_current_was_cycle_pt = TRUE;

    if (temp_it.current == other_it->cycle_pt)
      other_it->ex_current_was_cycle_pt = TRUE;

    temp_it.forward ();
  }
  while (temp_it.prev != other_it->current);

                                 //circularise sublist
  other_it->current->next = current;
  end_of_new_list = other_it->current;

                                 //sublist = whole list
  if (prev == other_it->current) {
    list->last = NULL;
    prev = current = next = NULL;
    other_it->prev = other_it->current = other_it->next = NULL;
  }
  else {
    prev->next = other_it->next;
    current = other_it->current = NULL;
    next = other_it->next;
    other_it->prev = prev;
  }
  return end_of_new_list;
}
