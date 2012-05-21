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
 *  fpix2.c
 *
 *    This file has these FPix utilities:
 *       - interconversion with pix
 *       - interconversion with dpix
 *       - min and max values
 *       - border functions
 *       - simple rasterop (source --> dest)
 *       - integer scaling
 *       - arithmetic operations
 *
 *    Interconversions between Pix and FPix
 *          FPIX          *pixConvertToFPix()
 *          PIX           *fpixConvertToPix()
 *          PIX           *fpixDisplayMaxDynamicRange()  [useful for debugging]
 *
 *    Interconversions between FPix and DPix
 *          DPIX          *fpixConvertToDPix()
 *          FPIX          *dpixConvertToFPix()
 *
 *    FPix min/max value
 *          l_int32        fpixGetMin()
 *          l_int32        fpixGetMax()
 *
 *    FPix border functions
 *          FPIX          *fpixAddBorder()
 *          FPIX          *fpixRemoveBorder()
 *          FPIX          *fpixAddMirroredBorder()
 *
 *    FPix simple rasterop
 *          l_int32        fpixRasterop()
 *
 *    Integer scaling
 *          FPIX          *fpixScaleByInteger()
 *          DPIX          *dpixScaleByInteger()
 *
 *    FPix arithmetic operations
 *          FPIX          *fpixLinearCombination()
 *          l_int32        fpixAddMultConstant()
 */

#include "allheaders.h"

/*--------------------------------------------------------------------*
 *                     FPix  <-->  Pix conversions                    *
 *--------------------------------------------------------------------*/
/*!
 *  pixConvertToFPix()
 *
 *      Input:  pix (1, 2, 4, 8, 16 or 32 bpp)
 *              ncomps (number of components: 3 for RGB, 1 otherwise)
 *      Return: fpix, or null on error
 *
 *  Notes:
 *      (1) If colormapped, remove to grayscale.
 *      (2) If 32 bpp and @ncomps == 3, this is RGB; convert to luminance.
 *          In all other cases the src image is treated as having a single
 *          component of pixel values.
 */
FPIX *
pixConvertToFPix(PIX     *pixs,
                 l_int32  ncomps)
{
l_int32     w, h, d, i, j, val, wplt, wpld;
l_uint32    uval;
l_uint32   *datat, *linet;
l_float32  *datad, *lined;
PIX        *pixt;
FPIX       *fpixd;

    PROCNAME("pixConvertToFPix");

    if (!pixs)
        return (FPIX *)ERROR_PTR("pixs not defined", procName, NULL);

    if (pixGetColormap(pixs))
        pixt = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
    else if (pixGetDepth(pixs) == 32 && ncomps == 3)
        pixt = pixConvertRGBToLuminance(pixs);
    else
        pixt = pixClone(pixs);

    pixGetDimensions(pixt, &w, &h, &d);
    if ((fpixd = fpixCreate(w, h)) == NULL)
        return (FPIX *)ERROR_PTR("fpixd not made", procName, NULL);
    datat = pixGetData(pixt);
    wplt = pixGetWpl(pixt);
    datad = fpixGetData(fpixd);
    wpld = fpixGetWpl(fpixd);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        lined = datad + i * wpld;
        if (d == 1) {
            for (j = 0; j < w; j++) {
                val = GET_DATA_BIT(linet, j);
                lined[j] = (l_float32)val;
            }
        }
        else if (d == 2) {
            for (j = 0; j < w; j++) {
                val = GET_DATA_DIBIT(linet, j);
                lined[j] = (l_float32)val;
            }
        }
        else if (d == 4) {
            for (j = 0; j < w; j++) {
                val = GET_DATA_QBIT(linet, j);
                lined[j] = (l_float32)val;
            }
        }
        else if (d == 8) {
            for (j = 0; j < w; j++) {
                val = GET_DATA_BYTE(linet, j);
                lined[j] = (l_float32)val;
            }
        }
        else if (d == 16) {
            for (j = 0; j < w; j++) {
                val = GET_DATA_TWO_BYTES(linet, j);
                lined[j] = (l_float32)val;
            }
        }
        else if (d == 32) {
            for (j = 0; j < w; j++) {
                uval = GET_DATA_FOUR_BYTES(linet, j);
                lined[j] = (l_float32)uval;
            }
        }
    }

    pixDestroy(&pixt);
    return fpixd;
}


