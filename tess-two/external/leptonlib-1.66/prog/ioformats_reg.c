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
 * ioformats_reg.c
 *
 *    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *    This is the primary Leptonica regression test for lossless
 *    read/write I/O to standard image files (png, tiff, bmp, etc.)
 *    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *    This tests reading and writing of images in different formats
 *    It should work properly on input images of any depth, with
 *    and without colormaps.
 *
 *    The first part of the test works by doing a write/read and
 *    testing the result for equality.  We only test the lossless
 *    file formats, with pix of various depths, both with and
 *    without colormaps.  Because jpeg works fine on grayscale
 *    and rgb, there is no need for explicit tests on jpeg
 *    compression here.
 *
 *    The second part tests all different tiff compressions, for
 *    read/write that is backed both by file and by memory.
 *    For r/w to file, it is actually redundant with the first part.)
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

    /* Needed for HAVE_FMEMOPEN */
#ifdef HAVE_CONFIG_H
#include <config_auto.h>
#endif /* HAVE_CONFIG_H */

#define   BMP_FILE      "test1.bmp"
#define   FILE_1BPP     "feyn.tif"
#define   FILE_2BPP     "speckle2.png"
#define   FILE_2BPP_C   "weasel2.4g.png"
#define   FILE_4BPP     "speckle4.png"
#define   FILE_4BPP_C   "weasel4.16c.png"
#define   FILE_8BPP_1   "dreyfus8.png"
#define   FILE_8BPP_2   "weasel8.240c.png"
#define   FILE_8BPP_3   "test8.jpg"
#define   FILE_16BPP    "test16.tif"
#define   FILE_32BPP    "marge.jpg"

static l_int32 testcomp(const char *filename, PIX *pix, l_int32 comptype);
static l_int32 testcomp_mem(PIX *pixs, PIX **ppixt, l_int32 index,
                            l_int32 format);
static l_int32 test_writemem(PIX *pixs, l_int32 format, char *psfile);
static PIX *make_24_bpp_pix(PIX *pixs);
static l_int32 get_header_data(const char *filename, l_int32 true_format);
static void get_tiff_compression_name(char *buf, l_int32 format);

LEPT_DLL extern const char *ImageFileFormatExtensions[];


