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
 *  compare_reg.c
 *
 *   This tests comparison of images that are translated with
 *   respect to each other.
 */

#include "string.h"
#include "allheaders.h"


l_int32 main(int    argc,
             char **argv)
{
char          buf[512];
l_int32       delx, dely, etransx, etransy, w, h, area1, area2;
l_int32      *stab, *ctab;
l_float32     cx1, cy1, cx2, cy2, score;
PIX          *pix0, *pix1, *pix2;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;


    /* ------------ Test of pixBestCorrelation() --------------- */
    pix0 = pixRead("harmoniam100-11.png");
    pix1 = pixConvertTo1(pix0, 160);
    pixGetDimensions(pix1, &w, &h, NULL);

        /* Now make a smaller image, translated by (-32, -12)
         * Except for the resizing, this is equivalent to
         *     pix2 = pixTranslate(NULL, pix1, -32, -12, L_BRING_IN_WHITE);  */
    pix2 = pixCreate(w - 10, h, 1);
    pixRasterop(pix2, 0, 0, w, h, PIX_SRC, pix1, 32, 12);

        /* Get the number of FG pixels and the centroid locations */
    stab = makePixelSumTab8();
    ctab = makePixelCentroidTab8();
    pixCountPixels(pix1, &area1, stab);
    pixCountPixels(pix2, &area2, stab);
    pixCentroid(pix1, ctab, stab, &cx1, &cy1);
    pixCentroid(pix2, ctab, stab, &cx2, &cy2);
    etransx = lept_roundftoi(cx1 - cx2);
    etransy = lept_roundftoi(cy1 - cy2);
    fprintf(stderr, "delta cx = %d, delta cy = %d\n",
            etransx, etransy);

        /* Get the best correlation, searching around the translation
         * where the centroids coincide */
    pixBestCorrelation(pix1, pix2, area1, area2, etransx, etransy,
                       4, stab, &delx, &dely, &score, 5);
    fprintf(stderr, "delx = %d, dely = %d, score = %7.4f\n",
            delx, dely, score);
    regTestCompareValues(rp, 32, delx, 0);   /* 0 */
    regTestCompareValues(rp, 12, dely, 0);   /* 1 */
    regTestCheckFile(rp, "/tmp/junkcorrel_5.png");   /* 2 */
    lept_rm(NULL, "junkcorrel_5.png");
    FREE(stab);
    FREE(ctab);
    pixDestroy(&pix0);
    pixDestroy(&pix1);
    pixDestroy(&pix2);


    /* ------------ Test of pixCompareWithTranslation() ------------ */
        /* Now use the pyramid to get the result.  Do a translation
         * to remove pixels at the bottom from pix2, so that the
         * centroids are initially far apart. */
    pix1 = pixRead("harmoniam-11.tif");
    pix2 = pixTranslate(NULL, pix1, -45, 25, L_BRING_IN_WHITE);
    l_pdfSetDateAndVersion(0);
    pixCompareWithTranslation(pix1, pix2, 160, &delx, &dely, &score, 1);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    fprintf(stderr, "delx = %d, dely = %d\n", delx, dely);
    regTestCompareValues(rp, 45, delx, 0);   /* 3 */
    regTestCompareValues(rp, -25, dely, 0);   /* 4 */
    regTestCheckFile(rp, "/tmp/junkcmp.pdf");   /* 5 */
    regTestCheckFile(rp, "/tmp/junkcorrel.pdf");  /* 6 */

    return regTestCleanup(rp);
}
