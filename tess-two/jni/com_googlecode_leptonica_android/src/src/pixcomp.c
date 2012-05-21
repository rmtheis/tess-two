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
 *           PIXAC    *pixacompCreateInitialized()
 *           PIXAC    *pixacompCreateFromPixa()
 *           PIXAC    *pixacompCreateFromFiles()
 *           PIXAC    *pixacompCreateFromSA()
 *           void      pixacompDestroy()
 *
 *      Pixacomp addition/replacement
 *           l_int32   pixacompAddPix()
 *           l_int32   pixacompAddPixcomp()
 *           l_int32   pixacompExtendArray()
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
 *   For random insertion and replacement of pixcomp into a pixcomp,
 *   initialize a fully populated array using pixacompCreateInitialized().
 *   Then use pixacompReplacePix() or pixacompReplacePixcomp() for
 *   the random insertion.
 */

#include <string.h>
#include "allheaders.h"

static const l_int32  INITIAL_PTR_ARRAYSIZE = 20;   /* n'import quoi */

    /* These two globals are defined in writefile.c */
extern l_int32 NumImageFileFormatExtensions;
extern const char *ImageFileFormatExtensions[];


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

    if ((pixc = (PIXC *)CALLOC(1, sizeof(PIXC))) == NULL)
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
        L_ERROR("write to memory failed", procName);
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
    if ((pixc = (PIXC *)CALLOC(1, sizeof(PIXC))) == NULL)
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
    if (format == IFF_UNKNOWN)
        return (PIXC *)ERROR_PTR("image file not readable", procName, NULL);

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
            FREE(data);
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
        L_WARNING("ptr address is null!", procName);
        return;
    }

    if ((pixc = *ppixc) == NULL)
        return;

    FREE(pixc->data);
    if (pixc->text)
        FREE(pixc->text);
    FREE(pixc);
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
    }
    else if (comptype == IFF_TIFF_G4 && d == 1)
        *pformat = IFF_TIFF_G4;
    else if (comptype == IFF_JFIF_JPEG && d >= 8 && !cmapflag)
        *pformat = IFF_JFIF_JPEG;

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
        L_INFO_INT2("pix width %d != pixc width %d", procName, w, pixc->w);
        L_ERROR_INT("pix width %d != pixc width", procName, w);
    }
    if (pixc->h != h)
        L_ERROR_INT("pix height %d != pixc height", procName, h);
    if (pixc->d != d) {
        if (pixc->d == 16)  /* we strip 16 --> 8 bpp by default */
            L_WARNING_INT("pix depth %d != pixc depth 16", procName, d);
        else
            L_ERROR_INT("pix depth %d != pixc depth", procName, d);
    }
    cmapinpix = (pixGetColormap(pix) != NULL);
    if ((cmapinpix && !pixc->cmapflag) || (!cmapinpix && pixc->cmapflag))
        L_ERROR("pix cmap flag inconsistent", procName);
    format = pixGetInputFormat(pix);
    if (format != pixc->comptype) {
        L_ERROR_INT("pix comptype %d not equal to pixc comptype",
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

    if ((pixac = (PIXAC *)CALLOC(1, sizeof(PIXAC))) == NULL)
        return (PIXAC *)ERROR_PTR("pixac not made", procName, NULL);
    pixac->n = 0;
    pixac->nalloc = n;
    
    if ((pixac->pixc = (PIXC **)CALLOC(n, sizeof(PIXC *))) == NULL)
        return (PIXAC *)ERROR_PTR("pixc ptrs not made", procName, NULL);
    if ((pixac->boxa = boxaCreate(n)) == NULL)
        return (PIXAC *)ERROR_PTR("boxa not made", procName, NULL);

    return pixac;
}


/*!
 *  pixacompCreateInitialized()
 *
 *      Input:  n  (initial number of ptrs)
 *              pix (initialize each ptr in pixacomp to this pix)
 *              comptype (IFF_DEFAULT, IFF_TIFF_G4, IFF_PNG, IFF_JFIF_JPEG)
 *      Return: pixac, or null on error
 *
 *  Notes:
 *      (1) Initializes a pixacomp to be fully populated with @pix.
 *      (2) Typically use a very small @pix (w = h = 1) with
 *          @comptype == IFF_TIFF_G4 for the initialization.
 *      (3) Example usage:
 *            Pix *pix = pixCreate(1, 1, 1);
 *            Pixacomp *pixac = pixacompCreateInitialized(50, pix, IFF_TIFF_G4);
 *            for (i = 0; i < 50; i++) {
 *                Pix *pixt = ...
 *                if (pixt)
 *                    pixacompReplacePix(pixac, i, pixt, IFF_TIFF_G4);
 *                pixDestroy(&pixt);
 *            }
 *          The result is a fully populated pixac with selected pixt
 *          replacing the placeholders.
 */
PIXAC *
pixacompCreateInitialized(l_int32  n,
                          PIX     *pix,
                          l_int32  comptype)
{
l_int32  i;
PIXC    *pixc;
PIXAC   *pixac;

    PROCNAME("pixacompCreateInitialized");

    if (n <= 0)
        return (PIXAC *)ERROR_PTR("n must be > 0", procName, NULL);
    if (!pix)
        return (PIXAC *)ERROR_PTR("pix not defined", procName, NULL);

    if ((pixac = pixacompCreate(n)) == NULL)
        return (PIXAC *)ERROR_PTR("pixac not made", procName, NULL);
    for (i = 0; i < n; i++) {
        pixc = pixcompCreateFromPix(pix, comptype);
        pixacompAddPixcomp(pixac, pixc);
    }

    return pixac;
}


/*!
 *  pixacompCreateFromPixa()
 *
 *      Input:  pixa
 *              comptype (IFF_DEFAULT, IFF_TIFF_G4, IFF_PNG, IFF_JFIF_JPEG)
 *              accesstype (L_COPY, L_CLONE, L_COPY_CLONE; for boxa)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If @format == IFF_DEFAULT, the conversion format for each
 *          image is chosen automatically.  Otherwise, we use the
 *          specified format unless it can't be done (e.g., jpeg
 *          for a 1, 2 or 4 bpp pix, or a pix with a colormap),
 *          in which case we use the default (assumed best) compression.
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
char     *str;
l_int32   i, n;
PIXC     *pixc;
PIXAC    *pixac;

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
            L_WARNING_STRING("pixc not read from file %s", procName, str);
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
        L_WARNING("ptr address is NULL!", procName);
        return;
    }

    if ((pixac = *ppixac) == NULL)
        return;

    for (i = 0; i < pixac->n; i++)
        pixcompDestroy(&pixac->pixc[i]);
    FREE(pixac->pixc);
    boxaDestroy(&pixac->boxa);
    FREE(pixac);

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
l_int32
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
 *              index (of pixc within pixac to be replaced)
 *              pix  (owned by the caller)
 *              comptype (IFF_DEFAULT, IFF_TIFF_G4, IFF_PNG, IFF_JFIF_JPEG)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The input @pix is converted to a pixc, which is then inserted
 *          into the pixac.
 */
l_int32
pixacompReplacePix(PIXAC   *pixac,
                   l_int32  index,
                   PIX     *pix,
                   l_int32  comptype)
{
l_int32  n;
PIXC    *pixc;

    PROCNAME("pixacompReplacePix");

    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 1);
    n = pixacompGetCount(pixac);
    if (index < 0 || index >= n)
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
 *              index (of pixc within pixac to be replaced)
 *              pixc  (to replace existing one, which is destroyed)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The inserted @pixc is now owned by the pixac.  The caller
 *          must not destroy it.
 */
l_int32
pixacompReplacePixcomp(PIXAC   *pixac,
                       l_int32  index,
                       PIXC    *pixc)
{
l_int32  n;
PIXC    *pixct;

    PROCNAME("pixacompReplacePixcomp");

    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 1);
    n = pixacompGetCount(pixac);
    if (index < 0 || index >= n)
        return ERROR_INT("array index out of bounds", procName, 1);
    if (!pixc)
        return ERROR_INT("pixc not defined", procName, 1);

    pixct = pixacompGetPixcomp(pixac, index);
    pixcompDestroy(&pixct);
    pixac->pixc[index] = pixc;  /* replace */

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
 *              index  (to the index-th pix)
 *      Return: pixc, or null on error
 *
 *  Notes:
 *      (1) Important: this is just a ptr to the pixc owned by the pixac.
 *          Do not destroy unless you are replacing the pixc.
 */
PIXC *
pixacompGetPixcomp(PIXAC   *pixac,
                   l_int32  index)
{
    PROCNAME("pixacompGetPixcomp");

    if (!pixac)
        return (PIXC *)ERROR_PTR("pixac not defined", procName, NULL);
    if (index < 0 || index >= pixac->n)
        return (PIXC *)ERROR_PTR("index not valid", procName, NULL);

    return pixac->pixc[index];
}


/*!
 *  pixacompGetPix()
 *
 *      Input:  pixac
 *              index  (to the index-th pix)
 *      Return: pix, or null on error
 */
PIX *
pixacompGetPix(PIXAC   *pixac,
               l_int32  index)
{
PIXC  *pixc;

    PROCNAME("pixacompGetPix");

    if (!pixac)
        return (PIX *)ERROR_PTR("pixac not defined", procName, NULL);
    if (index < 0 || index >= pixac->n)
        return (PIX *)ERROR_PTR("index not valid", procName, NULL);

    pixc = pixacompGetPixcomp(pixac, index);
    return pixCreateFromPixcomp(pixc);
}


/*!
 *  pixacompGetPixDimensions()
 *
 *      Input:  pixa
 *              index  (to the index-th box)
 *              &w, &h, &d (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixacompGetPixDimensions(PIXAC    *pixac,
                         l_int32   index,
                         l_int32  *pw,
                         l_int32  *ph,
                         l_int32  *pd)
{
PIXC  *pixc;

    PROCNAME("pixacompGetPixDimensions");

    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 1);
    if (index < 0 || index >= pixac->n)
        return ERROR_INT("index not valid", procName, 1);

    if ((pixc = pixac->pixc[index]) == NULL)
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
 *              index  (to the index-th pix)
 *              accesstype  (L_COPY or L_CLONE)
 *      Return: box (if null, not automatically an error), or null on error
 *
 *  Notes:
 *      (1) There is always a boxa with a pixac, and it is initialized so
 *          that each box ptr is NULL.
 *      (2) In general, we expect that there is either a box associated
 *          with each pixc, or no boxes at all in the boxa.
 *      (3) Having no boxes is thus not an automatic error.  Whether it
 *          is an actual error is determined by the calling program.
 *          If the caller expects to get a box, it is an error; see, e.g.,
 *          pixacGetBoxGeometry().
 */
BOX *
pixacompGetBox(PIXAC    *pixac,
               l_int32   index,
               l_int32   accesstype)
{
BOX  *box;

    PROCNAME("pixacompGetBox");

    if (!pixac)
        return (BOX *)ERROR_PTR("pixac not defined", procName, NULL);
    if (!pixac->boxa)
        return (BOX *)ERROR_PTR("boxa not defined", procName, NULL);
    if (index < 0 || index >= pixac->boxa->n)
        return (BOX *)ERROR_PTR("index not valid", procName, NULL);
    if (accesstype != L_COPY && accesstype != L_CLONE)
        return (BOX *)ERROR_PTR("invalid accesstype", procName, NULL);

    box = pixac->boxa->box[index];
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
 *  pixacompGetBoxGeometry()
 *
 *      Input:  pixac
 *              index  (to the index-th box)
 *              &x, &y, &w, &h (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixacompGetBoxGeometry(PIXAC     *pixac,
                       l_int32    index,
                       l_int32   *px,
                       l_int32   *py,
                       l_int32   *pw,
                       l_int32  *ph)
{
BOX  *box;

    PROCNAME("pixacompGetBoxGeometry");

    if (!pixac)
        return ERROR_INT("pixac not defined", procName, 1);
    if (index < 0 || index >= pixac->n)
        return ERROR_INT("index not valid", procName, 1);

    if ((box = pixacompGetBox(pixac, index, L_CLONE)) == NULL)
        return ERROR_INT("box not found!", procName, 1);
    boxGetGeometry(box, px, py, pw, ph);
    boxDestroy(&box);
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
            L_WARNING_INT("pix %d not made", procName, i);
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
l_uint8  *data;
l_int32   n, i, w, h, d, ignore;
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

    if ((pixac = pixacompCreate(n)) == NULL)
        return (PIXAC *)ERROR_PTR("pixac not made", procName, NULL);
    if ((boxa = boxaReadStream(fp)) == NULL)
        return (PIXAC *)ERROR_PTR("boxa not made", procName, NULL);
    boxaDestroy(&pixac->boxa);  /* empty */
    pixac->boxa = boxa;

    for (i = 0; i < n; i++) {
        if ((pixc = (PIXC *)CALLOC(1, sizeof(PIXC))) == NULL)
            return (PIXAC *)ERROR_PTR("pixc not made", procName, NULL);
        if (fscanf(fp, "  Pixcomp[%d]: w = %d, h = %d, d = %d\n",
                   &ignore, &w, &h, &d) != 4)
            return (PIXAC *)ERROR_PTR("size reading", procName, NULL);
        if (fscanf(fp, "    comptype = %d, size = %d, cmapflag = %d\n",
                   &comptype, &size, &cmapflag) != 3)
            return (PIXAC *)ERROR_PTR("comptype/size reading", procName, NULL);
        if (fscanf(fp, "    xres = %d, yres = %d\n", &xres, &yres) != 2)
            return (PIXAC *)ERROR_PTR("res reading", procName, NULL);
        if ((data = (l_uint8 *)CALLOC(1, size)) == NULL)
            return (PIXAC *)ERROR_PTR("calloc fail for data", procName, NULL);
        if (fread(data, 1, size, fp) != size)
            return (PIXAC *)ERROR_PTR("error reading data", procName, NULL);
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
    boxaWriteStream(fp, pixac->boxa);
    for (i = 0; i < n; i++) {
        if ((pixc = pixacompGetPixcomp(pixac, i)) == NULL)
            return ERROR_INT("pixc not found", procName, 1);
        fprintf(fp, "  Pixcomp[%d]: w = %d, h = %d, d = %d\n",
                i, pixc->w, pixc->h, pixc->d);
        fprintf(fp, "    comptype = %d, size = %ld, cmapflag = %d\n",
                pixc->comptype, pixc->size, pixc->cmapflag);
        fprintf(fp, "    xres = %d, yres = %d\n", pixc->xres, pixc->yres);
        fwrite(pixc->data, 1, pixc->size, fp);
    }
    return 0;
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
    if (nboxes  > 0)
        fprintf(fp, "Boxa has %d boxes\n", nboxes);
    else
        fprintf(fp, "Boxa is empty\n");
    for (i = 0; i < n; i++) {
        pixc = pixacompGetPixcomp(pixac, i);
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
    fprintf(fp, "    xres = %d, yres = %d, size in bytes = %ld\n",
            pixc->xres, pixc->yres, pixc->size);
    if (pixc->cmapflag)
        fprintf(fp, "    has colormap\n");
    else
        fprintf(fp, "    no colormap\n");
    if (pixc->comptype < NumImageFileFormatExtensions)
        fprintf(fp, "    comptype = %s (%d)\n",
                ImageFileFormatExtensions[pixc->comptype], pixc->comptype);
    else
        fprintf(fp, "    Error!! Invalid comptype index: %d\n", pixc->comptype);
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
        if ((pix = pixacompGetPix(pixac, i)) == NULL) {
            L_WARNING_INT("pix %d not made", procName, i);
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
    if ((rowht = (l_int32 *)CALLOC(nrows, sizeof(l_int32))) == NULL)
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
    FREE(rowht);
    return pixd;
}


