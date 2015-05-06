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
 *  pix2.c
 *
 *    This file has these basic operations:
 *
 *      (1) Get and set: individual pixels, full image, rectangular region,
 *          pad pixels, border pixels, and color components for RGB
 *      (2) Add and remove border pixels
 *      (3) Endian byte swaps
 *      (4) Simple method for byte-processing images (instead of words)
 *
 *      Pixel poking
 *           l_int32     pixGetPixel()
 *           l_int32     pixSetPixel()
 *           l_int32     pixGetRGBPixel()
 *           l_int32     pixSetRGBPixel()
 *           l_int32     pixGetRandomPixel()
 *           l_int32     pixClearPixel()
 *           l_int32     pixFlipPixel()
 *           void        setPixelLow()
 *
 *      Find black or white value
 *           l_int32     pixGetBlackOrWhiteVal()
 *
 *      Full image clear/set/set-to-arbitrary-value
 *           l_int32     pixClearAll()
 *           l_int32     pixSetAll()
 *           l_int32     pixSetAllGray()
 *           l_int32     pixSetAllArbitrary()
 *           l_int32     pixSetBlackOrWhite()
 *           l_int32     pixSetComponentArbitrary()
 *
 *      Rectangular region clear/set/set-to-arbitrary-value/blend
 *           l_int32     pixClearInRect()
 *           l_int32     pixSetInRect()
 *           l_int32     pixSetInRectArbitrary()
 *           l_int32     pixBlendInRect()
 *
 *      Set pad bits
 *           l_int32     pixSetPadBits()
 *           l_int32     pixSetPadBitsBand()
 *
 *      Assign border pixels
 *           l_int32     pixSetOrClearBorder()
 *           l_int32     pixSetBorderVal()
 *           l_int32     pixSetBorderRingVal()
 *           l_int32     pixSetMirroredBorder()
 *           PIX        *pixCopyBorder()
 *
 *      Add and remove border
 *           PIX        *pixAddBorder()
 *           PIX        *pixAddBlackOrWhiteBorder()
 *           PIX        *pixAddBorderGeneral()
 *           PIX        *pixRemoveBorder()
 *           PIX        *pixRemoveBorderGeneral()
 *           PIX        *pixRemoveBorderToSize()
 *           PIX        *pixAddMirroredBorder()
 *           PIX        *pixAddRepeatedBorder()
 *           PIX        *pixAddMixedBorder()
 *           PIX        *pixAddContinuedBorder()
 *
 *      Helper functions using alpha
 *           l_int32     pixShiftAndTransferAlpha()
 *           PIX        *pixDisplayLayersRGBA()
 *
 *      Color sample setting and extraction
 *           PIX        *pixCreateRGBImage()
 *           PIX        *pixGetRGBComponent()
 *           l_int32     pixSetRGBComponent()
 *           PIX        *pixGetRGBComponentCmap()
 *           l_int32     pixCopyRGBComponent()
 *           l_int32     composeRGBPixel()
 *           l_int32     composeRGBAPixel()
 *           void        extractRGBValues()
 *           void        extractRGBAValues()
 *           l_int32     extractMinMaxComponent()
 *           l_int32     pixGetRGBLine()
 *
 *      Conversion between big and little endians
 *           PIX        *pixEndianByteSwapNew()
 *           l_int32     pixEndianByteSwap()
 *           l_int32     lineEndianByteSwap()
 *           PIX        *pixEndianTwoByteSwapNew()
 *           l_int32     pixEndianTwoByteSwap()
 *
 *      Extract raster data as binary string
 *           l_int32     pixGetRasterData()
 *
 *      Test alpha component opaqueness
 *           l_int32     pixAlphaIsOpaque
 *
 *      Setup helpers for 8 bpp byte processing
 *           l_uint8   **pixSetupByteProcessing()
 *           l_int32     pixCleanupByteProcessing()
 *
 *      Setting parameters for antialias masking with alpha transforms
 *           void        l_setAlphaMaskBorder()
 *
 *      *** indicates implicit assumption about RGB component ordering
 */


#include <string.h>
#include "allheaders.h"

static const l_uint32 rmask32[] = {0x0,
    0x00000001, 0x00000003, 0x00000007, 0x0000000f,
    0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
    0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
    0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
    0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
    0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
    0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff};

    /* This is a global that determines the default 8 bpp alpha mask values
     * for rings at distance 1 and 2 from the border.  Declare extern
     * to use.  To change the values, use l_setAlphaMaskBorder(). */
LEPT_DLL l_float32  AlphaMaskBorderVals[2] = {0.0, 0.5};


#ifndef  NO_CONSOLE_IO
#define  DEBUG_SERIALIZE        0
#endif  /* ~NO_CONSOLE_IO */


/*-------------------------------------------------------------*
 *                         Pixel poking                        *
 *-------------------------------------------------------------*/
/*!
 *  pixGetPixel()
 *
 *      Input:  pix
 *              (x,y) pixel coords
 *              &val (<return> pixel value)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This returns the value in the data array.  If the pix is
 *          colormapped, it returns the colormap index, not the rgb value.
 *      (2) Because of the function overhead and the parameter checking,
 *          this is much slower than using the GET_DATA_*() macros directly.
 *          Speed on a 1 Mpixel RGB image, using a 3 GHz machine:
 *            * pixGet/pixSet: ~25 Mpix/sec
 *            * GET_DATA/SET_DATA: ~350 MPix/sec
 *          If speed is important and you're doing random access into
 *          the pix, use pixGetLinePtrs() and the array access macros.
 */
l_int32
pixGetPixel(PIX       *pix,
            l_int32    x,
            l_int32    y,
            l_uint32  *pval)
{
l_int32    w, h, d, wpl, val;
l_uint32  *line, *data;

    PROCNAME("pixGetPixel");

    if (!pval)
        return ERROR_INT("pval not defined", procName, 1);
    *pval = 0;
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixGetDimensions(pix, &w, &h, &d);
    if (x < 0 || x >= w)
        return ERROR_INT("x out of bounds", procName, 1);
    if (y < 0 || y >= h)
        return ERROR_INT("y out of bounds", procName, 1);

    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    line = data + y * wpl;
    switch (d)
    {
    case 1:
        val = GET_DATA_BIT(line, x);
        break;
    case 2:
        val = GET_DATA_DIBIT(line, x);
        break;
    case 4:
        val = GET_DATA_QBIT(line, x);
        break;
    case 8:
        val = GET_DATA_BYTE(line, x);
        break;
    case 16:
        val = GET_DATA_TWO_BYTES(line, x);
        break;
    case 32:
        val = line[x];
        break;
    default:
        return ERROR_INT("depth must be in {1,2,4,8,16,32} bpp", procName, 1);
    }

    *pval = val;
    return 0;
}


/*!
 *  pixSetPixel()
 *
 *      Input:  pix
 *              (x,y) pixel coords
 *              val (value to be inserted)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Warning: the input value is not checked for overflow with respect
 *          the the depth of @pix, and the sign bit (if any) is ignored.
 *          * For d == 1, @val > 0 sets the bit on.
 *          * For d == 2, 4, 8 and 16, @val is masked to the maximum allowable
 *            pixel value, and any (invalid) higher order bits are discarded.
 *      (2) See pixGetPixel() for information on performance.
 */
l_int32
pixSetPixel(PIX      *pix,
            l_int32   x,
            l_int32   y,
            l_uint32  val)
{
l_int32    w, h, d, wpl;
l_uint32  *line, *data;

    PROCNAME("pixSetPixel");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixGetDimensions(pix, &w, &h, &d);
    if (x < 0 || x >= w)
        return ERROR_INT("x out of bounds", procName, 1);
    if (y < 0 || y >= h)
        return ERROR_INT("y out of bounds", procName, 1);

    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    line = data + y * wpl;
    switch (d)
    {
    case 1:
        if (val)
            SET_DATA_BIT(line, x);
        else
            CLEAR_DATA_BIT(line, x);
        break;
    case 2:
        SET_DATA_DIBIT(line, x, val);
        break;
    case 4:
        SET_DATA_QBIT(line, x, val);
        break;
    case 8:
        SET_DATA_BYTE(line, x, val);
        break;
    case 16:
        SET_DATA_TWO_BYTES(line, x, val);
        break;
    case 32:
        line[x] = val;
        break;
    default:
        return ERROR_INT("depth must be in {1,2,4,8,16,32} bpp", procName, 1);
    }

    return 0;
}


/*!
 *  pixGetRGBPixel()
 *
 *      Input:  pix (32 bpp rgb, not colormapped)
 *              (x,y) pixel coords
 *              &rval (<optional return> red component)
 *              &gval (<optional return> green component)
 *              &bval (<optional return> blue component)
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixGetRGBPixel(PIX      *pix,
               l_int32   x,
               l_int32   y,
               l_int32  *prval,
               l_int32  *pgval,
               l_int32  *pbval)
{
l_int32    w, h, d, wpl;
l_uint32  *data, *ppixel;

    PROCNAME("pixGetRGBPixel");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 32)
        return ERROR_INT("pix not 32 bpp", procName, 1);
    if (x < 0 || x >= w)
        return ERROR_INT("x out of bounds", procName, 1);
    if (y < 0 || y >= h)
        return ERROR_INT("y out of bounds", procName, 1);

    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    ppixel = data + y * wpl + x;
    if (prval) *prval = GET_DATA_BYTE(ppixel, COLOR_RED);
    if (pgval) *pgval = GET_DATA_BYTE(ppixel, COLOR_GREEN);
    if (pbval) *pbval = GET_DATA_BYTE(ppixel, COLOR_BLUE);
    return 0;
}


/*!
 *  pixSetRGBPixel()
 *
 *      Input:  pix (32 bpp rgb)
 *              (x,y) pixel coords
 *              rval (red component)
 *              gval (green component)
 *              bval (blue component)
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixSetRGBPixel(PIX     *pix,
               l_int32  x,
               l_int32  y,
               l_int32  rval,
               l_int32  gval,
               l_int32  bval)
{
l_int32    w, h, d, wpl;
l_uint32   pixel;
l_uint32  *data, *line;

    PROCNAME("pixSetRGBPixel");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 32)
        return ERROR_INT("pix not 32 bpp", procName, 1);
    if (x < 0 || x >= w)
        return ERROR_INT("x out of bounds", procName, 1);
    if (y < 0 || y >= h)
        return ERROR_INT("y out of bounds", procName, 1);

    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    line = data + y * wpl;
    composeRGBPixel(rval, gval, bval, &pixel);
    *(line + x) = pixel;
    return 0;
}


/*!
 *  pixGetRandomPixel()
 *
 *      Input:  pix (any depth; can be colormapped)
 *              &val (<return> pixel value)
 *              &x (<optional return> x coordinate chosen; can be null)
 *              &y (<optional return> y coordinate chosen; can be null)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) If the pix is colormapped, it returns the rgb value.
 */
l_int32
pixGetRandomPixel(PIX       *pix,
                  l_uint32  *pval,
                  l_int32   *px,
                  l_int32   *py)
{
l_int32   w, h, x, y, rval, gval, bval;
l_uint32  val;
PIXCMAP  *cmap;

    PROCNAME("pixGetRandomPixel");

    if (!pval)
        return ERROR_INT("pval not defined", procName, 1);
    *pval = 0;
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixGetDimensions(pix, &w, &h, NULL);
    x = rand() % w;
    y = rand() % h;
    if (px) *px = x;
    if (py) *py = y;
    pixGetPixel(pix, x, y, &val);
    if ((cmap = pixGetColormap(pix)) != NULL) {
        pixcmapGetColor(cmap, val, &rval, &gval, &bval);
        composeRGBPixel(rval, gval, bval, pval);
    } else {
        *pval = val;
    }

    return 0;
}


/*!
 *  pixClearPixel()
 *
 *      Input:  pix
 *              (x,y) pixel coords
 *      Return: 0 if OK; 1 on error.
 */
