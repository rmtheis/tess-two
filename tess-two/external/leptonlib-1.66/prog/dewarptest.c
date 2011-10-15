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
 *   dewarptest.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define   DO_QUAD     1
#define   DO_CUBIC    0
#define   DO_QUARTIC  0

l_int32 main(int    argc,
             char **argv)
{
l_int32    i, n;
l_float32  a, b, c, d, e;
L_DEWARP  *dew;
FILE      *fp;
FPIX      *fpix;
NUMA      *nax, *nay, *nafit;
PIX       *pixs, *pixn, *pixg, *pixb, *pixt1, *pixt2, *pixt3;
PTA       *pta, *ptad;
PTAA      *ptaa1, *ptaa2;


    pixs = pixRead("1555-7.jpg");

        /* Normalize for varying background and binarize */
    pixn = pixBackgroundNormSimple(pixs, NULL, NULL);
    pixg = pixConvertRGBToGray(pixn, 0.5, 0.3, 0.2);
    pixb = pixThresholdToBinary(pixg, 130);

        /* Run the basic functions */
    dew = dewarpCreate(pixb, 30, 15, 1);
    dewarpBuildModel(dew, 1);
    dewarpApplyDisparity(dew, pixg, 1);
    dewarpDestroy(&dew);

        /* Get the textline centers */
    ptaa1 = pixGetTextlineCenters(pixb, 0);
    pixt1 = pixCreateTemplate(pixs);
    pixSetAll(pixt1);
    pixt3 = pixMorphSequence(pixb, "d3.3", 0);
    pixXor(pixt3, pixt3, pixb);
    pixSetMasked(pixt1, pixt3, 0);
    pixDestroy(&pixt3);
    pixt2 = pixDisplayPtaa(pixt1, ptaa1);
    pixWrite("/tmp/textline1.png", pixt2, IFF_PNG);
    pixDisplayWithTitle(pixt2, 500, 100, "textline centers", 1);
    pixDestroy(&pixt1);

        /* Remove short lines */
    fprintf(stderr, "Num all lines = %d\n", ptaaGetCount(ptaa1));
    ptaa2 = ptaaRemoveShortLines(pixb, ptaa1, 0.8, 0);

        /* Fit to curve */
    n = ptaaGetCount(ptaa2);
    fprintf(stderr, "Num long lines = %d\n", n);
    for (i = 0; i < n; i++) {
        pta = ptaaGetPta(ptaa2, i, L_CLONE);
        ptaGetArrays(pta, &nax, NULL);
#if DO_QUAD
        ptaGetQuadraticLSF(pta, &a, &b, &c, &nafit);
/*        fprintf(stderr, "a = %7.3f, b = %7.3f, c = %7.3f\n", a, b, c); */
#elif  DO_CUBIC
        ptaGetCubicLSF(pta, &a, &b, &c, &d, &nafit);
/*        fprintf(stderr, "a = %7.3f, b = %7.3f, c = %7.3f, d = %7.3f\n",
                a, b, c, d);  */
#elif DO_QUARTIC
        ptaGetQuarticLSF(pta, &a, &b, &c, &d, &e, &nafit);
/*        fprintf(stderr,
              "a = %7.3f, b = %7.3f, c = %7.3f, d = %7.3f, e = %7.3f\n",
              a, b, c, d, e); */
#endif
        ptad = ptaCreateFromNuma(nax, nafit);
        pixDisplayPta(pixt2, pixt2, ptad);
        ptaDestroy(&pta);
        ptaDestroy(&ptad);
        numaDestroy(&nax);
        numaDestroy(&nafit);
    }

    pixDisplayWithTitle(pixt2, 700, 100, "fitted lines superimposed", 1);
    pixWrite("/tmp/textline2.png", pixt2, IFF_PNG);
    ptaaDestroy(&ptaa1);
    ptaaDestroy(&ptaa2);
    pixDestroy(&pixt2);

    pixDestroy(&pixs);
    pixDestroy(&pixn);
    pixDestroy(&pixg);
    pixDestroy(&pixb);
    return 0;
}


