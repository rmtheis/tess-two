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
 * numa1_reg.c
 *
 *   Tests:
 *     * histograms
 *     * interpolation
 *     * integration/differentiation
 *     * rank extraction
 *     * numa-morphology
 */

#include <math.h>
#ifndef  _WIN32
#include <unistd.h>
#else
#include <windows.h>   /* for Sleep() */
#endif  /* _WIN32 */
#include "allheaders.h"

#define   DO_ALL     1


int main(int    argc,
         char **argv)
{
l_int32      i, j, w, h, wpls, n, binsize, binstart, nbins;
l_float32    pi, val, angle, xval, yval, x0, y0, rank, startval, fbinsize;
l_float32    minval, maxval, meanval, median, variance, rankval;
GPLOT       *gplot;
NUMA        *na, *nahisto, *nax, *nay, *nap, *nasx, *nasy;
NUMA        *nadx, *nady, *nafx, *nafy, *na1, *na2, *na3, *na4;
PIX         *pix, *pixs, *pix1, *pix2, *pix3, *pix4, *pix5, *pixg, *pixd;
PIXA        *pixa;
static char  mainName[] = "numa1_reg";

    if (argc != 1)
        return ERROR_INT(" Syntax:  numa1_reg", mainName, 1);

    /* -------------------------------------------------------------------*
     *                            Histograms                              *
     * -------------------------------------------------------------------*/
#if  DO_ALL
    pi = 3.1415926535;
    na = numaCreate(5000);
    for (i = 0; i < 500000; i++) {
        angle = 0.02293 * i * pi;
        val = (l_float32)(999. * sin(angle));
        numaAddNumber(na, val);
    }

    nahisto = numaMakeHistogramClipped(na, 6, 2000);
    nbins = numaGetCount(nahisto);
    nax = numaMakeSequence(0, 1, nbins);
    gplot = gplotCreate("/tmp/historoot1", GPLOT_X11, "example histo 1",
                        "i", "histo[i]");
    gplotAddPlot(gplot, nax, nahisto, GPLOT_LINES, "sine");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&nax);
    numaDestroy(&nahisto);

    nahisto = numaMakeHistogram(na, 1000, &binsize, &binstart);
    nbins = numaGetCount(nahisto);
    nax = numaMakeSequence(binstart, binsize, nbins);
    fprintf(stderr, " binsize = %d, binstart = %d\n", binsize, binstart);
    gplot = gplotCreate("/tmp/historoot2", GPLOT_X11, "example histo 2",
                        "i", "histo[i]");
    gplotAddPlot(gplot, nax, nahisto, GPLOT_LINES, "sine");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&nax);
    numaDestroy(&nahisto);

    nahisto = numaMakeHistogram(na, 1000, &binsize, NULL);
    nbins = numaGetCount(nahisto);
    nax = numaMakeSequence(0, binsize, nbins);
    fprintf(stderr, " binsize = %d, binstart = %d\n", binsize, 0);
    gplot = gplotCreate("/tmp/historoot3", GPLOT_X11, "example histo 3",
                        "i", "histo[i]");
    gplotAddPlot(gplot, nax, nahisto, GPLOT_LINES, "sine");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&nax);
    numaDestroy(&nahisto);

    nahisto = numaMakeHistogramAuto(na, 1000);
    nbins = numaGetCount(nahisto);
    numaGetParameters(nahisto, &startval, &fbinsize);
    nax = numaMakeSequence(startval, fbinsize, nbins);
    fprintf(stderr, " binsize = %7.4f, binstart = %8.3f\n",
            fbinsize, startval);
    gplot = gplotCreate("/tmp/historoot4", GPLOT_X11, "example histo 4",
                        "i", "histo[i]");
    gplotAddPlot(gplot, nax, nahisto, GPLOT_LINES, "sine");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&nax);
    numaDestroy(&nahisto);

    numaGetStatsUsingHistogram(na, 2000, &minval, &maxval, &meanval,
                               &variance, &median, 0.80, &rankval, &nahisto);
    fprintf(stderr, "Sin histogram: \n"
                    "  min val  = %7.2f    -- should be -999.00\n"
                    "  max val  = %7.2f    -- should be  999.00\n"
                    "  mean val = %7.2f    -- should be    0.06\n"
                    "  median   = %7.2f    -- should be    0.30\n"
                    "  rmsdev   = %7.2f    -- should be  706.41\n"
                    "  rank val = %7.2f    -- should be  808.15\n",
            minval, maxval, meanval, median, sqrt((l_float64)variance),
            rankval);
    numaHistogramGetRankFromVal(nahisto, 808.15, &rank);
    fprintf(stderr, "  rank     = %7.3f    -- should be  0.800\n", rank);
    numaDestroy(&nahisto);
    numaDestroy(&na);