l_int32
pixClearPixel(PIX     *pix,
              l_int32  x,
              l_int32  y)
{
l_int32    w, h, d, wpl;
l_uint32  *line, *data;

    PROCNAME("pixClearPixel");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixGetDimensions(pix, &w, &h, &d);
    if (x < 0 || x >= w)
        return ERROR_INT("x out of bounds", procName, 1);
    if (y < 0 || y >= h)
        return ERROR_INT("y out of bounds", procName, 1);

    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    line = data + y * wpl;
    switch (d)
    {
    case 1:
        CLEAR_DATA_BIT(line, x);
        break;
    case 2:
        CLEAR_DATA_DIBIT(line, x);
        break;
    case 4:
        CLEAR_DATA_QBIT(line, x);
        break;
    case 8:
        SET_DATA_BYTE(line, x, 0);
        break;
    case 16:
        SET_DATA_TWO_BYTES(line, x, 0);
        break;
    case 32:
        line[x] = 0;
        break;
    default:
        return ERROR_INT("depth must be in {1,2,4,8,16,32} bpp", procName, 1);
    }

    return 0;
}


/*!
 *  pixFlipPixel()
 *
 *      Input:  pix
 *              (x,y) pixel coords
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixFlipPixel(PIX     *pix,
             l_int32  x,
             l_int32  y)
{
l_int32    w, h, d, wpl;
l_uint32   val;
l_uint32  *line, *data;

    PROCNAME("pixFlipPixel");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixGetDimensions(pix, &w, &h, &d);
    if (x < 0 || x >= w)
        return ERROR_INT("x out of bounds", procName, 1);
    if (y < 0 || y >= h)
        return ERROR_INT("y out of bounds", procName, 1);

    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    line = data + y * wpl;
    switch (d)
    {
    case 1:
        val = GET_DATA_BIT(line, x);
        if (val)
            CLEAR_DATA_BIT(line, x);
        else
            SET_DATA_BIT(line, x);
        break;
    case 2:
        val = GET_DATA_DIBIT(line, x);
        val ^= 0x3;
        SET_DATA_DIBIT(line, x, val);
        break;
    case 4:
        val = GET_DATA_QBIT(line, x);
        val ^= 0xf;
        SET_DATA_QBIT(line, x, val);
        break;
    case 8:
        val = GET_DATA_BYTE(line, x);
        val ^= 0xff;
        SET_DATA_BYTE(line, x, val);
        break;
    case 16:
        val = GET_DATA_TWO_BYTES(line, x);
        val ^= 0xffff;
        SET_DATA_TWO_BYTES(line, x, val);
        break;
    case 32:
        val = line[x] ^ 0xffffffff;
        line[x] = val;
        break;
    default:
        return ERROR_INT("depth must be in {1,2,4,8,16,32} bpp", procName, 1);
    }

    return 0;
}


/*!
 *  setPixelLow()
 *
 *      Input:  line (ptr to beginning of line),
 *              x (pixel location in line)
 *              depth (bpp)
 *              val (to be inserted)
 *      Return: void
 *
 *  Notes:
 *      (1) Caution: input variables are not checked!
 */
void
setPixelLow(l_uint32  *line,
            l_int32    x,
            l_int32    depth,
            l_uint32   val)
{
    switch (depth)
    {
    case 1:
        if (val)
            SET_DATA_BIT(line, x);
        else
            CLEAR_DATA_BIT(line, x);
        break;
    case 2:
        SET_DATA_DIBIT(line, x, val);
        break;
    case 4:
        SET_DATA_QBIT(line, x, val);
        break;
    case 8:
        SET_DATA_BYTE(line, x, val);
        break;
    case 16:
        SET_DATA_TWO_BYTES(line, x, val);
        break;
    case 32:
        line[x] = val;
        break;
    default:
        fprintf(stderr, "illegal depth in setPixelLow()\n");
    }

    return;
}


/*-------------------------------------------------------------*
 *                     Find black or white value               *
 *-------------------------------------------------------------*/
/*!
 *  pixGetBlackOrWhiteVal()
 *
 *      Input:  pixs (all depths; cmap ok)
 *              op (L_GET_BLACK_VAL, L_GET_WHITE_VAL)
 *              &val (<return> pixel value)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Side effect.  For a colormapped image, if the requested
 *          color is not present and there is room to add it in the cmap,
 *          it is added and the new index is returned.  If there is no room,
 *          the index of the closest color in intensity is returned.
 */
l_int32
pixGetBlackOrWhiteVal(PIX       *pixs,
                      l_int32    op,
                      l_uint32  *pval)
{
l_int32   d, val;
PIXCMAP  *cmap;

    PROCNAME("pixGetBlackOrWhiteVal");

    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = 0;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (op != L_GET_BLACK_VAL && op != L_GET_WHITE_VAL)
        return ERROR_INT("invalid op", procName, 1);

    cmap = pixGetColormap(pixs);
    d = pixGetDepth(pixs);
    if (!cmap) {
        if ((d == 1 && op == L_GET_WHITE_VAL) ||
            (d > 1 && op == L_GET_BLACK_VAL)) {  /* min val */
            val = 0;
        } else {  /* max val */
            val = (d == 32) ? 0xffffff00 : (1 << d) - 1;
        }
    } else {  /* handle colormap */
        if (op == L_GET_BLACK_VAL)
            pixcmapAddBlackOrWhite(cmap, 0, &val);
        else  /* L_GET_WHITE_VAL */
            pixcmapAddBlackOrWhite(cmap, 1, &val);
    }
    *pval = val;

    return 0;
}


/*-------------------------------------------------------------*
 *     Full image clear/set/set-to-arbitrary-value/invert      *
 *-------------------------------------------------------------*/
/*!
 *  pixClearAll()
 *
 *      Input:  pix (all depths; use cmapped with caution)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Clears all data to 0.  For 1 bpp, this is white; for grayscale
 *          or color, this is black.
 *      (2) Caution: for colormapped pix, this sets the color to the first
 *          one in the colormap.  Be sure that this is the intended color!
 */
l_int32
pixClearAll(PIX  *pix)
{
    PROCNAME("pixClearAll");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixRasterop(pix, 0, 0, pixGetWidth(pix), pixGetHeight(pix),
                PIX_CLR, NULL, 0, 0);
    return 0;
}


/*!
 *  pixSetAll()
 *
 *      Input:  pix (all depths; use cmapped with caution)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Sets all data to 1.  For 1 bpp, this is black; for grayscale
 *          or color, this is white.
 *      (2) Caution: for colormapped pix, this sets the pixel value to the
 *          maximum value supported by the colormap: 2^d - 1.  However, this
 *          color may not be defined, because the colormap may not be full.
 */
l_int32
pixSetAll(PIX  *pix)
{
l_int32   n;
PIXCMAP  *cmap;

    PROCNAME("pixSetAll");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if ((cmap = pixGetColormap(pix)) != NULL) {
        n = pixcmapGetCount(cmap);
        if (n < cmap->nalloc)  /* cmap is not full */
            return ERROR_INT("cmap entry does not exist", procName, 1);
    }

    pixRasterop(pix, 0, 0, pixGetWidth(pix), pixGetHeight(pix),
                PIX_SET, NULL, 0, 0);
    return 0;
}


/*!
 *  pixSetAllGray()
 *
 *      Input:  pix (all depths, cmap ok)
 *              grayval (in range 0 ... 255)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) N.B.  For all images, @grayval == 0 represents black and
 *          @grayval == 255 represents white.
 *      (2) For depth < 8, we do our best to approximate the gray level.
 *          For 1 bpp images, any @grayval < 128 is black; >= 128 is white.
 *          For 32 bpp images, each r,g,b component is set to @grayval,
 *          and the alpha component is preserved.
 *      (3) If pix is colormapped, it adds the gray value, replicated in
 *          all components, to the colormap if it's not there and there
 *          is room.  If the colormap is full, it finds the closest color in
 *          L2 distance of components.  This index is written to all pixels.
 */
l_int32
pixSetAllGray(PIX     *pix,
              l_int32  grayval)
{
l_int32   d, spp, index;
l_uint32  val32;
PIX      *alpha;
PIXCMAP  *cmap;

    PROCNAME("pixSetAllGray");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (grayval < 0) {
        L_WARNING("grayval < 0; setting to 0\n", procName);
        grayval = 0;
    } else if (grayval > 255) {
        L_WARNING("grayval > 255; setting to 255\n", procName);
        grayval = 255;
    }

        /* Handle the colormap case */
    cmap = pixGetColormap(pix);
    if (cmap) {
        pixcmapAddNearestColor(cmap, grayval, grayval, grayval, &index);
        pixSetAllArbitrary(pix, index);
        return 0;
    }

        /* Non-cmapped */
    d = pixGetDepth(pix);
    spp = pixGetSpp(pix);
    if (d == 1) {
        if (grayval < 128)  /* black */
            pixSetAll(pix);
        else
            pixClearAll(pix);  /* white */
    } else if (d < 8) {
        grayval >>= 8 - d;
        pixSetAllArbitrary(pix, grayval);
    } else if (d == 8) {
        pixSetAllArbitrary(pix, grayval);
    } else if (d == 16) {
        grayval |= (grayval << 8);
        pixSetAllArbitrary(pix, grayval);
    } else if (d == 32 && spp == 3) {
        composeRGBPixel(grayval, grayval, grayval, &val32);
        pixSetAllArbitrary(pix, val32);
    } else if (d == 32 && spp == 4) {
        alpha = pixGetRGBComponent(pix, L_ALPHA_CHANNEL);
        composeRGBPixel(grayval, grayval, grayval, &val32);
        pixSetAllArbitrary(pix, val32);
        pixSetRGBComponent(pix, alpha, L_ALPHA_CHANNEL);
        pixDestroy(&alpha);
    } else {
        L_ERROR("invalid depth: %d\n", procName, d);
        return 1;
    }

    return 0;
}


/*!
 *  pixSetAllArbitrary()
 *
 *      Input:  pix (all depths; use cmapped with caution)
 *              val  (value to set all pixels)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Caution!  For colormapped pix, @val is used as an index
 *          into a colormap.  Be sure that index refers to the intended color.
 *          If the color is not in the colormap, you should first add it
 *          and then call this function.
 */
l_int32
pixSetAllArbitrary(PIX      *pix,
                   l_uint32  val)
{
l_int32    n, i, j, w, h, d, wpl, npix;
l_uint32   maxval, wordval;
l_uint32  *data, *line;
PIXCMAP   *cmap;

    PROCNAME("pixSetAllArbitrary");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

        /* If colormapped, make sure that val is less than the size
         * of the cmap array. */
    if ((cmap = pixGetColormap(pix)) != NULL) {
        n = pixcmapGetCount(cmap);
        if (val >= n) {
            L_WARNING("index not in colormap; using last color\n", procName);
            val = n - 1;
        }
    }

        /* Make sure val isn't too large for the pixel depth.
         * If it is too large, set the pixel color to white.  */
    pixGetDimensions(pix, &w, &h, &d);
    maxval = (d == 32) ? 0xffffff00 : (1 << d) - 1;
    if (val > maxval) {
        L_WARNING("val too large for depth; using maxval\n", procName);
        val = maxval;
    }

        /* Set up word to tile with */
    wordval = 0;
    npix = 32 / d;    /* number of pixels per 32 bit word */
    for (j = 0; j < npix; j++)
        wordval |= (val << (j * d));
    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < wpl; j++) {
            *(line + j) = wordval;
        }
    }
    return 0;
}


/*!
 *  pixSetBlackOrWhite()
 *
 *      Input:  pixs (all depths; cmap ok)
 *              op (L_SET_BLACK, L_SET_WHITE)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Function for setting all pixels in an image to either black
 *          or white.
 *      (2) If pixs is colormapped, it adds black or white to the
 *          colormap if it's not there and there is room.  If the colormap
 *          is full, it finds the closest color in intensity.
 *          This index is written to all pixels.
 */
