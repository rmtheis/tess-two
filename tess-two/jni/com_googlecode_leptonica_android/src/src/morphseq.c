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
 *  morphseq.c
 *
 *      Run a sequence of binary rasterop morphological operations
 *            PIX     *pixMorphSequence()
 *
 *      Run a sequence of binary composite rasterop morphological operations
 *            PIX     *pixMorphCompSequence()
 *
 *      Run a sequence of binary dwa morphological operations
 *            PIX     *pixMorphSequenceDwa()
 *
 *      Run a sequence of binary composite dwa morphological operations
 *            PIX     *pixMorphCompSequenceDwa()
 *
 *      Parser verifier for binary morphological operations
 *            l_int32  morphSequenceVerify()
 *
 *      Run a sequence of grayscale morphological operations
 *            PIX     *pixGrayMorphSequence()
 *
 *      Run a sequence of color morphological operations
 *            PIX     *pixColorMorphSequence()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"


/*-------------------------------------------------------------------------*
 *         Run a sequence of binary rasterop morphological operations      *
 *-------------------------------------------------------------------------*/
/*!
 *  pixMorphSequence()
 *
 *      Input:  pixs
 *              sequence (string specifying sequence)
 *              dispsep (horizontal separation in pixels between
 *                       successive displays; use zero to suppress display)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This does rasterop morphology on binary images.
 *      (2) This runs a pipeline of operations; no branching is allowed.
 *      (3) This only uses brick Sels, which are created on the fly.
 *          In the future this will be generalized to extract Sels from
 *          a Sela by name.
 *      (4) A new image is always produced; the input image is not changed.
 *      (5) This contains an interpreter, allowing sequences to be
 *          generated and run.
 *      (6) The format of the sequence string is defined below.
 *      (7) In addition to morphological operations, rank order reduction
 *          and replicated expansion allow operations to take place
 *          downscaled by a power of 2.
 *      (8) Intermediate results can optionally be displayed.
 *      (9) Thanks to Dar-Shyang Lee, who had the idea for this and
 *          built the first implementation.
 *      (10) The sequence string is formatted as follows:
 *            - An arbitrary number of operations,  each separated
 *              by a '+' character.  White space is ignored.
 *            - Each operation begins with a case-independent character
 *              specifying the operation:
 *                 d or D  (dilation)
 *                 e or E  (erosion)
 *                 o or O  (opening)
 *                 c or C  (closing)
 *                 r or R  (rank binary reduction)
 *                 x or X  (replicative binary expansion)
 *                 b or B  (add a border of 0 pixels of this size)
 *            - The args to the morphological operations are bricks of hits,
 *              and are formatted as a.b, where a and b are horizontal and
 *              vertical dimensions, rsp.
 *            - The args to the reduction are a sequence of up to 4 integers,
 *              each from 1 to 4.
 *            - The arg to the expansion is a power of two, in the set
 *              {2, 4, 8, 16}.
 *      (11) An example valid sequence is:
 *               "b32 + o1.3 + C3.1 + r23 + e2.2 + D3.2 + X4"
 *           In this example, the following operation sequence is carried out:
 *             * b32: Add a 32 pixel border around the input image
 *             * o1.3: Opening with vert sel of length 3 (e.g., 1 x 3)
 *             * C3.1: Closing with horiz sel of length 3  (e.g., 3 x 1)
 *             * r23: Two successive 2x2 reductions with rank 2 in the first
 *                    and rank 3 in the second.  The result is a 4x reduced pix.
 *             * e2.2: Erosion with a 2x2 sel (origin will be at x,y: 0,0)
 *             * d3.2: Dilation with a 3x2 sel (origin will be at x,y: 1,0)
 *             * X4: 4x replicative expansion, back to original resolution
 *      (12) The safe closing is used.  However, if you implement a
 *           closing as separable dilations followed by separable erosions,
 *           it will not be safe.  For that situation, you need to add
 *           a sufficiently large border as the first operation in
 *           the sequence.  This will be removed automatically at the
 *           end.  There are two cautions: 
 *              - When computing what is sufficient, remember that if
 *                reductions are carried out, the border is also reduced.
 *              - The border is removed at the end, so if a border is
 *                added at the beginning, the result must be at the
 *                same resolution as the input!
 */
