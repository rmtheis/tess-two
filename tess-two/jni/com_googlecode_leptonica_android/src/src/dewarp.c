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
 *  dewarp.c
 *
 *      Create/destroy dewarp
 *          L_DEWARP          *dewarpCreate()
 *          L_DEWARP          *dewarpCreateReference()
 *          void               dewarpDestroy()
 *
 *      Create/destroy dewarpa
 *          L_DEWARPA         *dewarpaCreate()
 *          L_DEWARPA         *dewarpaCreateFromPixacomp()
 *          void               dewarpaDestroy()
 *          l_int32            dewarpaDestroyDewarp()
 *
 *      Insert in dewarpa
 *          l_int32            dewarpaInsertDewarp()
 *          l_int32            dewarpaExtendArrayToSize()
 *
 *      Build warp model
 *          l_int32            dewarpBuildModel()
 *          l_int32            dewarpFindVertDisparity()
 *          l_int32            dewarpFindHorizDisparity()
 *          PTAA              *dewarpGetTextlineCenters()
 *          static PTA        *dewarpGetMeanVerticals()
 *          PTAA              *dewarpRemoveShortLines()
 *          static l_int32     dewarpGetLineEndpoints()
 *          static l_int32     dewarpQuadraticLSF()
 *
 *      Apply warping disparity array
 *          l_int32            dewarpaApplyDisparity()
 *          static l_int32     pixApplyVertDisparity()
 *          static l_int32     pixApplyHorizDisparity()
 *
 *      Stripping out data and populating full res disparity
 *          l_int32            dewarpMinimize()
 *          l_int32            dewarpPopulateFullRes()
 *
 *      Accessors
 *          L_DEWARP          *dewarpaGetDewarp()
 *          PIX               *dewarpaGetResult()
 *          PIX               *dewarpGetResult()
 *
 *      Operations on dewarpa
 *          L_DEWARP          *dewarpaGetDewarp()
 *          l_int32            dewarpaListPages()
 *          l_int32            dewarpaSetValidModels()
 *          l_int32            dewarpaInsertRefModels()
 *          l_int32            dewarpaStripRefModels()
 *
 *      Setting parameters to control rendering from the model
 *          l_int32            dewarpaSetCurvatures()
 *          l_int32            dewarpaUseFullModel()
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
 *      Dewarp debugging output
 *          l_int32            dewarpaInfo()
 *          l_int32            dewarpaModelStats()
 *          static l_int32     dewarpaTestForValidModel()
 *          l_int32            dewarpaShowArrays()
 *          l_int32            dewarpDebug()
 *          l_int32            dewarpShowResults()
 *
 *      Rendering helpers
 *          static l_int32     pixRenderFlats()
 *          static l_int32     pixRenderHorizEndPoints
 *
 *      Static functions not presently in use
 *          static FPIX       *fpixSampledDisparity()
 *          static FPIX       *fpixExtraHorizDisparity()
 *
 *
 *  Examples of usage
 *  =================
 *
 *  Basic functioning to dewarp a specific single page:
 *     // Make the Dewarpa for the pages
 *     L_Dewarpa *dewa = dewarpaCreate(1, 30, 1, 15, 50);
 *
 *     // Do the page: start with a binarized image
 *     Pix *pixb = "binarize"(pixs);
 *     // Initialize a Dewarp for this page (say, page 214)
 *     L_Dewarp *dew = dewarpCreate(pixb, 214);
 *     // Insert in Dewarpa and obtain parameters for building the model
 *     dewarpaInsertDewarp(dewa, dew);
 *     // Do the work
 *     dewarpBuildModel(dew, NULL);  // no debugging
 *     // Optionally set rendering parameters
 *     dewarpaSetCurvatures(dewa, 5, -1, -1, -1);
 *     dewarpaUseFullModel(dewa, 1);  // requires a full model
 *     // Apply model to the input pixs; the result goes into dew->pixd
 *     l_int32 ret = dewarpaApplyDisparity(dewa, 214, pixs, NULL);
 *     Pix *pixd = NULL;
 *     if (!ret)
 *         pixd = dewarpGetResult(dew);
 *     else  // dew is probably destroyed because models are invalid
 *         pixd = pixClone(pixs);
 *     pixDestroy(&pixb);
 *
 *  Basic functioning to dewarp many pages:
 *     // Make the Dewarpa for the set of pages; use fullres 1 bpp
 *     L_Dewarpa *dewa = dewarpaCreate(10, 30, 1, 15, 50);
 *     // Optionally set rendering parameters
 *     dewarpaSetCurvatures(dewa, 10, -1, -1, -1);
 *     dewarpaUseFullModel(dewa, 0);  // if partial (just vertical), use it
 *
 *     // Do first page: start with a binarized image
 *     Pix *pixb = "binarize"(pixs);
 *     // Initialize a Dewarp for this page (say, page 1)
 *     L_Dewarp *dew = dewarpCreate(pixb, 1);
 *     // Insert in Dewarpa and obtain parameters for building the model
 *     dewarpaInsertDewarp(dewa, dew);
 *     // Do the work
 *     dewarpBuildModel(dew, NULL);  // no debugging
 *     dewarpMinimze(dew);  // remove most heap storage
 *     pixDestroy(&pixb);
 *
 *     // Do the other pages the same way
 *     ...
 *
 *     // Apply models to each page.
 *     // The call to dewarpaInsertRefModels() is optional, because it
 *     // be called by dewarpaApplyDisparity() on the first page it acts on.
 *     dewarpaInsertRefModels(dewa, 1);  // use debug flag to get more detailed
 *                                       // information about the page models
 *     [For each page, where pixs is the fullres image to be dewarped] {
 *         L_Dewarp *dew = dewarpaGetDewarp(dewa, pageno);
 *         if (dew) {  // disparity model exists
 *             l_int32 ret = dewarpaApplyDisparity(dewa, pageno, pixs, NULL);
 *             Pix *pixd;
 *             if (!ret) {
 *                 pixd = dewarpGetResult(dew);  // save the result
 *                 dewarpMinimize(dew);  // clean out the pix and fpix arrays
 *             } else
 *                 pixd = pixClone(pixs);  // unchanged
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
 *     Pix *pixtiny = pixCreate(1, 1, 1);
 *     PixaComp *pixac = pixacompCreateInitialized(n, firstpage, pixtiny,
 *                                                 IFF_TIFF_G4);
 *     pixDestroy(&pixtiny);
 *     for (i = firstpage; i <= lastpage; i++) {
 *         Pix *pixb = "binarize"(pixs);
 *         pixacompReplacePix(pixac, i, pixb, IFF_TIFF_G4);
 *         pixDestroy(&pixb);
 *     }
 *
 *     // (2) Make the Dewarpa for the pages.
 *     L_Dewarpa *dewa =
 *           dewarpaCreateFromPixacomp(pixac, 30, 15, 20);
 *     dewarpaUseFullModel(dewa, 1);  // require full model for use
 *
 *     // (3) Finally, apply the models.  For page 'firstpage' with image pixs:
 *     l_int32 ret = dewarpaApplyDisparity(dewa, firstpage, pixs, NULL);
 *     Pix *pixd;
 *     if (!ret) {  // The result can be extracted:
 *         L_Dewarp *dew = dewarpaGetDewarp(dewa, firstpage);
 *         pixd = dewarpGetResult(dew);
 *         dewarpMinimize(dew);
 *     } else  // failed
 *         pixd = pixClone(pixs);
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
 *     dewarpaInsertRefModels(dewa, 1);
 *     // Use the assigned page model on page 'pageno', apply the
 *     // model to 'newpix', and save the result in the dew for 'pageno'.
 *     dewarpaApplyDisparity(dewa, pageno, newpix, NULL);
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
 *  The goal is to compute the "disparity" field, D(x,y), which
 *  is actually a vector composed of the horizontal and vertical
 *  disparity fields H(x,y) and V(x,y).  Each of these is a local
 *  function that gives the amount each point in the image is
 *  required to move in order to rectify the horizontal and vertical
 *  lines.
 *
 *  Effects (1) and (2) can be compensated for by calibrating
 *  the scene, using a flat page with horizontal and vertical lines.
 *  Then H(x,y) and V(x,y) can be found as two (non-parametric) arrays
 *  of values.  Suppose this has been done.  Then the remaining
 *  distortion is due to (3).
 *
 *  Now, if we knew everywhere the angle between the perpendicular
 *  to the paper and the optic axis (call it 'alpha'), the
 *  actual shape of the page could in principle be found by integration,
 *  and the remaining disparities, H(x,y) and V(x,y), could be
 *  found.  But we don't know alpha.  If there are text lines on
 *  the page, we can assume they should be horizontal, so we can
 *  compute the vertical disparity, which is the local translation
 *  required to make the text lines parallel to the rasters.
 *
 *  The basic question relating to (3) is this:
 *
 *     Is it possible, using the shape of the text lines alone,
 *     to compute both the vertical and horizontal disparity fields?
 *
 *  The problem is to find H(x,y).  In an image with horizontal
 *  text lines, the only vertical "lines" that we can infer are
 *  perhaps the left and right margins.
 *
 *  Start with a simple case.  Suppose the binding is along a
 *  vertical line, and the page curvature is independent of y.
 *  Then if the page curves in toward the binding, there will be
 *  a fractional foreshortening of that region in the x-direction, going
 *  as the sine of the angle between the optic axis and local the
 *  normal to the page.  For this situation, the horizontal
 *  disparity is independent of y: H(x,y) == H(x).
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
 *  There should be a simple mathematical relation between
 *  the horizontal and vertical disparities for the situation
 *  where the paper bends without stretching or kinking.
 *  I was hoping that we would get a relation between H and V such
 *  as Hx(x,y) ~ Vy(x,y), which would imply that H and V are real
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
 *  and constitute the bulk of the work done in dewarpBuildModel().
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

static PTA *dewarpGetMeanVerticals(PIX *pixs, l_int32 x, l_int32 y);
static l_int32 dewarpGetLineEndpoints(l_int32 h, PTAA *ptaa, PTA **pptal,
                                      PTA **pptar);
static l_int32 dewarpFindLongLines(PTA *ptal, PTA *ptar, PTA **pptald,
                                   PTA **pptard);
static l_int32 dewarpQuadraticLSF(PTA *ptad, l_float32 *pa, l_float32 *pb,
                                  l_float32 *pc, l_float32 *pmederr);
static PIX *pixApplyVertDisparity(L_DEWARP *dew, PIX *pixs);
static PIX * pixApplyHorizDisparity(L_DEWARP *dew, PIX *pixs);
static l_int32 pixRenderFlats(PIX *pixs, NUMA *naflats, l_int32 linew);
static l_int32 pixRenderHorizEndPoints(PIX *pixs, PTA *ptal, PTA *ptar,
                                       l_uint32 color);
static l_int32 dewarpaTestForValidModel(L_DEWARPA *dewa, L_DEWARP *dew);


#ifndef  NO_CONSOLE_IO
#define  DEBUG_TEXTLINE_CENTERS    0   /* set this to 1 for debuging */
#define  DEBUG_SHORT_LINES         0   /* ditto */
#else
#define  DEBUG_TEXTLINE_CENTERS    0   /* always must be 0 */
#define  DEBUG_SHORT_LINES         0   /* ditto */
#endif  /* !NO_CONSOLE_IO */

    /* Special parameter values */
static const l_int32     MIN_ARRAY_SAMPLING = 8;
static const l_int32     DEFAULT_ARRAY_SAMPLING = 30;
static const l_int32     DEFAULT_MIN_LINES = 15;
static const l_int32     DEFAULT_MAX_REF_DIST = 30;
static const l_float32   DEFAULT_SLOPE_FACTOR = 2000.;

static const l_int32     INITIAL_PTR_ARRAYSIZE = 20;   /* n'import quoi */
static const l_int32     MAX_PTR_ARRAYSIZE = 10000;

static const l_float32   MIN_RATIO_LINES_TO_HEIGHT = 0.45;
static const l_int32     DEFAULT_MIN_MEDCURV = 0;
static const l_int32     DEFAULT_MAX_MEDCURV = 150;
static const l_int32     DEFAULT_MAX_LEFTCURV = 60;
static const l_int32     DEFAULT_MAX_RIGHTCURV = 60;


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
 *  dewarpCreateReference()
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
dewarpCreateReference(l_int32  pageno,
                      l_int32  refpage)
{
L_DEWARP  *dew;

    PROCNAME("dewarpCreateReference");

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
        L_WARNING("ptr address is null!", procName);
        return;
    }
    if ((dew = *pdew) == NULL)
        return;

    pixDestroy(&dew->pixs);
    pixDestroy(&dew->pixd);
    fpixDestroy(&dew->sampvdispar);
    fpixDestroy(&dew->samphdispar);
    fpixDestroy(&dew->fullvdispar);
    fpixDestroy(&dew->fullhdispar);
    numaDestroy(&dew->naflats);
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
 *            sampling (use -1 or 0 for default value; otherwise minimum of 5)
 *            redfactor (of input images: 1 is full resolution; 2 is 2x reduced)
 *            minlines (minimum number of lines to accept; e.g., 10)
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
 *          number is 15.  Use a smaller number if you are willing to
 *          accept a questionable array, but not smaller than 4.
 *      (5) When a model can't be built for a page, it looks up to @maxdist
 *          in either direction for a valid model with the same page parity.
 *          Use -1 for default value; use 0 to avoid using a ref model.
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
        return (L_DEWARPA *)ERROR_PTR("redfactor not in {1,2}", procName, NULL);
    if (minlines < 4) {
        L_WARNING_INT("minlines < 4; setting to default value (%d)",
                      procName, DEFAULT_MIN_LINES);
        minlines = DEFAULT_MIN_LINES;
    }

    if ((dewa = (L_DEWARPA *)CALLOC(1, sizeof(L_DEWARPA))) == NULL)
        return (L_DEWARPA *)ERROR_PTR("dewa not made", procName, NULL);

    if ((dewa->dewarp = (L_DEWARP **)CALLOC(nptrs, sizeof(L_DEWARPA *)))
            == NULL)
        return (L_DEWARPA *)ERROR_PTR("dewarp ptrs not made", procName, NULL);
    dewa->nalloc = nptrs;

    if (sampling < MIN_ARRAY_SAMPLING) {
         L_WARNING("sampling too small; setting to default", procName);
         sampling = DEFAULT_ARRAY_SAMPLING;
    }
    dewa->sampling = sampling;
    dewa->redfactor = redfactor;
    dewa->minlines = minlines;
    if (maxdist < 0) {
         L_WARNING("maxdist must be >= 0; setting to default", procName);
         maxdist = DEFAULT_MAX_REF_DIST;
    }
    dewa->maxdist = maxdist;
    dewa->min_medcurv = DEFAULT_MIN_MEDCURV;
    dewa->max_medcurv = DEFAULT_MAX_MEDCURV;
    dewa->max_leftcurv = DEFAULT_MAX_LEFTCURV;
    dewa->max_rightcurv = DEFAULT_MAX_RIGHTCURV;

    return dewa;
}


/*!
 *  dewarpaCreateFromPixacomp()
 *
 *     Input: pixac (pixacomp of G4, 1 bpp images; with 1x1x1 placeholders)
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
            dewarpBuildModel(dew, NULL);
            if (!dew->success) {  /* will need to use model from nearby page */
                dewarpaDestroyDewarp(dewa, pageno);
                L_ERROR_INT("unable to build model for page %d", procName, i);
                continue;
            }
                /* Remove all extraneous data */
            dewarpMinimize(dew);
        }
        pixDestroy(&pixt);
    }
    dewarpaInsertRefModels(dewa, 0);

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
        L_WARNING("ptr address is null!", procName);
        return;
    }
    if ((dewa = *pdewa) == NULL)
        return;

    for (i = 0; i < dewa->nalloc; i++) {
        if ((dew = dewa->dewarp[i]) != NULL)
            dewarpDestroy(&dew);
    }
    numaDestroy(&dewa->namodels);
    numaDestroy(&dewa->napages);

    FREE(dewa->dewarp);
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
 *                              Add to Dewarpa                          *
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
        dewarpaExtendArrayToSize(dewa, newsize);

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
 *  dewarpaExtendArrayToSize()
 *
 *      Input:  dewa
 *              size (new size of dewarpa array)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) If necessary, reallocs new dewarpa ptr array to @size.
 */
