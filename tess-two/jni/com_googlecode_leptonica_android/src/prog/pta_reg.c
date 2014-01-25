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
 *  pta_reg.c
 *
 *  This tests several ptaa functions, including:
 *     -  ptaaGetBoundaryPixels()
 *     -  pixRenderRandomCmapPtaa()
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
l_int32  i, nbox, npta, fgcount, bgcount, count, same, ok;
BOXA    *boxa;
PIX     *pixs, *pixfg, *pixbg, *pixc, *pixb, *pixd;
PIXA    *pixa;
PTA     *pta;
PTAA    *ptaafg, *ptaabg;

    ok = TRUE;
    pixs = pixRead("feyn-fract.tif");
    boxa = pixConnComp(pixs, NULL, 8);
    nbox = boxaGetCount(boxa);
    if (nbox != 464) ok = FALSE;
    fprintf(stderr, "Num boxes = %d\n", nbox);

        /* Get fg and bg boundary pixels */
    pixfg = pixMorphSequence(pixs, "e3.3", 0);
    pixXor(pixfg, pixfg, pixs);
    pixCountPixels(pixfg, &fgcount, NULL);
    if (fgcount == 58764)
        fprintf(stderr, "num fg pixels = %d\n", fgcount);
    else {
        ok = FALSE;
        fprintf(stderr, "Error: num fg pixels = %d\n", fgcount);
    }

    pixbg = pixMorphSequence(pixs, "d3.3", 0);
    pixXor(pixbg, pixbg, pixs);
    pixCountPixels(pixbg, &bgcount, NULL);
    if (bgcount == 60335)
        fprintf(stderr, "num bg pixels = %d\n", bgcount);
    else {
        ok = FALSE;
        fprintf(stderr, "Error: num bg pixels = %d\n", bgcount);
    }

        /* Get ptaa of fg pixels */
    ptaafg = ptaaGetBoundaryPixels(pixs, L_BOUNDARY_FG, 8, NULL, NULL);
    ptaaWrite("/tmp/fg.ptaa", ptaafg, 1);
    npta = ptaaGetCount(ptaafg);
    if (npta != nbox) {
        ok = FALSE;
        fprintf(stderr, "Error: ptaa count = %d, boxa count = %d\n",
                npta, nbox);
    }
    count = 0;
    for (i = 0; i < npta; i++) {
        pta = ptaaGetPta(ptaafg, i, L_CLONE);
        count += ptaGetCount(pta);
        ptaDestroy(&pta);
    }
    fprintf(stderr, "num fg pts = %d\n", count);
    if (fgcount != count) {
        ok = FALSE;
        fprintf(stderr, "Error: npix = %d, num fg pts = %d\n", fgcount, count);
    }

        /* Get ptaa of bg pixels.  Note that the number of bg pts
         * is, in general, larger than the number of bg boundary pixels,
         * because bg boundary pixels are shared by two c.c. that
         * are 1 pixel apart. */
    ptaabg = ptaaGetBoundaryPixels(pixs, L_BOUNDARY_BG, 8, NULL, NULL);
    ptaaWrite("/tmp/bg.ptaa", ptaabg, 1);
    npta = ptaaGetCount(ptaabg);
    if (npta != nbox) {
        ok = FALSE;
        fprintf(stderr, "Error: ptaa count = %d, boxa count = %d\n",
                npta, nbox);
    }
    count = 0;
    for (i = 0; i < npta; i++) {
        pta = ptaaGetPta(ptaabg, i, L_CLONE);
        count += ptaGetCount(pta);
        ptaDestroy(&pta);
    }
    fprintf(stderr, "num bg pts = %d\n", count);
    if (count != 60602) {
        fprintf(stderr, "Error: npix = %d, num bg pts = %d\n", bgcount, count);
        ok = FALSE;
    }

        /* Render the fg boundary pixels on top of pixs. */
    pixa = pixaCreate(4);
    pixc = pixRenderRandomCmapPtaa(pixs, ptaafg, 0, 0, 0);
    pixSaveTiled(pixc, pixa, 1.0, 1, 30, 32);
    pixDestroy(&pixc);

        /* Render the bg boundary pixels on top of pixs. */
    pixc = pixRenderRandomCmapPtaa(pixs, ptaabg, 0, 0, 0);
    pixSaveTiled(pixc, pixa, 1.0, 0, 30, 32);
    pixDestroy(&pixc);

    pixClearAll(pixs);

        /* Render the fg boundary pixels alone. */
    pixc = pixRenderRandomCmapPtaa(pixs, ptaafg, 0, 0, 0);
    pixSaveTiled(pixc, pixa, 1.0, 1, 30, 32);

        /* Verify that the fg pixels are the same set as we
         * originally started with. */
    pixb = pixConvertTo1(pixc, 255);
    pixEqual(pixb, pixfg, &same);
    if (!same) {
        fprintf(stderr, "Fg pixel set not correct\n");
        ok = FALSE;
    }
    pixDestroy(&pixc);
    pixDestroy(&pixb);

        /* Render the fg boundary pixels alone. */
    pixc = pixRenderRandomCmapPtaa(pixs, ptaabg, 0, 0, 0);
    pixSaveTiled(pixc, pixa, 1.0, 0, 30, 32);

        /* Verify that the bg pixels are the same set as we
         * originally started with. */
    pixb = pixConvertTo1(pixc, 255);
    pixEqual(pixb, pixbg, &same);
    if (!same) {
        fprintf(stderr, "Bg pixel set not correct\n");
        ok = FALSE;
    }
    pixDestroy(&pixc);
    pixDestroy(&pixb);

    if (ok)
        fprintf(stderr, "OK!\n");
    else
        fprintf(stderr, "Error!\n");

    pixd = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/boundary.png", pixd, IFF_PNG);
    pixDisplay(pixd, 0, 0);

    ptaaDestroy(&ptaafg);
    ptaaDestroy(&ptaabg);
    pixDestroy(&pixs);
    pixDestroy(&pixfg);
    pixDestroy(&pixbg);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    boxaDestroy(&boxa);

    return 0;
}


