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
 *  convolve.c
 *
 *      Top level grayscale or color block convolution
 *          PIX       *pixBlockconv()
 *
 *      Grayscale block convolution
 *          PIX       *pixBlockconvGray()
 *
 *      Accumulator for 1, 8 and 32 bpp convolution
 *          PIX       *pixBlockconvAccum()
 *
 *      Un-normalized grayscale block convolution
 *          PIX       *pixBlockconvGrayUnnormalized()
 *
 *      Tiled grayscale or color block convolution
 *          PIX       *pixBlockconvTiled()
 *          PIX       *pixBlockconvGrayTile()
 *
 *      Convolution for mean, mean square, variance and rms deviation
 *      in specified window
 *          l_int32    pixWindowedStats()
 *          PIX       *pixWindowedMean()
 *          PIX       *pixWindowedMeanSquare()
 *          l_int32    pixWindowedVariance()
 *          DPIX      *pixMeanSquareAccum()
 *
 *      Binary block sum and rank filter
 *          PIX       *pixBlockrank()
 *          PIX       *pixBlocksum()
 *
 *      Census transform
 *          PIX       *pixCensusTransform()
 *
 *      Generic convolution (with Pix)
 *          PIX       *pixConvolve()
 *          PIX       *pixConvolveSep()
 *          PIX       *pixConvolveRGB()
 *          PIX       *pixConvolveRGBSep()
 *
 *      Generic convolution (with float arrays)
 *          FPIX      *fpixConvolve()
 *          FPIX      *fpixConvolveSep()
 *
 *      Convolution with bias (for non-negative output)
 *          PIX       *pixConvolveWithBias()
 *
 *      Set parameter for convolution subsampling
 *          void       l_setConvolveSampling()
 *
 *      Additive gaussian noise
 *          PIX       *pixAddGaussNoise()
 *          l_float32  gaussDistribSampling()
 */

#include <math.h>
#include "allheaders.h"

    /* These globals determine the subsampling factors for
     * generic convolution of pix and fpix.  Declare extern to use.
     * To change the values, use l_setConvolveSampling(). */
LEPT_DLL l_int32  ConvolveSamplingFactX = 1;
LEPT_DLL l_int32  ConvolveSamplingFactY = 1;

/*----------------------------------------------------------------------*
 *             Top-level grayscale or color block convolution           *
 *----------------------------------------------------------------------*/
/*!
 *  pixBlockconv()
 *
 *      Input:  pix (8 or 32 bpp; or 2, 4 or 8 bpp with colormap)
 *              wc, hc   (half width/height of convolution kernel)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) The full width and height of the convolution kernel
 *          are (2 * wc + 1) and (2 * hc + 1)
 *      (2) Returns a copy if both wc and hc are 0
 *      (3) Require that w >= 2 * wc + 1 and h >= 2 * hc + 1,
 *          where (w,h) are the dimensions of pixs.
 */
PIX  *
pixBlockconv(PIX     *pix,
             l_int32  wc,
             l_int32  hc)
{
l_int32  w, h, d;
PIX     *pixs, *pixd, *pixr, *pixrc, *pixg, *pixgc, *pixb, *pixbc;

    PROCNAME("pixBlockconv");

    if (!pix)
        return (PIX *)ERROR_PTR("pix not defined", procName, NULL);
    if (wc < 0) wc = 0;
    if (hc < 0) hc = 0;
    pixGetDimensions(pix, &w, &h, &d);
    if (w < 2 * wc + 1 || h < 2 * hc + 1) {
        wc = L_MIN(wc, (w - 1) / 2);
        hc = L_MIN(hc, (h - 1) / 2);
        L_WARNING("kernel too large; reducing!\n", procName);
        L_INFO("wc = %d, hc = %d\n", procName, wc, hc);
    }
    if (wc == 0 && hc == 0)   /* no-op */
        return pixCopy(NULL, pix);

        /* Remove colormap if necessary */
    if ((d == 2 || d == 4 || d == 8) && pixGetColormap(pix)) {
        L_WARNING("pix has colormap; removing\n", procName);
        pixs = pixRemoveColormap(pix, REMOVE_CMAP_BASED_ON_SRC);
        d = pixGetDepth(pixs);
    } else {
        pixs = pixClone(pix);
    }

    if (d != 8 && d != 32) {
        pixDestroy(&pixs);
        return (PIX *)ERROR_PTR("depth not 8 or 32 bpp", procName, NULL);
    }

    if (d == 8) {
        pixd = pixBlockconvGray(pixs, NULL, wc, hc);
    } else { /* d == 32 */
        pixr = pixGetRGBComponent(pixs, COLOR_RED);
        pixrc = pixBlockconvGray(pixr, NULL, wc, hc);
        pixDestroy(&pixr);
        pixg = pixGetRGBComponent(pixs, COLOR_GREEN);
        pixgc = pixBlockconvGray(pixg, NULL, wc, hc);
        pixDestroy(&pixg);
        pixb = pixGetRGBComponent(pixs, COLOR_BLUE);
        pixbc = pixBlockconvGray(pixb, NULL, wc, hc);
        pixDestroy(&pixb);
        pixd = pixCreateRGBImage(pixrc, pixgc, pixbc);
        pixDestroy(&pixrc);
        pixDestroy(&pixgc);
        pixDestroy(&pixbc);
    }

    pixDestroy(&pixs);
    return pixd;
}


/*----------------------------------------------------------------------*
 *                     Grayscale block convolution                      *
 *----------------------------------------------------------------------*/
/*!
 *  pixBlockconvGray()
 *
 *      Input:  pix (8 bpp)
 *              accum pix (32 bpp; can be null)
 *              wc, hc   (half width/height of convolution kernel)
 *      Return: pix (8 bpp), or null on error
 *
 *  Notes:
 *      (1) If accum pix is null, make one and destroy it before
 *          returning; otherwise, just use the input accum pix.
 *      (2) The full width and height of the convolution kernel
 *          are (2 * wc + 1) and (2 * hc + 1).
 *      (3) Returns a copy if both wc and hc are 0.
 *      (4) Require that w >= 2 * wc + 1 and h >= 2 * hc + 1,
 *          where (w,h) are the dimensions of pixs.
 */
PIX *
pixBlockconvGray(PIX     *pixs,
                 PIX     *pixacc,
                 l_int32  wc,
                 l_int32  hc)
{
l_int32    w, h, d, wpl, wpla;
l_uint32  *datad, *dataa;
PIX       *pixd, *pixt;

    PROCNAME("pixBlockconvGray");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (wc < 0) wc = 0;
    if (hc < 0) hc = 0;
    if (w < 2 * wc + 1 || h < 2 * hc + 1) {
        wc = L_MIN(wc, (w - 1) / 2);
        hc = L_MIN(hc, (h - 1) / 2);
        L_WARNING("kernel too large; reducing!\n", procName);
        L_INFO("wc = %d, hc = %d\n", procName, wc, hc);
    }
    if (wc == 0 && hc == 0)   /* no-op */
        return pixCopy(NULL, pixs);

    if (pixacc) {
        if (pixGetDepth(pixacc) == 32) {
            pixt = pixClone(pixacc);
        } else {
            L_WARNING("pixacc not 32 bpp; making new one\n", procName);
            if ((pixt = pixBlockconvAccum(pixs)) == NULL)
                return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
        }
    } else {
        if ((pixt = pixBlockconvAccum(pixs)) == NULL)
            return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
    }

    if ((pixd = pixCreateTemplate(pixs)) == NULL) {
        pixDestroy(&pixt);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }

    wpl = pixGetWpl(pixs);
    wpla = pixGetWpl(pixt);
    datad = pixGetData(pixd);
    dataa = pixGetData(pixt);
    blockconvLow(datad, w, h, wpl, dataa, wpla, wc, hc);

    pixDestroy(&pixt);
    return pixd;
}


/*----------------------------------------------------------------------*
 *              Accumulator for 1, 8 and 32 bpp convolution             *
 *----------------------------------------------------------------------*/
/*!
 *  pixBlockconvAccum()
 *
 *      Input:  pixs (1, 8 or 32 bpp)
 *      Return: accum pix (32 bpp), or null on error.
 *
 *  Notes:
 *      (1) The general recursion relation is
 *            a(i,j) = v(i,j) + a(i-1, j) + a(i, j-1) - a(i-1, j-1)
 *          For the first line, this reduces to the special case
 *            a(i,j) = v(i,j) + a(i, j-1)
 *          For the first column, the special case is
 *            a(i,j) = v(i,j) + a(i-1, j)
 */
