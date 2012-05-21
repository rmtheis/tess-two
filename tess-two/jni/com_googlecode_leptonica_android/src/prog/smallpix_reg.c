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
 *  smallpix_reg.c
 *
 *  This is a regression test for scaling and rotation.
 *
 *  The question to be answered is: in the quantization, where, if
 *  anywhere, do we add 0.5?
 *
 *  The answer is that it should usually, but not always, be omitted.
 *  To see this, we operate on a very small pix and for visualization,
 *  scale up with replication to avoid aliasing and shifting.
 *
 *  To determine that the current implementations in scalelow.c,
 *  rotate.c and rotateamlow.c are better, change the specific
 *  implementations and re-run.
 *
 *  In all cases here, the pix to be operated on is of odd size
 *  so that the center pixel is symmetrically located, and there
 *  are a couple of black pixels outside the pattern so that edge
 *  effects (e.g., in pixScaleSmooth()) do not affect the results.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


void DisplayPix(PIXA **ppixa, l_int32 x, l_int32 y, char *fname)
{
PIX  *pixt;

    pixt = pixaDisplay(*ppixa, 0, 0);
    if (fname)
        pixWrite(fname, pixt, IFF_PNG);
    pixDisplay(pixt, x, y);
    pixDestroy(&pixt);
    pixaDestroy(ppixa);
}


