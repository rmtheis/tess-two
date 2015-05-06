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
 *   boxfunc3.c
 *
 *      Boxa/Boxaa painting into pix
 *           PIX             *pixMaskConnComp()
 *           PIX             *pixMaskBoxa()
 *           PIX             *pixPaintBoxa()
 *           PIX             *pixSetBlackOrWhiteBoxa()
 *           PIX             *pixPaintBoxaRandom()
 *           PIX             *pixBlendBoxaRandom()
 *           PIX             *pixDrawBoxa()
 *           PIX             *pixDrawBoxaRandom()
 *           PIX             *boxaaDisplay()
 *
 *      Split mask components into Boxa
 *           BOXA            *pixSplitIntoBoxa()
 *           BOXA            *pixSplitComponentIntoBoxa()
 *           static l_int32   pixSearchForRectangle()
 *
 *      Represent horizontal or vertical mosaic strips
 *           BOXA            *makeMosaicStrips()
 *
 *      Comparison between boxa
 *           l_int32          boxaCompareRegions()
 *
 *  See summary in pixPaintBoxa() of various ways to paint and draw
 *  boxes on images.
 */

#include "allheaders.h"

static l_int32 pixSearchForRectangle(PIX *pixs, BOX *boxs, l_int32 minsum,
                                     l_int32 skipdist, l_int32 delta,
                                     l_int32 maxbg, l_int32 sideflag,
                                     BOXA *boxat, NUMA *nascore);

#ifndef NO_CONSOLE_IO
#define  DEBUG_SPLIT     0
#endif  /* ~NO_CONSOLE_IO */


/*---------------------------------------------------------------------*
 *                     Boxa/Boxaa painting into Pix                    *
 *---------------------------------------------------------------------*/
/*!
 *  pixMaskConnComp()
 *
 *      Input:  pixs (1 bpp)
 *              connectivity (4 or 8)
 *              &boxa (<optional return> bounding boxes of c.c.)
 *      Return: pixd (1 bpp mask over the c.c.), or null on error
 *
 *  Notes:
 *      (1) This generates a mask image with ON pixels over the
 *          b.b. of the c.c. in pixs.  If there are no ON pixels in pixs,
 *          pixd will also have no ON pixels.
 */
PIX *
pixMaskConnComp(PIX     *pixs,
                l_int32  connectivity,
                BOXA   **pboxa)
{
BOXA  *boxa;
PIX   *pixd;

    PROCNAME("pixMaskConnComp");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (connectivity != 4 && connectivity != 8)
        return (PIX *)ERROR_PTR("connectivity not 4 or 8", procName, NULL);

    boxa = pixConnComp(pixs, NULL, connectivity);
    pixd = pixCreateTemplate(pixs);
    if (boxaGetCount(boxa) != 0)
        pixMaskBoxa(pixd, pixd, boxa, L_SET_PIXELS);
    if (pboxa)
        *pboxa = boxa;
    else
        boxaDestroy(&boxa);
    return pixd;
}


/*!
 *  pixMaskBoxa()
 *
 *      Input:  pixd (<optional> may be null)
 *              pixs (any depth; not cmapped)
 *              boxa (of boxes, to paint)
 *              op (L_SET_PIXELS, L_CLEAR_PIXELS, L_FLIP_PIXELS)
 *      Return: pixd (with masking op over the boxes), or null on error
 *
 *  Notes:
 *      (1) This can be used with:
 *              pixd = NULL  (makes a new pixd)
 *              pixd = pixs  (in-place)
 *      (2) If pixd == NULL, this first makes a copy of pixs, and then
 *          bit-twiddles over the boxes.  Otherwise, it operates directly
 *          on pixs.
 *      (3) This simple function is typically used with 1 bpp images.
 *          It uses the 1-image rasterop function, rasteropUniLow(),
 *          to set, clear or flip the pixels in pixd.
 *      (4) If you want to generate a 1 bpp mask of ON pixels from the boxes
 *          in a Boxa, in a pix of size (w,h):
 *              pix = pixCreate(w, h, 1);
 *              pixMaskBoxa(pix, pix, boxa, L_SET_PIXELS);
 */
PIX *
pixMaskBoxa(PIX     *pixd,
            PIX     *pixs,
            BOXA    *boxa,
            l_int32  op)
{
l_int32  i, n, x, y, w, h;
BOX     *box;

    PROCNAME("pixMaskBoxa");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs is cmapped", procName, NULL);
    if (pixd && (pixd != pixs))
        return (PIX *)ERROR_PTR("if pixd, must be in-place", procName, NULL);
    if (!boxa)
        return (PIX *)ERROR_PTR("boxa not defined", procName, NULL);
    if (op != L_SET_PIXELS && op != L_CLEAR_PIXELS && op != L_FLIP_PIXELS)
        return (PIX *)ERROR_PTR("invalid op", procName, NULL);

    pixd = pixCopy(pixd, pixs);
    if ((n = boxaGetCount(boxa)) == 0) {
        L_WARNING("no boxes to mask\n", procName);
        return pixd;
    }

    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        boxGetGeometry(box, &x, &y, &w, &h);
        if (op == L_SET_PIXELS)
            pixRasterop(pixd, x, y, w, h, PIX_SET, NULL, 0, 0);
        else if (op == L_CLEAR_PIXELS)
            pixRasterop(pixd, x, y, w, h, PIX_CLR, NULL, 0, 0);
        else  /* op == L_FLIP_PIXELS */
            pixRasterop(pixd, x, y, w, h, PIX_NOT(PIX_DST), NULL, 0, 0);
        boxDestroy(&box);
    }

    return pixd;
}


