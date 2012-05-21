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
 * numaranktest.c
 *
 *     test on 8 bpp grayscale (e.g., w91frag.jpg)
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const l_int32   BIN_SIZE = 1;

main(int    argc,
     char **argv)
{
char        *filein;
l_int32      i, j, w, h, d, sampling;
l_float32    rank, rval;
l_uint32     val;
NUMA        *na, *nah, *nar, *nav;
PIX         *pix;
static char  mainName[] = "numaranktest";

    if (argc != 3)
	exit(ERROR_INT(" Syntax:  numaranktest filein sampling", mainName, 1));

    filein = argv[1];
    sampling = atoi(argv[2]);

    if ((pix = pixRead(filein)) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 8)
	return ERROR_INT("d != 8 bpp", mainName, 1);

    na = numaCreate(0);
    for (i = 0; i < h; i += sampling) {
        for (j = 0; j < w; j += sampling) {
	    pixGetPixel(pix, j, i, &val);
	    numaAddNumber(na, val);
	}
    }
    nah = numaMakeHistogramClipped(na, BIN_SIZE, 255);

    nar = numaCreate(0);
    for (rval = 0.0; rval < 256.0; rval += 2.56) {
        numaHistogramGetRankFromVal(nah, rval, &rank);
	numaAddNumber(nar, rank);
    }
    gplotSimple1(nar, GPLOT_X11, "/tmp/junkroot1", "rank vs val");

    nav = numaCreate(0);
    for (rank = 0.0; rank <= 1.0; rank += 0.01) {
        numaHistogramGetValFromRank(nah, rank, &rval);
	numaAddNumber(nav, rval);
    }
    gplotSimple1(nav, GPLOT_X11, "/tmp/junkroot2", "val vs rank");

    pixDestroy(&pix);
    numaDestroy(&na);
    numaDestroy(&nah);
    numaDestroy(&nar);
    numaDestroy(&nav);
    return 0;
}



