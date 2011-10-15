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
 * edgetest.c
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
l_int32      i, w, h, d;
l_float32    time;
PIX         *pixs, *pixf, *pixd;
PIXA        *pixa;
char        *filein, *fileout;
static char  mainName[] = "edgetest";

    if (argc != 3)
	exit(ERROR_INT(" Syntax:  edgetest filein fileout", mainName, 1));

    filein = argv[1];
    fileout = argv[2];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8)
	exit(ERROR_INT("pix not 8 bpp", mainName, 1));

        /* Speed: about 12 Mpix/GHz/sec */
    startTimer();
    pixf = pixSobelEdgeFilter(pixs, L_HORIZONTAL_EDGES);
    pixd = pixThresholdToBinary(pixf, 60);
    pixInvert(pixd, pixd);
    time = stopTimer();
    fprintf(stderr, "Time =  %7.3f sec\n", time);
    fprintf(stderr, "MPix/sec: %7.3f\n", 0.000001 * w * h / time);
    pixDisplay(pixs, 0, 0);
    pixInvert(pixf, pixf);
    pixDisplay(pixf, 480, 0);
    pixDisplay(pixd, 960, 0);
    pixWrite(fileout, pixf, IFF_PNG);
    pixDestroy(&pixd);

        /* Threshold at different values */
    pixInvert(pixf, pixf);
    for (i = 10; i <= 120; i += 10) {
        pixd = pixThresholdToBinary(pixf, i);
        pixInvert(pixd, pixd);
        pixDisplayWrite(pixd, 1);
        pixDestroy(&pixd);
    }
    pixDestroy(&pixf);

        /* Display tiled */
    pixa = pixaReadFiles("/tmp", "junk_write_display");
    pixd = pixaDisplayTiledAndScaled(pixa, 8, 400, 3, 0, 25, 2);
    pixWrite("/tmp/junktiles.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixDestroy(&pixs);
    exit(0);
}

