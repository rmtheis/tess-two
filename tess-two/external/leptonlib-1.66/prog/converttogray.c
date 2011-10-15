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
 * converttogray.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

main(int    argc,
     char **argv)
{
char         *filein, *fileout;
l_int32       d, same;
PIX          *pixs, *pixd, *pixt0, *pixt1, *pixt2, *pixt3, *pixt4, *pixt5;
static char   mainName[] = "converttogray";

    if (argc != 2 && argc != 3)
	exit(ERROR_INT(" Syntax:  converttogray filein [fileout]",
	               mainName, 1));

    filein = argv[1];
    fileout = argv[2];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));

#if 0
    pixd = pixConvertRGBToGray(pixs, 0.33, 0.34, 0.33);
    pixWrite(fileout, pixd, IFF_PNG);
    pixDestroy(&pixd);
#endif

#if 1
    d = pixGetDepth(pixs);
    if (d == 2) {
	pixt1 = pixConvert2To8(pixs, 0x00, 0x55, 0xaa, 0xff, TRUE);
	pixt2 = pixConvert2To8(pixs, 0x00, 0x55, 0xaa, 0xff, FALSE);
	pixEqual(pixt1, pixt2, &same);
	if (same)
	    fprintf(stderr, "images are the same\n");
	else
	    fprintf(stderr, "images are different!\n");
	pixWrite("/tmp/junkpixt1", pixt1, IFF_PNG);
	pixWrite("/tmp/junkpixt2", pixt2, IFF_PNG);
	pixDestroy(&pixt1);
	pixDestroy(&pixt2);
	pixSetColormap(pixs, NULL);
	pixt3 = pixConvert2To8(pixs, 0x00, 0x55, 0xaa, 0xff, TRUE);
	pixt4 = pixConvert2To8(pixs, 0x00, 0x55, 0xaa, 0xff, FALSE);
	pixEqual(pixt3, pixt4, &same);
	if (same)
	    fprintf(stderr, "images are the same\n");
	else
	    fprintf(stderr, "images are different!\n");
	pixWrite("/tmp/junkpixt3", pixt3, IFF_PNG);
	pixWrite("/tmp/junkpixt4", pixt4, IFF_PNG);
	pixDestroy(&pixt3);
	pixDestroy(&pixt4);
    }
    else if (d == 4) {
	pixt1 = pixConvert4To8(pixs, TRUE);
	pixt2 = pixConvert4To8(pixs, FALSE);
	pixEqual(pixt1, pixt2, &same);
	if (same)
	    fprintf(stderr, "images are the same\n");
	else
	    fprintf(stderr, "images are different!\n");
	pixWrite("/tmp/junkpixt1", pixt1, IFF_PNG);
	pixWrite("/tmp/junkpixt2", pixt2, IFF_PNG);
	pixDestroy(&pixt1);
	pixDestroy(&pixt2);
	pixSetColormap(pixs, NULL);
	pixt3 = pixConvert4To8(pixs, TRUE);
	pixt4 = pixConvert4To8(pixs, FALSE);
	pixEqual(pixt3, pixt4, &same);
	if (same)
	    fprintf(stderr, "images are the same\n");
	else
	    fprintf(stderr, "images are different!\n");
	pixWrite("/tmp/junkpixt3", pixt3, IFF_PNG);
	pixWrite("/tmp/junkpixt4", pixt4, IFF_PNG);
	pixDestroy(&pixt3);
	pixDestroy(&pixt4);
    }
#endif

    pixDestroy(&pixs);

    exit(0);
}

