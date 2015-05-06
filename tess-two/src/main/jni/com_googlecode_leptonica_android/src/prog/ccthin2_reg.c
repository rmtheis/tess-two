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
 * ccthin2_reg.c
 *
 *   Tests the examples in pixThinExamples()
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
l_int32      index, maxiters, type;
BOX         *box;
PIX         *pix, *pixs, *pixd, *pixt;
PIXA        *pixa;
static char  mainName[] = "ccthin2_reg";

    if (argc != 1 && argc != 3)
        return ERROR_INT(" Syntax: ccthin2_reg [index maxiters]", mainName, 1);

    pixDisplayWrite(NULL, 0);
    pix = pixRead("feyn.tif");
    box = boxCreate(683, 799, 970, 479);
    pixs = pixClipRectangle(pix, box, NULL);
    pixDisplayWrite(pixs, 1);

        /* Just do one of the examples */
    if (argc == 3) {
        index = atoi(argv[1]);
        maxiters = atoi(argv[2]);
        if (index <= 7)
            type = L_THIN_FG;
        else
            type = L_THIN_BG;
        pixt = pixThinExamples(pixs, type, index, maxiters,
                               "/tmp/junksels.png");
        pixDisplay(pixt, 100, 100);
        pixDisplayWrite(pixt, 1);
        pixDestroy(&pixt);
        pixDisplayMultiple("/tmp/display/file*");
        return 0;
    }

        /* Do all the examples */
    pixt = pixThinExamples(pixs, L_THIN_FG, 1, 0, "/tmp/junksel_example1.png");
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);
    pixt = pixThinExamples(pixs, L_THIN_FG, 2, 0, "/tmp/junksel_example2.png");
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);
    pixt = pixThinExamples(pixs, L_THIN_FG, 3, 0, "/tmp/junksel_example3.png");
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);
    pixt = pixThinExamples(pixs, L_THIN_FG, 4, 0, "/tmp/junksel_example4.png");
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);
    pixt = pixThinExamples(pixs, L_THIN_FG, 5, 0, "/tmp/junksel_example5.png");
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);
    pixt = pixThinExamples(pixs, L_THIN_FG, 6, 0, "/tmp/junksel_example6.png");
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);
    pixt = pixThinExamples(pixs, L_THIN_FG, 7, 0, "/tmp/junksel_example7.png");
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);
    pixt = pixThinExamples(pixs, L_THIN_BG, 8, 5, "/tmp/junksel_example8.png");
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);
    pixt = pixThinExamples(pixs, L_THIN_BG, 9, 5, "/tmp/junksel_example9.png");
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);

        /* Display the thinning results */
    pixa = pixaReadFiles("/tmp/display", "file");
    pixd = pixaDisplayTiledAndScaled(pixa, 8, 500, 1, 0, 25, 2);
    pixWrite("/tmp/junktiles.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

        /* Display the sels used in the examples */
    pixa = pixaReadFiles("/tmp", "junksel_example");
    pixd = pixaDisplayTiledInRows(pixa, 1, 500, 1.0, 0, 50, 2);
    pixWrite("/tmp/junksels.png", pixd, IFF_PNG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    pixDestroy(&pix);
    pixDestroy(&pixs);
    boxDestroy(&box);

    pixDisplayMultiple("/tmp/display/file*");
    return 0;
}


