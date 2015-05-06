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
 *  skew.c
 *
 *      Top-level deskew interfaces
 *          PIX       *pixDeskew()
 *          PIX       *pixFindSkewAndDeskew()
 *          PIX       *pixDeskewGeneral()
 *
 *      Top-level angle-finding interface
 *          l_int32    pixFindSkew()
 *
 *      Basic angle-finding functions
 *          l_int32    pixFindSkewSweep()
 *          l_int32    pixFindSkewSweepAndSearch()
 *          l_int32    pixFindSkewSweepAndSearchScore()
 *          l_int32    pixFindSkewSweepAndSearchScorePivot()
 *
 *      Search over arbitrary range of angles in orthogonal directions
 *          l_int32    pixFindSkewOrthogonalRange()
 *
 *      Differential square sum function for scoring
 *          l_int32    pixFindDifferentialSquareSum()
 *
 *      Measures of variance of row sums
 *          l_int32    pixFindNormalizedSquareSum()
 *
 *
 *      ==============================================================
 *      Page skew detection
 *
 *      Skew is determined by pixel profiles, which are computed
 *      as pixel sums along the raster line for each line in the
 *      image.  By vertically shearing the image by a given angle,
 *      the sums can be computed quickly along the raster lines
 *      rather than along lines at that angle.  The score is
 *      computed from these line sums by taking the square of
 *      the DIFFERENCE between adjacent line sums, summed over
 *      all lines.  The skew angle is then found as the angle
 *      that maximizes the score.  The actual computation for
 *      any sheared image is done in the function
 *      pixFindDifferentialSquareSum().
 *
 *      The search for the angle that maximizes this score is
 *      most efficiently performed by first sweeping coarsely
 *      over angles, using a significantly reduced image (say, 4x
 *      reduction), to find the approximate maximum within a half
 *      degree or so, and then doing an interval-halving binary
 *      search at higher resolution to get the skew angle to
 *      within 1/20 degree or better.
 *
 *      The differential signal is used (rather than just using
 *      that variance of line sums) because it rejects the
 *      background noise due to total number of black pixels,
 *      and has maximum contributions from the baselines and
 *      x-height lines of text when the textlines are aligned
 *      with the raster lines.  It also works well in multicolumn
 *      pages where the textlines do not line up across columns.
 *
 *      The method is fast, accurate to within an angle (in radians)
 *      of approximately the inverse width in pixels of the image,
 *      and will work on a surprisingly small amount of text data
 *      (just a couple of text lines).  Consequently, it can
 *      also be used to find local skew if the skew were to vary
 *      significantly over the page.  Local skew determination
 *      is not very important except for locating lines of
 *      handwritten text that may be mixed with printed text.
 */

#include <math.h>
#include "allheaders.h"

    /* Default sweep angle parameters for pixFindSkew() */
static const l_float32  DEFAULT_SWEEP_RANGE = 7.;    /* degrees */
static const l_float32  DEFAULT_SWEEP_DELTA = 1.;    /* degrees */

    /* Default final angle difference parameter for binary
     * search in pixFindSkew().  The expected accuracy is
     * not better than the inverse image width in pixels,
     * say, 1/2000 radians, or about 0.03 degrees. */
static const l_float32  DEFAULT_MINBS_DELTA = 0.01;  /* degrees */

    /* Default scale factors for pixFindSkew() */
static const l_int32  DEFAULT_SWEEP_REDUCTION = 4;  /* sweep part; 4 is good */
static const l_int32  DEFAULT_BS_REDUCTION = 2;  /* binary search part */

    /* Minimum angle for deskewing in pixDeskew() */
static const l_float32  MIN_DESKEW_ANGLE = 0.1;  /* degree */

    /* Minimum allowed confidence (ratio) for deskewing in pixDeskew() */
static const l_float32  MIN_ALLOWED_CONFIDENCE = 3.0;

    /* Minimum allowed maxscore to give nonzero confidence */
static const l_int32  MIN_VALID_MAXSCORE = 10000;

    /* Constant setting threshold for minimum allowed minscore
     * to give nonzero confidence; multiply this constant by
     *  (height * width^2) */
static const l_float32  MINSCORE_THRESHOLD_CONSTANT = 0.000002;

    /* Default binarization threshold value */
static const l_int32  DEFAULT_BINARY_THRESHOLD = 130;

#ifndef  NO_CONSOLE_IO
#define  DEBUG_PRINT_SCORES     0
#define  DEBUG_PRINT_SWEEP      0
#define  DEBUG_PRINT_BINARY     0
#define  DEBUG_PRINT_ORTH       0
#define  DEBUG_THRESHOLD        0
#define  DEBUG_PLOT_SCORES      0  /* requires the gnuplot executable */
#endif  /* ~NO_CONSOLE_IO */



/*-----------------------------------------------------------------------*
 *                       Top-level deskew interfaces                     *
 *-----------------------------------------------------------------------*/
/*!
 *  pixDeskew()
 *
 *      Input:  pixs (any depth)
 *              redsearch (for binary search: reduction factor = 1, 2 or 4;
 *                         use 0 for default)
 *      Return: pixd (deskewed pix), or null on error
 *
 *  Notes:
 *      (1) This binarizes if necessary and finds the skew angle.  If the
 *          angle is large enough and there is sufficient confidence,
 *          it returns a deskewed image; otherwise, it returns a clone.
 */
PIX *
pixDeskew(PIX     *pixs,
          l_int32  redsearch)
{
    PROCNAME("pixDeskew");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (redsearch == 0)
        redsearch = DEFAULT_BS_REDUCTION;
    else if (redsearch != 1 && redsearch != 2 && redsearch != 4)
        return (PIX *)ERROR_PTR("redsearch not in {1,2,4}", procName, NULL);

    return pixDeskewGeneral(pixs, 0, 0.0, 0.0, redsearch, 0, NULL, NULL);
}


