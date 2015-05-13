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
 *           l_int32   pixaAddBox()
 *           static l_int32   pixaExtendArray()
 *           l_int32   pixaExtendArrayToSize()
 *
 *      Pixa accessors
 *           l_int32   pixaGetCount()
 *           l_int32   pixaChangeRefcount()
 *           PIX      *pixaGetPix()
 *           l_int32   pixaGetPixDimensions()
 *           BOXA     *pixaGetBoxa()
 *           l_int32   pixaGetBoxaCount()
 *           BOX      *pixaGetBox()
 *           l_int32   pixaGetBoxGeometry()
 *           l_int32   pixaSetBoxa()
 *           PIX     **pixaGetPixArray()
 *           l_int32   pixaVerifyDepth()
 *           l_int32   pixaIsFull()
 *           l_int32   pixaCountText()
 *           void   ***pixaGetLinePtrs()
 *
 *      Pixa array modifiers
 *           l_int32   pixaReplacePix()
 *           l_int32   pixaInsertPix()
 *           l_int32   pixaRemovePix()
 *           l_int32   pixaRemovePixAndSave()
 *           l_int32   pixaInitFull()
 *           l_int32   pixaClear()
 *
 *      Pixa and Pixaa combination
 *           l_int32   pixaJoin()
 *           l_int32   pixaaJoin()
 *
 *      Pixaa creation, destruction
 *           PIXAA    *pixaaCreate()
 *           PIXAA    *pixaaCreateFromPixa()
 *           void      pixaaDestroy()
 *
 *      Pixaa addition
 *           l_int32   pixaaAddPixa()
 *           l_int32   pixaaExtendArray()
 *           l_int32   pixaaAddPix()
 *           l_int32   pixaaAddBox()
 *
 *      Pixaa accessors
 *           l_int32   pixaaGetCount()
 *           PIXA     *pixaaGetPixa()
 *           BOXA     *pixaaGetBoxa()
 *           PIX      *pixaaGetPix()
 *           l_int32   pixaaVerifyDepth()
 *           l_int32   pixaaIsFull()
 *
 *      Pixaa array modifiers
 *           l_int32   pixaaInitFull()
 *           l_int32   pixaaReplacePixa()
 *           l_int32   pixaaClear()
 *           l_int32   pixaaTruncate()
 *
 *      Pixa serialized I/O  (requires png support)
 *           PIXA     *pixaRead()
 *           PIXA     *pixaReadStream()
 *           l_int32   pixaWrite()
 *           l_int32   pixaWriteStream()
 *
 *      Pixaa serialized I/O  (requires png support)
 *           PIXAA    *pixaaReadFromFiles()
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

#include <string.h>
#include "allheaders.h"

static const l_int32  INITIAL_PTR_ARRAYSIZE = 20;   /* n'import quoi */

    /* Static functions */
static l_int32 pixaExtendArray(PIXA  *pixa);


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
 *  Notes:
 *      (1) For bpp = 1, we truncate each retrieved pix to the ON
 *          pixels, which we assume for now start at (0,0)
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
        } else {
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
            } else {
                pixSetAllArbitrary(pixt, bordercolor);
            }
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
        L_WARNING("ptr address is NULL!\n", procName);
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
 *              copyflag (see pix.h for details):
 *                L_COPY makes a new pixa and copies each pix and each box
 *                L_CLONE gives a new ref-counted handle to the input pixa
 *                L_COPY_CLONE makes a new pixa and inserts clones of
 *                    all pix and boxes
 *      Return: new pixa, or null on error
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
        } else {  /* copy-clone */
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


/*!
 *  pixaExtendArray()
 *
 *      Input:  pixa
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Doubles the size of the pixa and boxa ptr arrays.
 */
static l_int32
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
PIX  *pix;

    PROCNAME("pixaGetPix");

    if (!pixa)
        return (PIX *)ERROR_PTR("pixa not defined", procName, NULL);
    if (index < 0 || index >= pixa->n)
        return (PIX *)ERROR_PTR("index not valid", procName, NULL);
    if ((pix = pixa->pix[index]) == NULL) {
        L_ERROR("no pix at pixa[%d]\n", procName, index);
        return (PIX *)ERROR_PTR("pix not found!", procName, NULL);
    }

    if (accesstype == L_COPY)
        return pixCopy(NULL, pix);
    else if (accesstype == L_CLONE)
        return pixClone(pix);
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

    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (pd) *pd = 0;
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
    } else {
        return NULL;
    }
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

    if (px) *px = 0;
    if (py) *py = 0;
    if (pw) *pw = 0;
    if (ph) *ph = 0;
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
 *  pixaSetBoxa()
 *
 *      Input:  pixa
 *              boxa
 *              accesstype  (L_INSERT, L_COPY, L_CLONE)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This destroys the existing boxa in the pixa.
 */
