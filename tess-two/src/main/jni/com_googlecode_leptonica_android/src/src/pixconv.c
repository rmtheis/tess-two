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
 *  pixconv.c
 *
 *      These functions convert between images of different types
 *      without scaling.
 *
 *      Conversion from 8 bpp grayscale to 1, 2, 4 and 8 bpp
 *           PIX        *pixThreshold8()
 *
 *      Conversion from colormap to full color or grayscale
 *           PIX        *pixRemoveColormapGeneral()
 *           PIX        *pixRemoveColormap()
 *
 *      Add colormap losslessly (8 to 8)
 *           l_int32     pixAddGrayColormap8()
 *           PIX        *pixAddMinimalGrayColormap8()
 *
 *      Conversion from RGB color to grayscale
 *           PIX        *pixConvertRGBToLuminance()
 *           PIX        *pixConvertRGBToGray()
 *           PIX        *pixConvertRGBToGrayFast()
 *           PIX        *pixConvertRGBToGrayMinMax()
 *           PIX        *pixConvertRGBToGraySatBoost()
 *
 *      Conversion from grayscale to colormap
 *           PIX        *pixConvertGrayToColormap()  -- 2, 4, 8 bpp
 *           PIX        *pixConvertGrayToColormap8()  -- 8 bpp only
 *
 *      Colorizing conversion from grayscale to color
 *           PIX        *pixColorizeGray()  -- 8 bpp or cmapped
 *
 *      Conversion from RGB color to colormap
 *           PIX        *pixConvertRGBToColormap()
 *
 *      Quantization for relatively small number of colors in source
 *           l_int32     pixQuantizeIfFewColors()
 *
 *      Conversion from 16 bpp to 8 bpp
 *           PIX        *pixConvert16To8()
 *
 *      Conversion from grayscale to false color
 *           PIX        *pixConvertGrayToFalseColor()
 *
 *      Unpacking conversion from 1 bpp to 2, 4, 8, 16 and 32 bpp
 *           PIX        *pixUnpackBinary()
 *           PIX        *pixConvert1To16()
 *           PIX        *pixConvert1To32()
 *
 *      Unpacking conversion from 1 bpp to 2 bpp
 *           PIX        *pixConvert1To2Cmap()
 *           PIX        *pixConvert1To2()
 *
 *      Unpacking conversion from 1 bpp to 4 bpp
 *           PIX        *pixConvert1To4Cmap()
 *           PIX        *pixConvert1To4()
 *
 *      Unpacking conversion from 1, 2 and 4 bpp to 8 bpp
 *           PIX        *pixConvert1To8()
 *           PIX        *pixConvert2To8()
 *           PIX        *pixConvert4To8()
 *
 *      Unpacking conversion from 8 bpp to 16 bpp
 *           PIX        *pixConvert8To16()
 *
 *      Top-level conversion to 1 bpp
 *           PIX        *pixConvertTo1()
 *           PIX        *pixConvertTo1BySampling()
 *
 *      Top-level conversion to 8 bpp
 *           PIX        *pixConvertTo8()
 *           PIX        *pixConvertTo8BySampling()
 *           PIX        *pixConvertTo8Color()
 *
 *      Top-level conversion to 16 bpp
 *           PIX        *pixConvertTo16()
 *
 *      Top-level conversion to 32 bpp (RGB)
 *           PIX        *pixConvertTo32()   ***
 *           PIX        *pixConvertTo32BySampling()   ***
 *           PIX        *pixConvert8To32()  ***
 *
 *      Top-level conversion to 8 or 32 bpp, without colormap
 *           PIX        *pixConvertTo8Or32
 *
 *      Conversion between 24 bpp and 32 bpp rgb
 *           PIX        *pixConvert24To32()
 *           PIX        *pixConvert32To24()
 *
 *      Removal of alpha component by blending with white background
 *           PIX        *pixRemoveAlpha()
 *
 *      Addition of alpha component to 1 bpp
 *           PIX        *pixAddAlphaTo1bpp()
 *
 *      Lossless depth conversion (unpacking)
 *           PIX        *pixConvertLossless()
 *
 *      Conversion for printing in PostScript
 *           PIX        *pixConvertForPSWrap()
 *
 *      Scaling conversion to subpixel RGB
 *           PIX        *pixConvertToSubpixelRGB()
 *           PIX        *pixConvertGrayToSubpixelRGB()
 *           PIX        *pixConvertColorToSubpixelRGB()
 *
 *      *** indicates implicit assumption about RGB component ordering
 */

#include <string.h>
#include <math.h>
#include "allheaders.h"

#ifndef  NO_CONSOLE_IO
#define DEBUG_CONVERT_TO_COLORMAP  0
#define DEBUG_UNROLLING 0
#endif   /* ~NO_CONSOLE_IO */


/*-------------------------------------------------------------*
 *     Conversion from 8 bpp grayscale to 1, 2 4 and 8 bpp     *
 *-------------------------------------------------------------*/
/*!
 *  pixThreshold8()
 *
 *      Input:  pix (8 bpp grayscale)
 *              d (destination depth: 1, 2, 4 or 8)
 *              nlevels (number of levels to be used for colormap)
 *              cmapflag (1 if makes colormap; 0 otherwise)
 *      Return: pixd (thresholded with standard dest thresholds),
 *              or null on error
 *
 *  Notes:
 *      (1) This uses, by default, equally spaced "target" values
 *          that depend on the number of levels, with thresholds
 *          halfway between.  For N levels, with separation (N-1)/255,
 *          there are N-1 fixed thresholds.
 *      (2) For 1 bpp destination, the number of levels can only be 2
 *          and if a cmap is made, black is (0,0,0) and white
 *          is (255,255,255), which is opposite to the convention
 *          without a colormap.
 *      (3) For 1, 2 and 4 bpp, the nlevels arg is used if a colormap
 *          is made; otherwise, we take the most significant bits
 *          from the src that will fit in the dest.
 *      (4) For 8 bpp, the input pixs is quantized to nlevels.  The
 *          dest quantized with that mapping, either through a colormap
 *          table or directly with 8 bit values.
 *      (5) Typically you should not use make a colormap for 1 bpp dest.
 *      (6) This is not dithering.  Each pixel is treated independently.
 */
PIX *
pixThreshold8(PIX     *pixs,
              l_int32  d,
              l_int32  nlevels,
              l_int32  cmapflag)
{
PIX       *pixd;
PIXCMAP   *cmap;

    PROCNAME("pixThreshold8");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (cmapflag && nlevels < 2)
        return (PIX *)ERROR_PTR("nlevels must be at least 2", procName, NULL);

    switch (d) {
    case 1:
        pixd = pixThresholdToBinary(pixs, 128);
        if (cmapflag) {
            cmap = pixcmapCreateLinear(1, 2);
            pixSetColormap(pixd, cmap);
        }
        break;
    case 2:
        pixd = pixThresholdTo2bpp(pixs, nlevels, cmapflag);
        break;
    case 4:
        pixd = pixThresholdTo4bpp(pixs, nlevels, cmapflag);
        break;
    case 8:
        pixd = pixThresholdOn8bpp(pixs, nlevels, cmapflag);
        break;
    default:
        return (PIX *)ERROR_PTR("d must be in {1,2,4,8}", procName, NULL);
    }

    if (!pixd)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    return pixd;
}


/*-------------------------------------------------------------*
 *               Conversion from colormapped pix               *
 *-------------------------------------------------------------*/
/*!
 *  pixRemoveColormapGeneral()
 *
 *      Input:  pixs (any depth, with or without colormap)
 *              type (REMOVE_CMAP_TO_BINARY,
 *                    REMOVE_CMAP_TO_GRAYSCALE,
 *                    REMOVE_CMAP_TO_FULL_COLOR,
 *                    REMOVE_CMAP_WITH_ALPHA,
 *                    REMOVE_CMAP_BASED_ON_SRC)
 *              ifnocmap (L_CLONE, L_COPY)
 *      Return: pixd (always a new pix; without colormap), or null on error
 *
 *  Notes:
 *      (1) Convenience function that allows choice between returning
 *          a clone or a copy if pixs does not have a colormap.
 *      (2) See pixRemoveColormap().
 */
PIX *
pixRemoveColormapGeneral(PIX     *pixs,
                         l_int32  type,
                         l_int32  ifnocmap)
{
    PROCNAME("pixRemoveColormapGeneral");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (ifnocmap != L_CLONE && ifnocmap != L_COPY)
        return (PIX *)ERROR_PTR("invalid value for ifnocmap", procName, NULL);

    if (pixGetColormap(pixs))
        return pixRemoveColormap(pixs, type);

    if (ifnocmap == L_CLONE)
        return pixClone(pixs);
    else
        return pixCopy(NULL, pixs);
}


/*!
 *  pixRemoveColormap()
 *
 *      Input:  pixs (see restrictions below)
 *              type (REMOVE_CMAP_TO_BINARY,
 *                    REMOVE_CMAP_TO_GRAYSCALE,
 *                    REMOVE_CMAP_TO_FULL_COLOR,
 *                    REMOVE_CMAP_WITH_ALPHA,
 *                    REMOVE_CMAP_BASED_ON_SRC)
 *      Return: pixd (without colormap), or null on error
 *
 *  Notes:
 *      (1) If pixs does not have a colormap, a clone is returned.
 *      (2) Otherwise, the input pixs is restricted to 1, 2, 4 or 8 bpp.
 *      (3) Use REMOVE_CMAP_TO_BINARY only on 1 bpp pix.
 *      (4) For grayscale conversion from RGB, use a weighted average
 *          of RGB values, and always return an 8 bpp pix, regardless
 *          of whether the input pixs depth is 2, 4 or 8 bpp.
 *      (5) REMOVE_CMAP_TO_FULL_COLOR ignores the alpha component and
 *          returns a 32 bpp pix with spp == 3 and the alpha bytes are 0.
 *      (6) For REMOVE_CMAP_BASED_ON_SRC, if there is no color, this
 *          returns either a 1 bpp or 8 bpp grayscale pix.
 *          If there is color, this returns a 32 bpp pix, with either:
 *           * 3 spp, if the alpha values are all 255 (opaque), or
 *           * 4 spp (preserving the alpha), if any alpha values are not 255.
 */