PIX *
pixMorphSequence(PIX         *pixs,
                 const char  *sequence,
                 l_int32      dispsep)
{
char    *rawop, *op;
l_int32  nops, i, j, nred, fact, w, h, x, y, border;
l_int32  level[4];
PIX     *pixt1, *pixt2;
SARRAY  *sa;

    PROCNAME("pixMorphSequence");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!sequence)
        return (PIX *)ERROR_PTR("sequence not defined", procName, NULL);

        /* Split sequence into individual operations */
    sa = sarrayCreate(0);
    sarraySplitString(sa, sequence, "+");
    nops = sarrayGetCount(sa);

    if (!morphSequenceVerify(sa)) {
        sarrayDestroy(&sa);
        return (PIX *)ERROR_PTR("sequence not valid", procName, NULL);
    }

        /* Parse and operate */
    border = 0;
    pixt1 = pixCopy(NULL, pixs);
    pixt2 = NULL;
    x = y = 0;
    for (i = 0; i < nops; i++) {
        rawop = sarrayGetString(sa, i, 0);
        op = stringRemoveChars(rawop, " \n\t");
        switch (op[0])
        {
        case 'd':
        case 'D':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixt2 = pixDilateBrick(NULL, pixt1, w, h);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'e':
        case 'E':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixt2 = pixErodeBrick(NULL, pixt1, w, h);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'o':
        case 'O':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixOpenBrick(pixt1, pixt1, w, h);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'c':
        case 'C':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixCloseSafeBrick(pixt1, pixt1, w, h);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'r':
        case 'R':
            nred = strlen(op) - 1;
            for (j = 0; j < nred; j++)
                level[j] = op[j + 1] - '0';
            for (j = nred; j < 4; j++)
                level[j] = 0;
            pixt2 = pixReduceRankBinaryCascade(pixt1, level[0], level[1],
                                               level[2], level[3]);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'x':
        case 'X':
            sscanf(&op[1], "%d", &fact);
            pixt2 = pixExpandReplicate(pixt1, fact);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'b':
        case 'B':
            sscanf(&op[1], "%d", &border);
            pixt2 = pixAddBorder(pixt1, border, 0);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        default:
            /* All invalid ops are caught in the first pass */
            break;
        }
        FREE(op);
    }
    if (border > 0) {
        pixt2 = pixRemoveBorder(pixt1, border);
        pixDestroy(&pixt1);
        pixt1 = pixClone(pixt2);
        pixDestroy(&pixt2);
    }

    sarrayDestroy(&sa);
    return pixt1;
}


/*-------------------------------------------------------------------------*
 *   Run a sequence of binary composite rasterop morphological operations  *
 *-------------------------------------------------------------------------*/
/*!
 *  pixMorphCompSequence()
 *
 *      Input:  pixs
 *              sequence (string specifying sequence)
 *              dispsep (horizontal separation in pixels between
 *                       successive displays; use zero to suppress display)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This does rasterop morphology on binary images, using composite
 *          operations for extra speed on large Sels.
 *      (2) Safe closing is used atomically.  However, if you implement a
 *          closing as a sequence with a dilation followed by an
 *          erosion, it will not be safe, and to ensure that you have
 *          no boundary effects you must add a border in advance and
 *          remove it at the end.
 *      (3) For other usage details, see the notes for pixMorphSequence().
 *      (4) The sequence string is formatted as follows:
 *            - An arbitrary number of operations,  each separated
 *              by a '+' character.  White space is ignored.
 *            - Each operation begins with a case-independent character
 *              specifying the operation:
 *                 d or D  (dilation)
 *                 e or E  (erosion)
 *                 o or O  (opening)
 *                 c or C  (closing)
 *                 r or R  (rank binary reduction)
 *                 x or X  (replicative binary expansion)
 *                 b or B  (add a border of 0 pixels of this size)
 *            - The args to the morphological operations are bricks of hits,
 *              and are formatted as a.b, where a and b are horizontal and
 *              vertical dimensions, rsp.
 *            - The args to the reduction are a sequence of up to 4 integers,
 *              each from 1 to 4.
 *            - The arg to the expansion is a power of two, in the set
 *              {2, 4, 8, 16}.
 */
