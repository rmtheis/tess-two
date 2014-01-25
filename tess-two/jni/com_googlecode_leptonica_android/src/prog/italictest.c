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
 *  italictest.c
 *
 *     Demonstrates binary reconstruction for finding italic text.
 */

#include "allheaders.h"


int main(int    argc,
         char **argv)
{
char         opstring[32];
l_int32      size;
BOXA        *boxa1, *boxa2, *boxa3, *boxawm;
PIX         *pixs, *pixm, *pixt;
static char  mainName[] = "italictest";

    if (argc != 1 && argc != 2)
        return ERROR_INT(" Syntax:  italictest [filein]", mainName, 1);

    if (argc == 1)
        pixs = pixRead("italic.png");
    else
        pixs = pixRead(argv[1]);
    if (!pixs)
        return ERROR_INT("pixs not read", mainName, 1);

        /* Basic functionality with debug flag */
    pixItalicWords(pixs, NULL, NULL, &boxa1, 1);
    boxaWrite("/tmp/ital1.ba", boxa1);
    pixt = pixRead("/tmp/ital.png");
    pixDisplayWithTitle(pixt, 0, 0, "Intermediate steps", 1);
    pixDestroy(&pixt);
    pixt = pixRead("/tmp/runhisto.png");
    pixDisplayWithTitle(pixt, 800, 0, "Histogram of white runs", 1);
    pixDestroy(&pixt);

        /* Generate word mask */
    pixWordMaskByDilation(pixs, 20, NULL, &size);
    L_INFO("dilation size = %d\n", mainName, size);
    snprintf(opstring, sizeof(opstring), "d1.5 + c%d.1", size);
    pixm = pixMorphSequence(pixs, opstring, 0);
    pixWrite("/tmp/ital-wm.png", pixm, IFF_PNG);
    pixDisplayWithTitle(pixm, 800, 200, "Word mask", 1);

        /* Re-run italic finder using the word mask */
    pixItalicWords(pixs, NULL, pixm, &boxa2, 1);
    boxaWrite("/tmp/ital2.ba", boxa2);

        /* Re-run italic finder using word mask bounding boxes */
    boxawm = pixConnComp(pixm, NULL, 8);
    pixItalicWords(pixs, boxawm, NULL, &boxa3, 1);
    boxaWrite("/tmp/ital-wm.ba", boxawm);
    boxaWrite("/tmp/ital3.ba", boxa3);

    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
    boxaDestroy(&boxa3);
    boxaDestroy(&boxawm);
    pixDestroy(&pixs);
    pixDestroy(&pixm);
    return 0;
}

