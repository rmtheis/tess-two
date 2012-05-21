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
 * grayquant_reg.c
 *
 *     Tests gray thresholding to 1, 2 and 4 bpp, with and without colormaps
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const l_int32  THRESHOLD = 130;
    /* nlevels for 4 bpp output; anything between 2 and 16 */
static const l_int32  NLEVELS = 4;


main(int    argc,
     char **argv)
{
const char  *str;
l_int32      equal, index, w, h;
BOX         *box;
PIX         *pixs, *pixd, *pixt, *pixd1, *pixd2, *pixd3;
PIX         *pixt0, *pixt1, *pixt2, *pixt3, *pixt4;
PIXA        *pixa;
PIXCMAP     *cmap;
static char  mainName[] = "grayquant_reg";

    if ((pixs = pixRead("test8.jpg")) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));

    pixa = pixaCreate(0);
    pixSaveTiled(pixs, pixa, 1, 1, 20, 8);

        /* threshold to 1 bpp */
    pixd = pixThresholdToBinary(pixs, THRESHOLD);
    pixSaveTiled(pixd, pixa, 1, 1, 20, 0);
    pixWrite("/tmp/junkthr0.png", pixd, IFF_PNG);
    pixDestroy(&pixd);

        /* dither to 2 bpp, with and without colormap */
    pixd = pixDitherTo2bpp(pixs, 1);
    pixt = pixDitherTo2bpp(pixs, 0);
    pixt2 = pixConvertGrayToColormap(pixt);
    pixSaveTiled(pixd, pixa, 1, 1, 20, 0);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/junkthr1.png", pixd, IFF_PNG);
    pixWrite("/tmp/junkthr2.png", pixt, IFF_PNG);
    pixWrite("/tmp/junkthr3.png", pixt2, IFF_PNG);
/*    pixcmapWriteStream(stderr, pixGetColormap(pixd)); */
    pixEqual(pixd, pixt2, &equal);
    if (!equal)
        fprintf(stderr, "Error: thr2 != thr3\n");
    pixDestroy(&pixt);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);

        /* threshold to 2 bpp, with and without colormap */
    pixd = pixThresholdTo2bpp(pixs, 4, 1);
    pixt = pixThresholdTo2bpp(pixs, 4, 0);
    pixt2 = pixConvertGrayToColormap(pixt);
    pixSaveTiled(pixd, pixa, 1, 1, 20, 0);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/junkthr4.png", pixd, IFF_PNG);
    pixWrite("/tmp/junkthr5.png", pixt2, IFF_PNG);
    pixEqual(pixd, pixt2, &equal);
    if (!equal)
        fprintf(stderr, "Error: thr4 != thr5\n");
    pixDestroy(&pixt);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);

    pixd = pixThresholdTo2bpp(pixs, 3, 1);
    pixt = pixThresholdTo2bpp(pixs, 3, 0);
    pixSaveTiled(pixd, pixa, 1, 1, 20, 0);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/junkthr6.png", pixd, IFF_PNG);
    pixWrite("/tmp/junkthr7.png", pixt, IFF_PNG);
    pixDestroy(&pixt);
    pixDestroy(&pixd);

        /* threshold to 4 bpp, with and without colormap */
    pixd = pixThresholdTo4bpp(pixs, 9, 1);
    pixt = pixThresholdTo4bpp(pixs, 9, 0);
    pixt2 = pixConvertGrayToColormap(pixt);
    pixSaveTiled(pixd, pixa, 1, 1, 20, 0);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/junkthr8.png", pixd, IFF_PNG);
    pixWrite("/tmp/junkthr9.png", pixt, IFF_PNG);
    pixWrite("/tmp/junkthr10.png", pixt2, IFF_PNG);
/*    pixcmapWriteStream(stderr, pixGetColormap(pixd)); */
    pixDestroy(&pixt);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);

        /* threshold on 8 bpp, with and without colormap */
    pixd = pixThresholdOn8bpp(pixs, 9, 1);
    pixt = pixThresholdOn8bpp(pixs, 9, 0);
    pixt2 = pixConvertGrayToColormap(pixt);
    pixSaveTiled(pixd, pixa, 1, 1, 20, 0);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/junkthr11.png", pixd, IFF_PNG);
    pixWrite("/tmp/junkthr12.png", pixt2, IFF_PNG);
