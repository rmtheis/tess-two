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
 *
 *     We also test some of the fpix and dpix functions (scaling,
 *     serialization, interconversion)
 */

#include "allheaders.h"

l_int32 main(int    argc,
             char **argv)
{
l_int32       i, n;
l_float32     a, b, c;
L_DEWARP     *dew, *dew2;
DPIX         *dpix1, *dpix2, *dpix3;
FPIX         *fpix1, *fpix2, *fpix3;
NUMA         *nax, *nafit;
PIX          *pixs, *pixn, *pixg, *pixb, *pixt1, *pixt2;
PIX          *pixs2, *pixn2, *pixg2, *pixb2;
PTA          *pta, *ptad;
PTAA         *ptaa1, *ptaa2;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
              return 1;

    pixs = pixRead("1555-7.jpg");
    
        /* Normalize for varying background and binarize */
    pixn = pixBackgroundNormSimple(pixs, NULL, NULL);
    pixg = pixConvertRGBToGray(pixn, 0.5, 0.3, 0.2);
    pixb = pixThresholdToBinary(pixg, 130);
    pixDestroy(&pixn);
    pixDestroy(&pixg);
    regTestWritePixAndCheck(rp, pixb, IFF_PNG);  /* 0 */
    pixDisplayWithTitle(pixb, 0, 0, "binarized input", rp->display);

        /* Get the textline centers */
    ptaa1 = pixGetTextlineCenters(pixb, 0);
    pixt1 = pixCreateTemplate(pixs);
    pixt2 = pixDisplayPtaa(pixt1, ptaa1);
    regTestWritePixAndCheck(rp, pixt2, IFF_PNG);  /* 1 */
    pixDisplayWithTitle(pixt2, 0, 500, "textline centers", rp->display);
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
    regTestWritePixAndCheck(rp, pixt2, IFF_PNG);  /* 2 */
    pixDisplayWithTitle(pixt2, 300, 500, "fitted lines superimposed",
                        rp->display);
    ptaaDestroy(&ptaa1);
    ptaaDestroy(&ptaa2);
    pixDestroy(&pixt2);

        /* Run with only vertical disparity correction */
    if ((dew = dewarpCreate(pixb, 7, 30, 15, 0)) == NULL)
        return ERROR_INT("\n\n\n FAILURE !!! \n\n\n", rp->testname, 1);
    dewarpBuildModel(dew, 0);
    dewarpApplyDisparity(dew, pixb, 0);
    regTestWritePixAndCheck(rp, dew->pixd, IFF_PNG);  /* 3 */
    pixDisplayWithTitle(dew->pixd, 400, 0, "fixed for vert disparity",
                        rp->display);
    dewarpDestroy(&dew);

        /* Run with both vertical and horizontal disparity correction */
    if ((dew = dewarpCreate(pixb, 7, 30, 15, 1)) == NULL)
        return ERROR_INT("\n\n\n FAILURE !!! \n\n\n", rp->testname, 1);
    dewarpBuildModel(dew, 0);
    dewarpApplyDisparity(dew, pixb, 0);
    regTestWritePixAndCheck(rp, dew->pixd, IFF_PNG);  /* 4 */
    pixDisplayWithTitle(dew->pixd, 800, 0, "fixed for both disparities",
                        rp->display);

        /* Read another image, normalize background and binarize */
    pixs2 = pixRead("1555-3.jpg");
    pixn2 = pixBackgroundNormSimple(pixs2, NULL, NULL);
    pixg2 = pixConvertRGBToGray(pixn2, 0.5, 0.3, 0.2);
    pixb2 = pixThresholdToBinary(pixg2, 130);
    pixDestroy(&pixn2);
    pixDestroy(&pixg2);
    regTestWritePixAndCheck(rp, pixb, IFF_PNG);  /* 5 */
    pixDisplayWithTitle(pixb, 0, 400, "binarized input (2)", rp->display);

        /* Minimize and re-apply previous disparity to this image */
    dewarpMinimize(dew);
    dewarpApplyDisparity(dew, pixb2, 0);
    regTestWritePixAndCheck(rp, dew->pixd, IFF_PNG);  /* 6 */
    pixDisplayWithTitle(dew->pixd, 400, 400, "fixed (2) for both disparities",
                        rp->display);

        /* Write and read back minimized dewarp struct */
    dewarpWrite("/tmp/dewarp.7.dew", dew);
    regTestCheckFile(rp, "/tmp/dewarp.7.dew");  /* 7 */
    dew2 = dewarpRead("/tmp/dewarp.7.dew");
    dewarpWrite("/tmp/dewarp.8.dew", dew2);
    regTestCheckFile(rp, "/tmp/dewarp.8.dew");  /* 8 */
    regTestCompareFiles(rp, 7, 8);  /* 9 */

        /* Apply dew2 to pixb2 */
    dewarpApplyDisparity(dew2, pixb2, 0);
    regTestWritePixAndCheck(rp, dew2->pixd, IFF_PNG);  /* 10 */
    pixDisplayWithTitle(dew->pixd, 800, 400, "fixed (3) for both disparities",
                        rp->display);

        /* Minimize, repopulate disparity arrays, and apply again */
    dewarpMinimize(dew2);
    dewarpApplyDisparity(dew2, pixb2, 0);
    regTestWritePixAndCheck(rp, dew2->pixd, IFF_PNG);  /* 11 */
    regTestCompareFiles(rp, 10, 11);  /* 12 */
    pixDisplayWithTitle(dew->pixd, 900, 400, "fixed (4) for both disparities",
                        rp->display);

        /* Test a few of the fpix functions */
    fpix1 = fpixClone(dew->sampvdispar);
    fpixWrite("/tmp/sampv.13.fpix", fpix1);
    regTestCheckFile(rp, "/tmp/sampv.13.fpix");  /* 13 */
    fpix2 = fpixRead("/tmp/sampv.13.fpix");
    fpixWrite("/tmp/sampv.14.fpix", fpix2);
    regTestCheckFile(rp, "/tmp/sampv.14.fpix");  /* 14 */
    regTestCompareFiles(rp, 13, 14);  /* 15 */
    fpix3 = fpixScaleByInteger(fpix2, 30);
    pixt1 = fpixRenderContours(fpix3, -2., 2.0, 0.2);
    regTestWritePixAndCheck(rp, pixt1, IFF_PNG);  /* 16 */
    pixDisplayWithTitle(pixt1, 0, 800, "v. disparity contours", rp->display);
    fpixDestroy(&fpix1);
    fpixDestroy(&fpix2);
    fpixDestroy(&fpix3);
    pixDestroy(&pixt1);

        /* Test a few of the dpix functions */
    dpix1 = fpixConvertToDPix(dew->sampvdispar);
    dpixWrite("/tmp/sampv.17.dpix", dpix1);
    regTestCheckFile(rp, "/tmp/sampv.17.dpix");  /* 17 */
    dpix2 = dpixRead("/tmp/sampv.17.dpix");
    dpixWrite("/tmp/sampv.18.dpix", dpix2);
    regTestCheckFile(rp, "/tmp/sampv.18.dpix");  /* 18 */
    regTestCompareFiles(rp, 17, 18);  /* 19 */
    dpix3 = dpixScaleByInteger(dpix2, 30);
    fpix3 = dpixConvertToFPix(dpix3);
    pixt1 = fpixRenderContours(fpix3, -2., 2.0, 0.2);
    regTestWritePixAndCheck(rp, pixt1, IFF_PNG);  /* 20 */
    pixDisplayWithTitle(pixt1, 400, 800, "v. disparity contours", rp->display);
    regTestCompareFiles(rp, 16, 20);  /* 21 */
    dpixDestroy(&dpix1);
    dpixDestroy(&dpix2);
    dpixDestroy(&dpix3);
    fpixDestroy(&fpix3);
    pixDestroy(&pixt1);

    dewarpDestroy(&dew);
    dewarpDestroy(&dew2);
    pixDestroy(&pixs);
    pixDestroy(&pixb);
    pixDestroy(&pixs2);
    pixDestroy(&pixb2);
    regTestCleanup(rp);
    return 0;
}

