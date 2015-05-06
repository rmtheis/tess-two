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
 *  baseline.c
 *
 *      Locate text baselines in an image
 *           NUMA     *pixFindBaselines()
 *
 *      Projective transform to remove local skew
 *           PIX      *pixDeskewLocal()
 *
 *      Determine local skew
 *           l_int32   pixGetLocalSkewTransform()
 *           NUMA     *pixGetLocalSkewAngles()
 *
 *  We have two apparently different functions here:
 *    - finding baselines
 *    - finding a projective transform to remove keystone warping
 *  The function pixGetLocalSkewAngles() returns an array of angles,
 *  one for each raster line, and the baselines of the text lines
 *  should intersect the left edge of the image with that angle.
 */

#include <math.h>
#include "allheaders.h"

#ifndef  NO_CONSOLE_IO
#define  DEBUG_PLOT          0
#endif  /* NO_CONSOLE_IO */

    /* Min to travel after finding max before abandoning peak */
static const l_int32  MIN_DIST_IN_PEAK = 35;

    /* Thresholds for peaks and zeros, relative to the max peak */
static const l_int32  PEAK_THRESHOLD_RATIO = 20;
static const l_int32  ZERO_THRESHOLD_RATIO = 100;

    /* Default values for determining local skew */
static const l_int32  DEFAULT_SLICES = 10;
static const l_int32  DEFAULT_SWEEP_REDUCTION = 2;
static const l_int32  DEFAULT_BS_REDUCTION = 1;
static const l_float32  DEFAULT_SWEEP_RANGE = 5.;   /* degrees */
static const l_float32  DEFAULT_SWEEP_DELTA = 1.;   /* degrees */
static const l_float32  DEFAULT_MINBS_DELTA = 0.01;   /* degrees */

    /* Overlap slice fraction added to top and bottom of each slice */
static const l_float32  OVERLAP_FRACTION = 0.5;

    /* Minimum allowed confidence (ratio) for accepting a value */
static const l_float32  MIN_ALLOWED_CONFIDENCE = 3.0;


/*---------------------------------------------------------------------*
 *                    Locate text baselines in an image                *
 *---------------------------------------------------------------------*/
/*!
 *  pixFindBaselines()
 *
 *      Input:  pixs (1 bpp)
 *              &pta (<optional return> pairs of pts corresponding to
 *                    approx. ends of each text line)
 *              debug (usually 0; set to 1 for debugging output)
 *      Return: na (of baseline y values), or null on error
 *
 *  Notes:
 *      (1) Input binary image must have text lines already aligned
 *          horizontally.  This can be done by either rotating the
 *          image with pixDeskew(), or, if a projective transform
 *          is required, by doing pixDeskewLocal() first.
 *      (2) Input null for &pta if you don't want this returned.
 *          The pta will come in pairs of points (left and right end
 *          of each baseline).
 *      (3) Caution: this will not work properly on text with multiple
 *          columns, where the lines are not aligned between columns.
 *          If there are multiple columns, they should be extracted
 *          separately before finding the baselines.
 *      (4) This function constructs different types of output
 *          for baselines; namely, a set of raster line values and
 *          a set of end points of each baseline.
 *      (5) This function was designed to handle short and long text lines
 *          without using dangerous thresholds on the peak heights.  It does
 *          this by combining the differential signal with a morphological
 *          analysis of the locations of the text lines.  One can also
 *          combine this data to normalize the peak heights, by weighting
 *          the differential signal in the region of each baseline
 *          by the inverse of the width of the text line found there.
 *      (6) There are various debug sections that can be turned on
 *          with the debug flag.
 */
