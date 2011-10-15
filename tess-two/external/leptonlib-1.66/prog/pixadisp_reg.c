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
 *   pixadisp_reg.c
 *
 *     Exercises various pixaDisplay*() functions.
 *
 *     Syntax:  pixadisp_reg
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
l_int32      ws, hs;
BOX         *box;
BOXA        *boxa;
PIX         *pixs, *pixc, *pix32, *pixt, *pixd;
PIXA        *pixat, *pixas, *pixac;
static char  mainName[] = "pixadisp_reg";

    if (argc != 1)
        exit(ERROR_INT(" Syntax: pixadisp_reg", mainName, 1));

    if ((pixs = pixRead("feyn.tif")) == NULL)
        exit(ERROR_INT("pixs not made", mainName, 1));
    box = boxCreate(683, 799, 970, 479);
    pixc = pixClipRectangle(pixs, box, NULL);
    boxDestroy(&box);
    pixDisplayWrite(pixc, 1);
    if ((pix32 = pixRead("marge.jpg")) == NULL)
        exit(ERROR_INT("pix32 not made", mainName, 1));

        /* Generate pixas from pixs and pixac from pixc */
    boxa = pixConnComp(pixs, &pixat, 8);
    pixas = pixaSelectBySize(pixat, 60, 60, L_SELECT_IF_BOTH,
                             L_SELECT_IF_LTE, NULL);
    pixaDestroy(&pixat);
    boxaDestroy(&boxa);
    boxa = pixConnComp(pixc, &pixac, 8);
    boxaDestroy(&boxa);
 
        /* pixaDisplay() */
    pixGetDimensions(pixs, &ws, &hs, NULL);
    pixd = pixaDisplay(pixas, ws, hs);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);

        /* pixaDisplayRandomCmap() */
    pixd = pixaDisplayRandomCmap(pixas, ws, hs);  /* black bg */
    pixDisplayWrite(pixd, 1);
    pixcmapResetColor(pixGetColormap(pixd), 0, 255, 255, 255);  /* white bg */
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);

        /* pixaDisplayOnLattice() */
    pixd = pixaDisplayOnLattice(pixac, 50, 50);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);

        /* pixaDisplayUnsplit() */
    pixat = pixaSplitPix(pix32, 5, 7, 10, 0x0000ff00);
    pixd = pixaDisplayUnsplit(pixat, 5, 7, 10, 0x00ff0000);
    pixDisplayWrite(pixd, 1);
    pixaDestroy(&pixat);
    pixDestroy(&pixd);

        /* pixaDisplayTiled() */
    pixd = pixaDisplayTiled(pixac, 1000, 0, 10);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);

        /* pixaDisplayTiledInRows() */
    pixd = pixaDisplayTiledInRows(pixac, 1, 1000, 1.0, 0, 10, 2);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);


        /* pixaDisplayTiledAndScaled() */
    pixd = pixaDisplayTiledAndScaled(pixac, 1, 25, 20, 0, 5, 0);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd);

    pixat = pixaCreate(10);
    pixd = pixRankFilter(pix32, 8, 8, 0.5);
    pixaAddPix(pixat, pixd, L_INSERT);
    pixt = pixScale(pix32, 0.5, 0.5);
    pixd = pixRankFilter(pixt, 8, 8, 0.5);
    pixaAddPix(pixat, pixd, L_INSERT);
    pixDestroy(&pixt);
    pixt = pixScale(pix32, 0.25, 0.25);
    pixd = pixRankFilter(pixt, 8, 8, 0.5);
    pixaAddPix(pixat, pixd, L_INSERT);
    pixDestroy(&pixt);
    pixd = pixaDisplayTiledAndScaled(pixat, 32, 500, 1, 0, 25, 0);
    pixDisplayWrite(pixd, 1);
    pixaDestroy(&pixat);
    pixDestroy(&pixd);

    pixaDestroy(&pixas);
    pixaDestroy(&pixac);
    pixDestroy(&pixs);
    pixDestroy(&pixc);
    pixDestroy(&pix32);

    pixDisplayMultiple("/tmp/junk_write_display*");
    return 0;
}


