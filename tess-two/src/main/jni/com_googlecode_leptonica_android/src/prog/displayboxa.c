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
 * displayboxa.c
 *
 *        displayboxa filein width fileout [fontdir]
 *
 *   This reads a boxa from file and generates a composite view of the
 *   boxes, one per "page", tiled in rows.
 *   The pix that backs each box is chosen to be the minimum size that
 *   supports every box in the boxa.  Each pix (and the box it backs)
 *   is scaled so that the pix width is @width in pixels.
 *   If @fontdir is specified, the number of each box is written below it.
 *
 *   The minimum allowed width of the backing pix is 30, and the default
 *   width is 100.
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
char        *filein, *fileout, *fontdir;
l_int32      w, h, width, sep;
l_float32    scalefact;
L_BMF       *bmf;
BOXA        *boxa1, *boxa2;
PIX         *pixd;
static char  mainName[] = "displayboxa";

    if (argc != 4 && argc != 5) {
        fprintf(stderr, "Syntax error in displayboxa:\n"
           "   displayboxa filein width fileout [fontdir]\n");
         return 1;
    }

    filein = argv[1];
    fileout = argv[3];
    fontdir = (argc == 4) ? NULL : argv[4];
    width = atoi(argv[2]);
    if (width < 30) {
        L_ERROR("width too small; setting to 100\n", mainName);
        width = 100;
    }

    if ((boxa1 = boxaRead(filein)) == NULL)
        return ERROR_INT("boxa not made", mainName, 1);
    boxaGetExtent(boxa1, &w, &h, NULL);
    scalefact = (l_float32)width / (l_float32)w;
    boxa2 = boxaTransform(boxa1, 0, 0, scalefact, scalefact);
    sep = L_MIN(width / 5, 20);
    pixd = boxaDisplayTiled(boxa2, NULL, 1500, 2, 1.0, 0, sep, 2, fontdir);
    pixWrite(fileout, pixd, IFF_PNG);
    pixDisplay(pixd, 100, 100);

    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
    pixDestroy(&pixd);
    return 0;
}

