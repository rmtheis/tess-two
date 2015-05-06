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
 * skew_reg.c
 *
 *     Regression test for skew detection.
 */

#include "allheaders.h"

    /* deskew */
#define   DESKEW_REDUCTION      4      /* 1, 2 or 4 */

    /* sweep only */
#define   SWEEP_RANGE           5.     /* degrees */
#define   SWEEP_DELTA           0.2    /* degrees */
#define   SWEEP_REDUCTION       2      /* 1, 2, 4 or 8 */

    /* sweep and search */
#define   SWEEP_RANGE2          5.     /* degrees */
#define   SWEEP_DELTA2          1.     /* degrees */
#define   SWEEP_REDUCTION2      2      /* 1, 2, 4 or 8 */
#define   SEARCH_REDUCTION      2      /* 1, 2, 4 or 8 */
#define   SEARCH_MIN_DELTA      0.01   /* degrees */

static const l_int32  BORDER = 150;


int main(int    argc,
         char **argv)
{
l_int32       w, h, wd, hd;
l_float32     deg2rad, angle, conf;
PIX          *pixs, *pixb1, *pixb2, *pixr, *pixf, *pixd, *pixc;
PIXA         *pixa;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

    deg2rad = 3.1415926535 / 180.;

    pixa = pixaCreate(0);
    pixs = pixRead("feyn.tif");
    pixSetOrClearBorder(pixs, 100, 250, 100, 0, PIX_CLR);
    pixb1 = pixReduceRankBinaryCascade(pixs, 2, 2, 0, 0);
    regTestWritePixAndCheck(rp, pixb1, IFF_PNG);  /* 0 */
    pixDisplayWithTitle(pixb1, 0, 100, NULL, rp->display);

        /* Add a border and locate and deskew a 40 degree rotation */
    pixb2 = pixAddBorder(pixb1, BORDER, 0);
    pixGetDimensions(pixb2, &w, &h, NULL);
    pixSaveTiled(pixb2, pixa, 0.5, 1, 20, 8);
    pixr = pixRotateBySampling(pixb2, w / 2, h / 2,
                                    deg2rad * 40., L_BRING_IN_WHITE);
    regTestWritePixAndCheck(rp, pixr, IFF_PNG);  /* 1 */
    pixSaveTiled(pixr, pixa, 0.5, 0, 20, 0);
    pixFindSkewSweepAndSearchScorePivot(pixr, &angle, &conf, NULL, 1, 1,
                                        0.0, 45.0, 2.0, 0.03,
                                        L_SHEAR_ABOUT_CENTER);
    fprintf(stderr, "Should be 40 degrees: angle = %7.3f, conf = %7.3f\n",
            angle, conf);
    pixf = pixRotateBySampling(pixr, w / 2, h / 2,
                                    deg2rad * angle, L_BRING_IN_WHITE);
    pixd = pixRemoveBorder(pixf, BORDER);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);  /* 2 */
    pixSaveTiled(pixd, pixa, 0.5, 0, 20, 0);
    pixDestroy(&pixr);
    pixDestroy(&pixf);
    pixDestroy(&pixd);

        /* Do a rotation larger than 90 degrees using embedding;
         * Use 2 sets of measurements at 90 degrees to scan the
         * full range of possible rotation angles. */
    pixGetDimensions(pixb1, &w, &h, NULL);
    pixr = pixRotate(pixb1, deg2rad * 37., L_ROTATE_SAMPLING,
                     L_BRING_IN_WHITE, w, h);
    regTestWritePixAndCheck(rp, pixr, IFF_PNG);  /* 3 */
    pixSaveTiled(pixr, pixa, 0.5, 1, 20, 0);
    startTimer();
    pixFindSkewOrthogonalRange(pixr, &angle, &conf, 2, 1,
                               47.0, 1.0, 0.03, 0.0);
    fprintf(stderr, "Orth search time: %7.3f sec\n", stopTimer());
    fprintf(stderr, "Should be about -128 degrees: angle = %7.3f\n", angle);
    pixd = pixRotate(pixr, deg2rad * angle, L_ROTATE_SAMPLING,
                     L_BRING_IN_WHITE, w, h);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);  /* 4 */
    pixGetDimensions(pixd, &wd, &hd, NULL);
    pixc = pixCreate(w, h, 1);
    pixRasterop(pixc, 0, 0, w, h, PIX_SRC, pixd, (wd - w) / 2, (hd - h) / 2);
    regTestWritePixAndCheck(rp, pixc, IFF_PNG);  /* 5 */
    pixSaveTiled(pixc, pixa, 0.5, 0, 20, 0);
    pixDestroy(&pixr);
    pixDestroy(&pixf);
    pixDestroy(&pixd);
    pixDestroy(&pixc);

    pixd = pixaDisplay(pixa, 0, 0);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);  /* 6 */
    pixDisplayWithTitle(pixd, 100, 100, NULL, rp->display);
    pixDestroy(&pixd);

    pixDestroy(&pixs);
    pixDestroy(&pixb1);
    pixDestroy(&pixb2);
    pixaDestroy(&pixa);
    return regTestCleanup(rp);
}

#if 0
    pixFindSkewSweepAndSearchScore(pixs, &angle, &conf, &endscore,
                                   4, 2, 0.0, 5.0, 1.0, 0.01);
    fprintf(stderr, "angle = %8.4f, conf = %8.4f, endscore = %f\n",
            angle, conf, endscore);
    startTimer();
    pixd = pixDeskew(pixs, DESKEW_REDUCTION);
    fprintf(stderr, "Time to deskew = %7.4f sec\n", stopTimer());
    pixWrite(fileout, pixd, IFF_BMP);
    pixDestroy(&pixd);
#endif


#if 0
    if (pixFindSkew(pixs, &angle)) {
        L_WARNING("skew angle not valid\n", mainName);
        return 1;
    }
#endif

#if 0
    if (pixFindSkewSweep(pixs, &angle, SWEEP_REDUCTION,
                         SWEEP_RANGE, SWEEP_DELTA)) {
        L_WARNING("skew angle not valid\n", mainName);
        return 1;
    }
#endif

#if 0
    if (pixFindSkewSweepAndSearch(pixs, &angle, SWEEP_REDUCTION2,
                         SEARCH_REDUCTION, SWEEP_RANGE2, SWEEP_DELTA2,
                         SEARCH_MIN_DELTA)) {
        L_WARNING("skew angle not valid\n", mainName);
        return 1;
    }
#endif
