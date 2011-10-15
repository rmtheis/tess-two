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
 * splitimage2pdf.c
 *
 *   Syntax:  splitimage2pdf filein nx ny fileout
 *
 *        nx = number of horizontal tiles
 *        ny = number of vertical tiles
 *
 *   Generates pdf of image tiles.  Rotates the image before
 *   tiling if the tiles otherwise will have larger width than
 *   height.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

    /* fill factor on 8.5 x 11 inch output page */
static const l_float32   FILL_FACTOR = 0.95;


main(int    argc,
     char **argv)
{
char        *filein, *fileout;
char         buffer[512];
const char  *psfile = "/tmp/junk_split_image.ps";
l_int32      nx, ny, i, w, h, d, ws, hs, n, res;
l_float32    scale;
PIX         *pixs, *pixt, *pixr;
PIXA        *pixa;
static char  mainName[] = "splitimage2pdf";

    if (argc != 5)
	exit(ERROR_INT(" Syntax:  splitimage2pdf filein nx ny fileout",
                       mainName, 1));

    filein = argv[1];
    nx = atoi(argv[2]);
    ny = atoi(argv[3]);
    fileout = argv[4];

    system("rm -f /tmp/junk_split_image.ps");

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
    d = pixGetDepth(pixs);
    if (d == 1 )
	system("rm -f /tmp/junk_split_image.tif");
    else if (d == 8 || d == 32)
	system("rm -f /tmp/junk_split_image.jpg");
    else
	exit(ERROR_INT("d not in {1,8,32} bpp", mainName, 1));

    ws = pixGetWidth(pixs);
    hs = pixGetHeight(pixs);
    if (ny * ws > nx * hs)
        pixr = pixRotate90(pixs, 1);
    else
        pixr = pixClone(pixs);

    pixa = pixaSplitPix(pixr, nx, ny, 0, 0);
    n = pixaGetCount(pixa);
    res = 300;
    for (i = 0; i < n; i++) {
      pixt = pixaGetPix(pixa, i, L_CLONE);
      w = pixGetWidth(pixt);
      h = pixGetHeight(pixt);
      scale = L_MIN(FILL_FACTOR * 2550 / w, FILL_FACTOR * 3300 / h);
      if (d == 1) {
	  pixWrite("/tmp/junk_split_image.tif", pixt, IFF_TIFF_G4);
          if (i == 0)
	      convertTiffG4ToPS("/tmp/junk_split_image.tif", psfile,
			  "w", 0, 0, 300, scale, 1, FALSE, TRUE);
          else
	      convertTiffG4ToPS("/tmp/junk_split_image.tif", psfile,
			  "a", 0, 0, 300, scale, 1, FALSE, TRUE);
      }
      else {
	  pixWrite("/tmp/junk_split_image.jpg", pixt, IFF_JFIF_JPEG);
          if (i == 0)
	      convertJpegToPS("/tmp/junk_split_image.jpg", psfile,
			  "w", 0, 0, 300, scale, 1, TRUE);
          else
	      convertJpegToPS("/tmp/junk_split_image.jpg", psfile,
			  "a", 0, 0, 300, scale, 1, TRUE);
      }
      pixDestroy(&pixt);
    }

    sprintf(buffer, "ps2pdf %s %s", psfile, fileout);
    system(buffer);

    pixaDestroy(&pixa);
    pixDestroy(&pixr);
    pixDestroy(&pixs);
    exit(0);
}

