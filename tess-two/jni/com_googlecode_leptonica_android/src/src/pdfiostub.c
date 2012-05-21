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
 *  pdfiostub.c
 *
 *     Stubs for pdfio.c functions
 */

#include "allheaders.h"

/* --------------------------------------------*/
#if  !USE_PDFIO   /* defined in environ.h */
/* --------------------------------------------*/

l_int32 convertFilesToPdf(const char *dirname, const char *substr,
                          l_int32 res, l_float32 scalefactor, l_int32 quality,
                          const char *title, const char *fileout)
{
    return ERROR_INT("function not present", "convertFilesToPdf", 1);
}

l_int32 saConvertFilesToPdf(SARRAY *sa, l_int32 res,
                            l_float32 scalefactor, l_int32 quality,
                           const char *title, const char *fileout)
{
    return ERROR_INT("function not present", "saConvertFilesToPdf", 1);
}

l_int32 saConvertFilesToPdfData(SARRAY *sa, l_int32 res,
                                l_float32 scalefactor, l_int32 quality,
                                const char *title, l_uint8 **pdata,
                                size_t *pnbytes)
{
    return ERROR_INT("function not present", "saConvertFilesToPdfData", 1);
}

l_int32 selectDefaultPdfEncoding(PIX *pix, l_int32 *ptype)
{
    return ERROR_INT("function not present", "selectDefaultPdfEncoding", 1);
}

l_int32 convertToPdf(const char *filein,
                     l_int32 type, l_int32 quality,
                     const char *fileout,
                     l_int32 x, l_int32 y, l_int32 res,
                     L_PDF_DATA **plpd, l_int32 position,
                     const char *title)
{
    return ERROR_INT("function not present", "convertToPdf", 1);
}

l_int32 convertImageDataToPdf(l_uint8 *imdata, size_t size,
                              l_int32 type, l_int32 quality,
                              const char *fileout,
                              l_int32 x, l_int32 y, l_int32 res,
                              L_PDF_DATA **plpd, l_int32 position,
                              const char *title)
{
    return ERROR_INT("function not present", "convertImageDataToPdf", 1);
}

l_int32 convertToPdfData(const char *filein,
                         l_int32 type, l_int32 quality,
                         l_uint8 **pdata, size_t *pnbytes,
                         l_int32 x, l_int32 y, l_int32 res,
                         L_PDF_DATA **plpd, l_int32 position,
                         const char *title)
{
    return ERROR_INT("function not present", "convertToPdfData", 1);
}

l_int32 convertImageDataToPdfData(l_uint8 *imdata, size_t size,
                                  l_int32 type, l_int32 quality,
                                  l_uint8 **pdata, size_t *pnbytes,
                                  l_int32 x, l_int32 y, l_int32 res,
                                  L_PDF_DATA **plpd, l_int32 position,
                                  const char *title)
{
    return ERROR_INT("function not present", "convertImageDataToPdfData", 1);
}

l_int32 pixConvertToPdf(PIX *pix, l_int32 type, l_int32 quality,
                        const char *fileout,
                        l_int32 x, l_int32 y, l_int32 res,
                        L_PDF_DATA **plpd, l_int32 position,
                        const char *title)
{
    return ERROR_INT("function not present", "pixConvertToPdf", 1);
}

l_int32 pixConvertToPdfData(PIX *pix, l_int32 type, l_int32 quality,
                            l_uint8 **pdata, size_t *pnbytes,
                            l_int32 x, l_int32 y, l_int32 res,
                            L_PDF_DATA **plpd, l_int32 position,
                            const char *title)
{
    return ERROR_INT("function not present", "pixConvertToPdfData", 1);
}

l_int32 pixWriteStreamPdf(FILE *fp, PIX *pix, l_int32 res, const char *title)
{
    return ERROR_INT("function not present", "pixWriteStreamPdf", 1);
}

l_int32 convertSegmentedFilesToPdf(const char *dirname, const char *substr,
                                   l_int32 res, l_int32 type, l_int32 thresh,
                                   BOXAA *baa, l_int32 quality,
                                   l_float32 scalefactor, const char *title,
                                   const char *fileout)
{
    return ERROR_INT("function not present", "convertSegmentedFilesToPdf", 1);
}

l_int32 convertToPdfSegmented(const char *filein, l_int32 res, l_int32 type,
                              l_int32 thresh, BOXA *boxa, l_int32 quality,
                              l_float32 scalefactor, const char *fileout)
{
    return ERROR_INT("function not present", "convertToPdfSegmented", 1);
}

l_int32 pixConvertToPdfSegmented(PIX *pixs, l_int32 res, l_int32 type,
                                 l_int32 thresh, BOXA *boxa, l_int32 quality,
                                 l_float32 scalefactor, const char *fileout,
                                 const char *title)
{
    return ERROR_INT("function not present", "pixConvertToPdfSegmented", 1);
}

l_int32 convertToPdfDataSegmented(const char *filein, l_int32 res,
                                  l_int32 type, l_int32 thresh, BOXA *boxa,
                                  l_int32 quality, l_float32 scalefactor,
                                  l_uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "convertToPdfDataSegmented", 1);
}

l_int32 pixConvertToPdfDataSegmented(PIX *pixs, l_int32 res, l_int32 type,
                                     l_int32 thresh, BOXA *boxa,
                                     l_int32 quality, l_float32 scalefactor,
                                     l_uint8 **pdata, size_t *pnbytes,
                                     const char *title)
{
    return ERROR_INT("function not present", "pixConvertToPdfDataSegmented", 1);
}

l_int32 concatenatePdf(const char *dirname, const char *substr,
                       const char *fileout)
{
    return ERROR_INT("function not present", "concatenatePdf", 1);
}

l_int32 saConcatenatePdf(SARRAY *sa, const char *fileout)
{
    return ERROR_INT("function not present", "saConcatenatePdf", 1);
}

l_int32 ptraConcatenatePdf(L_PTRA *pa, const char *fileout)
{
    return ERROR_INT("function not present", "ptraConcatenatePdf", 1);
}

l_int32 concatenatePdfToData(const char *dirname, const char *substr,
                             l_uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "concatenatePdfToData", 1);
}

l_int32 saConcatenatePdfToData(SARRAY *sa, l_uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "saConcatenatePdfToData", 1);
}

l_int32 ptraConcatenatePdfToData(L_PTRA *pa_data, SARRAY *sa,
                                 l_uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "ptraConcatenatePdfToData", 1);
}

void l_pdfSetG4ImageMask(l_int32 flag)
{
    L_ERROR("function not present", "l_pdfSetG4ImageMask");
    return;
}

void l_pdfSetDateAndVersion(l_int32 flag)
{
    L_ERROR("function not present", "l_pdfSetDateAndVersion");
    return;
}


/* --------------------------------------------*/
#endif  /* !USE_PDFIO */
/* --------------------------------------------*/

