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
 *  tiffio.c
 *
 *     Reading tiff:
 *             PIX       *pixReadTiff()    [ special top level ]
 *             PIX       *pixReadStreamTiff()
 *      static PIX       *pixReadFromTiffStream()
 *
 *     Writing tiff:
 *             l_int32    pixWriteTiff()   [ special top level ]
 *             l_int32    pixWriteTiffCustom()   [ special top level ]
 *             l_int32    pixWriteStreamTiff()
 *      static l_int32    pixWriteToTiffStream()
 *      static l_int32    writeCustomTiffTags()
 *
 *     Reading and writing multipage tiff
 *             PIXA       pixaReadMultipageTiff()
 *             l_int32    writeMultipageTiff()  [ special top level ]
 *             l_int32    writeMultipageTiffSA()
 *
 *     Information about tiff file
 *             l_int32    fprintTiffInfo()
 *             l_int32    tiffGetCount()
 *             l_int32    getTiffResolution()
 *      static l_int32    getTiffStreamResolution()
 *             l_int32    readHeaderTiff()
 *             l_int32    freadHeaderTiff()
 *             l_int32    readHeaderMemTiff()
 *      static l_int32    tiffReadHeaderTiff()
 *             l_int32    findTiffCompression()
 *      static l_int32    getTiffCompressedFormat()
 *
 *     Extraction of tiff g4 data:
 *             l_int32    extractG4DataFromFile()
 *
 *     Open tiff stream from file stream
 *      static TIFF      *fopenTiff()
 *
 *     Wrapper for TIFFOpen:
 *      static TIFF      *openTiff()
 *
 *     Memory I/O: reading memory --> pix and writing pix --> memory
 *             [10 static helper functions]
 *             l_int32    pixReadMemTiff();
 *             l_int32    pixWriteMemTiff();
 *             l_int32    pixWriteMemTiffCustom();
 *
 *   Note:  You should be using version 3.7.4 of libtiff to be certain
 *          that all the necessary functions are included.
 */

#include <string.h>
#include <sys/types.h>
#ifndef _MSC_VER
#include <unistd.h>
#else  /* _MSC_VER */
#include <io.h>
#define seek _seek;
#endif  /* _MSC_VER */
#include <fcntl.h>
#include "allheaders.h"

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

/* --------------------------------------------*/
#if  HAVE_LIBTIFF   /* defined in environ.h */
/* --------------------------------------------*/

#include "tiff.h"
#include "tiffio.h"

static const l_int32  DEFAULT_RESOLUTION = 300;   /* ppi */
static const l_int32  MAX_PAGES_IN_TIFF_FILE = 3000;  /* should be enough */


    /* All functions with TIFF interfaces are static. */
static PIX      *pixReadFromTiffStream(TIFF *tif);
static l_int32   getTiffStreamResolution(TIFF *tif, l_int32 *pxres,
                                         l_int32 *pyres);
static l_int32   tiffReadHeaderTiff(TIFF *tif, l_int32 *pwidth,
                                    l_int32 *pheight, l_int32 *pbps,
                                    l_int32 *pspp, l_int32 *pres,
                                    l_int32 *pcmap, l_int32 *pformat);
static l_int32   writeCustomTiffTags(TIFF *tif, NUMA *natags,
                                     SARRAY *savals, SARRAY  *satypes,
                                     NUMA *nasizes);
static l_int32   pixWriteToTiffStream(TIFF *tif, PIX *pix, l_int32 comptype,
                                      NUMA *natags, SARRAY *savals,
                                      SARRAY *satypes, NUMA *nasizes);
static TIFF     *fopenTiff(FILE *fp, const char *modestring);
static TIFF     *openTiff(const char *filename, const char *modestring);

    /* Static helper for tiff compression type */
static l_int32   getTiffCompressedFormat(l_uint16 tiffcomp);

    /* Static function for memory I/O */
static TIFF     *fopenTiffMemstream(const char *filename, const char *operation,
                                    l_uint8 **pdata, size_t *pdatasize);

    /* This structure defines a transform to be performed on a TIFF image
     * (note that the same transformation can be represented in
     * several different ways using this structure since
     * vflip + hflip + counterclockwise == clockwise). */
struct tiff_transform {
    int vflip;    /* if non-zero, image needs a vertical fip */
    int hflip;    /* if non-zero, image needs a horizontal flip */
    int rotate;   /* -1 -> counterclockwise 90-degree rotation,
                      0 -> no rotation
                      1 -> clockwise 90-degree rotation */
};

    /* This describes the transformations needed for a given orientation
     * tag.  The tag values start at 1, so you need to subtract 1 to get a
     * valid index into this array. */
static struct tiff_transform tiff_orientation_transforms[] = {
    {0, 0, 0},
    {0, 1, 0},
    {1, 1, 0},
    {1, 0, 0},
    {0, 1, -1},
    {0, 0, 1},
    {0, 1, 1},
    {0, 0, -1}
};



/*--------------------------------------------------------------*
 *                      Reading from file                       *
 *--------------------------------------------------------------*/
/*!
 *  pixReadTiff()
 *
 *      Input:  filename
 *              page number (0 based)
 *      Return: pix, or null on error
 *
 *  Notes:
 *      (1) This is a version of pixRead(), specialized for tiff
 *          files, that allows specification of the page to be returned
 */
PIX *
pixReadTiff(const char  *filename,
            l_int32      n)
{
FILE  *fp;
PIX   *pix;

    PROCNAME("pixReadTiff");

    if (!filename)
        return (PIX *)ERROR_PTR("filename not defined", procName, NULL);

    if ((fp = fopenReadStream(filename)) == NULL)
        return (PIX *)ERROR_PTR("image file not found", procName, NULL);
    if ((pix = pixReadStreamTiff(fp, n)) == NULL) {
        fclose(fp);
        return (PIX *)ERROR_PTR("pix not read", procName, NULL);
    }
    fclose(fp);

    return pix;
}


/*--------------------------------------------------------------*
 *                     Reading from stream                      *
 *--------------------------------------------------------------*/
/*!
 *  pixReadStreamTiff()
 *
 *      Input:  stream
 *              n (page number: 0 based)
 *      Return: pix, or null on error (e.g., if the page number is invalid)
 */
PIX *
pixReadStreamTiff(FILE    *fp,
                  l_int32  n)
{
l_int32  i, pagefound;
PIX     *pix;
TIFF    *tif;

    PROCNAME("pixReadStreamTiff");

    if (!fp)
        return (PIX *)ERROR_PTR("stream not defined", procName, NULL);

    if ((tif = fopenTiff(fp, "rb")) == NULL)
        return (PIX *)ERROR_PTR("tif not opened", procName, NULL);

    pagefound = FALSE;
    pix = NULL;
    for (i = 0; i < MAX_PAGES_IN_TIFF_FILE; i++) {
        if (i == n) {
            pagefound = TRUE;
            if ((pix = pixReadFromTiffStream(tif)) == NULL) {
                TIFFCleanup(tif);
                return (PIX *)ERROR_PTR("pix not read", procName, NULL);
            }
            break;
        }
        if (TIFFReadDirectory(tif) == 0)
            break;
    }

    if (pagefound == FALSE) {
        L_WARNING("tiff page %d not found\n", procName, n);
        TIFFCleanup(tif);
        return NULL;
    }

    TIFFCleanup(tif);
    return pix;
}


/*!
 *  pixReadFromTiffStream()
 *
 *      Input:  stream
 *      Return: pix, or null on error
 *
 *  Notes:
 *      (1) We handle pixels up to 32 bits.  This includes:
 *          1 spp (grayscale): 1, 2, 4, 8, 16 bpp
 *          1 spp (colormapped): 1, 2, 4, 8 bpp
 *          3 spp (color): 8 bpp
 *          We do not handle 3 spp, 16 bpp (48 bits/pixel)
 *      (2) For colormapped images, we support 8 bits/color in the palette.
 *          Tiff colormaps have 16 bits/color, and we reduce them to 8.
 *      (3) Quoting the libtiff documenation at
 *               http://libtiff.maptools.org/libtiff.html
 *          "libtiff provides a high-level interface for reading image data
 *          from a TIFF file. This interface handles the details of data
 *          organization and format for a wide variety of TIFF files;
 *          at least the large majority of those files that one would
 *          normally encounter. Image data is, by default, returned as
 *          ABGR pixels packed into 32-bit words (8 bits per sample).
 *          Rectangular rasters can be read or data can be intercepted
 *          at an intermediate level and packed into memory in a format
 *          more suitable to the application. The library handles all
 *          the details of the format of data stored on disk and,
 *          in most cases, if any colorspace conversions are required:
 *          bilevel to RGB, greyscale to RGB, CMYK to RGB, YCbCr to RGB,
 *          16-bit samples to 8-bit samples, associated/unassociated alpha,
 *          etc."
 */