PIX *
pixBlockconvAccum(PIX  *pixs)
{
l_int32    w, h, d, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixBlockconvAccum");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pixs not 1, 8 or 32 bpp", procName, NULL);
    if ((pixd = pixCreate(w, h, 32)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    blockconvAccumLow(datad, w, h, wpld, datas, d, wpls);

    return pixd;
}


/*----------------------------------------------------------------------*
 *               Un-normalized grayscale block convolution              *
 *----------------------------------------------------------------------*/
/*!
 *  pixBlockconvGrayUnnormalized()
 *
 *      Input:  pixs (8 bpp)
 *              wc, hc   (half width/height of convolution kernel)
 *      Return: pix (32 bpp; containing the convolution without normalizing
 *                   for the window size), or null on error
 *
 *  Notes:
 *      (1) The full width and height of the convolution kernel
 *          are (2 * wc + 1) and (2 * hc + 1).
 *      (2) Require that w >= 2 * wc + 1 and h >= 2 * hc + 1,
 *          where (w,h) are the dimensions of pixs.
 *      (3) Returns a copy if both wc and hc are 0.
 *      (3) Adds mirrored border to avoid treating the boundary pixels
 *          specially.  Note that we add wc + 1 pixels to the left
 *          and wc to the right.  The added width is 2 * wc + 1 pixels,
 *          and the particular choice simplifies the indexing in the loop.
 *          Likewise, add hc + 1 pixels to the top and hc to the bottom.
 *      (4) To get the normalized result, divide by the area of the
 *          convolution kernel: (2 * wc + 1) * (2 * hc + 1)
 *          Specifically, do this:
 *               pixc = pixBlockconvGrayUnnormalized(pixs, wc, hc);
 *               fract = 1. / ((2 * wc + 1) * (2 * hc + 1));
 *               pixMultConstantGray(pixc, fract);
 *               pixd = pixGetRGBComponent(pixc, L_ALPHA_CHANNEL);
 *      (5) Unlike pixBlockconvGray(), this always computes the accumulation
 *          pix because its size is tied to wc and hc.
 *      (6) Compare this implementation with pixBlockconvGray(), where
 *          most of the code in blockconvLow() is special casing for
 *          efficiently handling the boundary.  Here, the use of
 *          mirrored borders and destination indexing makes the
 *          implementation very simple.
 */
PIX *
pixBlockconvGrayUnnormalized(PIX     *pixs,
                             l_int32  wc,
                             l_int32  hc)
{
l_int32    i, j, w, h, d, wpla, wpld, jmax;
l_uint32  *linemina, *linemaxa, *lined, *dataa, *datad;
PIX       *pixsb, *pixacc, *pixd;

    PROCNAME("pixBlockconvGrayUnnormalized");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (wc < 0) wc = 0;
    if (hc < 0) hc = 0;
    if (w < 2 * wc + 1 || h < 2 * hc + 1) {
        wc = L_MIN(wc, (w - 1) / 2);
        hc = L_MIN(hc, (h - 1) / 2);
        L_WARNING("kernel too large; reducing!\n", procName);
        L_INFO("wc = %d, hc = %d\n", procName, wc, hc);
    }
    if (wc == 0 && hc == 0)   /* no-op */
        return pixCopy(NULL, pixs);

    if ((pixsb = pixAddMirroredBorder(pixs, wc + 1, wc, hc + 1, hc)) == NULL)
        return (PIX *)ERROR_PTR("pixsb not made", procName, NULL);
    pixacc = pixBlockconvAccum(pixsb);
    pixDestroy(&pixsb);
    if (!pixacc)
        return (PIX *)ERROR_PTR("pixacc not made", procName, NULL);
    if ((pixd = pixCreate(w, h, 32)) == NULL) {
        pixDestroy(&pixacc);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }

    wpla = pixGetWpl(pixacc);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);
    dataa = pixGetData(pixacc);
    for (i = 0; i < h; i++) {
        lined = datad + i * wpld;
        linemina = dataa + i * wpla;
        linemaxa = dataa + (i + 2 * hc + 1) * wpla;
        for (j = 0; j < w; j++) {
            jmax = j + 2 * wc + 1;
            lined[j] = linemaxa[jmax] - linemaxa[j] -
                       linemina[jmax] + linemina[j];
        }
    }

    pixDestroy(&pixacc);
    return pixd;
}


/*----------------------------------------------------------------------*
 *               Tiled grayscale or color block convolution             *
 *----------------------------------------------------------------------*/
/*!
 *  pixBlockconvTiled()
 *
 *      Input:  pix (8 or 32 bpp; or 2, 4 or 8 bpp with colormap)
 *              wc, hc   (half width/height of convolution kernel)
 *              nx, ny  (subdivision into tiles)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) The full width and height of the convolution kernel
 *          are (2 * wc + 1) and (2 * hc + 1)
 *      (2) Returns a copy if both wc and hc are 0
 *      (3) Require that w >= 2 * wc + 1 and h >= 2 * hc + 1,
 *          where (w,h) are the dimensions of pixs.
 *      (4) For nx == ny == 1, this defaults to pixBlockconv(), which
 *          is typically about twice as fast, and gives nearly
 *          identical results as pixBlockconvGrayTile().
 *      (5) If the tiles are too small, nx and/or ny are reduced
 *          a minimum amount so that the tiles are expanded to the
 *          smallest workable size in the problematic direction(s).
 *      (6) Why a tiled version?  Three reasons:
 *          (a) Because the accumulator is a uint32, overflow can occur
 *              for an image with more than 16M pixels.
 *          (b) The accumulator array for 16M pixels is 64 MB; using
 *              tiles reduces the size of this array.
 *          (c) Each tile can be processed independently, in parallel,
 *              on a multicore processor.
 */
