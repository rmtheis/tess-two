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
 * scale_reg.c
 *
 *      This tests a number of scaling operations, through the pixScale()
 *      interface.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const char *image[10] = {"feyn.tif",         /* 1 bpp */
                                "weasel2.png",      /* 2 bpp; no cmap */
                                "weasel2.4c.png",   /* 2 bpp; cmap */
                                "weasel4.png",      /* 4 bpp; no cmap */
                                "weasel4.16c.png",  /* 4 bpp; cmap */
                                "weasel8.png",      /* 8 bpp; no cmap */
                                "weasel8.240c.png", /* 8 bpp; cmap */
                                "test16.png",       /* 16 bpp rgb */
                                "marge.jpg",        /* 32 bpp rgb */
                                "test24.jpg"};      /* 32 bpp rgb */


static const l_int32    SPACE = 30;
static const l_int32    WIDTH = 300;
static const l_float32  FACTOR[5] = {2.3, 1.5, 1.1, 0.6, 0.3};

static void AddScaledImages(PIXA *pixa, const char *fname, l_int32 width);
static void PixSave32(PIXA *pixa, PIX *pixc);
static void PixaSaveDisplay(PIXA *pixa, l_int32 *pcount, L_REGPARAMS *rp);


main(int    argc,
     char **argv)
{
l_int32       i, display, success, count;
FILE         *fp;
PIX          *pixs, *pixc, *pixd;
PIXA         *pixa;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &fp, &display, &success, &rp))
        return 1;
    count = 0;

        /* Test 1 bpp */
    fprintf(stderr, "\n-------------- Testing 1 bpp ----------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[0]);
    pixc = pixScale(pixs, 0.32, 0.32);
    regTestWritePixAndCheck(pixc, IFF_PNG, &count, rp);
    pixSaveTiled(pixc, pixa, 1, 1, SPACE, 32);
    pixDestroy(&pixc);

    pixc = pixScaleToGray3(pixs);
    regTestWritePixAndCheck(pixc, IFF_PNG, &count, rp);
    PixSave32(pixa, pixc);

    pixc = pixScaleToGray4(pixs);
    regTestWritePixAndCheck(pixc, IFF_PNG, &count, rp);
    pixSaveTiled(pixc, pixa, 1, 1, SPACE, 32);
    pixDestroy(&pixc);

    pixc = pixScaleToGray6(pixs);
    regTestWritePixAndCheck(pixc, IFF_PNG, &count, rp);
    PixSave32(pixa, pixc);

    pixc = pixScaleToGray8(pixs);
    regTestWritePixAndCheck(pixc, IFF_PNG, &count, rp);
    PixSave32(pixa, pixc);

    pixc = pixScaleToGray16(pixs);
    regTestWritePixAndCheck(pixc, IFF_PNG, &count, rp);
    PixSave32(pixa, pixc);
    pixDestroy(&pixs);
    PixaSaveDisplay(pixa, &count, rp);

    for (i = 1; i < 10; i++) {
        pixa = pixaCreate(0);
        AddScaledImages(pixa, image[i], WIDTH);
        PixaSaveDisplay(pixa, &count, rp);
    }

        /* Test 2 bpp without colormap */
    fprintf(stderr, "\n-------------- Testing 2 bpp without cmap ----------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[1]);
    pixSaveTiled(pixs, pixa, 1, 1, SPACE, 32);
    pixc = pixScale(pixs, 2.25, 2.25);
    regTestWritePixAndCheck(pixc, IFF_JFIF_JPEG, &count, rp);
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.85, 0.85);
    regTestWritePixAndCheck(pixc, IFF_JFIF_JPEG, &count, rp);
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.65, 0.65);
    regTestWritePixAndCheck(pixc, IFF_JFIF_JPEG, &count, rp);
    PixSave32(pixa, pixc);
    PixaSaveDisplay(pixa, &count, rp);
    pixDestroy(&pixs);

        /* Test 2 bpp with colormap */
    fprintf(stderr, "\n-------------- Testing 2 bpp with cmap ----------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[2]);
    pixSaveTiled(pixs, pixa, 1, 1, SPACE, 32);
    pixc = pixScale(pixs, 2.25, 2.25);
    regTestWritePixAndCheck(pixc, IFF_PNG, &count, rp);
    pixc = pixScale(pixs, 0.85, 0.85);
    regTestWritePixAndCheck(pixc, IFF_PNG, &count, rp);
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.65, 0.65);
    regTestWritePixAndCheck(pixc, IFF_PNG, &count, rp);
    PixSave32(pixa, pixc);
    PixaSaveDisplay(pixa, &count, rp);
    pixDestroy(&pixs);

        /* Test 4 bpp without colormap */
    fprintf(stderr, "\n-------------- Testing 4 bpp without cmap ----------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[3]);
    pixSaveTiled(pixs, pixa, 1, 1, SPACE, 32);
    pixc = pixScale(pixs, 1.72, 1.72);
    regTestWritePixAndCheck(pixc, IFF_PNG, &count, rp);
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.85, 0.85);
    regTestWritePixAndCheck(pixc, IFF_PNG, &count, rp);
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.65, 0.65);
    regTestWritePixAndCheck(pixc, IFF_PNG, &count, rp);
    PixSave32(pixa, pixc);
    PixaSaveDisplay(pixa, &count, rp);
    pixDestroy(&pixs);

        /* Test 4 bpp with colormap */
    fprintf(stderr, "\n-------------- Testing 4 bpp with cmap ----------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[4]);
    pixSaveTiled(pixs, pixa, 1, 1, SPACE, 32);
    pixc = pixScale(pixs, 1.72, 1.72);
    regTestWritePixAndCheck(pixc, IFF_PNG, &count, rp);
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.85, 0.85);
    regTestWritePixAndCheck(pixc, IFF_PNG, &count, rp);
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.65, 0.65);
    regTestWritePixAndCheck(pixc, IFF_PNG, &count, rp);
    PixSave32(pixa, pixc);
    PixaSaveDisplay(pixa, &count, rp);
    pixDestroy(&pixs);

        /* Test 8 bpp without colormap */
    fprintf(stderr, "\n-------------- Testing 8 bpp without cmap ----------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[5]);
    pixSaveTiled(pixs, pixa, 1, 1, SPACE, 32);
    pixc = pixScale(pixs, 1.92, 1.92);
    regTestWritePixAndCheck(pixc, IFF_JFIF_JPEG, &count, rp);
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.85, 0.85);
    regTestWritePixAndCheck(pixc, IFF_JFIF_JPEG, &count, rp);
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.65, 0.65);
    regTestWritePixAndCheck(pixc, IFF_JFIF_JPEG, &count, rp);
    PixSave32(pixa, pixc);
    PixaSaveDisplay(pixa, &count, rp);
    pixDestroy(&pixs);

        /* Test 8 bpp with colormap */
    fprintf(stderr, "\n-------------- Testing 8 bpp with cmap ----------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[6]);
    pixSaveTiled(pixs, pixa, 1, 1, SPACE, 32);
    pixc = pixScale(pixs, 1.92, 1.92);
    regTestWritePixAndCheck(pixc, IFF_JFIF_JPEG, &count, rp);
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.85, 0.85);
    regTestWritePixAndCheck(pixc, IFF_JFIF_JPEG, &count, rp);
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.65, 0.65);
    regTestWritePixAndCheck(pixc, IFF_JFIF_JPEG, &count, rp);
    PixSave32(pixa, pixc);
    PixaSaveDisplay(pixa, &count, rp);
    pixDestroy(&pixs);

        /* Test 16 bpp */
    fprintf(stderr, "\n-------------- Testing 16 bpp ------------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[7]);
    pixSaveTiled(pixs, pixa, 1, 1, SPACE, 32);
    pixc = pixScale(pixs, 1.92, 1.92);
    regTestWritePixAndCheck(pixc, IFF_JFIF_JPEG, &count, rp);
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.85, 0.85);
    regTestWritePixAndCheck(pixc, IFF_JFIF_JPEG, &count, rp);
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.65, 0.65);
    regTestWritePixAndCheck(pixc, IFF_JFIF_JPEG, &count, rp);
    PixSave32(pixa, pixc);
    PixaSaveDisplay(pixa, &count, rp);
    pixDestroy(&pixs);

        /* Test 32 bpp */
    fprintf(stderr, "\n-------------- Testing 32 bpp ------------\n");
    pixa = pixaCreate(0);
    pixs = pixRead(image[8]);
    pixSaveTiled(pixs, pixa, 1, 1, SPACE, 32);
    pixc = pixScale(pixs, 1.42, 1.42);
    regTestWritePixAndCheck(pixc, IFF_JFIF_JPEG, &count, rp);
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.85, 0.85);
    regTestWritePixAndCheck(pixc, IFF_JFIF_JPEG, &count, rp);
    PixSave32(pixa, pixc);
    pixc = pixScale(pixs, 0.65, 0.65);
    regTestWritePixAndCheck(pixc, IFF_JFIF_JPEG, &count, rp);
    PixSave32(pixa, pixc);
    PixaSaveDisplay(pixa, &count, rp);
    pixDestroy(&pixs);

    regTestCleanup(argc, argv, fp, success, rp);
    return 0;
}

