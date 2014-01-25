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
 * renderfonts.c
 *
 *     This tests the font rendering functions
 */

#include "allheaders.h"

#define   NFONTS   9
#define   DIRECTORY    "./fonts"

const l_int32 sizes[] = { 4, 6, 8, 10, 12, 14, 16, 18, 20 };

#define  DEBUG            0


int main(int    argc,
         char **argv)
{
char        *filein, *fileout, *textstr;
l_int32      i, d, size, width, wtext, overflow;
l_uint32     val;
L_BMF       *bmf;
PIX         *pixs, *pix;
static char  mainName[] = "renderfonts";

    if (argc != 4)
        return ERROR_INT("Syntax: renderfonts filein size fileout",
                         mainName, 1);

    filein = argv[1];
    size = atoi(argv[2]);
    fileout = argv[3];
    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);
    d = pixGetDepth(pixs);
    if (d == 8)
        val = 128;
    else if (d == 16)
        val = 0x8000;
    else if (d == 32)
        composeRGBPixel(128, 0, 255, &val);
    else
        return ERROR_INT("pixs not 8, 16 or 32 bpp", mainName, 1);

    bmf = bmfCreate(DIRECTORY, size);

#if 0  /* render a character of text */
    pix = pixaGetPix(bmf->pixa, 45, L_CLONE);
    startTimer();
    for (i = 0; i < 10000; i++)
        pixSetMaskedGeneral(pixs, pix, val, 150, 150);
    fprintf(stderr, "time: %7.3f sec\n", stopTimer());
    pixWrite(fileout, pixs, IFF_JFIF_JPEG);
    pixDestroy(&pix);
#endif

#if 0  /* render a line of text; use marge.jpg with size 14 */
    bmfGetStringWidth(bmf, "This is a funny cat!", &width);
    fprintf(stderr, "String width: %d pixels\n", width);

    pixSetTextline(pixs, bmf, "This is a funny cat!", 0x8000ff00, 50, 250,
                   &width, &overflow);
    pixWrite(fileout, pixs, IFF_JFIF_JPEG);
    fprintf(stderr, "Text width = %d\n", width);
    if (overflow)
        fprintf(stderr, "Text overflow beyond image boundary\n");
#endif

#if 1  /* render a block of text; use marge.jpg with size 14 */
    textstr = stringNew("This is a cat! This is a funny cat! This is a funny funny cat! This is a funny funny funny cat!");

    wtext = pixGetWidth(pixs) - 70;
    pixSetTextblock(pixs, bmf, textstr, 0x4040ff00, 50, 50, wtext,
                    1, &overflow);
    pixWrite(fileout, pixs, IFF_JFIF_JPEG);
    if (overflow)
        fprintf(stderr, "Text overflow beyond image boundary\n");
    lept_free(textstr);
#endif

    pixDestroy(&pixs);
    bmfDestroy(&bmf);
    return 0;
}

