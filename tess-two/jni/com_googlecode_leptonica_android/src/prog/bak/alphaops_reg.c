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
 * alphaops_reg.c
 *
 *   Tests various functions that use the alpha layer:
 *
 *     (1) Remove and add alpha layers.
 *         Removing is done by blending with a uniform image.
 *         Adding is done by setting all white pixels to transparent,
 *         and grading the alpha layer to opaque depending on
 *         the distance from the nearest transparent pixel.
 *
 *     (2) Tests transparency and cleaning under alpha.
 *
 *     (3) Blending with a uniform color.  Also tests an alternative
 *         way to "blend" to a color: component-wise multiplication by
 *         the color.
 */

#include "allheaders.h"

static PIX *DoBlendTest(PIX *pix, BOX *box, l_uint32 val, l_float32 gamma,
                        l_int32 minval, l_int32 maxval, l_int32 which);

main(int    argc,
     char **argv)
{
l_int32       w, h, n1, n2, n, i, minval, maxval;
l_uint32      color;
l_float32     gamma;
BOX          *box;
PIX          *pix1, *pix2, *pix3, *pix4;
PIX          *pixs, *pixb, *pixg, *pixc, *pixd;
PIX          *pixg2, *pixcs1, *pixcs2, *pixd1, *pixd2;
PIXA         *pixa, *pixa2, *pixa3;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

    /* ------------------------ (1) ----------------------------*/
        /* Blend with a white background */
    pix1 = pixReadRGBAPng("books_logo.png");
    pixDisplayWithTitle(pix1, 100, 0, NULL, rp->display);
    pix2 = pixAlphaBlendUniform(pix1, 0xffffff00);
    pixDisplayWithTitle(pix2, 100, 150, NULL, rp->display);
    regTestWritePixAndCheck(rp, pix1, IFF_PNG);  /* 0 */
    regTestWritePixAndCheck(rp, pix2, IFF_PNG);  /* 1 */

        /* Generate an alpha layer based on the white background */
    pix3 = pixSetAlphaOverWhite(pix2);
    regTestWritePixAndCheck(rp, pix3, IFF_PNG);  /* 2 */
    pixWriteRGBAPng("/tmp/alphaplay.3.png", pix3);
    regTestCheckFile(rp, "/tmp/alphaplay.3.png");   /* 3 */
    pixDisplayWithTitle(pix3, 100, 300, NULL, rp->display);

        /* Render on a light yellow background */
    pix4 = pixAlphaBlendUniform(pix3, 0xffffe000);
    regTestWritePixAndCheck(rp, pix4, IFF_PNG);  /* 4 */
    pixDisplayWithTitle(pix4, 100, 450, NULL, rp->display);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    pixDestroy(&pix4);

    /* ------------------------ (2) ----------------------------*/
    lept_rmdir("alpha");
    lept_mkdir("alpha");
        /* Make the transparency (alpha) layer.
         * pixs is the mask.  We turn it into a transparency (alpha)
         * layer by converting to 8 bpp.  A small convolution fuzzes
         * the mask edges so that you don't see the pixels. */
    pixs = pixRead("feyn-fract.tif");
    pixGetDimensions(pixs, &w, &h, NULL);
    pixg = pixConvert1To8(NULL, pixs, 0, 255);
    pixg2 = pixBlockconvGray(pixg, NULL, 1, 1);
    regTestWritePixAndCheck(rp, pixg2, IFF_JFIF_JPEG);  /* 5 */
    pixDisplayWithTitle(pixg2, 0, 0, "alpha", rp->display);

        /* Make the viewable image.
         * pixc is the image that we see where the alpha layer is
         * opaque -- i.e., greater than 0.  Scale it to the same
         * size as the mask.  To visualize what this will look like
         * when displayed over a black background, create the black
         * background image, pixb, and do the blending with pixcs1
         * explicitly using the alpha layer pixg2. */
    pixc = pixRead("tetons.jpg");
    pixcs1 = pixScaleToSize(pixc, w, h);
    regTestWritePixAndCheck(rp, pixcs1, IFF_JFIF_JPEG);  /* 6 */
    pixDisplayWithTitle(pixcs1, 300, 0, "viewable", rp->display);
    pixb = pixCreateTemplate(pixcs1);  /* black */
    pixd1 = pixBlendWithGrayMask(pixb, pixcs1, pixg2, 0, 0);
    regTestWritePixAndCheck(rp, pixd1, IFF_JFIF_JPEG);  /* 7 */
    pixDisplayWithTitle(pixd1, 600, 0, "alpha-blended 1", rp->display);

        /* Embed the alpha layer pixg2 into the color image pixc.
         * Write it out as is.  Then clean pixcs1 (to 0) under the fully
         * transparent part of the alpha layer, and write that result
         * out as well. */
    pixSetRGBComponent(pixcs1, pixg2, L_ALPHA_CHANNEL);
    pixWriteRGBAPng("/tmp/alpha/pixcs1.png", pixcs1);
    pixcs2 = pixSetUnderTransparency(pixcs1, 0, 0);
    pixWriteRGBAPng("/tmp/alpha/pixcs2.png", pixcs2);

        /* What will this look like over a black background?
         * Do the blending explicitly and display.  It should
         * look identical to the blended result pixd1 before cleaning. */
    pixd2 = pixBlendWithGrayMask(pixb, pixcs2, pixg2, 0, 0);
    regTestWritePixAndCheck(rp, pixd2, IFF_JFIF_JPEG);  /* 8 */
    pixDisplayWithTitle(pixd2, 0, 400, "alpha blended 2", rp->display);

        /* Read the two images back, ignoring the transparency layer.
         * The uncleaned image will come back identical to pixcs1.
         * However, the cleaned image will be black wherever
         * the alpha layer was fully transparent.  It will 
         * look the same when viewed through the alpha layer,
         * but have much better compression. */
    pix1 = pixRead("/tmp/alpha/pixcs1.png");  /* just pixcs1 */
    pix2 = pixRead("/tmp/alpha/pixcs2.png");  /* cleaned out under transparent */
    n1 = nbytesInFile("/tmp/alpha/pixcs1.png");
    n2 = nbytesInFile("/tmp/alpha/pixcs2.png");
    fprintf(stderr, " Original: %d bytes\n Cleaned: %d bytes\n", n1, n2);
    regTestWritePixAndCheck(rp, pix1, IFF_JFIF_JPEG);  /* 9 */
    regTestWritePixAndCheck(rp, pix2, IFF_JFIF_JPEG);  /* 10 */
    pixDisplayWithTitle(pix1, 300, 400, "without alpha", rp->display);
    pixDisplayWithTitle(pix2, 600, 400, "cleaned under transparent",
                        rp->display);

    pixa = pixaCreate(0);
    pixSaveTiled(pixg2, pixa, 1, 1, 20, 32);
    pixSaveTiled(pixcs1, pixa, 1, 1, 20, 0);
    pixSaveTiled(pix1, pixa, 1, 0, 20, 0);
    pixSaveTiled(pixd1, pixa, 1, 1, 20, 0);
    pixSaveTiled(pixd2, pixa, 1, 0, 20, 0);
    pixSaveTiled(pix2, pixa, 1, 1, 20, 0);
    pixd = pixaDisplay(pixa, 0, 0);
    regTestWritePixAndCheck(rp, pixd, IFF_JFIF_JPEG);  /* 11 */
    pixDisplayWithTitle(pixd, 200, 200, "composite", rp->display);
    pixWrite("/tmp/alpha/alpha.png", pixd, IFF_JFIF_JPEG);
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
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    /* ------------------------ (3) ----------------------------*/
    color = 0xffffa000;
    gamma = 1.0;
    minval = 0;
    maxval = 200;
    box = boxCreate(0, 85, 600, 100);
    pixa = pixaCreate(6);
    pix1 = pixRead("blend-green1.jpg");
    pixaAddPix(pixa, pix1, L_INSERT);
    pix1 = pixRead("blend-green2.png");
    pixaAddPix(pixa, pix1, L_INSERT);
    pix1 = pixRead("blend-green3.png");
    pixaAddPix(pixa, pix1, L_INSERT);
    pix1 = pixRead("blend-orange.jpg");
    pixaAddPix(pixa, pix1, L_INSERT);
    pix1 = pixRead("blend-yellow.jpg");
    pixaAddPix(pixa, pix1, L_INSERT);
    pix1 = pixRead("blend-red.png");
    pixaAddPix(pixa, pix1, L_INSERT);
    n = pixaGetCount(pixa);
    pixa2 = pixaCreate(n);
    pixa3 = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pix1 = pixaGetPix(pixa, i, L_CLONE);
        pix2 = DoBlendTest(pix1, box, color, gamma, minval, maxval, 1);
        regTestWritePixAndCheck(rp, pix2, IFF_JFIF_JPEG);  /* 12, 14, ... 22 */
        pixDisplayWithTitle(pix2, 150 * i, 0, NULL, rp->display);
        pixaAddPix(pixa2, pix2, L_INSERT);
        pix2 = DoBlendTest(pix1, box, color, gamma, minval, maxval, 2);
        regTestWritePixAndCheck(rp, pix2, IFF_JFIF_JPEG);  /* 13, 15, ... 23 */
        pixDisplayWithTitle(pix2, 150 * i, 200, NULL, rp->display);
        pixaAddPix(pixa3, pix2, L_INSERT);
        pixDestroy(&pix1);
    }
    if (rp->display) {
        pixaConvertToPdf(pixa2, 0, 0.75, L_FLATE_ENCODE, 0, "blend 1 test",
                         "/tmp/alpha/blending1.pdf");
        pixaConvertToPdf(pixa3, 0, 0.75, L_FLATE_ENCODE, 0, "blend 2 test",
                         "/tmp/alpha/blending2.pdf");
    }
    pixaDestroy(&pixa);
    pixaDestroy(&pixa2);
    pixaDestroy(&pixa3);
    boxDestroy(&box);

    return regTestCleanup(rp); 
}


    /* Use which == 1 to test alpha blending; which == 2 to test "blending"
     * by pixel multiplication.   This generates a composite of 5 images:
     * the original, blending over a box at the bottom (2 ways), and
     * multiplying over a box at the bottom (2 ways). */
