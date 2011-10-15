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
 *   warper_reg.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allheaders.h"

static void DisplayResult(PIXA *pixac, PIX **ppixd, l_int32 newline);
static void DisplayCaptcha(PIXA *pixac, PIX *pixs, l_int32 nterms,
                           l_uint32 seed, l_int32 newline);

static const l_int32 size = 4;
static const l_float32 xmag[] = {3.0, 4.0, 5.0, 7.0};
static const l_float32 ymag[] = {5.0, 6.0, 8.0, 10.0};
static const l_float32 xfreq[] = {0.11, 0.10, 0.10, 0.12};
static const l_float32 yfreq[] = {0.11, 0.13, 0.13, 0.15};
static const l_int32 nx[] = {4, 3, 2, 1};
static const l_int32 ny[] = {4, 3, 2, 1};

static l_int32 count = 0;

main(int    argc,
     char **argv)
{
char     namebuf[256];
l_int32  i, k, newline, success, display;
FILE    *fp;
PIX     *pixs, *pixt, *pixg, *pixd;
PIXA    *pixac;

    if (regTestSetup(argc, argv, &fp, &display, &success, NULL))
              return 1;

    pixs = pixRead("feyn-word.tif");
    pixt = pixAddBorder(pixs, 25, 0);
    pixg = pixConvertTo8(pixt, 0);

    for (k = 0; k < size; k++) {
        pixac = pixaCreate(0);
        for (i = 0; i < 50; i++) {
            pixd = pixRandomHarmonicWarp(pixg, xmag[k], ymag[k], xfreq[k],
                                         yfreq[k], nx[k], ny[k], 7 * i, 255);
            newline = (i % 10 == 0) ? 1 : 0;
            DisplayResult(pixac, &pixd, newline);
        }
        pixd = pixaDisplay(pixac, 0, 0);

        snprintf(namebuf, 240, "/tmp/warp.%d.png", count);
        pixWrite(namebuf, pixd, IFF_PNG);
        regTestCheckFile(fp, argv, namebuf, count++, &success);
        pixDisplayWithTitle(pixd, 100, 100, NULL, display);
        pixaDestroy(&pixac);
        pixDestroy(&pixd);
    }

    pixDestroy(&pixt);
    pixDestroy(&pixg);

    for (k = 1; k <= 4; k++) {
        pixac = pixaCreate(0);
        for (i = 0; i < 50; i++) {
            newline = (i % 10 == 0) ? 1 : 0;
            DisplayCaptcha(pixac, pixs, k, 7 * i, newline);
        }
        pixd = pixaDisplay(pixac, 0, 0);
        snprintf(namebuf, 240, "/tmp/warp.%d.png", count);
        pixWrite(namebuf, pixd, IFF_PNG);
        regTestCheckFile(fp, argv, namebuf, count++, &success);
        pixDisplayWithTitle(pixd, 100, 100, NULL, display);
        pixaDestroy(&pixac);
        pixDestroy(&pixd);
    }

    pixDestroy(&pixs);
    regTestCleanup(argc, argv, fp, success, NULL);
    return 0;
}


static void
DisplayResult(PIXA    *pixac,
              PIX    **ppixd,
              l_int32  newline)
{
l_uint32  color;
PIX      *pixt;

    color = 0;
    color = ((rand() >> 16) & 0xff) << L_RED_SHIFT |
            ((rand() >> 16) & 0xff) << L_GREEN_SHIFT |
            ((rand() >> 16) & 0xff) << L_BLUE_SHIFT;
    pixt = pixColorizeGray(*ppixd, color, 0);
    pixSaveTiled(pixt, pixac, 1, newline, 20, 32);
    pixDestroy(&pixt);
    pixDestroy(ppixd);
    return;
}


static void
DisplayCaptcha(PIXA    *pixac,
              PIX      *pixs,
              l_int32   nterms,
              l_uint32  seed,
              l_int32   newline)
{
l_uint32  color;
PIX      *pixd;

    color = 0;
    color = ((rand() >> 16) & 0xff) << L_RED_SHIFT |
            ((rand() >> 16) & 0xff) << L_GREEN_SHIFT |
            ((rand() >> 16) & 0xff) << L_BLUE_SHIFT;
    pixd = pixSimpleCaptcha(pixs, 25, nterms, seed, color, 0);
    pixSaveTiled(pixd, pixac, 1, newline, 20, 32);
    pixDestroy(&pixd);
    return;
}


