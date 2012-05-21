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
 * alphaclean_reg.c
 *
 *    Tests transparency and cleaning under alpha.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const l_int32   SHOW = 0;

main(int    argc,
     char **argv)
{
l_int32      w, h, n1, n2;
PIX         *pixs, *pixb, *pixg, *pixc, *pixd;
PIX         *pixg2, *pixcs1, *pixcs2, *pixd1, *pixd2, *pixt1, *pixt2;
PIXA        *pixa;
static char  mainName[] = "alphaclean_reg";

    if (argc != 1)
	exit(ERROR_INT(" Syntax:  alphaclean_reg", mainName, 1));

        /* Make the transparency (alpha) layer.
         * pixs is the mask.  We turn it into a transparency (alpha)
         * layer by converting to 8 bpp.  A small convolution fuzzes
         * the mask edges so that you don't see the pixels. */
    pixs = pixRead("feyn-fract.tif");
    pixGetDimensions(pixs, &w, &h, NULL);
    pixg = pixConvert1To8(NULL, pixs, 0, 255);
    pixg2 = pixBlockconvGray(pixg, NULL, 1, 1);
    pixDisplayWithTitle(pixg2, 300, 0, "alpha", SHOW);

        /* Make the viewable image.
         * pixc is the image that we see where the alpha layer is
         * opaque -- i.e., greater than 0.  Scale it to the same
         * size as the mask.  To visualize what this will look like
         * when displayed over a black background, create the black
         * background image, pixb, and do the blending with pixcs1
         * explicitly using the alpha layer pixg2. */
    pixc = pixRead("tetons.jpg");
    pixcs1 = pixScaleToSize(pixc, w, h);
    pixDisplayWithTitle(pixcs1, 100, 200, "viewable", SHOW);
    pixb = pixCreateTemplate(pixcs1);  /* black */
    pixd1 = pixBlendWithGrayMask(pixb, pixcs1, pixg2, 0, 0);
    pixDisplayWithTitle(pixd1, 100, 500, "alpha-blended 1", SHOW);

        /* Embed the alpha layer pixg2 into the color image pixc.
         * Write it out as is.  Then clean pixcs1 (to 0) under the fully
         * transparent part of the alpha layer, and write that result
         * out as well. */
    pixSetRGBComponent(pixcs1, pixg2, L_ALPHA_CHANNEL);
    pixWriteRGBAPng("/tmp/junkpixcs1.png", pixcs1);
    pixcs2 = pixSetUnderTransparency(pixcs1, 0, 0);
    pixWriteRGBAPng("/tmp/junkpixcs2.png", pixcs2);

        /* What will this look like over a black background?
         * Do the blending explicitly and display.  It should
         * look identical to the blended result pixd1 before cleaning. */
    pixd2 = pixBlendWithGrayMask(pixb, pixcs2, pixg2, 0, 0);
    pixDisplayWithTitle(pixd2, 600, 500, "alpha blended 2", SHOW);

        /* Read the two images back, ignoring the transparency layer.
         * The uncleaned image will come back identical to pixcs1.
         * However, the cleaned image will be black wherever
         * the alpha layer was fully transparent.  It will 
         * look the same when viewed through the alpha layer,
         * but have much better compression. */
    pixt1 = pixRead("/tmp/junkpixcs1.png");  /* just pixcs1 */
    pixt2 = pixRead("/tmp/junkpixcs2.png");  /* cleaned out under transparent */
    n1 = nbytesInFile("/tmp/junkpixcs1.png");
    n2 = nbytesInFile("/tmp/junkpixcs2.png");
    fprintf(stderr, " Original: %d bytes\n Cleaned: %d bytes\n", n1, n2);
    pixDisplayWithTitle(pixt1, 600, 200, "without alpha", SHOW);
    pixDisplayWithTitle(pixt2, 300, 800, "cleaned under transparent", SHOW);

    pixa = pixaCreate(0);
    pixSaveTiled(pixg2, pixa, 1, 1, 20, 32);
    pixSaveTiled(pixcs1, pixa, 1, 1, 20, 0);
    pixSaveTiled(pixt1, pixa, 1, 0, 20, 0);
    pixSaveTiled(pixd1, pixa, 1, 1, 20, 0);
    pixSaveTiled(pixd2, pixa, 1, 0, 20, 0);
    pixSaveTiled(pixt2, pixa, 1, 1, 20, 0);
    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/junkalpha.png", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixDestroy(&pixs);
    pixDestroy(&pixb);
    pixDestroy(&pixg);
    pixDestroy(&pixg2);
    pixDestroy(&pixc);
    pixDestroy(&pixcs1);
    pixDestroy(&pixcs2);
    pixDestroy(&pixd);
    pixDestroy(&pixd1);
    pixDestroy(&pixd2);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    return 0;
}


