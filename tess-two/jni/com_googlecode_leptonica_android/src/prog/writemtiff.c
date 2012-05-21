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
 * writemtiff.c
 *
 *   Writes all matched files into a multipage tiff
 */

#include "allheaders.h"

main(int    argc,
     char **argv)
{
char        *dirin, *pattern, *fileout;
static char  mainName[] = "writemtiff";

    if (argc != 4)
	exit(ERROR_INT(" Syntax:  writemtiff dirin pattern fileout",
                       mainName, 1));

    dirin = argv[1];
    pattern = argv[2];
    fileout = argv[3];

    writeMultipageTiff(dirin, pattern, fileout);
    return 0;
}