PIX *
pixMorphCompSequence(PIX         *pixs,
                     const char  *sequence,
                     l_int32      dispsep)
{
char    *rawop, *op;
l_int32  nops, i, j, nred, fact, w, h, x, y, border;
l_int32  level[4];
PIX     *pixt1, *pixt2;
SARRAY  *sa;

    PROCNAME("pixMorphCompSequence");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!sequence)
        return (PIX *)ERROR_PTR("sequence not defined", procName, NULL);

        /* Split sequence into individual operations */
    sa = sarrayCreate(0);
    sarraySplitString(sa, sequence, "+");
    nops = sarrayGetCount(sa);

    if (!morphSequenceVerify(sa)) {
        sarrayDestroy(&sa);
        return (PIX *)ERROR_PTR("sequence not valid", procName, NULL);
    }

        /* Parse and operate */
    border = 0;
    pixt1 = pixCopy(NULL, pixs);
    pixt2 = NULL;
    x = y = 0;
    for (i = 0; i < nops; i++) {
        rawop = sarrayGetString(sa, i, 0);
        op = stringRemoveChars(rawop, " \n\t");
        switch (op[0])
        {
        case 'd':
        case 'D':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixt2 = pixDilateCompBrick(NULL, pixt1, w, h);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'e':
        case 'E':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixt2 = pixErodeCompBrick(NULL, pixt1, w, h);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'o':
        case 'O':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixOpenCompBrick(pixt1, pixt1, w, h);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'c':
        case 'C':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixCloseSafeCompBrick(pixt1, pixt1, w, h);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'r':
        case 'R':
            nred = strlen(op) - 1;
            for (j = 0; j < nred; j++)
                level[j] = op[j + 1] - '0';
            for (j = nred; j < 4; j++)
                level[j] = 0;
            pixt2 = pixReduceRankBinaryCascade(pixt1, level[0], level[1],
                                               level[2], level[3]);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'x':
        case 'X':
            sscanf(&op[1], "%d", &fact);
            pixt2 = pixExpandReplicate(pixt1, fact);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'b':
        case 'B':
            sscanf(&op[1], "%d", &border);
            pixt2 = pixAddBorder(pixt1, border, 0);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        default:
            /* All invalid ops are caught in the first pass */
            break;
        }
        FREE(op);
    }
    if (border > 0) {
        pixt2 = pixRemoveBorder(pixt1, border);
        pixDestroy(&pixt1);
        pixt1 = pixClone(pixt2);
        pixDestroy(&pixt2);
    }

    sarrayDestroy(&sa);
    return pixt1;
}


/*-------------------------------------------------------------------------*
 *           Run a sequence of binary dwa morphological operations         *
 *-------------------------------------------------------------------------*/
/*!
 *  pixMorphSequenceDwa()
 *
 *      Input:  pixs
 *              sequence (string specifying sequence)
 *              dispsep (horizontal separation in pixels between
 *                       successive displays; use zero to suppress display)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This does dwa morphology on binary images.
 *      (2) This runs a pipeline of operations; no branching is allowed.
 *      (3) This only uses brick Sels that have been pre-compiled with
 *          dwa code.
 *      (4) A new image is always produced; the input image is not changed.
 *      (5) This contains an interpreter, allowing sequences to be
 *          generated and run.
 *      (6) See pixMorphSequence() for further information about usage.
 */