static PIX *
pixReadFromTiffStream(TIFF  *tif)
{
l_uint8   *linebuf, *data;
l_uint16   spp, bps, bpp, tiffbpl, photometry, tiffcomp, orientation;
l_uint16  *redmap, *greenmap, *bluemap;
l_int32    d, wpl, bpl, comptype, i, j, ncolors, rval, gval, bval;
l_int32    xres, yres;
l_uint32   w, h, tiffword;
l_uint32  *line, *ppixel, *tiffdata;
PIX       *pix;
PIXCMAP   *cmap;

    PROCNAME("pixReadFromTiffStream");

    if (!tif)
        return (PIX *)ERROR_PTR("tif not defined", procName, NULL);

        /* Use default fields for bps and spp */
    TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &bps);
    TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
    bpp = bps * spp;
    if (bpp > 32)
        return (PIX *)ERROR_PTR("can't handle bpp > 32", procName, NULL);
    if (spp == 1)
        d = bps;
    else if (spp == 3 || spp == 4)
        d = 32;
    else
        return (PIX *)ERROR_PTR("spp not in set {1,3,4}", procName, NULL);

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
    tiffbpl = TIFFScanlineSize(tif);

    if ((pix = pixCreate(w, h, d)) == NULL)
        return (PIX *)ERROR_PTR("pix not made", procName, NULL);
    data = (l_uint8 *)pixGetData(pix);
    wpl = pixGetWpl(pix);
    bpl = 4 * wpl;

        /* Read the data */
    if (spp == 1) {
        if ((linebuf = (l_uint8 *)CALLOC(tiffbpl + 1, sizeof(l_uint8))) == NULL)
            return (PIX *)ERROR_PTR("calloc fail for linebuf", procName, NULL);

        for (i = 0 ; i < h ; i++) {
            if (TIFFReadScanline(tif, linebuf, i, 0) < 0) {
                FREE(linebuf);
                pixDestroy(&pix);
                return (PIX *)ERROR_PTR("line read fail", procName, NULL);
            }
            memcpy((char *)data, (char *)linebuf, tiffbpl);
            data += bpl;
        }
        if (bps <= 8)
            pixEndianByteSwap(pix);
        else   /* bps == 16 */
            pixEndianTwoByteSwap(pix);
        FREE(linebuf);
    }
    else {  /* rgb */
        if ((tiffdata = (l_uint32 *)CALLOC(w * h, sizeof(l_uint32))) == NULL) {
            pixDestroy(&pix);
            return (PIX *)ERROR_PTR("calloc fail for tiffdata", procName, NULL);
        }
        if (!TIFFReadRGBAImageOriented(tif, w, h, (uint32 *)tiffdata,
                                       ORIENTATION_TOPLEFT, 0)) {
            FREE(tiffdata);
            pixDestroy(&pix);
            return (PIX *)ERROR_PTR("failed to read tiffdata", procName, NULL);
        }

        line = pixGetData(pix);
        for (i = 0 ; i < h ; i++, line += wpl) {
            for (j = 0, ppixel = line; j < w; j++) {
                    /* TIFFGet* are macros */
                tiffword = tiffdata[i * w + j];
                rval = TIFFGetR(tiffword);
                gval = TIFFGetG(tiffword);
                bval = TIFFGetB(tiffword);
                composeRGBPixel(rval, gval, bval, ppixel);
                ppixel++;
            }
        }
        FREE(tiffdata);
    }

    if (getTiffStreamResolution(tif, &xres, &yres) == 0) {
        pixSetXRes(pix, xres);
        pixSetYRes(pix, yres);
    }

        /* Find and save the compression type */
    TIFFGetFieldDefaulted(tif, TIFFTAG_COMPRESSION, &tiffcomp);
    comptype = getTiffCompressedFormat(tiffcomp);
    pixSetInputFormat(pix, comptype);

    if (TIFFGetField(tif, TIFFTAG_COLORMAP, &redmap, &greenmap, &bluemap)) {
            /* Save the colormap as a pix cmap.  Because the
             * tiff colormap components are 16 bit unsigned,
             * and go from black (0) to white (0xffff), the
             * the pix cmap takes the most significant byte. */
        if (bps > 8) {
            pixDestroy(&pix);
            return (PIX *)ERROR_PTR("invalid bps; > 8", procName, NULL);
        }
        if ((cmap = pixcmapCreate(bps)) == NULL) {
            pixDestroy(&pix);
            return (PIX *)ERROR_PTR("cmap not made", procName, NULL);
        }
        ncolors = 1 << bps;
        for (i = 0; i < ncolors; i++)
            pixcmapAddColor(cmap, redmap[i] >> 8, greenmap[i] >> 8,
                            bluemap[i] >> 8);
        pixSetColormap(pix, cmap);
    } else {   /* No colormap: check photometry and invert if necessary */
        if (!TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometry)) {
                /* Guess default photometry setting.  Assume min_is_white
                 * if compressed 1 bpp; min_is_black otherwise. */
            if (tiffcomp == COMPRESSION_CCITTFAX3 ||
                tiffcomp == COMPRESSION_CCITTFAX4 ||
                tiffcomp == COMPRESSION_CCITTRLE ||
                tiffcomp == COMPRESSION_CCITTRLEW) {
                photometry = PHOTOMETRIC_MINISWHITE;
            } else {
                photometry = PHOTOMETRIC_MINISBLACK;
            }
        }
        if ((d == 1 && photometry == PHOTOMETRIC_MINISBLACK) ||
            (d == 8 && photometry == PHOTOMETRIC_MINISWHITE))
            pixInvert(pix, pix);
    }

    if (TIFFGetField(tif, TIFFTAG_ORIENTATION, &orientation)) {
        if (orientation >= 1 && orientation <= 8) {
            struct tiff_transform *transform =
              &tiff_orientation_transforms[orientation - 1];
            if (transform->vflip) pixFlipTB(pix, pix);
            if (transform->hflip) pixFlipLR(pix, pix);
            if (transform->rotate) {
                PIX *oldpix = pix;
                pix = pixRotate90(oldpix, transform->rotate);
                pixDestroy(&oldpix);
            }
        }
    }

    return pix;
}


/*--------------------------------------------------------------*
 *                       Writing to file                        *
 *--------------------------------------------------------------*/
/*!
 *  pixWriteTiff()
 *
 *      Input:  filename (to write to)
 *              pix
 *              comptype (IFF_TIFF, IFF_TIFF_RLE, IFF_TIFF_PACKBITS,
 *                        IFF_TIFF_G3, IFF_TIFF_G4,
 *                        IFF_TIFF_LZW, IFF_TIFF_ZIP)
 *              modestring ("a" or "w")
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) For multi-page tiff, write the first pix with mode "w" and
 *          all subsequent pix with mode "a".
 */
l_int32
pixWriteTiff(const char  *filename,
             PIX         *pix,
             l_int32      comptype,
             const char  *modestring)
{
    return pixWriteTiffCustom(filename, pix, comptype, modestring,
                              NULL, NULL, NULL, NULL);
}


/*!
 *  pixWriteTiffCustom()
 *
 *      Input:  filename (to write to)
 *              pix
 *              comptype (IFF_TIFF, IFF_TIFF_RLE, IFF_TIFF_PACKBITS,
 *                        IFF_TIFF_G3, IFF_TIFF_G4)
 *                        IFF_TIFF_LZW, IFF_TIFF_ZIP)
 *              modestring ("a" or "w")
 *              natags (<optional> NUMA of custom tiff tags)
 *              savals (<optional> SARRAY of values)
 *              satypes (<optional> SARRAY of types)
 *              nasizes (<optional> NUMA of sizes)
 *      Return: 0 if OK, 1 on error
 *
 *  Usage:
 *      (1) This writes a page image to a tiff file, with optional
 *          extra tags defined in tiff.h
 *      (2) For multi-page tiff, write the first pix with mode "w" and
 *          all subsequent pix with mode "a".
 *      (3) For the custom tiff tags:
 *          (a) The three arrays {natags, savals, satypes} must all be
 *              either NULL or defined and of equal size.
 *          (b) If they are defined, the tags are an array of integers,
 *              the vals are an array of values in string format, and
 *              the types are an array of types in string format.
 *          (c) All valid tags are definined in tiff.h.
 *          (d) The types allowed are the set of strings:
 *                "char*"
 *                "l_uint8*"
 *                "l_uint16"
 *                "l_uint32"
 *                "l_int32"
 *                "l_float64"
 *                "l_uint16-l_uint16" (note the dash; use it between the
 *                                    two l_uint16 vals in the val string)
 *              Of these, "char*" and "l_uint16" are the most commonly used.
 *          (e) The last array, nasizes, is also optional.  It is for
 *              tags that take an array of bytes for a value, a number of
 *              elements in the array, and a type that is either "char*"
 *              or "l_uint8*" (probably either will work).
 *              Use NULL if there are no such tags.
 *          (f) VERY IMPORTANT: if there are any tags that require the
 *              extra size value, stored in nasizes, they must be
 *              written first!
 */
