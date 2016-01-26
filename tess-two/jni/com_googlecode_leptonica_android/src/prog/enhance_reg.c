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
 * enhance_reg.c
 *
 *   This tests the following global "enhancement" functions:
 *     * TRC transforms with variation of gamma and black point
 *     * HSV transforms with variation of hue, saturation and intensity
 *     * Contrast variation
 *     * Sharpening
 *     * Color mapping to lighten background with constant hue
 *     * Linear color transform without mixing (diagonal)
 */

#include "allheaders.h"

static const char *filein = "test24.jpg";
static const l_int32 WIDTH = 150;

int main(int    argc,
         char **argv)
{
char          textstr[256];
l_int32       w, h, d, i;
l_uint32      srcval, dstval;
l_float32     scalefact, sat, fract;
L_BMF        *bmf8;
L_KERNEL     *kel;
NUMA         *na;
PIX          *pix, *pixs, *pixs1, *pixs2, *pixd;
PIX          *pixt0, *pixt1, *pixt2, *pixt3, *pixt4;
PIXA         *pixa, *pixaf;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

    pix = pixRead(filein);
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 32)
        return ERROR_INT("file not 32 bpp", argv[0], 1);
    scalefact = (l_float32)WIDTH / (l_float32)w;
    pixs = pixScale(pix, scalefact, scalefact);
    w = pixGetWidth(pixs);
    pixaf = pixaCreate(5);

        /* TRC: vary gamma */
    pixa = pixaCreate(20);
    for (i = 0; i < 20; i++) {
        pixt0 = pixGammaTRC(NULL, pixs, 0.3 + 0.15 * i, 0, 255);
        pixaAddPix(pixa, pixt0, L_INSERT);
    }
    pixt1 = pixaDisplayTiledAndScaled(pixa, 32, w, 5, 0, 10, 2);
    pixSaveTiled(pixt1, pixaf, 1.0, 1, 20, 32);
    regTestWritePixAndCheck(rp, pixt1, IFF_PNG);  /* 0 */
    pixDisplayWithTitle(pixt1, 0, 100, "TRC Gamma", rp->display);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);

        /* TRC: vary black point */
    pixa = pixaCreate(20);
    for (i = 0; i < 20; i++) {
        pixt0 = pixGammaTRC(NULL, pixs, 1.0, 5 * i, 255);
        pixaAddPix(pixa, pixt0, L_INSERT);
    }
    pixt1 = pixaDisplayTiledAndScaled(pixa, 32, w, 5, 0, 10, 2);
    pixSaveTiled(pixt1, pixaf, 1.0, 1, 20, 0);
    regTestWritePixAndCheck(rp, pixt1, IFF_PNG);  /* 1 */
    pixDisplayWithTitle(pixt1, 300, 100, "TRC", rp->display);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);

        /* Vary hue */
    pixa = pixaCreate(20);
    for (i = 0; i < 20; i++) {
        pixt0 = pixModifyHue(NULL, pixs, 0.01 + 0.05 * i);
        pixaAddPix(pixa, pixt0, L_INSERT);
    }
    pixt1 = pixaDisplayTiledAndScaled(pixa, 32, w, 5, 0, 10, 2);
    pixSaveTiled(pixt1, pixaf, 1.0, 1, 20, 0);
    regTestWritePixAndCheck(rp, pixt1, IFF_PNG);  /* 2 */
    pixDisplayWithTitle(pixt1, 600, 100, "Hue", rp->display);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);

        /* Vary saturation */
    pixa = pixaCreate(20);
    na = numaCreate(20);
    for (i = 0; i < 20; i++) {
        pixt0 = pixModifySaturation(NULL, pixs, -0.9 + 0.1 * i);
        pixMeasureSaturation(pixt0, 1, &sat);
        pixaAddPix(pixa, pixt0, L_INSERT);
        numaAddNumber(na, sat);
    }
    pixt1 = pixaDisplayTiledAndScaled(pixa, 32, w, 5, 0, 10, 2);
    pixSaveTiled(pixt1, pixaf, 1.0, 1, 20, 0);
    gplotSimple1(na, GPLOT_PNG, "/tmp/lept/regout/enhance.7",
                 "Average Saturation");
    regTestWritePixAndCheck(rp, pixt1, IFF_PNG);  /* 3 */
    pixDisplayWithTitle(pixt1, 900, 100, "Saturation", rp->display);
    numaDestroy(&na);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);

        /* Vary contrast */
    pixa = pixaCreate(20);
    for (i = 0; i < 20; i++) {
        pixt0 = pixContrastTRC(NULL, pixs, 0.1 * i);
        pixaAddPix(pixa, pixt0, L_INSERT);
    }
    pixt1 = pixaDisplayTiledAndScaled(pixa, 32, w, 5, 0, 10, 2);
    pixSaveTiled(pixt1, pixaf, 1.0, 1, 20, 0);
    regTestWritePixAndCheck(rp, pixt1, IFF_PNG);  /* 4 */
    pixDisplayWithTitle(pixt1, 0, 400, "Contrast", rp->display);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);

        /* Vary sharpening */
    pixa = pixaCreate(20);
    for (i = 0; i < 20; i++) {
        pixt0 = pixUnsharpMasking(pixs, 3, 0.01 + 0.15 * i);
        pixaAddPix(pixa, pixt0, L_INSERT);
    }
    pixt1 = pixaDisplayTiledAndScaled(pixa, 32, w, 5, 0, 10, 2);
    pixSaveTiled(pixt1, pixaf, 1.0, 1, 20, 0);
    regTestWritePixAndCheck(rp, pixt1, IFF_PNG);  /* 5 */
    pixDisplayWithTitle(pixt1, 300, 400, "Sharp", rp->display);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);

        /* Hue constant mapping to lighter background */
    pixa = pixaCreate(11);
    bmf8 = bmfCreate("fonts", 8);
    pixt0 = pixRead("candelabrum.011.jpg");
    composeRGBPixel(230, 185, 144, &srcval);  /* select typical bg pixel */
    for (i = 0; i <= 10; i++) {
        fract = 0.10 * i;
        pixelFractionalShift(230, 185, 144, fract, &dstval);
        pixt1 = pixLinearMapToTargetColor(NULL, pixt0, srcval, dstval);
        snprintf(textstr, 50, "Fract = %5.1f", fract);
        pixt2 = pixAddSingleTextblock(pixt1, bmf8, textstr, 0xff000000,
                                      L_ADD_BELOW, NULL);
        pixSaveTiledOutline(pixt2, pixa, 1.0, (i % 4 == 0) ? 1 : 0, 30, 2, 32);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }
    pixDestroy(&pixt0);

    pixd = pixaDisplay(pixa, 0, 0);
    regTestWritePixAndCheck(rp, pixd, IFF_JFIF_JPEG);  /* 6 */
    pixDisplayWithTitle(pixd, 600, 400, "Constant hue", rp->display);
    bmfDestroy(&bmf8);
    pixaDestroy(&pixa);
    pixDestroy(&pixd);

        /* Delayed testing of saturation plot */
    regTestCheckFile(rp, "/tmp/lept/regout/enhance.7.png");  /* 7 */

        /* Display results */
    pixd = pixaDisplay(pixaf, 0, 0);
    regTestWritePixAndCheck(rp, pixd, IFF_JFIF_JPEG);  /* 8 */
    pixDisplayWithTitle(pixd, 100, 100, "All", rp->display);
    pixDestroy(&pixd);
    pixaDestroy(&pixaf);

    pixDestroy(&pix);
    pixDestroy(&pixs);

    /* -----------------------------------------------*
     *           Test global color transforms         *
     * -----------------------------------------------*/
        /* Make identical cmap and rgb images */
    pix = pixRead("wet-day.jpg");
    pixs1 = pixOctreeColorQuant(pix, 200, 0);
    pixs2 = pixRemoveColormap(pixs1, REMOVE_CMAP_TO_FULL_COLOR);
    regTestComparePix(rp, pixs1, pixs2);  /* 9 */

        /* Make a diagonal color transform matrix */
    kel = kernelCreate(3, 3);
    kernelSetElement(kel, 0, 0, 0.7);
    kernelSetElement(kel, 1, 1, 0.4);
    kernelSetElement(kel, 2, 2, 1.3);

        /* Apply to both cmap and rgb images. */
    pixt1 = pixMultMatrixColor(pixs1, kel);
    pixt2 = pixMultMatrixColor(pixs2, kel);
    regTestComparePix(rp, pixt1, pixt2);  /* 10 */
    kernelDestroy(&kel);

        /* Apply the same transform in the simpler interface */
    pixt3 = pixMultConstantColor(pixs1, 0.7, 0.4, 1.3);
    pixt4 = pixMultConstantColor(pixs2, 0.7, 0.4, 1.3);
    regTestComparePix(rp, pixt3, pixt4);  /* 11 */
    regTestComparePix(rp, pixt1, pixt3);  /* 12 */
    regTestWritePixAndCheck(rp, pixt1, IFF_JFIF_JPEG);  /* 13 */

    pixDestroy(&pix);
    pixDestroy(&pixs1);
    pixDestroy(&pixs2);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    return regTestCleanup(rp);
}
