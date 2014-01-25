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
 *  colormorphtest.c
 */

#include "allheaders.h"

static void pixCompare(PIX *pix, PIX *pix2, const char *msg1, const char *msg2);

    /* MSVC can't handle arrays dimensioned by static const integers */
#define L_BUF_SIZE    256


int main(int    argc,
         char **argv)
{
char        *filein;
char         buf[L_BUF_SIZE];
l_int32      size;
PIX         *pixs, *pixt1, *pixt2;
static char  mainName[] = "colormorphtest";

    if (argc != 3)
        return ERROR_INT(" Syntax:  colormorphtest filein size", mainName, 1);

    filein = argv[1];
    size = atoi(argv[2]);
    if (size % 2 == 0) size++;
    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pixs not read", mainName, 1);

    pixt1 = pixColorMorph(pixs, L_MORPH_DILATE, size, size);
    sprintf(buf, "d%d.%d", size, size);
    pixt2 = pixColorMorphSequence(pixs, buf, 0, 0);
    pixCompare(pixt1, pixt2, "Correct for dilation", "Error on dilation");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

    pixt1 = pixColorMorph(pixs, L_MORPH_ERODE, size, size);
    sprintf(buf, "e%d.%d", size, size);
    pixt2 = pixColorMorphSequence(pixs, buf, 0, 0);
    pixCompare(pixt1, pixt2, "Correct for erosion", "Error on erosion");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

    pixt1 = pixColorMorph(pixs, L_MORPH_OPEN, size, size);
    sprintf(buf, "o%d.%d", size, size);
    pixt2 = pixColorMorphSequence(pixs, buf, 0, 0);
    pixCompare(pixt1, pixt2, "Correct for opening", "Error on opening");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

    pixt1 = pixColorMorph(pixs, L_MORPH_CLOSE, size, size);
    sprintf(buf, "c%d.%d", size, size);
    pixt2 = pixColorMorphSequence(pixs, buf, 0, 0);
    pixCompare(pixt1, pixt2, "Correct for closing", "Error on closing");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

    pixDisplayMultiple("/tmp/display/file*");

    pixDestroy(&pixs);
    return 0;
}

    /* Simple comparison function */
static void pixCompare(PIX         *pix1,
                       PIX         *pix2,
                       const char  *msg1,
                       const char  *msg2)
{
l_int32  same;
    pixEqual(pix1, pix2, &same);
    if (same) {
        fprintf(stderr, "%s\n", msg1);
        pixDisplayWrite(pix1, 1);
    }
    else {
        fprintf(stderr, "%s\n", msg2);
        pixDisplayWrite(pix1, 1);
        pixDisplayWrite(pix2, 1);
    }
    return;
}

