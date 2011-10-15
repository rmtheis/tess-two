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
 * affine_reg.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static void MakePtas(l_int32 i, PTA **pptas, PTA **pptad);
l_int32 RenderHashedBoxa(PIX *pixt, BOXA *boxa, l_int32 i);


    /* Sample values.
     *    1-3: invertability tests
     *    4: comparison between sampling and sequential
     *    5: test with large distortion
     */
static const l_int32  x1[] =  { 300,  300,  300,  95,    32};
static const l_int32  y1[] =  {1200, 1200, 1250, 2821,  934};
static const l_int32  x2[] =  {1200, 1200, 1125, 1432,  487};
static const l_int32  y2[] =  {1100, 1100, 1100, 2682,  934};
static const l_int32  x3[] =  { 200,  200,  200,  232,   32};
static const l_int32  y3[] =  { 200,  200,  200,  657,   67};

static const l_int32  xp1[] = { 500,  300,  350,  117,   32};
static const l_int32  yp1[] = {1700, 1400, 1400, 2629,  934};
static const l_int32  xp2[] = {850, 1400, 1400, 1464,  487};
static const l_int32  yp2[] = {850, 1500, 1500, 2432,  804};
static const l_int32  xp3[] = { 450,  200,  400,  183,   61};
static const l_int32  yp3[] = { 300,  300,  400,  490,   83};

static const l_int32  SHIFTX = 44;
static const l_int32  SHIFTY = 39;
static const l_float32  SCALEX = 0.83;
static const l_float32  SCALEY = 0.78;
static const l_float32  ROTATION = 0.11;   /* radian */

#define   ADDED_BORDER_PIXELS       1000
#define   ALL     0


main(int    argc,
     char **argv)
{
char         bufname[256];
l_int32      i, j, w, h, d, x, y, wpls;
l_uint32    *datas, *lines;
l_float32   *vc;
l_float32   *mat1, *mat2, *mat3, *mat1i, *mat2i, *mat3i, *matdinv;
l_float32    matd[9], matdi[9];
BOXA        *boxa, *boxa2;
PIX         *pix, *pixs, *pixb, *pixg, *pixc, *pixcs;
PIX         *pixd, *pixt1, *pixt2, *pixt3;
PIXA        *pixa;
PTA         *ptas, *ptad;
static char  mainName[] = "affine_reg";

    if (argc != 1)
	exit(ERROR_INT(" Syntax:  affine_reg", mainName, 1));

    if ((pixs = pixRead("feyn.tif")) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));

#if ALL
        /* Test invertability of sequential. */
    pixa = pixaCreate(0);
    for (i = 0; i < 3; i++) {
        pixb = pixAddBorder(pixs, ADDED_BORDER_PIXELS, 0);
        MakePtas(i, &ptas, &ptad);
        pixt1 = pixAffineSequential(pixb, ptad, ptas, 0, 0);
        pixSaveTiled(pixt1, pixa, 3, 1, 20, 8);
        pixt2 = pixAffineSequential(pixt1, ptas, ptad, 0, 0);
        pixSaveTiled(pixt2, pixa, 3, 0, 20, 0);
        pixd = pixRemoveBorder(pixt2, ADDED_BORDER_PIXELS);
        pixXor(pixd, pixd, pixs);
        pixSaveTiled(pixd, pixa, 3, 0, 20, 0);
        sprintf(bufname, "/tmp/junkseq%d.png", i);
        pixWrite(bufname, pixd, IFF_PNG);
        pixDestroy(&pixb);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixd);
        ptaDestroy(&ptas);
        ptaDestroy(&ptad);
    }

    pixt1 = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/junkaffine1.png", pixt1, IFF_PNG);
    pixDisplay(pixt1, 100, 100);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);
#endif

#if ALL 
        /* Test invertability of sampling */
    pixa = pixaCreate(0);
    for (i = 0; i < 3; i++) {
        pixb = pixAddBorder(pixs, ADDED_BORDER_PIXELS, 0);
        MakePtas(i, &ptas, &ptad);
        pixt1 = pixAffineSampledPta(pixb, ptad, ptas, L_BRING_IN_WHITE);
        pixSaveTiled(pixt1, pixa, 3, 1, 20, 8);
        pixt2 = pixAffineSampledPta(pixt1, ptas, ptad, L_BRING_IN_WHITE);
        pixSaveTiled(pixt2, pixa, 3, 0, 20, 0);
        pixd = pixRemoveBorder(pixt2, ADDED_BORDER_PIXELS);
        pixXor(pixd, pixd, pixs);
        pixSaveTiled(pixd, pixa, 3, 0, 20, 0);
        if (i == 0) pixWrite("/tmp/junksamp.png", pixt1, IFF_PNG);
        pixDestroy(&pixb);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixd);
        ptaDestroy(&ptas);
        ptaDestroy(&ptad);
    }

    pixt1 = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/junkaffine2.png", pixt1, IFF_PNG);
    pixDisplay(pixt1, 100, 300);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);
