/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -
 -  Redistribution and use in source and binary forms, with or without
 -  modification, are permitted provided that the following conditions
 -  are met:
 -  1. Redistributions of source code must retain the above copyright
 -     notice, this list of conditions and the following disclaimer.
 -  2. Redistributions in binary form must reproduce the above
 -     copyright notice, this list of conditions and the following
 -     disclaimer in the documentation and/or other materials
 -     provided with the distribution.
 -
 -  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 -  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 -  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 -  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 -  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 -  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 -  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 -  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 -  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 -  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 -  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================*/

/*
 *  dewarp1.c
 *
 *    Basic operations and serialization
 *
 *      Create/destroy dewarp
 *          L_DEWARP          *dewarpCreate()
 *          L_DEWARP          *dewarpCreateRef()
 *          void               dewarpDestroy()
 *
 *      Create/destroy dewarpa
 *          L_DEWARPA         *dewarpaCreate()
 *          L_DEWARPA         *dewarpaCreateFromPixacomp()
 *          void               dewarpaDestroy()
 *          l_int32            dewarpaDestroyDewarp()
 *
 *      Dewarpa insertion/extraction
 *          l_int32            dewarpaInsertDewarp()
 *          static l_int32     dewarpaExtendArraysToSize()
 *          L_DEWARP          *dewarpaGetDewarp()
 *
 *      Setting parameters to control rendering from the model
 *          l_int32            dewarpaSetCurvatures()
 *          l_int32            dewarpaUseBothArrays()
 *          l_int32            dewarpaSetMaxDistance()
 *
 *      Dewarp serialized I/O
 *          L_DEWARP          *dewarpRead()
 *          L_DEWARP          *dewarpReadStream()
 *          l_int32            dewarpWrite()
 *          l_int32            dewarpWriteStream()
 *
 *      Dewarpa serialized I/O
 *          L_DEWARPA         *dewarpaRead()
 *          L_DEWARPA         *dewarpaReadStream()
 *          l_int32            dewarpaWrite()
 *          l_int32            dewarpaWriteStream()
 *
 *
 *  Examples of usage
 *  =================
 *
 *  See dewarpaCreateFromPixacomp() for an example of the basic
 *  operations, starting from a set of 1 bpp images.
 *
 *  Basic functioning to dewarp a specific single page:
 *     // Make the Dewarpa for the pages
 *     L_Dewarpa *dewa = dewarpaCreate(1, 30, 1, 15, 50);
 *     dewarpaSetCurvatures(dewa, -1, 5, -1, -1, -1, -1);
 *     dewarpaUseBothArrays(dewa, 1);  // try to use both disparity
 *                                     // arrays for this example
 *
 *     // Do the page: start with a binarized image
 *     Pix *pixb = "binarize"(pixs);
 *     // Initialize a Dewarp for this page (say, page 214)
 *     L_Dewarp *dew = dewarpCreate(pixb, 214);
 *     // Insert in Dewarpa and obtain parameters for building the model
 *     dewarpaInsertDewarp(dewa, dew);
 *     // Do the work
 *     dewarpBuildPageModel(dew, NULL);  // no debugging
 *     // Optionally set rendering parameters
 *     // Apply model to the input pixs
 *     Pix *pixd;
 *     dewarpaApplyDisparity(dewa, 214, pixs, 255, 0, 0, &pixd, NULL);
 *     pixDestroy(&pixb);
 *
 *  Basic functioning to dewarp many pages:
 *     // Make the Dewarpa for the set of pages; use fullres 1 bpp
 *     L_Dewarpa *dewa = dewarpaCreate(10, 30, 1, 15, 50);
 *     // Optionally set rendering parameters
 *     dewarpaSetCurvatures(dewa, -1, 10, -1, -1, -1, -1);
 *     dewarpaUseBothArrays(dewa, 0);  // just use the vertical disparity
 *                                     // array for this example
 *
 *     // Do first page: start with a binarized image
 *     Pix *pixb = "binarize"(pixs);
 *     // Initialize a Dewarp for this page (say, page 1)
 *     L_Dewarp *dew = dewarpCreate(pixb, 1);
 *     // Insert in Dewarpa and obtain parameters for building the model
 *     dewarpaInsertDewarp(dewa, dew);
 *     // Do the work
 *     dewarpBuildPageModel(dew, NULL);  // no debugging
 *     dewarpMinimze(dew);  // remove most heap storage
 *     pixDestroy(&pixb);
 *
 *     // Do the other pages the same way
 *     ...
 *
 *     // Apply models to each page; if the page model is invalid,
 *     // try to use a valid neighboring model.  Note that the call
 *     // to dewarpaInsertRefModels() is optional, because it is called
 *     // by dewarpaApplyDisparity() on the first page it acts on.
 *     dewarpaInsertRefModels(dewa, 0, 1); // use debug flag to get more
 *                         // detailed information about the page models
 *     [For each page, where pixs is the fullres image to be dewarped] {
 *         L_Dewarp *dew = dewarpaGetDewarp(dewa, pageno);
 *         if (dew) {  // disparity model exists
 *             Pix *pixd;
 *             dewarpaApplyDisparity(dewa, pageno, pixs, 255,
 *                                   0, 0, &pixd, NULL);
 *             dewarpMinimize(dew);  // clean out the pix and fpix arrays
 *             // Squirrel pixd away somewhere ...)
 *         }
 *     }
 *
 *  Basic functioning to dewarp a small set of pages, potentially
 *  using models from nearby pages:
 *     // (1) Generate a set of binarized images in the vicinity of the
 *     // pages to be dewarped.  We will attempt to compute models
 *     // for pages from 'firstpage' to 'lastpage'.
 *     // Store the binarized images in a compressed array of
 *     // size 'n', where 'n' is the number of images to be stored,
 *     // and where the offset is the first page.
 *     PixaComp *pixac = pixacompCreateInitialized(n, firstpage, NULL,
 *                                                 IFF_TIFF_G4);
 *     for (i = firstpage; i <= lastpage; i++) {
 *         Pix *pixb = "binarize"(pixs);
 *         pixacompReplacePix(pixac, i, pixb, IFF_TIFF_G4);
 *         pixDestroy(&pixb);
 *     }
 *
 *     // (2) Make the Dewarpa for the pages.
 *     L_Dewarpa *dewa =
 *           dewarpaCreateFromPixacomp(pixac, 30, 15, 20);
 *     dewarpaUseBothArrays(dewa, 1);  // try to use both disparity arrays
 *                                     // in this example
 *
 *     // (3) Finally, apply the models.  For page 'firstpage' with image pixs:
 *     L_Dewarp *dew = dewarpaGetDewarp(dewa, firstpage);
 *     if (dew) {  // disparity model exists
 *         Pix *pixd;
 *         dewarpaApplyDisparity(dewa, firstpage, pixs, 255, 0, 0, &pixd, NULL);
 *         dewarpMinimize(dew);
 *     }
 *
 *  Because in general some pages will not have enough text to build a
 *  model, we fill in for those pages with a reference to the page
 *  model to use.  Both the target page and the reference page must
 *  have the same parity.  We can also choose to use either a partial model
 *  (with only vertical disparity) or the full model of a nearby page.
 *
 *  Minimizing the data in a model by stripping out images,
 *  numas, and full resolution disparity arrays:
 *     dewarpMinimize(dew);
 *  This can be done at any time to save memory.  Serialization does
 *  not use the data that is stripped.
 *
 *  You can apply any model (in a dew), stripped or not, to another image:
 *     // For all pages with invalid models, assign the nearest valid
 *     // page model with same parity.
 *     dewarpaInsertRefModels(dewa, 0, 0);
 *     // You can then apply to 'newpix' the page model that was assigned
 *     // to 'pageno', giving the result in pixd:
 *     Pix *pixd;
 *     dewarpaApplyDisparity(dewa, pageno, newpix, 255, 0, 0, &pixd, NULL);
 *
 *  You can apply the disparity arrays to a deliberately undercropped
 *  image.  Suppose that you undercrop by (left, right, top, bot), so
 *  that the disparity arrays are aligned with their origin at (left, top).
 *  Dewarp the undercropped image with:
 *     Pix *pixd;
 *     dewarpaApplyDisparity(dewa, pageno, undercropped_pix, 255,
 *                           left, top, &pixd, NULL);
 *
 *
 *  Description of the approach to analyzing page image distortion
 *  ==============================================================
 *
 *  When a book page is scanned, there are several possible causes
 *  for the text lines to appear to be curved:
 *   (1) A barrel (fish-eye) effect because the camera is at
 *       a finite distance from the page.  Take the normal from
 *       the camera to the page (the 'optic axis').  Lines on
 *       the page "below" this point will appear to curve upward
 *       (negative curvature); lines "above" this will curve downward.
 *   (2) Radial distortion from the camera lens.  Probably not
 *       a big factor.
 *   (3) Local curvature of the page in to (or out of) the image
 *       plane (which is perpendicular to the optic axis).
 *       This has no effect if the page is flat.
 *
 *  In the following, the optic axis is in the z direction and is
 *  perpendicular to the xy plane;, the book is assumed to be aligned
 *  so that y is approximately along the binding.
 *  The goal is to compute the "disparity" field, D(x,y), which
 *  is actually a vector composed of the horizontal and vertical
 *  disparity fields H(x,y) and V(x,y).  Each of these is a local
 *  function that gives the amount each point in the image is
 *  required to move in order to rectify the horizontal and vertical
 *  lines.  It would also be nice to "flatten" the page to compensate
 *  for effect (3), foreshortening due to bending of the page into
 *  the z direction, but that is more difficult.
 *
 *  Effects (1) and (2) can be directly compensated by calibrating
 *  the scene, using a flat page with horizontal and vertical lines.
 *  Then H(x,y) and V(x,y) can be found as two (non-parametric) arrays
 *  of values.  Suppose this has been done.  Then the remaining
 *  distortion is due to (3).
 *
 *  We consider the simple situation where the page bending is independent
 *  of y, and is described by alpha(x), where alpha is the angle between
 *  the normal to the page and the optic axis.  cos(alpha(x)) is the local
 *  compression factor of the page image in the horizontal direction, at x.
 *  Thus, if we know alpha(x), we can compute the disparity H(x) required
 *  to flatten the image by simply integrating 1/cos(alpha), and we could
 *  compute the remaining disparities, H(x,y) and V(x,y), from the
 *  page content, as described below.  Unfortunately, we don't know
 *  alpha.  What do we know?  If there are horizontal text lines
 *  on the page, we can compute the vertical disparity, V(x,y), which
 *  is the local translation required to make the text lines parallel
 *  to the rasters.  If the margins are left and right aligned, we can
 *  also estimate the horizontal disparity, H(x,y), required to have
 *  uniform margins.  All that can be done from the image alone,
 *  assuming we have text lines covering a sufficient part of the page.
 *
 *  What about alpha(x)?  The basic question relating to (3) is this:
 *
 *     Is it possible, using the shape of the text lines alone,
 *     to compute both the vertical and horizontal disparity fields?
 *
 *  The underlying problem is to separate the line curvature effects due
 *  to the camera view from those due to actual bending of the page.
 *  I believe the proper way to do this is to make some measurements
 *  based on the camera setup, which will depend mostly on the distance
 *  of the camera from the page, and to a smaller extent on the location
 *  of the optic axis with respect to the page.
 *
 *  Here is the procedure.  Photograph a page with a fine 2D line grid
 *  several times, each with a different slope near the binding.
 *  This can be done by placing the grid page on books that have
 *  different shapes z(x) near the binding.  For each one you can
 *  measure, near the binding:
 *    (1) ds/dy, the vertical rate of change of slope of the horizontal lines
 *    (2) the local horizontal compression of the vertical lines due
 *        to the page angle dz/dx.
 *  As mentioned above, the local horizontal compression is simply
 *  cos(dz/dx).  But the measurement you can make on an actual book
 *  page is (1).  The difficulty is to generate (2) from (1).
 *
 *  Back to the procedure.  The function in (1), ds/dy, likely needs
 *  to be measured at a few y locations, because the relation
 *  between (1) and (2) may weakly depend on the y-location with
 *  respect to the y-coordinate of the optic axis of the camera.
 *  From these measurements you can determine, for the camera setup
 *  that you have, the local horizontal compression, cos(dz/dx), as a
 *  function of the both vertical location (y) and your measured vertical
 *  derivative of the text line slope there, ds/dy.  Then with
 *  appropriate smoothing of your measured values, you can set up a
 *  horizontal disparity array to correct for the compression due
 *  to dz/dx.
 *
 *  Now consider V(x,0) and V(x,h), the vertical disparity along
 *  the top and bottom of the image.  With a little thought you
 *  can convince yourself that the local foreshortening,
 *  as a function of x, is proportional to the difference
 *  between the slope of V(x,0) and V(x,h).  The horizontal
 *  disparity can then be computed by integrating the local foreshortening
 *  over x.  Integration of the slope of V(x,0) and V(x,h) gives
 *  the vertical disparity itself.  We have to normalize to h, the
 *  height of the page.  So the very simple result is that
 *
 *      H(x) ~ (V(x,0) - V(x,h)) / h         [1]
 *
 *  which is easily computed.  There is a proportionality constant
 *  that depends on the ratio of h to the distance to the camera.
 *  Can we actually believe this for the case where the bending
 *  is independent of y?  I believe the answer is yes,
 *  as long as you first remove the apparent distortion due
 *  to the camera being at a finite distance.
 *
 *  If you know the intersection of the optical axis with the page
 *  and the distance to the camera, and if the page is perpendicular
 *  to the optic axis, you can compute the horizontal and vertical
 *  disparities due to (1) and (2) and remove them.  The resulting
 *  distortion should be entirely due to bending (3), for which
 *  the relation
 *
 *      Hx(x) dx = C * ((Vx(x,0) - Vx(x, h))/h) dx         [2]
 *
 *  holds for each point in x (Hx and Vx are partial derivatives w/rt x).
 *  Integrating over x, and using H(0) = 0, we get the result [1].
 *
 *  I believe this result holds differentially for each value of y, so
 *  that in the case where the bending is not independent of y,
 *  the expression (V(x,0) - V(x,h)) / h goes over to Vy(x,y).  Then
 *
 *     H(x,y) = Integral(0,x) (Vyx(x,y) dx)         [3]
 *
 *  where Vyx() is the partial derivative of V w/rt both x and y.
 *
 *  It would be nice if there were a simple mathematical relation between
 *  the horizontal and vertical disparities for the situation
 *  where the paper bends without stretching or kinking.
 *  I had hoped to get a relation between H and V, such as
 *  Hx(x,y) ~ Vy(x,y), which would imply that H and V are real
 *  and imaginary parts of a complex potential, each of which
 *  satisfy the laplace equation.  But then the gradients of the
 *  two potentials would be normal, and that does not appear to be the case.
 *  Thus, the questions of proving the relations above (for small bending),
 *  or finding a simpler relation between H and V than those equations,
 *  remain open.  So far, we have only used [1] for the horizontal
 *  disparity H(x).
 *
 *  In the version of the code that follows, we first use text lines
 *  to find V(x,y).  Then, we try to compute H(x,y) that will align
 *  the text vertically on the left and right margins.  This is not
 *  always possible -- sometimes the right margin is not right justified.
 *  By default, we don't require the horizontal disparity to have a
 *  valid page model for dewarping a page, but this requirement can
 *  be forced using dewarpaUseFullModel().
 *
 *  As described above, one can add a y-independent component of
 *  the horizontal disparity H(x) to counter the foreshortening
 *  effect due to the bending of the page near the binding.
 *  This requires widening the image on the side near the binding,
 *  and we do not provide this option here.  However, we do provide
 *  a function that will generate this disparity field:
 *       fpixExtraHorizDisparity()
 *
 *  Here is the basic outline for building the disparity arrays.
 *
 *  (1) Find lines going approximately through the center of the
 *      text in each text line.  Accept only lines that are
 *      close in length to the longest line.
 *  (2) Use these lines to generate a regular and highly subsampled
 *      vertical disparity field V(x,y).
 *  (3) Interpolate this to generate a full resolution vertical
 *      disparity field.
 *  (4) For lines that are sufficiently long, determine if the lines
 *      are left and right-justified, and if so, construct a highly
 *      subsampled horizontal disparity field H(x,y) that will bring
 *      them into alignment.
 *  (5) Interpolate this to generate a full resolution horizontal
 *      disparity field.
 *  (6) Apply the vertical dewarping, followed by the horizontal dewarping.
 *
 *  Step (1) is clearly described by the code in pixGetTextlineCenters().
 *
 *  Steps (2) and (3) follow directly from the data in step (1),
 *  and constitute the bulk of the work done in dewarpBuildPageModel().
 *  Virtually all the noise in the data is smoothed out by doing
 *  least-square quadratic fits, first horizontally to the data
 *  points representing the text line centers, and then vertically.
 *  The trick is to sample these lines on a regular grid.
 *  First each horizontal line is sampled at equally spaced
 *  intervals horizontally.  We thus get a set of points,
 *  one in each line, that are vertically aligned, and
 *  the data we represent is the vertical distance of each point
 *  from the min or max value on the curve, depending on the
 *  sign of the curvature component.  Each of these vertically
 *  aligned sets of points constitutes a sampled vertical disparity,
 *  and we do a LS quartic fit to each of them, followed by
 *  vertical sampling at regular intervals.  We now have a subsampled
 *  grid of points, all equally spaced, giving at each point the local
 *  vertical disparity.  Finally, the full resolution vertical disparity
 *  is formed by interpolation.  All the least square fits do a
 *  great job of smoothing everything out, as can be observed by
 *  the contour maps that are generated for the vertical disparity field.
 */

