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
 *   pixcomp.c
 *
 *      Pixcomp creation and destruction
 *           PIXC     *pixcompCreateFromPix()
 *           PIXC     *pixcompCreateFromString()
 *           PIXC     *pixcompCreateFromFile()
 *           void      pixcompDestroy()

 *      Pixcomp accessors
 *           l_int32   pixcompGetDimensions()
 *           l_int32   pixcompDetermineFormat()
 *
 *      Pixcomp conversion to Pix
 *           PIX      *pixCreateFromPixcomp()
 *
 *      Pixacomp creation and destruction
 *           PIXAC    *pixacompCreate()
 *           PIXAC    *pixacompCreateWithInit()
 *           PIXAC    *pixacompCreateFromPixa()
 *           PIXAC    *pixacompCreateFromFiles()
 *           PIXAC    *pixacompCreateFromSA()
 *           void      pixacompDestroy()
 *
 *      Pixacomp addition/replacement
 *           l_int32   pixacompAddPix()
 *           l_int32   pixacompAddPixcomp()
 *           static l_int32  pixacompExtendArray()
 *           l_int32   pixacompReplacePix()
 *           l_int32   pixacompReplacePixcomp()
 *           l_int32   pixacompAddBox()
 *
 *      Pixacomp accessors
 *           l_int32   pixacompGetCount()
 *           PIXC     *pixacompGetPixcomp()
 *           PIX      *pixacompGetPix()
 *           l_int32   pixacompGetPixDimensions()
 *           BOXA     *pixacompGetBoxa()
 *           l_int32   pixacompGetBoxaCount()
 *           BOX      *pixacompGetBox()
 *           l_int32   pixacompGetBoxGeometry()
 *           l_int32   pixacompGetOffset()
 *           l_int32   pixacompSetOffset()
 *
 *      Pixacomp conversion to Pixa
 *           PIXA     *pixaCreateFromPixacomp()
 *
 *      Pixacomp serialized I/O
 *           PIXAC    *pixacompRead()
 *           PIXAC    *pixacompReadStream()
 *           l_int32   pixacompWrite()
 *           l_int32   pixacompWriteStream()
 *
 *      Conversion to pdf
 *           l_int32   pixacompConvertToPdf()
 *           l_int32   pixacompConvertToPdfData()
 *
 *      Output for debugging
 *           l_int32   pixacompWriteStreamInfo()
 *           l_int32   pixcompWriteStreamInfo()
 *           PIX      *pixacompDisplayTiledAndScaled()
 *
 *   The Pixacomp is an array of Pixcomp, where each Pixcomp is a compressed
 *   string of the image.  We don't use reference counting here.
 *   The basic application is to allow a large array of highly
 *   compressible images to reside in memory.  We purposely don't
 *   reuse the Pixa for this, to avoid confusion and programming errors.
 *
 *   Three compression formats are used: g4, png and jpeg.
 *   The compression type can be either specified or defaulted.
 *   If specified and it is not possible to compress (for example,
 *   you specify a jpeg on a 1 bpp image or one with a colormap),
 *   the compression type defaults to png.
 *
 *   The serialized version of the Pixacomp is similar to that for
 *   a Pixa, except that each Pixcomp can be compressed by one of
 *   tiffg4, png, or jpeg.  Unlike serialization of the Pixa,
 *   serialization of the Pixacomp does not require any imaging
 *   libraries because it simply reads and writes the compressed data.
 *
 *   There are two modes of use in accumulating images:
 *     (1) addition to the end of the array
 *     (2) random insertion (replacement) into the array
 *
 *   In use, we assume that the array is fully populated up to the
 *   index value (n - 1), where n is the value of the pixcomp field n.
 *   Addition can only be made to the end of the fully populated array,
 *   at the index value n.  Insertion can be made randomly, but again
 *   only within the array of pixcomps; i.e., within the set of
 *   indices {0 .... n-1}.  The functions are pixacompReplacePix()
 *   and pixacompReplacePixcomp(), and they destroy the existing pixcomp.
 *
 *   For addition to the end of the array, use pixacompCreate(), which
 *   generates an initially empty array of pixcomps.  For random
 *   insertion and replacement of pixcomp into a pixacomp,
 *   initialize a fully populated array using pixacompCreateWithInit().
 *
 *   The offset field allows you to use an offset-based index to
 *   access the 0-based ptr array in the pixacomp.  This would typically
 *   be used to map the pixacomp array index to a page number, or v.v.
 *   By default, the offset is 0.  For example, suppose you have 50 images,
 *   corresponding to page numbers 10 - 59.  Then you would use
 *      pixac = pixacompCreateWithInit(50, 10, ...);
 *   This would allocate an array of 50 pixcomps, but if you asked for
 *   the pix at index 10, using pixacompGetPix(pixac, 10), it would
 *   apply the offset internally, returning the pix at index 0 in the array.
 */

#include <string.h>
#include "allheaders.h"

static const l_int32  INITIAL_PTR_ARRAYSIZE = 20;   /* n'import quoi */

    /* These two globals are defined in writefile.c */
extern l_int32 NumImageFileFormatExtensions;
extern const char *ImageFileFormatExtensions[];

    /* Static function */
static l_int32 pixacompExtendArray(PIXAC *pixac);


/*---------------------------------------------------------------------*
 *                  Pixcomp creation and destruction                   *
 *---------------------------------------------------------------------*/
/*!
 *  pixcompCreateFromPix()
 *
 *      Input:  pix
 *              comptype (IFF_DEFAULT, IFF_TIFF_G4, IFF_PNG, IFF_JFIF_JPEG)
 *      Return: pixc, or null on error
 *
 *  Notes:
 *      (1) Use @comptype == IFF_DEFAULT to have the compression
 *          type automatically determined.
 */
PIXC *
pixcompCreateFromPix(PIX     *pix,
                     l_int32  comptype)
{
size_t    size;
char     *text;
l_int32   ret, format;
l_uint8  *data;
PIXC     *pixc;

    PROCNAME("pixcompCreateFromPix");

    if (!pix)
        return (PIXC *)ERROR_PTR("pix not defined", procName, NULL);
    if (comptype != IFF_DEFAULT && comptype != IFF_TIFF_G4 &&
        comptype != IFF_PNG && comptype != IFF_JFIF_JPEG)
        return (PIXC *)ERROR_PTR("invalid comptype", procName, NULL);

    if ((pixc = (PIXC *)LEPT_CALLOC(1, sizeof(PIXC))) == NULL)
        return (PIXC *)ERROR_PTR("pixc not made", procName, NULL);
    pixGetDimensions(pix, &pixc->w, &pixc->h, &pixc->d);
    pixGetResolution(pix, &pixc->xres, &pixc->yres);
    if (pixGetColormap(pix))
        pixc->cmapflag = 1;
    if ((text = pixGetText(pix)) != NULL)
        pixc->text = stringNew(text);

    pixcompDetermineFormat(comptype, pixc->d, pixc->cmapflag, &format);
    pixc->comptype = format;
    ret = pixWriteMem(&data, &size, pix, format);
    if (ret) {
        L_ERROR("write to memory failed\n", procName);
        pixcompDestroy(&pixc);
        return NULL;
    }
    pixc->data = data;
    pixc->size = size;

    return pixc;
}


/*!
 *  pixcompCreateFromString()
 *
 *      Input:  data (compressed string)
 *              size (number of bytes)
 *              copyflag (L_INSERT or L_COPY)
 *      Return: pixc, or null on error
 *
 *  Notes:
 *      (1) This works when the compressed string is png, jpeg or tiffg4.
 *      (2) The copyflag determines if the data in the new Pixcomp is
 *          a copy of the input data.
 */
