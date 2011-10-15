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
 *   pixabasic.c
 *
 *      Pixa creation, destruction, copying
 *           PIXA     *pixaCreate()
 *           PIXA     *pixaCreateFromPix()
 *           PIXA     *pixaCreateFromBoxa()
 *           PIXA     *pixaSplitPix()
 *           void      pixaDestroy()
 *           PIXA     *pixaCopy()
 *
 *      Pixa addition
 *           l_int32   pixaAddPix()
 *           l_int32   pixaExtendArray()
 *           l_int32   pixaExtendArrayToSize()
 *           l_int32   pixaAddBox()
 *
 *      Pixa accessors
 *           l_int32   pixaGetCount()
 *           l_int32   pixaChangeRefcount()
 *           PIX      *pixaGetPix()
 *           l_int32   pixaGetPixDimensions()
 *           PIX      *pixaGetBoxa()
 *           l_int32   pixaGetBoxaCount()
 *           BOX      *pixaGetBox()
 *           l_int32   pixaGetBoxGeometry()
 *           PIX     **pixaGetPixArray()
 *
 *      Pixa array modifiers
 *           l_int32   pixaReplacePix()
 *           l_int32   pixaInsertPix()
 *           l_int32   pixaRemovePix()
 *           l_int32   pixaInitFull()
 *           l_int32   pixaClear()
 *
 *      Pixa combination
 *           PIXA     *pixaJoin()
 *
 *      Pixaa creation, destruction
 *           PIXAA    *pixaaCreate()
 *           PIXAA    *pixaaCreateFromPixa()
 *           void      pixaaDestroy()
 *
 *      Pixaa addition
 *           l_int32   pixaaAddPixa()
 *           l_int32   pixaaExtendArray()
 *           l_int32   pixaaAddBox()
 *
 *      Pixaa accessors
 *           l_int32   pixaaGetCount()
 *           PIXA     *pixaaGetPixa()
 *           BOXA     *pixaaGetBoxa()
 *
 *      Pixa serialized I/O
 *           PIXA     *pixaRead()
 *           PIXA     *pixaReadStream()
 *           l_int32   pixaWrite()
 *           l_int32   pixaWriteStream()
 *
 *      Pixaa serialized I/O
 *           PIXAA    *pixaaRead()
 *           PIXAA    *pixaaReadStream()
 *           l_int32   pixaaWrite()
 *           l_int32   pixaaWriteStream()
 *
 *   
 *   Important note on reference counting:
 *     Reference counting for the Pixa is analogous to that for the Boxa.
 *     See pix.h for details.   pixaCopy() provides three possible modes
 *     of copy.  The basic rule is that however a Pixa is obtained
 *     (e.g., from pixaCreate*(), pixaCopy(), or a Pixaa accessor),
 *     it is necessary to call pixaDestroy() on it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

static const l_int32  INITIAL_PTR_ARRAYSIZE = 20;   /* n'import quoi */


/*---------------------------------------------------------------------*
 *                    Pixa creation, destruction, copy                 *
 *---------------------------------------------------------------------*/
/*!
 *  pixaCreate()
 *
 *      Input:  n  (initial number of ptrs)
 *      Return: pixa, or null on error
 */
