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

/*!
 * \file gifio.c
 * <pre>
 *
 *    Read gif file
 *          PIX            *pixReadStreamGif()
 *          static PIX     *gifToPix()
 *          static PIX     *pixUninterlaceGIF()
 *
 *    Write gif file
 *          l_int32         pixWriteStreamGif()
 *          static l_int32  pixToGif()
 *
 *    Read/write gif from/to memory
 *          PIX            *pixReadMemGif()
 *          static l_int32  gifReadFunc()
 *          l_int32         pixWriteMemGif()
 *          static l_int32  gifWriteFunc()
 *
 *    The initial version of this module was generously contribued by
 *    Antony Dovgal.  He can be contacted at:  tony *AT* daylessday.org
 *
 *    The functions that read and write from pix to gif-compressed memory,
 *    using gif internal functions DGifOpen() and EGifOpen() that are
 *    available in 5.1 and later, were contributed by Tobias Peirick.
 *
 *    Important version information:
 *
 *    (1) This uses the gif library, version 4.1.6 or later.
 *        Do not use 4.1.4.  It has serious problems handling 1 bpp images.
 *    (2) There are some issues with version 5.0:
 *        ~ valgrind detects uninitialized values used used for writing
 *          and conditionally jumping in EGifPutScreenDesc().  This has
 *          not been fixed as of 1/20/2016.
 *        ~ DGifSlurp() crashes on some images, apparently triggered by
 *          by some GIF extension records.  This has been fixed as of 5.1.
 *    (3) E. Raymond has changed the high-level interface with 5.0
 *        and again with 5.1, and to keep up we have used macros
 *        determined by the major and minor version numbers.  Note that
 *        tiff, jpeg, png and webp have maintained a consistent high-level
 *        interface through all versions; version-dependent code paths
 *        are not required to use them.
 *    (4) Version 5.1.2 came out on Jan 7, 2016.  Leptonica cannot
 *        successfully read gif files that it writes with this version;
 *        DGifSlurp() gets an internal error from an uninitialized array
 *        and returns failure.  E. Raymond fixed the problem for 5.1.3,
 *        and we disable leptonica with 5.1.2.
 *    (5) For Version 5.1 and later, the gif read-from-mem and write-to-mem
 *        now work without writing temporary files.
 * </pre>
 */

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

#include <string.h>
#include <sys/types.h>
#ifndef _MSC_VER
#include <unistd.h>
#else
#include <io.h>
#endif  /* _MSC_VER */
#include "allheaders.h"

/* --------------------------------------------------------------------*/
#if  HAVE_LIBGIF  || HAVE_LIBUNGIF             /* defined in environ.h */
/* --------------------------------------------------------------------*/

#include "gif_lib.h"

    /* GIF supports 4-way interlacing by raster lines.
     * Leptonica restores interlaced data to normal raster order. */
static PIX * pixUninterlaceGIF(PIX  *pixs);
static const l_int32 InterlacedOffset[] = {0, 4, 2, 1};
static const l_int32 InterlacedJumps[] = {8, 8, 4, 2};

    /* Interface that enables low-level GIF support for reading from memory */
static PIX * gifToPix(GifFileType *gif);
    /* Interface that enables low-level GIF support for writing to memory */
static l_int32 pixToGif(PIX *pix, GifFileType *gif);

    /* Basic interface changed in 5.0 (!).  We have to do this for
     * backward compatibililty with 4.1.6. */
#if GIFLIB_MAJOR < 5
#define GifMakeMapObject         MakeMapObject
#define GifFreeMapObject         FreeMapObject
#define DGifOpenFileHandle(a,b)  DGifOpenFileHandle(a)
#define EGifOpenFileHandle(a,b)  EGifOpenFileHandle(a)
#endif  /* before 5.0 */

    /* Basic interface changed again in 5.1 (!) */
#if GIFLIB_MAJOR < 5 || (GIFLIB_MAJOR == 5 && GIFLIB_MINOR == 0)
#define DGifCloseFile(a,b)       DGifCloseFile(a)
#define EGifCloseFile(a,b)       EGifCloseFile(a)
#endif  /* 4.1.6 or 5.0 */

    /*! For in-memory decoding of GIF; 5.1+ */
