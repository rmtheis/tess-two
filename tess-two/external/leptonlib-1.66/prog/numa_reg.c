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
 * numa_reg.c
 *
 *   Tests:
 *     * histograms
 *     * interpolation
 *     * integration/differentiation
 *     * rank extraction
 *     * numa-morphology
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include "allheaders.h"

#define   DO_ALL     1


main(int    argc,
     char **argv)
{
l_int32      i, n, binsize, binstart, nbins, size;
l_float32    pi, val, angle, xval, yval, x0, y0, rank, startval, fbinsize;
l_float32    minval, maxval, meanval, median, variance, rankval;
GPLOT       *gplot;
NUMA        *na, *nahisto, *nax, *nay, *nap, *nasx, *nasy;
NUMA        *nadx, *nady, *nafx, *nafy, *nae, *nad, *nao, *nac;
PIX         *pixs, *pixt1, *pixt2, *pixt3, *pixt4, *pixt5, *pixd;
PIXA        *pixa;
static char  mainName[] = "numa_reg";

    if (argc != 1)
	exit(ERROR_INT(" Syntax:  numa_reg", mainName, 1));

    pi = 3.1415926535;
    na = numaCreate(5000);
    for (i = 0; i < 500000; i++) {
        angle = 0.02293 * i * pi;
        val = (l_float32)(999. * sin(angle));
        numaAddNumber(na, val);
    }

    /* -------------------------------------------------------------------*
     *                              Histograms                            *
     * -------------------------------------------------------------------*/
    nahisto = numaMakeHistogramClipped(na, 6, 2000);
    nbins = numaGetCount(nahisto);
    nax = numaMakeSequence(0, 1, nbins);
    gplot = gplotCreate("/tmp/junkroot1", GPLOT_X11, "example histo 1",
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
    gplot = gplotCreate("/tmp/junkroot2", GPLOT_X11, "example histo 2",
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
    gplot = gplotCreate("/tmp/junkroot3", GPLOT_X11, "example histo 3",
                        "i", "histo[i]");
    gplotAddPlot(gplot, nax, nahisto, GPLOT_LINES, "sine");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&nax);
    numaDestroy(&nahisto);

    nahisto = numaMakeHistogramAuto(na, 1000);
    nbins = numaGetCount(nahisto);
    numaGetXParameters(nahisto, &startval, &fbinsize);
    nax = numaMakeSequence(startval, fbinsize, nbins);
    fprintf(stderr, " binsize = %7.4f, binstart = %8.3f\n",
            fbinsize, startval);
    gplot = gplotCreate("/tmp/junkroot4", GPLOT_X11, "example histo 4",
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

    /* -------------------------------------------------------------------*
     *                            Interpolation                           *
     * -------------------------------------------------------------------*/
#if  DO_ALL
        /* Test numaInterpolateEqxInterval() */
    pixs = pixRead("test8.jpg");
    na = pixGetGrayHistogramMasked(pixs, NULL, 0, 0, 1);
/*    numaWriteStream(stderr, na); */
    nasy = numaGetPartialSums(na);
    gplotSimple1(nasy, GPLOT_X11, "/tmp/junkroot5", "partial sums");
    gplotSimple1(na, GPLOT_X11, "/tmp/junkroot6", "simple test");
    numaInterpolateEqxInterval(0.0, 1.0, na, L_LINEAR_INTERP,
		            0.0, 255.0, 15, &nax, &nay);
    gplot = gplotCreate("/tmp/junkroot7", GPLOT_X11, "test interpolation",
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
/*    gplotSimple1(nasy, GPLOT_X11, "/tmp/junknasy", "partial sums"); */
    numaInterpolateArbxInterval(nasx, nasy, L_LINEAR_INTERP,
		            10.0, 250.0, 23, &nax, &nay);
    gplot = gplotCreate("/tmp/junkroot8", GPLOT_X11, "arbx interpolation",
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
/*    gplotSimple1(nasy, GPLOT_X11, "/tmp/junknasy", "partial sums"); */
    nax = numaMakeSequence(15.0, (250.0 - 15.0) / 23.0, 24);
    n = numaGetCount(nax);
    nay = numaCreate(n);
    for (i = 0; i < n; i++) {
        numaGetFValue(nax, i, &xval);
        numaInterpolateArbxVal(nasx, nasy, L_QUADRATIC_INTERP, xval, &yval);
	numaAddNumber(nay, yval);
    }
    gplot = gplotCreate("/tmp/junkroot9", GPLOT_X11, "arbx interpolation",
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
    nasx = numaRead("testangle.numa");
    nasy = numaRead("testscore.numa");
    gplot = gplotCreate("/tmp/junkroot10", GPLOT_X11, "arbx interpolation",
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
    gplot = gplotCreate("/tmp/junkroot11", GPLOT_X11, "arbx interpolation",
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
    nasx = numaRead("testangle.numa");
    nasy = numaRead("testscore.numa");
        /* ---------- Plot the derivative ---------- */
    numaDifferentiateInterval(nasx, nasy, -2.0, 0.0, 50, &nadx, &nady);
    gplot = gplotCreate("/tmp/junkroot12", GPLOT_X11, "derivative",
		    "angle", "slope");
    gplotAddPlot(gplot, nadx, nady, GPLOT_LINES, "derivative");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
        /*  ---------- Plot the original function ----------- */
        /*  and the integral of the derivative; the two       */
        /*  should be approximately the same.                 */
    gplot = gplotCreate("/tmp/junkroot13", GPLOT_X11, "integ-diff",
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
    gplot = gplotCreate("/tmp/junkroot14", GPLOT_X11, "test rank extractor",
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
    gplotSimple1(nap, GPLOT_X11, "/tmp/junkroot15", "rank value");
    numaDestroy(&na);
    numaDestroy(&nap);
    pixDestroy(&pixs);
#endif

    /* -------------------------------------------------------------------*
     *                             Numa-morphology                        *
     * -------------------------------------------------------------------*/
    na = numaRead("lyra-5.numa");
    gplotSimple1(na, GPLOT_PNG, "/tmp/junkroot1", "Original");
    nae = numaErode(na, 21);
    gplotSimple1(nae, GPLOT_PNG, "/tmp/junkroot2", "Erosion");
    nad = numaDilate(na, 21);
    gplotSimple1(nad, GPLOT_PNG, "/tmp/junkroot3", "Dilation");
    nao = numaOpen(na, 21);
    gplotSimple1(nao, GPLOT_PNG, "/tmp/junkroot4", "Opening");
    nac = numaClose(na, 21);
    gplotSimple1(nac, GPLOT_PNG, "/tmp/junkroot5", "Closing");
    sleep(1);
    pixa = pixaCreate(5);
    pixt1 = pixRead("/tmp/junkroot1.png");
    pixt2 = pixRead("/tmp/junkroot2.png");
    pixt3 = pixRead("/tmp/junkroot3.png");
    pixt4 = pixRead("/tmp/junkroot4.png");
    pixt5 = pixRead("/tmp/junkroot5.png");
    pixSaveTiled(pixt1, pixa, 1, 1, 25, 32);
    pixSaveTiled(pixt2, pixa, 1, 1, 25, 32);
    pixSaveTiled(pixt3, pixa, 1, 0, 25, 32);
    pixSaveTiled(pixt4, pixa, 1, 1, 25, 32);
    pixSaveTiled(pixt5, pixa, 1, 0, 25, 32);
    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/numamorph2.jpg", pixd, IFF_JFIF_JPEG);
    numaDestroy(&na);
    numaDestroy(&nae);
    numaDestroy(&nad);
    numaDestroy(&nao);
    numaDestroy(&nac);
    pixaDestroy(&pixa);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);
    pixDestroy(&pixd);


    return 0;
}
