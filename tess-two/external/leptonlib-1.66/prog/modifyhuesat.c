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
 * modifyhuesat.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
char        *filein, *fileout;
l_int32      i, j, w, d, nhue, nsat, tilewidth;
l_float32    dhue, dsat, delhue, delsat;
PIX         *pix, *pixs, *pixt1, *pixt2, *pixd;
PIXA        *pixa;
static char  mainName[] = "modifyhuesat";

    if (argc != 7) {
        ERROR_INT(" Syntax: modifyhuesat filein nhue dhue nsat dsat fileout",
                  mainName, 1);
        return 1;
    }

    filein = argv[1];
    nhue = atoi(argv[2]);
    dhue = atof(argv[3]);
    nsat = atoi(argv[4]);
    dsat = atof(argv[5]);
    fileout = argv[6];

    if (nhue % 2 == 0) {
        nhue++;
        fprintf(stderr, "nhue must be odd; raised to %d\n", nhue);
    }
    if (nsat % 2 == 0) {
        nsat++;
        fprintf(stderr, "nsat must be odd; raised to %d\n", nsat);
    }

    if ((pix = pixRead(filein)) == NULL)
        return ERROR_INT("pix not read", mainName, 1);
    pixs = pixScale(pix, 0.25, 0.25);
    pixGetDimensions(pixs, &w, NULL, &d);
    pixa = pixaCreate(nhue * nsat);
    for (i = 0; i < nsat; i++) {
        delsat = (i - nsat / 2) * dsat;
	pixt1 = pixModifySaturation(NULL, pixs, delsat);
        for (j = 0; j < nhue; j++) {
            delhue = (j - nhue / 2) * dhue;
            pixt2 = pixModifyHue(NULL, pixt1, delhue);
            pixaAddPix(pixa, pixt2, L_INSERT);
        }
        pixDestroy(&pixt1);
    }

    tilewidth = L_MIN(w, 1500 / nsat);
    pixd = pixaDisplayTiledAndScaled(pixa, d, tilewidth, nsat, 0, 25, 3);
    pixWrite(fileout, pixd, IFF_JFIF_JPEG);

    pixDestroy(&pix);
    pixDestroy(&pixs);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    return 0;
}

