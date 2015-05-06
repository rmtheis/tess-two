/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -
 -  Redistribution and use in source and binary forms, with or without
 -  modification, are permitted provided that the following conditions
 -  are met:
 -  1. Redistributions of source code must retain the above copyright
 -     notice, this list of conditions and the following disclaimer.
 -  2. Redistributions in binary form must reproduce the above
 -     copyright notice, this list of conditions and the following
 -     disclaimer in the documentation and/or other materials
 -     provided with the distribution.
 -
 -  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 -  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 -  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 -  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 -  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 -  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 -  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 -  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 -  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 -  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 -  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================*/

/*
 * binmorph2_reg.c
 *
 *    Thorough regression test for binary separable rasterops,
 *    using the sequence interpreters.  This compares the
 *    results for 2-way composite Sels with unitary Sels,
 *    all invoked on the separable block morph ops.
 */

#include "allheaders.h"

static const l_int32  MAX_SEL_SIZE = 120;

static void writeResult(char *sequence, l_int32 same);


int main(int    argc,
         char **argv)
{
char        *str;
char         buffer1[256];
char         buffer2[256];
l_int32      i, same, same2, factor1, factor2, diff, success;
PIX         *pixs, *pixsd, *pixt1, *pixt2, *pixt3;
SEL         *sel1, *sel2;
static char  mainName[] = "binmorph2_reg";

    if (argc != 1)
        return ERROR_INT(" Syntax:  binmorph2_reg", mainName, 1);

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
    lept_free(str);
    selectComposableSels(70, L_HORIZ, &sel1, &sel2);  /* 10, 7 */
    str = selPrintToString(sel2);
    selDestroy(&sel1);
    selDestroy(&sel2);
    fprintf(stderr, str);
    lept_free(str);
    selectComposableSels(85, L_HORIZ, &sel1, &sel2);  /* 17, 5 */
    str = selPrintToString(sel2);
    selDestroy(&sel1);
    selDestroy(&sel2);
    fprintf(stderr, str);
    lept_free(str);
    selectComposableSels(96, L_HORIZ, &sel1, &sel2);  /* 12, 8 */
    str = selPrintToString(sel2);
    selDestroy(&sel1);
    selDestroy(&sel2);
    fprintf(stderr, str);
    lept_free(str);

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

