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

l_int32 readHeaderWebP(const char *filename, l_int32 *pwidth, l_int32 *pheight)
{
    return ERROR_INT("function not present", "readHeaderWebP", 1);
}

l_int32 pixWriteWebP(const char *filename, PIX *pixs, l_int32 quality)
{
    return ERROR_INT("function not present", "pixWriteWebP", 1);
}

l_int32 pixWriteStreamWebP(FILE *fp, PIX *pixs, l_int32 quality)
{
    return ERROR_INT("function not present", "pixWriteStreamWebP", 1);
}

l_int32 pixWriteWebPwithTargetPSNR(const char *filename, PIX *pixs,
                                  l_float64 target_psnr, l_int32 *pquality)
{
    return ERROR_INT("function not present", "pixWriteWebPwithTargetPSNR", 1);
}

/* --------------------------------------------*/
#endif  /* !HAVE_LIBWEBP */
/* --------------------------------------------*/
