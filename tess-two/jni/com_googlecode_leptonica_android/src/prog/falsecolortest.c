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
 * falsecolortest.c
 */

#include <stdio.h>
#include <stdlib.h>

#include "allheaders.h"

#define   DEPTH     16    /* set to either 8 or 16 */
#define   WIDTH     768
#define   HEIGHT    100


main(int    argc,
     char **argv)
{
PIX            *pixs, *pixd, *pixt;
l_int32         i, j, maxval, val;
l_float32       gamma;
static char     mainName[] = "falsecolortest";

    if (argc != 2)
	exit(ERROR_INT(" Syntax:  falsecolortest gamma", mainName, 1));

    gamma = atof(argv[1]);
    maxval = 0xff;
    if (DEPTH == 16)
        maxval = 0xffff;

    pixs = pixCreate(WIDTH, HEIGHT, DEPTH);
    for (i = 0; i < HEIGHT; i++) {
        for (j = 0; j < WIDTH; j++) {
	    val = maxval * j / WIDTH;
	    pixSetPixel(pixs, j, i, val);
	}
    }
    fprintf(stderr, "before depth = %d\n", pixGetDepth(pixs));
    pixWrite("/tmp/junkout16.png", pixs, IFF_PNG);
    pixt = pixRead("/tmp/junkout16.png");
    pixWrite("/tmp/junkoutafter.png", pixt, IFF_PNG);
    fprintf(stderr, "after depth = %d\n", pixGetDepth(pixt));

    pixd = pixConvertGrayToFalseColor(pixt, gamma);
    pixDisplay(pixd, 50, 50);
    pixWrite("/tmp/junkout.png", pixd, IFF_PNG);
    pixDestroy(&pixs);
    pixDestroy(&pixt);
    pixDestroy(&pixd);
    return 0;
}

