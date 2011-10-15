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
 *  maze_reg.c
 *
 *    Tests the functions in maze.c: binary and gray maze search,
 *    largest rectangle in bg or fg.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

#define  NPATHS     6
static const l_int32 x0[NPATHS] = {42, 73, 73, 42, 324, 471};
static const l_int32 y0[NPATHS] = {117, 319, 319, 117, 170, 201};
static const l_int32 x1[NPATHS] = {419, 419, 233, 326, 418, 128};
static const l_int32 y1[NPATHS] = {383, 383, 112, 168, 371, 341};

static const l_int32  NBOXES = 20;
static const l_int32  POLARITY = 0;  /* background */

main(int    argc,
     char **argv)
{
l_int32      i, w, h, bx, by, bw, bh, index, rval, gval, bval, success, display;
BOX         *box;
BOXA        *boxa;
FILE        *fp;
PIX         *pixm, *pixs, *pixg, *pixt, *pixd;
PIXA        *pixa;
PIXCMAP     *cmap;
PTA         *pta;
PTAA        *ptaa;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &fp, &display, &success, &rp))
	return 1;
    pixa = pixaCreate(0);

    /* ---------------- Shortest path in binary maze ---------------- */
        /* Generate the maze */
    pixm = generateBinaryMaze(200, 200, 20, 20, 0.65, 0.25);
    pixd = pixExpandBinaryReplicate(pixm, 3);
    pixSaveTiledOutline(pixd, pixa, 1, 1, 20, 2, 32);
    pixDestroy(&pixd);

        /* Find the shortest path between two points */
    pta = pixSearchBinaryMaze(pixm, 20, 20, 170, 170, NULL);
    pixt = pixDisplayPta(NULL, pixm, pta);
    pixd = pixScaleBySampling(pixt, 3., 3.);
    pixSaveTiledOutline(pixd, pixa, 1, 0, 20, 2, 32);
    pixWrite("/tmp/pix0.png", pixd, IFF_PNG);
    ptaDestroy(&pta);
    pixDestroy(&pixt);
    pixDestroy(&pixd);
    pixDestroy(&pixm);


    /* ---------------- Shortest path in gray maze ---------------- */
    pixg = pixRead("test8.jpg");
    pixGetDimensions(pixg, &w, &h, NULL);
    ptaa = ptaaCreate(NPATHS);
    for (i = 0; i < NPATHS; i++) {
        if (x0[i] >= w || x1[i] >= w || y0[i] >= h || y1[i] >= h) {
            fprintf(stderr, "path %d extends beyond image; skipping\n", i);
            continue;
        }
        pta = pixSearchGrayMaze(pixg, x0[i], y0[i], x1[i], y1[i], NULL);
        ptaaAddPta(ptaa, pta, L_INSERT);
    }

    pixt = pixDisplayPtaa(pixg, ptaa);
    pixd = pixScaleBySampling(pixt, 2., 2.);
    pixSaveTiledOutline(pixd, pixa, 1, 1, 20, 2, 32);
    pixWrite("/tmp/pix1.jpg", pixd, IFF_PNG);
    ptaaDestroy(&ptaa);
    pixDestroy(&pixg);
    pixDestroy(&pixt);
    pixDestroy(&pixd);


    /* ---------------- Largest rectangles in image ---------------- */
    pixs = pixRead("test1.png");
    pixd = pixConvertTo8(pixs, FALSE);
    cmap = pixcmapCreateRandom(8, 1, 1);
    pixSetColormap(pixd, cmap);

    boxa = boxaCreate(0);
    for (i = 0; i < NBOXES; i++) {
        pixFindLargestRectangle(pixs, POLARITY, &box, NULL);
        boxGetGeometry(box, &bx, &by, &bw, &bh);
        pixSetInRect(pixs, box);
        fprintf(stderr, "bx = %5d, by = %5d, bw = %5d, bh = %5d, area = %d\n",
                bx, by, bw, bh, bw * bh);
        boxaAddBox(boxa, box, L_INSERT);
    }

    for (i = 0; i < NBOXES; i++) {
        index = 32 + (i & 254);
        pixcmapGetColor(cmap, index, &rval, &gval, &bval);
        box = boxaGetBox(boxa, i, L_CLONE);
        pixRenderHashBoxArb(pixd, box, 6, 2, L_NEG_SLOPE_LINE, 1,
                            rval, gval, bval);
        boxDestroy(&box);
    }
    pixSaveTiledOutline(pixd, pixa, 1, 1, 20, 2, 32);
    pixWrite("/tmp/pix2.png", pixd, IFF_PNG);
    pixDestroy(&pixs);
    pixDestroy(&pixd);
    boxaDestroy(&boxa);

    pixd = pixaDisplay(pixa, 0, 0);
    pixWrite("/tmp/pix3.png", pixd, IFF_PNG);
    pixDisplayWithTitle(pixd, 100, 100, NULL, display);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    regTestCheckFile(fp, argv, "/tmp/pix0.png", 0, &success);
    regTestCheckFile(fp, argv, "/tmp/pix1.jpg", 1, &success);
    regTestCheckFile(fp, argv, "/tmp/pix2.png", 2, &success);
    regTestCheckFile(fp, argv, "/tmp/pix3.png", 3, &success);
    regTestCleanup(argc, argv, fp, success, rp);
    return 0;
}

