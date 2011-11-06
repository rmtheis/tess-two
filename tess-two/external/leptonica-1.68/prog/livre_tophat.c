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
 * livre_tophat.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
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
    pixa = pixaReadFiles("/tmp", "junk_write_display");
    pixd = pixaDisplayTiledAndScaled(pixa, 8, 350, 3, 0, 25, 2);
    pixWrite("/tmp/tophat.jpg", pixd, IFF_JFIF_JPEG);
    pixDisplay(pixd, 0, 0);
    pixDestroy(&pixd);

    pixDestroy(&pixs);
    pixDestroy(&pixsg);
    return 0;
}

