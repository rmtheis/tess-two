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
 *  fpix2.c
 *
 *    This file has these FPix utilities:
 *       - interconversions with pix, fpix, dpix
 *       - min and max values
 *       - integer scaling
 *       - arithmetic operations
 *       - set all
 *       - border functions
 *       - simple rasterop (source --> dest)
 *       - geometric transforms
 *
 *    Interconversions between Pix, FPix and DPix
 *          FPIX          *pixConvertToFPix()
 *          DPIX          *pixConvertToDPix()
 *          PIX           *fpixConvertToPix()
 *          PIX           *fpixDisplayMaxDynamicRange()  [useful for debugging]
 *          DPIX          *fpixConvertToDPix()
 *          PIX           *dpixConvertToPix()
 *          FPIX          *dpixConvertToFPix()
 *
 *    Min/max value
 *          l_int32        fpixGetMin()
 *          l_int32        fpixGetMax()
 *          l_int32        dpixGetMin()
 *          l_int32        dpixGetMax()
 *
 *    Integer scaling
 *          FPIX          *fpixScaleByInteger()
 *          DPIX          *dpixScaleByInteger()
 *
 *    Arithmetic operations
 *          FPIX          *fpixLinearCombination()
 *          l_int32        fpixAddMultConstant()
 *          DPIX          *dpixLinearCombination()
 *          l_int32        dpixAddMultConstant()
 *
 *    Set all
 *          l_int32        fpixSetAllArbitrary()
 *          l_int32        dpixSetAllArbitrary()
 *
 *    FPix border functions
 *          FPIX          *fpixAddBorder()
 *          FPIX          *fpixRemoveBorder()
 *          FPIX          *fpixAddMirroredBorder()
 *          FPIX          *fpixAddContinuedBorder()
 *          FPIX          *fpixAddSlopeBorder()
 *
 *    FPix simple rasterop
 *          l_int32        fpixRasterop()
 *
 *    FPix rotation by multiples of 90 degrees
 *          FPIX          *fpixRotateOrth()
 *          FPIX          *fpixRotate180()
 *          FPIX          *fpixRotate90()
 *          FPIX          *fpixFlipLR()
 *          FPIX          *fpixFlipTB()
 *
 *    FPix affine and projective interpolated transforms
 *          FPIX          *fpixAffinePta()
 *          FPIX          *fpixAffine()
 *          FPIX          *fpixProjectivePta()
 *          FPIX          *fpixProjective()
 *          l_int32        linearInterpolatePixelFloat()
 *
 *    Thresholding to 1 bpp Pix
 *          PIX           *fpixThresholdToPix()
 *
 *    Generate function from components
 *          FPIX          *pixComponentFunction()
 */