#endif

    /* -------------------------------------------------------------------*
     *                            Interpolation                           *
     * -------------------------------------------------------------------*/
#if  DO_ALL
        /* Test numaInterpolateEqxInterval() */
    pixs = pixRead("test8.jpg");
    na = pixGetGrayHistogramMasked(pixs, NULL, 0, 0, 1);
/*    numaWriteStream(stderr, na); */
    nasy = numaGetPartialSums(na);
    gplotSimple1(nasy, GPLOT_X11, "/tmp/introot1", "partial sums");
    gplotSimple1(na, GPLOT_X11, "/tmp/introot2", "simple test");
    numaInterpolateEqxInterval(0.0, 1.0, na, L_LINEAR_INTERP,
                               0.0, 255.0, 15, &nax, &nay);
    gplot = gplotCreate("/tmp/introot3", GPLOT_X11, "test interpolation",
                        "pix val", "num pix");
    gplotAddPlot(gplot, nax, nay, GPLOT_LINES, "plot 1");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&na);
    numaDestroy(&nasy);
    numaDestroy(&nax);
    numaDestroy(&nay);
    pixDestroy(&pixs);
#endif

#if  DO_ALL
        /* Test numaInterpolateArbxInterval() */
    pixs = pixRead("test8.jpg");
    na = pixGetGrayHistogramMasked(pixs, NULL, 0, 0, 1);
    nasy = numaGetPartialSums(na);
    numaInsertNumber(nasy, 0, 0.0);
    nasx = numaMakeSequence(0.0, 1.0, 257);
/*    gplotSimple1(nasy, GPLOT_X11, "/tmp/nasyroot", "partial sums"); */
    numaInterpolateArbxInterval(nasx, nasy, L_LINEAR_INTERP,
                                10.0, 250.0, 23, &nax, &nay);
    gplot = gplotCreate("/tmp/introot4", GPLOT_X11, "arbx interpolation",
                        "pix val", "cum num pix");
    gplotAddPlot(gplot, nax, nay, GPLOT_LINES, "plot 1");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&na);
    numaDestroy(&nasx);
    numaDestroy(&nasy);
    numaDestroy(&nax);
    numaDestroy(&nay);
    pixDestroy(&pixs);
#endif

#if  DO_ALL
        /* Test numaInterpolateArbxVal() */
    pixs = pixRead("test8.jpg");
    na = pixGetGrayHistogramMasked(pixs, NULL, 0, 0, 1);
    nasy = numaGetPartialSums(na);
    numaInsertNumber(nasy, 0, 0.0);
    nasx = numaMakeSequence(0.0, 1.0, 257);