/*!
 *  pixFindSkewAndDeskew()
 *
 *      Input:  pixs (any depth)
 *              redsearch (for binary search: reduction factor = 1, 2 or 4;
 *                         use 0 for default)
 *              &angle   (<optional return> angle required to deskew,
 *                        in degrees; use NULL to skip)
 *              &conf    (<optional return> conf value is ratio
 *                        of max/min scores; use NULL to skip)
 *      Return: pixd (deskewed pix), or null on error
 *
 *  Notes:
 *      (1) This binarizes if necessary and finds the skew angle.  If the
 *          angle is large enough and there is sufficient confidence,
 *          it returns a deskewed image; otherwise, it returns a clone.
 */
PIX *
pixFindSkewAndDeskew(PIX        *pixs,
                     l_int32     redsearch,
                     l_float32  *pangle,
                     l_float32  *pconf)
{
    PROCNAME("pixFindSkewAndDeskew");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (redsearch == 0)
        redsearch = DEFAULT_BS_REDUCTION;
    else if (redsearch != 1 && redsearch != 2 && redsearch != 4)
        return (PIX *)ERROR_PTR("redsearch not in {1,2,4}", procName, NULL);

    return pixDeskewGeneral(pixs, 0, 0.0, 0.0, redsearch, 0, pangle, pconf);
}


/*!
 *  pixDeskewGeneral()
 *
 *      Input:  pixs  (any depth)
 *              redsweep  (for linear search: reduction factor = 1, 2 or 4;
 *                         use 0 for default)
 *              sweeprange (in degrees in each direction from 0;
 *                          use 0.0 for default)
 *              sweepdelta (in degrees; use 0.0 for default)
 *              redsearch  (for binary search: reduction factor = 1, 2 or 4;
 *                          use 0 for default;)
 *              thresh (for binarizing the image; use 0 for default)
 *              &angle   (<optional return> angle required to deskew,
 *                        in degrees; use NULL to skip)
 *              &conf    (<optional return> conf value is ratio
 *                        of max/min scores; use NULL to skip)
 *      Return: pixd (deskewed pix), or null on error
 *
 *  Notes:
 *      (1) This binarizes if necessary and finds the skew angle.  If the
 *          angle is large enough and there is sufficient confidence,
 *          it returns a deskewed image; otherwise, it returns a clone.
 */
PIX *
pixDeskewGeneral(PIX        *pixs,
                 l_int32     redsweep,
                 l_float32   sweeprange,
                 l_float32   sweepdelta,
                 l_int32     redsearch,
                 l_int32     thresh,
                 l_float32  *pangle,
                 l_float32  *pconf)
{
l_int32    ret, depth;
l_float32  angle, conf, deg2rad;
PIX       *pixb, *pixd;

    PROCNAME("pixDeskewGeneral");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (redsweep == 0)
        redsweep = DEFAULT_SWEEP_REDUCTION;
    else if (redsweep != 1 && redsweep != 2 && redsweep != 4)
        return (PIX *)ERROR_PTR("redsweep not in {1,2,4}", procName, NULL);
    if (sweeprange == 0.0)
        sweeprange = DEFAULT_SWEEP_RANGE;
    if (sweepdelta == 0.0)
        sweepdelta = DEFAULT_SWEEP_DELTA;
    if (redsearch == 0)
        redsearch = DEFAULT_BS_REDUCTION;
    else if (redsearch != 1 && redsearch != 2 && redsearch != 4)
        return (PIX *)ERROR_PTR("redsearch not in {1,2,4}", procName, NULL);
    if (thresh == 0)
        thresh = DEFAULT_BINARY_THRESHOLD;

    deg2rad = 3.1415926535 / 180.;

        /* Binarize if necessary */
    depth = pixGetDepth(pixs);
    if (depth == 1)
        pixb = pixClone(pixs);
    else
        pixb = pixConvertTo1(pixs, thresh);

        /* Use the 1 bpp image to find the skew */
    ret = pixFindSkewSweepAndSearch(pixb, &angle, &conf, redsweep, redsearch,
                                    sweeprange, sweepdelta,
                                    DEFAULT_MINBS_DELTA);
    pixDestroy(&pixb);
    if (pangle)
        *pangle = angle;
    if (pconf)
        *pconf = conf;
    if (ret)
        return pixClone(pixs);

    if (L_ABS(angle) < MIN_DESKEW_ANGLE || conf < MIN_ALLOWED_CONFIDENCE)
        return pixClone(pixs);

    if ((pixd = pixRotate(pixs, deg2rad * angle, L_ROTATE_AREA_MAP,
                          L_BRING_IN_WHITE, 0, 0)) == NULL)
        return pixClone(pixs);
    else
        return pixd;
}


/*-----------------------------------------------------------------------*
 *                  Simple top-level angle-finding interface             *
 *-----------------------------------------------------------------------*/
/*!
 *  pixFindSkew()
 *
 *      Input:  pixs  (1 bpp)
 *              &angle   (<return> angle required to deskew, in degrees)
 *              &conf    (<return> confidence value is ratio max/min scores)
 *      Return: 0 if OK, 1 on error or if angle measurment not valid
 *
 *  Notes:
 *      (1) This is a simple high-level interface, that uses default
 *          values of the parameters for reasonable speed and accuracy.
 *      (2) The angle returned is the negative of the skew angle of
 *          the image.  It is the angle required for deskew.
 *          Clockwise rotations are positive angles.
 */
