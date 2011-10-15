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
 * ccthin1_reg.c
 *
 *   Tests the "best" cc-preserving thinning functions.
 *   Displays all the strong cc-preserving 3x3 Sels.
 */

#include <stdio.h>
#include <stdlib.h>
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


main(int    argc,
     char **argv)
{
BOX         *box;
PIX         *pix, *pixs, *pixd, *pixt;
PIXA        *pixa;
SEL         *sel, *sel1, *sel2, *sel3;
SELA        *sela4, *sela8, *sela48;
static char  mainName[] = "ccthin1_reg";

    if (argc != 1)
	exit(ERROR_INT(" Syntax: ccthin1_reg", mainName, 1));

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
    if ((pix = pixRead("feyn.tif")) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));
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
    pixa = pixaReadFiles("/tmp", "junk_write_display");
    pixd = pixaDisplayTiledAndScaled(pixa, 8, 500, 1, 0, 25, 2);
    pixWrite("/tmp/junktiles.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    pixDestroy(&pix);
    pixDestroy(&pixs);
    boxDestroy(&box);

    pixDisplayMultiple("/tmp/junk_write_display*");
    return 0;
}


