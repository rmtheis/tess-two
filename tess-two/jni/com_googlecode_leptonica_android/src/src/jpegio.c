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
 *  jpegio.c
 *
 *    Read jpeg from file
 *          PIX             *pixReadJpeg()  [special top level]
 *          PIX             *pixReadStreamJpeg()
 *
 *    Read jpeg metadata from file
 *          l_int32          readHeaderJpeg()
 *          l_int32          freadHeaderJpeg()
 *          l_int32          fgetJpegResolution()
 *          l_int32          fgetJpegComment()
 *
 *    Write jpeg to file
 *          l_int32          pixWriteJpeg()  [special top level]
 *          l_int32          pixWriteStreamJpeg()
 *
 *    Read/write to memory
 *          PIX             *pixReadMemJpeg()
 *          l_int32          readHeaderMemJpeg()
 *          l_int32          pixWriteMemJpeg()
 *
 *    Setting special flag
 *          l_int32          pixSetChromaSampling()
 *
 *    Static system helpers
 *          static void      jpeg_error_catch_all_1()
 *          static void      jpeg_error_catch_all_2()
 *          static l_uint8   jpeg_getc()
 *          static l_int32   jpeg_comment_callback()
 *
 *    Extraction of jpeg header info by parsing  [deprecated]
 *          l_int32          extractJpegDataFromFile()
 *          l_int32          extractJpegDataFromArray()
 *          static l_int32   extractJpegHeaderDataFallback()
 *          static l_int32   locateJpegImageParameters()
 *          static l_int32   getNextJpegMarker()
 *          static l_int32   getTwoByteParameter()
 *
 *    Documentation: libjpeg.doc can be found, along with all
 *    source code, at ftp://ftp.uu.net/graphics/jpeg
 *    Download and untar the file:  jpegsrc.v6b.tar.gz
 *    A good paper on jpeg can also be found there: wallace.ps.gz
 *
 *    The functions in libjpeg make it very simple to compress
 *    and decompress images.  On input (decompression from file),
 *    3 component color images can be read into either an 8 bpp Pix
 *    with a colormap or a 32 bpp Pix with RGB components.  For output
 *    (compression to file), all color Pix, whether 8 bpp with a
 *    colormap or 32 bpp, are written compressed as a set of three
 *    8 bpp (rgb) images.
 *
 *    Low-level error handling
 *    ------------------------
 *    The default behavior of the jpeg library is to call exit.
 *    This is often undesirable, and the caller should make the
 *    decision when to abort a process.  To prevent the jpeg library
 *    from calling exit(), setjmp() has been inserted into all
 *    readers and writers, and the cinfo struct has been set up so that
 *    the low-level jpeg library will call a special error handler
 *    that doesn't exit, instead of the default function error_exit().
 *
 *    To avoid race conditions and make these functions thread-safe in
 *    the rare situation where calls to two threads are simultaneously
 *    failing on bad jpegs, we insert a local copy of the jmp_buf struct
 *    into the cinfo.client_data field, and use this on longjmp.
 *    For extracting the jpeg comment, we have the added complication
 *    that the client_data field must also return the jpeg comment,
 *    and we use a different error handler.
 *
 *    How to avoid subsampling the chroma channels
 *    --------------------------------------------
 *    When writing, you can avoid subsampling the U,V (chroma)
 *    channels.  This gives higher quality for the color, which is
 *    important for some situations.  The default subsampling is 2x2 on
 *    both channels.  Before writing, call pixSetChromaSampling(pix, 0)
 *    to prevent chroma subsampling.
 *
 *    Compressing to memory and decompressing from memory
 *    ---------------------------------------------------
 *    On systems like windows without fmemopen() and open_memstream(),
 *    we write data to a temp file and read it back for operations
 *    between pix and compressed-data, such as pixReadMemJpeg() and
 *    pixWriteMemJpeg().
 *
 *    Vestigial code: parsing the jpeg file for header metadata
 *    ---------------------------------------------------------
 *    For extracting header metadata, we used to parse the file, looking
 *    for specific markers.  This is error-prone because of non-standard
 *    jpeg files and you should use readHeaderJpeg() and readHeaderMemJpeg()
 *    instead.  Nevertheless, it is retained here in case you want to
 *    understand a bit about how to parse jpeg markers.
 */

#include <string.h>
#include "allheaders.h"

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

/* --------------------------------------------*/
#if  HAVE_LIBJPEG   /* defined in environ.h */
/* --------------------------------------------*/

#include <setjmp.h>

    /* jconfig.h makes the error of setting
     *   #define HAVE_STDLIB_H
     * which conflicts with config_auto.h (where it is set to 1) and results
     * for some gcc compiler versions in a warning.  The conflict is harmless
     * but we suppress it by undefining the variable. */
#undef HAVE_STDLIB_H
#include "jpeglib.h"

static void jpeg_error_catch_all_1(j_common_ptr cinfo);
static void jpeg_error_catch_all_2(j_common_ptr cinfo);
static l_uint8 jpeg_getc(j_decompress_ptr cinfo);

    /* Note: 'boolean' is defined in jmorecfg.h.  We use it explicitly
     * here because for windows where __MINGW32__ is defined,
     * the prototype for jpeg_comment_callback() is given as
     * returning a boolean.  */
static boolean jpeg_comment_callback(j_decompress_ptr cinfo);

    /* This is saved in the client_data field of cinfo, and used both
     * to retrieve the comment from its callback and to handle
     * exceptions with a longjmp. */
struct callback_data {
    jmp_buf   jmpbuf;
    l_uint8  *comment;
};

    /* Static helpers for extraction of jpeg data (not used) */
static l_int32  extractJpegHeaderDataFallback(const void *data, size_t nbytes,
                                              l_int32 *pw, l_int32 *ph,
                                              l_int32 *pbps, l_int32 *pspp);
static l_int32  locateJpegImageParameters(l_uint8 *, size_t, l_int32 *);
static l_int32  getNextJpegMarker(l_uint8 *, size_t, l_int32 *);
static l_int32  getTwoByteParameter(l_uint8 *, l_int32);


#ifndef  NO_CONSOLE_IO
#define  DEBUG_INFO      0
#endif  /* ~NO_CONSOLE_IO */


/*---------------------------------------------------------------------*
 *                          Read jpeg from file                        *
 *---------------------------------------------------------------------*/
/*!
 *  pixReadJpeg()
 *
 *      Input:  filename
 *              colormap flag (0 means return RGB image if color;
 *                             1 means create colormap and return 8 bpp
 *                               palette image if color)
 *              reduction (scaling factor: 1, 2, 4 or 8)
 *              &nwarn (<optional return> number of warnings about
 *                       corrupted data)
 *      Return: pix, or null on error
 *
 *  Images reduced by factors of 2, 4 or 8 can be returned
 *  significantly faster than full resolution images.
 *
 *  The jpeg library will return warnings (or exit) if
 *  the jpeg data is bad.  Use this function if you want the
 *  jpeg library to create an 8 bpp palette image, or to
 *  tell if the jpeg data has been corrupted.  For corrupt jpeg
 *  data, there are two possible outcomes:
 *    (1) a damaged pix will be returned, along with a nonzero
 *        number of warnings, or
 *    (2) for sufficiently serious problems, the library will attempt
 *        to exit (caught by our error handler) and no pix will be returned.
 */
