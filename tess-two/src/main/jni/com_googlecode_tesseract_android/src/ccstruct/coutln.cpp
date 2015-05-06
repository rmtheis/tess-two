/**********************************************************************
 * File:        coutln.c  (Formerly coutline.c)
 * Description: Code for the C_OUTLINE class.
 * Author:                  Ray Smith
 * Created:                 Mon Oct 07 16:01:57 BST 1991
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

#include <string.h>
#ifdef __UNIX__
#include <assert.h>
#endif

#include "coutln.h"

#include "allheaders.h"
#include "blobs.h"
#include "normalis.h"

// Include automatically generated configuration file if running autoconf.
#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif

ELISTIZE (C_OUTLINE)
ICOORD C_OUTLINE::step_coords[4] = {
  ICOORD (-1, 0), ICOORD (0, -1), ICOORD (1, 0), ICOORD (0, 1)
};

/**********************************************************************
 * C_OUTLINE::C_OUTLINE
 *
 * Constructor to build a C_OUTLINE from a CRACKEDGE LOOP.
 **********************************************************************/

C_OUTLINE::C_OUTLINE (
//constructor
CRACKEDGE * startpt,             //outline to convert
ICOORD bot_left,                 //bounding box
ICOORD top_right, inT16 length   //length of loop
):box (bot_left, top_right), start (startpt->pos), offsets(NULL) {
  inT16 stepindex;               //index to step
  CRACKEDGE *edgept;             //current point

  stepcount = length;            //no of steps
  if (length == 0) {
    steps = NULL;
    return;
  }
                                 //get memory
  steps = (uinT8 *) alloc_mem (step_mem());
  memset(steps, 0, step_mem());
  edgept = startpt;

  for (stepindex = 0; stepindex < length; stepindex++) {
                                 //set compact step
    set_step (stepindex, edgept->stepdir);
    edgept = edgept->next;
  }
}


/**********************************************************************
 * C_OUTLINE::C_OUTLINE
 *
 * Constructor to build a C_OUTLINE from a C_OUTLINE_FRAG.
 **********************************************************************/
C_OUTLINE::C_OUTLINE (
//constructor
                                 //steps to copy
ICOORD startpt, DIR128 * new_steps,
inT16 length                     //length of loop
):start (startpt), offsets(NULL) {
  inT8 dirdiff;                  //direction difference
  DIR128 prevdir;                //previous direction
  DIR128 dir;                    //current direction
  DIR128 lastdir;                //dir of last step
  TBOX new_box;                   //easy bounding
  inT16 stepindex;               //index to step
  inT16 srcindex;                //source steps
  ICOORD pos;                    //current position

  pos = startpt;
  stepcount = length;            // No. of steps.
  ASSERT_HOST(length >= 0);
  steps = reinterpret_cast<uinT8*>(alloc_mem(step_mem()));  // Get memory.
  memset(steps, 0, step_mem());

  lastdir = new_steps[length - 1];
  prevdir = lastdir;
  for (stepindex = 0, srcindex = 0; srcindex < length;
  stepindex++, srcindex++) {
    new_box = TBOX (pos, pos);
    box += new_box;
                                 //copy steps
    dir = new_steps[srcindex];
    set_step(stepindex, dir);
    dirdiff = dir - prevdir;
    pos += step (stepindex);
    if ((dirdiff == 64 || dirdiff == -64) && stepindex > 0) {
      stepindex -= 2;            //cancel there-and-back
      prevdir = stepindex >= 0 ? step_dir (stepindex) : lastdir;
    }
    else
      prevdir = dir;
  }
  ASSERT_HOST (pos.x () == startpt.x () && pos.y () == startpt.y ());
  do {
    dirdiff = step_dir (stepindex - 1) - step_dir (0);
    if (dirdiff == 64 || dirdiff == -64) {
      start += step (0);
      stepindex -= 2;            //cancel there-and-back
      for (int i = 0; i < stepindex; ++i)
        set_step(i, step_dir(i + 1));
    }
  }
  while (stepindex > 1 && (dirdiff == 64 || dirdiff == -64));
  stepcount = stepindex;
  ASSERT_HOST (stepcount >= 4);
}

/**********************************************************************
 * C_OUTLINE::C_OUTLINE
 *
 * Constructor to build a C_OUTLINE from a rotation of a C_OUTLINE.
 **********************************************************************/

