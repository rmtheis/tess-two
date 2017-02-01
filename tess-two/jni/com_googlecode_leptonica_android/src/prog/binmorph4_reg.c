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
 *  binmorph4_reg.c
 *
 *    Regression test for dwa brick morph operations
 *    We compare:
 *       (1) morph composite    vs.   morph non-composite
 *       (2) dwa non-composite  vs.   morph composite
 *       (3) dwa composite      vs.   dwa non-composite
 *       (4) dwa composite      vs.   morph composite
 *       (5) dwa composite      vs.   morph non-composite
 *    The brick functions all have a pre-allocated pix as the dest.
 */

#include "allheaders.h"

l_int32 DoComparisonDwa1(PIX *pixs, PIX *pix1, PIX *pix2, PIX *pix3,
                         PIX *pix4, PIX *pix5, PIX *pix6, l_int32 isize);
l_int32 DoComparisonDwa2(PIX *pixs, PIX *pix1, PIX *pix2, PIX *pix3,
                         PIX *pix4, PIX *pix5, PIX *pix6, l_int32 isize);
l_int32 DoComparisonDwa3(PIX *pixs, PIX *pix1, PIX *pix2, PIX *pix3,
                         PIX *pix4, PIX *pix5, PIX *pix6, l_int32 isize);
l_int32 DoComparisonDwa4(PIX *pixs, PIX *pix1, PIX *pix2, PIX *pix3,
                         PIX *pix4, PIX *pix5, PIX *pix6, l_int32 isize);
l_int32 DoComparisonDwa5(PIX *pixs, PIX *pix1, PIX *pix2, PIX *pix3,
                         PIX *pix4, PIX *pix5, PIX *pix6, l_int32 isize);
l_int32 PixCompareDwa(l_int32 size, const char *type, PIX *pix1, PIX *pix2,
                       PIX *pix3, PIX *pix4, PIX *pix5, PIX *pix6);

#define    TIMING           0

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
l_int32  i;
PIX     *pixs, *pix1, *pix2, *pix3, *pix4, *pix5, *pix6;

    pixs = pixRead("feyn.tif");

#if TEST_SYMMETRIC
        /* This works properly if there is an added border */
    resetMorphBoundaryCondition(SYMMETRIC_MORPH_BC);
#if 1
    pix1 = pixAddBorder(pixs, 64, 0);
    pixTransferAllData(pixs, &pix1, 0, 0);
#endif
#endif  /* TEST_SYMMETRIC */

    pix1 = pixCreateTemplateNoInit(pixs);
    pix2 = pixCreateTemplateNoInit(pixs);
    pix3 = pixCreateTemplateNoInit(pixs);
    pix4 = pixCreateTemplateNoInit(pixs);
    pix5 = pixCreateTemplateNoInit(pixs);
    pix6 = pixCreateTemplateNoInit(pixs);

    for (i = 2; i < 64; i++) {

#if 1
            /* Compare morph composite with morph non-composite */
        DoComparisonDwa1(pixs, pix1, pix2, pix3, pix4, pix5, pix6, i);
#endif

#if 1
            /* Compare DWA non-composite with morph composite */
        if (i < 16)
            DoComparisonDwa2(pixs, pix1, pix2, pix3, pix4, pix5, pix6, i);
            /* Compare DWA composite with DWA non-composite */
        if (i < 16)
            DoComparisonDwa3(pixs, pix1, pix2, pix3, pix4, pix5, pix6, i);
            /* Compare DWA composite with morph composite */
        DoComparisonDwa4(pixs, pix1, pix2, pix3, pix4, pix5, pix6, i);
            /* Compare DWA composite with morph non-composite */
        DoComparisonDwa5(pixs, pix1, pix2, pix3, pix4, pix5, pix6, i);
#endif
    }

    pixDestroy(&pixs);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    pixDestroy(&pix4);
    pixDestroy(&pix5);
    pixDestroy(&pix6);
    return 0;
}

    /* Morph composite with morph non-composite */