main(int    argc,
     char **argv)
{
char         psname[256];
char        *tempname;
l_uint8     *data;
l_int32      i, d, n, success, failure, nbytes, same, display;
l_int32      w, h, bps, spp;
l_float32    diff;
size_t       size;
FILE        *fp;
PIX         *pix1, *pix2, *pix4, *pix8, *pix16, *pix32;
PIX         *pix, *pixt, *pixd;
PIXA        *pixa;

    if (regTestSetup(argc, argv, &fp, &display, &success, NULL))
        return 1;

    /* --------- Part 1: Test all lossless formats for r/w to file ---------*/

    failure = FALSE;
    success = TRUE;
    fprintf(stderr, "Test bmp 1 bpp file:\n");
    if (ioFormatTest(BMP_FILE)) success = FALSE;
    fprintf(stderr, "\nTest other 1 bpp file:\n");
    if (ioFormatTest(FILE_1BPP)) success = FALSE;
    fprintf(stderr, "\nTest 2 bpp file:\n");
    if (ioFormatTest(FILE_2BPP)) success = FALSE;
    fprintf(stderr, "\nTest 2 bpp file with cmap:\n");
    if (ioFormatTest(FILE_2BPP_C)) success = FALSE;
    fprintf(stderr, "\nTest 4 bpp file:\n");
    if (ioFormatTest(FILE_4BPP)) success = FALSE;
    fprintf(stderr, "\nTest 4 bpp file with cmap:\n");
    if (ioFormatTest(FILE_4BPP_C)) success = FALSE;
    fprintf(stderr, "\nTest 8 bpp grayscale file with cmap:\n");
    if (ioFormatTest(FILE_8BPP_1)) success = FALSE;
    fprintf(stderr, "\nTest 8 bpp color file with cmap:\n");
    if (ioFormatTest(FILE_8BPP_2)) success = FALSE;
    fprintf(stderr, "\nTest 8 bpp file without cmap:\n");
    if (ioFormatTest(FILE_8BPP_3)) success = FALSE;
    fprintf(stderr, "\nTest 16 bpp file:\n");
    if (ioFormatTest(FILE_16BPP)) success = FALSE;
    fprintf(stderr, "\nTest 32 bpp file:\n");
    if (ioFormatTest(FILE_32BPP)) success = FALSE;
    if (success)
        fprintf(stderr,
            "\n  ********** Success on all i/o format tests *********\n");
    else
        fprintf(stderr,
            "\n  ******* Failure on at least one i/o format test ******\n");
    if (!success) failure = TRUE;

    /* ------------------ Part 2: Test tiff r/w to file ------------------- */

    fprintf(stderr, "\nTest tiff r/w and format extraction\n");
    pixa = pixaCreate(6);
    pix1 = pixRead(BMP_FILE);
    pix2 = pixConvert1To2(NULL, pix1, 3, 0);
    pix4 = pixConvert1To4(NULL, pix1, 15, 0);
    pix16 = pixRead(FILE_16BPP);
    fprintf(stderr, "Input format: %d\n", pixGetInputFormat(pix16));
    pix8 = pixConvert16To8(pix16, 1);
    pix32 = pixRead(FILE_32BPP);
    pixaAddPix(pixa, pix1, L_INSERT);
    pixaAddPix(pixa, pix2, L_INSERT);
    pixaAddPix(pixa, pix4, L_INSERT);
    pixaAddPix(pixa, pix8, L_INSERT);
    pixaAddPix(pixa, pix16, L_INSERT);
    pixaAddPix(pixa, pix32, L_INSERT);
    n = pixaGetCount(pixa);

    success = TRUE;
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
	d = pixGetDepth(pix);
        fprintf(stderr, "%d bpp\n", d);
	if (i == 0) {   /* 1 bpp */
            pixWrite("/tmp/junkg3.tif", pix, IFF_TIFF_G3);
            pixWrite("/tmp/junkg4.tif", pix, IFF_TIFF_G4);
            pixWrite("/tmp/junkrle.tif", pix, IFF_TIFF_RLE);
            pixWrite("/tmp/junkpb.tif", pix, IFF_TIFF_PACKBITS);
	    if (testcomp("/tmp/junkg3.tif", pix, IFF_TIFF_G3)) success = FALSE;
	    if (testcomp("/tmp/junkg4.tif", pix, IFF_TIFF_G4)) success = FALSE;
	    if (testcomp("/tmp/junkrle.tif", pix, IFF_TIFF_RLE))
                success = FALSE;
	    if (testcomp("/tmp/junkpb.tif", pix, IFF_TIFF_PACKBITS))
                success = FALSE;
	}
        pixWrite("/tmp/junklzw.tif", pix, IFF_TIFF_LZW);
        pixWrite("/tmp/junkzip.tif", pix, IFF_TIFF_ZIP);
        pixWrite("/tmp/junknon.tif", pix, IFF_TIFF);
        if (testcomp("/tmp/junklzw.tif", pix, IFF_TIFF_LZW)) success = FALSE;
        if (testcomp("/tmp/junkzip.tif", pix, IFF_TIFF_ZIP)) success = FALSE;
        if (testcomp("/tmp/junknon.tif", pix, IFF_TIFF)) success = FALSE;
	pixDestroy(&pix);
    }
    if (success)
        fprintf(stderr,
            "\n  ********** Success on tiff r/w to file *********\n\n");
    else
        fprintf(stderr,
            "\n  ******* Failure on at least one tiff r/w to file ******\n\n");
    if (!success) failure = TRUE;

    /* ------------------ Part 3: Test tiff r/w to memory ----------------- */

    success = TRUE;
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
	d = pixGetDepth(pix);
        fprintf(stderr, "%d bpp\n", d);
	if (i == 0) {   /* 1 bpp */
            pixWriteMemTiff(&data, &size, pix, IFF_TIFF_G3);
            nbytes = nbytesInFile("/tmp/junkg3.tif");
            fprintf(stderr, "nbytes = %d, size = %ld\n", nbytes, size);
            pixt = pixReadMemTiff(data, size, 0);
            if (testcomp_mem(pix, &pixt, i, IFF_TIFF_G3)) success = FALSE;
	    FREE(data);
            pixWriteMemTiff(&data, &size, pix, IFF_TIFF_G4);
            nbytes = nbytesInFile("/tmp/junkg4.tif");
            fprintf(stderr, "nbytes = %d, size = %ld\n", nbytes, size);
            pixt = pixReadMemTiff(data, size, 0);
            if (testcomp_mem(pix, &pixt, i, IFF_TIFF_G4)) success = FALSE;
            readHeaderMemTiff(data, size, 0, &w, &h, &bps, &spp,
                              NULL, NULL, NULL);
            fprintf(stderr, "(w,h,bps,spp) = (%d,%d,%d,%d)\n", w, h, bps, spp);
	    FREE(data);
            pixWriteMemTiff(&data, &size, pix, IFF_TIFF_RLE);
            nbytes = nbytesInFile("/tmp/junkrle.tif");
            fprintf(stderr, "nbytes = %d, size = %ld\n", nbytes, size);
            pixt = pixReadMemTiff(data, size, 0);
            if (testcomp_mem(pix, &pixt, i, IFF_TIFF_RLE)) success = FALSE;
	    FREE(data);
            pixWriteMemTiff(&data, &size, pix, IFF_TIFF_PACKBITS);
            nbytes = nbytesInFile("/tmp/junkpb.tif");
            fprintf(stderr, "nbytes = %d, size = %ld\n", nbytes, size);
            pixt = pixReadMemTiff(data, size, 0);
            if (testcomp_mem(pix, &pixt, i, IFF_TIFF_PACKBITS)) success = FALSE;
	    FREE(data);
	}
        pixWriteMemTiff(&data, &size, pix, IFF_TIFF_LZW);
        pixt = pixReadMemTiff(data, size, 0);
        if (testcomp_mem(pix, &pixt, i, IFF_TIFF_LZW)) success = FALSE;
        FREE(data);
        pixWriteMemTiff(&data, &size, pix, IFF_TIFF_ZIP);
        pixt = pixReadMemTiff(data, size, 0);
        if (testcomp_mem(pix, &pixt, i, IFF_TIFF_ZIP)) success = FALSE;
        readHeaderMemTiff(data, size, 0, &w, &h, &bps, &spp, NULL, NULL, NULL);
        fprintf(stderr, "(w,h,bps,spp) = (%d,%d,%d,%d)\n", w, h, bps, spp);
        FREE(data);
        pixWriteMemTiff(&data, &size, pix, IFF_TIFF);
        pixt = pixReadMemTiff(data, size, 0);
        if (testcomp_mem(pix, &pixt, i, IFF_TIFF)) success = FALSE;
        FREE(data);
        pixDestroy(&pix);
    }
    if (success)
        fprintf(stderr,
            "\n  ********** Success on tiff r/w to memory *********\n\n");
    else
        fprintf(stderr,
            "\n  ******* Failure on at least one tiff r/w to memory ******\n\n");
    if (!success) failure = TRUE;


    /* ---------------- Part 4: Test non-tiff r/w to memory ---------------- */

