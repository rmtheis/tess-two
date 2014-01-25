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
 * splitimage2pdf.c
 *
 *   Syntax:  splitimage2pdf filein nx ny fileout
 *
 *        nx = number of horizontal tiles
 *        ny = number of vertical tiles
 *
 *   Generates pdf of image tiles.  Rotates the image before
 *   tiling if the tiles otherwise will have larger width than
 *   height.
 */

#include "allheaders.h"

    /* fill factor on 8.5 x 11 inch output page */
static const l_float32   FILL_FACTOR = 0.95;


int main(int    argc,
         char **argv)
{
char        *filein, *fileout, *fname;
char         buffer[512];
const char  *psfile = "/tmp/junk_split_image.ps";
l_int32      nx, ny, i, w, h, d, ws, hs, n, res, ignore;
l_float32    scale;
PIX         *pixs, *pixt, *pixr;
PIXA        *pixa;
static char  mainName[] = "splitimage2pdf";

    if (argc != 5)
        return ERROR_INT(" Syntax:  splitimage2pdf filein nx ny fileout",
                         mainName, 1);

    filein = argv[1];
    nx = atoi(argv[2]);
    ny = atoi(argv[3]);
    fileout = argv[4];

    lept_rm(NULL, "junk_split_image.ps");

    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);
    d = pixGetDepth(pixs);
    if (d == 1 )
        lept_rm(NULL, "junk_split_image.tif");
    else if (d == 8 || d == 32)
        lept_rm(NULL, "junk_split_image.jpg");
    else
        return ERROR_INT("d not in {1,8,32} bpp", mainName, 1);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    if (ny * ws > nx * hs)
        pixr = pixRotate90(pixs, 1);
    else
        pixr = pixClone(pixs);

    pixa = pixaSplitPix(pixr, nx, ny, 0, 0);
    n = pixaGetCount(pixa);
    res = 300;
    for (i = 0; i < n; i++) {
        pixt = pixaGetPix(pixa, i, L_CLONE);
        pixGetDimensions(pixt, &w, &h, NULL);
        scale = L_MIN(FILL_FACTOR * 2550 / w, FILL_FACTOR * 3300 / h);
        fname = NULL;
        if (d == 1) {
            fname = genPathname("/tmp", "junk_split_image.tif");
            pixWrite(fname, pixt, IFF_TIFF_G4);
            if (i == 0) {
                convertG4ToPS(fname, psfile, "w", 0, 0, 300,
                              scale, 1, FALSE, TRUE);
            } else {
                convertG4ToPS(fname, psfile, "a", 0, 0, 300,
                              scale, 1, FALSE, TRUE);
            }
        } else {
            fname = genPathname("/tmp", "junk_split_image.jpg");
            pixWrite(fname, pixt, IFF_JFIF_JPEG);
            if (i == 0) {
                convertJpegToPS(fname, psfile, "w", 0, 0, 300,
                                scale, 1, TRUE);
            } else {
                convertJpegToPS(fname, psfile, "a", 0, 0, 300,
                                scale, 1, TRUE);
            }
        }
        lept_free(fname);
        pixDestroy(&pixt);
    }

    sprintf(buffer, "ps2pdf %s %s", psfile, fileout);
    ignore = system(buffer);

    pixaDestroy(&pixa);
    pixDestroy(&pixr);
    pixDestroy(&pixs);
    return 0;
}

