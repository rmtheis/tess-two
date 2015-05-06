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
 *  coloring.c
 *
 *      Coloring "gray" pixels
 *           PIX             *pixColorGrayRegions()
 *           l_int32          pixColorGray()
 *
 *      Adjusting one or more colors to a target color
 *           PIX             *pixSnapColor()
 *           PIX             *pixSnapColorCmap()
 *
 *      Piecewise linear color mapping based on a source/target pair
 *           PIX             *pixLinearMapToTargetColor()
 *           l_int32          pixelLinearMapToTargetColor()
 *
 *      Fractional shift of RGB towards black or white
 *           PIX             *pixShiftByComponent()
 *           l_int32          pixelShiftByComponent()
 *           l_int32          pixelFractionalShift()
 *
 *  There are several "coloring" functions in leptonica.
 *  You can find them in these files:
 *       coloring.c
 *       paintcmap.c
 *       pix2.c
 *       blend.c
 *       enhance.c
 *
 *  They fall into the following categories:
 *
 *  (1) Moving either the light or dark pixels toward a
 *      specified color. (pixColorGray)
 *  (2) Forcing all pixels whose color is within some delta of a
 *      specified color to move to that color. (pixSnapColor)
 *  (3) Doing a piecewise linear color shift specified by a source
 *      and a target color.  Each component shifts independently.
 *      (pixLinearMapToTargetColor)
 *  (4) Shifting all colors by a given fraction of their distance
 *      from 0 (if shifting down) or from 255 (if shifting up).
 *      This is useful for colorizing either the background or
 *      the foreground of a grayscale image. (pixShiftByComponent)
 *  (5) Shifting all colors by a component-dependent fraction of
 *      their distance from 0 (if shifting down) or from 255 (if
 *      shifting up).  This is useful for modifying the color to
 *      compensate for color shifts in acquisition, for example
 *      (enhance.c: pixColorShiftRGB).
 *  (6) Repainting selected pixels. (paintcmap.c: pixSetSelectMaskedCmap)
 *  (7) Blending a fraction of a specific color with the existing RGB
 *      color.  (pix2.c: pixBlendInRect())
 *  (8) Changing selected colors in a colormap.
 *      (paintcmap.c: pixSetSelectCmap, pixSetSelectMaskedCmap)
 *  (9) Shifting all the pixels towards black or white depending on
 *      the gray value of a second image.  (blend.c: pixFadeWithGray)
 *  (10) Changing the hue, saturation or brightness, by changing the
 *      appropriate parameter in HSV color space by a fraction of
 *      the distance toward its end-point.  For example, you can change
 *      the brightness by moving each pixel's v-parameter a specified
 *      fraction of the distance toward 0 (darkening) or toward 255
 *      (brightening).  (enhance.c: pixModifySaturation,
 *      pixModifyHue, pixModifyBrightness)
 */

#include "allheaders.h"


/*---------------------------------------------------------------------*
 *                        Coloring "gray" pixels                       *
 *---------------------------------------------------------------------*/
/*!
 *  pixColorGrayRegions()
 *
 *      Input:  pixs (2, 4 or 8 bpp gray, rgb, or colormapped)
 *              boxa (of regions in which to apply color)
 *              type (L_PAINT_LIGHT, L_PAINT_DARK)
 *              thresh (average value below/above which pixel is unchanged)
 *              rval, gval, bval (new color to paint)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This generates a new image, where some of the pixels in each
 *          box in the boxa are colorized.  See pixColorGray() for usage
 *          with @type and @thresh.  Note that @thresh is only used for
 *          rgb; it is ignored for colormapped images.
 *      (2) If the input image is colormapped, the new image will be 8 bpp
 *          colormapped if possible; otherwise, it will be converted
 *          to 32 bpp rgb.  Only pixels that are strictly gray will be
 *          colorized.
 *      (3) If the input image is not colormapped, it is converted to rgb.
 *          A "gray" value for a pixel is determined by averaging the
 *          components, and the output rgb value is determined from this.
 *      (4) This can be used in conjunction with pixFindColorRegions() to
 *          add highlight color to a grayscale image.
 */
