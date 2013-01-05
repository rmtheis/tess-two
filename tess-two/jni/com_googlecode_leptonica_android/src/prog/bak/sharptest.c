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
 * sharptest.c
 *
 *      sharptest filein smooth fract fileout
 *
 *      (1) Use smooth = 1 for 3x3 smoothing filter
 *              smooth = 2 for 5x5 smoothing filter, etc.
 *      (2) Use fract in typical range (0.2 - 0.7) 
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
PIX         *pixs, *pixd;
l_int32      smooth;
l_float32    fract;
char        *filein, *fileout;
static char  mainName[] = "sharptest";

    if (argc != 5)
	exit(ERROR_INT(" Syntax:  sharptest filein smooth fract fileout",
	    mainName, 1));

    filein = argv[1];
    smooth = atoi(argv[2]);
    fract = atof(argv[3]);
    fileout = argv[4];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
	    
    pixd = pixUnsharpMasking(pixs, smooth, fract);

    pixWrite(fileout, pixd, IFF_JFIF_JPEG);

    pixDestroy(&pixs);
    pixDestroy(&pixd);
    exit(0);
}

