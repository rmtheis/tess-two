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
 *  colormap.c
 *
 *      Colormap creation, copy, destruction, addition
 *           PIXCMAP    *pixcmapCreate()
 *           PIXCMAP    *pixcmapCreateRandom()
 *           PIXCMAP    *pixcmapCreateLinear()
 *           PIXCMAP    *pixcmapCopy()
 *           void        pixcmapDestroy()
 *           l_int32     pixcmapAddColor()
 *           l_int32     pixcmapAddRGBA()
 *           l_int32     pixcmapAddNewColor()
 *           l_int32     pixcmapAddNearestColor()
 *           l_int32     pixcmapUsableColor()
 *           l_int32     pixcmapAddBlackOrWhite()
 *           l_int32     pixcmapSetBlackAndWhite()
 *           l_int32     pixcmapGetCount()
 *           l_int32     pixcmapGetDepth()
 *           l_int32     pixcmapGetMinDepth()
 *           l_int32     pixcmapGetFreeCount()
 *           l_int32     pixcmapClear()
 *
 *      Colormap random access and test
 *           l_int32     pixcmapGetColor()
 *           l_int32     pixcmapGetColor32()
 *           l_int32     pixcmapGetRGBA()
 *           l_int32     pixcmapGetRGBA32()
 *           l_int32     pixcmapResetColor()
 *           l_int32     pixcmapGetIndex()
 *           l_int32     pixcmapHasColor()
 *           l_int32     pixcmapIsOpaque()
 *           l_int32     pixcmapCountGrayColors()
 *           l_int32     pixcmapGetRankIntensity()
 *           l_int32     pixcmapGetNearestIndex()
 *           l_int32     pixcmapGetNearestGrayIndex()
 *           l_int32     pixcmapGetComponentRange()
 *           l_int32     pixcmapGetExtremeValue()
 *
 *      Colormap conversion
 *           PIXCMAP    *pixcmapGrayToColor()
 *           PIXCMAP    *pixcmapColorToGray()
 *
 *      Colormap I/O
 *           l_int32     pixcmapReadStream()
 *           l_int32     pixcmapWriteStream()
 *
 *      Extract colormap arrays and serialization
 *           l_int32     pixcmapToArrays()
 *           l_int32     pixcmapToRGBTable()
 *           l_int32     pixcmapSerializeToMemory()
 *           PIXCMAP    *pixcmapDeserializeFromMemory()
 *           char       *pixcmapConvertToHex()
 *
 *      Colormap transforms
 *           l_int32     pixcmapGammaTRC()
 *           l_int32     pixcmapContrastTRC()
 *           l_int32     pixcmapShiftIntensity()
 *           l_int32     pixcmapShiftByComponent()
 */

#include <string.h>
#include "allheaders.h"


/*-------------------------------------------------------------*
 *                Colormap creation and addition               *
 *-------------------------------------------------------------*/
/*!
 *  pixcmapCreate()
 *
 *      Input:  depth (bpp, of pix)
 *      Return: cmap, or null on error
 */
PIXCMAP *
pixcmapCreate(l_int32  depth)
{
RGBA_QUAD  *cta;
PIXCMAP    *cmap;

    PROCNAME("pixcmapCreate");

    if (depth != 1 && depth != 2 && depth !=4 && depth != 8)
        return (PIXCMAP *)ERROR_PTR("depth not in {1,2,4,8}", procName, NULL);

    if ((cmap = (PIXCMAP *)CALLOC(1, sizeof(PIXCMAP))) == NULL)
        return (PIXCMAP *)ERROR_PTR("cmap not made", procName, NULL);
    cmap->depth = depth;
    cmap->nalloc = 1 << depth;
    if ((cta = (RGBA_QUAD *)CALLOC(cmap->nalloc, sizeof(RGBA_QUAD))) == NULL)
        return (PIXCMAP *)ERROR_PTR("cta not made", procName, NULL);
    cmap->array = cta;
    cmap->n = 0;

    return cmap;
}


/*!
 *  pixcmapCreateRandom()
 *
 *      Input:  depth (bpp, of pix; 2, 4 or 8)
 *              hasblack (1 if the first color is black; 0 if no black)
 *              haswhite (1 if the last color is white; 0 if no white)
 *      Return: cmap, or null on error
 *
 *  Notes:
 *      (1) This sets up a colormap with random colors,
 *          where the first color is optionally black, the last color
 *          is optionally white, and the remaining colors are
 *          chosen randomly.
 *      (2) The number of randomly chosen colors is:
 *               2^(depth) - haswhite - hasblack
 *      (3) Because rand() is seeded, it might disrupt otherwise
 *          deterministic results if also used elsewhere in a program.
 *      (4) rand() is not threadsafe, and will generate garbage if run
 *          on multiple threads at once -- though garbage is generally
 *          what you want from a random number generator!
 *      (5) Modern rand()s have equal randomness in low and high order
 *          bits, but older ones don't.  Here, we're just using rand()
 *          to choose colors for output.
 */
PIXCMAP *
pixcmapCreateRandom(l_int32  depth,
                    l_int32  hasblack,
                    l_int32  haswhite)
{
l_int32   ncolors, i;
l_int32   red[256], green[256], blue[256];
PIXCMAP  *cmap;

    PROCNAME("pixcmapCreateRandom");

    if (depth != 2 && depth != 4 && depth != 8)
        return (PIXCMAP *)ERROR_PTR("depth not in {2, 4, 8}", procName, NULL);
    if (hasblack != 0) hasblack = 1;
    if (haswhite != 0) haswhite = 1;

    cmap = pixcmapCreate(depth);
    ncolors = 1 << depth;
    if (hasblack)  /* first color is optionally black */
        pixcmapAddColor(cmap, 0, 0, 0);
    for (i = hasblack; i < ncolors - haswhite; i++) {
        red[i] = (l_uint32)rand() & 0xff;
        green[i] = (l_uint32)rand() & 0xff;
        blue[i] = (l_uint32)rand() & 0xff;
        pixcmapAddColor(cmap, red[i], green[i], blue[i]);
    }
    if (haswhite)  /* last color is optionally white */
        pixcmapAddColor(cmap, 255, 255, 255);

    return cmap;
}


/*!
 *  pixcmapCreateLinear()
 *
 *      Input:  d (depth of pix for this colormap; 1, 2, 4 or 8)
 *              nlevels (valid in range [2, 2^d])
 *      Return: cmap, or null on error
 *
 *  Notes:
 *      (1) Colormap has equally spaced gray color values
 *          from black (0, 0, 0) to white (255, 255, 255).
 */
