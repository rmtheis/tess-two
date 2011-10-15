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
 * binmorph2_reg.c
 * 
 *    Thorough regression test for binary separable rasterops,
 *    using the sequence interpreters.  This compares the
 *    results for 2-way composite Sels with unitary Sels,
 *    all invoked on the separable block morph ops.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const l_int32  MAX_SEL_SIZE = 120;

static void writeResult(char *sequence, l_int32 same);


main(int    argc,
     char **argv)
{
char        *str;
char         buffer1[256];
char         buffer2[256];
l_int32      i, same, same2, factor1, factor2, diff, success;
PIX         *pixs, *pixsd, *pixt1, *pixt2, *pixt3;
SEL         *sel1, *sel2;
static char  mainName[] = "binmorph2_reg";

#if 1
    pixs = pixRead("rabi.png");
    pixsd = pixMorphCompSequence(pixs, "d5.5", 0);
    success = TRUE;
    for (i = 1; i < MAX_SEL_SIZE; i++) {
      
            /* Check if the size is exactly decomposable */
        selectComposableSizes(i, &factor1, &factor2);
        diff = factor1 * factor2 - i;
        fprintf(stderr, "%d: (%d, %d): %d\n", i, factor1, factor2, diff);

	    /* Carry out operations on identical sized Sels: dilation */
        sprintf(buffer1, "d%d.%d", i + diff, i + diff);
        sprintf(buffer2, "d%d.%d", i, i);
        pixt1 = pixMorphSequence(pixsd, buffer1, 0);
        pixt2 = pixMorphCompSequence(pixsd, buffer2, 0);
        pixEqual(pixt1, pixt2, &same);
	if (i < 64) {
            pixt3 = pixMorphCompSequenceDwa(pixsd, buffer2, 0);
            pixEqual(pixt1, pixt3, &same2);
	} else {
            pixt3 = NULL;
	    same2 = TRUE;
	}
        if (same && same2)
            writeResult(buffer1, 1);
        else {
            writeResult(buffer1, 0);
            success = FALSE;
        }
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixt3);

	    /* ... erosion */
        sprintf(buffer1, "e%d.%d", i + diff, i + diff);
        sprintf(buffer2, "e%d.%d", i, i);
        pixt1 = pixMorphSequence(pixsd, buffer1, 0);
        pixt2 = pixMorphCompSequence(pixsd, buffer2, 0);
        pixEqual(pixt1, pixt2, &same);
	if (i < 64) {
            pixt3 = pixMorphCompSequenceDwa(pixsd, buffer2, 0);
            pixEqual(pixt1, pixt3, &same2);
	} else {
            pixt3 = NULL;
	    same2 = TRUE;
	}
        if (same && same2)
            writeResult(buffer1, 1);
        else {
            writeResult(buffer1, 0);
            success = FALSE;
        }
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixt3);

	    /* ... opening */
        sprintf(buffer1, "o%d.%d", i + diff, i + diff);
        sprintf(buffer2, "o%d.%d", i, i);
        pixt1 = pixMorphSequence(pixsd, buffer1, 0);
        pixt2 = pixMorphCompSequence(pixsd, buffer2, 0);
        pixEqual(pixt1, pixt2, &same);
	if (i < 64) {
            pixt3 = pixMorphCompSequenceDwa(pixsd, buffer2, 0);
            pixEqual(pixt1, pixt3, &same2);
	} else {
            pixt3 = NULL;
	    same2 = TRUE;
	}
        if (same && same2)
            writeResult(buffer1, 1);
        else {
            writeResult(buffer1, 0);
            success = FALSE;
        }
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixt3);

            /* ... closing */
        sprintf(buffer1, "c%d.%d", i + diff, i + diff);
        sprintf(buffer2, "c%d.%d", i, i);
        pixt1 = pixMorphSequence(pixsd, buffer1, 0);
        pixt2 = pixMorphCompSequence(pixsd, buffer2, 0);
        pixEqual(pixt1, pixt2, &same);
	if (i < 64) {
            pixt3 = pixMorphCompSequenceDwa(pixsd, buffer2, 0);
            pixEqual(pixt1, pixt3, &same2);
	} else {
            pixt3 = NULL;
	    same2 = TRUE;
	}
        if (same && same2)
            writeResult(buffer1, 1);
        else {
            writeResult(buffer1, 0);
            success = FALSE;
        }
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixt3);

    }
    pixDestroy(&pixs);
    pixDestroy(&pixsd);

    if (success)
        fprintf(stderr, "\n---------- Success: no errors ----------\n");
    else
        fprintf(stderr, "\n---------- Failure: error(s) found -----------\n");
#endif


#if 0
    for (i = 1; i < 400; i++) {
        selectComposableSizes(i, &factor1, &factor2);
        diff = factor1 * factor2 - i;
        fprintf(stderr, "%d: (%d, %d): %d\n",
                  i, factor1, factor2, diff);
        selectComposableSels(i, L_HORIZ, &sel1, &sel2);
        selDestroy(&sel1);
        selDestroy(&sel2);
    }
#endif

#if 0
    selectComposableSels(68, L_HORIZ, &sel1, &sel2);  /* 17, 4 */
    str = selPrintToString(sel2);
    fprintf(stderr, str);
    selDestroy(&sel1);
    selDestroy(&sel2);
    FREE(str);
    selectComposableSels(70, L_HORIZ, &sel1, &sel2);  /* 10, 7 */
    str = selPrintToString(sel2);
    selDestroy(&sel1);
    selDestroy(&sel2);
    fprintf(stderr, str);
    FREE(str);
    selectComposableSels(85, L_HORIZ, &sel1, &sel2);  /* 17, 5 */
    str = selPrintToString(sel2);
    selDestroy(&sel1);
    selDestroy(&sel2);
    fprintf(stderr, str);
    FREE(str);
    selectComposableSels(96, L_HORIZ, &sel1, &sel2);  /* 12, 8 */
    str = selPrintToString(sel2);
    selDestroy(&sel1);
    selDestroy(&sel2);
    fprintf(stderr, str);
    FREE(str);

    { SELA *sela;
    sela = selaAddBasic(NULL);
    selaWrite("/tmp/junksela.sela", sela);
    selaDestroy(&sela);
    }
#endif

    return 0;
}


static void writeResult(char *sequence,
                        l_int32 same)
{
    if (same) 
        fprintf(stderr, "Sequence %s: SUCCESS\n", sequence);
    else
        fprintf(stderr, "Sequence %s: FAILURE\n", sequence);
}