l_int32
pixFindSkew(PIX        *pixs,
            l_float32  *pangle,
            l_float32  *pconf)
{
    PROCNAME("pixFindSkew");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not 1 bpp", procName, 1);
    if (!pangle)
        return ERROR_INT("&angle not defined", procName, 1);
    if (!pconf)
        return ERROR_INT("&conf not defined", procName, 1);

    return pixFindSkewSweepAndSearch(pixs, pangle, pconf,
                                     DEFAULT_SWEEP_REDUCTION,
                                     DEFAULT_BS_REDUCTION,
                                     DEFAULT_SWEEP_RANGE,
                                     DEFAULT_SWEEP_DELTA,
                                     DEFAULT_MINBS_DELTA);
}


/*-----------------------------------------------------------------------*
 *                       Basic angle-finding functions                   *
 *-----------------------------------------------------------------------*/
/*!
 *  pixFindSkewSweep()
 *
 *      Input:  pixs  (1 bpp)
 *              &angle   (<return> angle required to deskew, in degrees)
 *              reduction  (factor = 1, 2, 4 or 8)
 *              sweeprange   (half the full range; assumed about 0; in degrees)
 *              sweepdelta   (angle increment of sweep; in degrees)
 *      Return: 0 if OK, 1 on error or if angle measurment not valid
 *
 *  Notes:
 *      (1) This examines the 'score' for skew angles with equal intervals.
 *      (2) Caller must check the return value for validity of the result.
 */
l_int32
pixFindSkewSweep(PIX        *pixs,
                 l_float32  *pangle,
                 l_int32     reduction,
                 l_float32   sweeprange,
                 l_float32   sweepdelta)
{
l_int32    ret, bzero, i, nangles;
l_float32  deg2rad, theta;
l_float32  sum, maxscore, maxangle;
NUMA      *natheta, *nascore;
PIX       *pix, *pixt;

    PROCNAME("pixFindSkewSweep");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not 1 bpp", procName, 1);
    if (!pangle)
        return ERROR_INT("&angle not defined", procName, 1);
    if (reduction != 1 && reduction != 2 && reduction != 4 && reduction != 8)
        return ERROR_INT("reduction must be in {1,2,4,8}", procName, 1);

    *pangle = 0.0;  /* init */
    deg2rad = 3.1415926535 / 180.;
    ret = 0;

        /* Generate reduced image, if requested */
    if (reduction == 1)
        pix = pixClone(pixs);
    else if (reduction == 2)
        pix = pixReduceRankBinaryCascade(pixs, 1, 0, 0, 0);
    else if (reduction == 4)
        pix = pixReduceRankBinaryCascade(pixs, 1, 1, 0, 0);
    else /* reduction == 8 */
        pix = pixReduceRankBinaryCascade(pixs, 1, 1, 2, 0);

    pixZero(pix, &bzero);
    if (bzero) {
        pixDestroy(&pix);
        return 1;
    }

    nangles = (l_int32)((2. * sweeprange) / sweepdelta + 1);
    natheta = numaCreate(nangles);
    nascore = numaCreate(nangles);
    pixt = pixCreateTemplate(pix);

    if (!pix || !pixt) {
        ret = ERROR_INT("pix and pixt not both made", procName, 1);
        goto cleanup;
    }
    if (!natheta || !nascore) {
        ret = ERROR_INT("natheta and nascore not both made", procName, 1);
        goto cleanup;
    }

    for (i = 0; i < nangles; i++) {
        theta = -sweeprange + i * sweepdelta;   /* degrees */

            /* Shear pix about the UL corner and put the result in pixt */
        pixVShearCorner(pixt, pix, deg2rad * theta, L_BRING_IN_WHITE);

            /* Get score */
        pixFindDifferentialSquareSum(pixt, &sum);

#if  DEBUG_PRINT_SCORES
        L_INFO("sum(%7.2f) = %7.0f\n", procName, theta, sum);
#endif  /* DEBUG_PRINT_SCORES */

            /* Save the result in the output arrays */
        numaAddNumber(nascore, sum);
        numaAddNumber(natheta, theta);
    }

        /* Find the location of the maximum (i.e., the skew angle)
         * by fitting the largest data point and its two neighbors
         * to a quadratic, using lagrangian interpolation.  */
    numaFitMax(nascore, &maxscore, natheta, &maxangle);
    *pangle = maxangle;

#if  DEBUG_PRINT_SWEEP
    L_INFO(" From sweep: angle = %7.3f, score = %7.3f\n", procName,
           maxangle, maxscore);
#endif  /* DEBUG_PRINT_SWEEP */

#if  DEBUG_PLOT_SCORES
        /* Plot the result -- the scores versus rotation angle --
         * using gnuplot with GPLOT_LINES (lines connecting data points).
         * The GPLOT data structure is first created, with the
         * appropriate data incorporated from the two input NUMAs,
         * and then the function gplotMakeOutput() uses gnuplot to
         * generate the output plot.  This can be either a .png file
         * or a .ps file, depending on whether you use GPLOT_PNG
         * or GPLOT_PS.  */
    {GPLOT  *gplot;
        gplot = gplotCreate("sweep_output", GPLOT_PNG,
                    "Sweep. Variance of difference of ON pixels vs. angle",
                    "angle (deg)", "score");
        gplotAddPlot(gplot, natheta, nascore, GPLOT_LINES, "plot1");
        gplotAddPlot(gplot, natheta, nascore, GPLOT_POINTS, "plot2");
        gplotMakeOutput(gplot);
        gplotDestroy(&gplot);
    }
#endif  /* DEBUG_PLOT_SCORES */

cleanup:
    pixDestroy(&pix);
    pixDestroy(&pixt);
    numaDestroy(&nascore);
    numaDestroy(&natheta);
    return ret;
}


