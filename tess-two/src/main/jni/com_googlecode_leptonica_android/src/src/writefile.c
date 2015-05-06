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
 * writefile.c
 *
 *     High-level procedures for writing images to file:
 *        l_int32     pixaWriteFiles()
 *        l_int32     pixWrite()    [behavior depends on WRITE_AS_NAMED]
 *        l_int32     pixWriteStream()
 *        l_int32     pixWriteImpliedFormat()
 *        l_int32     pixWriteTempfile()
 *
 *     Selection of output format if default is requested
 *        l_int32     pixChooseOutputFormat()
 *        l_int32     getImpliedFileFormat()
 *        const char *getFormatExtension()
 *
 *     Write to memory
 *        l_int32     pixWriteMem()
 *
 *     Image display for debugging
 *        l_int32     pixDisplay()
 *        l_int32     pixDisplayWithTitle()
 *        l_int32     pixDisplayMultiple()
 *        l_int32     pixDisplayWrite()
 *        l_int32     pixDisplayWriteFormat()
 *        l_int32     pixSaveTiled()
 *        l_int32     pixSaveTiledOutline()
 *        l_int32     pixSaveTiledWithText()
 *        void        l_chooseDisplayProg()
 *
 *  Supported file formats:
 *  (1) Writing is supported without any external libraries:
 *          bmp
 *          pnm   (including pbm, pgm, etc)
 *          spix  (raw serialized)
 *  (2) Writing is supported with installation of external libraries:
 *          png
 *          jpg   (standard jfif version)
 *          tiff  (including most varieties of compression)
 *          gif
 *          webp
 *  (3) Writing is supported through special interfaces:
 *          ps (PostScript, in psio1.c, psio2.c):
 *              level 1 (uncompressed)
 *              level 2 (g4 and dct encoding: requires tiff, jpg)
 *              level 3 (g4, dct and flate encoding: requires tiff, jpg, zlib)
 *          pdf (PDF, in pdfio.c):
 *              level 1 (g4 and dct encoding: requires tiff, jpg)
 *              level 2 (g4, dct and flate encoding: requires tiff, jpg, zlib)
 *  (4) No other output formats are supported, such as jp2 (jpeg2000)
 */

#include <string.h>
#include "allheaders.h"

    /*   Special flag for pixWrite().  The default for both unix and     */
    /*   windows is to use whatever filename is given, as opposed to     */
    /*   insuring the filename extension matches the image compression.  */
#define  WRITE_AS_NAMED    1

    /* Display program (xv, xli, xzgv, open) to be invoked by pixDisplay()  */
#ifdef _WIN32
static l_int32  var_DISPLAY_PROG = L_DISPLAY_WITH_IV;  /* default */
#elif  defined(__APPLE__)
static l_int32  var_DISPLAY_PROG = L_DISPLAY_WITH_OPEN;  /* default */
#else
static l_int32  var_DISPLAY_PROG = L_DISPLAY_WITH_XZGV;  /* default */
#endif  /* _WIN32 */

static const l_int32  L_BUF_SIZE = 512;
static const l_int32  MAX_DISPLAY_WIDTH = 1000;
static const l_int32  MAX_DISPLAY_HEIGHT = 800;
static const l_int32  MAX_SIZE_FOR_PNG = 200;

    /* PostScript output for printing */
static const l_float32  DEFAULT_SCALING = 1.0;

    /* Global array of image file format extension names.                */
    /* This is in 1-1 corrspondence with format enum in imageio.h.       */
    /* The empty string at the end represents the serialized format,     */
    /* which has no recognizable extension name, but the array must      */
    /* be padded to agree with the format enum.                          */
    /* (Note on 'const': The size of the array can't be defined 'const'  */
    /* because that makes it static.  The 'const' in the definition of   */
    /* the array refers to the strings in the array; the ptr to the      */
    /* array is not const and can be used 'extern' in other files.)      */
LEPT_DLL l_int32  NumImageFileFormatExtensions = 19;  /* array size */
LEPT_DLL const char *ImageFileFormatExtensions[] =
         {"unknown",
          "bmp",
          "jpg",
          "png",
          "tif",
          "tif",
          "tif",
          "tif",
          "tif",
          "tif",
          "tif",
          "pnm",
          "ps",
          "gif",
          "jp2",
          "webp",
          "pdf",
          "default",
          ""};

    /* Local map of image file name extension to output format */
struct ExtensionMap
{
    char     extension[8];
    l_int32  format;
};
static const struct ExtensionMap extension_map[] =
                            { { ".bmp",  IFF_BMP       },
                              { ".jpg",  IFF_JFIF_JPEG },
                              { ".jpeg", IFF_JFIF_JPEG },
                              { ".png",  IFF_PNG       },
                              { ".tif",  IFF_TIFF      },
                              { ".tiff", IFF_TIFF      },
                              { ".pnm",  IFF_PNM       },
                              { ".gif",  IFF_GIF       },
                              { ".jp2",  IFF_JP2       },
                              { ".ps",   IFF_PS        },
                              { ".pdf",  IFF_LPDF      },
                              { ".webp", IFF_WEBP      } };


/*---------------------------------------------------------------------*
 *           Top-level procedures for writing images to file           *
 *---------------------------------------------------------------------*/