NUMA *
pixFindBaselines(PIX     *pixs,
                 PTA    **ppta,
                 l_int32  debug)
{
l_int32    h, i, j, nbox, val1, val2, ndiff, bx, by, bw, bh;
l_int32    imaxloc, peakthresh, zerothresh, inpeak;
l_int32    mintosearch, max, maxloc, nloc, locval;
l_int32   *array;
l_float32  maxval;
BOXA      *boxa1, *boxa2, *boxa3;
GPLOT     *gplot;
NUMA      *nasum, *nadiff, *naloc, *naval;
PIX       *pixt1, *pixt2;
PTA       *pta;

    PROCNAME("pixFindBaselines");

    if (!pixs)
        return (NUMA *)ERROR_PTR("pixs not defined", procName, NULL);
    pta = NULL;
    if (ppta) {
        pta = ptaCreate(0);
        *ppta = pta;
    }

        /* Close up the text characters, removing noise */
    pixt1 = pixMorphSequence(pixs, "c25.1 + e3.1", 0);

        /* Save the difference of adjacent row sums.
         * The high positive-going peaks are the baselines */
    if ((nasum = pixCountPixelsByRow(pixt1, NULL)) == NULL)
        return (NUMA *)ERROR_PTR("nasum not made", procName, NULL);
    h = pixGetHeight(pixs);
    nadiff = numaCreate(h);
    numaGetIValue(nasum, 0, &val2);
    for (i = 0; i < h - 1; i++) {
        val1 = val2;
        numaGetIValue(nasum, i + 1, &val2);
        numaAddNumber(nadiff, val1 - val2);
    }

    if (debug)  /* show the difference signal */
        gplotSimple1(nadiff, GPLOT_X11, "junkdiff", "difference");

        /* Use the zeroes of the profile to locate each baseline. */
    array = numaGetIArray(nadiff);
    ndiff = numaGetCount(nadiff);
    numaGetMax(nadiff, &maxval, &imaxloc);
        /* Use this to begin locating a new peak: */
    peakthresh = (l_int32)maxval / PEAK_THRESHOLD_RATIO;
        /* Use this to begin a region between peaks: */
    zerothresh = (l_int32)maxval / ZERO_THRESHOLD_RATIO;
    naloc = numaCreate(0);
    naval = numaCreate(0);
    inpeak = FALSE;
    for (i = 0; i < ndiff; i++) {
        if (inpeak == FALSE) {
            if (array[i] > peakthresh) {  /* transition to in-peak */
                inpeak = TRUE;
                mintosearch = i + MIN_DIST_IN_PEAK; /* accept no zeros
                                               * between i and mintosearch */
                max = array[i];
                maxloc = i;
            }
        } else {  /* inpeak == TRUE; look for max */
            if (array[i] > max) {
                max = array[i];
                maxloc = i;
                mintosearch = i + MIN_DIST_IN_PEAK;
            } else if (i > mintosearch && array[i] <= zerothresh) {  /* leave */
                inpeak = FALSE;
                numaAddNumber(naval, max);
                numaAddNumber(naloc, maxloc);
            }
        }
    }

        /* If array[ndiff-1] is max, eg. no descenders, baseline at bottom */
    if (inpeak) {
        numaAddNumber(naval, max);
        numaAddNumber(naloc, maxloc);
    }
    FREE(array);

    if (debug) {  /* show the raster locations for the peaks */
        gplot = gplotCreate("junkloc", GPLOT_X11, "Peak locations",
                            "rasterline", "height");
        gplotAddPlot(gplot, naloc, naval, GPLOT_POINTS, "locs");
        gplotMakeOutput(gplot);
        gplotDestroy(&gplot);
    }

        /* Generate an approximate profile of text line width.
         * First, filter the boxes of text, where there may be
         * more than one box for a given textline. */
    pixt2 = pixMorphSequence(pixt1, "r11 + c25.1 + o7.1 +c1.3", 0);
    boxa1 = pixConnComp(pixt2, NULL, 4);
    boxa2 = boxaTransform(boxa1, 0, 0, 4., 4.);
    boxa3 = boxaSort(boxa2, L_SORT_BY_Y, L_SORT_INCREASING, NULL);

        /* Then find the baseline segments */
    if (pta) {
      nloc = numaGetCount(naloc);
      nbox = boxaGetCount(boxa3);
      for (i = 0; i < nbox; i++) {
          boxaGetBoxGeometry(boxa3, i, &bx, &by, &bw, &bh);
          for (j = 0; j < nloc; j++) {
              numaGetIValue(naloc, j, &locval);
              if (L_ABS(locval - (by + bh)) > 25)
                  continue;
              ptaAddPt(pta, bx, locval);
              ptaAddPt(pta, bx + bw, locval);
              break;
          }
      }
    }

    if (debug) {  /* display baselines */
        PIX     *pixd;
        l_int32  npts, x1, y1, x2, y2;
        if (pta) {
            pixd = pixConvertTo32(pixs);
            npts = ptaGetCount(pta);
            for (i = 0; i < npts; i += 2) {
                ptaGetIPt(pta, i, &x1, &y1);
                ptaGetIPt(pta, i + 1, &x2, &y2);
                pixRenderLineArb(pixd, x1, y1, x2, y2, 1, 255, 0, 0);
            }
            pixDisplay(pixd, 200, 200);
            pixWrite("junkbaselines", pixd, IFF_PNG);
            pixDestroy(&pixd);
        }
    }

    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
    boxaDestroy(&boxa3);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    numaDestroy(&nasum);
    numaDestroy(&nadiff);
    numaDestroy(&naval);
    return naloc;
}


