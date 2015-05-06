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
 *   jp2kio_reg.c
 *
 *    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *    This is the Leptonica regression test for lossy read/write
 *    I/O in jp2k format.
 *    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *    This tests reading and writing of images in jp2k format.
 *
 *    * jp2k supports 8 bpp gray, rgb, and rgba.
 *    * This makes calls into the jpeg2000 library libopenjp2.
 *    * Compared to reading and writing jpeg, reading jp2k is slow
 *      and writing jp2k is very slow.
 */

#include <math.h>
#include "allheaders.h"

    /* Needed for HAVE_LIBJP2K */
#ifdef HAVE_CONFIG_H
#include <config_auto.h>
#endif /* HAVE_CONFIG_H */

void DoJp2kTest1(L_REGPARAMS *rp, const char *fname);


int main(int    argc,
         char **argv)
{
L_REGPARAMS  *rp;

#if !HAVE_LIBJP2K
    fprintf(stderr, "jp2kio is not enabled\n"
            "libopenjp2 is required for jp2kio_reg\n"
            "See environ.h: #define HAVE_LIBJP2K\n"
            "See prog/Makefile: link in -lopenjp2\n\n");
    return 0;
#endif  /* abort */

        /* This test uses libjpeg */
#if !HAVE_LIBJPEG
    fprintf(stderr, "libjpeg is required for jp2kio_reg\n\n");
    return 0;
#endif  /* abort */

    if (regTestSetup(argc, argv, &rp))
        return 1;

    lept_mkdir("lept");
    DoJp2kTest1(rp, "karen8.jpg");
    DoJp2kTest1(rp, "test24.jpg");

    return regTestCleanup(rp);
}


void DoJp2kTest1(L_REGPARAMS  *rp,
                 const char   *fname)
{
char  buf[256];
l_int32  w, h;
BOX     *box;
PIX     *pix1, *pix2, *pix3;

    pix1 = pixRead(fname);
    pixGetDimensions(pix1, &w, &h, NULL);
    box = boxCreate(w / 4, h / 4, w / 2, h / 2);
    snprintf(buf, sizeof(buf), "/tmp/lept/jp2kio.%03d.jp2", rp->index + 1);
    pixWrite(buf, pix1, IFF_JP2);
    regTestCheckFile(rp, buf);
    pix2 = pixRead(buf);
    pixDisplayWithTitle(pix2, 0, 100, "1", rp->display);
    pixDestroy(&pix1);
    pixDestroy(&pix2);

    pix1 = pixReadJp2k(buf, 1, box, 0);  /* just read the box region */
    snprintf(buf, sizeof(buf), "/tmp/lept/jp2kio.%03d.jp2", rp->index + 1);
    pixWriteJp2k(buf, pix1, 38, 0, 0);
    regTestCheckFile(rp, buf);
    pix2 = pixRead(buf);
    regTestWritePixAndCheck(rp, pix2, IFF_JP2);
    pixDisplayWithTitle(pix2, 500, 100, "2", rp->display);
    pix3 = pixReadJp2k(buf, 2, NULL, 0);  /* read image at 2x reduction */
    regTestWritePixAndCheck(rp, pix3, IFF_JP2);
    pixDisplayWithTitle(pix3, 1000, 100, "3", rp->display);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    return;
}