l_int32
pixWriteTiffCustom(const char  *filename,
                   PIX         *pix,
                   l_int32      comptype,
                   const char  *modestring,
                   NUMA        *natags,
                   SARRAY      *savals,
                   SARRAY      *satypes,
                   NUMA        *nasizes)
{
l_int32  ret;
TIFF    *tif;

    PROCNAME("pixWriteTiffCustom");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    if ((tif = openTiff(filename, modestring)) == NULL)
        return ERROR_INT("tif not opened", procName, 1);
    ret = pixWriteToTiffStream(tif, pix, comptype, natags, savals,
                               satypes, nasizes);
    TIFFClose(tif);

    return ret;
}


/*--------------------------------------------------------------*
 *                       Writing to stream                      *
 *--------------------------------------------------------------*/
/*!
 *  pixWriteStreamTiff()
 *
 *      Input:  stream (opened for append or write)
 *              pix
 *              comptype (IFF_TIFF, IFF_TIFF_RLE, IFF_TIFF_PACKBITS,
 *                        IFF_TIFF_G3, IFF_TIFF_G4,
 *                        IFF_TIFF_LZW, IFF_TIFF_ZIP)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) For images with bpp > 1, this resets the comptype, if
 *          necessary, to write uncompressed data.
 *      (2) G3 and G4 are only defined for 1 bpp.
 *      (3) We only allow PACKBITS for bpp = 1, because for bpp > 1
 *          it typically expands images that are not synthetically generated.
 *      (4) G4 compression is typically about twice as good as G3.
 *          G4 is excellent for binary compression of text/line-art,
 *          but terrible for halftones and dithered patterns.  (In
 *          fact, G4 on halftones can give a file that is larger
 *          than uncompressed!)  If a binary image has dithered
 *          regions, it is usually better to compress with png.
 */
l_int32
pixWriteStreamTiff(FILE    *fp,
                   PIX     *pix,
                   l_int32  comptype)
{
TIFF  *tif;

    PROCNAME("pixWriteStreamTiff");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1 );
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1 );

    if (pixGetDepth(pix) != 1 && comptype != IFF_TIFF &&
        comptype != IFF_TIFF_LZW && comptype != IFF_TIFF_ZIP) {
        L_WARNING("invalid compression type for bpp > 1\n", procName);
        comptype = IFF_TIFF_ZIP;
    }

    if ((tif = fopenTiff(fp, "wb")) == NULL)
        return ERROR_INT("tif not opened", procName, 1);

    if (pixWriteToTiffStream(tif, pix, comptype, NULL, NULL, NULL, NULL)) {
        TIFFCleanup(tif);
        return ERROR_INT("tif write error", procName, 1);
    }

    TIFFCleanup(tif);
    return 0;
}


/*!
 *  pixWriteToTiffStream()
 *
 *      Input:  tif (data structure, opened to a file)
 *              pix
 *              comptype  (IFF_TIFF: for any image; no compression
 *                         IFF_TIFF_RLE, IFF_TIFF_PACKBITS: for 1 bpp only
 *                         IFF_TIFF_G4 and IFF_TIFF_G3: for 1 bpp only
 *                         IFF_TIFF_LZW, IFF_TIFF_ZIP: for any image
 *              natags (<optional> NUMA of custom tiff tags)
 *              savals (<optional> SARRAY of values)
 *              satypes (<optional> SARRAY of types)
 *              nasizes (<optional> NUMA of sizes)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This static function should only be called through higher
 *          level functions in this file; namely, pixWriteTiffCustom(),
 *          pixWriteTiff(), pixWriteStreamTiff(), pixWriteMemTiff()
 *          and pixWriteMemTiffCustom().
 *      (2) We only allow PACKBITS for bpp = 1, because for bpp > 1
 *          it typically expands images that are not synthetically generated.
 *      (3) See pixWriteTiffCustom() for details on how to use
 *          the last four parameters for customized tiff tags.
 *      (4) The only valid pixel depths in leptonica are 1, 2, 4, 8, 16
 *          and 32.  However, it is possible, and in some cases desirable,
 *          to write out a tiff file using an rgb pix that has 24 bpp.
 *          This can be created by appending the raster data for a 24 bpp
 *          image (with proper scanline padding) directly to a 24 bpp
 *          pix that was created without a data array.  See note in
 *          pixWriteStreamPng() for an example.
 */
static l_int32
pixWriteToTiffStream(TIFF    *tif,
                     PIX     *pix,
                     l_int32  comptype,
                     NUMA    *natags,
                     SARRAY  *savals,
                     SARRAY  *satypes,
                     NUMA    *nasizes)
{
l_uint8   *linebuf, *data;
l_uint16   redmap[256], greenmap[256], bluemap[256];
l_int32    w, h, d, i, j, k, wpl, bpl, tiffbpl, ncolors, cmapsize;
l_int32   *rmap, *gmap, *bmap;
l_int32    xres, yres;
l_uint32  *line, *ppixel;
PIX       *pixt;
PIXCMAP   *cmap;
char      *text;

    PROCNAME("pixWriteToTiffStream");

    if (!tif)
        return ERROR_INT("tif stream not defined", procName, 1);
    if (!pix)
        return ERROR_INT( "pix not defined", procName, 1 );

    pixGetDimensions(pix, &w, &h, &d);
    xres = pixGetXRes(pix);
    yres = pixGetYRes(pix);
    if (xres == 0) xres = DEFAULT_RESOLUTION;
    if (yres == 0) yres = DEFAULT_RESOLUTION;

        /* ------------------ Write out the header -------------  */
    TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, (l_uint32)RESUNIT_INCH);
    TIFFSetField(tif, TIFFTAG_XRESOLUTION, (l_float64)xres);
    TIFFSetField(tif, TIFFTAG_YRESOLUTION, (l_float64)yres);

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (l_uint32)w);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (l_uint32)h);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

    if ((text = pixGetText(pix)) != NULL)
        TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, text);

    if (d == 1)
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
    else if (d == 32 || d == 24) {
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,
                       (l_uint16)8, (l_uint16)8, (l_uint16)8);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (l_uint16)3);
    } else if ((cmap = pixGetColormap(pix)) == NULL) {
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    } else {  /* Save colormap in the tiff; not more than 256 colors */
        pixcmapToArrays(cmap, &rmap, &gmap, &bmap, NULL);
        ncolors = pixcmapGetCount(cmap);
        ncolors = L_MIN(256, ncolors);  /* max 256 */
        cmapsize = 1 << d;
        cmapsize = L_MIN(256, cmapsize);  /* power of 2; max 256 */
        if (ncolors > cmapsize) {
            L_WARNING("too many colors in cmap for tiff; truncating\n",
                      procName);
            ncolors = cmapsize;
        }
        for (i = 0; i < ncolors; i++) {
            redmap[i] = (rmap[i] << 8) | rmap[i];
            greenmap[i] = (gmap[i] << 8) | gmap[i];
            bluemap[i] = (bmap[i] << 8) | bmap[i];
        }
        for (i = ncolors; i < cmapsize; i++)  /* init, even though not used */
            redmap[i] = greenmap[i] = bluemap[i] = 0;
        FREE(rmap);
        FREE(gmap);
        FREE(bmap);

        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (l_uint16)1);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, (l_uint16)d);
        TIFFSetField(tif, TIFFTAG_COLORMAP, redmap, greenmap, bluemap);
    }

    if (d != 24 && d != 32) {
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, (l_uint16)d);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (l_uint16)1);
    }

    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    if (comptype == IFF_TIFF) {  /* no compression */
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    } else if (comptype == IFF_TIFF_G4) {
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX4);
    } else if (comptype == IFF_TIFF_G3) {
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX3);
    } else if (comptype == IFF_TIFF_RLE) {
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_CCITTRLE);
    } else if (comptype == IFF_TIFF_PACKBITS) {
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS);
    } else if (comptype == IFF_TIFF_LZW) {
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    } else if (comptype == IFF_TIFF_ZIP) {
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);
    } else {
        L_WARNING("unknown tiff compression; using none\n", procName);
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    }

        /* This is a no-op if arrays are NULL */
    writeCustomTiffTags(tif, natags, savals, satypes, nasizes);

        /* ------------- Write out the image data -------------  */
    tiffbpl = TIFFScanlineSize(tif);
    wpl = pixGetWpl(pix);
    bpl = 4 * wpl;
    if (tiffbpl > bpl)
        fprintf(stderr, "Big trouble: tiffbpl = %d, bpl = %d\n", tiffbpl, bpl);
    if ((linebuf = (l_uint8 *)CALLOC(1, bpl)) == NULL)
        return ERROR_INT("calloc fail for linebuf", procName, 1);

        /* Use single strip for image */
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, h);

    if (d != 24 && d != 32) {
        if (d == 16)
            pixt = pixEndianTwoByteSwapNew(pix);
        else
            pixt = pixEndianByteSwapNew(pix);
        data = (l_uint8 *)pixGetData(pixt);
        for (i = 0; i < h; i++, data += bpl) {
            memcpy((char *)linebuf, (char *)data, tiffbpl);
            if (TIFFWriteScanline(tif, linebuf, i, 0) < 0)
                break;
        }
        pixDestroy(&pixt);
    } else if (d == 24) {  /* See note 4 above: special case of 24 bpp rgb */
        for (i = 0; i < h; i++) {
            line = pixGetData(pix) + i * wpl;
            if (TIFFWriteScanline(tif, (l_uint8 *)line, i, 0) < 0)
                break;
        }
    } else {  /* standard 32 bpp rgb */
        for (i = 0; i < h; i++) {
            line = pixGetData(pix) + i * wpl;
            for (j = 0, k = 0, ppixel = line; j < w; j++) {
                linebuf[k++] = GET_DATA_BYTE(ppixel, COLOR_RED);
                linebuf[k++] = GET_DATA_BYTE(ppixel, COLOR_GREEN);
                linebuf[k++] = GET_DATA_BYTE(ppixel, COLOR_BLUE);
                ppixel++;
            }
            if (TIFFWriteScanline(tif, linebuf, i, 0) < 0)
                break;
        }
    }

