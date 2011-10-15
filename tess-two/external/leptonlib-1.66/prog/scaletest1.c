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
 * scaletest1.c
 *
 *      scaletest1 filein scalex scaley fileout
 *    where
 *      scalex, scaley are floating point input
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
char        *filein, *fileout;
l_int32      d;
l_float32    scalex, scaley;
PIX         *pixs, *pixd;
static char  mainName[] = "scaletest1";

    if (argc != 5)
	exit(ERROR_INT(" Syntax:  scaletest1 filein scalex scaley fileout",
	                 mainName, 1));

    filein = argv[1];
    scalex = atof(argv[2]);
    scaley = atof(argv[3]);
    fileout = argv[4];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
	    
        /* choose type of scaling operation */
#if 1
    pixd = pixScale(pixs, scalex, scaley);
#elif 0
    pixd = pixScaleLI(pixs, scalex, scaley);
#elif 0
    pixd = pixScaleSmooth(pixs, scalex, scaley);
#elif 0
    pixd = pixScaleAreaMap(pixs, scalex, scaley);
#else
    pixd = pixScaleBySampling(pixs, scalex, scaley);
#endif

    d = pixGetDepth(pixd);

#if 1
    if (d == 1)
        pixWrite(fileout, pixd, IFF_PNG);
    else
        pixWrite(fileout, pixd, IFF_JFIF_JPEG);
#else
    pixWrite(fileout, pixd, IFF_PNG);
#endif

    pixDestroy(&pixs);
    pixDestroy(&pixd);
    exit(0);
}

