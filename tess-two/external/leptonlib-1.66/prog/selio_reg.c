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
 * selio_reg.c
 *
 *    Runs a number of tests on reading and writing of Sels
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
l_int32  display, success;
FILE    *fp;
PIX     *pix;
SEL     *sel;
SELA    *sela1, *sela2;

    if (regTestSetup(argc, argv, &fp, &display, &success, NULL))
              return 1;

        /* selaRead() / selaWrite()  */
    sela1 = selaAddBasic(NULL);
    selaWrite("/tmp/sel.0.sela", sela1);
    regTestCheckFile(fp, argv, "/tmp/sel.0.sela", 0, &success);
    sela2 = selaRead("/tmp/sel.0.sela");
    selaWrite("/tmp/sel.1.sela", sela2);
    regTestCheckFile(fp, argv, "/tmp/sel.1.sela", 1, &success);
    regTestCompareFiles(fp, argv, 0, 1, &success);
    selaDestroy(&sela1);
    selaDestroy(&sela2);
    
	/* Create from file and display result */
    sela1 = selaCreateFromFile("flipsels.txt");
    pix = selaDisplayInPix(sela1, 31, 3, 15, 4);
    pixWrite("/tmp/sel.2.png", pix, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/sel.2.png", 2, &success);
    pixDisplayWithTitle(pix, 100, 100, NULL, display);
    selaWrite("/tmp/sel.3.sela", sela1);
    regTestCheckFile(fp, argv, "/tmp/sel.3.sela", 3, &success);
    pixDestroy(&pix);
    selaDestroy(&sela1);

        /* Create the same set of Sels from compiled strings and compare */
    sela2 = selaCreate(4);
    sel = selCreateFromString(textsel1, 5, 6, "textsel1");
    selaAddSel(sela2, sel, NULL, 0);
    sel = selCreateFromString(textsel2, 5, 6, "textsel2");
    selaAddSel(sela2, sel, NULL, 0);
    sel = selCreateFromString(textsel3, 5, 6, "textsel3");
    selaAddSel(sela2, sel, NULL, 0);
    sel = selCreateFromString(textsel4, 5, 6, "textsel4");
    selaAddSel(sela2, sel, NULL, 0);
    selaWrite("/tmp/sel.4.sela", sela2);
    regTestCheckFile(fp, argv, "/tmp/sel.4.sela", 4, &success);
    regTestCompareFiles(fp, argv, 3, 4, &success);
    selaDestroy(&sela2);

    regTestCleanup(argc, argv, fp, success, NULL);
    return 0;
}