/*!
 *  pixFindSkewSweepAndSearch()
 *
 *      Input:  pixs  (1 bpp)
 *              &angle   (<return> angle required to deskew; in degrees)
 *              &conf    (<return> confidence given by ratio of max/min score)
 *              redsweep  (sweep reduction factor = 1, 2, 4 or 8)
 *              redsearch  (binary search reduction factor = 1, 2, 4 or 8;
 *                          and must not exceed redsweep)
 *              sweeprange   (half the full range, assumed about 0; in degrees)
 *              sweepdelta   (angle increment of sweep; in degrees)
 *              minbsdelta   (min binary search increment angle; in degrees)
 *      Return: 0 if OK, 1 on error or if angle measurment not valid
 *
 *  Notes:
 *      (1) This finds the skew angle, doing first a sweep through a set
 *          of equal angles, and then doing a binary search until
 *          convergence.
 *      (2) Caller must check the return value for validity of the result.
 *      (3) In computing the differential line sum variance score, we sum
 *          the result over scanlines, but we always skip:
 *           - at least one scanline
 *           - not more than 10% of the image height
 *           - not more than 5% of the image width
 *      (4) See also notes in pixFindSkewSweepAndSearchScore()
 */
l_int32
pixFindSkewSweepAndSearch(PIX        *pixs,
                          l_float32  *pangle,
                          l_float32  *pconf,
                          l_int32     redsweep,
                          l_int32     redsearch,
                          l_float32   sweeprange,
                          l_float32   sweepdelta,
                          l_float32   minbsdelta)
{
    return pixFindSkewSweepAndSearchScore(pixs, pangle, pconf, NULL,
                                          redsweep, redsearch, 0.0, sweeprange,
                                          sweepdelta, minbsdelta);
}


/*!
 *  pixFindSkewSweepAndSearchScore()
 *
 *      Input:  pixs  (1 bpp)
 *              &angle   (<return> angle required to deskew; in degrees)
 *              &conf    (<return> confidence given by ratio of max/min score)
 *              &endscore (<optional return> max score; use NULL to ignore)
 *              redsweep  (sweep reduction factor = 1, 2, 4 or 8)
 *              redsearch  (binary search reduction factor = 1, 2, 4 or 8;
 *                          and must not exceed redsweep)
 *              sweepcenter  (angle about which sweep is performed; in degrees)
 *              sweeprange   (half the full range, taken about sweepcenter;
 *                            in degrees)
 *              sweepdelta   (angle increment of sweep; in degrees)
 *              minbsdelta   (min binary search increment angle; in degrees)
 *      Return: 0 if OK, 1 on error or if angle measurment not valid
 *
 *  Notes:
 *      (1) This finds the skew angle, doing first a sweep through a set
 *          of equal angles, and then doing a binary search until convergence.
 *      (2) There are two built-in constants that determine if the
 *          returned confidence is nonzero:
 *            - MIN_VALID_MAXSCORE (minimum allowed maxscore)
 *            - MINSCORE_THRESHOLD_CONSTANT (determines minimum allowed
 *                 minscore, by multiplying by (height * width^2)
 *          If either of these conditions is not satisfied, the returned
 *          confidence value will be zero.  The maxscore is optionally
 *          returned in this function to allow evaluation of the
 *          resulting angle by a method that is independent of the
 *          returned confidence value.
 *      (3) The larger the confidence value, the greater the probability
 *          that the proper alignment is given by the angle that maximizes
 *          variance.  It should be compared to a threshold, which depends
 *          on the application.  Values between 3.0 and 6.0 are common.
 *      (4) By default, the shear is about the UL corner.
 */
l_int32
pixFindSkewSweepAndSearchScore(PIX        *pixs,
                               l_float32  *pangle,
                               l_float32  *pconf,
                               l_float32  *pendscore,
                               l_int32     redsweep,
                               l_int32     redsearch,
                               l_float32   sweepcenter,
                               l_float32   sweeprange,
                               l_float32   sweepdelta,
                               l_float32   minbsdelta)
{
    return pixFindSkewSweepAndSearchScorePivot(pixs, pangle, pconf, pendscore,
                                               redsweep, redsearch, 0.0,
                                               sweeprange, sweepdelta,
                                               minbsdelta,
                                               L_SHEAR_ABOUT_CORNER);
}


/*!
 *  pixFindSkewSweepAndSearchScorePivot()
 *
 *      Input:  pixs  (1 bpp)
 *              &angle   (<return> angle required to deskew; in degrees)
 *              &conf    (<return> confidence given by ratio of max/min score)
 *              &endscore (<optional return> max score; use NULL to ignore)
 *              redsweep  (sweep reduction factor = 1, 2, 4 or 8)
 *              redsearch  (binary search reduction factor = 1, 2, 4 or 8;
 *                          and must not exceed redsweep)
 *              sweepcenter  (angle about which sweep is performed; in degrees)
 *              sweeprange   (half the full range, taken about sweepcenter;
 *                            in degrees)
 *              sweepdelta   (angle increment of sweep; in degrees)
 *              minbsdelta   (min binary search increment angle; in degrees)
 *              pivot  (L_SHEAR_ABOUT_CORNER, L_SHEAR_ABOUT_CENTER)
 *      Return: 0 if OK, 1 on error or if angle measurment not valid
 *
 *  Notes:
 *      (1) See notes in pixFindSkewSweepAndSearchScore().
 *      (2) This allows choice of shear pivoting from either the UL corner
 *          or the center.  For small angles, the ability to discriminate
 *          angles is better with shearing from the UL corner.  However,
 *          for large angles (say, greater than 20 degrees), it is better
 *          to shear about the center because a shear from the UL corner
 *          loses too much of the image.
 */