PIX *
pixColorGrayRegions(PIX     *pixs,
                    BOXA    *boxa,
                    l_int32  type,
                    l_int32  thresh,
                    l_int32  rval,
                    l_int32  gval,
                    l_int32  bval)
{
l_int32   i, n, ncolors, ngray;
BOX      *box;
PIX      *pixd;
PIXCMAP  *cmap;

    PROCNAME("pixColorGrayRegions");

    if (!pixs || pixGetDepth(pixs) == 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (!boxa)
        return (PIX *)ERROR_PTR("boxa not defined", procName, NULL);
    if (type != L_PAINT_LIGHT && type != L_PAINT_DARK)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);

        /* If cmapped and there is room in an 8 bpp colormap for
         * expansion, convert pixs to 8 bpp, and colorize. */
    cmap = pixGetColormap(pixs);
    if (cmap) {
        ncolors = pixcmapGetCount(cmap);
        pixcmapCountGrayColors(cmap, &ngray);
        if (ncolors + ngray < 255) {
            pixd = pixConvertTo8(pixs, 1);  /* always new image */
            pixColorGrayRegionsCmap(pixd, boxa, type, rval, gval, bval);
            return pixd;
        }
    }

        /* The output will be rgb.  Make sure the thresholds are valid */
    if (type == L_PAINT_LIGHT) {  /* thresh should be low */
        if (thresh >= 255)
            return (PIX *)ERROR_PTR("thresh must be < 255", procName, NULL);
        if (thresh > 127)
            L_WARNING("threshold set very high\n", procName);
    } else {  /* type == L_PAINT_DARK; thresh should be high */
        if (thresh <= 0)
            return (PIX *)ERROR_PTR("thresh must be > 0", procName, NULL);
        if (thresh < 128)
            L_WARNING("threshold set very low\n", procName);
    }

    pixd = pixConvertTo32(pixs);  /* always new image */
    n = boxaGetCount(boxa);
    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        pixColorGray(pixd, box, type, thresh, rval, gval, bval);
        boxDestroy(&box);
    }

    return pixd;
}


/*!
 *  pixColorGray()
 *
 *      Input:  pixs (8 bpp gray, rgb or colormapped image)
 *              box (<optional> region in which to apply color; can be NULL)
 *              type (L_PAINT_LIGHT, L_PAINT_DARK)
 *              thresh (average value below/above which pixel is unchanged)
 *              rval, gval, bval (new color to paint)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This is an in-place operation; pixs is modified.
 *          If pixs is colormapped, the operation will add colors to the
 *          colormap.  Otherwise, pixs will be converted to 32 bpp rgb if
 *          it is initially 8 bpp gray.
 *      (2) If type == L_PAINT_LIGHT, it colorizes non-black pixels,
 *          preserving antialiasing.
 *          If type == L_PAINT_DARK, it colorizes non-white pixels,
 *          preserving antialiasing.
 *      (3) If box is NULL, applies function to the entire image; otherwise,
 *          clips the operation to the intersection of the box and pix.
 *      (4) If colormapped, calls pixColorGrayCmap(), which applies the
 *          coloring algorithm only to pixels that are strictly gray.
 *      (5) For RGB, determines a "gray" value by averaging; then uses this
 *          value, plus the input rgb target, to generate the output
 *          pixel values.
 *      (6) thresh is only used for rgb; it is ignored for colormapped pix.
 *          If type == L_PAINT_LIGHT, use thresh = 0 if all pixels are to
 *          be colored (black pixels will be unaltered).
 *          In situations where there are a lot of black pixels,
 *          setting thresh > 0 will make the function considerably
 *          more efficient without affecting the final result.
 *          If type == L_PAINT_DARK, use thresh = 255 if all pixels
 *          are to be colored (white pixels will be unaltered).
 *          In situations where there are a lot of white pixels,
 *          setting thresh < 255 will make the function considerably
 *          more efficient without affecting the final result.
 */
