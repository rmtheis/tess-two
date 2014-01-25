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
 *  colorspace.c
 *
 *      Colorspace conversion between RGB and HSV
 *           PIX        *pixConvertRGBToHSV()
 *           PIX        *pixConvertHSVToRGB()
 *           l_int32     convertRGBToHSV()
 *           l_int32     convertHSVToRGB()
 *           l_int32     pixcmapConvertRGBToHSV()
 *           l_int32     pixcmapConvertHSVToRGB()
 *           PIX        *pixConvertRGBToHue()
 *           PIX        *pixConvertRGBToSaturation()
 *           PIX        *pixConvertRGBToValue()
 *
 *      Selection and display of range of colors in HSV space
 *           PIX        *pixMakeRangeMaskHS()
 *           PIX        *pixMakeRangeMaskHV()
 *           PIX        *pixMakeRangeMaskSV()
 *           PIX        *pixMakeHistoHS()
 *           PIX        *pixMakeHistoHV()
 *           PIX        *pixMakeHistoSV()
 *           PIX        *pixFindHistoPeaksHSV()
 *           PIX        *displayHSVColorRange()
 *
 *      Colorspace conversion between RGB and YUV
 *           PIX        *pixConvertRGBToYUV()
 *           PIX        *pixConvertYUVToRGB()
 *           l_int32     convertRGBToYUV()
 *           l_int32     convertYUVToRGB()
 *           l_int32     pixcmapConvertRGBToYUV()
 *           l_int32     pixcmapConvertYUVToRGB()
 */

#include <string.h>
#include <math.h>
#include "allheaders.h"

#ifndef  NO_CONSOLE_IO
#define  DEBUG_HISTO       1
#endif  /* ~NO_CONSOLE_IO */


/*---------------------------------------------------------------------------*
 *                  Colorspace conversion between RGB and HSB                *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvertRGBToHSV()
 *
 *      Input:  pixd (can be NULL; if not NULL, must == pixs)
 *              pixs
 *      Return: pixd always
 *
 *  Notes:
 *      (1) For pixs = pixd, this is in-place; otherwise pixd must be NULL.
 *      (2) The definition of our HSV space is given in convertRGBToHSV().
 *      (3) The h, s and v values are stored in the same places as
 *          the r, g and b values, respectively.  Here, they are explicitly
 *          placed in the 3 MS bytes in the pixel.
 *      (4) Normalizing to 1 and considering the r,g,b components,
 *          a simple way to understand the HSV space is:
 *           - v = max(r,g,b)
 *           - s = (max - min) / max
 *           - h ~ (mid - min) / (max - min)  [apart from signs and constants]
 *      (5) Normalizing to 1, some properties of the HSV space are:
 *           - For gray values (r = g = b) along the continuum between
 *             black and white:
 *                s = 0  (becoming undefined as you approach black)
 *                h is undefined everywhere
 *           - Where one component is saturated and the others are zero:
 *                v = 1
 *                s = 1
 *                h = 0 (r = max), 1/3 (g = max), 2/3 (b = max)
 *           - Where two components are saturated and the other is zero:
 *                v = 1
 *                s = 1
 *                h = 1/2 (if r = 0), 5/6 (if g = 0), 1/6 (if b = 0)
 */
PIX *
pixConvertRGBToHSV(PIX  *pixd,
                   PIX  *pixs)
{
l_int32    w, h, d, wpl, i, j, rval, gval, bval, hval, sval, vval;
l_uint32  *line, *data;
PIXCMAP   *cmap;

    PROCNAME("pixConvertRGBToHSV");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixd && pixd != pixs)
        return (PIX *)ERROR_PTR("pixd defined and not inplace", procName, pixd);

    d = pixGetDepth(pixs);
    cmap = pixGetColormap(pixs);
    if (!cmap && d != 32)
        return (PIX *)ERROR_PTR("not cmapped or rgb", procName, pixd);

    if (!pixd)
        pixd = pixCopy(NULL, pixs);

    cmap = pixGetColormap(pixd);
    if (cmap) {   /* just convert the colormap */
        pixcmapConvertRGBToHSV(cmap);
        return pixd;
    }

        /* Convert RGB image */
    pixGetDimensions(pixd, &w, &h, NULL);
    wpl = pixGetWpl(pixd);
    data = pixGetData(pixd);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < w; j++) {
            extractRGBValues(line[j], &rval, &gval, &bval);
            convertRGBToHSV(rval, gval, bval, &hval, &sval, &vval);
            line[j] = (hval << 24) | (sval << 16) | (vval << 8);
        }
    }

    return pixd;
}


/*!
 *  pixConvertHSVToRGB()
 *
 *      Input:  pixd (can be NULL; if not NULL, must == pixs)
 *              pixs
 *      Return: pixd always
 *
 *  Notes:
 *      (1) For pixs = pixd, this is in-place; otherwise pixd must be NULL.
 *      (2) The user takes responsibility for making sure that pixs is
 *          in our HSV space.  The definition of our HSV space is given
 *          in convertRGBToHSV().
 *      (3) The h, s and v values are stored in the same places as
 *          the r, g and b values, respectively.  Here, they are explicitly
 *          placed in the 3 MS bytes in the pixel.
 */
PIX *
pixConvertHSVToRGB(PIX  *pixd,
                   PIX  *pixs)
{
l_int32    w, h, d, wpl, i, j, rval, gval, bval, hval, sval, vval;
l_uint32   pixel;
l_uint32  *line, *data;
PIXCMAP   *cmap;

    PROCNAME("pixConvertHSVToRGB");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixd && pixd != pixs)
        return (PIX *)ERROR_PTR("pixd defined and not inplace", procName, pixd);

    d = pixGetDepth(pixs);
    cmap = pixGetColormap(pixs);
    if (!cmap && d != 32)
        return (PIX *)ERROR_PTR("not cmapped or hsv", procName, pixd);

    if (!pixd)
        pixd = pixCopy(NULL, pixs);

    cmap = pixGetColormap(pixd);
    if (cmap) {   /* just convert the colormap */
        pixcmapConvertHSVToRGB(cmap);
        return pixd;
    }

        /* Convert HSV image */
    pixGetDimensions(pixd, &w, &h, NULL);
    wpl = pixGetWpl(pixd);
    data = pixGetData(pixd);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < w; j++) {
            pixel = line[j];
            hval = pixel >> 24;
            sval = (pixel >> 16) & 0xff;
            vval = (pixel >> 8) & 0xff;
            convertHSVToRGB(hval, sval, vval, &rval, &gval, &bval);
            composeRGBPixel(rval, gval, bval, line + j);
        }
    }

    return pixd;
}


