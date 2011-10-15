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
 *  pngio.c
 *                     
 *    Read png from file
 *          PIX        *pixReadStreamPng()
 *          l_int32     readHeaderPng()
 *          l_int32     freadHeaderPng()
 *          l_int32     sreadHeaderPng()
 *
 *    Write png to file
 *          l_int32     pixWritePng()  [ special top level ]
 *          l_int32     pixWriteStreamPng()
 *          
 *    Read and write of png to/from RGBA pix
 *          PIX        *pixReadRGBAPng();
 *          l_int32     pixWriteRGBAPng();
 *
 *    Setting flags for special modes
 *          void        l_pngSetStrip16To8()
 *          void        l_pngSetStripAlpha()
 *          void        l_pngSetWriteAlpha()
 *          void        l_pngSetZlibCompression()
 *
 *    Read/write to memory   [not on windows]
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
 *    There are three special flags for determining the number or
 *    size of samples retained or written:
 *    (1) var_PNG_STRIP_16_to_8: default is TRUE.  This strips each
 *        16 bit sample down to 8 bps:
 *         - For 16 bps rgb (16 bps, 3 spp) --> 32 bpp rgb Pix
 *         - For 16 bps gray (16 bps, 1 spp) --> 8 bpp grayscale Pix
 *    (2) var_PNG_STRIP_ALPHA: default is TRUE.  This does not copy
 *        the alpha channel to the pix:
 *         - For 8 bps rgba (8 bps, 4 spp) --> 32 bpp rgb Pix
 *    (3) var_PNG_WRITE_ALPHA: default is FALSE.  The default generates
 *        an RGB png file with 3 spp.  If set to TRUE, this generates
 *        an RGBA png file with 4 spp, and writes the alpha channel.
 *    These are set with accessors.
 *
 *    Two convenience functions are included for reading the alpha
 *    channel (if it exists) into the pix, and for writing out the
 *    alpha sample of a pix to a png file:
 *        pixReadRGBAPng()
 *        pixWriteRGBAPng()
 *    These use two of the special flags, setting to the non-default
 *    value before use and resetting to default afterwards.
 *    In leptonica, we make almost no explicit use of the alpha channel.
 *        
 *    Another special flag, var_ZLIB_COMPRESSION, is used to determine
 *    the compression level.  Default is for standard png compression.
 *    The zlib compression value can be set [0 ... 9], with
 *         0     no compression (huge files)
 *         1     fastest compression
 *         -1    default compression  (equivalent to 6 in latest version)
 *         9     best compression
 *    If not set, we use the default compression in zlib.
 *    Note that if you are using the defined constants in zlib instead
 *    of the compression integers given above, you must include zlib.h.
 *
 *    Note: All special flags use global constants, so if used with
 *          multi-threaded applications, results can be non-deterministic.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

/* --------------------------------------------*/
#if  HAVE_LIBPNG   /* defined in environ.h */
/* --------------------------------------------*/

#include "png.h"

/* ----------------Set defaults for read/write options ----------------- */
    /* strip 16 bpp --> 8 bpp on reading png; default is for stripping */
static l_int32   var_PNG_STRIP_16_TO_8 = 1;
    /* strip alpha on reading png; default is for stripping */
static l_int32   var_PNG_STRIP_ALPHA = 1;
    /* write alpha for 32 bpp images; default is to write only RGB */
static l_int32   var_PNG_WRITE_ALPHA = 0;
    /* zlib compression in png; default is for standard compression */
static l_int32   var_ZLIB_COMPRESSION = Z_DEFAULT_COMPRESSION;


#ifndef  NO_CONSOLE_IO
#define  DEBUG     0
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
 */
