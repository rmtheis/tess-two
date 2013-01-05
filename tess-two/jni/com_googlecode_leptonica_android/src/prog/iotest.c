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
 * iotest.c
 *
 *   Tests all I/O except multipage/custom tiff and PostScript, which
 *   are separately tested in mtifftest and psiotest, respectively.
 */

#include "allheaders.h"

LEPT_DLL extern const char *ImageFileFormatExtensions[];

main(int    argc,
     char **argv)
{
l_int32      w, h, d, wpl, count, i, format, xres, yres;
FILE        *fp;
PIX         *pix, *pixt1, *pixt2;
PIXCMAP     *cmap;
char        *filein;
char        *fileout = NULL;
static char  mainName[] = "iotest";

    if (argc != 2 && argc != 3)
	exit(ERROR_INT(" Syntax:  iotest filein [fileout]", mainName, 1));

    filein = argv[1];
    if (argc == 3)
        fileout = argv[2];

#if 1
    if ((pix = pixRead(filein)) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));
#else
    if ((pix = pixReadJpeg(filein, 0, 4, NULL)) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));
#endif

    pixGetDimensions(pix, &w, &h, &d);
    wpl = pixGetWpl(pix);
    fprintf(stderr, "w = %d, h = %d, d = %d, wpl = %d\n", w, h, d, wpl);
    xres = pixGetXRes(pix);
    yres = pixGetXRes(pix);
    if (xres != 0 && yres != 0)
        fprintf(stderr, "xres = %d, yres = %d\n", xres, yres);
    if (pixGetColormap(pix)) {
	    /* Write and read back the colormap */
        pixcmapWriteStream(stderr, pixGetColormap(pix));
        fp = lept_fopen("/tmp/junkcmap1", "wb");
        pixcmapWriteStream(fp, pixGetColormap(pix));
        lept_fclose(fp);
        fp = lept_fopen("/tmp/junkcmap1", "rb");
        cmap = pixcmapReadStream(fp);
        lept_fclose(fp);
        fp = lept_fopen("/tmp/junkcmap2", "wb");
        pixcmapWriteStream(fp, cmap);
        lept_fclose(fp);
        pixcmapDestroy(&cmap);

            /* Remove and regenerate colormap */
        pixt1 = pixRemoveColormap(pix, REMOVE_CMAP_BASED_ON_SRC);
	if (pixGetDepth(pixt1) == 8) {
            fprintf(stderr, "Colormap: represents grayscale image\n");
            pixt2 = pixConvertGrayToColormap(pixt1);
	}
	else {  /* 32 bpp */
            fprintf(stderr, "Colormap: represents RGB image\n");
            pixt2 = pixConvertRGBToColormap(pixt1, 1);
	}
        pixWrite("/tmp/junkpixt2.png", pixt2, IFF_PNG);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }
    else {
        fprintf(stderr, "no colormap\n");
    }
    format = pixGetInputFormat(pix);
    fprintf(stderr, "Input format extension: %s\n",
            ImageFileFormatExtensions[format]);
    if (format == IFF_JFIF_JPEG)
        fprintf(stderr, "Jpeg comment: %s\n", pixGetText(pix));

    if (d == 1) {
        pixCountPixels(pix, &count, NULL);
        fprintf(stderr, "pixel ratio ON/OFF = %6.3f\n",
          (l_float32)count / (l_float32)(pixGetWidth(pix) * pixGetHeight(pix)));
    }

    if (argc == 3) {
#if 1
        d = pixGetDepth(pix);
        if (d == 16 || d < 8 || pixGetColormap(pix))
            pixWrite(fileout, pix, IFF_PNG);
        else
            pixWriteJpeg(fileout, pix, 75, 0);
#elif 0
        pixWrite(fileout, pix, IFF_BMP);
#elif 0
        pixWrite(fileout, pix, IFF_PNG);
#elif 0
        pixWrite(fileout, pix, IFF_TIFF);
        fprintTiffInfo(stderr, fileout);
#elif 0
        pixWrite(fileout, pix, IFF_TIFF_PACKBITS);
        fprintTiffInfo(stderr, fileout);
#elif 0
        pixWrite(fileout, pix, IFF_TIFF_G3);
        fprintTiffInfo(stderr, fileout);
#elif 0
        pixWrite(fileout, pix, IFF_TIFF_G4);
        fprintTiffInfo(stderr, fileout);
#elif 0
        pixWrite(fileout, pix, IFF_JFIF_JPEG);
#elif 0
        pixWriteJpeg(fileout, pix, 75, 0);
#elif 0
        pixWrite(fileout, pix, IFF_PNM);
#elif 0
        pixWrite(fileout, pix, IFF_PS);
#endif
    }

    pixDestroy(&pix);

#if 0   /* test tiff header reader */
{ l_int32 w, h, bps, spp, res, cmap;
    if (readHeaderTiff(filein, 0, &w, &h, &bps, &spp, &res, &cmap) == 0)
        fprintf(stderr,
        "w = %d, h = %d, bps = %d, spp = %d, res = %d, cmap = %d\n",
        w, h, bps, spp, res, cmap);
}
#endif

    return 0;
}

