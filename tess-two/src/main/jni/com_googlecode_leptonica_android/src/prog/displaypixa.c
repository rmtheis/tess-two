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
 * displaypixa.c
 *
 *        displaypixa filein fileout [fontdir]
 *        displaypixa filein scalefact border lossless disp fileout [fontdir]
 *
 *   where disp = 1 to display on the screen; 0 to skip
 *         lossless = 1 for tiff or png
 *
 *   This reads a pixa from file and generates a composite of the
 *   images tiled in rows.  It also optionally displays on the screen.
 *   No scaling is done if @scalefact == 0.0 or @scalefact == 1.0.
 *   If @fontdir is specified, the text field for all pix with text
 *   is written out below the image.
 */

#include <string.h>
#include "allheaders.h"

int main(int    argc,
         char **argv)
{
char         buf[32];
char        *filein, *fileout, *fontdir, *textstr;
l_int32      n, i, maxdepth, ntext, border, lossless, display;
l_float32    scalefact;
L_BMF       *bmf;
PIX         *pix1, *pix2, *pix3, *pix4, *pixd;
PIXA        *pixa, *pixat;
static char  mainName[] = "displaypixa";

    if (argc != 3 && argc != 4 && argc != 7 && argc != 8) {
        fprintf(stderr, "Syntax error in displaypixa:\n"
           "   displaypixa filein fileout [fontdir]\n"
           "   displaypixa filein scalefact border"
                 " lossless disp fileout [fontdir]\n");
         return 1;
    }

    filein = argv[1];
    if ((pixa = pixaRead(filein)) == NULL)
        return ERROR_INT("pixa not made", mainName, 1);
    pixaCountText(pixa, &ntext);

        /* Simple specification; no output text */
    if (argc == 3 || (argc == 4 && ntext == 0)) {  /* no text output */
        fileout = argv[2];
        pixaVerifyDepth(pixa, &maxdepth);
        pixd = pixaDisplayTiledInRows(pixa, maxdepth, 1400, 1.0, 0, 10, 0);
        pixDisplay(pixd, 100, 100);
        if (pixGetDepth(pixd) == 1)
            pixWrite(fileout, pixd, IFF_PNG);
        else
            pixWrite(fileout, pixd, IFF_JFIF_JPEG);
        pixDestroy(&pixd);
        pixaDestroy(&pixa);
        return 0;
    }

        /* Simple specification with output text */
    if (argc == 4) {  /* ntext > 0 */
        fileout = argv[2];
        fontdir = argv[3];
        n = pixaGetCount(pixa);
        if ((bmf = bmfCreate(fontdir, 6)) == NULL) {
            L_ERROR("couldn't read fontdir\n", mainName);
            pixat = pixaCopy(pixa, L_CLONE);
        } else {
            pixat = pixaCreate(n);
            for (i = 0; i < n; i++) {
                pix1 = pixaGetPix(pixa, i, L_CLONE);
                pix2 = pixConvertTo32(pix1);
                pix3 = pixAddBorderGeneral(pix2, 10, 10, 5, 5, 0xffffff00);
                textstr = pixGetText(pix1);
                if (textstr && strlen(textstr) > 0) {
                    snprintf(buf, sizeof(buf), "%s", textstr);
                    pix4 = pixAddSingleTextblock(pix3, bmf, buf, 0xff000000,
                                                 L_ADD_BELOW, NULL);
                } else {
                    pix4 = pixClone(pix3);
                }
                pixaAddPix(pixat, pix4, L_INSERT);
                pixDestroy(&pix1);
                pixDestroy(&pix2);
                pixDestroy(&pix3);
            }
            bmfDestroy(&bmf);
        }
        pixaVerifyDepth(pixat, &maxdepth);
        pixd = pixaDisplayTiledInRows(pixat, maxdepth, 1400, 1.0, 0, 10, 0);
        pixDisplay(pixd, 100, 100);
        if (pixGetDepth(pixd) == 1)
            pixWrite(fileout, pixd, IFF_PNG);
        else
            pixWrite(fileout, pixd, IFF_JFIF_JPEG);
        pixDestroy(&pixd);
        pixaDestroy(&pixa);
        pixaDestroy(&pixat);
        return 0;
    }

        /* Full specification */
    scalefact = atof(argv[2]);
    border = atoi(argv[3]);
    lossless = atoi(argv[4]);
    display = atoi(argv[5]);
    fileout = argv[6];
    fontdir = (argc == 8) ? argv[7] : NULL;
    if (fontdir && ntext == 0)
        L_WARNING("No text found in any of the pix\n", mainName);
    if (fontdir && ntext > 0) {
        if ((bmf = bmfCreate(fontdir, 6)) == NULL) {
            L_ERROR("couldn't read fontdir\n", mainName);
            pixat = pixaCopy(pixa, L_CLONE);
        } else {
            n = pixaGetCount(pixa);
            pixat = pixaCreate(n);
            for (i = 0; i < n; i++) {
                pix1 = pixaGetPix(pixa, i, L_CLONE);
                pix2 = pixConvertTo32(pix1);
                pix3 = pixAddBorderGeneral(pix2, 10, 10, 5, 5, 0xffffff00);
                textstr = pixGetText(pix1);
                if (textstr && strlen(textstr) > 0) {
                    snprintf(buf, sizeof(buf), "%s", textstr);
                    pix4 = pixAddSingleTextblock(pix3, bmf, buf, 0xff000000,
                                             L_ADD_BELOW, NULL);
                } else {
                    pix4 = pixClone(pix3);
                }
                pixaAddPix(pixat, pix4, L_INSERT);
                pixDestroy(&pix1);
                pixDestroy(&pix2);
                pixDestroy(&pix3);
            }
            bmfDestroy(&bmf);
        }
    } else {
        pixat = pixaCopy(pixa, L_CLONE);
    }

    pixaVerifyDepth(pixat, &maxdepth);
    pixd = pixaDisplayTiledInRows(pixat, maxdepth, 1400, scalefact,
                                  0, 10, border);
    if (display) pixDisplay(pixd, 20, 20);
    if (pixGetDepth(pixd) == 1 || lossless)
        pixWrite(fileout, pixd, IFF_PNG);
    else
        pixWrite(fileout, pixd, IFF_JFIF_JPEG);

    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    pixaDestroy(&pixat);
    return 0;
}