#if HAVE_FMEMOPEN
    pixDisplayWrite(NULL, -1);
    success = TRUE;
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
	d = pixGetDepth(pix);
        sprintf(psname, "/tmp/junkps.%d", d);
        fprintf(stderr, "%d bpp\n", d);
        if (d != 16) {
            if (test_writemem(pix, IFF_PNG, NULL)) success = FALSE;
            if (test_writemem(pix, IFF_BMP, NULL)) success = FALSE;
        }
        if (test_writemem(pix, IFF_PNM, NULL)) success = FALSE;
        if (test_writemem(pix, IFF_PS, psname)) success = FALSE;
	if (d == 8 || d == 32)
            if (test_writemem(pix, IFF_JFIF_JPEG, NULL)) success = FALSE;
        pixDestroy(&pix);
    }
    if (success)
        fprintf(stderr,
            "\n  ********** Success on non-tiff r/w to memory *********\n\n");
    else
        fprintf(stderr,
            "\n  **** Failure on at least one non-tiff r/w to memory *****\n\n");
    if (!success) failure = TRUE;
#else
        fprintf(stderr,
            "\n  ***** Non-tiff r/w to memory not enabled *****\n\n");
#endif  /*  HAVE_FMEMOPEN  */

    pixaDestroy(&pixa);

    /* ------------ Part 5: Test multipage tiff r/w to memory ------------ */

        /* Make a multipage tiff file, and read it back into memory */
    success = TRUE;
    pix = pixRead("feyn.tif");
    pixa = pixaSplitPix(pix, 3, 3, 0, 0);
    for (i = 0; i < 9; i++) {
        pixt = pixaGetPix(pixa, i, L_CLONE);
        if (i == 0)
            pixWriteTiff("/tmp/junktiffmpage.tif", pixt, IFF_TIFF_G4, "w");
        else
            pixWriteTiff("/tmp/junktiffmpage.tif", pixt, IFF_TIFF_G4, "a");
        pixDestroy(&pixt);
    }
    data = arrayRead("/tmp/junktiffmpage.tif", &nbytes);
    pixaDestroy(&pixa);

        /* Read the individual pages from memory to a pix */
    pixa = pixaCreate(9);
    for (i = 0; i < 9; i++) {
        pixt = pixReadMemTiff(data, nbytes, i);
        pixaAddPix(pixa, pixt, L_INSERT);
    }
    FREE(data);

        /* Un-tile the pix in the pixa back to the original image */
    pixt = pixaDisplayUnsplit(pixa, 3, 3, 0, 0);
    pixaDestroy(&pixa);

        /* Clip to foreground to remove any extra rows or columns */
    pixClipToForeground(pix, &pix1, NULL);
    pixClipToForeground(pixt, &pix2, NULL);
    pixEqual(pix1, pix2, &same); 
    if (same)
        fprintf(stderr,
            "\n  ******* Success on tiff multipage read from memory ******\n\n");
    else
        fprintf(stderr,
            "\n  ******* Failure on tiff multipage read from memory ******\n\n");
    if (!same) failure = TRUE;

    pixDestroy(&pix);
    pixDestroy(&pixt);
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    /* ------------ Part 6: Test 24 bpp writing ------------ */

        /* Generate a 24 bpp (not 32 bpp !!) rgb pix and write it out */
    success = TRUE;
    pix = pixRead("marge.jpg");
    pixt = make_24_bpp_pix(pix);
    pixWrite("/tmp/junk24.png", pixt, IFF_PNG);
    pixWrite("/tmp/junk24.jpg", pixt, IFF_JFIF_JPEG);
    pixWrite("/tmp/junk24.tif", pixt, IFF_TIFF);
    pixd = pixRead("/tmp/junk24.png");
    pixEqual(pix, pixd, &same);
    if (!same) success = FALSE;
    pixDestroy(&pixd);
    pixd = pixRead("/tmp/junk24.jpg");
    regTestCompareSimilarPix(fp, argv, pix, pixd, 10, 0.0002, 0, &success, 0);
    pixDestroy(&pixd);
    pixd = pixRead("/tmp/junk24.tif");
    pixEqual(pix, pixd, &same);
    if (!same) success = FALSE;
    pixDestroy(&pixd);
    if (success)
        fprintf(stderr,
            "\n  ******* Success on 24 bpp rgb writing *******\n\n");
    else
        fprintf(stderr,
            "\n  ******* Failure on 24 bpp rgb writing *******\n\n");
    if (!success) failure = TRUE;
    pixDestroy(&pix);
    pixDestroy(&pixt);

    /* -------------- Part 7: Read header information -------------- */
    success = TRUE;
    if (get_header_data(FILE_1BPP, IFF_TIFF_G4)) success = FALSE;
    if (get_header_data(FILE_2BPP, IFF_PNG)) success = FALSE;
    if (get_header_data(FILE_2BPP_C, IFF_PNG)) success = FALSE;
    if (get_header_data(FILE_4BPP, IFF_PNG)) success = FALSE;
    if (get_header_data(FILE_4BPP_C, IFF_PNG)) success = FALSE;
    if (get_header_data(FILE_8BPP_1, IFF_PNG)) success = FALSE;
    if (get_header_data(FILE_8BPP_2, IFF_PNG)) success = FALSE;
    if (get_header_data(FILE_8BPP_3, IFF_JFIF_JPEG)) success = FALSE;
    if (get_header_data(FILE_16BPP, IFF_TIFF_ZIP)) success = FALSE;
    if (get_header_data(FILE_32BPP, IFF_JFIF_JPEG)) success = FALSE;