static void
AddScaledImages(PIXA         *pixa,
                const char   *fname,
                l_int32       width)
{
l_int32    i, w;
l_float32  scalefactor;
PIX       *pixs, *pixt1, *pixt2, *pix32;

    pixs = pixRead(fname);
    w = pixGetWidth(pixs);
    for (i = 0; i < 5; i++) {
        scalefactor = (l_float32)width / (FACTOR[i] * (l_float32)w);
        pixt1 = pixScale(pixs, FACTOR[i], FACTOR[i]);
        pixt2 = pixScale(pixt1, scalefactor, scalefactor);
        pix32 = pixConvertTo32(pixt2);
        if (i == 0)
            pixSaveTiled(pix32, pixa, 1, 1, SPACE, 32);
        else
            pixSaveTiled(pix32, pixa, 1, 0, SPACE, 32);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pix32);
    }
    pixDestroy(&pixs);
    return;
}

static void
PixSave32(PIXA *pixa, PIX *pixc)
{
PIX  *pix32;
    pix32 = pixConvertTo32(pixc);
    pixSaveTiled(pix32, pixa, 1, 0, SPACE, 32);
    pixDestroy(&pixc);
    pixDestroy(&pix32);
    return;
}

static void
PixaSaveDisplay(PIXA *pixa, l_int32 *pcount, L_REGPARAMS *rp)
{
PIX  *pixd;

    pixd = pixaDisplay(pixa, 0, 0);
    regTestWritePixAndCheck(pixd, IFF_JFIF_JPEG, pcount, rp);
    pixDisplayWithTitle(pixd, 100, 100, NULL, rp->display);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    return;
}

