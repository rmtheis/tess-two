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
 *  pngio.c
 *
 *    Read png from file
 *          PIX        *pixReadStreamPng()
 *          l_int32     readHeaderPng()
 *          l_int32     freadHeaderPng()
 *          l_int32     readHeaderMemPng()
 *          l_int32     fgetPngResolution()
 *          l_int32     isPngInterlaced()
 *
 *    Write png to file
 *          l_int32     pixWritePng()  [ special top level ]
 *          l_int32     pixWriteStreamPng()
 *          l_int32     pixSetZlibCompression()
 *
 *    Setting flag for special read mode
 *          void        l_pngSetReadStrip16To8()
 *
 *    Read/write to memory
 *          PIX        *pixReadMemPng()
 *          l_int32     pixWriteMemPng()
 *
 *    Documentation: libpng.txt and example.c
 *
 *    On input (decompression from file), palette color images
 *    are read into an 8 bpp Pix with a colormap, and 24 bpp
 *    3 component color images are read into a 32 bpp Pix with
 *    rgb samples.  On output (compression to file), palette color
 *    images are written as 8 bpp with the colormap, and 32 bpp
 *    full color images are written compressed as a 24 bpp,
 *    3 component color image.
 *
 *    In the following, we use these abbreviations:
 *       bps == bit/sample
 *       spp == samples/pixel
 *       bpp == bits/pixel of image in Pix (memory)
 *    where each component is referred to as a "sample".
 *
 *    For reading and writing rgb and rgba images, we read and write
 *    alpha if it exists (spp == 4) and do not read or write if
 *    it doesn't (spp == 3).  The alpha component can be 'removed'
 *    simply by setting spp to 3.  In leptonica, we make relatively
 *    little explicit use of the alpha sample.  Note that the alpha
 *    sample in the image is also called "alpha transparency",
 *    "alpha component" and "alpha layer."
 *
 *    To change the zlib compression level, use pixSetZlibCompression()
 *    before writing the file.  The default is for standard png compression.
 *    The zlib compression value can be set [0 ... 9], with
 *         0     no compression (huge files)
 *         1     fastest compression
 *         -1    default compression  (equivalent to 6 in latest version)
 *         9     best compression
 *    Note that if you are using the defined constants in zlib instead
 *    of the compression integers given above, you must include zlib.h.
 *
 *    There is global for determining the size of retained samples:
 *             var_PNG_STRIP_16_to_8
 *    and a function l_pngSetReadStrip16To8() for setting it.
 *    The default is TRUE, which causes pixRead() to strip each 16 bit
 *    sample down to 8 bps:
 *     - For 16 bps rgb (16 bps, 3 spp) --> 32 bpp rgb Pix
 *     - For 16 bps gray (16 bps, 1 spp) --> 8 bpp grayscale Pix
 *    If the variable is set to FALSE, the 16 bit gray samples
 *    are saved when read; the 16 bit rgb samples return an error.
 *    Note: results can be non-deterministic if used with
 *    multi-threaded applications.
 *
 *    On systems like windows without fmemopen() and open_memstream(),
 *    we write data to a temp file and read it back for operations
 *    between pix and compressed-data, such as pixReadMemPng() and
 *    pixWriteMemPng().
 */

#include <string.h>
#include "allheaders.h"

#ifdef  HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

/* --------------------------------------------*/
#if  HAVE_LIBPNG   /* defined in environ.h */
/* --------------------------------------------*/

#include "png.h"

#if  HAVE_LIBZ
#include "zlib.h"
#else
#define  Z_DEFAULT_COMPRESSION (-1)
#endif  /* HAVE_LIBZ */

/* ------------------ Set default for read option -------------------- */
    /* Strip 16 bpp --> 8 bpp on reading png; default is for stripping.
     * If you don't strip, you can't read the gray-alpha spp = 2 images. */
static l_int32   var_PNG_STRIP_16_TO_8 = 1;


#ifndef  NO_CONSOLE_IO
#define  DEBUG_READ     0
#define  DEBUG_WRITE    0
#endif  /* ~NO_CONSOLE_IO */


/*---------------------------------------------------------------------*
 *                              Reading png                            *
 *---------------------------------------------------------------------*/
/*!
 *  pixReadStreamPng()
 *
 *      Input:  stream
 *      Return: pix, or null on error
 *
 *  Notes:
 *      (1) If called from pixReadStream(), the stream is positioned
 *          at the beginning of the file.
 *      (2) To do sequential reads of png format images from a stream,
 *          use pixReadStreamPng()
 *      (3) Any image with alpha is converted to RGBA (spp = 4, with
 *          equal red, green and blue channels) on reading.
 *          There are three important cases with alpha:
 *          (a) grayscale-with-alpha (spp = 2), where bpp = 8, and each
 *              pixel has an associated alpha (transparency) value
 *              in the second component of the image data.
 *          (b) spp = 1, d = 1 with colormap and alpha in the trans array.
 *              Transparency is usually associated with the white background.
 *          (c) spp = 1, d = 8 with colormap and alpha in the trans array.
 *              Each color in the colormap has a separate transparency value.
 *      (4) We use the high level png interface, where the transforms are set
 *          up in advance and the header and image are read with a single
 *          call.  The more complicated interface, where the header is
 *          read first and the buffers for the raster image are user-
 *          allocated before reading the image, works for single images,
 *          but I could not get it to work properly for the successive
 *          png reads that are required by pixaReadStream().
 */
