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
 * livre_seedgen.c
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

main(int    argc,
     char **argv)
{
l_int32      i;
PIX         *pixs, *pixt1, *pixt2, *pixt3, *pixd;
PIXA        *pixa;
static char  mainName[] = "livre_seedgen";

    pixs = pixRead("pageseg2.tif");

    startTimer();
    for (i = 0; i < 100; i++) {
        pixt1 = pixReduceRankBinaryCascade(pixs, 1, 4, 4, 3);
        pixDestroy(&pixt1);
    }
    fprintf(stderr, "Time: %8.4f sec\n", stopTimer() / 100.);

        /* 4 2x rank reductions (levels 1, 4, 4, 3), followed by 5x5 opening */
    pixDisplayWrite(NULL, -1);
    pixDisplayWriteFormat(pixs, 4, IFF_PNG);
    pixt1 = pixReduceRankBinaryCascade(pixs, 1, 4, 0, 0);
    pixDisplayWriteFormat(pixt1, 1, IFF_PNG);
    pixt2 = pixReduceRankBinaryCascade(pixt1, 4, 3, 0, 0);
    pixDisplayWriteFormat(pixt2, 1, IFF_PNG);
    pixOpenBrick(pixt2, pixt2, 5, 5);
    pixt3 = pixExpandBinaryReplicate(pixt2, 2);
    pixDisplayWriteFormat(pixt3, 1, IFF_PNG);

        /* Generate the output image */
    pixa = pixaReadFiles("/tmp", "junk_write_display");
    pixd = pixaDisplayTiledAndScaled(pixa, 8, 250, 4, 0, 25, 2);
    pixWrite("/tmp/seedgen.png", pixd, IFF_PNG);

    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    return 0;
}

