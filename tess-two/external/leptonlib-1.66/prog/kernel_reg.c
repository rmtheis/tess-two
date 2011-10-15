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
 * kernel_reg.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef  _WIN32
#include <unistd.h>
#else
    /* Need declaration of Sleep() defined in WinBase.h, but must
     * include Windows.h to avoid errors  */
#include <Windows.h>
#endif  /* _WIN32 */
#include "allheaders.h"

static const char  *kdatastr = " 20.3    50   80  50   20 "
                               " 51.4   100  140  100  50 "
                               " 92.5   160  200  160  90 "
                               " 53.7   100  140  100  50 "
                               " 24.9    50   80   50  20 ";

main(int    argc,
     char **argv)
{
char        *str;
l_int32      i, j, same, ok, success, display;
l_float32    sum, avediff, rmsdiff;
FILE        *fp;
L_KERNEL    *kel1, *kel2, *kel3, *kel4, *kelx, *kely;
BOX         *box;
PIX         *pix, *pixs, *pixb, *pixg, *pixr, *pixd, *pixp, *pixt;
PIX         *pixt1, *pixt2, *pixt3;
PIXA        *pixa;
SARRAY      *sa;

    if (regTestSetup(argc, argv, &fp, &display, &success, NULL))
              return 1;

    pixa = pixaCreate(0);

        /* Test creating from a string */
    kel1 = kernelCreateFromString(5, 5, 2, 2, kdatastr);
    pixd = kernelDisplayInPix(kel1, 41, 2);
    pixWrite("/tmp/pixkern.png", pixd, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/pixkern.png", 0, &success);
    pixSaveTiled(pixd, pixa, 1, 1, 20, 8);
    pixDestroy(&pixd);
    kernelDestroy(&kel1);

        /* Test read/write for kernel.  Note that both get
         * compared to the same golden file, which is
         * overwritten with a copy of /tmp/kern2.kel */
    kel1 = kernelCreateFromString(5, 5, 2, 2, kdatastr);
    kernelWrite("/tmp/kern1.kel", kel1);
    regTestCheckFile(fp, argv, "/tmp/kern1.kel", 1, &success);
    kel2 = kernelRead("/tmp/kern1.kel");
    kernelWrite("/tmp/kern2.kel", kel2);
    regTestCheckFile(fp, argv, "/tmp/kern2.kel", 2, &success);
    regTestCompareFiles(fp, argv, 1, 2, &success);
    kernelDestroy(&kel1);
    kernelDestroy(&kel2);

        /* Test creating from a file */
    sa = sarrayCreate(0);
    sarrayAddString(sa, (char *)"# small 3x3 kernel", L_COPY);
    sarrayAddString(sa, (char *)"3 5", L_COPY);
    sarrayAddString(sa, (char *)"1 2", L_COPY);
    sarrayAddString(sa, (char *)"20.5   50   80    50   20", L_COPY);
    sarrayAddString(sa, (char *)"82.    120  180   120  80", L_COPY);
    sarrayAddString(sa, (char *)"22.1   50   80    50   20", L_COPY);
    str = sarrayToString(sa, 1);
    arrayWrite("/tmp/kernfile.kel", "w", str, strlen(str));
    kel2 = kernelCreateFromFile("/tmp/kernfile.kel");
    pixd = kernelDisplayInPix(kel2, 41, 2);
    pixSaveTiled(pixd, pixa, 1, 1, 20, 0);
    pixWrite("/tmp/ker1.png", pixd, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/ker1.png", 3, &success);
    pixDestroy(&pixd);
    sarrayDestroy(&sa);
    FREE(str);
    kernelDestroy(&kel2);

        /* Test creating from a pix */
    pixt = pixCreate(5, 3, 8);
    pixSetPixel(pixt, 0, 0, 20);
    pixSetPixel(pixt, 1, 0, 50);
    pixSetPixel(pixt, 2, 0, 80);
    pixSetPixel(pixt, 3, 0, 50);
    pixSetPixel(pixt, 4, 0, 20);
    pixSetPixel(pixt, 0, 1, 80);
    pixSetPixel(pixt, 1, 1, 120);
    pixSetPixel(pixt, 2, 1, 180);
    pixSetPixel(pixt, 3, 1, 120);
    pixSetPixel(pixt, 4, 1, 80);
    pixSetPixel(pixt, 0, 0, 20);
    pixSetPixel(pixt, 1, 2, 50);
    pixSetPixel(pixt, 2, 2, 80);
    pixSetPixel(pixt, 3, 2, 50);
    pixSetPixel(pixt, 4, 2, 20);
    kel3 = kernelCreateFromPix(pixt, 1, 2);
    pixd = kernelDisplayInPix(kel3, 41, 2);
    pixSaveTiled(pixd, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/ker2.png", pixd, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/ker2.png", 4, &success);
    pixDestroy(&pixd);
    pixDestroy(&pixt);
    kernelDestroy(&kel3);

        /* Test convolution with kel1 */
    pixs = pixRead("test24.jpg");
    pixg = pixScaleRGBToGrayFast(pixs, 3, COLOR_GREEN);
    pixSaveTiled(pixg, pixa, 1, 1, 20, 0);
    kel1 = kernelCreateFromString(5, 5, 2, 2, kdatastr);
    pixd = pixConvolve(pixg, kel1, 8, 1);
    pixSaveTiled(pixd, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/ker3.png", pixd, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/ker3.png", 5, &success);
    pixDestroy(&pixs);
    pixDestroy(&pixg);
    pixDestroy(&pixd);
    kernelDestroy(&kel1);

        /* Test convolution with flat rectangular kel; also test
         * block convolution with tiling. */
    pixs = pixRead("test24.jpg");
    pixg = pixScaleRGBToGrayFast(pixs, 3, COLOR_GREEN);
    kel2 = makeFlatKernel(11, 11, 5, 5);
    pixd = pixConvolve(pixg, kel2, 8, 1);
    pixSaveTiled(pixd, pixa, 1, 1, 20, 0);
    pixWrite("/tmp/ker4.png", pixd, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/ker4.png", 6, &success);
    pixt = pixBlockconv(pixg, 5, 5);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/ker5.png", pixt, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/ker5.png", 7, &success);
    if (display)
        pixCompareGray(pixd, pixt, L_COMPARE_ABS_DIFF, GPLOT_X11, NULL,
                       NULL, NULL, NULL);
    pixt2 = pixBlockconvTiled(pixg, 5, 5, 3, 6);
    pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/ker5a.png", pixt2, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/ker5a.png", 8, &success);
    pixDestroy(&pixt2);

    ok = TRUE;
    for (i = 1; i <= 7; i++) {
        for (j = 1; j <= 7; j++) {
            if (i == 1 && j == 1) continue;
            pixt2 = pixBlockconvTiled(pixg, 5, 5, j, i);
            pixEqual(pixt2, pixd, &same);
            if (!same) {
                fprintf(stderr," Error for nx = %d, ny = %d\n", j, i);
                ok = FALSE;
            }
            pixDestroy(&pixt2);
        }
    }
    if (ok)
        fprintf(stderr, "OK: Tiled results identical to pixConvolve()\n");
    else
        fprintf(stderr, "ERROR: Tiled results not identical to pixConvolve()\n");
          
    pixDestroy(&pixs);
    pixDestroy(&pixg);
    pixDestroy(&pixd);
    pixDestroy(&pixt);
    kernelDestroy(&kel2);

        /* Do another flat rectangular test; this time with white at edge.
         * About 1% of the pixels near the image edge differ by 1 between
         * the pixConvolve() and pixBlockconv().  For what it's worth,
         * pixConvolve() gives the more accurate result; namely, 255 for
         * pixels at the edge. */
    pix = pixRead("pageseg1.tif");
    box = boxCreate(100, 100, 2260, 3160);
    pixb = pixClipRectangle(pix, box, NULL);
    pixs = pixScaleToGray4(pixb);

    kel3 = makeFlatKernel(7, 7, 3, 3);
    startTimer();
    pixt = pixConvolve(pixs, kel3, 8, 1);
    fprintf(stderr, "Generic convolution time: %5.3f sec\n", stopTimer());
    pixSaveTiled(pixt, pixa, 1, 1, 20, 0);
    pixWrite("/tmp/conv1.png", pixt, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/conv1.png", 9, &success);

    startTimer();
    pixt2 = pixBlockconv(pixs, 3, 3);
    fprintf(stderr, "Flat block convolution time: %5.3f sec\n", stopTimer());
    pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/conv2.png", pixt2, IFF_PNG);  /* ditto */
    regTestCheckFile(fp, argv, "/tmp/conv2.png", 10, &success);

    pixCompareGray(pixt, pixt2, L_COMPARE_ABS_DIFF, GPLOT_PNG, NULL,
                   &avediff, &rmsdiff, NULL);
#ifndef  _WIN32
    sleep(1);  /* give gnuplot time to write out the file */
#else
    Sleep(1000);
#endif  /* _WIN32 */
    pixp = pixRead("/tmp/grayroot.png");
    pixSaveTiled(pixp, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/conv3.png", pixp, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/conv3.png", 11, &success);
    fprintf(stderr, "Ave diff = %6.4f, RMS diff = %6.4f\n", avediff, rmsdiff);
    if (avediff <= 0.01)
        fprintf(stderr, "OK: avediff = %6.4f <= 0.01\n", avediff);
    else
        fprintf(stderr, "Bad?: avediff = %6.4f > 0.01\n", avediff);

    pixDestroy(&pixt);
    pixDestroy(&pixt2);
    pixDestroy(&pixs);
    pixDestroy(&pixp);
    pixDestroy(&pix);
    pixDestroy(&pixb);
    boxDestroy(&box);
    kernelDestroy(&kel3);

        /* Do yet another set of flat rectangular tests, this time
         * on an RGB image */
    pixs = pixRead("test24.jpg");
    kel4 = makeFlatKernel(7, 7, 3, 3);
    startTimer();
    pixt1 = pixConvolveRGB(pixs, kel4);
    fprintf(stderr, "Time 7x7 non-separable: %7.3f sec\n", stopTimer());
    pixWrite("/tmp/conv4.jpg", pixt1, IFF_JFIF_JPEG);
    regTestCheckFile(fp, argv, "/tmp/conv4.jpg", 12, &success);

    kelx = makeFlatKernel(1, 7, 0, 3);
    kely = makeFlatKernel(7, 1, 3, 0);
    startTimer();
    pixt2 = pixConvolveRGBSep(pixs, kelx, kely);
    fprintf(stderr, "Time 7x1,1x7 separable: %7.3f sec\n", stopTimer());
    pixWrite("/tmp/conv5.jpg", pixt2, IFF_JFIF_JPEG);
    regTestCheckFile(fp, argv, "/tmp/conv5.jpg", 13, &success);

    startTimer();
    pixt3 = pixBlockconv(pixs, 3, 3);
    fprintf(stderr, "Time 7x7 blockconv: %7.3f sec\n", stopTimer());
    pixWrite("/tmp/conv6.jpg", pixt3, IFF_JFIF_JPEG);
    regTestCheckFile(fp, argv, "/tmp/conv6.jpg", 14, &success);
    regTestComparePix(fp, argv, pixt1, pixt2, 0, &success);
    regTestCompareSimilarPix(fp, argv, pixt2, pixt3, 15, 0.0005, 1,
                             &success, 0);

    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    kernelDestroy(&kel4);
    kernelDestroy(&kelx);
    kernelDestroy(&kely);

        /* Test generation and convolution with gaussian kernel */
    pixs = pixRead("test8.jpg");
    pixSaveTiled(pixs, pixa, 1, 1, 20, 0);
    kel1 = makeGaussianKernel(5, 5, 3.0, 5.0);
    kernelGetSum(kel1, &sum);
    fprintf(stderr, "Sum for gaussian kernel = %f\n", sum);
    kernelWrite("/tmp/gauss.kel", kel1);
    pixt = pixConvolve(pixs, kel1, 8, 1);
    pixt2 = pixConvolve(pixs, kel1, 16, 0);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/ker6.png", pixt, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/ker6.png", 15, &success);
    pixDestroy(&pixt);
    pixDestroy(&pixt2);

    pixt = kernelDisplayInPix(kel1, 25, 2);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt);
    kernelDestroy(&kel1);
    pixDestroy(&pixs);

        /* Test generation and convolution with separable gaussian kernel */
    pixs = pixRead("test8.jpg");
    pixSaveTiled(pixs, pixa, 1, 1, 20, 0);
    makeGaussianKernelSep(5, 5, 3.0, 5.0, &kelx, &kely);
    kernelGetSum(kelx, &sum);
    fprintf(stderr, "Sum for x gaussian kernel = %f\n", sum);
    kernelGetSum(kely, &sum);
    fprintf(stderr, "Sum for y gaussian kernel = %f\n", sum);
    kernelWrite("/tmp/gauss.kelx", kelx);
    kernelWrite("/tmp/gauss.kely", kely);

    pixt = pixConvolveSep(pixs, kelx, kely, 8, 1);
    pixt2 = pixConvolveSep(pixs, kelx, kely, 16, 0);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixSaveTiled(pixt2, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/ker7.png", pixt, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/ker7.png", 16, &success);
    pixDestroy(&pixt);
    pixDestroy(&pixt2);

    pixt = kernelDisplayInPix(kelx, 25, 2);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt);
    pixt = kernelDisplayInPix(kely, 25, 2);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt);
    kernelDestroy(&kelx);
    kernelDestroy(&kely);
    pixDestroy(&pixs);

        /* Test generation and convolution with diff of gaussians kernel */
