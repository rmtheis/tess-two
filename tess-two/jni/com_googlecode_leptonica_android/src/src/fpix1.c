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
 *  fpix1.c
 *
 *    This file has basic constructors, destructors and field accessors
 *    for FPix, FPixa and DPix.  It also has uncompressed read/write.
 *
 *    FPix Create/copy/destroy
 *          FPIX          *fpixCreate()
 *          FPIX          *fpixCreateTemplate()
 *          FPIX          *fpixClone()
 *          FPIX          *fpixCopy()
 *          l_int32        fpixResizeImageData()
 *          void           fpixDestroy()
 *
 *    FPix accessors
 *          l_int32        fpixGetDimensions()
 *          l_int32        fpixSetDimensions()
 *          l_int32        fpixGetWpl()
 *          l_int32        fpixSetWpl()
 *          l_int32        fpixGetRefcount()
 *          l_int32        fpixChangeRefcount()
 *          l_int32        fpixGetResolution()
 *          l_int32        fpixSetResolution()
 *          l_int32        fpixCopyResolution()
 *          l_float32     *fpixGetData()
 *          l_int32        fpixSetData()
 *          l_int32        fpixGetPixel()
 *          l_int32        fpixSetPixel()
 *
 *    FPixa Create/copy/destroy
 *          FPIXA         *fpixaCreate()
 *          FPIXA         *fpixaCopy()
 *          void           fpixaDestroy()
 *
 *    FPixa addition
 *          l_int32        fpixaAddFPix()
 *          static l_int32 fpixaExtendArray()
 *          static l_int32 fpixaExtendArrayToSize()
 *
 *    FPixa accessors
 *          l_int32        fpixaGetCount()
 *          l_int32        fpixaChangeRefcount()
 *          FPIX          *fpixaGetFPix()
 *          l_int32        fpixaGetFPixDimensions()
 *          l_float32     *fpixaGetData()
 *          l_int32        fpixaGetPixel()
 *          l_int32        fpixaSetPixel()
 *
 *    DPix Create/copy/destroy
 *          DPIX          *dpixCreate()
 *          DPIX          *dpixCreateTemplate()
 *          DPIX          *dpixClone()
 *          DPIX          *dpixCopy()
 *          l_int32        dpixResizeImageData()
 *          void           dpixDestroy()
 *
 *    DPix accessors
 *          l_int32        dpixGetDimensions()
 *          l_int32        dpixSetDimensions()
 *          l_int32        dpixGetWpl()
 *          l_int32        dpixSetWpl()
 *          l_int32        dpixGetRefcount()
 *          l_int32        dpixChangeRefcount()
 *          l_int32        dpixGetResolution()
 *          l_int32        dpixSetResolution()
 *          l_int32        dpixCopyResolution()
 *          l_float64     *dpixGetData()
 *          l_int32        dpixSetData()
 *          l_int32        dpixGetPixel()
 *          l_int32        dpixSetPixel()
 *
 *    FPix serialized I/O
 *          FPIX          *fpixRead()
 *          FPIX          *fpixReadStream()
 *          l_int32        fpixWrite()
 *          l_int32        fpixWriteStream()
 *          FPIX          *fpixEndianByteSwap()
 *
 *    DPix serialized I/O
 *          DPIX          *dpixRead()
 *          DPIX          *dpixReadStream()
 *          l_int32        dpixWrite()
 *          l_int32        dpixWriteStream()
 *          DPIX          *dpixEndianByteSwap()
 *
 *    Print FPix (subsampled, for debugging)
 *          l_int32        fpixPrintStream()
 */

#include <string.h>
#include "allheaders.h"

static const l_int32  INITIAL_PTR_ARRAYSIZE = 20;   /* must be > 0 */

    /* Static functions */
static l_int32 fpixaExtendArray(FPIXA *fpixa);
static l_int32 fpixaExtendArrayToSize(FPIXA *fpixa, l_int32 size);


/*--------------------------------------------------------------------*
 *                     FPix Create/copy/destroy                       *
 *--------------------------------------------------------------------*/
/*!
 *  fpixCreate()
 *
 *      Input:  width, height
 *      Return: fpixd (with data allocated and initialized to 0),
 *                     or null on error
 *
 *  Notes:
 *      (1) Makes a FPix of specified size, with the data array
 *          allocated and initialized to 0.
 */
FPIX *
fpixCreate(l_int32  width,
           l_int32  height)
{
l_float32  *data;
l_uint64    bignum;
FPIX       *fpixd;

    PROCNAME("fpixCreate");

    if (width <= 0)
        return (FPIX *)ERROR_PTR("width must be > 0", procName, NULL);
    if (height <= 0)
        return (FPIX *)ERROR_PTR("height must be > 0", procName, NULL);

        /* Avoid overflow in malloc arg, malicious or otherwise */
    bignum = 4L * width * height;   /* max number of bytes requested */
    if (bignum > ((1LL << 31) - 1)) {
        L_ERROR("requested w = %d, h = %d\n", procName, width, height);
        return (FPIX *)ERROR_PTR("requested bytes >= 2^31", procName, NULL);
    }

    if ((fpixd = (FPIX *)CALLOC(1, sizeof(FPIX))) == NULL)
        return (FPIX *)ERROR_PTR("CALLOC fail for fpixd", procName, NULL);
    fpixSetDimensions(fpixd, width, height);
    fpixSetWpl(fpixd, width);  /* 4-byte words */
    fpixd->refcount = 1;

    data = (l_float32 *)CALLOC(width * height, sizeof(l_float32));
    if (!data)
        return (FPIX *)ERROR_PTR("CALLOC fail for data", procName, NULL);
    fpixSetData(fpixd, data);

    return fpixd;
}


/*!
 *  fpixCreateTemplate()
 *
 *      Input:  fpixs
 *      Return: fpixd, or null on error
 *
 *  Notes:
 *      (1) Makes a FPix of the same size as the input FPix, with the
 *          data array allocated and initialized to 0.
 *      (2) Copies the resolution.
 */
FPIX *
fpixCreateTemplate(FPIX  *fpixs)
{
l_int32  w, h;
FPIX    *fpixd;

    PROCNAME("fpixCreateTemplate");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);

    fpixGetDimensions(fpixs, &w, &h);
    fpixd = fpixCreate(w, h);
    fpixCopyResolution(fpixd, fpixs);
    return fpixd;
}


/*!
 *  fpixClone()
 *
 *      Input:  fpix
 *      Return: same fpix (ptr), or null on error
 *
 *  Notes:
 *      (1) See pixClone() for definition and usage.
 */
FPIX *
fpixClone(FPIX  *fpix)
{
    PROCNAME("fpixClone");

    if (!fpix)
        return (FPIX *)ERROR_PTR("fpix not defined", procName, NULL);
    fpixChangeRefcount(fpix, 1);

    return fpix;
}


