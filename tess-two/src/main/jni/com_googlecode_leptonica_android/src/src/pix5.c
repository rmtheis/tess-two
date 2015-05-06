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
 *  pix5.c
 *
 *    This file has these operations:
 *
 *      (1) Measurement of 1 bpp image properties
 *      (2) Extract rectangular regions
 *      (3) Clip to foreground
 *      (4) Extract pixel averages, reversals and variance along lines
 *      (5) Rank row and column transforms
 *
 *    Measurement of properties
 *           l_int32     pixaFindDimensions()
 *           l_int32     pixFindAreaPerimRatio()
 *           NUMA       *pixaFindPerimToAreaRatio()
 *           l_int32     pixFindPerimToAreaRatio()
 *           NUMA       *pixaFindPerimSizeRatio()
 *           l_int32     pixFindPerimSizeRatio()
 *           NUMA       *pixaFindAreaFraction()
 *           l_int32     pixFindAreaFraction()
 *           NUMA       *pixaFindAreaFractionMasked()
 *           l_int32     pixFindAreaFractionMasked()
 *           NUMA       *pixaFindWidthHeightRatio()
 *           NUMA       *pixaFindWidthHeightProduct()
 *           l_int32     pixFindOverlapFraction()
 *           BOXA       *pixFindRectangleComps()
 *           l_int32     pixConformsToRectangle()
 *
 *    Extract rectangular region
 *           PIXA       *pixClipRectangles()
 *           PIX        *pixClipRectangle()
 *           PIX        *pixClipMasked()
 *           l_int32     pixCropToMatch()
 *           PIX        *pixCropToSize()
 *           PIX        *pixResizeToMatch()
 *
 *    Clip to foreground
 *           PIX        *pixClipToForeground()
 *           l_int32     pixTestClipToForeground()
 *           l_int32     pixClipBoxToForeground()
 *           l_int32     pixScanForForeground()
 *           l_int32     pixClipBoxToEdges()
 *           l_int32     pixScanForEdge()
 *
 *    Extract pixel averages and reversals along lines
 *           NUMA       *pixExtractOnLine()
 *           l_float32   pixAverageOnLine()
 *           NUMA       *pixAverageIntensityProfile()
 *           NUMA       *pixReversalProfile()
 *
 *    Extract windowed variance along a line
 *           NUMA       *pixWindowedVarianceOnLine()
 *
 *    Extract min/max of pixel values near lines
 *           l_int32     pixMinMaxNearLine()
 *
 *    Rank row and column transforms
 *           PIX        *pixRankRowTransform()
 *           PIX        *pixRankColumnTransform()
 */

#include <string.h>
#include <math.h>
#include "allheaders.h"

static const l_uint32 rmask32[] = {0x0,
    0x00000001, 0x00000003, 0x00000007, 0x0000000f,
    0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
    0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
    0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
    0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
    0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
    0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff};

#ifndef  NO_CONSOLE_IO
#define  DEBUG_EDGES         0
#endif  /* ~NO_CONSOLE_IO */


/*-------------------------------------------------------------*
 *                 Measurement of properties                   *
 *-------------------------------------------------------------*/
/*!
 *  pixaFindDimensions()
 *
 *      Input:  pixa
 *              &naw (<optional return> numa of pix widths)
 *              &nah (<optional return> numa of pix heights)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixaFindDimensions(PIXA   *pixa,
                   NUMA  **pnaw,
                   NUMA  **pnah)
{
l_int32  i, n, w, h;
PIX     *pixt;

    PROCNAME("pixaFindDimensions");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    if (!pnaw && !pnah)
        return 0;

    n = pixaGetCount(pixa);
    if (pnaw) *pnaw = numaCreate(n);
    if (pnah) *pnah = numaCreate(n);
    for (i = 0; i < n; i++) {
        pixt = pixaGetPix(pixa, i, L_CLONE);
        pixGetDimensions(pixt, &w, &h, NULL);
        if (pnaw)
            numaAddNumber(*pnaw, w);
        if (pnah)
            numaAddNumber(*pnah, h);
        pixDestroy(&pixt);
    }
    return 0;
}


/*!
 *  pixFindAreaPerimRatio()
 *
 *      Input:  pixs (1 bpp)
 *              tab (<optional> pixel sum table, can be NULL)
 *              &fract (<return> area/perimeter ratio)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The area is the number of fg pixels that are not on the
 *          boundary (i.e., are not 8-connected to a bg pixel), and the
 *          perimeter is the number of fg boundary pixels.  Returns
 *          0.0 if there are no fg pixels.
 *      (2) This function is retained because clients are using it.
 */
l_int32
pixFindAreaPerimRatio(PIX        *pixs,
                      l_int32    *tab,
                      l_float32  *pfract)
{
l_int32  *tab8;
l_int32   nfg, nbound;
PIX      *pixt;

    PROCNAME("pixFindAreaPerimRatio");

    if (!pfract)
        return ERROR_INT("&fract not defined", procName, 1);
    *pfract = 0.0;
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

    if (!tab)
        tab8 = makePixelSumTab8();
    else
        tab8 = tab;

    pixt = pixErodeBrick(NULL, pixs, 3, 3);
    pixCountPixels(pixt, &nfg, tab8);
    if (nfg == 0) {
        pixDestroy(&pixt);
        if (!tab) FREE(tab8);
        return 0;
    }
    pixXor(pixt, pixt, pixs);
    pixCountPixels(pixt, &nbound, tab8);
    *pfract = (l_float32)nfg / (l_float32)nbound;
    pixDestroy(&pixt);

    if (!tab) FREE(tab8);
    return 0;
}


/*!
 *  pixaFindPerimToAreaRatio()
 *
 *      Input:  pixa (of 1 bpp pix)
 *      Return: na (of perimeter/arear ratio for each pix), or null on error
 *
 *  Notes:
 *      (1) This is typically used for a pixa consisting of
 *          1 bpp connected components.
 */
NUMA *
pixaFindPerimToAreaRatio(PIXA  *pixa)
{
l_int32    i, n;
l_int32   *tab;
l_float32  fract;
NUMA      *na;
PIX       *pixt;

    PROCNAME("pixaFindPerimToAreaRatio");

    if (!pixa)
        return (NUMA *)ERROR_PTR("pixa not defined", procName, NULL);

    n = pixaGetCount(pixa);
    na = numaCreate(n);
    tab = makePixelSumTab8();
    for (i = 0; i < n; i++) {
        pixt = pixaGetPix(pixa, i, L_CLONE);
        pixFindPerimToAreaRatio(pixt, tab, &fract);
        numaAddNumber(na, fract);
        pixDestroy(&pixt);
    }
    FREE(tab);
    return na;
}


/*!
 *  pixFindPerimToAreaRatio()
 *
 *      Input:  pixs (1 bpp)
 *              tab (<optional> pixel sum table, can be NULL)
 *              &fract (<return> perimeter/area ratio)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The perimeter is the number of fg boundary pixels, and the
 *          area is the number of fg pixels.  This returns 0.0 if
 *          there are no fg pixels.
 *      (2) Unlike pixFindAreaPerimRatio(), this uses the full set of
 *          fg pixels for the area, and the ratio is taken in the opposite
 *          order.
 *      (3) This is typically used for a single connected component.
 *          This always has a value <= 1.0, and if the average distance
 *          of a fg pixel from the nearest bg pixel is d, this has
 *          a value ~1/d.
 */
l_int32
pixFindPerimToAreaRatio(PIX        *pixs,
                        l_int32    *tab,
                        l_float32  *pfract)
{
l_int32  *tab8;
l_int32   nfg, nbound;
PIX      *pixt;

    PROCNAME("pixFindPerimToAreaRatio");

    if (!pfract)
        return ERROR_INT("&fract not defined", procName, 1);
    *pfract = 0.0;
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

    if (!tab)
        tab8 = makePixelSumTab8();
    else
        tab8 = tab;

    pixCountPixels(pixs, &nfg, tab8);
    if (nfg == 0) {
        if (!tab) FREE(tab8);
        return 0;
    }
    pixt = pixErodeBrick(NULL, pixs, 3, 3);
    pixXor(pixt, pixt, pixs);
    pixCountPixels(pixt, &nbound, tab8);
    *pfract = (l_float32)nbound / (l_float32)nfg;
    pixDestroy(&pixt);

    if (!tab) FREE(tab8);
    return 0;
}


/*!
 *  pixaFindPerimSizeRatio()
 *
 *      Input:  pixa (of 1 bpp pix)
 *      Return: na (of fg perimeter/(2*(w+h)) ratio for each pix),
 *                  or null on error
 *
 *  Notes:
 *      (1) This is typically used for a pixa consisting of
 *          1 bpp connected components.
 *      (2) This has a minimum value for a circle of pi/4; a value for
 *          a rectangle component of approx. 1.0; and a value much larger
 *          than 1.0 for a component with a highly irregular boundary.
 */
NUMA *
pixaFindPerimSizeRatio(PIXA  *pixa)
{
l_int32    i, n;
l_int32   *tab;
l_float32  ratio;
NUMA      *na;
PIX       *pixt;

    PROCNAME("pixaFindPerimSizeRatio");

    if (!pixa)
        return (NUMA *)ERROR_PTR("pixa not defined", procName, NULL);

    n = pixaGetCount(pixa);
    na = numaCreate(n);
    tab = makePixelSumTab8();
    for (i = 0; i < n; i++) {
        pixt = pixaGetPix(pixa, i, L_CLONE);
        pixFindPerimSizeRatio(pixt, tab, &ratio);
        numaAddNumber(na, ratio);
        pixDestroy(&pixt);
    }
    FREE(tab);
    return na;
}


/*!
 *  pixFindPerimSizeRatio()
 *
 *      Input:  pixs (1 bpp)
 *              tab (<optional> pixel sum table, can be NULL)
 *              &ratio (<return> perimeter/size ratio)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) We take the 'size' as twice the sum of the width and
 *          height of pixs, and the perimeter is the number of fg
 *          boundary pixels.  We use the fg pixels of the boundary
 *          because the pix may be clipped to the boundary, so an
 *          erosion is required to count all boundary pixels.
 *      (2) This has a large value for dendritic, fractal-like components
 *          with highly irregular boundaries.
 *      (3) This is typically used for a single connected component.
 *          It has a value of about 1.0 for rectangular components with
 *          relatively smooth boundaries.
 */
l_int32
pixFindPerimSizeRatio(PIX        *pixs,
                      l_int32    *tab,
                      l_float32  *pratio)
{
l_int32  *tab8;
l_int32   w, h, nbound;
PIX      *pixt;

    PROCNAME("pixFindPerimSizeRatio");

    if (!pratio)
        return ERROR_INT("&ratio not defined", procName, 1);
    *pratio = 0.0;
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

    if (!tab)
        tab8 = makePixelSumTab8();
    else
        tab8 = tab;

    pixt = pixErodeBrick(NULL, pixs, 3, 3);
    pixXor(pixt, pixt, pixs);
    pixCountPixels(pixt, &nbound, tab8);
    pixGetDimensions(pixs, &w, &h, NULL);
    *pratio = (0.5 * nbound) / (l_float32)(w + h);
    pixDestroy(&pixt);

    if (!tab) FREE(tab8);
    return 0;
}


