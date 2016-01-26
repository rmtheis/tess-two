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
 *  seedfill.c
 *
 *      Binary seedfill (source: Luc Vincent)
 *               PIX      *pixSeedfillBinary()
 *               PIX      *pixSeedfillBinaryRestricted()
 *
 *      Applications of binary seedfill to find and fill holes,
 *      remove c.c. touching the border and fill bg from border:
 *               PIX      *pixHolesByFilling()
 *               PIX      *pixFillClosedBorders()
 *               PIX      *pixExtractBorderConnComps()
 *               PIX      *pixRemoveBorderConnComps()
 *               PIX      *pixFillBgFromBorder()
 *
 *      Hole-filling of components to bounding rectangle
 *               PIX      *pixFillHolesToBoundingRect()
 *
 *      Gray seedfill (source: Luc Vincent:fast-hybrid-grayscale-reconstruction)
 *               l_int32   pixSeedfillGray()
 *               l_int32   pixSeedfillGrayInv()
 *
 *      Gray seedfill (source: Luc Vincent: sequential-reconstruction algorithm)
 *               l_int32   pixSeedfillGraySimple()
 *               l_int32   pixSeedfillGrayInvSimple()
 *
 *      Gray seedfill variations
 *               PIX      *pixSeedfillGrayBasin()
 *
 *      Distance function (source: Luc Vincent)
 *               PIX      *pixDistanceFunction()
 *
 *      Seed spread (based on distance function)
 *               PIX      *pixSeedspread()
 *
 *      Local extrema:
 *               l_int32   pixLocalExtrema()
 *        static l_int32   pixQualifyLocalMinima()
 *               l_int32   pixSelectedLocalExtrema()
 *               PIX      *pixFindEqualValues()
 *
 *      Selection of minima in mask of connected components
 *               PTA      *pixSelectMinInConnComp()
 *
 *      Removal of seeded connected components from a mask
 *               PIX      *pixRemoveSeededComponents()
 *
 *
 *           ITERATIVE RASTER-ORDER SEEDFILL
 *
 *      The basic method in the Vincent seedfill (aka reconstruction)
 *      algorithm is simple.  We describe here the situation for
 *      binary seedfill.  Pixels are sampled in raster order in
 *      the seed image.  If they are 4-connected to ON pixels
 *      either directly above or to the left, and are not masked
 *      out by the mask image, they are turned on (or remain on).
 *      (Ditto for 8-connected, except you need to check 3 pixels
 *      on the previous line as well as the pixel to the left
 *      on the current line.  This is extra computational work
 *      for relatively little gain, so it is preferable
 *      in most situations to use the 4-connected version.)
 *      The algorithm proceeds from UR to LL of the image, and
 *      then reverses and sweeps up from LL to UR.
 *      These double sweeps are iterated until there is no change.
 *      At this point, the seed has entirely filled the region it
 *      is allowed to, as delimited by the mask image.
 *
 *      The grayscale seedfill is a straightforward generalization
 *      of the binary seedfill, and is described in seedfillLowGray().
 *
 *      For some applications, the filled seed will later be OR'd
 *      with the negative of the mask.   This is used, for example,
 *      when you flood fill into a 4-connected region of OFF pixels
 *      and you want the result after those pixels are turned ON.
 *
 *      Note carefully that the mask we use delineates which pixels
 *      are allowed to be ON as the seed is filled.  We will call this
 *      a "filling mask".  As the seed expands, it is repeatedly
 *      ANDed with the filling mask: s & fm.  The process can equivalently
 *      be formulated using the inverse of the filling mask, which
 *      we will call a "blocking mask": bm = ~fm.   As the seed
 *      expands, the blocking mask is repeatedly used to prevent
 *      the seed from expanding into the blocking mask.  This is done
 *      by set subtracting the blocking mask from the expanded seed:
 *      s - bm.  Set subtraction of the blocking mask is equivalent
 *      to ANDing with the inverse of the blocking mask: s & (~bm).
 *      But from the inverse relation between blocking and filling
 *      masks, this is equal to s & fm, which proves the equivalence.
 *
 *      For efficiency, the pixels can be taken in larger units
 *      for processing, but still in raster order.  It is natural
 *      to take them in 32-bit words.  The outline of the work
 *      to be done for 4-cc (not including special cases for boundary
 *      words, such as the first line or the last word in each line)
 *      is as follows.  Let the filling mask be m.  The
 *      seed is to fill "under" the mask; i.e., limited by an AND
 *      with the mask.  Let the current word be w, the word
 *      in the line above be wa, and the previous word in the
 *      current line be wp.   Let t be a temporary word that
 *      is used in computation.  Note that masking is performed by
 *      w & m.  (If we had instead used a "blocking" mask, we
 *      would perform masking by the set subtraction operation,
 *      w - m, which is defined to be w & ~m.)
 *
 *      The entire operation can be implemented with shifts,
 *      logical operations and tests.  For each word in the seed image
 *      there are two steps.  The first step is to OR the word with
 *      the word above and with the rightmost pixel in wp (call it "x").
 *      Because wp is shifted one pixel to its right, "x" is ORed
 *      to the leftmost pixel of w.  We then clip to the ON pixels in
 *      the mask.  The result is
 *               t  <--  (w | wa | x000... ) & m
 *      We've now finished taking data from above and to the left.
 *      The second step is to allow filling to propagate horizontally
 *      in t, always making sure that it is properly masked at each
 *      step.  So if filling can be done (i.e., t is neither all 0s
 *      nor all 1s), iteratively take:
 *           t  <--  (t | (t >> 1) | (t << 1)) & m
 *      until t stops changing.  Then write t back into w.
 *
 *      Finally, the boundary conditions require we note that in doing
 *      the above steps:
 *          (a) The words in the first row have no wa
 *          (b) The first word in each row has no wp in that row
 *          (c) The last word in each row must be masked so that
 *              pixels don't propagate beyond the right edge of the
 *              actual image.  (This is easily accomplished by
 *              setting the out-of-bound pixels in m to OFF.)
 */

#include "allheaders.h"

#ifndef  NO_CONSOLE_IO
#define   DEBUG_PRINT_ITERS    0
#endif  /* ~NO_CONSOLE_IO */

  /* Two-way (UL --> LR, LR --> UL) sweep iterations; typically need only 4 */
static const l_int32  MAX_ITERS = 40;

    /* Static function */
static l_int32 pixQualifyLocalMinima(PIX *pixs, PIX *pixm, l_int32 maxval);


/*-----------------------------------------------------------------------*
 *              Vincent's Iterative Binary Seedfill method               *
 *-----------------------------------------------------------------------*/
/*!
 *  pixSeedfillBinary()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs; 1 bpp)
 *              pixs  (1 bpp seed)
 *              pixm  (1 bpp filling mask)
 *              connectivity  (4 or 8)
 *      Return: pixd always
 *
 *  Notes:
 *      (1) This is for binary seedfill (aka "binary reconstruction").
 *      (2) There are 3 cases:
 *            (a) pixd == null (make a new pixd)
 *            (b) pixd == pixs (in-place)
 *            (c) pixd != pixs
 *      (3) If you know the case, use these patterns for clarity:
 *            (a) pixd = pixSeedfillBinary(NULL, pixs, ...);
 *            (b) pixSeedfillBinary(pixs, pixs, ...);
 *            (c) pixSeedfillBinary(pixd, pixs, ...);
 *      (4) The resulting pixd contains the filled seed.  For some
 *          applications you want to OR it with the inverse of
 *          the filling mask.
 *      (5) The input seed and mask images can be different sizes, but
 *          in typical use the difference, if any, would be only
 *          a few pixels in each direction.  If the sizes differ,
 *          the clipping is handled by the low-level function
 *          seedfillBinaryLow().
 */
