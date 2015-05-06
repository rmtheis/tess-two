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
 *  webpiostub.c
 *
 *     Stubs for webpio.c functions
 */

#include "allheaders.h"

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

/* --------------------------------------------*/
#if  !HAVE_LIBWEBP   /* defined in environ.h */
/* --------------------------------------------*/

PIX * pixReadStreamWebP(FILE *fp)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadStreamWebP", NULL);
}

/* ----------------------------------------------------------------------*/

PIX * pixReadMemWebP(const l_uint8 *filedata, size_t filesize)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadMemWebP", NULL);
}

/* ----------------------------------------------------------------------*/

l_int32 readHeaderWebP(const char *filename, l_int32 *pw, l_int32 *ph,
                       l_int32 *pspp)
{
    return ERROR_INT("function not present", "readHeaderWebP", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 readHeaderMemWebP(const l_uint8 *data, size_t size,
                          l_int32 *pw, l_int32 *ph, l_int32 *pspp)
{
    return ERROR_INT("function not present", "readHeaderMemWebP", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWriteWebP(const char *filename, PIX *pixs, l_int32 quality,
                     l_int32 lossless)
{
    return ERROR_INT("function not present", "pixWriteWebP", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWriteStreamWebP(FILE *fp, PIX *pixs, l_int32 quality,
                           l_int32 lossless)
{
    return ERROR_INT("function not present", "pixWriteStreamWebP", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWriteMemWebP(l_uint8 **pencdata, size_t *pencsize, PIX *pixs,
                        l_int32 quality, l_int32 lossless)
{
    return ERROR_INT("function not present", "pixWriteMemWebP", 1);
}

/* --------------------------------------------*/
#endif  /* !HAVE_LIBWEBP */
/* --------------------------------------------*/