#include <string.h>
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

           /* Convert to a single component */
    if (pixGetColormap(pixs))
        pixt = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
    else if (pixGetDepth(pixs) == 32 && ncomps == 3)
        pixt = pixConvertRGBToLuminance(pixs);
    else
        pixt = pixClone(pixs);
    pixGetDimensions(pixt, &w, &h, &d);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32) {
        pixDestroy(&pixt);
        return (FPIX *)ERROR_PTR("invalid depth", procName, NULL);
    }

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
        } else if (d == 2) {
            for (j = 0; j < w; j++) {
                val = GET_DATA_DIBIT(linet, j);
                lined[j] = (l_float32)val;
            }
        } else if (d == 4) {
            for (j = 0; j < w; j++) {
                val = GET_DATA_QBIT(linet, j);
                lined[j] = (l_float32)val;
            }
        } else if (d == 8) {
            for (j = 0; j < w; j++) {
                val = GET_DATA_BYTE(linet, j);
                lined[j] = (l_float32)val;
            }
        } else if (d == 16) {
            for (j = 0; j < w; j++) {
                val = GET_DATA_TWO_BYTES(linet, j);
                lined[j] = (l_float32)val;
            }
        } else {  /* d == 32 */
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
 *  pixConvertToDPix()
 *
 *      Input:  pix (1, 2, 4, 8, 16 or 32 bpp)
 *              ncomps (number of components: 3 for RGB, 1 otherwise)
 *      Return: dpix, or null on error
 *
 *  Notes:
 *      (1) If colormapped, remove to grayscale.
 *      (2) If 32 bpp and @ncomps == 3, this is RGB; convert to luminance.
 *          In all other cases the src image is treated as having a single
 *          component of pixel values.
 */
DPIX *
pixConvertToDPix(PIX     *pixs,
                 l_int32  ncomps)
{
l_int32     w, h, d, i, j, val, wplt, wpld;
l_uint32    uval;
l_uint32   *datat, *linet;
l_float64  *datad, *lined;
PIX        *pixt;
DPIX       *dpixd;

    PROCNAME("pixConvertToDPix");

    if (!pixs)
        return (DPIX *)ERROR_PTR("pixs not defined", procName, NULL);

           /* Convert to a single component */
    if (pixGetColormap(pixs))
        pixt = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
    else if (pixGetDepth(pixs) == 32 && ncomps == 3)
        pixt = pixConvertRGBToLuminance(pixs);
    else
        pixt = pixClone(pixs);
    pixGetDimensions(pixt, &w, &h, &d);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32) {
        pixDestroy(&pixt);
        return (DPIX *)ERROR_PTR("invalid depth", procName, NULL);
    }

    if ((dpixd = dpixCreate(w, h)) == NULL)
        return (DPIX *)ERROR_PTR("dpixd not made", procName, NULL);
    datat = pixGetData(pixt);
    wplt = pixGetWpl(pixt);
    datad = dpixGetData(dpixd);
    wpld = dpixGetWpl(dpixd);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        lined = datad + i * wpld;
        if (d == 1) {
            for (j = 0; j < w; j++) {
                val = GET_DATA_BIT(linet, j);
                lined[j] = (l_float64)val;
            }
        } else if (d == 2) {
            for (j = 0; j < w; j++) {
                val = GET_DATA_DIBIT(linet, j);
                lined[j] = (l_float64)val;
            }
        } else if (d == 4) {
            for (j = 0; j < w; j++) {
                val = GET_DATA_QBIT(linet, j);
                lined[j] = (l_float64)val;
            }
        } else if (d == 8) {
            for (j = 0; j < w; j++) {
                val = GET_DATA_BYTE(linet, j);
                lined[j] = (l_float64)val;
            }
        } else if (d == 16) {
            for (j = 0; j < w; j++) {
                val = GET_DATA_TWO_BYTES(linet, j);
                lined[j] = (l_float64)val;
            }
        } else {  /* d == 32 */
            for (j = 0; j < w; j++) {
                uval = GET_DATA_FOUR_BYTES(linet, j);
                lined[j] = (l_float64)uval;
            }
        }
    }

    pixDestroy(&pixt);
    return dpixd;
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
        for (i = 0; i < h && outdepth < 32; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < w && outdepth < 32; j++) {
                if (lines[j] > 65535.5)
                    outdepth = 32;
                else if (lines[j] > 255.5)
                    outdepth = 16;
            }
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
            L_ERROR("Number of negative values: %d\n", procName, negs);
        if (overvals > 0)
            L_ERROR("Number of too-large values: %d\n", procName, overvals);
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
            else if (negvals == L_CLIP_TO_ZERO)  /* and val < 0.0 */
                vald = 0;
            else
                vald = (l_uint32)(-val + 0.5);
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
 *  dpixConvertToPix()
 *
 *      Input:  dpixs
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
dpixConvertToPix(DPIX    *dpixs,
                 l_int32  outdepth,
                 l_int32  negvals,
                 l_int32  errorflag)
{
l_int32     w, h, i, j, wpls, wpld, maxval;
l_uint32    vald;
l_float64   val;
l_float64  *datas, *lines;
l_uint32   *datad, *lined;
PIX        *pixd;

    PROCNAME("dpixConvertToPix");

    if (!dpixs)
        return (PIX *)ERROR_PTR("dpixs not defined", procName, NULL);
    if (negvals != L_CLIP_TO_ZERO && negvals != L_TAKE_ABSVAL)
        return (PIX *)ERROR_PTR("invalid negvals", procName, NULL);
    if (outdepth != 0 && outdepth != 8 && outdepth != 16 && outdepth != 32)
        return (PIX *)ERROR_PTR("outdepth not in {0,8,16,32}", procName, NULL);

    dpixGetDimensions(dpixs, &w, &h);
    datas = dpixGetData(dpixs);
    wpls = dpixGetWpl(dpixs);

        /* Adaptive determination of output depth */
    if (outdepth == 0) {
        outdepth = 8;
        for (i = 0; i < h && outdepth < 32; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < w && outdepth < 32; j++) {
                if (lines[j] > 65535.5)
                    outdepth = 32;
                else if (lines[j] > 255.5)
                    outdepth = 16;
            }
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
            L_ERROR("Number of negative values: %d\n", procName, negs);
        if (overvals > 0)
            L_ERROR("Number of too-large values: %d\n", procName, overvals);
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
            if (val >= 0.0) {
                vald = (l_uint32)(val + 0.5);
            } else {  /* val < 0.0 */
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
            if (line[j] > maxval) {
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


/*!
 *  dpixGetMin()
 *
 *      Input:  dpix
 *              &minval (<optional return> min value)
 *              &xminloc (<optional return> x location of min)
 *              &yminloc (<optional return> y location of min)
 *      Return: 0 if OK; 1 on error
 */
l_int32
dpixGetMin(DPIX       *dpix,
           l_float64  *pminval,
           l_int32    *pxminloc,
           l_int32    *pyminloc)
{
l_int32     i, j, w, h, wpl, xminloc, yminloc;
l_float64  *data, *line;
l_float64   minval;

    PROCNAME("dpixGetMin");

    if (!pminval && !pxminloc && !pyminloc)
        return ERROR_INT("nothing to do", procName, 1);
    if (pminval) *pminval = 0.0;
    if (pxminloc) *pxminloc = 0;
    if (pyminloc) *pyminloc = 0;
    if (!dpix)
        return ERROR_INT("dpix not defined", procName, 1);

    minval = +1.0e300;
    xminloc = 0;
    yminloc = 0;
    dpixGetDimensions(dpix, &w, &h);
    data = dpixGetData(dpix);
    wpl = dpixGetWpl(dpix);
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
 *  dpixGetMax()
 *
 *      Input:  dpix
 *              &maxval (<optional return> max value)
 *              &xmaxloc (<optional return> x location of max)
 *              &ymaxloc (<optional return> y location of max)
 *      Return: 0 if OK; 1 on error
 */
l_int32
dpixGetMax(DPIX       *dpix,
           l_float64  *pmaxval,
           l_int32    *pxmaxloc,
           l_int32    *pymaxloc)
{
l_int32     i, j, w, h, wpl, xmaxloc, ymaxloc;
l_float64  *data, *line;
l_float64   maxval;

    PROCNAME("dpixGetMax");

    if (!pmaxval && !pxmaxloc && !pymaxloc)
        return ERROR_INT("nothing to do", procName, 1);
    if (pmaxval) *pmaxval = 0.0;
    if (pxmaxloc) *pxmaxloc = 0;
    if (pymaxloc) *pymaxloc = 0;
    if (!dpix)
        return ERROR_INT("dpix not defined", procName, 1);

    maxval = -1.0e20;
    xmaxloc = 0;
    ymaxloc = 0;
    dpixGetDimensions(dpix, &w, &h);
    data = dpixGetData(dpix);
    wpl = dpixGetWpl(dpix);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < w; j++) {
            if (line[j] > maxval) {
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
                     lined[j * factor + m] =
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
                     lined[j * factor + m] =
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
 *  fpixLinearCombination()
 *
 *      Input:  fpixd (<optional>; this can be null, equal to fpixs1, or
 *                     different from fpixs1)
 *              fpixs1 (can be == to fpixd)
 *              fpixs2
 *              a, b (multiplication factors on fpixs1 and fpixs2, rsp.)
 *      Return: fpixd always
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
        for (j = 0; j < w; j++)
            lined[j] = a * lined[j] + b * lines[j];
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
                line[j] *= multc;
        } else if (multc == 1.0) {
            for (j = 0; j < w; j++)
                line[j] += addc;
        } else {
            for (j = 0; j < w; j++) {
                line[j] = multc * line[j] + addc;
            }
        }
    }

    return 0;
}


/*!
 *  dpixLinearCombination()
 *
 *      Input:  dpixd (<optional>; this can be null, equal to dpixs1, or
 *                     different from dpixs1)
 *              dpixs1 (can be == to dpixd)
 *              dpixs2
 *              a, b (multiplication factors on dpixs1 and dpixs2, rsp.)
 *      Return: dpixd always
 *
 *  Notes:
 *      (1) Computes pixelwise linear combination: a * src1 + b * src2
 *      (2) Alignment is to UL corner.
 *      (3) There are 3 cases.  The result can go to a new dest,
 *          in-place to dpixs1, or to an existing input dest:
 *          * dpixd == null:   (src1 + src2) --> new dpixd
 *          * dpixd == dpixs1:  (src1 + src2) --> src1  (in-place)
 *          * dpixd != dpixs1: (src1 + src2) --> input dpixd
 *      (4) dpixs2 must be different from both dpixd and dpixs1.
 */
DPIX *
dpixLinearCombination(DPIX      *dpixd,
                      DPIX      *dpixs1,
                      DPIX      *dpixs2,
                      l_float32  a,
                      l_float32  b)
{
l_int32     i, j, ws, hs, w, h, wpls, wpld;
l_float64  *datas, *datad, *lines, *lined;

    PROCNAME("dpixLinearCombination");

    if (!dpixs1)
        return (DPIX *)ERROR_PTR("dpixs1 not defined", procName, dpixd);
    if (!dpixs2)
        return (DPIX *)ERROR_PTR("dpixs2 not defined", procName, dpixd);
    if (dpixs1 == dpixs2)
        return (DPIX *)ERROR_PTR("dpixs1 == dpixs2", procName, dpixd);
    if (dpixs2 == dpixd)
        return (DPIX *)ERROR_PTR("dpixs2 == dpixd", procName, dpixd);

    if (dpixs1 != dpixd)
        dpixd = dpixCopy(dpixd, dpixs1);

    datas = dpixGetData(dpixs2);
    datad = dpixGetData(dpixd);
    wpls = dpixGetWpl(dpixs2);
    wpld = dpixGetWpl(dpixd);
    dpixGetDimensions(dpixs2, &ws, &hs);
    dpixGetDimensions(dpixd, &w, &h);
    w = L_MIN(ws, w);
    h = L_MIN(hs, h);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++)
            lined[j] = a * lined[j] + b * lines[j];
    }

    return dpixd;
}


/*!
 *  dpixAddMultConstant()
 *
 *      Input:  dpix
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
dpixAddMultConstant(DPIX      *dpix,
                    l_float64  addc,
                    l_float64  multc)
{
l_int32     i, j, w, h, wpl;
l_float64  *line, *data;

    PROCNAME("dpixAddMultConstant");

    if (!dpix)
        return ERROR_INT("dpix not defined", procName, 1);

    if (addc == 0.0 && multc == 1.0)
        return 0;

    dpixGetDimensions(dpix, &w, &h);
    data = dpixGetData(dpix);
    wpl = dpixGetWpl(dpix);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        if (addc == 0.0) {
            for (j = 0; j < w; j++)
                line[j] *= multc;
        } else if (multc == 1.0) {
            for (j = 0; j < w; j++)
                line[j] += addc;
        } else {
            for (j = 0; j < w; j++)
                line[j] = multc * line[j] + addc;
        }
    }

    return 0;
}


/*--------------------------------------------------------------------*
 *                              Set all                               *
 *--------------------------------------------------------------------*/
/*!
 *  fpixSetAllArbitrary()
 *
 *      Input:  fpix
 *              val (to set at each pixel)
 *      Return: 0 if OK, 1 on error
 */
l_int32
fpixSetAllArbitrary(FPIX      *fpix,
                    l_float32  inval)
{
l_int32     i, j, w, h;
l_float32  *data, *line;

    PROCNAME("fpixSetAllArbitrary");

    if (!fpix)
        return ERROR_INT("fpix not defined", procName, 1);

    fpixGetDimensions(fpix, &w, &h);
    data = fpixGetData(fpix);
    for (i = 0; i < h; i++) {
        line = data + i * w;
        for (j = 0; j < w; j++)
            *(line + j) = inval;
    }

    return 0;
}


/*!
 *  dpixSetAllArbitrary()
 *
 *      Input:  dpix
 *              val (to set at each pixel)
 *      Return: 0 if OK, 1 on error
 */
l_int32
dpixSetAllArbitrary(DPIX      *dpix,
                    l_float64  inval)
{
l_int32     i, j, w, h;
l_float64  *data, *line;

    PROCNAME("dpixSetAllArbitrary");

    if (!dpix)
        return ERROR_INT("dpix not defined", procName, 1);

    dpixGetDimensions(dpix, &w, &h);
    data = dpixGetData(dpix);
    for (i = 0; i < h; i++) {
        line = data + i * w;
        for (j = 0; j < w; j++)
            *(line + j) = inval;
    }

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


/*!
 *  fpixAddContinuedBorder()
 *
 *      Input:  fpixs
 *              left, right, top, bot (pixels on each side to be added)
 *      Return: fpixd, or null on error
 *
 *  Notes:
 *      (1) This adds pixels on each side whose values are equal to
 *          the value on the closest boundary pixel.
 */
FPIX *
fpixAddContinuedBorder(FPIX    *fpixs,
                       l_int32  left,
                       l_int32  right,
                       l_int32  top,
                       l_int32  bot)
{
l_int32  i, j, w, h;
FPIX    *fpixd;

    PROCNAME("fpixAddContinuedBorder");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);

    fpixd = fpixAddBorder(fpixs, left, right, top, bot);
    fpixGetDimensions(fpixs, &w, &h);
    for (j = 0; j < left; j++)
        fpixRasterop(fpixd, j, top, 1, h, fpixd, left, top);
    for (j = 0; j < right; j++)
        fpixRasterop(fpixd, left + w + j, top, 1, h, fpixd, left + w - 1, top);
    for (i = 0; i < top; i++)
        fpixRasterop(fpixd, 0, i, left + w + right, 1, fpixd, 0, top);
    for (i = 0; i < bot; i++)
        fpixRasterop(fpixd, 0, top + h + i, left + w + right, 1,
                     fpixd, 0, top + h - 1);

    return fpixd;
}


/*!
 *  fpixAddSlopeBorder()
 *
 *      Input:  fpixs
 *              left, right, top, bot (pixels on each side to be added)
 *      Return: fpixd, or null on error
 *
 *  Notes:
 *      (1) This adds pixels on each side whose values have a normal
 *          derivative equal to the normal derivative at the boundary
 *          of fpixs.
 */
FPIX *
fpixAddSlopeBorder(FPIX    *fpixs,
                   l_int32  left,
                   l_int32  right,
                   l_int32  top,
                   l_int32  bot)
{
l_int32    i, j, w, h, fullw, fullh;
l_float32  val1, val2, del;
FPIX      *fpixd;

    PROCNAME("fpixAddSlopeBorder");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);

    fpixd = fpixAddBorder(fpixs, left, right, top, bot);
    fpixGetDimensions(fpixs, &w, &h);

        /* Left */
    for (i = top; i < top + h; i++) {
        fpixGetPixel(fpixd, left, i, &val1);
        fpixGetPixel(fpixd, left + 1, i, &val2);
        del = val1 - val2;
        for (j = 0; j < left; j++)
            fpixSetPixel(fpixd, j, i, val1 + del * (left - j));
    }

        /* Right */
    fullw = left + w + right;
    for (i = top; i < top + h; i++) {
        fpixGetPixel(fpixd, left + w - 1, i, &val1);
        fpixGetPixel(fpixd, left + w - 2, i, &val2);
        del = val1 - val2;
        for (j = left + w; j < fullw; j++)
            fpixSetPixel(fpixd, j, i, val1 + del * (j - left - w + 1));
    }

        /* Top */
    for (j = 0; j < fullw; j++) {
        fpixGetPixel(fpixd, j, top, &val1);
        fpixGetPixel(fpixd, j, top + 1, &val2);
        del = val1 - val2;
        for (i = 0; i < top; i++)
            fpixSetPixel(fpixd, j, i, val1 + del * (top - i));
    }

        /* Bottom */
    fullh = top + h + bot;
    for (j = 0; j < fullw; j++) {
        fpixGetPixel(fpixd, j, top + h - 1, &val1);
        fpixGetPixel(fpixd, j, top + h - 2, &val2);
        del = val1 - val2;
        for (i = top + h; i < fullh; i++)
            fpixSetPixel(fpixd, j, i, val1 + del * (i - top - h + 1));
    }

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
 *                   Rotation by multiples of 90 degrees              *
 *--------------------------------------------------------------------*/
/*!
 *  fpixRotateOrth()
 *
 *      Input:  fpixs
 *              quads (0-3; number of 90 degree cw rotations)
 *      Return: fpixd, or null on error
 */
FPIX *
fpixRotateOrth(FPIX     *fpixs,
               l_int32  quads)
{
    PROCNAME("fpixRotateOrth");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);
    if (quads < 0 || quads > 3)
        return (FPIX *)ERROR_PTR("quads not in {0,1,2,3}", procName, NULL);

    if (quads == 0)
        return fpixCopy(NULL, fpixs);
    else if (quads == 1)
        return fpixRotate90(fpixs, 1);
    else if (quads == 2)
        return fpixRotate180(NULL, fpixs);
    else /* quads == 3 */
        return fpixRotate90(fpixs, -1);
}


/*!
 *  fpixRotate180()
 *
 *      Input:  fpixd  (<optional>; can be null, equal to fpixs,
 *                      or different from fpixs)
 *              fpixs
 *      Return: fpixd, or null on error
 *
 *  Notes:
 *      (1) This does a 180 rotation of the image about the center,
 *          which is equivalent to a left-right flip about a vertical
 *          line through the image center, followed by a top-bottom
 *          flip about a horizontal line through the image center.
 *      (2) There are 3 cases for input:
 *          (a) fpixd == null (creates a new fpixd)
 *          (b) fpixd == fpixs (in-place operation)
 *          (c) fpixd != fpixs (existing fpixd)
 *      (3) For clarity, use these three patterns, respectively:
 *          (a) fpixd = fpixRotate180(NULL, fpixs);
 *          (b) fpixRotate180(fpixs, fpixs);
 *          (c) fpixRotate180(fpixd, fpixs);
 */
FPIX *
fpixRotate180(FPIX  *fpixd,
              FPIX  *fpixs)
{
    PROCNAME("fpixRotate180");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);

        /* Prepare pixd for in-place operation */
    if ((fpixd = fpixCopy(fpixd, fpixs)) == NULL)
        return (FPIX *)ERROR_PTR("fpixd not made", procName, NULL);

    fpixFlipLR(fpixd, fpixd);
    fpixFlipTB(fpixd, fpixd);
    return fpixd;
}


