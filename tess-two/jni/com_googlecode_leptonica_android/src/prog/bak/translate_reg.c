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
 * translate_reg.c
 *
 *    Regression test for in-place translation
 */

#include "allheaders.h"

#define   BINARY_IMAGE        "test1.png"
#define   GRAYSCALE_IMAGE     "test8.jpg"
#define   FOUR_BPP_IMAGE      "weasel4.8g.png"
#define   COLORMAP_IMAGE      "dreyfus8.png"
#define   RGB_IMAGE           "marge.jpg"

void TranslateAndSave1(PIXA *pixa, l_int32 depth, PIX *pix,
                       l_int32 xshift, l_int32 yshift);

void TranslateAndSave2(PIXA *pixa, PIX *pix, l_int32 xshift, l_int32 yshift);


main(int    argc,
     char **argv)
{
BOX          *box;
PIX          *pix, *pixs, *pixd;
PIX          *pix1, *pix2, *pix3, *pix4, *pix5, *pix6, *pix7;
PIXA         *pixa;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

        /* Set up images */
    pix1 = pixRead("weasel2.4c.png");
    pix2 = pixScaleBySampling(pix1, 3.0, 3.0);
    box = boxCreate(0, 0, 209, 214);
    pixs = pixClipRectangle(pix2, box, NULL);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    boxDestroy(&box);
    pix1 = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
    pix2 = pixRemoveColormap(pixs, REMOVE_CMAP_TO_FULL_COLOR);
    pix3 = pixConvertTo1(pixs, 128);
    pix4 = pixRotateAM(pix1, 0.25, L_BRING_IN_BLACK);
    pix5 = pixRotateAM(pix1, -0.25, L_BRING_IN_WHITE);
    pix6 = pixRotateAM(pix2, -0.15, L_BRING_IN_BLACK);
    pix7 = pixRotateAM(pix2, +0.15, L_BRING_IN_WHITE);

    pixa = pixaCreate(0);
    TranslateAndSave1(pixa, 32, pixs, 30, 30);
    TranslateAndSave1(pixa, 32, pix1, 35, 20);
    TranslateAndSave1(pixa, 32, pix2, 20, 35);
    TranslateAndSave1(pixa, 32, pix3, 20, 35);
    pixd = pixaDisplayOnColor(pixa, 0, 0, 0x44aaaa00);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);  /* 0 */
    pixDisplayWithTitle(pixd, 0, 0, "trans0", rp->display);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixa = pixaCreate(0);
    TranslateAndSave1(pixa, 8, pix1, 35, 20);
    TranslateAndSave1(pixa, 8, pix4, 35, 20);
    pixd = pixaDisplayOnColor(pixa, 0, 0, 0x44);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);  /* 1 */
    pixDisplayWithTitle(pixd, 250, 0, "trans1", rp->display);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixa = pixaCreate(0);
    TranslateAndSave2(pixa, pixs, 30, 30);
    TranslateAndSave2(pixa, pix1, 30, 30);
    TranslateAndSave2(pixa, pix2, 35, 20);
    TranslateAndSave2(pixa, pix3, 20, 35);
    TranslateAndSave2(pixa, pix4, 25, 25);
    TranslateAndSave2(pixa, pix5, 25, 25);
    TranslateAndSave2(pixa, pix6, 25, 25);
    TranslateAndSave2(pixa, pix7, 25, 25);
    pixd = pixaDisplayTiledInRows(pixa, 32, 1200, 1.0, 0, 30, 3);
    regTestWritePixAndCheck(rp, pixd, IFF_PNG);  /* 2 */
    pixDisplayWithTitle(pixd, 500, 0, "trans2", rp->display);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
  
    pixDestroy(&pixs);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    pixDestroy(&pix4);
    pixDestroy(&pix5);
    pixDestroy(&pix6);
    pixDestroy(&pix7);
    return regTestCleanup(rp);
}


void
TranslateAndSave1(PIXA    *pixa,
                  l_int32  depth,
                  PIX     *pix,
                  l_int32  xshift,
                  l_int32  yshift)
{
PIX  *pix1, *pix2, *pix3, *pix4;

    pix1 = pixTranslate(NULL, pix, xshift, yshift, L_BRING_IN_WHITE);
    pix2 = pixTranslate(NULL, pix, xshift, yshift, L_BRING_IN_BLACK);
    pix3 = pixTranslate(NULL, pix, -xshift, -yshift, L_BRING_IN_WHITE);
    pix4 = pixTranslate(NULL, pix, -xshift, -yshift, L_BRING_IN_BLACK);
    pixSaveTiled(pix1, pixa, 1, 1, 25, depth);
    pixSaveTiled(pix2, pixa, 1, 0, 25, depth);
    pixSaveTiled(pix3, pixa, 1, 0, 25, depth);
    pixSaveTiled(pix4, pixa, 1, 0, 25, depth);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    pixDestroy(&pix4);
    return;
}

void
TranslateAndSave2(PIXA    *pixa,
                  PIX     *pix,
                  l_int32  xshift,
                  l_int32  yshift)
{
PIX  *pix1, *pix2, *pix3, *pix4;

    pix1 = pixTranslate(NULL, pix, xshift, yshift, L_BRING_IN_WHITE);
    pix2 = pixTranslate(NULL, pix, xshift, yshift, L_BRING_IN_BLACK);
    pix3 = pixTranslate(NULL, pix, -xshift, -yshift, L_BRING_IN_WHITE);
    pix4 = pixTranslate(NULL, pix, -xshift, -yshift, L_BRING_IN_BLACK);
    pixaAddPix(pixa, pix1, L_INSERT);
    pixaAddPix(pixa, pix2, L_INSERT);
    pixaAddPix(pixa, pix3, L_INSERT);
    pixaAddPix(pixa, pix4, L_INSERT);
    return;
}
