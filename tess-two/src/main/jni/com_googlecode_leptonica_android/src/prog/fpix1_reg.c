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
 *  fpix1_reg.c
 *
 *    Regression test for a number of functions in the FPix utility.
 *    FPix allows you to do floating point operations such as
 *    convolution, with conversions to and from Pix.
 */

#include <math.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>   /* for Sleep() */
#endif  /* _WIN32 */
#include "allheaders.h"

static void MakePtasAffine(l_int32 i, PTA **pptas, PTA **pptad);
static void MakePtas(l_int32 i, PTA **pptas, PTA **pptad);

static const l_int32  xs1[] =  { 300,  300, 1100,  300,   32};
static const l_int32  ys1[] =  {1200, 1200, 1200, 1250,  934};
static const l_int32  xs2[] =  {1200, 1200,  325, 1300,  487};
static const l_int32  ys2[] =  {1100, 1100, 1200, 1250,  934};
static const l_int32  xs3[] =  { 200,  200, 1200,  250,   32};
static const l_int32  ys3[] =  { 200,  200,  200,  300,   67};
static const l_int32  xs4[] =  {1200, 1200, 1100, 1250,  332};
static const l_int32  ys4[] =  { 400,  200,  200,  300,   57};

static const l_int32  xd1[] = { 300,  300, 1150,  300,   32};
static const l_int32  yd1[] = {1200, 1400, 1150, 1350,  934};
static const l_int32  xd2[] = {1100, 1400,  320, 1300,  487};
static const l_int32  yd2[] = {1000, 1500, 1300, 1200,  904};
static const l_int32  xd3[] = { 250,  200, 1310,  300,   61};
static const l_int32  yd3[] = { 200,  300,  250,  325,   83};
static const l_int32  xd4[] = {1250, 1200, 1140, 1250,  412};
static const l_int32  yd4[] = { 300,  300,  250,  350,   83};