/*!
 *  convertRGBToHSV()
 *
 *      Input:  rval, gval, bval (RGB input)
 *              &hval, &sval, &vval (<return> HSV values)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The range of returned values is:
 *            h [0 ... 239]
 *            s [0 ... 255]
 *            v [0 ... 255]
 *      (2) If r = g = b, the pixel is gray (s = 0), and we define h = 0.
 *      (3) h wraps around, so that h = 0 and h = 240 are equivalent
 *          in hue space.
 *      (4) h has the following correspondence to color:
 *            h = 0         magenta
 *            h = 40        red
 *            h = 80        yellow
 *            h = 120       green
 *            h = 160       cyan
 *            h = 200       blue
 */
l_int32
convertRGBToHSV(l_int32   rval,
                l_int32   gval,
                l_int32   bval,
                l_int32  *phval,
                l_int32  *psval,
                l_int32  *pvval)
{
l_int32    minrg, maxrg, min, max, delta;
l_float32  h;

    PROCNAME("convertRGBToHSV");

    if (!phval || !psval || !pvval)
        return ERROR_INT("&hval, &sval, &vval not all defined", procName, 1);

    minrg = L_MIN(rval, gval);
    min = L_MIN(minrg, bval);
    maxrg = L_MAX(rval, gval);
    max = L_MAX(maxrg, bval);
    delta = max - min;

    *pvval = max;
    if (delta == 0) {  /* gray; no chroma */
        *phval = 0;
        *psval = 0;
    } else {
        *psval = (l_int32)(255. * (l_float32)delta / (l_float32)max + 0.5);
        if (rval == max)  /* between magenta and yellow */
            h = (l_float32)(gval - bval) / (l_float32)delta;
        else if (gval == max)  /* between yellow and cyan */
            h = 2. + (l_float32)(bval - rval) / (l_float32)delta;
        else  /* between cyan and magenta */
            h = 4. + (l_float32)(rval - gval) / (l_float32)delta;
        h *= 40.0;
        if (h < 0.0)
            h += 240.0;
        if (h >= 239.5)
            h = 0.0;
        *phval = (l_int32)(h + 0.5);
    }

    return 0;
}


/*!
 *  convertHSVToRGB()
 *
 *      Input:  hval, sval, vval
 *              &rval, &gval, &bval (<return> RGB values)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) See convertRGBToHSV() for valid input range of HSV values
 *          and their interpretation in color space.
 */
l_int32
convertHSVToRGB(l_int32   hval,
                l_int32   sval,
                l_int32   vval,
                l_int32  *prval,
                l_int32  *pgval,
                l_int32  *pbval)
{
l_int32   i, x, y, z;
l_float32 h, f, s;

    PROCNAME("convertHSVToRGB");

    if (!prval || !pgval || !pbval)
        return ERROR_INT("&rval, &gval, &bval not all defined", procName, 1);

    if (sval == 0) {  /* gray */
        *prval = vval;
        *pgval = vval;
        *pbval = vval;
    } else {
        if (hval < 0 || hval > 240)
            return ERROR_INT("invalid hval", procName, 1);
        if (hval == 240)
            hval = 0;
        h = (l_float32)hval / 40.;
        i = (l_int32)h;
        f = h - i;
        s = (l_float32)sval / 255.;
        x = (l_int32)(vval * (1. - s) + 0.5);
        y = (l_int32)(vval * (1. - s * f) + 0.5);
        z = (l_int32)(vval * (1. - s * (1. - f)) + 0.5);
        switch (i)
        {
        case 0:
            *prval = vval;
            *pgval = z;
            *pbval = x;
            break;
        case 1:
            *prval = y;
            *pgval = vval;
            *pbval = x;
            break;
        case 2:
            *prval = x;
            *pgval = vval;
            *pbval = z;
            break;
        case 3:
            *prval = x;
            *pgval = y;
            *pbval = vval;
            break;
        case 4:
            *prval = z;
            *pgval = x;
            *pbval = vval;
            break;
        case 5:
            *prval = vval;
            *pgval = x;
            *pbval = y;
            break;
        default:  /* none possible */
            return 1;
        }
    }

    return 0;
}


/*!
 *  pixcmapConvertRGBToHSV()
 *
 *      Input:  colormap
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      - in-place transform
 *      - See convertRGBToHSV() for def'n of HSV space.
 *      - replaces: r --> h, g --> s, b --> v
 */
l_int32
pixcmapConvertRGBToHSV(PIXCMAP  *cmap)
{
l_int32   i, ncolors, rval, gval, bval, hval, sval, vval;

    PROCNAME("pixcmapConvertRGBToHSV");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    ncolors = pixcmapGetCount(cmap);
    for (i = 0; i < ncolors; i++) {
        pixcmapGetColor(cmap, i, &rval, &gval, &bval);
        convertRGBToHSV(rval, gval, bval, &hval, &sval, &vval);
        pixcmapResetColor(cmap, i, hval, sval, vval);
    }
    return 0;
}


/*!
 *  pixcmapConvertHSVToRGB()
 *
 *      Input:  colormap
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      - in-place transform
 *      - See convertRGBToHSV() for def'n of HSV space.
 *      - replaces: h --> r, s --> g, v --> b
 */
l_int32
pixcmapConvertHSVToRGB(PIXCMAP  *cmap)
{
l_int32   i, ncolors, rval, gval, bval, hval, sval, vval;

    PROCNAME("pixcmapConvertHSVToRGB");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    ncolors = pixcmapGetCount(cmap);
    for (i = 0; i < ncolors; i++) {
        pixcmapGetColor(cmap, i, &hval, &sval, &vval);
        convertHSVToRGB(hval, sval, vval, &rval, &gval, &bval);
        pixcmapResetColor(cmap, i, rval, gval, bval);
    }
    return 0;
}


/*!
 *  pixConvertRGBToHue()
 *
 *      Input:  pixs (32 bpp RGB or 8 bpp with colormap)
 *      Return: pixd (8 bpp hue of HSV), or null on error
 *
 *  Notes:
 *      (1) The conversion to HSV hue is in-lined here.
 *      (2) If there is a colormap, it is removed.
 *      (3) If you just want the hue component, this does it
 *          at about 10 Mpixels/sec/GHz, which is about
 *          2x faster than using pixConvertRGBToHSV()
 */
