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
 * logicops_reg.c
 *
 */

#include "allheaders.h"

#define   DISPLAY    0

int main(int    argc,
         char **argv)
{
l_int32      same;
PIX         *pixs, *pixt1, *pixt2, *pixt3, *pixt4;
static char  mainName[] = "logicops_reg";

    if (argc != 1)
        return ERROR_INT(" Syntax: logicops_reg", mainName, 1);

    pixs = pixRead("test1.png");
    pixDisplayWrite(pixs, DISPLAY);

        /* pixInvert */
    pixt1 = pixInvert(NULL, pixs);
    pixt2 = pixCreateTemplate(pixs);  /* into pixd of same size */
    pixInvert(pixt2, pixs);
    pixEqual(pixt1, pixt2, &same);
    if (!same)
        fprintf(stderr, "Error: pixInvert\n");
    else
        fprintf(stderr, "Correct: pixInvert\n");
    pixDisplayWrite(pixt1, DISPLAY);

    pixt3 = pixRead("marge.jpg");  /* into pixd of different size */
    pixInvert(pixt3, pixs);
    pixEqual(pixt1, pixt3, &same);
    if (!same)
        fprintf(stderr, "Error: pixInvert\n");
    else
        fprintf(stderr, "Correct: pixInvert\n");
    pixDisplayWrite(pixt3, DISPLAY);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);

    pixt1 = pixOpenBrick(NULL, pixs, 1, 9);
    pixt2 = pixDilateBrick(NULL, pixs, 1, 9);
    pixDisplayWrite(pixt1, DISPLAY);
    pixDisplayWrite(pixt2, DISPLAY);

        /* pixOr */
    pixt3 = pixCreateTemplate(pixs);
    pixOr(pixt3, pixs, pixt1);  /* existing */
    pixt4 = pixOr(NULL, pixs, pixt1);  /* new */
    pixEqual(pixt3, pixt4, &same);
    if (!same)
        fprintf(stderr, "Error: pixOr\n");
    else
        fprintf(stderr, "Correct: pixOr\n");
    pixDisplayWrite(pixt3, DISPLAY);
    pixCopy(pixt4, pixt1);
    pixOr(pixt4, pixt4, pixs);  /* in-place */
    pixEqual(pixt3, pixt4, &same);
    if (!same)
        fprintf(stderr, "Error: pixOr\n");
    else
        fprintf(stderr, "Correct: pixOr\n");
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

    pixt3 = pixCreateTemplate(pixs);
    pixOr(pixt3, pixs, pixt2);  /* existing */
    pixt4 = pixOr(NULL, pixs, pixt2);  /* new */
    pixEqual(pixt3, pixt4, &same);
    if (!same)
        fprintf(stderr, "Error: pixOr\n");
    else
        fprintf(stderr, "Correct: pixOr\n");
    pixDisplayWrite(pixt3, DISPLAY);
    pixCopy(pixt4, pixt2);
    pixOr(pixt4, pixt4, pixs);  /* in-place */
    pixEqual(pixt3, pixt4, &same);
    if (!same)
        fprintf(stderr, "Error: pixOr\n");
    else
        fprintf(stderr, "Correct: pixOr\n");
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

        /* pixAnd */
    pixt3 = pixCreateTemplate(pixs);
    pixAnd(pixt3, pixs, pixt1);  /* existing */
    pixt4 = pixAnd(NULL, pixs, pixt1);  /* new */
    pixEqual(pixt3, pixt4, &same);
    if (!same)
        fprintf(stderr, "Error: pixAnd\n");
    else
        fprintf(stderr, "Correct: pixAnd\n");
    pixDisplayWrite(pixt3, DISPLAY);
    pixCopy(pixt3, pixt1);
    pixAnd(pixt3, pixt3, pixs);  /* in-place */
    pixEqual(pixt3, pixt4, &same);
    if (!same)
        fprintf(stderr, "Error: pixAnd\n");
    else
        fprintf(stderr, "Correct: pixAnd\n");
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

    pixt3 = pixCreateTemplate(pixs);
    pixAnd(pixt3, pixs, pixt2);  /* existing */
    pixt4 = pixAnd(NULL, pixs, pixt2);  /* new */
    pixEqual(pixt3, pixt4, &same);
    if (!same)
        fprintf(stderr, "Error: pixAnd\n");
    else
        fprintf(stderr, "Correct: pixAnd\n");
    pixDisplayWrite(pixt3, DISPLAY);
    pixCopy(pixt3, pixt2);
    pixAnd(pixt3, pixt3, pixs);  /* in-place */
    pixEqual(pixt3, pixt4, &same);
    if (!same)
        fprintf(stderr, "Error: pixAnd\n");
    else
        fprintf(stderr, "Correct: pixAnd\n");
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

        /* pixXor */
    pixt3 = pixCreateTemplate(pixs);
    pixXor(pixt3, pixs, pixt1);  /* existing */
    pixt4 = pixXor(NULL, pixs, pixt1);  /* new */
    pixEqual(pixt3, pixt4, &same);
    if (!same)
        fprintf(stderr, "Error: pixXor\n");
    else
        fprintf(stderr, "Correct: pixXor\n");
    pixDisplayWrite(pixt3, DISPLAY);
    pixCopy(pixt3, pixt1);
    pixXor(pixt3, pixt3, pixs);  /* in-place */
    pixEqual(pixt3, pixt4, &same);
    if (!same)
        fprintf(stderr, "Error: pixXor\n");
    else
        fprintf(stderr, "Correct: pixXor\n");
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

    pixt3 = pixCreateTemplate(pixs);
    pixXor(pixt3, pixs, pixt2);  /* existing */
    pixt4 = pixXor(NULL, pixs, pixt2);  /* new */
    pixEqual(pixt3, pixt4, &same);
    if (!same)
        fprintf(stderr, "Error: pixXor\n");
    else
        fprintf(stderr, "Correct: pixXor\n");
    pixDisplayWrite(pixt3, DISPLAY);
    pixCopy(pixt3, pixt2);
    pixXor(pixt3, pixt3, pixs);  /* in-place */
    pixEqual(pixt3, pixt4, &same);
    if (!same)
        fprintf(stderr, "Error: pixXor\n");
    else
        fprintf(stderr, "Correct: pixXor\n");
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

        /* pixSubtract */
    pixt3 = pixCreateTemplate(pixs);
    pixSubtract(pixt3, pixs, pixt1);  /* existing */
    pixt4 = pixSubtract(NULL, pixs, pixt1);  /* new */
    pixEqual(pixt3, pixt4, &same);
    if (!same)
        fprintf(stderr, "Error: pixSubtract\n");
    else
        fprintf(stderr, "Correct: pixSubtract\n");
    pixDisplayWrite(pixt3, DISPLAY);
    pixCopy(pixt4, pixt1);
    pixSubtract(pixt4, pixs, pixt4);  /* in-place */
    pixEqual(pixt3, pixt4, &same);
    if (!same)
        fprintf(stderr, "Error: pixSubtract\n");
    else
        fprintf(stderr, "Correct: pixSubtract\n");
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

    pixt3 = pixCreateTemplate(pixs);
    pixSubtract(pixt3, pixs, pixt2);  /* existing */
    pixt4 = pixSubtract(NULL, pixs, pixt2);  /* new */
    pixEqual(pixt3, pixt4, &same);
    if (!same)
        fprintf(stderr, "Error: pixSubtract\n");
    else
        fprintf(stderr, "Correct: pixSubtract\n");
    pixDisplayWrite(pixt3, DISPLAY);
    pixCopy(pixt4, pixt2);
    pixSubtract(pixt4, pixs, pixt4);  /* in-place */
    pixEqual(pixt3, pixt4, &same);
    if (!same)
        fprintf(stderr, "Error: pixSubtract\n");
    else
        fprintf(stderr, "Correct: pixSubtract\n");
    pixCopy(pixt4, pixt2);
    pixSubtract(pixs, pixs, pixt4);  /* in-place */
    pixEqual(pixt3, pixs, &same);
    if (!same)
        fprintf(stderr, "Error: pixSubtract\n");
    else
        fprintf(stderr, "Correct: pixSubtract\n");
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

    pixt4 = pixRead("marge.jpg");
    pixSubtract(pixt4, pixs, pixs);  /* subtract from itself; should be empty */
    pixt3 = pixCreateTemplate(pixs);
    pixEqual(pixt3, pixt4, &same);
    if (!same)
        fprintf(stderr, "Error: pixSubtract\n");
    else
        fprintf(stderr, "Correct: pixSubtract\n");
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

    pixSubtract(pixs, pixs, pixs);  /* subtract from itself; should be empty */
    pixt3 = pixCreateTemplate(pixs);
    pixEqual(pixt3, pixs, &same);
    if (!same)
        fprintf(stderr, "Error: pixSubtract\n");
    else
        fprintf(stderr, "Correct: pixSubtract\n");
    pixDestroy(&pixt3);

    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    return 0;
}