C_OUTLINE::C_OUTLINE(                     //constructor
                     C_OUTLINE *srcline,  //outline to
                     FCOORD rotation      //rotate
                    ) : offsets(NULL) {
  TBOX new_box;                   //easy bounding
  inT16 stepindex;               //index to step
  inT16 dirdiff;                 //direction change
  ICOORD pos;                    //current position
  ICOORD prevpos;                //previous dest point

  ICOORD destpos;                //destination point
  inT16 destindex;               //index to step
  DIR128 dir;                    //coded direction
  uinT8 new_step;

  stepcount = srcline->stepcount * 2;
  if (stepcount == 0) {
    steps = NULL;
    box = srcline->box;
    box.rotate(rotation);
    return;
  }
                                 //get memory
  steps = (uinT8 *) alloc_mem (step_mem());
  memset(steps, 0, step_mem());

  for (int iteration = 0; iteration < 2; ++iteration) {
    DIR128 round1 = iteration == 0 ? 32 : 0;
    DIR128 round2 = iteration != 0 ? 32 : 0;
    pos = srcline->start;
    prevpos = pos;
    prevpos.rotate (rotation);
    start = prevpos;
    box = TBOX (start, start);
    destindex = 0;
    for (stepindex = 0; stepindex < srcline->stepcount; stepindex++) {
      pos += srcline->step (stepindex);
      destpos = pos;
      destpos.rotate (rotation);
      //  tprintf("%i %i %i %i ", destpos.x(), destpos.y(), pos.x(), pos.y());
      while (destpos.x () != prevpos.x () || destpos.y () != prevpos.y ()) {
        dir = DIR128 (FCOORD (destpos - prevpos));
        dir += 64;                 //turn to step style
        new_step = dir.get_dir ();
        //  tprintf(" %i\n", new_step);
        if (new_step & 31) {
          set_step(destindex++, dir + round1);
          prevpos += step(destindex - 1);
          if (destindex < 2
            || ((dirdiff =
            step_dir (destindex - 1) - step_dir (destindex - 2)) !=
            -64 && dirdiff != 64)) {
            set_step(destindex++, dir + round2);
            prevpos += step(destindex - 1);
          } else {
            prevpos -= step(destindex - 1);
            destindex--;
            prevpos -= step(destindex - 1);
            set_step(destindex - 1, dir + round2);
            prevpos += step(destindex - 1);
          }
        }
        else {
          set_step(destindex++, dir);
          prevpos += step(destindex - 1);
        }
        while (destindex >= 2 &&
               ((dirdiff =
                 step_dir (destindex - 1) - step_dir (destindex - 2)) == -64 ||
                dirdiff == 64)) {
          prevpos -= step(destindex - 1);
          prevpos -= step(destindex - 2);
          destindex -= 2;        // Forget u turn
        }
        //ASSERT_HOST(prevpos.x() == destpos.x() && prevpos.y() == destpos.y());
        new_box = TBOX (destpos, destpos);
        box += new_box;
      }
    }
    ASSERT_HOST (destpos.x () == start.x () && destpos.y () == start.y ());
    dirdiff = step_dir (destindex - 1) - step_dir (0);
    while ((dirdiff == 64 || dirdiff == -64) && destindex > 1) {
      start += step (0);
      destindex -= 2;
      for (int i = 0; i < destindex; ++i)
        set_step(i, step_dir(i + 1));
      dirdiff = step_dir (destindex - 1) - step_dir (0);
    }
    if (destindex >= 4)
      break;
  }
  ASSERT_HOST(destindex <= stepcount);
  stepcount = destindex;
  destpos = start;
  for (stepindex = 0; stepindex < stepcount; stepindex++) {
    destpos += step (stepindex);
  }
  ASSERT_HOST (destpos.x () == start.x () && destpos.y () == start.y ());
}

// Build a fake outline, given just a bounding box and append to the list.
void C_OUTLINE::FakeOutline(const TBOX& box, C_OUTLINE_LIST* outlines) {
  C_OUTLINE_IT ol_it(outlines);
  // Make a C_OUTLINE from the bounds. This is a bit of a hack,
  // as there is no outline, just a bounding box, but it works nicely.
  CRACKEDGE start;
  start.pos = box.topleft();
  C_OUTLINE* outline = new C_OUTLINE(&start, box.topleft(), box.botright(), 0);
  ol_it.add_to_end(outline);
}

/**********************************************************************
 * C_OUTLINE::area
 *
 * Compute the area of the outline.
 **********************************************************************/

inT32 C_OUTLINE::area() const {
  int stepindex;                 //current step
  inT32 total_steps;             //steps to do
  inT32 total;                   //total area
  ICOORD pos;                    //position of point
  ICOORD next_step;              //step to next pix
  // We aren't going to modify the list, or its contents, but there is
  // no const iterator.
  C_OUTLINE_IT it(const_cast<C_OUTLINE_LIST*>(&children));

  pos = start_pos ();
  total_steps = pathlength ();
  total = 0;
  for (stepindex = 0; stepindex < total_steps; stepindex++) {
                                 //all intersected
    next_step = step (stepindex);
    if (next_step.x () < 0)
      total += pos.y ();
    else if (next_step.x () > 0)
      total -= pos.y ();
    pos += next_step;
  }
  for (it.mark_cycle_pt (); !it.cycled_list (); it.forward ())
    total += it.data ()->area ();//add areas of children

  return total;
}

