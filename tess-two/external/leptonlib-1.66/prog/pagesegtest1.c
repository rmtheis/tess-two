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
 * pagesegtest1.c
 *
 *   Use on, e.g.:   feyn1.tif, witten.tif,
 *                   pageseg1.tif, pageseg1.tif, pageseg3.tif, pageseg4.tif
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
PIX         *pixs, *pixhm, *pixtm, *pixtb, *pixd;
PIXA        *pixa;
char        *filein;
static char  mainName[] = "pagesegtest1";

    if (argc != 2)
	exit(ERROR_INT(" Syntax:  pagesegtest1 filein", mainName, 1));

    filein = argv[1];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
	    
    pixGetRegionsBinary(pixs, &pixhm, &pixtm, &pixtb, 1);
    pixDestroy(&pixhm);
    pixDestroy(&pixtm);
    pixDestroy(&pixtb);
    pixDestroy(&pixs);

        /* Display intermediate images in a single image */
    pixa = pixaReadFiles(".", "junk_write");
    pixd = pixaDisplayTiledAndScaled(pixa, 32, 400, 4, 0, 20, 3);
    pixWrite("junkpixd", pixd, IFF_PNG);
    pixaDestroy(&pixa);
    pixDestroy(&pixd);

    exit(0);
}