l_int32
pixFindSkewSweepAndSearchScorePivot(PIX        *pixs,
                                    l_float32  *pangle,
                                    l_float32  *pconf,
                                    l_float32  *pendscore,
                                    l_int32     redsweep,
                                    l_int32     redsearch,
                                    l_float32   sweepcenter,
                                    l_float32   sweeprange,
                                    l_float32   sweepdelta,
                                    l_float32   minbsdelta,
                                    l_int32     pivot)
{
l_int32    ret, bzero, i, nangles, n, ratio, maxindex, minloc;
l_int32    width, height;
l_float32  deg2rad, theta, delta;
l_float32  sum, maxscore, maxangle;
l_float32  centerangle, leftcenterangle, rightcenterangle;
l_float32  lefttemp, righttemp;
l_float32  bsearchscore[5];
l_float32  minscore, minthresh;
l_float32  rangeleft;
NUMA      *natheta, *nascore;
PIX       *pixsw, *pixsch, *pixt1, *pixt2;

    PROCNAME("pixFindSkewSweepAndSearchScorePivot");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not 1 bpp", procName, 1);
    if (!pangle)
        return ERROR_INT("&angle not defined", procName, 1);
    if (!pconf)
        return ERROR_INT("&conf not defined", procName, 1);
    if (redsweep != 1 && redsweep != 2 && redsweep != 4 && redsweep != 8)
        return ERROR_INT("redsweep must be in {1,2,4,8}", procName, 1);
    if (redsearch != 1 && redsearch != 2 && redsearch != 4 && redsearch != 8)
        return ERROR_INT("redsearch must be in {1,2,4,8}", procName, 1);
    if (redsearch > redsweep)
        return ERROR_INT("redsearch must not exceed redsweep", procName, 1);
    if (pivot != L_SHEAR_ABOUT_CORNER && pivot != L_SHEAR_ABOUT_CENTER)
        return ERROR_INT("invalid pivot", procName, 1);

    *pangle = 0.0;
    *pconf = 0.0;
    deg2rad = 3.1415926535 / 180.;
    ret = 0;

        /* Generate reduced image for binary search, if requested */
    if (redsearch == 1)
        pixsch = pixClone(pixs);
    else if (redsearch == 2)
        pixsch = pixReduceRankBinaryCascade(pixs, 1, 0, 0, 0);
    else if (redsearch == 4)
        pixsch = pixReduceRankBinaryCascade(pixs, 1, 1, 0, 0);
    else  /* redsearch == 8 */
        pixsch = pixReduceRankBinaryCascade(pixs, 1, 1, 2, 0);

    pixZero(pixsch, &bzero);
    if (bzero) {
        pixDestroy(&pixsch);
        return 1;
    }

        /* Generate reduced image for sweep, if requested */
    ratio = redsweep / redsearch;
    if (ratio == 1) {
        pixsw = pixClone(pixsch);
    } else {  /* ratio > 1 */
        if (ratio == 2)
            pixsw = pixReduceRankBinaryCascade(pixsch, 1, 0, 0, 0);
        else if (ratio == 4)
            pixsw = pixReduceRankBinaryCascade(pixsch, 1, 2, 0, 0);
        else  /* ratio == 8 */
            pixsw = pixReduceRankBinaryCascade(pixsch, 1, 2, 2, 0);
    }

    pixt1 = pixCreateTemplate(pixsw);
    if (ratio == 1)
        pixt2 = pixClone(pixt1);
    else
        pixt2 = pixCreateTemplate(pixsch);

    nangles = (l_int32)((2. * sweeprange) / sweepdelta + 1);
    natheta = numaCreate(nangles);
    nascore = numaCreate(nangles);

    if (!pixsch || !pixsw) {
        ret = ERROR_INT("pixsch and pixsw not both made", procName, 1);
        goto cleanup;
    }
    if (!pixt1 || !pixt2) {
        ret = ERROR_INT("pixt1 and pixt2 not both made", procName, 1);
        goto cleanup;
    }
    if (!natheta || !nascore) {
        ret = ERROR_INT("natheta and nascore not both made", procName, 1);
        goto cleanup;
    }

        /* Do sweep */
    rangeleft = sweepcenter - sweeprange;
    for (i = 0; i < nangles; i++) {
        theta = rangeleft + i * sweepdelta;   /* degrees */

            /* Shear pix and put the result in pixt1 */
        if (pivot == L_SHEAR_ABOUT_CORNER)
            pixVShearCorner(pixt1, pixsw, deg2rad * theta, L_BRING_IN_WHITE);
        else
            pixVShearCenter(pixt1, pixsw, deg2rad * theta, L_BRING_IN_WHITE);

            /* Get score */
        pixFindDifferentialSquareSum(pixt1, &sum);

#if  DEBUG_PRINT_SCORES
        L_INFO("sum(%7.2f) = %7.0f\n", procName, theta, sum);
#endif  /* DEBUG_PRINT_SCORES */

            /* Save the result in the output arrays */
        numaAddNumber(nascore, sum);
        numaAddNumber(natheta, theta);
    }

        /* Find the largest of the set (maxscore at maxangle) */
    numaGetMax(nascore, &maxscore, &maxindex);
    numaGetFValue(natheta, maxindex, &maxangle);

#if  DEBUG_PRINT_SWEEP
    L_INFO(" From sweep: angle = %7.3f, score = %7.3f\n", procName,
           maxangle, maxscore);
#endif  /* DEBUG_PRINT_SWEEP */

#if  DEBUG_PLOT_SCORES
        /* Plot the sweep result -- the scores versus rotation angle --
         * using gnuplot with GPLOT_LINES (lines connecting data points). */
    {GPLOT  *gplot;
        gplot = gplotCreate("sweep_output", GPLOT_PNG,
                    "Sweep. Variance of difference of ON pixels vs. angle",
                    "angle (deg)", "score");
        gplotAddPlot(gplot, natheta, nascore, GPLOT_LINES, "plot1");
        gplotAddPlot(gplot, natheta, nascore, GPLOT_POINTS, "plot2");
        gplotMakeOutput(gplot);
        gplotDestroy(&gplot);
    }
#endif  /* DEBUG_PLOT_SCORES */

        /* Check if the max is at the end of the sweep. */
    n = numaGetCount(natheta);
    if (maxindex == 0 || maxindex == n - 1) {
        L_WARNING("max found at sweep edge\n", procName);
        goto cleanup;
    }

        /* Empty the numas for re-use */
    numaEmpty(nascore);
    numaEmpty(natheta);

        /* Do binary search to find skew angle.
         * First, set up initial three points. */
    centerangle = maxangle;
    if (pivot == L_SHEAR_ABOUT_CORNER) {
        pixVShearCorner(pixt2, pixsch, deg2rad * centerangle, L_BRING_IN_WHITE);
        pixFindDifferentialSquareSum(pixt2, &bsearchscore[2]);
        pixVShearCorner(pixt2, pixsch, deg2rad * (centerangle - sweepdelta),
                        L_BRING_IN_WHITE);
        pixFindDifferentialSquareSum(pixt2, &bsearchscore[0]);
        pixVShearCorner(pixt2, pixsch, deg2rad * (centerangle + sweepdelta),
                        L_BRING_IN_WHITE);
        pixFindDifferentialSquareSum(pixt2, &bsearchscore[4]);
    } else {
        pixVShearCenter(pixt2, pixsch, deg2rad * centerangle, L_BRING_IN_WHITE);
        pixFindDifferentialSquareSum(pixt2, &bsearchscore[2]);
        pixVShearCenter(pixt2, pixsch, deg2rad * (centerangle - sweepdelta),
                        L_BRING_IN_WHITE);
        pixFindDifferentialSquareSum(pixt2, &bsearchscore[0]);
        pixVShearCenter(pixt2, pixsch, deg2rad * (centerangle + sweepdelta),
                        L_BRING_IN_WHITE);
        pixFindDifferentialSquareSum(pixt2, &bsearchscore[4]);
    }

    numaAddNumber(nascore, bsearchscore[2]);
    numaAddNumber(natheta, centerangle);
    numaAddNumber(nascore, bsearchscore[0]);
    numaAddNumber(natheta, centerangle - sweepdelta);
    numaAddNumber(nascore, bsearchscore[4]);
    numaAddNumber(natheta, centerangle + sweepdelta);

        /* Start the search */
    delta = 0.5 * sweepdelta;
    while (delta >= minbsdelta)
    {
            /* Get the left intermediate score */
        leftcenterangle = centerangle - delta;
        if (pivot == L_SHEAR_ABOUT_CORNER)
            pixVShearCorner(pixt2, pixsch, deg2rad * leftcenterangle,
                            L_BRING_IN_WHITE);
        else
            pixVShearCenter(pixt2, pixsch, deg2rad * leftcenterangle,
                            L_BRING_IN_WHITE);
        pixFindDifferentialSquareSum(pixt2, &bsearchscore[1]);
        numaAddNumber(nascore, bsearchscore[1]);
        numaAddNumber(natheta, leftcenterangle);

            /* Get the right intermediate score */
        rightcenterangle = centerangle + delta;
        if (pivot == L_SHEAR_ABOUT_CORNER)
            pixVShearCorner(pixt2, pixsch, deg2rad * rightcenterangle,
                            L_BRING_IN_WHITE);
        else
            pixVShearCenter(pixt2, pixsch, deg2rad * rightcenterangle,
                            L_BRING_IN_WHITE);
        pixFindDifferentialSquareSum(pixt2, &bsearchscore[3]);
        numaAddNumber(nascore, bsearchscore[3]);
        numaAddNumber(natheta, rightcenterangle);

            /* Find the maximum of the five scores and its location.
             * Note that the maximum must be in the center
             * three values, not in the end two. */
        maxscore = bsearchscore[1];
        maxindex = 1;
        for (i = 2; i < 4; i++) {
            if (bsearchscore[i] > maxscore) {
                maxscore = bsearchscore[i];
                maxindex = i;
            }
        }

            /* Set up score array to interpolate for the next iteration */
        lefttemp = bsearchscore[maxindex - 1];
        righttemp = bsearchscore[maxindex + 1];
        bsearchscore[2] = maxscore;
        bsearchscore[0] = lefttemp;
        bsearchscore[4] = righttemp;

            /* Get new center angle and delta for next iteration */
        centerangle = centerangle + delta * (maxindex - 2);
        delta = 0.5 * delta;
    }
    *pangle = centerangle;

#if  DEBUG_PRINT_SCORES
    L_INFO(" Binary search score = %7.3f\n", procName, bsearchscore[2]);
#endif  /* DEBUG_PRINT_SCORES */

    if (pendscore)  /* save if requested */
        *pendscore = bsearchscore[2];

        /* Return the ratio of Max score over Min score
         * as a confidence value.  Don't trust if the Min score
         * is too small, which can happen if the image is all black
         * with only a few white pixels interspersed.  In that case,
         * we get a contribution from the top and bottom edges when
         * vertically sheared, but this contribution becomes zero when
         * the shear angle is zero.  For zero shear angle, the only
         * contribution will be from the white pixels.  We expect that
         * the signal goes as the product of the (height * width^2),
         * so we compute a (hopefully) normalized minimum threshold as
         * a function of these dimensions.  */
    numaGetMin(nascore, &minscore, &minloc);
    width = pixGetWidth(pixsch);
    height = pixGetHeight(pixsch);
    minthresh = MINSCORE_THRESHOLD_CONSTANT * width * width * height;

#if  DEBUG_THRESHOLD
    L_INFO(" minthresh = %10.2f, minscore = %10.2f\n", procName,
           minthresh, minscore);
    L_INFO(" maxscore = %10.2f\n", procName, maxscore);
#endif  /* DEBUG_THRESHOLD */

    if (minscore > minthresh)
        *pconf = maxscore / minscore;
    else
        *pconf = 0.0;

        /* Don't trust it if too close to the edge of the sweep
         * range or if maxscore is small */
    if ((centerangle > rangeleft + 2 * sweeprange - sweepdelta) ||
        (centerangle < rangeleft + sweepdelta) ||
        (maxscore < MIN_VALID_MAXSCORE))
        *pconf = 0.0;

#if  DEBUG_PRINT_BINARY
    fprintf(stderr, "Binary search: angle = %7.3f, score ratio = %6.2f\n",
            *pangle, *pconf);
    fprintf(stderr, "               max score = %8.0f\n", maxscore);
#endif  /* DEBUG_PRINT_BINARY */

#if  DEBUG_PLOT_SCORES
        /* Plot the result -- the scores versus rotation angle --
         * using gnuplot with GPLOT_POINTS.  Because the data
         * points are not ordered by theta (increasing or decreasing),
         * using GPLOT_LINES would be confusing! */
    {GPLOT  *gplot;
        gplot = gplotCreate("search_output", GPLOT_PNG,
                "Binary search.  Variance of difference of ON pixels vs. angle",
                "angle (deg)", "score");
        gplotAddPlot(gplot, natheta, nascore, GPLOT_POINTS, "plot1");
        gplotMakeOutput(gplot);
        gplotDestroy(&gplot);
    }
#endif  /* DEBUG_PLOT_SCORES */

cleanup:
    pixDestroy(&pixsw);
    pixDestroy(&pixsch);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    numaDestroy(&nascore);
    numaDestroy(&natheta);
    return ret;
}


