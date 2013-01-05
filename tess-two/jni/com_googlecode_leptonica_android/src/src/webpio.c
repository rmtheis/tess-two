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
 *  webpio.c
 *
 *    Read WebP from file
 *          PIX             *pixReadStreamWebP()
 *          l_int32          readHeaderWebP()
 *
 *    Write WebP to file
 *          l_int32          pixWriteWebP()  [ special top level ]
 *          l_int32          pixWriteStreamWebP()
 *
 */

#include <math.h>
#include "allheaders.h"

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

/* --------------------------------------------*/
#if  HAVE_LIBWEBP   /* defined in environ.h */
/* --------------------------------------------*/
#include "webp/decode.h"
#include "webp/encode.h"

/*---------------------------------------------------------------------*
 *                              Reading WebP                            *
 *---------------------------------------------------------------------*/

/*!
 *  pixReadStreamWebP()
 *
 *      Input:  stream corresponding to WebP image
 *      Return: pix (32 bpp), or null on error
 *
 *  Notes:
 *      (1) Use 'free', and not leptonica's 'FREE', for all heap data
 *          that is returned from the WebP library.
 */
PIX *
pixReadStreamWebP(FILE  *fp)
{
l_uint8   *filedata;
l_uint8   *out = NULL;
l_int32    w, h, wpl, stride;
l_uint32  *data;
size_t     nbytes, sz, out_size;
PIX       *pix;

    PROCNAME("pixReadStreamWebP");

    if (!fp)
        return (PIX *)ERROR_PTR("fp not defined", procName, NULL);

        /* Read data from file and decode into Y,U,V arrays */
    rewind(fp);
    if ((filedata = l_binaryReadStream(fp, &nbytes)) == NULL)
        return (PIX *)ERROR_PTR("filedata not read", procName, NULL);

    sz = WebPGetInfo(filedata, nbytes, &w, &h);
    if (sz == 0) {
        FREE(filedata);
        return (PIX *)ERROR_PTR("Bad WebP: Can't find size", procName, NULL);
    }

        /* Write from Y,U,V arrays to pix data */
    pix = pixCreate(w, h, 32);
    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    stride = wpl * 4;
    out_size = stride * h;
    out = WebPDecodeRGBAInto(filedata, nbytes, (uint8_t *)data, out_size,
                             stride);
    FREE(filedata);
    if (out == NULL) {
        pixDestroy(&pix);
        return (PIX *)ERROR_PTR("WebP decode failed", procName, NULL);
    }

        /* Webp decoder emits opposite byte order for RGBA components */
    pixEndianByteSwap(pix);

    return pix;
}


/*!
 *  readHeaderWebP()
 *
 *      Input:  filename
 *              &width (<return>)
 *              &height (<return>)
 *      Return: 0 if OK, 1 on error
 */
l_int32
readHeaderWebP(const char *filename,
               l_int32    *pwidth,
               l_int32    *pheight)
{
l_uint8  data[10];
l_int32  sz;
FILE    *fp;

    PROCNAME("readHeaderWebP");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!pwidth || !pheight)
        return ERROR_INT("input ptr(s) not defined", procName, 1);
    if ((fp = fopenReadStream(filename)) == NULL)
        return ERROR_INT("image file not found", procName, 1);
    if (fread((char *)data, 1, 10, fp) != 10)
        return ERROR_INT("failed to read 10 bytes of file", procName, 1);

    sz = WebPGetInfo(data, 10, pwidth, pheight);
    if (sz == 0)
        return ERROR_INT("Bad WebP: Can't find size", procName, 1);

    fclose(fp);
    return 0;
}


/*---------------------------------------------------------------------*
 *                             Writing WebP                             *
 *---------------------------------------------------------------------*/
/*!
 *  pixWriteWebP()
 *
 *      Input:  filename
 *              pixs
 *              quality (1 - 100; 75 is default)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixWriteWebP(const char  *filename,
             PIX         *pixs,
             l_int32      quality)
{
FILE  *fp;

    PROCNAME("pixWriteWebP");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "wb+")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);

    if (pixWriteStreamWebP(fp, pixs, quality) != 0) {
        fclose(fp);
        return ERROR_INT("pixs not written to stream", procName, 1);
    }

    fclose(fp);
    return 0;
}


/*!
 *  pixWriteStreampWebP()
 *
 *      Input:  stream
 *              pix  (all depths)
 *              quality (1 - 100; 75 is default)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) webp only encodes rgb images, so the input image is converted to rgb
 *          if necessary.
 */
l_int32
pixWriteStreamWebP(FILE    *fp,
                   PIX     *pixs,
                   l_int32  quality)
{
l_int32    w, h, d, wpl, stride, ret;
l_uint8   *filedata = NULL;
l_uint32  *data;
size_t     nbytes;
PIX       *pix, *pixt, *pix32;

    PROCNAME("pixWriteStreamWebP");

    if (!fp)
        return ERROR_INT("stream not open", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

    if (quality < 1)
        quality = 1;
    if (quality > 100)
        quality = 100;

    if ((pixt = pixRemoveColormap(pixs, REMOVE_CMAP_TO_FULL_COLOR)) == NULL)
        return ERROR_INT("failure to remove color map", procName, 1);
    pixGetDimensions(pixt, &w, &h, &d);

        /* Convert to rgb if not 32 bpp */
    if (d != 32) {
        if ((pix32 = pixConvertTo32(pixt)) != NULL) {
            pixDestroy(&pixt);
            pixt = pix32;
            d = pixGetDepth(pixt);
        }
    }

        /* Webp encoder assumes opposite byte order for RGBA components */
    pix = pixEndianByteSwapNew(pixt);
    pixDestroy(&pixt);

    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    if (d != 32 || w <= 0 || h <= 0 || wpl <= 0 || !data) {
        pixDestroy(&pix);
        return ERROR_INT("bad or empty input pix", procName, 1);
    }

    stride = wpl * 4;
    nbytes = WebPEncodeRGBA((uint8_t *)data, w, h, stride, quality, &filedata);

    if (nbytes == 0) {
        if (filedata) free(filedata);
        pixDestroy(&pix);
        return ERROR_INT("WebPEncode failed", procName, 1);
    }

    rewind(fp);

    ret = (fwrite(filedata, 1, nbytes, fp) != nbytes);
    free(filedata);
    pixDestroy(&pix);
    if (ret)
        return ERROR_INT("Write error", procName, 1);

    return 0;
}

/* --------------------------------------------*/
#endif  /* HAVE_LIBWEBP */
/* --------------------------------------------*/
