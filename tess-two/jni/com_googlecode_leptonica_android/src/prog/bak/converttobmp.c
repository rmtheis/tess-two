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
 * converttobmp.c
 *
 *   Converts an image file to bmp if it's not already in that format.
 *
 *   Syntax: converttobmp filein [fileout]
 *
 *   Return: 0 if conversion took place; 1 if no conversion
 */


#include <string.h>
#include "allheaders.h"

main(int    argc,
     char **argv)
{
char        *filein, *fileout;
l_int32      format;
PIX         *pixs;
static char  mainName[] = "converttobmp";

    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Syntax: converttobmp filein [fileout]\n");
        return 1;
    }

    filein = argv[1];
    if (argc == 2)
        fileout = (char *)"/tmp/fileout.bmp";
    else  /* argc == 3 */
        fileout = argv[2];

    pixReadHeader(filein, &format, NULL, NULL, NULL, NULL, NULL);
    if (format == IFF_BMP)
        return 1;  /* no conversion required */

    pixs = pixRead(filein);
    pixWrite(fileout, pixs, IFF_BMP);
    return 0;
}
