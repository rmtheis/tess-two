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
 * mtifftest.c
 *
 *   Tests tiff I/O for:
 *
 *       -- multipage tiff read/write
 *       -- writing special tiff tags to file
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const char *tempmtiff = "/tmp/junkmtiff";
static const char *tempnewmtiff = "/tmp/junknewmtiff";


main(int    argc,
     char **argv)
{
char        *filein, *fileout, *str, *fname, *filename;
char         buffer[512];
l_int32      i, count, npages, length;
FILE        *fp;
NUMA        *naflags, *nasizes;
PIX         *pix, *pixd;
PIXA        *pixa;
PIXCMAP     *cmap;
SARRAY      *savals, *satypes, *sa;
static char  mainName[] = "mtifftest";

    if (argc != 3)
	exit(ERROR_INT(" Syntax:  mtifftest filein fileout", mainName, 1));

    filein = argv[1];
    fileout = argv[2];

#if 1   /* ------------------  Test multipage I/O  -------------------*/
        /* This puts every image file in the directory with a string
         * match to "weasel" into a multipage tiff file.
         * Images with 1 bpp are coded as g4; the others as zip.
         * It then reads back into a pix and displays.  */
    writeMultipageTiff(".", "weasel", "/tmp/junkout.tif");
    pixa = pixaReadMultipageTiff("/tmp/junkout.tif");
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

#if 0   /* ------------ Test single-to-multipage I/O  -------------------*/
        /* Use 'filein' to specify a directory of tiff files.
	 * Read them in and generate a multipage tiff file.
	 * Then convert that to a G4 compressed and ascii85 encoded
	 * PS file. */
    sa = getFilenamesInDirectory(filein);
    sarrayWriteStream(stderr, sa);
    sarraySort(sa, sa, L_SORT_INCREASING);
    sarrayWriteStream(stderr, sa);
    npages = sarrayGetCount(sa);
    for (i = 0; i < npages; i++) {
        fname = sarrayGetString(sa, i, 0);
        filename = genPathname(filein, fname);
        pix = pixRead(filename);
	if (!pix) continue;
	if (i == 0)
	    pixWriteTiff(tempmtiff, pix, IFF_TIFF_G4, "w+");
        else
	    pixWriteTiff(tempmtiff, pix, IFF_TIFF_G4, "a");
        pixDestroy(&pix);
        FREE(filename);
    }

        /* write it out as a PS file */
    convertTiffMultipageToPS(tempmtiff, fileout, NULL, 0.95);
    sarrayDestroy(&sa);
#endif

#if 0   /* ------------------  Test multipage I/O  -------------------*/
        /* read count of tiff multipage */
    fp = fopen(filein, "r");
    if (fileFormatIsTiff(fp)) {
	tiffGetCount(fp, &npages);
	fprintf(stderr, " Tiff: %d page\n", npages);
    }
    else 
	exit(ERROR_INT(" file not tiff", mainName, 1));
    fclose(fp);

        /* split into separate page files */
    for (i = 0; i < npages + 1; i++) {   /* read one beyond to catch error */
	pix = pixReadTiff(filein, i);
	if (!pix) continue;
	sprintf(buffer, "/tmp/junkout.%d.tif", i);
	pixWrite(buffer, pix, IFF_TIFF_G4);
        pixDestroy(&pix);
    }

        /* read separate page files and write reversed file */
    for (i = npages - 1; i >= 0; i--) {
	sprintf(buffer, "/tmp/junkout.%d.tif", i);
        pix = pixRead(buffer);
	if (!pix) continue;
	if (i == npages - 1)
	    pixWriteTiff(tempmtiff, pix, IFF_TIFF_G4, "w+");
        else
	    pixWriteTiff(tempmtiff, pix, IFF_TIFF_G4, "a");
        pixDestroy(&pix);
    }

        /* read reversed file and reverse again */
    pixa = pixaCreate(npages);
    for (i = 0; i < 5; i++) {
	pix = pixReadTiff(tempmtiff, i);
	pixaAddPix(pixa, pix, L_INSERT);
    }
    for (i = npages - 1; i >= 0; i--) {
        pix = pixaGetPix(pixa, i, L_CLONE);
	if (i == npages - 1)
	    pixWriteTiff(tempnewmtiff, pix, IFF_TIFF_G4, "w+");
        else
	    pixWriteTiff(tempnewmtiff, pix, IFF_TIFF_G4, "a");
        pixDestroy(&pix);
    }
    pixaDestroy(&pixa);
#endif


#if 0    /* -----   test adding custom public tags to a tiff header ----- */
    pix = pixRead(filein);
    naflags = numaCreate(10);
    savals = sarrayCreate(10);
    satypes = sarrayCreate(10);
    nasizes = numaCreate(10);
    
/*    numaAddNumber(naflags, TIFFTAG_XMLPACKET);  */ /* XMP:  700 */
    numaAddNumber(naflags, 700);
    str = "<xmp>This is a Fake XMP packet</xmp>\n<text>Guess what ...?</text>";
    length = strlen(str);
    sarrayAddString(savals, str, 1);
    sarrayAddString(satypes, "char*", 1);
    numaAddNumber(nasizes, length);  /* get it all */

    numaAddNumber(naflags, 269);  /* DOCUMENTNAME */
    sarrayAddString(savals, "One silly title", 1);
    sarrayAddString(satypes, "char*", 1);
    numaAddNumber(naflags, 270);  /* IMAGEDESCRIPTION */
    sarrayAddString(savals, "One page of text", 1);
    sarrayAddString(satypes, "char*", 1);
        /* the max sample is used by rendering programs 
         * to scale the dynamic range */
    numaAddNumber(naflags, 281);  /* MAXSAMPLEVALUE */
    sarrayAddString(savals, "4", 1);
    sarrayAddString(satypes, "l_uint16", 1);
        /* note that date is required to be a 20 byte string */
    numaAddNumber(naflags, 306);  /* DATETIME */
    sarrayAddString(savals, "2004:10:11 09:35:15", 1);
    sarrayAddString(satypes, "char*", 1);
        /* note that page number requires 2 l_uint16 input */
    numaAddNumber(naflags, 297);  /* PAGENUMBER */
    sarrayAddString(savals, "1-412", 1);
    sarrayAddString(satypes, "l_uint16-l_uint16", 1);
    pixWriteTiffCustom(fileout, pix, IFF_TIFF_G4, "w", naflags,
                       savals, satypes, nasizes);
    fprintTiffInfo(stderr, fileout);
    numaDestroy(&naflags);
    numaDestroy(&nasizes);
    sarrayDestroy(&savals);
    sarrayDestroy(&satypes);
    pixDestroy(&pix);
#endif

    exit(0);
}