/**********************************************************************
 * C_OUTLINE::perimeter
 *
 * Compute the perimeter of the outline and its first level children.
 **********************************************************************/

inT32 C_OUTLINE::perimeter() const {
  inT32 total_steps;             // Return value.
  // We aren't going to modify the list, or its contents, but there is
  // no const iterator.
  C_OUTLINE_IT it(const_cast<C_OUTLINE_LIST*>(&children));

  total_steps = pathlength();
  for (it.mark_cycle_pt(); !it.cycled_list(); it.forward())
    total_steps += it.data()->pathlength();  // Add perimeters of children.

  return total_steps;
}


/**********************************************************************
 * C_OUTLINE::outer_area
 *
 * Compute the area of the outline.
 **********************************************************************/

inT32 C_OUTLINE::outer_area() const {
  int stepindex;                 //current step
  inT32 total_steps;             //steps to do
  inT32 total;                   //total area
  ICOORD pos;                    //position of point
  ICOORD next_step;              //step to next pix

  pos = start_pos ();
  total_steps = pathlength ();
  if (total_steps == 0)
    return box.area();
  total = 0;
  for (stepindex = 0; stepindex < total_steps; stepindex++) {
                                 //all intersected
    next_step = step (stepindex);
    if (next_step.x () < 0)
      total += pos.y ();
    else if (next_step.x () > 0)
      total -= pos.y ();
    pos += next_step;
  }

  return total;
}


/**********************************************************************
 * C_OUTLINE::count_transitions
 *
 * Compute the number of x and y maxes and mins in the outline.
 **********************************************************************/

inT32 C_OUTLINE::count_transitions(                 //winding number
                                   inT32 threshold  //on size
                                  ) {
  BOOL8 first_was_max_x;         //what was first
  BOOL8 first_was_max_y;
  BOOL8 looking_for_max_x;       //what is next
  BOOL8 looking_for_min_x;
  BOOL8 looking_for_max_y;       //what is next
  BOOL8 looking_for_min_y;
  int stepindex;                 //current step
  inT32 total_steps;             //steps to do
                                 //current limits
  inT32 max_x, min_x, max_y, min_y;
  inT32 initial_x, initial_y;    //initial limits
  inT32 total;                   //total changes
  ICOORD pos;                    //position of point
  ICOORD next_step;              //step to next pix

  pos = start_pos ();
  total_steps = pathlength ();
  total = 0;
  max_x = min_x = pos.x ();
  max_y = min_y = pos.y ();
  looking_for_max_x = TRUE;
  looking_for_min_x = TRUE;
  looking_for_max_y = TRUE;
  looking_for_min_y = TRUE;
  first_was_max_x = FALSE;
  first_was_max_y = FALSE;
  initial_x = pos.x ();
  initial_y = pos.y ();          //stop uninit warning
  for (stepindex = 0; stepindex < total_steps; stepindex++) {
                                 //all intersected
    next_step = step (stepindex);
    pos += next_step;
    if (next_step.x () < 0) {
      if (looking_for_max_x && pos.x () < min_x)
        min_x = pos.x ();
      if (looking_for_min_x && max_x - pos.x () > threshold) {
        if (looking_for_max_x) {
          initial_x = max_x;
          first_was_max_x = FALSE;
        }
        total++;
        looking_for_max_x = TRUE;
        looking_for_min_x = FALSE;
        min_x = pos.x ();        //reset min
      }
    }
    else if (next_step.x () > 0) {
      if (looking_for_min_x && pos.x () > max_x)
        max_x = pos.x ();
      if (looking_for_max_x && pos.x () - min_x > threshold) {
        if (looking_for_min_x) {
          initial_x = min_x;     //remember first min
          first_was_max_x = TRUE;
        }
        total++;
        looking_for_max_x = FALSE;
        looking_for_min_x = TRUE;
        max_x = pos.x ();
      }
    }
    else if (next_step.y () < 0) {
      if (looking_for_max_y && pos.y () < min_y)
        min_y = pos.y ();
      if (looking_for_min_y && max_y - pos.y () > threshold) {
        if (looking_for_max_y) {
          initial_y = max_y;     //remember first max
          first_was_max_y = FALSE;
        }
        total++;
        looking_for_max_y = TRUE;
        looking_for_min_y = FALSE;
        min_y = pos.y ();        //reset min
      }
    }
    else {
      if (looking_for_min_y && pos.y () > max_y)
        max_y = pos.y ();
      if (looking_for_max_y && pos.y () - min_y > threshold) {
        if (looking_for_min_y) {
          initial_y = min_y;     //remember first min
          first_was_max_y = TRUE;
        }
        total++;
        looking_for_max_y = FALSE;
        looking_for_min_y = TRUE;
        max_y = pos.y ();
      }
    }

  }
  if (first_was_max_x && looking_for_min_x) {
    if (max_x - initial_x > threshold)
      total++;
    else
      total--;
  }
  else if (!first_was_max_x && looking_for_max_x) {
    if (initial_x - min_x > threshold)
      total++;
    else
      total--;
  }
  if (first_was_max_y && looking_for_min_y) {
    if (max_y - initial_y > threshold)
      total++;
    else
      total--;
  }
  else if (!first_was_max_y && looking_for_max_y) {
    if (initial_y - min_y > threshold)
      total++;
    else
      total--;
  }

  return total;
}