/*!
 *  fpixRotate90()
 *
 *      Input:  fpixs
 *              direction (1 = clockwise,  -1 = counter-clockwise)
 *      Return: fpixd, or null on error
 *
 *  Notes:
 *      (1) This does a 90 degree rotation of the image about the center,
 *          either cw or ccw, returning a new pix.
 *      (2) The direction must be either 1 (cw) or -1 (ccw).
 */
FPIX *
fpixRotate90(FPIX    *fpixs,
             l_int32  direction)
{
l_int32     i, j, wd, hd, wpls, wpld;
l_float32  *datas, *datad, *lines, *lined;
FPIX       *fpixd;

    PROCNAME("fpixRotate90");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);
    if (direction != 1 && direction != -1)
        return (FPIX *)ERROR_PTR("invalid direction", procName, NULL);

    fpixGetDimensions(fpixs, &hd, &wd);
    if ((fpixd = fpixCreate(wd, hd)) == NULL)
        return (FPIX *)ERROR_PTR("fpixd not made", procName, NULL);
    fpixCopyResolution(fpixd, fpixs);

    datas = fpixGetData(fpixs);
    wpls = fpixGetWpl(fpixs);
    datad = fpixGetData(fpixd);
    wpld = fpixGetWpl(fpixd);
    if (direction == 1) {  /* clockwise */
        for (i = 0; i < hd; i++) {
            lined = datad + i * wpld;
            lines = datas + (wd - 1) * wpls;
            for (j = 0; j < wd; j++) {
                lined[j] = lines[i];
                lines -= wpls;
            }
        }
    } else {  /* ccw */
        for (i = 0; i < hd; i++) {
            lined = datad + i * wpld;
            lines = datas;
            for (j = 0; j < wd; j++) {
                lined[j] = lines[hd - 1 - i];
                lines += wpls;
            }
        }
    }

    return fpixd;
}


