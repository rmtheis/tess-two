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
 * cctest1.c
 *
 *    This is a test of the following function:
 *
 *        BOXA   *pixConnComp(PIX *pixs, PIXA  **ppixa, l_int32 connectivity)
 *
 *                    pixs:  input pix
 *                    ppixa: &pixa (<optional> pixa of each c.c.)
 *                    connectivity (4 or 8)
 *                    boxa: returned array of boxes of c.c.
 *
 *        Use NULL for &pixa if you don't want the pixa array.
 *
 *    It also demonstrates a few display modes.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define  NTIMES             2


main(int    argc,
     char **argv)
{
char        *filein;
l_int32      i, n, np, same, count;
FILE        *fp;
BOX         *box;
BOXA        *boxa, *boxa2;
PIX         *pixs, *pixd;
PIXA        *pixa;
PIXCMAP     *cmap;
static char  mainName[] = "cctest1";

    if (argc != 2)
	exit(ERROR_INT(" Syntax:  cctest1 filein", mainName, 1));

    filein = argv[1];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
    if (pixGetDepth(pixs) != 1)
	exit(ERROR_INT("pixs not 1 bpp", mainName, 1));
	    
	/* Test speed of pixCountConnComp() */
    startTimer();
    for (i = 0; i < NTIMES; i++)
	pixCountConnComp(pixs, 4, &count);
    fprintf(stderr, "Time to compute 4-cc: %6.3f sec\n", stopTimer()/NTIMES);
    fprintf(stderr, "Number of 4-cc: %d\n", count);
    startTimer();
    for (i = 0; i < NTIMES; i++)
	pixCountConnComp(pixs, 8, &count);
    fprintf(stderr, "Time to compute 8-cc: %6.3f sec\n", stopTimer()/NTIMES);
    fprintf(stderr, "Number of 8-cc: %d\n", count);

	/* Test speed of pixConnComp(), with only boxa output  */
    startTimer();
    for (i = 0; i < NTIMES; i++) {
	boxa = pixConnComp(pixs, NULL, 4);
	boxaDestroy(&boxa);
    }
    fprintf(stderr, "Time to compute 4-cc: %6.3f sec\n", stopTimer()/NTIMES);
    startTimer();
    for (i = 0; i < NTIMES; i++) {
	boxa = pixConnComp(pixs, NULL, 8);
	boxaDestroy(&boxa);
    }
    fprintf(stderr, "Time to compute 8-cc: %6.3f sec\n", stopTimer()/NTIMES);

	/* Draw outline of each c.c. box */
    boxa = pixConnComp(pixs, NULL, 4);
    n = boxaGetCount(boxa);
    fprintf(stderr, "Num 4-cc boxes: %d\n", n);
    for (i = 0; i < n; i++) {
	box = boxaGetBox(boxa, i, L_CLONE);
	pixRenderBox(pixs, box, 3, L_FLIP_PIXELS);
	boxDestroy(&box);   /* remember, clones need to be destroyed */
    }
    pixDisplayWrite(pixs, 1);
    boxaDestroy(&boxa);

        /* Display each component as a random color in cmapped 8 bpp.
         * Background is color 0; it is set to white. */
    boxa = pixConnComp(pixs, &pixa, 4);
    pixd = pixaDisplayRandomCmap(pixa, pixGetWidth(pixs), pixGetHeight(pixs));
    cmap = pixGetColormap(pixd);
    pixcmapResetColor(cmap, 0, 255, 255, 255);  /* reset background to white */
    pixDisplay(pixd, 100, 100);
    pixDisplayWrite(pixd, 1);
    boxaDestroy(&boxa);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixDestroy(&pixs);
    return 0;
}