/*    gplotSimple1(nasy, GPLOT_X11, "/tmp/nasyroot", "partial sums"); */
    nax = numaMakeSequence(15.0, (250.0 - 15.0) / 23.0, 24);
    n = numaGetCount(nax);
    nay = numaCreate(n);
    for (i = 0; i < n; i++) {
        numaGetFValue(nax, i, &xval);
        numaInterpolateArbxVal(nasx, nasy, L_QUADRATIC_INTERP, xval, &yval);
        numaAddNumber(nay, yval);
    }
    gplot = gplotCreate("/tmp/introot5", GPLOT_X11, "arbx interpolation",
                        "pix val", "cum num pix");
    gplotAddPlot(gplot, nax, nay, GPLOT_LINES, "plot 1");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&na);
    numaDestroy(&nasx);
    numaDestroy(&nasy);
    numaDestroy(&nax);
    numaDestroy(&nay);
    pixDestroy(&pixs);
#endif

#if  DO_ALL
        /* Test interpolation */
    nasx = numaRead("testangle.na");
    nasy = numaRead("testscore.na");
    gplot = gplotCreate("/tmp/introot6", GPLOT_X11, "arbx interpolation",
                        "angle", "score");
    numaInterpolateArbxInterval(nasx, nasy, L_LINEAR_INTERP,
                                -2.00, 0.0, 50, &nax, &nay);
    gplotAddPlot(gplot, nax, nay, GPLOT_LINES, "linear");
    numaDestroy(&nax);
    numaDestroy(&nay);
    numaInterpolateArbxInterval(nasx, nasy, L_QUADRATIC_INTERP,
                                -2.00, 0.0, 50, &nax, &nay);
    gplotAddPlot(gplot, nax, nay, GPLOT_LINES, "quadratic");
    numaDestroy(&nax);
    numaDestroy(&nay);
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    gplot = gplotCreate("/tmp/introot7", GPLOT_X11, "arbx interpolation",
                        "angle", "score");
    numaInterpolateArbxInterval(nasx, nasy, L_LINEAR_INTERP,
                                -1.2, -0.8, 50, &nax, &nay);
    gplotAddPlot(gplot, nax, nay, GPLOT_LINES, "quadratic");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaFitMax(nay, &yval, nax, &xval);
    fprintf(stderr, "max = %f at loc = %f\n", yval, xval);
    numaDestroy(&nasx);
    numaDestroy(&nasy);
    numaDestroy(&nax);
    numaDestroy(&nay);
#endif

    /* -------------------------------------------------------------------*
     *                   Integration and differentiation                  *
     * -------------------------------------------------------------------*/
#if  DO_ALL
        /* Test integration and differentiation */
    nasx = numaRead("testangle.na");
    nasy = numaRead("testscore.na");
        /* ---------- Plot the derivative ---------- */
    numaDifferentiateInterval(nasx, nasy, -2.0, 0.0, 50, &nadx, &nady);
    gplot = gplotCreate("/tmp/diffroot1", GPLOT_X11, "derivative",
                        "angle", "slope");
    gplotAddPlot(gplot, nadx, nady, GPLOT_LINES, "derivative");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
        /*  ---------- Plot the original function ----------- */
        /*  and the integral of the derivative; the two       */
        /*  should be approximately the same.                 */
    gplot = gplotCreate("/tmp/diffroot2", GPLOT_X11, "integ-diff",
                        "angle", "val");
    numaInterpolateArbxInterval(nasx, nasy, L_LINEAR_INTERP,
                                -2.00, 0.0, 50, &nafx, &nafy);
    gplotAddPlot(gplot, nafx, nafy, GPLOT_LINES, "function");
    n = numaGetCount(nadx);
    numaGetFValue(nafx, 0, &x0);
    numaGetFValue(nafy, 0, &y0);
    nay = numaCreate(n);
        /* (Note: this tests robustness of the integrator: we go from
         * i = 0, and choose to have only 1 point in the interpolation
         * there, which is too small and causes the function to bomb out.) */
    for (i = 0; i < n; i++) {
        numaGetFValue(nadx, i, &xval);
        numaIntegrateInterval(nadx, nady, x0, xval, 2 * i + 1, &yval);
        numaAddNumber(nay, y0 + yval);
    }
    fprintf(stderr, "It's required to get a 'npts < 2' error here!\n");
    gplotAddPlot(gplot, nafx, nay, GPLOT_LINES, "anti-derivative");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&nasx);
    numaDestroy(&nasy);
    numaDestroy(&nafx);
    numaDestroy(&nafy);
    numaDestroy(&nadx);
    numaDestroy(&nady);
    numaDestroy(&nay);
