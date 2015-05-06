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
 *  binmorph1_reg.c
 *
 *     This is a thorough regression test of different methods for
 *     doing binary morphology.  It should always be run if changes
 *     are made to the low-level morphology code.
 *
 *  Some things to note:
 *
 *  (1) We add a white border to guarantee safe closing; i.e., that
 *      closing is extensive for ASYMMETRIC_MORPH_BC.  The separable
 *      sequence for closing is not safe, so if we didn't add the border
 *      ab initio, we would get different results for the atomic sequence
 *      closing (which is safe) and the separable one.
 *
 *  (2) There are no differences in any of the operations:
 *           rasterop general
 *           rasterop brick
 *           morph sequence rasterop brick
 *           dwa brick
 *           morph sequence dwa brick
 *           morph sequence dwa composite brick
 *      when using ASYMMETRIC_MORPH_BC.
 *      However, when using SYMMETRIC_MORPH_BC, there are differences
 *      in two of the safe closing operations.  These differences
 *      are in pix numbers 4 and 5.  These differences are
 *      all due to the fact that for SYMMETRIC_MORPH_BC, we don't need
 *      to add any borders to get the correct answer.  When we do
 *      add a border of 0 pixels, we naturally get a different result.
 *
 *  (3) The 2-way Sel decomposition functions, implemented with the
 *      separable brick interface, are tested separately against
 *      the rasterop brick.  See binmorph2_reg.c.
 */

#include "allheaders.h"

    /* set these ad lib. */
#define    WIDTH            21    /* brick sel width */
#define    HEIGHT           15    /* brick sel height */
#define    TEST_SYMMETRIC   0     /* set to 1 to set symmetric b.c.;
                                     otherwise, it tests asymmetric b.c. */