#include <math.h>
#include "allheaders.h"

static l_int32 dewarpaExtendArraysToSize(L_DEWARPA *dewa, l_int32 size);


    /* Special parameter values */
static const l_int32     MIN_ARRAY_SAMPLING = 8;
static const l_int32     DEFAULT_ARRAY_SAMPLING = 30;
static const l_int32     MIN_MIN_LINES = 4;
static const l_int32     DEFAULT_MIN_LINES = 15;
static const l_int32     DEFAULT_MAX_REF_DIST = 16;
static const l_float32   DEFAULT_SLOPE_FACTOR = 2000.;

static const l_int32     INITIAL_PTR_ARRAYSIZE = 20;   /* n'import quoi */
static const l_int32     MAX_PTR_ARRAYSIZE = 10000;

static const l_int32     DEFAULT_MAX_LINECURV = 180;
static const l_int32     DEFAULT_MIN_DIFF_LINECURV = 0;
static const l_int32     DEFAULT_MAX_DIFF_LINECURV = 150;
static const l_int32     DEFAULT_MAX_EDGECURV = 50;
static const l_int32     DEFAULT_MAX_DIFF_EDGECURV = 30;
static const l_int32     DEFAULT_MAX_EDGESLOPE = 100;


/*----------------------------------------------------------------------*
 *                           Create/destroy Dewarp                      *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpCreate()
 *
 *     Input: pixs (1 bpp)
 *            pageno (page number)
 *     Return: dew (or null on error)
 *
 *  Notes:
 *      (1) The input pixs is either full resolution or 2x reduced.
 *      (2) The page number is typically 0-based.  If scanned from a book,
 *          the even pages are usually on the left.  Disparity arrays
 *          built for even pages should only be applied to even pages.
 */