/*!
 *  fpixConvertToPix()
 *
 *      Input:  fpixs
 *              outdepth (0, 8, 16 or 32 bpp)
 *              negvals (L_CLIP_TO_ZERO, L_TAKE_ABSVAL)
 *              errorflag (1 to output error stats; 0 otherwise)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Use @outdepth = 0 to programmatically determine the
 *          output depth.  If no values are greater than 255,
 *          it will set outdepth = 8; otherwise to 16 or 32.
 *      (2) Because we are converting a float to an unsigned int
 *          with a specified dynamic range (8, 16 or 32 bits), errors
 *          can occur.  If errorflag == TRUE, output the number
 *          of values out of range, both negative and positive.
 *      (3) If a pixel value is positive and out of range, clip to
 *          the maximum value represented at the outdepth of 8, 16
 *          or 32 bits.
 */
PIX *
fpixConvertToPix(FPIX    *fpixs,
                 l_int32  outdepth,
                 l_int32  negvals,
                 l_int32  errorflag)
{
l_int32     w, h, i, j, wpls, wpld, maxval;
l_uint32    vald;
l_float32   val;
l_float32  *datas, *lines;
l_uint32   *datad, *lined;
PIX        *pixd;

    PROCNAME("fpixConvertToPix");

    if (!fpixs)
        return (PIX *)ERROR_PTR("fpixs not defined", procName, NULL);
    if (negvals != L_CLIP_TO_ZERO && negvals != L_TAKE_ABSVAL)
        return (PIX *)ERROR_PTR("invalid negvals", procName, NULL);
    if (outdepth != 0 && outdepth != 8 && outdepth != 16 && outdepth != 32)
        return (PIX *)ERROR_PTR("outdepth not in {0,8,16,32}", procName, NULL);

    fpixGetDimensions(fpixs, &w, &h);
    datas = fpixGetData(fpixs);
    wpls = fpixGetWpl(fpixs);

        /* Adaptive determination of output depth */
    if (outdepth == 0) {
        outdepth = 8;
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < w; j++) {
                if (lines[j] > 65535.5) {
                    outdepth = 32;
                    break;
                }
                if (lines[j] > 255.5)
                    outdepth = 16;
            }
            if (outdepth == 32) break;
        }
    }
    maxval = (1 << outdepth) - 1;

        /* Gather statistics if @errorflag = TRUE */
    if (errorflag) {
        l_int32  negs = 0;
        l_int32  overvals = 0;
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < w; j++) {
                val = lines[j];
                if (val < 0.0)
                    negs++;
                else if (val > maxval)
                    overvals++;
            }
        }
        if (negs > 0)
            L_ERROR_INT("Number of negative values: %d", procName, negs);
        if (overvals > 0)
            L_ERROR_INT("Number of too-large values: %d", procName, overvals);
    }

        /* Make the pix and convert the data */
    if ((pixd = pixCreate(w, h, outdepth)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            val = lines[j];
            if (val >= 0.0)
                vald = (l_uint32)(val + 0.5);
            else {  /* val < 0.0 */
                if (negvals == L_CLIP_TO_ZERO)
                    vald = 0;
                else
                    vald = (l_uint32)(-val + 0.5);
            }
            if (vald > maxval)
                vald = maxval;
            if (outdepth == 8)
                SET_DATA_BYTE(lined, j, vald);
            else if (outdepth == 16)
                SET_DATA_TWO_BYTES(lined, j, vald);
            else  /* outdepth == 32 */
                SET_DATA_FOUR_BYTES(lined, j, vald);
        }
    }

    return pixd;
}


/*!
 *  fpixDisplayMaxDynamicRange()
 *
 *      Input:  fpixs
 *      Return: pixd (8 bpp), or null on error
 */