PIX *
pixRemoveColormap(PIX     *pixs,
                  l_int32  type)
{
l_int32    sval, rval, gval, bval;
l_int32    i, j, k, w, h, d, wpls, wpld, opaque, ncolors, count;
l_int32    colorfound;
l_int32   *rmap, *gmap, *bmap, *amap, *graymap;
l_uint32  *datas, *lines, *datad, *lined, *lut;
l_uint32   sword, dword;
PIXCMAP   *cmap;
PIX       *pixd;

    PROCNAME("pixRemoveColormap");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if ((cmap = pixGetColormap(pixs)) == NULL)
        return pixClone(pixs);

    if (type != REMOVE_CMAP_TO_BINARY &&
        type != REMOVE_CMAP_TO_GRAYSCALE &&
        type != REMOVE_CMAP_TO_FULL_COLOR &&
        type != REMOVE_CMAP_WITH_ALPHA &&
        type != REMOVE_CMAP_BASED_ON_SRC) {
        L_WARNING("Invalid type; converting based on src\n", procName);
        type = REMOVE_CMAP_BASED_ON_SRC;
    }

    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 2 && d != 4 && d != 8)
        return (PIX *)ERROR_PTR("pixs must be {1,2,4,8} bpp", procName, NULL);

    if (pixcmapToArrays(cmap, &rmap, &gmap, &bmap, &amap))
        return (PIX *)ERROR_PTR("colormap arrays not made", procName, NULL);

    if (d != 1 && type == REMOVE_CMAP_TO_BINARY) {
        L_WARNING("not 1 bpp; can't remove cmap to binary\n", procName);
        type = REMOVE_CMAP_BASED_ON_SRC;
    }

    if (type == REMOVE_CMAP_BASED_ON_SRC) {
            /* select output type depending on colormap */
        pixcmapIsOpaque(cmap, &opaque);
        pixcmapHasColor(cmap, &colorfound);
        if (!opaque) {  /* save the alpha */
            type = REMOVE_CMAP_WITH_ALPHA;
        } else if (colorfound) {
            type = REMOVE_CMAP_TO_FULL_COLOR;
        } else {  /* opaque and no color */
            if (d == 1)
                type = REMOVE_CMAP_TO_BINARY;
            else
                type = REMOVE_CMAP_TO_GRAYSCALE;
        }
    }

    ncolors = pixcmapGetCount(cmap);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if (type == REMOVE_CMAP_TO_BINARY) {
        if ((pixd = pixCopy(NULL, pixs)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
        pixcmapGetColor(cmap, 0, &rval, &gval, &bval);
        if (rval == 0)  /* photometrically inverted from standard */
            pixInvert(pixd, pixd);
        pixDestroyColormap(pixd);
    } else if (type == REMOVE_CMAP_TO_GRAYSCALE) {
        if ((pixd = pixCreate(w, h, 8)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
        pixCopyResolution(pixd, pixs);
        datad = pixGetData(pixd);
        wpld = pixGetWpl(pixd);
        if ((graymap = (l_int32 *)CALLOC(ncolors, sizeof(l_int32))) == NULL)
            return (PIX *)ERROR_PTR("calloc fail for graymap", procName, NULL);
        for (i = 0; i < pixcmapGetCount(cmap); i++) {
            graymap[i] = (rmap[i] + 2 * gmap[i] + bmap[i]) / 4;
        }
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            switch (d)   /* depth test above; no default permitted */
            {
                case 8:
                        /* Unrolled 4x */
                    for (j = 0, count = 0; j + 3 < w; j += 4, count++) {
                        sword = lines[count];
                        dword = (graymap[(sword >> 24) & 0xff] << 24) |
                            (graymap[(sword >> 16) & 0xff] << 16) |
                            (graymap[(sword >> 8) & 0xff] << 8) |
                            graymap[sword & 0xff];
                        lined[count] = dword;
                    }
                        /* Cleanup partial word */
                    for (; j < w; j++) {
                        sval = GET_DATA_BYTE(lines, j);
                        gval = graymap[sval];
                        SET_DATA_BYTE(lined, j, gval);
                    }
#if DEBUG_UNROLLING
#define CHECK_VALUE(a, b, c) if (GET_DATA_BYTE(a, b) != c) { \
    fprintf(stderr, "Error: mismatch at %d, %d vs %d\n", \
            j, GET_DATA_BYTE(a, b), c); }
                    for (j = 0; j < w; j++) {
                        sval = GET_DATA_BYTE(lines, j);
                        gval = graymap[sval];
                        CHECK_VALUE(lined, j, gval);
                    }
#endif
                    break;
                case 4:
                        /* Unrolled 8x */
                    for (j = 0, count = 0; j + 7 < w; j += 8, count++) {
                        sword = lines[count];
                        dword = (graymap[(sword >> 28) & 0xf] << 24) |
                            (graymap[(sword >> 24) & 0xf] << 16) |
                            (graymap[(sword >> 20) & 0xf] << 8) |
                            graymap[(sword >> 16) & 0xf];
                        lined[2 * count] = dword;
                        dword = (graymap[(sword >> 12) & 0xf] << 24) |
                            (graymap[(sword >> 8) & 0xf] << 16) |
                            (graymap[(sword >> 4) & 0xf] << 8) |
                            graymap[sword & 0xf];
                        lined[2 * count + 1] = dword;
                    }
                        /* Cleanup partial word */
                    for (; j < w; j++) {
                        sval = GET_DATA_QBIT(lines, j);
                        gval = graymap[sval];
                        SET_DATA_BYTE(lined, j, gval);
                    }
#if DEBUG_UNROLLING
                    for (j = 0; j < w; j++) {
                        sval = GET_DATA_QBIT(lines, j);
                        gval = graymap[sval];
                        CHECK_VALUE(lined, j, gval);
                    }
#endif
                    break;
                case 2:
                        /* Unrolled 16x */
                    for (j = 0, count = 0; j + 15 < w; j += 16, count++) {
                        sword = lines[count];
                        dword = (graymap[(sword >> 30) & 0x3] << 24) |
                            (graymap[(sword >> 28) & 0x3] << 16) |
                            (graymap[(sword >> 26) & 0x3] << 8) |
                            graymap[(sword >> 24) & 0x3];
                        lined[4 * count] = dword;
                        dword = (graymap[(sword >> 22) & 0x3] << 24) |
                            (graymap[(sword >> 20) & 0x3] << 16) |
                            (graymap[(sword >> 18) & 0x3] << 8) |
                            graymap[(sword >> 16) & 0x3];
                        lined[4 * count + 1] = dword;
                        dword = (graymap[(sword >> 14) & 0x3] << 24) |
                            (graymap[(sword >> 12) & 0x3] << 16) |
                            (graymap[(sword >> 10) & 0x3] << 8) |
                            graymap[(sword >> 8) & 0x3];
                        lined[4 * count + 2] = dword;
                        dword = (graymap[(sword >> 6) & 0x3] << 24) |
                            (graymap[(sword >> 4) & 0x3] << 16) |
                            (graymap[(sword >> 2) & 0x3] << 8) |
                            graymap[sword & 0x3];
                        lined[4 * count + 3] = dword;
                    }
                        /* Cleanup partial word */
                    for (; j < w; j++) {
                        sval = GET_DATA_DIBIT(lines, j);
                        gval = graymap[sval];
                        SET_DATA_BYTE(lined, j, gval);
                    }
#if DEBUG_UNROLLING
                    for (j = 0; j < w; j++) {
                        sval = GET_DATA_DIBIT(lines, j);
                        gval = graymap[sval];
                        CHECK_VALUE(lined, j, gval);
                    }
#endif
                    break;
                case 1:
                        /* Unrolled 8x */
                    for (j = 0, count = 0; j + 31 < w; j += 32, count++) {
                        sword = lines[count];
                        for (k = 0; k < 4; k++) {
                                /* The top byte is always the relevant one */
                            dword = (graymap[(sword >> 31) & 0x1] << 24) |
                                (graymap[(sword >> 30) & 0x1] << 16) |
                                (graymap[(sword >> 29) & 0x1] << 8) |
                                graymap[(sword >> 28) & 0x1];
                            lined[8 * count + 2 * k] = dword;
                            dword = (graymap[(sword >> 27) & 0x1] << 24) |
                                (graymap[(sword >> 26) & 0x1] << 16) |
                                (graymap[(sword >> 25) & 0x1] << 8) |
                                graymap[(sword >> 24) & 0x1];
                            lined[8 * count + 2 * k + 1] = dword;
                            sword <<= 8;  /* Move up the next byte */
                        }
                    }
                        /* Cleanup partial word */
                    for (; j < w; j++) {
                        sval = GET_DATA_BIT(lines, j);
                        gval = graymap[sval];
                        SET_DATA_BYTE(lined, j, gval);
                    }
#if DEBUG_UNROLLING
                    for (j = 0; j < w; j++) {
                        sval = GET_DATA_BIT(lines, j);
                        gval = graymap[sval];
                        CHECK_VALUE(lined, j, gval);
                    }
#undef CHECK_VALUE
#endif
                    break;
                default:
                    return NULL;
            }
        }
        if (graymap)
            FREE(graymap);
    } else {  /* type == REMOVE_CMAP_TO_FULL_COLOR or REMOVE_CMAP_WITH_ALPHA */
        if ((pixd = pixCreate(w, h, 32)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
        pixCopyResolution(pixd, pixs);
        if (type == REMOVE_CMAP_WITH_ALPHA)
            pixSetSpp(pixd, 4);
        datad = pixGetData(pixd);
        wpld = pixGetWpl(pixd);
        if ((lut = (l_uint32 *)CALLOC(ncolors, sizeof(l_uint32))) == NULL)
            return (PIX *)ERROR_PTR("calloc fail for lut", procName, NULL);
        for (i = 0; i < ncolors; i++) {
            if (type == REMOVE_CMAP_TO_FULL_COLOR)
                composeRGBPixel(rmap[i], gmap[i], bmap[i], lut + i);
            else  /* full color plus alpha */
                composeRGBAPixel(rmap[i], gmap[i], bmap[i], amap[i], lut + i);
        }

        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            for (j = 0; j < w; j++) {
                if (d == 8)
                    sval = GET_DATA_BYTE(lines, j);
                else if (d == 4)
                    sval = GET_DATA_QBIT(lines, j);
                else if (d == 2)
                    sval = GET_DATA_DIBIT(lines, j);
                else if (d == 1)
                    sval = GET_DATA_BIT(lines, j);
                else
                    return NULL;
                if (sval >= ncolors)
                    L_WARNING("pixel value out of bounds\n", procName);
                else
                    lined[j] = lut[sval];
            }
        }
        FREE(lut);
    }

    FREE(rmap);
    FREE(gmap);
    FREE(bmap);
    FREE(amap);
    return pixd;
}


/*-------------------------------------------------------------*
 *              Add colormap losslessly (8 to 8)               *
 *-------------------------------------------------------------*/
/*!
 *  pixAddGrayColormap8()
 *
 *      Input:  pixs (8 bpp)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If pixs has a colormap, this is a no-op.
 */
l_int32
pixAddGrayColormap8(PIX  *pixs)
{
PIXCMAP  *cmap;

    PROCNAME("pixAddGrayColormap8");

    if (!pixs || pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not defined or not 8 bpp", procName, 1);
    if (pixGetColormap(pixs))
        return 0;

    cmap = pixcmapCreateLinear(8, 256);
    pixSetColormap(pixs, cmap);
    return 0;
}


/*!
 *  pixAddMinimalGrayColormap8()
 *
 *      Input:  pixs (8 bpp)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This generates a colormapped version of the input image
 *          that has the same number of colormap entries as the
 *          input image has unique gray levels.
 */
PIX *
pixAddMinimalGrayColormap8(PIX  *pixs)
{
l_int32    ncolors, w, h, i, j, wplt, wpld, index, val;
l_int32   *inta, *revmap;
l_uint32  *datat, *datad, *linet, *lined;
PIX       *pixt, *pixd;
PIXCMAP   *cmap;

    PROCNAME("pixAddMinimalGrayColormap8");

    if (!pixs || pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);

        /* Eliminate the easy cases */
    pixNumColors(pixs, 1, &ncolors);
    cmap = pixGetColormap(pixs);
    if (cmap) {
        if (pixcmapGetCount(cmap) == ncolors)  /* irreducible */
            return pixCopy(NULL, pixs);
        else
            pixt = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
    } else {
        if (ncolors == 256) {
            pixt = pixCopy(NULL, pixs);
            pixAddGrayColormap8(pixt);
            return pixt;
        }
        pixt = pixClone(pixs);
    }

        /* Find the gray levels and make a reverse map */
    pixGetDimensions(pixt, &w, &h, NULL);
    datat = pixGetData(pixt);
    wplt = pixGetWpl(pixt);
    inta = (l_int32 *)CALLOC(256, sizeof(l_int32));
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        for (j = 0; j < w; j++) {
            val = GET_DATA_BYTE(linet, j);
            inta[val] = 1;
        }
    }
    cmap = pixcmapCreate(8);
    revmap = (l_int32 *)CALLOC(256, sizeof(l_int32));
    for (i = 0, index = 0; i < 256; i++) {
        if (inta[i]) {
            pixcmapAddColor(cmap, i, i, i);
            revmap[i] = index++;
        }
    }

        /* Set all pixels in pixd to the colormap index */
    pixd = pixCreateTemplate(pixt);
    pixSetColormap(pixd, cmap);
    pixCopyResolution(pixd, pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            val = GET_DATA_BYTE(linet, j);
            SET_DATA_BYTE(lined, j, revmap[val]);
        }
    }

    pixDestroy(&pixt);
    FREE(inta);
    FREE(revmap);
    return pixd;
}


/*-------------------------------------------------------------*
 *            Conversion from RGB color to grayscale           *
 *-------------------------------------------------------------*/
/*!
 *  pixConvertRGBToLuminance()
 *
 *      Input:  pix (32 bpp RGB)
 *      Return: 8 bpp pix, or null on error
 *
 *  Notes:
 *      (1) Use a standard luminance conversion.
 */
PIX *
pixConvertRGBToLuminance(PIX *pixs)
{
  return pixConvertRGBToGray(pixs, 0.0, 0.0, 0.0);
}


/*!
 *  pixConvertRGBToGray()
 *
 *      Input:  pix (32 bpp RGB)
 *              rwt, gwt, bwt  (non-negative; these should add to 1.0,
 *                              or use 0.0 for default)
 *      Return: 8 bpp pix, or null on error
 *
 *  Notes:
 *      (1) Use a weighted average of the RGB values.
 */
PIX *
pixConvertRGBToGray(PIX       *pixs,
                    l_float32  rwt,
                    l_float32  gwt,
                    l_float32  bwt)
{
l_int32    i, j, w, h, wpls, wpld, val;
l_uint32   word;
l_uint32  *datas, *lines, *datad, *lined;
l_float32  sum;
PIX       *pixd;

    PROCNAME("pixConvertRGBToGray");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs not 32 bpp", procName, NULL);
    if (rwt < 0.0 || gwt < 0.0 || bwt < 0.0)
        return (PIX *)ERROR_PTR("weights not all >= 0.0", procName, NULL);

        /* Make sure the sum of weights is 1.0; otherwise, you can get
         * overflow in the gray value. */
    if (rwt == 0.0 && gwt == 0.0 && bwt == 0.0) {
        rwt = L_RED_WEIGHT;
        gwt = L_GREEN_WEIGHT;
        bwt = L_BLUE_WEIGHT;
    }
    sum = rwt + gwt + bwt;
    if (L_ABS(sum - 1.0) > 0.0001) {  /* maintain ratios with sum == 1.0 */
        L_WARNING("weights don't sum to 1; maintaining ratios\n", procName);
        rwt = rwt / sum;
        gwt = gwt / sum;
        bwt = bwt / sum;
    }

    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            word = *(lines + j);
            val = (l_int32)(rwt * ((word >> L_RED_SHIFT) & 0xff) +
                            gwt * ((word >> L_GREEN_SHIFT) & 0xff) +
                            bwt * ((word >> L_BLUE_SHIFT) & 0xff) + 0.5);
            SET_DATA_BYTE(lined, j, val);
        }
    }

    return pixd;
}


/*!
 *  pixConvertRGBToGrayFast()
 *
 *      Input:  pix (32 bpp RGB)
 *      Return: 8 bpp pix, or null on error
 *
 *  Notes:
 *      (1) This function should be used if speed of conversion
 *          is paramount, and the green channel can be used as
 *          a fair representative of the RGB intensity.  It is
 *          several times faster than pixConvertRGBToGray().
 *      (2) To combine RGB to gray conversion with subsampling,
 *          use pixScaleRGBToGrayFast() instead.
 */
PIX *
pixConvertRGBToGrayFast(PIX  *pixs)
{
l_int32    i, j, w, h, wpls, wpld, val;
l_uint32  *datas, *lines, *datad, *lined;
PIX       *pixd;

    PROCNAME("pixConvertRGBToGrayFast");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs not 32 bpp", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++, lines++) {
            val = ((*lines) >> L_GREEN_SHIFT) & 0xff;
            SET_DATA_BYTE(lined, j, val);
        }
    }

    return pixd;
}


/*!
 *  pixConvertRGBToGrayMinMax()
 *
 *      Input:  pix (32 bpp RGB)
 *              type (L_CHOOSE_MIN, L_CHOOSE_MAX or L_CHOOSE_MAX_MIN_DIFF)
 *      Return: 8 bpp pix, or null on error
 *
 *  Notes:
 *      (1) This chooses the min, the max, or the difference between
 *          the max and the min, of the three RGB sample values.
 */