/*!
 *  pixaWriteFiles()
 *
 *      Input:  rootname
 *              pixa
 *              format  (defined in imageio.h; see notes for default)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Use @format = IFF_DEFAULT to decide the output format
 *          individually for each pix.
 */
l_int32
pixaWriteFiles(const char  *rootname,
               PIXA        *pixa,
               l_int32      format)
{
char     bigbuf[L_BUF_SIZE];
l_int32  i, n, pixformat;
PIX     *pix;

    PROCNAME("pixaWriteFiles");

    if (!rootname)
        return ERROR_INT("rootname not defined", procName, 1);
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);
    if (format < 0 || format == IFF_UNKNOWN ||
        format >= NumImageFileFormatExtensions)
        return ERROR_INT("invalid format", procName, 1);

    n = pixaGetCount(pixa);
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
        if (format == IFF_DEFAULT)
            pixformat = pixChooseOutputFormat(pix);
        else
            pixformat = format;
        snprintf(bigbuf, L_BUF_SIZE, "%s%03d.%s", rootname, i,
                 ImageFileFormatExtensions[pixformat]);
        pixWrite(bigbuf, pix, pixformat);
        pixDestroy(&pix);
    }

    return 0;
}


/*!
 *  pixWrite()
 *
 *      Input:  filename
 *              pix
 *              format  (defined in imageio.h)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Open for write using binary mode (with the "b" flag)
 *          to avoid having Windows automatically translate the NL
 *          into CRLF, which corrupts image files.  On non-windows
 *          systems this flag should be ignored, per ISO C90.
 *          Thanks to Dave Bryan for pointing this out.
 *      (2) If the default image format IFF_DEFAULT is requested:
 *          use the input format if known; otherwise, use a lossless format.
 *      (3) There are two modes with respect to file naming.
 *          (a) The default code writes to @filename.
 *          (b) If WRITE_AS_NAMED is defined to 0, it's a bit fancier.
 *              Then, if @filename does not have a file extension, one is
 *              automatically appended, depending on the requested format.
 *          The original intent for providing option (b) was to insure
 *          that filenames on Windows have an extension that matches
 *          the image compression.  However, this is not the default.
 */
l_int32
pixWrite(const char  *filename,
         PIX         *pix,
         l_int32      format)
{
char  *fname;
FILE  *fp;

    PROCNAME("pixWrite");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);

    fname = genPathname(filename, NULL);

#if  WRITE_AS_NAMED  /* Default */

    if ((fp = fopenWriteStream(fname, "wb+")) == NULL) {
        FREE(fname);
        return ERROR_INT("stream not opened", procName, 1);
    }

#else  /* Add an extension to the output name if none exists */

    {l_int32  extlen;
     char    *extension, *filebuf;
        splitPathAtExtension(fname, NULL, &extension);
        extlen = strlen(extension);
        FREE(extension);
        if (extlen == 0) {
            if (format == IFF_DEFAULT || format == IFF_UNKNOWN)
                format = pixChooseOutputFormat(pix);

            filebuf = (char *)CALLOC(strlen(fname) + 10, sizeof(char));
            if (!filebuf) {
                return ERROR_INT("filebuf not made", procName, 1);
                FREE(fname);
            }
            strncpy(filebuf, fname, strlen(fname));
            strcat(filebuf, ".");
            strcat(filebuf, ImageFileFormatExtensions[format]);
        } else {
            filebuf = (char *)fname;
        }

        fp = fopenWriteStream(filebuf, "wb+");
        if (filebuf != fname)
            FREE(filebuf);
        if (fp == NULL) {
            FREE(fname);
            return ERROR_INT("stream not opened", procName, 1);
        }
    }

#endif  /* WRITE_AS_NAMED */

    FREE(fname);
    if (pixWriteStream(fp, pix, format)) {
        fclose(fp);
        return ERROR_INT("pix not written to stream", procName, 1);
    }

        /* Close the stream except if GIF under windows, because
         * EGifCloseFile() closes the windows file stream! */
    if (format != IFF_GIF)
        fclose(fp);
#ifndef _WIN32
    else  /* gif file */
        fclose(fp);
#endif  /* ! _WIN32 */

    return 0;
}


/*!
 *  pixWriteStream()
 *
 *      Input:  stream
 *              pix
 *              format
 *      Return: 0 if OK; 1 on error.
 */