PIX *
pixReadJpeg(const char  *filename,
            l_int32      cmflag,
            l_int32      reduction,
            l_int32     *pnwarn)
{
l_int32   ret;
l_uint8  *comment;
FILE     *fp;
PIX      *pix;

    PROCNAME("pixReadJpeg");

    if (pnwarn) *pnwarn = 0;
    if (!filename)
        return (PIX *)ERROR_PTR("filename not defined", procName, NULL);
    if (cmflag != 0 && cmflag != 1)
        cmflag = 0;  /* default */
    if (reduction != 1 && reduction != 2 && reduction != 4 && reduction != 8)
        return (PIX *)ERROR_PTR("reduction not in {1,2,4,8}", procName, NULL);

    if ((fp = fopenReadStream(filename)) == NULL)
        return (PIX *)ERROR_PTR("image file not found", procName, NULL);
    pix = pixReadStreamJpeg(fp, cmflag, reduction, pnwarn, 0);
    if (pix) {
        ret = fgetJpegComment(fp, &comment);
        if (!ret && comment)
            pixSetText(pix, (char *)comment);
        FREE(comment);
    }
    fclose(fp);

    if (!pix)
        return (PIX *)ERROR_PTR("image not returned", procName, NULL);
    return pix;
}


/*!
 *  pixReadStreamJpeg()
 *
 *      Input:  stream
 *              colormap flag (0 means return RGB image if color;
 *                             1 means create colormap and return 8 bpp
 *                               palette image if color)
 *              reduction (scaling factor: 1, 2, 4 or 8)
 *              &nwarn (<optional return> number of warnings)
 *              hint: (a bitwise OR of L_HINT_* values); use 0 for no hints
 *      Return: pix, or null on error
 *
 *  Usage: see pixReadJpeg()
 *  Notes:
 *      (1) This does not get the jpeg comment.
 */
PIX *
pixReadStreamJpeg(FILE     *fp,
                  l_int32   cmflag,
                  l_int32   reduction,
                  l_int32  *pnwarn,
                  l_int32   hint)
{
l_int32                        cyan, yellow, magenta, black;
l_int32                        i, j, k, rval, gval, bval;
l_int32                        w, h, wpl, spp, ncolors, cindex, ycck, cmyk;
l_uint32                      *data;
l_uint32                      *line, *ppixel;
JSAMPROW                       rowbuffer;
PIX                           *pix;
PIXCMAP                       *cmap;
struct jpeg_decompress_struct  cinfo;
struct jpeg_error_mgr          jerr;
jmp_buf                        jmpbuf;  /* must be local to the function */

    PROCNAME("pixReadStreamJpeg");

    if (!fp)
        return (PIX *)ERROR_PTR("fp not defined", procName, NULL);
    if (pnwarn)
        *pnwarn = 0;  /* init */
    if (cmflag != 0 && cmflag != 1)
        cmflag = 0;  /* default */
    if (reduction != 1 && reduction != 2 && reduction != 4 && reduction != 8)
        return (PIX *)ERROR_PTR("reduction not in {1,2,4,8}", procName, NULL);

    if (BITS_IN_JSAMPLE != 8)  /* set in jmorecfg.h */
        return (PIX *)ERROR_PTR("BITS_IN_JSAMPLE != 8", procName, NULL);

    rewind(fp);
    pix = NULL;
    rowbuffer = NULL;

        /* Modify the jpeg error handling to catch fatal errors  */
    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = jpeg_error_catch_all_1;
    cinfo.client_data = (void *)&jmpbuf;
    if (setjmp(jmpbuf)) {
        pixDestroy(&pix);
        FREE(rowbuffer);
        return (PIX *)ERROR_PTR("internal jpeg error", procName, NULL);
    }

        /* Initialize jpeg structs for decompression */
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, TRUE);
    cinfo.scale_denom = reduction;
    cinfo.scale_num = 1;
    if (hint & L_HINT_GRAY)
        cinfo.out_color_space = JCS_GRAYSCALE;
    jpeg_calc_output_dimensions(&cinfo);

        /* Allocate the image and a row buffer */
    spp = cinfo.out_color_components;
    w = cinfo.output_width;
    h = cinfo.output_height;
    ycck = (cinfo.jpeg_color_space == JCS_YCCK && spp == 4 && cmflag == 0);
    cmyk = (cinfo.jpeg_color_space == JCS_CMYK && spp == 4 && cmflag == 0);
    if (spp != 1 && spp != 3 && !ycck && !cmyk) {
        return (PIX *)ERROR_PTR("spp must be 1 or 3, or YCCK or CMYK",
                                procName, NULL);
    }
    if ((spp == 3 && cmflag == 0) || ycck || cmyk) {  /* rgb or 4 bpp color */
        rowbuffer = (JSAMPROW)CALLOC(sizeof(JSAMPLE), spp * w);
        pix = pixCreate(w, h, 32);
    } else {  /* 8 bpp gray or colormapped */
        rowbuffer = (JSAMPROW)CALLOC(sizeof(JSAMPLE), w);
        pix = pixCreate(w, h, 8);
    }
    if (!rowbuffer || !pix) {
        if (rowbuffer) FREE(rowbuffer);
        pixDestroy(&pix);
        return (PIX *)ERROR_PTR("rowbuffer or pix not made", procName, NULL);
    }

    if (spp == 1) {  /* Grayscale or colormapped */
        jpeg_start_decompress(&cinfo);
    } else {        /* Color; spp == 3 or YCCK or CMYK */
        if (cmflag == 0) {   /* -- 24 bit color in 32 bit pix or YCCK/CMYK -- */
            cinfo.quantize_colors = FALSE;
            jpeg_start_decompress(&cinfo);
        } else {      /* Color quantize to 8 bits */
            cinfo.quantize_colors = TRUE;
            cinfo.desired_number_of_colors = 256;
            jpeg_start_decompress(&cinfo);

                /* Construct a pix cmap */
            cmap = pixcmapCreate(8);
            ncolors = cinfo.actual_number_of_colors;
            for (cindex = 0; cindex < ncolors; cindex++) {
                rval = cinfo.colormap[0][cindex];
                gval = cinfo.colormap[1][cindex];
                bval = cinfo.colormap[2][cindex];
                pixcmapAddColor(cmap, rval, gval, bval);
            }
            pixSetColormap(pix, cmap);
        }
    }
    wpl  = pixGetWpl(pix);
    data = pixGetData(pix);

        /* Decompress */
    if ((spp == 3 && cmflag == 0) || ycck || cmyk) {   /* -- 24 bit color -- */
        for (i = 0; i < h; i++) {
            if (jpeg_read_scanlines(&cinfo, &rowbuffer, (JDIMENSION)1) != 1)
                return (PIX *)ERROR_PTR("bad read scanline", procName, NULL);
            ppixel = data + i * wpl;
            if (spp == 3) {
                for (j = k = 0; j < w; j++) {
                    SET_DATA_BYTE(ppixel, COLOR_RED, rowbuffer[k++]);
                    SET_DATA_BYTE(ppixel, COLOR_GREEN, rowbuffer[k++]);
                    SET_DATA_BYTE(ppixel, COLOR_BLUE, rowbuffer[k++]);
                    ppixel++;
                }
            } else {
                    /* This is a conversion from CMYK -> RGB that ignores
                       color profiles, and is invoked when the image header
                       claims to be in CMYK or YCCK colorspace.  If in YCCK,
                       libjpeg may be doing YCCK -> CMYK under the hood.
                       To understand why the colors need to be inverted on
                       read-in for the Adobe marker, see the "Special
                       color spaces" section of "Using the IJG JPEG
                       Library" by Thomas G. Lane:
                         http://www.jpegcameras.com/libjpeg/libjpeg-3.html#ss3.1
                       The non-Adobe conversion is equivalent to:
                           rval = black - black * cyan / 255
                           ...
                       The Adobe conversion is equivalent to:
                           rval = black - black * (255 - cyan) / 255
                           ...
                       Note that cyan is the complement to red, and we
                       are subtracting the complement color (weighted
                       by black) from black.  For Adobe conversions,
                       where they've already inverted the CMY but not
                       the K, we have to invert again.  The results
                       must be clipped to [0 ... 255]. */
                for (j = k = 0; j < w; j++) {
                    cyan = rowbuffer[k++];
                    magenta = rowbuffer[k++];
                    yellow = rowbuffer[k++];
                    black = rowbuffer[k++];
                    if (cinfo.saw_Adobe_marker) {
                        rval = (black * cyan) / 255;
                        gval = (black * magenta) / 255;
                        bval = (black * yellow) / 255;
                    } else {
                        rval = black * (255 - cyan) / 255;
                        gval = black * (255 - magenta) / 255;
                        bval = black * (255 - yellow) / 255;
                    }
                    rval = L_MIN(L_MAX(rval, 0), 255);
                    gval = L_MIN(L_MAX(gval, 0), 255);
                    bval = L_MIN(L_MAX(bval, 0), 255);
                    composeRGBPixel(rval, gval, bval, ppixel);
                    ppixel++;
                }
            }
        }
    } else {    /* 8 bpp grayscale or colormapped pix */
        for (i = 0; i < h; i++) {
            if (jpeg_read_scanlines(&cinfo, &rowbuffer, (JDIMENSION)1) != 1)
                return (PIX *)ERROR_PTR("bad read scanline", procName, NULL);
            line = data + i * wpl;
            for (j = 0; j < w; j++)
                SET_DATA_BYTE(line, j, rowbuffer[j]);
        }
    }

    if (pnwarn)
        *pnwarn = cinfo.err->num_warnings;

        /* If the pixel density is neither 1 nor 2, it may not be defined.
         * In that case, don't set the resolution.  */
    if (cinfo.density_unit == 1) {  /* pixels per inch */
        pixSetXRes(pix, cinfo.X_density);
        pixSetYRes(pix, cinfo.Y_density);
    } else if (cinfo.density_unit == 2) {  /* pixels per centimeter */
        pixSetXRes(pix, (l_int32)((l_float32)cinfo.X_density * 2.54 + 0.5));
        pixSetYRes(pix, (l_int32)((l_float32)cinfo.Y_density * 2.54 + 0.5));
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    FREE(rowbuffer);
    return pix;
}