/*    pixcmapWriteStream(stderr, pixGetColormap(pixd)); */
    pixEqual(pixd, pixt2, &equal);
    if (!equal)
        fprintf(stderr, "Error: thr11 != thr12\n");
    pixDestroy(&pixt);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);

    pixd1 = pixaDisplay(pixa, 0, 0);
    pixDisplay(pixd1, 100, 100);
    pixWrite("/tmp/junkpixd1.jpg", pixd1, IFF_JFIF_JPEG);
    pixDestroy(&pixd1);
    pixaDestroy(&pixa);

    pixa = pixaCreate(0);
    pixSaveTiled(pixs, pixa, 1, 1, 20, 32);

        /* highlight 2 bpp with colormap */
    pixd = pixThresholdTo2bpp(pixs, 3, 1);
    cmap = pixGetColormap(pixd);
    pixcmapWriteStream(stderr, cmap);
    box = boxCreate(278, 35, 122, 50);
    pixSetSelectCmap(pixd, box, 2, 255, 255, 100);
    pixcmapWriteStream(stderr, cmap);
    pixDisplay(pixd, 0, 0);
    pixSaveTiled(pixd, pixa, 1, 1, 20, 0);
    pixWrite("/tmp/junkthr13.png", pixd, IFF_PNG);
    pixDestroy(&pixd);
    boxDestroy(&box);

        /* test pixThreshold8() */
    pixd = pixThreshold8(pixs, 1, 2, 1);  /* cmap */
    pixSaveTiled(pixd, pixa, 1, 1, 20, 0);
    pixWrite("/tmp/junkthr14.png", pixd, IFF_PNG);
    pixDisplay(pixd, 100, 0);
    pixDestroy(&pixd);
    pixd = pixThreshold8(pixs, 1, 2, 0);  /* no cmap */
    pixSaveTiled(pixd, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/junkthr15.png", pixd, IFF_PNG);
    pixDisplay(pixd, 200, 0);
    pixDestroy(&pixd);
    pixd = pixThreshold8(pixs, 2, 3, 1);  /* highlight one box */
    pixSaveTiled(pixd, pixa, 1, 0, 20, 0);
    box = boxCreate(278, 35, 122, 50);
    pixSetSelectCmap(pixd, box, 2, 255, 255, 100);
    pixWrite("/tmp/junkthr16.png", pixd, IFF_PNG);
    pixDisplay(pixd, 300, 0);
    cmap = pixGetColormap(pixd);
    pixcmapWriteStream(stderr, cmap);
    boxDestroy(&box);
    pixDestroy(&pixd);
    pixd = pixThreshold8(pixs, 2, 4, 0);  /* no cmap */
    pixSaveTiled(pixd, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/junkthr17.png", pixd, IFF_PNG);
    pixDisplay(pixd, 400, 0);
    pixDestroy(&pixd);
    pixd = pixThreshold8(pixs, 4, 6, 1);  /* highlight one box */
    box = boxCreate(278, 35, 122, 50);
    pixSetSelectCmap(pixd, box, 5, 255, 255, 100);
    pixSaveTiled(pixd, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/junkthr18.png", pixd, IFF_PNG);
    cmap = pixGetColormap(pixd);
    pixcmapWriteStream(stderr, cmap);
    boxDestroy(&box);
    pixDisplay(pixd, 500, 0);
    pixDestroy(&pixd);
    pixd = pixThreshold8(pixs, 4, 6, 0);  /* no cmap */
    pixSaveTiled(pixd, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/junkthr19.png", pixd, IFF_PNG);
    pixDisplay(pixd, 600, 0);
    pixDestroy(&pixd);

        /* highlight 4 bpp with 2 colormap entries */
        /* Note: We use 5 levels (0-4) for gray.           */
	/*       5 & 6 are used for highlight color.       */
    pixd = pixThresholdTo4bpp(pixs, 5, 1);
    cmap = pixGetColormap(pixd);
    pixcmapGetIndex(cmap, 255, 255, 255, &index);
    box = boxCreate(278, 35, 122, 50);
    pixSetSelectCmap(pixd, box, index, 255, 255, 100);  /* use 5 */
    boxDestroy(&box);
    box = boxCreate(4, 6, 157, 33);
    pixSetSelectCmap(pixd, box, index, 100, 255, 255);  /* use 6 */
    boxDestroy(&box);
    pixcmapWriteStream(stderr, cmap);
    pixSaveTiled(pixd, pixa, 1, 1, 20, 0);
    pixDisplay(pixd, 700, 0);
    pixWrite("/tmp/junkthr20.png", pixd, IFF_PNG);
    pixDestroy(&pixd);

        /* comparison 8 bpp jpeg with 2 bpp (highlight) */
    pixDestroy(&pixs);
    pixs = pixRead("feyn.tif");
    pixt = pixScaleToGray4(pixs);
    pixt2 = pixReduceRankBinaryCascade(pixs, 2, 2, 0, 0);
    pixd = pixThresholdTo2bpp(pixt, 3, 1);
    box = boxCreate(175, 208, 228, 88);
    pixSetSelectCmap(pixd, box, 2, 255, 255, 100);
    pixDisplay(pixd, 100, 200);
    cmap = pixGetColormap(pixd);
    pixcmapWriteStream(stderr, cmap);
    pixSaveTiled(pixt, pixa, 1, 1, 20, 0);
    pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);
    pixSaveTiled(pixd, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/junkthr21.jpg", pixt, IFF_JFIF_JPEG);
    pixWrite("/tmp/junkthr22.png", pixt2, IFF_PNG);
    pixWrite("/tmp/junkthr23.png", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixDestroy(&pixt2);
    boxDestroy(&box);

        /* thresholding to 4 bpp (highlight); use pixt from above */
    pixd = pixThresholdTo4bpp(pixt, NLEVELS, 1);
    box = boxCreate(175, 208, 228, 83);
    pixSetSelectCmap(pixd, box, NLEVELS - 1, 255, 255, 100);
    boxDestroy(&box);
    box = boxCreate(232, 298, 110, 25);
    pixSetSelectCmap(pixd, box, NLEVELS - 1, 100, 255, 255);
    boxDestroy(&box);
    box = boxCreate(21, 698, 246, 82);
    pixSetSelectCmap(pixd, box, NLEVELS - 1, 225, 100, 255);
    boxDestroy(&box);
    pixDisplay(pixd, 500, 200);
    cmap = pixGetColormap(pixd);
    pixcmapWriteStream(stderr, cmap);
    pixt2 = pixReduceRankBinaryCascade(pixs, 2, 2, 0, 0);
    pixSaveTiled(pixt2, pixa, 1, 1, 20, 0);
    pixSaveTiled(pixd, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/junkthr24.png", pixt2, IFF_PNG);
    pixWrite("/tmp/junkthr25.png", pixd, IFF_PNG);
    pixDestroy(&pixt2);
    pixDestroy(&pixd);

       /* Thresholding to 4 bpp at 2, 3, 4, 5 and 6 levels */
    box = boxCreate(25, 202, 136, 37);
    pixt1 = pixClipRectangle(pixt, box, NULL);
    pixt2 = pixScale(pixt1, 6., 6.);
    pixGetDimensions(pixt2, &w, &h, NULL);
    pixSaveTiled(pixt2, pixa, 1, 1, 20, 0);
    pixDisplay(pixt2, 0, 0);
    pixWrite("/tmp/junk-8.jpg", pixt2, IFF_JFIF_JPEG);
    pixd = pixCreate(w, 6 * h, 8);
    pixRasterop(pixd, 0, 0, w, h, PIX_SRC, pixt2, 0, 0);

    pixt3 = pixThresholdTo4bpp(pixt2, 6, 1);
    pixt4 = pixRemoveColormap(pixt3, REMOVE_CMAP_TO_GRAYSCALE);
    pixRasterop(pixd, 0, h, w, h, PIX_SRC, pixt4, 0, 0);
    pixSaveTiled(pixt3, pixa, 1, 0, 20, 0);
    pixDisplay(pixt3, 0, 100);
    pixWrite("/tmp/junk-4-6.png", pixt3, IFF_PNG);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

    pixt3 = pixThresholdTo4bpp(pixt2, 5, 1);
    pixt4 = pixRemoveColormap(pixt3, REMOVE_CMAP_TO_GRAYSCALE);
    pixRasterop(pixd, 0, 2 * h, w, h, PIX_SRC, pixt4, 0, 0);
    pixSaveTiled(pixt3, pixa, 1, 0, 20, 0);
    pixDisplay(pixt3, 0, 200);
    pixWrite("/tmp/junk-4-5.png", pixt3, IFF_PNG);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

    pixt3 = pixThresholdTo4bpp(pixt2, 4, 1);
    pixt4 = pixRemoveColormap(pixt3, REMOVE_CMAP_TO_GRAYSCALE);
    pixRasterop(pixd, 0, 3 * h, w, h, PIX_SRC, pixt4, 0, 0);
    pixSaveTiled(pixt3, pixa, 1, 0, 20, 0);
    pixDisplay(pixt3, 0, 300);
    pixWrite("/tmp/junk-4-4.png", pixt3, IFF_PNG);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

    pixt3 = pixThresholdTo4bpp(pixt2, 3, 1);
    pixt4 = pixRemoveColormap(pixt3, REMOVE_CMAP_TO_GRAYSCALE);
    pixRasterop(pixd, 0, 4 * h, w, h, PIX_SRC, pixt4, 0, 0);
    pixSaveTiled(pixt3, pixa, 1, 1, 20, 0);
    pixDisplay(pixt3, 0, 400);
    pixWrite("/tmp/junk-4-3.png", pixt3, IFF_PNG);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

    pixt3 = pixThresholdTo4bpp(pixt2, 2, 1);
    pixt4 = pixRemoveColormap(pixt3, REMOVE_CMAP_TO_GRAYSCALE);
    pixRasterop(pixd, 0, 5 * h, w, h, PIX_SRC, pixt4, 0, 0);
    pixDisplay(pixt3, 0, 500);
    pixSaveTiled(pixt3, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/junk-4-2.png", pixt3, IFF_PNG);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixWrite("/tmp/junk-all.png", pixd, IFF_PNG);
    pixDestroy(&pixd);

    pixd2 = pixaDisplay(pixa, 0, 0);
    pixDisplay(pixd2, 100, 100);
    pixWrite("/tmp/junkpixd2.jpg", pixd2, IFF_JFIF_JPEG);
    pixDestroy(&pixd2);
    pixaDestroy(&pixa);

#if 0   /* upscale 2x and threshold to 1 bpp; e.g., use test8.jpg */
    startTimer();
    pixd = pixScaleGray2xLIThresh(pixs, THRESHOLD);
    fprintf(stderr, " time for scale/dither = %7.3f sec\n", stopTimer());
    pixWrite(fileout, pixd, IFF_PNG);
    pixDestroy(&pixd);
#endif

#if 0   /* upscale 4x and threshold to 1 bpp; e.g., use test8.jpg */
    startTimer();
    pixd = pixScaleGray4xLIThresh(pixs, THRESHOLD);
    fprintf(stderr, " time for scale/dither = %7.3f sec\n", stopTimer());
    pixWrite(fileout, pixd, IFF_PNG);
    pixDestroy(&pixd);
#endif

    boxDestroy(&box);
    pixDestroy(&pixt);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixs);

       /* Thresholding with fixed and arbitrary bin boundaries */
    pixa = pixaCreate(0);
    pixs = pixRead("stampede2.jpg");
    pixSaveTiled(pixs, pixa, 1, 1, 20, 8);
    pixt = pixThresholdTo4bpp(pixs, 5, 1);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt);
    pixt = pixThresholdTo4bpp(pixs, 7, 1);
    cmap = pixGetColormap(pixt);
    pixcmapWriteStream(stderr, cmap);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt);
    pixt = pixThresholdTo4bpp(pixs, 11, 1);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt);

    pixSaveTiled(pixs, pixa, 1, 1, 20, 8);
    str = "45 75 115 185";
    pixt = pixThresholdGrayArb(pixs, str, 8, 0, 0, 0);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt);
    str = "38 65 85 115 160 210";
    pixt = pixThresholdGrayArb(pixs, str, 8, 0, 1, 1);
    cmap = pixGetColormap(pixt);
    pixcmapWriteStream(stderr, cmap);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt);
    str = "38 60 75 90 110 130 155 185 208 239";
    pixt = pixThresholdGrayArb(pixs, str, 8, 0, 0, 0);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt);

    pixSaveTiled(pixs, pixa, 1, 1, 20, 8);
    str = "45 75 115 185";
    pixt = pixThresholdGrayArb(pixs, str, 0, 1, 0, 1);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt);
    str = "38 65 85 115 160 210";
    pixt = pixThresholdGrayArb(pixs, str, 0, 1, 0, 1);
    cmap = pixGetColormap(pixt);
    pixcmapWriteStream(stderr, cmap);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt);
    str = "38 60 75 90 110 130 155 185 208 239";
    pixt = pixThresholdGrayArb(pixs, str, 4, 1, 0, 1);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt);

    pixd3 = pixaDisplay(pixa, 0, 0);
    pixDisplay(pixd3, 100, 100);
    pixWrite("/tmp/junkpixd3.jpg", pixd3, IFF_JFIF_JPEG);
    pixDestroy(&pixd3);
    pixaDestroy(&pixa);

    pixDestroy(&pixs);

    return 0;
}

