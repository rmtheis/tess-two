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
 * rankbin_reg.c
 *
 *   Tests rank bin functions:
 *      (1) numaDiscretizeRankAndIntensity()
 *      (2) numaGetRankBinValues()
 */

#ifndef  _WIN32
#include <unistd.h>
#else
#include <windows.h>   /* for Sleep() */
#endif  /* _WIN32 */

#include "allheaders.h"


main(int    argc,
     char **argv)
{
l_int32       i, n, w, h;
BOXA         *boxa;
NUMA         *naindex, *naw, *nah, *naw_med, *nah_med;
PIX          *pixs, *pixt, *pixd;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

        /* Generate arrays of word widths and heights */
    pixs = pixRead("feyn.tif");
    pixGetWordBoxesInTextlines(pixs, 1, 6, 6, 500, 50, &boxa, &naindex);
    n = boxaGetCount(boxa);
    naw = numaCreate(0);
    nah = numaCreate(0);
    for (i = 0; i < n; i++) {
        boxaGetBoxGeometry(boxa, i, NULL, NULL, &w, &h);
        numaAddNumber(naw, w);
        numaAddNumber(nah, h);
    }
    boxaDestroy(&boxa);
    numaDestroy(&naindex);

        /* Make the rank bin arrays of median values, with 10 bins */
    numaGetRankBinValues(naw, 10, NULL, &naw_med);
    numaGetRankBinValues(nah, 10, NULL, &nah_med);
    gplotSimple1(naw_med, GPLOT_PNG, "/tmp/w_10bin", 
                 "width vs rank bins (10)");
    gplotSimple1(nah_med, GPLOT_PNG, "/tmp/h_10bin", 
                 "height vs rank bins (10)");
    numaDestroy(&naw_med);
    numaDestroy(&nah_med);

        /* Make the rank bin arrays of median values, with 30 bins */
    numaGetRankBinValues(naw, 30, NULL, &naw_med);
    numaGetRankBinValues(nah, 30, NULL, &nah_med);
    gplotSimple1(naw_med, GPLOT_PNG, "/tmp/w_30bin", 
                 "width vs rank bins (30)");
    gplotSimple1(nah_med, GPLOT_PNG, "/tmp/h_30bin", 
                 "height vs rank bins (30)");
    numaDestroy(&naw_med);
    numaDestroy(&nah_med);

        /* Give gnuplot time to write out the files */
#ifndef  _WIN32
    sleep(2);
#else
    Sleep(2000);
#endif  /* _WIN32 */

        /* Save as golden files, or check against them */
    regTestCheckFile(rp, "/tmp/w_10bin.png");  /* 0 */
    regTestCheckFile(rp, "/tmp/h_10bin.png");  /* 1 */
    regTestCheckFile(rp, "/tmp/w_30bin.png");  /* 2 */
    regTestCheckFile(rp, "/tmp/h_30bin.png");  /* 3 */

        /* Display results for debugging */
    pixt = pixRead("/tmp/w_10bin.png");
    pixDisplayWithTitle(pixt, 0, 0, NULL, rp->display);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/h_10bin.png");
    pixDisplayWithTitle(pixt, 650, 0, NULL, rp->display);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/w_30bin.png");
    pixDisplayWithTitle(pixt, 0, 550, NULL, rp->display);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/h_30bin.png");
    pixDisplayWithTitle(pixt, 650, 550, NULL, rp->display);
    pixDestroy(&pixt);

    pixDestroy(&pixs);
    numaDestroy(&naw);
    numaDestroy(&nah);
    return regTestCleanup(rp);
}
