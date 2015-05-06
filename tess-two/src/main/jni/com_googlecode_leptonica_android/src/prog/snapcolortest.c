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
 * snapcolortest.c
 *
 *    This tests the color snapping in blend.c.
 *    It is used here to color the background on images in index.html.
 */

#include "allheaders.h"

static const l_uint32   LEPTONICA_YELLOW = 0xffffe400;

int main(int    argc,
         char **argv)
{
PIX         *pixs, *pixc, *pixd;
PIXA        *pixa;
static char  mainName[] = "snapcolortest";

    if (argc != 1)
        return ERROR_INT(" Syntax:  snapcolortest", mainName, 1);

    if ((pixs = pixRead("Leptonica.jpg")) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);
    pixa = pixaCreate(0);

        /* First, snap the color directly on the input rgb image. */
    pixSaveTiledOutline(pixs, pixa, 1.0, 1, 25, 2, 32);
    pixd = pixSnapColor(NULL, pixs, 0xffffff00, LEPTONICA_YELLOW, 30);
    pixSaveTiledOutline(pixd, pixa, 1.0, 0, 25, 2, 32);
    pixWrite("/tmp/logo1.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);

        /* Then make a colormapped version and snap the color */
    pixd = pixOctreeQuantNumColors(pixs, 250, 0);
    pixSaveTiledOutline(pixd, pixa, 1.0, 1, 25, 2, 32);
    pixSnapColor(pixd, pixd, 0xffffff00, LEPTONICA_YELLOW, 30);
    pixSaveTiledOutline(pixd, pixa, 1.0, 0, 25, 2, 32);
    pixWrite("/tmp/logo2.png", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixDestroy(&pixs);


        /* Set the background of the google searchbox to yellow.
         * The input image is colormapped with all 256 colors used. */
    if ((pixs = pixRead("google-searchbox.png")) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);
    pixSaveTiledOutline(pixs, pixa, 1.0, 1, 25, 2, 32);
    pixd = pixSnapColor(NULL, pixs, 0xffffff00, LEPTONICA_YELLOW, 30);
    pixSaveTiledOutline(pixd, pixa, 1.0, 0, 25, 2, 32);
    pixWrite("/tmp/logo3.png", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixDestroy(&pixs);

        /* A couple of more, setting pixels near white to strange colors */
    pixs = pixRead("weasel4.11c.png");
    pixSaveTiledOutline(pixs, pixa, 1.0, 1, 25, 2, 32);
    pixd = pixSnapColor(NULL, pixs, 0xfefefe00, 0x80800000, 50);
    pixSaveTiledOutline(pixd, pixa, 1.0, 0, 25, 2, 32);
    pixDestroy(&pixs);
    pixDestroy(&pixd);

    pixs = pixRead("wyom.jpg");
    pixc = pixFixedOctcubeQuant256(pixs, 0);
    pixSaveTiledOutline(pixc, pixa, 1.0, 1, 25, 2, 32);
    pixd = pixSnapColor(NULL, pixc, 0xf0f0f000, 0x80008000, 100);
    pixSaveTiledOutline(pixd, pixa, 1.0, 0, 25, 2, 32);
    pixDestroy(&pixs);
    pixDestroy(&pixc);
    pixDestroy(&pixd);

        /* --- Display results --- */
    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/snap.jpg", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    return 0;
}