PIX *
pixConvertRGBToHue(PIX  *pixs)
{
l_int32    w, h, d, wplt, wpld;
l_int32    i, j, rval, gval, bval, hval, minrg, min, maxrg, max, delta;
l_float32  fh;
l_uint32   pixel;
l_uint32  *linet, *lined, *datat, *datad;
PIX       *pixt, *pixd;

    PROCNAME("pixConvertRGBToHue");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 32 && !pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("not cmapped or rgb", procName, NULL);
    pixt = pixRemoveColormap(pixs, REMOVE_CMAP_TO_FULL_COLOR);

        /* Convert RGB image */
    pixd = pixCreate(w, h, 8);
    pixCopyResolution(pixd, pixs);
    wplt = pixGetWpl(pixt);
    datat = pixGetData(pixt);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            pixel = linet[j];
            extractRGBValues(pixel, &rval, &gval, &bval);
            minrg = L_MIN(rval, gval);
            min = L_MIN(minrg, bval);
            maxrg = L_MAX(rval, gval);
            max = L_MAX(maxrg, bval);
            delta = max - min;
            if (delta == 0) {  /* gray; no chroma */
                hval = 0;
            } else {
                if (rval == max)  /* between magenta and yellow */
                    fh = (l_float32)(gval - bval) / (l_float32)delta;
                else if (gval == max)  /* between yellow and cyan */
                    fh = 2. + (l_float32)(bval - rval) / (l_float32)delta;
                else  /* between cyan and magenta */
                    fh = 4. + (l_float32)(rval - gval) / (l_float32)delta;
                fh *= 40.0;
                if (fh < 0.0)
                    fh += 240.0;
                hval = (l_int32)(fh + 0.5);
            }
            SET_DATA_BYTE(lined, j, hval);
        }
    }
    pixDestroy(&pixt);

    return pixd;
}



/*!
 *  pixConvertRGBToSaturation()
 *
 *      Input:  pixs (32 bpp RGB or 8 bpp with colormap)
 *      Return: pixd (8 bpp sat of HSV), or null on error
 *
 *  Notes:
 *      (1) The conversion to HSV sat is in-lined here.
 *      (2) If there is a colormap, it is removed.
 *      (3) If you just want the saturation component, this does it
 *          at about 12 Mpixels/sec/GHz.
 */
PIX *
pixConvertRGBToSaturation(PIX  *pixs)
{
l_int32    w, h, d, wplt, wpld;
l_int32    i, j, rval, gval, bval, sval, minrg, min, maxrg, max, delta;
l_uint32   pixel;
l_uint32  *linet, *lined, *datat, *datad;
PIX       *pixt, *pixd;

    PROCNAME("pixConvertRGBToSaturation");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 32 && !pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("not cmapped or rgb", procName, NULL);
    pixt = pixRemoveColormap(pixs, REMOVE_CMAP_TO_FULL_COLOR);

        /* Convert RGB image */
    pixd = pixCreate(w, h, 8);
    pixCopyResolution(pixd, pixs);
    wplt = pixGetWpl(pixt);
    datat = pixGetData(pixt);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            pixel = linet[j];
            extractRGBValues(pixel, &rval, &gval, &bval);
            minrg = L_MIN(rval, gval);
            min = L_MIN(minrg, bval);
            maxrg = L_MAX(rval, gval);
            max = L_MAX(maxrg, bval);
            delta = max - min;
            if (delta == 0)  /* gray; no chroma */
                sval = 0;
            else
                sval = (l_int32)(255. *
                                 (l_float32)delta / (l_float32)max + 0.5);
            SET_DATA_BYTE(lined, j, sval);
        }
    }

    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixConvertRGBToValue()
 *
 *      Input:  pixs (32 bpp RGB or 8 bpp with colormap)
 *      Return: pixd (8 bpp max component intensity of HSV), or null on error
 *
 *  Notes:
 *      (1) The conversion to HSV sat is in-lined here.
 *      (2) If there is a colormap, it is removed.
 *      (3) If you just want the value component, this does it
 *          at about 35 Mpixels/sec/GHz.
 */
PIX *
pixConvertRGBToValue(PIX  *pixs)
{
l_int32    w, h, d, wplt, wpld;
l_int32    i, j, rval, gval, bval, maxrg, max;
l_uint32   pixel;
l_uint32  *linet, *lined, *datat, *datad;
PIX       *pixt, *pixd;

    PROCNAME("pixConvertRGBToValue");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 32 && !pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("not cmapped or rgb", procName, NULL);
    pixt = pixRemoveColormap(pixs, REMOVE_CMAP_TO_FULL_COLOR);

        /* Convert RGB image */
    pixd = pixCreate(w, h, 8);
    pixCopyResolution(pixd, pixs);
    wplt = pixGetWpl(pixt);
    datat = pixGetData(pixt);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            pixel = linet[j];
            extractRGBValues(pixel, &rval, &gval, &bval);
            maxrg = L_MAX(rval, gval);
            max = L_MAX(maxrg, bval);
            SET_DATA_BYTE(lined, j, max);
        }
    }

    pixDestroy(&pixt);
    return pixd;
}


/*---------------------------------------------------------------------------*
 *            Selection and display of range of colors in HSV space          *
 *---------------------------------------------------------------------------*/
/*!
 *  pixMakeRangeMaskHS()
 *
 *      Input:  pixs  (32 bpp rgb)
 *              huecenter (center value of hue range)
 *              huehw (half-width of hue range)
 *              satcenter (center value of saturation range)
 *              sathw (half-width of saturation range)
 *              regionflag (L_INCLUDE_REGION, L_EXCLUDE_REGION)
 *      Return: pixd (1 bpp mask over selected pixels), or null on error
 *
 *  Notes:
 *      (1) The pixels are selected based on the specified ranges of
 *          hue and saturation.  For selection or exclusion, the pixel
 *          HS component values must be within both ranges.  Care must
 *          be taken in finding the hue range because of wrap-around.
 *      (2) Use @regionflag == L_INCLUDE_REGION to take only those
 *          pixels within the rectangular region specified in HS space.
 *          Use @regionflag == L_EXCLUDE_REGION to take all pixels except
 *          those within the rectangular region specified in HS space.
 */