#if HAVE_FMEMOPEN
    pix = pixRead(FILE_8BPP_1);
    tempname = genTempFilename((const char *)"/tmp", (const char *)".pnm", 1);
    pixWrite(tempname, pix, IFF_PNM);
    if (get_header_data(tempname, IFF_PNM)) success = FALSE;
    pixDestroy(&pix);
#endif  /* HAVE_FMEMOPEN */

    pix = pixRead(FILE_1BPP);
    tempname = genTempFilename((const char *)"/tmp", (const char *)".tif", 1);
    pixWrite(tempname, pix, IFF_TIFF_G3);
    if (get_header_data(tempname, IFF_TIFF_G3)) success = FALSE;
    pixWrite(tempname, pix, IFF_TIFF_G4);
    if (get_header_data(tempname, IFF_TIFF_G4)) success = FALSE;
    pixWrite(tempname, pix, IFF_TIFF_PACKBITS);
    if (get_header_data(tempname, IFF_TIFF_PACKBITS)) success = FALSE;
    pixWrite(tempname, pix, IFF_TIFF_RLE);
    if (get_header_data(tempname, IFF_TIFF_RLE)) success = FALSE;
    pixWrite(tempname, pix, IFF_TIFF_LZW);
    if (get_header_data(tempname, IFF_TIFF_LZW)) success = FALSE;
    pixWrite(tempname, pix, IFF_TIFF_ZIP);
    if (get_header_data(tempname, IFF_TIFF_ZIP)) success = FALSE;
    pixWrite(tempname, pix, IFF_TIFF);
    if (get_header_data(tempname, IFF_TIFF)) success = FALSE;
    pixDestroy(&pix);
    FREE(tempname);

    if (success)
        fprintf(stderr,
            "\n  ******* Success on reading headers *******\n\n");
    else
        fprintf(stderr,
            "\n  ******* Failure on reading headers *******\n\n");
    if (!success) failure = TRUE;

    if (!failure)
        fprintf(stderr,
            "  ******* Success on all tests *******\n\n");
    else
        fprintf(stderr,
            "  ******* Failure on at least one test *******\n\n");

    regTestCleanup(argc, argv, fp, success, NULL);
    return 0;
}


    /* Returns 1 on error */
