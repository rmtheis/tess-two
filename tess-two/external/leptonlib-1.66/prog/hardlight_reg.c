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
 * hardlight_reg.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static l_int32  count = 0;

static PIXA *
TestHardlight(const char   *file1,
              const char   *file2,
              L_REGPARAMS  *rp)
{
char   fname[256];
PIX   *pixs1, *pixs2, *pixr, *pixt1, *pixt2, *pixd;
PIXA  *pixa;

    PROCNAME("TestHardlight");

        /* Read in images */
    pixs1 = pixRead(file1);
    pixs2 = pixRead(file2);
    if (!pixs1)
        return (PIXA *)ERROR_PTR("pixs1 not read", procName, NULL);
    if (!pixs2)
        return (PIXA *)ERROR_PTR("pixs2 not read", procName, NULL);

    pixa = pixaCreate(0);

        /* ---------- Test not-in-place; no colormaps ----------- */
    pixSaveTiled(pixs1, pixa, 1, 1, 20, 32);
    pixSaveTiled(pixs2, pixa, 1, 0, 20, 0);
    pixd = pixBlendHardLight(NULL, pixs1, pixs2, 0, 0, 1.0);
    snprintf(fname, 240, "/tmp/hardlight.%d.png", count);
    pixWrite(fname, pixd, IFF_PNG);
    regTestCheckFile(rp->fp, rp->argv, fname, count++, &rp->success);
    pixSaveTiled(pixd, pixa, 1, 1, 20, 0);
    pixDestroy(&pixd);

    pixt2 = pixConvertTo32(pixs2);
    pixd = pixBlendHardLight(NULL, pixs1, pixt2, 0, 0, 1.0);
    snprintf(fname, 240, "/tmp/hardlight.%d.png", count);
    pixWrite(fname, pixd, IFF_PNG);
    regTestCheckFile(rp->fp, rp->argv, fname, count++, &rp->success);
    pixSaveTiled(pixd, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);

    pixd = pixBlendHardLight(NULL, pixs2, pixs1, 0, 0, 1.0);
    pixSaveTiled(pixd, pixa, 1, 0, 20, 0);
    pixDestroy(&pixd);

        /* ---------- Test not-in-place; colormaps ----------- */
    pixt1 = pixMedianCutQuant(pixs1, 0);
    if (pixGetDepth(pixs2) == 8)
        pixt2 = pixConvertGrayToColormap8(pixs2, 8);
    else
/*        pixt2 = pixConvertTo8(pixs2, 1); */
        pixt2 = pixMedianCutQuant(pixs2, 0);
    pixSaveTiled(pixt1, pixa, 1, 1, 20, 0);
    pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);

    pixd = pixBlendHardLight(NULL, pixt1, pixs2, 0, 0, 1.0);
    snprintf(fname, 240, "/tmp/hardlight.%d.png", count);
    pixWrite(fname, pixd, IFF_PNG);
    regTestCheckFile(rp->fp, rp->argv, fname, count++, &rp->success);
    pixSaveTiled(pixd, pixa, 1, 1, 20, 0);
    pixDestroy(&pixd);

    pixd = pixBlendHardLight(NULL, pixt1, pixt2, 0, 0, 1.0);
    snprintf(fname, 240, "/tmp/hardlight.%d.png", count);
    pixWrite(fname, pixd, IFF_PNG);
    regTestCheckFile(rp->fp, rp->argv, fname, count++, &rp->success);
    pixSaveTiled(pixd, pixa, 1, 0, 20, 0);
    pixDestroy(&pixd);

    pixd = pixBlendHardLight(NULL, pixt2, pixt1, 0, 0, 1.0);
    snprintf(fname, 240, "/tmp/hardlight.%d.png", count);
    pixWrite(fname, pixd, IFF_PNG);
    regTestCheckFile(rp->fp, rp->argv, fname, count++, &rp->success);
    pixSaveTiled(pixd, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);

        /* ---------- Test in-place; no colormaps ----------- */
    pixBlendHardLight(pixs1, pixs1, pixs2, 0, 0, 1.0);
    snprintf(fname, 240, "/tmp/hardlight.%d.png", count);
    pixWrite(fname, pixs1, IFF_PNG);
    regTestCheckFile(rp->fp, rp->argv, fname, count++, &rp->success);
    pixSaveTiled(pixs1, pixa, 1, 1, 20, 0);
    pixDestroy(&pixs1);

    pixs1 = pixRead(file1);
    pixt2 = pixConvertTo32(pixs2);
    pixBlendHardLight(pixs1, pixs1, pixt2, 0, 0, 1.0);
    snprintf(fname, 240, "/tmp/hardlight.%d.png", count);
    pixWrite(fname, pixs1, IFF_PNG);
    regTestCheckFile(rp->fp, rp->argv, fname, count++, &rp->success);
    pixSaveTiled(pixs1, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt2);
    pixDestroy(&pixs1);

    pixs1 = pixRead(file1);
    pixBlendHardLight(pixs2, pixs2, pixs1, 0, 0, 1.0);
    snprintf(fname, 240, "/tmp/hardlight.%d.png", count);
    pixWrite(fname, pixs2, IFF_PNG);
    regTestCheckFile(rp->fp, rp->argv, fname, count++, &rp->success);
    pixSaveTiled(pixs2, pixa, 1, 0, 20, 0);
    pixDestroy(&pixs2);

    pixDestroy(&pixs1);
    pixDestroy(&pixs2);

    return pixa;
}



main(int    argc,
     char **argv)
{
char          fname[256];
l_int32       success, display;
FILE         *fp;
PIX          *pix;
PIXA         *pixa;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &fp, &display, &success, &rp))
        return 1;

    pixa = TestHardlight("hardlight1_1.jpg", "hardlight1_2.jpg", rp);
    pix = pixaDisplay(pixa, 0, 0);
    snprintf(fname, 240, "/tmp/hardlight.%d.png", count);
    pixWrite(fname, pix, IFF_PNG);
    regTestCheckFile(rp->fp, rp->argv, fname, count++, &rp->success);
    pixDisplayWithTitle(pix, 0, 0, NULL, display);
    pixaDestroy(&pixa);
    pixDestroy(&pix);

    pixa = TestHardlight("hardlight2_1.jpg", "hardlight2_2.jpg", rp);
    pix = pixaDisplay(pixa, 0, 500);
    snprintf(fname, 240, "/tmp/hardlight.%d.png", count);
    pixWrite(fname, pix, IFF_PNG);
    regTestCheckFile(rp->fp, rp->argv, fname, count++, &rp->success);
    pixDisplayWithTitle(pix, 0, 0, NULL, display);
    pixaDestroy(&pixa);
    pixDestroy(&pix);

    regTestCleanup(argc, argv, fp, success, rp);
    return 0;
}