/*!
 *  pixPaintBoxa()
 *
 *      Input:  pixs (any depth, can be cmapped)
 *              boxa (of boxes, to paint)
 *              val (rgba color to paint)
 *      Return: pixd (with painted boxes), or null on error
 *
 *  Notes:
 *      (1) If pixs is 1 bpp or is colormapped, it is converted to 8 bpp
 *          and the boxa is painted using a colormap; otherwise,
 *          it is converted to 32 bpp rgb.
 *      (2) There are several ways to display a box on an image:
 *            * Paint it as a solid color
 *            * Draw the outline
 *            * Blend the outline or region with the existing image
 *          We provide painting and drawing here; blending is in blend.c.
 *          When painting or drawing, the result can be either a
 *          cmapped image or an rgb image.  The dest will be cmapped
 *          if the src is either 1 bpp or has a cmap that is not full.
 *          To force RGB output, use pixConvertTo8(pixs, FALSE)
 *          before calling any of these paint and draw functions.
 */
PIX *
pixPaintBoxa(PIX      *pixs,
             BOXA     *boxa,
             l_uint32  val)
{
l_int32   i, n, d, rval, gval, bval, newindex;
l_int32   mapvacancy;   /* true only if cmap and not full */
BOX      *box;
PIX      *pixd;
PIXCMAP  *cmap;

    PROCNAME("pixPaintBoxa");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!boxa)
        return (PIX *)ERROR_PTR("boxa not defined", procName, NULL);

    if ((n = boxaGetCount(boxa)) == 0) {
        L_WARNING("no boxes to paint; returning a copy\n", procName);
        return pixCopy(NULL, pixs);
    }

    mapvacancy = FALSE;
    if ((cmap = pixGetColormap(pixs)) != NULL) {
        if (pixcmapGetCount(cmap) < 256)
            mapvacancy = TRUE;
    }
    if (pixGetDepth(pixs) == 1 || mapvacancy)
        pixd = pixConvertTo8(pixs, TRUE);
    else
        pixd = pixConvertTo32(pixs);
    if (!pixd)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

    d = pixGetDepth(pixd);
    if (d == 8) {  /* colormapped */
        cmap = pixGetColormap(pixd);
        extractRGBValues(val, &rval, &gval, &bval);
        if (pixcmapAddNewColor(cmap, rval, gval, bval, &newindex))
            return (PIX *)ERROR_PTR("cmap full; can't add", procName, NULL);
    }

    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        if (d == 8)
            pixSetInRectArbitrary(pixd, box, newindex);
        else
            pixSetInRectArbitrary(pixd, box, val);
        boxDestroy(&box);
    }

    return pixd;
}


/*!
 *  pixSetBlackOrWhiteBoxa()
 *
 *      Input:  pixs (any depth, can be cmapped)
 *              boxa (<optional> of boxes, to clear or set)
 *              op (L_SET_BLACK, L_SET_WHITE)
 *      Return: pixd (with boxes filled with white or black), or null on error
 */
PIX *
pixSetBlackOrWhiteBoxa(PIX     *pixs,
                       BOXA    *boxa,
                       l_int32  op)
{
l_int32   i, n, d, index;
l_uint32  color;
BOX      *box;
PIX      *pixd;
PIXCMAP  *cmap;

    PROCNAME("pixSetBlackOrWhiteBoxa");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!boxa)
        return pixCopy(NULL, pixs);
    if ((n = boxaGetCount(boxa)) == 0)
        return pixCopy(NULL, pixs);

    pixd = pixCopy(NULL, pixs);
    d = pixGetDepth(pixd);
    if (d == 1) {
        for (i = 0; i < n; i++) {
            box = boxaGetBox(boxa, i, L_CLONE);
            if (op == L_SET_WHITE)
                pixClearInRect(pixd, box);
            else
                pixSetInRect(pixd, box);
            boxDestroy(&box);
        }
        return pixd;
    }

    cmap = pixGetColormap(pixs);
    if (cmap) {
        color = (op == L_SET_WHITE) ? 1 : 0;
        pixcmapAddBlackOrWhite(cmap, color, &index);
    } else if (d == 8) {
        color = (op == L_SET_WHITE) ? 0xff : 0x0;
    } else if (d == 32) {
        color = (op == L_SET_WHITE) ? 0xffffff00 : 0x0;
    } else if (d == 2) {
        color = (op == L_SET_WHITE) ? 0x3 : 0x0;
    } else if (d == 4) {
        color = (op == L_SET_WHITE) ? 0xf : 0x0;
    } else if (d == 16) {
        color = (op == L_SET_WHITE) ? 0xffff : 0x0;
    } else {
        pixDestroy(&pixd);
        return (PIX *)ERROR_PTR("invalid depth", procName, NULL);
    }

    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        if (cmap)
            pixSetInRectArbitrary(pixd, box, index);
        else
            pixSetInRectArbitrary(pixd, box, color);
        boxDestroy(&box);
    }

    return pixd;
}


/*!
 *  pixPaintBoxaRandom()
 *
 *      Input:  pixs (any depth, can be cmapped)
 *              boxa (of boxes, to paint)
 *      Return: pixd (with painted boxes), or null on error
 *
 *  Notes:
 *      (1) If pixs is 1 bpp, we paint the boxa using a colormap;
 *          otherwise, we convert to 32 bpp.
 *      (2) We use up to 254 different colors for painting the regions.
 *      (3) If boxes overlap, the later ones paint over earlier ones.
 */
PIX *
pixPaintBoxaRandom(PIX   *pixs,
                   BOXA  *boxa)
{
l_int32   i, n, d, rval, gval, bval, index;
l_uint32  val;
BOX      *box;
PIX      *pixd;
PIXCMAP  *cmap;

    PROCNAME("pixPaintBoxaRandom");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!boxa)
        return (PIX *)ERROR_PTR("boxa not defined", procName, NULL);

    if ((n = boxaGetCount(boxa)) == 0) {
        L_WARNING("no boxes to paint; returning a copy\n", procName);
        return pixCopy(NULL, pixs);
    }

    if (pixGetDepth(pixs) == 1)
        pixd = pixConvert1To8(NULL, pixs, 255, 0);
    else
        pixd = pixConvertTo32(pixs);
    if (!pixd)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

    cmap = pixcmapCreateRandom(8, 1, 1);
    d = pixGetDepth(pixd);
    if (d == 8)  /* colormapped */
        pixSetColormap(pixd, cmap);

    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        index = 1 + (i % 254);
        if (d == 8) {
            pixSetInRectArbitrary(pixd, box, index);
        } else {  /* d == 32 */
            pixcmapGetColor(cmap, index, &rval, &gval, &bval);
            composeRGBPixel(rval, gval, bval, &val);
            pixSetInRectArbitrary(pixd, box, val);
        }
        boxDestroy(&box);
    }

    if (d == 32)
        pixcmapDestroy(&cmap);
    return pixd;
}


