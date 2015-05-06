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

#if HAVE_LIBWEBP
#include "webp/encode.h"
#endif

#if HAVE_LIBJP2K  /* assuming it's 2.1 */
#include "openjpeg-2.1/openjpeg.h"
#endif

#define stringJoinInPlace(s1, s2) \
    { tempStrP = stringJoin((s1),(s2)); FREE(s1); (s1) = tempStrP; }


/*---------------------------------------------------------------------*
 *                    Image Library Version number                     *
 *---------------------------------------------------------------------*/
/*!
 *  getImagelibVersions()
 *
 *      Return: string of version numbers; e.g.,
 *               libgif 5.0.3
 *               libjpeg 8b
 *               libpng 1.4.3
 *               libtiff 3.9.5
 *               zlib 1.2.5
 *               libwebp 0.3.0
 *               libopenjp2 2.1.0
 *
 *  Notes:
 *      (1) The caller has responsibility to free the memory.
 */
char *
getImagelibVersions()
{
char     buf[128];
l_int32  first = TRUE;

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
    first = FALSE;
    stringJoinInPlace(versionStrP, "libgif ");
  #ifdef GIFLIB_MAJOR
    snprintf(buf, sizeof(buf), "%d.%d.%d", GIFLIB_MAJOR, GIFLIB_MINOR,
             GIFLIB_RELEASE);
  #else
    stringCopy(buf, "4.1.6(?)", sizeof(buf));
  #endif
    stringJoinInPlace(versionStrP, buf);
#endif

#if HAVE_LIBJPEG
    cinfo.err = jpeg_std_error(&err);
    err.msg_code = JMSG_VERSION;
    (*err.format_message) ((j_common_ptr ) &cinfo, buffer);

    if (!first) stringJoinInPlace(versionStrP, " : ");
    first = FALSE;
    stringJoinInPlace(versionStrP, "libjpeg ");
    versionNumP = strtokSafe(buffer, " ", &nextTokenP);
    stringJoinInPlace(versionStrP, versionNumP);
    FREE(versionNumP);
#endif

#if HAVE_LIBPNG
    if (!first) stringJoinInPlace(versionStrP, " : ");
    first = FALSE;
    stringJoinInPlace(versionStrP, "libpng ");
    stringJoinInPlace(versionStrP, png_get_libpng_ver(NULL));
#endif

#if HAVE_LIBTIFF
    if (!first) stringJoinInPlace(versionStrP, " : ");
    first = FALSE;
    stringJoinInPlace(versionStrP, "libtiff ");
    versionNumP = strtokSafe((char *)TIFFGetVersion(), " \n", &nextTokenP);
    FREE(versionNumP);
    versionNumP = strtokSafe(NULL, " \n", &nextTokenP);
    FREE(versionNumP);
    versionNumP = strtokSafe(NULL, " \n", &nextTokenP);
    stringJoinInPlace(versionStrP, versionNumP);
    FREE(versionNumP);
#endif

#if HAVE_LIBZ
    if (!first) stringJoinInPlace(versionStrP, " : ");
    first = FALSE;
    stringJoinInPlace(versionStrP, "zlib ");
    stringJoinInPlace(versionStrP, zlibVersion());
#endif

#if HAVE_LIBWEBP
    {
    l_int32 val;
    char buf[32];
    if (!first) stringJoinInPlace(versionStrP, " : ");
    first = FALSE;
    stringJoinInPlace(versionStrP, "libwebp ");
    val = WebPGetEncoderVersion();
    snprintf(buf, sizeof(buf), "%d.%d.%d", val >> 16, (val >> 8) & 0xff,
             val & 0xff);
    stringJoinInPlace(versionStrP, buf);
    }
#endif

#if HAVE_LIBJP2K
    {
    const char *version;
    if (!first) stringJoinInPlace(versionStrP, " : ");
    first = FALSE;
    stringJoinInPlace(versionStrP, "libopenjp2 ");
    version = opj_version();
    stringJoinInPlace(versionStrP, version);
    }
#endif

    stringJoinInPlace(versionStrP, "\n");
    return versionStrP;
}
