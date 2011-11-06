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
 * bincompare.c
 *
 *    Bitwise comparison of two binary images
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

    /* set one of these to 1 */
#define   XOR                   1
#define   SUBTRACT_1_FROM_2     0
#define   SUBTRACT_2_FROM_1     0

main(int    argc,
     char **argv)
{
l_int32      w, h, d, n;
char        *filein1, *filein2, *fileout;
PIX         *pixs1, *pixs2, *pixd;
static char  mainName[] = "bincompare";

    if (argc != 4)
	exit(ERROR_INT(" Syntax:  bincompare filein1 filein2 fileout",
	               mainName, 1));

    filein1 = argv[1];
    filein2 = argv[2];
    fileout = argv[3];

    if ((pixs1 = pixRead(filein1)) == NULL)
	exit(ERROR_INT("pixs1 not made", mainName, 1));
    if ((pixs2 = pixRead(filein2)) == NULL)
	exit(ERROR_INT("pixs2 not made", mainName, 1));

    w = pixGetWidth(pixs1);
    h = pixGetHeight(pixs1);
    d = pixGetDepth(pixs1);
    if (d != 1)
	exit(ERROR_INT("pixs1 not binary", mainName, 1));

    pixCountPixels(pixs1, &n, NULL);
    fprintf(stderr, "Number of fg pixels in file1 = %d\n", n);
    pixCountPixels(pixs2, &n, NULL);
    fprintf(stderr, "Number of fg pixels in file2 = %d\n", n);

#if XOR 
    fprintf(stderr, "xor: 1 ^ 2\n");
    pixRasterop(pixs1, 0, 0, w, h, PIX_SRC ^ PIX_DST, pixs2, 0, 0);
    pixCountPixels(pixs1, &n, NULL);
    fprintf(stderr, "Number of fg pixels in XOR = %d\n", n);
    pixWrite(fileout, pixs1, IFF_PNG);
#elif  SUBTRACT_1_FROM_2
    fprintf(stderr, "subtract: 2 - 1\n");
    pixRasterop(pixs1, 0, 0, w, h, PIX_SRC & PIX_NOT(PIX_DST), pixs2, 0, 0);
    pixCountPixels(pixs1, &n, NULL);
    fprintf(stderr, "Number of fg pixels in 2 - 1 = %d\n", n);
    pixWrite(fileout, pixs1, IFF_PNG);
#elif  SUBTRACT_2_FROM_1
    fprintf(stderr, "subtract: 1 - 2\n");
    pixRasterop(pixs1, 0, 0, w, h, PIX_DST & PIX_NOT(PIX_SRC), pixs2, 0, 0);
    pixCountPixels(pixs1, &n, NULL);
    fprintf(stderr, "Number of fg pixels in 1 - 2 = %d\n", n);
    pixWrite(fileout, pixs1, IFF_PNG);
#else
    fprintf(stderr, "no comparison selected\n");
#endif

    return 0;
}

