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
 *  pdfio.c
 *
 *    |=============================================================|
 *    |                         Important note                      |
 *    |=============================================================|
 *    | Some of these functions require libtiff, libjpeg, and libz  |
 *    | If you do not have these libraries, you must set            |
 *    |      #define  USE_PDFIO     0                               |
 *    | in environ.h.  This will link pdfiostub.c                   |
 *    |=============================================================|
 *
 *     The first set of functions converts a set of images to a multi-page
 *     pdf file, with one image on each page.  All images are rendered
 *     at the same (input) resolution.  The images can be specified as
 *     being in a directory, or they can be in an sarray.  The output
 *     pdf can be either a file or an array of bytes in memory.
 *
 *     The second set of functions implements a pdf output "device driver"
 *     for wrapping (encoding) any number of images on a single page
 *     in pdf.  The images can be rendered using a pdf viewer,
 *     such as gv, evince, xpdf or acroread.
 *     See: http://www.adobe.com/devnet/pdf/pdf_reference_archive.html
 *
 *     The third set of functions (segmented) takes an image, an
 *     optional binary mask, an encoding flag, and some other parameters,
 *     and generates a single-page mixed raster pdf.
 *
 *     The fourth set of functions (concatenated) takes a set of single-page
 *     pdf files and concatenates them into a multi-page pdf
 *
 *     1. Convert specified image files to Pdf (one image file per page)
 *          l_int32             convertFilesToPdf()
 *          l_int32             saConvertFilesToPdf()
 *          l_int32             saConvertFilesToPdfData()
 *          l_int32             selectDefaultPdfEncoding()
 *
 *     2. Single page, multi-image converters
 *          l_int32             convertToPdf()
 *          l_int32             convertImageDataToPdf()
 *          l_int32             convertToPdfData()
 *          l_int32             convertImageDataToPdfData()
 *          l_int32             pixConvertToPdf()
 *          l_int32             pixConvertToPdfData()
 *          l_int32             pixWriteStreamPdf()
 *
 *     3. Segmented multi-page, multi-image converter
 *          l_int32             convertSegmentedFilesToPdf()
 *
 *     4. Segmented single page, multi-image converters
 *          l_int32             convertToPdfSegmented()
 *          l_int32             pixConvertToPdfSegmented()
 *          l_int32             convertToPdfDataSegmented()
 *          l_int32             pixConvertToPdfDataSegmented()
 *
 *     Helper functions for generating the output pdf string
 *          static l_int32      l_generatePdf()
 *          static void         generateFixedStringsPdf()
 *          static void         generateMediaboxPdf()
 *          static l_int32      generatePageStringPdf()
 *          static l_int32      generateContentStringPdf()
 *          static l_int32      generatePreXStringsPdf()
 *          static l_int32      generateColormapStringsPdf()
 *          static void         generateTrailerPdf()
 *          static l_int32      makeTrailerStringPdf()
 *          static l_int32      generateOutputDataPdf()
 *
 *     5. Multi-page concatenation
 *          l_int32             concatenatePdf()
 *          l_int32             saConcatenatePdf()
 *          l_int32             ptraConcatenatePdf()
 *          l_int32             concatenatePdfToData()
 *          l_int32             saConcatenatePdfToData()
 *          l_int32             ptraConcatenatePdfToData()
 *
 *     Helper functions for generating the multi-page pdf output
 *          static l_int32      parseTrailerPdf()
 *          static char        *generatePagesObjStringPdf()
 *          static L_BYTEA     *substituteObjectNumbers()
 *
 *     Create/destroy/access pdf data
 *          static L_PDF_DATA         *pdfdataCreate()
 *          static void                pdfdataDestroy()
 *          static L_COMPRESSED_DATA  *pdfdataGetCid()
 *
 *     Set flags for special modes
 *          void                l_pdfSetG4ImageMask()
 *          void                l_pdfSetDateAndVersion()
 *
 *     The top-level multi-image functions can be visualized as follows:
 *          Output pdf data to file:
 *             convertToPdf()  and  convertImageDataToPdf()
 *                     --> pixConvertToPdf()
 *                           --> pixConvertToPdfData()
 *
 *          Output pdf data to array in memory:
 *             convertToPdfData()  and  convertImageDataToPdfData()
 *                     --> pixConvertToPdfData()
 *
 *     The top-level segmented image functions can be visualized as follows:
 *          Output pdf data to file:
 *             convertToPdfSegmented()
 *                     --> pixConvertToPdfSegmented()
 *                           --> pixConvertToPdfDataSegmented()
 *
 *          Output pdf data to array in memory:
 *             convertToPdfDataSegmented()
 *                     --> pixConvertToPdfDataSegmented()
 *
 *     For multi-page concatenation, there are three different types of input
 *        (1) directory and optional filename filter
 *        (2) sarray of filenames
 *        (3) ptra of byte arrays of pdf data
 *     and two types of output for the concatenated pdf data
 *        (1) filename
 *        (2) data array and size
 *     High-level interfaces are given for each of the six combinations.
 */

#include <string.h>
#include <math.h>
#include "allheaders.h"

/* --------------------------------------------*/
#if  USE_PDFIO   /* defined in environ.h */
 /* --------------------------------------------*/
 
    /* Typical scan resolution in ppi (pixels/inch) */
static const l_int32  DEFAULT_INPUT_RES = 300;

    /* Static helpers */
static l_int32   l_generatePdf(l_uint8 **pdata, size_t *pnbytes,
                               L_PDF_DATA *lpd);
static void      generateFixedStringsPdf(L_PDF_DATA *lpd);
static void      generateMediaboxPdf(L_PDF_DATA *lpd);
static l_int32   generatePageStringPdf(L_PDF_DATA *lpd);
static l_int32   generateContentStringPdf(L_PDF_DATA *lpd);
static l_int32   generatePreXStringsPdf(L_PDF_DATA *lpd);
static l_int32   generateColormapStringsPdf(L_PDF_DATA *lpd);
static void      generateTrailerPdf(L_PDF_DATA *lpd);
static char     *makeTrailerStringPdf(NUMA *naloc);
static l_int32   generateOutputDataPdf(l_uint8 **pdata, size_t *pnbytes,
                                       L_PDF_DATA *lpd);

static l_int32   parseTrailerPdf(L_BYTEA *bas, NUMA **pna);
static char     *generatePagesObjStringPdf(NUMA *napage);
static L_BYTEA  *substituteObjectNumbers(L_BYTEA *bas, NUMA *na_objs);

static L_PDF_DATA         *pdfdataCreate(const char *title);
static void                pdfdataDestroy(L_PDF_DATA **plpd);
static L_COMPRESSED_DATA  *pdfdataGetCid(L_PDF_DATA *lpd, l_int32 index);


/* ---------------- Defaults for rendering options ----------------- */
    /* Output G4 as writing through image mask; this is the default */
static l_int32   var_WRITE_G4_IMAGE_MASK = 1;
    /* Write date/time and lib version into pdf; this is the default */
static l_int32   var_WRITE_DATE_AND_VERSION = 1;

#define L_SMALLBUF   256
#define L_BIGBUF    2048   /* must be able to hold hex colormap */


#ifndef  NO_CONSOLE_IO
#define  DEBUG_MULTIPAGE      0
#endif  /* ~NO_CONSOLE_IO */


/*---------------------------------------------------------------------*
 *    Convert specified image files to Pdf (one image file per page)   *
 *---------------------------------------------------------------------*/
/*!
 *  convertFilesToPdf()
 *
 *      Input:  directory name (containing images)
 *              substr (<optional> substring filter on filenames; can be NULL)
 *              res (input resolution of all images)
 *              scalefactor (scaling factor applied to each image)
 *              quality (used for JPEG only; 0 for default (75))
 *              title (<optional> pdf title; if null, taken from the first
 *                     image filename)
 *              fileout (pdf file of all images)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If @substr is not NULL, only image filenames that contain
 *          the substring can be used.  If @substr == NULL, all files
 *          in the directory are used.
 *      (2) The files in the directory, after optional filtering by
 *          the substring, are lexically sorted in increasing order
 *          before concatenation.
 *      (3) The images are encoded with G4 if 1 bpp; JPEG if 8 bpp without
 *          colormap and many colors, or 32 bpp; FLATE for anything else.
 */
l_int32
convertFilesToPdf(const char  *dirname,
                  const char  *substr,
                  l_int32      res,
                  l_float32    scalefactor,
                  l_int32      quality,
                  const char  *title,
                  const char  *fileout)
{
l_int32  ret;
SARRAY  *sa;

    PROCNAME("convertFilesToPdf");

    if (!dirname)
        return ERROR_INT("dirname not defined", procName, 1);
    if (!fileout)
        return ERROR_INT("fileout not defined", procName, 1);

    if ((sa = getSortedPathnamesInDirectory(dirname, substr, 0, 0)) == NULL)
        return ERROR_INT("sa not made", procName, 1);
    ret = saConvertFilesToPdf(sa, res, scalefactor, quality, title, fileout);
    sarrayDestroy(&sa);
    return ret;
}


/*!
 *  saConvertFilesToPdf()
 *
 *      Input:  sarray (of pathnames for images)
 *              res (input resolution of all images)
 *              scalefactor (scaling factor applied to each image)
 *              quality (used for JPEG only; 0 for default (75))
 *              title (<optional> pdf title; if null, taken from the first
 *                     image filename)
 *              fileout (pdf file of all images)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The images are encoded with G4 if 1 bpp; JPEG if 8 bpp without
 *          colormap and many colors, or 32 bpp; FLATE for anything else.
 */
l_int32
saConvertFilesToPdf(SARRAY      *sa,
                    l_int32      res,
                    l_float32    scalefactor,
                    l_int32      quality,
                    const char  *title,
                    const char  *fileout)
{
l_uint8  *data;
l_int32   ret;
size_t    nbytes;

    PROCNAME("saConvertFilesToPdf");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);

    ret = saConvertFilesToPdfData(sa, res, scalefactor, quality, title,
                                  &data, &nbytes);
    if (ret) {
        if (data) FREE(data);
        return ERROR_INT("pdf data not made", procName, 1);
    }

    ret = l_binaryWrite(fileout, "w", data, nbytes);
    FREE(data);
    if (ret)
        L_ERROR("pdf data not written to file", procName);
    return ret;
}


/*!
 *  saConvertFilesToPdfData()
 *
 *      Input:  sarray (of pathnames for images)
 *              res (input resolution of all images)
 *              scalefactor (scaling factor applied to each image)
 *              quality (used for JPEG only; 0 for default (75))
 *              title (<optional> pdf title; if null, taken from the first
 *                     image filename)
 *              &data (<return> output pdf data (of all images)
 *              &nbytes (<return> size of output pdf data)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The images are encoded with G4 if 1 bpp; JPEG if 8 bpp without
 *          colormap and many colors, or 32 bpp; FLATE for anything else.
 */
l_int32
saConvertFilesToPdfData(SARRAY      *sa,
                        l_int32      res,
                        l_float32    scalefactor,
                        l_int32      quality,
                        const char  *title,
                        l_uint8    **pdata,
                        size_t      *pnbytes)
{
char     *fname;
l_uint8  *imdata;
l_int32   i, n, ret, type, npages, scaledres;
size_t    imbytes;
L_BYTEA  *ba;
PIX      *pixs, *pix;
L_PTRA   *pa_data;

    PROCNAME("saConvertFilesToPdfData");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (scalefactor <= 0.0) scalefactor = 1.0;

        /* Generate all the encoded pdf strings */
    n = sarrayGetCount(sa);
    pa_data = ptraCreate(n);
    for (i = 0; i < n; i++) {
        fname = sarrayGetString(sa, i, L_NOCOPY);
        if ((pixs = pixRead(fname)) == NULL) {
            L_ERROR_STRING("image not readable from file %s", procName, fname);
            continue;
        }
        if (scalefactor != 1.0)
            pix = pixScale(pixs, scalefactor, scalefactor);
        else
            pix = pixClone(pixs);
        scaledres = (l_int32)(res * scalefactor);
        if (selectDefaultPdfEncoding(pix, &type)) {
            L_ERROR_STRING("encoding type selection failed for file %s",
                           procName, fname);
            pixDestroy(&pix);
            continue;
        }
        ret = pixConvertToPdfData(pix, type, quality, &imdata, &imbytes,
                                  0, 0, scaledres, NULL, 0, title);
        pixDestroy(&pix);
        pixDestroy(&pixs);
        if (ret) {
            L_ERROR_STRING("pdf encoding failed for %s", procName, fname);
            continue;
        }
        ba = l_byteaInitFromMem(imdata, imbytes);
        if (imdata) FREE(imdata);
        ptraAdd(pa_data, ba);
    }
    ptraGetActualCount(pa_data, &npages);
    if (npages == 0) {
        L_ERROR("no pdf files made", procName);
        ptraDestroy(&pa_data, FALSE, FALSE);
        return 1;
    }

        /* Concatenate them */
    ret = ptraConcatenatePdfToData(pa_data, NULL, pdata, pnbytes);

    ptraGetActualCount(pa_data, &npages);  /* recalculate in case it changes */
    for (i = 0; i < npages; i++) {
        ba = (L_BYTEA *)ptraRemove(pa_data, i, L_NO_COMPACTION);
        l_byteaDestroy(&ba);
    }
    ptraDestroy(&pa_data, FALSE, FALSE);
    return ret;
}


