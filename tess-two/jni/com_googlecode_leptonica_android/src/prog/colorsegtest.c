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
 * colorsegtest.c
 *
 *   See colorseg.c for details.
 *
 *   Just for fun, try these combinations of the 4 parameters below on
 *   the image tetons.jpg:
 *      30 20 5 10    (20 colors)
 *      40 20 7 15    (19 colors)
 *      50 12 5 12    (12 colors)
 *      50 12 3 12    (12 colors)
 *      30 13 3 13    (12 colors)
 *      30 20 3 20    (20 colors)
 *      15 20 5 15    (19 colors)
 *      80 20 3 20    (12 colors)
 *      100 15 5 15   (7 colors)
 *      100 15 2 15   (7 colors)
 *      100 15 0 15   (7 colors)
 *      30 15 0 15    (12 colors)
 *      150 15 0 15   (4 colors)
 *      150 15 2 15   (4 colors)
 *      180 6 2 6     (3 colors)
 *      180 6 0 6     (3 colors)
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const l_int32    MAX_DIST      = 120;
static const l_int32    MAX_COLORS    = 15;
static const l_int32    SEL_SIZE      = 4;
static const l_int32    FINAL_COLORS  = 15;


main(int    argc,
     char **argv)
{
l_int32      max_dist, max_colors, sel_size, final_colors;
PIX         *pixs, *pixd, *pixt;
char        *filein, *fileout;
static char  mainName[] = "colorsegtest";

    if (argc != 3 && argc != 7)
	exit(ERROR_INT(
            "Syntax: colorsegtest filein fileout"
            " [max_dist max_colors sel_size final_colors]\n"
            " Default values are: max_dist = 120\n"
            "                     max_colors = 15\n"
            "                     sel_size = 4\n"
            "                     final_colors = 15\n", mainName, 1));

    filein = argv[1];
    fileout = argv[2];
    if (argc == 3) {  /* use default values */
        max_dist = MAX_DIST;
        max_colors = MAX_COLORS;
        sel_size = SEL_SIZE;
        final_colors = FINAL_COLORS;
    }
    else {  /* 6 input args */
        max_dist = atoi(argv[3]);
        max_colors = atoi(argv[4]);
        sel_size = atoi(argv[5]);
        final_colors = atoi(argv[6]);
    }

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
	    
    startTimer();
    pixt = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    pixd = pixColorSegment(pixt, max_dist, max_colors, sel_size, final_colors);
    fprintf(stderr, "Time to segment: %7.3f sec\n", stopTimer());
    pixWrite(fileout, pixd, IFF_PNG);

    pixDestroy(&pixs);
    pixDestroy(&pixd);
    return 0;
}

