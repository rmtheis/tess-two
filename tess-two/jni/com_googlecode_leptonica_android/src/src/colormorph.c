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
 *  colormorph.c
 *
 *      Top-level color morphological operations
 *
 *            PIX     *pixColorMorph()
 *
 *      Method: Algorithm by van Herk and Gil and Werman, 1992
 *              Apply grayscale morphological operations separately
 *              to each component.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


/*-----------------------------------------------------------------*
 *              Top-level color morphological operations           *
 *-----------------------------------------------------------------*/
/*!
 *  pixColorMorph()
 *
 *      Input:  pixs
 *              type  (L_MORPH_DILATE, L_MORPH_ERODE, L_MORPH_OPEN,
 *                     or L_MORPH_CLOSE)
 *              hsize  (of Sel; must be odd; origin implicitly in center)
 *              vsize  (ditto)
 *      Return: pixd
 *
 *  Notes:
 *      (1) This does the morph operation on each component separately,
 *          and recombines the result.
 *      (2) Sel is a brick with all elements being hits.
 *      (3) If hsize = vsize = 1, just returns a copy.
 */
PIX *
pixColorMorph(PIX     *pixs,
              l_int32  type,
              l_int32  hsize,
              l_int32  vsize)
{
PIX  *pixr, *pixg, *pixb, *pixrm, *pixgm, *pixbm, *pixd;

    PROCNAME("pixColorMorph");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs not 32 bpp", procName, NULL);
    if (type != L_MORPH_DILATE && type != L_MORPH_ERODE &&
        type != L_MORPH_OPEN && type != L_MORPH_CLOSE)
        return (PIX *)ERROR_PTR("invalid morph type", procName, NULL);
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

    pixr = pixGetRGBComponent(pixs, COLOR_RED);
    pixg = pixGetRGBComponent(pixs, COLOR_GREEN);
    pixb = pixGetRGBComponent(pixs, COLOR_BLUE);
    if (type == L_MORPH_DILATE) {
        pixrm = pixDilateGray(pixr, hsize, vsize);
        pixgm = pixDilateGray(pixg, hsize, vsize);
        pixbm = pixDilateGray(pixb, hsize, vsize);
    } 
    else if (type == L_MORPH_ERODE) {
        pixrm = pixErodeGray(pixr, hsize, vsize);
        pixgm = pixErodeGray(pixg, hsize, vsize);
        pixbm = pixErodeGray(pixb, hsize, vsize);
    } 
    else if (type == L_MORPH_OPEN) {
        pixrm = pixOpenGray(pixr, hsize, vsize);
        pixgm = pixOpenGray(pixg, hsize, vsize);
        pixbm = pixOpenGray(pixb, hsize, vsize);
    } 
    else {   /* type == L_MORPH_CLOSE */
        pixrm = pixCloseGray(pixr, hsize, vsize);
        pixgm = pixCloseGray(pixg, hsize, vsize);
        pixbm = pixCloseGray(pixb, hsize, vsize);
    } 
    pixd = pixCreateRGBImage(pixrm, pixgm, pixbm);
    pixDestroy(&pixr);
    pixDestroy(&pixrm);
    pixDestroy(&pixg);
    pixDestroy(&pixgm);
    pixDestroy(&pixb);
    pixDestroy(&pixbm);

    return pixd;
}



