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
 *   dewarptest3.c
 *
 *   This exercise functions in dewarp.c for dewarping based on lines
 *   of horizontal text, showing results for different interpolations
 *   (quadratic, cubic, quartic).
 *
 *   Inspeciton of the output pdf shows that using LS fitting beyond
 *   quadratic has a tendency to overfit.  So we choose to use
 *   quadratic LSF for the textlines.
 */

#include "allheaders.h"

l_int32 main(int    argc,
             char **argv)
{
l_int32     i, n;
l_float32   a, b, c, d, e;
NUMA       *nax, *nafit;
PIX        *pixs, *pixn, *pixg, *pixb, *pixt1, *pixt2;
PIXA       *pixa;
PTA        *pta, *ptad;
PTAA       *ptaa1, *ptaa2;

    pixs = pixRead("cat-35.jpg");

        /* Normalize for varying background and binarize */
    pixn = pixBackgroundNormSimple(pixs, NULL, NULL);
    pixg = pixConvertRGBToGray(pixn, 0.5, 0.3, 0.2);
    pixb = pixThresholdToBinary(pixg, 130);
    pixDestroy(&pixn);
    pixDestroy(&pixg);

        /* Get the textline centers */
    pixa = pixaCreate(6);
    ptaa1 = dewarpGetTextlineCenters(pixb, 0);
    pixt1 = pixCreateTemplate(pixs);
    pixSetAll(pixt1);
    pixt2 = pixDisplayPtaa(pixt1, ptaa1);
    pixWrite("/tmp/textline1.png", pixt2, IFF_PNG);
    pixDisplayWithTitle(pixt2, 0, 100, "textline centers 1", 1);
    pixaAddPix(pixa, pixt2, L_INSERT);
    pixDestroy(&pixt1);

        /* Remove short lines */
    fprintf(stderr, "Num all lines = %d\n", ptaaGetCount(ptaa1));
    ptaa2 = dewarpRemoveShortLines(pixb, ptaa1, 0.8, 0);
    pixt1 = pixCreateTemplate(pixs);
    pixSetAll(pixt1);
    pixt2 = pixDisplayPtaa(pixt1, ptaa2);
    pixWrite("/tmp/textline2.png", pixt2, IFF_PNG);
    pixDisplayWithTitle(pixt2, 300, 100, "textline centers 2", 1);
    pixaAddPix(pixa, pixt2, L_INSERT);
    pixDestroy(&pixt1);
    n = ptaaGetCount(ptaa2);
    fprintf(stderr, "Num long lines = %d\n", n);
    ptaaDestroy(&ptaa1);
    pixDestroy(&pixb);

        /* Long lines over input image */
    pixt1 = pixCopy(NULL, pixs);
    pixt2 = pixDisplayPtaa(pixt1, ptaa2);
    pixWrite("/tmp/textline3.png", pixt2, IFF_PNG);
    pixDisplayWithTitle(pixt2, 600, 100, "textline centers 3", 1);
    pixaAddPix(pixa, pixt2, L_INSERT);
    pixDestroy(&pixt1);

        /* Quadratic fit to curve */
    pixt1 = pixCopy(NULL, pixs);
    for (i = 0; i < n; i++) {
        pta = ptaaGetPta(ptaa2, i, L_CLONE);
        ptaGetArrays(pta, &nax, NULL);
        ptaGetQuadraticLSF(pta, &a, &b, &c, &nafit);
        fprintf(stderr, "Quadratic: a = %10.6f, b = %7.3f, c = %7.3f\n",
                a, b, c);
        ptad = ptaCreateFromNuma(nax, nafit);
        pixDisplayPta(pixt1, pixt1, ptad);
        ptaDestroy(&pta);
        ptaDestroy(&ptad);
        numaDestroy(&nax);
        numaDestroy(&nafit);
    }
    pixWrite("/tmp/textline4.png", pixt1, IFF_PNG);
    pixDisplayWithTitle(pixt1, 900, 100, "textline centers 4", 1);
    pixaAddPix(pixa, pixt1, L_INSERT);

        /* Cubic fit to curve */
    pixt1 = pixCopy(NULL, pixs);
    for (i = 0; i < n; i++) {
        pta = ptaaGetPta(ptaa2, i, L_CLONE);
        ptaGetArrays(pta, &nax, NULL);
        ptaGetCubicLSF(pta, &a, &b, &c, &d, &nafit);
        fprintf(stderr, "Cubic: a = %10.6f, b = %10.6f, c = %7.3f, d = %7.3f\n",
                a, b, c, d);
        ptad = ptaCreateFromNuma(nax, nafit);
        pixDisplayPta(pixt1, pixt1, ptad);
        ptaDestroy(&pta);
        ptaDestroy(&ptad);
        numaDestroy(&nax);
        numaDestroy(&nafit);
    }
    pixWrite("/tmp/textline5.png", pixt1, IFF_PNG);
    pixDisplayWithTitle(pixt1, 1200, 100, "textline centers 5", 1);
    pixaAddPix(pixa, pixt1, L_INSERT);

        /* Quartic fit to curve */
    pixt1 = pixCopy(NULL, pixs);
    for (i = 0; i < n; i++) {
        pta = ptaaGetPta(ptaa2, i, L_CLONE);
        ptaGetArrays(pta, &nax, NULL);
        ptaGetQuarticLSF(pta, &a, &b, &c, &d, &e, &nafit);
        fprintf(stderr,
            "Quartic: a = %7.3f, b = %7.3f, c = %9.5f, d = %7.3f, e = %7.3f\n",
            a, b, c, d, e);
        ptad = ptaCreateFromNuma(nax, nafit);
        pixDisplayPta(pixt1, pixt1, ptad);
        ptaDestroy(&pta);
        ptaDestroy(&ptad);
        numaDestroy(&nax);
        numaDestroy(&nafit);
    }
    pixWrite("/tmp/textline6.png", pixt1, IFF_PNG);
    pixDisplayWithTitle(pixt1, 1500, 100, "textline centers 6", 1);
    pixaAddPix(pixa, pixt1, L_INSERT);

    pixaConvertToPdf(pixa, 300, 0.5, L_JPEG_ENCODE, 75,
                     "LS fittings to textlines", "/tmp/dewarp_fittings.pdf");

    pixaDestroy(&pixa);
    pixDestroy(&pixs);
    ptaaDestroy(&ptaa2);
    return 0;
}
