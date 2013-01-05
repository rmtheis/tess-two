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
 * blendcmaptest.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define  NX     4
#define  NY     5

#define  FADE_FRACTION    0.75

main(int    argc,
     char **argv)
{
l_int32    i, j, sindex, wb, hb, ws, hs, delx, dely, x, y, y0;
PIX       *pixs, *pixb, *pixt0, *pixt1;
PIXCMAP   *cmap;
static char   mainName[] = "blendcmaptest";

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

    pixDisplayMultiple("/tmp/junk_write_display*");

    pixDestroy(&pixs);
    pixDestroy(&pixb);
    pixDestroy(&pixt0);
    pixDestroy(&pixt1);
    return 0;
}

