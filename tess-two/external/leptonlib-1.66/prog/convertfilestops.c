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
 * convertfilestops.c
 *
 *    Converts all files in the given directory with matching substring
 *    to a level 3 compressed PostScript file, at the specified resolution.
 *
 *    To convert all files in the directory, use 'allfiles' for the substring.
 *
 *    To generate a ps that scales the images to fit a standard 8.5 x 11
 *    page, use res = 0.
 *
 *    Otherwise, this will convert at a specified resolution.
 *    Decreasing the resolution will cause the image to be rendered
 *    larger, and v.v.   For example, if the page was originally scanned
 *    at 400 ppi and you use 300 ppi for the resolution, the page will
 *    be rendered with larger pixels (i.e., be magnified) and you will
 *    lose a quarter of the page on the right side and a quarter
 *    at the bottom.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
char           *dirin, *substr, *fileout;
l_int32         res;
static char     mainName[] = "convertfilestops";

    if (argc != 5)
	exit(ERROR_INT(" Syntax:  convertfilestops dirin substr res fileout",
                       mainName, 1));

    dirin = argv[1];
    substr = argv[2];
    res = atoi(argv[3]);
    fileout = argv[4];
    if (!strcmp(substr, "allfiles"))
        substr = NULL;
    if (res != 0)
        return convertFilesToPS(dirin, substr, res, fileout);
    else
        return convertFilesFittedToPS(dirin, substr, 0.0, 0.0, fileout);
}


