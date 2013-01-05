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
 * textlinemask.c
 *
 *   This shows examples of finding textline masks for very
 *   simple geometries.  To do this task for more general layout,
 *   use pagesegtest* programs.
 *
 *      filein: e.g., use arabic.png
 *      fileout: debug output showing results
 */

#include "allheaders.h"

static const l_int32  DEBUG_OUTPUT = 1;

static const char *seltext = "xxxxxxx"
                             "   x   "
                             "   x   "
                             "   x   "
                             "   x   "
                             "   x   "
                             "   x   "
                             "   x   "
                             "o  X  o"
                             "   x   "
                             "   x   "
                             "   x   "
                             "   x   "
                             "   x   "
                             "   x   "
                             "   x   "
                             "xxxxxxx";


main(int    argc,
     char **argv)
{
char        *filein, *fileout;
l_int32      w, h, d, w2, h2, i, ncols;
l_float32    angle, conf;
BOX         *box;
BOXA        *boxa, *boxas, *boxad, *boxa2;
NUMA        *numa;
PIX         *pixs, *pixt, *pixb, *pixb2, *pixd;
PIX         *pixtlm, *pixvws;
PIX         *pixt1, *pixt2, *pixt3, *pixt4, *pixt5, *pixt6;
PIXA        *pixam, *pixac, *pixad, *pixat;
PIXAA       *pixaa, *pixaa2;
PTA         *pta;
SEL         *selsplit;
static char  mainName[] = "textlinemask";

    if (argc != 3)
        exit(ERROR_INT(" Syntax:  textlinemask filein fileout", mainName, 1));

    filein = argv[1];
    fileout = argv[2];

    pixDisplayWrite(NULL, -1);  /* init debug output */

    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);
    pixGetDimensions(pixs, &w, &h, &d);

        /* Binarize input */
    if (d == 8)
        pixt = pixThresholdToBinary(pixs, 128);
    else if (d == 1)
        pixt = pixClone(pixs);
    else {
        fprintf(stderr, "depth is %d\n", d);
        exit(1);
    }

        /* Deskew */
    pixb = pixFindSkewAndDeskew(pixt, 1, &angle, &conf);
    pixDestroy(&pixt);
    fprintf(stderr, "Skew angle: %7.2f degrees; %6.2f conf\n", angle, conf);
    pixDisplayWrite(pixb, DEBUG_OUTPUT);

