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
 * colorseg_reg.c
 *
 *   This explores the space of the four parameters input for color
 *   segmentation.  Of the four, only two strongly determine the
 *   output result:
 *      maxdist (the maximum distance between pixels that get
 *               clustered: 20 is very small, 180 is very large)
 *      selsize (responsible for smoothing the result: 0 is no
 *               smoothing (fine texture), 8 is large smoothing)
 *
 *   For large selsize (>~ 6), large regions get the same color,
 *   and there are few colors in the final result.
 *
 *   The other two parameters, maxcolors and finalcolors, can be
 *   set small (~4) or large (~20).  When set large, @maxdist will
 *   be most influential in determining the actual number of colors.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const l_int32  MaxColors[] = {4, 8, 16};
static const l_int32  FinalColors[] = {4, 8, 16};

main(int    argc,
     char **argv)
{
char         namebuf[256];
l_int32      i, j, k, maxdist, maxcolors, selsize, finalcolors;
PIX         *pixs, *pixt, *pixd;
PIXA        *pixa;
static char  mainName[] = "colorseg_reg";

    if (argc != 1)
	exit(ERROR_INT("Syntax: colorseg_reg", mainName, 1));

    pixs = pixRead("tetons.jpg");

    for (k = 0; k < 3; k++) {
        maxcolors = MaxColors[k];
        finalcolors = FinalColors[k];
        pixa = pixaCreate(0);
        pixSaveTiled(pixs, pixa, 1, 1, 15, 32);
        for (i = 1; i <= 9; i++) {
            maxdist = 20 * i;
            for (j = 0; j <= 6; j++) {
                selsize = j;
                pixt = pixColorSegment(pixs, maxdist, maxcolors, selsize,
                                       finalcolors);
                pixSaveTiled(pixt, pixa, 1, j == 0 ? 1 : 0, 15, 32);
                pixDestroy(&pixt);
            }
        }

        pixd = pixaDisplay(pixa, 0, 0);
        pixDisplay(pixd, 100, 100);
        sprintf(namebuf, "/tmp/junkcolorseg%d.jpg", k);
        pixWrite(namebuf, pixd, IFF_JFIF_JPEG);
        pixDestroy(&pixd);
        pixaDestroy(&pixa);
    }

    pixDestroy(&pixs);
    return 0;
}


