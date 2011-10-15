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
 * pixa2_reg.c
 *
 *    Tests various replacement functions on pixa.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
const char  *name;
l_int32      i, n, success, display;
BOX         *box;
FILE        *fp;
PIX         *pix, *pixt0, *pixt1, *pixd;
PIXA        *pixa;
SARRAY      *sa1, *sa2, *sa3, *sa4, *sa;
static char  mainName[] = "pixa2_reg";

    if (regTestSetup(argc, argv, &fp, &display, &success, NULL))
        return 1;

    /* ----------------  Find all the jpg and tif images --------------- */
    sa1 = getSortedPathnamesInDirectory(".", ".jpg", 0, 0);
    sa2 = getSortedPathnamesInDirectory(".", ".tif", 0, 0);
    sa3 = sarraySelectByRange(sa1, 0, 9);
    sa4 = sarraySelectByRange(sa2, 0, 9);
    sarrayConcatenate(sa3, sa4);
    n =sarrayGetCount(sa3);
    sarrayDestroy(&sa1);
    sarrayDestroy(&sa2);
    sarrayDestroy(&sa4);
    
    /* ---------------- Use replace to fill up a pixa -------------------*/
    pixa = pixaCreate(1);
    pixaExtendArrayToSize(pixa, n);
    pixt0 = pixRead("marge.jpg");
    pixt1 = pixScaleToSize(pixt0, 144, 108);  /* scale 0.25 */
    pixDestroy(&pixt0);
    pixaInitFull(pixa, pixt1, NULL);  /* fill it up */
    pixd = pixaDisplayTiledInRows(pixa, 32, 1000, 1.0, 0, 25, 2);
    pixDisplayWithTitle(pixd, 100, 100, NULL, display);
    pixWrite("/tmp/pix1.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixt1);
    pixDestroy(&pixd);

    /* ---------------- And again with jpgs and tifs -------------------*/
    for (i = 0; i < n; i++) {
        name = sarrayGetString(sa3, i, L_NOCOPY);
        pixt0 = pixRead(name);
        pixt1 = pixScaleToSize(pixt0, 144, 108);
        pixaReplacePix(pixa, i, pixt1, NULL);
        pixDestroy(&pixt0);
    }
    pixd = pixaDisplayTiledInRows(pixa, 32, 1000, 1.0, 0, 25, 2);
    pixDisplayWithTitle(pixd, 400, 100, NULL, display);
    pixWrite("/tmp/pix2.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);

    /* ---------------- And again, reversing the order ------------------*/
    pix = pixCreate(1, 1, 1);
    box = boxCreate(0, 0, 0, 0);
    pixaInitFull(pixa, pix, box);
    pixDestroy(&pix);
    boxDestroy(&box);
    for (i = 0; i < n; i++) {
        name = sarrayGetString(sa3, i, L_NOCOPY);
        pixt0 = pixRead(name);
        pixt1 = pixScaleToSize(pixt0, 144, 108);
        pixaReplacePix(pixa, n - 1 - i, pixt1, NULL);
        pixDestroy(&pixt0);
    }
    pixd = pixaDisplayTiledInRows(pixa, 32, 1000, 1.0, 0, 25, 2);
    pixDisplayWithTitle(pixd, 700, 100, NULL, display);
    pixWrite("/tmp/pix3.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);

    pixaDestroy(&pixa);
    regTestCleanup(argc, argv, fp, success, NULL);  /* always returns success */
    return 0;
}


