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
 * bilineartest.c
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static void MakePtas(l_int32 i, PTA **pptas, PTA **pptad);

    /* Sample values.
     *    1: test with relatively large distortion
     *    2-3: invertability tests
     */
static const l_int32  x1[] =  {  32,   32,   32};
static const l_int32  y1[] =  { 150,  150,  150};
static const l_int32  x2[] =  { 520,  520,  520};
static const l_int32  y2[] =  { 150,  150,  150};
static const l_int32  x3[] =  {  32,   32,   32};
static const l_int32  y3[] =  { 612,  612,  612};
static const l_int32  x4[] =  { 520,  520,  520};
static const l_int32  y4[] =  { 612,  612,  612};

static const l_int32  xp1[] = {  32,   32,   32};
static const l_int32  yp1[] = { 150,  150,  150};
static const l_int32  xp2[] = { 520,  520,  520};
static const l_int32  yp2[] = {  44,  124,  140};
static const l_int32  xp3[] = {  32,   32,   32};
static const l_int32  yp3[] = { 612,  612,  612};
static const l_int32  xp4[] = { 520,  520,  520};
static const l_int32  yp4[] = { 694,  624,  622};

#define  ALL                        1
#define  ADDED_BORDER_PIXELS      500


main(int    argc,
     char **argv)
{
l_int32      i, d, h;
l_float32    rat;
PIX         *pixs, *pixgb, *pixt1, *pixt2, *pixt3, *pixt4, *pixg, *pixd;
PIXA        *pixa;
PTA         *ptas, *ptad;
static char  mainName[] = "bilinear_reg";

    if (argc != 1)
	exit(ERROR_INT(" Syntax:  bilinear_reg", mainName, 1));

    pixs = pixRead("feyn.tif");
    pixg = pixScaleToGray3(pixs);

#if ALL
        /* Test non-invertability of sampling */
    pixa = pixaCreate(0);
    for (i = 1; i < 3; i++) {
        pixgb = pixAddBorder(pixg, ADDED_BORDER_PIXELS, 255);
        MakePtas(i, &ptas, &ptad);
        pixt1 = pixBilinearSampledPta(pixgb, ptad, ptas, L_BRING_IN_WHITE);
        pixSaveTiled(pixt1, pixa, 2, 1, 20, 8);
        pixt2 = pixBilinearSampledPta(pixt1, ptas, ptad, L_BRING_IN_WHITE);
        pixSaveTiled(pixt2, pixa, 2, 0, 20, 0);
        pixd = pixRemoveBorder(pixt2, ADDED_BORDER_PIXELS);
        pixInvert(pixd, pixd);
        pixXor(pixd, pixd, pixg);
        pixSaveTiled(pixd, pixa, 2, 0, 20, 0);
        if (i == 0) pixWrite("/tmp/junksamp.png", pixt1, IFF_PNG);
        pixDestroy(&pixgb);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixd);
        ptaDestroy(&ptas);
        ptaDestroy(&ptad);
    }

    pixt1 = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/junkbilin1.png", pixt1, IFF_PNG);
    pixDisplay(pixt1, 100, 300);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);
#endif

#if ALL
        /* Test non-invertability of interpolation */
    pixa = pixaCreate(0);
    for (i = 1; i < 3; i++) {
        pixgb = pixAddBorder(pixg, ADDED_BORDER_PIXELS, 255);
        MakePtas(i, &ptas, &ptad);
        pixt1 = pixBilinearPta(pixgb, ptad, ptas, L_BRING_IN_WHITE);
        pixSaveTiled(pixt1, pixa, 2, 1, 20, 8);
        pixt2 = pixBilinearPta(pixt1, ptas, ptad, L_BRING_IN_WHITE);
        pixSaveTiled(pixt2, pixa, 2, 0, 20, 0);
        pixd = pixRemoveBorder(pixt2, ADDED_BORDER_PIXELS);
        pixInvert(pixd, pixd);
        pixXor(pixd, pixd, pixg);
        pixSaveTiled(pixd, pixa, 2, 0, 20, 0);
        if (i == 0) pixWrite("/tmp/junkinterp.png", pixt1, IFF_PNG);
        pixDestroy(&pixgb);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixd);
        ptaDestroy(&ptas);
        ptaDestroy(&ptad);
    }

    pixt1 = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/junkbilin2.png", pixt1, IFF_PNG);
    pixDisplay(pixt1, 100, 300);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);
#endif

#if ALL   /* test with large distortion and inversion */
    MakePtas(0, &ptas, &ptad);
    pixa = pixaCreate(0);

    startTimer();
    pixt1 = pixBilinearSampledPta(pixg, ptas, ptad, L_BRING_IN_WHITE);
    fprintf(stderr, " Time for pixBilinearSampled(): %6.2f sec\n", stopTimer());
    pixSaveTiled(pixt1, pixa, 2, 1, 20, 8);

    startTimer();
    pixt2 = pixBilinearPta(pixg, ptas, ptad, L_BRING_IN_WHITE);
    fprintf(stderr, " Time for pixBilinearInterpolated(): %6.2f sec\n",
           stopTimer());
    pixSaveTiled(pixt2, pixa, 2, 0, 20, 8);

    pixt3 = pixBilinearSampledPta(pixt1, ptad, ptas, L_BRING_IN_WHITE);
    pixSaveTiled(pixt3, pixa, 2, 0, 20, 8);
    pixt4 = pixBilinearPta(pixt2, ptad, ptas, L_BRING_IN_WHITE);
    pixSaveTiled(pixt4, pixa, 2, 0, 20, 8);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

    pixt1 = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/junkbilin3.png", pixt1, IFF_PNG);
    pixDisplay(pixt1, 100, 300);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);

    pixDestroy(&pixs);
    pixDestroy(&pixg);
    ptaDestroy(&ptas);
    ptaDestroy(&ptad);
#endif

    return 0;
}


static void
MakePtas(l_int32  i,
         PTA    **pptas,
         PTA    **pptad)
{
    *pptas = ptaCreate(4);
    ptaAddPt(*pptas, x1[i], y1[i]);
    ptaAddPt(*pptas, x2[i], y2[i]);
    ptaAddPt(*pptas, x3[i], y3[i]);
    ptaAddPt(*pptas, x4[i], y4[i]);
    *pptad = ptaCreate(4);
    ptaAddPt(*pptad, xp1[i], yp1[i]);
    ptaAddPt(*pptad, xp2[i], yp2[i]);
    ptaAddPt(*pptad, xp3[i], yp3[i]);
    ptaAddPt(*pptad, xp4[i], yp4[i]);
    return;
}

