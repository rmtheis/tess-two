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

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

/*
 *  paintmask_reg.c
 *
 *    Regression test for painting through a mask onto various
 *    depth images.
 *
 *    This file shows how one can start with a 32 bpp RGB image and
 *    derive from it the following:
 *       8 bpp color, cmapped
 *       4 bpp color, cmapped
 *       2 bpp color, cmapped
 *       8 bpp gray
 *       4 bpp gray
 *       4 bpp gray, cmapped
 *       2 bpp gray
 *       2 bpp gray, cmapped
 *
 *    For each of these, pixClipMasked() is used to place a 1 bpp
 *    mask over part of the image, clip out the rectangular region
 *    supporting the mask, and paint a given color through the
 *    mask onto the result.
 *
 *    Finally we do a clip/mask operation on 1 bpp sources.
 */


main(int    argc,
char **argv)
{
BOX         *box;
PIX         *pixs, *pixs8, *pixm, *pixg, *pixt1, *pixt2, *pixd;
static char  mainName[] = "paintmask_reg";

    pixDisplayWrite(NULL, -1);  /* reset */

        /* Start with a 32 bpp image and a mask.  Use the
	 * same mask for all clip/masked operations. */
    pixs = pixRead("test24.jpg");
    pixt1 = pixRead("rabi.png");
    box = boxCreate(303, 1983, 800, 500);
    pixm = pixClipRectangle(pixt1, box, NULL);
    pixInvert(pixm, pixm);
    boxDestroy(&box);
    box = boxCreate(100, 100, 800, 500);  /* clips on pixs and derivatives */
    pixt2 = pixClipRectangle(pixs, box, NULL);
    pixDisplayWrite(pixt2, 1);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

        /* Clip 32 bpp RGB */
    pixd = pixClipMasked(pixs, pixm, 100, 100, 0x03c08000);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);

        /* Clip 8 bpp colormapped */
    pixt1 = pixMedianCutQuant(pixs, 0);
    pixt2 = pixClipRectangle(pixt1, box, NULL);
    pixDisplayWrite(pixt2, 1);
    pixd = pixClipMasked(pixt1, pixm, 100, 100, 0x03c08000);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);

        /* Clip 4 bpp colormapped */
    pixt1 = pixOctreeQuantNumColors(pixs, 16, 1);
    pixt2 = pixClipRectangle(pixt1, box, NULL);
    pixDisplayWrite(pixt2, 1);
    pixd = pixClipMasked(pixt1, pixm, 100, 100, 0x03c08000);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);

        /* Clip 2 bpp colormapped */
    pixt1 = pixMedianCutQuantGeneral(pixs, 0, 2, 4, 5, 1, 1);
    pixt2 = pixClipRectangle(pixt1, box, NULL);
    pixDisplayWrite(pixt2, 1);
    pixd = pixClipMasked(pixt1, pixm, 100, 100, 0x03608000);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);

        /* Clip 8 bpp gray */
    pixs8 = pixConvertRGBToLuminance(pixs);
    pixt2 = pixClipRectangle(pixs8, box, NULL);
    pixDisplayWrite(pixt2, 1);
    pixd = pixClipMasked(pixs8, pixm, 100, 100, 90);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);

        /* Clip 4 bpp gray */
    pixt1 = pixThresholdTo4bpp(pixs8, 16, 0);
    pixt2 = pixClipRectangle(pixt1, box, NULL);
    pixDisplayWrite(pixt2, 1);
    pixd = pixClipMasked(pixt1, pixm, 100, 100, 0);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);
    pixd = pixClipMasked(pixt1, pixm, 100, 100, 5);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);
    pixd = pixClipMasked(pixt1, pixm, 100, 100, 15);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);

        /* Clip 4 bpp gray, colormapped */
    pixt1 = pixThresholdTo4bpp(pixs8, 16, 1);
    pixt2 = pixClipRectangle(pixt1, box, NULL);
    pixDisplayWrite(pixt2, 1);
    pixd = pixClipMasked(pixt1, pixm, 100, 100, 0x55555500);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);

        /* Clip 2 bpp gray */
    pixt1 = pixThresholdTo2bpp(pixs8, 4, 0);
    pixt2 = pixClipRectangle(pixt1, box, NULL);
    pixDisplayWrite(pixt2, 1);
    pixd = pixClipMasked(pixt1, pixm, 100, 100, 1);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);

        /* Clip 2 bpp gray, colormapped */
    pixt1 = pixThresholdTo2bpp(pixs8, 4, 1);
    pixt2 = pixClipRectangle(pixt1, box, NULL);
    pixDisplayWrite(pixt2, 1);
    pixd = pixClipMasked(pixt1, pixm, 100, 100, 0x55555500);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);

    pixDestroy(&pixm);
    pixDestroy(&pixs);
    pixDestroy(&pixs8);
    boxDestroy(&box);

        /* Finally, do the 1 bpp painting through clipped region.
	 * We start with two 1 bpp text sources, use the inverse
	 * of the 2nd for the mask (so we take all of the 1st
	 * pixels under this mask), and for the remainder, which
	 * are the fg pixels in the 2nd, we paint them black (1).
	 * So this is a simple and fast blending of two 1 bpp pix. */
    pixs = pixRead("feyn.tif");
    box = boxCreate(670, 827, 800, 500);
    pixt2 = pixClipRectangle(pixs, box, NULL);
    boxDestroy(&box);
    pixDisplayWrite(pixt2, 1);
    pixt1 = pixRead("rabi.png");
    box = boxCreate(303, 1983, 800, 500);
    pixm = pixClipRectangle(pixt1, box, NULL);
    pixInvert(pixm, pixm);
    pixDisplayWrite(pixm, 1);
    pixd = pixClipMasked(pixs, pixm, 670, 827, 1);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixm);
    pixDestroy(&pixd);
    boxDestroy(&box);

    pixDisplayMultiple("/tmp/junk_write_display*");
    return 0;
}