/*!
 *  pixBlendBoxaRandom()
 *
 *      Input:  pixs (any depth; can be cmapped)
 *              boxa (of boxes, to blend/paint)
 *              fract (of box color to use)
 *      Return: pixd (32 bpp, with blend/painted boxes), or null on error
 *
 *  Notes:
 *      (1) pixs is converted to 32 bpp.
 *      (2) This differs from pixPaintBoxaRandom(), in that the
 *          colors here are blended with the color of pixs.
 *      (3) We use up to 254 different colors for painting the regions.
 *      (4) If boxes overlap, the final color depends only on the last
 *          rect that is used.
 */
PIX *
pixBlendBoxaRandom(PIX       *pixs,
                   BOXA      *boxa,
                   l_float32  fract)
{
l_int32   i, n, rval, gval, bval, index;
l_uint32  val;
BOX      *box;
PIX      *pixd;
PIXCMAP  *cmap;

    PROCNAME("pixBlendBoxaRandom");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!boxa)
        return (PIX *)ERROR_PTR("boxa not defined", procName, NULL);
    if (fract < 0.0 || fract > 1.0) {
        L_WARNING("fract must be in [0.0, 1.0]; setting to 0.5\n", procName);
        fract = 0.5;
    }

    if ((n = boxaGetCount(boxa)) == 0) {
        L_WARNING("no boxes to paint; returning a copy\n", procName);
        return pixCopy(NULL, pixs);
    }

    if ((pixd = pixConvertTo32(pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixd not defined", procName, NULL);

    cmap = pixcmapCreateRandom(8, 1, 1);
    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        index = 1 + (i % 254);
        pixcmapGetColor(cmap, index, &rval, &gval, &bval);
        composeRGBPixel(rval, gval, bval, &val);
        pixBlendInRect(pixd, box, val, fract);
        boxDestroy(&box);
    }

    pixcmapDestroy(&cmap);
    return pixd;
}


/*!
 *  pixDrawBoxa()
 *
 *      Input:  pixs (any depth; can be cmapped)
 *              boxa (of boxes, to draw)
 *              width (of lines)
 *              val (rgba color to draw)
 *      Return: pixd (with outlines of boxes added), or null on error
 *
 *  Notes:
 *      (1) If pixs is 1 bpp or is colormapped, it is converted to 8 bpp
 *          and the boxa is drawn using a colormap; otherwise,
 *          it is converted to 32 bpp rgb.
 */
PIX *
pixDrawBoxa(PIX      *pixs,
            BOXA     *boxa,
            l_int32   width,
            l_uint32  val)
{
l_int32   rval, gval, bval, newindex;
l_int32   mapvacancy;   /* true only if cmap and not full */
PIX      *pixd;
PIXCMAP  *cmap;

    PROCNAME("pixDrawBoxa");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!boxa)
        return (PIX *)ERROR_PTR("boxa not defined", procName, NULL);
    if (width < 1)
        return (PIX *)ERROR_PTR("width must be >= 1", procName, NULL);

    if (boxaGetCount(boxa) == 0) {
        L_WARNING("no boxes to draw; returning a copy\n", procName);
        return pixCopy(NULL, pixs);
    }

    mapvacancy = FALSE;
    if ((cmap = pixGetColormap(pixs)) != NULL) {
        if (pixcmapGetCount(cmap) < 256)
            mapvacancy = TRUE;
    }
    if (pixGetDepth(pixs) == 1 || mapvacancy)
        pixd = pixConvertTo8(pixs, TRUE);
    else
        pixd = pixConvertTo32(pixs);
    if (!pixd)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

    extractRGBValues(val, &rval, &gval, &bval);
    if (pixGetDepth(pixd) == 8) {  /* colormapped */
        cmap = pixGetColormap(pixd);
        pixcmapAddNewColor(cmap, rval, gval, bval, &newindex);
    }

    pixRenderBoxaArb(pixd, boxa, width, rval, gval, bval);
    return pixd;
}


/*!
 *  pixDrawBoxaRandom()
 *
 *      Input:  pixs (any depth, can be cmapped)
 *              boxa (of boxes, to draw)
 *              width (thickness of line)
 *      Return: pixd (with box outlines drawn), or null on error
 *
 *  Notes:
 *      (1) If pixs is 1 bpp, we draw the boxa using a colormap;
 *          otherwise, we convert to 32 bpp.
 *      (2) We use up to 254 different colors for drawing the boxes.
 *      (3) If boxes overlap, the later ones draw over earlier ones.
 */
PIX *
pixDrawBoxaRandom(PIX     *pixs,
                  BOXA    *boxa,
                  l_int32  width)
{
l_int32   i, n, rval, gval, bval, index;
BOX      *box;
PIX      *pixd;
PIXCMAP  *cmap;
PTAA     *ptaa;

    PROCNAME("pixDrawBoxaRandom");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!boxa)
        return (PIX *)ERROR_PTR("boxa not defined", procName, NULL);
    if (width < 1)
        return (PIX *)ERROR_PTR("width must be >= 1", procName, NULL);

    if ((n = boxaGetCount(boxa)) == 0) {
        L_WARNING("no boxes to draw; returning a copy\n", procName);
        return pixCopy(NULL, pixs);
    }

        /* Input depth = 1 bpp; generate cmapped output */
    if (pixGetDepth(pixs) == 1) {
        ptaa = generatePtaaBoxa(boxa);
        pixd = pixRenderRandomCmapPtaa(pixs, ptaa, 1, width, 1);
        ptaaDestroy(&ptaa);
        return pixd;
    }

        /* Generate rgb output */
    pixd = pixConvertTo32(pixs);
    cmap = pixcmapCreateRandom(8, 1, 1);
    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        index = 1 + (i % 254);
        pixcmapGetColor(cmap, index, &rval, &gval, &bval);
        pixRenderBoxArb(pixd, box, width, rval, gval, bval);
        boxDestroy(&box);
    }
    pixcmapDestroy(&cmap);
    return pixd;
}