L_DEWARP *
dewarpCreate(PIX     *pixs,
             l_int32  pageno)
{
L_DEWARP  *dew;

    PROCNAME("dewarpCreate");

    if (!pixs)
        return (L_DEWARP *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (L_DEWARP *)ERROR_PTR("pixs not 1 bpp", procName, NULL);

    if ((dew = (L_DEWARP *)CALLOC(1, sizeof(L_DEWARP))) == NULL)
        return (L_DEWARP *)ERROR_PTR("dew not made", procName, NULL);
    dew->pixs = pixClone(pixs);
    dew->pageno = pageno;
    dew->w = pixGetWidth(pixs);
    dew->h = pixGetHeight(pixs);
    return dew;
}


/*!
 *  dewarpCreateRef()
 *
 *     Input:  pageno (this page number)
 *             refpage (page number of dewarp disparity arrays to be used)
 *     Return: dew (or null on error)
 *
 *  Notes:
 *      (1) This specifies which dewarp struct should be used for
 *          the given page.  It is placed in dewarpa for pages
 *          for which no model can be built.
 *      (2) This page and the reference page have the same parity and
 *          the reference page is the closest page with a disparity model
 *          to this page.
 */
L_DEWARP *
dewarpCreateRef(l_int32  pageno,
                l_int32  refpage)
{
L_DEWARP  *dew;

    PROCNAME("dewarpCreateRef");

    if ((dew = (L_DEWARP *)CALLOC(1, sizeof(L_DEWARP))) == NULL)
        return (L_DEWARP *)ERROR_PTR("dew not made", procName, NULL);
    dew->pageno = pageno;
    dew->hasref = 1;
    dew->refpage = refpage;
    return dew;
}


/*!
 *  dewarpDestroy()
 *
 *      Input:  &dew (<will be set to null before returning>)
 *      Return: void
 */
void
dewarpDestroy(L_DEWARP  **pdew)
{
L_DEWARP  *dew;

    PROCNAME("dewarpDestroy");

    if (pdew == NULL) {
        L_WARNING("ptr address is null!\n", procName);
        return;
    }
    if ((dew = *pdew) == NULL)
        return;

    pixDestroy(&dew->pixs);
    fpixDestroy(&dew->sampvdispar);
    fpixDestroy(&dew->samphdispar);
    fpixDestroy(&dew->fullvdispar);
    fpixDestroy(&dew->fullhdispar);
    numaDestroy(&dew->namidys);
    numaDestroy(&dew->nacurves);
    FREE(dew);
    *pdew = NULL;
    return;
}


/*----------------------------------------------------------------------*
 *                          Create/destroy Dewarpa                      *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpaCreate()
 *
 *     Input: nptrs (number of dewarp page ptrs; typically the number of pages)
 *            sampling (use 0 for default value; the minimum allowed is 8)
 *            redfactor (of input images: 1 is full resolution; 2 is 2x reduced)
 *            minlines (minimum number of lines to accept; use 0 for default)
 *            maxdist (for locating reference disparity; use -1 for default)
 *     Return: dewa (or null on error)
 *
 *  Notes:
 *      (1) The sampling, minlines and maxdist parameters will be
 *          applied to all images.
 *      (2) The sampling factor is used for generating the disparity arrays
 *          from the input image.  For 2x reduced input, use a sampling
 *          factor that is half the sampling you want on the full resolution
 *          images.
 *      (3) Use @redfactor = 1 for full resolution; 2 for 2x reduction.
 *          All input images must be at one of these two resolutions.
 *      (4) @minlines is the minimum number of nearly full-length lines
 *          required to generate a vertical disparity array.  The default
 *          number is 15.  Use a smaller number to accept a questionable
 *          array, but not smaller than 4.
 *      (5) When a model can't be built for a page, it looks up to @maxdist
 *          in either direction for a valid model with the same page parity.
 *          Use -1 for the default value of @maxdist; use 0 to avoid using
 *          a ref model.
 *      (6) The ptr array is expanded as necessary to accommodate page images.
 */
L_DEWARPA *
dewarpaCreate(l_int32  nptrs,
              l_int32  sampling,
              l_int32  redfactor,
              l_int32  minlines,
              l_int32  maxdist)
{
L_DEWARPA  *dewa;

    PROCNAME("dewarpaCreate");

    if (nptrs <= 0)
        nptrs = INITIAL_PTR_ARRAYSIZE;
    if (nptrs > MAX_PTR_ARRAYSIZE)
        return (L_DEWARPA *)ERROR_PTR("too many pages", procName, NULL);
    if (redfactor != 1 && redfactor != 2)
        return (L_DEWARPA *)ERROR_PTR("redfactor not in {1,2}",
                                      procName, NULL);
    if (sampling == 0) {
         sampling = DEFAULT_ARRAY_SAMPLING;
    } else if (sampling < MIN_ARRAY_SAMPLING) {
         L_WARNING("sampling too small; setting to %d\n", procName,
                   MIN_ARRAY_SAMPLING);
         sampling = MIN_ARRAY_SAMPLING;
    }
    if (minlines == 0) {
        minlines = DEFAULT_MIN_LINES;
    } else if (minlines < MIN_MIN_LINES) {
        L_WARNING("minlines too small; setting to %d\n", procName,
                  MIN_MIN_LINES);
        minlines = DEFAULT_MIN_LINES;
    }
    if (maxdist < 0)
         maxdist = DEFAULT_MAX_REF_DIST;

    if ((dewa = (L_DEWARPA *)CALLOC(1, sizeof(L_DEWARPA))) == NULL)
        return (L_DEWARPA *)ERROR_PTR("dewa not made", procName, NULL);
    if ((dewa->dewarp =
         (L_DEWARP **)CALLOC(nptrs, sizeof(L_DEWARPA *))) == NULL)
        return (L_DEWARPA *)ERROR_PTR("dewarp ptrs not made", procName, NULL);
    if ((dewa->dewarpcache =
        (L_DEWARP **)CALLOC(nptrs, sizeof(L_DEWARPA *))) == NULL)
        return (L_DEWARPA *)ERROR_PTR("dewarpcache ptrs not made",
                                      procName, NULL);
    dewa->nalloc = nptrs;
    dewa->sampling = sampling;
    dewa->redfactor = redfactor;
    dewa->minlines = minlines;
    dewa->maxdist = maxdist;
    dewa->max_linecurv = DEFAULT_MAX_LINECURV;
    dewa->min_diff_linecurv = DEFAULT_MIN_DIFF_LINECURV;
    dewa->max_diff_linecurv = DEFAULT_MAX_DIFF_LINECURV;
    dewa->max_edgeslope = DEFAULT_MAX_EDGESLOPE;
    dewa->max_edgecurv = DEFAULT_MAX_EDGECURV;
    dewa->max_diff_edgecurv = DEFAULT_MAX_DIFF_EDGECURV;

    return dewa;
}


/*!
 *  dewarpaCreateFromPixacomp()
 *
 *     Input: pixac (pixacomp of G4, 1 bpp images; with 1x1x1 placeholders)
 *            useboth (0 for vert disparity; 1 for both vert and horiz)
 *            sampling (use -1 or 0 for default value; otherwise minimum of 5)
 *            minlines (minimum number of lines to accept; e.g., 10)
 *            maxdist (for locating reference disparity; use -1 for default)
 *     Return: dewa (or null on error)
 *
 *  Notes:
 *      (1) The returned dewa has disparity arrays calculated and
 *          is ready for serialization or for use in dewarping.
 *      (2) The sampling, minlines and maxdist parameters are
 *          applied to all images.  See notes in dewarpaCreate() for details.
 *      (3) The pixac is full.  Placeholders, if any, are w=h=d=1 images,
 *          and the real input images are 1 bpp at full resolution.
 *          They are assumed to be cropped to the actual page regions,
 *          and may be arbitrarily sparse in the array.
 *      (4) The output dewarpa is indexed by the page number.
 *          The offset in the pixac gives the mapping between the
 *          array index in the pixac and the page number.
 *      (5) This adds the ref page models.
 *      (6) This can be used to make models for any desired set of pages.
 *          The direct models are only made for pages with images in
 *          the pixacomp; the ref models are made for pages of the
 *          same parity within @maxdist of the nearest direct model.
 */
L_DEWARPA *
dewarpaCreateFromPixacomp(PIXAC   *pixac,
                          l_int32  useboth,
                          l_int32  sampling,
                          l_int32  minlines,
                          l_int32  maxdist)
{
l_int32     i, nptrs, pageno;
L_DEWARP   *dew;
L_DEWARPA  *dewa;
PIX        *pixt;

    PROCNAME("dewarpaCreateFromPixacomp");

    if (!pixac)
        return (L_DEWARPA *)ERROR_PTR("pixac not defined", procName, NULL);

    nptrs = pixacompGetCount(pixac);
    if ((dewa = dewarpaCreate(pixacompGetOffset(pixac) + nptrs,
                              sampling, 1, minlines, maxdist)) == NULL)
        return (L_DEWARPA *)ERROR_PTR("dewa not made", procName, NULL);
    dewarpaUseBothArrays(dewa, useboth);

    for (i = 0; i < nptrs; i++) {
        pageno = pixacompGetOffset(pixac) + i;  /* index into pixacomp */
        pixt = pixacompGetPix(pixac, pageno);
        if (pixt && (pixGetWidth(pixt) > 1)) {
            dew = dewarpCreate(pixt, pageno);
            pixDestroy(&pixt);
            if (!dew) {
                ERROR_INT("unable to make dew!", procName, 1);
                continue;
            }

               /* Insert into dewa for this page */
            dewarpaInsertDewarp(dewa, dew);

               /* Build disparity arrays for this page */
            dewarpBuildPageModel(dew, NULL);
            if (!dew->vsuccess) {  /* will need to use model from nearby page */
                dewarpaDestroyDewarp(dewa, pageno);
                L_ERROR("unable to build model for page %d\n", procName, i);
                continue;
            }
                /* Remove all extraneous data */
            dewarpMinimize(dew);
        }
        pixDestroy(&pixt);
    }
    dewarpaInsertRefModels(dewa, 0, 0);

    return dewa;
}


/*!
 *  dewarpaDestroy()
 *
 *      Input:  &dewa (<will be set to null before returning>)
 *      Return: void
 */
void
dewarpaDestroy(L_DEWARPA  **pdewa)
{
l_int32     i;
L_DEWARP   *dew;
L_DEWARPA  *dewa;

    PROCNAME("dewarpaDestroy");

    if (pdewa == NULL) {
        L_WARNING("ptr address is null!\n", procName);
        return;
    }
    if ((dewa = *pdewa) == NULL)
        return;

    for (i = 0; i < dewa->nalloc; i++) {
        if ((dew = dewa->dewarp[i]) != NULL)
            dewarpDestroy(&dew);
        if ((dew = dewa->dewarpcache[i]) != NULL)
            dewarpDestroy(&dew);
    }
    numaDestroy(&dewa->namodels);
    numaDestroy(&dewa->napages);

    FREE(dewa->dewarp);
    FREE(dewa->dewarpcache);
    FREE(dewa);
    *pdewa = NULL;
    return;
}


/*!
 *  dewarpaDestroyDewarp()
 *
 *      Input:  dewa
 *              pageno (of dew to be destroyed)
 *      Return: 0 if OK, 1 on error
 */
l_int32
dewarpaDestroyDewarp(L_DEWARPA  *dewa,
                     l_int32     pageno)
{
L_DEWARP   *dew;

    PROCNAME("dewarpaDestroyDewarp");

    if (!dewa)
        return ERROR_INT("dewa or dew not defined", procName, 1);
    if (pageno < 0 || pageno > dewa->maxpage)
        return ERROR_INT("page out of bounds", procName, 1);
    if ((dew = dewa->dewarp[pageno]) == NULL)
        return ERROR_INT("dew not defined", procName, 1);

    dewarpDestroy(&dew);
    dewa->dewarp[pageno] = NULL;
    return 0;
}


/*----------------------------------------------------------------------*
 *                       Dewarpa insertion/extraction                   *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpaInsertDewarp()
 *
 *      Input:  dewarpa
 *              dewarp  (to be added)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This inserts the dewarp into the array, which now owns it.
 *          It also keeps track of the largest page number stored.
 *          It must be done before the disparity model is built.
 *      (2) Note that this differs from the usual method of filling out
 *          arrays in leptonica, where the arrays are compact and
 *          new elements are typically added to the end.  Here,
 *          the dewarp can be added anywhere, even beyond the initial
 *          allocation.
 */
l_int32
dewarpaInsertDewarp(L_DEWARPA  *dewa,
                    L_DEWARP   *dew)
{
l_int32    pageno, n, newsize;
L_DEWARP  *prevdew;

    PROCNAME("dewarpaInsertDewarp");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);
    if (!dew)
        return ERROR_INT("dew not defined", procName, 1);

    dew->dewa = dewa;
    pageno = dew->pageno;
    if (pageno > MAX_PTR_ARRAYSIZE)
        return ERROR_INT("too many pages", procName, 1);
    if (pageno > dewa->maxpage)
        dewa->maxpage = pageno;
    dewa->modelsready = 0;  /* force re-evaluation at application time */

        /* Extend ptr array if necessary */
    n = dewa->nalloc;
    newsize = n;
    if (pageno >= 2 * n)
        newsize = 2 * pageno;
    else if (pageno >= n)
        newsize = 2 * n;
    if (newsize > n)
        dewarpaExtendArraysToSize(dewa, newsize);

    if ((prevdew = dewarpaGetDewarp(dewa, pageno)) != NULL)
        dewarpDestroy(&prevdew);
    dewa->dewarp[pageno] = dew;

    dew->sampling = dewa->sampling;
    dew->redfactor = dewa->redfactor;
    dew->minlines = dewa->minlines;

        /* Get the dimensions of the sampled array.  This will be
         * stored in an fpix, and the input resolution version is
         * guaranteed to be larger than pixs.  However, if you
         * want to apply the disparity to an image with a width
         *     w > nx * s - 2 * s + 2
         * you will need to extend the input res fpix.
         * And similarly for h.  */
    dew->nx = (dew->w + 2 * dew->sampling - 2) / dew->sampling;
    dew->ny = (dew->h + 2 * dew->sampling - 2) / dew->sampling;
    return 0;
}