l_int32
pixColorGray(PIX     *pixs,
             BOX     *box,
             l_int32  type,
             l_int32  thresh,
             l_int32  rval,
             l_int32  gval,
             l_int32  bval)
{
l_int32    i, j, w, h, d, wpl, x1, x2, y1, y2, bw, bh;
l_int32    nrval, ngval, nbval, aveval;
l_float32  factor;
l_uint32   val32;
l_uint32  *line, *data;
PIX       *pixt;
PIXCMAP   *cmap;

    PROCNAME("pixColorGray");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (type != L_PAINT_LIGHT && type != L_PAINT_DARK)
        return ERROR_INT("invalid type", procName, 1);

    cmap = pixGetColormap(pixs);
    pixGetDimensions(pixs, &w, &h, &d);
    if (!cmap && d != 8 && d != 32)
        return ERROR_INT("pixs not cmapped, 8 bpp or rgb", procName, 1);
    if (cmap)
        return pixColorGrayCmap(pixs, box, type, rval, gval, bval);

        /* rgb or 8 bpp gray image; check the thresh */
    if (type == L_PAINT_LIGHT) {  /* thresh should be low */
        if (thresh >= 255)
            return ERROR_INT("thresh must be < 255; else this is a no-op",
                             procName, 1);
        if (thresh > 127)
            L_WARNING("threshold set very high\n", procName);
    } else {  /* type == L_PAINT_DARK; thresh should be high */
        if (thresh <= 0)
            return ERROR_INT("thresh must be > 0; else this is a no-op",
                             procName, 1);
        if (thresh < 128)
            L_WARNING("threshold set very low\n", procName);
    }

        /* In-place conversion to 32 bpp if necessary */
    if (d == 8) {
        pixt = pixConvertTo32(pixs);
        pixTransferAllData(pixs, &pixt, 1, 0);
    }

    if (!box) {
        x1 = y1 = 0;
        x2 = w;
        y2 = h;
    } else {
        boxGetGeometry(box, &x1, &y1, &bw, &bh);
        x2 = x1 + bw - 1;
        y2 = y1 + bh - 1;
    }

    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    factor = 1. / 255.;
    for (i = y1; i <= y2; i++) {
        if (i < 0 || i >= h)
            continue;
        line = data + i * wpl;
        for (j = x1; j <= x2; j++) {
            if (j < 0 || j >= w)
                continue;
            val32 = *(line + j);
            aveval = ((val32 >> 24) + ((val32 >> 16) & 0xff) +
                      ((val32 >> 8) & 0xff)) / 3;
            if (type == L_PAINT_LIGHT) {
                if (aveval < thresh)  /* skip sufficiently dark pixels */
                    continue;
                nrval = (l_int32)(rval * aveval * factor);
                ngval = (l_int32)(gval * aveval * factor);
                nbval = (l_int32)(bval * aveval * factor);
            } else {  /* type == L_PAINT_DARK */
                if (aveval > thresh)  /* skip sufficiently light pixels */
                    continue;
                nrval = rval + (l_int32)((255. - rval) * aveval * factor);
                ngval = gval + (l_int32)((255. - gval) * aveval * factor);
                nbval = bval + (l_int32)((255. - bval) * aveval * factor);
            }
            composeRGBPixel(nrval, ngval, nbval, &val32);
            *(line + j) = val32;
        }
    }

    return 0;
}


/*------------------------------------------------------------------*
 *            Adjusting one or more colors to a target color        *
 *------------------------------------------------------------------*/
/*!
 *  pixSnapColor()
 *
 *      Input:  pixd (<optional>; either NULL or equal to pixs for in-place)
 *              pixs (colormapped or 8 bpp gray or 32 bpp rgb)
 *              srcval (color center to be selected for change: 0xrrggbb00)
 *              dstval (target color for pixels: 0xrrggbb00)
 *              diff (max absolute difference, applied to all components)
 *      Return: pixd (with all pixels within diff of pixval set to pixval),
 *                    or pixd on error
 *
 *  Notes:
 *      (1) For inplace operation, call it this way:
 *           pixSnapColor(pixs, pixs, ... )
 *      (2) For generating a new pixd:
 *           pixd = pixSnapColor(NULL, pixs, ...)
 *      (3) If pixs has a colormap, it is handled by pixSnapColorCmap().
 *      (4) All pixels within 'diff' of 'srcval', componentwise,
 *          will be changed to 'dstval'.
 */
