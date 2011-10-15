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
 * skewtest.c
 *
 *     Tests various skew finding methods, optionally deskewing
 *     the input (binary) image.  The best version does a linear
 *     sweep followed by a binary (angle-splitting) search.
 *     The basic method is to find the vertical shear angle such
 *     that the differential variance of ON pixels between each
 *     line and it's neighbor, when summed over all lines, is
 *     maximized.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

    /* deskew */
#define   DESKEW_REDUCTION      2      /* 1, 2 or 4 */

    /* sweep only */
#define   SWEEP_RANGE           5.     /* degrees */
#define   SWEEP_DELTA           0.2    /* degrees */
#define   SWEEP_REDUCTION       2      /* 1, 2, 4 or 8 */

    /* sweep and search */
#define   SWEEP_RANGE2          5.     /* degrees */
#define   SWEEP_DELTA2          1.     /* degrees */
#define   SWEEP_REDUCTION2      2      /* 1, 2, 4 or 8 */
#define   SEARCH_REDUCTION      2      /* 1, 2, 4 or 8 */
#define   SEARCH_MIN_DELTA      0.01   /* degrees */


main(int    argc,
     char **argv)
{
char        *filein, *fileout;
l_float32    deg2rad;
l_float32    angle, conf;
PIX         *pixs, *pixd;
static char  mainName[] = "skewtest";

    if (argc != 3)
	exit(ERROR_INT(" Syntax:  skewtest filein fileout", mainName, 1));

    filein = argv[1];
    fileout = argv[2];

    deg2rad = 3.1415926535 / 180.;

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));

#if 1
    pixd = pixDeskew(pixs, DESKEW_REDUCTION);
    pixWrite(fileout, pixd, IFF_PNG);
    pixDestroy(&pixd);
#endif

#if 0
    if (pixFindSkew(pixs, &angle, &conf)) {
	L_WARNING("skew angle not valid", mainName);
	exit(1);
    }
#endif

#if 0
    if (pixFindSkewSweep(pixs, &angle, SWEEP_REDUCTION,
                         SWEEP_RANGE, SWEEP_DELTA)) {
	L_WARNING("skew angle not valid", mainName);
	exit(1);
    }
#endif

#if 0
    if (pixFindSkewSweepAndSearch(pixs, &angle, &conf, SWEEP_REDUCTION2,
                         SEARCH_REDUCTION, SWEEP_RANGE2, SWEEP_DELTA2,
			 SEARCH_MIN_DELTA)) {
	L_WARNING("skew angle not valid", mainName);
	exit(1);
    }
#endif

    pixDestroy(&pixs);
    exit(0);
}