l_int32
pixWriteStream(FILE    *fp,
               PIX     *pix,
               l_int32  format)
{
    PROCNAME("pixWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    if (format == IFF_DEFAULT)
        format = pixChooseOutputFormat(pix);

    switch(format)
    {
    case IFF_BMP:
        pixWriteStreamBmp(fp, pix);
        break;

    case IFF_JFIF_JPEG:   /* default quality; baseline sequential */
        return pixWriteStreamJpeg(fp, pix, 75, 0);
        break;

    case IFF_PNG:   /* no gamma value stored */
        return pixWriteStreamPng(fp, pix, 0.0);
        break;

    case IFF_TIFF:           /* uncompressed */
    case IFF_TIFF_PACKBITS:  /* compressed, binary only */
    case IFF_TIFF_RLE:       /* compressed, binary only */
    case IFF_TIFF_G3:        /* compressed, binary only */
    case IFF_TIFF_G4:        /* compressed, binary only */
    case IFF_TIFF_LZW:       /* compressed, all depths */
    case IFF_TIFF_ZIP:       /* compressed, all depths */
        return pixWriteStreamTiff(fp, pix, format);
        break;

    case IFF_PNM:
        return pixWriteStreamPnm(fp, pix);
        break;

    case IFF_PS:
        return pixWriteStreamPS(fp, pix, NULL, 0, DEFAULT_SCALING);
        break;

    case IFF_GIF:
        return pixWriteStreamGif(fp, pix);
        break;

    case IFF_JP2:
        return pixWriteStreamJp2k(fp, pix, 34, 0, 0);
        break;

    case IFF_WEBP:
        return pixWriteStreamWebP(fp, pix, 80, 0);
        break;

    case IFF_LPDF:
        return pixWriteStreamPdf(fp, pix, 0, NULL);
        break;

    case IFF_SPIX:
        return pixWriteStreamSpix(fp, pix);
        break;

    default:
        return ERROR_INT("unknown format", procName, 1);
        break;
    }

    return 0;
}


/*!
 *  pixWriteImpliedFormat()
 *
 *      Input:  filename
 *              pix
 *              quality (iff JPEG; 1 - 100, 0 for default)
 *              progressive (iff JPEG; 0 for baseline seq., 1 for progressive)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This determines the output format from the filename extension.
 *      (2) The last two args are ignored except for requests for jpeg files.
 *      (3) The jpeg default quality is 75.
 */
l_int32
pixWriteImpliedFormat(const char  *filename,
                      PIX         *pix,
                      l_int32      quality,
                      l_int32      progressive)
{
l_int32  format;

    PROCNAME("pixWriteImpliedFormat");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

        /* Determine output format */
    format = getImpliedFileFormat(filename);
    if (format == IFF_UNKNOWN) {
        format = IFF_PNG;
    } else if (format == IFF_TIFF) {
        if (pixGetDepth(pix) == 1)
            format = IFF_TIFF_G4;
        else
#ifdef _WIN32
            format = IFF_TIFF_LZW;  /* poor compression */
#else
            format = IFF_TIFF_ZIP;  /* native windows tools can't handle this */
#endif  /* _WIN32 */
    }

    if (format == IFF_JFIF_JPEG) {
        quality = L_MIN(quality, 100);
        quality = L_MAX(quality, 0);
        if (progressive != 0 && progressive != 1) {
            progressive = 0;
            L_WARNING("invalid progressive; setting to baseline\n", procName);
        }
        if (quality == 0)
            quality = 75;
        pixWriteJpeg (filename, pix, quality, progressive);
    } else {
        pixWrite(filename, pix, format);
    }

    return 0;
}


/*!
 *  pixWriteTempfile()
 *
 *      Input:  dir (directory name; use '.' for local dir; no trailing '/')
 *              tail (<optional> tailname, including extension if any)
 *              pix
 *              format
 *              &filename (<optional> return actual filename used; use
 *                         null to skip)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This generates a temp filename, writes the pix to it,
 *          and optionally returns the temp filename.
 *      (2) If the filename is returned to a windows program from a DLL,
 *          use lept_free() to free it.
 *      (3) See genTempFilename() for details.  We omit the time and pid
 *          here.
 */
l_int32
pixWriteTempfile(const char  *dir,
                 const char  *tail,
                 PIX         *pix,
                 l_int32      format,
                 char       **pfilename)
{
char    *filename;
l_int32  ret;

    PROCNAME("pixWriteTempfile");

    if (!dir)
        return ERROR_INT("filename not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    if ((filename = genTempFilename(dir, tail, 0, 0)) == NULL)
        return ERROR_INT("temp filename not made", procName, 1);

    ret = pixWrite(filename, pix, format);
    if (pfilename)
        *pfilename = filename;
    else
        FREE(filename);

    return ret;
}


/*---------------------------------------------------------------------*
 *          Selection of output format if default is requested         *
 *---------------------------------------------------------------------*/
/*!
 *  pixChooseOutputFormat()
 *
 *      Input:  pix
 *      Return: output format, or 0 on error
 *
 *  Notes:
 *      (1) This should only be called if the requested format is IFF_DEFAULT.
 *      (2) If the pix wasn't read from a file, its input format value
 *          will be IFF_UNKNOWN, and in that case it is written out
 *          in a compressed but lossless format.
 */
l_int32
pixChooseOutputFormat(PIX  *pix)
{
l_int32  d, format;

    PROCNAME("pixChooseOutputFormat");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 0);

    d = pixGetDepth(pix);
    format = pixGetInputFormat(pix);
    if (format == IFF_UNKNOWN) {  /* output lossless */
        if (d == 1)
            format = IFF_TIFF_G4;
        else
            format = IFF_PNG;
    }

    return format;
}


/*!
 *  getImpliedFileFormat()
 *
 *      Input:  filename
 *      Return: output format, or IFF_UNKNOWN on error or invalid extension.
 *
 *  Notes:
 *      (1) This determines the output file format from the extension
 *          of the input filename.
 */
l_int32
getImpliedFileFormat(const char  *filename)
{
char    *extension;
int      i, numext;
l_int32  format = IFF_UNKNOWN;

    if (splitPathAtExtension (filename, NULL, &extension))
        return IFF_UNKNOWN;

    numext = sizeof(extension_map) / sizeof(extension_map[0]);
    for (i = 0; i < numext; i++) {
        if (!strcmp(extension, extension_map[i].extension)) {
            format = extension_map[i].format;
            break;
        }
    }

    FREE(extension);
    return format;
}


/*!
 *  getFormatExtension()
 *
 *      Input:  format (integer)
 *      Return: extension (string), or null if format is out of range
 *
 *  Notes:
 *      (1) This string is NOT owned by the caller; it is just a pointer
 *          to a global string.  Do not free it.
 */
const char *
getFormatExtension(l_int32  format)
{
    PROCNAME("getFormatExtension");

    if (format < 0 || format >= NumImageFileFormatExtensions)
        return (const char *)ERROR_PTR("invalid format", procName, NULL);

    return ImageFileFormatExtensions[format];
}


/*---------------------------------------------------------------------*
 *                            Write to memory                          *
 *---------------------------------------------------------------------*/
/*!
 *  pixWriteMem()
 *
 *      Input:  &data (<return> data of tiff compressed image)
 *              &size (<return> size of returned data)
 *              pix
 *              format  (defined in imageio.h)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) On windows, this will only write tiff and PostScript to memory.
 *          For other formats, it requires open_memstream(3).
 *      (2) PostScript output is uncompressed, in hex ascii.
 *          Most printers support level 2 compression (tiff_g4 for 1 bpp,
 *          jpeg for 8 and 32 bpp).
 */
l_int32
pixWriteMem(l_uint8  **pdata,
            size_t    *psize,
            PIX       *pix,
            l_int32    format)
{
l_int32  ret;

    PROCNAME("pixWriteMem");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1 );
    if (!psize)
        return ERROR_INT("&size not defined", procName, 1 );
    if (!pix)
        return ERROR_INT("&pix not defined", procName, 1 );

    if (format == IFF_DEFAULT)
        format = pixChooseOutputFormat(pix);

    switch(format)
    {
    case IFF_BMP:
        ret = pixWriteMemBmp(pdata, psize, pix);
        break;

    case IFF_JFIF_JPEG:   /* default quality; baseline sequential */
        ret = pixWriteMemJpeg(pdata, psize, pix, 75, 0);
        break;

    case IFF_PNG:   /* no gamma value stored */
        ret = pixWriteMemPng(pdata, psize, pix, 0.0);
        break;

    case IFF_TIFF:           /* uncompressed */
    case IFF_TIFF_PACKBITS:  /* compressed, binary only */
    case IFF_TIFF_RLE:       /* compressed, binary only */
    case IFF_TIFF_G3:        /* compressed, binary only */
    case IFF_TIFF_G4:        /* compressed, binary only */
    case IFF_TIFF_LZW:       /* compressed, all depths */
    case IFF_TIFF_ZIP:       /* compressed, all depths */
        ret = pixWriteMemTiff(pdata, psize, pix, format);
        break;

    case IFF_PNM:
        ret = pixWriteMemPnm(pdata, psize, pix);
        break;

    case IFF_PS:
        ret = pixWriteMemPS(pdata, psize, pix, NULL, 0, DEFAULT_SCALING);
        break;

    case IFF_GIF:
        ret = pixWriteMemGif(pdata, psize, pix);
        break;

    case IFF_JP2:
        ret = pixWriteMemJp2k(pdata, psize, pix, 34, 0, 0);
        break;

    case IFF_WEBP:
        ret = pixWriteMemWebP(pdata, psize, pix, 80, 0);
        break;

    case IFF_LPDF:
        ret = pixWriteMemPdf(pdata, psize, pix, 0, NULL);
        break;

    case IFF_SPIX:
        ret = pixWriteMemSpix(pdata, psize, pix);
        break;

    default:
        return ERROR_INT("unknown format", procName, 1);
        break;
    }

    return ret;
}


