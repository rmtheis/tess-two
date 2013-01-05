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
 * projective_reg.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static void MakePtas(l_int32 i, PTA **pptas, PTA **pptad);

    /* Sample values.
     *    1-3: invertability tests
     *    4: comparison between sampling and sequential
     *    5: test with large distortion
     */
static const l_int32  x1[] =  { 300,  300,  300,  300,   32};
static const l_int32  y1[] =  {1200, 1200, 1250, 1250,  934};
static const l_int32  x2[] =  {1200, 1200, 1125, 1300,  487};
static const l_int32  y2[] =  {1100, 1100, 1100, 1250,  934};
static const l_int32  x3[] =  { 200,  200,  200,  250,   32};
static const l_int32  y3[] =  { 200,  200,  200,  300,   67};
static const l_int32  x4[] =  {1200, 1200, 1300, 1250,  332};
static const l_int32  y4[] =  { 400,  200,  200,  300,   57};

static const l_int32  xp1[] = { 300,  300, 1150,  300,   32};
static const l_int32  yp1[] = {1200, 1400, 1150, 1350,  934};
static const l_int32  xp2[] = {1100, 1400,  320, 1300,  487};
static const l_int32  yp2[] = {1000, 1500, 1300, 1200,  904};
static const l_int32  xp3[] = { 250,  200, 1310,  300,   61};
static const l_int32  yp3[] = { 200,  300,  250,  325,   83};
static const l_int32  xp4[] = {1250, 1200,  240, 1250,  412};
static const l_int32  yp4[] = { 300,  300,  250,  350,   83};

#define   ADDED_BORDER_PIXELS       500
#define   ALL     1


main(int    argc,
     char **argv)
{
char         bufname[256];
l_int32      i, j, w, h, d, x, y, wpls;
l_uint32    *datas, *lines;
l_float32   *vc;
PIX         *pixs, *pixsc, *pixb, *pixg, *pixc, *pixcs, *pixd;
PIX         *pixt1, *pixt2, *pixt3;
PIXA        *pixa;
PTA         *ptas, *ptad;
static char  mainName[] = "projective_reg";

    if (argc != 1)
	exit(ERROR_INT(" Syntax:  projective_reg", mainName, 1));
    if ((pixs = pixRead("feyn.tif")) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
    pixsc = pixScale(pixs, 0.5, 0.5);

#if ALL
        /* Test invertability of sampling */
    pixa = pixaCreate(0);
    for (i = 0; i < 3; i++) {
        pixb = pixAddBorder(pixsc, ADDED_BORDER_PIXELS, 0);
        MakePtas(i, &ptas, &ptad);
        pixt1 = pixProjectiveSampledPta(pixb, ptad, ptas, L_BRING_IN_WHITE);
        pixSaveTiled(pixt1, pixa, 1, 1, 20, 8);
        pixt2 = pixProjectiveSampledPta(pixt1, ptas, ptad, L_BRING_IN_WHITE);
        pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);
        pixd = pixRemoveBorder(pixt2, ADDED_BORDER_PIXELS);
        pixXor(pixd, pixd, pixsc);
        pixSaveTiled(pixd, pixa, 1, 0, 20, 0);
        if (i == 0) pixWrite("/tmp/junksamp.png", pixt1, IFF_PNG);
        pixDestroy(&pixb);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixd);
        ptaDestroy(&ptas);
        ptaDestroy(&ptad);
    }

    pixt1 = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/junkproj1.png", pixt1, IFF_PNG);
    pixDisplay(pixt1, 100, 300);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);
#endif

#if ALL
        /* Test invertability of interpolation on grayscale */
    pixa = pixaCreate(0);
    pixg = pixScaleToGray3(pixs);
    for (i = 0; i < 3; i++) {
        pixb = pixAddBorder(pixg, ADDED_BORDER_PIXELS / 2, 255);
        MakePtas(i, &ptas, &ptad);
        pixt1 = pixProjectivePta(pixb, ptad, ptas, L_BRING_IN_WHITE);
        pixSaveTiled(pixt1, pixa, 1, 1, 20, 8);
        pixt2 = pixProjectivePta(pixt1, ptas, ptad, L_BRING_IN_WHITE);
        pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);
        pixd = pixRemoveBorder(pixt2, ADDED_BORDER_PIXELS / 2);
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
    pixWrite("/tmp/junkproj2.png", pixt1, IFF_PNG);
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
    for (i = 0; i < 5; i++) {
        pixb = pixAddBorder(pixcs, ADDED_BORDER_PIXELS, 0xffffff00);
        MakePtas(i, &ptas, &ptad);
        pixt1 = pixProjectivePta(pixb, ptad, ptas, L_BRING_IN_WHITE);
        pixSaveTiled(pixt1, pixa, 1, 1, 20, 32);
        pixt2 = pixProjectivePta(pixt1, ptas, ptad, L_BRING_IN_WHITE);
        pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);
        pixd = pixRemoveBorder(pixt2, ADDED_BORDER_PIXELS);
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
    pixWrite("/tmp/junkproj3.png", pixt1, IFF_PNG);
    pixDisplay(pixt1, 100, 500);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);
    pixDestroy(&pixc);
    pixDestroy(&pixcs);
#endif

#if ALL 
       /* Comparison between sampling and interpolated */
    MakePtas(3, &ptas, &ptad);
    pixa = pixaCreate(0);

	/* Use sampled transform */
    pixt1 = pixProjectiveSampledPta(pixs, ptas, ptad, L_BRING_IN_WHITE);
    pixSaveTiled(pixt1, pixa, 2, 1, 20, 8);

	/* Use interpolated transforms */
    pixt2 = pixProjectivePta(pixs, ptas, ptad, L_BRING_IN_WHITE);
    pixSaveTiled(pixt2, pixa, 2, 0, 20, 8);

        /* Compare the results */
    pixXor(pixt2, pixt2, pixt1);
    pixSaveTiled(pixt2, pixa, 2, 0, 20, 8);

    pixd = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/junkproj4.png", pixd, IFF_PNG);
    pixDisplay(pixd, 100, 700);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    ptaDestroy(&ptas);
    ptaDestroy(&ptad);
#endif

#if ALL
       /* Get timings */
    MakePtas(4, &ptas, &ptad);
    pixa = pixaCreate(0);
    pixg = pixScaleToGray3(pixs);

    startTimer();
    pixt1 = pixProjectiveSampledPta(pixg, ptas, ptad, L_BRING_IN_WHITE);
    fprintf(stderr, " Time for pixProjectiveSampledPta(): %6.2f sec\n", stopTimer());
    pixSaveTiled(pixt1, pixa, 1, 1, 20, 8);

    startTimer();
    pixt2 = pixProjectivePta(pixg, ptas, ptad, L_BRING_IN_WHITE);
    fprintf(stderr, " Time for pixProjectivePta(): %6.2f sec\n", stopTimer());
    pixSaveTiled(pixt2, pixa, 1, 0, 20, 8);

    pixXor(pixt1, pixt1, pixt2);
    pixSaveTiled(pixt1, pixa, 1, 0, 20, 8);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

    pixd = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/junkproj5.png", pixd, IFF_PNG);
    pixDisplay(pixd, 100, 900);
    pixDestroy(&pixd);
    pixDestroy(&pixg);
    pixaDestroy(&pixa);
    ptaDestroy(&ptas);
    ptaDestroy(&ptad);
#endif

    pixDestroy(&pixs);
    pixDestroy(&pixsc);
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