l_int32
dewarpaExtendArrayToSize(L_DEWARPA  *dewa,
                         l_int32     size)
{
    PROCNAME("dewarpaExtendArrayToSize");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    if (size > dewa->nalloc) {
        if ((dewa->dewarp = (L_DEWARP **)reallocNew((void **)&dewa->dewarp,
                                            sizeof(L_DEWARP *) * dewa->nalloc,
                                            size * sizeof(L_DEWARP *))) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);
        dewa->nalloc = size;
    }
    return 0;
}


/*----------------------------------------------------------------------*
 *                            Build warp model                          *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpBuildModel()
 *
 *      Input:  dew
 *              debugfile (use null to skip writing this)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is the basic function that builds the horizontal and
 *          vertical disparity arrays, which allow determination of the
 *          src pixel in the input image corresponding to each
 *          dest pixel in the dewarped image.
 *      (2) The method is as follows:
 *          (a) Estimate the points along the centers of all the
 *              long textlines.  If there are too few lines, no
 *              disparity models are built.
 *          (b) From the vertical deviation of the lines, estimate
 *              the vertical disparity.
 *          (c) From the ends of the lines, estimate the horizontal
 *              disparity, assuming that the text is made of lines
 *              that are left and right justified.
 *          (d) One can also compute an additional contribution to the
 *              horizontal disparity, inferred from slopes of the top
 *              and bottom lines.  We do not do this.
 *      (3) In more detail for the vertical disparity:
 *          (a) Fit a LS quadratic to center locations along each line.
 *              This smooths the curves.
 *          (b) Sample each curve at a regular interval, find the y-value
 *              of the flat point on each curve, and subtract the sampled
 *              curve value from this value.  This is the vertical
 *              disparity at sampled points along each curve.
 *          (c) Fit a LS quadratic to each set of vertically aligned
 *              disparity samples.  This smooths the disparity values
 *              in the vertical direction.  Then resample at the same
 *              regular interval.  We now have a regular grid of smoothed
 *              vertical disparity valuels.
 *      (4) Once the sampled vertical disparity array is found, it can be
 *          interpolated to get a full resolution vertical disparity map.
 *          This can be applied directly to the src image pixels
 *          to dewarp the image in the vertical direction, making
 *          all textlines horizontal.  Likewise, the horizontal
 *          disparity array is used to left- and right-align the
 *          longest textlines.
 */
l_int32
dewarpBuildModel(L_DEWARP    *dew,
                 const char  *debugfile)
{
l_int32  sampling, redfactor, ret;
PIX     *pixs, *pixt1, *pixt2;
PTAA    *ptaa1, *ptaa2;

    PROCNAME("dewarpBuildModel");

    if (!dew)
        return ERROR_INT("dew not defined", procName, 1);

    dew->debug = (debugfile) ? 1 : 0;
    sampling = dew->sampling;
    redfactor = dew->redfactor;
    pixs = dew->pixs;
    if (debugfile) {
        lept_rmdir("dewmod");  /* erase previous images */
        lept_mkdir("dewmod");
        pixDisplayWithTitle(pixs, 0, 0, "pixs", 1);
        pixWrite("/tmp/dewmod/001.png", pixs, IFF_PNG);
    }

        /* Make initial estimate of centers of textlines */
    ptaa1 = dewarpGetTextlineCenters(pixs, DEBUG_TEXTLINE_CENTERS);
    if (debugfile) {
        pixt1 = pixConvertTo32(pixs);
        pixt2 = pixDisplayPtaa(pixt1, ptaa1);
        pixWrite("/tmp/dewmod/002.png", pixt2, IFF_PNG);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }

        /* Remove all lines that are not near the length
         * of the longest line. */
    ptaa2 = dewarpRemoveShortLines(pixs, ptaa1, 0.8, DEBUG_SHORT_LINES);
    if (debugfile) {
        pixt1 = pixConvertTo32(pixs);
        pixt2 = pixDisplayPtaa(pixt1, ptaa2);
        pixWrite("/tmp/dewmod/003.png", pixt2, IFF_PNG);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }
    ptaaDestroy(&ptaa1);
    if (ptaaGetCount(ptaa2) < dew->minlines) {
        ptaaDestroy(&ptaa2);
        return ERROR_INT("insufficient lines to build model", procName, 1);
    }

        /* Get the sampled vertical disparity from the curvature of the
         * text lines.  The disparity array will push pixels down so
         * that each textline is flat and at the position of its
         * lowest point. */
    dewarpFindVertDisparity(dew, ptaa2);

        /* Get the sampled horizontal disparity from the left and right
         * edges of the text.  The disparity array will expand the image
         * linearly outward to align the text edges vertically. */
    ret = dewarpFindHorizDisparity(dew, ptaa2);

        /* Debug output */
    if (debugfile) {
        dewarpPopulateFullRes(dew, NULL);
        pixt1 = fpixRenderContours(dew->fullvdispar, 3.0, 0.3);
        pixWrite("/tmp/dewmod/006.png", pixt1, IFF_PNG);
        pixDisplay(pixt1, 1000, 0);
        pixDestroy(&pixt1);
        if (!ret) {
            pixt1 = fpixRenderContours(dew->fullhdispar, 3.0, 0.3);
            pixWrite("/tmp/dewmod/007.png", pixt1, IFF_PNG);
            pixDisplay(pixt1, 1000, 0);
            pixDestroy(&pixt1);
        }
        convertFilesToPdf("/tmp/dewmod", NULL, 135, 1.0, 0, 0,
                          "Dewarp Build Model", debugfile);
        fprintf(stderr, "pdf file made: %s\n", debugfile);
    }

    dew->success = 1;  /* at least the vertical disparity array is made */
    ptaaDestroy(&ptaa2);
    return 0;
}


/*!
 *  dewarpFindVertDisparity()
 *
 *      Input:  dew
 *              ptaa (unsmoothed lines, not vertically ordered)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This starts with points along the centers of textlines.
 *          It does quadratic fitting (and smoothing), first along the
 *          lines and then in the vertical direction, to generate
 *          the sampled vertical disparity map.  This can then be
 *          interpolated to full resolution and used to remove
 *          the vertical line warping.
 *      (2) The model fails to build if the vertical disparity fails.
 *          This sets the success flag to 1 on success.
 *      (3) Pix debug output goes to /tmp/dewvert/ for collection into
 *          a pdf.  Non-pix debug output goes to /tmp.
 */