/*    TIFFWriteDirectory(tif); */
    FREE(linebuf);

    return 0;
}


/*!
 *  writeCustomTiffTags()
 *
 *      Input:  tif
 *              natags (<optional> NUMA of custom tiff tags)
 *              savals (<optional> SARRAY of values)
 *              satypes (<optional> SARRAY of types)
 *              nasizes (<optional> NUMA of sizes)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This static function should be called indirectly through
 *          higher level functions, such as pixWriteTiffCustom(),
 *          which call pixWriteToTiffStream().  See details in
 *          pixWriteTiffCustom() for using the 4 input arrays.
 *      (2) This is a no-op if the first 3 arrays are all NULL.
 *      (3) Otherwise, the first 3 arrays must be defined and all
 *          of equal size.
 *      (4) The fourth array is always optional.
 *      (5) The most commonly used types are "char*" and "u_int16".
 *          See tiff.h for a full listing of the tiff tags.
 *          Note that many of these tags, in particular the bit tags,
 *          are intended to be private, and cannot be set by this function.
 *          Examples are the STRIPOFFSETS and STRIPBYTECOUNTS tags,
 *          which are bit tags that are automatically set in the header,
 *          and can be extracted using tiffdump.
 */
static l_int32
writeCustomTiffTags(TIFF    *tif,
                    NUMA    *natags,
                    SARRAY  *savals,
                    SARRAY  *satypes,
                    NUMA    *nasizes)
{
char      *sval, *type;
l_int32    i, n, ns, size, tagval, val;
l_float64  dval;
l_uint32   uval, uval2;

    PROCNAME("writeCustomTiffTags");

    if (!tif)
        return ERROR_INT("tif stream not defined", procName, 1);
    if (!natags && !savals && !satypes)
        return 0;
    if (!natags || !savals || !satypes)
        return ERROR_INT("not all arrays defined", procName, 1);
    n = numaGetCount(natags);
    if ((sarrayGetCount(savals) != n) || (sarrayGetCount(satypes) != n))
        return ERROR_INT("not all sa the same size", procName, 1);

        /* The sized arrays (4 args to TIFFSetField) are written first */
    if (nasizes) {
        ns = numaGetCount(nasizes);
        if (ns > n)
            return ERROR_INT("too many 4-arg tag calls", procName, 1);
        for (i = 0; i < ns; i++) {
            numaGetIValue(natags, i, &tagval);
            sval = sarrayGetString(savals, i, 0);
            type = sarrayGetString(satypes, i, 0);
            numaGetIValue(nasizes, i, &size);
            if (strcmp(type, "char*") && strcmp(type, "l_uint8*"))
                L_WARNING("array type not char* or l_uint8*; ignore\n",
                          procName);
            TIFFSetField(tif, tagval, size, sval);
        }
    } else {
        ns = 0;
    }

        /* The typical tags (3 args to TIFFSetField) are now written */
    for (i = ns; i < n; i++) {
        numaGetIValue(natags, i, &tagval);
        sval = sarrayGetString(savals, i, 0);
        type = sarrayGetString(satypes, i, 0);
        if (!strcmp(type, "char*")) {
            TIFFSetField(tif, tagval, sval);
        } else if (!strcmp(type, "l_uint16")) {
            if (sscanf(sval, "%u", &uval) == 1) {
                TIFFSetField(tif, tagval, (l_uint16)uval);
            } else {
                fprintf(stderr, "val %s not of type %s\n", sval, type);
                return ERROR_INT("custom tag(s) not written", procName, 1);
            }
        } else if (!strcmp(type, "l_uint32")) {
            if (sscanf(sval, "%u", &uval) == 1) {
                TIFFSetField(tif, tagval, uval);
            } else {
                fprintf(stderr, "val %s not of type %s\n", sval, type);
                return ERROR_INT("custom tag(s) not written", procName, 1);
            }
        } else if (!strcmp(type, "l_int32")) {
            if (sscanf(sval, "%d", &val) == 1) {
                TIFFSetField(tif, tagval, val);
            } else {
                fprintf(stderr, "val %s not of type %s\n", sval, type);
                return ERROR_INT("custom tag(s) not written", procName, 1);
            }
        } else if (!strcmp(type, "l_float64")) {
            if (sscanf(sval, "%lf", &dval) == 1) {
                TIFFSetField(tif, tagval, dval);
            } else {
                fprintf(stderr, "val %s not of type %s\n", sval, type);
                return ERROR_INT("custom tag(s) not written", procName, 1);
            }
        } else if (!strcmp(type, "l_uint16-l_uint16")) {
            if (sscanf(sval, "%u-%u", &uval, &uval2) == 2) {
                TIFFSetField(tif, tagval, (l_uint16)uval, (l_uint16)uval2);
            } else {
                fprintf(stderr, "val %s not of type %s\n", sval, type);
                return ERROR_INT("custom tag(s) not written", procName, 1);
            }
        } else {
            return ERROR_INT("unknown type; tag(s) not written", procName, 1);
        }
    }
    return 0;
}


/*--------------------------------------------------------------*
 *               Reading and writing multipage tiff             *
 *--------------------------------------------------------------*/
/*
 *  pixaReadMultipageTiff()
 *
 *      Input:  filename (input tiff file)
 *      Return: pixa (of page images), or null on error
 */
PIXA *
pixaReadMultipageTiff(const char  *filename)
{
l_int32  i, npages;
FILE    *fp;
PIX     *pix;
PIXA    *pixa;

    PROCNAME("pixaReadMultipageTiff");

    if (!filename)
        return (PIXA *)ERROR_PTR("filename not defined", procName, NULL);

    if ((fp = fopenReadStream(filename)) == NULL)
        return (PIXA *)ERROR_PTR("stream not opened", procName, NULL);
    if (fileFormatIsTiff(fp)) {
        tiffGetCount(fp, &npages);
        L_INFO(" Tiff: %d pages\n", procName, npages);
    } else {
        return (PIXA *)ERROR_PTR("file not tiff", procName, NULL);
    }
    fclose(fp);

    pixa = pixaCreate(npages);
    for (i = 0; i < npages; i++) {
        pix = pixReadTiff(filename, i);
        if (!pix) {
            L_WARNING("pix not read for page %d\n", procName, i);
            continue;
        }
        pixaAddPix(pixa, pix, L_INSERT);
    }

    return pixa;
}


/*
 *  writeMultipageTiff()
 *
 *      Input:  dirin (input directory)
 *              substr (<optional> substring filter on filenames; can be NULL)
 *              fileout (output ps file)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This writes a set of image files in a directory out
 *          as a multipage tiff file.  The images can be in any
 *          initial file format.
 *      (2) Images with a colormap have the colormap removed before
 *          re-encoding as tiff.
 *      (3) All images are encoded losslessly.  Those with 1 bpp are
 *          encoded 'g4'.  The rest are encoded as 'zip' (flate encoding).
 *          Because it is lossless, this is an expensive method for
 *          saving most rgb images.
 */
l_int32
writeMultipageTiff(const char  *dirin,
                   const char  *substr,
                   const char  *fileout)
{
SARRAY  *sa;

    PROCNAME("writeMultipageTiff");

    if (!dirin)
        return ERROR_INT("dirin not defined", procName, 1);
    if (!fileout)
        return ERROR_INT("fileout not defined", procName, 1);

        /* Get all filtered and sorted full pathnames. */
    sa = getSortedPathnamesInDirectory(dirin, substr, 0, 0);

        /* Generate the tiff file */
    writeMultipageTiffSA(sa, fileout);
    sarrayDestroy(&sa);
    return 0;
}


/*
 *  writeMultipageTiffSA()
 *
 *      Input:  sarray (of full path names)
 *              fileout (output ps file)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) See writeMultipageTiff()
 */