l_int32
pixSetBlackOrWhite(PIX     *pixs,
                   l_int32  op)
{
l_int32   d, index;
PIXCMAP  *cmap;

    PROCNAME("pixSetBlackOrWhite");

    if (!pixs)
        return ERROR_INT("pix not defined", procName, 1);
    if (op != L_SET_BLACK && op != L_SET_WHITE)
        return ERROR_INT("invalid op", procName, 1);

    cmap = pixGetColormap(pixs);
    d = pixGetDepth(pixs);
    if (!cmap) {
        if ((d == 1 && op == L_SET_BLACK) || (d > 1 && op == L_SET_WHITE))
            pixSetAll(pixs);
        else
            pixClearAll(pixs);
    } else {  /* handle colormap */
        if (op == L_SET_BLACK)
            pixcmapAddBlackOrWhite(cmap, 0, &index);
        else  /* L_SET_WHITE */
            pixcmapAddBlackOrWhite(cmap, 1, &index);
        pixSetAllArbitrary(pixs, index);
    }

    return 0;
}


/*!
 *  pixSetComponentArbitrary()
 *
 *      Input:  pix (32 bpp)
 *              comp (COLOR_RED, COLOR_GREEN, COLOR_BLUE, L_ALPHA_CHANNEL)
 *              val  (value to set this component)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) For example, this can be used to set the alpha component to opaque:
 *              pixSetComponentArbitrary(pix, L_ALPHA_CHANNEL, 255)
 */
l_int32
pixSetComponentArbitrary(PIX     *pix,
                         l_int32  comp,
                         l_int32  val)
{
l_int32    i, nwords;
l_uint32   mask1, mask2;
l_uint32  *data;

    PROCNAME("pixSetComponentArbitrary");

    if (!pix || pixGetDepth(pix) != 32)
        return ERROR_INT("pix not defined or not 32 bpp", procName, 1);
    if (comp != COLOR_RED && comp != COLOR_GREEN && comp != COLOR_BLUE &&
        comp != L_ALPHA_CHANNEL)
        return ERROR_INT("invalid component", procName, 1);
    if (val < 0 || val > 255)
        return ERROR_INT("val not in [0 ... 255]", procName, 1);

    mask1 = ~(255 << (8 * (3 - comp)));
    mask2 = val << (8 * (3 - comp));
    nwords = pixGetHeight(pix) * pixGetWpl(pix);
    data = pixGetData(pix);
    for (i = 0; i < nwords; i++) {
        data[i] &= mask1;  /* clear out the component */
        data[i] |= mask2;  /* insert the new component value */
    }

    return 0;
}


/*-------------------------------------------------------------*
 *     Rectangular region clear/set/set-to-arbitrary-value     *
 *-------------------------------------------------------------*/
/*!
 *  pixClearInRect()
 *
 *      Input:  pix (all depths; can be cmapped)
 *              box (in which all pixels will be cleared)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Clears all data in rect to 0.  For 1 bpp, this is white;
 *          for grayscale or color, this is black.
 *      (2) Caution: for colormapped pix, this sets the color to the first
 *          one in the colormap.  Be sure that this is the intended color!
 */
l_int32
pixClearInRect(PIX  *pix,
               BOX  *box)
{
l_int32  x, y, w, h;

    PROCNAME("pixClearInRect");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);

    boxGetGeometry(box, &x, &y, &w, &h);
    pixRasterop(pix, x, y, w, h, PIX_CLR, NULL, 0, 0);
    return 0;
}


/*!
 *  pixSetInRect()
 *
 *      Input:  pix (all depths, can be cmapped)
 *              box (in which all pixels will be set)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Sets all data in rect to 1.  For 1 bpp, this is black;
 *          for grayscale or color, this is white.
 *      (2) Caution: for colormapped pix, this sets the pixel value to the
 *          maximum value supported by the colormap: 2^d - 1.  However, this
 *          color may not be defined, because the colormap may not be full.
 */
l_int32
pixSetInRect(PIX  *pix,
             BOX  *box)
{
l_int32   n, x, y, w, h;
PIXCMAP  *cmap;

    PROCNAME("pixSetInRect");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if ((cmap = pixGetColormap(pix)) != NULL) {
        n = pixcmapGetCount(cmap);
        if (n < cmap->nalloc)  /* cmap is not full */
            return ERROR_INT("cmap entry does not exist", procName, 1);
    }

    boxGetGeometry(box, &x, &y, &w, &h);
    pixRasterop(pix, x, y, w, h, PIX_SET, NULL, 0, 0);
    return 0;
}


/*!
 *  pixSetInRectArbitrary()
 *
 *      Input:  pix (all depths; can be cmapped)
 *              box (in which all pixels will be set to val)
 *              val  (value to set all pixels)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) For colormapped pix, be sure the value is the intended
 *          one in the colormap.
 *      (2) Caution: for colormapped pix, this sets each pixel in the
 *          rect to the color at the index equal to val.  Be sure that
 *          this index exists in the colormap and that it is the intended one!
 */
l_int32
pixSetInRectArbitrary(PIX      *pix,
                      BOX      *box,
                      l_uint32  val)
{
l_int32    n, x, y, xstart, xend, ystart, yend, bw, bh, w, h, d, wpl, maxval;
l_uint32  *data, *line;
BOX       *boxc;
PIXCMAP   *cmap;

    PROCNAME("pixSetInRectArbitrary");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 1 && d != 2 && d != 4 && d !=8 && d != 16 && d != 32)
        return ERROR_INT("depth must be in {1,2,4,8,16,32} bpp", procName, 1);
    if ((cmap = pixGetColormap(pix)) != NULL) {
        n = pixcmapGetCount(cmap);
        if (val >= n) {
            L_WARNING("index not in colormap; using last color\n", procName);
            val = n - 1;
        }
    }

    maxval = (d == 32) ? 0xffffff00 : (1 << d) - 1;
    if (val > maxval) val = maxval;

        /* Handle the simple cases: the min and max values */
    if (val == 0) {
        pixClearInRect(pix, box);
        return 0;
    }
    if (d == 1 ||
        (d == 2 && val == 3) ||
        (d == 4 && val == 0xf) ||
        (d == 8 && val == 0xff) ||
        (d == 16 && val == 0xffff) ||
        (d == 32 && ((val ^ 0xffffff00) >> 8 == 0))) {
        pixSetInRect(pix, box);
        return 0;
    }

        /* Find the overlap of box with the input pix */
    if ((boxc = boxClipToRectangle(box, w, h)) == NULL)
        return ERROR_INT("no overlap of box with image", procName, 1);
    boxGetGeometry(boxc, &xstart, &ystart, &bw, &bh);
    xend = xstart + bw - 1;
    yend = ystart + bh - 1;
    boxDestroy(&boxc);

    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    for (y = ystart; y <= yend; y++) {
        line = data + y * wpl;
        for (x = xstart; x <= xend; x++) {
            switch(d)
            {
            case 2:
                SET_DATA_DIBIT(line, x, val);
                break;
            case 4:
                SET_DATA_QBIT(line, x, val);
                break;
            case 8:
                SET_DATA_BYTE(line, x, val);
                break;
            case 16:
                SET_DATA_TWO_BYTES(line, x, val);
                break;
            case 32:
                line[x] = val;
                break;
            default:
                return ERROR_INT("depth not 2|4|8|16|32 bpp", procName, 1);
            }
        }
    }

    return 0;
}


/*!
 *  pixBlendInRect()
 *
 *      Input:  pixs (32 bpp rgb)
 *              box (<optional> in which all pixels will be blended)
 *              val  (blend value; 0xrrggbb00)
 *              fract (fraction of color to be blended with each pixel in pixs)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This is an in-place function.  It blends the input color @val
 *          with the pixels in pixs in the specified rectangle.
 *          If no rectangle is specified, it blends over the entire image.
 */
l_int32
pixBlendInRect(PIX       *pixs,
               BOX       *box,
               l_uint32   val,
               l_float32  fract)
{
l_int32    i, j, bx, by, bw, bh, w, h, wpls;
l_int32    prval, pgval, pbval, rval, gval, bval;
l_uint32   val32;
l_uint32  *datas, *lines;

    PROCNAME("pixBlendInRect");

    if (!pixs || pixGetDepth(pixs) != 32)
        return ERROR_INT("pixs not defined or not 32 bpp", procName, 1);

    extractRGBValues(val, &rval, &gval, &bval);
    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if (!box) {
        for (i = 0; i < h; i++) {   /* scan over box */
            lines = datas +  i * wpls;
            for (j = 0; j < w; j++) {
                val32 = *(lines + j);
                extractRGBValues(val32, &prval, &pgval, &pbval);
                prval = (l_int32)((1. - fract) * prval + fract * rval);
                pgval = (l_int32)((1. - fract) * pgval + fract * gval);
                pbval = (l_int32)((1. - fract) * pbval + fract * bval);
                composeRGBPixel(prval, pgval, pbval, &val32);
                *(lines + j) = val32;
            }
        }
        return 0;
    }

    boxGetGeometry(box, &bx, &by, &bw, &bh);
    for (i = 0; i < bh; i++) {   /* scan over box */
        if (by + i < 0 || by + i >= h) continue;
        lines = datas + (by + i) * wpls;
        for (j = 0; j < bw; j++) {
            if (bx + j < 0 || bx + j >= w) continue;
            val32 = *(lines + bx + j);
            extractRGBValues(val32, &prval, &pgval, &pbval);
            prval = (l_int32)((1. - fract) * prval + fract * rval);
            pgval = (l_int32)((1. - fract) * pgval + fract * gval);
            pbval = (l_int32)((1. - fract) * pbval + fract * bval);
            composeRGBPixel(prval, pgval, pbval, &val32);
            *(lines + bx + j) = val32;
        }
    }
    return 0;
}


/*-------------------------------------------------------------*
 *                         Set pad bits                        *
 *-------------------------------------------------------------*/
/*!
 *  pixSetPadBits()
 *
 *      Input:  pix (1, 2, 4, 8, 16, 32 bpp)
 *              val  (0 or 1)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The pad bits are the bits that expand each scanline to a
 *          multiple of 32 bits.  They are usually not used in
 *          image processing operations.  When boundary conditions
 *          are important, as in seedfill, they must be set properly.
 *      (2) This sets the value of the pad bits (if any) in the last
 *          32-bit word in each scanline.
 *      (3) For 32 bpp pix, there are no pad bits, so this is a no-op.
 */
l_int32
pixSetPadBits(PIX     *pix,
              l_int32  val)
{
l_int32    i, w, h, d, wpl, endbits, fullwords;
l_uint32   mask;
l_uint32  *data, *pword;

    PROCNAME("pixSetPadBits");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixGetDimensions(pix, &w, &h, &d);
    if (d == 32)  /* no padding exists for 32 bpp */
        return 0;

    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    endbits = 32 - ((w * d) % 32);
    if (endbits == 32)  /* no partial word */
        return 0;
    fullwords = w * d / 32;

    mask = rmask32[endbits];
    if (val == 0)
        mask = ~mask;

    for (i = 0; i < h; i++) {
        pword = data + i * wpl + fullwords;
        if (val == 0) /* clear */
            *pword = *pword & mask;
        else  /* set */
            *pword = *pword | mask;
    }

    return 0;
}


/*!
 *  pixSetPadBitsBand()
 *
 *      Input:  pix (1, 2, 4, 8, 16, 32 bpp)
 *              by  (starting y value of band)
 *              bh  (height of band)
 *              val  (0 or 1)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The pad bits are the bits that expand each scanline to a
 *          multiple of 32 bits.  They are usually not used in
 *          image processing operations.  When boundary conditions
 *          are important, as in seedfill, they must be set properly.
 *      (2) This sets the value of the pad bits (if any) in the last
 *          32-bit word in each scanline, within the specified
 *          band of raster lines.
 *      (3) For 32 bpp pix, there are no pad bits, so this is a no-op.
 */