/**********************************************************************
 * C_OUTLINE::operator<
 *
 * Return TRUE if the left operand is inside the right one.
 **********************************************************************/

BOOL8
C_OUTLINE::operator< (           //winding number
const C_OUTLINE & other          //other outline
) const
{
  inT16 count = 0;               //winding count
  ICOORD pos;                    //position of point
  inT32 stepindex;               //index to cstep

  if (!box.overlap (other.box))
    return FALSE;                //can't be contained
  if (stepcount == 0)
    return other.box.contains(this->box);

  pos = start;
  for (stepindex = 0; stepindex < stepcount
    && (count = other.winding_number (pos)) == INTERSECTING; stepindex++)
    pos += step (stepindex);     //try all points
  if (count == INTERSECTING) {
                                 //all intersected
    pos = other.start;
    for (stepindex = 0; stepindex < other.stepcount
      && (count = winding_number (pos)) == INTERSECTING; stepindex++)
                                 //try other way round
      pos += other.step (stepindex);
    return count == INTERSECTING || count == 0;
  }
  return count != 0;
}


/**********************************************************************
 * C_OUTLINE::winding_number
 *
 * Return the winding number of the outline around the given point.
 **********************************************************************/

inT16 C_OUTLINE::winding_number(              //winding number
                                ICOORD point  //point to wind around
                               ) const {
  inT16 stepindex;               //index to cstep
  inT16 count;                   //winding count
  ICOORD vec;                    //to current point
  ICOORD stepvec;                //step vector
  inT32 cross;                   //cross product

  vec = start - point;           //vector to it
  count = 0;
  for (stepindex = 0; stepindex < stepcount; stepindex++) {
    stepvec = step (stepindex);  //get the step
                                 //crossing the line
    if (vec.y () <= 0 && vec.y () + stepvec.y () > 0) {
      cross = vec * stepvec;     //cross product
      if (cross > 0)
        count++;                 //crossing right half
      else if (cross == 0)
        return INTERSECTING;     //going through point
    }
    else if (vec.y () > 0 && vec.y () + stepvec.y () <= 0) {
      cross = vec * stepvec;
      if (cross < 0)
        count--;                 //crossing back
      else if (cross == 0)
        return INTERSECTING;     //illegal
    }
    vec += stepvec;              //sum vectors
  }
  return count;                  //winding number
}


/**********************************************************************
 * C_OUTLINE::turn_direction
 *
 * Return the sum direction delta of the outline.
 **********************************************************************/

inT16 C_OUTLINE::turn_direction() const {  //winding number
  DIR128 prevdir;                //previous direction
  DIR128 dir;                    //current direction
  inT16 stepindex;               //index to cstep
  inT8 dirdiff;                  //direction difference
  inT16 count;                   //winding count

  if (stepcount == 0)
    return 128;
  count = 0;
  prevdir = step_dir (stepcount - 1);
  for (stepindex = 0; stepindex < stepcount; stepindex++) {
    dir = step_dir (stepindex);
    dirdiff = dir - prevdir;
    ASSERT_HOST (dirdiff == 0 || dirdiff == 32 || dirdiff == -32);
    count += dirdiff;
    prevdir = dir;
  }
  ASSERT_HOST (count == 128 || count == -128);
  return count;                  //winding number
}


/**********************************************************************
 * C_OUTLINE::reverse
 *
 * Reverse the direction of an outline.
 **********************************************************************/

void C_OUTLINE::reverse() {  //reverse drection
  DIR128 halfturn = MODULUS / 2; //amount to shift
  DIR128 stepdir;                //direction of step
  inT16 stepindex;               //index to cstep
  inT16 farindex;                //index to other side
  inT16 halfsteps;               //half of stepcount

  halfsteps = (stepcount + 1) / 2;
  for (stepindex = 0; stepindex < halfsteps; stepindex++) {
    farindex = stepcount - stepindex - 1;
    stepdir = step_dir (stepindex);
    set_step (stepindex, step_dir (farindex) + halfturn);
    set_step (farindex, stepdir + halfturn);
  }
}


/**********************************************************************
 * C_OUTLINE::move
 *
 * Move C_OUTLINE by vector
 **********************************************************************/

void C_OUTLINE::move(                  // reposition OUTLINE
                     const ICOORD vec  // by vector
                    ) {
  C_OUTLINE_IT it(&children);  // iterator

  box.move (vec);
  start += vec;

  for (it.mark_cycle_pt (); !it.cycled_list (); it.forward ())
    it.data ()->move (vec);      // move child outlines
}