PIX *
pixMakeRangeMaskHS(PIX     *pixs,
                   l_int32  huecenter,
                   l_int32  huehw,
                   l_int32  satcenter,
                   l_int32  sathw,
                   l_int32  regionflag)
{
l_int32    i, j, w, h, wplt, wpld, hstart, hend, sstart, send, hval, sval;
l_int32   *hlut, *slut;
l_uint32   pixel;
l_uint32  *datat, *datad, *linet, *lined;
PIX       *pixt, *pixd;

    PROCNAME("pixMakeRangeMaskHS");

    if (!pixs || pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);
    if (regionflag != L_INCLUDE_REGION && regionflag != L_EXCLUDE_REGION)
        return (PIX *)ERROR_PTR("invalid regionflag", procName, NULL);

        /* Set up LUTs for hue and saturation.  These have the value 1
         * within the specified intervals of hue and saturation. */
    hlut = (l_int32 *)CALLOC(240, sizeof(l_int32));
    slut = (l_int32 *)CALLOC(256, sizeof(l_int32));
    sstart = L_MAX(0, satcenter - sathw);
    send = L_MIN(255, satcenter + sathw);
    for (i = sstart; i <= send; i++)
        slut[i] = 1;
    hstart = (huecenter - huehw + 240) % 240;
    hend = (huecenter + huehw + 240) % 240;
    if (hstart < hend) {
        for (i = hstart; i <= hend; i++)
            hlut[i] = 1;
    } else {  /* wrap */
        for (i = hstart; i < 240; i++)
            hlut[i] = 1;
        for (i = 0; i <= hend; i++)
            hlut[i] = 1;
    }

        /* Generate the mask */
    pixt = pixConvertRGBToHSV(NULL, pixs);
    pixGetDimensions(pixs, &w, &h, NULL);
    pixd = pixCreateNoInit(w, h, 1);
    if (regionflag == L_INCLUDE_REGION)
        pixClearAll(pixd);
    else  /* L_EXCLUDE_REGION */
        pixSetAll(pixd);
    datat = pixGetData(pixt);
    datad = pixGetData(pixd);
    wplt = pixGetWpl(pixt);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            pixel = linet[j];
            hval = (pixel >> L_RED_SHIFT) & 0xff;
            sval = (pixel >> L_GREEN_SHIFT) & 0xff;
            if (hlut[hval] == 1 && slut[sval] == 1) {
                if (regionflag == L_INCLUDE_REGION)
                    SET_DATA_BIT(lined, j);
                else  /* L_EXCLUDE_REGION */
                    CLEAR_DATA_BIT(lined, j);
            }
        }
    }

    FREE(hlut);
    FREE(slut);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixMakeRangeMaskHV()
 *
 *      Input:  pixs  (32 bpp rgb)
 *              huecenter (center value of hue range)
 *              huehw (half-width of hue range)
 *              valcenter (center value of max intensity range)
 *              valhw (half-width of max intensity range)
 *              regionflag (L_INCLUDE_REGION, L_EXCLUDE_REGION)
 *      Return: pixd (1 bpp mask over selected pixels), or null on error
 *
 *  Notes:
 *      (1) The pixels are selected based on the specified ranges of
 *          hue and max intensity values.  For selection or exclusion,
 *          the pixel HV component values must be within both ranges.
 *          Care must be taken in finding the hue range because of wrap-around.
 *      (2) Use @regionflag == L_INCLUDE_REGION to take only those
 *          pixels within the rectangular region specified in HV space.
 *          Use @regionflag == L_EXCLUDE_REGION to take all pixels except
 *          those within the rectangular region specified in HV space.
 */
PIX *
pixMakeRangeMaskHV(PIX     *pixs,
                   l_int32  huecenter,
                   l_int32  huehw,
                   l_int32  valcenter,
                   l_int32  valhw,
                   l_int32  regionflag)
{
l_int32    i, j, w, h, wplt, wpld, hstart, hend, vstart, vend, hval, vval;
l_int32   *hlut, *vlut;
l_uint32   pixel;
l_uint32  *datat, *datad, *linet, *lined;
PIX       *pixt, *pixd;

    PROCNAME("pixMakeRangeMaskHV");

    if (!pixs || pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);
    if (regionflag != L_INCLUDE_REGION && regionflag != L_EXCLUDE_REGION)
        return (PIX *)ERROR_PTR("invalid regionflag", procName, NULL);

        /* Set up LUTs for hue and maximum intensity (val).  These have
         * the value 1 within the specified intervals of hue and value. */
    hlut = (l_int32 *)CALLOC(240, sizeof(l_int32));
    vlut = (l_int32 *)CALLOC(256, sizeof(l_int32));
    vstart = L_MAX(0, valcenter - valhw);
    vend = L_MIN(255, valcenter + valhw);
    for (i = vstart; i <= vend; i++)
        vlut[i] = 1;
    hstart = (huecenter - huehw + 240) % 240;
    hend = (huecenter + huehw + 240) % 240;
    if (hstart < hend) {
        for (i = hstart; i <= hend; i++)
            hlut[i] = 1;
    } else {
        for (i = hstart; i < 240; i++)
            hlut[i] = 1;
        for (i = 0; i <= hend; i++)
            hlut[i] = 1;
    }

        /* Generate the mask */
    pixt = pixConvertRGBToHSV(NULL, pixs);
    pixGetDimensions(pixs, &w, &h, NULL);
    pixd = pixCreateNoInit(w, h, 1);
    if (regionflag == L_INCLUDE_REGION)
        pixClearAll(pixd);
    else  /* L_EXCLUDE_REGION */
        pixSetAll(pixd);
    datat = pixGetData(pixt);
    datad = pixGetData(pixd);
    wplt = pixGetWpl(pixt);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            pixel = linet[j];
            hval = (pixel >> L_RED_SHIFT) & 0xff;
            vval = (pixel >> L_BLUE_SHIFT) & 0xff;
            if (hlut[hval] == 1 && vlut[vval] == 1) {
                if (regionflag == L_INCLUDE_REGION)
                    SET_DATA_BIT(lined, j);
                else  /* L_EXCLUDE_REGION */
                    CLEAR_DATA_BIT(lined, j);
            }
        }
    }

    FREE(hlut);
    FREE(vlut);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixMakeRangeMaskSV()
 *
 *      Input:  pixs  (32 bpp rgb)
 *              satcenter (center value of saturation range)
 *              sathw (half-width of saturation range)
 *              valcenter (center value of max intensity range)
 *              valhw (half-width of max intensity range)
 *              regionflag (L_INCLUDE_REGION, L_EXCLUDE_REGION)
 *      Return: pixd (1 bpp mask over selected pixels), or null on error
 *
 *  Notes:
 *      (1) The pixels are selected based on the specified ranges of
 *          saturation and max intensity (val).  For selection or
 *          exclusion, the pixel SV component values must be within both ranges.
 *      (2) Use @regionflag == L_INCLUDE_REGION to take only those
 *          pixels within the rectangular region specified in SV space.
 *          Use @regionflag == L_EXCLUDE_REGION to take all pixels except
 *          those within the rectangular region specified in SV space.
 */
