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
 * arithtest.c
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

main(int    argc,
     char **argv)
{
char        *filein;
l_int32      w, h, same;
PIX         *pixs, *pix1, *pix2, *pix3, *pix4, *pix5;
static char  mainName[] = "arithtest";

    if (argc != 2)
	exit(ERROR_INT(" Syntax:  arithtest filein", mainName, 1));

    filein = argv[1];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));

    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);

        /* input a grayscale image and convert it to 16 bpp */
    pix1 = pixInitAccumulate(w, h, 0);
    pixAccumulate(pix1, pixs, L_ARITH_ADD);
    pixMultConstAccumulate(pix1, 255., 0);
    pix2 = pixFinalAccumulate(pix1, 0, 16);
    l_pngSetStrip16To8(0);
    pixWrite("/tmp/junkpix1.png", pix2, IFF_PNG);

        /* convert it back to 8 bpp, linear mapped */
    pix3 = pixMaxDynamicRange(pix2, L_LINEAR_SCALE);
    pixWrite("/tmp/junkpix2.png", pix3, IFF_PNG);

        /* convert it back to 8 bpp using the MSB */
    pix4 = pixRead("/tmp/junkpix1.png");
    pix5 = pixConvert16To8(pix4, 1);
    pixWrite("/tmp/junkpix3.png", pix5, IFF_PNG);
    return 0;
}