l_int32
dewarpFindVertDisparity(L_DEWARP  *dew,
                        PTAA      *ptaa)
{
l_int32     i, j, nlines, nx, ny, sampling;
l_float32   c0, c1, c2, x, y, flaty, val, medval, medvar;
l_float32  *faflats;
NUMA       *nax, *nafit, *nacurve0, *nacurve1, *nacurves, *nat;
NUMA       *naflat, *naflats, *naflatsi;
PIX        *pixt1, *pixt2, *pixcirc;
PTA        *pta, *ptad, *ptacirc;
PTAA       *ptaa0, *ptaa1, *ptaa2, *ptaa3, *ptaa4, *ptaa5, *ptaat;
FPIX       *fpix;

    PROCNAME("dewarpFindVertDisparity");

    if (!dew)
        return ERROR_INT("dew not defined", procName, 1);
    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);

        /* Do quadratic fit to smooth each line.  A single quadratic
         * over the entire width of the line appears to be sufficient.
         * Quartics tend to overfit to noise.  Each line is thus
         * represented by three coefficients: y(x) = c2 * x^2 + c1 * x + c0.
         * Using the coefficients, sample each fitted curve uniformly
         * across the full width of the image.  The result is in ptaa0.  */
    sampling = dew->sampling;
    nx = dew->nx;
    ny = dew->ny;
    nlines = ptaaGetCount(ptaa);
    dew->nlines = nlines;
    ptaa0 = ptaaCreate(nlines);
    nacurve0 = numaCreate(nlines);  /* stores curvature coeff c2 */
    for (i = 0; i < nlines; i++) {  /* for each line */
        pta = ptaaGetPta(ptaa, i, L_CLONE);
        ptaGetQuadraticLSF(pta, &c2, &c1, &c0, NULL);
        numaAddNumber(nacurve0, c2);
        ptad = ptaCreate(nx);
        for (j = 0; j < nx; j++) {  /* uniformly sampled in x */
             x = j * sampling;
             applyQuadraticFit(c2, c1, c0, x, &y);
             ptaAddPt(ptad, x, y);
        }
        ptaaAddPta(ptaa0, ptad, L_INSERT);
        ptaDestroy(&pta);
    }
    if (dew->debug) {
        ptaat = ptaaCreate(nlines);
        for (i = 0; i < nlines; i++) {
            pta = ptaaGetPta(ptaa, i, L_CLONE);
            ptaGetArrays(pta, &nax, NULL);
            ptaGetQuadraticLSF(pta, NULL, NULL, NULL, &nafit);
            ptad = ptaCreateFromNuma(nax, nafit);
            ptaaAddPta(ptaat, ptad, L_INSERT);
            ptaDestroy(&pta);
            numaDestroy(&nax);
            numaDestroy(&nafit);
        }
        pixt1 = pixConvertTo32(dew->pixs);
        pixt2 = pixDisplayPtaa(pixt1, ptaat);
        pixWrite("/tmp/dewmod/004a.png", pixt2, IFF_PNG);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        ptaaDestroy(&ptaat);
    }

        /* Remove lines with outlier curvatures */
    numaGetMedianVariation(nacurve0, &medval, &medvar);
    L_INFO_INT("\nPage %d", procName, dew->pageno);
    L_INFO_FLOAT2("Pass 1: Curvature: medval = %f, medvar = %f",
                  procName, medval, medvar);
    ptaa1 = ptaaCreate(nlines);
    nacurve1 = numaCreate(nlines);
    for (i = 0; i < nlines; i++) {  /* for each line */
        numaGetFValue(nacurve0, i, &val);
        if (L_ABS(val - medval) > 7.0 * medvar)
            continue;
        pta = ptaaGetPta(ptaa0, i, L_CLONE);
        ptaaAddPta(ptaa1, pta, L_INSERT);
        numaAddNumber(nacurve1, val);
    }
    nlines = ptaaGetCount(ptaa1);
    numaDestroy(&nacurve0);

        /* Save median absolute curvature (in micro-units) */
    nat = numaMakeAbsValue(NULL, nacurve1);
    numaGetMedian(nat, &medval);
    dew->medcurv = lept_roundftoi(1000000. * medval);
    L_INFO_INT("Pass 2: Median abs curvature = %d", procName, dew->medcurv);
    numaDestroy(&nat);

        /* Find and save the lowest (max y) points in each curve.
         * These are typically locations where the slope is zero.  */
    naflat = numaCreate(nlines);
    for (i = 0; i < nlines; i++) {
        pta = ptaaGetPta(ptaa1, i, L_CLONE);
        numaGetFValue(nacurve1, i, &c2);
        ptaGetRange(pta, NULL, NULL, NULL, &flaty);
        numaAddNumber(naflat, flaty);
        ptaDestroy(&pta);
    }

        /* Sort the lines in ptaa1c by their vertical position, going down */
    naflatsi = numaGetSortIndex(naflat, L_SORT_INCREASING);
    naflats = numaSortByIndex(naflat, naflatsi);
    nacurves = numaSortByIndex(nacurve1, naflatsi);
    dew->naflats = naflats;
    dew->nacurves = nacurves;
    ptaa2 = ptaaSortByIndex(ptaa1, naflatsi);
    numaDestroy(&naflat);
    numaDestroy(&nacurve1);
    numaDestroy(&naflatsi);
    if (dew->debug) {
        numaWrite("/tmp/naflats.na", naflats);
        numaWrite("/tmp/nacurves.na", nacurves);
        pixt1 = pixConvertTo32(dew->pixs);
        ptacirc = generatePtaFilledCircle(5);
        pixcirc = pixGenerateFromPta(ptacirc, 11, 11);
        srand(3);
        pixDisplayPtaaPattern(pixt1, pixt1, ptaa2, pixcirc, 5, 5);
        srand(3);  /* use the same colors for text and reference lines */
        pixRenderFlats(pixt1, naflats, 2);
        pixWrite("/tmp/dewmod/004b.png", pixt1, IFF_PNG);
        pixDisplay(pixt1, 0, 0);
        ptaDestroy(&ptacirc);
        pixDestroy(&pixcirc);
        pixDestroy(&pixt1);
    }

        /* Convert the sampled points in ptaa2 to a sampled disparity with
         * with respect to the flat point in the curve.  The disparity
         * is the distance the point needs to move; plus is downward.  */
    ptaa3 = ptaaCreate(nlines);
    for (i = 0; i < nlines; i++) {
        pta = ptaaGetPta(ptaa2, i, L_CLONE);
        numaGetFValue(naflats, i, &flaty);
        ptad = ptaCreate(nx);
        for (j = 0; j < nx; j++) {
            ptaGetPt(pta, j, &x, &y);
            ptaAddPt(ptad, x, flaty - y);
        }
        ptaaAddPta(ptaa3, ptad, L_INSERT);
        ptaDestroy(&pta);
    }
    if (dew->debug) {
        ptaaWrite("/tmp/ptaa3.ptaa", ptaa3, 0);
    }

        /* Generate ptaa4 by taking vertical 'columns' from ptaa3.
         * We want to fit the vertical disparity on the column to the
         * vertical position of the line, which we call 'y' here and
         * obtain from naflats.  So each pta in ptaa4 is the set of
         * vertical disparities down a column of points.  The columns
         * in ptaa4 are equally spaced in x. */
    ptaa4 = ptaaCreate(nx);
    faflats = numaGetFArray(naflats, L_NOCOPY);
    for (j = 0; j < nx; j++) {
        pta = ptaCreate(nlines);
        for (i = 0; i < nlines; i++) {
            y = faflats[i];
            ptaaGetPt(ptaa3, i, j, NULL, &val);  /* disparity value */
            ptaAddPt(pta, y, val);
        }
        ptaaAddPta(ptaa4, pta, L_INSERT);
    }
    if (dew->debug) {
        ptaaWrite("/tmp/ptaa4.ptaa", ptaa4, 0);
    }

        /* Do quadratic fit vertically on each of the pixel columns
         * in ptaa4, for the vertical displacement (which identifies the
         * src pixel(s) for each dest pixel) as a function of y (the
         * flat points for each line).  Then generate ptaa5 by sampling
         * the fitted vertical displacement on a regular grid in the
         * vertical direction.  Each pta in ptaa5 gives the vertical
         * displacement for regularly sampled y values at a fixed x. */
    ptaa5 = ptaaCreate(nx);  /* uniformly sampled across full height of image */
    for (j = 0; j < nx; j++) {  /* for each column */
        pta = ptaaGetPta(ptaa4, j, L_CLONE);
        ptaGetQuadraticLSF(pta, &c2, &c1, &c0, NULL);
        ptad = ptaCreate(ny);
        for (i = 0; i < ny; i++) {  /* uniformly sampled in y */
             y = i * sampling;
             applyQuadraticFit(c2, c1, c0, y, &val);
             ptaAddPt(ptad, y, val);
        }
        ptaaAddPta(ptaa5, ptad, L_INSERT);
        ptaDestroy(&pta);
    }
    if (dew->debug) {
        ptaaWrite("/tmp/ptaa5.ptaa", ptaa5, 0);
        convertFilesToPdf("/tmp/dewmod", "004", 135, 1.0, 0, 0,
                          "Dewarp Vert Disparity", "/tmp/dewarp_vert.pdf");
        fprintf(stderr, "pdf file made: /tmp/dewarp_vert.pdf\n");
    }

        /* Save the result in a fpix at the specified subsampling  */
    fpix = fpixCreate(nx, ny);
    for (i = 0; i < ny; i++) {
        for (j = 0; j < nx; j++) {
            ptaaGetPt(ptaa5, j, i, NULL, &val);
            fpixSetPixel(fpix, j, i, val);
        }
    }
    dew->sampvdispar = fpix;
    dew->success = 1;

    ptaaDestroy(&ptaa0);
    ptaaDestroy(&ptaa1);
    ptaaDestroy(&ptaa2);
    ptaaDestroy(&ptaa3);
    ptaaDestroy(&ptaa4);
    ptaaDestroy(&ptaa5);
    return 0;
}


/*!
 *  dewarpFindHorizDisparity()
 *
 *      Input:  dew
 *              ptaa (unsmoothed lines, not vertically ordered)
 *      Return: 0 if OK, 1 on error
 *
 *      (1) This is not required for a successful model; only the vertical
 *          disparity is required.  This will not be called if the
 *          function to build the vertical disparity fails.
 *      (2) Debug output goes to /tmp/dewhoriz/ for collection into a pdf.
 */
l_int32
dewarpFindHorizDisparity(L_DEWARP  *dew,
                         PTAA      *ptaa)
{
l_int32    i, j, h, nx, ny, sampling, evenpage, ret;
l_float32  c0, c1, cl0, cl1, cl2, cr0, cr1, cr2;
l_float32  x, y, minl, maxl, minr, maxr, refl, refr;
l_float32  val, mederr, leftcurv, rightcurv;
NUMA      *nald, *nard;
PIX       *pixt1;
PTA       *ptal, *ptar;  /* left and right end points of lines */
PTA       *ptalf, *ptarf;  /* left and right block, fitted, uniform spacing */
PTA       *pta, *ptat, *pta1, *pta2, *ptald, *ptard;
PTAA      *ptaah;
FPIX      *fpix;

    PROCNAME("dewarpFindHorizDisparity");

    if (!dew)
        return ERROR_INT("dew not defined", procName, 1);
    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);

        /* Get the endpoints of the lines */
    h = pixGetHeight(dew->pixs);
    ret = dewarpGetLineEndpoints(h, ptaa, &ptal, &ptar);
    if (ret) {
        L_INFO("Horiz disparity not built", procName);
        return 1;
    }
    if (dew->debug) {
        ptaWrite("/tmp/endpts_left.pta", ptal, 1);
        ptaWrite("/tmp/endpts_right.pta", ptar, 1);
    }

        /* Do a quadratic fit to the left and right endpoints of the
         * longest lines.  Each line is represented by 3 coefficients:
         *     x(y) = c2 * y^2 + c1 * y + c0.
         * Using the coefficients, sample each fitted curve uniformly
         * along the full height of the image.
         * TODO: Set right edge disparity to 0 if not flush-right aligned */
    sampling = dew->sampling;
    nx = dew->nx;
    ny = dew->ny;

        /* Find the top and bottom set of long lines, defined by being
         * within 3% of the length of the longest line in each set.
         * Quit if there are not at least 3 lines in each set. */
    ptald = ptard = NULL;  /* end points of longest lines */
    ret = dewarpFindLongLines(ptal, ptar, &ptald, &ptard);
    if (ret) {
        L_INFO("Horiz disparity not built", procName);
        ptaDestroy(&ptal);
        ptaDestroy(&ptar);
        return 1;
    }

        /* Fit the left side, using quadratic LSF on the set of long
         * lines.  It is not necessary to use the noisy LSF fit
         * function, because we've removed outlier end points by
         * selecting the long lines.  Then uniformly sample along
         * this fitted curve. */
    dewarpQuadraticLSF(ptald, &cl2, &cl1, &cl0, &mederr);
    leftcurv = cl2;
    dew->leftcurv = lept_roundftoi(1000000. * leftcurv);
    L_INFO_FLOAT("Left quad LSF median error = %5.2f", procName,  mederr);
    L_INFO_INT("Left edge curvature = %d", procName, dew->leftcurv);
    ptalf = ptaCreate(ny);
    for (i = 0; i < ny; i++) {  /* uniformly sampled in y */
        y = i * sampling;
        applyQuadraticFit(cl2, cl1, cl0, y, &x);
        ptaAddPt(ptalf, x, y);
    }

        /* Fit the right side in the same way. */
    dewarpQuadraticLSF(ptard, &cr2, &cr1, &cr0, &mederr);
    rightcurv = cr2;
    dew->rightcurv = lept_roundftoi(1000000. * rightcurv);
    L_INFO_FLOAT("Right quad LSF median error = %5.2f", procName,  mederr);
    L_INFO_INT("Right edge curvature = %d", procName, dew->rightcurv);
    ptarf = ptaCreate(ny);
    for (i = 0; i < ny; i++) {  /* uniformly sampled in y */
        y = i * sampling;
        applyQuadraticFit(cr2, cr1, cr0, y, &x);
        ptaAddPt(ptarf, x, y);
    }

    if (dew->debug) {
        PTA  *ptalft, *ptarft;
        h = pixGetHeight(dew->pixs);
        pta1 = ptaCreate(h);
        pta2 = ptaCreate(h);
        for (i = 0; i < h; i++) {
            applyQuadraticFit(cl2, cl1, cl0, i, &x);
            ptaAddPt(pta1, x, i);
            applyQuadraticFit(cr2, cr1, cr0, i, &x);
            ptaAddPt(pta2, x, i);
        }
        pixt1 = pixDisplayPta(NULL, dew->pixs, pta1);
        pixDisplayPta(pixt1, pixt1, pta2);
        pixRenderHorizEndPoints(pixt1, ptald, ptard, 0xff000000);
        pixDisplay(pixt1, 600, 800);
        pixWrite("/tmp/dewmod/005a.png", pixt1, IFF_PNG);
        pixDestroy(&pixt1);

        pixt1 = pixDisplayPta(NULL, dew->pixs, pta1);
        pixDisplayPta(pixt1, pixt1, pta2);
        ptalft = ptaTranspose(ptalf);
        ptarft = ptaTranspose(ptarf);
        pixRenderHorizEndPoints(pixt1, ptalft, ptarft, 0x0000ff00);
        pixDisplay(pixt1, 800, 800);
        pixWrite("/tmp/dewmod/005b.png", pixt1, IFF_PNG);
        convertFilesToPdf("/tmp/dewmod", "005", 135, 1.0, 0, 0,
                          "Dewarp Horiz Disparity", "/tmp/dewarp_horiz.pdf");
        fprintf(stderr, "pdf file made: /tmp/dewarp_horiz.pdf\n");
        pixDestroy(&pixt1);
        ptaDestroy(&pta1);
        ptaDestroy(&pta2);
        ptaDestroy(&ptalft);
        ptaDestroy(&ptarft);
    }

        /* Find the min (for ptalf) and max (for ptarf) x values.
         * Use the difference between the extreme values and the
         * actual x coordinates to generate the horizontal disparity
         * at those values of x for the sampled y values. */
    evenpage = (dew->pageno % 2 == 0) ? 1 : 0;
    ptaGetRange(ptalf, &minl, &maxl, NULL, NULL);
    ptaGetRange(ptarf, &minr, &maxr, NULL, NULL);
    refl = (evenpage) ? minl : maxl;
    refr = (evenpage) ? minr : maxr;
    nald = numaCreate(ny);
    nard = numaCreate(ny);
    for (i = 0; i < ny; i++) {
        ptaGetPt(ptalf, i, &x, NULL);
        numaAddNumber(nald, refl - x);
        ptaGetPt(ptarf, i, &x, NULL);
        numaAddNumber(nard, refr - x);
    }

        /* Now generate the horizontal disparity on all sampled points */
    ptaah = ptaaCreate(ny);
    for (i = 0; i < ny; i++) {
        pta = ptaCreate(2);
        numaGetFValue(nald, i, &val);
        ptaAddPt(pta, minl, val);
        numaGetFValue(nard, i, &val);
        ptaAddPt(pta, maxr, val);
        ptaGetLinearLSF(pta, &c1, &c0, NULL);  /* horiz disparity along line */
        ptat = ptaCreate(nx);
        for (j = 0; j < nx; j++) {
            x = j * sampling;
            applyLinearFit(c1, c0, x, &val);
            ptaAddPt(ptat, x, val);
        }
        ptaaAddPta(ptaah, ptat, L_INSERT);
        ptaDestroy(&pta);
    }
    numaDestroy(&nald);
    numaDestroy(&nard);

        /* Save the result in a fpix at the specified subsampling  */
    fpix = fpixCreate(nx, ny);
    for (i = 0; i < ny; i++) {
        for (j = 0; j < nx; j++) {
            ptaaGetPt(ptaah, i, j, NULL, &val);
            fpixSetPixel(fpix, j, i, val);
        }
    }
    dew->samphdispar = fpix;

    ptaDestroy(&ptal);
    ptaDestroy(&ptar);
    ptaDestroy(&ptald);
    ptaDestroy(&ptard);
    ptaDestroy(&ptalf);
    ptaDestroy(&ptarf);
    ptaDestroy(&ptard);
    ptaaDestroy(&ptaah);
    return 0;
}