l_int32
pixaSetBoxa(PIXA    *pixa,
            BOXA    *boxa,
            l_int32  accesstype)
{
    PROCNAME("pixaSetBoxa");

    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (accesstype != L_INSERT && accesstype != L_COPY &&
        accesstype != L_CLONE)
        return ERROR_INT("invalid access type", procName, 1);

    boxaDestroy(&pixa->boxa);
    if (accesstype == L_INSERT)
        pixa->boxa = boxa;
    else
        pixa->boxa = boxaCopy(boxa, accesstype);

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


/*!
 *  pixaVerifyDepth()
 *
 *      Input:  pixa
 *              &maxdepth (<optional return> max depth of all pix)
 *      Return: depth (return 0 if they're not all the same, or on error)
 *
 *  Notes:
 *      (1) It is considered to be an error if there are no pix.
 */
l_int32
pixaVerifyDepth(PIXA     *pixa,
                l_int32  *pmaxdepth)
{
l_int32  i, n, d, depth, maxdepth, same;

    PROCNAME("pixaVerifyDepth");

    if (pmaxdepth) *pmaxdepth = 0;
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 0);

    depth = 0;
    n = pixaGetCount(pixa);
    maxdepth = 0;
    same = 1;
    for (i = 0; i < n; i++) {
        if (pixaGetPixDimensions(pixa, i, NULL, NULL, &d))
            return ERROR_INT("pix depth not found", procName, 0);
        maxdepth = L_MAX(maxdepth, d);
        if (i == 0)
            depth = d;
        else if (d != depth)
            same = 0;
    }
    if (pmaxdepth) *pmaxdepth = maxdepth;
    return (same == 1) ? depth : 0;
}


/*!
 *  pixaIsFull()
 *
 *      Input:  pixa
 *              &fullpa (<optional return> 1 if pixa is full)
 *              &fullba (<optional return> 1 if boxa is full)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) A pixa is "full" if the array of pix is fully
 *          occupied from index 0 to index (pixa->n - 1).
 */
l_int32
pixaIsFull(PIXA     *pixa,
           l_int32  *pfullpa,
           l_int32  *pfullba)
{
l_int32  i, n, full;
BOXA    *boxa;
PIX     *pix;

    PROCNAME("pixaIsFull");

    if (pfullpa) *pfullpa = 0;
    if (pfullba) *pfullba = 0;
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    n = pixaGetCount(pixa);
    if (pfullpa) {
        full = 1;
        for (i = 0; i < n; i++) {
            if ((pix = pixaGetPix(pixa, i, L_CLONE)) == NULL) {
                full = 0;
                break;
            }
            pixDestroy(&pix);
        }
        *pfullpa = full;
    }
    if (pfullba) {
        boxa = pixaGetBoxa(pixa, L_CLONE);
        boxaIsFull(boxa, pfullba);
        boxaDestroy(&boxa);
    }
    return 0;
}


/*!
 *  pixaCountText()
 *
 *      Input:  pixa
 *              &ntext (<return> number of pix with non-empty text strings)
 *      Return: 0 if OK, 1 on error.
 *
 *  Notes:
 *      (1) All pix have non-empty text strings if the returned value @ntext
 *          equals the pixa count.
 */
l_int32
pixaCountText(PIXA     *pixa,
              l_int32  *pntext)
{
char    *text;
l_int32  i, n;
PIX     *pix;

    PROCNAME("pixaCountText");

    if (!pntext)
        return ERROR_INT("&ntext not defined", procName, 1);
    *pntext = 0;
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    n = pixaGetCount(pixa);
    for (i = 0; i < n; i++) {
        if ((pix = pixaGetPix(pixa, i, L_CLONE)) == NULL)
            continue;
        text = pixGetText(pix);
        if (text && strlen(text) > 0)
            (*pntext)++;
        pixDestroy(&pix);
    }

    return 0;
}


/*!
 *  pixaGetLinePtrs()
 *
 *      Input:  pixa (of pix that all have the same depth)
 *              &size (<optional return> number of pix in the pixa)
 *      Return: array of array of line ptrs, or null on error
 *
 *  Notes:
 *      (1) See pixGetLinePtrs() for details.
 *      (2) It is best if all pix in the pixa are the same size.
 *          The size of each line ptr array is equal to the height
 *          of the pix that it refers to.
 *      (3) This is an array of arrays.  To destroy it:
 *            for (i = 0; i < size; i++)
 *                FREE(lineset[i]);
 *            FREE(lineset);
 */
void ***
pixaGetLinePtrs(PIXA     *pixa,
                l_int32  *psize)
{
l_int32  i, n;
void   **lineptrs;
void  ***lineset;
PIX     *pix;

    PROCNAME("pixaGetLinePtrs");

    if (psize) *psize = 0;
    if (!pixa)
        return (void ***)ERROR_PTR("pixa not defined", procName, NULL);
    if (pixaVerifyDepth(pixa, NULL) == 0)
        return (void ***)ERROR_PTR("pixa not all same depth", procName, NULL);
    n = pixaGetCount(pixa);
    if (psize) *psize = n;
    if ((lineset = (void ***)CALLOC(n, sizeof(void **))) == NULL)
        return (void ***)ERROR_PTR("lineset not made", procName, NULL);
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
        lineptrs = pixGetLinePtrs(pix, NULL);
        lineset[i] = lineptrs;
        pixDestroy(&pix);
    }

    return lineset;
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

        /* Remove the box if it exists */
    boxa = pixa->boxa;
    nbox = boxaGetCount(boxa);
    if (index < nbox)
        boxaRemoveBox(boxa, index);

    return 0;
}


/*!
 *  pixaRemovePixAndSave()
 *
 *      Input:  pixa
 *              index (of pix to be removed)
 *              &pix (<optional return> removed pix)
 *              &box (<optional return> removed box)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This shifts pixa[i] --> pixa[i - 1] for all i > index.
 *      (2) It should not be used repeatedly on large arrays,
 *          because the function is O(n).
 *      (3) The corresponding box is removed as well, if it exists.
 *      (4) The removed pix and box can either be retained or destroyed.
 */