/*!
 *  dewarpaExtendArraysToSize()
 *
 *      Input:  dewa
 *              size (new size of dewarpa array)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) If necessary, reallocs main and cache dewarpa ptr arrays to @size.
 */
static l_int32
dewarpaExtendArraysToSize(L_DEWARPA  *dewa,
                         l_int32     size)
{
    PROCNAME("dewarpaExtendArraysToSize");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    if (size > dewa->nalloc) {
        if ((dewa->dewarp = (L_DEWARP **)reallocNew((void **)&dewa->dewarp,
                sizeof(L_DEWARP *) * dewa->nalloc,
                size * sizeof(L_DEWARP *))) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);
        if ((dewa->dewarpcache =
                (L_DEWARP **)reallocNew((void **)&dewa->dewarpcache,
                sizeof(L_DEWARP *) * dewa->nalloc,
                size * sizeof(L_DEWARP *))) == NULL)
            return ERROR_INT("new ptr cache array not returned", procName, 1);
        dewa->nalloc = size;
    }
    return 0;
}


/*!
 *  dewarpaGetDewarp()
 *
 *      Input:  dewa (populated with dewarp structs for pages)
 *              index (into dewa: this is the pageno)
 *      Return: dew (handle; still owned by dewa), or null on error
 */
L_DEWARP *
dewarpaGetDewarp(L_DEWARPA  *dewa,
                 l_int32     index)
{
    PROCNAME("dewarpaGetDewarp");

    if (!dewa)
        return (L_DEWARP *)ERROR_PTR("dewa not defined", procName, NULL);
    if (index < 0 || index > dewa->maxpage)
        return (L_DEWARP *)ERROR_PTR("invalid index", procName, NULL);

    return dewa->dewarp[index];
}