PIX *
pixReadStreamPng(FILE  *fp)
{
l_uint8      byte;
l_int32      rval, gval, bval;
l_int32      i, j, k, index, ncolors, bitval;
l_int32      wpl, d, spp, cindex, tRNS;
l_uint32     png_transforms;
l_uint32    *data, *line, *ppixel;
int          num_palette, num_text, num_trans;
png_byte     bit_depth, color_type, channels;
png_uint_32  w, h, rowbytes;
png_uint_32  xres, yres;
png_bytep    rowptr, trans;
png_bytep   *row_pointers;
png_structp  png_ptr;
png_infop    info_ptr, end_info;
png_colorp   palette;
png_textp    text_ptr;  /* ptr to text_chunk */
PIX         *pix;
PIXCMAP     *cmap;

    PROCNAME("pixReadStreamPng");

    if (!fp)
        return (PIX *)ERROR_PTR("fp not defined", procName, NULL);
    pix = NULL;

        /* Allocate the 3 data structures */
    if ((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                   (png_voidp)NULL, NULL, NULL)) == NULL)
        return (PIX *)ERROR_PTR("png_ptr not made", procName, NULL);

    if ((info_ptr = png_create_info_struct(png_ptr)) == NULL) {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        return (PIX *)ERROR_PTR("info_ptr not made", procName, NULL);
    }

    if ((end_info = png_create_info_struct(png_ptr)) == NULL) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        return (PIX *)ERROR_PTR("end_info not made", procName, NULL);
    }

        /* Set up png setjmp error handling */
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        return (PIX *)ERROR_PTR("internal png error", procName, NULL);
    }

    png_init_io(png_ptr, fp);

        /* ---------------------------------------------------------- *
         *  Set the transforms flags.  Whatever happens here,
         *  NEVER invert 1 bpp using PNG_TRANSFORM_INVERT_MONO.
         *  Also, do not use PNG_TRANSFORM_EXPAND, which would
         *  expand all images with bpp < 8 to 8 bpp.
         * ---------------------------------------------------------- */
        /* To strip 16 --> 8 bit depth, use PNG_TRANSFORM_STRIP_16 */
    if (var_PNG_STRIP_16_TO_8 == 1) {  /* our default */
        png_transforms = PNG_TRANSFORM_STRIP_16;
    } else {
        png_transforms = PNG_TRANSFORM_IDENTITY;
        L_INFO("not stripping 16 --> 8 in png reading\n", procName);
    }

        /* Read it */
    png_read_png(png_ptr, info_ptr, png_transforms, NULL);

    row_pointers = png_get_rows(png_ptr, info_ptr);
    w = png_get_image_width(png_ptr, info_ptr);
    h = png_get_image_height(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
    channels = png_get_channels(png_ptr, info_ptr);
    spp = channels;
    tRNS = png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) ? 1 : 0;

    if (spp == 1) {
        d = bit_depth;
    } else {  /* spp == 2 (gray + alpha), spp == 3 (rgb), spp == 4 (rgba) */
        d = 4 * bit_depth;
    }

        /* Remove if/when this is implemented for all bit_depths */
    if (spp == 3 && bit_depth != 8) {
        fprintf(stderr, "Help: spp = 3 and depth = %d != 8\n!!", bit_depth);
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        return (PIX *)ERROR_PTR("not implemented for this depth",
            procName, NULL);
    }

    if (color_type == PNG_COLOR_TYPE_PALETTE ||
        color_type == PNG_COLOR_MASK_PALETTE) {   /* generate a colormap */
        png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
        cmap = pixcmapCreate(d);  /* spp == 1 */
        for (cindex = 0; cindex < num_palette; cindex++) {
            rval = palette[cindex].red;
            gval = palette[cindex].green;
            bval = palette[cindex].blue;
            pixcmapAddColor(cmap, rval, gval, bval);
        }
    } else {
        cmap = NULL;
    }

    if ((pix = pixCreate(w, h, d)) == NULL) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        return (PIX *)ERROR_PTR("pix not made", procName, NULL);
    }
    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    pixSetColormap(pix, cmap);
    pixSetSpp(pix, spp);

    if (spp == 1 && !tRNS) {  /* copy straight from buffer to pix */
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            rowptr = row_pointers[i];
            for (j = 0; j < rowbytes; j++) {
                    SET_DATA_BYTE(line, j, rowptr[j]);
            }
        }
    } else if (spp == 2) {  /* grayscale + alpha; convert to RGBA */
        L_INFO("converting (gray + alpha) ==> RGBA\n", procName);
        for (i = 0; i < h; i++) {
            ppixel = data + i * wpl;
            rowptr = row_pointers[i];
            for (j = k = 0; j < w; j++) {
                    /* Copy gray value into r, g and b */
                SET_DATA_BYTE(ppixel, COLOR_RED, rowptr[k]);
                SET_DATA_BYTE(ppixel, COLOR_GREEN, rowptr[k]);
                SET_DATA_BYTE(ppixel, COLOR_BLUE, rowptr[k++]);
                SET_DATA_BYTE(ppixel, L_ALPHA_CHANNEL, rowptr[k++]);
                ppixel++;
            }
        }
        pixSetSpp(pix, 4);  /* we do not support 2 spp pix */
    } else if (spp == 3 || spp == 4) {
        for (i = 0; i < h; i++) {
            ppixel = data + i * wpl;
            rowptr = row_pointers[i];
            for (j = k = 0; j < w; j++) {
                SET_DATA_BYTE(ppixel, COLOR_RED, rowptr[k++]);
                SET_DATA_BYTE(ppixel, COLOR_GREEN, rowptr[k++]);
                SET_DATA_BYTE(ppixel, COLOR_BLUE, rowptr[k++]);
                if (spp == 4)
                    SET_DATA_BYTE(ppixel, L_ALPHA_CHANNEL, rowptr[k++]);
                ppixel++;
            }
        }
    }

        /* Special spp == 1 cases with transparency:
         *    (1) 8 bpp without colormap; assume full transparency
         *    (2) 1 bpp with colormap + trans array (for alpha)
         *    (3) 8 bpp with colormap + trans array (for alpha)
         * These all require converting to RGBA */
    if (spp == 1 && tRNS) {
        if (!cmap) {
                /* Case 1: make fully transparent RGBA image */
            L_INFO("transparency, 1 spp, no colormap, no transparency array: "
                   "convention is fully transparent image\n", procName);
            L_INFO("converting (fully transparent 1 spp) ==> RGBA\n", procName);
            pixDestroy(&pix);
            pix = pixCreate(w, h, 32);  /* init to alpha = 0 (transparent) */
            pixSetSpp(pix, 4);
        } else {
            L_INFO("converting (cmap + alpha) ==> RGBA\n", procName);

                /* Grab the transparency array */
            png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, NULL);
            if (!trans) {  /* invalid png file */
                pixDestroy(&pix);
                png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
                return (PIX *)ERROR_PTR("cmap, tRNS, but no transparency array",
                                        procName, NULL);
            }

                /* Save the cmap and destroy the pix */
            cmap = pixcmapCopy(pixGetColormap(pix));
            ncolors = pixcmapGetCount(cmap);
            pixDestroy(&pix);

                /* Start over with 32 bit RGBA */
            pix = pixCreate(w, h, 32);
            wpl = pixGetWpl(pix);
            data = pixGetData(pix);
            pixSetSpp(pix, 4);

#if DEBUG_READ
            fprintf(stderr, "ncolors = %d, num_trans = %d\n",
                    ncolors, num_trans);
            for (i = 0; i < ncolors; i++) {
                pixcmapGetColor(cmap, i, &rval, &gval, &bval);
                if (i < num_trans) {
                    fprintf(stderr, "(r,g,b,a) = (%d,%d,%d,%d)\n",
                            rval, gval, bval, trans[i]);
                } else {
                    fprintf(stderr, "(r,g,b,a) = (%d,%d,%d,<<255>>)\n",
                            rval, gval, bval);
                }
            }
#endif  /* DEBUG_READ */

                /* Extract the data and convert to RGBA */
            if (d == 1) {
                    /* Case 2: 1 bpp with transparency (usually) behind white */
                L_INFO("converting 1 bpp cmap with alpha ==> RGBA\n", procName);
                if (num_trans < 2)
                    L_WARNING("num_trans = %d; should be 2\n", procName,
                              num_trans);
                for (i = 0; i < h; i++) {
                    ppixel = data + i * wpl;
                    rowptr = row_pointers[i];
                    for (j = 0, index = 0; j < rowbytes; j++) {
                        byte = rowptr[j];
                        for (k = 0; k < 8 && index < w; k++, index++) {
                            bitval = (byte >> (7 - k)) & 1;
                            pixcmapGetColor(cmap, bitval, &rval, &gval, &bval);
                            composeRGBPixel(rval, gval, bval, ppixel);
                            SET_DATA_BYTE(ppixel, L_ALPHA_CHANNEL,
                                      bitval < num_trans ? trans[bitval] : 255);
                            ppixel++;
                        }
                    }
                }
            } else if (d == 8) {
                    /* Case 3: 8 bpp with cmap and associated transparency */
                L_INFO("converting 8 bpp cmap with alpha ==> RGBA\n", procName);
                for (i = 0; i < h; i++) {
                    ppixel = data + i * wpl;
                    rowptr = row_pointers[i];
                    for (j = 0; j < w; j++) {
                        index = rowptr[j];
                        pixcmapGetColor(cmap, index, &rval, &gval, &bval);
                        composeRGBPixel(rval, gval, bval, ppixel);
                            /* Assume missing entries to be 255 (opaque)
                             * according to the spec:
                             * http://www.w3.org/TR/PNG/#11tRNS */
                        SET_DATA_BYTE(ppixel, L_ALPHA_CHANNEL,
                                      index < num_trans ? trans[index] : 255);
                        ppixel++;
                    }
                }
            } else {
                L_ERROR("spp == 1, cmap, trans array, invalid depth: %d\n",
                        procName, d);
            }
            pixcmapDestroy(&cmap);
        }
    }

