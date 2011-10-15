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
 *   xformbox_reg.c
 *
 *      Tests ordered box transforms (rotation, scaling, translation).
 *      Also tests the various box hashing graphics operations.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

    /* Consts for second set */
static const l_int32  SHIFTX_2 = 50;
static const l_int32  SHIFTY_2 = 70;
static const l_float32  SCALEX_2 = 1.17;
static const l_float32  SCALEY_2 = 1.13;
static const l_float32  ROTATION_2 = 0.10;   /* radian */

    /* Consts for third set */
static const l_int32  SHIFTX_3 = 44;
static const l_int32  SHIFTY_3 = 39;
static const l_float32  SCALEX_3 = 0.83;
static const l_float32  SCALEY_3 = 0.78;
static const l_float32  ROTATION_3 = 0.11;   /* radian */


l_int32 RenderTransformedBoxa(PIX *pixt, BOXA *boxa, l_int32 i);


main(int    argc,
     char **argv)
{
l_int32      i, n, ws, hs, w, h, rval, gval, bval, order;
l_float32   *mat1, *mat2, *mat3;
l_float32    matd[9];
BOX         *box, *boxt;
BOXA        *boxa, *boxat, *boxa1, *boxa2, *boxa3, *boxa4, *boxa5;
PIX         *pix, *pixs, *pixb, *pixc, *pixt, *pixt1, *pixt2, *pixt3;
PIXA        *pixa;
static char  mainName[] = "xformbox_reg";

    /* ----------------------------------------------------------- *
     *                Test hash rendering in 3 modes               *
     * ----------------------------------------------------------- */
    pixs = pixRead("feyn.tif");
    box = boxCreate(461, 429, 1393, 342);
    pixt1 = pixClipRectangle(pixs, box, NULL);
    boxa = pixConnComp(pixt1, NULL, 8);
    n = boxaGetCount(boxa);
    pixt2 = pixConvertTo8(pixt1, 1);
    pixt3 = pixConvertTo32(pixt1);
    for (i = 0; i < n; i++) {
        boxt = boxaGetBox(boxa, i, L_CLONE);
	rval = (1413 * i) % 256;
	gval = (4917 * i) % 256;
	bval = (7341 * i) % 256;
	pixRenderHashBox(pixt1, boxt, 8, 2, i % 4, 1, L_SET_PIXELS);
	pixRenderHashBoxArb(pixt2, boxt, 7, 2, i % 4, 1, rval, gval, bval);
	pixRenderHashBoxBlend(pixt3, boxt, 7, 2, i % 4, 1, rval, gval, bval,
                              0.5);
	boxDestroy(&boxt);
    }
    pixDisplay(pixt1, 0, 0);
    pixDisplay(pixt2, 0, 300);
    pixDisplay(pixt3, 0, 570);
    pixWrite("/tmp/junkpixt1.png", pixt1, IFF_PNG);
    pixWrite("/tmp/junkpixt2.png", pixt2, IFF_PNG);
    pixWrite("/tmp/junkpixt3.png", pixt3, IFF_PNG);

    boxaDestroy(&boxa);
    boxDestroy(&box);
    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);


    /* ----------------------------------------------------------- *
     *    Test box transforms with either translation or scaling   *
     *    combined with rotation, using the simple 'ordered'       *
     *    function.  Show that the order of the operations does    *
     *    not matter; different hashing schemes end up in the      *
     *    identical boxes.                                         *
     * ----------------------------------------------------------- */
    pix = pixRead("feyn.tif");
    box = boxCreate(420, 360, 1500, 465);
    pixt = pixClipRectangle(pix, box, NULL);
    pixs = pixAddBorderGeneral(pixt, 0, 200, 0, 0, 0);
    boxDestroy(&box);
    pixDestroy(&pix);
    pixDestroy(&pixt);
    boxa = pixConnComp(pixs, NULL, 8);
    n = boxaGetCount(boxa);
    pixa = pixaCreate(0);

    pixt = pixConvertTo32(pixs);
    for (i = 0; i < 3; i++) {
        if (i == 0)
            order = L_TR_SC_RO;
        else if (i == 1)
            order = L_TR_RO_SC;
        else
            order = L_SC_TR_RO;
        boxat = boxaTransformOrdered(boxa, SHIFTX_2, SHIFTY_2, 1.0, 1.0,
                                     450, 250, ROTATION_2, order);
        RenderTransformedBoxa(pixt, boxat, i);
        boxaDestroy(&boxat);
    }
    pixSaveTiled(pixt, pixa, 1, 1, 30, 32);
    pixDestroy(&pixt);

    pixt = pixConvertTo32(pixs);
    for (i = 0; i < 3; i++) {
        if (i == 0)
            order = L_RO_TR_SC;
        else if (i == 1)
            order = L_RO_SC_TR;
        else
            order = L_SC_RO_TR;
        boxat = boxaTransformOrdered(boxa, SHIFTX_2, SHIFTY_2, 1.0, 1.0,
                                     450, 250, ROTATION_2, order);
        RenderTransformedBoxa(pixt, boxat, i + 4);
        boxaDestroy(&boxat);
    }
    pixSaveTiled(pixt, pixa, 1, 1, 30, 0);
    pixDestroy(&pixt);

    pixt = pixConvertTo32(pixs);
    for (i = 0; i < 3; i++) {
        if (i == 0)
            order = L_TR_SC_RO;
        else if (i == 1)
            order = L_SC_RO_TR;
        else
            order = L_SC_TR_RO;
        boxat = boxaTransformOrdered(boxa, 0, 0, SCALEX_2, SCALEY_2,
                                     450, 250, ROTATION_2, order);
        RenderTransformedBoxa(pixt, boxat, i + 8);
        boxaDestroy(&boxat);
    }
    pixSaveTiled(pixt, pixa, 1, 1, 30, 0);
    pixDestroy(&pixt);

    pixt = pixConvertTo32(pixs);
    for (i = 0; i < 3; i++) {
        if (i == 0)
            order = L_RO_TR_SC;
        else if (i == 1)
            order = L_RO_SC_TR;
        else
            order = L_TR_RO_SC;
        boxat = boxaTransformOrdered(boxa, 0, 0, SCALEX_2, SCALEY_2,
                                     450, 250, ROTATION_2, order);
        RenderTransformedBoxa(pixt, boxat, i + 16);
        boxaDestroy(&boxat);
    }
    pixSaveTiled(pixt, pixa, 1, 1, 30, 0);
    pixDestroy(&pixt);

    pixt = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/junkxform1.png", pixt, IFF_PNG);
    pixDisplay(pixt, 1000, 0);
    pixDestroy(&pixt);
    pixDestroy(&pixs);
    boxaDestroy(&boxa);
    pixaDestroy(&pixa);


    /* ----------------------------------------------------------- *
     *    Do more testing of box and pta transforms.  Show that    *
     *    resulting boxes are identical by three methods.          *
     * ----------------------------------------------------------- */
        /* Set up pix and boxa */ 
    pixa = pixaCreate(0);
    pix = pixRead("lucasta.1.300.tif");
    pixTranslate(pix, pix, 70, 0, L_BRING_IN_WHITE);
    pixt = pixCloseBrick(NULL, pix, 14, 5);
    pixOpenBrick(pixt, pixt, 1, 2);
    boxa = pixConnComp(pixt, NULL, 8);
    pixs = pixConvertTo32(pix);
    pixc = pixCopy(NULL, pixs);
    RenderTransformedBoxa(pixc, boxa, 113);
    pixSaveTiled(pixc, pixa, 2, 1, 30, 32);
    pixDestroy(&pix);
    pixDestroy(&pixc);
    pixDestroy(&pixt);

        /* (a) Do successive discrete operations: shift, scale, rotate */
    pixt1 = pixTranslate(NULL, pixs, SHIFTX_3, SHIFTY_3, L_BRING_IN_WHITE);    
    boxa1 = boxaTranslate(boxa, SHIFTX_3, SHIFTY_3);
    pixc = pixCopy(NULL, pixt1);
    RenderTransformedBoxa(pixc, boxa1, 213);
    pixSaveTiled(pixc, pixa, 2, 0, 30, 32);
    pixDestroy(&pixc);

    pixt2 = pixScale(pixt1, SCALEX_3, SCALEY_3);
    boxa2 = boxaScale(boxa1, SCALEX_3, SCALEY_3);
    pixc = pixCopy(NULL, pixt2);
    RenderTransformedBoxa(pixc, boxa2, 313);
    pixSaveTiled(pixc, pixa, 2, 1, 30, 32);
    pixDestroy(&pixc);

    pixGetDimensions(pixt2, &w, &h, NULL);
    pixt3 = pixRotateAM(pixt2, ROTATION_3, L_BRING_IN_WHITE);
    boxa3 = boxaRotate(boxa2, w / 2, h / 2, ROTATION_3);
    pixc = pixCopy(NULL, pixt3);
    RenderTransformedBoxa(pixc, boxa3, 413);
    pixSaveTiled(pixc, pixa, 2, 0, 30, 32);
    pixDestroy(&pixc);

        /* (b) Set up and use the composite transform */
    mat1 = createMatrix2dTranslate(SHIFTX_3, SHIFTY_3);
    mat2 = createMatrix2dScale(SCALEX_3, SCALEY_3);
    mat3 = createMatrix2dRotate(w / 2, h / 2, ROTATION_3);
    l_productMat3(mat3, mat2, mat1, matd, 3);
    boxa4 = boxaAffineTransform(boxa, matd);
    pixc = pixCopy(NULL, pixt3);
    RenderTransformedBoxa(pixc, boxa4, 513);
    pixSaveTiled(pixc, pixa, 2, 1, 30, 32);
    pixDestroy(&pixc);

        /* (c) Use the special 'ordered' function */
    pixGetDimensions(pixs, &ws, &hs, NULL);
    boxa5 = boxaTransformOrdered(boxa, SHIFTX_3, SHIFTY_3,
                                 SCALEX_3, SCALEY_3,
                                 ws / 2, hs / 2, ROTATION_3, L_TR_SC_RO);
    pixc = pixCopy(NULL, pixt3);
    RenderTransformedBoxa(pixc, boxa5, 613);
    pixSaveTiled(pixc, pixa, 2, 0, 30, 32);
    pixDestroy(&pixc);
                             
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
    boxaDestroy(&boxa3);
    boxaDestroy(&boxa4);
    boxaDestroy(&boxa5);
    FREE(mat1);
    FREE(mat2);
    FREE(mat3);

    pixt = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/junkxform2.png", pixt, IFF_PNG);
    pixDisplay(pixt, 1000, 300);
    pixDestroy(&pixt);
    pixDestroy(&pixs);
    boxaDestroy(&boxa);
    pixaDestroy(&pixa);
    return 0;
}


l_int32
RenderTransformedBoxa(PIX    *pixt,
                      BOXA   *boxa,
                      l_int32 i)
{
l_int32  j, n, rval, gval, bval;
BOX     *box;

    n = boxaGetCount(boxa);
    rval = (1413 * i) % 256;
    gval = (4917 * i) % 256;
    bval = (7341 * i) % 256;
    for (j = 0; j < n; j++) {
        box = boxaGetBox(boxa, j, L_CLONE);
        pixRenderHashBoxArb(pixt, box, 10, 3, i % 4, 1, rval, gval, bval);
        boxDestroy(&box);
    }
    return 0;
}