PIX *
pixMorphSequenceDwa(PIX         *pixs,
                    const char  *sequence,
                    l_int32      dispsep)
{
char    *rawop, *op;
l_int32  nops, i, j, nred, fact, w, h, x, y, border;
l_int32  level[4];
PIX     *pixt1, *pixt2;
SARRAY  *sa;

    PROCNAME("pixMorphSequenceDwa");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!sequence)
        return (PIX *)ERROR_PTR("sequence not defined", procName, NULL);

        /* Split sequence into individual operations */
    sa = sarrayCreate(0);
    sarraySplitString(sa, sequence, "+");
    nops = sarrayGetCount(sa);

    if (!morphSequenceVerify(sa)) {
        sarrayDestroy(&sa);
        return (PIX *)ERROR_PTR("sequence not valid", procName, NULL);
    }

        /* Parse and operate */
    border = 0;
    pixt1 = pixCopy(NULL, pixs);
    pixt2 = NULL;
    x = y = 0;
    for (i = 0; i < nops; i++) {
        rawop = sarrayGetString(sa, i, 0);
        op = stringRemoveChars(rawop, " \n\t");
        switch (op[0])
        {
        case 'd':
        case 'D':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixt2 = pixDilateBrickDwa(NULL, pixt1, w, h);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'e':
        case 'E':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixt2 = pixErodeBrickDwa(NULL, pixt1, w, h);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'o':
        case 'O':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixOpenBrickDwa(pixt1, pixt1, w, h);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'c':
        case 'C':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixCloseBrickDwa(pixt1, pixt1, w, h);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'r':
        case 'R':
            nred = strlen(op) - 1;
            for (j = 0; j < nred; j++)
                level[j] = op[j + 1] - '0';
            for (j = nred; j < 4; j++)
                level[j] = 0;
            pixt2 = pixReduceRankBinaryCascade(pixt1, level[0], level[1],
                                               level[2], level[3]);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'x':
        case 'X':
            sscanf(&op[1], "%d", &fact);
            pixt2 = pixExpandReplicate(pixt1, fact);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'b':
        case 'B':
            sscanf(&op[1], "%d", &border);
            pixt2 = pixAddBorder(pixt1, border, 0);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        default:
            /* All invalid ops are caught in the first pass */
            break;
        }
        FREE(op);
    }
    if (border > 0) {
        pixt2 = pixRemoveBorder(pixt1, border);
        pixDestroy(&pixt1);
        pixt1 = pixClone(pixt2);
        pixDestroy(&pixt2);
    }

    sarrayDestroy(&sa);
    return pixt1;
}


/*-------------------------------------------------------------------------*
 *      Run a sequence of binary composite dwa morphological operations    *
 *-------------------------------------------------------------------------*/
/*!
 *  pixMorphCompSequenceDwa()
 *
 *      Input:  pixs
 *              sequence (string specifying sequence)
 *              dispsep (horizontal separation in pixels between
 *                       successive displays; use zero to suppress display)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This does dwa morphology on binary images, using brick Sels.
 *      (2) This runs a pipeline of operations; no branching is allowed.
 *      (3) It implements all brick Sels that have dimensions up to 63
 *          on each side, using a composite (linear + comb) when useful.
 *      (4) A new image is always produced; the input image is not changed.
 *      (5) This contains an interpreter, allowing sequences to be
 *          generated and run.
 *      (6) See pixMorphSequence() for further information about usage.
 */
PIX *
pixMorphCompSequenceDwa(PIX         *pixs,
                        const char  *sequence,
                        l_int32      dispsep)
{
char    *rawop, *op;
l_int32  nops, i, j, nred, fact, w, h, x, y, border;
l_int32  level[4];
PIX     *pixt1, *pixt2;
SARRAY  *sa;

    PROCNAME("pixMorphCompSequenceDwa");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!sequence)
        return (PIX *)ERROR_PTR("sequence not defined", procName, NULL);

        /* Split sequence into individual operations */
    sa = sarrayCreate(0);
    sarraySplitString(sa, sequence, "+");
    nops = sarrayGetCount(sa);

    if (!morphSequenceVerify(sa)) {
        sarrayDestroy(&sa);
        return (PIX *)ERROR_PTR("sequence not valid", procName, NULL);
    }

        /* Parse and operate */
    border = 0;
    pixt1 = pixCopy(NULL, pixs);
    pixt2 = NULL;
    x = y = 0;
    for (i = 0; i < nops; i++) {
        rawop = sarrayGetString(sa, i, 0);
        op = stringRemoveChars(rawop, " \n\t");
        switch (op[0])
        {
        case 'd':
        case 'D':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixt2 = pixDilateCompBrickDwa(NULL, pixt1, w, h);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'e':
        case 'E':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixt2 = pixErodeCompBrickDwa(NULL, pixt1, w, h);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'o':
        case 'O':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixOpenCompBrickDwa(pixt1, pixt1, w, h);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'c':
        case 'C':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixCloseCompBrickDwa(pixt1, pixt1, w, h);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'r':
        case 'R':
            nred = strlen(op) - 1;
            for (j = 0; j < nred; j++)
                level[j] = op[j + 1] - '0';
            for (j = nred; j < 4; j++)
                level[j] = 0;
            pixt2 = pixReduceRankBinaryCascade(pixt1, level[0], level[1],
                                               level[2], level[3]);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'x':
        case 'X':
            sscanf(&op[1], "%d", &fact);
            pixt2 = pixExpandReplicate(pixt1, fact);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        case 'b':
        case 'B':
            sscanf(&op[1], "%d", &border);
            pixt2 = pixAddBorder(pixt1, border, 0);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, y);
                x += dispsep;
            }
            break;
        default:
            /* All invalid ops are caught in the first pass */
            break;
        }
        FREE(op);
    }
    if (border > 0) {
        pixt2 = pixRemoveBorder(pixt1, border);
        pixDestroy(&pixt1);
        pixt1 = pixClone(pixt2);
        pixDestroy(&pixt2);
    }

    sarrayDestroy(&sa);
    return pixt1;
}