#if (GIFLIB_MAJOR == 5 && GIFLIB_MINOR >= 1) || GIFLIB_MAJOR > 5
typedef struct GifReadBuffer
{
    size_t            size;    /*!< size of buffer                           */
    size_t            pos;     /*!< position relative to beginning of buffer */
    const l_uint8    *cdata;   /*!< data in the buffer                       */
} GifReadBuffer;

    /*! Low-level callback for in-memory decoding */
static l_int32  gifReadFunc(GifFileType *gif, GifByteType *dest,
                            l_int32 bytesToRead);
    /*! Low-level callback for in-memory encoding */
static l_int32  gifWriteFunc(GifFileType *gif, const GifByteType *src,
                             l_int32 bytesToWrite);
#endif  /* 5.1 and beyond */


/*---------------------------------------------------------------------*
 *                       Reading gif from file                         *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixReadStreamGif()
 *
 * \param[in]    fp file stream
 * \return  pix, or NULL on error
 */
PIX *
pixReadStreamGif(FILE  *fp)
{
l_int32          fd;
GifFileType     *gif;

    PROCNAME("pixReadStreamGif");

#if GIFLIB_MAJOR == 5 && GIFLIB_MINOR == 1 && GIFLIB_RELEASE == 2  /* 5.1.2 */
    L_ERROR("Can't use giflib-5.1.2; suggest 5.1.1 or earlier\n", procName);
    return NULL;
#endif  /* 5.1.2 */

    if ((fd = fileno(fp)) < 0)
        return (PIX *)ERROR_PTR("invalid file descriptor", procName, NULL);
#ifdef _WIN32
    fd = _dup(fd);
#endif /* _WIN32 */

#ifndef _MSC_VER
    lseek(fd, 0, SEEK_SET);
#else
    _lseek(fd, 0, SEEK_SET);
#endif  /* _MSC_VER */

    if ((gif = DGifOpenFileHandle(fd, NULL)) == NULL)
        return (PIX *)ERROR_PTR("invalid file or file not found",
                                procName, NULL);

    return gifToPix(gif);
}


/*!
 * \brief   gifToPix()
 *
 * \param[in]    gif  opened gif stream
 * \return  pix, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This decodes the pix from the compressed gif stream and
 *          closes the stream.
 *      (2) It is static so that the stream is not exposed to clients.
 * </pre>
 */
