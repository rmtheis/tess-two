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
 * equal_reg.c
 *
 *    Tests the pixEqual() function in many situations.
 *
 *    This also tests the quantization of grayscale and color
 *    images (to generate a colormapped image), and removal of
 *    the colormap to either RGB or grayscale.
 */

#include "allheaders.h"

    /* use this set */
#define   FEYN1            "feyn.tif"      /* 1 bpp */
#define   DREYFUS2         "dreyfus2.png"  /* 2 bpp cmapped */
#define   DREYFUS4         "dreyfus4.png"  /* 4 bpp cmapped */
#define   DREYFUS8         "dreyfus8.png"  /* 8 bpp cmapped */
#define   KAREN8           "karen8.jpg"    /* 8 bpp, not cmapped */
#define   MARGE32          "marge.jpg"     /* rgb */

int main(int    argc,
         char **argv)
{
l_int32      errorfound, same;
PIX         *pixs, *pixt1, *pixt2, *pixt3, *pixt4;
static char  mainName[] = "equal_reg";

    if (argc != 1)
        return ERROR_INT(" Syntax:  equal_reg", mainName, 1);

    errorfound = FALSE;
    pixs = pixRead(FEYN1);
    pixWrite("/tmp/junkfeyn.png", pixs, IFF_PNG);
    pixt1 = pixRead("/tmp/junkfeyn.png");
    pixEqual(pixs, pixt1, &same);
    if (same)
        L_INFO("equal for feyn1\n", mainName);
    else {
        L_INFO("FAILURE for equal for feyn1\n", mainName);
        errorfound = TRUE;
    }
    pixDestroy(&pixs);
    pixDestroy(&pixt1);

    pixs = pixRead(DREYFUS2);
    pixt1 = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    pixWrite("/tmp/junkdrey2-1.png", pixt1, IFF_PNG);
    pixt2 = pixRemoveColormap(pixs, REMOVE_CMAP_TO_FULL_COLOR);
    pixWrite("/tmp/junkdrey2-2.png", pixt2, IFF_PNG);
    pixt3 = pixOctreeQuantNumColors(pixt2, 64, 1);
    pixWrite("/tmp/junkdrey2-3.png", pixt3, IFF_PNG);
    pixt4 = pixConvertRGBToColormap(pixt2, 1);
    pixWrite("/tmp/junkdrey2-4.png", pixt4, IFF_PNG);
    pixEqual(pixs, pixt1, &same);
    if (same) {
        L_INFO("equal for pixt1 of dreyfus2\n", mainName);
    } else {
        L_INFO("FAILURE for pixt1 of dreyfus2\n", mainName);
        errorfound = TRUE;
    }
    pixEqual(pixs, pixt2, &same);
    if (same) {
        L_INFO("equal for pixt2 of dreyfus2\n", mainName);
    } else {
        L_INFO("FAILURE for pixt2 of dreyfus2\n", mainName);
        errorfound = TRUE;
    }
    pixEqual(pixs, pixt3, &same);
    if (same) {
        L_INFO("equal for pixt3 of dreyfus2\n", mainName);
    } else {
        L_INFO("FAILURE for pixt3 of dreyfus2\n", mainName);
        errorfound = TRUE;
    }
    pixEqual(pixs, pixt4, &same);
    if (same) {
        L_INFO("equal for pixt4 of dreyfus2\n", mainName);
    } else {
        L_INFO("FAILURE for pixt4 of dreyfus2\n", mainName);
        errorfound = TRUE;
    }
    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

    pixs = pixRead(DREYFUS4);
    pixt1 = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    pixWrite("/tmp/junkdrey4-1.png", pixt1, IFF_PNG);
    pixt2 = pixRemoveColormap(pixs, REMOVE_CMAP_TO_FULL_COLOR);
    pixWrite("/tmp/junkdrey4-2.png", pixt2, IFF_PNG);
    pixt3 = pixOctreeQuantNumColors(pixt2, 256, 1);
    pixWrite("/tmp/junkdrey4-3.png", pixt3, IFF_PNG);
    pixt4 = pixConvertRGBToColormap(pixt2, 1);
    pixWrite("/tmp/junkdrey4-4.png", pixt4, IFF_PNG);
    pixEqual(pixs, pixt1, &same);
    if (same) {
        L_INFO("equal for pixt1 of dreyfus4\n", mainName);
    } else {
        L_INFO("FAILURE for pixt1 of dreyfus4\n", mainName);
        errorfound = TRUE;
    }
    pixEqual(pixs, pixt2, &same);
    if (same) {
        L_INFO("equal for pixt2 of dreyfus4\n", mainName);
    } else {
        L_INFO("FAILURE for pixt2 of dreyfus4\n", mainName);
        errorfound = TRUE;
    }
    pixEqual(pixs, pixt3, &same);
    if (same) {
        L_INFO("equal for pixt3 of dreyfus4\n", mainName);
    } else {
        L_INFO("FAILURE for pixt3 of dreyfus4\n", mainName);
        errorfound = TRUE;
    }
    pixEqual(pixs, pixt4, &same);
    if (same) {
        L_INFO("equal for pixt4 of dreyfus4\n", mainName);
    } else {
        L_INFO("FAILURE for pixt4 of dreyfus4\n", mainName);
        errorfound = TRUE;
    }
    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

    pixs = pixRead(DREYFUS8);
    pixt1 = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    pixWrite("/tmp/junkdrey8-1.png", pixt1, IFF_PNG);
    pixt2 = pixRemoveColormap(pixs, REMOVE_CMAP_TO_FULL_COLOR);
    pixWrite("/tmp/junkdrey8-2.png", pixt2, IFF_PNG);
    pixt3 = pixConvertRGBToColormap(pixt2, 1);
    pixWrite("/tmp/junkdrey8-3.png", pixt3, IFF_PNG);
    pixEqual(pixs, pixt1, &same);
    if (same) {
        L_INFO("equal for pixt1 of dreyfus8\n", mainName);
    } else {
        L_INFO("FAILURE for pixt1 of dreyfus8\n", mainName);
        errorfound = TRUE;
    }
    pixEqual(pixs, pixt2, &same);
    if (same) {
        L_INFO("equal for pixt2 of dreyfus8\n", mainName);
    } else {
        L_INFO("FAILURE for pixt2 of dreyfus8\n", mainName);
        errorfound = TRUE;
    }
    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);

    pixs = pixRead(KAREN8);
    pixt1 = pixThresholdTo4bpp(pixs, 16, 1);
    pixWrite("/tmp/junkkar8-1.png", pixt1, IFF_PNG);
    pixt2 = pixRemoveColormap(pixt1, REMOVE_CMAP_BASED_ON_SRC);
    pixWrite("/tmp/junkkar8-2.png", pixt2, IFF_PNG);
    pixt3 = pixRemoveColormap(pixt1, REMOVE_CMAP_TO_FULL_COLOR);
    pixWrite("/tmp/junkkar8-3.png", pixt3, IFF_PNG);
    pixt4 = pixConvertRGBToColormap(pixt3, 1);
    pixEqual(pixt1, pixt2, &same);
    if (same) {
        L_INFO("equal for pixt2 of karen8\n", mainName);
    } else {
        L_INFO("FAILURE for pixt2 of karen8\n", mainName);
        errorfound = TRUE;
    }
    pixEqual(pixt1, pixt3, &same);
    if (same) {
        L_INFO("equal for pixt3 of karen8\n", mainName);
    } else {
        L_INFO("FAILURE for pixt3 of karen8\n", mainName);
        errorfound = TRUE;
    }
    pixEqual(pixt1, pixt4, &same);
    if (same) {
        L_INFO("equal for pixt4 of karen8\n", mainName);
    } else {
        L_INFO("FAILURE for pixt4 of karen8\n", mainName);
        errorfound = TRUE;
    }
    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

    pixs = pixRead(MARGE32);
    pixt1 = pixOctreeQuantNumColors(pixs, 32, 0);
    pixWrite("/tmp/junkmarge8-1.png", pixt1, IFF_PNG);
    pixt2 = pixRemoveColormap(pixt1, REMOVE_CMAP_TO_FULL_COLOR);
    pixWrite("/tmp/junkmarge8-2.png", pixt2, IFF_PNG);
    pixt3 = pixConvertRGBToColormap(pixt2, 1);
    pixWrite("/tmp/junkmarge8-3.png", pixt3, IFF_PNG);
    pixt4 = pixOctreeQuantNumColors(pixt2, 64, 0);
    pixWrite("/tmp/junkmarge8-4.png", pixt4, IFF_PNG);
    pixEqual(pixt1, pixt2, &same);
    if (same) {
        L_INFO("equal for pixt2 of marge32\n", mainName);
    } else {
        L_INFO("FAILURE for pixt2 of marge32\n", mainName);
        errorfound = TRUE;
    }
    pixEqual(pixt1, pixt3, &same);
    if (same) {
        L_INFO("equal for pixt3 of marge32\n", mainName);
    } else {
        L_INFO("FAILURE for pixt3 of marge32\n", mainName);
        errorfound = TRUE;
    }
    pixEqual(pixt1, pixt4, &same);
    if (same) {
        L_INFO("equal for pixt4 of marge32\n", mainName);
    } else {
        L_INFO("FAILURE for pixt4 of marge32\n", mainName);
        errorfound = TRUE;
    }
    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

    if (errorfound)
        L_INFO("FAILURE in processing this test\n", mainName);
    else
        L_INFO("SUCCESS in processing this test\n", mainName);
    return 0;
}

