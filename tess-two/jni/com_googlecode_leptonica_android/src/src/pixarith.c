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
 *  pixarith.c
 *
 *      One-image grayscale arithmetic operations (8, 16, 32 bpp)
 *           l_int32     pixAddConstantGray()
 *           l_int32     pixMultConstantGray()
 *
 *      Two-image grayscale arithmetic operations (8, 16, 32 bpp)
 *           PIX        *pixAddGray()
 *           PIX        *pixSubtractGray()
 *
 *      Grayscale threshold operation (8, 16, 32 bpp)
 *           PIX        *pixThresholdToValue()
 *
 *      Image accumulator arithmetic operations
 *           PIX        *pixInitAccumulate()
 *           PIX        *pixFinalAccumulate()
 *           PIX        *pixFinalAccumulateThreshold()
 *           l_int32     pixAccumulate() 
 *           l_int32     pixMultConstAccumulate()
 *
 *      Absolute value of difference
 *           PIX        *pixAbsDifference()
 *
 *      Two-image min and max operations (8 and 16 bpp)
 *           PIX        *pixMinOrMax()
 *
 *      Scale pix for maximum dynamic range in 8 bpp image:
 *           PIX        *pixMaxDynamicRange()
 *
 *      Log base2 lookup
 *           l_float32  *makeLogBase2Tab()
 *           l_float32   getLogBase2()
 *
 *      The image accumulator operations are used when you expect
 *      overflow from 8 bits on intermediate results.  For example,
 *      you might want a tophat contrast operator which is
 *         3*I - opening(I,S) - closing(I,S)
 *      To use these operations, first use the init to generate
 *      a 16 bpp image, use the accumulate to add or subtract 8 bpp
 *      images from that, or the multiply constant to multiply
 *      by a small constant (much less than 256 -- we don't want
 *      overflow from the 16 bit images!), and when you're finished
 *      use final to bring the result back to 8 bpp, clipped
 *      if necessary.  There is also a divide function, which
 *      can be used to divide one image by another, scaling the
 *      result for maximum dynamic range, and giving back the
 *      8 bpp result.
 *
 *      A simpler interface to the arithmetic operations is
 *      provided in pixacc.c.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "allheaders.h"


/*-------------------------------------------------------------*
 *          One-image grayscale arithmetic operations          *
 *-------------------------------------------------------------*/
/*!
 *  pixAddConstantGray()
 *
 *      Input:  pixs (8, 16 or 32 bpp)
 *              val  (amount to add to each pixel)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) In-place operation.
 *      (2) No clipping for 32 bpp.
 *      (3) For 8 and 16 bpp, if val > 0 the result is clipped
 *          to 0xff and 0xffff, rsp.
 *      (4) For 8 and 16 bpp, if val < 0 the result is clipped to 0.
 */
l_int32
pixAddConstantGray(PIX      *pixs,
                   l_int32   val)
{
l_int32    w, h, d, wpl;
l_uint32  *data;

    PROCNAME("pixAddConstantGray");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8 && d != 16 && d != 32)
        return ERROR_INT("pixs not 8, 16 or 32 bpp", procName, 1);

    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    addConstantGrayLow(data, w, h, d, wpl, val);

    return 0;
}


/*!
 *  pixMultConstantGray()
 *
 *      Input:  pixs (8, 16 or 32 bpp)
 *              val  (>= 0.0; amount to multiply by each pixel)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) In-place operation; val must be >= 0.
 *      (2) No clipping for 32 bpp.
 *      (3) For 8 and 16 bpp, the result is clipped to 0xff and 0xffff, rsp.
 */
l_int32
pixMultConstantGray(PIX       *pixs,
                    l_float32  val)
{
l_int32    w, h, d, wpl;
l_uint32  *data;

    PROCNAME("pixMultConstantGray");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8 && d != 16 && d != 32)
        return ERROR_INT("pixs not 8, 16 or 32 bpp", procName, 1);
    if (val < 0.0)
        return ERROR_INT("val < 0.0", procName, 1);

    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    multConstantGrayLow(data, w, h, d, wpl, val);

    return 0;
}

            
/*-------------------------------------------------------------*
 *             Two-image grayscale arithmetic ops              *
 *-------------------------------------------------------------*/