/*!
 *  boxaaDisplay()
 *
 *      Input:  baa
 *              linewba (line width to display boxa)
 *              linewb (line width to display box)
 *              colorba (color to display boxa)
 *              colorb (color to display box)
 *              w (of pix; use 0 if determined by baa)
 *              h (of pix; use 0 if determined by baa)
 *      Return: 0 if OK, 1 on error
 */
PIX *
boxaaDisplay(BOXAA    *baa,
             l_int32   linewba,
             l_int32   linewb,
             l_uint32  colorba,
             l_uint32  colorb,
             l_int32   w,
             l_int32   h)
{
l_int32   i, j, n, m, rbox, gbox, bbox, rboxa, gboxa, bboxa;
BOX      *box;
BOXA     *boxa;
PIX      *pix;
PIXCMAP  *cmap;

    PROCNAME("boxaaDisplay");

    if (!baa)
        return (PIX *)ERROR_PTR("baa not defined", procName, NULL);
    if (w == 0 || h == 0)
        boxaaGetExtent(baa, &w, &h, NULL, NULL);

    pix = pixCreate(w, h, 8);
    cmap = pixcmapCreate(8);
    pixSetColormap(pix, cmap);
    extractRGBValues(colorb, &rbox, &gbox, &bbox);
    extractRGBValues(colorba, &rboxa, &gboxa, &bboxa);
    pixcmapAddColor(cmap, 255, 255, 255);
    pixcmapAddColor(cmap, rbox, gbox, bbox);
    pixcmapAddColor(cmap, rboxa, gboxa, bboxa);

    n = boxaaGetCount(baa);
    for (i = 0; i < n; i++) {
        boxa = boxaaGetBoxa(baa, i, L_CLONE);
        boxaGetExtent(boxa, NULL, NULL, &box);
        pixRenderBoxArb(pix, box, linewba, rboxa, gboxa, bboxa);
        boxDestroy(&box);
        m = boxaGetCount(boxa);
        for (j = 0; j < m; j++) {
            box = boxaGetBox(boxa, j, L_CLONE);
            pixRenderBoxArb(pix, box, linewb, rbox, gbox, bbox);
            boxDestroy(&box);
        }
        boxaDestroy(&boxa);
    }

    return pix;
}


/*---------------------------------------------------------------------*
 *                   Split mask components into Boxa                   *
 *---------------------------------------------------------------------*/
/*!
 *  pixSplitIntoBoxa()
 *
 *      Input:  pixs (1 bpp)
 *              minsum  (minimum pixels to trigger propagation)
 *              skipdist (distance before computing sum for propagation)
 *              delta (difference required to stop propagation)
 *              maxbg (maximum number of allowed bg pixels in ref scan)
 *              maxcomps (use 0 for unlimited number of subdivided components)
 *              remainder (set to 1 to get b.b. of remaining stuff)
 *      Return: boxa (of rectangles covering the fg of pixs), or null on error
 *
 *  Notes:
 *      (1) This generates a boxa of rectangles that covers
 *          the fg of a mask.  For each 8-connected component in pixs,
 *          it does a greedy partitioning, choosing the largest
 *          rectangle found from each of the four directions at each iter.
 *          See pixSplitComponentIntoBoxa() for details.
 *      (2) The input parameters give some flexibility for boundary
 *          noise.  The resulting set of rectangles may cover some
 *          bg pixels.
 *      (3) This should be used when there are a small number of
 *          mask components, each of which has sides that are close
 *          to horizontal and vertical.  The input parameters @delta
 *          and @maxbg determine whether or not holes in the mask are covered.
 *      (4) The parameter @maxcomps gives the maximum number of allowed
 *          rectangles extracted from any single connected component.
 *          Use 0 if no limit is to be applied.
 *      (5) The flag @remainder specifies whether we take a final bounding
 *          box for anything left after the maximum number of allowed
 *          rectangle is extracted.
 */
BOXA *
pixSplitIntoBoxa(PIX     *pixs,
                 l_int32  minsum,
                 l_int32  skipdist,
                 l_int32  delta,
                 l_int32  maxbg,
                 l_int32  maxcomps,
                 l_int32  remainder)
{
l_int32  i, n;
BOX     *box;
BOXA    *boxa, *boxas, *boxad;
PIX     *pix;
PIXA    *pixas;

    PROCNAME("pixSplitIntoBoxa");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (BOXA *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);

    boxas = pixConnComp(pixs, &pixas, 8);
    n = boxaGetCount(boxas);
    boxad = boxaCreate(0);
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixas, i, L_CLONE);
        box = boxaGetBox(boxas, i, L_CLONE);
        boxa = pixSplitComponentIntoBoxa(pix, box, minsum, skipdist,
                                         delta, maxbg, maxcomps, remainder);
        boxaJoin(boxad, boxa, 0, -1);
        pixDestroy(&pix);
        boxDestroy(&box);
        boxaDestroy(&boxa);
    }

    pixaDestroy(&pixas);
    boxaDestroy(&boxas);
    return boxad;
}


