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
 * dwamorph2_reg.c
 *
 *     Compare the timings of various binary morphological implementations.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define  HALFWIDTH   3

    /* Complete set of linear brick dwa operations */
PIX *pixMorphDwa_3(PIX *pixd, PIX *pixs, l_int32 operation, char *selname);

static const l_int32  NTIMES = 20;

main(int    argc,
     char **argv)
{
char        *selname;
l_int32      i, j, nsels, sx, sy;
l_float32    fact, time;
GPLOT       *gplot;
NUMA        *na1, *na2, *na3, *na4, *nac1, *nac2, *nac3, *nac4, *nax;
PIX         *pixs, *pixt;
PIXA        *pixa;
SEL         *sel;
SELA        *selalinear;
static char  mainName[] = "dwamorph2_reg";

    if (argc != 1)
	exit(ERROR_INT(" Syntax: dwamorph2_reg", mainName, 1));

    if ((pixs = pixRead("feyn-fract.tif")) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));
    pixt = pixCreateTemplate(pixs);

    selalinear = selaAddDwaLinear(NULL);
    nsels = selaGetCount(selalinear);

    fact = 1000. / (l_float32)NTIMES;  /* converts to time in msec */
    na1 = numaCreate(64);
    na2 = numaCreate(64);
    na3 = numaCreate(64);
    na4 = numaCreate(64);

        /*  ---------  dilation  ----------*/

    for (i = 0; i < nsels / 2; i++)
    {
        sel = selaGetSel(selalinear, i);
        selGetParameters(sel, &sy, &sx, NULL, NULL);
        selname = selGetName(sel);
	fprintf(stderr, " %d .", i);

        startTimer();
        for (j = 0; j < NTIMES; j++)
            pixDilate(pixt, pixs, sel);
        time = fact * stopTimer();
        numaAddNumber(na1, time);

        startTimer();
        for (j = 0; j < NTIMES; j++)
            pixDilateCompBrick(pixt, pixs, sx, sy);
        time = fact * stopTimer();
        numaAddNumber(na2, time);

        startTimer();
        for (j = 0; j < NTIMES; j++)
            pixMorphDwa_3(pixt, pixs, L_MORPH_DILATE, selname);
        time = fact * stopTimer();
        numaAddNumber(na3, time);

        startTimer();
        for (j = 0; j < NTIMES; j++)
            pixDilateCompBrickDwa(pixt, pixs, sx, sy);
        time = fact * stopTimer();
        numaAddNumber(na4, time);
    }

    nax = numaMakeSequence(2, 1, nsels / 2);
    nac1 = numaConvolve(na1, HALFWIDTH);
    nac2 = numaConvolve(na2, HALFWIDTH);
    nac3 = numaConvolve(na3, HALFWIDTH);
    nac4 = numaConvolve(na4, HALFWIDTH);
    gplot = gplotCreate("/tmp/junkdilate", GPLOT_PNG,
                        "Dilation time vs sel size", "size", "time (ms)");
    gplotAddPlot(gplot, nax, nac1, GPLOT_LINES, "linear rasterop");
    gplotAddPlot(gplot, nax, nac2, GPLOT_LINES, "composite rasterop");
    gplotAddPlot(gplot, nax, nac3, GPLOT_LINES, "linear dwa");
    gplotAddPlot(gplot, nax, nac4, GPLOT_LINES, "composite dwa");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&nac1);
    numaDestroy(&nac2);
    numaDestroy(&nac3);
    numaDestroy(&nac4);

        /*  ---------  erosion  ----------*/

    numaEmpty(na1);
    numaEmpty(na2);
    numaEmpty(na3);
    numaEmpty(na4);
    for (i = 0; i < nsels / 2; i++)
    {
        sel = selaGetSel(selalinear, i);
        selGetParameters(sel, &sy, &sx, NULL, NULL);
        selname = selGetName(sel);
	fprintf(stderr, " %d .", i);

        startTimer();
        for (j = 0; j < NTIMES; j++)
            pixErode(pixt, pixs, sel);
        time = fact * stopTimer();
        numaAddNumber(na1, time);

        startTimer();
        for (j = 0; j < NTIMES; j++)
            pixErodeCompBrick(pixt, pixs, sx, sy);
        time = fact * stopTimer();
        numaAddNumber(na2, time);

        startTimer();
        for (j = 0; j < NTIMES; j++)
            pixMorphDwa_3(pixt, pixs, L_MORPH_ERODE, selname);
        time = fact * stopTimer();
        numaAddNumber(na3, time);

        startTimer();
        for (j = 0; j < NTIMES; j++)
            pixErodeCompBrickDwa(pixt, pixs, sx, sy);
        time = fact * stopTimer();
        numaAddNumber(na4, time);
    }

    nac1 = numaConvolve(na1, HALFWIDTH);
    nac2 = numaConvolve(na2, HALFWIDTH);
    nac3 = numaConvolve(na3, HALFWIDTH);
    nac4 = numaConvolve(na4, HALFWIDTH);
    gplot = gplotCreate("/tmp/junkerode", GPLOT_PNG,
                        "Erosion time vs sel size", "size", "time (ms)");
    gplotAddPlot(gplot, nax, nac1, GPLOT_LINES, "linear rasterop");
    gplotAddPlot(gplot, nax, nac2, GPLOT_LINES, "composite rasterop");
    gplotAddPlot(gplot, nax, nac3, GPLOT_LINES, "linear dwa");
    gplotAddPlot(gplot, nax, nac4, GPLOT_LINES, "composite dwa");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&nac1);
    numaDestroy(&nac2);
    numaDestroy(&nac3);
    numaDestroy(&nac4);

        /*  ---------  opening  ----------*/

    numaEmpty(na1);
    numaEmpty(na2);
    numaEmpty(na3);
    numaEmpty(na4);
    for (i = 0; i < nsels / 2; i++)
    {
        sel = selaGetSel(selalinear, i);
        selGetParameters(sel, &sy, &sx, NULL, NULL);
        selname = selGetName(sel);
	fprintf(stderr, " %d .", i);

        startTimer();
        for (j = 0; j < NTIMES; j++)
            pixOpen(pixt, pixs, sel);
        time = fact * stopTimer();
        numaAddNumber(na1, time);

        startTimer();
        for (j = 0; j < NTIMES; j++)
            pixOpenCompBrick(pixt, pixs, sx, sy);
        time = fact * stopTimer();
        numaAddNumber(na2, time);

        startTimer();
        for (j = 0; j < NTIMES; j++)
            pixMorphDwa_3(pixt, pixs, L_MORPH_OPEN, selname);
        time = fact * stopTimer();
        numaAddNumber(na3, time);

        startTimer();
        for (j = 0; j < NTIMES; j++)
            pixOpenCompBrickDwa(pixt, pixs, sx, sy);
        time = fact * stopTimer();
        numaAddNumber(na4, time);
    }

    nac1 = numaConvolve(na1, HALFWIDTH);
    nac2 = numaConvolve(na2, HALFWIDTH);
    nac3 = numaConvolve(na3, HALFWIDTH);
    nac4 = numaConvolve(na4, HALFWIDTH);
    gplot = gplotCreate("/tmp/junkopen", GPLOT_PNG,
                        "Opening time vs sel size", "size", "time (ms)");
    gplotAddPlot(gplot, nax, nac1, GPLOT_LINES, "linear rasterop");
    gplotAddPlot(gplot, nax, nac2, GPLOT_LINES, "composite rasterop");
    gplotAddPlot(gplot, nax, nac3, GPLOT_LINES, "linear dwa");
    gplotAddPlot(gplot, nax, nac4, GPLOT_LINES, "composite dwa");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&nac1);
    numaDestroy(&nac2);
    numaDestroy(&nac3);
    numaDestroy(&nac4);

        /*  ---------  closing  ----------*/

    numaEmpty(na1);
    numaEmpty(na2);
    numaEmpty(na3);
    numaEmpty(na4);
    for (i = 0; i < nsels / 2; i++)
    {
        sel = selaGetSel(selalinear, i);
        selGetParameters(sel, &sy, &sx, NULL, NULL);
        selname = selGetName(sel);
	fprintf(stderr, " %d .", i);

        startTimer();
        for (j = 0; j < NTIMES; j++)
            pixClose(pixt, pixs, sel);
        time = fact * stopTimer();
        numaAddNumber(na1, time);

        startTimer();
        for (j = 0; j < NTIMES; j++)
            pixCloseCompBrick(pixt, pixs, sx, sy);
        time = fact * stopTimer();
        numaAddNumber(na2, time);

        startTimer();
        for (j = 0; j < NTIMES; j++)
            pixMorphDwa_3(pixt, pixs, L_MORPH_CLOSE, selname);
        time = fact * stopTimer();
        numaAddNumber(na3, time);

        startTimer();
        for (j = 0; j < NTIMES; j++)
            pixCloseCompBrickDwa(pixt, pixs, sx, sy);
        time = fact * stopTimer();
        numaAddNumber(na4, time);
    }

    nac1 = numaConvolve(na1, HALFWIDTH);
    nac2 = numaConvolve(na2, HALFWIDTH);
    nac3 = numaConvolve(na3, HALFWIDTH);
    nac4 = numaConvolve(na4, HALFWIDTH);
    gplot = gplotCreate("/tmp/junkclose", GPLOT_PNG,
                        "Closing time vs sel size", "size", "time (ms)");
    gplotAddPlot(gplot, nax, nac1, GPLOT_LINES, "linear rasterop");
    gplotAddPlot(gplot, nax, nac2, GPLOT_LINES, "composite rasterop");
    gplotAddPlot(gplot, nax, nac3, GPLOT_LINES, "linear dwa");
    gplotAddPlot(gplot, nax, nac4, GPLOT_LINES, "composite dwa");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&nac1);
    numaDestroy(&nac2);
    numaDestroy(&nac3);
    numaDestroy(&nac4);


    numaDestroy(&na1);
    numaDestroy(&na2);
    numaDestroy(&na3);
    numaDestroy(&na4);
    numaDestroy(&nax);
    selaDestroy(&selalinear);
    pixDestroy(&pixt);
    pixDestroy(&pixs);

        /* Display the results together */
    pixa = pixaCreate(0);
    pixs = pixRead("/tmp/junkdilate.png");
    pixaAddPix(pixa, pixs, L_INSERT);
    pixs = pixRead("/tmp/junkerode.png");
    pixaAddPix(pixa, pixs, L_INSERT);
    pixs = pixRead("/tmp/junkopen.png");
    pixaAddPix(pixa, pixs, L_INSERT);
    pixs = pixRead("/tmp/junkclose.png");
    pixaAddPix(pixa, pixs, L_INSERT);
    pixt = pixaDisplayTiledInRows(pixa, 32, 1500, 1.0, 0, 40, 3);
    pixWrite("/tmp/junktimings.png", pixt, IFF_PNG);
    pixDisplay(pixt, 100, 100);
    pixDestroy(&pixt);
    pixaDestroy(&pixa);
    return 0;
}

