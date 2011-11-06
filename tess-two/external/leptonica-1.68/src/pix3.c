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
 *  pix3.c
 *
 *    This file has these operations:
 *
 *      (1) Mask-directed operations
 *      (2) Full-image bit-logical operations
 *      (3) Foreground pixel counting operations on 1 bpp images
 *      (4) Sum of pixel values
 *      (5) Mirrored tiling of a smaller image
 *
 *
 *    Masked operations
 *           l_int32     pixSetMasked()
 *           l_int32     pixSetMaskedGeneral()
 *           l_int32     pixCombineMasked()
 *           l_int32     pixCombineMaskedGeneral()
 *           l_int32     pixPaintThroughMask()
 *           PIX        *pixPaintSelfThroughMask()
 *           PIX        *pixMakeMaskFromLUT()
 *           PIX        *pixSetUnderTransparency()
 *
 *    One and two-image boolean operations on arbitrary depth images
 *           PIX        *pixInvert()
 *           PIX        *pixOr()
 *           PIX        *pixAnd()
 *           PIX        *pixXor()
 *           PIX        *pixSubtract()
 *
 *    Foreground pixel counting in 1 bpp images
 *           l_int32     pixZero()
 *           l_int32     pixCountPixels()
 *           NUMA       *pixaCountPixels()
 *           l_int32     pixCountPixelsInRow()
 *           NUMA       *pixCountPixelsByRow()
 *           NUMA       *pixCountPixelsByColumn()
 *           NUMA       *pixSumPixelsByRow()
 *           NUMA       *pixSumPixelsByColumn()
 *           l_int32     pixThresholdPixelSum()
 *           l_int32    *makePixelSumTab8()
 *           l_int32    *makePixelCentroidTab8()
 *
 *    Sum of pixel values
 *           l_int32     pixSumPixelValues()
 *
 *    Mirrored tiling
 *           PIX        *pixMirroredTiling()
 *
 *    Static helper function
 *           static l_int32  findTilePatchCenter()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

static l_int32 findTilePatchCenter(PIX *pixs, BOX *box, l_int32 dir,
                                   l_uint32 targdist, l_uint32 *pdist,
                                   l_int32 *pxc, l_int32 *pyc);

#ifndef  NO_CONSOLE_IO
#define   EQUAL_SIZE_WARNING      0
#endif  /* ~NO_CONSOLE_IO */


/*-------------------------------------------------------------*
 *                        Masked operations                    *
 *-------------------------------------------------------------*/
/*!
 *  pixSetMasked()
 *
 *      Input:  pixd (1, 2, 4, 8, 16 or 32 bpp; or colormapped)
 *              pixm (<optional> 1 bpp mask; no operation if NULL)
 *              val (value to set at each masked pixel)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) In-place operation.
 *      (2) NOTE: For cmapped images, this calls pixSetMaskedCmap().
 *          @val must be the 32-bit color representation of the RGB pixel.
 *          It is not the index into the colormap!
 *      (2) If pixm == NULL, a warning is given.
 *      (3) This is an implicitly aligned operation, where the UL
 *          corners of pixd and pixm coincide.  A warning is
 *          issued if the two image sizes differ significantly,
 *          but the operation proceeds.
 *      (4) Each pixel in pixd that co-locates with an ON pixel
 *          in pixm is set to the specified input value.
 *          Other pixels in pixd are not changed.
 *      (5) You can visualize this as painting the color through
 *          the mask, as a stencil.
 *      (6) If you do not want to have the UL corners aligned,
 *          use the function pixSetMaskedGeneral(), which requires
 *          you to input the UL corner of pixm relative to pixd.
 *      (7) Implementation details: see comments in pixPaintThroughMask()
 *          for when we use rasterop to do the painting.
 */
l_int32
pixSetMasked(PIX      *pixd,
             PIX      *pixm,
             l_uint32  val)
{
l_int32    wd, hd, wm, hm, w, h, d, wpld, wplm;
l_int32    i, j, rval, gval, bval;
l_uint32  *datad, *datam, *lined, *linem;

    PROCNAME("pixSetMasked");

    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (!pixm) {
        L_WARNING("no mask; nothing to do", procName);
        return 0;
    }
    if (pixGetColormap(pixd)) {
        extractRGBValues(val, &rval, &gval, &bval);
        return pixSetMaskedCmap(pixd, pixm, 0, 0, rval, gval, bval);
    }

    if (pixGetDepth(pixm) != 1)
        return ERROR_INT("pixm not 1 bpp", procName, 1);
    d = pixGetDepth(pixd);
    if (d == 1)
        val &= 1;
    else if (d == 2)
        val &= 3;
    else if (d == 4)
        val &= 0x0f;
    else if (d == 8)
        val &= 0xff;
    else if (d == 16)
        val &= 0xffff;
    else if (d != 32)
        return ERROR_INT("pixd not 1, 2, 4, 8, 16 or 32 bpp", procName, 1);
    pixGetDimensions(pixm, &wm, &hm, NULL);

        /* If d == 1, use rasterop; it's about 25x faster */
    if (d == 1) {
        if (val == 0) {
            PIX *pixmi = pixInvert(NULL, pixm);
            pixRasterop(pixd, 0, 0, wm, hm, PIX_MASK, pixmi, 0, 0);
            pixDestroy(&pixmi);
        }
        else  /* val == 1 */
            pixRasterop(pixd, 0, 0, wm, hm, PIX_PAINT, pixm, 0, 0);
        return 0;
    }
    
        /* For d < 32, use rasterop for val == 0 (black); ~3x faster. */
    if (d < 32 && val == 0) {
        PIX *pixmd = pixUnpackBinary(pixm, d, 1);
        pixRasterop(pixd, 0, 0, wm, hm, PIX_MASK, pixmd, 0, 0);
        pixDestroy(&pixmd);
        return 0;
    }

        /* For d < 32, use rasterop for val == maxval (white); ~3x faster. */
    if (d < 32 && val == ((1 << d) - 1)) {
        PIX *pixmd = pixUnpackBinary(pixm, d, 0);
        pixRasterop(pixd, 0, 0, wm, hm, PIX_PAINT, pixmd, 0, 0);
        pixDestroy(&pixmd);
        return 0;
    }

    pixGetDimensions(pixd, &wd, &hd, &d);
    w = L_MIN(wd, wm);
    h = L_MIN(hd, hm);
    if (L_ABS(wd - wm) > 7 || L_ABS(hd - hm) > 7)  /* allow a small tolerance */
        L_WARNING("pixd and pixm sizes differ", procName);

    datad = pixGetData(pixd);
    datam = pixGetData(pixm);
    wpld = pixGetWpl(pixd);
    wplm = pixGetWpl(pixm);
    for (i = 0; i < h; i++) {
        lined = datad + i * wpld;
        linem = datam + i * wplm;
        for (j = 0; j < w; j++) {
            if (GET_DATA_BIT(linem, j)) {
                switch(d)
                {
                case 2:
                    SET_DATA_DIBIT(lined, j, val);
                    break;
                case 4:
                    SET_DATA_QBIT(lined, j, val);
                    break;
                case 8:
                    SET_DATA_BYTE(lined, j, val);
                    break;
                case 16:
                    SET_DATA_TWO_BYTES(lined, j, val);
                    break;
                case 32:
                    *(lined + j) = val;
                    break;
                default:
                    return ERROR_INT("shouldn't get here", procName, 1);
                }
            }
        }
    }

    return 0;
}


/*!
 *  pixSetMaskedGeneral()
 *
 *      Input:  pixd (8, 16 or 32 bpp)
 *              pixm (<optional> 1 bpp mask; no operation if null)
 *              val (value to set at each masked pixel)
 *              x, y (location of UL corner of pixm relative to pixd;
 *                    can be negative)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This is an in-place operation.
 *      (2) Alignment is explicit.  If you want the UL corners of
 *          the two images to be aligned, use pixSetMasked().
 *      (3) A typical use would be painting through the foreground
 *          of a small binary mask pixm, located somewhere on a
 *          larger pixd.  Other pixels in pixd are not changed.
 *      (4) You can visualize this as painting the color through
 *          the mask, as a stencil.
 *      (5) This uses rasterop to handle clipping and different depths of pixd.
 *      (6) If pixd has a colormap, you should call pixPaintThroughMask().
 *      (7) Why is this function here, if pixPaintThroughMask() does the
 *          same thing, and does it more generally?  I've retained it here
 *          to show how one can paint through a mask using only full
 *          image rasterops, rather than pixel peeking in pixm and poking
 *          in pixd.  It's somewhat baroque, but I found it amusing.
 */
