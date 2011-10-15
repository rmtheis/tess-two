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
 * graymorphtest.c
 *
 *     Implements basic grayscale morphology; tests speed
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
char           *filein, *fileout;
l_int32         wsize, hsize, w, h;
PIX            *pixs, *pixd;
static char     mainName[] = "graymorphtest";

    if (argc != 5)
	exit(ERROR_INT(" Syntax:  graymorphtest wsize, hsizefilein fileout",
                       mainName, 1));

    filein = argv[1];
    wsize = atoi(argv[2]);
    hsize = atoi(argv[3]);
    fileout = argv[4];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));

    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);

    /* ---------- Choose an operation ----------  */
#if 1
    pixd = pixDilateGray(pixs, wsize, hsize);
    pixWrite(fileout, pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
#elif 0
    pixd = pixErodeGray(pixs, wsize, hsize);
    pixWrite(fileout, pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
#elif 0
    pixd = pixOpenGray(pixs, wsize, hsize);
    pixWrite(fileout, pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
#elif 0
    pixd = pixCloseGray(pixs, wsize, hsize);
    pixWrite(fileout, pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
#elif 0
    pixd = pixTophat(pixs, wsize, hsize, TOPHAT_WHITE);
    pixWrite(fileout, pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
#elif 0
    pixd = pixTophat(pixs, wsize, hsize, TOPHAT_BLACK);
    pixWrite(fileout, pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
#endif


    /* ---------- Speed ----------  */
#if 0 
    startTimer();
    pixd = pixCloseGray(pixs, wsize, hsize);
    fprintf(stderr, " Speed is %6.2f MPix/sec\n",
          (l_float32)(4 * w * h) / (1000000. * stopTimer()));
    pixWrite(fileout, pixd, IFF_PNG);
#endif

    pixDestroy(&pixs);
    exit(0);
}


