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
 * rotatetest1.c
 *
 *    rotatetest1 filein angle(in degrees) fileout
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define  NTIMES   180
#define  NITERS   3


main(int    argc,
     char **argv)
{
l_int32      i, w, h, d, rotflag;
PIX         *pixs, *pixt, *pixd;
l_float32    angle, deg2rad, pops, ang;
char        *filein, *fileout;
static char  mainName[] = "rotatetest1";

    if (argc != 4)
	exit(ERROR_INT(" Syntax:  rotatetest1 filein angle fileout",
                       mainName, 1));

    filein = argv[1];
    angle = atof(argv[2]);
    fileout = argv[3];
    deg2rad = 3.1415926535 / 180.;

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));
    if (pixGetDepth(pixs) == 1) {
        pixt = pixScaleToGray3(pixs);
        pixDestroy(&pixs);
        pixs = pixAddBorderGeneral(pixt, 1, 0, 1, 0, 255);
        pixDestroy(&pixt);
    }

    pixGetDimensions(pixs, &w, &h, &d);
    fprintf(stderr, "w = %d, h = %d\n", w, h);

#if 0
        /* repertory of rotation operations to choose from */
    pixd = pixRotateAM(pixs, deg2rad * angle, L_BRING_IN_WHITE);
    pixd = pixRotateAMColor(pixs, deg2rad * angle, 0xffffff00);
    pixd = pixRotateAMColorFast(pixs, deg2rad * angle, 255);
    pixd = pixRotateAMCorner(pixs, deg2rad * angle, L_BRING_IN_WHITE);
    pixd = pixRotateShear(pixs, w /2, h / 2, deg2rad * angle,
                          L_BRING_IN_WHITE);
    pixd = pixRotate3Shear(pixs, w /2, h / 2, deg2rad * angle,
                           L_BRING_IN_WHITE);
    pixRotateShearIP(pixs, w / 2, h / 2, deg2rad * angle); pixd = pixs;
#endif

#if 0
        /* timing of shear rotation */
    for (i = 0; i < NITERS; i++) {
	pixd = pixRotateShear(pixs, (i * w) / NITERS,
	                      (i * h) / NITERS, deg2rad * angle,
			      L_BRING_IN_WHITE);
	pixDisplay(pixd, 100 + 20 * i, 100 + 20 * i);
	pixDestroy(&pixd);
    }
#endif

#if 0
        /* timing of in-place shear rotation */
    for (i = 0; i < NITERS; i++) {
	pixRotateShearIP(pixs, w/2, h/2, deg2rad * angle, L_BRING_IN_WHITE);
/*	pixRotateShearCenterIP(pixs, deg2rad * angle, L_BRING_IN_WHITE); */
	pixDisplay(pixs, 100 + 20 * i, 100 + 20 * i);
    }
    pixd = pixs;
    if (pixGetDepth(pixd) == 1)
	pixWrite(fileout, pixd, IFF_PNG);
    else
	pixWrite(fileout, pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixs);
#endif

#if 0
        /* timing of various rotation operations (choose) */
    startTimer();
    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);
    for (i = 0; i < NTIMES; i++) {
	pixd = pixRotateShearCenter(pixs, deg2rad * angle, L_BRING_IN_WHITE);
	pixDestroy(&pixd);
    }
    pops = (l_float32)(w * h * NTIMES / 1000000.) / stopTimer();
    fprintf(stderr, "vers. 1, mpops: %f\n", pops);
    startTimer();
    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);
    for (i = 0; i < NTIMES; i++) {
	pixRotateShearIP(pixs, w/2, h/2, deg2rad * angle, L_BRING_IN_WHITE);
    }
    pops = (l_float32)(w * h * NTIMES / 1000000.) / stopTimer();
    fprintf(stderr, "shear, mpops: %f\n", pops);
    pixWrite(fileout, pixs, IFF_PNG);
    for (i = 0; i < NTIMES; i++) {
	pixRotateShearIP(pixs, w/2, h/2, -deg2rad * angle, L_BRING_IN_WHITE);
    }
    pixWrite("/usr/tmp/junkout", pixs, IFF_PNG);
