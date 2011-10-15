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
 *  bmpio.c
 *
 *      Read bmp from file
 *           PIX          *pixReadStreamBmp()
 *
 *      Write bmp to file
 *           l_int32       pixWriteStreamBmp()
 *
 *      Read/write to memory   [only on linux]
 *           PIX          *pixReadMemBmp()
 *           l_int32       pixWriteMemBmp()
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "allheaders.h"
#include "bmp.h"

/* --------------------------------------------*/
#if  USE_BMPIO   /* defined in environ.h */
/* --------------------------------------------*/

RGBA_QUAD   bwmap[2] = { {255,255,255,0}, {0,0,0,0} };

#ifndef  NO_CONSOLE_IO
#define  DEBUG     0
#endif  /* ~NO_CONSOLE_IO */


/*!
 *  pixReadStreamBmp()
 *
 *      Input:  stream opened for read
 *      Return: pix, or null on error
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
l_int32    fileBpl, extrabytes, readerror;
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
    fread((char *)&sval, 1, 2, fp);
    bfType = convertOnBigEnd16(sval);
    if (bfType != BMP_ID)
        return (PIX *)ERROR_PTR("not bmf format", procName, NULL);

    fread((char *)&sval, 1, 2, fp);
    bfSize = convertOnBigEnd16(sval);
    fread((char *)&sval, 1, 2, fp);
    bfFill1 = convertOnBigEnd16(sval);
    fread((char *)&sval, 1, 2, fp);
    bfReserved1 = convertOnBigEnd16(sval);
    fread((char *)&sval, 1, 2, fp);
    bfReserved2 = convertOnBigEnd16(sval);
    fread((char *)&sval, 1, 2, fp);
    offset = convertOnBigEnd16(sval);
    fread((char *)&sval, 1, 2, fp);
    bfFill2 = convertOnBigEnd16(sval);

        /* Read bitmap info header */
    fread((char *)&ival, 1, 4, fp);
    biSize = convertOnBigEnd32(ival);
    fread((char *)&ival, 1, 4, fp);
    width = convertOnBigEnd32(ival);
    fread((char *)&ival, 1, 4, fp);
    height = convertOnBigEnd32(ival);
    fread((char *)&sval, 1, 2, fp);
    biPlanes = convertOnBigEnd16(sval);
    fread((char *)&sval, 1, 2, fp);
    depth = convertOnBigEnd16(sval);
    fread((char *)&ival, 1, 4, fp);
    compression = convertOnBigEnd32(ival);
    fread((char *)&ival, 1, 4, fp);
    imagebytes = convertOnBigEnd32(ival);
    fread((char *)&ival, 1, 4, fp);
    xres = convertOnBigEnd32(ival);
    fread((char *)&ival, 1, 4, fp);
    yres = convertOnBigEnd32(ival);
    fread((char *)&ival, 1, 4, fp);
    biClrUsed = convertOnBigEnd32(ival);
    fread((char *)&ival, 1, 4, fp);
    biClrImportant = convertOnBigEnd32(ival);

    if (compression != 0)
        return (PIX *)ERROR_PTR("cannot read compressed BMP files",
                                procName,NULL);

        /* A little sanity checking.  It would be nice to check
         * if the number of bytes in the file equals the offset to
         * the data plus the imagedata, but this won't work when
         * reading from memory, because fmemopen() doesn't implement
         * ftell().  So we can't do that check. */
    if (width < 1)
        return (PIX *)ERROR_PTR("width < 1", procName,NULL);
    if (height < 1)
        return (PIX *)ERROR_PTR("height < 1", procName,NULL);
    if (depth < 1 || depth > 32)
        return (PIX *)ERROR_PTR("depth not in [1 ... 32]", procName,NULL);
    fileBpl = 4 * ((width * depth + 31)/32);
    if (imagebytes != fileBpl * height)
        return (PIX *)ERROR_PTR("invalid imagebytes", procName,NULL);
    if (offset < BMP_FHBYTES + BMP_IHBYTES)
        return (PIX *)ERROR_PTR("invalid offset: too small", procName,NULL);
    if (offset > BMP_FHBYTES + BMP_IHBYTES + 4 * 256)
        return (PIX *)ERROR_PTR("invalid offset: too large", procName,NULL);

        /* Handle the colormap */
    colormapEntries = (offset - BMP_FHBYTES - BMP_IHBYTES) / sizeof(RGBA_QUAD);
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
    if ((pix = pixCreate(width, height, d)) == NULL)
        return (PIX *)ERROR_PTR( "pix not made", procName, NULL);
    pixSetXRes(pix, (l_int32)((l_float32)xres / 39.37 + 0.5));  /* to ppi */
    pixSetYRes(pix, (l_int32)((l_float32)yres / 39.37 + 0.5));  /* to ppi */
    pixWpl = pixGetWpl(pix);
    pixBpl = 4 * pixWpl;

    cmap = NULL;
    if (colormapEntries > 256)
        L_WARNING("more than 256 colormap entries!", procName);
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
    }
    else {  /*  24 bpp file; 32 bpp pix
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
        readerror = 0;
        extrabytes = fileBpl - 3 * width;
        line = pixGetData(pix) + pixWpl * (height - 1);
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                pword = line + j;
                if (fread(&pel, 1, 3, fp) != 3)
                    readerror = 1;
                *((l_uint8 *)pword + COLOR_RED) = pel[2];
                *((l_uint8 *)pword + COLOR_GREEN) = pel[1];
                *((l_uint8 *)pword + COLOR_BLUE) = pel[0];
            }
            if (extrabytes) {
                for (k = 0; k < extrabytes; k++)
                    fread(&pel, 1, 1, fp);
            }
            line -= pixWpl;
        }
        if (readerror) {
            pixDestroy(&pix);
            return (PIX *)ERROR_PTR("BMP read fail", procName, NULL);
        }
    }

    pixEndianByteSwap(pix);

        /* ----------------------------------------------
         * The bmp colormap determines the values of black
         * and white pixels for binary in the following way:
         * if black = 1 (255), white = 0
         *      255, 255, 255, 0, 0, 0, 0, 0
         * if black = 0, white = 1 (255)
         *      0, 0, 0, 0, 255, 255, 255, 0
         * We have no need for a 1 bpp pix with a colormap!
         * ---------------------------------------------- */
    if (depth == 1 && cmap) {
/*        L_INFO("Removing colormap", procName); */
        pixt = pixRemoveColormap(pix, REMOVE_CMAP_BASED_ON_SRC);
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
        L_WARNING("writing 2 bpp bmp file; nobody else can read", procName);
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
    }
    else if ((cmap = pixGetColormap(pix))) {   /* existing colormap */
        ncolors = pixcmapGetCount(cmap);
        cmaplen = ncolors * sizeof(RGBA_QUAD);
        cta = (l_uint8 *)cmap->array;
    }
    else {   /* no existing colormap; make a binary or gray one */
        if (d == 1) {
            cmaplen  = sizeof(bwmap);
            ncolors = 2;
            cta = (l_uint8 *)bwmap;
        }
        else {   /* d != 32; output grayscale version */
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
    }
    else {  /* 32 bpp pix; 24 bpp file
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
l_uint8  *data;
FILE     *fp;
PIX      *pix;

    PROCNAME("pixReadMemBmp");

    if (!cdata)
        return (PIX *)ERROR_PTR("cdata not defined", procName, NULL);

    data = (l_uint8 *)cdata;  /* we're really not going to change this */
    if ((fp = fmemopen(data, size, "r")) == NULL)
        return (PIX *)ERROR_PTR("stream not opened", procName, NULL);
    pix = pixReadStreamBmp(fp);
    fclose(fp);
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

    if ((fp = open_memstream((char **)pdata, psize)) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    ret = pixWriteStreamBmp(fp, pix);
    fclose(fp);
    return ret;
}

#else

PIX *
pixReadMemBmp(const l_uint8  *cdata,
              size_t          size)
{
    return (PIX *)ERROR_PTR(
        "bmp read from memory not implemented on this platform",
        "pixReadMemBmp", NULL);
}


l_int32
pixWriteMemBmp(l_uint8  **pdata,
               size_t    *psize,
               PIX       *pix)
{
    return ERROR_INT(
        "bmp write to memory not implemented on this platform",
        "pixWriteMemBmp", 1);
}

#endif  /* HAVE_FMEMOPEN */

/* --------------------------------------------*/
#endif  /* USE_BMPIO */
/* --------------------------------------------*/