l_int32
writeMultipageTiffSA(SARRAY      *sa,
                     const char  *fileout)
{
char        *fname;
const char  *op;
l_int32      i, nfiles, firstfile, format;
PIX         *pix, *pixt;

    PROCNAME("writeMultipageTiffSA");

    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!fileout)
        return ERROR_INT("fileout not defined", procName, 1);

    nfiles = sarrayGetCount(sa);
    firstfile = TRUE;
    for (i = 0; i < nfiles; i++) {
        op = (firstfile) ? "w" : "a";
        fname = sarrayGetString(sa, i, L_NOCOPY);
        findFileFormat(fname, &format);
        if (format == IFF_UNKNOWN) {
            L_INFO("format of %s not known\n", procName, fname);
            continue;
        }

        if ((pix = pixRead(fname)) == NULL) {
            L_WARNING("pix not made for file: %s\n", procName, fname);
            continue;
        }
        if (pixGetDepth(pix) == 1) {
            pixWriteTiff(fileout, pix, IFF_TIFF_G4, op);
        } else {
            if (pixGetColormap(pix)) {
                pixt = pixRemoveColormap(pix, REMOVE_CMAP_BASED_ON_SRC);
            } else {
                pixt = pixClone(pix);
            }
            pixWriteTiff(fileout, pixt, IFF_TIFF_ZIP, op);
            pixDestroy(&pixt);
        }
        firstfile = FALSE;
        pixDestroy(&pix);
    }

    return 0;
}


/*--------------------------------------------------------------*
 *                    Print info to stream                      *
 *--------------------------------------------------------------*/
/*
 *  fprintTiffInfo()
 *
 *      Input:  stream (for output of tag data)
 *              tiffile (input)
 *      Return: 0 if OK; 1 on error
 */
l_int32
fprintTiffInfo(FILE        *fpout,
               const char  *tiffile)
{
TIFF  *tif;

    PROCNAME("fprintTiffInfo");

    if (!tiffile)
        return ERROR_INT("tiffile not defined", procName, 1);
    if (!fpout)
        return ERROR_INT("stream out not defined", procName, 1);

    if ((tif = openTiff(tiffile, "rb")) == NULL)
        return ERROR_INT("tif not open for read", procName, 1);

    TIFFPrintDirectory(tif, fpout, 0);
    TIFFClose(tif);

    return 0;
}


/*--------------------------------------------------------------*
 *                        Get page count                        *
 *--------------------------------------------------------------*/
/*
 *  tiffGetCount()
 *
 *      Input:  stream (opened for read)
 *              &n (<return> number of images)
 *      Return: 0 if OK; 1 on error
 */
l_int32
tiffGetCount(FILE     *fp,
             l_int32  *pn)
{
l_int32  i;
TIFF    *tif;

    PROCNAME("tiffGetCount");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!pn)
        return ERROR_INT("&n not defined", procName, 1);
    *pn = 0;

    if ((tif = fopenTiff(fp, "rb")) == NULL)
        return ERROR_INT("tif not open for read", procName, 1);

    for (i = 1; i < MAX_PAGES_IN_TIFF_FILE; i++) {
        if (TIFFReadDirectory(tif) == 0)
            break;
    }
    *pn = i;
    TIFFCleanup(tif);
    return 0;
}


/*--------------------------------------------------------------*
 *                   Get resolution from tif                    *
 *--------------------------------------------------------------*/
/*
 *  getTiffResolution()
 *
 *      Input:  stream (opened for read)
 *              &xres, &yres (<return> resolution in ppi)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) If neither resolution field is set, this is not an error;
 *          the returned resolution values are 0 (designating 'unknown').
 */
l_int32
getTiffResolution(FILE     *fp,
                  l_int32  *pxres,
                  l_int32  *pyres)
{
TIFF  *tif;

    PROCNAME("getTiffResolution");

    if (!pxres || !pyres)
        return ERROR_INT("&xres and &yres not both defined", procName, 1);
    *pxres = *pyres = 0;
    if (!fp)
        return ERROR_INT("stream not opened", procName, 1);

    if ((tif = fopenTiff(fp, "rb")) == NULL)
        return ERROR_INT("tif not open for read", procName, 1);
    getTiffStreamResolution(tif, pxres, pyres);
    TIFFCleanup(tif);
    return 0;
}


/*
 *  getTiffStreamResolution()
 *
 *      Input:  tiff stream (opened for read)
 *              &xres, &yres (<return> resolution in ppi)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) If neither resolution field is set, this is not an error;
 *          the returned resolution values are 0 (designating 'unknown').
 */
static l_int32
getTiffStreamResolution(TIFF     *tif,
                        l_int32  *pxres,
                        l_int32  *pyres)
{
l_uint16   resunit;
l_int32    foundxres, foundyres;
l_float32  fxres, fyres;

    PROCNAME("getTiffStreamResolution");

    if (!tif)
        return ERROR_INT("tif not opened", procName, 1);
    if (!pxres || !pyres)
        return ERROR_INT("&xres and &yres not both defined", procName, 1);
    *pxres = *pyres = 0;

    TIFFGetFieldDefaulted(tif, TIFFTAG_RESOLUTIONUNIT, &resunit);
    foundxres = TIFFGetField(tif, TIFFTAG_XRESOLUTION, &fxres);
    foundyres = TIFFGetField(tif, TIFFTAG_YRESOLUTION, &fyres);
    if (!foundxres && !foundyres) return 1;
    if (!foundxres && foundyres)
        fxres = fyres;
    else if (foundxres && !foundyres)
        fyres = fxres;

    if (resunit == RESUNIT_CENTIMETER) {  /* convert to ppi */
        *pxres = (l_int32)(2.54 * fxres + 0.5);
        *pyres = (l_int32)(2.54 * fyres + 0.5);
    } else {
        *pxres = (l_int32)fxres;
        *pyres = (l_int32)fyres;
    }

    return 0;
}


/*--------------------------------------------------------------*
 *              Get some tiff header information                *
 *--------------------------------------------------------------*/
/*!
 *  readHeaderTiff()
 *
 *      Input:  filename
 *              n (page image number: 0-based)
 *              &width (<return>)
 *              &height (<return>)
 *              &bps (<return> bits per sample -- 1, 2, 4 or 8)
 *              &spp (<return>; samples per pixel -- 1 or 3)
 *              &res (<optional return>; resolution in x dir; NULL to ignore)
 *              &cmap (<optional return>; colormap exists; input NULL to ignore)
 *              &format (<optional return>; tiff format; input NULL to ignore)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If there is a colormap, cmap is returned as 1; else 0.
 *      (2) If @n is equal to or greater than the number of images, returns 1.
 */
l_int32
readHeaderTiff(const char *filename,
               l_int32     n,
               l_int32    *pwidth,
               l_int32    *pheight,
               l_int32    *pbps,
               l_int32    *pspp,
               l_int32    *pres,
               l_int32    *pcmap,
               l_int32    *pformat)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("readHeaderTiff");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!pwidth || !pheight || !pbps || !pspp)
        return ERROR_INT("input ptr(s) not all defined", procName, 1);
    *pwidth = *pheight = *pbps = *pspp = 0;
    if (pres) *pres = 0;
    if (pcmap) *pcmap = 0;

    if ((fp = fopenReadStream(filename)) == NULL)
        return ERROR_INT("image file not found", procName, 1);
    ret = freadHeaderTiff(fp, n, pwidth, pheight, pbps, pspp,
                          pres, pcmap, pformat);
    fclose(fp);
    return ret;
}


/*!
 *  freadHeaderTiff()
 *
 *      Input:  stream
 *              n (page image number: 0-based)
 *              &width (<return>)
 *              &height (<return>)
 *              &bps (<return> bits per sample -- 1, 2, 4 or 8)
 *              &spp (<return>; samples per pixel -- 1 or 3)
 *              &res (<optional return>; resolution in x dir; NULL to ignore)
 *              &cmap (<optional return>; colormap exists; input NULL to ignore)
 *              &format (<optional return>; tiff format; input NULL to ignore)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If there is a colormap, cmap is returned as 1; else 0.
 *      (2) If @n is equal to or greater than the number of images, returns 1.
 */
l_int32
freadHeaderTiff(FILE     *fp,
                l_int32   n,
                l_int32  *pwidth,
                l_int32  *pheight,
                l_int32  *pbps,
                l_int32  *pspp,
                l_int32  *pres,
                l_int32  *pcmap,
                l_int32  *pformat)
{
l_int32  i, ret, format;
TIFF    *tif;

    PROCNAME("freadHeaderTiff");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (n < 0)
        return ERROR_INT("image index must be >= 0", procName, 1);
    if (!pwidth || !pheight || !pbps || !pspp)
        return ERROR_INT("input ptr(s) not all defined", procName, 1);
    *pwidth = *pheight = *pbps = *pspp = 0;
    if (pres) *pres = 0;
    if (pcmap) *pcmap = 0;
    if (pformat) *pformat = 0;

    findFileFormatStream(fp, &format);
    if (format != IFF_TIFF &&
        format != IFF_TIFF_G3 && format != IFF_TIFF_G4 &&
        format != IFF_TIFF_RLE && format != IFF_TIFF_PACKBITS &&
        format != IFF_TIFF_LZW && format != IFF_TIFF_ZIP)
        return ERROR_INT("file not tiff format", procName, 1);

    if ((tif = fopenTiff(fp, "rb")) == NULL)
        return ERROR_INT("tif not open for read", procName, 1);

    for (i = 0; i < n; i++) {
        if (TIFFReadDirectory(tif) == 0)
            return ERROR_INT("image n not found in file", procName, 1);
    }

    ret = tiffReadHeaderTiff(tif, pwidth, pheight, pbps, pspp,
                             pres, pcmap, pformat);
    TIFFCleanup(tif);
    return ret;
}