/*!
 *  pixaFindAreaFraction()
 *
 *      Input:  pixa (of 1 bpp pix)
 *      Return: na (of area fractions for each pix), or null on error
 *
 *  Notes:
 *      (1) This is typically used for a pixa consisting of
 *          1 bpp connected components.
 */
NUMA *
pixaFindAreaFraction(PIXA  *pixa)
{
l_int32    i, n;
l_int32   *tab;
l_float32  fract;
NUMA      *na;
PIX       *pixt;

    PROCNAME("pixaFindAreaFraction");

    if (!pixa)
        return (NUMA *)ERROR_PTR("pixa not defined", procName, NULL);

    n = pixaGetCount(pixa);
    na = numaCreate(n);
    tab = makePixelSumTab8();
    for (i = 0; i < n; i++) {
        pixt = pixaGetPix(pixa, i, L_CLONE);
        pixFindAreaFraction(pixt, tab, &fract);
        numaAddNumber(na, fract);
        pixDestroy(&pixt);
    }
    FREE(tab);
    return na;
}


/*!
 *  pixFindAreaFraction()
 *
 *      Input:  pixs (1 bpp)
 *              tab (<optional> pixel sum table, can be NULL)
 *              &fract (<return> fg area/size ratio)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This finds the ratio of the number of fg pixels to the
 *          size of the pix (w * h).  It is typically used for a
 *          single connected component.
 */
l_int32
pixFindAreaFraction(PIX        *pixs,
                    l_int32    *tab,
                    l_float32  *pfract)
{
l_int32   w, h, sum;
l_int32  *tab8;

    PROCNAME("pixFindAreaFraction");

    if (!pfract)
        return ERROR_INT("&fract not defined", procName, 1);
    *pfract = 0.0;
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

    if (!tab)
        tab8 = makePixelSumTab8();
    else
        tab8 = tab;
    pixGetDimensions(pixs, &w, &h, NULL);
    pixCountPixels(pixs, &sum, tab8);
    *pfract = (l_float32)sum / (l_float32)(w * h);

    if (!tab) FREE(tab8);
    return 0;
}


/*!
 *  pixaFindAreaFractionMasked()
 *
 *      Input:  pixa (of 1 bpp pix)
 *              pixm (mask image)
 *              debug (1 for output, 0 to suppress)
 *      Return: na (of ratio masked/total fractions for each pix),
 *                  or null on error
 *
 *  Notes:
 *      (1) This is typically used for a pixa consisting of
 *          1 bpp connected components, which has an associated
 *          boxa giving the location of the components relative
 *          to the mask origin.
 *      (2) The debug flag displays in green and red the masked and
 *          unmasked parts of the image from which pixa was derived.
 */
NUMA *
pixaFindAreaFractionMasked(PIXA    *pixa,
                           PIX     *pixm,
                           l_int32  debug)
{
l_int32    i, n, full;
l_int32   *tab;
l_float32  fract;
BOX       *box;
NUMA      *na;
PIX       *pix;

    PROCNAME("pixaFindAreaFractionMasked");

    if (!pixa)
        return (NUMA *)ERROR_PTR("pixa not defined", procName, NULL);
    if (!pixm || pixGetDepth(pixm) != 1)
        return (NUMA *)ERROR_PTR("pixm undefined or not 1 bpp", procName, NULL);

    n = pixaGetCount(pixa);
    na = numaCreate(n);
    tab = makePixelSumTab8();
    pixaIsFull(pixa, NULL, &full);  /* check boxa */
    box = NULL;
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
        if (full)
            box = pixaGetBox(pixa, i, L_CLONE);
        pixFindAreaFractionMasked(pix, box, pixm, tab, &fract);
        numaAddNumber(na, fract);
        boxDestroy(&box);
        pixDestroy(&pix);
    }
    FREE(tab);

    if (debug) {
        l_int32  w, h;
        PIX     *pix1, *pix2;
        pixGetDimensions(pixm, &w, &h, NULL);
        pix1 = pixaDisplay(pixa, w, h);  /* recover original image */
        pix2 = pixCreate(w, h, 8);  /* make an 8 bpp white image ... */
        pixSetColormap(pix2, pixcmapCreate(8));  /* that's cmapped ... */
        pixSetBlackOrWhite(pix2, L_SET_WHITE);  /* and init to white */
        pixSetMaskedCmap(pix2, pix1, 0, 0, 255, 0, 0);  /* color all fg red */
        pixRasterop(pix1, 0, 0, w, h, PIX_MASK, pixm, 0, 0);
        pixSetMaskedCmap(pix2, pix1, 0, 0, 0, 255, 0);  /* turn masked green */
        pixDisplay(pix2, 100, 100);
        pixDestroy(&pix1);
        pixDestroy(&pix2);
    }

    return na;
}


/*!
 *  pixFindAreaFractionMasked()
 *
 *      Input:  pixs (1 bpp, typically a single component)
 *              box (<optional> for pixs relative to pixm)
 *              pixm (1 bpp mask, typically over the entire image from
 *                    which the component pixs was extracted)
 *              tab (<optional> pixel sum table, can be NULL)
 *              &fract (<return> fg area/size ratio)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This finds the ratio of the number of masked fg pixels
 *          in pixs to the total number of fg pixels in pixs.
 *          It is typically used for a single connected component.
 *          If there are no fg pixels, this returns a ratio of 0.0.
 *      (2) The box gives the location of the pix relative to that
 *          of the UL corner of the mask.  Therefore, the rasterop
 *          is performed with the pix translated to its location
 *          (x, y) in the mask before ANDing.
 *          If box == NULL, the UL corners of pixs and pixm are aligned.
 */
l_int32
pixFindAreaFractionMasked(PIX        *pixs,
                          BOX        *box,
                          PIX        *pixm,
                          l_int32    *tab,
                          l_float32  *pfract)
{
l_int32   x, y, w, h, sum, masksum;
l_int32  *tab8;
PIX      *pix1;

    PROCNAME("pixFindAreaFractionMasked");

    if (!pfract)
        return ERROR_INT("&fract not defined", procName, 1);
    *pfract = 0.0;
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);
    if (!pixm || pixGetDepth(pixm) != 1)
        return ERROR_INT("pixm not defined or not 1 bpp", procName, 1);

    if (!tab)
        tab8 = makePixelSumTab8();
    else
        tab8 = tab;
    x = y = 0;
    if (box)
        boxGetGeometry(box, &x, &y, NULL, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);

    pix1 = pixCopy(NULL, pixs);
    pixRasterop(pix1, 0, 0, w, h, PIX_MASK, pixm, x, y);
    pixCountPixels(pixs, &sum, tab8);
    if (sum == 0) {
        pixDestroy(&pix1);
        if (!tab) FREE(tab8);
        return 0;
    }
    pixCountPixels(pix1, &masksum, tab8);
    *pfract = (l_float32)masksum / (l_float32)sum;

    if (!tab) FREE(tab8);
    pixDestroy(&pix1);
    return 0;
}


/*!
 *  pixaFindWidthHeightRatio()
 *
 *      Input:  pixa (of 1 bpp pix)
 *      Return: na (of width/height ratios for each pix), or null on error
 *
 *  Notes:
 *      (1) This is typically used for a pixa consisting of
 *          1 bpp connected components.
 */
NUMA *
pixaFindWidthHeightRatio(PIXA  *pixa)
{
l_int32  i, n, w, h;
NUMA    *na;
PIX     *pixt;

    PROCNAME("pixaFindWidthHeightRatio");

    if (!pixa)
        return (NUMA *)ERROR_PTR("pixa not defined", procName, NULL);

    n = pixaGetCount(pixa);
    na = numaCreate(n);
    for (i = 0; i < n; i++) {
        pixt = pixaGetPix(pixa, i, L_CLONE);
        pixGetDimensions(pixt, &w, &h, NULL);
        numaAddNumber(na, (l_float32)w / (l_float32)h);
        pixDestroy(&pixt);
    }
    return na;
}


/*!
 *  pixaFindWidthHeightProduct()
 *
 *      Input:  pixa (of 1 bpp pix)
 *      Return: na (of width*height products for each pix), or null on error
 *
 *  Notes:
 *      (1) This is typically used for a pixa consisting of
 *          1 bpp connected components.
 */
NUMA *
pixaFindWidthHeightProduct(PIXA  *pixa)
{
l_int32  i, n, w, h;
NUMA    *na;
PIX     *pixt;

    PROCNAME("pixaFindWidthHeightProduct");

    if (!pixa)
        return (NUMA *)ERROR_PTR("pixa not defined", procName, NULL);

    n = pixaGetCount(pixa);
    na = numaCreate(n);
    for (i = 0; i < n; i++) {
        pixt = pixaGetPix(pixa, i, L_CLONE);
        pixGetDimensions(pixt, &w, &h, NULL);
        numaAddNumber(na, w * h);
        pixDestroy(&pixt);
    }
    return na;
}


/*!
 *  pixFindOverlapFraction()
 *
 *      Input:  pixs1, pixs2 (1 bpp)
 *              x2, y2 (location in pixs1 of UL corner of pixs2)
 *              tab (<optional> pixel sum table, can be null)
 *              &ratio (<return> ratio fg intersection to fg union)
 *              &noverlap (<optional return> number of overlapping pixels)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The UL corner of pixs2 is placed at (x2, y2) in pixs1.
 *      (2) This measure is similar to the correlation.
 */
l_int32
pixFindOverlapFraction(PIX        *pixs1,
                       PIX        *pixs2,
                       l_int32     x2,
                       l_int32     y2,
                       l_int32    *tab,
                       l_float32  *pratio,
                       l_int32    *pnoverlap)
{
l_int32  *tab8;
l_int32   w, h, nintersect, nunion;
PIX      *pixt;

    PROCNAME("pixFindOverlapFraction");

    if (!pratio)
        return ERROR_INT("&ratio not defined", procName, 1);
    *pratio = 0.0;
    if (!pixs1 || pixGetDepth(pixs1) != 1)
        return ERROR_INT("pixs1 not defined or not 1 bpp", procName, 1);
    if (!pixs2 || pixGetDepth(pixs2) != 1)
        return ERROR_INT("pixs2 not defined or not 1 bpp", procName, 1);

    if (!tab)
        tab8 = makePixelSumTab8();
    else
        tab8 = tab;

    pixGetDimensions(pixs2, &w, &h, NULL);
    pixt = pixCopy(NULL, pixs1);
    pixRasterop(pixt, x2, y2, w, h, PIX_MASK, pixs2, 0, 0);  /* AND */
    pixCountPixels(pixt, &nintersect, tab8);
    if (pnoverlap)
        *pnoverlap = nintersect;
    pixCopy(pixt, pixs1);
    pixRasterop(pixt, x2, y2, w, h, PIX_PAINT, pixs2, 0, 0);  /* OR */
    pixCountPixels(pixt, &nunion, tab8);
    if (!tab) FREE(tab8);
    pixDestroy(&pixt);

    if (nunion > 0)
        *pratio = (l_float32)nintersect / (l_float32)nunion;
    return 0;
}