/*!
 *  fpixCopy()
 *
 *      Input:  fpixd (<optional>; can be null, or equal to fpixs,
 *                    or different from fpixs)
 *              fpixs
 *      Return: fpixd, or null on error
 *
 *  Notes:
 *      (1) There are three cases:
 *            (a) fpixd == null  (makes a new fpix; refcount = 1)
 *            (b) fpixd == fpixs  (no-op)
 *            (c) fpixd != fpixs  (data copy; no change in refcount)
 *          If the refcount of fpixd > 1, case (c) will side-effect
 *          these handles.
 *      (2) The general pattern of use is:
 *             fpixd = fpixCopy(fpixd, fpixs);
 *          This will work for all three cases.
 *          For clarity when the case is known, you can use:
 *            (a) fpixd = fpixCopy(NULL, fpixs);
 *            (c) fpixCopy(fpixd, fpixs);
 *      (3) For case (c), we check if fpixs and fpixd are the same size.
 *          If so, the data is copied directly.
 *          Otherwise, the data is reallocated to the correct size
 *          and the copy proceeds.  The refcount of fpixd is unchanged.
 *      (4) This operation, like all others that may involve a pre-existing
 *          fpixd, will side-effect any existing clones of fpixd.
 */
FPIX *
fpixCopy(FPIX  *fpixd,   /* can be null */
         FPIX  *fpixs)
{
l_int32     w, h, bytes;
l_float32  *datas, *datad;

    PROCNAME("fpixCopy");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);
    if (fpixs == fpixd)
        return fpixd;

        /* Total bytes in image data */
    fpixGetDimensions(fpixs, &w, &h);
    bytes = 4 * w * h;

        /* If we're making a new fpix ... */
    if (!fpixd) {
        if ((fpixd = fpixCreateTemplate(fpixs)) == NULL)
            return (FPIX *)ERROR_PTR("fpixd not made", procName, NULL);
        datas = fpixGetData(fpixs);
        datad = fpixGetData(fpixd);
        memcpy((char *)datad, (char *)datas, bytes);
        return fpixd;
    }

        /* Reallocate image data if sizes are different */
    fpixResizeImageData(fpixd, fpixs);

        /* Copy data */
    fpixCopyResolution(fpixd, fpixs);
    datas = fpixGetData(fpixs);
    datad = fpixGetData(fpixd);
    memcpy((char*)datad, (char*)datas, bytes);
    return fpixd;
}


/*!
 *  fpixResizeImageData()
 *
 *      Input:  fpixd, fpixs
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If the data sizes differ, this destroys the existing
 *          data in fpixd and allocates a new, uninitialized, data array
 *          of the same size as the data in fpixs.  Otherwise, this
 *          doesn't do anything.
 */
l_int32
fpixResizeImageData(FPIX  *fpixd,
                    FPIX  *fpixs)
{
l_int32     ws, hs, wd, hd, bytes;
l_float32  *data;

    PROCNAME("fpixResizeImageData");

    if (!fpixs)
        return ERROR_INT("fpixs not defined", procName, 1);
    if (!fpixd)
        return ERROR_INT("fpixd not defined", procName, 1);

    fpixGetDimensions(fpixs, &ws, &hs);
    fpixGetDimensions(fpixd, &wd, &hd);
    if (ws == wd && hs == hd)  /* nothing to do */
        return 0;

    fpixSetDimensions(fpixd, ws, hs);
    fpixSetWpl(fpixd, ws);
    bytes = 4 * ws * hs;
    data = fpixGetData(fpixd);
    if (data) FREE(data);
    if ((data = (l_float32 *)MALLOC(bytes)) == NULL)
        return ERROR_INT("MALLOC fail for data", procName, 1);
    fpixSetData(fpixd, data);
    return 0;
}


/*!
 *  fpixDestroy()
 *
 *      Input:  &fpix <will be nulled>
 *      Return: void
 *
 *  Notes:
 *      (1) Decrements the ref count and, if 0, destroys the fpix.
 *      (2) Always nulls the input ptr.
 */
void
fpixDestroy(FPIX  **pfpix)
{
l_float32  *data;
FPIX       *fpix;

    PROCNAME("fpixDestroy");

    if (!pfpix) {
        L_WARNING("ptr address is null!\n", procName);
        return;
    }

    if ((fpix = *pfpix) == NULL)
        return;

        /* Decrement the ref count.  If it is 0, destroy the fpix. */
    fpixChangeRefcount(fpix, -1);
    if (fpixGetRefcount(fpix) <= 0) {
        if ((data = fpixGetData(fpix)) != NULL)
            FREE(data);
        FREE(fpix);
    }

    *pfpix = NULL;
    return;
}


/*--------------------------------------------------------------------*
 *                          FPix  Accessors                           *
 *--------------------------------------------------------------------*/
/*!
 *  fpixGetDimensions()
 *
 *      Input:  fpix
 *              &w, &h (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
fpixGetDimensions(FPIX     *fpix,
                  l_int32  *pw,
                  l_int32  *ph)
{
    PROCNAME("fpixGetDimensions");

    if (!pw && !ph)
        return ERROR_INT("no return val requested", procName, 1);
    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (!fpix)
        return ERROR_INT("fpix not defined", procName, 1);
    if (pw) *pw = fpix->w;
    if (ph) *ph = fpix->h;
    return 0;
}


/*!
 *  fpixSetDimensions()
 *
 *      Input:  fpix
 *              w, h
 *      Return: 0 if OK, 1 on error
 */
l_int32
fpixSetDimensions(FPIX     *fpix,
                  l_int32   w,
                  l_int32   h)
{
    PROCNAME("fpixSetDimensions");

    if (!fpix)
        return ERROR_INT("fpix not defined", procName, 1);
    fpix->w = w;
    fpix->h = h;
    return 0;
}


l_int32
fpixGetWpl(FPIX  *fpix)
{
    PROCNAME("fpixGetWpl");

    if (!fpix)
        return ERROR_INT("fpix not defined", procName, UNDEF);
    return fpix->wpl;
}


l_int32
fpixSetWpl(FPIX    *fpix,
           l_int32  wpl)
{
    PROCNAME("fpixSetWpl");

    if (!fpix)
        return ERROR_INT("fpix not defined", procName, 1);

    fpix->wpl = wpl;
    return 0;
}


l_int32
fpixGetRefcount(FPIX  *fpix)
{
    PROCNAME("fpixGetRefcount");

    if (!fpix)
        return ERROR_INT("fpix not defined", procName, UNDEF);
    return fpix->refcount;
}


l_int32
fpixChangeRefcount(FPIX    *fpix,
                   l_int32  delta)
{
    PROCNAME("fpixChangeRefcount");

    if (!fpix)
        return ERROR_INT("fpix not defined", procName, 1);

    fpix->refcount += delta;
    return 0;
}


l_int32
fpixGetResolution(FPIX     *fpix,
                  l_int32  *pxres,
                  l_int32  *pyres)
{
    PROCNAME("fpixGetResolution");

    if (!fpix)
        return ERROR_INT("fpix not defined", procName, 1);
    if (pxres) *pxres = fpix->xres;
    if (pyres) *pyres = fpix->yres;
    return 0;
}