l_int32
pixSetPadBitsBand(PIX     *pix,
                  l_int32  by,
                  l_int32  bh,
                  l_int32  val)
{
l_int32    i, w, h, d, wpl, endbits, fullwords;
l_uint32   mask;
l_uint32  *data, *pword;

    PROCNAME("pixSetPadBitsBand");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixGetDimensions(pix, &w, &h, &d);
    if (d == 32)  /* no padding exists for 32 bpp */
        return 0;

    if (by < 0)
        by = 0;
    if (by >= h)
        return ERROR_INT("start y not in image", procName, 1);
    if (by + bh > h)
        bh = h - by;

    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    endbits = 32 - ((w * d) % 32);
    if (endbits == 32)  /* no partial word */
        return 0;
    fullwords = w * d / 32;

    mask = rmask32[endbits];
    if (val == 0)
        mask = ~mask;

    for (i = by; i < by + bh; i++) {
        pword = data + i * wpl + fullwords;
        if (val == 0) /* clear */
            *pword = *pword & mask;
        else  /* set */
            *pword = *pword | mask;
    }

    return 0;
}


/*-------------------------------------------------------------*
 *                       Set border pixels                     *
 *-------------------------------------------------------------*/
/*!
 *  pixSetOrClearBorder()
 *
 *      Input:  pixs (all depths)
 *              left, right, top, bot (amount to set or clear)
 *              operation (PIX_SET or PIX_CLR)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The border region is defined to be the region in the
 *          image within a specific distance of each edge.  Here, we
 *          allow the pixels within a specified distance of each
 *          edge to be set independently.  This either sets or
 *          clears all pixels in the border region.
 *      (2) For binary images, use PIX_SET for black and PIX_CLR for white.
 *      (3) For grayscale or color images, use PIX_SET for white
 *          and PIX_CLR for black.
 */
l_int32
pixSetOrClearBorder(PIX     *pixs,
                    l_int32  left,
                    l_int32  right,
                    l_int32  top,
                    l_int32  bot,
                    l_int32  op)
{
l_int32  w, h;

    PROCNAME("pixSetOrClearBorder");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (op != PIX_SET && op != PIX_CLR)
        return ERROR_INT("op must be PIX_SET or PIX_CLR", procName, 1);

    pixGetDimensions(pixs, &w, &h, NULL);
    pixRasterop(pixs, 0, 0, left, h, op, NULL, 0, 0);
    pixRasterop(pixs, w - right, 0, right, h, op, NULL, 0, 0);
    pixRasterop(pixs, 0, 0, w, top, op, NULL, 0, 0);
    pixRasterop(pixs, 0, h - bot, w, bot, op, NULL, 0, 0);

    return 0;
}


/*!
 *  pixSetBorderVal()
 *
 *      Input:  pixs (8, 16 or 32 bpp)
 *              left, right, top, bot (amount to set)
 *              val (value to set at each border pixel)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The border region is defined to be the region in the
 *          image within a specific distance of each edge.  Here, we
 *          allow the pixels within a specified distance of each
 *          edge to be set independently.  This sets the pixels
 *          in the border region to the given input value.
 *      (2) For efficiency, use pixSetOrClearBorder() if
 *          you're setting the border to either black or white.
 *      (3) If d != 32, the input value should be masked off
 *          to the appropriate number of least significant bits.
 *      (4) The code is easily generalized for 2 or 4 bpp.
 */
l_int32
pixSetBorderVal(PIX      *pixs,
                l_int32   left,
                l_int32   right,
                l_int32   top,
                l_int32   bot,
                l_uint32  val)
{
l_int32    w, h, d, wpls, i, j, bstart, rstart;
l_uint32  *datas, *lines;

    PROCNAME("pixSetBorderVal");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8 && d != 16 && d != 32)
        return ERROR_INT("depth must be 8, 16 or 32 bpp", procName, 1);

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if (d == 8) {
        val &= 0xff;
        for (i = 0; i < top; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < w; j++)
                SET_DATA_BYTE(lines, j, val);
        }
        rstart = w - right;
        bstart = h - bot;
        for (i = top; i < bstart; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < left; j++)
                SET_DATA_BYTE(lines, j, val);
            for (j = rstart; j < w; j++)
                SET_DATA_BYTE(lines, j, val);
        }
        for (i = bstart; i < h; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < w; j++)
                SET_DATA_BYTE(lines, j, val);
        }
    } else if (d == 16) {
        val &= 0xffff;
        for (i = 0; i < top; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < w; j++)
                SET_DATA_TWO_BYTES(lines, j, val);
        }
        rstart = w - right;
        bstart = h - bot;
        for (i = top; i < bstart; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < left; j++)
                SET_DATA_TWO_BYTES(lines, j, val);
            for (j = rstart; j < w; j++)
                SET_DATA_TWO_BYTES(lines, j, val);
        }
        for (i = bstart; i < h; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < w; j++)
                SET_DATA_TWO_BYTES(lines, j, val);
        }
    } else {   /* d == 32 */
        for (i = 0; i < top; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < w; j++)
                *(lines + j) = val;
        }
        rstart = w - right;
        bstart = h - bot;
        for (i = top; i < bstart; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < left; j++)
                *(lines + j) = val;
            for (j = rstart; j < w; j++)
                *(lines + j) = val;
        }
        for (i = bstart; i < h; i++) {
            lines = datas + i * wpls;
            for (j = 0; j < w; j++)
                *(lines + j) = val;
        }
    }

    return 0;
}


/*!
 *  pixSetBorderRingVal()
 *
 *      Input:  pixs (any depth; cmap OK)
 *              dist (distance from outside; must be > 0; first ring is 1)
 *              val (value to set at each border pixel)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The rings are single-pixel-wide rectangular sets of
 *          pixels at a given distance from the edge of the pix.
 *          This sets all pixels in a given ring to a value.
 */
l_int32
pixSetBorderRingVal(PIX      *pixs,
                    l_int32   dist,
                    l_uint32  val)
{
l_int32  w, h, d, i, j, xend, yend;

    PROCNAME("pixSetBorderRingVal");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (dist < 1)
        return ERROR_INT("dist must be > 0", procName, 1);
    pixGetDimensions(pixs, &w, &h, &d);
    if (w < 2 * dist + 1 || h < 2 * dist + 1)
        return ERROR_INT("ring doesn't exist", procName, 1);
    if (d < 32 && (val >= (1 << d)))
        return ERROR_INT("invalid pixel value", procName, 1);

    xend = w - dist;
    yend = h - dist;
    for (j = dist - 1; j <= xend; j++)
        pixSetPixel(pixs, j, dist - 1, val);
    for (j = dist - 1; j <= xend; j++)
        pixSetPixel(pixs, j, yend, val);
    for (i = dist - 1; i <= yend; i++)
        pixSetPixel(pixs, dist - 1, i, val);
    for (i = dist - 1; i <= yend; i++)
        pixSetPixel(pixs, xend, i, val);

    return 0;
}


/*!
 *  pixSetMirroredBorder()
 *
 *      Input:  pixs (all depths; colormap ok)
 *              left, right, top, bot (number of pixels to set)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This applies what is effectively mirror boundary conditions
 *          to a border region in the image.  It is in-place.
 *      (2) This is useful for setting pixels near the border to a
 *          value representative of the near pixels to the interior.
 *      (3) The general pixRasterop() is used for an in-place operation here
 *          because there is no overlap between the src and dest rectangles.
 */
l_int32
pixSetMirroredBorder(PIX     *pixs,
                     l_int32  left,
                     l_int32  right,
                     l_int32  top,
                     l_int32  bot)
{
l_int32  i, j, w, h;

    PROCNAME("pixSetMirroredBorder");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

    pixGetDimensions(pixs, &w, &h, NULL);
    for (j = 0; j < left; j++)
        pixRasterop(pixs, left - 1 - j, top, 1, h - top - bot, PIX_SRC,
                    pixs, left + j, top);
    for (j = 0; j < right; j++)
        pixRasterop(pixs, w - right + j, top, 1, h - top - bot, PIX_SRC,
                    pixs, w - right - 1 - j, top);
    for (i = 0; i < top; i++)
        pixRasterop(pixs, 0, top - 1 - i, w, 1, PIX_SRC,
                    pixs, 0, top + i);
    for (i = 0; i < bot; i++)
        pixRasterop(pixs, 0, h - bot + i, w, 1, PIX_SRC,
                    pixs, 0, h - bot - 1 - i);

    return 0;
}


/*!
 *  pixCopyBorder()
 *
 *      Input:  pixd (all depths; colormap ok; can be NULL)
 *              pixs (same depth and size as pixd)
 *              left, right, top, bot (number of pixels to copy)
 *      Return: pixd, or null on error if pixd is not defined
 *
 *  Notes:
 *      (1) pixd can be null, but otherwise it must be the same size
 *          and depth as pixs.  Always returns pixd.
 *      (1) This is useful in situations where by setting a few border
 *          pixels we can avoid having to copy all pixels in pixs into
 *          pixd as an initialization step for some operation.
 */
PIX *
pixCopyBorder(PIX     *pixd,
              PIX     *pixs,
              l_int32  left,
              l_int32  right,
              l_int32  top,
              l_int32  bot)
{
l_int32  w, h;

    PROCNAME("pixCopyBorder");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);

    if (pixd) {
        if (pixd == pixs) {
            L_WARNING("same: nothing to do\n", procName);
            return pixd;
        } else if (!pixSizesEqual(pixs, pixd)) {
            return (PIX *)ERROR_PTR("pixs and pixd sizes differ",
                                    procName, pixd);
        }
    } else {
        if ((pixd = pixCreateTemplateNoInit(pixs)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, pixd);
    }

    pixGetDimensions(pixs, &w, &h, NULL);
    pixRasterop(pixd, 0, 0, left, h, PIX_SRC, pixs, 0, 0);
    pixRasterop(pixd, w - right, 0, right, h, PIX_SRC, pixs, w - right, 0);
    pixRasterop(pixd, 0, 0, w, top, PIX_SRC, pixs, 0, 0);
    pixRasterop(pixd, 0, h - bot, w, bot, PIX_SRC, pixs, 0, h - bot);

    return pixd;
}



/*-------------------------------------------------------------*
 *                     Add and remove border                   *
 *-------------------------------------------------------------*/
/*!
 *  pixAddBorder()
 *
 *      Input:  pixs (all depths; colormap ok)
 *              npix (number of pixels to be added to each side)
 *              val  (value of added border pixels)
 *      Return: pixd (with the added exterior pixels), or null on error
 *
 *  Notes:
 *      (1) See pixGetBlackOrWhiteVal() for values of black and white pixels.
 */
PIX *
pixAddBorder(PIX      *pixs,
             l_int32   npix,
             l_uint32  val)
{
    PROCNAME("pixAddBorder");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (npix == 0)
        return pixClone(pixs);
    return pixAddBorderGeneral(pixs, npix, npix, npix, npix, val);
}


/*!
 *  pixAddBlackOrWhiteBorder()
 *
 *      Input:  pixs (all depths; colormap ok)
 *              left, right, top, bot  (number of pixels added)
 *              op (L_GET_BLACK_VAL, L_GET_WHITE_VAL)
 *      Return: pixd (with the added exterior pixels), or null on error
 *
 *  Notes:
 *      (1) See pixGetBlackOrWhiteVal() for possible side effect (adding
 *          a color to a colormap).
 *      (2) The only complication is that pixs may have a colormap.
 *          There are two ways to add the black or white border:
 *          (a) As done here (simplest, most efficient)
 *          (b) l_int32 ws, hs, d;
 *              pixGetDimensions(pixs, &ws, &hs, &d);
 *              Pix *pixd = pixCreate(ws + left + right, hs + top + bot, d);
 *              PixColormap *cmap = pixGetColormap(pixs);
 *              if (cmap != NULL)
 *                  pixSetColormap(pixd, pixcmapCopy(cmap));
 *              pixSetBlackOrWhite(pixd, L_SET_WHITE);  // uses cmap
 *              pixRasterop(pixd, left, top, ws, hs, PIX_SET, pixs, 0, 0);
 */