/*----------------------------------------------------------------------*
 *         Setting parameters to control rendering from the model       *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpaSetCurvatures()
 *
 *      Input:  dewa
 *              max_linecurv (-1 for default)
 *              min_diff_linecurv (-1 for default; 0 to accept all models)
 *              max_diff_linecurv (-1 for default)
 *              max_edgecurv (-1 for default)
 *              max_diff_edgecurv (-1 for default)
 *              max_edgeslope (-1 for default)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Approximating the line by a quadratic, the coefficent
 *          of the quadratic term is the curvature, and distance
 *          units are in pixels (of course).  The curvature is very
 *          small, so we multiply by 10^6 and express the constraints
 *          on the model curvatures in micro-units.
 *      (2) This sets five curvature thresholds and a slope threshold:
 *          * the maximum absolute value of the vertical disparity
 *            line curvatures
 *          * the minimum absolute value of the largest difference in
 *            vertical disparity line curvatures (Use a value of 0
 *            to accept all models.)
 *          * the maximum absolute value of the largest difference in
 *            vertical disparity line curvatures
 *          * the maximum absolute value of the left and right edge
 *            curvature for the horizontal disparity
 *          * the maximum absolute value of the difference between
 *            left and right edge curvature for the horizontal disparity
 *          all in micro-units, for dewarping to take place.
 *          Use -1 for default values.
 *      (3) An image with a line curvature less than about 0.00001
 *          has fairly straight textlines.  This is 10 micro-units.
 *      (4) For example, if @max_linecurv == 100, this would prevent dewarping
 *          if any of the lines has a curvature exceeding 100 micro-units.
 *          A model having maximum line curvature larger than about 150
 *          micro-units should probably not be used.
 *      (5) A model having a left or right edge curvature larger than
 *          about 100 micro-units should probably not be used.
 */