PIXA *
pixaCreate(l_int32  n)
{
PIXA  *pixa;

    PROCNAME("pixaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((pixa = (PIXA *)CALLOC(1, sizeof(PIXA))) == NULL)
        return (PIXA *)ERROR_PTR("pixa not made", procName, NULL);
    pixa->n = 0;
    pixa->nalloc = n;
    pixa->refcount = 1;
    
    if ((pixa->pix = (PIX **)CALLOC(n, sizeof(PIX *))) == NULL)
        return (PIXA *)ERROR_PTR("pix ptrs not made", procName, NULL);
    if ((pixa->boxa = boxaCreate(n)) == NULL)
        return (PIXA *)ERROR_PTR("boxa not made", procName, NULL);

    return pixa;
}


/*!
 *  pixaCreateFromPix()
 *
 *      Input:  pixs  (with individual components on a lattice)
 *              n   (number of components)
 *              cellw   (width of each cell)
 *              cellh   (height of each cell)
 *      Return: pixa, or null on error
 *
 *  Note: for bpp = 1, we truncate each retrieved pix to
 *        the ON pixels, which we assume for now start at (0,0)
 */
PIXA *
pixaCreateFromPix(PIX     *pixs,
                  l_int32  n,
                  l_int32  cellw,
                  l_int32  cellh)
{
l_int32  w, h, d, nw, nh, i, j, index;
PIX     *pix, *pixt;
PIXA    *pixa;

    PROCNAME("pixaCreateFromPix");

    if (!pixs)
        return (PIXA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (n <= 0)
        return (PIXA *)ERROR_PTR("n must be > 0", procName, NULL);

    if ((pixa = pixaCreate(n)) == NULL)
        return (PIXA *)ERROR_PTR("pixa not made", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if ((pixt = pixCreate(cellw, cellh, d)) == NULL)
        return (PIXA *)ERROR_PTR("pixt not made", procName, NULL);

    nw = (w + cellw - 1) / cellw;
    nh = (h + cellh - 1) / cellh;
    for (i = 0, index = 0; i < nh; i++) {
        for (j = 0; j < nw && index < n; j++, index++) {
            pixRasterop(pixt, 0, 0, cellw, cellh, PIX_SRC, pixs,
                   j * cellw, i * cellh);
            if (d == 1 && !pixClipToForeground(pixt, &pix, NULL))
                pixaAddPix(pixa, pix, L_INSERT);
            else
                pixaAddPix(pixa, pixt, L_COPY);
        }
    }

    pixDestroy(&pixt);
    return pixa;
}


/*!
 *  pixaCreateFromBoxa()
 *
 *      Input:  pixs
 *              boxa
 *              &cropwarn (<optional return> TRUE if the boxa extent
 *                         is larger than pixs.
 *      Return: pixad, or null on error
 *
 *  Notes:
 *      (1) This simply extracts from pixs the region corresponding to each
 *          box in the boxa.
 *      (2) The 3rd arg is optional.  If the extent of the boxa exceeds the
 *          size of the pixa, so that some boxes are either clipped
 *          or entirely outside the pix, a warning is returned as TRUE.
 *      (3) pixad will have only the properly clipped elements, and
 *          the internal boxa will be correct.
 */
PIXA *
pixaCreateFromBoxa(PIX      *pixs,
                   BOXA     *boxa,
                   l_int32  *pcropwarn)
{
l_int32  i, n, w, h, wbox, hbox, cropwarn;
BOX     *box, *boxc;
PIX     *pixd;
PIXA    *pixad;

    PROCNAME("pixaCreateFromBoxa");

    if (!pixs)
        return (PIXA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!boxa)
        return (PIXA *)ERROR_PTR("boxa not defined", procName, NULL);

    n = boxaGetCount(boxa);
    if ((pixad = pixaCreate(n)) == NULL)
        return (PIXA *)ERROR_PTR("pixad not made", procName, NULL);

    boxaGetExtent(boxa, &wbox, &hbox, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);
    cropwarn = FALSE;
    if (wbox > w || hbox > h)
        cropwarn = TRUE;
    if (pcropwarn)
        *pcropwarn = cropwarn;

    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_COPY);
        if (cropwarn) {  /* if box is outside pixs, pixd is NULL */
            pixd = pixClipRectangle(pixs, box, &boxc);  /* may be NULL */
            if (pixd) {
                pixaAddPix(pixad, pixd, L_INSERT);
                pixaAddBox(pixad, boxc, L_INSERT);
            }
            boxDestroy(&box);
        }
        else {
            pixd = pixClipRectangle(pixs, box, NULL);
            pixaAddPix(pixad, pixd, L_INSERT);
            pixaAddBox(pixad, box, L_INSERT);
        }
    }

    return pixad;
}
    

/*!
 *  pixaSplitPix()
 *
 *      Input:  pixs  (with individual components on a lattice)
 *              nx   (number of mosaic cells horizontally)
 *              ny   (number of mosaic cells vertically)
 *              borderwidth  (of added border on all sides)
 *              bordercolor  (in our RGBA format: 0xrrggbbaa)
 *      Return: pixa, or null on error
 *
 *  Notes:
 *      (1) This is a variant on pixaCreateFromPix(), where we
 *          simply divide the image up into (approximately) equal
 *          subunits.  If you want the subimages to have essentially
 *          the same aspect ratio as the input pix, use nx = ny.
 *      (2) If borderwidth is 0, we ignore the input bordercolor and
 *          redefine it to white.
 *      (3) The bordercolor is always used to initialize each tiled pix,
 *          so that if the src is clipped, the unblitted part will
 *          be this color.  This avoids 1 pixel wide black stripes at the
 *          left and lower edges.
 */
PIXA *
pixaSplitPix(PIX      *pixs,
             l_int32   nx,
             l_int32   ny,
             l_int32   borderwidth,
             l_uint32  bordercolor)
{
l_int32  w, h, d, cellw, cellh, i, j;
PIX     *pixt;
PIXA    *pixa;

    PROCNAME("pixaSplitPix");

    if (!pixs)
        return (PIXA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (nx <= 0 || ny <= 0)
        return (PIXA *)ERROR_PTR("nx and ny must be > 0", procName, NULL);
    borderwidth = L_MAX(0, borderwidth);

    if ((pixa = pixaCreate(nx * ny)) == NULL)
        return (PIXA *)ERROR_PTR("pixa not made", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    cellw = (w + nx - 1) / nx;  /* round up */
    cellh = (h + ny - 1) / ny;

    for (i = 0; i < ny; i++) {
        for (j = 0; j < nx; j++) {
            if ((pixt = pixCreate(cellw + 2 * borderwidth,
                                  cellh + 2 * borderwidth, d)) == NULL)
                return (PIXA *)ERROR_PTR("pixt not made", procName, NULL);
            pixCopyColormap(pixt, pixs);
            if (borderwidth == 0) {  /* initialize full image to white */
                if (d == 1)
                    pixClearAll(pixt);
                else
                    pixSetAll(pixt);
            }
            else
                pixSetAllArbitrary(pixt, bordercolor);
            pixRasterop(pixt, borderwidth, borderwidth, cellw, cellh,
                        PIX_SRC, pixs, j * cellw, i * cellh);
            pixaAddPix(pixa, pixt, L_INSERT);
        }
    }

    return pixa;
}


/*!
 *  pixaDestroy()
 *
 *      Input:  &pixa (<can be nulled>)
 *      Return: void
 *
 *  Notes:
 *      (1) Decrements the ref count and, if 0, destroys the pixa.
 *      (2) Always nulls the input ptr.
 */
void
pixaDestroy(PIXA  **ppixa)
{
l_int32  i;
PIXA    *pixa;

    PROCNAME("pixaDestroy");

    if (ppixa == NULL) {
        L_WARNING("ptr address is NULL!", procName);
        return;
    }

    if ((pixa = *ppixa) == NULL)
        return;

        /* Decrement the refcount.  If it is 0, destroy the pixa. */
    pixaChangeRefcount(pixa, -1);
    if (pixa->refcount <= 0) {
        for (i = 0; i < pixa->n; i++)
            pixDestroy(&pixa->pix[i]);
        FREE(pixa->pix);
        boxaDestroy(&pixa->boxa);
        FREE(pixa);
    }

    *ppixa = NULL;
    return;
}


/*!
 *  pixaCopy()
 *
 *      Input:  pixas
 *              copyflag:
 *                L_COPY makes a new pixa and copies each pix and each box
 *                L_CLONE gives a new ref-counted handle to the input pixa
 *                L_COPY_CLONE makes a new pixa and inserts clones of
 *                    all pix and boxes
 *      Return: new pixa, or null on error
 *
 *  Note: see pix.h for description of the copy types.
 */
PIXA *
pixaCopy(PIXA    *pixa,
         l_int32  copyflag)
{
l_int32  i;
BOX     *boxc;
PIX     *pixc;
PIXA    *pixac;

    PROCNAME("pixaCopy");

    if (!pixa)
        return (PIXA *)ERROR_PTR("pixa not defined", procName, NULL);

    if (copyflag == L_CLONE) {
        pixaChangeRefcount(pixa, 1);
        return pixa;
    }

    if (copyflag != L_COPY && copyflag != L_COPY_CLONE)
        return (PIXA *)ERROR_PTR("invalid copyflag", procName, NULL);

    if ((pixac = pixaCreate(pixa->n)) == NULL)
        return (PIXA *)ERROR_PTR("pixac not made", procName, NULL);
    for (i = 0; i < pixa->n; i++) {
        if (copyflag == L_COPY) {
            pixc = pixaGetPix(pixa, i, L_COPY);
            boxc = pixaGetBox(pixa, i, L_COPY);
        }
        else {  /* copy-clone */
            pixc = pixaGetPix(pixa, i, L_CLONE);
            boxc = pixaGetBox(pixa, i, L_CLONE);
        }
        pixaAddPix(pixac, pixc, L_INSERT);
        pixaAddBox(pixac, boxc, L_INSERT);
    }

    return pixac;
}



/*---------------------------------------------------------------------*
 *                              Pixa addition                          *
 *---------------------------------------------------------------------*/
/*!
 *  pixaAddPix()
 *
 *      Input:  pixa
 *              pix  (to be added)
 *              copyflag (L_INSERT, L_COPY, L_CLONE)
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixaAddPix(PIXA    *pixa,
           PIX     *pix,
           l_int32  copyflag)
{
l_int32  n;
PIX     *pixc;

    PROCNAME("pixaAddPix");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    if (copyflag == L_INSERT)
        pixc = pix;
    else if (copyflag == L_COPY)
        pixc = pixCopy(NULL, pix);
    else if (copyflag == L_CLONE)
        pixc = pixClone(pix);
    else
        return ERROR_INT("invalid copyflag", procName, 1);
    if (!pixc)
        return ERROR_INT("pixc not made", procName, 1);

    n = pixaGetCount(pixa);
    if (n >= pixa->nalloc)
        pixaExtendArray(pixa);
    pixa->pix[n] = pixc;
    pixa->n++;

    return 0;
}


/*!
 *  pixaExtendArray()
 *
 *      Input:  pixa
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Doubles the size of the pixa and boxa ptr arrays.
 */
l_int32
pixaExtendArray(PIXA  *pixa)
{
    PROCNAME("pixaExtendArray");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    return pixaExtendArrayToSize(pixa, 2 * pixa->nalloc);
}


/*!
 *  pixaExtendArrayToSize()
 *
 *      Input:  pixa
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) If necessary, reallocs new pixa and boxa ptrs arrays to @size.
 *          The pixa and boxa ptr arrays must always be equal in size.
 */
l_int32
pixaExtendArrayToSize(PIXA    *pixa,
                      l_int32  size)
{
    PROCNAME("pixaExtendArrayToSize");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    if (size > pixa->nalloc) {
        if ((pixa->pix = (PIX **)reallocNew((void **)&pixa->pix,
                                 sizeof(PIX *) * pixa->nalloc,
                                 size * sizeof(PIX *))) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);
        pixa->nalloc = size;
    }
    return boxaExtendArrayToSize(pixa->boxa, size);
}


/*!
 *  pixaAddBox()
 *
 *      Input:  pixa
 *              box
 *              copyflag (L_INSERT, L_COPY, L_CLONE)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixaAddBox(PIXA    *pixa,
           BOX     *box,
           l_int32  copyflag)
{
    PROCNAME("pixaAddBox");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if (copyflag != L_INSERT && copyflag != L_COPY && copyflag != L_CLONE)
        return ERROR_INT("invalid copyflag", procName, 1);

    boxaAddBox(pixa->boxa, box, copyflag);
    return 0;
}



/*---------------------------------------------------------------------*
 *                             Pixa accessors                          *
 *---------------------------------------------------------------------*/
/*!
 *  pixaGetCount()
 *
 *      Input:  pixa
 *      Return: count, or 0 if no pixa
 */
l_int32
pixaGetCount(PIXA  *pixa)
{
    PROCNAME("pixaGetCount");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 0);

    return pixa->n;
}


/*!
 *  pixaChangeRefcount()
 *
 *      Input:  pixa
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixaChangeRefcount(PIXA    *pixa,
                   l_int32  delta)
{
    PROCNAME("pixaChangeRefcount");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    pixa->refcount += delta;
    return 0;
}


/*!
 *  pixaGetPix()
 *
 *      Input:  pixa
 *              index  (to the index-th pix)
 *              accesstype  (L_COPY or L_CLONE)
 *      Return: pix, or null on error
 */
PIX *
pixaGetPix(PIXA    *pixa,
           l_int32  index,
           l_int32  accesstype)
{
    PROCNAME("pixaGetPix");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);
    if (index < 0 || index >= pixa->n)
        return (PIX *)ERROR_PTR("index not valid", procName, NULL);

    if (accesstype == L_COPY)
        return pixCopy(NULL, pixa->pix[index]);
    else if (accesstype == L_CLONE)
        return pixClone(pixa->pix[index]);
    else
        return (PIX *)ERROR_PTR("invalid accesstype", procName, NULL);
}


/*!
 *  pixaGetPixDimensions()
 *
 *      Input:  pixa
 *              index  (to the index-th box)
 *              &w, &h, &d (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixaGetPixDimensions(PIXA     *pixa,
                     l_int32   index,
                     l_int32  *pw,
                     l_int32  *ph,
                     l_int32  *pd)
{
PIX  *pix;

    PROCNAME("pixaGetPixDimensions");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    if (index < 0 || index >= pixa->n)
        return ERROR_INT("index not valid", procName, 1);

    if ((pix = pixaGetPix(pixa, index, L_CLONE)) == NULL)
        return ERROR_INT("pix not found!", procName, 1);
    pixGetDimensions(pix, pw, ph, pd);
    pixDestroy(&pix);
    return 0;
}


/*!
 *  pixaGetBoxa()
 *
 *      Input:  pixa
 *              accesstype  (L_COPY, L_CLONE, L_COPY_CLONE)
 *      Return: boxa, or null on error
 */
BOXA *
pixaGetBoxa(PIXA    *pixa,
            l_int32  accesstype)
{
    PROCNAME("pixaGetBoxa");

    if (!pixa)
        return (BOXA *)ERROR_PTR("pixa not defined", procName, NULL);
    if (!pixa->boxa)
        return (BOXA *)ERROR_PTR("boxa not defined", procName, NULL);
    if (accesstype != L_COPY && accesstype != L_CLONE &&
        accesstype != L_COPY_CLONE)
        return (BOXA *)ERROR_PTR("invalid accesstype", procName, NULL);

    return boxaCopy(pixa->boxa, accesstype);
}


/*!
 *  pixaGetBoxaCount()
 *
 *      Input:  pixa
 *      Return: count, or 0 on error
 */
l_int32
pixaGetBoxaCount(PIXA  *pixa)
{
    PROCNAME("pixaGetBoxaCount");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 0);
    
    return boxaGetCount(pixa->boxa);
}


