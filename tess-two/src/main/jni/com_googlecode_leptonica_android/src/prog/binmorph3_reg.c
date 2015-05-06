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
 *  binmorph3_reg.c
 *
 *     This is a regression test of dwa functions.  It should always
 *     be run if changes are made to the low-level morphology code.
 *
 *  Some things to note:
 *
 *  (1) This compares results for these operations:
 *         - rasterop brick (non-separable, separable)
 *         - dwa brick (separable), as implemented in morphdwa.c
 *         - dwa brick separable, but using lower-level non-separable
 *           autogen'd code.
 *
 *  (2) See in-line comments for ordinary closing and safe closing.
 *      The complication is due to the fact that the results differ
 *      for symmetric and asymmetric b.c., so we must do some
 *      fine adjustments of the border when implementing using
 *      the lower-level code directly.
 */

#include "allheaders.h"

#define    TEST_SYMMETRIC   0     /* set to 1 for symmetric b.c.;
                                     otherwise, it tests asymmetric b.c. */

int main(int    argc,
         char **argv)
{
char        *selnameh, *selnamev;
l_int32      ok, same, w, h, i, bordercolor, extraborder;
l_int32      width[3] = {21, 1, 21};
l_int32      height[3] = {1, 7, 7};
PIX         *pixs, *pixref;
PIX         *pixt0, *pixt1, *pixt2, *pixt3, *pixt4;
SEL         *sel;
SELA        *sela;
static char  mainName[] = "binmorph3_reg";

    if (argc != 1)
        return ERROR_INT(" Syntax: binmorph3_reg", mainName, 1);
    if ((pixs = pixRead("feyn.tif")) == NULL)
        return ERROR_INT("pix not made", mainName, 1);

#if TEST_SYMMETRIC
    resetMorphBoundaryCondition(SYMMETRIC_MORPH_BC);
#endif  /* TEST_SYMMETRIC */

    for (i = 0; i < 3; i++) {
        w = width[i];
        h = height[i];
        sel = selCreateBrick(h, w, h / 2, w / 2, SEL_HIT);
        selnameh = NULL;
        selnamev = NULL;


            /* Get the selnames for horiz and vert */
        sela = selaAddBasic(NULL);
        if (w > 1) {
            if ((selnameh = selaGetBrickName(sela, w, 1)) == NULL) {
                selaDestroy(&sela);
                return ERROR_INT("dwa hor sel not defined", mainName, 1);
            }
        }
        if (h > 1) {
            if ((selnamev = selaGetBrickName(sela, 1, h)) == NULL) {
                selaDestroy(&sela);
                return ERROR_INT("dwa vert sel not defined", mainName, 1);
            }
        }
        fprintf(stderr, "w = %d, h = %d, selh = %s, selv = %s\n",
                w, h, selnameh, selnamev);
        ok = TRUE;
        selaDestroy(&sela);

            /* ----------------- Dilation ----------------- */
        fprintf(stderr, "Testing dilation\n");
        pixref = pixDilate(NULL, pixs, sel);
        pixt1 = pixDilateBrickDwa(NULL, pixs, w, h);
        pixEqual(pixref, pixt1, &same);
        if (!same) {
            fprintf(stderr, "pixref != pixt1 !\n"); ok = FALSE;
        }
        pixDestroy(&pixt1);

        if (w > 1)
            pixt1 = pixMorphDwa_1(NULL, pixs, L_MORPH_DILATE, selnameh);
        else
            pixt1 = pixClone(pixs);
        if (h > 1)
            pixt2 = pixMorphDwa_1(NULL, pixt1, L_MORPH_DILATE, selnamev);
        else
            pixt2 = pixClone(pixt1);
        pixEqual(pixref, pixt2, &same);
        if (!same) {
            fprintf(stderr, "pixref != pixt2 !\n"); ok = FALSE;
        }
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);

        pixt1 = pixAddBorder(pixs, 32, 0);
        if (w > 1)
            pixt2 = pixFMorphopGen_1(NULL, pixt1, L_MORPH_DILATE, selnameh);
        else
            pixt2 = pixClone(pixt1);
        if (h > 1)
            pixt3 = pixFMorphopGen_1(NULL, pixt2, L_MORPH_DILATE, selnamev);
        else
            pixt3 = pixClone(pixt2);
        pixt4 = pixRemoveBorder(pixt3, 32);
        pixEqual(pixref, pixt4, &same);
        if (!same) {
            fprintf(stderr, "pixref != pixt4 !\n"); ok = FALSE;
        }
        pixDestroy(&pixref);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixt3);
        pixDestroy(&pixt4);

            /* ----------------- Erosion ----------------- */
        fprintf(stderr, "Testing erosion\n");
        pixref = pixErode(NULL, pixs, sel);
        pixt1 = pixErodeBrickDwa(NULL, pixs, w, h);
        pixEqual(pixref, pixt1, &same);
        if (!same) {
            fprintf(stderr, "pixref != pixt1 !\n"); ok = FALSE;
        }
        pixDestroy(&pixt1);

        if (w > 1)
            pixt1 = pixMorphDwa_1(NULL, pixs, L_MORPH_ERODE, selnameh);
        else
            pixt1 = pixClone(pixs);
        if (h > 1)
            pixt2 = pixMorphDwa_1(NULL, pixt1, L_MORPH_ERODE, selnamev);
        else
            pixt2 = pixClone(pixt1);
        pixEqual(pixref, pixt2, &same);
        if (!same) {
            fprintf(stderr, "pixref != pixt2 !\n"); ok = FALSE;
        }
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);

        pixt1 = pixAddBorder(pixs, 32, 0);
        if (w > 1)
            pixt2 = pixFMorphopGen_1(NULL, pixt1, L_MORPH_ERODE, selnameh);
        else
            pixt2 = pixClone(pixt1);
        if (h > 1)
            pixt3 = pixFMorphopGen_1(NULL, pixt2, L_MORPH_ERODE, selnamev);
        else
            pixt3 = pixClone(pixt2);
        pixt4 = pixRemoveBorder(pixt3, 32);
        pixEqual(pixref, pixt4, &same);
        if (!same) {
            fprintf(stderr, "pixref != pixt4 !\n"); ok = FALSE;
        }
        pixDestroy(&pixref);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixt3);
        pixDestroy(&pixt4);

            /* ----------------- Opening ----------------- */
        fprintf(stderr, "Testing opening\n");
        pixref = pixOpen(NULL, pixs, sel);
        pixt1 = pixOpenBrickDwa(NULL, pixs, w, h);
        pixEqual(pixref, pixt1, &same);
        if (!same) {
            fprintf(stderr, "pixref != pixt1 !\n"); ok = FALSE;
        }
        pixDestroy(&pixt1);

        if (h == 1)
            pixt2 = pixMorphDwa_1(NULL, pixs, L_MORPH_OPEN, selnameh);
        else if (w == 1)
            pixt2 = pixMorphDwa_1(NULL, pixs, L_MORPH_OPEN, selnamev);
        else {
            pixt1 = pixMorphDwa_1(NULL, pixs, L_MORPH_ERODE, selnameh);
            pixt2 = pixMorphDwa_1(NULL, pixt1, L_MORPH_ERODE, selnamev);
            pixMorphDwa_1(pixt1, pixt2, L_MORPH_DILATE, selnameh);
            pixMorphDwa_1(pixt2, pixt1, L_MORPH_DILATE, selnamev);
            pixDestroy(&pixt1);
        }
        pixEqual(pixref, pixt2, &same);
        if (!same) {
            fprintf(stderr, "pixref != pixt2 !\n"); ok = FALSE;
        }
        pixDestroy(&pixt2);

        pixt1 = pixAddBorder(pixs, 32, 0);
        if (h == 1)
            pixt3 = pixFMorphopGen_1(NULL, pixt1, L_MORPH_OPEN, selnameh);
        else if (w == 1)
            pixt3 = pixFMorphopGen_1(NULL, pixt1, L_MORPH_OPEN, selnamev);
        else {
            pixt2 = pixFMorphopGen_1(NULL, pixt1, L_MORPH_ERODE, selnameh);
            pixt3 = pixFMorphopGen_1(NULL, pixt2, L_MORPH_ERODE, selnamev);
            pixFMorphopGen_1(pixt2, pixt3, L_MORPH_DILATE, selnameh);
            pixFMorphopGen_1(pixt3, pixt2, L_MORPH_DILATE, selnamev);
            pixDestroy(&pixt2);
        }
        pixt4 = pixRemoveBorder(pixt3, 32);
        pixEqual(pixref, pixt4, &same);
        if (!same) {
            fprintf(stderr, "pixref != pixt4 !\n"); ok = FALSE;
        }
        pixDestroy(&pixref);
        pixDestroy(&pixt1);
        pixDestroy(&pixt3);
        pixDestroy(&pixt4);

            /* ----------------- Closing ----------------- */
        fprintf(stderr, "Testing closing\n");
        pixref = pixClose(NULL, pixs, sel);

            /* Note: L_MORPH_CLOSE for h==1 or w==1 gives safe closing,
             * so we can't use it here. */
        if (h == 1) {
            pixt1 = pixMorphDwa_1(NULL, pixs, L_MORPH_DILATE, selnameh);
            pixt2 = pixMorphDwa_1(NULL, pixt1, L_MORPH_ERODE, selnameh);
        }
        else if (w == 1) {
            pixt1 = pixMorphDwa_1(NULL, pixs, L_MORPH_DILATE, selnamev);
            pixt2 = pixMorphDwa_1(NULL, pixt1, L_MORPH_ERODE, selnamev);
        }
        else {
            pixt1 = pixMorphDwa_1(NULL, pixs, L_MORPH_DILATE, selnameh);
            pixt2 = pixMorphDwa_1(NULL, pixt1, L_MORPH_DILATE, selnamev);
            pixMorphDwa_1(pixt1, pixt2, L_MORPH_ERODE, selnameh);
            pixMorphDwa_1(pixt2, pixt1, L_MORPH_ERODE, selnamev);
        }
        pixDestroy(&pixt1);
        pixEqual(pixref, pixt2, &same);
        if (!same) {
            fprintf(stderr, "pixref != pixt2 !\n"); ok = FALSE;
        }
        pixDestroy(&pixt2);

            /* Note: by adding only 32 pixels of border, we get
             * the normal closing operation, even when calling
             * with L_MORPH_CLOSE, because it requires 32 pixels
             * of border to be safe. */
        pixt1 = pixAddBorder(pixs, 32, 0);
        if (h == 1)
            pixt3 = pixFMorphopGen_1(NULL, pixt1, L_MORPH_CLOSE, selnameh);
        else if (w == 1)
            pixt3 = pixFMorphopGen_1(NULL, pixt1, L_MORPH_CLOSE, selnamev);
        else {
            pixt2 = pixFMorphopGen_1(NULL, pixt1, L_MORPH_DILATE, selnameh);
            pixt3 = pixFMorphopGen_1(NULL, pixt2, L_MORPH_DILATE, selnamev);
            pixFMorphopGen_1(pixt2, pixt3, L_MORPH_ERODE, selnameh);
            pixFMorphopGen_1(pixt3, pixt2, L_MORPH_ERODE, selnamev);
            pixDestroy(&pixt2);
        }
        pixt4 = pixRemoveBorder(pixt3, 32);
        pixEqual(pixref, pixt4, &same);
        if (!same) {
            fprintf(stderr, "pixref != pixt4 !\n"); ok = FALSE;
        }
        pixDestroy(&pixref);
        pixDestroy(&pixt1);
        pixDestroy(&pixt3);
        pixDestroy(&pixt4);

            /* ------------- Safe Closing ----------------- */
        fprintf(stderr, "Testing safe closing\n");
        pixref = pixCloseSafe(NULL, pixs, sel);
        pixt0 = pixCloseSafeBrick(NULL, pixs, w, h);
        pixEqual(pixref, pixt0, &same);
        if (!same) {
            fprintf(stderr, "pixref != pixt0 !\n"); ok = FALSE;
        }
        pixDestroy(&pixt0);

        pixt1 = pixCloseBrickDwa(NULL, pixs, w, h);
        pixEqual(pixref, pixt1, &same);
        if (!same) {
            fprintf(stderr, "pixref != pixt1 !\n"); ok = FALSE;
        }
        pixDestroy(&pixt1);

        bordercolor = getMorphBorderPixelColor(L_MORPH_ERODE, 1);
        if (bordercolor == 0)   /* asymmetric b.c. */
            extraborder = 32;
        else   /* symmetric b.c. */
            extraborder = 0;

            /* Note: for safe closing we need 64 border pixels.
             * However, when we implement a separable Sel
             * with pixMorphDwa_*(), we must do dilation and
             * erosion explicitly, and these functions only
             * add/remove a 32-pixel border.  Thus, for that
             * case we must add an additional 32-pixel border
             * before doing the operations.  That is the reason
             * why the implementation in morphdwa.c adds the
             * 64 bit border and then uses the lower-level
             * pixFMorphopGen_*() functions. */
        if (h == 1)
            pixt3 = pixMorphDwa_1(NULL, pixs, L_MORPH_CLOSE, selnameh);
        else if (w == 1)
            pixt3 = pixMorphDwa_1(NULL, pixs, L_MORPH_CLOSE, selnamev);
        else {
            pixt0 = pixAddBorder(pixs, extraborder, 0);
            pixt1 = pixMorphDwa_1(NULL, pixt0, L_MORPH_DILATE, selnameh);
            pixt2 = pixMorphDwa_1(NULL, pixt1, L_MORPH_DILATE, selnamev);
            pixMorphDwa_1(pixt1, pixt2, L_MORPH_ERODE, selnameh);
            pixMorphDwa_1(pixt2, pixt1, L_MORPH_ERODE, selnamev);
            pixt3 = pixRemoveBorder(pixt2, extraborder);
            pixDestroy(&pixt0);
            pixDestroy(&pixt1);
            pixDestroy(&pixt2);
        }
        pixEqual(pixref, pixt3, &same);
        if (!same) {
            fprintf(stderr, "pixref != pixt3 !\n"); ok = FALSE;
        }
        pixDestroy(&pixt3);

        pixt1 = pixAddBorder(pixs, 32 + extraborder, 0);
        if (h == 1)
            pixt3 = pixFMorphopGen_1(NULL, pixt1, L_MORPH_CLOSE, selnameh);
        else if (w == 1)
            pixt3 = pixFMorphopGen_1(NULL, pixt1, L_MORPH_CLOSE, selnamev);
        else {
            pixt2 = pixFMorphopGen_1(NULL, pixt1, L_MORPH_DILATE, selnameh);
            pixt3 = pixFMorphopGen_1(NULL, pixt2, L_MORPH_DILATE, selnamev);
            pixFMorphopGen_1(pixt2, pixt3, L_MORPH_ERODE, selnameh);
            pixFMorphopGen_1(pixt3, pixt2, L_MORPH_ERODE, selnamev);
            pixDestroy(&pixt2);
        }
        pixt4 = pixRemoveBorder(pixt3, 32 + extraborder);
        pixEqual(pixref, pixt4, &same);
        if (!same) {
            fprintf(stderr, "pixref != pixt4 !\n"); ok = FALSE;
        }
        pixDestroy(&pixref);
        pixDestroy(&pixt1);
        pixDestroy(&pixt3);
        pixDestroy(&pixt4);

        if (ok)
            fprintf(stderr, "All morph tests OK!\n");
        selDestroy(&sel);
        lept_free(selnameh);
        lept_free(selnamev);

    }

    pixDestroy(&pixs);
    return 0;
}