#if  DEBUG_READ
    if (cmap) {
        for (i = 0; i < 16; i++) {
            fprintf(stderr, "[%d] = %d\n", i,
                   ((l_uint8 *)(cmap->array))[i]);
        }
    }
#endif  /* DEBUG_READ */

        /* If there is no colormap, PNG defines black = 0 and
         * white = 1 by default for binary monochrome.  Therefore,
         * since we use the opposite definition, we must invert
         * the image colors in either of these cases:
         *    (i) there is no colormap (default)
         *    (ii) there is a colormap which defines black to
         *         be 0 and white to be 1.
         * We cannot use the PNG_TRANSFORM_INVERT_MONO flag
         * because that flag (since version 1.0.9) inverts 8 bpp
         * grayscale as well, which we don't want to do.
         * (It also doesn't work if there is a colormap.)
         * If there is a colormap that defines black = 1 and
         * white = 0, we don't need to do anything.
         *
         * How do we check the polarity of the colormap?
         * The RGBA colormap determines the values of black and
         * white pixels in the following way:
         *     if black = 1 (255), white = 0
         *          255, 255, 255, 0, 0, 0, 0, 0
         *     if black = 0, white = 1 (255)
         *          0, 0, 0, 0, 255, 255, 255, 0
         * So we test the first byte to see if it is 0;
         * if so, invert the colors.
         *
         * If there is a colormap, after inverting the pixels it is
         * necessary to destroy the colormap.  Otherwise, calling
         * pixRemoveColormap() would invert the pixel values again!
         *
         * Note that if the input png is a 1-bit with colormap and
         * transparency, we are rendering it as a 32 bpp, spp = 4 rgba pix.
         * Do not invert the image data!
         */
    d = pixGetDepth(pix);
    if (d == 1 && (!cmap || (cmap && ((l_uint8 *)(cmap->array))[0] == 0x0))) {
/*        fprintf(stderr, "Inverting binary data on png read\n"); */
        pixInvert(pix, pix);
        pixDestroyColormap(pix);
    }

    xres = png_get_x_pixels_per_meter(png_ptr, info_ptr);
    yres = png_get_y_pixels_per_meter(png_ptr, info_ptr);
    pixSetXRes(pix, (l_int32)((l_float32)xres / 39.37 + 0.5));  /* to ppi */
    pixSetYRes(pix, (l_int32)((l_float32)yres / 39.37 + 0.5));  /* to ppi */

        /* Get the text if there is any */
    png_get_text(png_ptr, info_ptr, &text_ptr, &num_text);
    if (num_text && text_ptr)
        pixSetText(pix, text_ptr->text);

    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    return pix;
}


