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
 *  pixlabel.c
 *
 *     Label pixels by an index for connected component membership
 *           PIX         *pixConnCompTransform()
 *
 *     Label pixels by the area of their connected component
 *           PIX         *pixConnCompAreaTransform()
 *
 *     Label pixels with spatially-dependent color coding
 *           PIX         *pixLocToColorTransform()
 *
 *  Pixels get labelled in various ways throughout the leptonica library,
 *  but most of the labelling is implicit, where the new value isn't
 *  even considered to be a label -- it is just a transformed pixel value
 *  that may be transformed again by another operation.  Quantization
 *  by thresholding, and dilation by a structuring element, are examples
 *  of these typical image processing operations.
 *
 *  However, there are some explicit labelling procedures that are useful
 *  as end-points of analysis, where it typically would not make sense
 *  to do further image processing on the result.  Assigning false color
 *  based on pixel properties is an example of such labelling operations.
 *  Such operations typically have 1 bpp input images, and result
 *  in grayscale or color images.
 *
 *  The procedures in this file are concerned with such explicit labelling.
 *  Some of these labelling procedures are also in other places in leptonica:
 *
 *    runlength.c:
 *       This file has two labelling transforms based on runlengths:
 *       pixStrokeWidthTransform() and pixvRunlengthTransform().
 *       The pixels are labelled based on the width of the "stroke" to
 *       which they belong, or on the length of the horizontal or
 *       vertical run in which they are a member.  Runlengths can easily
 *       be filtered using a threshold.
 *
 *    pixafunc2.c:
 *       This file has an operation, pixaDisplayRandomCmap(), that
 *       randomly labels pix in a pixa (that are typically found using
 *       pixConnComp) with up to 256 values, and assigns each value to
 *       a random colormap color.
 *
 *    seedfill.c:
 *       This file has pixDistanceFunction(), that labels each pixel with
 *       its distance from either the foreground or the background.
 */

#include <string.h>
#include <math.h>
#include "allheaders.h"



/*-----------------------------------------------------------------------*
 *      Label pixels by an index for connected component membership      *
 *-----------------------------------------------------------------------*/
/*!
 *  pixConnCompTransform()
 *
 *      Input:   pixs (1 bpp)
 *               connect (connectivity: 4 or 8)
 *               depth (of pixd: 8 or 16 bpp; use 0 for auto determination)
 *      Return:  pixd (8 or 16 bpp), or null on error
 *
 *  Notes:
 *      (1) pixd is 8 or 16 bpp, and the pixel values label the fg component,
 *          starting with 1.  Pixels in the bg are labelled 0.
 *      (2) If @depth = 0, the depth of pixd is 8 if the number of c.c.
 *          is less than 254, and 16 otherwise.
 *      (3) If @depth = 8, the assigned label for the n-th component is
 *          1 + n % 254.  We use mod 254 because 0 is uniquely assigned
 *          to black: e.g., see pixcmapCreateRandom().  Likewise,
 *          if @depth = 16, the assigned label uses mod(2^16 - 2).
 */
PIX *
pixConnCompTransform(PIX     *pixs,
                     l_int32  connect,
                     l_int32  depth)
{
l_int32  i, n, index, w, h, xb, yb, wb, hb;
BOXA    *boxa;
PIX     *pix1, *pix2, *pixd;
PIXA    *pixa;

    PROCNAME("pixConnCompTransform");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (connect != 4 && connect != 8)
        return (PIX *)ERROR_PTR("connectivity must be 4 or 8", procName, NULL);
    if (depth != 0 && depth != 8 && depth != 16)
        return (PIX *)ERROR_PTR("depth must be 0, 8 or 16", procName, NULL);

    boxa = pixConnComp(pixs, &pixa, connect);
    n = pixaGetCount(pixa);
    boxaDestroy(&boxa);
    pixGetDimensions(pixs, &w, &h, NULL);
    if (depth == 0) {
        depth = (n < 254) ? 8 : 16;
    }
    pixd = pixCreate(w, h, depth);
    if (n == 0) {  /* no fg */
        pixaDestroy(&pixa);
        return pixd;
    }

       /* Label each component and blit it in */
    for (i = 0; i < n; i++) {
        pixaGetBoxGeometry(pixa, i, &xb, &yb, &wb, &hb);
        pix1 = pixaGetPix(pixa, i, L_CLONE);
        if (depth == 8) {
            index = 1 + (i % 254);
            pix2 = pixConvert1To8(NULL, pix1, 0, index);
        } else {  /* depth == 16 */
            index = 1 + (i % 0xfffe);
            pix2 = pixConvert1To16(NULL, pix1, 0, index);
        }
        pixRasterop(pixd, xb, yb, wb, hb, PIX_PAINT, pix2, 0, 0);
        pixDestroy(&pix1);
        pixDestroy(&pix2);
    }

    pixaDestroy(&pixa);
    return pixd;
}


/*-----------------------------------------------------------------------*
 *         Label pixels by the area of their connected component         *
 *-----------------------------------------------------------------------*/
/*!
 *  pixConnCompAreaTransform()
 *
 *      Input:   pixs (1 bpp)
 *               connect (connectivity: 4 or 8)
 *      Return:  pixd (16 bpp), or null on error
 *
 *  Notes:
 *      (1) The pixel values in pixd label the area of the fg component
 *          to which the pixel belongs.  Pixels in the bg are labelled 0.
 *      (2) The pixel values cannot exceed 2^16 - 1, even if the area
 *          of the c.c. is larger.
 *      (3) For purposes of visualization, the output can be converted
 *          to 8 bpp, using pixConvert16To8() or pixMaxDynamicRange().
 */