PIXC *
pixcompCreateFromString(l_uint8  *data,
                        size_t    size,
                        l_int32   copyflag)
{
l_int32  format, w, h, d, bps, spp, iscmap;
PIXC    *pixc;

    PROCNAME("pixcompCreateFromString");

    if (!data)
        return (PIXC *)ERROR_PTR("data not defined", procName, NULL);
    if (copyflag != L_INSERT && copyflag != L_COPY)
        return (PIXC *)ERROR_PTR("invalid copyflag", procName, NULL);

    if (pixReadHeaderMem(data, size, &format, &w, &h, &bps, &spp, &iscmap) == 1)
        return (PIXC *)ERROR_PTR("header data not read", procName, NULL);
    if ((pixc = (PIXC *)LEPT_CALLOC(1, sizeof(PIXC))) == NULL)
        return (PIXC *)ERROR_PTR("pixc not made", procName, NULL);
    d = (spp == 3) ? 32 : bps * spp;
    pixc->w = w;
    pixc->h = h;
    pixc->d = d;
    pixc->comptype = format;
    pixc->cmapflag = iscmap;
    if (copyflag == L_INSERT)
        pixc->data = data;
    else
        pixc->data = l_binaryCopy(data, size);
    pixc->size = size;
    return pixc;
}


/*!
 *  pixcompCreateFromFile()
 *
 *      Input:  filename
 *              comptype (IFF_DEFAULT, IFF_TIFF_G4, IFF_PNG, IFF_JFIF_JPEG)
 *      Return: pixc, or null on error
 *
 *  Notes:
 *      (1) Use @comptype == IFF_DEFAULT to have the compression
 *          type automatically determined.
 *      (2) If the comptype is invalid for this file, the default will
 *          be substituted.
 */
PIXC *
pixcompCreateFromFile(const char  *filename,
                      l_int32      comptype)
{
l_int32   format;
size_t    nbytes;
l_uint8  *data;
PIX      *pix;
PIXC     *pixc;

    PROCNAME("pixcompCreateFromFile");

    if (!filename)
        return (PIXC *)ERROR_PTR("filename not defined", procName, NULL);
    if (comptype != IFF_DEFAULT && comptype != IFF_TIFF_G4 &&
        comptype != IFF_PNG && comptype != IFF_JFIF_JPEG)
        return (PIXC *)ERROR_PTR("invalid comptype", procName, NULL);

    findFileFormat(filename, &format);
    if (format == IFF_UNKNOWN) {
        L_ERROR("unreadable file: %s\n", procName, filename);
        return NULL;
    }

        /* Can we accept the encoded file directly?  Remember that
         * png is the "universal" compression type, so if requested
         * it takes precedence.  Otherwise, if the file is already
         * compressed in g4 or jpeg, just accept the string. */
    if ((format == IFF_TIFF_G4 && comptype != IFF_PNG) ||
        (format == IFF_JFIF_JPEG && comptype != IFF_PNG))
        comptype = format;
    if (comptype != IFF_DEFAULT && comptype == format) {
        data = l_binaryRead(filename, &nbytes);
        if ((pixc = pixcompCreateFromString(data, nbytes, L_INSERT)) == NULL) {
            LEPT_FREE(data);
            return (PIXC *)ERROR_PTR("pixc not made (string)", procName, NULL);
        }
        return pixc;
    }

        /* Need to recompress in the default format */
    if ((pix = pixRead(filename)) == NULL)
        return (PIXC *)ERROR_PTR("pix not read", procName, NULL);
    if ((pixc = pixcompCreateFromPix(pix, comptype)) == NULL) {
        pixDestroy(&pix);
        return (PIXC *)ERROR_PTR("pixc not made", procName, NULL);
    }
    pixDestroy(&pix);
    return pixc;
}


/*!
 *  pixcompDestroy()
 *
 *      Input:  &pixc <will be nulled>
 *      Return: void
 *
 *  Notes:
 *      (1) Always nulls the input ptr.
 */
void
pixcompDestroy(PIXC  **ppixc)
{
PIXC  *pixc;

    PROCNAME("pixcompDestroy");

    if (!ppixc) {
        L_WARNING("ptr address is null!\n", procName);
        return;
    }

    if ((pixc = *ppixc) == NULL)
        return;

    LEPT_FREE(pixc->data);
    if (pixc->text)
        LEPT_FREE(pixc->text);
    LEPT_FREE(pixc);
    *ppixc = NULL;
    return;
}


/*---------------------------------------------------------------------*
 *                           Pixcomp accessors                         *
 *---------------------------------------------------------------------*/
/*!
 *  pixcompGetDimensions()
 *
 *      Input:  pixc
 *              &w, &h, &d (<optional return>)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixcompGetDimensions(PIXC     *pixc,
                     l_int32  *pw,
                     l_int32  *ph,
                     l_int32  *pd)
{
    PROCNAME("pixcompGetDimensions");

    if (!pixc)
        return ERROR_INT("pixc not defined", procName, 1);
    if (pw) *pw = pixc->w;
    if (ph) *ph = pixc->h;
    if (pd) *pd = pixc->d;
    return 0;
}


/*!
 *  pixcompDetermineFormat()
 *
 *      Input:  comptype (IFF_DEFAULT, IFF_TIFF_G4, IFF_PNG, IFF_JFIF_JPEG)
 *              d (pix depth)
 *              cmapflag (1 if pix to be compressed as a colormap; 0 otherwise)
 *              &format (return IFF_TIFF, IFF_PNG or IFF_JFIF_JPEG)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This determines the best format for a pix, given both
 *          the request (@comptype) and the image characteristics.
 *      (2) If @comptype == IFF_DEFAULT, this does not necessarily result
 *          in png encoding.  Instead, it returns one of the three formats
 *          that is both valid and most likely to give best compression.
 *      (3) If the pix cannot be compressed by the input value of
 *          @comptype, this selects IFF_PNG, which can compress all pix.
 */
l_int32
pixcompDetermineFormat(l_int32   comptype,
                       l_int32   d,
                       l_int32   cmapflag,
                       l_int32  *pformat)
{

    PROCNAME("pixcompDetermineFormat");

    if (!pformat)
        return ERROR_INT("&format not defined", procName, 1);
    *pformat = IFF_PNG;  /* init value and default */
    if (comptype != IFF_DEFAULT && comptype != IFF_TIFF_G4 &&
        comptype != IFF_PNG && comptype != IFF_JFIF_JPEG)
        return ERROR_INT("invalid comptype", procName, 1);

    if (comptype == IFF_DEFAULT) {
        if (d == 1)
            *pformat = IFF_TIFF_G4;
        else if (d == 16)
            *pformat = IFF_PNG;
        else if (d >= 8 && !cmapflag)
            *pformat = IFF_JFIF_JPEG;
    } else if (comptype == IFF_TIFF_G4 && d == 1) {
        *pformat = IFF_TIFF_G4;
    } else if (comptype == IFF_JFIF_JPEG && d >= 8 && !cmapflag) {
        *pformat = IFF_JFIF_JPEG;
    }

    return 0;
}


/*---------------------------------------------------------------------*
 *                      Pixcomp conversion to Pix                      *
 *---------------------------------------------------------------------*/
/*!
 *  pixCreateFromPixcomp()
 *
 *      Input:  pixc
 *      Return: pix, or null on error
 */
