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
 *  binmorph5_reg.c
 *
 *    Regression test for expanded dwa morph operations.
 *    We compare:
 *       (1) dwa composite     vs.    morph composite
 *       (2) dwa composite     vs.    morph non-composite
 */

#include "allheaders.h"

l_int32 DoComparisonDwa1(PIX *pixs, PIX *pixt1, PIX *pixt2, PIX *pixt3,
                         PIX *pixt4, PIX *pixt5, PIX *pixt6, l_int32 isize);
l_int32 DoComparisonDwa2(PIX *pixs, PIX *pixt1, PIX *pixt2, PIX *pixt3,
                         PIX *pixt4, PIX *pixt5, PIX *pixt6, l_int32 size);
l_int32 PixCompareDwa(l_int32 size, const char *type, PIX *pixt1, PIX *pixt2,
                      PIX *pixt3, PIX *pixt4, PIX *pixt5, PIX *pixt6);

#define  TIMING         0
#define  FASTER_TEST    1
#define  SLOWER_TEST    1

    /* This fails on the symmetric case, but the differences are
     * relatively small.  Most of the problems seems to be in the
     * non-dwa code, because we are doing sequential erosions
     * without an extra border, and things aren't being properly
     * initialized.  To avoid these errors, add a border in advance
     * for symmetric b.c.  Note that asymmetric b.c. are recommended
     * for document image operations, and this test passes for
     * asymmetric b.c. */
#define    TEST_SYMMETRIC   0     /* set to 1 for symmetric b.c.;
                                     otherwise, it tests asymmetric b.c. */


int main(int    argc,
         char **argv)
{
l_int32  i, n, rsize, fact1, fact2, extra;
l_int32  size, lastsize;
l_int32  dwasize[256];
l_int32  ropsize[256];
PIX     *pixs, *pixt1, *pixt2, *pixt3, *pixt4, *pixt5, *pixt6;

    pixs = pixRead("feyn.tif");

#if TEST_SYMMETRIC
        /* This works properly if there's an added border */
    resetMorphBoundaryCondition(SYMMETRIC_MORPH_BC);
#if 1
    pixt1 = pixAddBorder(pixs, 64, 0);
    pixTransferAllData(pixs, &pixt1, 0, 0);
#endif
#endif  /* TEST_SYMMETRIC */

    pixt1 = pixCreateTemplateNoInit(pixs);
    pixt2 = pixCreateTemplateNoInit(pixs);
    pixt3 = pixCreateTemplateNoInit(pixs);
    pixt4 = pixCreateTemplateNoInit(pixs);
    pixt5 = pixCreateTemplateNoInit(pixs);
    pixt6 = pixCreateTemplateNoInit(pixs);


    /* ---------------------------------------------------------------- *
     *                  Faster test; testing fewer sizes                *
     * ---------------------------------------------------------------- */
#if  FASTER_TEST
        /* Compute the actual sizes used for each input size 'i' */
    for (i = 0; i < 256; i++) {
        dwasize[i] = 0;
        ropsize[i] = 0;
    }
    for (i = 65; i < 256; i++) {
        selectComposableSizes(i, &fact1, &fact2);
        rsize = fact1 * fact2;
        ropsize[i] = rsize;
        getExtendedCompositeParameters(i, &n, &extra, &dwasize[i]);
    }

        /* Use only values where the resulting sizes are equal */
    for (i = 65; i < 240; i++) {
        n = 1 + (l_int32)((i - 63) / 62);
        extra = i - 63 - (n - 1) * 62 + 1;
        if (extra == 2) continue;  /* don't use this one (e.g., i == 126) */
        if (ropsize[i] == dwasize[i])
            DoComparisonDwa1(pixs, pixt1, pixt2, pixt3, pixt4,
                             pixt5, pixt6, i);
    }
#endif  /* FASTER_TEST */

    /* ---------------------------------------------------------------- *
     *          Slower test; testing maximum number of sizes            *
     * ---------------------------------------------------------------- */
#if  SLOWER_TEST
    lastsize = 0;
    for (i = 65; i < 199; i++) {
        getExtendedCompositeParameters(i, &n, &extra, &size);
        if (size == lastsize) continue;
        if (size == 126 || size == 188) continue;  /* deliberately off by one */
        lastsize = size;
        DoComparisonDwa2(pixs, pixt1, pixt2, pixt3, pixt4,
                         pixt5, pixt6, size);
    }
#endif  /* SLOWER_TEST */

    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);
    pixDestroy(&pixt6);
    return 0;
}


