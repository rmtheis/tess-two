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
 * sheartest.c
 *
 *     sheartest filein angle fileout
 *
 *     where angle is expressed in degrees
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define   NTIMES   10 


main(int    argc,
     char **argv)
{
char        *filein, *fileout;
l_int32      i, w, h, liney, linex, same;
l_float32    angle, deg2rad;
PIX         *pixt1, *pixt2, *pixs, *pixd;
static char  mainName[] = "sheartest";

    if (argc != 4)
	exit(ERROR_INT(" Syntax:  sheartest filein angle fileout",
		    mainName, 1));

        /* Compare in-place H shear with H shear to a new pix */
    pixt1 = pixRead("marge.jpg");
    pixGetDimensions(pixt1, &w, &h, NULL);
    pixt2 = pixHShear(NULL, pixt1, (l_int32)(0.3 * h), 0.17, L_BRING_IN_WHITE);
    pixHShearIP(pixt1, (l_int32)(0.3 * h), 0.17, L_BRING_IN_WHITE);
    pixEqual(pixt1, pixt2, &same);
    if (same)
        fprintf(stderr, "Correct for H shear\n");
    else
        fprintf(stderr, "Error for H shear\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

        /* Compare in-place V shear with V shear to a new pix */
    pixt1 = pixRead("marge.jpg");
    pixGetDimensions(pixt1, &w, &h, NULL);
    pixt2 = pixVShear(NULL, pixt1, (l_int32)(0.3 * w), 0.17, L_BRING_IN_WHITE);
    pixVShearIP(pixt1, (l_int32)(0.3 * w), 0.17, L_BRING_IN_WHITE);
    pixEqual(pixt1, pixt2, &same);
    if (same)
        fprintf(stderr, "Correct for V shear\n");
    else
        fprintf(stderr, "Error for V shear\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

    filein = argv[1];
    angle = atof(argv[2]);
    fileout = argv[3];
    deg2rad = 3.1415926535 / 180.;

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));

    pixGetDimensions(pixs, &w, &h, NULL);

#if 0
        /* Select an operation from this list ...
	 * ------------------------------------------
    pixd = pixHShear(NULL, pixs, liney, deg2rad * angle, L_BRING_IN_WHITE);
    pixd = pixVShear(NULL, pixs, linex, deg2rad * angle, L_BRING_IN_WHITE);
    pixd = pixHShearCorner(NULL, pixs, deg2rad * angle, L_BRING_IN_WHITE);
    pixd = pixVShearCorner(NULL, pixs, deg2rad * angle, L_BRING_IN_WHITE);
    pixd = pixHShearCenter(NULL, pixs, deg2rad * angle, L_BRING_IN_WHITE);
    pixd = pixVShearCenter(NULL, pixs, deg2rad * angle, L_BRING_IN_WHITE);
    pixHShearIP(pixs, liney, deg2rad * angle, L_BRING_IN_WHITE); pixd = pixs;
    pixVShearIP(pixs, linex, deg2rad * angle, L_BRING_IN_WHITE); pixd = pixs;
    pixRasteropHip(pixs, 0, h/3, -50, L_BRING_IN_WHITE); pixd = pixs;
    pixRasteropVip(pixs, 0, w/3, -50, L_BRING_IN_WHITE); pixd = pixs;
	 * ------------------------------------------
	 *  ... and use it in the following:         */
    pixd = pixHShear(NULL, pixs, liney, deg2rad * angle, L_BRING_IN_WHITE);
    pixWrite(fileout, pixd, IFF_PNG);
    pixDisplay(pixd, 50, 50);
    pixDestroy(&pixd);
#endif

#if 0
	/* Do a horizontal shear about a line */
    for (i = 0; i < NTIMES; i++) {
	liney = i * h / (NTIMES - 1);
	if (liney >= h)
	    liney = h - 1;
	pixd = pixHShear(NULL, pixs, liney, deg2rad * angle, L_BRING_IN_WHITE);
	pixDisplay(pixd, 50 + 10 * i, 50 + 10 * i);
	pixDestroy(&pixd);
    }
#endif

#if 0
	/* Do a vertical shear about a line */
    for (i = 0; i < NTIMES; i++) {
	linex = i * w / (NTIMES - 1);
	if (linex >= w)
	    linex = w - 1;
	pixd = pixVShear(NULL, pixs, linex, deg2rad * angle, L_BRING_IN_WHITE);
	pixDisplay(pixd, 50 + 10 * i, 50 + 10 * i);
	pixDestroy(&pixd);
    }
#endif

#if 0
	/* Do a horizontal in-place shear about a line */
    pixSetPadBits(pixs, 0);
    for (i = 0; i < NTIMES; i++) {
	pixd = pixCopy(NULL, pixs);
	liney = i * h / (NTIMES - 1);
	if (liney >= h)
	    liney = h - 1;
	pixHShearIP(pixd, liney, deg2rad * angle, L_BRING_IN_WHITE);
	pixDisplay(pixd, 50 + 10 * i, 50 + 10 * i);
	pixDestroy(&pixd);
    }
#endif

#if 0
	/* Do a vertical in-place shear about a line */
    for (i = 0; i < NTIMES; i++) {
	pixd = pixCopy(NULL, pixs);
	linex = i * w / (NTIMES - 1);
	if (linex >= w)
	    linex = w - 1;
	pixVShearIP(pixd, linex, deg2rad * angle, L_BRING_IN_WHITE);
	pixDisplay(pixd, 50 + 10 * i, 50 + 10 * i);
	pixDestroy(&pixd);
    }
#endif

    pixDestroy(&pixs);
    exit(0);
}

