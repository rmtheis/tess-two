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

l_int32 DoComparisonDwa1(PIX *pixs, PIX *pixt1, PIX *pixt2, PIX *pixt3,
                         PIX *pixt4, PIX *pixt5, PIX *pixt6, l_int32 isize);
l_int32 DoComparisonDwa2(PIX *pixs, PIX *pixt1, PIX *pixt2, PIX *pixt3,
                         PIX *pixt4, PIX *pixt5, PIX *pixt6, l_int32 isize);
l_int32 DoComparisonDwa3(PIX *pixs, PIX *pixt1, PIX *pixt2, PIX *pixt3,
                         PIX *pixt4, PIX *pixt5, PIX *pixt6, l_int32 isize);
l_int32 DoComparisonDwa4(PIX *pixs, PIX *pixt1, PIX *pixt2, PIX *pixt3,
                         PIX *pixt4, PIX *pixt5, PIX *pixt6, l_int32 isize);
l_int32 DoComparisonDwa5(PIX *pixs, PIX *pixt1, PIX *pixt2, PIX *pixt3,
                         PIX *pixt4, PIX *pixt5, PIX *pixt6, l_int32 isize);
l_int32 PixCompareDwa(l_int32 size, const char *type, PIX *pixt1, PIX *pixt2,
                       PIX *pixt3, PIX *pixt4, PIX *pixt5, PIX *pixt6);

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
PIX     *pixs, *pixt1, *pixt2, *pixt3, *pixt4, *pixt5, *pixt6;

    pixs = pixRead("feyn.tif");