PIX *
fpixDisplayMaxDynamicRange(FPIX  *fpixs)
{
l_uint8     dval;
l_int32     i, j, w, h, wpls, wpld;
l_float32   factor, sval, maxval;
l_float32  *lines, *datas;
l_uint32   *lined, *datad;
PIX        *pixd;

    PROCNAME("fpixDisplayMaxDynamicRange");

    if (!fpixs)
        return (PIX *)ERROR_PTR("fpixs not defined", procName, NULL);

    fpixGetDimensions(fpixs, &w, &h);
    datas = fpixGetData(fpixs);
    wpls = fpixGetWpl(fpixs);

    maxval = 0.0;
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        for (j = 0; j < w; j++) {
            sval = *(lines + j);
            if (sval > maxval)
                maxval = sval;
        }
    }

    pixd = pixCreate(w, h, 8);
    if (maxval == 0.0)
        return pixd;  /* all pixels are 0 */

    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    factor = 255. / maxval;
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            sval = *(lines + j);
            if (sval < 0.0) sval = 0.0;
            dval = (l_uint8)(factor * sval + 0.5);
            SET_DATA_BYTE(lined, j, dval);
        }
    }

    return pixd;
}


/*--------------------------------------------------------------------*
 *                     FPix  <-->  DPix conversions                   *
 *--------------------------------------------------------------------*/
/*!
 *  fpixConvertToDPix()
 *
 *      Input:  fpix
 *      Return: dpix, or null on error
 */
DPIX *
fpixConvertToDPix(FPIX  *fpix)
{
l_int32     w, h, i, j, wpls, wpld;
l_float32   val;
l_float32  *datas, *lines;
l_float64  *datad, *lined;
DPIX       *dpix;

    PROCNAME("fpixConvertToDPix");

    if (!fpix)
        return (DPIX *)ERROR_PTR("fpix not defined", procName, NULL);

    fpixGetDimensions(fpix, &w, &h);
    if ((dpix = dpixCreate(w, h)) == NULL)
        return (DPIX *)ERROR_PTR("dpix not made", procName, NULL);

    datas = fpixGetData(fpix);
    datad = dpixGetData(dpix);
    wpls = fpixGetWpl(fpix);
    wpld = dpixGetWpl(dpix);  /* 8 byte words */
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            val = lines[j];
            lined[j] = val;
        }
    }

    return dpix;
}


/*!
 *  dpixConvertToFPix()
 *
 *      Input:  dpix
 *      Return: fpix, or null on error
 */
FPIX *
dpixConvertToFPix(DPIX  *dpix)
{
l_int32     w, h, i, j, wpls, wpld;
l_float64   val;
l_float32  *datad, *lined;
l_float64  *datas, *lines;
FPIX       *fpix;

    PROCNAME("dpixConvertToFPix");

    if (!dpix)
        return (FPIX *)ERROR_PTR("dpix not defined", procName, NULL);

    dpixGetDimensions(dpix, &w, &h);
    if ((fpix = fpixCreate(w, h)) == NULL)
        return (FPIX *)ERROR_PTR("fpix not made", procName, NULL);

    datas = dpixGetData(dpix);
    datad = fpixGetData(fpix);
    wpls = dpixGetWpl(dpix);  /* 8 byte words */
    wpld = fpixGetWpl(fpix);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            val = lines[j];
            lined[j] = (l_float32)val;
        }
    }

    return fpix;
}



/*--------------------------------------------------------------------*
 *                           Min/max value                            *
 *--------------------------------------------------------------------*/
/*!
 *  fpixGetMin()
 *
 *      Input:  fpix
 *              &minval (<optional return> min value)
 *              &xminloc (<optional return> x location of min)
 *              &yminloc (<optional return> y location of min)
 *      Return: 0 if OK; 1 on error
 */
l_int32
fpixGetMin(FPIX       *fpix,
           l_float32  *pminval,
           l_int32    *pxminloc,
           l_int32    *pyminloc)
{
l_int32     i, j, w, h, wpl, xminloc, yminloc;
l_float32  *data, *line;
l_float32   minval;

    PROCNAME("fpixGetMin");

    if (!pminval && !pxminloc && !pyminloc)
        return ERROR_INT("nothing to do", procName, 1);
    if (pminval) *pminval = 0.0;
    if (pxminloc) *pxminloc = 0;
    if (pyminloc) *pyminloc = 0;
    if (!fpix)
        return ERROR_INT("fpix not defined", procName, 1);

    minval = +1.0e20;
    xminloc = 0;
    yminloc = 0;
    fpixGetDimensions(fpix, &w, &h);
    data = fpixGetData(fpix);
    wpl = fpixGetWpl(fpix);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < w; j++) {
            if (line[j] < minval) {
                minval = line[j];
                xminloc = j;
                yminloc = i;
            }
        }
    }

    if (pminval) *pminval = minval;
    if (pxminloc) *pxminloc = xminloc;
    if (pyminloc) *pyminloc = yminloc;
    return 0;
}