PIX *
pixConvertRGBToGrayMinMax(PIX     *pixs,
                          l_int32  type)
{
l_int32    i, j, w, h, wpls, wpld, rval, gval, bval, val, minval, maxval;
l_uint32  *datas, *lines, *datad, *lined;
PIX       *pixd;

    PROCNAME("pixConvertRGBToGrayMinMax");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs not 32 bpp", procName, NULL);
    if (type != L_CHOOSE_MIN && type != L_CHOOSE_MAX &&
        type != L_CHOOSE_MAX_MIN_DIFF)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            extractRGBValues(lines[j], &rval, &gval, &bval);
            if (type == L_CHOOSE_MIN) {
                val = L_MIN(rval, gval);
                val = L_MIN(val, bval);
            } else if (type == L_CHOOSE_MAX) {
                val = L_MAX(rval, gval);
                val = L_MAX(val, bval);
            } else {  /* L_CHOOSE_MAX_MIN_DIFF */
                minval = L_MIN(rval, gval);
                minval = L_MIN(minval, bval);
                maxval = L_MAX(rval, gval);
                maxval = L_MAX(maxval, bval);
                val = maxval - minval;
            }
            SET_DATA_BYTE(lined, j, val);
        }
    }

    return pixd;
}


/*!
 *  pixConvertRGBToGraySatBoost()
 *
 *      Input:  pixs (32 bpp rgb)
 *              refval (between 1 and 255; typ. less than 128)
 *      Return: pixd (8 bpp), or null on error
 *
 *  Notes:
 *      (1) This returns the max component value, boosted by
 *          the saturation. The maximum boost occurs where
 *          the maximum component value is equal to some reference value.
 *          This particular weighting is due to Dany Qumsiyeh.
 *      (2) For gray pixels (zero saturation), this returns
 *          the intensity of any component.
 *      (3) For fully saturated pixels ('fullsat'), this rises linearly
 *          with the max value and has a slope equal to 255 divided
 *          by the reference value; for a max value greater than
 *          the reference value, it is clipped to 255.
 *      (4) For saturation values in between, the output is a linear
 *          combination of (2) and (3), weighted by saturation.
 *          It falls between these two curves, and does not exceed 255.
 *      (5) This can be useful for distinguishing an object that has nonzero
 *          saturation from a gray background.  For this, the refval
 *          should be chosen near the expected value of the background,
 *          to achieve maximum saturation boost there.
 */
PIX  *
pixConvertRGBToGraySatBoost(PIX     *pixs,
                            l_int32  refval)
{
l_int32     w, h, d, i, j, wplt, wpld;
l_int32     rval, gval, bval, sval, minrg, maxrg, min, max, delta;
l_int32     fullsat, newval;
l_float32  *invmax, *ratio;
l_uint32   *linet, *lined, *datat, *datad;
PIX        *pixt, *pixd;

    PROCNAME("pixConvertRGBToGraySatBoost");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 32 && !pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs not cmapped or rgb", procName, NULL);
    if (refval < 1 || refval > 255)
        return (PIX *)ERROR_PTR("refval not in [1 ... 255]", procName, NULL);

    pixt = pixRemoveColormap(pixs, REMOVE_CMAP_TO_FULL_COLOR);
    pixd = pixCreate(w, h, 8);
    pixCopyResolution(pixd, pixs);
    wplt = pixGetWpl(pixt);
    datat = pixGetData(pixt);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);
    invmax = (l_float32 *)CALLOC(256, sizeof(l_float32));
    ratio = (l_float32 *)CALLOC(256, sizeof(l_float32));
    for (i = 1; i < 256; i++) {  /* i == 0  --> delta = sval = newval = 0 */
        invmax[i] = 1.0 / (l_float32)i;
        ratio[i] = (l_float32)i / (l_float32)refval;
    }
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            extractRGBValues(linet[j], &rval, &gval, &bval);
            minrg = L_MIN(rval, gval);
            min = L_MIN(minrg, bval);
            maxrg = L_MAX(rval, gval);
            max = L_MAX(maxrg, bval);
            delta = max - min;
            if (delta == 0)  /* gray; no chroma */
                sval = 0;
            else
                sval = (l_int32)(255. * (l_float32)delta * invmax[max] + 0.5);

            fullsat = L_MIN(255, 255 * ratio[max]);
            newval = (sval * fullsat + (255 - sval) * max) / 255;
            SET_DATA_BYTE(lined, j, newval);
        }
    }

    pixDestroy(&pixt);
    FREE(invmax);
    FREE(ratio);
    return pixd;
}


/*---------------------------------------------------------------------------*
 *                  Conversion from grayscale to colormap                    *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvertGrayToColormap()
 *
 *      Input:  pixs (2, 4 or 8 bpp grayscale)
 *      Return: pixd (2, 4 or 8 bpp with colormap), or null on error
 *
 *  Notes:
 *      (1) This is a simple interface for adding a colormap to a
 *          2, 4 or 8 bpp grayscale image without causing any
 *          quantization.  There is some similarity to operations
 *          in grayquant.c, such as pixThresholdOn8bpp(), where
 *          the emphasis is on quantization with an arbitrary number
 *          of levels, and a colormap is an option.
 *      (2) Returns a copy if pixs already has a colormap.
 *      (3) For 8 bpp src, this is a lossless transformation.
 *      (4) For 2 and 4 bpp src, this generates a colormap that
 *          assumes full coverage of the gray space, with equally spaced
 *          levels: 4 levels for d = 2 and 16 levels for d = 4.
 *      (5) In all cases, the depth of the dest is the same as the src.
 */
PIX *
pixConvertGrayToColormap(PIX  *pixs)
{
l_int32    d;
PIX       *pixd;
PIXCMAP   *cmap;

    PROCNAME("pixConvertGrayToColormap");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    if (d != 2 && d != 4 && d != 8)
        return (PIX *)ERROR_PTR("pixs not 2, 4 or 8 bpp", procName, NULL);

    if (pixGetColormap(pixs)) {
        L_WARNING("pixs already has a colormap\n", procName);
        return pixCopy(NULL, pixs);
    }

    if (d == 8)  /* lossless conversion */
        return pixConvertGrayToColormap8(pixs, 2);

        /* Build a cmap with equally spaced target values over the
         * full 8 bpp range. */
    pixd = pixCopy(NULL, pixs);
    cmap = pixcmapCreateLinear(d, 1 << d);
    pixSetColormap(pixd, cmap);
    return pixd;
}


/*!
 *  pixConvertGrayToColormap8()
 *
 *      Input:  pixs (8 bpp grayscale)
 *              mindepth (of pixd; valid values are 2, 4 and 8)
 *      Return: pixd (2, 4 or 8 bpp with colormap), or null on error
 *
 *  Notes:
 *      (1) Returns a copy if pixs already has a colormap.
 *      (2) This is a lossless transformation; there is no quantization.
 *          We compute the number of different gray values in pixs,
 *          and construct a colormap that has exactly these values.
 *      (3) 'mindepth' is the minimum depth of pixd.  If mindepth == 8,
 *          pixd will always be 8 bpp.  Let the number of different
 *          gray values in pixs be ngray.  If mindepth == 4, we attempt
 *          to save pixd as a 4 bpp image, but if ngray > 16,
 *          pixd must be 8 bpp.  Likewise, if mindepth == 2,
 *          the depth of pixd will be 2 if ngray <= 4 and 4 if ngray > 4
 *          but <= 16.
 */
PIX *
pixConvertGrayToColormap8(PIX     *pixs,
                          l_int32  mindepth)
{
l_int32    ncolors, w, h, depth, i, j, wpls, wpld;
l_int32    index, num, val, newval;
l_int32    array[256];
l_uint32  *lines, *lined, *datas, *datad;
NUMA      *na;
PIX       *pixd;
PIXCMAP   *cmap;

    PROCNAME("pixConvertGrayToColormap8");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (mindepth != 2 && mindepth != 4 && mindepth != 8) {
        L_WARNING("invalid value of mindepth; setting to 8\n", procName);
        mindepth = 8;
    }

    if (pixGetColormap(pixs)) {
        L_WARNING("pixs already has a colormap\n", procName);
        return pixCopy(NULL, pixs);
    }

    na = pixGetGrayHistogram(pixs, 1);
    numaGetCountRelativeToZero(na, L_GREATER_THAN_ZERO, &ncolors);
    if (mindepth == 8 || ncolors > 16)
        depth = 8;
    else if (mindepth == 4 || ncolors > 4)
        depth = 4;
    else
        depth = 2;

    pixGetDimensions(pixs, &w, &h, NULL);
    pixd = pixCreate(w, h, depth);
    cmap = pixcmapCreate(depth);
    pixSetColormap(pixd, cmap);
    pixCopyResolution(pixd, pixs);

    index = 0;
    for (i = 0; i < 256; i++) {
        numaGetIValue(na, i, &num);
        if (num > 0) {
            pixcmapAddColor(cmap, i, i, i);
            array[i] = index;
            index++;
        }
    }

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            val = GET_DATA_BYTE(lines, j);
            newval = array[val];
            if (depth == 2)
                SET_DATA_DIBIT(lined, j, newval);
            else if (depth == 4)
                SET_DATA_QBIT(lined, j, newval);
            else  /* depth == 8 */
                SET_DATA_BYTE(lined, j, newval);
        }
    }

    numaDestroy(&na);
    return pixd;
}


/*---------------------------------------------------------------------------*
 *                Colorizing conversion from grayscale to color              *
 *---------------------------------------------------------------------------*/
/*!
 *  pixColorizeGray()
 *
 *      Input:  pixs (8 bpp gray; 2, 4 or 8 bpp colormapped)
 *              color (32 bit rgba pixel)
 *              cmapflag (1 for result to have colormap; 0 for RGB)
 *      Return: pixd (8 bpp colormapped or 32 bpp rgb), or null on error
 *
 *  Notes:
 *      (1) This applies the specific color to the grayscale image.
 *      (2) If pixs already has a colormap, it is removed to gray
 *          before colorizing.
 */
