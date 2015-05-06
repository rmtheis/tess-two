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
 *  bmpio.c
 *
 *      Read bmp from file
 *           PIX          *pixReadStreamBmp()
 *
 *      Write bmp to file
 *           l_int32       pixWriteStreamBmp()
 *
 *      Read/write to memory
 *           PIX          *pixReadMemBmp()
 *           l_int32       pixWriteMemBmp()
 *
 *    On systems like windows without fmemopen() and open_memstream(),
 *    we write data to a temp file and read it back for operations
 *    between pix and compressed-data, such as pixReadMemPng() and
 *    pixWriteMemPng().
 */

#include <string.h>
#include "allheaders.h"
#include "bmp.h"

/* --------------------------------------------*/
#if  USE_BMPIO   /* defined in environ.h */
/* --------------------------------------------*/

    /* Here we're setting the pixel value 0 to white (255) and the
     * value 1 to black (0).  This is the convention for grayscale, but
     * the opposite of the convention for 1 bpp, where 0 is white
     * and 1 is black.  Both colormap entries are opaque (alpha = 255) */
RGBA_QUAD   bwmap[2] = { {255,255,255,255}, {0,0,0,255} };

    /* Colormap size limit */
static const l_int32  L_MAX_ALLOWED_NUM_COLORS = 256;

#ifndef  NO_CONSOLE_IO
#define  DEBUG     0
#endif  /* ~NO_CONSOLE_IO */


/*!
 *  pixReadStreamBmp()
 *
 *      Input:  stream opened for read
 *      Return: pix, or null on error
 *
 *  Notes:
 *      (1) Here are references on the bmp file format:
 *          http://en.wikipedia.org/wiki/BMP_file_format
 *          http://www.fortunecity.com/skyscraper/windows/364/bmpffrmt.html
 */
