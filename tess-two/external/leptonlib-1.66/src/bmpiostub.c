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
 *   bmpiostub.c
 *
 *      Stubs for bmpio.c functions
 */

#include <stdio.h>
#include "allheaders.h"

/* --------------------------------------------*/
#if  !USE_BMPIO   /* defined in environ.h */
/* --------------------------------------------*/

PIX * pixReadStreamBmp(FILE *fp)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadStreamBmp", NULL);
}

l_int32 pixWriteStreamBmp(FILE *fp, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteStreamBmp", 1);
}

PIX * pixReadMemBmp(const l_uint8 *cdata, size_t size)
{
    return (PIX *)ERROR_PTR("function not present", "pixReadMemBmp", NULL);
}

l_int32 pixWriteMemBmp(l_uint8 **pdata, size_t *psize, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteMemBmp", 1);
}


/* --------------------------------------------*/
#endif  /* !USE_BMPIO */
/* --------------------------------------------*/