/*!
 *  selectDefaultPdfEncoding()
 *
 *      Input:  pix
 *              &type (<return> L_G4_ENCODE, L_JPEG_ENCODE, L_FLATE_ENCODE)
 *
 *  Notes:
 *      (1) This attempts to choose an encoding for the pix that results
 *          in the smallest file, assuming that if jpeg encoded, it will
 *          use quality = 75.  The decision is approximate, in that
 *          (a) all colormapped images will be losslessly encoded with
 *          gzip (flate), and (b) an image with less than about 20 colors
 *          is likely to be smaller if flate encoded than if encoded
 *          as a jpeg (dct).  For example, an image made by pixScaleToGray3()
 *          will have 10 colors, and flate encoding will give about
 *          twice the compression as jpeg with quality = 75.
 */
l_int32
selectDefaultPdfEncoding(PIX      *pix,
                         l_int32  *ptype)
{
l_int32   w, h, d, factor, ncolors;
PIXCMAP  *cmap;

    PROCNAME("selectDefaultPdfEncoding");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!ptype)
        return ERROR_INT("&type not defined", procName, 1);
    *ptype = L_FLATE_ENCODE;  /* default universal encoding */
    pixGetDimensions(pix, &w, &h, &d);
    cmap = pixGetColormap(pix);
    if (d == 8 && !cmap) {
        factor = L_MAX(1, (l_int32)sqrt((l_float64)(w * h) / 20000.));
        pixNumColors(pix, factor, &ncolors);
        if (ncolors < 20)
            *ptype = L_FLATE_ENCODE;
        else
            *ptype = L_JPEG_ENCODE;
    }
    else if (d == 1)
        *ptype = L_G4_ENCODE;
    else if (cmap || d == 2 || d == 4)
        *ptype = L_FLATE_ENCODE;
    else if (d == 8 || d == 32)
        *ptype = L_JPEG_ENCODE;
    else
        return ERROR_INT("type selection failure", procName, 1);

    return 0;
}


/*---------------------------------------------------------------------*
 *                Single page, multi-image converters                  *
 *---------------------------------------------------------------------*/
/*!
 *  convertToPdf()
 *
 *      Input:  filein (input image file -- any format)
 *              type (L_G4_ENCODE, L_JPEG_ENCODE, L_FLATE_ENCODE)
 *              quality (used for JPEG only; 0 for default (75))
 *              fileout (output pdf file; only required on last image on page)
 *              x, y (location of lower-left corner of image, in pixels,
 *                    relative to the PostScript origin (0,0) at
 *                    the lower-left corner of the page)
 *              res (override the resolution of the input image, in ppi;
 *                   use 0 to respect the resolution embedded in the input)
 *              &lpd (ptr to lpd, which is created on the first invocation
 *                    and returned until last image is processed, at which
 *                    time it is destroyed)
 *              position (in image sequence: L_FIRST_IMAGE, L_NEXT_IMAGE,
 *                       L_LAST_IMAGE)
 *              title (<optional> pdf title; if null, taken from the first
 *                     image placed on a page; e.g., an input image filename)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) To wrap only one image in pdf, input @plpd = NULL, and
 *          the value of @position will be ignored:
 *            convertToPdf(...  type, quality, x, y, res, NULL, 0);
 *      (2) To wrap multiple images on a single pdf page, this is called
 *          once for each successive image.  Do it this way:
 *            L_PDF_DATA   *lpd;
 *            convertToPdf(...  type, quality, x, y, res, &lpd, L_FIRST_IMAGE);
 *            convertToPdf(...  type, quality, x, y, res, &lpd, L_NEXT_IMAGE);
 *            ...
 *            convertToPdf(...  type, quality, x, y, res, &lpd, L_LAST_IMAGE);
 *          This will write the result to the value of @fileout specified
 *          in the first call; succeeding values of @fileout are ignored.
 *          On the last call: the pdf data bytes are computed and written
 *          to @fileout, lpd is destroyed internally, and the returned
 *          value of lpd is null.  So the client has nothing to clean up.
 *      (3) (a) Set @res == 0 to respect the resolution embedded in the
 *              image file.  If no resolution is embedded, it will be set
 *              to the default value.
 *          (b) Set @res to some other value to override the file resolution.
 *      (4) (a) If the input @res and the resolution of the output device
 *              are equal, the image will be "displayed" at the same size
 *              as the original.
 *          (b) If the input @res is 72, the output device will render
 *              the image at 1 pt/pixel.
 *          (c) Some possible choices for the default input pix resolution are:
 *                 72 ppi     Render pix on any output device at one pt/pixel
 *                 96 ppi     Windows default for generated display images
 *                300 ppi     Typical default for scanned images.
 *              We choose 300, which is sensible for rendering page images.
 *              However,  images come from a variety of sources, and
 *              some are explicitly created for viewing on a display.
 */
l_int32
convertToPdf(const char   *filein,
             l_int32       type,
             l_int32       quality,
             const char   *fileout,
             l_int32       x,
             l_int32       y,
             l_int32       res,
             L_PDF_DATA  **plpd,
             l_int32       position,
             const char   *title)
{
l_uint8  *data;
l_int32   ret;
size_t    nbytes;

    PROCNAME("convertToPdf");

    if (!filein)
        return ERROR_INT("filein not defined", procName, 1);
    if (!plpd || (position == L_LAST_IMAGE)) {
        if (!fileout)
            return ERROR_INT("fileout not defined", procName, 1);
    }
    if (type != L_G4_ENCODE && type != L_JPEG_ENCODE &&
        type != L_FLATE_ENCODE)
        return ERROR_INT("invalid conversion type", procName, 1);

    if (convertToPdfData(filein, type, quality, &data, &nbytes, x, y,
                         res, plpd, position, title))
        return ERROR_INT("pdf data not made", procName, 1);

    if (!plpd || (position == L_LAST_IMAGE)) {
        ret = l_binaryWrite(fileout, "w", data, nbytes);
        FREE(data);
        if (ret)
            return ERROR_INT("pdf data not written to file", procName, 1);
    }

    return 0;
}


/*!
 *  convertImageDataToPdf()
 *
 *      Input:  imdata (array of formatted image data; e.g., png, jpeg)
 *              size (size of image data)
 *              type (L_G4_ENCODE, L_JPEG_ENCODE, L_FLATE_ENCODE)
 *              quality (used for JPEG only; 0 for default (75))
 *              fileout (output pdf file; only required on last image on page)
 *              x, y (location of lower-left corner of image, in pixels,
 *                    relative to the PostScript origin (0,0) at
 *                    the lower-left corner of the page)
 *              res (override the resolution of the input image, in ppi;
 *                   use 0 to respect the resolution embedded in the input)
 *              &lpd (ptr to lpd, which is created on the first invocation
 *                    and returned until last image is processed, at which
 *                    time it is destroyed)
 *              position (in image sequence: L_FIRST_IMAGE, L_NEXT_IMAGE,
 *                       L_LAST_IMAGE)
 *              title (<optional> pdf title; taken from the first image
 *                     placed on a page; e.g., an input image filename)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If @res == 0 and the input resolution field is 0,
 *          this will use DEFAULT_INPUT_RES.
 *      (2) See comments in convertToPdf().
 */
l_int32
convertImageDataToPdf(l_uint8      *imdata,
                      size_t        size,
                      l_int32       type,
                      l_int32       quality,
                      const char   *fileout,
                      l_int32       x,
                      l_int32       y,
                      l_int32       res,
                      L_PDF_DATA  **plpd,
                      l_int32       position,
                      const char   *title)
{
l_int32  ret;
PIX     *pix;

    PROCNAME("convertImageDataToPdf");

    if (!imdata)
        return ERROR_INT("image data not defined", procName, 1);
    if (type != L_G4_ENCODE && type != L_JPEG_ENCODE &&
        type != L_FLATE_ENCODE)
        return ERROR_INT("invalid conversion type", procName, 1);
    if (!plpd || (position == L_LAST_IMAGE)) {
        if (!fileout)
            return ERROR_INT("fileout not defined", procName, 1);
    }

    if ((pix = pixReadMem(imdata, size)) == NULL)
        return ERROR_INT("pix not read", procName, 1);
    ret = pixConvertToPdf(pix, type, quality, fileout, x, y, res,
                          plpd, position, title);
    pixDestroy(&pix);
    return ret;
}


/*!
 *  convertToPdfData()
 *
 *      Input:  filein (input image file -- any format)
 *              type (L_G4_ENCODE, L_JPEG_ENCODE, L_FLATE_ENCODE)
 *              quality (used for JPEG only; 0 for default (75))
 *              &data (<return> pdf data in memory)
 *              &nbytes (<return> number of bytes in pdf data)
 *              x, y (location of lower-left corner of image, in pixels,
 *                    relative to the PostScript origin (0,0) at
 *                    the lower-left corner of the page)
 *              res (override the resolution of the input image, in ppi;
 *                   use 0 to respect the resolution embedded in the input)
 *              &lpd (ptr to lpd, which is created on the first invocation
 *                    and returned until last image is processed, at which
 *                    time it is destroyed)
 *              position (in image sequence: L_FIRST_IMAGE, L_NEXT_IMAGE,
 *                       L_LAST_IMAGE)
 *              title (<optional> pdf title; taken from the first image
 *                     placed on a page; e.g., an input image filename)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If @res == 0 and the input resolution field is 0,
 *          this will use DEFAULT_INPUT_RES.
 *      (2) See comments in convertToPdf().
 */
l_int32
convertToPdfData(const char   *filein,
                 l_int32       type,
                 l_int32       quality,
                 l_uint8     **pdata,
                 size_t       *pnbytes,
                 l_int32       x,
                 l_int32       y,
                 l_int32       res,
                 L_PDF_DATA  **plpd,
                 l_int32       position,
                 const char   *title)
{
PIX  *pix;

    PROCNAME("convertToPdfData");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    *pdata = NULL;
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);
    *pnbytes = 0;
    if (!filein)
        return ERROR_INT("filein not defined", procName, 1);
    if (type != L_G4_ENCODE && type != L_JPEG_ENCODE &&
        type != L_FLATE_ENCODE)
        return ERROR_INT("invalid conversion type", procName, 1);

    if ((pix = pixRead(filein)) == NULL)
        return ERROR_INT("pix not made", procName, 1);

    pixConvertToPdfData(pix, type, quality, pdata, pnbytes,
                        x, y, res, plpd, position, title);
    pixDestroy(&pix);
    return 0;
}


/*!
 *  convertImageDataToPdfData()
 *
 *      Input:  imdata (array of formatted image data; e.g., png, jpeg)
 *              size (size of image data)
 *              type (L_G4_ENCODE, L_JPEG_ENCODE, L_FLATE_ENCODE)
 *              quality (used for JPEG only; 0 for default (75))
 *              &data (<return> pdf data in memory)
 *              &nbytes (<return> number of bytes in pdf data)
 *              x, y (location of lower-left corner of image, in pixels,
 *                    relative to the PostScript origin (0,0) at
 *                    the lower-left corner of the page)
 *              res (override the resolution of the input image, in ppi;
 *                   use 0 to respect the resolution embedded in the input)
 *              &lpd (ptr to lpd, which is created on the first invocation
 *                    and returned until last image is processed, at which
 *                    time it is destroyed)
 *              position (in image sequence: L_FIRST_IMAGE, L_NEXT_IMAGE,
 *                       L_LAST_IMAGE)
 *              title (<optional> pdf title; taken from the first image
 *                     placed on a page; e.g., an input image filename)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If @res == 0 and the input resolution field is 0,
 *          this will use DEFAULT_INPUT_RES.
 *      (2) See comments in convertToPdf().
 */