/*---------------------------------------------------------------------*
 *                     Read jpeg metadata from file                    *
 *---------------------------------------------------------------------*/
/*!
 *  readHeaderJpeg()
 *
 *      Input:  filename
 *              &w (<optional return>)
 *              &h (<optional return>)
 *              &spp (<optional return>, samples/pixel)
 *              &ycck (<optional return>, 1 if ycck color space; 0 otherwise)
 *              &cmyk (<optional return>, 1 if cmyk color space; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 */
l_int32
readHeaderJpeg(const char  *filename,
               l_int32     *pw,
               l_int32     *ph,
               l_int32     *pspp,
               l_int32     *pycck,
               l_int32     *pcmyk)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("readHeaderJpeg");

    if (!pw && !ph && !pspp && !pycck && !pcmyk)
        return ERROR_INT("no results requested", procName, 1);
    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (pspp) *pspp = 0;
    if (pycck) *pycck = 0;
    if (pcmyk) *pcmyk = 0;
    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);

    if ((fp = fopenReadStream(filename)) == NULL)
        return ERROR_INT("image file not found", procName, 1);
    ret = freadHeaderJpeg(fp, pw, ph, pspp, pycck, pcmyk);
    fclose(fp);
    return ret;
}


/*!
 *  freadHeaderJpeg()
 *
 *      Input:  stream
 *              &w (<optional return>)
 *              &h (<optional return>)
 *              &spp (<optional return>, samples/pixel)
 *              &ycck (<optional return>, 1 if ycck color space; 0 otherwise)
 *              &cmyk (<optional return>, 1 if cmyk color space; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 */
l_int32
freadHeaderJpeg(FILE     *fp,
                l_int32  *pw,
                l_int32  *ph,
                l_int32  *pspp,
                l_int32  *pycck,
                l_int32  *pcmyk)
{
l_int32                        spp;
struct jpeg_decompress_struct  cinfo;
struct jpeg_error_mgr          jerr;
jmp_buf                        jmpbuf;  /* must be local to the function */

    PROCNAME("freadHeaderJpeg");

    if (!pw && !ph && !pspp && !pycck && !pcmyk)
        return ERROR_INT("no results requested", procName, 1);
    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (pspp) *pspp = 0;
    if (pycck) *pycck = 0;
    if (pcmyk) *pcmyk = 0;
    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);

    rewind(fp);

        /* Modify the jpeg error handling to catch fatal errors  */
    cinfo.err = jpeg_std_error(&jerr);
    cinfo.client_data = (void *)&jmpbuf;
    jerr.error_exit = jpeg_error_catch_all_1;
    if (setjmp(jmpbuf))
        return ERROR_INT("internal jpeg error", procName, 1);

        /* Initialize the jpeg structs for reading the header */
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_calc_output_dimensions(&cinfo);

    spp = cinfo.out_color_components;
    if (pspp) *pspp = spp;
    if (pw) *pw = cinfo.output_width;
    if (ph) *ph = cinfo.output_height;
    if (pycck) *pycck =
        (cinfo.jpeg_color_space == JCS_YCCK && spp == 4);
    if (pcmyk) *pcmyk =
        (cinfo.jpeg_color_space == JCS_CMYK && spp == 4);

    jpeg_destroy_decompress(&cinfo);
    rewind(fp);
    return 0;
}


