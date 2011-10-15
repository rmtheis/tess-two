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
 * findpattern_reg.c
 *
 *    This uses pixGenerateSelBoundary() to generate hit-miss Sels
 *    that are a good fit for two 1 bpp patterns:
 *       * a "T" in the banner name
 *       * the banner name ("Tribune")
 *    The sels are first displayed, showing the hit and miss elements
 *    in color.
 *
 *    The sels are then used to identify and remove the components
 *    in a page image in which thay are found.  We demonstrate
 *    the ability to find these components are reductions from 4 to 16x.
 *    (16x is extreme -- don't do this at home!)  The results are displayed
 *    with the matched pattern either highlighted or removed.
 *
 *    Some of these Sels are also made by livre_hmt.c for figures
 *    in the Document Image Applications chapter.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

    /* for pixDisplayHitMissSel() */
static const l_uint32  HitColor = 0x33aa4400;
static const l_uint32  MissColor = 0xaa44bb00;

    /* Patterns at full resolution */
static const char *patname[2] = {
    "tribune-word.png",   /* patno = 0 */
    "tribune-t.png"};     /* patno = 1 */

l_int32 GeneratePattern(l_int32 patno, l_int32 red, L_REGPARAMS  *rp,
                        l_int32 *count);


main(int    argc,
     char **argv)
{
l_int32      patno, red, count, success, display;
FILE         *fp;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &fp, &display, &success, &rp))
        return 1;

    count = 0;
    for (patno = 0; patno < 2; patno++) {
        for (red = 4; red <= 16; red *= 2) {
            if (patno == 1 && red == 16) continue;
            GeneratePattern(patno, red, rp, &count);
        }
    }

    regTestCleanup(argc, argv, fp, success, rp);
    return 0;
}


l_int32
GeneratePattern(l_int32       patno,
                l_int32       red,
                L_REGPARAMS  *rp,
                l_int32      *pcount)
{
l_int32  width, cx, cy;
PIX     *pixs, *pixt, *pix, *pixr, *pixp, *pixsel, *pixhmt;
PIX     *pixc1, *pixc2, *pixc3, *pixd;
PIXA    *pixa;
SEL     *selhm;

    PROCNAME("GeneratePattern");

    if ((pixs = pixRead(patname[patno])) == NULL)
	return ERROR_INT("pixs not made", procName, 1);

        /* Make a hit-miss sel at specified reduction factor */
    if (red == 4) {
        pixt = pixReduceRankBinaryCascade(pixs, 4, 4, 0, 0);
        selhm = pixGenerateSelBoundary(pixt, 2, 2, 20, 30, 1, 1, 0, 0, &pixp);
    }
    else if (red == 8) {
        pixt = pixReduceRankBinaryCascade(pixs, 4, 4, 2, 0);
        selhm = pixGenerateSelBoundary(pixt, 1, 2, 6, 12, 1, 1, 0, 0, &pixp);
    }
    else {  /*  red == 16 */
        pixt = pixReduceRankBinaryCascade(pixs, 4, 4, 2, 2);
        selhm = pixGenerateSelBoundary(pixt, 1, 1, 4, 8, 0, 0, 0, 0, &pixp);
    }
    pixDestroy(&pixt);

        /* Display the sel */
    pixsel = pixDisplayHitMissSel(pixp, selhm, 7, HitColor, MissColor);
    pixa = pixaCreate(2);
    pixaAddPix(pixa, pixs, L_CLONE);
    pixaAddPix(pixa, pixsel, L_CLONE);
    width = (patno == 0) ? 1200 : 400;
    pixd = pixaDisplayTiledAndScaled(pixa, 32, width, 2, 0, 30, 2);
    regTestWritePixAndCheck(pixd, IFF_PNG, pcount, rp);
    pixDisplayWithTitle(pixd, 100, 100 + 100 * (3 * patno + red / 4),
                        NULL, rp->display);
    pixaDestroy(&pixa);
    pixDestroy(&pixd);

        /* Use the sel to find all instances in the page */
    pix = pixRead("tribune-page-4x.png");  /* 4x reduced */
    if (red == 4)
        pixr = pixClone(pix);
    else if (red == 8)
        pixr = pixReduceRankBinaryCascade(pix, 2, 0, 0, 0);
    else if (red == 16)
        pixr = pixReduceRankBinaryCascade(pix, 2, 2, 0, 0);
    pixDestroy(&pix);

    startTimer();
    pixhmt = pixHMT(NULL, pixr, selhm);
    fprintf(stderr, "Time to find patterns = %7.3f\n", stopTimer());

        /* Color each instance at full res */
    selGetParameters(selhm, NULL, NULL, &cy, &cx);
    pixc1 = pixDisplayMatchedPattern(pixr, pixp, pixhmt,
                                     cx, cy, 0x0000ff00, 1.0, 5);
    regTestWritePixAndCheck(pixc1, IFF_PNG, pcount, rp);
    pixDisplayWithTitle(pixc1, 500, 100, NULL, rp->display);
    
        /* Color each instance at 0.5 scale */
    pixc2 = pixDisplayMatchedPattern(pixr, pixp, pixhmt,
                                     cx, cy, 0x0000ff00, 0.5, 5);
    regTestWritePixAndCheck(pixc2, IFF_PNG, pcount, rp);

        /* Remove each instance from the input image */
    pixc3 = pixCopy(NULL, pixr);
    pixRemoveMatchedPattern(pixc3, pixp, pixhmt, cx, cy, 1);
    regTestWritePixAndCheck(pixc3, IFF_PNG, pcount, rp);

    selDestroy(&selhm);
    pixDestroy(&pixp);
    pixDestroy(&pixsel);
    pixDestroy(&pixhmt);
    pixDestroy(&pixc1);
    pixDestroy(&pixc2);
    pixDestroy(&pixc3);
    pixDestroy(&pixd);
    pixDestroy(&pixs);
    return 0;
}