/*!
 *  pixFindRectangleComps()
 *
 *      Input:  pixs (1 bpp)
 *              dist (max distance allowed between bounding box and nearest
 *                    foreground pixel within it)
 *              minw, minh (minimum size in each direction as a requirement
 *                          for a conforming rectangle)
 *      Return: boxa (of components that conform), or null on error
 *
 *  Notes:
 *      (1) This applies the function pixConformsToRectangle() to
 *          each 8-c.c. in pixs, and returns a boxa containing the
 *          regions of all components that are conforming.
 *      (2) Conforming components must satisfy both the size constraint
 *          given by @minsize and the slop in conforming to a rectangle
 *          determined by @dist.
 */
BOXA *
pixFindRectangleComps(PIX     *pixs,
                      l_int32  dist,
                      l_int32  minw,
                      l_int32  minh)
{
l_int32  w, h, i, n, conforms;
BOX     *box;
BOXA    *boxa, *boxad;
PIX     *pix;
PIXA    *pixa;

    PROCNAME("pixFindRectangleComps");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (BOXA *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (dist < 0)
        return (BOXA *)ERROR_PTR("dist must be >= 0", procName, NULL);
    if (minw <= 2 * dist && minh <= 2 * dist)
        return (BOXA *)ERROR_PTR("invalid parameters", procName, NULL);

    boxa = pixConnComp(pixs, &pixa, 8);
    boxad = boxaCreate(0);
    n = pixaGetCount(pixa);
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
        pixGetDimensions(pix, &w, &h, NULL);
        if (w < minw || h < minh) {
            pixDestroy(&pix);
            continue;
        }
        pixConformsToRectangle(pix, NULL, dist, &conforms);
        if (conforms) {
            box = boxaGetBox(boxa, i, L_COPY);
            boxaAddBox(boxad, box, L_INSERT);
        }
        pixDestroy(&pix);
    }
    boxaDestroy(&boxa);
    pixaDestroy(&pixa);
    return boxad;
}


/*!
 *  pixConformsToRectangle()
 *
 *      Input:  pixs (1 bpp)
 *              box (<optional> if null, use the entire pixs)
 *              dist (max distance allowed between bounding box and nearest
 *                    foreground pixel within it)
 *              &conforms (<return> 0 (false) if not conforming;
 *                        1 (true) if conforming)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) There are several ways to test if a connected component has
 *          an essentially rectangular boundary, such as:
 *           a. Fraction of fill into the bounding box
 *           b. Max-min distance of fg pixel from periphery of bounding box
 *           c. Max depth of bg intrusions into component within bounding box
 *          The weakness of (a) is that it is highly sensitive to holes
 *          within the c.c.  The weakness of (b) is that it can have
 *          arbitrarily large intrusions into the c.c.  Method (c) tests
 *          the integrity of the outer boundary of the c.c., with respect
 *          to the enclosing bounding box, so we use it.
 *      (2) This tests if the connected component within the box conforms
 *          to the box at all points on the periphery within @dist.
 *          Inside, at a distance from the box boundary that is greater
 *          than @dist, we don't care about the pixels in the c.c.
 *      (3) We can think of the conforming condition as follows:
 *          No pixel inside a distance @dist from the boundary
 *          can connect to the boundary through a path through the bg.
 *          To implement this, we need to do a flood fill.  We can go
 *          either from inside toward the boundary, or the other direction.
 *          It's easiest to fill from the boundary, and then verify that
 *          there are no filled pixels farther than @dist from the boundary.
 */
l_int32
pixConformsToRectangle(PIX      *pixs,
                       BOX      *box,
                       l_int32   dist,
                       l_int32  *pconforms)
{
l_int32  w, h, empty;
PIX     *pix1, *pix2;

    PROCNAME("pixConformsToRectangle");

    if (!pconforms)
        return ERROR_INT("&conforms not defined", procName, 1);
    *pconforms = 0;
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);
    if (dist < 0)
        return ERROR_INT("dist must be >= 0", procName, 1);
    pixGetDimensions(pixs, &w, &h, NULL);
    if (w <= 2 * dist || h <= 2 * dist) {
        L_WARNING("automatic conformation: distance too large\n", procName);
        *pconforms = 1;
        return 0;
    }

        /* Extract the region, if necessary */
    if (box)
        pix1 = pixClipRectangle(pixs, box, NULL);
    else
        pix1 = pixCopy(NULL, pixs);

        /* Invert and fill from the boundary into the interior.
         * Because we're considering the connected component in an
         * 8-connected sense, we do the background filling as 4 c.c. */
    pixInvert(pix1, pix1);
    pix2 = pixExtractBorderConnComps(pix1, 4);

        /* Mask out all pixels within a distance @dist from the box
         * boundary.  Any remaining pixels are from filling that goes
         * more than @dist from the boundary.  If no pixels remain,
         * the component conforms to the bounding rectangle within
         * a distance @dist. */
    pixSetOrClearBorder(pix2, dist, dist, dist, dist, PIX_CLR);
    pixZero(pix2, &empty);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    *pconforms = (empty) ? 1 : 0;
    return 0;
}


/*-----------------------------------------------------------------------*
 *                      Extract rectangular region                       *
 *-----------------------------------------------------------------------*/
/*!
 *  pixClipRectangles()
 *
 *      Input:  pixs
 *              boxa (requested clipping regions)
 *      Return: pixa (consisting of requested regions), or null on error
 *
 *  Notes:
 *     (1) The returned pixa includes the actual regions clipped out from
 *         the input pixs.
 */
PIXA *
pixClipRectangles(PIX   *pixs,
                  BOXA  *boxa)
{
l_int32  i, n;
BOX     *box, *boxc;
PIX     *pix;
PIXA    *pixa;

    PROCNAME("pixClipRectangles");

    if (!pixs)
        return (PIXA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!boxa)
        return (PIXA *)ERROR_PTR("boxa not defined", procName, NULL);

    n = boxaGetCount(boxa);
    pixa = pixaCreate(n);
    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        pix = pixClipRectangle(pixs, box, &boxc);
        pixaAddPix(pixa, pix, L_INSERT);
        pixaAddBox(pixa, boxc, L_INSERT);
        boxDestroy(&box);
    }

    return pixa;
}


/*!
 *  pixClipRectangle()
 *
 *      Input:  pixs
 *              box  (requested clipping region; const)
 *              &boxc (<optional return> actual box of clipped region)
 *      Return: clipped pix, or null on error or if rectangle
 *              doesn't intersect pixs
 *
 *  Notes:
 *
 *  This should be simple, but there are choices to be made.
 *  The box is defined relative to the pix coordinates.  However,
 *  if the box is not contained within the pix, we have two choices:
 *
 *      (1) clip the box to the pix
 *      (2) make a new pix equal to the full box dimensions,
 *          but let rasterop do the clipping and positioning
 *          of the src with respect to the dest
 *
 *  Choice (2) immediately brings up the problem of what pixel values
 *  to use that were not taken from the src.  For example, on a grayscale
 *  image, do you want the pixels not taken from the src to be black
 *  or white or something else?  To implement choice 2, one needs to
 *  specify the color of these extra pixels.
 *
 *  So we adopt (1), and clip the box first, if necessary,
 *  before making the dest pix and doing the rasterop.  But there
 *  is another issue to consider.  If you want to paste the
 *  clipped pix back into pixs, it must be properly aligned, and
 *  it is necessary to use the clipped box for alignment.
 *  Accordingly, this function has a third (optional) argument, which is
 *  the input box clipped to the src pix.
 */
