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
 *  zlibmemstub.c
 *
 *     Stubs for zlibmem.c functions
 */

#include "allheaders.h"

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

/* --------------------------------------------*/
#if  !HAVE_LIBZ   /* defined in environ.h */
/* --------------------------------------------*/

l_uint8 * zlibCompress(l_uint8 *datain, size_t nin, size_t *pnout)
{
    return (l_uint8 *)ERROR_PTR("function not present", "zlibCompress", NULL);
}

l_uint8 * zlibUncompress(l_uint8 *datain, size_t nin, size_t *pnout)
{
    return (l_uint8 *)ERROR_PTR("function not present", "zlibUncompress", NULL);
}

/* --------------------------------------------*/
#endif  /* !HAVE_LIBZ */
/* --------------------------------------------*/