/*!
 *  dewarpGetTextlineCenters()
 *
 *      Input:  pixs (1 bpp)
 *              debugflag (1 for debug output)
 *      Return: ptaa (of center values of textlines)
 *
 *  Notes:
 *      (1) This in general does not have a point for each value
 *          of x, because there will be gaps between words.
 *          It doesn't matter because we will fit a quadratic to the
 *          points that we do have.
 */
PTAA *
dewarpGetTextlineCenters(PIX     *pixs,
                         l_int32  debugflag)
{
l_int32   i, w, h, bx, by, nsegs;
BOXA     *boxa;
PIX      *pix, *pixt1, *pixt2, *pixt3;
PIXA     *pixa1, *pixa2;
PTA      *pta;
PTAA     *ptaa;

    PROCNAME("dewarpGetTextlineCenters");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PTAA *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);

        /* Filter to solidify the text lines within the x-height region,
         * and to remove most of the ascenders and descenders.
         * We start with a small vertical opening to remove noise beyond
         * the line that can cause an error in the line end points. */
    pixt1 = pixMorphSequence(pixs, "o1.3 + c15.1 + o15.1 + c30.1", 0);
    pixDisplayWithTitle(pixt1, 0, 800, "pix1", debugflag);

        /* Get the 8-connected components ... */
    boxa = pixConnComp(pixt1, &pixa1, 8);
    pixDestroy(&pixt1);
    boxaDestroy(&boxa);
    if (pixaGetCount(pixa1) == 0) {
        pixaDestroy(&pixa1);
        return NULL;
    }

        /* ... and remove the short and thin c.c */
    pixa2 = pixaSelectBySize(pixa1, 100, 4, L_SELECT_IF_BOTH,
                                   L_SELECT_IF_GT, 0);
    if ((nsegs = pixaGetCount(pixa2)) == 0) {
        pixaDestroy(&pixa1);
        pixaDestroy(&pixa2);
        return NULL;
    }
    if (debugflag) {
        pixt2 = pixaDisplay(pixa2, w, h);
        pixDisplayWithTitle(pixt2, 800, 800, "pix2", 1);
        pixDestroy(&pixt2);
    }

        /* For each c.c., get the weighted center of each vertical column.
         * The result is a set of points going approximately through
         * the center of the x-height part of the text line.  */
    ptaa = ptaaCreate(nsegs);
    for (i = 0; i < nsegs; i++) {
        pixaGetBoxGeometry(pixa2, i, &bx, &by, NULL, NULL);
        pix = pixaGetPix(pixa2, i, L_CLONE);
        pta = dewarpGetMeanVerticals(pix, bx, by);
        ptaaAddPta(ptaa, pta, L_INSERT);
        pixDestroy(&pix);
    }
    if (debugflag) {
        pixt3 = pixCreateTemplate(pixt2);
        pix = pixDisplayPtaa(pixt3, ptaa);
        pixDisplayWithTitle(pix, 0, 1400, "pix3", 1);
        pixDestroy(&pix);
        pixDestroy(&pixt3);
    }

    pixaDestroy(&pixa1);
    pixaDestroy(&pixa2);
    return ptaa;
}


/*!
 *  dewarpGetMeanVerticals()
 *
 *      Input:  pixs (1 bpp, single c.c.)
 *              x,y (location of UL corner of pixs with respect to page image
 *      Return: pta (mean y-values in component for each x-value,
 *                   both translated by (x,y)
 */
static PTA *
dewarpGetMeanVerticals(PIX     *pixs,
                       l_int32  x,
                       l_int32  y)
{
l_int32    w, h, i, j, wpl, sum, count;
l_uint32  *line, *data;
PTA       *pta;

    PROCNAME("pixGetMeanVerticals");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PTA *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    pta = ptaCreate(w);
    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    for (j = 0; j < w; j++) {
        line = data;
        sum = count = 0;
        for (i = 0; i < h; i++) {
            if (GET_DATA_BIT(line, j) == 1) {
                sum += i;
                count += 1;
            }
            line += wpl;
        }
        if (count == 0) continue;
        ptaAddPt(pta, x + j, y + (sum / count));
    }

    return pta;
}


/*!
 *  dewarpRemoveShortLines()
 *
 *      Input:  pixs (1 bpp)
 *              ptaas (input lines)
 *              fract (minimum fraction of longest line to keep)
 *              debugflag
 *      Return: ptaad (containing only lines of sufficient length),
 *                     or null on error
 */
PTAA *
dewarpRemoveShortLines(PIX       *pixs,
                       PTAA      *ptaas,
                       l_float32  fract,
                       l_int32    debugflag)
{
l_int32    w, n, i, index, maxlen, len;
l_float32  minx, maxx;
NUMA      *na, *naindex;
PIX       *pixt1, *pixt2;
PTA       *pta;
PTAA      *ptaad;

    PROCNAME("dewarpRemoveShortLines");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PTAA *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (!ptaas)
        return (PTAA *)ERROR_PTR("ptaas undefined", procName, NULL);

    pixGetDimensions(pixs, &w, NULL, NULL);
    n = ptaaGetCount(ptaas);
    ptaad = ptaaCreate(n);
    na = numaCreate(n);
    for (i = 0; i < n; i++) {
        pta = ptaaGetPta(ptaas, i, L_CLONE);
        ptaGetRange(pta, &minx, &maxx, NULL, NULL);
        numaAddNumber(na, maxx - minx + 1);
        ptaDestroy(&pta);
    }

        /* Sort by length and find all that are long enough */
    naindex = numaGetSortIndex(na, L_SORT_DECREASING);
    numaGetIValue(naindex, 0, &index);
    numaGetIValue(na, index, &maxlen);
    if (maxlen < 0.5 * w)
        L_WARNING("lines are relatively short", procName);
    pta = ptaaGetPta(ptaas, index, L_CLONE);
    ptaaAddPta(ptaad, pta, L_INSERT);
    for (i = 1; i < n; i++) {
        numaGetIValue(naindex, i, &index);
        numaGetIValue(na, index, &len);
        if (len < fract * maxlen) break;
        pta = ptaaGetPta(ptaas, index, L_CLONE);
        ptaaAddPta(ptaad, pta, L_INSERT);
    }

    if (debugflag) {
        pixt1 = pixCopy(NULL, pixs);
        pixt2 = pixDisplayPtaa(pixt1, ptaad);
        pixDisplayWithTitle(pixt2, 0, 200, "pix4", 1);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }

    numaDestroy(&na);
    numaDestroy(&naindex);
    return ptaad;
}


/*!
 *  dewarpGetLineEndpoints()
 *
 *      Input:  h (height of pixs)
 *              ptaa (lines)
 *              &ptal (<return> left end points of each line)
 *              &ptar (<return> right end points of each line)
 *      Return: 0 if OK, 1 on error.
 *
 *  Notes:
 *      (1) We require that the set of end points extends over 45% of the
 *          height of the input image, to insure good coverage and
 *          avoid extrapolating too far.  Large extrapolations are
 *          dangerous if used as a reference model.
 *      (2) For fitting the endpoints, x = f(y), we transpose x and y.
 *          Thus all these ptas have x and y swapped!
 */
static l_int32
dewarpGetLineEndpoints(l_int32  h,
                       PTAA    *ptaa,
                       PTA    **pptal,
                       PTA    **pptar)
{
l_int32    i, n, npt, x, y;
l_float32  miny, maxy, ratio;
PTA       *pta, *ptal, *ptar;

    PROCNAME("dewarpGetLineEndpoints");

    if (!pptal || !pptar)
        return ERROR_INT("&ptal and &ptar not both defined", procName, 1);
    *pptal = *pptar = NULL;
    if (!ptaa)
        return ERROR_INT("ptaa undefined", procName, 1);

    n = ptaaGetCount(ptaa);
    ptal = ptaCreate(n);
    ptar = ptaCreate(n);
    for (i = 0; i < n; i++) {
        pta = ptaaGetPta(ptaa, i, L_CLONE);
        ptaGetIPt(pta, 0, &x, &y);
        ptaAddPt(ptal, y, x);
        npt = ptaGetCount(pta);
        ptaGetIPt(pta, npt - 1, &x, &y);
        ptaAddPt(ptar, y, x);
        ptaDestroy(&pta);
    }

        /* Get the min and max of the y value on the left side */
    ptaGetRange(ptal, &miny, &maxy, NULL, NULL);
    ratio = (maxy - miny) / (l_float32)h;
    if (ratio < MIN_RATIO_LINES_TO_HEIGHT) {
        L_INFO_FLOAT("ratio lines to height, %f, too small", procName, ratio);
        ptaDestroy(&ptal);
        ptaDestroy(&ptar);
        return 1;
    }

    *pptal = ptal;
    *pptar = ptar;
    return 0;
}


/*!
 *  dewarpQuadraticLSF()
 *
 *      Input:  ptad (left or right end points of longest lines)
 *              &a  (<return> coeff a of LSF: y = ax^2 + bx + c)
 *              &b  (<return> coeff b of LSF: y = ax^2 + bx + c)
 *              &c  (<return> coeff c of LSF: y = ax^2 + bx + c)
 *              &mederr (<optional return> median error)
 *      Return: 0 if OK, 1 on error.
 *
 *  Notes:
 *      (1) This is used for finding the left or right sides of
 *          the text block, computed as a quadratic curve.
 *          Only the longest lines are input, so there are
 *          no outliers.
 *      (2) The ptas for the end points all have x and y swapped.
 */
static l_int32
dewarpQuadraticLSF(PTA        *ptad,
                   l_float32  *pa,
                   l_float32  *pb,
                   l_float32  *pc,
                   l_float32  *pmederr)
{
l_int32    i, n;
l_float32  x, y, xp, c0, c1, c2;
NUMA      *naerr;

    PROCNAME("dewarpQuadraticLSF");

    if (pmederr) *pmederr = 0.0;
    if (!pa || !pb || !pc)
        return ERROR_INT("not all ptrs are defined", procName, 1);
    *pa = *pb = *pc = 0.0;
    if (!ptad)
        return ERROR_INT("ptad not defined", procName, 1);

        /* Fit to the longest lines */
    ptaGetQuadraticLSF(ptad, &c2, &c1, &c0, NULL);
    *pa = c2;
    *pb = c1;
    *pc = c0;

        /* Optionally, find the median error */
    if (pmederr) {
        n = ptaGetCount(ptad);
        naerr = numaCreate(n);
        for (i = 0; i < n; i++) {
            ptaGetPt(ptad, i, &y, &xp);
            applyQuadraticFit(c2, c1, c0, y, &x);
            numaAddNumber(naerr, L_ABS(x - xp));
        }
        numaGetMedian(naerr, pmederr);
        numaDestroy(&naerr);
    }
    return 0;
}


/*!
 *  dewarpFindLongLines()
 *
 *      Input:  ptal (left end points of lines)
 *              ptar (right end points of lines)
 *              &ptald (<return> left end points of longest lines)
 *              &ptard (<return> right end points of longest lines)
 *      Return: 0 if OK, 1 on error or if there aren't enough long lines
 *
 *  Notes:
 *      (1) We do the following:
 *         (a) Sort the lines from top to bottom, and divide equally
 *             into Top and Bottom sets.
 *         (b) For each set, select the lines that are within 3% of
 *             the longest line in the set.
 *         (c) Accumulate the left and right end points from both
 *             sets into the two returned ptas.
 */
static l_int32
dewarpFindLongLines(PTA   *ptal,
                    PTA   *ptar,
                    PTA  **pptald,
                    PTA  **pptard)
{
l_int32    i, n, ntop, nt, nb;
l_float32  xl, xr, yl, yr, len, maxlen;
NUMA      *nalen, *naindex;
PTA       *ptals, *ptars, *ptald, *ptard;

    PROCNAME("dewarpFindLongLines");

    if (!pptald || !pptard)
        return ERROR_INT("&ptald and &ptard are not both defined", procName, 1);
    *pptald = *pptard = NULL;
    if (!ptal || !ptar)
        return ERROR_INT("ptal and ptar are not both defined", procName, 1);

        /* Sort from top to bottom, remembering that x <--> y in the pta */
    n = ptaGetCount(ptal);
    ptaGetSortIndex(ptal, L_SORT_BY_X, L_SORT_INCREASING, &naindex);
    ptals = ptaSortByIndex(ptal, naindex);
    ptars = ptaSortByIndex(ptar, naindex);
    numaDestroy(&naindex);

    ptald = ptaCreate(n);  /* output of long lines */
    ptard = ptaCreate(n);  /* ditto */

        /* Find all lines in the top half that are within 3% of
         * the length of the longest line in that set. */
    ntop = n / 2;
    nalen = numaCreate(n / 2);  /* lengths of top lines */
    for (i = 0; i < ntop; i++) {
        ptaGetPt(ptals, i, NULL, &xl);
        ptaGetPt(ptars, i, NULL, &xr);
        numaAddNumber(nalen, xr - xl);
    }
    numaGetMax(nalen, &maxlen, NULL);
    L_INFO_FLOAT("Top: maxlen = %8.3f", procName, maxlen);
    for (i = 0; i < ntop; i++) {
        numaGetFValue(nalen, i, &len);
        if (len >= 0.97 * maxlen) {
            ptaGetPt(ptals, i, &yl, &xl);
            ptaAddPt(ptald, yl, xl);
            ptaGetPt(ptars, i, &yr, &xr);
            ptaAddPt(ptard, yr, xr);
        }
    }
    numaDestroy(&nalen);

    nt = ptaGetCount(ptald);
    if (nt < 3) {
        L_INFO("too few long lines at top", procName);
        ptaDestroy(&ptals);
        ptaDestroy(&ptars);
        ptaDestroy(&ptald);
        ptaDestroy(&ptard);
        return 1;
    }

        /* Find all lines in the bottom half that are within 3% of
         * the length of the longest line in that set. */
    nalen = numaCreate(0);  /* lengths of bottom lines */
    for (i = ntop; i < n; i++) {
        ptaGetPt(ptals, i, NULL, &xl);
        ptaGetPt(ptars, i, NULL, &xr);
        numaAddNumber(nalen, xr - xl);
    }
    numaGetMax(nalen, &maxlen, NULL);
    L_INFO_FLOAT("Bottom: maxlen = %8.3f", procName, maxlen);
    for (i = 0; i < n - ntop; i++) {
        numaGetFValue(nalen, i, &len);
        if (len >= 0.97 * maxlen) {
            ptaGetPt(ptals, ntop + i, &yl, &xl);
            ptaAddPt(ptald, yl, xl);
            ptaGetPt(ptars, ntop + i, &yr, &xr);
            ptaAddPt(ptard, yr, xr);
        }
    }
    numaDestroy(&nalen);
    ptaDestroy(&ptals);
    ptaDestroy(&ptars);

    nb = ptaGetCount(ptald) - nt;
    if (nb < 3) {
        L_INFO("too few long lines at bottom", procName);
        ptaDestroy(&ptald);
        ptaDestroy(&ptard);
        return 1;
    }
    else {
        *pptald = ptald;
        *pptard = ptard;
    }
    return 0;
}



