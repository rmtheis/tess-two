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
 *    webp supports 32 bpp rgb and rgba.
 *    Lossy writing is slow; reading is fast, comparable to reading jpeg files.
 *    Lossless writing is extremely slow.
 */

#include <math.h>
#include "allheaders.h"

    /* Needed for HAVE_LIBWEBP */
#ifdef HAVE_CONFIG_H
#include <config_auto.h>
#endif /* HAVE_CONFIG_H */

void DoWebpTest1(L_REGPARAMS *rp, const char *fname);
void DoWebpTest2(L_REGPARAMS *rp, const char *fname, l_int32 quality,
                 l_int32 lossless, l_float32 expected, l_float32 delta);


int main(int    argc,
         char **argv)
{
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

    DoWebpTest2(rp, "test24.jpg", 50, 0, 43.217, 0.1);
    DoWebpTest2(rp, "test24.jpg", 75, 0, 45.759, 0.1);
    DoWebpTest2(rp, "test24.jpg", 90, 0, 52.066, 0.1);
    DoWebpTest2(rp, "test24.jpg", 100, 0, 57.879, 0.1);
    DoWebpTest2(rp, "test24.jpg", 0, 1, 1000.0, 0.1);

    return regTestCleanup(rp);
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
                 l_int32       lossless,
                 l_float32     expected,
                 l_float32     delta)
{
char       buf[256];
l_float32  psnr;
PIX       *pixs, *pix1;

    pixs = pixRead(fname);
    snprintf(buf, sizeof(buf), "/tmp/webpio.%d.webp", rp->index + 1);
    if (lossless) startTimer();
    pixWriteWebP("/tmp/junk.webp", pixs, quality, lossless);
    if (lossless) fprintf(stderr, "Lossless write: %7.3f sec\n", stopTimer());
    pix1 = pixRead("/tmp/junk.webp");
    pixGetPSNR(pixs, pix1, 4, &psnr);
    if (lossless)
        fprintf(stderr, "lossless; psnr should be 1000: psnr = %7.3f\n", psnr);
    else
        fprintf(stderr, "qual = %d, psnr = %7.3f\n", quality, psnr);
    regTestCompareValues(rp, expected, psnr, delta);
    pixDestroy(&pixs);
    pixDestroy(&pix1);
    return;
}
