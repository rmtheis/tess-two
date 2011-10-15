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
 *  tiffiostub.c
 *
 *     Stubs for tiffio.c functions
 */

#include <stdio.h>
#include "allheaders.h"

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

/* --------------------------------------------*/
#if  !HAVE_LIBTIFF   /* defined in environ.h */
/* --------------------------------------------*/

PIX * pixReadTiff(const char *filename, l_int32 n)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadTiff", NULL);
}

PIX * pixReadStreamTiff(FILE *fp, l_int32 n)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadStreamTiff", NULL);
}

l_int32 pixWriteTiff(const char *filename, PIX *pix, l_int32 comptype,
                     const char *modestring)
{
    return ERROR_INT("function not present", "pixWriteTiff", 1);
}

l_int32 pixWriteTiffCustom(const char *filename, PIX *pix, l_int32 comptype,
                           const char *modestring, NUMA *natags,
                           SARRAY *savals, SARRAY *satypes, NUMA *nasizes)
{
    return ERROR_INT("function not present", "pixWriteTiffCustom", 1);
}

l_int32 pixWriteStreamTiff(FILE *fp, PIX *pix, l_int32 comptype)
{
    return ERROR_INT("function not present", "pixWriteStreamTiff", 1);
}

PIXA * pixaReadMultipageTiff(const char *filename)
{
    return (PIXA * )ERROR_PTR("function not present",
                              "pixaReadMultipageTiff", NULL);
}

l_int32 writeMultipageTiff(const char *dirin, const char *substr,
                           const char *fileout)
{
    return ERROR_INT("function not present", "writeMultipageTiff", 1);
}

l_int32 writeMultipageTiffSA(SARRAY *sa, const char *fileout)
{
    return ERROR_INT("function not present", "writeMultipageTiffSA", 1);
}

l_int32 fprintTiffInfo(FILE *fpout, const char *tiffile)
{
    return ERROR_INT("function not present", "fprintTiffInfo", 1);
}

l_int32 tiffGetCount(FILE *fp, l_int32 *pn)
{
    return ERROR_INT("function not present", "tiffGetCount", 1);
}

l_int32 readHeaderTiff(const char *filename, l_int32 n, l_int32 *pwidth,
                       l_int32 *pheight, l_int32 *pbps, l_int32 *pspp,
                       l_int32 *pres, l_int32 *pcmap, l_int32 *pformat)
{
    return ERROR_INT("function not present", "readHeaderTiff", 1);
}

l_int32 freadHeaderTiff(FILE *fp, l_int32 n, l_int32 *pwidth,
                        l_int32 *pheight, l_int32 *pbps, l_int32 *pspp,
                        l_int32 *pres, l_int32 *pcmap, l_int32 *pformat)
{
    return ERROR_INT("function not present", "freadHeaderTiff", 1);
}

l_int32 readHeaderMemTiff(const l_uint8 *cdata, size_t size, l_int32 n,
                          l_int32 *pwidth, l_int32 *pheight, l_int32 *pbps,
                          l_int32 *pspp, l_int32 *pres, l_int32 *pcmap,
                          l_int32 *pformat)
{
    return ERROR_INT("function not present", "readHeaderMemTiff", 1);
}

l_int32 findTiffCompression(FILE *fp, l_int32 *pcomptype)
{
    return ERROR_INT("function not present", "findTiffCompression", 1);
}

l_int32 extractTiffG4DataFromFile(const char *filein, l_uint8 **pdata,
                                  l_int32 *pnbytes, l_int32 *pw,
                                  l_int32 *ph, l_int32 *pminisblack)
{
    return ERROR_INT("function not present", "extractTiffG4DataFromFile", 1);
}

PIX * pixReadMemTiff(const l_uint8 *cdata, size_t size, l_int32 n)
{
    return (PIX *)ERROR_PTR("function not present", "pixReadMemTiff", NULL);
}

l_int32 pixWriteMemTiff(l_uint8 **pdata, size_t *psize, PIX *pix,
                        l_int32 comptype)
{
    return ERROR_INT("function not present", "pixWriteMemTiff", 1);
}

l_int32 pixWriteMemTiffCustom(l_uint8 **pdata, size_t *psize, PIX *pix,
                              l_int32 comptype, NUMA *natags, SARRAY *savals,
                              SARRAY *satypes, NUMA *nasizes)
{
    return ERROR_INT("function not present", "pixWriteMemTiffCustom", 1);
}

/* --------------------------------------------*/
#endif  /* !HAVE_LIBTIFF */
/* --------------------------------------------*/