PIXCMAP *
pixcmapCreateLinear(l_int32  d,
                    l_int32  nlevels)
{
l_int32   maxlevels, i, val;
PIXCMAP  *cmap;

    PROCNAME("pixcmapCreateLinear");

    if (d != 1 && d != 2 && d !=4 && d != 8)
        return (PIXCMAP *)ERROR_PTR("d not in {1, 2, 4, 8}", procName, NULL);
    maxlevels = 1 << d;
    if (nlevels < 2 || nlevels > maxlevels)
        return (PIXCMAP *)ERROR_PTR("invalid nlevels", procName, NULL);

    cmap = pixcmapCreate(d);
    for (i = 0; i < nlevels; i++) {
        val = (255 * i) / (nlevels - 1);
        pixcmapAddColor(cmap, val, val, val);
    }
    return cmap;
}


/*!
 *  pixcmapCopy()
 *
 *      Input:  cmaps
 *      Return: cmapd, or null on error
 */
PIXCMAP *
pixcmapCopy(PIXCMAP  *cmaps)
{
l_int32   nbytes;
PIXCMAP  *cmapd;

    PROCNAME("pixcmapCopy");

    if (!cmaps)
        return (PIXCMAP *)ERROR_PTR("cmaps not defined", procName, NULL);

    if ((cmapd = (PIXCMAP *)CALLOC(1, sizeof(PIXCMAP))) == NULL)
        return (PIXCMAP *)ERROR_PTR("cmapd not made", procName, NULL);
    nbytes = cmaps->nalloc * sizeof(RGBA_QUAD);
    if ((cmapd->array = (void *)CALLOC(1, nbytes)) == NULL)
        return (PIXCMAP *)ERROR_PTR("cmap array not made", procName, NULL);
    memcpy(cmapd->array, cmaps->array, nbytes);
    cmapd->n = cmaps->n;
    cmapd->nalloc = cmaps->nalloc;
    cmapd->depth = cmaps->depth;
    return cmapd;
}


/*!
 *  pixcmapDestroy()
 *
 *      Input:  &cmap (<set to null>)
 *      Return: void
 */
void
pixcmapDestroy(PIXCMAP  **pcmap)
{
PIXCMAP  *cmap;

    PROCNAME("pixcmapDestroy");

    if (pcmap == NULL) {
        L_WARNING("ptr address is null!\n", procName);
        return;
    }

    if ((cmap = *pcmap) == NULL)
        return;

    FREE(cmap->array);
    FREE(cmap);
    *pcmap = NULL;
    return;
}


/*!
 *  pixcmapAddColor()
 *
 *      Input:  cmap
 *              rval, gval, bval (colormap entry to be added; each number
 *                                is in range [0, ... 255])
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This always adds the color if there is room.
 *      (2) The alpha component is 255 (opaque)
 */
l_int32
pixcmapAddColor(PIXCMAP  *cmap,
                l_int32   rval,
                l_int32   gval,
                l_int32   bval)
{
RGBA_QUAD  *cta;

    PROCNAME("pixcmapAddColor");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    if (cmap->n >= cmap->nalloc)
        return ERROR_INT("no free color entries", procName, 1);

    cta = (RGBA_QUAD *)cmap->array;
    cta[cmap->n].red = rval;
    cta[cmap->n].green = gval;
    cta[cmap->n].blue = bval;
    cta[cmap->n].alpha = 255;
    cmap->n++;
    return 0;
}


/*!
 *  pixcmapAddRGBA()
 *
 *      Input:  cmap
 *              rval, gval, bval, aval (colormap entry to be added;
 *                                      each number is in range [0, ... 255])
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This always adds the color if there is room.
 */
l_int32
pixcmapAddRGBA(PIXCMAP  *cmap,
               l_int32   rval,
               l_int32   gval,
               l_int32   bval,
               l_int32   aval)
{
RGBA_QUAD  *cta;

    PROCNAME("pixcmapAddRGBA");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    if (cmap->n >= cmap->nalloc)
        return ERROR_INT("no free color entries", procName, 1);

    cta = (RGBA_QUAD *)cmap->array;
    cta[cmap->n].red = rval;
    cta[cmap->n].green = gval;
    cta[cmap->n].blue = bval;
    cta[cmap->n].alpha = aval;
    cmap->n++;
    return 0;
}


/*!
 *  pixcmapAddNewColor()
 *
 *      Input:  cmap
 *              rval, gval, bval (colormap entry to be added; each number
 *                                is in range [0, ... 255])
 *              &index (<return> index of color)
 *      Return: 0 if OK, 1 on error; 2 if unable to add color
 *
 *  Notes:
 *      (1) This only adds color if not already there.
 *      (2) The alpha component is 255 (opaque)
 *      (3) This returns the index of the new (or existing) color.
 *      (4) Returns 2 with a warning if unable to add this color;
 *          the caller should check the return value.
 */
l_int32
pixcmapAddNewColor(PIXCMAP  *cmap,
                   l_int32   rval,
                   l_int32   gval,
                   l_int32   bval,
                   l_int32  *pindex)
{
    PROCNAME("pixcmapAddNewColor");

    if (!pindex)
        return ERROR_INT("&index not defined", procName, 1);
    *pindex = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

        /* Check if the color is already present. */
    if (!pixcmapGetIndex(cmap, rval, gval, bval, pindex))  /* found */
        return 0;

        /* We need to add the color.  Is there room? */
    if (cmap->n >= cmap->nalloc) {
        L_WARNING("no free color entries\n", procName);
        return 2;
    }

        /* There's room.  Add it. */
    pixcmapAddColor(cmap, rval, gval, bval);
    *pindex = pixcmapGetCount(cmap) - 1;
    return 0;
}


/*!
 *  pixcmapAddNearestColor()
 *
 *      Input:  cmap
 *              rval, gval, bval (colormap entry to be added; each number
 *                                is in range [0, ... 255])
 *              &index (<return> index of color)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This only adds color if not already there.
 *      (2) The alpha component is 255 (opaque)
 *      (3) If it's not in the colormap and there is no room to add
 *          another color, this returns the index of the nearest color.
 */
l_int32
pixcmapAddNearestColor(PIXCMAP  *cmap,
                       l_int32   rval,
                       l_int32   gval,
                       l_int32   bval,
                       l_int32  *pindex)
{
    PROCNAME("pixcmapAddNearestColor");

    if (!pindex)
        return ERROR_INT("&index not defined", procName, 1);
    *pindex = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

        /* Check if the color is already present. */
    if (!pixcmapGetIndex(cmap, rval, gval, bval, pindex))  /* found */
        return 0;

        /* We need to add the color.  Is there room? */
    if (cmap->n < cmap->nalloc) {
        pixcmapAddColor(cmap, rval, gval, bval);
        *pindex = pixcmapGetCount(cmap) - 1;
        return 0;
    }

        /* There's no room.  Return the index of the nearest color */
    pixcmapGetNearestIndex(cmap, rval, gval, bval, pindex);
    return 0;
}


/*!
 *  pixcmapUsableColor()
 *
 *      Input:  cmap
 *              rval, gval, bval (colormap entry to be added; each number
 *                                is in range [0, ... 255])
 *              usable (<return> 1 if usable; 0 if not)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This checks if the color already exists or if there is
 *          room to add it.  It makes no change in the colormap.
 */
