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
 -  Author: krish@google.com (krish Chaudhury)
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
 *    Write WebP to file with target psnr
 *          l_int32          pixWriteWebPwithTargetPSNR
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
    pix = pixEndianByteSwapNew(pixt);
    pixDestroy(&pixt);
    pixGetDimensions(pix, &w, &h, &d);

        /* Convert to rgb if not 32 bpp */
    if (d != 32) {
        if ((pix32 = pixConvertTo32(pix)) != NULL) {
            pixDestroy(&pix);
            pix = pix32;
            d = pixGetDepth(pix);
        }
    }

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


/*!
 *  pixWriteWebPwithTargetPSNR()
 *
 *      Input:  filename
 *              pix  (all depths)
 *              target_psnr (target psnr to control the quality [1 ... 100])
 *              pquality (<optional return> final quality value used to obtain
 *                   the target_psnr; can be null)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The parameter to control quality while encoding WebP is quality.
 *          This function does a line search over the quality values between
 *          MIN_QUALITY and MAX_QUALITY to achieve the target PSNR as close as
 *          possible.
 */
l_int32
pixWriteWebPwithTargetPSNR(const char  *filename,
                           PIX         *pixs,
                           l_float64    target_psnr,
                           l_int32     *pquality)
{
l_uint8   *filedata = NULL;
l_uint8   *tmp_filedata = NULL;
l_int32    MIN_QUALITY = 1;    /* min allowed value of quality */
l_int32    MAX_QUALITY = 100;  /* max allowed value of quality */
l_int32    w, h, d, wpl, stride, ret;
l_int32    quality, delta_quality, quality_test, accept;
l_uint32  *data;
l_float64  psnr, psnr_test;
size_t     nbytes, tmp_nbytes = 0;
FILE      *fp;
PIX       *pix, *pix32;

    PROCNAME("pixWriteWebPwithTargetPSNR");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (target_psnr <= 0 || target_psnr >= 100)
        return ERROR_INT("Target psnr out of range", procName, 1);

    if ((pix = pixRemoveColormap(pixs, REMOVE_CMAP_TO_FULL_COLOR)) == NULL)
        return ERROR_INT("cannot remove color map", procName, 1);
    pixGetDimensions(pix, &w, &h, &d);

        /* Convert to rgb if not 32 bpp */
    if (d != 32) {
        if ((pix32 = pixConvertTo32(pix)) != NULL) {
            pixDestroy(&pix);
            pix = pix32;
            d = pixGetDepth(pix);
        }
    }

    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    if (d != 32 || w <= 0 || h <= 0 || wpl <= 0 || !data) {
        pixDestroy(&pix);
        return ERROR_INT("bad or empty input pix", procName, 1);
    }

        /* Set the initial value of the Quality parameter.  In each iteration
         * it will then increase or decrease the Quality value, based on
         * whether the achieved psnr is higher or lower than the target_psnr */
    quality = 75;
    stride = wpl * 4;

    nbytes = WebPEncodeRGBA((uint8_t *)data, w, h, stride, quality, &filedata);

    if (nbytes == 0) {
        if (filedata) free(filedata);
        pixDestroy(&pix);
        return ERROR_INT("WebPEncode failed", procName, 1);
    }

        /* Rationale about the delta_quality being limited: we expect optimal
         * quality to be not too far from target quality in practice.
         * So instead of a full dichotomy for the whole range we cap
         * |delta_quality| to only explore quickly around the starting value
         * and maximize the return in investment. */
    delta_quality = (psnr > target_psnr) ?
        L_MAX((MAX_QUALITY - quality) / 4, 1) :
        L_MIN((MIN_QUALITY - quality) / 4, -1);

    while (delta_quality != 0) {
            /* Advance quality and clip to valid range */
        quality_test = L_MIN(L_MAX(quality + delta_quality, MIN_QUALITY),
                             MAX_QUALITY);
            /* Re-adjust delta value after Quality-clipping. */
        delta_quality = quality_test - quality;

        tmp_nbytes = WebPEncodeRGBA((uint8_t *)data, w, h, stride, quality_test,
                                    &tmp_filedata);
        if (tmp_nbytes == 0) {
            free(filedata);
            if (tmp_filedata) free(tmp_filedata);
            pixDestroy(&pix);
            return ERROR_INT("WebPEncode failed", procName, 1);
        }

            /* Accept or reject new settings */
        accept = (psnr_test > target_psnr) ^ (delta_quality < 0);
        if (accept) {
            free(filedata);
            filedata = tmp_filedata;
            nbytes = tmp_nbytes;
            quality = quality_test;
            psnr = psnr_test;
        }
        else {
            delta_quality /= 2;
            free(tmp_filedata);
        }
    }
    if (pquality) *pquality = quality;

    if ((fp = fopenWriteStream(filename, "wb+")) == NULL) {
        free(filedata);
        pixDestroy(&pix);
        return ERROR_INT("stream not opened", procName, 1);
    }

    ret = (fwrite(filedata, 1, nbytes, fp) != nbytes);
    fclose(fp);
    free(filedata);
    pixDestroy(&pix);
    if (ret)
        return ERROR_INT("Write error", procName, 1);

    return 0;
}

/* --------------------------------------------*/
#endif  /* HAVE_LIBWEBP */
/* --------------------------------------------*/