l_int32
dewarpaSetCurvatures(L_DEWARPA  *dewa,
                     l_int32     max_linecurv,
                     l_int32     min_diff_linecurv,
                     l_int32     max_diff_linecurv,
                     l_int32     max_edgecurv,
                     l_int32     max_diff_edgecurv,
                     l_int32     max_edgeslope)
{
    PROCNAME("dewarpaSetCurvatures");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    if (max_linecurv == -1)
        dewa->max_linecurv = DEFAULT_MAX_LINECURV;
    else
        dewa->max_linecurv = L_ABS(max_linecurv);

    if (min_diff_linecurv == -1)
        dewa->min_diff_linecurv = DEFAULT_MIN_DIFF_LINECURV;
    else
        dewa->min_diff_linecurv = L_ABS(min_diff_linecurv);

    if (max_diff_linecurv == -1)
        dewa->max_diff_linecurv = DEFAULT_MAX_DIFF_LINECURV;
    else
        dewa->max_diff_linecurv = L_ABS(max_diff_linecurv);

    if (max_edgecurv == -1)
        dewa->max_edgecurv = DEFAULT_MAX_EDGECURV;
    else
        dewa->max_edgecurv = L_ABS(max_edgecurv);

    if (max_diff_edgecurv == -1)
        dewa->max_diff_edgecurv = DEFAULT_MAX_DIFF_EDGECURV;
    else
        dewa->max_diff_edgecurv = L_ABS(max_diff_edgecurv);

    if (max_edgeslope == -1)
        dewa->max_edgeslope = DEFAULT_MAX_EDGESLOPE;
    else
        dewa->max_edgeslope = L_ABS(max_edgeslope);

    dewa->modelsready = 0;  /* force validation */
    return 0;
}


/*!
 *  dewarpaUseBothArrays()
 *
 *      Input:  dewa
 *              useboth (0 for false, 1 for true)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This sets the useboth field.  If set, this will attempt
 *          to apply both vertical and horizontal disparity arrays.
 *          Note that a model with only a vertical disparity array will
 *          always be valid.
 */
l_int32
dewarpaUseBothArrays(L_DEWARPA  *dewa,
                     l_int32     useboth)
{
    PROCNAME("dewarpaUseBothArrays");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    dewa->useboth = useboth;
    dewa->modelsready = 0;  /* force validation */
    return 0;
}


/*!
 *  dewarpaSetMaxDistance()
 *
 *      Input:  dewa
 *              maxdist (for using ref models)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This sets the maxdist field.
 */
l_int32
dewarpaSetMaxDistance(L_DEWARPA  *dewa,
                      l_int32     maxdist)
{
    PROCNAME("dewarpaSetMaxDistance");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    dewa->maxdist = maxdist;
    dewa->modelsready = 0;  /* force validation */
    return 0;
}


/*----------------------------------------------------------------------*
 *                       Dewarp serialized I/O                          *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpRead()
 *
 *      Input:  filename
 *      Return: dew, or null on error
 */
L_DEWARP *
dewarpRead(const char  *filename)
{
FILE      *fp;
L_DEWARP  *dew;

    PROCNAME("dewarpRead");

    if (!filename)
        return (L_DEWARP *)ERROR_PTR("filename not defined", procName, NULL);
    if ((fp = fopenReadStream(filename)) == NULL)
        return (L_DEWARP *)ERROR_PTR("stream not opened", procName, NULL);

    if ((dew = dewarpReadStream(fp)) == NULL) {
        fclose(fp);
        return (L_DEWARP *)ERROR_PTR("dew not read", procName, NULL);
    }

    fclose(fp);
    return dew;
}


/*!
 *  dewarpReadStream()
 *
 *      Input:  stream
 *      Return: dew, or null on error
 *
 *  Notes:
 *      (1) The dewarp struct is stored in minimized format, with only
 *          subsampled disparity arrays.
 *      (2) The sampling and extra horizontal disparity parameters are
 *          stored here.  During generation of the dewarp struct, they
 *          are passed in from the dewarpa.  In readback, it is assumed
 *          that they are (a) the same for each page and (b) the same
 *          as the values used to create the dewarpa.
 */