/*!
 *  pixaGetBox()
 *
 *      Input:  pixa
 *              index  (to the index-th pix)
 *              accesstype  (L_COPY or L_CLONE)
 *      Return: box (if null, not automatically an error), or null on error
 *
 *  Notes:
 *      (1) There is always a boxa with a pixa, and it is initialized so
 *          that each box ptr is NULL.
 *      (2) In general, we expect that there is either a box associated
 *          with each pix, or no boxes at all in the boxa.
 *      (3) Having no boxes is thus not an automatic error.  Whether it
 *          is an actual error is determined by the calling program.
 *          If the caller expects to get a box, it is an error; see, e.g.,
 *          pixaGetBoxGeometry().
 */
BOX *
pixaGetBox(PIXA    *pixa,
           l_int32  index,
           l_int32  accesstype)
{
BOX  *box;

    PROCNAME("pixaGetBox");

    if (!pixa)
        return (BOX *)ERROR_PTR("pixa not defined", procName, NULL);
    if (!pixa->boxa)
        return (BOX *)ERROR_PTR("boxa not defined", procName, NULL);
    if (index < 0 || index >= pixa->boxa->n)
        return (BOX *)ERROR_PTR("index not valid", procName, NULL);
    if (accesstype != L_COPY && accesstype != L_CLONE)
        return (BOX *)ERROR_PTR("invalid accesstype", procName, NULL);

    box = pixa->boxa->box[index];
    if (box) {
        if (accesstype == L_COPY)
            return boxCopy(box);
        else  /* accesstype == L_CLONE */
            return boxClone(box);
    }
    else
        return NULL;
}


