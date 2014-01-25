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
 *  compare.c
 *
 *      Test for pix equality
 *           l_int32     pixEqual()
 *           l_int32     pixEqualWithAlpha()
 *           l_int32     pixEqualWithCmap()
 *           l_int32     pixUsesCmapColor()
 *
 *      Binary correlation
 *           l_int32     pixCorrelationBinary()
 *
 *      Difference of two images of same size
 *           l_int32     pixDisplayDiffBinary()
 *           l_int32     pixCompareBinary()
 *           l_int32     pixCompareGrayOrRGB()
 *           l_int32     pixCompareGray()
 *           l_int32     pixCompareRGB()
 *           l_int32     pixCompareTiled()
 *
 *      Other measures of the difference of two images of the same size
 *           NUMA       *pixCompareRankDifference()
 *           l_int32     pixTestForSimilarity()
 *           l_int32     pixGetDifferenceStats()
 *           NUMA       *pixGetDifferenceHistogram()
 *           l_int32     pixGetPerceptualDiff()
 *           l_int32     pixGetPSNR()
 *
 *      Translated images at the same resolution
 *           l_int32     pixCompareWithTranslation()
 *           l_int32     pixBestCorrelation()
 */

#include <string.h>
#include <math.h>
#include "allheaders.h"

    /* Small enough to consider equal to 0.0, for plot output */
static const l_float32  TINY = 0.00001;


/*------------------------------------------------------------------*
 *                        Test for pix equality                     *
 *------------------------------------------------------------------*/
/*!
 *  pixEqual()
 *
 *      Input:  pix1
 *              pix2
 *              &same  (<return> 1 if same; 0 if different)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Equality is defined as having the same pixel values for
 *          each respective image pixel.
 *      (2) This works on two pix of any depth.  If one or both pix
 *          have a colormap, the depths can be different and the
 *          two pix can still be equal.
 *      (3) This ignores the alpha component for 32 bpp images.
 *      (4) If both pix have colormaps and the depths are equal,
 *          use the pixEqualWithCmap() function, which does a fast
 *          comparison if the colormaps are identical and a relatively
 *          slow comparison otherwise.
 *      (5) In all other cases, any existing colormaps must first be
 *          removed before doing pixel comparison.  After the colormaps
 *          are removed, the resulting two images must have the same depth.
 *          The "lowest common denominator" is RGB, but this is only
 *          chosen when necessary, or when both have colormaps but
 *          different depths.
 *      (6) For images without colormaps that are not 32 bpp, all bits
 *          in the image part of the data array must be identical.
 */
l_int32
pixEqual(PIX      *pix1,
         PIX      *pix2,
         l_int32  *psame)
{
    return pixEqualWithAlpha(pix1, pix2, 0, psame);
}


/*!
 *  pixEqualWithAlpha()
 *
 *      Input:  pix1
 *              pix2
 *              use_alpha (1 to compare alpha in RGBA; 0 to ignore)
 *              &same  (<return> 1 if same; 0 if different)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) See notes in pixEqual().
 *      (2) This is more general than pixEqual(), in that for 32 bpp
 *          RGBA images, where spp = 4, you can optionally include
 *          the alpha component in the comparison.
 */
l_int32
pixEqualWithAlpha(PIX      *pix1,
                  PIX      *pix2,
                  l_int32   use_alpha,
                  l_int32  *psame)
{
l_int32    w1, h1, d1, w2, h2, d2, wpl1, wpl2;
l_int32    spp1, spp2, i, j, color, mismatch, opaque;
l_int32    fullwords, linebits, endbits;
l_uint32   endmask, wordmask;
l_uint32  *data1, *data2, *line1, *line2;
PIX       *pixs1, *pixs2, *pixt1, *pixt2, *pixalpha;
PIXCMAP   *cmap1, *cmap2;

    PROCNAME("pixEqualWithAlpha");

    if (!psame)
        return ERROR_INT("psame not defined", procName, 1);
    *psame = 0;  /* init to not equal */
    if (!pix1)
        return ERROR_INT("pix1 not defined", procName, 1);
    if (!pix2)
        return ERROR_INT("pix2 not defined", procName, 1);
    pixGetDimensions(pix1, &w1, &h1, &d1);
    pixGetDimensions(pix2, &w2, &h2, &d2);
    if (w1 != w2 || h1 != h2) {
        L_INFO("pix sizes differ\n", procName);
        return 0;
    }

        /* Suppose the use_alpha flag is true.
         * If only one of two 32 bpp images has spp == 4, we call that
         * a "mismatch" of the alpha component.  In the case of a mismatch,
         * if the 4 bpp pix does not have all alpha components opaque (255),
         * the images are not-equal.  However if they are all opaque,
         * this image is equivalent to spp == 3, so we allow the
         * comparison to go forward, testing only for the RGB equality. */
    spp1 = pixGetSpp(pix1);
    spp2 = pixGetSpp(pix2);
    mismatch = 0;
    if (use_alpha && d1 == 32 && d2 == 32) {
        mismatch = ((spp1 == 4 && spp2 != 4) || (spp1 != 4 && spp2 == 4));
        if (mismatch) {
            pixalpha = (spp1 == 4) ? pix1 : pix2;
            pixAlphaIsOpaque(pixalpha, &opaque);
            if (!opaque) {
                L_INFO("just one pix has a non-opaque alpha layer\n", procName);
                return 0;
            }
        }
    }

    cmap1 = pixGetColormap(pix1);
    cmap2 = pixGetColormap(pix2);
    if (!cmap1 && !cmap2 && (d1 != d2) && (d1 == 32 || d2 == 32)) {
        L_INFO("no colormaps, pix depths unequal, and one of them is RGB\n",
               procName);
        return 0;
    }

    if (cmap1 && cmap2 && (d1 == d2))   /* use special function */
        return pixEqualWithCmap(pix1, pix2, psame);

        /* Must remove colormaps if they exist, and in the process
         * end up with the resulting images having the same depth. */
    if (cmap1 && !cmap2) {
        pixUsesCmapColor(pix1, &color);
        if (color && d2 <= 8)  /* can't be equal */
            return 0;
        if (d2 < 8)
            pixs2 = pixConvertTo8(pix2, FALSE);
        else
            pixs2 = pixClone(pix2);
        if (d2 <= 8)
            pixs1 = pixRemoveColormap(pix1, REMOVE_CMAP_TO_GRAYSCALE);
        else
            pixs1 = pixRemoveColormap(pix1, REMOVE_CMAP_TO_FULL_COLOR);
    } else if (!cmap1 && cmap2) {
        pixUsesCmapColor(pix2, &color);
        if (color && d1 <= 8)  /* can't be equal */
            return 0;
        if (d1 < 8)
            pixs1 = pixConvertTo8(pix1, FALSE);
        else
            pixs1 = pixClone(pix1);
        if (d1 <= 8)
            pixs2 = pixRemoveColormap(pix2, REMOVE_CMAP_TO_GRAYSCALE);
        else
            pixs2 = pixRemoveColormap(pix2, REMOVE_CMAP_TO_FULL_COLOR);
    } else if (cmap1 && cmap2) {  /* depths not equal; use rgb */
        pixs1 = pixRemoveColormap(pix1, REMOVE_CMAP_TO_FULL_COLOR);
        pixs2 = pixRemoveColormap(pix2, REMOVE_CMAP_TO_FULL_COLOR);
    } else {  /* no colormaps */
        pixs1 = pixClone(pix1);
        pixs2 = pixClone(pix2);
    }

        /* OK, we have no colormaps, but the depths may still be different */
    d1 = pixGetDepth(pixs1);
    d2 = pixGetDepth(pixs2);
    if (d1 != d2) {
        if (d1 == 16 || d2 == 16) {
            L_INFO("one pix is 16 bpp\n", procName);
            pixDestroy(&pixs1);
            pixDestroy(&pixs2);
            return 0;
        }
        pixt1 = pixConvertLossless(pixs1, 8);
        pixt2 = pixConvertLossless(pixs2, 8);
        if (!pixt1 || !pixt2) {
            L_INFO("failure to convert to 8 bpp\n", procName);
            pixDestroy(&pixs1);
            pixDestroy(&pixs2);
            pixDestroy(&pixt1);
            pixDestroy(&pixt2);
            return 0;
        }
    } else {
        pixt1 = pixClone(pixs1);
        pixt2 = pixClone(pixs2);
    }
    pixDestroy(&pixs1);
    pixDestroy(&pixs2);

        /* No colormaps, equal depths; do pixel comparisons */
    d1 = pixGetDepth(pixt1);
    d2 = pixGetDepth(pixt2);
    wpl1 = pixGetWpl(pixt1);
    wpl2 = pixGetWpl(pixt2);
    data1 = pixGetData(pixt1);
    data2 = pixGetData(pixt2);

    if (d1 == 32) {  /* test either RGB or RGBA pixels */
        if (use_alpha && !mismatch)
            wordmask = (spp1 == 3) ? 0xffffff00 : 0xffffffff;
        else
            wordmask = 0xffffff00;
        for (i = 0; i < h1; i++) {
            line1 = data1 + wpl1 * i;
            line2 = data2 + wpl2 * i;
            for (j = 0; j < wpl1; j++) {
                if ((*line1 ^ *line2) & wordmask) {
                    pixDestroy(&pixt1);
                    pixDestroy(&pixt2);
                    return 0;
                }
                line1++;
                line2++;
            }
        }
    } else {  /* all bits count */
        linebits = d1 * w1;
        fullwords = linebits / 32;
        endbits = linebits & 31;
        endmask = 0xffffffff << (32 - endbits);
        for (i = 0; i < h1; i++) {
            line1 = data1 + wpl1 * i;
            line2 = data2 + wpl2 * i;
            for (j = 0; j < fullwords; j++) {
                if (*line1 ^ *line2) {
                    pixDestroy(&pixt1);
                    pixDestroy(&pixt2);
                    return 0;
                }
                line1++;
                line2++;
            }
            if (endbits) {
                if ((*line1 ^ *line2) & endmask) {
                    pixDestroy(&pixt1);
                    pixDestroy(&pixt2);
                    return 0;
                }
            }
        }
    }

    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    *psame = 1;
    return 0;
}