l_int32
DoComparisonDwa1(PIX     *pixs,
                 PIX     *pix1,
                 PIX     *pix2,
                 PIX     *pix3,
                 PIX     *pix4,
                 PIX     *pix5,
                 PIX     *pix6,
                 l_int32  isize)
{
l_int32  fact1, fact2, size;

    selectComposableSizes(isize, &fact1, &fact2);
    size = fact1 * fact2;

    fprintf(stderr, "..%d..", size);

    if (TIMING) startTimer();
    pixDilateCompBrick(pix1, pixs, size, 1);
    pixDilateCompBrick(pix3, pixs, 1, size);
    pixDilateCompBrick(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixDilateBrick(pix2, pixs, size, 1);
    pixDilateBrick(pix4, pixs, 1, size);
    pixDilateBrick(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "dilate", pix1, pix2, pix3, pix4, pix5, pix6);

    if (TIMING) startTimer();
    pixErodeCompBrick(pix1, pixs, size, 1);
    pixErodeCompBrick(pix3, pixs, 1, size);
    pixErodeCompBrick(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixErodeBrick(pix2, pixs, size, 1);
    pixErodeBrick(pix4, pixs, 1, size);
    pixErodeBrick(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "erode", pix1, pix2, pix3, pix4, pix5, pix6);

    if (TIMING) startTimer();
    pixOpenCompBrick(pix1, pixs, size, 1);
    pixOpenCompBrick(pix3, pixs, 1, size);
    pixOpenCompBrick(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixOpenBrick(pix2, pixs, size, 1);
    pixOpenBrick(pix4, pixs, 1, size);
    pixOpenBrick(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "open", pix1, pix2, pix3, pix4, pix5, pix6);

#if 1
    pixWrite("/tmp/junko1.png", pix1, IFF_PNG);
    pixWrite("/tmp/junko2.png", pix2, IFF_PNG);
    pixXor(pix1, pix1, pix2);
    pixWrite("/tmp/junkoxor.png", pix1, IFF_PNG);
#endif

#if 0
    pixDisplay(pix1, 100, 100);
    pixDisplay(pix2, 800, 100);
    pixWrite("/tmp/junkpix1.png", pix1, IFF_PNG);
    pixWrite("/tmp/junkpix2.png", pix2, IFF_PNG);
#endif

    if (TIMING) startTimer();
    pixCloseSafeCompBrick(pix1, pixs, size, 1);
    pixCloseSafeCompBrick(pix3, pixs, 1, size);
    pixCloseSafeCompBrick(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixCloseSafeBrick(pix2, pixs, size, 1);
    pixCloseSafeBrick(pix4, pixs, 1, size);
    pixCloseSafeBrick(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "close", pix1, pix2, pix3, pix4, pix5, pix6);

#if 1
    pixWrite("/tmp/junkc1.png", pix1, IFF_PNG);
    pixWrite("/tmp/junkc2.png", pix2, IFF_PNG);
    pixXor(pix1, pix1, pix2);
    pixWrite("/tmp/junkcxor.png", pix1, IFF_PNG);
#endif

    return 0;
}


    /* Dwa non-composite with morph composite */
l_int32
DoComparisonDwa2(PIX     *pixs,
                 PIX     *pix1,
                 PIX     *pix2,
                 PIX     *pix3,
                 PIX     *pix4,
                 PIX     *pix5,
                 PIX     *pix6,
                 l_int32  isize)
{
l_int32  fact1, fact2, size;

    selectComposableSizes(isize, &fact1, &fact2);
    size = fact1 * fact2;

    fprintf(stderr, "..%d..", size);

    if (TIMING) startTimer();
    pixDilateBrickDwa(pix1, pixs, size, 1);
    pixDilateBrickDwa(pix3, pixs, 1, size);
    pixDilateBrickDwa(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixDilateCompBrick(pix2, pixs, size, 1);
    pixDilateCompBrick(pix4, pixs, 1, size);
    pixDilateCompBrick(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "dilate", pix1, pix2, pix3, pix4, pix5, pix6);

/*    pixDisplay(pix1, 100, 100); */
/*    pixDisplay(pix2, 800, 100); */

    if (TIMING) startTimer();
    pixErodeBrickDwa(pix1, pixs, size, 1);
    pixErodeBrickDwa(pix3, pixs, 1, size);
    pixErodeBrickDwa(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixErodeCompBrick(pix2, pixs, size, 1);
    pixErodeCompBrick(pix4, pixs, 1, size);
    pixErodeCompBrick(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "erode", pix1, pix2, pix3, pix4, pix5, pix6);

    if (TIMING) startTimer();
    pixOpenBrickDwa(pix1, pixs, size, 1);
    pixOpenBrickDwa(pix3, pixs, 1, size);
    pixOpenBrickDwa(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixOpenCompBrick(pix2, pixs, size, 1);
    pixOpenCompBrick(pix4, pixs, 1, size);
    pixOpenCompBrick(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "open", pix1, pix2, pix3, pix4, pix5, pix6);

    if (TIMING) startTimer();
    pixCloseBrickDwa(pix1, pixs, size, 1);
    pixCloseBrickDwa(pix3, pixs, 1, size);
    pixCloseBrickDwa(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixCloseSafeCompBrick(pix2, pixs, size, 1);
    pixCloseSafeCompBrick(pix4, pixs, 1, size);
    pixCloseSafeCompBrick(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "close", pix1, pix2, pix3, pix4, pix5, pix6);

/*    pixWrite("/tmp/junkpix1.png", pix1, IFF_PNG); */
/*    pixWrite("/tmp/junkpix2.png", pix2, IFF_PNG); */
/*    pixXor(pix1, pix1, pix2); */
/*    pixWrite("/tmp/junkxor.png", pix1, IFF_PNG); */

    return 0;
}


    /* Dwa composite with dwa non-composite */
l_int32
DoComparisonDwa3(PIX     *pixs,
                 PIX     *pix1,
                 PIX     *pix2,
                 PIX     *pix3,
                 PIX     *pix4,
                 PIX     *pix5,
                 PIX     *pix6,
                 l_int32  isize)
{
l_int32  fact1, fact2, size;

    selectComposableSizes(isize, &fact1, &fact2);
    size = fact1 * fact2;

    fprintf(stderr, "..%d..", size);

    if (TIMING) startTimer();
    pixDilateCompBrickDwa(pix1, pixs, size, 1);
    pixDilateCompBrickDwa(pix3, pixs, 1, size);
    pixDilateCompBrickDwa(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixDilateBrickDwa(pix2, pixs, size, 1);
    pixDilateBrickDwa(pix4, pixs, 1, size);
    pixDilateBrickDwa(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "dilate", pix1, pix2, pix3, pix4, pix5, pix6);

/*    pixDisplay(pix1, 100, 100); */
/*    pixDisplay(pix2, 800, 100); */

    if (TIMING) startTimer();
    pixErodeCompBrickDwa(pix1, pixs, size, 1);
    pixErodeCompBrickDwa(pix3, pixs, 1, size);
    pixErodeCompBrickDwa(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixErodeBrickDwa(pix2, pixs, size, 1);
    pixErodeBrickDwa(pix4, pixs, 1, size);
    pixErodeBrickDwa(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "erode", pix1, pix2, pix3, pix4, pix5, pix6);

    if (TIMING) startTimer();
    pixOpenCompBrickDwa(pix1, pixs, size, 1);
    pixOpenCompBrickDwa(pix3, pixs, 1, size);
    pixOpenCompBrickDwa(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixOpenBrickDwa(pix2, pixs, size, 1);
    pixOpenBrickDwa(pix4, pixs, 1, size);
    pixOpenBrickDwa(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "open", pix1, pix2, pix3, pix4, pix5, pix6);

    if (TIMING) startTimer();
    pixCloseCompBrickDwa(pix1, pixs, size, 1);
    pixCloseCompBrickDwa(pix3, pixs, 1, size);
    pixCloseCompBrickDwa(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixCloseBrickDwa(pix2, pixs, size, 1);
    pixCloseBrickDwa(pix4, pixs, 1, size);
    pixCloseBrickDwa(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "close", pix1, pix2, pix3, pix4, pix5, pix6);

#if 0
    pixWrite("/tmp/junkpix1.png", pix1, IFF_PNG);
    pixWrite("/tmp/junkpix2.png", pix2, IFF_PNG);
    pixXor(pix1, pix1, pix2);
    pixWrite("/tmp/junkxor.png", pix1, IFF_PNG);
#endif

    return 0;
}


    /* Dwa composite with morph composite */
l_int32
DoComparisonDwa4(PIX     *pixs,
                 PIX     *pix1,
                 PIX     *pix2,
                 PIX     *pix3,
                 PIX     *pix4,
                 PIX     *pix5,
                 PIX     *pix6,
                 l_int32  isize)
{
l_int32  fact1, fact2, size;

    selectComposableSizes(isize, &fact1, &fact2);
    size = fact1 * fact2;

    fprintf(stderr, "..%d..", size);

    if (TIMING) startTimer();
    pixDilateCompBrickDwa(pix1, pixs, size, 1);
    pixDilateCompBrickDwa(pix3, pixs, 1, size);
    pixDilateCompBrickDwa(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixDilateCompBrick(pix2, pixs, size, 1);
    pixDilateCompBrick(pix4, pixs, 1, size);
    pixDilateCompBrick(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "dilate", pix1, pix2, pix3, pix4, pix5, pix6);

/*    pixDisplay(pix1, 100, 100); */
/*    pixDisplay(pix2, 800, 100); */

    if (TIMING) startTimer();
    pixErodeCompBrickDwa(pix1, pixs, size, 1);
    pixErodeCompBrickDwa(pix3, pixs, 1, size);
    pixErodeCompBrickDwa(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixErodeCompBrick(pix2, pixs, size, 1);
    pixErodeCompBrick(pix4, pixs, 1, size);
    pixErodeCompBrick(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "erode", pix1, pix2, pix3, pix4, pix5, pix6);

    if (TIMING) startTimer();
    pixOpenCompBrickDwa(pix1, pixs, size, 1);
    pixOpenCompBrickDwa(pix3, pixs, 1, size);
    pixOpenCompBrickDwa(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixOpenCompBrick(pix2, pixs, size, 1);
    pixOpenCompBrick(pix4, pixs, 1, size);
    pixOpenCompBrick(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "open", pix1, pix2, pix3, pix4, pix5, pix6);

/*    pixDisplay(pix1, 100, 100);   */
/*    pixDisplay(pix2, 800, 100);   */
/*    pixWrite("/tmp/junkpix1.png", pix1, IFF_PNG);  */
/*    pixWrite("/tmp/junkpix2.png", pix2, IFF_PNG);  */

    if (TIMING) startTimer();
    pixCloseCompBrickDwa(pix1, pixs, size, 1);
    pixCloseCompBrickDwa(pix3, pixs, 1, size);
    pixCloseCompBrickDwa(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixCloseSafeCompBrick(pix2, pixs, size, 1);
    pixCloseSafeCompBrick(pix4, pixs, 1, size);
    pixCloseSafeCompBrick(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "close", pix1, pix2, pix3, pix4, pix5, pix6);

    return 0;
}


    /* Dwa composite with morph non-composite */
l_int32
DoComparisonDwa5(PIX     *pixs,
                 PIX     *pix1,
                 PIX     *pix2,
                 PIX     *pix3,
                 PIX     *pix4,
                 PIX     *pix5,
                 PIX     *pix6,
                 l_int32  isize)
{
l_int32  fact1, fact2, size;

    selectComposableSizes(isize, &fact1, &fact2);
    size = fact1 * fact2;

    fprintf(stderr, "..%d..", size);

    if (TIMING) startTimer();
    pixDilateCompBrickDwa(pix1, pixs, size, 1);
    pixDilateCompBrickDwa(pix3, pixs, 1, size);
    pixDilateCompBrickDwa(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixDilateBrick(pix2, pixs, size, 1);
    pixDilateBrick(pix4, pixs, 1, size);
    pixDilateBrick(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "dilate", pix1, pix2, pix3, pix4, pix5, pix6);

/*    pixDisplay(pix1, 100, 100);  */
/*    pixDisplay(pix2, 800, 100);  */

    if (TIMING) startTimer();
    pixErodeCompBrickDwa(pix1, pixs, size, 1);
    pixErodeCompBrickDwa(pix3, pixs, 1, size);
    pixErodeCompBrickDwa(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixErodeBrick(pix2, pixs, size, 1);
    pixErodeBrick(pix4, pixs, 1, size);
    pixErodeBrick(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "erode", pix1, pix2, pix3, pix4, pix5, pix6);

    if (TIMING) startTimer();
    pixOpenCompBrickDwa(pix1, pixs, size, 1);
    pixOpenCompBrickDwa(pix3, pixs, 1, size);
    pixOpenCompBrickDwa(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixOpenBrick(pix2, pixs, size, 1);
    pixOpenBrick(pix4, pixs, 1, size);
    pixOpenBrick(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "open", pix1, pix2, pix3, pix4, pix5, pix6);

    if (TIMING) startTimer();
    pixCloseCompBrickDwa(pix1, pixs, size, 1);
    pixCloseCompBrickDwa(pix3, pixs, 1, size);
    pixCloseCompBrickDwa(pix5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixCloseSafeBrick(pix2, pixs, size, 1);
    pixCloseSafeBrick(pix4, pixs, 1, size);
    pixCloseSafeBrick(pix6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "close", pix1, pix2, pix3, pix4, pix5, pix6);

    return 0;
}


l_int32
PixCompareDwa(l_int32      size,
              const char  *type,
              PIX         *pix1,
              PIX         *pix2,
              PIX         *pix3,
              PIX         *pix4,
              PIX         *pix5,
              PIX         *pix6)
{
l_int32  same, fail;

    fail = FALSE;
    pixEqual(pix1, pix2, &same);
    if (!same) {
        fail = TRUE;
        fprintf(stderr, "%s (%d, 1) not same\n", type, size);
    }
    pixEqual(pix3, pix4, &same);
    if (!same) {
        fail = TRUE;
        fprintf(stderr, "%s (1, %d) not same\n", type, size);
    }
    pixEqual(pix5, pix6, &same);
    if (!same) {
        fail = TRUE;
        fprintf(stderr, "%s (%d, %d) not same\n", type, size, size);
    }
    return fail;
}