l_int32
pixcmapUsableColor(PIXCMAP  *cmap,
                   l_int32   rval,
                   l_int32   gval,
                   l_int32   bval,
                   l_int32  *pusable)
{
l_int32  index;

    PROCNAME("pixcmapUsableColor");

    if (!pusable)
        return ERROR_INT("&usable not defined", procName, 1);
    *pusable = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

        /* Is there room to add it? */
    if (cmap->n < cmap->nalloc) {
        *pusable = 1;
        return 0;
    }

        /* No room; check if the color is already present. */
    if (!pixcmapGetIndex(cmap, rval, gval, bval, &index))   /* found */
        *pusable = 1;
    return 0;
}


/*!
 *  pixcmapAddBlackOrWhite()
 *
 *      Input:  cmap
 *              color (0 for black, 1 for white)
 *              &index (<optional return> index of color; can be null)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This only adds color if not already there.
 *      (2) The alpha component is 255 (opaque)
 *      (3) This sets index to the requested color.
 *      (4) If there is no room in the colormap, returns the index
 *          of the closest color.
 */
l_int32
pixcmapAddBlackOrWhite(PIXCMAP  *cmap,
                       l_int32   color,
                       l_int32  *pindex)
{
l_int32  index;

    PROCNAME("pixcmapAddBlackOrWhite");

    if (pindex) *pindex = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    if (color == 0) {  /* black */
        if (pixcmapGetFreeCount(cmap) > 0)
            pixcmapAddNewColor(cmap, 0, 0, 0, &index);
        else
            pixcmapGetRankIntensity(cmap, 0.0, &index);
    } else {  /* white */
        if (pixcmapGetFreeCount(cmap) > 0)
            pixcmapAddNewColor(cmap, 255, 255, 255, &index);
        else
            pixcmapGetRankIntensity(cmap, 1.0, &index);
    }

    if (pindex)
        *pindex = index;
    return 0;
}


/*!
 *  pixcmapSetBlackAndWhite()
 *
 *      Input:  cmap
 *              setblack (0 for no operation; 1 to set darkest color to black)
 *              setwhite (0 for no operation; 1 to set lightest color to white)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixcmapSetBlackAndWhite(PIXCMAP  *cmap,
                        l_int32   setblack,
                        l_int32   setwhite)
{
l_int32  index;

    PROCNAME("pixcmapSetBlackAndWhite");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    if (setblack) {
        pixcmapGetRankIntensity(cmap, 0.0, &index);
        pixcmapResetColor(cmap, index, 0, 0, 0);
    }
    if (setwhite) {
        pixcmapGetRankIntensity(cmap, 1.0, &index);
        pixcmapResetColor(cmap, index, 255, 255, 255);
    }
    return 0;
}


/*!
 *  pixcmapGetCount()
 *
 *      Input:  cmap
 *      Return: count, or 0 on error
 */
l_int32
pixcmapGetCount(PIXCMAP  *cmap)
{
    PROCNAME("pixcmapGetCount");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 0);
    return cmap->n;
}


/*!
 *  pixcmapGetFreeCount()
 *
 *      Input:  cmap
 *      Return: free entries, or 0 on error
 */
l_int32
pixcmapGetFreeCount(PIXCMAP  *cmap)
{
    PROCNAME("pixcmapGetFreeCount");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 0);
    return (cmap->nalloc - cmap->n);
}


/*!
 *  pixcmapGetDepth()
 *
 *      Input:  cmap
 *      Return: depth, or 0 on error
 */
l_int32
pixcmapGetDepth(PIXCMAP  *cmap)
{
    PROCNAME("pixcmapGetDepth");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 0);
    return cmap->depth;
}


/*!
 *  pixcmapGetMinDepth()
 *
 *      Input:  cmap
 *              &mindepth (<return> minimum depth to support the colormap)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) On error, &mindepth is returned as 0.
 */
l_int32
pixcmapGetMinDepth(PIXCMAP  *cmap,
                   l_int32  *pmindepth)
{
l_int32  ncolors;

    PROCNAME("pixcmapGetMinDepth");

    if (!pmindepth)
        return ERROR_INT("&mindepth not defined", procName, 1);
    *pmindepth = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    ncolors = pixcmapGetCount(cmap);
    if (ncolors <= 4)
        *pmindepth = 2;
    else if (ncolors <= 16)
        *pmindepth = 4;
    else  /* ncolors > 16 */
        *pmindepth = 8;
    return 0;
}


/*!
 *  pixcmapClear()
 *
 *      Input:  cmap
 *      Return: 0 if OK, 1 on error
 *
 *  Note: this removes the colors by setting the count to 0.
 */
l_int32
pixcmapClear(PIXCMAP  *cmap)
{
    PROCNAME("pixcmapClear");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    cmap->n = 0;
    return 0;
}


/*-------------------------------------------------------------*
 *                      Colormap random access                 *
 *-------------------------------------------------------------*/
/*!
 *  pixcmapGetColor()
 *
 *      Input:  cmap
 *              index
 *              &rval, &gval, &bval (<return> each color value)
 *      Return: 0 if OK, 1 if not accessable (caller should check)
 */
l_int32
pixcmapGetColor(PIXCMAP  *cmap,
                l_int32   index,
                l_int32  *prval,
                l_int32  *pgval,
                l_int32  *pbval)
{
RGBA_QUAD  *cta;

    PROCNAME("pixcmapGetColor");

    if (!prval || !pgval || !pbval)
        return ERROR_INT("&rval, &gval, &bval not all defined", procName, 1);
    *prval = *pgval = *pbval = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    if (index < 0 || index >= cmap->n)
        return ERROR_INT("index out of bounds", procName, 1);

    cta = (RGBA_QUAD *)cmap->array;
    *prval = cta[index].red;
    *pgval = cta[index].green;
    *pbval = cta[index].blue;
    return 0;
}


/*!
 *  pixcmapGetColor32()
 *
 *      Input:  cmap
 *              index
 *              &val32 (<return> 32-bit rgb color value)
 *      Return: 0 if OK, 1 if not accessable (caller should check)
 *
 *  Notes:
 *      (1) The returned alpha channel value is 255.
 */
l_int32
pixcmapGetColor32(PIXCMAP   *cmap,
                  l_int32    index,
                  l_uint32  *pval32)
{
l_int32  rval, gval, bval;

    PROCNAME("pixcmapGetColor32");

    if (!pval32)
        return ERROR_INT("&val32 not defined", procName, 1);
    *pval32 = 0;

    if (pixcmapGetColor(cmap, index, &rval, &gval, &bval) != 0)
        return ERROR_INT("rgb values not found", procName, 1);
    composeRGBAPixel(rval, gval, bval, 255, pval32);
    return 0;
}


/*!
 *  pixcmapGetRGBA()
 *
 *      Input:  cmap
 *              index
 *              &rval, &gval, &bval, &aval (<return> each color value)
 *      Return: 0 if OK, 1 if not accessable (caller should check)
 */
