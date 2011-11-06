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
 *  pngiostub.c
 *
 *     Stubs for pngio.c functions
 */

#include "allheaders.h"

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

/* --------------------------------------------*/
#if  !HAVE_LIBPNG   /* defined in environ.h */
/* --------------------------------------------*/

PIX * pixReadStreamPng(FILE *fp)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadStreamPng", NULL);
}

l_int32 readHeaderPng(const char *filename, l_int32 *pwidth, l_int32 *pheight, l_int32 *pbps, l_int32 *pspp, l_int32 *piscmap)
{
    return ERROR_INT("function not present", "readHeaderPng", 1);
}

l_int32 freadHeaderPng(FILE *fp, l_int32 *pwidth, l_int32 *pheight, l_int32 *pbps, l_int32 *pspp, l_int32 *piscmap)
{
    return ERROR_INT("function not present", "freadHeaderPng", 1);
}

l_int32 sreadHeaderPng(const l_uint8 *data, l_int32 *pwidth, l_int32 *pheight, l_int32 *pbps, l_int32 *pspp, l_int32 *piscmap)
{
    return ERROR_INT("function not present", "sreadHeaderPng", 1);
}

l_int32 fgetPngResolution(FILE *fp, l_int32 *pxres, l_int32 *pyres)
{
    return ERROR_INT("function not present", "fgetPngResolution", 1);
}

l_int32 pixWritePng(const char *filename, PIX *pix, l_float32 gamma)
{
    return ERROR_INT("function not present", "pixWritePng", 1);
}

l_int32 pixWriteStreamPng(FILE *fp, PIX *pix, l_float32 gamma)
{
    return ERROR_INT("function not present", "pixWriteStreamPng", 1);
}

PIX * pixReadRGBAPng(const char *filename)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadRGBAPng", NULL);
}

l_int32 pixWriteRGBAPng(const char *filename, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteRGBAPng", 1);
}

void l_pngSetStrip16To8(l_int32 flag)
{
    L_ERROR("function not present", "l_pngSetSTrip16To8");
    return;
}

void l_pngSetStripAlpha(l_int32 flag)
{
    L_ERROR("function not present", "l_pngSetStripAlpha");
    return;
}

void l_pngSetWriteAlpha(l_int32 flag)
{
    L_ERROR("function not present", "l_pngSetWriteAlpha");
    return;
}

void l_pngSetZlibCompression(l_int32 val)

{
    L_ERROR("function not present", "l_pngSetZlibCompression");
    return;
}

PIX * pixReadMemPng(const l_uint8 *cdata, size_t size)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadMemPng", NULL);
}

l_int32 pixWriteMemPng(l_uint8 **pdata, size_t *psize, PIX *pix, l_float32 gamma)
{
    return ERROR_INT("function not present", "pixWriteMemPng", 1);
}

/* --------------------------------------------*/
#endif  /* !HAVE_LIBPNG */
/* --------------------------------------------*/

