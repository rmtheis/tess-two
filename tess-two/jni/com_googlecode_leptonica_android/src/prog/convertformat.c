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
        lept_free(ext);
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

