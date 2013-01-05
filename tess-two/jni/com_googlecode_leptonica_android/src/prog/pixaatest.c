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
 *   pixaatest.c
 *
 *     Syntax:  pixaatest
 */

#include "allheaders.h"

static const l_int32  nx = 10;
static const l_int32  ny = 10;
static const l_int32  ncols = 3;

main(int    argc,
     char **argv)
{
l_int32      w, d, tilewidth;
PIX         *pixs;
PIXA        *pixa, *pixad1, *pixad2;
PIXAA       *pixaa1, *pixaa2;
static char  mainName[] = "pixaatest";

    if (argc != 1)
        exit(ERROR_INT(" Syntax: pixaatest", mainName, 1));

        /* Read in file; generate tiled pixaa; write pixaa to file */
    pixs = pixRead("test24.jpg");
    pixGetDimensions(pixs, &w, NULL, &d);
    tilewidth = w / nx;
    pixa = pixaSplitPix(pixs, nx, ny, 0, 0);
    pixaa1 = pixaaCreateFromPixa(pixa, nx, L_CHOOSE_CONSECUTIVE, L_CLONE);
    pixaa2 = pixaaCreateFromPixa(pixa, nx, L_CHOOSE_SKIP_BY, L_CLONE);
    pixaaWrite("/tmp/junkpixaa1", pixaa1);
    pixaaWrite("/tmp/junkpixaa2", pixaa2);
    pixaDestroy(&pixa);
    pixaaDestroy(&pixaa1);
    pixaaDestroy(&pixaa2);

        /* Read pixaa from file; tile/scale; write result; convert to PS */
    pixaa1 = pixaaRead("/tmp/junkpixaa1");
    pixaa2 = pixaaRead("/tmp/junkpixaa2");
    pixad1 = pixaaDisplayTiledAndScaled(pixaa1, d, tilewidth, ncols, 0, 10, 0);
    pixad2 = pixaaDisplayTiledAndScaled(pixaa2, d, tilewidth, ncols, 0, 10, 0);
    pixaWriteFiles("/tmp/junksplit1.", pixad1, IFF_JFIF_JPEG);
    pixaWriteFiles("/tmp/junksplit2.", pixad2, IFF_JFIF_JPEG);
    convertFilesToPS("/tmp", "junksplit1", 40, "/tmp/junkout1.ps");
    convertFilesToPS("/tmp", "junksplit2", 40, "/tmp/junkout2.ps");
    pixDestroy(&pixs);
    pixaaDestroy(&pixaa1);
    pixaaDestroy(&pixaa2);
    pixaDestroy(&pixad1);
    pixaDestroy(&pixad2);
    return 0;
}