PIX *
pixMakeRangeMaskSV(PIX     *pixs,
                   l_int32  satcenter,
                   l_int32  sathw,
                   l_int32  valcenter,
                   l_int32  valhw,
                   l_int32  regionflag)
{
l_int32    i, j, w, h, wplt, wpld, sval, vval, sstart, send, vstart, vend;
l_int32   *slut, *vlut;
l_uint32   pixel;
l_uint32  *datat, *datad, *linet, *lined;
PIX       *pixt, *pixd;

    PROCNAME("pixMakeRangeMaskSV");

    if (!pixs || pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);
    if (regionflag != L_INCLUDE_REGION && regionflag != L_EXCLUDE_REGION)
        return (PIX *)ERROR_PTR("invalid regionflag", procName, NULL);

        /* Set up LUTs for saturation and max intensity (val).
         * These have the value 1 within the specified intervals of
         * saturation and max intensity. */
    slut = (l_int32 *)CALLOC(256, sizeof(l_int32));
    vlut = (l_int32 *)CALLOC(256, sizeof(l_int32));
    sstart = L_MAX(0, satcenter - sathw);
    send = L_MIN(255, satcenter + sathw);
    vstart = L_MAX(0, valcenter - valhw);
    vend = L_MIN(255, valcenter + valhw);
    for (i = sstart; i <= send; i++)
        slut[i] = 1;
    for (i = vstart; i <= vend; i++)
        vlut[i] = 1;

        /* Generate the mask */
    pixt = pixConvertRGBToHSV(NULL, pixs);
    pixGetDimensions(pixs, &w, &h, NULL);
    pixd = pixCreateNoInit(w, h, 1);
    if (regionflag == L_INCLUDE_REGION)
        pixClearAll(pixd);
    else  /* L_EXCLUDE_REGION */
        pixSetAll(pixd);
    datat = pixGetData(pixt);
    datad = pixGetData(pixd);
    wplt = pixGetWpl(pixt);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            pixel = linet[j];
            sval = (pixel >> L_GREEN_SHIFT) & 0xff;
            vval = (pixel >> L_BLUE_SHIFT) & 0xff;
            if (slut[sval] == 1 && vlut[vval] == 1) {
                if (regionflag == L_INCLUDE_REGION)
                    SET_DATA_BIT(lined, j);
                else  /* L_EXCLUDE_REGION */
                    CLEAR_DATA_BIT(lined, j);
            }
        }
    }

    FREE(slut);
    FREE(vlut);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixMakeHistoHS()
 *
 *      Input:  pixs  (HSV colorspace)
 *              factor (subsampling factor; integer)
 *              &nahue (<optional return> hue histogram)
 *              &nasat (<optional return> saturation histogram)
 *      Return: pixd (32 bpp histogram in hue and saturation), or null on error
 *
 *  Notes:
 *      (1) pixs is a 32 bpp image in HSV colorspace; hue is in the "red"
 *          byte, saturation is in the "green" byte.
 *      (2) In pixd, hue is displayed vertically; saturation horizontally.
 *          The dimensions of pixd are w = 256, h = 240, and the depth
 *          is 32 bpp.  The value at each point is simply the number
 *          of pixels found at that value of hue and saturation.
 */
PIX *
pixMakeHistoHS(PIX     *pixs,
               l_int32  factor,
               NUMA   **pnahue,
               NUMA   **pnasat)
{
l_int32    i, j, w, h, wplt, hval, sval, nd;
l_uint32   pixel;
l_uint32  *datat, *linet;
void     **lined32;
NUMA      *nahue, *nasat;
PIX       *pixt, *pixd;

    PROCNAME("pixMakeHistoHS");

    if (pnahue) *pnahue = NULL;
    if (pnasat) *pnasat = NULL;
    if (!pixs || pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);

    if (pnahue) {
        nahue = numaCreate(240);
        numaSetCount(nahue, 240);
        *pnahue = nahue;
    }
    if (pnasat) {
        nasat = numaCreate(256);
        numaSetCount(nasat, 256);
        *pnasat = nasat;
    }

    if (factor <= 1)
        pixt = pixClone(pixs);
    else
        pixt = pixScaleBySampling(pixs, 1.0 / (l_float32)factor,
                                  1.0 / (l_float32)factor);

        /* Create the hue-saturation histogram */
    pixd = pixCreate(256, 240, 32);
    lined32 = pixGetLinePtrs(pixd, NULL);
    pixGetDimensions(pixt, &w, &h, NULL);
    datat = pixGetData(pixt);
    wplt = pixGetWpl(pixt);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        for (j = 0; j < w; j++) {
            pixel = linet[j];
            hval = (pixel >> L_RED_SHIFT) & 0xff;

#if  DEBUG_HISTO
            if (hval > 239) {
                fprintf(stderr, "hval = %d for (%d,%d)\n", hval, i, j);
                continue;
            }
#endif  /* DEBUG_HISTO */

            sval = (pixel >> L_GREEN_SHIFT) & 0xff;
            if (pnahue)
                numaShiftValue(nahue, hval, 1.0);
            if (pnasat)
                numaShiftValue(nasat, sval, 1.0);
            nd = GET_DATA_FOUR_BYTES(lined32[hval], sval);
            SET_DATA_FOUR_BYTES(lined32[hval], sval, nd + 1);
        }
    }

    FREE(lined32);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixMakeHistoHV()
 *
 *      Input:  pixs  (HSV colorspace)
 *              factor (subsampling factor; integer)
 *              &nahue (<optional return> hue histogram)
 *              &naval (<optional return> max intensity (value) histogram)
 *      Return: pixd (32 bpp histogram in hue and value), or null on error
 *
 *  Notes:
 *      (1) pixs is a 32 bpp image in HSV colorspace; hue is in the "red"
 *          byte, max intensity ("value") is in the "blue" byte.
 *      (2) In pixd, hue is displayed vertically; intensity horizontally.
 *          The dimensions of pixd are w = 256, h = 240, and the depth
 *          is 32 bpp.  The value at each point is simply the number
 *          of pixels found at that value of hue and intensity.
 */
PIX *
pixMakeHistoHV(PIX     *pixs,
               l_int32  factor,
               NUMA   **pnahue,
               NUMA   **pnaval)
{
l_int32    i, j, w, h, wplt, hval, vval, nd;
l_uint32   pixel;
l_uint32  *datat, *linet;
void     **lined32;
NUMA      *nahue, *naval;
PIX       *pixt, *pixd;

    PROCNAME("pixMakeHistoHV");

    if (pnahue) *pnahue = NULL;
    if (pnaval) *pnaval = NULL;
    if (!pixs || pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);

    if (pnahue) {
        nahue = numaCreate(240);
        numaSetCount(nahue, 240);
        *pnahue = nahue;
    }
    if (pnaval) {
        naval = numaCreate(256);
        numaSetCount(naval, 256);
        *pnaval = naval;
    }

    if (factor <= 1)
        pixt = pixClone(pixs);
    else
        pixt = pixScaleBySampling(pixs, 1.0 / (l_float32)factor,
                                  1.0 / (l_float32)factor);

        /* Create the hue-value histogram */
    pixd = pixCreate(256, 240, 32);
    lined32 = pixGetLinePtrs(pixd, NULL);
    pixGetDimensions(pixt, &w, &h, NULL);
    datat = pixGetData(pixt);
    wplt = pixGetWpl(pixt);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        for (j = 0; j < w; j++) {
            pixel = linet[j];
            hval = (pixel >> L_RED_SHIFT) & 0xff;
            vval = (pixel >> L_BLUE_SHIFT) & 0xff;
            if (pnahue)
                numaShiftValue(nahue, hval, 1.0);
            if (pnaval)
                numaShiftValue(naval, vval, 1.0);
            nd = GET_DATA_FOUR_BYTES(lined32[hval], vval);
            SET_DATA_FOUR_BYTES(lined32[hval], vval, nd + 1);
        }
    }

    FREE(lined32);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixMakeHistoSV()
 *
 *      Input:  pixs  (HSV colorspace)
 *              factor (subsampling factor; integer)
 *              &nasat (<optional return> sat histogram)
 *              &naval (<optional return> max intensity (value) histogram)
 *      Return: pixd (32 bpp histogram in sat and value), or null on error
 *
 *  Notes:
 *      (1) pixs is a 32 bpp image in HSV colorspace; sat is in the "green"
 *          byte, max intensity ("value") is in the "blue" byte.
 *      (2) In pixd, sat is displayed vertically; intensity horizontally.
 *          The dimensions of pixd are w = 256, h = 256, and the depth
 *          is 32 bpp.  The value at each point is simply the number
 *          of pixels found at that value of saturation and intensity.
 */