PIX *
pixConnCompAreaTransform(PIX     *pixs,
                         l_int32  connect)
{
l_int32   i, n, npix, w, h, xb, yb, wb, hb;
l_int32  *tab8;
BOXA     *boxa;
PIX      *pix1, *pix2, *pixd;
PIXA     *pixa;

    PROCNAME("pixConnCompTransform");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (connect != 4 && connect != 8)
        return (PIX *)ERROR_PTR("connectivity must be 4 or 8", procName, NULL);

    boxa = pixConnComp(pixs, &pixa, connect);
    n = pixaGetCount(pixa);
    boxaDestroy(&boxa);
    pixGetDimensions(pixs, &w, &h, NULL);
    pixd = pixCreate(w, h, 16);
    if (n == 0) {  /* no fg */
        pixaDestroy(&pixa);
        return pixd;
    }

       /* Label each component and blit it in */
    tab8 = makePixelSumTab8();
    for (i = 0; i < n; i++) {
        pixaGetBoxGeometry(pixa, i, &xb, &yb, &wb, &hb);
        pix1 = pixaGetPix(pixa, i, L_CLONE);
        pixCountPixels(pix1, &npix, tab8);
        npix = L_MIN(npix, 0xffff);
        pix2 = pixConvert1To16(NULL, pix1, 0, npix);
        pixRasterop(pixd, xb, yb, wb, hb, PIX_PAINT, pix2, 0, 0);
        pixDestroy(&pix1);
        pixDestroy(&pix2);
    }

    pixaDestroy(&pixa);
    FREE(tab8);
    return pixd;
}


/*-----------------------------------------------------------------------*
 *          Label pixels with spatially-dependent color coding           *
 *-----------------------------------------------------------------------*/
/*!
 *  pixLocToColorTransform()
 *
 *      Input:   pixs (1 bpp)
 *      Return:  pixd (32 bpp rgb), or null on error
 *
 *  Notes:
 *      (1) This generates an RGB image where each component value
 *          is coded depending on the (x.y) location and the size
 *          of the fg connected component that the pixel in pixs belongs to.
 *          It is independent of the 4-fold orthogonal orientation, and
 *          only weakly depends on translations and small angle rotations.
 *          Background pixels are black.
 *      (2) Such encodings can be compared between two 1 bpp images
 *          by performing this transform and calculating the
 *          "earth-mover" distance on the resulting R,G,B histograms.
 */
PIX *
pixLocToColorTransform(PIX  *pixs)
{
l_int32    w, h, w2, h2, wpls, wplr, wplg, wplb, wplcc, i, j, rval, gval, bval;
l_float32  invw2, invh2;
l_uint32  *datas, *datar, *datag, *datab, *datacc;
l_uint32  *lines, *liner, *lineg, *lineb, *linecc;
PIX       *pix1, *pixcc, *pixr, *pixg, *pixb, *pixd;

    PROCNAME("pixLocToColorTransform");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);

        /* Label each pixel with the area of the c.c. to which it belongs.
         * Clip the result to 255 in an 8 bpp pix. This is used for
         * the blue component of pixd.  */
    pixGetDimensions(pixs, &w, &h, NULL);
    w2 = w / 2;
    h2 = h / 2;
    invw2 = 255.0 / (l_float32)w2;
    invh2 = 255.0 / (l_float32)h2;
    pix1 = pixConnCompAreaTransform(pixs, 8);
    pixcc = pixConvert16To8(pix1, L_CLIP_TO_255);
    pixDestroy(&pix1);

        /* Label the red and green components depending on the location
         * of the fg pixels, in a way that is 4-fold rotationally invariant. */
    pixr = pixCreate(w, h, 8);
    pixg = pixCreate(w, h, 8);
    pixb = pixCreate(w, h, 8);
    wpls = pixGetWpl(pixs);
    wplr = pixGetWpl(pixr);
    wplg = pixGetWpl(pixg);
    wplb = pixGetWpl(pixb);
    wplcc = pixGetWpl(pixcc);
    datas = pixGetData(pixs);
    datar = pixGetData(pixr);
    datag = pixGetData(pixg);
    datab = pixGetData(pixb);
    datacc = pixGetData(pixcc);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        liner = datar + i * wplr;
        lineg = datag + i * wplg;
        lineb = datab + i * wplb;
        linecc = datacc+ i * wplcc;
        for (j = 0; j < w; j++) {
            if (GET_DATA_BIT(lines, j) == 0) continue;
            if (w < h) {
                rval = invh2 * L_ABS((l_float32)(i - h2));
                gval = invw2 * L_ABS((l_float32)(j - w2));
            } else {
                rval = invw2 * L_ABS((l_float32)(j - w2));
                gval = invh2 * L_ABS((l_float32)(i - h2));
            }
            bval = GET_DATA_BYTE(linecc, j);
            SET_DATA_BYTE(liner, j, rval);
            SET_DATA_BYTE(lineg, j, gval);
            SET_DATA_BYTE(lineb, j, bval);
        }
    }
    pixd = pixCreateRGBImage(pixr, pixg, pixb);

    pixDestroy(&pixcc);
    pixDestroy(&pixr);
    pixDestroy(&pixg);
    pixDestroy(&pixb);
    return pixd;
}

