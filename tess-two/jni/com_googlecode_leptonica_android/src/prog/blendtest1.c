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
 * blendtest1.c
 *
 *     fract must be in interval [0.0, 1.0]
 */

#include "allheaders.h"

#define   X     140
#define   Y     40


main(int    argc,
     char **argv)
{
char        *file1, *file2, *fileout;
l_int32      d;
l_float32    fract;
PIX         *pixs1, *pixs2, *pixt1, *pixt2, *pixt3, *pixt4, *pixd;
static char  mainName[] = "blendtest1";

    if (argc != 5)
	exit(ERROR_INT(" Syntax:  blendtest1 file1 file2 fract fileout",
	    mainName, 1));

    file1 = argv[1];
    file2 = argv[2];
    fract = atof(argv[3]);
    fileout = argv[4];

    if ((pixs1 = pixRead(file1)) == NULL)
	exit(ERROR_INT("pixs1 not made", mainName, 1));
    if ((pixs2 = pixRead(file2)) == NULL)
	exit(ERROR_INT("pixs2 not made", mainName, 1));

#if 0
    d = pixGetDepth(pixs2);
    if (d == 1) {
	pixt1 = pixBlend(pixs1, pixs2, X, Y, fract);
	pixt2 = pixBlend(pixt1, pixs2, X, Y + 60, fract);
	pixt3 = pixBlend(pixt2, pixs2, X, Y + 120, fract);
	pixt4 = pixBlend(pixt3, pixs2, X, Y + 180, fract);
	pixt5 = pixBlend(pixt4, pixs2, X, Y + 240, fract);
	pixd = pixBlend(pixt5, pixs2, X, Y + 300, fract);
	pixWrite(fileout, pixd, IFF_DEFAULT);
	pixDestroy(&pixt1);
	pixDestroy(&pixt2);
	pixDestroy(&pixt3);
	pixDestroy(&pixt4);
	pixDestroy(&pixt5);
    }
    else {
	pixt1 = pixBlend(pixs1, pixs2, X, Y, fract);
	pixt2 = pixBlend(pixt1, pixs2, X + 80, Y + 80, fract);
	pixt3 = pixBlend(pixt2, pixs2, X + 160, Y + 160, fract);
	pixt4 = pixBlend(pixt3, pixs2, X + 240, Y + 240, fract);
	pixt5 = pixBlend(pixt4, pixs2, X + 320, Y + 320, fract);
	pixd = pixBlend(pixt5, pixs2, X + 360, Y + 360, fract);
	pixWrite(fileout, pixd, IFF_DEFAULT);
	pixDestroy(&pixt1);
	pixDestroy(&pixt2);
	pixDestroy(&pixt3);
	pixDestroy(&pixt4);
	pixDestroy(&pixt5);
    }
    pixDestroy(&pixd);
#endif

#if 1  /* e.g., weasel8.png with fract = 0.3 */
    pixSnapColor(pixs2, pixs2, 0xff, 0xff, 50);
    pixBlendGray(pixs1, pixs1, pixs2, 200, 100, fract,
                 L_BLEND_GRAY, 1, 0xff);
    pixBlendGray(pixs1, pixs1, pixs2, 200, 200, fract,
                 L_BLEND_GRAY, 1, 0xff);
    pixBlendGray(pixs1, pixs1, pixs2, 200, 260, fract,
                 L_BLEND_GRAY, 1, 0xff);
    pixBlendGray(pixs1, pixs1, pixs2, 200, 340, fract,
                 L_BLEND_GRAY, 1, 0xff);
    pixWrite(fileout, pixs1, IFF_JFIF_JPEG);
    pixDisplay(pixs1, 200, 200);
#endif

#if 0  /* e.g., weasel8.png with fract = 0.5 */
    pixSnapColor(pixs2, pixs2, 0xff, 0xff, 50);
    pixBlendGray(pixs1, pixs1, pixs2, 200, 100, fract,
                 L_BLEND_GRAY_WITH_INVERSE, 1, 0xff);
    pixBlendGray(pixs1, pixs1, pixs2, 200, 200, fract,
                 L_BLEND_GRAY_WITH_INVERSE, 1, 0xff);
    pixBlendGray(pixs1, pixs1, pixs2, 200, 260, fract,
                 L_BLEND_GRAY_WITH_INVERSE, 1, 0xff);
    pixBlendGray(pixs1, pixs1, pixs2, 200, 340, fract,
                 L_BLEND_GRAY_WITH_INVERSE, 1, 0xff);
    pixWrite(fileout, pixs1, IFF_JFIF_JPEG);
    pixDisplay(pixs1, 200, 200);
#endif

#if 0  /* e.g., weasel32.png with fract = 0.2 */
    pixSnapColor(pixs2, pixs2, 0xffffff00, 0xffffff00, 50);
    pixBlendColor(pixs1, pixs1, pixs2, 200, 100, fract, 1, 0xffffff00);
    pixBlendColor(pixs1, pixs1, pixs2, 200, 200, fract, 1, 0xffffff00);
    pixBlendColor(pixs1, pixs1, pixs2, 200, 260, fract, 1, 0xffffff00);
    pixBlendColor(pixs1, pixs1, pixs2, 200, 340, fract, 1, 0xffffff00);
    pixWrite(fileout, pixs1, IFF_JFIF_JPEG);
    pixDisplay(pixs1, 200, 200);
#endif

    pixDestroy(&pixs1);
    pixDestroy(&pixs2);

    exit(0);
}

