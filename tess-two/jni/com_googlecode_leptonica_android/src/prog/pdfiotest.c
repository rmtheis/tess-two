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
 *  pdfiotest.c
 *
 *    Note: pdftk (pdftk.exe for Windows) is required to run the
 *          last part of this test.
 */

#include <string.h>
#include "allheaders.h"

static void GetImageMask(PIX *pixs, l_int32 res, BOXA **pboxa,
                         const char *debugfile);
static PIX * QuantizeNonImageRegion(PIX *pixs, PIX *pixm, l_int32 levels);


int main(int    argc,
         char **argv)
{
char         buffer[512];
char        *tempfile1, *tempfile2;
l_uint8     *data;
l_int32      i, j, w, h, seq, ret, same;
size_t       nbytes;
const char  *title;
BOX         *box;
BOXA        *boxa1, *boxa2;
L_BYTEA     *ba;
L_PDF_DATA  *lpd;
PIX         *pix1, *pix2, *pix3, *pix4, *pix5, *pix6;
PIX         *pixs, *pixt, *pixg, *pixgc, *pixc;
static char  mainName[] = "pdfiotest";

    if (argc != 1)
        return ERROR_INT("syntax: pdfiotest", mainName, 1);
    l_pdfSetDateAndVersion(0);

#if 1
    /* ---------------  Single image tests  ------------------- */
    fprintf(stderr, "\n*** Writing single images as pdf files\n");

    convertToPdf("weasel2.4c.png", L_FLATE_ENCODE, 0, "/tmp/pdffile01.pdf",
                 0, 0, 72, "weasel2.4c.png", NULL, 0);
    convertToPdf("test24.jpg", L_JPEG_ENCODE, 0, "/tmp/pdffile02.pdf",
                 0, 0, 72, "test24.jpg", NULL, 0);
    convertToPdf("feyn.tif", L_G4_ENCODE, 0, "/tmp/pdffile03.pdf",
                 0, 0, 300, "feyn.tif", NULL, 0);

    pixs = pixRead("feyn.tif");
    pixConvertToPdf(pixs, L_G4_ENCODE, 0, "/tmp/pdffile04.pdf", 0, 0, 300,
                    "feyn.tif", NULL, 0);
    pixDestroy(&pixs);

    pixs = pixRead("test24.jpg");
    pixConvertToPdf(pixs, L_JPEG_ENCODE, 5, "/tmp/pdffile05.pdf", 0, 0, 72,
                    "test24.jpg", NULL, 0);
    pixDestroy(&pixs);

    pixs = pixRead("feyn.tif");
    pixt = pixScaleToGray2(pixs);
    pixWrite("/tmp/feyn8.png", pixt, IFF_PNG);
    convertToPdf("/tmp/feyn8.png", L_JPEG_ENCODE, 0, "/tmp/pdffile06.pdf",
                 0, 0, 150, "feyn8.png", NULL, 0);
    pixDestroy(&pixs);
    pixDestroy(&pixt);

    convertToPdf("weasel4.16g.png", L_FLATE_ENCODE, 0, "/tmp/pdffile07.pdf",
                 0, 0, 30, "weasel4.16g.png", NULL, 0);

    pixs = pixRead("test24.jpg");
    pixg = pixConvertTo8(pixs, 0);
    box = boxCreate(100, 100, 100, 100);
    pixc = pixClipRectangle(pixs, box, NULL);
    pixgc = pixClipRectangle(pixg, box, NULL);
    pixWrite("/tmp/pix32.jpg", pixc, IFF_JFIF_JPEG);
    pixWrite("/tmp/pix8.jpg", pixgc, IFF_JFIF_JPEG);
    convertToPdf("/tmp/pix32.jpg", L_FLATE_ENCODE, 0, "/tmp/pdffile08.pdf",
                 0, 0, 72, "pix32.jpg", NULL, 0);
    convertToPdf("/tmp/pix8.jpg", L_FLATE_ENCODE, 0, "/tmp/pdffile09.pdf",
                 0, 0, 72, "pix8.jpg", NULL, 0);
    pixDestroy(&pixs);
    pixDestroy(&pixg);
    pixDestroy(&pixc);
    pixDestroy(&pixgc);
    boxDestroy(&box);
#endif


#if 1
    /* ---------------  Multiple image tests  ------------------- */
    fprintf(stderr, "\n*** Writing multiple images as single page pdf files\n");

    pix1 = pixRead("feyn-fract.tif");
    pix2 = pixRead("weasel8.240c.png");

/*    l_pdfSetDateAndVersion(0); */
        /* First, write the 1 bpp image through the mask onto the weasels */
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 10; j++) {
            seq = (i == 0 && j == 0) ? L_FIRST_IMAGE : L_NEXT_IMAGE;
            title = (i == 0 && j == 0) ? "feyn-fract.tif" : NULL;
            pixConvertToPdf(pix2, L_FLATE_ENCODE, 0, NULL, 100 * j,
                            100 * i, 70, title, &lpd, seq);
        }
    }
    pixConvertToPdf(pix1, L_G4_ENCODE, 0, "/tmp/pdffile10.pdf", 0, 0, 80,
                    NULL, &lpd, L_LAST_IMAGE);

        /* Now, write the 1 bpp image over the weasels */
    l_pdfSetG4ImageMask(0);
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 10; j++) {
            seq = (i == 0 && j == 0) ? L_FIRST_IMAGE : L_NEXT_IMAGE;
            title = (i == 0 && j == 0) ? "feyn-fract.tif" : NULL;
            pixConvertToPdf(pix2, L_FLATE_ENCODE, 0, NULL, 100 * j,
                            100 * i, 70, title, &lpd, seq);
        }
    }
    pixConvertToPdf(pix1, L_G4_ENCODE, 0, "/tmp/pdffile11.pdf", 0, 0, 80,
                    NULL, &lpd, L_LAST_IMAGE);
    l_pdfSetG4ImageMask(1);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
