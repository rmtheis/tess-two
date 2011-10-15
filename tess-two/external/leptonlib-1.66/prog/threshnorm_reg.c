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
 * threshnorm__reg.c
 *
 *      Regression test for adaptive threshold normalization.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

static void AddTestSet(PIXA *pixa, PIX *pixs,
                       l_int32 filtertype, l_int32 edgethresh,
                       l_int32 smoothx, l_int32 smoothy,
                       l_float32 gamma, l_int32 minval,
                       l_int32 maxval, l_int32 targetthresh);

main(int    argc,
     char **argv)
{
l_int32  display, success;
FILE    *fp;
PIX     *pixs, *pixd;
PIXA    *pixa;

    if (regTestSetup(argc, argv, &fp, &display, &success, NULL))
           return 1;

    if ((pixs = pixRead("stampede2.jpg")) == NULL)
	exit(ERROR_INT("pixs not made", argv[0], 1));
    pixa = pixaCreate(0);
    pixSaveTiled(pixs, pixa, 1, 1, 20, 8);
	    
    AddTestSet(pixa, pixs, L_SOBEL_EDGE, 18, 40, 40, 0.7, -25, 280, 128);
    AddTestSet(pixa, pixs, L_TWO_SIDED_EDGE, 18, 40, 40, 0.7, -25, 280, 128);
    AddTestSet(pixa, pixs, L_SOBEL_EDGE, 10, 40, 40, 0.7, -15, 305, 128);
    AddTestSet(pixa, pixs, L_TWO_SIDED_EDGE, 10, 40, 40, 0.7, -15, 305, 128);
    AddTestSet(pixa, pixs, L_SOBEL_EDGE, 15, 40, 40, 0.6, -45, 285, 158);
    AddTestSet(pixa, pixs, L_TWO_SIDED_EDGE, 15, 40, 40, 0.6, -45, 285, 158);

    pixDestroy(&pixs);
    pixd = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/thresh.0.jpg", pixd, IFF_JFIF_JPEG);
    regTestCheckFile(fp, argv, "/tmp/thresh.0.jpg", 0, &success);
    pixDisplayWithTitle(pixd, 100, 100, NULL, display);

    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    regTestCleanup(argc, argv, fp, success, NULL);
    return 0;
}


void
AddTestSet(PIXA      *pixa,
           PIX       *pixs,
           l_int32    filtertype,
           l_int32    edgethresh,
           l_int32    smoothx,
           l_int32    smoothy,
           l_float32  gamma,
           l_int32    minval,
           l_int32    maxval,
           l_int32    targetthresh)
{
PIX  *pixb, *pixd, *pixth;

    pixThresholdSpreadNorm(pixs, filtertype, edgethresh,
                           smoothx, smoothy, gamma, minval,
                           maxval, targetthresh, &pixth, NULL, &pixd); 
    pixSaveTiled(pixth, pixa, 1, 1, 20, 0);
    pixSaveTiled(pixd, pixa, 1, 0, 20, 0);
    pixb = pixThresholdToBinary(pixd, targetthresh - 20);
    pixSaveTiled(pixb, pixa, 1, 0, 20, 0);
    pixDestroy(&pixb);
    pixb = pixThresholdToBinary(pixd, targetthresh);
    pixSaveTiled(pixb, pixa, 1, 0, 20, 0);
    pixDestroy(&pixb);
    pixb = pixThresholdToBinary(pixd, targetthresh + 20);
    pixSaveTiled(pixb, pixa, 1, 0, 20, 0);
    pixDestroy(&pixb);
    pixb = pixThresholdToBinary(pixd, targetthresh + 40);
    pixSaveTiled(pixb, pixa, 1, 0, 20, 0);
    pixDestroy(&pixb);
    pixDestroy(&pixth);
    pixDestroy(&pixd);
    return;
}