/*---------------------------------------------------------------------*
 *               Projective transform to remove local skew             *
 *---------------------------------------------------------------------*/
/*!
 *  pixDeskewLocal()
 *
 *      Input:  pixs
 *              nslices  (the number of horizontal overlapping slices; must
 *                  be larger than 1 and not exceed 20; use 0 for default)
 *              redsweep (sweep reduction factor: 1, 2, 4 or 8;
 *                        use 0 for default value)
 *              redsearch (search reduction factor: 1, 2, 4 or 8, and
 *                         not larger than redsweep; use 0 for default value)
 *              sweeprange (half the full range, assumed about 0; in degrees;
 *                          use 0.0 for default value)
 *              sweepdelta (angle increment of sweep; in degrees;
 *                          use 0.0 for default value)
 *              minbsdelta (min binary search increment angle; in degrees;
 *                          use 0.0 for default value)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This function allows deskew of a page whose skew changes
 *          approximately linearly with vertical position.  It uses
 *          a projective tranform that in effect does a differential
 *          shear about the LHS of the page, and makes all text lines
 *          horizontal.
 *      (2) The origin of the keystoning can be either a cheap document
 *          feeder that rotates the page as it is passed through, or a
 *          camera image taken from either the left or right side
 *          of the vertical.
 *      (3) The image transformation is a projective warping,
 *          not a rotation.  Apart from this function, the text lines
 *          must be properly aligned vertically with respect to each
 *          other.  This can be done by pre-processing the page; e.g.,
 *          by rotating or horizontally shearing it.
 *          Typically, this can be achieved by vertically aligning
 *          the page edge.
 */
PIX *
pixDeskewLocal(PIX       *pixs,
               l_int32    nslices,
               l_int32    redsweep,
               l_int32    redsearch,
               l_float32  sweeprange,
               l_float32  sweepdelta,
               l_float32  minbsdelta)
{
l_int32    ret;
PIX       *pixd;
PTA       *ptas, *ptad;

    PROCNAME("pixDeskewLocal");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

        /* Skew array gives skew angle (deg) as fctn of raster line
         * where it intersects the LHS of the image */
    ret = pixGetLocalSkewTransform(pixs, nslices, redsweep, redsearch,
                                   sweeprange, sweepdelta, minbsdelta,
                                   &ptas, &ptad);
    if (ret != 0)
        return (PIX *)ERROR_PTR("transform pts not found", procName, NULL);

        /* Use a projective transform */
    pixd = pixProjectiveSampledPta(pixs, ptad, ptas, L_BRING_IN_WHITE);

    ptaDestroy(&ptas);
    ptaDestroy(&ptad);
    return pixd;
}


/*---------------------------------------------------------------------*
 *                       Determine the local skew                      *
 *---------------------------------------------------------------------*/
/*!
 *  pixGetLocalSkewTransform()
 *
 *      Input:  pixs
 *              nslices  (the number of horizontal overlapping slices; must
 *                  be larger than 1 and not exceed 20; use 0 for default)
 *              redsweep (sweep reduction factor: 1, 2, 4 or 8;
 *                        use 0 for default value)
 *              redsearch (search reduction factor: 1, 2, 4 or 8, and
 *                         not larger than redsweep; use 0 for default value)
 *              sweeprange (half the full range, assumed about 0; in degrees;
 *                          use 0.0 for default value)
 *              sweepdelta (angle increment of sweep; in degrees;
 *                          use 0.0 for default value)
 *              minbsdelta (min binary search increment angle; in degrees;
 *                          use 0.0 for default value)
 *              &ptas  (<return> 4 points in the source)
 *              &ptad  (<return> the corresponding 4 pts in the dest)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This generates two pairs of points in the src, each pair
 *          corresponding to a pair of points that would lie along
 *          the same raster line in a transformed (dewarped) image.
 *      (2) The sets of 4 src and 4 dest points returned by this function
 *          can then be used, in a projective or bilinear transform,
 *          to remove keystoning in the src.
 */
