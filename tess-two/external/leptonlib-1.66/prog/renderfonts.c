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
 * renderfonts.c
 *
 *     This tests the font rendering functions
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define   NFONTS   9
#define   DIRECTORY    "./fonts"

const l_int32 sizes[] = { 4, 6, 8, 10, 12, 14, 16, 18, 20 };

#define  DEBUG            0


main(int    argc,
     char **argv)
{
char        *filein, *fileout, *textstr;
l_int32      i, d, size, width, wtext, overflow;
l_uint32     val;
L_BMF       *bmf;
PIX         *pixs, *pix;
static char  mainName[] = "renderfonts";

    if (argc != 4)
	exit(ERROR_INT("Syntax: renderfonts filein size fileout", mainName, 1));

    filein = argv[1];
    size = atoi(argv[2]);
    fileout = argv[3];

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
    d = pixGetDepth(pixs);
    if (d == 8)
        val = 128;
    else if (d == 16)
        val = 0x8000;
    else if (d == 32)
        composeRGBPixel(128, 0, 255, &val);
    else
	exit(ERROR_INT("pixs not 8, 16 or 32 bpp", mainName, 1));

    bmf = bmfCreate(DIRECTORY, size);

#if 0  /* render a character of text */
    pix = pixaGetPix(bmf->pixa, 45, L_CLONE);
    startTimer();
    for (i = 0; i < 10000; i++)
	pixSetMaskedGeneral(pixs, pix, val, 150, 150);
    fprintf(stderr, "time: %7.3f sec\n", stopTimer());
    pixWrite(fileout, pixs, IFF_JFIF_JPEG);
    pixDestroy(&pix);
#endif

#if 0  /* render a line of text; use marge.jpg with size 14 */
    bmfGetStringWidth(bmf, "This is a funny cat!", &width);
    fprintf(stderr, "String width: %d pixels\n", width);

    pixSetTextline(pixs, bmf, "This is a funny cat!", 0x8000ff00, 50, 250,
                   &width, &overflow);
    pixWrite(fileout, pixs, IFF_JFIF_JPEG);
    fprintf(stderr, "Text width = %d\n", width);
    if (overflow)
        fprintf(stderr, "Text overflow beyond image boundary\n");
#endif

#if 1  /* render a block of text; use marge.jpg with size 14 */
    textstr = stringNew("This is a cat! This is a funny cat! This is a funny funny cat! This is a funny funny funny cat!");

    wtext = pixGetWidth(pixs) - 70;
    pixSetTextblock(pixs, bmf, textstr, 0x4040ff00, 50, 50, wtext,
                    1, &overflow);
    pixWrite(fileout, pixs, IFF_JFIF_JPEG);
    if (overflow)
        fprintf(stderr, "Text overflow beyond image boundary\n");
    FREE((void *)textstr);
#endif

    pixDestroy(&pixs);
    bmfDestroy(&bmf);
    return 0;
}