/*-------------------------------------------------------------------------*
 *            Parser verifier for binary morphological operations          *
 *-------------------------------------------------------------------------*/
/*!
 *  morphSequenceVerify()
 *
 *      Input:  sarray (of operation sequence)
 *      Return: TRUE if valid; FALSE otherwise or on error
 *
 *  Notes:
 *      (1) This does verification of valid binary morphological
 *          operation sequences.
 *      (2) See pixMorphSequence() for notes on valid operations
 *          in the sequence.
 */
l_int32
morphSequenceVerify(SARRAY  *sa)
{
char    *rawop, *op;
l_int32  nops, i, j, nred, fact, valid, w, h, netred, border;
l_int32  level[4];
l_int32  intlogbase2[5] = {1, 2, 3, 0, 4};  /* of arg/4 */

    PROCNAME("morphSequenceVerify");

    if (!sa)
        return ERROR_INT("sa not defined", procName, FALSE);

    nops = sarrayGetCount(sa);
    valid = TRUE;
    netred = 0;
    border = 0;
    for (i = 0; i < nops; i++) {
        rawop = sarrayGetString(sa, i, 0);
        op = stringRemoveChars(rawop, " \n\t");
        switch (op[0])
        {
        case 'd':
        case 'D':
        case 'e':
        case 'E':
        case 'o':
        case 'O':
        case 'c':
        case 'C':
            if (sscanf(&op[1], "%d.%d", &w, &h) != 2) {
                fprintf(stderr, "*** op: %s invalid\n", op);
                valid = FALSE;
                break;
            }
            if (w <= 0 || h <= 0) {
                fprintf(stderr,
                        "*** op: %s; w = %d, h = %d; must both be > 0\n",
                        op, w, h);
                valid = FALSE;
                break;
            }
/*            fprintf(stderr, "op = %s; w = %d, h = %d\n", op, w, h); */
            break;
        case 'r':
        case 'R':
            nred = strlen(op) - 1;
            netred += nred;
            if (nred < 1 || nred > 4) {
                fprintf(stderr,
                        "*** op = %s; num reduct = %d; must be in {1,2,3,4}\n",
                        op, nred);
                valid = FALSE;
                break;
            }
            for (j = 0; j < nred; j++) {
                level[j] = op[j + 1] - '0';
                if (level[j] < 1 || level[j] > 4) {
                    fprintf(stderr, "*** op = %s; level[%d] = %d is invalid\n",
                            op, j, level[j]);
                    valid = FALSE;
                    break;
                }
            }
            if (!valid)
                break;
/*            fprintf(stderr, "op = %s", op); */
            for (j = 0; j < nred; j++) {
                level[j] = op[j + 1] - '0';
/*                fprintf(stderr, ", level[%d] = %d", j, level[j]); */
            }
/*            fprintf(stderr, "\n"); */
            break;
        case 'x':
        case 'X':
            if (sscanf(&op[1], "%d", &fact) != 1) {
                fprintf(stderr, "*** op: %s; fact invalid\n", op);
                valid = FALSE;
                break;
            }
            if (fact != 2 && fact != 4 && fact != 8 && fact != 16) {
                fprintf(stderr, "*** op = %s; invalid fact = %d\n", op, fact);
                valid = FALSE;
                break;
            }
            netred -= intlogbase2[fact / 4];
/*            fprintf(stderr, "op = %s; fact = %d\n", op, fact); */
            break;
        case 'b':
        case 'B':
            if (sscanf(&op[1], "%d", &fact) != 1) {
                fprintf(stderr, "*** op: %s; fact invalid\n", op);
                valid = FALSE;
                break;
            }
            if (i > 0) {
                fprintf(stderr, "*** op = %s; must be first op\n", op);
                valid = FALSE;
                break;
            }
            if (fact < 1) {
                fprintf(stderr, "*** op = %s; invalid fact = %d\n", op, fact);
                valid = FALSE;
                break;
            }
            border = fact;
/*            fprintf(stderr, "op = %s; fact = %d\n", op, fact); */
            break;
        default:
            fprintf(stderr, "*** nonexistent op = %s\n", op);
            valid = FALSE;
        }
        FREE(op);
    }

    if (border != 0 && netred != 0) {
        fprintf(stderr,
                "*** op = %s; border added but net reduction not 0\n", op);
        valid = FALSE;
    }
    return valid;
}


