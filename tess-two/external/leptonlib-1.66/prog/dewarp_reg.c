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
 *   dewarp_reg.c
 *
 *     Regression test for image dewarp based on text lines
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


l_int32 main(int    argc,
             char **argv)
{
l_int32    i, n, index, display, success;
l_float32  a, b, c;
L_DEWARP  *dew;
FILE      *fp;
NUMA      *nax, *nafit;
PIX       *pixs, *pixn, *pixg, *pixb, *pixt1, *pixt2;
PTA       *pta, *ptad;
PTAA      *ptaa1, *ptaa2;

    if (regTestSetup(argc, argv, &fp, &display, &success, NULL))
              return 1;

    pixs = pixRead("1555-7.jpg");
    
        /* Normalize for varying background and binarize */
    pixn = pixBackgroundNormSimple(pixs, NULL, NULL);
    pixg = pixConvertRGBToGray(pixn, 0.5, 0.3, 0.2);
    pixb = pixThresholdToBinary(pixg, 130);
    pixDestroy(&pixn);
    pixDestroy(&pixg);
    pixWrite("/tmp/dewarp.0.png", pixb, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/dewarp.0.png", 0, &success);
    pixDisplayWithTitle(pixb, 0, 0, "binarized input", display);

        /* Get the textline centers */
    ptaa1 = pixGetTextlineCenters(pixb, 0);
    pixt1 = pixCreateTemplate(pixs);
    pixt2 = pixDisplayPtaa(pixt1, ptaa1);
    pixWrite("/tmp/dewarp.1.png", pixt2, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/dewarp.1.png", 1, &success);
    pixDisplayWithTitle(pixt2, 0, 500, "textline centers", display);
    pixDestroy(&pixt1);

        /* Remove short lines */
    ptaa2 = ptaaRemoveShortLines(pixb, ptaa1, 0.8, 0);

        /* Fit to quadratic */
    n = ptaaGetCount(ptaa2);
    for (i = 0; i < n; i++) {
        pta = ptaaGetPta(ptaa2, i, L_CLONE);
        ptaGetArrays(pta, &nax, NULL);
        ptaGetQuadraticLSF(pta, &a, &b, &c, &nafit);
        ptad = ptaCreateFromNuma(nax, nafit);
        pixDisplayPta(pixt2, pixt2, ptad);
        ptaDestroy(&pta);
        ptaDestroy(&ptad);
        numaDestroy(&nax);
        numaDestroy(&nafit);
    }
    pixWrite("/tmp/dewarp.2.png", pixt2, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/dewarp.2.png", 2, &success);
    pixDisplayWithTitle(pixt2, 300, 500, "fitted lines superimposed", display);
    ptaaDestroy(&ptaa1);
    ptaaDestroy(&ptaa2);
    pixDestroy(&pixt2);

        /* Run with only vertical disparity correction */
    dew = dewarpCreate(pixb, 30, 15, 0);
    dewarpBuildModel(dew, 0);
    dewarpApplyDisparity(dew, pixb, 0);
    pixWrite("/tmp/dewarp.3.png", dew->pixd, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/dewarp.3.png", 3, &success);
    pixDisplayWithTitle(dew->pixd, 400, 0, "fixed for vert disparity",
                        display);
    dewarpDestroy(&dew);

        /* Run with both vertical and horizontal disparity correction */
    dew = dewarpCreate(pixb, 30, 15, 1);
    dewarpBuildModel(dew, 0);
    dewarpApplyDisparity(dew, pixb, 0);
    pixWrite("/tmp/dewarp.4.png", dew->pixd, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/dewarp.4.png", 4, &success);
    pixDisplayWithTitle(dew->pixd, 800, 0, "fixed for both disparities",
                        display);
    dewarpDestroy(&dew);

    pixDestroy(&pixs);
    pixDestroy(&pixb);
    regTestCleanup(argc, argv, fp, success, NULL);
    return 0;
}


