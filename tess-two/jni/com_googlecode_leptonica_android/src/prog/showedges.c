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
 * showedges.c
 *
 *    Uses computation of half edge function, along with thresholding.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define   SMOOTH_WIDTH_1       2  /* must be smaller */
#define   SMOOTH_WIDTH_2       4  /* must be larger */
#define   THRESHOLD            5  /* low works best */


main(int    argc,
     char **argv)
{
char        *infile, *outfile;
l_int32      d;
PIX         *pixs, *pixgr, *pixb;
static char  mainName[] = "showedges";

    if (argc != 3)
	exit(ERROR_INT(" Syntax: showedges infile outfile", mainName, 1));

    infile = argv[1];
    outfile = argv[2];

    pixs = pixRead(infile);
    d = pixGetDepth(pixs);
    if (d != 8 && d != 32) 
	exit(ERROR_INT("d not 8 or 32 bpp", mainName, 1));

    pixgr = pixHalfEdgeByBandpass(pixs, SMOOTH_WIDTH_1, SMOOTH_WIDTH_1,
                                        SMOOTH_WIDTH_2, SMOOTH_WIDTH_2);
    pixb = pixThresholdToBinary(pixgr, THRESHOLD);
    pixInvert(pixb, pixb);
/*    pixWrite("junkpixgr", pixgr, IFF_JFIF_JPEG); */
    pixWrite(outfile, pixb, IFF_PNG);

    exit(0);
}

