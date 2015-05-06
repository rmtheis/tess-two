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
 * removecmap.c
 *
 *      removecmap filein type fileout
 *
 *          type:  1 for conversion to 8 bpp gray
 *                 2 for conversion to 24 bpp full color
 *                 3 for conversion depending on src
 *
 *      Removes the colormap and does the conversion
 *      Works on palette images of 2, 4 and 8 bpp
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
char        *filein, *fileout;
l_int32      type, numcolors;
PIX         *pixs, *pixd;
PIXCMAP     *cmap;
static char  mainName[] = "removecmap";

    if (argc != 4)
        return ERROR_INT("Syntax:  removecmap filein type fileout",
                         mainName, 1);

    filein = argv[1];
    type = atoi(argv[2]);
    fileout = argv[3];

    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);

    fprintf(stderr, " depth = %d\n", pixGetDepth(pixs));
    if ((cmap = pixGetColormap(pixs)) != NULL) {
        numcolors = pixcmapGetCount(cmap);
        pixcmapWriteStream(stderr, cmap);
        fprintf(stderr, " colormap found; num colors = %d\n", numcolors);
    } else {
        fprintf(stderr, " no colormap\n");
    }

    pixd = pixRemoveColormap(pixs, type);
    pixWrite(fileout, pixd, IFF_PNG);
    pixDestroy(&pixs);
    pixDestroy(&pixd);
    return 0;
}