l_int32
convertImageDataToPdfData(l_uint8      *imdata,
                          size_t        size,
                          l_int32       type,
                          l_int32       quality,
                          l_uint8     **pdata,
                          size_t       *pnbytes,
                          l_int32       x,
                          l_int32       y,
                          l_int32       res,
                          L_PDF_DATA  **plpd,
                          l_int32       position,
                          const char   *title)
{
l_int32  ret;
PIX     *pix;

    PROCNAME("convertImageDataToPdfData");

    if (!imdata)
        return ERROR_INT("image data not defined", procName, 1);
    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    *pdata = NULL;
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);
    *pnbytes = 0;
    if (plpd) {  /* part of multi-page invocation */
        if (position == L_FIRST_IMAGE)
            *plpd = NULL;
    }

    if ((pix = pixReadMem(imdata, size)) == NULL)
        return ERROR_INT("pix not read", procName, 1);
    ret = pixConvertToPdfData(pix, type, quality, pdata, pnbytes,
                              x, y, res, plpd, position, title);
    pixDestroy(&pix);
    return ret;
}


/*!
 *  pixConvertToPdf()
 *
 *      Input:  pix
 *              type (L_G4_ENCODE, L_JPEG_ENCODE, L_FLATE_ENCODE)
 *              quality (used for JPEG only; 0 for default (75))
 *              fileout (output pdf file; only required on last image on page)
 *              x, y (location of lower-left corner of image, in pixels,
 *                    relative to the PostScript origin (0,0) at
 *                    the lower-left corner of the page)
 *              res (override the resolution of the input image, in ppi;
 *                   use 0 to respect the resolution embedded in the input)
 *              &lpd (ptr to lpd, which is created on the first invocation
 *                    and returned until last image is processed)
 *              position (in image sequence: L_FIRST_IMAGE, L_NEXT_IMAGE,
 *                       L_LAST_IMAGE)
 *              title (<optional> pdf title; taken from the first image
 *                     placed on a page; e.g., an input image filename)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If @res == 0 and the input resolution field is 0,
 *          this will use DEFAULT_INPUT_RES.
 *      (2) This only writes data to fileout if it is the last
 *          image to be written on the page.
 *      (3) See comments in convertToPdf().
 */
l_int32
pixConvertToPdf(PIX          *pix,
                l_int32       type,
                l_int32       quality,
                const char   *fileout,
                l_int32       x,
                l_int32       y,
                l_int32       res,
                L_PDF_DATA  **plpd,
                l_int32       position,
                const char   *title)
{
l_uint8  *data;
l_int32   ret;
size_t    nbytes;

    PROCNAME("pixConvertToPdf");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (type != L_G4_ENCODE && type != L_JPEG_ENCODE &&
        type != L_FLATE_ENCODE)
        return ERROR_INT("invalid conversion type", procName, 1);
    if (!plpd || (position == L_LAST_IMAGE)) {
        if (!fileout)
            return ERROR_INT("fileout not defined", procName, 1);
    }

    if (pixConvertToPdfData(pix, type, quality, &data, &nbytes,
                            x, y, res, plpd, position, title))
        return ERROR_INT("pdf data not made", procName, 1);

    if (!plpd || (position == L_LAST_IMAGE)) {
        ret = l_binaryWrite(fileout, "w", data, nbytes);
        FREE(data);
        if (ret)
            return ERROR_INT("pdf data not written to file", procName, 1);
    }
    return 0;
}


/*!
 *  pixConvertToPdfData()
 *
 *      Input:  pix (all depths; cmap OK)
 *              type (L_G4_ENCODE, L_JPEG_ENCODE, L_FLATE_ENCODE)
 *              quality (used for JPEG only; 0 for default (75))
 *              &data (<return> pdf array)
 *              &nbytes (<return> number of bytes in pdf array)
 *              x, y (location of lower-left corner of image, in pixels,
 *                    relative to the PostScript origin (0,0) at
 *                    the lower-left corner of the page)
 *              res (override the resolution of the input image, in ppi;
 *                   use 0 to respect the resolution embedded in the input)
 *              &lpd (ptr to lpd, which is created on the first invocation
 *                    and returned until last image is processed)
 *              position (in image sequence: L_FIRST_IMAGE, L_NEXT_IMAGE,
 *                       L_LAST_IMAGE)
 *              title (<optional> pdf title; taken from the first image
 *                     placed on a page; e.g., an input image filename)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If @res == 0 and the input resolution field is 0,
 *          this will use DEFAULT_INPUT_RES.
 *      (2) This only writes @data if it is the last image to be
 *          written on the page.
 *      (3) See comments in convertToPdf().
 */
l_int32
pixConvertToPdfData(PIX          *pix,
                    l_int32       type,
                    l_int32       quality,
                    l_uint8     **pdata,
                    size_t       *pnbytes,
                    l_int32       x,
                    l_int32       y,
                    l_int32       res,
                    L_PDF_DATA  **plpd,
                    l_int32       position,
                    const char   *title)
{
l_int32             pixres, w, h, d, ret;
l_float32           xpt, ypt, wpt, hpt;
L_COMPRESSED_DATA  *cid = NULL;
L_PDF_DATA         *lpd = NULL;
PIXCMAP            *cmap;

    PROCNAME("pixConvertToPdfData");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    *pdata = NULL;
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);
    *pnbytes = 0;
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (plpd) {  /* part of multi-page invocation */
        if (position == L_FIRST_IMAGE)
            *plpd = NULL;
    }

        /* Sanity check on requested encoding */
    d = pixGetDepth(pix);
    cmap = pixGetColormap(pix);
    if (cmap && type != L_FLATE_ENCODE) {
        L_WARNING("pix has cmap; using flate encoding", procName);
        type = L_FLATE_ENCODE;
    }
    else if (d < 8 && type == L_JPEG_ENCODE) {
        L_WARNING("pix has < 8 bpp; using flate encoding", procName);
        type = L_FLATE_ENCODE;
    }
    else if (d > 1 && type == L_G4_ENCODE) {
        L_WARNING("pix has > 1 bpp; using flate encoding", procName);
        type = L_FLATE_ENCODE;
    }

    if (type == L_JPEG_ENCODE) {
        if ((cid = pixGenerateJpegData(pix, 0, quality)) == NULL)
            return ERROR_INT("jpeg data not made", procName, 1);
        pixres = cid->res;
        w = cid->w;
        h = cid->h;
    }
    else if (type == L_G4_ENCODE) {
        if ((cid = pixGenerateG4Data(pix, 0)) == NULL)
            return ERROR_INT("g4 data not made", procName, 1);
        pixres = cid->res;
        w = cid->w;
        h = cid->h;
    }
    else if (type == L_FLATE_ENCODE) {
        if ((cid = pixGenerateFlateData(pix, 0)) == NULL)
            return ERROR_INT("flate data not made", procName, 1);
        pixres = cid->res;
        w = cid->w;
        h = cid->h;
    }
    else
        return ERROR_INT("invalid conversion type", procName, 1);

        /* Get media box in pts.  Guess the input image resolution
         * based on the input parameter @res, the resolution data in
         * the pix, and the size of the image. */
    if (res <= 0.0) {
        if (pixres > 0)
            res = pixres;
        else
            res = DEFAULT_INPUT_RES;
    }
    xpt = x * 72. / res;
    ypt = y * 72. / res;
    wpt = w * 72. / res;
    hpt = h * 72. / res;

        /* Set up lpd */
    if (!plpd) {  /* single image */
        if ((lpd = pdfdataCreate(title)) == NULL)
            return ERROR_INT("lpd not made", procName, 1);
    }
    else if (position == L_FIRST_IMAGE) {  /* first of multiple images */
        if ((lpd = pdfdataCreate(title)) == NULL)
            return ERROR_INT("lpd not made", procName, 1);
        *plpd = lpd;
    }
    else  /* not the first of multiple images */
        lpd = *plpd;

        /* Add the data to the lpd */
    ptraAdd(lpd->cida, cid);
    lpd->n++;
    ptaAddPt(lpd->xy, xpt, ypt);
    ptaAddPt(lpd->wh, wpt, hpt);

        /* If a single image or the last of multiple images,
         * generate the pdf and destroy the lpd */
    if (!plpd || (position == L_LAST_IMAGE)) {
        ret = l_generatePdf(pdata, pnbytes, lpd);
        pdfdataDestroy(&lpd);
        if (plpd) *plpd = NULL;
        if (ret)
            return ERROR_INT("pdf output not made", procName, 1);
    }

    return 0;
}


/*!
 *  pixWriteStreamPdf()
 *
 *      Input:  fp (stream opened for writing)
 *              pix (all depths, cmap OK)
 *              res (override the resolution of the input image, in ppi;
 *                   use 0 to respect the resolution embedded in the input)
 *              title (<optional> pdf title; taken from the first image
 *                     placed on a page; e.g., an input image filename)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is the simplest interface for writing a single image
 *          with pdf encoding.  It uses G4 encoding for 1 bpp,
 *          JPEG encoding for 8 bpp (no cmap) and 32 bpp, and FLATE
 *          encoding for everything else.
 */
l_int32
pixWriteStreamPdf(FILE        *fp,
                  PIX         *pix,
                  l_int32      res,
                  const char  *title)
{
l_uint8  *data;
l_int32   ret, d, type;
size_t    nbytes;
PIXCMAP  *cmap;

    PROCNAME("pixWriteStreamPdf");

    if (!fp)
        return ERROR_INT("stream not opened", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    d = pixGetDepth(pix);
    cmap = pixGetColormap(pix);
    if (d == 1)
        type = L_G4_ENCODE;
    else if (cmap || d == 2 || d == 4 || d == 16)
        type = L_FLATE_ENCODE;
    else  /* d == 8 (no cmap) or d == 32 */
        type = L_JPEG_ENCODE;
    if (pixConvertToPdfData(pix, type, 75, &data, &nbytes,
                            0, 0, res, NULL, 0, title))
        return ERROR_INT("pdf data not made", procName, 1);
    ret = fwrite(data, 1, nbytes, fp);

    FREE(data);
    if (ret)
        return ERROR_INT("pdf data not written to stream", procName, 1);
    return 0;
}


/*---------------------------------------------------------------------*
 *            Segmented multi-page, multi-image converter              *
 *---------------------------------------------------------------------*/
/*!
 *  convertSegmentedFilesToPdf()
 *
 *      Input:  directory name (containing images)
 *              substr (<optional> substring filter on filenames; can be NULL)
 *              res (input resolution of all images)
 *              type (compression type for non-image regions; the
 *                    image regions are always compressed with L_JPEG_ENCODE)
 *              thresh (used for converting gray --> 1 bpp with L_G4_ENCODE)
 *              boxaa (of image regions)
 *              quality (used for JPEG only; 0 for default (75))
 *              scalefactor (scaling factor applied to each image region)
 *              title (<optional> pdf title; if null, taken from the first
 *                     image filename)
 *              fileout (pdf file of all images)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If @substr is not NULL, only image filenames that contain
 *          the substring can be used.  If @substr == NULL, all files
 *          in the directory are used.
 *      (2) The files in the directory, after optional filtering by
 *          the substring, are lexically sorted in increasing order
 *          before concatenation.
 *      (3) The images are encoded with G4 if 1 bpp; JPEG if 8 bpp without
 *          colormap and many colors, or 32 bpp; FLATE for anything else.
 *      (4) The boxaa contains one boxa of "image regions" for each
 *          image file.  The boxa must all exist, but they can be empty.
 *          They must be aligned with the sorted set of images.
 *      (5) The scalefactor is applied to each image region.  It is
 *          typically < 1.0, to save bytes in the final pdf, because
 *          the resolution is often not critical in non-text regions.
 *      (6) The non-image regions are automatically scaled up by 2x and
 *          thresholded if the encoding type is G4.  If the non-image
 *          regions are not encoded with G4, no scaling is performed on them.
 */
l_int32
convertSegmentedFilesToPdf(const char  *dirname,
                           const char  *substr,
                           l_int32      res,
                           l_int32      type,
                           l_int32      thresh,
                           BOXAA       *baa,
                           l_int32      quality,
                           l_float32    scalefactor,
                           const char  *title,
                           const char  *fileout)
{
char     *fname;
l_uint8  *imdata, *data;
l_int32   i, npages, nboxa, nboxes, ret;
size_t    imbytes, databytes;
BOXA     *boxa;
L_BYTEA  *ba;
L_PTRA   *pa_data;
SARRAY   *sa;

    PROCNAME("convertSegmentedFilesToPdf");

    if (!dirname)
        return ERROR_INT("dirname not defined", procName, 1);
    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);
    if (!fileout)
        return ERROR_INT("fileout not defined", procName, 1);

    if ((sa = getSortedPathnamesInDirectory(dirname, substr, 0, 0)) == NULL)
        return ERROR_INT("sa not made", procName, 1);

        /* Generate and save all the encoded pdf strings */
    npages = sarrayGetCount(sa);
    nboxa = boxaaGetCount(baa);
    if (npages != nboxa) {
        sarrayDestroy(&sa);
        return ERROR_INT("npages != nboxa", procName, 1);
    }
    pa_data = ptraCreate(npages);
    for (i = 0; i < npages; i++) {
        fname = sarrayGetString(sa, i, L_NOCOPY);
        boxa = boxaaGetBoxa(baa, i, L_CLONE);
        nboxes = boxaGetCount(boxa);
        if (nboxes == 0)
            boxaDestroy(&boxa);
        ret = convertToPdfDataSegmented(fname, res, type, thresh, boxa,
                                        quality, scalefactor,
                                        &imdata, &imbytes);
        boxaDestroy(&boxa);  /* safe; in case nboxes > 0 */
        if (ret) {
            L_ERROR_STRING("pdf encoding failed for %s", procName, fname);
            continue;
        }
        ba = l_byteaInitFromMem(imdata, imbytes);
        if (imdata) FREE(imdata);
        ptraAdd(pa_data, ba);
    }
    sarrayDestroy(&sa);

    ptraGetActualCount(pa_data, &npages);
    if (npages == 0) {
        L_ERROR("no pdf files made", procName);
        ptraDestroy(&pa_data, FALSE, FALSE);
        return 1;
    }

        /* Concatenate */
    ret = ptraConcatenatePdfToData(pa_data, NULL, &data, &databytes);

        /* Clean up */
    ptraGetActualCount(pa_data, &npages);  /* recalculate in case it changes */
    for (i = 0; i < npages; i++) {
        ba = (L_BYTEA *)ptraRemove(pa_data, i, L_NO_COMPACTION);
        l_byteaDestroy(&ba);
    }
    ptraDestroy(&pa_data, FALSE, FALSE);

    if (ret) {
        if (data) FREE(data);
        return ERROR_INT("pdf data not made", procName, 1);
    }

    ret = l_binaryWrite(fileout, "w", data, databytes);
    FREE(data);
    if (ret)
        L_ERROR("pdf data not written to file", procName);
    return ret;
}