/*-----------------------------------------------------------------*
 *       Run a sequence of grayscale morphological operations      *
 *-----------------------------------------------------------------*/
/*!
 *  pixGrayMorphSequence()
 *
 *      Input:  pixs
 *              sequence (string specifying sequence)
 *              dispsep (horizontal separation in pixels between
 *                       successive displays; use zero to suppress display)
 *              dispy (if dispsep != 0, this gives the y-value of the
 *                     UL corner for display; otherwise it is ignored)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This works on 8 bpp grayscale images.
 *      (2) This runs a pipeline of operations; no branching is allowed.
 *      (3) This only uses brick SELs.
 *      (4) A new image is always produced; the input image is not changed.
 *      (5) This contains an interpreter, allowing sequences to be
 *          generated and run.
 *      (6) The format of the sequence string is defined below.
 *      (7) In addition to morphological operations, the composite
 *          morph/subtract tophat can be performed.
 *      (8) Sel sizes (width, height) must each be odd numbers.
 *      (9) Intermediate results can optionally be displayed
 *      (10) The sequence string is formatted as follows:
 *            - An arbitrary number of operations,  each separated
 *              by a '+' character.  White space is ignored.
 *            - Each operation begins with a case-independent character
 *              specifying the operation:
 *                 d or D  (dilation)
 *                 e or E  (erosion)
 *                 o or O  (opening)
 *                 c or C  (closing)
 *                 t or T  (tophat)
 *            - The args to the morphological operations are bricks of hits,
 *              and are formatted as a.b, where a and b are horizontal and
 *              vertical dimensions, rsp. (each must be an odd number)
 *            - The args to the tophat are w or W (for white tophat)
 *              or b or B (for black tophat), followed by a.b as for
 *              the dilation, erosion, opening and closing.
 *           Example valid sequences are:
 *             "c5.3 + o7.5"
 *             "c9.9 + tw9.9"
 */