PIX *
pixCreateFromPixcomp(PIXC  *pixc)
{
l_int32  w, h, d, cmapinpix, format;
PIX     *pix;

    PROCNAME("pixCreateFromPixcomp");

    if (!pixc)
        return (PIX *)ERROR_PTR("pixc not defined", procName, NULL);

    if ((pix = pixReadMem(pixc->data, pixc->size)) == NULL)
        return (PIX *)ERROR_PTR("pix not read", procName, NULL);
    pixSetResolution(pix, pixc->xres, pixc->yres);
    if (pixc->text)
        pixSetText(pix, pixc->text);

        /* Check fields for consistency */
    pixGetDimensions(pix, &w, &h, &d);
    if (pixc->w != w) {
        L_INFO("pix width %d != pixc width %d\n", procName, w, pixc->w);
        L_ERROR("pix width %d != pixc width\n", procName, w);
    }
    if (pixc->h != h)
        L_ERROR("pix height %d != pixc height\n", procName, h);
    if (pixc->d != d) {
        if (pixc->d == 16)  /* we strip 16 --> 8 bpp by default */
            L_WARNING("pix depth %d != pixc depth 16\n", procName, d);
        else
            L_ERROR("pix depth %d != pixc depth\n", procName, d);
    }
    cmapinpix = (pixGetColormap(pix) != NULL);
    if ((cmapinpix && !pixc->cmapflag) || (!cmapinpix && pixc->cmapflag))
        L_ERROR("pix cmap flag inconsistent\n", procName);
    format = pixGetInputFormat(pix);
    if (format != pixc->comptype) {
        L_ERROR("pix comptype %d not equal to pixc comptype\n",
                    procName, format);
    }

    return pix;
}


/*---------------------------------------------------------------------*
 *                Pixacomp creation and destruction                    *
 *---------------------------------------------------------------------*/
/*!
 *  pixacompCreate()
 *
 *      Input:  n  (initial number of ptrs)
 *      Return: pixac, or null on error
 */
PIXAC *
pixacompCreate(l_int32  n)
{
PIXAC  *pixac;

    PROCNAME("pixacompCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((pixac = (PIXAC *)LEPT_CALLOC(1, sizeof(PIXAC))) == NULL)
        return (PIXAC *)ERROR_PTR("pixac not made", procName, NULL);
    pixac->n = 0;
    pixac->nalloc = n;
    pixac->offset = 0;

    if ((pixac->pixc = (PIXC **)LEPT_CALLOC(n, sizeof(PIXC *))) == NULL)
        return (PIXAC *)ERROR_PTR("pixc ptrs not made", procName, NULL);
    if ((pixac->boxa = boxaCreate(n)) == NULL)
        return (PIXAC *)ERROR_PTR("boxa not made", procName, NULL);

    return pixac;
}


/*!
 *  pixacompCreateWithInit()
 *
 *      Input:  n  (initial number of ptrs)
 *              offset (difference: accessor index - pixacomp array index)
 *              pix (<optional> initialize each ptr in pixacomp to this pix;
 *                   can be NULL)
 *              comptype (IFF_DEFAULT, IFF_TIFF_G4, IFF_PNG, IFF_JFIF_JPEG)
 *      Return: pixac, or null on error
 *
 *  Notes:
 *      (1) Initializes a pixacomp to be fully populated with @pix,
 *          compressed using @comptype.  If @pix == NULL, @comptype
 *          is ignored.
 *      (2) Typically, the array is initialized with a tiny pix.
 *          This is most easily done by setting @pix == NULL, causing
 *          initialization of each array element with a tiny placeholder
 *          pix (w = h = d = 1), using comptype = IFF_TIFF_G4 .
 *      (3) Example usage:
 *            // Generate pixacomp for pages 30 - 49.  This has an array
 *            // size of 20 and the page number offset is 30.
 *            PixaComp *pixac = pixacompCreateWithInit(20, 30, NULL,
 *                                                     IFF_TIFF_G4);
 *            // Now insert png-compressed images into the initialized array
 *            for (pageno = 30; pageno < 50; pageno++) {
 *                Pix *pixt = ...   // derived from image[pageno]
 *                if (pixt)
 *                    pixacompReplacePix(pixac, pageno, pixt, IFF_PNG);
 *                pixDestroy(&pixt);
 *            }
 *          The result is a pixac with 20 compressed strings, and with
 *          selected pixt replacing the placeholders.
 *          To extract the image for page 38, which is decompressed
 *          from element 8 in the array, use:
 *            pixt = pixacompGetPix(pixac, 38);
 */
PIXAC *
pixacompCreateWithInit(l_int32  n,
                       l_int32  offset,
                       PIX     *pix,
                       l_int32  comptype)
{
l_int32  i;
PIX     *pixt;
PIXC    *pixc;
PIXAC   *pixac;

    PROCNAME("pixacompCreateWithInit");

    if (n <= 0)
        return (PIXAC *)ERROR_PTR("n must be > 0", procName, NULL);
    if (pix) {
        if (comptype != IFF_DEFAULT && comptype != IFF_TIFF_G4 &&
            comptype != IFF_PNG && comptype != IFF_JFIF_JPEG)
            return (PIXAC *)ERROR_PTR("invalid comptype", procName, NULL);
    } else {
        comptype = IFF_TIFF_G4;
    }
    if (offset < 0) {
        L_WARNING("offset < 0; setting to 0\n", procName);
        offset = 0;
    }

    if ((pixac = pixacompCreate(n)) == NULL)
        return (PIXAC *)ERROR_PTR("pixac not made", procName, NULL);
    pixacompSetOffset(pixac, offset);
    if (pix)
        pixt = pixClone(pix);
    else
        pixt = pixCreate(1, 1, 1);
    for (i = 0; i < n; i++) {
        pixc = pixcompCreateFromPix(pixt, comptype);
        pixacompAddPixcomp(pixac, pixc);
    }
    pixDestroy(&pixt);

    return pixac;
}


/*!
 *  pixacompCreateFromPixa()
 *
 *      Input:  pixa
 *              comptype (IFF_DEFAULT, IFF_TIFF_G4, IFF_PNG, IFF_JFIF_JPEG)
 *              accesstype (L_COPY, L_CLONE, L_COPY_CLONE)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If @format == IFF_DEFAULT, the conversion format for each
 *          image is chosen automatically.  Otherwise, we use the
 *          specified format unless it can't be done (e.g., jpeg
 *          for a 1, 2 or 4 bpp pix, or a pix with a colormap),
 *          in which case we use the default (assumed best) compression.
 *      (2) @accesstype is used to extract a boxa from @pixa.
 */
PIXAC *
pixacompCreateFromPixa(PIXA    *pixa,
                       l_int32  comptype,
                       l_int32  accesstype)
{
l_int32  i, n;
BOXA    *boxa;
PIX     *pix;
PIXAC   *pixac;

    PROCNAME("pixacompCreateFromPixa");

    if (!pixa)
        return (PIXAC *)ERROR_PTR("pixa not defined", procName, NULL);
    if (comptype != IFF_DEFAULT && comptype != IFF_TIFF_G4 &&
        comptype != IFF_PNG && comptype != IFF_JFIF_JPEG)
        return (PIXAC *)ERROR_PTR("invalid comptype", procName, NULL);
    if (accesstype != L_COPY && accesstype != L_CLONE &&
        accesstype != L_COPY_CLONE)
        return (PIXAC *)ERROR_PTR("invalid accesstype", procName, NULL);

    n = pixaGetCount(pixa);
    if ((pixac = pixacompCreate(n)) == NULL)
        return (PIXAC *)ERROR_PTR("pixac not made", procName, NULL);
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
        pixacompAddPix(pixac, pix, comptype);
        pixDestroy(&pix);
    }
    if ((boxa = pixaGetBoxa(pixa, accesstype)) != NULL) {
        if (pixac->boxa) {
            boxaDestroy(&pixac->boxa);
            pixac->boxa = boxa;
        }
    }

    return pixac;
}


/*!
 *  pixacompCreateFromFiles()
 *
 *      Input:  dirname
 *              substr (<optional> substring filter on filenames; can be null)
 *              comptype (IFF_DEFAULT, IFF_TIFF_G4, IFF_PNG, IFF_JFIF_JPEG)
 *      Return: pixac, or null on error
 *
 *  Notes:
 *      (1) @dirname is the full path for the directory.
 *      (2) @substr is the part of the file name (excluding
 *          the directory) that is to be matched.  All matching
 *          filenames are read into the Pixa.  If substr is NULL,
 *          all filenames are read into the Pixa.
 *      (3) Use @comptype == IFF_DEFAULT to have the compression
 *          type automatically determined for each file.
 *      (4) If the comptype is invalid for a file, the default will
 *          be substituted.
 */
