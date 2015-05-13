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
 * pnmio_reg.c
 *
 *   Tests read and write of both ascii and packed pnm, using
 *   pix with 1, 2, 4, 8 and 32 bpp.
 */

#include "allheaders.h"

l_int32 main(l_int32  argc,
             char   **argv)
{
FILE         *fp;
PIX          *pix1, *pix2, *pix3, *pix4;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

        /* Test 1 bpp (pbm) read/write */
    pix1 = pixRead("char.tif");
    fp = lept_fopen("/tmp/pix1.1.pnm", "wb");
    pixWriteStreamAsciiPnm(fp, pix1);
    lept_fclose(fp);
    pix2 = pixRead("/tmp/pix1.1.pnm");
    pixWrite("/tmp/pix2.1.pnm", pix2, IFF_PNM);
    pix3 = pixRead("/tmp/pix2.1.pnm");
    regTestComparePix(rp, pix1, pix3);  /* 0 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);

        /* Test 2, 4 and 8 bpp (pgm) read/write */
    pix1 = pixRead("weasel8.png");
    pix2 = pixThresholdTo2bpp(pix1, 4, 0);
    fp = lept_fopen("/tmp/pix2.2.pnm", "wb");
    pixWriteStreamAsciiPnm(fp, pix2);
    lept_fclose(fp);
    pix3 = pixRead("/tmp/pix2.2.pnm");
    pixWrite("/tmp/pix3.2.pnm", pix3, IFF_PNM);
    pix4 = pixRead("/tmp/pix3.2.pnm");
    regTestComparePix(rp, pix2, pix4);  /* 1 */
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    pixDestroy(&pix4);

    pix2 = pixThresholdTo4bpp(pix1, 16, 0);
    fp = lept_fopen("/tmp/pix2.4.pnm", "wb");
    pixWriteStreamAsciiPnm(fp, pix2);
    lept_fclose(fp);
    pix3 = pixRead("/tmp/pix2.4.pnm");
    pixWrite("/tmp/pix3.4.pnm", pix3, IFF_PNM);
    pix4 = pixRead("/tmp/pix3.4.pnm");
    regTestComparePix(rp, pix2, pix4);  /* 2 */
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    pixDestroy(&pix4);

    fp = lept_fopen("/tmp/pix1.8.pnm", "wb");
    pixWriteStreamAsciiPnm(fp, pix1);
    lept_fclose(fp);
    pix2 = pixRead("/tmp/pix1.8.pnm");
    pixWrite("/tmp/pix2.8.pnm", pix2, IFF_PNM);
    pix3 = pixRead("/tmp/pix2.8.pnm");
    regTestComparePix(rp, pix1, pix3);  /* 3 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);

        /* Test ppm (24 bpp rgb) read/write */
    pix1 = pixRead("marge.jpg");
    fp = lept_fopen("/tmp/pix1.24.pnm", "wb");
    pixWriteStreamAsciiPnm(fp, pix1);
    lept_fclose(fp);
    pix2 = pixRead("/tmp/pix1.24.pnm");
    pixWrite("/tmp/pix2.24.pnm", pix2, IFF_PNM);
    pix3 = pixRead("/tmp/pix2.24.pnm");
    regTestComparePix(rp, pix1, pix3);  /* 4 */
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);

    return regTestCleanup(rp);
}