#endif

#if 0
        /* area-mapping rotation operations */
    pixd = pixRotateAM(pixs, deg2rad * angle, L_BRING_IN_WHITE);
/*    pixd = pixRotateAMColorFast(pixs, deg2rad * angle, 255); */
    if (pixGetDepth(pixd) == 1)
	pixWrite(fileout, pixd, IFF_PNG);
    else
	pixWrite(fileout, pixd, IFF_JFIF_JPEG);
#endif

#if 0
	/* compare the standard area-map color rotation with
	 * the fast area-map color rotation, on a pixel basis */
    {
    PIX    *pix1, *pix2;
    NUMA   *nar, *nag, *nab, *naseq;
    GPLOT  *gplot;

    startTimer();
    pix1 = pixRotateAMColor(pixs, 0.12, 0xffffff00);
    fprintf(stderr, " standard color rotate: %7.2f sec\n", stopTimer());
    pixWrite("junkcolor1", pix1, IFF_JFIF_JPEG);
    startTimer();
    pix2 = pixRotateAMColorFast(pixs, 0.12, 0xffffff00);
    fprintf(stderr, " fast color rotate: %7.2f sec\n", stopTimer());
    pixWrite("junkcolor2", pix2, IFF_JFIF_JPEG);
    pixd = pixAbsDifference(pix1, pix2);
    pixGetColorHistogram(pixd, 1, &nar, &nag, &nab);
    naseq = numaMakeSequence(0., 1., 256);
    gplot = gplotCreate("junk_absdiff", GPLOT_X11, "Number vs diff",
			"diff", "number");
    gplotAddPlot(gplot, naseq, nar, GPLOT_POINTS, "red");
    gplotAddPlot(gplot, naseq, nag, GPLOT_POINTS, "green");
    gplotAddPlot(gplot, naseq, nab, GPLOT_POINTS, "blue");
    gplotMakeOutput(gplot);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pixd);
    numaDestroy(&nar);
    numaDestroy(&nag);
    numaDestroy(&nab);
    numaDestroy(&naseq);
    gplotDestroy(&gplot);
    }
#endif

        /* Do a succession of 180 7-degree rotations in a cw
	 * direction, and unwind the result with another set in
	 * a ccw direction.  Although there is a considerable amount
	 * of distortion after successive rotations, after all
	 * 360 rotations, the resulting image is restored to
	 * its original pristine condition! */
#if 1
    rotflag = L_ROTATE_AREA_MAP;
/*    rotflag = L_ROTATE_SHEAR;     */
/*    rotflag = L_ROTATE_SAMPLING;   */
    ang = 7.0 * deg2rad;
    pixGetDimensions(pixs, &w, &h, NULL);
    pixd = pixRotate(pixs, ang, rotflag, L_BRING_IN_WHITE, w, h);
    pixWrite("junkrot7", pixd, IFF_PNG);
    for (i = 1; i < 180; i++) {
        pixs = pixd;
        pixd = pixRotate(pixs, ang, rotflag, L_BRING_IN_WHITE, w, h);
        if ((i % 30) == 0)  pixDisplay(pixd, 600, 0);
        pixDestroy(&pixs);
    }

    pixWrite("junkspin", pixd, IFF_PNG);
    pixDisplay(pixd, 0, 0);

    for (i = 0; i < 180; i++) {
        pixs = pixd;
        pixd = pixRotate(pixs, -ang, rotflag, L_BRING_IN_WHITE, w, h);
        if (i && (i % 30) == 0)  pixDisplay(pixd, 600, 500);
        pixDestroy(&pixs);
    }

    pixWrite("junkunspin", pixd, IFF_PNG);
    pixDisplay(pixd, 0, 500);
    pixDestroy(&pixd);
#endif

    return 0;
}