/*---------------------------------------------------------------------*
 *    Search over arbitrary range of angles in orthogonal directions   *
 *---------------------------------------------------------------------*/
/*
 *   pixFindSkewOrthogonalRange()
 *
 *      Input:  pixs  (1 bpp)
 *              &angle  (<return> angle required to deskew; in degrees cw)
 *              &conf   (<return> confidence given by ratio of max/min score)
 *              redsweep  (sweep reduction factor = 1, 2, 4 or 8)
 *              redsearch  (binary search reduction factor = 1, 2, 4 or 8;
 *                          and must not exceed redsweep)
 *              sweeprange  (half the full range in each orthogonal
 *                           direction, taken about 0, in degrees)
 *              sweepdelta   (angle increment of sweep; in degrees)
 *              minbsdelta   (min binary search increment angle; in degrees)
 *              confprior  (amount by which confidence of 90 degree rotated
 *                          result is reduced when comparing with unrotated
 *                          confidence value)
 *      Return: 0 if OK, 1 on error or if angle measurment not valid
 *
 *  Notes:
 *      (1) This searches for the skew angle, first in the range
 *          [-sweeprange, sweeprange], and then in
 *          [90 - sweeprange, 90 + sweeprange], with angles measured
 *          clockwise.  For exploring the full range of possibilities,
 *          suggest using sweeprange = 47.0 degrees, giving some overlap
 *          at 45 and 135 degrees.  From these results, and discounting
 *          the the second confidence by @confprior, it selects the
 *          angle for maximal differential variance.  If the angle
 *          is larger than pi/4, the angle found after 90 degree rotation
 *          is selected.
 *      (2) The larger the confidence value, the greater the probability
 *          that the proper alignment is given by the angle that maximizes
 *          variance.  It should be compared to a threshold, which depends
 *          on the application.  Values between 3.0 and 6.0 are common.
 *      (3) Allowing for both portrait and landscape searches is more
 *          difficult, because if the signal from the text lines is weak,
 *          a signal from vertical rules can be larger!
 *          The most difficult documents to deskew have some or all of:
 *            (a) Multiple columns, not aligned
 *            (b) Black lines along the vertical edges
 *            (c) Text from two pages, and at different angles
 *          Rule of thumb for resolution:
 *            (a) If the margins are clean, you can work at 75 ppi,
 *                although 100 ppi is safer.
 *            (b) If there are vertical lines in the margins, do not
 *                work below 150 ppi.  The signal from the text lines must
 *                exceed that from the margin lines.
 *      (4) Choosing the @confprior parameter depends on knowing something
 *          about the source of image.  However, we're not using
 *          real probabilities here, so its use is qualitative.
 *          If landscape and portrait are equally likely, use
 *          @confprior = 0.0.  If the likelihood of portrait (non-rotated)
 *          is 100 times higher than that of landscape, we want to reduce
 *          the chance that we rotate to landscape in a situation where
 *          the landscape signal is accidentally larger than the
 *          portrait signal.  To do this use a positive value of
 *          @confprior; say 1.5.
 */