/*    pixt = pixRead("marge.jpg");
    pixs = pixConvertRGBToLuminance(pixt);
    pixDestroy(&pixt); */
    pixs = pixRead("test8.jpg");
    pixSaveTiled(pixs, pixa, 1, 1, 20, 0);
    kel1 = makeDoGKernel(7, 7, 1.5, 2.7);
    kernelGetSum(kel1, &sum);
    fprintf(stderr, "Sum for DoG kernel = %f\n", sum);
    kernelWrite("/tmp/dog.kel", kel1);
    pixt = pixConvolve(pixs, kel1, 8, 0);
/*    pixInvert(pixt, pixt); */
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixWrite("/tmp/ker8.png", pixt, IFF_PNG);
    regTestCheckFile(fp, argv, "/tmp/ker8.png", 17, &success);
    pixDestroy(&pixt);

    pixt = kernelDisplayInPix(kel1, 20, 2);
    pixSaveTiled(pixt, pixa, 1, 0, 20, 0);
    pixDestroy(&pixt);
    kernelDestroy(&kel1);
    pixDestroy(&pixs);

    pixd = pixaDisplay(pixa, 0, 0);
    pixDisplayWithTitle(pixd, 100, 100, NULL, display);
    pixWrite("/tmp/kernel.jpg", pixd, IFF_JFIF_JPEG);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);

    regTestCleanup(argc, argv, fp, success, NULL);
    return 0;
}

