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
 *  jp2kiostub.c
 *
 *     Stubs for jp2kio.c functions
 */

#include "allheaders.h"

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

/* --------------------------------------------*/
#if  !HAVE_LIBJP2K   /* defined in environ.h */
/* --------------------------------------------*/

/* ----------------------------------------------------------------------*/

PIX * pixReadJp2k(const char *filename, l_uint32 reduction, BOX *box,
                  l_int32 hint)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadJp2k", NULL);
}

/* ----------------------------------------------------------------------*/

PIX * pixReadStreamJp2k(FILE *fp, l_uint32 reduction, BOX *box, l_int32 hint)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadStreamJp2k", NULL);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWriteJp2k(const char *filename, PIX *pix, l_int32 quality,
                     l_int32 nlevels, l_int32 hint)
{
    return ERROR_INT("function not present", "pixWriteJp2k", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWriteStreamJp2k(FILE *fp, PIX *pix, l_int32 quality,
                           l_int32 nlevels, l_int32 hint)
{
    return ERROR_INT("function not present", "pixWriteStreamJp2k", 1);
}

/* ----------------------------------------------------------------------*/

PIX * pixReadMemJp2k(const l_uint8 *data, size_t size, l_uint32 reduction,
                     BOX *box, l_int32 hint)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadMemJp2k", NULL);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWriteMemJp2k(l_uint8 **pdata, size_t *psize, PIX *pix,
                        l_int32 quality, l_int32 nlevels, l_int32 hint)
{
    return ERROR_INT("function not present", "pixWriteMemJp2k", 1);
}

/* ----------------------------------------------------------------------*/

/* --------------------------------------------*/
#endif  /* !HAVE_LIBJP2K */
/* --------------------------------------------*/