/*
 *  fgetJpegResolution()
 *
 *      Input:  stream (opened for read)
 *              &xres, &yres (<return> resolution in ppi)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) If neither resolution field is set, this is not an error;
 *          the returned resolution values are 0 (designating 'unknown').
 *      (2) Side-effect: this rewinds the stream.
 */
l_int32
fgetJpegResolution(FILE     *fp,
                   l_int32  *pxres,
                   l_int32  *pyres)
{
struct jpeg_decompress_struct  cinfo;
struct jpeg_error_mgr          jerr;
jmp_buf                        jmpbuf;  /* must be local to the function */

    PROCNAME("fgetJpegResolution");

    if (!pxres || !pyres)
        return ERROR_INT("&xres and &yres not both defined", procName, 1);
    *pxres = *pyres = 0;
    if (!fp)
        return ERROR_INT("stream not opened", procName, 1);

    rewind(fp);

        /* Modify the jpeg error handling to catch fatal errors  */
    cinfo.err = jpeg_std_error(&jerr);
    cinfo.client_data = (void *)&jmpbuf;
    jerr.error_exit = jpeg_error_catch_all_1;
    if (setjmp(jmpbuf))
        return ERROR_INT("internal jpeg error", procName, 1);

        /* Initialize the jpeg structs for reading the header */
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, TRUE);

        /* It is common for the input resolution to be omitted from the
         * jpeg file.  If density_unit is not 1 or 2, simply return 0. */
    if (cinfo.density_unit == 1) {  /* pixels/inch */
        *pxres = cinfo.X_density;
        *pyres = cinfo.Y_density;
    } else if (cinfo.density_unit == 2) {  /* pixels/cm */
        *pxres = (l_int32)((l_float32)cinfo.X_density * 2.54 + 0.5);
        *pyres = (l_int32)((l_float32)cinfo.Y_density * 2.54 + 0.5);
    }

    jpeg_destroy_decompress(&cinfo);
    rewind(fp);
    return 0;
}


/*
 *  fgetJpegComment()
 *
 *      Input:  stream (opened for read)
 *              &comment (<return> comment)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Side-effect: this rewinds the stream.
 */
l_int32
fgetJpegComment(FILE      *fp,
                l_uint8  **pcomment)
{
struct jpeg_decompress_struct  cinfo;
struct jpeg_error_mgr          jerr;
struct callback_data           cb_data;  /* contains local jmp_buf */

    PROCNAME("fgetJpegComment");

    if (!pcomment)
        return ERROR_INT("&comment not defined", procName, 1);
    *pcomment = NULL;
    if (!fp)
        return ERROR_INT("stream not opened", procName, 1);

    rewind(fp);

        /* Modify the jpeg error handling to catch fatal errors  */
    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = jpeg_error_catch_all_2;
    cb_data.comment = NULL;
    cinfo.client_data = (void *)&cb_data;
    if (setjmp(cb_data.jmpbuf)) {
        FREE(cb_data.comment);
        return ERROR_INT("internal jpeg error", procName, 1);
    }

        /* Initialize the jpeg structs for reading the header */
    jpeg_create_decompress(&cinfo);
    jpeg_set_marker_processor(&cinfo, JPEG_COM, jpeg_comment_callback);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, TRUE);

        /* Save the result */
    *pcomment = cb_data.comment;
    jpeg_destroy_decompress(&cinfo);
    rewind(fp);
    return 0;
}


/*---------------------------------------------------------------------*
 *                             Writing Jpeg                            *
 *---------------------------------------------------------------------*/
/*!
 *  pixWriteJpeg()
 *
 *      Input:  filename
 *              pix
 *              quality (1 - 100; 75 is default)
 *              progressive (0 for baseline sequential; 1 for progressive)
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixWriteJpeg(const char  *filename,
             PIX         *pix,
             l_int32      quality,
             l_int32      progressive)
{
FILE  *fp;

    PROCNAME("pixWriteJpeg");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "wb+")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);

    if (pixWriteStreamJpeg(fp, pix, quality, progressive)) {
        fclose(fp);
        return ERROR_INT("pix not written to stream", procName, 1);
    }

    fclose(fp);
    return 0;
}


/*!
 *  pixWriteStreamJpeg()
 *
 *      Input:  stream
 *              pix  (8 or 32 bpp)
 *              quality  (1 - 100; 75 is default value; 0 is also default)
 *              progressive (0 for baseline sequential; 1 for progressive)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Under the covers, the library transforms rgb to a
 *          luminence-chromaticity triple, each component of which is
 *          also 8 bits, and compresses that.  It uses 2 Huffman tables,
 *          a higher resolution one (with more quantization levels)
 *          for luminosity and a lower resolution one for the chromas.
 *      (2) Progressive encoding gives better compression, at the
 *          expense of slower encoding and decoding.
 *      (3) Standard chroma subsampling is 2x2 on both the U and V
 *          channels.  For highest quality, use no subsampling; this
 *          option is set by pixSetChromaSampling(pix, 0).
 *      (4) There are three possibilities:
 *          * Grayscale image, no colormap: compress as 8 bpp image.
 *          * rgb full color image: copy each line into the color
 *            line buffer, and compress as three 8 bpp images.
 *          * 8 bpp colormapped image: convert each line to three
 *            8 bpp line images in the color line buffer, and
 *            compress as three 8 bpp images.
 *      (5) The only valid pixel depths in leptonica are 1, 2, 4, 8, 16
 *          and 32 bpp.  However, it is possible, and in some cases desirable,
 *          to write out a jpeg file using an rgb pix that has 24 bpp.
 *          This can be created by appending the raster data for a 24 bpp
 *          image (with proper scanline padding) directly to a 24 bpp
 *          pix that was created without a data array.  See note in
 *          pixWriteStreamPng() for an example.
 */