int main(int    argc,
         char **argv)
{
l_float32     sum, sumx, sumy, diff;
L_DEWARP     *dew;
L_DEWARPA    *dewa;
FPIX         *fpixs, *fpixs2, *fpixs3, *fpixs4, *fpixg, *fpixd;
FPIX         *fpix1, *fpix2, *fpixt1, *fpixt2;
DPIX         *dpix, *dpix2;
L_KERNEL     *kel, *kelx, *kely;
PIX          *pixs, *pixs2, *pixs3, *pixt, *pixd, *pixg, *pixb, *pixn;
PIX          *pixt1, *pixt2, *pixt3, *pixt4, *pixt5, *pixt6;
PIXA         *pixa;
PTA          *ptas, *ptad;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

    pixa = pixaCreate(0);

        /* Gaussian kernel */
    kel = makeGaussianKernel(5, 5, 3.0, 4.0);
    kernelGetSum(kel, &sum);
    if (rp->display) fprintf(stderr, "Sum for 2d gaussian kernel = %f\n", sum);
    pixt = kernelDisplayInPix(kel, 41, 2);
    regTestWritePixAndCheck(rp, pixt, IFF_PNG);  /* 0 */
    pixSaveTiled(pixt, pixa, 1.0, 1, 20, 8);
    pixDestroy(&pixt);

        /* Separable gaussian kernel */
    makeGaussianKernelSep(5, 5, 3.0, 4.0, &kelx, &kely);
    kernelGetSum(kelx, &sumx);
    if (rp->display) fprintf(stderr, "Sum for x gaussian kernel = %f\n", sumx);
    kernelGetSum(kely, &sumy);
    if (rp->display) fprintf(stderr, "Sum for y gaussian kernel = %f\n", sumy);
    if (rp->display) fprintf(stderr, "Sum for x * y gaussian kernel = %f\n",
                         sumx * sumy);
    pixt = kernelDisplayInPix(kelx, 41, 2);
    regTestWritePixAndCheck(rp, pixt, IFF_PNG);  /* 1 */
    pixSaveTiled(pixt, pixa, 1.0, 0, 20, 8);
    pixDestroy(&pixt);
    pixt = kernelDisplayInPix(kely, 41, 2);
    regTestWritePixAndCheck(rp, pixt, IFF_PNG);  /* 2 */
    pixSaveTiled(pixt, pixa, 1.0, 0, 20, 8);
    pixDestroy(&pixt);

        /* Use pixRasterop() to generate source image */
    pixs = pixRead("test8.jpg");
    pixs2 = pixRead("karen8.jpg");
    pixRasterop(pixs, 150, 125, 150, 100, PIX_SRC, pixs2, 75, 100);
    regTestWritePixAndCheck(rp, pixs, IFF_JFIF_JPEG);  /* 3 */

        /* Convolution directly with pix */
    pixt1 = pixConvolve(pixs, kel, 8, 1);
    regTestWritePixAndCheck(rp, pixt1, IFF_JFIF_JPEG);  /* 4 */
    pixSaveTiled(pixt1, pixa, 1.0, 1, 20, 8);
    pixt2 = pixConvolveSep(pixs, kelx, kely, 8, 1);
    regTestWritePixAndCheck(rp, pixt2, IFF_JFIF_JPEG);  /* 5 */
    pixSaveTiled(pixt2, pixa, 1.0, 0, 20, 8);

        /* Convolution indirectly with fpix, using fpixRasterop()
         * to generate the source image. */
    fpixs = pixConvertToFPix(pixs, 3);
    fpixs2 = pixConvertToFPix(pixs2, 3);
    fpixRasterop(fpixs, 150, 125, 150, 100, fpixs2, 75, 100);
    fpixt1 = fpixConvolve(fpixs, kel, 1);
    pixt3 = fpixConvertToPix(fpixt1, 8, L_CLIP_TO_ZERO, 1);
    regTestWritePixAndCheck(rp, pixt3, IFF_JFIF_JPEG);  /* 6 */
    pixSaveTiled(pixt3, pixa, 1.0, 1, 20, 8);
    fpixt2 = fpixConvolveSep(fpixs, kelx, kely, 1);
    pixt4 = fpixConvertToPix(fpixt2, 8, L_CLIP_TO_ZERO, 1);
    regTestWritePixAndCheck(rp, pixt4, IFF_JFIF_JPEG);  /* 7 */
    pixSaveTiled(pixt4, pixa, 1.0, 0, 20, 8);
    pixDestroy(&pixs2);
    fpixDestroy(&fpixs2);
    fpixDestroy(&fpixt1);
    fpixDestroy(&fpixt2);

        /* Comparison of results */
    pixCompareGray(pixt1, pixt2, L_COMPARE_ABS_DIFF, 0, NULL,
                   &diff, NULL, NULL);
    if (rp->display)
        fprintf(stderr, "Ave diff of pixConvolve and pixConvolveSep: %f\n",
                diff);
    pixCompareGray(pixt3, pixt4, L_COMPARE_ABS_DIFF, 0, NULL,
                   &diff, NULL, NULL);
    if (rp->display)
        fprintf(stderr, "Ave diff of fpixConvolve and fpixConvolveSep: %f\n",
                diff);
    pixCompareGray(pixt1, pixt3, L_COMPARE_ABS_DIFF, 0, NULL,
                   &diff, NULL, NULL);
    if (rp->display)
        fprintf(stderr, "Ave diff of pixConvolve and fpixConvolve: %f\n", diff);
    pixCompareGray(pixt2, pixt4, L_COMPARE_ABS_DIFF, GPLOT_PNG, NULL,
                   &diff, NULL, NULL);
    if (rp->display)
        fprintf(stderr, "Ave diff of pixConvolveSep and fpixConvolveSep: %f\n",
                diff);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

        /* Test arithmetic operations; add in a fraction rotated by 180 */
    pixs3 = pixRotate180(NULL, pixs);
    regTestWritePixAndCheck(rp, pixs3, IFF_JFIF_JPEG);  /* 8 */
    pixSaveTiled(pixs3, pixa, 1.0, 1, 20, 8);
    fpixs3 = pixConvertToFPix(pixs3, 3);
    fpixd = fpixLinearCombination(NULL, fpixs, fpixs3, 20.0, 5.0);
    fpixAddMultConstant(fpixd, 0.0, 23.174);   /* multiply up in magnitude */
    pixd = fpixDisplayMaxDynamicRange(fpixd);  /* bring back to 8 bpp */
    regTestWritePixAndCheck(rp, pixd, IFF_JFIF_JPEG);  /* 9 */
    pixSaveTiled(pixd, pixa, 1.0, 0, 20, 8);
    pixDestroy(&pixs3);
    fpixDestroy(&fpixs3);
    fpixDestroy(&fpixd);
    pixDestroy(&pixd);
    pixDestroy(&pixs);
    fpixDestroy(&fpixs);

        /* Save the comparison graph; gnuplot should have made it by now! */
#ifndef _WIN32
    sleep(2);
#else
    Sleep(2000);
#endif  /* _WIN32 */
    pixt5 = pixRead("/tmp/lept/compare_gray0.png");
    regTestWritePixAndCheck(rp, pixt5, IFF_PNG);  /* 10 */
    pixSaveTiled(pixt5, pixa, 1.0, 1, 20, 8);
    pixDestroy(&pixt5);

        /* Display results */
    pixd = pixaDisplay(pixa, 0, 0);
    regTestWritePixAndCheck(rp, pixd, IFF_JFIF_JPEG);  /* 11 */
    pixDisplayWithTitle(pixd, 100, 100, NULL, rp->display);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

        /* Test some more convolutions, with sampled output. First on pix */
    pixa = pixaCreate(0);
    pixs = pixRead("1555-7.jpg");
    pixg = pixConvertTo8(pixs, 0);
    l_setConvolveSampling(5, 5);
    pixt1 = pixConvolve(pixg, kel, 8, 1);
    regTestWritePixAndCheck(rp, pixt1, IFF_JFIF_JPEG);  /* 12 */
    pixSaveTiled(pixt1, pixa, 1.0, 1, 20, 32);
    pixt2 = pixConvolveSep(pixg, kelx, kely, 8, 1);
    regTestWritePixAndCheck(rp, pixt2, IFF_JFIF_JPEG);  /* 13 */
    pixSaveTiled(pixt2, pixa, 1.0, 0, 20, 32);
    pixt3 = pixConvolveRGB(pixs, kel);
    regTestWritePixAndCheck(rp, pixt3, IFF_JFIF_JPEG);  /* 14 */
    pixSaveTiled(pixt3, pixa, 1.0, 0, 20, 32);
    pixt4 = pixConvolveRGBSep(pixs, kelx, kely);
    regTestWritePixAndCheck(rp, pixt4, IFF_JFIF_JPEG);  /* 15 */
    pixSaveTiled(pixt4, pixa, 1.0, 0, 20, 32);

        /* Then on fpix */
    fpixg = pixConvertToFPix(pixg, 1);
    fpixt1 = fpixConvolve(fpixg, kel, 1);
    pixt5 = fpixConvertToPix(fpixt1, 8, L_CLIP_TO_ZERO, 0);
    regTestWritePixAndCheck(rp, pixt5, IFF_JFIF_JPEG);  /* 16 */
    pixSaveTiled(pixt5, pixa, 1.0, 1, 20, 32);
    fpixt2 = fpixConvolveSep(fpixg, kelx, kely, 1);
    pixt6 = fpixConvertToPix(fpixt2, 8, L_CLIP_TO_ZERO, 0);
    regTestWritePixAndCheck(rp, pixt6, IFF_JFIF_JPEG);  /* 17 */
    pixSaveTiled(pixt2, pixa, 1.0, 0, 20, 32);
    regTestCompareSimilarPix(rp, pixt1, pixt5, 2, 0.00, 0);  /* 18 */
    regTestCompareSimilarPix(rp, pixt2, pixt6, 2, 0.00, 0);  /* 19 */
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);
    pixDestroy(&pixt6);
    fpixDestroy(&fpixg);
    fpixDestroy(&fpixt1);
    fpixDestroy(&fpixt2);

    pixd = pixaDisplay(pixa, 0, 0);
    regTestWritePixAndCheck(rp, pixd, IFF_JFIF_JPEG);  /* 20 */
    pixDisplayWithTitle(pixd, 600, 100, NULL, rp->display);
    pixDestroy(&pixs);
    pixDestroy(&pixg);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

        /* Test extension (continued and slope).
         * First, build a smooth vertical disparity array;
         * then extend and show the contours. */
    pixs = pixRead("cat-35.jpg");
    pixn = pixBackgroundNormSimple(pixs, NULL, NULL);
    pixg = pixConvertRGBToGray(pixn, 0.5, 0.3, 0.2);
    pixb = pixThresholdToBinary(pixg, 130);
    dewa = dewarpaCreate(1, 30, 1, 15, 0);
    if ((dew = dewarpCreate(pixb, 35)) == NULL) {
        rp->success = FALSE;
        L_ERROR("dew not made; tests 21-28 skipped (failed)\n", "fpix1_reg");
        return regTestCleanup(rp);
    }
    dewarpaInsertDewarp(dewa, dew);
    dewarpBuildPageModel(dew, NULL);
    dewarpPopulateFullRes(dew, NULL, 0, 0);
    fpixs = dew->fullvdispar;
    fpixs2 = fpixAddContinuedBorder(fpixs, 200, 200, 100, 300);
    fpixs3 = fpixAddSlopeBorder(fpixs, 200, 200, 100, 300);
    dpix = fpixConvertToDPix(fpixs3);
    fpixs4 = dpixConvertToFPix(dpix);
    pixt1 = fpixRenderContours(fpixs, 2.0, 0.2);
    pixt2 = fpixRenderContours(fpixs2, 2.0, 0.2);
    pixt3 = fpixRenderContours(fpixs3, 2.0, 0.2);
    pixt4 = fpixRenderContours(fpixs4, 2.0, 0.2);
    pixt5 = pixRead("karen8.jpg");
    dpix2 = pixConvertToDPix(pixt5, 1);
    pixt6 = dpixConvertToPix(dpix2, 8, L_CLIP_TO_ZERO, 0);
    regTestWritePixAndCheck(rp, pixt1, IFF_PNG);  /* 21 */
    pixDisplayWithTitle(pixt1, 0, 100, NULL, rp->display);
    regTestWritePixAndCheck(rp, pixt2, IFF_PNG);  /* 22 */
    pixDisplayWithTitle(pixt2, 470, 100, NULL, rp->display);
    regTestWritePixAndCheck(rp, pixt3, IFF_PNG);  /* 23 */
    pixDisplayWithTitle(pixt3, 1035, 100, NULL, rp->display);
    regTestComparePix(rp, pixt3, pixt4);  /* 24 */
    regTestComparePix(rp, pixt5, pixt6);  /* 25 */
    pixDestroy(&pixs);
    pixDestroy(&pixn);
    pixDestroy(&pixg);
    pixDestroy(&pixb);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);
    pixDestroy(&pixt6);
    fpixDestroy(&fpixs2);
    fpixDestroy(&fpixs3);
    fpixDestroy(&fpixs4);
    dpixDestroy(&dpix);
    dpixDestroy(&dpix2);

        /* Test affine and projective transforms on fpix */
    fpixWrite("/tmp/regout/fpix1.fp", dew->fullvdispar);
    fpix1 = fpixRead("/tmp/regout/fpix1.fp");
    pixt1 = fpixAutoRenderContours(fpix1, 40);
    regTestWritePixAndCheck(rp, pixt1, IFF_PNG);  /* 26 */
    pixDisplayWithTitle(pixt1, 0, 500, NULL, rp->display);
    pixDestroy(&pixt1);

    MakePtasAffine(1, &ptas, &ptad);
    fpix2 = fpixAffinePta(fpix1, ptad, ptas, 200, 0.0);
    pixt2 = fpixAutoRenderContours(fpix2, 40);
    regTestWritePixAndCheck(rp, pixt2, IFF_PNG);  /* 27 */
    pixDisplayWithTitle(pixt2, 400, 500, NULL, rp->display);
    fpixDestroy(&fpix2);
    pixDestroy(&pixt2);
    ptaDestroy(&ptas);
    ptaDestroy(&ptad);

    MakePtas(1, &ptas, &ptad);
    fpix2 = fpixProjectivePta(fpix1, ptad, ptas, 200, 0.0);
    pixt3 = fpixAutoRenderContours(fpix2, 40);
    regTestWritePixAndCheck(rp, pixt3, IFF_PNG);  /* 28 */
    pixDisplayWithTitle(pixt3, 400, 500, NULL, rp->display);
    fpixDestroy(&fpix2);
    pixDestroy(&pixt3);
    ptaDestroy(&ptas);
    ptaDestroy(&ptad);
    fpixDestroy(&fpix1);
    dewarpaDestroy(&dewa);

    kernelDestroy(&kel);
    kernelDestroy(&kelx);
    kernelDestroy(&kely);
    return regTestCleanup(rp);
}