PIX *
pixGrayMorphSequence(PIX         *pixs,
                     const char  *sequence,
                     l_int32      dispsep,
                     l_int32      dispy)
{
char    *rawop, *op;
l_int32  nops, i, valid, w, h, x;
PIX     *pixt1, *pixt2;
SARRAY  *sa;

    PROCNAME("pixGrayMorphSequence");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!sequence)
        return (PIX *)ERROR_PTR("sequence not defined", procName, NULL);

        /* Split sequence into individual operations */
    sa = sarrayCreate(0);
    sarraySplitString(sa, sequence, "+");
    nops = sarrayGetCount(sa);

        /* Verify that the operation sequence is valid */
    valid = TRUE;
    for (i = 0; i < nops; i++) {
        rawop = sarrayGetString(sa, i, 0);
        op = stringRemoveChars(rawop, " \n\t");
        switch (op[0])
        {
        case 'd':
        case 'D':
        case 'e':
        case 'E':
        case 'o':
        case 'O':
        case 'c':
        case 'C':
            if (sscanf(&op[1], "%d.%d", &w, &h) != 2) {
                fprintf(stderr, "*** op: %s invalid\n", op);
                valid = FALSE;
                break;
            }
            if (w < 1 || (w & 1) == 0 || h < 1 || (h & 1) == 0 ) {
                fprintf(stderr,
                        "*** op: %s; w = %d, h = %d; must both be odd\n",
                        op, w, h);
                valid = FALSE;
                break;
            }
/*            fprintf(stderr, "op = %s; w = %d, h = %d\n", op, w, h); */
            break;
        case 't':
        case 'T':
            if (op[1] != 'w' && op[1] != 'W' &&
                op[1] != 'b' && op[1] != 'B') {
                fprintf(stderr,
                        "*** op = %s; arg %c must be 'w' or 'b'\n", op, op[1]);
                valid = FALSE;
                break;
            }
            sscanf(&op[2], "%d.%d", &w, &h);
            if (w < 1 || (w & 1) == 0 || h < 1 || (h & 1) == 0 ) {
                fprintf(stderr,
                        "*** op: %s; w = %d, h = %d; must both be odd\n",
                        op, w, h);
                valid = FALSE;
                break;
            }
/*            fprintf(stderr, "op = %s", op); */
            break;
        default:
            fprintf(stderr, "*** nonexistent op = %s\n", op);
            valid = FALSE;
        }
        FREE(op);
    }
    if (!valid) {
        sarrayDestroy(&sa);
        return (PIX *)ERROR_PTR("sequence invalid", procName, NULL);
    }

        /* Parse and operate */
    pixt1 = pixCopy(NULL, pixs);
    pixt2 = NULL;
    x = 0;
    for (i = 0; i < nops; i++) {
        rawop = sarrayGetString(sa, i, 0);
        op = stringRemoveChars(rawop, " \n\t");
        switch (op[0])
        {
        case 'd':
        case 'D':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixt2 = pixDilateGray(pixt1, w, h);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, dispy);
                x += dispsep;
            }
            break;
        case 'e':
        case 'E':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixt2 = pixErodeGray(pixt1, w, h);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, dispy);
                x += dispsep;
            }
            break;
        case 'o':
        case 'O':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixt2 = pixOpenGray(pixt1, w, h);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, dispy);
                x += dispsep;
            }
            break;
        case 'c':
        case 'C':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixt2 = pixCloseGray(pixt1, w, h);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, dispy);
                x += dispsep;
            }
            break;
        case 't':
        case 'T':
            sscanf(&op[2], "%d.%d", &w, &h);
            if (op[1] == 'w' || op[1] == 'W')
                pixt2 = pixTophat(pixt1, w, h, L_TOPHAT_WHITE);
            else   /* 'b' or 'B' */
                pixt2 = pixTophat(pixt1, w, h, L_TOPHAT_BLACK);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, dispy);
                x += dispsep;
            }
            break;
        default:
            /* All invalid ops are caught in the first pass */
            break;
        }
        FREE(op);
    }

    sarrayDestroy(&sa);
    return pixt1;
}


/*-----------------------------------------------------------------*
 *         Run a sequence of color morphological operations        *
 *-----------------------------------------------------------------*/