l_int32
pixWriteStreamJpeg(FILE    *fp,
                   PIX     *pix,
                   l_int32  quality,
                   l_int32  progressive)
{
l_uint8                      byteval;
l_int32                      xres, yres;
l_int32                      i, j, k;
l_int32                      w, h, d, wpl, spp, colorflg, rowsamples;
l_int32                     *rmap, *gmap, *bmap;
l_uint32                    *ppixel, *line, *data;
JSAMPROW                     rowbuffer;
PIXCMAP                     *cmap;
struct jpeg_compress_struct  cinfo;
struct jpeg_error_mgr        jerr;
const char                  *text;
jmp_buf                      jmpbuf;  /* must be local to the function */

    PROCNAME("pixWriteStreamJpeg");

    if (!fp)
        return ERROR_INT("stream not open", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 8 && d != 24 && d != 32)
        return ERROR_INT("bpp must be 8, 24 or 32", procName, 1);
    if (quality <= 0)
        quality = 75;  /* default */

    rewind(fp);
    rowbuffer = NULL;
    rmap = NULL;
    gmap = NULL;
    bmap = NULL;

        /* Modify the jpeg error handling to catch fatal errors  */
    cinfo.err = jpeg_std_error(&jerr);
    cinfo.client_data = (void *)&jmpbuf;
    jerr.error_exit = jpeg_error_catch_all_1;
    if (setjmp(jmpbuf)) {
        FREE(rowbuffer);
        if (colorflg == 1) {
            FREE(rmap);
            FREE(gmap);
            FREE(bmap);
        }
        return ERROR_INT("internal jpeg error", procName, 1);
    }

        /* Initialize the jpeg structs for compression */
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);
    cinfo.image_width  = w;
    cinfo.image_height = h;

        /* Set the color space and number of components */
    if (d == 32 || d == 24) {
        colorflg = 2;    /* rgb; no colormap */
    } else if ((cmap = pixGetColormap(pix)) == NULL) {
        colorflg = 0;    /* 8 bpp grayscale; no colormap */
    } else {
        colorflg = 1;    /* 8 bpp; colormap */
        pixcmapToArrays(cmap, &rmap, &gmap, &bmap, NULL);
    }
    if (colorflg == 0) {
        cinfo.input_components = 1;
        cinfo.in_color_space = JCS_GRAYSCALE;
    } else {  /* colorflg == 1 or 2 */
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
    }

    jpeg_set_defaults(&cinfo);

        /* Setting optimize_coding to TRUE seems to improve compression
         * by approx 2-4 percent, and increases comp time by approx 20%. */
    cinfo.optimize_coding = FALSE;

        /* Set resolution in pixels/in (density_unit: 1 = in, 2 = cm) */
    xres = pixGetXRes(pix);
    yres = pixGetYRes(pix);
    if ((xres != 0) && (yres != 0)) {
        cinfo.density_unit = 1;  /* designates pixels per inch */
        cinfo.X_density = xres;
        cinfo.Y_density = yres;
    }

        /* Set the quality and progressive parameters */
    jpeg_set_quality(&cinfo, quality, TRUE);
    if (progressive)
        jpeg_simple_progression(&cinfo);

        /* Set the chroma subsampling parameters.  This is done in
         * YUV color space.  The Y (intensity) channel is never subsampled.
         * The standard subsampling is 2x2 on both the U and V channels.
         * Notation on this is confusing.  For a nice illustrations, see
         *   http://en.wikipedia.org/wiki/Chroma_subsampling
         * The standard subsampling is written as 4:2:0.
         * We allow high quality where there is no subsampling on the
         * chroma channels: denoted as 4:4:4.  */
    if (pix->special == L_NO_CHROMA_SAMPLING_JPEG) {
        cinfo.comp_info[0].h_samp_factor = 1;
        cinfo.comp_info[0].v_samp_factor = 1;
        cinfo.comp_info[1].h_samp_factor = 1;
        cinfo.comp_info[1].v_samp_factor = 1;
        cinfo.comp_info[2].h_samp_factor = 1;
        cinfo.comp_info[2].v_samp_factor = 1;
        pix->special = 0;
    }

    jpeg_start_compress(&cinfo, TRUE);

    if ((text = pixGetText(pix)))
        jpeg_write_marker(&cinfo, JPEG_COM, (const JOCTET *)text, strlen(text));

        /* Allocate row buffer */
    spp = cinfo.input_components;
    rowsamples = spp * w;
    if ((rowbuffer = (JSAMPROW)CALLOC(sizeof(JSAMPLE), rowsamples)) == NULL)
        return ERROR_INT("calloc fail for rowbuffer", procName, 1);

    data = pixGetData(pix);
    wpl  = pixGetWpl(pix);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        if (colorflg == 0) {        /* 8 bpp gray */
            for (j = 0; j < w; j++)
                rowbuffer[j] = GET_DATA_BYTE(line, j);
        } else if (colorflg == 1) {  /* 8 bpp colormapped */
            for (j = 0; j < w; j++) {
                byteval = GET_DATA_BYTE(line, j);
                rowbuffer[3 * j + COLOR_RED] = rmap[byteval];
                rowbuffer[3 * j + COLOR_GREEN] = gmap[byteval];
                rowbuffer[3 * j + COLOR_BLUE] = bmap[byteval];
            }
        } else {  /* colorflg == 2 */
            if (d == 24) {  /* See note 4 above; special case of 24 bpp rgb */
                jpeg_write_scanlines(&cinfo, (JSAMPROW *)&line, 1);
            } else {  /* standard 32 bpp rgb */
                ppixel = line;
                for (j = k = 0; j < w; j++) {
                    rowbuffer[k++] = GET_DATA_BYTE(ppixel, COLOR_RED);
                    rowbuffer[k++] = GET_DATA_BYTE(ppixel, COLOR_GREEN);
                    rowbuffer[k++] = GET_DATA_BYTE(ppixel, COLOR_BLUE);
                    ppixel++;
                }
            }
        }
        if (d != 24)
            jpeg_write_scanlines(&cinfo, &rowbuffer, 1);
    }
    jpeg_finish_compress(&cinfo);

    FREE(rowbuffer);
    if (colorflg == 1) {
        FREE(rmap);
        FREE(gmap);
        FREE(bmap);
    }
    jpeg_destroy_compress(&cinfo);
    return 0;
}


/*---------------------------------------------------------------------*
 *                         Read/write to memory                        *
 *---------------------------------------------------------------------*/
#if HAVE_FMEMOPEN
extern FILE *open_memstream(char **data, size_t *size);
extern FILE *fmemopen(void *data, size_t size, const char *mode);
#endif  /* HAVE_FMEMOPEN */

/*!
 *  pixReadMemJpeg()
 *
 *      Input:  cdata (const; jpeg-encoded)
 *              size (of data)
 *              colormap flag (0 means return RGB image if color;
 *                             1 means create colormap and return 8 bpp
 *                               palette image if color)
 *              reduction (scaling factor: 1, 2, 4 or 8)
 *              &nwarn (<optional return> number of warnings)
 *              hint (bitwise OR of L_HINT_* values; use 0 for no hint)
 *      Return: pix, or null on error
 *
 *  Notes:
 *      (1) The @size byte of @data must be a null character.
 *      (2) See pixReadJpeg() for usage.
 */