/*---------------------------------------------------------------------*
 *            Segmented single page, multi-image converters            *
 *---------------------------------------------------------------------*/
/*!
 *  convertToPdfSegmented()
 *
 *      Input:  filein (input image file -- any format)
 *              res (input image resolution; typ. 300 ppi; use 0 for default)
 *              type (compression type for non-image regions; the
 *                    image regions are always compressed with L_JPEG_ENCODE)
 *              thresh (used for converting gray --> 1 bpp with L_G4_ENCODE)
 *              boxa (of image regions; can be null)
 *              quality (used for jpeg image regions; 0 for default)
 *              scalefactor (used for jpeg regions; must be <= 1.0)
 *              fileout (output pdf file)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If there are no image regions, set @boxa == NULL;
 *          @quality and @scalefactor are ignored.
 *      (2) Typically, @scalefactor is < 1.0, because the image regions
 *          can be rendered at a lower resolution (for better compression)
 *          than the text regions.  If @scalefactor == 0, we use 1.0.
 *          If the input image is 1 bpp and scalefactor < 1.0, we
 *          use scaleToGray() to downsample the image regions to gray
 *          before compressing them.
 *      (3) If the compression type for non-image regions is L_G4_ENCODE
 *          and bpp > 1, the image is upscaled 2x and thresholded
 *          to 1 bpp.  That is the only situation where @thresh is used.
 *      (4) The parameter @quality is only used for image regions.
 *          If @type == L_JPEG_ENCODE, default jpeg quality (75) is
 *          used for the non-image regions.
 *      (5) Processing matrix for non-image regions.
 *
 *          Input           G4              JPEG                FLATE
 *          ----------|---------------------------------------------------
 *          1 bpp     |  1x, 1 bpp       1x flate, 1 bpp     1x, 1 bpp
 *                    |
 *          cmap      |  2x, 1 bpp       1x flate, cmap      1x, cmap
 *                    |
 *          2,4 bpp   |  2x, 1 bpp       1x flate            1x, 2,4 bpp
 *          no cmap   |                  2,4 bpp
 *                    |
 *          8,32 bpp  |  2x, 1 bpp       1x (jpeg)           1x, 8,32 bpp
 *          no cmap   |                  8,32 bpp
 *
 *          Summary:
 *          (a) if G4 is requested, G4 is used, with 2x upscaling
 *              for all cases except 1 bpp.
 *          (b) if JPEG is requested, use flate encoding for all cases
 *              except 8 bpp without cmap and 32 bpp (rgb).
 *          (c) if FLATE is requested, use flate with no transformation
 *              of the raster data.
 *      (6) Calling options/sequence for these functions:
 *              file  -->  file      (convertToPdfSegmented)
 *                  pix  -->  file      (pixConvertToPdfSegmented)
 *                      pix  -->  data      (pixConvertToPdfDataSegmented)
 *              file  -->  data      (convertToPdfDataSegmented)
 *                      pix  -->  data      (pixConvertToPdfDataSegmented)
 */
l_int32
convertToPdfSegmented(const char  *filein,
                      l_int32      res,
                      l_int32      type,
                      l_int32      thresh,
                      BOXA        *boxa,
                      l_int32      quality,
                      l_float32    scalefactor,
                      const char  *fileout)
{
l_int32  ret;
PIX     *pixs;

    PROCNAME("convertToPdfSegmented");

    if (!filein)
        return ERROR_INT("filein not defined", procName, 1);
    if (!fileout)
        return ERROR_INT("fileout not defined", procName, 1);
    if (type != L_G4_ENCODE && type != L_JPEG_ENCODE &&
        type != L_FLATE_ENCODE)
        return ERROR_INT("invalid conversion type", procName, 1);
    if (boxa && scalefactor > 1.0) {
        L_WARNING("setting scalefactor to 1.0", procName);
        scalefactor = 1.0;
    }

    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pixs not made", procName, 1);

    ret = pixConvertToPdfSegmented(pixs, res, type, thresh, boxa, quality,
                                   scalefactor, fileout, filein);
    pixDestroy(&pixs);
    return ret;
}


/*!
 *  pixConvertToPdfSegmented()
 *
 *      Input:  pixs (any depth, cmap OK)
 *              res (input image resolution; typ. 300 ppi; use 0 for default)
 *              type (compression type for non-image regions; the
 *                    image regions are always compressed with L_JPEG_ENCODE)
 *              thresh (used for converting gray --> 1 bpp with L_G4_ENCODE)
 *              boxa (of image regions; can be null)
 *              quality (used for jpeg image regions; 0 for default)
 *              scalefactor (used for jpeg regions; must be <= 1.0)
 *              fileout (output pdf file)
 *              title (<optional> pdf title; typically taken from the
 *                     input file for the pix)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) See convertToPdfSegmented() for details.
 */
l_int32
pixConvertToPdfSegmented(PIX         *pixs,
                         l_int32      res,
                         l_int32      type,
                         l_int32      thresh,
                         BOXA        *boxa,
                         l_int32      quality,
                         l_float32    scalefactor,
                         const char  *fileout,
                         const char  *title)
{
l_uint8  *data;
l_int32   ret;
size_t    nbytes;

    PROCNAME("pixConvertToPdfSegmented");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!fileout)
        return ERROR_INT("fileout not defined", procName, 1);
    if (type != L_G4_ENCODE && type != L_JPEG_ENCODE &&
        type != L_FLATE_ENCODE)
        return ERROR_INT("invalid conversion type", procName, 1);
    if (boxa && scalefactor > 1.0) {
        L_WARNING("setting scalefactor to 1.0", procName);
        scalefactor = 1.0;
    }

    ret = pixConvertToPdfDataSegmented(pixs, res, type, thresh, boxa, quality,
                                       scalefactor, &data, &nbytes, title);
    if (ret)
        return ERROR_INT("pdf generation failure", procName, 1);

    ret = l_binaryWrite(fileout, "w", data, nbytes);
    if (data) FREE(data);
    return ret;
}


/*!
 *  convertToPdfDataSegmented()
 *
 *      Input:  filein (input image file -- any format)
 *              res (input image resolution; typ. 300 ppi; use 0 for default)
 *              type (compression type for non-image regions; the
 *                    image regions are always compressed with L_JPEG_ENCODE)
 *              thresh (used for converting gray --> 1 bpp with L_G4_ENCODE)
 *              boxa (of image regions; can be null)
 *              quality (used for jpeg image regions; 0 for default)
 *              scalefactor (used for jpeg regions; must be <= 1.0)
 *              &data (<return> pdf data in memory)
 *              &nbytes (<return> number of bytes in pdf data)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If there are no image regions, set @boxa == NULL;
 *          @quality and @scalefactor are ignored.
 *      (2) Typically, @scalefactor is < 1.0.  The image regions are
 */
l_int32
convertToPdfDataSegmented(const char  *filein,
                          l_int32      res,
                          l_int32      type,
                          l_int32      thresh,
                          BOXA        *boxa,
                          l_int32      quality,
                          l_float32    scalefactor,
                          l_uint8    **pdata,
                          size_t      *pnbytes)
{
l_int32  ret;
PIX     *pixs;

    PROCNAME("convertToPdfDataSegmented");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    *pdata = NULL;
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);
    *pnbytes = 0;
    if (!filein)
        return ERROR_INT("filein not defined", procName, 1);
    if (type != L_G4_ENCODE && type != L_JPEG_ENCODE &&
        type != L_FLATE_ENCODE)
        return ERROR_INT("invalid conversion type", procName, 1);
    if (boxa && scalefactor > 1.0) {
        L_WARNING("setting scalefactor to 1.0", procName);
        scalefactor = 1.0;
    }

    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pixs not made", procName, 1);

    ret = pixConvertToPdfDataSegmented(pixs, res, type, thresh, boxa,
                                       quality, scalefactor,
                                       pdata, pnbytes, filein);
    pixDestroy(&pixs);
    return ret;
}


/*!
 *  pixConvertToPdfDataSegmented()
 *
 *      Input:  pixs (any depth, cmap OK)
 *              res (input image resolution; typ. 300 ppi; use 0 for default)
 *              type (compression type for non-image regions; the
 *                    image regions are always compressed with L_JPEG_ENCODE)
 *              thresh (used for converting gray --> 1 bpp with L_G4_ENCODE)
 *              boxa (of image regions; can be null)
 *              quality (used for jpeg image regions; 0 for default)
 *              scalefactor (used for jpeg regions; must be <= 1.0)
 *              &data (<return> pdf data in memory)
 *              &nbytes (<return> number of bytes in pdf data)
 *              title (<optional> pdf title; typically taken from the
 *                     input file for the pix)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) See convertToPdfSegmented() for details.
 */