/*!
 *  fpixGetMax()
 *
 *      Input:  fpix
 *              &maxval (<optional return> max value)
 *              &xmaxloc (<optional return> x location of max)
 *              &ymaxloc (<optional return> y location of max)
 *      Return: 0 if OK; 1 on error
 */
l_int32
fpixGetMax(FPIX       *fpix,
           l_float32  *pmaxval,
           l_int32    *pxmaxloc,
           l_int32    *pymaxloc)
{
l_int32     i, j, w, h, wpl, xmaxloc, ymaxloc;
l_float32  *data, *line;
l_float32   maxval;

    PROCNAME("fpixGetMax");

    if (!pmaxval && !pxmaxloc && !pymaxloc)
        return ERROR_INT("nothing to do", procName, 1);
    if (pmaxval) *pmaxval = 0.0;
    if (pxmaxloc) *pxmaxloc = 0;
    if (pymaxloc) *pymaxloc = 0;
    if (!fpix)
        return ERROR_INT("fpix not defined", procName, 1);

    maxval = -1.0e20;
    xmaxloc = 0;
    ymaxloc = 0;
    fpixGetDimensions(fpix, &w, &h);
    data = fpixGetData(fpix);
    wpl = fpixGetWpl(fpix);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < w; j++) {
            if (line[j] < maxval) {
                maxval = line[j];
                xmaxloc = j;
                ymaxloc = i;
            }
        }
    }

    if (pmaxval) *pmaxval = maxval;
    if (pxmaxloc) *pxmaxloc = xmaxloc;
    if (pymaxloc) *pymaxloc = ymaxloc;
    return 0;
}


/*--------------------------------------------------------------------*
 *                          Border functions                          *
 *--------------------------------------------------------------------*/
/*!
 *  fpixAddBorder()
 *
 *      Input:  fpixs
 *              left, right, top, bot (pixels on each side to be added)
 *      Return: fpixd, or null on error
 *
 *  Notes:
 *      (1) Adds border of '0' 32-bit pixels
 */
FPIX *
fpixAddBorder(FPIX    *fpixs,
              l_int32  left,
              l_int32  right,
              l_int32  top,
              l_int32  bot)
{
l_int32  ws, hs, wd, hd;
FPIX    *fpixd;

    PROCNAME("fpixAddBorder");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);

    if (left <= 0 && right <= 0 && top <= 0 && bot <= 0)
        return fpixCopy(NULL, fpixs);
    fpixGetDimensions(fpixs, &ws, &hs);
    wd = ws + left + right;
    hd = hs + top + bot;
    if ((fpixd = fpixCreate(wd, hd)) == NULL)
        return (FPIX *)ERROR_PTR("fpixd not made", procName, NULL);

    fpixCopyResolution(fpixd, fpixs);
    fpixRasterop(fpixd, left, top, ws, hs, fpixs, 0, 0);
    return fpixd;
}


/*!
 *  fpixRemoveBorder()
 *
 *      Input:  fpixs
 *              left, right, top, bot (pixels on each side to be removed)
 *      Return: fpixd, or null on error
 */
FPIX *
fpixRemoveBorder(FPIX    *fpixs,
                 l_int32  left,
                 l_int32  right,
                 l_int32  top,
                 l_int32  bot)
{
l_int32  ws, hs, wd, hd;
FPIX    *fpixd;

    PROCNAME("fpixRemoveBorder");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);

    if (left <= 0 && right <= 0 && top <= 0 && bot <= 0)
        return fpixCopy(NULL, fpixs);
    fpixGetDimensions(fpixs, &ws, &hs);
    wd = ws - left - right;
    hd = hs - top - bot;
    if (wd <= 0 || hd <= 0)
        return (FPIX *)ERROR_PTR("width & height not both > 0", procName, NULL);
    if ((fpixd = fpixCreate(wd, hd)) == NULL)
        return (FPIX *)ERROR_PTR("fpixd not made", procName, NULL);

    fpixCopyResolution(fpixd, fpixs);
    fpixRasterop(fpixd, 0, 0, wd, hd, fpixs, left, top);
    return fpixd;
}