/*!
 *  pixFlipLR()
 *
 *      Input:  fpixd (<optional>; can be null, equal to fpixs,
 *                     or different from fpixs)
 *              fpixs
 *      Return: fpixd, or null on error
 *
 *  Notes:
 *      (1) This does a left-right flip of the image, which is
 *          equivalent to a rotation out of the plane about a
 *          vertical line through the image center.
 *      (2) There are 3 cases for input:
 *          (a) fpixd == null (creates a new fpixd)
 *          (b) fpixd == fpixs (in-place operation)
 *          (c) fpixd != fpixs (existing fpixd)
 *      (3) For clarity, use these three patterns, respectively:
 *          (a) fpixd = fpixFlipLR(NULL, fpixs);
 *          (b) fpixFlipLR(fpixs, fpixs);
 *          (c) fpixFlipLR(fpixd, fpixs);
 *      (4) If an existing fpixd is not the same size as fpixs, the
 *          image data will be reallocated.
 */
FPIX *
fpixFlipLR(FPIX  *fpixd,
           FPIX  *fpixs)
{
l_int32     i, j, w, h, wpl, bpl;
l_float32  *line, *data, *buffer;

    PROCNAME("fpixFlipLR");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);

    fpixGetDimensions(fpixs, &w, &h);

        /* Prepare fpixd for in-place operation */
    if ((fpixd = fpixCopy(fpixd, fpixs)) == NULL)
        return (FPIX *)ERROR_PTR("fpixd not made", procName, NULL);

    data = fpixGetData(fpixd);
    wpl = fpixGetWpl(fpixd);  /* 4-byte words */
    bpl = 4 * wpl;
    if ((buffer = (l_float32 *)CALLOC(wpl, sizeof(l_float32))) == NULL)
        return (FPIX *)ERROR_PTR("buffer not made", procName, NULL);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        memcpy(buffer, line, bpl);
        for (j = 0; j < w; j++)
            line[j] = buffer[w - 1 - j];
    }
    FREE(buffer);
    return fpixd;
}