#if TEST_SYMMETRIC
        /* This works properly if there is an added border */
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

    for (i = 2; i < 64; i++) {

#if 1
            /* Compare morph composite with morph non-composite */
        DoComparisonDwa1(pixs, pixt1, pixt2, pixt3, pixt4,
                         pixt5, pixt6, i);
#endif

#if 1
            /* Compare DWA non-composite with morph composite */
        if (i < 16)
            DoComparisonDwa2(pixs, pixt1, pixt2, pixt3, pixt4,
                             pixt5, pixt6, i);
            /* Compare DWA composite with DWA non-composite */
        if (i < 16)
            DoComparisonDwa3(pixs, pixt1, pixt2, pixt3, pixt4,
                             pixt5, pixt6, i);
            /* Compare DWA composite with morph composite */
        DoComparisonDwa4(pixs, pixt1, pixt2, pixt3, pixt4,
                         pixt5, pixt6, i);
            /* Compare DWA composite with morph non-composite */
        DoComparisonDwa5(pixs, pixt1, pixt2, pixt3, pixt4,
                         pixt5, pixt6, i);
#endif
    }

    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);
    pixDestroy(&pixt6);
    return 0;
}

    /* morph composite with morph non-composite */
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
    pixDilateCompBrick(pixt1, pixs, size, 1);
    pixDilateCompBrick(pixt3, pixs, 1, size);
    pixDilateCompBrick(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixDilateBrick(pixt2, pixs, size, 1);
    pixDilateBrick(pixt4, pixs, 1, size);
    pixDilateBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "dilate", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

    if (TIMING) startTimer();
    pixErodeCompBrick(pixt1, pixs, size, 1);
    pixErodeCompBrick(pixt3, pixs, 1, size);
    pixErodeCompBrick(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixErodeBrick(pixt2, pixs, size, 1);
    pixErodeBrick(pixt4, pixs, 1, size);
    pixErodeBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "erode", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

    if (TIMING) startTimer();
    pixOpenCompBrick(pixt1, pixs, size, 1);
    pixOpenCompBrick(pixt3, pixs, 1, size);
    pixOpenCompBrick(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixOpenBrick(pixt2, pixs, size, 1);
    pixOpenBrick(pixt4, pixs, 1, size);
    pixOpenBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "open", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

#if 1
    pixWrite("/tmp/junko1.png", pixt1, IFF_PNG);
    pixWrite("/tmp/junko2.png", pixt2, IFF_PNG);
    pixXor(pixt1, pixt1, pixt2);
    pixWrite("/tmp/junkoxor.png", pixt1, IFF_PNG);
#endif

#if 0
    pixDisplay(pixt1, 100, 100);
    pixDisplay(pixt2, 800, 100);
    pixWrite("/tmp/junkpixt1.png", pixt1, IFF_PNG);
    pixWrite("/tmp/junkpixt2.png", pixt2, IFF_PNG);
#endif

    if (TIMING) startTimer();
    pixCloseSafeCompBrick(pixt1, pixs, size, 1);
    pixCloseSafeCompBrick(pixt3, pixs, 1, size);
    pixCloseSafeCompBrick(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixCloseSafeBrick(pixt2, pixs, size, 1);
    pixCloseSafeBrick(pixt4, pixs, 1, size);
    pixCloseSafeBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "close", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

#if 1
    pixWrite("/tmp/junkc1.png", pixt1, IFF_PNG);
    pixWrite("/tmp/junkc2.png", pixt2, IFF_PNG);
    pixXor(pixt1, pixt1, pixt2);
    pixWrite("/tmp/junkcxor.png", pixt1, IFF_PNG);
#endif

    return 0;
}


    /* dwa non-composite with morph composite */
l_int32
DoComparisonDwa2(PIX     *pixs,
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
    pixDilateBrickDwa(pixt1, pixs, size, 1);
    pixDilateBrickDwa(pixt3, pixs, 1, size);
    pixDilateBrickDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixDilateCompBrick(pixt2, pixs, size, 1);
    pixDilateCompBrick(pixt4, pixs, 1, size);
    pixDilateCompBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "dilate", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

/*    pixDisplay(pixt1, 100, 100); */
/*    pixDisplay(pixt2, 800, 100); */

    if (TIMING) startTimer();
    pixErodeBrickDwa(pixt1, pixs, size, 1);
    pixErodeBrickDwa(pixt3, pixs, 1, size);
    pixErodeBrickDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixErodeCompBrick(pixt2, pixs, size, 1);
    pixErodeCompBrick(pixt4, pixs, 1, size);
    pixErodeCompBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "erode", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

    if (TIMING) startTimer();
    pixOpenBrickDwa(pixt1, pixs, size, 1);
    pixOpenBrickDwa(pixt3, pixs, 1, size);
    pixOpenBrickDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixOpenCompBrick(pixt2, pixs, size, 1);
    pixOpenCompBrick(pixt4, pixs, 1, size);
    pixOpenCompBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "open", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

    if (TIMING) startTimer();
    pixCloseBrickDwa(pixt1, pixs, size, 1);
    pixCloseBrickDwa(pixt3, pixs, 1, size);
    pixCloseBrickDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixCloseSafeCompBrick(pixt2, pixs, size, 1);
    pixCloseSafeCompBrick(pixt4, pixs, 1, size);
    pixCloseSafeCompBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "close", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

/*    pixWrite("/tmp/junkpixt1.png", pixt1, IFF_PNG); */
/*    pixWrite("/tmp/junkpixt2.png", pixt2, IFF_PNG); */
/*    pixXor(pixt1, pixt1, pixt2); */
/*    pixWrite("/tmp/junkxor.png", pixt1, IFF_PNG); */

    return 0;
}


    /* dwa composite with dwa non-composite */
l_int32
DoComparisonDwa3(PIX     *pixs,
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
    pixDilateCompBrickDwa(pixt1, pixs, size, 1);
    pixDilateCompBrickDwa(pixt3, pixs, 1, size);
    pixDilateCompBrickDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixDilateBrickDwa(pixt2, pixs, size, 1);
    pixDilateBrickDwa(pixt4, pixs, 1, size);
    pixDilateBrickDwa(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "dilate", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

/*    pixDisplay(pixt1, 100, 100); */
/*    pixDisplay(pixt2, 800, 100); */

    if (TIMING) startTimer();
    pixErodeCompBrickDwa(pixt1, pixs, size, 1);
    pixErodeCompBrickDwa(pixt3, pixs, 1, size);
    pixErodeCompBrickDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixErodeBrickDwa(pixt2, pixs, size, 1);
    pixErodeBrickDwa(pixt4, pixs, 1, size);
    pixErodeBrickDwa(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "erode", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

    if (TIMING) startTimer();
    pixOpenCompBrickDwa(pixt1, pixs, size, 1);
    pixOpenCompBrickDwa(pixt3, pixs, 1, size);
    pixOpenCompBrickDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixOpenBrickDwa(pixt2, pixs, size, 1);
    pixOpenBrickDwa(pixt4, pixs, 1, size);
    pixOpenBrickDwa(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "open", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

    if (TIMING) startTimer();
    pixCloseCompBrickDwa(pixt1, pixs, size, 1);
    pixCloseCompBrickDwa(pixt3, pixs, 1, size);
    pixCloseCompBrickDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixCloseBrickDwa(pixt2, pixs, size, 1);
    pixCloseBrickDwa(pixt4, pixs, 1, size);
    pixCloseBrickDwa(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "close", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

#if 0
    pixWrite("/tmp/junkpixt1.png", pixt1, IFF_PNG);
    pixWrite("/tmp/junkpixt2.png", pixt2, IFF_PNG);
    pixXor(pixt1, pixt1, pixt2);
    pixWrite("/tmp/junkxor.png", pixt1, IFF_PNG);
#endif

    return 0;
}


    /* dwa composite with morph composite */
l_int32
DoComparisonDwa4(PIX     *pixs,
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
    pixDilateCompBrickDwa(pixt1, pixs, size, 1);
    pixDilateCompBrickDwa(pixt3, pixs, 1, size);
    pixDilateCompBrickDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixDilateCompBrick(pixt2, pixs, size, 1);
    pixDilateCompBrick(pixt4, pixs, 1, size);
    pixDilateCompBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "dilate", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

/*    pixDisplay(pixt1, 100, 100); */
/*    pixDisplay(pixt2, 800, 100); */

    if (TIMING) startTimer();
    pixErodeCompBrickDwa(pixt1, pixs, size, 1);
    pixErodeCompBrickDwa(pixt3, pixs, 1, size);
    pixErodeCompBrickDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixErodeCompBrick(pixt2, pixs, size, 1);
    pixErodeCompBrick(pixt4, pixs, 1, size);
    pixErodeCompBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "erode", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

    if (TIMING) startTimer();
    pixOpenCompBrickDwa(pixt1, pixs, size, 1);
    pixOpenCompBrickDwa(pixt3, pixs, 1, size);
    pixOpenCompBrickDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixOpenCompBrick(pixt2, pixs, size, 1);
    pixOpenCompBrick(pixt4, pixs, 1, size);
    pixOpenCompBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "open", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

/*    pixDisplay(pixt1, 100, 100);   */
/*    pixDisplay(pixt2, 800, 100);   */
/*    pixWrite("/tmp/junkpixt1.png", pixt1, IFF_PNG);  */
/*    pixWrite("/tmp/junkpixt2.png", pixt2, IFF_PNG);  */

    if (TIMING) startTimer();
    pixCloseCompBrickDwa(pixt1, pixs, size, 1);
    pixCloseCompBrickDwa(pixt3, pixs, 1, size);
    pixCloseCompBrickDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixCloseSafeCompBrick(pixt2, pixs, size, 1);
    pixCloseSafeCompBrick(pixt4, pixs, 1, size);
    pixCloseSafeCompBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "close", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

    return 0;
}


    /* dwa composite with morph non-composite */
l_int32
DoComparisonDwa5(PIX     *pixs,
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
    pixDilateCompBrickDwa(pixt1, pixs, size, 1);
    pixDilateCompBrickDwa(pixt3, pixs, 1, size);
    pixDilateCompBrickDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixDilateBrick(pixt2, pixs, size, 1);
    pixDilateBrick(pixt4, pixs, 1, size);
    pixDilateBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "dilate", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

/*    pixDisplay(pixt1, 100, 100);  */
/*    pixDisplay(pixt2, 800, 100);  */

    if (TIMING) startTimer();
    pixErodeCompBrickDwa(pixt1, pixs, size, 1);
    pixErodeCompBrickDwa(pixt3, pixs, 1, size);
    pixErodeCompBrickDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixErodeBrick(pixt2, pixs, size, 1);
    pixErodeBrick(pixt4, pixs, 1, size);
    pixErodeBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "erode", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

    if (TIMING) startTimer();
    pixOpenCompBrickDwa(pixt1, pixs, size, 1);
    pixOpenCompBrickDwa(pixt3, pixs, 1, size);
    pixOpenCompBrickDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixOpenBrick(pixt2, pixs, size, 1);
    pixOpenBrick(pixt4, pixs, 1, size);
    pixOpenBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "open", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

    if (TIMING) startTimer();
    pixCloseCompBrickDwa(pixt1, pixs, size, 1);
    pixCloseCompBrickDwa(pixt3, pixs, 1, size);
    pixCloseCompBrickDwa(pixt5, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Dwa: %7.3f sec\n", stopTimer());
    if (TIMING) startTimer();
    pixCloseSafeBrick(pixt2, pixs, size, 1);
    pixCloseSafeBrick(pixt4, pixs, 1, size);
    pixCloseSafeBrick(pixt6, pixs, size, size);
    if (TIMING) fprintf(stderr, "Time Rop: %7.3f sec\n", stopTimer());
    PixCompareDwa(size, "close", pixt1, pixt2, pixt3, pixt4, pixt5, pixt6);

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