l_int32
pixConvertToPdfDataSegmented(PIX         *pixs,
                             l_int32      res,
                             l_int32      type,
                             l_int32      thresh,
                             BOXA        *boxa,
                             l_int32      quality,
                             l_float32    scalefactor,
                             l_uint8    **pdata,
                             size_t      *pnbytes,
                             const char  *title)
{
l_int32      i, nbox, seq, bx, by, bw, bh, upscale;
l_float32    scale;
BOX         *box, *boxc, *box2;
PIX         *pix, *pixt1, *pixt2, *pixt3, *pixt4, *pixt5, *pixt6;
PIXCMAP     *cmap;
L_PDF_DATA  *lpd;

    PROCNAME("pixConvertToPdfDataSegmented");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    *pdata = NULL;
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);
    *pnbytes = 0;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (type != L_G4_ENCODE && type != L_JPEG_ENCODE &&
        type != L_FLATE_ENCODE)
        return ERROR_INT("invalid conversion type", procName, 1);
    if (boxa && (scalefactor <= 0.0 || scalefactor > 1.0)) {
        L_WARNING("setting scalefactor to 1.0", procName);
        scalefactor = 1.0;
    }

        /* Adjust scalefactor so that the product with res gives an integer */
    if (res <= 0)
        res = DEFAULT_INPUT_RES;
    scale = (l_float32)((l_int32)(scalefactor * res + 0.5)) / (l_float32)res;
    cmap = pixGetColormap(pixs);

        /* Simple case: single image to be encoded */
    if (!boxa || boxaGetCount(boxa) == 0) {
        if (pixGetDepth(pixs) > 1 && type == L_G4_ENCODE) {
            if (cmap)
                pixt1 = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
            else
                pixt1 = pixConvertTo8(pixs, FALSE);
            pixt2 = pixScaleGray2xLIThresh(pixt1, thresh);
            pixConvertToPdfData(pixt2, type, quality, pdata, pnbytes,
                                0, 0, 2 * res, NULL, 0, title);
            pixDestroy(&pixt1);
            pixDestroy(&pixt2);
        }
        else {
            pixConvertToPdfData(pixs, type, quality, pdata, pnbytes,
                                0, 0, res, NULL, 0, title);
        }
        return 0;
    }

        /* Multiple images to be encoded.  If @type == L_G4_ENCODE,
         * jpeg encode a version of pixs that is blanked in the non-image
         * regions, and paint the scaled non-image part onto it through a mask.
         * Otherwise, we must put the non-image part down first and
         * then render all the image regions separately on top of it,
         * at their own resolution. */
    pixt1 = pixSetBlackOrWhiteBoxa(pixs, boxa, L_SET_WHITE);  /* non-image */
    nbox = boxaGetCount(boxa);
    if (type == L_G4_ENCODE) {
        pixt2 = pixCreateTemplate(pixs);  /* only image regions */
        pixSetBlackOrWhite(pixt2, L_SET_WHITE);
        for (i = 0; i < nbox; i++) {
             box = boxaGetBox(boxa, i, L_CLONE);
             pix = pixClipRectangle(pixs, box, &boxc);
             boxGetGeometry(boxc, &bx, &by, &bw, &bh);
             pixRasterop(pixt2, bx, by, bw, bh, PIX_SRC, pix, 0, 0);
             pixDestroy(&pix);
             boxDestroy(&box);
             boxDestroy(&boxc);
        }
        pixt3 = pixRemoveColormap(pixt2, REMOVE_CMAP_BASED_ON_SRC);
        if (pixGetDepth(pixt3) == 1)
            pixt4 = pixScaleToGray(pixt3, scale);
        else
            pixt4 = pixScale(pixt3, scale, scale);
        pixConvertToPdfData(pixt4, L_JPEG_ENCODE, quality, pdata, pnbytes,
                            0, 0, (l_int32)(scale * res),
                            &lpd, L_FIRST_IMAGE, title);

        if (pixGetDepth(pixt1) == 1) {
            pixt5 = pixClone(pixt1);
            upscale = 1;
        }
        else {
            pixt6 = pixConvertTo8(pixt1, 0);
            pixt5 = pixScaleGray2xLIThresh(pixt6, thresh);
            pixDestroy(&pixt6);
            upscale = 2;
        }
        pixConvertToPdfData(pixt5, L_G4_ENCODE, quality, pdata, pnbytes,
                            0, 0, upscale * res, &lpd, L_LAST_IMAGE, title);
        pixDestroy(&pixt2);
        pixDestroy(&pixt3);
        pixDestroy(&pixt4);
        pixDestroy(&pixt5);
    }
    else {
            /* Put the non-image part down first.  This is the full
               size of the page, so we can use it to find the page
               height in pixels, which is required for determining
               the LL corner of the image relative to the LL corner
               of the page. */
        pixConvertToPdfData(pixt1, type, quality, pdata, pnbytes, 0, 0,
                            res, &lpd, L_FIRST_IMAGE, title);
        for (i = 0; i < nbox; i++) {
            box = boxaGetBox(boxa, i, L_CLONE);
            pixt2 = pixClipRectangle(pixs, box, &boxc);
            pixt3 = pixRemoveColormap(pixt2, REMOVE_CMAP_BASED_ON_SRC);
            if (pixGetDepth(pixt3) == 1)
                pixt4 = pixScaleToGray(pixt3, scale);
            else
                pixt4 = pixScale(pixt3, scale, scale);
            box2 = boxTransform(boxc, 0, 0, scale, scale);
            boxGetGeometry(box2, &bx, &by, NULL, &bh);
            seq = (i == nbox - 1) ? L_LAST_IMAGE : L_NEXT_IMAGE;
            pixConvertToPdfData(pixt4, L_JPEG_ENCODE, quality, pdata, pnbytes,
                                bx, by, (l_int32)(scale * res),
                                &lpd, seq, title);
            pixDestroy(&pixt2);
            pixDestroy(&pixt3);
            pixDestroy(&pixt4);
            boxDestroy(&box);
            boxDestroy(&boxc);
            boxDestroy(&box2);
        }
    }

    pixDestroy(&pixt1);
    return 0;
}


/*---------------------------------------------------------------------*
 *         Helper functions for generating the output pdf string       *
 *---------------------------------------------------------------------*/
/*!
 *  l_generatePdf()
 *
 *      Input:  &data (<return> pdf array)
 *              &nbytes (<return> number of bytes in pdf array)
 *              lpd (all the required input image data)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) On error, no data is returned.
 *      (2) The objects are:
 *            1: Catalog
 *            2: Info
 *            3: Pages
 *            4: Page
 *            5: Contents  (rendering command)
 *            6 to 6+n-1: n XObjects
 *            6+n to 6+n+m-1: m colormaps
 */
static l_int32
l_generatePdf(l_uint8    **pdata,
              size_t      *pnbytes,
              L_PDF_DATA  *lpd)
{
    PROCNAME("l_generatePdf");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    *pdata = NULL;
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);
    *pnbytes = 0;
    if (!lpd)
        return ERROR_INT("lpd not defined", procName, 1);

    generateFixedStringsPdf(lpd);
    generateMediaboxPdf(lpd);
    generatePageStringPdf(lpd);
    generateContentStringPdf(lpd);
    generatePreXStringsPdf(lpd);
    generateColormapStringsPdf(lpd);
    generateTrailerPdf(lpd);
    return generateOutputDataPdf(pdata, pnbytes, lpd);
}


static void
generateFixedStringsPdf(L_PDF_DATA  *lpd)
{
char     buf[L_SMALLBUF];
char    *version, *datestr;
SARRAY  *sa;

        /* Accumulate data for the header and objects 1-3 */
    lpd->id = stringNew("%PDF-1.2\n");
    numaAddNumber(lpd->objsize, strlen(lpd->id));

    lpd->obj1 = stringNew("1 0 obj\n"
                          "<<\n"
                          "/Type /Catalog\n"
                          "/Pages 3 0 R\n"
                          ">>\n"
                          "endobj\n");
    numaAddNumber(lpd->objsize, strlen(lpd->obj1));

    sa = sarrayCreate(0);
    sarrayAddString(sa, (char *)"2 0 obj\n"
                                 "<<\n", L_COPY);
    if (lpd->title) {
        snprintf(buf, sizeof(buf), "/Title (%s)\n", lpd->title);
        sarrayAddString(sa, (char *)buf, L_COPY);
    }
    if (var_WRITE_DATE_AND_VERSION) {
        version = getLeptonicaVersion();
        snprintf(buf, sizeof(buf),
                 "/Producer (leptonica: %s)\n", version);
        FREE(version);
    }
    else
        snprintf(buf, sizeof(buf), "/Producer (leptonica)\n");
    sarrayAddString(sa, (char *)buf, L_COPY);
    if (var_WRITE_DATE_AND_VERSION) {
        datestr = l_getFormattedDate();
        snprintf(buf, sizeof(buf), "/CreationDate (D:%s)\n", datestr);
        sarrayAddString(sa, (char *)buf, L_COPY);
        FREE(datestr);
    }
    sarrayAddString(sa, (char *)">>\n"
                                "endobj\n", L_COPY);
    lpd->obj2 = sarrayToString(sa, 0);
    numaAddNumber(lpd->objsize, strlen(lpd->obj2));
    sarrayDestroy(&sa);

    lpd->obj3 = stringNew("3 0 obj\n"
                          "<<\n"
                          "/Type /Pages\n"
                          "/Kids [ 4 0 R ]\n"
                          "/Count 1\n"
                          ">>\n");
    numaAddNumber(lpd->objsize, strlen(lpd->obj3));

        /* Do the post-datastream string */
    lpd->poststream = stringNew("\n"
                                "endstream\n"
                                "endobj\n");
    return;
}


static void
generateMediaboxPdf(L_PDF_DATA  *lpd)
{
l_int32    i;
l_float32  xpt, ypt, wpt, hpt, maxx, maxy;

        /* First get the full extent of all the images.
         * This is the mediabox, in pts. */
    maxx = maxy = 0;
    for (i = 0; i < lpd->n; i++) {
        ptaGetPt(lpd->xy, i, &xpt, &ypt);
        ptaGetPt(lpd->wh, i, &wpt, &hpt);
        maxx = L_MAX(maxx, xpt + wpt);
        maxy = L_MAX(maxy, ypt + hpt);
    }

    lpd->mediabox = boxCreate(0, 0, (l_int32)(maxx + 0.5),
                              (l_int32)(maxy + 0.5));

        /* ypt is in standard image coordinates: the location of
         * the UL image corner with respect to the UL media box corner.
         * Rewrite each ypt for PostScript coordinates: the location of
         * the LL image corner with respect to the LL media box corner. */
    for (i = 0; i < lpd->n; i++) {
        ptaGetPt(lpd->xy, i, &xpt, &ypt);
        ptaGetPt(lpd->wh, i, &wpt, &hpt);
        ptaSetPt(lpd->xy, i, xpt, maxy - ypt - hpt);
    }

    return;
}


static l_int32
generatePageStringPdf(L_PDF_DATA  *lpd)
{
char    *buf;
char    *xstr;
l_int32  bufsize, i, wpt, hpt;
SARRAY  *sa;
    
    PROCNAME("generatePageStringPdf");

        /* Allocate 1000 bytes for the boilerplate text, and
         * 50 bytes for each reference to an image in the
         * ProcSet array.  */
    bufsize = 1000 + 50 * lpd->n;
    if ((buf = (char *)CALLOC(bufsize, sizeof(char))) == NULL)
        return ERROR_INT("calloc fail for buf", procName, 1);

    boxGetGeometry(lpd->mediabox, NULL, NULL, &wpt, &hpt);
    sa = sarrayCreate(lpd->n);
    for (i = 0; i < lpd->n; i++) {
        snprintf(buf, bufsize, "/Im%d %d 0 R   ", i + 1, 6 + i);
        sarrayAddString(sa, buf, L_COPY);
    }
    if ((xstr = sarrayToString(sa, 0)) == NULL)
        return ERROR_INT("xstr not found", procName, 1);
    sarrayDestroy(&sa);
        
    snprintf(buf, bufsize, "4 0 obj\n"
                           "<<\n"
                           "/Type /Page\n"
                           "/Parent 3 0 R\n"
                           "/MediaBox [%d %d %d %d]\n"
                           "/Contents 5 0 R\n"
                           "/Resources\n"
                           "<<\n"
                           "/XObject << %s >>\n"
                           "/ProcSet [ /ImageB /ImageI /ImageC ]\n"
                           ">>\n"
                           ">>\n"
                           "endobj\n",
                           0, 0, wpt, hpt, xstr);

    lpd->obj4 = stringNew(buf);
    numaAddNumber(lpd->objsize, strlen(lpd->obj4));
    sarrayDestroy(&sa);
    FREE(buf);
    FREE(xstr);
    return 0;
}


