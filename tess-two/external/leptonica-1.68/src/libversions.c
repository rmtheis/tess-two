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
 *  libversions.c
 *
 *       Image library version number
 *           char      *getImagelibVersions()
 */

#include "allheaders.h"

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

#if HAVE_LIBGIF
#include "gif_lib.h"
#endif

#if HAVE_LIBJPEG
/* jpeglib.h includes jconfig.h, which makes the error of setting
 *   #define HAVE_STDLIB_H
 * which conflicts with config_auto.h (where it is set to 1) and results
 * for some gcc compiler versions in a warning.  The conflict is harmless
 * but we suppress it by undefining the variable. */
#undef HAVE_STDLIB_H
#include "jpeglib.h"
#include "jerror.h"
#endif

#if HAVE_LIBPNG
#include "png.h"
#endif

#if HAVE_LIBTIFF
#include "tiffio.h"
#endif

#if HAVE_LIBZ
#include "zlib.h"
#endif

#define stringJoinInPlace(s1, s2) \
    tempStrP = stringJoin(s1,s2); FREE(s1); s1 = tempStrP;


/*---------------------------------------------------------------------*
 *                    Image Library Version number                     *
 *---------------------------------------------------------------------*/
/*! 
 *  getImagelibVersions()
 *
 *      Return: string of version numbers (e.g.,
 *               libgif 4.1.6
 *               libjpeg 8b
 *               libpng 1.4.3
 *               libtiff 3.9.4
 *               zlib 1.2.5
 *
 *  Notes:
 *      (1) The caller has responsibility to free the memory.
 */
char *
getImagelibVersions()
{
#if HAVE_LIBJPEG
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr err;
    char buffer[JMSG_LENGTH_MAX];
#endif
    char *tempStrP;
    char *versionNumP;
    char *nextTokenP;
    char *versionStrP = stringNew("");

#if HAVE_LIBGIF
    stringJoinInPlace(versionStrP, "libgif 4.1.6 : ");
#endif

#if HAVE_LIBJPEG
    cinfo.err = jpeg_std_error(&err);
    err.msg_code = JMSG_VERSION;
    (*err.format_message) ((j_common_ptr ) &cinfo, buffer);

    stringJoinInPlace(versionStrP, "libjpeg ");
    versionNumP = strtokSafe(buffer, " ", &nextTokenP);
    stringJoinInPlace(versionStrP, versionNumP);
    stringJoinInPlace(versionStrP, " : ");
    FREE(versionNumP);
#endif

#if HAVE_LIBPNG
    stringJoinInPlace(versionStrP, "libpng ");
    stringJoinInPlace(versionStrP, png_get_libpng_ver(NULL));
    stringJoinInPlace(versionStrP, " : ");
#endif

#if HAVE_LIBTIFF
    stringJoinInPlace(versionStrP, "libtiff ");
    versionNumP = strtokSafe((char *)TIFFGetVersion(), " \n", &nextTokenP);
    FREE(versionNumP);
    versionNumP = strtokSafe(NULL, " \n", &nextTokenP);
    FREE(versionNumP);
    versionNumP = strtokSafe(NULL, " \n", &nextTokenP);
    stringJoinInPlace(versionStrP, versionNumP);
    stringJoinInPlace(versionStrP, " : ");
    FREE(versionNumP);
#endif

#if HAVE_LIBZ
    stringJoinInPlace(versionStrP, "zlib ");
    stringJoinInPlace(versionStrP, zlibVersion());
#endif
    stringJoinInPlace(versionStrP, "\n");

    return versionStrP;
}

