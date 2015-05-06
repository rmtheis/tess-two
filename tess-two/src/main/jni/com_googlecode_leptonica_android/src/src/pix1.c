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
 *  pix1.c
 *
 *    The pixN.c {N = 1,2,3,4,5} files are sorted by the type of operation.
 *    The primary functions in these files are:
 *
 *        pix1.c: constructors, destructors and field accessors
 *        pix2.c: pixel poking of image, pad and border pixels
 *        pix3.c: masking and logical ops, counting, mirrored tiling
 *        pix4.c: histograms, statistics, fg/bg estimation
 *        pix5.c: property measurements, rectangle extraction
 *
 *
 *    This file has the basic constructors, destructors and field accessors
 *
 *    Pix memory management (allows custom allocator and deallocator)
 *          static void  *pix_malloc()
 *          static void   pix_free()
 *          void          setPixMemoryManager()
 *
 *    Pix creation
 *          PIX          *pixCreate()
 *          PIX          *pixCreateNoInit()
 *          PIX          *pixCreateTemplate()
 *          PIX          *pixCreateTemplateNoInit()
 *          PIX          *pixCreateHeader()
 *          PIX          *pixClone()
 *
 *    Pix destruction
 *          void          pixDestroy()
 *          static void   pixFree()
 *
 *    Pix copy
 *          PIX          *pixCopy()
 *          l_int32       pixResizeImageData()
 *          l_int32       pixCopyColormap()
 *          l_int32       pixSizesEqual()
 *          l_int32       pixTransferAllData()
 *          l_int32       pixSwapAndDestroy()
 *
 *    Pix accessors
 *          l_int32       pixGetWidth()
 *          l_int32       pixSetWidth()
 *          l_int32       pixGetHeight()
 *          l_int32       pixSetHeight()
 *          l_int32       pixGetDepth()
 *          l_int32       pixSetDepth()
 *          l_int32       pixGetDimensions()
 *          l_int32       pixSetDimensions()
 *          l_int32       pixCopyDimensions()
 *          l_int32       pixGetSpp()
 *          l_int32       pixSetSpp()
 *          l_int32       pixCopySpp()
 *          l_int32       pixGetWpl()
 *          l_int32       pixSetWpl()
 *          l_int32       pixGetRefcount()
 *          l_int32       pixChangeRefcount()
 *          l_uint32      pixGetXRes()
 *          l_int32       pixSetXRes()
 *          l_uint32      pixGetYRes()
 *          l_int32       pixSetYRes()
 *          l_int32       pixGetResolution()
 *          l_int32       pixSetResolution()
 *          l_int32       pixCopyResolution()
 *          l_int32       pixScaleResolution()
 *          l_int32       pixGetInputFormat()
 *          l_int32       pixSetInputFormat()
 *          l_int32       pixCopyInputFormat()
 *          char         *pixGetText()
 *          l_int32       pixSetText()
 *          l_int32       pixAddText()
 *          l_int32       pixCopyText()
 *          PIXCMAP      *pixGetColormap()
 *          l_int32       pixSetColormap()
 *          l_int32       pixDestroyColormap()
 *          l_uint32     *pixGetData()
 *          l_int32       pixSetData()
 *          l_uint32     *pixExtractData()
 *          l_int32       pixFreeData()
 *
 *    Pix line ptrs
 *          void        **pixGetLinePtrs()
 *
 *    Pix debug
 *          l_int32       pixPrintStreamInfo()
 *
 *
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *      Important notes on direct management of pix image data
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *  Custom allocator and deallocator
 *  --------------------------------
 *
 *  At the lowest level, you can specify the function that does the
 *  allocation and deallocation of the data field in the pix.
 *  By default, this is malloc and free.  However, by calling
 *  setPixMemoryManager(), custom functions can be substituted.
 *  When using this, keep two things in mind:
 *
 *   (1) Call setPixMemoryManager() before any pix have been allocated
 *   (2) Destroy all pix as usual, in order to prevent leaks.
 *
 *  In pixalloc.c, we provide an example custom allocator and deallocator.
 *  To use it, you must call pmsCreate() before any pix have been allocated
 *  and pmsDestroy() at the end after all pix have been destroyed.
 *
 *
 *  Direct manipulation of the pix data field
 *  -----------------------------------------
 *
 *  Memory management of the (image) data field in the pix is
 *  handled differently from that in the colormap or text fields.
 *  For colormap and text, the functions pixSetColormap() and
 *  pixSetText() remove the existing heap data and insert the
 *  new data.  For the image data, pixSetData() just reassigns the
 *  data field; any existing data will be lost if there isn't
 *  another handle for it.
 *
 *  Why is pixSetData() limited in this way?  Because the image
 *  data can be very large, we need flexible ways to handle it,
 *  particularly when you want to re-use the data in a different
 *  context without making a copy.  Here are some different
 *  things you might want to do:
 *
 *  (1) Use pixCopy(pixd, pixs) where pixd is not the same size
 *      as pixs.  This will remove the data in pixd, allocate a
 *      new data field in pixd, and copy the data from pixs, leaving
 *      pixs unchanged.
 *
 *  (2) Use pixTransferAllData(pixd, &pixs, ...) to transfer the
 *      data from pixs to pixd without making a copy of it.  If
 *      pixs is not cloned, this will do the transfer and destroy pixs.
 *      But if the refcount of pixs is greater than 1, it just copies
 *      the data and decrements the ref count.
 *
 *  (3) Use pixSwapAndDestroy(pixd, &pixs) to replace pixs by an
 *      existing pixd.  This is similar to pixTransferAllData(), but
 *      simpler, in that it never makes any copies and if pixs is
 *      cloned, the other references are not changed by this operation.
 *
 *  (4) Use pixExtractData() to extract the image data from the pix
 *      without copying if possible.  This could be used, for example,
 *      to convert from a pix to some other data structure with minimal
 *      heap allocation.  After the data is extracated, the pixels can
 *      be munged and used in another context.  However, the danger
 *      here is that the pix might have a refcount > 1, in which case
 *      a copy of the data must be made and the input pix left unchanged.
 *      If there are no clones, the image data can be extracted without
 *      a copy, and the data ptr in the pix must be nulled before
 *      destroying it because the pix will no longer 'own' the data.
 *
 *  We have provided accessors and functions here that should be
 *  sufficient so that you can do anything you want without
 *  explicitly referencing any of the pix member fields.
 *
 *  However, to avoid memory smashes and leaks when doing special operations
 *  on the pix data field, look carefully at the behavior of the image
 *  data accessors and keep in mind that when you invoke pixDestroy(),
 *  the pix considers itself the owner of all its heap data.
 */