l_int32
pixaRemovePixAndSave(PIXA    *pixa,
                     l_int32  index,
                     PIX    **ppix,
                     BOX    **pbox)
{
l_int32  i, n, nbox;
BOXA    *boxa;
PIX    **array;

    PROCNAME("pixaRemovePixAndSave");

    if (ppix) *ppix = NULL;
    if (pbox) *pbox = NULL;
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    n = pixaGetCount(pixa);
    if (index < 0 || index >= n)
        return ERROR_INT("index not in {0...n - 1}", procName, 1);

        /* Remove the pix */
    array = pixa->pix;
    if (ppix)
        *ppix = pixaGetPix(pixa, index, L_CLONE);
    pixDestroy(&array[index]);
    for (i = index + 1; i < n; i++)
        array[i - 1] = array[i];
    array[n - 1] = NULL;
    pixa->n--;

        /* Remove the box if it exists  */
    boxa = pixa->boxa;
    nbox = boxaGetCount(boxa);
    if (index < nbox)
        boxaRemoveBoxAndSave(boxa, index, pbox);

    return 0;
}


/*!
 *  pixaInitFull()
 *
 *      Input:  pixa (typically empty)
 *              pix (<optional> to be replicated into the entire pixa ptr array)
 *              box (<optional> to be replicated into the entire boxa ptr array)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This initializes a pixa by filling up the entire pix ptr array
 *          with copies of @pix.  If @pix == NULL, we use a tiny placeholder
 *          pix (w = h = d = 1).  Any existing pix are destroyed.
 *          It also optionally fills the boxa with copies of @box.
 *          After this operation, the numbers of pix and (optionally)
 *          boxes are equal to the number of allocated ptrs.
 *      (2) Note that we use pixaReplacePix() instead of pixaInsertPix().
 *          They both have the same effect when inserting into a NULL ptr
 *          in the pixa ptr array:
 *      (3) If the boxa is not initialized (i.e., filled with boxes),
 *          later insertion of boxes will cause an error, because the
 *          'n' field is 0.
 *      (4) Example usage.  This function is useful to prepare for a
 *          random insertion (or replacement) of pix into a pixa.
 *          To randomly insert pix into a pixa, without boxes, up to
 *          some index "max":
 *             Pixa *pixa = pixaCreate(max);
 *             pixaInitFull(pixa, NULL, NULL);
 *          An existing pixa with a smaller ptr array can also be reused:
 *             pixaExtendArrayToSize(pixa, max);
 *             pixaInitFull(pixa, NULL, NULL);
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

    n = pixa->nalloc;
    pixa->n = n;
    for (i = 0; i < n; i++) {
        if (pix)
            pixt = pixCopy(NULL, pix);
        else
            pixt = pixCreate(1, 1, 1);
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
    return boxaClear(pixa->boxa);
}


/*---------------------------------------------------------------------*
 *                      Pixa and Pixaa combination                     *
 *---------------------------------------------------------------------*/
/*!
 *  pixaJoin()
 *
 *      Input:  pixad  (dest pixa; add to this one)
 *              pixas  (<optional> source pixa; add from this one)
 *              istart  (starting index in pixas)
 *              iend  (ending index in pixas; use -1 to cat all)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This appends a clone of each indicated pix in pixas to pixad
 *      (2) istart < 0 is taken to mean 'read from the start' (istart = 0)
 *      (3) iend < 0 means 'read to the end'
 *      (4) If pixas is NULL or contains no pix, this is a no-op.
 */
l_int32
pixaJoin(PIXA    *pixad,
         PIXA    *pixas,
         l_int32  istart,
         l_int32  iend)
{
l_int32  i, n, nb;
BOXA    *boxas, *boxad;
PIX     *pix;

    PROCNAME("pixaJoin");

    if (!pixad)
        return ERROR_INT("pixad not defined", procName, 1);
    if (!pixas || ((n = pixaGetCount(pixas)) == 0))
        return 0;

    if (istart < 0)
        istart = 0;
    if (iend < 0 || iend >= n)
        iend = n - 1;
    if (istart > iend)
        return ERROR_INT("istart > iend; nothing to add", procName, 1);

    for (i = istart; i <= iend; i++) {
        pix = pixaGetPix(pixas, i, L_CLONE);
        pixaAddPix(pixad, pix, L_INSERT);
    }

    boxas = pixaGetBoxa(pixas, L_CLONE);
    boxad = pixaGetBoxa(pixad, L_CLONE);
    nb = pixaGetBoxaCount(pixas);
    iend = L_MIN(iend, nb - 1);
    boxaJoin(boxad, boxas, istart, iend);
    boxaDestroy(&boxas);  /* just the clones */
    boxaDestroy(&boxad);
    return 0;
}


/*!
 *  pixaaJoin()
 *
 *      Input:  paad  (dest pixaa; add to this one)
 *              paas  (<optional> source pixaa; add from this one)
 *              istart  (starting index in pixaas)
 *              iend  (ending index in pixaas; use -1 to cat all)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This appends a clone of each indicated pixa in paas to pixaad
 *      (2) istart < 0 is taken to mean 'read from the start' (istart = 0)
 *      (3) iend < 0 means 'read to the end'
 */
l_int32
pixaaJoin(PIXAA   *paad,
          PIXAA   *paas,
          l_int32  istart,
          l_int32  iend)
{
l_int32  i, n;
PIXA    *pixa;

    PROCNAME("pixaaJoin");

    if (!paad)
        return ERROR_INT("pixaad not defined", procName, 1);
    if (!paas)
        return 0;

    if (istart < 0)
        istart = 0;
    n = pixaaGetCount(paas, NULL);
    if (iend < 0 || iend >= n)
        iend = n - 1;
    if (istart > iend)
        return ERROR_INT("istart > iend; nothing to add", procName, 1);

    for (i = istart; i <= iend; i++) {
        pixa = pixaaGetPixa(paas, i, L_CLONE);
        pixaaAddPixa(paad, pixa, L_INSERT);
    }

    return 0;
}


