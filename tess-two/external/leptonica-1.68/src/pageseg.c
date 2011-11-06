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
 *   pageseg.c
 *
 *      Top level page segmentation
 *          l_int32   pixGetRegionsBinary()
 *
 *      Halftone region extraction
 *          PIX      *pixGenHalftoneMask()
 *
 *      Textline extraction
 *          PIX      *pixGenTextlineMask()
 *
 *      Textblock extraction
 *          PIX      *pixGenTextblockMask()
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


/*------------------------------------------------------------------*
 *                     Top level page segmentation                  *
 *------------------------------------------------------------------*/
/*!
 *  pixGetRegionsBinary()
 *
 *      Input:  pixs (1 bpp, assumed to be 300 to 400 ppi)
 *              &pixhm (<optional return> halftone mask)
 *              &pixtm (<optional return> textline mask)
 *              &pixtb (<optional return> textblock mask)
 *              debug (flag: set to 1 for debug output)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) It is best to deskew the image before segmenting.
 *      (2) The debug flag enables a number of outputs.  These
 *          are included to show how to generate and save/display
 *          these results.
 */
l_int32
pixGetRegionsBinary(PIX     *pixs,
                    PIX    **ppixhm,
                    PIX    **ppixtm,
                    PIX    **ppixtb,
                    l_int32  debug)
{
char    *tempname;
l_int32  htfound, tlfound;
PIX     *pixr, *pixt1, *pixt2;
PIX     *pixtext;  /* text pixels only */
PIX     *pixhm2;   /* halftone mask; 2x reduction */
PIX     *pixhm;    /* halftone mask;  */
PIX     *pixtm2;   /* textline mask; 2x reduction */
PIX     *pixtm;    /* textline mask */
PIX     *pixvws;   /* vertical white space mask */
PIX     *pixtb2;   /* textblock mask; 2x reduction */
PIX     *pixtbf2;  /* textblock mask; 2x reduction; small comps filtered */
PIX     *pixtb;    /* textblock mask */

    PROCNAME("pixGetRegionsBinary");

    if (ppixhm) *ppixhm = NULL;
    if (ppixtm) *ppixtm = NULL;
    if (ppixtb) *ppixtb = NULL;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not 1 bpp", procName, 1);

        /* 2x reduce, to 150 -200 ppi */
    pixr = pixReduceRankBinaryCascade(pixs, 1, 0, 0, 0);
    pixDisplayWrite(pixr, debug);

        /* Get the halftone mask */
    pixhm2 = pixGenHalftoneMask(pixr, &pixtext, &htfound, debug);

        /* Get the textline mask from the text pixels */
    pixtm2 = pixGenTextlineMask(pixtext, &pixvws, &tlfound, debug);

        /* Get the textblock mask from the textline mask */
    pixtb2 = pixGenTextblockMask(pixtm2, pixvws, debug);
    pixDestroy(&pixr);
    pixDestroy(&pixtext);
    pixDestroy(&pixvws);

        /* Remove small components from the mask, where a small
         * component is defined as one with both width and height < 60 */
    pixtbf2 = pixSelectBySize(pixtb2, 60, 60, 4, L_SELECT_IF_EITHER,
                              L_SELECT_IF_GTE, NULL);
    pixDestroy(&pixtb2);
    pixDisplayWriteFormat(pixtbf2, debug, IFF_PNG);

        /* Expand all masks to full resolution, and do filling or
         * small dilations for better coverage. */
    pixhm = pixExpandReplicate(pixhm2, 2);
    pixt1 = pixSeedfillBinary(NULL, pixhm, pixs, 8);
    pixOr(pixhm, pixhm, pixt1);
    pixDestroy(&pixt1);
    pixDisplayWriteFormat(pixhm, debug, IFF_PNG);

    pixt1 = pixExpandReplicate(pixtm2, 2);
    pixtm = pixDilateBrick(NULL, pixt1, 3, 3);
    pixDestroy(&pixt1);
    pixDisplayWriteFormat(pixtm, debug, IFF_PNG);

    pixt1 = pixExpandReplicate(pixtbf2, 2);
    pixtb = pixDilateBrick(NULL, pixt1, 3, 3);
    pixDestroy(&pixt1);
    pixDisplayWriteFormat(pixtb, debug, IFF_PNG);

    pixDestroy(&pixhm2);
    pixDestroy(&pixtm2);
    pixDestroy(&pixtbf2);

        /* Debug: identify objects that are neither text nor halftone image */
    if (debug) {
        pixt1 = pixSubtract(NULL, pixs, pixtm);  /* remove text pixels */
        pixt2 = pixSubtract(NULL, pixt1, pixhm);  /* remove halftone pixels */
        pixDisplayWriteFormat(pixt2, 1, IFF_PNG);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }

        /* Debug: display textline components with random colors */
    if (debug) {
        l_int32  w, h;
        BOXA    *boxa;
        PIXA    *pixa;
        boxa = pixConnComp(pixtm, &pixa, 8);
        pixGetDimensions(pixtm, &w, &h, NULL);
        pixt1 = pixaDisplayRandomCmap(pixa, w, h);
        pixcmapResetColor(pixGetColormap(pixt1), 0, 255, 255, 255);
        pixDisplay(pixt1, 100, 100);
        pixDisplayWriteFormat(pixt1, 1, IFF_PNG);
        pixaDestroy(&pixa);
        boxaDestroy(&boxa);
        pixDestroy(&pixt1);
    }

        /* Debug: identify the outlines of each textblock */
    if (debug) {
        PIXCMAP  *cmap;
        PTAA     *ptaa;
        ptaa = pixGetOuterBordersPtaa(pixtb);
        tempname = genTempFilename("/tmp", "tb_outlines.ptaa", 0, 0);
        ptaaWrite(tempname, ptaa, 1);
        FREE(tempname);
        pixt1 = pixRenderRandomCmapPtaa(pixtb, ptaa, 1, 16, 1);
        cmap = pixGetColormap(pixt1);
        pixcmapResetColor(cmap, 0, 130, 130, 130);
        pixDisplay(pixt1, 500, 100);
        pixDisplayWriteFormat(pixt1, 1, IFF_PNG);
        pixDestroy(&pixt1);
        ptaaDestroy(&ptaa);
    }

        /* Debug: get b.b. for all mask components */
    if (debug) {
        BOXA  *bahm, *batm, *batb;
        bahm = pixConnComp(pixhm, NULL, 4);
        batm = pixConnComp(pixtm, NULL, 4);
        batb = pixConnComp(pixtb, NULL, 4);
        tempname = genTempFilename("/tmp", "htmask.boxa", 0, 0);
        boxaWrite(tempname, bahm);
        FREE(tempname);
        tempname = genTempFilename("/tmp", "textmask.boxa", 0, 0);
        boxaWrite(tempname, batm);
        FREE(tempname);
        tempname = genTempFilename("/tmp", "textblock.boxa", 0, 0);
        boxaWrite(tempname, batb);
        FREE(tempname);
	boxaDestroy(&bahm);
	boxaDestroy(&batm);
	boxaDestroy(&batb);
    }

    if (ppixhm)
        *ppixhm = pixhm;
    else
        pixDestroy(&pixhm);
    if (ppixtm)
        *ppixtm = pixtm;
    else
        pixDestroy(&pixtm);
    if (ppixtb)
        *ppixtb = pixtb;
    else
        pixDestroy(&pixtb);

    return 0;
}