/*---------------------------------------------------------------------*
 *                       Image display for debugging                   *
 *---------------------------------------------------------------------*/
/*!
 *  pixDisplay()
 *
 *      Input:  pix (1, 2, 4, 8, 16, 32 bpp)
 *              x, y  (location of display frame on the screen)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This displays the image using xzgv, xli or xv on Unix,
 *          or i_view on Windows.  The display program must be on
 *          your $PATH variable.  It is chosen by setting the global
 *          var_DISPLAY_PROG, using l_chooseDisplayProg().
 *          Default on Unix is xzgv.
 *      (2) Images with dimensions larger than MAX_DISPLAY_WIDTH or
 *          MAX_DISPLAY_HEIGHT are downscaled to fit those constraints.
 *          This is particulary important for displaying 1 bpp images
 *          with xv, because xv automatically downscales large images
 *          by subsampling, which looks poor.  For 1 bpp, we use
 *          scale-to-gray to get decent-looking anti-aliased images.
 *          In all cases, we write a temporary file to /tmp, that is
 *          read by the display program.
 *      (3) For spp == 4, we call pixDisplayLayersRGBA() to show 3
 *          versions of the image: the image with a fully opaque
 *          alpha, the alpha, and the image as it would appear with
 *          a white background.
 *      (4) Note: this function uses a static internal variable to number
 *          output files written by a single process.  Behavior with a
 *          shared library may be unpredictable.
 */
l_int32
pixDisplay(PIX     *pixs,
           l_int32  x,
           l_int32  y)
{
    return pixDisplayWithTitle(pixs, x, y, NULL, 1);
}