/*----------------------------------------------------------------------*
 *                     Apply warping disparity array                    *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpaApplyDisparity()
 *
 *      Input:  dewa
 *              pageno
 *              pixs (image to be modified; can be 1, 8 or 32 bpp)
 *              debugfile (use null to skip writing this)
 *      Return: 0 if OK, 1 on error (no models or ref models available)
 *
 *  Notes:
 *      (1) This applies the disparity arrays to the specified image.
 *      (2) If the models and ref models have not been validated, this
 *          will do so by calling dewarpaInsertRefModels().
 *      (3) This works with both stripped and full resolution page models.
 *          If the full res disparity array(s) are missing, they are remade.
 *      (4) The caller must handle errors that are returned because there
 *          are no valid models or ref models for the page -- typically
 *          by using the input pixs.
 *      (5) If there is no model for @pageno, this will use the model for
 *          'refpage' and put the result in the dew for @pageno.
 *      (6) This populates the full resolution disparity arrays if
 *          necessary.  When applying to a number of images, after
 *          calling this function and saving the resulting pixd,
 *          you should call dewarpMinimize(dew) on the dew for @pageno.
 *          This will remove pixs and pixd (or their clones) stored in dew,
 *          as well as the full resolution disparity arrays.  Together,
 *          these hold approximately 16 bytes for each pixel in pixs.
 */
l_int32
dewarpaApplyDisparity(L_DEWARPA   *dewa,
                      l_int32      pageno,
                      PIX         *pixs,
                      const char  *debugfile)
{
l_int32    debug;
L_DEWARP  *dew1, *dew2;
PIX       *pixv, *pixd;

    PROCNAME("dewarpaApplyDisparity");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);
    if (pageno < 0 || pageno > dewa->maxpage)
        return ERROR_INT("invalid pageno", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

        /* Make sure all models are valid and all refmodels have
         * been added to dewa */
    debug = (debugfile) ? 1 : 0;
    if (dewa->modelsready == 0)
        dewarpaInsertRefModels(dewa, debug);

        /* Initialize the output with the input, so we'll have that
         * in case we don't have a usable page model. */
    if ((dew1 = dewarpaGetDewarp(dewa, pageno)) == NULL)
        return ERROR_INT("dew1 not defined; no valid model", procName, 1);
    pixDestroy(&dew1->pixd);  /* remove any previous result */
    dew1->pixd = pixClone(pixs);

        /* Get the page model that we will use and sanity-check that
         * it is valid.  The ultimate result will be put in dew1->pixd. */
    if (dew1->hasref)  /* point to another page with a model */
        dew2 = dewarpaGetDewarp(dewa, dew1->refpage);
    else
        dew2 = dew1;
    if (dew2->valid == 0)
        return ERROR_INT("no model; shouldn't happen", procName, 1);

        /* OK, we have a usable model; clear out the default result. */
    pixDestroy(&dew1->pixd);

        /* Generate the full res disparity arrays if they don't exist
         * (e.g., if they've been minimized or read from file), or if
         * they are too small for the current image.  */
    dewarpPopulateFullRes(dew2, pixs);

    if ((pixv = pixApplyVertDisparity(dew2, pixs)) == NULL)
        return ERROR_INT("pixv not made", procName, 1);
    if (debugfile) {
        pixDisplayWithTitle(pixv, 300, 0, "pixv", 1);
        lept_rmdir("dewapply");  /* remove previous images */
        lept_mkdir("dewapply");
        pixWrite("/tmp/dewapply/001.png", pixs, IFF_PNG);
        pixWrite("/tmp/dewapply/002.png", pixv, IFF_PNG);
    }

    pixd = pixApplyHorizDisparity(dew2, pixv);
    if (pixd) {
        pixDestroy(&pixv);
        dew1->pixd = pixd;
        if (debugfile) {
            pixDisplayWithTitle(pixd, 600, 0, "pixd", 1);
            pixWrite("/tmp/dewapply/003.png", pixd, IFF_PNG);
        }
    }
    else
        dew1->pixd = pixv;

    if (debugfile) {
        dewarpDebug(dew1, "dewapply", 0);
        convertFilesToPdf("/tmp/dewapply", NULL, 135, 1.0, 0, 0,
                         "Dewarp Apply Disparity", debugfile);
        fprintf(stderr, "pdf file made: %s\n", debugfile);
    }

        /* If this was a reference model, remove the full res arrays */
    if (dew1->hasref)
        dewarpMinimize(dew2);

    return 0;
}


/*!
 *  pixApplyVertDisparity()
 *
 *      Input:  dew
 *              pixs (1, 8 or 32 bpp)
 *      Return: pixd (modified to remove vertical disparity), or null on error
 *
 *  Notes:
 *      (1) This applies the vertical disparity array to the specified
 *          image.  For src pixels above the image, we use the pixels
 *          in the first raster line.
 */
static PIX *
pixApplyVertDisparity(L_DEWARP  *dew,
                      PIX       *pixs)
{
l_int32     i, j, w, h, d, fw, fh, wpld, wplf, isrc, val8;
l_uint32   *datad, *lined;
l_float32  *dataf, *linef;
void      **lineptrs;
FPIX       *fpix;
PIX        *pixd;

    PROCNAME("pixApplyVertDisparity");

    if (!dew)
        return (PIX *)ERROR_PTR("dew not defined", procName, NULL);
    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    if ((fpix = dew->fullvdispar) == NULL)
        return (PIX *)ERROR_PTR("fullvdispar not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pix not 1, 8 or 32 bpp", procName, NULL);
    fpixGetDimensions(fpix, &fw, &fh);
    if (fw < w || fh < h) {
        fprintf(stderr, "fw = %d, w = %d, fh = %d, h = %d\n", fw, w, fh, h);
        return (PIX *)ERROR_PTR("invalid fpix size", procName, NULL);
    }

    pixd = pixCreateTemplate(pixs);
    datad = pixGetData(pixd);
    dataf = fpixGetData(fpix);
    wpld = pixGetWpl(pixd);
    wplf = fpixGetWpl(fpix);
    if (d == 1) {  /* use white pixels if outside pixs */
        lineptrs = pixGetLinePtrs(pixs, NULL);
        for (i = 0; i < h; i++) {
            lined = datad + i * wpld;
            linef = dataf + i * wplf;
            for (j = 0; j < w; j++) {
                isrc = (l_int32)(i - linef[j] + 0.5);
                if (isrc >= 0 && isrc < h) {
                    if (GET_DATA_BIT(lineptrs[isrc], j))
                        SET_DATA_BIT(lined, j);
                }
            }
        }
    }
    else if (d == 8) {
        lineptrs = pixGetLinePtrs(pixs, NULL);
        for (i = 0; i < h; i++) {
            lined = datad + i * wpld;
            linef = dataf + i * wplf;
            for (j = 0; j < w; j++) {
                isrc = (l_int32)(i - linef[j] + 0.5);
                if (isrc < 0) isrc = 0;
                if (isrc > h - 1) isrc = h - 1;
                val8 = GET_DATA_BYTE(lineptrs[isrc], j);
                SET_DATA_BYTE(lined, j, val8);
            }
        }
    }
    else {  /* d == 32 */
        lineptrs = pixGetLinePtrs(pixs, NULL);
        for (i = 0; i < h; i++) {
            lined = datad + i * wpld;
            linef = dataf + i * wplf;
            for (j = 0; j < w; j++) {
                isrc = (l_int32)(i - linef[j] + 0.5);
                if (isrc < 0) isrc = 0;
                if (isrc > h - 1) isrc = h - 1;
                lined[j] = GET_DATA_FOUR_BYTES(lineptrs[isrc], j);
            }
        }
    }

    FREE(lineptrs);
    return pixd;
}


/*!
 *  pixApplyHorizDisparity()
 *
 *      Input:  dew
 *              pixs (1, 8 or 32 bpp)
 *      Return: pixd (modified to remove horizontal disparity if possible),
 *                   or null on error
 *
 *  Notes:
 *      (1) This applies the horizontal disparity array to the specified
 *          image.
 *      (2) The input pixs has already been corrected for vertical disparity.
 *          If the horizontal disparity array doesn't exist, this returns
 *          a clone of @pixs.
 */
static PIX *
pixApplyHorizDisparity(L_DEWARP  *dew,
                       PIX       *pixs)
{
l_int32     i, j, w, h, d, fw, fh, wpls, wpld, wplf, jsrc, val8;
l_uint32   *datas, *lines, *datad, *lined;
l_float32  *dataf, *linef;
FPIX       *fpix;
PIX        *pixd;

    PROCNAME("pixApplyHorizDisparity");

    if (!dew)
        return (PIX *)ERROR_PTR("dew not defined", procName, NULL);
    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    if ((fpix = dew->fullhdispar) == NULL) {
        L_WARNING("fullhdispar not defined; returning input", procName);
        return pixClone(pixs);
    }
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pix not 1, 8 or 32 bpp", procName, NULL);
    fpixGetDimensions(fpix, &fw, &fh);
    if (fw < w || fh < h) {
        fprintf(stderr, "fw = %d, w = %d, fh = %d, h = %d\n", fw, w, fh, h);
        return (PIX *)ERROR_PTR("invalid fpix size", procName, NULL);
    }

        /* Use white pixels if pulling src pixels from outside pixs */
    pixd = pixCreate(w, h, d);
    if (d == 8 || d == 32)
        pixSetBlackOrWhite(pixd, L_SET_WHITE);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    dataf = fpixGetData(fpix);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    wplf = fpixGetWpl(fpix);
    if (d == 1) {
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            linef = dataf + i * wplf;
            for (j = 0; j < w; j++) {
                jsrc = (l_int32)(j - linef[j] + 0.5);
                if (jsrc >= 0 && jsrc < w) {
                    if (GET_DATA_BIT(lines, jsrc))
                        SET_DATA_BIT(lined, j);
                }
            }
        }
    }
    else if (d == 8) {
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            linef = dataf + i * wplf;
            for (j = 0; j < w; j++) {
                jsrc = (l_int32)(j - linef[j] + 0.5);
                if (jsrc >= 0 && jsrc < w) {
                    val8 = GET_DATA_BYTE(lines, jsrc);
                    SET_DATA_BYTE(lined, j, val8);
                }
            }
        }
    }
    else {  /* d == 32 */
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            linef = dataf + i * wplf;
            for (j = 0; j < w; j++) {
                jsrc = (l_int32)(j - linef[j] + 0.5);
                if (jsrc >= 0 && jsrc < w) {
                    lined[j] = lines[jsrc];
                }
            }
        }
    }

    return pixd;
}


/*----------------------------------------------------------------------*
 *          Stripping out data and populating full res disparity        *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpMinimize()
 *
 *      Input:  dew
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This removes all data that is not needed for serialization.
 *          It keeps the subsampled disparity array(s), so the full
 *          resolution arrays can be reconstructed.
 */
l_int32
dewarpMinimize(L_DEWARP  *dew)
{
    PROCNAME("dewarpMinimize");

    if (!dew)
        return ERROR_INT("dew not defined", procName, 1);

    pixDestroy(&dew->pixs);
    pixDestroy(&dew->pixd);
    fpixDestroy(&dew->fullvdispar);
    fpixDestroy(&dew->fullhdispar);
    numaDestroy(&dew->naflats);
    numaDestroy(&dew->nacurves);
    return 0;
}


/*!
 *  dewarpPopulateFullRes()
 *
 *      Input:  dew
 *              pix (<optional>, to give size of actual image)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If the full resolution vertical and horizontal disparity
 *          arrays do not exist, they are built from the subsampled ones.
 *      (2) If pixs is not given, the size of the arrays is determined
 *          by the original image from which the sampled version was
 *          generated.
 *      (3) If pixs is given:
 *          (a) If the arrays do not exist, the size of pixs is used to
 *              determine the size of the full resolution arrays.
 *          (b) If the arrays exist and pixs is too large, the existing
 *              full res arrays are destroyed and new ones are made.
 */