PIXAC *
pixacompCreateFromFiles(const char  *dirname,
                        const char  *substr,
                        l_int32      comptype)
{
PIXAC    *pixac;
SARRAY   *sa;

    PROCNAME("pixacompCreateFromFiles");

    if (!dirname)
        return (PIXAC *)ERROR_PTR("dirname not defined", procName, NULL);
    if (comptype != IFF_DEFAULT && comptype != IFF_TIFF_G4 &&
        comptype != IFF_PNG && comptype != IFF_JFIF_JPEG)
        return (PIXAC *)ERROR_PTR("invalid comptype", procName, NULL);

    if ((sa = getSortedPathnamesInDirectory(dirname, substr, 0, 0)) == NULL)
        return (PIXAC *)ERROR_PTR("sa not made", procName, NULL);
    pixac = pixacompCreateFromSA(sa, comptype);
    sarrayDestroy(&sa);
    return pixac;
}


/*!
 *  pixacompCreateFromSA()
 *
 *      Input:  sarray (full pathnames for all files)
 *              comptype (IFF_DEFAULT, IFF_TIFF_G4, IFF_PNG, IFF_JFIF_JPEG)
 *      Return: pixac, or null on error
 *
 *  Notes:
 *      (1) Use @comptype == IFF_DEFAULT to have the compression
 *          type automatically determined for each file.
 *      (2) If the comptype is invalid for a file, the default will
 *          be substituted.
 */
PIXAC *
pixacompCreateFromSA(SARRAY  *sa,
                     l_int32  comptype)
{
char    *str;
l_int32  i, n;
PIXC    *pixc;
PIXAC   *pixac;

    PROCNAME("pixacompCreateFromSA");

    if (!sa)
        return (PIXAC *)ERROR_PTR("sarray not defined", procName, NULL);
    if (comptype != IFF_DEFAULT && comptype != IFF_TIFF_G4 &&
        comptype != IFF_PNG && comptype != IFF_JFIF_JPEG)
        return (PIXAC *)ERROR_PTR("invalid comptype", procName, NULL);

    n = sarrayGetCount(sa);
    pixac = pixacompCreate(n);
    for (i = 0; i < n; i++) {
        str = sarrayGetString(sa, i, L_NOCOPY);
        if ((pixc = pixcompCreateFromFile(str, comptype)) == NULL) {
            L_ERROR("pixc not read from file: %s\n", procName, str);
            continue;
        }
        pixacompAddPixcomp(pixac, pixc);
    }
    return pixac;
}


/*!
 *  pixacompDestroy()
 *
 *      Input:  &pixac (<to be nulled>)
 *      Return: void
 *
 *  Notes:
 *      (1) Always nulls the input ptr.
 */
void
pixacompDestroy(PIXAC  **ppixac)
{
l_int32  i;
PIXAC   *pixac;

    PROCNAME("pixacompDestroy");

    if (ppixac == NULL) {
        L_WARNING("ptr address is NULL!\n", procName);
        return;
    }

    if ((pixac = *ppixac) == NULL)
        return;

    for (i = 0; i < pixac->n; i++)
        pixcompDestroy(&pixac->pixc[i]);
    LEPT_FREE(pixac->pixc);
    boxaDestroy(&pixac->boxa);
    LEPT_FREE(pixac);

    *ppixac = NULL;
    return;
}


/*---------------------------------------------------------------------*
 *                          Pixacomp addition                          *
 *---------------------------------------------------------------------*/
/*!
 *  pixacompAddPix()
 *
 *      Input:  pixac
 *              pix  (to be added)
 *              comptype (IFF_DEFAULT, IFF_TIFF_G4, IFF_PNG, IFF_JFIF_JPEG)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The array is filled up to the (n-1)-th element, and this
 *          converts the input pix to a pixcomp and adds it at
 *          the n-th position.
 */
l_int32
pixacompAddPix(PIXAC   *pixac,
               PIX     *pix,
               l_int32  comptype)
{
l_int32  cmapflag, format;
PIXC    *pixc;

    PROCNAME("pixacompAddPix");

    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (comptype != IFF_DEFAULT && comptype != IFF_TIFF_G4 &&
        comptype != IFF_PNG && comptype != IFF_JFIF_JPEG)
        return ERROR_INT("invalid format", procName, 1);

    cmapflag = pixGetColormap(pix) ? 1 : 0;
    pixcompDetermineFormat(comptype, pixGetDepth(pix), cmapflag, &format);
    if ((pixc = pixcompCreateFromPix(pix, format)) == NULL)
        return ERROR_INT("pixc not made", procName, 1);
    pixacompAddPixcomp(pixac, pixc);
    return 0;
}


/*!
 *  pixacompAddPixcomp()
 *
 *      Input:  pixac
 *              pixc  (to be added by insertion)
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixacompAddPixcomp(PIXAC  *pixac,
                   PIXC   *pixc)
{
l_int32  n;

    PROCNAME("pixacompAddPixcomp");

    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 1);
    if (!pixc)
        return ERROR_INT("pixc not defined", procName, 1);

    n = pixac->n;
    if (n >= pixac->nalloc)
        pixacompExtendArray(pixac);
    pixac->pixc[n] = pixc;
    pixac->n++;

    return 0;
}


/*!
 *  pixacompExtendArray()
 *
 *      Input:  pixac
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) We extend the boxa array simultaneously.  This is
 *          necessary in case we are NOT adding boxes simultaneously
 *          with adding pixc.  We always want the sizes of the
 *          pixac and boxa ptr arrays to be equal.
 */
static l_int32
pixacompExtendArray(PIXAC  *pixac)
{
    PROCNAME("pixacompExtendArray");

    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 1);

    if ((pixac->pixc = (PIXC **)reallocNew((void **)&pixac->pixc,
                            sizeof(PIXC *) * pixac->nalloc,
                            2 * sizeof(PIXC *) * pixac->nalloc)) == NULL)
        return ERROR_INT("new ptr array not returned", procName, 1);
    pixac->nalloc = 2 * pixac->nalloc;
    boxaExtendArray(pixac->boxa);
    return 0;
}


/*!
 *  pixacompReplacePix()
 *
 *      Input:  pixac
 *              index (caller's view of index within pixac; includes offset)
 *              pix  (owned by the caller)
 *              comptype (IFF_DEFAULT, IFF_TIFF_G4, IFF_PNG, IFF_JFIF_JPEG)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The @index includes the offset, which must be subtracted
 *          to get the actual index into the ptr array.
 *      (2) The input @pix is converted to a pixc, which is then inserted
 *          into the pixac.
 */
l_int32
pixacompReplacePix(PIXAC   *pixac,
                   l_int32  index,
                   PIX     *pix,
                   l_int32  comptype)
{
l_int32  n, aindex;
PIXC    *pixc;

    PROCNAME("pixacompReplacePix");

    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 1);
    n = pixacompGetCount(pixac);
    aindex = index - pixac->offset;
    if (aindex < 0 || aindex >= n)
        return ERROR_INT("array index out of bounds", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (comptype != IFF_DEFAULT && comptype != IFF_TIFF_G4 &&
        comptype != IFF_PNG && comptype != IFF_JFIF_JPEG)
        return ERROR_INT("invalid format", procName, 1);

    pixc = pixcompCreateFromPix(pix, comptype);
    pixacompReplacePixcomp(pixac, index, pixc);
    return 0;
}


/*!
 *  pixacompReplacePixcomp()
 *
 *      Input:  pixac
 *              index (caller's view of index within pixac; includes offset)
 *              pixc  (to replace existing one, which is destroyed)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The @index includes the offset, which must be subtracted
 *          to get the actual index into the ptr array.
 *      (2) The inserted @pixc is now owned by the pixac.  The caller
 *          must not destroy it.
 */
