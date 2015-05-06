/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -
 -  Redistribution and use in source and binary forms, with or without
 -  modification, are permitted provided that the following conditions
 -  are met:
 -  1. Redistributions of source code must retain the above copyright
 -     notice, this list of conditions and the following disclaimer.
 -  2. Redistributions in binary form must reproduce the above
 -     copyright notice, this list of conditions and the following
 -     disclaimer in the documentation and/or other materials
 -     provided with the distribution.
 -
 -  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 -  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 -  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 -  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 -  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 -  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 -  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 -  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 -  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 -  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 -  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================*/

/*
 * findpattern1.c
 *
 *    findpattern1 filein patternfile fileout
 *
 *    This is setup with input parameters to generate a hit-miss
 *    Sel from the instance char.tif of a "c" bitmap, from
 *    the page image feyn.tif, scanned at 300 ppi:
 *
 *       findpattern1 feyn.tif char.tif junkcharout
 *
 *    It shows a number of different outputs, including a magnified
 *    image of the Sel superimposed on the "c" bitmap.
 */

#include "allheaders.h"

    /* for pixGenerateSelWithRuns() */
static const l_int32  NumHorLines = 11;
static const l_int32  NumVertLines = 8;
static const l_int32  MinRunlength = 1;

    /* for pixDisplayHitMissSel() */
static const l_uint32  HitColor = 0xff880000;
static const l_uint32  MissColor = 0x00ff8800;


int main(int    argc,
         char **argv)
{
char        *filein, *fileout, *patternfile;
l_int32      w, h, i, n;
BOX         *box, *boxe;
BOXA        *boxa1, *boxa2;
PIX         *pixs, *pixp, *pixpe;
PIX         *pixd, *pixt1, *pixt2, *pixhmt;
SEL         *sel_2h, *sel;
static char  mainName[] = "findpattern1";

    if (argc != 4)
        return ERROR_INT(" Syntax:  findpattern1 filein patternfile fileout",
                         mainName, 1);

    filein = argv[1];
    patternfile = argv[2];
    fileout = argv[3];

    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);
    if ((pixp = pixRead(patternfile)) == NULL)
        return ERROR_INT("pixp not made", mainName, 1);
    pixGetDimensions(pixp, &w, &h, NULL);

        /* generate the hit-miss Sel with runs */
    sel = pixGenerateSelWithRuns(pixp, NumHorLines, NumVertLines, 0,
                                MinRunlength, 7, 7, 0, 0, &pixpe);

        /* display the Sel two ways */
    selWriteStream(stderr, sel);
    pixt1 = pixDisplayHitMissSel(pixpe, sel, 9, HitColor, MissColor);
    pixDisplay(pixt1, 200, 200);
    pixWrite("/tmp/junkpixt", pixt1, IFF_PNG);

        /* use the Sel to find all instances in the page */
    startTimer();
    pixhmt = pixHMT(NULL, pixs, sel);
    fprintf(stderr, "Time to find patterns = %7.3f\n", stopTimer());

        /* small erosion to remove noise; typically not necessary if
         * there are enough elements in the Sel */
    sel_2h = selCreateBrick(1, 2, 0, 0, SEL_HIT);
    pixt2 = pixErode(NULL, pixhmt, sel_2h);

        /* display the result visually by placing the Sel at each
         * location found */
    pixd = pixDilate(NULL, pixt2, sel);
    pixWrite(fileout, pixd, IFF_TIFF_G4);

        /* display outut with an outline around each located pattern */
    boxa1 = pixConnCompBB(pixt2, 8);
    n = boxaGetCount(boxa1);
    boxa2 = boxaCreate(n);
    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa1, i, L_COPY);
        boxe = boxCreate(box->x - w / 2, box->y - h / 2, w + 4, h + 4);
        boxaAddBox(boxa2, boxe, L_INSERT);
        pixRenderBox(pixs, boxe, 4, L_FLIP_PIXELS);
        boxDestroy(&box);
    }
    pixWrite("/tmp/junkoutline", pixs, IFF_TIFF_G4);
    boxaWriteStream(stderr, boxa2);

    pixDestroy(&pixs);
    pixDestroy(&pixp);
    pixDestroy(&pixpe);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixhmt);
    pixDestroy(&pixd);
    selDestroy(&sel);
    selDestroy(&sel_2h);
    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
    return 0;
}