/*!
 *  pixAddGray()
 *
 *      Input:  pixd (<optional>; this can be null, equal to pixs1, or
 *                    different from pixs1)
 *              pixs1 (can be == to pixd)
 *              pixs2
 *      Return: pixd always
 *
 *  Notes:
 *      (1) Arithmetic addition of two 8, 16 or 32 bpp images.
 *      (2) For 8 and 16 bpp, we do explicit clipping to 0xff and 0xffff,
 *          respectively.
 *      (3) Alignment is to UL corner.
 *      (4) There are 3 cases.  The result can go to a new dest,
 *          in-place to pixs1, or to an existing input dest:
 *          * pixd == null:   (src1 + src2) --> new pixd
 *          * pixd == pixs1:  (src1 + src2) --> src1  (in-place)
 *          * pixd != pixs1:  (src1 + src2) --> input pixd
 *      (5) pixs2 must be different from both pixd and pixs1.
 */
PIX *
pixAddGray(PIX  *pixd,
           PIX  *pixs1,
           PIX  *pixs2)
{
l_int32    d, ws, hs, w, h, wpls, wpld;
l_uint32  *datas, *datad;

    PROCNAME("pixAddGray");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, pixd);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, pixd);
    if (pixs2 == pixs1)
        return (PIX *)ERROR_PTR("pixs2 and pixs1 must differ", procName, pixd);
    if (pixs2 == pixd)
        return (PIX *)ERROR_PTR("pixs2 and pixd must differ", procName, pixd);
    d = pixGetDepth(pixs1);
    if (d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("pix are not 8, 16 or 32 bpp", procName, pixd);
    if (pixGetDepth(pixs2) != d)
        return (PIX *)ERROR_PTR("depths differ (pixs1, pixs2)", procName, pixd);
    if (pixd && (pixGetDepth(pixd) != d))
        return (PIX *)ERROR_PTR("depths differ (pixs1, pixd)", procName, pixd);

    if (!pixSizesEqual(pixs1, pixs2))
        L_WARNING("pixs1 and pixs2 not equal in size", procName);
    if (pixd && !pixSizesEqual(pixs1, pixd))
        L_WARNING("pixs1 and pixd not equal in size", procName);

    if (pixs1 != pixd)
        pixd = pixCopy(pixd, pixs1);

        /* pixd + pixs2 ==> pixd  */
    datas = pixGetData(pixs2);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs2);
    wpld = pixGetWpl(pixd);
    pixGetDimensions(pixs2, &ws, &hs, NULL);
    pixGetDimensions(pixd, &w, &h, NULL);
    w = L_MIN(ws, w);
    h = L_MIN(hs, h);
    addGrayLow(datad, w, h, d, wpld, datas, wpls);

    return pixd;
}


/*!
 *  pixSubtractGray()
 *
 *      Input:  pixd (<optional>; this can be null, equal to pixs1, or
 *                    different from pixs1)
 *              pixs1 (can be == to pixd)
 *              pixs2
 *      Return: pixd always
 *
 *  Notes:
 *      (1) Arithmetic subtraction of two 8, 16 or 32 bpp images.
 *      (2) Source pixs2 is always subtracted from source pixs1.
 *      (3) Do explicit clipping to 0.
 *      (4) Alignment is to UL corner.
 *      (5) There are 3 cases.  The result can go to a new dest,
 *          in-place to pixs1, or to an existing input dest:
 *          (a) pixd == null   (src1 - src2) --> new pixd
 *          (b) pixd == pixs1  (src1 - src2) --> src1  (in-place)
 *          (d) pixd != pixs1  (src1 - src2) --> input pixd
 *      (6) pixs2 must be different from both pixd and pixs1.
 */
