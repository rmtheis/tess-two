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
 * blend_reg.c
 *
 *   Regression test for these functions:
 *       pixBlendGray()
 *       pixBlendGrayAdapt()
 *       pixBlendColor()
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define  DISPLAY   0

void GrayBlend(PIX  *pixs, PIX  *pixb, l_int32 op, l_float32 fract);
void AdaptiveGrayBlend(PIX  *pixs, PIX  *pixb, l_float32 fract);
void ColorBlend(PIX  *pixs, PIX  *pixb, l_float32 fract);
PIX *MakeGrayWash(l_int32 w, l_int32 h);
PIX *MakeColorWash(l_int32 w, l_int32 h, l_int32 color);

main(int    argc,
     char **argv)
{
PIX         *pixs, *pixg, *pixc, *pixt, *pixd;
PIXA        *pixa;
static char  mainName[] = "blend_reg";

    pixg = pixRead("blender8.png");
    pixt = pixRead("weasel4.11c.png");
    pixc = pixRemoveColormap(pixt, REMOVE_CMAP_TO_FULL_COLOR);
    pixDisplayWrite(NULL, -1);
    pixa = pixaCreate(0);

#if 1
        /* Gray blend (straight) */
    pixs = pixRead("test24.jpg");
    GrayBlend(pixs, pixg, L_BLEND_GRAY, 0.3);
    pixSaveTiled(pixs, pixa, 1, 1, 20, 32);
    pixDisplayWithTitle(pixs, 100, 100, NULL, DISPLAY);
    pixDestroy(&pixs);
    pixs = pixRead("marge.jpg");
    GrayBlend(pixs, pixg, L_BLEND_GRAY, 0.2);
    pixSaveTiled(pixs, pixa, 1, 1, 20, 32);
    pixDisplayWithTitle(pixs, 100, 100, NULL, DISPLAY);
    pixDestroy(&pixs);
    pixs = pixRead("marge.jpg");
    pixt = pixConvertRGBToLuminance(pixs);
    GrayBlend(pixt, pixg, L_BLEND_GRAY, 0.2);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 32);
    pixDisplayWithTitle(pixt, 100, 100, NULL, DISPLAY);
    pixDestroy(&pixs);
    pixDestroy(&pixt);
#endif

#if 1
        /* Gray blend (inverse) */
    pixs = pixRead("test24.jpg");
    GrayBlend(pixs, pixg, L_BLEND_GRAY_WITH_INVERSE, 0.6);
    pixSaveTiled(pixs, pixa, 1, 1, 20, 32);
    pixDisplayWithTitle(pixs, 100, 100, NULL, DISPLAY);
    pixDestroy(&pixs);
    pixs = pixRead("marge.jpg");
    GrayBlend(pixs, pixg, L_BLEND_GRAY_WITH_INVERSE, 0.6);
    pixSaveTiled(pixs, pixa, 1, 1, 20, 32);
    pixDisplayWithTitle(pixs, 100, 100, NULL, DISPLAY);
    pixDestroy(&pixs);
    pixs = pixRead("marge.jpg");
    pixt = pixConvertRGBToLuminance(pixs);
    GrayBlend(pixt, pixg, L_BLEND_GRAY_WITH_INVERSE, 0.6);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 32);
    pixDisplayWithTitle(pixt, 100, 100, NULL, DISPLAY);
    pixDestroy(&pixs);
    pixDestroy(&pixt);
    pixs = MakeGrayWash(1000, 120);
    GrayBlend(pixs, pixg, L_BLEND_GRAY_WITH_INVERSE, 0.3);
    pixSaveTiled(pixs, pixa, 1, 1, 20, 32);
    pixDisplayWithTitle(pixs, 200, 100, NULL, DISPLAY);
    pixDestroy(&pixs);
    pixs = MakeColorWash(1000, 120, COLOR_RED);
    GrayBlend(pixs, pixg, L_BLEND_GRAY_WITH_INVERSE, 1.0);
    pixSaveTiled(pixs, pixa, 1, 1, 20, 32);
    pixDisplayWithTitle(pixs, 200, 100, NULL, DISPLAY);
    pixDestroy(&pixs);
#endif

#if 1
        /* Adaptive gray blend */
    pixs = pixRead("test24.jpg");
    AdaptiveGrayBlend(pixs, pixg, 0.8);
    pixSaveTiled(pixs, pixa, 1, 1, 20, 32);
    pixDisplayWithTitle(pixs, 200, 100, NULL, DISPLAY);
    pixDestroy(&pixs);
    pixs = pixRead("marge.jpg");
    AdaptiveGrayBlend(pixs, pixg, 0.8);
    pixSaveTiled(pixs, pixa, 1, 1, 20, 32);
    pixDisplayWithTitle(pixs, 200, 100, NULL, DISPLAY);
    pixt = pixConvertRGBToLuminance(pixs);
    AdaptiveGrayBlend(pixt, pixg, 0.1);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 32);
    pixDisplayWithTitle(pixt, 200, 100, NULL, DISPLAY);
    pixDestroy(&pixs);
    pixDestroy(&pixt);
    pixs = MakeGrayWash(1000, 120);
    AdaptiveGrayBlend(pixs, pixg, 0.3);
    pixSaveTiled(pixs, pixa, 1, 1, 20, 32);
    pixDisplayWithTitle(pixs, 200, 100, NULL, DISPLAY);
    pixDestroy(&pixs);
    pixs = MakeColorWash(1000, 120, COLOR_RED);
    AdaptiveGrayBlend(pixs, pixg, 0.5);
    pixSaveTiled(pixs, pixa, 1, 1, 20, 32);
    pixDisplayWithTitle(pixs, 200, 100, NULL, DISPLAY);
    pixDestroy(&pixs);
