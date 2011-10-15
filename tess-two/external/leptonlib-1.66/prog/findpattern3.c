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
 * findpattern3.c
 *
 *    findpattern3
 *
 *    This is setup with input parameters to work on feyn.tif.
 *
 *    This uses pixGenerateSelBoundary() to generate the sels.
 *
 *    (1) We extract a "c" bitmap, generate a hit-miss sel, and
 *    then produce several 4 bpp colormapped renditions,
 *    with the pattern either removed or highlighted.
 *
 *    (2) We do the same with the word "Caltech".
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

    /* for pixDisplayHitMissSel() */
static const l_uint32  HitColor = 0x33aa4400;
static const l_uint32  MissColor = 0xaa44bb00;


main(int    argc,
     char **argv)
{
BOX         *box;
PIX         *pixs, *pixc, *pixp, *pixsel, *pixhmt;
PIX         *pixd1, *pixd2, *pixd3;
SEL         *selhm;
static char  mainName[] = "findpattern3";

    if (argc != 1)
	exit(ERROR_INT(" Syntax:  findpattern3", mainName, 1));

    if ((pixs = pixRead("feyn.tif")) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));

        /* -------------------------------------------- *
         * Extract the pattern for a single character   *
         * ---------------------------------------------*/
    box = boxCreate(599, 1055, 18, 23);
    pixc = pixClipRectangle(pixs, box, NULL);

        /* Make a hit-miss sel */
    selhm = pixGenerateSelBoundary(pixc, 1, 2, 2, 2, 1, 1, 0, 0, &pixp);

        /* Display the sel */
    pixsel = pixDisplayHitMissSel(pixp, selhm, 7, HitColor, MissColor);
    pixDisplay(pixsel, 200, 200);
    pixWrite("/tmp/junkpixsel1", pixsel, IFF_PNG);

        /* Use the Sel to find all instances in the page */
    startTimer();
    pixhmt = pixHMT(NULL, pixs, selhm);
    fprintf(stderr, "Time to find patterns = %7.3f\n", stopTimer());

        /* Color each instance at full res */
    pixd1 = pixDisplayMatchedPattern(pixs, pixp, pixhmt, selhm->cx,
                                     selhm->cy, 0x0000ff00, 1.0, 5);
    pixWrite("/tmp/junkpixd11", pixd1, IFF_PNG);

        /* Color each instance at 0.3 scale */
    pixd2 = pixDisplayMatchedPattern(pixs, pixp, pixhmt, selhm->cx,
                                     selhm->cy, 0x0000ff00, 0.5, 5);
    pixWrite("/tmp/junkpixd12", pixd2, IFF_PNG);

        /* Remove each instance from the input image */
    pixd3 = pixCopy(NULL, pixs);
    pixRemoveMatchedPattern(pixd3, pixp, pixhmt, selhm->cx,
                                    selhm->cy, 1);
    pixWrite("/tmp/junkpixr1", pixd3, IFF_PNG);

    boxDestroy(&box);
    selDestroy(&selhm);
    pixDestroy(&pixc);
    pixDestroy(&pixp);
    pixDestroy(&pixsel);
    pixDestroy(&pixhmt);
    pixDestroy(&pixd1);
    pixDestroy(&pixd2);
    pixDestroy(&pixd3);


        /* -------------------------------------------- *
         *        Extract the pattern for a word        *
         * ---------------------------------------------*/
    box = boxCreate(208, 872, 130, 35);
    pixc = pixClipRectangle(pixs, box, NULL);

        /* Make a hit-miss sel */
    selhm = pixGenerateSelBoundary(pixc, 2, 2, 1, 4, 1, 1, 0, 0, &pixp);

        /* Display the sel */
    pixsel = pixDisplayHitMissSel(pixp, selhm, 7, HitColor, MissColor);
    pixDisplay(pixsel, 200, 200);
    pixWrite("/tmp/junkpixsel2", pixsel, IFF_PNG);

        /* Use the Sel to find all instances in the page */
    startTimer();
    pixhmt = pixHMT(NULL, pixs, selhm);
    fprintf(stderr, "Time to find word patterns = %7.3f\n", stopTimer());

        /* Color each instance at full res */
    pixd1 = pixDisplayMatchedPattern(pixs, pixp, pixhmt, selhm->cx,
                                     selhm->cy, 0x0000ff00, 1.0, 5);
    pixWrite("/tmp/junkpixd21", pixd1, IFF_PNG);

        /* Color each instance at 0.3 scale */
    pixd2 = pixDisplayMatchedPattern(pixs, pixp, pixhmt, selhm->cx,
                                     selhm->cy, 0x0000ff00, 0.5, 5);
    pixWrite("/tmp/junkpixd22", pixd2, IFF_PNG);

        /* Remove each instance from the input image */
    pixd3 = pixCopy(NULL, pixs);
    pixRemoveMatchedPattern(pixd3, pixp, pixhmt, selhm->cx,
                                    selhm->cy, 1);
    pixWrite("/tmp/junkpixr2", pixd3, IFF_PNG);

    selDestroy(&selhm);
    boxDestroy(&box);
    pixDestroy(&pixc);
    pixDestroy(&pixp);
    pixDestroy(&pixsel);
    pixDestroy(&pixhmt);
    pixDestroy(&pixd1);
    pixDestroy(&pixd2);
    pixDestroy(&pixd3);
    pixDestroy(&pixs);
    return 0;
}

