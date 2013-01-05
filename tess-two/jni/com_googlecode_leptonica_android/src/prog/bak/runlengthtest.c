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
 * runlengthtest.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

main(int    argc,
     char **argv)
{
PIX         *pixs, *pixh, *pixv, *pix, *pixd;
char        *filein, *fileout;
static char  mainName[] = "runlengthtest";

    if (argc != 3)
	exit(ERROR_INT(" Syntax:  runlengthtest filein fileout", mainName, 1));

    filein = argv[1];
    fileout = argv[2];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
	    
    startTimer();
    pixh = pixRunlengthTransform(pixs, 0, L_HORIZONTAL_RUNS, 8);
    pixv = pixRunlengthTransform(pixs, 0, L_VERTICAL_RUNS, 8);
    pix = pixMinOrMax(NULL, pixh, pixv, L_CHOOSE_MAX);
    pixd = pixMaxDynamicRange(pix, L_LINEAR_SCALE);
    fprintf(stderr, "Total time: %7.3f sec\n", stopTimer());
    pixDisplay(pixh, 0, 0);
    pixDisplay(pixv, 400, 0);
    pixDisplay(pix, 800, 0);
    pixDisplay(pixd, 1200, 0);
    pixWrite("/tmp/junkpixh.png", pixh, IFF_PNG);
    pixWrite("/tmp/junkpixv.png", pixv, IFF_PNG);
    pixWrite("/tmp/junkpix.png", pix, IFF_PNG);
    pixWrite(fileout, pixd, IFF_PNG);

    pixDestroy(&pixs);
    pixDestroy(&pixh);
    pixDestroy(&pixv);
    pixDestroy(&pix);
    pixDestroy(&pixd);
    exit(0);
}