/*!
 *  pixColorMorphSequence()
 *
 *      Input:  pixs
 *              sequence (string specifying sequence)
 *              dispsep (horizontal separation in pixels between
 *                       successive displays; use zero to suppress display)
 *              dispy (if dispsep != 0, this gives the y-value of the
 *                     UL corner for display; otherwise it is ignored)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This works on 32 bpp rgb images.
 *      (2) Each component is processed separately.
 *      (3) This runs a pipeline of operations; no branching is allowed.
 *      (4) This only uses brick SELs.
 *      (5) A new image is always produced; the input image is not changed.
 *      (6) This contains an interpreter, allowing sequences to be
 *          generated and run.
 *      (7) Sel sizes (width, height) must each be odd numbers.
 *      (8) The format of the sequence string is defined below.
 *      (9) Intermediate results can optionally be displayed.
 *      (10) The sequence string is formatted as follows:
 *            - An arbitrary number of operations,  each separated
 *              by a '+' character.  White space is ignored.
 *            - Each operation begins with a case-independent character
 *              specifying the operation:
 *                 d or D  (dilation)
 *                 e or E  (erosion)
 *                 o or O  (opening)
 *                 c or C  (closing)
 *            - The args to the morphological operations are bricks of hits,
 *              and are formatted as a.b, where a and b are horizontal and
 *              vertical dimensions, rsp. (each must be an odd number)
 *           Example valid sequences are:
 *             "c5.3 + o7.5"
 *             "D9.1"
 */
PIX *
pixColorMorphSequence(PIX         *pixs,
                      const char  *sequence,
                      l_int32      dispsep,
                      l_int32      dispy)
{
char    *rawop, *op;
l_int32  nops, i, valid, w, h, x;
PIX     *pixt1, *pixt2;
SARRAY  *sa;

    PROCNAME("pixColorMorphSequence");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!sequence)
        return (PIX *)ERROR_PTR("sequence not defined", procName, NULL);

        /* Split sequence into individual operations */
    sa = sarrayCreate(0);
    sarraySplitString(sa, sequence, "+");
    nops = sarrayGetCount(sa);

        /* Verify that the operation sequence is valid */
    valid = TRUE;
    for (i = 0; i < nops; i++) {
        rawop = sarrayGetString(sa, i, 0);
        op = stringRemoveChars(rawop, " \n\t");
        switch (op[0])
        {
        case 'd':
        case 'D':
        case 'e':
        case 'E':
        case 'o':
        case 'O':
        case 'c':
        case 'C':
            if (sscanf(&op[1], "%d.%d", &w, &h) != 2) {
                fprintf(stderr, "*** op: %s invalid\n", op);
                valid = FALSE;
                break;
            }
            if (w < 1 || (w & 1) == 0 || h < 1 || (h & 1) == 0 ) {
                fprintf(stderr,
                        "*** op: %s; w = %d, h = %d; must both be odd\n",
                        op, w, h);
                valid = FALSE;
                break;
            }
/*            fprintf(stderr, "op = %s; w = %d, h = %d\n", op, w, h); */
            break;
        default:
            fprintf(stderr, "*** nonexistent op = %s\n", op);
            valid = FALSE;
        }
        FREE(op);
    }
    if (!valid) {
        sarrayDestroy(&sa);
        return (PIX *)ERROR_PTR("sequence invalid", procName, NULL);
    }

        /* Parse and operate */
    pixt1 = pixCopy(NULL, pixs);
    pixt2 = NULL;
    x = 0;
    for (i = 0; i < nops; i++) {
        rawop = sarrayGetString(sa, i, 0);
        op = stringRemoveChars(rawop, " \n\t");
        switch (op[0])
        {
        case 'd':
        case 'D':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixt2 = pixColorMorph(pixt1, L_MORPH_DILATE, w, h);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, dispy);
                x += dispsep;
            }
            break;
        case 'e':
        case 'E':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixt2 = pixColorMorph(pixt1, L_MORPH_ERODE, w, h);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, dispy);
                x += dispsep;
            }
            break;
        case 'o':
        case 'O':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixt2 = pixColorMorph(pixt1, L_MORPH_OPEN, w, h);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, dispy);
                x += dispsep;
            }
            break;
        case 'c':
        case 'C':
            sscanf(&op[1], "%d.%d", &w, &h);
            pixt2 = pixColorMorph(pixt1, L_MORPH_CLOSE, w, h);
            pixDestroy(&pixt1);
            pixt1 = pixClone(pixt2);
            pixDestroy(&pixt2);
            if (dispsep > 0) {
                pixDisplay(pixt1, x, dispy);
                x += dispsep;
            }
            break;
        default:
            /* All invalid ops are caught in the first pass */
            break;
        }
        FREE(op);
    }

    sarrayDestroy(&sa);
    return pixt1;
}