/*!
 *  pixaGetBoxGeometry()
 *
 *      Input:  pixa
 *              index  (to the index-th box)
 *              &x, &y, &w, &h (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixaGetBoxGeometry(PIXA     *pixa,
                   l_int32   index,
                   l_int32  *px,
                   l_int32  *py,
                   l_int32  *pw,
                   l_int32  *ph)
{
BOX  *box;

    PROCNAME("pixaGetBoxGeometry");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    if (index < 0 || index >= pixa->n)
        return ERROR_INT("index not valid", procName, 1);

    if ((box = pixaGetBox(pixa, index, L_CLONE)) == NULL)
        return ERROR_INT("box not found!", procName, 1);
    boxGetGeometry(box, px, py, pw, ph);
    boxDestroy(&box);
    return 0;
}


/*!
 *  pixaGetPixArray()
 *
 *      Input:  pixa
 *      Return: pix array, or null on error
 *
 *  Notes:
 *      (1) This returns a ptr to the actual array.  The array is
 *          owned by the pixa, so it must not be destroyed.
 *      (2) The caller should always check if the return value is NULL
 *          before accessing any of the pix ptrs in this array!
 */
PIX **
pixaGetPixArray(PIXA  *pixa)
{
    PROCNAME("pixaGetPixArray");

    if (!pixa)
        return (PIX **)ERROR_PTR("pixa not defined", procName, NULL);

    return pixa->pix;
}



/*---------------------------------------------------------------------*
 *                       Pixa array modifiers                          *
 *---------------------------------------------------------------------*/
/*!
 *  pixaReplacePix()
 *
 *      Input:  pixa
 *              index  (to the index-th pix)
 *              pix (insert to replace existing one)
 *              box (<optional> insert to replace existing)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) In-place replacement of one pix.
 *      (2) The previous pix at that location is destroyed.
 */