static l_int32
testcomp(const char  *filename,
         PIX         *pix,
         l_int32      comptype)
{
l_int32  format, sameformat, sameimage;
FILE    *fp;
PIX     *pixt;

    fp = fopen(filename, "rb");
    findFileFormat(fp, &format);
    sameformat = TRUE;
    if (format != comptype) {
        fprintf(stderr, "File %s has format %d, not comptype %d\n",
                filename, format, comptype);
        sameformat = FALSE;
    }
    fclose(fp);
    pixt = pixRead(filename);
    pixEqual(pix, pixt, &sameimage);
    pixDestroy(&pixt);
    if (!sameimage)
        fprintf(stderr, "Write/read fail for file %s with format %d\n",
                filename, format);
    return (!sameformat || !sameimage);
}


    /* Returns 1 on error */
static l_int32
testcomp_mem(PIX     *pixs,
             PIX    **ppixt,
             l_int32  index,
             l_int32  format)
{
l_int32  sameimage;
PIX     *pixt;

    pixt = *ppixt;
    pixEqual(pixs, pixt, &sameimage);
    if (!sameimage)
        fprintf(stderr, "Mem Write/read fail for file %d with format %d\n",
                index, format);
    pixDestroy(&pixt);
    *ppixt = NULL;
    return (!sameimage);
}


    /* Returns 1 on error */
static l_int32
test_writemem(PIX      *pixs,
              l_int32   format,
              char     *psfile)
{
l_uint8  *data = NULL;
l_int32   same;
size_t    size = 0;
PIX      *pixd = NULL;

    if (format == IFF_PS) {
       pixWriteMemPS(&data, &size, pixs, NULL, 0, 1.0);
       arrayWrite(psfile, "w", data, size);
       FREE(data);
       return 0;
    }

    if (pixWriteMem(&data, &size, pixs, format)) {
        fprintf(stderr, "Mem write fail for format %d\n", format);
        return 1;
    }
    if ((pixd = pixReadMem(data, size)) == NULL) {
        fprintf(stderr, "Mem read fail for format %d\n", format);
        FREE(data);
        return 1;
    }
    if (format == IFF_JFIF_JPEG) {
        fprintf(stderr, "jpeg size = %ld\n", size);
        pixDisplayWrite(pixd, 1);
        same = TRUE;
    }
    else {
        pixEqual(pixs, pixd, &same);
        if (!same)
           fprintf(stderr, "Mem write/read fail for format %d\n", format);
    }
    pixDestroy(&pixd);
    FREE(data);
    return (!same);
}


    /* Composes 24 bpp rgb pix */
