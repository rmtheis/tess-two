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
 * trctest.c
 *
 *   Example: trctest wet-day.jpg 3.1 50 160 /tmp/junk.png
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
PIX         *pixs, *pixd;
l_int32      minval, maxval;
l_float32    gamma;
char        *filein, *fileout;
static char  mainName[] = "trctest";

    if (argc != 6)
	exit(ERROR_INT(" Syntax:  trctest filein gamma minval maxval fileout",
             mainName, 1));

    filein = argv[1];
    gamma = atof(argv[2]);
    minval = atoi(argv[3]);
    maxval = atoi(argv[4]);
    fileout = argv[5];

    if ((pixs = pixRead(filein)) == NULL)
        exit(ERROR_INT("pixs not made", mainName, 1));
	    
    pixd = pixGammaTRC(NULL, pixs, gamma, minval, maxval);

    pixWrite(fileout, pixd, IFF_PNG);
    pixDestroy(&pixs);
    pixDestroy(&pixd);
    return 0;
}

