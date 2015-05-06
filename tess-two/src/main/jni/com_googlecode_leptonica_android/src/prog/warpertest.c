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
 *  warpertest.c
 *
 *    Tests stereoscopic warp and associated shear and stretching functions.
 *    Uses gthumb for visually identifying problems.
 */

#include "allheaders.h"

static const char  *opstr[3] = {"", "interpolated", "sampled"};
static const char  *dirstr[3] = {"", "to left", "to right"};

#define  RUN_WARP                 1
#define  RUN_QUAD_VERT_SHEAR      0
#define  RUN_LIN_HORIZ_STRETCH    0
#define  RUN_QUAD_HORIZ_STRETCH   0
#define  RUN_HORIZ_SHEAR          0
#define  RUN_VERT_SHEAR           0

int main(int    argc,
         char **argv)
{
char       buf[256];
l_int32    w, h, i, j, k, index, op, dir, stretch;
l_float32  del, angle, angledeg;
BOX       *box;
L_BMF     *bmf;
PIX       *pixs, *pixt, *pixt2, *pixd;
static char  mainName[] = "warpertest";

    if (argc != 1)
        return ERROR_INT("syntax: warpertest", mainName, 1);

    /* --------   Stereoscopic warping --------------*/
#if RUN_WARP
    pixs = pixRead("german.png");
    pixGetDimensions(pixs, &w, &h, NULL);
    for (i = 0; i < 50; i++) {  /* need to test > 2 widths ! */
        j = 7 * i;
        box = boxCreate(0, 0, w - j, h - j);
        pixt = pixClipRectangle(pixs, box, NULL);
        pixd = pixWarpStereoscopic(pixt, 15, 22, 8, 30, -20, 1);
        snprintf(buf, sizeof(buf), "/tmp/junkpixw.%02d.jpg", i);
        pixSetChromaSampling(pixd, 0);
        pixWrite(buf, pixd, IFF_JFIF_JPEG);
        pixDestroy(&pixd);
        pixDestroy(&pixt);
        boxDestroy(&box);
    }
    pixDestroy(&pixs);
    pixDisplayMultiple("/tmp/junkpixw*.jpg");
#endif

    /* --------   Quadratic Vertical Shear  --------------*/
#if RUN_QUAD_VERT_SHEAR
    pixs = pixCreate(501, 501, 32);
    pixGetDimensions(pixs, &w, &h, NULL);
    pixSetAll(pixs);
    pixRenderLineArb(pixs, 0, 30, 500, 30, 5, 0, 0, 255);
    pixRenderLineArb(pixs, 0, 110, 500, 110, 5, 0, 255, 0);
    pixRenderLineArb(pixs, 0, 190, 500, 190, 5, 0, 255, 255);
    pixRenderLineArb(pixs, 0, 270, 500, 270, 5, 255, 0, 0);
    pixRenderLineArb(pixs, 0, 360, 500, 360, 5, 255, 0, 255);
    pixRenderLineArb(pixs, 0, 450, 500, 450, 5, 255, 255, 0);
    bmf = bmfCreate("./fonts", 6);
    for (i = 0; i < 50; i++) {
        j = 3 * i;
        dir = ((i / 2) & 1) ? L_WARP_TO_RIGHT : L_WARP_TO_LEFT;
        op = (i & 1) ? L_INTERPOLATED : L_SAMPLED;
        Box *box = boxCreate(0, 0, w - j, h - j);
        pixt = pixClipRectangle(pixs, box, NULL);
        pixt2 = pixQuadraticVShear(pixt, dir, 60, -20, op, L_BRING_IN_WHITE);

        snprintf(buf, sizeof(buf), "%s, %s", dirstr[dir], opstr[op]);
        pixd = pixAddSingleTextblock(pixt2, bmf, buf, 0xff000000,
                                     L_ADD_BELOW, 0);
        snprintf(buf, sizeof(buf), "/tmp/junkpixvs.%02d.png", i);
        pixWrite(buf, pixd, IFF_PNG);
        pixDestroy(&pixd);
        pixDestroy(&pixt);
        pixDestroy(&pixt2);
        boxDestroy(&box);
    }
    pixDestroy(&pixs);
    bmfDestroy(&bmf);
    pixDisplayMultiple("/tmp/junkpixvs*.png");
#endif

    /* --------  Linear Horizontal stretching  --------------*/
#if RUN_LIN_HORIZ_STRETCH
    pixs = pixRead("german.png");
    bmf = bmfCreate("./fonts", 6);
    for (k = 0; k < 2; k++) {
        for (i = 0; i < 25; i++) {
            index = 25 * k + i;
            stretch = 10 + 4 * i;
            if (k == 0) stretch = -stretch;
            dir = (k == 1) ? L_WARP_TO_RIGHT : L_WARP_TO_LEFT;
            op = (i & 1) ? L_INTERPOLATED : L_SAMPLED;
            pixt = pixStretchHorizontal(pixs, dir, L_LINEAR_WARP,
                                        stretch, op, L_BRING_IN_WHITE);
            snprintf(buf, sizeof(buf), "%s, %s", dirstr[dir], opstr[op]);
            pixd = pixAddSingleTextblock(pixt, bmf, buf, 0xff000000,
                                         L_ADD_BELOW, 0);
            snprintf(buf, sizeof(buf), "/tmp/junkpixlhs.%02d.jpg", index);
            pixWrite(buf, pixd, IFF_JFIF_JPEG);
            pixDestroy(&pixd);
            pixDestroy(&pixt);
        }
    }

    pixDestroy(&pixs);
    bmfDestroy(&bmf);
    pixDisplayMultiple("/tmp/junkpixlhs*.jpg");
#endif

    /* --------  Quadratic Horizontal stretching  --------------*/
#if RUN_QUAD_HORIZ_STRETCH
    pixs = pixRead("german.png");
    bmf = bmfCreate("./fonts", 6);
    for (k = 0; k < 2; k++) {
        for (i = 0; i < 25; i++) {
            index = 25 * k + i;
            stretch = 10 + 4 * i;
            if (k == 0) stretch = -stretch;
            dir = (k == 1) ? L_WARP_TO_RIGHT : L_WARP_TO_LEFT;
            op = (i & 1) ? L_INTERPOLATED : L_SAMPLED;
            pixt = pixStretchHorizontal(pixs, dir, L_QUADRATIC_WARP,
                                        stretch, op, L_BRING_IN_WHITE);
            snprintf(buf, sizeof(buf), "%s, %s", dirstr[dir], opstr[op]);
            pixd = pixAddSingleTextblock(pixt, bmf, buf, 0xff000000,
                                         L_ADD_BELOW, 0);
            snprintf(buf, sizeof(buf), "/tmp/junkpixqhs.%02d.jpg", index);
            pixWrite(buf, pixd, IFF_JFIF_JPEG);
            pixDestroy(&pixd);
            pixDestroy(&pixt);
        }
    }

    pixDestroy(&pixs);
    bmfDestroy(&bmf);
    pixDisplayMultiple("/tmp/junkpixqhs*.jpg");
#endif

    /* --------  Horizontal Shear --------------*/
#if RUN_HORIZ_SHEAR
    pixs = pixRead("german.png");
    pixGetDimensions(pixs, &w, &h, NULL);
    bmf = bmfCreate("./fonts", 6);
    for (i = 0; i < 25; i++) {
        del = 0.2 / 12.;
        angle = -0.2 + (i - (i & 1)) * del;
        angledeg = 180. * angle / 3.14159265;
        op = (i & 1) ? L_INTERPOLATED : L_SAMPLED;
        if (op == L_SAMPLED)
            pixt = pixHShear(NULL, pixs, h / 2, angle, L_BRING_IN_WHITE);
        else
            pixt = pixHShearLI(pixs, h / 2, angle, L_BRING_IN_WHITE);
        snprintf(buf, sizeof(buf), "%6.2f degree, %s", angledeg, opstr[op]);
        pixd = pixAddSingleTextblock(pixt, bmf, buf, 0xff000000,
                                     L_ADD_BELOW, 0);
        snprintf(buf, sizeof(buf), "/tmp/junkpixsh.%02d.jpg", i);
        pixWrite(buf, pixd, IFF_JFIF_JPEG);
        pixDestroy(&pixd);
        pixDestroy(&pixt);
    }

    pixDestroy(&pixs);
    bmfDestroy(&bmf);
    pixDisplayMultiple("/tmp/junkpixsh*.jpg");
#endif

    /* --------  Vertical Shear --------------*/
#if RUN_VERT_SHEAR
    pixs = pixRead("german.png");
    pixGetDimensions(pixs, &w, &h, NULL);
    bmf = bmfCreate("./fonts", 6);
    for (i = 0; i < 25; i++) {
        del = 0.2 / 12.;
        angle = -0.2 + (i - (i & 1)) * del;
        angledeg = 180. * angle / 3.14159265;
        op = (i & 1) ? L_INTERPOLATED : L_SAMPLED;
        if (op == L_SAMPLED)
            pixt = pixVShear(NULL, pixs, w / 2, angle, L_BRING_IN_WHITE);
        else
            pixt = pixVShearLI(pixs, w / 2, angle, L_BRING_IN_WHITE);
        snprintf(buf, sizeof(buf), "%6.2f degree, %s", angledeg, opstr[op]);
        pixd = pixAddSingleTextblock(pixt, bmf, buf, 0xff000000,
                                     L_ADD_BELOW, 0);
        snprintf(buf, sizeof(buf), "/tmp/junkpixsv.%02d.jpg", i);
        pixWrite(buf, pixd, IFF_JFIF_JPEG);
        pixDestroy(&pixd);
        pixDestroy(&pixt);
    }

    pixDestroy(&pixs);
    bmfDestroy(&bmf);
    pixDisplayMultiple("/tmp/junkpixsv*.jpg");
#endif

    return 0;
}
