/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -  This software is distributed in the hope that it will be
 -  useful, but with NO WARRANTY OF ANY KIND.
 -  No author or distributor accepts responsibility to anyone for the
 -  consequences of using this software, or for whether it serves any
 -  particular purpose or works at all, unless he or she says so in
 -  writing.  Everyone is granted permission to copy, modify and
 -  redistribute this source code, for commercial or non-commercial
 -  purposes, with the following restrictions: (1) the origin of this
 -  source code must not be misrepresented; (2) modified versions must
 -  be plainly marked as such; and (3) this notice may not be removed
 -  or altered from any source or modified source distribution.
 *====================================================================*/

/*
 * quadtreetest.c
 *
 *     test of quadtree statistical functions
 */

#include "allheaders.h"

main(int    argc,
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
    pixt1 = fpixaDisplayQuadtree(fpixam, 4);
    pixDisplay(pixt1, 100, 0);
    pixWrite("/tmp/quadtree1.png", pixt1, IFF_PNG);
    pixQuadtreeVariance(pixg, 8, NULL, NULL, &fpixav, &fpixarv);
    pixt2 = fpixaDisplayQuadtree(fpixav, 4);
    pixDisplay(pixt2, 100, 200);
    pixWrite("/tmp/quadtree2.png", pixt2, IFF_PNG);
    pixt3 = fpixaDisplayQuadtree(fpixarv, 4);
    pixDisplay(pixt3, 100, 400);
    pixWrite("/tmp/quadtree3.png", pixt3, IFF_PNG);

        /* Compare with fixed-size tiling at a resolution corresponding
         * to the deepest level of the quadtree above */
    pixt4 = pixGetAverageTiled(pixg, 5, 6, L_MEAN_ABSVAL);
    pixt5 = pixExpandReplicate(pixt4, 4);
    pixWrite("/tmp/quadtree4.png", pixt5, IFF_PNG);
    pixDisplay(pixt5, 800, 0);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);
    pixt4 = pixGetAverageTiled(pixg, 5, 6, L_STANDARD_DEVIATION);
    pixt5 = pixExpandReplicate(pixt4, 4);
    pixWrite("/tmp/quadtree5.png", pixt5, IFF_PNG);
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