static void
MakePtas(l_int32  i,
         PTA    **pptas,
         PTA    **pptad)
{
PTA  *ptas, *ptad;
    ptas = ptaCreate(4);
    ptaAddPt(ptas, xs1[i], ys1[i]);
    ptaAddPt(ptas, xs2[i], ys2[i]);
    ptaAddPt(ptas, xs3[i], ys3[i]);
    ptaAddPt(ptas, xs4[i], ys4[i]);
    ptad = ptaCreate(4);
    ptaAddPt(ptad, xd1[i], yd1[i]);
    ptaAddPt(ptad, xd2[i], yd2[i]);
    ptaAddPt(ptad, xd3[i], yd3[i]);
    ptaAddPt(ptad, xd4[i], yd4[i]);
    *pptas = ptas;
    *pptad = ptad;
    return;
}

static void
MakePtasAffine(l_int32  i,
               PTA    **pptas,
               PTA    **pptad)
{
PTA  *ptas, *ptad;
    ptas = ptaCreate(3);
    ptaAddPt(ptas, xs1[i], ys1[i]);
    ptaAddPt(ptas, xs2[i], ys2[i]);
    ptaAddPt(ptas, xs3[i], ys3[i]);
    ptad = ptaCreate(3);
    ptaAddPt(ptad, xd1[i], yd1[i]);
    ptaAddPt(ptad, xd2[i], yd2[i]);
    ptaAddPt(ptad, xd3[i], yd3[i]);
    *pptas = ptas;
    *pptad = ptad;
    return;
}
