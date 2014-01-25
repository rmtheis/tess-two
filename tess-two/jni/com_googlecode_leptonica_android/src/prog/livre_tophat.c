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
 * livre_tophat.c
 *
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
PIX         *pixs, *pixsg, *pixg, *pixd;
PIXA        *pixa;
static char  mainName[] = "livre_tophat";

    if (argc != 1)
	return ERROR_INT(" Syntax: livre_tophat", mainName, 1);

        /* Read the image in at 150 ppi. */
    if ((pixs = pixRead("brothers.150.jpg")) == NULL)
	return ERROR_INT("pix not made", mainName, 1);
    pixDisplayWrite(NULL, -1);
    pixDisplayWriteFormat(pixs, 2, IFF_JFIF_JPEG);

    pixsg = pixConvertRGBToLuminance(pixs);

        /* Black tophat (closing - original-image) and invert */
    pixg = pixTophat(pixsg, 15, 15, L_TOPHAT_BLACK);
    pixInvert(pixg, pixg);
    pixDisplayWriteFormat(pixg, 2, IFF_JFIF_JPEG);

        /* Set black point at 200, white point at 245. */
    pixd = pixGammaTRC(NULL, pixg, 1.0, 200, 245);
    pixDisplayWriteFormat(pixd, 2, IFF_JFIF_JPEG);
    pixDestroy(&pixg);
    pixDestroy(&pixd);

        /* Generate the output image */
    pixa = pixaReadFiles("/tmp/display", "file");
    pixd = pixaDisplayTiledAndScaled(pixa, 8, 350, 3, 0, 25, 2);
    pixWrite("/tmp/tophat.jpg", pixd, IFF_JFIF_JPEG);
    pixDisplay(pixd, 0, 0);
    pixDestroy(&pixd);

    pixDestroy(&pixs);
    pixDestroy(&pixsg);
    return 0;
}