#endif

#if 1
    /* -------- pdf convert segmented with no image regions -------- */
    fprintf(stderr, "\n*** Writing segmented images without image regions\n");

    pix1 = pixRead("rabi.png");
    pix2 = pixScaleToGray2(pix1);
    pixWrite("/tmp/rabi8.jpg", pix2, IFF_JFIF_JPEG);
    pix3 = pixThresholdTo4bpp(pix2, 16, 1);
    pixWrite("/tmp/rabi4.png", pix3, IFF_PNG);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);

        /* 1 bpp input */
    convertToPdfSegmented("rabi.png", 300, L_G4_ENCODE, 128, NULL, 0, 0,
                          NULL, "/tmp/pdffile12.pdf");
    convertToPdfSegmented("rabi.png", 300, L_JPEG_ENCODE, 128, NULL, 0, 0,
                          NULL, "/tmp/pdffile13.pdf");
    convertToPdfSegmented("rabi.png", 300, L_FLATE_ENCODE, 128, NULL, 0, 0,
                          NULL, "/tmp/pdffile14.pdf");

        /* 8 bpp input, no cmap */
    convertToPdfSegmented("/tmp/rabi8.jpg", 150, L_G4_ENCODE, 128,
                          NULL, 0, 0, NULL, "/tmp/pdffile15.pdf");
    convertToPdfSegmented("/tmp/rabi8.jpg", 150, L_JPEG_ENCODE, 128,
                          NULL, 0, 0, NULL, "/tmp/pdffile16.pdf");
    convertToPdfSegmented("/tmp/rabi8.jpg", 150, L_FLATE_ENCODE, 128,
                          NULL, 0, 0, NULL, "/tmp/pdffile17.pdf");

        /* 4 bpp input, cmap */
    convertToPdfSegmented("/tmp/rabi4.png", 150, L_G4_ENCODE, 128,
                          NULL, 0, 0, NULL, "/tmp/pdffile18.pdf");
    convertToPdfSegmented("/tmp/rabi4.png", 150, L_JPEG_ENCODE, 128,
                          NULL, 0, 0, NULL, "/tmp/pdffile19.pdf");
    convertToPdfSegmented("/tmp/rabi4.png", 150, L_FLATE_ENCODE, 128,
                          NULL, 0, 0, NULL, "/tmp/pdffile20.pdf");

#endif