static PIX *
gifToPix(GifFileType  *gif)
{
l_int32          wpl, i, j, w, h, d, cindex, ncolors;
l_int32          rval, gval, bval;
l_uint32        *data, *line;
PIX             *pixd, *pixdi;
PIXCMAP         *cmap;
ColorMapObject  *gif_cmap;
SavedImage       si;
#if (GIFLIB_MAJOR == 5 && GIFLIB_MINOR >= 1) || GIFLIB_MAJOR > 5
int              giferr;
#endif  /* 5.1 and beyond */

    PROCNAME("gifToPix");

        /* Read all the data, but use only the first image found */
    if (DGifSlurp(gif) != GIF_OK) {
        DGifCloseFile(gif, &giferr);
        return (PIX *)ERROR_PTR("failed to read GIF data", procName, NULL);
    }

    if (gif->SavedImages == NULL) {
        DGifCloseFile(gif, &giferr);
        return (PIX *)ERROR_PTR("no images found in GIF", procName, NULL);
    }

    si = gif->SavedImages[0];
    w = si.ImageDesc.Width;
    h = si.ImageDesc.Height;
    if (w <= 0 || h <= 0) {
        DGifCloseFile(gif, &giferr);
        return (PIX *)ERROR_PTR("invalid image dimensions", procName, NULL);
    }

    if (si.RasterBits == NULL) {
        DGifCloseFile(gif, &giferr);
        return (PIX *)ERROR_PTR("no raster data in GIF", procName, NULL);
    }

    if (si.ImageDesc.ColorMap) {
            /* private cmap for this image */
        gif_cmap = si.ImageDesc.ColorMap;
    } else if (gif->SColorMap) {
            /* global cmap for whole picture */
        gif_cmap = gif->SColorMap;
    } else {
            /* don't know where to take cmap from */
        DGifCloseFile(gif, &giferr);
        return (PIX *)ERROR_PTR("color map is missing", procName, NULL);
    }

    ncolors = gif_cmap->ColorCount;
    if (ncolors <= 2)
        d = 1;
    else if (ncolors <= 4)
        d = 2;
    else if (ncolors <= 16)
        d = 4;
    else
        d = 8;
    if ((cmap = pixcmapCreate(d)) == NULL) {
        DGifCloseFile(gif, &giferr);
        return (PIX *)ERROR_PTR("cmap creation failed", procName, NULL);
    }

    for (cindex = 0; cindex < ncolors; cindex++) {
        rval = gif_cmap->Colors[cindex].Red;
        gval = gif_cmap->Colors[cindex].Green;
        bval = gif_cmap->Colors[cindex].Blue;
        pixcmapAddColor(cmap, rval, gval, bval);
    }

    if ((pixd = pixCreate(w, h, d)) == NULL) {
        DGifCloseFile(gif, &giferr);
        pixcmapDestroy(&cmap);
        return (PIX *)ERROR_PTR("failed to allocate pixd", procName, NULL);
    }
    pixSetInputFormat(pixd, IFF_GIF);
    pixSetColormap(pixd, cmap);

    wpl = pixGetWpl(pixd);
    data = pixGetData(pixd);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        if (d == 1) {
            for (j = 0; j < w; j++) {
                if (si.RasterBits[i * w + j])
                    SET_DATA_BIT(line, j);
            }
        } else if (d == 2) {
            for (j = 0; j < w; j++)
                SET_DATA_DIBIT(line, j, si.RasterBits[i * w + j]);
        } else if (d == 4) {
            for (j = 0; j < w; j++)
                SET_DATA_QBIT(line, j, si.RasterBits[i * w + j]);
        } else {  /* d == 8 */
            for (j = 0; j < w; j++)
                SET_DATA_BYTE(line, j, si.RasterBits[i * w + j]);
        }
    }

        /* If the image has been interlaced (for viewing in a browser),
         * this restores the raster lines to normal order. */
    if (gif->Image.Interlace) {
        pixdi = pixUninterlaceGIF(pixd);
        pixTransferAllData(pixd, &pixdi, 0, 0);
    }

    DGifCloseFile(gif, &giferr);
    return pixd;
}


static PIX *
pixUninterlaceGIF(PIX  *pixs)
{
l_int32    w, h, d, wpl, j, k, srow, drow;
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixUninterlaceGIF");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    pixGetDimensions(pixs, &w, &h, &d);
    wpl = pixGetWpl(pixs);
    pixd = pixCreateTemplate(pixs);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    for (k = 0, srow = 0; k < 4; k++) {
        for (drow = InterlacedOffset[k]; drow < h;
             drow += InterlacedJumps[k], srow++) {
            lines = datas + srow * wpl;
            lined = datad + drow * wpl;
            for (j = 0; j < w; j++)
                memcpy(lined, lines, 4 * wpl);
        }
    }

    return pixd;
}


/*---------------------------------------------------------------------*
 *                         Writing gif to file                         *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixWriteStreamGif()
 *
 * \param[in]    fp file stream
 * \param[in]    pix 1, 2, 4, 8, 16 or 32 bpp
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) All output gif have colormaps.  If the pix is 32 bpp rgb,
 *          this quantizes the colors and writes out 8 bpp.
 *          If the pix is 16 bpp grayscale, it converts to 8 bpp first.
 *      (2) We can't write to memory using open_memstream() because
 *          the gif functions write through a file descriptor, not a
 *          file stream.
 * </pre>
 */