/*!
 *  readHeaderPng()
 *
 *      Input:  filename
 *              &w (<optional return>)
 *              &h (<optional return>)
 *              &bps (<optional return>, bits/sample)
 *              &spp (<optional return>, samples/pixel)
 *              &iscmap (<optional return>)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If there is a colormap, iscmap is returned as 1; else 0.
 *      (2) For gray+alpha, although the png records bps = 16, we
 *          consider this as two 8 bpp samples (gray and alpha).
 *          When a gray+alpha is read, it is converted to 32 bpp RGBA.
 */
l_int32
readHeaderPng(const char *filename,
              l_int32    *pw,
              l_int32    *ph,
              l_int32    *pbps,
              l_int32    *pspp,
              l_int32    *piscmap)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("readHeaderPng");

    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (pbps) *pbps = 0;
    if (pspp) *pspp = 0;
    if (piscmap) *piscmap = 0;
    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if ((fp = fopenReadStream(filename)) == NULL)
        return ERROR_INT("image file not found", procName, 1);
    ret = freadHeaderPng(fp, pw, ph, pbps, pspp, piscmap);
    fclose(fp);
    return ret;
}


/*!
 *  freadHeaderPng()
 *
 *      Input:  stream
 *              &w (<optional return>)
 *              &h (<optional return>)
 *              &bps (<optional return>, bits/sample)
 *              &spp (<optional return>, samples/pixel)
 *              &iscmap (<optional return>)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) See readHeaderPng().  We only need the first 40 bytes in the file.
 */
l_int32
freadHeaderPng(FILE     *fp,
               l_int32  *pw,
               l_int32  *ph,
               l_int32  *pbps,
               l_int32  *pspp,
               l_int32  *piscmap)
{
l_int32   nbytes, ret;
l_uint8  *data;

    PROCNAME("freadHeaderPng");

    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (pbps) *pbps = 0;
    if (pspp) *pspp = 0;
    if (piscmap) *piscmap = 0;
    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);

    nbytes = fnbytesInFile(fp);
    if (nbytes < 40)
        return ERROR_INT("file too small to be png", procName, 1);
    if ((data = (l_uint8 *)CALLOC(40, sizeof(l_uint8))) == NULL)
        return ERROR_INT("CALLOC fail for data", procName, 1);
    if (fread(data, 1, 40, fp) != 40)
        return ERROR_INT("error reading data", procName, 1);
    ret = readHeaderMemPng(data, 40, pw, ph, pbps, pspp, piscmap);
    FREE(data);
    return ret;
}


/*!
 *  readHeaderMemPng()
 *
 *      Input:  data
 *              size (40 bytes is sufficient)
 *              &w (<optional return>)
 *              &h (<optional return>)
 *              &bps (<optional return>, bits/sample)
 *              &spp (<optional return>, samples/pixel)
 *              &iscmap (<optional return>; input NULL to ignore)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) See readHeaderPng().
 *      (2) png colortypes (see png.h: PNG_COLOR_TYPE_*):
 *          0:  gray; fully transparent (with tRNS) (1 spp)
 *          2:  RGB (3 spp)
 *          3:  colormap; colormap+alpha (with tRNS) (1 spp)
 *          4:  gray + alpha (2 spp)
 *          6:  RGBA (4 spp)
 *          Note:
 *            0 and 3 have the alpha information in a tRNS chunk
 *            4 and 6 have separate alpha samples with each pixel.
 */
