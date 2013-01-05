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
 *  coloring_reg.c
 *
 *   This tests simple coloring functions.
 */

#include "string.h"
#include "allheaders.h"

static const char *bgcolors[] = {"255 255 235",
                                 "255 245 235",
                                 "255 235 245",
                                 "235 245 255"};


l_int32 main(int    argc,
             char **argv)
{
char          buf[512];
l_int32       i, n, index;
l_int32       rval[4], gval[4], bval[4];
l_uint32      scolor, dcolor;
L_BMF        *bmf;
PIX          *pix0, *pix1, *pix2, *pix3, *pix4, *pix5;
PIXA         *pixa;
PIXCMAP      *cmap;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

        /* Read in the bg colors */
    for (i = 0; i < 4; i++)
        sscanf(bgcolors[i], "%d %d %d", &rval[i], &gval[i], &bval[i]);
    bmf = bmfCreate("fonts", 8);

        /* Get the input image (100 ppi resolution) */
    pix0 = pixRead("harmoniam100-11.png");
    cmap = pixGetColormap(pix0);
    pixa = pixaCreate(0);

        /* Do cmapped coloring on the white pixels only */
    pixcmapGetIndex(cmap, 255, 255, 255, &index);  /* index of white pixels */
    for (i = 0; i < 4; i++) {
        pixcmapResetColor(cmap, index, rval[i], gval[i], bval[i]);
        snprintf(buf, sizeof(buf), "(rval, bval, gval) = (%d, %d, %d)",
                 i + 1, rval[i], gval[i], bval[i]);
        pix1 = pixAddSingleTextblock(pix0, bmf, buf, 0xff000000,
                                     L_ADD_AT_BOTTOM, NULL);
        pixaAddPix(pixa, pix1, L_INSERT);
    }

        /* Do cmapped background coloring on all the pixels */
    for (i = 0; i < 4; i++) {
        scolor = 0xffffff00;  /* source color */
        composeRGBPixel(rval[i], gval[i], bval[i], &dcolor);  /* dest color */
        pix1 = pixShiftByComponent(NULL, pix0, scolor, dcolor);
        snprintf(buf, sizeof(buf), "(rval, bval, gval) = (%d, %d, %d)",
                 i + 1, rval[i], gval[i], bval[i]);
        pix2 = pixAddSingleTextblock(pix1, bmf, buf, 0xff000000,
                                     L_ADD_AT_BOTTOM, NULL);
        pixaAddPix(pixa, pix2, L_INSERT);
        pixDestroy(&pix1);
    }


        /* Do background coloring on rgb */
    pix1 = pixConvertTo32(pix0);
    for (i = 0; i < 4; i++) {
        scolor = 0xffffff00;
        composeRGBPixel(rval[i], gval[i], bval[i], &dcolor);
        pix2 = pixShiftByComponent(NULL, pix1, scolor, dcolor);
        snprintf(buf, sizeof(buf), "(rval, bval, gval) = (%d, %d, %d)",
                 i + 1, rval[i], gval[i], bval[i]);
        pix3 = pixAddSingleTextblock(pix2, bmf, buf, 0xff000000,
                                     L_ADD_AT_BOTTOM, NULL);
        pixaAddPix(pixa, pix3, L_INSERT);
        pixDestroy(&pix2);
    }
    pixDestroy(&pix1);

        /* Compare cmapped & rgb foreground coloring */
    scolor = 0x0;  /* source color */
    composeRGBPixel(200, 30, 150, &dcolor);  /* ugly fg dest color */
    pix1 = pixShiftByComponent(NULL, pix0, scolor, dcolor);  /* cmapped */
    snprintf(buf, sizeof(buf), "(rval, bval, gval) = (%d, %d, %d)",
             200, 100, 50);
    pix2 = pixAddSingleTextblock(pix1, bmf, buf, 0xff000000,
                                 L_ADD_AT_BOTTOM, NULL);
    pixaAddPix(pixa, pix2, L_INSERT);
    pix3 = pixConvertTo32(pix0);
    pix4 = pixShiftByComponent(NULL, pix3, scolor, dcolor);  /* rgb */
    snprintf(buf, sizeof(buf), "(rval, bval, gval) = (%d, %d, %d)",
             200, 100, 50);
    pix5 = pixAddSingleTextblock(pix4, bmf, buf, 0xff000000,
                                 L_ADD_AT_BOTTOM, NULL);
    pixaAddPix(pixa, pix5, L_INSERT);
    regTestComparePix(rp, pix1, pix4);
    regTestComparePix(rp, pix2, pix5);
    pixDestroy(&pix1);
    pixDestroy(&pix3);
    pixDestroy(&pix4);

        /* Log all the results */
    n = pixaGetCount(pixa);
    for (i = 0; i < n; i++) {
        pix1 = pixaGetPix(pixa, i, L_CLONE);
        regTestWritePixAndCheck(rp, pix1, IFF_PNG);
        pixDestroy(&pix1);
    }

        /* If in testing mode, make a pdf */
    if (rp->display) {
        pixaConvertToPdf(pixa, 100, 1.0, L_FLATE_ENCODE, 0,
                         "Colored background", "/tmp/coloring.pdf");
    }

    pixaDestroy(&pixa);
    pixDestroy(&pix0);
    bmfDestroy(&bmf);
    return regTestCleanup(rp);
}
