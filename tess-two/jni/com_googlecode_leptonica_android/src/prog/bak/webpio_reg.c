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
 *   webpio_reg.c
 *
 *    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *    This is the Leptonica regression test for lossy read/write
 *    I/O in webp format.
 *    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *    This tests reading and writing of images in webp format.
 *         http://code.google.com/speed/webp/index.html
 *
 *    webp currently only supports 32 bpp rgb.  Writing is very
 *    slow; reading is fast, comparable to reading jpeg files.
 */

#include <math.h>
#include "allheaders.h"


    /* Needed for HAVE_LIBWEBP */
#ifdef HAVE_CONFIG_H
#include <config_auto.h>
#endif /* HAVE_CONFIG_H */

void DoWebpTest1(L_REGPARAMS *rp, const char *fname);
void DoWebpTest2(L_REGPARAMS *rp, const char *fname, l_int32 quality,
                 l_float32 expected, l_float32 delta);


main(int    argc,
char **argv)
{
l_int32       success;
L_REGPARAMS  *rp;
 
#if !HAVE_LIBWEBP
    fprintf(stderr, "webpio is not enabled\n"
            "libwebp is required for webpio_reg\n"
            "See environ.h: #define HAVE_LIBWEBP\n"
            "See prog/Makefile: link in -lwebp\n\n");
    return 0;
#endif  /* abort */

        /* This test uses libjpeg */
#if !HAVE_LIBJPEG
    fprintf(stderr, "libjpeg is required for webpio_reg\n\n");
    return 0;
#endif  /* abort */

    if (regTestSetup(argc, argv, &rp))
        return 1;

    DoWebpTest1(rp, "weasel2.4c.png");
    DoWebpTest1(rp, "weasel8.240c.png");
    DoWebpTest1(rp, "karen8.jpg");
    DoWebpTest1(rp, "test24.jpg");

    DoWebpTest2(rp, "test24.jpg", 50, 43.217, 0.1);
    DoWebpTest2(rp, "test24.jpg", 75, 45.989, 0.1);
    DoWebpTest2(rp, "test24.jpg", 90, 52.243, 0.1);

    regTestCleanup(rp);
    return 0;
}


void DoWebpTest1(L_REGPARAMS  *rp,
                 const char   *fname)
{
char  buf[256];
PIX  *pixs, *pix1;

    startTimer();
    pixs = pixRead(fname);
    fprintf(stderr, "Time to read jpg: %7.3f\n", stopTimer());
    startTimer();
    snprintf(buf, sizeof(buf), "/tmp/webpio.%d.webp", rp->index + 1);
    pixWrite(buf, pixs, IFF_WEBP);
    fprintf(stderr, "Time to write webp: %7.3f\n", stopTimer());
    regTestCheckFile(rp, buf);
    startTimer();
    pix1 = pixRead(buf);
    fprintf(stderr, "Time to read webp: %7.3f\n", stopTimer());
    pixDisplayWithTitle(pix1, 100, 100, "pix1", 1);
    pixDestroy(&pixs);
    pixDestroy(&pix1);
    return;
}

void DoWebpTest2(L_REGPARAMS  *rp,
                 const char   *fname,
                 l_int32       quality,
                 l_float32     expected,
                 l_float32     delta)
{
char       buf[256];
l_float32  psnr;
PIX       *pixs, *pix1;

    pixs = pixRead(fname);
    snprintf(buf, sizeof(buf), "/tmp/webpio.%d.webp", rp->index + 1);
    pixWriteWebP("/tmp/junk.webp", pixs, quality);
    pix1 = pixRead("/tmp/junk.webp");
    pixGetPSNR(pixs, pix1, 4, &psnr);
    fprintf(stderr, "qual = %d, psnr = %7.3f\n", quality, psnr);
    regTestCompareValues(rp, expected, psnr, delta);
    pixDestroy(&pixs);
    pixDestroy(&pix1);
    return;
}
