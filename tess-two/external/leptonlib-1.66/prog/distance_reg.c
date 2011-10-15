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
 * distance_reg.c
 *
 *   This tests pixDistanceFunction for a variety of usage
 *   with all 8 combinations of these parameters:
 *
 *     connectivity :   4 or 8
 *     dest depth :     8 or 16
 *     boundary cond :  L_BOUNDARY_BG or L_BOUNDARY_FG
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static void TestDistance(PIXA *pixa, PIX *pixs, l_int32 conn,
                         l_int32 depth, l_int32 bc, l_int32 *pcount,
                         L_REGPARAMS *rp);

main(int    argc,
     char **argv)
{
char          buf[256];
l_int32       i, j, k, index, conn, depth, bc, success, display, count;
BOX          *box;
FILE         *fp;
PIX          *pix, *pixs, *pixd;
PIXA         *pixa;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &fp, &display, &success, &rp))
        return 1;

    pix = pixRead("feyn.tif");
    box = boxCreate(383, 338, 1480, 1050);
    pixs = pixClipRectangle(pix, box, NULL);
    count = 0;
    regTestWritePixAndCheck(pixs, IFF_PNG, &count, rp);
	    
    for (i = 0; i < 2; i++) {
        conn = 4 + 4 * i;
        for (j = 0; j < 2; j++) {
            depth = 8 + 8 * j;
            for (k = 0; k < 2; k++) {
                bc = k + 1;
                index = 4 * i + 2 * j + k;
                fprintf(stderr, "Set %d\n", index);
                pixa = pixaCreate(0);
                pixSaveTiled(pixs, pixa, 1, 1, 20, 8);
                TestDistance(pixa, pixs, conn, depth, bc, &count, rp);
                pixd = pixaDisplay(pixa, 0, 0);
                pixDisplayWithTitle(pixd, 0, 0, NULL, display);
                pixaDestroy(&pixa);
                pixDestroy(&pixd);
            }
        }
    }

    boxDestroy(&box);
    pixDestroy(&pix);
    pixDestroy(&pixs);
    regTestCleanup(argc, argv, fp, success, rp);
    return 0;
}


static void
TestDistance(PIXA         *pixa,
             PIX          *pixs,
             l_int32       conn,
             l_int32       depth,
             l_int32       bc,
             l_int32      *pcount,
             L_REGPARAMS  *rp)
{
PIX  *pixt1, *pixt2, *pixt3, *pixt4, *pixt5;

        /* Test the distance function and display */
    pixInvert(pixs, pixs);
    pixt1 = pixDistanceFunction(pixs, conn, depth, bc);
    regTestWritePixAndCheck(pixt1, IFF_PNG, pcount, rp);
    pixSaveTiled(pixt1, pixa, 1, 1, 20, 0);
    pixInvert(pixs, pixs);
    pixt2 = pixMaxDynamicRange(pixt1, L_LOG_SCALE);
    regTestWritePixAndCheck(pixt2, IFF_JFIF_JPEG, pcount, rp);
    pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

	/* Test the distance function and display with contour rendering */
    pixInvert(pixs, pixs);
    pixt1 = pixDistanceFunction(pixs, conn, depth, bc);
    regTestWritePixAndCheck(pixt1, IFF_PNG, pcount, rp);
    pixSaveTiled(pixt1, pixa, 1, 1, 20, 0);
    pixInvert(pixs, pixs);
    pixt2 = pixRenderContours(pixt1, 2, 4, 1);  /* binary output */
    regTestWritePixAndCheck(pixt2, IFF_PNG, pcount, rp);
    pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);
    pixt3 = pixRenderContours(pixt1, 2, 4, depth);
    pixt4 = pixMaxDynamicRange(pixt3, L_LINEAR_SCALE);
    regTestWritePixAndCheck(pixt4, IFF_JFIF_JPEG, pcount, rp);
    pixSaveTiled(pixt4, pixa, 1, 0, 20, 0);
    pixt5 = pixMaxDynamicRange(pixt3, L_LOG_SCALE);
    regTestWritePixAndCheck(pixt5, IFF_JFIF_JPEG, pcount, rp);
    pixSaveTiled(pixt5, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);

	/* Label all pixels in each c.c. with a color equal to the
         * max distance of any pixel within that c.c. from the bg.
         * Note that we've normalized so the dynamic range extends
         * to 255.  For the image here, each unit of distance is
         * represented by about 21 grayscale units.  The largest
         * distance is 12.  */
    if (depth == 8) {
        pixt1 = pixDistanceFunction(pixs, conn, depth, bc);
        pixt4 = pixMaxDynamicRange(pixt1, L_LOG_SCALE);
        regTestWritePixAndCheck(pixt4, IFF_JFIF_JPEG, pcount, rp);
        pixSaveTiled(pixt4, pixa, 1, 1, 20, 0);
        pixt2 = pixCreateTemplate(pixt1);
        pixSetMasked(pixt2, pixs, 255);
        regTestWritePixAndCheck(pixt2, IFF_JFIF_JPEG, pcount, rp);
        pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);
        pixSeedfillGray(pixt1, pixt2, 4);
        pixt3 = pixMaxDynamicRange(pixt1, L_LINEAR_SCALE);
        regTestWritePixAndCheck(pixt3, IFF_JFIF_JPEG, pcount, rp);
        pixSaveTiled(pixt3, pixa, 1, 0, 20, 0);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixt3);
        pixDestroy(&pixt4);
    }

    return;
}