/*------------------------------------------------------------------*
 *                    Halftone region extraction                    *
 *------------------------------------------------------------------*/
/*!
 *  pixGenHalftoneMask()
 *
 *      Input:  pixs (1 bpp, assumed to be 150 to 200 ppi)
 *              &pixtext (<optional return> text part of pixs)
 *              &htfound (<optional return> 1 if the mask is not empty)
 *              debug (flag: 1 for debug output)
 *      Return: pixd (halftone mask), or null on error
 */
PIX *
pixGenHalftoneMask(PIX      *pixs,
                   PIX     **ppixtext,
                   l_int32  *phtfound,
                   l_int32   debug)
{
l_int32  empty;
PIX     *pixt1, *pixt2, *pixhs, *pixhm, *pixd;

    PROCNAME("pixGenHalftoneMask");

    if (ppixtext) *ppixtext = NULL;
    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);

        /* Compute seed for halftone parts at 8x reduction */
    pixt1 = pixReduceRankBinaryCascade(pixs, 4, 4, 3, 0);
    pixt2 = pixOpenBrick(NULL, pixt1, 5, 5);
    pixhs = pixExpandReplicate(pixt2, 8);  /* back to 2x reduction */
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDisplayWriteFormat(pixhs, debug, IFF_PNG);

        /* Compute mask for connected regions */
    pixhm = pixCloseSafeBrick(NULL, pixs, 4, 4);
    pixDisplayWriteFormat(pixhm, debug, IFF_PNG);

        /* Fill seed into mask to get halftone mask */
    pixd = pixSeedfillBinary(NULL, pixhs, pixhm, 4);

#if 0
        /* Moderate opening to remove thin lines, etc. */
    pixOpenBrick(pixd, pixd, 10, 10);
    pixDisplayWrite(pixd, debug);
#endif

        /* Check if mask is empty */
    pixZero(pixd, &empty);
    if (phtfound) {
        *phtfound = 0;
        if (!empty)
            *phtfound = 1;
    }

        /* Optionally, get all pixels that are not under the halftone mask */
    if (ppixtext) {
        if (empty)
            *ppixtext = pixCopy(NULL, pixs);
        else
            *ppixtext = pixSubtract(NULL, pixs, pixd);
        pixDisplayWriteFormat(*ppixtext, debug, IFF_PNG);
    }

    pixDestroy(&pixhs);
    pixDestroy(&pixhm);
    return pixd;
}


/*------------------------------------------------------------------*
 *                         Textline extraction                      *
 *------------------------------------------------------------------*/
