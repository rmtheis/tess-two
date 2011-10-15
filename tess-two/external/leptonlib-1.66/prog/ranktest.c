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
 * ranktest.c
 *  
 *     Tests rank filters on 8 and 32 bpp images.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
l_int32      i, wf, hf, w, h, d, same;
l_float32    rank, time;
PIX         *pixs, *pixd, *pixt1, *pixt2, *pixt3, *pixt4;
PIXA        *pixa;
char        *filein, *fileout;
static char  mainName[] = "ranktest";

    if (argc != 6)
	exit(ERROR_INT(" Syntax:  ranktest filein wf hf rank fileout",
                       mainName, 1));

    filein = argv[1];
    wf = atoi(argv[2]);
    hf = atoi(argv[3]);
    rank = atof(argv[4]);
    fileout = argv[5];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8 && d != 32)
	exit(ERROR_INT("pix neither 8 nor 32 bpp", mainName, 1));

    startTimer();
    pixd = pixRankFilter(pixs, wf, hf, rank);
    time = stopTimer();
    fprintf(stderr, "Time =  %7.3f sec\n", time);
    fprintf(stderr, "MPix/sec: %7.3f\n", 0.000001 * w * h / time);
    pixDisplay(pixs, 0, 0);
    pixDisplay(pixd, 600, 0);
    pixWrite(fileout, pixd, IFF_PNG);
    pixDestroy(&pixd);

        /* Get results for different rank values */
    for (i = 0; i <= 10; i++) {
        pixd = pixRankFilter(pixs, wf, hf, 0.1 * i);
        pixDisplayWrite(pixd, 1);
        pixDestroy(&pixd);
    }

        /* Make the dimensions odd to compare with dilation & erosion */
    if (wf % 2 == 0) wf++;
    if (hf % 2 == 0) hf++;

        /* Get results for dilation and erosion */
    if (d == 8) {
        pixt1 = pixDilateGray(pixs, wf, hf);
        pixt2 = pixErodeGray(pixs, wf, hf);
    } else {
        pixt1 = pixColorMorph(pixs, L_MORPH_DILATE, wf, hf);
        pixt2 = pixColorMorph(pixs, L_MORPH_ERODE, wf, hf);
    }
    pixDisplayWrite(pixt1, 1);  /* dilation */

        /* Get results using the rank filter for rank = 0.0 and 1.0.
         * Don't use 0.0 or 1.0, because those are dispatched
         * automatically to erosion and dilation! */
    pixt3 = pixRankFilter(pixs, wf, hf, 0.0001);
    pixt4 = pixRankFilter(pixs, wf, hf, 0.9999);

        /* Compare */
    pixEqual(pixt1, pixt4, &same);
    if (same)
        fprintf(stderr, "Correct: dilation results same as rank 1.0\n");
    else
        fprintf(stderr, "Error: dilation results differ from rank 1.0\n");
    pixEqual(pixt2, pixt3, &same);
    if (same)
        fprintf(stderr, "Correct: erosion results same as rank 0.0\n");
    else
        fprintf(stderr, "Error: erosion results differ from rank 0.0\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

        /* Display tiled */
    pixa = pixaReadFiles("/tmp", "junk_write_display");
    pixd = pixaDisplayTiledAndScaled(pixa, d, 400, 3, 0, 25, 2);
    pixWrite("/tmp/junktiles.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixDestroy(&pixs);
    return 0;
}