PIX *
pixMakeHistoSV(PIX     *pixs,
               l_int32  factor,
               NUMA   **pnasat,
               NUMA   **pnaval)
{
l_int32    i, j, w, h, wplt, sval, vval, nd;
l_uint32   pixel;
l_uint32  *datat, *linet;
void     **lined32;
NUMA      *nasat, *naval;
PIX       *pixt, *pixd;

    PROCNAME("pixMakeHistoSV");

    if (pnasat) *pnasat = NULL;
    if (pnaval) *pnaval = NULL;
    if (!pixs || pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);

    if (pnasat) {
        nasat = numaCreate(256);
        numaSetCount(nasat, 256);
        *pnasat = nasat;
    }
    if (pnaval) {
        naval = numaCreate(256);
        numaSetCount(naval, 256);
        *pnaval = naval;
    }

    if (factor <= 1)
        pixt = pixClone(pixs);
    else
        pixt = pixScaleBySampling(pixs, 1.0 / (l_float32)factor,
                                  1.0 / (l_float32)factor);

        /* Create the hue-value histogram */
    pixd = pixCreate(256, 256, 32);
    lined32 = pixGetLinePtrs(pixd, NULL);
    pixGetDimensions(pixt, &w, &h, NULL);
    datat = pixGetData(pixt);
    wplt = pixGetWpl(pixt);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        for (j = 0; j < w; j++) {
            pixel = linet[j];
            sval = (pixel >> L_GREEN_SHIFT) & 0xff;
            vval = (pixel >> L_BLUE_SHIFT) & 0xff;
            if (pnasat)
                numaShiftValue(nasat, sval, 1.0);
            if (pnaval)
                numaShiftValue(naval, vval, 1.0);
            nd = GET_DATA_FOUR_BYTES(lined32[sval], vval);
            SET_DATA_FOUR_BYTES(lined32[sval], vval, nd + 1);
        }
    }

    FREE(lined32);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixFindHistoPeaksHSV()
 *
 *      Input:  pixs (32 bpp; HS, HV or SV histogram; not changed)
 *              type (L_HS_HISTO, L_HV_HISTO or L_SV_HISTO)
 *              width (half width of sliding window)
 *              height (half height of sliding window)
 *              npeaks (number of peaks to look for)
 *              erasefactor (ratio of erase window size to sliding window size)
 *              &pta (locations of maximum for each integrated peak area)
 *              &natot (integrated peak areas)
 *              &pixa (<optional return> pixa for debugging; NULL to skip)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) pixs is a 32 bpp histogram in a pair of HSV colorspace.  It
 *          should be thought of as a single sample with 32 bps (bits/sample).
 *      (2) After each peak is found, the peak is erased with a window
 *          that is centered on the peak and scaled from the sliding
 *          window by @erasefactor.  Typically, @erasefactor is chosen
 *          to be > 1.0.
 *      (3) Data for a maximum of @npeaks is returned in @pta and @natot.
 *      (4) For debugging, after the pixa is returned, display with:
 *          pixd = pixaDisplayTiledInRows(pixa, 32, 1000, 1.0, 0, 30, 2);
 */
l_int32
pixFindHistoPeaksHSV(PIX       *pixs,
                     l_int32    type,
                     l_int32    width,
                     l_int32    height,
                     l_int32    npeaks,
                     l_float32  erasefactor,
                     PTA      **ppta,
                     NUMA     **pnatot,
                     PIXA     **ppixa)
{
l_int32   i, xmax, ymax, ewidth, eheight;
l_uint32  maxval;
BOX      *box;
NUMA     *natot;
PIX      *pixh, *pixw, *pixt1, *pixt2, *pixt3;
PTA      *pta;

    PROCNAME("pixFindHistoPeaksHSV");

    if (!pixs || pixGetDepth(pixs) != 32)
        return ERROR_INT("pixs undefined or not 32 bpp", procName, 1);
    if (!ppta || !pnatot)
        return ERROR_INT("&pta and &natot not both defined", procName, 1);
    if (type != L_HS_HISTO && type != L_HV_HISTO && type != L_SV_HISTO)
        return ERROR_INT("invalid HSV histo type", procName, 1);

    if ((pta = ptaCreate(npeaks)) == NULL)
        return ERROR_INT("pta not made", procName, 1);
    *ppta = pta;
    if ((natot = numaCreate(npeaks)) == NULL)
        return ERROR_INT("natot not made", procName, 1);
    *pnatot = natot;

    *ppta = pta;
    if (type == L_SV_HISTO)
        pixh = pixAddMirroredBorder(pixs, width + 1, width + 1, height + 1,
                                    height + 1);
    else  /* type == L_HS_HISTO or type == L_HV_HISTO */
        pixh = pixAddMixedBorder(pixs, width + 1, width + 1, height + 1,
                                 height + 1);

        /* Get the total count in the sliding window.  If the window
         * fully covers the peak, this will be the integrated
         * volume under the peak. */
    pixw = pixWindowedMean(pixh, width, height, 1, 0);
    pixDestroy(&pixh);

        /* Sequentially identify and erase peaks in the histogram.
         * If requested for debugging, save a pixa of the sequence of
         * false color histograms. */
    if (ppixa)
        *ppixa = pixaCreate(0);
    for (i = 0; i < npeaks; i++) {
        pixGetMaxValueInRect(pixw, NULL, &maxval, &xmax, &ymax);
        if (maxval == 0) break;
        numaAddNumber(natot, maxval);
        ptaAddPt(pta, xmax, ymax);
        ewidth = (l_int32)(width * erasefactor);
        eheight = (l_int32)(height * erasefactor);
        box = boxCreate(xmax - ewidth, ymax - eheight, 2 * ewidth + 1,
                        2 * eheight + 1);

        if (ppixa) {
            pixt1 = pixMaxDynamicRange(pixw, L_LINEAR_SCALE);
            pixaAddPix(*ppixa, pixt1, L_INSERT);
            pixt2 = pixConvertGrayToFalseColor(pixt1, 1.0);
            pixaAddPix(*ppixa, pixt2, L_INSERT);
            pixt1 = pixMaxDynamicRange(pixw, L_LOG_SCALE);
            pixt2 = pixConvertGrayToFalseColor(pixt1, 1.0);
            pixaAddPix(*ppixa, pixt2, L_INSERT);
            pixt3 = pixConvertTo32(pixt1);
            pixRenderHashBoxArb(pixt3, box, 6, 2, L_NEG_SLOPE_LINE,
                                1, 255, 100, 100);
            pixaAddPix(*ppixa, pixt3, L_INSERT);
            pixDestroy(&pixt1);
        }

        pixClearInRect(pixw, box);
        boxDestroy(&box);
        if (type == L_HS_HISTO || type == L_HV_HISTO) {
                /* clear wraps at bottom and top */
            if (ymax - eheight < 0) {  /* overlap to bottom */
                box = boxCreate(xmax - ewidth, 240 + ymax - eheight,
                                2 * ewidth + 1, eheight - ymax);
            } else if (ymax + eheight > 239) {  /* overlap to top */
                box = boxCreate(xmax - ewidth, 0, 2 * ewidth + 1,
                                ymax + eheight - 239);
            } else {
                box = NULL;
            }
            if (box) {
                pixClearInRect(pixw, box);
                boxDestroy(&box);
            }
        }
    }

    pixDestroy(&pixw);
    return 0;
}