l_int32
dewarpPopulateFullRes(L_DEWARP  *dew,
                      PIX       *pix)
{
l_int32  width, height, fw, fh, deltaw, deltah, redfactor;
FPIX    *fpixt1, *fpixt2;

    PROCNAME("dewarpPopulateFullRes");

    if (!dew)
        return ERROR_INT("dew not defined", procName, 1);
    if (!dew->sampvdispar)
        return ERROR_INT("no sampled vert disparity", procName, 1);

        /* Establish the target size for the full res arrays */
    if (pix)
        pixGetDimensions(pix, &width, &height, NULL);
    else {
        width = dew->w;
        height = dew->h;
    }

        /* Destroy the existing arrays if they are too small */
    if (dew->fullvdispar) {
        fpixGetDimensions(dew->fullvdispar, &fw, &fh);
        if (width > fw || height > fw)
            fpixDestroy(&dew->fullvdispar);
    }
    if (dew->fullhdispar) {
        fpixGetDimensions(dew->fullhdispar, &fw, &fh);
        if (width > fw || height > fw)
            fpixDestroy(&dew->fullhdispar);
    }

        /* Find the required expansion deltas */
    deltaw = width - dew->sampling * (dew->nx - 1) + 2;
    deltah = height - dew->sampling * (dew->ny - 1) + 2;
    redfactor = dew->redfactor;
    deltaw = redfactor * L_MAX(0, deltaw);
    deltah = redfactor * L_MAX(0, deltah);

        /* Generate the arrays if they don't exist, extending to the
         * left and down as required to make them big enough. */
    if (!dew->fullvdispar) {
        fpixt1 = fpixCopy(NULL, dew->sampvdispar);
        if (redfactor == 2)
            fpixAddMultConstant(fpixt1, 0.0, (l_float32)redfactor);
        fpixt2 = fpixScaleByInteger(fpixt1, dew->sampling * redfactor);
        fpixDestroy(&fpixt1);
        if (deltah == 0 && deltaw == 0) {
            dew->fullvdispar = fpixt2;
        }
        else {
            dew->fullvdispar = fpixAddSlopeBorder(fpixt2, 0, deltaw, 0, deltah);
            fpixDestroy(&fpixt2);
        }
    }

    if (!dew->fullhdispar && dew->samphdispar) {
        fpixt1 = fpixCopy(NULL, dew->samphdispar);
        if (redfactor == 2)
            fpixAddMultConstant(fpixt1, 0.0, (l_float32)redfactor);
        fpixt2 = fpixScaleByInteger(fpixt1, dew->sampling * redfactor);
        fpixDestroy(&fpixt1);
        if (deltah == 0 && deltaw == 0) {
            dew->fullhdispar = fpixt2;
        }
        else {
            dew->fullhdispar = fpixAddSlopeBorder(fpixt2, 0, deltaw, 0, deltah);
            fpixDestroy(&fpixt2);
        }
    }

    return 0;
}


/*----------------------------------------------------------------------*
 *                                 Accessors                            *
 *----------------------------------------------------------------------*/
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


/*!
 *  dewarpaGetResult()
 *
 *      Input:  dewa (populated with dewarp structs for pages)
 *              index (into dewa: this is the pageno)
 *      Return: pixd (clone of the pix in dew), or null on error
 */
PIX *
dewarpaGetResult(L_DEWARPA  *dewa,
                 l_int32     index)
{
L_DEWARP  *dew;

    PROCNAME("dewarpaGetResult");

    if (!dewa)
        return (PIX *)ERROR_PTR("dewa not defined", procName, NULL);

    if ((dew = dewarpaGetDewarp(dewa, index)) != NULL)
        return dewarpGetResult(dew);
    else
        return (PIX *)ERROR_PTR("dew not found", procName, NULL);
}


/*!
 *  dewarpGetResult()
 *
 *      Input:  dew (after applying disparity correction)
 *      Return: pixd (clone of the pix in dew), or null on error
 */
PIX *
dewarpGetResult(L_DEWARP  *dew)
{
    PROCNAME("dewarpGetResult");

    if (!dew)
        return (PIX *)ERROR_PTR("dew not defined", procName, NULL);

    if (dew->pixd)
        return pixClone(dew->pixd);
    else
        return (PIX *)ERROR_PTR("pixd not defined", procName, NULL);
}


/*----------------------------------------------------------------------*
 *                         Operations on dewarpa                        *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpaListPages()
 *
 *      Input:  dewa (populated with dewarp structs for pages)
 *      Return: 0 if OK, 1 on error (list of page numbers), or null on error
 *
 *  Notes:
 *      (1) This generates two numas, stored in the dewarpa, that give:
 *          (a) the page number for each dew that has a page model.
 *          (b) the page number for each dew that has either a page
 *              model or a reference model.
 *          It can be called at any time.
 *      (2) It is called by the dewarpa serializer before writing.
 */
l_int32
dewarpaListPages(L_DEWARPA  *dewa)
{
l_int32    i;
L_DEWARP  *dew;
NUMA      *namodels, *napages;

    PROCNAME("dewarpaListPages");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    numaDestroy(&dewa->namodels);
    numaDestroy(&dewa->napages);
    namodels = numaCreate(dewa->maxpage + 1);
    napages = numaCreate(dewa->maxpage + 1);
    dewa->namodels = namodels;
    dewa->napages = napages;
    for (i = 0; i <= dewa->maxpage; i++) {
        if ((dew = dewarpaGetDewarp(dewa, i)) != NULL) {
            numaAddNumber(napages, dew->pageno);
            if (dew->hasref == 0)
                numaAddNumber(namodels, dew->pageno);
        }
    }
    return 0;
}


/*!
 *  dewarpaSetValidModels()
 *
 *      Input:  dewa
 *              debug (1 to output information on invalid page models)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) A valid model must meet the rendering requirements, which
 *          include whether or not a horizontal disparity model exists
 *          and conditions on curvatures for vertical and horizontal
 *          disparity models.
 *      (2) This function is called by dewarpaInsertRefModels(), which
 *          will destroy all invalid dewarps.  It does not need to
 *          be called by the application.  If you want to inspect
 *          an invalid dewarp model, you must do so before calling
 *          dewarpaInsertRefModels().
 */
l_int32
dewarpaSetValidModels(L_DEWARPA  *dewa,
                      l_int32     debug)
{
l_int32    i, n;
L_DEWARP  *dew;

    PROCNAME("dewarpaSetValidModels");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    n = dewa->maxpage + 1;
    for (i = 0; i < n; i++) {
        if ((dew = dewarpaGetDewarp(dewa, i)) == NULL)
            continue;

        if (debug) {
            if (dew->hasref == 1)
                L_INFO_INT("page %d: has only a ref model", procName, i);
            else if (dew->success == 0)
                L_INFO_INT("page %d: no model successfully built", procName, i);
            else if (dewa->fullmodel && !dew->samphdispar)
                L_INFO_INT("page %d: full model requested; no horiz disparity",
                       procName, i);
            else {
                if (dew->medcurv < dewa->min_medcurv)
                    L_INFO_INT2("page %d: curvature %d < min_medcurv",
                                procName, i, dew->medcurv);
                if (dew->medcurv > dewa->max_medcurv)
                    L_INFO_INT2("page %d: curvature %d > max_medcurv",
                                procName, i, dew->medcurv);
                if (dew->samphdispar) {
                    if (L_ABS(dew->leftcurv) > dewa->max_leftcurv)
                        L_INFO_INT2("page %d: left curvature %d > max_leftcurv",
                                    procName, i, dew->leftcurv);
                    if (L_ABS(dew->rightcurv) > dewa->max_rightcurv)
                      L_INFO_INT2("page %d: right curvature %d > max_rightcurv",
                                  procName, i, dew->rightcurv);
                }
            }
        }

        dew->valid = dewarpaTestForValidModel(dewa, dew);
    }

    return 0;
}


/*!
 *  dewarpaInsertRefModels()
 *
 *      Input:  dewa
 *              debug (1 to output information on invalid page models)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This destroys all dewarp models that are invalid, and then
 *          inserts reference models where possible.
 *      (2) For all pages without a model, this clears out any existing
 *          reference dewarps, finds the nearest valid model with the same
 *          parity, and inserts an empty dewarp with the reference page.
 *      (2) If the nearest page is greater than dewa->maxdist, it does
 *          not use it.  As a consequence, there will be no model for
 *          that page.  Note that if dewa->maxdist < 2, no reference
 *          models will be inserted.
 *      (3) Important: this function must be called, even if reference
 *          models will not be used!  It is typically called after
 *          building models on all available pages, and after setting
 *          the rendering parameters.
 *      (4) If the dewa has been serialized, this function is called by
 *          dewarpaRead() when it is read back.  It is also called
 *          any time the rendering parameters are changed.
 */
l_int32
dewarpaInsertRefModels(L_DEWARPA  *dewa,
                       l_int32     debug)
{
l_int32    i, j, n, val, min, distdown, distup;
L_DEWARP  *dew;
NUMA      *na;

    PROCNAME("dewarpaInsertRefModels");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);
    if (dewa->maxdist < 2)
        L_INFO("maxdist < 2; no ref models can be used", procName);

        /* Make an indicator numa for pages with a valid model. */
    n = dewa->maxpage + 1;
    na = numaMakeConstant(0, n);
    dewarpaSetValidModels(dewa, debug);
    for (i = 0; i < n; i++) {
        dew = dewarpaGetDewarp(dewa, i);
        if (dew && dew->valid)
            numaReplaceNumber(na, i, 1);
    }

        /* Remove all existing invalid or ref models, and insert
         * reference dewarps for pages that need to borrow a model */
    for (i = 0; i < n; i++) {
        numaGetIValue(na, i, &val);
        if (val == 1) continue;  /* already has a valid model */
        dewarpDestroy(&dewa->dewarp[i]);  /* it's not valid; remove it */
        if (dewa->maxdist < 2) continue;  /* can't use a ref model */
        distdown = distup = 100000;
        for (j = i - 2; j >= 0; j -= 2) {  /* look back for nearest model */
            numaGetIValue(na, j, &val);
            if (val == 1) {
                distdown = i - j;
                break;
            }
        }
        for (j = i + 2; j < n; j += 2) {  /* look ahead for nearest model */
            numaGetIValue(na, j, &val);
            if (val == 1) {
                distup = j - i;
                break;
            }
        }
        min = L_MIN(distdown, distup);
        if (min > dewa->maxdist) continue;  /* no valid model within range */
        if (distdown <= distup)
            dewarpaInsertDewarp(dewa, dewarpCreateReference(i, i - distdown));
        else
            dewarpaInsertDewarp(dewa, dewarpCreateReference(i, i + distup));
    }

    dewa->modelsready = 1;  /* validated */
    numaDestroy(&na);
    return 0;
}


/*!
 *  dewarpaStripRefModels()
 *
 *      Input:  dewa (populated with dewarp structs for pages)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This examines each dew in a dewarpa, and removes
 *          all that don't have their own page model (i.e., all
 *          that have "references" to nearby pages with valid models).
 *          These references were generated by dewarpaInsertRefModels(dewa).
 *      (2) Note that even if dewa->fullmodel == 1 (i.e., a full model
 *          is required, with both vertical and horizontal disparity arrays),
 *          this function will leave it in.  These "invalid" models will
 *          be removed by dewarpaInsertRefModels() and replaced by
 *          reference page models.
 */
l_int32
dewarpaStripRefModels(L_DEWARPA  *dewa)
{
l_int32    i;
L_DEWARP  *dew;

    PROCNAME("dewarpaStripRefModels");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    for (i = 0; i <= dewa->maxpage; i++) {
        if ((dew = dewarpaGetDewarp(dewa, i)) != NULL) {
            if (dew->hasref)
                dewarpDestroy(&dewa->dewarp[i]);
        }
    }
    dewa->modelsready = 0;

        /* Regenerate the page lists */
    dewarpaListPages(dewa);
    return 0;
}


/*----------------------------------------------------------------------*
 *         Setting parameters to control rendering from the model       *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpaSetCurvatures()
 *
 *      Input:  dewa
 *              min_medcurv
 *              max_medcurv
 *              max_leftcurv
 *              max_rightcurv
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This sets four curvature thresholds:
 *          * the minimum absolute value of the median for the
 *            vertical disparity line curvatures (Use a value of 0
 *            to accept all models.)
 *          * the maximum absolute value of the median for the
 *            vertical disparity line curvatures
 *          * the maximum absolute value of the left edge for the
 *            horizontal disparity
 *          * the maximum absolute value of the right edge for the
 *            horizontal disparity
 *          all in micro-units, for dewarping to take place.
 *          Use -1 for default values.
 *      (2) An image with a median line curvature less than about 0.00001
 *          has fairly straight textlines.  This is 10 micro-units, and
 *          if @min_medcurv == 11, this would prevent dewarping using the
 *          disparity arrays.
 *      (3) A model having median line curvature larger than about 200
 *          micro-units should probably not be used.
 *      (4) A model having left or right curvature larger than about 100
 *          micro-units should probably not be used.
 */
l_int32
dewarpaSetCurvatures(L_DEWARPA  *dewa,
                     l_int32     min_medcurv,
                     l_int32     max_medcurv,
                     l_int32     max_leftcurv,
                     l_int32     max_rightcurv)
{
    PROCNAME("dewarpaSetCurvatures");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    if (min_medcurv == -1)
        dewa->min_medcurv = DEFAULT_MIN_MEDCURV;
    else
        dewa->min_medcurv = L_MAX(0, min_medcurv);

    if (max_medcurv == -1)
        dewa->max_medcurv = DEFAULT_MAX_MEDCURV;
    else
        dewa->max_medcurv = L_ABS(max_medcurv);

    if (max_leftcurv == -1)
        dewa->max_leftcurv = DEFAULT_MAX_LEFTCURV;
    else
        dewa->max_leftcurv = L_ABS(max_leftcurv);

    if (max_rightcurv == -1)
        dewa->max_rightcurv = DEFAULT_MAX_RIGHTCURV;
    else
        dewa->max_rightcurv = L_ABS(max_rightcurv);

    dewa->modelsready = 0;  /* force validation */
    return 0;
}