static l_int32
generateContentStringPdf(L_PDF_DATA  *lpd)
{
char      *buf;
char      *cstr;
l_int32    i, bufsize;
l_float32  xpt, ypt, wpt, hpt;
SARRAY    *sa;
    
    PROCNAME("generateContentStringPdf");

    bufsize = 1000 + 200 * lpd->n;
    if ((buf = (char *)CALLOC(bufsize, sizeof(char))) == NULL)
        return ERROR_INT("calloc fail for buf", procName, 1);

    sa = sarrayCreate(lpd->n);
    for (i = 0; i < lpd->n; i++) {
        ptaGetPt(lpd->xy, i, &xpt, &ypt);
        ptaGetPt(lpd->wh, i, &wpt, &hpt);
        snprintf(buf, bufsize,
                 "q %.4f %.4f %.4f %.4f %.4f %.4f cm /Im%d Do Q\n",
                 wpt, 0.0, 0.0, hpt, xpt, ypt, i + 1);
        sarrayAddString(sa, buf, L_COPY);
    }
    if ((cstr = sarrayToString(sa, 0)) == NULL)
        return ERROR_INT("cstr not found", procName, 1);
    sarrayDestroy(&sa);
        
    snprintf(buf, bufsize, "5 0 obj\n"
                           "<< /Length %d >>\n"
                           "stream\n"
                           "%s"
                           "endstream\n"
                           "endobj\n",
                           (l_int32)strlen(cstr), cstr);

    lpd->obj5 = stringNew(buf);
    numaAddNumber(lpd->objsize, strlen(lpd->obj5));
    sarrayDestroy(&sa);
    FREE(buf);
    FREE(cstr);
    return 0;
}


static l_int32
generatePreXStringsPdf(L_PDF_DATA  *lpd)
{
char                buff[256];
char                buf[L_BIGBUF];
char               *cstr, *bstr, *fstr, *xstr;
l_int32             i, cmindex;
L_COMPRESSED_DATA  *cid;
SARRAY             *sa;
    
    PROCNAME("generatePreXStringsPdf");

    sa = lpd->saprex;
    cmindex = 6 + lpd->n;  /* starting value */
    for (i = 0; i < lpd->n; i++) {
        if ((cid = pdfdataGetCid(lpd, i)) == NULL)
            return ERROR_INT("cid not found", procName, 1);

        if (cid->type == L_G4_ENCODE) {
            if (var_WRITE_G4_IMAGE_MASK) {
                cstr = stringNew("/ImageMask true\n"
                                 "/ColorSpace /DeviceGray");
            }
            else
                cstr = stringNew("/ColorSpace /DeviceGray");
            bstr = stringNew("/BitsPerComponent 1\n"
                             "/Interpolate true");
            snprintf(buff, sizeof(buff),
                     "/Filter /CCITTFaxDecode\n"
                     "/DecodeParms\n"
                     "<<\n"
                     "/K -1\n"
                     "/Columns %d\n"
                     ">>", cid->w);
            fstr = stringNew(buff);
        }
        else if (cid->type == L_JPEG_ENCODE) {
            if (cid->spp == 1)
                cstr = stringNew("/ColorSpace /DeviceGray");
            else if (cid->spp == 3)
                cstr = stringNew("/ColorSpace /DeviceRGB");
            else
                L_ERROR("spp!= 1 && spp != 3", procName);
            bstr = stringNew("/BitsPerComponent 8");
            fstr = stringNew("/Filter /DCTDecode");
        }
        else {  /* type == L_FLATE_ENCODE */
            if (cid->ncolors > 0) {  /* cmapped */
                snprintf(buff, sizeof(buff), "/ColorSpace %d 0 R", cmindex++);
                cstr = stringNew(buff);
            }
            else {
                if (cid->spp == 1 && cid->bps == 1)
                    cstr = stringNew("/ColorSpace /DeviceGray\n"
                                     "/Decode [1 0]");
                else if (cid->spp == 1)  /* 8 bpp */
                    cstr = stringNew("/ColorSpace /DeviceGray");
                else if (cid->spp == 3)
                    cstr = stringNew("/ColorSpace /DeviceRGB");
                else 
                    L_ERROR("unknown colorspace", procName);
            }
            snprintf(buff, sizeof(buff), "/BitsPerComponent %d", cid->bps);
            bstr = stringNew(buff);
            fstr = stringNew("/Filter /FlateDecode");
        }

        snprintf(buf, sizeof(buf), 
                 "%d 0 obj\n"
                 "<<\n"
                 "/Length %ld\n"
                 "/Subtype /Image\n"
                 "%s\n"  /* colorspace */
                 "/Width %d\n"
                 "/Height %d\n"
                 "%s\n"  /* bits/component */
                 "%s\n"  /* filter */
                 ">>\n"
                 "stream\n",
                 6 + i, cid->nbytescomp, cstr, cid->w, cid->h, bstr, fstr);
        xstr = stringNew(buf);
        sarrayAddString(sa, xstr, L_INSERT);
        numaAddNumber(lpd->objsize,
                      strlen(xstr) + cid->nbytescomp + strlen(lpd->poststream));
        FREE(cstr);
        FREE(bstr);
        FREE(fstr);
    }

    return 0;
}


static l_int32
generateColormapStringsPdf(L_PDF_DATA  *lpd)
{
char                buf[L_BIGBUF];
char               *cmstr;
l_int32             i, cmindex, ncmap;
L_COMPRESSED_DATA  *cid;
SARRAY             *sa;
    
    PROCNAME("generateColormapStringsPdf");

        /* In our canonical format, we have 5 objects, followed
         * by n XObjects, followed by m colormaps, so the index of
         * the first colormap object is 6 + n. */
    sa = lpd->sacmap;
    cmindex = 6 + lpd->n;  /* starting value */
    ncmap = 0;
    for (i = 0; i < lpd->n; i++) {
        if ((cid = pdfdataGetCid(lpd, i)) == NULL)
            return ERROR_INT("cid not found", procName, 1);
        if (cid->ncolors == 0) continue;

        ncmap++;
        snprintf(buf, sizeof(buf), "%d 0 obj\n"
                                   "[ /Indexed /DeviceRGB\n"
                                   "%d\n"
                                   "%s\n"
                                   "]\n"
                                   "endobj\n",
                                   cmindex, cid->ncolors - 1, cid->cmapdatahex);
        cmindex++;
        cmstr = stringNew(buf);
        numaAddNumber(lpd->objsize, strlen(cmstr));
        sarrayAddString(sa, cmstr, L_INSERT);
    }

    lpd->ncmap = ncmap;
    return 0;
}


static void
generateTrailerPdf(L_PDF_DATA  *lpd)
{
l_int32  i, n, size, linestart;
NUMA    *naloc, *nasize;

        /* Let nobj be the number of numbered objects.  These numbered
         * objects are indexed by their pdf number in arrays naloc[]
         * and nasize[].  The 0th object is the 9 byte header.  Then
         * the number of objects in nasize, which includes the header,
         * is n = nobj + 1.  The array naloc[] has n + 1 elements,
         * because it includes as the last element the starting
         * location of xref.  The indexing of these objects, their
         * starting locations and sizes are:
         *
         *     Object number         Starting location         Size
         *     -------------         -----------------     --------------
         *          0                   naloc[0] = 0       nasize[0] = 9
         *          1                   naloc[1] = 9       nasize[1] = 49
         *          n                   naloc[n]           nasize[n] 
         *          xref                naloc[n+1] 
         *
         * We first generate naloc.
         */
    nasize = lpd->objsize;
    naloc = lpd->objloc;
    linestart = 0;
    numaAddNumber(naloc, linestart);  /* header */
    n = numaGetCount(nasize);
    for (i = 0; i < n; i++) {
        numaGetIValue(nasize, i, &size);
        linestart += size;
        numaAddNumber(naloc, linestart);
    }
    numaGetIValue(naloc, n, &lpd->xrefloc);  /* save it */

        /* Now make the actual trailer string */
    lpd->trailer = makeTrailerStringPdf(naloc);
}


static char *
makeTrailerStringPdf(NUMA  *naloc)
{
char    *outstr;
char     buf[L_BIGBUF];
l_int32  i, n, linestart, xrefloc;
SARRAY  *sa;

    PROCNAME("makeTrailerStringPdf");

    if (!naloc)
        return (char *)ERROR_PTR("naloc not defined", procName, NULL);
    n = numaGetCount(naloc) - 1;  /* numbered objects + 1 (yes, +1) */

    sa = sarrayCreate(0);
    snprintf(buf, sizeof(buf), "xref\n"
                               "0 %d\n"
                               "0000000000 65535 f \n", n);
    sarrayAddString(sa, (char *)buf, L_COPY);
    for (i = 1; i < n; i++) {
        numaGetIValue(naloc, i, &linestart);
        snprintf(buf, sizeof(buf), "%010d 00000 n \n", linestart);
        sarrayAddString(sa, (char *)buf, L_COPY);
    }

    numaGetIValue(naloc, n, &xrefloc);
    snprintf(buf, sizeof(buf), "trailer\n"
                               "<<\n"
                               "/Size %d\n"
                               "/Root 1 0 R\n"
                               "/Info 2 0 R\n"
                               ">>\n"
                               "startxref\n"
                               "%d\n"
                               "%%%%EOF\n", n, xrefloc);
    sarrayAddString(sa, (char *)buf, L_COPY);
    outstr = sarrayToString(sa, 0);
    sarrayDestroy(&sa);
    return outstr;
}


/*!
 *  generateOutputDataPdf()
 *
 *      Input:  &data (<return> pdf data array)
 *              &nbytes (<return> size of pdf data array)
 *              lpd (input data used to make pdf)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Only called from l_generatePdf().  On error, no data is returned.
 */
static l_int32
generateOutputDataPdf(l_uint8    **pdata,
                      size_t      *pnbytes,
                      L_PDF_DATA  *lpd)
{
char               *str;
l_uint8            *data;
l_int32             nimages, i, len;
l_int32            *sizes, *locs;
size_t              nbytes;
L_COMPRESSED_DATA  *cid;

    PROCNAME("generateOutputDataPdf");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    *pdata = NULL;
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);
    nbytes = lpd->xrefloc + strlen(lpd->trailer);
    *pnbytes = nbytes;
    if ((data = (l_uint8 *)CALLOC(nbytes, sizeof(l_uint8))) == NULL)
        return ERROR_INT("calloc fail for data", procName, 1);
    *pdata = data;

    sizes = numaGetIArray(lpd->objsize);
    locs = numaGetIArray(lpd->objloc);
    memcpy((char *)data, lpd->id, sizes[0]);
    memcpy((char *)(data + locs[1]), lpd->obj1, sizes[1]);
    memcpy((char *)(data + locs[2]), lpd->obj2, sizes[2]);
    memcpy((char *)(data + locs[3]), lpd->obj3, sizes[3]);
    memcpy((char *)(data + locs[4]), lpd->obj4, sizes[4]);
    memcpy((char *)(data + locs[5]), lpd->obj5, sizes[5]);

        /* Each image has 3 parts: variable preamble, the compressed
         * data stream, and the fixed poststream. */
    nimages = lpd->n;
    for (i = 0; i < nimages; i++) {
        if ((cid = pdfdataGetCid(lpd, i)) == NULL)  /* this should not happen */
            return ERROR_INT("cid not found", procName, 1);
        str = sarrayGetString(lpd->saprex, i, L_NOCOPY);
        len = strlen(str);
        memcpy((char *)(data + locs[6 + i]), str, len);
        memcpy((char *)(data + locs[6 + i] + len),
               (char *)cid->datacomp, cid->nbytescomp);
        memcpy((char *)(data + locs[6 + i] + len + cid->nbytescomp),
               lpd->poststream, strlen(lpd->poststream));
    }

        /* Each colormap is simply a stored string */
    for (i = 0; i < lpd->ncmap; i++) {
        str = sarrayGetString(lpd->sacmap, i, L_NOCOPY);
        memcpy((char *)(data + locs[6 + nimages + i]), str, strlen(str));
    }

        /* And finally the trailer */
    memcpy((char *)(data + lpd->xrefloc), lpd->trailer, strlen(lpd->trailer));
    FREE(sizes);
    FREE(locs);
    return 0;
}


/*---------------------------------------------------------------------*
 *                         Multi-page concatenation                    *
 *---------------------------------------------------------------------*/
/*!
 *  concatenatePdf()
 *
 *      Input:  directory name (containing single-page pdf files)
 *              substr (<optional> substring filter on filenames; can be NULL)
 *              fileout (concatenated pdf file)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This only works with leptonica-formatted single-page pdf files.
 *      (2) If @substr is not NULL, only filenames that contain
 *          the substring can be returned.  If @substr == NULL,
 *          none of the filenames are filtered out.
 *      (3) The files in the directory, after optional filtering by
 *          the substring, are lexically sorted in increasing order
 *          before concatenation.
 */
