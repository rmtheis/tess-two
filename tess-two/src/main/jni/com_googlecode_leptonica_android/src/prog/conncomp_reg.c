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
 * conncomp_reg.c
 *
 *        Regression test for connected components (both 4 and 8
 *        connected), including regeneration of the original
 *        image from the components.  This is also an implicit
 *        test of rasterop.
 */

#include <string.h>
#include "allheaders.h"

#define  NTIMES             10

int main(int    argc,
         char **argv)
{
l_uint8     *array1, *array2;
l_int32      n, np, same, diff;
size_t       nbytes1, nbytes2;
FILE        *fp;
BOXA        *boxa, *boxa2;
PIX         *pixs, *pixd;
PIXA        *pixa;
PIXCMAP     *cmap;
static char  mainName[] = "conncomp_reg";

    if (argc != 1)
        return ERROR_INT(" Syntax: conncomp_reg", mainName, 1);

    pixs = pixRead("feyn.tif");

        /* Test pixConnComp() with output to both boxa and pixa */
        /* First, test with 4-cc */
    boxa = pixConnComp(pixs, &pixa, 4);
    n = boxaGetCount(boxa);
    fprintf(stderr, "Number of 4 c.c. b.b: %d\n", n);
    np = pixaGetCount(pixa);
    fprintf(stderr, "Number of 4 c.c. pix: %d\n", np);
    pixd = pixaDisplay(pixa, pixGetWidth(pixs), pixGetHeight(pixs));
    pixWrite("/tmp/junkout1.png", pixd, IFF_PNG);
    pixEqual(pixs, pixd, &same);
    if (same == 1)
        fprintf(stderr, "Source and reconstructed pix are the same.\n");
    else
        fprintf(stderr, "Error: source and reconstructed pix differ!\n");
    pixaDestroy(&pixa);
    boxaDestroy(&boxa);
    pixDestroy(&pixd);

        /* Test with 8-cc */
    boxa = pixConnComp(pixs, &pixa, 8);
    n = boxaGetCount(boxa);
    fprintf(stderr, "Number of 8 c.c. b.b: %d\n", n);
    np = pixaGetCount(pixa);
    fprintf(stderr, "Number of 8 c.c. pix: %d\n", np);
    pixd = pixaDisplay(pixa, pixGetWidth(pixs), pixGetHeight(pixs));
    pixWrite("/tmp/junkout2.png", pixd, IFF_PNG);
    pixEqual(pixs, pixd, &same);
    if (same == 1)
        fprintf(stderr, "Source and reconstructed pix are the same.\n");
    else
        fprintf(stderr, "Error: source and reconstructed pix differ!\n");
    pixaDestroy(&pixa);
    boxaDestroy(&boxa);
    pixDestroy(&pixd);

        /* Test i/o */
    boxa = pixConnComp(pixs, NULL, 4);
    fp = lept_fopen("/tmp/junk1.ba", "wb+");
    boxaWriteStream(fp, boxa);
    lept_fclose(fp);
    fp = lept_fopen("/tmp/junk1.ba", "rb");
    boxa2 = boxaReadStream(fp);
    lept_fclose(fp);
    fp = lept_fopen("/tmp/junk2.ba", "wb+");
    boxaWriteStream(fp, boxa2);
    lept_fclose(fp);
    array1 = l_binaryRead("/tmp/junk1.ba", &nbytes1);
    array2 = l_binaryRead("/tmp/junk2.ba", &nbytes2);
    diff = strcmp((char *)array1, (char *)array2);
    if (nbytes1 != nbytes2 || diff)
        fprintf(stderr, "I/O error for boxes.\n");
    else
        fprintf(stderr, "I/O valid for boxes.\n");
    lept_free(array1);
    lept_free(array2);
    boxaDestroy(&boxa);
    boxaDestroy(&boxa2);

        /* Just for fun, display each component as a random color
         * in cmapped 8 bpp.  Background is color 0; it is set to white. */
    boxa = pixConnComp(pixs, &pixa, 4);
    pixd = pixaDisplayRandomCmap(pixa, pixGetWidth(pixs), pixGetHeight(pixs));
    cmap = pixGetColormap(pixd);
    pixcmapResetColor(cmap, 0, 255, 255, 255);  /* reset background to white */
    pixDisplay(pixd, 100, 100);
    boxaDestroy(&boxa);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixDestroy(&pixs);
    return 0;
}