#endif

#if ALL
        /* Test invertability of interpolation on grayscale */
    pixa = pixaCreate(0);
    pixg = pixScaleToGray3(pixs);
    for (i = 0; i < 3; i++) {
        pixb = pixAddBorder(pixg, ADDED_BORDER_PIXELS / 3, 255);
        MakePtas(i, &ptas, &ptad);
        pixt1 = pixAffinePta(pixb, ptad, ptas, L_BRING_IN_WHITE);
        pixSaveTiled(pixt1, pixa, 1, 1, 20, 8);
        pixt2 = pixAffinePta(pixt1, ptas, ptad, L_BRING_IN_WHITE);
        pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);
        pixd = pixRemoveBorder(pixt2, ADDED_BORDER_PIXELS / 3);
        pixXor(pixd, pixd, pixg);
        pixSaveTiled(pixd, pixa, 1, 0, 20, 0);
        if (i == 0) pixWrite("/tmp/junkinterp.png", pixt1, IFF_PNG);
        pixDestroy(&pixb);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixd);
        ptaDestroy(&ptas);
        ptaDestroy(&ptad);
    }

    pixt1 = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/junkaffine3.png", pixt1, IFF_PNG);
    pixDisplay(pixt1, 100, 500);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);
    pixDestroy(&pixg);
#endif

#if ALL
        /* Test invertability of interpolation on color */
    pixa = pixaCreate(0);
    pixc = pixRead("test24.jpg");
    pixcs = pixScale(pixc, 0.3, 0.3);
    for (i = 0; i < 3; i++) {
        pixb = pixAddBorder(pixcs, ADDED_BORDER_PIXELS / 4, 0xffffff00);
        MakePtas(i, &ptas, &ptad);
        pixt1 = pixAffinePta(pixb, ptad, ptas, L_BRING_IN_WHITE);
        pixSaveTiled(pixt1, pixa, 1, 1, 20, 32);
        pixt2 = pixAffinePta(pixt1, ptas, ptad, L_BRING_IN_WHITE);
        pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);
        pixd = pixRemoveBorder(pixt2, ADDED_BORDER_PIXELS / 4);
        pixXor(pixd, pixd, pixcs);
        pixSaveTiled(pixd, pixa, 1, 0, 20, 0);
        pixDestroy(&pixb);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixd);
        ptaDestroy(&ptas);
        ptaDestroy(&ptad);
    }

    pixt1 = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/junkaffine4.png", pixt1, IFF_PNG);
    pixDisplay(pixt1, 100, 500);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);
    pixDestroy(&pixc);
    pixDestroy(&pixcs);
#endif

#if ALL 
       /* Comparison between sequential and sampling */
    MakePtas(3, &ptas, &ptad);
    pixa = pixaCreate(0);

	/* Use sequential transforms */
    pixt1 = pixAffineSequential(pixs, ptas, ptad,
                     ADDED_BORDER_PIXELS, ADDED_BORDER_PIXELS);
    pixSaveTiled(pixt1, pixa, 2, 0, 20, 8);

	/* Use sampled transform */
    pixt2 = pixAffineSampledPta(pixs, ptas, ptad, L_BRING_IN_WHITE);
    pixSaveTiled(pixt2, pixa, 2, 0, 20, 8);

        /* Compare the results */
    pixXor(pixt2, pixt2, pixt1);
    pixSaveTiled(pixt2, pixa, 2, 0, 20, 8);

    pixd = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/junkaffine5.png", pixd, IFF_PNG);
    pixDisplay(pixd, 100, 700);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    ptaDestroy(&ptas);
    ptaDestroy(&ptad);
#endif

#if ALL 
       /* Get timings and test with large distortion */
    MakePtas(4, &ptas, &ptad);
    pixa = pixaCreate(0);
    pixg = pixScaleToGray3(pixs);

    startTimer();
    pixt1 = pixAffineSequential(pixg, ptas, ptad, 0, 0);
    fprintf(stderr, " Time for pixAffineSequentialPta(): %6.2f sec\n",
            stopTimer());
    pixSaveTiled(pixt1, pixa, 1, 1, 20, 8);

    startTimer();
    pixt2 = pixAffineSampledPta(pixg, ptas, ptad, L_BRING_IN_WHITE);
    fprintf(stderr, " Time for pixAffineSampledPta(): %6.2f sec\n", stopTimer());
    pixSaveTiled(pixt2, pixa, 1, 0, 20, 8);

    startTimer();
    pixt3 = pixAffinePta(pixg, ptas, ptad, L_BRING_IN_WHITE);
    fprintf(stderr, " Time for pixAffinePta(): %6.2f sec\n", stopTimer());
    pixSaveTiled(pixt3, pixa, 1, 0, 20, 8);

    pixXor(pixt1, pixt1, pixt2);
    pixSaveTiled(pixt1, pixa, 1, 1, 20, 8);
    pixXor(pixt2, pixt2, pixt3);
    pixSaveTiled(pixt2, pixa, 1, 0, 20, 8);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);

    pixd = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/junkaffine6.png", pixd, IFF_PNG);
    pixDisplay(pixd, 100, 900);
    pixDestroy(&pixd);
    pixDestroy(&pixg);
    pixaDestroy(&pixa);
    ptaDestroy(&ptas);
    ptaDestroy(&ptad);