PIX *
pixSnapColor(PIX      *pixd,
             PIX      *pixs,
             l_uint32  srcval,
             l_uint32  dstval,
             l_int32   diff)
{
l_int32    val, sval, dval;
l_int32    rval, gval, bval, rsval, gsval, bsval;
l_int32    i, j, w, h, d, wpl;
l_uint32   pixel;
l_uint32  *line, *data;

    PROCNAME("pixSnapColor");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixd && (pixd != pixs))
        return (PIX *)ERROR_PTR("pixd not null or == pixs", procName, pixd);

    if (pixGetColormap(pixs))
        return pixSnapColorCmap(pixd, pixs, srcval, dstval, diff);

        /* pixs does not have a colormap; it must be 8 bpp gray or
         * 32 bpp rgb. */
    if (pixGetDepth(pixs) < 8)
        return (PIX *)ERROR_PTR("pixs is < 8 bpp", procName, pixd);

        /* Do the work on pixd */
    if (!pixd)
        pixd = pixCopy(NULL, pixs);

    pixGetDimensions(pixd, &w, &h, &d);
    data = pixGetData(pixd);
    wpl = pixGetWpl(pixd);
    if (d == 8) {
        sval = srcval & 0xff;
        dval = dstval & 0xff;
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            for (j = 0; j < w; j++) {
                val = GET_DATA_BYTE(line, j);
                if (L_ABS(val - sval) <= diff)
                    SET_DATA_BYTE(line, j, dval);
            }
        }
    } else {  /* d == 32 */
        extractRGBValues(srcval, &rsval, &gsval, &bsval);
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            for (j = 0; j < w; j++) {
                pixel = *(line + j);
                extractRGBValues(pixel, &rval, &gval, &bval);
                if ((L_ABS(rval - rsval) <= diff) &&
                    (L_ABS(gval - gsval) <= diff) &&
                    (L_ABS(bval - bsval) <= diff))
                    *(line + j) = dstval;  /* replace */
            }
        }
    }

    return pixd;
}


/*!
 *  pixSnapColorCmap()
 *
 *      Input:  pixd (<optional>; either NULL or equal to pixs for in-place)
 *              pixs (colormapped)
 *              srcval (color center to be selected for change: 0xrrggbb00)
 *              dstval (target color for pixels: 0xrrggbb00)
 *              diff (max absolute difference, applied to all components)
 *      Return: pixd (with all pixels within diff of srcval set to dstval),
 *                    or pixd on error
 *
 *  Notes:
 *      (1) For inplace operation, call it this way:
 *           pixSnapCcmap(pixs, pixs, ... )
 *      (2) For generating a new pixd:
 *           pixd = pixSnapCmap(NULL, pixs, ...)
 *      (3) pixs must have a colormap.
 *      (4) All colors within 'diff' of 'srcval', componentwise,
 *          will be changed to 'dstval'.
 */