l_int32
fpixSetResolution(FPIX    *fpix,
                  l_int32  xres,
                  l_int32  yres)
{
    PROCNAME("fpixSetResolution");

    if (!fpix)
        return ERROR_INT("fpix not defined", procName, 1);

    fpix->xres = xres;
    fpix->yres = yres;
    return 0;
}


l_int32
fpixCopyResolution(FPIX  *fpixd,
                   FPIX  *fpixs)
{
l_int32  xres, yres;
    PROCNAME("fpixCopyResolution");

    if (!fpixs || !fpixd)
        return ERROR_INT("fpixs and fpixd not both defined", procName, 1);

    fpixGetResolution(fpixs, &xres, &yres);
    fpixSetResolution(fpixd, xres, yres);
    return 0;
}


l_float32 *
fpixGetData(FPIX  *fpix)
{
    PROCNAME("fpixGetData");

    if (!fpix)
        return (l_float32 *)ERROR_PTR("fpix not defined", procName, NULL);
    return fpix->data;
}


l_int32
fpixSetData(FPIX       *fpix,
            l_float32  *data)
{
    PROCNAME("fpixSetData");

    if (!fpix)
        return ERROR_INT("fpix not defined", procName, 1);

    fpix->data = data;
    return 0;
}


/*!
 *  fpixGetPixel()
 *
 *      Input:  fpix
 *              (x,y) pixel coords
 *              &val (<return> pixel value)
 *      Return: 0 if OK; 1 on error
 */
l_int32
fpixGetPixel(FPIX       *fpix,
             l_int32     x,
             l_int32     y,
             l_float32  *pval)
{
l_int32  w, h;

    PROCNAME("fpixGetPixel");

    if (!pval)
        return ERROR_INT("pval not defined", procName, 1);
    *pval = 0.0;
    if (!fpix)
        return ERROR_INT("fpix not defined", procName, 1);

    fpixGetDimensions(fpix, &w, &h);
    if (x < 0 || x >= w)
        return ERROR_INT("x out of bounds", procName, 1);
    if (y < 0 || y >= h)
        return ERROR_INT("y out of bounds", procName, 1);

    *pval = *(fpix->data + y * w + x);
    return 0;
}


/*!
 *  fpixSetPixel()
 *
 *      Input:  fpix
 *              (x,y) pixel coords
 *              val (pixel value)
 *      Return: 0 if OK; 1 on error
 */
l_int32
fpixSetPixel(FPIX      *fpix,
             l_int32    x,
             l_int32    y,
             l_float32  val)
{
l_int32  w, h;

    PROCNAME("fpixSetPixel");

    if (!fpix)
        return ERROR_INT("fpix not defined", procName, 1);

    fpixGetDimensions(fpix, &w, &h);
    if (x < 0 || x >= w)
        return ERROR_INT("x out of bounds", procName, 1);
    if (y < 0 || y >= h)
        return ERROR_INT("y out of bounds", procName, 1);

    *(fpix->data + y * w + x) = val;
    return 0;
}


/*--------------------------------------------------------------------*
 *                     FPixa Create/copy/destroy                      *
 *--------------------------------------------------------------------*/
/*!
 *  fpixaCreate()
 *
 *      Input:  n  (initial number of ptrs)
 *      Return: fpixa, or null on error
 */