l_int32
pixacompReplacePixcomp(PIXAC   *pixac,
                       l_int32  index,
                       PIXC    *pixc)
{
l_int32  n, aindex;
PIXC    *pixct;

    PROCNAME("pixacompReplacePixcomp");

    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 1);
    n = pixacompGetCount(pixac);
    aindex = index - pixac->offset;
    if (aindex < 0 || aindex >= n)
        return ERROR_INT("array index out of bounds", procName, 1);
    if (!pixc)
        return ERROR_INT("pixc not defined", procName, 1);

    pixct = pixacompGetPixcomp(pixac, index);  /* use @index */
    pixcompDestroy(&pixct);
    pixac->pixc[aindex] = pixc;  /* replace; use array index */

    return 0;
}


/*!
 *  pixacompAddBox()
 *
 *      Input:  pixac
 *              box
 *              copyflag (L_INSERT, L_COPY)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixacompAddBox(PIXAC   *pixac,
               BOX     *box,
               l_int32  copyflag)
{
    PROCNAME("pixacompAddBox");

    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if (copyflag != L_INSERT && copyflag != L_COPY)
        return ERROR_INT("invalid copyflag", procName, 1);

    boxaAddBox(pixac->boxa, box, copyflag);
    return 0;
}


/*---------------------------------------------------------------------*
 *                         Pixacomp accessors                          *
 *---------------------------------------------------------------------*/
/*!
 *  pixacompGetCount()
 *
 *      Input:  pixac
 *      Return: count, or 0 if no pixa
 */
l_int32
pixacompGetCount(PIXAC  *pixac)
{
    PROCNAME("pixacompGetCount");

    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 0);

    return pixac->n;
}


/*!
 *  pixacompGetPixcomp()
 *
 *      Input:  pixac
 *              index (caller's view of index within pixac; includes offset)
 *      Return: pixc, or null on error
 *
 *  Notes:
 *      (1) The @index includes the offset, which must be subtracted
 *          to get the actual index into the ptr array.
 *      (2) Important: this is just a ptr to the pixc owned by the pixac.
 *          Do not destroy unless you are replacing the pixc.
 */
PIXC *
pixacompGetPixcomp(PIXAC   *pixac,
                   l_int32  index)
{
l_int32  aindex;

    PROCNAME("pixacompGetPixcomp");

    if (!pixac)
        return (PIXC *)ERROR_PTR("pixac not defined", procName, NULL);
    aindex = index - pixac->offset;
    if (aindex < 0 || aindex >= pixac->n)
        return (PIXC *)ERROR_PTR("array index not valid", procName, NULL);

    return pixac->pixc[aindex];
}


/*!
 *  pixacompGetPix()
 *
 *      Input:  pixac
 *              index (caller's view of index within pixac; includes offset)
 *      Return: pix, or null on error
 *
 *  Notes:
 *      (1) The @index includes the offset, which must be subtracted
 *          to get the actual index into the ptr array.
 */
PIX *
pixacompGetPix(PIXAC   *pixac,
               l_int32  index)
{
l_int32  aindex;
PIXC    *pixc;

    PROCNAME("pixacompGetPix");

    if (!pixac)
        return (PIX *)ERROR_PTR("pixac not defined", procName, NULL);
    aindex = index - pixac->offset;
    if (aindex < 0 || aindex >= pixac->n)
        return (PIX *)ERROR_PTR("array index not valid", procName, NULL);

    pixc = pixacompGetPixcomp(pixac, index);
    return pixCreateFromPixcomp(pixc);
}


/*!
 *  pixacompGetPixDimensions()
 *
 *      Input:  pixa
 *              index (caller's view of index within pixac; includes offset)
 *              &w, &h, &d (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The @index includes the offset, which must be subtracted
 *          to get the actual index into the ptr array.
 */
l_int32
pixacompGetPixDimensions(PIXAC    *pixac,
                         l_int32   index,
                         l_int32  *pw,
                         l_int32  *ph,
                         l_int32  *pd)
{
l_int32  aindex;
PIXC    *pixc;

    PROCNAME("pixacompGetPixDimensions");

    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 1);
    aindex = index - pixac->offset;
    if (aindex < 0 || aindex >= pixac->n)
        return ERROR_INT("array index not valid", procName, 1);

    if ((pixc = pixac->pixc[aindex]) == NULL)
        return ERROR_INT("pixc not found!", procName, 1);
    pixcompGetDimensions(pixc, pw, ph, pd);
    return 0;
}


/*!
 *  pixacompGetBoxa()
 *
 *      Input:  pixac
 *              accesstype  (L_COPY, L_CLONE, L_COPY_CLONE)
 *      Return: boxa, or null on error
 */
BOXA *
pixacompGetBoxa(PIXAC   *pixac,
                l_int32  accesstype)
{
    PROCNAME("pixacompGetBoxa");

    if (!pixac)
        return (BOXA *)ERROR_PTR("pixac not defined", procName, NULL);
    if (!pixac->boxa)
        return (BOXA *)ERROR_PTR("boxa not defined", procName, NULL);
    if (accesstype != L_COPY && accesstype != L_CLONE &&
        accesstype != L_COPY_CLONE)
        return (BOXA *)ERROR_PTR("invalid accesstype", procName, NULL);

    return boxaCopy(pixac->boxa, accesstype);
}


/*!
 *  pixacompGetBoxaCount()
 *
 *      Input:  pixac
 *      Return: count, or 0 on error
 */
l_int32
pixacompGetBoxaCount(PIXAC  *pixac)
{
    PROCNAME("pixacompGetBoxaCount");

    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 0);

    return boxaGetCount(pixac->boxa);
}


/*!
 *  pixacompGetBox()
 *
 *      Input:  pixac
 *              index (caller's view of index within pixac; includes offset)
 *              accesstype  (L_COPY or L_CLONE)
 *      Return: box (if null, not automatically an error), or null on error
 *
 *  Notes:
 *      (1) The @index includes the offset, which must be subtracted
 *          to get the actual index into the ptr array.
 *      (2) There is always a boxa with a pixac, and it is initialized so
 *          that each box ptr is NULL.
 *      (3) In general, we expect that there is either a box associated
 *          with each pixc, or no boxes at all in the boxa.
 *      (4) Having no boxes is thus not an automatic error.  Whether it
 *          is an actual error is determined by the calling program.
 *          If the caller expects to get a box, it is an error; see, e.g.,
 *          pixacGetBoxGeometry().
 */
BOX *
pixacompGetBox(PIXAC    *pixac,
               l_int32   index,
               l_int32   accesstype)
{
l_int32  aindex;
BOX     *box;

    PROCNAME("pixacompGetBox");

    if (!pixac)
        return (BOX *)ERROR_PTR("pixac not defined", procName, NULL);
    if (!pixac->boxa)
        return (BOX *)ERROR_PTR("boxa not defined", procName, NULL);
    aindex = index - pixac->offset;
    if (aindex < 0 || aindex >= pixac->boxa->n)
        return (BOX *)ERROR_PTR("array index not valid", procName, NULL);
    if (accesstype != L_COPY && accesstype != L_CLONE)
        return (BOX *)ERROR_PTR("invalid accesstype", procName, NULL);

    box = pixac->boxa->box[aindex];
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
 *  pixacompGetBoxGeometry()
 *
 *      Input:  pixac
 *              index (caller's view of index within pixac; includes offset)
 *              &x, &y, &w, &h (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The @index includes the offset, which must be subtracted
 *          to get the actual index into the ptr array.
 */
l_int32
pixacompGetBoxGeometry(PIXAC    *pixac,
                       l_int32   index,
                       l_int32  *px,
                       l_int32  *py,
                       l_int32  *pw,
                       l_int32  *ph)
{
l_int32  aindex;
BOX     *box;

    PROCNAME("pixacompGetBoxGeometry");

    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 1);
    aindex = index - pixac->offset;
    if (aindex < 0 || aindex >= pixac->n)
        return ERROR_INT("array index not valid", procName, 1);

    if ((box = pixacompGetBox(pixac, aindex, L_CLONE)) == NULL)
        return ERROR_INT("box not found!", procName, 1);
    boxGetGeometry(box, px, py, pw, ph);
    boxDestroy(&box);
    return 0;
}


/*!
 *  pixacompGetOffset()
 *
 *      Input:  pixac
 *      Return: offset, or 0 on error
 *
 *  Notes:
 *      (1) The offset is the difference between the caller's view of
 *          the index into the array and the actual array index.
 *          By default it is 0.
 */
l_int32
pixacompGetOffset(PIXAC   *pixac)
{
    PROCNAME("pixacompGetOffset");

    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 0);
    return pixac->offset;
}


/*!
 *  pixacompSetOffset()
 *
 *      Input:  pixac
 *              offset (non-negative)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The offset is the difference between the caller's view of
 *          the index into the array and the actual array index.
 *          By default it is 0.
 */
l_int32
pixacompSetOffset(PIXAC   *pixac,
                  l_int32  offset)
{
    PROCNAME("pixacompSetOffset");

    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 1);
    pixac->offset = L_MAX(0, offset);
    return 0;
}