/*!
 *  fpixAddMirroredBorder()
 *
 *      Input:  fpixs
 *              left, right, top, bot (pixels on each side to be added)
 *      Return: fpixd, or null on error
 *
 *  Notes:
 *      (1) See pixAddMirroredBorder() for situations of usage.
 */
FPIX *
fpixAddMirroredBorder(FPIX    *fpixs,
                      l_int32  left,
                      l_int32  right,
                      l_int32  top,
                      l_int32  bot)
{
l_int32  i, j, w, h;
FPIX    *fpixd;

    PROCNAME("fpixAddMirroredBorder");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);

    fpixd = fpixAddBorder(fpixs, left, right, top, bot);
    fpixGetDimensions(fpixs, &w, &h);
    for (j = 0; j < left; j++)
        fpixRasterop(fpixd, left - 1 - j, top, 1, h,
                     fpixd, left + j, top);
    for (j = 0; j < right; j++)
        fpixRasterop(fpixd, left + w + j, top, 1, h,
                     fpixd, left + w - 1 - j, top);
    for (i = 0; i < top; i++)
        fpixRasterop(fpixd, 0, top - 1 - i, left + w + right, 1,
                     fpixd, 0, top + i);
    for (i = 0; i < bot; i++)
        fpixRasterop(fpixd, 0, top + h + i, left + w + right, 1,
                     fpixd, 0, top + h - 1 - i);

    return fpixd;
}


/*--------------------------------------------------------------------*
 *                          Simple rasterop                           *
 *--------------------------------------------------------------------*/
/*!
 *  fpixRasterop()
 *
 *      Input:  fpixd  (dest fpix)
 *              dx     (x val of UL corner of dest rectangle)
 *              dy     (y val of UL corner of dest rectangle)
 *              dw     (width of dest rectangle)
 *              dh     (height of dest rectangle)
 *              fpixs  (src fpix)
 *              sx     (x val of UL corner of src rectangle)
 *              sy     (y val of UL corner of src rectangle)
 *      Return: 0 if OK; 1 on error.
 *
 *  Notes:
 *      (1) This is similiar in structure to pixRasterop(), except
 *          it only allows copying from the source into the destination.
 *          For that reason, no op code is necessary.  Additionally,
 *          all pixels are 32 bit words (float values), which makes
 *          the copy very simple.
 *      (2) Clipping of both src and dest fpix are done automatically.
 *      (3) This allows in-place copying, without checking to see if
 *          the result is valid:  use for in-place with caution!
 */
l_int32
fpixRasterop(FPIX    *fpixd,
             l_int32  dx,
             l_int32  dy,
             l_int32  dw,
             l_int32  dh,
             FPIX    *fpixs,
             l_int32  sx,
             l_int32  sy)
{
l_int32     fsw, fsh, fdw, fdh, dhangw, shangw, dhangh, shangh;
l_int32     i, j, wpls, wpld;
l_float32  *datas, *datad, *lines, *lined;

    PROCNAME("fpixRasterop");

    if (!fpixs)
        return ERROR_INT("fpixs not defined", procName, 1);
    if (!fpixd)
        return ERROR_INT("fpixd not defined", procName, 1);

    /* -------------------------------------------------------- *
     *      Clip to maximum rectangle with both src and dest    *
     * -------------------------------------------------------- */
    fpixGetDimensions(fpixs, &fsw, &fsh);
    fpixGetDimensions(fpixd, &fdw, &fdh);

        /* First clip horizontally (sx, dx, dw) */
    if (dx < 0) {
        sx -= dx;  /* increase sx */
        dw += dx;  /* reduce dw */
        dx = 0;
    }
    if (sx < 0) {
        dx -= sx;  /* increase dx */
        dw += sx;  /* reduce dw */
        sx = 0;
    }
    dhangw = dx + dw - fdw;  /* rect overhang of dest to right */
    if (dhangw > 0)
        dw -= dhangw;  /* reduce dw */
    shangw = sx + dw - fsw;   /* rect overhang of src to right */
    if (shangw > 0)
        dw -= shangw;  /* reduce dw */

        /* Then clip vertically (sy, dy, dh) */
    if (dy < 0) {
        sy -= dy;  /* increase sy */
        dh += dy;  /* reduce dh */
        dy = 0;
    }
    if (sy < 0) {
        dy -= sy;  /* increase dy */
        dh += sy;  /* reduce dh */
        sy = 0;
    }
    dhangh = dy + dh - fdh;  /* rect overhang of dest below */
    if (dhangh > 0)
        dh -= dhangh;  /* reduce dh */
    shangh = sy + dh - fsh;  /* rect overhang of src below */
    if (shangh > 0)
        dh -= shangh;  /* reduce dh */

        /* if clipped entirely, quit */
    if ((dw <= 0) || (dh <= 0))
        return 0;

    /* -------------------------------------------------------- *
     *                    Copy block of data                    *
     * -------------------------------------------------------- */
    datas = fpixGetData(fpixs);
    datad = fpixGetData(fpixd);
    wpls = fpixGetWpl(fpixs);
    wpld = fpixGetWpl(fpixd);
    datas += sy * wpls + sx;  /* at UL corner of block */
    datad += dy * wpld + dx;  /* at UL corner of block */
    for (i = 0; i < dh; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < dw; j++) {
            *lined = *lines;
            lines++;
            lined++;
        }
    }

    return 0;
}