l_int32
pixSetMaskedGeneral(PIX      *pixd,
                    PIX      *pixm,
                    l_uint32  val,
                    l_int32   x,
                    l_int32   y)
{
l_int32    wm, hm, d;
PIX       *pixmu, *pixc;

    PROCNAME("pixSetMaskedGeneral");

    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (!pixm)  /* nothing to do */
        return 0;

    d = pixGetDepth(pixd);
    if (d != 8 && d != 16 && d != 32)
        return ERROR_INT("pixd not 8, 16 or 32 bpp", procName, 1);
    if (pixGetDepth(pixm) != 1)
        return ERROR_INT("pixm not 1 bpp", procName, 1);

        /* Unpack binary to depth d, with inversion:  1 --> 0, 0 --> 0xff... */
    if ((pixmu = pixUnpackBinary(pixm, d, 1)) == NULL)
        return ERROR_INT("pixmu not made", procName, 1);

        /* Clear stenciled pixels in pixd */
    pixGetDimensions(pixm, &wm, &hm, NULL);
    pixRasterop(pixd, x, y, wm, hm, PIX_SRC & PIX_DST, pixmu, 0, 0);

        /* Generate image with requisite color */
    if ((pixc = pixCreateTemplate(pixmu)) == NULL)
        return ERROR_INT("pixc not made", procName, 1);
    pixSetAllArbitrary(pixc, val);

        /* Invert stencil mask, and paint color color into stencil */
    pixInvert(pixmu, pixmu);
    pixAnd(pixmu, pixmu, pixc);

        /* Finally, repaint stenciled pixels, with val, in pixd */
    pixRasterop(pixd, x, y, wm, hm, PIX_SRC | PIX_DST, pixmu, 0, 0);

    pixDestroy(&pixmu);
    pixDestroy(&pixc);
    return 0;
}


/*!
 *  pixCombineMasked()
 *
 *      Input:  pixd (1 bpp, 8 bpp gray or 32 bpp rgb; no cmap)
 *              pixs (1 bpp, 8 bpp gray or 32 bpp rgb; no cmap)
 *              pixm (<optional> 1 bpp mask; no operation if NULL)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) In-place operation; pixd is changed.
 *      (2) This sets each pixel in pixd that co-locates with an ON
 *          pixel in pixm to the corresponding value of pixs.
 *      (3) pixs and pixd must be the same depth and not colormapped.
 *      (4) All three input pix are aligned at the UL corner, and the
 *          operation is clipped to the intersection of all three images.
 *      (5) If pixm == NULL, it's a no-op.
 *      (6) Implementation: see notes in pixCombineMaskedGeneral().
 *          For 8 bpp selective masking, you might guess that it
 *          would be faster to generate an 8 bpp version of pixm,
 *          using pixConvert1To8(pixm, 0, 255), and then use a
 *          general combine operation
 *               d = (d & ~m) | (s & m)
 *          on a word-by-word basis.  Not always.  The word-by-word
 *          combine takes a time that is independent of the mask data.
 *          If the mask is relatively sparse, the byte-check method
 *          is actually faster!
 */
l_int32
pixCombineMasked(PIX  *pixd,
                 PIX  *pixs,
                 PIX  *pixm)
{
l_int32    w, h, d, ws, hs, ds, wm, hm, dm, wmin, hmin;
l_int32    wpl, wpls, wplm, i, j, val;
l_uint32  *data, *datas, *datam, *line, *lines, *linem;
PIX       *pixt;

    PROCNAME("pixCombineMasked");

    if (!pixm)  /* nothing to do */
        return 0;
    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    pixGetDimensions(pixd, &w, &h, &d);
    pixGetDimensions(pixs, &ws, &hs, &ds);
    pixGetDimensions(pixm, &wm, &hm, &dm);
    if (d != ds)
        return ERROR_INT("pixs and pixd depths differ", procName, 1);
    if (dm != 1)
        return ERROR_INT("pixm not 1 bpp", procName, 1);
    if (d != 1 && d != 8 && d != 32)
        return ERROR_INT("pixd not 1, 8 or 32 bpp", procName, 1);
    if (pixGetColormap(pixd) || pixGetColormap(pixs))
        return ERROR_INT("pixs and/or pixd is cmapped", procName, 1);

        /* For d = 1, use rasterop.  pixt is the part from pixs, under
         * the fg of pixm, that is to be combined with pixd.  We also
         * use pixt to remove all fg of pixd that is under the fg of pixm.
         * Then pixt and pixd are combined by ORing. */
    wmin = L_MIN(w, L_MIN(ws, wm));
    hmin = L_MIN(h, L_MIN(hs, hm));
    if (d == 1) {
        pixt = pixAnd(NULL, pixs, pixm);
        pixRasterop(pixd, 0, 0, wmin, hmin, PIX_DST & PIX_NOT(PIX_SRC),
                    pixm, 0, 0);
        pixRasterop(pixd, 0, 0, wmin, hmin, PIX_SRC | PIX_DST, pixt, 0, 0);
        pixDestroy(&pixt);
        return 0;
    }

    data = pixGetData(pixd);
    datas = pixGetData(pixs);
    datam = pixGetData(pixm);
    wpl = pixGetWpl(pixd);
    wpls = pixGetWpl(pixs);
    wplm = pixGetWpl(pixm);
    if (d == 8) {
        for (i = 0; i < hmin; i++) {
            line = data + i * wpl;
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = 0; j < wmin; j++) {
                if (GET_DATA_BIT(linem, j)) {
                   val = GET_DATA_BYTE(lines, j);
                   SET_DATA_BYTE(line, j, val);
                }
            }
        }
    }
    else {  /* d == 32 */
        for (i = 0; i < hmin; i++) {
            line = data + i * wpl;
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = 0; j < wmin; j++) {
                if (GET_DATA_BIT(linem, j))
                   line[j] = lines[j];
            }
        }
    }

    return 0;
}


/*!
 *  pixCombineMaskedGeneral()
 *
 *      Input:  pixd (1 bpp, 8 bpp gray or 32 bpp rgb)
 *              pixs (1 bpp, 8 bpp gray or 32 bpp rgb)
 *              pixm (<optional> 1 bpp mask)
 *              x, y (origin of pixs and pixm relative to pixd; can be negative)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) In-place operation; pixd is changed.
 *      (2) This is a generalized version of pixCombinedMasked(), where
 *          the source and mask can be placed at the same (arbitrary)
 *          location relative to pixd.
 *      (3) pixs and pixd must be the same depth and not colormapped.
 *      (4) The UL corners of both pixs and pixm are aligned with
 *          the point (x, y) of pixd, and the operation is clipped to
 *          the intersection of all three images.
 *      (5) If pixm == NULL, it's a no-op.
 *      (6) Implementation.  There are two ways to do these.  In the first,
 *          we use rasterop, ORing the part of pixs under the mask
 *          with pixd (which has been appropriately cleared there first).
 *          In the second, the mask is used one pixel at a time to
 *          selectively replace pixels of pixd with those of pixs.
 *          Here, we use rasterop for 1 bpp and pixel-wise replacement
 *          for 8 and 32 bpp.  To use rasterop for 8 bpp, for example,
 *          we must first generate an 8 bpp version of the mask.
 *          The code is simple:
 *
 *             Pix *pixm8 = pixConvert1To8(NULL, pixm, 0, 255);
 *             Pix *pixt = pixAnd(NULL, pixs, pixm8);
 *             pixRasterop(pixd, x, y, wmin, hmin, PIX_DST & PIX_NOT(PIX_SRC),
 *                         pixm8, 0, 0);
 *             pixRasterop(pixd, x, y, wmin, hmin, PIX_SRC | PIX_DST,
 *                         pixt, 0, 0);
 *             pixDestroy(&pixt);
 *             pixDestroy(&pixm8);
 */
l_int32
pixCombineMaskedGeneral(PIX      *pixd,
                        PIX      *pixs,
                        PIX      *pixm,
                        l_int32   x,
                        l_int32   y)
{
l_int32    d, w, h, ws, hs, ds, wm, hm, dm, wmin, hmin;
l_int32    wpl, wpls, wplm, i, j, val;
l_uint32  *data, *datas, *datam, *line, *lines, *linem;
PIX       *pixt;

    PROCNAME("pixCombineMaskedGeneral");

    if (!pixm)  /* nothing to do */
        return 0;
    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    pixGetDimensions(pixd, &w, &h, &d);
    pixGetDimensions(pixs, &ws, &hs, &ds);
    pixGetDimensions(pixm, &wm, &hm, &dm);
    if (d != ds)
        return ERROR_INT("pixs and pixd depths differ", procName, 1);
    if (dm != 1)
        return ERROR_INT("pixm not 1 bpp", procName, 1);
    if (d != 1 && d != 8 && d != 32)
        return ERROR_INT("pixd not 1, 8 or 32 bpp", procName, 1);
    if (pixGetColormap(pixd) || pixGetColormap(pixs))
        return ERROR_INT("pixs and/or pixd is cmapped", procName, 1);

        /* For d = 1, use rasterop.  pixt is the part from pixs, under
         * the fg of pixm, that is to be combined with pixd.  We also
         * use pixt to remove all fg of pixd that is under the fg of pixm.
         * Then pixt and pixd are combined by ORing. */
    wmin = L_MIN(ws, wm);
    hmin = L_MIN(hs, hm);
    if (d == 1) {
        pixt = pixAnd(NULL, pixs, pixm);
        pixRasterop(pixd, x, y, wmin, hmin, PIX_DST & PIX_NOT(PIX_SRC),
                    pixm, 0, 0);
        pixRasterop(pixd, x, y, wmin, hmin, PIX_SRC | PIX_DST, pixt, 0, 0);
        pixDestroy(&pixt);
        return 0;
    }

    wpl = pixGetWpl(pixd);
    data = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    datas = pixGetData(pixs);
    wplm = pixGetWpl(pixm);
    datam = pixGetData(pixm);

    for (i = 0; i < hmin; i++) {
        if (y + i < 0 || y + i >= h) continue;
        line = data + (y + i) * wpl;
        lines = datas + i * wpls;
        linem = datam + i * wplm;
        for (j = 0; j < wmin; j++) {
            if (x + j < 0 || x + j >= w) continue;
            if (GET_DATA_BIT(linem, j)) {
                switch (d)
                {
                case 8:
                    val = GET_DATA_BYTE(lines, j);
                    SET_DATA_BYTE(line, x + j, val);
                    break;
                case 32:
                    *(line + x + j) = *(lines + j);
                    break;
                default:
                    return ERROR_INT("shouldn't get here", procName, 1);
                }
            }
        }
    }

    return 0;
}