PIX *
pixColorizeGray(PIX      *pixs,
                l_uint32  color,
                l_int32   cmapflag)
{
l_int32    i, j, w, h, wplt, wpld, val8;
l_uint32  *datad, *datat, *lined, *linet, *tab;
PIX       *pixt, *pixd;
PIXCMAP   *cmap;

    PROCNAME("pixColorizeGray");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8 && !pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs not 8 bpp or cmapped", procName, NULL);

    if (pixGetColormap(pixs))
        pixt = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
    else
        pixt = pixClone(pixs);

    cmap = pixcmapGrayToColor(color);
    if (cmapflag) {
        pixd = pixCopy(NULL, pixt);
        pixSetColormap(pixd, cmap);
        pixDestroy(&pixt);
        return pixd;
    }

        /* Make an RGB pix */
    pixcmapToRGBTable(cmap, &tab, NULL);
    pixGetDimensions(pixt, &w, &h, NULL);
    pixd = pixCreate(w, h, 32);
    pixCopyResolution(pixd, pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    datat = pixGetData(pixt);
    wplt = pixGetWpl(pixt);
    for (i = 0; i < h; i++) {
        lined = datad + i * wpld;
        linet = datat + i * wplt;
        for (j = 0; j < w; j++) {
            val8 = GET_DATA_BYTE(linet, j);
            lined[j] = tab[val8];
        }
    }

    pixDestroy(&pixt);
    pixcmapDestroy(&cmap);
    FREE(tab);
    return pixd;
}


/*---------------------------------------------------------------------------*
 *                    Conversion from RGB color to colormap                  *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvertRGBToColormap()
 *
 *      Input:  pixs (32 bpp rgb)
 *              ditherflag (1 to dither, 0 otherwise)
 *      Return: pixd (2, 4 or 8 bpp with colormap), or null on error
 *
 *  Notes:
 *      (1) This function has two relatively simple modes of color
 *          quantization:
 *            (a) If the image is made orthographically and has not more
 *                than 256 'colors' at the level 4 octcube leaves,
 *                it is quantized nearly exactly.  The ditherflag
 *                is ignored.
 *            (b) Most natural images have more than 256 different colors;
 *                in that case we use adaptive octree quantization,
 *                with dithering if requested.
 *      (2) If there are not more than 256 occupied level 4 octcubes,
 *          the color in the colormap that represents all pixels in
 *          one of those octcubes is given by the first pixel that
 *          falls into that octcube.
 *      (3) If there are more than 256 colors, we use adaptive octree
 *          color quantization.
 *      (4) Dithering gives better visual results on images where
 *          there is a color wash (a slow variation of color), but it
 *          is about twice as slow and results in significantly larger
 *          files when losslessly compressed (e.g., into png).
 */
PIX *
pixConvertRGBToColormap(PIX     *pixs,
                        l_int32  ditherflag)
{
l_int32  ncolors;
NUMA    *na;
PIX     *pixd;

    PROCNAME("pixConvertRGBToColormap");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs not 32 bpp", procName, NULL);
    if (pixGetSpp(pixs) == 4)
        L_WARNING("pixs has alpha; removing\n", procName);

        /* Get the histogram and count the number of occupied level 4
         * leaf octcubes.  We don't yet know if this is the number of
         * actual colors, but if it's not, all pixels falling into
         * the same leaf octcube will be assigned to the color of the
         * first pixel that lands there. */
    na = pixOctcubeHistogram(pixs, 4, &ncolors);

        /* If there are too many occupied leaf octcubes to be
         * represented directly in a colormap, fall back to octree
         * quantization, optionally with dithering. */
    if (ncolors > 256) {
        numaDestroy(&na);
        if (ditherflag)
            L_INFO("More than 256 colors; using octree quant with dithering\n",
                   procName);
        else
            L_INFO("More than 256 colors; using octree quant; no dithering\n",
                   procName);
        return pixOctreeColorQuant(pixs, 240, ditherflag);
    }

        /* There are not more than 256 occupied leaf octcubes.
         * Quantize to those octcubes. */
    pixd = pixFewColorsOctcubeQuant2(pixs, 4, na, ncolors, NULL);
    numaDestroy(&na);
    return pixd;
}


/*---------------------------------------------------------------------------*
 *        Quantization for relatively small number of colors in source       *
 *---------------------------------------------------------------------------*/
/*!
 *  pixQuantizeIfFewColors()
 *
 *      Input:  pixs (8 bpp gray or 32 bpp rgb)
 *              maxcolors (max number of colors allowed to be returned
 *                         from pixColorsForQuantization(); use 0 for default)
 *              mingraycolors (min number of gray levels that a grayscale
 *                             image is quantized to; use 0 for default)
 *              octlevel (for octcube quantization: 3 or 4)
 *              &pixd (2, 4 or 8 bpp quantized; null if too many colors)
 *      Return: 0 if OK, 1 on error or if pixs can't be quantized into
 *              a small number of colors.
 *
 *  Notes:
 *      (1) This is a wrapper that tests if the pix can be quantized
 *          with good quality using a small number of colors.  If so,
 *          it does the quantization, defining a colormap and using
 *          pixels whose value is an index into the colormap.
 *      (2) If the image has color, it is quantized with 8 bpp pixels.
 *          If the image is essentially grayscale, the pixels are
 *          either 4 or 8 bpp, depending on the size of the required
 *          colormap.
 *      (3) @octlevel = 4 generates a larger colormap and larger
 *          compressed image than @octlevel = 3.  If image quality is
 *          important, you should use @octlevel = 4.
 *      (4) If the image already has a colormap, it returns a clone.
 */
l_int32
pixQuantizeIfFewColors(PIX     *pixs,
                       l_int32  maxcolors,
                       l_int32  mingraycolors,
                       l_int32  octlevel,
                       PIX    **ppixd)
{
l_int32  d, ncolors, iscolor, graycolors;
PIX     *pixg, *pixd;

    PROCNAME("pixQuantizeIfFewColors");

    if (!ppixd)
        return ERROR_INT("&pixd not defined", procName, 1);
    *ppixd = NULL;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    d = pixGetDepth(pixs);
    if (d != 8 && d != 32)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetColormap(pixs) != NULL) {
        *ppixd = pixClone(pixs);
        return 0;
    }
    if (maxcolors <= 0)
        maxcolors = 15;  /* default */
    if (maxcolors > 50)
        L_WARNING("maxcolors > 50; very large!\n", procName);
    if (mingraycolors <= 0)
        mingraycolors = 10;  /* default */
    if (mingraycolors > 30)
        L_WARNING("mingraycolors > 30; very large!\n", procName);
    if (octlevel != 3 && octlevel != 4) {
        L_WARNING("invalid octlevel; setting to 3\n", procName);
        octlevel = 3;
    }

        /* Test the number of colors.  For color, the octcube leaves
         * are at level 4. */
    pixColorsForQuantization(pixs, 0, &ncolors, &iscolor, 0);
    if (ncolors > maxcolors)
        return ERROR_INT("too many colors", procName, 1);

        /* Quantize!
         *  (1) For color:
         *      If octlevel == 4, try to quantize to an octree where
         *      the octcube leaves are at level 4. If that fails,
         *      back off to level 3.
         *      If octlevel == 3, quantize to level 3 directly.
         *      For level 3, the quality is usually good enough and there
         *      is negligible chance of getting more than 256 colors.
         *  (2) For grayscale, multiply ncolors by 1.5 for extra quality,
         *      but use at least mingraycolors and not more than 256. */
    if (iscolor) {
        pixd = pixFewColorsOctcubeQuant1(pixs, octlevel);
        if (!pixd) {  /* backoff */
            pixd = pixFewColorsOctcubeQuant1(pixs, octlevel - 1);
            if (octlevel == 3)  /* shouldn't happen */
                L_WARNING("quantized at level 2; low quality\n", procName);
        }
    } else { /* image is really grayscale */
        if (d == 32)
            pixg = pixConvertRGBToLuminance(pixs);
        else
            pixg = pixClone(pixs);
        graycolors = L_MAX(mingraycolors, (l_int32)(1.5 * ncolors));
        graycolors = L_MIN(graycolors, 256);
        if (graycolors < 16)
            pixd = pixThresholdTo4bpp(pixg, graycolors, 1);
        else
            pixd = pixThresholdOn8bpp(pixg, graycolors, 1);
        pixDestroy(&pixg);
    }
    *ppixd = pixd;

    if (!pixd)
        return ERROR_INT("pixd not made", procName, 1);
    else
        return 0;
}



/*---------------------------------------------------------------------------*
 *                    Conversion from 16 bpp to 8 bpp                        *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvert16To8()
 *
 *      Input:  pixs (16 bpp)
 *              type (L_LS_BYTE, L_MS_BYTE, L_CLIP_TO_255)
 *      Return: pixd (8 bpp), or null on error
 *
 *  Notes:
 *      (1) For each dest pixel, use either the LSB, the MSB, or the
 *          min(val, 255) for each 16-bit src pixel.
 */
PIX *
pixConvert16To8(PIX     *pixs,
                l_int32  type)
{
l_uint16   dsword;
l_int32    w, h, wpls, wpld, i, j;
l_uint32   sword, first, second;
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixConvert16To8");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 16)
        return (PIX *)ERROR_PTR("pixs not 16 bpp", procName, NULL);
    if (type != L_LS_BYTE && type != L_MS_BYTE && type != L_CLIP_TO_255)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    wpls = pixGetWpl(pixs);
    datas = pixGetData(pixs);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);

        /* Convert 2 pixels at a time */
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        if (type == L_LS_BYTE) {
            for (j = 0; j < wpls; j++) {
                sword = *(lines + j);
                dsword = ((sword >> 8) & 0xff00) | (sword & 0xff);
                SET_DATA_TWO_BYTES(lined, j, dsword);
            }
        } else if (type == L_MS_BYTE) {
            for (j = 0; j < wpls; j++) {
                sword = *(lines + j);
                dsword = ((sword >> 16) & 0xff00) | ((sword >> 8) & 0xff);
                SET_DATA_TWO_BYTES(lined, j, dsword);
            }
        } else {  /* type == L_CLIP_TO_255 */
            for (j = 0; j < wpls; j++) {
                sword = *(lines + j);
                first = (sword >> 24) ? 255 : ((sword >> 16) & 0xff);
                second = ((sword >> 8) & 0xff) ? 255 : (sword & 0xff);
                dsword = (first << 8) | second;
                SET_DATA_TWO_BYTES(lined, j, dsword);
            }
        }
    }

    return pixd;
}



/*---------------------------------------------------------------------------*
 *                Conversion from grayscale to false color
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvertGrayToFalseColor()
 *
 *      Input:  pixs (8 or 16 bpp grayscale)
 *              gamma factor (0.0 or 1.0 for default; > 1.0 for brighter;
 *                            2.0 is quite nice)
 *      Return: pixd (8 bpp with colormap), or null on error
 *
 *  Notes:
 *      (1) For 8 bpp input, this simply adds a colormap to the input image.
 *      (2) For 16 bpp input, it first converts to 8 bpp, using the MSB,
 *          and then adds the colormap.
 *      (3) The colormap is modeled after the Matlab "jet" configuration.
 */
PIX *
pixConvertGrayToFalseColor(PIX       *pixs,
                           l_float32  gamma)
{
l_int32    d, i, rval, bval, gval;
l_int32   *curve;
l_float32  invgamma, x;
PIX       *pixd;
PIXCMAP   *cmap;

    PROCNAME("pixConvertGrayToFalseColor");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    if (d != 8 && d != 16)
        return (PIX *)ERROR_PTR("pixs not 8 or 16 bpp", procName, NULL);

    if (d == 16) {
        pixd = pixConvert16To8(pixs, L_MS_BYTE);
    } else {  /* d == 8 */
        if (pixGetColormap(pixs))
            pixd = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
        else
            pixd = pixCopy(NULL, pixs);
    }
    if (!pixd)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    if ((cmap = pixcmapCreate(8)) == NULL)
        return (PIX *)ERROR_PTR("cmap not made", procName, NULL);
    pixSetColormap(pixd, cmap);
    pixCopyResolution(pixd, pixs);

        /* Generate curve for transition part of color map */
    if ((curve = (l_int32 *)CALLOC(64, sizeof(l_int32)))== NULL)
        return (PIX *)ERROR_PTR("curve not made", procName, NULL);
    if (gamma == 0.0) gamma = 1.0;
    invgamma = 1. / gamma;
    for (i = 0; i < 64; i++) {
        x = (l_float32)i / 64.;
        curve[i] = (l_int32)(255. * powf(x, invgamma) + 0.5);
    }

    for (i = 0; i < 256; i++) {
        if (i < 32) {
            rval = 0;
            gval = 0;
            bval = curve[i + 32];
        } else if (i < 96) {   /* 32 - 95 */
            rval = 0;
            gval = curve[i - 32];
            bval = 255;
        } else if (i < 160) {  /* 96 - 159 */
            rval = curve[i - 96];
            gval = 255;
            bval = curve[159 - i];
        } else if (i < 224) {  /* 160 - 223 */
            rval = 255;
            gval = curve[223 - i];
            bval = 0;
        } else {  /* 224 - 255 */
            rval = curve[287 - i];
            gval = 0;
            bval = 0;
        }
        pixcmapAddColor(cmap, rval, gval, bval);
    }

    FREE(curve);
    return pixd;
}


/*---------------------------------------------------------------------------*
 *         Unpacking conversion from 1 bpp to 2, 4, 8, 16 and 32 bpp         *
 *---------------------------------------------------------------------------*/
/*!
 *  pixUnpackBinary()
 *
 *      Input:  pixs (1 bpp)
 *              depth (of destination: 2, 4, 8, 16 or 32 bpp)
 *              invert (0:  binary 0 --> grayscale 0
 *                          binary 1 --> grayscale 0xff...
 *                      1:  binary 0 --> grayscale 0xff...
 *                          binary 1 --> grayscale 0)
 *      Return: pixd (2, 4, 8, 16 or 32 bpp), or null on error
 *
 *  Notes:
 *      (1) This function calls special cases of pixConvert1To*(),
 *          for 2, 4, 8, 16 and 32 bpp destinations.
 */
PIX *
pixUnpackBinary(PIX     *pixs,
                l_int32  depth,
                l_int32  invert)
{
PIX  *pixd;

    PROCNAME("pixUnpackBinary");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);
    if (depth != 2 && depth != 4 && depth != 8 && depth != 16 && depth != 32)
        return (PIX *)ERROR_PTR("depth not 2, 4, 8, 16 or 32 bpp",
                                procName, NULL);

    if (depth == 2) {
        if (invert == 0)
            pixd = pixConvert1To2(NULL, pixs, 0, 3);
        else  /* invert bits */
            pixd = pixConvert1To2(NULL, pixs, 3, 0);
    } else if (depth == 4) {
        if (invert == 0)
            pixd = pixConvert1To4(NULL, pixs, 0, 15);
        else  /* invert bits */
            pixd = pixConvert1To4(NULL, pixs, 15, 0);
    } else if (depth == 8) {
        if (invert == 0)
            pixd = pixConvert1To8(NULL, pixs, 0, 255);
        else  /* invert bits */
            pixd = pixConvert1To8(NULL, pixs, 255, 0);
    } else if (depth == 16) {
        if (invert == 0)
            pixd = pixConvert1To16(NULL, pixs, 0, 0xffff);
        else  /* invert bits */
            pixd = pixConvert1To16(NULL, pixs, 0xffff, 0);
    } else {
        if (invert == 0)
            pixd = pixConvert1To32(NULL, pixs, 0, 0xffffffff);
        else  /* invert bits */
            pixd = pixConvert1To32(NULL, pixs, 0xffffffff, 0);
    }

    return pixd;
}


/*!
 *  pixConvert1To16()
 *
 *      Input:  pixd (<optional> 16 bpp, can be null)
 *              pixs (1 bpp)
 *              val0 (16 bit value to be used for 0s in pixs)
 *              val1 (16 bit value to be used for 1s in pixs)
 *      Return: pixd (16 bpp)
 *
 *  Notes:
 *      (1) If pixd is null, a new pix is made.
 *      (2) If pixd is not null, it must be of equal width and height
 *          as pixs.  It is always returned.
 */
