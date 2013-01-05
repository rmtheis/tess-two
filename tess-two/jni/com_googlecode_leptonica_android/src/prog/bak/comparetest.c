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
 * comparetest.c
 *
 *      comparetest filein1 filein2 type fileout
 *
 *    where
 *      type = {0, 1} for {abs-diff and subtraction} comparisons
 *
 *    Compares two images, using either the absolute value of the
 *    pixel differences or the difference clipped to 0.  For RGB,
 *    the differences are computed separately on each component.
 *    If one has a colormap and the other doesn't, the colormap
 *    is removed before making the comparison.
 *
 *    Warning: you usually want to use abs-diff to compare
 *    two grayscale or color images.  If you use subtraction,
 *    the result you get will depend on the order of the input images.
 *    For example, if pix2 = pixDilateGray(pix1), then every
 *    pixel in pix1 will be equal to or greater than pix2.  So if
 *    you subtract pix2 from pix1, you will get 0 for all pixels,
 *    which looks like they're the same!
 *
 *    Here's an interesting observation.  Take an image that has
 *    been jpeg compressed at a quality = 75.  If you re-compress
 *    the image, what quality factor should be used to minimize
 *    the change?  Answer:  75 (!)
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
l_int32      type, comptype, d1, d2, same, first, last;
l_float32    fract, diff, rmsdiff;
char        *filein1, *filein2, *fileout;
GPLOT       *gplot;
NUMA        *na1, *na2;
PIX         *pixs1, *pixs2, *pixd;
static char  mainName[] = "comparetest";

    if (argc != 5)
	exit(ERROR_INT(" Syntax:  comparetest filein1 filein2 type fileout",
	               mainName, 1));

    filein1 = argv[1];
    filein2 = argv[2];
    type = atoi(argv[3]);
    pixd = NULL;
    fileout = argv[4];
    l_pngSetStrip16To8(0);

    if ((pixs1 = pixRead(filein1)) == NULL)
	exit(ERROR_INT("pixs1 not made", mainName, 1));
    if ((pixs2 = pixRead(filein2)) == NULL)
	exit(ERROR_INT("pixs2 not made", mainName, 1));
    d1 = pixGetDepth(pixs1);
    d2 = pixGetDepth(pixs2);

    if (d1 == 1 && d2 == 1) {
        pixEqual(pixs1, pixs2, &same);
        if (same) {
            fprintf(stderr, "Images are identical\n");
            pixd = pixCreateTemplate(pixs1);  /* write empty pix for diff */
        }
        else {
            if (type == 0)
                comptype = L_COMPARE_XOR;
            else
                comptype = L_COMPARE_SUBTRACT;
            pixCompareBinary(pixs1, pixs2, comptype, &fract, &pixd);
            fprintf(stderr, "Fraction of different pixels: %10.6f\n", fract);
        }
        pixWrite(fileout, pixd, IFF_PNG);
    }
    else {
        if (type == 0)
            comptype = L_COMPARE_ABS_DIFF;
        else
            comptype = L_COMPARE_SUBTRACT;
        pixCompareGrayOrRGB(pixs1, pixs2, comptype, GPLOT_X11, &same, &diff,
                            &rmsdiff, &pixd);
        if (type == 0) {
            if (same)
                fprintf(stderr, "Images are identical\n");
            else {
                fprintf(stderr, "Images differ: <diff> = %10.6f\n", diff);
                fprintf(stderr, "               <rmsdiff> = %10.6f\n", rmsdiff);
            }
        }
        else {  /* subtraction */
            if (same)
                fprintf(stderr, "pixs2 strictly greater than pixs1\n");
            else {
                fprintf(stderr, "Images differ: <diff> = %10.6f\n", diff);
                fprintf(stderr, "               <rmsdiff> = %10.6f\n", rmsdiff);
            }
        }
        if (d1 != 16)
            pixWrite(fileout, pixd, IFF_JFIF_JPEG);
        else 
            pixWrite(fileout, pixd, IFF_PNG);

        if (d1 != 16) {
            na1 = pixCompareRankDifference(pixs1, pixs2, 1);
            if (na1) {
                fprintf(stderr, "na1[150] = %20.10f\n", na1->array[150]);
                fprintf(stderr, "na1[200] = %20.10f\n", na1->array[200]);
                fprintf(stderr, "na1[250] = %20.10f\n", na1->array[250]);
                numaGetNonzeroRange(na1, 0.00005, &first, &last);
                fprintf(stderr, "Nonzero diff range: first = %d, last = %d\n",
                        first, last);
                na2 = numaClipToInterval(na1, first, last);
                gplot = gplotCreate("/tmp/junkrank", GPLOT_X11,
                                    "Pixel Rank Difference", "pixel val",
                                    "rank");
                gplotAddPlot(gplot, NULL, na2, GPLOT_LINES, "rank");
                gplotMakeOutput(gplot);
                gplotDestroy(&gplot);
                numaDestroy(&na1);
                numaDestroy(&na2);
            }
        }
    } 

    pixDestroy(&pixs1);
    pixDestroy(&pixs2);
    pixDestroy(&pixd);
    return 0;
}

