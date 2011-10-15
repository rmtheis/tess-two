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
 * psioseg_reg.c
 *
 *   This tests the PostScript output for images with mixed
 *   text and images, coming from source of different depths,
 *   with and without colormaps.
 *
 *   Both convertFilesFittedToPS() and convertSegmentedPagesToPS()
 *   generate a compressed PostScript file from a subset of images in
 *   a directory.  However, the latter function can also accept 1 bpp
 *   masks that delineate image (as opposed to text) regions in
 *   the corresponding page image file.  Then, for page images that
 *   are not 1 bpp, it generates mixed raster PostScript with
 *   g4 encoding for the text and jpeg ("DCT") encoding for the
 *   remaining image parts.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

main(int    argc,
     char **argv)
{
l_int32      i, j, w, h, wc, hc, display, success;
l_float32    scalefactor;
FILE        *fp;
PIX         *pixs, *pixc, *pixht, *pixtxt, *pixmfull;
PIX         *pix4c, *pix8c, *pix8g, *pix32, *pixcs, *pixcs2;

    if (regTestSetup(argc, argv, &fp, &display, &success, NULL))
              return 1;

        /* Source for generating images */
    pixs = pixRead("pageseg2.tif");   /* 1 bpp */
    pixc = pixRead("tetons.jpg");     /* 32 bpp */

        /* Get a halftone segmentation mask for pixs */
    pixGetRegionsBinary(pixs, &pixht, NULL, NULL, 0);
    pixtxt = pixSubtract(NULL, pixs, pixht);

        /* Construct a 32 bpp image in full page size, along with
         * a mask that can be used to render it. */
    pixGetDimensions(pixs, &w, &h, NULL);
    pixGetDimensions(pixc, &wc, NULL, NULL);
    scalefactor = (l_float32)w / (l_float32)wc;
    pixcs = pixScale(pixc, scalefactor, scalefactor);
    pixGetDimensions(pixcs, &wc, &hc, NULL);
    pixcs2 = pixCreate(w, h, 32);
    pixRasterop(pixcs2, 0, 0, w, hc, PIX_SRC, pixcs, 0, 0);
    pixRasterop(pixcs2, 0, hc, w, hc, PIX_SRC, pixcs, 0, 0);
    pixWrite("/tmp/junkpsioseg.0.jpg", pixcs2, IFF_JFIF_JPEG);
    regTestCheckFile(fp, argv, "/tmp/junkpsioseg.0.jpg", 0, &success);
    pixmfull = pixCreate(w, h, 1);
    pixSetAll(pixmfull);  /* use as mask to render the color image */
    
         /* Now make a 32 bpp input image, taking text parts from the
          * page image and image parts from pixcs2. */
    pix32 = pixConvertTo32(pixtxt);
    pixCombineMasked(pix32, pixcs2, pixht);
    pixWrite("/tmp/junkpsioseg.1.jpg", pix32, IFF_JFIF_JPEG);
    regTestCheckFile(fp, argv, "/tmp/junkpsioseg.1.jpg", 1, &success);
    
         /* Make an 8 bpp gray version */
    pix8g = pixConvertRGBToLuminance(pix32);
    pixWrite("/tmp/junkpsioseg.2.jpg", pix8g, IFF_JFIF_JPEG);
    regTestCheckFile(fp, argv, "/tmp/junkpsioseg.2.jpg", 2, &success);
    
         /* Make an 8 bpp colormapped version */
    pix8c = pixOctreeColorQuant(pix32, 240, 0);
    pixWrite("/tmp/junkpsioseg.3.png", pix8c, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/junkpsioseg.3.png", 3, &success);
    
         /* Make a 4 bpp colormapped version */
    pix4c = pixOctreeQuantNumColors(pix32, 16, 4);
    pixWrite("/tmp/junkpsioseg.4.png", pix4c, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/junkpsioseg.4.png", 4, &success);

         /* Write out the files to be imaged */
#ifndef _WIN32
    system("mkdir /tmp/junkimagedir");
    system("mkdir /tmp/junkmaskdir");
#else
    system("mkdir \\tmp\\junkimagedir");
    system("mkdir \\tmp\\junkmaskdir");
#endif  /* _WIN32 */
    pixWrite("/tmp/junkimagedir/001.tif", pixs, IFF_TIFF_G4);
    pixWrite("/tmp/junkimagedir/002.tif", pixht, IFF_TIFF_G4);
    pixWrite("/tmp/junkimagedir/003.tif", pixtxt, IFF_TIFF_G4);
    pixWrite("/tmp/junkimagedir/004.jpg", pixcs2, IFF_JFIF_JPEG);
    pixWrite("/tmp/junkmaskdir/004.tif", pixmfull, IFF_TIFF_G4);
    pixWrite("/tmp/junkimagedir/005.jpg", pix32, IFF_JFIF_JPEG);
    pixWrite("/tmp/junkmaskdir/005.tif", pixht, IFF_TIFF_G4);
    pixWrite("/tmp/junkimagedir/006.jpg", pix8g, IFF_JFIF_JPEG);
    pixWrite("/tmp/junkmaskdir/006.tif", pixht, IFF_TIFF_G4);
    pixWrite("/tmp/junkimagedir/007.png", pix8c, IFF_PNG);
    pixWrite("/tmp/junkmaskdir/007.tif", pixht, IFF_TIFF_G4);
    pixWrite("/tmp/junkimagedir/008.png", pix4c, IFF_PNG);
    pixWrite("/tmp/junkmaskdir/008.tif", pixht, IFF_TIFF_G4);
    pixDestroy(&pixs);
    pixDestroy(&pixc);
    pixDestroy(&pixht);
    pixDestroy(&pixtxt);
    pixDestroy(&pixcs);
    pixDestroy(&pixcs2);
    pixDestroy(&pixmfull);
    pixDestroy(&pix32);
    pixDestroy(&pix8g);
    pixDestroy(&pix8c);
    pixDestroy(&pix4c);
    
        /* Generate the 8 page ps and pdf files */
    convertSegmentedPagesToPS("/tmp/junkimagedir", NULL,
                              "/tmp/junkmaskdir", NULL,
                              0, 0, 10, 2.0, 0.15, 190, "/tmp/junkseg.ps");
    regTestCheckFile(fp, argv, "/tmp/junkseg.ps", 5, &success);
    fprintf(stderr, "ps file made: /tmp/junkseg.ps\n");
    system("ps2pdf /tmp/junkseg.ps /tmp/junkseg.pdf");
    fprintf(stderr, "pdf file made: /tmp/junkseg.pdf\n");

    regTestCleanup(argc, argv, fp, success, NULL);
    return 0;
}