#include <string.h>
#include "allheaders.h"

static void pixFree(PIX *pix);


/*-------------------------------------------------------------------------*
 *                        Pix Memory Management                            *
 *                                                                         *
 *  These functions give you the freedom to specify at compile or run      *
 *  time the allocator and deallocator to be used for pix.  It has no      *
 *  effect on memory management for other data structs, which are          *
 *  controlled by the #defines in environ.h.  Likewise, the #defines       *
 *  in environ.h have no effect on the pix memory management.              *
 *  The default functions are malloc and free.  Use setPixMemoryManager()  *
 *  to specify other functions to use.                                     *
 *-------------------------------------------------------------------------*/
struct PixMemoryManager
{
    void     *(*allocator)(size_t);
    void      (*deallocator)(void *);
};

static struct PixMemoryManager  pix_mem_manager = {
    &malloc,
    &free
};

static void *
pix_malloc(size_t  size)
{
#ifndef _MSC_VER
    return (*pix_mem_manager.allocator)(size);
#else  /* _MSC_VER */
    /* Under MSVC++, pix_mem_manager is initialized after a call
     * to pix_malloc.  Just ignore the custom allocator feature. */
    return malloc(size);
#endif  /* _MSC_VER */
}

static void
pix_free(void  *ptr)
{
#ifndef _MSC_VER
    (*pix_mem_manager.deallocator)(ptr);
    return;
#else  /* _MSC_VER */
    /* Under MSVC++, pix_mem_manager is initialized after a call
     * to pix_malloc.  Just ignore the custom allocator feature. */
    free(ptr);
    return;
#endif  /* _MSC_VER */
}

/*!
 *  setPixMemoryManager()
 *
 *      Input: allocator (<optional>; use null to skip)
 *             deallocator (<optional>; use null to skip)
 *      Return: void
 *
 *  Notes:
 *      (1) Use this to change the alloc and/or dealloc functions;
 *          e.g., setPixMemoryManager(my_malloc, my_free).
 *      (2) The C99 standard (section 6.7.5.3, par. 8) says:
 *            A declaration of a parameter as "function returning type"
 *            shall be adjusted to "pointer to function returning type"
 *          so that it can be in either of these two forms:
 *            (a) type (function-ptr(type, ...))
 *            (b) type ((*function-ptr)(type, ...))
 *          because form (a) is implictly converted to form (b), as in the
 *          definition of struct PixMemoryManager above.  So, for example,
 *          we should be able to declare either of these:
 *            (a) void *(allocator(size_t))
 *            (b) void *((*allocator)(size_t))
 *          However, MSVC++ only accepts the second version.
 */
void
setPixMemoryManager(void  *((*allocator)(size_t)),
                    void   ((*deallocator)(void *)))
{
    if (allocator) pix_mem_manager.allocator = allocator;
    if (deallocator) pix_mem_manager.deallocator = deallocator;
    return;
}


/*--------------------------------------------------------------------*
 *                              Pix Creation                          *
 *--------------------------------------------------------------------*/
/*!
 *  pixCreate()
 *
 *      Input:  width, height, depth
 *      Return: pixd (with data allocated and initialized to 0),
 *                    or null on error
 */
