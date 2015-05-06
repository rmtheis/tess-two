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
 *  graymorph.c
 *
 *      Top-level binary morphological operations (van Herk / Gil-Werman)
 *            PIX     *pixErodeGray()
 *            PIX     *pixDilateGray()
 *            PIX     *pixOpenGray()
 *            PIX     *pixCloseGray()
 *
 *      Special operations for 1x3, 3x1 and 3x3 Sels  (direct)
 *            PIX     *pixErodeGray3()
 *            PIX     *pixDilateGray3()
 *            PIX     *pixOpenGray3()
 *            PIX     *pixCloseGray3()
 *
 *
 *      Method: Algorithm by van Herk and Gil and Werman, 1992
 *
 *      Measured speed of the vH/G-W implementation is about 1 output
 *      pixel per 120 PIII clock cycles, for a horizontal or vertical
 *      erosion or dilation.  The computation time doubles for opening
 *      or closing, or for a square SE, as expected, and is independent
 *      of the size of the SE.
 *
 *      A faster implementation can be made directly for brick Sels
 *      of maximum size 3.  We unroll the computation for sets of 8 bytes.
 *      It needs to be called explicitly; the general functions do not
 *      default for the size 3 brick Sels.
 */

#include "allheaders.h"

static PIX *pixErodeGray3h(PIX *pixs);
static PIX *pixErodeGray3v(PIX *pixs);
static PIX *pixDilateGray3h(PIX *pixs);
static PIX *pixDilateGray3v(PIX *pixs);


/*-----------------------------------------------------------------*
 *              Top-level gray morphological operations            *
 *-----------------------------------------------------------------*/
/*!
 *  pixErodeGray()
 *
 *      Input:  pixs
 *              hsize  (of Sel; must be odd; origin implicitly in center)
 *              vsize  (ditto)
 *      Return: pixd
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) If hsize = vsize = 1, just returns a copy.
 */