/*!
 *  fpixFlipTB()
 *
 *      Input:  fpixd (<optional>; can be null, equal to fpixs,
 *                     or different from fpixs)
 *              fpixs
 *      Return: fpixd, or null on error
 *
 *  Notes:
 *      (1) This does a top-bottom flip of the image, which is
 *          equivalent to a rotation out of the plane about a
 *          horizontal line through the image center.
 *      (2) There are 3 cases for input:
 *          (a) fpixd == null (creates a new fpixd)
 *          (b) fpixd == fpixs (in-place operation)
 *          (c) fpixd != fpixs (existing fpixd)
 *      (3) For clarity, use these three patterns, respectively:
 *          (a) fpixd = fpixFlipTB(NULL, fpixs);
 *          (b) fpixFlipTB(fpixs, fpixs);
 *          (c) fpixFlipTB(fpixd, fpixs);
 *      (4) If an existing fpixd is not the same size as fpixs, the
 *          image data will be reallocated.
 */
FPIX *
fpixFlipTB(FPIX  *fpixd,
           FPIX  *fpixs)
{
l_int32     i, k, h, h2, wpl, bpl;
l_float32  *linet, *lineb, *data, *buffer;

    PROCNAME("fpixFlipTB");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);

        /* Prepare fpixd for in-place operation */
    if ((fpixd = fpixCopy(fpixd, fpixs)) == NULL)
        return (FPIX *)ERROR_PTR("fpixd not made", procName, NULL);

    data = fpixGetData(fpixd);
    wpl = fpixGetWpl(fpixd);
    fpixGetDimensions(fpixd, NULL, &h);
    if ((buffer = (l_float32 *)CALLOC(wpl, sizeof(l_float32))) == NULL)
        return (FPIX *)ERROR_PTR("buffer not made", procName, NULL);
    h2 = h / 2;
    bpl = 4 * wpl;
    for (i = 0, k = h - 1; i < h2; i++, k--) {
        linet = data + i * wpl;
        lineb = data + k * wpl;
        memcpy(buffer, linet, bpl);
        memcpy(linet, lineb, bpl);
        memcpy(lineb, buffer, bpl);
    }
    FREE(buffer);
    return fpixd;
}


