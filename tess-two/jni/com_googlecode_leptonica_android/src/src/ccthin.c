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
 *  ccthin.c
 *
 *     PIX    *pixThin()
 *     PIX    *pixThinGeneral()
 *     PIX    *pixThinExamples()
 */

#include "allheaders.h"


    /* ------------------------------------------------------------
     * These sels (and their rotated counterparts) are the useful
     * 3x3 Sels for thinning.   The notation is based on
     * "Connectivity-preserving morphological image transformations,"
     * a version of which can be found at
     *           http://www.leptonica.com/papers/conn.pdf
     * ------------------------------------------------------------ */

    /* Sels for 4-connected thinning */
static const char *sel_4_1 = "  x"
                             "oCx"
                             "  x";

static const char *sel_4_2 = "  x"
                             "oCx"
                             " o ";

static const char *sel_4_3 = " o "
                             "oCx"
                             "  x";

static const char *sel_4_4 = " o "
                             "oCx"
                             " o ";

static const char *sel_4_5 = " ox"
                             "oCx"
                             " o ";

static const char *sel_4_6 = " o "
                             "oCx"
                             " ox";

static const char *sel_4_7 = " xx"
                             "oCx"
                             " o ";

static const char *sel_4_8 = "  x"
                             "oCx"
                             "o x";

static const char *sel_4_9 = "o x"
                             "oCx"
                             "  x";

    /* Sels for 8-connected thinning */
static const char *sel_8_1 = " x "
                             "oCx"
                             " x ";

static const char *sel_8_2 = " x "
                             "oCx"
                             "o  ";

static const char *sel_8_3 = "o  "
                             "oCx"
                             " x ";

static const char *sel_8_4 = "o  "
                             "oCx"
                             "o  ";

static const char *sel_8_5 = "o x"
                             "oCx"
                             "o  ";

static const char *sel_8_6 = "o  "
                             "oCx"
                             "o x";

static const char *sel_8_7 = " x "
                             "oCx"
                             "oo ";

static const char *sel_8_8 = " x "
                             "oCx"
                             "ox ";

static const char *sel_8_9 = "ox "
                             "oCx"
                             " x ";

    /* Sels for both 4 and 8-connected thinning */
static const char *sel_48_1 = " xx"
                              "oCx"
                              "oo ";

static const char *sel_48_2 = "o x"
                              "oCx"
                              "o x";

#ifndef NO_CONSOLE_IO
#define  DEBUG_SELS     0
#endif   /* ~NO_CONSOLE_IO */


/*----------------------------------------------------------------*
 *                      CC-preserving thinning                    *
 *----------------------------------------------------------------*/
/*!
 *  pixThin()
 *
 *      Input:  pixs (1 bpp)
 *              type (L_THIN_FG, L_THIN_BG)
 *              connectivity (4 or 8)
 *              maxiters (max number of iters allowed; use 0 to iterate
 *                        until completion)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) See "Connectivity-preserving morphological image transformations,"
 *          Dan S. Bloomberg, in SPIE Visual Communications and Image
 *          Processing, Conference 1606, pp. 320-334, November 1991,
 *          Boston, MA.   A web version is available at
 *              http://www.leptonica.com/papers/conn.pdf
 *      (2) We implement here two of the best iterative
 *          morphological thinning algorithms, for 4 c.c and 8 c.c.
 *          Each iteration uses a mixture of parallel operations
 *          (using several different 3x3 Sels) and serial operations.
 *          Specifically, each thinning iteration consists of
 *          four sequential thinnings from each of four directions.
 *          Each of these thinnings is a parallel composite
 *          operation, where the union of a set of HMTs are set
 *          subtracted from the input.  For 4-cc thinning, we
 *          use 3 HMTs in parallel, and for 8-cc thinning we use 4 HMTs.
 *      (3) A "good" thinning algorithm is one that generates a skeleton
 *          that is near the medial axis and has neither pruned
 *          real branches nor left extra dendritic branches.
 *      (4) To thin the foreground, which is the usual situation,
 *          use type == L_THIN_FG.  Thickening the foreground is equivalent
 *          to thinning the background (type == L_THIN_BG), where the
 *          opposite connectivity gets preserved.  For example, to thicken
 *          the fg using 4-connectivity, we thin the bg using Sels that
 *          preserve 8-connectivity.
 */
PIX *
pixThin(PIX     *pixs,
        l_int32  type,
        l_int32  connectivity,
        l_int32  maxiters)
{
PIX   *pixd;
SEL   *sel;
SELA  *sela;

    PROCNAME("pixThin");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);
    if (type != L_THIN_FG && type != L_THIN_BG)
        return (PIX *)ERROR_PTR("invalid fg/bg type", procName, NULL);
    if (connectivity != 4 && connectivity != 8)
        return (PIX *)ERROR_PTR("connectivity not 4 or 8", procName, NULL);
    if (maxiters == 0) maxiters = 10000;

    sela = selaCreate(4);
    if (connectivity == 4) {
        sel = selCreateFromString(sel_4_1, 3, 3, "sel_4_1");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_4_2, 3, 3, "sel_4_2");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_4_3, 3, 3, "sel_4_3");
        selaAddSel(sela, sel, NULL, 0);
    } else {  /* connectivity == 8 */
        sel = selCreateFromString(sel_8_2, 3, 3, "sel_8_2");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_8_3, 3, 3, "sel_8_3");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_8_5, 3, 3, "sel_8_5");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_8_6, 3, 3, "sel_8_6");
        selaAddSel(sela, sel, NULL, 0);
    }

    pixd = pixThinGeneral(pixs, type, sela, maxiters);

    selaDestroy(&sela);
    return pixd;
}


