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
 * livre_adapt.c
 *
 * This shows two ways to normalize a document image for uneven
 * illumination.  It is somewhat more complicated than using the
 * morphological tophat.
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
PIX         *pixs, *pixc, *pixr, *pixg, *pixb, *pixsg, *pixsm, *pixd;
PIXA        *pixa;
static char  mainName[] = "livre_adapt";

    if (argc != 1)
        return ERROR_INT(" Syntax:  livre_adapt", mainName, 1);

        /* Read the image in at 150 ppi. */
    pixDisplayWrite(NULL, -1);
    if ((pixs = pixRead("brothers.150.jpg")) == NULL)
        return ERROR_INT("pix not made", mainName, 1);
    pixDisplayWriteFormat(pixs, 2, IFF_JFIF_JPEG);

        /* Normalize for uneven illumination on RGB image */
    pixBackgroundNormRGBArraysMorph(pixs, NULL, 4, 5, 200,
                                    &pixr, &pixg, &pixb);
    pixd = pixApplyInvBackgroundRGBMap(pixs, pixr, pixg, pixb, 4, 4);
    pixDisplayWriteFormat(pixd, 2, IFF_JFIF_JPEG);
    pixDestroy(&pixr);
    pixDestroy(&pixg);
    pixDestroy(&pixb);
    pixDestroy(&pixd);

        /* Convert the RGB image to grayscale. */
    pixsg = pixConvertRGBToLuminance(pixs);
    pixDisplayWriteFormat(pixsg, 2, IFF_JFIF_JPEG);

        /* Remove the text in the fg. */
    pixc = pixCloseGray(pixsg, 25, 25);
    pixDisplayWriteFormat(pixc, 2, IFF_JFIF_JPEG);

        /* Smooth the bg with a convolution. */
    pixsm = pixBlockconv(pixc, 15, 15);
    pixDisplayWriteFormat(pixsm, 2, IFF_JFIF_JPEG);
    pixDestroy(&pixc);

        /* Normalize for uneven illumination on gray image. */
    pixBackgroundNormGrayArrayMorph(pixsg, NULL, 4, 5, 200, &pixg);
    pixc = pixApplyInvBackgroundGrayMap(pixsg, pixg, 4, 4);
    pixDisplayWriteFormat(pixc, 2, IFF_JFIF_JPEG);
    pixDestroy(&pixg);

        /* Increase the dynamic range. */
    pixd = pixGammaTRC(NULL, pixc, 1.0, 30, 180);
    pixDisplayWriteFormat(pixd, 2, IFF_JFIF_JPEG);
    pixDestroy(&pixc);

        /* Threshold to 1 bpp. */
    pixb = pixThresholdToBinary(pixd, 120);
    pixDisplayWriteFormat(pixb, 2, IFF_PNG);
    pixDestroy(&pixd);
    pixDestroy(&pixb);

            /* Generate the output image */
    pixa = pixaReadFiles("/tmp/display", "file");
    pixd = pixaDisplayTiledAndScaled(pixa, 8, 350, 4, 0, 25, 2);
    pixWrite("/tmp/adapt.jpg", pixd, IFF_JFIF_JPEG);
    pixDisplayWithTitle(pixd, 100, 100, NULL, 1);
    pixDestroy(&pixd);

    pixDestroy(&pixs);
    pixDestroy(&pixsg);
    return 0;
}

