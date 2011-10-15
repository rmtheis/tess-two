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
 * rotateorthtest1.c
 *
 *    Tests and timings for 90 and 180 degree rotations
 *        rotateorthtest1 filein fileout [direction]
 *    where
 *        direction = 1 for cw; -1 for ccw
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define  NTIMES   10


main(int    argc,
     char **argv)
{
l_int32      i, w, h, dir;
PIX         *pixs, *pixd, *pixt;
l_float32    pops;
char        *filein, *fileout;
static char  mainName[] = "rotateorthtest1";

    if (argc != 3 && argc != 4)
	exit(ERROR_INT(" Syntax:  rotateorthtest1 filein fileout [direction]",
	       mainName, 1));

    filein = argv[1];
    fileout = argv[2];
    if (argc == 4)
	dir = atoi(argv[3]);
    else
	dir = 1;

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));

        /* Do a single operation */
#if 1
    pixd = pixRotate90(pixs, dir);
#elif 0
    pixd = pixRotate180(NULL, pixs);
#elif 0
    pixd = pixRotateLR(NULL, pixs);
#elif 0
    pixd = pixRotateTB(NULL, pixs);
#endif

	/* Time rotate 90, allocating & destroying each time */
#if 0
    startTimer();
    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);
    for (i = 0; i < NTIMES; i++) {
	pixd = pixRotate90(pixs, dir);
	pixDestroy(&pixd);
    }
    pops = (l_float32)(w * h * NTIMES) / stopTimer();
    fprintf(stderr, "MPops for 90 rotation: %7.3f\n", pops / 1000000.);
    pixd = pixRotate90(pixs, dir);
#endif

	/* Time rotate 180, with no alloc/destroy */
#if 0
    startTimer();
    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);
    pixd = pixCreateTemplate(pixs);
    for (i = 0; i < NTIMES; i++)
	pixRotate180(pixd, pixs);
    pops = (l_float32)(w * h * NTIMES) / stopTimer();
    fprintf(stderr, "MPops for 180 rotation: %7.3f\n", pops / 1000000.);
#endif


	/* Test rotate 180 not in-place */
#if 0
    pixt = pixRotate180(NULL, pixs);
    pixd = pixRotate180(NULL, pixt);
    pixEqual(pixs, pixd, &eq);
    if (eq) fprintf(stderr, "2 rots gives I\n");
    else fprintf(stderr, "2 rots fail to give I\n");
    pixDestroy(&pixt);
#endif

       /* Test rotate 180 in-place */
#if 0
    pixd = pixCopy(NULL, pixs);
    pixRotate180(pixd, pixd);
    pixRotate180(pixd, pixd);
    pixEqual(pixs, pixd, &eq);
    if (eq) fprintf(stderr, "2 rots gives I\n");
    else fprintf(stderr, "2 rots fail to give I\n");
#endif

	/* Mix rotate 180 with LR/TB */
#if 0
    pixd = pixRotate180(NULL, pixs);
    pixRotateLR(pixd, pixd);
    pixRotateTB(pixd, pixd);
    pixEqual(pixs, pixd, &eq);
    if (eq) fprintf(stderr, "180 rot OK\n");
    else fprintf(stderr, "180 rot error\n");
#endif

    if (pixGetDepth(pixd) < 8)
	pixWrite(fileout, pixd, IFF_PNG);
    else
	pixWrite(fileout, pixd, IFF_JFIF_JPEG);

    pixDestroy(&pixs);
    pixDestroy(&pixd);
    exit(0);
}