/*!
 *  pixThinGeneral()
 *
 *      Input:  pixs (1 bpp)
 *              type (L_THIN_FG, L_THIN_BG)
 *              sela (of Sels for parallel composite HMTs)
 *              maxiters (max number of iters allowed; use 0 to iterate
 *                        until completion)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) See notes in pixThin().  That function chooses among
 *          the best of the Sels for thinning.
 *      (2) This is a general function that takes a Sela of HMTs
 *          that are used in parallel for thinning from each
 *          of four directions.  One iteration consists of four
 *          such parallel thins.
 */
PIX *
pixThinGeneral(PIX     *pixs,
               l_int32  type,
               SELA    *sela,
               l_int32  maxiters)
{
l_int32  i, j, r, nsels, same;
PIXA    *pixahmt;
PIX    **pixhmt;  /* array owned by pixahmt; do not destroy! */
PIX     *pixd, *pixt;
SEL     *sel, *selr;

    PROCNAME("pixThinGeneral");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);
    if (type != L_THIN_FG && type != L_THIN_BG)
        return (PIX *)ERROR_PTR("invalid fg/bg type", procName, NULL);
    if (!sela)
        return (PIX *)ERROR_PTR("sela not defined", procName, NULL);
    if (maxiters == 0) maxiters = 10000;

        /* Set up array of temp pix to hold hmts */
    nsels = selaGetCount(sela);
    pixahmt = pixaCreate(nsels);
    for (i = 0; i < nsels; i++) {
        pixt = pixCreateTemplate(pixs);
        pixaAddPix(pixahmt, pixt, L_INSERT);
    }
    pixhmt = pixaGetPixArray(pixahmt);
    if (!pixhmt)
        return (PIX *)ERROR_PTR("pixhmt array not made", procName, NULL);

#if  DEBUG_SELS
    pixt = selaDisplayInPix(sela, 35, 3, 15, 4);
    pixDisplayWithTitle(pixt, 100, 100, "allsels", 1);
    pixDestroy(&pixt);
#endif  /* DEBUG_SELS */

        /* Set up initial image for fg thinning */
    if (type == L_THIN_FG)
        pixd = pixCopy(NULL, pixs);
    else  /* bg thinning */
        pixd = pixInvert(NULL, pixs);

        /* Thin the fg, with up to maxiters iterations */
    for (i = 0; i < maxiters; i++) {
        pixt = pixCopy(NULL, pixd);  /* test for completion */
        for (r = 0; r < 4; r++) {  /* over 90 degree rotations of Sels */
            for (j = 0; j < nsels; j++) {  /* over individual sels in sela */
                sel = selaGetSel(sela, j);  /* not a copy */
                selr = selRotateOrth(sel, r);
                pixHMT(pixhmt[j], pixd, selr);
                selDestroy(&selr);
                if (j > 0)
                    pixOr(pixhmt[0], pixhmt[0], pixhmt[j]);  /* accum result */
            }
            pixSubtract(pixd, pixd, pixhmt[0]);  /* remove result */
        }
        pixEqual(pixd, pixt, &same);
        pixDestroy(&pixt);
        if (same) {
            L_INFO("%d iterations to completion\n", procName, i);
            break;
        }
    }

    if (type == L_THIN_BG)
        pixInvert(pixd, pixd);

    pixaDestroy(&pixahmt);
    return pixd;
}


/*!
 *  pixThinExamples()
 *
 *      Input:  pixs (1 bpp)
 *              type (L_THIN_FG, L_THIN_BG)
 *              index (into specific examples; valid 1-9; see notes)
 *              maxiters (max number of iters allowed; use 0 to iterate
 *                        until completion)
 *              selfile (<optional> filename for output sel display)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) See notes in pixThin().  The examples are taken from
 *          the paper referenced there.
 *      (2) Here we allow specific sets of HMTs to be used in
 *          parallel for thinning from each of four directions.
 *          One iteration consists of four such parallel thins.
 *      (3) The examples are indexed as follows:
 *          Thinning  (e.g., run to completion):
 *              index = 1     sel_4_1, sel_4_5, sel_4_6
 *              index = 2     sel_4_1, sel_4_7, sel_4_7_rot
 *              index = 3     sel_48_1, sel_48_1_rot, sel_48_2
 *              index = 4     sel_8_2, sel_8_3, sel_48_2
 *              index = 5     sel_8_1, sel_8_5, sel_8_6
 *              index = 6     sel_8_2, sel_8_3, sel_8_8, sel_8_9
 *              index = 7     sel_8_5, sel_8_6, sel_8_7, sel_8_7_rot
 *          Thickening:
 *              index = 8     sel_4_2, sel_4_3 (e.g,, do just a few iterations)
 *              index = 9     sel_8_4 (e.g., do just a few iterations)
 */