l_int32
pixFindSkewOrthogonalRange(PIX        *pixs,
                           l_float32  *pangle,
                           l_float32  *pconf,
                           l_int32     redsweep,
                           l_int32     redsearch,
                           l_float32   sweeprange,
                           l_float32   sweepdelta,
                           l_float32   minbsdelta,
                           l_float32   confprior)
{
l_float32  angle1, conf1, score1, angle2, conf2, score2;
PIX       *pixr;

    PROCNAME("pixFindSkewOrthogonalRange");

    if (!pangle || !pconf)
        return ERROR_INT("&angle and &conf not both defined", procName, 1);
    *pangle = *pconf = 0.0;
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

    pixFindSkewSweepAndSearchScorePivot(pixs, &angle1, &conf1, &score1,
                                        redsweep, redsearch, 0.0,
                                        sweeprange, sweepdelta, minbsdelta,
                                        L_SHEAR_ABOUT_CORNER);
    pixr = pixRotateOrth(pixs, 1);
    pixFindSkewSweepAndSearchScorePivot(pixr, &angle2, &conf2, &score2,
                                        redsweep, redsearch, 0.0,
                                        sweeprange, sweepdelta, minbsdelta,
                                        L_SHEAR_ABOUT_CORNER);
    pixDestroy(&pixr);

    if (conf1 > conf2 - confprior) {
        *pangle = angle1;
        *pconf = conf1;
    } else {
        *pangle = -90.0 + angle2;
        *pconf = conf2;
    }

#if  DEBUG_PRINT_ORTH
    fprintf(stderr, " About 0:  angle1 = %7.3f, conf1 = %7.3f, score1 = %f\n",
            angle1, conf1, score1);
    fprintf(stderr, " About 90: angle2 = %7.3f, conf2 = %7.3f, score2 = %f\n",
            angle2, conf2, score2);
    fprintf(stderr, " Final:    angle = %7.3f, conf = %7.3f\n", *pangle, *pconf);
#endif  /* DEBUG_PRINT_ORTH */

    return 0;
}