PIX *
pixReadStreamPng(FILE  *fp)
{
l_uint8      rval, gval, bval;
l_int32      i, j, k;
l_int32      wpl, d, spp, cindex;
l_uint32     png_transforms;
l_uint32    *data, *line, *ppixel;
int          num_palette, num_text;
png_byte     bit_depth, color_type, channels;
png_uint_32  w, h, rowbytes;
png_uint_32  xres, yres;
png_bytep    rowptr;
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
         * ---------------------------------------------------------- */
        /* To strip 16 --> 8 bit depth, use PNG_TRANSFORM_STRIP_16 */
    if (var_PNG_STRIP_16_TO_8 == 1)   /* our default */
        png_transforms = PNG_TRANSFORM_STRIP_16;
    else
        png_transforms = PNG_TRANSFORM_IDENTITY;
        /* To remove alpha channel, use PNG_TRANSFORM_STRIP_ALPHA */
    if (var_PNG_STRIP_ALPHA == 1)   /* our default */
        png_transforms |= PNG_TRANSFORM_STRIP_ALPHA;

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

    if (spp == 1)
        d = bit_depth;
    else if (spp == 2) {
        d = 2 * bit_depth;
        L_WARNING("there shouldn't be 2 spp!", procName);
    }
    else  /* spp == 3 (rgb), spp == 4 (rgba) */
        d = 4 * bit_depth;

        /* Remove if/when this is implemented for all bit_depths */
    if (spp == 3 && bit_depth != 8) {
        fprintf(stderr, "Help: spp = 3 and depth = %d != 8\n!!", bit_depth);
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
    }
    else
        cmap = NULL;

    if ((pix = pixCreate(w, h, d)) == NULL) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        return (PIX *)ERROR_PTR("pix not made", procName, NULL);
    }
    wpl = pixGetWpl(pix);
    data = pixGetData(pix);
    pixSetColormap(pix, cmap);

    if (spp == 1) {   /* copy straight from buffer to pix */
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            rowptr = row_pointers[i];
            for (j = 0; j < rowbytes; j++) {
                SET_DATA_BYTE(line, j, rowptr[j]);
            }
        }
    }
    else  {   /* spp == 3 or spp == 4 */
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

#if  DEBUG
    if (cmap) {
        for (i = 0; i < 16; i++) {
            fprintf(stderr, "[%d] = %d\n", i,
                   ((l_uint8 *)(cmap->array))[i]);
        }
    }
#endif  /* DEBUG */

        /* If there is no colormap, PNG defines black = 0 and
         * white = 1 by default for binary monochrome.  Therefore,
         * since we use the opposite definition, we must invert
         * the image in either of these cases:
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
         * The colormap determines the values of black and
         * white pixels in the following way:
         *     if black = 1 (255), white = 0
         *          255, 255, 255, 0, 0, 0, 0, 0, 0
         *     if black = 0, white = 1 (255)
         *          0, 0, 0, 0, 255, 255, 255, 0
         * So we test the first byte to see if it is 0;
         * if so, invert the data.  */
    if (d == 1 && (!cmap || (cmap && ((l_uint8 *)(cmap->array))[0] == 0x0))) {
/*        fprintf(stderr, "Inverting binary data on png read\n"); */
        pixInvert(pix, pix);
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
 *              &width (<return>)
 *              &height (<return>)
 *              &bps (<return>, bits/sample)
 *              &spp (<return>, samples/pixel)
 *              &iscmap (<optional return>; input NULL to ignore)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If there is a colormap, iscmap is returned as 1; else 0.
 */
l_int32
readHeaderPng(const char *filename,
              l_int32    *pwidth,
              l_int32    *pheight,
              l_int32    *pbps,
              l_int32    *pspp,
              l_int32    *piscmap)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("readHeaderPng");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!pwidth || !pheight || !pbps || !pspp)
        return ERROR_INT("input ptr(s) not defined", procName, 1);
    if ((fp = fopenReadStream(filename)) == NULL)
        return ERROR_INT("image file not found", procName, 1);
    ret = freadHeaderPng(fp, pwidth, pheight, pbps, pspp, piscmap);
    fclose(fp);
    return ret;
}


/*!
 *  freadHeaderPng()
 *
 *      Input:  stream
 *              &width (<return>)
 *              &height (<return>)
 *              &bps (<return>, bits/sample)
 *              &spp (<return>, samples/pixel)
 *              &iscmap (<optional return>; input NULL to ignore)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If there is a colormap, iscmap is returned as 1; else 0.
 */