/*!
 *  pixSplitComponentIntoBoxa()
 *
 *      Input:  pixs (1 bpp)
 *              box (<optional> location of pixs w/rt an origin)
 *              minsum  (minimum pixels to trigger propagation)
 *              skipdist (distance before computing sum for propagation)
 *              delta (difference required to stop propagation)
 *              maxbg (maximum number of allowed bg pixels in ref scan)
 *              maxcomps (use 0 for unlimited number of subdivided components)
 *              remainder (set to 1 to get b.b. of remaining stuff)
 *      Return: boxa (of rectangles covering the fg of pixs), or null on error
 *
 *  Notes:
 *      (1) This generates a boxa of rectangles that covers
 *          the fg of a mask.  It does so by a greedy partitioning of
 *          the mask, choosing the largest rectangle found from
 *          each of the four directions at each step.
 *      (2) The input parameters give some flexibility for boundary
 *          noise.  The resulting set of rectangles must cover all
 *          the fg pixels and, in addition, may cover some bg pixels.
 *          Using small input parameters on a noiseless mask (i.e., one
 *          that has only large vertical and horizontal edges) will
 *          result in a proper covering of only the fg pixels of the mask.
 *      (3) The input is assumed to be a single connected component, that
 *          may have holes.  From each side, sweep inward, counting
 *          the pixels.  If the count becomes greater than @minsum,
 *          and we have moved forward a further amount @skipdist,
 *          record that count ('countref'), but don't accept if the scan
 *          contains more than @maxbg bg pixels.  Continue the scan
 *          until we reach a count that differs from countref by at
 *          least @delta, at which point the propagation stops.  The box
 *          swept out gets a score, which is the sum of fg pixels
 *          minus a penalty.  The penalty is the number of bg pixels
 *          in the box.  This is done from all four sides, and the
 *          side with the largest score is saved as a rectangle.
 *          The process repeats until there is either no rectangle
 *          left, or there is one that can't be captured from any
 *          direction.  For the latter case, we simply accept the
 *          last rectangle.
 *      (4) The input box is only used to specify the location of
 *          the UL corner of pixs, with respect to an origin that
 *          typically represents the UL corner of an underlying image,
 *          of which pixs is one component.  If @box is null,
 *          the UL corner is taken to be (0, 0).
 *      (5) The parameter @maxcomps gives the maximum number of allowed
 *          rectangles extracted from any single connected component.
 *          Use 0 if no limit is to be applied.
 *      (6) The flag @remainder specifies whether we take a final bounding
 *          box for anything left after the maximum number of allowed
 *          rectangle is extracted.
 *      (7) So if @maxcomps > 0, it specifies that we want no more than
 *          the first @maxcomps rectangles that satisfy the input
 *          criteria.  After this, we can get a final rectangle that
 *          bounds everything left over by setting @remainder == 1.
 *          If @remainder == 0, we only get rectangles that satisfy
 *          the input criteria.
 *      (8) It should be noted that the removal of rectangles can
 *          break the original c.c. into several c.c.
 *      (9) Summing up:
 *            * If @maxcomp == 0, the splitting proceeds as far as possible.
 *            * If @maxcomp > 0, the splitting stops when @maxcomps are
 *                found, or earlier if no more components can be selected.
 *            * If @remainder == 1 and components remain that cannot be
 *                selected, they are returned as a single final rectangle;
 *                otherwise, they are ignored.
 */
BOXA *
pixSplitComponentIntoBoxa(PIX     *pix,
                          BOX     *box,
                          l_int32  minsum,
                          l_int32  skipdist,
                          l_int32  delta,
                          l_int32  maxbg,
                          l_int32  maxcomps,
                          l_int32  remainder)
{
l_int32  i, w, h, boxx, boxy, bx, by, bw, bh, maxdir, maxscore;
l_int32  iter;
BOX     *boxs;  /* shrinks as rectangular regions are removed */
BOX     *boxt1, *boxt2, *boxt3;
BOXA    *boxat;  /* stores rectangle data for each side in an iteration */
BOXA    *boxad;
NUMA    *nascore, *nas;
PIX     *pixs;

    PROCNAME("pixSplitComponentIntoBoxa");

    if (!pix || pixGetDepth(pix) != 1)
        return (BOXA *)ERROR_PTR("pix undefined or not 1 bpp", procName, NULL);

    pixs = pixCopy(NULL, pix);
    pixGetDimensions(pixs, &w, &h, NULL);
    if (box)
        boxGetGeometry(box, &boxx, &boxy, NULL, NULL);
    else
        boxx = boxy = 0;
    boxs = boxCreate(0, 0, w, h);
    boxad = boxaCreate(0);

    iter = 0;
    while (boxs != NULL) {
        boxGetGeometry(boxs, &bx, &by, &bw, &bh);
        boxat = boxaCreate(4);  /* potential rectangular regions */
        nascore = numaCreate(4);
        for (i = 0; i < 4; i++) {
            pixSearchForRectangle(pixs, boxs, minsum, skipdist, delta, maxbg,
                                  i, boxat, nascore);
        }
        nas = numaGetSortIndex(nascore, L_SORT_DECREASING);
        numaGetIValue(nas, 0, &maxdir);
        numaGetIValue(nascore, maxdir, &maxscore);
#if  DEBUG_SPLIT
        fprintf(stderr, "Iteration: %d\n", iter);
        boxPrintStreamInfo(stderr, boxs);
        boxaWriteStream(stderr, boxat);
        fprintf(stderr, "\nmaxdir = %d, maxscore = %d\n\n", maxdir, maxscore);
#endif  /* DEBUG_SPLIT */
        if (maxscore > 0) {  /* accept this */
            boxt1 = boxaGetBox(boxat, maxdir, L_CLONE);
            boxt2 = boxTransform(boxt1, boxx, boxy, 1.0, 1.0);
            boxaAddBox(boxad, boxt2, L_INSERT);
            pixClearInRect(pixs, boxt1);
            boxDestroy(&boxt1);
            pixClipBoxToForeground(pixs, boxs, NULL, &boxt3);
            boxDestroy(&boxs);
            boxs = boxt3;
            if (boxs) {
                boxGetGeometry(boxs, NULL, NULL, &bw, &bh);
                if (bw < 2 || bh < 2)
                    boxDestroy(&boxs);  /* we're done */
            }
        } else {  /* no more valid rectangles can be found */
            if (remainder == 1) {  /* save the last box */
                boxt1 = boxTransform(boxs, boxx, boxy, 1.0, 1.0);
                boxaAddBox(boxad, boxt1, L_INSERT);
            }
            boxDestroy(&boxs);  /* we're done */
        }
        boxaDestroy(&boxat);
        numaDestroy(&nascore);
        numaDestroy(&nas);

        iter++;
        if ((iter == maxcomps) && boxs) {
            if (remainder == 1) {  /* save the last box */
                boxt1 = boxTransform(boxs, boxx, boxy, 1.0, 1.0);
                boxaAddBox(boxad, boxt1, L_INSERT);
            }
            boxDestroy(&boxs);  /* we're done */
        }
    }

    pixDestroy(&pixs);
    return boxad;
}