l_int32
readHeaderMemPng(const l_uint8  *data,
                 size_t          size,
                 l_int32        *pw,
                 l_int32        *ph,
                 l_int32        *pbps,
                 l_int32        *pspp,
                 l_int32        *piscmap)
{
l_uint16   twobytes;
l_uint16  *pshort;
l_int32    colortype, bps, spp;
l_uint32  *pword;

    PROCNAME("readHeaderMemPng");

    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (pbps) *pbps = 0;
    if (pspp) *pspp = 0;
    if (piscmap) *piscmap = 0;
    if (!data)
        return ERROR_INT("data not defined", procName, 1);
    if (size < 40)
        return ERROR_INT("size < 40", procName, 1);

        /* Check password */
    if (data[0] != 137 || data[1] != 80 || data[2] != 78 ||
        data[3] != 71 || data[4] != 13 || data[5] != 10 ||
        data[6] != 26 || data[7] != 10)
        return ERROR_INT("not a valid png file", procName, 1);

    pword = (l_uint32 *)data;
    pshort = (l_uint16 *)data;
    if (pw) *pw = convertOnLittleEnd32(pword[4]);
    if (ph) *ph = convertOnLittleEnd32(pword[5]);
    twobytes = convertOnLittleEnd16(pshort[12]); /* contains depth/sample  */
                                                 /* and the color type     */
    colortype = twobytes & 0xff;  /* color type */
    bps = twobytes >> 8;   /* bits/sample */

        /* Special case with alpha that is extracted as RGBA.
         * Note that the cmap+alpha is also extracted as RGBA,
         * but only if the tRNS chunk exists, which we can't tell
         * by this simple parser.*/
    if (colortype == 4)
        L_INFO("gray + alpha: will extract as RGBA (spp = 4)\n", procName);

    if (colortype == 2) {  /* RGB */
        spp = 3;
    } else if (colortype == 6) {  /* RGBA */
        spp = 4;
    } else if (colortype == 4) {  /* gray + alpha */
        spp = 2;
        bps = 8;  /* both the gray and alpha are 8-bit samples */
    } else {  /* gray (0) or cmap (3) or cmap+alpha (3) */
        spp = 1;
    }
    if (pbps) *pbps = bps;
    if (pspp) *pspp = spp;
    if (piscmap) {
        if (colortype & 1)  /* palette */
            *piscmap = 1;
        else
            *piscmap = 0;
    }

    return 0;
}


/*
 *  fgetPngResolution()
 *
 *      Input:  stream (opened for read)
 *              &xres, &yres (<return> resolution in ppi)
 *      Return: 0 if OK; 0 on error
 *
 *  Notes:
 *      (1) If neither resolution field is set, this is not an error;
 *          the returned resolution values are 0 (designating 'unknown').
 *      (2) Side-effect: this rewinds the stream.
 */
l_int32
fgetPngResolution(FILE     *fp,
                  l_int32  *pxres,
                  l_int32  *pyres)
{
png_uint_32  xres, yres;
png_structp  png_ptr;
png_infop    info_ptr;

    PROCNAME("fgetPngResolution");

    if (pxres) *pxres = 0;
    if (pyres) *pyres = 0;
    if (!fp)
        return ERROR_INT("stream not opened", procName, 1);
    if (!pxres || !pyres)
        return ERROR_INT("&xres and &yres not both defined", procName, 1);

       /* Make the two required structs */
    if ((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                   (png_voidp)NULL, NULL, NULL)) == NULL)
        return ERROR_INT("png_ptr not made", procName, 1);
    if ((info_ptr = png_create_info_struct(png_ptr)) == NULL) {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        return ERROR_INT("info_ptr not made", procName, 1);
    }

        /* Set up png setjmp error handling.
         * Without this, an error calls exit. */
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        return ERROR_INT("internal png error", procName, 1);
    }

        /* Read the metadata */
    rewind(fp);
    png_init_io(png_ptr, fp);
    png_read_png(png_ptr, info_ptr, 0, NULL);

    xres = png_get_x_pixels_per_meter(png_ptr, info_ptr);
    yres = png_get_y_pixels_per_meter(png_ptr, info_ptr);
    *pxres = (l_int32)((l_float32)xres / 39.37 + 0.5);  /* to ppi */
    *pyres = (l_int32)((l_float32)yres / 39.37 + 0.5);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    rewind(fp);
    return 0;
}


/*!
 *  isPngInterlaced()
 *
 *      Input:  filename
 *              &interlaced (<return> 1 if interlaced png; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 */
l_int32
isPngInterlaced(const char *filename,
                l_int32    *pinterlaced)
{
l_uint8  buf[32];
FILE    *fp;

    PROCNAME("isPngInterlaced");

    if (!pinterlaced)
        return ERROR_INT("&interlaced not defined", procName, 1);
    *pinterlaced = 0;
    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);

    if ((fp = fopenReadStream(filename)) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (fread(buf, 1, 32, fp) != 32)
        return ERROR_INT("data not read", procName, 1);
    fclose(fp);

    *pinterlaced = (buf[28] == 0) ? 0 : 1;
    return 0;
}


/*---------------------------------------------------------------------*
 *                              Writing png                            *
 *---------------------------------------------------------------------*/
/*!
 *  pixWritePng()
 *
 *      Input:  filename
 *              pix
 *              gamma
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Special version for writing png with a specified gamma.
 *          When using pixWrite(), no field is given for gamma.
 */
l_int32
pixWritePng(const char  *filename,
            PIX         *pix,
            l_float32    gamma)
{
FILE  *fp;

    PROCNAME("pixWritePng");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "wb+")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);

    if (pixWriteStreamPng(fp, pix, gamma)) {
        fclose(fp);
        return ERROR_INT("pix not written to stream", procName, 1);
    }

    fclose(fp);
    return 0;
}


