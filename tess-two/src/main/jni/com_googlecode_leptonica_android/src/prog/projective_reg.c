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
 * projective_reg.c
 *
 */

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


int main(int    argc,
         char **argv)
{
l_int32      i;
PIX         *pixs, *pixsc, *pixb, *pixg, *pixc, *pixcs, *pix1, *pix2, *pixd;
PIXA        *pixa;
PTA         *ptas, *ptad;
static char  mainName[] = "projective_reg";

    if (argc != 1)
        return ERROR_INT(" Syntax:  projective_reg", mainName, 1);
    if ((pixs = pixRead("feyn.tif")) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);
    pixsc = pixScale(pixs, 0.5, 0.5);

#if ALL
        /* Test invertability of sampling */
    pixa = pixaCreate(0);
    for (i = 0; i < 3; i++) {
        pixb = pixAddBorder(pixsc, ADDED_BORDER_PIXELS, 0);
        MakePtas(i, &ptas, &ptad);
        pix1 = pixProjectiveSampledPta(pixb, ptad, ptas, L_BRING_IN_WHITE);
        pixSaveTiled(pix1, pixa, 1.0, 1, 20, 8);
        pix2 = pixProjectiveSampledPta(pix1, ptas, ptad, L_BRING_IN_WHITE);
        pixSaveTiled(pix2, pixa, 1.0, 0, 20, 0);
        pixd = pixRemoveBorder(pix2, ADDED_BORDER_PIXELS);
        pixXor(pixd, pixd, pixsc);
        pixSaveTiled(pixd, pixa, 1.0, 0, 20, 0);
        if (i == 0) pixWrite("/tmp/samp.png", pix1, IFF_PNG);
        pixDestroy(&pixb);
        pixDestroy(&pix1);
        pixDestroy(&pix2);
        pixDestroy(&pixd);
        ptaDestroy(&ptas);
        ptaDestroy(&ptad);
    }

    pix1 = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/proj1.png", pix1, IFF_PNG);
    pixDisplay(pix1, 100, 300);
    pixDestroy(&pix1);
    pixaDestroy(&pixa);
#endif

#if ALL
        /* Test invertability of interpolation on grayscale */
    pixa = pixaCreate(0);
    pixg = pixScaleToGray3(pixs);
    for (i = 0; i < 3; i++) {
        pixb = pixAddBorder(pixg, ADDED_BORDER_PIXELS / 2, 255);
        MakePtas(i, &ptas, &ptad);
        pix1 = pixProjectivePta(pixb, ptad, ptas, L_BRING_IN_WHITE);
        pixSaveTiled(pix1, pixa, 1.0, 1, 20, 8);
        pix2 = pixProjectivePta(pix1, ptas, ptad, L_BRING_IN_WHITE);
        pixSaveTiled(pix2, pixa, 1.0, 0, 20, 0);
        pixd = pixRemoveBorder(pix2, ADDED_BORDER_PIXELS / 2);
        pixXor(pixd, pixd, pixg);
        pixSaveTiled(pixd, pixa, 1.0, 0, 20, 0);
        if (i == 0) pixWrite("/tmp/interp.png", pix1, IFF_PNG);
        pixDestroy(&pixb);
        pixDestroy(&pix1);
        pixDestroy(&pix2);
        pixDestroy(&pixd);
        ptaDestroy(&ptas);
        ptaDestroy(&ptad);
    }

    pix1 = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/proj2.png", pix1, IFF_PNG);
    pixDisplay(pix1, 100, 500);
    pixDestroy(&pix1);
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
        pix1 = pixProjectivePta(pixb, ptad, ptas, L_BRING_IN_WHITE);
        pixSaveTiled(pix1, pixa, 1.0, 1, 20, 32);
        pix2 = pixProjectivePta(pix1, ptas, ptad, L_BRING_IN_WHITE);
        pixSaveTiled(pix2, pixa, 1.0, 0, 20, 0);
        pixd = pixRemoveBorder(pix2, ADDED_BORDER_PIXELS);
        pixXor(pixd, pixd, pixcs);
        pixSaveTiled(pixd, pixa, 1.0, 0, 20, 0);
        pixDestroy(&pixb);
        pixDestroy(&pix1);
        pixDestroy(&pix2);
        pixDestroy(&pixd);
        ptaDestroy(&ptas);
        ptaDestroy(&ptad);
    }

    pix1 = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/proj3.png", pix1, IFF_PNG);
    pixDisplay(pix1, 100, 500);
    pixDestroy(&pix1);
    pixaDestroy(&pixa);
    pixDestroy(&pixc);
    pixDestroy(&pixcs);
#endif

#if ALL
       /* Comparison between sampling and interpolated */
    MakePtas(3, &ptas, &ptad);
    pixa = pixaCreate(0);

        /* Use sampled transform */
    pix1 = pixProjectiveSampledPta(pixs, ptas, ptad, L_BRING_IN_WHITE);
    pixSaveTiled(pix1, pixa, 0.5, 1, 20, 8);

        /* Use interpolated transforms */
    pix2 = pixProjectivePta(pixs, ptas, ptad, L_BRING_IN_WHITE);
    pixSaveTiled(pix2, pixa, 0.5, 0, 20, 8);

        /* Compare the results */
    pixXor(pix2, pix2, pix1);
    pixSaveTiled(pix2, pixa, 0.5, 0, 20, 8);

    pixd = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/proj4.png", pixd, IFF_PNG);
    pixDisplay(pixd, 100, 700);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
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
    pix1 = pixProjectiveSampledPta(pixg, ptas, ptad, L_BRING_IN_WHITE);
    fprintf(stderr, " Time for pixProjectiveSampledPta(): %6.2f sec\n", stopTimer());
    pixSaveTiled(pix1, pixa, 1.0, 1, 20, 8);

    startTimer();
    pix2 = pixProjectivePta(pixg, ptas, ptad, L_BRING_IN_WHITE);
    fprintf(stderr, " Time for pixProjectivePta(): %6.2f sec\n", stopTimer());
    pixSaveTiled(pix2, pixa, 1.0, 0, 20, 8);

    pixXor(pix1, pix1, pix2);
    pixSaveTiled(pix1, pixa, 1.0, 0, 20, 8);
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pixd = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/proj5.png", pixd, IFF_PNG);
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