main(int    argc,
     char **argv)
{
l_int32      i;
l_float32    pi, scale, angle;
PIX         *pixc, *pixm, *pix1, *pix2, *pix3;
PIXA        *pixa;
PTA         *pta1, *pta2, *pta3, *pta4;
static char  mainName[] = "smallpix_reg";

        /* Make a small test image, the hard way! */
    pi = 3.1415926535;
    pixc = pixCreate(9, 9, 32);
    pixm = pixCreate(9, 9, 1);
    pta1 = generatePtaLineFromPt(4, 4, 3.1, 0.0);
    pta2 = generatePtaLineFromPt(4, 4, 3.1, 0.5 * pi);
    pta3 = generatePtaLineFromPt(4, 4, 3.1, pi);
    pta4 = generatePtaLineFromPt(4, 4, 3.1, 1.5 * pi);
    ptaJoin(pta1, pta2, 0, 0);
    ptaJoin(pta1, pta3, 0, 0);
    ptaJoin(pta1, pta4, 0, 0);
    pixRenderPta(pixm, pta1, L_SET_PIXELS);
    pixPaintThroughMask(pixc, pixm, 0, 0, 0x00ff0000);
    ptaDestroy(&pta1);
    ptaDestroy(&pta2);
    ptaDestroy(&pta3);
    ptaDestroy(&pta4);
    pixDestroy(&pixm);

        /* Results differ for scaleSmoothLow() w/ and w/out + 0.5.
         * Neither is properly symmetric (with symm pattern on odd-sized
         * pix, because the smoothing is destroying the symmetry. */
    pixa = pixaCreate(11);
    pix1 = pixExpandReplicate(pixc, 2);
    for (i = 0; i < 11; i++) {
        scale = 0.30 + 0.035 * (l_float32)i;
        pix2 = pixScaleSmooth(pix1, scale, scale);
        pix3 = pixExpandReplicate(pix2, 6);
        pixSaveTiled(pix3, pixa, 1, (i == 0), 20, 32);
        pixDestroy(&pix2);
        pixDestroy(&pix3);
    }
    pixDestroy(&pix1);
    DisplayPix(&pixa, 100, 100, NULL);

        /* Results same for pixScaleAreaMap w/ and w/out + 0.5 */
    pixa = pixaCreate(11);
    pix1 = pixExpandReplicate(pixc, 2);
    for (i = 0; i < 11; i++) {
        scale = 0.30 + 0.035 * (l_float32)i;
        pix2 = pixScaleAreaMap(pix1, scale, scale);
        pix3 = pixExpandReplicate(pix2, 6);
        pixSaveTiled(pix3, pixa, 1, (i == 0), 20, 32);
        pixDestroy(&pix2);
        pixDestroy(&pix3);
    }
    pixDestroy(&pix1);
    DisplayPix(&pixa, 100, 200, NULL);

        /* Results better for pixScaleBySampling with + 0.5, for small,
         * odd-dimension pix.  */
    pixa = pixaCreate(11);
    pix1 = pixExpandReplicate(pixc, 2);
    for (i = 0; i < 11; i++) {
        scale = 0.30 + 0.035 * (l_float32)i;
        pix2 = pixScaleBySampling(pix1, scale, scale);
        pix3 = pixExpandReplicate(pix2, 6);
        pixSaveTiled(pix3, pixa, 1, (i == 0), 20, 32);
        pixDestroy(&pix2);
        pixDestroy(&pix3);
    }
    pixDestroy(&pix1);
    DisplayPix(&pixa, 100, 300, NULL);

        /* Results same for pixRotateAM w/ and w/out + 0.5 */
    pixa = pixaCreate(11);
    pix1 = pixExpandReplicate(pixc, 1);
    for (i = 0; i < 11; i++) {
        angle = 0.10 + 0.05 * (l_float32)i;
        pix2 = pixRotateAM(pix1, angle, L_BRING_IN_BLACK);
        pix3 = pixExpandReplicate(pix2, 8);
        pixSaveTiled(pix3, pixa, 1, (i == 0), 20, 32);
        pixDestroy(&pix2);
        pixDestroy(&pix3);
    }
    pixDestroy(&pix1);
    DisplayPix(&pixa, 100, 400, NULL);

        /* If the size is odd, we express the center exactly, and the
         * results are better for pixRotateBySampling() w/out 0.5
         * However, if the size is even, the center value is not
         * exact, and if we choose it 0.5 smaller than the actual
         * center, we get symmetrical results with +0.5. 
         * So we choose not to include + 0.5. */
    pixa = pixaCreate(11);
    pix1 = pixExpandReplicate(pixc, 1);
    for (i = 0; i < 11; i++) {
        angle = 0.10 + 0.05 * (l_float32)i;
        pix2 = pixRotateBySampling(pix1, 4, 4, angle, L_BRING_IN_BLACK);
        pix3 = pixExpandReplicate(pix2, 8);
        pixSaveTiled(pix3, pixa, 1, (i == 0), 20, 32);
        pixDestroy(&pix2);
        pixDestroy(&pix3);
    }
    pixDestroy(&pix1);
    DisplayPix(&pixa, 100, 500, NULL);

        /* Results same for pixRotateAMCorner w/ and w/out + 0.5 */
    pixa = pixaCreate(11);
    pix1 = pixExpandReplicate(pixc, 1);
    for (i = 0; i < 11; i++) {
        angle = 0.10 + 0.05 * (l_float32)i;
        pix2 = pixRotateAMCorner(pix1, angle, L_BRING_IN_BLACK);
        pix3 = pixExpandReplicate(pix2, 8);
        pixSaveTiled(pix3, pixa, 1, (i == 0), 20, 32);
        pixDestroy(&pix2);
        pixDestroy(&pix3);
    }
    pixDestroy(&pix1);
    DisplayPix(&pixa, 100, 600, NULL);

        /* Results better for pixRotateAMColorFast without + 0.5 */
    pixa = pixaCreate(11);
    pix1 = pixExpandReplicate(pixc, 1);
    for (i = 0; i < 11; i++) {
        angle = 0.10 + 0.05 * (l_float32)i;
        pix2 = pixRotateAMColorFast(pix1, angle, 0);
        pix3 = pixExpandReplicate(pix2, 8);
        pixSaveTiled(pix3, pixa, 1, (i == 0), 20, 32);
        pixDestroy(&pix2);
        pixDestroy(&pix3);
    }
    pixDestroy(&pix1);
    DisplayPix(&pixa, 100, 700, NULL);

        /* Results slightly better for pixScaleColorLI() w/out + 0.5 */
    pixa = pixaCreate(11);
    pix1 = pixExpandReplicate(pixc, 1);
    for (i = 0; i < 11; i++) {
        scale = 1.0 + 0.2 * (l_float32)i;
        pix2 = pixScaleColorLI(pix1, scale, scale);
        pix3 = pixExpandReplicate(pix2, 4);
        pixSaveTiled(pix3, pixa, 1, (i == 0), 20, 32);
        pixDestroy(&pix2);
        pixDestroy(&pix3);
    }
    pixDestroy(&pix1);
    DisplayPix(&pixa, 100, 800, NULL);

        /* Results slightly better for pixScaleColorLI() w/out + 0.5 */
    pixa = pixaCreate(11);
    pix1 = pixExpandReplicate(pixc, 1);
    for (i = 0; i < 11; i++) {
        scale = 1.0 + 0.2 * (l_float32)i;
        pix2 = pixScaleLI(pix1, scale, scale);
        pix3 = pixExpandReplicate(pix2, 4);
        pixSaveTiled(pix3, pixa, 1, (i == 0), 20, 32);
        pixDestroy(&pix2);
        pixDestroy(&pix3);
    }
    pixDestroy(&pix1);
    DisplayPix(&pixa, 100, 940, NULL);

    pixDestroy(&pixc);
    return 0;
}


