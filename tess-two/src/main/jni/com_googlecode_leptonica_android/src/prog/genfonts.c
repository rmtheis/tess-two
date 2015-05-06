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
 * genfonts.c
 *
 *    This program can be used to generate characters for a
 *    font and save them in .pa format.
 *
 *    The tiff images of bitmaps fonts, which are used as input
 *    to this generator, are supplied in Leptonica in the prog/fonts
 *    directory.  The tiff images were generated from the PostScript files
 *    in that directory, using the shell script prog/ps2tiff.  If you
 *    want to generate other fonts, modify the PostScript files
 *    and use ps2tiff.  ps2tiff uses GhostScript.
 */

#include "allheaders.h"

#define   NFONTS        9
#define   TEST_DIR      "/tmp/fonts"
#define   INSTALL_DIR   "fonts"

const char  *outputfonts[] = {"chars-4.pa", "chars-6.pa",
                              "chars-8.pa", "chars-10.pa",
                              "chars-12.pa", "chars-14.pa",
                              "chars-16.pa", "chars-18.pa",
                              "chars-20.pa"};

const l_int32 sizes[] = { 4, 6, 8, 10, 12, 14, 16, 18, 20 };

#define  DEBUG            1


int main(int    argc,
         char **argv)
{
char         buf[512];
char        *pathname;
l_int32      i, bl1, bl2, bl3;
PIX         *pixd;
PIXA        *pixa;
static char  mainName[] = "genfonts";

    if (argc != 1)
        return ERROR_INT(" Syntax:  genfonts", mainName, 1);

    /* ------------  Generate all the pixa char bitmap files ----------- */
#if 1
    lept_rmdir("fonts");
    lept_mkdir("fonts");
    for (i = 0; i < 9; i++) {
        pixaSaveFont(INSTALL_DIR, "/tmp/fonts", sizes[i]);

#if DEBUG
        pathname = genPathname(INSTALL_DIR, outputfonts[i]);
        pixa = pixaRead(pathname);
        fprintf(stderr, "Found %d chars in font size %d\n",
                pixaGetCount(pixa), sizes[i]);
        pixd = pixaDisplayTiled(pixa, 1500, 0, 15);
        pixDisplay(pixd, 100 * i, 200);
        pixDestroy(&pixd);
        pixaDestroy(&pixa);
        lept_free(pathname);
#endif  /* DEBUG */

    }
#endif


    /* -----  Use pixaGetFont() and write the result out  -----*/
#if 1
    for (i = 0; i < 9; i++) {
        pixa = pixaGetFont(INSTALL_DIR, sizes[i], &bl1, &bl2, &bl3);
        fprintf(stderr, "Baselines are at: %d, %d, %d\n", bl1, bl2, bl3);
        snprintf(buf, sizeof(buf), "/tmp/junkchars.%d.pixa", sizes[i]);
        pixaWrite(buf, pixa);

#if DEBUG
        pixd = pixaDisplayTiled(pixa, 1500, 0, 15);
        pixDisplay(pixd, 100 * i, 700);
        pixDestroy(&pixd);
#endif  /* DEBUG */

        pixaDestroy(&pixa);
    }
#endif


    /* ------------  Get timing for font generation ----------- */
#if 0
    startTimer();
    i = 8;
    pixa = pixaGenerateFont(DIRECTORY, sizes[i], &bl1, &bl2, &bl3);
    pixaDestroy(&pixa);
    fprintf(stderr, "Time for font gen = %7.3f sec\n", stopTimer());
#endif

    return 0;
}