/*!
 *  pixSearchForRectangle()
 *
 *      Input:  pixs (1 bpp)
 *              boxs (current region to investigate)
 *              minsum  (minimum pixels to trigger propagation)
 *              skipdist (distance before computing sum for propagation)
 *              delta (difference required to stop propagation)
 *              maxbg (maximum number of allowed bg pixels in ref scan)
 *              sideflag (side to search from)
 *              boxat (add result of rectangular region found here)
 *              nascore (add score for this rectangle here)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) See pixSplitComponentIntoBoxa() for an explanation of the algorithm.
 *          This does the sweep from a single side.  For each iteration
 *          in pixSplitComponentIntoBoxa(), this will be called 4 times,
 *          for @sideflag = {0, 1, 2, 3}.
 *      (2) If a valid rectangle is not found, add a score of 0 and
 *          input a minimum box.
 */
static l_int32
pixSearchForRectangle(PIX     *pixs,
                      BOX     *boxs,
                      l_int32  minsum,
                      l_int32  skipdist,
                      l_int32  delta,
                      l_int32  maxbg,
                      l_int32  sideflag,
                      BOXA    *boxat,
                      NUMA    *nascore)
{
l_int32  bx, by, bw, bh, width, height, setref, atref;
l_int32  minincol, maxincol, mininrow, maxinrow, minval, maxval, bgref;
l_int32  x, y, x0, y0, xref, yref, colsum, rowsum, score, countref, diff;
void   **lines1;
BOX     *boxr;

    PROCNAME("pixSearchForRectangle");

    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs undefined or not 1 bpp", procName, 1);
    if (!boxs)
        return ERROR_INT("boxs not defined", procName, 1);
    if (!boxat)
        return ERROR_INT("boxat not defined", procName, 1);
    if (!nascore)
        return ERROR_INT("nascore not defined", procName, 1);

    lines1 = pixGetLinePtrs(pixs, NULL);
    boxGetGeometry(boxs, &bx, &by, &bw, &bh);
    boxr = NULL;
    setref = 0;
    atref = 0;
    maxval = 0;
    minval = 100000;
    score = 0;  /* sum of all (fg - bg) pixels seen in the scan */
    xref = yref = 100000;  /* init to impossibly big number */
    if (sideflag == L_FROM_LEFT) {
        for (x = bx; x < bx + bw; x++) {
            colsum = 0;
            maxincol = 0;
            minincol = 100000;
            for (y = by; y < by + bh; y++) {
                if (GET_DATA_BIT(lines1[y], x)) {
                    colsum++;
                    if (y > maxincol) maxincol = y;
                    if (y < minincol) minincol = y;
                }
            }
            score += colsum;

                /* Enough fg to sweep out a rectangle? */
            if (!setref && colsum >= minsum) {
                setref = 1;
                xref = x + 10;
                if (xref >= bx + bw)
                    goto failure;
            }

                /* Reached the reference line; save the count;
                 * if there is too much bg, the rectangle is invalid. */
            if (setref && x == xref) {
                atref = 1;
                countref = colsum;
                bgref = maxincol - minincol + 1 - countref;
                if (bgref > maxbg)
                    goto failure;
            }

                /* Have we left the rectangle?  If so, save it along
                 * with the score. */
            if (atref) {
                diff = L_ABS(colsum - countref);
                if (diff >= delta || x == bx + bw - 1) {
                    height = maxval - minval + 1;
                    width = x - bx;
                    if (x == bx + bw - 1) width = x - bx + 1;
                    boxr = boxCreate(bx, minval, width, height);
                    score = 2 * score - width * height;
                    goto success;
                }
            }
            maxval = L_MAX(maxval, maxincol);
            minval = L_MIN(minval, minincol);
        }
        goto failure;
    } else if (sideflag == L_FROM_RIGHT) {
        for (x = bx + bw - 1; x >= bx; x--) {
            colsum = 0;
            maxincol = 0;
            minincol = 100000;
            for (y = by; y < by + bh; y++) {
                if (GET_DATA_BIT(lines1[y], x)) {
                    colsum++;
                    if (y > maxincol) maxincol = y;
                    if (y < minincol) minincol = y;
                }
            }
            score += colsum;
            if (!setref && colsum >= minsum) {
                setref = 1;
                xref = x - 10;
                if (xref < bx)
                    goto failure;
            }
            if (setref && x == xref) {
                atref = 1;
                countref = colsum;
                bgref = maxincol - minincol + 1 - countref;
                if (bgref > maxbg)
                    goto failure;
            }
            if (atref) {
                diff = L_ABS(colsum - countref);
                if (diff >= delta || x == bx) {
                    height = maxval - minval + 1;
                    x0 = x + 1;
                    if (x == bx) x0 = x;
                    width = bx + bw - x0;
                    boxr = boxCreate(x0, minval, width, height);
                    score = 2 * score - width * height;
                    goto success;
                }
            }
            maxval = L_MAX(maxval, maxincol);
            minval = L_MIN(minval, minincol);
        }
        goto failure;
    } else if (sideflag == L_FROM_TOP) {
        for (y = by; y < by + bh; y++) {
            rowsum = 0;
            maxinrow = 0;
            mininrow = 100000;
            for (x = bx; x < bx + bw; x++) {
                if (GET_DATA_BIT(lines1[y], x)) {
                    rowsum++;
                    if (x > maxinrow) maxinrow = x;
                    if (x < mininrow) mininrow = x;
                }
            }
            score += rowsum;
            if (!setref && rowsum >= minsum) {
                setref = 1;
                yref = y + 10;
                if (yref >= by + bh)
                    goto failure;
            }
            if (setref && y == yref) {
                atref = 1;
                countref = rowsum;
                bgref = maxinrow - mininrow + 1 - countref;
                if (bgref > maxbg)
                    goto failure;
            }
            if (atref) {
                diff = L_ABS(rowsum - countref);
                if (diff >= delta || y == by + bh - 1) {
                    width = maxval - minval + 1;
                    height = y - by;
                    if (y == by + bh - 1) height = y - by + 1;
                    boxr = boxCreate(minval, by, width, height);
                    score = 2 * score - width * height;
                    goto success;
                }
            }
            maxval = L_MAX(maxval, maxinrow);
            minval = L_MIN(minval, mininrow);
        }
        goto failure;
    } else if (sideflag == L_FROM_BOT) {
        for (y = by + bh - 1; y >= by; y--) {
            rowsum = 0;
            maxinrow = 0;
            mininrow = 100000;
            for (x = bx; x < bx + bw; x++) {
                if (GET_DATA_BIT(lines1[y], x)) {
                    rowsum++;
                    if (x > maxinrow) maxinrow = x;
                    if (x < mininrow) mininrow = x;
                }
            }
            score += rowsum;
            if (!setref && rowsum >= minsum) {
                setref = 1;
                yref = y - 10;
                if (yref < by)
                    goto failure;
            }
            if (setref && y == yref) {
                atref = 1;
                countref = rowsum;
                bgref = maxinrow - mininrow + 1 - countref;
                if (bgref > maxbg)
                    goto failure;
            }
            if (atref) {
                diff = L_ABS(rowsum - countref);
                if (diff >= delta || y == by) {
                    width = maxval - minval + 1;
                    y0 = y + 1;
                    if (y == by) y0 = y;
                    height = by + bh - y0;
                    boxr = boxCreate(minval, y0, width, height);
                    score = 2 * score - width * height;
                    goto success;
                }
            }
            maxval = L_MAX(maxval, maxinrow);
            minval = L_MIN(minval, mininrow);
        }
        goto failure;
    }

failure:
    numaAddNumber(nascore, 0);
    boxaAddBox(boxat, boxCreate(0, 0, 1, 1), L_INSERT);  /* min box */
    FREE(lines1);
    return 0;

success:
    numaAddNumber(nascore, score);
    boxaAddBox(boxat, boxr, L_INSERT);
    FREE(lines1);
    return 0;
}


