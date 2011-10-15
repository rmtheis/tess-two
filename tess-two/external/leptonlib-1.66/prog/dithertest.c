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
 * dithertest.c
 *
 *    Input is 8 bpp grayscale
 *    Output file is PostScript, 2 bpp dithered
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const l_float32  FACTOR = 0.95;
static const l_float32  GAMMA = 1.0;


main(int    argc,
     char **argv)
{
char        *filein, *fileout;
l_int32      w, h;
l_float32    scale;
FILE        *fp;
PIX         *pix, *pixs, *pixd;
PIXCMAP     *cmap;
static char  mainName[] = "dithertest";

    if (argc != 3)
	exit(ERROR_INT(" Syntax:  dithertest filein fileout", mainName, 1));

    filein = argv[1];
    fileout = argv[2];

    if ((pix = pixRead(filein)) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));
    if (pixGetDepth(pix) != 8)
	exit(ERROR_INT("pix not 8 bpp", mainName, 1));
    pixs = pixGammaTRC(NULL, pix, GAMMA, 0, 255);

    startTimer();
    pixd = pixDitherToBinary(pixs);
    fprintf(stderr, " time for binarized dither = %7.3f sec\n", stopTimer());
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);

         /* Dither to 2 bpp, with colormap */
    startTimer();
    pixd = pixDitherTo2bpp(pixs, 1);
    fprintf(stderr, " time for dither = %7.3f sec\n", stopTimer());
    pixDisplayWrite(pixd, 1);
    cmap = pixGetColormap(pixd);
    pixcmapWriteStream(stderr, cmap);
    pixDestroy(&pixd);

         /* Dither to 2 bpp, without colormap */
    startTimer();
    pixd = pixDitherTo2bpp(pixs, 0);
    fprintf(stderr, " time for dither = %7.3f sec\n", stopTimer());
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);

         /* Dither to 2 bpp, without colormap; output in PostScript */
    pixd = pixDitherTo2bpp(pixs, 0);
    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);
    scale = L_MIN(FACTOR * 2550 / w, FACTOR * 3300 / h);
    fp = fopen(fileout, "wb+");
    pixWriteStreamPS(fp, pixd, NULL, 300, scale);
    fclose(fp);
    pixDestroy(&pixd);

        /* Dither 2x upscale to 1 bpp */
    startTimer();
    pixd = pixScaleGray2xLIDither(pixs);
    fprintf(stderr, " time for scale/dither = %7.3f sec\n", stopTimer());
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);

        /* Dither 4x upscale to 1 bpp */
    startTimer();
    pixd = pixScaleGray4xLIDither(pixs);
    fprintf(stderr, " time for scale/dither = %7.3f sec\n", stopTimer());
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);

    pixDisplayMultiple("/tmp/junk_write_display*");

    pixDestroy(&pix);
    pixDestroy(&pixs);
    return 0;
}