#if 1
    /* ---------- pdf convert segmented with image regions ---------- */
    fprintf(stderr, "\n*** Writing segmented images with image regions\n");

        /* Get the image region(s) for rabi.png.  There are two
         * small bogus regions at the top, but we'll keep them for
         * the demonstration. */
    pix1 = pixRead("rabi.png");
    pixSetResolution(pix1, 300, 300);
    pixGetDimensions(pix1, &w, &h, NULL);
    pix2 = pixGenHalftoneMask(pix1, NULL, NULL, 0);
    pix3 = pixMorphSequence(pix2, "c20.1 + c1.20", 0);
    boxa1 = pixConnComp(pix3, NULL, 8);
    boxa2 = boxaTransform(boxa1, 0, 0, 0.5, 0.5);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);

        /* 1 bpp input */
    convertToPdfSegmented("rabi.png", 300, L_G4_ENCODE, 128, boxa1,
                          0, 0.25, NULL, "/tmp/pdffile21.pdf");
    convertToPdfSegmented("rabi.png", 300, L_JPEG_ENCODE, 128, boxa1,
                          0, 0.25, NULL, "/tmp/pdffile22.pdf");
    convertToPdfSegmented("rabi.png", 300, L_FLATE_ENCODE, 128, boxa1,
                          0, 0.25, NULL, "/tmp/pdffile23.pdf");

        /* 8 bpp input, no cmap */
    convertToPdfSegmented("/tmp/rabi8.jpg", 150, L_G4_ENCODE, 128, boxa2,
                          0, 0.5, NULL, "/tmp/pdffile24.pdf");
    convertToPdfSegmented("/tmp/rabi8.jpg", 150, L_JPEG_ENCODE, 128, boxa2,
                          0, 0.5, NULL, "/tmp/pdffile25.pdf");
    convertToPdfSegmented("/tmp/rabi8.jpg", 150, L_FLATE_ENCODE, 128, boxa2,
                          0, 0.5, NULL, "/tmp/pdffile26.pdf");

        /* 4 bpp input, cmap */
    convertToPdfSegmented("/tmp/rabi4.png", 150, L_G4_ENCODE, 128, boxa2,
                          0, 0.5, NULL, "/tmp/pdffile27.pdf");
    convertToPdfSegmented("/tmp/rabi4.png", 150, L_JPEG_ENCODE, 128, boxa2,
                          0, 0.5, NULL, "/tmp/pdffile28.pdf");
    convertToPdfSegmented("/tmp/rabi4.png", 150, L_FLATE_ENCODE, 128, boxa2,
                          0, 0.5, NULL, "/tmp/pdffile29.pdf");

        /* 4 bpp input, cmap, data output */
    data = NULL;
    convertToPdfDataSegmented("/tmp/rabi4.png", 150, L_G4_ENCODE, 128, boxa2,
                              0, 0.5, NULL, &data, &nbytes);
    l_binaryWrite("/tmp/pdffile30.pdf", "w", data, nbytes);
    lept_free(data);
    convertToPdfDataSegmented("/tmp/rabi4.png", 150, L_JPEG_ENCODE, 128, boxa2,
                              0, 0.5, NULL, &data, &nbytes);
    l_binaryWrite("/tmp/pdffile31.pdf", "w", data, nbytes);
    lept_free(data);
    convertToPdfDataSegmented("/tmp/rabi4.png", 150, L_FLATE_ENCODE, 128, boxa2,
                              0, 0.5, NULL, &data, &nbytes);
    l_binaryWrite("/tmp/pdffile32.pdf", "w", data, nbytes);
    lept_free(data);

    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
#endif