l_int32
pixcmapGetRGBA(PIXCMAP  *cmap,
               l_int32   index,
               l_int32  *prval,
               l_int32  *pgval,
               l_int32  *pbval,
               l_int32  *paval)
{
RGBA_QUAD  *cta;

    PROCNAME("pixcmapGetRGBA");

    if (!prval || !pgval || !pbval || !paval)
        return ERROR_INT("&rval, &gval, &bval, &aval not all defined",
                procName, 1);
    *prval = *pgval = *pbval = *paval = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    if (index < 0 || index >= cmap->n)
        return ERROR_INT("index out of bounds", procName, 1);

    cta = (RGBA_QUAD *)cmap->array;
    *prval = cta[index].red;
    *pgval = cta[index].green;
    *pbval = cta[index].blue;
    *paval = cta[index].alpha;
    return 0;
}


/*!
 *  pixcmapGetRGBA32()
 *
 *      Input:  cmap
 *              index
 *              &val32 (<return> 32-bit rgba color value)
 *      Return: 0 if OK, 1 if not accessable (caller should check)
 */
l_int32
pixcmapGetRGBA32(PIXCMAP   *cmap,
                 l_int32    index,
                 l_uint32  *pval32)
{
l_int32  rval, gval, bval, aval;

    PROCNAME("pixcmapGetRGBA32");

    if (!pval32)
        return ERROR_INT("&val32 not defined", procName, 1);
    *pval32 = 0;

    if (pixcmapGetRGBA(cmap, index, &rval, &gval, &bval, &aval) != 0)
        return ERROR_INT("rgba values not found", procName, 1);
    composeRGBAPixel(rval, gval, bval, aval, pval32);
    return 0;
}


/*!
 *  pixcmapResetColor()
 *
 *      Input:  cmap
 *              index
 *              rval, gval, bval (colormap entry to be reset; each number
 *                                is in range [0, ... 255])
 *      Return: 0 if OK, 1 if not accessable (caller should check)
 *
 *  Notes:
 *      (1) This resets sets the color of an entry that has already
 *          been set and included in the count of colors.
 *      (2) The alpha component is 255 (opaque)
 */
l_int32
pixcmapResetColor(PIXCMAP  *cmap,
                  l_int32   index,
                  l_int32   rval,
                  l_int32   gval,
                  l_int32   bval)
{
RGBA_QUAD  *cta;

    PROCNAME("pixcmapResetColor");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    if (index < 0 || index >= cmap->n)
        return ERROR_INT("index out of bounds", procName, 1);

    cta = (RGBA_QUAD *)cmap->array;
    cta[index].red = rval;
    cta[index].green = gval;
    cta[index].blue = bval;
    cta[index].alpha = 255;
    return 0;
}


/*!
 *  pixcmapGetIndex()
 *
 *      Input:  cmap
 *              rval, gval, bval (colormap colors to search for; each number
 *                                is in range [0, ... 255])
 *              &index (<return>)
 *      Return: 0 if found, 1 if not found (caller must check)
 */
l_int32
pixcmapGetIndex(PIXCMAP  *cmap,
                l_int32   rval,
                l_int32   gval,
                l_int32   bval,
                l_int32  *pindex)
{
l_int32     n, i;
RGBA_QUAD  *cta;

    PROCNAME("pixcmapGetIndex");

    if (!pindex)
        return ERROR_INT("&index not defined", procName, 1);
    *pindex = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    n = pixcmapGetCount(cmap);

    cta = (RGBA_QUAD *)cmap->array;
    for (i = 0; i < n; i++) {
        if (rval == cta[i].red &&
            gval == cta[i].green &&
            bval == cta[i].blue) {
            *pindex = i;
            return 0;
        }
    }
    return 1;
}


/*!
 *  pixcmapHasColor()
 *
 *      Input:  cmap
 *              &color (<return> TRUE if cmap has color; FALSE otherwise)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixcmapHasColor(PIXCMAP  *cmap,
                l_int32  *pcolor)
{
l_int32   n, i;
l_int32  *rmap, *gmap, *bmap;

    PROCNAME("pixcmapHasColor");

    if (!pcolor)
        return ERROR_INT("&color not defined", procName, 1);
    *pcolor = FALSE;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    if (pixcmapToArrays(cmap, &rmap, &gmap, &bmap, NULL))
        return ERROR_INT("colormap arrays not made", procName, 1);
    n = pixcmapGetCount(cmap);
    for (i = 0; i < n; i++) {
        if ((rmap[i] != gmap[i]) || (rmap[i] != bmap[i])) {
            *pcolor = TRUE;
            break;
        }
    }

    FREE(rmap);
    FREE(gmap);
    FREE(bmap);
    return 0;
}


/*!
 *  pixcmapIsOpaque()
 *
 *      Input:  cmap
 *              &opaque (<return> TRUE if fully opaque: all entries are 255)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixcmapIsOpaque(PIXCMAP  *cmap,
                l_int32  *popaque)
{
l_int32     i, n;
RGBA_QUAD  *cta;

    PROCNAME("pixcmapIsOpaque");

    if (!popaque)
        return ERROR_INT("&opaque not defined", procName, 1);
    *popaque = TRUE;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    n = pixcmapGetCount(cmap);
    cta = (RGBA_QUAD *)cmap->array;
    for (i = 0; i < n; i++) {
        if (cta[i].alpha != 255) {
            *popaque = FALSE;
            break;
        }
    }
    return 0;
}


/*!
 *  pixcmapCountGrayColors()
 *
 *      Input:  cmap
 *              &ngray (<return> number of gray colors)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This counts the unique gray colors, including black and white.
 */
l_int32
pixcmapCountGrayColors(PIXCMAP  *cmap,
                       l_int32  *pngray)
{
l_int32   n, i, rval, gval, bval, count;
l_int32  *array;

    PROCNAME("pixcmapCountGrayColors");

    if (!pngray)
        return ERROR_INT("&ngray not defined", procName, 1);
    *pngray = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    array = (l_int32 *)CALLOC(256, sizeof(l_int32));
    n = pixcmapGetCount(cmap);
    count = 0;
    for (i = 0; i < n; i++) {
        pixcmapGetColor(cmap, i, &rval, &gval, &bval);
        if ((rval == gval) && (rval == bval) && (array[rval] == 0)) {
            array[rval] = 1;
            count++;
        }
    }

    FREE(array);
    *pngray = count;
    return 0;
}


