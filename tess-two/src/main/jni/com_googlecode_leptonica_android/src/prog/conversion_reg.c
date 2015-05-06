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
 * conversion_reg.c
 *
 *    Regression test (_reg) for depth conversion functions,
 *    including some of the octcube quantization.
 */

#define  DFLAG    1

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
char        *errorstr;
l_int32      same, error;
PIX         *pixs1, *pixs2, *pixs4, *pixs8, *pixs16, *pixs32;
PIX         *pixc2, *pixc4, *pixc8;
PIX         *pixt1, *pixt2, *pixt3, *pixt4, *pixt5, *pixt6;
PIXCMAP     *cmap;
SARRAY      *sa;
static char  mainName[] = "convert_reg";

    if (argc != 1)
        return ERROR_INT(" Syntax:  convert_reg", mainName, 1);

    pixs1 = pixRead("test1.png");
    pixs2 = pixRead("dreyfus2.png");
    pixc2 = pixRead("weasel2.4c.png");
    pixs4 = pixRead("weasel4.16g.png");
    pixc4 = pixRead("weasel4.11c.png");
    pixs8 = pixRead("karen8.jpg");
    pixc8 = pixRead("weasel8.240c.png");
    pixs16 = pixRead("test16.tif");
    pixs32 = pixRead("marge.jpg");
    error = FALSE;
    sa = sarrayCreate(0);

        /* Conversion: 1 bpp --> 8 bpp --> 1 bpp */
    pixt1 = pixConvertTo8(pixs1, FALSE);
    pixt2 = pixThreshold8(pixt1, 1, 0, 0);
    pixEqual(pixs1, pixt2, &same);
    if (!same) {
        pixDisplayWithTitle(pixs1, 100, 100, "1 bpp, no cmap", DFLAG);
        pixDisplayWithTitle(pixt2, 500, 100, "1 bpp, no cmap", DFLAG);
        error = TRUE;
        sarrayAddString(sa, (char *)"conversion 1 bpp <==> 8 bpp", L_COPY);
    } else
        fprintf(stderr, "OK: conversion 1 bpp <==> 8 bpp\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

        /* Conversion: 2 bpp --> 8 bpp --> 2 bpp */
        /* Conversion: 2 bpp cmap --> 8 bpp cmap --> 2 bpp cmap */
    pixt1 = pixRemoveColormap(pixs2, REMOVE_CMAP_TO_GRAYSCALE);
    pixt2 = pixThreshold8(pixt1, 2, 4, 0);
    pixt3 = pixConvertTo8(pixt2, FALSE);
    pixt4 = pixThreshold8(pixt3, 2, 4, 0);
    pixEqual(pixt2, pixt4, &same);
    if (!same) {
        pixDisplayWithTitle(pixt2, 100, 100, "2 bpp, no cmap", DFLAG);
        pixDisplayWithTitle(pixt4, 500, 100, "2 bpp, no cmap", DFLAG);
        error = TRUE;
        sarrayAddString(sa, (char *)"conversion 2 bpp <==> 8 bpp", L_COPY);
    } else
        fprintf(stderr, "OK: conversion 2 bpp <==> 8 bpp\n");
    pixt5 = pixConvertTo8(pixs2, TRUE);
    pixt6 = pixThreshold8(pixt5, 2, 4, 1);
    pixEqual(pixs2, pixt6, &same);
    if (!same) {
        pixDisplayWithTitle(pixs2, 100, 100, "2 bpp, cmap", DFLAG);
        pixDisplayWithTitle(pixt6, 500, 100, "2 bpp, cmap", DFLAG);
        error = TRUE;
        sarrayAddString(sa, (char *)"conversion 2 bpp <==> 8 bpp; cmap",
                        L_COPY);
    } else
        fprintf(stderr, "OK: conversion 2 bpp <==> 8 bpp; cmap\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);
    pixDestroy(&pixt6);

        /* Conversion: 4 bpp --> 8 bpp --> 4 bpp */
        /* Conversion: 4 bpp cmap --> 8 bpp cmap --> 4 bpp cmap */
    pixt1 = pixRemoveColormap(pixs4, REMOVE_CMAP_TO_GRAYSCALE);
    pixt2 = pixThreshold8(pixt1, 4, 16, 0);
    pixt3 = pixConvertTo8(pixt2, FALSE);
    pixt4 = pixThreshold8(pixt3, 4, 16, 0);
    pixEqual(pixt2, pixt4, &same);
    if (!same) {
        pixDisplayWithTitle(pixt2, 100, 100, "4 bpp, no cmap", DFLAG);
        pixDisplayWithTitle(pixt4, 500, 100, "4 bpp, no cmap", DFLAG);
        error = TRUE;
        sarrayAddString(sa, (char *)"conversion 4 bpp <==> 8 bpp", L_COPY);
    } else
        fprintf(stderr, "OK: conversion 4 bpp <==> 8 bpp\n");
    pixt5 = pixConvertTo8(pixs4, TRUE);
    pixt6 = pixThreshold8(pixt5, 4, 16, 1);
    pixEqual(pixs4, pixt6, &same);
    if (!same) {
        pixDisplayWithTitle(pixs4, 100, 100, "4 bpp, cmap", DFLAG);
        pixDisplayWithTitle(pixt6, 500, 100, "4 bpp, cmap", DFLAG);
        error = TRUE;
        sarrayAddString(sa, (char *)"conversion 4 bpp <==> 8 bpp, cmap",
                        L_COPY);
    } else
        fprintf(stderr, "OK: conversion 4 bpp <==> 8 bpp; cmap\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixt5);
    pixDestroy(&pixt6);

        /* Conversion: 2 bpp cmap --> 2 bpp --> 2 bpp cmap --> 2 bpp */
    pixt1 = pixRemoveColormap(pixs2, REMOVE_CMAP_TO_GRAYSCALE);
    pixt2 = pixConvertGrayToColormap(pixt1);
    pixt3 = pixRemoveColormap(pixt2, REMOVE_CMAP_TO_GRAYSCALE);
    pixt4 = pixThresholdTo2bpp(pixt3, 4, 1);
    pixEqual(pixt1, pixt4, &same);
    if (!same) {
        pixDisplayWithTitle(pixs2, 100, 100, "2 bpp, cmap", DFLAG);
        pixDisplayWithTitle(pixt4, 500, 100, "2 bpp, cmap", DFLAG);
        error = TRUE;
        sarrayAddString(sa, (char *)"conversion 2 bpp <==> 2 bpp", L_COPY);
    } else
        fprintf(stderr, "OK: conversion 2 bpp <==> 2 bpp\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

        /* Conversion: 4 bpp cmap --> 4 bpp --> 4 bpp cmap --> 4 bpp */
    pixt1 = pixRemoveColormap(pixs4, REMOVE_CMAP_TO_GRAYSCALE);
    pixt2 = pixConvertGrayToColormap(pixt1);
    pixt3 = pixRemoveColormap(pixt2, REMOVE_CMAP_TO_GRAYSCALE);
    pixt4 = pixThresholdTo4bpp(pixt3, 16, 1);
    pixEqual(pixt1, pixt4, &same);
    if (!same) {
        pixDisplayWithTitle(pixs4, 100, 100, "4 bpp, cmap", DFLAG);
        pixDisplayWithTitle(pixt4, 500, 100, "4 bpp, cmap", DFLAG);
        error = TRUE;
        sarrayAddString(sa, (char *)"conversion 4 bpp <==> 4 bpp", L_COPY);
    } else
        fprintf(stderr, "OK: conversion 4 bpp <==> 4 bpp\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);


        /* Conversion: 8 bpp --> 8 bpp cmap --> 8 bpp */
    pixt1 = pixConvertTo8(pixs8, TRUE);
    pixt2 = pixConvertTo8(pixt1, FALSE);
    pixEqual(pixs8, pixt2, &same);
    if (!same) {
        pixDisplayWithTitle(pixt1, 100, 100, "8 bpp, cmap", DFLAG);
        pixDisplayWithTitle(pixt2, 500, 100, "8 bpp, no cmap", DFLAG);
        error = TRUE;
        sarrayAddString(sa, (char *)"conversion 8 bpp <==> 8 bpp", L_COPY);
    } else
        fprintf(stderr, "OK: conversion 8 bpp <==> 8 bpp\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

        /* Conversion: 2 bpp cmap --> 32 bpp --> 2 bpp cmap */
    pixt1 = pixConvertTo8(pixc2, TRUE);
    pixt2 = pixConvertTo32(pixt1);
    pixt3 = pixConvertTo32(pixc2);
    pixEqual(pixt2, pixt3, &same);
    if (!same) {
        pixDisplayWithTitle(pixt2, 100, 100, "32 bpp", DFLAG);
        pixDisplayWithTitle(pixt3, 500, 100, "32 bpp", DFLAG);
        error = TRUE;
        sarrayAddString(sa, (char *)"conversion 2 bpp ==> 32 bpp", L_COPY);
    } else
        fprintf(stderr, "OK: conversion 2 bpp <==> 32 bpp\n");
    cmap = pixGetColormap(pixc2);
    pixt4 = pixOctcubeQuantFromCmap(pixt3, cmap, 2, 4, L_EUCLIDEAN_DISTANCE);
    pixEqual(pixc2, pixt4, &same);
    if (!same) {
        pixDisplayWithTitle(pixc2, 100, 100, "4 bpp, cmap", DFLAG);
        pixDisplayWithTitle(pixt4, 500, 100, "4 bpp, cmap", DFLAG);
        error = TRUE;
        sarrayAddString(sa, (char *)"conversion 2 bpp <==> 32 bpp", L_COPY);
    } else
        fprintf(stderr, "OK: conversion 2 bpp <==> 32 bpp\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

        /* Conversion: 4 bpp cmap --> 32 bpp --> 4 bpp cmap */
    pixt1 = pixConvertTo8(pixc4, TRUE);
    pixt2 = pixConvertTo32(pixt1);
    pixt3 = pixConvertTo32(pixc4);
    pixEqual(pixt2, pixt3, &same);
    if (!same) {
        pixDisplayWithTitle(pixt2, 100, 100, "32 bpp", DFLAG);
        pixDisplayWithTitle(pixt3, 500, 100, "32 bpp", DFLAG);
        error = TRUE;
        sarrayAddString(sa, (char *)"conversion 4 bpp ==> 32 bpp", L_COPY);
    } else
        fprintf(stderr, "OK: conversion 4 bpp <==> 32 bpp\n");
    cmap = pixGetColormap(pixc4);
    pixt4 = pixOctcubeQuantFromCmap(pixt3, cmap, 2, 4, L_EUCLIDEAN_DISTANCE);
    pixEqual(pixc4, pixt4, &same);
    if (!same) {
        pixDisplayWithTitle(pixc4, 100, 100, "4 bpp, cmap", DFLAG);
        pixDisplayWithTitle(pixt4, 500, 100, "4 bpp, cmap", DFLAG);
        error = TRUE;
        sarrayAddString(sa, (char *)"conversion 4 bpp <==> 32 bpp", L_COPY);
    } else
        fprintf(stderr, "OK: conversion 4 bpp <==> 32 bpp\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

        /* Conversion: 8 bpp --> 32 bpp --> 8 bpp */
    pixt1 = pixConvertTo32(pixs8);
    pixt2 = pixConvertTo8(pixt1, FALSE);
    pixEqual(pixs8, pixt2, &same);
    if (!same) {
        pixDisplayWithTitle(pixs8, 100, 100, "8 bpp", DFLAG);
        pixDisplayWithTitle(pixt2, 500, 100, "8 bpp", DFLAG);
        error = TRUE;
        sarrayAddString(sa, (char *)"conversion 8 bpp <==> 32 bpp", L_COPY);
    } else
        fprintf(stderr, "OK: conversion 8 bpp <==> 32 bpp\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

        /* Conversion: 8 bpp --> 16 bpp --> 8 bpp */
    pixt1 = pixConvert8To16(pixs8, 8);
    pixt2 = pixConvertTo8(pixt1, FALSE);
    pixEqual(pixs8, pixt2, &same);
    if (!same) {
        pixDisplayWithTitle(pixs8, 100, 100, "8 bpp", DFLAG);
        pixDisplayWithTitle(pixt2, 500, 100, "8 bpp", DFLAG);
        error = TRUE;
        sarrayAddString(sa, (char *)"conversion 8 bpp <==> 16 bpp", L_COPY);
    } else
        fprintf(stderr, "OK: conversion 8 bpp <==> 16 bpp\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

        /* Conversion: 16 bpp --> 8 bpp --> 16 bpp */
    pixt1 = pixConvert16To8(pixs16, 1);
    pixt2 = pixConvertTo16(pixt1);
    pixWrite("/tmp/junkpix.png", pixt2, IFF_PNG);
    pixEqual(pixs16, pixt2, &same);
    if (!same) {
        pixDisplayWithTitle(pixs16, 100, 100, "16 bpp", DFLAG);
        pixDisplayWithTitle(pixt2, 500, 100, "16 bpp", DFLAG);
        error = TRUE;
        sarrayAddString(sa, (char *)"conversion 16 bpp <==> 8 bpp", L_COPY);
    } else
        fprintf(stderr, "OK: conversion 16 bpp <==> 8 bpp\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

        /* Conversion: 8 bpp cmap --> 32 bpp --> 8 bpp cmap */
        /* Required to go to level 6 of octcube to get identical result */
    pixt1 = pixConvertTo32(pixc8);
    cmap = pixGetColormap(pixc8);
    pixt2 = pixOctcubeQuantFromCmap(pixt1, cmap, 2, 6, L_EUCLIDEAN_DISTANCE);
    pixEqual(pixc8, pixt2, &same);
    if (!same) {
        pixDisplayWithTitle(pixc8, 100, 100, "8 bpp cmap", DFLAG);
        pixDisplayWithTitle(pixt2, 500, 100, "8 bpp cmap", DFLAG);
        error = TRUE;
        sarrayAddString(sa, (char *)"conversion 8 bpp cmap <==> 32 bpp cmap",
                        L_COPY);
    } else
        fprintf(stderr, "OK: conversion 8 bpp <==> 32 bpp\n");
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

        /* Summarize results */
    if (error == FALSE)
        fprintf(stderr, "No errors found\n");
    else {
        errorstr = sarrayToString(sa, 1);
        fprintf(stderr, "Errors in the following:\n %s", errorstr);
        lept_free(errorstr);
    }

    sarrayDestroy(&sa);
    pixDestroy(&pixs1);
    pixDestroy(&pixs2);
    pixDestroy(&pixs4);
    pixDestroy(&pixc2);
    pixDestroy(&pixc4);
    pixDestroy(&pixs8);
    pixDestroy(&pixc8);
    pixDestroy(&pixs16);
    pixDestroy(&pixs32);
    return 0;
}

