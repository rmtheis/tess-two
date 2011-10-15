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
 * livre_hmt.c
 *
 *    This demonstrates use of pixGenerateSelBoundary() to
 *    generate a hit-miss Sel.
 *
 *    (1) The Sel is displayed with the hit and miss elements in color.
 *
 *    (2) We produce several 4 bpp colormapped renditions,
 *        with the matched pattern either hightlighted or removed.
 *
 *    (3) For figures in the Document Image Applications chapter:
 *           fig 7:  livre_hmt 1 8
 *           fig 8:  livre_hmt 2 4
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

    /* for pixDisplayHitMissSel() */
static const l_uint32  HitColor = 0x33aa4400;
static const l_uint32  MissColor = 0xaa44bb00;

    /* Patterns at full resolution */
static const char *patname[3] = {
    "",
    "tribune-word.png",   /* patno = 1 */
    "tribune-t.png"};     /* patno = 2 */


main(int    argc,
     char **argv)
{
l_int32      patno, reduction, width, cols, cx, cy;
BOX         *box;
PIX         *pixs, *pixt, *pix, *pixr, *pixp, *pixsel, *pixhmt;
PIX         *pixd1, *pixd2, *pixd3, *pixd;
PIXA        *pixa;
SEL         *selhm;
static char  mainName[] = "livre_hmt";

    if (argc != 3)
	exit(ERROR_INT(" Syntax:  livre_hmt pattern reduction", mainName, 1));
    patno = atoi(argv[1]);
    reduction = atoi(argv[2]);

    if ((pixs = pixRead(patname[patno])) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
    if (reduction != 4 && reduction != 8 && reduction != 16)
	exit(ERROR_INT("reduction not 4, 8 or 16", mainName, 1));

    if (reduction == 4)
        pixt = pixReduceRankBinaryCascade(pixs, 4, 4, 0, 0);
    else if (reduction == 8)
        pixt = pixReduceRankBinaryCascade(pixs, 4, 4, 2, 0);
    else if (reduction == 16)
        pixt = pixReduceRankBinaryCascade(pixs, 4, 4, 2, 2);

        /* Make a hit-miss sel */
    if (reduction == 4)
        selhm = pixGenerateSelBoundary(pixt, 2, 2, 20, 30, 1, 1, 0, 0, &pixp);
    else if (reduction == 8)
        selhm = pixGenerateSelBoundary(pixt, 1, 2, 6, 12, 1, 1, 0, 0, &pixp);
    else if (reduction == 16)
        selhm = pixGenerateSelBoundary(pixt, 1, 1, 4, 8, 0, 0, 0, 0, &pixp);

        /* Display the sel */
    pixsel = pixDisplayHitMissSel(pixp, selhm, 7, HitColor, MissColor);
    pixDisplay(pixsel, 200, 200);
    pixWrite("/tmp/pixsel1", pixsel, IFF_PNG);

        /* Use the Sel to find all instances in the page */
    pix = pixRead("tribune-page-4x.png");  /* 4x reduced */
    if (reduction == 4)
        pixr = pixClone(pix);
    else if (reduction == 8)
        pixr = pixReduceRankBinaryCascade(pix, 2, 0, 0, 0);
    else if (reduction == 16)
        pixr = pixReduceRankBinaryCascade(pix, 2, 2, 0, 0);

    startTimer();
    pixhmt = pixHMT(NULL, pixr, selhm);
    fprintf(stderr, "Time to find patterns = %7.3f\n", stopTimer());

        /* Color each instance at full res */
    selGetParameters(selhm, NULL, NULL, &cy, &cx);
    pixd1 = pixDisplayMatchedPattern(pixr, pixp, pixhmt,
                                     cx, cy, 0x0000ff00, 1.0, 5);
    pixWrite("/tmp/pixd11", pixd1, IFF_PNG);

        /* Color each instance at 0.5 scale */
    pixd2 = pixDisplayMatchedPattern(pixr, pixp, pixhmt,
                                     cx, cy, 0x0000ff00, 0.5, 5);
    pixWrite("/tmp/pixd12", pixd2, IFF_PNG);

        /* Remove each instance from the input image */
    pixd3 = pixCopy(NULL, pixr);
    pixRemoveMatchedPattern(pixd3, pixp, pixhmt, cx, cy, 1);
    pixWrite("/tmp/pixr1", pixd3, IFF_PNG);

    pixa = pixaCreate(2);
    pixaAddPix(pixa, pixs, L_CLONE);
    pixaAddPix(pixa, pixsel, L_CLONE);
    cols = (patno == 1) ? 1 : 2;
    width = (patno == 1) ? 800 : 400;
    pixd = pixaDisplayTiledAndScaled(pixa, 32, width, cols, 0, 30, 2);
    pixWrite("/tmp/hmt.png", pixd, IFF_PNG);
    pixDisplay(pixd, 0, 300);

    selDestroy(&selhm);
    pixDestroy(&pixp);
    pixDestroy(&pixsel);
    pixDestroy(&pixhmt);
    pixDestroy(&pixd1);
    pixDestroy(&pixd2);
    pixDestroy(&pixd3);
    pixDestroy(&pixs);
    pixDestroy(&pix);
    pixDestroy(&pixt);
    return 0;
}