PIX *
pixErodeGray(PIX     *pixs,
             l_int32  hsize,
             l_int32  vsize)
{
l_uint8   *buffer, *minarray;
l_int32    w, h, wplb, wplt;
l_int32    leftpix, rightpix, toppix, bottompix, maxsize;
l_uint32  *datab, *datat;
PIX       *pixb, *pixt, *pixd;

    PROCNAME("pixErodeGray");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize or vsize < 1", procName, NULL);
    if ((hsize & 1) == 0 ) {
        L_WARNING("horiz sel size must be odd; increasing by 1\n", procName);
        hsize++;
    }
    if ((vsize & 1) == 0 ) {
        L_WARNING("vert sel size must be odd; increasing by 1\n", procName);
        vsize++;
    }

    if (hsize == 1 && vsize == 1)
        return pixCopy(NULL, pixs);

    if (vsize == 1) {  /* horizontal sel */
        leftpix = (hsize + 1) / 2;
        rightpix = (3 * hsize + 1) / 2;
        toppix = 0;
        bottompix = 0;
    } else if (hsize == 1) {  /* vertical sel */
        leftpix = 0;
        rightpix = 0;
        toppix = (vsize + 1) / 2;
        bottompix = (3 * vsize + 1) / 2;
    } else {
        leftpix = (hsize + 1) / 2;
        rightpix = (3 * hsize + 1) / 2;
        toppix = (vsize + 1) / 2;
        bottompix = (3 * vsize + 1) / 2;
    }

    if ((pixb = pixAddBorderGeneral(pixs,
                leftpix, rightpix, toppix, bottompix, 255)) == NULL)
        return (PIX *)ERROR_PTR("pixb not made", procName, NULL);
    if ((pixt = pixCreateTemplate(pixb)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, NULL);

    pixGetDimensions(pixt, &w, &h, NULL);
    datab = pixGetData(pixb);
    datat = pixGetData(pixt);
    wplb = pixGetWpl(pixb);
    wplt = pixGetWpl(pixt);

    if ((buffer = (l_uint8 *)CALLOC(L_MAX(w, h), sizeof(l_uint8))) == NULL)
        return (PIX *)ERROR_PTR("buffer not made", procName, NULL);
    maxsize = L_MAX(hsize, vsize);
    if ((minarray = (l_uint8 *)CALLOC(2 * maxsize, sizeof(l_uint8))) == NULL)
        return (PIX *)ERROR_PTR("minarray not made", procName, NULL);

    if (vsize == 1) {
        erodeGrayLow(datat, w, h, wplt, datab, wplb, hsize, L_HORIZ,
                     buffer, minarray);
    } else if (hsize == 1) {
        erodeGrayLow(datat, w, h, wplt, datab, wplb, vsize, L_VERT,
                     buffer, minarray);
    } else {
        erodeGrayLow(datat, w, h, wplt, datab, wplb, hsize, L_HORIZ,
                     buffer, minarray);
        pixSetOrClearBorder(pixt, leftpix, rightpix, toppix, bottompix,
                            PIX_SET);
        erodeGrayLow(datab, w, h, wplb, datat, wplt, vsize, L_VERT,
                     buffer, minarray);
        pixDestroy(&pixt);
        pixt = pixClone(pixb);
    }

    if ((pixd = pixRemoveBorderGeneral(pixt,
                leftpix, rightpix, toppix, bottompix)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

    FREE(buffer);
    FREE(minarray);
    pixDestroy(&pixb);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixDilateGray()
 *
 *      Input:  pixs
 *              hsize  (of Sel; must be odd; origin implicitly in center)
 *              vsize  (ditto)
 *      Return: pixd
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) If hsize = vsize = 1, just returns a copy.
 */
PIX *
pixDilateGray(PIX     *pixs,
              l_int32  hsize,
              l_int32  vsize)
{
l_uint8   *buffer, *maxarray;
l_int32    w, h, wplb, wplt;
l_int32    leftpix, rightpix, toppix, bottompix, maxsize;
l_uint32  *datab, *datat;
PIX       *pixb, *pixt, *pixd;

    PROCNAME("pixDilateGray");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize or vsize < 1", procName, NULL);
    if ((hsize & 1) == 0 ) {
        L_WARNING("horiz sel size must be odd; increasing by 1\n", procName);
        hsize++;
    }
    if ((vsize & 1) == 0 ) {
        L_WARNING("vert sel size must be odd; increasing by 1\n", procName);
        vsize++;
    }

    if (hsize == 1 && vsize == 1)
        return pixCopy(NULL, pixs);

    if (vsize == 1) {  /* horizontal sel */
        leftpix = (hsize + 1) / 2;
        rightpix = (3 * hsize + 1) / 2;
        toppix = 0;
        bottompix = 0;
    } else if (hsize == 1) {  /* vertical sel */
        leftpix = 0;
        rightpix = 0;
        toppix = (vsize + 1) / 2;
        bottompix = (3 * vsize + 1) / 2;
    } else {
        leftpix = (hsize + 1) / 2;
        rightpix = (3 * hsize + 1) / 2;
        toppix = (vsize + 1) / 2;
        bottompix = (3 * vsize + 1) / 2;
    }

    if ((pixb = pixAddBorderGeneral(pixs,
                leftpix, rightpix, toppix, bottompix, 0)) == NULL)
        return (PIX *)ERROR_PTR("pixb not made", procName, NULL);
    if ((pixt = pixCreateTemplate(pixb)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, NULL);

    pixGetDimensions(pixt, &w, &h, NULL);
    datab = pixGetData(pixb);
    datat = pixGetData(pixt);
    wplb = pixGetWpl(pixb);
    wplt = pixGetWpl(pixt);

    if ((buffer = (l_uint8 *)CALLOC(L_MAX(w, h), sizeof(l_uint8))) == NULL)
        return (PIX *)ERROR_PTR("buffer not made", procName, NULL);
    maxsize = L_MAX(hsize, vsize);
    if ((maxarray = (l_uint8 *)CALLOC(2 * maxsize, sizeof(l_uint8))) == NULL)
        return (PIX *)ERROR_PTR("buffer not made", procName, NULL);

    if (vsize == 1) {
        dilateGrayLow(datat, w, h, wplt, datab, wplb, hsize, L_HORIZ,
                      buffer, maxarray);
    } else if (hsize == 1) {
        dilateGrayLow(datat, w, h, wplt, datab, wplb, vsize, L_VERT,
                      buffer, maxarray);
    } else {
        dilateGrayLow(datat, w, h, wplt, datab, wplb, hsize, L_HORIZ,
                      buffer, maxarray);
        pixSetOrClearBorder(pixt, leftpix, rightpix, toppix, bottompix,
                            PIX_CLR);
        dilateGrayLow(datab, w, h, wplb, datat, wplt, vsize, L_VERT,
                      buffer, maxarray);
        pixDestroy(&pixt);
        pixt = pixClone(pixb);
    }

    if ((pixd = pixRemoveBorderGeneral(pixt,
                leftpix, rightpix, toppix, bottompix)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

    FREE(buffer);
    FREE(maxarray);
    pixDestroy(&pixb);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixOpenGray()
 *
 *      Input:  pixs
 *              hsize  (of Sel; must be odd; origin implicitly in center)
 *              vsize  (ditto)
 *      Return: pixd
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) If hsize = vsize = 1, just returns a copy.
 */
PIX *
pixOpenGray(PIX     *pixs,
            l_int32  hsize,
            l_int32  vsize)
{
l_uint8   *buffer;
l_uint8   *array;  /* used to find either min or max in interval */
l_int32    w, h, wplb, wplt;
l_int32    leftpix, rightpix, toppix, bottompix, maxsize;
l_uint32  *datab, *datat;
PIX       *pixb, *pixt, *pixd;

    PROCNAME("pixOpenGray");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize or vsize < 1", procName, NULL);
    if ((hsize & 1) == 0 ) {
        L_WARNING("horiz sel size must be odd; increasing by 1\n", procName);
        hsize++;
    }
    if ((vsize & 1) == 0 ) {
        L_WARNING("vert sel size must be odd; increasing by 1\n", procName);
        vsize++;
    }

    if (hsize == 1 && vsize == 1)
        return pixCopy(NULL, pixs);

    if (vsize == 1) {  /* horizontal sel */
        leftpix = (hsize + 1) / 2;
        rightpix = (3 * hsize + 1) / 2;
        toppix = 0;
        bottompix = 0;
    } else if (hsize == 1) {  /* vertical sel */
        leftpix = 0;
        rightpix = 0;
        toppix = (vsize + 1) / 2;
        bottompix = (3 * vsize + 1) / 2;
    } else {
        leftpix = (hsize + 1) / 2;
        rightpix = (3 * hsize + 1) / 2;
        toppix = (vsize + 1) / 2;
        bottompix = (3 * vsize + 1) / 2;
    }

    if ((pixb = pixAddBorderGeneral(pixs,
                leftpix, rightpix, toppix, bottompix, 255)) == NULL)
        return (PIX *)ERROR_PTR("pixb not made", procName, NULL);
    if ((pixt = pixCreateTemplate(pixb)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, NULL);

    pixGetDimensions(pixt, &w, &h, NULL);
    datab = pixGetData(pixb);
    datat = pixGetData(pixt);
    wplb = pixGetWpl(pixb);
    wplt = pixGetWpl(pixt);

    if ((buffer = (l_uint8 *)CALLOC(L_MAX(w, h), sizeof(l_uint8))) == NULL)
        return (PIX *)ERROR_PTR("buffer not made", procName, NULL);
    maxsize = L_MAX(hsize, vsize);
    if ((array = (l_uint8 *)CALLOC(2 * maxsize, sizeof(l_uint8))) == NULL)
        return (PIX *)ERROR_PTR("array not made", procName, NULL);

    if (vsize == 1) {
        erodeGrayLow(datat, w, h, wplt, datab, wplb, hsize, L_HORIZ,
                     buffer, array);
        pixSetOrClearBorder(pixt, leftpix, rightpix, toppix, bottompix,
                            PIX_CLR);
        dilateGrayLow(datab, w, h, wplb, datat, wplt, hsize, L_HORIZ,
                      buffer, array);
    }
    else if (hsize == 1) {
        erodeGrayLow(datat, w, h, wplt, datab, wplb, vsize, L_VERT,
                     buffer, array);
        pixSetOrClearBorder(pixt, leftpix, rightpix, toppix, bottompix,
                            PIX_CLR);
        dilateGrayLow(datab, w, h, wplb, datat, wplt, vsize, L_VERT,
                      buffer, array);
    } else {
        erodeGrayLow(datat, w, h, wplt, datab, wplb, hsize, L_HORIZ,
                     buffer, array);
        pixSetOrClearBorder(pixt, leftpix, rightpix, toppix, bottompix,
                            PIX_SET);
        erodeGrayLow(datab, w, h, wplb, datat, wplt, vsize, L_VERT,
                     buffer, array);
        pixSetOrClearBorder(pixb, leftpix, rightpix, toppix, bottompix,
                            PIX_CLR);
        dilateGrayLow(datat, w, h, wplt, datab, wplb, hsize, L_HORIZ,
                      buffer, array);
        pixSetOrClearBorder(pixt, leftpix, rightpix, toppix, bottompix,
                            PIX_CLR);
        dilateGrayLow(datab, w, h, wplb, datat, wplt, vsize, L_VERT,
                      buffer, array);
    }

    if ((pixd = pixRemoveBorderGeneral(pixb,
                leftpix, rightpix, toppix, bottompix)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

    FREE(buffer);
    FREE(array);
    pixDestroy(&pixb);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixCloseGray()
 *
 *      Input:  pixs
 *              hsize  (of Sel; must be odd; origin implicitly in center)
 *              vsize  (ditto)
 *      Return: pixd
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) If hsize = vsize = 1, just returns a copy.
 */
PIX *
pixCloseGray(PIX     *pixs,
             l_int32  hsize,
             l_int32  vsize)
{
l_uint8   *buffer;
l_uint8   *array;  /* used to find either min or max in interval */
l_int32    w, h, wplb, wplt;
l_int32    leftpix, rightpix, toppix, bottompix, maxsize;
l_uint32  *datab, *datat;
PIX       *pixb, *pixt, *pixd;

    PROCNAME("pixCloseGray");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize or vsize < 1", procName, NULL);
    if ((hsize & 1) == 0 ) {
        L_WARNING("horiz sel size must be odd; increasing by 1\n", procName);
        hsize++;
    }
    if ((vsize & 1) == 0 ) {
        L_WARNING("vert sel size must be odd; increasing by 1\n", procName);
        vsize++;
    }

    if (hsize == 1 && vsize == 1)
        return pixCopy(NULL, pixs);

    if (vsize == 1) {  /* horizontal sel */
        leftpix = (hsize + 1) / 2;
        rightpix = (3 * hsize + 1) / 2;
        toppix = 0;
        bottompix = 0;
    } else if (hsize == 1) {  /* vertical sel */
        leftpix = 0;
        rightpix = 0;
        toppix = (vsize + 1) / 2;
        bottompix = (3 * vsize + 1) / 2;
    } else {
        leftpix = (hsize + 1) / 2;
        rightpix = (3 * hsize + 1) / 2;
        toppix = (vsize + 1) / 2;
        bottompix = (3 * vsize + 1) / 2;
    }

    if ((pixb = pixAddBorderGeneral(pixs,
                leftpix, rightpix, toppix, bottompix, 0)) == NULL)
        return (PIX *)ERROR_PTR("pixb not made", procName, NULL);
    if ((pixt = pixCreateTemplate(pixb)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, NULL);

    pixGetDimensions(pixt, &w, &h, NULL);
    datab = pixGetData(pixb);
    datat = pixGetData(pixt);
    wplb = pixGetWpl(pixb);
    wplt = pixGetWpl(pixt);

    if ((buffer = (l_uint8 *)CALLOC(L_MAX(w, h), sizeof(l_uint8))) == NULL)
        return (PIX *)ERROR_PTR("buffer not made", procName, NULL);
    maxsize = L_MAX(hsize, vsize);
    if ((array = (l_uint8 *)CALLOC(2 * maxsize, sizeof(l_uint8))) == NULL)
        return (PIX *)ERROR_PTR("array not made", procName, NULL);

    if (vsize == 1) {
        dilateGrayLow(datat, w, h, wplt, datab, wplb, hsize, L_HORIZ,
                     buffer, array);
        pixSetOrClearBorder(pixt, leftpix, rightpix, toppix, bottompix,
                            PIX_SET);
        erodeGrayLow(datab, w, h, wplb, datat, wplt, hsize, L_HORIZ,
                      buffer, array);
    } else if (hsize == 1) {
        dilateGrayLow(datat, w, h, wplt, datab, wplb, vsize, L_VERT,
                     buffer, array);
        pixSetOrClearBorder(pixt, leftpix, rightpix, toppix, bottompix,
                            PIX_SET);
        erodeGrayLow(datab, w, h, wplb, datat, wplt, vsize, L_VERT,
                      buffer, array);
    } else {
        dilateGrayLow(datat, w, h, wplt, datab, wplb, hsize, L_HORIZ,
                      buffer, array);
        pixSetOrClearBorder(pixt, leftpix, rightpix, toppix, bottompix,
                            PIX_CLR);
        dilateGrayLow(datab, w, h, wplb, datat, wplt, vsize, L_VERT,
                      buffer, array);
        pixSetOrClearBorder(pixb, leftpix, rightpix, toppix, bottompix,
                            PIX_SET);
        erodeGrayLow(datat, w, h, wplt, datab, wplb, hsize, L_HORIZ,
                     buffer, array);
        pixSetOrClearBorder(pixt, leftpix, rightpix, toppix, bottompix,
                            PIX_SET);
        erodeGrayLow(datab, w, h, wplb, datat, wplt, vsize, L_VERT,
                     buffer, array);
    }

    if ((pixd = pixRemoveBorderGeneral(pixb,
                leftpix, rightpix, toppix, bottompix)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

    FREE(buffer);
    FREE(array);
    pixDestroy(&pixb);
    pixDestroy(&pixt);
    return pixd;
}


/*-----------------------------------------------------------------*
 *           Special operations for 1x3, 3x1 and 3x3 Sels          *
 *-----------------------------------------------------------------*/
/*!
 *  pixErodeGray3()
 *
 *      Input:  pixs (8 bpp, not cmapped)
 *              hsize  (1 or 3)
 *              vsize  (1 or 3)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Special case for 1x3, 3x1 or 3x3 brick sel (all hits)
 *      (2) If hsize = vsize = 1, just returns a copy.
 *      (3) It would be nice not to add a border, but it is required
 *          if we want the same results as from the general case.
 *          We add 4 bytes on the left to speed up the copying, and
 *          8 bytes at the right and bottom to allow unrolling of
 *          the computation of 8 pixels.
 */
PIX *
pixErodeGray3(PIX     *pixs,
              l_int32  hsize,
              l_int32  vsize)
{
PIX  *pixt, *pixb, *pixbd, *pixd;

    PROCNAME("pixErodeGray3");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pix has colormap", procName, NULL);
    if ((hsize != 1 && hsize != 3) ||
        (vsize != 1 && vsize != 3))
        return (PIX *)ERROR_PTR("invalid size: must be 1 or 3", procName, NULL);

    if (hsize == 1 && vsize == 1)
        return pixCopy(NULL, pixs);

    pixb = pixAddBorderGeneral(pixs, 4, 8, 2, 8, 255);

    if (vsize == 1)
        pixbd = pixErodeGray3h(pixb);
    else if (hsize == 1)
        pixbd = pixErodeGray3v(pixb);
    else {  /* vize == hsize == 3 */
        pixt = pixErodeGray3h(pixb);
        pixbd = pixErodeGray3v(pixt);
        pixDestroy(&pixt);
    }

    pixd = pixRemoveBorderGeneral(pixbd, 4, 8, 2, 8);
    pixDestroy(&pixb);
    pixDestroy(&pixbd);
    return pixd;
}


/*!
 *  pixErodeGray3h()
 *
 *      Input:  pixs (8 bpp, not cmapped)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Special case for horizontal 3x1 brick Sel;
 *          also used as the first step for the 3x3 brick Sel.
 */
static PIX *
pixErodeGray3h(PIX  *pixs)
{
l_uint32  *datas, *datad, *lines, *lined;
l_int32    w, h, wpl, i, j;
l_int32    val0, val1, val2, val3, val4, val5, val6, val7, val8, val9, minval;
PIX       *pixd;

    PROCNAME("pixErodeGray3h");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);

    pixd = pixCreateTemplateNoInit(pixs);
    pixSetBorderVal(pixd, 4, 8, 2, 8, 0);  /* only to silence valgrind */
    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpl = pixGetWpl(pixs);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpl;
        lined = datad + i * wpl;
        for (j = 1; j < w - 8; j += 8) {
            val0 = GET_DATA_BYTE(lines, j - 1);
            val1 = GET_DATA_BYTE(lines, j);
            val2 = GET_DATA_BYTE(lines, j + 1);
            val3 = GET_DATA_BYTE(lines, j + 2);
            val4 = GET_DATA_BYTE(lines, j + 3);
            val5 = GET_DATA_BYTE(lines, j + 4);
            val6 = GET_DATA_BYTE(lines, j + 5);
            val7 = GET_DATA_BYTE(lines, j + 6);
            val8 = GET_DATA_BYTE(lines, j + 7);
            val9 = GET_DATA_BYTE(lines, j + 8);
            minval = L_MIN(val1, val2);
            SET_DATA_BYTE(lined, j, L_MIN(val0, minval));
            SET_DATA_BYTE(lined, j + 1, L_MIN(minval, val3));
            minval = L_MIN(val3, val4);
            SET_DATA_BYTE(lined, j + 2, L_MIN(val2, minval));
            SET_DATA_BYTE(lined, j + 3, L_MIN(minval, val5));
            minval = L_MIN(val5, val6);
            SET_DATA_BYTE(lined, j + 4, L_MIN(val4, minval));
            SET_DATA_BYTE(lined, j + 5, L_MIN(minval, val7));
            minval = L_MIN(val7, val8);
            SET_DATA_BYTE(lined, j + 6, L_MIN(val6, minval));
            SET_DATA_BYTE(lined, j + 7, L_MIN(minval, val9));
        }
    }
    return pixd;
}


/*!
 *  pixErodeGray3v()
 *
 *      Input:  pixs (8 bpp, not cmapped)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Special case for vertical 1x3 brick Sel;
 *          also used as the second step for the 3x3 brick Sel.
 *      (2) Surprisingly, this is faster than setting up the
 *          lineptrs array and accessing into it; e.g.,
 *              val4 = GET_DATA_BYTE(lines8[i + 3], j);
 */
static PIX *
pixErodeGray3v(PIX  *pixs)
{
l_uint32  *datas, *datad, *linesi, *linedi;
l_int32    w, h, wpl, i, j;
l_int32    val0, val1, val2, val3, val4, val5, val6, val7, val8, val9, minval;
PIX       *pixd;

    PROCNAME("pixErodeGray3v");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);

    pixd = pixCreateTemplateNoInit(pixs);
    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpl = pixGetWpl(pixs);
    for (j = 0; j < w; j++) {
        for (i = 1; i < h - 8; i += 8) {
            linesi = datas + i * wpl;
            linedi = datad + i * wpl;
            val0 = GET_DATA_BYTE(linesi - wpl, j);
            val1 = GET_DATA_BYTE(linesi, j);
            val2 = GET_DATA_BYTE(linesi + wpl, j);
            val3 = GET_DATA_BYTE(linesi + 2 * wpl, j);
            val4 = GET_DATA_BYTE(linesi + 3 * wpl, j);
            val5 = GET_DATA_BYTE(linesi + 4 * wpl, j);
            val6 = GET_DATA_BYTE(linesi + 5 * wpl, j);
            val7 = GET_DATA_BYTE(linesi + 6 * wpl, j);
            val8 = GET_DATA_BYTE(linesi + 7 * wpl, j);
            val9 = GET_DATA_BYTE(linesi + 8 * wpl, j);
            minval = L_MIN(val1, val2);
            SET_DATA_BYTE(linedi, j, L_MIN(val0, minval));
            SET_DATA_BYTE(linedi + wpl, j, L_MIN(minval, val3));
            minval = L_MIN(val3, val4);
            SET_DATA_BYTE(linedi + 2 * wpl, j, L_MIN(val2, minval));
            SET_DATA_BYTE(linedi + 3 * wpl, j, L_MIN(minval, val5));
            minval = L_MIN(val5, val6);
            SET_DATA_BYTE(linedi + 4 * wpl, j, L_MIN(val4, minval));
            SET_DATA_BYTE(linedi + 5 * wpl, j, L_MIN(minval, val7));
            minval = L_MIN(val7, val8);
            SET_DATA_BYTE(linedi + 6 * wpl, j, L_MIN(val6, minval));
            SET_DATA_BYTE(linedi + 7 * wpl, j, L_MIN(minval, val9));
        }
    }
    return pixd;
}


/*!
 *  pixDilateGray3()
 *
 *      Input:  pixs (8 bpp, not cmapped)
 *              hsize  (1 or 3)
 *              vsize  (1 or 3)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Special case for 1x3, 3x1 or 3x3 brick sel (all hits)
 *      (2) If hsize = vsize = 1, just returns a copy.
 */
PIX *
pixDilateGray3(PIX     *pixs,
               l_int32  hsize,
               l_int32  vsize)
{
PIX  *pixt, *pixb, *pixbd, *pixd;

    PROCNAME("pixDilateGray3");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pix has colormap", procName, NULL);
    if ((hsize != 1 && hsize != 3) ||
        (vsize != 1 && vsize != 3))
        return (PIX *)ERROR_PTR("invalid size: must be 1 or 3", procName, NULL);

    if (hsize == 1 && vsize == 1)
        return pixCopy(NULL, pixs);

    pixb = pixAddBorderGeneral(pixs, 4, 8, 2, 8, 0);

    if (vsize == 1)
        pixbd = pixDilateGray3h(pixb);
    else if (hsize == 1)
        pixbd = pixDilateGray3v(pixb);
    else {  /* vize == hsize == 3 */
        pixt = pixDilateGray3h(pixb);
        pixbd = pixDilateGray3v(pixt);
        pixDestroy(&pixt);
    }

    pixd = pixRemoveBorderGeneral(pixbd, 4, 8, 2, 8);
    pixDestroy(&pixb);
    pixDestroy(&pixbd);
    return pixd;
}


/*!
 *  pixDilateGray3h()
 *
 *      Input:  pixs (8 bpp, not cmapped)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Special case for horizontal 3x1 brick Sel;
 *          also used as the first step for the 3x3 brick Sel.
 */
static PIX *
pixDilateGray3h(PIX  *pixs)
{
l_uint32  *datas, *datad, *lines, *lined;
l_int32    w, h, wpl, i, j;
l_int32    val0, val1, val2, val3, val4, val5, val6, val7, val8, val9, maxval;
PIX       *pixd;

    PROCNAME("pixDilateGray3h");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);

    pixd = pixCreateTemplateNoInit(pixs);
    pixSetBorderVal(pixd, 4, 8, 2, 8, 0);  /* only to silence valgrind */
    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpl = pixGetWpl(pixs);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpl;
        lined = datad + i * wpl;
        for (j = 1; j < w - 8; j += 8) {
            val0 = GET_DATA_BYTE(lines, j - 1);
            val1 = GET_DATA_BYTE(lines, j);
            val2 = GET_DATA_BYTE(lines, j + 1);
            val3 = GET_DATA_BYTE(lines, j + 2);
            val4 = GET_DATA_BYTE(lines, j + 3);
            val5 = GET_DATA_BYTE(lines, j + 4);
            val6 = GET_DATA_BYTE(lines, j + 5);
            val7 = GET_DATA_BYTE(lines, j + 6);
            val8 = GET_DATA_BYTE(lines, j + 7);
            val9 = GET_DATA_BYTE(lines, j + 8);
            maxval = L_MAX(val1, val2);
            SET_DATA_BYTE(lined, j, L_MAX(val0, maxval));
            SET_DATA_BYTE(lined, j + 1, L_MAX(maxval, val3));
            maxval = L_MAX(val3, val4);
            SET_DATA_BYTE(lined, j + 2, L_MAX(val2, maxval));
            SET_DATA_BYTE(lined, j + 3, L_MAX(maxval, val5));
            maxval = L_MAX(val5, val6);
            SET_DATA_BYTE(lined, j + 4, L_MAX(val4, maxval));
            SET_DATA_BYTE(lined, j + 5, L_MAX(maxval, val7));
            maxval = L_MAX(val7, val8);
            SET_DATA_BYTE(lined, j + 6, L_MAX(val6, maxval));
            SET_DATA_BYTE(lined, j + 7, L_MAX(maxval, val9));
        }
    }
    return pixd;
}


/*!
 *  pixDilateGray3v()
 *
 *      Input:  pixs (8 bpp, not cmapped)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Special case for vertical 1x3 brick Sel;
 *          also used as the second step for the 3x3 brick Sel.
 */
static PIX *
pixDilateGray3v(PIX  *pixs)
{
l_uint32  *datas, *datad, *linesi, *linedi;
l_int32    w, h, wpl, i, j;
l_int32    val0, val1, val2, val3, val4, val5, val6, val7, val8, val9, maxval;
PIX       *pixd;

    PROCNAME("pixDilateGray3v");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);

    pixd = pixCreateTemplateNoInit(pixs);
    pixGetDimensions(pixs, &w, &h, NULL);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpl = pixGetWpl(pixs);
    for (j = 0; j < w; j++) {
        for (i = 1; i < h - 8; i += 8) {
            linesi = datas + i * wpl;
            linedi = datad + i * wpl;
            val0 = GET_DATA_BYTE(linesi - wpl, j);
            val1 = GET_DATA_BYTE(linesi, j);
            val2 = GET_DATA_BYTE(linesi + wpl, j);
            val3 = GET_DATA_BYTE(linesi + 2 * wpl, j);
            val4 = GET_DATA_BYTE(linesi + 3 * wpl, j);
            val5 = GET_DATA_BYTE(linesi + 4 * wpl, j);
            val6 = GET_DATA_BYTE(linesi + 5 * wpl, j);
            val7 = GET_DATA_BYTE(linesi + 6 * wpl, j);
            val8 = GET_DATA_BYTE(linesi + 7 * wpl, j);
            val9 = GET_DATA_BYTE(linesi + 8 * wpl, j);
            maxval = L_MAX(val1, val2);
            SET_DATA_BYTE(linedi, j, L_MAX(val0, maxval));
            SET_DATA_BYTE(linedi + wpl, j, L_MAX(maxval, val3));
            maxval = L_MAX(val3, val4);
            SET_DATA_BYTE(linedi + 2 * wpl, j, L_MAX(val2, maxval));
            SET_DATA_BYTE(linedi + 3 * wpl, j, L_MAX(maxval, val5));
            maxval = L_MAX(val5, val6);
            SET_DATA_BYTE(linedi + 4 * wpl, j, L_MAX(val4, maxval));
            SET_DATA_BYTE(linedi + 5 * wpl, j, L_MAX(maxval, val7));
            maxval = L_MAX(val7, val8);
            SET_DATA_BYTE(linedi + 6 * wpl, j, L_MAX(val6, maxval));
            SET_DATA_BYTE(linedi + 7 * wpl, j, L_MAX(maxval, val9));
        }
    }
    return pixd;
}


/*!
 *  pixOpenGray3()
 *
 *      Input:  pixs (8 bpp, not cmapped)
 *              hsize  (1 or 3)
 *              vsize  (1 or 3)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Special case for 1x3, 3x1 or 3x3 brick sel (all hits)
 *      (2) If hsize = vsize = 1, just returns a copy.
 *      (3) It would be nice not to add a border, but it is required
 *          to get the same results as for the general case.
 */
PIX *
pixOpenGray3(PIX     *pixs,
             l_int32  hsize,
             l_int32  vsize)
{
PIX  *pixt, *pixb, *pixbd, *pixd;

    PROCNAME("pixOpenGray3");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pix has colormap", procName, NULL);
    if ((hsize != 1 && hsize != 3) ||
        (vsize != 1 && vsize != 3))
        return (PIX *)ERROR_PTR("invalid size: must be 1 or 3", procName, NULL);

    if (hsize == 1 && vsize == 1)
        return pixCopy(NULL, pixs);

    pixb = pixAddBorderGeneral(pixs, 4, 8, 2, 8, 255);  /* set to max */

    if (vsize == 1) {
        pixt = pixErodeGray3h(pixb);
        pixSetBorderVal(pixt, 4, 8, 2, 8, 0);  /* set to min */
        pixbd = pixDilateGray3h(pixt);
        pixDestroy(&pixt);
    } else if (hsize == 1) {
        pixt = pixErodeGray3v(pixb);
        pixSetBorderVal(pixt, 4, 8, 2, 8, 0);
        pixbd = pixDilateGray3v(pixt);
        pixDestroy(&pixt);
    } else {  /* vize == hsize == 3 */
        pixt = pixErodeGray3h(pixb);
        pixbd = pixErodeGray3v(pixt);
        pixDestroy(&pixt);
        pixSetBorderVal(pixbd, 4, 8, 2, 8, 0);
        pixt = pixDilateGray3h(pixbd);
        pixDestroy(&pixbd);
        pixbd = pixDilateGray3v(pixt);
        pixDestroy(&pixt);
    }

    pixd = pixRemoveBorderGeneral(pixbd, 4, 8, 2, 8);
    pixDestroy(&pixb);
    pixDestroy(&pixbd);
    return pixd;
}


/*!
 *  pixCloseGray3()
 *
 *      Input:  pixs (8 bpp, not cmapped)
 *              hsize  (1 or 3)
 *              vsize  (1 or 3)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Special case for 1x3, 3x1 or 3x3 brick sel (all hits)
 *      (2) If hsize = vsize = 1, just returns a copy.
 */
PIX *
pixCloseGray3(PIX     *pixs,
              l_int32  hsize,
              l_int32  vsize)
{
PIX  *pixt, *pixb, *pixbd, *pixd;

    PROCNAME("pixCloseGray3");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pix has colormap", procName, NULL);
    if ((hsize != 1 && hsize != 3) ||
        (vsize != 1 && vsize != 3))
        return (PIX *)ERROR_PTR("invalid size: must be 1 or 3", procName, NULL);

    if (hsize == 1 && vsize == 1)
        return pixCopy(NULL, pixs);

    pixb = pixAddBorderGeneral(pixs, 4, 8, 2, 8, 0);  /* set to min */

    if (vsize == 1) {
        pixt = pixDilateGray3h(pixb);
        pixSetBorderVal(pixt, 4, 8, 2, 8, 255);  /* set to max */
        pixbd = pixErodeGray3h(pixt);
        pixDestroy(&pixt);
    } else if (hsize == 1) {
        pixt = pixDilateGray3v(pixb);
        pixSetBorderVal(pixt, 4, 8, 2, 8, 255);
        pixbd = pixErodeGray3v(pixt);
        pixDestroy(&pixt);
    } else {  /* vize == hsize == 3 */
        pixt = pixDilateGray3h(pixb);
        pixbd = pixDilateGray3v(pixt);
        pixDestroy(&pixt);
        pixSetBorderVal(pixbd, 4, 8, 2, 8, 255);
        pixt = pixErodeGray3h(pixbd);
        pixDestroy(&pixbd);
        pixbd = pixErodeGray3v(pixt);
        pixDestroy(&pixt);
    }

    pixd = pixRemoveBorderGeneral(pixbd, 4, 8, 2, 8);
    pixDestroy(&pixb);
    pixDestroy(&pixbd);
    return pixd;
}
