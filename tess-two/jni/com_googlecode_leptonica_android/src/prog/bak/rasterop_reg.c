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
 * rasterop_reg.c
 *
 *     This is a fairly rigorous test of rasterop.
 *     It demonstrates both that the results are correct
 *     with many different rop configurations, and,
 *     if done under valgrind, that no memory violations occur.
 *
 *     Use it on images with a significant amount of FG
 *     that extends to the edges.
 */

#include "allheaders.h"

main(int    argc,
     char **argv)
{
l_int32      i, j, w, h, same, width, height, cx, cy;
l_uint32     val;
PIX         *pixs, *pixse, *pixd1, *pixd2;
SEL         *sel;
static char  mainName[] = "rasterop_reg";

    if (argc != 1)
	return ERROR_INT(" Syntax:  rasterop_reg", mainName, 1);
    pixs = pixRead("feyn.tif");

    for (width = 1; width <= 25; width += 3) {
	for (height = 1; height <= 25; height += 4) {

	    cx = width / 2;
	    cy = height / 2;

		/* Dilate using an actual sel */
	    sel = selCreateBrick(height, width, cy, cx, SEL_HIT);
	    pixd1 = pixDilate(NULL, pixs, sel);

		/* Dilate using a pix as a sel */
	    pixse = pixCreate(width, height, 1);
	    pixSetAll(pixse);
	    pixd2 = pixCopy(NULL, pixs);
	    w = pixGetWidth(pixs);
	    h = pixGetHeight(pixs);
	    for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
		    pixGetPixel(pixs, j, i, &val);
		    if (val)
			pixRasterop(pixd2, j - cx, i - cy, width, height,
				    PIX_SRC | PIX_DST, pixse, 0, 0);
		}
	    }
	    
	    pixEqual(pixd1, pixd2, &same);
	    if (same == 1)
		fprintf(stderr, "Correct for (%d,%d)\n", width, height);
	    else {
		fprintf(stderr, "Error: results are different!\n");
		fprintf(stderr, "SE: width = %d, height = %d\n", width, height);
		pixWrite("/tmp/junkout1", pixd1, IFF_PNG);
		pixWrite("/tmp/junkout2", pixd2, IFF_PNG);
                return 1;
	    }

	    pixDestroy(&pixse);
	    pixDestroy(&pixd1);
	    pixDestroy(&pixd2);
	    selDestroy(&sel);
	}
    }
    pixDestroy(&pixs);
    return 0;
}