PIX *
pixSubtractGray(PIX  *pixd,
                PIX  *pixs1,
                PIX  *pixs2)
{
l_int32    w, h, ws, hs, d, wpls, wpld;
l_uint32  *datas, *datad;

    PROCNAME("pixSubtractGray");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, pixd);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, pixd);
    if (pixs2 == pixs1)
        return (PIX *)ERROR_PTR("pixs2 and pixs1 must differ", procName, pixd);
    if (pixs2 == pixd)
        return (PIX *)ERROR_PTR("pixs2 and pixd must differ", procName, pixd);
    d = pixGetDepth(pixs1);
    if (d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("pix are not 8, 16 or 32 bpp", procName, pixd);
    if (pixGetDepth(pixs2) != d)
        return (PIX *)ERROR_PTR("depths differ (pixs1, pixs2)", procName, pixd);
    if (pixd && (pixGetDepth(pixd) != d))
        return (PIX *)ERROR_PTR("depths differ (pixs1, pixd)", procName, pixd);

    if (!pixSizesEqual(pixs1, pixs2))
        L_WARNING("pixs1 and pixs2 not equal in size", procName);
    if (pixd && !pixSizesEqual(pixs1, pixd))
        L_WARNING("pixs1 and pixd not equal in size", procName);

    if (pixs1 != pixd)
        pixd = pixCopy(pixd, pixs1);

        /* pixd - pixs2 ==> pixd  */
    datas = pixGetData(pixs2);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs2);
    wpld = pixGetWpl(pixd);
    pixGetDimensions(pixs2, &ws, &hs, NULL);
    pixGetDimensions(pixd, &w, &h, NULL);
    w = L_MIN(ws, w);
    h = L_MIN(hs, h);
    subtractGrayLow(datad, w, h, d, wpld, datas, wpls);

    return pixd;
}


/*-------------------------------------------------------------*
 *                Grayscale threshold operation                *
 *-------------------------------------------------------------*/
/*!
 *  pixThresholdToValue()
 *
 *      Input:  pixd (<optional>; if not null, must be equal to pixs)
 *              pixs (8, 16, 32 bpp)
 *              threshval
 *              setval
 *      Return: pixd always
 *
 *  Notes:
 *    - operation can be in-place (pixs == pixd) or to a new pixd
 *    - if setval > threshval, sets pixels with a value >= threshval to setval
 *    - if setval < threshval, sets pixels with a value <= threshval to setval
 *    - if setval == threshval, no-op
 */