#endif

    pixDestroy(&pixs);

#if 1
        /* Set up pix and boxa */
    pixa = pixaCreate(0);
    pix = pixRead("lucasta.1.300.tif");
    pixTranslate(pix, pix, 70, 0, L_BRING_IN_WHITE);
    pixt1 = pixCloseBrick(NULL, pix, 14, 5);
    pixOpenBrick(pixt1, pixt1, 1, 2);
    boxa = pixConnComp(pixt1, NULL, 8);
    pixs = pixConvertTo32(pix);
    pixGetDimensions(pixs, &w, &h, NULL);
    pixc = pixCopy(NULL, pixs);
    RenderHashedBoxa(pixc, boxa, 113);
    pixSaveTiled(pixc, pixa, 2, 1, 30, 32);
    pixDestroy(&pix);
    pixDestroy(&pixc);
    pixDestroy(&pixt1);

        /* Set up an affine transform in matd, and apply it to boxa */
    mat1 = createMatrix2dTranslate(SHIFTX, SHIFTY);
    mat2 = createMatrix2dScale(SCALEX, SCALEY);
    mat3 = createMatrix2dRotate(w / 2, h / 2, ROTATION);
    l_productMat3(mat3, mat2, mat1, matd, 3);
    boxa2 = boxaAffineTransform(boxa, matd);

        /* Set up the inverse transform in matdi */
    mat1i = createMatrix2dTranslate(-SHIFTX, -SHIFTY);
    mat2i = createMatrix2dScale(1.0/ SCALEX, 1.0 / SCALEY);
    mat3i = createMatrix2dRotate(w / 2, h / 2, -ROTATION);
    l_productMat3(mat1i, mat2i, mat3i, matdi, 3);

        /* Invert the original affine transform in matdinv */
    affineInvertXform(matd, &matdinv);
    fprintf(stderr, "Affine transform, applied to boxa\n");
    for (i = 0; i < 9; i++) {
        if (i && (i % 3 == 0))  fprintf(stderr, "\n");
        fprintf(stderr, " %7.3f ", matd[i]);
    }
    fprintf(stderr, "\nInverse transform, made by composing inverse parts");
    for (i = 0; i < 9; i++) {
        if (i % 3 == 0)  fprintf(stderr, "\n");
        fprintf(stderr, " %7.3f ", matdi[i]);
    }
    fprintf(stderr, "\nInverse transform, made by inverting the affine xform");
    for (i = 0; i < 6; i++) {
        if (i % 3 == 0)  fprintf(stderr, "\n");
        fprintf(stderr, " %7.3f ", matdinv[i]);
    }
    fprintf(stderr, "\n");

        /* Apply the inverted affine transform pixs */
    pixd = pixAffine(pixs, matdinv, L_BRING_IN_WHITE);
    RenderHashedBoxa(pixd, boxa2, 513);
    pixSaveTiled(pixd, pixa, 2, 0, 30, 32);
    pixDestroy(&pixd);

    pixd = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/junkaffine7.png", pixd, IFF_PNG);
    pixDisplay(pixd, 100, 900);
    pixDestroy(&pixd);
    pixDestroy(&pixs);
    pixaDestroy(&pixa);
    boxaDestroy(&boxa);
    boxaDestroy(&boxa2);
    FREE(mat1);
    FREE(mat2);
    FREE(mat3);
    FREE(mat1i);
    FREE(mat2i);
    FREE(mat3i);
#endif

    return 0;
}

static void
MakePtas(l_int32  i,
         PTA    **pptas,
         PTA    **pptad)
{

    *pptas = ptaCreate(3);
    ptaAddPt(*pptas, x1[i], y1[i]);
    ptaAddPt(*pptas, x2[i], y2[i]);
    ptaAddPt(*pptas, x3[i], y3[i]);
    *pptad = ptaCreate(3);
    ptaAddPt(*pptad, xp1[i], yp1[i]);
    ptaAddPt(*pptad, xp2[i], yp2[i]);
    ptaAddPt(*pptad, xp3[i], yp3[i]);
    return;
}


l_int32
RenderHashedBoxa(PIX    *pixt,
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