PIX *
pixSeedfillBinary(PIX     *pixd,
                  PIX     *pixs,
                  PIX     *pixm,
                  l_int32  connectivity)
{
l_int32    i, boolval;
l_int32    hd, hm, wpld, wplm;
l_uint32  *datad, *datam;
PIX       *pixt;

    PROCNAME("pixSeedfillBinary");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, pixd);
    if (!pixm || pixGetDepth(pixm) != 1)
        return (PIX *)ERROR_PTR("pixm undefined or not 1 bpp", procName, pixd);
    if (connectivity != 4 && connectivity != 8)
        return (PIX *)ERROR_PTR("connectivity not in {4,8}", procName, pixd);

        /* Prepare pixd as a copy of pixs if not identical */
    if ((pixd = pixCopy(pixd, pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

        /* pixt is used to test for completion */
    if ((pixt = pixCreateTemplate(pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, pixd);

    hd = pixGetHeight(pixd);
    hm = pixGetHeight(pixm);  /* included so seedfillBinaryLow() can clip */
    datad = pixGetData(pixd);
    datam = pixGetData(pixm);
    wpld = pixGetWpl(pixd);
    wplm = pixGetWpl(pixm);

    pixSetPadBits(pixm, 0);

    for (i = 0; i < MAX_ITERS; i++) {
        pixCopy(pixt, pixd);
        seedfillBinaryLow(datad, hd, wpld, datam, hm, wplm, connectivity);
        pixEqual(pixd, pixt, &boolval);
        if (boolval == 1) {
#if DEBUG_PRINT_ITERS
            fprintf(stderr, "Binary seed fill converged: %d iters\n", i + 1);
#endif  /* DEBUG_PRINT_ITERS */
            break;
        }
    }

    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixSeedfillBinaryRestricted()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs; 1 bpp)
 *              pixs  (1 bpp seed)
 *              pixm  (1 bpp filling mask)
 *              connectivity  (4 or 8)
 *              xmax (max distance in x direction of fill into the mask)
 *              ymax (max distance in y direction of fill into the mask)
 *      Return: pixd always
 *
 *  Notes:
 *      (1) See usage for pixSeedfillBinary(), which has unrestricted fill.
 *          In pixSeedfillBinary(), the filling distance is unrestricted
 *          and can be larger than pixs, depending on the topology of
 *          th mask.
 *      (2) There are occasions where it is useful not to permit the
 *          fill to go more than a certain distance into the mask.
 *          @xmax specifies the maximum horizontal distance allowed
 *          in the fill; @ymax does likewise in the vertical direction.
 *      (3) Operationally, the max "distance" allowed for the fill
 *          is a linear distance from the original seed, independent
 *          of the actual mask topology.
 *      (4) Another formulation of this problem, not implemented,
 *          would use the manhattan distance from the seed, as
 *          determined by a breadth-first search starting at the seed
 *          boundaries and working outward where the mask fg allows.
 *          How this might use the constraints of separate xmax and ymax
 *          is not clear.
 */
PIX *
pixSeedfillBinaryRestricted(PIX     *pixd,
                            PIX     *pixs,
                            PIX     *pixm,
                            l_int32  connectivity,
                            l_int32  xmax,
                            l_int32  ymax)
{
l_int32  w, h;
PIX     *pix1, *pix2;

    PROCNAME("pixSeedfillBinaryRestricted");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, pixd);
    if (!pixm || pixGetDepth(pixm) != 1)
        return (PIX *)ERROR_PTR("pixm undefined or not 1 bpp", procName, pixd);
    if (connectivity != 4 && connectivity != 8)
        return (PIX *)ERROR_PTR("connectivity not in {4,8}", procName, pixd);
    if (xmax == 0 && ymax == 0)  /* no filling permitted */
        return pixClone(pixs);
    if (xmax < 0 || ymax < 0) {
        L_ERROR("xmax and ymax must be non-negative", procName);
        return pixClone(pixs);
    }

        /* Full fill from the seed into the mask. */
    if ((pix1 = pixSeedfillBinary(NULL, pixs, pixm, connectivity)) == NULL)
        return (PIX *)ERROR_PTR("pix1 not made", procName, pixd);

        /* Dilate the seed.  This gives the maximal region where changes
         * are permitted.  Invert to get the region where pixs is
         * not allowed to change.  */
    pix2 = pixDilateCompBrick(NULL, pixs, 2 * xmax + 1, 2 * ymax + 1);
    pixInvert(pix2, pix2);

        /* Blank the region of pix1 specified by the fg of pix2.
         * This is not yet the final result, because it may have fg pixels
         * that are not accessible from the seed in the restricted distance.
         * For example, such pixels may be connected to the original seed,
         * but through a path that goes outside the permitted region. */
    pixGetDimensions(pixs, &w, &h, NULL);
    pixRasterop(pix1, 0, 0, w, h, PIX_DST & PIX_NOT(PIX_SRC), pix2, 0, 0);

        /* To get the accessible pixels in the restricted region, do
         * a second seedfill from the original seed, using pix1 as
         * a mask.  The result, in pixd, will not have any bad fg
         * pixels that were in pix1. */
    pixd = pixSeedfillBinary(pixd, pixs, pix1, connectivity);

    pixDestroy(&pix1);
    pixDestroy(&pix2);
    return pixd;
}


/*!
 *  pixHolesByFilling()
 *
 *      Input:  pixs (1 bpp)
 *              connectivity (4 or 8)
 *      Return: pixd  (inverted image of all holes), or null on error
 *
 * Action:
 *     (1) Start with 1-pixel black border on otherwise white pixd
 *     (2) Use the inverted pixs as the filling mask to fill in
 *         all the pixels from the border to the pixs foreground
 *     (3) OR the result with pixs to have an image with all
 *         ON pixels except for the holes.
 *     (4) Invert the result to get the holes as foreground
 *
 * Notes:
 *     (1) To get 4-c.c. holes of the 8-c.c. as foreground, use
 *         4-connected filling; to get 8-c.c. holes of the 4-c.c.
 *         as foreground, use 8-connected filling.
 */
PIX *
pixHolesByFilling(PIX     *pixs,
                  l_int32  connectivity)
{
PIX  *pixsi, *pixd;

    PROCNAME("pixHolesByFilling");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (connectivity != 4 && connectivity != 8)
        return (PIX *)ERROR_PTR("connectivity not 4 or 8", procName, NULL);

    if ((pixd = pixCreateTemplate(pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    if ((pixsi = pixInvert(NULL, pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixsi not made", procName, NULL);

    pixSetOrClearBorder(pixd, 1, 1, 1, 1, PIX_SET);
    pixSeedfillBinary(pixd, pixd, pixsi, connectivity);
    pixOr(pixd, pixd, pixs);
    pixInvert(pixd, pixd);
    pixDestroy(&pixsi);

    return pixd;
}


/*!
 *  pixFillClosedBorders()
 *
 *      Input:  pixs (1 bpp)
 *              filling connectivity (4 or 8)
 *      Return: pixd  (all topologically outer closed borders are filled
 *                     as connected comonents), or null on error
 *
 *  Notes:
 *      (1) Start with 1-pixel black border on otherwise white pixd
 *      (2) Subtract input pixs to remove border pixels that were
 *          also on the closed border
 *      (3) Use the inverted pixs as the filling mask to fill in
 *          all the pixels from the outer border to the closed border
 *          on pixs
 *      (4) Invert the result to get the filled component, including
 *          the input border
 *      (5) If the borders are 4-c.c., use 8-c.c. filling, and v.v.
 *      (6) Closed borders within c.c. that represent holes, etc., are filled.
 */
PIX *
pixFillClosedBorders(PIX     *pixs,
                     l_int32  connectivity)
{
PIX  *pixsi, *pixd;

    PROCNAME("pixFillClosedBorders");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (connectivity != 4 && connectivity != 8)
        return (PIX *)ERROR_PTR("connectivity not 4 or 8", procName, NULL);

    if ((pixd = pixCreateTemplate(pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixSetOrClearBorder(pixd, 1, 1, 1, 1, PIX_SET);
    pixSubtract(pixd, pixd, pixs);
    if ((pixsi = pixInvert(NULL, pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixsi not made", procName, NULL);

    pixSeedfillBinary(pixd, pixd, pixsi, connectivity);
    pixInvert(pixd, pixd);
    pixDestroy(&pixsi);

    return pixd;
}


/*!
 *  pixExtractBorderConnComps()
 *
 *      Input:  pixs (1 bpp)
 *              filling connectivity (4 or 8)
 *      Return: pixd  (all pixels in the src that are in connected
 *                     components touching the border), or null on error
 */
PIX *
pixExtractBorderConnComps(PIX     *pixs,
                          l_int32  connectivity)
{
PIX  *pixd;

    PROCNAME("pixExtractBorderConnComps");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (connectivity != 4 && connectivity != 8)
        return (PIX *)ERROR_PTR("connectivity not 4 or 8", procName, NULL);

        /* Start with 1 pixel wide black border as seed in pixd */
    if ((pixd = pixCreateTemplate(pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixSetOrClearBorder(pixd, 1, 1, 1, 1, PIX_SET);

       /* Fill in pixd from the seed, using pixs as the filling mask.
        * This fills all components from pixs that are touching the border. */
    pixSeedfillBinary(pixd, pixd, pixs, connectivity);

    return pixd;
}


/*!
 *  pixRemoveBorderConnComps()
 *
 *      Input:  pixs (1 bpp)
 *              filling connectivity (4 or 8)
 *      Return: pixd  (all pixels in the src that are not touching the
 *                     border) or null on error
 *
 *  Notes:
 *      (1) This removes all fg components touching the border.
 */
PIX *
pixRemoveBorderConnComps(PIX     *pixs,
                         l_int32  connectivity)
{
PIX  *pixd;

    PROCNAME("pixRemoveBorderConnComps");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (connectivity != 4 && connectivity != 8)
        return (PIX *)ERROR_PTR("connectivity not 4 or 8", procName, NULL);

       /* Fill from a 1 pixel wide seed at the border into all components
        * in pixs (the filling mask) that are touching the border */
    pixd = pixExtractBorderConnComps(pixs, connectivity);

       /* Save in pixd only those components in pixs not touching the border */
    pixXor(pixd, pixd, pixs);
    return pixd;
}


/*!
 *  pixFillBgFromBorder()
 *
 *      Input:  pixs (1 bpp)
 *              filling connectivity (4 or 8)
 *      Return: pixd (with the background c.c. touching the border
 *                    filled to foreground), or null on error
 *
 *  Notes:
 *      (1) This fills all bg components touching the border to fg.
 *          It is the photometric inverse of pixRemoveBorderConnComps().
 *      (2) Invert the result to get the "holes" left after this fill.
 *          This can be done multiple times, extracting holes within
 *          holes after each pair of fillings.  Specifically, this code
 *          peels away n successive embeddings of components:
 *              pix1 = <initial image>
 *              for (i = 0; i < 2 * n; i++) {
 *                   pix2 = pixFillBgFromBorder(pix1, 8);
 *                   pixInvert(pix2, pix2);
 *                   pixDestroy(&pix1);
 *                   pix1 = pix2;
 *              }

 */
PIX *
pixFillBgFromBorder(PIX     *pixs,
                    l_int32  connectivity)
{
PIX  *pixd;

    PROCNAME("pixFillBgFromBorder");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (connectivity != 4 && connectivity != 8)
        return (PIX *)ERROR_PTR("connectivity not 4 or 8", procName, NULL);

       /* Invert to turn bg touching the border to a fg component.
        * Extract this by filling from a 1 pixel wide seed at the border. */
    pixInvert(pixs, pixs);
    pixd = pixExtractBorderConnComps(pixs, connectivity);
    pixInvert(pixs, pixs);  /* restore pixs */

       /* Bit-or the filled bg component with pixs */
    pixOr(pixd, pixd, pixs);
    return pixd;
}


/*-----------------------------------------------------------------------*
 *            Hole-filling of components to bounding rectangle           *
 *-----------------------------------------------------------------------*/
/*!
 *  pixFillHolesToBoundingRect()
 *
 *      Input:  pixs (1 bpp)
 *              minsize (min number of pixels in the hole)
 *              maxhfract (max hole area as fraction of fg pixels in the cc)
 *              minfgfract (min fg area as fraction of bounding rectangle)
 *      Return: pixd (pixs, with some holes possibly filled and some c.c.
 *                    possibly expanded to their bounding rects),
 *                    or null on error
 *
 *  Notes:
 *      (1) This does not fill holes that are smaller in area than 'minsize'.
 *      (2) This does not fill holes with an area larger than
 *          'maxhfract' times the fg area of the c.c.
 *      (3) This does not expand the fg of the c.c. to bounding rect if
 *          the fg area is less than 'minfgfract' times the area of the
 *          bounding rect.
 *      (4) The decisions are made as follows:
 *           - Decide if we are filling the holes; if so, when using
 *             the fg area, include the filled holes.
 *           - Decide based on the fg area if we are filling to a bounding rect.
 *             If so, do it.
 *             If not, fill the holes if the condition is satisfied.
 *      (5) The choice of minsize depends on the resolution.
 *      (6) For solidifying image mask regions on printed materials,
 *          which tend to be rectangular, values for maxhfract
 *          and minfgfract around 0.5 are reasonable.
 */
PIX *
pixFillHolesToBoundingRect(PIX       *pixs,
                           l_int32    minsize,
                           l_float32  maxhfract,
                           l_float32  minfgfract)
{
l_int32    i, x, y, w, h, n, nfg, nh, ntot, area;
l_int32   *tab;
l_float32  hfract;  /* measured hole fraction */
l_float32  fgfract;  /* measured fg fraction */
BOXA      *boxa;
PIX       *pixd, *pixfg, *pixh;
PIXA      *pixa;

    PROCNAME("pixFillHolesToBoundingRect");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);

    pixd = pixCopy(NULL, pixs);
    boxa = pixConnComp(pixd, &pixa, 8);
    n = boxaGetCount(boxa);
    tab = makePixelSumTab8();
    for (i = 0; i < n; i++) {
        boxaGetBoxGeometry(boxa, i, &x, &y, &w, &h);
        area = w * h;
        if (area < minsize)
            continue;
        pixfg = pixaGetPix(pixa, i, L_COPY);
        pixh = pixHolesByFilling(pixfg, 4);  /* holes only */
        pixCountPixels(pixfg, &nfg, tab);
        pixCountPixels(pixh, &nh, tab);
        hfract = (l_float32)nh / (l_float32)nfg;
        ntot = nfg;
        if (hfract <= maxhfract)  /* we will fill the holes (at least) */
            ntot = nfg + nh;
        fgfract = (l_float32)ntot / (l_float32)area;
        if (fgfract >= minfgfract) {  /* fill to bounding rect */
            pixSetAll(pixfg);
            pixRasterop(pixd, x, y, w, h, PIX_SRC, pixfg, 0, 0);
        } else if (hfract <= maxhfract) {  /* fill just the holes */
            pixRasterop(pixd, x, y, w, h, PIX_DST | PIX_SRC , pixh, 0, 0);
        }
        pixDestroy(&pixfg);
        pixDestroy(&pixh);
    }
    boxaDestroy(&boxa);
    pixaDestroy(&pixa);
    LEPT_FREE(tab);

    return pixd;
}


/*-----------------------------------------------------------------------*
 *             Vincent's hybrid Grayscale Seedfill method             *
 *-----------------------------------------------------------------------*/
/*!
 *  pixSeedfillGray()
 *
 *      Input:  pixs  (8 bpp seed; filled in place)
 *              pixm  (8 bpp filling mask)
 *              connectivity  (4 or 8)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is an in-place filling operation on the seed, pixs,
 *          where the clipping mask is always above or at the level
 *          of the seed as it is filled.
 *      (2) For details of the operation, see the description in
 *          seedfillGrayLow() and the code there.
 *      (3) As an example of use, see the description in pixHDome().
 *          There, the seed is an image where each pixel is a fixed
 *          amount smaller than the corresponding mask pixel.
 *      (4) Reference paper :
 *            L. Vincent, Morphological grayscale reconstruction in image
 *            analysis: applications and efficient algorithms, IEEE Transactions
 *            on  Image Processing, vol. 2, no. 2, pp. 176-201, 1993.
 */
l_int32
pixSeedfillGray(PIX     *pixs,
                PIX     *pixm,
                l_int32  connectivity)
{
l_int32    h, w, wpls, wplm;
l_uint32  *datas, *datam;

    PROCNAME("pixSeedfillGray");

    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (!pixm || pixGetDepth(pixm) != 8)
        return ERROR_INT("pixm not defined or not 8 bpp", procName, 1);
    if (connectivity != 4 && connectivity != 8)
        return ERROR_INT("connectivity not in {4,8}", procName, 1);

        /* Make sure the sizes of seed and mask images are the same */
    if (pixSizesEqual(pixs, pixm) == 0)
        return ERROR_INT("pixs and pixm sizes differ", procName, 1);

    datas = pixGetData(pixs);
    datam = pixGetData(pixm);
    wpls = pixGetWpl(pixs);
    wplm = pixGetWpl(pixm);
    pixGetDimensions(pixs, &w, &h, NULL);
    seedfillGrayLow(datas, w, h, wpls, datam, wplm, connectivity);

    return 0;
}


/*!
 *  pixSeedfillGrayInv()
 *
 *      Input:  pixs  (8 bpp seed; filled in place)
 *              pixm  (8 bpp filling mask)
 *              connectivity  (4 or 8)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is an in-place filling operation on the seed, pixs,
 *          where the clipping mask is always below or at the level
 *          of the seed as it is filled.  Think of filling up a basin
 *          to a particular level, given by the maximum seed value
 *          in the basin.  Outside the filled region, the mask
 *          is above the filling level.
 *      (2) Contrast this with pixSeedfillGray(), where the clipping mask
 *          is always above or at the level of the fill.  An example
 *          of its use is the hdome fill, where the seed is an image
 *          where each pixel is a fixed amount smaller than the
 *          corresponding mask pixel.
 *      (3) The basin fill, pixSeedfillGrayBasin(), is a special case
 *          where the seed pixel values are generated from the mask,
 *          and where the implementation uses pixSeedfillGray() by
 *          inverting both the seed and mask.
 */
l_int32
pixSeedfillGrayInv(PIX     *pixs,
                   PIX     *pixm,
                   l_int32  connectivity)
{
l_int32    h, w, wpls, wplm;
l_uint32  *datas, *datam;

    PROCNAME("pixSeedfillGrayInv");

    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (!pixm || pixGetDepth(pixm) != 8)
        return ERROR_INT("pixm not defined or not 8 bpp", procName, 1);
    if (connectivity != 4 && connectivity != 8)
        return ERROR_INT("connectivity not in {4,8}", procName, 1);

        /* Make sure the sizes of seed and mask images are the same */
    if (pixSizesEqual(pixs, pixm) == 0)
        return ERROR_INT("pixs and pixm sizes differ", procName, 1);

    datas = pixGetData(pixs);
    datam = pixGetData(pixm);
    wpls = pixGetWpl(pixs);
    wplm = pixGetWpl(pixm);
    pixGetDimensions(pixs, &w, &h, NULL);
    seedfillGrayInvLow(datas, w, h, wpls, datam, wplm, connectivity);

    return 0;
}

/*-----------------------------------------------------------------------*
 *             Vincent's Iterative Grayscale Seedfill method             *
 *-----------------------------------------------------------------------*/
/*!
 *  pixSeedfillGraySimple()
 *
 *      Input:  pixs  (8 bpp seed; filled in place)
 *              pixm  (8 bpp filling mask)
 *              connectivity  (4 or 8)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is an in-place filling operation on the seed, pixs,
 *          where the clipping mask is always above or at the level
 *          of the seed as it is filled.
 *      (2) For details of the operation, see the description in
 *          seedfillGrayLowSimple() and the code there.
 *      (3) As an example of use, see the description in pixHDome().
 *          There, the seed is an image where each pixel is a fixed
 *          amount smaller than the corresponding mask pixel.
 *      (4) Reference paper :
 *            L. Vincent, Morphological grayscale reconstruction in image
 *            analysis: applications and efficient algorithms, IEEE Transactions
 *            on  Image Processing, vol. 2, no. 2, pp. 176-201, 1993.
 */
l_int32
pixSeedfillGraySimple(PIX     *pixs,
                      PIX     *pixm,
                      l_int32  connectivity)
{
l_int32    i, h, w, wpls, wplm, boolval;
l_uint32  *datas, *datam;
PIX       *pixt;

    PROCNAME("pixSeedfillGraySimple");

    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (!pixm || pixGetDepth(pixm) != 8)
        return ERROR_INT("pixm not defined or not 8 bpp", procName, 1);
    if (connectivity != 4 && connectivity != 8)
        return ERROR_INT("connectivity not in {4,8}", procName, 1);

        /* Make sure the sizes of seed and mask images are the same */
    if (pixSizesEqual(pixs, pixm) == 0)
        return ERROR_INT("pixs and pixm sizes differ", procName, 1);

        /* This is used to test for completion */
    if ((pixt = pixCreateTemplate(pixs)) == NULL)
        return ERROR_INT("pixt not made", procName, 1);

    datas = pixGetData(pixs);
    datam = pixGetData(pixm);
    wpls = pixGetWpl(pixs);
    wplm = pixGetWpl(pixm);
    pixGetDimensions(pixs, &w, &h, NULL);
    for (i = 0; i < MAX_ITERS; i++) {
        pixCopy(pixt, pixs);
        seedfillGrayLowSimple(datas, w, h, wpls, datam, wplm, connectivity);
        pixEqual(pixs, pixt, &boolval);
        if (boolval == 1) {
#if DEBUG_PRINT_ITERS
            L_INFO("Gray seed fill converged: %d iters\n", procName, i + 1);
#endif  /* DEBUG_PRINT_ITERS */
            break;
        }
    }

    pixDestroy(&pixt);
    return 0;
}


/*!
 *  pixSeedfillGrayInvSimple()
 *
 *      Input:  pixs  (8 bpp seed; filled in place)
 *              pixm  (8 bpp filling mask)
 *              connectivity  (4 or 8)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is an in-place filling operation on the seed, pixs,
 *          where the clipping mask is always below or at the level
 *          of the seed as it is filled.  Think of filling up a basin
 *          to a particular level, given by the maximum seed value
 *          in the basin.  Outside the filled region, the mask
 *          is above the filling level.
 *      (2) Contrast this with pixSeedfillGraySimple(), where the clipping mask
 *          is always above or at the level of the fill.  An example
 *          of its use is the hdome fill, where the seed is an image
 *          where each pixel is a fixed amount smaller than the
 *          corresponding mask pixel.
 */
l_int32
pixSeedfillGrayInvSimple(PIX     *pixs,
                         PIX     *pixm,
                         l_int32  connectivity)
{
l_int32    i, h, w, wpls, wplm, boolval;
l_uint32  *datas, *datam;
PIX       *pixt;

    PROCNAME("pixSeedfillGrayInvSimple");

    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (!pixm || pixGetDepth(pixm) != 8)
        return ERROR_INT("pixm not defined or not 8 bpp", procName, 1);
    if (connectivity != 4 && connectivity != 8)
        return ERROR_INT("connectivity not in {4,8}", procName, 1);

        /* Make sure the sizes of seed and mask images are the same */
    if (pixSizesEqual(pixs, pixm) == 0)
        return ERROR_INT("pixs and pixm sizes differ", procName, 1);

        /* This is used to test for completion */
    if ((pixt = pixCreateTemplate(pixs)) == NULL)
        return ERROR_INT("pixt not made", procName, 1);

    datas = pixGetData(pixs);
    datam = pixGetData(pixm);
    wpls = pixGetWpl(pixs);
    wplm = pixGetWpl(pixm);
    pixGetDimensions(pixs, &w, &h, NULL);
    for (i = 0; i < MAX_ITERS; i++) {
        pixCopy(pixt, pixs);
        seedfillGrayInvLowSimple(datas, w, h, wpls, datam, wplm, connectivity);
        pixEqual(pixs, pixt, &boolval);
        if (boolval == 1) {
#if DEBUG_PRINT_ITERS
            L_INFO("Gray seed fill converged: %d iters\n", procName, i + 1);
#endif  /* DEBUG_PRINT_ITERS */
            break;
        }
    }

    pixDestroy(&pixt);
    return 0;
}


/*-----------------------------------------------------------------------*
 *                         Gray seedfill variations                      *
 *-----------------------------------------------------------------------*/
/*!
 *  pixSeedfillGrayBasin()
 *
 *      Input:  pixb  (binary mask giving seed locations)
 *              pixm  (8 bpp basin-type filling mask)
 *              delta (amount of seed value above mask)
 *              connectivity  (4 or 8)
 *      Return: pixd (filled seed) if OK, null on error
 *
 *  Notes:
 *      (1) This fills from a seed within basins defined by a filling mask.
 *          The seed value(s) are greater than the corresponding
 *          filling mask value, and the result has the bottoms of
 *          the basins raised by the initial seed value.
 *      (2) The seed has value 255 except where pixb has fg (1), which
 *          are the seed 'locations'.  At the seed locations, the seed
 *          value is the corresponding value of the mask pixel in pixm
 *          plus @delta.  If @delta == 0, we return a copy of pixm.
 *      (3) The actual filling is done using the standard grayscale filling
 *          operation on the inverse of the mask and using the inverse
 *          of the seed image.  After filling, we return the inverse of
 *          the filled seed.
 *      (4) As an example of use: pixm can describe a grayscale image
 *          of text, where the (dark) text pixels are basins of
 *          low values; pixb can identify the local minima in pixm (say, at
 *          the bottom of the basins); and delta is the amount that we wish
 *          to raise (lighten) the basins.  We construct the seed
 *          (a.k.a marker) image from pixb, pixm and @delta.
 */
PIX *
pixSeedfillGrayBasin(PIX     *pixb,
                     PIX     *pixm,
                     l_int32  delta,
                     l_int32  connectivity)
{
PIX  *pixbi, *pixmi, *pixsd;

    PROCNAME("pixSeedfillGrayBasin");

    if (!pixb || pixGetDepth(pixb) != 1)
        return (PIX *)ERROR_PTR("pixb undefined or not 1 bpp", procName, NULL);
    if (!pixm || pixGetDepth(pixm) != 8)
        return (PIX *)ERROR_PTR("pixm undefined or not 8 bpp", procName, NULL);
    if (connectivity != 4 && connectivity != 8)
        return (PIX *)ERROR_PTR("connectivity not in {4,8}", procName, NULL);

    if (delta <= 0) {
        L_WARNING("delta <= 0; returning a copy of pixm\n", procName);
        return pixCopy(NULL, pixm);
    }

        /* Add delta to every pixel in pixm */
    pixsd = pixCopy(NULL, pixm);
    pixAddConstantGray(pixsd, delta);

        /* Prepare the seed.  Write 255 in all pixels of
         * ([pixm] + delta) where pixb is 0. */
    pixbi = pixInvert(NULL, pixb);
    pixSetMasked(pixsd, pixbi, 255);

        /* Fill the inverse seed, using the inverse clipping mask */
    pixmi = pixInvert(NULL, pixm);
    pixInvert(pixsd, pixsd);
    pixSeedfillGray(pixsd, pixmi, connectivity);

        /* Re-invert the filled seed */
    pixInvert(pixsd, pixsd);

    pixDestroy(&pixbi);
    pixDestroy(&pixmi);
    return pixsd;
}


/*-----------------------------------------------------------------------*
 *                   Vincent's Distance Function method                  *
 *-----------------------------------------------------------------------*/
/*!
 *  pixDistanceFunction()
 *
 *      Input:  pixs  (1 bpp source)
 *              connectivity  (4 or 8)
 *              outdepth (8 or 16 bits for pixd)
 *              boundcond (L_BOUNDARY_BG, L_BOUNDARY_FG)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This computes the distance of each pixel from the nearest
 *          background pixel.  All bg pixels therefore have a distance of 0,
 *          and the fg pixel distances increase linearly from 1 at the
 *          boundary.  It can also be used to compute the distance of
 *          each pixel from the nearest fg pixel, by inverting the input
 *          image before calling this function.  Then all fg pixels have
 *          a distance 0 and the bg pixel distances increase linearly
 *          from 1 at the boundary.
 *      (2) The algorithm, described in Leptonica on the page on seed
 *          filling and connected components, is due to Luc Vincent.
 *          In brief, we generate an 8 or 16 bpp image, initialized
 *          with the fg pixels of the input pix set to 1 and the
 *          1-boundary pixels (i.e., the boundary pixels of width 1 on
 *          the four sides set as either:
 *            * L_BOUNDARY_BG: 0
 *            * L_BOUNDARY_FG:  max
 *          where max = 0xff for 8 bpp and 0xffff for 16 bpp.
 *          Then do raster/anti-raster sweeps over all pixels interior
 *          to the 1-boundary, where the value of each new pixel is
 *          taken to be 1 more than the minimum of the previously-seen
 *          connected pixels (using either 4 or 8 connectivity).
 *          Finally, set the 1-boundary pixels using the mirrored method;
 *          this removes the max values there.
 *      (3) Using L_BOUNDARY_BG clamps the distance to 0 at the
 *          boundary.  Using L_BOUNDARY_FG allows the distance
 *          at the image boundary to "float".
 *      (4) For 4-connected, one could initialize only the left and top
 *          1-boundary pixels, and go all the way to the right
 *          and bottom; then coming back reset left and top.  But we
 *          instead use a method that works for both 4- and 8-connected.
 */
PIX *
pixDistanceFunction(PIX     *pixs,
                    l_int32  connectivity,
                    l_int32  outdepth,
                    l_int32  boundcond)
{
l_int32    w, h, wpld;
l_uint32  *datad;
PIX       *pixd;

    PROCNAME("pixDistanceFunction");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("!pixs or pixs not 1 bpp", procName, NULL);
    if (connectivity != 4 && connectivity != 8)
        return (PIX *)ERROR_PTR("connectivity not 4 or 8", procName, NULL);
    if (outdepth != 8 && outdepth != 16)
        return (PIX *)ERROR_PTR("outdepth not 8 or 16 bpp", procName, NULL);
    if (boundcond != L_BOUNDARY_BG && boundcond != L_BOUNDARY_FG)
        return (PIX *)ERROR_PTR("invalid boundcond", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    if ((pixd = pixCreate(w, h, outdepth)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

        /* Initialize the fg pixels to 1 and the bg pixels to 0 */
    pixSetMasked(pixd, pixs, 1);

    if (boundcond == L_BOUNDARY_BG) {
        distanceFunctionLow(datad, w, h, outdepth, wpld, connectivity);
    } else {  /* L_BOUNDARY_FG: set boundary pixels to max val */
        pixRasterop(pixd, 0, 0, w, 1, PIX_SET, NULL, 0, 0);   /* top */
        pixRasterop(pixd, 0, h - 1, w, 1, PIX_SET, NULL, 0, 0);   /* bot */
        pixRasterop(pixd, 0, 0, 1, h, PIX_SET, NULL, 0, 0);   /* left */
        pixRasterop(pixd, w - 1, 0, 1, h, PIX_SET, NULL, 0, 0);   /* right */

        distanceFunctionLow(datad, w, h, outdepth, wpld, connectivity);

            /* Set each boundary pixel equal to the pixel next to it */
        pixSetMirroredBorder(pixd, 1, 1, 1, 1);
    }

    return pixd;
}


/*-----------------------------------------------------------------------*
 *                Seed spread (based on distance function)               *
 *-----------------------------------------------------------------------*/
/*!
 *  pixSeedspread()
 *
 *      Input:  pixs  (8 bpp source)
 *              connectivity  (4 or 8)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) The raster/anti-raster method for implementing this filling
 *          operation was suggested by Ray Smith.
 *      (2) This takes an arbitrary set of nonzero pixels in pixs, which
 *          can be sparse, and spreads (extrapolates) the values to
 *          fill all the pixels in pixd with the nonzero value it is
 *          closest to in pixs.  This is similar (though not completely
 *          equivalent) to doing a Voronoi tiling of the image, with a
 *          tile surrounding each pixel that has a nonzero value.
 *          All pixels within a tile are then closer to its "central"
 *          pixel than to any others.  Then assign the value of the
 *          "central" pixel to each pixel in the tile.
 *      (3) This is implemented by computing a distance function in parallel
 *          with the fill.  The distance function uses free boundary
 *          conditions (assumed maxval outside), and it controls the
 *          propagation of the pixels in pixd away from the nonzero
 *          (seed) values.  This is done in 2 traversals (raster/antiraster).
 *          In the raster direction, whenever the distance function
 *          is nonzero, the spread pixel takes on the value of its
 *          predecessor that has the minimum distance value.  In the
 *          antiraster direction, whenever the distance function is nonzero
 *          and its value is replaced by a smaller value, the spread
 *          pixel takes the value of the predecessor with the minimum
 *          distance value.
 *      (4) At boundaries where a pixel is equidistant from two
 *          nearest nonzero (seed) pixels, the decision of which value
 *          to use is arbitrary (greedy in search for minimum distance).
 *          This can give rise to strange-looking results, particularly
 *          for 4-connectivity where the L1 distance is computed from
 *          steps in N,S,E and W directions (no diagonals).
 */
PIX *
pixSeedspread(PIX     *pixs,
              l_int32  connectivity)
{
l_int32    w, h, wplt, wplg;
l_uint32  *datat, *datag;
PIX       *pixm, *pixt, *pixg, *pixd;

    PROCNAME("pixSeedspread");

    if (!pixs || pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("!pixs or pixs not 8 bpp", procName, NULL);
    if (connectivity != 4 && connectivity != 8)
        return (PIX *)ERROR_PTR("connectivity not 4 or 8", procName, NULL);

        /* Add a 4 byte border to pixs.  This simplifies the computation. */
    pixg = pixAddBorder(pixs, 4, 0);
    pixGetDimensions(pixg, &w, &h, NULL);

        /* Initialize distance function pixt.  Threshold pixs to get
         * a 0 at the seed points where the pixs pixel is nonzero, and
         * a 1 at all points that need to be filled.  Use this as a
         * mask to set a 1 in pixt at all non-seed points.  Also, set all
         * pixt pixels in an interior boundary of width 1 to the
         * maximum value.   For debugging, to view the distance function,
         * use pixConvert16To8(pixt, 0) on small images.  */
    pixm = pixThresholdToBinary(pixg, 1);
    pixt = pixCreate(w, h, 16);
    pixSetMasked(pixt, pixm, 1);
    pixRasterop(pixt, 0, 0, w, 1, PIX_SET, NULL, 0, 0);   /* top */
    pixRasterop(pixt, 0, h - 1, w, 1, PIX_SET, NULL, 0, 0);   /* bot */
    pixRasterop(pixt, 0, 0, 1, h, PIX_SET, NULL, 0, 0);   /* left */
    pixRasterop(pixt, w - 1, 0, 1, h, PIX_SET, NULL, 0, 0);   /* right */
    datat = pixGetData(pixt);
    wplt = pixGetWpl(pixt);

        /* Do the interpolation and remove the border. */
    datag = pixGetData(pixg);
    wplg = pixGetWpl(pixg);
    seedspreadLow(datag, w, h, wplg, datat, wplt, connectivity);
    pixd = pixRemoveBorder(pixg, 4);

    pixDestroy(&pixm);
    pixDestroy(&pixg);
    pixDestroy(&pixt);
    return pixd;
}



/*-----------------------------------------------------------------------*
 *                              Local extrema                            *
 *-----------------------------------------------------------------------*/
/*!
 *  pixLocalExtrema()
 *
 *      Input:  pixs  (8 bpp)
 *              maxmin (max allowed for the min in a 3x3 neighborhood;
 *                      use 0 for default which is to have no upper bound)
 *              minmax (min allowed for the max in a 3x3 neighborhood;
 *                      use 0 for default which is to have no lower bound)
 *              &ppixmin (<optional return> mask of local minima)
 *              &ppixmax (<optional return> mask of local maxima)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This gives the actual local minima and maxima.
 *          A local minimum is a pixel whose surrounding pixels all
 *          have values at least as large, and likewise for a local
 *          maximum.  For the local minima, @maxmin is the upper
 *          bound for the value of pixs.  Likewise, for the local maxima,
 *          @minmax is the lower bound for the value of pixs.
 *      (2) The minima are found by starting with the erosion-and-equality
 *          approach of pixSelectedLocalExtrema().  This is followed
 *          by a qualification step, where each c.c. in the resulting
 *          minimum mask is extracted, the pixels bordering it are
 *          located, and they are queried.  If all of those pixels
 *          are larger than the value of that minimum, it is a true
 *          minimum and its c.c. is saved; otherwise the c.c. is
 *          rejected.  Note that if a bordering pixel has the
 *          same value as the minimum, it must then have a
 *          neighbor that is smaller, so the component is not a
 *          true minimum.
 *      (3) The maxima are found by inverting the image and looking
 *          for the minima there.
 *      (4) The generated masks can be used as markers for
 *          further operations.
 */
l_int32
pixLocalExtrema(PIX     *pixs,
                l_int32  maxmin,
                l_int32  minmax,
                PIX    **ppixmin,
                PIX    **ppixmax)
{
PIX  *pixmin, *pixmax, *pixt1, *pixt2;

    PROCNAME("pixLocalExtrema");

    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (!ppixmin && !ppixmax)
        return ERROR_INT("neither &pixmin, &pixmax are defined", procName, 1);
    if (maxmin <= 0) maxmin = 254;
    if (minmax <= 0) minmax = 1;

    if (ppixmin) {
        pixt1 = pixErodeGray(pixs, 3, 3);
        pixmin = pixFindEqualValues(pixs, pixt1);
        pixDestroy(&pixt1);
        pixQualifyLocalMinima(pixs, pixmin, maxmin);
        *ppixmin = pixmin;
    }

    if (ppixmax) {
        pixt1 = pixInvert(NULL, pixs);
        pixt2 = pixErodeGray(pixt1, 3, 3);
        pixmax = pixFindEqualValues(pixt1, pixt2);
        pixDestroy(&pixt2);
        pixQualifyLocalMinima(pixt1, pixmax, 255 - minmax);
        *ppixmax = pixmax;
        pixDestroy(&pixt1);
    }

    return 0;
}


/*!
 *  pixQualifyLocalMinima()
 *
 *      Input:  pixs  (8 bpp image from which pixm has been extracted)
 *              pixm  (1 bpp mask of values equal to min in 3x3 neighborhood)
 *              maxval (max allowed for the min in a 3x3 neighborhood;
 *                      use 0 for default which is to have no upper bound)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This function acts in-place to remove all c.c. in pixm
 *          that are not true local minima in pixs.  As seen in
 *          pixLocalExtrema(), the input pixm are found by selecting those
 *          pixels of pixs whose values do not change with a 3x3
 *          grayscale erosion.  Here, we require that for each c.c.
 *          in pixm, all pixels in pixs that correspond to the exterior
 *          boundary pixels of the c.c. have values that are greater
 *          than the value within the c.c.
 *      (2) The maximum allowed value for each local minimum can be
 *          bounded with @maxval.  Use 0 for default, which is to have
 *          no upper bound (equivalent to maxval == 254).
 */
static l_int32
pixQualifyLocalMinima(PIX     *pixs,
                      PIX     *pixm,
                      l_int32  maxval)
{
l_int32    n, i, j, k, x, y, w, h, xc, yc, wc, hc, xon, yon;
l_int32    vals, wpls, wplc, ismin;
l_uint32   val;
l_uint32  *datas, *datac, *lines, *linec;
BOXA      *boxa;
PIX       *pixt1, *pixt2, *pixc;
PIXA      *pixa;

    PROCNAME("pixQualifyLocalMinima");

    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (!pixm || pixGetDepth(pixm) != 1)
        return ERROR_INT("pixm not defined or not 1 bpp", procName, 1);
    if (maxval <= 0) maxval = 254;

    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    boxa = pixConnComp(pixm, &pixa, 8);
    n = pixaGetCount(pixa);
    for (k = 0; k < n; k++) {
        boxaGetBoxGeometry(boxa, k, &xc, &yc, &wc, &hc);
        pixt1 = pixaGetPix(pixa, k, L_COPY);
        pixt2 = pixAddBorder(pixt1, 1, 0);
        pixc = pixDilateBrick(NULL, pixt2, 3, 3);
        pixXor(pixc, pixc, pixt2);  /* exterior boundary pixels */
        datac = pixGetData(pixc);
        wplc = pixGetWpl(pixc);
        nextOnPixelInRaster(pixt1, 0, 0, &xon, &yon);
        pixGetPixel(pixs, xc + xon, yc + yon, &val);
        if (val > maxval) {  /* too large; erase */
            pixRasterop(pixm, xc, yc, wc, hc, PIX_XOR, pixt1, 0, 0);
            pixDestroy(&pixt1);
            pixDestroy(&pixt2);
            pixDestroy(&pixc);
            continue;
        }
        ismin = TRUE;

            /* Check all values in pixs that correspond to the exterior
             * boundary pixels of the c.c. in pixm.  Verify that the
             * value in the c.c. is always less. */
        for (i = 0, y = yc - 1; i < hc + 2 && y >= 0 && y < h; i++, y++) {
            lines = datas + y * wpls;
            linec = datac + i * wplc;
            for (j = 0, x = xc - 1; j < wc + 2 && x >= 0 && x < w; j++, x++) {
                if (GET_DATA_BIT(linec, j)) {
                    vals = GET_DATA_BYTE(lines, x);
                    if (vals <= val) {  /* not a minimum! */
                        ismin = FALSE;
                        break;
                    }
                }
            }
            if (!ismin)
                break;
        }
        if (!ismin)  /* erase it */
            pixRasterop(pixm, xc, yc, wc, hc, PIX_XOR, pixt1, 0, 0);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixc);
    }

    boxaDestroy(&boxa);
    pixaDestroy(&pixa);
    return 0;
}


/*!
 *  pixSelectedLocalExtrema()
 *
 *      Input:  pixs  (8 bpp)
 *              mindist (-1 for keeping all pixels; >= 0 specifies distance)
 *              &ppixmin (<return> mask of local minima)
 *              &ppixmax (<return> mask of local maxima)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This selects those local 3x3 minima that are at least a
 *          specified distance from the nearest local 3x3 maxima, and v.v.
 *          for the selected set of local 3x3 maxima.
 *          The local 3x3 minima is the set of pixels whose value equals
 *          the value after a 3x3 brick erosion, and the local 3x3 maxima
 *          is the set of pixels whose value equals the value after
 *          a 3x3 brick dilation.
 *      (2) mindist is the minimum distance allowed between
 *          local 3x3 minima and local 3x3 maxima, in an 8-connected sense.
 *          mindist == 1 keeps all pixels found in step 1.
 *          mindist == 0 removes all pixels from each mask that are
 *          both a local 3x3 minimum and a local 3x3 maximum.
 *          mindist == 1 removes any local 3x3 minimum pixel that touches a
 *          local 3x3 maximum pixel, and likewise for the local maxima.
 *          To make the decision, visualize each local 3x3 minimum pixel
 *          as being surrounded by a square of size (2 * mindist + 1)
 *          on each side, such that no local 3x3 maximum pixel is within
 *          that square; and v.v.
 *      (3) The generated masks can be used as markers for further operations.
 */
l_int32
pixSelectedLocalExtrema(PIX     *pixs,
                        l_int32  mindist,
                        PIX    **ppixmin,
                        PIX    **ppixmax)
{
PIX  *pixmin, *pixmax, *pixt, *pixtmin, *pixtmax;

    PROCNAME("pixSelectedLocalExtrema");

    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (!ppixmin || !ppixmax)
        return ERROR_INT("&pixmin and &pixmax not both defined", procName, 1);

    pixt = pixErodeGray(pixs, 3, 3);
    pixmin = pixFindEqualValues(pixs, pixt);
    pixDestroy(&pixt);
    pixt = pixDilateGray(pixs, 3, 3);
    pixmax = pixFindEqualValues(pixs, pixt);
    pixDestroy(&pixt);

        /* Remove all points that are within the prescribed distance
         * from each other. */
    if (mindist < 0) {  /* remove no points */
        *ppixmin = pixmin;
        *ppixmax = pixmax;
    } else if (mindist == 0) {  /* remove points belonging to both sets */
        pixt = pixAnd(NULL, pixmin, pixmax);
        *ppixmin = pixSubtract(pixmin, pixmin, pixt);
        *ppixmax = pixSubtract(pixmax, pixmax, pixt);
        pixDestroy(&pixt);
    } else {
        pixtmin = pixDilateBrick(NULL, pixmin,
                                 2 * mindist + 1, 2 * mindist + 1);
        pixtmax = pixDilateBrick(NULL, pixmax,
                                 2 * mindist + 1, 2 * mindist + 1);
        *ppixmin = pixSubtract(pixmin, pixmin, pixtmax);
        *ppixmax = pixSubtract(pixmax, pixmax, pixtmin);
        pixDestroy(&pixtmin);
        pixDestroy(&pixtmax);
    }
    return 0;
}


/*!
 *  pixFindEqualValues()
 *
 *      Input:  pixs1 (8 bpp)
 *              pixs2 (8 bpp)
 *      Return: pixd (1 bpp mask), or null on error
 *
 *  Notes:
 *      (1) The two images are aligned at the UL corner, and the returned
 *          image has ON pixels where the pixels in pixs1 and pixs2
 *          have equal values.
 */
PIX *
pixFindEqualValues(PIX  *pixs1,
                   PIX  *pixs2)
{
l_int32    w1, h1, w2, h2, w, h;
l_int32    i, j, val1, val2, wpls1, wpls2, wpld;
l_uint32  *datas1, *datas2, *datad, *lines1, *lines2, *lined;
PIX       *pixd;

    PROCNAME("pixFindEqualValues");

    if (!pixs1 || pixGetDepth(pixs1) != 8)
        return (PIX *)ERROR_PTR("pixs1 undefined or not 8 bpp", procName, NULL);
    if (!pixs2 || pixGetDepth(pixs2) != 8)
        return (PIX *)ERROR_PTR("pixs2 undefined or not 8 bpp", procName, NULL);
    pixGetDimensions(pixs1, &w1, &h1, NULL);
    pixGetDimensions(pixs2, &w2, &h2, NULL);
    w = L_MIN(w1, w2);
    h = L_MIN(h1, h2);
    pixd = pixCreate(w, h, 1);
    datas1 = pixGetData(pixs1);
    datas2 = pixGetData(pixs2);
    datad = pixGetData(pixd);
    wpls1 = pixGetWpl(pixs1);
    wpls2 = pixGetWpl(pixs2);
    wpld = pixGetWpl(pixd);

    for (i = 0; i < h; i++) {
        lines1 = datas1 + i * wpls1;
        lines2 = datas2 + i * wpls2;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            val1 = GET_DATA_BYTE(lines1, j);
            val2 = GET_DATA_BYTE(lines2, j);
            if (val1 == val2)
                SET_DATA_BIT(lined, j);
        }
    }

    return pixd;
}


/*-----------------------------------------------------------------------*
 *             Selection of minima in mask connected components          *
 *-----------------------------------------------------------------------*/
/*!
 *  pixSelectMinInConnComp()
 *
 *      Input:  pixs (8 bpp)
 *              pixm (1 bpp)
 *              &pta (<return> pta of min pixel locations)
 *              &nav (<optional return> numa of minima values)
 *      Return: 0 if OK, 1 on error.
 *
 *  Notes:
 *      (1) For each 8 connected component in pixm, this finds
 *          a pixel in pixs that has the lowest value, and saves
 *          it in a Pta.  If several pixels in pixs have the same
 *          minimum value, it picks the first one found.
 *      (2) For a mask pixm of true local minima, all pixels in each
 *          connected component have the same value in pixs, so it is
 *          fastest to select one of them using a special seedfill
 *          operation.  Not yet implemented.
 */
l_int32
pixSelectMinInConnComp(PIX    *pixs,
                       PIX    *pixm,
                       PTA   **ppta,
                       NUMA  **pnav)
{
l_int32    bx, by, bw, bh, i, j, c, n;
l_int32    xs, ys, minx, miny, wpls, wplt, val, minval;
l_uint32  *datas, *datat, *lines, *linet;
BOXA      *boxa;
NUMA      *nav;
PIX       *pixt, *pixs2, *pixm2;
PIXA      *pixa;
PTA       *pta;

    PROCNAME("pixSelectMinInConnComp");

    if (!ppta)
        return ERROR_INT("&pta not defined", procName, 1);
    *ppta = NULL;
    if (pnav) *pnav = NULL;
    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs undefined or not 8 bpp", procName, 1);
    if (!pixm || pixGetDepth(pixm) != 1)
        return ERROR_INT("pixm undefined or not 1 bpp", procName, 1);

        /* Crop to the min size if necessary */
    if (pixCropToMatch(pixs, pixm, &pixs2, &pixm2)) {
        pixDestroy(&pixs2);
        pixDestroy(&pixm2);
        return ERROR_INT("cropping failure", procName, 1);
    }

        /* Find value and location of min value pixel in each component */
    boxa = pixConnComp(pixm2, &pixa, 8);
    n = boxaGetCount(boxa);
    pta = ptaCreate(n);
    *ppta = pta;
    nav = numaCreate(n);
    datas = pixGetData(pixs2);
    wpls = pixGetWpl(pixs2);
    for (c = 0; c < n; c++) {
        pixt = pixaGetPix(pixa, c, L_CLONE);
        boxaGetBoxGeometry(boxa, c, &bx, &by, &bw, &bh);
        if (bw == 1 && bh == 1) {
            ptaAddPt(pta, bx, by);
            numaAddNumber(nav, GET_DATA_BYTE(datas + by * wpls, bx));
            pixDestroy(&pixt);
            continue;
        }
        datat = pixGetData(pixt);
        wplt = pixGetWpl(pixt);
        minx = miny = 1000000;
        minval = 256;
        for (i = 0; i < bh; i++) {
            ys = by + i;
            lines = datas + ys * wpls;
            linet = datat + i * wplt;
            for (j = 0; j < bw; j++) {
                xs = bx + j;
                if (GET_DATA_BIT(linet, j)) {
                    val = GET_DATA_BYTE(lines, xs);
                    if (val < minval) {
                        minval = val;
                        minx = xs;
                        miny = ys;
                    }
                }
            }
        }
        ptaAddPt(pta, minx, miny);
        numaAddNumber(nav, GET_DATA_BYTE(datas + miny * wpls, minx));
        pixDestroy(&pixt);
    }

    boxaDestroy(&boxa);
    pixaDestroy(&pixa);
    if (pnav)
        *pnav = nav;
    else
        numaDestroy(&nav);
    pixDestroy(&pixs2);
    pixDestroy(&pixm2);
    return 0;
}


/*-----------------------------------------------------------------------*
 *            Removal of seeded connected components from a mask         *
 *-----------------------------------------------------------------------*/
/*!
 *  pixRemoveSeededComponents()
 *
 *      Input:  pixd  (<optional>; this can be null or equal to pixm; 1 bpp)
 *              pixs  (1 bpp seed)
 *              pixm  (1 bpp filling mask)
 *              connectivity  (4 or 8)
 *              bordersize (amount of border clearing)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This removes each component in pixm for which there is
 *          at least one seed in pixs.  If pixd == NULL, this returns
 *          the result in a new pixd.  Otherwise, it is an in-place
 *          operation on pixm.  In no situation is pixs altered,
 *          because we do the filling with a copy of pixs.
 *      (2) If bordersize > 0, it also clears all pixels within a
 *          distance @bordersize of the edge of pixd.  This is here
 *          because pixLocalExtrema() typically finds local minima
 *          at the border.  Use @bordersize >= 2 to remove these.
 */
PIX *
pixRemoveSeededComponents(PIX     *pixd,
                          PIX     *pixs,
                          PIX     *pixm,
                          l_int32  connectivity,
                          l_int32  bordersize)
{
PIX  *pixt;

    PROCNAME("pixRemoveSeededComponents");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, pixd);
    if (!pixm || pixGetDepth(pixm) != 1)
        return (PIX *)ERROR_PTR("pixm undefined or not 1 bpp", procName, pixd);
    if (pixd && pixd != pixm)
        return (PIX *)ERROR_PTR("operation not inplace", procName, pixd);

    pixt = pixCopy(NULL, pixs);
    pixSeedfillBinary(pixt, pixt, pixm, connectivity);
    pixd = pixXor(pixd, pixm, pixt);
    if (bordersize > 0)
        pixSetOrClearBorder(pixd, bordersize, bordersize, bordersize,
                            bordersize, PIX_CLR);
    pixDestroy(&pixt);
    return pixd;
}
