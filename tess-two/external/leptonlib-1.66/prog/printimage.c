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
 * printimage.c
 *
 *   Syntax:  printimage filein [-P<printer> [-#<number>]
 *
 *   If you want the image printed, use the standard lpr flags
 *   for either (or both) the printer and the number of copies.
 *
 *   If neither a printer nor a number of copies is specified, the
 *   only action is that a new PostScript file,
 *          /tmp/junk_print_image.ps
 *   is generated for the image.
 *
 *   The PS file generated is level 1.  This is large, but will work
 *   on all PS printers.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const l_float32  FILL_FACTOR = 0.95;   /* fill factor on 8.5 x 11 page */


main(int    argc,
     char **argv)
{
char        *filein, *argp, *argn;
char         buffer[512];
l_int32      i, w, h;
l_float32    scale;
FILE        *fp;
PIX         *pixs, *pixt;
static char  mainName[] = "printimage";

    if (argc < 2 || argc > 4)
	exit(ERROR_INT(" Syntax:  printimage filein [-P<printer>] [-#<number>]",
                       mainName, 1));

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

    system("rm -f /tmp/junk_print_image.ps");

    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));

    pixGetDimensions(pixs, &w, &h, NULL);
    if (w > h) {
        pixt = pixRotate90(pixs, 1);
        pixGetDimensions(pixt, &w, &h, NULL);
    }
    else
        pixt = pixClone(pixs);
    scale = L_MIN(FILL_FACTOR * 2550 / w, FILL_FACTOR * 3300 / h);
    fp = fopen("/tmp/junk_print_image.ps", "wb+");
    pixWriteStreamPS(fp, pixt, NULL, 300, scale);
    fclose(fp);

        /* print it out */
    if (argp && !argn) {
        sprintf(buffer, "lpr %s /tmp/junk_print_image.ps &", argp);
        system(buffer);
    }
    else if (!argp && argn) {
        sprintf(buffer, "lpr %s /tmp/junk_print_image.ps &", argn);
        system(buffer);
    }
    else if (argp && argn) {
        sprintf(buffer, "lpr %s %s /tmp/junk_print_image.ps &",
                argp, argn);
        system(buffer);
    }

    pixDestroy(&pixs);
    pixDestroy(&pixt);
    exit(0);
}

