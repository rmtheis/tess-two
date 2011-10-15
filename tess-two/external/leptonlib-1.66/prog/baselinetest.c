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
 * baselinetest.c
 *
 *   - e.g., use keystone.png as the input image
 *   - to get plots of baseline locations and other derived
 *     parameters, set DEBUG_PLOT to 1 in baseline.c 
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
char        *filein, *fileout;
NUMA        *na;
PIX         *pixs, *pixd;
PTA         *pta;
static char  mainName[] = "baselinetest";

    if (argc != 3)
	exit(ERROR_INT(" Syntax:  baselinetest filein fileout", mainName, 1));

    filein = argv[1];
    fileout = argv[2];

    pixs = pixRead(filein);

#if 1
        /* test function for deskewing using projective transform
	 * on linear approximation for local skew angle */
    pixd = pixDeskewLocal(pixs, 10, 0, 0, 0.0, 0.0, 0.0);
    pixWrite(fileout, pixd, IFF_TIFF_G4);

        /* test baseline finder */
    na = pixFindBaselines(pixd, &pta, 1);
/*    ptaWriteStream(stderr, pta, 1); */
    pixDestroy(&pixd);
    numaDestroy(&na);
    ptaDestroy(&pta);
#endif

#if 0
        /* test function for finding local skew angles */
    na = pixGetLocalSkewAngles(pixs, 10, 0, 0, 0.0, 0.0, 0.0, NULL, NULL);
    numaDestroy(&na);
#endif

    pixDestroy(&pixs);
    exit(0);
}



