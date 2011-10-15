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
 *  binarize_reg.c
 *
 *     Tests Sauvola local binarization and variants
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

PIX *PixTest1(PIX *pixs, l_int32 size, l_float32 factor,
              l_int32 *pcount, L_REGPARAMS  *rp);
PIX *PixTest2(PIX *pixs, l_int32 size, l_float32 factor, l_int32 nx,
              l_int32 ny, l_int32 *pcount, L_REGPARAMS *rp);
void PixTest3(PIX *pixs, l_int32 size, l_float32 factor,
              l_int32 nx, l_int32 ny, l_int32 paircount,
              l_int32 *pcount, L_REGPARAMS *rp);

main(int    argc,
     char **argv)
{
l_int32       count, success, display;
FILE         *fp;
PIX          *pixs, *pixt1, *pixt2;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &fp, &display, &success, &rp))
        return 1;

    pixs = pixRead("w91frag.jpg");
    count = 0;

    PixTest3(pixs, 3, 0.20, 2, 3, 0, &count, rp);
    PixTest3(pixs, 6, 0.20, 100, 100, 1, &count, rp);
    PixTest3(pixs, 10, 0.40, 10, 10, 2, &count, rp);
    PixTest3(pixs, 10, 0.40, 20, 20, 3, &count, rp);
    PixTest3(pixs, 20, 0.34, 30, 30, 4, &count, rp);

    pixt1 = PixTest1(pixs, 7, 0.34, &count, rp);
    pixt2 = PixTest2(pixs, 7, 0.34, 4, 4, &count, rp);
    regTestComparePix(fp, argv, pixt1, pixt2, 5, &success);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

        /* Do combination of contrast norm and sauvola */
    pixt1 = pixContrastNorm(NULL, pixs, 100, 100, 55, 1, 1);
    pixSauvolaBinarizeTiled(pixt1, 8, 0.34, 1, 1, NULL, &pixt2);
    regTestWritePixAndCheck(pixt1, IFF_PNG, &count, rp);
    regTestWritePixAndCheck(pixt2, IFF_PNG, &count, rp);
    pixDisplayWithTitle(pixt1, 100, 500, NULL, rp->display);
    pixDisplayWithTitle(pixt2, 700, 500, NULL, rp->display);

    regTestCleanup(argc, argv, fp, success, rp);
    pixDestroy(&pixs);
    return 0;
}


PIX *
PixTest1(PIX          *pixs,
         l_int32       size,
         l_float32     factor,
         l_int32      *pcount,
         L_REGPARAMS  *rp)
{
l_int32  w, h;
PIX     *pixm, *pixsd, *pixth, *pixd, *pixt;
PIXA    *pixa;

    pixm = pixsd = pixth = pixd = NULL;
    pixGetDimensions(pixs, &w, &h, NULL);

        /* Get speed */
    startTimer();
    pixSauvolaBinarize(pixs, size, factor, 1, NULL, NULL, NULL, &pixd);
    fprintf(stderr, "\nSpeed: 1 tile,  %7.3f Mpix/sec\n",
            (w * h / 1000000.) / stopTimer());
    pixDestroy(&pixd);

        /* Get results */
    pixSauvolaBinarize(pixs, size, factor, 1, &pixm, &pixsd, &pixth, &pixd);
    pixa = pixaCreate(0);
    pixSaveTiled(pixm, pixa, 1, 1, 30, 8);
    pixSaveTiled(pixsd, pixa, 1, 0, 30, 8);
    pixSaveTiled(pixth, pixa, 1, 1, 30, 8);
    pixSaveTiled(pixd, pixa, 1, 0, 30, 8);
    pixt = pixaDisplay(pixa, 0, 0);
    regTestWritePixAndCheck(pixt, IFF_JFIF_JPEG, pcount, rp);
    if (*pcount < 5)
        pixDisplayWithTitle(pixt, 100, 100, NULL, rp->display);
    regTestWritePixAndCheck(pixd, IFF_PNG, pcount, rp);

    pixaDestroy(&pixa);
    pixDestroy(&pixm);
    pixDestroy(&pixsd);
    pixDestroy(&pixth);
    pixDestroy(&pixt);
    return pixd;
}
   

PIX *
PixTest2(PIX          *pixs,
         l_int32       size,
         l_float32     factor,
         l_int32       nx,
         l_int32       ny,
         l_int32      *pcount,
         L_REGPARAMS  *rp)
{
l_int32  w, h;
PIX     *pixth, *pixd, *pixt;
PIXA    *pixa;

    pixth = pixd = NULL;
    pixGetDimensions(pixs, &w, &h, NULL);

        /* Get speed */
    startTimer();
    pixSauvolaBinarizeTiled(pixs, size, factor, nx, ny, NULL, &pixd);
    fprintf(stderr, "Speed: %d x %d tiles,  %7.3f Mpix/sec\n",
            nx, ny, (w * h / 1000000.) / stopTimer());
    pixDestroy(&pixd);

        /* Get results */
    pixSauvolaBinarizeTiled(pixs, size, factor, nx, ny, &pixth, &pixd);
    regTestWritePixAndCheck(pixth, IFF_JFIF_JPEG, pcount, rp);
    regTestWritePixAndCheck(pixd, IFF_PNG, pcount, rp);
    if (*pcount < 5 && rp->display) {
        pixa = pixaCreate(0);
        pixSaveTiled(pixth, pixa, 1, 1, 30, 8);
        pixSaveTiled(pixd, pixa, 1, 0, 30, 8);
        pixt = pixaDisplay(pixa, 0, 0);
        pixDisplayWithTitle(pixt, 100, 400, NULL, rp->display);
        pixDestroy(&pixt);
        pixaDestroy(&pixa);
    }

    pixDestroy(&pixth);
    return pixd;
}
 

void
PixTest3(PIX          *pixs,
         l_int32       size,
         l_float32     factor,
         l_int32       nx,
         l_int32       ny,
         l_int32       paircount,
         l_int32      *pcount,
         L_REGPARAMS  *rp)
{
PIX  *pixt1, *pixt2;

    pixt1 = PixTest1(pixs, size, factor, pcount, rp);
    pixt2 = PixTest2(pixs, size, factor, nx, ny, pcount, rp);
    regTestComparePix(rp->fp, rp->argv, pixt1, pixt2, paircount, &rp->success);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    return;
}

