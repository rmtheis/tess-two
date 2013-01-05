/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -  This software is distributed in the hope that it will be
 -  useful, but with NO WARRANTY OF ANY KIND.
 -  No author or distributor accepts responsibility to anyone for the
 -  consequences of using this software, or for whether it serves any
 -  particular purpose or works at all, unless he or she says so in
 -  writing.  Everyone is granted permission to copy, modify and
 -  redistribute this source code, for commercial or non-commercial
 -  purposes, with the following restrictions: (1) the origin of this
 -  source code must not be misrepresented; (2) modified versions must
 -  be plainly marked as such; and (3) this notice may not be removed
 -  or altered from any source or modified source distribution.
 *====================================================================*/

/*
 * livre_adapt.c
 *
 * This shows two ways to normalize a document image for uneven
 * illumination.  It is somewhat more complicated than using the
 * morphological tophat.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
l_int32      d;
PIX         *pixs, *pixc, *pixr, *pixg, *pixb, *pixsg, *pixsm, *pixd;
PIXA        *pixa;
static char  mainName[] = "livre_adapt";

    if (argc != 1)
	exit(ERROR_INT(" Syntax:  livre_adapt", mainName, 1));

        /* Read the image in at 150 ppi. */
    pixDisplayWrite(NULL, -1);
    if ((pixs = pixRead("brothers.150.jpg")) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));
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
    pixa = pixaReadFiles("/tmp", "junk_write_display");
    pixd = pixaDisplayTiledAndScaled(pixa, 8, 350, 4, 0, 25, 2);
    pixWrite("/tmp/adapt.jpg", pixd, IFF_JFIF_JPEG);
    pixDisplayWithTitle(pixd, 100, 100, NULL, 1);
    pixDestroy(&pixd);

    pixDestroy(&pixs);
    pixDestroy(&pixsg);
    return 0;
}

