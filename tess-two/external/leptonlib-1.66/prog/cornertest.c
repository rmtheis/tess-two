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
 * cornertest.c
 *
 *   e.g., use on witten.tif
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define   LINE_SIZE   9


main(int    argc,
     char **argv)
{
char        *filein, *fileout;
l_int32      x, y, n, i;
PIX         *pixs;
PTA         *pta;
PTAA        *ptaa, *ptaa2, *ptaa3;
static char  mainName[] = "cornertest";

    if (argc != 3)
	exit(ERROR_INT(" Syntax:  cornertest filein fileout", mainName, 1));

    filein = argv[1];
    fileout = argv[2];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));

	/* Clean noise in LR corner of witten.tif */
    pixSetPixel(pixs, 2252, 3051, 0);
    pixSetPixel(pixs, 2252, 3050, 0);
    pixSetPixel(pixs, 2251, 3050, 0);
	    
    pta = pixFindCornerPixels(pixs);
    ptaWriteStream(stderr, pta, 1);

        /* Test pta and ptaa I/O */
#if 1
    ptaa = ptaaCreate(3);
    ptaaAddPta(ptaa, pta, L_COPY);
    ptaaAddPta(ptaa, pta, L_COPY);
    ptaaAddPta(ptaa, pta, L_COPY);
    ptaaWriteStream(stderr, ptaa, 1);
    ptaaWrite("/tmp/junkptaa", ptaa, 1);
    ptaa2 = ptaaRead("/tmp/junkptaa");
    ptaaWrite("/tmp/junkptaa2", ptaa2, 1);
    ptaaWrite("/tmp/junkptaa3", ptaa, 0);
    ptaa3 = ptaaRead("/tmp/junkptaa3");
    ptaaWrite("/tmp/junkptaa4", ptaa3, 0);
    ptaaDestroy(&ptaa);
    ptaaDestroy(&ptaa2);
    ptaaDestroy(&ptaa3);
#endif

	/* mark corner pixels */
    n = ptaGetCount(pta);
    for (i = 0; i < n; i++) {
	ptaGetIPt(pta, i, &x, &y);
	pixRenderLine(pixs, x - LINE_SIZE, y, x + LINE_SIZE, y, 3,
	              L_FLIP_PIXELS);
	pixRenderLine(pixs, x, y - LINE_SIZE, x, y + LINE_SIZE, 3,
	              L_FLIP_PIXELS);
    }

    pixWrite(fileout, pixs, IFF_PNG);
    
    pixDestroy(&pixs);
    ptaDestroy(&pta);
    ptaDestroy(&pta);

    exit(0);
}