l_int32
concatenatePdf(const char  *dirname,
               const char  *substr,
               const char  *fileout)
{
l_int32  ret;
SARRAY  *sa;

    PROCNAME("concatenatePdf");

    if (!dirname)
        return ERROR_INT("dirname not defined", procName, 1);
    if (!fileout)
        return ERROR_INT("fileout not defined", procName, 1);

    if ((sa = getSortedPathnamesInDirectory(dirname, substr, 0, 0)) == NULL)
        return ERROR_INT("sa not made", procName, 1);
    ret = saConcatenatePdf(sa, fileout);
    sarrayDestroy(&sa);
    return ret;
}


/*!
 *  saConcatenatePdf()
 *
 *      Input:  sarray (of pathnames for single-page pdf files)
 *              fileout (concatenated pdf file)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This only works with leptonica-formatted single-page pdf files.
 */
l_int32
saConcatenatePdf(SARRAY      *sa,
                 const char  *fileout)
{
l_uint8  *data;
l_int32   ret;
size_t    nbytes;

    PROCNAME("saConcatenatePdf");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!fileout)
        return ERROR_INT("fileout not defined", procName, 1);

    ret = saConcatenatePdfToData(sa, &data, &nbytes);
    if (ret)
        return ERROR_INT("pdf data not made", procName, 1);
    ret = l_binaryWrite(fileout, "w", data, nbytes);
    FREE(data);
    return ret;
}


/*!
 *  ptraConcatenatePdf()
 *
 *      Input:  ptra (array of pdf strings, each for a single-page pdf file)
 *              fileout (concatenated pdf file)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This only works with leptonica-formatted single-page pdf files.
 */
l_int32
ptraConcatenatePdf(L_PTRA      *pa,
                   const char  *fileout)
{
l_uint8  *data;
l_int32   ret;
size_t    nbytes;

    PROCNAME("ptraConcatenatePdf");

    if (!pa)
        return ERROR_INT("pa not defined", procName, 1);
    if (!fileout)
        return ERROR_INT("fileout not defined", procName, 1);

    ret = ptraConcatenatePdfToData(pa, NULL, &data, &nbytes);
    if (ret)
        return ERROR_INT("pdf data not made", procName, 1);
    ret = l_binaryWrite(fileout, "w", data, nbytes);
    FREE(data);
    return ret;
}


/*!
 *  concatenatePdfToData()
 *
 *      Input:  directory name (containing single-page pdf files)
 *              substr (<optional> substring filter on filenames; can be NULL)
 *              &data (<return> concatenated pdf data in memory)
 *              &nbytes (<return> number of bytes in pdf data)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This only works with leptonica-formatted single-page pdf files.
 *      (2) If @substr is not NULL, only filenames that contain
 *          the substring can be returned.  If @substr == NULL,
 *          none of the filenames are filtered out.
 *      (3) The files in the directory, after optional filtering by
 *          the substring, are lexically sorted in increasing order
 *          before concatenation.
 */
l_int32
concatenatePdfToData(const char  *dirname,
                     const char  *substr,
                     l_uint8    **pdata,
                     size_t      *pnbytes)
{
l_int32  ret;
SARRAY  *sa;

    PROCNAME("concatenatePdfToData");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    *pdata = NULL;
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);
    *pnbytes = 0;
    if (!dirname)
        return ERROR_INT("dirname not defined", procName, 1);

    if ((sa = getSortedPathnamesInDirectory(dirname, substr, 0, 0)) == NULL)
        return ERROR_INT("sa not made", procName, 1);
    ret = saConcatenatePdfToData(sa, pdata, pnbytes);
    sarrayDestroy(&sa);
    return ret;
}


/*!
 *  saConcatenatePdfToData()
 *
 *      Input:  sarray (of pathnames for single-page pdf files)
 *              &data (<return> concatenated pdf data in memory)
 *              &nbytes (<return> number of bytes in pdf data)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This only works with leptonica-formatted single-page pdf files.
 */
l_int32
saConcatenatePdfToData(SARRAY    *sa,
                       l_uint8  **pdata,
                       size_t    *pnbytes)
{
char     *fname;
l_int32   i, npages, ret;
L_BYTEA  *bas;
L_PTRA   *pa_data;  /* input pdf data for each page */

    PROCNAME("saConcatenatePdfToData");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    *pdata = NULL;
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);
    *pnbytes = 0;
    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);

        /* Read the pdf files into memory */
    if ((npages = sarrayGetCount(sa)) == 0)
        return ERROR_INT("no filenames found", procName, 1);
    pa_data = ptraCreate(npages);
    for (i = 0; i < npages; i++) {
        fname = sarrayGetString(sa, i, L_NOCOPY);
        bas = l_byteaInitFromFile(fname);
        ptraAdd(pa_data, bas);
    }

    ret = ptraConcatenatePdfToData(pa_data, sa, pdata, pnbytes);

        /* Cleanup: some pages could have been removed */
    ptraGetActualCount(pa_data, &npages);
    for (i = 0; i < npages; i++) {
        bas = (L_BYTEA *)ptraRemove(pa_data, i, L_NO_COMPACTION);
        l_byteaDestroy(&bas);
    }
    ptraDestroy(&pa_data, FALSE, FALSE);
    return ret;
}


/*!
 *  ptraConcatenatePdfToData()
 *
 *      Input:  ptra (array of pdf strings, each for a single-page pdf file)
 *              sarray (<optional> of pathnames for input pdf files)
 *              &data (<return> concatenated pdf data in memory)
 *              &nbytes (<return> number of bytes in pdf data)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This only works with leptonica-formatted single-page pdf files.
 *          pdf files generated by other programs will have unpredictable
 *          (and usually bad) results.  The requirements for each pdf file:
 *            (a) The Catalog and Info objects are the first two.
 *            (b) Object 3 is Pages
 *            (c) Object 4 is Page
 *            (d) The remaining objects are Contents, XObjects, and ColorSpace
 *      (2) We remove trailers from each page, and append the full trailer
 *          for all pages at the end.
 *      (3) For all but the first file, remove the ID and the first 3
 *          objects (catalog, info, pages), so that each subsequent
 *          file has only objects of these classes:
 *              Page, Contents, XObject, ColorSpace (Indexed RGB).
 *          For those objects, we substitute these refs to objects
 *          in the local file:
 *              Page:  Parent(object 3), Contents, XObject(typically multiple)
 *              XObject:  [ColorSpace if indexed]
 *          The Pages object on the first page (object 3) has a Kids array
 *          of references to all the Page objects, with a Count equal
 *          to the number of pages.  Each Page object refers back to
 *          this parent.
 */
l_int32
ptraConcatenatePdfToData(L_PTRA    *pa_data,
                         SARRAY    *sa,
                         l_uint8  **pdata,
                         size_t    *pnbytes)
{
char     *fname, *str_pages, *str_trailer;
l_uint8  *pdfdata, *data;
l_int32   i, j, index, nobj, npages;
l_int32  *sizes, *locs;
size_t    size;
L_BYTEA  *bas, *bad, *bat1, *bat2;
NUMA     *na, *na_locs, *na_objs, *napage, *na_sizes, *na_outlocs;
NUMAA    *naa_locs;  /* object locations on each page */
NUMAA    *naa_objs;  /* object mapping numbers to new values */

    PROCNAME("ptraConcatenatePdfToData");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    *pdata = NULL;
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);
    *pnbytes = 0;
    if (!pa_data)
        return ERROR_INT("pa_data not defined", procName, 1);

        /* Parse the files and find the object locations.
         * Remove file data that cannot be parsed. */
    ptraGetActualCount(pa_data, &npages);
    naa_locs = numaaCreate(npages);
    for (i = 0; i < npages; i++) {
        bas = (L_BYTEA *)ptraGetHandle(pa_data, i);
        if (parseTrailerPdf(bas, &na_locs) != 0) {
            bas = (L_BYTEA *)ptraRemove(pa_data, i, L_NO_COMPACTION);
            l_byteaDestroy(&bas);
            if (sa) {
                fname = sarrayGetString(sa, i, L_NOCOPY);
                L_ERROR_STRING("can't parse file %s; skipping",
                               procName, fname);
            }
            else {
                L_ERROR_INT("can't parse file %d; skipping", procName, i);
            }
        }
        else {
            numaaAddNuma(naa_locs, na_locs, L_INSERT);
        }
    }

        /* Recompute npages in case some of the files were not pdf */
    ptraCompactArray(pa_data);
    ptraGetActualCount(pa_data, &npages);
    if (npages == 0) {
        numaaDestroy(&naa_locs);
        return ERROR_INT("no parsable pdf files found", procName, 1);
    }

        /* Find the mapping from initial to final object numbers */
    naa_objs = numaaCreate(npages);  /* stores final object numbers */
    napage = numaCreate(npages);  /* stores "Page" object numbers */
    index = 0;
    for (i = 0; i < npages; i++) {
        na = numaaGetNuma(naa_locs, i, L_CLONE);
        nobj = numaGetCount(na);
        if (i == 0) {
            numaAddNumber(napage, 4);  /* object 4 on first page */
            na_objs = numaMakeSequence(0.0, 1.0, nobj - 1);
            index = nobj - 1;
        }
        else {  /* skip the first 3 objects in each file */
            numaAddNumber(napage, index);  /* Page object is first we add */
            na_objs = numaMakeConstant(0.0, nobj - 1);
            numaReplaceNumber(na_objs, 3, 3);  /* refers to parent of all */
            for (j = 4; j < nobj - 1; j++)
                numaSetValue(na_objs, j, index++);
        }
        numaaAddNuma(naa_objs, na_objs, L_INSERT);
        numaDestroy(&na);
    }

        /* Make the Pages object (#3) */
    str_pages = generatePagesObjStringPdf(napage);

        /* Build the output */
    bad = l_byteaCreate(5000);
    na_outlocs = numaCreate(0);  /* locations of all output objects */
    for (i = 0; i < npages; i++) {
        bas = (L_BYTEA *)ptraGetHandle(pa_data, i);
        pdfdata = l_byteaGetData(bas, &size);
        na_locs = numaaGetNuma(naa_locs, i, L_CLONE);  /* locs on this page */
        na_objs = numaaGetNuma(naa_objs, i, L_CLONE);  /* obj # on this page */
        nobj = numaGetCount(na_locs) - 1;
        na_sizes = numaMakeDelta(na_locs);  /* object sizes on this page */
        sizes = numaGetIArray(na_sizes);
        locs = numaGetIArray(na_locs);
        if (i == 0) {
            l_byteaAppendData(bad, pdfdata, sizes[0]);
            l_byteaAppendData(bad, pdfdata + locs[1], sizes[1]);
            l_byteaAppendData(bad, pdfdata + locs[2], sizes[2]);
            l_byteaAppendString(bad, str_pages);
            for (j = 0; j < 4; j++)
                numaAddNumber(na_outlocs, locs[j]);
        }
        for (j = 4; j < nobj; j++) {
            numaAddNumber(na_outlocs, l_byteaGetSize(bad));
            bat1 = l_byteaInitFromMem(pdfdata + locs[j], sizes[j]);
            bat2 = substituteObjectNumbers(bat1, na_objs);
            data = l_byteaGetData(bat2, &size);
            l_byteaAppendData(bad, data, size);
            l_byteaDestroy(&bat1);
            l_byteaDestroy(&bat2);
        }
        if (i == npages - 1)  /* last one */
            numaAddNumber(na_outlocs, l_byteaGetSize(bad));
        FREE(sizes);
        FREE(locs);
        numaDestroy(&na_locs);
        numaDestroy(&na_objs);
        numaDestroy(&na_sizes);
    }

        /* Add the trailer */
    str_trailer = makeTrailerStringPdf(na_outlocs);
    l_byteaAppendString(bad, str_trailer);

        /* Transfer the output data */
    *pdata = l_byteaCopyData(bad, pnbytes);
    l_byteaDestroy(&bad);

#if  DEBUG_MULTIPAGE
    fprintf(stderr, "******** object mapper **********");
    numaaWriteStream(stderr, naa_objs);

    fprintf(stderr, "******** Page object numbers ***********");
    numaWriteStream(stderr, napage);

    fprintf(stderr, "******** Pages object ***********\n");
    fprintf(stderr, "%s\n", str_pages);
