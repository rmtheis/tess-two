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
 *  graymorph.c
 *
 *      Top-level binary morphological operations
 *
 *            PIX     *pixErodeGray()
 *            PIX     *pixDilateGray()
 *            PIX     *pixOpenGray()
 *            PIX     *pixCloseGray()
 *
 *
 *      Method: Algorithm by van Herk and Gil and Werman, 1992
 *
 *      Measured speed is about 1 output pixel per 120 PIII clock cycles,
 *      for a horizontal or vertical erosion or dilation.  The 
 *      computation time doubles for opening or closing, or for a
 *      square SE, as expected, and is independent of the size of the SE.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


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
        L_WARNING("horiz sel size must be odd; increasing by 1", procName);
        hsize++;
    }
    if ((vsize & 1) == 0 ) {
        L_WARNING("vert sel size must be odd; increasing by 1", procName);
        vsize++;
    }

    if (hsize == 1 && vsize == 1)
        return pixCopy(NULL, pixs);

    if (vsize == 1) {  /* horizontal sel */
        leftpix = (hsize + 1) / 2;
        rightpix = (3 * hsize + 1) / 2;
        toppix = 0;
        bottompix = 0;
    }
    else if (hsize == 1) {  /* vertical sel */
        leftpix = 0;
        rightpix = 0;
        toppix = (vsize + 1) / 2;
        bottompix = (3 * vsize + 1) / 2;
    }
    else {
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
    
    w = pixGetWidth(pixt);
    h = pixGetHeight(pixt);
    datab = pixGetData(pixb);
    datat = pixGetData(pixt);
    wplb = pixGetWpl(pixb);
    wplt = pixGetWpl(pixt);

    if ((buffer = (l_uint8 *)CALLOC(L_MAX(w, h), sizeof(l_uint8))) == NULL)
        return (PIX *)ERROR_PTR("buffer not made", procName, NULL);
    maxsize = L_MAX(hsize, vsize);
    if ((minarray = (l_uint8 *)CALLOC(2 * maxsize, sizeof(l_uint8))) == NULL)
        return (PIX *)ERROR_PTR("minarray not made", procName, NULL);

    if (vsize == 1)
        erodeGrayLow(datat, w, h, wplt, datab, wplb, hsize, L_HORIZ,
                     buffer, minarray);
    else if (hsize == 1)
        erodeGrayLow(datat, w, h, wplt, datab, wplb, vsize, L_VERT,
                     buffer, minarray);
    else {
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
        L_WARNING("horiz sel size must be odd; increasing by 1", procName);
        hsize++;
    }
    if ((vsize & 1) == 0 ) {
        L_WARNING("vert sel size must be odd; increasing by 1", procName);
        vsize++;
    }

    if (hsize == 1 && vsize == 1)
        return pixCopy(NULL, pixs);

    if (vsize == 1) {  /* horizontal sel */
        leftpix = (hsize + 1) / 2;
        rightpix = (3 * hsize + 1) / 2;
        toppix = 0;
        bottompix = 0;
    }
    else if (hsize == 1) {  /* vertical sel */
        leftpix = 0;
        rightpix = 0;
        toppix = (vsize + 1) / 2;
        bottompix = (3 * vsize + 1) / 2;
    }
    else {
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
    
    w = pixGetWidth(pixt);
    h = pixGetHeight(pixt);
    datab = pixGetData(pixb);
    datat = pixGetData(pixt);
    wplb = pixGetWpl(pixb);
    wplt = pixGetWpl(pixt);

    if ((buffer = (l_uint8 *)CALLOC(L_MAX(w, h), sizeof(l_uint8))) == NULL)
        return (PIX *)ERROR_PTR("buffer not made", procName, NULL);
    maxsize = L_MAX(hsize, vsize);
    if ((maxarray = (l_uint8 *)CALLOC(2 * maxsize, sizeof(l_uint8))) == NULL)
        return (PIX *)ERROR_PTR("buffer not made", procName, NULL);

    if (vsize == 1)
        dilateGrayLow(datat, w, h, wplt, datab, wplb, hsize, L_HORIZ,
                      buffer, maxarray);
    else if (hsize == 1)
        dilateGrayLow(datat, w, h, wplt, datab, wplb, vsize, L_VERT,
                      buffer, maxarray);
    else {
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
        L_WARNING("horiz sel size must be odd; increasing by 1", procName);
        hsize++;
    }
    if ((vsize & 1) == 0 ) {
        L_WARNING("vert sel size must be odd; increasing by 1", procName);
        vsize++;
    }

    if (hsize == 1 && vsize == 1)
        return pixCopy(NULL, pixs);

    if (vsize == 1) {  /* horizontal sel */
        leftpix = (hsize + 1) / 2;
        rightpix = (3 * hsize + 1) / 2;
        toppix = 0;
        bottompix = 0;
    }
    else if (hsize == 1) {  /* vertical sel */
        leftpix = 0;
        rightpix = 0;
        toppix = (vsize + 1) / 2;
        bottompix = (3 * vsize + 1) / 2;
    }
    else {
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
    
    w = pixGetWidth(pixt);
    h = pixGetHeight(pixt);
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
    }
    else {
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
        L_WARNING("horiz sel size must be odd; increasing by 1", procName);
        hsize++;
    }
    if ((vsize & 1) == 0 ) {
        L_WARNING("vert sel size must be odd; increasing by 1", procName);
        vsize++;
    }

    if (hsize == 1 && vsize == 1)
        return pixCopy(NULL, pixs);

    if (vsize == 1) {  /* horizontal sel */
        leftpix = (hsize + 1) / 2;
        rightpix = (3 * hsize + 1) / 2;
        toppix = 0;
        bottompix = 0;
    }
    else if (hsize == 1) {  /* vertical sel */
        leftpix = 0;
        rightpix = 0;
        toppix = (vsize + 1) / 2;
        bottompix = (3 * vsize + 1) / 2;
    }
    else {
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
    
    w = pixGetWidth(pixt);
    h = pixGetHeight(pixt);
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
    }
    else if (hsize == 1) {
        dilateGrayLow(datat, w, h, wplt, datab, wplb, vsize, L_VERT,
                     buffer, array);
        pixSetOrClearBorder(pixt, leftpix, rightpix, toppix, bottompix,
                            PIX_SET);
        erodeGrayLow(datab, w, h, wplb, datat, wplt, vsize, L_VERT,
                      buffer, array);
    }
    else {
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