#if 1
    /* -------- pdf convert segmented from color image -------- */
    fprintf(stderr, "\n*** Writing color segmented images\n");

    pix1 = pixRead("candelabrum-11.jpg");
    pix2 = pixScale(pix1, 3.0, 3.0);
    pixWrite("/tmp/candelabrum3.jpg", pix2, IFF_JFIF_JPEG);
    GetImageMask(pix2, 200, &boxa1, "/tmp/seg1.jpg");
    convertToPdfSegmented("/tmp/candelabrum3.jpg", 200, L_G4_ENCODE,
                          100, boxa1, 0, 0.25, NULL, "/tmp/pdffile33.pdf");
    convertToPdfSegmented("/tmp/candelabrum3.jpg", 200, L_JPEG_ENCODE,
                          100, boxa1, 0, 0.25, NULL, "/tmp/pdffile34.pdf");
    convertToPdfSegmented("/tmp/candelabrum3.jpg", 200, L_FLATE_ENCODE,
                          100, boxa1, 0, 0.25, NULL, "/tmp/pdffile35.pdf");

    pixDestroy(&pix1);
    pixDestroy(&pix2);
    boxaDestroy(&boxa1);

    pix1 = pixRead("lion-page.00016.jpg");
    pix2 = pixScale(pix1, 3.0, 3.0);
    pixWrite("/tmp/lion16.jpg", pix2, IFF_JFIF_JPEG);
    pix3 = pixRead("lion-mask.00016.tif");
    boxa1 = pixConnComp(pix3, NULL, 8);
    boxa2 = boxaTransform(boxa1, 0, 0, 3.0, 3.0);
    convertToPdfSegmented("/tmp/lion16.jpg", 200, L_G4_ENCODE,
                          190, boxa2, 0, 0.5, NULL, "/tmp/pdffile36.pdf");
    convertToPdfSegmented("/tmp/lion16.jpg", 200, L_JPEG_ENCODE,
                          190, boxa2, 0, 0.5, NULL, "/tmp/pdffile37.pdf");
    convertToPdfSegmented("/tmp/lion16.jpg", 200, L_FLATE_ENCODE,
                          190, boxa2, 0, 0.5, NULL, "/tmp/pdffile38.pdf");

        /* Quantize the non-image part and flate encode.
         * This is useful because it results in a smaller file than
         * when you flate-encode the un-quantized non-image regions. */
    pix4 = pixScale(pix3, 3.0, 3.0);  /* higher res mask, for combining */
    pix5 = QuantizeNonImageRegion(pix2, pix4, 12);
    pixWrite("/tmp/lion16-quant.png", pix5, IFF_PNG);
    convertToPdfSegmented("/tmp/lion16-quant.png", 200, L_FLATE_ENCODE,
                          190, boxa2, 0, 0.5, NULL, "/tmp/pdffile39.pdf");

    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    pixDestroy(&pix4);
    pixDestroy(&pix5);
    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
#endif

#if 1
    /* ------------------ Test multipage pdf generation ----------------- */
    fprintf(stderr, "\n*** Writing multipage pdfs from single page pdfs\n");

        /* Generate a multi-page pdf from all these files */
    startTimer();
    concatenatePdf("/tmp", "pdffile", "/tmp/cat_lept.pdf");
    fprintf(stderr, "All files have been concatenated: /tmp/cat_lept.pdf\n"
                    "Concatenation time: %7.3f\n", stopTimer());
#endif

#if 1
    /* -------------------- Test corruption recovery ------------------- */
        /* Put two good pdf files in a directory */
    lept_mkdir("good");
    lept_cp("testfile1.pdf", "/tmp/good");
    lept_cp("testfile2.pdf", "/tmp/good");
    concatenatePdf("/tmp/good", "file", "/tmp/good.pdf");

        /* Make a version with the pdf id removed, so that it is not
         * recognized as a pdf */
    ba = l_byteaInitFromFile("testfile2.pdf");
    data = l_byteaGetData(ba, &nbytes);
    l_binaryWrite("testfile0.notpdf.pdf", "w", data + 10, nbytes - 10);

        /* Make a version with a corrupted trailer */
    if (data)
        data[2297] = '2';  /* munge trailer object 6: change 458 --> 428 */
    l_binaryWrite("testfile2.bad.pdf", "w", data, nbytes);

        /* Put these two bad files, along with a good file, in a directory */
    lept_mkdir("bad");
    lept_mv("testfile0.notpdf.pdf", "/tmp/bad");
    lept_cp("testfile1.pdf", "/tmp/bad");
    lept_mv("testfile2.bad.pdf", "/tmp/bad");
    l_byteaDestroy(&ba);

        /* Run concat on the bad files.   In the /tmp/bad/ directory,
         * the "not pdf" file should be ignored, and the corrupted pdf
         * file should be properly parsed, so the resulting
         * concatenated files should be identical.  */
    fprintf(stderr, "\nWe attempt to build from the bad directory\n");
    concatenatePdf("/tmp/bad", "file", "/tmp/bad.pdf");
    filesAreIdentical("/tmp/good.pdf", "/tmp/bad.pdf", &same);
    if (same)
        fprintf(stderr, "Fixed: files are the same\n"
                        "Attempt succeeded\n\n");
    else
        fprintf(stderr, "Busted: files are different\n");

        /* pdftk fails because the first file is not a pdf */
    fprintf(stderr, "pdftk attempts to build from the bad directory\n");
    tempfile1 = genPathname("/tmp/bad", "*.pdf");
    tempfile2 = genPathname("/tmp", "pdftk.bad.pdf");
    snprintf(buffer, sizeof(buffer), "pdftk %s output %s",
             tempfile1, tempfile2);
    ret = system(buffer);
    lept_free(tempfile1);
    lept_free(tempfile2);
    fprintf(stderr, "Attempt failed\n\n");