l_int32
pixWriteStreamGif(FILE  *fp,
                  PIX   *pix)
{
l_int32      result;
l_int32      fd;
GifFileType  *gif;
#if (GIFLIB_MAJOR == 5 && GIFLIB_MINOR >= 1) || GIFLIB_MAJOR > 5
int           giferr;
#endif  /* 5.1 and beyond */

    PROCNAME("pixWriteStreamGif");

        /* See version information at top of this file */
#if GIFLIB_MAJOR == 5 && GIFLIB_MINOR == 1 && GIFLIB_RELEASE == 2  /* 5.1.2 */
    return ERROR_INT("Can't use giflib-5.1.2; suggest 5.1.1 or earlier\n",
                     procName, 1);
#endif  /* 5.1.2 */

    if (!fp)
        return ERROR_INT("stream not open", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    rewind(fp);

    if ((fd = fileno(fp)) < 0)
        return ERROR_INT("invalid file descriptor", procName, 1);
#ifdef _WIN32
    fd = _dup(fd);
#endif /* _WIN32 */

        /* Get the gif file handle */
    if ((gif = EGifOpenFileHandle(fd, NULL)) == NULL) {
        return ERROR_INT("failed to create GIF image handle", procName, 1);
    }

    result = pixToGif(pix, gif);
    EGifCloseFile(gif, &giferr);
    return result;
}

/*!
 * \brief   pixToGif()
 *
 * \param[in]    pix 1, 2, 4, 8, 16 or 32 bpp
 * \param[in]    gif  opened gif stream
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This encodes the pix to the gif stream. The stream is not
 *          closes by this function.
 *      (2) It is static to make this function private.
 * </pre>
 */
static l_int32
pixToGif(PIX *pix, GifFileType *gif)
{
char            *text;
l_int32          wpl, i, j, w, h, d, ncolor, rval, gval, bval;
l_int32          gif_ncolor = 0;
l_uint32        *data, *line;
PIX             *pixd;
PIXCMAP         *cmap;
ColorMapObject  *gif_cmap;
GifByteType     *gif_line;
#if (GIFLIB_MAJOR == 5 && GIFLIB_MINOR >= 1) || GIFLIB_MAJOR > 5
int              giferr;
#endif  /* 5.1 and beyond */

    PROCNAME("pixToGif");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!gif)
        return ERROR_INT("gif not defined", procName, 1);

    d = pixGetDepth(pix);
    if (d == 32) {
        pixd = pixConvertRGBToColormap(pix, 1);
    } else if (d > 1) {
        pixd = pixConvertTo8(pix, TRUE);
    } else {  /* d == 1; make sure there's a colormap */
        pixd = pixClone(pix);
        if (!pixGetColormap(pixd)) {
            cmap = pixcmapCreate(1);
            pixcmapAddColor(cmap, 255, 255, 255);
            pixcmapAddColor(cmap, 0, 0, 0);
            pixSetColormap(pixd, cmap);
        }
    }

    if (!pixd)
        return ERROR_INT("failed to convert image to indexed", procName, 1);
    d = pixGetDepth(pixd);

    if ((cmap = pixGetColormap(pixd)) == NULL) {
        pixDestroy(&pixd);
        return ERROR_INT("cmap is missing", procName, 1);
    }

        /* 'Round' the number of gif colors up to a power of 2 */
    ncolor = pixcmapGetCount(cmap);
    for (i = 0; i <= 8; i++) {
        if ((1 << i) >= ncolor) {
            gif_ncolor = (1 << i);
            break;
        }
    }
    if (gif_ncolor < 1) {
        pixDestroy(&pixd);
        return ERROR_INT("number of colors is invalid", procName, 1);
    }

        /* Save the cmap colors in a gif_cmap */
    if ((gif_cmap = GifMakeMapObject(gif_ncolor, NULL)) == NULL) {
        pixDestroy(&pixd);
        return ERROR_INT("failed to create GIF color map", procName, 1);
    }
    for (i = 0; i < gif_ncolor; i++) {
        rval = gval = bval = 0;
        if (ncolor > 0) {
            if (pixcmapGetColor(cmap, i, &rval, &gval, &bval) != 0) {
                pixDestroy(&pixd);
                GifFreeMapObject(gif_cmap);
                return ERROR_INT("failed to get color from color map",
                                 procName, 1);
            }
            ncolor--;
        }
        gif_cmap->Colors[i].Red = rval;
        gif_cmap->Colors[i].Green = gval;
        gif_cmap->Colors[i].Blue = bval;
    }

    pixGetDimensions(pixd, &w, &h, NULL);
    if (EGifPutScreenDesc(gif, w, h, gif_cmap->BitsPerPixel, 0, gif_cmap)
        != GIF_OK) {
        pixDestroy(&pixd);
        GifFreeMapObject(gif_cmap);
        return ERROR_INT("failed to write screen description", procName, 1);
    }
    GifFreeMapObject(gif_cmap); /* not needed after this point */

    if (EGifPutImageDesc(gif, 0, 0, w, h, FALSE, NULL) != GIF_OK) {
        pixDestroy(&pixd);
        return ERROR_INT("failed to image screen description", procName, 1);
    }

    data = pixGetData(pixd);
    wpl = pixGetWpl(pixd);
    if (d != 1 && d != 2 && d != 4 && d != 8) {
        pixDestroy(&pixd);
        return ERROR_INT("image depth is not in {1, 2, 4, 8}", procName, 1);
    }

    if ((gif_line = (GifByteType *)LEPT_CALLOC(sizeof(GifByteType), w))
        == NULL) {
        pixDestroy(&pixd);
        return ERROR_INT("mem alloc fail for data line", procName, 1);
    }

    for (i = 0; i < h; i++) {
        line = data + i * wpl;
            /* Gif's way of setting the raster line up for compression */
        for (j = 0; j < w; j++) {
            switch(d)
            {
            case 8:
                gif_line[j] = GET_DATA_BYTE(line, j);
                break;
            case 4:
                gif_line[j] = GET_DATA_QBIT(line, j);
                break;
            case 2:
                gif_line[j] = GET_DATA_DIBIT(line, j);
                break;
            case 1:
                gif_line[j] = GET_DATA_BIT(line, j);
                break;
            }
        }

            /* Compress and save the line */
        if (EGifPutLine(gif, gif_line, w) != GIF_OK) {
            LEPT_FREE(gif_line);
            pixDestroy(&pixd);
            return ERROR_INT("failed to write data line into GIF", procName, 1);
        }
    }

        /* Write a text comment.  This must be placed after writing the
         * data (!!)  Note that because libgif does not provide a function
         * for reading comments from file, you will need another way
         * to read comments. */
    if ((text = pixGetText(pix)) != NULL) {
        if (EGifPutComment(gif, text) != GIF_OK)
            L_WARNING("gif comment not written\n", procName);
    }

    LEPT_FREE(gif_line);
    pixDestroy(&pixd);
    return 0;
}


