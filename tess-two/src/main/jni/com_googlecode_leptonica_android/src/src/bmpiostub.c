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
 *   bmpiostub.c
 *
 *      Stubs for bmpio.c functions
 */

#include "allheaders.h"

/* --------------------------------------------*/
#if  !USE_BMPIO   /* defined in environ.h */
/* --------------------------------------------*/

PIX * pixReadStreamBmp(FILE *fp)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadStreamBmp", NULL);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWriteStreamBmp(FILE *fp, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteStreamBmp", 1);
}

/* ----------------------------------------------------------------------*/

PIX * pixReadMemBmp(const l_uint8 *cdata, size_t size)
{
    return (PIX *)ERROR_PTR("function not present", "pixReadMemBmp", NULL);
}

/* ----------------------------------------------------------------------*/

l_int32 pixWriteMemBmp(l_uint8 **pdata, size_t *psize, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteMemBmp", 1);
}

/* --------------------------------------------*/
#endif  /* !USE_BMPIO */
/* --------------------------------------------*/