PIX *
pixAddBlackOrWhiteBorder(PIX     *pixs,
                         l_int32  left,
                         l_int32  right,
                         l_int32  top,
                         l_int32  bot,
                         l_int32  op)
{
l_uint32  val;

    PROCNAME("pixAddBlackOrWhiteBorder");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (op != L_GET_BLACK_VAL && op != L_GET_WHITE_VAL)
        return (PIX *)ERROR_PTR("invalid op", procName, NULL);

    pixGetBlackOrWhiteVal(pixs, op, &val);
    return pixAddBorderGeneral(pixs, left, right, top, bot, val);
}


/*!
 *  pixAddBorderGeneral()
 *
 *      Input:  pixs (all depths; colormap ok)
 *              left, right, top, bot  (number of pixels added)
 *              val   (value of added border pixels)
 *      Return: pixd (with the added exterior pixels), or null on error
 *
 *  Notes:
 *      (1) For binary images:
 *             white:  val = 0
 *             black:  val = 1
 *          For grayscale images:
 *             white:  val = 2 ** d - 1
 *             black:  val = 0
 *          For rgb color images:
 *             white:  val = 0xffffff00
 *             black:  val = 0
 *          For colormapped images, set val to the appropriate colormap index.
 *      (2) If the added border is either black or white, you can use
 *             pixAddBlackOrWhiteBorder()
 *          The black and white values for all images can be found with
 *             pixGetBlackOrWhiteVal()
 *          which, if pixs is cmapped, may add an entry to the colormap.
 *          Alternatively, if pixs has a colormap, you can find the index
 *          of the pixel whose intensity is closest to white or black:
 *             white: pixcmapGetRankIntensity(cmap, 1.0, &index);
 *             black: pixcmapGetRankIntensity(cmap, 0.0, &index);
 *          and use that for val.
 */