l_int32
freadHeaderPng(FILE     *fp,
               l_int32  *pwidth,
               l_int32  *pheight,
               l_int32  *pbps,
               l_int32  *pspp,
               l_int32  *piscmap)
{
l_int32   nbytes, ret;
l_uint8  *data;

    PROCNAME("freadHeaderPng");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!pwidth || !pheight || !pbps || !pspp)
        return ERROR_INT("input ptr(s) not defined", procName, 1);
    
    nbytes = fnbytesInFile(fp);
    if (nbytes < 40)
        return ERROR_INT("file too small to be png", procName, 1);
    if ((data = (l_uint8 *)CALLOC(40, sizeof(l_uint8))) == NULL)
        return ERROR_INT("CALLOC fail for data", procName, 1);
    fread(data, 40, 1, fp);
    ret = sreadHeaderPng(data, pwidth, pheight, pbps, pspp, piscmap);
    FREE(data);
    return ret;
}


/*!
 *  sreadHeaderPng()
 *
 *      Input:  data
 *              &width (<return>)
 *              &height (<return>)
 *              &bps (<return>, bits/sample)
 *              &spp (<return>, samples/pixel)
 *              &iscmap (<optional return>; input NULL to ignore)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If there is a colormap, iscmap is returned as 1; else 0.
 */