l_int32
pixGetLocalSkewTransform(PIX       *pixs,
                         l_int32    nslices,
                         l_int32    redsweep,
                         l_int32    redsearch,
                         l_float32  sweeprange,
                         l_float32  sweepdelta,
                         l_float32  minbsdelta,
                         PTA      **pptas,
                         PTA      **pptad)
{
l_int32    w, h, i;
l_float32  deg2rad, angr, angd, dely;
NUMA      *naskew;
PTA       *ptas, *ptad;

    PROCNAME("pixGetLocalSkewTransform");

    if (!pptas || !pptad)
        return ERROR_INT("&ptas and &ptad not defined", procName, 1);
    *pptas = *pptad = NULL;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (nslices < 2 || nslices > 20)
        nslices = DEFAULT_SLICES;
    if (redsweep < 1 || redsweep > 8)
        redsweep = DEFAULT_SWEEP_REDUCTION;
    if (redsearch < 1 || redsearch > redsweep)
        redsearch = DEFAULT_BS_REDUCTION;
    if (sweeprange == 0.0)
        sweeprange = DEFAULT_SWEEP_RANGE;
    if (sweepdelta == 0.0)
        sweepdelta = DEFAULT_SWEEP_DELTA;
    if (minbsdelta == 0.0)
        minbsdelta = DEFAULT_MINBS_DELTA;

    naskew = pixGetLocalSkewAngles(pixs, nslices, redsweep, redsearch,
                                   sweeprange, sweepdelta, minbsdelta,
                                   NULL, NULL);
    if (!naskew)
        return ERROR_INT("naskew not made", procName, 1);

    deg2rad = 3.14159265 / 180.;
    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);
    ptas = ptaCreate(4);
    ptad = ptaCreate(4);
    *pptas = ptas;
    *pptad = ptad;

        /* Find i for skew line that intersects LHS at i and RHS at h / 20 */
    for (i = 0; i < h; i++) {
        numaGetFValue(naskew, i, &angd);
        angr = angd * deg2rad;
        dely = w * tan(angr);
        if (i - dely > 0.05 * h)
            break;
    }
    ptaAddPt(ptas, 0, i);
    ptaAddPt(ptas, w - 1, i - dely);
    ptaAddPt(ptad, 0, i);
    ptaAddPt(ptad, w - 1, i);

        /* Find i for skew line that intersects LHS at i and RHS at 19h / 20 */
    for (i = h - 1; i > 0; i--) {
        numaGetFValue(naskew, i, &angd);
        angr = angd * deg2rad;
        dely = w * tan(angr);
        if (i - dely < 0.95 * h)
            break;
    }
    ptaAddPt(ptas, 0, i);
    ptaAddPt(ptas, w - 1, i - dely);
    ptaAddPt(ptad, 0, i);
    ptaAddPt(ptad, w - 1, i);

    numaDestroy(&naskew);
    return 0;
}


/*!
 *  pixGetLocalSkewAngles()
 *
 *      Input:  pixs
 *              nslices  (the number of horizontal overlapping slices; must
 *                  be larger than 1 and not exceed 20; use 0 for default)
 *              redsweep (sweep reduction factor: 1, 2, 4 or 8;
 *                        use 0 for default value)
 *              redsearch (search reduction factor: 1, 2, 4 or 8, and
 *                         not larger than redsweep; use 0 for default value)
 *              sweeprange (half the full range, assumed about 0; in degrees;
 *                          use 0.0 for default value)
 *              sweepdelta (angle increment of sweep; in degrees;
 *                          use 0.0 for default value)
 *              minbsdelta (min binary search increment angle; in degrees;
 *                          use 0.0 for default value)
 *              &a (<optional return> slope of skew as fctn of y)
 *              &b (<optional return> intercept at y=0 of skew as fctn of y)
 *      Return: naskew, or null on error
 *
 *  Notes:
 *      (1) The local skew is measured in a set of overlapping strips.
 *          We then do a least square linear fit parameters to get
 *          the slope and intercept parameters a and b in
 *              skew-angle = a * y + b  (degrees)
 *          for the local skew as a function of raster line y.
 *          This is then used to make naskew, which can be interpreted
 *          as the computed skew angle (in degrees) at the left edge
 *          of each raster line.
 *      (2) naskew can then be used to find the baselines of text, because
 *          each text line has a baseline that should intersect
 *          the left edge of the image with the angle given by this
 *          array, evaluated at the raster line of intersection.
 */
