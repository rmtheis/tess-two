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
 * pixcomp_reg.c
 *
 *    Regression test for compressed pix and compressed pix arrays
 *    in memory.
 *
 *    Most of the functions tested here requires the ability to write
 *    a pix to a compressed string in memory, and conversely to
 *    read a compressed image string from memory to generate a pix.
 *    This functionality is not enabled by default, because it requires
 *    the gnu runtime.  If we detect a failure, we bomb out early.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allheaders.h"


LEPT_DLL extern const char *ImageFileFormatExtensions[];
static void get_format_data(l_int32 i, l_uint8 *data, size_t size);

#define  DO_PNG     1  /* set to 0 for valgrind to remove most png errors */

main(int    argc,
     char **argv)
{
l_int32     i, n;
BOX        *box;
PIX        *pix, *pixs, *pixd, *pixd2;
PIXA       *pixad, *pixa1, *pixa2, *pixa3;
PIXC       *pixc;
PIXAC      *pixac, *pixac1, *pixac2;
static char     mainName[] = "pixcomp_reg";

    pixad = pixaCreate(0);

        /* --- Read in the images --- */
    pixac = pixacompCreate(1);
    pixs = pixRead("marge.jpg");
    pixc = pixcompCreateFromPix(pixs, IFF_JFIF_JPEG);
    if (!pixc) {
        L_ERROR("Exiting because jpeg write to memory is not enabled", mainName);
        return 1;
    }
    pixd = pixCreateFromPixcomp(pixc);
    pixSaveTiledOutline(pixd, pixad, 1, 1, 30, 2, 32);
    pixDestroy(&pixd);
    pixcompDestroy(&pixc);
    pixacompAddPix(pixac, pixs, IFF_DEFAULT);
    pixDestroy(&pixs);

    pix = pixRead("feyn.tif");
    pixs = pixScaleToGray6(pix);
    pixc = pixcompCreateFromPix(pixs, IFF_JFIF_JPEG);
    pixd = pixCreateFromPixcomp(pixc);
    pixSaveTiledOutline(pixd, pixad, 1, 0, 30, 2, 32);
    pixDestroy(&pixd);
    pixcompDestroy(&pixc);
    pixacompAddPix(pixac, pixs, IFF_DEFAULT);
    pixDestroy(&pixs);

    box = boxCreate(1144, 611, 690, 180);
    pixs = pixClipRectangle(pix, box, NULL);
    pixc = pixcompCreateFromPix(pixs, IFF_TIFF_G4);
    pixd = pixCreateFromPixcomp(pixc);
    pixSaveTiledOutline(pixd, pixad, 1, 0, 30, 2, 32);
    pixDestroy(&pixd);
    pixcompDestroy(&pixc);
    pixacompAddPix(pixac, pixs, IFF_DEFAULT);
    pixDestroy(&pixs);
    boxDestroy(&box);
    pixDestroy(&pix);

#if DO_PNG
    pixs = pixRead("weasel4.11c.png");
    pixc = pixcompCreateFromPix(pixs, IFF_PNG);
    pixd = pixCreateFromPixcomp(pixc);
    pixSaveTiledOutline(pixd, pixad, 1, 0, 30, 2, 32);
    pixDestroy(&pixd);
    pixcompDestroy(&pixc);
    pixacompAddPix(pixac, pixs, IFF_DEFAULT);
    pixDestroy(&pixs);
#endif

        /* --- Retrieve to pix --- */
    n = pixacompGetCount(pixac);
    for (i = 0; i < n; i++) {
        pixs = pixacompGetPix(pixac, i);
        pixSaveTiledOutline(pixs, pixad, 1, i == 0 ? 1 : 0, 30, 2, 32);
        pixDestroy(&pixs);
    }

        /* --- Retrieve to pixa --- */
    pixa1 = pixaCreateFromPixacomp(pixac, L_CLONE);
    for (i = 0; i < n; i++) {
        pixs = pixaGetPix(pixa1, i, L_CLONE);
        pixSaveTiledOutline(pixs, pixad, 1, i == 0 ? 1 : 0, 30, 2, 32);
        pixDestroy(&pixs);
    }

        /* --- Do (pixa <==> pixac) conversions --- */
    pixaWrite("/tmp/junkpixa1.pa", pixa1);
    pixac1 = pixacompCreateFromPixa(pixa1, IFF_DEFAULT, L_CLONE);
    pixa2 = pixaCreateFromPixacomp(pixac1, L_CLONE);
    pixaWrite("/tmp/junkpixa2.pa", pixa2);
    pixac2 = pixacompCreateFromPixa(pixa2, IFF_DEFAULT, L_CLONE);
    pixa3 = pixaCreateFromPixacomp(pixac2, L_CLONE);
    pixaWrite("/tmp/junkpixa3.pa", pixa3);

        /* --- Extract formatting info from compressed strings --- */
    for (i = 0; i < n; i++) {
        pixc = pixacompGetPixcomp(pixac1, i);
        get_format_data(i, pixc->data, pixc->size);
    }

        /* --- Display results --- */
    pixd = pixaDisplay(pixad, 0, 0);
    pixDisplay(pixd, 100, 100);
    pixWrite("/tmp/junkcomp.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixad);

    pixaDestroy(&pixa1);
    pixaDestroy(&pixa2);
    pixaDestroy(&pixa3);
    pixacompDestroy(&pixac);
    pixacompDestroy(&pixac1);
    pixacompDestroy(&pixac2);

        /* --- Read all the 'weasel' files and display results --- */
    pixac = pixacompCreateFromFiles(".", "weasel", IFF_DEFAULT);
    fprintf(stderr, "found %d weasel files\n", pixacompGetCount(pixac));
    pixcompWriteStreamInfo(stderr, pixac->pixc[7], NULL);
    pixd = pixacompDisplayTiledAndScaled(pixac, 32, 100, 8, 0, 15, 2);
    pixWrite("/tmp/junkweasel.jpg", pixd, IFF_JFIF_JPEG);
    pixDisplay(pixd, 100, 100);
    pixacompDestroy(&pixac);
    pixDestroy(&pixd);

        /* --- Use serialized I/O on the pixacomp --- */
    pixac = pixacompCreateFromFiles(".", "hardlight", IFF_DEFAULT);
    fprintf(stderr, "found %d jpg files\n", pixacompGetCount(pixac));
    pixd = pixacompDisplayTiledAndScaled(pixac, 32, 200, 6, 0, 15, 2);
    pixWrite("/tmp/junkhardlight.png", pixd, IFF_PNG);
    pixDisplay(pixd, 100, 300);
    pixacompWrite("/tmp/junkpixac1.pa", pixac);
    pixac2 = pixacompRead("/tmp/junkpixac1.pa");
    pixacompWrite("/tmp/junkpixac2.pa", pixac2);
    pixd2 = pixacompDisplayTiledAndScaled(pixac2, 32, 1200, 4, 0, 30, 2);
    pixDisplay(pixd2, 500, 300);
    pixacompWriteStreamInfo(stderr, pixac2, NULL);
    pixacompDestroy(&pixac);
    pixacompDestroy(&pixac2);
    pixDestroy(&pixd);
    pixDestroy(&pixd2);

        /* --- Read all the 'tif' files and display results --- */
    pixac = pixacompCreateFromFiles(".", "tif", IFF_DEFAULT);
    fprintf(stderr, "found %d tiff files\n", pixacompGetCount(pixac));
    pixcompWriteStreamInfo(stderr, pixac->pixc[0], NULL);
    pixd = pixacompDisplayTiledAndScaled(pixac, 32, 200, 6, 0, 15, 2);
    pixWrite("/tmp/junktiffs.png", pixd, IFF_PNG);
    pixDisplay(pixd, 100, 500);
    pixacompDestroy(&pixac);
    pixDestroy(&pixd);

    return 0;
}


static void
get_format_data(l_int32   i,
                l_uint8  *data,
                size_t    size)
{
l_int32  ret, format, w, h, d, bps, spp, iscmap;

    ret = pixReadHeaderMem(data, size, &format, &w, &h, &bps, &spp, &iscmap);
    d = bps * spp;
    if (d == 24) d = 32;
    if (ret)
        fprintf(stderr, "Error: couldn't read data: size = %d\n", size);
    else
        fprintf(stderr, "Format data for image %d:\n"
                "  format: %s, size (w, h, d) = (%d, %d, %d)\n"
                "  bps = %d, spp = %d, iscmap = %d\n",
                i, ImageFileFormatExtensions[format], w, h, d,
                bps, spp, iscmap);
    return;
}