/*!
 *  pixPaintThroughMask()
 *
 *      Input:  pixd (1, 2, 4, 8, 16 or 32 bpp; or colormapped)
 *              pixm (<optional> 1 bpp mask)
 *              x, y (origin of pixm relative to pixd; can be negative)
 *              val (pixel value to set at each masked pixel)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) In-place operation.  Calls pixSetMaskedCmap() for colormapped
 *          images.
 *      (2) For 1, 2, 4, 8 and 16 bpp gray, we take the appropriate
 *          number of least significant bits of val.
 *      (3) If pixm == NULL, it's a no-op.
 *      (4) The mask origin is placed at (x,y) on pixd, and the
 *          operation is clipped to the intersection of rectangles.
 *      (5) For rgb, the components in val are in the canonical locations,
 *          with red in location COLOR_RED, etc.
 *      (6) Implementation detail 1:
 *          For painting with val == 0 or val == maxval, you can use rasterop.
 *          If val == 0, invert the mask so that it's 0 over the region
 *          into which you want to write, and use PIX_SRC & PIX_DST to
 *          clear those pixels.  To write with val = maxval (all 1's),
 *          use PIX_SRC | PIX_DST to set all bits under the mask.
 *      (7) Implementation detail 2:
 *          The rasterop trick can be used for depth > 1 as well.
 *          For val == 0, generate the mask for depth d from the binary
 *          mask using
 *              pixmd = pixUnpackBinary(pixm, d, 1);
 *          and use pixRasterop() with PIX_MASK.  For val == maxval,
 *              pixmd = pixUnpackBinary(pixm, d, 0);
 *          and use pixRasterop() with PIX_PAINT.
 *          But note that if d == 32 bpp, it is about 3x faster to use
 *          the general implementation (not pixRasterop()).
 *      (8) Implementation detail 3:
 *          It might be expected that the switch in the inner loop will
 *          cause large branching delays and should be avoided.
 *          This is not the case, because the entrance is always the
 *          same and the compiler can correctly predict the jump.
 */
l_int32
pixPaintThroughMask(PIX      *pixd,
                    PIX      *pixm,
                    l_int32   x,
                    l_int32   y,
                    l_uint32  val)
{
l_int32    d, w, h, wm, hm, wpl, wplm, i, j, rval, gval, bval;
l_uint32  *data, *datam, *line, *linem;

    PROCNAME("pixPaintThroughMask");

    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (!pixm)  /* nothing to do */
        return 0;
    if (pixGetColormap(pixd)) {
        extractRGBValues(val, &rval, &gval, &bval);
        return pixSetMaskedCmap(pixd, pixm, x, y, rval, gval, bval);
    }

    if (pixGetDepth(pixm) != 1)
        return ERROR_INT("pixm not 1 bpp", procName, 1);
    d = pixGetDepth(pixd);
    if (d == 1)
        val &= 1;
    else if (d == 2)
        val &= 3;
    else if (d == 4)
        val &= 0x0f;
    else if (d == 8)
        val &= 0xff;
    else if (d == 16)
        val &= 0xffff;
    else if (d != 32)
        return ERROR_INT("pixd not 1, 2, 4, 8, 16 or 32 bpp", procName, 1);
    pixGetDimensions(pixm, &wm, &hm, NULL);

        /* If d == 1, use rasterop; it's about 25x faster. */
    if (d == 1) {
        if (val == 0) {
            PIX *pixmi = pixInvert(NULL, pixm);
            pixRasterop(pixd, x, y, wm, hm, PIX_MASK, pixmi, 0, 0);
            pixDestroy(&pixmi);
        }
        else  /* val == 1 */
            pixRasterop(pixd, x, y, wm, hm, PIX_PAINT, pixm, 0, 0);
        return 0;
    }
    
        /* For d < 32, use rasterop if val == 0 (black); ~3x faster. */
    if (d < 32 && val == 0) {
        PIX *pixmd = pixUnpackBinary(pixm, d, 1);
        pixRasterop(pixd, x, y, wm, hm, PIX_MASK, pixmd, 0, 0);
        pixDestroy(&pixmd);
        return 0;
    }

        /* For d < 32, use rasterop if val == maxval (white); ~3x faster. */
    if (d < 32 && val == ((1 << d) - 1)) {
        PIX *pixmd = pixUnpackBinary(pixm, d, 0);
        pixRasterop(pixd, x, y, wm, hm, PIX_PAINT, pixmd, 0, 0);
        pixDestroy(&pixmd);
        return 0;
    }

        /* All other cases */
    pixGetDimensions(pixd, &w, &h, NULL);
    wpl = pixGetWpl(pixd);
    data = pixGetData(pixd);
    wplm = pixGetWpl(pixm);
    datam = pixGetData(pixm);
    for (i = 0; i < hm; i++) {
        if (y + i < 0 || y + i >= h) continue;
        line = data + (y + i) * wpl;
        linem = datam + i * wplm;
        for (j = 0; j < wm; j++) {
            if (x + j < 0 || x + j >= w) continue;
            if (GET_DATA_BIT(linem, j)) {
                switch (d)
                {
                case 2:
                    SET_DATA_DIBIT(line, x + j, val);
                    break;
                case 4:
                    SET_DATA_QBIT(line, x + j, val);
                    break;
                case 8:
                    SET_DATA_BYTE(line, x + j, val);
                    break;
                case 16:
                    SET_DATA_TWO_BYTES(line, x + j, val);
                    break;
                case 32:
                    *(line + x + j) = val;
                    break;
                default:
                    return ERROR_INT("shouldn't get here", procName, 1);
                }
            }
        }
    }

    return 0;
}
    

/*!
 *  pixPaintSelfThroughMask()
 *
 *      Input:  pixd (8 bpp gray or 32 bpp rgb; not colormapped)
 *              pixm (1 bpp mask)
 *              x, y (origin of pixm relative to pixd; must not be negative)
 *              tilesize (requested size for tiling)
 *              searchdir (L_HORIZ, L_VERT)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) In-place operation; pixd is changed.
 *      (2) If pixm == NULL, it's a no-op.
 *      (3) The mask origin is placed at (x,y) on pixd, and the
 *          operation is clipped to the intersection of pixd and the
 *          fg of the mask.
 *      (4) The tilesize is the the requested size for tiling.  The
 *          actual size for each c.c. will be bounded by the minimum
 *          dimension of the c.c. and the distance at which the tile
 *          center is located.
 *      (5) searchdir is the direction with respect to the b.b. of each
 *          mask component, from which the square patch is chosen and
 *          tiled onto the image, clipped by the mask component.
 *      (6) Specifically, a mirrored tiling, generated from pixd,
 *          is used to construct the pixels that are painted onto
 *          pixd through pixm.
 */