/*---------------------------------------------------------------------*
 *                    Pixaa creation and destruction                   *
 *---------------------------------------------------------------------*/
/*!
 *  pixaaCreate()
 *
 *      Input:  n  (initial number of pixa ptrs)
 *      Return: paa, or null on error
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
PIXAA  *paa;

    PROCNAME("pixaaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((paa = (PIXAA *)CALLOC(1, sizeof(PIXAA))) == NULL)
        return (PIXAA *)ERROR_PTR("paa not made", procName, NULL);
    paa->n = 0;
    paa->nalloc = n;

    if ((paa->pixa = (PIXA **)CALLOC(n, sizeof(PIXA *))) == NULL) {
        pixaaDestroy(&paa);
        return (PIXAA *)ERROR_PTR("pixa ptrs not made", procName, NULL);
    }
    paa->boxa = boxaCreate(n);

    return paa;
}


/*!
 *  pixaaCreateFromPixa()
 *
 *      Input:  pixa
 *              n (number specifying subdivision of pixa)
 *              type (L_CHOOSE_CONSECUTIVE, L_CHOOSE_SKIP_BY)
 *              copyflag (L_CLONE, L_COPY)
 *      Return: paa, or null on error
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
PIXAA   *paa;

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
    paa = pixaaCreate(npixa);
    if (type == L_CHOOSE_CONSECUTIVE) {
        for (i = 0; i < count; i++) {
            if (i % n == 0)
                pixat = pixaCreate(n);
            pix = pixaGetPix(pixa, i, copyflag);
            pixaAddPix(pixat, pix, L_INSERT);
            if (i % n == n - 1)
                pixaaAddPixa(paa, pixat, L_INSERT);
        }
        if (i % n != 0)
            pixaaAddPixa(paa, pixat, L_INSERT);
    } else {  /* L_CHOOSE_SKIP_BY */
        for (i = 0; i < npixa; i++) {
            pixat = pixaCreate(count / npixa + 1);
            for (j = i; j < count; j += n) {
                pix = pixaGetPix(pixa, j, copyflag);
                pixaAddPix(pixat, pix, L_INSERT);
            }
            pixaaAddPixa(paa, pixat, L_INSERT);
        }
    }

    return paa;
}


/*!
 *  pixaaDestroy()
 *
 *      Input:  &paa <to be nulled>
 *      Return: void
 */
void
pixaaDestroy(PIXAA  **ppaa)
{
l_int32  i;
PIXAA   *paa;

    PROCNAME("pixaaDestroy");

    if (ppaa == NULL) {
        L_WARNING("ptr address is NULL!\n", procName);
        return;
    }

    if ((paa = *ppaa) == NULL)
        return;

    for (i = 0; i < paa->n; i++)
        pixaDestroy(&paa->pixa[i]);
    FREE(paa->pixa);
    boxaDestroy(&paa->boxa);

    FREE(paa);
    *ppaa = NULL;

    return;
}


/*---------------------------------------------------------------------*
 *                             Pixaa addition                          *
 *---------------------------------------------------------------------*/
/*!
 *  pixaaAddPixa()
 *
 *      Input:  paa
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
pixaaAddPixa(PIXAA   *paa,
             PIXA    *pixa,
             l_int32  copyflag)
{
l_int32  n;
PIXA    *pixac;

    PROCNAME("pixaaAddPixa");

    if (!paa)
        return ERROR_INT("paa not defined", procName, 1);
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    if (copyflag != L_INSERT && copyflag != L_COPY &&
        copyflag != L_CLONE && copyflag != L_COPY_CLONE)
        return ERROR_INT("invalid copyflag", procName, 1);

    if (copyflag == L_INSERT) {
        pixac = pixa;
    } else {
        if ((pixac = pixaCopy(pixa, copyflag)) == NULL)
            return ERROR_INT("pixac not made", procName, 1);
    }

    n = pixaaGetCount(paa, NULL);
    if (n >= paa->nalloc)
        pixaaExtendArray(paa);
    paa->pixa[n] = pixac;
    paa->n++;

    return 0;
}


/*!
 *  pixaaExtendArray()
 *
 *      Input:  paa
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixaaExtendArray(PIXAA  *paa)
{
    PROCNAME("pixaaExtendArray");

    if (!paa)
        return ERROR_INT("paa not defined", procName, 1);

    if ((paa->pixa = (PIXA **)reallocNew((void **)&paa->pixa,
                             sizeof(PIXA *) * paa->nalloc,
                             2 * sizeof(PIXA *) * paa->nalloc)) == NULL)
        return ERROR_INT("new ptr array not returned", procName, 1);

    paa->nalloc = 2 * paa->nalloc;
    return 0;
}


/*!
 *  pixaaAddPix()
 *
 *      Input:  paa  (input paa)
 *              index (index of pixa in paa)
 *              pix (to be added)
 *              box (<optional> to be added)
 *              copyflag (L_INSERT, L_COPY, L_CLONE)
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixaaAddPix(PIXAA   *paa,
            l_int32  index,
            PIX     *pix,
            BOX     *box,
            l_int32  copyflag)
{
PIXA  *pixa;

    PROCNAME("pixaaAddPix");

    if (!paa)
        return ERROR_INT("paa not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    if ((pixa = pixaaGetPixa(paa, index, L_CLONE)) == NULL)
        return ERROR_INT("pixa not found", procName, 1);
    pixaAddPix(pixa, pix, copyflag);
    if (box) pixaAddBox(pixa, box, copyflag);
    pixaDestroy(&pixa);
    return 0;
}


/*!
 *  pixaaAddBox()
 *
 *      Input:  paa
 *              box
 *              copyflag (L_INSERT, L_COPY, L_CLONE)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The box can be used, for example, to hold the support region
 *          of a pixa that is being added to the pixaa.
 */