int main(int    argc,
         char **argv)
{
l_int32      ok, same;
char         sequence[512];
PIX         *pixs, *pixref;
PIX         *pixt1, *pixt2, *pixt3, *pixt4, *pixt5, *pixt6;
PIX         *pixt7, *pixt8, *pixt9, *pixt10, *pixt11;
PIX         *pixt12, *pixt13, *pixt14;
SEL         *sel;
static char  mainName[] = "binmorph1_reg";

    if (argc != 1)
        return ERROR_INT(" Syntax: binmorph1_reg", mainName, 1);
    if ((pixs = pixRead("feyn.tif")) == NULL)
        return ERROR_INT("pix not made", mainName, 1);

#if TEST_SYMMETRIC
        /* This works properly if there is an added border */
    resetMorphBoundaryCondition(SYMMETRIC_MORPH_BC);
#if 1
    pixt1 = pixAddBorder(pixs, 32, 0);
    pixTransferAllData(pixs, &pixt1, 0, 0);
#endif
#endif  /* TEST_SYMMETRIC */

        /* This is our test sel */
    sel = selCreateBrick(HEIGHT, WIDTH, HEIGHT / 2, WIDTH / 2, SEL_HIT);

        /* Dilation */
    fprintf(stderr, "Testing dilation\n");
    ok = TRUE;
    pixref = pixDilate(NULL, pixs, sel);   /* new one */
    pixt1 = pixCreateTemplate(pixs);
    pixDilate(pixt1, pixs, sel);           /* existing one */
    pixEqual(pixref, pixt1, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt1 !\n"); ok = FALSE;
    }
    pixt2 = pixCopy(NULL, pixs);
    pixDilate(pixt2, pixt2, sel);          /* in-place */
    pixEqual(pixref, pixt2, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt2 !\n"); ok = FALSE;
    }
    sprintf(sequence, "d%d.%d", WIDTH, HEIGHT);
    pixt3 = pixMorphSequence(pixs, sequence, 0);    /* sequence, atomic */
    pixEqual(pixref, pixt3, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt3 !\n"); ok = FALSE;
    }
    sprintf(sequence, "d%d.1 + d1.%d", WIDTH, HEIGHT);
    pixt4 = pixMorphSequence(pixs, sequence, 0);    /* sequence, separable */
    pixEqual(pixref, pixt4, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt4 !\n"); ok = FALSE;
    }
    pixt5 = pixDilateBrick(NULL, pixs, WIDTH, HEIGHT);  /* new one */
    pixEqual(pixref, pixt5, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt5 !\n"); ok = FALSE;
    }
    pixt6 = pixCreateTemplate(pixs);
    pixDilateBrick(pixt6, pixs, WIDTH, HEIGHT);  /* existing one */
    pixEqual(pixref, pixt6, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt6 !\n"); ok = FALSE;
    }
    pixt7 = pixCopy(NULL, pixs);
    pixDilateBrick(pixt7, pixt7, WIDTH, HEIGHT);  /* in-place */
    pixEqual(pixref, pixt7, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt7 !\n"); ok = FALSE;
    }
    pixt8 = pixDilateBrickDwa(NULL, pixs, WIDTH, HEIGHT);  /* new one */
    pixEqual(pixref, pixt8, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt8 !\n"); ok = FALSE;
    }
    pixt9 = pixCreateTemplate(pixs);
    pixDilateBrickDwa(pixt9, pixs, WIDTH, HEIGHT);  /* existing one */
    pixEqual(pixref, pixt9, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt9 !\n"); ok = FALSE;
    }
    pixt10 = pixCopy(NULL, pixs);
    pixDilateBrickDwa(pixt10, pixt10, WIDTH, HEIGHT);  /* in-place */
    pixEqual(pixref, pixt10, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt10 !\n"); ok = FALSE;
    }
    pixt11 = pixCreateTemplate(pixs);
    pixDilateCompBrickDwa(pixt11, pixs, WIDTH, HEIGHT);  /* existing one */
    pixEqual(pixref, pixt11, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt11 !\n"); ok = FALSE;
    }
    sprintf(sequence, "d%d.%d", WIDTH, HEIGHT);
    pixt12 = pixMorphCompSequence(pixs, sequence, 0);    /* comp sequence */
    pixEqual(pixref, pixt12, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt12!\n"); ok = FALSE;
    }
    pixt13 = pixMorphSequenceDwa(pixs, sequence, 0);    /* dwa sequence */
    pixEqual(pixref, pixt13, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt13!\n"); ok = FALSE;
    }
    pixDestroy(&pixref);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);
    pixDestroy(&pixt6);
    pixDestroy(&pixt7);
    pixDestroy(&pixt8);
    pixDestroy(&pixt9);
    pixDestroy(&pixt10);
    pixDestroy(&pixt11);
    pixDestroy(&pixt12);
    pixDestroy(&pixt13);

        /* Erosion */
    fprintf(stderr, "Testing erosion\n");
    pixref = pixErode(NULL, pixs, sel);   /* new one */
    pixt1 = pixCreateTemplate(pixs);
    pixErode(pixt1, pixs, sel);           /* existing one */
    pixEqual(pixref, pixt1, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt1 !\n"); ok = FALSE;
    }
    pixt2 = pixCopy(NULL, pixs);
    pixErode(pixt2, pixt2, sel);          /* in-place */
    pixEqual(pixref, pixt2, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt2 !\n"); ok = FALSE;
    }
    sprintf(sequence, "e%d.%d", WIDTH, HEIGHT);
    pixt3 = pixMorphSequence(pixs, sequence, 0);    /* sequence, atomic */
    pixEqual(pixref, pixt3, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt3 !\n"); ok = FALSE;
    }
    sprintf(sequence, "e%d.1 + e1.%d", WIDTH, HEIGHT);
    pixt4 = pixMorphSequence(pixs, sequence, 0);    /* sequence, separable */
    pixEqual(pixref, pixt4, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt4 !\n"); ok = FALSE;
    }
    pixt5 = pixErodeBrick(NULL, pixs, WIDTH, HEIGHT);  /* new one */
    pixEqual(pixref, pixt5, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt5 !\n"); ok = FALSE;
    }
    pixt6 = pixCreateTemplate(pixs);
    pixErodeBrick(pixt6, pixs, WIDTH, HEIGHT);  /* existing one */
    pixEqual(pixref, pixt6, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt6 !\n"); ok = FALSE;
    }
    pixt7 = pixCopy(NULL, pixs);
    pixErodeBrick(pixt7, pixt7, WIDTH, HEIGHT);  /* in-place */
    pixEqual(pixref, pixt7, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt7 !\n"); ok = FALSE;
    }
    pixt8 = pixErodeBrickDwa(NULL, pixs, WIDTH, HEIGHT);  /* new one */
    pixEqual(pixref, pixt8, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt8 !\n"); ok = FALSE;
    }
    pixt9 = pixCreateTemplate(pixs);
    pixErodeBrickDwa(pixt9, pixs, WIDTH, HEIGHT);  /* existing one */
    pixEqual(pixref, pixt9, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt9 !\n"); ok = FALSE;
    }
    pixt10 = pixCopy(NULL, pixs);
    pixErodeBrickDwa(pixt10, pixt10, WIDTH, HEIGHT);  /* in-place */
    pixEqual(pixref, pixt10, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt10 !\n"); ok = FALSE;
    }
    pixt11 = pixCreateTemplate(pixs);
    pixErodeCompBrickDwa(pixt11, pixs, WIDTH, HEIGHT);  /* existing one */
    pixEqual(pixref, pixt11, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt11 !\n"); ok = FALSE;
    }

    sprintf(sequence, "e%d.%d", WIDTH, HEIGHT);
    pixt12 = pixMorphCompSequence(pixs, sequence, 0);    /* comp sequence */
    pixEqual(pixref, pixt12, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt12!\n"); ok = FALSE;
    }
    pixt13 = pixMorphSequenceDwa(pixs, sequence, 0);    /* dwa sequence */
    pixEqual(pixref, pixt13, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt13!\n"); ok = FALSE;
    }
    pixDestroy(&pixref);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);
    pixDestroy(&pixt6);
    pixDestroy(&pixt7);
    pixDestroy(&pixt8);
    pixDestroy(&pixt9);
    pixDestroy(&pixt10);
    pixDestroy(&pixt11);
    pixDestroy(&pixt12);
    pixDestroy(&pixt13);

        /* Opening */
    fprintf(stderr, "Testing opening\n");
    pixref = pixOpen(NULL, pixs, sel);   /* new one */
    pixt1 = pixCreateTemplate(pixs);
    pixOpen(pixt1, pixs, sel);           /* existing one */
    pixEqual(pixref, pixt1, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt1 !\n"); ok = FALSE;
    }
    pixt2 = pixCopy(NULL, pixs);
    pixOpen(pixt2, pixt2, sel);          /* in-place */
    pixEqual(pixref, pixt2, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt2 !\n"); ok = FALSE;
    }
    sprintf(sequence, "o%d.%d", WIDTH, HEIGHT);
    pixt3 = pixMorphSequence(pixs, sequence, 0);    /* sequence, atomic */
    pixEqual(pixref, pixt3, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt3 !\n"); ok = FALSE;
    }
    sprintf(sequence, "e%d.%d + d%d.%d", WIDTH, HEIGHT, WIDTH, HEIGHT);
    pixt4 = pixMorphSequence(pixs, sequence, 0);    /* sequence, separable */
    pixEqual(pixref, pixt4, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt4 !\n"); ok = FALSE;
    }
    sprintf(sequence, "e%d.1 + e1.%d + d%d.1 + d1.%d", WIDTH, HEIGHT,
            WIDTH, HEIGHT);
    pixt5 = pixMorphSequence(pixs, sequence, 0);    /* sequence, separable^2 */
    pixEqual(pixref, pixt5, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt5 !\n"); ok = FALSE;
    }
    pixt6 = pixOpenBrick(NULL, pixs, WIDTH, HEIGHT);  /* new one */
    pixEqual(pixref, pixt6, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt6 !\n"); ok = FALSE;
    }
    pixt7 = pixCreateTemplate(pixs);
    pixOpenBrick(pixt7, pixs, WIDTH, HEIGHT);  /* existing one */
    pixEqual(pixref, pixt7, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt7 !\n"); ok = FALSE;
    }
    pixt8 = pixCopy(NULL, pixs);  /* in-place */
    pixOpenBrick(pixt8, pixt8, WIDTH, HEIGHT);  /* existing one */
    pixEqual(pixref, pixt8, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt8 !\n"); ok = FALSE;
    }
    pixt9 = pixOpenBrickDwa(NULL, pixs, WIDTH, HEIGHT);  /* new one */
    pixEqual(pixref, pixt9, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt9 !\n"); ok = FALSE;
    }
    pixt10 = pixCreateTemplate(pixs);
    pixOpenBrickDwa(pixt10, pixs, WIDTH, HEIGHT);  /* existing one */
    pixEqual(pixref, pixt10, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt10 !\n"); ok = FALSE;
    }
    pixt11 = pixCopy(NULL, pixs);
    pixOpenBrickDwa(pixt11, pixt11, WIDTH, HEIGHT);  /* in-place */
    pixEqual(pixref, pixt11, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt11 !\n"); ok = FALSE;
    }
    sprintf(sequence, "o%d.%d", WIDTH, HEIGHT);
    pixt12 = pixMorphCompSequence(pixs, sequence, 0);    /* comp sequence */
    pixEqual(pixref, pixt12, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt12!\n"); ok = FALSE;
    }