l_int32
pixPaintSelfThroughMask(PIX      *pixd,
                        PIX      *pixm,
                        l_int32   x,
                        l_int32   y,
                        l_int32   tilesize,
                        l_int32   searchdir)
{
l_int32   w, h, d, wm, hm, dm, i, n, xc, yc, bx, by, bw, bh;
l_int32   depth, cctilesize;
l_uint32  dist, minside, retval;
BOX      *box, *boxt;
BOXA     *boxa;
PIX      *pix, *pixf, *pixdf, *pixt, *pixc;
PIXA     *pixa;

    PROCNAME("pixPaintSelfThroughMask");

    if (!pixm)  /* nothing to do */
        return 0;
    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (pixGetColormap(pixd) != NULL)
        return ERROR_INT("pixd has colormap", procName, 1);
    pixGetDimensions(pixd, &w, &h, &d);
    if (d != 8 && d != 32)
        return ERROR_INT("pixd not 8 or 32 bpp", procName, 1);
    pixGetDimensions(pixm, &wm, &hm, &dm);
    if (dm != 1)
        return ERROR_INT("pixm not 1 bpp", procName, 1);
    if (x < 0 || y < 0)
        return ERROR_INT("x and y must be non-negative", procName, 1);
    if (tilesize < 1)
        return ERROR_INT("tilesize must be >= 1", procName, 1);
    if (searchdir != L_HORIZ && searchdir != L_VERT)
        return ERROR_INT("searchdir not in {L_HORIZ, L_VERT}", procName, 1);

        /* Embed mask in full sized mask */
    if (wm < w || hm < h) {
        pixf = pixCreate(w, h, 1);
        pixRasterop(pixf, x, y, wm, hm, PIX_SRC, pixm, 0, 0);
    }
    else
        pixf = pixCopy(NULL, pixm);

        /* Get connected components of mask */
    boxa = pixConnComp(pixf, &pixa, 8);
    if ((n = pixaGetCount(pixa)) == 0) {
        L_WARNING("no fg in mask", procName);
        pixDestroy(&pixf);
        pixaDestroy(&pixa);
        boxaDestroy(&boxa);
        return 1;
    }

        /* Get distance function for the mask */
    pixInvert(pixf, pixf);
    depth = (tilesize < 256) ? 8 : 16;
    pixdf = pixDistanceFunction(pixf, 4, depth, L_BOUNDARY_BG);
    pixDestroy(&pixf);

        /* For each c.c., generate a representative tile for texturizing
         * and apply it through the mask.  The input 'tilesize' is the
         * requested value.  findTilePatchCenter() returns the distance
         * at which this patch can safely be found. */
    retval = 0;
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
        box = pixaGetBox(pixa, i, L_CLONE);
        boxGetGeometry(box, &bx, &by, &bw, &bh);
	minside = L_MIN(bw, bh);

        findTilePatchCenter(pixdf, box, searchdir, L_MIN(minside, tilesize),
                            &dist, &xc, &yc);
        cctilesize = L_MIN(tilesize, dist);  /* for this c.c. */
        if (cctilesize < 1) {
            L_WARNING("region not found!", procName);
            pixDestroy(&pix);
            boxDestroy(&box);
            retval = 1;
            continue;
        }

            /* Extract the selected square from pixd, and generate 
             * an image the size of the b.b. of the c.c., which
             * is then painted through the c.c. mask.  */
        boxt = boxCreate(L_MAX(0, xc - dist / 2), L_MAX(0, yc - dist / 2),
                         cctilesize, cctilesize);
        pixt = pixClipRectangle(pixd, boxt, NULL);
        pixc = pixMirroredTiling(pixt, bw, bh);
        pixCombineMaskedGeneral(pixd, pixc, pix, bx, by);
        pixDestroy(&pix);
        pixDestroy(&pixt);
        pixDestroy(&pixc);
        boxDestroy(&box);
        boxDestroy(&boxt);
    }

    pixDestroy(&pixdf);
    pixaDestroy(&pixa);
    boxaDestroy(&boxa);
    return retval;
}


/*!
 *  pixMakeMaskFromLUT()
 *
 *      Input:  pixs (2, 4 or 8 bpp; can be colormapped)
 *              tab (256-entry LUT; 1 means to write to mask)
 *      Return: pixd (1 bpp mask), or null on error
 *
 *  Notes:
 *      (1) This generates a 1 bpp mask image, where a 1 is written in
 *          the mask for each pixel in pixs that has a value corresponding
 *          to a 1 in the LUT.
 *      (2) The LUT should be of size 256.
 */
PIX *
pixMakeMaskFromLUT(PIX      *pixs,
                   l_int32  *tab)
{
l_int32    w, h, d, i, j, val, wpls, wpld;
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixMakeMaskFromLUT");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!tab)
        return (PIX *)ERROR_PTR("tab not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 2 && d != 4 && d != 8)
        return (PIX *)ERROR_PTR("pix not 2, 4 or 8 bpp", procName, NULL);

    pixd = pixCreate(w, h, 1);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            if (d == 2)
                val = GET_DATA_DIBIT(lines, j);
            else if (d == 4)
                val = GET_DATA_QBIT(lines, j);
            else  /* d == 8 */
                val = GET_DATA_BYTE(lines, j);
            if (tab[val] == 1)
                SET_DATA_BIT(lined, j);
        }
    }

    return pixd;
}


/*!
 *  pixSetUnderTransparency()
 *
 *      Input:  pixs (32 bpp rgba)
 *              val (32 bit unsigned color to use where alpha == 0)
 *              debugflag (generates intermediate images)
 *      Return: pixd (32 bpp rgba), or null on error
 *
 *  Notes:
 *      (1) This is one of the few operations in leptonica that uses
 *          the alpha blending component in rgba images.  It sets
 *          the r, g and b components under every fully transparent alpha
 *          component to @val.
 *      (2) Full transparency is denoted by alpha == 0.  By setting
 *          all pixels to @val where alpha == 0, this can improve
 *          compressibility by reducing the entropy.
 *      (3) The visual result depends on how the image is displayed.
 *          (a) For display devices that respect the use of the alpha
 *              layer, this will not affect the appearance.
 *          (b) For typical leptonica operations, alpha is ignored,
 *              so there will be a change in appearance because this
 *              resets the rgb values in the fully transparent region.
 *      (4) For reading and writing rgba pix in png format, use
 *          pixReadRGBAPng() and pixWriteRGBAPng().
 *      (5) For example, if you want to rewrite all fully transparent
 *          pixels in a png file to white:
 *              pixs = pixReadRGBAPng(<infile>);  // special read
 *              pixd = pixSetUnderTransparency(pixs, 0xffffff00, 0);
 *          Then either use a normal write if you won't be using transparency:
 *              pixWrite(<outfile>, pixd, IFF_PNG);
 *          or an RGBA write if you want to preserve the transparency layer
 *              pixWriteRGBAPng(<outfile>, pixd);  // special write
 *      (6) Caution.  Because rgb images in leptonica typically
 *          have value 0 in the alpha channel, this function would
 *          interpret the entire image as fully transparent, and set
 *          every pixel to @val.  Because this is not desirable, instead
 *          we issue a warning and return a copy of the input pix.
 *          If you really want to set every pixel to the same value,
 *          use pixSetAllArbitrary().
 */
PIX *
pixSetUnderTransparency(PIX      *pixs,
                        l_uint32  val,
                        l_int32   debugflag)
{
l_int32   isblack, rval, gval, bval;
PIX      *pixr, *pixg, *pixb, *pixalpha, *pixm, *pixt, *pixd;
PIXA     *pixa;

    PROCNAME("pixSetUnderTransparency");

    if (!pixs || pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs not defined or not 32 bpp",
                                procName, NULL);

    pixalpha = pixGetRGBComponent(pixs, L_ALPHA_CHANNEL);
    pixZero(pixalpha, &isblack);
    if (isblack) {
        L_WARNING(
            "alpha channel is fully transparent; likely invalid; ignoring",
            procName);
        pixDestroy(&pixalpha);
        return pixCopy(NULL, pixs);
    }
    pixr = pixGetRGBComponent(pixs, COLOR_RED);
    pixg = pixGetRGBComponent(pixs, COLOR_GREEN);
    pixb = pixGetRGBComponent(pixs, COLOR_BLUE);

        /* Make a mask from the alpha component with ON pixels
         * wherever the alpha component is fully transparent (0).
         * One can do this:
         *     l_int32 *lut = (l_int32 *)CALLOC(256, sizeof(l_int32));
         *     lut[0] = 1;
         *     pixm = pixMakeMaskFromLUT(pixalpha, lut);
         *     FREE(lut);
         * But there's an easier way to set pixels in a mask where
         * the alpha component is 0 ...  */
    pixm = pixThresholdToBinary(pixalpha, 1);

    if (debugflag) {
        pixa = pixaCreate(0);
        pixSaveTiled(pixs, pixa, 1, 1, 20, 32);
        pixSaveTiled(pixm, pixa, 1, 0, 20, 0);
        pixSaveTiled(pixr, pixa, 1, 1, 20, 0);
        pixSaveTiled(pixg, pixa, 1, 0, 20, 0);
        pixSaveTiled(pixb, pixa, 1, 0, 20, 0);
        pixSaveTiled(pixalpha, pixa, 1, 0, 20, 0);
    }

        /* Clean each component and reassemble */
    extractRGBValues(val, &rval, &gval, &bval);
    pixSetMasked(pixr, pixm, rval);
    pixSetMasked(pixg, pixm, gval);
    pixSetMasked(pixb, pixm, bval);
    pixd = pixCreateRGBImage(pixr, pixg, pixb);
    pixSetRGBComponent(pixd, pixalpha, L_ALPHA_CHANNEL);

    if (debugflag) {
        pixSaveTiled(pixr, pixa, 1, 1, 20, 0);
        pixSaveTiled(pixg, pixa, 1, 0, 20, 0);
        pixSaveTiled(pixb, pixa, 1, 0, 20, 0);
        pixSaveTiled(pixd, pixa, 1, 1, 20, 0);
        pixt = pixaDisplay(pixa, 0, 0);
        pixWriteTempfile("/tmp", "rgb.png", pixt, IFF_PNG, NULL);
        pixDestroy(&pixt);
        pixaDestroy(&pixa);
    }

    pixDestroy(&pixr);
    pixDestroy(&pixg);
    pixDestroy(&pixb);
    pixDestroy(&pixm);
    pixDestroy(&pixalpha);
    return pixd;
}


/*-------------------------------------------------------------*
 *    One and two-image boolean ops on arbitrary depth images  *
 *-------------------------------------------------------------*/
