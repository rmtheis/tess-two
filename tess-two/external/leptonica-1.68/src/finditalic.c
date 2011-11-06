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
 * finditalic.c
 *
 *    Locate italic words.  This is an example of the use of
 *    hit-miss binary morphology with binary reconstruction (filling
 *    from a seed into a mask).
 *    Example: use with prog/italics.png
 *
 *      l_int32   pixItalicWords()
 */

#include <stdio.h>
#include <stdlib.h>
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

	/* Make the italic seed: extract with HMT; remove noise */
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

	/* Binary reconstruction */
    pixd = pixSeedfillBinary(NULL, pixsd, pixm, 8);
    boxa = pixConnComp(pixd, NULL, 8);
    *pboxa = boxa;

    if (debugflag) {
        BOXA  *boxat;
        PIXA  *pad;
        PIX   *pixt1, *pixt2;
        pad = pixaCreate(0);
        pixSaveTiledOutline(pixs, pad, 2, 1, 20, 2, 32);
        pixSaveTiledOutline(pixsd, pad, 2, 1, 20, 2, 0);
        boxat = pixConnComp(pixm, NULL, 8);
        boxaWrite("/tmp/junkboxa.ba", boxat);
        pixt1 = pixConvertTo32(pixm);
        pixRenderBoxaArb(pixt1, boxat, 3, 255, 0, 0);
        pixSaveTiledOutline(pixt1, pad, 2, 1, 20, 2, 0);
        pixDestroy(&pixt1);
        pixSaveTiledOutline(pixd, pad, 2, 1, 20, 2, 0);
        pixt1 = pixConvertTo32(pixs);
        pixRenderBoxaArb(pixt1, boxa, 3, 255, 0, 0);
        pixSaveTiledOutline(pixt1, pad, 2, 1, 20, 2, 0);
        pixt2 = pixaDisplay(pad, 0, 0);
        pixWrite("/tmp/junkdebug.png", pixt2, IFF_PNG);
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

