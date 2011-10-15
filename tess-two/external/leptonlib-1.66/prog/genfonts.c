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
 * genfonts.c
 *
 *    This program can be used to generate characters for a
 *    font and save them in .pixa format.
 *
 *    The tiff images of bitmaps fonts, which are used as input
 *    to this generator, are supplied in Leptonica in the prog/fonts
 *    directory.  The tiff images were generated from the PostScript files
 *    in that directory, using the shell script prog/ps2tiff.  If you
 *    want to generate other fonts, modify the PostScript files
 *    and use ps2tiff.  ps2tiff uses GhostScript.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define   NFONTS   9
#define   DIRECTORY    "./fonts"

const char  *outputfonts[] = {"chars-4.pixa", "chars-6.pixa",
                              "chars-8.pixa", "chars-10.pixa",
                              "chars-12.pixa", "chars-14.pixa",
                              "chars-16.pixa", "chars-18.pixa",
                              "chars-20.pixa"};

const l_int32 sizes[] = { 4, 6, 8, 10, 12, 14, 16, 18, 20 };

#define  DEBUG            1


main(int    argc,
     char **argv)
{
char        *pathname;
l_int32      i, bl1, bl2, bl3;
PIX         *pixd;
PIXA        *pixa;
static char  mainName[] = "genfonts";

    if (argc != 1)
	exit(ERROR_INT(" Syntax:  genfonts", mainName, 1));

#if 1   /* Generate all the pixa char bitmap files */
    for (i = 0; i < 9; i++) { 
        pixaSaveFont(DIRECTORY, DIRECTORY, sizes[i]);
#if DEBUG
        pathname = genPathname(DIRECTORY, outputfonts[i]);
	pixa = pixaRead(pathname);
	fprintf(stderr, "Found %d chars in font size %d\n",
	        pixaGetCount(pixa), sizes[i]);
	fprintf(stderr, "Baselines are at: %d, %d, %d\n", bl1, bl2, bl3);
	pixd = pixaDisplayTiled(pixa, 1500, 0, 15);
	pixDisplay(pixd, 100 * i, 200);
	pixDestroy(&pixd);
	pixaDestroy(&pixa);
	FREE(pathname);
#endif  /* DEBUG */
    }
#endif

#if 0   /* Get timing for font generation */
    startTimer();
    i = 8;
    pixa = pixaGenerateFont(DIRECTORY, sizes[i], &bl1, &bl2, &bl3);
    pixaDestroy(&pixa);
    fprintf(stderr, "Time for font gen = %7.3f sec\n", stopTimer());
#endif

#if 1   /* Use pixaGetFont() and write the result out */
    pixa = pixaGetFont(DIRECTORY, 10, &bl1, &bl2, &bl3);
    pixaWrite("junkchars16", pixa);
#if DEBUG
    pixd = pixaDisplayTiled(pixa, 1500, 0, 15);
    pixDisplay(pixd, 200, 200);
    pixDestroy(&pixd);
#endif  /* DEBUG */
#endif

    exit(0);
}


