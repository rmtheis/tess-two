/* -*-C-*-
 ********************************************************************************
 *
 * File:        blobs.h  (Formerly blobs.h)
 * Description:  Blob definition
 * Author:       Mark Seaman, OCR Technology
 * Created:      Fri Oct 27 15:39:52 1989
 * Modified:     Thu Mar 28 15:33:38 1991 (Mark Seaman) marks@hpgrlt
 * Language:     C
 * Package:      N/A
 * Status:       Experimental (Do Not Distribute)
 *
 * (c) Copyright 1989, Hewlett-Packard Company.
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
 *********************************************************************************/

#ifndef BLOBS_H
#define BLOBS_H

/*----------------------------------------------------------------------
              I n c l u d e s
----------------------------------------------------------------------*/
#include "clst.h"
#include "normalis.h"
#include "publictypes.h"
#include "rect.h"
#include "vecfuncs.h"

class BLOCK;
class C_BLOB;
class C_OUTLINE;
class LLSQ;
class ROW;
class WERD;

/*----------------------------------------------------------------------
              T y p e s
----------------------------------------------------------------------*/
#define EDGEPTFLAGS     4        /*concavity,length etc. */

struct TPOINT {
  TPOINT(): x(0), y(0) {}
  TPOINT(inT16 vx, inT16 vy) : x(vx), y(vy) {}
  TPOINT(const ICOORD &ic) : x(ic.x()), y(ic.y()) {}

  void operator+=(const TPOINT& other) {
    x += other.x;
    y += other.y;
  }
  void operator/=(int divisor) {
    x /= divisor;
    y /= divisor;
  }

  inT16 x;                       // absolute x coord.
  inT16 y;                       // absolute y coord.
};
typedef TPOINT VECTOR;           // structure for coordinates.

struct EDGEPT {
  EDGEPT()
  : next(NULL), prev(NULL), src_outline(NULL), start_step(0), step_count(0) {
    memset(flags, 0, EDGEPTFLAGS * sizeof(flags[0]));
  }
  EDGEPT(const EDGEPT& src) : next(NULL), prev(NULL) {
    CopyFrom(src);
  }
  EDGEPT& operator=(const EDGEPT& src) {
    CopyFrom(src);
    return *this;
  }
  // Copies the data elements, but leaves the pointers untouched.
  void CopyFrom(const EDGEPT& src) {
    pos = src.pos;
    vec = src.vec;
    memcpy(flags, src.flags, EDGEPTFLAGS * sizeof(flags[0]));
    src_outline = src.src_outline;
    start_step = src.start_step;
    step_count = src.step_count;
  }
  // Accessors to hide or reveal a cut edge from feature extractors.
  void Hide() {
    flags[0] = true;
  }
  void Reveal() {
    flags[0] = false;
  }
  bool IsHidden() const {
    return flags[0] != 0;
  }
  void MarkChop() {
    flags[2] = true;
  }
  void UnmarkChop() {
    flags[2] = false;
  }
  bool IsChopPt() const {
    return flags[2] != 0;
  }

  TPOINT pos;                    // position
  VECTOR vec;                    // vector to next point
  // TODO(rays) Remove flags and replace with
  // is_hidden, runlength, dir, and fixed. The only use
  // of the flags other than is_hidden is in polyaprx.cpp.
  char flags[EDGEPTFLAGS];       // concavity, length etc
  EDGEPT* next;                  // anticlockwise element
  EDGEPT* prev;                  // clockwise element
  C_OUTLINE* src_outline;        // Outline it came from.
  // The following fields are not used if src_outline is NULL.
  int start_step;                // Location of pos in src_outline.
  int step_count;                // Number of steps used (may wrap around).
};

// For use in chop and findseam to keep a list of which EDGEPTs were inserted.
CLISTIZEH(EDGEPT);

