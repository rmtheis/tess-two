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
 * ccthin1_reg.c
 *
 *   Tests the "best" cc-preserving thinning functions.
 *   Displays all the strong cc-preserving 3x3 Sels.
 */

#include "allheaders.h"

    /* Sels for 4-connected thinning */
static const char *sel_4_1 = "  x"
                             "oCx"
                             "  x";

static const char *sel_4_2 = "  x"
                             "oCx"
                             " o ";

static const char *sel_4_3 = " o "
                             "oCx"
                             "  x";

static const char *sel_4_4 = " o "
                             "oCx"
                             " o ";

static const char *sel_4_5 = " ox"
                             "oCx"
                             " o ";

static const char *sel_4_6 = " o "
                             "oCx"
                             " ox";

static const char *sel_4_7 = " xx"
                             "oCx"
                             " o ";

static const char *sel_4_8 = "  x"
                             "oCx"
                             "o x";

static const char *sel_4_9 = "o x"
                             "oCx"
                             "  x";

    /* Sels for 8-connected thinning */
static const char *sel_8_1 = " x "
                             "oCx"
                             " x ";

static const char *sel_8_2 = " x "
                             "oCx"
                             "o  ";

static const char *sel_8_3 = "o  "
                             "oCx"
                             " x ";

static const char *sel_8_4 = "o  "
                             "oCx"
                             "o  ";

static const char *sel_8_5 = "o x"
                             "oCx"
                             "o  ";

static const char *sel_8_6 = "o  "
                             "oCx"
                             "o x";

static const char *sel_8_7 = " x "
                             "oCx"
                             "oo ";

static const char *sel_8_8 = " x "
                             "oCx"
                             "ox ";

static const char *sel_8_9 = "ox "
                             "oCx"
                             " x ";

    /* Sels for both 4 and 8-connected thinning */
static const char *sel_48_1 = " xx"
                              "oCx"
                              "oo ";

static const char *sel_48_2 = "o x"
                              "oCx"
                              "o x";