/*!
 *  pixWriteStreamPng()
 *
 *      Input:  stream
 *              pix
 *              gamma (use 0.0 if gamma is not defined)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) If called from pixWriteStream(), the stream is positioned
 *          at the beginning of the file.
 *      (2) To do sequential writes of png format images to a stream,
 *          use pixWriteStreamPng() directly.
 *      (3) gamma is an optional png chunk.  If no gamma value is to be
 *          placed into the file, use gamma = 0.0.  Otherwise, if
 *          gamma > 0.0, its value is written into the header.
 *      (4) The use of gamma in png is highly problematic.  For an illuminating
 *          discussion, see:  http://hsivonen.iki.fi/png-gamma/
 *      (5) What is the effect/meaning of gamma in the png file?  This
 *          gamma, which we can call the 'source' gamma, is the
 *          inverse of the gamma that was used in enhance.c to brighten
 *          or darken images.  The 'source' gamma is supposed to indicate
 *          the intensity mapping that was done at the time the
 *          image was captured.  Display programs typically apply a
 *          'display' gamma of 2.2 to the output, which is intended
 *          to linearize the intensity based on the response of
 *          thermionic tubes (CRTs).  Flat panel LCDs have typically
 *          been designed to give a similar response as CRTs (call it
 *          "backward compatibility").  The 'display' gamma is
 *          in some sense the inverse of the 'source' gamma.
 *          jpeg encoders attached to scanners and cameras will lighten
 *          the pixels, applying a gamma corresponding to approximately
 *          a square-root relation of output vs input:
 *                output = input^(gamma)
 *          where gamma is often set near 0.4545  (1/gamma is 2.2).
 *          This is stored in the image file.  Then if the display
 *          program reads the gamma, it will apply a display gamma,
 *          typically about 2.2; the product is 1.0, and the
 *          display program produces a linear output.  This works because
 *          the dark colors were appropriately boosted by the scanner,
 *          as described by the 'source' gamma, so they should not
 *          be further boosted by the display program.
 *      (6) As an example, with xv and display, if no gamma is stored,
 *          the program acts as if gamma were 0.4545, multiplies this by 2.2,
 *          and does a linear rendering.  Taking this as a baseline
 *          brightness, if the stored gamma is:
 *              > 0.4545, the image is rendered lighter than baseline
 *              < 0.4545, the image is rendered darker than baseline
 *          In contrast, gqview seems to ignore the gamma chunk in png.
 *      (7) The only valid pixel depths in leptonica are 1, 2, 4, 8, 16
 *          and 32.  However, it is possible, and in some cases desirable,
 *          to write out a png file using an rgb pix that has 24 bpp.
 *          For example, the open source xpdf SplashBitmap class generates
 *          24 bpp rgb images.  Consequently, we enable writing 24 bpp pix.
 *          To generate such a pix, you can make a 24 bpp pix without data
 *          and assign the data array to the pix; e.g.,
 *              pix = pixCreateHeader(w, h, 24);
 *              pixSetData(pix, rgbdata);
 *          See pixConvert32To24() for an example, where we get rgbdata
 *          from the 32 bpp pix.  Caution: do not call pixSetPadBits(),
 *          because the alignment is wrong and you may erase part of the
 *          last pixel on each line.
 *      (8) If the pix has a colormap, it is written to file.  In most
 *          situations, the alpha component is 255 for each colormap entry,
 *          which is opaque and indicates that it should be ignored.
 *          However, if any alpha component is not 255, it is assumed that
 *          the alpha values are valid, and they are written to the png
 *          file in a tRNS segment.  On readback, the tRNS segment is
 *          identified, and the colormapped image with alpha is converted
 *          to a 4 spp rgba image.
 */
l_int32
pixWriteStreamPng(FILE      *fp,
                  PIX       *pix,
                  l_float32  gamma)
{
char         commentstring[] = "Comment";
l_int32      i, j, k;
l_int32      wpl, d, spp, cmflag, opaque;
l_int32      ncolors, compval;
l_int32     *rmap, *gmap, *bmap, *amap;
l_uint32    *data, *ppixel;
png_byte     bit_depth, color_type;
png_byte     alpha[256];
png_uint_32  w, h;
png_uint_32  xres, yres;
png_bytep   *row_pointers;
png_bytep    rowbuffer;
png_structp  png_ptr;
png_infop    info_ptr;
png_colorp   palette;
PIX         *pixt;
PIXCMAP     *cmap;
char        *text;

    PROCNAME("pixWriteStreamPng");

    if (!fp)
        return ERROR_INT("stream not open", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

        /* Allocate the 2 data structures */
    if ((png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                   (png_voidp)NULL, NULL, NULL)) == NULL)
        return ERROR_INT("png_ptr not made", procName, 1);

    if ((info_ptr = png_create_info_struct(png_ptr)) == NULL) {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        return ERROR_INT("info_ptr not made", procName, 1);
    }

        /* Set up png setjmp error handling */
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return ERROR_INT("internal png error", procName, 1);
    }

    png_init_io(png_ptr, fp);

        /* With best zlib compression (9), get between 1 and 10% improvement
         * over default (6), but the compression is 3 to 10 times slower.
         * Use the zlib default (6) as our default compression unless
         * pix->special falls in the range [10 ... 19]; then subtract 10
         * to get the compression value.  */
    compval = Z_DEFAULT_COMPRESSION;
    if (pix->special >= 10 && pix->special < 20)
        compval = pix->special - 10;
    png_set_compression_level(png_ptr, compval);

    w = pixGetWidth(pix);
    h = pixGetHeight(pix);
    d = pixGetDepth(pix);
    spp = pixGetSpp(pix);
    if ((cmap = pixGetColormap(pix)))
        cmflag = 1;
    else
        cmflag = 0;

        /* Set the color type and bit depth. */
    if (d == 32 && spp == 4) {
        bit_depth = 8;
        color_type = PNG_COLOR_TYPE_RGBA;   /* 6 */
        cmflag = 0;  /* ignore if it exists */
    } else if (d == 24 || d == 32) {
        bit_depth = 8;
        color_type = PNG_COLOR_TYPE_RGB;   /* 2 */
        cmflag = 0;  /* ignore if it exists */
    } else {
        bit_depth = d;
        color_type = PNG_COLOR_TYPE_GRAY;  /* 0 */
    }
    if (cmflag)
        color_type = PNG_COLOR_TYPE_PALETTE;  /* 3 */

