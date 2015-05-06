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
 *  morph.c
 *
 *     Generic binary morphological ops implemented with rasterop
 *         PIX     *pixDilate()
 *         PIX     *pixErode()
 *         PIX     *pixHMT()
 *         PIX     *pixOpen()
 *         PIX     *pixClose()
 *         PIX     *pixCloseSafe()
 *         PIX     *pixOpenGeneralized()
 *         PIX     *pixCloseGeneralized()
 *
 *     Binary morphological (raster) ops with brick Sels
 *         PIX     *pixDilateBrick()
 *         PIX     *pixErodeBrick()
 *         PIX     *pixOpenBrick()
 *         PIX     *pixCloseBrick()
 *         PIX     *pixCloseSafeBrick()
 *
 *     Binary composed morphological (raster) ops with brick Sels
 *         l_int32  selectComposableSels()
 *         l_int32  selectComposableSizes()
 *         PIX     *pixDilateCompBrick()
 *         PIX     *pixErodeCompBrick()
 *         PIX     *pixOpenCompBrick()
 *         PIX     *pixCloseCompBrick()
 *         PIX     *pixCloseSafeCompBrick()
 *
 *     Functions associated with boundary conditions
 *         void     resetMorphBoundaryCondition()
 *         l_int32  getMorphBorderPixelColor()
 *
 *     Static helpers for arg processing
 *         static PIX     *processMorphArgs1()
 *         static PIX     *processMorphArgs2()
 *
 *  You are provided with many simple ways to do binary morphology.
 *  In particular, if you are using brick Sels, there are six
 *  convenient methods, all specially tailored for separable operations
 *  on brick Sels.  A "brick" Sel is a Sel that is a rectangle
 *  of solid SEL_HITs with the origin at or near the center.
 *  Note that a brick Sel can have one dimension of size 1.
 *  This is very common.  All the brick Sel operations are
 *  separable, meaning the operation is done first in the horizontal
 *  direction and then in the vertical direction.  If one of the
 *  dimensions is 1, this is a special case where the operation is
 *  only performed in the other direction.
 *
 *  These six brick Sel methods are enumerated as follows:
 *
 *  (1) Brick Sels: pix*Brick(), where * = {Dilate, Erode, Open, Close}.
 *      These are separable rasterop implementations.  The Sels are
 *      automatically generated, used, and destroyed at the end.
 *      You can get the result as a new Pix, in-place back into the src Pix,
 *      or written to another existing Pix.
 *
 *  (2) Brick Sels: pix*CompBrick(), where * = {Dilate, Erode, Open, Close}.
 *      These are separable, 2-way composite, rasterop implementations.
 *      The Sels are automatically generated, used, and destroyed at the end.
 *      You can get the result as a new Pix, in-place back into the src Pix,
 *      or written to another existing Pix.  For large Sels, these are
 *      considerably faster than the corresponding pix*Brick() functions.
 *      N.B.:  The size of the Sels that are actually used are typically
 *      close to, but not exactly equal to, the size input to the function.
 *
 *  (3) Brick Sels: pix*BrickDwa(), where * = {Dilate, Erode, Open, Close}.
 *      These are separable dwa (destination word accumulation)
 *      implementations.  They use auto-gen'd dwa code.  You can get
 *      the result as a new Pix, in-place back into the src Pix,
 *      or written to another existing Pix.  This is typically
 *      about 3x faster than the analogous rasterop pix*Brick()
 *      function, but it has the limitation that the Sel size must
 *      be less than 63.  This is pre-set to work on a number
 *      of pre-generated Sels.  If you want to use other Sels, the
 *      code can be auto-gen'd for them; see the instructions in morphdwa.c.
 *
 *  (4) Same as (1), but you run it through pixMorphSequence(), with
 *      the sequence string either compiled in or generated using sprintf.
 *      All intermediate images and Sels are created, used and destroyed.
 *      You always get the result as a new Pix.  For example, you can
 *      specify a separable 11 x 17 brick opening as "o11.17",
 *      or you can specify the horizontal and vertical operations
 *      explicitly as "o11.1 + o1.11".  See morphseq.c for details.
 *
 *  (5) Same as (2), but you run it through pixMorphCompSequence(), with
 *      the sequence string either compiled in or generated using sprintf.
 *      All intermediate images and Sels are created, used and destroyed.
 *      You always get the result as a new Pix.  See morphseq.c for details.
 *
 *  (6) Same as (3), but you run it through pixMorphSequenceDwa(), with
 *      the sequence string either compiled in or generated using sprintf.
 *      All intermediate images and Sels are created, used and destroyed.
 *      You always get the result as a new Pix.  See morphseq.c for details.
 *
 *  If you are using Sels that are not bricks, you have two choices:
 *      (a) simplest: use the basic rasterop implementations (pixDilate(), ...)
 *      (b) fastest: generate the destination word accumumlation (dwa)
 *          code for your Sels and compile it with the library.
 *
 *      For an example, see flipdetect.c, which gives implementations
 *      using hit-miss Sels with both the rasterop and dwa versions.
 *      For the latter, the dwa code resides in fliphmtgen.c, and it
 *      was generated by prog/flipselgen.c.  Both the rasterop and dwa
 *      implementations are tested by prog/fliptest.c.
 *
 *  A global constant MORPH_BC is used to set the boundary conditions
 *  for rasterop-based binary morphology.  MORPH_BC, in morph.c,
 *  is set by default to ASYMMETRIC_MORPH_BC for a non-symmetric
 *  convention for boundary pixels in dilation and erosion:
 *      All pixels outside the image are assumed to be OFF
 *      for both dilation and erosion.
 *  To use a symmetric definition, see comments in pixErode()
 *  and reset MORPH_BC to SYMMETRIC_MORPH_BC, using
 *  resetMorphBoundaryCondition().
 *
 *  Boundary artifacts are possible in closing when the non-symmetric
 *  boundary conditions are used, because foreground pixels very close
 *  to the edge can be removed.  This can be avoided by using either
 *  the symmetric boundary conditions or the function pixCloseSafe(),
 *  which adds a border before the operation and removes it afterwards.
 *
 *  The hit-miss transform (HMT) is the bit-and of 2 erosions:
 *     (erosion of the src by the hits)  &  (erosion of the bit-inverted
 *                                           src by the misses)
 *
 *  The 'generalized opening' is an HMT followed by a dilation that uses
 *  only the hits of the hit-miss Sel.
 *  The 'generalized closing' is a dilation (again, with the hits
 *  of a hit-miss Sel), followed by the HMT.
 *  Both of these 'generalized' functions are idempotent.
 *
 *  These functions are extensively tested in prog/binmorph1_reg.c,
 *  prog/binmorph2_reg.c, and prog/binmorph3_reg.c.
 */

#include <math.h>
#include "allheaders.h"

    /* Global constant; initialized here; must be declared extern
     * in other files to access it directly.  However, in most
     * cases that is not necessary, because it can be reset
     * using resetMorphBoundaryCondition().  */
LEPT_DLL l_int32  MORPH_BC = ASYMMETRIC_MORPH_BC;

    /* We accept this cost in extra rasterops for decomposing exactly. */