static PIX *
make_24_bpp_pix(PIX  *pixs)
{
l_int32    i, j, w, h, wpls, wpld, rval, gval, bval;
l_uint32  *lines, *lined, *datas, *datad;
PIX       *pixd;

    pixGetDimensions(pixs, &w, &h, NULL);
    pixd = pixCreate(w, h, 24);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            extractRGBValues(lines[j], &rval, &gval, &bval);
            *((l_uint8 *)lined + 3 * j) = rval;
            *((l_uint8 *)lined + 3 * j + 1) = gval;
            *((l_uint8 *)lined + 3 * j + 2) = bval;
        }
    }

    return pixd;
}


    /* Retrieve header data from file */
static l_int32
get_header_data(const char  *filename,
                l_int32      true_format)
{
char      buf[64];
l_uint8  *data;
l_int32   ret1, ret2, nbytes1, nbytes2, format1, format2;
l_int32   w1, w2, h1, h2, d1, d2, bps1, bps2, spp1, spp2, iscmap1, iscmap2;

        /* Read header from file */
    nbytes1 = nbytesInFile(filename);
    ret1 = pixReadHeader(filename, &format1, &w1, &h1, &bps1, &spp1, &iscmap1);
    d1 = bps1 * spp1;
    if (d1 == 24) d1 = 32;
    if (ret1)
        fprintf(stderr, "Error: couldn't read header data: %s\n", filename);
    else {
        if (format1 > IFF_PNG && format1 < IFF_PNM) {
            get_tiff_compression_name(buf, format1);
            fprintf(stderr, "Format data for image %s with format %s:\n"
                "  nbytes = %d, size (w, h, d) = (%d, %d, %d)\n"
                "  bps = %d, spp = %d, iscmap = %d\n",
                filename, buf, nbytes1, w1, h1, d1, bps1, spp1, iscmap1);
        } else {
            fprintf(stderr, "Format data for image %s with format %s:\n"
                "  nbytes = %d, size (w, h, d) = (%d, %d, %d)\n"
                "  bps = %d, spp = %d, iscmap = %d\n",
                filename, ImageFileFormatExtensions[format1], nbytes1,
                w1, h1, d1, bps1, spp1, iscmap1);
        }
        if (format1 != true_format) {
            fprintf(stderr, "Error: format is %d; should be %d\n",
                    format1, true_format);
            ret1 = 1;
        }
    }

        /* Read header from array in memory */
    data = arrayRead(filename, &nbytes2);
    ret2 = pixReadHeaderMem(data, nbytes2, &format2, &w2, &h2, &bps2,
                            &spp2, &iscmap2);
    d2 = bps2 * spp2;
    if (d2 == 24) d2 = 32;
    if (ret2)
        fprintf(stderr, "Error: couldn't mem-read header data: %s\n", filename);
    else {
        if (nbytes1 != nbytes2 || format1 != format2 || w1 != w2 ||
            h1 != h2 || d1 != d2 || bps1 != bps2 || spp1 != spp2 ||
            iscmap1 != iscmap2) {
            fprintf(stderr, "Incomsistency reading image %s with format %s\n",
                    filename, buf);
            ret2 = 1;
        }
    }
    return ret1 || ret2;
}


static void
get_tiff_compression_name(char    *buf,
                          l_int32  format)
{
    if (format == IFF_TIFF_G4)
        sprintf(buf, "tiff_g4");
    else if (format == IFF_TIFF_G3)
        sprintf(buf, "tiff_g3");
    else if (format == IFF_TIFF_ZIP)
        sprintf(buf, "tiff_zip");
    else if (format == IFF_TIFF_LZW)
        sprintf(buf, "tiff_lzw");
    else if (format == IFF_TIFF_RLE)
        sprintf(buf, "tiff_rle");
    else if (format == IFF_TIFF_PACKBITS)
        sprintf(buf, "tiff_packbits");
    else if (format == IFF_TIFF)
        sprintf(buf, "tiff_uncompressed");
    else
        fprintf(stderr, "format %d: not tiff\n", format);
    return;
}


