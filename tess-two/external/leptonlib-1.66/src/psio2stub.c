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
 *  psio2stub.c
 *
 *     Stubs for psio2.c functions
 */

#include <stdio.h>
#include "allheaders.h"

/* --------------------------------------------*/
#if  !USE_PSIO   /* defined in environ.h */
/* --------------------------------------------*/

l_int32 pixWritePSEmbed(const char *filein, const char *fileout)
{
    return ERROR_INT("function not present", "pixWritePSEmbed", 1);
}

l_int32 pixWriteStreamPS(FILE *fp, PIX *pix, BOX *box, l_int32 res,
                         l_float32 scale)
{
    return ERROR_INT("function not present", "pixWriteStreamPS", 1);
}

char * pixWriteStringPS(PIX *pixs, BOX *box, l_int32 res, l_float32 scale)
{
    return (char *)ERROR_PTR("function not present", "pixWriteStringPS", NULL);
}

void getScaledParametersPS(BOX *box, l_int32 wpix, l_int32 hpix, l_int32 res,
                           l_float32 scale, l_float32 *pxpt, l_float32 *pypt,
                           l_float32 *pwpt, l_float32 *phpt)
{
    L_ERROR("function not present", "getScaledParametersPS");
    return;
}

void convertByteToHexAscii(l_uint8 byteval, char *pnib1, char *pnib2)
{
    L_ERROR("function not present", "convertByteToHexAscii");
    return;
}

l_int32 convertJpegToPSEmbed(const char *filein, const char *fileout)
{
    return ERROR_INT("function not present", "convertJpegToPSEmbed", 1);
}

l_int32 convertJpegToPS(const char *filein, const char *fileout,
                        const char *operation, l_int32 x, l_int32 y,
                        l_int32 res, l_float32 scale, l_int32 pageno,
                        l_int32 endpage)
{
    return ERROR_INT("function not present", "convertJpegToPS", 1);
}

l_int32 convertJpegToPSString(const char *filein, char **poutstr,
                              l_int32 *pnbytes, l_int32 x, l_int32 y,
                              l_int32 res, l_float32 scale, l_int32 pageno,
                              l_int32 endpage)
{
    return ERROR_INT("function not present", "convertJpegToPSString", 1);
}

char * generateJpegPS(const char *filein, char *data85, l_int32 w,
                      l_int32 h, l_int32 bps, l_int32 spp, l_float32 xpt,
                      l_float32 ypt, l_float32 wpt, l_float32 hpt,
                      l_int32 pageno, l_int32 endpage)
{
    return (char *)ERROR_PTR("function not present", "generateJpegPS", NULL);
}

l_int32 convertTiffG4ToPSEmbed(const char *filein, const char *fileout)
{
    return ERROR_INT("function not present", "convertTiffG4ToPSEmbed", 1);
}

l_int32 convertTiffG4ToPS(const char *filein, const char *fileout,
                          const char *operation, l_int32 x, l_int32 y,
                          l_int32 res, l_float32 scale, l_int32 pageno,
                          l_int32 mask, l_int32 endpage)
{
    return ERROR_INT("function not present", "convertTiffG4ToPS", 1);
}

l_int32 convertTiffG4ToPSString(const char *filein, char **poutstr,
                                l_int32 *pnbytes, l_int32 x, l_int32 y,
                                l_int32 res, l_float32 scale, l_int32 pageno,
                                l_int32 mask, l_int32 endpage)
{
    return ERROR_INT("function not present", "convertTiffG4ToPSString", 1);
}

char * generateTiffG4PS(const char *filein, char *data85, l_int32 w,
                        l_int32 h, l_float32 xpt, l_float32 ypt,
                        l_float32 wpt, l_float32 hpt,
                        l_int32 minisblack, l_int32 maskflag,
                        l_int32 pageno, l_int32 endpage)
{
    return (char *)ERROR_PTR("function not present", "generateTiffG4PS", NULL);
}


l_int32 convertFlateToPSEmbed(const char *filein, const char *fileout)
{
    return ERROR_INT("function not present", "convertFlateToPSEmbed", 1);
}

l_int32 convertFlateToPS(const char *filein, const char *fileout,
                         const char *operation, l_int32 x, l_int32 y,
                         l_int32 res, l_float32 scale, l_int32 pageno,
                         l_int32 endpage)
{
    return ERROR_INT("function not present", "convertFlateToPS", 1);
}

l_int32 convertFlateToPSString(const char *filein, char **poutstr,
                               l_int32 *pnbytes, l_int32 x, l_int32 y,
                               l_int32 res, l_float32 scale,
                               l_int32 pageno, l_int32 endpage)
{
    return ERROR_INT("function not present", "convertFlateToPSString", 1);
}

char * generateFlatePS(const char *filein, char *data85, char *cmapdata85,
                       l_int32 ncolors, l_int32 w, l_int32 h, l_int32 bps,
                       l_int32 spp, l_float32 xpt, l_float32 ypt,
                       l_float32 wpt, l_float32 hpt,
                       l_int32 pageno, l_int32 endpage)
{
    return (char *)ERROR_PTR("function not present", "generateFlatePS", NULL);
}

l_int32 convertTiffMultipageToPS(const char *filein, const char *fileout,
                                 const char *tempfile, l_float32 fillfract)
{
    return ERROR_INT("function not present", "convertTiffMultipageToPS", 1);
}

l_int32 pixWriteMemPS(l_uint8 **pdata, size_t *psize, PIX *pix, BOX *box,
                      l_int32 res, l_float32 scale)
{
    return ERROR_INT("function not present", "pixWriteMemPS", 1);
}

l_int32 getResLetterPage(l_int32 w, l_int32 h, l_float32 fillfract)
{
    return ERROR_INT("function not present", "getResLetterPage", 1);
}

l_int32 getResA4Page(l_int32 w, l_int32 h, l_float32 fillfract)
{
    return ERROR_INT("function not present", "getResA4Page", 1);
}

char * encodeAscii85(l_uint8 *inarray, l_int32 insize, l_int32 *poutsize)
{
    return (char *)ERROR_PTR("function not present", "encodeAscii85", NULL);
}

l_int32 convertChunkToAscii85(l_uint8 *inarray, l_int32 insize, l_int32 *pindex, char *outbuf, l_int32 *pnbout)
{
    return ERROR_INT("function not present", "convertChunkToAscii85", 1);
}

l_uint8 * decodeAscii85(char *ina, l_int32 insize, l_int32 *poutsize)
{
    return (l_uint8 *)ERROR_PTR("function not present", "decodeAscii85", NULL);
}

void l_psWriteBoundingBox(l_int32 flag)
{
    L_ERROR("function not present", "l_psWriteBoundingBox");
    return;
}

/* --------------------------------------------*/
#endif  /* !USE_PSIO */
/* --------------------------------------------*/