l_int32
sreadHeaderPng(const l_uint8  *data,
               l_int32        *pwidth,
               l_int32        *pheight,
               l_int32        *pbps,
               l_int32        *pspp,
               l_int32        *piscmap)
{
l_uint8    colortype, bps;
l_uint16   twobytes;
l_uint16  *pshort;
l_uint32  *pword;

    PROCNAME("sreadHeaderPng");

    if (!data)
        return ERROR_INT("data not defined", procName, 1);
    if (!pwidth || !pheight || !pbps || !pspp)
        return ERROR_INT("input ptr(s) not defined", procName, 1);
    *pwidth = *pheight = *pbps = *pspp = 0;
    if (piscmap)
      *piscmap = 0;
    
        /* Check password */
    if (data[0] != 137 || data[1] != 80 || data[2] != 78 ||
        data[3] != 71 || data[4] != 13 || data[5] != 10 ||
        data[6] != 26 || data[7] != 10)
        return ERROR_INT("not a valid png file", procName, 1);

    pword = (l_uint32 *)data;
    pshort = (l_uint16 *)data;
    *pwidth = convertOnLittleEnd32(pword[4]);
    *pheight = convertOnLittleEnd32(pword[5]);
    twobytes = convertOnLittleEnd16(pshort[12]); /* contains depth/sample  */
                                                 /* and the color type     */
    colortype = twobytes & 0xff;  /* color type */
    bps = twobytes >> 8;   /* bits/sample */
    *pbps = bps;
    if (colortype == 2)  /* RGB */
        *pspp = 3;
    else if (colortype == 6)  /* RGBA */
        *pspp = 4;
    else   /* palette or gray */
        *pspp = 1;
    if (piscmap) {
        if (colortype & 1)  /* palette: see png.h, PNG_COLOR_TYPE_... */
            *piscmap = 1;
        else
            *piscmap = 0;
    }

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

    if ((fp = fopen(filename, "wb+")) == NULL)
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
 *          24 bpp rgb images.  Consequently, we anble writing 24 bpp pix.
 *          To generate such a pix, you can make a 24 bpp pix without data
 *          and assign the data array to the pix; e.g.,
 *              pix = pixCreateHeader(w, h, 24);
 *              pixSetData(pix, rgbdata);
 *          See pixConvert32To24() for an example, where we get rgbdata
 *          from the 32 bpp pix.  Caution: do not call pixSetPadBits(),
 *          because the alignment is wrong and you may erase part of the
 *          last pixel on each line.
 */
l_int32
pixWriteStreamPng(FILE      *fp,
                  PIX       *pix,
                  l_float32  gamma)
{
char         commentstring[] = "Comment";
l_int32      i, j, k;
l_int32      wpl, d, cmflag;
l_int32      ncolors;
l_int32     *rmap, *gmap, *bmap;
l_uint32    *data, *ppixel;
png_byte     bit_depth, color_type;
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
         * over default (5), but the compression is 3 to 10 times slower.
         * Our default compression is the zlib default (5). */
    png_set_compression_level(png_ptr, var_ZLIB_COMPRESSION);

    w = pixGetWidth(pix);
    h = pixGetHeight(pix);
    d = pixGetDepth(pix);
    if ((cmap = pixGetColormap(pix)))
        cmflag = 1;
    else
        cmflag = 0;

        /* Set the color type and bit depth. */
    if (d == 32 && var_PNG_WRITE_ALPHA == 1) {
        bit_depth = 8;
        color_type = PNG_COLOR_TYPE_RGBA;   /* 6 */
        cmflag = 0;  /* ignore if it exists */
    }
    else if (d == 24 || d == 32) {
        bit_depth = 8;
        color_type = PNG_COLOR_TYPE_RGB;   /* 2 */
        cmflag = 0;  /* ignore if it exists */
    }
    else {
        bit_depth = d;
        color_type = PNG_COLOR_TYPE_GRAY;  /* 0 */
    }
    if (cmflag)
        color_type = PNG_COLOR_TYPE_PALETTE;  /* 3 */

#if  DEBUG
    fprintf(stderr, "cmflag = %d, bit_depth = %d, color_type = %d\n",
            cmflag, bit_depth, color_type);
#endif  /* DEBUG */

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
        pixcmapToArrays(cmap, &rmap, &gmap, &bmap);
        ncolors = pixcmapGetCount(cmap);

            /* Make and save the palette */
        if ((palette = (png_colorp)(CALLOC(ncolors, sizeof(png_color))))
                == NULL)
            return ERROR_INT("palette not made", procName, 1);

        for (i = 0; i < ncolors; i++) {
            palette[i].red = (png_byte)rmap[i];
            palette[i].green = (png_byte)gmap[i];
            palette[i].blue = (png_byte)bmap[i];
        }

        png_set_PLTE(png_ptr, info_ptr, palette, (int)ncolors);
        FREE(rmap);
        FREE(gmap);
        FREE(bmap);
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
        }
        else 
            pixt = pixEndianByteSwapNew(pix);
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
    }
    else {  /* 32 bpp rgb and rgba */
        if ((rowbuffer = (png_bytep)CALLOC(w, 4)) == NULL)
            return ERROR_INT("rowbuffer not made", procName, 1);
        for (i = 0; i < h; i++) {
            ppixel = data + i * wpl;
            for (j = k = 0; j < w; j++) {
                rowbuffer[k++] = GET_DATA_BYTE(ppixel, COLOR_RED);
                rowbuffer[k++] = GET_DATA_BYTE(ppixel, COLOR_GREEN);
                rowbuffer[k++] = GET_DATA_BYTE(ppixel, COLOR_BLUE);
                if (var_PNG_WRITE_ALPHA == 1)
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


/*---------------------------------------------------------------------*
 *                    Read and write of png to RGBA                    *
 *---------------------------------------------------------------------*/
extern const char *ImageFileFormatExtensions[];

/*!
 *  pixReadRGBAPng()
 *
 *      Input:  filename (of png file)
 *      Return: pix, or null on error
 *
 *  Notes:
 *      (1) Wrapper to keep the alpha channel of a png, if it exists.
 *      (2) The default behavior of pix read functions is to ignore
 *          the alpha channel.
 *      (3) This always leaves alpha stripping in the same mode as
 *          when this function begins.  So if alpha stripping is in
 *          default mode, this disables it, reads the file (including
 *          the alpha channel), and resets back to stripping.  Otherwise,
 *          it leaves stripping disabled.
 */
PIX *
pixReadRGBAPng(const char  *filename)
{
l_int32  format;
FILE    *fp;
PIX     *pix;

    PROCNAME("pixReadRGBAPng");

    if (!filename)
        return (PIX *)ERROR_PTR("filename not defined", procName, NULL);

        /* If alpha channel reading is enabled, just read it */
    if (var_PNG_STRIP_ALPHA == FALSE)
        return pixRead(filename);

        /* Make sure it's a png file */
    if ((fp = fopenReadStream(filename)) == NULL)
        return (PIX *)ERROR_PTR("image file not found", procName, NULL);
    findFileFormat(fp, &format);
    if (format != IFF_PNG) {
        fclose(fp);
        L_ERROR_STRING("file format is %s, not png", procName,
                       ImageFileFormatExtensions[format]);
        return NULL;
    }

    l_pngSetStripAlpha(0);
    pix = pixReadStreamPng(fp);
    l_pngSetStripAlpha(1);  /* reset to default */
    fclose(fp);
    if (!pix) L_ERROR("pix not read", procName);
    return pix;
}


/*!
 *  pixWriteRGBAPng()
 *
 *      Input:  filename
 *              pix (rgba)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Wrapper to write the alpha sample of a 32 bpp pix to
 *          a png file in rgba format.
 *      (2) The default behavior of pix write to png is to ignore
 *          the alpha sample.
 *      (3) This always leaves alpha writing in the same mode as
 *          when this function begins.  So if alpha writing is in
 *          default mode, this enables it, writes out a rgba png file
 *          that includes the alpha channel, and resets to default.
 *          Otherwise, it leaves alpha writing enabled.
 */
l_int32
pixWriteRGBAPng(const char *filename,
                PIX        *pix)
{
l_int32  ret;

    PROCNAME("pixWriteRGBAPng");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);

        /* If alpha channel writing is enabled, just write it */
    if (var_PNG_WRITE_ALPHA == TRUE)
        return pixWrite(filename, pix, IFF_PNG);

    l_pngSetWriteAlpha(1);
    ret = pixWritePng(filename, pix, 0.0);
    l_pngSetWriteAlpha(0);  /* reset to default */
    return ret;
}


/*---------------------------------------------------------------------*
 *                   Setting flags for special modes                   *
 *---------------------------------------------------------------------*/
/*!
 *  l_pngSetStrip16To8()
 *
 *      Input:  flag (1 for stripping 16 bpp to 8 bpp; 0 for leaving 16 bpp)
 *      Return: void
 */
void
l_pngSetStrip16To8(l_int32  flag)
{
    var_PNG_STRIP_16_TO_8 = flag;
}


/*!
 *  l_pngSetStripAlpha()
 *
 *      Input:  flag (1 for stripping alpha channel on read; 0 for leaving it)
 *      Return: void
 */
void
l_pngSetStripAlpha(l_int32  flag)
{
    var_PNG_STRIP_ALPHA = flag;
}


/*!
 *  l_pngSetWriteAlpha()
 *
 *      Input:  flag (1 for writing alpha channel; 0 for just writing rgb)
 *      Return: void
 */
void
l_pngSetWriteAlpha(l_int32  flag)
{
    var_PNG_WRITE_ALPHA = flag;
}


/*!
 *  l_pngSetZlibCompression()
 *
 *      Input:  val (zlib compression value)
 *      Return: void
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
void
l_pngSetZlibCompression(l_int32  val)
{
    PROCNAME("l_pngSetZlibCompression");
    if (val < -1 || val > 9) {
        L_ERROR("Invalid zlib comp val; using default", procName);
        val = Z_DEFAULT_COMPRESSION;
    }
    var_ZLIB_COMPRESSION = val;
}



/*---------------------------------------------------------------------*
 *                         Read/write to memory                        *
 *---------------------------------------------------------------------*/
#if HAVE_FMEMOPEN

extern FILE *open_memstream(char **data, size_t *size);
extern FILE *fmemopen(void *data, size_t size, const char *mode);

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
l_uint8  *data;
FILE     *fp;
PIX      *pix;

    PROCNAME("pixReadMemPng");

    if (!cdata)
        return (PIX *)ERROR_PTR("cdata not defined", procName, NULL);

    data = (l_uint8 *)cdata;  /* we're really not going to change this */
    if ((fp = fmemopen(data, size, "r")) == NULL)
        return (PIX *)ERROR_PTR("stream not opened", procName, NULL);
    pix = pixReadStreamPng(fp);
    fclose(fp);
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

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1 );
    if (!psize)
        return ERROR_INT("&size not defined", procName, 1 );
    if (!pix)
        return ERROR_INT("&pix not defined", procName, 1 );

    if ((fp = open_memstream((char **)pdata, psize)) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    ret = pixWriteStreamPng(fp, pix, gamma);
    fclose(fp);
    return ret;
}

#else

PIX *
pixReadMemPng(const l_uint8  *cdata,
              size_t          size)
{
    return (PIX *)ERROR_PTR(
        "png read from memory not implemented on this platform",
        "pixReadMemPng", NULL);
}


l_int32
pixWriteMemPng(l_uint8  **pdata,
               size_t    *psize,
               PIX       *pix,
               l_float32  gamma)
{
    return ERROR_INT(
        "png write to memory not implemented on this platform",
        "pixWriteMemPng", 1);
}

#endif  /* HAVE_FMEMOPEN */

/* --------------------------------------------*/
#endif  /* HAVE_LIBPNG */
/* --------------------------------------------*/