/*--------------------------------------------------------------------*
 *            Affine and projective interpolated transforms           *
 *--------------------------------------------------------------------*/
/*!
 *  fpixAffinePta()
 *
 *      Input:  fpixs (8 bpp)
 *              ptad  (4 pts of final coordinate space)
 *              ptas  (4 pts of initial coordinate space)
 *              border (size of extension with constant normal derivative)
 *              inval (value brought in; typ. 0)
 *      Return: fpixd, or null on error
 *
 *  Notes:
 *      (1) If @border > 0, all four sides are extended by that distance,
 *          and removed after the transformation is finished.  Pixels
 *          that would be brought in to the trimmed result from outside
 *          the extended region are assigned @inval.  The purpose of
 *          extending the image is to avoid such assignments.
 *      (2) On the other hand, you may want to give all pixels that
 *          are brought in from outside fpixs a specific value.  In that
 *          case, set @border == 0.
 */
FPIX *
fpixAffinePta(FPIX      *fpixs,
              PTA       *ptad,
              PTA       *ptas,
              l_int32    border,
              l_float32  inval)
{
l_float32  *vc;
PTA        *ptas2, *ptad2;
FPIX       *fpixs2, *fpixd, *fpixd2;

    PROCNAME("fpixAffinePta");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);
    if (!ptas)
        return (FPIX *)ERROR_PTR("ptas not defined", procName, NULL);
    if (!ptad)
        return (FPIX *)ERROR_PTR("ptad not defined", procName, NULL);

        /* If a border is to be added, also translate the ptas */
    if (border > 0) {
        ptas2 = ptaTransform(ptas, border, border, 1.0, 1.0);
        ptad2 = ptaTransform(ptad, border, border, 1.0, 1.0);
        fpixs2 = fpixAddSlopeBorder(fpixs, border, border, border, border);
    } else {
        ptas2 = ptaClone(ptas);
        ptad2 = ptaClone(ptad);
        fpixs2 = fpixClone(fpixs);
    }

        /* Get backwards transform from dest to src, and apply it */
    getAffineXformCoeffs(ptad2, ptas2, &vc);
    fpixd2 = fpixAffine(fpixs2, vc, inval);
    fpixDestroy(&fpixs2);
    ptaDestroy(&ptas2);
    ptaDestroy(&ptad2);
    FREE(vc);

    if (border == 0)
        return fpixd2;

        /* Remove the added border */
    fpixd = fpixRemoveBorder(fpixd2, border, border, border, border);
    fpixDestroy(&fpixd2);
    return fpixd;
}


/*!
 *  fpixAffine()
 *
 *      Input:  fpixs (8 bpp)
 *              vc  (vector of 8 coefficients for projective transformation)
 *              inval (value brought in; typ. 0)
 *      Return: fpixd, or null on error
 */
FPIX *
fpixAffine(FPIX       *fpixs,
           l_float32  *vc,
           l_float32   inval)
{
l_int32     i, j, w, h, wpld;
l_float32   val;
l_float32  *datas, *datad, *lined;
l_float32   x, y;
FPIX       *fpixd;

    PROCNAME("fpixAffine");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);
    fpixGetDimensions(fpixs, &w, &h);
    if (!vc)
        return (FPIX *)ERROR_PTR("vc not defined", procName, NULL);

    datas = fpixGetData(fpixs);
    fpixd = fpixCreateTemplate(fpixs);
    fpixSetAllArbitrary(fpixd, inval);
    datad = fpixGetData(fpixd);
    wpld = fpixGetWpl(fpixd);

        /* Iterate over destination pixels */
    for (i = 0; i < h; i++) {
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
                /* Compute float src pixel location corresponding to (i,j) */
            affineXformPt(vc, j, i, &x, &y);
            linearInterpolatePixelFloat(datas, w, h, x, y, inval, &val);
            *(lined + j) = val;
        }
    }

    return fpixd;
}


