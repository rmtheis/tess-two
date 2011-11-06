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
 * scaleandtile.c
 *
 *    Generates a single image tiling of all images in a directory
 *    whose filename contains a given substring.  The filenames
 *    are filtered and sorted, and read into a pixa, which is
 *    then tiled into a pix at a specified depth, and finally
 *    written out to file.
 *
 *    Input:  dirin:  directory that has image files
 *            depth (output depth: 1, 8 or 32; use 32 for RGB)
 *            width (of each tile; all pix are scaled to the same width)
 *            ncols (number of tiles in each row)
 *            background (0 for white, 1 for black)
 *            fileout:  output tiled image file
 *    
 *    Note: this program is Unix only; it will not compile under cygwin.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

    /* Change these and recompile if necessary */
static const l_int32  BACKGROUND_COLOR = 0;
static const l_int32  SPACING = 25;  /* between images and on outside */
static const l_int32  BLACK_BORDER = 2;  /* surrounding each image */


main(int    argc,
     char **argv)
{
char        *dirin, *substr, *fileout;
l_int32      depth, width, ncols;
PIX         *pixd;
PIXA        *pixa;
static char  mainName[] = "scaleandtile";

    if (argc != 7)
	return ERROR_INT(
	    "Syntax:  scaleandtile dirin substr depth width ncols fileout",
	    mainName, 1);

    dirin = argv[1];
    substr = argv[2];
    depth = atoi(argv[3]);
    width = atoi(argv[4]);
    ncols = atoi(argv[5]);
    fileout = argv[6];

        /* Read the specified images from file */
    if ((pixa = pixaReadFiles(dirin, substr)) == NULL)
	return ERROR_INT("safiles not made", mainName, 1);
    fprintf(stderr, "Number of pix: %d\n", pixaGetCount(pixa));

    	/* Tile them */
    pixd = pixaDisplayTiledAndScaled(pixa, depth, width, ncols,
                                     BACKGROUND_COLOR, SPACING, BLACK_BORDER);

    if (depth < 8)
        pixWrite(fileout, pixd, IFF_PNG);
    else
        pixWrite(fileout, pixd, IFF_JFIF_JPEG);

    pixaDestroy(&pixa);
    pixDestroy(&pixd);
    return 0;
}