PIX *
pixSnapColorCmap(PIX      *pixd,
                 PIX      *pixs,
                 l_uint32  srcval,
                 l_uint32  dstval,
                 l_int32   diff)
{
l_int32    i, ncolors, index, found;
l_int32    rval, gval, bval, rsval, gsval, bsval, rdval, gdval, bdval;
l_int32   *tab;
PIX       *pixm;
PIXCMAP   *cmap;

    PROCNAME("pixSnapColorCmap");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (!pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("cmap not found", procName, pixd);
    if (pixd && (pixd != pixs))
        return (PIX *)ERROR_PTR("pixd not null or == pixs", procName, pixd);

    if (!pixd)
        pixd = pixCopy(NULL, pixs);

        /* If no free colors, look for one close to the target
         * that can be commandeered. */
    cmap = pixGetColormap(pixd);
    ncolors = pixcmapGetCount(cmap);
    extractRGBValues(srcval, &rsval, &gsval, &bsval);
    extractRGBValues(dstval, &rdval, &gdval, &bdval);
    found = FALSE;
    if (pixcmapGetFreeCount(cmap) == 0) {
        for (i = 0; i < ncolors; i++) {
            pixcmapGetColor(cmap, i, &rval, &gval, &bval);
            if ((L_ABS(rval - rsval) <= diff) &&
                (L_ABS(gval - gsval) <= diff) &&
                (L_ABS(bval - bsval) <= diff)) {
                index = i;
                pixcmapResetColor(cmap, index, rdval, gdval, bdval);
                found = TRUE;
                break;
            }
        }
    } else {  /* just add the new color */
        pixcmapAddColor(cmap, rdval, gdval, bdval);
        ncolors = pixcmapGetCount(cmap);
        index = ncolors - 1;  /* index of new destination color */
        found = TRUE;
    }

    if (!found) {
        L_INFO("nothing to do\n", procName);
        return pixd;
    }

        /* For each color in cmap that is close enough to srcval,
         * set the tab value to 1.  Then generate a 1 bpp mask with
         * fg pixels for every pixel in pixd that is close enough
         * to srcval (i.e., has value 1 in tab). */
    if ((tab = (l_int32 *)CALLOC(256, sizeof(l_int32))) == NULL)
        return (PIX *)ERROR_PTR("tab not made", procName, pixd);
    for (i = 0; i < ncolors; i++) {
        pixcmapGetColor(cmap, i, &rval, &gval, &bval);
        if ((L_ABS(rval - rsval) <= diff) &&
            (L_ABS(gval - gsval) <= diff) &&
            (L_ABS(bval - bsval) <= diff))
            tab[i] = 1;
    }
    pixm = pixMakeMaskFromLUT(pixd, tab);
    FREE(tab);

        /* Use the binary mask to set all selected pixels to
         * the dest color index. */
    pixSetMasked(pixd, pixm, dstval);
    pixDestroy(&pixm);

        /* Remove all unused colors from the colormap. */
    pixRemoveUnusedColors(pixd);

    return pixd;
}


/*---------------------------------------------------------------------*
 *     Piecewise linear color mapping based on a source/target pair    *
 *---------------------------------------------------------------------*/
/*!
 *  pixLinearMapToTargetColor()
 *
 *      Input:  pixd (<optional>; either NULL or equal to pixs for in-place)
 *              pixs (32 bpp rgb)
 *              srcval (source color: 0xrrggbb00)
 *              dstval (target color: 0xrrggbb00)
 *      Return: pixd (with all pixels mapped based on the srcval/destval
 *                    mapping), or pixd on error
 *
 *  Notes:
 *      (1) For each component (r, b, g) separately, this does a piecewise
 *          linear mapping of the colors in pixs to colors in pixd.
 *          If rs and rd are the red src and dest components in @srcval and
 *          @dstval, then the range [0 ... rs] in pixs is mapped to
 *          [0 ... rd] in pixd.  Likewise, the range [rs ... 255] in pixs
 *          is mapped to [rd ... 255] in pixd.  And similarly for green
 *          and blue.
 *      (2) The mapping will in general change the hue of the pixels.
 *          However, if the src and dst targets are related by
 *          a transformation given by pixelFractionalShift(), the hue
 *          is invariant.
 *      (3) For inplace operation, call it this way:
 *            pixLinearMapToTargetColor(pixs, pixs, ... )
 *      (4) For generating a new pixd:
 *            pixd = pixLinearMapToTargetColor(NULL, pixs, ...)
 */
PIX *
pixLinearMapToTargetColor(PIX      *pixd,
                          PIX      *pixs,
                          l_uint32  srcval,
                          l_uint32  dstval)
{
l_int32    i, j, w, h, wpl;
l_int32    rval, gval, bval, rsval, gsval, bsval, rdval, gdval, bdval;
l_int32   *rtab, *gtab, *btab;
l_uint32   pixel;
l_uint32  *line, *data;

    PROCNAME("pixLinearMapToTargetColor");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixd && (pixd != pixs))
        return (PIX *)ERROR_PTR("pixd not null or == pixs", procName, pixd);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs is not 32 bpp", procName, pixd);

        /* Do the work on pixd */
    if (!pixd)
        pixd = pixCopy(NULL, pixs);

    extractRGBValues(srcval, &rsval, &gsval, &bsval);
    extractRGBValues(dstval, &rdval, &gdval, &bdval);
    rsval = L_MIN(254, L_MAX(1, rsval));
    gsval = L_MIN(254, L_MAX(1, gsval));
    bsval = L_MIN(254, L_MAX(1, bsval));
    rtab = (l_int32 *)CALLOC(256, sizeof(l_int32));
    gtab = (l_int32 *)CALLOC(256, sizeof(l_int32));
    btab = (l_int32 *)CALLOC(256, sizeof(l_int32));
    for (i = 0; i < 256; i++) {
        if (i <= rsval)
            rtab[i] = (i * rdval) / rsval;
        else
            rtab[i] = rdval + ((255 - rdval) * (i - rsval)) / (255 - rsval);
        if (i <= gsval)
            gtab[i] = (i * gdval) / gsval;
        else
            gtab[i] = gdval + ((255 - gdval) * (i - gsval)) / (255 - gsval);
        if (i <= bsval)
            btab[i] = (i * bdval) / bsval;
        else
            btab[i] = bdval + ((255 - bdval) * (i - bsval)) / (255 - bsval);
    }
    pixGetDimensions(pixd, &w, &h, NULL);
    data = pixGetData(pixd);
    wpl = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < w; j++) {
            pixel = line[j];
            extractRGBValues(pixel, &rval, &gval, &bval);
            composeRGBPixel(rtab[rval], gtab[gval], btab[bval], &pixel);
            line[j] = pixel;
        }
    }

    FREE(rtab);
    FREE(gtab);
    FREE(btab);
    return pixd;
}


