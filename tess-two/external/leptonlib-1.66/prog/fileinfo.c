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
 * fileinfo.c
 *
 *   Returns information about the image data file
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

LEPT_DLL extern const char *ImageFileFormatExtensions[];

main(int    argc,
     char **argv)
{
char        *text;
l_int32      w, h, d, wpl, count, npages, color, format;
FILE        *fp;
PIX         *pix;
PIXCMAP     *cmap;
char        *filein;
static char  mainName[] = "fileinfo";

    if (argc != 2)
	exit(ERROR_INT(" Syntax:  fileinfo filein", mainName, 1));
    filein = argv[1];

    l_pngSetStrip16To8(0);  /* to preserve 16 bpp if format is png */
    if ((pix = pixRead(filein)) == NULL)
	exit(ERROR_INT("image not returned from file", mainName, 1));

    format = pixGetInputFormat(pix);
    fprintf(stderr, "Input image format type: %s\n",
            ImageFileFormatExtensions[format]);
    pixGetDimensions(pix, &w, &h, &d);
    wpl = pixGetWpl(pix);
    fprintf(stderr, "w = %d, h = %d, d = %d, wpl = %d\n", w, h, d, wpl);
    fprintf(stderr, "xres = %d, yres = %d\n", pixGetXRes(pix), pixGetYRes(pix));

    text = pixGetText(pix);
    if (text)  /*  not null */
        fprintf(stderr, "Text: %s\n", text);

    cmap = pixGetColormap(pix);
    if (cmap) {
        pixcmapHasColor(cmap, &color);
        if (color)
            fprintf(stderr, "Colormap exists and has color values:");
        else
            fprintf(stderr, "Colormap exists and has only gray values:");
	pixcmapWriteStream(stderr, pixGetColormap(pix));
    }
    else
	fprintf(stderr, "Colormap does not exist.\n");

    if (format == IFF_TIFF || format == IFF_TIFF_G4 ||
        format == IFF_TIFF_G3 || format == IFF_TIFF_PACKBITS) {
        fprintf(stderr, "Tiff header information:\n");
        fp = fopen(filein, "r");
        tiffGetCount(fp, &npages);
        fclose(fp);
        if (npages == 1)
            fprintf(stderr, "One page in file\n", npages);
        else
            fprintf(stderr, "%d pages in file\n", npages);
        fprintTiffInfo(stderr, filein);
    }

    if (d == 1) {
        pixCountPixels(pix, &count, NULL);
	fprintf(stderr, "pixel ratio ON/OFF = %6.3f\n",
          (l_float32)count / (l_float32)(pixGetWidth(pix) * pixGetHeight(pix)));
    }

    pixDestroy(&pix);
    exit(0);
}

