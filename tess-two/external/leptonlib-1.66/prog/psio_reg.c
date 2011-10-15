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
 * psio_reg.c
 *
 *   Tests writing of images in PS, with arbitrary scaling and
 *   translation, in the following formats:
 *
 *      - uncompressed
 *      - DCT compressed (jpeg for 8 bpp grayscale and RGB)
 *      - CCITT-G4 compressed (g4 fax compression for 1 bpp)
 *      - Flate compressed (gzip compression)
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
l_int32       w, h, success, display;
l_float32     factor, scale;
BOX          *box;
FILE         *fp, *fp1;
PIX          *pixs, *pixt;

    if (regTestSetup(argc, argv, &fp, &display, &success, NULL))
        return 1;

    factor = 0.95;

        /* Uncompressed PS with scaling but centered on the page */
    pixs = pixRead("feyn-fract.tif");
    pixGetDimensions(pixs, &w, &h, NULL);
    scale = L_MIN(factor * 2550 / w, factor * 3300 / h);
    fp1 = fopen("/tmp/psio0.ps", "wb+");
    pixWriteStreamPS(fp1, pixs, NULL, 300, scale);
    fclose(fp1);
    regTestCheckFile(fp, argv, "/tmp/psio0.ps", 0, &success);
    pixDestroy(&pixs);

        /* Uncompressed PS with scaling, with LL corner at (1500, 1500) mils */
    pixs = pixRead("weasel4.11c.png");
    pixGetDimensions(pixs, &w, &h, NULL);
    scale = L_MIN(factor * 2550 / w, factor * 3300 / h);
    box = boxCreate(1500, 1500, (l_int32)(1000 * scale * w / 300),
                    (l_int32)(1000 * scale * h / 300));
    fp1 = fopen("/tmp/psio1.ps", "wb+");
    pixWriteStreamPS(fp1, pixs, box, 300, 1.0);
    fclose(fp1);
    regTestCheckFile(fp, argv, "/tmp/psio1.ps", 1, &success);
    boxDestroy(&box);
    pixDestroy(&pixs);

        /* DCT compressed PS with LL corner at (300, 1000) pixels */
    pixs = pixRead("marge.jpg");
    pixt = pixConvertTo32(pixs);
    pixWrite("/tmp/psio2.jpg", pixt, IFF_JFIF_JPEG);
    convertJpegToPS("/tmp/psio2.jpg", "/tmp/psio3.ps",
                    "w", 300, 1000, 0, 4.0, 1, 1);
    regTestCheckFile(fp, argv, "/tmp/psio2.jpg", 2, &success);
    regTestCheckFile(fp, argv, "/tmp/psio3.ps", 3, &success);
    pixDestroy(&pixt);
    pixDestroy(&pixs);

        /* For each page, apply tiff g4 image first; then jpeg or png over it */
    convertTiffG4ToPS("feyn.tif", "/tmp/psio4.ps",
                      "w", 0, 0, 0, 1.0, 1, 1, 0);
    convertJpegToPS("marge.jpg", "/tmp/psio4.ps",
                    "a", 500, 100, 300, 2.0, 1,  0);
    convertFlateToPS("weasel4.11c.png", "/tmp/psio4.ps",
                     "a", 300, 400, 300, 6.0, 1,  0);
    convertJpegToPS("marge.jpg", "/tmp/psio4.ps",
                    "a", 100, 800, 300, 1.5, 1, 1);

    convertTiffG4ToPS("feyn.tif", "/tmp/psio4.ps",
                      "a", 0, 0, 0, 1.0, 2, 1, 0);
    convertJpegToPS("marge.jpg", "/tmp/psio4.ps",
                    "a", 1000, 700, 300, 2.0, 2, 0);
    convertJpegToPS("marge.jpg", "/tmp/psio4.ps",
                    "a", 100, 200, 300, 2.0, 2, 1);

    convertTiffG4ToPS("feyn.tif", "/tmp/psio4.ps",
                      "a", 0, 0, 0, 1.0, 3, 1, 0);
    convertJpegToPS("marge.jpg", "/tmp/psio4.ps",
                    "a", 200, 200, 300, 2.0, 3, 0);
    convertJpegToPS("marge.jpg", "/tmp/psio4.ps",
                    "a", 200, 900, 300, 2.0, 3, 1);
    regTestCheckFile(fp, argv, "/tmp/psio4.ps", 4, &success);

        /* Now apply jpeg first; then paint through a g4 mask.
         * For gv, the first image with a b.b. determines the
         * window size for the canvas, so we put down the largest
         * image first.  If we had rendered a small image first,
         * gv and evince will not show the entire page.  However, after
         * conversion to pdf, everything works fine, regardless of the
         * order in which images are placed into the PS.  That is
         * because the pdf interpreter is robust to bad hints, ignoring
         * the page hints and computing the bounding box from the
         * set of images rendered on the page. */
    pixs = pixRead("wyom.jpg");
    pixt = pixScaleToSize(pixs, 2528, 3300);
    pixWrite("/tmp/psio5.jpg", pixt, IFF_JFIF_JPEG);
    pixDestroy(&pixs);
    pixDestroy(&pixt);
    convertJpegToPS("/tmp/psio5.jpg", "/tmp/psio5.ps",
                      "w", 0, 0, 0, 1.0, 1, 0);
    convertFlateToPS("weasel8.240c.png", "/tmp/psio5.ps",
                     "a", 100, 100, 300, 5.0, 1, 0);
    convertFlateToPS("weasel8.149g.png", "/tmp/psio5.ps",
                     "a", 200, 300, 300, 5.0, 1, 0);
    convertFlateToPS("weasel4.11c.png", "/tmp/psio5.ps",
                     "a", 300, 500, 300, 5.0, 1, 0);
    convertTiffG4ToPS("feyn.tif", "/tmp/psio5.ps",
                      "a", 0, 0, 0, 1.0, 1, 1, 1);

    convertJpegToPS("marge.jpg", "/tmp/psio5.ps",
                    "a", 500, 100, 300, 2.0, 2,  0);
    convertFlateToPS("weasel4.11c.png", "/tmp/psio5.ps",
                     "a", 300, 400, 300, 6.0, 2,  0);
    convertJpegToPS("marge.jpg", "/tmp/psio5.ps",
                    "a", 100, 800, 300, 1.5, 2, 0);
    convertTiffG4ToPS("feyn.tif", "/tmp/psio5.ps",
                      "a", 0, 0, 0, 1.0, 2, 1, 1);

    convertJpegToPS("marge.jpg", "/tmp/psio5.ps",
                    "a", 500, 100, 300, 2.0, 3,  0);
    convertJpegToPS("marge.jpg", "/tmp/psio5.ps",
                    "a", 100, 800, 300, 2.0, 3, 0);
    convertTiffG4ToPS("feyn.tif", "/tmp/psio5.ps",
                      "a", 0, 0, 0, 1.0, 3, 1, 1);

    convertJpegToPS("marge.jpg", "/tmp/psio5.ps",
                    "a", 1000, 700, 300, 2.0, 4, 0);
    convertJpegToPS("marge.jpg", "/tmp/psio5.ps",
                    "a", 100, 200, 300, 2.0, 4, 0);
    convertTiffG4ToPS("feyn.tif", "/tmp/psio5.ps",
                      "a", 0, 0, 0, 1.0, 4, 1, 1);

    convertJpegToPS("marge.jpg", "/tmp/psio5.ps",
                    "a", 200, 200, 300, 2.0, 5, 0);
    convertJpegToPS("marge.jpg", "/tmp/psio5.ps",
                    "a", 200, 900, 300, 2.0, 5, 0);
    convertTiffG4ToPS("feyn.tif", "/tmp/psio5.ps",
                      "a", 0, 0, 0, 1.0, 5, 1, 1);
    regTestCheckFile(fp, argv, "/tmp/psio5.ps", 5, &success);
 
        /* Generation using segmentation masks */
    convertSegmentedPagesToPS(".", "lion-page", ".", "lion-mask",
                              10, 0, 100, 2.0, 0.8, 190, "/tmp/psio6.ps");
    regTestCheckFile(fp, argv, "/tmp/psio6.ps", 6, &success);


    regTestCleanup(argc, argv, fp, success, NULL);
    return 0;
}