l_int32
pixaaAddBox(PIXAA   *paa,
            BOX     *box,
            l_int32  copyflag)
{
    PROCNAME("pixaaAddBox");

    if (!paa)
        return ERROR_INT("paa not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if (copyflag != L_INSERT && copyflag != L_COPY && copyflag != L_CLONE)
        return ERROR_INT("invalid copyflag", procName, 1);

    boxaAddBox(paa->boxa, box, copyflag);
    return 0;
}



/*---------------------------------------------------------------------*
 *                            Pixaa accessors                          *
 *---------------------------------------------------------------------*/
/*!
 *  pixaaGetCount()
 *
 *      Input:  paa
 *              &na (<optional return> number of pix in each pixa)
 *      Return: count, or 0 if no pixaa
 *
 *  Notes:
 *      (1) If paa is empty, a returned na will also be empty.
 */
l_int32
pixaaGetCount(PIXAA  *paa,
              NUMA  **pna)
{
l_int32  i, n;
NUMA    *na;
PIXA    *pixa;

    PROCNAME("pixaaGetCount");

    if (pna) *pna = NULL;
    if (!paa)
        return ERROR_INT("paa not defined", procName, 0);

    n = paa->n;
    if (pna) {
        if ((na = numaCreate(n)) == NULL)
            return ERROR_INT("na not made", procName, 0);
        *pna = na;
        for (i = 0; i < n; i++) {
            pixa = pixaaGetPixa(paa, i, L_CLONE);
            numaAddNumber(na, pixaGetCount(pixa));
            pixaDestroy(&pixa);
        }
    }
    return n;
}


/*!
 *  pixaaGetPixa()
 *
 *      Input:  paa
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
pixaaGetPixa(PIXAA   *paa,
             l_int32  index,
             l_int32  accesstype)
{
PIXA  *pixa;

    PROCNAME("pixaaGetPixa");

    if (!paa)
        return (PIXA *)ERROR_PTR("paa not defined", procName, NULL);
    if (index < 0 || index >= paa->n)
        return (PIXA *)ERROR_PTR("index not valid", procName, NULL);
    if (accesstype != L_COPY && accesstype != L_CLONE &&
        accesstype != L_COPY_CLONE)
        return (PIXA *)ERROR_PTR("invalid accesstype", procName, NULL);

    if ((pixa = paa->pixa[index]) == NULL) {  /* shouldn't happen! */
        L_ERROR("missing pixa[%d]\n", procName, index);
        return (PIXA *)ERROR_PTR("pixa not found at index", procName, NULL);
    }
    return pixaCopy(pixa, accesstype);
}


/*!
 *  pixaaGetBoxa()
 *
 *      Input:  paa
 *              accesstype  (L_COPY, L_CLONE)
 *      Return: boxa, or null on error
 *
 *  Notes:
 *      (1) L_COPY returns a copy; L_CLONE returns a new reference to the boxa.
 *      (2) In both cases, invoke boxaDestroy() on the returned boxa.
 */
BOXA *
pixaaGetBoxa(PIXAA   *paa,
             l_int32  accesstype)
{
    PROCNAME("pixaaGetBoxa");

    if (!paa)
        return (BOXA *)ERROR_PTR("paa not defined", procName, NULL);
    if (accesstype != L_COPY && accesstype != L_CLONE)
        return (BOXA *)ERROR_PTR("invalid access type", procName, NULL);

    return boxaCopy(paa->boxa, accesstype);
}


/*!
 *  pixaaGetPix()
 *
 *      Input:  paa
 *              index  (index into the pixa array in the pixaa)
 *              ipix  (index into the pix array in the pixa)
 *              accessflag  (L_COPY or L_CLONE)
 *      Return: pix, or null on error
 */
PIX *
pixaaGetPix(PIXAA   *paa,
            l_int32  index,
            l_int32  ipix,
            l_int32  accessflag)
{
PIX   *pix;
PIXA  *pixa;

    PROCNAME("pixaaGetPix");

    if ((pixa = pixaaGetPixa(paa, index, L_CLONE)) == NULL)
        return (PIX *)ERROR_PTR("pixa not retrieved", procName, NULL);
    if ((pix = pixaGetPix(pixa, ipix, accessflag)) == NULL)
        L_ERROR("pix not retrieved\n", procName);
    pixaDestroy(&pixa);
    return pix;
}


/*!
 *  pixaaVerifyDepth()
 *
 *      Input:  paa
 *              &maxdepth (<optional return> max depth of all pix in pixaa)
 *      Return: depth (return 0 if they're not all the same, or on error)
 */