/*---------------------------------------------------------------------*
 *                      Read/write from/to memory                      *
 *---------------------------------------------------------------------*/
/*!
 * \brief   pixReadMemGif()
 *
 * \param[in]    data const; gif-encoded
 * \param[in]    size of data
 * \return  pix, or NULL on error
 *
 * <pre>
 * Notes:
 *   * For Giflib version >= 5.1, this uses the DGifOpen() buffer
 *     interface.  No temp files are required.
 *   * For Giflib version < 5.1:
 *       (1) Write the gif compressed data to file and read it back.
 *           Note: we can't use the GNU runtime extension fmemopen()
 *           because libgif doesn't have a file stream interface.
 *       (2) This should be relatively safe from a sophisticated attack,
 *           because we use mkstemp (or its Windows equivalent) to generate
 *           a filename and link the file.  It would be nice to go further
 *           and do this:
 *               l_int32 fd = mkstemp(template);
 *               FILE *fp = fdopen(fd, "w+b");
 *               fwrite(data, 1, size, fp);
 *               rewind(fp);
 *               Pix *pix = pixReadStreamGif(fp);
 *           but this can't be done with gif files because of the way
 *           that libgif handles the file descriptors: fp is in a
 *           bad state after writing.
 * </pre>
 */
PIX *
pixReadMemGif(const l_uint8  *cdata,
              size_t          size)
{
#if (GIFLIB_MAJOR == 5 && GIFLIB_MINOR >= 1) || GIFLIB_MAJOR > 5
GifFileType   *gif;
GifReadBuffer  buffer;
#else
char          *fname;
PIX           *pix;
#endif  /* 5.1 and beyond */

    PROCNAME("pixReadMemGif");

    if (!cdata)
        return (PIX *)ERROR_PTR("cdata not defined", procName, NULL);

#if (GIFLIB_MAJOR == 5 && GIFLIB_MINOR >= 1) || GIFLIB_MAJOR > 5
    buffer.cdata = cdata;
    buffer.size = size;
    buffer.pos = 0;
    if ((gif = DGifOpen((void*)&buffer, gifReadFunc, NULL)) == NULL)
        return (PIX *)ERROR_PTR("could not open gif stream from memory",
                                procName, NULL);

    return gifToPix(gif);
#else
    L_INFO("using a temp file; not reading from memory\n", procName); 
        /* Write to a temp file */
    fname = l_makeTempFilename();
    l_binaryWrite(fname, "w", (l_uint8 *)cdata, size);

        /* Read back from the file */
    pix = pixRead(fname);
    lept_rmfile(fname);
    LEPT_FREE(fname);
    if (!pix) L_ERROR("pix not read\n", procName);
    return pix;
#endif  /* 5.1 and beyond */
}