/*!
 *  readHeaderMemTiff()
 *
 *      Input:  cdata (const; tiff-encoded)
 *              size (size of data)
 *              n (page image number: 0-based)
 *              &width (<return>)
 *              &height (<return>)
 *              &bps (<return> bits per sample -- 1, 2, 4 or 8)
 *              &spp (<return>; samples per pixel -- 1 or 3)
 *              &res (<optional return>; resolution in x dir; NULL to ignore)
 *              &cmap (<optional return>; colormap exists; input NULL to ignore)
 *              &format (<optional return>; tiff format; input NULL to ignore)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Use TIFFClose(); TIFFCleanup() doesn't free internal memstream.
 */
l_int32
readHeaderMemTiff(const l_uint8  *cdata,
                  size_t          size,
                  l_int32         n,
                  l_int32        *pwidth,
                  l_int32        *pheight,
                  l_int32        *pbps,
                  l_int32        *pspp,
                  l_int32        *pres,
                  l_int32        *pcmap,
                  l_int32        *pformat)
{
l_uint8  *data;
l_int32   i, ret;
TIFF     *tif;

    PROCNAME("readHeaderMemTiff");

    if (!cdata)
        return ERROR_INT("cdata not defined", procName, 1);
    if (!pwidth || !pheight || !pbps || !pspp)
        return ERROR_INT("input ptr(s) not all defined", procName, 1);
    *pwidth = *pheight = *pbps = *pspp = 0;
    if (pres) *pres = 0;
    if (pcmap) *pcmap = 0;
    if (pformat) *pformat = 0;

        /* Open a tiff stream to memory */
    data = (l_uint8 *)cdata;  /* we're really not going to change this */
    if ((tif = fopenTiffMemstream("tifferror", "r", &data, &size)) == NULL)
        return ERROR_INT("tiff stream not opened", procName, 1);

    for (i = 0; i < n; i++) {
        if (TIFFReadDirectory(tif) == 0) {
            TIFFClose(tif);
            return ERROR_INT("image n not found in file", procName, 1);
        }
    }

    ret = tiffReadHeaderTiff(tif, pwidth, pheight, pbps, pspp,
                             pres, pcmap, pformat);
    TIFFClose(tif);
    return ret;
}


/*!
 *  tiffReadHeaderTiff()
 *
 *      Input:  tif
 *              &width (<return>)
 *              &height (<return>)
 *              &bps (<return> bits per sample -- 1, 2, 4 or 8)
 *              &spp (<return>; samples per pixel -- 1 or 3)
 *              &res (<optional return>; resolution in x dir; NULL to ignore)
 *              &cmap (<optional return>; cmap exists; input NULL to ignore)
 *              &format (<optional return>; tiff format; input NULL to ignore)
 *      Return: 0 if OK, 1 on error
 */
static l_int32
tiffReadHeaderTiff(TIFF     *tif,
                   l_int32  *pwidth,
                   l_int32  *pheight,
                   l_int32  *pbps,
                   l_int32  *pspp,
                   l_int32  *pres,
                   l_int32  *pcmap,
                   l_int32  *pformat)
{
l_uint16   tiffcomp;
l_uint16   bps, spp;
l_uint16  *rmap, *gmap, *bmap;
l_int32    xres, yres;
l_uint32   w, h;

    PROCNAME("tiffReadHeaderTiff");

    if (!tif)
        return ERROR_INT("tif not opened", procName, 1);

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
    *pwidth = w;
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
    *pheight = h;
    TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &bps);
    *pbps = bps;
    TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
    *pspp = spp;

    if (pres) {
        *pres = 300;  /* default ppi */
        if (getTiffStreamResolution(tif, &xres, &yres) == 0)
            *pres = (l_int32)xres;
    }

    if (pcmap) {
        *pcmap = 0;
        if (TIFFGetField(tif, TIFFTAG_COLORMAP, &rmap, &gmap, &bmap))
            *pcmap = 1;
    }

    if (pformat) {
        TIFFGetFieldDefaulted(tif, TIFFTAG_COMPRESSION, &tiffcomp);
        *pformat = getTiffCompressedFormat(tiffcomp);
    }
    return 0;
}


/*!
 *  findTiffCompression()
 *
 *      Input:  stream (must be rewound to BOF)
 *              &comptype (<return> compression type)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The returned compression type is that defined in
 *          the enum in imageio.h.  It is not the tiff flag value.
 *      (2) The compression type is initialized to IFF_UNKNOWN.
 *          If it is not one of the specified types, the returned
 *          type is IFF_TIFF, which indicates no compression.
 *      (3) When this function is called, the stream must be at BOF.
 *          If the opened stream is to be used again to read the
 *          file, it must be rewound to BOF after calling this function.
 */
l_int32
findTiffCompression(FILE     *fp,
                    l_int32  *pcomptype)
{
l_uint16  tiffcomp;
TIFF     *tif;

    PROCNAME("findTiffCompression");

    if (!pcomptype)
        return ERROR_INT("&comptype not defined", procName, 1);
    *pcomptype = IFF_UNKNOWN;  /* init */
    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);

    if ((tif = fopenTiff(fp, "rb")) == NULL)
        return ERROR_INT("tif not opened", procName, 1);
    TIFFGetFieldDefaulted(tif, TIFFTAG_COMPRESSION, &tiffcomp);
    *pcomptype = getTiffCompressedFormat(tiffcomp);
    TIFFCleanup(tif);
    return 0;
}


/*!
 *  getTiffCompressedFormat()
 *
 *      Input:  tiffcomp (defined in tiff.h)
 *      Return: compression format (defined in imageio.h)
 *
 *  Notes:
 *      (1) The input must be the actual tiff compression type
 *          returned by a tiff library call.  It should always be
 *          a valid tiff type.
 *      (2) The return type is defined in the enum in imageio.h.
 */
static l_int32
getTiffCompressedFormat(l_uint16  tiffcomp)
{
l_int32  comptype;

    switch (tiffcomp)
    {
    case COMPRESSION_CCITTFAX4:
        comptype = IFF_TIFF_G4;
        break;
    case COMPRESSION_CCITTFAX3:
        comptype = IFF_TIFF_G3;
        break;
    case COMPRESSION_CCITTRLE:
        comptype = IFF_TIFF_RLE;
        break;
    case COMPRESSION_PACKBITS:
        comptype = IFF_TIFF_PACKBITS;
        break;
    case COMPRESSION_LZW:
        comptype = IFF_TIFF_LZW;
        break;
    case COMPRESSION_ADOBE_DEFLATE:
        comptype = IFF_TIFF_ZIP;
        break;
    default:
        comptype = IFF_TIFF;
        break;
    }
    return comptype;
}


/*--------------------------------------------------------------*
 *                   Extraction of tiff g4 data                 *
 *--------------------------------------------------------------*/
/*!
 *  extractG4DataFromFile()
 *
 *      Input:  filein
 *              &data (<return> binary data of ccitt g4 encoded stream)
 *              &nbytes (<return> size of binary data)
 *              &w (<return optional> image width)
 *              &h (<return optional> image height)
 *              &minisblack (<return optional> boolean)
 *      Return: 0 if OK, 1 on error
 */
l_int32
extractG4DataFromFile(const char  *filein,
                      l_uint8    **pdata,
                      size_t      *pnbytes,
                      l_int32     *pw,
                      l_int32     *ph,
                      l_int32     *pminisblack)
{
l_uint8  *inarray, *data;
l_uint16  minisblack, comptype;  /* accessors require l_uint16 */
l_int32   istiff;
l_uint32  w, h, rowsperstrip;  /* accessors require l_uint32 */
l_uint32  diroff;
size_t    fbytes, nbytes;
FILE     *fpin;
TIFF     *tif;

    PROCNAME("extractG4DataFromFile");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);
    if (!pw && !ph && !pminisblack)
        return ERROR_INT("no output data requested", procName, 1);
    *pdata = NULL;
    *pnbytes = 0;

    if ((fpin = fopenReadStream(filein)) == NULL)
        return ERROR_INT("stream not opened to file", procName, 1);
    istiff = fileFormatIsTiff(fpin);
    fclose(fpin);
    if (!istiff)
        return ERROR_INT("filein not tiff", procName, 1);

    if ((inarray = l_binaryRead(filein, &fbytes)) == NULL)
        return ERROR_INT("inarray not made", procName, 1);

        /* Get metadata about the image */
    if ((tif = openTiff(filein, "rb")) == NULL)
        return ERROR_INT("tif not open for read", procName, 1);
    TIFFGetField(tif, TIFFTAG_COMPRESSION, &comptype);
    if (comptype != COMPRESSION_CCITTFAX4) {
        FREE(inarray);
        TIFFClose(tif);
        return ERROR_INT("filein is not g4 compressed", procName, 1);
    }

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
    TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
    if (h != rowsperstrip)
        L_WARNING("more than 1 strip\n", procName);
    TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &minisblack);  /* for 1 bpp */