/*---------------------------------------------------------------------*
 *                      Pixacomp conversion to Pixa                    *
 *---------------------------------------------------------------------*/
/*!
 *  pixaCreateFromPixacomp()
 *
 *      Input:  pixac
 *              accesstype (L_COPY, L_CLONE, L_COPY_CLONE; for boxa)
 *      Return: pixa if OK, or null on error
 */
PIXA *
pixaCreateFromPixacomp(PIXAC   *pixac,
                       l_int32  accesstype)
{
l_int32  i, n;
PIX     *pix;
PIXA    *pixa;

    PROCNAME("pixaCreateFromPixacomp");

    if (!pixac)
        return (PIXA *)ERROR_PTR("pixac not defined", procName, NULL);
    if (accesstype != L_COPY && accesstype != L_CLONE &&
        accesstype != L_COPY_CLONE)
        return (PIXA *)ERROR_PTR("invalid accesstype", procName, NULL);

    n = pixacompGetCount(pixac);
    if ((pixa = pixaCreate(n)) == NULL)
        return (PIXA *)ERROR_PTR("pixa not made", procName, NULL);
    for (i = 0; i < n; i++) {
        if ((pix = pixacompGetPix(pixac, i)) == NULL) {
            L_WARNING("pix %d not made\n", procName, i);
            continue;
        }
        pixaAddPix(pixa, pix, L_INSERT);
    }
    if (pixa->boxa) {
        boxaDestroy(&pixa->boxa);
        pixa->boxa = pixacompGetBoxa(pixac, accesstype);
    }

    return pixa;
}


/*---------------------------------------------------------------------*
 *                       Pixacomp serialized I/O                       *
 *---------------------------------------------------------------------*/
/*!
 *  pixacompRead()
 *
 *      Input:  filename
 *      Return: pixac, or null on error
 *
 *  Notes:
 *      (1) Unlike the situation with serialized Pixa, where the image
 *          data is stored in png format, the Pixacomp image data
 *          can be stored in tiffg4, png and jpg formats.
 */
PIXAC *
pixacompRead(const char  *filename)
{
FILE   *fp;
PIXAC  *pixac;

    PROCNAME("pixacompRead");

    if (!filename)
        return (PIXAC *)ERROR_PTR("filename not defined", procName, NULL);
    if ((fp = fopenReadStream(filename)) == NULL)
        return (PIXAC *)ERROR_PTR("stream not opened", procName, NULL);

    if ((pixac = pixacompReadStream(fp)) == NULL) {
        fclose(fp);
        return (PIXAC *)ERROR_PTR("pixac not read", procName, NULL);
    }

    fclose(fp);
    return pixac;
}


/*!
 *  pixacompReadStream()
 *
 *      Input:  stream
 *      Return: pixac, or null on error
 */