l_int32
pixaaVerifyDepth(PIXAA    *paa,
                 l_int32  *pmaxdepth)
{
l_int32  i, npixa, d, maxd, maxdepth, same;
PIXA    *pixa;

    PROCNAME("pixaaVerifyDepth");

    if (pmaxdepth) *pmaxdepth = 0;
    if (!paa)
        return ERROR_INT("paa not defined", procName, 0);

    npixa = pixaaGetCount(paa, NULL);
    maxdepth = 0;
    same = 1;
    for (i = 0; i < npixa; i++) {
        pixa = pixaaGetPixa(paa, i, L_CLONE);
        if (pixaGetCount(pixa) > 0) {
            d = pixaVerifyDepth(pixa, &maxd);
            maxdepth = L_MAX(maxdepth, maxd);  /* biggest up to this point */
            if (d != maxdepth) same = 0;
        }
        pixaDestroy(&pixa);
    }
    if (pmaxdepth) *pmaxdepth = maxdepth;
    return (same == 1) ? maxdepth : 0;
}


/*!
 *  pixaaIsFull()
 *
 *      Input:  paa
 *              &full (<return> 1 if all pixa in the paa have full pix arrays)
 *      Return: return 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Does not require boxa associated with each pixa to be full.
 */
l_int32
pixaaIsFull(PIXAA    *paa,
            l_int32  *pfull)
{
l_int32  i, n, full;
PIXA    *pixa;

    PROCNAME("pixaaIsFull");

    if (!pfull)
        return ERROR_INT("&full not defined", procName, 0);
    *pfull = 0;
    if (!paa)
        return ERROR_INT("paa not defined", procName, 0);

    n = pixaaGetCount(paa, NULL);
    full = 1;
    for (i = 0; i < n; i++) {
        pixa = pixaaGetPixa(paa, i, L_CLONE);
        pixaIsFull(pixa, &full, NULL);
        pixaDestroy(&pixa);
        if (!full) break;
    }
    *pfull = full;
    return 0;
}


/*---------------------------------------------------------------------*
 *                         Pixaa array modifiers                       *
 *---------------------------------------------------------------------*/
/*!
 *  pixaaInitFull()
 *
 *      Input:  paa (typically empty)
 *              pixa (to be replicated into the entire pixa ptr array)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This initializes a pixaa by filling up the entire pixa ptr array
 *          with copies of @pixa.  Any existing pixa are destroyed.
 *      (2) Example usage.  This function is useful to prepare for a
 *          random insertion (or replacement) of pixa into a pixaa.
 *          To randomly insert pixa into a pixaa, up to some index "max":
 *             Pixaa *paa = pixaaCreate(max);
 *             Pixa *pixa = pixaCreate(1);  // if you want little memory
 *             pixaaInitFull(paa, pixa);  // copy it to entire array
 *             pixaDestroy(&pixa);  // no longer needed
 *          The initialization allows the pixaa to always be properly filled.
 */
l_int32
pixaaInitFull(PIXAA  *paa,
              PIXA   *pixa)
{
l_int32  i, n;
PIXA    *pixat;

    PROCNAME("pixaaInitFull");

    if (!paa)
        return ERROR_INT("paa not defined", procName, 1);
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    n = paa->nalloc;
    paa->n = n;
    for (i = 0; i < n; i++) {
        pixat = pixaCopy(pixa, L_COPY);
        pixaaReplacePixa(paa, i, pixat);
    }

    return 0;
}


/*!
 *  pixaaReplacePixa()
 *
 *      Input:  paa
 *              index  (to the index-th pixa)
 *              pixa (insert to replace existing one)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This allows random insertion of a pixa into a pixaa, with
 *          destruction of any existing pixa at that location.
 *          The input pixa is now owned by the pixaa.
 *      (2) No other pixa in the array are affected.
 *      (3) The index must be within the allowed set.
 */
l_int32
pixaaReplacePixa(PIXAA   *paa,
                 l_int32  index,
                 PIXA    *pixa)
{

    PROCNAME("pixaaReplacePixa");

    if (!paa)
        return ERROR_INT("paa not defined", procName, 1);
    if (index < 0 || index >= paa->n)
        return ERROR_INT("index not valid", procName, 1);
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    pixaDestroy(&(paa->pixa[index]));
    paa->pixa[index] = pixa;
    return 0;
}


/*!
 *  pixaaClear()
 *
 *      Input:  paa
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This destroys all pixa in the pixaa, and nulls the ptrs
 *          in the pixa ptr array.
 */
l_int32
pixaaClear(PIXAA  *paa)
{
l_int32  i, n;

    PROCNAME("pixaClear");

    if (!paa)
        return ERROR_INT("paa not defined", procName, 1);

    n = pixaaGetCount(paa, NULL);
    for (i = 0; i < n; i++)
        pixaDestroy(&paa->pixa[i]);
    paa->n = 0;
    return 0;
}


/*!
 *  pixaaTruncate()
 *
 *      Input:  paa
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This identifies the largest index containing a pixa that
 *          has any pix within it, destroys all pixa above that index,
 *          and resets the count.
 */
l_int32
pixaaTruncate(PIXAA  *paa)
{
l_int32  i, n, np;
PIXA    *pixa;

    PROCNAME("pixaaTruncate");

    if (!paa)
        return ERROR_INT("paa not defined", procName, 1);

    n = pixaaGetCount(paa, NULL);
    for (i = n - 1; i >= 0; i--) {
        pixa = pixaaGetPixa(paa, i, L_CLONE);
        if (!pixa) {
            paa->n--;
            continue;
        }
        np = pixaGetCount(pixa);
        pixaDestroy(&pixa);
        if (np == 0) {
            pixaDestroy(&paa->pixa[i]);
            paa->n--;
        } else {
            break;
        }
    }
    return 0;
}



/*---------------------------------------------------------------------*
 *                          Pixa serialized I/O                        *
 *---------------------------------------------------------------------*/
