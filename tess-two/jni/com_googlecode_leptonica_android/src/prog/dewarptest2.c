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
 *   dewarptest2.c
 *
 *   This runs the basic functions for a single page.  It can be used
 *   to debug the disparity model-building.
 *
 *     dewarptest2 [image pageno]
 *
 *   Default image is cat-35.jpg.
 *   Others are 1555-7.jpg, etc.
 */

#include "allheaders.h"

#define  NORMALIZE     1

l_int32 main(int    argc,
             char **argv)
{
l_int32      pageno;
L_DEWARP    *dew1;
L_DEWARPA   *dewa;
PIX         *pixs, *pixn, *pixg, *pixb, *pixd;
static char  mainName[] = "dewarptest2";

    if (argc != 1 && argc != 3)
        return ERROR_INT("Syntax: dewarptest2 [image pageno]", mainName, 1);

    if (argc == 1) {
        pixs = pixRead("cat-35.jpg");
        pageno = 35;
    }
    else {
        pixs = pixRead(argv[1]);
        pageno = atoi(argv[2]);
    }
    if (!pixs)
        return ERROR_INT("image not read", mainName, 1);

#if 1
    dewarpSinglePage(pixs, 100, 1, 1, &pixd, NULL, 1);
    pixDisplay(pixd, 100, 100);

#else

    dewa = dewarpaCreate(40, 30, 1, 6, 50);
    dewarpaUseBothArrays(dewa, 1);

#if NORMALIZE
        /* Normalize for varying background and binarize */
    pixn = pixBackgroundNormSimple(pixs, NULL, NULL);
    pixg = pixConvertRGBToGray(pixn, 0.5, 0.3, 0.2);
    pixb = pixThresholdToBinary(pixg, 130);
    pixDestroy(&pixn);
#else
        /* Don't normalize; just threshold and clean edges */
    pixg = pixConvertTo8(pixs, 0);
    pixb = pixThresholdToBinary(pixg, 100);
    pixSetOrClearBorder(pixb, 30, 30, 40, 40, PIX_CLR);
#endif

        /* Run the basic functions */
    dew1 = dewarpCreate(pixb, pageno);
    dewarpaInsertDewarp(dewa, dew1);
    dewarpBuildPageModel(dew1, "/tmp/dewarp_model1.pdf");
    dewarpaApplyDisparity(dewa, pageno, pixg, -1, 0, 0, &pixd,
                          "/tmp/dewarp_apply1.pdf");

    dewarpaDestroy(&dewa);
    pixDestroy(&pixg);
    pixDestroy(&pixb);

#endif

    pixDestroy(&pixs);
    pixDestroy(&pixd);
    return 0;
}