struct TESSLINE {
  TESSLINE() : is_hole(false), loop(NULL), next(NULL) {}
  TESSLINE(const TESSLINE& src) : loop(NULL), next(NULL) {
    CopyFrom(src);
  }
  ~TESSLINE() {
    Clear();
  }
  TESSLINE& operator=(const TESSLINE& src) {
    CopyFrom(src);
    return *this;
  }
  // Consume the circular list of EDGEPTs to make a TESSLINE.
  static TESSLINE* BuildFromOutlineList(EDGEPT* outline);
  // Copies the data and the outline, but leaves next untouched.
  void CopyFrom(const TESSLINE& src);
  // Deletes owned data.
  void Clear();
  // Normalize in-place using the DENORM.
  void Normalize(const DENORM& denorm);
  // Rotates by the given rotation in place.
  void Rotate(const FCOORD rotation);
  // Moves by the given vec in place.
  void Move(const ICOORD vec);
  // Scales by the given factor in place.
  void Scale(float factor);
  // Sets up the start and vec members of the loop from the pos members.
  void SetupFromPos();
  // Recomputes the bounding box from the points in the loop.
  void ComputeBoundingBox();
  // Computes the min and max cross product of the outline points with the
  // given vec and returns the results in min_xp and max_xp. Geometrically
  // this is the left and right edge of the outline perpendicular to the
  // given direction, but to get the distance units correct, you would
  // have to divide by the modulus of vec.
  void MinMaxCrossProduct(const TPOINT vec, int* min_xp, int* max_xp) const;

  TBOX bounding_box() const;
  // Returns true if the point is contained within the outline box.
  bool Contains(const TPOINT& pt) {
    return topleft.x <= pt.x && pt.x <= botright.x &&
           botright.y <= pt.y && pt.y <= topleft.y;
  }

  #ifndef GRAPHICS_DISABLED
  void plot(ScrollView* window, ScrollView::Color color,
            ScrollView::Color child_color);
  #endif  // GRAPHICS_DISABLED

  // Returns the first outline point that has a different src_outline to its
  // predecessor, or, if all the same, the lowest indexed point.
  EDGEPT* FindBestStartPt() const;


  int BBArea() const {
    return (botright.x - topleft.x) * (topleft.y - botright.y);
  }

  TPOINT topleft;                // Top left of loop.
  TPOINT botright;               // Bottom right of loop.
  TPOINT start;                  // Start of loop.
  bool is_hole;                  // True if this is a hole/child outline.
  EDGEPT *loop;                  // Edgeloop.
  TESSLINE *next;                // Next outline in blob.
};                               // Outline structure.

struct TBLOB {
  TBLOB() : outlines(NULL) {}
  TBLOB(const TBLOB& src) : outlines(NULL) {
    CopyFrom(src);
  }
  ~TBLOB() {
    Clear();
  }
  TBLOB& operator=(const TBLOB& src) {
    CopyFrom(src);
    return *this;
  }
  // Factory to build a TBLOB from a C_BLOB with polygonal approximation along
  // the way. If allow_detailed_fx is true, the EDGEPTs in the returned TBLOB
  // contain pointers to the input C_OUTLINEs that enable higher-resolution
  // feature extraction that does not use the polygonal approximation.
  static TBLOB* PolygonalCopy(bool allow_detailed_fx, C_BLOB* src);
  // Factory builds a blob with no outlines, but copies the other member data.
  static TBLOB* ShallowCopy(const TBLOB& src);
  // Normalizes the blob for classification only if needed.
  // (Normally this means a non-zero classify rotation.)
  // If no Normalization is needed, then NULL is returned, and the input blob
  // can be used directly. Otherwise a new TBLOB is returned which must be
  // deleted after use.
  TBLOB* ClassifyNormalizeIfNeeded() const;

  // Copies the data and the outlines, but leaves next untouched.
  void CopyFrom(const TBLOB& src);
  // Deletes owned data.
  void Clear();
  // Sets up the built-in DENORM and normalizes the blob in-place.
  // For parameters see DENORM::SetupNormalization, plus the inverse flag for
  // this blob and the Pix for the full image.
  void Normalize(const BLOCK* block,
                 const FCOORD* rotation,
                 const DENORM* predecessor,
                 float x_origin, float y_origin,
                 float x_scale, float y_scale,
                 float final_xshift, float final_yshift,
                 bool inverse, Pix* pix);
  // Rotates by the given rotation in place.
  void Rotate(const FCOORD rotation);
  // Moves by the given vec in place.
  void Move(const ICOORD vec);
  // Scales by the given factor in place.
  void Scale(float factor);
  // Recomputes the bounding boxes of the outlines.
  void ComputeBoundingBoxes();

  // Returns the number of outlines.
  int NumOutlines() const;

  TBOX bounding_box() const;

  const DENORM& denorm() const {
    return denorm_;
  }

  #ifndef GRAPHICS_DISABLED
  void plot(ScrollView* window, ScrollView::Color color,
            ScrollView::Color child_color);
  #endif  // GRAPHICS_DISABLED

