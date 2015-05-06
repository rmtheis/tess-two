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
 *  jp2kheaderstub.c
 *
 *     Stubs for jp2kheader.c functions
 */

#include "allheaders.h"

/* --------------------------------------------*/
#if  !USE_JP2KHEADER   /* defined in environ.h */
/* --------------------------------------------*/

l_int32 readHeaderJp2k(const char *filename, l_int32 *pw, l_int32 *ph,
                       l_int32 *pbps, l_int32 *pspp)
{
    return ERROR_INT("function not present", "readHeaderJp2k", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 freadHeaderJp2k(FILE *fp, l_int32 *pw, l_int32 *ph,
                        l_int32 *pbps, l_int32 *pspp)
{
    return ERROR_INT("function not present", "freadHeaderJp2k", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 readHeaderMemJp2k(const l_uint8 *cdata, size_t size, l_int32 *pw,
                          l_int32 *ph, l_int32 *pbps, l_int32 *pspp)
{
    return ERROR_INT("function not present", "readHeaderMemJp2k", 1);
}

/* ----------------------------------------------------------------------*/

l_int32 fgetJp2kResolution(FILE *fp, l_int32 *pxres, l_int32 *pyres)
{
    return ERROR_INT("function not present", "fgetJp2kResolution", 1);
}

/* --------------------------------------------*/
#endif  /* !USE_JP2KHEADER */
/* --------------------------------------------*/