PIX *
pixThinExamples(PIX         *pixs,
                l_int32      type,
                l_int32      index,
                l_int32      maxiters,
                const char  *selfile)
{
PIX   *pixd, *pixt;
SEL   *sel;
SELA  *sela;

    PROCNAME("pixThinExamples");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);
    if (type != L_THIN_FG && type != L_THIN_BG)
        return (PIX *)ERROR_PTR("invalid fg/bg type", procName, NULL);
    if (index < 1 || index > 9)
        return (PIX *)ERROR_PTR("invalid index", procName, NULL);
    if (maxiters == 0) maxiters = 10000;

    switch(index)
    {
    case 1:
        sela = selaCreate(3);
        sel = selCreateFromString(sel_4_1, 3, 3, "sel_4_1");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_4_5, 3, 3, "sel_4_5");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_4_6, 3, 3, "sel_4_6");
        selaAddSel(sela, sel, NULL, 0);
        break;
    case 2:
        sela = selaCreate(3);
        sel = selCreateFromString(sel_4_1, 3, 3, "sel_4_1");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_4_7, 3, 3, "sel_4_7");
        selaAddSel(sela, sel, NULL, 0);
        sel = selRotateOrth(sel, 1);
        selaAddSel(sela, sel, "sel_4_7_rot", 0);
        break;
    case 3:
        sela = selaCreate(3);
        sel = selCreateFromString(sel_48_1, 3, 3, "sel_48_1");
        selaAddSel(sela, sel, NULL, 0);
        sel = selRotateOrth(sel, 1);
        selaAddSel(sela, sel, "sel_48_1_rot", 0);
        sel = selCreateFromString(sel_48_2, 3, 3, "sel_48_2");
        selaAddSel(sela, sel, NULL, 0);
        break;
    case 4:
        sela = selaCreate(3);
        sel = selCreateFromString(sel_8_2, 3, 3, "sel_8_2");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_8_3, 3, 3, "sel_8_3");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_48_2, 3, 3, "sel_48_2");
        selaAddSel(sela, sel, NULL, 0);
        break;
    case 5:
        sela = selaCreate(3);
        sel = selCreateFromString(sel_8_1, 3, 3, "sel_8_1");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_8_5, 3, 3, "sel_8_5");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_8_6, 3, 3, "sel_8_6");
        selaAddSel(sela, sel, NULL, 0);
        break;
    case 6:
        sela = selaCreate(4);
        sel = selCreateFromString(sel_8_2, 3, 3, "sel_8_2");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_8_3, 3, 3, "sel_8_3");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_8_8, 3, 3, "sel_8_8");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_8_9, 3, 3, "sel_8_9");
        selaAddSel(sela, sel, NULL, 0);
        break;
    case 7:
        sela = selaCreate(4);
        sel = selCreateFromString(sel_8_5, 3, 3, "sel_8_5");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_8_6, 3, 3, "sel_8_6");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_8_7, 3, 3, "sel_8_7");
        selaAddSel(sela, sel, NULL, 0);
        sel = selRotateOrth(sel, 1);
        selaAddSel(sela, sel, "sel_8_7_rot", 0);
        break;
    case 8:  /* thicken for this one; just a few iterations */
        sela = selaCreate(2);
        sel = selCreateFromString(sel_4_2, 3, 3, "sel_4_2");
        selaAddSel(sela, sel, NULL, 0);
        sel = selCreateFromString(sel_4_3, 3, 3, "sel_4_3");
        selaAddSel(sela, sel, NULL, 0);
        pixt = pixThinGeneral(pixs, type, sela, maxiters);
        pixd = pixRemoveBorderConnComps(pixt, 4);
        pixDestroy(&pixt);
        break;
    case 9:  /* thicken for this one; just a few iterations */
        sela = selaCreate(1);
        sel = selCreateFromString(sel_8_4, 3, 3, "sel_8_4");
        selaAddSel(sela, sel, NULL, 0);
        pixt = pixThinGeneral(pixs, type, sela, maxiters);
        pixd = pixRemoveBorderConnComps(pixt, 4);
        pixDestroy(&pixt);
        break;
    default:
        return (PIX *)ERROR_PTR("invalid index", procName, NULL);
    }

    if (index <= 7)
        pixd = pixThinGeneral(pixs, type, sela, maxiters);

        /* Optionally display the sels */
    if (selfile) {
        pixt = selaDisplayInPix(sela, 35, 3, 15, 4);
        pixWrite(selfile, pixt, IFF_PNG);
        pixDestroy(&pixt);
    }

    selaDestroy(&sela);
    return pixd;
}
