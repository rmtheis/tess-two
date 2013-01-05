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
 * snapcolortest.c
 *
 *    This tests the color snapping in blend.c.
 *    It is used here to color the background on images in index.html.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const l_uint32   LEPTONICA_YELLOW = 0xffffe400;

main(int    argc,
     char **argv)
{
PIX         *pixs, *pixc, *pixd;
PIXA        *pixa;
static char  mainName[] = "snapcolortest";

    if (argc != 1)
	exit(ERROR_INT(" Syntax:  snapcolortest", mainName, 1));

    if ((pixs = pixRead("Leptonica.jpg")) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));

    pixa = pixaCreate(0);

        /* First, snap the color directly on the input rgb image. */
    pixSaveTiledOutline(pixs, pixa, 1, 1, 25, 2, 32);
    pixd = pixSnapColor(NULL, pixs, 0xffffff00, LEPTONICA_YELLOW, 30);
    pixSaveTiledOutline(pixd, pixa, 1, 0, 25, 2, 32);
    pixWrite("/tmp/junklogo1.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);

        /* Then make a colormapped version and snap the color */
    pixd = pixOctreeQuantNumColors(pixs, 250, 0);
    pixSaveTiledOutline(pixd, pixa, 1, 1, 25, 2, 32);
    pixSnapColor(pixd, pixd, 0xffffff00, LEPTONICA_YELLOW, 30);
    pixSaveTiledOutline(pixd, pixa, 1, 0, 25, 2, 32);
    pixWrite("/tmp/junklogo2.png", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixDestroy(&pixs);


        /* Set the background of the google searchbox to yellow.
	 * The input image is colormapped with all 256 colors used. */
    if ((pixs = pixRead("google-searchbox.png")) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
    pixSaveTiledOutline(pixs, pixa, 1, 1, 25, 2, 32);
    pixd = pixSnapColor(NULL, pixs, 0xffffff00, LEPTONICA_YELLOW, 30);
    pixSaveTiledOutline(pixd, pixa, 1, 0, 25, 2, 32);
    pixWrite("/tmp/junklogo3.png", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixDestroy(&pixs);

        /* A couple of more, setting pixels near white to strange colors */
    pixs = pixRead("weasel4.11c.png");
    pixSaveTiledOutline(pixs, pixa, 1, 1, 25, 2, 32);
    pixd = pixSnapColor(NULL, pixs, 0xfefefe00, 0x80800000, 50);
    pixSaveTiledOutline(pixd, pixa, 1, 0, 25, 2, 32);
    pixDestroy(&pixs);
    pixDestroy(&pixd);

    pixs = pixRead("wyom.jpg");
    pixc = pixFixedOctcubeQuant256(pixs, 0);
    pixSaveTiledOutline(pixc, pixa, 1, 1, 25, 2, 32);
    pixd = pixSnapColor(NULL, pixc, 0xf0f0f000, 0x80008000, 100);
    pixSaveTiledOutline(pixd, pixa, 1, 0, 25, 2, 32);
    pixDestroy(&pixs);
    pixDestroy(&pixc);
    pixDestroy(&pixd);

        /* --- Display results --- */
    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/junksnap.jpg", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    return 0;
}

