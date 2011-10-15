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
 * convertformat.c
 *
 *   Converts an image file from one format to another.
 *
 *   Syntax: convertformat filein fileout [format]
 *
 *      where format is one of these:
 *
 *         BMP
 *         JPEG  (only applicable for 8 bpp or rgb)
 *         PNG
 *         TIFF
 *         TIFF_G4  (only applicable for 1 bpp)
 *         PNM
 *
 *   The output format can be chosen by the extension of fileout:
 *         BMP       .bmp
 *         JPEG      .jpg
 *         PNG       .png
 *         TIFF      .tif
 *         TIFF_G4   .tif
 *         PNM       .pnm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
PIX         *pixs;
char        *filein, *fileout, *base, *ext;
const char  *format;
char         error_msg[] = "Valid formats: BMP, JPEG, PNG, TIFF, TIFF_G4, PNM";
l_int32      d;
static char  mainName[] = "convertformat";

    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Syntax: convertformat filein fileout [format]\n");
        fprintf(stderr, "%s\n", error_msg);
        fprintf(stderr, "If you don't specify a format, the output file needs");
	fprintf(stderr, " an extension such as:\n");
	fprintf(stderr, " .bmp, .jpg, .png, .tif or .pnm\n");
        return 1;
    }

    filein = argv[1];
    fileout = argv[2];

    if (argc == 3) {
        splitPathAtExtension(fileout, NULL, &ext);
        if (!strcmp(ext, ".bmp"))
            format = "BMP";
        else if (!strcmp(ext, ".jpg"))
            format = "JPEG";
        else if (!strcmp(ext, ".png"))
            format = "PNG";
        else if (!strcmp(ext, ".tif"))
            format = "TIFF_G4";
        else if (!strcmp(ext, ".pnm"))
            format = "PNM";
        else
            return ERROR_INT(error_msg, mainName, 1);
        FREE(ext);
    }
    else
        format = argv[3];

    pixs = pixRead(filein);
    d = pixGetDepth(pixs);
    if (d != 1 && !strcmp(format, "TIFF_G4")) {
        L_WARNING("can't convert to tiff_g4; converting to tiff", mainName);
        format = "TIFF";
    }
    if (d < 8 && !strcmp(format, "JPEG")) {
        L_WARNING("can't convert to jpeg; converting to png", mainName);
        splitPathAtExtension(fileout, &base, &ext);
        fileout = stringJoin(base, ".png");
        format = "PNG";
    }

    if (strcmp(format, "BMP") == 0)
        pixWrite(fileout, pixs, IFF_BMP);
    else if (strcmp(format, "JPEG") == 0)
        pixWrite(fileout, pixs, IFF_JFIF_JPEG);
    else if (strcmp(format, "PNG") == 0)
        pixWrite(fileout, pixs, IFF_PNG);
    else if (strcmp(format, "TIFF") == 0)
        pixWrite(fileout, pixs, IFF_TIFF_ZIP);
    else if (strcmp(format, "TIFF_G4") == 0)
        pixWrite(fileout, pixs, IFF_TIFF_G4);
    else if (strcmp(format, "PNM") == 0)
        pixWrite(fileout, pixs, IFF_PNM);
    else
        return ERROR_INT(error_msg, mainName, 1);

    return 0;
}