l_int32
pixaReplacePix(PIXA    *pixa,
               l_int32  index,
               PIX     *pix,
               BOX     *box)
{
BOXA  *boxa;

    PROCNAME("pixaReplacePix");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    if (index < 0 || index >= pixa->n)
        return ERROR_INT("index not valid", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixDestroy(&(pixa->pix[index]));
    pixa->pix[index] = pix;

    if (box) {
        boxa = pixa->boxa;
        if (index > boxa->n)
            return ERROR_INT("boxa index not valid", procName, 1);
        boxaReplaceBox(boxa, index, box);
    }

    return 0;
}


/*!
 *  pixaInsertPix()
 *
 *      Input:  pixa
 *              index (at which pix is to be inserted)
 *              pixs (new pix to be inserted)
 *              box (<optional> new box to be inserted)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This shifts pixa[i] --> pixa[i + 1] for all i >= index,
 *          and then inserts at pixa[index].
 *      (2) To insert at the beginning of the array, set index = 0.
 *      (3) It should not be used repeatedly on large arrays,
 *          because the function is O(n).
 *      (4) To append a pix to a pixa, it's easier to use pixaAddPix().
 */
l_int32
pixaInsertPix(PIXA    *pixa,
              l_int32  index,
              PIX     *pixs,
              BOX     *box)
{
l_int32  i, n;

    PROCNAME("pixaInsertPix");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    n = pixaGetCount(pixa);
    if (index < 0 || index > n)
        return ERROR_INT("index not in {0...n}", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

    if (n >= pixa->nalloc) {  /* extend both ptr arrays */
        pixaExtendArray(pixa);
        boxaExtendArray(pixa->boxa);
    }
    pixa->n++;
    for (i = n; i > index; i--)
      pixa->pix[i] = pixa->pix[i - 1];
    pixa->pix[index] = pixs;

        /* Optionally, insert the box */
    if (box)
        boxaInsertBox(pixa->boxa, index, box);

    return 0;
}


/*!
 *  pixaRemovePix()
 *
 *      Input:  pixa
 *              index (of pix to be removed)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This shifts pixa[i] --> pixa[i - 1] for all i > index.
 *      (2) It should not be used repeatedly on large arrays,
 *          because the function is O(n).
 *      (3) The corresponding box is removed as well, if it exists.
 */
l_int32
pixaRemovePix(PIXA    *pixa,
              l_int32  index)
{
l_int32  i, n, nbox;
BOXA    *boxa;
PIX    **array;

    PROCNAME("pixaRemovePix");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    n = pixaGetCount(pixa);
    if (index < 0 || index >= n)
        return ERROR_INT("index not in {0...n - 1}", procName, 1);

        /* Remove the pix */
    array = pixa->pix;
    pixDestroy(&array[index]);
    for (i = index + 1; i < n; i++)
        array[i - 1] = array[i];
    array[n - 1] = NULL;
    pixa->n--;

        /* Remove the box if it exists  */
    boxa = pixa->boxa;
    nbox = boxaGetCount(boxa);
    if (index < nbox)
        boxaRemoveBox(boxa, index);

    return 0;
}


/*!
 *  pixaInitFull()
 *
 *      Input:  pixa (typically empty)
 *              pix (to be replicated into the entire pixa ptr array)
 *              box (<optional> to be replicated into the entire boxa ptr array)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This initializes a pixa by filling up the entire pix ptr array
 *          with copies of @pix.  Any existing pix are destroyed.
 *          It also fills the boxa with copies of @box.
 *          After this oepration, the numbers of pix and boxes are equal to
 *          the number of allocated ptrs.
 *      (2) Note that we use pixaReplacePix() instead of pixaInsertPix().
 *          They both have the same effect when inserting into a NULL ptr
 *          in the pixa ptr array:
 *      (3) Example usage.  This function is useful to prepare for a
 *          random insertion (or replacement) of pix into a pixa.
 *          To randomly insert pix into a pixa, up to some index "max":
 *             Pixa *pixa = pixaCreate(max);
 *             Pix *pix = pixCreate(1, 1, 1);  // little memory
 *             Box *box = boxCreate(...);
 *             pixaInitFull(pixa, pix, box);
 *          An existing pixa with a smaller ptr array can also be reused:
 *             pixaExtendArrayToSize(pixa, max);
 *             Pix *pix = pixCreate(...);
 *             Box *box = boxCreate(...);
 *             pixaInitFull(pixa, pix, box);
 *          For these situations, the pix should be small and disposable.
 *          The initialization allows the pixa to always be properly
 *          filled, even if all pix (and boxes) are not later replaced.
 */
l_int32
pixaInitFull(PIXA  *pixa,
             PIX   *pix,
             BOX   *box)
{
l_int32  i, n;
PIX     *pixt;

    PROCNAME("pixaInitFull");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    n = pixa->nalloc;
    pixa->n = n;
    for (i = 0; i < n; i++) {
        pixt = pixCopy(NULL, pix);
        pixaReplacePix(pixa, i, pixt, NULL);
    }
    if (box)
        boxaInitFull(pixa->boxa, box);

    return 0;
}


/*!
 *  pixaClear()
 *
 *      Input:  pixa
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This destroys all pix in the pixa, as well as
 *          all boxes in the boxa.  The ptrs in the pix ptr array
 *          are all null'd.  The number of allocated pix, n, is set to 0.
 */
l_int32
pixaClear(PIXA  *pixa)
{
l_int32  i, n;

    PROCNAME("pixaClear");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    n = pixaGetCount(pixa);
    for (i = 0; i < n; i++)
        pixDestroy(&pixa->pix[i]);
    pixa->n = 0;
    boxaClear(pixa->boxa);
    return 0;
}


/*---------------------------------------------------------------------*
 *                           Pixa combination                          *
 *---------------------------------------------------------------------*/
/*!
 *  pixaJoin()
 *
 *      Input:  pixad  (dest pixa; add to this one)
 *              pixas  (source pixa; add from this one)
 *              istart  (starting index in nas)
 *              iend  (ending index in nas; use 0 to cat all)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This appends a clone of each indicated pix in pixas to pixad
 *      (2) istart < 0 is taken to mean 'read from the start' (istart = 0)
 *      (3) iend <= 0 means 'read to the end'
 */
l_int32
pixaJoin(PIXA    *pixad,
         PIXA    *pixas,
         l_int32  istart,
         l_int32  iend)
{
l_int32  ns, i;
BOXA    *boxas, *boxad;
PIX     *pix;

    PROCNAME("pixaJoin");

    if (!pixad)
        return ERROR_INT("pixad not defined", procName, 1);
    if (!pixas)
        return ERROR_INT("pixas not defined", procName, 1);
    if ((ns = pixaGetCount(pixas)) == 0) {
        L_INFO("empty pixas", procName);
        return 0;
    }
    if (istart < 0)
        istart = 0;
    if (istart >= ns)
        return ERROR_INT("istart out of bounds", procName, 1);
    if (iend <= 0)
        iend = ns - 1;
    if (iend >= ns)
        return ERROR_INT("iend out of bounds", procName, 1);
    if (istart > iend)
        return ERROR_INT("istart > iend; nothing to add", procName, 1);

    for (i = istart; i <= iend; i++) {
        pix = pixaGetPix(pixas, i, L_CLONE);
        pixaAddPix(pixad, pix, L_INSERT);
    }

    boxas = pixaGetBoxa(pixas, L_CLONE);
    boxad = pixaGetBoxa(pixad, L_CLONE);
    boxaJoin(boxad, boxas, 0, 0);
    boxaDestroy(&boxas);  /* just the clones */
    boxaDestroy(&boxad);

    return 0;
}


/*---------------------------------------------------------------------*
 *                    Pixaa creation and destruction                   *
 *---------------------------------------------------------------------*/
/*!
 *  pixaaCreate()
 *
 *      Input:  n  (initial number of pixa ptrs)
 *      Return: pixaa, or null on error
 *
 *  Notes:
 *      (1) A pixaa provides a 2-level hierarchy of images.
 *          A common use is for segmentation masks, which are
 *          inexpensive to store in png format.
 *      (2) For example, suppose you want a mask for each textline
 *          in a two-column page.  The textline masks for each column
 *          can be represented by a pixa, of which there are 2 in the pixaa.
 *          The boxes for the textline mask components within a column
 *          can have their origin referred to the column rather than the page.
 *          Then the boxa field can be used to represent the two box (regions)
 *          for the columns, and the (x,y) components of each box can
 *          be used to get the absolute position of the textlines on
 *          the page.
 */
PIXAA *
pixaaCreate(l_int32  n)
{
PIXAA  *pixaa;

    PROCNAME("pixaaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((pixaa = (PIXAA *)CALLOC(1, sizeof(PIXAA))) == NULL)
        return (PIXAA *)ERROR_PTR("pixaa not made", procName, NULL);
    pixaa->n = 0;
    pixaa->nalloc = n;
    
    if ((pixaa->pixa = (PIXA **)CALLOC(n, sizeof(PIXA *))) == NULL)
        return (PIXAA *)ERROR_PTR("pixa ptrs not made", procName, NULL);
    pixaa->boxa = boxaCreate(n);

    return pixaa;
}


/*!
 *  pixaaCreateFromPixa()
 *
 *      Input:  pixa
 *              n (number specifying subdivision of pixa)
 *              type (L_CHOOSE_CONSECUTIVE, L_CHOOSE_SKIP_BY)
 *              copyflag (L_CLONE, L_COPY)
 *      Return: pixaa, or null on error
 *
 *  Notes:
 *      (1) This subdivides a pixa into a set of smaller pixa that
 *          are accumulated into a pixaa.
 *      (2) If type == L_CHOOSE_CONSECUTIVE, the first 'n' pix are
 *          put in a pixa and added to pixaa, then the next 'n', etc.
 *          If type == L_CHOOSE_SKIP_BY, the first pixa is made by
 *          aggregating pix[0], pix[n], pix[2*n], etc.
 *      (3) The copyflag specifies if each new pix is a copy or a clone.
 */
PIXAA *
pixaaCreateFromPixa(PIXA    *pixa,
                    l_int32  n,
                    l_int32  type,
                    l_int32  copyflag)
{
l_int32  count, i, j, npixa;
PIX     *pix;
PIXA    *pixat;
PIXAA   *pixaa;

    PROCNAME("pixaaCreateFromPixa");

    if (!pixa)
        return (PIXAA *)ERROR_PTR("pixa not defined", procName, NULL);
    count = pixaGetCount(pixa);
    if (count == 0)
        return (PIXAA *)ERROR_PTR("no pix in pixa", procName, NULL);
    if (n <= 0)
        return (PIXAA *)ERROR_PTR("n must be > 0", procName, NULL);
    if (type != L_CHOOSE_CONSECUTIVE && type != L_CHOOSE_SKIP_BY)
        return (PIXAA *)ERROR_PTR("invalid type", procName, NULL);
    if (copyflag != L_CLONE && copyflag != L_COPY)
        return (PIXAA *)ERROR_PTR("invalid copyflag", procName, NULL);

    if (type == L_CHOOSE_CONSECUTIVE)
        npixa = (count + n - 1) / n;
    else  /* L_CHOOSE_SKIP_BY */
        npixa = L_MIN(n, count);
    pixaa = pixaaCreate(npixa);
    if (type == L_CHOOSE_CONSECUTIVE) {
        for (i = 0; i < count; i++) {
            if (i % n == 0)
                pixat = pixaCreate(n);
            pix = pixaGetPix(pixa, i, copyflag);
            pixaAddPix(pixat, pix, L_INSERT);
            if (i % n == n - 1)
                pixaaAddPixa(pixaa, pixat, L_INSERT);
        }
        if (i % n != 0)
            pixaaAddPixa(pixaa, pixat, L_INSERT);
    }
    else {  /* L_CHOOSE_SKIP_BY */
        for (i = 0; i < npixa; i++) {
            pixat = pixaCreate(count / npixa + 1);
            for (j = i; j < count; j += n) {
                pix = pixaGetPix(pixa, j, copyflag);
                pixaAddPix(pixat, pix, L_INSERT);
            }
            pixaaAddPixa(pixaa, pixat, L_INSERT);
        }
    }

    return pixaa;
}


/*!
 *  pixaaDestroy()
 *
 *      Input:  &pixaa <to be nulled>
 *      Return: void
 */
void
pixaaDestroy(PIXAA  **ppixaa)
{
l_int32  i;
PIXAA   *pixaa;

    PROCNAME("pixaaDestroy");

    if (ppixaa == NULL) {
        L_WARNING("ptr address is NULL!", procName);
        return;
    }

    if ((pixaa = *ppixaa) == NULL)
        return;

    for (i = 0; i < pixaa->n; i++)
        pixaDestroy(&pixaa->pixa[i]);
    FREE(pixaa->pixa);
    boxaDestroy(&pixaa->boxa);

    FREE(pixaa);
    *ppixaa = NULL;

    return;
}


/*---------------------------------------------------------------------*
 *                             Pixaa addition                          *
 *---------------------------------------------------------------------*/
/*!
 *  pixaaAddPixa()
 *
 *      Input:  pixaa
 *              pixa  (to be added)
 *              copyflag:
 *                L_INSERT inserts the pixa directly
 *                L_COPY makes a new pixa and copies each pix and each box
 *                L_CLONE gives a new handle to the input pixa
 *                L_COPY_CLONE makes a new pixa and inserts clones of
 *                    all pix and boxes
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixaaAddPixa(PIXAA   *pixaa,
             PIXA    *pixa,
             l_int32  copyflag)
{
l_int32  n;
PIXA    *pixac;

    PROCNAME("pixaaAddPixa");

    if (!pixaa)
        return ERROR_INT("pixaa not defined", procName, 1);
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    if (copyflag != L_INSERT && copyflag != L_COPY &&
        copyflag != L_CLONE && copyflag != L_COPY_CLONE)
        return ERROR_INT("invalid copyflag", procName, 1);

    if (copyflag == L_INSERT)
        pixac = pixa;
    else {
        if ((pixac = pixaCopy(pixa, copyflag)) == NULL)
            return ERROR_INT("pixac not made", procName, 1);
    }

    n = pixaaGetCount(pixaa);
    if (n >= pixaa->nalloc)
        pixaaExtendArray(pixaa);
    pixaa->pixa[n] = pixac;
    pixaa->n++;

    return 0;
}


/*!
 *  pixaaExtendArray()
 *
 *      Input:  pixaa
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixaaExtendArray(PIXAA  *pixaa)
{
    PROCNAME("pixaaExtendArray");

    if (!pixaa)
        return ERROR_INT("pixaa not defined", procName, 1);

    if ((pixaa->pixa = (PIXA **)reallocNew((void **)&pixaa->pixa,
                             sizeof(PIXA *) * pixaa->nalloc,
                             2 * sizeof(PIXA *) * pixaa->nalloc)) == NULL)
        return ERROR_INT("new ptr array not returned", procName, 1);

    pixaa->nalloc = 2 * pixaa->nalloc;
    return 0;
}


/*!
 *  pixaaAddBox()
 *
 *      Input:  pixaa
 *              box
 *              copyflag (L_INSERT, L_COPY, L_CLONE)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The box can be used, for example, to hold the support region
 *          of a pixa that is being added to the pixaa.
 */
l_int32
pixaaAddBox(PIXAA   *pixaa,
            BOX     *box,
            l_int32  copyflag)
{
    PROCNAME("pixaaAddBox");

    if (!pixaa)
        return ERROR_INT("pixaa not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if (copyflag != L_INSERT && copyflag != L_COPY && copyflag != L_CLONE)
        return ERROR_INT("invalid copyflag", procName, 1);

    boxaAddBox(pixaa->boxa, box, copyflag);
    return 0;
}



/*---------------------------------------------------------------------*
 *                            Pixaa accessors                          *
 *---------------------------------------------------------------------*/
/*!
 *  pixaaGetCount()
 *
 *      Input:  pixaa
 *      Return: count, or 0 if no pixaa
 */
l_int32
pixaaGetCount(PIXAA  *pixaa)
{
    PROCNAME("pixaaGetCount");

    if (!pixaa)
        return ERROR_INT("pixaa not defined", procName, 0);

    return pixaa->n;
}


/*!
 *  pixaaGetPixa()
 *
 *      Input:  pixaa
 *              index  (to the index-th pixa)
 *              accesstype  (L_COPY, L_CLONE, L_COPY_CLONE)
 *      Return: pixa, or null on error
 *
 *  Notes:
 *      (1) L_COPY makes a new pixa with a copy of every pix
 *      (2) L_CLONE just makes a new reference to the pixa,
 *          and bumps the counter.  You would use this, for example,
 *          when you need to extract some data from a pix within a
 *          pixa within a pixaa.
 *      (3) L_COPY_CLONE makes a new pixa with a clone of every pix
 *          and box
 *      (4) In all cases, you must invoke pixaDestroy() on the returned pixa
 */
PIXA *
pixaaGetPixa(PIXAA   *pixaa,
             l_int32  index,
             l_int32  accesstype)
{
PIXA  *pixa;

    PROCNAME("pixaaGetPixa");

    if (!pixaa)
        return (PIXA *)ERROR_PTR("pixaa not defined", procName, NULL);
    if (index < 0 || index >= pixaa->n)
        return (PIXA *)ERROR_PTR("index not valid", procName, NULL);
    if (accesstype != L_COPY && accesstype != L_CLONE &&
        accesstype != L_COPY_CLONE)
        return (PIXA *)ERROR_PTR("invalid accesstype", procName, NULL);

    if ((pixa = pixaa->pixa[index]) == NULL)  /* shouldn't happen! */
        return (PIXA *)ERROR_PTR("no pixa[index]", procName, NULL);
    return pixaCopy(pixa, accesstype);
}


/*!
 *  pixaaGetBoxa()
 *
 *      Input:  pixaa
 *              accesstype  (L_COPY, L_CLONE)
 *      Return: boxa, or null on error
 *
 *  Notes:
 *      (1) L_COPY returns a copy; L_CLONE returns a new reference to the boxa.
 *      (2) In both cases, invoke boxaDestroy() on the returned boxa.
 */
BOXA *
pixaaGetBoxa(PIXAA   *pixaa,
             l_int32  accesstype)
{
    PROCNAME("pixaaGetBoxa");

    if (!pixaa)
        return (BOXA *)ERROR_PTR("pixaa not defined", procName, NULL);
    if (accesstype != L_COPY && accesstype != L_CLONE)
        return (BOXA *)ERROR_PTR("invalid access type", procName, NULL);

    return boxaCopy(pixaa->boxa, accesstype);
}


/*---------------------------------------------------------------------*
 *                          Pixa serialized I/O                        *
 *---------------------------------------------------------------------*/
/*!
 *  pixaRead()
 *
 *      Input:  filename
 *      Return: pixa, or null on error
 *
 *  Notes:
 *      (1) The pix are stored in the file as png.
 */
PIXA *
pixaRead(const char  *filename)
{
FILE  *fp;
PIXA  *pixa;

    PROCNAME("pixaRead");

    if (!filename)
        return (PIXA *)ERROR_PTR("filename not defined", procName, NULL);
    if ((fp = fopenReadStream(filename)) == NULL)
        return (PIXA *)ERROR_PTR("stream not opened", procName, NULL);

    if ((pixa = pixaReadStream(fp)) == NULL) {
        fclose(fp);
        return (PIXA *)ERROR_PTR("pixa not read", procName, NULL);
    }

    fclose(fp);
    return pixa;
}


/*!
 *  pixaReadStream()
 *
 *      Input:  stream
 *      Return: pixa, or null on error
 */
PIXA *
pixaReadStream(FILE  *fp)
{
l_int32  n, i, xres, yres, version;
l_int32  ignore;
BOXA    *boxa;
PIX     *pix;
PIXA    *pixa;

    PROCNAME("pixaReadStream");

#if  !HAVE_LIBPNG  /* defined in environ.h */
    return (PIXA *)ERROR_PTR("no libpng: can't read data", procName, NULL);
#else

    if (!fp)
        return (PIXA *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nPixa Version %d\n", &version) != 1)
        return (PIXA *)ERROR_PTR("not a pixa file", procName, NULL);
    if (version != PIXA_VERSION_NUMBER)
        return (PIXA *)ERROR_PTR("invalid pixa version", procName, NULL);
    if (fscanf(fp, "Number of pix = %d\n", &n) != 1)
        return (PIXA *)ERROR_PTR("not a pixa file", procName, NULL);

    if ((pixa = pixaCreate(n)) == NULL)
        return (PIXA *)ERROR_PTR("pixa not made", procName, NULL);
    if ((boxa = boxaReadStream(fp)) == NULL)
        return (PIXA *)ERROR_PTR("boxa not made", procName, NULL);
    boxaDestroy(&pixa->boxa);
    pixa->boxa = boxa;

    for (i = 0; i < n; i++) {
        if ((fscanf(fp, " pix[%d]: xres = %d, yres = %d\n",
              &ignore, &xres, &yres)) != 3)
            return (PIXA *)ERROR_PTR("res reading", procName, NULL);
        if ((pix = pixReadStreamPng(fp)) == NULL)
            return (PIXA *)ERROR_PTR("pix not read", procName, NULL);
        pixSetXRes(pix, xres);
        pixSetYRes(pix, yres);
        pixaAddPix(pixa, pix, L_INSERT);
    }

    return pixa;

#endif  /* !HAVE_LIBPNG */
}


/*!
 *  pixaWrite()
 *
 *      Input:  filename
 *              pixa
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The pix are written to file in png format.
 */
l_int32
pixaWrite(const char  *filename,
          PIXA        *pixa)
{
FILE  *fp;

    PROCNAME("pixaWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    if ((fp = fopen(filename, "w")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (pixaWriteStream(fp, pixa))
        return ERROR_INT("pixa not written to stream", procName, 1);
    fclose(fp);

    return 0;
}


/*!
 *  pixaWriteStream()
 *
 *      Input:  stream
 *              pixa
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixaWriteStream(FILE  *fp,
                PIXA  *pixa)
{
l_int32  n, i;
PIX     *pix;

    PROCNAME("pixaWriteStream");

#if  !HAVE_LIBPNG  /* defined in environ.h */
    return ERROR_INT("no libpng: can't write data", procName, 1);
#else

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    n = pixaGetCount(pixa);
    fprintf(fp, "\nPixa Version %d\n", PIXA_VERSION_NUMBER);
    fprintf(fp, "Number of pix = %d\n", n);
    boxaWriteStream(fp, pixa->boxa);
    for (i = 0; i < n; i++) {
        if ((pix = pixaGetPix(pixa, i, L_CLONE)) == NULL)
            return ERROR_INT("pix not found", procName, 1);
        fprintf(fp, " pix[%d]: xres = %d, yres = %d\n",
                i, pix->xres, pix->yres);
        pixWriteStreamPng(fp, pix, 0.0);
        pixDestroy(&pix);
    }
    return 0;

#endif  /* !HAVE_LIBPNG */
}


/*---------------------------------------------------------------------*
 *                         Pixaa serialized I/O                        *
 *---------------------------------------------------------------------*/
/*!
 *  pixaaRead()
 *
 *      Input:  filename
 *      Return: pixaa, or null on error
 *
 *  Notes:
 *      (1) The pix are stored in the file as png.
 */
PIXAA *
pixaaRead(const char  *filename)
{
FILE   *fp;
PIXAA  *pixaa;

    PROCNAME("pixaaRead");

    if (!filename)
        return (PIXAA *)ERROR_PTR("filename not defined", procName, NULL);
    if ((fp = fopenReadStream(filename)) == NULL)
        return (PIXAA *)ERROR_PTR("stream not opened", procName, NULL);

    if ((pixaa = pixaaReadStream(fp)) == NULL) {
        fclose(fp);
        return (PIXAA *)ERROR_PTR("pixaa not read", procName, NULL);
    }

    fclose(fp);
    return pixaa;
}


/*!
 *  pixaaReadStream()
 *
 *      Input:  stream
 *      Return: pixaa, or null on error
 */
PIXAA *
pixaaReadStream(FILE  *fp)
{
l_int32  n, i, version;
l_int32  ignore;
BOXA    *boxa;
PIXA    *pixa;
PIXAA   *pixaa;

    PROCNAME("pixaaReadStream");

    if (!fp)
        return (PIXAA *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nPixaa Version %d\n", &version) != 1)
        return (PIXAA *)ERROR_PTR("not a pixaa file", procName, NULL);
    if (version != PIXAA_VERSION_NUMBER)
        return (PIXAA *)ERROR_PTR("invalid pixaa version", procName, NULL);
    if (fscanf(fp, "Number of pixa = %d\n", &n) != 1)
        return (PIXAA *)ERROR_PTR("not a pixaa file", procName, NULL);

    if ((pixaa = pixaaCreate(n)) == NULL)
        return (PIXAA *)ERROR_PTR("pixaa not made", procName, NULL);
    if ((boxa = boxaReadStream(fp)) == NULL)
        return (PIXAA *)ERROR_PTR("boxa not made", procName, NULL);
    boxaDestroy(&pixaa->boxa);
    pixaa->boxa = boxa;

    for (i = 0; i < n; i++) {
        if ((fscanf(fp, "\n\n --------------- pixa[%d] ---------------\n",
                    &ignore)) != 1) {
            return (PIXAA *)ERROR_PTR("text reading", procName, NULL);
        }
        if ((pixa = pixaReadStream(fp)) == NULL)
            return (PIXAA *)ERROR_PTR("pixa not read", procName, NULL);
        pixaaAddPixa(pixaa, pixa, L_INSERT);
    }

    return pixaa;
}


/*!
 *  pixaaWrite()
 *
 *      Input:  filename
 *              pixaa
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Serialization of pixaa; the pix are written in png
 */
l_int32
pixaaWrite(const char  *filename,
           PIXAA       *pixaa)
{
FILE  *fp;

    PROCNAME("pixaaWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!pixaa)
        return ERROR_INT("pixaa not defined", procName, 1);

    if ((fp = fopen(filename, "w")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (pixaaWriteStream(fp, pixaa))
        return ERROR_INT("pixaa not written to stream", procName, 1);
    fclose(fp);

    return 0;
}


/*!
 *  pixaaWriteStream()
 *
 *      Input:  stream
 *              pixaa
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixaaWriteStream(FILE   *fp,
                 PIXAA  *pixaa)
{
l_int32  n, i;
PIXA    *pixa;

    PROCNAME("pixaaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!pixaa)
        return ERROR_INT("pixaa not defined", procName, 1);

    n = pixaaGetCount(pixaa);
    fprintf(fp, "\nPixaa Version %d\n", PIXAA_VERSION_NUMBER);
    fprintf(fp, "Number of pixa = %d\n", n);
    boxaWriteStream(fp, pixaa->boxa);
    for (i = 0; i < n; i++) {
        if ((pixa = pixaaGetPixa(pixaa, i, L_CLONE)) == NULL)
            return ERROR_INT("pixa not found", procName, 1);
        fprintf(fp, "\n\n --------------- pixa[%d] ---------------\n", i);
        pixaWriteStream(fp, pixa);
        pixaDestroy(&pixa);
    }
    return 0;
}


