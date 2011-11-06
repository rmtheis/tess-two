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
 * morphtest1.c
 *
 *   - Timing test for rasterop-based morphological operations
 *   - Example repository of binary morph operations
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define   NTIMES         100
#define   IMAGE_SIZE     8.     /* megapixels */
#define   SEL_SIZE       9
#define   BASIC_OPS      1.     /* 1 for erosion/dilation; 2 for open/close */
#define   CPU_SPEED      866.   /* MHz: set it for the machine you're using */



main(int    argc,
     char **argv)
{
l_int32      i, index;
l_float32    cputime, epo;
char        *filein, *fileout;
PIX         *pixs, *pixd;
SEL         *sel;
SELA        *sela;
static char  mainName[] = "morphtest1";

    if (argc != 3)
	exit(ERROR_INT(" Syntax:  morphtest1 filein fileout", mainName, 1));

    filein = argv[1];
    fileout = argv[2];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));
    sela = selaAddBasic(NULL);

    /* ------------------------   Timing  -------------------------------*/
#if 1
    selaFindSelByName(sela, "sel_9h", &index, &sel);
    selWriteStream(stderr, sel);
    pixd = pixCreateTemplate(pixs);

    startTimer();
    for (i = 0; i < NTIMES; i++)  {
	pixDilate(pixd, pixs, sel);
/*	if ((i % 10) == 0) fprintf(stderr, "%d iters\n", i); */
    }
    cputime = stopTimer();
        /* Get the elementary pixel operations/sec */
    epo = BASIC_OPS * SEL_SIZE * NTIMES * IMAGE_SIZE /(cputime * CPU_SPEED);

    fprintf(stderr, "Time: %7.3f sec\n", cputime);
    fprintf(stderr, "Speed: %7.3f epo/cycle\n", epo);
    pixWrite(fileout, pixd, IFF_PNG);
    pixDestroy(&pixd);
#endif

    /* ------------------  Example operation from repository --------------*/
#if 1
	/* Select a structuring element */
    selaFindSelByName(sela, "sel_50h", &index, &sel);
    selWriteStream(stderr, sel);

        /* Do these operations.  See below for other ops
	 * that can be substituted here. */
    pixd = pixOpen(NULL, pixs, sel);
    pixXor(pixd, pixd, pixs);
    pixWrite(fileout, pixd, IFF_PNG);
    pixDestroy(&pixd);
#endif

    pixDestroy(&pixs);
    exit(0);
}


/* ==================================================================== */

/* -------------------------------------------------------------------- *
 *                 Repository for selecting various operations          *
 *                              that might be used                      *
 * -------------------------------------------------------------------- */
#if 0
    pixd = pixCreateTemplate(pixs);

    pixd = pixDilate(NULL, pixs, sel);
    pixd = pixErode(NULL, pixs, sel);
    pixd = pixOpen(NULL, pixs, sel);
    pixd = pixClose(NULL, pixs, sel);

    pixDilate(pixd, pixs, sel);
    pixErode(pixd, pixs, sel);
    pixOpen(pixd, pixs, sel);
    pixClose(pixd, pixs, sel);

    pixAnd(pixd, pixd, pixs);
    pixOr(pixd, pixd, pixs);
    pixXor(pixd, pixd, pixs);
    pixSubtract(pixd, pixd, pixs);
    pixInvert(pixd, pixs);

    pixd = pixAnd(NULL, pixd, pixs);
    pixd = pixOr(NULL, pixd, pixs);
    pixd = pixXor(NULL, pixd, pixs);
    pixd = pixSubtract(NULL, pixd, pixs);
    pixd = pixInvert(NULL, pixs);

    pixInvert(pixs, pixs);
#endif  /* 0 */