// Returns true if *this and its children are legally nested.
// The outer area of a child should have the opposite sign to the
// parent. If not, it means we have discarded an outline in between
// (probably due to excessive length).
bool C_OUTLINE::IsLegallyNested() const {
  if (stepcount == 0) return true;
  int parent_area = outer_area();
  // We aren't going to modify the list, or its contents, but there is
  // no const iterator.
  C_OUTLINE_IT child_it(const_cast<C_OUTLINE_LIST*>(&children));
  for (child_it.mark_cycle_pt(); !child_it.cycled_list(); child_it.forward()) {
    const C_OUTLINE* child = child_it.data();
    if (child->outer_area() * parent_area > 0 || !child->IsLegallyNested())
      return false;
  }
  return true;
}

// If this outline is smaller than the given min_size, delete this and
// remove from its list, via *it, after checking that *it points to this.
// Otherwise, if any children of this are too small, delete them.
// On entry, *it must be an iterator pointing to this. If this gets deleted
// then this is extracted from *it, so an iteration can continue.
void C_OUTLINE::RemoveSmallRecursive(int min_size, C_OUTLINE_IT* it) {
  if (box.width() < min_size || box.height() < min_size) {
    ASSERT_HOST(this == it->data());
    delete it->extract();  // Too small so get rid of it and any children.
  } else if (!children.empty()) {
    // Search the children of this, deleting any that are too small.
    C_OUTLINE_IT child_it(&children);
    for (child_it.mark_cycle_pt(); !child_it.cycled_list();
         child_it.forward()) {
      C_OUTLINE* child = child_it.data();
      child->RemoveSmallRecursive(min_size, &child_it);
    }
  }
}

// Factored out helpers below are used only by ComputeEdgeOffsets to operate
// on data from an 8-bit Pix, and assume that any input x and/or y are already
// constrained to be legal Pix coordinates.

// Helper computes the local 2-D gradient (dx, dy) from the 2x2 cell centered
// on the given (x,y). If the cell would go outside the image, it is padded
// with white.
static void ComputeGradient(const l_uint32* data, int wpl,
                            int x, int y, int width, int height,
                            ICOORD* gradient) {
  const l_uint32* line = data + y * wpl;
  int pix_x_y = x < width && y < height ?
      GET_DATA_BYTE(const_cast<void*> (reinterpret_cast<const void *>(line)), x) : 255;
  int pix_x_prevy = x < width && y > 0 ?
      GET_DATA_BYTE(const_cast<void*> (reinterpret_cast<const void *>(line - wpl)), x) : 255;
  int pix_prevx_prevy = x > 0 && y > 0 ?
      GET_DATA_BYTE(const_cast<void*> (reinterpret_cast<void const*>(line - wpl)), x - 1) : 255;
  int pix_prevx_y = x > 0 && y < height ?
      GET_DATA_BYTE(const_cast<void*> (reinterpret_cast<const void *>(line)), x - 1) : 255;
  gradient->set_x(pix_x_y + pix_x_prevy - (pix_prevx_y + pix_prevx_prevy));
  gradient->set_y(pix_x_prevy + pix_prevx_prevy - (pix_x_y + pix_prevx_y));
}

// Helper evaluates a vertical difference, (x,y) - (x,y-1), returning true if
// the difference, matches diff_sign and updating the best_diff, best_sum,
// best_y if a new max.
static bool EvaluateVerticalDiff(const l_uint32* data, int wpl, int diff_sign,
                                 int x, int y, int height,
                                 int* best_diff, int* best_sum, int* best_y) {
  if (y <= 0 || y >= height)
    return false;
  const l_uint32* line = data + y * wpl;
  int pixel1 = GET_DATA_BYTE(const_cast<void*> (reinterpret_cast<const void *>(line - wpl)), x);
  int pixel2 = GET_DATA_BYTE(const_cast<void*> (reinterpret_cast<const void *>(line)), x);
  int diff = (pixel2 - pixel1) * diff_sign;
  if (diff > *best_diff) {
    *best_diff = diff;
    *best_sum = pixel1 + pixel2;
    *best_y = y;
  }
  return diff > 0;
}

// Helper evaluates a horizontal difference, (x,y) - (x-1,y), where y is implied
// by the input image line, returning true if the difference matches diff_sign
// and updating the best_diff, best_sum, best_x if a new max.
static bool EvaluateHorizontalDiff(const l_uint32* line, int diff_sign,
                                   int x, int width,
                                   int* best_diff, int* best_sum, int* best_x) {
  if (x <= 0 || x >= width)
    return false;
  int pixel1 = GET_DATA_BYTE(const_cast<void*> (reinterpret_cast<const void *>(line)), x - 1);
  int pixel2 = GET_DATA_BYTE(const_cast<void*> (reinterpret_cast<const void *>(line)), x);
  int diff = (pixel2 - pixel1) * diff_sign;
  if (diff > *best_diff) {
    *best_diff = diff;
    *best_sum = pixel1 + pixel2;
    *best_x = x;
  }
  return diff > 0;
}

