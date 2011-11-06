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
 * convolvetest.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const char  *kdatastr = " 20    50   80  50   20 "
                               " 50   100  140  100  50 "
                               " 90   160  200  160  90 "
                               " 50   100  140  100  50 "
                               " 20    50   80   50  20 ";

#define  NTIMES   100

main(int    argc,
     char **argv)
{
l_int32      i, j, wc, hc, d;
L_KERNEL    *kel1, *kel2;
PIX         *pixs, *pixg, *pixacc, *pixd, *pixt;
char        *filein, *fileout;
static char  mainName[] = "convolvetest";

    if (argc != 5)
	exit(ERROR_INT(" Syntax:  convolvetest filein wc hc fileout", mainName, 1));

    filein = argv[1];
    wc = atoi(argv[2]);
    hc = atoi(argv[3]);
    fileout = argv[4];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));

#if 0  /* Measure speed */
    pixacc = pixBlockconvAccum(pixs);
    for (i = 0; i < NTIMES; i++) {
	pixd = pixBlockconvGray(pixs, pixacc, wc, hc);
	if ((i+1) % 10 == 0)
	    fprintf(stderr, "%d iters\n", i + 1);
	pixDestroy(&pixd);
    }
    pixd = pixBlockconvGray(pixs, pixacc, wc, hc);
    pixWrite(fileout, pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixacc);
#endif

#if 0  /* Test pixBlockconvGray() */
    pixacc = pixBlockconvAccum(pixs);
    pixd = pixBlockconvGray(pixs, pixacc, wc, hc);
    pixWrite(fileout, pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixacc);
#endif

#if 0  /* Test pixBlockconv() */
    pixd = pixBlockconv(pixs, wc, hc);
    pixWrite(fileout, pixd, IFF_JFIF_JPEG);
#endif

#if 0  /* Test pixBlockrank() */
    pixacc = pixBlockconvAccum(pixs);
    pixd = pixBlockrank(pixs, pixacc, wc, hc, 0.5);
    pixWrite(fileout, pixd, IFF_TIFF_G4);
    pixDestroy(&pixacc);
#endif

#if 0  /* Test pixBlocksum() */
    pixacc = pixBlockconvAccum(pixs);
    pixd = pixBlocksum(pixs, pixacc, wc, hc);
    pixInvert(pixd, pixd);
    pixWrite(fileout, pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixacc);
#endif

#if 0  /* Test pixCensusTransform() */
    d = pixGetDepth(pixs);
    if (d == 32)
        pixt = pixConvertRGBToLuminance(pixs);
    else
        pixt = pixClone(pixs);
    pixacc = pixBlockconvAccum(pixt);
    pixd = pixCensusTransform(pixt, wc, NULL);
    pixDestroy(&pixt);
    pixDestroy(&pixacc);
    pixWrite(fileout, pixd, IFF_PNG);
#endif

#if 1   /* Test generic convolution with kel1 */
    if (pixGetDepth(pixs) == 32)
        pixg = pixScaleRGBToGrayFast(pixs, 2, COLOR_GREEN);
    else
        pixg = pixScale(pixs, 0.5, 0.5);
    pixDisplay(pixg, 0, 600);
    kel1 = kernelCreateFromString(5, 5, 2, 2, kdatastr);
    pixd = pixConvolve(pixg, kel1, 8, 1);
    pixDisplay(pixd, 700, 0);
    pixWrite("/tmp/junkpixd4.bmp", pixd, IFF_BMP);
    pixDestroy(&pixd);
    kernelDestroy(&kel1);

        /* Test convolution with flat rectangular kel */
    kel2 = kernelCreate(11, 11);
    kernelSetOrigin(kel2, 5, 5);
    for (i = 0; i < 11; i++) {
        for (j = 0; j < 11; j++)
            kernelSetElement(kel2, i, j, 1);
    }
    startTimer();
    pixd = pixConvolve(pixg, kel2, 8, 1);
    fprintf(stderr, "Generic convolution: %7.3f sec\n", stopTimer());
    pixDisplay(pixd, 1200, 0);
    pixWrite("/tmp/junkpixd5.bmp", pixd, IFF_BMP);
    startTimer();
    pixt = pixBlockconv(pixg, 5, 5);
    fprintf(stderr, "Block convolution: %7.3f sec\n", stopTimer());
    pixDisplay(pixd, 1200, 600);
    pixWrite("/tmp/junkpixd6.bmp", pixt, IFF_BMP);
    pixCompareGray(pixd, pixt, L_COMPARE_ABS_DIFF, GPLOT_X11, NULL,
                   NULL, NULL, NULL);
    pixDestroy(&pixg);
    pixDestroy(&pixt);
    kernelDestroy(&kel2);
#endif

    pixDestroy(&pixs);
    pixDestroy(&pixd);
    return 0;
}

