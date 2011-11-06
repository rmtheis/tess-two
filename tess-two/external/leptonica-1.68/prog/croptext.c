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
 *  croptext.c
 *
 *     Simple program that crops text pages to a given border
 *
 *     Syntax:
 *         croptext dirin border dirout
 *     where
 *         border = number of pixels added on each side (e.g., 50)
 *
 *     The output file name has the same tail as the input file name.
 *     If dirout is the same as dirin, you overwrite the input files.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
char **argv)
{
char        *dirin, *dirout, *infile, *outfile, *tail;
l_int32      i, nfiles, border, x, y, w, h, xb, yb, wb, hb;
BOX         *box1, *box2;
BOXA        *boxa1, *boxa2;
PIX         *pixs, *pixt1, *pixd;
SARRAY      *safiles;
static char  mainName[] = "croptext";

    if (argc != 4)
        exit(ERROR_INT("Syntax: croptext dirin border dirout", mainName, 1)); 

    dirin = argv[1];
    border = atoi(argv[2]);
    dirout = argv[3];

    safiles = getSortedPathnamesInDirectory(dirin, NULL, 0, 0);
    nfiles = sarrayGetCount(safiles);

    for (i = 0; i < nfiles; i++) {
        infile = sarrayGetString(safiles, i, 0);
	splitPathAtDirectory(infile, NULL, &tail);
	outfile = genPathname(dirout, tail);
        pixs = pixRead(infile);
        pixt1 = pixMorphSequence(pixs, "r11 + c10.40 + o5.5 + x4", 0);
        boxa1 = pixConnComp(pixt1, NULL, 8);
        if (boxaGetCount(boxa1) == 0) {
            fprintf(stderr, "Warning: no components on page %s\n", tail);
            continue;
        }
        boxa2 = boxaSort(boxa1, L_SORT_BY_AREA, L_SORT_DECREASING, NULL);
        box1 = boxaGetBox(boxa2, 0, L_CLONE);
        boxGetGeometry(box1, &x, &y, &w, &h);
        xb = L_MAX(0, x - border);
        yb = L_MAX(0, y - border);
        wb = w + 2 * border;
        hb = h + 2 * border;
        box2 = boxCreate(xb, yb, wb, hb);
        pixd = pixClipRectangle(pixs, box2, NULL);
        pixWrite(outfile, pixd, IFF_TIFF_G4);

	pixDestroy(&pixs);
	pixDestroy(&pixt1);
	pixDestroy(&pixd);
	boxaDestroy(&boxa1);
	boxaDestroy(&boxa2);
    }

    return 0;
}

