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

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const char *filein = "test24.jpg";
static const l_int32 WIDTH = 150;

main(int    argc,
     char **argv)
{
char         textstr[256];
l_int32      w, h, d, i, same, success, display;
l_uint32     srcval, dstval;
l_float32    scalefact, sat, fract;
L_BMF       *bmf8;
L_KERNEL    *kel;
FILE        *fp;
NUMA        *na;
PIX         *pix, *pixs, *pixs1, *pixs2, *pixd;
PIX         *pixt0, *pixt1, *pixt2, *pixt3, *pixt4;
PIXA        *pixa, *pixaf;
static char  mainName[] = "enhance_reg";

    if (regTestSetup(argc, argv, &fp, &display, &success, NULL))
              return 1;

    pix = pixRead(filein);
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 32)
        return ERROR_INT("file not 32 bpp", mainName, 1);
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
    pixSaveTiled(pixt1, pixaf, 1, 1, 20, 32);
    pixWrite("/tmp/enhance.0.png", pixt1, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/enhance.0.png", 0, &success);
    pixDisplayWithTitle(pixt1, 0, 100, "TRC Gamma", display);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);

        /* TRC: vary black point */
    pixa = pixaCreate(20);
    for (i = 0; i < 20; i++) {
        pixt0 = pixGammaTRC(NULL, pixs, 1.0, 5 * i, 255);
        pixaAddPix(pixa, pixt0, L_INSERT);
    }
    pixt1 = pixaDisplayTiledAndScaled(pixa, 32, w, 5, 0, 10, 2);
    pixSaveTiled(pixt1, pixaf, 1, 1, 20, 0);
    pixWrite("/tmp/enhance.1.png", pixt1, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/enhance.1.png", 1, &success);
    pixDisplayWithTitle(pixt1, 300, 100, "TRC", display);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);

        /* Vary hue */
    pixa = pixaCreate(20);
    for (i = 0; i < 20; i++) {
        pixt0 = pixModifyHue(NULL, pixs, 0.01 + 0.05 * i);
        pixaAddPix(pixa, pixt0, L_INSERT);
    }
    pixt1 = pixaDisplayTiledAndScaled(pixa, 32, w, 5, 0, 10, 2);
    pixSaveTiled(pixt1, pixaf, 1, 1, 20, 0);
    pixWrite("/tmp/enhance.2.png", pixt1, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/enhance.2.png", 2, &success);
    pixDisplayWithTitle(pixt1, 600, 100, "Hue", display);
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
    pixSaveTiled(pixt1, pixaf, 1, 1, 20, 0);
    gplotSimple1(na, GPLOT_PNG, "/tmp/enhance.7", "Average Saturation");
    pixWrite("/tmp/enhance.3.png", pixt1, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/enhance.3.png", 3, &success);
    pixDisplayWithTitle(pixt1, 900, 100, "Saturation", display);
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
    pixSaveTiled(pixt1, pixaf, 1, 1, 20, 0);
    pixWrite("/tmp/enhance.4.png", pixt1, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/enhance.4.png", 4, &success);
    pixDisplayWithTitle(pixt1, 0, 400, "Contrast", display);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);

        /* Vary sharpening */
    pixa = pixaCreate(20);
    for (i = 0; i < 20; i++) {
        pixt0 = pixUnsharpMasking(pixs, 3, 0.01 + 0.15 * i);
        pixaAddPix(pixa, pixt0, L_INSERT);
    }
    pixt1 = pixaDisplayTiledAndScaled(pixa, 32, w, 5, 0, 10, 2);
    pixSaveTiled(pixt1, pixaf, 1, 1, 20, 0);
    pixWrite("/tmp/enhance.5.png", pixt1, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/enhance.5.png", 5, &success);
    pixDisplayWithTitle(pixt1, 300, 400, "Sharp", display);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);

        /* Hue constant mapping to lighter background */
    pixa = pixaCreate(11);
    bmf8 = bmfCreate("fonts", 8);
    pixt0 = pixRead("candelabrum-11.jpg");
    composeRGBPixel(230, 185, 144, &srcval);
    for (i = 0; i <= 10; i++) {
        fract = 0.10 * i;
        pixelFractionalShift(230, 185, 144, fract, &dstval);
        pixt1 = pixLinearMapToTargetColor(NULL, pixt0, srcval, dstval);
        snprintf(textstr, 50, "Fract = %5.1f", fract);
        pixt2 = pixAddSingleTextblock(pixt1, bmf8, textstr, 0xff000000,
                                      L_ADD_BELOW, NULL);
        pixSaveTiledOutline(pixt2, pixa, 1, (i % 4 == 0) ? 1 : 0, 30, 2, 32);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }
    pixDestroy(&pixt0);

    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplayWithTitle(pixd, 600, 400, "Constant hue", display);
    pixWrite("/tmp/enhance.6.jpg", pixd, IFF_JFIF_JPEG);
    regTestCheckFile(fp, argv, "/tmp/enhance.6.jpg", 6, &success);
    bmfDestroy(&bmf8);
    pixaDestroy(&pixa);
    pixDestroy(&pixd);

        /* Delayed testing of saturation plot */
    regTestCheckFile(fp, argv, "/tmp/enhance.7.png", 7, &success);

        /* Display results */
    pixd = pixaDisplay(pixaf, 0, 0);
    pixDisplayWithTitle(pixd, 100, 100, "All", display);
    pixWrite("/tmp/enhance.8.jpg", pixd, IFF_JFIF_JPEG);
    regTestCheckFile(fp, argv, "/tmp/enhance.8.jpg", 8, &success);
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
    regTestComparePix(fp, argv, pixs1, pixs2, 0, &success);

        /* Make a diagonal color transform matrix */
    kel = kernelCreate(3, 3);
    kernelSetElement(kel, 0, 0, 0.7);
    kernelSetElement(kel, 1, 1, 0.4);
    kernelSetElement(kel, 2, 2, 1.3);

        /* Apply to both cmap and rgb images. */
    pixt1 = pixMultMatrixColor(pixs1, kel);
    pixt2 = pixMultMatrixColor(pixs2, kel);
    regTestComparePix(fp, argv, pixt1, pixt2, 1, &success);

        /* Apply the same transform in the simpler interface */
    pixt3 = pixMultConstantColor(pixs1, 0.7, 0.4, 1.3);
    pixt4 = pixMultConstantColor(pixs2, 0.7, 0.4, 1.3);
    regTestComparePix(fp, argv, pixt3, pixt4, 2, &success);
    regTestComparePix(fp, argv, pixt1, pixt3, 3, &success);
    pixWrite("/tmp/enhance.9.jpg", pixt1, IFF_JFIF_JPEG);
    regTestCheckFile(fp, argv, "/tmp/enhance.9.jpg", 9, &success);

    regTestCleanup(argc, argv, fp, success, NULL);
    pixDestroy(&pix);
    pixDestroy(&pixs1);
    pixDestroy(&pixs2);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    return 0;
}


