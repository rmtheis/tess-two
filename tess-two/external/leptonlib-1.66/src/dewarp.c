/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -  This software is distributed in the hope that it will be
 -  useful, but with NO WARRANTY OF ANY KIND.
 -  No author or distributor accepts responsibility to anyone for the
 -  consequences of using this software, or for whether it serves any
 -  particular purpose or works at all, unless he or she says so in
 -  writing.  Everyone is granted permission to copy, modify and
 -  redistribute this source code, for commercial or non-commercial
 -  purposes, with the following restrictions: (1) the origin of this
 -  source code must not be misrepresented; (2) modified versions must
 -  be plainly marked as such; and (3) this notice may not be removed
 -  or altered from any source or modified source distribution.
 *====================================================================*/

/*
 *  dewarp.c
 *
 *      Create/destroy
 *          L_DEWARP      *dewarpCreate()
 *          void           dewarpDestroy()
 *
 *      Build warp model
 *          l_int32        dewarpBuildModel()
 *          PTAA          *pixGetTextlineCenters()
 *          PTA           *pixGetMeanVerticals()
 *          PTAA          *ptaaRemoveShortLines()
 *          FPIX          *fpixBuildHorizontalDisparity()
 *
 *      Apply warping disparity array
 *          l_int32        dewarpApplyDisparity()
 *          l_int32        pixApplyVerticalDisparity()
 *          l_int32        pixApplyHorizontalDisparity()
 *
 *  Basic functioning:
 *
 *     PIX *pixs, *pixb, *pixd;
 *     L_DEWARP *dew;
 *     pixb = "binarize"(pixs);
 *     dew = dewarpCreate(pixb, ...);
 *     dewarpBuildModel(dew, 0);
 *     dewarpApplyDisparity(dew, pixs, 0);
 *     pixd = dew->pixd;
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
 *  In the version of the code that follows, we use text lines
 *  to find V(x,y), and then, optionally, approximate H(x)
 *  from the values V(x,0) and V(x,h), as described above.
 *  The details are all in the code, but here is the basic outline.
 *  We assume that in the plane perpendicular to the optic axis
 *  (alpha = 0), horizontal and vertical lines have been rectified.
 *  (If not, they can be rectified using the methods described below,
 *  applied separately as steps (1,2,3) in the horizontal and
 *  vertical directions.)
 *
 *  (1) Find lines going approximately through the center of the
 *      text in each text line.  Accept only lines that are
 *      close in length to the longest line.
 *  (2) Generate a regular and highly subsampled vertical
 *      disparity field V(x,y).
 *  (3) Interpolate this to generate a full resolution vertical
 *      disparity field.
 *  (4) Optionally generate a full resolution horizontal disparity
 *      field, H(x).
 *  (5) Apply the vertical dewarping, followed optionally by the
 *      horizontal dewarping.
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
 *
 *  Step (4) is trivially done with the approximation described above.
 *  Once V(x,y) and H(x,y) are derived, step (5) is done trivially.
 *  For vertical dewarp, source pixels at the top and bottom image
 *  boundaries are used whenever a request is made for a pixel that
 *  is outside the image.  For horizontal dewarp, the dest image width
 *  is increased to hold all transformed source pixels (remember,
 *  in that step, the image is widened).
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allheaders.h"

#ifndef  NO_CONSOLE_IO
#define  DEBUG_TEXTLINE_CENTERS    0   /* you can set this to 1 for debug */
#define  DEBUG_SHORT_LINES         0   /* you can set this to 1 for debug */
#else
#define  DEBUG_TEXTLINE_CENTERS    0   /* always must be 0 */
#define  DEBUG_SHORT_LINES         0   /* always must be 0 */
#endif  /* !NO_CONSOLE_IO */

    /* Default parameter values */
static const l_int32     L_DEFAULT_SAMPLING = 30;
static const l_float32   DEFAULT_SLOPE_FACTOR = 2000.;



/*----------------------------------------------------------------------*
 *                             Create/destroy                           *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpCreate()
 *
 *     Input: pixs (1 bpp)
 *            sampling (use -1 or 0 for default value; otherwise minimum of 5)
 *            minlines (minimum number of lines to accept; e.g., 10)
 *            applyhoriz (1 to estimate horiz disparity; 0 to skip)
 *     Return: dew (or null on error)
 *
 *  Notes:
 *      (1) The sampling factor is for the disparity array.  The number
 *          used is not critical; anything between 10 and 60 should be fine.
 *      (2) The minimum number of nearly full-length lines required
 *          to generate a vertical disparity array.  Use a small number
 *          if you are willing to accept a questionable array.
 */