/*---------------------------------------------------------------------*
 *             Represent horizontal or vertical mosaic strips          *
 *---------------------------------------------------------------------*/
/*!
 *  makeMosaicStrips()
 *
 *      Input:  w, h
 *              direction (L_SCAN_HORIZONTAL or L_SCAN_VERTICAL)
 *              size (of strips in the scan direction)
 *      Return: boxa, or null on error
 *
 *  Notes:
 *      (1) For example, this can be used to generate a pixa of
 *          vertical strips of width 10 from an image, using:
 *             pixGetDimensions(pix, &w, &h, NULL);
 *             boxa = makeMosaicStrips(w, h, L_SCAN_HORIZONTAL, 10);
 *             pixa = pixClipRectangles(pix, boxa);
 *          All strips except the last will be the same width.  The
 *          last strip will have width w % 10.
 */
BOXA *
makeMosaicStrips(l_int32  w,
                 l_int32  h,
                 l_int32  direction,
                 l_int32  size)
{
l_int32  i, nstrips, extra;
BOX     *box;
BOXA    *boxa;

    PROCNAME("makeMosaicStrips");

    if (w < 1 || h < 1)
        return (BOXA *)ERROR_PTR("invalid w or h", procName, NULL);
    if (direction != L_SCAN_HORIZONTAL && direction != L_SCAN_VERTICAL)
        return (BOXA *)ERROR_PTR("invalid direction", procName, NULL);
    if (size < 1)
        return (BOXA *)ERROR_PTR("size < 1", procName, NULL);

    boxa = boxaCreate(0);
    if (direction == L_SCAN_HORIZONTAL) {
        nstrips = w / size;
        for (i = 0; i < nstrips; i++) {
            box = boxCreate(i * size, 0, size, h);
            boxaAddBox(boxa, box, L_INSERT);
        }
        if ((extra = w % size) > 0) {
            box = boxCreate(nstrips * size, 0, extra, h);
            boxaAddBox(boxa, box, L_INSERT);
        }
    } else {
        nstrips = h / size;
        for (i = 0; i < nstrips; i++) {
            box = boxCreate(0, i * size, w, size);
            boxaAddBox(boxa, box, L_INSERT);
        }
        if ((extra = h % size) > 0) {
            box = boxCreate(0, nstrips * size, w, extra);
            boxaAddBox(boxa, box, L_INSERT);
        }
    }
    return boxa;
}


/*---------------------------------------------------------------------*
 *                        Comparison between boxa                      *
 *---------------------------------------------------------------------*/
/*!
 *  boxaCompareRegions()
 *
 *      Input:  boxa1, boxa2
 *              areathresh (minimum area of boxes to be considered)
 *              &pnsame  (<return> true if same number of boxes)
 *              &pdiffarea (<return> fractional difference in total area)
 *              &pdiffxor (<optional return> fractional difference
 *                         in xor of regions)
 *              &pixdb (<optional return> debug pix showing two boxa)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This takes 2 boxa, removes all boxes smaller than a given area,
 *          and compares the remaining boxes between the boxa.
 *      (2) The area threshold is introduced to help remove noise from
 *          small components.  Any box with a smaller value of w * h
 *          will be removed from consideration.
 *      (3) The xor difference is the most stringent test, requiring alignment
 *          of the corresponding boxes.  It is also more computationally
 *          intensive and is optionally returned.  Alignment is to the
 *          UL corner of each region containing all boxes, as given by
 *          boxaGetExtent().
 *      (4) Both fractional differences are with respect to the total
 *          area in the two boxa.  They range from 0.0 to 1.0.
 *          A perfect match has value 0.0.  If both boxa are empty,
 *          we return 0.0; if one is empty we return 1.0.
 *      (5) An example input might be the rectangular regions of a
 *          segmentation mask for text or images from two pages.
 */