NUMA *
pixGetLocalSkewAngles(PIX        *pixs,
                      l_int32     nslices,
                      l_int32     redsweep,
                      l_int32     redsearch,
                      l_float32   sweeprange,
                      l_float32   sweepdelta,
                      l_float32   minbsdelta,
                      l_float32  *pa,
                      l_float32  *pb)
{
l_int32    w, h, hs, i, ystart, yend, ovlap, npts;
l_float32  angle, conf, ycenter, a, b;
BOX       *box;
NUMA      *naskew;
PIX       *pix;
PTA       *pta;

    PROCNAME("pixGetLocalSkewAngles");

    if (!pixs)
        return (NUMA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (nslices < 2 || nslices > 20)
        nslices = DEFAULT_SLICES;
    if (redsweep < 1 || redsweep > 8)
        redsweep = DEFAULT_SWEEP_REDUCTION;
    if (redsearch < 1 || redsearch > redsweep)
        redsearch = DEFAULT_BS_REDUCTION;
    if (sweeprange == 0.0)
        sweeprange = DEFAULT_SWEEP_RANGE;
    if (sweepdelta == 0.0)
        sweepdelta = DEFAULT_SWEEP_DELTA;
    if (minbsdelta == 0.0)
        minbsdelta = DEFAULT_MINBS_DELTA;

    h = pixGetHeight(pixs);
    w = pixGetWidth(pixs);
    hs = h / nslices;
    ovlap = (l_int32)(OVERLAP_FRACTION * hs);
    pta = ptaCreate(nslices);
    for (i = 0; i < nslices; i++) {
        ystart = L_MAX(0, hs * i - ovlap);
        yend = L_MIN(h - 1, hs * (i + 1) + ovlap);
        ycenter = (ystart + yend) / 2;
        box = boxCreate(0, ystart, w, yend - ystart + 1);
        pix = pixClipRectangle(pixs, box, NULL);
        pixFindSkewSweepAndSearch(pix, &angle, &conf, redsweep, redsearch,
                                  sweeprange, sweepdelta, minbsdelta);
        if (conf > MIN_ALLOWED_CONFIDENCE)
            ptaAddPt(pta, ycenter, angle);
        pixDestroy(&pix);
        boxDestroy(&box);
    }
/*    ptaWriteStream(stderr, pta, 0); */

        /* Do linear least squares fit */
    if ((npts = ptaGetCount(pta)) < 2) {
        ptaDestroy(&pta);
        return (NUMA *)ERROR_PTR("can't fit skew", procName, NULL);
    }
    ptaGetLinearLSF(pta, &a, &b, NULL);
    if (pa) *pa = a;
    if (pb) *pb = b;

        /* Make skew angle array as function of raster line */
    naskew = numaCreate(h);
    for (i = 0; i < h; i++) {
        angle = a * i + b;
        numaAddNumber(naskew, angle);
    }

#if  DEBUG_PLOT
{ NUMA   *nax, *nay;
  GPLOT  *gplot;
    ptaGetArrays(pta, &nax, &nay);
    gplot = gplotCreate("junkskew", GPLOT_X11, "skew as fctn of y",
                        "y (in raster lines from top)", "angle (in degrees)");
    gplotAddPlot(gplot, NULL, naskew, GPLOT_POINTS, "linear lsf");
    gplotAddPlot(gplot, nax, nay, GPLOT_POINTS, "actual data pts");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&nax);
    numaDestroy(&nay);
}
#endif  /* DEBUG_PLOT */

    ptaDestroy(&pta);
    return naskew;
}
