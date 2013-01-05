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
 * fpixcontours.c
 *
 *   Generates and displays an fpix as a set of contours
 *
 *   Syntax: fpixcontours filein [ncontours]
 *   Default for ncontours is 40.
 */

#include <string.h>
#include "allheaders.h"

static const char *fileout = "/tmp/fpixcontours.png";

main(int    argc,
     char **argv)
{
char        *filein, *fileout;
l_int32      ncontours;
FPIX        *fpix;
PIX         *pix;
static char  mainName[] = "fpixcontours";

    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Syntax: fpixcontours filein [ncontours]\n");
        return 1;
    }

    filein = argv[1];
    if (argc == 2)
        ncontours = 40;
    else  /* argc == 3 */
        ncontours = atoi(argv[2]);

    if ((fpix = fpixRead(filein)) == NULL)
        return ERROR_INT(mainName, "fpix not read", 1);
    pix = fpixAutoRenderContours(fpix, ncontours);
    pixWrite(fileout, pix, IFF_PNG);
    pixDisplay(pix, 100, 100);

    pixDestroy(&pix);
    fpixDestroy(&fpix);
    return 0;
}
