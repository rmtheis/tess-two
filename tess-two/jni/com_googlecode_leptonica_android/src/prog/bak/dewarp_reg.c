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
L_DEWARP     *dew1, *dew2, *dew3, *dew4;
L_DEWARPA    *dewa1, *dewa2;
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

        /* Read page 7, normalize for varying background and binarize */
    pixs = pixRead("1555-7.jpg");
    pixn = pixBackgroundNormSimple(pixs, NULL, NULL);
    pixg = pixConvertRGBToGray(pixn, 0.5, 0.3, 0.2);
    pixb = pixThresholdToBinary(pixg, 130);
    pixDestroy(&pixn);
    pixDestroy(&pixg);
    regTestWritePixAndCheck(rp, pixb, IFF_PNG);  /* 0 */
    pixDisplayWithTitle(pixb, 0, 0, "page 7 binarized input", rp->display);

        /* Get the textline centers */
    ptaa1 = dewarpGetTextlineCenters(pixb, 0);
    pixt1 = pixCreateTemplate(pixs);
    pixt2 = pixDisplayPtaa(pixt1, ptaa1);
    regTestWritePixAndCheck(rp, pixt2, IFF_PNG);  /* 1 */
    pixDisplayWithTitle(pixt2, 0, 500, "textline centers", rp->display);
    pixDestroy(&pixt1);

        /* Remove short lines */
    ptaa2 = dewarpRemoveShortLines(pixb, ptaa1, 0.8, 0);

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

        /* Build the model for page 7 and dewarp */
    dewa1 = dewarpaCreate(2, 30, 1, 15, 30);
    if ((dew1 = dewarpCreate(pixb, 7)) == NULL)
        return ERROR_INT("\n\n\n FAILURE !!! \n\n\n", rp->testname, 1);
    dewarpaInsertDewarp(dewa1, dew1);
    dewarpBuildModel(dew1, NULL);
    dewarpaApplyDisparity(dewa1, 7, pixb, NULL);
    regTestWritePixAndCheck(rp, dew1->pixd, IFF_PNG);  /* 3 */
    pixDisplayWithTitle(dew1->pixd, 400, 0, "page 7 dewarped", rp->display);

        /* Read page 3, normalize background and binarize */
    pixs2 = pixRead("1555-3.jpg");
    pixn2 = pixBackgroundNormSimple(pixs2, NULL, NULL);
    pixg2 = pixConvertRGBToGray(pixn2, 0.5, 0.3, 0.2);
    pixb2 = pixThresholdToBinary(pixg2, 130);
    pixDestroy(&pixn2);
    pixDestroy(&pixg2);
    regTestWritePixAndCheck(rp, pixb, IFF_PNG);  /* 4 */
    pixDisplayWithTitle(pixb, 0, 400, "binarized input (2)", rp->display);

        /* Minimize and re-apply page 7 disparity to this image */
    dewarpaInsertRefModels(dewa1, 0);
    dewarpaApplyDisparity(dewa1, 3, pixb2, NULL);
    dew2 = dewarpaGetDewarp(dewa1, 3);
    regTestWritePixAndCheck(rp, dew2->pixd, IFF_PNG);  /* 5 */
    pixDisplayWithTitle(dew2->pixd, 400, 400, "page 3 dewarped", rp->display);

        /* Write and read back minimized dewarp struct */
    dewarpMinimize(dew1);
    dewarpWrite("/tmp/dewarp.6.dew", dew1);
    regTestCheckFile(rp, "/tmp/dewarp.6.dew");  /* 6 */
    dew3 = dewarpRead("/tmp/dewarp.6.dew");
    dewarpWrite("/tmp/dewarp.7.dew", dew3);
    regTestCheckFile(rp, "/tmp/dewarp.7.dew");  /* 7 */
    regTestCompareFiles(rp, 6, 7);  /* 8 */

        /* Apply this minimized dew to page 3 in a new dewa */
    dewa2 = dewarpaCreate(2, 30, 1, 15, 30);
    dewarpaInsertDewarp(dewa2, dew3);
    dewarpaInsertRefModels(dewa2, 0);
    dewarpaListPages(dewa2);  /* just for fun: should be 1, 3, 5, 7 */
    dewarpaApplyDisparity(dewa2, 3, pixb2, NULL);
    dew4 = dewarpaGetDewarp(dewa2, 3);
    regTestWritePixAndCheck(rp, dew4->pixd, IFF_PNG);  /* 9 */
    pixDisplayWithTitle(dew4->pixd, 800, 400, "page 3 dewarped again",
                        rp->display);

        /* Minimize, re-populate disparity arrays, and apply again */
    dewarpMinimize(dew3);
    dewarpaApplyDisparity(dewa2, 3, pixb2, NULL);
    regTestWritePixAndCheck(rp, dew4->pixd, IFF_PNG);  /* 10 */
    regTestCompareFiles(rp, 9, 10);  /* 11 */
    pixDisplayWithTitle(dew3->pixd, 900, 400, "page 3 dewarped yet again",
                        rp->display);

        /* Test a few of the fpix functions */
    fpix1 = fpixClone(dew3->sampvdispar);
    fpixWrite("/tmp/sampv.12.fpix", fpix1);
    regTestCheckFile(rp, "/tmp/sampv.12.fpix");  /* 12 */
    fpix2 = fpixRead("/tmp/sampv.12.fpix");
    fpixWrite("/tmp/sampv.13.fpix", fpix2);
    regTestCheckFile(rp, "/tmp/sampv.13.fpix");  /* 13 */
    regTestCompareFiles(rp, 12, 13);  /* 14 */
    fpix3 = fpixScaleByInteger(fpix2, 30);
    pixt1 = fpixRenderContours(fpix3, 2.0, 0.2);
    regTestWritePixAndCheck(rp, pixt1, IFF_PNG);  /* 15 */
    pixDisplayWithTitle(pixt1, 0, 800, "v. disparity contours", rp->display);
    fpixDestroy(&fpix1);
    fpixDestroy(&fpix2);
    fpixDestroy(&fpix3);
    pixDestroy(&pixt1);

        /* Test a few of the dpix functions */
    dpix1 = fpixConvertToDPix(dew3->sampvdispar);
    dpixWrite("/tmp/sampv.16.dpix", dpix1);
    regTestCheckFile(rp, "/tmp/sampv.16.dpix");  /* 16 */
    dpix2 = dpixRead("/tmp/sampv.16.dpix");
    dpixWrite("/tmp/sampv.17.dpix", dpix2);
    regTestCheckFile(rp, "/tmp/sampv.17.dpix");  /* 17 */
    regTestCompareFiles(rp, 16, 17);  /* 18 */
    dpix3 = dpixScaleByInteger(dpix2, 30);
    fpix3 = dpixConvertToFPix(dpix3);
    pixt1 = fpixRenderContours(fpix3, 2.0, 0.2);
    regTestWritePixAndCheck(rp, pixt1, IFF_PNG);  /* 19 */
    pixDisplayWithTitle(pixt1, 400, 800, "v. disparity contours", rp->display);
    regTestCompareFiles(rp, 15, 19);  /* 20 */
    dpixDestroy(&dpix1);
    dpixDestroy(&dpix2);
    dpixDestroy(&dpix3);
    fpixDestroy(&fpix3);
    pixDestroy(&pixt1);

    dewarpaDestroy(&dewa1);
    dewarpaDestroy(&dewa2);
    pixDestroy(&pixs);
    pixDestroy(&pixb);
    pixDestroy(&pixs2);
    pixDestroy(&pixb2);
    return regTestCleanup(rp);
}
