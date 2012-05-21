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

l_int32 freadHeaderPnm(FILE *fp, PIX **ppix, l_int32 *pwidth, l_int32 *pheight,                        l_int32 *pdepth, l_int32 *ptype, l_int32 *pbps,
                       l_int32 *pspp)
{
    return ERROR_INT("function not present", "freadHeaderPnm", 1);
}

l_int32 pixWriteStreamPnm(FILE *fp, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteStreamPnm", 1);
}

l_int32 pixWriteStreamAsciiPnm(FILE *fp, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteStreamAsciiPnm", 1);
}

PIX * pixReadMemPnm(const l_uint8 *cdata, size_t size)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadMemPnm", NULL);
}

l_int32 sreadHeaderPnm(const l_uint8 *cdata, size_t size, l_int32 *pwidth,
                       l_int32 *pheight, l_int32 *pdepth, l_int32 *ptype,
                       l_int32 *pbps, l_int32 *pspp)
{
    return ERROR_INT("function not present", "sreadHeaderPnm", 1);
}

l_int32 pixWriteMemPnm(l_uint8 **pdata, size_t *psize, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteMemPnm", 1);
}



/* --------------------------------------------*/
#endif  /* !USE_PNMIO */
/* --------------------------------------------*/