PIX *
pixThresholdToValue(PIX      *pixd,
                    PIX      *pixs,
                    l_int32   threshval,
                    l_int32   setval)
{
l_int32    w, h, d, wpld;
l_uint32  *datad;

    PROCNAME("pixThresholdToValue");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    d = pixGetDepth(pixs);
    if (d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("pixs not 8, 16 or 32 bpp", procName, pixd);
    if (pixd && (pixs != pixd))
        return (PIX *)ERROR_PTR("pixd exists and is not pixs", procName, pixd);
    if (threshval < 0 || setval < 0)
        return (PIX *)ERROR_PTR("threshval & setval not < 0", procName, pixd);
    if (d == 8 && setval > 255)
        return (PIX *)ERROR_PTR("setval > 255 for 8 bpp", procName, pixd);
    if (d == 16 && setval > 0xffff)
        return (PIX *)ERROR_PTR("setval > 0xffff for 16 bpp", procName, pixd);

    if (!pixd)
        pixd = pixCopy(NULL, pixs);
    if (setval == threshval) {
        L_WARNING("setval == threshval; no operation", procName);
        return pixd;
    }

    datad = pixGetData(pixd);
    pixGetDimensions(pixd, &w, &h, NULL);
    wpld = pixGetWpl(pixd);

    thresholdToValueLow(datad, w, h, d, wpld, threshval, setval);
    return pixd;
}



/*-------------------------------------------------------------*
 *            Image accumulator arithmetic operations          *
 *-------------------------------------------------------------*/
/*!
 *  pixInitAccumulate()
 *
 *      Input:  w, h (of accumulate array)
 *              offset (initialize the 32 bpp to have this
 *                      value; not more than 0x40000000)
 *      Return: pixd (32 bpp), or null on error
 *
 *  Notes:
 *      (1) The offset must be >= 0.
 *      (2) The offset is used so that we can do arithmetic
 *          with negative number results on l_uint32 data; it
 *          prevents the l_uint32 data from going negative.
 *      (3) Because we use l_int32 intermediate data results,
 *          these should never exceed the max of l_int32 (0x7fffffff).
 *          We do not permit the offset to be above 0x40000000,
 *          which is half way between 0 and the max of l_int32.
 *      (4) The same offset should be used for initialization,
 *          multiplication by a constant, and final extraction!
 *      (5) If you're only adding positive values, offset can be 0.
 */
PIX *
pixInitAccumulate(l_int32   w,
                  l_int32   h,
                  l_uint32  offset)
{
PIX  *pixd;

    PROCNAME("pixInitAccumulate");

    if ((pixd = pixCreate(w, h, 32)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    if (offset > 0x40000000)
        offset = 0x40000000;
    pixSetAllArbitrary(pixd, offset);
    return pixd;
}


/*!
 *  pixFinalAccumulate()
 *
 *      Input:  pixs (32 bpp)
 *              offset (same as used for initialization)
 *              depth  (8, 16 or 32 bpp, of destination)
 *      Return: pixd (8, 16 or 32 bpp), or null on error
 *
 *  Notes:
 *      (1) The offset must be >= 0 and should not exceed 0x40000000.
 *      (2) The offset is subtracted from the src 32 bpp image
 *      (3) For 8 bpp dest, the result is clipped to [0, 0xff]
 *      (4) For 16 bpp dest, the result is clipped to [0, 0xffff]
 */
PIX *
pixFinalAccumulate(PIX      *pixs,
                   l_uint32  offset,
                   l_int32   depth)
{
l_int32    w, h, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixFinalAccumulate");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs not 32 bpp", procName, NULL);
    if (depth != 8 && depth != 16 && depth != 32)
        return (PIX *)ERROR_PTR("dest depth not 8, 16, 32 bpp", procName, NULL);
    if (offset > 0x40000000)
        offset = 0x40000000;

    pixGetDimensions(pixs, &w, &h, NULL);
    if ((pixd = pixCreate(w, h, depth)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);  /* but how did pixs get it initially? */
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);

    finalAccumulateLow(datad, w, h, depth, wpld, datas, wpls, offset);
    return pixd;
}


/*!
 *  pixFinalAccumulateThreshold()
 *
 *      Input:  pixs (32 bpp)
 *              offset (same as used for initialization)
 *              threshold (values less than this are set in the destination)
 *      Return: pixd (1 bpp), or null on error
 *
 *  Notes:
 *      (1) The offset must be >= 0 and should not exceed 0x40000000.
 *      (2) The offset is subtracted from the src 32 bpp image
 */
PIX *
pixFinalAccumulateThreshold(PIX      *pixs,
                            l_uint32  offset,
                            l_uint32  threshold)
{
l_int32    w, h, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixFinalAccumulateThreshold");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs not 32 bpp", procName, NULL);
    if (offset > 0x40000000)
        offset = 0x40000000;

    pixGetDimensions(pixs, &w, &h, NULL);
    if ((pixd = pixCreate(w, h, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);  /* but how did pixs get it initially? */
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);

    finalAccumulateThreshLow(datad, w, h, wpld, datas, wpls, offset, threshold);
    return pixd;
}


/*!
 *  pixAccumulate() 
 *
 *      Input:  pixd (32 bpp)
 *              pixs (1, 8, 16 or 32 bpp)
 *              op  (L_ARITH_ADD or L_ARITH_SUBTRACT)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This adds or subtracts each pixs value from pixd.
 *      (2) This clips to the minimum of pixs and pixd, so they
 *          do not need to be the same size.
 *      (3) The alignment is to the origin (UL corner) of pixs & pixd.
 */
l_int32
pixAccumulate(PIX     *pixd,
              PIX     *pixs,
              l_int32  op)
{
l_int32    w, h, d, wd, hd, wpls, wpld;
l_uint32  *datas, *datad;

    PROCNAME("pixAccumulate");

    if (!pixd || (pixGetDepth(pixd) != 32))
        return ERROR_INT("pixd not defined or not 32 bpp", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    d = pixGetDepth(pixs);
    if (d != 1 && d != 8 && d != 16 && d != 32)
        return ERROR_INT("pixs not 1, 8, 16 or 32 bpp", procName, 1);
    if (op != L_ARITH_ADD && op != L_ARITH_SUBTRACT)
        return ERROR_INT("op must be in {L_ARITH_ADD, L_ARITH_SUBTRACT}",
                         procName, 1);

    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    pixGetDimensions(pixs, &w, &h, NULL);
    pixGetDimensions(pixd, &wd, &hd, NULL);
    w = L_MIN(w, wd);
    h = L_MIN(h, hd);

    accumulateLow(datad, w, h, wpld, datas, d, wpls, op);
    return 0;
}


/*!
 *  pixMultConstAccumulate() 
 *
 *      Input:  pixs (32 bpp)
 *              factor
 *              offset (same as used for initialization)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The offset must be >= 0 and should not exceed 0x40000000.
 *      (2) This multiplies each pixel, relative to offset, by the input factor
 *      (3) The result is returned with the offset back in place.
 */
l_int32
pixMultConstAccumulate(PIX       *pixs,
                       l_float32  factor,
                       l_uint32   offset)
{
l_int32    w, h, wpl;
l_uint32  *data;

    PROCNAME("pixMultConstAccumulate");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetDepth(pixs) != 32)
        return ERROR_INT("pixs not 32 bpp", procName, 1);
    if (offset > 0x40000000)
        offset = 0x40000000;

    pixGetDimensions(pixs, &w, &h, NULL);
    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);

    multConstAccumulateLow(data, w, h, wpl, factor, offset);
    return 0;
}


/*-----------------------------------------------------------------------*
 *                      Absolute value of difference                     *
 *-----------------------------------------------------------------------*/
/*!
 *  pixAbsDifference()
 *
 *      Input:  pixs1, pixs2  (both either 8 or 16 bpp gray, or 32 bpp RGB)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) The depth of pixs1 and pixs2 must be equal.
 *      (2) Clips computation to the min size, aligning the UL corners
 *      (3) For 8 and 16 bpp, assumes one gray component.
 *      (4) For 32 bpp, assumes 3 color components, and ignores the
 *          LSB of each word (the alpha channel)
 *      (5) Computes the absolute value of the difference between
 *          each component value.
 */
PIX *
pixAbsDifference(PIX  *pixs1,
                 PIX  *pixs2)
{
l_int32    w, h, w2, h2, d, wpls, wpld;
l_uint32  *datas1, *datas2, *datad;
PIX       *pixd;

    PROCNAME("pixAbsDifference");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, NULL);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, NULL);
    d = pixGetDepth(pixs1);
    if (d != pixGetDepth(pixs2))
        return (PIX *)ERROR_PTR("src1 and src2 depths unequal", procName, NULL);
    if (d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("depths not in {8, 16, 32}", procName, NULL);

    pixGetDimensions(pixs1, &w, &h, NULL);
    pixGetDimensions(pixs2, &w2, &h2, NULL);
    w = L_MIN(w, w2);
    h = L_MIN(h, h2);
    if ((pixd = pixCreate(w, h, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs1);
    datas1 = pixGetData(pixs1);
    datas2 = pixGetData(pixs2);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs1);
    wpld = pixGetWpl(pixd);

    absDifferenceLow(datad, w, h, wpld, datas1, datas2, d, wpls);

    return pixd;
}


/*-----------------------------------------------------------------------*
 *             Two-image min and max operations (8 and 16 bpp)           *
 *-----------------------------------------------------------------------*/
/*!
 *  pixMinOrMax()
 *
 *      Input:  pixd  (<optional> destination: this can be null,
 *                     equal to pixs1, or different from pixs1)
 *              pixs1 (can be == to pixd)
 *              pixs2
 *              type (L_CHOOSE_MIN, L_CHOOSE_MAX)
 *      Return: pixd always
 *
 *  Notes:
 *      (1) This gives the min or max of two images.
 *      (2) The depth can be 8 or 16 bpp.
 *      (3) There are 3 cases:
 *          -  if pixd == null,   Min(src1, src2) --> new pixd
 *          -  if pixd == pixs1,  Min(src1, src2) --> src1  (in-place)
 *          -  if pixd != pixs1,  Min(src1, src2) --> input pixd
 */
PIX *
pixMinOrMax(PIX     *pixd,
            PIX     *pixs1,
            PIX     *pixs2,
            l_int32  type)
{
l_int32    d, ws, hs, w, h, wpls, wpld, i, j;
l_int32    vals, vald, val;
l_uint32  *datas, *datad, *lines, *lined;

    PROCNAME("pixMinOrMax");

    if (!pixs1)
        return (PIX *)ERROR_PTR("pixs1 not defined", procName, pixd);
    if (!pixs2)
        return (PIX *)ERROR_PTR("pixs2 not defined", procName, pixd);
    if (pixs1 == pixs2)
        return (PIX *)ERROR_PTR("pixs1 and pixs2 must differ", procName, pixd);
    if (type != L_CHOOSE_MIN && type != L_CHOOSE_MAX)
        return (PIX *)ERROR_PTR("invalid type", procName, pixd);
    d = pixGetDepth(pixs1);
    if (pixGetDepth(pixs2) != d)
        return (PIX *)ERROR_PTR("depths unequal", procName, pixd);
    if (d != 8 && d != 16)
        return (PIX *)ERROR_PTR("depth not 8 or 16 bpp", procName, pixd);

    if (pixs1 != pixd)
        pixd = pixCopy(pixd, pixs1);

    pixGetDimensions(pixs2, &ws, &hs, NULL);
    pixGetDimensions(pixd, &w, &h, NULL);
    w = L_MIN(w, ws);
    h = L_MIN(h, hs);
    datas = pixGetData(pixs2);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs2);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        if (d == 8) {
            if (type == L_CHOOSE_MIN) {
                for (j = 0; j < w; j++) {
                    vals = GET_DATA_BYTE(lines, j);
                    vald = GET_DATA_BYTE(lined, j);
                    val = L_MIN(vals, vald);
                    SET_DATA_BYTE(lined, j, val);
                }
            } else {  /* type == L_CHOOSE_MAX */
                for (j = 0; j < w; j++) {
                    vals = GET_DATA_BYTE(lines, j);
                    vald = GET_DATA_BYTE(lined, j);
                    val = L_MAX(vals, vald);
                    SET_DATA_BYTE(lined, j, val);
                }
            }
        } else {  /* d == 16 */
            if (type == L_CHOOSE_MIN) {
                for (j = 0; j < w; j++) {
                    vals = GET_DATA_TWO_BYTES(lines, j);
                    vald = GET_DATA_TWO_BYTES(lined, j);
                    val = L_MIN(vals, vald);
                    SET_DATA_TWO_BYTES(lined, j, val);
                }
            } else {  /* type == L_CHOOSE_MAX */
                for (j = 0; j < w; j++) {
                    vals = GET_DATA_TWO_BYTES(lines, j);
                    vald = GET_DATA_TWO_BYTES(lined, j);
                    val = L_MAX(vals, vald);
                    SET_DATA_TWO_BYTES(lined, j, val);
                }
            }
        }
    }

    return pixd;
}


