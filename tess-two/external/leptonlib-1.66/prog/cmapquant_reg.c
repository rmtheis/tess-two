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
 * cmapquant_reg.c
 *
 *   Tests quantization of rgb image to a specific colormap.
 *   Does this by starting with a grayscale image, doing a grayscale
 *   quantization with a colormap in the dest, then adding new
 *   colors, scaling (which removes the colormap), and finally
 *   re-quantizing back to the original colormap.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define  LEVEL       3
#define  MIN_DEPTH   4

main(int    argc,
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
        exit(ERROR_INT("syntax: cmapquant_req", mainName, 1));

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
    FREE(cmaptab);
    FREE(rtab);
    FREE(gtab);
    FREE(btab);

    pixDisplayMultiple("/tmp/junk_write_display*");

    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    return 0;
}


