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
 *  pnmiostub.c
 *
 *     Stubs for pnmio.c functions
 */

#include "allheaders.h"

/* --------------------------------------------*/
#if  !USE_PNMIO   /* defined in environ.h */
/* --------------------------------------------*/

PIX * pixReadStreamPnm(FILE *fp)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadStreamPnm", NULL);
}

/* ----------------------------------------------------------------------*/

l_int32 readHeaderPnm(const char *filename, l_int32 *pw, l_int32 *ph,
                      l_int32 *pd, l_int32 *ptype, l_int32 *pbps,
                      l_int32 *pspp)
{
    return ERROR_INT("function not present", "readHeaderPnm", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 freadHeaderPnm(FILE *fp, l_int32 *pw, l_int32 *ph, l_int32 *pd,
                       l_int32 *ptype, l_int32 *pbps, l_int32 *pspp)
{
    return ERROR_INT("function not present", "freadHeaderPnm", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWriteStreamPnm(FILE *fp, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteStreamPnm", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWriteStreamAsciiPnm(FILE *fp, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteStreamAsciiPnm", 1);
}

/* ----------------------------------------------------------------------*/

PIX * pixReadMemPnm(const l_uint8 *cdata, size_t size)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadMemPnm", NULL);
}

/* ----------------------------------------------------------------------*/

l_int32 readHeaderMemPnm(const l_uint8 *cdata, size_t size, l_int32 *pw,
                         l_int32 *ph, l_int32 *pd, l_int32 *ptype,
                         l_int32 *pbps, l_int32 *pspp)
{
    return ERROR_INT("function not present", "readHeaderMemPnm", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWriteMemPnm(l_uint8 **pdata, size_t *psize, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteMemPnm", 1);
}

/* --------------------------------------------*/
#endif  /* !USE_PNMIO */
/* --------------------------------------------*/
