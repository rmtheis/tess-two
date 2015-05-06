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

int main(int    argc,
         char **argv)
{
SEL         *sel1, *sel2, *sel3, *sel4;
SELA        *sela;
PIX         *pix, *pixd;
PIXA        *pixa;
static char  mainName[] = "flipselgen";

    if (argc != 1)
        return ERROR_INT(" Syntax: flipselgen", mainName, 1);

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
        return ERROR_INT(" Generation failed", mainName, 1);

    selaDestroy(&sela);
    return 0;
}

