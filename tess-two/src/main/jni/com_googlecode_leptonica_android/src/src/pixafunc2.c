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
 *   pixafunc2.c
 *
 *      Pixa Display (render into a pix)
 *           PIX      *pixaDisplay()
 *           PIX      *pixaDisplayOnColor()
 *           PIX      *pixaDisplayRandomCmap()
 *           PIX      *pixaDisplayLinearly()
 *           PIX      *pixaDisplayOnLattice()
 *           PIX      *pixaDisplayUnsplit()
 *           PIX      *pixaDisplayTiled()
 *           PIX      *pixaDisplayTiledInRows()
 *           PIX      *pixaDisplayTiledAndScaled()
 *
 *      Pixaa Display (render into a pix)
 *           PIX      *pixaaDisplay()
 *           PIX      *pixaaDisplayByPixa()
 *           PIXA     *pixaaDisplayTiledAndScaled()
 *
 *      Conversion of all pix to specified type (e.g., depth)
 *           PIXA     *pixaConvertTo1()
 *           PIXA     *pixaConvertTo8()
 *           PIXA     *pixaConvertTo8Color()
 *           PIXA     *pixaConvertTo32()
 *
 *      Tile N-Up
 *           l_int32   convertToNUpFiles()
 *           PIXA     *convertToNUpPixa()
 *
 *  We give seven methods for displaying a pixa in a pix.
 *  Some work for 1 bpp input; others for any input depth.
 *  Some give an output depth that depends on the input depth;
 *  others give a different output depth or allow you to choose it.
 *  Some use a boxes to determine where each pix goes; others tile
 *  onto a regular lattice; yet others tile onto an irregular lattice.
 *
 *  Here is a brief description of what the pixa display functions do.
 *
 *    pixaDisplay()
 *        This uses the boxes to lay out each pix.  It is typically
 *        used to reconstruct a pix that has been broken into components.
 *    pixaDisplayOnColor()
 *        pixaDisplay() with choice of background color
 *    pixaDisplayRandomCmap()
 *        This also uses the boxes to lay out each pix.  However, it creates
 *        a colormapped dest, where each 1 bpp pix is given a randomly
 *        generated color (up to 256 are used).
 *    pixaDisplayLinearly()
 *        This puts each pix, sequentially, in a line, either horizontally
 *        or vertically.
 *    pixaDisplayOnLattice()
 *        This puts each pix, sequentially, onto a regular lattice,
 *        omitting any pix that are too big for the lattice size.
 *        This is useful, for example, to store bitmapped fonts,
 *        where all the characters are stored in a single image.
 *    pixaDisplayUnsplit()
 *        This lays out a mosaic of tiles (the pix in the pixa) that
 *        are all of equal size.  (Don't use this for unequal sized pix!)
 *        For example, it can be used to invert the action of
 *        pixaSplitPix().
 *    pixaDisplayTiled()
 *        Like pixaDisplayOnLattice(), this places each pix on a regular
 *        lattice, but here the lattice size is determined by the
 *        largest component, and no components are omitted.  This is
 *        dangerous if there are thousands of small components and
 *        one or more very large one, because the size of the resulting
 *        pix can be huge!
 *    pixaDisplayTiledInRows()
 *        This puts each pix down in a series of rows, where the upper
 *        edges of each pix in a row are aligned and there is a uniform
 *        spacing between the pix.  The height of each row is determined
 *        by the tallest pix that was put in the row.  This function
 *        is a reasonably efficient way to pack the subimages.
 *        A boxa of the locations of each input pix is stored in the output.
 *    pixaDisplayTiledAndScaled()
 *        This scales each pix to a given width and output depth,
 *        and then tiles them in rows with a given number placed in
 *        each row.  This is very useful for presenting a sequence
 *        of images that can be at different resolutions, but which
 *        are derived from the same initial image.
 */

#include <string.h>
#include <math.h>   /* for sqrt() */
#include "allheaders.h"


/*---------------------------------------------------------------------*
 *                               Pixa Display                          *
 *---------------------------------------------------------------------*/
/*!
 *  pixaDisplay()
 *
 *      Input:  pixa
 *              w, h (if set to 0, determines the size from the
 *                    b.b. of the components in pixa)
 *      Return: pix, or null on error
 *
 *  Notes:
 *      (1) This uses the boxes to place each pix in the rendered composite.
 *      (2) Set w = h = 0 to use the b.b. of the components to determine
 *          the size of the returned pix.
 *      (3) Uses the first pix in pixa to determine the depth.
 *      (4) The background is written "white".  On 1 bpp, each successive
 *          pix is "painted" (adding foreground), whereas for grayscale
 *          or color each successive pix is blitted with just the src.
 *      (5) If the pixa is empty, returns an empty 1 bpp pix.
 */
PIX *
pixaDisplay(PIXA    *pixa,
            l_int32  w,
            l_int32  h)
{
l_int32  i, n, d, xb, yb, wb, hb;
BOXA    *boxa;
PIX     *pixt, *pixd;

    PROCNAME("pixaDisplay");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);

    n = pixaGetCount(pixa);
    if (n == 0 && w == 0 && h == 0)
        return (PIX *)ERROR_PTR("no components; no size", procName, NULL);
    if (n == 0) {
        L_WARNING("no components; returning empty 1 bpp pix\n", procName);
        return pixCreate(w, h, 1);
    }

        /* If w and h not input, determine the minimum size required
         * to contain the origin and all c.c. */
    if (w == 0 || h == 0) {
        boxa = pixaGetBoxa(pixa, L_CLONE);
        boxaGetExtent(boxa, &w, &h, NULL);
        boxaDestroy(&boxa);
    }

        /* Use the first pix in pixa to determine the depth.  */
    pixt = pixaGetPix(pixa, 0, L_CLONE);
    d = pixGetDepth(pixt);
    pixDestroy(&pixt);

    if ((pixd = pixCreate(w, h, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    if (d > 1)
        pixSetAll(pixd);
    for (i = 0; i < n; i++) {
        if (pixaGetBoxGeometry(pixa, i, &xb, &yb, &wb, &hb)) {
            L_WARNING("no box found!\n", procName);
            continue;
        }
        pixt = pixaGetPix(pixa, i, L_CLONE);
        if (d == 1)
            pixRasterop(pixd, xb, yb, wb, hb, PIX_PAINT, pixt, 0, 0);
        else
            pixRasterop(pixd, xb, yb, wb, hb, PIX_SRC, pixt, 0, 0);
        pixDestroy(&pixt);
    }

    return pixd;
}


/*!
 *  pixaDisplayOnColor()
 *
 *      Input:  pixa
 *              w, h (if set to 0, determines the size from the
 *                    b.b. of the components in pixa)
 *              color (background color to use)
 *      Return: pix, or null on error
 *
 *  Notes:
 *      (1) This uses the boxes to place each pix in the rendered composite.
 *      (2) Set w = h = 0 to use the b.b. of the components to determine
 *          the size of the returned pix.
 *      (3) If any pix in @pixa are colormapped, or if the pix have
 *          different depths, it returns a 32 bpp pix.  Otherwise,
 *          the depth of the returned pixa equals that of the pix in @pixa.
 *      (4) If the pixa is empty, return null.
 */
PIX *
pixaDisplayOnColor(PIXA     *pixa,
                   l_int32   w,
                   l_int32   h,
                   l_uint32  bgcolor)
{
l_int32  i, n, xb, yb, wb, hb, hascmap, maxdepth, same;
BOXA    *boxa;
PIX     *pixt1, *pixt2, *pixd;
PIXA    *pixat;

    PROCNAME("pixaDisplayOnColor");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);
    if ((n = pixaGetCount(pixa)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);

        /* If w and h are not input, determine the minimum size
         * required to contain the origin and all c.c. */
    if (w == 0 || h == 0) {
        boxa = pixaGetBoxa(pixa, L_CLONE);
        boxaGetExtent(boxa, &w, &h, NULL);
        boxaDestroy(&boxa);
    }

        /* If any pix have colormaps, or if they have different depths,
         * generate rgb */
    pixaAnyColormaps(pixa, &hascmap);
    pixaGetDepthInfo(pixa, &maxdepth, &same);
    if (hascmap || !same) {
        maxdepth = 32;
        pixat = pixaCreate(n);
        for (i = 0; i < n; i++) {
            pixt1 = pixaGetPix(pixa, i, L_CLONE);
            pixt2 = pixConvertTo32(pixt1);
            pixaAddPix(pixat, pixt2, L_INSERT);
            pixDestroy(&pixt1);
        }
    } else {
        pixat = pixaCopy(pixa, L_CLONE);
    }

        /* Make the output pix and set the background color */
    if ((pixd = pixCreate(w, h, maxdepth)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    if ((maxdepth == 1 && bgcolor > 0) ||
        (maxdepth == 2 && bgcolor >= 0x3) ||
        (maxdepth == 4 && bgcolor >= 0xf) ||
        (maxdepth == 8 && bgcolor >= 0xff) ||
        (maxdepth == 16 && bgcolor >= 0xffff) ||
        (maxdepth == 32 && bgcolor >= 0xffffff00)) {
        pixSetAll(pixd);
    } else if (bgcolor > 0) {
        pixSetAllArbitrary(pixd, bgcolor);
    }

        /* Blit each pix into its place */
    for (i = 0; i < n; i++) {
        if (pixaGetBoxGeometry(pixat, i, &xb, &yb, &wb, &hb)) {
            L_WARNING("no box found!\n", procName);
            continue;
        }
        pixt1 = pixaGetPix(pixat, i, L_CLONE);
        pixRasterop(pixd, xb, yb, wb, hb, PIX_SRC, pixt1, 0, 0);
        pixDestroy(&pixt1);
    }

    pixaDestroy(&pixat);
    return pixd;
}


/*!
 *  pixaDisplayRandomCmap()
 *
 *      Input:  pixa (of 1 bpp components, with boxa)
 *              w, h (if set to 0, determines the size from the
 *                    b.b. of the components in pixa)
 *      Return: pix (8 bpp, cmapped, with random colors on the components),
 *              or null on error
 *
 *  Notes:
 *      (1) This uses the boxes to place each pix in the rendered composite.
 *      (2) By default, the background color is: black, cmap index 0.
 *          This can be changed by pixcmapResetColor()
 */
PIX *
pixaDisplayRandomCmap(PIXA    *pixa,
                      l_int32  w,
                      l_int32  h)
{
l_int32   i, n, maxdepth, index, xb, yb, wb, hb;
BOXA     *boxa;
PIX      *pixs, *pixt, *pixd;
PIXCMAP  *cmap;

    PROCNAME("pixaDisplayRandomCmap");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);

    if ((n = pixaGetCount(pixa)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);
    pixaVerifyDepth(pixa, &maxdepth);
    if (maxdepth != 1)
        return (PIX *)ERROR_PTR("not all components are 1 bpp", procName, NULL);

        /* If w and h are not input, determine the minimum size required
         * to contain the origin and all c.c. */
    if (w == 0 || h == 0) {
        boxa = pixaGetBoxa(pixa, L_CLONE);
        boxaGetExtent(boxa, &w, &h, NULL);
        boxaDestroy(&boxa);
    }

        /* Set up an 8 bpp dest pix, with a colormap with 254 random colors */
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    cmap = pixcmapCreateRandom(8, 1, 1);
    pixSetColormap(pixd, cmap);

        /* Color each component and blit it in */
    for (i = 0; i < n; i++) {
        index = 1 + (i % 254);
        pixaGetBoxGeometry(pixa, i, &xb, &yb, &wb, &hb);
        pixs = pixaGetPix(pixa, i, L_CLONE);
        pixt = pixConvert1To8(NULL, pixs, 0, index);
        pixRasterop(pixd, xb, yb, wb, hb, PIX_PAINT, pixt, 0, 0);
        pixDestroy(&pixs);
        pixDestroy(&pixt);
    }

    return pixd;
}


/*!
 *  pixaDisplayLinearly()
 *
 *      Input:  pixa
 *              direction (L_HORIZ or L_VERT)
 *              scalefactor (applied to every pix; use 1.0 for no scaling)
 *              background (0 for white, 1 for black; this is the color
 *                 of the spacing between the images)
 *              spacing  (between images, and on outside)
 *              border (width of black border added to each image;
 *                      use 0 for no border)
 *              &boxa (<optional return> location of images in output pix
 *      Return: pix of composite images, or null on error
 *
 *  Notes:
 *      (1) This puts each pix, sequentially, in a line, either horizontally
 *          or vertically.
 *      (2) If any pix has a colormap, all pix are rendered in rgb.
 *      (3) The boxa gives the location of each image.
 */
PIX *
pixaDisplayLinearly(PIXA      *pixas,
                    l_int32    direction,
                    l_float32  scalefactor,
                    l_int32    background,  /* not used */
                    l_int32    spacing,
                    l_int32    border,
                    BOXA     **pboxa)
{
l_int32  i, n, x, y, w, h, size, depth, bordval;
BOX     *box;
PIX     *pix1, *pix2, *pix3, *pixd;
PIXA    *pixa1, *pixa2;

    PROCNAME("pixaDisplayLinearly");

    if (pboxa) *pboxa = NULL;
    if (!pixas)
        return (PIX *)ERROR_PTR("pixas not defined", procName, NULL);
    if (direction != L_HORIZ && direction != L_VERT)
        return (PIX *)ERROR_PTR("invalid direction", procName, NULL);

        /* Make sure all pix are at the same depth */
    pixa1 = pixaConvertToSameDepth(pixas);
    pixaGetDepthInfo(pixa1, &depth, NULL);

        /* Scale and add border if requested */
    n = pixaGetCount(pixa1);
    pixa2 = pixaCreate(n);
    bordval = (depth == 1) ? 1 : 0;
    size = (n - 1) * spacing;
    x = y = 0;
    for (i = 0; i < n; i++) {
        if ((pix1 = pixaGetPix(pixa1, i, L_CLONE)) == NULL) {
            L_WARNING("missing pix at index %d\n", procName, i);
            continue;
        }

        if (scalefactor != 1.0)
            pix2 = pixScale(pix1, scalefactor, scalefactor);
        else
            pix2 = pixClone(pix1);
        if (border)
            pix3 = pixAddBorder(pix2, border, bordval);
        else
            pix3 = pixClone(pix2);

        pixGetDimensions(pix3, &w, &h, NULL);
        box = boxCreate(x, y, w, h);
        if (direction == L_HORIZ) {
            size += w;
            x += w + spacing;
        } else {  /* vertical */
            size += h;
            y += h + spacing;
        }
        pixaAddPix(pixa2, pix3, L_INSERT);
        pixaAddBox(pixa2, box, L_INSERT);
        pixDestroy(&pix1);
        pixDestroy(&pix2);
    }
    pixd = pixaDisplay(pixa2, 0, 0);

    if (pboxa)
        *pboxa = pixaGetBoxa(pixa2, L_COPY);
    pixaDestroy(&pixa1);
    pixaDestroy(&pixa2);
    return pixd;
}


/*!
 *  pixaDisplayOnLattice()
 *
 *      Input:  pixa
 *              cellw (lattice cell width)
 *              cellh (lattice cell height)
 *              &ncols (<optional return> number of columns in output lattice)
 *              &boxa (<optional return> location of images in lattice)
 *      Return: pix of composite images, or null on error
 *
 *  Notes:
 *      (1) This places each pix on sequentially on a regular lattice
 *          in the rendered composite.  If a pix is too large to fit in the
 *          allocated lattice space, it is not rendered.
 *      (2) If any pix has a colormap, all pix are rendered in rgb.
 *      (3) This is useful when putting bitmaps of components,
 *          such as characters, into a single image.
 *      (4) The boxa gives the location of each image.  The UL corner
 *          of each image is on a lattice cell corner.  Omitted images
 *          (due to size) are assigned an invalid width and height of 0.
 */
PIX *
pixaDisplayOnLattice(PIXA     *pixa,
                     l_int32   cellw,
                     l_int32   cellh,
                     l_int32  *pncols,
                     BOXA    **pboxa)
{
l_int32  n, nw, nh, w, h, d, wt, ht;
l_int32  index, i, j, hascmap;
BOX     *box;
BOXA    *boxa;
PIX     *pix, *pixt, *pixd;
PIXA    *pixat;

    PROCNAME("pixaDisplayOnLattice");

    if (pncols) *pncols = 0;
    if (pboxa) *pboxa = NULL;
    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);

        /* If any pix have colormaps, generate rgb */
    if ((n = pixaGetCount(pixa)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);
    pixaAnyColormaps(pixa, &hascmap);
    if (hascmap) {
        pixat = pixaCreate(n);
        for (i = 0; i < n; i++) {
            pixt = pixaGetPix(pixa, i, L_CLONE);
            pix = pixConvertTo32(pixt);
            pixaAddPix(pixat, pix, L_INSERT);
            pixDestroy(&pixt);
        }
    } else {
        pixat = pixaCopy(pixa, L_CLONE);
    }
    boxa = boxaCreate(n);

        /* Have number of rows and columns approximately equal */
    nw = (l_int32)sqrt((l_float64)n);
    nh = (n + nw - 1) / nw;
    w = cellw * nw;
    h = cellh * nh;

        /* Use the first pix in pixa to determine the output depth.  */
    pixaGetPixDimensions(pixat, 0, NULL, NULL, &d);
    if ((pixd = pixCreate(w, h, d)) == NULL) {
        pixaDestroy(&pixat);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    pixSetBlackOrWhite(pixd, L_SET_WHITE);

        /* Tile the output */
    index = 0;
    for (i = 0; i < nh; i++) {
        for (j = 0; j < nw && index < n; j++, index++) {
            pixt = pixaGetPix(pixat, index, L_CLONE);
            pixGetDimensions(pixt, &wt, &ht, NULL);
            if (wt > cellw || ht > cellh) {
                L_INFO("pix(%d) omitted; size %dx%x\n", procName, index,
                       wt, ht);
                box = boxCreate(0, 0, 0, 0);
                boxaAddBox(boxa, box, L_INSERT);
                pixDestroy(&pixt);
                continue;
            }
            pixRasterop(pixd, j * cellw, i * cellh, wt, ht,
                        PIX_SRC, pixt, 0, 0);
            box = boxCreate(j * cellw, i * cellh, wt, ht);
            boxaAddBox(boxa, box, L_INSERT);
            pixDestroy(&pixt);
        }
    }

    if (pncols) *pncols = nw;
    if (pboxa)
        *pboxa = boxa;
    else
        boxaDestroy(&boxa);
    pixaDestroy(&pixat);
    return pixd;
}


/*!
 *  pixaDisplayUnsplit()
 *
 *      Input:  pixa
 *              nx   (number of mosaic cells horizontally)
 *              ny   (number of mosaic cells vertically)
 *              borderwidth  (of added border on all sides)
 *              bordercolor  (in our RGBA format: 0xrrggbbaa)
 *      Return: pix of tiled images, or null on error
 *
 *  Notes:
 *      (1) This is a logical inverse of pixaSplitPix().  It
 *          constructs a pix from a mosaic of tiles, all of equal size.
 *      (2) For added generality, a border of arbitrary color can
 *          be added to each of the tiles.
 *      (3) In use, pixa will typically have either been generated
 *          from pixaSplitPix() or will derived from a pixa that
 *          was so generated.
 *      (4) All pix in the pixa must be of equal depth, and, if
 *          colormapped, have the same colormap.
 */
PIX *
pixaDisplayUnsplit(PIXA     *pixa,
                   l_int32   nx,
                   l_int32   ny,
                   l_int32   borderwidth,
                   l_uint32  bordercolor)
{
l_int32  w, h, d, wt, ht;
l_int32  i, j, k, x, y, n;
PIX     *pixt, *pixd;

    PROCNAME("pixaDisplayUnsplit");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);
    if (nx <= 0 || ny <= 0)
        return (PIX *)ERROR_PTR("nx and ny must be > 0", procName, NULL);
    if ((n = pixaGetCount(pixa)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);
    if (n != nx * ny)
        return (PIX *)ERROR_PTR("n != nx * ny", procName, NULL);
    borderwidth = L_MAX(0, borderwidth);

    pixaGetPixDimensions(pixa, 0, &wt, &ht, &d);
    w = nx * (wt + 2 * borderwidth);
    h = ny * (ht + 2 * borderwidth);

    if ((pixd = pixCreate(w, h, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixt = pixaGetPix(pixa, 0, L_CLONE);
    pixCopyColormap(pixd, pixt);
    pixDestroy(&pixt);
    if (borderwidth > 0)
        pixSetAllArbitrary(pixd, bordercolor);

    y = borderwidth;
    for (i = 0, k = 0; i < ny; i++) {
        x = borderwidth;
        for (j = 0; j < nx; j++, k++) {
            pixt = pixaGetPix(pixa, k, L_CLONE);
            pixRasterop(pixd, x, y, wt, ht, PIX_SRC, pixt, 0, 0);
            pixDestroy(&pixt);
            x += wt + 2 * borderwidth;
        }
        y += ht + 2 * borderwidth;
    }

    return pixd;
}


/*!
 *  pixaDisplayTiled()
 *
 *      Input:  pixa
 *              maxwidth (of output image)
 *              background (0 for white, 1 for black)
 *              spacing
 *      Return: pix of tiled images, or null on error
 *
 *  Notes:
 *      (1) This renders a pixa to a single image file of width not to
 *          exceed maxwidth, with background color either white or black,
 *          and with each subimage spaced on a regular lattice.
 *      (2) The lattice size is determined from the largest width and height,
 *          separately, of all pix in the pixa.
 *      (3) All pix in the pixa must be of equal depth.
 *      (4) If any pix has a colormap, all pix are rendered in rgb.
 *      (5) Careful: because no components are omitted, this is
 *          dangerous if there are thousands of small components and
 *          one or more very large one, because the size of the
 *          resulting pix can be huge!
 */
PIX *
pixaDisplayTiled(PIXA    *pixa,
                 l_int32  maxwidth,
                 l_int32  background,
                 l_int32  spacing)
{
l_int32  w, h, wmax, hmax, wd, hd, d, hascmap;
l_int32  i, j, n, ni, ncols, nrows;
l_int32  ystart, xstart, wt, ht;
PIX     *pix, *pixt, *pixd;
PIXA    *pixat;

    PROCNAME("pixaDisplayTiled");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);

        /* If any pix have colormaps, generate rgb */
    if ((n = pixaGetCount(pixa)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);
    pixaAnyColormaps(pixa, &hascmap);
    if (hascmap) {
        pixat = pixaCreate(n);
        for (i = 0; i < n; i++) {
            pixt = pixaGetPix(pixa, i, L_CLONE);
            pix = pixConvertTo32(pixt);
            pixaAddPix(pixat, pix, L_INSERT);
            pixDestroy(&pixt);
        }
    } else {
        pixat = pixaCopy(pixa, L_CLONE);
    }

        /* Find the largest width and height of the subimages */
    wmax = hmax = 0;
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixat, i, L_CLONE);
        pixGetDimensions(pix, &w, &h, NULL);
        if (i == 0) {
            d = pixGetDepth(pix);
        } else if (d != pixGetDepth(pix)) {
            pixDestroy(&pix);
            pixaDestroy(&pixat);
            return (PIX *)ERROR_PTR("depths not equal", procName, NULL);
        }
        if (w > wmax)
            wmax = w;
        if (h > hmax)
            hmax = h;
        pixDestroy(&pix);
    }

        /* Get the number of rows and columns and the output image size */
    spacing = L_MAX(spacing, 0);
    ncols = (l_int32)((l_float32)(maxwidth - spacing) /
                      (l_float32)(wmax + spacing));
    nrows = (n + ncols - 1) / ncols;
    wd = wmax * ncols + spacing * (ncols + 1);
    hd = hmax * nrows + spacing * (nrows + 1);
    if ((pixd = pixCreate(wd, hd, d)) == NULL) {
        pixaDestroy(&pixat);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }

        /* Reset the background color if necessary */
    if ((background == 1 && d == 1) || (background == 0 && d != 1))
        pixSetAll(pixd);

        /* Blit the images to the dest */
    for (i = 0, ni = 0; i < nrows; i++) {
        ystart = spacing + i * (hmax + spacing);
        for (j = 0; j < ncols && ni < n; j++, ni++) {
            xstart = spacing + j * (wmax + spacing);
            pix = pixaGetPix(pixat, ni, L_CLONE);
            wt = pixGetWidth(pix);
            ht = pixGetHeight(pix);
            pixRasterop(pixd, xstart, ystart, wt, ht, PIX_SRC, pix, 0, 0);
            pixDestroy(&pix);
        }
    }

    pixaDestroy(&pixat);
    return pixd;
}


/*!
 *  pixaDisplayTiledInRows()
 *
 *      Input:  pixa
 *              outdepth (output depth: 1, 8 or 32 bpp)
 *              maxwidth (of output image)
 *              scalefactor (applied to every pix; use 1.0 for no scaling)
 *              background (0 for white, 1 for black; this is the color
 *                 of the spacing between the images)
 *              spacing  (between images, and on outside)
 *              border (width of black border added to each image;
 *                      use 0 for no border)
 *      Return: pixd (of tiled images), or null on error
 *
 *  Notes:
 *      (1) This renders a pixa to a single image file of width not to
 *          exceed maxwidth, with background color either white or black,
 *          and with each row tiled such that the top of each pix is
 *          aligned and separated by 'spacing' from the next one.
 *          A black border can be added to each pix.
 *      (2) All pix are converted to outdepth; existing colormaps are removed.
 *      (3) This does a reasonably spacewise-efficient job of laying
 *          out the individual pix images into a tiled composite.
 *      (4) A serialized boxa giving the location in pixd of each input
 *          pix (without added border) is stored in the text string of pixd.
 *          This allows, e.g., regeneration of a pixa from pixd, using
 *          pixaCreateFromBoxa().  If there is no scaling and the depth of
 *          each input pix in the pixa is the same, this tiling operation
 *          can be inverted using the boxa (except for loss of text in
 *          each of the input pix):
 *            pix1 = pixaDisplayTiledInRows(pixa1, 1, 1500, 1.0, 0, 30, 0);
 *            char *boxatxt = pixGetText(pix1);
 *            boxa1 = boxaReadMem((l_uint8 *)boxatxt, strlen(boxatxt));
 *            pixa2 = pixaCreateFromBoxa(pix1, boxa1, NULL);
 */
PIX *
pixaDisplayTiledInRows(PIXA      *pixa,
                       l_int32    outdepth,
                       l_int32    maxwidth,
                       l_float32  scalefactor,
                       l_int32    background,
                       l_int32    spacing,
                       l_int32    border)
{
l_int32   h;  /* cumulative height over all the rows */
l_int32   w;  /* cumulative height in the current row */
l_int32   bordval, wtry, wt, ht;
l_int32   irow;  /* index of current pix in current row */
l_int32   wmaxrow;  /* width of the largest row */
l_int32   maxh;  /* max height in row */
l_int32   i, j, index, n, x, y, nrows, ninrow;
size_t    size;
l_uint8  *data;
BOXA     *boxa;
NUMA     *nainrow;  /* number of pix in the row */
NUMA     *namaxh;  /* height of max pix in the row */
PIX      *pix, *pixn, *pixt, *pixd;
PIXA     *pixan;

    PROCNAME("pixaDisplayTiledInRows");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);
    if (outdepth != 1 && outdepth != 8 && outdepth != 32)
        return (PIX *)ERROR_PTR("outdepth not in {1, 8, 32}", procName, NULL);
    if (border < 0)
        border = 0;
    if (scalefactor <= 0.0) scalefactor = 1.0;

    if ((n = pixaGetCount(pixa)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);

        /* Normalize depths, scale, remove colormaps; optionally add border */
    pixan = pixaCreate(n);
    bordval = (outdepth == 1) ? 1 : 0;
    for (i = 0; i < n; i++) {
        if ((pix = pixaGetPix(pixa, i, L_CLONE)) == NULL)
            continue;

        if (outdepth == 1)
            pixn = pixConvertTo1(pix, 128);
        else if (outdepth == 8)
            pixn = pixConvertTo8(pix, FALSE);
        else  /* outdepth == 32 */
            pixn = pixConvertTo32(pix);
        pixDestroy(&pix);

        if (scalefactor != 1.0)
            pixt = pixScale(pixn, scalefactor, scalefactor);
        else
            pixt = pixClone(pixn);
        if (border)
            pixd = pixAddBorder(pixt, border, bordval);
        else
            pixd = pixClone(pixt);
        pixDestroy(&pixn);
        pixDestroy(&pixt);

        pixaAddPix(pixan, pixd, L_INSERT);
    }
    if (pixaGetCount(pixan) != n) {
        n = pixaGetCount(pixan);
        L_WARNING("only got %d components\n", procName, n);
        if (n == 0) {
            pixaDestroy(&pixan);
            return (PIX *)ERROR_PTR("no components", procName, NULL);
        }
    }

        /* Compute parameters for layout */
    nainrow = numaCreate(0);
    namaxh = numaCreate(0);
    wmaxrow = 0;
    w = h = spacing;
    maxh = 0;  /* max height in row */
    for (i = 0, irow = 0; i < n; i++, irow++) {
        pixaGetPixDimensions(pixan, i, &wt, &ht, NULL);
        wtry = w + wt + spacing;
        if (wtry > maxwidth) {  /* end the current row and start next one */
            numaAddNumber(nainrow, irow);
            numaAddNumber(namaxh, maxh);
            wmaxrow = L_MAX(wmaxrow, w);
            h += maxh + spacing;
            irow = 0;
            w = wt + 2 * spacing;
            maxh = ht;
        } else {
            w = wtry;
            maxh = L_MAX(maxh, ht);
        }
    }

        /* Enter the parameters for the last row */
    numaAddNumber(nainrow, irow);
    numaAddNumber(namaxh, maxh);
    wmaxrow = L_MAX(wmaxrow, w);
    h += maxh + spacing;

    if ((pixd = pixCreate(wmaxrow, h, outdepth)) == NULL) {
        numaDestroy(&nainrow);
        numaDestroy(&namaxh);
        pixaDestroy(&pixan);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }

        /* Reset the background color if necessary */
    if ((background == 1 && outdepth == 1) ||
        (background == 0 && outdepth != 1))
        pixSetAll(pixd);

        /* Blit the images to the dest, and save the boxa identifying
         * the image regions that do not include the borders. */
    nrows = numaGetCount(nainrow);
    y = spacing;
    boxa = boxaCreate(n);
    for (i = 0, index = 0; i < nrows; i++) {  /* over rows */
        numaGetIValue(nainrow, i, &ninrow);
        numaGetIValue(namaxh, i, &maxh);
        x = spacing;
        for (j = 0; j < ninrow; j++, index++) {   /* over pix in row */
            pix = pixaGetPix(pixan, index, L_CLONE);
            pixGetDimensions(pix, &wt, &ht, NULL);
            boxaAddBox(boxa, boxCreate(x + border, y + border,
                wt - 2 * border, ht - 2 *border), L_INSERT);
            pixRasterop(pixd, x, y, wt, ht, PIX_SRC, pix, 0, 0);
            pixDestroy(&pix);
            x += wt + spacing;
        }
        y += maxh + spacing;
    }
    boxaWriteMem(&data, &size, boxa);
    pixSetText(pixd, (char *)data);  /* data is ascii */
    FREE(data);
    boxaDestroy(&boxa);

    numaDestroy(&nainrow);
    numaDestroy(&namaxh);
    pixaDestroy(&pixan);
    return pixd;
}


/*!
 *  pixaDisplayTiledAndScaled()
 *
 *      Input:  pixa
 *              outdepth (output depth: 1, 8 or 32 bpp)
 *              tilewidth (each pix is scaled to this width)
 *              ncols (number of tiles in each row)
 *              background (0 for white, 1 for black; this is the color
 *                 of the spacing between the images)
 *              spacing  (between images, and on outside)
 *              border (width of additional black border on each image;
 *                      use 0 for no border)
 *      Return: pix of tiled images, or null on error
 *
 *  Notes:
 *      (1) This can be used to tile a number of renderings of
 *          an image that are at different scales and depths.
 *      (2) Each image, after scaling and optionally adding the
 *          black border, has width 'tilewidth'.  Thus, the border does
 *          not affect the spacing between the image tiles.  The
 *          maximum allowed border width is tilewidth / 5.
 */
PIX *
pixaDisplayTiledAndScaled(PIXA    *pixa,
                          l_int32  outdepth,
                          l_int32  tilewidth,
                          l_int32  ncols,
                          l_int32  background,
                          l_int32  spacing,
                          l_int32  border)
{
l_int32    x, y, w, h, wd, hd, d;
l_int32    i, n, nrows, maxht, ninrow, irow, bordval;
l_int32   *rowht;
l_float32  scalefact;
PIX       *pix, *pixn, *pixt, *pixb, *pixd;
PIXA      *pixan;

    PROCNAME("pixaDisplayTiledAndScaled");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);
    if (outdepth != 1 && outdepth != 8 && outdepth != 32)
        return (PIX *)ERROR_PTR("outdepth not in {1, 8, 32}", procName, NULL);
    if (border < 0 || border > tilewidth / 5)
        border = 0;

    if ((n = pixaGetCount(pixa)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);

        /* Normalize scale and depth for each pix; optionally add border */
    pixan = pixaCreate(n);
    bordval = (outdepth == 1) ? 1 : 0;
    for (i = 0; i < n; i++) {
        if ((pix = pixaGetPix(pixa, i, L_CLONE)) == NULL)
            continue;

        pixGetDimensions(pix, &w, &h, &d);
        scalefact = (l_float32)(tilewidth - 2 * border) / (l_float32)w;
        if (d == 1 && outdepth > 1 && scalefact < 1.0)
            pixt = pixScaleToGray(pix, scalefact);
        else
            pixt = pixScale(pix, scalefact, scalefact);

        if (outdepth == 1)
            pixn = pixConvertTo1(pixt, 128);
        else if (outdepth == 8)
            pixn = pixConvertTo8(pixt, FALSE);
        else  /* outdepth == 32 */
            pixn = pixConvertTo32(pixt);
        pixDestroy(&pixt);

        if (border)
            pixb = pixAddBorder(pixn, border, bordval);
        else
            pixb = pixClone(pixn);

        pixaAddPix(pixan, pixb, L_INSERT);
        pixDestroy(&pix);
        pixDestroy(&pixn);
    }
    if ((n = pixaGetCount(pixan)) == 0) { /* should not have changed! */
        pixaDestroy(&pixan);
        return (PIX *)ERROR_PTR("no components", procName, NULL);
    }

        /* Determine the size of each row and of pixd */
    wd = tilewidth * ncols + spacing * (ncols + 1);
    nrows = (n + ncols - 1) / ncols;
    if ((rowht = (l_int32 *)CALLOC(nrows, sizeof(l_int32))) == NULL)
        return (PIX *)ERROR_PTR("rowht array not made", procName, NULL);
    maxht = 0;
    ninrow = 0;
    irow = 0;
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixan, i, L_CLONE);
        ninrow++;
        pixGetDimensions(pix, &w, &h, NULL);
        maxht = L_MAX(h, maxht);
        if (ninrow == ncols) {
            rowht[irow] = maxht;
            maxht = ninrow = 0;  /* reset */
            irow++;
        }
        pixDestroy(&pix);
    }
    if (ninrow > 0) {   /* last fencepost */
        rowht[irow] = maxht;
        irow++;  /* total number of rows */
    }
    nrows = irow;
    hd = spacing * (nrows + 1);
    for (i = 0; i < nrows; i++)
        hd += rowht[i];

    pixd = pixCreate(wd, hd, outdepth);
    if ((background == 1 && outdepth == 1) ||
        (background == 0 && outdepth != 1))
        pixSetAll(pixd);

        /* Now blit images to pixd */
    x = y = spacing;
    irow = 0;
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixan, i, L_CLONE);
        pixGetDimensions(pix, &w, &h, NULL);
        if (i && ((i % ncols) == 0)) {  /* start new row */
            x = spacing;
            y += spacing + rowht[irow];
            irow++;
        }
        pixRasterop(pixd, x, y, w, h, PIX_SRC, pix, 0, 0);
        x += tilewidth + spacing;
        pixDestroy(&pix);
    }

    pixaDestroy(&pixan);
    FREE(rowht);
    return pixd;
}


/*---------------------------------------------------------------------*
 *                              Pixaa Display                          *
 *---------------------------------------------------------------------*/
/*!
 *  pixaaDisplay()
 *
 *      Input:  paa
 *              w, h (if set to 0, determines the size from the
 *                    b.b. of the components in paa)
 *      Return: pix, or null on error
 *
 *  Notes:
 *      (1) Each pix of the paa is displayed at the location given by
 *          its box, translated by the box of the containing pixa
 *          if it exists.
 */
PIX *
pixaaDisplay(PIXAA   *paa,
             l_int32  w,
             l_int32  h)
{
l_int32  i, j, n, nbox, na, d, wmax, hmax, x, y, xb, yb, wb, hb;
BOXA    *boxa1;  /* top-level boxa */
BOXA    *boxa;
PIX     *pixt, *pixd;
PIXA    *pixa;

    PROCNAME("pixaaDisplay");

    if (!paa)
        return (PIX *)ERROR_PTR("paa not defined", procName, NULL);

    n = pixaaGetCount(paa, NULL);
    if (n == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);

        /* If w and h not input, determine the minimum size required
         * to contain the origin and all c.c. */
    boxa1 = pixaaGetBoxa(paa, L_CLONE);
    nbox = boxaGetCount(boxa1);
    if (w == 0 || h == 0) {
        if (nbox == n) {
            boxaGetExtent(boxa1, &w, &h, NULL);
        } else {  /* have to use the lower-level boxa for each pixa */
            wmax = hmax = 0;
            for (i = 0; i < n; i++) {
                pixa = pixaaGetPixa(paa, i, L_CLONE);
                boxa = pixaGetBoxa(pixa, L_CLONE);
                boxaGetExtent(boxa, &w, &h, NULL);
                wmax = L_MAX(wmax, w);
                hmax = L_MAX(hmax, h);
                pixaDestroy(&pixa);
                boxaDestroy(&boxa);
            }
            w = wmax;
            h = hmax;
        }
    }

        /* Get depth from first pix */
    pixa = pixaaGetPixa(paa, 0, L_CLONE);
    pixt = pixaGetPix(pixa, 0, L_CLONE);
    d = pixGetDepth(pixt);
    pixaDestroy(&pixa);
    pixDestroy(&pixt);

    if ((pixd = pixCreate(w, h, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

    x = y = 0;
    for (i = 0; i < n; i++) {
        pixa = pixaaGetPixa(paa, i, L_CLONE);
        if (nbox == n)
            boxaGetBoxGeometry(boxa1, i, &x, &y, NULL, NULL);
        na = pixaGetCount(pixa);
        for (j = 0; j < na; j++) {
            pixaGetBoxGeometry(pixa, j, &xb, &yb, &wb, &hb);
            pixt = pixaGetPix(pixa, j, L_CLONE);
            pixRasterop(pixd, x + xb, y + yb, wb, hb, PIX_PAINT, pixt, 0, 0);
            pixDestroy(&pixt);
        }
        pixaDestroy(&pixa);
    }
    boxaDestroy(&boxa1);

    return pixd;
}


/*!
 *  pixaaDisplayByPixa()
 *
 *      Input:  paa (with pix that may have different depths)
 *              xspace between pix in pixa
 *              yspace between pixa
 *              max width of output pix
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Displays each pixa on a line (or set of lines),
 *          in order from top to bottom.  Within each pixa,
 *          the pix are displayed in order from left to right.
 *      (2) The sizes and depths of each pix can differ.  The output pix
 *          has a depth equal to the max depth of all the pix.
 *      (3) This ignores the boxa of the paa.
 */
PIX *
pixaaDisplayByPixa(PIXAA   *paa,
                   l_int32  xspace,
                   l_int32  yspace,
                   l_int32  maxw)
{
l_int32   i, j, npixa, npix, same, use_maxw, x, y, w, h, hindex;
l_int32   maxwidth, maxdepth, width, lmaxh, lmaxw;
l_int32  *harray;
NUMA     *nah;
PIX      *pix, *pixt, *pixd;
PIXA     *pixa;

    PROCNAME("pixaaDisplayByPixa");

    if (!paa)
        return (PIX *)ERROR_PTR("paa not defined", procName, NULL);

    if ((npixa = pixaaGetCount(paa, NULL)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);
    same = pixaaVerifyDepth(paa, &maxdepth);
    if (!same && maxdepth < 8)
        return (PIX *)ERROR_PTR("depths differ; max < 8", procName, NULL);

        /* Be sure the widest box fits in the output pix */
    pixaaSizeRange(paa, NULL, NULL, &maxwidth, NULL);
    if (maxwidth > maxw) {
        L_WARNING("maxwidth > maxw; using maxwidth\n", procName);
        maxw = maxwidth;
    }


        /* Get size of output pix.  The width is the minimum of the
         * maxw and the largest pixa line width.  The height is whatever
         * it needs to be to accommodate all pixa. */
    lmaxw = 0;  /* widest line found */
    use_maxw = FALSE;
    nah = numaCreate(0);  /* store height of each line */
    y = yspace;
    for (i = 0; i < npixa; i++) {
        pixa = pixaaGetPixa(paa, i, L_CLONE);
        npix = pixaGetCount(pixa);
        if (npix == 0) {
            pixaDestroy(&pixa);
            continue;
        }
        x = xspace;
        lmaxh = 0;  /* max height found in the line */
        for (j = 0; j < npix; j++) {
            pix = pixaGetPix(pixa, j, L_CLONE);
            pixGetDimensions(pix, &w, &h, NULL);
            if (x + w >= maxw) {  /* start new line */
                x = xspace;
                y += lmaxh + yspace;
                numaAddNumber(nah, lmaxh);
                lmaxh = 0;
                use_maxw = TRUE;
            }
            x += w + xspace;
            lmaxh = L_MAX(h, lmaxh);
            lmaxw = L_MAX(lmaxw, x);
            pixDestroy(&pix);
        }
        y += lmaxh + yspace;
        numaAddNumber(nah, lmaxh);
        pixaDestroy(&pixa);
    }
    width = (use_maxw) ? maxw : lmaxw;

    if ((pixd = pixCreate(width, y, maxdepth)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

        /* Now layout the pix by pixa */
    y = yspace;
    harray = numaGetIArray(nah);
    hindex = 0;
    for (i = 0; i < npixa; i++) {
        x = xspace;
        pixa = pixaaGetPixa(paa, i, L_CLONE);
        npix = pixaGetCount(pixa);
        if (npix == 0) {
            pixaDestroy(&pixa);
            continue;
        }
        for (j = 0; j < npix; j++) {
            pix = pixaGetPix(pixa, j, L_CLONE);
            if (pixGetDepth(pix) != maxdepth) {
                if (maxdepth == 8)
                     pixt = pixConvertTo8(pix, 0);
                else  /* 32 bpp */
                     pixt = pixConvertTo32(pix);
            } else {
                pixt = pixClone(pix);
            }
            pixGetDimensions(pixt, &w, &h, NULL);
            if (x + w >= maxw) {  /* start new line */
                x = xspace;
                y += harray[hindex++] + yspace;
            }
            pixRasterop(pixd, x, y, w, h, PIX_PAINT, pixt, 0, 0);
            pixDestroy(&pix);
            pixDestroy(&pixt);
            x += w + xspace;
        }
        y += harray[hindex++] + yspace;
        pixaDestroy(&pixa);
    }
    FREE(harray);

    numaDestroy(&nah);
    return pixd;
}


/*!
 *  pixaaDisplayTiledAndScaled()
 *
 *      Input:  paa
 *              outdepth (output depth: 1, 8 or 32 bpp)
 *              tilewidth (each pix is scaled to this width)
 *              ncols (number of tiles in each row)
 *              background (0 for white, 1 for black; this is the color
 *                 of the spacing between the images)
 *              spacing  (between images, and on outside)
 *              border (width of additional black border on each image;
 *                      use 0 for no border)
 *      Return: pixa (of tiled images, one image for each pixa in
 *                    the paa), or null on error
 *
 *  Notes:
 *      (1) For each pixa, this generates from all the pix a
 *          tiled/scaled output pix, and puts it in the output pixa.
 *      (2) See comments in pixaDisplayTiledAndScaled().
 */
PIXA *
pixaaDisplayTiledAndScaled(PIXAA   *paa,
                           l_int32  outdepth,
                           l_int32  tilewidth,
                           l_int32  ncols,
                           l_int32  background,
                           l_int32  spacing,
                           l_int32  border)
{
l_int32  i, n;
PIX     *pix;
PIXA    *pixa, *pixad;

    PROCNAME("pixaaDisplayTiledAndScaled");

    if (!paa)
        return (PIXA *)ERROR_PTR("paa not defined", procName, NULL);
    if (outdepth != 1 && outdepth != 8 && outdepth != 32)
        return (PIXA *)ERROR_PTR("outdepth not in {1, 8, 32}", procName, NULL);
    if (border < 0 || border > tilewidth / 5)
        border = 0;

    if ((n = pixaaGetCount(paa, NULL)) == 0)
        return (PIXA *)ERROR_PTR("no components", procName, NULL);

    pixad = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pixa = pixaaGetPixa(paa, i, L_CLONE);
        pix = pixaDisplayTiledAndScaled(pixa, outdepth, tilewidth, ncols,
                                        background, spacing, border);
        pixaAddPix(pixad, pix, L_INSERT);
        pixaDestroy(&pixa);
    }

    return pixad;
}


/*---------------------------------------------------------------------*
 *         Conversion of all pix to specified type (e.g., depth)       *
 *---------------------------------------------------------------------*/
/*!
 *  pixaConvertTo1()
 *
 *      Input:  pixas
 *              thresh (threshold for final binarization from 8 bpp gray)
 *      Return: pixad, or null on error
 */
PIXA *
pixaConvertTo1(PIXA    *pixas,
               l_int32  thresh)
{
l_int32  i, n;
BOXA    *boxa;
PIX     *pix1, *pix2;
PIXA    *pixad;

    PROCNAME("pixaConvertTo1");

    if (!pixas)
        return (PIXA *)ERROR_PTR("pixas not defined", procName, NULL);

    n = pixaGetCount(pixas);
    pixad = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pix1 = pixaGetPix(pixas, i, L_CLONE);
        pix2 = pixConvertTo1(pix1, thresh);
        pixaAddPix(pixad, pix2, L_INSERT);
        pixDestroy(&pix1);
    }

    boxa = pixaGetBoxa(pixas, L_COPY);
    pixaSetBoxa(pixad, boxa, L_INSERT);
    return pixad;
}


/*!
 *  pixaConvertTo8()
 *
 *      Input:  pixas
 *              cmapflag (1 to give pixd a colormap; 0 otherwise)
 *      Return: pixad (each pix is 8 bpp), or null on error
 *
 *  Notes:
 *      (1) See notes for pixConvertTo8(), applied to each pix in pixas.
 */
PIXA *
pixaConvertTo8(PIXA    *pixas,
               l_int32  cmapflag)
{
l_int32  i, n;
BOXA    *boxa;
PIX     *pix1, *pix2;
PIXA    *pixad;

    PROCNAME("pixaConvertTo8");

    if (!pixas)
        return (PIXA *)ERROR_PTR("pixas not defined", procName, NULL);

    n = pixaGetCount(pixas);
    pixad = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pix1 = pixaGetPix(pixas, i, L_CLONE);
        pix2 = pixConvertTo8(pix1, cmapflag);
        pixaAddPix(pixad, pix2, L_INSERT);
        pixDestroy(&pix1);
    }

    boxa = pixaGetBoxa(pixas, L_COPY);
    pixaSetBoxa(pixad, boxa, L_INSERT);
    return pixad;
}


/*!
 *  pixaConvertTo8Color()
 *
 *      Input:  pixas
 *              ditherflag (1 to dither if necessary; 0 otherwise)
 *      Return: pixad (each pix is 8 bpp), or null on error
 *
 *  Notes:
 *      (1) See notes for pixConvertTo8Color(), applied to each pix in pixas.
 */
PIXA *
pixaConvertTo8Color(PIXA    *pixas,
                    l_int32  dither)
{
l_int32  i, n;
BOXA    *boxa;
PIX     *pix1, *pix2;
PIXA    *pixad;

    PROCNAME("pixaConvertTo8Color");

    if (!pixas)
        return (PIXA *)ERROR_PTR("pixas not defined", procName, NULL);

    n = pixaGetCount(pixas);
    pixad = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pix1 = pixaGetPix(pixas, i, L_CLONE);
        pix2 = pixConvertTo8Color(pix1, dither);
        pixaAddPix(pixad, pix2, L_INSERT);
        pixDestroy(&pix1);
    }

    boxa = pixaGetBoxa(pixas, L_COPY);
    pixaSetBoxa(pixad, boxa, L_INSERT);
    return pixad;
}


/*!
 *  pixaConvertTo32()
 *
 *      Input:  pixas
 *      Return: pixad (32 bpp rgb), or null on error
 *
 *  Notes:
 *      (1) See notes for pixConvertTo32(), applied to each pix in pixas.
 */
PIXA *
pixaConvertTo32(PIXA    *pixas)
{
l_int32  i, n;
BOXA    *boxa;
PIX     *pix1, *pix2;
PIXA    *pixad;

    PROCNAME("pixaConvertTo32");

    if (!pixas)
        return (PIXA *)ERROR_PTR("pixas not defined", procName, NULL);

    n = pixaGetCount(pixas);
    pixad = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pix1 = pixaGetPix(pixas, i, L_CLONE);
        pix2 = pixConvertTo32(pix1);
        pixaAddPix(pixad, pix2, L_INSERT);
        pixDestroy(&pix1);
    }

    boxa = pixaGetBoxa(pixas, L_COPY);
    pixaSetBoxa(pixad, boxa, L_INSERT);
    return pixad;
}


/*---------------------------------------------------------------------*
 *                               Tile N-Up                             *
 *---------------------------------------------------------------------*/
/*!
 *  convertToNUpFiles()
 *
 *      Input:  indir (full path to directory of images)
 *              substr (<optional> can be null)
 *              nx, ny (in [1, ... 50], tiling factors in each direction)
 *              tw (target width, in pixels; must be >= 20)
 *              spacing  (between images, and on outside)
 *              border (width of additional black border on each image;
 *                      use 0 for no border)
 *              fontdir (<optional> prints tail of filename with image)
 *              outdir (subdirectory of /tmp to put N-up tiled images)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Each set of nx*ny images is scaled and tiled into a single
 *          image, that is written out to @outdir.
 *      (2) All images in each nx*ny set are scaled to the same width.
 *          This is typically used when all images are roughly the same
 *          size.
 *      (3) Typical values for nx and ny are in [2 ... 5].
 *      (4) All images are scaled to a width @tw.  They are not rescaled
 *          when placed in the (nx,ny) mosaic.
 */
l_int32
convertToNUpFiles(const char  *dir,
                  const char  *substr,
                  l_int32      nx,
                  l_int32      ny,
                  l_int32      tw,
                  l_int32      spacing,
                  l_int32      border,
                  const char  *fontdir,
                  const char  *outdir)
{
l_int32  d, format;
char     rootpath[256];
PIXA    *pixa;

    PROCNAME("convertToNUpFiles");

    if (!dir)
        return ERROR_INT("dir not defined", procName, 1);
    if (nx < 1 || ny < 1 || nx > 50 || ny > 50)
        return ERROR_INT("invalid tiling N-factor", procName, 1);
    if (!outdir)
        return ERROR_INT("outdir not defined", procName, 1);

    pixa = convertToNUpPixa(dir, substr, nx, ny, tw, spacing, border,
                            fontdir);
    if (!pixa)
        return ERROR_INT("pixa not made", procName, 1);

    lept_rmdir(outdir);
    lept_mkdir(outdir);
    pixaGetRenderingDepth(pixa, &d);
    format = (d == 1) ? IFF_TIFF_G4 : IFF_JFIF_JPEG;
    makeTempDirname(rootpath, 256, outdir);
    modifyTrailingSlash(rootpath, 256, L_ADD_TRAIL_SLASH);
    pixaWriteFiles(rootpath, pixa, format);
    pixaDestroy(&pixa);
    return 0;
}


/*!
 *  convertToNUpPixa()
 *
 *      Input:  dir (full path to directory of images)
 *              substr (<optional> can be null)
 *              nx, ny (in [1, ... 50], tiling factors in each direction)
 *              tw (target width, in pixels; must be >= 20)
 *              spacing  (between images, and on outside)
 *              border (width of additional black border on each image;
 *                      use 0 for no border)
 *              fontdir (<optional> prints tail of filename with image)
 *      Return: pixad, or null on error
 *
 *  Notes:
 *      (1) See notes for convertToNUpFiles()
 */
PIXA *
convertToNUpPixa(const char  *dir,
                 const char  *substr,
                 l_int32      nx,
                 l_int32      ny,
                 l_int32      tw,
                 l_int32      spacing,
                 l_int32      border,
                 const char  *fontdir)
{
l_int32    i, j, k, nt, n2, nout, d;
char      *fname, *tail;
L_BMF     *bmf;
PIX       *pix1, *pix2, *pix3, *pix4;
PIXA      *pixat, *pixad;
SARRAY    *sa;

    PROCNAME("convertToNUpPixa");

    if (!dir)
        return (PIXA *)ERROR_PTR("dir not defined", procName, NULL);
    if (nx < 1 || ny < 1 || nx > 50 || ny > 50)
        return (PIXA *)ERROR_PTR("invalid tiling N-factor", procName, NULL);
    if (tw < 20)
        return (PIXA *)ERROR_PTR("tw must be >= 20", procName, NULL);

    sa = getSortedPathnamesInDirectory(dir, substr, 0, 0);
    nt = sarrayGetCount(sa);
    n2 = nx * ny;
    nout = (nt + n2 - 1) / n2;
    pixad = pixaCreate(nout);
    bmf = (fontdir) ? bmfCreate(fontdir, 6) : NULL;   /* 6 pt font */
    for (i = 0, j = 0; i < nout; i++) {
        pixat = pixaCreate(n2);
        for (k = 0; k < n2 && j < nt; j++, k++) {
            fname = sarrayGetString(sa, j, L_NOCOPY);
            if ((pix1 = pixRead(fname)) == NULL) {
                L_ERROR("image not read from %s\n", procName, fname);
                continue;
            }
            pix2 = pixScaleToSize(pix1, tw, 0);  /* all images have width tw */
            if (fontdir) {
                splitPathAtDirectory(fname, NULL, &tail);
                pix3 = pixAddSingleTextline(pix2, bmf, tail, 0xff000000,
                                            L_ADD_BELOW);
                FREE(tail);
            } else {
                pix3 = pixClone(pix2);
            }
            pixaAddPix(pixat, pix3, L_INSERT);
            pixDestroy(&pix1);
            pixDestroy(&pix2);
        }
        if (pixaGetCount(pixat) == 0) continue;
        pixaGetRenderingDepth(pixat, &d);
            /* add 2 * border to image width to prevent scaling */
        pix4 = pixaDisplayTiledAndScaled(pixat, d, tw + 2 * border, nx, 0,
                                         spacing, border);
        pixaAddPix(pixad, pix4, L_INSERT);
        pixaDestroy(&pixat);
    }

    sarrayDestroy(&sa);
    bmfDestroy(&bmf);
    return pixad;
}

