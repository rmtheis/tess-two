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
 *   pixaatest.c
 *
 *     Syntax:  pixaatest
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const l_int32  nx = 10;
static const l_int32  ny = 10;
static const l_int32  ncols = 3;

main(int    argc,
     char **argv)
{
l_int32      w, d, tilewidth;
PIX         *pixs;
PIXA        *pixa, *pixad1, *pixad2;
PIXAA       *pixaa1, *pixaa2;
static char  mainName[] = "pixaatest";

    if (argc != 1)
        exit(ERROR_INT(" Syntax: pixaatest", mainName, 1));

        /* Read in file; generate tiled pixaa; write pixaa to file */
    pixs = pixRead("test24.jpg");
    pixGetDimensions(pixs, &w, NULL, &d);
    tilewidth = w / nx;
    pixa = pixaSplitPix(pixs, nx, ny, 0, 0);
    pixaa1 = pixaaCreateFromPixa(pixa, nx, L_CHOOSE_CONSECUTIVE, L_CLONE);
    pixaa2 = pixaaCreateFromPixa(pixa, nx, L_CHOOSE_SKIP_BY, L_CLONE);
    pixaaWrite("/tmp/junkpixaa1", pixaa1);
    pixaaWrite("/tmp/junkpixaa2", pixaa2);
    pixaDestroy(&pixa);
    pixaaDestroy(&pixaa1);
    pixaaDestroy(&pixaa2);

        /* Read pixaa from file; tile/scale; write result; convert to PS */
    pixaa1 = pixaaRead("/tmp/junkpixaa1");
    pixaa2 = pixaaRead("/tmp/junkpixaa2");
    pixad1 = pixaaDisplayTiledAndScaled(pixaa1, d, tilewidth, ncols, 0, 10, 0);
    pixad2 = pixaaDisplayTiledAndScaled(pixaa2, d, tilewidth, ncols, 0, 10, 0);
    pixaWriteFiles("/tmp/junksplit1.", pixad1, IFF_JFIF_JPEG);
    pixaWriteFiles("/tmp/junksplit2.", pixad2, IFF_JFIF_JPEG);
    convertFilesToPS("/tmp", "junksplit1", 40, "/tmp/junkout1.ps");
    convertFilesToPS("/tmp", "junksplit2", 40, "/tmp/junkout2.ps");
    pixDestroy(&pixs);
    pixaaDestroy(&pixaa1);
    pixaaDestroy(&pixaa2);
    pixaDestroy(&pixad1);
    pixaDestroy(&pixad2);
    return 0;
}