/*!
 *  fpixProjectivePta()
 *
 *      Input:  fpixs (8 bpp)
 *              ptad  (4 pts of final coordinate space)
 *              ptas  (4 pts of initial coordinate space)
 *              border (size of extension with constant normal derivative)
 *              inval (value brought in; typ. 0)
 *      Return: fpixd, or null on error
 *
 *  Notes:
 *      (1) If @border > 0, all four sides are extended by that distance,
 *          and removed after the transformation is finished.  Pixels
 *          that would be brought in to the trimmed result from outside
 *          the extended region are assigned @inval.  The purpose of
 *          extending the image is to avoid such assignments.
 *      (2) On the other hand, you may want to give all pixels that
 *          are brought in from outside fpixs a specific value.  In that
 *          case, set @border == 0.
 */
FPIX *
fpixProjectivePta(FPIX      *fpixs,
                  PTA       *ptad,
                  PTA       *ptas,
                  l_int32    border,
                  l_float32  inval)
{
l_float32  *vc;
PTA        *ptas2, *ptad2;
FPIX       *fpixs2, *fpixd, *fpixd2;

    PROCNAME("fpixProjectivePta");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);
    if (!ptas)
        return (FPIX *)ERROR_PTR("ptas not defined", procName, NULL);
    if (!ptad)
        return (FPIX *)ERROR_PTR("ptad not defined", procName, NULL);

        /* If a border is to be added, also translate the ptas */
    if (border > 0) {
        ptas2 = ptaTransform(ptas, border, border, 1.0, 1.0);
        ptad2 = ptaTransform(ptad, border, border, 1.0, 1.0);
        fpixs2 = fpixAddSlopeBorder(fpixs, border, border, border, border);
    } else {
        ptas2 = ptaClone(ptas);
        ptad2 = ptaClone(ptad);
        fpixs2 = fpixClone(fpixs);
    }

        /* Get backwards transform from dest to src, and apply it */
    getProjectiveXformCoeffs(ptad2, ptas2, &vc);
    fpixd2 = fpixProjective(fpixs2, vc, inval);
    fpixDestroy(&fpixs2);
    ptaDestroy(&ptas2);
    ptaDestroy(&ptad2);
    FREE(vc);

    if (border == 0)
        return fpixd2;

        /* Remove the added border */
    fpixd = fpixRemoveBorder(fpixd2, border, border, border, border);
    fpixDestroy(&fpixd2);
    return fpixd;
}


/*!
 *  fpixProjective()
 *
 *      Input:  fpixs (8 bpp)
 *              vc  (vector of 8 coefficients for projective transformation)
 *              inval (value brought in; typ. 0)
 *      Return: fpixd, or null on error
 */
FPIX *
fpixProjective(FPIX       *fpixs,
               l_float32  *vc,
               l_float32   inval)
{
l_int32     i, j, w, h, wpld;
l_float32   val;
l_float32  *datas, *datad, *lined;
l_float32   x, y;
FPIX       *fpixd;

    PROCNAME("fpixProjective");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);
    fpixGetDimensions(fpixs, &w, &h);
    if (!vc)
        return (FPIX *)ERROR_PTR("vc not defined", procName, NULL);

    datas = fpixGetData(fpixs);
    fpixd = fpixCreateTemplate(fpixs);
    fpixSetAllArbitrary(fpixd, inval);
    datad = fpixGetData(fpixd);
    wpld = fpixGetWpl(fpixd);

        /* Iterate over destination pixels */
    for (i = 0; i < h; i++) {
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
                /* Compute float src pixel location corresponding to (i,j) */
            projectiveXformPt(vc, j, i, &x, &y);
            linearInterpolatePixelFloat(datas, w, h, x, y, inval, &val);
            *(lined + j) = val;
        }
    }

    return fpixd;
}


/*!
 *  linearInterpolatePixelFloat()
 *
 *      Input:  datas (ptr to beginning of float image data)
 *              wpls (32-bit word/line for this data array)
 *              w, h (of image)
 *              x, y (floating pt location for evaluation)
 *              inval (float value brought in from the outside when the
 *                     input x,y location is outside the image)
 *              &val (<return> interpolated float value)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is a standard linear interpolation function.  It is
 *          equivalent to area weighting on each component, and
 *          avoids "jaggies" when rendering sharp edges.
 */
l_int32
linearInterpolatePixelFloat(l_float32  *datas,
                            l_int32     w,
                            l_int32     h,
                            l_float32   x,
                            l_float32   y,
                            l_float32   inval,
                            l_float32  *pval)
{
l_int32     xpm, ypm, xp, yp, xf, yf;
l_float32   v00, v01, v10, v11;
l_float32  *lines;

    PROCNAME("linearInterpolatePixelFloat");

    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = inval;
    if (!datas)
        return ERROR_INT("datas not defined", procName, 1);

        /* Skip if off the edge */
    if (x < 0.0 || y < 0.0 || x > w - 2.0 || y > h - 2.0)
        return 0;

    xpm = (l_int32)(16.0 * x + 0.5);
    ypm = (l_int32)(16.0 * y + 0.5);
    xp = xpm >> 4;
    yp = ypm >> 4;
    xf = xpm & 0x0f;
    yf = ypm & 0x0f;

#if  DEBUG
    if (xf < 0 || yf < 0)
        fprintf(stderr, "xp = %d, yp = %d, xf = %d, yf = %d\n", xp, yp, xf, yf);
#endif  /* DEBUG */

        /* Interpolate by area weighting. */
    lines = datas + yp * w;
    v00 = (16.0 - xf) * (16.0 - yf) * (*(lines + xp));
    v10 = xf * (16.0 - yf) * (*(lines + xp + 1));
    v01 = (16.0 - xf) * yf * (*(lines + w + xp));
    v11 = xf * yf * (*(lines + w + xp + 1));
    *pval = (v00 + v01 + v10 + v11) / 256.0;
    return 0;
}