PIX *
pixReadStreamBmp(FILE  *fp)
{
l_uint16   sval;
l_uint32   ival;
l_int16    bfType, bfSize, bfFill1, bfReserved1, bfReserved2;
l_int16    offset, bfFill2, biPlanes, depth, d;
l_int32    biSize, width, height, xres, yres, compression;
l_int32    imagebytes, biClrUsed, biClrImportant;
l_uint8   *colormapBuf;
l_int32    colormapEntries;
l_int32    fileBpl, extrabytes;
l_int32    pixWpl, pixBpl;
l_int32    i, j, k;
l_uint8    pel[4];
l_uint8   *data;
l_uint32  *line, *pword;
PIX        *pix, *pixt;
PIXCMAP   *cmap;

    PROCNAME("pixReadStreamBmp");

    if (!fp)
        return (PIX *)ERROR_PTR("fp not defined", procName, NULL);

        /* Read bitmap file header */
    if (fread((char *)&sval, 2, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 1 not read", procName, NULL);
    bfType = convertOnBigEnd16(sval);
    if (bfType != BMP_ID)
        return (PIX *)ERROR_PTR("not bmf format", procName, NULL);

    if (fread((char *)&sval, 2, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 2 not read", procName, NULL);
    bfSize = convertOnBigEnd16(sval);
    if (fread((char *)&sval, 2, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 3 not read", procName, NULL);
    bfFill1 = convertOnBigEnd16(sval);
    if (fread((char *)&sval, 2, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 4 not read", procName, NULL);
    bfReserved1 = convertOnBigEnd16(sval);
    if (fread((char *)&sval, 2, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 5 not read", procName, NULL);
    bfReserved2 = convertOnBigEnd16(sval);
    if (fread((char *)&sval, 2, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 6 not read", procName, NULL);
    offset = convertOnBigEnd16(sval);
    if (fread((char *)&sval, 2, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 7 not read", procName, NULL);
    bfFill2 = convertOnBigEnd16(sval);

        /* Read bitmap info header */
    if (fread((char *)&ival, 4, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 8 not read", procName, NULL);
    biSize = convertOnBigEnd32(ival);
    if (fread((char *)&ival, 4, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 9 not read", procName, NULL);
    width = convertOnBigEnd32(ival);
    if (fread((char *)&ival, 4, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 10 not read", procName, NULL);
    height = convertOnBigEnd32(ival);
    if (fread((char *)&sval, 2, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 11 not read", procName, NULL);
    biPlanes = convertOnBigEnd16(sval);
    if (fread((char *)&sval, 2, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 12 not read", procName, NULL);
    depth = convertOnBigEnd16(sval);
    if (fread((char *)&ival, 4, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 13 not read", procName, NULL);
    compression = convertOnBigEnd32(ival);
    if (fread((char *)&ival, 4, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 14 not read", procName, NULL);
    imagebytes = convertOnBigEnd32(ival);
    if (fread((char *)&ival, 4, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 15 not read", procName, NULL);
    xres = convertOnBigEnd32(ival);
    if (fread((char *)&ival, 4, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 16 not read", procName, NULL);
    yres = convertOnBigEnd32(ival);
    if (fread((char *)&ival, 4, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 17 not read", procName, NULL);
    biClrUsed = convertOnBigEnd32(ival);
    if (fread((char *)&ival, 4, 1, fp) != 1)
        return (PIX *)ERROR_PTR("item 18 not read", procName, NULL);
    biClrImportant = convertOnBigEnd32(ival);

    if (compression != 0)
        return (PIX *)ERROR_PTR("cannot read compressed BMP files",
                                procName, NULL);

        /* A little sanity checking.  It would be nice to check
         * if the number of bytes in the file equals the offset to
         * the data plus the imagedata, but this won't work when
         * reading from memory, because fmemopen() doesn't implement
         * ftell().  So we can't do that check.  The imagebytes for
         * uncompressed images is either 0 or the size of the file data.
         * (The fact that it can be 0 is perhaps some legacy glitch).  */
    if (width < 1)
        return (PIX *)ERROR_PTR("width < 1", procName, NULL);
    if (height < 1)
        return (PIX *)ERROR_PTR("height < 1", procName, NULL);
    if (depth != 1 && depth != 2 && depth != 4 && depth != 8 &&
        depth != 16 && depth != 24 && depth != 32)
        return (PIX *)ERROR_PTR("depth not in {1, 2, 4, 8, 16, 24, 32}",
                                procName,NULL);
    fileBpl = 4 * ((width * depth + 31)/32);
    if (imagebytes != 0 && imagebytes != fileBpl * height)
        return (PIX *)ERROR_PTR("invalid imagebytes", procName, NULL);
    if (offset < BMP_FHBYTES + BMP_IHBYTES)
        return (PIX *)ERROR_PTR("invalid offset: too small", procName, NULL);
    if (offset > BMP_FHBYTES + BMP_IHBYTES + 4 * 256)
        return (PIX *)ERROR_PTR("invalid offset: too large", procName, NULL);

        /* Handle the colormap */
    colormapEntries = (offset - BMP_FHBYTES - BMP_IHBYTES) / sizeof(RGBA_QUAD);
    colormapBuf = NULL;
    if (colormapEntries > L_MAX_ALLOWED_NUM_COLORS)
        return (PIX *)ERROR_PTR("colormap too large", procName,NULL);
    if (colormapEntries > 0) {
        if ((colormapBuf = (l_uint8 *)CALLOC(colormapEntries,
                                             sizeof(RGBA_QUAD))) == NULL)
            return (PIX *)ERROR_PTR("colormapBuf alloc fail", procName, NULL );

            /* Read colormap */
        if (fread(colormapBuf, sizeof(RGBA_QUAD), colormapEntries, fp)
                 != colormapEntries) {
            FREE(colormapBuf);
            return (PIX *)ERROR_PTR( "colormap read fail", procName, NULL);
        }
    }

        /* Make a 32 bpp pix if depth is 24 bpp */
    d = depth;
    if (depth == 24)
        d = 32;
    if ((pix = pixCreate(width, height, d)) == NULL) {
        FREE(colormapBuf);
        return (PIX *)ERROR_PTR( "pix not made", procName, NULL);
    }
    pixSetXRes(pix, (l_int32)((l_float32)xres / 39.37 + 0.5));  /* to ppi */
    pixSetYRes(pix, (l_int32)((l_float32)yres / 39.37 + 0.5));  /* to ppi */
    pixWpl = pixGetWpl(pix);
    pixBpl = 4 * pixWpl;

    cmap = NULL;
    if (colormapEntries > 256)
        L_WARNING("more than 256 colormap entries!\n", procName);
    if (colormapEntries > 0) {  /* import the colormap to the pix cmap */
        cmap = pixcmapCreate(L_MIN(d, 8));
        FREE(cmap->array);  /* remove generated cmap array */
        cmap->array  = (void *)colormapBuf;  /* and replace */
        cmap->n = L_MIN(colormapEntries, 256);
    }
    pixSetColormap(pix, cmap);

        /* Seek to the start of the bitmap in the file */
    fseek(fp, offset, 0);

    if (depth != 24) {  /* typ. 1 or 8 bpp */
        data = (l_uint8 *)pixGetData(pix) + pixBpl * (height - 1);
        for (i = 0; i < height; i++) {
            if (fread(data, 1, fileBpl, fp) != fileBpl) {
                pixDestroy(&pix);
                return (PIX *)ERROR_PTR("BMP read fail", procName, NULL);
            }
            data -= pixBpl;
        }
    } else {  /*  24 bpp file; 32 bpp pix
             *  Note: for bmp files, pel[0] is blue, pel[1] is green,
             *  and pel[2] is red.  This is opposite to the storage
             *  in the pix, which puts the red pixel in the 0 byte,
             *  the green in the 1 byte and the blue in the 2 byte.
             *  Note also that all words are endian flipped after
             *  assignment on L_LITTLE_ENDIAN platforms.
             *
             *  We can then make these assignments for little endians:
             *      SET_DATA_BYTE(pword, 1, pel[0]);      blue
             *      SET_DATA_BYTE(pword, 2, pel[1]);      green
             *      SET_DATA_BYTE(pword, 3, pel[2]);      red
             *  This looks like:
             *          3  (R)     2  (G)        1  (B)        0
             *      |-----------|------------|-----------|-----------|
             *  and after byte flipping:
             *           3          2  (B)     1  (G)        0  (R)
             *      |-----------|------------|-----------|-----------|
             *
             *  For big endians we set:
             *      SET_DATA_BYTE(pword, 2, pel[0]);      blue
             *      SET_DATA_BYTE(pword, 1, pel[1]);      green
             *      SET_DATA_BYTE(pword, 0, pel[2]);      red
             *  This looks like:
             *          0  (R)     1  (G)        2  (B)        3
             *      |-----------|------------|-----------|-----------|
             *  so in both cases we get the correct assignment in the PIX.
             *
             *  Can we do a platform-independent assignment?
             *  Yes, set the bytes without using macros:
             *      *((l_uint8 *)pword) = pel[2];           red
             *      *((l_uint8 *)pword + 1) = pel[1];       green
             *      *((l_uint8 *)pword + 2) = pel[0];       blue
             *  For little endians, before flipping, this looks again like:
             *          3  (R)     2  (G)        1  (B)        0
             *      |-----------|------------|-----------|-----------|
             */
        extrabytes = fileBpl - 3 * width;
        line = pixGetData(pix) + pixWpl * (height - 1);
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                pword = line + j;
                if (fread(&pel, 1, 3, fp) != 3) {
                    pixDestroy(&pix);
                    return (PIX *)ERROR_PTR("bmp(1) read fail", procName, NULL);
                }
                *((l_uint8 *)pword + COLOR_RED) = pel[2];
                *((l_uint8 *)pword + COLOR_GREEN) = pel[1];
                *((l_uint8 *)pword + COLOR_BLUE) = pel[0];
            }
            if (extrabytes) {
                for (k = 0; k < extrabytes; k++) {
                    if (fread(&pel, 1, 1, fp) != 1) {
                        pixDestroy(&pix);
                        return (PIX *)ERROR_PTR("bmp(2) read fail",
                                                procName, NULL);
                    }
                }
            }
            line -= pixWpl;
        }
    }

    pixEndianByteSwap(pix);

        /* ----------------------------------------------
         * The bmp colormap determines the values of black
         * and white pixels for binary in the following way:
         * (a) white = 0 [255], black = 1 [0]
         *      255, 255, 255, 255, 0, 0, 0, 255
         * (b) black = 0 [0], white = 1 [255]
         *      0, 0, 0, 255, 255, 255, 255, 255
         * We have no need for a 1 bpp pix with a colormap!
         * Note: the alpha component here is 255 (opaque)
         * ---------------------------------------------- */
    if (depth == 1 && cmap) {
        pixt = pixRemoveColormap(pix, REMOVE_CMAP_TO_BINARY);
        pixDestroy(&pix);
        pix = pixt;  /* rename */
    }

    return pix;
}



/*!
 *  pixWriteStreamBmp()
 *
 *      Input:  stream opened for write
 *              pix (1, 4, 8, 32 bpp)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) We position fp at the beginning of the stream, so it
 *          truncates any existing data
 *      (2) 2 bpp Bmp files are apparently not valid!.  We can
 *          write and read them, but nobody else can read ours.
 */
l_int32
pixWriteStreamBmp(FILE  *fp,
                  PIX   *pix)
{
l_uint32    offbytes, filebytes, fileimagebytes;
l_int32     width, height, depth, d, xres, yres;
l_uint16    bfType, bfSize, bfFill1, bfReserved1, bfReserved2;
l_uint16    bfOffBits, bfFill2, biPlanes, biBitCount;
l_uint16    sval;
l_uint32    biSize, biWidth, biHeight, biCompression, biSizeImage;
l_uint32    biXPelsPerMeter, biYPelsPerMeter, biClrUsed, biClrImportant;
l_int32     pixWpl, pixBpl, extrabytes, writeerror;
l_int32     fileBpl, fileWpl;
l_int32     i, j, k;
l_int32     heapcm;  /* extra copy of cta on the heap ? 1 : 0 */
l_uint8    *data;
l_uint8     pel[4];
l_uint32   *line, *pword;
PIXCMAP    *cmap;
l_uint8    *cta;          /* address of the bmp color table array */
l_int32     cmaplen;      /* number of bytes in the bmp colormap */
l_int32     ncolors, val, stepsize;
RGBA_QUAD  *pquad;

    PROCNAME("pixWriteStreamBmp");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    width  = pixGetWidth(pix);
    height = pixGetHeight(pix);
    d  = pixGetDepth(pix);
    if (d == 2)
        L_WARNING("writing 2 bpp bmp file; nobody else can read\n", procName);
    depth = d;
    if (d == 32)
        depth = 24;
    xres = (l_int32)(39.37 * (l_float32)pixGetXRes(pix) + 0.5);  /* to ppm */
    yres = (l_int32)(39.37 * (l_float32)pixGetYRes(pix) + 0.5);  /* to ppm */

    pixWpl = pixGetWpl(pix);
    pixBpl = 4 * pixWpl;
    fileWpl = (width * depth + 31) / 32;
    fileBpl = 4 * fileWpl;
    fileimagebytes = height * fileBpl;

    heapcm = 0;
    if (d == 32) {   /* 24 bpp rgb; no colormap */
        ncolors = 0;
        cmaplen = 0;
    } else if ((cmap = pixGetColormap(pix))) {   /* existing colormap */
        ncolors = pixcmapGetCount(cmap);
        cmaplen = ncolors * sizeof(RGBA_QUAD);
        cta = (l_uint8 *)cmap->array;
    } else {   /* no existing colormap; make a binary or gray one */
        if (d == 1) {
            cmaplen  = sizeof(bwmap);
            ncolors = 2;
            cta = (l_uint8 *)bwmap;
        } else {   /* d != 32; output grayscale version */
            ncolors = 1 << depth;
            cmaplen = ncolors * sizeof(RGBA_QUAD);

            heapcm = 1;
            if ((cta = (l_uint8 *)CALLOC(cmaplen, 1)) == NULL)
                return ERROR_INT("colormap alloc fail", procName, 1);

            stepsize = 255 / (ncolors - 1);
            for (i = 0, val = 0, pquad = (RGBA_QUAD *)cta;
                 i < ncolors;
                 i++, val += stepsize, pquad++) {
                pquad->blue = pquad->green = pquad->red = val;
                pquad->alpha = 255;  /* opaque */
            }
        }
    }

#if DEBUG
    {l_uint8  *pcmptr;
        pcmptr = (l_uint8 *)pixGetColormap(pix)->array;
        fprintf(stderr, "Pix colormap[0] = %c%c%c%d\n",
            pcmptr[0], pcmptr[1], pcmptr[2], pcmptr[3]);
        fprintf(stderr, "Pix colormap[1] = %c%c%c%d\n",
            pcmptr[4], pcmptr[5], pcmptr[6], pcmptr[7]);
    }
#endif  /* DEBUG */

    fseek(fp, 0L, 0);

        /* Convert to little-endian and write the file header data */
    bfType = convertOnBigEnd16(BMP_ID);
    offbytes = BMP_FHBYTES + BMP_IHBYTES + cmaplen;
    filebytes = offbytes + fileimagebytes;
    sval = filebytes & 0x0000ffff;
    bfSize = convertOnBigEnd16(sval);
    sval = (filebytes >> 16) & 0x0000ffff;
    bfFill1 = convertOnBigEnd16(sval);
    bfReserved1 = 0;
    bfReserved2 = 0;
    sval = offbytes & 0x0000ffff;
    bfOffBits = convertOnBigEnd16(sval);
    sval = (offbytes >> 16) & 0x0000ffff;
    bfFill2 = convertOnBigEnd16(sval);
    fwrite(&bfType, 1, 2, fp);
    fwrite(&bfSize, 1, 2, fp);
    fwrite(&bfFill1, 1, 2, fp);
    fwrite(&bfReserved1, 1, 2, fp);
    fwrite(&bfReserved1, 1, 2, fp);
    fwrite(&bfOffBits, 1, 2, fp);
    fwrite(&bfFill2, 1, 2, fp);

        /* Convert to little-endian and write the info header data */
    biSize = convertOnBigEnd32(BMP_IHBYTES);
    biWidth = convertOnBigEnd32(width);
    biHeight = convertOnBigEnd32(height);
    biPlanes = convertOnBigEnd16(1);
    biBitCount = convertOnBigEnd16(depth);
    biCompression   = 0;
    biSizeImage = convertOnBigEnd32(fileimagebytes);
    biXPelsPerMeter = convertOnBigEnd32(xres);
    biYPelsPerMeter = convertOnBigEnd32(yres);
    biClrUsed = convertOnBigEnd32(ncolors);
    biClrImportant = convertOnBigEnd32(ncolors);
    fwrite(&biSize, 1, 4, fp);
    fwrite(&biWidth, 1, 4, fp);
    fwrite(&biHeight, 1, 4, fp);
    fwrite(&biPlanes, 1, 2, fp);
    fwrite(&biBitCount, 1, 2, fp);
    fwrite(&biCompression, 1, 4, fp);
    fwrite(&biSizeImage, 1, 4, fp);
    fwrite(&biXPelsPerMeter, 1, 4, fp);
    fwrite(&biYPelsPerMeter, 1, 4, fp);
    fwrite(&biClrUsed, 1, 4, fp);
    fwrite(&biClrImportant, 1, 4, fp);

        /* Write the colormap data */
    if (ncolors > 0) {
        if (fwrite(cta, 1, cmaplen, fp) != cmaplen) {
            if (heapcm)
                FREE(cta);
            return ERROR_INT("colormap write fail", procName, 1);
        }
        if (heapcm)
            FREE(cta);
    }

        /* When you write a binary image with a colormap
         * that sets BLACK to 0, you must invert the data */
    if (depth == 1 && cmap && ((l_uint8 *)(cmap->array))[0] == 0x0) {
        pixInvert(pix, pix);
    }

    pixEndianByteSwap(pix);

    writeerror = 0;
    if (depth != 24) {   /* typ 1 or 8 bpp */
        data = (l_uint8 *)pixGetData(pix) + pixBpl * (height - 1);
        for (i = 0; i < height; i++) {
            if (fwrite(data, 1, fileBpl, fp) != fileBpl)
                writeerror = 1;
            data -= pixBpl;
        }
    } else {  /* 32 bpp pix; 24 bpp file
             * See the comments in pixReadStreamBMP() to
             * understand the logic behind the pixel ordering below.
             * Note that we have again done an endian swap on
             * little endian machines before arriving here, so that
             * the bytes are ordered on both platforms as:
                        Red         Green        Blue         --
                    |-----------|------------|-----------|-----------|
             */
        extrabytes = fileBpl - 3 * width;
        line = pixGetData(pix) + pixWpl * (height - 1);
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                pword = line + j;
                pel[2] = *((l_uint8 *)pword + COLOR_RED);
                pel[1] = *((l_uint8 *)pword + COLOR_GREEN);
                pel[0] = *((l_uint8 *)pword + COLOR_BLUE);
                if (fwrite(&pel, 1, 3, fp) != 3)
                    writeerror = 1;
            }
            if (extrabytes) {
                for (k = 0; k < extrabytes; k++)
                    fwrite(&pel, 1, 1, fp);
            }
            line -= pixWpl;
        }
    }

        /* Restore to original state */
    pixEndianByteSwap(pix);
    if (depth == 1 && cmap && ((l_uint8 *)(cmap->array))[0] == 0x0)
        pixInvert(pix, pix);

    if (writeerror)
        return ERROR_INT("image write fail", procName, 1);

    return 0;
}


/*---------------------------------------------------------------------*
 *                         Read/write to memory                        *
 *---------------------------------------------------------------------*/
#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

#if HAVE_FMEMOPEN
extern FILE *open_memstream(char **data, size_t *size);
extern FILE *fmemopen(void *data, size_t size, const char *mode);
#endif  /* HAVE_FMEMOPEN */

/*!
 *  pixReadMemBmp()
 *
 *      Input:  cdata (const; bmp-encoded)
 *              size (of data)
 *      Return: pix, or null on error
 *
 *  Notes:
 *      (1) The @size byte of @data must be a null character.
 */
PIX *
pixReadMemBmp(const l_uint8  *cdata,
              size_t          size)
{
FILE  *fp;
PIX   *pix;

    PROCNAME("pixReadMemBmp");

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
    pix = pixReadStreamBmp(fp);
    fclose(fp);
    if (!pix) L_ERROR("pix not read\n", procName);
    return pix;
}


/*!
 *  pixWriteMemBmp()
 *
 *      Input:  &data (<return> data of tiff compressed image)
 *              &size (<return> size of returned data)
 *              pix
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) See pixWriteStreamBmp() for usage.  This version writes to
 *          memory instead of to a file stream.
 */
l_int32
pixWriteMemBmp(l_uint8  **pdata,
               size_t    *psize,
               PIX       *pix)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("pixWriteMemBmp");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1 );
    if (!psize)
        return ERROR_INT("&size not defined", procName, 1 );
    if (!pix)
        return ERROR_INT("&pix not defined", procName, 1 );

#if HAVE_FMEMOPEN
    if ((fp = open_memstream((char **)pdata, psize)) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    ret = pixWriteStreamBmp(fp, pix);
#else
    L_WARNING("work-around: writing to a temp file\n", procName);
    fp = tmpfile();
    ret = pixWriteStreamBmp(fp, pix);
    rewind(fp);
    *pdata = l_binaryReadStream(fp, psize);
#endif  /* HAVE_FMEMOPEN */
    fclose(fp);
    return ret;
}

/* --------------------------------------------*/
#endif  /* USE_BMPIO */
/* --------------------------------------------*/
