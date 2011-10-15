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
 *  colormorphtest.c
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static void pixCompare(PIX *pix, PIX *pix2, const char *msg1, const char *msg2);

    /* MSVC can't handle arrays dimensioned by static const integers */
#define L_BUF_SIZE    256


main(int    argc,
     char **argv)
{
char        *filein;
char         buf[L_BUF_SIZE];
l_int32      size;
PIX         *pixs, *pixt1, *pixt2;
static char  mainName[] = "colormorphtest";

    if (argc != 3)
        exit(ERROR_INT(" Syntax:  colormorphtest filein size",
             mainName, 1));

    filein = argv[1];
    size = atoi(argv[2]);
    if (size % 2 == 0) size++;

    if ((pixs = pixRead(filein)) == NULL)
        exit(ERROR_INT("pixs not read", mainName, 1));

    pixt1 = pixColorMorph(pixs, L_MORPH_DILATE, size, size);
    sprintf(buf, "d%d.%d", size, size);
    pixt2 = pixColorMorphSequence(pixs, buf, 0, 0);
    pixCompare(pixt1, pixt2, "Correct for dilation", "Error on dilation");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

    pixt1 = pixColorMorph(pixs, L_MORPH_ERODE, size, size);
    sprintf(buf, "e%d.%d", size, size);
    pixt2 = pixColorMorphSequence(pixs, buf, 0, 0);
    pixCompare(pixt1, pixt2, "Correct for erosion", "Error on erosion");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

    pixt1 = pixColorMorph(pixs, L_MORPH_OPEN, size, size);
    sprintf(buf, "o%d.%d", size, size);
    pixt2 = pixColorMorphSequence(pixs, buf, 0, 0);
    pixCompare(pixt1, pixt2, "Correct for opening", "Error on opening");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

    pixt1 = pixColorMorph(pixs, L_MORPH_CLOSE, size, size);
    sprintf(buf, "c%d.%d", size, size);
    pixt2 = pixColorMorphSequence(pixs, buf, 0, 0);
    pixCompare(pixt1, pixt2, "Correct for closing", "Error on closing");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

    pixDisplayMultiple("/tmp/junk_write_display*");

    pixDestroy(&pixs);
    return 0;
}

    /* Simple comparison function */
static void pixCompare(PIX         *pix1,
                       PIX         *pix2,
                       const char  *msg1,
                       const char  *msg2)
{
l_int32  same;
    pixEqual(pix1, pix2, &same);
    if (same) {
        fprintf(stderr, "%s\n", msg1);
        pixDisplayWrite(pix1, 1);
    }
    else {
        fprintf(stderr, "%s\n", msg2);
        pixDisplayWrite(pix1, 1);
        pixDisplayWrite(pix2, 1);
    }
    return;
}

