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
 * mtifftest.c
 *
 *   Tests tiff I/O for:
 *
 *       -- multipage tiff read/write
 *       -- writing special tiff tags to file
 */

#include "allheaders.h"
#include <string.h>

static const char *weasel_rev = "/tmp/tiff/weasel_rev";
static const char *weasel_rev_rev = "/tmp/tiff/weasel_rev_rev";
static const char *weasel_orig = "/tmp/tiff/weasel_orig";


int main(int    argc,
         char **argv)
{
char        *fname, *filename;
const char  *str;
char         buffer[512];
l_int32      i, npages;
size_t       length;
FILE        *fp;
NUMA        *naflags, *nasizes;
PIX         *pix, *pix1, *pix2, *pixd;
PIXA        *pixa;
PIXCMAP     *cmap;
SARRAY      *savals, *satypes, *sa;
static char  mainName[] = "mtifftest";

    if (argc != 1)
        return ERROR_INT(" Syntax:  mtifftest", mainName, 1);

    lept_mkdir("tiff");

#if 1   /* ------------------  Test multipage I/O  -------------------*/
        /* This puts every image file in the directory with a string
         * match to "weasel" into a multipage tiff file.
         * Images with 1 bpp are coded as g4; the others as zip.
         * It then reads back into a pix and displays.  */
    writeMultipageTiff(".", "weasel8.", "/tmp/tiff/weasel8.tif");
    pixa = pixaReadMultipageTiff("/tmp/tiff/weasel8.tif");
    pixd = pixaDisplayTiledInRows(pixa, 1, 1200, 0.5, 0, 15, 4);
    pixDisplay(pixd, 100, 0);
    pixDestroy(&pixd);
    pixd = pixaDisplayTiledInRows(pixa, 8, 1200, 0.8, 0, 15, 4);
    pixDisplay(pixd, 100, 200);
    pixDestroy(&pixd);
    pixd = pixaDisplayTiledInRows(pixa, 32, 1200, 1.2, 0, 15, 4);
    pixDisplay(pixd, 100, 400);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
#endif

#if 1   /* ------------ Test single-to-multipage I/O  -------------------*/
        /* Read the files and generate a multipage tiff file of G4 images.
         * Then convert that to a G4 compressed and ascii85 encoded PS file. */
    sa = getSortedPathnamesInDirectory(".", "weasel4.", 0, 4);
    sarrayWriteStream(stderr, sa);
    sarraySort(sa, sa, L_SORT_INCREASING);
    sarrayWriteStream(stderr, sa);
    npages = sarrayGetCount(sa);
    for (i = 0; i < npages; i++) {
        fname = sarrayGetString(sa, i, 0);
        filename = genPathname(".", fname);
        pix1 = pixRead(filename);
        if (!pix1) continue;
        pix2 = pixConvertTo1(pix1, 128);
        if (i == 0)
            pixWriteTiff("/tmp/tiff/weasel4", pix2, IFF_TIFF_G4, "w+");
        else
            pixWriteTiff("/tmp/tiff/weasel4", pix2, IFF_TIFF_G4, "a");
        pixDestroy(&pix1);
        pixDestroy(&pix2);
        lept_free(filename);
    }

        /* Write it out as a PS file */
    convertTiffMultipageToPS("/tmp/tiff/weasel4", "/tmp/tiff/weasel4.ps",
                             NULL, 0.95);
    sarrayDestroy(&sa);
#endif

#if 1   /* ------------------  Test multipage I/O  -------------------*/
        /* Read count of pages in tiff multipage  file */
    writeMultipageTiff(".", "weasel2", weasel_orig);
    fp = lept_fopen(weasel_orig, "rb");
    if (fileFormatIsTiff(fp)) {
        tiffGetCount(fp, &npages);
        fprintf(stderr, " Tiff: %d page\n", npages);
    }
    else
        return ERROR_INT(" file not tiff", mainName, 1);
    lept_fclose(fp);

        /* Split into separate page files */
    for (i = 0; i < npages + 1; i++) {   /* read one beyond to catch error */
        if (i == npages)
            L_INFO("Errors in next 2 lines are intentional!\n", mainName);
        pix = pixReadTiff(weasel_orig, i);
        if (!pix) continue;
        sprintf(buffer, "/tmp/tiff/%03d.tif", i);
        pixWrite(buffer, pix, IFF_TIFF_ZIP);
        pixDestroy(&pix);
    }

        /* Read separate page files and write reversed file */
    for (i = npages - 1; i >= 0; i--) {
        sprintf(buffer, "/tmp/tiff/%03d.tif", i);
        pix = pixRead(buffer);
        if (!pix) continue;
        if (i == npages - 1)
            pixWriteTiff(weasel_rev, pix, IFF_TIFF_ZIP, "w+");
        else
            pixWriteTiff(weasel_rev, pix, IFF_TIFF_ZIP, "a");
        pixDestroy(&pix);
    }

        /* Read reversed file and reverse again */
    pixa = pixaCreate(npages);
    for (i = 0; i < npages; i++) {
        pix = pixReadTiff(weasel_rev, i);
        pixaAddPix(pixa, pix, L_INSERT);
    }
    for (i = npages - 1; i >= 0; i--) {
        pix = pixaGetPix(pixa, i, L_CLONE);
        if (i == npages - 1)
            pixWriteTiff(weasel_rev_rev, pix, IFF_TIFF_ZIP, "w+");
        else
            pixWriteTiff(weasel_rev_rev, pix, IFF_TIFF_ZIP, "a");
        pixDestroy(&pix);
    }
    pixaDestroy(&pixa);
#endif


#if 0    /* -----   test adding custom public tags to a tiff header ----- */
    pix = pixRead("feyn.tif");
    naflags = numaCreate(10);
    savals = sarrayCreate(10);
    satypes = sarrayCreate(10);
    nasizes = numaCreate(10);

/*    numaAddNumber(naflags, TIFFTAG_XMLPACKET);  */ /* XMP:  700 */
    numaAddNumber(naflags, 700);
    str = "<xmp>This is a Fake XMP packet</xmp>\n<text>Guess what ...?</text>";
    length = strlen(str);
    sarrayAddString(savals, (char *)str, 1);
    sarrayAddString(satypes, (char *)"char*", 1);
    numaAddNumber(nasizes, length);  /* get it all */

    numaAddNumber(naflags, 269);  /* DOCUMENTNAME */
    sarrayAddString(savals, (char *)"One silly title", 1);
    sarrayAddString(satypes, (char *)"const char*", 1);
    numaAddNumber(naflags, 270);  /* IMAGEDESCRIPTION */
    sarrayAddString(savals, (char *)"One page of text", 1);
    sarrayAddString(satypes, (char *)"const char*", 1);
        /* the max sample is used by rendering programs
         * to scale the dynamic range */
    numaAddNumber(naflags, 281);  /* MAXSAMPLEVALUE */
    sarrayAddString(savals, (char *)"4", 1);
    sarrayAddString(satypes, (char *)"l_uint16", 1);
        /* note that date is required to be a 20 byte string */
    numaAddNumber(naflags, 306);  /* DATETIME */
    sarrayAddString(savals, (char *)"2004:10:11 09:35:15", 1);
    sarrayAddString(satypes, (char *)"const char*", 1);
        /* note that page number requires 2 l_uint16 input */
    numaAddNumber(naflags, 297);  /* PAGENUMBER */
    sarrayAddString(savals, (char *)"1-412", 1);
    sarrayAddString(satypes, (char *)"l_uint16-l_uint16", 1);
    pixWriteTiffCustom("/tmp/tiff/tags.tif", pix, IFF_TIFF_G4, "w", naflags,
                       savals, satypes, nasizes);
    fprintTiffInfo(stderr, (char *)"/tmp/tiff/tags.tif");
    fprintf(stderr, "num flags = %d\n", numaGetCount(naflags));
    fprintf(stderr, "num sizes = %d\n", numaGetCount(nasizes));
    fprintf(stderr, "num vals = %d\n", sarrayGetCount(savals));
    fprintf(stderr, "num types = %d\n", sarrayGetCount(satypes));
    numaDestroy(&naflags);
    numaDestroy(&nasizes);
    sarrayDestroy(&savals);
    sarrayDestroy(&satypes);
    pixDestroy(&pix);
#endif

    return 0;
}

