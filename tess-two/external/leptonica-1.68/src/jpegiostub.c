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
 *  jpegiostub.c
 *
 *     Stubs for jpegio.c functions
 */

#include "allheaders.h"

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

/* --------------------------------------------*/
#if  !HAVE_LIBJPEG   /* defined in environ.h */
/* --------------------------------------------*/

PIX * pixReadJpeg(const char *filename, l_int32 cmflag, l_int32 reduction,
                  l_int32 *pnwarn)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadJpeg", NULL);
}

PIX * pixReadStreamJpeg(FILE *fp, l_int32 cmflag, l_int32 reduction,
                        l_int32 *pnwarn, l_int32 hint)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadStreamJpeg", NULL);
}

l_int32 readHeaderJpeg(const char *filename, l_int32 *pw, l_int32 *ph,
                       l_int32 *pspp, l_int32 *pycck, l_int32 *pcmyk)
{
    return ERROR_INT("function not present", "readHeaderJpeg", 1);
}

l_int32 freadHeaderJpeg(FILE *fp, l_int32 *pw, l_int32 *ph,
                       l_int32 *pspp, l_int32 *pycck, l_int32 *pcmyk)
{
    return ERROR_INT("function not present", "freadHeaderJpeg", 1);
}

l_int32 fgetJpegResolution(FILE *fp, l_int32 *pxres, l_int32 *pyres)
{
    return ERROR_INT("function not present", "fgetJpegResolution", 1);
}

l_int32 pixWriteJpeg(const char *filename, PIX *pix, l_int32 quality,
                     l_int32 progressive)
{
    return ERROR_INT("function not present", "pixWriteJpeg", 1);
}

l_int32 pixWriteStreamJpeg(FILE *fp, PIX *pix, l_int32 quality,
                           l_int32 progressive)
{
    return ERROR_INT("function not present", "pixWriteStreamJpeg", 1);
}

PIX * pixReadMemJpeg(const l_uint8 *cdata, size_t size, l_int32 cmflag,
                     l_int32 reduction, l_int32 *pnwarn, l_int32 hint)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadMemJpeg", NULL);
}

l_int32 readHeaderMemJpeg(const l_uint8 *cdata, size_t size,
                          l_int32 *pw, l_int32 *ph, l_int32 *pspp,
                          l_int32 *pycck, l_int32 *pcmyk)
{
    return ERROR_INT("function not present", "readHeaderMemJpeg", 1);
}

l_int32 pixWriteMemJpeg(l_uint8 **pdata, size_t *psize, PIX *pix,
                        l_int32 quality, l_int32 progressive)
{
    return ERROR_INT("function not present", "pixWriteMemJpeg", 1);
}

void l_jpegSetNoChromaSampling(l_int32 flag)
{
    L_ERROR("function not present", "l_jpegSetNoChromaSampling");
    return;
}

l_int32 extractJpegDataFromFile(const char *filein, l_uint8 **pdata,
                                size_t *pnbytes, l_int32 *pw, l_int32 *ph,
                                l_int32 *pbps, l_int32 *pspp)
{
    return ERROR_INT("function not present", "extractJpegDataFromFile", 1);
}

l_int32 extractJpegDataFromArray(const void *data, size_t nbytes,
                                 l_int32 *pw, l_int32 *ph, l_int32 *pbps,
                                 l_int32 *pspp)
{
    return ERROR_INT("function not present", "extractJpegDataFromArray", 1);
}

/* --------------------------------------------*/
#endif  /* !HAVE_LIBJPEG */
/* --------------------------------------------*/