FPIXA *
fpixaCreate(l_int32  n)
{
FPIXA  *fpixa;

    PROCNAME("fpixaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((fpixa = (FPIXA *)CALLOC(1, sizeof(FPIXA))) == NULL)
        return (FPIXA *)ERROR_PTR("pixa not made", procName, NULL);
    fpixa->n = 0;
    fpixa->nalloc = n;
    fpixa->refcount = 1;

    if ((fpixa->fpix = (FPIX **)CALLOC(n, sizeof(FPIX *))) == NULL)
        return (FPIXA *)ERROR_PTR("fpix ptrs not made", procName, NULL);

    return fpixa;
}


/*!
 *  fpixaCopy()
 *
 *      Input:  fpixas
 *              copyflag:
 *                L_COPY makes a new fpixa and copies each fpix
 *                L_CLONE gives a new ref-counted handle to the input fpixa
 *                L_COPY_CLONE makes a new fpixa with clones of all fpix
 *      Return: new fpixa, or null on error
 */
FPIXA *
fpixaCopy(FPIXA   *fpixa,
          l_int32  copyflag)
{
l_int32  i;
FPIX    *fpixc;
FPIXA   *fpixac;

    PROCNAME("fpixaCopy");

    if (!fpixa)
        return (FPIXA *)ERROR_PTR("fpixa not defined", procName, NULL);

    if (copyflag == L_CLONE) {
        fpixaChangeRefcount(fpixa, 1);
        return fpixa;
    }

    if (copyflag != L_COPY && copyflag != L_COPY_CLONE)
        return (FPIXA *)ERROR_PTR("invalid copyflag", procName, NULL);

    if ((fpixac = fpixaCreate(fpixa->n)) == NULL)
        return (FPIXA *)ERROR_PTR("fpixac not made", procName, NULL);
    for (i = 0; i < fpixa->n; i++) {
        if (copyflag == L_COPY)
            fpixc = fpixaGetFPix(fpixa, i, L_COPY);
        else  /* copy-clone */
            fpixc = fpixaGetFPix(fpixa, i, L_CLONE);
        fpixaAddFPix(fpixac, fpixc, L_INSERT);
    }

    return fpixac;
}


/*!
 *  fpixaDestroy()
 *
 *      Input:  &fpixa (<can be nulled>)
 *      Return: void
 *
 *  Notes:
 *      (1) Decrements the ref count and, if 0, destroys the fpixa.
 *      (2) Always nulls the input ptr.
 */
void
fpixaDestroy(FPIXA  **pfpixa)
{
l_int32  i;
FPIXA   *fpixa;

    PROCNAME("fpixaDestroy");

    if (pfpixa == NULL) {
        L_WARNING("ptr address is NULL!\n", procName);
        return;
    }

    if ((fpixa = *pfpixa) == NULL)
        return;

        /* Decrement the refcount.  If it is 0, destroy the pixa. */
    fpixaChangeRefcount(fpixa, -1);
    if (fpixa->refcount <= 0) {
        for (i = 0; i < fpixa->n; i++)
            fpixDestroy(&fpixa->fpix[i]);
        FREE(fpixa->fpix);
        FREE(fpixa);
    }

    *pfpixa = NULL;
    return;
}


/*--------------------------------------------------------------------*
 *                           FPixa addition                           *
 *--------------------------------------------------------------------*/
/*!
 *  fpixaAddFPix()
 *
 *      Input:  fpixa
 *              fpix  (to be added)
 *              copyflag (L_INSERT, L_COPY, L_CLONE)
 *      Return: 0 if OK; 1 on error
 */
l_int32
fpixaAddFPix(FPIXA   *fpixa,
             FPIX    *fpix,
             l_int32  copyflag)
{
l_int32  n;
FPIX    *fpixc;

    PROCNAME("fpixaAddFPix");

    if (!fpixa)
        return ERROR_INT("fpixa not defined", procName, 1);
    if (!fpix)
        return ERROR_INT("fpix not defined", procName, 1);

    if (copyflag == L_INSERT)
        fpixc = fpix;
    else if (copyflag == L_COPY)
        fpixc = fpixCopy(NULL, fpix);
    else if (copyflag == L_CLONE)
        fpixc = fpixClone(fpix);
    else
        return ERROR_INT("invalid copyflag", procName, 1);
    if (!fpixc)
        return ERROR_INT("fpixc not made", procName, 1);

    n = fpixaGetCount(fpixa);
    if (n >= fpixa->nalloc)
        fpixaExtendArray(fpixa);
    fpixa->fpix[n] = fpixc;
    fpixa->n++;

    return 0;
}


/*!
 *  fpixaExtendArray()
 *
 *      Input:  fpixa
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Doubles the size of the fpixa ptr array.
 */
static l_int32
fpixaExtendArray(FPIXA  *fpixa)
{
    PROCNAME("fpixaExtendArray");

    if (!fpixa)
        return ERROR_INT("fpixa not defined", procName, 1);

    return fpixaExtendArrayToSize(fpixa, 2 * fpixa->nalloc);
}


/*!
 *  fpixaExtendArrayToSize()
 *
 *      Input:  fpixa
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) If necessary, reallocs new fpixa ptrs array to @size.
 */
static l_int32
fpixaExtendArrayToSize(FPIXA   *fpixa,
                       l_int32  size)
{
    PROCNAME("fpixaExtendArrayToSize");

    if (!fpixa)
        return ERROR_INT("fpixa not defined", procName, 1);

    if (size > fpixa->nalloc) {
        if ((fpixa->fpix = (FPIX **)reallocNew((void **)&fpixa->fpix,
                                 sizeof(FPIX *) * fpixa->nalloc,
                                 size * sizeof(FPIX *))) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);
        fpixa->nalloc = size;
    }
    return 0;
}


/*--------------------------------------------------------------------*
 *                          FPixa accessors                           *
 *--------------------------------------------------------------------*/
/*!
 *  fpixaGetCount()
 *
 *      Input:  fpixa
 *      Return: count, or 0 if no pixa
 */
l_int32
fpixaGetCount(FPIXA  *fpixa)
{
    PROCNAME("fpixaGetCount");

    if (!fpixa)
        return ERROR_INT("fpixa not defined", procName, 0);

    return fpixa->n;
}


/*!
 *  fpixaChangeRefcount()
 *
 *      Input:  fpixa
 *      Return: 0 if OK, 1 on error
 */
l_int32
fpixaChangeRefcount(FPIXA   *fpixa,
                    l_int32  delta)
{
    PROCNAME("fpixaChangeRefcount");

    if (!fpixa)
        return ERROR_INT("fpixa not defined", procName, 1);

    fpixa->refcount += delta;
    return 0;
}


/*!
 *  fpixaGetFPix()
 *
 *      Input:  fpixa
 *              index  (to the index-th fpix)
 *              accesstype  (L_COPY or L_CLONE)
 *      Return: fpix, or null on error
 */
FPIX *
fpixaGetFPix(FPIXA   *fpixa,
             l_int32  index,
             l_int32  accesstype)
{
    PROCNAME("fpixaGetFPix");

    if (!fpixa)
        return (FPIX *)ERROR_PTR("fpixa not defined", procName, NULL);
    if (index < 0 || index >= fpixa->n)
        return (FPIX *)ERROR_PTR("index not valid", procName, NULL);

    if (accesstype == L_COPY)
        return fpixCopy(NULL, fpixa->fpix[index]);
    else if (accesstype == L_CLONE)
        return fpixClone(fpixa->fpix[index]);
    else
        return (FPIX *)ERROR_PTR("invalid accesstype", procName, NULL);
}


/*!
 *  fpixaGetFPixDimensions()
 *
 *      Input:  fpixa
 *              index  (to the index-th box)
 *              &w, &h (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
fpixaGetFPixDimensions(FPIXA    *fpixa,
                       l_int32   index,
                       l_int32  *pw,
                       l_int32  *ph)
{
FPIX  *fpix;

    PROCNAME("fpixaGetFPixDimensions");

    if (!pw && !ph)
        return ERROR_INT("no return val requested", procName, 1);
    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (!fpixa)
        return ERROR_INT("fpixa not defined", procName, 1);
    if (index < 0 || index >= fpixa->n)
        return ERROR_INT("index not valid", procName, 1);

    if ((fpix = fpixaGetFPix(fpixa, index, L_CLONE)) == NULL)
        return ERROR_INT("fpix not found!", procName, 1);
    fpixGetDimensions(fpix, pw, ph);
    fpixDestroy(&fpix);
    return 0;
}


/*!
 *  fpixaGetData()
 *
 *      Input:  fpixa
 *              index (into fpixa array)
 *      Return: data (not a copy), or null on error
 */
l_float32 *
fpixaGetData(FPIXA      *fpixa,
             l_int32     index)
{
l_int32     n;
l_float32  *data;
FPIX       *fpix;

    PROCNAME("fpixaGetData");

    if (!fpixa)
        return (l_float32 *)ERROR_PTR("fpixa not defined", procName, NULL);
    n = fpixaGetCount(fpixa);
    if (index < 0 || index >= n)
        return (l_float32 *)ERROR_PTR("invalid index", procName, NULL);

    fpix = fpixaGetFPix(fpixa, index, L_CLONE);
    data = fpixGetData(fpix);
    fpixDestroy(&fpix);
    return data;
}


/*!
 *  fpixaGetPixel()
 *
 *      Input:  fpixa
 *              index (into fpixa array)
 *              (x,y) pixel coords
 *              &val (<return> pixel value)
 *      Return: 0 if OK; 1 on error
 */
l_int32
fpixaGetPixel(FPIXA      *fpixa,
              l_int32     index,
              l_int32     x,
              l_int32     y,
              l_float32  *pval)
{
l_int32  n, ret;
FPIX    *fpix;

    PROCNAME("fpixaGetPixel");

    if (!pval)
        return ERROR_INT("pval not defined", procName, 1);
    *pval = 0.0;
    if (!fpixa)
        return ERROR_INT("fpixa not defined", procName, 1);
    n = fpixaGetCount(fpixa);
    if (index < 0 || index >= n)
        return ERROR_INT("invalid index into fpixa", procName, 1);

    fpix = fpixaGetFPix(fpixa, index, L_CLONE);
    ret = fpixGetPixel(fpix, x, y, pval);
    fpixDestroy(&fpix);
    return ret;
}


/*!
 *  fpixaSetPixel()
 *
 *      Input:  fpixa
 *              index (into fpixa array)
 *              (x,y) pixel coords
 *              val (pixel value)
 *      Return: 0 if OK; 1 on error
 */
l_int32
fpixaSetPixel(FPIXA     *fpixa,
              l_int32    index,
              l_int32    x,
              l_int32    y,
              l_float32  val)
{
l_int32  n, ret;
FPIX    *fpix;

    PROCNAME("fpixaSetPixel");

    if (!fpixa)
        return ERROR_INT("fpixa not defined", procName, 1);
    n = fpixaGetCount(fpixa);
    if (index < 0 || index >= n)
        return ERROR_INT("invalid index into fpixa", procName, 1);

    fpix = fpixaGetFPix(fpixa, index, L_CLONE);
    ret = fpixSetPixel(fpix, x, y, val);
    fpixDestroy(&fpix);
    return ret;
}


/*--------------------------------------------------------------------*
 *                     DPix Create/copy/destroy                       *
 *--------------------------------------------------------------------*/
/*!
 *  dpixCreate()
 *
 *      Input:  width, height
 *      Return: dpix (with data allocated and initialized to 0),
 *                     or null on error
 *
 *  Notes:
 *      (1) Makes a DPix of specified size, with the data array
 *          allocated and initialized to 0.
 */
DPIX *
dpixCreate(l_int32  width,
           l_int32  height)
{
l_float64  *data;
l_uint64    bignum;
DPIX       *dpix;

    PROCNAME("dpixCreate");

    if (width <= 0)
        return (DPIX *)ERROR_PTR("width must be > 0", procName, NULL);
    if (height <= 0)
        return (DPIX *)ERROR_PTR("height must be > 0", procName, NULL);

        /* Avoid overflow in malloc arg, malicious or otherwise */
    bignum = 8L * width * height;   /* max number of bytes requested */
    if (bignum > ((1LL << 31) - 1)) {
        L_ERROR("requested w = %d, h = %d\n", procName, width, height);
        return (DPIX *)ERROR_PTR("requested bytes >= 2^31", procName, NULL);
    }

    if ((dpix = (DPIX *)CALLOC(1, sizeof(DPIX))) == NULL)
        return (DPIX *)ERROR_PTR("CALLOC fail for dpix", procName, NULL);
    dpixSetDimensions(dpix, width, height);
    dpixSetWpl(dpix, width);  /* 8 byte words */
    dpix->refcount = 1;

    data = (l_float64 *)CALLOC(width * height, sizeof(l_float64));
    if (!data)
        return (DPIX *)ERROR_PTR("CALLOC fail for data", procName, NULL);
    dpixSetData(dpix, data);

    return dpix;
}


/*!
 *  dpixCreateTemplate()
 *
 *      Input:  dpixs
 *      Return: dpixd, or null on error
 *
 *  Notes:
 *      (1) Makes a DPix of the same size as the input DPix, with the
 *          data array allocated and initialized to 0.
 *      (2) Copies the resolution.
 */
DPIX *
dpixCreateTemplate(DPIX  *dpixs)
{
l_int32  w, h;
DPIX    *dpixd;

    PROCNAME("dpixCreateTemplate");

    if (!dpixs)
        return (DPIX *)ERROR_PTR("dpixs not defined", procName, NULL);

    dpixGetDimensions(dpixs, &w, &h);
    dpixd = dpixCreate(w, h);
    dpixCopyResolution(dpixd, dpixs);
    return dpixd;
}


/*!
 *  dpixClone()
 *
 *      Input:  dpix
 *      Return: same dpix (ptr), or null on error
 *
 *  Notes:
 *      (1) See pixClone() for definition and usage.
 */
DPIX *
dpixClone(DPIX  *dpix)
{
    PROCNAME("dpixClone");

    if (!dpix)
        return (DPIX *)ERROR_PTR("dpix not defined", procName, NULL);
    dpixChangeRefcount(dpix, 1);

    return dpix;
}


/*!
 *  dpixCopy()
 *
 *      Input:  dpixd (<optional>; can be null, or equal to dpixs,
 *                    or different from dpixs)
 *              dpixs
 *      Return: dpixd, or null on error
 *
 *  Notes:
 *      (1) There are three cases:
 *            (a) dpixd == null  (makes a new dpix; refcount = 1)
 *            (b) dpixd == dpixs  (no-op)
 *            (c) dpixd != dpixs  (data copy; no change in refcount)
 *          If the refcount of dpixd > 1, case (c) will side-effect
 *          these handles.
 *      (2) The general pattern of use is:
 *             dpixd = dpixCopy(dpixd, dpixs);
 *          This will work for all three cases.
 *          For clarity when the case is known, you can use:
 *            (a) dpixd = dpixCopy(NULL, dpixs);
 *            (c) dpixCopy(dpixd, dpixs);
 *      (3) For case (c), we check if dpixs and dpixd are the same size.
 *          If so, the data is copied directly.
 *          Otherwise, the data is reallocated to the correct size
 *          and the copy proceeds.  The refcount of dpixd is unchanged.
 *      (4) This operation, like all others that may involve a pre-existing
 *          dpixd, will side-effect any existing clones of dpixd.
 */
DPIX *
dpixCopy(DPIX  *dpixd,   /* can be null */
         DPIX  *dpixs)
{
l_int32     w, h, bytes;
l_float64  *datas, *datad;

    PROCNAME("dpixCopy");

    if (!dpixs)
        return (DPIX *)ERROR_PTR("dpixs not defined", procName, NULL);
    if (dpixs == dpixd)
        return dpixd;

        /* Total bytes in image data */
    dpixGetDimensions(dpixs, &w, &h);
    bytes = 8 * w * h;

        /* If we're making a new dpix ... */
    if (!dpixd) {
        if ((dpixd = dpixCreateTemplate(dpixs)) == NULL)
            return (DPIX *)ERROR_PTR("dpixd not made", procName, NULL);
        datas = dpixGetData(dpixs);
        datad = dpixGetData(dpixd);
        memcpy((char *)datad, (char *)datas, bytes);
        return dpixd;
    }

        /* Reallocate image data if sizes are different */
    dpixResizeImageData(dpixd, dpixs);

        /* Copy data */
    dpixCopyResolution(dpixd, dpixs);
    datas = dpixGetData(dpixs);
    datad = dpixGetData(dpixd);
    memcpy((char*)datad, (char*)datas, bytes);
    return dpixd;
}


/*!
 *  dpixResizeImageData()
 *
 *      Input:  dpixd, dpixs
 *      Return: 0 if OK, 1 on error
 */
l_int32
dpixResizeImageData(DPIX  *dpixd,
                    DPIX  *dpixs)
{
l_int32     ws, hs, wd, hd, bytes;
l_float64  *data;

    PROCNAME("dpixResizeImageData");

    if (!dpixs)
        return ERROR_INT("dpixs not defined", procName, 1);
    if (!dpixd)
        return ERROR_INT("dpixd not defined", procName, 1);

    dpixGetDimensions(dpixs, &ws, &hs);
    dpixGetDimensions(dpixd, &wd, &hd);
    if (ws == wd && hs == hd)  /* nothing to do */
        return 0;

    dpixSetDimensions(dpixd, ws, hs);
    dpixSetWpl(dpixd, ws);  /* 8 byte words */
    bytes = 8 * ws * hs;
    data = dpixGetData(dpixd);
    if (data) FREE(data);
    if ((data = (l_float64 *)MALLOC(bytes)) == NULL)
        return ERROR_INT("MALLOC fail for data", procName, 1);
    dpixSetData(dpixd, data);
    return 0;
}


/*!
 *  dpixDestroy()
 *
 *      Input:  &dpix <will be nulled>
 *      Return: void
 *
 *  Notes:
 *      (1) Decrements the ref count and, if 0, destroys the dpix.
 *      (2) Always nulls the input ptr.
 */
void
dpixDestroy(DPIX  **pdpix)
{
l_float64  *data;
DPIX       *dpix;

    PROCNAME("dpixDestroy");

    if (!pdpix) {
        L_WARNING("ptr address is null!\n", procName);
        return;
    }

    if ((dpix = *pdpix) == NULL)
        return;

        /* Decrement the ref count.  If it is 0, destroy the dpix. */
    dpixChangeRefcount(dpix, -1);
    if (dpixGetRefcount(dpix) <= 0) {
        if ((data = dpixGetData(dpix)) != NULL)
            FREE(data);
        FREE(dpix);
    }

    *pdpix = NULL;
    return;
}


/*--------------------------------------------------------------------*
 *                          DPix  Accessors                           *
 *--------------------------------------------------------------------*/
/*!
 *  dpixGetDimensions()
 *
 *      Input:  dpix
 *              &w, &h (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
dpixGetDimensions(DPIX     *dpix,
                  l_int32  *pw,
                  l_int32  *ph)
{
    PROCNAME("dpixGetDimensions");

    if (!pw && !ph)
        return ERROR_INT("no return val requested", procName, 1);
    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (!dpix)
        return ERROR_INT("dpix not defined", procName, 1);
    if (pw) *pw = dpix->w;
    if (ph) *ph = dpix->h;
    return 0;
}


/*!
 *  dpixSetDimensions()
 *
 *      Input:  dpix
 *              w, h
 *      Return: 0 if OK, 1 on error
 */
l_int32
dpixSetDimensions(DPIX     *dpix,
                  l_int32   w,
                  l_int32   h)
{
    PROCNAME("dpixSetDimensions");

    if (!dpix)
        return ERROR_INT("dpix not defined", procName, 1);
    dpix->w = w;
    dpix->h = h;
    return 0;
}


l_int32
dpixGetWpl(DPIX  *dpix)
{
    PROCNAME("dpixGetWpl");

    if (!dpix)
        return ERROR_INT("dpix not defined", procName, 1);
    return dpix->wpl;
}


l_int32
dpixSetWpl(DPIX    *dpix,
           l_int32  wpl)
{
    PROCNAME("dpixSetWpl");

    if (!dpix)
        return ERROR_INT("dpix not defined", procName, 1);

    dpix->wpl = wpl;
    return 0;
}


l_int32
dpixGetRefcount(DPIX  *dpix)
{
    PROCNAME("dpixGetRefcount");

    if (!dpix)
        return ERROR_INT("dpix not defined", procName, UNDEF);
    return dpix->refcount;
}


l_int32
dpixChangeRefcount(DPIX    *dpix,
                   l_int32  delta)
{
    PROCNAME("dpixChangeRefcount");

    if (!dpix)
        return ERROR_INT("dpix not defined", procName, 1);

    dpix->refcount += delta;
    return 0;
}


l_int32
dpixGetResolution(DPIX     *dpix,
                  l_int32  *pxres,
                  l_int32  *pyres)
{
    PROCNAME("dpixGetResolution");

    if (!dpix)
        return ERROR_INT("dpix not defined", procName, 1);
    if (pxres) *pxres = dpix->xres;
    if (pyres) *pyres = dpix->yres;
    return 0;
}


l_int32
dpixSetResolution(DPIX    *dpix,
                  l_int32  xres,
                  l_int32  yres)
{
    PROCNAME("dpixSetResolution");

    if (!dpix)
        return ERROR_INT("dpix not defined", procName, 1);

    dpix->xres = xres;
    dpix->yres = yres;
    return 0;
}


l_int32
dpixCopyResolution(DPIX  *dpixd,
                   DPIX  *dpixs)
{
l_int32  xres, yres;
    PROCNAME("dpixCopyResolution");

    if (!dpixs || !dpixd)
        return ERROR_INT("dpixs and dpixd not both defined", procName, 1);

    dpixGetResolution(dpixs, &xres, &yres);
    dpixSetResolution(dpixd, xres, yres);
    return 0;
}


l_float64 *
dpixGetData(DPIX  *dpix)
{
    PROCNAME("dpixGetData");

    if (!dpix)
        return (l_float64 *)ERROR_PTR("dpix not defined", procName, NULL);
    return dpix->data;
}


l_int32
dpixSetData(DPIX       *dpix,
            l_float64  *data)
{
    PROCNAME("dpixSetData");

    if (!dpix)
        return ERROR_INT("dpix not defined", procName, 1);

    dpix->data = data;
    return 0;
}


/*!
 *  dpixGetPixel()
 *
 *      Input:  dpix
 *              (x,y) pixel coords
 *              &val (<return> pixel value)
 *      Return: 0 if OK; 1 on error
 */
l_int32
dpixGetPixel(DPIX       *dpix,
             l_int32     x,
             l_int32     y,
             l_float64  *pval)
{
l_int32  w, h;

    PROCNAME("dpixGetPixel");

    if (!pval)
        return ERROR_INT("pval not defined", procName, 1);
    *pval = 0.0;
    if (!dpix)
        return ERROR_INT("dpix not defined", procName, 1);

    dpixGetDimensions(dpix, &w, &h);
    if (x < 0 || x >= w)
        return ERROR_INT("x out of bounds", procName, 1);
    if (y < 0 || y >= h)
        return ERROR_INT("y out of bounds", procName, 1);

    *pval = *(dpix->data + y * w + x);
    return 0;
}


/*!
 *  dpixSetPixel()
 *
 *      Input:  dpix
 *              (x,y) pixel coords
 *              val (pixel value)
 *      Return: 0 if OK; 1 on error
 */
l_int32
dpixSetPixel(DPIX      *dpix,
             l_int32    x,
             l_int32    y,
             l_float64  val)
{
l_int32  w, h;

    PROCNAME("dpixSetPixel");

    if (!dpix)
        return ERROR_INT("dpix not defined", procName, 1);

    dpixGetDimensions(dpix, &w, &h);
    if (x < 0 || x >= w)
        return ERROR_INT("x out of bounds", procName, 1);
    if (y < 0 || y >= h)
        return ERROR_INT("y out of bounds", procName, 1);

    *(dpix->data + y * w + x) = val;
    return 0;
}


/*--------------------------------------------------------------------*
 *                       FPix serialized I/O                          *
 *--------------------------------------------------------------------*/
/*!
 *  fpixRead()
 *
 *      Input:  filename
 *      Return: fpix, or null on error
 */
FPIX *
fpixRead(const char  *filename)
{
FILE  *fp;
FPIX  *fpix;

    PROCNAME("fpixRead");

    if (!filename)
        return (FPIX *)ERROR_PTR("filename not defined", procName, NULL);
    if ((fp = fopenReadStream(filename)) == NULL)
        return (FPIX *)ERROR_PTR("stream not opened", procName, NULL);

    if ((fpix = fpixReadStream(fp)) == NULL) {
        fclose(fp);
        return (FPIX *)ERROR_PTR("fpix not read", procName, NULL);
    }

    fclose(fp);
    return fpix;
}


/*!
 *  fpixReadStream()
 *
 *      Input:  stream
 *      Return: fpix, or null on error
 */
FPIX *
fpixReadStream(FILE  *fp)
{
char        buf[256];
l_int32     w, h, nbytes, xres, yres, version;
l_float32  *data;
FPIX       *fpix;

    PROCNAME("fpixReadStream");

    if (!fp)
        return (FPIX *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nFPix Version %d\n", &version) != 1)
        return (FPIX *)ERROR_PTR("not a fpix file", procName, NULL);
    if (version != FPIX_VERSION_NUMBER)
        return (FPIX *)ERROR_PTR("invalid fpix version", procName, NULL);
    if (fscanf(fp, "w = %d, h = %d, nbytes = %d\n", &w, &h, &nbytes) != 3)
        return (FPIX *)ERROR_PTR("read fail for data size", procName, NULL);

        /* Use fgets() and sscanf(); not fscanf(), for the last
         * bit of header data before the float data.  The reason is
         * that fscanf throws away white space, and if the float data
         * happens to begin with ascii character(s) that are white
         * space, it will swallow them and all will be lost!  */
    if (fgets(buf, sizeof(buf), fp) == NULL)
        return (FPIX *)ERROR_PTR("fgets read fail", procName, NULL);
    if (sscanf(buf, "xres = %d, yres = %d\n", &xres, &yres) != 2)
        return (FPIX *)ERROR_PTR("read fail for xres, yres", procName, NULL);

    if ((fpix = fpixCreate(w, h)) == NULL)
        return (FPIX *)ERROR_PTR("fpix not made", procName, NULL);
    fpixSetResolution(fpix, xres, yres);
    data = fpixGetData(fpix);
    if (fread(data, 1, nbytes, fp) != nbytes)
        return (FPIX *)ERROR_PTR("read error for nbytes", procName, NULL);
    fgetc(fp);  /* ending nl */

        /* Convert to little-endian if necessary */
    fpixEndianByteSwap(fpix, fpix);
    return fpix;
}


/*!
 *  fpixWrite()
 *
 *      Input:  filename
 *              fpix
 *      Return: 0 if OK, 1 on error
 */
l_int32
fpixWrite(const char  *filename,
          FPIX        *fpix)
{
FILE  *fp;

    PROCNAME("fpixWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!fpix)
        return ERROR_INT("fpix not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "wb")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (fpixWriteStream(fp, fpix))
        return ERROR_INT("fpix not written to stream", procName, 1);
    fclose(fp);

    return 0;
}


/*!
 *  fpixWriteStream()
 *
 *      Input:  stream (opened for "wb")
 *              fpix
 *      Return: 0 if OK, 1 on error
 */
l_int32
fpixWriteStream(FILE  *fp,
                FPIX  *fpix)
{
l_int32     w, h, nbytes, xres, yres;
l_float32  *data;
FPIX       *fpixt;

    PROCNAME("fpixWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!fpix)
        return ERROR_INT("fpix not defined", procName, 1);

        /* Convert to little-endian if necessary */
    fpixt = fpixEndianByteSwap(NULL, fpix);

    fpixGetDimensions(fpixt, &w, &h);
    data = fpixGetData(fpixt);
    nbytes = w * h * sizeof(l_float32);
    fpixGetResolution(fpixt, &xres, &yres);
    fprintf(fp, "\nFPix Version %d\n", FPIX_VERSION_NUMBER);
    fprintf(fp, "w = %d, h = %d, nbytes = %d\n", w, h, nbytes);
    fprintf(fp, "xres = %d, yres = %d\n", xres, yres);
    fwrite(data, 1, nbytes, fp);
    fprintf(fp, "\n");

    fpixDestroy(&fpixt);
    return 0;
}


/*!
 *  fpixEndianByteSwap()
 *
 *      Input:  fpixd (can be equal to fpixs or NULL)
 *              fpixs
 *      Return: fpixd always
 *
 *  Notes:
 *      (1) On big-endian hardware, this does byte-swapping on each of
 *          the 4-byte floats in the fpix data.  On little-endians,
 *          the data is unchanged.  This is used for serialization
 *          of fpix; the data is serialized in little-endian byte
 *          order because most hardware is little-endian.
 *      (2) The operation can be either in-place or, if fpixd == NULL,
 *          a new fpix is made.  If not in-place, caller must catch
 *          the returned pointer.
 */
FPIX *
fpixEndianByteSwap(FPIX  *fpixd,
                   FPIX  *fpixs)
{
    PROCNAME("fpixEndianByteSwap");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, fpixd);
    if (fpixd && (fpixs != fpixd))
        return (FPIX *)ERROR_PTR("fpixd != fpixs", procName, fpixd);

#ifdef L_BIG_ENDIAN
    {
    l_uint32  *data;
    l_int32    i, j, w, h;
    l_uint32   word;

        fpixGetDimensions(fpixs, &w, &h);
        fpixd = fpixCopy(fpixd, fpixs);  /* no copy if fpixd == fpixs */

        data = (l_uint32 *)fpixGetData(fpixd);
        for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++, data++) {
                word = *data;
                *data = (word >> 24) |
                        ((word >> 8) & 0x0000ff00) |
                        ((word << 8) & 0x00ff0000) |
                        (word << 24);
            }
        }
        return fpixd;
    }
#else   /* L_LITTLE_ENDIAN */

    if (fpixd)
        return fpixd;  /* no-op */
    else
        return fpixClone(fpixs);

#endif   /* L_BIG_ENDIAN */
}


/*--------------------------------------------------------------------*
 *                       DPix serialized I/O                          *
 *--------------------------------------------------------------------*/
/*!
 *  dpixRead()
 *
 *      Input:  filename
 *      Return: dpix, or null on error
 */
DPIX *
dpixRead(const char  *filename)
{
FILE  *fp;
DPIX  *dpix;

    PROCNAME("dpixRead");

    if (!filename)
        return (DPIX *)ERROR_PTR("filename not defined", procName, NULL);
    if ((fp = fopenReadStream(filename)) == NULL)
        return (DPIX *)ERROR_PTR("stream not opened", procName, NULL);

    if ((dpix = dpixReadStream(fp)) == NULL) {
        fclose(fp);
        return (DPIX *)ERROR_PTR("dpix not read", procName, NULL);
    }

    fclose(fp);
    return dpix;
}


/*!
 *  dpixReadStream()
 *
 *      Input:  stream
 *      Return: dpix, or null on error
 */
DPIX *
dpixReadStream(FILE  *fp)
{
char        buf[256];
l_int32     w, h, nbytes, version, xres, yres;
l_float64  *data;
DPIX       *dpix;

    PROCNAME("dpixReadStream");

    if (!fp)
        return (DPIX *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nDPix Version %d\n", &version) != 1)
        return (DPIX *)ERROR_PTR("not a dpix file", procName, NULL);
    if (version != DPIX_VERSION_NUMBER)
        return (DPIX *)ERROR_PTR("invalid dpix version", procName, NULL);
    if (fscanf(fp, "w = %d, h = %d, nbytes = %d\n", &w, &h, &nbytes) != 3)
        return (DPIX *)ERROR_PTR("read fail for data size", procName, NULL);

        /* Use fgets() and sscanf(); not fscanf(), for the last
         * bit of header data before the float data.  The reason is
         * that fscanf throws away white space, and if the float data
         * happens to begin with ascii character(s) that are white
         * space, it will swallow them and all will be lost!  */
    if (fgets(buf, sizeof(buf), fp) == NULL)
        return (DPIX *)ERROR_PTR("fgets read fail", procName, NULL);
    if (sscanf(buf, "xres = %d, yres = %d\n", &xres, &yres) != 2)
        return (DPIX *)ERROR_PTR("read fail for xres, yres", procName, NULL);

    if ((dpix = dpixCreate(w, h)) == NULL)
        return (DPIX *)ERROR_PTR("dpix not made", procName, NULL);
    dpixSetResolution(dpix, xres, yres);
    data = dpixGetData(dpix);
    if (fread(data, 1, nbytes, fp) != nbytes)
        return (DPIX *)ERROR_PTR("read error for nbytes", procName, NULL);
    fgetc(fp);  /* ending nl */

        /* Convert to little-endian if necessary */
    dpixEndianByteSwap(dpix, dpix);
    return dpix;
}


/*!
 *  dpixWrite()
 *
 *      Input:  filename
 *              dpix
 *      Return: 0 if OK, 1 on error
 */
l_int32
dpixWrite(const char  *filename,
          DPIX        *dpix)
{
FILE  *fp;

    PROCNAME("dpixWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!dpix)
        return ERROR_INT("dpix not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "wb")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (dpixWriteStream(fp, dpix))
        return ERROR_INT("dpix not written to stream", procName, 1);
    fclose(fp);

    return 0;
}


/*!
 *  dpixWriteStream()
 *
 *      Input:  stream (opened for "wb")
 *              dpix
 *      Return: 0 if OK, 1 on error
 */
l_int32
dpixWriteStream(FILE  *fp,
                DPIX  *dpix)
{
l_int32     w, h, nbytes, xres, yres;
l_float64  *data;
DPIX       *dpixt;

    PROCNAME("dpixWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!dpix)
        return ERROR_INT("dpix not defined", procName, 1);

        /* Convert to little-endian if necessary */
    dpixt = dpixEndianByteSwap(NULL, dpix);

    dpixGetDimensions(dpixt, &w, &h);
    dpixGetResolution(dpixt, &xres, &yres);
    data = dpixGetData(dpixt);
    nbytes = w * h * sizeof(l_float64);
    fprintf(fp, "\nDPix Version %d\n", DPIX_VERSION_NUMBER);
    fprintf(fp, "w = %d, h = %d, nbytes = %d\n", w, h, nbytes);
    fprintf(fp, "xres = %d, yres = %d\n", xres, yres);
    fwrite(data, 1, nbytes, fp);
    fprintf(fp, "\n");

    dpixDestroy(&dpixt);
    return 0;
}


/*!
 *  dpixEndianByteSwap()
 *
 *      Input:  dpixd (can be equal to dpixs or NULL)
 *              dpixs
 *      Return: dpixd always
 *
 *  Notes:
 *      (1) On big-endian hardware, this does byte-swapping on each of
 *          the 4-byte words in the dpix data.  On little-endians,
 *          the data is unchanged.  This is used for serialization
 *          of dpix; the data is serialized in little-endian byte
 *          order because most hardware is little-endian.
 *      (2) The operation can be either in-place or, if dpixd == NULL,
 *          a new dpix is made.  If not in-place, caller must catch
 *          the returned pointer.
 */
DPIX *
dpixEndianByteSwap(DPIX  *dpixd,
                   DPIX  *dpixs)
{
    PROCNAME("dpixEndianByteSwap");

    if (!dpixs)
        return (DPIX *)ERROR_PTR("dpixs not defined", procName, dpixd);
    if (dpixd && (dpixs != dpixd))
        return (DPIX *)ERROR_PTR("dpixd != dpixs", procName, dpixd);

#ifdef L_BIG_ENDIAN
    {
    l_uint32  *data;
    l_int32    i, j, w, h;
    l_uint32   word;

        dpixGetDimensions(dpixs, &w, &h);
        dpixd = dpixCopy(dpixd, dpixs);  /* no copy if dpixd == dpixs */

        data = (l_uint32 *)dpixGetData(dpixd);
        for (i = 0; i < h; i++) {
            for (j = 0; j < 2 * w; j++, data++) {
                word = *data;
                *data = (word >> 24) |
                        ((word >> 8) & 0x0000ff00) |
                        ((word << 8) & 0x00ff0000) |
                        (word << 24);
            }
        }
        return dpixd;
    }
#else   /* L_LITTLE_ENDIAN */

    if (dpixd)
        return dpixd;  /* no-op */
    else
        return dpixClone(dpixs);

#endif   /* L_BIG_ENDIAN */
}


/*--------------------------------------------------------------------*
 *                 Print FPix (subsampled, for debugging)             *
 *--------------------------------------------------------------------*/
/*!
 *  fpixPrintStream()
 *
 *      Input:  stream
 *              fpix
 *              factor (subsampled)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Subsampled printout of fpix for debugging.
 */
l_int32
fpixPrintStream(FILE    *fp,
                FPIX    *fpix,
                l_int32  factor)
{
l_int32    i, j, w, h, count;
l_float32  val;

    PROCNAME("fpixPrintStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!fpix)
        return ERROR_INT("fpix not defined", procName, 1);
    if (factor < 1)
        return ERROR_INT("sampling factor < 1f", procName, 1);

    fpixGetDimensions(fpix, &w, &h);
    fprintf(fp, "\nFPix: w = %d, h = %d\n", w, h);
    for (i = 0; i < h; i += factor) {
        for (count = 0, j = 0; j < w; j += factor, count++) {
            fpixGetPixel(fpix, j, i, &val);
            fprintf(fp, "val[%d, %d] = %f   ", i, j, val);
            if ((count + 1) % 3 == 0) fprintf(fp, "\n");
        }
        if (count % 3) fprintf(fp, "\n");
     }
     fprintf(fp, "\n");
     return 0;
}
