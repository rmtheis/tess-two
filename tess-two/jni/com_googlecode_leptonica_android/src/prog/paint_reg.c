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
 * paint_reg.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
l_int32      index;
l_uint32     val32;
BOX         *box, *box1, *box2, *box3, *box4, *box5;
PIX         *pixs, *pixg, *pixb, *pixt, *pixt1, *pixt2, *pixt3;
PIXCMAP     *cmap;
static char  mainName[] = "paint_reg";

    if (argc != 1)
        exit(ERROR_INT(" Syntax: paint_reg", mainName, 1));

    if ((pixs = pixRead("lucasta-frag.jpg")) == NULL)
        exit(ERROR_INT("pixs not made", mainName, 1));
    pixDisplayWrite(NULL, -1);
    pixDisplayWrite(pixs, 1);

        /* Color non-white pixels on RGB */
    pixt = pixConvert8To32(pixs);
    box = boxCreate(120, 30, 200, 200);
    pixColorGray(pixt, box, L_PAINT_DARK, 220, 0, 0, 255);
    pixDisplayWrite(pixt, 1);
    pixColorGray(pixt, NULL, L_PAINT_DARK, 220, 255, 100, 100);
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);
    boxDestroy(&box);

        /* Color non-white pixels on colormap */
    pixt = pixThresholdTo4bpp(pixs, 6, 1);
    box = boxCreate(120, 30, 200, 200);
    pixColorGray(pixt, box, L_PAINT_DARK, 220, 0, 0, 255);
    pixDisplayWrite(pixt, 1);
    pixColorGray(pixt, NULL, L_PAINT_DARK, 220, 255, 100, 100);
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);
    boxDestroy(&box);

        /* Color non-black pixels on RGB */
    pixt = pixConvert8To32(pixs);
    box = boxCreate(120, 30, 200, 200);
    pixColorGray(pixt, box, L_PAINT_LIGHT, 20, 0, 0, 255);
    pixDisplayWrite(pixt, 1);
    pixColorGray(pixt, NULL, L_PAINT_LIGHT, 80, 255, 100, 100);
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);
    boxDestroy(&box);

        /* Color non-black pixels on colormap */
    pixt = pixThresholdTo4bpp(pixs, 6, 1);
    box = boxCreate(120, 30, 200, 200);
    pixColorGray(pixt, box, L_PAINT_LIGHT, 20, 0, 0, 255);
    pixDisplayWrite(pixt, 1);
    pixColorGray(pixt, NULL, L_PAINT_LIGHT, 20, 255, 100, 100);
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);
    boxDestroy(&box);

        /* Add highlight color to RGB */
    pixt = pixConvert8To32(pixs);
    box = boxCreate(507, 5, 385, 45);
    pixg = pixClipRectangle(pixs, box, NULL);
    pixb = pixThresholdToBinary(pixg, 180);
    pixInvert(pixb, pixb);
    pixDisplayWrite(pixb, 1);
    composeRGBPixel(50, 0, 250, &val32);
    pixPaintThroughMask(pixt, pixb, box->x, box->y, val32);
    boxDestroy(&box);
    pixDestroy(&pixg);
    pixDestroy(&pixb);
    box = boxCreate(236, 107, 262, 40);
    pixg = pixClipRectangle(pixs, box, NULL);
    pixb = pixThresholdToBinary(pixg, 180);
    pixInvert(pixb, pixb);
    composeRGBPixel(250, 0, 50, &val32);
    pixPaintThroughMask(pixt, pixb, box->x, box->y, val32);
    boxDestroy(&box);
    pixDestroy(&pixg);
    pixDestroy(&pixb);
    box = boxCreate(222, 208, 247, 43);
    pixg = pixClipRectangle(pixs, box, NULL);
    pixb = pixThresholdToBinary(pixg, 180);
    pixInvert(pixb, pixb);
    composeRGBPixel(60, 250, 60, &val32);
    pixPaintThroughMask(pixt, pixb, box->x, box->y, val32);
    boxDestroy(&box);
    pixDestroy(&pixg);
    pixDestroy(&pixb);
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);

        /* Add highlight color to colormap */
    pixt = pixThresholdTo4bpp(pixs, 5, 1);
    cmap = pixGetColormap(pixt);
    pixcmapGetIndex(cmap, 255, 255, 255, &index);
    box = boxCreate(507, 5, 385, 45);
    pixSetSelectCmap(pixt, box, index, 50, 0, 250);
    boxDestroy(&box);
    box = boxCreate(236, 107, 262, 40);
    pixSetSelectCmap(pixt, box, index, 250, 0, 50);
    boxDestroy(&box);
    box = boxCreate(222, 208, 247, 43);
    pixSetSelectCmap(pixt, box, index, 60, 250, 60);
    boxDestroy(&box);
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);

        /* Paint lines on RGB */
    pixt = pixConvert8To32(pixs);
    pixRenderLineArb(pixt, 450, 20, 850, 320, 5, 200, 50, 125);
    pixRenderLineArb(pixt, 30, 40, 440, 40, 5, 100, 200, 25);
    box = boxCreate(70, 80, 300, 245);
    pixRenderBoxArb(pixt, box, 3, 200, 200, 25);
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);
    boxDestroy(&box);

        /* Paint lines on colormap */
    pixt = pixThresholdTo4bpp(pixs, 5, 1);
    pixRenderLineArb(pixt, 450, 20, 850, 320, 5, 200, 50, 125);
    pixRenderLineArb(pixt, 30, 40, 440, 40, 5, 100, 200, 25);
    box = boxCreate(70, 80, 300, 245);
    pixRenderBoxArb(pixt, box, 3, 200, 200, 25);
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);
    boxDestroy(&box);

        /* Blend lines on RGB */
    pixt = pixConvert8To32(pixs);
    pixRenderLineBlend(pixt, 450, 20, 850, 320, 5, 200, 50, 125, 0.35);
    pixRenderLineBlend(pixt, 30, 40, 440, 40, 5, 100, 200, 25, 0.35);
    box = boxCreate(70, 80, 300, 245);
    pixRenderBoxBlend(pixt, box, 3, 200, 200, 25, 0.6);
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);
    boxDestroy(&box);

        /* Colorize gray on cmapped image. */
    pixt1 = pixRead("lucasta.150.jpg");
    pixt2 = pixThresholdTo4bpp(pixt1, 7, 1);
    box1 = boxCreate(73, 206, 140, 27);
    pixColorGrayCmap(pixt2, box1, L_PAINT_LIGHT, 130, 207, 43);
    pixDisplayWrite(pixt2, 1);
    pixPrintStreamInfo(stderr, pixt2, "One box added");

    box2 = boxCreate(255, 404, 197, 25);
    pixColorGrayCmap(pixt2, box2, L_PAINT_LIGHT, 230, 67, 119);
    pixDisplayWrite(pixt2, 1);
    pixPrintStreamInfo(stderr, pixt2, "Two boxes added");

    box3 = boxCreate(122, 756, 224, 22);
    pixColorGrayCmap(pixt2, box3, L_PAINT_DARK, 230, 67, 119);
    pixDisplayWrite(pixt2, 1);
    pixPrintStreamInfo(stderr, pixt2, "Three boxes added");

    box4 = boxCreate(11, 780, 147, 22);
    pixColorGrayCmap(pixt2, box4, L_PAINT_LIGHT, 70, 137, 229);
    pixDisplayWrite(pixt2, 1);
    pixPrintStreamInfo(stderr, pixt2, "Four boxes added");

    box5 = boxCreate(163, 605, 78, 22);
    pixColorGrayCmap(pixt2, box5, L_PAINT_LIGHT, 70, 137, 229);
    pixDisplayWrite(pixt2, 1);
    pixPrintStreamInfo(stderr, pixt2, "Five boxes added");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    boxDestroy(&box1);
    boxDestroy(&box2);
    boxDestroy(&box3);
    boxDestroy(&box4);
    boxDestroy(&box5);

    pixDisplayMultiple("/tmp/junk_write_display*");
    pixDestroy(&pixs);
    return 0;
}