  int BBArea() const {
    int total_area = 0;
    for (TESSLINE* outline = outlines; outline != NULL; outline = outline->next)
      total_area += outline->BBArea();
    return total_area;
  }

  // Computes the center of mass and second moments for the old baseline and
  // 2nd moment normalizations. Returns the outline length.
  // The input denorm should be the normalizations that have been applied from
  // the image to the current state of this TBLOB.
  int ComputeMoments(FCOORD* center, FCOORD* second_moments) const;
  // Computes the precise bounding box of the coords that are generated by
  // GetEdgeCoords. This may be different from the bounding box of the polygon.
  void GetPreciseBoundingBox(TBOX* precise_box) const;
  // Adds edges to the given vectors.
  // For all the edge steps in all the outlines, or polygonal approximation
  // where there are no edge steps, collects the steps into x_coords/y_coords.
  // x_coords is a collection of the x-coords of vertical edges for each
  // y-coord starting at box.bottom().
  // y_coords is a collection of the y-coords of horizontal edges for each
  // x-coord starting at box.left().
  // Eg x_coords[0] is a collection of the x-coords of edges at y=bottom.
  // Eg x_coords[1] is a collection of the x-coords of edges at y=bottom + 1.
  void GetEdgeCoords(const TBOX& box,
                     GenericVector<GenericVector<int> >* x_coords,
                     GenericVector<GenericVector<int> >* y_coords) const;

  TESSLINE *outlines;            // List of outlines in blob.

 private:  // TODO(rays) Someday the data members will be private too.
  // For all the edge steps in all the outlines, or polygonal approximation
  // where there are no edge steps, collects the steps into the bounding_box,
  // llsq and/or the x_coords/y_coords. Both are used in different kinds of
  // normalization.
  // For a description of x_coords, y_coords, see GetEdgeCoords above.
  void CollectEdges(const TBOX& box,
                    TBOX* bounding_box, LLSQ* llsq,
                    GenericVector<GenericVector<int> >* x_coords,
                    GenericVector<GenericVector<int> >* y_coords) const;

 private:
  // DENORM indicating the transformations that this blob has undergone so far.
  DENORM denorm_;
};                               // Blob structure.

struct TWERD {
  TWERD() : latin_script(false) {}
  TWERD(const TWERD& src) {
    CopyFrom(src);
  }
  ~TWERD() {
    Clear();
  }
  TWERD& operator=(const TWERD& src) {
    CopyFrom(src);
    return *this;
  }
  // Factory to build a TWERD from a (C_BLOB) WERD, with polygonal
  // approximation along the way.
  static TWERD* PolygonalCopy(bool allow_detailed_fx, WERD* src);
  // Baseline normalizes the blobs in-place, recording the normalization in the
  // DENORMs in the blobs.
  void BLNormalize(const BLOCK* block, const ROW* row, Pix* pix, bool inverse,
                   float x_height, bool numeric_mode,
                   tesseract::OcrEngineMode hint,
                   const TBOX* norm_box,
                   DENORM* word_denorm);
  // Copies the data and the blobs, but leaves next untouched.
  void CopyFrom(const TWERD& src);
  // Deletes owned data.
  void Clear();
  // Recomputes the bounding boxes of the blobs.
  void ComputeBoundingBoxes();

  // Returns the number of blobs in the word.
  int NumBlobs() const {
    return blobs.size();
  }
  TBOX bounding_box() const;

  // Merges the blobs from start to end, not including end, and deletes
  // the blobs between start and end.
  void MergeBlobs(int start, int end);

  void plot(ScrollView* window);

  GenericVector<TBLOB*> blobs;   // Blobs in word.
  bool latin_script;             // This word is in a latin-based script.
};

/*----------------------------------------------------------------------
              M a c r o s
----------------------------------------------------------------------*/
/**********************************************************************
 * free_widths
 *
 * Free the memory taken up by a width array.
 **********************************************************************/
#define free_widths(w)  \
if (w) memfree (w)

/*----------------------------------------------------------------------
              F u n c t i o n s
----------------------------------------------------------------------*/
// TODO(rays) This will become a member of TBLOB when TBLOB's definition
// moves to blobs.h

// Returns the center of blob's bounding box in origin.
void blob_origin(TBLOB *blob, TPOINT *origin);

bool divisible_blob(TBLOB *blob, bool italic_blob, TPOINT* location);

void divide_blobs(TBLOB *blob, TBLOB *other_blob, bool italic_blob,
                  const TPOINT& location);

#endif
