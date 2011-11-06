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
 *   pixafunc2.c
 *
 *      Pixa Display (render into a pix)
 *           PIX      *pixaDisplay()
 *           PIX      *pixaDisplayOnColor()
 *           PIX      *pixaDisplayRandomCmap()
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
 *  We give seven methods for displaying a pixa in a pix.
 *  Some work for 1 bpp input; others for any input depth.
 *  Some give an output depth that depends on the input depth;
 *  others give a different output depth or allow you to choose it.
 *  Some use a boxes to determine where each pix goes; others tile
 *  onto a regular lattice; yet others tile onto an irregular lattice.
 *
 *  Here is a brief description of what these functions do.
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
 *        edges of each pix in a row are alined and there is a uniform
 *        spacing between the pix.  The height of each row is determined
 *        by the tallest pix that was put in the row.  This function
 *        is a reasonably efficient way to pack the subimages.
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
        L_WARNING("no components; returning empty 1 bpp pix", procName);
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
            L_WARNING("no box found!", procName);
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
    }
    else 
        pixat = pixaCopy(pixa, L_CLONE);

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
    }
    else if (bgcolor > 0)
        pixSetAllArbitrary(pixd, bgcolor);

        /* Blit each pix into its place */
    for (i = 0; i < n; i++) {
        if (pixaGetBoxGeometry(pixat, i, &xb, &yb, &wb, &hb)) {
            L_WARNING("no box found!", procName);
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
l_int32   i, n, d, index, xb, yb, wb, hb;
BOXA     *boxa;
PIX      *pixs, *pixt, *pixd;
PIXCMAP  *cmap;

    PROCNAME("pixaDisplayRandomCmap");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);
    
    n = pixaGetCount(pixa);
    if (n == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);

        /* Use the first pix in pixa to verify depth is 1 bpp  */
    pixs = pixaGetPix(pixa, 0, L_CLONE);
    d = pixGetDepth(pixs);
    pixDestroy(&pixs);
    if (d != 1)
        return (PIX *)ERROR_PTR("components not 1 bpp", procName, NULL);

        /* If w and h not input, determine the minimum size required
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
 *  pixaDisplayOnLattice()
 *
 *      Input:  pixa
 *              xspace
 *              yspace
 *      Return: pix of composite images, or null on error
 *
 *  Notes:
 *      (1) This places each pix on sequentially on a regular lattice
 *          in the rendered composite.  If a pix is too large to fit in the
 *          allocated lattice space, it is not rendered.
 *      (2) If any pix has a colormap, all pix are rendered in rgb.
 *      (3) This is useful when putting bitmaps of components,
 *          such as characters, into a single image.
 */
PIX *
pixaDisplayOnLattice(PIXA    *pixa,
                     l_int32  xspace,
                     l_int32  yspace)
{
l_int32  n, nw, nh, w, h, d, wt, ht;
l_int32  index, i, j, hascmap;
PIX     *pix, *pixt, *pixd;
PIXA    *pixat;

    PROCNAME("pixaDisplayOnLattice");

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
    }
    else
        pixat = pixaCopy(pixa, L_CLONE);

    nw = (l_int32)sqrt((l_float64)n);
    nh = (n + nw - 1) / nw;
    w = xspace * nw;
    h = yspace * nh;

        /* Use the first pix in pixa to determine the depth.  */
    pixaGetPixDimensions(pixat, 0, NULL, NULL, &d);

    if ((pixd = pixCreate(w, h, d)) == NULL) {
        pixaDestroy(&pixat);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    
    index = 0;
    for (i = 0; i < nh; i++) {
        for (j = 0; j < nw && index < n; j++, index++) {
            pixt = pixaGetPix(pixat, index, L_CLONE);
            pixGetDimensions(pixt, &wt, &ht, NULL);
            if (wt > xspace || ht > yspace) {
                fprintf(stderr, "pix(%d) omitted; size %dx%d\n", index, wt, ht);
                pixDestroy(&pixt);
                continue;
            }
            pixRasterop(pixd, j * xspace, i * yspace, wt, ht, 
                        PIX_PAINT, pixt, 0, 0);
            pixDestroy(&pixt);
        }
    }

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
 *      (1) This saves a pixa to a single image file of width not to
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
    }
    else
        pixat = pixaCopy(pixa, L_CLONE);

        /* Find the largest width and height of the subimages */
    wmax = hmax = 0;
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixat, i, L_CLONE);
        pixGetDimensions(pix, &w, &h, NULL);
        if (i == 0)
            d = pixGetDepth(pix);
        else if (d != pixGetDepth(pix)) {
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

#if 0
    fprintf(stderr, " nrows = %d, ncols = %d, wmax = %d, hmax = %d\n",
            nrows, ncols, wmax, hmax);
    fprintf(stderr, " space = %d, wd = %d, hd = %d, n = %d\n",
            space, wd, hd, n);
#endif

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
 *      (1) This saves a pixa to a single image file of width not to
 *          exceed maxwidth, with background color either white or black,
 *          and with each row tiled such that the top of each pix is
 *          aligned and separated by 'spacing' from the next one.
 *          A black border can be added to each pix.
 *      (2) All pix are converted to outdepth; existing colormaps are removed.
 *      (3) This does a reasonably spacewise-efficient job of laying
 *          out the individual pix images into a tiled composite.
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
l_int32  h;  /* cumulative height over all the rows */
l_int32  w;  /* cumulative height in the current row */
l_int32  bordval, wtry, wt, ht;
l_int32  irow;  /* index of current pix in current row */
l_int32  wmaxrow;  /* width of the largest row */
l_int32  maxh;  /* max height in row */
l_int32  i, j, index, n, x, y, nrows, ninrow;
NUMA    *nainrow;  /* number of pix in the row */
NUMA    *namaxh;  /* height of max pix in the row */
PIX     *pix, *pixn, *pixt, *pixd;
PIXA    *pixan;

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
        L_WARNING_INT("only got %d components", procName, n);
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

        /* Blit the images to the dest */
    nrows = numaGetCount(nainrow);
    y = spacing;
    for (i = 0, index = 0; i < nrows; i++) {  /* over rows */
        numaGetIValue(nainrow, i, &ninrow);
        numaGetIValue(namaxh, i, &maxh);
        x = spacing;
        for (j = 0; j < ninrow; j++, index++) {   /* over pix in row */
            pix = pixaGetPix(pixan, index, L_CLONE);
            pixGetDimensions(pix, &wt, &ht, NULL);
            pixRasterop(pixd, x, y, wt, ht, PIX_SRC, pix, 0, 0);
            pixDestroy(&pix);
            x += wt + spacing;
        }
        y += maxh + spacing;
    }

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
 *      Input:  pixaa
 *              w, h (if set to 0, determines the size from the
 *                    b.b. of the components in pixaa)
 *      Return: pix, or null on error
 *
 *  Notes:
 *      (1) Each pix of the pixaa is displayed at the location given by
 *          its box, translated by the box of the containing pixa
 *          if it exists.
 */
PIX *
pixaaDisplay(PIXAA   *pixaa,
             l_int32  w,
             l_int32  h)
{
l_int32  i, j, n, nbox, na, d, wmax, hmax, x, y, xb, yb, wb, hb;
BOXA    *boxa1;  /* top-level boxa */
BOXA    *boxa;
PIX     *pixt, *pixd;
PIXA    *pixa;

    PROCNAME("pixaaDisplay");

    if (!pixaa)
        return (PIX *)ERROR_PTR("pixaa not defined", procName, NULL);
    
    n = pixaaGetCount(pixaa);
    if (n == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);

        /* If w and h not input, determine the minimum size required
         * to contain the origin and all c.c. */
    boxa1 = pixaaGetBoxa(pixaa, L_CLONE);
    nbox = boxaGetCount(boxa1);
    if (w == 0 || h == 0) {
        if (nbox == n)
            boxaGetExtent(boxa1, &w, &h, NULL);
        else {  /* have to use the lower-level boxa for each pixa */
            wmax = hmax = 0;
            for (i = 0; i < n; i++) {
                pixa = pixaaGetPixa(pixaa, i, L_CLONE);
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
    pixa = pixaaGetPixa(pixaa, 0, L_CLONE);
    pixt = pixaGetPix(pixa, 0, L_CLONE);
    d = pixGetDepth(pixt);
    pixaDestroy(&pixa);
    pixDestroy(&pixt);

    if ((pixd = pixCreate(w, h, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    
    x = y = 0;
    for (i = 0; i < n; i++) {
        pixa = pixaaGetPixa(pixaa, i, L_CLONE);
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
 *      Input:  pixaa
 *              xspace between pix in pixa
 *              yspace between pixa
 *              max width of output pix
 *      Return: pix, or null on error
 *
 *  Notes:
 *      (1) Displays each pixa on a line (or set of lines),
 *          in order from top to bottom.  Within each pixa,
 *          the pix are displayed in order from left to right.
 *      (2) The size of each pix in each pixa is assumed to be
 *          approximately equal to the size of the first pix in
 *          the pixa.  If this assumption is not correct, this
 *          function will not work properly.
 *      (3) This ignores the boxa of the pixaa.
 */
PIX *
pixaaDisplayByPixa(PIXAA   *pixaa,
                   l_int32  xspace,
                   l_int32  yspace,
                   l_int32  maxw)
{
l_int32  i, j, npixa, npix;
l_int32  width, height, depth, nlines, lwidth;
l_int32  x, y, w, h, w0, h0;
PIX     *pixt, *pixd;
PIXA    *pixa;

    PROCNAME("pixaaDisplayByPixa");

    if (!pixaa)
        return (PIX *)ERROR_PTR("pixaa not defined", procName, NULL);
    
    if ((npixa = pixaaGetCount(pixaa)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);

        /* Get size of output pix.  The width is the minimum of the
         * maxw and the largest pixa line width.  The height is whatever
         * it needs to be to accommodate all pixa. */
    height = 2 * yspace;
    width = 0;
    for (i = 0; i < npixa; i++) {
        pixa = pixaaGetPixa(pixaa, i, L_CLONE);
        npix = pixaGetCount(pixa);
        pixt = pixaGetPix(pixa, 0, L_CLONE);
        if (i == 0)
            depth = pixGetDepth(pixt);
        w = pixGetWidth(pixt);
        lwidth = npix * (w + xspace);
        nlines = (lwidth + maxw - 1) / maxw;
        if (nlines > 1)
            width = maxw;
        else
            width = L_MAX(lwidth, width);
        height += nlines * (pixGetHeight(pixt) + yspace);
        pixDestroy(&pixt);
        pixaDestroy(&pixa);
    }

    if ((pixd = pixCreate(width, height, depth)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

        /* Now layout the pix by pixa */
    y = yspace;
    for (i = 0; i < npixa; i++) {
        x = 0;
        pixa = pixaaGetPixa(pixaa, i, L_CLONE);
        npix = pixaGetCount(pixa);
        for (j = 0; j < npix; j++) {
            pixt = pixaGetPix(pixa, j, L_CLONE);
            if (j == 0) {
                w0 = pixGetWidth(pixt);
                h0 = pixGetHeight(pixt);
            }
            w = pixGetWidth(pixt);
            if (width == maxw && x + w >= maxw) {
                x = 0;
                y += h0 + yspace;
            }
            h = pixGetHeight(pixt);
            pixRasterop(pixd, x, y, w, h, PIX_PAINT, pixt, 0, 0);
            pixDestroy(&pixt);
            x += w0 + xspace;
        }
        y += h0 + yspace;
        pixaDestroy(&pixa);
    }

    return pixd;
}


/*!
 *  pixaaDisplayTiledAndScaled()
 *
 *      Input:  pixaa
 *              outdepth (output depth: 1, 8 or 32 bpp)
 *              tilewidth (each pix is scaled to this width)
 *              ncols (number of tiles in each row)
 *              background (0 for white, 1 for black; this is the color
 *                 of the spacing between the images)
 *              spacing  (between images, and on outside)
 *              border (width of additional black border on each image;
 *                      use 0 for no border)
 *      Return: pixa (of tiled images, one image for each pixa in
 *                    the pixaa), or null on error
 *
 *  Notes:
 *      (1) For each pixa, this generates from all the pix a
 *          tiled/scaled output pix, and puts it in the output pixa.
 *      (2) See comments in pixaDisplayTiledAndScaled().
 */
PIXA *
pixaaDisplayTiledAndScaled(PIXAA   *pixaa,
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

    if (!pixaa)
        return (PIXA *)ERROR_PTR("pixaa not defined", procName, NULL);
    if (outdepth != 1 && outdepth != 8 && outdepth != 32)
        return (PIXA *)ERROR_PTR("outdepth not in {1, 8, 32}", procName, NULL);
    if (border < 0 || border > tilewidth / 5)
        border = 0;
    
    if ((n = pixaaGetCount(pixaa)) == 0)
        return (PIXA *)ERROR_PTR("no components", procName, NULL);

    pixad = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pixa = pixaaGetPixa(pixaa, i, L_CLONE);
        pix = pixaDisplayTiledAndScaled(pixa, outdepth, tilewidth, ncols,
                                        background, spacing, border);
        pixaAddPix(pixad, pix, L_INSERT);
        pixaDestroy(&pixa);
    }

    return pixad;
}
