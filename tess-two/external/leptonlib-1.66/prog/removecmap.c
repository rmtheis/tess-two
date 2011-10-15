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
 * removecmap.c
 *
 *      removecmap filein type fileout
 *
 *          type:  1 for conversion to 8 bpp gray
 *                 2 for conversion to 24 bpp full color
 *                 3 for conversion depending on src
 *
 *      Removes the colormap and does the conversion
 *      Works on palette images of 2, 4 and 8 bpp
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

main(int    argc,
     char **argv)
{
char        *filein, *fileout;
l_int32      d, type, numcolors;
PIX         *pixs, *pixd;
PIXCMAP     *cmap;
static char  mainName[] = "removecmap";

    if (argc != 4)
	exit(ERROR_INT("Syntax:  removecmap filein type fileout", mainName, 1));

    filein = argv[1];
    type = atoi(argv[2]);
    fileout = argv[3];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
	    
    fprintf(stderr, " depth = %d\n", pixGetDepth(pixs));
    if (cmap = pixGetColormap(pixs)) {
	numcolors = pixcmapGetCount(cmap);
	pixcmapWriteStream(stderr, cmap);
	fprintf(stderr, " colormap found; num colors = %d\n", numcolors);
    }
    else
	fprintf(stderr, " no colormap\n");

    pixd = pixRemoveColormap(pixs, type);
    pixWrite(fileout, pixd, IFF_PNG);

    return 0;
}