/*--------------------------------------------------------------------*
 *                       Special integer scaling                      *
 *--------------------------------------------------------------------*/
/*!
 *  fpixScaleByInteger()
 *
 *      Input:  fpixs (low resolution, subsampled)
 *              factor (scaling factor)
 *      Return: fpixd (interpolated result), or null on error
 *
 *  Notes:
 *      (1) The width wd of fpixd is related to ws of fpixs by:
 *              wd = factor * (ws - 1) + 1   (and ditto for the height)
 *          We avoid special-casing boundary pixels in the interpolation
 *          by constructing fpixd by inserting (factor - 1) interpolated
 *          pixels between each pixel in fpixs.  Then
 *               wd = ws + (ws - 1) * (factor - 1)    (same as above)
 *          This also has the advantage that if we subsample by @factor,
 *          throwing out all the interpolated pixels, we regain the
 *          original low resolution fpix.
 */
FPIX *
fpixScaleByInteger(FPIX    *fpixs,
                   l_int32  factor)
{
l_int32     i, j, k, m, ws, hs, wd, hd, wpls, wpld;
l_float32   val0, val1, val2, val3;
l_float32  *datas, *datad, *lines, *lined, *fract;
FPIX       *fpixd;

    PROCNAME("fpixScaleByInteger");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);

    fpixGetDimensions(fpixs, &ws, &hs);
    wd = factor * (ws - 1) + 1;
    hd = factor * (hs - 1) + 1;
    fpixd = fpixCreate(wd, hd);
    datas = fpixGetData(fpixs);
    datad = fpixGetData(fpixd);
    wpls = fpixGetWpl(fpixs);
    wpld = fpixGetWpl(fpixd);
    fract = (l_float32 *)CALLOC(factor, sizeof(l_float32));
    for (i = 0; i < factor; i++)
        fract[i] = i / (l_float32)factor;
    for (i = 0; i < hs - 1; i++) {
        lines = datas + i * wpls;
        for (j = 0; j < ws - 1; j++) {
            val0 = lines[j];
            val1 = lines[j + 1];
            val2 = lines[wpls + j];
            val3 = lines[wpls + j + 1];
            for (k = 0; k < factor; k++) {  /* rows of sub-block */
                lined = datad + (i * factor + k) * wpld;
                for (m = 0; m < factor; m++) {  /* cols of sub-block */
                     *(lined + j * factor + m) =
                            val0 * (1.0 - fract[m]) * (1.0 - fract[k]) +
                            val1 * fract[m] * (1.0 - fract[k]) +
                            val2 * (1.0 - fract[m]) * fract[k] +
                            val3 * fract[m] * fract[k];
                }
            }
        }
    }

        /* Do the right-most column of fpixd, skipping LR corner */
    for (i = 0; i < hs - 1; i++) {
        lines = datas + i * wpls;
        val0 = lines[ws - 1];
        val1 = lines[wpls + ws - 1];
        for (k = 0; k < factor; k++) {
            lined = datad + (i * factor + k) * wpld;
            lined[wd - 1] = val0 * (1.0 - fract[k]) + val1 * fract[k];
        }
    }

        /* Do the bottom-most row of fpixd */
    lines = datas + (hs - 1) * wpls;
    lined = datad + (hd - 1) * wpld;
    for (j = 0; j < ws - 1; j++) {
        val0 = lines[j];
        val1 = lines[j + 1];
        for (m = 0; m < factor; m++)
            lined[j * factor + m] = val0 * (1.0 - fract[m]) + val1 * fract[m];
        lined[wd - 1] = lines[ws - 1];  /* LR corner */
    }

    FREE(fract);
    return fpixd;
}


