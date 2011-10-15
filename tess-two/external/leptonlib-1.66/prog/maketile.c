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
 * maketile.c
 *
 *    Generates a single image tiling of all images of a specific depth
 *    in a directory.  The tiled images are scaled by a specified
 *    isotropic scale factor.  One can also specify the approximate width
 *    of the output image file, and the background color that is between
 *    the tiled images.
 *
 *    Input:  dirin:  directory that has image files
 *            depth (use 32 for RGB)
 *            scale factor
 *            width (approx. width of output tiled image)
 *            background (0 for white, 1 for black)
 *            fileout:  output tiled image file
 *    
 *    Note: this program is Unix only; it will not compile under cygwin.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
char        *dirin, *fileout, *fname, *fullname;
l_int32      depth, width, background, i, index, nfiles, n;
l_float32    scale;
SARRAY      *safiles;
PIX         *pix, *pixt, *pixd;
PIXA        *pixa;
static char  mainName[] = "maketile";

    if (argc != 7)
	exit(ERROR_INT(
	    "Syntax:  maketile dirin depth scale width background fileout",
	    mainName, 1));

    dirin = argv[1];
    depth = atoi(argv[2]);
    scale = atof(argv[3]);
    width = atoi(argv[4]);
    background = atoi(argv[5]);
    fileout = argv[6];

        /* capture the filenames in the input directory; ignore directories */
    if ((safiles = getFilenamesInDirectory(dirin)) == NULL)
	exit(ERROR_INT("safiles not made", mainName, 1));

    	/* capture images with the requisite depth */
    nfiles = sarrayGetCount(safiles);
    pixa = pixaCreate(nfiles);
    for (i = 0, index = 0; i < nfiles; i++) {
        fname = sarrayGetString(safiles, i, 0);
	fullname = genPathname(dirin, fname);
        pix = pixRead(fullname);
	FREE((void *)fullname);
        if (!pix)
            continue;
        if (pixGetDepth(pix) != depth) {
            pixDestroy(&pix);
            continue;
        }
	if (pixGetHeight(pix) > 5000) {
	    fprintf(stderr, "%s too tall\n", fname);
	    continue;
	}
        pixt = pixScale(pix, scale, scale);
        pixaAddPix(pixa, pixt, L_INSERT);
        pixDestroy(&pix);
/*        fprintf(stderr, "%d..", i); */
    }
    fprintf(stderr, "\n");

        /* tile them */
    pixd = pixaDisplayTiled(pixa, width, background, 15);

    if (depth < 8)
      pixWrite(fileout, pixd, IFF_PNG);
    else
      pixWrite(fileout, pixd, IFF_JFIF_JPEG);

    pixaDestroy(&pixa);
    pixDestroy(&pixd);
    sarrayDestroy(&safiles);
    return 0;
}

