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
 * morphseq_reg.c
 *
 *    Simple regression test for binary morph sequence (interpreter),
 *    showing display mode and rejection of invalid sequence components.
 */

#include "allheaders.h"

#define  SEQUENCE1    "O1.3 + C3.1 + R22 + D2.2 + X4"
#define  SEQUENCE2    "O2.13 + C5.23 + R22 + X4"
#define  SEQUENCE3    "e3.3 + d3.3 + tw5.5"
#define  SEQUENCE4    "O3.3 + C3.3"
#define  SEQUENCE5    "O5.5 + C5.5"
#define  BAD_SEQUENCE  "O1.+D8 + E2.4 + e.4 + r25 + R + R.5 + X + x5 + y7.3"

#define  DISPLAY_SEPARATION   0   /* use 250 to get images displayed */

main(int    argc,
     char **argv)
{
char        *infile;
PIX         *pixs, *pixg, *pixc, *pixd;
static char  mainName[] = "morphseq_reg";

    if (argc != 1)
	return ERROR_INT(" Syntax:  morphseq_reg", mainName, 1);

    pixs = pixRead("feyn.tif");

        /* 1 bpp */
    pixd = pixMorphSequence(pixs, SEQUENCE1, -1);
    pixDestroy(&pixd);
    pixd = pixMorphSequence(pixs, SEQUENCE1, DISPLAY_SEPARATION);
    pixWrite("/tmp/morphseq1.png", pixd, IFF_PNG);
    pixDestroy(&pixd);

    pixd = pixMorphCompSequence(pixs, SEQUENCE2, -2);
    pixDestroy(&pixd);
    pixd = pixMorphCompSequence(pixs, SEQUENCE2, DISPLAY_SEPARATION);
    pixWrite("/tmp/morphseq2.png", pixd, IFF_PNG);
    pixDestroy(&pixd);

    pixd = pixMorphSequenceDwa(pixs, SEQUENCE2, -3);
    pixDestroy(&pixd);
    pixd = pixMorphSequenceDwa(pixs, SEQUENCE2, DISPLAY_SEPARATION);
    pixWrite("/tmp/morphseq3.png", pixd, IFF_PNG);
    pixDestroy(&pixd);

    pixd = pixMorphCompSequenceDwa(pixs, SEQUENCE2, -4);
    pixDestroy(&pixd);
    pixd = pixMorphCompSequenceDwa(pixs, SEQUENCE2, DISPLAY_SEPARATION);
    pixWrite("/tmp/morphseq4.png", pixd, IFF_PNG);
    pixDestroy(&pixd);

        /* 8 bpp */
    pixg = pixScaleToGray(pixs, 0.25);
    pixd = pixGrayMorphSequence(pixg, SEQUENCE3, -5, 150);
    pixDestroy(&pixd);
    pixd = pixGrayMorphSequence(pixg, SEQUENCE3, DISPLAY_SEPARATION, 150);
    pixWrite("/tmp/morphseq5.png", pixd, IFF_PNG);
    pixDestroy(&pixd);

    pixd = pixGrayMorphSequence(pixg, SEQUENCE4, -6, 300);
    pixWrite("/tmp/morphseq6.png", pixd, IFF_PNG);
    pixDestroy(&pixd);

        /* 32 bpp */
    pixc = pixRead("wyom.jpg");
    pixd = pixColorMorphSequence(pixc, SEQUENCE5, -7, 150);
    pixDestroy(&pixd);
    pixd = pixColorMorphSequence(pixc, SEQUENCE5, DISPLAY_SEPARATION, 450);
    pixWrite("/tmp/morphseq7.png", pixd, IFF_PNG);
    pixDestroy(&pixc);
    pixDestroy(&pixd);

        /* Syntax error handling */
    fprintf(stderr, " ------------ Error messages follow ------------------\n");
    pixd = pixMorphSequence(pixs, BAD_SEQUENCE, 50);  /* fails; returns null */
    pixd = pixGrayMorphSequence(pixg, BAD_SEQUENCE, 50, 0);  /* this fails */

    pixDestroy(&pixg);
    pixDestroy(&pixs);
    return 0;
}
