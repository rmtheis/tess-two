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
 * sorttest.c
 *
 *   Tests sorting of connected components by various attributes,
 *   in increasing or decreasing order.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
char        *filein;
l_int32      i, n, ns;
BOX         *box;
BOXA        *boxa, *boxas;
PIX         *pixs, *pixt;
PIXA        *pixa, *pixas, *pixas2;
static char  mainName[] = "sorttest";

    if (argc != 2)
	exit(ERROR_INT(" Syntax:  sorttest filein", mainName, 1));

    filein = argv[1];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
	    
#if 0
    boxa = pixConnComp(pixs, NULL, 8);
    n = boxaGetCount(boxa);

    boxas = boxaSort(boxa, L_SORT_BY_PERIMETER, L_SORT_DECREASING, NULL);
    ns = boxaGetCount(boxas);
    fprintf(stderr, "Number of cc: n = %d, ns = %d\n", n, ns);
    boxaWrite("/tmp/junkboxa.ba", boxas);

    for (i = 0; i < n; i++) {
	box = boxaGetBox(boxas, i, L_CLONE);
	pixRenderBox(pixs, box, 2, L_FLIP_PIXELS);
	boxDestroy(&box); 
    }
    pixWrite("/tmp/junkout.png", pixs, IFF_PNG);
    boxaDestroy(&boxa);
    boxaDestroy(&boxas);
#endif


#if 1
    boxa = pixConnComp(pixs, &pixa, 8);
    n = pixaGetCount(pixa);

    pixas = pixaSort(pixa, L_SORT_BY_Y, L_SORT_INCREASING, NULL, L_CLONE);
    ns = pixaGetCount(pixas);
    fprintf(stderr, "Number of cc: n = %d, ns = %d\n", n, ns);
    pixaWrite("/tmp/junkpixa.pa", pixas);
    pixas2 = pixaRead("/tmp/junkpixa.pa");
    pixaWrite("/tmp/junkpixa2.pa", pixas2);

    pixt = pixaDisplayOnLattice(pixas, 100, 100);
    pixWrite("/tmp/junkpix.png", pixt, IFF_PNG);
    boxaWrite("/tmp/junkboxa.ba", pixas->boxa);
    pixDestroy(&pixt);
    pixaDestroy(&pixa);
    pixaDestroy(&pixas);
    pixaDestroy(&pixas2);
    boxaDestroy(&boxa);
#endif

    pixDestroy(&pixs);
    exit(0);
}