PIX *
pixReadMemJpeg(const l_uint8  *cdata,
               size_t          size,
               l_int32         cmflag,
               l_int32         reduction,
               l_int32        *pnwarn,
               l_int32         hint)
{
l_int32   ret;
l_uint8  *comment;
FILE     *fp;
PIX      *pix;

    PROCNAME("pixReadMemJpeg");

    if (!cdata)
        return (PIX *)ERROR_PTR("cdata not defined", procName, NULL);

#if HAVE_FMEMOPEN
    if ((fp = fmemopen((l_uint8 *)cdata, size, "r")) == NULL)
        return (PIX *)ERROR_PTR("stream not opened", procName, NULL);
#else
    L_WARNING("work-around: writing to a temp file\n", procName);
    fp = tmpfile();
    fwrite(cdata, 1, size, fp);
    rewind(fp);
#endif  /* HAVE_FMEMOPEN */
    pix = pixReadStreamJpeg(fp, cmflag, reduction, pnwarn, hint);
    if (pix) {
        ret = fgetJpegComment(fp, &comment);
        if (!ret && comment) {
            pixSetText(pix, (char *)comment);
            FREE(comment);
        }
    }
    fclose(fp);
    if (!pix) L_ERROR("pix not read\n", procName);
    return pix;
}


/*!
 *  readHeaderMemJpeg()
 *
 *      Input:  cdata (const; jpeg-encoded)
 *              size (of data)
 *              &w (<optional return>)
 *              &h (<optional return>)
 *              &spp (<optional return>, samples/pixel)
 *              &ycck (<optional return>, 1 if ycck color space; 0 otherwise)
 *              &cmyk (<optional return>, 1 if cmyk color space; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 */
l_int32
readHeaderMemJpeg(const l_uint8  *cdata,
                  size_t          size,
                  l_int32        *pw,
                  l_int32        *ph,
                  l_int32        *pspp,
                  l_int32        *pycck,
                  l_int32        *pcmyk)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("readHeaderMemJpeg");

    if (!pw && !ph && !pspp && !pycck && !pcmyk)
        return ERROR_INT("no results requested", procName, 1);
    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (pspp) *pspp = 0;
    if (pycck) *pycck = 0;
    if (pcmyk) *pcmyk = 0;
    if (!cdata)
        return ERROR_INT("cdata not defined", procName, 1);

#if HAVE_FMEMOPEN
    if ((fp = fmemopen((l_uint8 *)cdata, size, "r")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
#else
    L_WARNING("work-around: writing to a temp file\n", procName);
    fp = tmpfile();
    fwrite(cdata, 1, size, fp);
    rewind(fp);
#endif  /* HAVE_FMEMOPEN */
    ret = freadHeaderJpeg(fp, pw, ph, pspp, pycck, pcmyk);
    fclose(fp);
    return ret;
}


/*!
 *  pixWriteMemJpeg()
 *
 *      Input:  &data (<return> data of jpeg compressed image)
 *              &size (<return> size of returned data)
 *              pix
 *              quality  (1 - 100; 75 is default value; 0 is also default)
 *              progressive (0 for baseline sequential; 1 for progressive)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) See pixWriteStreamJpeg() for usage.  This version writes to
 *          memory instead of to a file stream.
 */
l_int32
pixWriteMemJpeg(l_uint8  **pdata,
                size_t    *psize,
                PIX       *pix,
                l_int32    quality,
                l_int32    progressive)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("pixWriteMemJpeg");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1 );
    *pdata = NULL;
    if (!psize)
        return ERROR_INT("&size not defined", procName, 1 );
    *psize = 0;
    if (!pix)
        return ERROR_INT("&pix not defined", procName, 1 );

#if HAVE_FMEMOPEN
    if ((fp = open_memstream((char **)pdata, psize)) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    ret = pixWriteStreamJpeg(fp, pix, quality, progressive);
#else
    L_WARNING("work-around: writing to a temp file\n", procName);
    fp = tmpfile();
    ret = pixWriteStreamJpeg(fp, pix, quality, progressive);
    rewind(fp);
    *pdata = l_binaryReadStream(fp, psize);
#endif  /* HAVE_FMEMOPEN */
    fclose(fp);
    return ret;
}


/*---------------------------------------------------------------------*
 *                  Setting for chroma sampling on write               *
 *---------------------------------------------------------------------*/
/*!
 *  pixSetChromaSampling()
 *
 *      Input:  pix
 *              sampling (1 for subsampling; 0 for no subsampling)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The default is for 2x2 chroma subsampling because the files are
 *          considerably smaller and the appearance is typically satisfactory.
 *          Call this with @sampling == 0 for full resolution output in
 *          chroma channels for jpeg writing.
 */
l_int32
pixSetChromaSampling(PIX     *pix,
                     l_int32  sampling)
{
    PROCNAME("pixSetChromaSampling");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1 );
    if (sampling)
        pix->special = 0;  /* default */
    else
        pix->special = L_NO_CHROMA_SAMPLING_JPEG;
    return 0;
}


/*---------------------------------------------------------------------*
 *                        Static system helpers                        *
 *---------------------------------------------------------------------*/
/*!
 *  jpeg_error_catch_all_1()
 *
 *  Notes:
 *      (1) The default jpeg error_exit() kills the process, but we
 *          never want a call to leptonica to kill a process.  If you
 *          do want this behavior, remove the calls to these error handlers.
 *      (2) This is used where cinfo->client_data holds only jmpbuf.
 */
static void
jpeg_error_catch_all_1(j_common_ptr cinfo)
{
    jmp_buf *pjmpbuf = (jmp_buf *)cinfo->client_data;
    (*cinfo->err->output_message) (cinfo);
    jpeg_destroy(cinfo);
    longjmp(*pjmpbuf, 1);
    return;
}

/*!
 *  jpeg_error_catch_all_2()
 *
 *  Notes:
 *      (1) This is used where cinfo->client_data needs to hold both
 *          the jmpbuf and the jpeg comment data.
 *      (2) On error, the comment data will be freed by the caller.
 */
static void
jpeg_error_catch_all_2(j_common_ptr cinfo)
{
struct callback_data  *pcb_data;

    pcb_data = (struct callback_data *)cinfo->client_data;
    (*cinfo->err->output_message) (cinfo);
    jpeg_destroy(cinfo);
    longjmp(pcb_data->jmpbuf, 1);
    return;
}

/* This function was borrowed from libjpeg */
static l_uint8
jpeg_getc(j_decompress_ptr cinfo)
{
struct jpeg_source_mgr *datasrc;

    datasrc = cinfo->src;
    if (datasrc->bytes_in_buffer == 0) {
        if (! (*datasrc->fill_input_buffer) (cinfo)) {
            return 0;
        }
    }
    datasrc->bytes_in_buffer--;
    return GETJOCTET(*datasrc->next_input_byte++);
}

/*!
 *  jpeg_comment_callback()
 *
 *  Notes:
 *      (1) This is used to read the jpeg comment (JPEG_COM).
 *          See the note above the declaration for why it returns
 *          a "boolean".
 */