/*!
 *  dpixScaleByInteger()
 *
 *      Input:  dpixs (low resolution, subsampled)
 *              factor (scaling factor)
 *      Return: dpixd (interpolated result), or null on error
 *
 *  Notes:
 *      (1) The width wd of dpixd is related to ws of dpixs by:
 *              wd = factor * (ws - 1) + 1   (and ditto for the height)
 *          We avoid special-casing boundary pixels in the interpolation
 *          by constructing fpixd by inserting (factor - 1) interpolated
 *          pixels between each pixel in fpixs.  Then
 *               wd = ws + (ws - 1) * (factor - 1)    (same as above)
 *          This also has the advantage that if we subsample by @factor,
 *          throwing out all the interpolated pixels, we regain the
 *          original low resolution dpix.
 */
DPIX *
dpixScaleByInteger(DPIX    *dpixs,
                   l_int32  factor)
{
l_int32     i, j, k, m, ws, hs, wd, hd, wpls, wpld;
l_float64   val0, val1, val2, val3;
l_float64  *datas, *datad, *lines, *lined, *fract;
DPIX       *dpixd;

    PROCNAME("dpixScaleByInteger");

    if (!dpixs)
        return (DPIX *)ERROR_PTR("dpixs not defined", procName, NULL);

    dpixGetDimensions(dpixs, &ws, &hs);
    wd = factor * (ws - 1) + 1;
    hd = factor * (hs - 1) + 1;
    dpixd = dpixCreate(wd, hd);
    datas = dpixGetData(dpixs);
    datad = dpixGetData(dpixd);
    wpls = dpixGetWpl(dpixs);
    wpld = dpixGetWpl(dpixd);
    fract = (l_float64 *)CALLOC(factor, sizeof(l_float64));
    for (i = 0; i < factor; i++)
        fract[i] = i / (l_float64)factor;
    for (i = 0; i < hs - 1; i++) {
        lines = datas + i * wpls;
        for (j = 0; j < ws - 1; j++) {
            val0 = lines[j];
            val1 = lines[j + 1];
            val2 = lines[wpls + j];
            val3 = lines[wpls + j + 1];
            for (k = 0; k < factor; k++) {  /* rows of sub-block */
                lined = datad + (i * factor + k) * wpld;
                for (m = 0; m < factor; m++) {  /* cols of sub-block */
                     *(lined + j * factor + m) =
                            val0 * (1.0 - fract[m]) * (1.0 - fract[k]) +
                            val1 * fract[m] * (1.0 - fract[k]) +
                            val2 * (1.0 - fract[m]) * fract[k] +
                            val3 * fract[m] * fract[k];
                }
            }
        }
    }

        /* Do the right-most column of dpixd, skipping LR corner */
    for (i = 0; i < hs - 1; i++) {
        lines = datas + i * wpls;
        val0 = lines[ws - 1];
        val1 = lines[wpls + ws - 1];
        for (k = 0; k < factor; k++) {
            lined = datad + (i * factor + k) * wpld;
            lined[wd - 1] = val0 * (1.0 - fract[k]) + val1 * fract[k];
        }
    }

        /* Do the bottom-most row of dpixd */
    lines = datas + (hs - 1) * wpls;
    lined = datad + (hd - 1) * wpld;
    for (j = 0; j < ws - 1; j++) {
        val0 = lines[j];
        val1 = lines[j + 1];
        for (m = 0; m < factor; m++)
            lined[j * factor + m] = val0 * (1.0 - fract[m]) + val1 * fract[m];
        lined[wd - 1] = lines[ws - 1];  /* LR corner */
    }

    FREE(fract);
    return dpixd;
}


/*--------------------------------------------------------------------*
 *                        Arithmetic operations                       *
 *--------------------------------------------------------------------*/