l_int32
boxaCompareRegions(BOXA       *boxa1,
                   BOXA       *boxa2,
                   l_int32     areathresh,
                   l_int32    *pnsame,
                   l_float32  *pdiffarea,
                   l_float32  *pdiffxor,
                   PIX       **ppixdb)
{
l_int32   w, h, x3, y3, w3, h3, x4, y4, w4, h4, n3, n4, area1, area2;
l_int32   count3, count4, countxor;
l_int32  *tab;
BOX      *box3, *box4;
BOXA     *boxa3, *boxa4, *boxa3t, *boxa4t;
PIX      *pix1, *pix2, *pix3, *pix4, *pix5;
PIXA     *pixa;

    PROCNAME("boxaCompareRegions");

    if (!pnsame)
        return ERROR_INT("&nsame not defined", procName, 1);
    *pnsame = FALSE;
    if (!pdiffarea)
        return ERROR_INT("&diffarea not defined", procName, 1);
    *pdiffarea = 1.0;
    if (!boxa1 || !boxa2)
        return ERROR_INT("boxa1 and boxa2 not both defined", procName, 1);
    if (pdiffxor) *pdiffxor = 1.0;
    if (ppixdb) *ppixdb = NULL;

    boxa3 = boxaSelectByArea(boxa1, areathresh, L_SELECT_IF_GTE, NULL);
    boxa4 = boxaSelectByArea(boxa2, areathresh, L_SELECT_IF_GTE, NULL);
    n3 = boxaGetCount(boxa3);
    n4 = boxaGetCount(boxa4);
    if (n3 == n4)
        *pnsame = TRUE;

        /* There are no boxes in one or both */
    if (n3 == 0 || n4 == 0) {
        boxaDestroy(&boxa3);
        boxaDestroy(&boxa4);
        if (n3 == 0 && n4 == 0) { /* they are both empty: we say they are the
                                   * same; otherwise, they differ maximally
                                   * and retain the default value. */
            *pdiffarea = 0.0;
            if (pdiffxor) *pdiffxor = 0.0;
        }
        return 0;
    }

        /* There are boxes in both */
    boxaGetArea(boxa3, &area1);
    boxaGetArea(boxa4, &area2);
    *pdiffarea = (l_float32)L_ABS(area1 - area2) / (l_float32)(area1 + area2);
    if (!pdiffxor) {
        boxaDestroy(&boxa3);
        boxaDestroy(&boxa4);
        return 0;
    }

        /* The easiest way to get the xor of aligned boxes is to work
         * with images of each boxa.  This is done by translating each
         * boxa so that the UL corner of the region that includes all
         * boxes in the boxa is placed at the origin of each pix. */
    boxaGetExtent(boxa3, &w, &h, &box3);
    boxaGetExtent(boxa4, &w, &h, &box4);
    boxGetGeometry(box3, &x3, &y3, &w3, &h3);
    boxGetGeometry(box4, &x4, &y4, &w4, &h4);
    boxa3t = boxaTransform(boxa3, -x3, -y3, 1.0, 1.0);
    boxa4t = boxaTransform(boxa4, -x4, -y4, 1.0, 1.0);
    w = L_MAX(x3 + w3, x4 + w4);
    h = L_MAX(y3 + h3, y4 + h4);
    pix3 = pixCreate(w, h, 1);  /* use the max to keep everything in the xor */
    pix4 = pixCreate(w, h, 1);
    pixMaskBoxa(pix3, pix3, boxa3t, L_SET_PIXELS);
    pixMaskBoxa(pix4, pix4, boxa4t, L_SET_PIXELS);
    tab = makePixelSumTab8();
    pixCountPixels(pix3, &count3, tab);
    pixCountPixels(pix4, &count4, tab);
    pix5 = pixXor(NULL, pix3, pix4);
    pixCountPixels(pix5, &countxor, tab);
    FREE(tab);
    *pdiffxor = (l_float32)countxor / (l_float32)(count3 + count4);

    if (ppixdb) {
        pixa = pixaCreate(2);
        pix1 = pixCreate(w, h, 32);
        pixSetAll(pix1);
        pixRenderHashBoxaBlend(pix1, boxa3, 5, 1, L_POS_SLOPE_LINE, 2,
                               255, 0, 0, 0.5);
        pixRenderHashBoxaBlend(pix1, boxa4, 5, 1, L_NEG_SLOPE_LINE, 2,
                               0, 255, 0, 0.5);
        pixaAddPix(pixa, pix1, L_INSERT);
        pix2 = pixCreate(w, h, 32);
        pixPaintThroughMask(pix2, pix3, x3, y3, 0xff000000);
        pixPaintThroughMask(pix2, pix4, x4, y4, 0x00ff0000);
        pixAnd(pix3, pix3, pix4);
        pixPaintThroughMask(pix2, pix3, x3, y3, 0x0000ff00);
        pixaAddPix(pixa, pix2, L_INSERT);
        *ppixdb = pixaDisplayTiledInRows(pixa, 32, 1000, 1.0, 0, 30, 2);
        pixaDestroy(&pixa);
    }

    boxDestroy(&box3);
    boxDestroy(&box4);
    boxaDestroy(&boxa3);
    boxaDestroy(&boxa3t);
    boxaDestroy(&boxa4);
    boxaDestroy(&boxa4t);
    pixDestroy(&pix3);
    pixDestroy(&pix4);
    pixDestroy(&pix5);
    return 0;
}