/*!
 *  pixInvert()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This inverts pixs, for all pixel depths.
 *      (2) There are 3 cases:
 *           (a) pixd == null,   ~src --> new pixd
 *           (b) pixd == pixs,   ~src --> src  (in-place)
 *           (c) pixd != pixs,   ~src --> input pixd
 *      (3) For clarity, if the case is known, use these patterns:
 *           (a) pixd = pixInvert(NULL, pixs);
 *           (b) pixInvert(pixs, pixs);
 *           (c) pixInvert(pixd, pixs);
 */
PIX *
pixInvert(PIX  *pixd,
          PIX  *pixs)
{
    PROCNAME("pixInvert");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

        /* Prepare pixd for in-place operation */
    if ((pixd = pixCopy(pixd, pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

    pixRasterop(pixd, 0, 0, pixGetWidth(pixd), pixGetHeight(pixd),
                PIX_NOT(PIX_DST), NULL, 0, 0);   /* invert pixd */

    return pixd;
}


/*!
 *  pixOr()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs1,
 *                     different from pixs1)
 *              pixs1 (can be == pixd)
 *              pixs2 (must be != pixd)
 *      Return: pixd always
 *
 *  Notes:
 *      (1) This gives the union of two images with equal depth,
 *          aligning them to the the UL corner.  pixs1 and pixs2
 *          need not have the same width and height.
 *      (2) There are 3 cases:
 *            (a) pixd == null,   (src1 | src2) --> new pixd
 *            (b) pixd == pixs1,  (src1 | src2) --> src1  (in-place)
 *            (c) pixd != pixs1,  (src1 | src2) --> input pixd
 *      (3) For clarity, if the case is known, use these patterns:
 *            (a) pixd = pixOr(NULL, pixs1, pixs2);
 *            (b) pixOr(pixs1, pixs1, pixs2);
 *            (c) pixOr(pixd, pixs1, pixs2);
 *      (4) The size of the result is determined by pixs1.
 *      (5) The depths of pixs1 and pixs2 must be equal.
 *      (6) Note carefully that the order of pixs1 and pixs2 only matters
 *          for the in-place case.  For in-place, you must have
 *          pixd == pixs1.  Setting pixd == pixs2 gives an incorrect
 *          result: the copy puts pixs1 image data in pixs2, and
 *          the rasterop is then between pixs2 and pixs2 (a no-op).
 */
PIX *
pixOr(PIX  *pixd,
      PIX  *pixs1,
      PIX  *pixs2)
{
    PROCNAME("pixOr");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, pixd);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, pixd);
    if (pixd == pixs2)
        return (PIX *)ERROR_PTR("cannot have pixs2 == pixd", procName, pixd);
    if (pixGetDepth(pixs1) != pixGetDepth(pixs2))
        return (PIX *)ERROR_PTR("depths of pixs* unequal", procName, pixd);

#if  EQUAL_SIZE_WARNING
    if (!pixSizesEqual(pixs1, pixs2))
        L_WARNING("pixs1 and pixs2 not equal sizes", procName);
#endif  /* EQUAL_SIZE_WARNING */

        /* Prepare pixd to be a copy of pixs1 */
    if ((pixd = pixCopy(pixd, pixs1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, pixd);

        /* src1 | src2 --> dest */
    pixRasterop(pixd, 0, 0, pixGetWidth(pixd), pixGetHeight(pixd),
                PIX_SRC | PIX_DST, pixs2, 0, 0);

    return pixd;
}


/*!
 *  pixAnd()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs1,
 *                     different from pixs1)
 *              pixs1 (can be == pixd)
 *              pixs2 (must be != pixd)
 *      Return: pixd always
 *
 *  Notes:
 *      (1) This gives the intersection of two images with equal depth,
 *          aligning them to the the UL corner.  pixs1 and pixs2 
 *          need not have the same width and height.
 *      (2) There are 3 cases:
 *            (a) pixd == null,   (src1 & src2) --> new pixd
 *            (b) pixd == pixs1,  (src1 & src2) --> src1  (in-place)
 *            (c) pixd != pixs1,  (src1 & src2) --> input pixd
 *      (3) For clarity, if the case is known, use these patterns:
 *            (a) pixd = pixAnd(NULL, pixs1, pixs2);
 *            (b) pixAnd(pixs1, pixs1, pixs2);
 *            (c) pixAnd(pixd, pixs1, pixs2);
 *      (4) The size of the result is determined by pixs1.
 *      (5) The depths of pixs1 and pixs2 must be equal.
 *      (6) Note carefully that the order of pixs1 and pixs2 only matters
 *          for the in-place case.  For in-place, you must have
 *          pixd == pixs1.  Setting pixd == pixs2 gives an incorrect
 *          result: the copy puts pixs1 image data in pixs2, and
 *          the rasterop is then between pixs2 and pixs2 (a no-op).
 */
PIX *
pixAnd(PIX  *pixd,
       PIX  *pixs1,
       PIX  *pixs2)
{
    PROCNAME("pixAnd");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, pixd);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, pixd);
    if (pixd == pixs2)
        return (PIX *)ERROR_PTR("cannot have pixs2 == pixd", procName, pixd);
    if (pixGetDepth(pixs1) != pixGetDepth(pixs2))
        return (PIX *)ERROR_PTR("depths of pixs* unequal", procName, pixd);

#if  EQUAL_SIZE_WARNING
    if (!pixSizesEqual(pixs1, pixs2))
        L_WARNING("pixs1 and pixs2 not equal sizes", procName);
#endif  /* EQUAL_SIZE_WARNING */

        /* Prepare pixd to be a copy of pixs1 */
    if ((pixd = pixCopy(pixd, pixs1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, pixd);

        /* src1 & src2 --> dest */
    pixRasterop(pixd, 0, 0, pixGetWidth(pixd), pixGetHeight(pixd),
                PIX_SRC & PIX_DST, pixs2, 0, 0);

    return pixd;
}


/*!
 *  pixXor()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs1,
 *                     different from pixs1)
 *              pixs1 (can be == pixd)
 *              pixs2 (must be != pixd)
 *      Return: pixd always
 *
 *  Notes:
 *      (1) This gives the XOR of two images with equal depth,
 *          aligning them to the the UL corner.  pixs1 and pixs2
 *          need not have the same width and height.
 *      (2) There are 3 cases:
 *            (a) pixd == null,   (src1 ^ src2) --> new pixd
 *            (b) pixd == pixs1,  (src1 ^ src2) --> src1  (in-place)
 *            (c) pixd != pixs1,  (src1 ^ src2) --> input pixd
 *      (3) For clarity, if the case is known, use these patterns:
 *            (a) pixd = pixXor(NULL, pixs1, pixs2);
 *            (b) pixXor(pixs1, pixs1, pixs2);
 *            (c) pixXor(pixd, pixs1, pixs2);
 *      (4) The size of the result is determined by pixs1.
 *      (5) The depths of pixs1 and pixs2 must be equal.
 *      (6) Note carefully that the order of pixs1 and pixs2 only matters
 *          for the in-place case.  For in-place, you must have
 *          pixd == pixs1.  Setting pixd == pixs2 gives an incorrect
 *          result: the copy puts pixs1 image data in pixs2, and
 *          the rasterop is then between pixs2 and pixs2 (a no-op).
 */
PIX *
pixXor(PIX  *pixd,
       PIX  *pixs1,
       PIX  *pixs2)
{
    PROCNAME("pixXor");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, pixd);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, pixd);
    if (pixd == pixs2)
        return (PIX *)ERROR_PTR("cannot have pixs2 == pixd", procName, pixd);
    if (pixGetDepth(pixs1) != pixGetDepth(pixs2))
        return (PIX *)ERROR_PTR("depths of pixs* unequal", procName, pixd);

#if  EQUAL_SIZE_WARNING
    if (!pixSizesEqual(pixs1, pixs2))
        L_WARNING("pixs1 and pixs2 not equal sizes", procName);
#endif  /* EQUAL_SIZE_WARNING */

        /* Prepare pixd to be a copy of pixs1 */
    if ((pixd = pixCopy(pixd, pixs1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, pixd);

        /* src1 ^ src2 --> dest */
    pixRasterop(pixd, 0, 0, pixGetWidth(pixd), pixGetHeight(pixd),
                PIX_SRC ^ PIX_DST, pixs2, 0, 0);

    return pixd;
}


/*!
 *  pixSubtract()
 *
 *      Input:  pixd  (<optional>; this can be null, equal to pixs1,
 *                     equal to pixs2, or different from both pixs1 and pixs2)
 *              pixs1 (can be == pixd)
 *              pixs2 (can be == pixd)
 *      Return: pixd always
 *
 *  Notes:
 *      (1) This gives the set subtraction of two images with equal depth,
 *          aligning them to the the UL corner.  pixs1 and pixs2
 *          need not have the same width and height.
 *      (2) Source pixs2 is always subtracted from source pixs1.
 *          The result is
 *                  pixs1 \ pixs2 = pixs1 & (~pixs2)
 *      (3) There are 4 cases:
 *            (a) pixd == null,   (src1 - src2) --> new pixd
 *            (b) pixd == pixs1,  (src1 - src2) --> src1  (in-place)
 *            (c) pixd == pixs2,  (src1 - src2) --> src2  (in-place)
 *            (d) pixd != pixs1 && pixd != pixs2),
 *                                 (src1 - src2) --> input pixd
 *      (4) For clarity, if the case is known, use these patterns:
 *            (a) pixd = pixSubtract(NULL, pixs1, pixs2);
 *            (b) pixSubtract(pixs1, pixs1, pixs2);
 *            (c) pixSubtract(pixs2, pixs1, pixs2);
 *            (d) pixSubtract(pixd, pixs1, pixs2);
 *      (5) The size of the result is determined by pixs1.
 *      (6) The depths of pixs1 and pixs2 must be equal.
 */
PIX *
pixSubtract(PIX  *pixd,
            PIX  *pixs1,
            PIX  *pixs2)
{
l_int32  w, h;

    PROCNAME("pixSubtract");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, pixd);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, pixd);
    if (pixGetDepth(pixs1) != pixGetDepth(pixs2))
        return (PIX *)ERROR_PTR("depths of pixs* unequal", procName, pixd);

#if  EQUAL_SIZE_WARNING
    if (!pixSizesEqual(pixs1, pixs2))
        L_WARNING("pixs1 and pixs2 not equal sizes", procName);
#endif  /* EQUAL_SIZE_WARNING */

    pixGetDimensions(pixs1, &w, &h, NULL);
    if (!pixd) {
        pixd = pixCopy(NULL, pixs1);
        pixRasterop(pixd, 0, 0, w, h, PIX_DST & PIX_NOT(PIX_SRC),
            pixs2, 0, 0);   /* src1 & (~src2)  */
    }
    else if (pixd == pixs1) {
        pixRasterop(pixd, 0, 0, w, h, PIX_DST & PIX_NOT(PIX_SRC),
            pixs2, 0, 0);   /* src1 & (~src2)  */
    }
    else if (pixd == pixs2) {
        pixRasterop(pixd, 0, 0, w, h, PIX_NOT(PIX_DST) & PIX_SRC,
            pixs1, 0, 0);   /* src1 & (~src2)  */
    }
    else  { /* pixd != pixs1 && pixd != pixs2 */
        pixCopy(pixd, pixs1);  /* sizes pixd to pixs1 if unequal */
        pixRasterop(pixd, 0, 0, w, h, PIX_DST & PIX_NOT(PIX_SRC),
            pixs2, 0, 0);   /* src1 & (~src2)  */
    }

    return pixd;
}


/*-------------------------------------------------------------*
 *                         Pixel counting                      *
 *-------------------------------------------------------------*/
/*!
 *  pixZero()
 *
 *      Input:  pix (all depths; not colormapped)
 *              &empty  (<return> 1 if all bits in image are 0; 0 otherwise)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) For a binary image, if there are no fg (black) pixels, empty = 1.
 *      (2) For a grayscale image, if all pixels are black (0), empty = 1.
 *      (3) For an RGB image, if all 4 components in every pixel is 0,
 *          empty = 1. 
 */
l_int32
pixZero(PIX      *pix,
        l_int32  *pempty)
{
l_int32    w, h, wpl, i, j, fullwords, endbits;
l_uint32   endmask;
l_uint32  *data, *line;

    PROCNAME("pixZero");

    if (!pempty)
        return ERROR_INT("pempty not defined", procName, 1);
    *pempty = 1;
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (pixGetColormap(pix) != NULL)
        return ERROR_INT("pix is colormapped", procName, 1);

    w = pixGetWidth(pix) * pixGetDepth(pix);
    h = pixGetHeight(pix);
    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    fullwords = w / 32;
    endbits = w & 31;
    endmask = 0xffffffff << (32 - endbits);

    for (i = 0; i < h; i++) {
        line = data + wpl * i;
        for (j = 0; j < fullwords; j++)
            if (*line++) {
                *pempty = 0;
                return 0;
            }
        if (endbits) {
            if (*line & endmask) {
                *pempty = 0;
                return 0;
            }
        }
    }

    return 0;
}


/*!
 *  pixCountPixels()
 *
 *      Input:  pix (1 bpp)
 *              &count (<return> count of ON pixels)
 *              tab8  (<optional> 8-bit pixel lookup table)
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixCountPixels(PIX      *pix,
               l_int32  *pcount,
               l_int32  *tab8)
{
l_uint32   endmask;
l_int32    w, h, wpl, i, j;
l_int32    fullwords, endbits, sum;
l_int32   *tab;
l_uint32  *data;

    PROCNAME("pixCountPixels");

    if (!pcount)
        return ERROR_INT("pcount not defined", procName, 1);
    *pcount = 0;
    if (!pix || pixGetDepth(pix) != 1)
        return ERROR_INT("pix not defined or not 1 bpp", procName, 1);

    if (!tab8)
        tab = makePixelSumTab8();
    else
        tab = tab8;

    pixGetDimensions(pix, &w, &h, NULL);
    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    fullwords = w >> 5;
    endbits = w & 31;
    endmask = 0xffffffff << (32 - endbits);

    sum = 0;
    for (i = 0; i < h; i++, data += wpl) {
        for (j = 0; j < fullwords; j++) {
            l_uint32 word = data[j];
            if (word) {
                sum += tab[word & 0xff] +
                       tab[(word >> 8) & 0xff] +
                       tab[(word >> 16) & 0xff] +
                       tab[(word >> 24) & 0xff];
            }
        }
        if (endbits) {
            l_uint32 word = data[j] & endmask;
            if (word) {
                sum += tab[word & 0xff] +
                       tab[(word >> 8) & 0xff] +
                       tab[(word >> 16) & 0xff] +
                       tab[(word >> 24) & 0xff];
            }
        }
    }
    *pcount = sum;

    if (!tab8)
        FREE(tab);
    return 0;
}


/*!
 *  pixaCountPixels()
 *
 *      Input:  pixa (array of 1 bpp pix)
 *      Return: na of ON pixels in each pix, or null on error
 */
NUMA *
pixaCountPixels(PIXA  *pixa)
{
l_int32   d, i, n, count;
l_int32  *tab;
NUMA     *na;
PIX      *pix;

    PROCNAME("pixaCountPixels");

    if (!pixa)
        return (NUMA *)ERROR_PTR("pix not defined", procName, NULL);

    if ((n = pixaGetCount(pixa)) == 0)
        return numaCreate(1);

    pix = pixaGetPix(pixa, 0, L_CLONE);
    d = pixGetDepth(pix);
    pixDestroy(&pix);
    if (d != 1)
        return (NUMA *)ERROR_PTR("pixa not 1 bpp", procName, NULL);

    tab = makePixelSumTab8();
    if ((na = numaCreate(n)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
        pixCountPixels(pix, &count, tab);
        numaAddNumber(na, count);
        pixDestroy(&pix);
    }
        
    FREE(tab);
    return na;
}


/*!
 *  pixCountPixelsInRow()
 *
 *      Input:  pix (1 bpp)
 *              row number
 *              &count (<return> sum of ON pixels in raster line)
 *              tab8  (<optional> 8-bit pixel lookup table)
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixCountPixelsInRow(PIX      *pix,
                    l_int32   row,
                    l_int32  *pcount,
                    l_int32  *tab8)
{
l_uint32   word, endmask;
l_int32    j, w, h, wpl;
l_int32    fullwords, endbits, sum;
l_int32   *tab;
l_uint32  *line;

    PROCNAME("pixCountPixelsInRow");

    if (!pcount)
        return ERROR_INT("pcount not defined", procName, 1);
    *pcount = 0;
    if (!pix || pixGetDepth(pix) != 1)
        return ERROR_INT("pix not defined or not 1 bpp", procName, 1);

    pixGetDimensions(pix, &w, &h, NULL);
    if (row < 0 || row >= h)
        return ERROR_INT("row out of bounds", procName, 1);
    wpl = pixGetWpl(pix);
    line = pixGetData(pix) + row * wpl;
    fullwords = w >> 5;
    endbits = w & 31;
    endmask = 0xffffffff << (32 - endbits);

    if (!tab8)
        tab = makePixelSumTab8();
    else
        tab = tab8;

    sum = 0;
    for (j = 0; j < fullwords; j++) {
        word = line[j];
        if (word) {
            sum += tab[word & 0xff] +
                   tab[(word >> 8) & 0xff] +
                   tab[(word >> 16) & 0xff] +
                   tab[(word >> 24) & 0xff];
        }
    }
    if (endbits) {
        word = line[j] & endmask;
        if (word) {
            sum += tab[word & 0xff] +
                   tab[(word >> 8) & 0xff] +
                   tab[(word >> 16) & 0xff] +
                   tab[(word >> 24) & 0xff];
        }
    }
    *pcount = sum;

    if (!tab8)
        FREE(tab);
    return 0;
}


/*!
 *  pixCountPixelsByRow()
 *
 *      Input:  pix (1 bpp)
 *              tab8  (<optional> 8-bit pixel lookup table)
 *      Return: na of counts, or null on error
 */
NUMA *
pixCountPixelsByRow(PIX      *pix,
                    l_int32  *tab8)
{
l_int32   h, i, count;
l_int32  *tab;
NUMA     *na;

    PROCNAME("pixCountPixelsByRow");

    if (!pix || pixGetDepth(pix) != 1)
        return (NUMA *)ERROR_PTR("pix undefined or not 1 bpp", procName, NULL);

    if (!tab8)
        tab = makePixelSumTab8();
    else
        tab = tab8;

    h = pixGetHeight(pix);
    if ((na = numaCreate(h)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    for (i = 0; i < h; i++) {
        pixCountPixelsInRow(pix, i, &count, tab);
        numaAddNumber(na, count);
    }

    if (!tab8)
        FREE(tab);

    return na;
}


/*!
 *  pixCountPixelsByColumn()
 *
 *      Input:  pix (1 bpp)
 *      Return: na of counts in each column, or null on error
 */
NUMA *
pixCountPixelsByColumn(PIX  *pix)
{
l_int32     i, j, w, h, wpl;
l_uint32   *line, *data;
l_float32  *array;
NUMA       *na;

    PROCNAME("pixCountPixelsByColumn");

    if (!pix || pixGetDepth(pix) != 1)
        return (NUMA *)ERROR_PTR("pix undefined or not 1 bpp", procName, NULL);

    pixGetDimensions(pix, &w, &h, NULL);
    if ((na = numaCreate(w)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    numaSetCount(na, w);
    array = numaGetFArray(na, L_NOCOPY);
    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    for (i = 0; i < h; i++) {
        line = data + wpl * i;
        for (j = 0; j < w; j++) {
            if (GET_DATA_BIT(line, j))
                array[j] += 1.0;
        }
    }

    return na;
}


/*!
 *  pixSumPixelsByRow()
 *
 *      Input:  pix (1, 8 or 16 bpp; no colormap)
 *              tab8  (<optional> lookup table for 1 bpp; use null for 8 bpp)
 *      Return: na of pixel sums by row, or null on error
 *
 *  Notes:
 *      (1) To resample for a bin size different from 1, use
 *          numaUniformSampling() on the result of this function.
 */
NUMA *
pixSumPixelsByRow(PIX      *pix,
                  l_int32  *tab8)
{
l_int32    i, j, w, h, d, wpl;
l_uint32  *line, *data;
l_float32  sum;
NUMA      *na;

    PROCNAME("pixSumPixelsByRow");

    if (!pix)
        return (NUMA *)ERROR_PTR("pix not defined", procName, NULL);
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 1 && d != 8 && d != 16)
        return (NUMA *)ERROR_PTR("pix not 1, 8 or 16 bpp", procName, NULL);
    if (pixGetColormap(pix) != NULL)
        return (NUMA *)ERROR_PTR("pix colormapped", procName, NULL);

    if (d == 1)
        return pixCountPixelsByRow(pix, tab8);

    if ((na = numaCreate(h)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    for (i = 0; i < h; i++) {
        sum = 0.0;
        line = data + i * wpl;
        if (d == 8) {
            sum += w * 255;
            for (j = 0; j < w; j++)
                sum -= GET_DATA_BYTE(line, j);
        }
        else {  /* d == 16 */
            sum += w * 0xffff;
            for (j = 0; j < w; j++)
                sum -= GET_DATA_TWO_BYTES(line, j);
        }
        numaAddNumber(na, sum);
    }

    return na;
}


/*!
 *  pixSumPixelsByColumn()
 *
 *      Input:  pix (1, 8 or 16 bpp; no colormap)
 *      Return: na of pixel sums by column, or null on error
 *
 *  Notes:
 *      (1) To resample for a bin size different from 1, use
 *          numaUniformSampling() on the result of this function.
 */
NUMA *
pixSumPixelsByColumn(PIX  *pix)
{
l_int32     i, j, w, h, d, wpl;
l_uint32   *line, *data;
l_float32  *array;
NUMA       *na;

    PROCNAME("pixSumPixelsByColumn");

    if (!pix)
        return (NUMA *)ERROR_PTR("pix not defined", procName, NULL);
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 1 && d != 8 && d != 16)
        return (NUMA *)ERROR_PTR("pix not 1, 8 or 16 bpp", procName, NULL);
    if (pixGetColormap(pix) != NULL)
        return (NUMA *)ERROR_PTR("pix colormapped", procName, NULL);

    if (d == 1)
        return pixCountPixelsByColumn(pix);

    if ((na = numaCreate(w)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    numaSetCount(na, w);
    array = numaGetFArray(na, L_NOCOPY);
    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    for (i = 0; i < h; i++) {
        line = data + wpl * i;
        if (d == 8) {
            for (j = 0; j < w; j++)
                array[j] += 255 - GET_DATA_BYTE(line, j);
        }
        else {  /* d == 16 */
            for (j = 0; j < w; j++)
                array[j] += 0xffff - GET_DATA_TWO_BYTES(line, j);
        }
    }

    return na;
}


/*!
 *  pixThresholdPixelSum()
 *
 *      Input:  pix (1 bpp)
 *              threshold
 *              &above (<return> 1 if above threshold;
 *                               0 if equal to or less than threshold)
 *              tab8  (<optional> 8-bit pixel lookup table)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This sums the ON pixels and returns immediately if the count
 *          goes above threshold.  It is therefore more efficient
 *          for matching images (by running this function on the xor of
 *          the 2 images) than using pixCountPixels(), which counts all
 *          pixels before returning.
 */
l_int32
pixThresholdPixelSum(PIX      *pix,
                     l_int32   thresh,
                     l_int32  *pabove,
                     l_int32  *tab8)
{
l_uint32   word, endmask;
l_int32   *tab;
l_int32    w, h, wpl, i, j;
l_int32    fullwords, endbits, sum;
l_uint32  *line, *data;

    PROCNAME("pixThresholdPixelSum");

    if (!pabove)
        return ERROR_INT("pabove not defined", procName, 1);
    *pabove = 0;
    if (!pix || pixGetDepth(pix) != 1)
        return ERROR_INT("pix not defined or not 1 bpp", procName, 1);

    if (!tab8)
        tab = makePixelSumTab8();
    else
        tab = tab8;

    pixGetDimensions(pix, &w, &h, NULL);
    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    fullwords = w >> 5;
    endbits = w & 31;
    endmask = 0xffffffff << (32 - endbits);

    sum = 0;
    for (i = 0; i < h; i++) {
        line = data + wpl * i;
        for (j = 0; j < fullwords; j++) {
            word = line[j];
            if (word) {
                sum += tab[word & 0xff] +
                       tab[(word >> 8) & 0xff] +
                       tab[(word >> 16) & 0xff] +
                       tab[(word >> 24) & 0xff];
            }
        }
        if (endbits) {
            word = line[j] & endmask;
            if (word) {
                sum += tab[word & 0xff] +
                       tab[(word >> 8) & 0xff] +
                       tab[(word >> 16) & 0xff] +
                       tab[(word >> 24) & 0xff];
            }
        }
        if (sum > thresh) {
            *pabove = 1;
            if (!tab8)
                FREE(tab);
            return 0;
        }
    }

    if (!tab8)
        FREE(tab);
    return 0;
}


/*!
 *  makePixelSumTab8()
 *
 *      Input:  void
 *      Return: table of 256 l_int32, or null on error
 *
 *  Notes:
 *      (1) This table of integers gives the number of 1 bits
 *          in the 8 bit index.
 */
l_int32 *
makePixelSumTab8(void)
{
l_uint8   byte;
l_int32   i;
l_int32  *tab;

    PROCNAME("makePixelSumTab8");

    if ((tab = (l_int32 *)CALLOC(256, sizeof(l_int32))) == NULL)
        return (l_int32 *)ERROR_PTR("tab not made", procName, NULL);

    for (i = 0; i < 256; i++) {
        byte = (l_uint8)i;
        tab[i] = (byte & 0x1) +
                 ((byte >> 1) & 0x1) +
                 ((byte >> 2) & 0x1) +
                 ((byte >> 3) & 0x1) +
                 ((byte >> 4) & 0x1) +
                 ((byte >> 5) & 0x1) +
                 ((byte >> 6) & 0x1) +
                 ((byte >> 7) & 0x1);
    }

    return tab;
}


/*!
 *  makePixelCentroidTab8()
 *
 *      Input:  void
 *      Return: table of 256 l_int32, or null on error
 *
 *  Notes:
 *      (1) This table of integers gives the centroid weight of the 1 bits
 *          in the 8 bit index.  In other words, if sumtab is obtained by
 *          makePixelSumTab8, and centroidtab is obtained by
 *          makePixelCentroidTab8, then, for 1 <= i <= 255,
 *          centroidtab[i] / (float)sumtab[i]
 *          is the centroid of the 1 bits in the 8-bit index i, where the
 *          MSB is considered to have position 0 and the LSB is considered
 *          to have position 7.
 */
l_int32 *
makePixelCentroidTab8(void)
{
l_int32   i;
l_int32  *tab;

    PROCNAME("makePixelCentroidTab8");

    if ((tab = (l_int32 *)CALLOC(256, sizeof(l_int32))) == NULL)
        return (l_int32 *)ERROR_PTR("tab not made", procName, NULL);

    tab[0] = 0;
    tab[1] = 7;
    for (i = 2; i < 4; i++) {
        tab[i] = tab[i - 2] + 6;
    }
    for (i = 4; i < 8; i++) {
        tab[i] = tab[i - 4] + 5;
    }
    for (i = 8; i < 16; i++) {
        tab[i] = tab[i - 8] + 4;
    }
    for (i = 16; i < 32; i++) {
        tab[i] = tab[i - 16] + 3;
    }
    for (i = 32; i < 64; i++) {
        tab[i] = tab[i - 32] + 2;
    }
    for (i = 64; i < 128; i++) {
        tab[i] = tab[i - 64] + 1;
    }
    for (i = 128; i < 256; i++) {
        tab[i] = tab[i - 128];
    }

    return tab;
}


/*-------------------------------------------------------------*
 *                       Sum of pixel values                   *
 *-------------------------------------------------------------*/
/*!
 *  pixSumPixelValues()
 *
 *      Input:  pix (1, 2, 4, 8, 16, 32 bpp; not cmapped)
 *              box (<optional> if null, use entire image)
 *              &sum (<return> sum of pixel values in region)
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixSumPixelValues(PIX        *pix,
                  BOX        *box,
                  l_float64  *psum)
{
l_int32    w, h, d, wpl, i, j, xstart, xend, ystart, yend, bw, bh;
l_uint32  *data, *line;
l_float64  sum;
BOX       *boxc;

    PROCNAME("pixSumPixelValues");

    if (!psum)
        return ERROR_INT("psum not defined", procName, 1);
    *psum = 0;
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (pixGetColormap(pix) != NULL)
        return ERROR_INT("pix is colormapped", procName, 1);
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32)
        return ERROR_INT("pix not 1, 2, 4, 8, 16 or 32 bpp", procName, 1);

    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    boxc = NULL;
    if (box) {
        boxc = boxClipToRectangle(box, w, h);
        if (!boxc)
            return ERROR_INT("box outside image", procName, 1);
    }
    xstart = ystart = 0;
    xend = w;
    yend = h;
    if (boxc) {
        boxGetGeometry(boxc, &xstart, &ystart, &bw, &bh);
        xend = xstart + bw;  /* 1 past the end */
        yend = ystart + bh;  /* 1 past the end */
        boxDestroy(&boxc);
    }

    sum = 0;
    for (i = ystart; i < yend; i++) {
        line = data + i * wpl;
        for (j = xstart; j < xend; j++) {
            if (d == 1)
                sum += GET_DATA_BIT(line, j);
            else if (d == 2)
                sum += GET_DATA_DIBIT(line, j);
            else if (d == 4)
                sum += GET_DATA_QBIT(line, j);
            else if (d == 8)
                sum += GET_DATA_BYTE(line, j);
            else if (d == 16)
                sum += GET_DATA_TWO_BYTES(line, j);
            else if (d == 32)
                sum += line[j];
        }
    }
    *psum = sum;

    return 0;
}



/*-------------------------------------------------------------*
 *              Mirrored tiling of a smaller image             *
 *-------------------------------------------------------------*/
/*!
 *  pixMirroredTiling()
 *
 *      Input:  pixs (8 or 32 bpp, small tile; to be replicated)
 *              w, h (dimensions of output pix)
 *      Return: pixd (usually larger pix, mirror-tiled with pixs),
 *              or null on error
 *
 *  Notes:
 *      (1) This uses mirrored tiling, where each row alternates
 *          with LR flips and every column alternates with TB
 *          flips, such that the result is a tiling with identical
 *          2 x 2 tiles, each of which is composed of these transforms:
 *                  -----------------
 *                  | 1    |  LR    |
 *                  -----------------
 *                  | TB   |  LR/TB |
 *                  -----------------
 */
PIX *
pixMirroredTiling(PIX     *pixs,
                  l_int32  w,
                  l_int32  h)
{
l_int32   wt, ht, d, i, j, nx, ny;
PIX      *pixd, *pixsfx, *pixsfy, *pixsfxy, *pix;

    PROCNAME("pixMirroredTiling");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &wt, &ht, &d);
    if (wt <= 0 || ht <= 0)
        return (PIX *)ERROR_PTR("pixs size illegal", procName, NULL);
    if (d != 8 && d != 32)
        return (PIX *)ERROR_PTR("depth not 32 bpp", procName, NULL);
    if ((pixd = pixCreate(w, h, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

    nx = (w + wt - 1) / wt;
    ny = (h + ht - 1) / ht;
    pixsfx = pixFlipLR(NULL, pixs);
    pixsfy = pixFlipTB(NULL, pixs);
    pixsfxy = pixFlipTB(NULL, pixsfx);
    for (i = 0; i < ny; i++) {
        for (j = 0; j < nx; j++) {
            pix = pixs;
            if ((i & 1) && !(j & 1))
                pix = pixsfy;
            else if (!(i & 1) && (j & 1))
                pix = pixsfx;
            else if ((i & 1) && (j & 1))
                pix = pixsfxy;
            pixRasterop(pixd, j * wt, i * ht, wt, ht, PIX_SRC, pix, 0, 0);
        }
    }

    pixDestroy(&pixsfx);
    pixDestroy(&pixsfy);
    pixDestroy(&pixsfxy);
    return pixd;
}


/*!
 *  findTilePatchCenter()
 *
 *      Input:  pixs (8 or 16 bpp; distance function of a binary mask)
 *              box (region of pixs to search around)
 *              searchdir (L_HORIZ or L_VERT; direction to search)
 *              targdist (desired distance of selected patch center from fg)
 *              &dist (<return> actual distance of selected location)
 *              &xc, &yc (<return> location of selected patch center)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This looks for a patch of non-masked image, that is outside
 *          but near the input box.  The input pixs is a distance
 *          function giving the distance from the fg in a binary mask.
 *      (2) The target distance implicitly specifies a desired size
 *          for the patch.  The location of the center of the patch,
 *          and the actual distance from fg are returned.
 *      (3) If the target distance is larger than 255, a 16-bit distance
 *          transform is input.
 *      (4) It is assured that a square centered at (xc, yc) and of
 *          size 'dist' will not intersect with the fg of the binary
 *          mask that was used to generate pixs.
 *      (5) We search away from the component, in approximately
 *          the center 1/3 of its dimension.  This gives a better chance
 *          of finding patches that are close to the component.
 */
static l_int32
findTilePatchCenter(PIX       *pixs,
                    BOX       *box,
                    l_int32    searchdir,
                    l_uint32   targdist,
                    l_uint32  *pdist,
                    l_int32   *pxc,
                    l_int32   *pyc)
{
l_int32   w, h, bx, by, bw, bh, left, right, top, bot, i, j;
l_int32   xstart, xend, ystart, yend;
l_uint32  val, maxval;

    PROCNAME("findTilePatchCenter");

    if (!pdist || !pxc || !pyc)
        return ERROR_INT("&pdist, &pxc, &pyc not all defined", procName, 1);
    *pdist = *pxc = *pyc = 0;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);

    pixGetDimensions(pixs, &w, &h, NULL);
    boxGetGeometry(box, &bx, &by, &bw, &bh);

    if (searchdir == L_HORIZ) {
        left = bx;   /* distance to left of box */
        right = w - bx - bw + 1;   /* distance to right of box */
        ystart = by + bh / 3;
        yend = by + 2 * bh / 3;
        maxval = 0;
        if (left > right) {  /* search to left */
            for (j = bx - 1; j >= 0; j--) {
                for (i = ystart; i <= yend; i++) {
                    pixGetPixel(pixs, j, i, &val);
                    if (val > maxval) {
                        maxval = val;
                        *pxc = j;
                        *pyc = i;
                        *pdist = val;
                        if (val >= targdist)
                            return 0;
                    }
                }
            }
        }
        else {  /* search to right */
            for (j = bx + bw; j < w; j++) {
                for (i = ystart; i <= yend; i++) {
                    pixGetPixel(pixs, j, i, &val);
                    if (val > maxval) {
                        maxval = val;
                        *pxc = j;
                        *pyc = i;
                        *pdist = val;
                        if (val >= targdist)
                            return 0;
                    }
                }
            }
        }
    }
    else {  /* searchdir == L_VERT */
        top = by;    /* distance above box */
        bot = h - by - bh + 1;   /* distance below box */
        xstart = bx + bw / 3;
        xend = bx + 2 * bw / 3;
        maxval = 0;
        if (top > bot) {  /* search above */
            for (i = by - 1; i >= 0; i--) {
                for (j = xstart; j <=xend; j++) {
                    pixGetPixel(pixs, j, i, &val);
                    if (val > maxval) {
                        maxval = val;
                        *pxc = j;
                        *pyc = i;
                        *pdist = val;
                        if (val >= targdist)
                            return 0;
                    }
                }
            }
        }
        else {  /* search below */
            for (i = by + bh; i < h; i++) {
                for (j = xstart; j <=xend; j++) {
                    pixGetPixel(pixs, j, i, &val);
                    if (val > maxval) {
                        maxval = val;
                        *pxc = j;
                        *pyc = i;
                        *pdist = val;
                        if (val >= targdist)
                            return 0;
                    }
                }
            }
        }
    }


    pixGetPixel(pixs, *pxc, *pyc, pdist);
    return 0;
}