#if 0
    pixWrite("/tmp/junkref.png", pixref, IFF_PNG);
    pixWrite("/tmp/junk12.png", pixt12, IFF_PNG);
    pixt13 = pixXor(NULL, pixref, pixt12);
    pixWrite("/tmp/junk12a.png", pixt13, IFF_PNG);
    pixDestroy(&pixt13);
#endif

    pixt13 = pixMorphSequenceDwa(pixs, sequence, 0);    /* dwa sequence */
    pixEqual(pixref, pixt13, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt13!\n"); ok = FALSE;
    }
    pixt14 = pixCreateTemplate(pixs);
    pixOpenCompBrickDwa(pixt14, pixs, WIDTH, HEIGHT);  /* existing one */
    pixEqual(pixref, pixt14, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt14 !\n"); ok = FALSE;
    }

    pixDestroy(&pixref);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);
    pixDestroy(&pixt6);
    pixDestroy(&pixt7);
    pixDestroy(&pixt8);
    pixDestroy(&pixt9);
    pixDestroy(&pixt10);
    pixDestroy(&pixt11);
    pixDestroy(&pixt12);
    pixDestroy(&pixt13);
    pixDestroy(&pixt14);

        /* Closing */
    fprintf(stderr, "Testing closing\n");
    pixref = pixClose(NULL, pixs, sel);   /* new one */
    pixt1 = pixCreateTemplate(pixs);
    pixClose(pixt1, pixs, sel);           /* existing one */
    pixEqual(pixref, pixt1, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt1 !\n"); ok = FALSE;
    }
    pixt2 = pixCopy(NULL, pixs);
    pixClose(pixt2, pixt2, sel);          /* in-place */
    pixEqual(pixref, pixt2, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt2 !\n"); ok = FALSE;
    }
    sprintf(sequence, "d%d.%d + e%d.%d", WIDTH, HEIGHT, WIDTH, HEIGHT);
    pixt3 = pixMorphSequence(pixs, sequence, 0);    /* sequence, separable */
    pixEqual(pixref, pixt3, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt3 !\n"); ok = FALSE;
    }
    sprintf(sequence, "d%d.1 + d1.%d + e%d.1 + e1.%d", WIDTH, HEIGHT,
            WIDTH, HEIGHT);
    pixt4 = pixMorphSequence(pixs, sequence, 0);    /* sequence, separable^2 */
    pixEqual(pixref, pixt4, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt4 !\n"); ok = FALSE;
    }
    pixt5 = pixCloseBrick(NULL, pixs, WIDTH, HEIGHT);  /* new one */
    pixEqual(pixref, pixt5, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt5 !\n"); ok = FALSE;
    }
    pixt6 = pixCreateTemplate(pixs);
    pixCloseBrick(pixt6, pixs, WIDTH, HEIGHT);  /* existing one */
    pixEqual(pixref, pixt6, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt6 !\n"); ok = FALSE;
    }
    pixt7 = pixCopy(NULL, pixs);  /* in-place */
    pixCloseBrick(pixt7, pixt7, WIDTH, HEIGHT);  /* existing one */
    pixEqual(pixref, pixt7, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt7 !\n"); ok = FALSE;
    }
    pixDestroy(&pixref);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);
    pixDestroy(&pixt6);
    pixDestroy(&pixt7);

        /* Safe closing (using pix, not pixs) */
    fprintf(stderr, "Testing safe closing\n");
    pixref = pixCloseSafe(NULL, pixs, sel);   /* new one */
    pixt1 = pixCreateTemplate(pixs);
    pixCloseSafe(pixt1, pixs, sel);           /* existing one */
    pixEqual(pixref, pixt1, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt1 !\n"); ok = FALSE;
    }
    pixt2 = pixCopy(NULL, pixs);
    pixCloseSafe(pixt2, pixt2, sel);          /* in-place */
    pixEqual(pixref, pixt2, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt2 !\n"); ok = FALSE;
    }
    sprintf(sequence, "c%d.%d", WIDTH, HEIGHT);
    pixt3 = pixMorphSequence(pixs, sequence, 0);    /* sequence, atomic */
    pixEqual(pixref, pixt3, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt3 !\n"); ok = FALSE;
    }
    sprintf(sequence, "b32 + d%d.%d + e%d.%d", WIDTH, HEIGHT, WIDTH, HEIGHT);
    pixt4 = pixMorphSequence(pixs, sequence, 0);    /* sequence, separable */
    pixEqual(pixref, pixt4, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt4 !\n"); ok = FALSE;
    }
    sprintf(sequence, "b32 + d%d.1 + d1.%d + e%d.1 + e1.%d", WIDTH, HEIGHT,
            WIDTH, HEIGHT);
    pixt5 = pixMorphSequence(pixs, sequence, 0);    /* sequence, separable^2 */
    pixEqual(pixref, pixt5, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt5 !\n"); ok = FALSE;
    }
    pixt6 = pixCloseSafeBrick(NULL, pixs, WIDTH, HEIGHT);  /* new one */
    pixEqual(pixref, pixt6, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt6 !\n"); ok = FALSE;
    }
    pixt7 = pixCreateTemplate(pixs);
    pixCloseSafeBrick(pixt7, pixs, WIDTH, HEIGHT);  /* existing one */
    pixEqual(pixref, pixt7, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt7 !\n"); ok = FALSE;
    }
    pixt8 = pixCopy(NULL, pixs);  /* in-place */
    pixCloseSafeBrick(pixt8, pixt8, WIDTH, HEIGHT);  /* existing one */
    pixEqual(pixref, pixt8, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt8 !\n"); ok = FALSE;
    }
    pixt9 = pixCloseBrickDwa(NULL, pixs, WIDTH, HEIGHT);  /* new one */
    pixEqual(pixref, pixt9, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt9 !\n"); ok = FALSE;
    }
    pixt10 = pixCreateTemplate(pixs);
    pixCloseBrickDwa(pixt10, pixs, WIDTH, HEIGHT);  /* existing one */
    pixEqual(pixref, pixt10, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt10 !\n"); ok = FALSE;
    }
    pixt11 = pixCopy(NULL, pixs);
    pixCloseBrickDwa(pixt11, pixt11, WIDTH, HEIGHT);  /* in-place */
    pixEqual(pixref, pixt11, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt11 !\n"); ok = FALSE;
    }
    sprintf(sequence, "c%d.%d", WIDTH, HEIGHT);
    pixt12 = pixMorphCompSequence(pixs, sequence, 0);    /* comp sequence */
    pixEqual(pixref, pixt12, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt12!\n"); ok = FALSE;
    }
    pixt13 = pixMorphSequenceDwa(pixs, sequence, 0);    /* dwa sequence */
    pixEqual(pixref, pixt13, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt13!\n"); ok = FALSE;
    }
    pixt14 = pixCreateTemplate(pixs);
    pixCloseCompBrickDwa(pixt14, pixs, WIDTH, HEIGHT);  /* existing one */
    pixEqual(pixref, pixt14, &same);
    if (!same) {
        fprintf(stderr, "pixref != pixt14 !\n"); ok = FALSE;
    }

#if 0
    pixWrite("/tmp/junkref.png", pixref, IFF_PNG);
    pixWrite("/tmp/junk12.png", pixt12, IFF_PNG);
    pixt13 = pixXor(NULL, pixref, pixt12);
    pixWrite("/tmp/junk12a.png", pixt13, IFF_PNG);
    pixDestroy(&pixt13);
#endif

    pixDestroy(&pixref);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);
    pixDestroy(&pixt6);
    pixDestroy(&pixt7);
    pixDestroy(&pixt8);
    pixDestroy(&pixt9);
    pixDestroy(&pixt10);
    pixDestroy(&pixt11);
    pixDestroy(&pixt12);
    pixDestroy(&pixt13);
    pixDestroy(&pixt14);

    if (ok)
        fprintf(stderr, "All morph tests OK!\n");

    pixDestroy(&pixs);
    selDestroy(&sel);
    return 0;
}