/*!
 *  pixelLinearMapToTargetColor()
 *
 *      Input:  scolor (rgb source color: 0xrrggbb00)
 *              srcmap (source mapping color: 0xrrggbb00)
 *              dstmap (target mapping color: 0xrrggbb00)
 *              &pdcolor (<return> rgb dest color: 0xrrggbb00)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This does this does a piecewise linear mapping of each
 *          component of @scolor to @dcolor, based on the relation
 *          between the components of @srcmap and @dstmap.  It is the
 *          same transformation, performed on a single color, as mapped
 *          on every pixel in a pix by pixLinearMapToTargetColor().
 *      (2) For each component, if the sval is larger than the smap,
 *          the dval will be pushed up from dmap towards white.
 *          Otherwise, dval will be pushed down from dmap towards black.
 *          This is because you can visualize the transformation as
 *          a linear stretching where smap moves to dmap, and everything
 *          else follows linearly with 0 and 255 fixed.
 *      (3) The mapping will in general change the hue of @scolor.
 *          However, if the @srcmap and @dstmap targets are related by
 *          a transformation given by pixelFractionalShift(), the hue
 *          will be invariant.
 */
l_int32
pixelLinearMapToTargetColor(l_uint32   scolor,
                            l_uint32   srcmap,
                            l_uint32   dstmap,
                            l_uint32  *pdcolor)
{
l_int32    srval, sgval, sbval, drval, dgval, dbval;
l_int32    srmap, sgmap, sbmap, drmap, dgmap, dbmap;

    PROCNAME("pixelLinearMapToTargetColor");

    if (!pdcolor)
        return ERROR_INT("&dcolor not defined", procName, 1);
    *pdcolor = 0;

    extractRGBValues(scolor, &srval, &sgval, &sbval);
    extractRGBValues(srcmap, &srmap, &sgmap, &sbmap);
    extractRGBValues(dstmap, &drmap, &dgmap, &dbmap);
    srmap = L_MIN(254, L_MAX(1, srmap));
    sgmap = L_MIN(254, L_MAX(1, sgmap));
    sbmap = L_MIN(254, L_MAX(1, sbmap));

    if (srval <= srmap)
        drval = (srval * drmap) / srmap;
    else
        drval = drmap + ((255 - drmap) * (srval - srmap)) / (255 - srmap);
    if (sgval <= sgmap)
        dgval = (sgval * dgmap) / sgmap;
    else
        dgval = dgmap + ((255 - dgmap) * (sgval - sgmap)) / (255 - sgmap);
    if (sbval <= sbmap)
        dbval = (sbval * dbmap) / sbmap;
    else
        dbval = dbmap + ((255 - dbmap) * (sbval - sbmap)) / (255 - sbmap);

    composeRGBPixel(drval, dgval, dbval, pdcolor);
    return 0;
}