static boolean
jpeg_comment_callback(j_decompress_ptr cinfo)
{
l_int32                length, i;
l_uint8               *comment;
struct callback_data  *pcb_data;

        /* Get the size of the comment */
    length = jpeg_getc(cinfo) << 8;
    length += jpeg_getc(cinfo);
    length -= 2;
    if (length <= 0)
        return 1;

        /* Extract the comment from the file */
    if ((comment = (l_uint8 *)CALLOC(length + 1, sizeof(l_uint8))) == NULL)
        return 0;
    for (i = 0; i < length; i++)
        comment[i] = jpeg_getc(cinfo);

        /* Save the comment and return */
    pcb_data = (struct callback_data *)cinfo->client_data;
    pcb_data->comment = comment;
    return 1;
}


/*---------------------------------------------------------------------*
 *       Extraction of jpeg header info by parsing  -- deprecated      *
 *---------------------------------------------------------------------*/
/*!
 *  extractJpegDataFromFile()
 *
 *      Input:  filein
 *              &data (<optional return> binary jpeg compressed file data)
 *              &nbytes (<optional return> size of binary jpeg data)
 *              &w (<optional return> image width)
 *              &h (<optional return> image height)
 *              &bps (<optional return> bits/sample; should be 8)
 *              &spp (<optional return> samples/pixel; should be 1 or 3)
 *      Return: 0 if OK, 1 on error
 */
l_int32
extractJpegDataFromFile(const char  *filein,
                        l_uint8    **pdata,
                        size_t      *pnbytes,
                        l_int32     *pw,
                        l_int32     *ph,
                        l_int32     *pbps,
                        l_int32     *pspp)
{
l_uint8  *data;
l_int32   format, ret;
size_t    nbytes;

    PROCNAME("extractJpegDataFromFile");

    if (pdata) *pdata = NULL;
    if (pnbytes) *pnbytes = 0;
    if (!pw && !ph && !pbps && !pspp)
        return ERROR_INT("no output data requested", procName, 1);
    if (pw) *pw = 0;
    if (ph) *pw = 0;
    if (pbps) *pbps = 0;
    if (pspp) *pspp = 0;
    if (!filein)
        return ERROR_INT("filein not defined", procName, 1);

    findFileFormat(filein, &format);
    if (format != IFF_JFIF_JPEG)
        return ERROR_INT("filein not jfif jpeg", procName, 1);

    if ((data = l_binaryRead(filein, &nbytes)) == NULL)
        return ERROR_INT("inarray not made", procName, 1);

        /* On error, free the data */
    ret = extractJpegDataFromArray(data, nbytes, pw, ph, pbps, pspp);
    if (ret) {
        FREE(data);
    } else {
        if (pnbytes) *pnbytes = nbytes;
        if (pdata)
            *pdata = data;
        else
            FREE(data);
    }
    return ret;
}


/*!
 *  extractJpegDataFromArray()
 *
 *      Input:  data (binary data consisting of the entire jpeg file)
 *              nbytes (size of binary data)
 *              &w (<optional return> image width)
 *              &h (<optional return> image height)
 *              &bps (<optional return> bits/sample; should be 8)
 *              &spp (<optional return> samples/pixel; should be 1, 3 or 4)
 *      Return: 0 if OK, 1 on error
 */
l_int32
extractJpegDataFromArray(const void  *data,
                         size_t       nbytes,
                         l_int32     *pw,
                         l_int32     *ph,
                         l_int32     *pbps,
                         l_int32     *pspp)
{
l_uint8  *data8;
l_int32   imeta, msize, bps, w, h, spp;

    PROCNAME("extractJpegDataFromArray");

    if (!pw && !ph && !pbps && !pspp)
        return ERROR_INT("no output data requested", procName, 1);
    if (pw) *pw = 0;
    if (ph) *pw = 0;
    if (pbps) *pbps = 0;
    if (pspp) *pspp = 0;
    if (!data)
        return ERROR_INT("data not defined", procName, 1);
    data8 = (l_uint8 *)data;

        /* Find where the image metadata begins in header:
         * 0xc0 is start of metadata for baseline DCT;
         * 0xc1 is start of metadata for extended sequential DCT;
         * ...   */
    imeta = 0;
    if (locateJpegImageParameters(data8, nbytes, &imeta) == 0) {
            /* Save the metadata */
        msize = getTwoByteParameter(data8, imeta);   /* metadata size */
        bps = data8[imeta + 2];
        h = getTwoByteParameter(data8, imeta + 3);
        w = getTwoByteParameter(data8, imeta + 5);
        spp = data8[imeta + 7];
        if (pbps) *pbps = bps;
        if (ph) *ph = h;
        if (pw) *pw = w;
        if (pspp) *pspp = spp;

#if  DEBUG_INFO
        fprintf(stderr, "w = %d, h = %d, bps = %d, spp = %d\n", w, h, bps, spp);
        fprintf(stderr, "imeta = %d, msize = %d\n", imeta, msize);
#endif   /* DEBUG_INFO */

            /* Is the data obviously bad? */
        if (h <= 0 || w <= 0 || bps != 8 || (spp != 1 && spp !=3 && spp != 4)) {
            L_WARNING("invalid image parameters:\n", procName);
            L_WARNING("fallback to read the entire file\n", procName);
            return extractJpegHeaderDataFallback(data, nbytes,
                                                 pw, ph, pbps, pspp);
        }
    } else {
        L_WARNING("parsing failure; fallback to read entire file\n", procName);
        return extractJpegHeaderDataFallback(data, nbytes, pw, ph, pbps, pspp);
    }

    return 0;
}


/*!
 *  extractJpegHeaderDataFallback()
 *
 *      Input:  data (binary data consisting of the entire jpeg file)
 *              nbytes (size of binary data)
 *              &w (<optional return> image width)
 *              &h (<optional return> image height)
 *              &bps (<optional return> bits/sample; should be 8)
 *              &spp (<optional return> samples/pixel; should be 1 or 3)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This gets the header data by uncompressing the jpeg
 *          data into a pix.  It can be used when header parsing fails.
 *      (2) This cannot distinguish between 3 and 4 spp, so it returns 3.
 */
static l_int32
extractJpegHeaderDataFallback(const void  *data,
                              size_t       nbytes,
                              l_int32     *pw,
                              l_int32     *ph,
                              l_int32     *pbps,
                              l_int32     *pspp)
{
l_uint8  *data8;
l_int32   w, h, d, spp;
PIX      *pix;

    PROCNAME("extractJpegHeaderDataFallback");

    if (!pw && !ph && !pbps && !pspp)
        return ERROR_INT("no output data requested", procName, 1);
    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (pbps) *pbps = 8;
    if (pspp) *pspp = 0;
    if (!data)
        return ERROR_INT("data not defined", procName, 1);
    data8 = (l_uint8 *)data;

    if ((pix = pixReadMemJpeg(data8, nbytes, 0, 1, NULL, 0)) == NULL)
        return ERROR_INT("unable to read jpeg", procName, 1);
    pixGetDimensions(pix, &w, &h, &d);
    spp = (d == 8) ? 1 : 3;
    if (ph) *ph = h;
    if (pw) *pw = w;
    if (pspp) *pspp = spp;
    pixDestroy(&pix);
    return 0;
}


