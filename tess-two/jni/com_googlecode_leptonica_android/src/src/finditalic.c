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
 * finditalic.c
 *
 *    Locate italic words.  This is an example of the use of
 *    hit-miss binary morphology with binary reconstruction (filling
 *    from a seed into a mask).
 *    Example: use with prog/italics.png
 *
 *      l_int32   pixItalicWords()
 */

#include "allheaders.h"

    /* ---------------------------------------------------------------  *
     * These hit-miss sels match the slanted edge of italic characters  *
     * ---------------------------------------------------------------  */
static const char *str_ital1 = "   o x"
                               "      "
                               "      "
                               "      "
                               "  o x "
                               "      "
                               "  C   "
                               "      "
                               " o x  "
                               "      "
                               "      "
                               "      "
                               "o x   ";

static const char *str_ital2 = "   o x"
                               "      "
                               "      "
                               "  o x "
                               "  C   "
                               "      "
                               " o x  "
                               "      "
                               "      "
                               "o x   ";

    /* ------------------------------------------------------------- *
     * This sel removes noise that is not oriented as a slanted edge *
     * ------------------------------------------------------------- */
static const char *str_ital3 = " x"
                               "Cx"
                               "x "
                               "x ";


/*!
 *  pixItalicWords()
 *
 *      Input:  pixs (1 bpp)
 *              boxaw (<optional> word bounding boxes; can be NULL)
 *              pixw (<optional> word box mask; can be NULL)
 *              &boxa (<return> boxa of italian words)
 *              debugflag (1 for debug output; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) You can input the bounding boxes for the words in one of
 *          two forms: as bounding boxes (@boxaw) or as a word mask with
 *          the word bounding boxes filled (@pixw).  For example,
 *          to compute @pixw, you can use pixWordMaskByDilation().
 *      (2) Alternatively, you can set both of these inputs to NULL,
 *          in which case the word mask is generated here.  This is
 *          done by dilating and closing the input image to connect
 *          letters within a word, while leaving the words separated.
 *          The parameters are chosen under the assumption that the
 *          input is 10 to 12 pt text, scanned at about 300 ppi.
 *      (3) sel_ital1 and sel_ital2 detect the right edges that are
 *          nearly vertical, at approximately the angle of italic
 *          strokes.  We use the right edge to avoid getting seeds
 *          from lower-case 'y'.  The typical italic slant has a smaller
 *          angle with the vertical than the 'W', so in most cases we
 *          will not trigger on the slanted lines in the 'W'.
 *      (4) Note that sel_ital2 is shorter than sel_ital1.  It is
 *          more appropriate for a typical font scanned at 200 ppi.
 */
l_int32
pixItalicWords(PIX     *pixs,
               BOXA    *boxaw,
               PIX     *pixw,
               BOXA   **pboxa,
               l_int32  debugflag)
{
BOXA  *boxa;
PIX   *pixsd, *pixm, *pixd;
SEL   *sel_ital1, *sel_ital2, *sel_ital3;

    PROCNAME("pixItalicWords");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pboxa)
        return ERROR_INT("&boxa not defined", procName, 1);
    if (boxaw && pixw)
        return ERROR_INT("both boxaw and pixw are defined", procName, 1);

    sel_ital1 = selCreateFromString(str_ital1, 13, 6, NULL);
    sel_ital2 = selCreateFromString(str_ital2, 10, 6, NULL);
    sel_ital3 = selCreateFromString(str_ital3, 4, 2, NULL);

        /* Make the italic seed: extract with HMT; remove noise.
         * The noise removal close/open is important to exclude
         * situations where a small slanted line accidentally
         * matches sel_ital1. */
    pixsd = pixHMT(NULL, pixs, sel_ital1);
    pixClose(pixsd, pixsd, sel_ital3);
    pixOpen(pixsd, pixsd, sel_ital3);

        /* Make the word mask.  Use input boxes or mask if given. */
    if (boxaw) {
        pixm = pixCreateTemplate(pixs);
        pixMaskBoxa(pixm, pixm, boxaw, L_SET_PIXELS);
    }
    else if (pixw) {
        pixm = pixClone(pixw);
    }
    else
        pixm = pixMorphSequence(pixs, "d1.5 + c6.1", 0);

        /* Binary reconstruction to fill in those word mask
         * components for which there is at least one seed pixel. */
    pixd = pixSeedfillBinary(NULL, pixsd, pixm, 8);
    boxa = pixConnComp(pixd, NULL, 8);
    *pboxa = boxa;

    if (debugflag) {
        l_int32  res;
        BOXA  *boxat;
        PIXA  *pad;
        PIX   *pixt1, *pixt2;
        pad = pixaCreate(0);
            /* Save these at 2x reduction */
        pixSaveTiledOutline(pixs, pad, 2, 1, 20, 2, 32);
        pixSaveTiledOutline(pixsd, pad, 2, 1, 20, 2, 0);
        boxat = pixConnComp(pixm, NULL, 8);
        boxaWrite("/tmp/junkital.ba", boxat);
        pixt1 = pixConvertTo32(pixm);
        pixRenderBoxaArb(pixt1, boxat, 3, 255, 0, 0);
        pixSaveTiledOutline(pixt1, pad, 2, 1, 20, 2, 0);
        pixDestroy(&pixt1);
        pixSaveTiledOutline(pixd, pad, 2, 1, 20, 2, 0);
        pixt1 = pixConvertTo32(pixs);
        pixRenderBoxaArb(pixt1, boxa, 3, 255, 0, 0);
        pixSaveTiledOutline(pixt1, pad, 2, 1, 20, 2, 0);
        pixt2 = pixaDisplay(pad, 0, 0);
        pixWrite("/tmp/junkital.png", pixt2, IFF_PNG);
            /* The pixs resolution is approximately:
             *    300 * (width of pixs) / 2000
             * and the images have been saved at half this resolution.   */
        res = (150 * pixGetWidth(pixs)) / 2000;
        L_INFO_INT("resolution = %d", procName, res);
        pixaConvertToPdf(pad, res, 1.0, 3, 75, "Italic Finder",
                         "/tmp/junkital.pdf");
        pixaDestroy(&pad);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        boxaDestroy(&boxat);
    }

    selDestroy(&sel_ital1);
    selDestroy(&sel_ital2);
    selDestroy(&sel_ital3);
    pixDestroy(&pixsd);
    pixDestroy(&pixm);
    pixDestroy(&pixd);
    return 0;
}