/*!
 *  pixEqualWithCmap()
 *
 *      Input:  pix1
 *              pix2
 *              &same
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This returns same = TRUE if the images have identical content.
 *      (2) Both pix must have a colormap, and be of equal size and depth.
 *          If these conditions are not satisfied, it is not an error;
 *          the returned result is same = FALSE.
 *      (3) We then check whether the colormaps are the same; if so,
 *          the comparison proceeds 32 bits at a time.
 *      (4) If the colormaps are different, the comparison is done by
 *          slow brute force.
 */
l_int32
pixEqualWithCmap(PIX      *pix1,
                 PIX      *pix2,
                 l_int32  *psame)
{
l_int32    d, w, h, wpl1, wpl2, i, j, linebits, fullwords, endbits;
l_int32    nc1, nc2, samecmaps;
l_int32    rval1, rval2, gval1, gval2, bval1, bval2;
l_uint32   endmask, val1, val2;
l_uint32  *data1, *data2, *line1, *line2;
PIXCMAP   *cmap1, *cmap2;

    PROCNAME("pixEqualWithCmap");

    if (!psame)
        return ERROR_INT("&same not defined", procName, 1);
    *psame = 0;
    if (!pix1)
        return ERROR_INT("pix1 not defined", procName, 1);
    if (!pix2)
        return ERROR_INT("pix2 not defined", procName, 1);

    if (pixSizesEqual(pix1, pix2) == 0)
        return 0;

    cmap1 = pixGetColormap(pix1);
    cmap2 = pixGetColormap(pix2);
    if (!cmap1 || !cmap2) {
        L_INFO("both images don't have colormap\n", procName);
        return 0;
    }
    d = pixGetDepth(pix1);
    if (d != 1 && d != 2 && d != 4 && d != 8) {
        L_INFO("pix depth not in {1, 2, 4, 8}\n", procName);
        return 0;
    }

    nc1 = pixcmapGetCount(cmap1);
    nc2 = pixcmapGetCount(cmap2);
    samecmaps = TRUE;
    if (nc1 != nc2) {
        L_INFO("colormap sizes are different\n", procName);
        samecmaps = FALSE;
    }

        /* Check if colormaps are identical */
    if (samecmaps == TRUE) {
        for (i = 0; i < nc1; i++) {
            pixcmapGetColor(cmap1, i, &rval1, &gval1, &bval1);
            pixcmapGetColor(cmap2, i, &rval2, &gval2, &bval2);
            if (rval1 != rval2 || gval1 != gval2 || bval1 != bval2) {
                samecmaps = FALSE;
                break;
            }
        }
    }

    h = pixGetHeight(pix1);
    w = pixGetWidth(pix1);
    if (samecmaps == TRUE) {  /* colormaps are identical; compare by words */
        linebits = d * w;
        wpl1 = pixGetWpl(pix1);
        wpl2 = pixGetWpl(pix2);
        data1 = pixGetData(pix1);
        data2 = pixGetData(pix2);
        fullwords = linebits / 32;
        endbits = linebits & 31;
        endmask = 0xffffffff << (32 - endbits);
        for (i = 0; i < h; i++) {
            line1 = data1 + wpl1 * i;
            line2 = data2 + wpl2 * i;
            for (j = 0; j < fullwords; j++) {
                if (*line1 ^ *line2)
                    return 0;
                line1++;
                line2++;
            }
            if (endbits) {
                if ((*line1 ^ *line2) & endmask)
                    return 0;
            }
        }
        *psame = 1;
        return 0;
    }

        /* Colormaps aren't identical; compare pixel by pixel */
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            pixGetPixel(pix1, j, i, &val1);
            pixGetPixel(pix2, j, i, &val2);
            pixcmapGetColor(cmap1, val1, &rval1, &gval1, &bval1);
            pixcmapGetColor(cmap2, val2, &rval2, &gval2, &bval2);
            if (rval1 != rval2 || gval1 != gval2 || bval1 != bval2)
                return 0;
        }
    }

    *psame = 1;
    return 0;
}


/*!
 *  pixUsesCmapColor()
 *
 *      Input:  pixs
 *              &color (<return>)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This returns color = TRUE if three things are obtained:
 *          (a) the pix has a colormap
 *          (b) the colormap has at least one color entry
 *          (c) a color entry is actually used
 *      (2) It is used in pixEqual() for comparing two images, in a
 *          situation where it is required to know if the colormap
 *          has color entries that are actually used in the image.
 */
l_int32
pixUsesCmapColor(PIX      *pixs,
                 l_int32  *pcolor)
{
l_int32   n, i, rval, gval, bval, numpix;
NUMA     *na;
PIXCMAP  *cmap;

    PROCNAME("pixUsesCmapColor");

    if (!pcolor)
        return ERROR_INT("&color not defined", procName, 1);
    *pcolor = 0;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

    if ((cmap = pixGetColormap(pixs)) == NULL)
        return 0;

    pixcmapHasColor(cmap, pcolor);
    if (*pcolor == 0)  /* no color */
        return 0;

        /* The cmap has color entries.  Are they used? */
    na = pixGetGrayHistogram(pixs, 1);
    n = pixcmapGetCount(cmap);
    for (i = 0; i < n; i++) {
        pixcmapGetColor(cmap, i, &rval, &gval, &bval);
        numaGetIValue(na, i, &numpix);
        if ((rval != gval || rval != bval) && numpix) {  /* color found! */
            *pcolor = 1;
            break;
        }
    }
    numaDestroy(&na);

    return 0;
}


/*------------------------------------------------------------------*
 *                          Binary correlation                      *
 *------------------------------------------------------------------*/
/*!
 *  pixCorrelationBinary()
 *
 *      Input:  pix1 (1 bpp)
 *              pix2 (1 bpp)
 *              &val (<return> correlation)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The correlation is a number between 0.0 and 1.0,
 *          based on foreground similarity:
 *                           (|1 AND 2|)**2
 *            correlation =  --------------
 *                             |1| * |2|
 *          where |x| is the count of foreground pixels in image x.
 *          If the images are identical, this is 1.0.
 *          If they have no fg pixels in common, this is 0.0.
 *          If one or both images have no fg pixels, the correlation is 0.0.
 *      (2) Typically the two images are of equal size, but this
 *          is not enforced.  Instead, the UL corners are aligned.
 */
l_int32
pixCorrelationBinary(PIX        *pix1,
                     PIX        *pix2,
                     l_float32  *pval)
{
l_int32   count1, count2, countn;
l_int32  *tab8;
PIX      *pixn;

    PROCNAME("pixCorrelationBinary");

    if (!pval)
        return ERROR_INT("&pval not defined", procName, 1);
    *pval = 0.0;
    if (!pix1)
        return ERROR_INT("pix1 not defined", procName, 1);
    if (!pix2)
        return ERROR_INT("pix2 not defined", procName, 1);

    tab8 = makePixelSumTab8();
    pixCountPixels(pix1, &count1, tab8);
    pixCountPixels(pix2, &count2, tab8);
    pixn = pixAnd(NULL, pix1, pix2);
    pixCountPixels(pixn, &countn, tab8);
    *pval = (l_float32)(countn * countn) / (l_float32)(count1 * count2);
    FREE(tab8);
    return 0;
}


/*------------------------------------------------------------------*
 *                   Difference of two images                       *
 *------------------------------------------------------------------*/
/*!
 *  pixDisplayDiffBinary()
 *
 *      Input:  pix1 (1 bpp)
 *              pix2 (1 bpp)
 *      Return: pixd (4 bpp cmapped), or null on error
 *
 *  Notes:
 *      (1) This gives a color representation of the difference between
 *          pix1 and pix2.  The color difference depends on the order.
 *          The pixels in pixd have 4 colors:
 *           * unchanged:  black (on), white (off)
 *           * on in pix1, off in pix2: red
 *           * on in pix2, off in pix1: green
 *      (2) This aligns the UL corners of pix1 and pix2, and crops
 *          to the overlapping pixels.
 */