#endif

    /* -------------------------------------------------------------------*
     *                             Rank extraction                        *
     * -------------------------------------------------------------------*/
#if  DO_ALL
        /* Rank extraction with interpolation */
    pixs = pixRead("test8.jpg");
    nasy= pixGetGrayHistogramMasked(pixs, NULL, 0, 0, 1);
    numaMakeRankFromHistogram(0.0, 1.0, nasy, 350, &nax, &nay);
    gplot = gplotCreate("/tmp/rankroot1", GPLOT_X11, "test rank extractor",
                        "pix val", "rank val");
    gplotAddPlot(gplot, nax, nay, GPLOT_LINES, "plot 1");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&nasy);
    numaDestroy(&nax);
    numaDestroy(&nay);
    pixDestroy(&pixs);
#endif

#if  DO_ALL
        /* Rank extraction, point by point */
    pixs = pixRead("test8.jpg");
    nap = numaCreate(200);
    pixGetRankValueMasked(pixs, NULL, 0, 0, 2, 0.0, &val, &na);
    for (i = 0; i < 101; i++) {
      rank = 0.01 * i;
      numaHistogramGetValFromRank(na, rank, &val);
      numaAddNumber(nap, val);
    }
    gplotSimple1(nap, GPLOT_X11, "/tmp/rankroot2", "rank value");
    numaDestroy(&na);
    numaDestroy(&nap);
    pixDestroy(&pixs);
#endif

    /* -------------------------------------------------------------------*
     *                           Numa-morphology                          *
     * -------------------------------------------------------------------*/
#if  DO_ALL
    na = numaRead("lyra-5.numa");
    gplotSimple1(na, GPLOT_PNG, "/tmp/lyraroot1", "Original");
    na1 = numaErode(na, 21);
    gplotSimple1(na1, GPLOT_PNG, "/tmp/lyraroot2", "Erosion");
    na2 = numaDilate(na, 21);
    gplotSimple1(na2, GPLOT_PNG, "/tmp/lyraroot3", "Dilation");
    na3 = numaOpen(na, 21);
    gplotSimple1(na3, GPLOT_PNG, "/tmp/lyraroot4", "Opening");
    na4 = numaClose(na, 21);
    gplotSimple1(na4, GPLOT_PNG, "/tmp/lyraroot5", "Closing");
#ifndef  _WIN32
    sleep(1);
#else
    Sleep(1000);
#endif  /* _WIN32 */
    pixa = pixaCreate(5);
    pix1 = pixRead("/tmp/lyraroot1.png");
    pix2 = pixRead("/tmp/lyraroot2.png");
    pix3 = pixRead("/tmp/lyraroot3.png");
    pix4 = pixRead("/tmp/lyraroot4.png");
    pix5 = pixRead("/tmp/lyraroot5.png");
    pixSaveTiled(pix1, pixa, 1.0, 1, 25, 32);
    pixSaveTiled(pix2, pixa, 1.0, 1, 25, 32);
    pixSaveTiled(pix3, pixa, 1.0, 0, 25, 32);
    pixSaveTiled(pix4, pixa, 1.0, 1, 25, 32);
    pixSaveTiled(pix5, pixa, 1.0, 0, 25, 32);
    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/numamorph.png", pixd, IFF_PNG);
    numaDestroy(&na);
    numaDestroy(&na1);
    numaDestroy(&na2);
    numaDestroy(&na3);
    numaDestroy(&na4);
    pixaDestroy(&pixa);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    pixDestroy(&pix4);
    pixDestroy(&pix5);
    pixDestroy(&pixd);
#endif

    return 0;
}
