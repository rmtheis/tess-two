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
 *   gifio_reg.c
 *
 *     This tests reading and writing gif for various image types.
 *
 *     The relative times for writing of gif and png are interesting.
 *
 *     For 1 bpp:
 *
 *        png writing is about 2x faster than gif writing, using giflib.
 *
 *     For 32 bpp, using a 1 Mpix rgb image:
 *
 *       png:  Lossless: 1.16 sec (2.0 MB output file)
 *             Lossy: 0.43 sec, composed of:
 *                       0.22 sec (octree quant with dithering)
 *                       0.21 sec (to compress and write out)
 *             
 *       gif:  Lossy: 0.34 sec, composed of:
 *                       0.22 sec (octree quant with dithering)
 *                       0.12 sec (to compress and write out) 
 *             (note: no lossless mode; gif can't write out rgb)          
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allheaders.h"

    /* Needed for HAVE_LIBGIF and or HAVE_LIBUNGIF */
#ifdef HAVE_CONFIG_H
#include <config_auto.h>
#endif /* HAVE_CONFIG_H */

#define   FILE_1BPP     "feyn.tif"
#define   FILE_2BPP     "weasel2.4g.png"
#define   FILE_4BPP     "weasel4.16c.png"
#define   FILE_8BPP_1   "dreyfus8.png"
#define   FILE_8BPP_2   "weasel8.240c.png"
#define   FILE_8BPP_3   "test8.jpg"
#define   FILE_16BPP    "test16.tif"
#define   FILE_32BPP    "marge.jpg"

#define   REDUCTION     1

main(int    argc,
char **argv)
{
l_int32      w, h, d, same, ret;
PIX         *pixs, *pix1, *pix2;
static char  mainName[] = "gifio_reg";
 
#if (!HAVE_LIBGIF) && (!HAVE_LIBUNGIF)
    fprintf(stderr, "!!!!!!!!!!!!!!!!!!!!\n"
            "gifio not enabled!\n"
            "See environ.h: #define HAVE_LIBGIF or HAVE_LIBUNGIF 1\n"
            "See prog/Makefile: link in -lgif or -lungif\n"
            "!!!!!!!!!!!!!!!!!!!!\n");
    return 1;
#endif

    pixDisplayWrite(NULL, -1);

    pixs = pixRead(FILE_1BPP);
    pixGetDimensions(pixs, &w, &h, &d);
    pixWrite("/tmp/junk1.gif", pixs, IFF_GIF);
    startTimer();
    pix1 = pixRead("/tmp/junk1.gif");
    fprintf(stderr, "Read time for 8 Mpix 1 bpp: %7.3f sec\n", stopTimer());
    startTimer();
    pixWrite("/tmp/junk1n.gif", pix1, IFF_GIF);
    fprintf(stderr, "Write time for 8 Mpix 1 bpp: %7.3f\n", stopTimer());
    pix2 = pixRead("/tmp/junk1n.gif");
    pixDisplayWrite(pix2, REDUCTION);
    pixEqual(pixs, pix2, &same);
    if (!same)
        fprintf(stderr, "Error for %s\n", FILE_1BPP);
    else
        fprintf(stderr, "Correct for %s\n", FILE_1BPP);
    fprintf(stderr, "   depth: pixs = %d, pix1 = %d\n", d, pixGetDepth(pix1));
    pixDestroy(&pixs);
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pixs = pixRead(FILE_2BPP);
    pixGetDimensions(pixs, &w, &h, &d);
    pixWrite("/tmp/junk2.gif", pixs, IFF_GIF);
    pix1 = pixRead("/tmp/junk2.gif");
    pixWrite("/tmp/junk2n.gif", pix1, IFF_GIF);
    pix2 = pixRead("/tmp/junk2n.gif");
    pixDisplayWrite(pix2, REDUCTION);
    pixEqual(pixs, pix2, &same);
    if (!same)
        fprintf(stderr, "Error for %s\n", FILE_2BPP);
    else
        fprintf(stderr, "Correct for %s\n", FILE_2BPP);
    fprintf(stderr, "   depth: pixs = %d, pix1 = %d\n", d, pixGetDepth(pix1));
    pixDestroy(&pixs);
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pixs = pixRead(FILE_4BPP);
    pixGetDimensions(pixs, &w, &h, &d);
    pixWrite("/tmp/junk3.gif", pixs, IFF_GIF);
    pix1 = pixRead("/tmp/junk3.gif");
    pixWrite("/tmp/junk3n.gif", pix1, IFF_GIF);
    pix2 = pixRead("/tmp/junk3n.gif");
    pixDisplayWrite(pix2, REDUCTION);
    pixEqual(pixs, pix2, &same);
    if (!same)
        fprintf(stderr, "Error for %s\n", FILE_4BPP);
    else
        fprintf(stderr, "Correct for %s\n", FILE_4BPP);
    fprintf(stderr, "   depth: pixs = %d, pix1 = %d\n", d, pixGetDepth(pix1));
    pixDestroy(&pixs);
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pixs = pixRead(FILE_8BPP_1);
    pixGetDimensions(pixs, &w, &h, &d);
    pixWrite("/tmp/junk4.gif", pixs, IFF_GIF);
    pix1 = pixRead("/tmp/junk4.gif");
    pixWrite("/tmp/junk4n.gif", pix1, IFF_GIF);
    pix2 = pixRead("/tmp/junk4n.gif");
    pixDisplayWrite(pix2, REDUCTION);
    pixEqual(pixs, pix2, &same);
    if (!same)
        fprintf(stderr, "Error for %s\n", FILE_8BPP_1);
    else
        fprintf(stderr, "Correct for %s\n", FILE_8BPP_1);
    fprintf(stderr, "   depth: pixs = %d, pix1 = %d\n", d, pixGetDepth(pix1));
    pixDestroy(&pixs);
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pixs = pixRead(FILE_8BPP_2);
    pixGetDimensions(pixs, &w, &h, &d);
    pixWrite("/tmp/junk5.gif", pixs, IFF_GIF);
    pix1 = pixRead("/tmp/junk5.gif");
    pixWrite("/tmp/junk5n.gif", pix1, IFF_GIF);
    pix2 = pixRead("/tmp/junk5n.gif");
    pixDisplayWrite(pix2, REDUCTION);
    pixEqual(pixs, pix2, &same);
    if (!same)
        fprintf(stderr, "Error for %s\n", FILE_8BPP_2);
    else
        fprintf(stderr, "Correct for %s\n", FILE_8BPP_2);
    fprintf(stderr, "   depth: pixs = %d, pix1 = %d\n", d, pixGetDepth(pix1));
    pixDestroy(&pixs);
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pixs = pixRead(FILE_8BPP_3);
    pixGetDimensions(pixs, &w, &h, &d);
    pixWrite("/tmp/junk6.gif", pixs, IFF_GIF);
    pix1 = pixRead("/tmp/junk6.gif");
    pixWrite("/tmp/junk6n.gif", pix1, IFF_GIF);
    pix2 = pixRead("/tmp/junk6n.gif");
    pixDisplayWrite(pix2, REDUCTION);
    pixEqual(pixs, pix2, &same);
    if (!same)
        fprintf(stderr, "Error for %s\n", FILE_8BPP_3);
    else
        fprintf(stderr, "Correct for %s\n", FILE_8BPP_3);
    fprintf(stderr, "   depth: pixs = %d, pix1 = %d\n", d, pixGetDepth(pix1));
    pixDestroy(&pixs);
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pixs = pixRead(FILE_16BPP);
    pixGetDimensions(pixs, &w, &h, &d);
    pixWrite("/tmp/junk7.gif", pixs, IFF_GIF);
    pix1 = pixRead("/tmp/junk7.gif");
    pixWrite("/tmp/junk7n.gif", pix1, IFF_GIF);
    pix2 = pixRead("/tmp/junk7n.gif");
    pixDisplayWrite(pix2, REDUCTION);
    pixEqual(pix1, pix2, &same);
    if (!same)
        fprintf(stderr, "Error for %s\n", FILE_16BPP);
    else
        fprintf(stderr, "Correct for %s\n", FILE_16BPP);
    fprintf(stderr, "   depth: pixs = %d, pix1 = %d\n", d, pixGetDepth(pix1));
    pixDestroy(&pixs);
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pixs = pixRead(FILE_32BPP);
    pixGetDimensions(pixs, &w, &h, &d);
    pixSetText(pixs, "Junk GIF comment");
    pixWrite("/tmp/junk8.gif", pixs, IFF_GIF);
    pix1 = pixRead("/tmp/junk8.gif");
    pixWrite("/tmp/junk8n.gif", pix1, IFF_GIF);
    pix2 = pixRead("/tmp/junk8n.gif");
    pixDisplayWrite(pix2, REDUCTION);
    pixEqual(pix1, pix2, &same);
    if (!same)
        fprintf(stderr, "Error for %s\n", FILE_32BPP);
    else
        fprintf(stderr, "Correct for %s\n", FILE_32BPP);
    fprintf(stderr, "   depth: pixs = %d, pix1 = %d\n", d, pixGetDepth(pix1));
    pixDestroy(&pixs);
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pixDisplayMultiple("/tmp/junk_write_display*");
    return 0;
}
