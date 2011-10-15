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
 *   seedspread_reg.c
 *
 *   Tests the seedspreading (voronoi finding & filling) function
 *   for both 4 and 8 connectivity.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define  REDUCTION     1

main(int    argc,
     char **argv)
{
l_int32      i, j, x, y, val;
PIX         *pixsq, *pixs, *pixc, *pixd;
PIXA        *pixa;
static char  mainName[] = "seedspread_reg";

    pixsq = pixCreate(3, 3, 32);
    pixSetAllArbitrary(pixsq, 0x00ff0000);
    pixa = pixaCreate(6);

    pixs = pixCreate(300, 300, 8);
    for (i = 0; i < 100; i++) {
        x = (153 * i * i * i + 59) % 299;
        y = (117 * i * i * i + 241) % 299;
        val = (97 * i + 74) % 256;
        pixSetPixel(pixs, x, y, val);
    }
    pixd = pixSeedspread(pixs, 4);
    pixc = pixConvertTo32(pixd);
    for (i = 0; i < 100; i++) {
        x = (153 * i * i * i + 59) % 299;
        y = (117 * i * i * i + 241) % 299;
        pixRasterop(pixc, x - 1, y - 1, 3, 3, PIX_SRC, pixsq, 0, 0);
    }
    pixSaveTiled(pixc, pixa, REDUCTION, 1, 20, 32);
    pixWrite("/tmp/junkpix4-1.png", pixc, IFF_PNG);
    pixDisplay(pixc, 100, 100);
    pixDestroy(&pixd);
    pixDestroy(&pixc);
    pixd = pixSeedspread(pixs, 8);
    pixc = pixConvertTo32(pixd);
    for (i = 0; i < 100; i++) {
        x = (153 * i * i * i + 59) % 299;
        y = (117 * i * i * i + 241) % 299;
        pixRasterop(pixc, x - 1, y - 1, 3, 3, PIX_SRC, pixsq, 0, 0);
    }
    pixSaveTiled(pixc, pixa, REDUCTION, 0, 20, 0);
    pixWrite("/tmp/junkpix8-1.png", pixc, IFF_PNG);
    pixDisplay(pixc, 410, 100);
    pixDestroy(&pixd);
    pixDestroy(&pixc);
    pixDestroy(&pixs);

    pixs = pixCreate(200, 200, 8);
    for (i = 5; i <= 195; i += 10) {
        for (j = 5; j <= 195; j += 10) {
            pixSetPixel(pixs, i, j, (7 * i + 17 * j) % 255);
        }
    }
    pixd = pixSeedspread(pixs, 4);
    pixc = pixConvertTo32(pixd);
    for (i = 5; i <= 195; i += 10) {
        for (j = 5; j <= 195; j += 10) {
            pixRasterop(pixc, j - 1, i - 1, 3, 3, PIX_SRC, pixsq, 0, 0);
        }
    }
    pixSaveTiled(pixc, pixa, REDUCTION, 1, 20, 0);
    pixWrite("/tmp/junkpix4-2.png", pixc, IFF_PNG);
    pixDisplay(pixc, 100, 430);
    pixDestroy(&pixd);
    pixDestroy(&pixc);
    pixd = pixSeedspread(pixs, 8);
    pixc = pixConvertTo32(pixd);
    for (i = 5; i <= 195; i += 10) {
        for (j = 5; j <= 195; j += 10) {
            pixRasterop(pixc, j - 1, i - 1, 3, 3, PIX_SRC, pixsq, 0, 0);
        }
    }
    pixSaveTiled(pixc, pixa, REDUCTION, 0, 20, 0);
    pixWrite("/tmp/junkpix8-2.png", pixc, IFF_PNG);
    pixDisplay(pixc, 310, 430);
    pixDestroy(&pixd);
    pixDestroy(&pixc);
    pixDestroy(&pixs);

    pixs = pixCreate(200, 200, 8);
    pixSetPixel(pixs, 60, 20, 90);
    pixSetPixel(pixs, 160, 40, 130);
    pixSetPixel(pixs, 80, 80, 205);
    pixSetPixel(pixs, 40, 160, 115);
    pixd = pixSeedspread(pixs, 4);
    pixc = pixConvertTo32(pixd);
    pixRasterop(pixc, 60 - 1, 20 - 1, 3, 3, PIX_SRC, pixsq, 0, 0);
    pixRasterop(pixc, 160 - 1, 40 - 1, 3, 3, PIX_SRC, pixsq, 0, 0);
    pixRasterop(pixc, 80 - 1, 80 - 1, 3, 3, PIX_SRC, pixsq, 0, 0);
    pixRasterop(pixc, 40 - 1, 160 - 1, 3, 3, PIX_SRC, pixsq, 0, 0);
    pixSaveTiled(pixc, pixa, REDUCTION, 1, 20, 0);
    pixWrite("/tmp/junkpix4-3.png", pixc, IFF_PNG);
    pixDisplay(pixc, 100, 660);
    pixDestroy(&pixd);
    pixDestroy(&pixc);
    pixd = pixSeedspread(pixs, 8);
    pixc = pixConvertTo32(pixd);
    pixRasterop(pixc, 60 - 1, 20 - 1, 3, 3, PIX_SRC, pixsq, 0, 0);
    pixRasterop(pixc, 160 - 1, 40 - 1, 3, 3, PIX_SRC, pixsq, 0, 0);
    pixRasterop(pixc, 80 - 1, 80 - 1, 3, 3, PIX_SRC, pixsq, 0, 0);
    pixRasterop(pixc, 40 - 1, 160 - 1, 3, 3, PIX_SRC, pixsq, 0, 0);
    pixSaveTiled(pixc, pixa, REDUCTION, 0, 20, 0);
    pixWrite("/tmp/junkpix8-3.png", pixc, IFF_PNG);
    pixDisplay(pixc, 310, 660);
    pixDestroy(&pixd);
    pixDestroy(&pixc);
    pixDestroy(&pixs);
    pixDestroy(&pixsq);

    pixd = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/junkpixd.png", pixd, IFF_PNG);
    pixDisplay(pixd, 720, 100);

    pixaDestroy(&pixa);
    pixDestroy(&pixd);
    return 0;
}