PIX *
pixAddBorderGeneral(PIX      *pixs,
                    l_int32   left,
                    l_int32   right,
                    l_int32   top,
                    l_int32   bot,
                    l_uint32  val)
{
l_int32  ws, hs, wd, hd, d, maxval, op;
PIX     *pixd;

    PROCNAME("pixAddBorderGeneral");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (left < 0 || right < 0 || top < 0 || bot < 0)
        return (PIX *)ERROR_PTR("negative border added!", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, &d);
    wd = ws + left + right;
    hd = hs + top + bot;
    if ((pixd = pixCreateNoInit(wd, hd, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixCopyColormap(pixd, pixs);

        /* Set the new border pixels */
    maxval = (d == 32) ? 0xffffff00 : (1 << d) - 1;
    op = UNDEF;
    if (val == 0)
        op = PIX_CLR;
    else if (val >= maxval)
        op = PIX_SET;
    if (op == UNDEF) {
        pixSetAllArbitrary(pixd, val);
    } else {  /* just set or clear the border pixels */
        pixRasterop(pixd, 0, 0, left, hd, op, NULL, 0, 0);
        pixRasterop(pixd, wd - right, 0, right, hd, op, NULL, 0, 0);
        pixRasterop(pixd, 0, 0, wd, top, op, NULL, 0, 0);
        pixRasterop(pixd, 0, hd - bot, wd, bot, op, NULL, 0, 0);
    }

        /* Copy pixs into the interior */
    pixRasterop(pixd, left, top, ws, hs, PIX_SRC, pixs, 0, 0);
    return pixd;
}


/*!
 *  pixRemoveBorder()
 *
 *      Input:  pixs (all depths; colormap ok)
 *              npix (number to be removed from each of the 4 sides)
 *      Return: pixd (with pixels removed around border), or null on error
 */
PIX *
pixRemoveBorder(PIX     *pixs,
                l_int32  npix)
{
    PROCNAME("pixRemoveBorder");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (npix == 0)
        return pixClone(pixs);
    return pixRemoveBorderGeneral(pixs, npix, npix, npix, npix);
}


/*!
 *  pixRemoveBorderGeneral()
 *
 *      Input:  pixs (all depths; colormap ok)
 *              left, right, top, bot  (number of pixels removed)
 *      Return: pixd (with pixels removed around border), or null on error
 */
PIX *
pixRemoveBorderGeneral(PIX     *pixs,
                       l_int32  left,
                       l_int32  right,
                       l_int32  top,
                       l_int32  bot)
{
l_int32  ws, hs, wd, hd, d;
PIX     *pixd;

    PROCNAME("pixRemoveBorderGeneral");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (left < 0 || right < 0 || top < 0 || bot < 0)
        return (PIX *)ERROR_PTR("negative border removed!", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, &d);
    wd = ws - left - right;
    hd = hs - top - bot;
    if (wd <= 0)
        return (PIX *)ERROR_PTR("width must be > 0", procName, NULL);
    if (hd <= 0)
        return (PIX *)ERROR_PTR("height must be > 0", procName, NULL);
    if ((pixd = pixCreateNoInit(wd, hd, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixCopySpp(pixd, pixs);
    pixCopyColormap(pixd, pixs);

    pixRasterop(pixd, 0, 0, wd, hd, PIX_SRC, pixs, left, top);
    if (pixGetDepth(pixs) == 32 && pixGetSpp(pixs) == 4)
        pixShiftAndTransferAlpha(pixd, pixs, -left, -top);
    return pixd;
}


/*!
 *  pixRemoveBorderToSize()
 *
 *      Input:  pixs (all depths; colormap ok)
 *              wd  (target width; use 0 if only removing from height)
 *              hd  (target height; use 0 if only removing from width)
 *      Return: pixd (with pixels removed around border), or null on error
 *
 *  Notes:
 *      (1) Removes pixels as evenly as possible from the sides of the
 *          image, leaving the central part.
 *      (2) Returns clone if no pixels requested removed, or the target
 *          sizes are larger than the image.
 */
PIX *
pixRemoveBorderToSize(PIX     *pixs,
                      l_int32  wd,
                      l_int32  hd)
{
l_int32  w, h, top, bot, left, right, delta;

    PROCNAME("pixRemoveBorderToSize");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    if ((wd <= 0 || wd >= w) && (hd <= 0 || hd >= h))
        return pixClone(pixs);

    left = right = (w - wd) / 2;
    delta = w - 2 * left - wd;
    right += delta;
    top = bot = (h - hd) / 2;
    delta = h - hd - 2 * top;
    bot += delta;
    if (wd <= 0 || wd > w)
        left = right = 0;
    else if (hd <= 0 || hd > h)
        top = bot = 0;

    return pixRemoveBorderGeneral(pixs, left, right, top, bot);
}


/*!
 *  pixAddMirroredBorder()
 *
 *      Input:  pixs (all depths; colormap ok)
 *              left, right, top, bot (number of pixels added)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This applies what is effectively mirror boundary conditions.
 *          For the added border pixels in pixd, the pixels in pixs
 *          near the border are mirror-copied into the border region.
 *      (2) This is useful for avoiding special operations near
 *          boundaries when doing image processing operations
 *          such as rank filters and convolution.  In use, one first
 *          adds mirrored pixels to each side of the image.  The number
 *          of pixels added on each side is half the filter dimension.
 *          Then the image processing operations proceed over a
 *          region equal to the size of the original image, and
 *          write directly into a dest pix of the same size as pixs.
 *      (3) The general pixRasterop() is used for an in-place operation here
 *          because there is no overlap between the src and dest rectangles.
 */
PIX  *
pixAddMirroredBorder(PIX      *pixs,
                      l_int32  left,
                      l_int32  right,
                      l_int32  top,
                      l_int32  bot)
{
l_int32  i, j, w, h;
PIX     *pixd;

    PROCNAME("pixAddMirroredBorder");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);
    if (left > w || right > w || top > h || bot > h)
        return (PIX *)ERROR_PTR("border too large", procName, NULL);

        /* Set pixels on left, right, top and bottom, in that order */
    pixd = pixAddBorderGeneral(pixs, left, right, top, bot, 0);
    for (j = 0; j < left; j++)
        pixRasterop(pixd, left - 1 - j, top, 1, h, PIX_SRC,
                    pixd, left + j, top);
    for (j = 0; j < right; j++)
        pixRasterop(pixd, left + w + j, top, 1, h, PIX_SRC,
                    pixd, left + w - 1 - j, top);
    for (i = 0; i < top; i++)
        pixRasterop(pixd, 0, top - 1 - i, left + w + right, 1, PIX_SRC,
                    pixd, 0, top + i);
    for (i = 0; i < bot; i++)
        pixRasterop(pixd, 0, top + h + i, left + w + right, 1, PIX_SRC,
                    pixd, 0, top + h - 1 - i);

    return pixd;
}


/*!
 *  pixAddRepeatedBorder()
 *
 *      Input:  pixs (all depths; colormap ok)
 *              left, right, top, bot (number of pixels added)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This applies a repeated border, as if the central part of
 *          the image is tiled over the plane.  So, for example, the
 *          pixels in the left border come from the right side of the image.
 *      (2) The general pixRasterop() is used for an in-place operation here
 *          because there is no overlap between the src and dest rectangles.
 */
PIX  *
pixAddRepeatedBorder(PIX      *pixs,
                     l_int32  left,
                     l_int32  right,
                     l_int32  top,
                     l_int32  bot)
{
l_int32  w, h;
PIX     *pixd;

    PROCNAME("pixAddRepeatedBorder");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);
    if (left > w || right > w || top > h || bot > h)
        return (PIX *)ERROR_PTR("border too large", procName, NULL);

    pixd = pixAddBorderGeneral(pixs, left, right, top, bot, 0);

        /* Set pixels on left, right, top and bottom, in that order */
    pixRasterop(pixd, 0, top, left, h, PIX_SRC, pixd, w, top);
    pixRasterop(pixd, left + w, top, right, h, PIX_SRC, pixd, left, top);
    pixRasterop(pixd, 0, 0, left + w + right, top, PIX_SRC, pixd, 0, h);
    pixRasterop(pixd, 0, top + h, left + w + right, bot, PIX_SRC, pixd, 0, top);

    return pixd;
}


/*!
 *  pixAddMixedBorder()
 *
 *      Input:  pixs (all depths; colormap ok)
 *              left, right, top, bot (number of pixels added)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This applies mirrored boundary conditions horizontally
 *          and repeated b.c. vertically.
 *      (2) It is specifically used for avoiding special operations
 *          near boundaries when convolving a hue-saturation histogram
 *          with a given window size.  The repeated b.c. are used
 *          vertically for hue, and the mirrored b.c. are used
 *          horizontally for saturation.  The number of pixels added
 *          on each side is approximately (but not quite) half the
 *          filter dimension.  The image processing operations can
 *          then proceed over a region equal to the size of the original
 *          image, and write directly into a dest pix of the same
 *          size as pixs.
 *      (3) The general pixRasterop() can be used for an in-place
 *          operation here because there is no overlap between the
 *          src and dest rectangles.
 */
PIX  *
pixAddMixedBorder(PIX      *pixs,
                  l_int32  left,
                  l_int32  right,
                  l_int32  top,
                  l_int32  bot)
{
l_int32  j, w, h;
PIX     *pixd;

    PROCNAME("pixAddMixedBorder");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);
    if (left > w || right > w || top > h || bot > h)
        return (PIX *)ERROR_PTR("border too large", procName, NULL);

        /* Set mirrored pixels on left and right;
         * then set repeated pixels on top and bottom. */
    pixd = pixAddBorderGeneral(pixs, left, right, top, bot, 0);
    for (j = 0; j < left; j++)
        pixRasterop(pixd, left - 1 - j, top, 1, h, PIX_SRC,
                    pixd, left + j, top);
    for (j = 0; j < right; j++)
        pixRasterop(pixd, left + w + j, top, 1, h, PIX_SRC,
                    pixd, left + w - 1 - j, top);
    pixRasterop(pixd, 0, 0, left + w + right, top, PIX_SRC, pixd, 0, h);
    pixRasterop(pixd, 0, top + h, left + w + right, bot, PIX_SRC, pixd, 0, top);

    return pixd;
}


/*!
 *  pixAddContinuedBorder()
 *
 *      Input:  pixs
 *              left, right, top, bot (pixels on each side to be added)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This adds pixels on each side whose values are equal to
 *          the value on the closest boundary pixel.
 */
PIX *
pixAddContinuedBorder(PIX     *pixs,
                      l_int32  left,
                      l_int32  right,
                      l_int32  top,
                      l_int32  bot)
{
l_int32  i, j, w, h;
PIX     *pixd;

    PROCNAME("pixAddContinuedBorder");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    pixd = pixAddBorderGeneral(pixs, left, right, top, bot, 0);
    pixGetDimensions(pixs, &w, &h, NULL);
    for (j = 0; j < left; j++)
        pixRasterop(pixd, j, top, 1, h, PIX_SRC, pixd, left, top);
    for (j = 0; j < right; j++)
        pixRasterop(pixd, left + w + j, top, 1, h,
                    PIX_SRC, pixd, left + w - 1, top);
    for (i = 0; i < top; i++)
        pixRasterop(pixd, 0, i, left + w + right, 1, PIX_SRC, pixd, 0, top);
    for (i = 0; i < bot; i++)
        pixRasterop(pixd, 0, top + h + i, left + w + right, 1,
                    PIX_SRC, pixd, 0, top + h - 1);

    return pixd;
}


/*-------------------------------------------------------------------*
 *                     Helper functions using alpha                  *
 *-------------------------------------------------------------------*/
/*!
 *  pixShiftAndTransferAlpha()
 *
 *      Input:  pixd  (32 bpp)
 *              pixs  (32 bpp)
 *              shiftx, shifty
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixShiftAndTransferAlpha(PIX       *pixd,
                         PIX       *pixs,
                         l_float32  shiftx,
                         l_float32  shifty)
{
l_int32  w, h;
PIX     *pix1, *pix2;

    PROCNAME("pixShiftAndTransferAlpha");

    if (!pixs || !pixd)
        return ERROR_INT("pixs and pixd not both defined", procName, 1);
    if (pixGetDepth(pixs) != 32 || pixGetSpp(pixs) != 4)
        return ERROR_INT("pixs not 32 bpp and 4 spp", procName, 1);
    if (pixGetDepth(pixd) != 32)
        return ERROR_INT("pixd not 32 bpp", procName, 1);

    if (shiftx == 0 && shifty == 0) {
        pixCopyRGBComponent(pixd, pixs, L_ALPHA_CHANNEL);
        return 0;
    }

    pix1 = pixGetRGBComponent(pixs, L_ALPHA_CHANNEL);
    pixGetDimensions(pixd, &w, &h, NULL);
    pix2 = pixCreate(w, h, 8);
    pixRasterop(pix2, 0, 0, w, h, PIX_SRC, pix1, -shiftx, -shifty);
    pixSetRGBComponent(pixd, pix2, L_ALPHA_CHANNEL);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    return 0;
}


/*!
 *  pixDisplayLayersRGBA()
 *
 *      Input:  pixs (cmap or 32 bpp rgba)
 *              val (32 bit unsigned color to use as background)
 *              maxw (max output image width; 0 for no scaling)
 *      Return: pixd (showing various image views), or null on error
 *
 *  Notes:
 *      (1) Use @val == 0xffffff00 for white background.
 *      (2) Three views are given:
 *           - the image with a fully opaque alpha
 *           - the alpha layer
 *           - the image as it would appear with a white background.
 */
PIX *
pixDisplayLayersRGBA(PIX      *pixs,
                     l_uint32  val,
                     l_int32   maxw)
{
l_int32    w, width;
l_float32  scalefact;
PIX       *pix1, *pix2, *pixd;
PIXA      *pixa;
PIXCMAP   *cmap;

    PROCNAME("pixDisplayLayersRGBA");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    cmap = pixGetColormap(pixs);
    if (!cmap && !(pixGetDepth(pixs) == 32 && pixGetSpp(pixs) == 4))
        return (PIX *)ERROR_PTR("pixs not cmap and not 32 bpp rgba",
                                procName, NULL);
    if ((w = pixGetWidth(pixs)) == 0)
        return (PIX *)ERROR_PTR("pixs width 0 !!", procName, NULL);

    if (cmap)
        pix1 = pixRemoveColormap(pixs, REMOVE_CMAP_WITH_ALPHA);
    else
        pix1 = pixCopy(NULL, pixs);

        /* Scale if necessary so the output width is not larger than maxw */
    scalefact = (maxw == 0) ? 1.0 : L_MIN(1.0, maxw / w);
    width = (l_int32)(scalefact * w);

    pixa = pixaCreate(3);
    pixSetSpp(pix1, 3);
    pixaAddPix(pixa, pix1, L_INSERT);  /* show the rgb values */
    pix1 = pixGetRGBComponent(pixs, L_ALPHA_CHANNEL);
    pix2 = pixConvertTo32(pix1);
    pixaAddPix(pixa, pix2, L_INSERT);  /* show the alpha channel */
    pixDestroy(&pix1);
    pix1 = pixAlphaBlendUniform(pixs, (val & 0xffffff00));
    pixaAddPix(pixa, pix1, L_INSERT);  /* with @val color bg showing */
    pixd = pixaDisplayTiledInRows(pixa, 32, width, scalefact, 0, 25, 2);
    pixaDestroy(&pixa);
    return pixd;
}


/*-------------------------------------------------------------*
 *                Color sample setting and extraction          *
 *-------------------------------------------------------------*/
/*!
 *  pixCreateRGBImage()
 *
 *      Input:  8 bpp red pix
 *              8 bpp green pix
 *              8 bpp blue pix
 *      Return: 32 bpp pix, interleaved with 4 samples/pixel,
 *              or null on error
 *
 *  Notes:
 *      (1) the 4th byte, sometimes called the "alpha channel",
 *          and which is often used for blending between different
 *          images, is left with 0 value.
 *      (2) see Note (4) in pix.h for details on storage of
 *          8-bit samples within each 32-bit word.
 *      (3) This implementation, setting the r, g and b components
 *          sequentially, is much faster than setting them in parallel
 *          by constructing an RGB dest pixel and writing it to dest.
 *          The reason is there are many more cache misses when reading
 *          from 3 input images simultaneously.
 */
PIX *
pixCreateRGBImage(PIX  *pixr,
                  PIX  *pixg,
                  PIX  *pixb)
{
l_int32  wr, wg, wb, hr, hg, hb, dr, dg, db;
PIX     *pixd;

    PROCNAME("pixCreateRGBImage");

    if (!pixr)
        return (PIX *)ERROR_PTR("pixr not defined", procName, NULL);
    if (!pixg)
        return (PIX *)ERROR_PTR("pixg not defined", procName, NULL);
    if (!pixb)
        return (PIX *)ERROR_PTR("pixb not defined", procName, NULL);
    pixGetDimensions(pixr, &wr, &hr, &dr);
    pixGetDimensions(pixg, &wg, &hg, &dg);
    pixGetDimensions(pixb, &wb, &hb, &db);
    if (dr != 8 || dg != 8 || db != 8)
        return (PIX *)ERROR_PTR("input pix not all 8 bpp", procName, NULL);
    if (wr != wg || wr != wb)
        return (PIX *)ERROR_PTR("widths not the same", procName, NULL);
    if (hr != hg || hr != hb)
        return (PIX *)ERROR_PTR("heights not the same", procName, NULL);

    if ((pixd = pixCreate(wr, hr, 32)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixr);
    pixSetRGBComponent(pixd, pixr, COLOR_RED);
    pixSetRGBComponent(pixd, pixg, COLOR_GREEN);
    pixSetRGBComponent(pixd, pixb, COLOR_BLUE);

    return pixd;
}


/*!
 *  pixGetRGBComponent()
 *
 *      Input:  pixs (32 bpp, or colormapped)
 *              comp (one of {COLOR_RED, COLOR_GREEN, COLOR_BLUE,
 *                    L_ALPHA_CHANNEL})
 *      Return: pixd (the selected 8 bpp component image of the
 *                    input 32 bpp image) or null on error
 *
 *  Notes:
 *      (1) Three calls to this function generate the r, g and b 8 bpp
 *          component images.  This is much faster than generating the
 *          three images in parallel, by extracting a src pixel and setting
 *          the pixels of each component image from it.  The reason is
 *          there are many more cache misses when writing to three
 *          output images simultaneously.
 */
PIX *
pixGetRGBComponent(PIX     *pixs,
                   l_int32  comp)
{
l_int32    i, j, w, h, wpls, wpld, val;
l_uint32  *lines, *lined;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixGetRGBComponent");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetColormap(pixs))
        return pixGetRGBComponentCmap(pixs, comp);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs not 32 bpp", procName, NULL);
    if (comp != COLOR_RED && comp != COLOR_GREEN &&
        comp != COLOR_BLUE && comp != L_ALPHA_CHANNEL)
        return (PIX *)ERROR_PTR("invalid comp", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            val = GET_DATA_BYTE(lines + j, comp);
            SET_DATA_BYTE(lined, j, val);
        }
    }

    return pixd;
}


/*!
 *  pixSetRGBComponent()
 *
 *      Input:  pixd  (32 bpp)
 *              pixs  (8 bpp)
 *              comp  (one of the set: {COLOR_RED, COLOR_GREEN,
 *                                      COLOR_BLUE, L_ALPHA_CHANNEL})
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This places the 8 bpp pixel in pixs into the
 *          specified component (properly interleaved) in pixd,
 *      (2) The two images are registered to the UL corner; the sizes
 *          need not be the same, but a warning is issued if they differ.
 */
l_int32
pixSetRGBComponent(PIX     *pixd,
                   PIX     *pixs,
                   l_int32  comp)
{
l_uint8    srcbyte;
l_int32    i, j, w, h, ws, hs, wd, hd;
l_int32    wpls, wpld;
l_uint32  *lines, *lined;
l_uint32  *datas, *datad;

    PROCNAME("pixSetRGBComponent");

    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetDepth(pixd) != 32)
        return ERROR_INT("pixd not 32 bpp", procName, 1);
    if (pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not 8 bpp", procName, 1);
    if (comp != COLOR_RED && comp != COLOR_GREEN &&
        comp != COLOR_BLUE && comp != L_ALPHA_CHANNEL)
        return ERROR_INT("invalid comp", procName, 1);
    pixGetDimensions(pixs, &ws, &hs, NULL);
    pixGetDimensions(pixd, &wd, &hd, NULL);
    if (ws != wd || hs != hd)
        L_WARNING("images sizes not equal\n", procName);
    w = L_MIN(ws, wd);
    h = L_MIN(hs, hd);
    if (comp == L_ALPHA_CHANNEL)
        pixSetSpp(pixd, 4);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            srcbyte = GET_DATA_BYTE(lines, j);
            SET_DATA_BYTE(lined + j, comp, srcbyte);
        }
    }

    return 0;
}


/*!
 *  pixGetRGBComponentCmap()
 *
 *      Input:  pixs  (colormapped)
 *              comp  (one of the set: {COLOR_RED, COLOR_GREEN, COLOR_BLUE})
 *      Return: pixd  (the selected 8 bpp component image of the
 *                     input cmapped image), or null on error
 *
 *  Notes:
 *      (1) In leptonica, we do not support alpha in colormaps.
 */
PIX *
pixGetRGBComponentCmap(PIX     *pixs,
                       l_int32  comp)
{
l_int32     i, j, w, h, val, index;
l_int32     wplc, wpld;
l_uint32   *linec, *lined;
l_uint32   *datac, *datad;
PIX        *pixc, *pixd;
PIXCMAP    *cmap;
RGBA_QUAD  *cta;

    PROCNAME("pixGetRGBComponentCmap");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if ((cmap = pixGetColormap(pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixs not cmapped", procName, NULL);
    if (comp == L_ALPHA_CHANNEL)
        return (PIX *)ERROR_PTR("alpha in cmaps not supported", procName, NULL);
    if (comp != COLOR_RED && comp != COLOR_GREEN && comp != COLOR_BLUE)
        return (PIX *)ERROR_PTR("invalid comp", procName, NULL);

        /* If not 8 bpp, make a cmapped 8 bpp pix */
    if (pixGetDepth(pixs) == 8)
        pixc = pixClone(pixs);
    else
        pixc = pixConvertTo8(pixs, TRUE);

    pixGetDimensions(pixs, &w, &h, NULL);
    if ((pixd = pixCreateNoInit(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    wplc = pixGetWpl(pixc);
    wpld = pixGetWpl(pixd);
    datac = pixGetData(pixc);
    datad = pixGetData(pixd);
    cta = (RGBA_QUAD *)cmap->array;

    for (i = 0; i < h; i++) {
        linec = datac + i * wplc;
        lined = datad + i * wpld;
        if (comp == COLOR_RED) {
            for (j = 0; j < w; j++) {
                index = GET_DATA_BYTE(linec, j);
                val = cta[index].red;
                SET_DATA_BYTE(lined, j, val);
            }
        } else if (comp == COLOR_GREEN) {
            for (j = 0; j < w; j++) {
                index = GET_DATA_BYTE(linec, j);
                val = cta[index].green;
                SET_DATA_BYTE(lined, j, val);
            }
        } else if (comp == COLOR_BLUE) {
            for (j = 0; j < w; j++) {
                index = GET_DATA_BYTE(linec, j);
                val = cta[index].blue;
                SET_DATA_BYTE(lined, j, val);
            }
        }
    }

    pixDestroy(&pixc);
    return pixd;
}


/*!
 *  pixCopyRGBComponent()
 *
 *      Input:  pixd (32 bpp)
 *              pixs (32 bpp)
 *              comp (one of the set: {COLOR_RED, COLOR_GREEN,
 *                                     COLOR_BLUE, L_ALPHA_CHANNEL})
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The two images are registered to the UL corner.  The sizes
 *          are usually the same, and a warning is issued if they differ.
 */
l_int32
pixCopyRGBComponent(PIX     *pixd,
                    PIX     *pixs,
                    l_int32  comp)
{
l_int32    i, j, w, h, ws, hs, wd, hd, val;
l_int32    wpls, wpld;
l_uint32  *lines, *lined;
l_uint32  *datas, *datad;

    PROCNAME("pixCopyRGBComponent");

    if (!pixd && pixGetDepth(pixd) != 32)
        return ERROR_INT("pixd not defined or not 32 bpp", procName, 1);
    if (!pixs && pixGetDepth(pixs) != 32)
        return ERROR_INT("pixs not defined or not 32 bpp", procName, 1);
    if (comp != COLOR_RED && comp != COLOR_GREEN &&
        comp != COLOR_BLUE && comp != L_ALPHA_CHANNEL)
        return ERROR_INT("invalid component", procName, 1);
    pixGetDimensions(pixs, &ws, &hs, NULL);
    pixGetDimensions(pixd, &wd, &hd, NULL);
    if (ws != wd || hs != hd)
        L_WARNING("images sizes not equal\n", procName);
    w = L_MIN(ws, wd);
    h = L_MIN(hs, hd);
    if (comp == L_ALPHA_CHANNEL)
        pixSetSpp(pixd, 4);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            val = GET_DATA_BYTE(lines + j, comp);
            SET_DATA_BYTE(lined + j, comp, val);
        }
    }
    return 0;
}


/*!
 *  composeRGBPixel()
 *
 *      Input:  rval, gval, bval
 *              &pixel  (<return> 32-bit pixel)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) All channels are 8 bits: the input values must be between
 *          0 and 255.  For speed, this is not enforced by masking
 *          with 0xff before shifting.
 *      (2) A slower implementation uses macros:
 *            SET_DATA_BYTE(ppixel, COLOR_RED, rval);
 *            SET_DATA_BYTE(ppixel, COLOR_GREEN, gval);
 *            SET_DATA_BYTE(ppixel, COLOR_BLUE, bval);
 */
l_int32
composeRGBPixel(l_int32    rval,
                l_int32    gval,
                l_int32    bval,
                l_uint32  *ppixel)
{
    PROCNAME("composeRGBPixel");

    if (!ppixel)
        return ERROR_INT("&pixel not defined", procName, 1);

    *ppixel = (rval << L_RED_SHIFT) | (gval << L_GREEN_SHIFT) |
              (bval << L_BLUE_SHIFT);
    return 0;
}


/*!
 *  composeRGBAPixel()
 *
 *      Input:  rval, gval, bval, aval
 *              &pixel  (<return> 32-bit pixel)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) All channels are 8 bits: the input values must be between
 *          0 and 255.  For speed, this is not enforced by masking
 *          with 0xff before shifting.
 */
l_int32
composeRGBAPixel(l_int32    rval,
                 l_int32    gval,
                 l_int32    bval,
                 l_int32    aval,
                 l_uint32  *ppixel)
{
    PROCNAME("composeRGBAPixel");

    if (!ppixel)
        return ERROR_INT("&pixel not defined", procName, 1);

    *ppixel = (rval << L_RED_SHIFT) | (gval << L_GREEN_SHIFT) |
              (bval << L_BLUE_SHIFT) | aval;
    return 0;
}


/*!
 *  extractRGBValues()
 *
 *      Input:  pixel (32 bit)
 *              &rval (<optional return> red component)
 *              &gval (<optional return> green component)
 *              &bval (<optional return> blue component)
 *      Return: void
 *
 *  Notes:
 *      (1) A slower implementation uses macros:
 *             *prval = GET_DATA_BYTE(&pixel, COLOR_RED);
 *             *pgval = GET_DATA_BYTE(&pixel, COLOR_GREEN);
 *             *pbval = GET_DATA_BYTE(&pixel, COLOR_BLUE);
 */
void
extractRGBValues(l_uint32  pixel,
                 l_int32  *prval,
                 l_int32  *pgval,
                 l_int32  *pbval)
{
    if (prval) *prval = (pixel >> L_RED_SHIFT) & 0xff;
    if (pgval) *pgval = (pixel >> L_GREEN_SHIFT) & 0xff;
    if (pbval) *pbval = (pixel >> L_BLUE_SHIFT) & 0xff;
    return;
}


/*!
 *  extractRGBAValues()
 *
 *      Input:  pixel (32 bit)
 *              &rval (<optional return> red component)
 *              &gval (<optional return> green component)
 *              &bval (<optional return> blue component)
 *              &aval (<optional return> alpha component)
 *      Return: void
 */
void
extractRGBAValues(l_uint32  pixel,
                  l_int32  *prval,
                  l_int32  *pgval,
                  l_int32  *pbval,
                  l_int32  *paval)
{
    if (prval) *prval = (pixel >> L_RED_SHIFT) & 0xff;
    if (pgval) *pgval = (pixel >> L_GREEN_SHIFT) & 0xff;
    if (pbval) *pbval = (pixel >> L_BLUE_SHIFT) & 0xff;
    if (paval) *paval = (pixel >> L_ALPHA_SHIFT) & 0xff;
    return;
}


/*!
 *  extractMinMaxComponent()
 *
 *      Input:  pixel (32 bpp RGB)
 *              type (L_CHOOSE_MIN or L_CHOOSE_MAX)
 *      Return: component (in range [0 ... 255], or null on error
 */
l_int32
extractMinMaxComponent(l_uint32  pixel,
                       l_int32   type)
{
l_int32  rval, gval, bval, val;

    extractRGBValues(pixel, &rval, &gval, &bval);
    if (type == L_CHOOSE_MIN) {
        val = L_MIN(rval, gval);
        val = L_MIN(val, bval);
    } else {  /* type == L_CHOOSE_MAX */
        val = L_MAX(rval, gval);
        val = L_MAX(val, bval);
    }
    return val;
}


/*!
 *  pixGetRGBLine()
 *
 *      Input:  pixs  (32 bpp)
 *              row
 *              bufr  (array of red samples; size w bytes)
 *              bufg  (array of green samples; size w bytes)
 *              bufb  (array of blue samples; size w bytes)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This puts rgb components from the input line in pixs
 *          into the given buffers.
 */
l_int32
pixGetRGBLine(PIX      *pixs,
              l_int32   row,
              l_uint8  *bufr,
              l_uint8  *bufg,
              l_uint8  *bufb)
{
l_uint32  *lines;
l_int32    j, w, h;
l_int32    wpls;

    PROCNAME("pixGetRGBLine");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetDepth(pixs) != 32)
        return ERROR_INT("pixs not 32 bpp", procName, 1);
    if (!bufr || !bufg || !bufb)
        return ERROR_INT("buffer not defined", procName, 1);

    pixGetDimensions(pixs, &w, &h, NULL);
    if (row < 0 || row >= h)
        return ERROR_INT("row out of bounds", procName, 1);
    wpls = pixGetWpl(pixs);
    lines = pixGetData(pixs) + row * wpls;

    for (j = 0; j < w; j++) {
        bufr[j] = GET_DATA_BYTE(lines + j, COLOR_RED);
        bufg[j] = GET_DATA_BYTE(lines + j, COLOR_GREEN);
        bufb[j] = GET_DATA_BYTE(lines + j, COLOR_BLUE);
    }

    return 0;
}


/*-------------------------------------------------------------*
 *                    Pixel endian conversion                  *
 *-------------------------------------------------------------*/
/*!
 *  pixEndianByteSwapNew()
 *
 *      Input:  pixs
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This is used to convert the data in a pix to a
 *          serialized byte buffer in raster order, and, for RGB,
 *          in order RGBA.  This requires flipping bytes within
 *          each 32-bit word for little-endian platforms, because the
 *          words have a MSB-to-the-left rule, whereas byte raster-order
 *          requires the left-most byte in each word to be byte 0.
 *          For big-endians, no swap is necessary, so this returns a clone.
 *      (2) Unlike pixEndianByteSwap(), which swaps the bytes in-place,
 *          this returns a new pix (or a clone).  We provide this
 *          because often when serialization is done, the source
 *          pix needs to be restored to canonical little-endian order,
 *          and this requires a second byte swap.  In such a situation,
 *          it is twice as fast to make a new pix in big-endian order,
 *          use it, and destroy it.
 */
PIX *
pixEndianByteSwapNew(PIX  *pixs)
{
l_uint32  *datas, *datad;
l_int32    i, j, h, wpl;
l_uint32   word;
PIX       *pixd;

    PROCNAME("pixEndianByteSwapNew");

#ifdef L_BIG_ENDIAN

    return pixClone(pixs);

#else   /* L_LITTLE_ENDIAN */

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    datas = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    h = pixGetHeight(pixs);
    pixd = pixCreateTemplate(pixs);
    datad = pixGetData(pixd);
    for (i = 0; i < h; i++) {
        for (j = 0; j < wpl; j++, datas++, datad++) {
            word = *datas;
            *datad = (word >> 24) |
                    ((word >> 8) & 0x0000ff00) |
                    ((word << 8) & 0x00ff0000) |
                    (word << 24);
        }
    }

    return pixd;

#endif   /* L_BIG_ENDIAN */

}


/*!
 *  pixEndianByteSwap()
 *
 *      Input:  pixs
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is used on little-endian platforms to swap
 *          the bytes within a word; bytes 0 and 3 are swapped,
 *          and bytes 1 and 2 are swapped.
 *      (2) This is required for little-endians in situations
 *          where we convert from a serialized byte order that is
 *          in raster order, as one typically has in file formats,
 *          to one with MSB-to-the-left in each 32-bit word, or v.v.
 *          See pix.h for a description of the canonical format
 *          (MSB-to-the left) that is used for both little-endian
 *          and big-endian platforms.   For big-endians, the
 *          MSB-to-the-left word order has the bytes in raster
 *          order when serialized, so no byte flipping is required.
 */
l_int32
pixEndianByteSwap(PIX  *pixs)
{
l_uint32  *data;
l_int32    i, j, h, wpl;
l_uint32   word;

    PROCNAME("pixEndianByteSwap");

#ifdef L_BIG_ENDIAN

    return 0;

#else   /* L_LITTLE_ENDIAN */

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    h = pixGetHeight(pixs);
    for (i = 0; i < h; i++) {
        for (j = 0; j < wpl; j++, data++) {
            word = *data;
            *data = (word >> 24) |
                    ((word >> 8) & 0x0000ff00) |
                    ((word << 8) & 0x00ff0000) |
                    (word << 24);
        }
    }

    return 0;

#endif   /* L_BIG_ENDIAN */

}


/*!
 *  lineEndianByteSwap()
 *
 *      Input   datad (dest byte array data, reordered on little-endians)
 *              datas (a src line of pix data)
 *              wpl (number of 32 bit words in the line)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is used on little-endian platforms to swap
 *          the bytes within each word in the line of image data.
 *          Bytes 0 <==> 3 and 1 <==> 2 are swapped in the dest
 *          byte array data8d, relative to the pix data in datas.
 *      (2) The bytes represent 8 bit pixel values.  They are swapped
 *          for little endians so that when the dest array (char *)datad
 *          is addressed by bytes, the pixels are chosen sequentially
 *          from left to right in the image.
 */
l_int32
lineEndianByteSwap(l_uint32  *datad,
                   l_uint32  *datas,
                   l_int32    wpl)
{
l_int32   j;
l_uint32  word;

    PROCNAME("lineEndianByteSwap");

    if (!datad || !datas)
        return ERROR_INT("datad and datas not both defined", procName, 1);

#ifdef L_BIG_ENDIAN

    memcpy((char *)datad, (char *)datas, 4 * wpl);
    return 0;

#else   /* L_LITTLE_ENDIAN */

    for (j = 0; j < wpl; j++, datas++, datad++) {
        word = *datas;
        *datad = (word >> 24) |
                 ((word >> 8) & 0x0000ff00) |
                 ((word << 8) & 0x00ff0000) |
                 (word << 24);
    }
    return 0;

#endif   /* L_BIG_ENDIAN */

}


/*!
 *  pixEndianTwoByteSwapNew()
 *
 *      Input:  pixs
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is used on little-endian platforms to swap the
 *          2-byte entities within a 32-bit word.
 *      (2) This is equivalent to a full byte swap, as performed
 *          by pixEndianByteSwap(), followed by byte swaps in
 *          each of the 16-bit entities separately.
 *      (3) Unlike pixEndianTwoByteSwap(), which swaps the shorts in-place,
 *          this returns a new pix (or a clone).  We provide this
 *          to avoid having to swap twice in situations where the input
 *          pix must be restored to canonical little-endian order.
 */
PIX *
pixEndianTwoByteSwapNew(PIX  *pixs)
{
l_uint32  *datas, *datad;
l_int32    i, j, h, wpl;
l_uint32   word;
PIX       *pixd;

    PROCNAME("pixEndianTwoByteSwapNew");

#ifdef L_BIG_ENDIAN

    return pixClone(pixs);

#else   /* L_LITTLE_ENDIAN */

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    datas = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    h = pixGetHeight(pixs);
    pixd = pixCreateTemplate(pixs);
    datad = pixGetData(pixd);
    for (i = 0; i < h; i++) {
        for (j = 0; j < wpl; j++, datas++, datad++) {
            word = *datas;
            *datad = (word << 16) | (word >> 16);
        }
    }

    return pixd;

#endif   /* L_BIG_ENDIAN */

}


/*!
 *  pixEndianTwoByteSwap()
 *
 *      Input:  pixs
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is used on little-endian platforms to swap the
 *          2-byte entities within a 32-bit word.
 *      (2) This is equivalent to a full byte swap, as performed
 *          by pixEndianByteSwap(), followed by byte swaps in
 *          each of the 16-bit entities separately.
 */
l_int32
pixEndianTwoByteSwap(PIX  *pixs)
{
l_uint32  *data;
l_int32    i, j, h, wpl;
l_uint32   word;

    PROCNAME("pixEndianTwoByteSwap");

#ifdef L_BIG_ENDIAN

    return 0;

#else   /* L_LITTLE_ENDIAN */

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    h = pixGetHeight(pixs);
    for (i = 0; i < h; i++) {
        for (j = 0; j < wpl; j++, data++) {
            word = *data;
            *data = (word << 16) | (word >> 16);
        }
    }

    return 0;

#endif   /* L_BIG_ENDIAN */

}


/*-------------------------------------------------------------*
 *             Extract raster data as binary string            *
 *-------------------------------------------------------------*/
/*!
 *  pixGetRasterData()
 *
 *      Input:  pixs (1, 8, 32 bpp)
 *              &data (<return> raster data in memory)
 *              &nbytes (<return> number of bytes in data string)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This returns the raster data as a byte string, padded to the
 *          byte.  For 1 bpp, the first pixel is the MSbit in the first byte.
 *          For rgb, the bytes are in (rgb) order.  This is the format
 *          required for flate encoding of pixels in a PostScript file.
 */
l_int32
pixGetRasterData(PIX       *pixs,
                 l_uint8  **pdata,
                 size_t    *pnbytes)
{
l_int32    w, h, d, wpl, i, j, rval, gval, bval;
l_int32    databpl;  /* bytes for each raster line in returned data */
l_uint8   *line, *data;  /* packed data in returned array */
l_uint32  *rline, *rdata;  /* data in pix raster */

    PROCNAME("pixGetRasterData");

    if (!pdata || !pnbytes)
        return ERROR_INT("&data and &nbytes not both defined", procName, 1);
    *pdata = NULL;
    *pnbytes = 0;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32)
        return ERROR_INT("depth not in {1,2,4,8,16,32}", procName, 1);
    rdata = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    if (d == 1)
        databpl = (w + 7) / 8;
    else if (d == 2)
        databpl = (w + 3) / 4;
    else if (d == 4)
        databpl = (w + 1) / 2;
    else if (d == 8 || d == 16)
        databpl = w * (d / 8);
    else  /* d == 32 bpp rgb */
        databpl = 3 * w;
    if ((data = (l_uint8 *)CALLOC(databpl * h, sizeof(l_uint8))) == NULL)
        return ERROR_INT("data not allocated", procName, 1);
    *pdata = data;
    *pnbytes = databpl * h;

    for (i = 0; i < h; i++) {
         rline = rdata + i * wpl;
         line = data + i * databpl;
         if (d <= 8) {
             for (j = 0; j < databpl; j++)
                  line[j] = GET_DATA_BYTE(rline, j);
         } else if (d == 16) {
             for (j = 0; j < w; j++)
                  line[2 * j] = GET_DATA_TWO_BYTES(rline, j);
         } else {  /* d == 32 bpp rgb */
             for (j = 0; j < w; j++) {
                  extractRGBValues(rline[j], &rval, &gval, &bval);
                  *(line + 3 * j) = rval;
                  *(line + 3 * j + 1) = gval;
                  *(line + 3 * j + 2) = bval;
             }
         }
    }

    return 0;
}


/*-------------------------------------------------------------*
 *                 Test alpha component opaqueness             *
 *-------------------------------------------------------------*/
/*!
 *  pixAlphaIsOpaque()
 *
 *      Input:  pix (32 bpp, spp == 4)
 *              &opaque (<return> 1 if spp == 4 and all alpha component
 *                       values are 255 (opaque); 0 otherwise)
 *      Return: 0 if OK, 1 on error
 *      Notes:
 *          (1) On error, opaque is returned as 0 (FALSE).
 */
l_int32
pixAlphaIsOpaque(PIX      *pix,
                 l_int32  *popaque)
{
l_int32    w, h, wpl, i, j, alpha;
l_uint32  *data, *line;

    PROCNAME("pixAlphaIsOpaque");

    if (!popaque)
        return ERROR_INT("&opaque not defined", procName, 1);
    *popaque = FALSE;
    if (!pix)
        return ERROR_INT("&pix not defined", procName, 1);
    if (pixGetDepth(pix) != 32)
        return ERROR_INT("&pix not 32 bpp", procName, 1);
    if (pixGetSpp(pix) != 4)
        return ERROR_INT("&pix not 4 spp", procName, 1);

    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    pixGetDimensions(pix, &w, &h, NULL);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < w; j++) {
            alpha = GET_DATA_BYTE(line + j, L_ALPHA_CHANNEL);
            if (alpha ^ 0xff)  /* not opaque */
                return 0;
        }
    }

    *popaque = TRUE;
    return 0;
}


/*-------------------------------------------------------------*
 *             Setup helpers for 8 bpp byte processing         *
 *-------------------------------------------------------------*/
/*!
 *  pixSetupByteProcessing()
 *
 *      Input:  pix (8 bpp, no colormap)
 *              &w (<optional return> width)
 *              &h (<optional return> height)
 *      Return: line ptr array, or null on error
 *
 *  Notes:
 *      (1) This is a simple helper for processing 8 bpp images with
 *          direct byte access.  It can swap byte order within each word.
 *      (2) After processing, you must call pixCleanupByteProcessing(),
 *          which frees the lineptr array and restores byte order.
 *      (3) Usage:
 *              l_uint8 **lineptrs = pixSetupByteProcessing(pix, &w, &h);
 *              for (i = 0; i < h; i++) {
 *                  l_uint8 *line = lineptrs[i];
 *                  for (j = 0; j < w; j++) {
 *                      val = line[j];
 *                      ...
 *                  }
 *              }
 *              pixCleanupByteProcessing(pix, lineptrs);
 */
l_uint8 **
pixSetupByteProcessing(PIX      *pix,
                       l_int32  *pw,
                       l_int32  *ph)
{
l_int32  w, h;

    PROCNAME("pixSetupByteProcessing");

    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (!pix || pixGetDepth(pix) != 8)
        return (l_uint8 **)ERROR_PTR("pix not defined or not 8 bpp",
                                     procName, NULL);
    if (pixGetColormap(pix))
        return (l_uint8 **)ERROR_PTR("pix has colormap", procName, NULL);

    pixGetDimensions(pix, &w, &h, NULL);
    if (pw) *pw = w;
    if (ph) *ph = h;
    pixEndianByteSwap(pix);
    return (l_uint8 **)pixGetLinePtrs(pix, NULL);
}


/*!
 *  pixCleanupByteProcessing()
 *
 *      Input:  pix (8 bpp, no colormap)
 *              lineptrs (ptrs to the beginning of each raster line of data)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This must be called after processing that was initiated
 *          by pixSetupByteProcessing() has finished.
 */
l_int32
pixCleanupByteProcessing(PIX      *pix,
                         l_uint8 **lineptrs)
{
    PROCNAME("pixCleanupByteProcessing");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!lineptrs)
        return ERROR_INT("lineptrs not defined", procName, 1);

    pixEndianByteSwap(pix);
    FREE(lineptrs);
    return 0;
}


