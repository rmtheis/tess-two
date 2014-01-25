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
 * arabic_lines.c
 *
 *   Demonstrates some segmentation techniques and display options
 *   To see the results in one image: /tmp/result.png.
 */

#include "allheaders.h"


/* Hit-miss transform that splits lightly touching lines */
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

int main(int    argc,
         char **argv)
{
l_int32      w, h, d, w2, h2, i, ncols, ret;
l_float32    angle, conf;
BOX         *box;
BOXA        *boxa, *boxa2;
PIX         *pix, *pixs, *pixb, *pixb2, *pixd;
PIX         *pix1, *pix2, *pix3, *pix4, *pix5, *pix6;
PIXA        *pixam;  /* mask with a single component over each column */
PIXA        *pixac, *pixad, *pixat;
PIXAA       *pixaa, *pixaa2;
SEL         *selsplit;
static char  mainName[] = "arabic_lines";

    if (argc != 1)
        return ERROR_INT(" Syntax:  arabic_lines", mainName, 1);

    pixDisplayWrite(NULL, -1);  /* init debug output */

        /* Binarize input */
    pixs = pixRead("arabic_lines.png");
    pixGetDimensions(pixs, &w, &h, &d);
    pix = pixConvertTo1(pixs, 128);

        /* Deskew */
    pixb = pixFindSkewAndDeskew(pix, 1, &angle, &conf);
    pixDestroy(&pix);
    fprintf(stderr, "Skew angle: %7.2f degrees; %6.2f conf\n", angle, conf);
    pixDisplayWrite(pixb, 1);

        /* Use full image morphology to find columns, at 2x reduction.
           This only works for very simple layouts where each column
           of text extends the full height of the input image.  */
    pixb2 = pixReduceRankBinary2(pixb, 2, NULL);
    pix1 = pixMorphCompSequence(pixb2, "c5.500", 0);
    boxa = pixConnComp(pix1, &pixam, 8);
    ncols = boxaGetCount(boxa);
    fprintf(stderr, "Num columns: %d\n", ncols);
    pixDisplayWrite(pix1, 1);

        /* Use selective region-based morphology to get the textline mask. */
    pixad = pixaMorphSequenceByRegion(pixb2, pixam, "c100.3", 0, 0);
    pixGetDimensions(pixb2, &w2, &h2, NULL);
    pix2 = pixaDisplay(pixad, w2, h2);
    pixDisplayWrite(pix2, 1);
    pixDestroy(&pix2);

        /* Some of the lines may be touching, so use a HMT to split the
           lines in each column, and use a pixaa to save the results. */
    selsplit = selCreateFromString(seltext, 17, 7, "selsplit");
    pixaa = pixaaCreate(ncols);
    for (i = 0; i < ncols; i++) {
        pix3 = pixaGetPix(pixad, i, L_CLONE);
        box = pixaGetBox(pixad, i, L_COPY);
        pix4 = pixHMT(NULL, pix3, selsplit);
        pixXor(pix4, pix4, pix3);
        boxa2 = pixConnComp(pix4, &pixac, 8);
        pixaaAddPixa(pixaa, pixac, L_INSERT);
        pixaaAddBox(pixaa, box, L_INSERT);
        pix5 = pixaDisplayRandomCmap(pixac, 0, 0);
        pixDisplayWrite(pix5, 1);
        fprintf(stderr, "Num textlines in col %d: %d\n", i,
                boxaGetCount(boxa2));
        pixDestroy(&pix5);
        pixDestroy(&pix3);
        pixDestroy(&pix4);
        boxaDestroy(&boxa2);
    }

        /* Visual output */
    ret = system("gthumb /tmp/display/file* &");
    pixat = pixaReadFiles("/tmp/display", "file");
    pix5 = selDisplayInPix(selsplit, 31, 2);
    pixaAddPix(pixat, pix5, L_INSERT);
    pix6 = pixaDisplayTiledAndScaled(pixat, 32, 400, 3, 0, 35, 3);
    pixWrite("/tmp/result.png", pix6, IFF_PNG);
    pixaDestroy(&pixat);
    pixDestroy(&pix6);

        /* Test pixaa I/O */
    pixaaWrite("/tmp/pixaa", pixaa);
    pixaa2 = pixaaRead("/tmp/pixaa");
    pixaaWrite("/tmp/pixaa2", pixaa2);

        /* Test pixaa display */
    pixd = pixaaDisplay(pixaa, w2, h2);
    pixWrite("/tmp/textlines.png", pixd, IFF_PNG);
    pixDestroy(&pixd);

        /* Cleanup */
    pixDestroy(&pixb2);
    pixDestroy(&pix1);
    pixaDestroy(&pixam);
    pixaDestroy(&pixad);
    pixaaDestroy(&pixaa);
    pixaaDestroy(&pixaa2);
    boxaDestroy(&boxa);
    selDestroy(&selsplit);
    pixDestroy(&pixs);
    pixDestroy(&pixb);
    return 0;
}