#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

/*!
 *  pixaRead()
 *
 *      Input:  filename
 *      Return: pixa, or null on error
 *
 *  Notes:
 *      (1) The pix are stored in the file as png.
 *          If the png library is not linked, this will fail.
 */
PIXA *
pixaRead(const char  *filename)
{
FILE  *fp;
PIXA  *pixa;

    PROCNAME("pixaRead");

#if !HAVE_LIBPNG     /* defined in environ.h and config_auto.h */
    return (PIXA *)ERROR_PTR("no libpng: can't read data", procName, NULL);
#endif  /* !HAVE_LIBPNG */

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
 *
 *  Notes:
 *      (1) The pix are stored in the file as png.
 *          If the png library is not linked, this will fail.
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

#if !HAVE_LIBPNG     /* defined in environ.h and config_auto.h */
    return (PIXA *)ERROR_PTR("no libpng: can't read data", procName, NULL);
#endif  /* !HAVE_LIBPNG */

    if (!fp)
        return (PIXA *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nPixa Version %d\n", &version) != 1)
        return (PIXA *)ERROR_PTR("not a pixa file", procName, NULL);
    if (version != PIXA_VERSION_NUMBER)
        return (PIXA *)ERROR_PTR("invalid pixa version", procName, NULL);
    if (fscanf(fp, "Number of pix = %d\n", &n) != 1)
        return (PIXA *)ERROR_PTR("not a pixa file", procName, NULL);

    if ((boxa = boxaReadStream(fp)) == NULL)
        return (PIXA *)ERROR_PTR("boxa not made", procName, NULL);
    if ((pixa = pixaCreate(n)) == NULL) {
        boxaDestroy(&boxa);
        return (PIXA *)ERROR_PTR("pixa not made", procName, NULL);
    }
    boxaDestroy(&pixa->boxa);
    pixa->boxa = boxa;

    for (i = 0; i < n; i++) {
        if ((fscanf(fp, " pix[%d]: xres = %d, yres = %d\n",
              &ignore, &xres, &yres)) != 3) {
            pixaDestroy(&pixa);
            return (PIXA *)ERROR_PTR("res reading error", procName, NULL);
        }
        if ((pix = pixReadStreamPng(fp)) == NULL) {
            pixaDestroy(&pixa);
            return (PIXA *)ERROR_PTR("pix not read", procName, NULL);
        }
        pixSetXRes(pix, xres);
        pixSetYRes(pix, yres);
        pixaAddPix(pixa, pix, L_INSERT);
    }
    return pixa;
}


/*!
 *  pixaWrite()
 *
 *      Input:  filename
 *              pixa
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The pix are stored in the file as png.
 *          If the png library is not linked, this will fail.
 */
l_int32
pixaWrite(const char  *filename,
          PIXA        *pixa)
{
FILE  *fp;

    PROCNAME("pixaWrite");

#if !HAVE_LIBPNG     /* defined in environ.h and config_auto.h */
    return ERROR_INT("no libpng: can't write data", procName, 1);
#endif  /* !HAVE_LIBPNG */

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "wb")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (pixaWriteStream(fp, pixa))
        return ERROR_INT("pixa not written to stream", procName, 1);
    fclose(fp);
    return 0;
}


/*!
 *  pixaWriteStream()
 *
 *      Input:  stream (opened for "wb")
 *              pixa
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The pix are stored in the file as png.
 *          If the png library is not linked, this will fail.
 */
l_int32
pixaWriteStream(FILE  *fp,
                PIXA  *pixa)
{
l_int32  n, i;
PIX     *pix;

    PROCNAME("pixaWriteStream");

#if !HAVE_LIBPNG     /* defined in environ.h and config_auto.h */
    return ERROR_INT("no libpng: can't write data", procName, 1);
#endif  /* !HAVE_LIBPNG */

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
}


/*---------------------------------------------------------------------*
 *                         Pixaa serialized I/O                        *
 *---------------------------------------------------------------------*/
/*!
 *  pixaaReadFromFiles()
 *
 *      Input:  dirname (directory)
 *              substr (<optional> substring filter on filenames; can be NULL)
 *              first (0-based)
 *              nfiles (use 0 for everything from @first to the end)
 *      Return: paa, or null on error or if no pixa files are found.
 *
 *  Notes:
 *      (1) The files must be serialized pixa files (e.g., *.pa)
 *          If some files cannot be read, warnings are issued.
 *      (2) Use @substr to filter filenames in the directory.  If
 *          @substr == NULL, this takes all files.
 *      (3) After filtering, use @first and @nfiles to select
 *          a contiguous set of files, that have been lexically
 *          sorted in increasing order.
 */
PIXAA *
pixaaReadFromFiles(const char  *dirname,
                   const char  *substr,
                   l_int32      first,
                   l_int32      nfiles)
{
char    *fname;
l_int32  i, n;
PIXA    *pixa;
PIXAA   *paa;
SARRAY  *sa;

  PROCNAME("pixaaReadFromFiles");

  if (!dirname)
      return (PIXAA *)ERROR_PTR("dirname not defined", procName, NULL);

  sa = getSortedPathnamesInDirectory(dirname, substr, first, nfiles);
  if (!sa || ((n = sarrayGetCount(sa)) == 0)) {
      sarrayDestroy(&sa);
      return (PIXAA *)ERROR_PTR("no pixa files found", procName, NULL);
  }

  paa = pixaaCreate(n);
  for (i = 0; i < n; i++) {
      fname = sarrayGetString(sa, i, L_NOCOPY);
      if ((pixa = pixaRead(fname)) == NULL) {
          L_ERROR("pixa not read for %d-th file", procName, i);
          continue;
      }
      pixaaAddPixa(paa, pixa, L_INSERT);
  }

  sarrayDestroy(&sa);
  return paa;
}