PIXAC *
pixacompReadStream(FILE  *fp)
{
char      buf[256];
l_uint8  *data;
l_int32   n, offset, i, w, h, d, ignore;
l_int32   comptype, size, cmapflag, version, xres, yres;
BOXA     *boxa;
PIXC     *pixc;
PIXAC    *pixac;

    PROCNAME("pixacompReadStream");

    if (!fp)
        return (PIXAC *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nPixacomp Version %d\n", &version) != 1)
        return (PIXAC *)ERROR_PTR("not a pixacomp file", procName, NULL);
    if (version != PIXACOMP_VERSION_NUMBER)
        return (PIXAC *)ERROR_PTR("invalid pixacomp version", procName, NULL);
    if (fscanf(fp, "Number of pixcomp = %d", &n) != 1)
        return (PIXAC *)ERROR_PTR("not a pixacomp file", procName, NULL);
    if (fscanf(fp, "Offset of index into array = %d", &offset) != 1)
        return (PIXAC *)ERROR_PTR("offset not read", procName, NULL);

    if ((pixac = pixacompCreate(n)) == NULL)
        return (PIXAC *)ERROR_PTR("pixac not made", procName, NULL);
    if ((boxa = boxaReadStream(fp)) == NULL)
        return (PIXAC *)ERROR_PTR("boxa not made", procName, NULL);
    boxaDestroy(&pixac->boxa);  /* empty */
    pixac->boxa = boxa;
    pixacompSetOffset(pixac, offset);

    for (i = 0; i < n; i++) {
        if ((pixc = (PIXC *)LEPT_CALLOC(1, sizeof(PIXC))) == NULL)
            return (PIXAC *)ERROR_PTR("pixc not made", procName, NULL);
        if (fscanf(fp, "\nPixcomp[%d]: w = %d, h = %d, d = %d\n",
                   &ignore, &w, &h, &d) != 4)
            return (PIXAC *)ERROR_PTR("size reading", procName, NULL);
        if (fscanf(fp, "  comptype = %d, size = %d, cmapflag = %d\n",
                   &comptype, &size, &cmapflag) != 3)
            return (PIXAC *)ERROR_PTR("comptype/size reading", procName, NULL);

           /* Use fgets() and sscanf(); not fscanf(), for the last
             * bit of header data before the binary data.  The reason is
             * that fscanf throws away white space, and if the binary data
             * happens to begin with ascii character(s) that are white
             * space, it will swallow them and all will be lost!  */
        if (fgets(buf, sizeof(buf), fp) == NULL)
            return (PIXAC *)ERROR_PTR("fgets read fail", procName, NULL);
        if (sscanf(buf, "  xres = %d, yres = %d\n", &xres, &yres) != 2)
            return (PIXAC *)ERROR_PTR("read fail for res", procName, NULL);

        if ((data = (l_uint8 *)LEPT_CALLOC(1, size)) == NULL)
            return (PIXAC *)ERROR_PTR("calloc fail for data", procName, NULL);
        if (fread(data, 1, size, fp) != size)
            return (PIXAC *)ERROR_PTR("error reading data", procName, NULL);
        fgetc(fp);  /* swallow the ending nl */
        pixc->w = w;
        pixc->h = h;
        pixc->d = d;
        pixc->xres = xres;
        pixc->yres = yres;
        pixc->comptype = comptype;
        pixc->cmapflag = cmapflag;
        pixc->data = data;
        pixc->size = size;
        pixacompAddPixcomp(pixac, pixc);
    }
    return pixac;
}


/*!
 *  pixacompWrite()
 *
 *      Input:  filename
 *              pixac
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Unlike the situation with serialized Pixa, where the image
 *          data is stored in png format, the Pixacomp image data
 *          can be stored in tiffg4, png and jpg formats.
 */
l_int32
pixacompWrite(const char  *filename,
              PIXAC       *pixac)
{
FILE  *fp;

    PROCNAME("pixacompWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!pixac)
        return ERROR_INT("pixacomp not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "wb")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (pixacompWriteStream(fp, pixac))
        return ERROR_INT("pixacomp not written to stream", procName, 1);
    fclose(fp);
    return 0;
}


/*!
 *  pixacompWriteStream()
 *
 *      Input:  stream
 *              pixac
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixacompWriteStream(FILE   *fp,
                    PIXAC  *pixac)
{
l_int32  n, i;
PIXC    *pixc;

    PROCNAME("pixacompWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 1);

    n = pixacompGetCount(pixac);
    fprintf(fp, "\nPixacomp Version %d\n", PIXACOMP_VERSION_NUMBER);
    fprintf(fp, "Number of pixcomp = %d", n);
    fprintf(fp, "Offset of index into array = %d", pixac->offset);
    boxaWriteStream(fp, pixac->boxa);
    for (i = 0; i < n; i++) {
        if ((pixc =
             pixacompGetPixcomp(pixac, pixacompGetOffset(pixac) + i)) == NULL)
            return ERROR_INT("pixc not found", procName, 1);
        fprintf(fp, "\nPixcomp[%d]: w = %d, h = %d, d = %d\n",
                i, pixc->w, pixc->h, pixc->d);
        fprintf(fp, "  comptype = %d, size = %lu, cmapflag = %d\n",
                pixc->comptype, (unsigned long)pixc->size, pixc->cmapflag);
        fprintf(fp, "  xres = %d, yres = %d\n", pixc->xres, pixc->yres);
        fwrite(pixc->data, 1, pixc->size, fp);
        fprintf(fp, "\n");
    }
    return 0;
}


/*--------------------------------------------------------------------*
 *                         Conversion to pdf                          *
 *--------------------------------------------------------------------*/
/*!
 *  pixacompConvertToPdf()
 *
 *      Input:  pixac (containing images all at the same resolution)
 *              res (override the resolution of each input image, in ppi;
 *                   use 0 to respect the resolution embedded in the input)
 *              scalefactor (scaling factor applied to each image; > 0.0)
 *              type (encoding type (L_JPEG_ENCODE, L_G4_ENCODE,
 *                    L_FLATE_ENCODE, or L_DEFAULT_ENCODE for default)
 *              quality (used for JPEG only; 0 for default (75))
 *              title (<optional> pdf title)
 *              fileout (pdf file of all images)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This follows closely the function pixaConvertToPdf() in pdfio.c.
 *      (2) The images are encoded with G4 if 1 bpp; JPEG if 8 bpp without
 *          colormap and many colors, or 32 bpp; FLATE for anything else.
 *      (3) The scalefactor must be > 0.0; otherwise it is set to 1.0.
 *      (4) Specifying one of the three encoding types for @type forces
 *          all images to be compressed with that type.  Use 0 to have
 *          the type determined for each image based on depth and whether
 *          or not it has a colormap.
 */
l_int32
pixacompConvertToPdf(PIXAC       *pixac,
                     l_int32      res,
                     l_float32    scalefactor,
                     l_int32      type,
                     l_int32      quality,
                     const char  *title,
                     const char  *fileout)
{
l_uint8  *data;
l_int32   ret;
size_t    nbytes;

    PROCNAME("pixacompConvertToPdf");

    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 1);

    ret = pixacompConvertToPdfData(pixac, res, scalefactor, type, quality,
                                   title, &data, &nbytes);
    if (ret) {
        LEPT_FREE(data);
        return ERROR_INT("conversion to pdf failed", procName, 1);
    }

    ret = l_binaryWrite(fileout, "w", data, nbytes);
    LEPT_FREE(data);
    if (ret)
        L_ERROR("pdf data not written to file\n", procName);
    return ret;
}


/*!
 *  pixacompConvertToPdfData()
 *
 *      Input:  pixac (containing images all at the same resolution)
 *              res (input resolution of all images)
 *              scalefactor (scaling factor applied to each image; > 0.0)
 *              type (encoding type (L_JPEG_ENCODE, L_G4_ENCODE,
 *                    L_FLATE_ENCODE, or L_DEFAULT_ENCODE for default)
 *              quality (used for JPEG only; 0 for default (75))
 *              title (<optional> pdf title)
 *              &data (<return> output pdf data (of all images)
 *              &nbytes (<return> size of output pdf data)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) See pixacompConvertToPdf().
 */
l_int32
pixacompConvertToPdfData(PIXAC       *pixac,
                         l_int32      res,
                         l_float32    scalefactor,
                         l_int32      type,
                         l_int32      quality,
                         const char  *title,
                         l_uint8    **pdata,
                         size_t      *pnbytes)
{
l_uint8  *imdata;
l_int32   i, n, ret, scaledres, pagetype;
size_t    imbytes;
L_BYTEA  *ba;
PIX      *pixs, *pix;
L_PTRA   *pa_data;

    PROCNAME("pixacompConvertToPdfData");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    *pdata = NULL;
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);
    *pnbytes = 0;
    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 1);
    if (scalefactor <= 0.0) scalefactor = 1.0;
    if (type < L_DEFAULT_ENCODE || type > L_FLATE_ENCODE) {
        L_WARNING("invalid compression type; using per-page default\n",
                  procName);
        type = L_DEFAULT_ENCODE;
    }

        /* Generate all the encoded pdf strings */
    n = pixacompGetCount(pixac);
    pa_data = ptraCreate(n);
    for (i = 0; i < n; i++) {
        if ((pixs =
             pixacompGetPix(pixac, pixacompGetOffset(pixac) + i)) == NULL) {
            L_ERROR("pix[%d] not retrieved\n", procName, i);
            continue;
        }
        if (pixGetWidth(pixs) == 1) {  /* used sometimes as placeholders */
            L_INFO("placeholder image[%d] has w = 1\n", procName, i);
            pixDestroy(&pixs);
            continue;
        }
        if (scalefactor != 1.0)
            pix = pixScale(pixs, scalefactor, scalefactor);
        else
            pix = pixClone(pixs);
        pixDestroy(&pixs);
        scaledres = (l_int32)(res * scalefactor);
        if (type != L_DEFAULT_ENCODE) {
            pagetype = type;
        } else if (selectDefaultPdfEncoding(pix, &pagetype) != 0) {
            L_ERROR("encoding type selection failed for pix[%d]\n",
                    procName, i);
            pixDestroy(&pix);
            continue;
        }
        ret = pixConvertToPdfData(pix, pagetype, quality, &imdata, &imbytes,
                                  0, 0, scaledres, title, NULL, 0);
        pixDestroy(&pix);
        if (ret) {
            L_ERROR("pdf encoding failed for pix[%d]\n", procName, i);
            continue;
        }
        ba = l_byteaInitFromMem(imdata, imbytes);
        if (imdata) LEPT_FREE(imdata);
        ptraAdd(pa_data, ba);
    }
    ptraGetActualCount(pa_data, &n);
    if (n == 0) {
        L_ERROR("no pdf files made\n", procName);
        ptraDestroy(&pa_data, FALSE, FALSE);
        return 1;
    }

        /* Concatenate them */
    ret = ptraConcatenatePdfToData(pa_data, NULL, pdata, pnbytes);

    ptraGetActualCount(pa_data, &n);  /* recalculate in case it changes */
    for (i = 0; i < n; i++) {
        ba = (L_BYTEA *)ptraRemove(pa_data, i, L_NO_COMPACTION);
        l_byteaDestroy(&ba);
    }
    ptraDestroy(&pa_data, FALSE, FALSE);
    return ret;
}


/*--------------------------------------------------------------------*
 *                        Output for debugging                        *
 *--------------------------------------------------------------------*/
