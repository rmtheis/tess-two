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
 * falsecolortest.c
 */

#include "allheaders.h"

#define   DEPTH     16    /* set to either 8 or 16 */
#define   WIDTH     768
#define   HEIGHT    100


int main(int    argc,
         char **argv)
{
PIX            *pixs, *pixd, *pixt;
l_int32         i, j, maxval, val;
l_float32       gamma;
static char     mainName[] = "falsecolortest";

    if (argc != 2)
        return ERROR_INT(" Syntax:  falsecolortest gamma", mainName, 1);

    gamma = atof(argv[1]);
    maxval = 0xff;
    if (DEPTH == 16)
        maxval = 0xffff;

    pixs = pixCreate(WIDTH, HEIGHT, DEPTH);
    for (i = 0; i < HEIGHT; i++) {
        for (j = 0; j < WIDTH; j++) {
            val = maxval * j / WIDTH;
            pixSetPixel(pixs, j, i, val);
        }
    }
    fprintf(stderr, "before depth = %d\n", pixGetDepth(pixs));
    pixWrite("/tmp/out16.png", pixs, IFF_PNG);
    pixt = pixRead("/tmp/out16.png");
    pixWrite("/tmp/outafter.png", pixt, IFF_PNG);
    fprintf(stderr, "after depth = %d\n", pixGetDepth(pixt));

    pixd = pixConvertGrayToFalseColor(pixt, gamma);
    pixDisplay(pixd, 50, 50);
    pixWrite("/tmp/out.png", pixd, IFF_PNG);
    pixDestroy(&pixs);
    pixDestroy(&pixt);
    pixDestroy(&pixd);
    return 0;
}