#endif

#if 1
    fprintf(stderr, "\n*** pdftk writes multipage pdfs from images\n");
    tempfile1 = genPathname("/tmp", "pdffile*.pdf");
    tempfile2 = genPathname("/tmp", "cat_pdftk.pdf");
    snprintf(buffer, sizeof(buffer), "pdftk %s output %s",
             tempfile1, tempfile2);
    ret = system(buffer);
    lept_free(tempfile1);
    lept_free(tempfile2);
#endif

#if 1
    /* -- Test simple interface for generating multi-page pdf from images -- */
    fprintf(stderr, "\n*** Writing multipage pdfs from images\n");

        /* Put four image files in a directory.  They will be encoded thus:
         *     file1.png:  flate (8 bpp, only 10 colors)
         *     file2.jpg:  dct (8 bpp, 256 colors because of the jpeg encoding)
         *     file3.tif:  g4 (1 bpp)
         *     file4.jpg:  dct (32 bpp)    */
    lept_mkdir("image");
    pix1 = pixRead("feyn.tif");
    pix2 = pixRead("rabi.png");
    pix3 = pixScaleToGray3(pix1);
    pix4 = pixScaleToGray3(pix2);
    pix5 = pixScale(pix1, 0.33, 0.33);
    pix6 = pixRead("test24.jpg");
    pixWrite("/tmp/image/file1.png", pix3, IFF_PNG);  /* 10 colors */
    pixWrite("/tmp/image/file2.jpg", pix4, IFF_JFIF_JPEG);  /* 256 colors */
    pixWrite("/tmp/image/file3.tif", pix5, IFF_TIFF_G4);
    pixWrite("/tmp/image/file4.jpg", pix6, IFF_JFIF_JPEG);

    startTimer();
    convertFilesToPdf("/tmp/image", "file", 100, 0.8, 0, 75, "4 file test",
                      "/tmp/fourimages.pdf");
    fprintf(stderr, "4-page pdf generated: /tmp/fourimages.pdf\n"
                    "Time: %7.3f\n", stopTimer());
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    pixDestroy(&pix4);
    pixDestroy(&pix5);
    pixDestroy(&pix6);
#endif

    return 0;
}


static void
GetImageMask(PIX         *pixs,
             l_int32      res,
             BOXA       **pboxa,
             const char  *debugfile)
{
PIX   *pix1, *pix2, *pix3, *pix4;
PIXA  *pixa;

    pixSetResolution(pixs, 200, 200);
    pix1 = pixConvertTo1(pixs, 100);
    pix2 = pixGenHalftoneMask(pix1, NULL, NULL, 0);
    pix3 = pixMorphSequence(pix2, "c20.1 + c1.20", 0);
    *pboxa = pixConnComp(pix3, NULL, 8);
    if (debugfile) {
        pixa = pixaCreate(0);
        pixaAddPix(pixa, pixs, L_COPY);
        pixaAddPix(pixa, pix1, L_INSERT);
        pixaAddPix(pixa, pix2, L_INSERT);
        pixaAddPix(pixa, pix3, L_INSERT);
        pix4 = pixaDisplayTiledInRows(pixa, 32, 1800, 0.25, 0, 25, 2);
        pixWrite(debugfile, pix4, IFF_JFIF_JPEG);
        pixDisplay(pix4, 100, 100);
        pixDestroy(&pix4);
        pixaDestroy(&pixa);
    } else {
        pixDestroy(&pix1);
        pixDestroy(&pix2);
        pixDestroy(&pix3);
    }

    return;
}

static PIX *
QuantizeNonImageRegion(PIX     *pixs,
                       PIX     *pixm,
                       l_int32  levels)
{
PIX  *pix1, *pix2, *pixd;

    pix1 = pixConvertTo8(pixs, 0);
    pix2 = pixThresholdOn8bpp(pix1, levels, 1);
    pixd = pixConvertTo32(pix2);  /* save in rgb */
    pixCombineMasked(pixd, pixs, pixm);  /* rgb result */
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    return pixd;
}
