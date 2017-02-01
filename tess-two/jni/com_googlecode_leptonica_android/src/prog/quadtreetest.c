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
 * quadtreetest.c
 *
 *     test of quadtree statistical functions
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
l_int32      i, j, w, h, error;
l_float32    val1, val2;
l_float32    val00, val10, val01, val11, valc00, valc10, valc01, valc11;
PIX         *pixs, *pixg, *pixt1, *pixt2, *pixt3, *pixt4, *pixt5;
FPIXA       *fpixam, *fpixav, *fpixarv;
BOXAA       *baa;
static char  mainName[] = "quadtreetest";

    if (argc != 1)
        return ERROR_INT(" Syntax:  quadtreetest", mainName, 1);

    lept_mkdir("lept/quad");

        /* Test generation of quadtree regions. */
    baa = boxaaQuadtreeRegions(1000, 500, 3);
    boxaaWriteStream(stderr, baa);
    boxaaDestroy(&baa);
    baa = boxaaQuadtreeRegions(1001, 501, 3);
    boxaaWriteStream(stderr, baa);
    boxaaDestroy(&baa);

        /* Test quadtree stats generation */
#if 1
    pixs = pixRead("rabi.png");
    pixg = pixScaleToGray4(pixs);
#else
    pixs = pixRead("test24.jpg");
    pixg = pixConvertTo8(pixs, 0);
#endif
    pixQuadtreeMean(pixg, 8, NULL, &fpixam);
    pixt1 = fpixaDisplayQuadtree(fpixam, 2, 10);
    pixDisplay(pixt1, 100, 0);
    pixWrite("/tmp/lept/quad/tree1.png", pixt1, IFF_PNG);
    pixQuadtreeVariance(pixg, 8, NULL, NULL, &fpixav, &fpixarv);
    pixt2 = fpixaDisplayQuadtree(fpixav, 2, 10);
    pixDisplay(pixt2, 100, 200);
    pixWrite("/tmp/lept/quad/tree2.png", pixt2, IFF_PNG);
    pixt3 = fpixaDisplayQuadtree(fpixarv, 2, 10);
    pixDisplay(pixt3, 100, 400);
    pixWrite("/tmp/lept/quad/tree3.png", pixt3, IFF_PNG);

        /* Compare with fixed-size tiling at a resolution corresponding
         * to the deepest level of the quadtree above */
    pixt4 = pixGetAverageTiled(pixg, 5, 6, L_MEAN_ABSVAL);
    pixt5 = pixExpandReplicate(pixt4, 4);
    pixWrite("/tmp/lept/quad/tree4.png", pixt5, IFF_PNG);
    pixDisplay(pixt5, 800, 0);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);
    pixt4 = pixGetAverageTiled(pixg, 5, 6, L_STANDARD_DEVIATION);
    pixt5 = pixExpandReplicate(pixt4, 4);
    pixWrite("/tmp/lept/quad/tree5.png", pixt5, IFF_PNG);
    pixDisplay(pixt5, 800, 400);

        /* Test quadtree parent/child access */
    error = FALSE;
    fpixaGetFPixDimensions(fpixam, 4, &w, &h);
    for (i = 0; i < w; i += 2) {
        for (j = 0; j < h; j += 2) {
            quadtreeGetParent(fpixam, 4, j, i, &val1);
            fpixaGetPixel(fpixam, 3, j / 2, i / 2, &val2);
            if (val1 != val2) error = TRUE;
        }
    }
    if (error)
        fprintf(stderr, "\n======================\nError: parent access\n");
    else
        fprintf(stderr, "\n======================\nSuccess: parent access\n");
    error = FALSE;
    for (i = 0; i < w; i++) {
        for (j = 0; j < h; j++) {
            quadtreeGetChildren(fpixam, 4, j, i,
                                &val00, &val10, &val01, &val11);
            fpixaGetPixel(fpixam, 5, 2 * j, 2 * i, &valc00);
            fpixaGetPixel(fpixam, 5, 2 * j + 1, 2 * i, &valc10);
            fpixaGetPixel(fpixam, 5, 2 * j, 2 * i + 1, &valc01);
            fpixaGetPixel(fpixam, 5, 2 * j + 1, 2 * i + 1, &valc11);
            if ((val00 != valc00) || (val10 != valc10) ||
                (val01 != valc01) || (val11 != valc11))
                error = TRUE;
        }
    }
    if (error)
        fprintf(stderr, "Error: child access\n======================\n");
    else
        fprintf(stderr, "Success: child access\n======================\n");

    pixDestroy(&pixs);
    pixDestroy(&pixg);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);
    fpixaDestroy(&fpixam);
    fpixaDestroy(&fpixav);
    fpixaDestroy(&fpixarv);
    return 0;
}


