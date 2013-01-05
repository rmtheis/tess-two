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
 * reducetest.c
 *
 *    Carries out a rank binary cascade of up to four 2x reductions.
 *    This requires all four rank levels to be input; to stop the
 *    cascade, use 0 for the final rank level(s).
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
PIX         *pixs, *pixd;
l_int32      level1, level2, level3, level4;
char        *filein, *fileout;
static char  mainName[] = "reducetest";

    if (argc != 7)
	exit(ERROR_INT(" Syntax:  reducetest filein fileout l1 l2 l3 l4",
		    mainName, 1));

    filein = argv[1];
    fileout = argv[2];
    level1 = atoi(argv[3]);
    level2 = atoi(argv[4]);
    level3 = atoi(argv[5]);
    level4 = atoi(argv[6]);

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
	    
#if 1
    pixd = pixReduceRankBinaryCascade(pixs, level1, level2, level3, level4);
#endif  

#if 0
    pixd = pixReduce2(pixs, NULL);
#endif  

    pixWrite(fileout, pixd, IFF_PNG);

    exit(0);
}

