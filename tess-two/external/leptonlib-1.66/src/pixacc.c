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
 *   pixacc.c
 *
 *      Pixacc creation, destruction
 *           PIXACC   *pixaccCreate()
 *           PIXACC   *pixaccCreateWithPix()
 *           void      pixaccDestroy()
 *
 *      Pixacc finalization
 *           PIX      *pixaccFinal()
 *
 *      Pixacc accessors
 *           PIX      *pixaccGetPix()
 *           l_int32   pixaccGetOffset()
 *
 *      Pixacc accumulators
 *           l_int32   pixaccAdd()
 *           l_int32   pixaccSubtract()
 *           l_int32   pixaccMultConst()
 *           l_int32   pixaccMultConstAccumulate()
 *
 *  This is a simple interface for some of the pixel arithmetic operations 
 *  in pixarith.c.  These are easy to code up, but not as fast as
 *  hand-coded functions that do arithmetic on corresponding pixels.
 *
 *  Suppose you want to make a linear combination of pix1 and pix2:
 *     pixd = 0.4 * pix1 + 0.6 * pix2
 *  where pix1 and pix2 are the same size and have depth 'd'.  Then:
 *     Pixacc *pacc = pixaccCreateWithPix(pix1, 0);  // first; addition only
 *     pixaccMultConst(pacc, 0.4);
 *     pixaccMultConstAccumulate(pacc, pix2, 0.6);  // Add in 0.6 of the second
 *     pixd = pixaccFinal(pacc, d);  // Get the result
 *     pixaccDestroy(&pacc);
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


/*---------------------------------------------------------------------*
 *                     Pixacc creation, destruction                    *
 *---------------------------------------------------------------------*/
/*!
 *  pixaccCreate()
 *
 *      Input:  w, h (of 32 bpp internal Pix)
 *              negflag (0 if only positive numbers are involved;
 *                       1 if there will be negative numbers)
 *      Return: pixacc, or null on error
 *
 *  Notes:
 *      (1) Use @negflag = 1 for safety if any negative numbers are going
 *          to be used in the chain of operations.  Negative numbers
 *          arise, e.g., by subtracting a pix, or by adding a pix
 *          that has been pre-multiplied by a negative number.
 *      (2) Initializes the internal 32 bpp pix, similarly to the
 *          initialization in pixInitAccumulate().
 */
PIXACC *
pixaccCreate(l_int32  w,
             l_int32  h,
             l_int32  negflag)
{
PIXACC  *pixacc;

    PROCNAME("pixaccCreate");

    if ((pixacc = (PIXACC *)CALLOC(1, sizeof(PIXACC))) == NULL)
        return (PIXACC *)ERROR_PTR("pixacc not made", procName, NULL);
    pixacc->w = w;
    pixacc->h = h;
    
    if ((pixacc->pix = pixCreate(w, h, 32)) == NULL)
        return (PIXACC *)ERROR_PTR("pix not made", procName, NULL);

    if (negflag) {
        pixacc->offset = 0x40000000;
        pixSetAllArbitrary(pixacc->pix, pixacc->offset);
    }

    return pixacc;
}


/*!
 *  pixaccCreateWithPix()
 *
 *      Input:  pix
 *              negflag (0 if only positive numbers are involved;
 *                       1 if there will be negative numbers)
 *      Return: pixacc, or null on error
 *
 *  Notes:
 *      (1) See pixaccCreate()
 */
PIXACC *
pixaccCreateWithPix(PIX     *pix,
                    l_int32  negflag)
{
l_int32  w, h;
PIXACC  *pixacc;

    PROCNAME("pixaccCreateWithPix");

    if (!pix)
        return (PIXACC *)ERROR_PTR("pix not defined", procName, NULL);

    pixGetDimensions(pix, &w, &h, NULL);
    pixacc = pixaccCreate(w, h, negflag);
    pixaccAdd(pixacc, pix);
    return pixacc;
}


/*!
 *  pixaccDestroy()
 *
 *      Input:  &pixacc (<can be null>)
 *      Return: void
 *
 *  Notes:
 *      (1) Always nulls the input ptr.
 */
void
pixaccDestroy(PIXACC  **ppixacc)
{
PIXACC  *pixacc;

    PROCNAME("pixaccDestroy");

    if (ppixacc == NULL) {
        L_WARNING("ptr address is NULL!", procName);
        return;
    }

    if ((pixacc = *ppixacc) == NULL)
        return;

    pixDestroy(&pixacc->pix);
    FREE(pixacc);
    *ppixacc = NULL;
    return;
}


