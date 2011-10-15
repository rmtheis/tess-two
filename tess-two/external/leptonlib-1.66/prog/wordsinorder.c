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
 * wordsinorder.c
 *
 *     wordsinorder dirin rootname [firstpage npages]
 *
 *         dirin:  directory of input pages
 *         rootname: used for naming the two output files (templates
 *                   and c.c. data)
 *         firstpage: <optional> 0-based; default is 0
 *         npages: <optional> use 0 for all pages; default is 0
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

    /* Input variables */
static const l_int32  MIN_WORD_WIDTH = 6;
static const l_int32  MIN_WORD_HEIGHT = 4;
static const l_int32  MAX_WORD_WIDTH = 500;
static const l_int32  MAX_WORD_HEIGHT = 100;


#define   BUF_SIZE                  512
#define   RENDER_PAGES              1


main(int    argc,
     char **argv)
{
char         filename[BUF_SIZE];
char        *dirin, *rootname, *fname;
l_int32      i, j, w, h, firstpage, npages, nfiles, ncomp;
l_int32      index, ival, rval, gval, bval;
BOX         *box;
BOXA        *boxa;
BOXAA       *baa;
JBDATA      *data;
JBCLASSER   *classer;
NUMA        *nai;
NUMAA       *naa;
SARRAY      *safiles;
PIX         *pixs, *pixt1, *pixt2, *pixd;
PIXCMAP     *cmap;
static char  mainName[] = "wordsinorder";

    if (argc != 3 && argc != 5)
	exit(ERROR_INT(
	     " Syntax: wordsinorder dirin rootname [firstpage, npages]",
	     mainName, 1));

    dirin = argv[1];
    rootname = argv[2];

    if (argc == 3) {
        firstpage = 0;
	npages = 0;
    }
    else {
        firstpage = atoi(argv[3]);
        npages = atoi(argv[4]);
    }

        /* Compute the word bounding boxes at 2x reduction, along with
         * the textlines that they are in. */
    safiles = getSortedPathnamesInDirectory(dirin, NULL, firstpage, npages);
    nfiles = sarrayGetCount(safiles);
    baa = boxaaCreate(nfiles);
    naa = numaaCreate(nfiles);
    for (i = 0; i < nfiles; i++) {
        fname = sarrayGetString(safiles, i, 0);
        if ((pixs = pixRead(fname)) == NULL) {
            L_WARNING_INT("image file %d not read", mainName, i);
            continue;
        }
        pixGetWordBoxesInTextlines(pixs, 2, MIN_WORD_WIDTH, MIN_WORD_HEIGHT,
			           MAX_WORD_WIDTH, MAX_WORD_HEIGHT,
				   &boxa, &nai);
        boxaaAddBoxa(baa, boxa, L_INSERT);
        numaaAddNuma(naa, nai, L_INSERT);
        
#if  RENDER_PAGES
            /* Show the results on a 2x reduced image, where each
             * word is outlined and the color of the box depends on the
             * computed textline. */
	pixt1 = pixReduceRankBinary2(pixs, 2, NULL);
        pixGetDimensions(pixt1, &w, &h, NULL);
        pixd = pixCreate(w, h, 8);
        cmap = pixcmapCreateRandom(8, 1, 1);  /* first color is black */
        pixSetColormap(pixd, cmap);

        pixt2 = pixUnpackBinary(pixt1, 8, 1);
        pixRasterop(pixd, 0, 0, w, h, PIX_SRC | PIX_DST, pixt2, 0, 0);
        ncomp = boxaGetCount(boxa);
        for (j = 0; j < ncomp; j++) {
            box = boxaGetBox(boxa, j, L_CLONE);
            numaGetIValue(nai, j, &ival);
            index = 1 + (ival % 254);  /* omit black and white */
            pixcmapGetColor(cmap, index, &rval, &gval, &bval);
            pixRenderBoxArb(pixd, box, 2, rval, gval, bval);
            boxDestroy(&box);
        }

	snprintf(filename, BUF_SIZE, "%s.%05d", rootname, i);
	fprintf(stderr, "filename: %s\n", filename);
	pixWrite(filename, pixd, IFF_PNG);
	pixDestroy(&pixt1);
	pixDestroy(&pixt2);
	pixDestroy(&pixs);
	pixDestroy(&pixd);
#endif  /* RENDER_PAGES */
    }

    boxaaDestroy(&baa);
    numaaDestroy(&naa);
    sarrayDestroy(&safiles);
    return 0;
}