// Adds sub-pixel resolution EdgeOffsets for the outline if the supplied
// pix is 8-bit. Does nothing otherwise.
// Operation: Consider the following near-horizontal line:
// _________
//          |________
//                   |________
// At *every* position along this line, the gradient direction will be close
// to vertical. Extrapoaltion/interpolation of the position of the threshold
// that was used to binarize the image gives a more precise vertical position
// for each horizontal step, and the conflict in step direction and gradient
// direction can be used to ignore the vertical steps.
void C_OUTLINE::ComputeEdgeOffsets(int threshold, Pix* pix) {
  if (pixGetDepth(pix) != 8) return;
  const l_uint32* data = pixGetData(pix);
  int wpl = pixGetWpl(pix);
  int width = pixGetWidth(pix);
  int height = pixGetHeight(pix);
  bool negative = flag(COUT_INVERSE);
  delete [] offsets;
  offsets = new EdgeOffset[stepcount];
  ICOORD pos = start;
  ICOORD prev_gradient;
  ComputeGradient(data, wpl, pos.x(), height - pos.y(), width, height,
                  &prev_gradient);
  for (int s = 0; s < stepcount; ++s) {
    ICOORD step_vec = step(s);
    TPOINT pt1(pos);
    pos += step_vec;
    TPOINT pt2(pos);
    ICOORD next_gradient;
    ComputeGradient(data, wpl, pos.x(), height - pos.y(), width, height,
                    &next_gradient);
    // Use the sum of the prev and next as the working gradient.
    ICOORD gradient = prev_gradient + next_gradient;
    // best_diff will be manipulated to be always positive.
    int best_diff = 0;
    // offset will be the extrapolation of the location of the greyscale
    // threshold from the edge with the largest difference, relative to the
    // location of the binary edge.
    int offset = 0;
    if (pt1.y == pt2.y && abs(gradient.y()) * 2 >= abs(gradient.x())) {
      // Horizontal step. diff_sign == 1 indicates black above.
      int diff_sign = (pt1.x > pt2.x) == negative ? 1 : -1;
      int x = MIN(pt1.x, pt2.x);
      int y = height - pt1.y;
      int best_sum = 0;
      int best_y = y;
      EvaluateVerticalDiff(data, wpl, diff_sign, x, y, height,
                           &best_diff, &best_sum, &best_y);
      // Find the strongest edge.
      int test_y = y;
      do {
        ++test_y;
      } while (EvaluateVerticalDiff(data, wpl, diff_sign, x, test_y, height,
                                    &best_diff, &best_sum, &best_y));
      test_y = y;
      do {
        --test_y;
      } while (EvaluateVerticalDiff(data, wpl, diff_sign, x, test_y, height,
                                    &best_diff, &best_sum, &best_y));
      offset = diff_sign * (best_sum / 2 - threshold) +
          (y - best_y) * best_diff;
    } else if (pt1.x == pt2.x && abs(gradient.x()) * 2 >= abs(gradient.y())) {
      // Vertical step. diff_sign == 1 indicates black on the left.
      int diff_sign = (pt1.y > pt2.y) == negative ? 1 : -1;
      int x = pt1.x;
      int y = height - MAX(pt1.y, pt2.y);
      const l_uint32* line = pixGetData(pix) + y * wpl;
      int best_sum = 0;
      int best_x = x;
      EvaluateHorizontalDiff(line, diff_sign, x, width,
                             &best_diff, &best_sum, &best_x);
      // Find the strongest edge.
      int test_x = x;
      do {
        ++test_x;
      } while (EvaluateHorizontalDiff(line, diff_sign, test_x, width,
                                      &best_diff, &best_sum, &best_x));
      test_x = x;
      do {
        --test_x;
      } while (EvaluateHorizontalDiff(line, diff_sign, test_x, width,
                                      &best_diff, &best_sum, &best_x));
      offset = diff_sign * (threshold - best_sum / 2) +
          (best_x - x) * best_diff;
    }
    offsets[s].offset_numerator =
        static_cast<inT8>(ClipToRange(offset, -MAX_INT8, MAX_INT8));
    offsets[s].pixel_diff = static_cast<uinT8>(ClipToRange(best_diff, 0 ,
                                                           MAX_UINT8));
    if (negative) gradient = -gradient;
    // Compute gradient angle quantized to 256 directions, rotated by 64 (pi/2)
    // to convert from gradient direction to edge direction.
    offsets[s].direction =
        Modulo(FCOORD::binary_angle_plus_pi(gradient.angle()) + 64, 256);
    prev_gradient = next_gradient;
  }
}