/*    TIFFPrintDirectory(tif, stderr, 0); */
    TIFFClose(tif);
    if (pw) *pw = (l_int32)w;
    if (ph) *ph = (l_int32)h;
    if (pminisblack) *pminisblack = (l_int32)minisblack;

        /* The header has 8 bytes: the first 2 are the magic number,
         * the next 2 are the version, and the last 4 are the
         * offset to the first directory.  That's what we want here.
         * We have to test the byte order before decoding 4 bytes! */
    if (inarray[0] == 0x4d) {  /* big-endian */
        diroff = (inarray[4] << 24) | (inarray[5] << 16) |
                 (inarray[6] << 8) | inarray[7];
    } else  {   /* inarray[0] == 0x49 :  little-endian */
        diroff = (inarray[7] << 24) | (inarray[6] << 16) |
                 (inarray[5] << 8) | inarray[4];
    }
/*    fprintf(stderr, " diroff = %d, %x\n", diroff, diroff); */

        /* Extract the ccittg4 encoded data from the tiff file.
         * We skip the 8 byte header and take nbytes of data,
         * up to the beginning of the directory (at diroff)  */
    nbytes = diroff - 8;
    *pnbytes = nbytes;
    if ((data = (l_uint8 *)CALLOC(nbytes, sizeof(l_uint8))) == NULL) {
        FREE(inarray);
        return ERROR_INT("data not allocated", procName, 1);
    }
    *pdata = data;
    memcpy(data, inarray + 8, nbytes);
    FREE(inarray);

    return 0;
}


/*--------------------------------------------------------------*
 *               Open tiff stream from file stream              *
 *--------------------------------------------------------------*/
/*!
 *  fopenTiff()
 *
 *      Input:  stream
 *              modestring ("r", "w", ...)
 *      Return: tiff (data structure, opened for a file descriptor)
 *
 *  Notes:
 *      (1) Why is this here?  Leffler did not provide a function that
 *          takes a stream and gives a TIFF.  He only gave one that
 *          generates a TIFF starting with a file descriptor.  So we
 *          need to make it here, because it is useful to have functions
 *          that take a stream as input.
 *      (2) Requires lseek to rewind to BOF; fseek won't hack it.
 *      (3) When linking with windows, suggest you use tif_unix.c
 *          instead of tif_win32.c, because it has been reported that
 *          the file descriptor returned from fileno() does not work
 *          with TIFFFdOpen() in tif_win32.c.  (win32 requires a
 *          "handle", which is an integer returned by _get_osfhandle(fd).)
 */
static TIFF *
fopenTiff(FILE        *fp,
          const char  *modestring)
{
l_int32  fd;

    PROCNAME("fopenTiff");

    if (!fp)
        return (TIFF *)ERROR_PTR("stream not opened", procName, NULL);
    if (!modestring)
        return (TIFF *)ERROR_PTR("modestring not defined", procName, NULL);

    if ((fd = fileno(fp)) < 0)
        return (TIFF *)ERROR_PTR("invalid file descriptor", procName, NULL);
    lseek(fd, 0, SEEK_SET);

    return TIFFFdOpen(fd, "TIFFstream", modestring);
}


/*--------------------------------------------------------------*
 *                      Wrapper for TIFFOpen                    *
 *--------------------------------------------------------------*/
/*!
 *  openTiff()
 *
 *      Input:  filename
 *              modestring ("r", "w", ...)
 *      Return: tiff (data structure)
 *
 *  Notes:
 *      (1) This handles multi-platform file naming.
 */
static TIFF *
openTiff(const char  *filename,
         const char  *modestring)
{
char  *fname;
TIFF  *tif;

    PROCNAME("openTiff");

    if (!filename)
        return (TIFF *)ERROR_PTR("filename not defined", procName, NULL);
    if (!modestring)
        return (TIFF *)ERROR_PTR("modestring not defined", procName, NULL);

    fname = genPathname(filename, NULL);
    tif = TIFFOpen(fname, modestring);
    FREE(fname);
    return tif;
}


/*----------------------------------------------------------------------*
 *     Memory I/O: reading memory --> pix and writing pix --> memory    *
 *----------------------------------------------------------------------*/
/*  It would be nice to use open_memstream() and fmemopen()
 *  for writing and reading to memory, rsp.  These functions manage
 *  memory for writes and reads that use a file streams interface.
 *  Unfortunately, the tiff library only has an interface for reading
 *  and writing to file descriptors, not to file streams.  The tiff
 *  library procedure is to open a "tiff stream" and read/write to it.
 *  The library provides a client interface for managing the I/O
 *  from memory, which requires seven callbacks.  See the TIFFClientOpen
 *  man page for callback signatures.  Adam Langley provided the code
 *  to do this.  */

/*
 *  The L_Memstram @buffer has different functions in writing and reading.
 *
 *     * In reading, it is assigned to the data and read from as
 *       the tiff library uncompresses the data and generates the pix.
 *       The @offset points to the current read position in the data,
 *       and the @hw always gives the number of bytes of data.
 *       The @outdata and @outsize ptrs are not used.
 *       When finished, tiffCloseCallback() simply frees the L_Memstream.
 *
 *     * In writing, it accepts the data that the tiff library
 *       produces when a pix is compressed.  the buffer points to a
 *       malloced area of @bufsize bytes.  The current writing position
 *       in the buffer is @offset and the most ever written is @hw.
 *       The buffer is expanded as necessary.  When finished,
 *       tiffCloseCallback() assigns the @outdata and @outsize ptrs
 *       to the @buffer and @bufsize results, and frees the L_Memstream.
 */
struct L_Memstream
{
    l_uint8   *buffer;    /* expands to hold data when written to;         */
                          /* fixed size when read from.                    */
    size_t     bufsize;   /* current size allocated when written to;       */
                          /* fixed size of input data when read from.      */
    size_t     offset;    /* byte offset from beginning of buffer.         */
    size_t     hw;        /* high-water mark; max bytes in buffer.         */
    l_uint8  **poutdata;  /* input param for writing; data goes here.      */
    size_t    *poutsize;  /* input param for writing; data size goes here. */
};
typedef struct L_Memstream  L_MEMSTREAM;


    /* These are static functions for memory I/O */
static L_MEMSTREAM *memstreamCreateForRead(l_uint8 *indata, size_t pinsize);
static L_MEMSTREAM *memstreamCreateForWrite(l_uint8 **poutdata,
                                            size_t *poutsize);
static tsize_t tiffReadCallback(thandle_t handle, tdata_t data, tsize_t length);
static tsize_t tiffWriteCallback(thandle_t handle, tdata_t data,
                                 tsize_t length);
static toff_t tiffSeekCallback(thandle_t handle, toff_t offset, l_int32 whence);
static l_int32 tiffCloseCallback(thandle_t handle);
static toff_t tiffSizeCallback(thandle_t handle);
static l_int32 tiffMapCallback(thandle_t handle, tdata_t *data, toff_t *length);
static void tiffUnmapCallback(thandle_t handle, tdata_t data, toff_t length);


static L_MEMSTREAM *
memstreamCreateForRead(l_uint8  *indata,
                       size_t    insize)
{
L_MEMSTREAM  *mstream;

    mstream = (L_MEMSTREAM *)CALLOC(1, sizeof(L_MEMSTREAM));
    mstream->buffer = indata;   /* handle to input data array */
    mstream->bufsize = insize;  /* amount of input data */
    mstream->hw = insize;       /* high-water mark fixed at input data size */
    mstream->offset = 0;        /* offset always starts at 0 */
    return mstream;
}


static L_MEMSTREAM *
memstreamCreateForWrite(l_uint8  **poutdata,
                        size_t    *poutsize)
{
L_MEMSTREAM  *mstream;

    mstream = (L_MEMSTREAM *)CALLOC(1, sizeof(L_MEMSTREAM));
    mstream->buffer = (l_uint8 *)CALLOC(8 * 1024, 1);
    mstream->bufsize = 8 * 1024;
    mstream->poutdata = poutdata;  /* used only at end of write */
    mstream->poutsize = poutsize;  /* ditto  */
    mstream->hw = mstream->offset = 0;
    return mstream;
}


static tsize_t
tiffReadCallback(thandle_t  handle,
                 tdata_t    data,
                 tsize_t    length)
{
L_MEMSTREAM  *mstream;
size_t        amount;

    mstream = (L_MEMSTREAM *)handle;
    amount = L_MIN((size_t)length, mstream->hw - mstream->offset);
    memcpy(data, mstream->buffer + mstream->offset, amount);
    mstream->offset += amount;
    return amount;
}


static tsize_t
tiffWriteCallback(thandle_t  handle,
                  tdata_t    data,
                  tsize_t    length)
{
L_MEMSTREAM  *mstream;
size_t        newsize;

        /* reallocNew() uses calloc to initialize the array.
         * If malloc is used instead, for some of the encoding methods,
         * not all the data in 'bufsize' bytes in the buffer will
         * have been initialized by the end of the compression. */
    mstream = (L_MEMSTREAM *)handle;
    if (mstream->offset + length > mstream->bufsize) {
        newsize = 2 * (mstream->offset + length);
        mstream->buffer = (l_uint8 *)reallocNew((void **)&mstream->buffer,
                                                mstream->hw, newsize);
        mstream->bufsize = newsize;
    }

    memcpy(mstream->buffer + mstream->offset, data, length);
    mstream->offset += length;
    mstream->hw = L_MAX(mstream->offset, mstream->hw);
    return length;
}