/*!
 *  pixGenTextlineMask()
 *
 *      Input:  pixs (1 bpp, assumed to be 150 to 200 ppi)
 *              &pixvws (<return> vertical whitespace mask)
 *              &tlfound (<optional return> 1 if the mask is not empty)
 *              debug (flag: 1 for debug output)
 *      Return: pixd (textline mask), or null on error
 *
 *  Notes:
 *      (1) The input pixs should be deskewed.
 *      (2) pixs should have no halftone pixels.
 *      (3) Both the input image and the returned textline mask
 *          are at the same resolution.
 */
PIX *
pixGenTextlineMask(PIX      *pixs,
                   PIX     **ppixvws,
                   l_int32  *ptlfound,
                   l_int32   debug)
{
l_int32  empty;
PIX     *pixt1, *pixt2, *pixvws, *pixd;

    PROCNAME("pixGenTextlineMask");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!ppixvws)
        return (PIX *)ERROR_PTR("&pixvws not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);

        /* First we need a vertical whitespace mask.  Invert the image. */
    pixt1 = pixInvert(NULL, pixs);

        /* The whitespace mask will break textlines where there
         * is a large amount of white space below or above.
         * This can be prevented by identifying regions of the
         * inverted image that have large horizontal extent (bigger than
	 * the separation between columns) and significant
         * vertical extent (bigger than the separation between
	 * textlines), and subtracting this from the bg. */
    pixt2 = pixMorphCompSequence(pixt1, "o80.60", 0);
    pixSubtract(pixt1, pixt1, pixt2);
    pixDisplayWriteFormat(pixt1, debug, IFF_PNG);
    pixDestroy(&pixt2);

        /* Identify vertical whitespace by opening the remaining bg.
         * o5.1 removes thin vertical bg lines and o1.200 extracts
         * long vertical bg lines. */
    pixvws = pixMorphCompSequence(pixt1, "o5.1 + o1.200", 0);
    *ppixvws = pixvws;
    pixDisplayWriteFormat(pixvws, debug, IFF_PNG);
    pixDestroy(&pixt1);

        /* Three steps to getting text line mask:
         *   (1) close the characters and words in the textlines
         *   (2) open the vertical whitespace corridors back up
         *   (3) small opening to remove noise    */
    pixt1 = pixCloseSafeBrick(NULL, pixs, 30, 1);
    pixDisplayWrite(pixt1, debug);
    pixd = pixSubtract(NULL, pixt1, pixvws);
    pixOpenBrick(pixd, pixd, 3, 3);
    pixDisplayWriteFormat(pixd, debug, IFF_PNG);
    pixDestroy(&pixt1);

        /* Check if text line mask is empty */
    if (ptlfound) {
        *ptlfound = 0;
        pixZero(pixd, &empty);
        if (!empty)
            *ptlfound = 1;
    }

    return pixd;
}


/*------------------------------------------------------------------*
 *                       Textblock extraction                       *
 *------------------------------------------------------------------*/
/*!
 *  pixGenTextblockMask()
 *
 *      Input:  pixs (1 bpp, textline mask, assumed to be 150 to 200 ppi)
 *              pixvws (vertical white space mask) 
 *              debug (flag: 1 for debug output)
 *      Return: pixd (textblock mask), or null on error
 *
 *  Notes:
 *      (1) Both the input masks (textline and vertical white space) and
 *          the returned textblock mask are at the same resolution.
 *      (2) The result is somewhat noisy, in that small "blocks" of
 *          text may be included.  These can be removed by post-processing,
 *          using, e.g.,
 *             pixSelectBySize(pix, 60, 60, 4, L_SELECT_IF_EITHER,
 *                             L_SELECT_IF_GTE, NULL);
 */
PIX *
pixGenTextblockMask(PIX     *pixs,
                    PIX     *pixvws,
                    l_int32  debug)
{
PIX  *pixt1, *pixt2, *pixt3, *pixd;

    PROCNAME("pixGenTextblockMask");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!pixvws)
        return (PIX *)ERROR_PTR("pixvws not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);

        /* Join pixels vertically to make a textblock mask */
    pixt1 = pixMorphSequence(pixs, "c1.10 + o4.1", 0);
    pixDisplayWriteFormat(pixt1, debug, IFF_PNG);

        /* Solidify the textblock mask and remove noise: 
         *   (1) For each cc, close the blocks and dilate slightly
	 *       to form a solid mask.
         *   (2) Small horizontal closing between components.
         *   (3) Open the white space between columns, again.
         *   (4) Remove small components. */
    pixt2 = pixMorphSequenceByComponent(pixt1, "c30.30 + d3.3", 8, 0, 0, NULL);
    pixCloseSafeBrick(pixt2, pixt2, 10, 1);
    pixDisplayWriteFormat(pixt2, debug, IFF_PNG);
    pixt3 = pixSubtract(NULL, pixt2, pixvws);
    pixDisplayWriteFormat(pixt3, debug, IFF_PNG);
    pixd = pixSelectBySize(pixt3, 25, 5, 8, L_SELECT_IF_BOTH,
                            L_SELECT_IF_GTE, NULL);
    pixDisplayWriteFormat(pixd, debug, IFF_PNG);

    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);
    return pixd;
}