/*!
 *  pixcmapGetRankIntensity()
 *
 *      Input:  cmap
 *              rankval (0.0 for darkest, 1.0 for lightest color)
 *              &index (<return> the index into the colormap that
 *                      corresponds to the rank intensity color)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixcmapGetRankIntensity(PIXCMAP    *cmap,
                        l_float32   rankval,
                        l_int32    *pindex)
{
l_int32  n, i, rval, gval, bval, rankindex;
NUMA    *na, *nasort;

    PROCNAME("pixcmapGetRankIntensity");

    if (!pindex)
        return ERROR_INT("&index not defined", procName, 1);
    *pindex = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    if (rankval < 0.0 || rankval > 1.0)
        return ERROR_INT("rankval not in [0.0 ... 1.0]", procName, 1);

    n = pixcmapGetCount(cmap);
    na = numaCreate(n);
    for (i = 0; i < n; i++) {
        pixcmapGetColor(cmap, i, &rval, &gval, &bval);
        numaAddNumber(na, rval + gval + bval);
    }
    nasort = numaGetSortIndex(na, L_SORT_INCREASING);
    rankindex = (l_int32)(rankval * (n - 1) + 0.5);
    numaGetIValue(nasort, rankindex, pindex);

    numaDestroy(&na);
    numaDestroy(&nasort);
    return 0;
}


/*!
 *  pixcmapGetNearestIndex()
 *
 *      Input:  cmap
 *              rval, gval, bval (colormap colors to search for; each number
 *                                is in range [0, ... 255])
 *              &index (<return> the index of the nearest color)
 *      Return: 0 if OK, 1 on error (caller must check)
 *
 *  Notes:
 *      (1) Returns the index of the exact color if possible, otherwise the
 *          index of the color closest to the target color.
 *      (2) Nearest color is that which is the least sum-of-squares distance
 *          from the target color.
 */
l_int32
pixcmapGetNearestIndex(PIXCMAP  *cmap,
                       l_int32   rval,
                       l_int32   gval,
                       l_int32   bval,
                       l_int32  *pindex)
{
l_int32     i, n, delta, dist, mindist;
RGBA_QUAD  *cta;

    PROCNAME("pixcmapGetNearestIndex");

    if (!pindex)
        return ERROR_INT("&index not defined", procName, 1);
    *pindex = UNDEF;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    if ((cta = (RGBA_QUAD *)cmap->array) == NULL)
        return ERROR_INT("cta not defined(!)", procName, 1);
    n = pixcmapGetCount(cmap);

    mindist = 3 * 255 * 255 + 1;
    for (i = 0; i < n; i++) {
        delta = cta[i].red - rval;
        dist = delta * delta;
        delta = cta[i].green - gval;
        dist += delta * delta;
        delta = cta[i].blue - bval;
        dist += delta * delta;
        if (dist < mindist) {
            *pindex = i;
            if (dist == 0)
                break;
            mindist = dist;
        }
    }

    return 0;
}


/*!
 *  pixcmapGetNearestGrayIndex()
 *
 *      Input:  cmap
 *              val (gray value to search for; in range [0, ... 255])
 *              &index (<return> the index of the nearest color)
 *      Return: 0 if OK, 1 on error (caller must check)
 *
 *  Notes:
 *      (1) This should be used on gray colormaps.  It uses only the
 *          green value of the colormap.
 *      (2) Returns the index of the exact color if possible, otherwise the
 *          index of the color closest to the target color.
 */
l_int32
pixcmapGetNearestGrayIndex(PIXCMAP  *cmap,
                           l_int32   val,
                           l_int32  *pindex)
{
l_int32     i, n, dist, mindist;
RGBA_QUAD  *cta;

    PROCNAME("pixcmapGetNearestGrayIndex");

    if (!pindex)
        return ERROR_INT("&index not defined", procName, 1);
    *pindex = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    if (val < 0 || val > 255)
        return ERROR_INT("val not in [0 ... 255]", procName, 1);

    if ((cta = (RGBA_QUAD *)cmap->array) == NULL)
        return ERROR_INT("cta not defined(!)", procName, 1);
    n = pixcmapGetCount(cmap);

    mindist = 256;
    for (i = 0; i < n; i++) {
        dist = cta[i].green - val;
        dist = L_ABS(dist);
        if (dist < mindist) {
            *pindex = i;
            if (dist == 0)
                break;
            mindist = dist;
        }
    }

    return 0;
}


/*!
 *  pixcmapGetComponentRange()
 *
 *      Input:  cmap
 *              color (L_SELECT_RED, L_SELECT_GREEN or L_SELECT_BLUE)
 *              &minval (<optional return> minimum value of component)
 *              &maxval (<optional return> minimum value of component)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Returns for selected components the extreme value
 *          (either min or max) of the color component that is
 *          found in the colormap.
 */
l_int32
pixcmapGetComponentRange(PIXCMAP  *cmap,
                         l_int32   color,
                         l_int32  *pminval,
                         l_int32  *pmaxval)
{
    PROCNAME("pixcmapGetComponentRange");

    if (pminval) *pminval = 0;
    if (pmaxval) *pmaxval = 0;
    if (!pminval && !pmaxval)
        return ERROR_INT("no result requested", procName, 1);

    if (color == L_SELECT_RED) {
        pixcmapGetExtremeValue(cmap, L_SELECT_MIN, pminval, NULL, NULL);
        pixcmapGetExtremeValue(cmap, L_SELECT_MAX, pmaxval, NULL, NULL);
    } else if (color == L_SELECT_GREEN) {
        pixcmapGetExtremeValue(cmap, L_SELECT_MIN, NULL, pminval, NULL);
        pixcmapGetExtremeValue(cmap, L_SELECT_MAX, NULL, pmaxval, NULL);
    } else if (color == L_SELECT_BLUE) {
        pixcmapGetExtremeValue(cmap, L_SELECT_MIN, NULL, NULL, pminval);
        pixcmapGetExtremeValue(cmap, L_SELECT_MAX, NULL, NULL, pmaxval);
    } else {
        return ERROR_INT("invalid color", procName, 1);
    }

    return 0;
}


/*!
 *  pixcmapGetExtremeValue()
 *
 *      Input:  cmap
 *              type (L_SELECT_MIN or L_SELECT_MAX)
 *              &rval (<optional return> red component)
 *              &gval (<optional return> green component)
 *              &bval (<optional return> blue component)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Returns for selected components the extreme value
 *          (either min or max) of the color component that is
 *          found in the colormap.
 */
l_int32
pixcmapGetExtremeValue(PIXCMAP  *cmap,
                       l_int32   type,
                       l_int32  *prval,
                       l_int32  *pgval,
                       l_int32  *pbval)
{
l_int32  i, n, rval, gval, bval, extrval, extgval, extbval;

    PROCNAME("pixcmapGetExtremeValue");

    if (!prval && !pgval && !pbval)
        return ERROR_INT("no result requested for return", procName, 1);
    if (prval) *prval = 0;
    if (pgval) *pgval = 0;
    if (pbval) *pbval = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    if (type != L_SELECT_MIN && type != L_SELECT_MAX)
        return ERROR_INT("invalid type", procName, 1);

    if (type == L_SELECT_MIN) {
        extrval = 100000;
        extgval = 100000;
        extbval = 100000;
    } else {
        extrval = 0;
        extgval = 0;
        extbval = 0;
    }

    n = pixcmapGetCount(cmap);
    for (i = 0; i < n; i++) {
        pixcmapGetColor(cmap, i, &rval, &gval, &bval);
        if ((type == L_SELECT_MIN && rval < extrval) ||
            (type == L_SELECT_MAX && rval > extrval))
            extrval = rval;
        if ((type == L_SELECT_MIN && gval < extgval) ||
            (type == L_SELECT_MAX && gval > extgval))
            extgval = gval;
        if ((type == L_SELECT_MIN && bval < extbval) ||
            (type == L_SELECT_MAX && bval > extbval))
            extbval = bval;
    }
    if (prval) *prval = extrval;
    if (pgval) *pgval = extgval;
    if (pbval) *pbval = extbval;
    return 0;
}