/*---------------------------------------------------------------------*
 *                            Pixacc finalization                      *
 *---------------------------------------------------------------------*/
/*!
 *  pixaccFinal()
 *
 *      Input:  pixacc
 *              outdepth (8, 16 or 32 bpp)
 *      Return: pixd (8 , 16 or 32 bpp), or null on error
 */
PIX *
pixaccFinal(PIXACC  *pixacc,
            l_int32  outdepth)
{
    PROCNAME("pixaccFinal");

    if (!pixacc)
        return (PIX *)ERROR_PTR("pixacc not defined", procName, NULL);

    return pixFinalAccumulate(pixaccGetPix(pixacc), pixaccGetOffset(pixacc),
                              outdepth);
}


/*---------------------------------------------------------------------*
 *                            Pixacc accessors                         *
 *---------------------------------------------------------------------*/
/*!
 *  pixaccGetPix()
 *
 *      Input:  pixacc
 *      Return: pix, or null on error
 */
PIX *
pixaccGetPix(PIXACC  *pixacc)
{
    PROCNAME("pixaccGetPix");

    if (!pixacc)
        return (PIX *)ERROR_PTR("pixacc not defined", procName, NULL);
    return pixacc->pix;
}


/*!
 *  pixaccGetOffset()
 *
 *      Input:  pixacc
 *      Return: offset, or -1 on error
 */
l_int32
pixaccGetOffset(PIXACC  *pixacc)
{
    PROCNAME("pixaccGetOffset");

    if (!pixacc)
        return ERROR_INT("pixacc not defined", procName, -1);
    return pixacc->offset;
}


/*---------------------------------------------------------------------*
 *                          Pixacc accumulators                        *
 *---------------------------------------------------------------------*/
/*!
 *  pixaccAdd()
 *
 *      Input:  pixacc
 *              pix (to be added)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixaccAdd(PIXACC  *pixacc,
          PIX     *pix)
{
    PROCNAME("pixaccAdd");

    if (!pixacc)
        return ERROR_INT("pixacc not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    pixAccumulate(pixaccGetPix(pixacc), pix, L_ARITH_ADD);
    return 0;
}


/*!
 *  pixaccSubtract()
 *
 *      Input:  pixacc
 *              pix (to be subtracted)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixaccSubtract(PIXACC  *pixacc,
               PIX     *pix)
{
    PROCNAME("pixaccSubtract");

    if (!pixacc)
        return ERROR_INT("pixacc not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    pixAccumulate(pixaccGetPix(pixacc), pix, L_ARITH_SUBTRACT);
    return 0;
}


/*!
 *  pixaccMultConst()
 *
 *      Input:  pixacc
 *              factor
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixaccMultConst(PIXACC    *pixacc,
                l_float32  factor)
{
    PROCNAME("pixaccMultConst");

    if (!pixacc)
        return ERROR_INT("pixacc not defined", procName, 1);
    pixMultConstAccumulate(pixaccGetPix(pixacc), factor,
                           pixaccGetOffset(pixacc));
    return 0;
}


/*!
 *  pixaccMultConstAccumulate()
 *
 *      Input:  pixacc
 *              pix
 *              factor
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This creates a temp pix that is @pix multiplied by the
 *          constant @factor.  It then adds that into @pixacc.
 */
l_int32
pixaccMultConstAccumulate(PIXACC    *pixacc,
                          PIX       *pix,
                          l_float32  factor)
{
l_int32  w, h, d, negflag;
PIX     *pixt;
PIXACC  *pacct;

    PROCNAME("pixaccMultConstAccumulate");

    if (!pixacc)
        return ERROR_INT("pixacc not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    if (factor == 0.0) return 0;

    pixGetDimensions(pix, &w, &h, &d);
    negflag = (factor > 0.0) ? 0 : 1;
    pacct = pixaccCreate(w, h, negflag);
    pixaccAdd(pacct, pix);
    pixaccMultConst(pacct, factor);
    pixt = pixaccFinal(pacct, d);
    pixaccAdd(pixacc, pixt);

    pixaccDestroy(&pacct);
    pixDestroy(&pixt);
    return 0;
}