PIX *
pixCreate(l_int32  width,
          l_int32  height,
          l_int32  depth)
{
PIX  *pixd;

    PROCNAME("pixCreate");

    if ((pixd = pixCreateNoInit(width, height, depth)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    memset(pixd->data, 0, 4 * pixd->wpl * pixd->h);
    return pixd;
}


/*!
 *  pixCreateNoInit()
 *
 *      Input:  width, height, depth
 *      Return: pixd (with data allocated but not initialized),
 *                    or null on error
 *
 *  Notes:
 *      (1) Must set pad bits to avoid reading unitialized data, because
 *          some optimized routines (e.g., pixConnComp()) read from pad bits.
 */
PIX *
pixCreateNoInit(l_int32  width,
                l_int32  height,
                l_int32  depth)
{
l_int32    wpl;
PIX       *pixd;
l_uint32  *data;

    PROCNAME("pixCreateNoInit");
    if ((pixd = pixCreateHeader(width, height, depth)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    wpl = pixGetWpl(pixd);
    if ((data = (l_uint32 *)pix_malloc(4 * wpl * height)) == NULL) {
        pixDestroy(&pixd);
        return (PIX *)ERROR_PTR("pix_malloc fail for data", procName, NULL);
    }
    pixSetData(pixd, data);
    pixSetPadBits(pixd, 0);
    return pixd;
}


/*!
 *  pixCreateTemplate()
 *
 *      Input:  pixs
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Makes a Pix of the same size as the input Pix, with the
 *          data array allocated and initialized to 0.
 *      (2) Copies the other fields, including colormap if it exists.
 */
PIX *
pixCreateTemplate(PIX  *pixs)
{
PIX  *pixd;

    PROCNAME("pixCreateTemplate");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    if ((pixd = pixCreateTemplateNoInit(pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    memset(pixd->data, 0, 4 * pixd->wpl * pixd->h);
    return pixd;
}


/*!
 *  pixCreateTemplateNoInit()
 *
 *      Input:  pixs
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Makes a Pix of the same size as the input Pix, with
 *          the data array allocated but not initialized to 0.
 *      (2) Copies the other fields, including colormap if it exists.
 */
PIX *
pixCreateTemplateNoInit(PIX  *pixs)
{
l_int32  w, h, d;
PIX     *pixd;

    PROCNAME("pixCreateTemplateNoInit");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    pixGetDimensions(pixs, &w, &h, &d);
    if ((pixd = pixCreateNoInit(w, h, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopySpp(pixd, pixs);
    pixCopyResolution(pixd, pixs);
    pixCopyColormap(pixd, pixs);
    pixCopyText(pixd, pixs);
    pixCopyInputFormat(pixd, pixs);

    return pixd;
}


/*!
 *  pixCreateHeader()
 *
 *      Input:  width, height, depth
 *      Return: pixd (with no data allocated), or null on error
 *
 *  Notes:
 *      (1) It is assumed that all 32 bit pix have 3 spp.  If there is
 *          a valid alpha channel, this will be set to 4 spp later.
 *      (2) If the number of bytes to be allocated is larger than the
 *          maximum value in an int32, we can get overflow, resulting
 *          in a smaller amount of memory actually being allocated.
 *          Later, an attempt to access memory that wasn't allocated will
 *          cause a crash.  So to avoid crashing a program (or worse)
 *          with bad (or malicious) input, this is where we limit the
 *          requested allocation of image data in a typesafe way.
 */
PIX *
pixCreateHeader(l_int32  width,
                l_int32  height,
                l_int32  depth)
{
l_int32   wpl;
l_uint64  bignum;
PIX      *pixd;

    PROCNAME("pixCreateHeader");

    if ((depth != 1) && (depth != 2) && (depth != 4) && (depth != 8)
         && (depth != 16) && (depth != 24) && (depth != 32))
        return (PIX *)ERROR_PTR("depth must be {1, 2, 4, 8, 16, 24, 32}",
                                procName, NULL);
    if (width <= 0)
        return (PIX *)ERROR_PTR("width must be > 0", procName, NULL);
    if (height <= 0)
        return (PIX *)ERROR_PTR("height must be > 0", procName, NULL);

        /* Avoid overflow in malloc arg, malicious or otherwise */
    wpl = (width * depth + 31) / 32;
    bignum = 4L * wpl * height;   /* number of bytes to be requested */
    if (bignum > ((1LL << 31) - 1)) {
        L_ERROR("requested w = %d, h = %d, d = %d\n",
                procName, width, height, depth);
        return (PIX *)ERROR_PTR("requested bytes >= 2^31", procName, NULL);
    }

    if ((pixd = (PIX *)CALLOC(1, sizeof(PIX))) == NULL)
        return (PIX *)ERROR_PTR("CALLOC fail for pixd", procName, NULL);
    pixSetWidth(pixd, width);
    pixSetHeight(pixd, height);
    pixSetDepth(pixd, depth);
    pixSetWpl(pixd, wpl);
    if (depth == 24 || depth == 32)
        pixSetSpp(pixd, 3);
    else
        pixSetSpp(pixd, 1);

    pixd->refcount = 1;
    pixd->informat = IFF_UNKNOWN;
    return pixd;
}


/*!
 *  pixClone()
 *
 *      Input:  pix
 *      Return: same pix (ptr), or null on error
 *
 *  Notes:
 *      (1) A "clone" is simply a handle (ptr) to an existing pix.
 *          It is implemented because (a) images can be large and
 *          hence expensive to copy, and (b) extra handles to a data
 *          structure need to be made with a simple policy to avoid
 *          both double frees and memory leaks.  Pix are reference
 *          counted.  The side effect of pixClone() is an increase
 *          by 1 in the ref count.
 *      (2) The protocol to be used is:
 *          (a) Whenever you want a new handle to an existing image,
 *              call pixClone(), which just bumps a ref count.
 *          (b) Always call pixDestroy() on all handles.  This
 *              decrements the ref count, nulls the handle, and
 *              only destroys the pix when pixDestroy() has been
 *              called on all handles.
 */
PIX *
pixClone(PIX  *pixs)
{
    PROCNAME("pixClone");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixChangeRefcount(pixs, 1);

    return pixs;
}


/*--------------------------------------------------------------------*
 *                           Pix Destruction                          *
 *--------------------------------------------------------------------*/
/*!
 *  pixDestroy()
 *
 *      Input:  &pix <will be nulled>
 *      Return: void
 *
 *  Notes:
 *      (1) Decrements the ref count and, if 0, destroys the pix.
 *      (2) Always nulls the input ptr.
 */
void
pixDestroy(PIX  **ppix)
{
PIX  *pix;

    PROCNAME("pixDestroy");

    if (!ppix) {
        L_WARNING("ptr address is null!\n", procName);
        return;
    }

    if ((pix = *ppix) == NULL)
        return;

    pixFree(pix);
    *ppix = NULL;
    return;
}


/*!
 *  pixFree()
 *
 *      Input:  pix
 *      Return: void
 *
 *  Notes:
 *      (1) Decrements the ref count and, if 0, destroys the pix.
 */
static void
pixFree(PIX  *pix)
{
l_uint32  *data;
char      *text;

    if (!pix) return;

    pixChangeRefcount(pix, -1);
    if (pixGetRefcount(pix) <= 0) {
        if ((data = pixGetData(pix)) != NULL)
            pix_free(data);
        if ((text = pixGetText(pix)) != NULL)
            FREE(text);
        pixDestroyColormap(pix);
        FREE(pix);
    }
    return;
}


/*-------------------------------------------------------------------------*
 *                                 Pix Copy                                *
 *-------------------------------------------------------------------------*/
/*!
 *  pixCopy()
 *
 *      Input:  pixd (<optional>; can be null, or equal to pixs,
 *                    or different from pixs)
 *              pixs
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) There are three cases:
 *            (a) pixd == null  (makes a new pix; refcount = 1)
 *            (b) pixd == pixs  (no-op)
 *            (c) pixd != pixs  (data copy; no change in refcount)
 *          If the refcount of pixd > 1, case (c) will side-effect
 *          these handles.
 *      (2) The general pattern of use is:
 *             pixd = pixCopy(pixd, pixs);
 *          This will work for all three cases.
 *          For clarity when the case is known, you can use:
 *            (a) pixd = pixCopy(NULL, pixs);
 *            (c) pixCopy(pixd, pixs);
 *      (3) For case (c), we check if pixs and pixd are the same
 *          size (w,h,d).  If so, the data is copied directly.
 *          Otherwise, the data is reallocated to the correct size
 *          and the copy proceeds.  The refcount of pixd is unchanged.
 *      (4) This operation, like all others that may involve a pre-existing
 *          pixd, will side-effect any existing clones of pixd.
 */
PIX *
pixCopy(PIX  *pixd,   /* can be null */
        PIX  *pixs)
{
l_int32    bytes;
l_uint32  *datas, *datad;

    PROCNAME("pixCopy");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixs == pixd)
        return pixd;

        /* Total bytes in image data */
    bytes = 4 * pixGetWpl(pixs) * pixGetHeight(pixs);

        /* If we're making a new pix ... */
    if (!pixd) {
        if ((pixd = pixCreateTemplate(pixs)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
        datas = pixGetData(pixs);
        datad = pixGetData(pixd);
        memcpy((char *)datad, (char *)datas, bytes);
        return pixd;
    }

        /* Reallocate image data if sizes are different */
    if (pixResizeImageData(pixd, pixs) == 1)
        return (PIX *)ERROR_PTR("reallocation of data failed", procName, NULL);

        /* Copy non-image data fields */
    pixCopyColormap(pixd, pixs);
    pixCopySpp(pixd, pixs);
    pixCopyResolution(pixd, pixs);
    pixCopyInputFormat(pixd, pixs);
    pixCopyText(pixd, pixs);

        /* Copy image data */
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    memcpy((char*)datad, (char*)datas, bytes);
    return pixd;
}


/*!
 *  pixResizeImageData()
 *
 *      Input:  pixd (gets new uninitialized buffer for image data)
 *              pixs (determines the size of the buffer; not changed)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This removes any existing image data from pixd and
 *          allocates an uninitialized buffer that will hold the
 *          amount of image data that is in pixs.
 */
l_int32
pixResizeImageData(PIX  *pixd,
                   PIX  *pixs)
{
l_int32    w, h, d, wpl, bytes;
l_uint32  *data;

    PROCNAME("pixResizeImageData");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);

    if (pixSizesEqual(pixs, pixd))  /* nothing to do */
        return 0;

    pixGetDimensions(pixs, &w, &h, &d);
    wpl = pixGetWpl(pixs);
    pixSetWidth(pixd, w);
    pixSetHeight(pixd, h);
    pixSetDepth(pixd, d);
    pixSetWpl(pixd, wpl);
    bytes = 4 * wpl * h;
    pixFreeData(pixd);  /* free any existing image data */
    if ((data = (l_uint32 *)pix_malloc(bytes)) == NULL)
        return ERROR_INT("pix_malloc fail for data", procName, 1);
    pixSetData(pixd, data);
    return 0;
}


/*!
 *  pixCopyColormap()
 *
 *      Input:  src and dest Pix
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This always destroys any colormap in pixd (except if
 *          the operation is a no-op.
 */
l_int32
pixCopyColormap(PIX  *pixd,
                PIX  *pixs)
{
PIXCMAP  *cmaps, *cmapd;

    PROCNAME("pixCopyColormap");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (pixs == pixd)
        return 0;   /* no-op */

    pixDestroyColormap(pixd);
    if ((cmaps = pixGetColormap(pixs)) == NULL)  /* not an error */
        return 0;

    if ((cmapd = pixcmapCopy(cmaps)) == NULL)
        return ERROR_INT("cmapd not made", procName, 1);
    pixSetColormap(pixd, cmapd);

    return 0;
}


/*!
 *  pixSizesEqual()
 *
 *      Input:  two pix
 *      Return: 1 if the two pix have same {h, w, d}; 0 otherwise.
 */
l_int32
pixSizesEqual(PIX  *pix1,
              PIX  *pix2)
{
    PROCNAME("pixSizesEqual");

    if (!pix1 || !pix2)
        return ERROR_INT("pix1 and pix2 not both defined", procName, 0);

    if (pix1 == pix2)
        return 1;

    if ((pixGetWidth(pix1) != pixGetWidth(pix2)) ||
        (pixGetHeight(pix1) != pixGetHeight(pix2)) ||
        (pixGetDepth(pix1) != pixGetDepth(pix2)))
        return 0;
    else
        return 1;
}


/*!
 *  pixTransferAllData()
 *
 *      Input:  pixd (must be different from pixs)
 *              &pixs (will be nulled if refcount goes to 0)
 *              copytext (1 to copy the text field; 0 to skip)
 *              copyformat (1 to copy the informat field; 0 to skip)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This does a complete data transfer from pixs to pixd,
 *          followed by the destruction of pixs (refcount permitting).
 *      (2) If the refcount of pixs is 1, pixs is destroyed.  Otherwise,
 *          the data in pixs is copied (rather than transferred) to pixd.
 *      (3) This operation, like all others with a pre-existing pixd,
 *          will side-effect any existing clones of pixd.  The pixd
 *          refcount does not change.
 *      (4) When might you use this?  Suppose you have an in-place Pix
 *          function (returning void) with the typical signature:
 *              void function-inplace(PIX *pix, ...)
 *          where "..." are non-pointer input parameters, and suppose
 *          further that you sometimes want to return an arbitrary Pix
 *          in place of the input Pix.  There are two ways you can do this:
 *          (a) The straightforward way is to change the function
 *              signature to take the address of the Pix ptr:
 *                  void function-inplace(PIX **ppix, ...) {
 *                      PIX *pixt = function-makenew(*ppix);
 *                      pixDestroy(ppix);
 *                      *ppix = pixt;
 *                      return;
 *                  }
 *              Here, the input and returned pix are different, as viewed
 *              by the calling function, and the inplace function is
 *              expected to destroy the input pix to avoid a memory leak.
 *          (b) Keep the signature the same and use pixTransferAllData()
 *              to return the new Pix in the input Pix struct:
 *                  void function-inplace(PIX *pix, ...) {
 *                      PIX *pixt = function-makenew(pix);
 *                      pixTransferAllData(pix, &pixt, 0, 0);
 *                               // pixDestroy() is called on pixt
 *                      return;
 *                  }
 *              Here, the input and returned pix are the same, as viewed
 *              by the calling function, and the inplace function must
 *              never destroy the input pix, because the calling function
 *              maintains an unchanged handle to it.
 */
l_int32
pixTransferAllData(PIX     *pixd,
                   PIX    **ppixs,
                   l_int32  copytext,
                   l_int32  copyformat)
{
l_int32  nbytes;
PIX     *pixs;

    PROCNAME("pixTransferAllData");

    if (!ppixs)
        return ERROR_INT("&pixs not defined", procName, 1);
    if ((pixs = *ppixs) == NULL)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (pixs == pixd)  /* no-op */
        return ERROR_INT("pixd == pixs", procName, 1);

    if (pixGetRefcount(pixs) == 1) {  /* transfer the data, cmap, text */
        pixFreeData(pixd);  /* dealloc any existing data */
        pixSetData(pixd, pixGetData(pixs));  /* transfer new data from pixs */
        pixs->data = NULL;  /* pixs no longer owns data */
        pixSetColormap(pixd, pixGetColormap(pixs));  /* frees old; sets new */
        pixs->colormap = NULL;  /* pixs no longer owns colormap */
        if (copytext) {
            pixSetText(pixd, pixGetText(pixs));
            pixSetText(pixs, NULL);
        }
    } else {  /* preserve pixs by making a copy of the data, cmap, text */
        pixResizeImageData(pixd, pixs);
        nbytes = 4 * pixGetWpl(pixs) * pixGetHeight(pixs);
        memcpy((char *)pixGetData(pixd), (char *)pixGetData(pixs), nbytes);
        pixCopyColormap(pixd, pixs);
        if (copytext)
            pixCopyText(pixd, pixs);
    }

    pixCopySpp(pixd, pixs);
    pixCopyResolution(pixd, pixs);
    pixCopyDimensions(pixd, pixs);
    if (copyformat)
        pixCopyInputFormat(pixd, pixs);

        /* This will destroy pixs if data was transferred;
         * otherwise, it just decrements its refcount. */
    pixDestroy(ppixs);
    return 0;
}


/*!
 *  pixSwapAndDestroy()
 *
 *      Input:  &pixd (<optional, return> input pixd can be null,
 *                     and it must be different from pixs)
 *              &pixs (will be nulled after the swap)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Simple operation to change the handle name safely.
 *          After this operation, the original image in pixd has
 *          been destroyed, pixd points to what was pixs, and
 *          the input pixs ptr has been nulled.
 *      (2) This works safely whether or not pixs and pixd are cloned.
 *          If pixs is cloned, the other handles still point to
 *          the original image, with the ref count reduced by 1.
 *      (3) Usage example:
 *            Pix *pix1 = pixRead("...");
 *            Pix *pix2 = function(pix1, ...);
 *            pixSwapAndDestroy(&pix1, &pix2);
 *            pixDestroy(&pix1);  // holds what was in pix2
 *          Example with clones ([] shows ref count of image generated
 *                               by the function):
 *            Pix *pixs = pixRead("...");
 *            Pix *pix1 = pixClone(pixs);
 *            Pix *pix2 = function(pix1, ...);   [1]
 *            Pix *pix3 = pixClone(pix2);   [1] --> [2]
 *            pixSwapAndDestroy(&pix1, &pix2);
 *            pixDestroy(&pixs);  // still holds read image
 *            pixDestroy(&pix1);  // holds what was in pix2  [2] --> [1]
 *            pixDestroy(&pix3);  // holds what was in pix2  [1] --> [0]
 */
l_int32
pixSwapAndDestroy(PIX  **ppixd,
                  PIX  **ppixs)
{
    PROCNAME("pixSwapAndDestroy");

    if (!ppixd)
        return ERROR_INT("&pixd not defined", procName, 1);
    if (!ppixs)
        return ERROR_INT("&pixs not defined", procName, 1);
    if (*ppixs == NULL)
        return ERROR_INT("pixs not defined", procName, 1);
    if (ppixs == ppixd)  /* no-op */
        return ERROR_INT("&pixd == &pixs", procName, 1);

    pixDestroy(ppixd);
    *ppixd = pixClone(*ppixs);
    pixDestroy(ppixs);
    return 0;
}


/*--------------------------------------------------------------------*
 *                                Accessors                           *
 *--------------------------------------------------------------------*/
l_int32
pixGetWidth(PIX  *pix)
{
    PROCNAME("pixGetWidth");

    if (!pix)
        return ERROR_INT("pix not defined", procName, UNDEF);

    return pix->w;
}


l_int32
pixSetWidth(PIX     *pix,
            l_int32  width)
{
    PROCNAME("pixSetWidth");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (width < 0) {
        pix->w = 0;
        return ERROR_INT("width must be >= 0", procName, 1);
    }

    pix->w = width;
    return 0;
}


l_int32
pixGetHeight(PIX  *pix)
{
    PROCNAME("pixGetHeight");

    if (!pix)
        return ERROR_INT("pix not defined", procName, UNDEF);

    return pix->h;
}


l_int32
pixSetHeight(PIX     *pix,
             l_int32  height)
{
    PROCNAME("pixSetHeight");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (height < 0) {
        pix->h = 0;
        return ERROR_INT("h must be >= 0", procName, 1);
    }

    pix->h = height;
    return 0;
}


l_int32
pixGetDepth(PIX  *pix)
{
    PROCNAME("pixGetDepth");

    if (!pix)
        return ERROR_INT("pix not defined", procName, UNDEF);

    return pix->d;
}


l_int32
pixSetDepth(PIX     *pix,
            l_int32  depth)
{
    PROCNAME("pixSetDepth");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (depth < 1)
        return ERROR_INT("d must be >= 1", procName, 1);

    pix->d = depth;
    return 0;
}


/*!
 *  pixGetDimensions()
 *
 *      Input:  pix
 *              &w, &h, &d (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixGetDimensions(PIX      *pix,
                 l_int32  *pw,
                 l_int32  *ph,
                 l_int32  *pd)
{
    PROCNAME("pixGetDimensions");

    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (pd) *pd = 0;
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (pw) *pw = pix->w;
    if (ph) *ph = pix->h;
    if (pd) *pd = pix->d;
    return 0;
}


/*!
 *  pixSetDimensions()
 *
 *      Input:  pix
 *              w, h, d (use 0 to skip the setting for any of these)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixSetDimensions(PIX     *pix,
                 l_int32  w,
                 l_int32  h,
                 l_int32  d)
{
    PROCNAME("pixSetDimensions");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (w > 0) pixSetWidth(pix, w);
    if (h > 0) pixSetHeight(pix, h);
    if (d > 0) pixSetDepth(pix, d);
    return 0;
}


/*!
 *  pixCopyDimensions()
 *
 *      Input:  pixd
 *              pixd
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixCopyDimensions(PIX  *pixd,
                  PIX  *pixs)
{
    PROCNAME("pixCopyDimensions");

    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixs == pixd)
        return 0;   /* no-op */

    pixSetWidth(pixd, pixGetWidth(pixs));
    pixSetHeight(pixd, pixGetHeight(pixs));
    pixSetDepth(pixd, pixGetDepth(pixs));
    pixSetWpl(pixd, pixGetWpl(pixs));
    return 0;
}


l_int32
pixGetSpp(PIX  *pix)
{
    PROCNAME("pixGetSpp");

    if (!pix)
        return ERROR_INT("pix not defined", procName, UNDEF);

    return pix->spp;
}


/*
 *  pixSetSpp()
 *      Input:  pix
 *              spp (1, 3 or 4)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) For a 32 bpp pix, this can be used to ignore the
 *          alpha sample (spp == 3) or to use it (spp == 4).
 *          For example, to write a spp == 4 image without the alpha
 *          sample (as an rgb pix), call pixSetSpp(pix, 3) and
 *          then write it out as a png.
 */
l_int32
pixSetSpp(PIX     *pix,
          l_int32  spp)
{
    PROCNAME("pixSetSpp");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (spp < 1)
        return ERROR_INT("spp must be >= 1", procName, 1);

    pix->spp = spp;
    return 0;
}


/*!
 *  pixCopySpp()
 *
 *      Input:  pixd
 *              pixs
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixCopySpp(PIX  *pixd,
           PIX  *pixs)
{
    PROCNAME("pixCopySpp");

    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixs == pixd)
        return 0;   /* no-op */

    pixSetSpp(pixd, pixGetSpp(pixs));
    return 0;
}


l_int32
pixGetWpl(PIX  *pix)
{
    PROCNAME("pixGetWpl");

    if (!pix)
        return ERROR_INT("pix not defined", procName, UNDEF);
    return pix->wpl;
}


l_int32
pixSetWpl(PIX     *pix,
          l_int32  wpl)
{
    PROCNAME("pixSetWpl");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pix->wpl = wpl;
    return 0;
}


l_int32
pixGetRefcount(PIX  *pix)
{
    PROCNAME("pixGetRefcount");

    if (!pix)
        return ERROR_INT("pix not defined", procName, UNDEF);
    return pix->refcount;
}


l_int32
pixChangeRefcount(PIX     *pix,
                  l_int32  delta)
{
    PROCNAME("pixChangeRefcount");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pix->refcount += delta;
    return 0;
}


l_int32
pixGetXRes(PIX  *pix)
{
    PROCNAME("pixGetXRes");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 0);
    return pix->xres;
}


l_int32
pixSetXRes(PIX     *pix,
           l_int32  res)
{
    PROCNAME("pixSetXRes");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pix->xres = res;
    return 0;
}


l_int32
pixGetYRes(PIX  *pix)
{
    PROCNAME("pixGetYRes");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 0);
    return pix->yres;
}


l_int32
pixSetYRes(PIX     *pix,
           l_int32  res)
{
    PROCNAME("pixSetYRes");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pix->yres = res;
    return 0;
}


/*!
 *  pixGetResolution()
 *
 *      Input:  pix
 *              &xres, &yres (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixGetResolution(PIX      *pix,
                 l_int32  *pxres,
                 l_int32  *pyres)
{
    PROCNAME("pixGetResolution");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (pxres) *pxres = pix->xres;
    if (pyres) *pyres = pix->yres;
    return 0;
}


/*!
 *  pixSetResolution()
 *
 *      Input:  pix
 *              xres, yres (use 0 to skip the setting for either of these)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixSetResolution(PIX     *pix,
                 l_int32  xres,
                 l_int32  yres)
{
    PROCNAME("pixSetResolution");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (xres > 0) pix->xres = xres;
    if (yres > 0) pix->yres = yres;
    return 0;
}


l_int32
pixCopyResolution(PIX  *pixd,
                  PIX  *pixs)
{
    PROCNAME("pixCopyResolution");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (pixs == pixd)
        return 0;   /* no-op */

    pixSetXRes(pixd, pixGetXRes(pixs));
    pixSetYRes(pixd, pixGetYRes(pixs));
    return 0;
}


l_int32
pixScaleResolution(PIX       *pix,
                   l_float32  xscale,
                   l_float32  yscale)
{
    PROCNAME("pixScaleResolution");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    if (pix->xres != 0 && pix->yres != 0) {
        pix->xres = (l_uint32)(xscale * (l_float32)(pix->xres) + 0.5);
        pix->yres = (l_uint32)(yscale * (l_float32)(pix->yres) + 0.5);
    }
    return 0;
}


l_int32
pixGetInputFormat(PIX  *pix)
{
    PROCNAME("pixGetInputFormat");

    if (!pix)
        return ERROR_INT("pix not defined", procName, UNDEF);
    return pix->informat;
}


l_int32
pixSetInputFormat(PIX     *pix,
                  l_int32  informat)
{
    PROCNAME("pixSetInputFormat");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pix->informat = informat;
    return 0;
}


l_int32
pixCopyInputFormat(PIX  *pixd,
                   PIX  *pixs)
{
    PROCNAME("pixCopyInputFormat");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (pixs == pixd)
        return 0;   /* no-op */

    pixSetInputFormat(pixd, pixGetInputFormat(pixs));
    return 0;
}


/*!
 *  pixGetText()
 *
 *      Input:  pix
 *      Return: ptr to existing text string
 *
 *  Notes:
 *      (1) The text string belongs to the pix.  The caller must
 *          NOT free it!
 */
char *
pixGetText(PIX  *pix)
{
    PROCNAME("pixGetText");

    if (!pix)
        return (char *)ERROR_PTR("pix not defined", procName, NULL);
    return pix->text;
}


/*!
 *  pixSetText()
 *
 *      Input:  pix
 *              textstring (can be null)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This removes any existing textstring and puts a copy of
 *          the input textstring there.
 */
l_int32
pixSetText(PIX         *pix,
           const char  *textstring)
{
    PROCNAME("pixSetText");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    stringReplace(&pix->text, textstring);
    return 0;
}


/*!
 *  pixAddText()
 *
 *      Input:  pix
 *              textstring
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This adds the new textstring to any existing text.
 *      (2) Either or both the existing text and the new text
 *          string can be null.
 */
l_int32
pixAddText(PIX         *pix,
           const char  *textstring)
{
char  *newstring;

    PROCNAME("pixAddText");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    newstring = stringJoin(pixGetText(pix), textstring);
    stringReplace(&pix->text, newstring);
    FREE(newstring);
    return 0;
}


l_int32
pixCopyText(PIX  *pixd,
            PIX  *pixs)
{
    PROCNAME("pixCopyText");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (pixs == pixd)
        return 0;   /* no-op */

    pixSetText(pixd, pixGetText(pixs));
    return 0;
}


PIXCMAP *
pixGetColormap(PIX  *pix)
{
    PROCNAME("pixGetColormap");

    if (!pix)
        return (PIXCMAP *)ERROR_PTR("pix not defined", procName, NULL);
    return pix->colormap;
}


/*!
 *  pixSetColormap()
 *
 *      Input:  pix
 *              colormap (to be assigned)
 *      Return: 0 if OK, 1 on error.
 *
 *  Notes:
 *      (1) Unlike with the pix data field, pixSetColormap() destroys
 *          any existing colormap before assigning the new one.
 *          Because colormaps are not ref counted, it is important that
 *          the new colormap does not belong to any other pix.
 */
l_int32
pixSetColormap(PIX      *pix,
               PIXCMAP  *colormap)
{
    PROCNAME("pixSetColormap");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pixDestroyColormap(pix);
    pix->colormap = colormap;
    return 0;
}


/*!
 *  pixDestroyColormap()
 *
 *      Input:  pix
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixDestroyColormap(PIX  *pix)
{
PIXCMAP  *cmap;

    PROCNAME("pixDestroyColormap");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    if ((cmap = pix->colormap) != NULL) {
        pixcmapDestroy(&cmap);
        pix->colormap = NULL;
    }
    return 0;
}


/*!
 *  pixGetData()
 *
 *  Notes:
 *      (1) This gives a new handle for the data.  The data is still
 *          owned by the pix, so do not call FREE() on it.
 */
l_uint32 *
pixGetData(PIX  *pix)
{
    PROCNAME("pixGetData");

    if (!pix)
        return (l_uint32 *)ERROR_PTR("pix not defined", procName, NULL);
    return pix->data;
}


/*!
 *  pixSetData()
 *
 *  Notes:
 *      (1) This does not free any existing data.  To free existing
 *          data, use pixFreeData() before pixSetData().
 */
l_int32
pixSetData(PIX       *pix,
           l_uint32  *data)
{
    PROCNAME("pixSetData");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    pix->data = data;
    return 0;
}


/*!
 *  pixExtractData()
 *
 *  Notes:
 *      (1) This extracts the pix image data for use in another context.
 *          The caller still needs to use pixDestroy() on the input pix.
 *      (2) If refcount == 1, the data is extracted and the
 *          pix->data ptr is set to NULL.
 *      (3) If refcount > 1, this simply returns a copy of the data,
 *          using the pix allocator, and leaving the input pix unchanged.
 */
l_uint32 *
pixExtractData(PIX  *pixs)
{
l_int32    count, bytes;
l_uint32  *data, *datas;

    PROCNAME("pixExtractData");

    if (!pixs)
        return (l_uint32 *)ERROR_PTR("pixs not defined", procName, NULL);

    count = pixGetRefcount(pixs);
    if (count == 1) {  /* extract */
        data = pixGetData(pixs);
        pixSetData(pixs, NULL);
    } else {  /* refcount > 1; copy */
        bytes = 4 * pixGetWpl(pixs) * pixGetHeight(pixs);
        datas = pixGetData(pixs);
        if ((data = (l_uint32 *)pix_malloc(bytes)) == NULL)
            return (l_uint32 *)ERROR_PTR("data not made", procName, NULL);
        memcpy((char *)data, (char *)datas, bytes);
    }

    return data;
}


/*!
 *  pixFreeData()
 *
 *  Notes:
 *      (1) This frees the data and sets the pix data ptr to null.
 *          It should be used before pixSetData() in the situation where
 *          you want to free any existing data before doing
 *          a subsequent assignment with pixSetData().
 */
l_int32
pixFreeData(PIX  *pix)
{
l_uint32  *data;

    PROCNAME("pixFreeData");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    if ((data = pixGetData(pix)) != NULL) {
        pix_free(data);
        pix->data = NULL;
    }
    return 0;
}


/*--------------------------------------------------------------------*
 *                          Pix line ptrs                             *
 *--------------------------------------------------------------------*/
/*!
 *  pixGetLinePtrs()
 *
 *      Input:  pix
 *              &size (<optional return> array size, which is the pix height)
 *      Return: array of line ptrs, or null on error
 *
 *  Notes:
 *      (1) This is intended to be used for fast random pixel access.
 *          For example, for an 8 bpp image,
 *              val = GET_DATA_BYTE(lines8[i], j);
 *          is equivalent to, but much faster than,
 *              pixGetPixel(pix, j, i, &val);
 *      (2) How much faster?  For 1 bpp, it's from 6 to 10x faster.
 *          For 8 bpp, it's an amazing 30x faster.  So if you are
 *          doing random access over a substantial part of the image,
 *          use this line ptr array.
 *      (3) When random access is used in conjunction with a stack,
 *          queue or heap, the overall computation time depends on
 *          the operations performed on each struct that is popped
 *          or pushed, and whether we are using a priority queue (O(logn))
 *          or a queue or stack (O(1)).  For example, for maze search,
 *          the overall ratio of time for line ptrs vs. pixGet/Set* is
 *             Maze type     Type                   Time ratio
 *               binary      queue                     0.4
 *               gray        heap (priority queue)     0.6
 *      (4) Because this returns a void** and the accessors take void*,
 *          the compiler cannot check the pointer types.  It is
 *          strongly recommended that you adopt a naming scheme for
 *          the returned ptr arrays that indicates the pixel depth.
 *          (This follows the original intent of Simonyi's "Hungarian"
 *          application notation, where naming is used proactively
 *          to make errors visibly obvious.)  By doing this, you can
 *          tell by inspection if the correct accessor is used.
 *          For example, for an 8 bpp pixg:
 *              void **lineg8 = pixGetLinePtrs(pixg, NULL);
 *              val = GET_DATA_BYTE(lineg8[i], j);  // fast access; BYTE, 8
 *              ...
 *              FREE(lineg8);  // don't forget this
 *      (5) These are convenient for accessing bytes sequentially in an
 *          8 bpp grayscale image.  People who write image processing code
 *          on 8 bpp images are accustomed to grabbing pixels directly out
 *          of the raster array.  Note that for little endians, you first
 *          need to reverse the byte order in each 32-bit word.
 *          Here's a typical usage pattern:
 *              pixEndianByteSwap(pix);   // always safe; no-op on big-endians
 *              l_uint8 **lineptrs = (l_uint8 **)pixGetLinePtrs(pix, NULL);
 *              pixGetDimensions(pix, &w, &h, NULL);
 *              for (i = 0; i < h; i++) {
 *                  l_uint8 *line = lineptrs[i];
 *                  for (j = 0; j < w; j++) {
 *                      val = line[j];
 *                      ...
 *                  }
 *              }
 *              pixEndianByteSwap(pix);  // restore big-endian order
 *              FREE(lineptrs);
 *          This can be done even more simply as follows:
 *              l_uint8 **lineptrs = pixSetupByteProcessing(pix, &w, &h);
 *              for (i = 0; i < h; i++) {
 *                  l_uint8 *line = lineptrs[i];
 *                  for (j = 0; j < w; j++) {
 *                      val = line[j];
 *                      ...
 *                  }
 *              }
 *              pixCleanupByteProcessing(pix, lineptrs);
 */
void **
pixGetLinePtrs(PIX      *pix,
               l_int32  *psize)
{
l_int32    i, h, wpl;
l_uint32  *data;
void     **lines;

    PROCNAME("pixGetLinePtrs");

    if (!pix)
        return (void **)ERROR_PTR("pix not defined", procName, NULL);

    h = pixGetHeight(pix);
    if (psize) *psize = h;
    if ((lines = (void **)CALLOC(h, sizeof(void *))) == NULL)
        return (void **)ERROR_PTR("lines not made", procName, NULL);
    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    for (i = 0; i < h; i++)
        lines[i] = (void *)(data + i * wpl);

    return lines;
}


/*--------------------------------------------------------------------*
 *                    Print output for debugging                      *
 *--------------------------------------------------------------------*/
extern const char *ImageFileFormatExtensions[];

/*!
 *  pixPrintStreamInfo()
 *
 *      Input:  fp (file stream)
 *              pix
 *              text (<optional> identifying string; can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixPrintStreamInfo(FILE        *fp,
                   PIX         *pix,
                   const char  *text)
{
l_int32   informat;
PIXCMAP  *cmap;

    PROCNAME("pixPrintStreamInfo");

    if (!fp)
        return ERROR_INT("fp not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    if (text)
        fprintf(fp, "  Pix Info for %s:\n", text);
    fprintf(fp, "    width = %d, height = %d, depth = %d, spp = %d\n",
               pixGetWidth(pix), pixGetHeight(pix), pixGetDepth(pix),
               pixGetSpp(pix));
    fprintf(fp, "    wpl = %d, data = %p, refcount = %d\n",
               pixGetWpl(pix), pixGetData(pix), pixGetRefcount(pix));
    if ((cmap = pixGetColormap(pix)) != NULL)
        pixcmapWriteStream(fp, cmap);
    else
        fprintf(fp, "    no colormap\n");
    informat = pixGetInputFormat(pix);
    fprintf(fp, "    input format: %d (%s)\n", informat,
            ImageFileFormatExtensions[informat]);

    return 0;
}