/*!
 *  pixacompWriteStreamInfo()
 *
 *      Input:  fp (file stream)
 *              pixac
 *              text (<optional> identifying string; can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixacompWriteStreamInfo(FILE        *fp,
                        PIXAC       *pixac,
                        const char  *text)
{
l_int32  i, n, nboxes;
PIXC    *pixc;

    PROCNAME("pixacompWriteStreamInfo");

    if (!fp)
        return ERROR_INT("fp not defined", procName, 1);
    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 1);

    if (text)
        fprintf(fp, "Pixacomp Info for %s:\n", text);
    else
        fprintf(fp, "Pixacomp Info:\n");
    n = pixacompGetCount(pixac);
    nboxes = pixacompGetBoxaCount(pixac);
    fprintf(fp, "Number of pixcomp: %d\n", n);
    fprintf(fp, "Size of pixcomp array alloc: %d\n", pixac->nalloc);
    fprintf(fp, "Offset of index into array: %d\n", pixac->offset);
    if (nboxes  > 0)
        fprintf(fp, "Boxa has %d boxes\n", nboxes);
    else
        fprintf(fp, "Boxa is empty\n");
    for (i = 0; i < n; i++) {
        pixc = pixacompGetPixcomp(pixac, pixac->offset + i);
        pixcompWriteStreamInfo(fp, pixc, NULL);
    }
    return 0;
}


/*!
 *  pixcompWriteStreamInfo()
 *
 *      Input:  fp (file stream)
 *              pixc
 *              text (<optional> identifying string; can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixcompWriteStreamInfo(FILE        *fp,
                       PIXC        *pixc,
                       const char  *text)
{
    PROCNAME("pixcompWriteStreamInfo");

    if (!fp)
        return ERROR_INT("fp not defined", procName, 1);
    if (!pixc)
        return ERROR_INT("pixc not defined", procName, 1);

    if (text)
        fprintf(fp, "  Pixcomp Info for %s:", text);
    else
        fprintf(fp, "  Pixcomp Info:");
    fprintf(fp, " width = %d, height = %d, depth = %d\n",
            pixc->w, pixc->h, pixc->d);
    fprintf(fp, "    xres = %d, yres = %d, size in bytes = %lu\n",
            pixc->xres, pixc->yres, (unsigned long)pixc->size);
    if (pixc->cmapflag)
        fprintf(fp, "    has colormap\n");
    else
        fprintf(fp, "    no colormap\n");
    if (pixc->comptype < NumImageFileFormatExtensions) {
        fprintf(fp, "    comptype = %s (%d)\n",
                ImageFileFormatExtensions[pixc->comptype], pixc->comptype);
    } else {
        fprintf(fp, "    Error!! Invalid comptype index: %d\n", pixc->comptype);
    }
    return 0;
}


/*!
 *  pixacompDisplayTiledAndScaled()
 *
 *      Input:  pixac
 *              outdepth (output depth: 1, 8 or 32 bpp)
 *              tilewidth (each pix is scaled to this width)
 *              ncols (number of tiles in each row)
 *              background (0 for white, 1 for black; this is the color
 *                 of the spacing between the images)
 *              spacing  (between images, and on outside)
 *              border (width of additional black border on each image;
 *                      use 0 for no border)
 *      Return: pix of tiled images, or null on error
 *
 *  Notes:
 *      (1) This is the same function as pixaDisplayTiledAndScaled(),
 *          except it works on a Pixacomp instead of a Pix.  It is particularly
 *          useful for showing the images in a Pixacomp at reduced resolution.
 *      (2) This can be used to tile a number of renderings of
 *          an image that are at different scales and depths.
 *      (3) Each image, after scaling and optionally adding the
 *          black border, has width 'tilewidth'.  Thus, the border does
 *          not affect the spacing between the image tiles.  The
 *          maximum allowed border width is tilewidth / 5.
 */
PIX *
pixacompDisplayTiledAndScaled(PIXAC   *pixac,
                              l_int32  outdepth,
                              l_int32  tilewidth,
                              l_int32  ncols,
                              l_int32  background,
                              l_int32  spacing,
                              l_int32  border)
{
l_int32    x, y, w, h, wd, hd, d;
l_int32    i, n, nrows, maxht, ninrow, irow, bordval;
l_int32   *rowht;
l_float32  scalefact;
PIX       *pix, *pixn, *pixt, *pixb, *pixd;
PIXA      *pixan;

    PROCNAME("pixacompDisplayTiledAndScaled");

    if (!pixac)
        return (PIX *)ERROR_PTR("pixac not defined", procName, NULL);
    if (outdepth != 1 && outdepth != 8 && outdepth != 32)
        return (PIX *)ERROR_PTR("outdepth not in {1, 8, 32}", procName, NULL);
    if (border < 0 || border > tilewidth / 5)
        border = 0;

    if ((n = pixacompGetCount(pixac)) == 0)
        return (PIX *)ERROR_PTR("no components", procName, NULL);

        /* Normalize scale and depth for each pix; optionally add border */
    pixan = pixaCreate(n);
    bordval = (outdepth == 1) ? 1 : 0;
    for (i = 0; i < n; i++) {
        if ((pix =
             pixacompGetPix(pixac, pixacompGetOffset(pixac) + i)) == NULL) {
            L_WARNING("pix %d not made\n", procName, i);
            continue;
        }

        pixGetDimensions(pix, &w, &h, &d);
        scalefact = (l_float32)(tilewidth - 2 * border) / (l_float32)w;
        if (d == 1 && outdepth > 1 && scalefact < 1.0)
            pixt = pixScaleToGray(pix, scalefact);
        else
            pixt = pixScale(pix, scalefact, scalefact);

        if (outdepth == 1)
            pixn = pixConvertTo1(pixt, 128);
        else if (outdepth == 8)
            pixn = pixConvertTo8(pixt, FALSE);
        else  /* outdepth == 32 */
            pixn = pixConvertTo32(pixt);
        pixDestroy(&pixt);

        if (border)
            pixb = pixAddBorder(pixn, border, bordval);
        else
            pixb = pixClone(pixn);

        pixaAddPix(pixan, pixb, L_INSERT);
        pixDestroy(&pix);
        pixDestroy(&pixn);
    }
    if ((n = pixaGetCount(pixan)) == 0) { /* should not have changed! */
        pixaDestroy(&pixan);
        return (PIX *)ERROR_PTR("no components", procName, NULL);
    }

        /* Determine the size of each row and of pixd */
    wd = tilewidth * ncols + spacing * (ncols + 1);
    nrows = (n + ncols - 1) / ncols;
    if ((rowht = (l_int32 *)LEPT_CALLOC(nrows, sizeof(l_int32))) == NULL)
        return (PIX *)ERROR_PTR("rowht array not made", procName, NULL);
    maxht = 0;
    ninrow = 0;
    irow = 0;
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixan, i, L_CLONE);
        ninrow++;
        pixGetDimensions(pix, &w, &h, NULL);
        maxht = L_MAX(h, maxht);
        if (ninrow == ncols) {
            rowht[irow] = maxht;
            maxht = ninrow = 0;  /* reset */
            irow++;
        }
        pixDestroy(&pix);
    }
    if (ninrow > 0) {   /* last fencepost */
        rowht[irow] = maxht;
        irow++;  /* total number of rows */
    }
    nrows = irow;
    hd = spacing * (nrows + 1);
    for (i = 0; i < nrows; i++)
        hd += rowht[i];

    pixd = pixCreate(wd, hd, outdepth);
    if ((background == 1 && outdepth == 1) ||
        (background == 0 && outdepth != 1))
        pixSetAll(pixd);

        /* Now blit images to pixd */
    x = y = spacing;
    irow = 0;
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixan, i, L_CLONE);
        pixGetDimensions(pix, &w, &h, NULL);
        if (i && ((i % ncols) == 0)) {  /* start new row */
            x = spacing;
            y += spacing + rowht[irow];
            irow++;
        }
        pixRasterop(pixd, x, y, w, h, PIX_SRC, pix, 0, 0);
        x += tilewidth + spacing;
        pixDestroy(&pix);
    }

    pixaDestroy(&pixan);
    LEPT_FREE(rowht);
    return pixd;
}
