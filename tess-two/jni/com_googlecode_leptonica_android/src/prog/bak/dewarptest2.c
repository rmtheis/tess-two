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

#define  NORMALIZE     0

l_int32 main(int    argc,
             char **argv)
{
l_int32      pageno;
L_DEWARP    *dew1;
L_DEWARPA   *dewa;
PIX         *pixs, *pixn, *pixg, *pixb;
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

    dewa = dewarpaCreate(40, 30, 1, 6, 50);

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
    dewarpBuildModel(dew1, "/tmp/dewarp_model1.pdf");
    dewarpaApplyDisparity(dewa, pageno, pixg, "/tmp/dewarp_apply1.pdf");

    dewarpaDestroy(&dewa);
    pixDestroy(&pixs);
    pixDestroy(&pixg);
    pixDestroy(&pixb);
    return 0;
}
