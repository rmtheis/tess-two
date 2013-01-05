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
 * seedfilltest.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define  NTIMES             5
#define  CONNECTIVITY       8
#define  XS                 150
#define  YS                 150

#define  DFLAG              1

main(int    argc,
     char **argv)
{
char        *filein, *fileout;
l_int32      i;
l_uint32     val;
l_float32    size;
PIX         *pixs, *pixd, *pixm, *pixmi, *pixt1, *pixt2, *pixt3;
static char  mainName[] = "seedfilltest";

    if (argc != 3)
	exit(ERROR_INT(" Syntax:  seedfilltest filein fileout", mainName, 1));

    filein = argv[1];
    fileout = argv[2];
    pixd = NULL;

    if ((pixm = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixm not made", mainName, 1));
    pixmi = pixInvert(NULL, pixm);
	    
    size = pixGetWidth(pixm) * pixGetHeight(pixm);
    pixs = pixCreateTemplate(pixm);
    for (i = 0; i < 100; i++) {
	pixGetPixel(pixm, XS + 5 * i, YS + 5 * i, &val);
	if (val == 0) break;
    }
    if (i == 100)
	exit(ERROR_INT("no seed pixel found", mainName, 1));
    pixSetPixel(pixs, XS + 5 * i, YS + 5 * i, 1);

#if 0
        /* hole filling; use "hole-filler.png" */
    pixt1 = pixHDome(pixmi, 100, 4);
    pixt2 = pixThresholdToBinary(pixt1, 10);
/*    pixInvert(pixt1, pixt1); */
    pixDisplay(pixt1, 100, 500);
    pixDisplay(pixt2, 600, 500);
    pixt3 = pixHolesByFilling(pixt2, 4);
    pixDilateBrick(pixt3, pixt3, 7, 7);
    pixd = pixConvertTo8(pixt3, FALSE);
    pixDisplay(pixd, 0, 100);
    pixSeedfillGray(pixd, pixmi, CONNECTIVITY);
    pixInvert(pixd, pixd);
    pixDisplay(pixmi, 500, 100);
    pixDisplay(pixd, 1000, 100);
    pixWrite("/tmp/junkpixm.png", pixmi, IFF_PNG);
    pixWrite("/tmp/junkpixd.png", pixd, IFF_PNG);
#endif

#if 0
        /* hole filling; use "hole-filler.png" */
    pixt1 = pixThresholdToBinary(pixm, 110);
    pixInvert(pixt1, pixt1);
    pixDisplay(pixt1, 100, 500);
    pixt2 = pixHolesByFilling(pixt1, 4);
    pixd = pixConvertTo8(pixt2, FALSE);
    pixDisplay(pixd, 0, 100);
    pixSeedfillGray(pixd, pixmi, CONNECTIVITY);
    pixInvert(pixd, pixd);
    pixDisplay(pixmi, 500, 100);
    pixDisplay(pixd, 1000, 100);
    pixWrite("/tmp/junkpixm.png", pixmi, IFF_PNG);
    pixWrite("/tmp/junkpixd.png", pixd, IFF_PNG);
#endif

#if 0
        /* hole filling; use "hole-filler.png" */
    pixd = pixInvert(NULL, pixm);
    pixAddConstantGray(pixd, -50);
    pixDisplay(pixd, 0, 100);
/*    pixt1 = pixThresholdToBinary(pixd, 20);
    pixDisplayWithTitle(pixt1, 600, 600, "pixt1", DFLAG); */
    pixSeedfillGray(pixd, pixmi, CONNECTIVITY);
/*    pixInvert(pixd, pixd); */
    pixDisplay(pixmi, 500, 100);
    pixDisplay(pixd, 1000, 100);
    pixWrite("/tmp/junkpixm.png", pixmi, IFF_PNG);
    pixWrite("/tmp/junkpixd.png", pixd, IFF_PNG);
#endif

#if 0  
	/* test in-place seedfill for speed */
    pixd = pixClone(pixs);
    startTimer();
    pixSeedfillBinary(pixs, pixs, pixmi, CONNECTIVITY);
    fprintf(stderr, "Filling rate: %7.4f Mpix/sec\n",
        (size/1000000.) / stopTimer());

    pixWrite(fileout, pixd, IFF_PNG);
    pixOr(pixd, pixd, pixm);
    pixWrite("/tmp/junkout1.png", pixd, IFF_PNG);
#endif

#if 0
	/* test seedfill to dest for speed */
    pixd = pixCreateTemplate(pixm);
    startTimer();
    for (i = 0; i < NTIMES; i++) {
	pixSeedfillBinary(pixd, pixs, pixmi, CONNECTIVITY);
    }
    fprintf(stderr, "Filling rate: %7.4f Mpix/sec\n",
        (size/1000000.) * NTIMES / stopTimer());

    pixWrite(fileout, pixd, IFF_PNG);
    pixOr(pixd, pixd, pixm);
    pixWrite("/tmp/junkout1.png", pixd, IFF_PNG);
#endif

        /* use same connectivity to compare with the result of the
	 * slow parallel operation */
#if 0
    pixDestroy(&pixd);
    pixd = pixSeedfillMorph(pixs, pixmi, CONNECTIVITY);
    pixOr(pixd, pixd, pixm);
    pixWrite("/tmp/junkout2.png", pixd, IFF_PNG);
#endif

    pixDestroy(&pixs);
    pixDestroy(&pixm);
    pixDestroy(&pixmi);
    pixDestroy(&pixd);

    return 0;
}


