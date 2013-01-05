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
 * flipselgen.c
 *
 *    Generates dwa code for hit-miss transform (hmt) that is
 *    used in pixPageFlipDetectDWA().
 *
 *    Results are two files:
 *        fmorphgen.3.c
 *        fmorphgenlow.3.c
 *    using INDEX = 3.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define   INDEX      3
#define   DFLAG      1

    /* Sels for pixPageFlipDetectDWA() */
static const char *textsel1 = "x  oo "
                              "x oOo "
                              "x  o  "
                              "x     "
                              "xxxxxx";

static const char *textsel2 = " oo  x"
                              " oOo x"
                              "  o  x"
                              "     x"
                              "xxxxxx";

static const char *textsel3 = "xxxxxx"
                              "x     "
                              "x  o  "
                              "x oOo "
                              "x  oo ";

static const char *textsel4 = "xxxxxx"
                              "     x"
                              "  o  x"
                              " oOo x"
                              " oo  x";

main(int    argc,
     char **argv)
{
SEL         *sel1, *sel2, *sel3, *sel4;
SELA        *sela;
PIX         *pix, *pixd;
PIXA        *pixa;
static char  mainName[] = "flipselgen";

    if (argc != 1)
        exit(ERROR_INT(" Syntax: flipselgen", mainName, 1));

    sela = selaCreate(0);
    sel1 = selCreateFromString(textsel1, 5, 6, "flipsel1");
    sel2 = selCreateFromString(textsel2, 5, 6, "flipsel2");
    sel3 = selCreateFromString(textsel3, 5, 6, "flipsel3");
    sel4 = selCreateFromString(textsel4, 5, 6, "flipsel4");
    selaAddSel(sela, sel1, NULL, 0);
    selaAddSel(sela, sel2, NULL, 0);
    selaAddSel(sela, sel3, NULL, 0);
    selaAddSel(sela, sel4, NULL, 0);

    pixa = pixaCreate(4);
    pix = selDisplayInPix(sel1, 23, 2);
    pixDisplayWithTitle(pix, 100, 100, "sel1", DFLAG);
    pixaAddPix(pixa, pix, L_INSERT);
    pix = selDisplayInPix(sel2, 23, 2);
    pixDisplayWithTitle(pix, 275, 100, "sel2", DFLAG);
    pixaAddPix(pixa, pix, L_INSERT);
    pix = selDisplayInPix(sel3, 23, 2);
    pixDisplayWithTitle(pix, 450, 100, "sel3", DFLAG);
    pixaAddPix(pixa, pix, L_INSERT);
    pix = selDisplayInPix(sel4, 23, 2);
    pixDisplayWithTitle(pix, 625, 100, "sel4", DFLAG);
    pixaAddPix(pixa, pix, L_INSERT);

    pixd = pixaDisplayTiled(pixa, 800, 0, 15);
    pixDisplayWithTitle(pixd, 100, 300, "allsels", DFLAG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    if (fhmtautogen(sela, INDEX, NULL))
        exit(ERROR_INT(" Generation failed", mainName, 1));

    selaDestroy(&sela);
    return 0;
}