l_int32
DoComparisonDwa1(PIX     *pixs,
                 PIX     *pixt1,
                 PIX     *pixt2,
                 PIX     *pixt3,
                 PIX     *pixt4,
                 PIX     *pixt5,
                 PIX     *pixt6,
                 l_int32  isize)
{
l_int32   fact1, fact2, size;

    selectComposableSizes(isize, &fact1, &fact2);
    size = fact1 * fact2;

    fprintf(stderr, "..%d..", size);

    if (TIMING) startTimer();
    pixDilateCompBrickExtendDwa(pixt1, pixs, size, 1);
    pixDilateCompBrickExtendDwa(pixt3, pixs, 1, size);
    pixDilateCompBrickExtendDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixDilateCompBrick(pixt2, pixs, size, 1);
    pixDilateCompBrick(pixt4, pixs, 1, size);
    pixDilateCompBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "dilate", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

    if (TIMING) startTimer();
    pixErodeCompBrickExtendDwa(pixt1, pixs, size, 1);
    pixErodeCompBrickExtendDwa(pixt3, pixs, 1, size);
    pixErodeCompBrickExtendDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixErodeCompBrick(pixt2, pixs, size, 1);
    pixErodeCompBrick(pixt4, pixs, 1, size);
    pixErodeCompBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "erode", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

    if (TIMING) startTimer();
    pixOpenCompBrickExtendDwa(pixt1, pixs, size, 1);
    pixOpenCompBrickExtendDwa(pixt3, pixs, 1, size);
    pixOpenCompBrickExtendDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixOpenCompBrick(pixt2, pixs, size, 1);
    pixOpenCompBrick(pixt4, pixs, 1, size);
    pixOpenCompBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "open", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

    if (TIMING) startTimer();
    pixCloseCompBrickExtendDwa(pixt1, pixs, size, 1);
    pixCloseCompBrickExtendDwa(pixt3, pixs, 1, size);
    pixCloseCompBrickExtendDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixCloseSafeCompBrick(pixt2, pixs, size, 1);
    pixCloseSafeCompBrick(pixt4, pixs, 1, size);
    pixCloseSafeCompBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "close", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

#if 0
    pixWrite("/tmp/junkpixt3.png", pixt3, IFF_PNG);
    pixWrite("/tmp/junkpixt4.png", pixt4, IFF_PNG);
    pixXor(pixt3, pixt3, pixt4);
    pixWrite("/tmp/junkxor.png", pixt3, IFF_PNG);
#endif

    return 0;
}


l_int32
DoComparisonDwa2(PIX     *pixs,
                 PIX     *pixt1,
                 PIX     *pixt2,
                 PIX     *pixt3,
                 PIX     *pixt4,
                 PIX     *pixt5,
                 PIX     *pixt6,
                 l_int32  size)  /* exactly decomposable */
{
    fprintf(stderr, "..%d..", size);

    if (TIMING) startTimer();
    pixDilateCompBrickExtendDwa(pixt1, pixs, size, 1);
    pixDilateCompBrickExtendDwa(pixt3, pixs, 1, size);
    pixDilateCompBrickExtendDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixDilateBrick(pixt2, pixs, size, 1);
    pixDilateBrick(pixt4, pixs, 1, size);
    pixDilateBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "dilate", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

    if (TIMING) startTimer();
    pixErodeCompBrickExtendDwa(pixt1, pixs, size, 1);
    pixErodeCompBrickExtendDwa(pixt3, pixs, 1, size);
    pixErodeCompBrickExtendDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixErodeBrick(pixt2, pixs, size, 1);
    pixErodeBrick(pixt4, pixs, 1, size);
    pixErodeBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "erode", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

    if (TIMING) startTimer();
    pixOpenCompBrickExtendDwa(pixt1, pixs, size, 1);
    pixOpenCompBrickExtendDwa(pixt3, pixs, 1, size);
    pixOpenCompBrickExtendDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixOpenBrick(pixt2, pixs, size, 1);
    pixOpenBrick(pixt4, pixs, 1, size);
    pixOpenBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "open", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

    if (TIMING) startTimer();
    pixCloseCompBrickExtendDwa(pixt1, pixs, size, 1);
    pixCloseCompBrickExtendDwa(pixt3, pixs, 1, size);
    pixCloseCompBrickExtendDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixCloseSafeBrick(pixt2, pixs, size, 1);
    pixCloseSafeBrick(pixt4, pixs, 1, size);
    pixCloseSafeBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "close", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

#if 0
    pixWrite("/tmp/junkpixt3.png", pixt3, IFF_PNG);
    pixWrite("/tmp/junkpixt4.png", pixt4, IFF_PNG);
    pixXor(pixt3, pixt3, pixt4);
    pixWrite("/tmp/junkxor.png", pixt3, IFF_PNG);
#endif

    return 0;
}


l_int32
PixCompareDwa(l_int32      size,
              const char  *type,
              PIX         *pixt1,
              PIX         *pixt2,
              PIX         *pixt3,
              PIX         *pixt4,
              PIX         *pixt5,
              PIX         *pixt6)
{
l_int32  same, fail;

    fail = FALSE;
    pixEqual(pixt1, pixt2, &same);
    if (!same) {
        fail = TRUE;
        fprintf(stderr, "%s (%d, 1) not same\n", type, size);
    }
    pixEqual(pixt3, pixt4, &same);
    if (!same) {
        fail = TRUE;
        fprintf(stderr, "%s (1, %d) not same\n", type, size);
    }
    pixEqual(pixt5, pixt6, &same);
    if (!same) {
        fail = TRUE;
        fprintf(stderr, "%s (%d, %d) not same\n", type, size, size);
    }
    return fail;
}