L_DEWARP *
dewarpReadStream(FILE  *fp)
{
l_int32    version, sampling, redfactor, minlines, pageno, hasref, refpage;
l_int32    w, h, nx, ny, vdispar, hdispar, nlines;
l_int32    mincurv, maxcurv, leftslope, rightslope, leftcurv, rightcurv;
L_DEWARP  *dew;
FPIX      *fpixv, *fpixh;

    PROCNAME("dewarpReadStream");

    if (!fp)
        return (L_DEWARP *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nDewarp Version %d\n", &version) != 1)
        return (L_DEWARP *)ERROR_PTR("not a dewarp file", procName, NULL);
    if (version != DEWARP_VERSION_NUMBER)
        return (L_DEWARP *)ERROR_PTR("invalid dewarp version", procName, NULL);
    if (fscanf(fp, "pageno = %d\n", &pageno) != 1)
        return (L_DEWARP *)ERROR_PTR("read fail for pageno", procName, NULL);
    if (fscanf(fp, "hasref = %d, refpage = %d\n", &hasref, &refpage) != 2)
        return (L_DEWARP *)ERROR_PTR("read fail for hasref, refpage",
                                     procName, NULL);
    if (fscanf(fp, "sampling = %d, redfactor = %d\n", &sampling, &redfactor)
               != 2)
        return (L_DEWARP *)ERROR_PTR("read fail for sampling/redfactor",
                                     procName, NULL);
    if (fscanf(fp, "nlines = %d, minlines = %d\n", &nlines, &minlines) != 2)
        return (L_DEWARP *)ERROR_PTR("read fail for nlines/minlines",
                                     procName, NULL);
    if (fscanf(fp, "w = %d, h = %d\n", &w, &h) != 2)
        return (L_DEWARP *)ERROR_PTR("read fail for w, h", procName, NULL);
    if (fscanf(fp, "nx = %d, ny = %d\n", &nx, &ny) != 2)
        return (L_DEWARP *)ERROR_PTR("read fail for nx, ny", procName, NULL);
    if (fscanf(fp, "vert_dispar = %d, horiz_dispar = %d\n", &vdispar, &hdispar)
               != 2)
        return (L_DEWARP *)ERROR_PTR("read fail for flags", procName, NULL);
    if (vdispar) {
        if (fscanf(fp, "min line curvature = %d, max line curvature = %d\n",
                   &mincurv, &maxcurv) != 2)
            return (L_DEWARP *)ERROR_PTR("read fail for mincurv & maxcurv",
                                         procName, NULL);
    }
    if (hdispar) {
        if (fscanf(fp, "left edge slope = %d, right edge slope = %d\n",
                   &leftslope, &rightslope) != 2)
            return (L_DEWARP *)ERROR_PTR("read fail for leftslope & rightslope",
                                         procName, NULL);
        if (fscanf(fp, "left edge curvature = %d, right edge curvature = %d\n",
                   &leftcurv, &rightcurv) != 2)
            return (L_DEWARP *)ERROR_PTR("read fail for leftcurv & rightcurv",
                                         procName, NULL);
    }
    if (vdispar) {
        if ((fpixv = fpixReadStream(fp)) == NULL)
            return (L_DEWARP *)ERROR_PTR("read fail for vdispar",
                                         procName, NULL);
    }
    if (hdispar) {
        if ((fpixh = fpixReadStream(fp)) == NULL)
            return (L_DEWARP *)ERROR_PTR("read fail for hdispar",
                                         procName, NULL);
    }
    getc(fp);

    dew = (L_DEWARP *)CALLOC(1, sizeof(L_DEWARP));
    dew->w = w;
    dew->h = h;
    dew->pageno = pageno;
    dew->sampling = sampling;
    dew->redfactor = redfactor;
    dew->minlines = minlines;
    dew->nlines = nlines;
    dew->hasref = hasref;
    dew->refpage = refpage;
    if (hasref == 0)  /* any dew without a ref has an actual model */
        dew->vsuccess = 1;
    dew->nx = nx;
    dew->ny = ny;
    if (vdispar) {
        dew->mincurv = mincurv;
        dew->maxcurv = maxcurv;
        dew->vsuccess = 1;
        dew->sampvdispar = fpixv;
    }
    if (hdispar) {
        dew->leftslope = leftslope;
        dew->rightslope = rightslope;
        dew->leftcurv = leftcurv;
        dew->rightcurv = rightcurv;
        dew->hsuccess = 1;
        dew->samphdispar = fpixh;
    }

    return dew;
}


/*!
 *  dewarpWrite()
 *
 *      Input:  filename
 *              dew
 *      Return: 0 if OK, 1 on error
 */
l_int32
dewarpWrite(const char  *filename,
            L_DEWARP    *dew)
{
FILE  *fp;

    PROCNAME("dewarpWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!dew)
        return ERROR_INT("dew not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "wb")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (dewarpWriteStream(fp, dew))
        return ERROR_INT("dew not written to stream", procName, 1);
    fclose(fp);

    return 0;
}


/*!
 *  dewarpWriteStream()
 *
 *      Input:  stream (opened for "wb")
 *              dew
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This should not be written if there is no sampled
 *          vertical disparity array, which means that no model has
 *          been built for this page.
 */
l_int32
dewarpWriteStream(FILE      *fp,
                  L_DEWARP  *dew)
{
l_int32  vdispar, hdispar;

    PROCNAME("dewarpWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!dew)
        return ERROR_INT("dew not defined", procName, 1);

    fprintf(fp, "\nDewarp Version %d\n", DEWARP_VERSION_NUMBER);
    fprintf(fp, "pageno = %d\n", dew->pageno);
    fprintf(fp, "hasref = %d, refpage = %d\n", dew->hasref, dew->refpage);
    fprintf(fp, "sampling = %d, redfactor = %d\n",
            dew->sampling, dew->redfactor);
    fprintf(fp, "nlines = %d, minlines = %d\n", dew->nlines, dew->minlines);
    fprintf(fp, "w = %d, h = %d\n", dew->w, dew->h);
    fprintf(fp, "nx = %d, ny = %d\n", dew->nx, dew->ny);
    vdispar = (dew->sampvdispar) ? 1 : 0;
    hdispar = (dew->samphdispar) ? 1 : 0;
    fprintf(fp, "vert_dispar = %d, horiz_dispar = %d\n", vdispar, hdispar);
    if (vdispar)
        fprintf(fp, "min line curvature = %d, max line curvature = %d\n",
                dew->mincurv, dew->maxcurv);
    if (hdispar) {
        fprintf(fp, "left edge slope = %d, right edge slope = %d\n",
                dew->leftslope, dew->rightslope);
        fprintf(fp, "left edge curvature = %d, right edge curvature = %d\n",
                dew->leftcurv, dew->rightcurv);
    }
    if (vdispar) fpixWriteStream(fp, dew->sampvdispar);
    if (hdispar) fpixWriteStream(fp, dew->samphdispar);
    fprintf(fp, "\n");

    if (!vdispar)
        L_WARNING("no disparity arrays!\n", procName);
    return 0;
}


/*----------------------------------------------------------------------*
 *                       Dewarpa serialized I/O                          *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpaRead()
 *
 *      Input:  filename
 *      Return: dewa, or null on error
 */
L_DEWARPA *
dewarpaRead(const char  *filename)
{
FILE       *fp;
L_DEWARPA  *dewa;

    PROCNAME("dewarpaRead");

    if (!filename)
        return (L_DEWARPA *)ERROR_PTR("filename not defined", procName, NULL);
    if ((fp = fopenReadStream(filename)) == NULL)
        return (L_DEWARPA *)ERROR_PTR("stream not opened", procName, NULL);

    if ((dewa = dewarpaReadStream(fp)) == NULL) {
        fclose(fp);
        return (L_DEWARPA *)ERROR_PTR("dewa not read", procName, NULL);
    }

    fclose(fp);
    return dewa;
}


/*!
 *  dewarpaReadStream()
 *
 *      Input:  stream
 *      Return: dewa, or null on error
 *
 *  Notes:
 *      (1) The serialized dewarp contains a Numa that gives the
 *          (increasing) page number of the dewarp structs that are
 *          contained.
 *      (2) Reference pages are added in after readback.
 */
L_DEWARPA *
dewarpaReadStream(FILE  *fp)
{
l_int32     i, version, ndewarp, maxpage;
l_int32     sampling, redfactor, minlines, maxdist, useboth;
l_int32     max_linecurv, min_diff_linecurv, max_diff_linecurv;
l_int32     max_edgeslope, max_edgecurv, max_diff_edgecurv;
L_DEWARP   *dew;
L_DEWARPA  *dewa;
NUMA       *namodels;

    PROCNAME("dewarpaReadStream");

    if (!fp)
        return (L_DEWARPA *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nDewarpa Version %d\n", &version) != 1)
        return (L_DEWARPA *)ERROR_PTR("not a dewarpa file", procName, NULL);
    if (version != DEWARP_VERSION_NUMBER)
        return (L_DEWARPA *)ERROR_PTR("invalid dewarp version", procName, NULL);

    if (fscanf(fp, "ndewarp = %d, maxpage = %d\n", &ndewarp, &maxpage) != 2)
        return (L_DEWARPA *)ERROR_PTR("read fail for maxpage+", procName, NULL);
    if (fscanf(fp,
               "sampling = %d, redfactor = %d, minlines = %d, maxdist = %d\n",
               &sampling, &redfactor, &minlines, &maxdist) != 4)
        return (L_DEWARPA *)ERROR_PTR("read fail for 4 params", procName, NULL);
    if (fscanf(fp,
          "max_linecurv = %d, min_diff_linecurv = %d, max_diff_linecurv = %d\n",
          &max_linecurv, &min_diff_linecurv, &max_diff_linecurv) != 3)
        return (L_DEWARPA *)ERROR_PTR("read fail for linecurv", procName, NULL);
    if (fscanf(fp,
              "max_edgeslope = %d, max_edgecurv = %d, max_diff_edgecurv = %d\n",
               &max_edgeslope, &max_edgecurv, &max_diff_edgecurv) != 3)
        return (L_DEWARPA *)ERROR_PTR("read fail for edgecurv", procName, NULL);
    if (fscanf(fp, "fullmodel = %d\n", &useboth) != 1)
        return (L_DEWARPA *)ERROR_PTR("read fail for useboth", procName, NULL);

    dewa = dewarpaCreate(maxpage + 1, sampling, redfactor, minlines, maxdist);
    dewa->maxpage = maxpage;
    dewa->max_linecurv = max_linecurv;
    dewa->min_diff_linecurv = min_diff_linecurv;
    dewa->max_diff_linecurv = max_diff_linecurv;
    dewa->max_edgeslope = max_edgeslope;
    dewa->max_edgecurv = max_edgecurv;
    dewa->max_diff_edgecurv = max_diff_edgecurv;
    dewa->useboth = useboth;
    namodels = numaCreate(ndewarp);
    dewa->namodels = namodels;
    for (i = 0; i < ndewarp; i++) {
        if ((dew = dewarpReadStream(fp)) == NULL) {
            L_ERROR("read fail for dew[%d]\n", procName, i);
            return NULL;
        }
        dewarpaInsertDewarp(dewa, dew);
        numaAddNumber(namodels, dew->pageno);
    }

        /* Validate the models and insert reference models */
    dewarpaInsertRefModels(dewa, 0, 0);

    return dewa;
}


/*!
 *  dewarpaWrite()
 *
 *      Input:  filename
 *              dewa
 *      Return: 0 if OK, 1 on error
 */
l_int32
dewarpaWrite(const char  *filename,
             L_DEWARPA   *dewa)
{
FILE  *fp;

    PROCNAME("dewarpaWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "wb")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (dewarpaWriteStream(fp, dewa))
        return ERROR_INT("dewa not written to stream", procName, 1);
    fclose(fp);
    return 0;
}


/*!
 *  dewarpaWriteStream()
 *
 *      Input:  stream (opened for "wb")
 *              dewa
 *      Return: 0 if OK, 1 on error
 */
l_int32
dewarpaWriteStream(FILE       *fp,
                   L_DEWARPA  *dewa)
{
l_int32  ndewarp, i, pageno;

    PROCNAME("dewarpaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

        /* Generate the list of page numbers for which a model exists.
         * Note that no attempt is made to determine if the model is
         * valid, because that determination is associated with
         * using the model to remove the warping, which typically
         * can happen later, after all the models have been built. */
    dewarpaListPages(dewa);
    if (!dewa->namodels)
        return ERROR_INT("dewa->namodels not made", procName, 1);
    ndewarp = numaGetCount(dewa->namodels);  /*  with actual page models */

    fprintf(fp, "\nDewarpa Version %d\n", DEWARP_VERSION_NUMBER);
    fprintf(fp, "ndewarp = %d, maxpage = %d\n", ndewarp, dewa->maxpage);
    fprintf(fp, "sampling = %d, redfactor = %d, minlines = %d, maxdist = %d\n",
            dewa->sampling, dewa->redfactor, dewa->minlines, dewa->maxdist);
    fprintf(fp,
        "max_linecurv = %d, min_diff_linecurv = %d, max_diff_linecurv = %d\n",
        dewa->max_linecurv, dewa->min_diff_linecurv, dewa->max_diff_linecurv);
    fprintf(fp,
            "max_edgeslope = %d, max_edgecurv = %d, max_diff_edgecurv = %d\n",
            dewa->max_edgeslope, dewa->max_edgecurv, dewa->max_diff_edgecurv);
    fprintf(fp, "fullmodel = %d\n", dewa->useboth);
    for (i = 0; i < ndewarp; i++) {
        numaGetIValue(dewa->namodels, i, &pageno);
        dewarpWriteStream(fp, dewarpaGetDewarp(dewa, pageno));
    }

    return 0;
}