/*------------------------------------------------------------------*
 *          Fractional shift of RGB towards black or white          *
 *------------------------------------------------------------------*/
/*!
 *  pixShiftByComponent()
 *
 *      Input:  pixd (<optional>; either NULL or equal to pixs for in-place)
 *              pixs (32 bpp rgb)
 *              srcval (source color: 0xrrggbb00)
 *              dstval (target color: 0xrrggbb00)
 *      Return: pixd (with all pixels mapped based on the srcval/destval
 *                    mapping), or pixd on error
 *
 *  Notes:
 *      (1) For each component (r, b, g) separately, this does a linear
 *          mapping of the colors in pixs to colors in pixd.
 *          Let rs and rd be the red src and dest components in @srcval and
 *          @dstval, and rval is the red component of the src pixel.
 *          Then for all pixels in pixs, the mapping for the red
 *          component from pixs to pixd is:
 *             if (rd <= rs)   (shift toward black)
 *                 rval --> (rd/rs) * rval
 *             if (rd > rs)    (shift toward white)
 *                (255 - rval) --> ((255 - rs)/(255 - rd)) * (255 - rval)
 *          Thus if rd <= rs, the red component of all pixels is
 *          mapped by the same fraction toward white, and if rd > rs,
 *          they are mapped by the same fraction toward black.
 *          This is essentially a different linear TRC (gamma = 1)
 *          for each component.  The source and target color inputs are
 *          just used to generate the three fractions.
 *      (2) Note that this mapping differs from that in
 *          pixLinearMapToTargetColor(), which maps rs --> rd and does
 *          a piecewise stretching in between.
 *      (3) For inplace operation, call it this way:
 *            pixFractionalShiftByComponent(pixs, pixs, ... )
 *      (4) For generating a new pixd:
 *            pixd = pixLinearMapToTargetColor(NULL, pixs, ...)
 *      (5) A simple application is to color a grayscale image.
 *          A light background can be colored using srcval = 0xffffff00
 *          and picking a target background color for dstval.
 *          A dark foreground can be colored by using srcval = 0x0
 *          and choosing a target foreground color for dstval.
 */
PIX *
pixShiftByComponent(PIX      *pixd,
                    PIX      *pixs,
                    l_uint32  srcval,
                    l_uint32  dstval)
{
l_int32    i, j, w, h, wpl;
l_int32    rval, gval, bval, rsval, gsval, bsval, rdval, gdval, bdval;
l_int32   *rtab, *gtab, *btab;
l_uint32   pixel;
l_uint32  *line, *data;
PIXCMAP   *cmap;

    PROCNAME("pixShiftByComponent");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixd && (pixd != pixs))
        return (PIX *)ERROR_PTR("pixd not null or == pixs", procName, pixd);
    if (pixGetDepth(pixs) != 32 && !pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs not cmapped or 32 bpp", procName, pixd);

        /* Do the work on pixd */
    if (!pixd)
        pixd = pixCopy(NULL, pixs);

        /* If colormapped, just modify it */
    if ((cmap = pixGetColormap(pixd)) != NULL) {
        pixcmapShiftByComponent(cmap, srcval, dstval);
        return pixd;
    }

    extractRGBValues(srcval, &rsval, &gsval, &bsval);
    extractRGBValues(dstval, &rdval, &gdval, &bdval);
    rtab = (l_int32 *)CALLOC(256, sizeof(l_int32));
    gtab = (l_int32 *)CALLOC(256, sizeof(l_int32));
    btab = (l_int32 *)CALLOC(256, sizeof(l_int32));
    for (i = 0; i < 256; i++) {
        if (rdval == rsval)
            rtab[i] = i;
        else if (rdval < rsval)
            rtab[i] = (i * rdval) / rsval;
        else
            rtab[i] = 255 - (255 - rdval) * (255 - i) / (255 - rsval);
        if (gdval == gsval)
            gtab[i] = i;
        else if (gdval < gsval)
            gtab[i] = (i * gdval) / gsval;
        else
            gtab[i] = 255 - (255 - gdval) * (255 - i) / (255 - gsval);
        if (bdval == bsval)
            btab[i] = i;
        else if (bdval < bsval)
            btab[i] = (i * bdval) / bsval;
        else
            btab[i] = 255 - (255 - bdval) * (255 - i) / (255 - bsval);
    }
    pixGetDimensions(pixd, &w, &h, NULL);
    data = pixGetData(pixd);
    wpl = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < w; j++) {
            pixel = line[j];
            extractRGBValues(pixel, &rval, &gval, &bval);
            composeRGBPixel(rtab[rval], gtab[gval], btab[bval], &pixel);
            line[j] = pixel;
        }
    }

    FREE(rtab);
    FREE(gtab);
    FREE(btab);
    return pixd;
}