static toff_t
tiffSeekCallback(thandle_t  handle,
                 toff_t     offset,
                 l_int32    whence)
{
L_MEMSTREAM  *mstream;

    PROCNAME("tiffSeekCallback");
    mstream = (L_MEMSTREAM *)handle;
    switch (whence) {
        case SEEK_SET:
/*            fprintf(stderr, "seek_set: offset = %d\n", offset); */
            mstream->offset = offset;
            break;
        case SEEK_CUR:
/*            fprintf(stderr, "seek_cur: offset = %d\n", offset); */
            mstream->offset += offset;
            break;
        case SEEK_END:
/*            fprintf(stderr, "seek end: hw = %d, offset = %d\n",
                    mstream->hw, offset); */
            mstream->offset = mstream->hw - offset;  /* offset >= 0 */
            break;
        default:
            return (toff_t)ERROR_INT("bad whence value", procName,
                                     mstream->offset);
    }

    return mstream->offset;
}


static l_int32
tiffCloseCallback(thandle_t  handle)
{
L_MEMSTREAM  *mstream;

    mstream = (L_MEMSTREAM *)handle;
    if (mstream->poutdata) {   /* writing: save the output data */
        *mstream->poutdata = mstream->buffer;
        *mstream->poutsize = mstream->hw;
    }
    FREE(mstream);  /* never free the buffer! */
    return 0;
}


static toff_t
tiffSizeCallback(thandle_t  handle)
{
L_MEMSTREAM  *mstream;

    mstream = (L_MEMSTREAM *)handle;
    return mstream->hw;
}


static l_int32
tiffMapCallback(thandle_t  handle,
                tdata_t   *data,
                toff_t    *length)
{
L_MEMSTREAM  *mstream;

    mstream = (L_MEMSTREAM *)handle;
    *data = mstream->buffer;
    *length = mstream->hw;
    return 0;
}


static void
tiffUnmapCallback(thandle_t  handle,
                  tdata_t    data,
                  toff_t     length)
{
    return;
}


/*!
 *  fopenTiffMemstream()
 *
 *      Input:  filename (for error output; can be "")
 *              operation ("w" for write, "r" for read)
 *              &data (<return> written data)
 *              &datasize (<return> size of written data)
 *      Return: tiff (data structure, opened for write to memory)
 *
 *  Notes:
 *      (1) This wraps up a number of callbacks for either:
 *            * reading from tiff in memory buffer --> pix
 *            * writing from pix --> tiff in memory buffer
 *      (2) After use, the memstream is automatically destroyed when
 *          TIFFClose() is called.  TIFFCleanup() doesn't free the memstream.
 */
static TIFF *
fopenTiffMemstream(const char  *filename,
                   const char  *operation,
                   l_uint8    **pdata,
                   size_t      *pdatasize)
{
L_MEMSTREAM  *mstream;

    PROCNAME("fopenTiffMemstream");

    if (!filename)
        return (TIFF *)ERROR_PTR("filename not defined", procName, NULL);
    if (!operation)
        return (TIFF *)ERROR_PTR("operation not defined", procName, NULL);
    if (!pdata)
        return (TIFF *)ERROR_PTR("&data not defined", procName, NULL);
    if (!pdatasize)
        return (TIFF *)ERROR_PTR("&datasize not defined", procName, NULL);
    if (!strcmp(operation, "r") && !strcmp(operation, "w"))
        return (TIFF *)ERROR_PTR("operation not 'r' or 'w'}", procName, NULL);

    if (!strcmp(operation, "r"))
        mstream = memstreamCreateForRead(*pdata, *pdatasize);
    else
        mstream = memstreamCreateForWrite(pdata, pdatasize);

    return TIFFClientOpen(filename, operation, mstream,
                          tiffReadCallback, tiffWriteCallback,
                          tiffSeekCallback, tiffCloseCallback,
                          tiffSizeCallback, tiffMapCallback,
                          tiffUnmapCallback);
}


/*!
 *  pixReadMemTiff()
 *
 *      Input:  data (const; tiff-encoded)
 *              datasize (size of data)
 *              n (page image number: 0-based)
 *      Return: pix, or null on error
 *
 *  Notes:
 *      (1) This is a version of pixReadTiff(), where the data is read
 *          from a memory buffer and uncompressed.
 *      (2) Use TIFFClose(); TIFFCleanup() doesn't free internal memstream.
 */
PIX *
pixReadMemTiff(const l_uint8  *cdata,
               size_t          size,
               l_int32         n)
{
l_uint8  *data;
l_int32   i, pagefound;
PIX      *pix;
TIFF     *tif;

    PROCNAME("pixReadMemTiff");

    if (!cdata)
        return (PIX *)ERROR_PTR("cdata not defined", procName, NULL);

    data = (l_uint8 *)cdata;  /* we're really not going to change this */
    if ((tif = fopenTiffMemstream("tifferror", "r", &data, &size)) == NULL)
        return (PIX *)ERROR_PTR("tiff stream not opened", procName, NULL);

    pagefound = FALSE;
    pix = NULL;
    for (i = 0; i < MAX_PAGES_IN_TIFF_FILE; i++) {
        if (i == n) {
            pagefound = TRUE;
            if ((pix = pixReadFromTiffStream(tif)) == NULL) {
                TIFFClose(tif);
                return (PIX *)ERROR_PTR("pix not read", procName, NULL);
            }
            pixSetInputFormat(pix, IFF_TIFF);
            break;
        }
        if (TIFFReadDirectory(tif) == 0)
            break;
    }

    if (pagefound == FALSE)
        L_WARNING("tiff page %d not found\n", procName, n);

    TIFFClose(tif);
    return pix;
}


/*!
 *  pixWriteMemTiff()
 *
 *      Input:  &data (<return> data of tiff compressed image)
 *              &size (<return> size of returned data)
 *              pix
 *              comptype (IFF_TIFF, IFF_TIFF_RLE, IFF_TIFF_PACKBITS,
 *                        IFF_TIFF_G3, IFF_TIFF_G4,
 *                        IFF_TIFF_LZW, IFF_TIFF_ZIP)
 *      Return: 0 if OK, 1 on error
 *
 *  Usage:
 *      (1) See pixWriteTiff().  This version writes to
 *          memory instead of to a file.
 */
l_int32
pixWriteMemTiff(l_uint8  **pdata,
                size_t    *psize,
                PIX       *pix,
                l_int32    comptype)
{
    return pixWriteMemTiffCustom(pdata, psize, pix, comptype,
                                 NULL, NULL, NULL, NULL);
}


/*!
 *  pixWriteMemTiffCustom()
 *
 *      Input:  &data (<return> data of tiff compressed image)
 *              &size (<return> size of returned data)
 *              pix
 *              comptype (IFF_TIFF, IFF_TIFF_RLE, IFF_TIFF_PACKBITS,
 *                        IFF_TIFF_G3, IFF_TIFF_G4,
 *                        IFF_TIFF_LZW, IFF_TIFF_ZIP)
 *              natags (<optional> NUMA of custom tiff tags)
 *              savals (<optional> SARRAY of values)
 *              satypes (<optional> SARRAY of types)
 *              nasizes (<optional> NUMA of sizes)
 *      Return: 0 if OK, 1 on error
 *
 *  Usage:
 *      (1) See pixWriteTiffCustom().  This version writes to
 *          memory instead of to a file.
 *      (2) Use TIFFClose(); TIFFCleanup() doesn't free internal memstream.
 */
l_int32
pixWriteMemTiffCustom(l_uint8  **pdata,
                      size_t    *psize,
                      PIX       *pix,
                      l_int32    comptype,
                      NUMA      *natags,
                      SARRAY    *savals,
                      SARRAY    *satypes,
                      NUMA      *nasizes)
{
l_int32  ret;
TIFF    *tif;

    PROCNAME("pixWriteMemTiffCustom");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    if (!psize)
        return ERROR_INT("&size not defined", procName, 1);
    if (!pix)
        return ERROR_INT("&pix not defined", procName, 1);
    if (pixGetDepth(pix) != 1 && comptype != IFF_TIFF &&
        comptype != IFF_TIFF_LZW && comptype != IFF_TIFF_ZIP) {
        L_WARNING("invalid compression type for bpp > 1\n", procName);
        comptype = IFF_TIFF_ZIP;
    }

    if ((tif = fopenTiffMemstream("tifferror", "w", pdata, psize)) == NULL)
        return ERROR_INT("tiff stream not opened", procName, 1);
    ret = pixWriteToTiffStream(tif, pix, comptype, natags, savals,
                               satypes, nasizes);

    TIFFClose(tif);
    return ret;
}

/* --------------------------------------------*/
#endif  /* HAVE_LIBTIFF */
/* --------------------------------------------*/