static const l_int32  ACCEPTABLE_COST = 5;

    /* Static helpers for arg processing */
static PIX * processMorphArgs1(PIX *pixd, PIX *pixs, SEL *sel, PIX **ppixt);
static PIX * processMorphArgs2(PIX *pixd, PIX *pixs, SEL *sel);


/*-----------------------------------------------------------------*
 *    Generic binary morphological ops implemented with rasterop   *
 *-----------------------------------------------------------------*/
/*!
 *  pixDilate()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              sel
 *      Return: pixd
 *
 *  Notes:
 *      (1) This dilates src using hits in Sel.
 *      (2) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (3) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixDilate(NULL, pixs, ...);
 *          (b) pixDilate(pixs, pixs, ...);
 *          (c) pixDilate(pixd, pixs, ...);
 *      (4) The size of the result is determined by pixs.
 */
PIX *
pixDilate(PIX  *pixd,
          PIX  *pixs,
          SEL  *sel)
{
l_int32  i, j, w, h, sx, sy, cx, cy, seldata;
PIX     *pixt;

    PROCNAME("pixDilate");

    if ((pixd = processMorphArgs1(pixd, pixs, sel, &pixt)) == NULL)
        return (PIX *)ERROR_PTR("processMorphArgs1 failed", procName, pixd);

    pixGetDimensions(pixs, &w, &h, NULL);
    selGetParameters(sel, &sy, &sx, &cy, &cx);
    pixClearAll(pixd);
    for (i = 0; i < sy; i++) {
        for (j = 0; j < sx; j++) {
            seldata = sel->data[i][j];
            if (seldata == 1) {   /* src | dst */
                pixRasterop(pixd, j - cx, i - cy, w, h, PIX_SRC | PIX_DST,
                            pixt, 0, 0);
            }
        }
    }

    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixErode()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              sel
 *      Return: pixd
 *
 *  Notes:
 *      (1) This erodes src using hits in Sel.
 *      (2) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (3) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixErode(NULL, pixs, ...);
 *          (b) pixErode(pixs, pixs, ...);
 *          (c) pixErode(pixd, pixs, ...);
 *      (4) The size of the result is determined by pixs.
 */
PIX *
pixErode(PIX  *pixd,
         PIX  *pixs,
         SEL  *sel)
{
l_int32  i, j, w, h, sx, sy, cx, cy, seldata;
l_int32  xp, yp, xn, yn;
PIX     *pixt;

    PROCNAME("pixErode");

    if ((pixd = processMorphArgs1(pixd, pixs, sel, &pixt)) == NULL)
        return (PIX *)ERROR_PTR("processMorphArgs1 failed", procName, pixd);

    pixGetDimensions(pixs, &w, &h, NULL);
    selGetParameters(sel, &sy, &sx, &cy, &cx);
    pixSetAll(pixd);
    for (i = 0; i < sy; i++) {
        for (j = 0; j < sx; j++) {
            seldata = sel->data[i][j];
            if (seldata == 1) {   /* src & dst */
                    pixRasterop(pixd, cx - j, cy - i, w, h, PIX_SRC & PIX_DST,
                                pixt, 0, 0);
            }
        }
    }

        /* Clear near edges.  We do this for the asymmetric boundary
         * condition convention that implements erosion assuming all
         * pixels surrounding the image are OFF.  If you use a
         * use a symmetric b.c. convention, where the erosion is
         * implemented assuming pixels surrounding the image
         * are ON, these operations are omitted.  */
    if (MORPH_BC == ASYMMETRIC_MORPH_BC) {
        selFindMaxTranslations(sel, &xp, &yp, &xn, &yn);
        if (xp > 0)
            pixRasterop(pixd, 0, 0, xp, h, PIX_CLR, NULL, 0, 0);
        if (xn > 0)
            pixRasterop(pixd, w - xn, 0, xn, h, PIX_CLR, NULL, 0, 0);
        if (yp > 0)
            pixRasterop(pixd, 0, 0, w, yp, PIX_CLR, NULL, 0, 0);
        if (yn > 0)
            pixRasterop(pixd, 0, h - yn, w, yn, PIX_CLR, NULL, 0, 0);
    }

    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixHMT()
 *
 *      Input:  pixd (<optional>; this can be null, equal to pixs,
 *                    or different from pixs)
 *              pixs (1 bpp)
 *              sel
 *      Return: pixd
 *
 *  Notes:
 *      (1) The hit-miss transform erodes the src, using both hits
 *          and misses in the Sel.  It ANDs the shifted src for hits
 *          and ANDs the inverted shifted src for misses.
 *      (2) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (3) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixHMT(NULL, pixs, ...);
 *          (b) pixHMT(pixs, pixs, ...);
 *          (c) pixHMT(pixd, pixs, ...);
 *      (4) The size of the result is determined by pixs.
 */
PIX *
pixHMT(PIX  *pixd,
       PIX  *pixs,
       SEL  *sel)
{
l_int32  i, j, w, h, sx, sy, cx, cy, firstrasterop, seldata;
l_int32  xp, yp, xn, yn;
PIX     *pixt;

    PROCNAME("pixHMT");

    if ((pixd = processMorphArgs1(pixd, pixs, sel, &pixt)) == NULL)
        return (PIX *)ERROR_PTR("processMorphArgs1 failed", procName, pixd);

    pixGetDimensions(pixs, &w, &h, NULL);
    selGetParameters(sel, &sy, &sx, &cy, &cx);
    firstrasterop = TRUE;
    for (i = 0; i < sy; i++) {
        for (j = 0; j < sx; j++) {
            seldata = sel->data[i][j];
            if (seldata == 1) {  /* hit */
                if (firstrasterop == TRUE) {  /* src only */
                    pixClearAll(pixd);
                    pixRasterop(pixd, cx - j, cy - i, w, h, PIX_SRC,
                                pixt, 0, 0);
                    firstrasterop = FALSE;
                } else {   /* src & dst */
                    pixRasterop(pixd, cx - j, cy - i, w, h, PIX_SRC & PIX_DST,
                                pixt, 0, 0);
                }
            } else if (seldata == 2) {  /* miss */
                if (firstrasterop == TRUE) {  /* ~src only */
                    pixSetAll(pixd);
                    pixRasterop(pixd, cx - j, cy - i, w, h, PIX_NOT(PIX_SRC),
                             pixt, 0, 0);
                    firstrasterop = FALSE;
                } else {  /* ~src & dst */
                    pixRasterop(pixd, cx - j, cy - i, w, h,
                                PIX_NOT(PIX_SRC) & PIX_DST,
                                pixt, 0, 0);
                }
            }
        }
    }

        /* Clear near edges */
    selFindMaxTranslations(sel, &xp, &yp, &xn, &yn);
    if (xp > 0)
        pixRasterop(pixd, 0, 0, xp, h, PIX_CLR, NULL, 0, 0);
    if (xn > 0)
        pixRasterop(pixd, w - xn, 0, xn, h, PIX_CLR, NULL, 0, 0);
    if (yp > 0)
        pixRasterop(pixd, 0, 0, w, yp, PIX_CLR, NULL, 0, 0);
    if (yn > 0)
        pixRasterop(pixd, 0, h - yn, w, yn, PIX_CLR, NULL, 0, 0);

    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixOpen()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              sel
 *      Return: pixd
 *
 *  Notes:
 *      (1) Generic morphological opening, using hits in the Sel.
 *      (2) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (3) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixOpen(NULL, pixs, ...);
 *          (b) pixOpen(pixs, pixs, ...);
 *          (c) pixOpen(pixd, pixs, ...);
 *      (4) The size of the result is determined by pixs.
 */
PIX *
pixOpen(PIX  *pixd,
        PIX  *pixs,
        SEL  *sel)
{
PIX  *pixt;

    PROCNAME("pixOpen");

    if ((pixd = processMorphArgs2(pixd, pixs, sel)) == NULL)
        return (PIX *)ERROR_PTR("pixd not returned", procName, pixd);

    if ((pixt = pixErode(NULL, pixs, sel)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, pixd);
    pixDilate(pixd, pixt, sel);
    pixDestroy(&pixt);

    return pixd;
}


/*!
 *  pixClose()
 *
 *      Input:  pixd (<optional>; this can be null, equal to pixs,
 *                    or different from pixs)
 *              pixs (1 bpp)
 *              sel
 *      Return: pixd
 *
 *  Notes:
 *      (1) Generic morphological closing, using hits in the Sel.
 *      (2) This implementation is a strict dual of the opening if
 *          symmetric boundary conditions are used (see notes at top
 *          of this file).
 *      (3) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (4) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixClose(NULL, pixs, ...);
 *          (b) pixClose(pixs, pixs, ...);
 *          (c) pixClose(pixd, pixs, ...);
 *      (5) The size of the result is determined by pixs.
 */
PIX *
pixClose(PIX  *pixd,
         PIX  *pixs,
         SEL  *sel)
{
PIX  *pixt;

    PROCNAME("pixClose");

    if ((pixd = processMorphArgs2(pixd, pixs, sel)) == NULL)
        return (PIX *)ERROR_PTR("pixd not returned", procName, pixd);

    if ((pixt = pixDilate(NULL, pixs, sel)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, pixd);
    pixErode(pixd, pixt, sel);
    pixDestroy(&pixt);

    return pixd;
}


/*!
 *  pixCloseSafe()
 *
 *      Input:  pixd (<optional>; this can be null, equal to pixs,
 *                    or different from pixs)
 *              pixs (1 bpp)
 *              sel
 *      Return: pixd
 *
 *  Notes:
 *      (1) Generic morphological closing, using hits in the Sel.
 *      (2) If non-symmetric boundary conditions are used, this
 *          function adds a border of OFF pixels that is of
 *          sufficient size to avoid losing pixels from the dilation,
 *          and it removes the border after the operation is finished.
 *          It thus enforces a correct extensive result for closing.
 *      (3) If symmetric b.c. are used, it is not necessary to add
 *          and remove this border.
 *      (4) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (5) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixCloseSafe(NULL, pixs, ...);
 *          (b) pixCloseSafe(pixs, pixs, ...);
 *          (c) pixCloseSafe(pixd, pixs, ...);
 *      (6) The size of the result is determined by pixs.
 */
PIX *
pixCloseSafe(PIX  *pixd,
             PIX  *pixs,
             SEL  *sel)
{
l_int32  xp, yp, xn, yn, xmax, xbord;
PIX     *pixt1, *pixt2;

    PROCNAME("pixCloseSafe");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (!sel)
        return (PIX *)ERROR_PTR("sel not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);

        /* Symmetric b.c. handles correctly without added pixels */
    if (MORPH_BC == SYMMETRIC_MORPH_BC)
        return pixClose(pixd, pixs, sel);

    selFindMaxTranslations(sel, &xp, &yp, &xn, &yn);
    xmax = L_MAX(xp, xn);
    xbord = 32 * ((xmax + 31) / 32);  /* full 32 bit words */

    if ((pixt1 = pixAddBorderGeneral(pixs, xbord, xbord, yp, yn, 0)) == NULL)
        return (PIX *)ERROR_PTR("pixt1 not made", procName, pixd);
    pixClose(pixt1, pixt1, sel);
    if ((pixt2 = pixRemoveBorderGeneral(pixt1, xbord, xbord, yp, yn)) == NULL)
        return (PIX *)ERROR_PTR("pixt2 not made", procName, pixd);
    pixDestroy(&pixt1);

    if (!pixd)
        return pixt2;

    pixCopy(pixd, pixt2);
    pixDestroy(&pixt2);
    return pixd;
}


/*!
 *  pixOpenGeneralized()
 *
 *      Input:  pixd (<optional>; this can be null, equal to pixs,
 *                    or different from pixs)
 *              pixs (1 bpp)
 *              sel
 *      Return: pixd
 *
 *  Notes:
 *      (1) Generalized morphological opening, using both hits and
 *          misses in the Sel.
 *      (2) This does a hit-miss transform, followed by a dilation
 *          using the hits.
 *      (3) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (4) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixOpenGeneralized(NULL, pixs, ...);
 *          (b) pixOpenGeneralized(pixs, pixs, ...);
 *          (c) pixOpenGeneralized(pixd, pixs, ...);
 *      (5) The size of the result is determined by pixs.
 */
PIX *
pixOpenGeneralized(PIX  *pixd,
                   PIX  *pixs,
                   SEL  *sel)
{
PIX  *pixt;

    PROCNAME("pixOpenGeneralized");

    if ((pixd = processMorphArgs2(pixd, pixs, sel)) == NULL)
        return (PIX *)ERROR_PTR("pixd not returned", procName, pixd);

    if ((pixt = pixHMT(NULL, pixs, sel)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, pixd);
    pixDilate(pixd, pixt, sel);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixCloseGeneralized()
 *
 *      Input:  pixd (<optional>; this can be null, equal to pixs,
 *                    or different from pixs)
 *              pixs (1 bpp)
 *              sel
 *      Return: pixd
 *
 *  Notes:
 *      (1) Generalized morphological closing, using both hits and
 *          misses in the Sel.
 *      (2) This does a dilation using the hits, followed by a
 *          hit-miss transform.
 *      (3) This operation is a dual of the generalized opening.
 *      (4) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (5) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixCloseGeneralized(NULL, pixs, ...);
 *          (b) pixCloseGeneralized(pixs, pixs, ...);
 *          (c) pixCloseGeneralized(pixd, pixs, ...);
 *      (6) The size of the result is determined by pixs.
 */
PIX *
pixCloseGeneralized(PIX  *pixd,
                    PIX  *pixs,
                    SEL  *sel)
{
PIX  *pixt;

    PROCNAME("pixCloseGeneralized");

    if ((pixd = processMorphArgs2(pixd, pixs, sel)) == NULL)
        return (PIX *)ERROR_PTR("pixd not returned", procName, pixd);

    if ((pixt = pixDilate(NULL, pixs, sel)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, pixd);
    pixHMT(pixd, pixt, sel);
    pixDestroy(&pixt);

    return pixd;
}


/*-----------------------------------------------------------------*
 *          Binary morphological (raster) ops with brick Sels      *
 *-----------------------------------------------------------------*/
/*!
 *  pixDilateBrick()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              hsize (width of brick Sel)
 *              vsize (height of brick Sel)
 *      Return: pixd
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) The origin is at (x, y) = (hsize/2, vsize/2)
 *      (3) Do separably if both hsize and vsize are > 1.
 *      (4) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (5) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixDilateBrick(NULL, pixs, ...);
 *          (b) pixDilateBrick(pixs, pixs, ...);
 *          (c) pixDilateBrick(pixd, pixs, ...);
 *      (6) The size of the result is determined by pixs.
 */
PIX *
pixDilateBrick(PIX     *pixd,
               PIX     *pixs,
               l_int32  hsize,
               l_int32  vsize)
{
PIX  *pixt;
SEL  *sel, *selh, *selv;

    PROCNAME("pixDilateBrick");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize and vsize not >= 1", procName, pixd);

    if (hsize == 1 && vsize == 1)
        return pixCopy(pixd, pixs);
    if (hsize == 1 || vsize == 1) {  /* no intermediate result */
        sel = selCreateBrick(vsize, hsize, vsize / 2, hsize / 2, SEL_HIT);
        pixd = pixDilate(pixd, pixs, sel);
        selDestroy(&sel);
    } else {
        selh = selCreateBrick(1, hsize, 0, hsize / 2, SEL_HIT);
        selv = selCreateBrick(vsize, 1, vsize / 2, 0, SEL_HIT);
        pixt = pixDilate(NULL, pixs, selh);
        pixd = pixDilate(pixd, pixt, selv);
        pixDestroy(&pixt);
        selDestroy(&selh);
        selDestroy(&selv);
    }

    return pixd;
}


/*!
 *  pixErodeBrick()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              hsize (width of brick Sel)
 *              vsize (height of brick Sel)
 *      Return: pixd
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) The origin is at (x, y) = (hsize/2, vsize/2)
 *      (3) Do separably if both hsize and vsize are > 1.
 *      (4) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (5) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixErodeBrick(NULL, pixs, ...);
 *          (b) pixErodeBrick(pixs, pixs, ...);
 *          (c) pixErodeBrick(pixd, pixs, ...);
 *      (6) The size of the result is determined by pixs.
 */
PIX *
pixErodeBrick(PIX     *pixd,
              PIX     *pixs,
              l_int32  hsize,
              l_int32  vsize)
{
PIX  *pixt;
SEL  *sel, *selh, *selv;

    PROCNAME("pixErodeBrick");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize and vsize not >= 1", procName, pixd);

    if (hsize == 1 && vsize == 1)
        return pixCopy(pixd, pixs);
    if (hsize == 1 || vsize == 1) {  /* no intermediate result */
        sel = selCreateBrick(vsize, hsize, vsize / 2, hsize / 2, SEL_HIT);
        pixd = pixErode(pixd, pixs, sel);
        selDestroy(&sel);
    } else {
        selh = selCreateBrick(1, hsize, 0, hsize / 2, SEL_HIT);
        selv = selCreateBrick(vsize, 1, vsize / 2, 0, SEL_HIT);
        pixt = pixErode(NULL, pixs, selh);
        pixd = pixErode(pixd, pixt, selv);
        pixDestroy(&pixt);
        selDestroy(&selh);
        selDestroy(&selv);
    }

    return pixd;
}


/*!
 *  pixOpenBrick()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              hsize (width of brick Sel)
 *              vsize (height of brick Sel)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) The origin is at (x, y) = (hsize/2, vsize/2)
 *      (3) Do separably if both hsize and vsize are > 1.
 *      (4) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (5) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixOpenBrick(NULL, pixs, ...);
 *          (b) pixOpenBrick(pixs, pixs, ...);
 *          (c) pixOpenBrick(pixd, pixs, ...);
 *      (6) The size of the result is determined by pixs.
 */
PIX *
pixOpenBrick(PIX     *pixd,
             PIX     *pixs,
             l_int32  hsize,
             l_int32  vsize)
{
PIX  *pixt;
SEL  *sel, *selh, *selv;

    PROCNAME("pixOpenBrick");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize and vsize not >= 1", procName, pixd);

    if (hsize == 1 && vsize == 1)
        return pixCopy(pixd, pixs);
    if (hsize == 1 || vsize == 1) {  /* no intermediate result */
        sel = selCreateBrick(vsize, hsize, vsize / 2, hsize / 2, SEL_HIT);
        pixd = pixOpen(pixd, pixs, sel);
        selDestroy(&sel);
    } else {  /* do separably */
        selh = selCreateBrick(1, hsize, 0, hsize / 2, SEL_HIT);
        selv = selCreateBrick(vsize, 1, vsize / 2, 0, SEL_HIT);
        pixt = pixErode(NULL, pixs, selh);
        pixd = pixErode(pixd, pixt, selv);
        pixDilate(pixt, pixd, selh);
        pixDilate(pixd, pixt, selv);
        pixDestroy(&pixt);
        selDestroy(&selh);
        selDestroy(&selv);
    }

    return pixd;
}


/*!
 *  pixCloseBrick()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              hsize (width of brick Sel)
 *              vsize (height of brick Sel)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) The origin is at (x, y) = (hsize/2, vsize/2)
 *      (3) Do separably if both hsize and vsize are > 1.
 *      (4) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (5) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixCloseBrick(NULL, pixs, ...);
 *          (b) pixCloseBrick(pixs, pixs, ...);
 *          (c) pixCloseBrick(pixd, pixs, ...);
 *      (6) The size of the result is determined by pixs.
 */
PIX *
pixCloseBrick(PIX     *pixd,
              PIX     *pixs,
              l_int32  hsize,
              l_int32  vsize)
{
PIX  *pixt;
SEL  *sel, *selh, *selv;

    PROCNAME("pixCloseBrick");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize and vsize not >= 1", procName, pixd);

    if (hsize == 1 && vsize == 1)
        return pixCopy(pixd, pixs);
    if (hsize == 1 || vsize == 1) {  /* no intermediate result */
        sel = selCreateBrick(vsize, hsize, vsize / 2, hsize / 2, SEL_HIT);
        pixd = pixClose(pixd, pixs, sel);
        selDestroy(&sel);
    } else {  /* do separably */
        selh = selCreateBrick(1, hsize, 0, hsize / 2, SEL_HIT);
        selv = selCreateBrick(vsize, 1, vsize / 2, 0, SEL_HIT);
        pixt = pixDilate(NULL, pixs, selh);
        pixd = pixDilate(pixd, pixt, selv);
        pixErode(pixt, pixd, selh);
        pixErode(pixd, pixt, selv);
        pixDestroy(&pixt);
        selDestroy(&selh);
        selDestroy(&selv);
    }

    return pixd;
}


/*!
 *  pixCloseSafeBrick()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              hsize (width of brick Sel)
 *              vsize (height of brick Sel)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) The origin is at (x, y) = (hsize/2, vsize/2)
 *      (3) Do separably if both hsize and vsize are > 1.
 *      (4) Safe closing adds a border of 0 pixels, of sufficient size so
 *          that all pixels in input image are processed within
 *          32-bit words in the expanded image.  As a result, there is
 *          no special processing for pixels near the boundary, and there
 *          are no boundary effects.  The border is removed at the end.
 *      (5) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (6) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixCloseBrick(NULL, pixs, ...);
 *          (b) pixCloseBrick(pixs, pixs, ...);
 *          (c) pixCloseBrick(pixd, pixs, ...);
 *      (7) The size of the result is determined by pixs.
 */
PIX *
pixCloseSafeBrick(PIX     *pixd,
                  PIX     *pixs,
                  l_int32  hsize,
                  l_int32  vsize)
{
l_int32  maxtrans, bordsize;
PIX     *pixsb, *pixt, *pixdb;
SEL     *sel, *selh, *selv;

    PROCNAME("pixCloseSafeBrick");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize and vsize not >= 1", procName, pixd);

    if (hsize == 1 && vsize == 1)
        return pixCopy(pixd, pixs);

        /* Symmetric b.c. handles correctly without added pixels */
    if (MORPH_BC == SYMMETRIC_MORPH_BC)
        return pixCloseBrick(pixd, pixs, hsize, vsize);

    maxtrans = L_MAX(hsize / 2, vsize / 2);
    bordsize = 32 * ((maxtrans + 31) / 32);  /* full 32 bit words */
    pixsb = pixAddBorder(pixs, bordsize, 0);

    if (hsize == 1 || vsize == 1) {  /* no intermediate result */
        sel = selCreateBrick(vsize, hsize, vsize / 2, hsize / 2, SEL_HIT);
        pixdb = pixClose(NULL, pixsb, sel);
        selDestroy(&sel);
    } else {  /* do separably */
        selh = selCreateBrick(1, hsize, 0, hsize / 2, SEL_HIT);
        selv = selCreateBrick(vsize, 1, vsize / 2, 0, SEL_HIT);
        pixt = pixDilate(NULL, pixsb, selh);
        pixdb = pixDilate(NULL, pixt, selv);
        pixErode(pixt, pixdb, selh);
        pixErode(pixdb, pixt, selv);
        pixDestroy(&pixt);
        selDestroy(&selh);
        selDestroy(&selv);
    }

    pixt = pixRemoveBorder(pixdb, bordsize);
    pixDestroy(&pixsb);
    pixDestroy(&pixdb);

    if (!pixd) {
        pixd = pixt;
    } else {
        pixCopy(pixd, pixt);
        pixDestroy(&pixt);
    }

    return pixd;
}


/*-----------------------------------------------------------------*
 *     Binary composed morphological (raster) ops with brick Sels  *
 *-----------------------------------------------------------------*/
/*  selectComposableSels()
 *
 *      Input:  size (of composed sel)
 *              direction (L_HORIZ, L_VERT)
 *              &sel1 (<optional return> contiguous sel; can be null)
 *              &sel2 (<optional return> comb sel; can be null)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) When using composable Sels, where the original Sel is
 *          decomposed into two, the best you can do in terms
 *          of reducing the computation is by a factor:
 *
 *               2 * sqrt(size) / size
 *
 *          In practice, you get quite close to this.  E.g.,
 *
 *             Sel size     |   Optimum reduction factor
 *             --------         ------------------------
 *                36        |          1/3
 *                64        |          1/4
 *               144        |          1/6
 *               256        |          1/8
 */
l_int32
selectComposableSels(l_int32  size,
                     l_int32  direction,
                     SEL    **psel1,
                     SEL    **psel2)
{
l_int32  factor1, factor2;

    PROCNAME("selectComposableSels");

    if (!psel1 && !psel2)
        return ERROR_INT("neither &sel1 nor &sel2 are defined", procName, 1);
    if (psel1) *psel1 = NULL;
    if (psel2) *psel2 = NULL;
    if (size < 1 || size > 250 * 250)
        return ERROR_INT("size < 1", procName, 1);
    if (direction != L_HORIZ && direction != L_VERT)
        return ERROR_INT("invalid direction", procName, 1);

    if (selectComposableSizes(size, &factor1, &factor2))
        return ERROR_INT("factors not found", procName, 1);

    if (psel1) {
        if (direction == L_HORIZ)
            *psel1 = selCreateBrick(1, factor1, 0, factor1 / 2, SEL_HIT);
        else
            *psel1 = selCreateBrick(factor1, 1, factor1 / 2 , 0, SEL_HIT);
    }
    if (psel2)
        *psel2 = selCreateComb(factor1, factor2, direction);
    return 0;
}


/*!
 *  selectComposableSizes()
 *
 *      Input:  size (of sel to be decomposed)
 *              &factor1 (<return> larger factor)
 *              &factor2 (<return> smaller factor)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This works for Sel sizes up to 62500, which seems sufficient.
 *      (2) The composable sel size is typically within +- 1 of
 *          the requested size.  Up to size = 300, the maximum difference
 *          is +- 2.
 *      (3) We choose an overall cost function where the penalty for
 *          the size difference between input and actual is 4 times
 *          the penalty for additional rasterops.
 *      (4) Returned values: factor1 >= factor2
 *          If size > 1, then factor1 > 1.
 */
l_int32
selectComposableSizes(l_int32   size,
                      l_int32  *pfactor1,
                      l_int32  *pfactor2)
{
l_int32  i, midval, val1, val2m, val2p;
l_int32  index, prodm, prodp;
l_int32  mincost, totcost, rastcostm, rastcostp, diffm, diffp;
l_int32  lowval[256];
l_int32  hival[256];
l_int32  rastcost[256];  /* excess in sum of sizes (extra rasterops) */
l_int32  diff[256];  /* diff between product (sel size) and input size */

    PROCNAME("selectComposableSizes");

    if (size < 1 || size > 250 * 250)
        return ERROR_INT("size < 1", procName, 1);
    if (!pfactor1 || !pfactor2)
        return ERROR_INT("&factor1 or &factor2 not defined", procName, 1);

    midval = (l_int32)(sqrt((l_float64)size) + 0.001);
    if (midval * midval == size) {
        *pfactor1 = *pfactor2 = midval;
        return 0;
    }

        /* Set up arrays.  For each val1, optimize for lowest diff,
         * and save the rastcost, the diff, and the two factors. */
    for (val1 = midval + 1, i = 0; val1 > 0; val1--, i++) {
        val2m = size / val1;
        val2p = val2m + 1;
        prodm = val1 * val2m;
        prodp = val1 * val2p;
        rastcostm = val1 + val2m - 2 * midval;
        rastcostp = val1 + val2p - 2 * midval;
        diffm = L_ABS(size - prodm);
        diffp = L_ABS(size - prodp);
        if (diffm <= diffp) {
            lowval[i] = L_MIN(val1, val2m);
            hival[i] = L_MAX(val1, val2m);
            rastcost[i] = rastcostm;
            diff[i] = diffm;
        } else {
            lowval[i] = L_MIN(val1, val2p);
            hival[i] = L_MAX(val1, val2p);
            rastcost[i] = rastcostp;
            diff[i] = diffp;
        }
    }

        /* Choose the optimum factors; use cost ratio 4 on diff */
    mincost = 10000;
    for (i = 0; i < midval + 1; i++) {
        if (diff[i] == 0 && rastcost[i] < ACCEPTABLE_COST) {
            *pfactor1 = hival[i];
            *pfactor2 = lowval[i];
            return 0;
        }
        totcost = 4 * diff[i] + rastcost[i];
        if (totcost < mincost) {
            mincost = totcost;
            index = i;
        }
    }
    *pfactor1 = hival[index];
    *pfactor2 = lowval[index];

    return 0;
}


/*!
 *  pixDilateCompBrick()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              hsize (width of brick Sel)
 *              vsize (height of brick Sel)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) The origin is at (x, y) = (hsize/2, vsize/2)
 *      (3) Do compositely for each dimension > 1.
 *      (4) Do separably if both hsize and vsize are > 1.
 *      (5) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (6) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixDilateCompBrick(NULL, pixs, ...);
 *          (b) pixDilateCompBrick(pixs, pixs, ...);
 *          (c) pixDilateCompBrick(pixd, pixs, ...);
 *      (7) The dimensions of the resulting image are determined by pixs.
 *      (8) CAUTION: both hsize and vsize are being decomposed.
 *          The decomposer chooses a product of sizes (call them
 *          'terms') for each that is close to the input size,
 *          but not necessarily equal to it.  It attempts to optimize:
 *             (a) for consistency with the input values: the product
 *                 of terms is close to the input size
 *             (b) for efficiency of the operation: the sum of the
 *                 terms is small; ideally about twice the square
 *                 root of the input size.
 *          So, for example, if the input hsize = 37, which is
 *          a prime number, the decomposer will break this into two
 *          terms, 6 and 6, so that the net result is a dilation
 *          with hsize = 36.
 */
PIX *
pixDilateCompBrick(PIX     *pixd,
                   PIX     *pixs,
                   l_int32  hsize,
                   l_int32  vsize)
{
PIX  *pixt1, *pixt2, *pixt3;
SEL  *selh1, *selh2, *selv1, *selv2;

    PROCNAME("pixDilateCompBrick");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize and vsize not >= 1", procName, pixd);

    pixt1 = pixAddBorder(pixs, 32, 0);

    if (hsize == 1 && vsize == 1)
        return pixCopy(pixd, pixs);
    if (hsize > 1)
        selectComposableSels(hsize, L_HORIZ, &selh1, &selh2);
    if (vsize > 1)
        selectComposableSels(vsize, L_VERT, &selv1, &selv2);
    if (vsize == 1) {
        pixt2 = pixDilate(NULL, pixt1, selh1);
        pixt3 = pixDilate(NULL, pixt2, selh2);
    } else if (hsize == 1) {
        pixt2 = pixDilate(NULL, pixt1, selv1);
        pixt3 = pixDilate(NULL, pixt2, selv2);
    } else {
        pixt2 = pixDilate(NULL, pixt1, selh1);
        pixt3 = pixDilate(NULL, pixt2, selh2);
        pixDilate(pixt2, pixt3, selv1);
        pixDilate(pixt3, pixt2, selv2);
    }
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

    if (hsize > 1) {
        selDestroy(&selh1);
        selDestroy(&selh2);
    }
    if (vsize > 1) {
        selDestroy(&selv1);
        selDestroy(&selv2);
    }

    pixt1 = pixRemoveBorder(pixt3, 32);
    pixDestroy(&pixt3);
    if (!pixd)
        return pixt1;
    pixCopy(pixd, pixt1);
    pixDestroy(&pixt1);
    return pixd;
}


/*!
 *  pixErodeCompBrick()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              hsize (width of brick Sel)
 *              vsize (height of brick Sel)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) The origin is at (x, y) = (hsize/2, vsize/2)
 *      (3) Do compositely for each dimension > 1.
 *      (4) Do separably if both hsize and vsize are > 1.
 *      (5) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (6) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixErodeCompBrick(NULL, pixs, ...);
 *          (b) pixErodeCompBrick(pixs, pixs, ...);
 *          (c) pixErodeCompBrick(pixd, pixs, ...);
 *      (7) The dimensions of the resulting image are determined by pixs.
 *      (8) CAUTION: both hsize and vsize are being decomposed.
 *          The decomposer chooses a product of sizes (call them
 *          'terms') for each that is close to the input size,
 *          but not necessarily equal to it.  It attempts to optimize:
 *             (a) for consistency with the input values: the product
 *                 of terms is close to the input size
 *             (b) for efficiency of the operation: the sum of the
 *                 terms is small; ideally about twice the square
 *                 root of the input size.
 *          So, for example, if the input hsize = 37, which is
 *          a prime number, the decomposer will break this into two
 *          terms, 6 and 6, so that the net result is a dilation
 *          with hsize = 36.
 */
PIX *
pixErodeCompBrick(PIX     *pixd,
                  PIX     *pixs,
                  l_int32  hsize,
                  l_int32  vsize)
{
PIX  *pixt;
SEL  *selh1, *selh2, *selv1, *selv2;

    PROCNAME("pixErodeCompBrick");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize and vsize not >= 1", procName, pixd);

    if (hsize == 1 && vsize == 1)
        return pixCopy(pixd, pixs);
    if (hsize > 1)
        selectComposableSels(hsize, L_HORIZ, &selh1, &selh2);
    if (vsize > 1)
        selectComposableSels(vsize, L_VERT, &selv1, &selv2);
    if (vsize == 1) {
        pixt = pixErode(NULL, pixs, selh1);
        pixd = pixErode(pixd, pixt, selh2);
    } else if (hsize == 1) {
        pixt = pixErode(NULL, pixs, selv1);
        pixd = pixErode(pixd, pixt, selv2);
    } else {
        pixt = pixErode(NULL, pixs, selh1);
        pixd = pixErode(pixd, pixt, selh2);
        pixErode(pixt, pixd, selv1);
        pixErode(pixd, pixt, selv2);
    }
    pixDestroy(&pixt);

    if (hsize > 1) {
        selDestroy(&selh1);
        selDestroy(&selh2);
    }
    if (vsize > 1) {
        selDestroy(&selv1);
        selDestroy(&selv2);
    }

    return pixd;
}


/*!
 *  pixOpenCompBrick()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              hsize (width of brick Sel)
 *              vsize (height of brick Sel)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) The origin is at (x, y) = (hsize/2, vsize/2)
 *      (3) Do compositely for each dimension > 1.
 *      (4) Do separably if both hsize and vsize are > 1.
 *      (5) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (6) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixOpenCompBrick(NULL, pixs, ...);
 *          (b) pixOpenCompBrick(pixs, pixs, ...);
 *          (c) pixOpenCompBrick(pixd, pixs, ...);
 *      (7) The dimensions of the resulting image are determined by pixs.
 *      (8) CAUTION: both hsize and vsize are being decomposed.
 *          The decomposer chooses a product of sizes (call them
 *          'terms') for each that is close to the input size,
 *          but not necessarily equal to it.  It attempts to optimize:
 *             (a) for consistency with the input values: the product
 *                 of terms is close to the input size
 *             (b) for efficiency of the operation: the sum of the
 *                 terms is small; ideally about twice the square
 *                 root of the input size.
 *          So, for example, if the input hsize = 37, which is
 *          a prime number, the decomposer will break this into two
 *          terms, 6 and 6, so that the net result is a dilation
 *          with hsize = 36.
 */
PIX *
pixOpenCompBrick(PIX     *pixd,
                 PIX     *pixs,
                 l_int32  hsize,
                 l_int32  vsize)
{
PIX  *pixt;
SEL  *selh1, *selh2, *selv1, *selv2;

    PROCNAME("pixOpenCompBrick");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize and vsize not >= 1", procName, pixd);

    if (hsize == 1 && vsize == 1)
        return pixCopy(pixd, pixs);
    if (hsize > 1)
        selectComposableSels(hsize, L_HORIZ, &selh1, &selh2);
    if (vsize > 1)
        selectComposableSels(vsize, L_VERT, &selv1, &selv2);
    if (vsize == 1) {
        pixt = pixErode(NULL, pixs, selh1);
        pixd = pixErode(pixd, pixt, selh2);
        pixDilate(pixt, pixd, selh1);
        pixDilate(pixd, pixt, selh2);
    } else if (hsize == 1) {
        pixt = pixErode(NULL, pixs, selv1);
        pixd = pixErode(pixd, pixt, selv2);
        pixDilate(pixt, pixd, selv1);
        pixDilate(pixd, pixt, selv2);
    } else {  /* do separably */
        pixt = pixErode(NULL, pixs, selh1);
        pixd = pixErode(pixd, pixt, selh2);
        pixErode(pixt, pixd, selv1);
        pixErode(pixd, pixt, selv2);
        pixDilate(pixt, pixd, selh1);
        pixDilate(pixd, pixt, selh2);
        pixDilate(pixt, pixd, selv1);
        pixDilate(pixd, pixt, selv2);
    }
    pixDestroy(&pixt);

    if (hsize > 1) {
        selDestroy(&selh1);
        selDestroy(&selh2);
    }
    if (vsize > 1) {
        selDestroy(&selv1);
        selDestroy(&selv2);
    }

    return pixd;
}


/*!
 *  pixCloseCompBrick()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              hsize (width of brick Sel)
 *              vsize (height of brick Sel)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) The origin is at (x, y) = (hsize/2, vsize/2)
 *      (3) Do compositely for each dimension > 1.
 *      (4) Do separably if both hsize and vsize are > 1.
 *      (5) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (6) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixCloseCompBrick(NULL, pixs, ...);
 *          (b) pixCloseCompBrick(pixs, pixs, ...);
 *          (c) pixCloseCompBrick(pixd, pixs, ...);
 *      (7) The dimensions of the resulting image are determined by pixs.
 *      (8) CAUTION: both hsize and vsize are being decomposed.
 *          The decomposer chooses a product of sizes (call them
 *          'terms') for each that is close to the input size,
 *          but not necessarily equal to it.  It attempts to optimize:
 *             (a) for consistency with the input values: the product
 *                 of terms is close to the input size
 *             (b) for efficiency of the operation: the sum of the
 *                 terms is small; ideally about twice the square
 *                 root of the input size.
 *          So, for example, if the input hsize = 37, which is
 *          a prime number, the decomposer will break this into two
 *          terms, 6 and 6, so that the net result is a dilation
 *          with hsize = 36.
 */
PIX *
pixCloseCompBrick(PIX     *pixd,
                  PIX     *pixs,
                  l_int32  hsize,
                  l_int32  vsize)
{
PIX  *pixt;
SEL  *selh1, *selh2, *selv1, *selv2;

    PROCNAME("pixCloseCompBrick");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize and vsize not >= 1", procName, pixd);

    if (hsize == 1 && vsize == 1)
        return pixCopy(pixd, pixs);
    if (hsize > 1)
        selectComposableSels(hsize, L_HORIZ, &selh1, &selh2);
    if (vsize > 1)
        selectComposableSels(vsize, L_VERT, &selv1, &selv2);
    if (vsize == 1) {
        pixt = pixDilate(NULL, pixs, selh1);
        pixd = pixDilate(pixd, pixt, selh2);
        pixErode(pixt, pixd, selh1);
        pixErode(pixd, pixt, selh2);
    } else if (hsize == 1) {
        pixt = pixDilate(NULL, pixs, selv1);
        pixd = pixDilate(pixd, pixt, selv2);
        pixErode(pixt, pixd, selv1);
        pixErode(pixd, pixt, selv2);
    } else {  /* do separably */
        pixt = pixDilate(NULL, pixs, selh1);
        pixd = pixDilate(pixd, pixt, selh2);
        pixDilate(pixt, pixd, selv1);
        pixDilate(pixd, pixt, selv2);
        pixErode(pixt, pixd, selh1);
        pixErode(pixd, pixt, selh2);
        pixErode(pixt, pixd, selv1);
        pixErode(pixd, pixt, selv2);
    }
    pixDestroy(&pixt);

    if (hsize > 1) {
        selDestroy(&selh1);
        selDestroy(&selh2);
    }
    if (vsize > 1) {
        selDestroy(&selv1);
        selDestroy(&selv2);
    }

    return pixd;
}


/*!
 *  pixCloseSafeCompBrick()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (1 bpp)
 *              hsize (width of brick Sel)
 *              vsize (height of brick Sel)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) The origin is at (x, y) = (hsize/2, vsize/2)
 *      (3) Do compositely for each dimension > 1.
 *      (4) Do separably if both hsize and vsize are > 1.
 *      (5) Safe closing adds a border of 0 pixels, of sufficient size so
 *          that all pixels in input image are processed within
 *          32-bit words in the expanded image.  As a result, there is
 *          no special processing for pixels near the boundary, and there
 *          are no boundary effects.  The border is removed at the end.
 *      (6) There are three cases:
 *          (a) pixd == null   (result into new pixd)
 *          (b) pixd == pixs   (in-place; writes result back to pixs)
 *          (c) pixd != pixs   (puts result into existing pixd)
 *      (7) For clarity, if the case is known, use these patterns:
 *          (a) pixd = pixCloseSafeCompBrick(NULL, pixs, ...);
 *          (b) pixCloseSafeCompBrick(pixs, pixs, ...);
 *          (c) pixCloseSafeCompBrick(pixd, pixs, ...);
 *      (8) The dimensions of the resulting image are determined by pixs.
 *      (9) CAUTION: both hsize and vsize are being decomposed.
 *          The decomposer chooses a product of sizes (call them
 *          'terms') for each that is close to the input size,
 *          but not necessarily equal to it.  It attempts to optimize:
 *             (a) for consistency with the input values: the product
 *                 of terms is close to the input size
 *             (b) for efficiency of the operation: the sum of the
 *                 terms is small; ideally about twice the square
 *                 root of the input size.
 *          So, for example, if the input hsize = 37, which is
 *          a prime number, the decomposer will break this into two
 *          terms, 6 and 6, so that the net result is a dilation
 *          with hsize = 36.
 */
PIX *
pixCloseSafeCompBrick(PIX     *pixd,
                      PIX     *pixs,
                      l_int32  hsize,
                      l_int32  vsize)
{
l_int32  maxtrans, bordsize;
PIX     *pixsb, *pixt, *pixdb;
SEL     *selh1, *selh2, *selv1, *selv2;

    PROCNAME("pixCloseSafeCompBrick");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize and vsize not >= 1", procName, pixd);

    if (hsize == 1 && vsize == 1)
        return pixCopy(pixd, pixs);

        /* Symmetric b.c. handles correctly without added pixels */
    if (MORPH_BC == SYMMETRIC_MORPH_BC)
        return pixCloseCompBrick(pixd, pixs, hsize, vsize);

    maxtrans = L_MAX(hsize / 2, vsize / 2);
    bordsize = 32 * ((maxtrans + 31) / 32);  /* full 32 bit words */
    pixsb = pixAddBorder(pixs, bordsize, 0);

    if (hsize > 1)
        selectComposableSels(hsize, L_HORIZ, &selh1, &selh2);
    if (vsize > 1)
        selectComposableSels(vsize, L_VERT, &selv1, &selv2);
    if (vsize == 1) {
        pixt = pixDilate(NULL, pixsb, selh1);
        pixdb = pixDilate(NULL, pixt, selh2);
        pixErode(pixt, pixdb, selh1);
        pixErode(pixdb, pixt, selh2);
    } else if (hsize == 1) {
        pixt = pixDilate(NULL, pixsb, selv1);
        pixdb = pixDilate(NULL, pixt, selv2);
        pixErode(pixt, pixdb, selv1);
        pixErode(pixdb, pixt, selv2);
    } else {  /* do separably */
        pixt = pixDilate(NULL, pixsb, selh1);
        pixdb = pixDilate(NULL, pixt, selh2);
        pixDilate(pixt, pixdb, selv1);
        pixDilate(pixdb, pixt, selv2);
        pixErode(pixt, pixdb, selh1);
        pixErode(pixdb, pixt, selh2);
        pixErode(pixt, pixdb, selv1);
        pixErode(pixdb, pixt, selv2);
    }
    pixDestroy(&pixt);

    pixt = pixRemoveBorder(pixdb, bordsize);
    pixDestroy(&pixsb);
    pixDestroy(&pixdb);

    if (!pixd) {
        pixd = pixt;
    } else {
        pixCopy(pixd, pixt);
        pixDestroy(&pixt);
    }

    if (hsize > 1) {
        selDestroy(&selh1);
        selDestroy(&selh2);
    }
    if (vsize > 1) {
        selDestroy(&selv1);
        selDestroy(&selv2);
    }

    return pixd;
}


/*-----------------------------------------------------------------*
 *           Functions associated with boundary conditions         *
 *-----------------------------------------------------------------*/
/*!
 *  resetMorphBoundaryCondition()
 *
 *      Input:  bc (SYMMETRIC_MORPH_BC, ASYMMETRIC_MORPH_BC)
 *      Return: void
 */
void
resetMorphBoundaryCondition(l_int32  bc)
{
    PROCNAME("resetMorphBoundaryCondition");

    if (bc != SYMMETRIC_MORPH_BC && bc != ASYMMETRIC_MORPH_BC) {
        L_WARNING("invalid bc; using asymmetric\n", procName);
        bc = ASYMMETRIC_MORPH_BC;
    }
    MORPH_BC = bc;
    return;
}


/*!
 *  getMorphBorderPixelColor()
 *
 *      Input:  type (L_MORPH_DILATE, L_MORPH_ERODE)
 *              depth (of pix)
 *      Return: color of border pixels for this operation
 */
l_uint32
getMorphBorderPixelColor(l_int32  type,
                         l_int32  depth)
{
    PROCNAME("getMorphBorderPixelColor");

    if (type != L_MORPH_DILATE && type != L_MORPH_ERODE)
        return ERROR_INT("invalid type", procName, 0);
    if (depth != 1 && depth != 2 && depth != 4 && depth != 8 &&
        depth != 16 && depth != 32)
        return ERROR_INT("invalid depth", procName, 0);

    if (MORPH_BC == ASYMMETRIC_MORPH_BC || type == L_MORPH_DILATE)
        return 0;

        /* Symmetric & erosion */
    if (depth < 32)
        return ((1 << depth) - 1);
    else  /* depth == 32 */
        return 0xffffff00;
}


/*-----------------------------------------------------------------*
 *               Static helpers for arg processing                 *
 *-----------------------------------------------------------------*/
/*!
 *  processMorphArgs1()
 *
 *      Input:  pixd (<optional>; this can be null, equal to pixs,
 *                    or different from pixs)
 *              pixs (1 bpp)
 *              sel
 *              &pixt (<returned>)
 *      Return: pixd, or null on error.
 *
 *  Notes:
 *      (1) This is used for generic erosion, dilation and HMT.
 */
static PIX *
processMorphArgs1(PIX   *pixd,
                  PIX   *pixs,
                  SEL   *sel,
                  PIX  **ppixt)
{
l_int32  sx, sy;

    PROCNAME("processMorphArgs1");

    if (!ppixt)
        return (PIX *)ERROR_PTR("&pixt not defined", procName, pixd);
    *ppixt = NULL;
    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (!sel)
        return (PIX *)ERROR_PTR("sel not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);

    selGetParameters(sel, &sx, &sy, NULL, NULL);
    if (sx == 0 || sy == 0)
        return (PIX *)ERROR_PTR("sel of size 0", procName, pixd);

        /* We require pixd to exist and to be the same size as pixs.
         * Further, pixt must be a copy (or clone) of pixs.  */
    if (!pixd) {
        if ((pixd = pixCreateTemplate(pixs)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
        *ppixt = pixClone(pixs);
    } else {
        pixResizeImageData(pixd, pixs);
        if (pixd == pixs) {  /* in-place; must make a copy of pixs */
            if ((*ppixt = pixCopy(NULL, pixs)) == NULL)
                return (PIX *)ERROR_PTR("pixt not made", procName, pixd);
        } else {
            *ppixt = pixClone(pixs);
        }
    }
    return pixd;
}


/*!
 *  processMorphArgs2()
 *
 *  This is used for generic openings and closings.
 */
static PIX *
processMorphArgs2(PIX   *pixd,
                  PIX   *pixs,
                  SEL   *sel)
{
l_int32  sx, sy;

    PROCNAME("processMorphArgs2");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (!sel)
        return (PIX *)ERROR_PTR("sel not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);

    selGetParameters(sel, &sx, &sy, NULL, NULL);
    if (sx == 0 || sy == 0)
        return (PIX *)ERROR_PTR("sel of size 0", procName, pixd);

    if (!pixd)
        return pixCreateTemplate(pixs);
    pixResizeImageData(pixd, pixs);
    return pixd;
}
