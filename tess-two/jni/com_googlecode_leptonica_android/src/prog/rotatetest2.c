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
 *   rotatetest2.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define   BINARY_IMAGE        "test1.png"
#define   GRAYSCALE_IMAGE     "test8.jpg"
#define   FOUR_BPP_IMAGE      "weasel4.8g.png"
#define   COLORMAP_IMAGE      "dreyfus8.png"
#define   RGB_IMAGE           "marge.jpg"

static const l_int32    MODSIZE = 7;  /* set to 7 for display */

static const l_float32  ANGLE1 = 3.14159265 / 12;
static const l_int32    NTIMES = 24;

void rotateTest(const char *fname, PIXA *pixa);


main(int    argc,
     char **argv)
{
PIX         *pixd;
PIXA        *pixa;
static char  mainName[] = "rotatetest2";

    if (argc != 1)
	exit(ERROR_INT(" Syntax:  rotatetest2", mainName, 1));

    pixa = pixaCreate(0);

    fprintf(stderr, "Test binary image:\n");
    rotateTest((char *)BINARY_IMAGE, pixa);
    fprintf(stderr, "Test 4 bpp colormapped image:\n");
    rotateTest((char *)FOUR_BPP_IMAGE, pixa);
    fprintf(stderr, "Test grayscale image:\n");
    rotateTest((char *)GRAYSCALE_IMAGE, pixa);
    fprintf(stderr, "Test colormap image:\n");
    rotateTest((char *)COLORMAP_IMAGE, pixa);
    fprintf(stderr, "Test rgb image:\n");
    rotateTest((char *)RGB_IMAGE, pixa);

        /* Display results */
    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/junkrot2.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    return 0;
}


void
rotateTest(const char *fname,
           PIXA       *pixa)
{
l_int32   w, h, d, i;
PIX      *pixs, *pixt, *pixd1, *pixd2;
PIXCMAP  *cmap;

    PROCNAME("rotateTest");

    if ((pixs = pixRead(fname)) == NULL) {
        L_ERROR("pixs not read", procName);
        return;
    }

    pixGetDimensions(pixs, &w, &h, &d);
    cmap = pixGetColormap(pixs);
    pixd1 = pixRotate(pixs, ANGLE1, L_ROTATE_SHEAR, L_BRING_IN_WHITE, w, h);
    for (i = 1; i < NTIMES; i++) {
        if (((i + MODSIZE - 1) % MODSIZE) == 0)
            pixSaveTiledOutline(pixd1, pixa, 1, (i == 1) ? 1 : 0, 20, 2, 32);
        pixt = pixRotate(pixd1, ANGLE1, L_ROTATE_SHEAR,
                         L_BRING_IN_WHITE, w, h);
        pixDestroy(&pixd1);
        pixd1 = pixt;
    }

    pixd2 = pixRotate(pixs, ANGLE1, L_ROTATE_AREA_MAP, L_BRING_IN_WHITE, w, h);
    for (i = 1; i < NTIMES; i++) {
        if (((i + MODSIZE - 1) % MODSIZE) == 0)
            pixSaveTiledOutline(pixd2, pixa, 1, (i == 1) ? 1 : 0, 20, 2, 32);
        pixt = pixRotate(pixd2, ANGLE1, L_ROTATE_AREA_MAP,
                         L_BRING_IN_WHITE, w, h);
        pixDestroy(&pixd2);
        pixd2 = pixt;
    }

    if (d == 1) {
        pixWrite("/tmp/junkbin1", pixd1, IFF_PNG);
        pixWrite("/tmp/junkbin2", pixd2, IFF_PNG);
    } else if (cmap) {
        pixWrite("/tmp/junkcmap1", pixd1, IFF_PNG);
        pixWrite("/tmp/junkcmap2", pixd2, IFF_PNG);
    } else if (d == 8) {
        pixWrite("/tmp/junkgray1", pixd1, IFF_JFIF_JPEG);
        pixWrite("/tmp/junkgray2", pixd2, IFF_JFIF_JPEG);
    } else if (d == 32) {
        pixWrite("/tmp/junkrgb1", pixd1, IFF_JFIF_JPEG);
        pixWrite("/tmp/junkrgb2", pixd2, IFF_JFIF_JPEG);
    }

    pixDestroy(&pixd1);
    pixDestroy(&pixd2);
    pixDestroy(&pixs);
    return;
}