#endif  /* DEBUG_MULTIPAGE */

    numaaDestroy(&naa_locs);
    numaaDestroy(&naa_objs);
    numaDestroy(&napage);
    numaDestroy(&na_outlocs);
    FREE(str_pages);
    FREE(str_trailer);
    return 0;
}


/*---------------------------------------------------------------------*
 *       Helper functions for generating the multi-page pdf output      *
 *---------------------------------------------------------------------*/
/*!
 *  parseTrailerPdf()
 *
 *  Input:  bas (lba of a pdf file)
 *          na (<return> byte locations of the beginning of each object)
 *  Return: 0 if OK, 1 on error
 */
static l_int32
parseTrailerPdf(L_BYTEA  *bas,
                NUMA    **pna)
{
char     *str;
l_uint8   nl = '\n';
l_uint8  *data;
l_int32   i, j, start, startloc, xrefloc, found, loc, nobj, objno, trailer_ok;
size_t    size;
NUMA     *na, *naobj, *naxref;
SARRAY   *sa;

    PROCNAME("parseTrailerPdf");

    if (!pna)
        return ERROR_INT("&na not defined", procName, 1);
    *pna = NULL;
    if (!bas)
        return ERROR_INT("bas not defined", procName, 1);
    data = l_byteaGetData(bas, &size);
    if (strncmp((char *)data, "%PDF-1.", 7) != 0)
        return ERROR_INT("PDF header signature not found", procName, 1);

        /* Search for "startxref" starting 50 bytes from the EOF */
    start = 0;
    if (size > 50)
        start = size - 50;
    arrayFindSequence(data + start, size - start,
                      (l_uint8 *)"startxref\n", 10, &loc, &found);
    if (!found)
        return ERROR_INT("startxref not found!", procName, 1);
    if (sscanf((char *)(data + start + loc + 10), "%d\n", &xrefloc) != 1)
        return ERROR_INT("xrefloc not found!", procName, 1);
    if (xrefloc < 0 || xrefloc >= size)
        return ERROR_INT("invalid xrefloc!", procName, 1);
    sa = sarrayCreateLinesFromString((char *)(data + xrefloc), 0);
    str = sarrayGetString(sa, 1, L_NOCOPY);
    if ((sscanf(str, "0 %d", &nobj)) != 1)
        return ERROR_INT("nobj not found", procName, 1);

        /* Get starting locations.  The numa index is the
         * object number.  loc[0] is the ID; loc[nobj + 1] is xrefloc.  */
    na = numaCreate(nobj + 1);
    *pna = na;
    for (i = 0; i < nobj; i++) {
        str = sarrayGetString(sa, i + 2, L_NOCOPY);
        sscanf(str, "%d", &startloc);
        numaAddNumber(na, startloc);
    }
    numaAddNumber(na, xrefloc);

#if  DEBUG_MULTIPAGE
    fprintf(stderr, "************** Trailer string ************\n");
    fprintf(stderr, "xrefloc = %d", xrefloc);
    sarrayWriteStream(stderr, sa);

    fprintf(stderr, "************** Object locations ************");
    numaWriteStream(stderr, na);
#endif  /* DEBUG_MULTIPAGE */
    sarrayDestroy(&sa);

        /* Verify correct parsing */
    trailer_ok = TRUE;
    for (i = 1; i < nobj; i++) {
        numaGetIValue(na, i, &startloc);
        if ((sscanf((char *)(data + startloc), "%d 0 obj", &objno)) != 1) {
            L_ERROR_INT("bad trailer for object %d", procName, i);
            trailer_ok = FALSE;
            break;
        }
    }

        /* If the trailer is broken, reconstruct the correct obj locations */
    if (!trailer_ok) {
        L_INFO("rebuilding pdf trailer", procName);
        numaEmpty(na);
        numaAddNumber(na, 0);
        l_byteaFindEachSequence(bas, (l_uint8 *)" 0 obj\n", 7, &naobj);
        nobj = numaGetCount(naobj);
        for (i = 0; i < nobj; i++) {
            numaGetIValue(naobj, i, &loc);
            for (j = loc - 1; j > 0; j--) {
                if (data[j] == nl)
                    break;
            }
            numaAddNumber(na, j + 1);
        }
        l_byteaFindEachSequence(bas, (l_uint8 *)"xref", 4, &naxref);
        numaGetIValue(naxref, 0, &loc);
        numaAddNumber(na, loc);
        numaDestroy(&naobj);
        numaDestroy(&naxref);
    }

    return 0;
}


static char *
generatePagesObjStringPdf(NUMA  *napage)
{
char    *str, *outstr;
char    *buf;
l_int32  i, n, index, bufsize;
SARRAY  *sa;

    PROCNAME("generatePagesObjStringPdf");

    if (!napage)
        return (char *)ERROR_PTR("napage not defined", procName, NULL);

    n = numaGetCount(napage);
    bufsize = 100 + 16 * n;
    buf = (char *)CALLOC(bufsize, sizeof(char));
    sa = sarrayCreate(n);
    for (i = 0; i < n; i++) {
        numaGetIValue(napage, i, &index);
        snprintf(buf, bufsize, " %d 0 R ", index);
        sarrayAddString(sa, buf, L_COPY);
    }

    str = sarrayToString(sa, 0);
    snprintf(buf, bufsize, "3 0 obj\n"
                           "<<\n"
                           "/Type /Pages\n"
                           "/Kids [%s]\n"
                           "/Count %d\n"
                           ">>\n", str, n);
    outstr = stringNew(buf);
    sarrayDestroy(&sa);
    FREE(str);
    FREE(buf);
    return outstr;
}


/*!
 *  substituteObjectNumbers()
 *
 *  Input:  bas (lba of a pdf object)
 *          na_objs (object number mapping array)
 *  Return: bad (lba of rewritten pdf for the object)
 *
 *  Notes:
 *      (1) Interpret the first set of bytes as the object number,
 *          map to the new number, and write it out.
 *      (2) Find all occurrences of this 4-byte sequence: " 0 R"
 *      (3) Find the location and value of the integer preceeding this,
 *          and map it to the new value.
 *      (4) Rewrite the object with new object numbers.
 */
static L_BYTEA *
substituteObjectNumbers(L_BYTEA  *bas,
                        NUMA     *na_objs)
{
l_uint8   space = ' ';
l_uint8  *datas;
l_uint8   buf[32];  /* only needs to hold one integer in ascii format */
l_int32   start, nrepl, i, j, objin, objout;
l_int32  *objs, *matches;
size_t    size;
L_BYTEA  *bad;
NUMA     *na_match;

    datas = l_byteaGetData(bas, &size);
    bad = l_byteaCreate(100);
    objs = numaGetIArray(na_objs);  /* object number mapper */

        /* Substitute the object number on the first line */
    sscanf((char *)datas, "%d", &objin);
    objout = objs[objin];
    snprintf((char *)buf, 32, "%d", objout);
    l_byteaAppendString(bad, (char *)buf);

        /* Find the set of matching locations for object references */
    arrayFindSequence(datas, size, &space, 1, &start, NULL);
    na_match = arrayFindEachSequence(datas, size, (l_uint8 *)" 0 R", 4);
    if (!na_match) {
        l_byteaAppendData(bad, datas + start, size - start);
        FREE(objs);
        return bad;
    }

        /* Substitute all the object reference numbers */
    nrepl = numaGetCount(na_match);
    matches = numaGetIArray(na_match);
    for (i = 0; i < nrepl; i++) {
            /* Find the first space before the object number */
        for (j = matches[i] - 1; j > 0; j--) {
            if (datas[j] == space)
                break;
        }
            /* Copy bytes from 'start' up to the object number */
        l_byteaAppendData(bad, datas + start, j - start + 1);
        sscanf((char *)(datas + j + 1), "%d", &objin);
        objout = objs[objin];
        snprintf((char *)buf, 32, "%d", objout);
        l_byteaAppendString(bad, (char *)buf);
        start = matches[i];
    }
    l_byteaAppendData(bad, datas + start, size - start);

    FREE(objs);
    FREE(matches);
    numaDestroy(&na_match);
    return bad;
}


/*---------------------------------------------------------------------*
 *                     Create/destroy/access pdf data                  *
 *---------------------------------------------------------------------*/
static L_PDF_DATA *
pdfdataCreate(const char  *title)
{
L_PDF_DATA *lpd;

    lpd = (L_PDF_DATA *)CALLOC(1, sizeof(L_PDF_DATA));
    if (title) lpd->title = stringNew(title);
    lpd->cida = ptraCreate(10);
    lpd->xy = ptaCreate(10);
    lpd->wh = ptaCreate(10);
    lpd->saprex = sarrayCreate(10);
    lpd->sacmap = sarrayCreate(10);
    lpd->objsize = numaCreate(20);
    lpd->objloc = numaCreate(20);
    return lpd;
}

static void
pdfdataDestroy(L_PDF_DATA  **plpd)
{
l_int32             i;
L_COMPRESSED_DATA  *cid;
L_PDF_DATA         *lpd;

    PROCNAME("pdfdataDestroy");

    if (plpd== NULL) {
        L_WARNING("ptr address is null!", procName);
        return;
    }
    if ((lpd = *plpd) == NULL)
        return;

    if (lpd->title) FREE(lpd->title);
    for (i = 0; i < lpd->n; i++) {
        cid = (L_COMPRESSED_DATA *)ptraRemove(lpd->cida, i, L_NO_COMPACTION);
        compressed_dataDestroy(&cid);
    }

    ptraDestroy(&lpd->cida, 0, 0);
    if (lpd->id) FREE(lpd->id);
    if (lpd->obj1) FREE(lpd->obj1);
    if (lpd->obj2) FREE(lpd->obj2);
    if (lpd->obj3) FREE(lpd->obj3);
    if (lpd->obj4) FREE(lpd->obj4);
    if (lpd->obj5) FREE(lpd->obj5);
    if (lpd->poststream) FREE(lpd->poststream);
    if (lpd->trailer) FREE(lpd->trailer);
    if (lpd->xy) ptaDestroy(&lpd->xy);
    if (lpd->wh) ptaDestroy(&lpd->wh);
    if (lpd->mediabox) boxDestroy(&lpd->mediabox);
    if (lpd->saprex) sarrayDestroy(&lpd->saprex);
    if (lpd->sacmap) sarrayDestroy(&lpd->sacmap);
    if (lpd->objsize) numaDestroy(&lpd->objsize);
    if (lpd->objloc) numaDestroy(&lpd->objloc);
    FREE(lpd);
    *plpd = NULL;
    return;
}


static L_COMPRESSED_DATA *
pdfdataGetCid(L_PDF_DATA  *lpd,
              l_int32      index)
{
    PROCNAME("pdfdataGetCid");

    if (!lpd)
        return (L_COMPRESSED_DATA *)ERROR_PTR("lpd not defined",
                                              procName, NULL);
    if (index < 0 || index >= lpd->n)
        return (L_COMPRESSED_DATA *)ERROR_PTR("invalid image index",
                                              procName, NULL);

    return (L_COMPRESSED_DATA *)ptraGetHandle(lpd->cida, index);
}


/*---------------------------------------------------------------------*
 *                       Set flags for special modes                   *
 *---------------------------------------------------------------------*/
/*!
 *  l_pdfSetG4ImageMask()
 *
 *      Input:  flag (1 for writing g4 data as fg only through a mask;
 *                    0 for writing fg and bg)
 *      Return: void
 *
 *  Notes:
 *      (1) The default is for writing only the fg (through the mask).
 *          That way when you write a 1 bpp image, the bg is transparent,
 *          so any previously written image remains visible behind it.
 */
void
l_pdfSetG4ImageMask(l_int32  flag)
{
    var_WRITE_G4_IMAGE_MASK = flag;
}


/*!
 *  l_pdfSetDateAndVersion()
 *
 *      Input:  flag (1 for writing date/time and leptonica version;
 *                    0 for omitting this from the metadata)
 *      Return: void
 *
 *  Notes:
 *      (1) The default is for writing this data.  For regression tests
 *          that compare output against golden files, it is useful to omit.
 */
void
l_pdfSetDateAndVersion(l_int32  flag)
{
    var_WRITE_DATE_AND_VERSION = flag;
}


/* --------------------------------------------*/
#endif  /* USE_PDFIO */
/* --------------------------------------------*/