#if  DEBUG_WRITE
    fprintf(stderr, "cmflag = %d, bit_depth = %d, color_type = %d\n",
            cmflag, bit_depth, color_type);
#endif  /* DEBUG_WRITE */

    png_set_IHDR(png_ptr, info_ptr, w, h, bit_depth, color_type,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);

        /* Store resolution in ppm, if known */
    xres = (png_uint_32)(39.37 * (l_float32)pixGetXRes(pix) + 0.5);
    yres = (png_uint_32)(39.37 * (l_float32)pixGetYRes(pix) + 0.5);
    if ((xres == 0) || (yres == 0))
        png_set_pHYs(png_ptr, info_ptr, 0, 0, PNG_RESOLUTION_UNKNOWN);
    else
        png_set_pHYs(png_ptr, info_ptr, xres, yres, PNG_RESOLUTION_METER);

    if (cmflag) {
        pixcmapToArrays(cmap, &rmap, &gmap, &bmap, &amap);
        ncolors = pixcmapGetCount(cmap);
        pixcmapIsOpaque(cmap, &opaque);

            /* Make and save the palette */
        if ((palette = (png_colorp)(CALLOC(ncolors, sizeof(png_color))))
                == NULL)
            return ERROR_INT("palette not made", procName, 1);

        for (i = 0; i < ncolors; i++) {
            palette[i].red = (png_byte)rmap[i];
            palette[i].green = (png_byte)gmap[i];
            palette[i].blue = (png_byte)bmap[i];
            alpha[i] = (png_byte)amap[i];
        }

        png_set_PLTE(png_ptr, info_ptr, palette, (int)ncolors);
        if (!opaque)  /* alpha channel has some transparency; assume valid */
            png_set_tRNS(png_ptr, info_ptr, (png_bytep)alpha,
                         (int)ncolors, NULL);
        FREE(rmap);
        FREE(gmap);
        FREE(bmap);
        FREE(amap);
    }

        /* 0.4545 is treated as the default by some image
         * display programs (not gqview).  A value > 0.4545 will
         * lighten an image as displayed by xv, display, etc. */
    if (gamma > 0.0)
        png_set_gAMA(png_ptr, info_ptr, (l_float64)gamma);

    if ((text = pixGetText(pix))) {
        png_text text_chunk;
        text_chunk.compression = PNG_TEXT_COMPRESSION_NONE;
        text_chunk.key = commentstring;
        text_chunk.text = text;
        text_chunk.text_length = strlen(text);
#ifdef PNG_ITXT_SUPPORTED
        text_chunk.itxt_length = 0;
        text_chunk.lang = NULL;
        text_chunk.lang_key = NULL;
#endif
        png_set_text(png_ptr, info_ptr, &text_chunk, 1);
    }

        /* Write header and palette info */
    png_write_info(png_ptr, info_ptr);

    if ((d != 32) && (d != 24)) {  /* not rgb color */
            /* Generate a temporary pix with bytes swapped.
             * For a binary image, there are two conditions in
             * which you must first invert the data for writing png:
             *    (a) no colormap
             *    (b) colormap with BLACK set to 0
             * png writes binary with BLACK = 0, unless contradicted
             * by a colormap.  If the colormap has BLACK = "1"
             * (typ. about 255), do not invert the data.  If there
             * is no colormap, you must invert the data to store
             * in default BLACK = 0 state.  */
        if (d == 1 &&
            (!cmap || (cmap && ((l_uint8 *)(cmap->array))[0] == 0x0))) {
            pixt = pixInvert(NULL, pix);
            pixEndianByteSwap(pixt);
        } else {
            pixt = pixEndianByteSwapNew(pix);
        }
        if (!pixt) {
            png_destroy_write_struct(&png_ptr, &info_ptr);
            return ERROR_INT("pixt not made", procName, 1);
        }

            /* Make and assign array of image row pointers */
        if ((row_pointers = (png_bytep *)CALLOC(h, sizeof(png_bytep))) == NULL)
            return ERROR_INT("row-pointers not made", procName, 1);
        wpl = pixGetWpl(pixt);
        data = pixGetData(pixt);
        for (i = 0; i < h; i++)
            row_pointers[i] = (png_bytep)(data + i * wpl);
        png_set_rows(png_ptr, info_ptr, row_pointers);

            /* Transfer the data */
        png_write_image(png_ptr, row_pointers);
        png_write_end(png_ptr, info_ptr);

        if (cmflag)
            FREE(palette);
        FREE(row_pointers);
        pixDestroy(&pixt);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return 0;
    }

        /* For rgb, compose and write a row at a time */
    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    if (d == 24) {  /* See note 7 above: special case of 24 bpp rgb */
        for (i = 0; i < h; i++) {
            ppixel = data + i * wpl;
            png_write_rows(png_ptr, (png_bytepp)&ppixel, 1);
        }
    } else {  /* 32 bpp rgb and rgba.  Write out the alpha channel if either
             * the pix has 4 spp or writing it is requested anyway */
        if ((rowbuffer = (png_bytep)CALLOC(w, 4)) == NULL)
            return ERROR_INT("rowbuffer not made", procName, 1);
        for (i = 0; i < h; i++) {
            ppixel = data + i * wpl;
            for (j = k = 0; j < w; j++) {
                rowbuffer[k++] = GET_DATA_BYTE(ppixel, COLOR_RED);
                rowbuffer[k++] = GET_DATA_BYTE(ppixel, COLOR_GREEN);
                rowbuffer[k++] = GET_DATA_BYTE(ppixel, COLOR_BLUE);
                if (spp == 4)
                    rowbuffer[k++] = GET_DATA_BYTE(ppixel, L_ALPHA_CHANNEL);
                ppixel++;
            }

            png_write_rows(png_ptr, &rowbuffer, 1);
        }
        FREE(rowbuffer);
    }

    png_write_end(png_ptr, info_ptr);

    if (cmflag)
        FREE(palette);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return 0;

}