int main(int    argc,
         char **argv)
{
BOX         *box;
PIX         *pix, *pixs, *pixd, *pixt;
PIXA        *pixa;
SEL         *sel, *sel1, *sel2, *sel3;
SELA        *sela4, *sela8, *sela48;
static char  mainName[] = "ccthin1_reg";

    if (argc != 1)
        return ERROR_INT(" Syntax: ccthin1_reg", mainName, 1);

        /* Generate and display all of the 4-cc sels */
    sela4 = selaCreate(9);
    sel = selCreateFromString(sel_4_1, 3, 3, "sel_4_1");
    selaAddSel(sela4, sel, NULL, 0);
    sel = selCreateFromString(sel_4_2, 3, 3, "sel_4_2");
    selaAddSel(sela4, sel, NULL, 0);
    sel = selCreateFromString(sel_4_3, 3, 3, "sel_4_3");
    selaAddSel(sela4, sel, NULL, 0);
    sel = selCreateFromString(sel_4_4, 3, 3, "sel_4_4");
    selaAddSel(sela4, sel, NULL, 0);
    sel = selCreateFromString(sel_4_5, 3, 3, "sel_4_5");
    selaAddSel(sela4, sel, NULL, 0);
    sel = selCreateFromString(sel_4_6, 3, 3, "sel_4_6");
    selaAddSel(sela4, sel, NULL, 0);
    sel = selCreateFromString(sel_4_7, 3, 3, "sel_4_7");
    selaAddSel(sela4, sel, NULL, 0);
    sel = selCreateFromString(sel_4_8, 3, 3, "sel_4_8");
    selaAddSel(sela4, sel, NULL, 0);
    sel = selCreateFromString(sel_4_9, 3, 3, "sel_4_9");
    selaAddSel(sela4, sel, NULL, 0);
    pixt = selaDisplayInPix(sela4, 35, 3, 15, 3);
    pixWrite("/tmp/junkallsel4.png", pixt, IFF_PNG);
    pixDestroy(&pixt);
    selaDestroy(&sela4);

        /* Generate and display all of the 8-cc sels */
    sela8 = selaCreate(9);
    sel = selCreateFromString(sel_8_1, 3, 3, "sel_8_1");
    selaAddSel(sela8, sel, NULL, 0);
    sel = selCreateFromString(sel_8_2, 3, 3, "sel_8_2");
    selaAddSel(sela8, sel, NULL, 0);
    sel = selCreateFromString(sel_8_3, 3, 3, "sel_8_3");
    selaAddSel(sela8, sel, NULL, 0);
    sel = selCreateFromString(sel_8_4, 3, 3, "sel_8_4");
    selaAddSel(sela8, sel, NULL, 0);
    sel = selCreateFromString(sel_8_5, 3, 3, "sel_8_5");
    selaAddSel(sela8, sel, NULL, 0);
    sel = selCreateFromString(sel_8_6, 3, 3, "sel_8_6");
    selaAddSel(sela8, sel, NULL, 0);
    sel = selCreateFromString(sel_8_7, 3, 3, "sel_8_7");
    selaAddSel(sela8, sel, NULL, 0);
    sel = selCreateFromString(sel_8_8, 3, 3, "sel_8_8");
    selaAddSel(sela8, sel, NULL, 0);
    sel = selCreateFromString(sel_8_9, 3, 3, "sel_8_9");
    selaAddSel(sela8, sel, NULL, 0);
    pixt = selaDisplayInPix(sela8, 35, 3, 15, 3);
    pixWrite("/tmp/junkallsel8.png", pixt, IFF_PNG);
    pixDestroy(&pixt);
    selaDestroy(&sela8);

        /* Generate and display all of the 4 and 8-cc preserving sels */
    sela48 = selaCreate(3);
    sel = selCreateFromString(sel_48_1, 3, 3, "sel_48_1");
    selaAddSel(sela48, sel, NULL, 0);
    sel = selCreateFromString(sel_48_2, 3, 3, "sel_48_2");
    selaAddSel(sela48, sel, NULL, 0);
    pixt = selaDisplayInPix(sela48, 35, 3, 15, 4);
    pixWrite("/tmp/junkallsel48.png", pixt, IFF_PNG);
    pixDestroy(&pixt);
    selaDestroy(&sela48);

        /* Generate and display three of the 4-cc sels and their rotations */
    sela4 = selaCreate(3);
    sel = selCreateFromString(sel_4_1, 3, 3, "sel_4_1");
    sel1 = selRotateOrth(sel, 1);
    sel2 = selRotateOrth(sel, 2);
    sel3 = selRotateOrth(sel, 3);
    selaAddSel(sela4, sel, NULL, 0);
    selaAddSel(sela4, sel1, "sel_4_1_90", 0);
    selaAddSel(sela4, sel2, "sel_4_1_180", 0);
    selaAddSel(sela4, sel3, "sel_4_1_270", 0);
    sel = selCreateFromString(sel_4_2, 3, 3, "sel_4_2");
    sel1 = selRotateOrth(sel, 1);
    sel2 = selRotateOrth(sel, 2);
    sel3 = selRotateOrth(sel, 3);
    selaAddSel(sela4, sel, NULL, 0);
    selaAddSel(sela4, sel1, "sel_4_2_90", 0);
    selaAddSel(sela4, sel2, "sel_4_2_180", 0);
    selaAddSel(sela4, sel3, "sel_4_2_270", 0);
    sel = selCreateFromString(sel_4_3, 3, 3, "sel_4_3");
    sel1 = selRotateOrth(sel, 1);
    sel2 = selRotateOrth(sel, 2);
    sel3 = selRotateOrth(sel, 3);
    selaAddSel(sela4, sel, NULL, 0);
    selaAddSel(sela4, sel1, "sel_4_3_90", 0);
    selaAddSel(sela4, sel2, "sel_4_3_180", 0);
    selaAddSel(sela4, sel3, "sel_4_3_270", 0);
    pixt = selaDisplayInPix(sela4, 35, 3, 15, 4);
    pixWrite("/tmp/junksel4.png", pixt, IFF_PNG);
    pixDestroy(&pixt);
    selaDestroy(&sela4);

        /* Generate and display four of the 8-cc sels and their rotations */
    sela8 = selaCreate(4);
    sel = selCreateFromString(sel_8_2, 3, 3, "sel_8_2");
    sel1 = selRotateOrth(sel, 1);
    sel2 = selRotateOrth(sel, 2);
    sel3 = selRotateOrth(sel, 3);
    selaAddSel(sela8, sel, NULL, 0);
    selaAddSel(sela8, sel1, "sel_8_2_90", 0);
    selaAddSel(sela8, sel2, "sel_8_2_180", 0);
    selaAddSel(sela8, sel3, "sel_8_2_270", 0);
    sel = selCreateFromString(sel_8_3, 3, 3, "sel_8_3");
    sel1 = selRotateOrth(sel, 1);
    sel2 = selRotateOrth(sel, 2);
    sel3 = selRotateOrth(sel, 3);
    selaAddSel(sela8, sel, NULL, 0);
    selaAddSel(sela8, sel1, "sel_8_3_90", 0);
    selaAddSel(sela8, sel2, "sel_8_3_180", 0);
    selaAddSel(sela8, sel3, "sel_8_3_270", 0);
    sel = selCreateFromString(sel_8_5, 3, 3, "sel_8_5");
    sel1 = selRotateOrth(sel, 1);
    sel2 = selRotateOrth(sel, 2);
    sel3 = selRotateOrth(sel, 3);
    selaAddSel(sela8, sel, NULL, 0);
    selaAddSel(sela8, sel1, "sel_8_5_90", 0);
    selaAddSel(sela8, sel2, "sel_8_5_180", 0);
    selaAddSel(sela8, sel3, "sel_8_5_270", 0);
    sel = selCreateFromString(sel_8_6, 3, 3, "sel_8_6");
    sel1 = selRotateOrth(sel, 1);
    sel2 = selRotateOrth(sel, 2);
    sel3 = selRotateOrth(sel, 3);
    selaAddSel(sela8, sel, NULL, 0);
    selaAddSel(sela8, sel1, "sel_8_6_90", 0);
    selaAddSel(sela8, sel2, "sel_8_6_180", 0);
    selaAddSel(sela8, sel3, "sel_8_6_270", 0);
    pixt = selaDisplayInPix(sela8, 35, 3, 15, 4);
    pixWrite("/tmp/junksel8.png", pixt, IFF_PNG);
    pixDestroy(&pixt);
    selaDestroy(&sela8);

        /* Test the best 4 and 8 cc thinning */
    pixDisplayWrite(NULL, 0);
    pix = pixRead("feyn.tif");
    box = boxCreate(683, 799, 970, 479);
    pixs = pixClipRectangle(pix, box, NULL);
    pixDisplayWrite(pixs, 1);

    pixt = pixThin(pixs, L_THIN_FG, 4, 0);
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);
    pixt = pixThin(pixs, L_THIN_BG, 4, 0);
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);

    pixt = pixThin(pixs, L_THIN_FG, 8, 0);
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);
    pixt = pixThin(pixs, L_THIN_BG, 8, 0);
    pixDisplayWrite(pixt, 1);
    pixDestroy(&pixt);

        /* Display tiled */
    pixa = pixaReadFiles("/tmp/display", "file");
    pixd = pixaDisplayTiledAndScaled(pixa, 8, 500, 1, 0, 25, 2);
    pixWrite("/tmp/junktiles.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    pixDestroy(&pix);
    pixDestroy(&pixs);
    boxDestroy(&box);

    pixDisplayMultiple("/tmp/display/file*");
    return 0;
}