PIX *
pixConvert1To16(PIX      *pixd,
                PIX      *pixs,
                l_uint16  val0,
                l_uint16  val1)
{
l_int32    w, h, i, j, dibit, ndibits, wpls, wpld;
l_uint16   val[2];
l_uint32   index;
l_uint32  *tab, *datas, *datad, *lines, *lined;

    PROCNAME("pixConvert1To16");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);

    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);
    if (pixd) {
        if (w != pixGetWidth(pixd) || h != pixGetHeight(pixd))
            return (PIX *)ERROR_PTR("pix sizes unequal", procName, pixd);
        if (pixGetDepth(pixd) != 16)
            return (PIX *)ERROR_PTR("pixd not 16 bpp", procName, pixd);
    } else {
        if ((pixd = pixCreate(w, h, 16)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    pixCopyResolution(pixd, pixs);

        /* Use a table to convert 2 src bits at a time */
    if ((tab = (l_uint32 *)CALLOC(4, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("tab not made", procName, NULL);
    val[0] = val0;
    val[1] = val1;
    for (index = 0; index < 4; index++) {
        tab[index] = (val[(index >> 1) & 1] << 16) | val[index & 1];
    }

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    ndibits = (w + 1) / 2;
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < ndibits; j++) {
            dibit = GET_DATA_DIBIT(lines, j);
            lined[j] = tab[dibit];
        }
    }

    FREE(tab);
    return pixd;
}


/*!
 *  pixConvert1To32()
 *
 *      Input:  pixd (<optional> 32 bpp, can be null)
 *              pixs (1 bpp)
 *              val0 (32 bit value to be used for 0s in pixs)
 *              val1 (32 bit value to be used for 1s in pixs)
 *      Return: pixd (32 bpp)
 *
 *  Notes:
 *      (1) If pixd is null, a new pix is made.
 *      (2) If pixd is not null, it must be of equal width and height
 *          as pixs.  It is always returned.
 */
PIX *
pixConvert1To32(PIX      *pixd,
                PIX      *pixs,
                l_uint32  val0,
                l_uint32  val1)
{
l_int32    w, h, i, j, wpls, wpld, bit;
l_uint32   val[2];
l_uint32  *datas, *datad, *lines, *lined;

    PROCNAME("pixConvert1To32");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    if (pixd) {
        if (w != pixGetWidth(pixd) || h != pixGetHeight(pixd))
            return (PIX *)ERROR_PTR("pix sizes unequal", procName, pixd);
        if (pixGetDepth(pixd) != 32)
            return (PIX *)ERROR_PTR("pixd not 32 bpp", procName, pixd);
    } else {
        if ((pixd = pixCreate(w, h, 32)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    pixCopyResolution(pixd, pixs);

    val[0] = val0;
    val[1] = val1;
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j <w; j++) {
            bit = GET_DATA_BIT(lines, j);
            lined[j] = val[bit];
        }
    }

    return pixd;
}


/*---------------------------------------------------------------------------*
 *                    Conversion from 1 bpp to 2 bpp                         *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvert1To2Cmap()
 *
 *      Input:  pixs (1 bpp)
 *      Return: pixd (2 bpp, cmapped)
 *
 *  Notes:
 *      (1) Input 0 is mapped to (255, 255, 255); 1 is mapped to (0, 0, 0)
 */
PIX *
pixConvert1To2Cmap(PIX  *pixs)
{
PIX      *pixd;
PIXCMAP  *cmap;

    PROCNAME("pixConvert1To2Cmap");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);

    if ((pixd = pixConvert1To2(NULL, pixs, 0, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    cmap = pixcmapCreate(2);
    pixcmapAddColor(cmap, 255, 255, 255);
    pixcmapAddColor(cmap, 0, 0, 0);
    pixSetColormap(pixd, cmap);

    return pixd;
}


/*!
 *  pixConvert1To2()
 *
 *      Input:  pixd (<optional> 2 bpp, can be null)
 *              pixs (1 bpp)
 *              val0 (2 bit value to be used for 0s in pixs)
 *              val1 (2 bit value to be used for 1s in pixs)
 *      Return: pixd (2 bpp)
 *
 *  Notes:
 *      (1) If pixd is null, a new pix is made.
 *      (2) If pixd is not null, it must be of equal width and height
 *          as pixs.  It is always returned.
 *      (3) A simple unpacking might use val0 = 0 and val1 = 3.
 *      (4) If you want a colormapped pixd, use pixConvert1To2Cmap().
 */
PIX *
pixConvert1To2(PIX     *pixd,
               PIX     *pixs,
               l_int32  val0,
               l_int32  val1)
{
l_int32    w, h, i, j, byteval, nbytes, wpls, wpld;
l_uint8    val[2];
l_uint32   index;
l_uint16  *tab;
l_uint32  *datas, *datad, *lines, *lined;

    PROCNAME("pixConvert1To2");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);

    pixGetDimensions(pixs, &w, &h, NULL);
    if (pixd) {
        if (w != pixGetWidth(pixd) || h != pixGetHeight(pixd))
            return (PIX *)ERROR_PTR("pix sizes unequal", procName, pixd);
        if (pixGetDepth(pixd) != 2)
            return (PIX *)ERROR_PTR("pixd not 2 bpp", procName, pixd);
    } else {
        if ((pixd = pixCreate(w, h, 2)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    pixCopyResolution(pixd, pixs);

        /* Use a table to convert 8 src bits to 16 dest bits */
    if ((tab = (l_uint16 *)CALLOC(256, sizeof(l_uint16))) == NULL)
        return (PIX *)ERROR_PTR("tab not made", procName, NULL);
    val[0] = val0;
    val[1] = val1;
    for (index = 0; index < 256; index++) {
        tab[index] = (val[(index >> 7) & 1] << 14) |
                     (val[(index >> 6) & 1] << 12) |
                     (val[(index >> 5) & 1] << 10) |
                     (val[(index >> 4) & 1] << 8) |
                     (val[(index >> 3) & 1] << 6) |
                     (val[(index >> 2) & 1] << 4) |
                     (val[(index >> 1) & 1] << 2) | val[index & 1];
    }

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    nbytes = (w + 7) / 8;
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < nbytes; j++) {
            byteval = GET_DATA_BYTE(lines, j);
            SET_DATA_TWO_BYTES(lined, j, tab[byteval]);
        }
    }

    FREE(tab);
    return pixd;
}


/*---------------------------------------------------------------------------*
 *                    Conversion from 1 bpp to 4 bpp                         *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvert1To4Cmap()
 *
 *      Input:  pixs (1 bpp)
 *      Return: pixd (4 bpp, cmapped)
 *
 *  Notes:
 *      (1) Input 0 is mapped to (255, 255, 255); 1 is mapped to (0, 0, 0)
 */
PIX *
pixConvert1To4Cmap(PIX  *pixs)
{
PIX      *pixd;
PIXCMAP  *cmap;

    PROCNAME("pixConvert1To4Cmap");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);

    if ((pixd = pixConvert1To4(NULL, pixs, 0, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    cmap = pixcmapCreate(4);
    pixcmapAddColor(cmap, 255, 255, 255);
    pixcmapAddColor(cmap, 0, 0, 0);
    pixSetColormap(pixd, cmap);

    return pixd;
}


/*!
 *  pixConvert1To4()
 *
 *      Input:  pixd (<optional> 4 bpp, can be null)
 *              pixs (1 bpp)
 *              val0 (4 bit value to be used for 0s in pixs)
 *              val1 (4 bit value to be used for 1s in pixs)
 *      Return: pixd (4 bpp)
 *
 *  Notes:
 *      (1) If pixd is null, a new pix is made.
 *      (2) If pixd is not null, it must be of equal width and height
 *          as pixs.  It is always returned.
 *      (3) A simple unpacking might use val0 = 0 and val1 = 15, or v.v.
 *      (4) If you want a colormapped pixd, use pixConvert1To4Cmap().
 */
PIX *
pixConvert1To4(PIX     *pixd,
               PIX     *pixs,
               l_int32  val0,
               l_int32  val1)
{
l_int32    w, h, i, j, byteval, nbytes, wpls, wpld;
l_uint8    val[2];
l_uint32   index;
l_uint32  *tab, *datas, *datad, *lines, *lined;

    PROCNAME("pixConvert1To4");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);

    pixGetDimensions(pixs, &w, &h, NULL);
    if (pixd) {
        if (w != pixGetWidth(pixd) || h != pixGetHeight(pixd))
            return (PIX *)ERROR_PTR("pix sizes unequal", procName, pixd);
        if (pixGetDepth(pixd) != 4)
            return (PIX *)ERROR_PTR("pixd not 4 bpp", procName, pixd);
    } else {
        if ((pixd = pixCreate(w, h, 4)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    pixCopyResolution(pixd, pixs);

        /* Use a table to convert 8 src bits to 32 bit dest word */
    if ((tab = (l_uint32 *)CALLOC(256, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("tab not made", procName, NULL);
    val[0] = val0;
    val[1] = val1;
    for (index = 0; index < 256; index++) {
        tab[index] = (val[(index >> 7) & 1] << 28) |
                     (val[(index >> 6) & 1] << 24) |
                     (val[(index >> 5) & 1] << 20) |
                     (val[(index >> 4) & 1] << 16) |
                     (val[(index >> 3) & 1] << 12) |
                     (val[(index >> 2) & 1] << 8) |
                     (val[(index >> 1) & 1] << 4) | val[index & 1];
    }

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    nbytes = (w + 7) / 8;
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < nbytes; j++) {
            byteval = GET_DATA_BYTE(lines, j);
            lined[j] = tab[byteval];
        }
    }

    FREE(tab);
    return pixd;
}


/*---------------------------------------------------------------------------*
 *               Conversion from 1, 2 and 4 bpp to 8 bpp                     *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvert1To8()
 *
 *      Input:  pixd (<optional> 8 bpp, can be null)
 *              pixs (1 bpp)
 *              val0 (8 bit value to be used for 0s in pixs)
 *              val1 (8 bit value to be used for 1s in pixs)
 *      Return: pixd (8 bpp)
 *
 *  Notes:
 *      (1) If pixd is null, a new pix is made.
 *      (2) If pixd is not null, it must be of equal width and height
 *          as pixs.  It is always returned.
 *      (3) A simple unpacking might use val0 = 0 and val1 = 255, or v.v.
 *      (4) In a typical application where one wants to use a colormap
 *          with the dest, you can use val0 = 0, val1 = 1 to make a
 *          non-cmapped 8 bpp pix, and then make a colormap and set 0
 *          and 1 to the desired colors.  Here is an example:
 *             pixd = pixConvert1To8(NULL, pixs, 0, 1);
 *             cmap = pixCreate(8);
 *             pixcmapAddColor(cmap, 255, 255, 255);
 *             pixcmapAddColor(cmap, 0, 0, 0);
 *             pixSetColormap(pixd, cmap);
 */
PIX *
pixConvert1To8(PIX     *pixd,
               PIX     *pixs,
               l_uint8  val0,
               l_uint8  val1)
{
l_int32    w, h, i, j, qbit, nqbits, wpls, wpld;
l_uint8    val[2];
l_uint32   index;
l_uint32  *tab, *datas, *datad, *lines, *lined;

    PROCNAME("pixConvert1To8");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, pixd);

    pixGetDimensions(pixs, &w, &h, NULL);
    if (pixd) {
        if (w != pixGetWidth(pixd) || h != pixGetHeight(pixd))
            return (PIX *)ERROR_PTR("pix sizes unequal", procName, pixd);
        if (pixGetDepth(pixd) != 8)
            return (PIX *)ERROR_PTR("pixd not 8 bpp", procName, pixd);
    } else {
        if ((pixd = pixCreate(w, h, 8)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    pixCopyResolution(pixd, pixs);

        /* Use a table to convert 4 src bits at a time */
    if ((tab = (l_uint32 *)CALLOC(16, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("tab not made", procName, NULL);
    val[0] = val0;
    val[1] = val1;
    for (index = 0; index < 16; index++) {
        tab[index] = (val[(index >> 3) & 1] << 24) |
                     (val[(index >> 2) & 1] << 16) |
                     (val[(index >> 1) & 1] << 8) | val[index & 1];
    }

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    nqbits = (w + 3) / 4;
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < nqbits; j++) {
            qbit = GET_DATA_QBIT(lines, j);
            lined[j] = tab[qbit];
        }
    }

    FREE(tab);
    return pixd;
}


/*!
 *  pixConvert2To8()
 *
 *      Input:  pixs (2 bpp)
 *              val0 (8 bit value to be used for 00 in pixs)
 *              val1 (8 bit value to be used for 01 in pixs)
 *              val2 (8 bit value to be used for 10 in pixs)
 *              val3 (8 bit value to be used for 11 in pixs)
 *              cmapflag (TRUE if pixd is to have a colormap; FALSE otherwise)
 *      Return: pixd (8 bpp), or null on error
 *
 *  Notes:
 *      - A simple unpacking might use val0 = 0,
 *        val1 = 85 (0x55), val2 = 170 (0xaa), val3 = 255.
 *      - If cmapflag is TRUE:
 *          - The 8 bpp image is made with a colormap.
 *          - If pixs has a colormap, the input values are ignored and
 *            the 8 bpp image is made using the colormap
 *          - If pixs does not have a colormap, the input values are
 *            used to build the colormap.
 *      - If cmapflag is FALSE:
 *          - The 8 bpp image is made without a colormap.
 *          - If pixs has a colormap, the input values are ignored,
 *            the colormap is removed, and the values stored in the 8 bpp
 *            image are from the colormap.
 *          - If pixs does not have a colormap, the input values are
 *            used to populate the 8 bpp image.
 */
PIX *
pixConvert2To8(PIX     *pixs,
               l_uint8  val0,
               l_uint8  val1,
               l_uint8  val2,
               l_uint8  val3,
               l_int32  cmapflag)
{
l_int32    w, h, i, j, nbytes, wpls, wpld, dibit, ncolor;
l_int32    rval, gval, bval, byte;
l_uint8    val[4];
l_uint32   index;
l_uint32  *tab, *datas, *datad, *lines, *lined;
PIX       *pixd;
PIXCMAP   *cmaps, *cmapd;

    PROCNAME("pixConvert2To8");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 2)
        return (PIX *)ERROR_PTR("pixs not 2 bpp", procName, NULL);

    cmaps = pixGetColormap(pixs);
    if (cmaps && cmapflag == FALSE)
        return pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);

    pixGetDimensions(pixs, &w, &h, NULL);
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    if (cmapflag == TRUE) {  /* pixd will have a colormap */
        cmapd = pixcmapCreate(8);  /* 8 bpp standard cmap */
        if (cmaps) {  /* use the existing colormap from pixs */
            ncolor = pixcmapGetCount(cmaps);
            for (i = 0; i < ncolor; i++) {
                pixcmapGetColor(cmaps, i, &rval, &gval, &bval);
                pixcmapAddColor(cmapd, rval, gval, bval);
            }
        } else {  /* make a colormap from the input values */
            pixcmapAddColor(cmapd, val0, val0, val0);
            pixcmapAddColor(cmapd, val1, val1, val1);
            pixcmapAddColor(cmapd, val2, val2, val2);
            pixcmapAddColor(cmapd, val3, val3, val3);
        }
        pixSetColormap(pixd, cmapd);
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            for (j = 0; j < w; j++) {
                dibit = GET_DATA_DIBIT(lines, j);
                SET_DATA_BYTE(lined, j, dibit);
            }
        }
        return pixd;
    }

        /* Last case: no colormap in either pixs or pixd.
         * Use input values and build a table to convert 1 src byte
         * (4 src pixels) at a time */
    if ((tab = (l_uint32 *)CALLOC(256, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("tab not made", procName, NULL);
    val[0] = val0;
    val[1] = val1;
    val[2] = val2;
    val[3] = val3;
    for (index = 0; index < 256; index++) {
        tab[index] = (val[(index >> 6) & 3] << 24) |
                     (val[(index >> 4) & 3] << 16) |
                     (val[(index >> 2) & 3] << 8) | val[index & 3];
    }

    nbytes = (w + 3) / 4;
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < nbytes; j++) {
            byte = GET_DATA_BYTE(lines, j);
            lined[j] = tab[byte];
        }
    }

    FREE(tab);
    return pixd;
}


/*!
 *  pixConvert4To8()
 *
 *      Input:  pixs (4 bpp)
 *              cmapflag (TRUE if pixd is to have a colormap; FALSE otherwise)
 *      Return: pixd (8 bpp), or null on error
 *
 *  Notes:
 *      - If cmapflag is TRUE:
 *          - pixd is made with a colormap.
 *          - If pixs has a colormap, it is copied and the colormap
 *            index values are placed in pixd.
 *          - If pixs does not have a colormap, a colormap with linear
 *            trc is built and the pixel values in pixs are placed in
 *            pixd as colormap index values.
 *      - If cmapflag is FALSE:
 *          - pixd is made without a colormap.
 *          - If pixs has a colormap, it is removed and the values stored
 *            in pixd are from the colormap (converted to gray).
 *          - If pixs does not have a colormap, the pixel values in pixs
 *            are used, with shift replication, to populate pixd.
 */
PIX *
pixConvert4To8(PIX     *pixs,
               l_int32  cmapflag)
{
l_int32    w, h, i, j, wpls, wpld, ncolor;
l_int32    rval, gval, bval, byte, qbit;
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;
PIXCMAP   *cmaps, *cmapd;

    PROCNAME("pixConvert4To8");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 4)
        return (PIX *)ERROR_PTR("pixs not 4 bpp", procName, NULL);

    cmaps = pixGetColormap(pixs);
    if (cmaps && cmapflag == FALSE)
        return pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);

    pixGetDimensions(pixs, &w, &h, NULL);
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    if (cmapflag == TRUE) {  /* pixd will have a colormap */
        cmapd = pixcmapCreate(8);
        if (cmaps) {  /* use the existing colormap from pixs */
            ncolor = pixcmapGetCount(cmaps);
            for (i = 0; i < ncolor; i++) {
                pixcmapGetColor(cmaps, i, &rval, &gval, &bval);
                pixcmapAddColor(cmapd, rval, gval, bval);
            }
        } else {  /* make a colormap with a linear trc */
            for (i = 0; i < 16; i++)
                pixcmapAddColor(cmapd, 17 * i, 17 * i, 17 * i);
        }
        pixSetColormap(pixd, cmapd);
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            for (j = 0; j < w; j++) {
                qbit = GET_DATA_QBIT(lines, j);
                SET_DATA_BYTE(lined, j, qbit);
            }
        }
        return pixd;
    }

        /* Last case: no colormap in either pixs or pixd.
         * Replicate the qbit value into 8 bits. */
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            qbit = GET_DATA_QBIT(lines, j);
            byte = (qbit << 4) | qbit;
            SET_DATA_BYTE(lined, j, byte);
        }
    }
    return pixd;
}



/*---------------------------------------------------------------------------*
 *               Unpacking conversion from 8 bpp to 16 bpp                   *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvert8To16()
 *
 *      Input:  pixs (8 bpp; colormap removed to gray)
 *              leftshift (number of bits: 0 is no shift;
 *                         8 replicates in MSB and LSB of dest)
 *      Return: pixd (16 bpp), or null on error
 *
 *  Notes:
 *      (1) For left shift of 8, the 8 bit value is replicated in both
 *          the MSB and the LSB of the pixels in pixd.  That way, we get
 *          proportional mapping, with a correct map from 8 bpp white
 *          (0xff) to 16 bpp white (0xffff).
 */
PIX *
pixConvert8To16(PIX     *pixs,
                l_int32  leftshift)
{
l_int32    i, j, w, h, d, wplt, wpld, val;
l_uint32  *datat, *datad, *linet, *lined;
PIX       *pixt, *pixd;

    PROCNAME("pixConvert8To16");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (leftshift < 0 || leftshift > 8)
        return (PIX *)ERROR_PTR("leftshift not in [0 ... 8]", procName, NULL);

    if (pixGetColormap(pixs) != NULL)
        pixt = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
    else
        pixt = pixClone(pixs);

    pixd = pixCreate(w, h, 16);
    pixCopyResolution(pixd, pixs);
    datat = pixGetData(pixt);
    datad = pixGetData(pixd);
    wplt = pixGetWpl(pixt);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            val = GET_DATA_BYTE(linet, j);
            if (leftshift == 8)
                val = val | (val << leftshift);
            else
                val <<= leftshift;
            SET_DATA_TWO_BYTES(lined, j, val);
        }
    }

    pixDestroy(&pixt);
    return pixd;
}



/*---------------------------------------------------------------------------*
 *                     Top-level conversion to 1 bpp                         *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvertTo1()
 *
 *      Input:  pixs (1, 2, 4, 8, 16 or 32 bpp)
 *              threshold (for final binarization, relative to 8 bpp)
 *      Return: pixd (1 bpp), or null on error
 *
 *  Notes:
 *      (1) This is a top-level function, with simple default values
 *          used in pixConvertTo8() if unpacking is necessary.
 *      (2) Any existing colormap is removed.
 *      (3) If the input image has 1 bpp and no colormap, the operation is
 *          lossless and a copy is returned.
 */
PIX *
pixConvertTo1(PIX     *pixs,
              l_int32  threshold)
{
l_int32   d, color0, color1, rval, gval, bval;
PIX      *pixg, *pixd;
PIXCMAP  *cmap;

    PROCNAME("pixConvertTo1");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("depth not {1,2,4,8,16,32}", procName, NULL);

    cmap = pixGetColormap(pixs);
    if (d == 1) {
        if (!cmap) {
            return pixCopy(NULL, pixs);
        } else {  /* strip the colormap off, and invert if reasonable
                   for standard binary photometry.  */
            pixcmapGetColor(cmap, 0, &rval, &gval, &bval);
            color0 = rval + gval + bval;
            pixcmapGetColor(cmap, 1, &rval, &gval, &bval);
            color1 = rval + gval + bval;
            pixd = pixCopy(NULL, pixs);
            pixDestroyColormap(pixd);
            if (color1 > color0)
                pixInvert(pixd, pixd);
            return pixd;
        }
    }

        /* For all other depths, use 8 bpp as an intermediary */
    pixg = pixConvertTo8(pixs, FALSE);
    pixd = pixThresholdToBinary(pixg, threshold);
    pixDestroy(&pixg);
    return pixd;
}


/*!
 *  pixConvertTo1BySampling()
 *
 *      Input:  pixs (1, 2, 4, 8, 16 or 32 bpp)
 *              factor (submsampling factor; integer >= 1)
 *              threshold (for final binarization, relative to 8 bpp)
 *      Return: pixd (1 bpp), or null on error
 *
 *  Notes:
 *      (1) This is a fast, quick/dirty, top-level converter.
 *      (2) See pixConvertTo1() for default values.
 */
PIX *
pixConvertTo1BySampling(PIX     *pixs,
                        l_int32  factor,
                        l_int32  threshold)
{
l_float32  scalefactor;
PIX       *pixt, *pixd;

    PROCNAME("pixConvertTo1BySampling");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (factor < 1)
        return (PIX *)ERROR_PTR("factor must be >= 1", procName, NULL);

    scalefactor = 1. / (l_float32)factor;
    pixt = pixScaleBySampling(pixs, scalefactor, scalefactor);
    pixd = pixConvertTo1(pixt, threshold);

    pixDestroy(&pixt);
    return pixd;
}


/*---------------------------------------------------------------------------*
 *                     Top-level conversion to 8 bpp                         *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvertTo8()
 *
 *      Input:  pixs (1, 2, 4, 8, 16 or 32 bpp)
 *              cmapflag (TRUE if pixd is to have a colormap; FALSE otherwise)
 *      Return: pixd (8 bpp), or null on error
 *
 *  Notes:
 *      (1) This is a top-level function, with simple default values
 *          for unpacking.
 *      (2) The result, pixd, is made with a colormap if specified.
 *          It is always a new image -- never a clone.  For example,
 *          if d == 8, and cmapflag matches the existence of a cmap
 *          in pixs, the operation is lossless and it returns a copy.
 *      (3) The default values used are:
 *          - 1 bpp: val0 = 255, val1 = 0
 *          - 2 bpp: 4 bpp:  even increments over dynamic range
 *          - 8 bpp: lossless if cmap matches cmapflag
 *          - 16 bpp: use most significant byte
 *      (4) If 32 bpp RGB, this is converted to gray.  If you want
 *          to do color quantization, you must specify the type
 *          explicitly, using the color quantization code.
 */
PIX *
pixConvertTo8(PIX     *pixs,
              l_int32  cmapflag)
{
l_int32   d;
PIX      *pixd;
PIXCMAP  *cmap;

    PROCNAME("pixConvertTo8");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("depth not {1,2,4,8,16,32}", procName, NULL);

    if (d == 1) {
        if (!cmapflag) {
            return pixConvert1To8(NULL, pixs, 255, 0);
        } else {
            pixd = pixConvert1To8(NULL, pixs, 0, 1);
            cmap = pixcmapCreate(8);
            pixcmapAddColor(cmap, 255, 255, 255);
            pixcmapAddColor(cmap, 0, 0, 0);
            pixSetColormap(pixd, cmap);
            return pixd;
        }
    } else if (d == 2) {
        return pixConvert2To8(pixs, 0, 85, 170, 255, cmapflag);
    } else if (d == 4) {
        return pixConvert4To8(pixs, cmapflag);
    } else if (d == 8) {
        cmap = pixGetColormap(pixs);
        if ((cmap && cmapflag) || (!cmap && !cmapflag)) {
            return pixCopy(NULL, pixs);
        } else if (cmap) {  /* !cmapflag */
            return pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
        } else {  /* !cmap && cmapflag; add colormap to pixd */
            pixd = pixCopy(NULL, pixs);
            pixAddGrayColormap8(pixd);
            return pixd;
        }
    } else if (d == 16) {
        pixd = pixConvert16To8(pixs, L_MS_BYTE);
        if (cmapflag)
            pixAddGrayColormap8(pixd);
        return pixd;
    } else { /* d == 32 */
        pixd = pixConvertRGBToLuminance(pixs);
        if (cmapflag)
            pixAddGrayColormap8(pixd);
        return pixd;
    }
}


/*!
 *  pixConvertTo8BySampling()
 *
 *      Input:  pixs (1, 2, 4, 8, 16 or 32 bpp)
 *              factor (submsampling factor; integer >= 1)
 *              cmapflag (TRUE if pixd is to have a colormap; FALSE otherwise)
 *      Return: pixd (8 bpp), or null on error
 *
 *  Notes:
 *      (1) This is a fast, quick/dirty, top-level converter.
 *      (2) See pixConvertTo8() for default values.
 */
PIX *
pixConvertTo8BySampling(PIX     *pixs,
                        l_int32  factor,
                        l_int32  cmapflag)
{
l_float32  scalefactor;
PIX       *pixt, *pixd;

    PROCNAME("pixConvertTo8BySampling");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (factor < 1)
        return (PIX *)ERROR_PTR("factor must be >= 1", procName, NULL);

    scalefactor = 1. / (l_float32)factor;
    pixt = pixScaleBySampling(pixs, scalefactor, scalefactor);
    pixd = pixConvertTo8(pixt, cmapflag);

    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixConvertTo8Color()
 *
 *      Input:  pixs (1, 2, 4, 8, 16 or 32 bpp)
 *              dither (1 to dither if necessary; 0 otherwise)
 *      Return: pixd (8 bpp, cmapped), or null on error
 *
 *  Notes:
 *      (1) This is a top-level function, with simple default values
 *          for unpacking.
 *      (2) The result, pixd, is always made with a colormap.
 *      (3) If d == 8, the operation is lossless and it returns a copy.
 *      (4) The default values used for increasing depth are:
 *          - 1 bpp: val0 = 255, val1 = 0
 *          - 2 bpp: 4 bpp:  even increments over dynamic range
 *      (5) For 16 bpp, use the most significant byte.
 *      (6) For 32 bpp RGB, use octcube quantization with optional dithering.
 */
PIX *
pixConvertTo8Color(PIX     *pixs,
                   l_int32  dither)
{
l_int32  d;

    PROCNAME("pixConvertTo8Color");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("depth not {1,2,4,8,16,32}", procName, NULL);

    if (d != 32)
        return pixConvertTo8(pixs, 1);

    return pixConvertRGBToColormap(pixs, dither);
}


/*---------------------------------------------------------------------------*
 *                    Top-level conversion to 16 bpp                         *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvertTo16()
 *
 *      Input:  pixs (1, 8 bpp)
 *      Return: pixd (16 bpp), or null on error
 *
 *  Usage: Top-level function, with simple default values for unpacking.
 *      1 bpp:  val0 = 0xffff, val1 = 0
 *      8 bpp:  replicates the 8 bit value in both the MSB and LSB
 *              of the 16 bit pixel.
 */
PIX *
pixConvertTo16(PIX  *pixs)
{
l_int32  d;

    PROCNAME("pixConvertTo16");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    d = pixGetDepth(pixs);
    if (d == 1)
        return pixConvert1To16(NULL, pixs, 0xffff, 0);
    else if (d == 8)
        return pixConvert8To16(pixs, 8);
    else
        return (PIX *)ERROR_PTR("src depth not 1 or 8 bpp", procName, NULL);
}



/*---------------------------------------------------------------------------*
 *                    Top-level conversion to 32 bpp                         *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvertTo32()
 *
 *      Input:  pixs (1, 2, 4, 8, 16 or 32 bpp)
 *      Return: pixd (32 bpp), or null on error
 *
 *  Usage: Top-level function, with simple default values for unpacking.
 *      1 bpp:  val0 = 255, val1 = 0
 *              and then replication into R, G and B components
 *      2 bpp:  if colormapped, use the colormap values; otherwise,
 *              use val0 = 0, val1 = 0x55, val2 = 0xaa, val3 = 255
 *              and replicate gray into R, G and B components
 *      4 bpp:  if colormapped, use the colormap values; otherwise,
 *              replicate 2 nybs into a byte, and then into R,G,B components
 *      8 bpp:  if colormapped, use the colormap values; otherwise,
 *              replicate gray values into R, G and B components
 *      16 bpp: replicate MSB into R, G and B components
 *      24 bpp: unpack the pixels, maintaining word alignment on each scanline
 *      32 bpp: makes a copy
 *
 *  Notes:
 *      (1) Never returns a clone of pixs.
 *      (2) Implicit assumption about RGB component ordering.
 */
PIX *
pixConvertTo32(PIX  *pixs)
{
l_int32  d;
PIX     *pixt, *pixd;

    PROCNAME("pixConvertTo32");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    d = pixGetDepth(pixs);
    if (d == 1) {
        return pixConvert1To32(NULL, pixs, 0xffffffff, 0);
    } else if (d == 2) {
        pixt = pixConvert2To8(pixs, 0, 85, 170, 255, TRUE);
        pixd = pixConvert8To32(pixt);
        pixDestroy(&pixt);
        return pixd;
    } else if (d == 4) {
        pixt = pixConvert4To8(pixs, TRUE);
        pixd = pixConvert8To32(pixt);
        pixDestroy(&pixt);
        return pixd;
    } else if (d == 8) {
        return pixConvert8To32(pixs);
    } else if (d == 16) {
        pixt = pixConvert16To8(pixs, L_MS_BYTE);
        pixd = pixConvert8To32(pixt);
        pixDestroy(&pixt);
        return pixd;
    } else if (d == 24) {
        return pixConvert24To32(pixs);
    } else if (d == 32) {
        return pixCopy(NULL, pixs);
    } else {
        return (PIX *)ERROR_PTR("depth not 1, 2, 4, 8, 16, 32 bpp",
                                procName, NULL);
    }
}


/*!
 *  pixConvertTo32BySampling()
 *
 *      Input:  pixs (1, 2, 4, 8, 16 or 32 bpp)
 *              factor (submsampling factor; integer >= 1)
 *      Return: pixd (32 bpp), or null on error
 *
 *  Notes:
 *      (1) This is a fast, quick/dirty, top-level converter.
 *      (2) See pixConvertTo32() for default values.
 */
PIX *
pixConvertTo32BySampling(PIX     *pixs,
                         l_int32  factor)
{
l_float32  scalefactor;
PIX       *pixt, *pixd;

    PROCNAME("pixConvertTo32BySampling");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (factor < 1)
        return (PIX *)ERROR_PTR("factor must be >= 1", procName, NULL);

    scalefactor = 1. / (l_float32)factor;
    pixt = pixScaleBySampling(pixs, scalefactor, scalefactor);
    pixd = pixConvertTo32(pixt);

    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixConvert8To32()
 *
 *      Input:  pix (8 bpp)
 *      Return: 32 bpp rgb pix, or null on error
 *
 *  Notes:
 *      (1) If there is no colormap, replicates the gray value
 *          into the 3 MSB of the dest pixel.
 *      (2) Implicit assumption about RGB component ordering.
 */
PIX *
pixConvert8To32(PIX  *pixs)
{
l_int32    i, j, w, h, wpls, wpld, val;
l_uint32  *datas, *datad, *lines, *lined;
l_uint32  *tab;
PIX       *pixd;

    PROCNAME("pixConvert8To32");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);

    if (pixGetColormap(pixs))
        return pixRemoveColormap(pixs, REMOVE_CMAP_TO_FULL_COLOR);

        /* Replication table */
    if ((tab = (l_uint32 *)CALLOC(256, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("tab not made", procName, NULL);
    for (i = 0; i < 256; i++)
      tab[i] = (i << 24) | (i << 16) | (i << 8);

    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if ((pixd = pixCreate(w, h, 32)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            val = GET_DATA_BYTE(lines, j);
            lined[j] = tab[val];
        }
    }

    FREE(tab);
    return pixd;
}


/*---------------------------------------------------------------------------*
 *           Top-level conversion to 8 or 32 bpp, without colormap           *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvertTo8Or32()
 *
 *      Input:  pixs (1, 2, 4, 8, 16, with or without colormap; or 32 bpp rgb)
 *              copyflag (use 0 to return clone if pixs does not need to
 *                         be changed; 1 to return a copy in those situations)
 *              warnflag (1 to issue warning if colormap is removed; else 0)
 *      Return: pixd (8 bpp grayscale or 32 bpp rgb), or null on error
 *
 *  Notes:
 *      (1) If there is a colormap, the colormap is removed to 8 or 32 bpp,
 *          depending on whether the colors in the colormap are all gray.
 *      (2) If the input is either rgb or 8 bpp without a colormap,
 *          this returns either a clone or a copy, depending on @copyflag.
 *      (3) Otherwise, the pix is converted to 8 bpp grayscale.
 *          In all cases, pixd does not have a colormap.
 */
PIX *
pixConvertTo8Or32(PIX     *pixs,
                  l_int32  copyflag,
                  l_int32  warnflag)
{
l_int32  d;
PIX     *pixd;

    PROCNAME("pixConvertTo8Or32");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    d = pixGetDepth(pixs);
    if (pixGetColormap(pixs)) {
        if (warnflag) L_WARNING("pix has colormap; removing\n", procName);
        pixd = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    } else if (d == 8 || d == 32) {
        if (copyflag == 0)
            pixd = pixClone(pixs);
        else
            pixd = pixCopy(NULL, pixs);
    } else {
        pixd = pixConvertTo8(pixs, 0);
    }

        /* Sanity check on result */
    d = pixGetDepth(pixd);
    if (d != 8 && d != 32) {
        pixDestroy(&pixd);
        return (PIX *)ERROR_PTR("depth not 8 or 32 bpp", procName, NULL);
    }

    return pixd;
}


/*---------------------------------------------------------------------------*
 *                 Conversion between 24 bpp and 32 bpp rgb                  *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvert24To32()
 *
 *      Input:  pixs (24 bpp rgb)
 *      Return: pixd (32 bpp rgb), or null on error
 *
 *  Notes:
 *      (1) 24 bpp rgb pix are not supported in leptonica, except for a small
 *          number of formatted write operations.  The data is a byte array,
 *          with pixels in order r,g,b, and padded to 32 bit boundaries
 *          in each line.
 *      (2) Because 24 bpp rgb pix are conveniently generated by programs
 *          such as xpdf (which has SplashBitmaps that store the raster
 *          data in consecutive 24-bit rgb pixels), it is useful to provide
 *          24 bpp pix that simply incorporate that data.  The only things
 *          we can do with these are:
 *            (a) write them to file in png, jpeg, tiff and pnm
 *            (b) interconvert between 24 and 32 bpp in memory (for testing).
 */
PIX *
pixConvert24To32(PIX  *pixs)
{
l_uint8   *lines;
l_int32    w, h, d, i, j, wpls, wpld, rval, gval, bval;
l_uint32   pixel;
l_uint32  *datas, *datad, *lined;
PIX       *pixd;

    PROCNAME("pixConvert24to32");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 24)
        return (PIX *)ERROR_PTR("pixs not 24 bpp", procName, NULL);

    pixd = pixCreateNoInit(w, h, 32);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = (l_uint8 *)(datas + i * wpls);
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            rval = *lines++;
            gval = *lines++;
            bval = *lines++;
            composeRGBPixel(rval, gval, bval, &pixel);
            lined[j] = pixel;
        }
    }
    pixCopyResolution(pixd, pixs);
    pixCopyInputFormat(pixd, pixs);
    return pixd;
}


/*!
 *  pixConvert32To24()
 *
 *      Input:  pixs (32 bpp rgb)
 *      Return: pixd (24 bpp rgb), or null on error
 *
 *  Notes:
 *      (1) See pixconvert24To32().
 */
PIX *
pixConvert32To24(PIX  *pixs)
{
l_uint8   *rgbdata8;
l_int32    w, h, d, i, j, wpls, wpld, rval, gval, bval;
l_uint32  *datas, *lines, *rgbdata;
PIX       *pixd;

    PROCNAME("pixConvert32to24");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 32)
        return (PIX *)ERROR_PTR("pixs not 32 bpp", procName, NULL);

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    pixd = pixCreateNoInit(w, h, 24);
    rgbdata = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        rgbdata8 = (l_uint8 *)(rgbdata + i * wpld);
        for (j = 0; j < w; j++) {
            extractRGBValues(lines[j], &rval, &gval, &bval);
            *rgbdata8++ = rval;
            *rgbdata8++ = gval;
            *rgbdata8++ = bval;
        }
    }
    pixCopyResolution(pixd, pixs);
    pixCopyInputFormat(pixd, pixs);
    return pixd;
}


/*---------------------------------------------------------------------------*
 *        Removal of alpha component by blending with white background       *
 *---------------------------------------------------------------------------*/
/*!
 *  pixRemoveAlpha()
 *
 *      Input:  pixs (any depth)
 *      Return: pixd (if 32 bpp rgba, pixs blended over a white background;
 *                    a clone of pixs otherwise), and null on error
 *
 *  Notes:
 *      (1) This is a wrapper on pixAlphaBlendUniform()
 */
PIX *
pixRemoveAlpha(PIX *pixs)
{
    PROCNAME("pixRemoveAlpha");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    if (pixGetDepth(pixs) == 32 && pixGetSpp(pixs) == 4)
        return pixAlphaBlendUniform(pixs, 0xffffff00);
    else
        return pixClone(pixs);
}


/*---------------------------------------------------------------------------*
 *                  Addition of alpha component to 1 bpp                     *
 *---------------------------------------------------------------------------*/
/*!
 *  pixAddAlphaTo1bpp()
 *
 *      Input:  pixd (<optional> 1 bpp, can be null or equal to pixs
 *              pixs (1 bpp)
 *      Return: pixd (1 bpp with colormap and non-opaque alpha),
 *                    or null on error
 *
 *  Notes:
 *      (1) We don't use 1 bpp colormapped images with alpha in leptonica,
 *          but we support generating them (here), writing to png, and reading
 *          the png.  On reading, they are converted to 32 bpp RGBA.
 *      (2) The background pixels in pixs become fully transparent, and the
 *          foreground pixels are fully opaque.  Thus, this is a compact
 *          1 bpp representation of a stencil, to paint over pixels of
 *          a backing image that are masked by the foreground in pixs.
 */
PIX *
pixAddAlphaTo1bpp(PIX  *pixd,
                  PIX  *pixs)
{
PIXCMAP  *cmap;

    PROCNAME("pixAddAlphaTo1bpp");

    if (!pixs || (pixGetDepth(pixs) != 1))
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (pixd && (pixd != pixs))
        return (PIX *)ERROR_PTR("pixd defined but != pixs", procName, NULL);

    pixd = pixInvert(pixd, pixs);
    cmap = pixcmapCreate(1);
    pixSetColormap(pixd, cmap);
    pixcmapAddRGBA(cmap, 0, 0, 0, 255);  /* black, fully opaque */
    pixcmapAddRGBA(cmap, 255, 255, 255, 0);  /* white, fully transparent */
    return pixd;
}


/*---------------------------------------------------------------------------*
 *                  Lossless depth conversion (unpacking)                    *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvertLossless()
 *
 *      Input:  pixs (1, 2, 4, 8 bpp, not cmapped)
 *              d (destination depth: 2, 4 or 8)
 *      Return: pixd (2, 4 or 8 bpp), or null on error
 *
 *  Notes:
 *      (1) This is a lossless unpacking (depth-increasing)
 *          conversion.  If ds is the depth of pixs, then
 *           - if d < ds, returns NULL
 *           - if d == ds, returns a copy
 *           - if d > ds, does the unpacking conversion
 *      (2) If pixs has a colormap, this is an error.
 */
PIX *
pixConvertLossless(PIX     *pixs,
                   l_int32  d)
{
l_int32    w, h, ds, wpls, wpld, i, j, val;
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixConvertLossless");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs has colormap", procName, NULL);
    if (d != 2 && d != 4 && d != 8)
        return (PIX *)ERROR_PTR("invalid dest depth", procName, NULL);

    pixGetDimensions(pixs, &w, &h, &ds);
    if (d < ds)
        return (PIX *)ERROR_PTR("depth > d", procName, NULL);
    else if (d == ds)
        return pixCopy(NULL, pixs);

    if ((pixd = pixCreate(w, h, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);

        /* Unpack the bits */
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        switch (ds)
        {
        case 1:
            for (j = 0; j < w; j++) {
                val = GET_DATA_BIT(lines, j);
                if (d == 8)
                    SET_DATA_BYTE(lined, j, val);
                else if (d == 4)
                    SET_DATA_QBIT(lined, j, val);
                else  /* d == 2 */
                    SET_DATA_DIBIT(lined, j, val);
            }
            break;
        case 2:
            for (j = 0; j < w; j++) {
                val = GET_DATA_DIBIT(lines, j);
                if (d == 8)
                    SET_DATA_BYTE(lined, j, val);
                else  /* d == 4 */
                    SET_DATA_QBIT(lined, j, val);
            }
        case 4:
            for (j = 0; j < w; j++) {
                val = GET_DATA_DIBIT(lines, j);
                SET_DATA_BYTE(lined, j, val);
            }
            break;
        }
    }

    return pixd;
}


/*---------------------------------------------------------------------------*
 *                     Conversion for printing in PostScript                 *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvertForPSWrap()
 *
 *      Input:  pixs (1, 2, 4, 8, 16, 32 bpp)
 *      Return: pixd (1, 8, or 32 bpp), or null on error
 *
 *  Notes:
 *      (1) For wrapping in PostScript, we convert pixs to
 *          1 bpp, 8 bpp (gray) and 32 bpp (RGB color).
 *      (2) Colormaps are removed.  For pixs with colormaps, the
 *          images are converted to either 8 bpp gray or 32 bpp
 *          RGB, depending on whether the colormap has color content.
 *      (3) Images without colormaps, that are not 1 bpp or 32 bpp,
 *          are converted to 8 bpp gray.
 */
PIX *
pixConvertForPSWrap(PIX  *pixs)
{
l_int32   d;
PIX      *pixd;
PIXCMAP  *cmap;

    PROCNAME("pixConvertForPSWrap");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    cmap = pixGetColormap(pixs);
    d = pixGetDepth(pixs);
    switch (d)
    {
    case 1:
    case 32:
        pixd = pixClone(pixs);
        break;
    case 2:
        if (cmap)
            pixd = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
        else
            pixd = pixConvert2To8(pixs, 0, 0x55, 0xaa, 0xff, FALSE);
        break;
    case 4:
        if (cmap)
            pixd = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
        else
            pixd = pixConvert4To8(pixs, FALSE);
        break;
    case 8:
        pixd = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
        break;
    case 16:
        pixd = pixConvert16To8(pixs, L_MS_BYTE);
        break;
    default:
        fprintf(stderr, "depth not in {1, 2, 4, 8, 16, 32}");
        return NULL;
   }

   return pixd;
}


/*---------------------------------------------------------------------------*
 *                      Scaling conversion to subpixel RGB                   *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvertToSubpixelRGB()
 *
 *      Input:  pixs (8 bpp grayscale, 32 bpp rgb, or colormapped)
 *              scalex, scaley (anisotropic scaling permitted between
 *                              source and destination)
 *              order (of subpixel rgb color components in composition of pixd:
 *                     L_SUBPIXEL_ORDER_RGB, L_SUBPIXEL_ORDER_BGR,
 *                     L_SUBPIXEL_ORDER_VRGB, L_SUBPIXEL_ORDER_VBGR)
 *
 *      Return: pixd (32 bpp), or null on error
 *
 *  Notes:
 *      (1) If pixs has a colormap, it is removed based on its contents
 *          to either 8 bpp gray or rgb.
 *      (2) For horizontal subpixel splitting, the input image
 *          is rescaled by @scaley vertically and by 3.0 times
 *          @scalex horizontally.  Then each horizontal triplet
 *          of pixels is mapped back to a single rgb pixel, with the
 *          r, g and b values being assigned based on the pixel triplet.
 *          For gray triplets, the r, g, and b values are set equal to
 *          the three gray values.  For color triplets, the r, g and b
 *          values are set equal to the components from the appropriate
 *          subpixel.  Vertical subpixel splitting is handled similarly.
 *      (3) See pixConvertGrayToSubpixelRGB() and
 *          pixConvertColorToSubpixelRGB() for further details.
 */
PIX *
pixConvertToSubpixelRGB(PIX       *pixs,
                        l_float32  scalex,
                        l_float32  scaley,
                        l_int32    order)
{
l_int32    d;
PIX       *pixt, *pixd;
PIXCMAP   *cmap;

    PROCNAME("pixConvertToSubpixelRGB");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    cmap = pixGetColormap(pixs);
    if (d != 8 && d != 32 && !cmap)
        return (PIX *)ERROR_PTR("pix not 8 or 32 bpp and not cmapped",
                                procName, NULL);
    if (scalex <= 0.0 || scaley <= 0.0)
        return (PIX *)ERROR_PTR("scale factors must be > 0", procName, NULL);
    if (order != L_SUBPIXEL_ORDER_RGB && order != L_SUBPIXEL_ORDER_BGR &&
        order != L_SUBPIXEL_ORDER_VRGB && order != L_SUBPIXEL_ORDER_VBGR)
        return (PIX *)ERROR_PTR("invalid subpixel order", procName, NULL);
    if ((pixt = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, NULL);

    d = pixGetDepth(pixt);
    pixd = NULL;
    if (d == 8)
        pixd = pixConvertGrayToSubpixelRGB(pixt, scalex, scaley, order);
    else if (d == 32)
        pixd = pixConvertColorToSubpixelRGB(pixt, scalex, scaley, order);
    else
        L_ERROR("invalid depth %d\n", procName, d);

    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixConvertGrayToSubpixelRGB()
 *
 *      Input:  pixs (8 bpp or colormapped)
 *              scalex, scaley
 *              order (of subpixel rgb color components in composition of pixd:
 *                     L_SUBPIXEL_ORDER_RGB, L_SUBPIXEL_ORDER_BGR,
 *                     L_SUBPIXEL_ORDER_VRGB, L_SUBPIXEL_ORDER_VBGR)
 *
 *      Return: pixd (32 bpp), or null on error
 *
 *  Notes:
 *      (1) If pixs has a colormap, it is removed to 8 bpp.
 *      (2) For horizontal subpixel splitting, the input gray image
 *          is rescaled by @scaley vertically and by 3.0 times
 *          @scalex horizontally.  Then each horizontal triplet
 *          of pixels is mapped back to a single rgb pixel, with the
 *          r, g and b values being assigned from the triplet of gray values.
 *          Similar operations are used for vertical subpixel splitting.
 *      (3) This is a form of subpixel rendering that tends to give the
 *          resulting text a sharper and somewhat chromatic display.
 *          For horizontal subpixel splitting, the observable difference
 *          between @order=L_SUBPIXEL_ORDER_RGB and
 *          @order=L_SUBPIXEL_ORDER_BGR is reduced by optical diffusers
 *          in the display that make the pixel color appear to emerge
 *          from the entire pixel.
 */
PIX *
pixConvertGrayToSubpixelRGB(PIX       *pixs,
                            l_float32  scalex,
                            l_float32  scaley,
                            l_int32    order)
{
l_int32    w, h, d, wd, hd, wplt, wpld, i, j, rval, gval, bval, direction;
l_uint32  *datat, *datad, *linet, *lined;
PIX       *pixt1, *pixt2, *pixd;
PIXCMAP   *cmap;

    PROCNAME("pixConvertGrayToSubpixelRGB");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    cmap = pixGetColormap(pixs);
    if (d != 8 && !cmap)
        return (PIX *)ERROR_PTR("pix not 8 bpp & not cmapped", procName, NULL);
    if (scalex <= 0.0 || scaley <= 0.0)
        return (PIX *)ERROR_PTR("scale factors must be > 0", procName, NULL);
    if (order != L_SUBPIXEL_ORDER_RGB && order != L_SUBPIXEL_ORDER_BGR &&
        order != L_SUBPIXEL_ORDER_VRGB && order != L_SUBPIXEL_ORDER_VBGR)
        return (PIX *)ERROR_PTR("invalid subpixel order", procName, NULL);

    direction =
        (order == L_SUBPIXEL_ORDER_RGB || order == L_SUBPIXEL_ORDER_BGR)
        ? L_HORIZ : L_VERT;
    pixt1 = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
    if (direction == L_HORIZ)
        pixt2 = pixScale(pixt1, 3.0 * scalex, scaley);
    else  /* L_VERT */
        pixt2 = pixScale(pixt1, scalex, 3.0 * scaley);

    pixGetDimensions(pixt2, &w, &h, NULL);
    wd = (direction == L_HORIZ) ? w / 3 : w;
    hd = (direction == L_VERT) ? h / 3 : h;
    pixd = pixCreate(wd, hd, 32);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    datat = pixGetData(pixt2);
    wplt = pixGetWpl(pixt2);
    if (direction == L_HORIZ) {
        for (i = 0; i < hd; i++) {
            linet = datat + i * wplt;
            lined = datad + i * wpld;
            for (j = 0; j < wd; j++) {
                rval = GET_DATA_BYTE(linet, 3 * j);
                gval = GET_DATA_BYTE(linet, 3 * j + 1);
                bval = GET_DATA_BYTE(linet, 3 * j + 2);
                if (order == L_SUBPIXEL_ORDER_RGB)
                    composeRGBPixel(rval, gval, bval, &lined[j]);
                else  /* order BGR */
                    composeRGBPixel(bval, gval, rval, &lined[j]);
            }
        }
    } else {  /* L_VERT */
        for (i = 0; i < hd; i++) {
            linet = datat + 3 * i * wplt;
            lined = datad + i * wpld;
            for (j = 0; j < wd; j++) {
                rval = GET_DATA_BYTE(linet, j);
                gval = GET_DATA_BYTE(linet + wplt, j);
                bval = GET_DATA_BYTE(linet + 2 * wplt, j);
                if (order == L_SUBPIXEL_ORDER_VRGB)
                    composeRGBPixel(rval, gval, bval, &lined[j]);
                else  /* order VBGR */
                    composeRGBPixel(bval, gval, rval, &lined[j]);
            }
        }
    }

    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    return pixd;
}


/*!
 *  pixConvertColorToSubpixelRGB()
 *
 *      Input:  pixs (32 bpp or colormapped)
 *              scalex, scaley
 *              order (of subpixel rgb color components in composition of pixd:
 *                     L_SUBPIXEL_ORDER_RGB, L_SUBPIXEL_ORDER_BGR,
 *                     L_SUBPIXEL_ORDER_VRGB, L_SUBPIXEL_ORDER_VBGR)
 *
 *      Return: pixd (32 bpp), or null on error
 *
 *  Notes:
 *      (1) If pixs has a colormap, it is removed to 32 bpp rgb.
 *          If the colormap has no color, pixConvertGrayToSubpixelRGB()
 *          should be called instead, because it will give the same result
 *          more efficiently.  The function pixConvertToSubpixelRGB()
 *          will do the best thing for all cases.
 *      (2) For horizontal subpixel splitting, the input rgb image
 *          is rescaled by @scaley vertically and by 3.0 times
 *          @scalex horizontally.  Then for each horizontal triplet
 *          of pixels, the r component of the final pixel is selected
 *          from the r component of the appropriate pixel in the triplet,
 *          and likewise for g and b.  Vertical subpixel splitting is
 *          handled similarly.
 */
PIX *
pixConvertColorToSubpixelRGB(PIX       *pixs,
                             l_float32  scalex,
                             l_float32  scaley,
                             l_int32    order)
{
l_int32    w, h, d, wd, hd, wplt, wpld, i, j, rval, gval, bval, direction;
l_uint32  *datat, *datad, *linet, *lined;
PIX       *pixt1, *pixt2, *pixd;
PIXCMAP   *cmap;

    PROCNAME("pixConvertColorToSubpixelRGB");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    cmap = pixGetColormap(pixs);
    if (d != 32 && !cmap)
        return (PIX *)ERROR_PTR("pix not 32 bpp & not cmapped", procName, NULL);
    if (scalex <= 0.0 || scaley <= 0.0)
        return (PIX *)ERROR_PTR("scale factors must be > 0", procName, NULL);
    if (order != L_SUBPIXEL_ORDER_RGB && order != L_SUBPIXEL_ORDER_BGR &&
        order != L_SUBPIXEL_ORDER_VRGB && order != L_SUBPIXEL_ORDER_VBGR)
        return (PIX *)ERROR_PTR("invalid subpixel order", procName, NULL);

    direction =
        (order == L_SUBPIXEL_ORDER_RGB || order == L_SUBPIXEL_ORDER_BGR)
        ? L_HORIZ : L_VERT;
    pixt1 = pixRemoveColormap(pixs, REMOVE_CMAP_TO_FULL_COLOR);
    if (direction == L_HORIZ)
        pixt2 = pixScale(pixt1, 3.0 * scalex, scaley);
    else  /* L_VERT */
        pixt2 = pixScale(pixt1, scalex, 3.0 * scaley);

    pixGetDimensions(pixt2, &w, &h, NULL);
    wd = (direction == L_HORIZ) ? w / 3 : w;
    hd = (direction == L_VERT) ? h / 3 : h;
    pixd = pixCreate(wd, hd, 32);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    datat = pixGetData(pixt2);
    wplt = pixGetWpl(pixt2);
    if (direction == L_HORIZ) {
        for (i = 0; i < hd; i++) {
            linet = datat + i * wplt;
            lined = datad + i * wpld;
            for (j = 0; j < wd; j++) {
                if (order == L_SUBPIXEL_ORDER_RGB) {
                    extractRGBValues(linet[3 * j], &rval, NULL, NULL);
                    extractRGBValues(linet[3 * j + 1], NULL, &gval, NULL);
                    extractRGBValues(linet[3 * j + 2], NULL, NULL, &bval);
                } else {  /* order BGR */
                    extractRGBValues(linet[3 * j], NULL, NULL, &bval);
                    extractRGBValues(linet[3 * j + 1], NULL, &gval, NULL);
                    extractRGBValues(linet[3 * j + 2], &rval, NULL, NULL);
                }
                composeRGBPixel(rval, gval, bval, &lined[j]);
            }
        }
    } else {  /* L_VERT */
        for (i = 0; i < hd; i++) {
            linet = datat + 3 * i * wplt;
            lined = datad + i * wpld;
            for (j = 0; j < wd; j++) {
                if (order == L_SUBPIXEL_ORDER_VRGB) {
                    extractRGBValues(linet[j], &rval, NULL, NULL);
                    extractRGBValues((linet + wplt)[j], NULL, &gval, NULL);
                    extractRGBValues((linet + 2 * wplt)[j], NULL, NULL, &bval);
                } else {  /* order VBGR */
                    extractRGBValues(linet[j], NULL, NULL, &bval);
                    extractRGBValues((linet + wplt)[j], NULL, &gval, NULL);
                    extractRGBValues((linet + 2 * wplt)[j], &rval, NULL, NULL);
                }
                composeRGBPixel(rval, gval, bval, &lined[j]);
            }
        }
    }

    if (pixGetSpp(pixs) == 4)
        pixScaleAndTransferAlpha(pixd, pixs, scalex, scaley);

    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    return pixd;
}