/*----------------------------------------------------------------*
 *                  Differential square sum function              *
 *----------------------------------------------------------------*/
/*!
 *  pixFindDifferentialSquareSum()
 *
 *      Input:  pixs
 *              &sum  (<return> result)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) At the top and bottom, we skip:
 *           - at least one scanline
 *           - not more than 10% of the image height
 *           - not more than 5% of the image width
 */
l_int32
pixFindDifferentialSquareSum(PIX        *pixs,
                             l_float32  *psum)
{
l_int32    i, n;
l_int32    w, h, skiph, skip, nskip;
l_float32  val1, val2, diff, sum;
NUMA      *na;

    PROCNAME("pixFindDifferentialSquareSum");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

        /* Generate a number array consisting of the sum
         * of pixels in each row of pixs */
    if ((na = pixCountPixelsByRow(pixs, NULL)) == NULL)
        return ERROR_INT("na not made", procName, 1);

        /* Compute the number of rows at top and bottom to omit.
         * We omit these to avoid getting a spurious signal from
         * the top and bottom of a (nearly) all black image. */
    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);
    skiph = (l_int32)(0.05 * w);  /* skip for max shear of 0.025 radians */
    skip = L_MIN(h / 10, skiph);  /* don't remove more than 10% of image */
    nskip = L_MAX(skip / 2, 1);  /* at top & bot; skip at least one line */

        /* Sum the squares of differential row sums, on the
         * allowed rows.  Note that nskip must be >= 1. */
    n = numaGetCount(na);
    sum = 0.0;
    for (i = nskip; i < n - nskip; i++) {
        numaGetFValue(na, i - 1, &val1);
        numaGetFValue(na, i, &val2);
        diff = val2 - val1;
        sum += diff * diff;
    }
    numaDestroy(&na);
    *psum = sum;
    return 0;
}


/*----------------------------------------------------------------*
 *                        Normalized square sum                   *
 *----------------------------------------------------------------*/
/*!
 *  pixFindNormalizedSquareSum()
 *
 *      Input:  pixs
 *              &hratio (<optional return> ratio of normalized horiz square sum
 *                       to result if the pixel distribution were uniform)
 *              &vratio (<optional return> ratio of normalized vert square sum
 *                       to result if the pixel distribution were uniform)
 *              &fract  (<optional return> ratio of fg pixels to total pixels)
 *      Return: 0 if OK, 1 on error or if there are no fg pixels
 *
 *  Notes:
 *      (1) Let the image have h scanlines and N fg pixels.
 *          If the pixels were uniformly distributed on scanlines,
 *          the sum of squares of fg pixels on each scanline would be
 *          h * (N / h)^2.  However, if the pixels are not uniformly
 *          distributed (e.g., for text), the sum of squares of fg
 *          pixels will be larger.  We return in hratio and vratio the
 *          ratio of these two values.
 *      (2) If there are no fg pixels, hratio and vratio are returned as 0.0.
 */
l_int32
pixFindNormalizedSquareSum(PIX        *pixs,
                           l_float32  *phratio,
                           l_float32  *pvratio,
                           l_float32  *pfract)
{
l_int32    i, w, h, empty;
l_float32  sum, sumsq, uniform, val;
NUMA      *na;
PIX       *pixt;

    PROCNAME("pixFindNormalizedSquareSum");

    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);
    pixGetDimensions(pixs, &w, &h, NULL);

    if (!phratio && !pvratio)
        return ERROR_INT("nothing to do", procName, 1);
    if (phratio) *phratio = 0.0;
    if (pvratio) *pvratio = 0.0;

    empty = 0;
    if (phratio) {
        na = pixCountPixelsByRow(pixs, NULL);
        numaGetSum(na, &sum);  /* fg pixels */
        if (pfract) *pfract = sum / (l_float32)(w * h);
        if (sum != 0.0) {
            uniform = sum * sum / h;   /*  h*(sum / h)^2  */
            sumsq = 0.0;
            for (i = 0; i < h; i++) {
                numaGetFValue(na, i, &val);
                sumsq += val * val;
            }
            *phratio = sumsq / uniform;
        } else {
            empty = 1;
        }
        numaDestroy(&na);
    }

    if (pvratio) {
        if (empty == 1) return 1;
        pixt = pixRotateOrth(pixs, 1);
        na = pixCountPixelsByRow(pixt, NULL);
        numaGetSum(na, &sum);
        if (pfract) *pfract = sum / (l_float32)(w * h);
        if (sum != 0.0) {
            uniform = sum * sum / w;
            sumsq = 0.0;
            for (i = 0; i < w; i++) {
                numaGetFValue(na, i, &val);
                sumsq += val * val;
            }
            *pvratio = sumsq / uniform;
        } else {
            empty = 1;
        }
        pixDestroy(&pixt);
        numaDestroy(&na);
    }

    return empty;
}