PIX *
pixBlockconvTiled(PIX     *pix,
                  l_int32  wc,
                  l_int32  hc,
                  l_int32  nx,
                  l_int32  ny)
{
l_int32     i, j, w, h, d, xrat, yrat;
PIX        *pixs, *pixd, *pixc, *pixt;
PIX        *pixr, *pixrc, *pixg, *pixgc, *pixb, *pixbc;
PIXTILING  *pt;

    PROCNAME("pixBlockconvTiled");

    if (!pix)
        return (PIX *)ERROR_PTR("pix not defined", procName, NULL);
    if (wc < 0) wc = 0;
    if (hc < 0) hc = 0;
    pixGetDimensions(pix, &w, &h, &d);
    if (w < 2 * wc + 3 || h < 2 * hc + 3) {
        wc = L_MAX(0, L_MIN(wc, (w - 3) / 2));
        hc = L_MAX(0, L_MIN(hc, (h - 3) / 2));
        L_WARNING("kernel too large; reducing!\n", procName);
        L_INFO("wc = %d, hc = %d\n", procName, wc, hc);
    }
    if (wc == 0 && hc == 0)   /* no-op */
        return pixCopy(NULL, pix);
    if (nx <= 1 && ny <= 1)
        return pixBlockconv(pix, wc, hc);

        /* Test to see if the tiles are too small.  The required
         * condition is that the tile dimensions must be at least
         * (wc + 2) x (hc + 2). */
    xrat = w / nx;
    yrat = h / ny;
    if (xrat < wc + 2) {
        nx = w / (wc + 2);
        L_WARNING("tile width too small; nx reduced to %d\n", procName, nx);
    }
    if (yrat < hc + 2) {
        ny = h / (hc + 2);
        L_WARNING("tile height too small; ny reduced to %d\n", procName, ny);
    }

        /* Remove colormap if necessary */
    if ((d == 2 || d == 4 || d == 8) && pixGetColormap(pix)) {
        L_WARNING("pix has colormap; removing\n", procName);
        pixs = pixRemoveColormap(pix, REMOVE_CMAP_BASED_ON_SRC);
        d = pixGetDepth(pixs);
    } else {
        pixs = pixClone(pix);
    }

    if (d != 8 && d != 32) {
        pixDestroy(&pixs);
        return (PIX *)ERROR_PTR("depth not 8 or 32 bpp", procName, NULL);
    }

       /* Note that the overlaps in the width and height that
        * are added to the tile are (wc + 2) and (hc + 2).
        * These overlaps are removed by pixTilingPaintTile().
        * They are larger than the extent of the filter because
        * although the filter is symmetric with respect to its origin,
        * the implementation is asymmetric -- see the implementation in
        * pixBlockconvGrayTile(). */
    if ((pixd = pixCreateTemplateNoInit(pixs)) == NULL) {
        pixDestroy(&pixs);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    pt = pixTilingCreate(pixs, nx, ny, 0, 0, wc + 2, hc + 2);
    for (i = 0; i < ny; i++) {
        for (j = 0; j < nx; j++) {
            pixt = pixTilingGetTile(pt, i, j);

                /* Convolve over the tile */
            if (d == 8) {
                pixc = pixBlockconvGrayTile(pixt, NULL, wc, hc);
            } else { /* d == 32 */
                pixr = pixGetRGBComponent(pixt, COLOR_RED);
                pixrc = pixBlockconvGrayTile(pixr, NULL, wc, hc);
                pixDestroy(&pixr);
                pixg = pixGetRGBComponent(pixt, COLOR_GREEN);
                pixgc = pixBlockconvGrayTile(pixg, NULL, wc, hc);
                pixDestroy(&pixg);
                pixb = pixGetRGBComponent(pixt, COLOR_BLUE);
                pixbc = pixBlockconvGrayTile(pixb, NULL, wc, hc);
                pixDestroy(&pixb);
                pixc = pixCreateRGBImage(pixrc, pixgc, pixbc);
                pixDestroy(&pixrc);
                pixDestroy(&pixgc);
                pixDestroy(&pixbc);
            }

            pixTilingPaintTile(pixd, i, j, pixc, pt);
            pixDestroy(&pixt);
            pixDestroy(&pixc);
        }
    }

    pixDestroy(&pixs);
    pixTilingDestroy(&pt);
    return pixd;
}


/*!
 *  pixBlockconvGrayTile()
 *
 *      Input:  pixs (8 bpp gray)
 *              pixacc (32 bpp accum pix)
 *              wc, hc   (half width/height of convolution kernel)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) The full width and height of the convolution kernel
 *          are (2 * wc + 1) and (2 * hc + 1)
 *      (2) Assumes that the input pixs is padded with (wc + 1) pixels on
 *          left and right, and with (hc + 1) pixels on top and bottom.
 *          The returned pix has these stripped off; they are only used
 *          for computation.
 *      (3) Returns a copy if both wc and hc are 0
 *      (4) Require that w > 2 * wc + 1 and h > 2 * hc + 1,
 *          where (w,h) are the dimensions of pixs.
 */
PIX *
pixBlockconvGrayTile(PIX     *pixs,
                     PIX     *pixacc,
                     l_int32  wc,
                     l_int32  hc)
{
l_int32    w, h, d, wd, hd, i, j, imin, imax, jmin, jmax, wplt, wpld;
l_float32  norm;
l_uint32   val;
l_uint32  *datat, *datad, *lined, *linemint, *linemaxt;
PIX       *pixt, *pixd;

    PROCNAME("pixBlockconvGrayTile");

    if (!pixs)
        return (PIX *)ERROR_PTR("pix not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (wc < 0) wc = 0;
    if (hc < 0) hc = 0;
    if (w < 2 * wc + 3 || h < 2 * hc + 3) {
        wc = L_MAX(0, L_MIN(wc, (w - 3) / 2));
        hc = L_MAX(0, L_MIN(hc, (h - 3) / 2));
        L_WARNING("kernel too large; reducing!\n", procName);
        L_INFO("wc = %d, hc = %d\n", procName, wc, hc);
    }
    if (wc == 0 && hc == 0)
        return pixCopy(NULL, pixs);
    wd = w - 2 * wc;
    hd = h - 2 * hc;

    if (pixacc) {
        if (pixGetDepth(pixacc) == 32) {
            pixt = pixClone(pixacc);
        } else {
            L_WARNING("pixacc not 32 bpp; making new one\n", procName);
            if ((pixt = pixBlockconvAccum(pixs)) == NULL)
                return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
        }
    } else {
        if ((pixt = pixBlockconvAccum(pixs)) == NULL)
            return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
    }

    if ((pixd = pixCreateTemplate(pixs)) == NULL) {
        pixDestroy(&pixt);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    datat = pixGetData(pixt);
    wplt = pixGetWpl(pixt);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    norm = 1. / (l_float32)((2 * wc + 1) * (2 * hc + 1));

        /* Do the convolution over the subregion of size (wd - 2, hd - 2),
         * which exactly corresponds to the size of the subregion that
         * will be extracted by pixTilingPaintTile().  Note that the
         * region in which points are computed is not symmetric about
         * the center of the images; instead the computation in
         * the accumulator image is shifted up and to the left by 1,
         * relative to the center, because the 4 accumulator sampling
         * points are taken at the LL corner of the filter and at 3 other
         * points that are shifted -wc and -hc to the left and above.  */
    for (i = hc; i < hc + hd - 2; i++) {
        imin = L_MAX(i - hc - 1, 0);
        imax = L_MIN(i + hc, h - 1);
        lined = datad + i * wpld;
        linemint = datat + imin * wplt;
        linemaxt = datat + imax * wplt;
        for (j = wc; j < wc + wd - 2; j++) {
            jmin = L_MAX(j - wc - 1, 0);
            jmax = L_MIN(j + wc, w - 1);
            val = linemaxt[jmax] - linemaxt[jmin]
                  + linemint[jmin] - linemint[jmax];
            val = (l_uint8)(norm * val + 0.5);
            SET_DATA_BYTE(lined, j, val);
        }
    }

    pixDestroy(&pixt);
    return pixd;
}


/*----------------------------------------------------------------------*
 *     Convolution for mean, mean square, variance and rms deviation    *
 *----------------------------------------------------------------------*/
/*!
 *  pixWindowedStats()
 *
 *      Input:  pixs (8 bpp grayscale)
 *              wc, hc   (half width/height of convolution kernel)
 *              hasborder (use 1 if it already has (wc + 1) border pixels
 *                         on left and right, and (hc + 1) on top and bottom;
 *                         use 0 to add kernel-dependent border)
 *              &pixm (<optional return> 8 bpp mean value in window)
 *              &pixms (<optional return> 32 bpp mean square value in window)
 *              &fpixv (<optional return> float variance in window)
 *              &fpixrv (<optional return> float rms deviation from the mean)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is a high-level convenience function for calculating
 *          any or all of these derived images.
 *      (2) If @hasborder = 0, a border is added and the result is
 *          computed over all pixels in pixs.  Otherwise, no border is
 *          added and the border pixels are removed from the output images.
 *      (3) These statistical measures over the pixels in the
 *          rectangular window are:
 *            - average value: <p>  (pixm)
 *            - average squared value: <p*p> (pixms)
 *            - variance: <(p - <p>)*(p - <p>)> = <p*p> - <p>*<p>  (pixv)
 *            - square-root of variance: (pixrv)
 *          where the brackets < .. > indicate that the average value is
 *          to be taken over the window.
 *      (4) Note that the variance is just the mean square difference from
 *          the mean value; and the square root of the variance is the
 *          root mean square difference from the mean, sometimes also
 *          called the 'standard deviation'.
 *      (5) The added border, along with the use of an accumulator array,
 *          allows computation without special treatment of pixels near
 *          the image boundary, and runs in a time that is independent
 *          of the size of the convolution kernel.
 */
l_int32
pixWindowedStats(PIX     *pixs,
                 l_int32  wc,
                 l_int32  hc,
                 l_int32  hasborder,
                 PIX    **ppixm,
                 PIX    **ppixms,
                 FPIX   **pfpixv,
                 FPIX   **pfpixrv)
{
PIX  *pixb, *pixm, *pixms;

    PROCNAME("pixWindowedStats");

    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (wc < 2 || hc < 2)
        return ERROR_INT("wc and hc not >= 2", procName, 1);
    if (!ppixm && !ppixms && !pfpixv && !pfpixrv)
        return ERROR_INT("no output requested", procName, 1);
    if (ppixm) *ppixm = NULL;
    if (ppixms) *ppixms = NULL;
    if (pfpixv) *pfpixv = NULL;
    if (pfpixrv) *pfpixrv = NULL;

        /* Add border if requested */
    if (!hasborder)
        pixb = pixAddBorderGeneral(pixs, wc + 1, wc + 1, hc + 1, hc + 1, 0);
    else
        pixb = pixClone(pixs);

    if (!pfpixv && !pfpixrv) {
        if (ppixm) *ppixm = pixWindowedMean(pixb, wc, hc, 1, 1);
        if (ppixms) *ppixms = pixWindowedMeanSquare(pixb, wc, hc, 1);
        pixDestroy(&pixb);
        return 0;
    }

    pixm = pixWindowedMean(pixb, wc, hc, 1, 1);
    pixms = pixWindowedMeanSquare(pixb, wc, hc, 1);
    pixWindowedVariance(pixm, pixms, pfpixv, pfpixrv);
    if (ppixm)
        *ppixm = pixm;
    else
        pixDestroy(&pixm);
    if (ppixms)
        *ppixms = pixms;
    else
        pixDestroy(&pixms);
    pixDestroy(&pixb);
    return 0;
}


/*!
 *  pixWindowedMean()
 *
 *      Input:  pixs (8 or 32 bpp grayscale)
 *              wc, hc   (half width/height of convolution kernel)
 *              hasborder (use 1 if it already has (wc + 1) border pixels
 *                         on left and right, and (hc + 1) on top and bottom;
 *                         use 0 to add kernel-dependent border)
 *              normflag (1 for normalization to get average in window;
 *                        0 for the sum in the window (un-normalized))
 *      Return: pixd (8 or 32 bpp, average over kernel window)
 *
 *  Notes:
 *      (1) The input and output depths are the same.
 *      (2) A set of border pixels of width (wc + 1) on left and right,
 *          and of height (hc + 1) on top and bottom, must be on the
 *          pix before the accumulator is found.  The output pixd
 *          (after convolution) has this border removed.
 *          If @hasborder = 0, the required border is added.
 *      (3) Typically, @normflag == 1.  However, if you want the sum
 *          within the window, rather than a normalized convolution,
 *          use @normflag == 0.
 *      (4) This builds a block accumulator pix, uses it here, and
 *          destroys it.
 *      (5) The added border, along with the use of an accumulator array,
 *          allows computation without special treatment of pixels near
 *          the image boundary, and runs in a time that is independent
 *          of the size of the convolution kernel.
 */
PIX *
pixWindowedMean(PIX     *pixs,
                l_int32  wc,
                l_int32  hc,
                l_int32  hasborder,
                l_int32  normflag)
{
l_int32    i, j, w, h, d, wd, hd, wplc, wpld, wincr, hincr;
l_uint32   val;
l_uint32  *datac, *datad, *linec1, *linec2, *lined;
l_float32  norm;
PIX       *pixb, *pixc, *pixd;

    PROCNAME("pixWindowedMean");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    if (d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pixs not 8 or 32 bpp", procName, NULL);
    if (wc < 2 || hc < 2)
        return (PIX *)ERROR_PTR("wc and hc not >= 2", procName, NULL);

        /* Add border if requested */
    if (!hasborder)
        pixb = pixAddBorderGeneral(pixs, wc + 1, wc + 1, hc + 1, hc + 1, 0);
    else
        pixb = pixClone(pixs);

        /* The output has wc + 1 border pixels stripped from each side
         * of pixb, and hc + 1 border pixels stripped from top and bottom. */
    pixGetDimensions(pixb, &w, &h, NULL);
    wd = w - 2 * (wc + 1);
    hd = h - 2 * (hc + 1);
    if (wd < 2 || hd < 2)
        return (PIX *)ERROR_PTR("w or h too small for kernel", procName, NULL);
    if ((pixd = pixCreate(wd, hd, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

        /* Make the accumulator pix from pixb */
    if ((pixc = pixBlockconvAccum(pixb)) == NULL) {
        pixDestroy(&pixb);
        pixDestroy(&pixd);
        return (PIX *)ERROR_PTR("pixc not made", procName, NULL);
    }
    wplc = pixGetWpl(pixc);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);
    datac = pixGetData(pixc);

    wincr = 2 * wc + 1;
    hincr = 2 * hc + 1;
    norm = 1.0;  /* use this for sum-in-window */
    if (normflag)
        norm = 1.0 / (wincr * hincr);
    for (i = 0; i < hd; i++) {
        linec1 = datac + i * wplc;
        linec2 = datac + (i + hincr) * wplc;
        lined = datad + i * wpld;
        for (j = 0; j < wd; j++) {
            val = linec2[j + wincr] - linec2[j] - linec1[j + wincr] + linec1[j];
            if (d == 8) {
                val = (l_uint8)(norm * val);
                SET_DATA_BYTE(lined, j, val);
            } else {  /* d == 32 */
                val = (l_uint32)(norm * val);
                lined[j] = val;
            }
        }
    }

    pixDestroy(&pixc);
    pixDestroy(&pixb);
    return pixd;
}


/*!
 *  pixWindowedMeanSquare()
 *
 *      Input:  pixs (8 bpp grayscale)
 *              wc, hc   (half width/height of convolution kernel)
 *              hasborder (use 1 if it already has (wc + 1) border pixels
 *                         on left and right, and (hc + 1) on top and bottom;
 *                         use 0 to add kernel-dependent border)
 *      Return: pixd (32 bpp, average over rectangular window of
 *                    width = 2 * wc + 1 and height = 2 * hc + 1)
 *
 *  Notes:
 *      (1) A set of border pixels of width (wc + 1) on left and right,
 *          and of height (hc + 1) on top and bottom, must be on the
 *          pix before the accumulator is found.  The output pixd
 *          (after convolution) has this border removed.
 *          If @hasborder = 0, the required border is added.
 *      (2) The advantage is that we are unaffected by the boundary, and
 *          it is not necessary to treat pixels within @wc and @hc of the
 *          border differently.  This is because processing for pixd
 *          only takes place for pixels in pixs for which the
 *          kernel is entirely contained in pixs.
 *      (3) Why do we have an added border of width (@wc + 1) and
 *          height (@hc + 1), when we only need @wc and @hc pixels
 *          to satisfy this condition?  Answer: the accumulators
 *          are asymmetric, requiring an extra row and column of
 *          pixels at top and left to work accurately.
 *      (4) The added border, along with the use of an accumulator array,
 *          allows computation without special treatment of pixels near
 *          the image boundary, and runs in a time that is independent
 *          of the size of the convolution kernel.
 */
PIX *
pixWindowedMeanSquare(PIX     *pixs,
                      l_int32  wc,
                      l_int32  hc,
                      l_int32  hasborder)
{
l_int32     i, j, w, h, wd, hd, wpl, wpld, wincr, hincr;
l_uint32    ival;
l_uint32   *datad, *lined;
l_float64   norm;
l_float64   val;
l_float64  *data, *line1, *line2;
DPIX       *dpix;
PIX        *pixb, *pixd;

    PROCNAME("pixWindowedMeanSquare");

    if (!pixs || (pixGetDepth(pixs) != 8))
        return (PIX *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);
    if (wc < 2 || hc < 2)
        return (PIX *)ERROR_PTR("wc and hc not >= 2", procName, NULL);

        /* Add border if requested */
    if (!hasborder)
        pixb = pixAddBorderGeneral(pixs, wc + 1, wc + 1, hc + 1, hc + 1, 0);
    else
        pixb = pixClone(pixs);

    if ((dpix = pixMeanSquareAccum(pixb)) == NULL)
        return (PIX *)ERROR_PTR("dpix not made", procName, NULL);
    wpl = dpixGetWpl(dpix);
    data = dpixGetData(dpix);

        /* The output has wc + 1 border pixels stripped from each side
         * of pixb, and hc + 1 border pixels stripped from top and bottom. */
    pixGetDimensions(pixb, &w, &h, NULL);
    wd = w - 2 * (wc + 1);
    hd = h - 2 * (hc + 1);
    if (wd < 2 || hd < 2)
        return (PIX *)ERROR_PTR("w or h too small for kernel", procName, NULL);
    if ((pixd = pixCreate(wd, hd, 32)) == NULL) {
        dpixDestroy(&dpix);
        pixDestroy(&pixb);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);

    wincr = 2 * wc + 1;
    hincr = 2 * hc + 1;
    norm = 1.0 / (wincr * hincr);
    for (i = 0; i < hd; i++) {
        line1 = data + i * wpl;
        line2 = data + (i + hincr) * wpl;
        lined = datad + i * wpld;
        for (j = 0; j < wd; j++) {
            val = line2[j + wincr] - line2[j] - line1[j + wincr] + line1[j];
            ival = (l_uint32)(norm * val);
            lined[j] = ival;
        }
    }

    dpixDestroy(&dpix);
    pixDestroy(&pixb);
    return pixd;
}


/*!
 *  pixWindowedVariance()
 *
 *      Input:  pixm (mean over window; 8 or 32 bpp grayscale)
 *              pixms (mean square over window; 32 bpp)
 *              &fpixv (<optional return> float variance -- the ms deviation
 *                      from the mean)
 *              &fpixrv (<optional return> float rms deviation from the mean)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The mean and mean square values are precomputed, using
 *          pixWindowedMean() and pixWindowedMeanSquare().
 *      (2) Either or both of the variance and square-root of variance
 *          are returned as an fpix, where the variance is the
 *          average over the window of the mean square difference of
 *          the pixel value from the mean:
 *                <(p - <p>)*(p - <p>)> = <p*p> - <p>*<p>
 *      (3) To visualize the results:
 *            - for both, use fpixDisplayMaxDynamicRange().
 *            - for rms deviation, simply convert the output fpix to pix,
 */
l_int32
pixWindowedVariance(PIX    *pixm,
                    PIX    *pixms,
                    FPIX  **pfpixv,
                    FPIX  **pfpixrv)
{
l_int32     i, j, w, h, ws, hs, ds, wplm, wplms, wplv, wplrv, valm, valms;
l_float32   var;
l_uint32   *linem, *linems, *datam, *datams;
l_float32  *linev, *linerv, *datav, *datarv;
FPIX       *fpixv, *fpixrv;  /* variance and square root of variance */

    PROCNAME("pixWindowedVariance");

    if (!pfpixv && !pfpixrv)
        return ERROR_INT("&fpixv and &fpixrv not defined", procName, 1);
    if (pfpixv) *pfpixv = NULL;
    if (pfpixrv) *pfpixrv = NULL;
    if (!pixm || pixGetDepth(pixm) != 8)
        return ERROR_INT("pixm undefined or not 8 bpp", procName, 1);
    if (!pixms || pixGetDepth(pixms) != 32)
        return ERROR_INT("pixms undefined or not 32 bpp", procName, 1);
    pixGetDimensions(pixm, &w, &h, NULL);
    pixGetDimensions(pixms, &ws, &hs, &ds);
    if (w != ws || h != hs)
        return ERROR_INT("pixm and pixms sizes differ", procName, 1);

    if (pfpixv) {
        fpixv = fpixCreate(w, h);
        *pfpixv = fpixv;
        wplv = fpixGetWpl(fpixv);
        datav = fpixGetData(fpixv);
    }
    if (pfpixrv) {
        fpixrv = fpixCreate(w, h);
        *pfpixrv = fpixrv;
        wplrv = fpixGetWpl(fpixrv);
        datarv = fpixGetData(fpixrv);
    }

    wplm = pixGetWpl(pixm);
    wplms = pixGetWpl(pixms);
    datam = pixGetData(pixm);
    datams = pixGetData(pixms);
    for (i = 0; i < h; i++) {
        linem = datam + i * wplm;
        linems = datams + i * wplms;
        if (pfpixv)
            linev = datav + i * wplv;
        if (pfpixrv)
            linerv = datarv + i * wplrv;
        for (j = 0; j < w; j++) {
            valm = GET_DATA_BYTE(linem, j);
            if (ds == 8)
                valms = GET_DATA_BYTE(linems, j);
            else  /* ds == 32 */
                valms = (l_int32)linems[j];
            var = (l_float32)valms - (l_float32)valm * valm;
            if (pfpixv)
                linev[j] = var;
            if (pfpixrv)
                linerv[j] = (l_float32)sqrt(var);
        }
    }

    return 0;
}


/*!
 *  pixMeanSquareAccum()
 *
 *      Input:  pixs (8 bpp grayscale)
 *      Return: dpix (64 bit array), or null on error
 *
 *  Notes:
 *      (1) Similar to pixBlockconvAccum(), this computes the
 *          sum of the squares of the pixel values in such a way
 *          that the value at (i,j) is the sum of all squares in
 *          the rectangle from the origin to (i,j).
 *      (2) The general recursion relation (v are squared pixel values) is
 *            a(i,j) = v(i,j) + a(i-1, j) + a(i, j-1) - a(i-1, j-1)
 *          For the first line, this reduces to the special case
 *            a(i,j) = v(i,j) + a(i, j-1)
 *          For the first column, the special case is
 *            a(i,j) = v(i,j) + a(i-1, j)
 */
DPIX *
pixMeanSquareAccum(PIX  *pixs)
{
l_int32     i, j, w, h, wpl, wpls, val;
l_uint32   *datas, *lines;
l_float64  *data, *line, *linep;
DPIX       *dpix;

    PROCNAME("pixMeanSquareAccum");


    if (!pixs || (pixGetDepth(pixs) != 8))
        return (DPIX *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);
    if ((dpix = dpixCreate(w, h)) ==  NULL)
        return (DPIX *)ERROR_PTR("dpix not made", procName, NULL);

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    data = dpixGetData(dpix);
    wpl = dpixGetWpl(dpix);

    lines = datas;
    line = data;
    for (j = 0; j < w; j++) {   /* first line */
        val = GET_DATA_BYTE(lines, j);
        if (j == 0)
            line[0] = val * val;
        else
            line[j] = line[j - 1] + val * val;
    }

        /* Do the other lines */
    for (i = 1; i < h; i++) {
        lines = datas + i * wpls;
        line = data + i * wpl;  /* current dest line */
        linep = line - wpl;;  /* prev dest line */
        for (j = 0; j < w; j++) {
            val = GET_DATA_BYTE(lines, j);
            if (j == 0)
                line[0] = linep[0] + val * val;
            else
                line[j] = line[j - 1] + linep[j] - linep[j - 1] + val * val;
        }
    }

    return dpix;
}


/*----------------------------------------------------------------------*
 *                        Binary block sum/rank                         *
 *----------------------------------------------------------------------*/
/*!
 *  pixBlockrank()
 *
 *      Input:  pixs (1 bpp)
 *              accum pix (<optional> 32 bpp)
 *              wc, hc   (half width/height of block sum/rank kernel)
 *              rank   (between 0.0 and 1.0; 0.5 is median filter)
 *      Return: pixd (1 bpp)
 *
 *  Notes:
 *      (1) The full width and height of the convolution kernel
 *          are (2 * wc + 1) and (2 * hc + 1)
 *      (2) This returns a pixd where each pixel is a 1 if the
 *          neighborhood (2 * wc + 1) x (2 * hc + 1)) pixels
 *          contains the rank fraction of 1 pixels.  Otherwise,
 *          the returned pixel is 0.  Note that the special case
 *          of rank = 0.0 is always satisfied, so the returned
 *          pixd has all pixels with value 1.
 *      (3) If accum pix is null, make one, use it, and destroy it
 *          before returning; otherwise, just use the input accum pix
 *      (4) If both wc and hc are 0, returns a copy unless rank == 0.0,
 *          in which case this returns an all-ones image.
 *      (5) Require that w >= 2 * wc + 1 and h >= 2 * hc + 1,
 *          where (w,h) are the dimensions of pixs.
 */
PIX *
pixBlockrank(PIX       *pixs,
             PIX       *pixacc,
             l_int32    wc,
             l_int32    hc,
             l_float32  rank)
{
l_int32  w, h, d, thresh;
PIX     *pixt, *pixd;

    PROCNAME("pixBlockrank");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);
    if (rank < 0.0 || rank > 1.0)
        return (PIX *)ERROR_PTR("rank must be in [0.0, 1.0]", procName, NULL);

    if (rank == 0.0) {
        pixd = pixCreateTemplate(pixs);
        pixSetAll(pixd);
        return pixd;
    }

    if (wc < 0) wc = 0;
    if (hc < 0) hc = 0;
    if (w < 2 * wc + 1 || h < 2 * hc + 1) {
        wc = L_MIN(wc, (w - 1) / 2);
        hc = L_MIN(hc, (h - 1) / 2);
        L_WARNING("kernel too large; reducing!\n", procName);
        L_INFO("wc = %d, hc = %d\n", procName, wc, hc);
    }
    if (wc == 0 && hc == 0)
        return pixCopy(NULL, pixs);

    if ((pixt = pixBlocksum(pixs, pixacc, wc, hc)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, NULL);

        /* 1 bpp block rank filter output.
         * Must invert because threshold gives 1 for values < thresh,
         * but we need a 1 if the value is >= thresh. */
    thresh = (l_int32)(255. * rank);
    pixd = pixThresholdToBinary(pixt, thresh);
    pixInvert(pixd, pixd);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixBlocksum()
 *
 *      Input:  pixs (1 bpp)
 *              accum pix (<optional> 32 bpp)
 *              wc, hc   (half width/height of block sum/rank kernel)
 *      Return: pixd (8 bpp)
 *
 *  Notes:
 *      (1) If accum pix is null, make one and destroy it before
 *          returning; otherwise, just use the input accum pix
 *      (2) The full width and height of the convolution kernel
 *          are (2 * wc + 1) and (2 * hc + 1)
 *      (3) Use of wc = hc = 1, followed by pixInvert() on the
 *          8 bpp result, gives a nice anti-aliased, and somewhat
 *          darkened, result on text.
 *      (4) Require that w >= 2 * wc + 1 and h >= 2 * hc + 1,
 *          where (w,h) are the dimensions of pixs.
 *      (5) Returns in each dest pixel the sum of all src pixels
 *          that are within a block of size of the kernel, centered
 *          on the dest pixel.  This sum is the number of src ON
 *          pixels in the block at each location, normalized to 255
 *          for a block containing all ON pixels.  For pixels near
 *          the boundary, where the block is not entirely contained
 *          within the image, we then multiply by a second normalization
 *          factor that is greater than one, so that all results
 *          are normalized by the number of participating pixels
 *          within the block.
 */
PIX *
pixBlocksum(PIX     *pixs,
            PIX     *pixacc,
            l_int32  wc,
            l_int32  hc)
{
l_int32    w, h, d, wplt, wpld;
l_uint32  *datat, *datad;
PIX       *pixt, *pixd;

    PROCNAME("pixBlocksum");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);
    if (wc < 0) wc = 0;
    if (hc < 0) hc = 0;
    if (w < 2 * wc + 1 || h < 2 * hc + 1) {
        wc = L_MIN(wc, (w - 1) / 2);
        hc = L_MIN(hc, (h - 1) / 2);
        L_WARNING("kernel too large; reducing!\n", procName);
        L_INFO("wc = %d, hc = %d\n", procName, wc, hc);
    }
    if (wc == 0 && hc == 0)
        return pixCopy(NULL, pixs);

    if (pixacc) {
        if (pixGetDepth(pixacc) != 32)
            return (PIX *)ERROR_PTR("pixacc not 32 bpp", procName, NULL);
        pixt = pixClone(pixacc);
    } else {
        if ((pixt = pixBlockconvAccum(pixs)) == NULL)
            return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
    }

        /* 8 bpp block sum output */
    if ((pixd = pixCreate(w, h, 8)) == NULL) {
        pixDestroy(&pixt);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    pixCopyResolution(pixd, pixs);

    wpld = pixGetWpl(pixd);
    wplt = pixGetWpl(pixt);
    datad = pixGetData(pixd);
    datat = pixGetData(pixt);
    blocksumLow(datad, w, h, wpld, datat, wplt, wc, hc);

    pixDestroy(&pixt);
    return pixd;
}


/*----------------------------------------------------------------------*
 *                          Census transform                            *
 *----------------------------------------------------------------------*/
/*!
 *  pixCensusTransform()
 *
 *      Input:  pixs (8 bpp)
 *              halfsize (of square over which neighbors are averaged)
 *              accum pix (<optional> 32 bpp)
 *      Return: pixd (1 bpp)
 *
 *  Notes:
 *      (1) The Census transform was invented by Ramin Zabih and John Woodfill
 *          ("Non-parametric local transforms for computing visual
 *          correspondence", Third European Conference on Computer Vision,
 *          Stockholm, Sweden, May 1994); see publications at
 *             http://www.cs.cornell.edu/~rdz/index.htm
 *          This compares each pixel against the average of its neighbors,
 *          in a square of odd dimension centered on the pixel.
 *          If the pixel is greater than the average of its neighbors,
 *          the output pixel value is 1; otherwise it is 0.
 *      (2) This can be used as an encoding for an image that is
 *          fairly robust against slow illumination changes, with
 *          applications in image comparison and mosaicing.
 *      (3) The size of the convolution kernel is (2 * halfsize + 1)
 *          on a side.  The halfsize parameter must be >= 1.
 *      (4) If accum pix is null, make one, use it, and destroy it
 *          before returning; otherwise, just use the input accum pix
 */
PIX *
pixCensusTransform(PIX     *pixs,
                   l_int32  halfsize,
                   PIX     *pixacc)
{
l_int32    i, j, w, h, wpls, wplv, wpld;
l_int32    vals, valv;
l_uint32  *datas, *datav, *datad, *lines, *linev, *lined;
PIX       *pixav, *pixd;

    PROCNAME("pixCensusTransform");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (halfsize < 1)
        return (PIX *)ERROR_PTR("halfsize must be >= 1", procName, NULL);

        /* Get the average of each pixel with its neighbors */
    if ((pixav = pixBlockconvGray(pixs, pixacc, halfsize, halfsize))
          == NULL)
        return (PIX *)ERROR_PTR("pixav not made", procName, NULL);

        /* Subtract the pixel from the average, and then compare
         * the pixel value with the remaining average */
    pixGetDimensions(pixs, &w, &h, NULL);
    if ((pixd = pixCreate(w, h, 1)) == NULL) {
        pixDestroy(&pixav);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    datas = pixGetData(pixs);
    datav = pixGetData(pixav);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wplv = pixGetWpl(pixav);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        linev = datav + i * wplv;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            vals = GET_DATA_BYTE(lines, j);
            valv = GET_DATA_BYTE(linev, j);
            if (vals > valv)
                SET_DATA_BIT(lined, j);
        }
    }

    pixDestroy(&pixav);
    return pixd;
}


/*----------------------------------------------------------------------*
 *                         Generic convolution                          *
 *----------------------------------------------------------------------*/
/*!
 *  pixConvolve()
 *
 *      Input:  pixs (8, 16, 32 bpp; no colormap)
 *              kernel
 *              outdepth (of pixd: 8, 16 or 32)
 *              normflag (1 to normalize kernel to unit sum; 0 otherwise)
 *      Return: pixd (8, 16 or 32 bpp)
 *
 *  Notes:
 *      (1) This gives a convolution with an arbitrary kernel.
 *      (2) The input pixs must have only one sample/pixel.
 *          To do a convolution on an RGB image, use pixConvolveRGB().
 *      (3) The parameter @outdepth determines the depth of the result.
 *          If the kernel is normalized to unit sum, the output values
 *          can never exceed 255, so an output depth of 8 bpp is sufficient.
 *          If the kernel is not normalized, it may be necessary to use
 *          16 or 32 bpp output to avoid overflow.
 *      (4) If normflag == 1, the result is normalized by scaling all
 *          kernel values for a unit sum.  If the sum of kernel values
 *          is very close to zero, the kernel can not be normalized and
 *          the convolution will not be performed.  A warning is issued.
 *      (5) The kernel values can be positive or negative, but the
 *          result for the convolution can only be stored as a positive
 *          number.  Consequently, if it goes negative, the choices are
 *          to clip to 0 or take the absolute value.  We're choosing
 *          to take the absolute value.  (Another possibility would be
 *          to output a second unsigned image for the negative values.)
 *          If you want to get a clipped result, or to keep the negative
 *          values in the result, use fpixConvolve(), with the
 *          converters in fpix2.c between pix and fpix.
 *      (6) This uses a mirrored border to avoid special casing on
 *          the boundaries.
 *      (7) To get a subsampled output, call l_setConvolveSampling().
 *          The time to make a subsampled output is reduced by the
 *          product of the sampling factors.
 *      (8) The function is slow, running at about 12 machine cycles for
 *          each pixel-op in the convolution.  For example, with a 3 GHz
 *          cpu, a 1 Mpixel grayscale image, and a kernel with
 *          (sx * sy) = 25 elements, the convolution takes about 100 msec.
 */
PIX *
pixConvolve(PIX       *pixs,
            L_KERNEL  *kel,
            l_int32    outdepth,
            l_int32    normflag)
{
l_int32    i, j, id, jd, k, m, w, h, d, wd, hd, sx, sy, cx, cy, wplt, wpld;
l_int32    val;
l_uint32  *datat, *datad, *linet, *lined;
l_float32  sum;
L_KERNEL  *keli, *keln;
PIX       *pixt, *pixd;

    PROCNAME("pixConvolve");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs has colormap", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("pixs not 8, 16, or 32 bpp", procName, NULL);
    if (!kel)
        return (PIX *)ERROR_PTR("kel not defined", procName, NULL);

    keli = kernelInvert(kel);
    kernelGetParameters(keli, &sy, &sx, &cy, &cx);
    if (normflag)
        keln = kernelNormalize(keli, 1.0);
    else
        keln = kernelCopy(keli);

    if ((pixt = pixAddMirroredBorder(pixs, cx, sx - cx, cy, sy - cy)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, NULL);

    wd = (w + ConvolveSamplingFactX - 1) / ConvolveSamplingFactX;
    hd = (h + ConvolveSamplingFactY - 1) / ConvolveSamplingFactY;
    pixd = pixCreate(wd, hd, outdepth);
    datat = pixGetData(pixt);
    datad = pixGetData(pixd);
    wplt = pixGetWpl(pixt);
    wpld = pixGetWpl(pixd);
    for (i = 0, id = 0; id < hd; i += ConvolveSamplingFactY, id++) {
        lined = datad + id * wpld;
        for (j = 0, jd = 0; jd < wd; j += ConvolveSamplingFactX, jd++) {
            sum = 0.0;
            for (k = 0; k < sy; k++) {
                linet = datat + (i + k) * wplt;
                if (d == 8) {
                    for (m = 0; m < sx; m++) {
                        val = GET_DATA_BYTE(linet, j + m);
                        sum += val * keln->data[k][m];
                    }
                } else if (d == 16) {
                    for (m = 0; m < sx; m++) {
                        val = GET_DATA_TWO_BYTES(linet, j + m);
                        sum += val * keln->data[k][m];
                    }
                } else {  /* d == 32 */
                    for (m = 0; m < sx; m++) {
                        val = *(linet + j + m);
                        sum += val * keln->data[k][m];
                    }
                }
            }
            if (sum < 0.0) sum = -sum;  /* make it non-negative */
            if (outdepth == 8)
                SET_DATA_BYTE(lined, jd, (l_int32)(sum + 0.5));
            else if (outdepth == 16)
                SET_DATA_TWO_BYTES(lined, jd, (l_int32)(sum + 0.5));
            else  /* outdepth == 32 */
                *(lined + jd) = (l_uint32)(sum + 0.5);
        }
    }

    kernelDestroy(&keli);
    kernelDestroy(&keln);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixConvolveSep()
 *
 *      Input:  pixs (8, 16, 32 bpp; no colormap)
 *              kelx (x-dependent kernel)
 *              kely (y-dependent kernel)
 *              outdepth (of pixd: 8, 16 or 32)
 *              normflag (1 to normalize kernel to unit sum; 0 otherwise)
 *      Return: pixd (8, 16 or 32 bpp)
 *
 *  Notes:
 *      (1) This does a convolution with a separable kernel that is
 *          is a sequence of convolutions in x and y.  The two
 *          one-dimensional kernel components must be input separately;
 *          the full kernel is the product of these components.
 *          The support for the full kernel is thus a rectangular region.
 *      (2) The input pixs must have only one sample/pixel.
 *          To do a convolution on an RGB image, use pixConvolveSepRGB().
 *      (3) The parameter @outdepth determines the depth of the result.
 *          If the kernel is normalized to unit sum, the output values
 *          can never exceed 255, so an output depth of 8 bpp is sufficient.
 *          If the kernel is not normalized, it may be necessary to use
 *          16 or 32 bpp output to avoid overflow.
 *      (2) The @normflag parameter is used as in pixConvolve().
 *      (4) The kernel values can be positive or negative, but the
 *          result for the convolution can only be stored as a positive
 *          number.  Consequently, if it goes negative, the choices are
 *          to clip to 0 or take the absolute value.  We're choosing
 *          the former for now.  Another possibility would be to output
 *          a second unsigned image for the negative values.
 *      (5) Warning: if you use l_setConvolveSampling() to get a
 *          subsampled output, and the sampling factor is larger than
 *          the kernel half-width, it is faster to use the non-separable
 *          version pixConvolve().  This is because the first convolution
 *          here must be done on every raster line, regardless of the
 *          vertical sampling factor.  If the sampling factor is smaller
 *          than kernel half-width, it's faster to use the separable
 *          convolution.
 *      (6) This uses mirrored borders to avoid special casing on
 *          the boundaries.
 */
PIX *
pixConvolveSep(PIX       *pixs,
               L_KERNEL  *kelx,
               L_KERNEL  *kely,
               l_int32    outdepth,
               l_int32    normflag)
{
l_int32    d, xfact, yfact;
L_KERNEL  *kelxn, *kelyn;
PIX       *pixt, *pixd;

    PROCNAME("pixConvolveSep");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    if (d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("pixs not 8, 16, or 32 bpp", procName, NULL);
    if (!kelx)
        return (PIX *)ERROR_PTR("kelx not defined", procName, NULL);
    if (!kely)
        return (PIX *)ERROR_PTR("kely not defined", procName, NULL);

    xfact = ConvolveSamplingFactX;
    yfact = ConvolveSamplingFactY;
    if (normflag) {
        kelxn = kernelNormalize(kelx, 1000.0);
        kelyn = kernelNormalize(kely, 0.001);
        l_setConvolveSampling(xfact, 1);
        pixt = pixConvolve(pixs, kelxn, 32, 0);
        l_setConvolveSampling(1, yfact);
        pixd = pixConvolve(pixt, kelyn, outdepth, 0);
        l_setConvolveSampling(xfact, yfact);  /* restore */
        kernelDestroy(&kelxn);
        kernelDestroy(&kelyn);
    } else {  /* don't normalize */
        l_setConvolveSampling(xfact, 1);
        pixt = pixConvolve(pixs, kelx, 32, 0);
        l_setConvolveSampling(1, yfact);
        pixd = pixConvolve(pixt, kely, outdepth, 0);
        l_setConvolveSampling(xfact, yfact);
    }

    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixConvolveRGB()
 *
 *      Input:  pixs (32 bpp rgb)
 *              kernel
 *      Return: pixd (32 bpp rgb)
 *
 *  Notes:
 *      (1) This gives a convolution on an RGB image using an
 *          arbitrary kernel (which we normalize to keep each
 *          component within the range [0 ... 255].
 *      (2) The input pixs must be RGB.
 *      (3) The kernel values can be positive or negative, but the
 *          result for the convolution can only be stored as a positive
 *          number.  Consequently, if it goes negative, we clip the
 *          result to 0.
 *      (4) To get a subsampled output, call l_setConvolveSampling().
 *          The time to make a subsampled output is reduced by the
 *          product of the sampling factors.
 *      (5) This uses a mirrored border to avoid special casing on
 *          the boundaries.
 */
PIX *
pixConvolveRGB(PIX       *pixs,
               L_KERNEL  *kel)
{
PIX  *pixt, *pixr, *pixg, *pixb, *pixd;

    PROCNAME("pixConvolveRGB");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs is not 32 bpp", procName, NULL);
    if (!kel)
        return (PIX *)ERROR_PTR("kel not defined", procName, NULL);

    pixt = pixGetRGBComponent(pixs, COLOR_RED);
    pixr = pixConvolve(pixt, kel, 8, 1);
    pixDestroy(&pixt);
    pixt = pixGetRGBComponent(pixs, COLOR_GREEN);
    pixg = pixConvolve(pixt, kel, 8, 1);
    pixDestroy(&pixt);
    pixt = pixGetRGBComponent(pixs, COLOR_BLUE);
    pixb = pixConvolve(pixt, kel, 8, 1);
    pixDestroy(&pixt);
    pixd = pixCreateRGBImage(pixr, pixg, pixb);

    pixDestroy(&pixr);
    pixDestroy(&pixg);
    pixDestroy(&pixb);
    return pixd;
}


/*!
 *  pixConvolveRGBSep()
 *
 *      Input:  pixs (32 bpp rgb)
 *              kelx (x-dependent kernel)
 *              kely (y-dependent kernel)
 *      Return: pixd (32 bpp rgb)
 *
 *  Notes:
 *      (1) This does a convolution on an RGB image using a separable
 *          kernel that is a sequence of convolutions in x and y.  The two
 *          one-dimensional kernel components must be input separately;
 *          the full kernel is the product of these components.
 *          The support for the full kernel is thus a rectangular region.
 *      (2) The kernel values can be positive or negative, but the
 *          result for the convolution can only be stored as a positive
 *          number.  Consequently, if it goes negative, we clip the
 *          result to 0.
 *      (3) To get a subsampled output, call l_setConvolveSampling().
 *          The time to make a subsampled output is reduced by the
 *          product of the sampling factors.
 *      (4) This uses a mirrored border to avoid special casing on
 *          the boundaries.
 */
PIX *
pixConvolveRGBSep(PIX       *pixs,
                  L_KERNEL  *kelx,
                  L_KERNEL  *kely)
{
PIX  *pixt, *pixr, *pixg, *pixb, *pixd;

    PROCNAME("pixConvolveRGBSep");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs is not 32 bpp", procName, NULL);
    if (!kelx || !kely)
        return (PIX *)ERROR_PTR("kelx, kely not both defined", procName, NULL);

    pixt = pixGetRGBComponent(pixs, COLOR_RED);
    pixr = pixConvolveSep(pixt, kelx, kely, 8, 1);
    pixDestroy(&pixt);
    pixt = pixGetRGBComponent(pixs, COLOR_GREEN);
    pixg = pixConvolveSep(pixt, kelx, kely, 8, 1);
    pixDestroy(&pixt);
    pixt = pixGetRGBComponent(pixs, COLOR_BLUE);
    pixb = pixConvolveSep(pixt, kelx, kely, 8, 1);
    pixDestroy(&pixt);
    pixd = pixCreateRGBImage(pixr, pixg, pixb);

    pixDestroy(&pixr);
    pixDestroy(&pixg);
    pixDestroy(&pixb);
    return pixd;
}


/*----------------------------------------------------------------------*
 *                  Generic convolution with float array                *
 *----------------------------------------------------------------------*/
/*!
 *  fpixConvolve()
 *
 *      Input:  fpixs (32 bit float array)
 *              kernel
 *              normflag (1 to normalize kernel to unit sum; 0 otherwise)
 *      Return: fpixd (32 bit float array)
 *
 *  Notes:
 *      (1) This gives a float convolution with an arbitrary kernel.
 *      (2) If normflag == 1, the result is normalized by scaling all
 *          kernel values for a unit sum.  If the sum of kernel values
 *          is very close to zero, the kernel can not be normalized and
 *          the convolution will not be performed.  A warning is issued.
 *      (3) With the FPix, there are no issues about negative
 *          array or kernel values.  The convolution is performed
 *          with single precision arithmetic.
 *      (4) To get a subsampled output, call l_setConvolveSampling().
 *          The time to make a subsampled output is reduced by the
 *          product of the sampling factors.
 *      (5) This uses a mirrored border to avoid special casing on
 *          the boundaries.
 */
FPIX *
fpixConvolve(FPIX      *fpixs,
             L_KERNEL  *kel,
             l_int32    normflag)
{
l_int32     i, j, id, jd, k, m, w, h, wd, hd, sx, sy, cx, cy, wplt, wpld;
l_float32   val;
l_float32  *datat, *datad, *linet, *lined;
l_float32   sum;
L_KERNEL   *keli, *keln;
FPIX       *fpixt, *fpixd;

    PROCNAME("fpixConvolve");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);
    if (!kel)
        return (FPIX *)ERROR_PTR("kel not defined", procName, NULL);

    keli = kernelInvert(kel);
    kernelGetParameters(keli, &sy, &sx, &cy, &cx);
    if (normflag)
        keln = kernelNormalize(keli, 1.0);
    else
        keln = kernelCopy(keli);

    fpixGetDimensions(fpixs, &w, &h);
    fpixt = fpixAddMirroredBorder(fpixs, cx, sx - cx, cy, sy - cy);
    if (!fpixt)
        return (FPIX *)ERROR_PTR("fpixt not made", procName, NULL);

    wd = (w + ConvolveSamplingFactX - 1) / ConvolveSamplingFactX;
    hd = (h + ConvolveSamplingFactY - 1) / ConvolveSamplingFactY;
    fpixd = fpixCreate(wd, hd);
    datat = fpixGetData(fpixt);
    datad = fpixGetData(fpixd);
    wplt = fpixGetWpl(fpixt);
    wpld = fpixGetWpl(fpixd);
    for (i = 0, id = 0; id < hd; i += ConvolveSamplingFactY, id++) {
        lined = datad + id * wpld;
        for (j = 0, jd = 0; jd < wd; j += ConvolveSamplingFactX, jd++) {
            sum = 0.0;
            for (k = 0; k < sy; k++) {
                linet = datat + (i + k) * wplt;
                for (m = 0; m < sx; m++) {
                    val = *(linet + j + m);
                    sum += val * keln->data[k][m];
                }
            }
            *(lined + jd) = sum;
        }
    }

    kernelDestroy(&keli);
    kernelDestroy(&keln);
    fpixDestroy(&fpixt);
    return fpixd;
}


/*!
 *  fpixConvolveSep()
 *
 *      Input:  fpixs (32 bit float array)
 *              kelx (x-dependent kernel)
 *              kely (y-dependent kernel)
 *              normflag (1 to normalize kernel to unit sum; 0 otherwise)
 *      Return: fpixd (32 bit float array)
 *
 *  Notes:
 *      (1) This does a convolution with a separable kernel that is
 *          is a sequence of convolutions in x and y.  The two
 *          one-dimensional kernel components must be input separately;
 *          the full kernel is the product of these components.
 *          The support for the full kernel is thus a rectangular region.
 *      (2) The normflag parameter is used as in fpixConvolve().
 *      (3) Warning: if you use l_setConvolveSampling() to get a
 *          subsampled output, and the sampling factor is larger than
 *          the kernel half-width, it is faster to use the non-separable
 *          version pixConvolve().  This is because the first convolution
 *          here must be done on every raster line, regardless of the
 *          vertical sampling factor.  If the sampling factor is smaller
 *          than kernel half-width, it's faster to use the separable
 *          convolution.
 *      (4) This uses mirrored borders to avoid special casing on
 *          the boundaries.
 */
FPIX *
fpixConvolveSep(FPIX      *fpixs,
                L_KERNEL  *kelx,
                L_KERNEL  *kely,
                l_int32    normflag)
{
l_int32    xfact, yfact;
L_KERNEL  *kelxn, *kelyn;
FPIX      *fpixt, *fpixd;

    PROCNAME("fpixConvolveSep");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!kelx)
        return (FPIX *)ERROR_PTR("kelx not defined", procName, NULL);
    if (!kely)
        return (FPIX *)ERROR_PTR("kely not defined", procName, NULL);

    xfact = ConvolveSamplingFactX;
    yfact = ConvolveSamplingFactY;
    if (normflag) {
        kelxn = kernelNormalize(kelx, 1.0);
        kelyn = kernelNormalize(kely, 1.0);
        l_setConvolveSampling(xfact, 1);
        fpixt = fpixConvolve(fpixs, kelxn, 0);
        l_setConvolveSampling(1, yfact);
        fpixd = fpixConvolve(fpixt, kelyn, 0);
        l_setConvolveSampling(xfact, yfact);  /* restore */
        kernelDestroy(&kelxn);
        kernelDestroy(&kelyn);
    } else {  /* don't normalize */
        l_setConvolveSampling(xfact, 1);
        fpixt = fpixConvolve(fpixs, kelx, 0);
        l_setConvolveSampling(1, yfact);
        fpixd = fpixConvolve(fpixt, kely, 0);
        l_setConvolveSampling(xfact, yfact);
    }

    fpixDestroy(&fpixt);
    return fpixd;
}


/*------------------------------------------------------------------------*
 *              Convolution with bias (for non-negative output)           *
 *------------------------------------------------------------------------*/
/*!
 *  pixConvolveWithBias()
 *
 *      Input:  pixs (8 bpp; no colormap)
 *              kel1
 *              kel2  (can be null; use if separable)
 *              force8 (if 1, force output to 8 bpp; otherwise, determine
 *                      output depth by the dynamic range of pixel values)
 *              &bias (<return> applied bias)
 *      Return: pixd (8 or 16 bpp)
 *
 *  Notes:
 *      (1) This does a convolution with either a single kernel or
 *          a pair of separable kernels, and automatically applies whatever
 *          bias (shift) is required so that the resulting pixel values
 *          are non-negative.
 *      (2) The kernel is always normalized.  If there are no negative
 *          values in the kernel, a standard normalized convolution is
 *          performed, with 8 bpp output.  If the sum of kernel values is
 *          very close to zero, the kernel can not be normalized and
 *          the convolution will not be performed.  An error message results.
 *      (3) If there are negative values in the kernel, the pix is
 *          converted to an fpix, the convolution is done on the fpix, and
 *          a bias (shift) may need to be applied.
 *      (4) If force8 == TRUE and the range of values after the convolution
 *          is > 255, the output values will be scaled to fit in [0 ... 255].
 *          If force8 == FALSE, the output will be either 8 or 16 bpp,
 *          to accommodate the dynamic range of output values without scaling.
 */
PIX *
pixConvolveWithBias(PIX       *pixs,
                    L_KERNEL  *kel1,
                    L_KERNEL  *kel2,
                    l_int32    force8,
                    l_int32   *pbias)
{
l_int32    outdepth;
l_float32  min1, min2, min, minval, maxval, range;
FPIX      *fpix1, *fpix2;
PIX       *pixd;

    PROCNAME("pixConvolveWithBias");

    if (!pixs || pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs has colormap", procName, NULL);
    if (!kel1)
        return (PIX *)ERROR_PTR("kel1 not defined", procName, NULL);

        /* Determine if negative values can be produced in the convolution */
    kernelGetMinMax(kel1, &min1, NULL);
    min2 = 0.0;
    if (kel2)
        kernelGetMinMax(kel2, &min2, NULL);
    min = L_MIN(min1, min2);

    if (min >= 0.0) {
        if (!kel2)
            return pixConvolve(pixs, kel1, 8, 1);
        else
            return pixConvolveSep(pixs, kel1, kel2, 8, 1);
    }

        /* Bias may need to be applied; convert to fpix and convolve */
    fpix1 = pixConvertToFPix(pixs, 1);
    if (!kel2)
        fpix2 = fpixConvolve(fpix1, kel1, 1);
    else
        fpix2 = fpixConvolveSep(fpix1, kel1, kel2, 1);
    fpixDestroy(&fpix1);

        /* Determine the bias and the dynamic range.
         * If the dynamic range is <= 255, just shift the values by the
         * bias, if any.
         * If the dynamic range is > 255, there are two cases:
         *    (1) the output depth is not forced to 8 bpp
         *           ==> apply the bias without scaling; outdepth = 16
         *    (2) the output depth is forced to 8
         *           ==> linearly map the pixel values to [0 ... 255].  */
    fpixGetMin(fpix2, &minval, NULL, NULL);
    fpixGetMax(fpix2, &maxval, NULL, NULL);
    range = maxval - minval;
    *pbias = (minval < 0.0) ? -minval : 0.0;
    fpixAddMultConstant(fpix2, *pbias, 1.0);  /* shift: min val ==> 0 */
    if (range <= 255 || !force8) {  /* no scaling of output values */
        outdepth = (range > 255) ? 16 : 8;
    } else {  /* scale output values to fit in 8 bpp */
        fpixAddMultConstant(fpix2, 0.0, (255.0 / range));
        outdepth = 8;
    }

        /* Convert back to pix; it won't do any clipping */
    pixd = fpixConvertToPix(fpix2, outdepth, L_CLIP_TO_ZERO, 0);
    fpixDestroy(&fpix2);

    return pixd;
}


/*------------------------------------------------------------------------*
 *                Set parameter for convolution subsampling               *
 *------------------------------------------------------------------------*/
/*!
 *  l_setConvolveSampling()

 *
 *      Input:  xfact, yfact (integer >= 1)
 *      Return: void
 *
 *  Notes:
 *      (1) This sets the x and y output subsampling factors for generic pix
 *          and fpix convolution.  The default values are 1 (no subsampling).
 */
void
l_setConvolveSampling(l_int32  xfact,
                      l_int32  yfact)
{
    if (xfact < 1) xfact = 1;
    if (yfact < 1) yfact = 1;
    ConvolveSamplingFactX = xfact;
    ConvolveSamplingFactY = yfact;
}


/*------------------------------------------------------------------------*
 *                          Additive gaussian noise                       *
 *------------------------------------------------------------------------*/
/*!
 *  pixAddGaussianNoise()
 *
 *      Input:  pixs (8 bpp gray or 32 bpp rgb; no colormap)
 *              stdev (of noise)
 *      Return: pixd (8 or 32 bpp), or null on error
 *
 *  Notes:
 *      (1) This adds noise to each pixel, taken from a normal
 *          distribution with zero mean and specified standard deviation.
 */
PIX *
pixAddGaussianNoise(PIX       *pixs,
                    l_float32  stdev)
{
l_int32    i, j, w, h, d, wpls, wpld, val, rval, gval, bval;
l_uint32   pixel;
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixAddGaussianNoise");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs has colormap", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pixs not 8 or 32 bpp", procName, NULL);

    pixd = pixCreateTemplateNoInit(pixs);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            if (d == 8) {
                val = GET_DATA_BYTE(lines, j);
                val += (l_int32)(stdev * gaussDistribSampling() + 0.5);
                val = L_MIN(255, L_MAX(0, val));
                SET_DATA_BYTE(lined, j, val);
            } else {  /* d = 32 */
                pixel = *(lines + j);
                extractRGBValues(pixel, &rval, &gval, &bval);
                rval += (l_int32)(stdev * gaussDistribSampling() + 0.5);
                rval = L_MIN(255, L_MAX(0, rval));
                gval += (l_int32)(stdev * gaussDistribSampling() + 0.5);
                gval = L_MIN(255, L_MAX(0, gval));
                bval += (l_int32)(stdev * gaussDistribSampling() + 0.5);
                bval = L_MIN(255, L_MAX(0, bval));
                composeRGBPixel(rval, gval, bval, lined + j);
            }
        }
    }
    return pixd;
}


/*!
 *  gaussDistribSampling()
 *
 *      Return: gaussian distributed variable with zero mean and unit stdev
 *
 *  Notes:
 *      (1) For an explanation of the Box-Muller method for generating
 *          a normally distributed random variable with zero mean and
 *          unit standard deviation, see Numerical Recipes in C,
 *          2nd edition, p. 288ff.
 *      (2) This can be called sequentially to get samples that can be
 *          used for adding noise to each pixel of an image, for example.
 */
l_float32
gaussDistribSampling()
{
static l_int32    select = 0;  /* flips between 0 and 1 on successive calls */
static l_float32  saveval;
l_float32         frand, xval, yval, rsq, factor;

    if (select == 0) {
        while (1) {  /* choose a point in a 2x2 square, centered at origin */
            frand = (l_float32)rand() / (l_float32)RAND_MAX;
            xval = 2.0 * frand - 1.0;
            frand = (l_float32)rand() / (l_float32)RAND_MAX;
            yval = 2.0 * frand - 1.0;
            rsq = xval * xval + yval * yval;
            if (rsq > 0.0 && rsq < 1.0)  /* point is inside the unit circle */
                break;
        }
        factor = sqrt(-2.0 * log(rsq) / rsq);
        saveval = xval * factor;
        select = 1;
        return yval * factor;
    }
    else {
        select = 0;
        return saveval;
    }
}