/*!
 *  dewarpaUseFullModel()
 *
 *      Input:  dewa
 *              fullmodel (0 for false, 1 for true)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This sets the fullmodel field.  If set, the fullmodel
 *          (both vertical and horizontal disparity) is used if available.
 *          Default is false, so a page model with only vertical disparity
 *          will be considered a valid model and will be used.
 */
l_int32
dewarpaUseFullModel(L_DEWARPA  *dewa,
                    l_int32     fullmodel)
{
    PROCNAME("dewarpaUseFullModel");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    dewa->fullmodel = fullmodel;
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
l_int32    w, h, nx, ny, vdispar, hdispar, nlines, medcurv, leftcurv, rightcurv;
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
        if (fscanf(fp, "median curvature = %d\n", &medcurv) != 1)
            return (L_DEWARP *)ERROR_PTR("read fail for medcurv",
                                         procName, NULL);
    }
    if (hdispar) {
        if (fscanf(fp, "left curvature = %d, right curvature = %d\n",
                   &leftcurv, &rightcurv) != 2)
            return (L_DEWARP *)ERROR_PTR("read fail for leftcurv/rightcurv",
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
        dew->success = 1;
    dew->nx = nx;
    dew->ny = ny;
    if (vdispar) {
        dew->medcurv = medcurv;
        dew->success = 1;
        dew->sampvdispar = fpixv;
    }
    if (hdispar) {
        dew->leftcurv = leftcurv;
        dew->rightcurv = rightcurv;
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
        fprintf(fp, "median curvature = %d\n", dew->medcurv);
    if (hdispar) {
        fprintf(fp, "left curvature = %d, right curvature = %d\n",
                dew->leftcurv, dew->rightcurv);
    }
    if (vdispar) fpixWriteStream(fp, dew->sampvdispar);
    if (hdispar) fpixWriteStream(fp, dew->samphdispar);
    fprintf(fp, "\n");

    if (!vdispar)
        L_WARNING("no disparity arrays!", procName);
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
l_int32     sampling, redfactor, minlines, maxdist;
l_int32     min_medcurv, max_medcurv, max_leftcurv, max_rightcurv, fullmodel;
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
    if (fscanf(fp, "min_medcurv = %d, max_medcurv = %d\n",
               &min_medcurv, &max_medcurv) != 2)
        return (L_DEWARPA *)ERROR_PTR("read fail for medcurv", procName, NULL);
    if (fscanf(fp, "max_leftcurv = %d, max_rightcurv = %d\n",
               &max_leftcurv, &max_rightcurv) != 2)
        return (L_DEWARPA *)ERROR_PTR("read fail for l/r curv", procName, NULL);
    if (fscanf(fp, "fullmodel = %d\n", &fullmodel) != 1)
        return (L_DEWARPA *)ERROR_PTR("read fail for 1 fullmodel",
                                      procName, NULL);

    dewa = dewarpaCreate(maxpage + 1, sampling, redfactor, minlines, maxdist);
    dewa->maxpage = maxpage;
    dewa->min_medcurv = min_medcurv;
    dewa->max_medcurv = max_medcurv;
    dewa->max_leftcurv = max_leftcurv;
    dewa->max_rightcurv = max_rightcurv;
    dewa->fullmodel = fullmodel;
    namodels = numaCreate(ndewarp);
    dewa->namodels = namodels;
    for (i = 0; i < ndewarp; i++) {
        if ((dew = dewarpReadStream(fp)) == NULL) {
            L_ERROR_INT("read fail for dew[%d]", procName, i);
            return NULL;
        }
        dewarpaInsertDewarp(dewa, dew);
        numaAddNumber(namodels, dew->pageno);
    }

        /* Validate the models and insert reference models */
    dewarpaInsertRefModels(dewa, 0);

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
    fprintf(fp, "min_medcurv = %d, max_medcurv = %d\n",
            dewa->min_medcurv, dewa->max_medcurv);
    fprintf(fp, "max_leftcurv = %d, max_rightcurv = %d\n",
            dewa->max_leftcurv, dewa->max_rightcurv);
    fprintf(fp, "fullmodel = %d\n", dewa->fullmodel);
    for (i = 0; i < ndewarp; i++) {
        numaGetIValue(dewa->namodels, i, &pageno);
        dewarpWriteStream(fp, dewarpaGetDewarp(dewa, pageno));
    }

    return 0;
}


/*----------------------------------------------------------------------*
 *                      Dewarp debugging output                         *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpaInfo()
 *
 *      Input:  fp
 *              dewa
 *      Return: 0 if OK, 1 on error
 */
l_int32
dewarpaInfo(FILE       *fp,
            L_DEWARPA  *dewa)
{
l_int32    i, n, pageno, nnone, nactual, nvalid, nref;
L_DEWARP  *dew;

    PROCNAME("dewarpaInfo");

    if (!fp)
        return ERROR_INT("dewa not defined", procName, 1);
    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    fprintf(fp, "\nDewarpaInfo: %p\n", dewa);
    fprintf(fp, "nalloc = %d, maxpage = %d\n", dewa->nalloc, dewa->maxpage);
    fprintf(fp, "sampling = %d, redfactor = %d, minlines = %d\n",
            dewa->sampling, dewa->redfactor, dewa->minlines);
    fprintf(fp, "maxdist = %d, fullmodel = %d\n",
            dewa->maxdist, dewa->fullmodel);

    dewarpaModelStats(dewa, &nnone, &nactual, &nvalid, &nref);
    n = numaGetCount(dewa->napages);
    fprintf(stderr, "Total number of pages with a dew = %d\n", n);
    fprintf(stderr, "Number of pages without any models = %d\n", nnone);
    fprintf(stderr, "Number of pages with an actual model = %d\n", nactual);
    fprintf(stderr, "Number of pages with a valid model = %d\n", nvalid);
    fprintf(stderr, "Number of pages with a ref model = %d\n", nref);

    for (i = 0; i < n; i++) {
        numaGetIValue(dewa->napages, i, &pageno);
        if ((dew = dewarpaGetDewarp(dewa, pageno)) == NULL)
            continue;
        fprintf(stderr, "Page: %d\n", dew->pageno);
        fprintf(stderr, "  hasref = %d, refpage = %d\n",
                dew->hasref, dew->refpage);
        fprintf(stderr, "  nlines = %d\n", dew->nlines);
        fprintf(stderr, "  w = %d, h = %d, nx = %d, ny = %d\n",
                dew->w, dew->h, dew->nx, dew->ny);
    }
    return 0;
}


/*!
 *  dewarpaModelStats()
 *
 *      Input:  dewa
 *              &nnone (<optional return> number without any model)
 *              &nactual (<optional return> number with an actual model)
 *              &nvalid (<optional return> number with a valid model)
 *              &nref (<optional return> number with a reference model)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) A page without a model has no dew.  It most likely failed to
 *          generate an actual model, and has not been assigned a ref
 *          model from a neighboring page with a valid model.
 *      (2) An actual model has a computation of at least the vertical
 *          disparity, where the build resulted in 'success'.  With
 *          further processing by dewarpaInsertRefModels(),
 *          it may be found to be invalid, in which case the dew will
 *          be destroyed and replaced by a ref model if possible.
 *      (3) A valid model is an actual model whose parameters satisfy
 *          the constraints given in dewarpaSetValidModels().
 *      (4) A page has a ref model if it failed to generate a valid
 *          model but was assigned a valid model on another page
 *          (within maxdist) by dewarpaInsertRefModel().
 *      (5) This evaluates the validity of each model internally, and
 *          does not use the 'valid' field in each dew.
 */
l_int32
dewarpaModelStats(L_DEWARPA  *dewa,
                  l_int32    *pnnone,
                  l_int32    *pnactual,
                  l_int32    *pnvalid,
                  l_int32    *pnref)
{
l_int32    i, n, pageno, nnone, nactual, nvalid, nref;
L_DEWARP  *dew;

    PROCNAME("dewarpaModelStats");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    dewarpaListPages(dewa);
    n = numaGetCount(dewa->napages);
    nnone = nactual = nvalid = nref = 0;
    for (i = 0; i < n; i++) {
        numaGetIValue(dewa->napages, i, &pageno);
        dew = dewarpaGetDewarp(dewa, pageno);
        if (!dew) {
            nnone++;
            continue;
        }
        if (dew->hasref == 1)
            nref++;
        if (dew->success == 1)
            nactual++;
        if (dewarpaTestForValidModel(dewa, dew))
            nvalid++;
    }

    if (pnnone) *pnnone = nnone;
    if (pnactual) *pnactual = nactual;
    if (pnvalid) *pnvalid = nvalid;
    if (pnref) *pnref = nref;
    return 0;
}


/*!
 *  dewarpaTestForValidModel()
 *
 *      Input:  dewa
 *              dew
 *      Return: valid (1 if true; 0 otherwise)
 */
static l_int32
dewarpaTestForValidModel(L_DEWARPA  *dewa,
                         L_DEWARP   *dew)
{
        /* No actual model was built */
    if (dew->success == 0) return 0;

        /* Was previously found not to have a valid model  */
    if (dew->hasref == 1) return 0;

        /* Only partial model, but full model required */
    if (dewa->fullmodel && !dew->samphdispar) return 0;

        /* Curvatures out of allowed bounds */
    if (dew->medcurv < dewa->min_medcurv ||
        dew->medcurv > dewa->max_medcurv)
        return 0;
    if (dew->samphdispar) {
        if (L_ABS(dew->leftcurv) > dewa->max_leftcurv ||
            L_ABS(dew->rightcurv) > dewa->max_rightcurv)
            return 0;
    }
    return 1;
}


/*!
 *  dewarpaShowArrays()
 *
 *      Input:  dewa
 *              scalefact (on contour images; typ. 0.5)
 *              first (first page model to render)
 *              last (last page model to render; use 0 to go to end)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Generates a pdf of contour plots of the disparity arrays.
 *      (2) This only shows actual models; not ref models
 */
l_int32
dewarpaShowArrays(L_DEWARPA  *dewa,
                  l_float32   scalefact,
                  l_int32     first,
                  l_int32     last)
{
char       buf[256];
char      *pathname;
l_int32    i, svd, shd;
L_BMF     *bmf;
L_DEWARP  *dew;
PIX       *pixv, *pixvs, *pixh, *pixhs, *pixt, *pixd;
PIXA      *pixa;

    PROCNAME("dewarpaShowArrays");

    if (!dewa)
        return ERROR_INT("dew not defined", procName, 1);
    if (first < 0 || first > dewa->maxpage)
        return ERROR_INT("first out of bounds", procName, 1);
    if (last <= 0 || last > dewa->maxpage) last = dewa->maxpage;
    if (last < first)
        return ERROR_INT("last < first", procName, 1);

    lept_rmdir("dewarparrays");
    lept_mkdir("dewarparrays");

    bmf = bmfCreate("./fonts", 8);
    fprintf(stderr, "Generating contour plots\n");
    for (i = first; i <= last; i++) {
        if (i && ((i % 10) == 0))
            fprintf(stderr, " .. %d", i);
        dew = dewarpaGetDewarp(dewa, i);
        if (!dew) continue;
        if (dew->hasref == 1) continue;
        svd = shd = 0;
        if (dew->sampvdispar) svd = 1;
        if (dew->samphdispar) shd = 1;
        if (!svd) {
            L_ERROR_INT("sampvdispar not made for page %d!", procName, i);
            continue;
        }

            /* Generate contour plots at reduced resolution */
        dewarpPopulateFullRes(dew, NULL);
        pixv = fpixRenderContours(dew->fullvdispar, 3.0, 0.3);
        pixvs = pixScaleBySampling(pixv, scalefact, scalefact);
        pixDestroy(&pixv);
        if (shd) {
            pixh = fpixRenderContours(dew->fullhdispar, 3.0, 0.3);
            pixhs = pixScaleBySampling(pixh, scalefact, scalefact);
            pixDestroy(&pixh);
        }
        dewarpMinimize(dew);

            /* Save side-by-side */
        pixa = pixaCreate(2);
        pixaAddPix(pixa, pixvs, L_INSERT);
        if (shd)
            pixaAddPix(pixa, pixhs, L_INSERT);
        pixt = pixaDisplayTiledInRows(pixa, 32, 1500, 1.0, 0, 30, 2);
        snprintf(buf, sizeof(buf), "Page %d", i);
        pixd = pixAddSingleTextblock(pixt, bmf, buf, 0x0000ff00,
                                     L_ADD_BELOW, NULL);
        snprintf(buf, sizeof(buf), "arrays_%04d.png", i);
        pathname = genPathname("/tmp/dewarparrays", buf);
        pixWrite(pathname, pixd, IFF_PNG);
        pixaDestroy(&pixa);
        pixDestroy(&pixt);
        pixDestroy(&pixd);
        FREE(pathname);
    }
    bmfDestroy(&bmf);
    fprintf(stderr, "\n");

    fprintf(stderr, "Generating pdf of contour plots\n");
    convertFilesToPdf("/tmp/dewarparrays", NULL, 90, 1.0, L_FLATE_ENCODE,
                      0, "Disparity arrays", "/tmp/disparity_arrays.pdf");
    return 0;
}


/*!
 *  dewarpDebug()
 *
 *      Input:  dew
 *              subdir (a subdirectory of /tmp; e.g., "dew1")
 *              index (to help label output images; e.g., the page number)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Prints dewarp fields and generates disparity array contour images.
 *          The contour images are written to file:
 *                /tmp/[subdir]/pixv_[index].png
 */
l_int32
dewarpDebug(L_DEWARP    *dew,
            const char  *subdir,
            l_int32      index)
{
char     outdir[256], fname[64];
char    *pathname;
l_int32  svd, shd;
PIX     *pixv, *pixh;

    PROCNAME("dewarpDebug");

    if (!dew)
        return ERROR_INT("dew not defined", procName, 1);
    if (!subdir)
        return ERROR_INT("subdir not defined", procName, 1);

    fprintf(stderr, "pageno = %d, hasref = %d, refpage = %d\n",
            dew->pageno, dew->hasref, dew->refpage);
    fprintf(stderr, "sampling = %d, redfactor = %d, minlines = %d\n",
            dew->sampling, dew->redfactor, dew->minlines);
    svd = shd = 0;
    if (!dew->hasref) {
        if (dew->sampvdispar) svd = 1;
        if (dew->samphdispar) shd = 1;
        fprintf(stderr, "sampv = %d, samph = %d\n", svd, shd);
        fprintf(stderr, "w = %d, h = %d\n", dew->w, dew->h);
        fprintf(stderr, "nx = %d, ny = %d\n", dew->nx, dew->ny);
        fprintf(stderr, "nlines = %d\n", dew->nlines);
        if (svd)
            fprintf(stderr, "median curvature = %d\n", dew->medcurv);
        if (shd)
            fprintf(stderr, "left curvature = %d, right curvature = %d\n",
                    dew->leftcurv, dew->rightcurv);
    }
    if (!svd && !shd) {
        fprintf(stderr, "No disparity arrays\n");
        return 0;
    }

    dewarpPopulateFullRes(dew, NULL);
    lept_mkdir(subdir);
    snprintf(outdir, sizeof(outdir), "/tmp/%s", subdir);
    if (svd) {
        pixv = fpixRenderContours(dew->fullvdispar, 3.0, 0.3);
        snprintf(fname, sizeof(fname), "pixv_%d.png", index);
        pathname = genPathname(outdir, fname);
        pixWrite(pathname, pixv, IFF_PNG);
        pixDestroy(&pixv);
        FREE(pathname);
    }
    if (shd) {
        pixh = fpixRenderContours(dew->fullhdispar, 3.0, 0.3);
        snprintf(fname, sizeof(fname), "pixh_%d.png", index);
        pathname = genPathname(outdir, fname);
        pixWrite(pathname, pixh, IFF_PNG);
        pixDestroy(&pixh);
        FREE(pathname);
    }
    return 0;
}


/*!
 *  dewarpShowResults()
 *
 *      Input:  dewa
 *              sarray (of indexed input images)
 *              boxa (crop boxes for input images; can be null)
 *              firstpage, lastpage
 *              pdfout (filename)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This generates a pdf of image pairs (before, after) for
 *          the designated set of input pages.
 *      (2) If the boxa exists, its elements are aligned with numbers
 *          in the filenames in @sa.  It is used to crop the input images.
 */
l_int32
dewarpShowResults(L_DEWARPA   *dewa,
                  SARRAY      *sa,
                  BOXA        *boxa,
                  l_int32      firstpage,
                  l_int32      lastpage,
                  const char  *pdfout)
{
char       bufstr[256];
l_int32    i, ret, modelpage;
L_BMF     *bmf;
BOX       *box;
L_DEWARP  *dew;
PIX       *pixs, *pixc, *pixd, *pixt1, *pixt2;
PIXA      *pixa;

    PROCNAME("dewarpShowResults");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);
    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!pdfout)
        return ERROR_INT("pdfout not defined", procName, 1);
    if (firstpage > lastpage)
        return ERROR_INT("invalid first/last page numbers", procName, 1);

    fprintf(stderr, "Dewarping and generating s/by/s view\n");
    bmf = bmfCreate("./fonts", 6);
    lept_rmdir("dewarp_pdfout");
    lept_mkdir("dewarp_pdfout");
    for (i = firstpage; i <= lastpage; i++) {
        if (i && (i % 10 == 0)) fprintf(stderr, ".. %d ", i);
        pixs = pixReadIndexed(sa, i);
        if (boxa) {
            box = boxaGetBox(boxa, i, L_CLONE);
            pixc = pixClipRectangle(pixs, box, NULL);
            boxDestroy(&box);
        }
        else
            pixc = pixClone(pixs);
        dew = dewarpaGetDewarp(dewa, i);
        pixd = NULL;
        if (dew) {
            ret = dewarpaApplyDisparity(dewa, dew->pageno, pixc, NULL);
            if (!ret) {
                pixd = dewarpGetResult(dew);
                dewarpMinimize(dew);
            }
        }
        pixa = pixaCreate(2);
        pixaAddPix(pixa, pixc, L_INSERT);
        if (pixd)
            pixaAddPix(pixa, pixd, L_INSERT);
        pixt1 = pixaDisplayTiledAndScaled(pixa, 32, 500, 2, 0, 35, 2);
        if (dew) {
            modelpage = (dew->hasref) ? dew->refpage : dew->pageno;
            snprintf(bufstr, sizeof(bufstr), "Page %d; using %d\n",
                     i, modelpage);
        }
        else
            snprintf(bufstr, sizeof(bufstr), "Page %d; no dewarp\n", i);
        pixt2 = pixAddSingleTextblock(pixt1, bmf, bufstr, 0x0000ff00,
                                      L_ADD_BELOW, 0);
        snprintf(bufstr, sizeof(bufstr), "/tmp/dewarp_pdfout/%05d", i);
        pixWrite(bufstr, pixt2, IFF_JFIF_JPEG);
        pixaDestroy(&pixa);
        pixDestroy(&pixs);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "Generating pdf of result\n");
    convertFilesToPdf("/tmp/dewarp_pdfout", NULL, 100, 1.0, L_JPEG_ENCODE,
                      0, "Dewarp sequence", pdfout);
    bmfDestroy(&bmf);
    return 0;
}