/*!
 *  pixSetZlibCompression()
 *
 *      Input:  pix
 *              compval (zlib compression value)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Valid zlib compression values are in the interval [0 ... 9],
 *          where, as defined in zlib.h:
 *            0         Z_NO_COMPRESSION
 *            1         Z_BEST_SPEED    (poorest compression)
 *            9         Z_BEST_COMPRESSION
 *          For the default value, use either of these:
 *            6         Z_DEFAULT_COMPRESSION
 *           -1         (resolves to Z_DEFAULT_COMPRESSION)
 *      (2) If you use the defined constants in zlib.h instead of the
 *          compression integers given above, you must include zlib.h.
 */
l_int32
pixSetZlibCompression(PIX     *pix,
                      l_int32  compval)
{
    PROCNAME("pixSetZlibCompression");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (compval < 0 || compval > 9) {
        L_ERROR("Invalid zlib comp val; using default\n", procName);
        compval = Z_DEFAULT_COMPRESSION;
    }
    pix->special = 10 + compval;  /* valid range [10 ... 19] */
    return 0;
}


/*---------------------------------------------------------------------*
 *             Setting flag for stripping 16 bits on reading           *
 *---------------------------------------------------------------------*/
/*!
 *  l_pngSetReadStrip16To8()
 *
 *      Input:  flag (1 for stripping 16 bpp to 8 bpp on reading;
 *                    0 for leaving 16 bpp)
 *      Return: void
 */
void
l_pngSetReadStrip16To8(l_int32  flag)
{
    var_PNG_STRIP_16_TO_8 = flag;
}


/*---------------------------------------------------------------------*
 *                         Read/write to memory                        *
 *---------------------------------------------------------------------*/
#if HAVE_FMEMOPEN
extern FILE *open_memstream(char **data, size_t *size);
extern FILE *fmemopen(void *data, size_t size, const char *mode);
#endif  /* HAVE_FMEMOPEN */

/*!
 *  pixReadMemPng()
 *
 *      Input:  cdata (const; png-encoded)
 *              size (of data)
 *      Return: pix, or null on error
 *
 *  Notes:
 *      (1) The @size byte of @data must be a null character.
 */
PIX *
pixReadMemPng(const l_uint8  *cdata,
              size_t          size)
{
FILE  *fp;
PIX   *pix;

    PROCNAME("pixReadMemPng");

    if (!cdata)
        return (PIX *)ERROR_PTR("cdata not defined", procName, NULL);

#if HAVE_FMEMOPEN
    if ((fp = fmemopen((void *)cdata, size, "r")) == NULL)
        return (PIX *)ERROR_PTR("stream not opened", procName, NULL);
#else
    L_WARNING("work-around: writing to a temp file\n", procName);
    fp = tmpfile();
    fwrite(cdata, 1, size, fp);
    rewind(fp);
#endif  /* HAVE_FMEMOPEN */
    pix = pixReadStreamPng(fp);
    fclose(fp);
    if (!pix) L_ERROR("pix not read\n", procName);
    return pix;
}


/*!
 *  pixWriteMemPng()
 *
 *      Input:  &data (<return> data of tiff compressed image)
 *              &size (<return> size of returned data)
 *              pix
 *              gamma (use 0.0 if gamma is not defined)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) See pixWriteStreamPng() for usage.  This version writes to
 *          memory instead of to a file stream.
 */
l_int32
pixWriteMemPng(l_uint8  **pdata,
               size_t    *psize,
               PIX       *pix,
               l_float32  gamma)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("pixWriteMemPng");

    if (pdata) *pdata = NULL;
    if (psize) *psize = 0;
    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1 );
    if (!psize)
        return ERROR_INT("&size not defined", procName, 1 );
    if (!pix)
        return ERROR_INT("&pix not defined", procName, 1 );

#if HAVE_FMEMOPEN
    if ((fp = open_memstream((char **)pdata, psize)) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    ret = pixWriteStreamPng(fp, pix, gamma);
#else
    L_WARNING("work-around: writing to a temp file\n", procName);
    fp = tmpfile();
    ret = pixWriteStreamPng(fp, pix, gamma);
    rewind(fp);
    *pdata = l_binaryReadStream(fp, psize);
#endif  /* HAVE_FMEMOPEN */
    fclose(fp);
    return ret;
}

/* --------------------------------------------*/
#endif  /* HAVE_LIBPNG */
/* --------------------------------------------*/
