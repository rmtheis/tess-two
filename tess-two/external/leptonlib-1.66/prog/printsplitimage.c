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
 * printsplitimage.c
 *
 *   Syntax:  printsplitimage filein nx ny [printer]
 *
 *        nx = number of horizontal tiles
 *        ny = number of vertical tiles
 *
 *   If printer is not specified, the only action is that the
 *   image is split into a set of tiles, and these are written
 *   out as a set of uncompressed (i.e., very large) level 1
 *   PostScript files.  The images in the PostScript files are
 *   scaled to each fill an 8.5 x 11 inch page, up to the
 *   FILLING_FACTOR fraction in each direction.
 *
 *   If printer is specified, these are printed on separate pages.
 *   We do this (separate, uncompressed PostScript pages) because
 *   this is the lowest common denominator: many PostScript printers
 *   will not print multi-page PostScript of images, or images that
 *   are level 2 compressed.  Hard to believe, but true.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

    /* fill factor on 8.5 x 11 inch output page */
static const l_float32   FILL_FACTOR = 0.95;


main(int    argc,
     char **argv)
{
char        *filein, *name, *printer;
char         buffer[512];
l_int32      nx, ny, i, w, h, ws, hs, n;
l_float32    scale;
FILE        *fp;
PIX         *pixs, *pixt, *pixr;
PIXA        *pixa;
SARRAY      *sa;
static char  mainName[] = "printsplitimage";

    if (argc != 4 && argc != 5)
	exit(ERROR_INT(" Syntax:  printsplitimage filein nx ny [printer]",
                       mainName, 1));

    filein = argv[1];
    nx = atoi(argv[2]);
    ny = atoi(argv[3]);
    if (argc == 5)
	printer = argv[4];

    system("rm -f /tmp/junk_print_image_*.ps");

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
    ws = pixGetWidth(pixs);
    hs = pixGetHeight(pixs);
    if (ny * ws > nx * hs) {
        pixr = pixRotate90(pixs, 1);
        pixa = pixaSplitPix(pixr, ny, nx, 0, 0);
    }
    else {
        pixr = pixClone(pixs);
        pixa = pixaSplitPix(pixr, nx, ny, 0, 0);
    }

    n = pixaGetCount(pixa);
    sa = sarrayCreate(n);
    for (i = 0; i < n; i++) {
      pixt = pixaGetPix(pixa, i, L_CLONE);
      w = pixGetWidth(pixt);
      h = pixGetHeight(pixt);
      scale = L_MIN(FILL_FACTOR * 2550 / w, FILL_FACTOR * 3300 / h);
      sprintf(buffer, "/tmp/junk_print_image_%d.ps", i);
      fp = fopen(buffer, "wb+");
      sarrayAddString(sa, buffer, 1);
      pixWriteStreamPS(fp, pixt, NULL, 300, scale);
      fclose(fp);
      pixDestroy(&pixt);
    }

    if (argc == 5) {
        for (i = 0; i < n; i++) {
            name = sarrayGetString(sa, i, 0);
            sprintf(buffer, "lpr -P%s %s &", printer, name);
            system(buffer);
        }
    }

    sarrayDestroy(&sa);
    pixaDestroy(&pixa);
    pixDestroy(&pixr);
    pixDestroy(&pixs);
    exit(0);
}