L_DEWARP *
dewarpCreate(PIX     *pixs,
             l_int32  sampling,
             l_int32  minlines,
             l_int32  applyhoriz)
{
l_int32    w, h;
L_DEWARP  *dew;

    PROCNAME("dewarpCreate");

    if (!pixs)
        return (L_DEWARP *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (L_DEWARP *)ERROR_PTR("pixs not 1 bpp", procName, NULL);

    if ((dew = (L_DEWARP *)CALLOC(1, sizeof(L_DEWARP))) == NULL)
        return (L_DEWARP *)ERROR_PTR("dew not made", procName, NULL);
    dew->pixs = pixClone(pixs);

    if (sampling <= 0)
        sampling = L_DEFAULT_SAMPLING;
    else if (sampling < 5) {
         L_WARNING("sampling too small; setting to 5", procName);
         sampling = 5;
    }
    dew->sampling = sampling;
    dew->minlines = minlines;
    dew->applyhoriz = applyhoriz;

        /* Get the dimensions of the sampled array.  This will be
         * stored in an fpix, and the full resolution version is
         * guaranteed to be larger than pixs. */
    pixGetDimensions(pixs, &w, &h, NULL);
    dew->nx = (w + 2 * sampling - 2) / sampling;
    dew->ny = (h + 2 * sampling - 2) / sampling;
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
    fpixDestroy(&dew->fullvdispar);
    fpixDestroy(&dew->fullhdispar);
    numaDestroy(&dew->naflats);
    numaDestroy(&dew->nacurves);
    FREE(dew);
    *pdew = NULL;
    return;
}


/*----------------------------------------------------------------------*
 *                            Build warp model                          *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpBuildModel()
 *
 *      Input:  dew
 *              debugflag (1 for debugging output)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is the basic function that builds the vertical
 *          disparity array, which allows determination of the
 *          src pixel in the input image corresponding to each
 *          dest pixel in the dewarped image.
 *      (2) The method is as follows:
 *          * Estimate the centers of all the long textlines and
 *            fit a LS quadratic to each one.  This smooths the curves.
 *          * Sample each curve at a regular interval, find the y-value
 *            of the flat point on each curve, and subtract the sampled
 *            curve value from this value.  This is the vertical
 *            disparity.
 *          * Fit a LS quadratic to each set of vertically aligned
 *            disparity samples.  This smooths the disparity values
 *            in the vertical direction.  Then resample at the same
 *            regular interval,  We now have a regular grid of smoothed
 *            vertical disparity valuels.
 *          * Interpolate this grid to get a full resolution disparity
 *            map.  This can be applied directly to the src image
 *            pixels to dewarp the image in the vertical direction,
 *            making all textlines horizontal.
 */
l_int32
dewarpBuildModel(L_DEWARP  *dew,
                 l_int32    debugflag)
{
char       *tempname;
l_int32     i, j, nlines, nx, ny, sampling;
l_float32   c0, c1, c2, x, y, flaty, val;
l_float32  *faflats;
NUMA       *nax, *nafit, *nacurve, *nacurves, *naflat, *naflats, *naflatsi;
PIX        *pixs, *pixt1, *pixt2;
PTA        *pta, *ptad;
PTAA       *ptaa1, *ptaa2, *ptaa3, *ptaa4, *ptaa5, *ptaa6, *ptaa7;
FPIX       *fpix1, *fpix2, *fpix3;

    PROCNAME("dewarpBuildModel");

    if (!dew)
        return ERROR_INT("dew not defined", procName, 1);

    pixs = dew->pixs;
    if (debugflag) {
        pixDisplayWithTitle(pixs, 0, 0, "pixs", 1);
        pixWriteTempfile("/tmp", "pixs.png", pixs, IFF_PNG, NULL);
    }

        /* Make initial estimate of centers of textlines */
    ptaa1 = pixGetTextlineCenters(pixs, DEBUG_TEXTLINE_CENTERS);
    if (debugflag) {
        pixt1 = pixConvertTo32(pixs);
        pixt2 = pixDisplayPtaa(pixt1, ptaa1);
        pixWriteTempfile("/tmp", "lines1.png", pixt2, IFF_PNG, NULL);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }

        /* Remove all lines that are not near the length
         * of the longest line. */
    ptaa2 = ptaaRemoveShortLines(pixs, ptaa1, 0.8, DEBUG_SHORT_LINES);
    if (debugflag) {
        pixt1 = pixConvertTo32(pixs);
        pixt2 = pixDisplayPtaa(pixt1, ptaa2);
        pixWriteTempfile("/tmp", "lines2.png", pixt2, IFF_PNG, NULL);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }
    nlines = ptaaGetCount(ptaa2);
    if (nlines < dew->minlines)
        return ERROR_INT("insufficient lines to build model", procName, 1);

        /* Do quadratic fit to smooth each line.  A single quadratic
         * over the entire width of the line appears to be sufficient.
         * Quartics tend to overfit to noise.  Each line is thus
         * represented by three coefficients: c2 * x^2 + c1 * x + c0.
         * Using the coefficients, sample each fitted curve uniformly
         * across the full width of the image.  */
    sampling = dew->sampling;
    nx = dew->nx;
    ny = dew->ny;
    ptaa3 = ptaaCreate(nlines);
    nacurve = numaCreate(nlines);  /* stores curvature coeff c2 */
    for (i = 0; i < nlines; i++) {  /* for each line */
        pta = ptaaGetPta(ptaa2, i, L_CLONE);
        ptaGetQuadraticLSF(pta, &c2, &c1, &c0, NULL);
        numaAddNumber(nacurve, c2);
        ptad = ptaCreate(nx);
        for (j = 0; j < nx; j++) {  /* uniformly sampled in x */
             x = j * sampling;
             applyQuadraticFit(c2, c1, c0, x, &y);
             ptaAddPt(ptad, x, y);
        }
        ptaaAddPta(ptaa3, ptad, L_INSERT);
        ptaDestroy(&pta);
    }
    if (debugflag) {
        ptaa4 = ptaaCreate(nlines);
        for (i = 0; i < nlines; i++) {
            pta = ptaaGetPta(ptaa2, i, L_CLONE);
            ptaGetArrays(pta, &nax, NULL);
            ptaGetQuadraticLSF(pta, NULL, NULL, NULL, &nafit);
            ptad = ptaCreateFromNuma(nax, nafit);
            ptaaAddPta(ptaa4, ptad, L_INSERT);
            ptaDestroy(&pta);
            numaDestroy(&nax);
            numaDestroy(&nafit);
        }
        pixt1 = pixConvertTo32(pixs);
        pixt2 = pixDisplayPtaa(pixt1, ptaa4);
        pixWriteTempfile("/tmp", "lines3.png", pixt2, IFF_PNG, NULL);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        ptaaDestroy(&ptaa4);
    }

        /* Find and save the flat points in each curve. */
    naflat = numaCreate(nlines);
    for (i = 0; i < nlines; i++) {
        pta = ptaaGetPta(ptaa3, i, L_CLONE);
        numaGetFValue(nacurve, i, &c2);
        if (c2 <= 0)  /* flat point at bottom; max value of y in curve */
            ptaGetRange(pta, NULL, NULL, NULL, &flaty);
        else  /* flat point at top; min value of y in curve */
            ptaGetRange(pta, NULL, NULL, &flaty, NULL);
        numaAddNumber(naflat, flaty);
        ptaDestroy(&pta);
    }

        /* Sort the lines in ptaa3 by their position */
    naflatsi = numaGetSortIndex(naflat, L_SORT_INCREASING);
    naflats = numaSortByIndex(naflat, naflatsi);
    nacurves = numaSortByIndex(nacurve, naflatsi);
    dew->naflats = naflats;
    dew->nacurves = nacurves;
    ptaa4 = ptaaSortByIndex(ptaa3, naflatsi);
    numaDestroy(&naflat);
    numaDestroy(&nacurve);
    numaDestroy(&naflatsi);
    if (debugflag) {
        tempname = genTempFilename("/tmp", "naflats.na", 0);
        numaWrite(tempname, naflats);
        FREE(tempname);
    }

        /* Convert the sampled points in ptaa3 to a sampled disparity with
         * with respect to the flat point in the curve. */
    ptaa5 = ptaaCreate(nlines);
    for (i = 0; i < nlines; i++) {
        pta = ptaaGetPta(ptaa4, i, L_CLONE);
        numaGetFValue(naflats, i, &flaty);
        ptad = ptaCreate(nx);
        for (j = 0; j < nx; j++) {
            ptaGetPt(pta, j, &x, &y);
            ptaAddPt(ptad, x, flaty - y);
        }
        ptaaAddPta(ptaa5, ptad, L_INSERT);
        ptaDestroy(&pta);
    }
    if (debugflag) {
        tempname = genTempFilename("/tmp", "ptaa5.ptaa", 0);
        ptaaWrite(tempname, ptaa5, 0);
        FREE(tempname);
    }

        /* Generate a ptaa taking vertical 'columns' from ptaa5.
         * We want to fit the vertical disparity on the column to the
         * vertical position of the line, which we call 'y' here and
         * obtain from naflats. */
    ptaa6 = ptaaCreate(nx);
    faflats = numaGetFArray(naflats, L_NOCOPY);
    for (j = 0; j < nx; j++) {
        pta = ptaCreate(nlines);
        for (i = 0; i < nlines; i++) {
            y = faflats[i];
            ptaaGetPt(ptaa5, i, j, NULL, &val);  /* disparity value */
            ptaAddPt(pta, y, val);
        }
        ptaaAddPta(ptaa6, pta, L_INSERT);
    }
    if (debugflag) {
        tempname = genTempFilename("/tmp", "ptaa6.ptaa", 0);
        ptaaWrite(tempname, ptaa6, 0);
        FREE(tempname);
    }

        /* Do quadratic fit vertically on a subset of pixel columns
         * for the vertical displacement, which identifies the
         * src pixel(s) for each dest pixel.  Sample the displacement
         * on a regular grid in the vertical direction.   */
    ptaa7 = ptaaCreate(nx);  /* uniformly sampled across full height of image */
    for (j = 0; j < nx; j++) {  /* for each column */
        pta = ptaaGetPta(ptaa6, j, L_CLONE);
        ptaGetQuadraticLSF(pta, &c2, &c1, &c0, NULL);
        ptad = ptaCreate(ny);
        for (i = 0; i < ny; i++) {  /* uniformly sampled in y */
             y = i * sampling;
             applyQuadraticFit(c2, c1, c0, y, &val);
             ptaAddPt(ptad, y, val);
        }
        ptaaAddPta(ptaa7, ptad, L_INSERT);
        ptaDestroy(&pta);
    }
    if (debugflag) {
        tempname = genTempFilename("/tmp", "ptaa7.ptaa", 0);
        ptaaWrite(tempname, ptaa7, 0);
        FREE(tempname);
    }

        /* Save the result in a fpix at the specified subsampling  */
    fpix1 = fpixCreate(nx, ny);
    for (i = 0; i < ny; i++) {
        for (j = 0; j < nx; j++) {
            ptaaGetPt(ptaa7, j, i, NULL, &val);
            fpixSetPixel(fpix1, j, i, val);
        }
    }
    dew->sampvdispar = fpix1;

        /* Generate a full res fpix for vertical dewarping.  We require that
         * the size of this fpix is at least as big as the input image. */
    fpix2 = fpixScaleByInteger(fpix1, sampling);
    dew->fullvdispar = fpix2;
    if (debugflag) {
        pixt1 = fpixRenderContours(fpix2, -2., 2.0, 0.2);
        pixWriteTempfile("/tmp", "vert-contours.png", pixt1, IFF_PNG, NULL);
        pixDisplay(pixt1, 1000, 0);
        pixDestroy(&pixt1);
    }

        /* Generate a full res fpix for horizontal dewarping.  This
         * works to the extent that the line curvature is due to bending
         * out of the plane normal to the camera, and not wide-angle
         * "fishbowl" distortion.  */
    if (dew->applyhoriz) {
        fpix3 = fpixBuildHorizontalDisparity(fpix2, 0, &dew->extraw);
        dew->fullhdispar = fpix3;
    }
    if (debugflag) {
        pixt1 = fpixRenderContours(fpix3, -2., 2.0, 0.2);
        pixWriteTempfile("/tmp", "horiz-contours.png", pixt1, IFF_PNG, NULL);
        pixDisplay(pixt1, 1000, 0);
        pixDestroy(&pixt1);
    }

    dew->success = 1;

    ptaaDestroy(&ptaa1);
    ptaaDestroy(&ptaa2);
    ptaaDestroy(&ptaa3);
    ptaaDestroy(&ptaa4);
    ptaaDestroy(&ptaa5);
    ptaaDestroy(&ptaa6);
    ptaaDestroy(&ptaa7);
    return 0;
}


/*!
 *  pixGetTextlineCenters()
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
pixGetTextlineCenters(PIX     *pixs,
                      l_int32  debugflag)
{
l_int32   i, w, h, bx, by, nsegs;
BOXA     *boxa;
PIX      *pix, *pixt1, *pixt2, *pixt3;
PIXA     *pixa1, *pixa2;
PTA      *pta;
PTAA     *ptaa;

    PROCNAME("pixGetTextlineCenters");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PTAA *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);

        /* Filter to solidify the text lines within the x-height region,
         * and to remove most of the ascenders and descenders. */
    pixt1 = pixMorphSequence(pixs, "c15.1 + o15.1 + c30.1", 0);
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
        pta = pixGetMeanVerticals(pix, bx, by);
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
 *  ptaGetMeanVerticals()
 *
 *      Input:  pixs (1 bpp, single c.c.)
 *              x,y (location of UL corner of pixs with respect to page image
 *      Return: pta (mean y-values in component for each x-value,
 *                   both translated by (x,y)
 */
PTA *
pixGetMeanVerticals(PIX     *pixs,
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
 *  ptaaRemoveShortLines()
 *
 *      Input:  pixs (1 bpp)
 *              ptaas (input lines)
 *              fract (minimum fraction of longest line to keep)
 *              debugflag
 *      Return: ptaad (containing only lines of sufficient length),
 *                     or null on error
 */
PTAA *
ptaaRemoveShortLines(PIX       *pixs,
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

    PROCNAME("ptaaRemoveShortLines");

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
 *  fpixBuildHorizontalDisparity()
 *
 *      Input:  fpixv (vertical disparity model)
 *              factor (conversion factor for vertical disparity slope;
 *                      use 0 for default)
 *              &extraw (<return> extra width to be added to dewarped pix)
 *      Return: fpixh, or null on error
 *
 *  Notes:
 *      (1) This takes the difference in vertical disparity at top
 *          and bottom of the image, and converts it to an assumed
 *          horizontal disparity.
 */
FPIX *
fpixBuildHorizontalDisparity(FPIX      *fpixv,
                             l_float32  factor,
                             l_int32   *pextraw)
{
l_int32     w, h, i, j, fw, wpl, maxloc;
l_float32   val1, val2, vdisp, vdisp0, maxval;
l_float32  *data, *line, *fadiff;
NUMA       *nadiff;
FPIX       *fpixh;

    PROCNAME("fpixBuildHorizontalDisparity");

    if (!fpixv)
        return (FPIX *)ERROR_PTR("fpixv not defined", procName, NULL);
    if (!pextraw)
        return (FPIX *)ERROR_PTR("&extraw not defined", procName, NULL);
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
    *pextraw = (l_int32)(maxval + 0.5);

    fw = w + *pextraw;
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



/*----------------------------------------------------------------------*
 *                     Apply warping disparity array                    *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpApplyDisparity()
 *
 *      Input:  dew
 *              pixs (image to be modified; can be 1, 8 or 32 bpp)
 *              debugflag
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This applies the vertical disparity array to the specified
 *          image.  For src pixels above the image, we use the pixels
 *          in the first raster line.
 */
l_int32
dewarpApplyDisparity(L_DEWARP  *dew,
                     PIX       *pixs,
                     l_int32    debugflag)
{
PIX  *pixv, *pixd;

    PROCNAME("dewarpApplyDisparity");

    if (!dew)
        return ERROR_INT("dew not defined", procName, 1);
    if (dew->success == 0)
        return ERROR_INT("model failed to build", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

    if ((pixv = pixApplyVerticalDisparity(pixs, dew->fullvdispar)) == NULL)
        return ERROR_INT("pixv not made", procName, 1);
    if (debugflag) {
        pixDisplayWithTitle(pixv, 300, 0, "pixv", 1);
        pixWriteTempfile("/tmp", "pixv.png", pixv, IFF_PNG, NULL);
    }

    if (dew->applyhoriz) {
        if ((pixd = pixApplyHorizontalDisparity(pixv, dew->fullhdispar,
                                                dew->extraw)) == NULL)
            return ERROR_INT("pixd not made", procName, 1);
        pixDestroy(&pixv);
        dew->pixd = pixd;
        if (debugflag) {
            pixDisplayWithTitle(pixd, 600, 0, "pixd", 1);
            pixWriteTempfile("/tmp", "pixd.png", pixd, IFF_PNG, NULL);
        }
    }
    else
        dew->pixd = pixv;
    return 0;
}


/*!
 *  pixApplyVerticalDisparity()
 *
 *      Input:  pixs (1, 8 or 32 bpp)
 *              fpix (vertical disparity array)
 *      Return: pixd (modified by fpix), or null on error
 *
 *  Notes:
 *      (1) This applies the vertical disparity array to the specified
 *          image.  For src pixels above the image, we use the pixels
 *          in the first raster line.
 */
PIX *
pixApplyVerticalDisparity(PIX   *pixs,
                          FPIX  *fpix)
{
l_int32     i, j, w, h, d, fw, fh, wpld, wplf, isrc, val8;
l_uint32   *datad, *lined;
l_float32  *dataf, *linef;
void      **lineptrs;
PIX        *pixd;

    PROCNAME("pixApplyVerticalDisparity");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!fpix)
        return (PIX *)ERROR_PTR("fpix not defined", procName, NULL);
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
    if (d == 1) {
        lineptrs = pixGetLinePtrs(pixs, NULL);
        for (i = 0; i < h; i++) {
            lined = datad + i * wpld;
            linef = dataf + i * wplf;
            for (j = 0; j < w; j++) {
                isrc = (l_int32)(i - linef[j] + 0.5);
                if (isrc < 0) isrc = 0;
                if (isrc > h - 1) isrc = h - 1;
                if (GET_DATA_BIT(lineptrs[isrc], j))
                    SET_DATA_BIT(lined, j);
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
 *  pixApplyHorizontalDisparity()
 *
 *      Input:  pixs (1, 8 or 32 bpp)
 *              fpix (horizontal disparity array)
 *              extraw (extra width added to pixd)
 *      Return: pixd (modified by fpix), or null on error
 *
 *  Notes:
 *      (1) This applies the horizontal disparity array to the specified
 *          image.
 */
PIX *
pixApplyHorizontalDisparity(PIX     *pixs,
                            FPIX    *fpix,
                            l_int32  extraw)
{
l_int32     i, j, w, h, d, wd, fw, fh, wpls, wpld, wplf, jsrc, val8;
l_uint32   *datas, *lines, *datad, *lined;
l_float32  *dataf, *linef;
PIX        *pixd;

    PROCNAME("pixApplyHorizontalDisparity");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!fpix)
        return (PIX *)ERROR_PTR("fpix not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pix not 1, 8 or 32 bpp", procName, NULL);
    fpixGetDimensions(fpix, &fw, &fh);
    if (fw < w + extraw || fh < h) {
        fprintf(stderr, "fw = %d, w = %d, fh = %d, h = %d\n", fw, w, fh, h);
        return (PIX *)ERROR_PTR("invalid fpix size", procName, NULL);
    }

    wd = w + extraw;
    pixd = pixCreate(wd, h, d);
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
            for (j = 0; j < wd; j++) {
                jsrc = (l_int32)(j - linef[j] + 0.5);
                if (jsrc < 0) jsrc = 0;
                if (jsrc > w - 1) jsrc = w - 1;
                if (GET_DATA_BIT(lines, jsrc))
                    SET_DATA_BIT(lined, j);
            }
        }
    }
    else if (d == 8) {
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            linef = dataf + i * wplf;
            for (j = 0; j < wd; j++) {
                jsrc = (l_int32)(j - linef[j] + 0.5);
                if (jsrc < 0) jsrc = 0;
                if (jsrc > w - 1) jsrc = w - 1;
                val8 = GET_DATA_BYTE(lines, jsrc);
                SET_DATA_BYTE(lined, j, val8);
            }
        }
    }
    else {  /* d == 32 */
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            linef = dataf + i * wplf;
            for (j = 0; j < wd; j++) {
                jsrc = (l_int32)(j - linef[j] + 0.5);
                if (jsrc < 0) jsrc = 0;
                if (jsrc > w - 1) jsrc = w - 1;
                lined[j] = lines[jsrc];
            }
        }
    }

    return pixd;
}


