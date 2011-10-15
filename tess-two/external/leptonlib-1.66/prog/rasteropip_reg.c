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
 *   rasteropip_reg.c
 *
 *   Tests in-place operation using the general 2-image pixRasterop().
 *   The in-place operation works because there is no overlap
 *   between the src and dest rectangles.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
char **argv)
{
l_int32      i, j, equal;
PIX         *pixs, *pixt, *pixd;
static char  mainName[] = "rasteropip_reg";


    pixs = pixRead("test8.jpg");
    pixt = pixCopy(NULL, pixs);

        /* Copy, in-place and one COLUMN at a time, from the right
           side to the left side. */
    for (j = 0; j < 200; j++)
        pixRasterop(pixs, 20 + j, 20, 1, 250, PIX_SRC, pixs, 250 + j, 20);
    pixDisplay(pixs, 50, 50);

        /* Copy, in-place and one ROW at a time, from the right
           side to the left side. */
    for (i = 0; i < 250; i++)
        pixRasterop(pixt, 20, 20 + i, 200, 1, PIX_SRC, pixt, 250, 20 + i);
    pixDisplay(pixt, 620, 50);

        /* Test */
    pixEqual(pixs, pixt, &equal);
    if (equal)
         fprintf(stderr, "OK: images are the same\n");
    else
         fprintf(stderr, "Error: images are different\n");
    pixWrite("/tmp/junkpix.png", pixs, IFF_PNG);
    pixDestroy(&pixs);
    pixDestroy(&pixt);


        /* Show the mirrored border, which uses the general
           pixRasterop() on an image in-place.  */
    pixs = pixRead("test8.jpg");
    pixt = pixRemoveBorder(pixs, 40);
    pixd = pixAddMirroredBorder(pixt, 25, 25, 25, 25);
    pixDisplay(pixd, 50, 550);
    pixDestroy(&pixs);
    pixDestroy(&pixt);
    pixDestroy(&pixd);

    return 0;
}