/*-------------------------------------------------------------*
 *                       Colormap conversion                   *
 *-------------------------------------------------------------*/
/*!
 *  pixcmapGrayToColor()
 *
 *      Input:  color
 *      Return: cmap, or null on error
 *
 *  Notes:
 *      (1) This creates a colormap that maps from gray to
 *          a specific color.  In the mapping, each component
 *          is faded to white, depending on the gray value.
 *      (2) In use, this is simply attached to a grayscale pix
 *          to give it the input color.
 */
PIXCMAP *
pixcmapGrayToColor(l_uint32  color)
{
l_int32   i, rval, gval, bval;
PIXCMAP  *cmap;

    extractRGBValues(color, &rval, &gval, &bval);
    cmap = pixcmapCreate(8);
    for (i = 0; i < 256; i++) {
        pixcmapAddColor(cmap, rval + (i * (255 - rval)) / 255,
                        gval + (i * (255 - gval)) / 255,
                        bval + (i * (255 - bval)) / 255);
    }

    return cmap;
}


/*!
 *  pixcmapColorToGray()
 *
 *      Input:  cmap
 *              rwt, gwt, bwt  (non-negative; these should add to 1.0)
 *      Return: cmap (gray), or null on error
 *
 *  Notes:
 *      (1) This creates a gray colormap from an arbitrary colormap.
 *      (2) In use, attach the output gray colormap to the pix
 *          (or a copy of it) that provided the input colormap.
 */
PIXCMAP *
pixcmapColorToGray(PIXCMAP   *cmaps,
                   l_float32  rwt,
                   l_float32  gwt,
                   l_float32  bwt)
{
l_int32    i, n, rval, gval, bval, val;
l_float32  sum;
PIXCMAP   *cmapd;

    PROCNAME("pixcmapColorToGray");

    if (!cmaps)
        return (PIXCMAP *)ERROR_PTR("cmaps not defined", procName, NULL);
    if (rwt < 0.0 || gwt < 0.0 || bwt < 0.0)
        return (PIXCMAP *)ERROR_PTR("weights not all >= 0.0", procName, NULL);

        /* Make sure the sum of weights is 1.0; otherwise, you can get
         * overflow in the gray value. */
    sum = rwt + gwt + bwt;
    if (sum == 0.0) {
        L_WARNING("all weights zero; setting equal to 1/3\n", procName);
        rwt = gwt = bwt = 0.33333;
        sum = 1.0;
    }
    if (L_ABS(sum - 1.0) > 0.0001) {  /* maintain ratios with sum == 1.0 */
        L_WARNING("weights don't sum to 1; maintaining ratios\n", procName);
        rwt = rwt / sum;
        gwt = gwt / sum;
        bwt = bwt / sum;
    }

    cmapd = pixcmapCopy(cmaps);
    n = pixcmapGetCount(cmapd);
    for (i = 0; i < n; i++) {
        pixcmapGetColor(cmapd, i, &rval, &gval, &bval);
        val = (l_int32)(rwt * rval + gwt * gval + bwt * bval + 0.5);
        pixcmapResetColor(cmapd, i, val, val, val);
    }

    return cmapd;
}


/*-------------------------------------------------------------*
 *                         Colormap I/O                        *
 *-------------------------------------------------------------*/
/*!
 *  pixcmapReadStream()
 *
 *      Input:  stream
 *      Return: cmap, or null on error
 */
PIXCMAP *
pixcmapReadStream(FILE  *fp)
{
l_int32   rval, gval, bval, aval, ignore;
l_int32   i, index, ret, depth, ncolors;
PIXCMAP  *cmap;

    PROCNAME("pixcmapReadStream");

    if (!fp)
        return (PIXCMAP *)ERROR_PTR("stream not defined", procName, NULL);

    ret = fscanf(fp, "\nPixcmap: depth = %d bpp; %d colors\n",
                 &depth, &ncolors);
    if (ret != 2 ||
        (depth != 1 && depth != 2 && depth != 4 && depth != 8) ||
        (ncolors < 2 || ncolors > 256))
        return (PIXCMAP *)ERROR_PTR("invalid cmap size", procName, NULL);
    ignore = fscanf(fp, "Color    R-val    G-val    B-val   Alpha\n");
    ignore = fscanf(fp, "----------------------------------------\n");

    if ((cmap = pixcmapCreate(depth)) == NULL)
        return (PIXCMAP *)ERROR_PTR("cmap not made", procName, NULL);
    for (i = 0; i < ncolors; i++) {
        if (fscanf(fp, "%3d       %3d      %3d      %3d      %3d\n",
                        &index, &rval, &gval, &bval, &aval) != 5)
            return (PIXCMAP *)ERROR_PTR("invalid entry", procName, NULL);
        pixcmapAddRGBA(cmap, rval, gval, bval, aval);
    }

    return cmap;
}


/*!
 *  pixcmapWriteStream()
 *
 *      Input:  stream, cmap
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixcmapWriteStream(FILE     *fp,
                   PIXCMAP  *cmap)
{
l_int32  *rmap, *gmap, *bmap, *amap;
l_int32   i;

    PROCNAME("pixcmapWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    if (pixcmapToArrays(cmap, &rmap, &gmap, &bmap, &amap))
        return ERROR_INT("colormap arrays not made", procName, 1);

    fprintf(fp, "\nPixcmap: depth = %d bpp; %d colors\n", cmap->depth, cmap->n);
    fprintf(fp, "Color    R-val    G-val    B-val   Alpha\n");
    fprintf(fp, "----------------------------------------\n");
    for (i = 0; i < cmap->n; i++)
        fprintf(fp, "%3d       %3d      %3d      %3d      %3d\n",
                i, rmap[i], gmap[i], bmap[i], amap[i]);
    fprintf(fp, "\n");

    FREE(rmap);
    FREE(gmap);
    FREE(bmap);
    FREE(amap);
    return 0;
}


/*----------------------------------------------------------------------*
 *               Extract colormap arrays and serialization              *
 *----------------------------------------------------------------------*/