/*-----------------------------------------------------------------------*
 *            Scale for maximum dynamic range in 8 bpp image             *
 *-----------------------------------------------------------------------*/
/*!
 *  pixMaxDynamicRange()
 *
 *      Input:  pixs  (4, 8, 16 or 32 bpp source)
 *              type  (L_LINEAR_SCALE or L_LOG_SCALE)
 *      Return: pixd (8 bpp), or null on error
 *
 *  Notes:
 *      (1) Scales pixel values to fit maximally within the dest 8 bpp pixd
 *      (2) Uses a LUT for log scaling
 */
PIX *
pixMaxDynamicRange(PIX     *pixs,
                   l_int32  type)
{
l_uint8     dval;
l_int32     i, j, w, h, d, wpls, wpld, max, sval;
l_uint32   *datas, *datad;
l_uint32    word;
l_uint32   *lines, *lined;
l_float32   factor;
l_float32  *tab;
PIX        *pixd;

    PROCNAME("pixMaxDynamicRange");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    if (d != 4 && d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("pixs not in {4,8,16,32} bpp", procName, NULL);
    if (type != L_LINEAR_SCALE && type != L_LOG_SCALE)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);

        /* Get max */
    max = 0;
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        for (j = 0; j < wpls; j++) {
            word = *(lines + j);
            if (d == 4) {
                max = L_MAX(max, word >> 28);
                max = L_MAX(max, (word >> 24) & 0xf);
                max = L_MAX(max, (word >> 20) & 0xf);
                max = L_MAX(max, (word >> 16) & 0xf);
                max = L_MAX(max, (word >> 12) & 0xf);
                max = L_MAX(max, (word >> 8) & 0xf);
                max = L_MAX(max, (word >> 4) & 0xf);
                max = L_MAX(max, word & 0xf);
            } else if (d == 8) {
                max = L_MAX(max, word >> 24);
                max = L_MAX(max, (word >> 16) & 0xff);
                max = L_MAX(max, (word >> 8) & 0xff);
                max = L_MAX(max, word & 0xff);
            } else if (d == 16) {
                max = L_MAX(max, word >> 16);
                max = L_MAX(max, word & 0xffff);
            } else {  /* d == 32 */
                max = L_MAX(max, word);
            }
        }
    }

        /* Map to the full dynamic range of 8 bpp output */
    if (d == 4) {
        if (type == L_LINEAR_SCALE) {
            factor = 255. / (l_float32)max;
            for (i = 0; i < h; i++) {
                lines = datas + i * wpls;
                lined = datad + i * wpld;
                for (j = 0; j < w; j++) {
                    sval = GET_DATA_QBIT(lines, j);
                    dval = (l_uint8)(factor * (l_float32)sval + 0.5);
                    SET_DATA_QBIT(lined, j, dval);
                }
            }
        } else {  /* type == L_LOG_SCALE) */
            tab = makeLogBase2Tab();
            factor = 255. / getLogBase2(max, tab);
            for (i = 0; i < h; i++) {
                lines = datas + i * wpls;
                lined = datad + i * wpld;
                for (j = 0; j < w; j++) {
                    sval = GET_DATA_QBIT(lines, j);
                    dval = (l_uint8)(factor * getLogBase2(sval, tab) + 0.5);
                    SET_DATA_BYTE(lined, j, dval);
                }
            }
            FREE(tab);
        }
    } else if (d == 8) {
        if (type == L_LINEAR_SCALE) {
            factor = 255. / (l_float32)max;
            for (i = 0; i < h; i++) {
                lines = datas + i * wpls;
                lined = datad + i * wpld;
                for (j = 0; j < w; j++) {
                    sval = GET_DATA_BYTE(lines, j);
                    dval = (l_uint8)(factor * (l_float32)sval + 0.5);
                    SET_DATA_BYTE(lined, j, dval);
                }
            }
        } else {  /* type == L_LOG_SCALE) */
            tab = makeLogBase2Tab();
            factor = 255. / getLogBase2(max, tab);
            for (i = 0; i < h; i++) {
                lines = datas + i * wpls;
                lined = datad + i * wpld;
                for (j = 0; j < w; j++) {
                    sval = GET_DATA_BYTE(lines, j);
                    dval = (l_uint8)(factor * getLogBase2(sval, tab) + 0.5);
                    SET_DATA_BYTE(lined, j, dval);
                }
            }
            FREE(tab);
        }
    } else if (d == 16) {
        if (type == L_LINEAR_SCALE) {
            factor = 255. / (l_float32)max;
            for (i = 0; i < h; i++) {
                lines = datas + i * wpls;
                lined = datad + i * wpld;
                for (j = 0; j < w; j++) {
                    sval = GET_DATA_TWO_BYTES(lines, j);
                    dval = (l_uint8)(factor * (l_float32)sval + 0.5);
                    SET_DATA_BYTE(lined, j, dval);
                }
            }
        } else {  /* type == L_LOG_SCALE) */
            tab = makeLogBase2Tab();
            factor = 255. / getLogBase2(max, tab);
            for (i = 0; i < h; i++) {
                lines = datas + i * wpls;
                lined = datad + i * wpld;
                for (j = 0; j < w; j++) {
                    sval = GET_DATA_TWO_BYTES(lines, j);
                    dval = (l_uint8)(factor * getLogBase2(sval, tab) + 0.5);
                    SET_DATA_BYTE(lined, j, dval);
                }
            }
            FREE(tab);
        }
    } else {  /* d == 32 */
        if (type == L_LINEAR_SCALE) {
            factor = 255. / (l_float32)max;
            for (i = 0; i < h; i++) {
                lines = datas + i * wpls;
                lined = datad + i * wpld;
                for (j = 0; j < w; j++) {
                    sval = lines[j];
                    dval = (l_uint8)(factor * (l_float32)sval + 0.5);
                    SET_DATA_BYTE(lined, j, dval);
                }
            }
        } else {  /* type == L_LOG_SCALE) */
            tab = makeLogBase2Tab();
            factor = 255. / getLogBase2(max, tab);
            for (i = 0; i < h; i++) {
                lines = datas + i * wpls;
                lined = datad + i * wpld;
                for (j = 0; j < w; j++) {
                    sval = lines[j];
                    dval = (l_uint8)(factor * getLogBase2(sval, tab) + 0.5);
                    SET_DATA_BYTE(lined, j, dval);
                }
            }
            FREE(tab);
        }
    }

    return pixd;
}


