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
 * livre_orient.c
 *
 *    This generates an image of the set of 4 HMT Sels that are
 *    used for counting ascenders and descenders to detect
 *    text orientation.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

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
PIX         *pixd;
SEL         *sel1, *sel2, *sel3, *sel4;
SELA        *sela;
static char  mainName[] = "livre_orient";

    sel1 = selCreateFromString(textsel1, 5, 6, NULL);
    sel2 = selCreateFromString(textsel2, 5, 6, NULL);
    sel3 = selCreateFromString(textsel3, 5, 6, NULL);
    sel4 = selCreateFromString(textsel4, 5, 6, NULL);

    sela = selaCreate(4);
    selaAddSel(sela, sel1, "textsel1", L_INSERT);
    selaAddSel(sela, sel2, "textsel2", L_INSERT);
    selaAddSel(sela, sel3, "textsel3", L_INSERT);
    selaAddSel(sela, sel4, "textsel4", L_INSERT);

    pixd = selaDisplayInPix(sela, 28, 3, 30, 4);
    pixWrite("/tmp/orient.png", pixd, IFF_PNG);
    pixDisplay(pixd, 100, 100);

    pixDestroy(&pixd);
    selaDestroy(&sela);
    return 0;
}