/*!
 *  pixDisplayWithTitle()
 *
 *      Input:  pix (1, 2, 4, 8, 16, 32 bpp)
 *              x, y  (location of display frame)
 *              title (<optional> on frame; can be NULL);
 *              dispflag (1 to write, else disabled)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) See notes for pixDisplay().
 *      (2) This displays the image if dispflag == 1.
 */
l_int32
pixDisplayWithTitle(PIX         *pixs,
                    l_int32      x,
                    l_int32      y,
                    const char  *title,
                    l_int32      dispflag)
{
char           *tempname;
char            buffer[L_BUF_SIZE];
static l_int32  index = 0;  /* caution: not .so or thread safe */
l_int32         w, h, d, spp, maxheight, opaque, threeviews, ignore;
l_float32       ratw, rath, ratmin;
PIX            *pix0, *pix1, *pix2;
PIXCMAP        *cmap;
#ifndef _WIN32
l_int32         wt, ht;
#else
char           *pathname;
char            fullpath[_MAX_PATH];
#endif  /* _WIN32 */

    PROCNAME("pixDisplayWithTitle");

    if (dispflag != 1) return 0;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (var_DISPLAY_PROG != L_DISPLAY_WITH_XZGV &&
        var_DISPLAY_PROG != L_DISPLAY_WITH_XLI &&
        var_DISPLAY_PROG != L_DISPLAY_WITH_XV &&
        var_DISPLAY_PROG != L_DISPLAY_WITH_IV &&
        var_DISPLAY_PROG != L_DISPLAY_WITH_OPEN) {
        return ERROR_INT("no program chosen for display", procName, 1);
    }

        /* Display with three views if either spp = 4 or if colormapped
         * and the alpha component is not fully opaque */
    opaque = TRUE;
    if ((cmap = pixGetColormap(pixs)) != NULL)
        pixcmapIsOpaque(cmap, &opaque);
    spp = pixGetSpp(pixs);
    threeviews = (spp == 4 || !opaque) ? TRUE : FALSE;

        /* If colormapped and not opaque, remove the colormap to RGBA */
    if (!opaque)
        pix0 = pixRemoveColormap(pixs, REMOVE_CMAP_WITH_ALPHA);
    else
        pix0 = pixClone(pixs);

        /* Scale if necessary; this will also remove a colormap */
    pixGetDimensions(pix0, &w, &h, &d);
    maxheight = (threeviews) ? MAX_DISPLAY_HEIGHT / 3 : MAX_DISPLAY_HEIGHT;
    if (w <= MAX_DISPLAY_WIDTH && h <= maxheight) {
        if (d == 16)  /* take MSB */
            pix1 = pixConvert16To8(pix0, 1);
        else
            pix1 = pixClone(pix0);
    } else {
        ratw = (l_float32)MAX_DISPLAY_WIDTH / (l_float32)w;
        rath = (l_float32)maxheight / (l_float32)h;
        ratmin = L_MIN(ratw, rath);
        if (ratmin < 0.125 && d == 1)
            pix1 = pixScaleToGray8(pix0);
        else if (ratmin < 0.25 && d == 1)
            pix1 = pixScaleToGray4(pix0);
        else if (ratmin < 0.33 && d == 1)
            pix1 = pixScaleToGray3(pix0);
        else if (ratmin < 0.5 && d == 1)
            pix1 = pixScaleToGray2(pix0);
        else
            pix1 = pixScale(pix0, ratmin, ratmin);
    }
    pixDestroy(&pix0);
    if (!pix1)
        return ERROR_INT("pix1 not made", procName, 1);

        /* Generate the three views if required */
    if (threeviews)
        pix2 = pixDisplayLayersRGBA(pix1, 0xffffff00, 0);
    else
        pix2 = pixClone(pix1);

    if (index == 0) {
        lept_rmdir("disp");
        lept_mkdir("disp");
    }

    index++;
    if (pixGetDepth(pix2) < 8 ||
        (w < MAX_SIZE_FOR_PNG && h < MAX_SIZE_FOR_PNG)) {
        snprintf(buffer, L_BUF_SIZE, "/tmp/disp/write.%03d.png", index);
        pixWrite(buffer, pix2, IFF_PNG);
    } else {
        snprintf(buffer, L_BUF_SIZE, "/tmp/disp/write.%03d.jpg", index);
        pixWrite(buffer, pix2, IFF_JFIF_JPEG);
    }
    tempname = genPathname(buffer, NULL);

#ifndef _WIN32

        /* Unix */
    if (var_DISPLAY_PROG == L_DISPLAY_WITH_XZGV) {
            /* no way to display title */
        pixGetDimensions(pix2, &wt, &ht, NULL);
        snprintf(buffer, L_BUF_SIZE,
                 "xzgv --geometry %dx%d+%d+%d %s &", wt + 10, ht + 10,
                 x, y, tempname);
    } else if (var_DISPLAY_PROG == L_DISPLAY_WITH_XLI) {
        if (title) {
            snprintf(buffer, L_BUF_SIZE,
               "xli -dispgamma 1.0 -quiet -geometry +%d+%d -title \"%s\" %s &",
               x, y, title, tempname);
        } else {
            snprintf(buffer, L_BUF_SIZE,
               "xli -dispgamma 1.0 -quiet -geometry +%d+%d %s &",
               x, y, tempname);
        }
    } else if (var_DISPLAY_PROG == L_DISPLAY_WITH_XV) {
        if (title) {
            snprintf(buffer, L_BUF_SIZE,
                     "xv -quit -geometry +%d+%d -name \"%s\" %s &",
                     x, y, title, tempname);
        } else {
            snprintf(buffer, L_BUF_SIZE,
                     "xv -quit -geometry +%d+%d %s &", x, y, tempname);
        }
    } else if (var_DISPLAY_PROG == L_DISPLAY_WITH_OPEN) {
        snprintf(buffer, L_BUF_SIZE, "open %s &", tempname);
    }
    ignore = system(buffer);

#else  /* _WIN32 */

        /* Windows: L_DISPLAY_WITH_IV */
    pathname = genPathname(tempname, NULL);
    _fullpath(fullpath, pathname, sizeof(fullpath));
    if (title) {
        snprintf(buffer, L_BUF_SIZE,
                 "i_view32.exe \"%s\" /pos=(%d,%d) /title=\"%s\"",
                 fullpath, x, y, title);
    } else {
        snprintf(buffer, L_BUF_SIZE, "i_view32.exe \"%s\" /pos=(%d,%d)",
                 fullpath, x, y);
    }
    ignore = system(buffer);
    FREE(pathname);

#endif  /* _WIN32 */

    pixDestroy(&pix1);
    pixDestroy(&pix2);
    FREE(tempname);
    return 0;
}


