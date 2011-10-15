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
 *   shear2_reg.c
 *
 *    Regression test for quadratic shear, both sampled and interpolated.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

void PixSave(PIX **ppixs, PIXA *pixa, l_int32 newrow,
             L_BMF *bmf, const char *textstr);


l_int32 main(int    argc,
             char **argv)
{
l_int32       count, display, success;
L_BMF        *bmf;
FILE         *fp;
PIX          *pixs1, *pixs2, *pixg, *pixt, *pixd;
PIXA         *pixa;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &fp, &display, &success, &rp))
              return 1;

    count = 0;
    bmf = bmfCreate("./fonts", 8);
    pixs1 = pixCreate(301, 301, 32);
    pixs2 = pixCreate(601, 601, 32);
    pixSetAll(pixs1);
    pixSetAll(pixs2);
    pixRenderLineArb(pixs1, 0, 20, 300, 20, 5, 0, 0, 255);
    pixRenderLineArb(pixs1, 0, 70, 300, 70, 5, 0, 255, 0);
    pixRenderLineArb(pixs1, 0, 120, 300, 120, 5, 0, 255, 255);
    pixRenderLineArb(pixs1, 0, 170, 300, 170, 5, 255, 0, 0);
    pixRenderLineArb(pixs1, 0, 220, 300, 220, 5, 255, 0, 255);
    pixRenderLineArb(pixs1, 0, 270, 300, 270, 5, 255, 255, 0);
    pixRenderLineArb(pixs2, 0, 20, 300, 20, 5, 0, 0, 255);
    pixRenderLineArb(pixs2, 0, 70, 300, 70, 5, 0, 255, 0);
    pixRenderLineArb(pixs2, 0, 120, 300, 120, 5, 0, 255, 255);
    pixRenderLineArb(pixs2, 0, 170, 300, 170, 5, 255, 0, 0);
    pixRenderLineArb(pixs2, 0, 220, 300, 220, 5, 255, 0, 255);
    pixRenderLineArb(pixs2, 0, 270, 300, 270, 5, 255, 255, 0);

        /* Color, small pix */
    pixa = pixaCreate(0);
    pixt = pixQuadraticVShear(pixs1, L_WARP_TO_LEFT,
                                   60, -20, L_SAMPLED, L_BRING_IN_WHITE);
    PixSave(&pixt, pixa, 1, bmf, "sampled-left");
    pixt = pixQuadraticVShear(pixs1, L_WARP_TO_RIGHT,
                                   60, -20, L_SAMPLED, L_BRING_IN_WHITE);
    PixSave(&pixt, pixa, 0, bmf, "sampled-right");
    pixt = pixQuadraticVShear(pixs1, L_WARP_TO_LEFT,
                                   60, -20, L_INTERPOLATED, L_BRING_IN_WHITE);
    PixSave(&pixt, pixa, 1, bmf, "interpolated-left");
    pixt = pixQuadraticVShear(pixs1, L_WARP_TO_RIGHT,
                                   60, -20, L_INTERPOLATED, L_BRING_IN_WHITE);
    PixSave(&pixt, pixa, 0, bmf, "interpolated-right");
    pixd = pixaDisplay(pixa, 0, 0);
    regTestWritePixAndCheck(pixd, IFF_PNG, &count, rp);
    pixDisplayWithTitle(pixd, 50, 50, NULL, display);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

        /* Grayscale, small pix */
    pixg = pixConvertTo8(pixs1, 0);
    pixa = pixaCreate(0);
    pixt = pixQuadraticVShear(pixg, L_WARP_TO_LEFT,
                                   60, -20, L_SAMPLED, L_BRING_IN_WHITE);
    PixSave(&pixt, pixa, 1, bmf, "sampled-left");
    pixt = pixQuadraticVShear(pixg, L_WARP_TO_RIGHT,
                                   60, -20, L_SAMPLED, L_BRING_IN_WHITE);
    PixSave(&pixt, pixa, 0, bmf, "sampled-right");
    pixt = pixQuadraticVShear(pixg, L_WARP_TO_LEFT,
                                   60, -20, L_INTERPOLATED, L_BRING_IN_WHITE);
    PixSave(&pixt, pixa, 1, bmf, "interpolated-left");
    pixt = pixQuadraticVShear(pixg, L_WARP_TO_RIGHT,
                                   60, -20, L_INTERPOLATED, L_BRING_IN_WHITE);
    PixSave(&pixt, pixa, 0, bmf, "interpolated-right");
    pixd = pixaDisplay(pixa, 0, 0);
    regTestWritePixAndCheck(pixd, IFF_PNG, &count, rp);
    pixDisplayWithTitle(pixd, 250, 50, NULL, display);
    pixDestroy(&pixg);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

        /* Color, larger pix */
    pixa = pixaCreate(0);
    pixt = pixQuadraticVShear(pixs2, L_WARP_TO_LEFT,
                              120, -40, L_SAMPLED, L_BRING_IN_WHITE);
    PixSave(&pixt, pixa, 1, bmf, "sampled-left");
    pixt = pixQuadraticVShear(pixs2, L_WARP_TO_RIGHT,
                              120, -40, L_SAMPLED, L_BRING_IN_WHITE);
    PixSave(&pixt, pixa, 0, bmf, "sampled-right");
    pixt = pixQuadraticVShear(pixs2, L_WARP_TO_LEFT,
                              120, -40, L_INTERPOLATED, L_BRING_IN_WHITE);
    PixSave(&pixt, pixa, 1, bmf, "interpolated-left");
    pixt = pixQuadraticVShear(pixs2, L_WARP_TO_RIGHT,
                              120, -40, L_INTERPOLATED, L_BRING_IN_WHITE);
    PixSave(&pixt, pixa, 0, bmf, "interpolated-right");
    pixd = pixaDisplay(pixa, 0, 0);
    regTestWritePixAndCheck(pixd, IFF_PNG, &count, rp);
    pixDisplayWithTitle(pixd, 550, 50, NULL, display);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

        /* Grayscale, larger pix */
    pixg = pixConvertTo8(pixs2, 0);
    pixa = pixaCreate(0);
    pixt = pixQuadraticVShear(pixg, L_WARP_TO_LEFT,
                              60, -20, L_SAMPLED, L_BRING_IN_WHITE);
    PixSave(&pixt, pixa, 1, bmf, "sampled-left");
    pixt = pixQuadraticVShear(pixg, L_WARP_TO_RIGHT,
                              60, -20, L_SAMPLED, L_BRING_IN_WHITE);
    PixSave(&pixt, pixa, 0, bmf, "sampled-right");
    pixt = pixQuadraticVShear(pixg, L_WARP_TO_LEFT,
                              60, -20, L_INTERPOLATED, L_BRING_IN_WHITE);
    PixSave(&pixt, pixa, 1, bmf, "interpolated-left");
    pixt = pixQuadraticVShear(pixg, L_WARP_TO_RIGHT,
                              60, -20, L_INTERPOLATED, L_BRING_IN_WHITE);
    PixSave(&pixt, pixa, 0, bmf, "interpolated-right");
    pixd = pixaDisplay(pixa, 0, 0);
    regTestWritePixAndCheck(pixd, IFF_PNG, &count, rp);
    pixDisplayWithTitle(pixd, 850, 50, NULL, display);
    pixDestroy(&pixg);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixDestroy(&pixs1);
    pixDestroy(&pixs2);
    bmfDestroy(&bmf);
    regTestCleanup(argc, argv, fp, success, rp);
    return 0;
}


void 
PixSave(PIX        **ppixs,
        PIXA        *pixa,
        l_int32      newrow,
        L_BMF       *bmf,
        const char  *textstr)
{
    PROCNAME("PixSave");
    if (!ppixs || !(*ppixs)) {
        L_ERROR("pixs not defined", procName);
        return;
    }

        /* Scaling is done after adding border pixels.  Therefore, to
         * avoid rescaling, add twice the border pixels to the target width */
    pixSaveTiledWithText(*ppixs, pixa, pixGetWidth(*ppixs) + 6,
                         newrow, 20, 3, bmf, textstr, 0xff000000, L_ADD_BELOW);
    pixDestroy(ppixs);
}

  