/*!
 *  pixaaRead()
 *
 *      Input:  filename
 *      Return: paa, or null on error
 *
 *  Notes:
 *      (1) The pix are stored in the file as png.
 *          If the png library is not linked, this will fail.
 */
PIXAA *
pixaaRead(const char  *filename)
{
FILE   *fp;
PIXAA  *paa;

    PROCNAME("pixaaRead");

#if !HAVE_LIBPNG     /* defined in environ.h and config_auto.h */
    return (PIXAA *)ERROR_PTR("no libpng: can't read data", procName, NULL);
#endif  /* !HAVE_LIBPNG */

    if (!filename)
        return (PIXAA *)ERROR_PTR("filename not defined", procName, NULL);
    if ((fp = fopenReadStream(filename)) == NULL)
        return (PIXAA *)ERROR_PTR("stream not opened", procName, NULL);

    if ((paa = pixaaReadStream(fp)) == NULL) {
        fclose(fp);
        return (PIXAA *)ERROR_PTR("paa not read", procName, NULL);
    }

    fclose(fp);
    return paa;
}


/*!
 *  pixaaReadStream()
 *
 *      Input:  stream
 *      Return: paa, or null on error
 *
 *  Notes:
 *      (1) The pix are stored in the file as png.
 *          If the png library is not linked, this will fail.
 */
PIXAA *
pixaaReadStream(FILE  *fp)
{
l_int32  n, i, version;
l_int32  ignore;
BOXA    *boxa;
PIXA    *pixa;
PIXAA   *paa;

    PROCNAME("pixaaReadStream");

#if !HAVE_LIBPNG     /* defined in environ.h and config_auto.h */
    return (PIXAA *)ERROR_PTR("no libpng: can't read data", procName, NULL);
#endif  /* !HAVE_LIBPNG */

    if (!fp)
        return (PIXAA *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nPixaa Version %d\n", &version) != 1)
        return (PIXAA *)ERROR_PTR("not a pixaa file", procName, NULL);
    if (version != PIXAA_VERSION_NUMBER)
        return (PIXAA *)ERROR_PTR("invalid pixaa version", procName, NULL);
    if (fscanf(fp, "Number of pixa = %d\n", &n) != 1)
        return (PIXAA *)ERROR_PTR("not a pixaa file", procName, NULL);

    if ((paa = pixaaCreate(n)) == NULL)
        return (PIXAA *)ERROR_PTR("paa not made", procName, NULL);
    if ((boxa = boxaReadStream(fp)) == NULL)
        return (PIXAA *)ERROR_PTR("boxa not made", procName, NULL);
    boxaDestroy(&paa->boxa);
    paa->boxa = boxa;

    for (i = 0; i < n; i++) {
        if ((fscanf(fp, "\n\n --------------- pixa[%d] ---------------\n",
                    &ignore)) != 1) {
            return (PIXAA *)ERROR_PTR("text reading", procName, NULL);
        }
        if ((pixa = pixaReadStream(fp)) == NULL)
            return (PIXAA *)ERROR_PTR("pixa not read", procName, NULL);
        pixaaAddPixa(paa, pixa, L_INSERT);
    }

    return paa;
}


/*!
 *  pixaaWrite()
 *
 *      Input:  filename
 *              paa
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The pix are stored in the file as png.
 *          If the png library is not linked, this will fail.
 */
l_int32
pixaaWrite(const char  *filename,
           PIXAA       *paa)
{
FILE  *fp;

    PROCNAME("pixaaWrite");

#if !HAVE_LIBPNG     /* defined in environ.h and config_auto.h */
    return ERROR_INT("no libpng: can't read data", procName, 1);
#endif  /* !HAVE_LIBPNG */

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!paa)
        return ERROR_INT("paa not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "wb")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (pixaaWriteStream(fp, paa))
        return ERROR_INT("paa not written to stream", procName, 1);
    fclose(fp);

    return 0;
}


/*!
 *  pixaaWriteStream()
 *
 *      Input:  stream (opened for "wb")
 *              paa
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The pix are stored in the file as png.
 *          If the png library is not linked, this will fail.
 */
l_int32
pixaaWriteStream(FILE   *fp,
                 PIXAA  *paa)
{
l_int32  n, i;
PIXA    *pixa;

    PROCNAME("pixaaWriteStream");

#if !HAVE_LIBPNG     /* defined in environ.h and config_auto.h */
    return ERROR_INT("no libpng: can't read data", procName, 1);
#endif  /* !HAVE_LIBPNG */

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!paa)
        return ERROR_INT("paa not defined", procName, 1);

    n = pixaaGetCount(paa, NULL);
    fprintf(fp, "\nPixaa Version %d\n", PIXAA_VERSION_NUMBER);
    fprintf(fp, "Number of pixa = %d\n", n);
    boxaWriteStream(fp, paa->boxa);
    for (i = 0; i < n; i++) {
        if ((pixa = pixaaGetPixa(paa, i, L_CLONE)) == NULL)
            return ERROR_INT("pixa not found", procName, 1);
        fprintf(fp, "\n\n --------------- pixa[%d] ---------------\n", i);
        pixaWriteStream(fp, pixa);
        pixaDestroy(&pixa);
    }
    return 0;
}