/*!
 *  displayHSVColorRange()
 *
 *      Input:  hval (hue center value; in range [0 ... 240]
 *              sval (saturation center value; in range [0 ... 255]
 *              vval (max intensity value; in range [0 ... 255]
 *              huehw (half-width of hue range; > 0)
 *              sathw (half-width of saturation range; > 0)
 *              nsamp (number of samplings in each half-width in hue and sat)
 *              factor (linear size of each color square, in pixels; > 3)
 *      Return: pixd (32 bpp set of color squares over input range),
 *                     or null on error
 *
 *  Notes:
 *      (1) The total number of color samplings in each of the hue
 *          and saturation directions is 2 * nsamp + 1.
 */
PIX *
displayHSVColorRange(l_int32  hval,
                     l_int32  sval,
                     l_int32  vval,
                     l_int32  huehw,
                     l_int32  sathw,
                     l_int32  nsamp,
                     l_int32  factor)
{
l_int32  i, j, w, huedelta, satdelta, hue, sat, rval, gval, bval;
PIX     *pixt, *pixd;

    PROCNAME("displayHSVColorRange");

    if (hval < 0 || hval > 240)
        return (PIX *)ERROR_PTR("invalid hval", procName, NULL);
    if (huehw < 5 || huehw > 120)
        return (PIX *)ERROR_PTR("invalid huehw", procName, NULL);
    if (sval - sathw < 0 || sval + sathw > 255)
        return (PIX *)ERROR_PTR("invalid sval/sathw", procName, NULL);
    if (nsamp < 1 || factor < 3)
        return (PIX *)ERROR_PTR("invalid nsamp or rep. factor", procName, NULL);
    if (vval < 0 || vval > 255)
        return (PIX *)ERROR_PTR("invalid vval", procName, NULL);

    w = (2 * nsamp + 1);
    huedelta = (l_int32)((l_float32)huehw / (l_float32)nsamp);
    satdelta = (l_int32)((l_float32)sathw / (l_float32)nsamp);
    pixt = pixCreate(w, w, 32);
    for (i = 0; i < w; i++) {
        hue = hval + huedelta * (i - nsamp);
        if (hue < 0) hue += 240;
        if (hue >= 240) hue -= 240;
        for (j = 0; j < w; j++) {
            sat = sval + satdelta * (j - nsamp);
            convertHSVToRGB(hue, sat, vval, &rval, &gval, &bval);
            pixSetRGBPixel(pixt, j, i, rval, gval, bval);
        }
    }

    pixd = pixExpandReplicate(pixt, factor);
    pixDestroy(&pixt);
    return pixd;
}


/*---------------------------------------------------------------------------*
 *                Colorspace conversion between RGB and YUV                  *
 *---------------------------------------------------------------------------*/
/*!
 *  pixConvertRGBToYUV()
 *
 *      Input:  pixd (can be NULL; if not NULL, must == pixs)
 *              pixs
 *      Return: pixd always
 *
 *  Notes:
 *      (1) For pixs = pixd, this is in-place; otherwise pixd must be NULL.
 *      (2) The Y, U and V values are stored in the same places as
 *          the r, g and b values, respectively.  Here, they are explicitly
 *          placed in the 3 MS bytes in the pixel.
 *      (3) Normalizing to 1 and considering the r,g,b components,
 *          a simple way to understand the YUV space is:
 *           - Y = weighted sum of (r,g,b)
 *           - U = weighted difference between Y and B
 *           - V = weighted difference between Y and R
 *      (4) Following video conventions, Y, U and V are in the range:
 *             Y: [16, 235]
 *             U: [16, 240]
 *             V: [16, 240]
 *      (5) For the coefficients in the transform matrices, see eq. 4 in
 *          "Frequently Asked Questions about Color" by Charles Poynton,
 *          http://www.poynton.com/notes/colour_and_gamma/ColorFAQ.html
 */
PIX *
pixConvertRGBToYUV(PIX  *pixd,
                   PIX  *pixs)
{
l_int32    w, h, d, wpl, i, j, rval, gval, bval, yval, uval, vval;
l_uint32  *line, *data;
PIXCMAP   *cmap;

    PROCNAME("pixConvertRGBToYUV");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixd && pixd != pixs)
        return (PIX *)ERROR_PTR("pixd defined and not inplace", procName, pixd);

    d = pixGetDepth(pixs);
    cmap = pixGetColormap(pixs);
    if (!cmap && d != 32)
        return (PIX *)ERROR_PTR("not cmapped or rgb", procName, pixd);

    if (!pixd)
        pixd = pixCopy(NULL, pixs);

    cmap = pixGetColormap(pixd);
    if (cmap) {   /* just convert the colormap */
        pixcmapConvertRGBToYUV(cmap);
        return pixd;
    }

        /* Convert RGB image */
    pixGetDimensions(pixd, &w, &h, NULL);
    wpl = pixGetWpl(pixd);
    data = pixGetData(pixd);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < w; j++) {
            extractRGBValues(line[j], &rval, &gval, &bval);
            convertRGBToYUV(rval, gval, bval, &yval, &uval, &vval);
            line[j] = (yval << 24) | (uval << 16) | (vval << 8);
        }
    }

    return pixd;
}


