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
 * cmapquant_reg.c
 *
 *   Tests quantization of rgb image to a specific colormap.
 *   Does this by starting with a grayscale image, doing a grayscale
 *   quantization with a colormap in the dest, then adding new
 *   colors, scaling (which removes the colormap), and finally
 *   re-quantizing back to the original colormap.
 */

#include "allheaders.h"

#define  LEVEL       3
#define  MIN_DEPTH   4

int main(int    argc,
         char **argv)
{
l_int32      same;
l_uint32    *rtab, *gtab, *btab;
l_int32     *cmaptab;
BOX         *box;
PIX         *pixs, *pixt1, *pixt2, *pixt3, *pixt4;
PIXCMAP     *cmap;
static char  mainName[] = "cmapquant_reg";

    pixs = pixRead("lucasta-frag.jpg");
    if (argc != 1)
        return ERROR_INT("syntax: cmapquant_req", mainName, 1);

        /* Convert to 4 bpp with 6 levels and a colormap */
    pixt1 = pixThresholdTo4bpp(pixs, 6, 1);

        /* Color some non-white pixels, preserving antialiasing, and
         * adding these colors to the colormap */
    box = boxCreate(120, 30, 200, 200);
    pixColorGray(pixt1, box, L_PAINT_DARK, 220, 0, 0, 255);
    pixDisplayWrite(pixt1, 1);
    boxDestroy(&box);

        /* Scale up by 1.5; losing the colormap */
    startTimer();
    pixt2 = pixScale(pixt1, 1.5, 1.5);
    fprintf(stderr, "Time to scale by 1.5x = %7.3f sec\n", stopTimer());
    pixDisplayWrite(pixt2, 1);

        /* Re-quantize using the same colormap */
    startTimer();
    cmap = pixGetColormap(pixt1);
    pixt3 = pixOctcubeQuantFromCmap(pixt2, cmap, MIN_DEPTH,
                                    LEVEL, L_EUCLIDEAN_DISTANCE);
    fprintf(stderr, "Time to requantize to cmap = %7.3f sec\n", stopTimer());
    pixDisplayWrite(pixt3, 1);

        /* Re-quantize first making the tables and then
         * using the lower-level function */
    startTimer();
    makeRGBToIndexTables(&rtab, &gtab, &btab, LEVEL);
    cmaptab = pixcmapToOctcubeLUT(cmap, LEVEL, L_EUCLIDEAN_DISTANCE);
    fprintf(stderr, "Time to make tables = %7.3f sec\n", stopTimer());
    startTimer();
    pixt4 = pixOctcubeQuantFromCmapLUT(pixt2, cmap, MIN_DEPTH,
                                       cmaptab, rtab, gtab, btab);
    fprintf(stderr, "Time for lowlevel re-quant = %7.3f sec\n", stopTimer());
    pixDisplayWrite(pixt4, 1);

    pixEqual(pixt3, pixt4, &same);
    if (same)
        fprintf(stderr, "Correct: images are the same\n");
    else
        fprintf(stderr, "Error: images differ\n");
    lept_free(cmaptab);
    lept_free(rtab);
    lept_free(gtab);
    lept_free(btab);

    pixDisplayMultiple("/tmp/display/file*");

    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    return 0;
}


