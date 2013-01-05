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
 * pagesegtest2.c
 *
 *   Demonstrates morphological approach to segmenting images.
 *
 *       pagesegtest2 filein fileout
 *   
 *   where:
 *       filein: 1, 8 or 32 bpp page image
 *       fileout: photomask for image regions at full resolution
 *
 *   This example shows how to use the morphseq specification of
 *   a sequence of morphological and reduction/expansion operations.
 *
 *   This is much simpler than generating the structuring elements
 *   for the morph operations, specifying each of the function
 *   calls, keeping track of the intermediate images, and removing
 *   them at the end.
 *
 *   The specific sequences below tend to work ok for images scanned at
 *   about 600 ppi.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

    /* Mask at 4x reduction */
static const char *mask_sequence = "r11";
    /* Seed at 4x reduction, formed by doing a 16x reduction,
     * an opening, and finally a 4x replicative expansion. */
static const char *seed_sequence = "r1143 + o5.5+ x4"; 
    /* Simple dilation */
static const char *dilation_sequence = "d3.3";

#define  DFLAG     1

main(int    argc,
     char **argv)
{
char        *filein, *fileout;
l_int32      thresh;
PIX         *pixs, *pixg, *pixb;
PIX         *pixmask4, *pixseed4, *pixsf4, *pixd4, *pixd;
static char  mainName[] = "pagesegtest2";

    if (argc != 4)
	exit(ERROR_INT(" Syntax:  pagesegtest2 filein thresh fileout",
                       mainName, 1));

    filein = argv[1];
    thresh = atoi(argv[2]);
    fileout = argv[3];

        /* Get a 1 bpp version of the page */
    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
    if (pixGetDepth(pixs) == 32)
        pixg = pixConvertRGBToGrayFast(pixs);
    else
        pixg = pixClone(pixs);
    if (pixGetDepth(pixg) == 8)
        pixb = pixThresholdToBinary(pixg, thresh);
    else
        pixb = pixClone(pixg);

        /* Make seed and mask, and fill seed into mask */
    pixseed4 = pixMorphSequence(pixb, seed_sequence, 0);
    pixmask4 = pixMorphSequence(pixb, mask_sequence, 0);
    pixsf4 = pixSeedfillBinary(NULL, pixseed4, pixmask4, 8);
    pixd4 = pixMorphSequence(pixsf4, dilation_sequence, 0);

        /* Mask at full resolution */
    pixd = pixExpandBinaryPower2(pixd4, 4);
    pixWrite(fileout, pixd, IFF_TIFF_G4);

        /* Extract non-image parts (e.g., text) at full resolution */
    pixSubtract(pixb, pixb, pixd);

    pixDisplayWithTitle(pixseed4, 400, 100, "halftone seed", DFLAG);
    pixDisplayWithTitle(pixmask4, 100, 100, "halftone seed mask", DFLAG);
    pixDisplayWithTitle(pixd4, 700, 100, "halftone mask", DFLAG);
    pixDisplayWithTitle(pixb, 1000, 100, "non-halftone", DFLAG);

#if 1
    pixWrite("junkseed", pixseed4, IFF_TIFF_G4);
    pixWrite("junkmask", pixmask4, IFF_TIFF_G4);
    pixWrite("junkfill", pixd4, IFF_TIFF_G4);
    pixWrite("junktext", pixb, IFF_TIFF_G4);
#endif

    pixDestroy(&pixs);
    pixDestroy(&pixg);
    pixDestroy(&pixb);
    pixDestroy(&pixseed4);
    pixDestroy(&pixmask4);
    pixDestroy(&pixsf4);
    pixDestroy(&pixd4);
    pixDestroy(&pixd);
    exit(0);
}