// Adds sub-pixel resolution EdgeOffsets for the outline using only
// a binary image source.
// Runs a sliding window of 5 edge steps over the outline, maintaining a count
// of the number of steps in each of the 4 directions in the window, and a
// sum of the x or y position of each step (as appropriate to its direction.)
// Ignores single-count steps EXCEPT the sharp U-turn and smoothes out the
// perpendicular direction. Eg
// ___              ___       Chain code from the left:
//    |___    ___   ___|      222122212223221232223000
//        |___|  |_|          Corresponding counts of each direction:
//                          0   00000000000000000123
//                          1   11121111001111100000
//                          2   44434443443333343321
//                          3   00000001111111112111
// Count of direction at center 41434143413313143313
// Step gets used?              YNYYYNYYYNYYNYNYYYyY (y= U-turn exception)
// Path redrawn showing only the used points:
// ___              ___
//     ___    ___   ___|
//         ___    _
// Sub-pixel edge position cannot be shown well with ASCII-art, but each
// horizontal step's y position is the mean of the y positions of the steps
// in the same direction in the sliding window, which makes a much smoother
// outline, without losing important detail.
void C_OUTLINE::ComputeBinaryOffsets() {
  delete [] offsets;
  offsets = new EdgeOffset[stepcount];
  // Count of the number of steps in each direction in the sliding window.
  int dir_counts[4];
  // Sum of the positions (y for a horizontal step, x for vertical) in each
  // direction in the sliding window.
  int pos_totals[4];
  memset(dir_counts, 0, sizeof(dir_counts));
  memset(pos_totals, 0, sizeof(pos_totals));
  ICOORD pos = start;
  ICOORD tail_pos = pos;
  // tail_pos is the trailing position, with the next point to be lost from
  // the window.
  tail_pos -= step(stepcount - 1);
  tail_pos -= step(stepcount - 2);
  // head_pos is the leading position, with the next point to be added to the
  // window.
  ICOORD head_pos = tail_pos;
  // Set up the initial window with 4 points in [-2, 2)
  for (int s = -2; s < 2; ++s) {
    increment_step(s, 1, &head_pos, dir_counts, pos_totals);
  }
  for (int s = 0; s < stepcount; pos += step(s++)) {
    // At step s, s in in the middle of [s-2, s+2].
    increment_step(s + 2, 1, &head_pos, dir_counts, pos_totals);
    int dir_index = chain_code(s);
    ICOORD step_vec = step(s);
    int best_diff = 0;
    int offset = 0;
    // Use only steps that have a count of >=2 OR the strong U-turn with a
    // single d and 2 at d-1 and 2 at d+1 (mod 4).
    if (dir_counts[dir_index] >= 2 || (dir_counts[dir_index] == 1 &&
        dir_counts[Modulo(dir_index - 1, 4)] == 2 &&
        dir_counts[Modulo(dir_index + 1, 4)] == 2)) {
      // Valid step direction.
      best_diff = dir_counts[dir_index];
      int edge_pos = step_vec.x() == 0 ? pos.x() : pos.y();
      // The offset proposes that the actual step should be positioned at
      // the mean position of the steps in the window of the same direction.
      // See ASCII art above.
      offset = pos_totals[dir_index] - best_diff * edge_pos;
    }
    offsets[s].offset_numerator =
        static_cast<inT8>(ClipToRange(offset, -MAX_INT8, MAX_INT8));
    offsets[s].pixel_diff = static_cast<uinT8>(ClipToRange(best_diff, 0 ,
                                                           MAX_UINT8));
    // The direction is just the vector from start to end of the window.
    FCOORD direction(head_pos.x() - tail_pos.x(), head_pos.y() - tail_pos.y());
    offsets[s].direction = direction.to_direction();
    increment_step(s - 2, -1, &tail_pos, dir_counts, pos_totals);
  }
}

// Renders the outline to the given pix, with left and top being
// the coords of the upper-left corner of the pix.
void C_OUTLINE::render(int left, int top, Pix* pix) const {
  ICOORD pos = start;
  for (int stepindex = 0; stepindex < stepcount; ++stepindex) {
    ICOORD next_step = step(stepindex);
    if (next_step.y() < 0) {
      pixRasterop(pix, 0, top - pos.y(), pos.x() - left, 1,
                  PIX_NOT(PIX_DST), NULL, 0, 0);
    } else if (next_step.y() > 0) {
      pixRasterop(pix, 0, top - pos.y() - 1, pos.x() - left, 1,
                  PIX_NOT(PIX_DST), NULL, 0, 0);
    }
    pos += next_step;
  }
}

