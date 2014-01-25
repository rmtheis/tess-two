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
 *  jp2kio.c
 *
 *      Read header
 *          l_int32          readHeaderJp2k()
 *          l_int32          freadHeaderJp2k()
 *          l_int32          sreadHeaderJp2k()
 */

#include <string.h>
#include "allheaders.h"

/* --------------------------------------------*/
#if  USE_JP2KIO   /* defined in environ.h */
/* --------------------------------------------*/

    /* a sanity check on the size read from file */
static const l_int32  MAX_JP2K_WIDTH = 100000;
static const l_int32  MAX_JP2K_HEIGHT = 100000;


/*--------------------------------------------------------------------*
 *                          Stream interface                          *
 *--------------------------------------------------------------------*/
/*!
 *  readHeaderJp2k()
 *
 *      Input:  filename
 *              &w (<optional return>)
 *              &h (<optional return>)
 *              &spp (<optional return>, samples/pixel)
 *      Return: 0 if OK, 1 on error
 */
l_int32
readHeaderJp2k(const char *filename,
               l_int32    *pw,
               l_int32    *ph,
               l_int32    *pspp)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("readHeaderJp2k");

    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (pspp) *pspp = 0;
    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);

    if ((fp = fopenReadStream(filename)) == NULL)
        return ERROR_INT("image file not found", procName, 1);
    ret = freadHeaderJp2k(fp, pw, ph, pspp);
    fclose(fp);
    return ret;
}


/*!
 *  freadHeaderJp2k()
 *
 *      Input:  stream opened for read
 *              &w (<optional return>)
 *              &h (<optional return>)
 *              &spp (<optional return>, samples/pixel)
 *      Return: 0 if OK, 1 on error
 */
l_int32
freadHeaderJp2k(FILE     *fp,
                l_int32  *pw,
                l_int32  *ph,
                l_int32  *pspp)
{
l_uint8  buf[60];
l_int32  nread;

    PROCNAME("freadHeaderJp2k");

    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (pspp) *pspp = 0;
    if (!fp)
        return ERROR_INT("fp not defined", procName, 1);

    rewind(fp);
    nread = fread(buf, 1, sizeof(buf), fp);
    if (nread != sizeof(buf))
        return ERROR_INT("read failure", procName, 1);

    sreadHeaderJp2k(buf, sizeof(buf), pw, ph, pspp);
    rewind(fp);
    return 0;
}


/*!
 *  sreadHeaderJp2k()
 *
 *      Input:  data
 *              size
 *              &w (<optional return>)
 *              &h (<optional return>)
 *              &spp (<optional return>, samples/pixel)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The metadata is stored as follows:
 *          h:    4 bytes @ 48
 *          w:    4 bytes @ 52
 *          spp:  2 bytes @ 56
 */
l_int32
sreadHeaderJp2k(const l_uint8  *data,
                size_t          size,
                l_int32        *pw,
                l_int32        *ph,
                l_int32        *pspp)
{
l_int32  format, val, w, h, spp;

    PROCNAME("sreadHeaderJp2k");

    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (pspp) *pspp = 0;
    if (!data)
        return ERROR_INT("data not defined", procName, 1);
    if (size < 60)
        return ERROR_INT("size < 60", procName, 1);
    findFileFormatBuffer(data, &format);
    if (format != IFF_JP2)
        return ERROR_INT("not jp2 file", procName, 1);

    val = *((l_uint32 *)data + 12);
    h = convertOnLittleEnd32(val);
    val = *((l_uint32 *)data + 13);
    w = convertOnLittleEnd32(val);
    val = *((l_uint16 *)data + 28);
    spp = convertOnLittleEnd16(val);
    if (w > MAX_JP2K_WIDTH || h > MAX_JP2K_HEIGHT)
        return ERROR_INT("unrealistically large sizes", procName, 1);
    if (pw) *pw = w;
    if (ph) *ph = h;
    if (pspp) *pspp = spp;
    return 0;
}

/* --------------------------------------------*/
#endif  /* USE_JP2KIO */
/* --------------------------------------------*/