/*
 *  locateJpegImageParameters()
 *
 *      Input:  inarray (binary jpeg)
 *              size (of the data array)
 *              &index (<return> location of image metadata)
 *      Return: 0 if OK, 1 on error.  Caller must check this!
 *
 *  Notes:
 *      (1) The metadata in jpeg files is a mess.  There are markers
 *          for the chunks that are always preceeded by 0xff.
 *          It is possible to have 0xff in the binary data that is
 *          not a marker, and this is always 'escaped' by a following
 *          0x0 byte.  The two bytes following the marker give the
 *          chunk size, inclusive of those two bytes.  The jpeg parser
 *          runs through the file, looking for special markers such
 *          as 0xc0 and 0xc2 that indicate the beginning of a metadata
 *          frame that gives the image size, depth, etc.
 *      (2) The markers listed here appear to be the only ones that
 *          we need to worry about.  It would have been nice to have
 *          avoided the switch with all these markers, but
 *          unfortunately the parser for the jpeg header is set
 *          to accept any byte marker that's not on the approved list!
 *          So we have to look for a flag that's not on the list
 *          (and is not 0 or followed by 0xff), and then interpret
 *          the size of the data chunk and skip it.  Why do this?
 *          Such a chunk may contain a thumbnail version of the image,
 *          so if we don't skip it, we will find a pair of bytes such
 *          as 0xffc0 within the chunk, followed by the metadata
 *          (e.g., w and h dimensions) for the thumbnail.  Not what we want.
 *      (3) There exist jpeg files with the sequence 0xffXXff, where XX
 *          is apparently a random marker not on the 'approved' list.
 *          These clearly need to be escaped, because there are no
 *          chunks of size as great as 0xff00 that can be skipped
 *          (remember: for chunks that must be skipped, the 2 bytes
 *          after the marker give the chunk size).
 *      (4) For marker definitions, see, e.g.:
 *               http://www.digicamsoft.com/itu/itu-t81-36.html
 */
static l_int32
locateJpegImageParameters(l_uint8  *inarray,
                          size_t    size,
                          l_int32  *pindex)
{
l_uint8  val;
l_int32  index, skiplength;

    PROCNAME("locateJpegImageParameters");

    if (!inarray)
        return ERROR_INT("inarray not defined", procName, 1);
    if (!pindex)
        return ERROR_INT("&index not defined", procName, 1);

    index = 0;  /* start at the beginning of the data */
    while (1) {
        if (getNextJpegMarker(inarray, size, &index))
            break;
        if ((val = inarray[index]) == 0)  /* ignore if "escaped" */
            continue;
        if (inarray[index + 1] == 0xff)  /* ignore if 'ff' immediately after */
            continue;
/*        fprintf(stderr, " marker %x at %o, %d\n", val, index, index); */
        switch(val) {
            /* These are valid metadata start of frame locations */
        case 0xc0:  /* M_SOF0 */
        case 0xc1:  /* M_SOF1 */
        case 0xc2:  /* M_SOF2 */
        case 0xc3:  /* M_SOF3 */
        case 0xc5:  /* M_SOF5 */
        case 0xc6:  /* M_SOF6 */
        case 0xc7:  /* M_SOF7 */
        case 0xc9:  /* M_SOF9 */
        case 0xca:  /* M_SOF10 */
        case 0xcd:  /* M_SOF13 */
        case 0xce:  /* M_SOF14 */
        case 0xcf:  /* M_SOF15 */
            *pindex = index + 1;  /* found it */
            return 0;

            /* Go on -- these are on the 'approved' list and are
             * not chunks that must be skipped */
        case 0x01:  /* M_TEM */
        case 0xd0:  /* M_RST0 */
        case 0xd1:  /* M_RST1 */
        case 0xd2:  /* M_RST2 */
        case 0xd3:  /* M_RST3 */
        case 0xd4:  /* M_RST4 */
        case 0xd5:  /* M_RST5 */
        case 0xd6:  /* M_RST6 */
        case 0xd7:  /* M_RST7 */
        case 0xd8:  /* M_SOI */
        case 0xd9:  /* M_EOI */
        case 0xe0:  /* M_APP0 */
        case 0xee:  /* M_APP14 */
            break;

            /* Everything else is assumed to be a chunk that must be skipped */
        default:
            skiplength = getTwoByteParameter(inarray, index + 1);
/*            fprintf(stderr, "  skipping: %d bytes at %d\n",
                    skiplength, index);  */
            index += skiplength;
            break;
        }
    }

    return 1;  /* not found */
}


/*
 *  getNextJpegMarker()
 *
 *      Input:  array (jpeg data)
 *              size (from current point to the end)
 *              &index (input current and <return> the last position searched.
 *                      If it is not at the end of the array, we return
 *                      the first byte that is not 0xff, after
 *                      having encountered at least one 0xff.)
 *      Return: 0 if a marker is found, 1 if the end of the array is reached
 *
 *  Notes:
 *      (1) In jpeg, 0xff is used to mark the end of a data segment.
 *          There may be more than one 0xff in succession.  But not every
 *          0xff marks the end of a segment.  It is possible, though
 *          rare, that 0xff can occur within some data.  In that case,
 *          the marker is "escaped", by following it with 0x00.
 *      (2) This function parses a jpeg data stream.  It doesn't
 *          _really_ get the next marker, because it doesn't check if
 *          the 0xff is escaped.  But the caller checks for this escape
 *          condition, and ignores the marker if escaped.
 */
static l_int32
getNextJpegMarker(l_uint8  *array,
                  size_t    size,
                  l_int32  *pindex)
{
l_uint8  val;
l_int32  index;

    PROCNAME("getNextJpegMarker");

    if (!array)
        return ERROR_INT("array not defined", procName, 1);
    if (!pindex)
        return ERROR_INT("&index not defined", procName, 1);

    index = *pindex;  /* initial location in array */

    while (index < size) {  /* skip to 0xff */
       val = array[index++];
       if (val == 0xff)
           break;
    }

    while (index < size) {  /* skip repeated 0xff */
       val = array[index++];
       if (val != 0xff)
           break;
    }

    *pindex = index - 1;
    if (index >= size)
        return 1;
    else
        return 0;
}


static l_int32
getTwoByteParameter(l_uint8  *array,
                    l_int32   index)
{
    return (l_int32)((array[index]) << 8) + (l_int32)(array[index + 1]);
}


/* --------------------------------------------*/
#endif  /* HAVE_LIBJPEG */
/* --------------------------------------------*/