/*!
 *  pixDisplayMultiple()
 *
 *      Input:  filepattern
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This allows display of multiple images using gthumb on unix
 *          and i_view32 on windows.  The @filepattern is a regular
 *          expression that is expanded by the shell.
 *      (2) _fullpath automatically changes '/' to '\' if necessary.
 */
l_int32
pixDisplayMultiple(const char  *filepattern)
{
char     buffer[L_BUF_SIZE];
l_int32  ignore;
#ifdef _WIN32
char    *pathname;
char    *dir, *tail;
char     fullpath[_MAX_PATH];
#endif  /* _WIN32 */

    PROCNAME("pixDisplayMultiple");

    if (!filepattern || strlen(filepattern) == 0)
        return ERROR_INT("filepattern not defined", procName, 1);

#ifndef _WIN32
    snprintf(buffer, L_BUF_SIZE, "gthumb %s &", filepattern);
#else
        /* irFanView wants absolute path for directory */
    pathname = genPathname(filepattern, NULL);
    splitPathAtDirectory(pathname, &dir, &tail);
    _fullpath(fullpath, dir, sizeof(fullpath));

    snprintf(buffer, L_BUF_SIZE,
             "i_view32.exe \"%s\" /filepattern=\"%s\" /thumbs", fullpath, tail);
    FREE(pathname);
    FREE(dir);
    FREE(tail);
#endif  /* _WIN32 */

    ignore = system(buffer);  /* gthumb || i_view32.exe */
    return 0;
}


/*!
 *  pixDisplayWrite()
 *
 *      Input:  pix (1, 2, 4, 8, 16, 32 bpp)
 *              reduction (-1 to reset/erase; 0 to disable;
 *                         otherwise this is a reduction factor)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This defaults to jpeg output for pix that are 32 bpp or
 *          8 bpp without a colormap.  If you want to write all images
 *          losslessly, use format == IFF_PNG in pixDisplayWriteFormat().
 *      (2) See pixDisplayWriteFormat() for usage details.
 */
l_int32
pixDisplayWrite(PIX     *pixs,
                l_int32  reduction)
{
    return pixDisplayWriteFormat(pixs, reduction, IFF_JFIF_JPEG);
}


/*!
 *  pixDisplayWriteFormat()
 *
 *      Input:  pix (1, 2, 4, 8, 16, 32 bpp)
 *              reduction (-1 to reset/erase; 0 to disable;
 *                         otherwise this is a reduction factor)
 *              format (IFF_PNG or IFF_JFIF_JPEG)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This writes files if reduction > 0.  These can be displayed using
 *            pixDisplayMultiple("/tmp/display/file*");
 *      (2) All previously written files can be erased by calling with
 *          reduction < 0; the value of pixs is ignored.
 *      (3) If reduction > 1 and depth == 1, this does a scale-to-gray
 *          reduction.
 *      (4) This function uses a static internal variable to number
 *          output files written by a single process.  Behavior
 *          with a shared library may be unpredictable.
 *      (5) Output file format is as follows:
 *            format == IFF_JFIF_JPEG:
 *                png if d < 8 or d == 16 or if the output pix
 *                has a colormap.   Otherwise, output is jpg.
 *            format == IFF_PNG:
 *                png (lossless) on all images.
 *      (6) For 16 bpp, the choice of full dynamic range with log scale
 *          is the best for displaying these images.  Alternative outputs are
 *             pix8 = pixMaxDynamicRange(pixt, L_LINEAR_SCALE);
 *             pix8 = pixConvert16To8(pixt, 0);  // low order byte
 *             pix8 = pixConvert16To8(pixt, 1);  // high order byte
 */