/*!
 *  pixConvertYUVToRGB()
 *
 *      Input:  pixd (can be NULL; if not NULL, must == pixs)
 *              pixs
 *      Return: pixd always
 *
 *  Notes:
 *      (1) For pixs = pixd, this is in-place; otherwise pixd must be NULL.
 *      (2) The user takes responsibility for making sure that pixs is
 *          in YUV space.
 *      (3) The Y, U and V values are stored in the same places as
 *          the r, g and b values, respectively.  Here, they are explicitly
 *          placed in the 3 MS bytes in the pixel.
 */
PIX *
pixConvertYUVToRGB(PIX  *pixd,
                   PIX  *pixs)
{
l_int32    w, h, d, wpl, i, j, rval, gval, bval, yval, uval, vval;
l_uint32   pixel;
l_uint32  *line, *data;
PIXCMAP   *cmap;

    PROCNAME("pixConvertYUVToRGB");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixd && pixd != pixs)
        return (PIX *)ERROR_PTR("pixd defined and not inplace", procName, pixd);

    d = pixGetDepth(pixs);
    cmap = pixGetColormap(pixs);
    if (!cmap && d != 32)
        return (PIX *)ERROR_PTR("not cmapped or hsv", procName, pixd);

    if (!pixd)
        pixd = pixCopy(NULL, pixs);

    cmap = pixGetColormap(pixd);
    if (cmap) {   /* just convert the colormap */
        pixcmapConvertYUVToRGB(cmap);
        return pixd;
    }

        /* Convert YUV image */
    pixGetDimensions(pixd, &w, &h, NULL);
    wpl = pixGetWpl(pixd);
    data = pixGetData(pixd);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < w; j++) {
            pixel = line[j];
            yval = pixel >> 24;
            uval = (pixel >> 16) & 0xff;
            vval = (pixel >> 8) & 0xff;
            convertYUVToRGB(yval, uval, vval, &rval, &gval, &bval);
            composeRGBPixel(rval, gval, bval, line + j);
        }
    }

    return pixd;
}


/*!
 *  convertRGBToYUV()
 *
 *      Input:  rval, gval, bval (RGB input)
 *              &yval, &uval, &vval (<return> YUV values)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The range of returned values is:
 *            Y [16 ... 235]
 *            U [16 ... 240]
 *            V [16 ... 240]
 */
l_int32
convertRGBToYUV(l_int32   rval,
                l_int32   gval,
                l_int32   bval,
                l_int32  *pyval,
                l_int32  *puval,
                l_int32  *pvval)
{
l_float32  norm;

    PROCNAME("convertRGBToYUV");

    if (!pyval || !puval || !pvval)
        return ERROR_INT("&yval, &uval, &vval not all defined", procName, 1);

    norm = 1.0 / 256.;
    *pyval = (l_int32)(16.0 +
                norm * (65.738 * rval + 129.057 * gval + 25.064 * bval) + 0.5);
    *puval = (l_int32)(128.0 +
                norm * (-37.945 * rval -74.494 * gval + 112.439 * bval) + 0.5);
    *pvval = (l_int32)(128.0 +
                norm * (112.439 * rval - 94.154 * gval - 18.285 * bval) + 0.5);
    return 0;
}


/*!
 *  convertYUVToRGB()
 *
 *      Input:  yval, uval, vval
 *              &rval, &gval, &bval (<return> RGB values)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The range of valid input values is:
 *            Y [16 ... 235]
 *            U [16 ... 240]
 *            V [16 ... 240]
 *      (2) Conversion of RGB --> YUV --> RGB leaves the image unchanged.
 *      (3) The YUV gamut is larger than the RBG gamut; many YUV values
 *          will result in an invalid RGB value.  We clip individual
 *          r,g,b components to the range [0, 255], and do not test input.
 */
l_int32
convertYUVToRGB(l_int32   yval,
                l_int32   uval,
                l_int32   vval,
                l_int32  *prval,
                l_int32  *pgval,
                l_int32  *pbval)
{
l_int32    rval, gval, bval;
l_float32  norm, ym, um, vm;

    PROCNAME("convertYUVToRGB");

    if (!prval || !pgval || !pbval)
        return ERROR_INT("&rval, &gval, &bval not all defined", procName, 1);

    norm = 1.0 / 256.;
    ym = yval - 16.0;
    um = uval - 128.0;
    vm = vval - 128.0;
    rval =  (l_int32)(norm * (298.082 * ym + 408.583 * vm) + 0.5);
    gval = (l_int32)(norm * (298.082 * ym - 100.291 * um - 208.120 * vm) +
                       0.5);
    bval = (l_int32)(norm * (298.082 * ym + 516.411 * um) + 0.5);
    *prval = L_MIN(255, L_MAX(0, rval));
    *pgval = L_MIN(255, L_MAX(0, gval));
    *pbval = L_MIN(255, L_MAX(0, bval));

    return 0;
}


/*!
 *  pixcmapConvertRGBToYUV()
 *
 *      Input:  colormap
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      - in-place transform
 *      - See convertRGBToYUV() for def'n of YUV space.
 *      - replaces: r --> y, g --> u, b --> v
 */
l_int32
pixcmapConvertRGBToYUV(PIXCMAP  *cmap)
{
l_int32   i, ncolors, rval, gval, bval, yval, uval, vval;

    PROCNAME("pixcmapConvertRGBToYUV");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    ncolors = pixcmapGetCount(cmap);
    for (i = 0; i < ncolors; i++) {
        pixcmapGetColor(cmap, i, &rval, &gval, &bval);
        convertRGBToYUV(rval, gval, bval, &yval, &uval, &vval);
        pixcmapResetColor(cmap, i, yval, uval, vval);
    }
    return 0;
}


/*!
 *  pixcmapConvertYUVToRGB()
 *
 *      Input:  colormap
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      - in-place transform
 *      - See convertRGBToYUV() for def'n of YUV space.
 *      - replaces: y --> r, u --> g, v --> b
 */
l_int32
pixcmapConvertYUVToRGB(PIXCMAP  *cmap)
{
l_int32   i, ncolors, rval, gval, bval, yval, uval, vval;

    PROCNAME("pixcmapConvertYUVToRGB");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    ncolors = pixcmapGetCount(cmap);
    for (i = 0; i < ncolors; i++) {
        pixcmapGetColor(cmap, i, &yval, &uval, &vval);
        convertYUVToRGB(yval, uval, vval, &rval, &gval, &bval);
        pixcmapResetColor(cmap, i, rval, gval, bval);
    }
    return 0;
}
