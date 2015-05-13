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
 *   dewarptest1.c
 *
 *   This exercise functions in dewarp.c for dewarping based on lines
 *   of horizontal text.  It also creates a 24-image pdf of steps
 *   in the process.
 */

#include "allheaders.h"

#define   DO_QUAD     1
#define   DO_CUBIC    0
#define   DO_QUARTIC  0

l_int32 main(int    argc,
             char **argv)
{
L_DEWARP   *dew1, *dew2;
L_DEWARPA  *dewa;
PIX        *pixs, *pixn, *pixg, *pixb, *pixd, *pixt1, *pixt2;
PIX        *pixs2, *pixn2, *pixg2, *pixb2, *pixd2;

/*    pixs = pixRead("1555-7.jpg"); */
    pixs = pixRead("cat-35.jpg");
/*    pixs = pixRead("cat-10.jpg"); */

        /* Normalize for varying background and binarize */
    pixn = pixBackgroundNormSimple(pixs, NULL, NULL);
    pixg = pixConvertRGBToGray(pixn, 0.5, 0.3, 0.2);
    pixb = pixThresholdToBinary(pixg, 130);

        /* Run the basic functions */
    dewa = dewarpaCreate(2, 30, 1, 10, 30);
    dewarpaUseBothArrays(dewa, 1);
    dew1 = dewarpCreate(pixb, 35);
    dewarpaInsertDewarp(dewa, dew1);
    dewarpBuildPageModel(dew1, "/tmp/lept/dewarp_model1.pdf");
    dewarpaApplyDisparity(dewa, 35, pixg, 200, 0, 0, &pixd,
                          "/tmp/lept/dewarp_apply1.pdf");

         /* Write out some of the files to be imaged */
    lept_mkdir("lept");
    lept_rmdir("dewtest");
    lept_mkdir("dewtest");
    pixWrite("/tmp/dewtest/001.jpg", pixs, IFF_JFIF_JPEG);
    pixWrite("/tmp/dewtest/002.jpg", pixn, IFF_JFIF_JPEG);
    pixWrite("/tmp/dewtest/003.jpg", pixg, IFF_JFIF_JPEG);
    pixWrite("/tmp/dewtest/004.png", pixb, IFF_TIFF_G4);
    pixWrite("/tmp/dewtest/005.jpg", pixd, IFF_JFIF_JPEG);
    pixt1 = pixRead("/tmp/dewmod/0020.png");
    pixWrite("/tmp/dewtest/006.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixt1 = pixRead("/tmp/dewmod/0030.png");
    pixWrite("/tmp/dewtest/007.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixt1 = pixRead("/tmp/dewmod/0060.png");
    pixWrite("/tmp/dewtest/008.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixt1 = pixRead("/tmp/dewmod/0070.png");
    pixWrite("/tmp/dewtest/009.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixt1 = pixRead("/tmp/dewapply/002.png");
    pixWrite("/tmp/dewtest/010.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixt1 = pixRead("/tmp/dewapply/003.png");
    pixWrite("/tmp/dewtest/011.png", pixt1, IFF_PNG);
    pixt2 = pixThresholdToBinary(pixt1, 130);
    pixWrite("/tmp/dewtest/012.png", pixt2, IFF_TIFF_G4);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixt1 = pixRead("/tmp/dewmod/0041.png");
    pixWrite("/tmp/dewtest/013.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixt1 = pixRead("/tmp/dewmod/0042.png");
    pixWrite("/tmp/dewtest/014.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixt1 = pixRead("/tmp/dewmod/0051.png");
    pixWrite("/tmp/dewtest/015.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixt1 = pixRead("/tmp/dewmod/0052.png");
    pixWrite("/tmp/dewtest/016.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);

        /* Normalize another image, that may not have enough textlines
         * to build an accurate model */
/*    pixs2 = pixRead("1555-3.jpg");  */
    pixs2 = pixRead("cat-7.jpg");
/*    pixs2 = pixRead("cat-14.jpg"); */
    pixn2 = pixBackgroundNormSimple(pixs2, NULL, NULL);
    pixg2 = pixConvertRGBToGray(pixn2, 0.5, 0.3, 0.2);
    pixb2 = pixThresholdToBinary(pixg2, 130);

        /* Apply the previous disparity model to this image */
    dew2 = dewarpCreate(pixb2, 7);
    dewarpaInsertDewarp(dewa, dew2);
    dewarpaInsertRefModels(dewa, 0, 1);
    dewarpaInfo(stderr, dewa);
    dewarpaApplyDisparity(dewa, 7, pixg2, 200, 0, 0, &pixd2,
                          "/tmp/lept/dewarp_apply2.pdf");
    dewarpaDestroy(&dewa);

        /* Write out files for the second image */
    pixWrite("/tmp/dewtest/017.jpg", pixs2, IFF_JFIF_JPEG);
    pixWrite("/tmp/dewtest/018.jpg", pixg2, IFF_JFIF_JPEG);
    pixWrite("/tmp/dewtest/019.png", pixb2, IFF_TIFF_G4);
    pixWrite("/tmp/dewtest/020.jpg", pixd2, IFF_JFIF_JPEG);
    pixt1 = pixRead("/tmp/dewmod/0060.png");
    pixWrite("/tmp/dewtest/021.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixt1 = pixRead("/tmp/dewapply/002.png");
    pixWrite("/tmp/dewtest/022.png", pixt1, IFF_PNG);
    pixt2 = pixThresholdToBinary(pixt1, 130);
    pixWrite("/tmp/dewtest/023.png", pixt2, IFF_TIFF_G4);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixt1 = pixRead("/tmp/dewmod/0070.png");
    pixWrite("/tmp/dewtest/024.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixt1 = pixRead("/tmp/dewapply/003.png");
    pixWrite("/tmp/dewtest/025.png", pixt1, IFF_PNG);
    pixt2 = pixThresholdToBinary(pixt1, 130);
    pixWrite("/tmp/dewtest/026.png", pixt2, IFF_TIFF_G4);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

        /* Generate the big pdf file */
    convertFilesToPdf("/tmp/dewtest", NULL, 135, 1.0, 0, 0, "Dewarp Test",
                      "/tmp/lept/dewarptest1.pdf");
    fprintf(stderr, "pdf file made: /tmp/lept/dewarptest1.pdf\n");

    lept_rmdir("dewmod");
    lept_rmdir("dewtest");
    pixDestroy(&pixs);
    pixDestroy(&pixn);
    pixDestroy(&pixg);
    pixDestroy(&pixb);
    pixDestroy(&pixd);
    pixDestroy(&pixs2);
    pixDestroy(&pixn2);
    pixDestroy(&pixg2);
    pixDestroy(&pixb2);
    pixDestroy(&pixd2);
    return 0;
}
