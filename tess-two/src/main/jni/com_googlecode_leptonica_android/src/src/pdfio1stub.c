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
 *  pdfio1stub.c
 *
 *     Stubs for pdfio1.c functions
 */

#include "allheaders.h"

/* --------------------------------------------*/
#if  !USE_PDFIO   /* defined in environ.h */
/* --------------------------------------------*/

/* ----------------------------------------------------------------------*/

l_int32 convertFilesToPdf(const char *dirname, const char *substr,
                          l_int32 res, l_float32 scalefactor,
                          l_int32 type, l_int32 quality,
                          const char *title, const char *fileout)
{
    return ERROR_INT("function not present", "convertFilesToPdf", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 saConvertFilesToPdf(SARRAY *sa, l_int32 res, l_float32 scalefactor,
                            l_int32 type, l_int32 quality,
                            const char *title, const char *fileout)
{
    return ERROR_INT("function not present", "saConvertFilesToPdf", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 saConvertFilesToPdfData(SARRAY *sa, l_int32 res,
                                l_float32 scalefactor, l_int32 type,
                                l_int32 quality, const char *title,
                                l_uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "saConvertFilesToPdfData", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 selectDefaultPdfEncoding(PIX *pix, l_int32 *ptype)
{
    return ERROR_INT("function not present", "selectDefaultPdfEncoding", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 convertUnscaledFilesToPdf(const char *dirname, const char *substr,
                                  const char *title, const char *fileout)
{
    return ERROR_INT("function not present", "convertUnscaledFilesToPdf", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 saConvertUnscaledFilesToPdf(SARRAY *sa, const char *title,
                                    const char *fileout)
{
    return ERROR_INT("function not present", "saConvertUnscaledFilesToPdf", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 saConvertUnscaledFilesToPdfData(SARRAY *sa, const char *title,
                                        l_uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present",
                     "saConvertUnscaledFilesToPdfData", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 convertUnscaledToPdfData(const char *fname, const char *title,
                                 l_uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "convertUnscaledToPdfData", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixaConvertToPdf(PIXA *pixa, l_int32 res, l_float32 scalefactor,
                         l_int32 type, l_int32 quality,
                         const char *title, const char *fileout)
{
    return ERROR_INT("function not present", "pixaConvertToPdf", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixaConvertToPdfData(PIXA *pixa, l_int32 res, l_float32 scalefactor,
                             l_int32 type, l_int32 quality, const char *title,
                             l_uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "pixaConvertToPdfData", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 convertToPdf(const char *filein,
                     l_int32 type, l_int32 quality,
                     const char *fileout,
                     l_int32 x, l_int32 y, l_int32 res,
                     const char *title,
                     L_PDF_DATA **plpd, l_int32 position)
{
    return ERROR_INT("function not present", "convertToPdf", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 convertImageDataToPdf(l_uint8 *imdata, size_t size,
                              l_int32 type, l_int32 quality,
                              const char *fileout,
                              l_int32 x, l_int32 y, l_int32 res,
                              const char *title,
                              L_PDF_DATA **plpd, l_int32 position)
{
    return ERROR_INT("function not present", "convertImageDataToPdf", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 convertToPdfData(const char *filein,
                         l_int32 type, l_int32 quality,
                         l_uint8 **pdata, size_t *pnbytes,
                         l_int32 x, l_int32 y, l_int32 res,
                         const char *title,
                         L_PDF_DATA **plpd, l_int32 position)
{
    return ERROR_INT("function not present", "convertToPdfData", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 convertImageDataToPdfData(l_uint8 *imdata, size_t size,
                                  l_int32 type, l_int32 quality,
                                  l_uint8 **pdata, size_t *pnbytes,
                                  l_int32 x, l_int32 y, l_int32 res,
                                  const char *title,
                                  L_PDF_DATA **plpd, l_int32 position)
{
    return ERROR_INT("function not present", "convertImageDataToPdfData", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixConvertToPdf(PIX *pix, l_int32 type, l_int32 quality,
                        const char *fileout,
                        l_int32 x, l_int32 y, l_int32 res,
                        const char *title,
                        L_PDF_DATA **plpd, l_int32 position)
{
    return ERROR_INT("function not present", "pixConvertToPdf", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWriteStreamPdf(FILE *fp, PIX *pix, l_int32 res, const char *title)
{
    return ERROR_INT("function not present", "pixWriteStreamPdf", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWriteMemPdf(l_uint8 **pdata, size_t *pnbytes, PIX *pix,
                       l_int32 res, const char *title)
{
    return ERROR_INT("function not present", "pixWriteMemPdf", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 convertSegmentedFilesToPdf(const char *dirname, const char *substr,
                                   l_int32 res, l_int32 type, l_int32 thresh,
                                   BOXAA *baa, l_int32 quality,
                                   l_float32 scalefactor, const char *title,
                                   const char *fileout)
{
    return ERROR_INT("function not present", "convertSegmentedFilesToPdf", 1);
}

/* ----------------------------------------------------------------------*/

BOXAA * convertNumberedMasksToBoxaa(const char *dirname, const char *substr,
                                    l_int32 numpre, l_int32 numpost)
{
    return (BOXAA *)ERROR_PTR("function not present",
                              "convertNumberedMasksToBoxaa", NULL);
}

/* ----------------------------------------------------------------------*/

l_int32 convertToPdfSegmented(const char *filein, l_int32 res, l_int32 type,
                              l_int32 thresh, BOXA *boxa, l_int32 quality,
                              l_float32 scalefactor, const char *title,
                              const char *fileout)
{
    return ERROR_INT("function not present", "convertToPdfSegmented", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixConvertToPdfSegmented(PIX *pixs, l_int32 res, l_int32 type,
                                 l_int32 thresh, BOXA *boxa, l_int32 quality,
                                 l_float32 scalefactor, const char *title,
                                 const char *fileout)
{
    return ERROR_INT("function not present", "pixConvertToPdfSegmented", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 convertToPdfDataSegmented(const char *filein, l_int32 res,
                                  l_int32 type, l_int32 thresh, BOXA *boxa,
                                  l_int32 quality, l_float32 scalefactor,
                                  const char *title,
                                  l_uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "convertToPdfDataSegmented", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixConvertToPdfDataSegmented(PIX *pixs, l_int32 res, l_int32 type,
                                     l_int32 thresh, BOXA *boxa,
                                     l_int32 quality, l_float32 scalefactor,
                                     const char *title,
                                     l_uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "pixConvertToPdfDataSegmented", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 concatenatePdf(const char *dirname, const char *substr,
                       const char *fileout)
{
    return ERROR_INT("function not present", "concatenatePdf", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 saConcatenatePdf(SARRAY *sa, const char *fileout)
{
    return ERROR_INT("function not present", "saConcatenatePdf", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 ptraConcatenatePdf(L_PTRA *pa, const char *fileout)
{
    return ERROR_INT("function not present", "ptraConcatenatePdf", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 concatenatePdfToData(const char *dirname, const char *substr,
                             l_uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "concatenatePdfToData", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 saConcatenatePdfToData(SARRAY *sa, l_uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "saConcatenatePdfToData", 1);
}

/* ----------------------------------------------------------------------*/

/* --------------------------------------------*/
#endif  /* !USE_PDFIO */
/* --------------------------------------------*/