/*-----------------------------------------------------------------------*
 *                            Log base2 lookup                           *
 *-----------------------------------------------------------------------*/
/*
 *  makeLogBase2Tab()
 *
 *      Input: void
 *      Return: table (giving the log[base 2] of val)
 */
l_float32 *
makeLogBase2Tab(void)
{
l_int32     i;
l_float32   log2;
l_float32  *tab;

    PROCNAME("makeLogBase2Tab");

    if ((tab = (l_float32 *)CALLOC(256, sizeof(l_float32))) == NULL)
        return (l_float32 *)ERROR_PTR("tab not made", procName, NULL);

    log2 = (l_float32)log((l_float32)2);
    for (i = 0; i < 256; i++)
        tab[i] = (l_float32)log((l_float32)i) / log2;

    return tab;
}


/*
 * getLogBase2()
 *
 *     Input:  val
 *             logtab (256-entry table of logs)
 *     Return: logdist, or 0 on error
 */
l_float32
getLogBase2(l_int32     val,
            l_float32  *logtab)
{
    PROCNAME("getLogBase2");

    if (!logtab)
        return ERROR_INT("logtab not defined", procName, 0);

    if (val < 0x100)
        return logtab[val];
    else if (val < 0x10000)
        return 8.0 + logtab[val >> 8];
    else if (val < 0x1000000)
        return 16.0 + logtab[val >> 16];
    else
        return 24.0 + logtab[val >> 24];
}