#if (GIFLIB_MAJOR == 5 && GIFLIB_MINOR >= 1) || GIFLIB_MAJOR > 5
static l_int32
gifReadFunc(GifFileType *gif, GifByteType *dest, l_int32 bytesToRead) 
{
GifReadBuffer  *buffer;
l_int32         bytesRead;

    PROCNAME("gifReadFunc");

    if ((buffer = (GifReadBuffer*)gif->UserData) == NULL)
        return ERROR_INT("UserData not set", procName, -1);

    if(buffer->pos >= buffer->size)
        return -1;

    bytesRead = (buffer->pos < buffer->size - bytesToRead)
              ? bytesToRead : buffer->size - buffer->pos;
    memcpy(dest, buffer->cdata + buffer->pos, bytesRead);
    buffer->pos += bytesRead;
    return bytesRead;
}
#endif  /* 5.1 and beyond */


/*!
 * \brief   pixWriteMemGif()
 *
 * \param[out]   pdata data of gif compressed image
 * \param[out]   psize size of returned data
 * \param[in]    pix
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) See comments in pixReadMemGif()
 *      (2) For Giflib version >= 5.1, this uses the EGifOpen() buffer
 *          interface.  No temp files are required.
 * </pre>
 */
l_int32
pixWriteMemGif(l_uint8  **pdata,
               size_t    *psize,
               PIX       *pix)
{
#if (GIFLIB_MAJOR == 5 && GIFLIB_MINOR >= 1) || GIFLIB_MAJOR > 5
int           giferr;
l_int32       result;
GifFileType  *gif;
L_BBUFFER     *buffer;
#else
char         *fname;
#endif  /* 5.1 and beyond */

    PROCNAME("pixWriteMemGif");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1 );
    *pdata = NULL;
    if (!psize)
        return ERROR_INT("&size not defined", procName, 1 );
    *psize = 0;
    if (!pix)
        return ERROR_INT("&pix not defined", procName, 1 );

#if (GIFLIB_MAJOR == 5 && GIFLIB_MINOR >= 1) || GIFLIB_MAJOR > 5
    if((buffer = bbufferCreate(NULL, 0)) == NULL) {
        return ERROR_INT("failed to create buffer", procName, 1);
    }

    if ((gif = EGifOpen((void*)buffer, gifWriteFunc, NULL)) == NULL) {
        bbufferDestroy(&buffer);
        return ERROR_INT("failed to create GIF image handle", procName, 1);
    }

    result = pixToGif(pix, gif);
    EGifCloseFile(gif, &giferr);

    if(result == 0) {
        *pdata = bbufferDestroyAndSaveData(&buffer, psize);
    } else {
        bbufferDestroy(&buffer);
    }
    return result;
#else
    L_INFO("writing to a temp file, not directly to memory\n", procName);

        /* Write to a temp file */
    fname = l_makeTempFilename();
    pixWrite(fname, pix, IFF_GIF);

        /* Read back into memory */
    *pdata = l_binaryRead(fname, psize);
    lept_rmfile(fname);
    LEPT_FREE(fname);
    return 0;
#endif
}


#if (GIFLIB_MAJOR == 5 && GIFLIB_MINOR >= 1) || GIFLIB_MAJOR > 5
static l_int32
gifWriteFunc(GifFileType *gif, const GifByteType *src, l_int32 bytesToWrite) 
{
L_BBUFFER  *buffer;

    PROCNAME("gifWriteFunc");

    if ((buffer = (L_BBUFFER*)gif->UserData) == NULL)
        return ERROR_INT("UserData not set", procName, -1);

    if(bbufferRead(buffer, (l_uint8*)src, bytesToWrite) == 0)
        return bytesToWrite;
    return 0;
}
#endif  /* 5.1 and beyond */


/* -----------------------------------------------------------------*/
#endif    /* HAVE_LIBGIF || HAVE_LIBUNGIF  */