/*!
 *  pixcmapToArrays()
 *
 *      Input:  colormap
 *              &rmap, &gmap, &bmap  (<return> colormap arrays)
 *              &amap (<optional return> alpha array)
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixcmapToArrays(PIXCMAP   *cmap,
                l_int32  **prmap,
                l_int32  **pgmap,
                l_int32  **pbmap,
                l_int32  **pamap)
{
l_int32    *rmap, *gmap, *bmap, *amap;
l_int32     i, ncolors;
RGBA_QUAD  *cta;

    PROCNAME("pixcmapToArrays");

    if (!prmap || !pgmap || !pbmap)
        return ERROR_INT("&rmap, &gmap, &bmap not all defined", procName, 1);
    *prmap = *pgmap = *pbmap = NULL;
    if (pamap) *pamap = NULL;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    ncolors = pixcmapGetCount(cmap);
    if (((rmap = (l_int32 *)CALLOC(ncolors, sizeof(l_int32))) == NULL) ||
        ((gmap = (l_int32 *)CALLOC(ncolors, sizeof(l_int32))) == NULL) ||
        ((bmap = (l_int32 *)CALLOC(ncolors, sizeof(l_int32))) == NULL))
            return ERROR_INT("calloc fail for *map", procName, 1);
    *prmap = rmap;
    *pgmap = gmap;
    *pbmap = bmap;
    if (pamap) {
        amap = (l_int32 *)CALLOC(ncolors, sizeof(l_int32));
        *pamap = amap;
    }

    cta = (RGBA_QUAD *)cmap->array;
    for (i = 0; i < ncolors; i++) {
        rmap[i] = cta[i].red;
        gmap[i] = cta[i].green;
        bmap[i] = cta[i].blue;
        if (pamap)
            amap[i] = cta[i].alpha;
    }

    return 0;
}


/*!
 *  pixcmapToRGBTable()
 *
 *      Input:  colormap
 *              &tab (<return> table of rgba values for the colormap)
 *              &ncolors (<optional return> size of table)
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixcmapToRGBTable(PIXCMAP    *cmap,
                  l_uint32  **ptab,
                  l_int32    *pncolors)
{
l_int32    i, ncolors, rval, gval, bval, aval;
l_uint32  *tab;

    PROCNAME("pixcmapToRGBTable");

    if (!ptab)
        return ERROR_INT("&tab not defined", procName, 1);
    *ptab = NULL;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    ncolors = pixcmapGetCount(cmap);
    if (pncolors)
        *pncolors = ncolors;
    if ((tab = (l_uint32 *)CALLOC(ncolors, sizeof(l_uint32))) == NULL)
        return ERROR_INT("tab not made", procName, 1);
    *ptab = tab;

    for (i = 0; i < ncolors; i++) {
        pixcmapGetRGBA(cmap, i, &rval, &gval, &bval, &aval);
        composeRGBAPixel(rval, gval, bval, aval, &tab[i]);
    }
    return 0;
}


/*!
 *  pixcmapSerializeToMemory()
 *
 *      Input:  colormap
 *              cpc (components/color: 3 for rgb, 4 for rgba)
 *              &ncolors (<return> number of colors in table)
 *              &data (<return> binary string, cpc bytes per color)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) When serializing to store in a pdf, use @cpc = 3.
 */
l_int32
pixcmapSerializeToMemory(PIXCMAP   *cmap,
                         l_int32    cpc,
                         l_int32   *pncolors,
                         l_uint8  **pdata)
{
l_int32   i, ncolors, rval, gval, bval, aval;
l_uint8  *data;

    PROCNAME("pixcmapSerializeToMemory");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    *pdata = NULL;
    if (!pncolors)
        return ERROR_INT("&ncolors not defined", procName, 1);
    *pncolors = 0;
    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    if (cpc != 3 && cpc != 4)
        return ERROR_INT("cpc not 3 or 4", procName, 1);

    ncolors = pixcmapGetCount(cmap);
    *pncolors = ncolors;
    if ((data = (l_uint8 *)CALLOC(cpc * ncolors, sizeof(l_uint8))) == NULL)
        return ERROR_INT("data not made", procName, 1);
    *pdata = data;

    for (i = 0; i < ncolors; i++) {
        pixcmapGetRGBA(cmap, i, &rval, &gval, &bval, &aval);
        data[cpc * i] = rval;
        data[cpc * i + 1] = gval;
        data[cpc * i + 2] = bval;
        if (cpc == 4)
            data[cpc * i + 3] = aval;
    }
    return 0;
}


/*!
 *  pixcmapDeserializeFromMemory()
 *
 *      Input:  data (binary string, 3 or 4 bytes per color)
 *              cpc (components/color: 3 for rgb, 4 for rgba)
 *              ncolors
 *      Return: cmap, or null on error
 */
PIXCMAP *
pixcmapDeserializeFromMemory(l_uint8  *data,
                             l_int32   cpc,
                             l_int32   ncolors)
{
l_int32   i, d, rval, gval, bval, aval;
PIXCMAP  *cmap;

    PROCNAME("pixcmapDeserializeFromMemory");

    if (!data)
        return (PIXCMAP *)ERROR_PTR("data not defined", procName, NULL);
    if (cpc != 3 && cpc != 4)
        return (PIXCMAP *)ERROR_PTR("cpc not 3 or 4", procName, NULL);
    if (ncolors == 0)
        return (PIXCMAP *)ERROR_PTR("no entries", procName, NULL);
    if (ncolors > 256)
        return (PIXCMAP *)ERROR_PTR("ncolors > 256", procName, NULL);

    if (ncolors > 16)
        d = 8;
    else if (ncolors > 4)
        d = 4;
    else if (ncolors > 2)
        d = 2;
    else
        d = 1;
    cmap = pixcmapCreate(d);
    for (i = 0; i < ncolors; i++) {
        rval = data[cpc * i];
        gval = data[cpc * i + 1];
        bval = data[cpc * i + 2];
        if (cpc == 4)
            aval = data[cpc * i + 3];
        else
            aval = 255;  /* opaque */
        pixcmapAddRGBA(cmap, rval, gval, bval, aval);
    }

    return cmap;
}


/*!
 *  pixcmapConvertToHex()
 *
 *      Input:  data  (binary serialized data)
 *              ncolors (in colormap)
 *      Return: hexdata (bracketed, space-separated ascii hex string),
 *                       or null on error.
 *
 *  Notes:
 *      (1) The number of bytes in @data is 3 * ncolors.
 *      (2) Output is in form:
 *             < r0g0b0 r1g1b1 ... rngnbn >
 *          where r0, g0, b0 ... are each 2 bytes of hex ascii
 *      (3) This is used in pdf files to express the colormap as an
 *          array in ascii (human-readable) format.
 */