/*!
 *  fpixLinearCombo()
 *
 *      Input:  fpixd (<optional>; this can be null, equal to fpixs1, or
 *                     different from fpixs1)
 *              fpixs1 (can be == to fpixd)
 *              fpixs2
 *      Return: pixd always
 *
 *  Notes:
 *      (1) Computes pixelwise linear combination: a * src1 + b * src2
 *      (2) Alignment is to UL corner.
 *      (3) There are 3 cases.  The result can go to a new dest,
 *          in-place to fpixs1, or to an existing input dest:
 *          * fpixd == null:   (src1 + src2) --> new fpixd
 *          * fpixd == fpixs1:  (src1 + src2) --> src1  (in-place)
 *          * fpixd != fpixs1: (src1 + src2) --> input fpixd
 *      (4) fpixs2 must be different from both fpixd and fpixs1.
 */
FPIX *
fpixLinearCombination(FPIX      *fpixd,
                      FPIX      *fpixs1,
                      FPIX      *fpixs2,
                      l_float32  a,
                      l_float32  b)
{
l_int32     i, j, ws, hs, w, h, wpls, wpld;
l_float32   val;
l_float32  *datas, *datad, *lines, *lined;

    PROCNAME("fpixLinearCombination");

    if (!fpixs1)
        return (FPIX *)ERROR_PTR("fpixs1 not defined", procName, fpixd);
    if (!fpixs2)
        return (FPIX *)ERROR_PTR("fpixs2 not defined", procName, fpixd);
    if (fpixs1 == fpixs2)
        return (FPIX *)ERROR_PTR("fpixs1 == fpixs2", procName, fpixd);
    if (fpixs2 == fpixd)
        return (FPIX *)ERROR_PTR("fpixs2 == fpixd", procName, fpixd);

    if (fpixs1 != fpixd)
        fpixd = fpixCopy(fpixd, fpixs1);

    datas = fpixGetData(fpixs2);
    datad = fpixGetData(fpixd);
    wpls = fpixGetWpl(fpixs2);
    wpld = fpixGetWpl(fpixd);
    fpixGetDimensions(fpixs2, &ws, &hs);
    fpixGetDimensions(fpixd, &w, &h);
    w = L_MIN(ws, w);
    h = L_MIN(hs, h);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        if (a == 1.0 && b == 1.0) {  /* sum */
            for (j = 0; j < w; j++)
                *(lined + j) += *(lines + j);
        }
        else if (a == 1.0 && b == -1.0) {  /* diff */
            for (j = 0; j < w; j++)
                *(lined + j) -= *(lines + j);
        }
        else if (a == -1.0 && b == 1.0) {  /* diff */
            for (j = 0; j < w; j++) {
                val = *(lined + j);
                *(lined + j) = -val + *(lines + j);
            }
        }
        else if (a == -1.0 && b == -1.0) {
            for (j = 0; j < w; j++) {
                val = *(lined + j);
                *(lined + j) = -val - *(lines + j);
            }
        }
        else {
            for (j = 0; j < w; j++)
                *(lined + j) = a * lined[j] + b * lines[j];
        }
    }

    return fpixd;
}


/*!
 *  fpixAddMultConstant()
 *
 *      Input:  fpix
 *              addc  (use 0.0 to skip the operation)
 *              multc (use 1.0 to skip the operation)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is an in-place operation.
 *      (2) It can be used to multiply each pixel by a constant,
 *          and also to add a constant to each pixel.  Multiplication
 *          is done first.
 */
l_int32
fpixAddMultConstant(FPIX      *fpix,
                    l_float32  addc,
                    l_float32  multc)
{
l_int32     i, j, w, h, wpl;
l_float32   val;
l_float32  *line, *data;

    PROCNAME("fpixAddMultConstant");

    if (!fpix)
        return ERROR_INT("fpix not defined", procName, 1);

    if (addc == 0.0 && multc == 1.0)
        return 0;

    fpixGetDimensions(fpix, &w, &h);
    data = fpixGetData(fpix);
    wpl = fpixGetWpl(fpix);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        if (addc == 0.0) {
            for (j = 0; j < w; j++)
                *(line + j) *= multc;
        }
        else if (multc == 1.0) {
            for (j = 0; j < w; j++)
                *(line + j) += addc;
        }
        else  {
            for (j = 0; j < w; j++) {
                val = *(line + j);
                *(line + j) = multc * val + addc;
            }
        }
    }

    return 0;
}

