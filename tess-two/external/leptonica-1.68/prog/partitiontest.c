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
 *   partitiontest.c
 *
 *      partitiontest <fname> type [maxboxes  ovlap]
 *
 *  where type is:
 *      5:   L_SORT_BY_WIDTH
 *      6:   L_SORT_BY_HEIGHT
 *      7:   L_SORT_BY_MIN_DIMENSION
 *      8:   L_SORT_BY_MAX_DIMENSION
 *      9:   L_SORT_BY_PERIMETER
 *      10:  L_SORT_BY_AREA
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define  REDUCTION     1


main(int    argc,
     char **argv)
{
char        *filename;
l_int32      w, h, type, maxboxes;
l_float32    ovlap;
BOX         *box;
BOXA        *boxa, *boxat, *boxad;
PIX         *pix, *pixt, *pixs, *pixd;
static char  mainName[] = "partitiontest";

    if (argc != 3 && argc != 5)
        return ERROR_INT("syntax: partitiontest <fname> type [maxboxes ovlap]",
                         mainName, 1);

    filename = argv[1];
    type = atoi(argv[2]);
    if (type == L_SORT_BY_WIDTH)
        fprintf(stderr, "Sorting by width:\n");
    else if (type == L_SORT_BY_HEIGHT)
        fprintf(stderr, "Sorting by height:\n");
    else if (type == L_SORT_BY_MAX_DIMENSION)
        fprintf(stderr, "Sorting by maximum dimension:\n");
    else if (type == L_SORT_BY_MIN_DIMENSION)
        fprintf(stderr, "Sorting by minimum dimension:\n");
    else if (type == L_SORT_BY_PERIMETER)
        fprintf(stderr, "Sorting by perimeter:\n");
    else if (type == L_SORT_BY_AREA)
        fprintf(stderr, "Sorting by area:\n");
    else {
        fprintf(stderr, "Use one of the following for 'type':\n"
               "     5:   L_SORT_BY_WIDTH\n"
               "     6:   L_SORT_BY_HEIGHT\n"
               "     7:   L_SORT_BY_MIN_DIMENSION\n"
               "     8:   L_SORT_BY_MAX_DIMENSION\n"
               "     9:   L_SORT_BY_PERIMETER\n"
               "    10:   L_SORT_BY_AREA\n");
        return ERROR_INT("invalid type: see source", mainName, 1);
    }
    if (argc == 5) {
        maxboxes = atoi(argv[3]);
	ovlap = atof(argv[4]);
    } else {
        maxboxes = 100;
	ovlap = 0.2;
    }
        

    pix = pixRead(filename);
    pixs = pixConvertTo1(pix, 128);
    pixDilateBrick(pixs, pixs, 5, 5);
    boxa = pixConnComp(pixs, NULL, 4);
    pixGetDimensions(pixs, &w, &h, NULL);
    box = boxCreate(0, 0, w, h);
    startTimer();
    boxaPermuteRandom(boxa, boxa);
    boxat = boxaSelectBySize(boxa, 500, 500, L_SELECT_IF_BOTH,
                             L_SELECT_IF_LT, NULL);
    boxad = boxaGetWhiteblocks(boxat, box, type, maxboxes, ovlap,
                               200, 0.15, 20000);
    fprintf(stderr, "Time: %7.3f sec\n", stopTimer());
    boxaWriteStream(stderr, boxad);

    pixDisplayWrite(NULL, -1);
    pixDisplayWrite(pixs, REDUCTION);

        /* Display box outlines in a single color in a cmapped image */
    pixd = pixDrawBoxa(pixs, boxad, 7, 0xe0708000);
    pixDisplayWrite(pixd, REDUCTION);
    pixDestroy(&pixd);

        /* Display box outlines in a single color in an RGB image */
    pixt = pixConvertTo8(pixs, FALSE);
    pixd = pixDrawBoxa(pixt, boxad, 7, 0x40a0c000);
    pixDisplayWrite(pixd, REDUCTION);
    pixDestroy(&pixt);
    pixDestroy(&pixd);

        /* Display box outlines with random colors in a cmapped image */
    pixd = pixDrawBoxaRandom(pixs, boxad, 7);
    pixDisplayWrite(pixd, REDUCTION);
    pixDestroy(&pixd);

        /* Display box outlines with random colors in an RGB image */
    pixt = pixConvertTo8(pixs, FALSE);
    pixd = pixDrawBoxaRandom(pixt, boxad, 7);
    pixDisplayWrite(pixd, REDUCTION);
    pixDestroy(&pixt);
    pixDestroy(&pixd);

        /* Display boxes in the same color in a cmapped image */
    pixd = pixPaintBoxa(pixs, boxad, 0x60e0a000);
    pixDisplayWrite(pixd, REDUCTION);
    pixDestroy(&pixd);

        /* Display boxes in the same color in an RGB image */
    pixt = pixConvertTo8(pixs, FALSE);
    pixd = pixPaintBoxa(pixt, boxad, 0xc030a000);
    pixDisplayWrite(pixd, REDUCTION);
    pixDestroy(&pixt);
    pixDestroy(&pixd);

        /* Display boxes in random colors in a cmapped image */
    pixd = pixPaintBoxaRandom(pixs, boxad);
    pixDisplayWrite(pixd, REDUCTION);
    pixDestroy(&pixd);

        /* Display boxes in random colors in an RGB image */
    pixt = pixConvertTo8(pixs, FALSE);
    pixd = pixPaintBoxaRandom(pixt, boxad);
    pixDisplayWrite(pixd, REDUCTION);
    pixDestroy(&pixt);
    pixDestroy(&pixd);

    pixDisplayMultiple("/tmp/junk_write_display*");

    pixDestroy(&pix);
    pixDestroy(&pixs);
    boxDestroy(&box);
    boxaDestroy(&boxa);
    boxaDestroy(&boxat);
    boxaDestroy(&boxad);
    return 0;
}


