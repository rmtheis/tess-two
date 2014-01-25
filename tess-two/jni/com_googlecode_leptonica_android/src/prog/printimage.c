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
 * printimage.c
 *
 *   Syntax:  printimage filein [-P<printer> [-#<number>]
 *
 *   If you want the image printed, use the standard lpr flags
 *   for either (or both) the printer and the number of copies.
 *
 *   If neither a printer nor a number of copies is specified, the
 *   only action is that a new PostScript file,
 *          /tmp/print_image.ps
 *   is generated for the image.
 *
 *   To get color output, you may need a special lpr flag.  In that case,
 *   first generate the PostScript file and then use a printer-dependent
 *   output flag, such as "-o ColorModel=Color" or "-o ColorModel=CMYK":
 *         lpr -P<printer> <output flag> /tmp/print_image.ps
 *   A simple way to get the flag in linux is to bring up the print-driver
 *   interface in acroread, select the color printer, select properties,
 *   select color, and look at the print command line that would be used.
 *
 *   The PS file generated is level 1.  This is large, but will work
 *   on all PS printers.
 */

#include "allheaders.h"

static const l_float32  FILL_FACTOR = 0.95;   /* fill factor on 8.5 x 11 page */


int main(int    argc,
         char **argv)
{
char        *filein, *fname, *argp, *argn;
char         buffer[512];
l_int32      i, w, h, ignore;
l_float32    scale;
FILE        *fp;
PIX         *pixs, *pixt;
static char  mainName[] = "printimage";

    if (argc < 2 || argc > 4)
        return ERROR_INT(
            " Syntax:  printimage filein [-P<printer>] [-#<number>]",
            mainName, 1);

        /* parse args */
    filein = argv[1];
    argp = argn = NULL;
    if (argc > 2) {
        for (i = 2; i < argc; i++) {
            if (argv[i][1] == 'P')
                argp = argv[i];
            else if (argv[i][1] == '#')
                argn = argv[i];
        }
    }

    lept_rm(NULL, "print_image.ps");

    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);

    pixGetDimensions(pixs, &w, &h, NULL);
    if (w > h) {
        pixt = pixRotate90(pixs, 1);
        pixGetDimensions(pixt, &w, &h, NULL);
    }
    else {
        pixt = pixClone(pixs);
    }
    scale = L_MIN(FILL_FACTOR * 2550 / w, FILL_FACTOR * 3300 / h);
    fname = genPathname("/tmp", "print_image.ps");
    fp = lept_fopen(fname, "wb+");
    pixWriteStreamPS(fp, pixt, NULL, 300, scale);
    lept_fclose(fp);

        /* print it out */
    if (argp && !argn) {
        sprintf(buffer, "lpr %s %s &", argp, fname);
        ignore = system(buffer);
    } else if (!argp && argn) {
        sprintf(buffer, "lpr %s %s &", argn, fname);
        ignore = system(buffer);
    } else if (argp && argn) {
        sprintf(buffer, "lpr %s %s %s &", argp, argn, fname);
        ignore = system(buffer);
    }

    lept_free(fname);
    pixDestroy(&pixs);
    pixDestroy(&pixt);
    return 0;
}