/*----------------------------------------------------------------------*
 *                          Rendering helpers                           *
 *----------------------------------------------------------------------*/
/*!
 *  pixRenderFlats()
 *
 *      Input:  pixs (32 bpp)
 *              naflats (y location of reference lines for vertical disparity)
 *              linew (width of rendered line; typ 2)
 *      Return: 0 if OK, 1 on error
 */
static l_int32
pixRenderFlats(PIX     *pixs,
               NUMA    *naflats,
               l_int32  linew)
{
l_int32   i, n, w, yval, rval, gval, bval;
PIXCMAP  *cmap;

    PROCNAME("pixRenderFlats");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!naflats)
        return ERROR_INT("naflats not defined", procName, 1);

    w = pixGetWidth(pixs);
    n = numaGetCount(naflats);
    cmap = pixcmapCreateRandom(8, 0, 0);
    for (i = 0; i < n; i++) {
        pixcmapGetColor(cmap, i % 256, &rval, &gval, &bval);
        numaGetIValue(naflats, i, &yval);
        pixRenderLineArb(pixs, 0, yval, w, yval, linew, rval, gval, bval);
    }
    pixcmapDestroy(&cmap);
    return 0;
}


/*!
 *  pixRenderHorizEndPoints()
 *
 *      Input:  pixs (32 bpp)
 *              ptal (left side line end points)
 *              ptar (right side line end points)
 *              color (0xrrggbb00)
 *      Return: 0 if OK, 1 on error
 */
static l_int32
pixRenderHorizEndPoints(PIX      *pixs,
                        PTA      *ptal,
                        PTA      *ptar,
                        l_uint32  color)
{
PIX      *pixcirc;
PTA      *ptalt, *ptart, *ptacirc;

    PROCNAME("pixRenderHorizEndPoints");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!ptal || !ptar)
        return ERROR_INT("ptal and ptar not both defined", procName, 1);

    ptacirc = generatePtaFilledCircle(5);
    pixcirc = pixGenerateFromPta(ptacirc, 11, 11);
    ptalt = ptaTranspose(ptal);
    ptart = ptaTranspose(ptar);

    pixDisplayPtaPattern(pixs, pixs, ptalt, pixcirc, 5, 5, color);
    pixDisplayPtaPattern(pixs, pixs, ptart, pixcirc, 5, 5, color);
    ptaDestroy(&ptacirc);
    ptaDestroy(&ptalt);
    ptaDestroy(&ptart);
    pixDestroy(&pixcirc);
    return 0;
}



#if 0
/*----------------------------------------------------------------------*
 *                Static functions not presently in use                 *
 *----------------------------------------------------------------------*/
/*!
 *  fpixSampledDisparity()
 *
 *      Input:  fpixs (full resolution disparity model)
 *              sampling (sampling factor)
 *      Return: fpixd (sampled disparity model), or null on error
 *
 *  Notes:
 *      (1) This converts full to sampled disparity.
 *      (2) The input array is sampled at the right and top edges, and
 *          at every @sampling pixels horizontally and vertically.
 *      (3) The sampled array may not extend to the right and bottom
 *          pixels in fpixs.  This will occur if fpixs was generated
 *          with slope extension because the image on that page was
 *          larger than normal.  This is fine, because in use the
 *          sampled array will be interpolated back to full resolution
 *          and then extended as required.  So the operations of
 *          sampling and interpolation will be idempotent.
 *      (4) There must be at least 3 sampled points horizontally and
 *          vertically.
 */
static FPIX *
fpixSampledDisparity(FPIX    *fpixs,
                     l_int32  sampling)
{
l_int32    w, h, wd, hd, i, j, is, js;
l_float32  val;
FPIX      *fpixd;

    PROCNAME("fpixSampledDisparity");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);
    if (sampling < 1)
        return (FPIX *)ERROR_PTR("sampling < 1", procName, NULL);

    fpixGetDimensions(fpixs, &w, &h);
    wd = 1 + (w + sampling - 2) / sampling;
    hd = 1 + (h + sampling - 2) / sampling;
    if (wd < 3 || hd < 3)
        return (FPIX *)ERROR_PTR("wd < 3 or hd < 3", procName, NULL);
    fpixd = fpixCreate(wd, hd);
    for (i = 0; i < hd; i++) {
        is = sampling * i;
        if (is >= h) continue;
        for (j = 0; j < wd; j++) {
            js = sampling * j;
            if (js >= w) continue;
            fpixGetPixel(fpixs, js, is, &val);
            fpixSetPixel(fpixd, j, i, val);
        }
    }

    return fpixd;
}


/*!
 *  fpixExtraHorizDisparity()
 *
 *      Input:  fpixv (vertical disparity model)
 *              factor (conversion factor for vertical disparity slope;
 *                      use 0 for default)
 *              &xwid (<return> extra width to be added to dewarped pix)
 *      Return: fpixh, or null on error
 *
 *  Notes:
 *      (1) This takes the difference in vertical disparity at top
 *          and bottom of the image, and converts it to an assumed
 *          horizontal disparity.  In use, we add this to the
 *          horizontal disparity determined by the left and right
 *          ends of textlines.
 *      (2) Usage:
 *            l_int32 xwid = [extra width to be added to fpix and image]
 *            FPix *fpix = fpixExtraHorizDisparity(dew->fullvdispar, 0, &xwid);
 *            fpixLinearCombination(dew->fullhdispar, dew->fullhdispar,
 *                                  fpix, 1.0, 1.0);
 */
static FPIX *
fpixExtraHorizDisparity(FPIX      *fpixv,
                        l_float32  factor,
                        l_int32   *pxwid)
{
l_int32     w, h, i, j, fw, wpl, maxloc;
l_float32   val1, val2, vdisp, vdisp0, maxval;
l_float32  *data, *line, *fadiff;
NUMA       *nadiff;
FPIX       *fpixh;

    PROCNAME("fpixExtraHorizDisparity");

    if (!fpixv)
        return (FPIX *)ERROR_PTR("fpixv not defined", procName, NULL);
    if (!pxwid)
        return (FPIX *)ERROR_PTR("&xwid not defined", procName, NULL);
    if (factor == 0.0)
        factor = DEFAULT_SLOPE_FACTOR;

        /* Estimate horizontal disparity from the vertical disparity
         * difference between the top and bottom, normalized to the
         * image height.  Add the maximum value to the width of the
         * output image, so that all src pixels can be mapped
         * into the dest. */
    fpixGetDimensions(fpixv, &w, &h);
    nadiff = numaCreate(w);
    for (j = 0; j < w; j++) {
        fpixGetPixel(fpixv, j, 0, &val1);
        fpixGetPixel(fpixv, j, h - 1, &val2);
        vdisp = factor * (val2 - val1) / (l_float32)h;
        if (j == 0) vdisp0 = vdisp;
        vdisp = vdisp0 - vdisp;
        numaAddNumber(nadiff, vdisp);
    }
    numaGetMax(nadiff, &maxval, &maxloc);
    *pxwid = (l_int32)(maxval + 0.5);

    fw = w + *pxwid;
    fpixh = fpixCreate(fw, h);
    data = fpixGetData(fpixh);
    wpl = fpixGetWpl(fpixh);
    fadiff = numaGetFArray(nadiff, L_NOCOPY);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < fw; j++) {
            if (j < maxloc)   /* this may not work for even pages */
                line[j] = fadiff[j];
            else  /* keep it at the max value the rest of the way across */
                line[j] = maxval;
        }
    }

    numaDestroy(&nadiff);
    return fpixh;
}
#endif
