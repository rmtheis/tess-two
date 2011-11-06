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
 * fhmtauto_reg.c
 *
 *    Basic regression test for hit-miss transform: rasterops & dwa.
 *
 *    Tests hmt from a set of hmt structuring elements
 *    by comparing the full image rasterop results with the
 *    automatically generated dwa results.
 *
 *    Results must be identical for all operations.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
l_int32      i, nsels, same1, same2, ok;
char        *filein, *selname;
PIX         *pixs, *pixref, *pixt1, *pixt2, *pixt3, *pixt4;
SEL         *sel;
SELA        *sela;
static char  mainName[] = "fhmtauto_reg";

    if (argc != 2)
	exit(ERROR_INT(" Syntax:  fhmtauto_reg filein", mainName, 1));

    filein = argv[1];
    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));

    sela = selaAddHitMiss(NULL);
    nsels = selaGetCount(sela);

    ok = TRUE;
    for (i = 0; i < nsels; i++)
    {
	sel = selaGetSel(sela, i);
	selname = selGetName(sel);

	pixref = pixHMT(NULL, pixs, sel);

	pixt1 = pixAddBorder(pixs, 32, 0);
	pixt2 = pixFHMTGen_1(NULL, pixt1, selname);
	pixt3 = pixRemoveBorder(pixt2, 32);
       
	pixt4 = pixHMTDwa_1(NULL, pixs, selname);

	pixEqual(pixref, pixt3, &same1);
	pixEqual(pixref, pixt4, &same2);
	if (same1 && same2)
	    fprintf(stderr, "hmt are identical for sel %d (%s)\n", i, selname);
	else {
	    fprintf(stderr, "hmt differ for sel %d (%s)\n", i, selname);
	    ok = FALSE;
	}

	pixDestroy(&pixref);
	pixDestroy(&pixt1);
	pixDestroy(&pixt2);
	pixDestroy(&pixt3);
	pixDestroy(&pixt4);
    }

    if (ok)
        fprintf(stderr, "\n ********  All hmt are correct *******\n");
    else
        fprintf(stderr, "\n ********  ERROR in at least one hmt *******\n");

    pixDestroy(&pixs);
    selaDestroy(&sela);
    exit(0);
}