#if 1
        /* Use full image morphology to find columns, at 2x reduction.
         * This only works for very simple layouts where each column
         * of text extends the full height of the input image.
         * pixam has a pix component over each column.  */
    pixb2 = pixReduceRankBinary2(pixb, 2, NULL);
    pixt1 = pixMorphCompSequence(pixb2, "c5.500", 0);
    boxa = pixConnComp(pixt1, &pixam, 8);
    ncols = boxaGetCount(boxa);
    fprintf(stderr, "Num columns: %d\n", ncols);
    pixDisplayWrite(pixt1, DEBUG_OUTPUT);

        /* Use selective region-based morphology to get the textline mask. */
    pixad = pixaMorphSequenceByRegion(pixb2, pixam, "c100.3", 0, 0);
    pixGetDimensions(pixb2, &w2, &h2, NULL);
    if (DEBUG_OUTPUT) {
        pixt2 = pixaDisplay(pixad, w2, h2);
        pixDisplayWrite(pixt2, DEBUG_OUTPUT);
        pixDestroy(&pixt2);
    }

        /* Some of the lines may be touching, so use a HMT to split the
         * lines in each column, and use a pixaa to save the results. */
    selsplit = selCreateFromString(seltext, 17, 7, "selsplit");
    pixaa = pixaaCreate(ncols);
    for (i = 0; i < ncols; i++) {
        pixt3 = pixaGetPix(pixad, i, L_CLONE);
        box = pixaGetBox(pixad, i, L_COPY);
        pixt4 = pixHMT(NULL, pixt3, selsplit);
        pixXor(pixt4, pixt4, pixt3);
        boxa2 = pixConnComp(pixt4, &pixac, 8);
        pixaaAddPixa(pixaa, pixac, L_INSERT);
        pixaaAddBox(pixaa, box, L_INSERT);
        if (DEBUG_OUTPUT) {
            pixt5 = pixaDisplayRandomCmap(pixac, 0, 0);
            pixDisplayWrite(pixt5, DEBUG_OUTPUT);
            fprintf(stderr, "Num textlines in col %d: %d\n", i,
                    boxaGetCount(boxa2));
            pixDestroy(&pixt5);
        }
        pixDestroy(&pixt3);
        pixDestroy(&pixt4);
        boxaDestroy(&boxa2);
    }

        /* Visual output */
    if (DEBUG_OUTPUT) {
        pixDisplayMultiple("/tmp/junk_write_display*");
        pixat = pixaReadFiles("/tmp", "junk_write_display");
        pixt5 = selDisplayInPix(selsplit, 31, 2);
        pixaAddPix(pixat, pixt5, L_INSERT);
        pixt6 = pixaDisplayTiledAndScaled(pixat, 32, 400, 3, 0, 35, 3);
        pixWrite(fileout, pixt6, IFF_PNG);
        pixaDestroy(&pixat);
        pixDestroy(&pixt6);
    }

        /* Test pixaa I/O */
    pixaaWrite("/tmp/junkpixaa", pixaa);
    pixaa2 = pixaaRead("/tmp/junkpixaa");
    pixaaWrite("/tmp/junkpixaa2", pixaa2);

        /* Test pixaa display */
    pixd = pixaaDisplay(pixaa, w2, h2);
    pixWrite("/tmp/junkdisplay", pixd, IFF_PNG);
    pixDestroy(&pixd);

        /* Cleanup */
    pixDestroy(&pixb2);
    pixDestroy(&pixt1);
    pixaDestroy(&pixam);
    pixaDestroy(&pixad);
    pixaaDestroy(&pixaa);
    pixaaDestroy(&pixaa2);
    boxaDestroy(&boxa);
    selDestroy(&selsplit);
#endif

#if 0
        /*  Use the baseline finder; not really what is needed */
    numa = pixFindBaselines(pixb, &pta, 1);
#endif

#if 0
        /* Use the textline mask function; parameters are not quite right */
    pixb2 = pixReduceRankBinary2(pixb, 2, NULL);
    pixtlm = pixGenTextlineMask(pixb2, &pixvws, NULL, 1);
    pixDisplay(pixtlm, 0, 100);
    pixDisplay(pixvws, 500, 100);
    pixDestroy(&pixb2);
    pixDestroy(&pixtlm);
    pixDestroy(&pixvws);
#endif

#if 0
        /* Use the Breuel whitespace partition method; slow and we would
         * still need to work to extract the fg regions. */
    pixb2 = pixReduceRankBinary2(pixb, 2, NULL);
    boxas = pixConnComp(pixb2, NULL, 8);
    boxad = boxaGetWhiteblocks(boxas, NULL, L_SORT_BY_HEIGHT,
                              3, 0.1, 200, 0.2, 0);
    pixd = pixDrawBoxa(pixb2, boxad, 7, 0xe0708000);
    pixDisplay(pixd, 100, 500);
    pixDestroy(&pixb2);
    pixDestroy(&pixd);
    boxaDestroy(&boxas);
    boxaDestroy(&boxad);
#endif


#if 0
        /* Use morphology to find columns and then selective
         * region-based morphology to get the textline mask.
         * This is for display; we really want to get a pixa of the
         * specific textline masks.   */
    startTimer();
    pixb2 = pixReduceRankBinary2(pixb, 2, NULL);
    pixt1 = pixMorphCompSequence(pixb2, "c5.500", 0);  /* column mask */
    pixt2 = pixMorphSequenceByRegion(pixb2, pixt1, "c100.3", 8, 0, 0, &boxa);
    fprintf(stderr, "time = %7.3f sec\n", stopTimer());
    pixDisplay(pixt1, 100, 500);
    pixDisplay(pixt2, 800, 500);
    pixDestroy(&pixb2);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    boxaDestroy(&boxa);
#endif

    pixDestroy(&pixs);
    pixDestroy(&pixb);
    return 0;
}