char *
pixcmapConvertToHex(l_uint8 *data,
                    l_int32  ncolors)
{
l_int32  i, j, hexbytes;
char    *hexdata = NULL;
char     buf[4];

    PROCNAME("pixcmapConvertToHex");

    if (!data)
        return (char *)ERROR_PTR("data not defined", procName, NULL);
    if (ncolors < 1)
        return (char *)ERROR_PTR("no colors", procName, NULL);

    hexbytes = 2 + (2 * 3 + 1) * ncolors + 2;
    hexdata = (char *)CALLOC(hexbytes, sizeof(char));
    hexdata[0] = '<';
    hexdata[1] = ' ';

    for (i = 0; i < ncolors; i++) {
        j = 2 + (2 * 3 + 1) * i;
        snprintf(buf, sizeof(buf), "%02x", data[3 * i]);
        hexdata[j] = buf[0];
        hexdata[j + 1] = buf[1];
        snprintf(buf, sizeof(buf), "%02x", data[3 * i + 1]);
        hexdata[j + 2] = buf[0];
        hexdata[j + 3] = buf[1];
        snprintf(buf, sizeof(buf), "%02x", data[3 * i + 2]);
        hexdata[j + 4] = buf[0];
        hexdata[j + 5] = buf[1];
        hexdata[j + 6] = ' ';
    }
    hexdata[j + 7] = '>';
    hexdata[j + 8] = '\0';
    return hexdata;
}


/*-------------------------------------------------------------*
 *                     Colormap transforms                     *
 *-------------------------------------------------------------*/
/*!
 *  pixcmapGammaTRC()
 *
 *      Input:  colormap
 *              gamma (gamma correction; must be > 0.0)
 *              minval  (input value that gives 0 for output; can be < 0)
 *              maxval  (input value that gives 255 for output; can be > 255)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This is an in-place transform
 *      (2) See pixGammaTRC() and numaGammaTRC() in enhance.c
 *          for description and use of transform
 */
l_int32
pixcmapGammaTRC(PIXCMAP   *cmap,
                l_float32  gamma,
                l_int32    minval,
                l_int32    maxval)
{
l_int32  rval, gval, bval, trval, tgval, tbval, i, ncolors;
NUMA    *nag;

    PROCNAME("pixcmapGammaTRC");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    if (gamma <= 0.0) {
        L_WARNING("gamma must be > 0.0; setting to 1.0\n", procName);
        gamma = 1.0;
    }
    if (minval >= maxval)
        return ERROR_INT("minval not < maxval", procName, 1);

    if (gamma == 1.0 && minval == 0 && maxval == 255)  /* no-op */
        return 0;

    if ((nag = numaGammaTRC(gamma, minval, maxval)) == NULL)
        return ERROR_INT("nag not made", procName, 1);

    ncolors = pixcmapGetCount(cmap);
    for (i = 0; i < ncolors; i++) {
        pixcmapGetColor(cmap, i, &rval, &gval, &bval);
        numaGetIValue(nag, rval, &trval);
        numaGetIValue(nag, gval, &tgval);
        numaGetIValue(nag, bval, &tbval);
        pixcmapResetColor(cmap, i, trval, tgval, tbval);
    }

    numaDestroy(&nag);
    return 0;
}


/*!
 *  pixcmapContrastTRC()
 *
 *      Input:  colormap
 *              factor (generally between 0.0 (no enhancement)
 *                      and 1.0, but can be larger than 1.0)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This is an in-place transform
 *      (2) See pixContrastTRC() and numaContrastTRC() in enhance.c
 *          for description and use of transform
 */
l_int32
pixcmapContrastTRC(PIXCMAP   *cmap,
                   l_float32  factor)
{
l_int32  i, ncolors, rval, gval, bval, trval, tgval, tbval;
NUMA    *nac;

    PROCNAME("pixcmapContrastTRC");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    if (factor < 0.0) {
        L_WARNING("factor must be >= 0.0; setting to 0.0\n", procName);
        factor = 0.0;
    }

    if ((nac = numaContrastTRC(factor)) == NULL)
        return ERROR_INT("nac not made", procName, 1);

    ncolors = pixcmapGetCount(cmap);
    for (i = 0; i < ncolors; i++) {
        pixcmapGetColor(cmap, i, &rval, &gval, &bval);
        numaGetIValue(nac, rval, &trval);
        numaGetIValue(nac, gval, &tgval);
        numaGetIValue(nac, bval, &tbval);
        pixcmapResetColor(cmap, i, trval, tgval, tbval);
    }

    numaDestroy(&nac);
    return 0;
}


/*!
 *  pixcmapShiftIntensity()
 *
 *      Input:  colormap
 *              fraction (between -1.0 and +1.0)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This is an in-place transform
 *      (2) It does a proportional shift of the intensity for each color.
 *      (3) If fraction < 0.0, it moves all colors towards (0,0,0).
 *          This darkens the image.
 *          If fraction > 0.0, it moves all colors towards (255,255,255)
 *          This fades the image.
 *      (4) The equivalent transform can be accomplished with pixcmapGammaTRC(),
 *          but it is considerably more difficult (see numaGammaTRC()).
 */
l_int32
pixcmapShiftIntensity(PIXCMAP   *cmap,
                      l_float32  fraction)
{
l_int32  i, ncolors, rval, gval, bval;

    PROCNAME("pixcmapShiftIntensity");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);
    if (fraction < -1.0 || fraction > 1.0)
        return ERROR_INT("fraction not in [-1.0, 1.0]", procName, 1);

    ncolors = pixcmapGetCount(cmap);
    for (i = 0; i < ncolors; i++) {
        pixcmapGetColor(cmap, i, &rval, &gval, &bval);
        if (fraction < 0.0)
            pixcmapResetColor(cmap, i,
                              (l_int32)((1.0 + fraction) * rval),
                              (l_int32)((1.0 + fraction) * gval),
                              (l_int32)((1.0 + fraction) * bval));
        else
            pixcmapResetColor(cmap, i,
                              rval + (l_int32)(fraction * (255 - rval)),
                              gval + (l_int32)(fraction * (255 - gval)),
                              bval + (l_int32)(fraction * (255 - bval)));
    }

    return 0;
}


/*!
 *  pixcmapShiftByComponent()
 *
 *      Input:  colormap
 *              srcval (source color: 0xrrggbb00)
 *              dstval (target color: 0xrrggbb00)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This is an in-place transform
 *      (2) It implements pixelShiftByComponent() for each color.
 *          The mapping is specified by srcval and dstval.
 *      (3) If a component decreases, the component in the colormap
 *          decreases by the same ratio.  Likewise for increasing, except
 *          all ratios are taken with respect to the distance from 255.
 */
l_int32
pixcmapShiftByComponent(PIXCMAP  *cmap,
                        l_uint32  srcval,
                        l_uint32  dstval)
{
l_int32   i, ncolors, rval, gval, bval;
l_uint32  newval;

    PROCNAME("pixcmapShiftByComponent");

    if (!cmap)
        return ERROR_INT("cmap not defined", procName, 1);

    ncolors = pixcmapGetCount(cmap);
    for (i = 0; i < ncolors; i++) {
        pixcmapGetColor(cmap, i, &rval, &gval, &bval);
        pixelShiftByComponent(rval, gval, bval, srcval, dstval, &newval);
        extractRGBValues(newval, &rval, &gval, &bval);
        pixcmapResetColor(cmap, i, rval, gval, bval);
    }

    return 0;
}
