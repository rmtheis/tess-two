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
 * jbwords.c
 *
 *     jbwords dirin thresh weight rootname [firstpage npages]
 *
 *         dirin:  directory of input pages
 *         reduction: 1 (full res) or 2 (half-res)
 *         thresh: 0.80 is a reasonable compromise between accuracy
 *                 and number of classes, for characters
 *         weight: 0.6 seems to work reasonably with thresh = 0.8.
 *         rootname: used for naming the two output files (templates
 *                   and c.c. data)
 *         firstpage: <optional> 0-based; default is 0
 *         npages: <optional> use 0 for all pages; default is 0
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

    /* Eliminate very large "words" */
static const l_int32  MAX_WORD_WIDTH = 500;
static const l_int32  MAX_WORD_HEIGHT = 200;

#define   BUF_SIZE                  512

    /* select additional debug output */
#define   RENDER_PAGES              1
#define   RENDER_DEBUG              1


main(int    argc,
     char **argv)
{
char         filename[BUF_SIZE];
char        *dirin, *rootname, *fname;
l_int32      reduction, i, firstpage, npages, nfiles;
l_float32    thresh, weight;
JBDATA      *data;
JBCLASSER   *classer;
NUMA        *natl;
SARRAY      *safiles;
PIX         *pix;
PIXA        *pixa, *pixadb;
static char  mainName[] = "jbwords";

    if (argc != 6 && argc != 8)
	exit(ERROR_INT(
 " Syntax: jbwords dirin reduction thresh weight rootname [firstpage, npages]",
	     mainName, 1));

    dirin = argv[1];
    reduction = atoi(argv[2]);
    thresh = atof(argv[3]);
    weight = atof(argv[4]);
    rootname = argv[5];

    if (argc == 6) {
        firstpage = 0;
	npages = 0;
    }
    else {
        firstpage = atoi(argv[6]);
        npages = atoi(argv[7]);
    }

    classer = jbWordsInTextlines(dirin, reduction, MAX_WORD_WIDTH,
                                 MAX_WORD_HEIGHT, thresh, weight,
                                 &natl, firstpage, npages);

        /* Save and write out the result */
    data = jbDataSave(classer);
    jbDataWrite(rootname, data);

#if  RENDER_PAGES
        /* Render the pages from the classifier data, and write to file.
	 * Use debugflag == FALSE to omit outlines of each component. */
    pixa = jbDataRender(data, FALSE);
    npages = pixaGetCount(pixa);
    for (i = 0; i < npages; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
	snprintf(filename, BUF_SIZE, "%s.%05d", rootname, i);
	fprintf(stderr, "filename: %s\n", filename);
	pixWrite(filename, pix, IFF_PNG);
	pixDestroy(&pix);
    }
    pixaDestroy(&pixa);
#endif  /* RENDER_PAGES */

#if  RENDER_DEBUG
	/* Use debugflag == TRUE to see outlines of each component. */
    pixadb = jbDataRender(data, TRUE);
        /* Write the debug pages out */
    npages = pixaGetCount(pixadb);
    for (i = 0; i < npages; i++) {
        pix = pixaGetPix(pixadb, i, L_CLONE);
	snprintf(filename, BUF_SIZE, "%s.db.%05d", rootname, i);
	fprintf(stderr, "filename: %s\n", filename);
	pixWrite(filename, pix, IFF_PNG);
	pixDestroy(&pix);
    }
    pixaDestroy(&pixadb);
#endif  /* RENDER_DEBUG */

    jbClasserDestroy(&classer);
    jbDataDestroy(&data);
    numaDestroy(&natl);

    exit(0);
}