/*------------------------------------------------------------------------*
 *      Setting parameters for antialias masking with alpha transforms    *
 *------------------------------------------------------------------------*/
/*!
 *  l_setAlphaMaskBorder()
 *
 *      Input:  val1, val2 (in [0.0 ... 1.0])
 *      Return: void
 *
 *  Notes:
 *      (1) This sets the opacity values used to generate the two outer
 *          boundary rings in the alpha mask associated with geometric
 *          transforms such as pixRotateWithAlpha().
 *      (2) The default values are val1 = 0.0 (completely transparent
 *          in the outermost ring) and val2 = 0.5 (half transparent
 *          in the second ring).  When the image is blended, this
 *          completely removes the outer ring (shrinking the image by
 *          2 in each direction), and alpha-blends with 0.5 the second ring.
 *          Using val1 = 0.25 and val2 = 0.75 gives a slightly more
 *          blurred border, with no perceptual difference at screen resolution.
 *      (3) The actual mask values are found by multiplying these
 *          normalized opacity values by 255.
 */
void
l_setAlphaMaskBorder(l_float32  val1,
                     l_float32  val2)
{
    val1 = L_MAX(0.0, L_MIN(1.0, val1));
    val2 = L_MAX(0.0, L_MIN(1.0, val2));
    AlphaMaskBorderVals[0] = val1;
    AlphaMaskBorderVals[1] = val2;
}