PIX *
pixClipRectangle(PIX   *pixs,
                 BOX   *box,
                 BOX  **pboxc)
{
l_int32  w, h, d, bx, by, bw, bh;
BOX     *boxc;
PIX     *pixd;

    PROCNAME("pixClipRectangle");

    if (pboxc)
        *pboxc = NULL;
    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!box)
        return (PIX *)ERROR_PTR("box not defined", procName, NULL);

        /* Clip the input box to the pix */
    pixGetDimensions(pixs, &w, &h, &d);
    if ((boxc = boxClipToRectangle(box, w, h)) == NULL) {
        L_WARNING("box doesn't overlap pix\n", procName);
        return NULL;
    }
    boxGetGeometry(boxc, &bx, &by, &bw, &bh);

        /* Extract the block */
    if ((pixd = pixCreate(bw, bh, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixCopyColormap(pixd, pixs);
    pixRasterop(pixd, 0, 0, bw, bh, PIX_SRC, pixs, bx, by);

    if (pboxc)
        *pboxc = boxc;
    else
        boxDestroy(&boxc);

    return pixd;
}


/*!
 *  pixClipMasked()
 *
 *      Input:  pixs (1, 2, 4, 8, 16, 32 bpp; colormap ok)
 *              pixm  (clipping mask, 1 bpp)
 *              x, y (origin of clipping mask relative to pixs)
 *              outval (val to use for pixels that are outside the mask)
 *      Return: pixd, (clipped pix) or null on error or if pixm doesn't
 *              intersect pixs
 *
 *  Notes:
 *      (1) If pixs has a colormap, it is preserved in pixd.
 *      (2) The depth of pixd is the same as that of pixs.
 *      (3) If the depth of pixs is 1, use @outval = 0 for white background
 *          and 1 for black; otherwise, use the max value for white
 *          and 0 for black.  If pixs has a colormap, the max value for
 *          @outval is 0xffffffff; otherwise, it is 2^d - 1.
 *      (4) When using 1 bpp pixs, this is a simple clip and
 *          blend operation.  For example, if both pix1 and pix2 are
 *          black text on white background, and you want to OR the
 *          fg on the two images, let pixm be the inverse of pix2.
 *          Then the operation takes all of pix1 that's in the bg of
 *          pix2, and for the remainder (which are the pixels
 *          corresponding to the fg of the pix2), paint them black
 *          (1) in pix1.  The function call looks like
 *             pixClipMasked(pix2, pixInvert(pix1, pix1), x, y, 1);
 */
PIX *
pixClipMasked(PIX      *pixs,
              PIX      *pixm,
              l_int32   x,
              l_int32   y,
              l_uint32  outval)
{
l_int32   wm, hm, index, rval, gval, bval;
l_uint32  pixel;
BOX      *box;
PIX      *pixmi, *pixd;
PIXCMAP  *cmap;

    PROCNAME("pixClipMasked");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!pixm || pixGetDepth(pixm) != 1)
        return (PIX *)ERROR_PTR("pixm undefined or not 1 bpp", procName, NULL);

        /* Clip out the region specified by pixm and (x,y) */
    pixGetDimensions(pixm, &wm, &hm, NULL);
    box = boxCreate(x, y, wm, hm);
    pixd = pixClipRectangle(pixs, box, NULL);

        /* Paint 'outval' (or something close to it if cmapped) through
         * the pixels not masked by pixm */
    cmap = pixGetColormap(pixd);
    pixmi = pixInvert(NULL, pixm);
    if (cmap) {
        extractRGBValues(outval, &rval, &gval, &bval);
        pixcmapGetNearestIndex(cmap, rval, gval, bval, &index);
        pixcmapGetColor(cmap, index, &rval, &gval, &bval);
        composeRGBPixel(rval, gval, bval, &pixel);
        pixPaintThroughMask(pixd, pixmi, 0, 0, pixel);
    } else {
        pixPaintThroughMask(pixd, pixmi, 0, 0, outval);
    }

    boxDestroy(&box);
    pixDestroy(&pixmi);
    return pixd;
}


/*!
 *  pixCropToMatch()
 *
 *      Input:  pixs1 (any depth, colormap OK)
 *              pixs2 (any depth, colormap OK)
 *              &pixd1 (<return> may be a clone)
 *              &pixd2 (<return> may be a clone)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This resizes pixs1 and/or pixs2 by cropping at the right
 *          and bottom, so that they're the same size.
 *      (2) If a pix doesn't need to be cropped, a clone is returned.
 *      (3) Note: the images are implicitly aligned to the UL corner.
 */
l_int32
pixCropToMatch(PIX   *pixs1,
               PIX   *pixs2,
               PIX  **ppixd1,
               PIX  **ppixd2)
{
l_int32  w1, h1, w2, h2, w, h;

    PROCNAME("pixCropToMatch");

    if (!ppixd1 || !ppixd2)
        return ERROR_INT("&pixd1 and &pixd2 not both defined", procName, 1);
    *ppixd1 = *ppixd2 = NULL;
    if (!pixs1 || !pixs2)
        return ERROR_INT("pixs1 and pixs2 not defined", procName, 1);

    pixGetDimensions(pixs1, &w1, &h1, NULL);
    pixGetDimensions(pixs2, &w2, &h2, NULL);
    w = L_MIN(w1, w2);
    h = L_MIN(h1, h2);

    *ppixd1 = pixCropToSize(pixs1, w, h);
    *ppixd2 = pixCropToSize(pixs2, w, h);
    if (*ppixd1 == NULL || *ppixd2 == NULL)
        return ERROR_INT("cropped image failure", procName, 1);
    return 0;
}


/*!
 *  pixCropToSize()
 *
 *      Input:  pixs (any depth, colormap OK)
 *              w, h (max dimensions of cropped image)
 *      Return: pixd (cropped if necessary) or null on error.
 *
 *  Notes:
 *      (1) If either w or h is smaller than the corresponding dimension
 *          of pixs, this returns a cropped image; otherwise it returns
 *          a clone of pixs.
 */
PIX *
pixCropToSize(PIX     *pixs,
              l_int32  w,
              l_int32  h)
{
l_int32  ws, hs, wd, hd, d;
PIX     *pixd;

    PROCNAME("pixCropToSize");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, &d);
    if (ws <= w && hs <= h)  /* no cropping necessary */
        return pixClone(pixs);

    wd = L_MIN(ws, w);
    hd = L_MIN(hs, h);
    if ((pixd = pixCreate(wd, hd, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixCopyColormap(pixd, pixs);
    pixCopyText(pixd, pixs);
    pixCopyInputFormat(pixd, pixs);
    pixRasterop(pixd, 0, 0, wd, hd, PIX_SRC, pixs, 0, 0);
    return pixd;
}


/*!
 *  pixResizeToMatch()
 *
 *      Input:  pixs (1, 2, 4, 8, 16, 32 bpp; colormap ok)
 *              pixt  (can be null; we use only the size)
 *              w, h (ignored if pixt is defined)
 *      Return: pixd (resized to match) or null on error
 *
 *  Notes:
 *      (1) This resizes pixs to make pixd, without scaling, by either
 *          cropping or extending separately in both width and height.
 *          Extension is done by replicating the last row or column.
 *          This is useful in a situation where, due to scaling
 *          operations, two images that are expected to be the
 *          same size can differ slightly in each dimension.
 *      (2) You can use either an existing pixt or specify
 *          both @w and @h.  If pixt is defined, the values
 *          in @w and @h are ignored.
 *      (3) If pixt is larger than pixs (or if w and/or d is larger
 *          than the dimension of pixs, replicate the outer row and
 *          column of pixels in pixs into pixd.
 */
PIX *
pixResizeToMatch(PIX     *pixs,
                 PIX     *pixt,
                 l_int32  w,
                 l_int32  h)
{
l_int32  i, j, ws, hs, d;
PIX     *pixd;

    PROCNAME("pixResizeToMatch");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!pixt && (w <= 0 || h <= 0))
        return (PIX *)ERROR_PTR("both w and h not > 0", procName, NULL);

    if (pixt)  /* redefine w, h */
        pixGetDimensions(pixt, &w, &h, NULL);
    pixGetDimensions(pixs, &ws, &hs, &d);
    if (ws == w && hs == h)
        return pixCopy(NULL, pixs);

    if ((pixd = pixCreate(w, h, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixCopyColormap(pixd, pixs);
    pixCopyText(pixd, pixs);
    pixCopyInputFormat(pixd, pixs);
    pixRasterop(pixd, 0, 0, ws, hs, PIX_SRC, pixs, 0, 0);
    if (ws >= w && hs >= h)
        return pixd;

        /* Replicate the last column and then the last row */
    if (ws < w) {
        for (j = ws; j < w; j++)
            pixRasterop(pixd, j, 0, 1, h, PIX_SRC, pixd, ws - 1, 0);
    }
    if (hs < h) {
        for (i = hs; i < h; i++)
            pixRasterop(pixd, 0, i, w, 1, PIX_SRC, pixd, 0, hs - 1);
    }

    return pixd;
}


/*---------------------------------------------------------------------*
 *                           Clip to Foreground                        *
 *---------------------------------------------------------------------*/
/*!
 *  pixClipToForeground()
 *
 *      Input:  pixs (1 bpp)
 *              &pixd  (<optional return> clipped pix returned)
 *              &box   (<optional return> bounding box)
 *      Return: 0 if OK; 1 on error or if there are no fg pixels
 *
 *  Notes:
 *      (1) At least one of {&pixd, &box} must be specified.
 *      (2) If there are no fg pixels, the returned ptrs are null.
 */
l_int32
pixClipToForeground(PIX   *pixs,
                    PIX  **ppixd,
                    BOX  **pbox)
{
l_int32    w, h, wpl, nfullwords, extra, i, j;
l_int32    minx, miny, maxx, maxy;
l_uint32   result, mask;
l_uint32  *data, *line;
BOX       *box;

    PROCNAME("pixClipToForeground");

    if (!ppixd && !pbox)
        return ERROR_INT("neither &pixd nor &box defined", procName, 1);
    if (ppixd)
        *ppixd = NULL;
    if (pbox)
        *pbox = NULL;
    if (!pixs || (pixGetDepth(pixs) != 1))
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

    pixGetDimensions(pixs, &w, &h, NULL);
    nfullwords = w / 32;
    extra = w & 31;
    mask = ~rmask32[32 - extra];
    wpl = pixGetWpl(pixs);
    data = pixGetData(pixs);

    result = 0;
    for (i = 0, miny = 0; i < h; i++, miny++) {
        line = data + i * wpl;
        for (j = 0; j < nfullwords; j++)
            result |= line[j];
        if (extra)
            result |= (line[j] & mask);
        if (result)
            break;
    }
    if (miny == h)  /* no ON pixels */
        return 1;

    result = 0;
    for (i = h - 1, maxy = h - 1; i >= 0; i--, maxy--) {
        line = data + i * wpl;
        for (j = 0; j < nfullwords; j++)
            result |= line[j];
        if (extra)
            result |= (line[j] & mask);
        if (result)
            break;
    }

    minx = 0;
    for (j = 0, minx = 0; j < w; j++, minx++) {
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            if (GET_DATA_BIT(line, j))
                goto minx_found;
        }
    }

minx_found:
    for (j = w - 1, maxx = w - 1; j >= 0; j--, maxx--) {
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            if (GET_DATA_BIT(line, j))
                goto maxx_found;
        }
    }

maxx_found:
    box = boxCreate(minx, miny, maxx - minx + 1, maxy - miny + 1);

    if (ppixd)
        *ppixd = pixClipRectangle(pixs, box, NULL);
    if (pbox)
        *pbox = box;
    else
        boxDestroy(&box);

    return 0;
}


/*!
 *  pixTestClipToForeground()
 *
 *      Input:  pixs (1 bpp)
 *              &canclip (<return> 1 if fg does not extend to all four edges)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This is a lightweight test to determine if a 1 bpp image
 *          can be further cropped without loss of fg pixels.
 *          If it cannot, canclip is set to 0.
 */
l_int32
pixTestClipToForeground(PIX      *pixs,
                        l_int32  *pcanclip)
{
l_int32    i, j, w, h, wpl, found;
l_uint32  *data, *line;

    PROCNAME("pixTestClipToForeground");

    if (!pcanclip)
        return ERROR_INT("&canclip not defined", procName, 1);
    *pcanclip = 0;
    if (!pixs || (pixGetDepth(pixs) != 1))
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

        /* Check top and bottom raster lines */
    pixGetDimensions(pixs, &w, &h, NULL);
    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    found = FALSE;
    for (j = 0; found == FALSE && j < w; j++)
        found = GET_DATA_BIT(data, j);
    if (!found) {
        *pcanclip = 1;
        return 0;
    }

    line = data + (h - 1) * wpl;
    found = FALSE;
    for (j = 0; found == FALSE && j < w; j++)
        found = GET_DATA_BIT(data, j);
    if (!found) {
        *pcanclip = 1;
        return 0;
    }

        /* Check left and right edges */
    found = FALSE;
    for (i = 0, line = data; found == FALSE && i < h; line += wpl, i++)
        found = GET_DATA_BIT(line, 0);
    if (!found) {
        *pcanclip = 1;
        return 0;
    }

    found = FALSE;
    for (i = 0, line = data; found == FALSE && i < h; line += wpl, i++)
        found = GET_DATA_BIT(line, w - 1);
    if (!found)
        *pcanclip = 1;

    return 0;  /* fg pixels found on all edges */
}


/*!
 *  pixClipBoxToForeground()
 *
 *      Input:  pixs (1 bpp)
 *              boxs  (<optional> ; use full image if null)
 *              &pixd  (<optional return> clipped pix returned)
 *              &boxd  (<optional return> bounding box)
 *      Return: 0 if OK; 1 on error or if there are no fg pixels
 *
 *  Notes:
 *      (1) At least one of {&pixd, &boxd} must be specified.
 *      (2) If there are no fg pixels, the returned ptrs are null.
 *      (3) Do not use &pixs for the 3rd arg or &boxs for the 4th arg;
 *          this will leak memory.
 */
l_int32
pixClipBoxToForeground(PIX   *pixs,
                       BOX   *boxs,
                       PIX  **ppixd,
                       BOX  **pboxd)
{
l_int32  w, h, bx, by, bw, bh, cbw, cbh, left, right, top, bottom;
BOX     *boxt, *boxd;

    PROCNAME("pixClipBoxToForeground");

    if (!ppixd && !pboxd)
        return ERROR_INT("neither &pixd nor &boxd defined", procName, 1);
    if (ppixd) *ppixd = NULL;
    if (pboxd) *pboxd = NULL;
    if (!pixs || (pixGetDepth(pixs) != 1))
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

    if (!boxs)
        return pixClipToForeground(pixs, ppixd, pboxd);

    pixGetDimensions(pixs, &w, &h, NULL);
    boxGetGeometry(boxs, &bx, &by, &bw, &bh);
    cbw = L_MIN(bw, w - bx);
    cbh = L_MIN(bh, h - by);
    if (cbw < 0 || cbh < 0)
        return ERROR_INT("box not within image", procName, 1);
    boxt = boxCreate(bx, by, cbw, cbh);

    if (pixScanForForeground(pixs, boxt, L_FROM_LEFT, &left)) {
        boxDestroy(&boxt);
        return 1;
    }
    pixScanForForeground(pixs, boxt, L_FROM_RIGHT, &right);
    pixScanForForeground(pixs, boxt, L_FROM_TOP, &top);
    pixScanForForeground(pixs, boxt, L_FROM_BOT, &bottom);

    boxd = boxCreate(left, top, right - left + 1, bottom - top + 1);
    if (ppixd)
        *ppixd = pixClipRectangle(pixs, boxd, NULL);
    if (pboxd)
        *pboxd = boxd;
    else
        boxDestroy(&boxd);

    boxDestroy(&boxt);
    return 0;
}


/*!
 *  pixScanForForeground()
 *
 *      Input:  pixs (1 bpp)
 *              box  (<optional> within which the search is conducted)
 *              scanflag (direction of scan; e.g., L_FROM_LEFT)
 *              &loc (location in scan direction of first black pixel)
 *      Return: 0 if OK; 1 on error or if no fg pixels are found
 *
 *  Notes:
 *      (1) If there are no fg pixels, the position is set to 0.
 *          Caller must check the return value!
 *      (2) Use @box == NULL to scan from edge of pixs
 */
l_int32
pixScanForForeground(PIX      *pixs,
                     BOX      *box,
                     l_int32   scanflag,
                     l_int32  *ploc)
{
l_int32    bx, by, bw, bh, x, xstart, xend, y, ystart, yend, wpl;
l_uint32  *data, *line;
BOX       *boxt;

    PROCNAME("pixScanForForeground");

    if (!ploc)
        return ERROR_INT("&loc not defined", procName, 1);
    *ploc = 0;
    if (!pixs || (pixGetDepth(pixs) != 1))
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

        /* Clip box to pixs if it exists */
    pixGetDimensions(pixs, &bw, &bh, NULL);
    if (box) {
        if ((boxt = boxClipToRectangle(box, bw, bh)) == NULL)
            return ERROR_INT("invalid box", procName, 1);
        boxGetGeometry(boxt, &bx, &by, &bw, &bh);
        boxDestroy(&boxt);
    } else {
        bx = by = 0;
    }
    xstart = bx;
    ystart = by;
    xend = bx + bw - 1;
    yend = by + bh - 1;

    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    if (scanflag == L_FROM_LEFT) {
        for (x = xstart; x <= xend; x++) {
            for (y = ystart; y <= yend; y++) {
                line = data + y * wpl;
                if (GET_DATA_BIT(line, x)) {
                    *ploc = x;
                    return 0;
                }
            }
        }
    } else if (scanflag == L_FROM_RIGHT) {
        for (x = xend; x >= xstart; x--) {
            for (y = ystart; y <= yend; y++) {
                line = data + y * wpl;
                if (GET_DATA_BIT(line, x)) {
                    *ploc = x;
                    return 0;
                }
            }
        }
    } else if (scanflag == L_FROM_TOP) {
        for (y = ystart; y <= yend; y++) {
            line = data + y * wpl;
            for (x = xstart; x <= xend; x++) {
                if (GET_DATA_BIT(line, x)) {
                    *ploc = y;
                    return 0;
                }
            }
        }
    } else if (scanflag == L_FROM_BOT) {
        for (y = yend; y >= ystart; y--) {
            line = data + y * wpl;
            for (x = xstart; x <= xend; x++) {
                if (GET_DATA_BIT(line, x)) {
                    *ploc = y;
                    return 0;
                }
            }
        }
    } else {
        return ERROR_INT("invalid scanflag", procName, 1);
    }

    return 1;  /* no fg found */
}


/*!
 *  pixClipBoxToEdges()
 *
 *      Input:  pixs (1 bpp)
 *              boxs  (<optional> ; use full image if null)
 *              lowthresh (threshold to choose clipping location)
 *              highthresh (threshold required to find an edge)
 *              maxwidth (max allowed width between low and high thresh locs)
 *              factor (sampling factor along pixel counting direction)
 *              &pixd  (<optional return> clipped pix returned)
 *              &boxd  (<optional return> bounding box)
 *      Return: 0 if OK; 1 on error or if a fg edge is not found from
 *              all four sides.
 *
 *  Notes:
 *      (1) At least one of {&pixd, &boxd} must be specified.
 *      (2) If there are no fg pixels, the returned ptrs are null.
 *      (3) This function attempts to locate rectangular "image" regions
 *          of high-density fg pixels, that have well-defined edges
 *          on the four sides.
 *      (4) Edges are searched for on each side, iterating in order
 *          from left, right, top and bottom.  As each new edge is
 *          found, the search box is resized to use that location.
 *          Once an edge is found, it is held.  If no more edges
 *          are found in one iteration, the search fails.
 *      (5) See pixScanForEdge() for usage of the thresholds and @maxwidth.
 *      (6) The thresholds must be at least 1, and the low threshold
 *          cannot be larger than the high threshold.
 *      (7) If the low and high thresholds are both 1, this is equivalent
 *          to pixClipBoxToForeground().
 */
l_int32
pixClipBoxToEdges(PIX     *pixs,
                  BOX     *boxs,
                  l_int32  lowthresh,
                  l_int32  highthresh,
                  l_int32  maxwidth,
                  l_int32  factor,
                  PIX    **ppixd,
                  BOX    **pboxd)
{
l_int32  w, h, bx, by, bw, bh, cbw, cbh, left, right, top, bottom;
l_int32  lfound, rfound, tfound, bfound, change;
BOX     *boxt, *boxd;

    PROCNAME("pixClipBoxToEdges");

    if (!ppixd && !pboxd)
        return ERROR_INT("neither &pixd nor &boxd defined", procName, 1);
    if (ppixd) *ppixd = NULL;
    if (pboxd) *pboxd = NULL;
    if (!pixs || (pixGetDepth(pixs) != 1))
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);
    if (lowthresh < 1 || highthresh < 1 ||
        lowthresh > highthresh || maxwidth < 1)
        return ERROR_INT("invalid thresholds", procName, 1);
    factor = L_MIN(1, factor);

    if (lowthresh == 1 && highthresh == 1)
        return pixClipBoxToForeground(pixs, boxs, ppixd, pboxd);

    pixGetDimensions(pixs, &w, &h, NULL);
    if (boxs) {
        boxGetGeometry(boxs, &bx, &by, &bw, &bh);
        cbw = L_MIN(bw, w - bx);
        cbh = L_MIN(bh, h - by);
        if (cbw < 0 || cbh < 0)
            return ERROR_INT("box not within image", procName, 1);
        boxt = boxCreate(bx, by, cbw, cbh);
    } else {
        boxt = boxCreate(0, 0, w, h);
    }

    lfound = rfound = tfound = bfound = 0;
    while (!lfound || !rfound || !tfound || !bfound) {
        change = 0;
        if (!lfound) {
            if (!pixScanForEdge(pixs, boxt, lowthresh, highthresh, maxwidth,
                                factor, L_FROM_LEFT, &left)) {
                lfound = 1;
                change = 1;
                boxRelocateOneSide(boxt, boxt, left, L_FROM_LEFT);
            }
        }
        if (!rfound) {
            if (!pixScanForEdge(pixs, boxt, lowthresh, highthresh, maxwidth,
                                factor, L_FROM_RIGHT, &right)) {
                rfound = 1;
                change = 1;
                boxRelocateOneSide(boxt, boxt, right, L_FROM_RIGHT);
            }
        }
        if (!tfound) {
            if (!pixScanForEdge(pixs, boxt, lowthresh, highthresh, maxwidth,
                                factor, L_FROM_TOP, &top)) {
                tfound = 1;
                change = 1;
                boxRelocateOneSide(boxt, boxt, top, L_FROM_TOP);
            }
        }
        if (!bfound) {
            if (!pixScanForEdge(pixs, boxt, lowthresh, highthresh, maxwidth,
                                factor, L_FROM_BOT, &bottom)) {
                bfound = 1;
                change = 1;
                boxRelocateOneSide(boxt, boxt, bottom, L_FROM_BOT);
            }
        }

#if DEBUG_EDGES
        fprintf(stderr, "iter: %d %d %d %d\n", lfound, rfound, tfound, bfound);
#endif  /* DEBUG_EDGES */

        if (change == 0) break;
    }
    boxDestroy(&boxt);

    if (change == 0)
        return ERROR_INT("not all edges found", procName, 1);

    boxd = boxCreate(left, top, right - left + 1, bottom - top + 1);
    if (ppixd)
        *ppixd = pixClipRectangle(pixs, boxd, NULL);
    if (pboxd)
        *pboxd = boxd;
    else
        boxDestroy(&boxd);

    return 0;
}


/*!
 *  pixScanForEdge()
 *
 *      Input:  pixs (1 bpp)
 *              box  (<optional> within which the search is conducted)
 *              lowthresh (threshold to choose clipping location)
 *              highthresh (threshold required to find an edge)
 *              maxwidth (max allowed width between low and high thresh locs)
 *              factor (sampling factor along pixel counting direction)
 *              scanflag (direction of scan; e.g., L_FROM_LEFT)
 *              &loc (location in scan direction of first black pixel)
 *      Return: 0 if OK; 1 on error or if the edge is not found
 *
 *  Notes:
 *      (1) If there are no fg pixels, the position is set to 0.
 *          Caller must check the return value!
 *      (2) Use @box == NULL to scan from edge of pixs
 *      (3) As the scan progresses, the location where the sum of
 *          pixels equals or excees @lowthresh is noted (loc).  The
 *          scan is stopped when the sum of pixels equals or exceeds
 *          @highthresh.  If the scan distance between loc and that
 *          point does not exceed @maxwidth, an edge is found and
 *          its position is taken to be loc.  @maxwidth implicitly
 *          sets a minimum on the required gradient of the edge.
 *      (4) The thresholds must be at least 1, and the low threshold
 *          cannot be larger than the high threshold.
 */
l_int32
pixScanForEdge(PIX      *pixs,
               BOX      *box,
               l_int32   lowthresh,
               l_int32   highthresh,
               l_int32   maxwidth,
               l_int32   factor,
               l_int32   scanflag,
               l_int32  *ploc)
{
l_int32    bx, by, bw, bh, foundmin, loc, sum, wpl;
l_int32    x, xstart, xend, y, ystart, yend;
l_uint32  *data, *line;
BOX       *boxt;

    PROCNAME("pixScanForEdge");

    if (!ploc)
        return ERROR_INT("&ploc not defined", procName, 1);
    *ploc = 0;
    if (!pixs || (pixGetDepth(pixs) != 1))
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);
    if (lowthresh < 1 || highthresh < 1 ||
        lowthresh > highthresh || maxwidth < 1)
        return ERROR_INT("invalid thresholds", procName, 1);
    factor = L_MIN(1, factor);

        /* Clip box to pixs if it exists */
    pixGetDimensions(pixs, &bw, &bh, NULL);
    if (box) {
        if ((boxt = boxClipToRectangle(box, bw, bh)) == NULL)
            return ERROR_INT("invalid box", procName, 1);
        boxGetGeometry(boxt, &bx, &by, &bw, &bh);
        boxDestroy(&boxt);
    } else {
        bx = by = 0;
    }
    xstart = bx;
    ystart = by;
    xend = bx + bw - 1;
    yend = by + bh - 1;

    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    foundmin = 0;
    if (scanflag == L_FROM_LEFT) {
        for (x = xstart; x <= xend; x++) {
            sum = 0;
            for (y = ystart; y <= yend; y += factor) {
                line = data + y * wpl;
                if (GET_DATA_BIT(line, x))
                    sum++;
            }
            if (!foundmin && sum < lowthresh)
                continue;
            if (!foundmin) {  /* save the loc of the beginning of the edge */
                foundmin = 1;
                loc = x;
            }
            if (sum >= highthresh) {
#if DEBUG_EDGES
                fprintf(stderr, "Left: x = %d, loc = %d\n", x, loc);
#endif  /* DEBUG_EDGES */
                if (x - loc < maxwidth) {
                    *ploc = loc;
                    return 0;
                } else {
                  return 1;
                }
            }
        }
    } else if (scanflag == L_FROM_RIGHT) {
        for (x = xend; x >= xstart; x--) {
            sum = 0;
            for (y = ystart; y <= yend; y += factor) {
                line = data + y * wpl;
                if (GET_DATA_BIT(line, x))
                    sum++;
            }
            if (!foundmin && sum < lowthresh)
                continue;
            if (!foundmin) {
                foundmin = 1;
                loc = x;
            }
            if (sum >= highthresh) {
#if DEBUG_EDGES
                fprintf(stderr, "Right: x = %d, loc = %d\n", x, loc);
#endif  /* DEBUG_EDGES */
                if (loc - x < maxwidth) {
                    *ploc = loc;
                    return 0;
                } else {
                  return 1;
                }
            }
        }
    } else if (scanflag == L_FROM_TOP) {
        for (y = ystart; y <= yend; y++) {
            sum = 0;
            line = data + y * wpl;
            for (x = xstart; x <= xend; x += factor) {
                if (GET_DATA_BIT(line, x))
                    sum++;
            }
            if (!foundmin && sum < lowthresh)
                continue;
            if (!foundmin) {
                foundmin = 1;
                loc = y;
            }
            if (sum >= highthresh) {
#if DEBUG_EDGES
                fprintf(stderr, "Top: y = %d, loc = %d\n", y, loc);
#endif  /* DEBUG_EDGES */
                if (y - loc < maxwidth) {
                    *ploc = loc;
                    return 0;
                } else {
                  return 1;
                }
            }
        }
    } else if (scanflag == L_FROM_BOT) {
        for (y = yend; y >= ystart; y--) {
            sum = 0;
            line = data + y * wpl;
            for (x = xstart; x <= xend; x += factor) {
                if (GET_DATA_BIT(line, x))
                    sum++;
            }
            if (!foundmin && sum < lowthresh)
                continue;
            if (!foundmin) {
                foundmin = 1;
                loc = y;
            }
            if (sum >= highthresh) {
#if DEBUG_EDGES
                fprintf(stderr, "Bottom: y = %d, loc = %d\n", y, loc);
#endif  /* DEBUG_EDGES */
                if (loc - y < maxwidth) {
                    *ploc = loc;
                    return 0;
                } else {
                  return 1;
                }
            }
        }
    } else {
        return ERROR_INT("invalid scanflag", procName, 1);
    }

    return 1;  /* edge not found */
}


/*---------------------------------------------------------------------*
 *           Extract pixel averages and reversals along lines          *
 *---------------------------------------------------------------------*/
/*!
 *  pixExtractOnLine()
 *
 *      Input:  pixs (1 bpp or 8 bpp; no colormap)
 *              x1, y1 (one end point for line)
 *              x2, y2 (another end pt for line)
 *              factor (sampling; >= 1)
 *      Return: na (of pixel values along line), or null on error.
 *
 *  Notes:
 *      (1) Input end points are clipped to the pix.
 *      (2) If the line is either horizontal, or closer to horizontal
 *          than to vertical, the points will be extracted from left
 *          to right in the pix.  Likewise, if the line is vertical,
 *          or closer to vertical than to horizontal, the points will
 *          be extracted from top to bottom.
 *      (3) Can be used with numaCountReverals(), for example, to
 *          characterize the intensity smoothness along a line.
 */
NUMA *
pixExtractOnLine(PIX     *pixs,
                 l_int32  x1,
                 l_int32  y1,
                 l_int32  x2,
                 l_int32  y2,
                 l_int32  factor)
{
l_int32    i, w, h, d, xmin, ymin, xmax, ymax, npts, direction;
l_uint32   val;
l_float32  x, y;
l_float64  slope;
NUMA      *na;
PTA       *pta;

    PROCNAME("pixExtractOnLine");

    if (!pixs)
        return (NUMA *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 8)
        return (NUMA *)ERROR_PTR("d not 1 or 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        return (NUMA *)ERROR_PTR("pixs has a colormap", procName, NULL);
    if (factor < 1) {
        L_WARNING("factor must be >= 1; setting to 1\n", procName);
        factor = 1;
    }

        /* Clip line to the image */
    x1 = L_MAX(0, L_MIN(x1, w - 1));
    x2 = L_MAX(0, L_MIN(x2, w - 1));
    y1 = L_MAX(0, L_MIN(y1, h - 1));
    y2 = L_MAX(0, L_MIN(y2, h - 1));

    if (x1 == x2 && y1 == y2) {
        pixGetPixel(pixs, x1, y1, &val);
        na = numaCreate(1);
        numaAddNumber(na, val);
        return na;
    }

    if (y1 == y2)
        direction = L_HORIZONTAL_LINE;
    else if (x1 == x2)
        direction = L_VERTICAL_LINE;
    else
        direction = L_OBLIQUE_LINE;

    na = numaCreate(0);
    if (direction == L_HORIZONTAL_LINE) {  /* plot against x */
        xmin = L_MIN(x1, x2);
        xmax = L_MAX(x1, x2);
        numaSetParameters(na, xmin, factor);
        for (i = xmin; i <= xmax; i += factor) {
            pixGetPixel(pixs, i, y1, &val);
            numaAddNumber(na, val);
        }
    } else if (direction == L_VERTICAL_LINE) {  /* plot against y */
        ymin = L_MIN(y1, y2);
        ymax = L_MAX(y1, y2);
        numaSetParameters(na, ymin, factor);
        for (i = ymin; i <= ymax; i += factor) {
            pixGetPixel(pixs, x1, i, &val);
            numaAddNumber(na, val);
        }
    } else {  /* direction == L_OBLIQUE_LINE */
        slope = (l_float64)((y2 - y1) / (x2 - x1));
        if (L_ABS(slope) < 1.0) {  /* quasi-horizontal */
            xmin = L_MIN(x1, x2);
            xmax = L_MAX(x1, x2);
            ymin = (xmin == x1) ? y1 : y2;  /* pt that goes with xmin */
            ymax = (ymin == y1) ? y2 : y1;  /* pt that goes with xmax */
            pta = generatePtaLine(xmin, ymin, xmax, ymax);
            numaSetParameters(na, xmin, (l_float32)factor);
        } else {  /* quasi-vertical */
            ymin = L_MIN(y1, y2);
            ymax = L_MAX(y1, y2);
            xmin = (ymin == y1) ? x1 : x2;  /* pt that goes with ymin */
            xmax = (xmin == x1) ? x2 : x1;  /* pt that goes with ymax */
            pta = generatePtaLine(xmin, ymin, xmax, ymax);
            numaSetParameters(na, ymin, (l_float32)factor);
        }
        npts = ptaGetCount(pta);
        for (i = 0; i < npts; i += factor) {
            ptaGetPt(pta, i, &x, &y);
            pixGetPixel(pixs, (l_int32)x, (l_int32)y, &val);
            numaAddNumber(na, val);
        }

#if 0  /* debugging */
        pixPlotAlongPta(pixs, pta, GPLOT_X11, NULL);
#endif

        ptaDestroy(&pta);
    }

    return na;
}


/*!
 *  pixAverageOnLine()
 *
 *      Input:  pixs (1 bpp or 8 bpp; no colormap)
 *              x1, y1 (starting pt for line)
 *              x2, y2 (end pt for line)
 *              factor (sampling; >= 1)
 *      Return: average of pixel values along line, or null on error.
 *
 *  Notes:
 *      (1) The line must be either horizontal or vertical, so either
 *          y1 == y2 (horizontal) or x1 == x2 (vertical).
 *      (2) If horizontal, x1 must be <= x2.
 *          If vertical, y1 must be <= y2.
 *          characterize the intensity smoothness along a line.
 *      (3) Input end points are clipped to the pix.
 */
l_float32
pixAverageOnLine(PIX     *pixs,
                 l_int32  x1,
                 l_int32  y1,
                 l_int32  x2,
                 l_int32  y2,
                 l_int32  factor)
{
l_int32    i, j, w, h, d, direction, count, wpl;
l_uint32  *data, *line;
l_float32  sum;

    PROCNAME("pixAverageOnLine");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 8)
        return ERROR_INT("d not 1 or 8 bpp", procName, 1);
    if (pixGetColormap(pixs))
        return ERROR_INT("pixs has a colormap", procName, 1);
    if (x1 > x2 || y1 > y2)
        return ERROR_INT("x1 > x2 or y1 > y2", procName, 1);

    if (y1 == y2) {
        x1 = L_MAX(0, x1);
        x2 = L_MIN(w - 1, x2);
        y1 = L_MAX(0, L_MIN(y1, h - 1));
        direction = L_HORIZONTAL_LINE;
    } else if (x1 == x2) {
        y1 = L_MAX(0, y1);
        y2 = L_MIN(h - 1, y2);
        x1 = L_MAX(0, L_MIN(x1, w - 1));
        direction = L_VERTICAL_LINE;
    } else {
        return ERROR_INT("line neither horiz nor vert", procName, 1);
    }

    if (factor < 1) {
        L_WARNING("factor must be >= 1; setting to 1\n", procName);
        factor = 1;
    }

    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    sum = 0;
    if (direction == L_HORIZONTAL_LINE) {
        line = data + y1 * wpl;
        for (j = x1, count = 0; j <= x2; count++, j += factor) {
            if (d == 1)
                sum += GET_DATA_BIT(line, j);
            else  /* d == 8 */
                sum += GET_DATA_BYTE(line, j);
        }
    } else if (direction == L_VERTICAL_LINE) {
        for (i = y1, count = 0; i <= y2; count++, i += factor) {
            line = data + i * wpl;
            if (d == 1)
                sum += GET_DATA_BIT(line, x1);
            else  /* d == 8 */
                sum += GET_DATA_BYTE(line, x1);
        }
    }

    return sum / (l_float32)count;
}


/*!
 *  pixAverageIntensityProfile()
 *
 *      Input:  pixs (any depth; colormap OK)
 *              fract (fraction of image width or height to be used)
 *              dir (averaging direction: L_HORIZONTAL_LINE or L_VERTICAL_LINE)
 *              first, last (span of rows or columns to measure)
 *              factor1 (sampling along fast scan direction; >= 1)
 *              factor2 (sampling along slow scan direction; >= 1)
 *      Return: na (of reversal profile), or null on error.
 *
 *  Notes:
 *      (1) If d != 1 bpp, colormaps are removed and the result
 *          is converted to 8 bpp.
 *      (2) If @dir == L_HORIZONTAL_LINE, the intensity is averaged
 *          along each horizontal raster line (sampled by @factor1),
 *          and the profile is the array of these averages in the
 *          vertical direction between @first and @last raster lines,
 *          and sampled by @factor2.
 *      (3) If @dir == L_VERTICAL_LINE, the intensity is averaged
 *          along each vertical line (sampled by @factor1),
 *          and the profile is the array of these averages in the
 *          horizontal direction between @first and @last columns,
 *          and sampled by @factor2.
 *      (4) The averages are measured over the central @fract of the image.
 *          Use @fract == 1.0 to average across the entire width or height.
 */
NUMA *
pixAverageIntensityProfile(PIX       *pixs,
                           l_float32  fract,
                           l_int32    dir,
                           l_int32    first,
                           l_int32    last,
                           l_int32    factor1,
                           l_int32    factor2)
{
l_int32    i, j, w, h, d, start, end;
l_float32  ave;
NUMA      *nad;
PIX       *pixr, *pixg;

    PROCNAME("pixAverageIntensityProfile");

    if (!pixs)
        return (NUMA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (fract < 0.0 || fract > 1.0)
        return (NUMA *)ERROR_PTR("fract < 0.0 or > 1.0", procName, NULL);
    if (dir != L_HORIZONTAL_LINE && dir != L_VERTICAL_LINE)
        return (NUMA *)ERROR_PTR("invalid direction", procName, NULL);
    if (first < 0) first = 0;
    if (last < first)
        return (NUMA *)ERROR_PTR("last must be >= first", procName, NULL);
    if (factor1 < 1) {
        L_WARNING("factor1 must be >= 1; setting to 1\n", procName);
        factor1 = 1;
    }
    if (factor2 < 1) {
        L_WARNING("factor2 must be >= 1; setting to 1\n", procName);
        factor2 = 1;
    }

        /* Use 1 or 8 bpp, without colormap */
    if (pixGetColormap(pixs))
        pixr = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
    else
        pixr = pixClone(pixs);
    pixGetDimensions(pixr, &w, &h, &d);
    if (d == 1)
        pixg = pixClone(pixr);
    else
        pixg = pixConvertTo8(pixr, 0);

    nad = numaCreate(0);  /* output: samples in slow scan direction */
    numaSetParameters(nad, 0, factor2);
    if (dir == L_HORIZONTAL_LINE) {
        start = (l_int32)(0.5 * (1.0 - fract) * (l_float32)w);
        end = w - start;
        if (last > h - 1) {
            L_WARNING("last > h - 1; clipping\n", procName);
            last = h - 1;
        }
        for (i = first; i <= last; i += factor2) {
            ave = pixAverageOnLine(pixg, start, i, end, i, factor1);
            numaAddNumber(nad, ave);
        }
    } else if (dir == L_VERTICAL_LINE) {
        start = (l_int32)(0.5 * (1.0 - fract) * (l_float32)h);
        end = h - start;
        if (last > w - 1) {
            L_WARNING("last > w - 1; clipping\n", procName);
            last = w - 1;
        }
        for (j = first; j <= last; j += factor2) {
            ave = pixAverageOnLine(pixg, j, start, j, end, factor1);
            numaAddNumber(nad, ave);
        }
    }

    pixDestroy(&pixr);
    pixDestroy(&pixg);
    return nad;
}


/*!
 *  pixReversalProfile()
 *
 *      Input:  pixs (any depth; colormap OK)
 *              fract (fraction of image width or height to be used)
 *              dir (profile direction: L_HORIZONTAL_LINE or L_VERTICAL_LINE)
 *              first, last (span of rows or columns to measure)
 *              minreversal (minimum change in intensity to trigger a reversal)
 *              factor1 (sampling along raster line (fast scan); >= 1)
 *              factor2 (sampling of raster lines (slow scan); >= 1)
 *      Return: na (of reversal profile), or null on error.
 *
 *  Notes:
 *      (1) If d != 1 bpp, colormaps are removed and the result
 *          is converted to 8 bpp.
 *      (2) If @dir == L_HORIZONTAL_LINE, the the reversals are counted
 *          along each horizontal raster line (sampled by @factor1),
 *          and the profile is the array of these sums in the
 *          vertical direction between @first and @last raster lines,
 *          and sampled by @factor2.
 *      (3) If @dir == L_VERTICAL_LINE, the the reversals are counted
 *          along each vertical column (sampled by @factor1),
 *          and the profile is the array of these sums in the
 *          horizontal direction between @first and @last columns,
 *          and sampled by @factor2.
 *      (4) For each row or column, the reversals are summed over the
 *          central @fract of the image.  Use @fract == 1.0 to sum
 *          across the entire width (of row) or height (of column).
 *      (5) @minreversal is the relative change in intensity that is
 *          required to resolve peaks and valleys.  A typical number for
 *          locating text in 8 bpp might be 50.  For 1 bpp, minreversal
 *          must be 1.
 *      (6) The reversal profile is simply the number of reversals
 *          in a row or column, vs the row or column index.
 */
NUMA *
pixReversalProfile(PIX       *pixs,
                   l_float32  fract,
                   l_int32    dir,
                   l_int32    first,
                   l_int32    last,
                   l_int32    minreversal,
                   l_int32    factor1,
                   l_int32    factor2)
{
l_int32   i, j, w, h, d, start, end, nr;
NUMA     *naline, *nad;
PIX      *pixr, *pixg;

    PROCNAME("pixReversalProfile");

    if (!pixs)
        return (NUMA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (fract < 0.0 || fract > 1.0)
        return (NUMA *)ERROR_PTR("fract < 0.0 or > 1.0", procName, NULL);
    if (dir != L_HORIZONTAL_LINE && dir != L_VERTICAL_LINE)
        return (NUMA *)ERROR_PTR("invalid direction", procName, NULL);
    if (first < 0) first = 0;
    if (last < first)
        return (NUMA *)ERROR_PTR("last must be >= first", procName, NULL);
    if (factor1 < 1) {
        L_WARNING("factor1 must be >= 1; setting to 1\n", procName);
        factor1 = 1;
    }
    if (factor2 < 1) {
        L_WARNING("factor2 must be >= 1; setting to 1\n", procName);
        factor2 = 1;
    }

        /* Use 1 or 8 bpp, without colormap */
    if (pixGetColormap(pixs))
        pixr = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
    else
        pixr = pixClone(pixs);
    pixGetDimensions(pixr, &w, &h, &d);
    if (d == 1) {
        pixg = pixClone(pixr);
        minreversal = 1;  /* enforce this */
    } else {
        pixg = pixConvertTo8(pixr, 0);
    }

    nad = numaCreate(0);  /* output: samples in slow scan direction */
    numaSetParameters(nad, 0, factor2);
    if (dir == L_HORIZONTAL_LINE) {
        start = (l_int32)(0.5 * (1.0 - fract) * (l_float32)w);
        end = w - start;
        if (last > h - 1) {
            L_WARNING("last > h - 1; clipping\n", procName);
            last = h - 1;
        }
        for (i = first; i <= last; i += factor2) {
            naline = pixExtractOnLine(pixg, start, i, end, i, factor1);
            numaCountReversals(naline, minreversal, &nr, NULL);
            numaAddNumber(nad, nr);
            numaDestroy(&naline);
        }
    } else if (dir == L_VERTICAL_LINE) {
        start = (l_int32)(0.5 * (1.0 - fract) * (l_float32)h);
        end = h - start;
        if (last > w - 1) {
            L_WARNING("last > w - 1; clipping\n", procName);
            last = w - 1;
        }
        for (j = first; j <= last; j += factor2) {
            naline = pixExtractOnLine(pixg, j, start, j, end, factor1);
            numaCountReversals(naline, minreversal, &nr, NULL);
            numaAddNumber(nad, nr);
            numaDestroy(&naline);
        }
    }

    pixDestroy(&pixr);
    pixDestroy(&pixg);
    return nad;
}


/*---------------------------------------------------------------------*
 *                 Extract windowed variance along a line              *
 *---------------------------------------------------------------------*/
/*!
 *  pixWindowedVarianceOnLine()
 *
 *      Input:  pixs (8 bpp; no colormap)
 *              dir (L_HORIZONTAL_LINE or L_VERTICAL_LINE)
 *              loc (location of the constant coordinate for the line)
 *              c1, c2 (end point coordinates for the line)
 *              size (window size; must be > 1)
 *              &nad (<return> windowed square root of variance)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The returned variance array traverses the line starting
 *          from the smallest coordinate, min(c1,c2).
 *      (2) Line end points are clipped to pixs.
 *      (3) The reference point for the variance calculation is the center of
 *          the window.  Therefore, the numa start parameter from
 *          pixExtractOnLine() is incremented by @size/2,
 *          to align the variance values with the pixel coordinate.
 *      (4) The square root of the variance is the RMS deviation from the mean.
 */
l_int32
pixWindowedVarianceOnLine(PIX     *pixs,
                          l_int32  dir,
                          l_int32  loc,
                          l_int32  c1,
                          l_int32  c2,
                          l_int32  size,
                          NUMA   **pnad)
{
l_int32     i, j, w, h, cmin, cmax, maxloc, n, x, y;
l_uint32    val;
l_float32   norm, rootvar;
l_float32  *array;
l_float64   sum1, sum2, ave, var;
NUMA       *na1, *nad;
PTA        *pta;

    PROCNAME("pixWindowedVarianceOnLine");

    if (!pnad)
        return ERROR_INT("&nad not defined", procName, 1);
    *pnad = NULL;
    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8bpp", procName, 1);
    if (size < 2)
        return ERROR_INT("window size must be > 1", procName, 1);
    if (dir != L_HORIZONTAL_LINE && dir != L_VERTICAL_LINE)
        return ERROR_INT("invalid direction", procName, 1);
    pixGetDimensions(pixs, &w, &h, NULL);
    maxloc = (dir == L_HORIZONTAL_LINE) ? h - 1 : w - 1;
    if (loc < 0 || loc > maxloc)
        return ERROR_INT("invalid line position", procName, 1);

        /* Clip line to the image */
    cmin = L_MIN(c1, c2);
    cmax = L_MAX(c1, c2);
    maxloc = (dir == L_HORIZONTAL_LINE) ? w - 1 : h - 1;
    cmin = L_MAX(0, L_MIN(cmin, maxloc));
    cmax = L_MAX(0, L_MIN(cmax, maxloc));
    n = cmax - cmin + 1;

        /* Generate pta along the line */
    pta = ptaCreate(n);
    if (dir == L_HORIZONTAL_LINE) {
        for (i = cmin; i <= cmax; i++)
            ptaAddPt(pta, i, loc);
    } else {  /* vertical line */
        for (i = cmin; i <= cmax; i++)
            ptaAddPt(pta, loc, i);
    }

        /* Get numa of pixel values on the line */
    na1 = numaCreate(n);
    numaSetParameters(na1, cmin, 1);
    for (i = 0; i < n; i++) {
        ptaGetIPt(pta, i, &x, &y);
        pixGetPixel(pixs, x, y, &val);
        numaAddNumber(na1, val);
    }
    array = numaGetFArray(na1, L_NOCOPY);
    ptaDestroy(&pta);

        /* Compute root variance on overlapping windows */
    nad = numaCreate(n);
    *pnad = nad;
    numaSetParameters(nad, cmin + size / 2, 1);
    norm = 1.0 / (l_float32)size;
    for (i = 0; i < n - size; i++) {  /* along the line */
        sum1 = sum2 = 0;
        for (j = 0; j < size; j++) {  /* over the window */
            val = array[i + j];
            sum1 += val;
            sum2 += val * val;
        }
        ave = norm * sum1;
        var = norm * sum2 - ave * ave;
        rootvar = (l_float32)sqrt(var);
        numaAddNumber(nad, rootvar);
    }

    numaDestroy(&na1);
    return 0;
}


/*---------------------------------------------------------------------*
 *              Extract min/max of pixel values near lines             *
 *---------------------------------------------------------------------*/
/*!
 *  pixMinMaxNearLine()
 *
 *      Input:  pixs (8 bpp; no colormap)
 *              x1, y1 (starting pt for line)
 *              x2, y2 (end pt for line)
 *              dist (distance to search from line in each direction)
 *              direction (L_SCAN_NEGATIVE, L_SCAN_POSITIVE, L_SCAN_BOTH)
 *              &namin (<optional return> minimum values)
 *              &namax (<optional return> maximum values)
 *              &minave (<optional return> average of minimum values)
 *              &maxave (<optional return> average of maximum values)
 *      Return: 0 if OK; 1 on error or if there are no sampled points
 *              within the image.
 *
 *  Notes:
 *      (1) If the line is more horizontal than vertical, the values
 *          are computed for [x1, x2], and the pixels are taken
 *          below and/or above the local y-value.  Otherwise, the
 *          values are computed for [y1, y2] and the pixels are taken
 *          to the left and/or right of the local x value.
 *      (2) @direction specifies which side (or both sides) of the
 *          line are scanned for min and max values.
 *      (3) There are two ways to tell if the returned values of min
 *          and max averages are valid: the returned values cannot be
 *          negative and the function must return 0.
 *      (4) All accessed pixels are clipped to the pix.
 */
l_int32
pixMinMaxNearLine(PIX        *pixs,
                  l_int32     x1,
                  l_int32     y1,
                  l_int32     x2,
                  l_int32     y2,
                  l_int32     dist,
                  l_int32     direction,
                  NUMA      **pnamin,
                  NUMA      **pnamax,
                  l_float32  *pminave,
                  l_float32  *pmaxave)
{
l_int32    i, j, w, h, d, x, y, n, dir, found, minval, maxval, negloc, posloc;
l_uint32   val;
l_float32  sum;
NUMA      *namin, *namax;
PTA       *pta;

    PROCNAME("pixMinMaxNearLine");

    if (pnamin) *pnamin = NULL;
    if (pnamax) *pnamax = NULL;
    if (pminave) *pminave = UNDEF;
    if (pmaxave) *pmaxave = UNDEF;
    if (!pnamin && !pnamax && !pminave && !pmaxave)
        return ERROR_INT("no output requested", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8 || pixGetColormap(pixs))
        return ERROR_INT("pixs not 8 bpp or has colormap", procName, 1);
    dist = L_ABS(dist);
    if (direction != L_SCAN_NEGATIVE && direction != L_SCAN_POSITIVE &&
        direction != L_SCAN_BOTH)
        return ERROR_INT("invalid direction", procName, 1);

    pta = generatePtaLine(x1, y1, x2, y2);
    n = ptaGetCount(pta);
    dir = (L_ABS(x1 - x2) == n - 1) ? L_HORIZ : L_VERT;
    namin = numaCreate(n);
    namax = numaCreate(n);
    negloc = -dist;
    posloc = dist;
    if (direction == L_SCAN_NEGATIVE)
        posloc = 0;
    else if (direction == L_SCAN_POSITIVE)
        negloc = 0;
    for (i = 0; i < n; i++) {
        ptaGetIPt(pta, i, &x, &y);
        minval = 255;
        maxval = 0;
        found = FALSE;
        if (dir == L_HORIZ) {
            if (x < 0 || x >= w) continue;
            for (j = negloc; j <= posloc; j++) {
                if (y + j < 0 || y + j >= h) continue;
                pixGetPixel(pixs, x, y + j, &val);
                found = TRUE;
                if (val < minval) minval = val;
                if (val > maxval) maxval = val;
            }
        } else {  /* dir == L_VERT */
            if (y < 0 || y >= h) continue;
            for (j = negloc; j <= posloc; j++) {
                if (x + j < 0 || x + j >= w) continue;
                pixGetPixel(pixs, x + j, y, &val);
                found = TRUE;
                if (val < minval) minval = val;
                if (val > maxval) maxval = val;
            }
        }
        if (found) {
            numaAddNumber(namin, minval);
            numaAddNumber(namax, maxval);
        }
    }

    n = numaGetCount(namin);
    if (n == 0) {
        numaDestroy(&namin);
        numaDestroy(&namax);
        ptaDestroy(&pta);
        return ERROR_INT("no output from this line", procName, 1);
    }

    if (pminave) {
        numaGetSum(namin, &sum);
        *pminave = sum / n;
    }
    if (pmaxave) {
        numaGetSum(namax, &sum);
        *pmaxave = sum / n;
    }
    if (pnamin)
        *pnamin = namin;
    else
        numaDestroy(&namin);
    if (pnamax)
        *pnamax = namax;
    else
        numaDestroy(&namax);
    ptaDestroy(&pta);
    return 0;
}


/*---------------------------------------------------------------------*
 *                     Rank row and column transforms                  *
 *---------------------------------------------------------------------*/
/*!
 *  pixRankRowTransform()
 *
 *      Input:  pixs (8 bpp; no colormap)
 *      Return: pixd (with pixels sorted in each row, from
 *                    min to max value)
 *
 * Notes:
 *     (1) The time is O(n) in the number of pixels and runs about
 *         100 Mpixels/sec on a 3 GHz machine.
 */
PIX *
pixRankRowTransform(PIX  *pixs)
{
l_int32    i, j, k, m, w, h, wpl, val;
l_int32    histo[256];
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixRankRowTransform");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs has a colormap", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    pixd = pixCreateTemplateNoInit(pixs);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpl = pixGetWpl(pixs);
    for (i = 0; i < h; i++) {
        memset(histo, 0, 1024);
        lines = datas + i * wpl;
        lined = datad + i * wpl;
        for (j = 0; j < w; j++) {
            val = GET_DATA_BYTE(lines, j);
            histo[val]++;
        }
        for (m = 0, j = 0; m < 256; m++) {
            for (k = 0; k < histo[m]; k++, j++)
                SET_DATA_BYTE(lined, j, m);
        }
    }

    return pixd;
}


/*!
 *  pixRankColumnTransform()
 *
 *      Input:  pixs (8 bpp; no colormap)
 *      Return: pixd (with pixels sorted in each column, from
 *                    min to max value)
 *
 * Notes:
 *     (1) The time is O(n) in the number of pixels and runs about
 *         50 Mpixels/sec on a 3 GHz machine.
 */
PIX *
pixRankColumnTransform(PIX  *pixs)
{
l_int32    i, j, k, m, w, h, val;
l_int32    histo[256];
void     **lines8, **lined8;
PIX       *pixd;

    PROCNAME("pixRankColumnTransform");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs has a colormap", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    pixd = pixCreateTemplateNoInit(pixs);
    lines8 = pixGetLinePtrs(pixs, NULL);
    lined8 = pixGetLinePtrs(pixd, NULL);
    for (j = 0; j < w; j++) {
        memset(histo, 0, 1024);
        for (i = 0; i < h; i++) {
            val = GET_DATA_BYTE(lines8[i], j);
            histo[val]++;
        }
        for (m = 0, i = 0; m < 256; m++) {
            for (k = 0; k < histo[m]; k++, i++)
                SET_DATA_BYTE(lined8[i], j, m);
        }
    }

    FREE(lines8);
    FREE(lined8);
    return pixd;
}
