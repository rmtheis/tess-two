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
 * conncomp_reg.c
 *
 *        Regression test for connected components (both 4 and 8
 *        connected), including regeneration of the original
 *        image from the components.  This is also an implicit
 *        test of rasterop.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

#define  NTIMES             10


main(int    argc,
     char **argv)
{
l_uint8     *array1, *array2;
l_int32      i, n, np, same, diff, nbytes1, nbytes2;
FILE        *fp;
BOX         *box;
BOXA        *boxa, *boxa2;
PIX         *pixs, *pixd;
PIXA        *pixa;
PIXCMAP     *cmap;
static char  mainName[] = "conncomp_reg";

    if (argc != 1)
	exit(ERROR_INT(" Syntax: conncomp_reg", mainName, 1));

    if ((pixs = pixRead("feyn.tif")) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
	    
	/* Test pixConnComp() with output to both boxa and pixa */
	/* First, test with 4-cc */
    boxa = pixConnComp(pixs, &pixa, 4);
    n = boxaGetCount(boxa);
    fprintf(stderr, "Number of 4 c.c. b.b: %d\n", n);
    np = pixaGetCount(pixa);
    fprintf(stderr, "Number of 4 c.c. pix: %d\n", np);
    pixd = pixaDisplay(pixa, pixGetWidth(pixs), pixGetHeight(pixs));
    pixWrite("/tmp/junkout1.png", pixd, IFF_PNG);
    pixEqual(pixs, pixd, &same);
    if (same == 1)
	fprintf(stderr, "Source and reconstructed pix are the same.\n");
    else
	fprintf(stderr, "Error: source and reconstructed pix differ!\n");
    pixaDestroy(&pixa);
    boxaDestroy(&boxa);
    pixDestroy(&pixd);

	/* Test with 8-cc */
    boxa = pixConnComp(pixs, &pixa, 8);
    n = boxaGetCount(boxa);
    fprintf(stderr, "Number of 8 c.c. b.b: %d\n", n);
    np = pixaGetCount(pixa);
    fprintf(stderr, "Number of 8 c.c. pix: %d\n", np);
    pixd = pixaDisplay(pixa, pixGetWidth(pixs), pixGetHeight(pixs));
    pixWrite("/tmp/junkout2.png", pixd, IFF_PNG);
    pixEqual(pixs, pixd, &same);
    if (same == 1)
	fprintf(stderr, "Source and reconstructed pix are the same.\n");
    else
	fprintf(stderr, "Error: source and reconstructed pix differ!\n");
    pixaDestroy(&pixa);
    boxaDestroy(&boxa);
    pixDestroy(&pixd);

	/* Test i/o */
    boxa = pixConnComp(pixs, NULL, 4);
    fp = fopen("/tmp/junk1.ba", "wb+");
    boxaWriteStream(fp, boxa);
    fclose(fp);
    fp = fopen("/tmp/junk1.ba", "r");
    boxa2 = boxaReadStream(fp);
    fclose(fp);
    fp = fopen("/tmp/junk2.ba", "wb+");
    boxaWriteStream(fp, boxa2);
    fclose(fp);
    array1 = arrayRead("/tmp/junk1.ba", &nbytes1);
    array2 = arrayRead("/tmp/junk2.ba", &nbytes2);
    diff = strcmp((char *)array1, (char *)array2);
    if (nbytes1 != nbytes2 || diff)
	fprintf(stderr, "I/O error for boxes.\n");
    else
	fprintf(stderr, "I/O valid for boxes.\n");
    FREE(array1);
    FREE(array2);
    boxaDestroy(&boxa);
    boxaDestroy(&boxa2);

        /* Just for fun, display each component as a random color
	 * in cmapped 8 bpp.  Background is color 0; it is set to white. */
    boxa = pixConnComp(pixs, &pixa, 4);
    pixd = pixaDisplayRandomCmap(pixa, pixGetWidth(pixs), pixGetHeight(pixs));
    cmap = pixGetColormap(pixd);
    pixcmapResetColor(cmap, 0, 255, 255, 255);  /* reset background to white */
    pixDisplay(pixd, 100, 100);
    boxaDestroy(&boxa);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixDestroy(&pixs);

    exit(0);
}


