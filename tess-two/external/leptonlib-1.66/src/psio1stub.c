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
 *  psio1stub.c
 *
 *     Stubs for psio1.c functions
 */

#include <stdio.h>
#include "allheaders.h"

/* --------------------------------------------*/
#if  !USE_PSIO   /* defined in environ.h */
/* --------------------------------------------*/

l_int32 convertFilesToPS(const char *dirin, const char *substr,
                         l_int32 res, const char *fileout)
{
    return ERROR_INT("function not present", "convertFilesToPS", 1);
}

l_int32 sarrayConvertFilesToPS(SARRAY *sa, l_int32 res, const char *fileout)
{
    return ERROR_INT("function not present", "sarrayConvertFilesToPS", 1);
}

l_int32 convertFilesFittedToPS(const char *dirin, const char *substr,
                               l_float32 xpts, l_float32 ypts,
                               const char *fileout)
{
    return ERROR_INT("function not present", "convertFilesFittedToPS", 1);
}

l_int32 sarrayConvertFilesFittedToPS(SARRAY *sa, l_float32 xpts,
                                     l_float32 ypts, const char *fileout)
{
    return ERROR_INT("function not present", "sarrayConvertFilesFittedToPS", 1);
}

l_int32 convertSegmentedPagesToPS(const char *pagedir, const char *pagestr,
                                  const char *maskdir, const char *maskstr,
                                  l_int32 numpre, l_int32 numpost,
                                  l_int32 maxnum, l_float32 textscale,
                                  l_float32 imagescale, l_int32 threshold,
                                  const char *fileout)
{
    return ERROR_INT("function not present", "convertSegmentedPagesToPS", 1);
}

l_int32 pixWriteSegmentedPageToPS(PIX *pixs, PIX *pixm, l_float32 textscale,
                                  l_float32 imagescale, l_int32 threshold,
                                  l_int32 pageno, const char *fileout)
{
    return ERROR_INT("function not present", "pixWriteSegmentedPagesToPS", 1);
}

l_int32 pixWriteMixedToPS(PIX *pixb, PIX *pixc, l_float32 scale,
                          l_int32 pageno, const char *fileout)
{
    return ERROR_INT("function not present", "pixWriteMixedToPS", 1);
}

l_int32 convertToPSEmbed(const char *filein, const char *fileout, l_int32 level)
{
    return ERROR_INT("function not present", "convertToPSEmbed", 1);
}

l_int32 pixaWriteCompressedToPS(PIXA *pixa, const char *fileout,
                                l_int32 res, l_int32 level)
{
    return ERROR_INT("function not present", "pixaWriteCompressedtoPS", 1);
}

/* --------------------------------------------*/
#endif  /* !USE_PSIO */
/* --------------------------------------------*/