static PIX *
DoBlendTest(PIX *pix, BOX *box, l_uint32 color, l_float32 gamma,
            l_int32 minval, l_int32 maxval, l_int32 which)
{
PIX   *pix1, *pix2, *pix3, *pixd;
PIXA  *pixa;
  pixa = pixaCreate(5);
  pix1 = pixRemoveColormap(pix, REMOVE_CMAP_TO_FULL_COLOR);
  pix2 = pixCopy(NULL, pix1);
  pixaAddPix(pixa, pix2, L_COPY);
  if (which == 1)
      pix3 =  pixBlendBackgroundToColor(NULL, pix2, box, color,
                                        gamma, minval, maxval);
  else
      pix3 =  pixMultiplyByColor(NULL, pix2, box, color);
  pixaAddPix(pixa, pix3, L_INSERT);
  if (which == 1)
      pixBlendBackgroundToColor(pix2, pix2, box, color,
                                gamma, minval, maxval);
  else
      pixMultiplyByColor(pix2, pix2, box, color);
  pixaAddPix(pixa, pix2, L_INSERT);
  pix2 = pixCopy(NULL, pix1);
  if (which == 1)
      pix3 =  pixBlendBackgroundToColor(NULL, pix2, NULL, color,
                                        gamma, minval, maxval);
  else
      pix3 =  pixMultiplyByColor(NULL, pix2, NULL, color);
  pixaAddPix(pixa, pix3, L_INSERT);
  if (which == 1)
      pixBlendBackgroundToColor(pix2, pix2, NULL, color,
                                gamma, minval, maxval);
  else
      pixMultiplyByColor(pix2, pix2, NULL, color);
  pixaAddPix(pixa, pix2, L_INSERT);

  pixd = pixaDisplayTiledInRows(pixa, 32, 800, 1.0, 0, 30, 2);
  pixDestroy(&pix1);
  pixaDestroy(&pixa);
  return pixd;
}