PIX *
pixDisplayDiffBinary(PIX  *pix1,
                     PIX  *pix2)
{
l_int32   w1, h1, d1, w2, h2, d2, minw, minh;
PIX      *pixt, *pixd;
PIXCMAP  *cmap;

    PROCNAME("pixDisplayDiffBinary");

    if (!pix1 || !pix2)
        return (PIX *)ERROR_PTR("pix1, pix2 not both defined", procName, NULL);
    pixGetDimensions(pix1, &w1, &h1, &d1);
    pixGetDimensions(pix2, &w2, &h2, &d2);
    if (d1 != 1 || d2 != 1)
        return (PIX *)ERROR_PTR("pix1 and pix2 not 1 bpp", procName, NULL);
    minw = L_MIN(w1, w2);
    minh = L_MIN(h1, h2);

    pixd = pixCreate(minw, minh, 4);
    cmap = pixcmapCreate(4);
    pixcmapAddColor(cmap, 255, 255, 255);  /* initialized to white */
    pixcmapAddColor(cmap, 0, 0, 0);
    pixcmapAddColor(cmap, 255, 0, 0);
    pixcmapAddColor(cmap, 0, 255, 0);
    pixSetColormap(pixd, cmap);

    pixt = pixAnd(NULL, pix1, pix2);
    pixPaintThroughMask(pixd, pixt, 0, 0, 0x0);  /* black */
    pixSubtract(pixt, pix1, pix2);
    pixPaintThroughMask(pixd, pixt, 0, 0, 0xff000000);  /* red */
    pixSubtract(pixt, pix2, pix1);
    pixPaintThroughMask(pixd, pixt, 0, 0, 0x00ff0000);  /* green */
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixCompareBinary()
 *
 *      Input:  pix1 (1 bpp)
 *              pix2 (1 bpp)
 *              comptype (L_COMPARE_XOR, L_COMPARE_SUBTRACT)
 *              &fract (<return> fraction of pixels that are different)
 *              &pixdiff (<optional return> pix of difference)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The two images are aligned at the UL corner, and do not
 *          need to be the same size.
 *      (2) If using L_COMPARE_SUBTRACT, pix2 is subtracted from pix1.
 *      (3) The total number of pixels is determined by pix1.
 */
l_int32
pixCompareBinary(PIX        *pix1,
                 PIX        *pix2,
                 l_int32     comptype,
                 l_float32  *pfract,
                 PIX       **ppixdiff)
{
l_int32   w, h, count;
PIX      *pixt;

    PROCNAME("pixCompareBinary");

    if (ppixdiff) *ppixdiff = NULL;
    if (!pfract)
        return ERROR_INT("&pfract not defined", procName, 1);
    *pfract = 0.0;
    if (!pix1 || pixGetDepth(pix1) != 1)
        return ERROR_INT("pix1 not defined or not 1 bpp", procName, 1);
    if (!pix2 || pixGetDepth(pix2) != 1)
        return ERROR_INT("pix2 not defined or not 1 bpp", procName, 1);
    if (comptype != L_COMPARE_XOR && comptype != L_COMPARE_SUBTRACT)
        return ERROR_INT("invalid comptype", procName, 1);

    if (comptype == L_COMPARE_XOR)
        pixt = pixXor(NULL, pix1, pix2);
    else  /* comptype == L_COMPARE_SUBTRACT) */
        pixt = pixSubtract(NULL, pix1, pix2);
    pixCountPixels(pixt, &count, NULL);
    pixGetDimensions(pix1, &w, &h, NULL);
    *pfract = (l_float32)(count) / (l_float32)(w * h);

    if (ppixdiff)
        *ppixdiff = pixt;
    else
        pixDestroy(&pixt);
    return 0;
}


/*!
 *  pixCompareGrayOrRGB()
 *
 *      Input:  pix1 (8 or 16 bpp gray, 32 bpp rgb, or colormapped)
 *              pix2 (8 or 16 bpp gray, 32 bpp rgb, or colormapped)
 *              comptype (L_COMPARE_SUBTRACT, L_COMPARE_ABS_DIFF)
 *              plottype (gplot plot output type, or 0 for no plot)
 *              &same (<optional return> 1 if pixel values are identical)
 *              &diff (<optional return> average difference)
 *              &rmsdiff (<optional return> rms of difference)
 *              &pixdiff (<optional return> pix of difference)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The two images are aligned at the UL corner, and do not
 *          need to be the same size.  If they are not the same size,
 *          the comparison will be made over overlapping pixels.
 *      (2) If there is a colormap, it is removed and the result
 *          is either gray or RGB depending on the colormap.
 *      (3) If RGB, each component is compared separately.
 *      (4) If type is L_COMPARE_ABS_DIFF, pix2 is subtracted from pix1
 *          and the absolute value is taken.
 *      (5) If type is L_COMPARE_SUBTRACT, pix2 is subtracted from pix1
 *          and the result is clipped to 0.
 *      (6) The plot output types are specified in gplot.h.
 *          Use 0 if no difference plot is to be made.
 *      (7) If the images are pixelwise identical, no difference
 *          plot is made, even if requested.  The result (TRUE or FALSE)
 *          is optionally returned in the parameter 'same'.
 *      (8) The average difference (either subtracting or absolute value)
 *          is optionally returned in the parameter 'diff'.
 *      (9) The RMS difference is optionally returned in the
 *          parameter 'rmsdiff'.  For RGB, we return the average of
 *          the RMS differences for each of the components.
 */
l_int32
pixCompareGrayOrRGB(PIX        *pix1,
                    PIX        *pix2,
                    l_int32     comptype,
                    l_int32     plottype,
                    l_int32    *psame,
                    l_float32  *pdiff,
                    l_float32  *prmsdiff,
                    PIX       **ppixdiff)
{
l_int32  retval, d;
PIX     *pixt1, *pixt2;

    PROCNAME("pixCompareGrayOrRGB");

    if (ppixdiff) *ppixdiff = NULL;
    if (!pix1)
        return ERROR_INT("pix1 not defined", procName, 1);
    if (!pix2)
        return ERROR_INT("pix2 not defined", procName, 1);
    if (pixGetDepth(pix1) < 8 && !pixGetColormap(pix1))
        return ERROR_INT("pix1 depth < 8 bpp and not cmapped", procName, 1);
    if (pixGetDepth(pix2) < 8 && !pixGetColormap(pix2))
        return ERROR_INT("pix2 depth < 8 bpp and not cmapped", procName, 1);
    if (comptype != L_COMPARE_SUBTRACT && comptype != L_COMPARE_ABS_DIFF)
        return ERROR_INT("invalid comptype", procName, 1);
    if (plottype > NUM_GPLOT_OUTPUTS)
        return ERROR_INT("invalid plottype", procName, 1);

    pixt1 = pixRemoveColormap(pix1, REMOVE_CMAP_BASED_ON_SRC);
    pixt2 = pixRemoveColormap(pix2, REMOVE_CMAP_BASED_ON_SRC);
    d = pixGetDepth(pixt1);
    if (d != pixGetDepth(pixt2)) {
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        return ERROR_INT("intrinsic depths are not equal", procName, 1);
    }

    if (d == 8 || d == 16)
        retval = pixCompareGray(pixt1, pixt2, comptype, plottype, psame,
                                pdiff, prmsdiff, ppixdiff);
    else  /* d == 32 */
        retval = pixCompareRGB(pixt1, pixt2, comptype, plottype, psame,
                               pdiff, prmsdiff, ppixdiff);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    return retval;
}


/*!
 *  pixCompareGray()
 *
 *      Input:  pix1 (8 or 16 bpp, not cmapped)
 *              pix2 (8 or 16 bpp, not cmapped)
 *              comptype (L_COMPARE_SUBTRACT, L_COMPARE_ABS_DIFF)
 *              plottype (gplot plot output type, or 0 for no plot)
 *              &same (<optional return> 1 if pixel values are identical)
 *              &diff (<optional return> average difference)
 *              &rmsdiff (<optional return> rms of difference)
 *              &pixdiff (<optional return> pix of difference)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) See pixCompareGrayOrRGB() for details.
 *      (2) Use pixCompareGrayOrRGB() if the input pix are colormapped.
 */
l_int32
pixCompareGray(PIX        *pix1,
               PIX        *pix2,
               l_int32     comptype,
               l_int32     plottype,
               l_int32    *psame,
               l_float32  *pdiff,
               l_float32  *prmsdiff,
               PIX       **ppixdiff)
{
l_int32  d1, d2, first, last;
GPLOT   *gplot;
NUMA    *na, *nac;
PIX     *pixt;

    PROCNAME("pixCompareGray");

    if (psame) *psame = 0;
    if (pdiff) *pdiff = 0.0;
    if (prmsdiff) *prmsdiff = 0.0;
    if (ppixdiff) *ppixdiff = NULL;
    if (!pix1)
        return ERROR_INT("pix1 not defined", procName, 1);
    if (!pix2)
        return ERROR_INT("pix2 not defined", procName, 1);
    d1 = pixGetDepth(pix1);
    d2 = pixGetDepth(pix2);
    if ((d1 != d2) || (d1 != 8 && d1 != 16))
        return ERROR_INT("depths unequal or not 8 or 16 bpp", procName, 1);
    if (pixGetColormap(pix1) || pixGetColormap(pix2))
        return ERROR_INT("pix1 and/or pix2 are colormapped", procName, 1);
    if (comptype != L_COMPARE_SUBTRACT && comptype != L_COMPARE_ABS_DIFF)
        return ERROR_INT("invalid comptype", procName, 1);
    if (plottype > NUM_GPLOT_OUTPUTS)
        return ERROR_INT("invalid plottype", procName, 1);

    if (comptype == L_COMPARE_SUBTRACT)
        pixt = pixSubtractGray(NULL, pix1, pix2);
    else  /* comptype == L_COMPARE_ABS_DIFF) */
        pixt = pixAbsDifference(pix1, pix2);

    if (psame)
        pixZero(pixt, psame);

    if (pdiff)
        pixGetAverageMasked(pixt, NULL, 0, 0, 1, L_MEAN_ABSVAL, pdiff);

    if (plottype) {
        na = pixGetGrayHistogram(pixt, 1);
        numaGetNonzeroRange(na, TINY, &first, &last);
        nac = numaClipToInterval(na, 0, last);
        gplot = gplotCreate("/tmp/grayroot", plottype,
                            "Pixel Difference Histogram", "diff val",
                            "number of pixels");
        gplotAddPlot(gplot, NULL, nac, GPLOT_LINES, "gray");
        gplotMakeOutput(gplot);
        gplotDestroy(&gplot);
        numaDestroy(&na);
        numaDestroy(&nac);
    }

    if (ppixdiff)
        *ppixdiff = pixCopy(NULL, pixt);

    if (prmsdiff) {
        if (comptype == L_COMPARE_SUBTRACT) {  /* wrong type for rms diff */
            pixDestroy(&pixt);
            pixt = pixAbsDifference(pix1, pix2);
        }
        pixGetAverageMasked(pixt, NULL, 0, 0, 1, L_ROOT_MEAN_SQUARE, prmsdiff);
    }

    pixDestroy(&pixt);
    return 0;
}


/*!
 *  pixCompareRGB()
 *
 *      Input:  pix1 (32 bpp rgb)
 *              pix2 (32 bpp rgb)
 *              comptype (L_COMPARE_SUBTRACT, L_COMPARE_ABS_DIFF)
 *              plottype (gplot plot output type, or 0 for no plot)
 *              &same (<optional return> 1 if pixel values are identical)
 *              &diff (<optional return> average difference)
 *              &rmsdiff (<optional return> rms of difference)
 *              &pixdiff (<optional return> pix of difference)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) See pixCompareGrayOrRGB() for details.
 */
l_int32
pixCompareRGB(PIX        *pix1,
              PIX        *pix2,
              l_int32     comptype,
              l_int32     plottype,
              l_int32    *psame,
              l_float32  *pdiff,
              l_float32  *prmsdiff,
              PIX       **ppixdiff)
{
l_int32    rsame, gsame, bsame, first, rlast, glast, blast, last;
l_float32  rdiff, gdiff, bdiff;
GPLOT     *gplot;
NUMA      *nar, *nag, *nab, *narc, *nagc, *nabc;
PIX       *pixr1, *pixr2, *pixg1, *pixg2, *pixb1, *pixb2, *pixr, *pixg, *pixb;

    PROCNAME("pixCompareRGB");

    if (ppixdiff) *ppixdiff = NULL;
    if (!pix1 || pixGetDepth(pix1) != 32)
        return ERROR_INT("pix1 not defined or not 32 bpp", procName, 1);
    if (!pix2 || pixGetDepth(pix2) != 32)
        return ERROR_INT("pix2 not defined or not ew bpp", procName, 1);
    if (comptype != L_COMPARE_SUBTRACT && comptype != L_COMPARE_ABS_DIFF)
        return ERROR_INT("invalid comptype", procName, 1);
    if (plottype > NUM_GPLOT_OUTPUTS)
        return ERROR_INT("invalid plottype", procName, 1);

    pixr1 = pixGetRGBComponent(pix1, COLOR_RED);
    pixr2 = pixGetRGBComponent(pix2, COLOR_RED);
    pixg1 = pixGetRGBComponent(pix1, COLOR_GREEN);
    pixg2 = pixGetRGBComponent(pix2, COLOR_GREEN);
    pixb1 = pixGetRGBComponent(pix1, COLOR_BLUE);
    pixb2 = pixGetRGBComponent(pix2, COLOR_BLUE);
    if (comptype == L_COMPARE_SUBTRACT) {
        pixr = pixSubtractGray(NULL, pixr1, pixr2);
        pixg = pixSubtractGray(NULL, pixg1, pixg2);
        pixb = pixSubtractGray(NULL, pixb1, pixb2);
    } else { /* comptype == L_COMPARE_ABS_DIFF) */
        pixr = pixAbsDifference(pixr1, pixr2);
        pixg = pixAbsDifference(pixg1, pixg2);
        pixb = pixAbsDifference(pixb1, pixb2);
    }

    if (psame) {
        pixZero(pixr, &rsame);
        pixZero(pixg, &gsame);
        pixZero(pixb, &bsame);
        if (!rsame || !gsame || !bsame)
            *psame = 0;
        else
            *psame = 1;
    }

    if (pdiff) {
        pixGetAverageMasked(pixr, NULL, 0, 0, 1, L_MEAN_ABSVAL, &rdiff);
        pixGetAverageMasked(pixg, NULL, 0, 0, 1, L_MEAN_ABSVAL, &gdiff);
        pixGetAverageMasked(pixb, NULL, 0, 0, 1, L_MEAN_ABSVAL, &bdiff);
        *pdiff = (rdiff + gdiff + bdiff) / 3.0;
    }

    if (plottype) {
        nar = pixGetGrayHistogram(pixr, 1);
        nag = pixGetGrayHistogram(pixg, 1);
        nab = pixGetGrayHistogram(pixb, 1);
        numaGetNonzeroRange(nar, TINY, &first, &rlast);
        numaGetNonzeroRange(nag, TINY, &first, &glast);
        numaGetNonzeroRange(nab, TINY, &first, &blast);
        last = L_MAX(rlast, glast);
        last = L_MAX(last, blast);
        narc = numaClipToInterval(nar, 0, last);
        nagc = numaClipToInterval(nag, 0, last);
        nabc = numaClipToInterval(nab, 0, last);
        gplot = gplotCreate("/tmp/rgbroot", plottype,
                            "Pixel Difference Histogram", "diff val",
                            "number of pixels");
        gplotAddPlot(gplot, NULL, narc, GPLOT_LINES, "red");
        gplotAddPlot(gplot, NULL, nagc, GPLOT_LINES, "green");
        gplotAddPlot(gplot, NULL, nabc, GPLOT_LINES, "blue");
        gplotMakeOutput(gplot);
        gplotDestroy(&gplot);
        numaDestroy(&nar);
        numaDestroy(&nag);
        numaDestroy(&nab);
        numaDestroy(&narc);
        numaDestroy(&nagc);
        numaDestroy(&nabc);
    }

    if (ppixdiff)
        *ppixdiff = pixCreateRGBImage(pixr, pixg, pixb);

    if (prmsdiff) {
        if (comptype == L_COMPARE_SUBTRACT) {
            pixDestroy(&pixr);
            pixDestroy(&pixg);
            pixDestroy(&pixb);
            pixr = pixAbsDifference(pixr1, pixr2);
            pixg = pixAbsDifference(pixg1, pixg2);
            pixb = pixAbsDifference(pixb1, pixb2);
        }
        pixGetAverageMasked(pixr, NULL, 0, 0, 1, L_ROOT_MEAN_SQUARE, &rdiff);
        pixGetAverageMasked(pixg, NULL, 0, 0, 1, L_ROOT_MEAN_SQUARE, &gdiff);
        pixGetAverageMasked(pixb, NULL, 0, 0, 1, L_ROOT_MEAN_SQUARE, &bdiff);
        *prmsdiff = (rdiff + gdiff + bdiff) / 3.0;
    }

    pixDestroy(&pixr1);
    pixDestroy(&pixr2);
    pixDestroy(&pixg1);
    pixDestroy(&pixg2);
    pixDestroy(&pixb1);
    pixDestroy(&pixb2);
    pixDestroy(&pixr);
    pixDestroy(&pixg);
    pixDestroy(&pixb);
    return 0;
}


/*!
 *  pixCompareTiled()
 *
 *      Input:  pix1 (8 bpp or 32 bpp rgb)
 *              pix2 (8 bpp 32 bpp rgb)
 *              sx, sy (tile size; must be > 1)
 *              type (L_MEAN_ABSVAL or L_ROOT_MEAN_SQUARE)
 *              &pixdiff (<return> pix of difference)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) With L_MEAN_ABSVAL, we compute for each tile the
 *          average abs value of the pixel component difference between
 *          the two (aligned) images.  With L_ROOT_MEAN_SQUARE, we
 *          compute instead the rms difference over all components.
 *      (2) The two input pix must be the same depth.  Comparison is made
 *          using UL corner alignment.
 *      (3) For 32 bpp, the distance between corresponding tiles
 *          is found by averaging the measured difference over all three
 *          components of each pixel in the tile.
 *      (4) The result, pixdiff, contains one pixel for each source tile.
 */
l_int32
pixCompareTiled(PIX     *pix1,
                PIX     *pix2,
                l_int32  sx,
                l_int32  sy,
                l_int32  type,
                PIX    **ppixdiff)
{
l_int32    d1, d2, w, h;
PIX       *pixt, *pixr, *pixg, *pixb;
PIX       *pixrdiff, *pixgdiff, *pixbdiff;
PIXACC    *pixacc;

    PROCNAME("pixCompareTiled");

    if (!ppixdiff)
        return ERROR_INT("&pixdiff not defined", procName, 1);
    *ppixdiff = NULL;
    if (!pix1)
        return ERROR_INT("pix1 not defined", procName, 1);
    if (!pix2)
        return ERROR_INT("pix2 not defined", procName, 1);
    d1 = pixGetDepth(pix1);
    d2 = pixGetDepth(pix2);
    if (d1 != d2)
        return ERROR_INT("depths not equal", procName, 1);
    if (d1 != 8 && d1 != 32)
        return ERROR_INT("pix1 not 8 or 32 bpp", procName, 1);
    if (d2 != 8 && d2 != 32)
        return ERROR_INT("pix2 not 8 or 32 bpp", procName, 1);
    if (sx < 2 || sy < 2)
        return ERROR_INT("sx and sy not both > 1", procName, 1);
    if (type != L_MEAN_ABSVAL && type != L_ROOT_MEAN_SQUARE)
        return ERROR_INT("invalid type", procName, 1);

    pixt = pixAbsDifference(pix1, pix2);
    if (d1 == 8) {
        *ppixdiff = pixGetAverageTiled(pixt, sx, sy, type);
    } else {  /* d1 == 32 */
        pixr = pixGetRGBComponent(pixt, COLOR_RED);
        pixg = pixGetRGBComponent(pixt, COLOR_GREEN);
        pixb = pixGetRGBComponent(pixt, COLOR_BLUE);
        pixrdiff = pixGetAverageTiled(pixr, sx, sy, type);
        pixgdiff = pixGetAverageTiled(pixg, sx, sy, type);
        pixbdiff = pixGetAverageTiled(pixb, sx, sy, type);
        pixGetDimensions(pixrdiff, &w, &h, NULL);
        pixacc = pixaccCreate(w, h, 0);
        pixaccAdd(pixacc, pixrdiff);
        pixaccAdd(pixacc, pixgdiff);
        pixaccAdd(pixacc, pixbdiff);
        pixaccMultConst(pixacc, 1. / 3.);
        *ppixdiff = pixaccFinal(pixacc, 8);
        pixDestroy(&pixr);
        pixDestroy(&pixg);
        pixDestroy(&pixb);
        pixDestroy(&pixrdiff);
        pixDestroy(&pixgdiff);
        pixDestroy(&pixbdiff);
        pixaccDestroy(&pixacc);
    }
    pixDestroy(&pixt);
    return 0;
}


/*------------------------------------------------------------------*
 *            Other measures of the difference of two images        *
 *------------------------------------------------------------------*/
/*!
 *  pixCompareRankDifference()
 *
 *      Input:  pix1 (8 bpp gray or 32 bpp rgb, or colormapped)
 *              pix2 (8 bpp gray or 32 bpp rgb, or colormapped)
 *              factor (subsampling factor; use 0 or 1 for no subsampling)
 *      Return: narank (numa of rank difference), or null on error
 *
 *  Notes:
 *      (1) This answers the question: if the pixel values in each
 *          component are compared by absolute difference, for
 *          any value of difference, what is the fraction of
 *          pixel pairs that have a difference of this magnitude
 *          or greater.  For a difference of 0, the fraction is 1.0.
 *          In this sense, it is a mapping from pixel difference to
 *          rank order of difference.
 *      (2) The two images are aligned at the UL corner, and do not
 *          need to be the same size.  If they are not the same size,
 *          the comparison will be made over overlapping pixels.
 *      (3) If there is a colormap, it is removed and the result
 *          is either gray or RGB depending on the colormap.
 *      (4) If RGB, pixel differences for each component are aggregated
 *          into a single histogram.
 */
NUMA *
pixCompareRankDifference(PIX     *pix1,
                         PIX     *pix2,
                         l_int32  factor)
{
l_int32     i;
l_float32  *array1, *array2;
NUMA       *nah, *nan, *nad;

    PROCNAME("pixCompareRankDifference");

    if (!pix1)
        return (NUMA *)ERROR_PTR("pix1 not defined", procName, NULL);
    if (!pix2)
        return (NUMA *)ERROR_PTR("pix2 not defined", procName, NULL);

    if ((nah = pixGetDifferenceHistogram(pix1, pix2, factor)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);

    nan = numaNormalizeHistogram(nah, 1.0);
    array1 = numaGetFArray(nan, L_NOCOPY);

    nad = numaCreate(256);
    numaSetCount(nad, 256);  /* all initialized to 0.0 */
    array2 = numaGetFArray(nad, L_NOCOPY);

        /* Do rank accumulation on normalized histo of diffs */
    array2[0] = 1.0;
    for (i = 1; i < 256; i++)
        array2[i] = array2[i - 1] - array1[i - 1];

    numaDestroy(&nah);
    numaDestroy(&nan);
    return nad;
}


/*!
 *  pixTestForSimilarity()
 *
 *      Input:  pix1 (8 bpp gray or 32 bpp rgb, or colormapped)
 *              pix2 (8 bpp gray or 32 bpp rgb, or colormapped)
 *              factor (subsampling factor; use 0 or 1 for no subsampling)
 *              mindiff (minimum pixel difference to be counted; > 0)
 *              maxfract (maximum fraction of pixels allowed to have
 *                        diff greater than or equal to mindiff)
 *              maxave (maximum average difference of pixels allowed for
 *                      pixels with diff greater than or equal to mindiff,
 *                      after subtracting mindiff)
 *              &similar (<return> 1 if similar, 0 otherwise)
 *              printstats (use 1 to print normalized histogram to stderr)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This takes 2 pix that are the same size and determines using
 *          3 input parameters if they are "similar".  The first parameter
 *          @mindiff establishes a criterion of pixel-to-pixel similarity:
 *          two pixels are not similar if their difference in value is
 *          at least mindiff.  Then @maxfract and @maxave are thresholds
 *          on the number and distribution of dissimilar pixels
 *          allowed for the two pix to be similar.   If the pix are
 *          to be similar, neither threshold can be exceeded.
 *      (2) In setting the @maxfract and @maxave thresholds, you have
 *          these options:
 *            (a) Base the comparison only on @maxfract.  Then set
 *                @maxave = 0.0 or 256.0.  (If 0, we always ignore it.)
 *            (b) Base the comparison only on @maxave.  Then set
 *                @maxfract = 1.0.
 *            (c) Base the comparison on both thresholds.
 *      (3) Example of values that can be expected at mindiff = 15 when
 *          comparing lossless png encoding with jpeg encoding, q=75:
 *             (smoothish bg)       fractdiff = 0.01, avediff = 2.5
 *             (natural scene)      fractdiff = 0.13, avediff = 3.5
 *          To identify these images as 'similar', select maxfract
 *          and maxave to be upper bounds of what you expect.
 *      (4) See pixGetDifferenceStats() for a discussion of why we subtract
 *          mindiff from the computed average diff of the nonsimilar pixels
 *          to get the 'avediff' returned by that function.
 *      (5) If there is a colormap, it is removed and the result
 *          is either gray or RGB depending on the colormap.
 *      (6) If RGB, the maximum difference between pixel components is
 *          saved in the histogram.
 */
l_int32
pixTestForSimilarity(PIX       *pix1,
                     PIX       *pix2,
                     l_int32    factor,
                     l_int32    mindiff,
                     l_float32  maxfract,
                     l_float32  maxave,
                     l_int32   *psimilar,
                     l_int32    printstats)
{
l_float32   fractdiff, avediff;

    PROCNAME("pixTestForSimilarity");

    if (!psimilar)
        return ERROR_INT("&similar not defined", procName, 1);
    *psimilar = 0;
    if (!pix1)
        return ERROR_INT("pix1 not defined", procName, 1);
    if (!pix2)
        return ERROR_INT("pix2 not defined", procName, 1);
    if (pixSizesEqual(pix1, pix2) == 0)
        return ERROR_INT("pix sizes not equal", procName, 1);
    if (mindiff <= 0)
        return ERROR_INT("mindiff must be > 0", procName, 1);

    if (pixGetDifferenceStats(pix1, pix2, factor, mindiff,
                              &fractdiff, &avediff, printstats))
        return ERROR_INT("diff stats not found", procName, 1);

    if (maxave <= 0.0) maxave = 256.0;
    if (fractdiff <= maxfract && avediff <= maxave)
        *psimilar = 1;
    return 0;
}


/*!
 *  pixGetDifferenceStats()
 *
 *      Input:  pix1 (8 bpp gray or 32 bpp rgb, or colormapped)
 *              pix2 (8 bpp gray or 32 bpp rgb, or colormapped)
 *              factor (subsampling factor; use 0 or 1 for no subsampling)
 *              mindiff (minimum pixel difference to be counted; > 0)
 *              &fractdiff (<return> fraction of pixels with diff greater
 *                          than or equal to mindiff)
 *              &avediff (<return> average difference of pixels with diff
 *                        greater than or equal to mindiff, less mindiff)
 *              printstats (use 1 to print normalized histogram to stderr)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This takes a threshold @mindiff and describes the difference
 *          between two images in terms of two numbers:
 *            (a) the fraction of pixels, @fractdiff, whose difference
 *                equals or exceeds the threshold @mindiff, and
 *            (b) the average value @avediff of the difference in pixel value
 *                for the pixels in the set given by (a), after you subtract
 *                @mindiff.  The reason for subtracting @mindiff is that
 *                you then get a useful measure for the rate of falloff
 *                of the distribution for larger differences.  For example,
 *                if @mindiff = 10 and you find that @avediff = 2.5, it
 *                says that of the pixels with diff > 10, the average of
 *                their diffs is just mindiff + 2.5 = 12.5.  This is a
 *                fast falloff in the histogram with increasing difference.
 *      (2) The two images are aligned at the UL corner, and do not
 *          need to be the same size.  If they are not the same size,
 *          the comparison will be made over overlapping pixels.
 *      (3) If there is a colormap, it is removed and the result
 *          is either gray or RGB depending on the colormap.
 *      (4) If RGB, the maximum difference between pixel components is
 *          saved in the histogram.
 */
l_int32
pixGetDifferenceStats(PIX        *pix1,
                      PIX        *pix2,
                      l_int32     factor,
                      l_int32     mindiff,
                      l_float32  *pfractdiff,
                      l_float32  *pavediff,
                      l_int32     printstats)
{
l_int32     i, first, last, diff;
l_float32   fract, ave;
l_float32  *array;
NUMA       *nah, *nan, *nac;

    PROCNAME("pixGetDifferenceStats");

    if (!pfractdiff)
        return ERROR_INT("&fractdiff not defined", procName, 1);
    *pfractdiff = 0.0;
    if (!pavediff)
        return ERROR_INT("&avediff not defined", procName, 1);
    *pavediff = 0.0;
    if (!pix1)
        return ERROR_INT("pix1 not defined", procName, 1);
    if (!pix2)
        return ERROR_INT("pix2 not defined", procName, 1);
    if (mindiff <= 0)
        return ERROR_INT("mindiff must be > 0", procName, 1);

    if ((nah = pixGetDifferenceHistogram(pix1, pix2, factor)) == NULL)
        return ERROR_INT("na not made", procName, 1);

    if ((nan = numaNormalizeHistogram(nah, 1.0)) == NULL) {
        numaDestroy(&nah);
        return ERROR_INT("nan not made", procName, 1);
    }
    array = numaGetFArray(nan, L_NOCOPY);

    if (printstats) {
        numaGetNonzeroRange(nan, 0.0, &first, &last);
        nac = numaClipToInterval(nan, first, last);
        fprintf(stderr, "\nNonzero values in normalized histogram:");
        numaWriteStream(stderr, nac);
        numaDestroy(&nac);
        fprintf(stderr, " Mindiff      fractdiff      avediff\n");
        fprintf(stderr, " -----------------------------------\n");
        for (diff = 1; diff < L_MIN(2 * mindiff, last); diff++) {
            fract = 0.0;
            ave = 0.0;
            for (i = diff; i <= last; i++) {
                fract += array[i];
                ave += (l_float32)i * array[i];
            }
            ave = (fract == 0.0) ? 0.0 : ave / fract;
            ave -= diff;
            fprintf(stderr, "%5d         %7.4f        %7.4f\n",
                    diff, fract, ave);
        }
        fprintf(stderr, " -----------------------------------\n");
    }

    fract = 0.0;
    ave = 0.0;
    for (i = mindiff; i < 256; i++) {
      fract += array[i];
      ave += (l_float32)i * array[i];
    }
    ave = (fract == 0.0) ? 0.0 : ave / fract;
    ave -= mindiff;

    *pfractdiff = fract;
    *pavediff = ave;

    numaDestroy(&nah);
    numaDestroy(&nan);
    return 0;
}


/*!
 *  pixGetDifferenceHistogram()
 *
 *      Input:  pix1 (8 bpp gray or 32 bpp rgb, or colormapped)
 *              pix2 (8 bpp gray or 32 bpp rgb, or colormapped)
 *              factor (subsampling factor; use 0 or 1 for no subsampling)
 *      Return: na (Numa of histogram of differences), or null on error
 *
 *  Notes:
 *      (1) The two images are aligned at the UL corner, and do not
 *          need to be the same size.  If they are not the same size,
 *          the comparison will be made over overlapping pixels.
 *      (2) If there is a colormap, it is removed and the result
 *          is either gray or RGB depending on the colormap.
 *      (3) If RGB, the maximum difference between pixel components is
 *          saved in the histogram.
 */
NUMA *
pixGetDifferenceHistogram(PIX     *pix1,
                          PIX     *pix2,
                          l_int32  factor)
{
l_int32     w1, h1, d1, w2, h2, d2, w, h, wpl1, wpl2;
l_int32     i, j, val, val1, val2;
l_int32     rval1, rval2, gval1, gval2, bval1, bval2;
l_int32     rdiff, gdiff, bdiff, maxdiff;
l_uint32   *data1, *data2, *line1, *line2;
l_float32  *array;
NUMA       *na;
PIX        *pixt1, *pixt2;

    PROCNAME("pixGetDifferenceHistogram");

    if (!pix1)
        return (NUMA *)ERROR_PTR("pix1 not defined", procName, NULL);
    if (!pix2)
        return (NUMA *)ERROR_PTR("pix2 not defined", procName, NULL);
    d1 = pixGetDepth(pix1);
    d2 = pixGetDepth(pix2);
    if (d1 == 16 || d2 == 16)
        return (NUMA *)ERROR_PTR("d == 16 not supported", procName, NULL);
    if (d1 < 8 && !pixGetColormap(pix1))
        return (NUMA *)ERROR_PTR("pix1 depth < 8 bpp and not cmapped",
                                 procName, NULL);
    if (d2 < 8 && !pixGetColormap(pix2))
        return (NUMA *)ERROR_PTR("pix2 depth < 8 bpp and not cmapped",
                                 procName, NULL);
    pixt1 = pixRemoveColormap(pix1, REMOVE_CMAP_BASED_ON_SRC);
    pixt2 = pixRemoveColormap(pix2, REMOVE_CMAP_BASED_ON_SRC);
    pixGetDimensions(pixt1, &w1, &h1, &d1);
    pixGetDimensions(pixt2, &w2, &h2, &d2);
    if (d1 != d2) {
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        return (NUMA *)ERROR_PTR("pix depths not equal", procName, NULL);
    }
    if (factor < 1) factor = 1;

    na = numaCreate(256);
    numaSetCount(na, 256);  /* all initialized to 0.0 */
    array = numaGetFArray(na, L_NOCOPY);
    w = L_MIN(w1, w2);
    h = L_MIN(h1, h2);
    data1 = pixGetData(pixt1);
    data2 = pixGetData(pixt2);
    wpl1 = pixGetWpl(pixt1);
    wpl2 = pixGetWpl(pixt2);
    if (d1 == 8) {
        for (i = 0; i < h; i += factor) {
            line1 = data1 + i * wpl1;
            line2 = data2 + i * wpl2;
            for (j = 0; j < w; j += factor) {
                val1 = GET_DATA_BYTE(line1, j);
                val2 = GET_DATA_BYTE(line2, j);
                val = L_ABS(val1 - val2);
                array[val]++;
            }
        }
    } else {  /* d1 == 32 */
        for (i = 0; i < h; i += factor) {
            line1 = data1 + i * wpl1;
            line2 = data2 + i * wpl2;
            for (j = 0; j < w; j += factor) {
                extractRGBValues(line1[j], &rval1, &gval1, &bval1);
                extractRGBValues(line2[j], &rval2, &gval2, &bval2);
                rdiff = L_ABS(rval1 - rval2);
                gdiff = L_ABS(gval1 - gval2);
                bdiff = L_ABS(bval1 - bval2);
                maxdiff = L_MAX(rdiff, gdiff);
                maxdiff = L_MAX(maxdiff, bdiff);
                array[maxdiff]++;
            }
        }
    }

    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    return na;
}


/*!
 *  pixGetPerceptualDiff()
 *
 *      Input:  pix1 (8 bpp gray or 32 bpp rgb, or colormapped)
 *              pix2 (8 bpp gray or 32 bpp rgb, or colormapped)
 *              sampling (subsampling factor; use 0 or 1 for no subsampling)
 *              dilation (size of grayscale or color Sel; odd)
 *              mindiff (minimum pixel difference to be counted; > 0)
 *              &fract (<return> fraction of pixels with diff greater than
 *                      mindiff)
 *              &pixdiff1 (<optional return> showing difference (gray or color))
 *              &pixdiff2 (<optional return> showing pixels of sufficient diff)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This takes 2 pix and determines, using 2 input parameters:
 *           * @dilation specifies the amount of grayscale or color
 *             dilation to apply to the images, to compensate for
 *             a small amount of misregistration.  A typical number might
 *             be 5, which uses a 5x5 Sel.  Grayscale dilation expands
 *             lighter pixels into darker pixel regions.
 *           * @mindiff determines the threshold on the difference in
 *             pixel values to be counted -- two pixels are not similar
 *             if their difference in value is at least @mindiff.  For
 *             color pixels, we use the maximum component difference.
 *      (2) The pixelwise comparison is always done with the UL corners
 *          aligned.  The sizes of pix1 and pix2 need not be the same,
 *          although in practice it can be useful to scale to the same size.
 *      (3) If there is a colormap, it is removed and the result
 *          is either gray or RGB depending on the colormap.
 *      (4) Two optional diff images can be retrieved (typ. for debugging):
 *           pixdiff1: the gray or color difference
 *           pixdiff2: thresholded to 1 bpp for pixels exceeding @mindiff
 *      (5) The returned value of fract can be compared to some threshold,
 *          which is application dependent.
 *      (6) This method is in analogy to the two-sided hausdorff transform,
 *          except here it is for d > 1.  For d == 1 (see pixRankHaustest()),
 *          we verify that when one pix1 is dilated, it covers at least a
 *          given fraction of the pixels in pix2, and v.v.; in that
 *          case, the two pix are sufficiently similar.  Here, we
 *          do an analogous thing: subtract the dilated pix1 from pix2 to
 *          get a 1-sided hausdorff-like transform.  Then do it the
 *          other way.  Take the component-wise max of the two results,
 *          and threshold to get the fraction of pixels with a difference
 *          below the threshold.
 */
l_int32
pixGetPerceptualDiff(PIX        *pixs1,
                     PIX        *pixs2,
                     l_int32     sampling,
                     l_int32     dilation,
                     l_int32     mindiff,
                     l_float32  *pfract,
                     PIX       **ppixdiff1,
                     PIX       **ppixdiff2)
{
l_int32  d1, d2, w, h, count;
PIX     *pix1, *pix2, *pix3, *pix4, *pix5, *pix6, *pix7, *pix8, *pix9;
PIX     *pix10, *pix11;

    PROCNAME("pixGetPerceptualDiff");

    if (!pfract)
        return ERROR_INT("&fract not defined", procName, 1);
    *pfract = 1.0;  /* init to completely different */
    if (ppixdiff1) *ppixdiff1 = NULL;
    if (ppixdiff2) *ppixdiff2 = NULL;
    if ((dilation & 1) == 0)
        return ERROR_INT("dilation must be odd", procName, 1);
    if (!pixs1)
        return ERROR_INT("pixs1 not defined", procName, 1);
    if (!pixs2)
        return ERROR_INT("pixs2 not defined", procName, 1);
    d1 = pixGetDepth(pixs1);
    d2 = pixGetDepth(pixs2);
    if (!pixGetColormap(pixs1) && d1 < 8)
        return ERROR_INT("pixs1 not cmapped or >=8 bpp", procName, 1);
    if (!pixGetColormap(pixs2) && d2 < 8)
        return ERROR_INT("pixs2 not cmapped or >=8 bpp", procName, 1);

        /* Integer downsample if requested */
    if (sampling > 1) {
        pix1 = pixScaleByIntSubsampling(pixs1, sampling);
        pix2 = pixScaleByIntSubsampling(pixs2, sampling);
    } else {
        pix1 = pixClone(pixs1);
        pix2 = pixClone(pixs2);
    }

        /* Remove colormaps */
    if (pixGetColormap(pix1)) {
        pix3 = pixRemoveColormap(pix1, REMOVE_CMAP_BASED_ON_SRC);
        d1 = pixGetDepth(pix3);
    } else {
        pix3 = pixClone(pix1);
    }
    if (pixGetColormap(pix2)) {
        pix4 = pixRemoveColormap(pix2, REMOVE_CMAP_BASED_ON_SRC);
        d2 = pixGetDepth(pix4);
    } else {
        pix4 = pixClone(pix2);
    }
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    if (d1 != d2) {
        pixDestroy(&pix3);
        pixDestroy(&pix4);
        return ERROR_INT("pix3 and pix4 depths not equal", procName, 1);
    }

        /* In each direction, do a small dilation and subtract the dilated
         * image from the other image to get a one-sided difference.
         * Then take the max of the differences for each direction
         * and clipping each component to 255 if necessary.  Note that
         * for RGB images, the dilations and max selection are done
         * component-wise, and the conversion to grayscale also uses the
         * maximum component.  The resulting grayscale images are
         * thresholded using @mindiff. */
    if (d1 == 8) {
        pix5 = pixDilateGray(pix3, dilation, dilation);
        pixCompareGray(pix4, pix5, L_COMPARE_SUBTRACT, 0, NULL, NULL, NULL,
                       &pix7);
        pix6 = pixDilateGray(pix4, dilation, dilation);
        pixCompareGray(pix3, pix6, L_COMPARE_SUBTRACT, 0, NULL, NULL, NULL,
                       &pix8);
        pix9 = pixMinOrMax(NULL, pix7, pix8, L_CHOOSE_MAX);
        pix10 = pixThresholdToBinary(pix9, mindiff);
        pixInvert(pix10, pix10);
        pixCountPixels(pix10, &count, NULL);
        pixGetDimensions(pix10, &w, &h, NULL);
        *pfract = (l_float32)count / (l_float32)(w * h);
        pixDestroy(&pix5);
        pixDestroy(&pix6);
        pixDestroy(&pix7);
        pixDestroy(&pix8);
        if (ppixdiff1)
            *ppixdiff1 = pix9;
        else
            pixDestroy(&pix9);
        if (ppixdiff2)
            *ppixdiff2 = pix10;
        else
            pixDestroy(&pix10);
    } else {  /* d1 == 32 */
        pix5 = pixColorMorph(pix3, L_MORPH_DILATE, dilation, dilation);
        pixCompareRGB(pix4, pix5, L_COMPARE_SUBTRACT, 0, NULL, NULL, NULL,
                       &pix7);
        pix6 = pixColorMorph(pix4, L_MORPH_DILATE, dilation, dilation);
        pixCompareRGB(pix3, pix6, L_COMPARE_SUBTRACT, 0, NULL, NULL, NULL,
                      &pix8);
        pix9 = pixMinOrMax(NULL, pix7, pix8, L_CHOOSE_MAX);
        pix10 = pixConvertRGBToGrayMinMax(pix9, L_CHOOSE_MAX);
        pix11 = pixThresholdToBinary(pix10, mindiff);
        pixInvert(pix11, pix11);
        pixCountPixels(pix11, &count, NULL);
        pixGetDimensions(pix11, &w, &h, NULL);
        *pfract = (l_float32)count / (l_float32)(w * h);
        pixDestroy(&pix5);
        pixDestroy(&pix6);
        pixDestroy(&pix7);
        pixDestroy(&pix8);
        pixDestroy(&pix10);
        if (ppixdiff1)
            *ppixdiff1 = pix9;
        else
            pixDestroy(&pix9);
        if (ppixdiff2)
            *ppixdiff2 = pix11;
        else
            pixDestroy(&pix11);

    }
    pixDestroy(&pix3);
    pixDestroy(&pix4);
    return 0;
}


/*!
 *  pixGetPSNR()
 *
 *      Input:  pix1, pix2 (8 or 32 bpp; no colormap)
 *              factor (sampling factor; >= 1)
 *              &psnr (<return> power signal/noise ratio difference)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This computes the power S/N ratio, in dB, for the difference
 *          between two images.  By convention, the power S/N
 *          for a grayscale image is ('log' == log base 10,
 *          and 'ln == log base e):
 *            PSNR = 10 * log((255/MSE)^2)
 *                 = 4.3429 * ln((255/MSE)^2)
 *                 = -4.3429 * ln((MSE/255)^2)
 *          where MSE is the mean squared error.
 *          Here are some examples:
 *             MSE             PSNR
 *             ---             ----
 *             10              28.1
 *             3               38.6
 *             1               48.1
 *             0.1             68.1
 *      (2) If pix1 and pix2 have the same pixel values, the MSE = 0.0
 *          and the PSNR is infinity.  For that case, this returns
 *          PSNR = 1000, which corresponds to the very small MSE of
 *          about 10^(-48).
 */
l_int32
pixGetPSNR(PIX        *pix1,
           PIX        *pix2,
           l_int32     factor,
           l_float32  *ppsnr)
{
l_int32    same, i, j, w, h, d, wpl1, wpl2, v1, v2, r1, g1, b1, r2, g2, b2;
l_uint32  *data1, *data2, *line1, *line2;
l_float32  mse;  /* mean squared error */

    PROCNAME("pixGetPSNR");

    if (!ppsnr)
        return ERROR_INT("&psnr not defined", procName, 1);
    *ppsnr = 0.0;
    if (!pix1 || !pix2)
        return ERROR_INT("empty input pix", procName, 1);
    if (!pixSizesEqual(pix1, pix2))
        return ERROR_INT("pix sizes unequal", procName, 1);
    if (pixGetColormap(pix1))
        return ERROR_INT("pix1 has colormap", procName, 1);
    if (pixGetColormap(pix2))
        return ERROR_INT("pix2 has colormap", procName, 1);
    pixGetDimensions(pix1, &w, &h, &d);
    if (d != 8 && d != 32)
        return ERROR_INT("pix not 8 or 32 bpp", procName, 1);
    if (factor < 1)
        return ERROR_INT("invalid sampling factor", procName, 1);

    pixEqual(pix1, pix2, &same);
    if (same) {
        *ppsnr = 1000.0;  /* crazy big exponent */
        return 0;
    }

    data1 = pixGetData(pix1);
    data2 = pixGetData(pix2);
    wpl1 = pixGetWpl(pix1);
    wpl2 = pixGetWpl(pix2);
    mse = 0.0;
    if (d == 8) {
        for (i = 0; i < h; i += factor) {
            line1 = data1 + i * wpl1;
            line2 = data2 + i * wpl2;
            for (j = 0; j < w; j += factor) {
                v1 = GET_DATA_BYTE(line1, j);
                v2 = GET_DATA_BYTE(line2, j);
                mse += (v1 - v2) * (v1 - v2);
            }
        }
    } else {  /* d == 32 */
        for (i = 0; i < h; i += factor) {
            line1 = data1 + i * wpl1;
            line2 = data2 + i * wpl2;
            for (j = 0; j < w; j += factor) {
                extractRGBValues(line1[j], &r1, &g1, &b1);
                extractRGBValues(line2[j], &r2, &g2, &b2);
                mse += ((r1 - r2) * (r1 - r2) +
                        (g1 - g2) * (g1 - g2) +
                        (b1 - b2) * (b1 - b2)) / 3.0;
            }
        }
    }
    mse = mse / (w * h);

    *ppsnr = -4.3429448 * log(mse / (255 * 255));
    return 0;
}


/*------------------------------------------------------------------*
 *             Translated images at the same resolution             *
 *------------------------------------------------------------------*/
/*!
 *  pixCompareWithTranslation()
 *
 *      Input:  pix1, pix2 (any depth; colormap OK)
 *              thresh (threshold for converting to 1 bpp)
 *              &delx (<return> x translation on pix2 to align with pix1)
 *              &dely (<return> y translation on pix2 to align with pix1)
 *              &score (<return> correlation score at best alignment)
 *              debugflag (1 for debug output; 0 for no debugging)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This does a coarse-to-fine search for best translational
 *          alignment of two images, measured by a scoring function
 *          that is the correlation between the fg pixels.
 *      (2) The threshold is used if the images aren't 1 bpp.
 *      (3) With debug on, you get a pdf that shows, as a grayscale
 *          image, the score as a function of shift from the initial
 *          estimate, for each of the four levels.  The shift is 0 at
 *          the center of the image.
 *      (4) With debug on, you also get a pdf that shows the
 *          difference at the best alignment between the two images,
 *          at each of the four levels.  The red and green pixels
 *          show locations where one image has a fg pixel and the
 *          other doesn't.  The black pixels are where both images
 *          have fg pixels, and white pixels are where neither image
 *          has fg pixels.
 */
l_int32
pixCompareWithTranslation(PIX        *pix1,
                          PIX        *pix2,
                          l_int32     thresh,
                          l_int32    *pdelx,
                          l_int32    *pdely,
                          l_float32  *pscore,
                          l_int32     debugflag)
{
l_uint8   *subtab;
l_int32    i, level, area1, area2, delx, dely;
l_int32    etransx, etransy, maxshift, dbint;
l_int32   *stab, *ctab;
l_float32  cx1, cx2, cy1, cy2, score;
PIX       *pixb1, *pixb2, *pixt1, *pixt2, *pixt3, *pixt4;
PIXA      *pixa1, *pixa2, *pixadb;

    PROCNAME("pixCompareWithTranslation");

    if (!pdelx || !pdely)
        return ERROR_INT("&delx and &dely not defined", procName, 1);
    *pdelx = *pdely = 0;
    if (!pscore)
        return ERROR_INT("&score not defined", procName, 1);
    *pscore = 0.0;
    if (!pix1)
        return ERROR_INT("pix1 not defined", procName, 1);
    if (!pix2)
        return ERROR_INT("pix2 not defined", procName, 1);

        /* Make tables */
    subtab = makeSubsampleTab2x();
    stab = makePixelSumTab8();
    ctab = makePixelCentroidTab8();

        /* Binarize each image */
    pixb1 = pixConvertTo1(pix1, thresh);
    pixb2 = pixConvertTo1(pix2, thresh);

        /* Make a cascade of 2x reduced images for each, thresholding
         * with level 2 (neutral), down to 8x reduction */
    pixa1 = pixaCreate(4);
    pixa2 = pixaCreate(4);
    if (debugflag)
        pixadb = pixaCreate(4);
    pixaAddPix(pixa1, pixb1, L_INSERT);
    pixaAddPix(pixa2, pixb2, L_INSERT);
    for (i = 0; i < 3; i++) {
        pixt1 = pixReduceRankBinary2(pixb1, 2, subtab);
        pixt2 = pixReduceRankBinary2(pixb2, 2, subtab);
        pixaAddPix(pixa1, pixt1, L_INSERT);
        pixaAddPix(pixa2, pixt2, L_INSERT);
        pixb1 = pixt1;
        pixb2 = pixt2;
    }

        /* At the lowest level, use the centroids with a maxshift of 6
         * to search for the best alignment.  Then at higher levels,
         * use the result from the level below as the initial approximation
         * for the alignment, and search with a maxshift of 2. */
    for (level = 3; level >= 0; level--) {
        pixt1 = pixaGetPix(pixa1, level, L_CLONE);
        pixt2 = pixaGetPix(pixa2, level, L_CLONE);
        pixCountPixels(pixt1, &area1, stab);
        pixCountPixels(pixt2, &area2, stab);
        if (level == 3) {
            pixCentroid(pixt1, ctab, stab, &cx1, &cy1);
            pixCentroid(pixt2, ctab, stab, &cx2, &cy2);
            etransx = lept_roundftoi(cx1 - cx2);
            etransy = lept_roundftoi(cy1 - cy2);
            maxshift = 6;
        } else {
            etransx = 2 * delx;
            etransy = 2 * dely;
            maxshift = 2;
        }
        dbint = (debugflag) ? level + 1 : 0;
        pixBestCorrelation(pixt1, pixt2, area1, area2, etransx, etransy,
                           maxshift, stab, &delx, &dely, &score, dbint);
        if (debugflag) {
            fprintf(stderr, "Level %d: delx = %d, dely = %d, score = %7.4f\n",
                    level, delx, dely, score);
            pixRasteropIP(pixt2, delx, dely, L_BRING_IN_WHITE);
            pixt3 = pixDisplayDiffBinary(pixt1, pixt2);
            pixt4 = pixExpandReplicate(pixt3, 8 / (1 << (3 - level)));
            pixaAddPix(pixadb, pixt4, L_INSERT);
            pixDestroy(&pixt3);
        }
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }

    if (debugflag) {
        pixaConvertToPdf(pixadb, 300, 1.0, L_FLATE_ENCODE, 0, NULL,
                         "/tmp/cmp.pdf");
        convertFilesToPdf("/tmp", "correl_", 30, 1.0, L_FLATE_ENCODE,
                          0, "Correlation scores at levels 1 through 5",
                          "/tmp/correl.pdf");
        pixaDestroy(&pixadb);
    }

    *pdelx = delx;
    *pdely = dely;
    *pscore = score;
    pixaDestroy(&pixa1);
    pixaDestroy(&pixa2);
    FREE(subtab);
    FREE(stab);
    FREE(ctab);
    return 0;
}


/*!
 *  pixBestCorrelation()
 *
 *      Input:  pix1   (1 bpp)
 *              pix2   (1 bpp)
 *              area1  (number of on pixels in pix1)
 *              area2  (number of on pixels in pix2)
 *              etransx (estimated x translation of pix2 to align with pix1)
 *              etransy (estimated y translation of pix2 to align with pix1)
 *              maxshift  (max x and y shift of pix2, around the estimated
 *                          alignment location, relative to pix1)
 *              tab8 (<optional> sum tab for ON pixels in byte; can be NULL)
 *              &delx (<optional return> best x shift of pix2 relative to pix1
 *              &dely (<optional return> best y shift of pix2 relative to pix1
 *              &score (<optional return> maximum score found; can be NULL)
 *              debugflag (<= 0 to skip; positive to generate output.
 *                         The integer is used to label the debug image.)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This maximizes the correlation score between two 1 bpp images,
 *          by starting with an estimate of the alignment
 *          (@etransx, @etransy) and computing the correlation around this.
 *          It optionally returns the shift (@delx, @dely) that maximizes
 *          the correlation score when pix2 is shifted by this amount
 *          relative to pix1.
 *      (2) Get the centroids of pix1 and pix2, using pixCentroid(),
 *          to compute (@etransx, @etransy).  Get the areas using
 *          pixCountPixels().
 *      (3) The centroid of pix2 is shifted with respect to the centroid
 *          of pix1 by all values between -maxshiftx and maxshiftx,
 *          and likewise for the y shifts.  Therefore, the number of
 *          correlations computed is:
 *               (2 * maxshiftx + 1) * (2 * maxshifty + 1)
 *          Consequently, if pix1 and pix2 are large, you should do this
 *          in a coarse-to-fine sequence.  See the use of this function
 *          in pixCompareWithTranslation().
 */
l_int32
pixBestCorrelation(PIX        *pix1,
                   PIX        *pix2,
                   l_int32     area1,
                   l_int32     area2,
                   l_int32     etransx,
                   l_int32     etransy,
                   l_int32     maxshift,
                   l_int32    *tab8,
                   l_int32    *pdelx,
                   l_int32    *pdely,
                   l_float32  *pscore,
                   l_int32     debugflag)
{
l_int32    shiftx, shifty, delx, dely;
l_int32   *tab;
l_float32  maxscore, score;
FPIX      *fpix;
PIX       *pixt1, *pixt2;

    PROCNAME("pixBestCorrelation");

    if (pdelx) *pdelx = 0;
    if (pdely) *pdely = 0;
    if (pscore) *pscore = 0.0;
    if (!pix1 || pixGetDepth(pix1) != 1)
        return ERROR_INT("pix1 not defined or not 1 bpp", procName, 1);
    if (!pix2 || pixGetDepth(pix2) != 1)
        return ERROR_INT("pix2 not defined or not 1 bpp", procName, 1);
    if (!area1 || !area2)
        return ERROR_INT("areas must be > 0", procName, 1);

    if (debugflag > 0)
        fpix = fpixCreate(2 * maxshift + 1, 2 * maxshift + 1);

    if (!tab8)
        tab = makePixelSumTab8();
    else
        tab = tab8;

        /* Search over a set of {shiftx, shifty} for the max */
    maxscore = 0;
    delx = etransx;
    dely = etransy;
    for (shifty = -maxshift; shifty <= maxshift; shifty++) {
        for (shiftx = -maxshift; shiftx <= maxshift; shiftx++) {
            pixCorrelationScoreShifted(pix1, pix2, area1, area2,
                                       etransx + shiftx,
                                       etransy + shifty, tab, &score);
            if (debugflag > 0) {
                fpixSetPixel(fpix, maxshift + shiftx, maxshift + shifty,
                             1000.0 * score);
/*                fprintf(stderr, "(sx, sy) = (%d, %d): score = %6.4f\n",
                        shiftx, shifty, score); */
            }
            if (score > maxscore) {
                maxscore = score;
                delx = etransx + shiftx;
                dely = etransy + shifty;
            }
        }
    }

    if (debugflag > 0) {
        char  buf[128];
        pixt1 = fpixDisplayMaxDynamicRange(fpix);
        pixt2 = pixExpandReplicate(pixt1, 20);
        snprintf(buf, sizeof(buf), "/tmp/correl_%d.png", debugflag);
        pixWrite(buf, pixt2, IFF_PNG);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        fpixDestroy(&fpix);
    }

    if (pdelx) *pdelx = delx;
    if (pdely) *pdely = dely;
    if (pscore) *pscore = maxscore;
    if (!tab8) FREE(tab);
    return 0;
}