/*!
 *  pixelShiftByComponent()
 *
 *      Input:  rval, gval, bval
 *              srcval (source color: 0xrrggbb00)
 *              dstval (target color: 0xrrggbb00)
 *              &ppixel (<return> rgb value)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is a linear transformation that gives the same result
 *          on a single pixel as pixShiftByComponent() gives
 *          on a pix.  Each component is handled separately.  If
 *          the dest component is larger than the src, then the
 *          component is pushed toward 255 by the same fraction as
 *          the src --> dest shift.
 */
l_int32
pixelShiftByComponent(l_int32    rval,
                      l_int32    gval,
                      l_int32    bval,
                      l_uint32   srcval,
                      l_uint32   dstval,
                      l_uint32  *ppixel)
{
l_int32  rsval, rdval, gsval, gdval, bsval, bdval, rs, gs, bs;

    PROCNAME("pixelShiftByComponent");

    if (!ppixel)
        return ERROR_INT("&pixel defined", procName, 1);

    extractRGBValues(srcval, &rsval, &gsval, &bsval);
    extractRGBValues(dstval, &rdval, &gdval, &bdval);
    if (rdval == rsval)
        rs = rval;
    else if (rdval < rsval)
        rs = (rval * rdval) / rsval;
    else
        rs = 255 - (255 - rdval) * (255 - rval) / (255 - rsval);
    if (gdval == gsval)
        gs = gval;
    else if (gdval < gsval)
        gs = (gval * gdval) / gsval;
    else
        gs = 255 - (255 - gdval) * (255 - gval) / (255 - gsval);
    if (bdval == bsval)
        bs = bval;
    else if (bdval < bsval)
        bs = (bval * bdval) / bsval;
    else
        bs = 255 - (255 - bdval) * (255 - bval) / (255 - bsval);
    composeRGBPixel(rs, gs, bs, ppixel);
    return 0;
}


/*!
 *  pixelFractionalShift()
 *
 *      Input:  rval, gval, bval
 *              fraction (negative toward black; positive toward white)
 *              &ppixel (<return> rgb value)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This transformation leaves the hue invariant, while changing
 *          the saturation and intensity.  It can be used for that
 *          purpose in pixLinearMapToTargetColor().
 *      (2) @fraction is in the range [-1 .... +1].  If @fraction < 0,
 *          saturation is increased and brightness is reduced.  The
 *          opposite results if @fraction > 0.  If @fraction == -1,
 *          the resulting pixel is black; @fraction == 1 results in white.
 */
l_int32
pixelFractionalShift(l_int32    rval,
                     l_int32    gval,
                     l_int32    bval,
                     l_float32  fraction,
                     l_uint32  *ppixel)
{
l_int32  nrval, ngval, nbval;

    PROCNAME("pixelFractionalShift");

    if (!ppixel)
        return ERROR_INT("&pixel defined", procName, 1);
    if (fraction < -1.0 || fraction > 1.0)
        return ERROR_INT("fraction not in [-1 ... +1]", procName, 1);

    nrval = (fraction < 0) ? (l_int32)((1.0 + fraction) * rval + 0.5) :
            rval + (l_int32)(fraction * (255 - rval) + 0.5);
    ngval = (fraction < 0) ? (l_int32)((1.0 + fraction) * gval + 0.5) :
            gval + (l_int32)(fraction * (255 - gval) + 0.5);
    nbval = (fraction < 0) ? (l_int32)((1.0 + fraction) * bval + 0.5) :
            bval + (l_int32)(fraction * (255 - bval) + 0.5);
    composeRGBPixel(nrval, ngval, nbval, ppixel);
    return 0;
}
