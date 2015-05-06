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
 * blendcmaptest.c
 *
 */

#include "allheaders.h"

#define  NX     4
#define  NY     5

#define  FADE_FRACTION    0.75

int main(int    argc,
         char **argv)
{
l_int32   i, j, sindex, wb, hb, ws, hs, delx, dely, x, y, y0;
PIX      *pixs, *pixb, *pixt0, *pixt1;
PIXCMAP  *cmap;

    pixs = pixRead("rabi.png");
    pixb = pixRead("weasel4.11c.png");
    pixDisplayWrite(NULL, -1);

        /* Fade the blender */
    pixcmapShiftIntensity(pixGetColormap(pixb), FADE_FRACTION);

        /* Downscale the input */
    wb = pixGetWidth(pixb);
    hb = pixGetHeight(pixb);
    pixt0 = pixScaleToGray4(pixs);

        /* Threshold to 5 levels, 4 bpp */
    ws = pixGetWidth(pixt0);
    hs = pixGetHeight(pixt0);
    pixt1 = pixThresholdTo4bpp(pixt0, 5, 1);
    pixDisplayWriteFormat(pixt1, 1, IFF_PNG);
    pixDisplayWrite(pixb, 1);
    cmap = pixGetColormap(pixt1);
    pixcmapWriteStream(stderr, cmap);

        /* Overwrite the white pixels (at sindex in pixt1) */
    pixcmapGetIndex(cmap, 255, 255, 255, &sindex);
    delx = ws / NX;
    dely = hs / NY;
    for (i = 0; i < NY; i++) {
        y = 20 + i * dely;
        if (y >= hs + hb)
            continue;
        for (j = 0; j < NX; j++) {
            x = 30 + j * delx;
            y0 = y;
            if (j & 1) {
                y0 = y + dely / 2;
                if (y0 >= hs + hb)
                    continue;
            }
            if (x >= ws + wb)
                continue;
            pixBlendCmap(pixt1, pixb, x, y0, sindex);
        }
    }
    pixDisplayWriteFormat(pixt1, 1, IFF_PNG);
    cmap = pixGetColormap(pixt1);
    pixcmapWriteStream(stderr, cmap);

    pixDisplayMultiple("/tmp/display/file*");

    pixDestroy(&pixs);
    pixDestroy(&pixb);
    pixDestroy(&pixt0);
    pixDestroy(&pixt1);
    return 0;
}