/*--------------------------------------------------------------------*
 *                      Thresholding to 1 bpp Pix                     *
 *--------------------------------------------------------------------*/
/*!
 *  fpixThresholdToPix()
 *
 *      Input:  fpix
 *              thresh
 *      Return: pixd (1 bpp), or null on error
 *
 *  Notes:
 *      (1) For all values of fpix that are <= thresh, sets the pixel
 *          in pixd to 1.
 */
PIX *
fpixThresholdToPix(FPIX      *fpix,
                   l_float32  thresh)
{
l_int32     i, j, w, h, wpls, wpld;
l_float32  *datas, *lines;
l_uint32   *datad, *lined;
PIX        *pixd;

    PROCNAME("fpixThresholdToPix");

    if (!fpix)
        return (PIX *)ERROR_PTR("fpix not defined", procName, NULL);

    fpixGetDimensions(fpix, &w, &h);
    datas = fpixGetData(fpix);
    wpls = fpixGetWpl(fpix);
    pixd = pixCreate(w, h, 1);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            if (lines[j] <= thresh)
                SET_DATA_BIT(lined, j);
        }
    }

    return pixd;
}


/*--------------------------------------------------------------------*
 *                Generate function from components                   *
 *--------------------------------------------------------------------*/
/*!
 *  pixComponentFunction()
 *
 *      Input:  pix (32 bpp rgb)
 *              rnum, gnum, bnum (coefficients for numerator)
 *              rdenom, gdenom, bdenom (coefficients for denominator)
 *      Return: fpixd, or null on error
 *
 *  Notes:
 *      (1) This stores a function of the component values of each
 *          input pixel in @fpixd.
 *      (2) The function is a ratio of linear combinations of component values.
 *          There are two special cases for denominator coefficients:
 *          (a) The denominator is 1.0: input 0 for all denominator coefficients
 *          (b) Only one component is used in the denominator: input 1.0
 *              for that denominator component and 0.0 for the other two.
 *      (3) If the denominator is 0, multiply by an arbitrary number that
 *          is much larger than 1.  Choose 256 "arbitrarily".
 *
 */
FPIX *
pixComponentFunction(PIX       *pix,
                     l_float32  rnum,
                     l_float32  gnum,
                     l_float32  bnum,
                     l_float32  rdenom,
                     l_float32  gdenom,
                     l_float32  bdenom)
{
l_int32     i, j, w, h, wpls, wpld, rval, gval, bval, zerodenom, onedenom;
l_float32   fnum, fdenom;
l_uint32   *datas, *lines;
l_float32  *datad, *lined, *recip;
FPIX       *fpixd;

    PROCNAME("pixComponentFunction");

    if (!pix || pixGetDepth(pix) != 32)
        return (FPIX *)ERROR_PTR("pix undefined or not 32 bpp", procName, NULL);

    pixGetDimensions(pix, &w, &h, NULL);
    datas = pixGetData(pix);
    wpls = pixGetWpl(pix);
    fpixd = fpixCreate(w, h);
    datad = fpixGetData(fpixd);
    wpld = fpixGetWpl(fpixd);
    zerodenom = (rdenom == 0.0 && gdenom == 0.0 && bdenom == 0.0) ? 1: 0;
    onedenom = ((rdenom == 1.0 && gdenom == 0.0 && bdenom == 0.0) ||
                (rdenom == 0.0 && gdenom == 1.0 && bdenom == 0.0) ||
                (rdenom == 0.0 && gdenom == 0.0 && bdenom == 1.0)) ? 1 : 0;
    recip = NULL;
    if (onedenom) {
        recip = (l_float32 *)CALLOC(256, sizeof(l_float32));
        recip[0] = 256;  /* arbitrary large number */
        for (i = 1; i < 256; i++)
            recip[i] = 1.0 / (l_float32)i;
    }
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        if (zerodenom) {
            for (j = 0; j < w; j++) {
                extractRGBValues(lines[j], &rval, &gval, &bval);
                lined[j] = rnum * rval + gnum * gval + bnum * bval;
            }
        } else if (onedenom && rdenom == 1.0) {
            for (j = 0; j < w; j++) {
                extractRGBValues(lines[j], &rval, &gval, &bval);
                lined[j]
                    = recip[rval] * (rnum * rval + gnum * gval + bnum * bval);
            }
        } else if (onedenom && gdenom == 1.0) {
            for (j = 0; j < w; j++) {
                extractRGBValues(lines[j], &rval, &gval, &bval);
                lined[j]
                    = recip[gval] * (rnum * rval + gnum * gval + bnum * bval);
            }
        } else if (onedenom && bdenom == 1.0) {
            for (j = 0; j < w; j++) {
                extractRGBValues(lines[j], &rval, &gval, &bval);
                lined[j]
                    = recip[bval] * (rnum * rval + gnum * gval + bnum * bval);
            }
        } else {  /* general case */
            for (j = 0; j < w; j++) {
                extractRGBValues(lines[j], &rval, &gval, &bval);
                fnum = rnum * rval + gnum * gval + bnum * bval;
                fdenom = rdenom * rval + gdenom * gval + bdenom * bval;
                lined[j] = (fdenom == 0) ? 256.0 * fnum : fnum / fdenom;
            }
        }
    }

    FREE(recip);
    return fpixd;
}

