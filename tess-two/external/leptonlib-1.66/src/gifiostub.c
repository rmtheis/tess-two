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
 *  gifiostub.c
 *
 *     Stubs for gifio.c functions
 */

#include <stdio.h>
#include "allheaders.h"

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

/* -----------------------------------------------------------------*/
#if  (!HAVE_LIBGIF) && (!HAVE_LIBUNGIF)     /* defined in environ.h */
/* -----------------------------------------------------------------*/

PIX * pixReadStreamGif(FILE *fp)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadStreamGif", NULL);
}

l_int32 pixWriteStreamGif(FILE *fp, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteStreamGif", 1);
}

PIX * pixReadMemGif(const l_uint8 *cdata, size_t size)
{
    return (PIX *)ERROR_PTR("function not present", "pixReadMemGif", NULL);
}

l_int32 pixWriteMemGif(l_uint8 **pdata, size_t *psize, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteMemGif", 1);
}

/* -----------------------------------------------------------------*/
#endif      /* !HAVE_LIBGIF && !HAVE_LIBUNGIF */
/* -----------------------------------------------------------------*/

