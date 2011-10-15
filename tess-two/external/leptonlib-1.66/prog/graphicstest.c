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
 * graphicstest.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
char       *filein, *fileout;
l_int32     d;
BOX        *box1, *box2, *box3, *box4;
BOXA       *boxa;
PIX        *pixs, *pixt1, *pixt2, *pixt3;
PTA        *pta;
static char     mainName[] = "graphicstest";

    if (argc != 3)
        exit(ERROR_INT(" Syntax: graphicstest filein fileout", mainName, 1));

    filein = argv[1];
    fileout = argv[2];
    if ((pixs = pixRead(filein)) == NULL)
        exit(ERROR_INT(" Syntax: pixs not made", mainName, 1));
    d = pixGetDepth(pixs);
    if (d <= 8)
        pixt1 = pixConvertTo32(pixs);
    else
        pixt1 = pixClone(pixs);

        /* Paint on RGB */
    pixRenderLineArb(pixt1, 450, 20, 850, 320, 5, 200, 50, 125);
    pixRenderLineArb(pixt1, 30, 40, 440, 40, 5, 100, 200, 25);
    pixRenderLineBlend(pixt1, 30, 60, 440, 70, 5, 115, 200, 120, 0.3);
    pixRenderLineBlend(pixt1, 30, 600, 440, 670, 9, 215, 115, 30, 0.5);
    pixRenderLineBlend(pixt1, 130, 700, 540, 770, 9, 255, 255, 250, 0.4);
    pixRenderLineBlend(pixt1, 130, 800, 540, 870, 9, 0, 0, 0, 0.4);
    box1 = boxCreate(70, 80, 300, 245);
    box2 = boxCreate(470, 180, 150, 205);
    box3 = boxCreate(520, 220, 160, 220);
    box4 = boxCreate(570, 260, 160, 220);
    boxa = boxaCreate(3);
    boxaAddBox(boxa, box2, L_INSERT);
    boxaAddBox(boxa, box3, L_INSERT);
    boxaAddBox(boxa, box4, L_INSERT);
    pixRenderBoxArb(pixt1, box1, 3, 200, 200, 25);
    pixRenderBoxaBlend(pixt1, boxa, 17, 200, 200, 25, 0.4, 1);
    pta = ptaCreate(5);
    ptaAddPt(pta, 250, 300);
    ptaAddPt(pta, 350, 450);
    ptaAddPt(pta, 400, 600);
    ptaAddPt(pta, 212, 512);
    ptaAddPt(pta, 180, 375);
    pixRenderPolylineBlend(pixt1, pta, 17, 25, 200, 200, 0.5, 1, 1);
    pixWrite(fileout, pixt1, IFF_JFIF_JPEG);
    pixDisplay(pixt1, 200, 200);

    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    boxDestroy(&box1);
    boxaDestroy(&boxa);
    ptaDestroy(&pta);
    pixDestroy(&pixs);
}