#endif

#if 1
        /* Color blend */
    pixs = pixRead("test24.jpg");
    ColorBlend(pixs, pixc, 0.3);
    pixSaveTiled(pixs, pixa, 1, 1, 20, 32);
    pixDisplayWithTitle(pixs, 300, 100, NULL, DISPLAY);
    pixDestroy(&pixs);
    pixs = pixRead("marge.jpg");
    ColorBlend(pixs, pixc, 0.30);
    pixSaveTiled(pixs, pixa, 1, 1, 20, 32);
    pixDisplayWithTitle(pixs, 300, 100, NULL, DISPLAY);
    pixDestroy(&pixs);
    pixs = pixRead("marge.jpg");
    ColorBlend(pixs, pixc, 0.15);
    pixSaveTiled(pixs, pixa, 1, 0, 20, 32);
    pixDisplayWithTitle(pixs, 300, 100, NULL, DISPLAY);
    pixDestroy(&pixs);
#endif

        /* Display results */
    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/junkblend.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixDisplayMultiple("/tmp/junk_write_display*");

    pixDestroy(&pixg);
    pixDestroy(&pixt);
    pixDestroy(&pixc);
    return 0;
}


void
GrayBlend(PIX       *pixs,
          PIX       *pixb,
          l_int32    op,
          l_float32  fract)
{
l_int32   i, j, wb, hb, ws, hs, delx, dely, x, y;

    pixGetDimensions(pixs, &ws, &hs, NULL);
    pixGetDimensions(pixb, &wb, &hb, NULL);
    delx = wb + 30;
    dely = hb + 25;
    x = 200;
    y = 300;
    for (i = 0; i < 20; i++) {
        y = 20 + i * dely;
        if (y >= hs - hb)
            continue;
        for (j = 0; j < 20; j++) {
            x = 30 + j * delx;
            if (x >= ws - wb)
                continue;
            pixBlendGray(pixs, pixs, pixb, x, y, fract, op, 1, 255);
        }
    }
    pixDisplayWriteFormat(pixs, 1, IFF_PNG);
}


void
AdaptiveGrayBlend(PIX       *pixs,
                  PIX       *pixb,
                  l_float32  fract)
{
l_int32   i, j, wb, hb, ws, hs, delx, dely, x, y;

    pixGetDimensions(pixs, &ws, &hs, NULL);
    pixGetDimensions(pixb, &wb, &hb, NULL);
    delx = wb + 30;
    dely = hb + 25;
    x = 200;
    y = 300;
    for (i = 0; i < 20; i++) {
        y = 20 + i * dely;
        if (y >= hs - hb)
            continue;
        for (j = 0; j < 20; j++) {
            x = 30 + j * delx;
            if (x >= ws - wb)
                continue;
            pixBlendGrayAdapt(pixs, pixs, pixb, x, y, fract, 80);
        }
    }
    pixDisplayWriteFormat(pixs, 1, IFF_PNG);
}


void
ColorBlend(PIX       *pixs,
           PIX       *pixb,
           l_float32  fract)
{
l_int32   i, j, wb, hb, ws, hs, delx, dely, x, y;

    pixGetDimensions(pixs, &ws, &hs, NULL);
    pixGetDimensions(pixb, &wb, &hb, NULL);
    delx = wb + 30;
    dely = hb + 25;
    x = 200;
    y = 300;
    for (i = 0; i < 20; i++) {
        y = 20 + i * dely;
        if (y >= hs - hb)
            continue;
        for (j = 0; j < 20; j++) {
            x = 30 + j * delx;
            if (x >= ws - wb)
                continue;
            pixBlendColor(pixs, pixs, pixb, x, y, fract, 1, 255);
        }
    }
    pixDisplayWriteFormat(pixs, 1, IFF_PNG);
}


PIX *
MakeGrayWash(l_int32  w,
             l_int32  h)
{
l_int32    i, j, wpl, val;
l_uint32  *data, *line;
PIX       *pixd;

    pixd = pixCreate(w, h, 8);
    data = pixGetData(pixd);
    wpl = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < w; j++) {
            val = (j * 255) / w;
            SET_DATA_BYTE(line, j, val);
        }
    }
    return pixd;
}


PIX *
MakeColorWash(l_int32  w,
              l_int32  h,
              l_int32  color)
{
l_int32    i, j, wpl;
l_uint32   val;
l_uint32  *data, *line;
PIX       *pixd;

    pixd = pixCreate(w, h, 32);
    data = pixGetData(pixd);
    wpl = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < w; j++) {
            if (color == COLOR_RED)
                val = ((j * 255) / w) << L_GREEN_SHIFT |
                      ((j * 255) / w) << L_BLUE_SHIFT |
                      255 << L_RED_SHIFT;
            else if (color == COLOR_GREEN)
                val = ((j * 255) / w) << L_RED_SHIFT |
                      ((j * 255) / w) << L_BLUE_SHIFT |
                      255 << L_GREEN_SHIFT;
            else
                val = ((j * 255) / w) << L_RED_SHIFT |
                      ((j * 255) / w) << L_GREEN_SHIFT |
                      255 << L_BLUE_SHIFT;
            line[j] = val;
        }
    }
    return pixd;
}