l_int32
pixDisplayWriteFormat(PIX     *pixs,
                      l_int32  reduction,
                      l_int32  format)
{
char            buf[L_BUF_SIZE];
char           *fname;
l_float32       scale;
PIX            *pixt, *pix8;
static l_int32  index = 0;  /* caution: not .so or thread safe */

    PROCNAME("pixDisplayWriteFormat");

    if (reduction == 0) return 0;

    if (reduction < 0) {
        index = 0;  /* reset; this will cause erasure at next call to write */
        return 0;
    }

    if (format != IFF_JFIF_JPEG && format != IFF_PNG)
        return ERROR_INT("invalid format", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

    if (index == 0) {
        lept_rmdir("display");
        lept_mkdir("display");
    }
    index++;

    if (reduction == 1) {
        pixt = pixClone(pixs);
    } else {
        scale = 1. / (l_float32)reduction;
        if (pixGetDepth(pixs) == 1)
            pixt = pixScaleToGray(pixs, scale);
        else
            pixt = pixScale(pixs, scale, scale);
    }

    if (pixGetDepth(pixt) == 16) {
        pix8 = pixMaxDynamicRange(pixt, L_LOG_SCALE);
        snprintf(buf, L_BUF_SIZE, "file.%03d.png", index);
        fname = genPathname("/tmp/display", buf);
        pixWrite(fname, pix8, IFF_PNG);
        pixDestroy(&pix8);
    } else if (pixGetDepth(pixt) < 8 || pixGetColormap(pixt) ||
             format == IFF_PNG) {
        snprintf(buf, L_BUF_SIZE, "file.%03d.png", index);
        fname = genPathname("/tmp/display", buf);
        pixWrite(fname, pixt, IFF_PNG);
    } else {
        snprintf(buf, L_BUF_SIZE, "file.%03d.jpg", index);
        fname = genPathname("/tmp/display", buf);
        pixWrite(fname, pixt, format);
    }
    FREE(fname);
    pixDestroy(&pixt);

    return 0;
}


/*!
 *  pixSaveTiled()
 *
 *      Input:  pixs (1, 2, 4, 8, 32 bpp)
 *              pixa (the pix are accumulated here)
 *              scalefactor (0.0 to disable; otherwise this is a scale factor)
 *              newrow (0 if placed on the same row as previous; 1 otherwise)
 *              space (horizontal and vertical spacing, in pixels)
 *              dp (depth of pixa; 8 or 32 bpp; only used on first call)
 *      Return: 0 if OK, 1 on error.
 */
l_int32
pixSaveTiled(PIX       *pixs,
             PIXA      *pixa,
             l_float32  scalefactor,
             l_int32    newrow,
             l_int32    space,
             l_int32    dp)
{
        /* Save without an outline */
    return pixSaveTiledOutline(pixs, pixa, scalefactor, newrow, space, 0, dp);
}


/*!
 *  pixSaveTiledOutline()
 *
 *      Input:  pixs (1, 2, 4, 8, 32 bpp)
 *              pixa (the pix are accumulated here)
 *              scalefactor (0.0 to disable; otherwise this is a scale factor)
 *              newrow (0 if placed on the same row as previous; 1 otherwise)
 *              space (horizontal and vertical spacing, in pixels)
 *              linewidth (width of added outline for image; 0 for no outline)
 *              dp (depth of pixa; 8 or 32 bpp; only used on first call)
 *      Return: 0 if OK, 1 on error.
 *
 *  Notes:
 *      (1) Before calling this function for the first time, use
 *          pixaCreate() to make the @pixa that will accumulate the pix.
 *          This is passed in each time pixSaveTiled() is called.
 *      (2) @scalefactor scales the input image.  After scaling and
 *          possible depth conversion, the image is saved in the input
 *          pixa, along with a box that specifies the location to
 *          place it when tiled later.  Disable saving the pix by
 *          setting @scalefactor == 0.0.
 *      (3) @newrow and @space specify the location of the new pix
 *          with respect to the last one(s) that were entered.
 *      (4) @dp specifies the depth at which all pix are saved.  It can
 *          be only 8 or 32 bpp.  Any colormap is removed.  This is only
 *          used at the first invocation.
 *      (5) This function uses two variables from call to call.
 *          If they were static, the function would not be .so or thread
 *          safe, and furthermore, there would be interference with two or
 *          more pixa accumulating images at a time.  Consequently,
 *          we use the first pix in the pixa to store and obtain both
 *          the depth and the current position of the bottom (one pixel
 *          below the lowest image raster line when laid out using
 *          the boxa).  The bottom variable is stored in the input format
 *          field, which is the only field available for storing an int.
 */
l_int32
pixSaveTiledOutline(PIX       *pixs,
                    PIXA      *pixa,
                    l_float32  scalefactor,
                    l_int32    newrow,
                    l_int32    space,
                    l_int32    linewidth,
                    l_int32    dp)
{
l_int32  n, top, left, bx, by, bw, w, h, depth, bottom;
BOX     *box;
PIX     *pix1, *pix2, *pix3, *pix4;

    PROCNAME("pixSaveTiledOutline");

    if (scalefactor == 0.0) return 0;

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    n = pixaGetCount(pixa);
    if (n == 0) {
        bottom = 0;
        if (dp != 8 && dp != 32) {
            L_WARNING("dp not 8 or 32 bpp; using 32\n", procName);
            depth = 32;
        } else {
            depth = dp;
        }
    } else {  /* extract the depth and bottom params from the first pix */
        pix1 = pixaGetPix(pixa, 0, L_CLONE);
        depth = pixGetDepth(pix1);
        bottom = pixGetInputFormat(pix1);  /* not typical usage! */
        pixDestroy(&pix1);
    }

        /* Remove colormap if it exists; otherwise a copy.  This
         * guarantees that pix4 is not a clone of pixs. */
    pix1 = pixRemoveColormapGeneral(pixs, REMOVE_CMAP_BASED_ON_SRC, L_COPY);

        /* Scale and convert to output depth */
    if (scalefactor == 1.0) {
        pix2 = pixClone(pix1);
    } else if (scalefactor > 1.0) {
        pix2 = pixScale(pix1, scalefactor, scalefactor);
    } else if (scalefactor < 1.0) {
        if (pixGetDepth(pix1) == 1)
            pix2 = pixScaleToGray(pix1, scalefactor);
        else
            pix2 = pixScale(pix1, scalefactor, scalefactor);
    }
    pixDestroy(&pix1);
    if (depth == 8)
        pix3 = pixConvertTo8(pix2, 0);
    else
        pix3 = pixConvertTo32(pix2);
    pixDestroy(&pix2);

        /* Add black outline */
    if (linewidth > 0)
        pix4 = pixAddBorder(pix3, linewidth, 0);
    else
        pix4 = pixClone(pix3);
    pixDestroy(&pix3);

        /* Find position of current pix (UL corner plus size) */
    if (n == 0) {
        top = 0;
        left = 0;
    } else if (newrow == 1) {
        top = bottom + space;
        left = 0;
    } else if (n > 0) {
        pixaGetBoxGeometry(pixa, n - 1, &bx, &by, &bw, NULL);
        top = by;
        left = bx + bw + space;
    }

    pixGetDimensions(pix4, &w, &h, NULL);
    bottom = L_MAX(bottom, top + h);
    box = boxCreate(left, top, w, h);
    pixaAddPix(pixa, pix4, L_INSERT);
    pixaAddBox(pixa, box, L_INSERT);

        /* Save the new bottom value */
    pix1 = pixaGetPix(pixa, 0, L_CLONE);
    pixSetInputFormat(pix1, bottom);  /* not typical usage! */
    pixDestroy(&pix1);
    return 0;
}


/*!
 *  pixSaveTiledWithText()
 *
 *      Input:  pixs (1, 2, 4, 8, 32 bpp)
 *              pixa (the pix are accumulated here; as 32 bpp)
 *              outwidth (in pixels; use 0 to disable entirely)
 *              newrow (1 to start a new row; 0 to go on same row as previous)
 *              space (horizontal and vertical spacing, in pixels)
 *              linewidth (width of added outline for image; 0 for no outline)
 *              bmf (<optional> font struct)
 *              textstr (<optional> text string to be added)
 *              val (color to set the text)
 *              location (L_ADD_ABOVE, L_ADD_AT_TOP, L_ADD_AT_BOT, L_ADD_BELOW)
 *      Return: 0 if OK, 1 on error.
 *
 *  Notes:
 *      (1) Before calling this function for the first time, use
 *          pixaCreate() to make the @pixa that will accumulate the pix.
 *          This is passed in each time pixSaveTiled() is called.
 *      (2) @outwidth is the scaled width.  After scaling, the image is
 *          saved in the input pixa, along with a box that specifies
 *          the location to place it when tiled later.  Disable saving
 *          the pix by setting @outwidth == 0.
 *      (3) @newrow and @space specify the location of the new pix
 *          with respect to the last one(s) that were entered.
 *      (4) All pix are saved as 32 bpp RGB.
 *      (5) If both @bmf and @textstr are defined, this generates a pix
 *          with the additional text; otherwise, no text is written.
 *      (6) The text is written before scaling, so it is properly
 *          antialiased in the scaled pix.  However, if the pix on
 *          different calls have different widths, the size of the
 *          text will vary.
 *      (7) See pixSaveTiledOutline() for other implementation details.
 */
l_int32
pixSaveTiledWithText(PIX         *pixs,
                     PIXA        *pixa,
                     l_int32      outwidth,
                     l_int32      newrow,
                     l_int32      space,
                     l_int32      linewidth,
                     L_BMF       *bmf,
                     const char  *textstr,
                     l_uint32     val,
                     l_int32      location)
{
PIX  *pix1, *pix2, *pix3, *pix4;

    PROCNAME("pixSaveTiledWithText");

    if (outwidth == 0) return 0;

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    pix1 = pixConvertTo32(pixs);
    if (linewidth > 0)
        pix2 = pixAddBorder(pix1, linewidth, 0);
    else
        pix2 = pixClone(pix1);
    if (bmf && textstr)
        pix3 = pixAddSingleTextblock(pix2, bmf, textstr, val, location, NULL);
    else
        pix3 = pixClone(pix2);
    pix4 = pixScaleToSize(pix3, outwidth, 0);
    pixSaveTiled(pix4, pixa, 1.0, newrow, space, 32);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    pixDestroy(&pix4);
    return 0;
}


void
l_chooseDisplayProg(l_int32  selection)
{
    if (selection == L_DISPLAY_WITH_XLI ||
        selection == L_DISPLAY_WITH_XZGV ||
        selection == L_DISPLAY_WITH_XV ||
        selection == L_DISPLAY_WITH_IV ||
        selection == L_DISPLAY_WITH_OPEN) {
        var_DISPLAY_PROG = selection;
    } else {
        L_ERROR("invalid display program\n", "l_chooseDisplayProg");
    }
    return;
}