// Renders just the outline to the given pix (no fill), with left and top
// being the coords of the upper-left corner of the pix.
void C_OUTLINE::render_outline(int left, int top, Pix* pix) const {
  ICOORD pos = start;
  for (int stepindex = 0; stepindex < stepcount; ++stepindex) {
    ICOORD next_step = step(stepindex);
    if (next_step.y() < 0) {
      pixSetPixel(pix, pos.x() - left, top - pos.y(), 1);
    } else if (next_step.y() > 0) {
      pixSetPixel(pix, pos.x() - left - 1, top - pos.y() - 1, 1);
    } else if (next_step.x() < 0) {
      pixSetPixel(pix, pos.x() - left - 1, top - pos.y(), 1);
    } else if (next_step.x() > 0) {
      pixSetPixel(pix, pos.x() - left, top - pos.y() - 1, 1);
    }
    pos += next_step;
  }
}

/**********************************************************************
 * C_OUTLINE::plot
 *
 * Draw the outline in the given colour.
 **********************************************************************/

#ifndef GRAPHICS_DISABLED
void C_OUTLINE::plot(                //draw it
                     ScrollView* window,       // window to draw in
                     ScrollView::Color colour  // colour to draw in
                    ) const {
  inT16 stepindex;               // index to cstep
  ICOORD pos;                    // current position
  DIR128 stepdir;                // direction of step

  pos = start;                   // current position
  window->Pen(colour);
  if (stepcount == 0) {
    window->Rectangle(box.left(), box.top(), box.right(), box.bottom());
    return;
  }
  window->SetCursor(pos.x(), pos.y());

  stepindex = 0;
  while (stepindex < stepcount) {
    pos += step(stepindex);    // step to next
    stepdir = step_dir(stepindex);
    stepindex++;               // count steps
    // merge straight lines
    while (stepindex < stepcount &&
           stepdir.get_dir() == step_dir(stepindex).get_dir()) {
      pos += step(stepindex);
      stepindex++;
    }
    window->DrawTo(pos.x(), pos.y());
  }
}
// Draws the outline in the given colour, normalized using the given denorm,
// making use of sub-pixel accurate information if available.
void C_OUTLINE::plot_normed(const DENORM& denorm, ScrollView::Color colour,
                            ScrollView* window) const {
  window->Pen(colour);
  if (stepcount == 0) {
    window->Rectangle(box.left(), box.top(), box.right(), box.bottom());
    return;
  }
  const DENORM* root_denorm = denorm.RootDenorm();
  ICOORD pos = start;                   // current position
  FCOORD f_pos = sub_pixel_pos_at_index(pos, 0);
  FCOORD pos_normed;
  denorm.NormTransform(root_denorm, f_pos, &pos_normed);
  window->SetCursor(IntCastRounded(pos_normed.x()),
                    IntCastRounded(pos_normed.y()));
  for (int s = 0; s < stepcount; pos += step(s++)) {
    int edge_weight = edge_strength_at_index(s);
    if (edge_weight == 0) {
      // This point has conflicting gradient and step direction, so ignore it.
      continue;
    }
    FCOORD f_pos = sub_pixel_pos_at_index(pos, s);
    FCOORD pos_normed;
    denorm.NormTransform(root_denorm, f_pos, &pos_normed);
    window->DrawTo(IntCastRounded(pos_normed.x()),
                   IntCastRounded(pos_normed.y()));
  }
}
#endif


/**********************************************************************
 * C_OUTLINE::operator=
 *
 * Assignment - deep copy data
 **********************************************************************/

                                 //assignment
C_OUTLINE & C_OUTLINE::operator= (
const C_OUTLINE & source         //from this
) {
  box = source.box;
  start = source.start;
  if (steps != NULL)
    free_mem(steps);
  stepcount = source.stepcount;
  steps = (uinT8 *) alloc_mem (step_mem());
  memmove (steps, source.steps, step_mem());
  if (!children.empty ())
    children.clear ();
  children.deep_copy(&source.children, &deep_copy);
  delete [] offsets;
  if (source.offsets != NULL) {
    offsets = new EdgeOffset[stepcount];
    memcpy(offsets, source.offsets, stepcount * sizeof(*offsets));
  } else {
    offsets = NULL;
  }
  return *this;
}

// Helper for ComputeBinaryOffsets. Increments pos, dir_counts, pos_totals
// by the step, increment, and vertical step ? x : y position * increment
// at step s Mod stepcount respectively. Used to add or subtract the
// direction and position to/from accumulators of a small neighbourhood.
void C_OUTLINE::increment_step(int s, int increment, ICOORD* pos,
                               int* dir_counts, int* pos_totals) const {
  int step_index = Modulo(s, stepcount);
  int dir_index = chain_code(step_index);
  dir_counts[dir_index] += increment;
  ICOORD step_vec = step(step_index);
  if (step_vec.x() == 0)
    pos_totals[dir_index] += pos->x() * increment;
  else
    pos_totals[dir_index] += pos->y() * increment;
  *pos += step_vec;
}

ICOORD C_OUTLINE::chain_step(int chaindir) {
  return step_coords[chaindir % 4];
}
