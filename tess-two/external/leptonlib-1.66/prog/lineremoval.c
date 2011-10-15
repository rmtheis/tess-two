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
 * lineremoval.c
 *
 *     Use with dave-orig.png
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
char        *filein;
l_float32    angle, conf, deg2rad;
PIX         *pixs, *pix1, *pix2, *pix3, *pix4, *pix5;
PIX         *pix6, *pix7, *pix8, *pix9;
static char  mainName[] = "lineremoval";

    if (argc != 2)
	exit(ERROR_INT(" Syntax:  lineremoval filein", mainName, 1));

    filein = argv[1];

    deg2rad = 3.14159 / 180.;
    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));

        /* threshold to binary, extracting much of the lines */
    pix1 = pixThresholdToBinary(pixs, 170);
    pixWrite("/tmp/dave-proc1.png", pix1, IFF_PNG);
    pixDisplayWrite(pix1, 1);

	/* find the skew angle and deskew using an interpolated
	 * rotator for anti-aliasing (to avoid jaggies) */
    pixFindSkew(pix1, &angle, &conf);
    pix2 = pixRotateAMGray(pixs, deg2rad * angle, 255);
    pixWrite("/tmp/dave-proc2.png", pix2, IFF_PNG);
    pixDisplayWrite(pix2, 1);

        /* extract the lines to be removed */
    pix3 = pixCloseGray(pix2, 51, 1);
    pixWrite("/tmp/dave-proc3.png", pix3, IFF_PNG);
    pixDisplayWrite(pix3, 1);

        /* solidify the lines to be removed */
    pix4 = pixErodeGray(pix3, 1, 5);
    pixWrite("/tmp/dave-proc4.png", pix4, IFF_PNG);
    pixDisplayWrite(pix4, 1);

        /* clean the background of those lines */
    pix5 = pixThresholdToValue(NULL, pix4, 210, 255);
    pixWrite("/tmp/dave-proc5.png", pix5, IFF_PNG);
    pixDisplayWrite(pix5, 1);

    pix6 = pixThresholdToValue(NULL, pix5, 200, 0);
    pixWrite("/tmp/dave-proc6.png", pix6, IFF_PNG);
    pixDisplayWrite(pix6, 1);

        /* get paint-through mask for changed pixels */
    pix7 = pixThresholdToBinary(pix6, 210);
    pixWrite("/tmp/dave-proc7.png", pix7, IFF_PNG);
    pixDisplayWrite(pix7, 1);

        /* add the inverted, cleaned lines to orig.  Because 
	 * the background was cleaned, the inversion is 0,
	 * so when you add, it doesn't lighten those pixels.
	 * It only lightens (to white) the pixels in the lines! */
    pixInvert(pix6, pix6);
    pix8 = pixAddGray(NULL, pix2, pix6);
    pixWrite("/tmp/dave-proc8.png", pix8, IFF_PNG);
    pixDisplayWrite(pix8, 1);

    pix9 = pixOpenGray(pix8, 1, 9);
    pixWrite("/tmp/dave-proc9.png", pix9, IFF_PNG);
    pixDisplayWrite(pix9, 1);

    pixCombineMasked(pix8, pix9, pix7);
    pixWrite("/tmp/dave-result.png", pix8, IFF_PNG);
    pixDisplayWrite(pix8, 1);

    pixDisplayMultiple("/tmp/junk_write_display*");
    return 0;
}

