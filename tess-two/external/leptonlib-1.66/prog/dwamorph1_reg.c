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
 * dwamorph1_reg.c
 *
 *     Fairly thorough regression test for autogen'd dwa.
 *
 *     The dwa code always implements safe closing.  With asymmetric
 *     b.c., the rasterop function must be pixCloseSafe().
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

    /* defined in morph.c */
LEPT_DLL extern l_int32 MORPH_BC;

    /* Complete set of linear brick dwa operations */
PIX *pixMorphDwa_3(PIX *pixd, PIX *pixs, l_int32 operation, char *selname);
PIX *pixFMorphopGen_3(PIX *pixd, PIX *pixs, l_int32 operation, char *selname);


main(int    argc,
     char **argv)
{
l_int32      i, nsels, same, xorcount, success, display;
char        *selname;
FILE        *fp;
PIX         *pixs, *pixs1, *pixt1, *pixt2, *pixt3;
SEL         *sel;
SELA        *sela;
static char  mainName[] = "dwamorph1_reg";

    if (regTestSetup(argc, argv, &fp, &display, &success, NULL))
        return 1;
    if ((pixs = pixRead("feyn-fract.tif")) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));

    sela = selaAddDwaLinear(NULL);
    nsels = selaGetCount(sela);
    success = TRUE;

    for (i = 0; i < nsels; i++)
    {
	sel = selaGetSel(sela, i);
	selname = selGetName(sel);

	    /*  ---------  dilation  ----------*/

	pixt1 = pixDilate(NULL, pixs, sel);
        pixt2 = pixMorphDwa_3(NULL, pixs, L_MORPH_DILATE, selname);
        pixEqual(pixt1, pixt2, &same);

	if (same == 1) {
	    fprintf(stderr, "dilations are identical for sel %d (%s)\n",
	            i, selname);
	}
	else {
            success = FALSE;
	    fprintf(fp, "dilations differ for sel %d (%s)\n", i, selname);
	    pixt3 = pixXor(NULL, pixt1, pixt2);
	    pixCountPixels(pixt3, &xorcount, NULL);
	    fprintf(fp, "Number of pixels in XOR: %d\n", xorcount);
            pixDestroy(&pixt3);
	}
	pixDestroy(&pixt1);
	pixDestroy(&pixt2);

	    /*  ---------  erosion with asymmetric b.c  ----------*/

        resetMorphBoundaryCondition(ASYMMETRIC_MORPH_BC);
        fprintf(stderr, "MORPH_BC = %d ... ", MORPH_BC);

	pixt1 = pixErode(NULL, pixs, sel);
        pixt2 = pixMorphDwa_3(NULL, pixs, L_MORPH_ERODE, selname);
        pixEqual(pixt1, pixt2, &same);

	if (same == 1) {
	    fprintf(stderr, "erosions are identical for sel %d (%s)\n",
	            i, selname);
	}
	else {
            success = FALSE;
	    fprintf(fp, "erosions differ for sel %d (%s)\n", i, selname);
	    pixt3 = pixXor(NULL, pixt1, pixt2);
	    pixCountPixels(pixt3, &xorcount, NULL);
	    fprintf(fp, "Number of pixels in XOR: %d\n", xorcount);
            pixDestroy(&pixt3);
	}
	pixDestroy(&pixt1);
	pixDestroy(&pixt2);

	    /*  ---------  erosion with symmetric b.c  ----------*/

        resetMorphBoundaryCondition(SYMMETRIC_MORPH_BC);
        fprintf(stderr, "MORPH_BC = %d ... ", MORPH_BC);

	pixt1 = pixErode(NULL, pixs, sel);
        pixt2 = pixMorphDwa_3(NULL, pixs, L_MORPH_ERODE, selname);
        pixEqual(pixt1, pixt2, &same);

	if (same == 1) {
	    fprintf(stderr, "erosions are identical for sel %d (%s)\n",
	            i, selname);
	}
	else {
            success = FALSE;
	    fprintf(fp, "erosions differ for sel %d (%s)\n", i, selname);
	    pixt3 = pixXor(NULL, pixt1, pixt2);
	    pixCountPixels(pixt3, &xorcount, NULL);
	    fprintf(fp, "Number of pixels in XOR: %d\n", xorcount);
            pixDestroy(&pixt3);
	}
	pixDestroy(&pixt1);
	pixDestroy(&pixt2);

	    /*  ---------  opening with asymmetric b.c  ----------*/

        resetMorphBoundaryCondition(ASYMMETRIC_MORPH_BC);
        fprintf(stderr, "MORPH_BC = %d ... ", MORPH_BC);

	pixt1 = pixOpen(NULL, pixs, sel);
        pixt2 = pixMorphDwa_3(NULL, pixs, L_MORPH_OPEN, selname);
        pixEqual(pixt1, pixt2, &same);

	if (same == 1) {
	    fprintf(stderr, "openings are identical for sel %d (%s)\n",
	            i, selname);
	}
	else {
            success = FALSE;
	    fprintf(fp, "openings differ for sel %d (%s)\n", i, selname);
	    pixt3 = pixXor(NULL, pixt1, pixt2);
	    pixCountPixels(pixt3, &xorcount, NULL);
	    fprintf(fp, "Number of pixels in XOR: %d\n", xorcount);
            pixDestroy(&pixt3);
	}
	pixDestroy(&pixt1);
	pixDestroy(&pixt2);

	    /*  ---------  opening with symmetric b.c  ----------*/

        resetMorphBoundaryCondition(SYMMETRIC_MORPH_BC);
        fprintf(stderr, "MORPH_BC = %d ... ", MORPH_BC);

	pixt1 = pixOpen(NULL, pixs, sel);
        pixt2 = pixMorphDwa_3(NULL, pixs, L_MORPH_OPEN, selname);
        pixEqual(pixt1, pixt2, &same);

	if (same == 1) {
	    fprintf(stderr, "openings are identical for sel %d (%s)\n",
	            i, selname);
	}
	else {
            success = FALSE;
	    fprintf(fp, "openings differ for sel %d (%s)\n", i, selname);
	    pixt3 = pixXor(NULL, pixt1, pixt2);
	    pixCountPixels(pixt3, &xorcount, NULL);
	    fprintf(fp, "Number of pixels in XOR: %d\n", xorcount);
            pixDestroy(&pixt3);
	}
	pixDestroy(&pixt1);
	pixDestroy(&pixt2);

	    /*  ---------  safe closing with asymmetric b.c  ----------*/

        resetMorphBoundaryCondition(ASYMMETRIC_MORPH_BC);
        fprintf(stderr, "MORPH_BC = %d ... ", MORPH_BC);

	pixt1 = pixCloseSafe(NULL, pixs, sel);  /* must use safe version */
        pixt2 = pixMorphDwa_3(NULL, pixs, L_MORPH_CLOSE, selname);
        pixEqual(pixt1, pixt2, &same);

	if (same == 1) {
	    fprintf(stderr, "closings are identical for sel %d (%s)\n",
	            i, selname);
	}
	else {
            success = FALSE;
	    fprintf(fp, "closings differ for sel %d (%s)\n", i, selname);
	    pixt3 = pixXor(NULL, pixt1, pixt2);
	    pixCountPixels(pixt3, &xorcount, NULL);
	    fprintf(fp, "Number of pixels in XOR: %d\n", xorcount);
            pixDestroy(&pixt3);
	}
	pixDestroy(&pixt1);
	pixDestroy(&pixt2);

	    /*  ---------  safe closing with symmetric b.c  ----------*/

        resetMorphBoundaryCondition(SYMMETRIC_MORPH_BC);
        fprintf(stderr, "MORPH_BC = %d ... ", MORPH_BC);

	pixt1 = pixClose(NULL, pixs, sel);  /* safe version not required */
        pixt2 = pixMorphDwa_3(NULL, pixs, L_MORPH_CLOSE, selname);
        pixEqual(pixt1, pixt2, &same);

	if (same == 1) {
	    fprintf(stderr, "closings are identical for sel %d (%s)\n",
	            i, selname);
	}
	else {
            success = FALSE;
	    fprintf(fp, "closings differ for sel %d (%s)\n", i, selname);
	    pixt3 = pixXor(NULL, pixt1, pixt2);
	    pixCountPixels(pixt3, &xorcount, NULL);
	    fprintf(fp, "Number of pixels in XOR: %d\n", xorcount);
            pixDestroy(&pixt3);
	}
	pixDestroy(&pixt1);
	pixDestroy(&pixt2);
    }

    selaDestroy(&sela);
    pixDestroy(&pixs);
    regTestCleanup(argc, argv, fp, success, NULL);
    return 0;
}


