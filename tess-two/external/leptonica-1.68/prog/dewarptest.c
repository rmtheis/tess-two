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
 *   This exercise functions in dewarp.c for dewarping based on lines
 *   of horizontal text.  It also creates a 19-image pdf of steps
 *   in the process.
 */

#include "allheaders.h"

#define   DO_QUAD     1
#define   DO_CUBIC    0
#define   DO_QUARTIC  0

l_int32 main(int    argc,
             char **argv)
{
l_int32    i, n, ignore;
l_float32  a, b, c, d, e;
L_DEWARP  *dew;
FILE      *fp;
FPIX      *fpix;
NUMA      *nax, *nay, *nafit;
PIX       *pixs, *pixn, *pixg, *pixb, *pixt1, *pixt2, *pixt3;
PIX       *pixs2, *pixn2, *pixg2, *pixb2, *pixv, *pixd;
PTA       *pta, *ptad;
PTAA      *ptaa1, *ptaa2;

    pixs = pixRead("1555-7.jpg");

        /* Normalize for varying background and binarize */
    pixn = pixBackgroundNormSimple(pixs, NULL, NULL);
    pixg = pixConvertRGBToGray(pixn, 0.5, 0.3, 0.2);
    pixb = pixThresholdToBinary(pixg, 130);

        /* Run the basic functions */
    dew = dewarpCreate(pixb, 7, 30, 15, 1);
    dewarpBuildModel(dew, 1);
    dewarpApplyDisparity(dew, pixg, 1);

        /* Save the intermediate dewarped images */
    pixv = pixRead("/tmp/pixv.png");
    pixd = pixRead("/tmp/pixd.png");

        /* Normalize another image, that doesn't have enough textlines
         * to build an accurate model */
    pixs2 = pixRead("1555-3.jpg");
    pixn2 = pixBackgroundNormSimple(pixs2, NULL, NULL);
    pixg2 = pixConvertRGBToGray(pixn2, 0.5, 0.3, 0.2);
    pixb2 = pixThresholdToBinary(pixg2, 130);

        /* Apply the previous disparity model to this image */
    dewarpApplyDisparity(dew, pixg2, 1);
    dewarpDestroy(&dew);

        /* Get the textline centers */
    ptaa1 = pixGetTextlineCenters(pixb, 0);
    pixt1 = pixCreateTemplate(pixs);
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

         /* Write out the files to be imaged */
    lept_mkdir("junkdir");
    pixWrite("/tmp/junkdir/001.jpg", pixs, IFF_JFIF_JPEG);
    pixWrite("/tmp/junkdir/002.jpg", pixn, IFF_JFIF_JPEG);
    pixWrite("/tmp/junkdir/003.jpg", pixg, IFF_JFIF_JPEG);
    pixWrite("/tmp/junkdir/004.png", pixb, IFF_TIFF_G4);
    pixt1 = pixRead("/tmp/textline1.png");
    pixWrite("/tmp/junkdir/005.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixt1 = pixRead("/tmp/textline2.png");
    pixWrite("/tmp/junkdir/006.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixt1 = pixRead("/tmp/lines1.png");
    pixWrite("/tmp/junkdir/007.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixt1 = pixRead("/tmp/lines2.png");
    pixWrite("/tmp/junkdir/008.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixt1 = pixRead("/tmp/vert-contours.png");
    pixWrite("/tmp/junkdir/009.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixWrite("/tmp/junkdir/010.png", pixv, IFF_PNG);
    pixt1 = pixThresholdToBinary(pixv, 130);
    pixWrite("/tmp/junkdir/011.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixt1 = pixRead("/tmp/horiz-contours.png");
    pixWrite("/tmp/junkdir/012.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixWrite("/tmp/junkdir/013.png", pixd, IFF_PNG);
    pixt1 = pixThresholdToBinary(pixd, 130);
    pixWrite("/tmp/junkdir/014.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixWrite("/tmp/junkdir/015.png", pixb, IFF_TIFF_G4);

        /* (these are for the second image) */
    pixWrite("/tmp/junkdir/016.jpg", pixs2, IFF_JFIF_JPEG);
    pixWrite("/tmp/junkdir/017.png", pixb2, IFF_TIFF_G4);
    pixt1 = pixRead("/tmp/pixv.png");
    pixt2 = pixThresholdToBinary(pixt1, 130);
    pixWrite("/tmp/junkdir/018.png", pixt2, IFF_PNG);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixt1 = pixRead("/tmp/pixd.png");
    pixt2 = pixThresholdToBinary(pixt1, 130);
    pixWrite("/tmp/junkdir/019.png", pixt2, IFF_PNG);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

        /* Generate the 19 page ps and pdf files */
    convertFilesToPS("/tmp/junkdir", NULL, 135, "/tmp/dewarp.ps");
    fprintf(stderr, "ps file made: /tmp/dewarp.ps\n");
    ignore = system("ps2pdf /tmp/dewarp.ps /tmp/dewarp.pdf");
    fprintf(stderr, "pdf file made: /tmp/dewarp.pdf\n");

    pixDestroy(&pixs);
    pixDestroy(&pixn);
    pixDestroy(&pixg);
    pixDestroy(&pixb);
    pixDestroy(&pixs2);
    pixDestroy(&pixn2);
    pixDestroy(&pixg2);
    pixDestroy(&pixb2);
    pixDestroy(&pixv);
    pixDestroy(&pixd);

    return 0;
}


