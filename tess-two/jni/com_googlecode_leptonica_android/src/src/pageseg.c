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
 *
 *      Location of page foreground
 *          PIX      *pixFindPageForeground()
 *
 *      Extraction of characters from image with only text
 *          l_int32   pixSplitIntoCharacters()
 *          BOXA     *pixSplitComponentWithProfile()
 */

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
        lept_mkdir("pageseg");
        ptaaWrite("/tmp/pageseg/tb_outlines.ptaa", ptaa, 1);
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
        boxaWrite("/tmp/pageseg/htmask.boxa", bahm);
        boxaWrite("/tmp/pageseg/textmask.boxa", batm);
        boxaWrite("/tmp/pageseg/textblock.boxa", batb);
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


/*------------------------------------------------------------------*
 *                    Location of page foreground                   *
 *------------------------------------------------------------------*/
/*!
 *  pixFindPageForeground()
 *
 *      Input:  pixs (full resolution (any type or depth)
 *              threshold (for binarization; typically about 128)
 *              mindist (min distance of text from border to allow
 *                       cleaning near border; at 2x reduction, this
 *                       should be larger than 50; typically about 70)
 *              erasedist (when conditions are satisfied, erase anything
 *                         within this distance of the edge;
 *                         typically 30 at 2x reduction)
 *              pagenum (use for debugging when called repeatedly; labels
 *                       debug images that are assembled into pdfdir)
 *              showmorph (set to a negative integer to show steps in
 *                         generating masks; this is typically used
 *                         for debugging region extraction)
 *              display (set to 1  to display mask and selected region
 *                       for debugging a single page)
 *              pdfdir (subdirectory of /tmp where images showing the
 *                      result are placed when called repeatedly; use
 *                      null if no output requested)
 *      Return: box (region including foreground, with some pixel noise
 *                   removed), or null if not found
 *
 *  Notes:
 *      (1) This doesn't simply crop to the fg.  It attempts to remove
 *          pixel noise and junk at the edge of the image before cropping.
 *          The input @threshold is used if pixs is not 1 bpp.
 *      (2) There are several debugging options, determined by the
 *          last 4 arguments.
 *      (3) If you want pdf output of results when called repeatedly,
 *          the pagenum arg labels the images written, which go into
 *          /tmp/<pdfdir>/<pagenum>.png.  In that case,
 *          you would clean out the /tmp directory before calling this
 *          function on each page:
 *              lept_rmdir(pdfdir);
 *              lept_mkdir(pdfdir);
 */
BOX *
pixFindPageForeground(PIX         *pixs,
                      l_int32      threshold,
                      l_int32      mindist,
                      l_int32      erasedist,
                      l_int32      pagenum,
                      l_int32      showmorph,
                      l_int32      display,
                      const char  *pdfdir)
{
char     buf[64];
l_int32  flag, nbox, intersects;
l_int32  w, h, bx, by, bw, bh, left, right, top, bottom;
PIX     *pixb, *pixb2, *pixseed, *pixsf, *pixm, *pix1, *pixg2;
BOX     *box, *boxfg, *boxin, *boxd;
BOXA    *ba1, *ba2;

    PROCNAME("pixFindPageForeground");

    if (!pixs)
        return (BOX *)ERROR_PTR("pixs not defined", procName, NULL);

        /* Binarize, downscale by 0.5, remove the noise to generate a seed,
         * and do a seedfill back from the seed into those 8-connected
         * components of the binarized image for which there was at least
         * one seed pixel.  Also clear out any components that are within
         * 10 pixels of the edge at 2x reduction. */
    flag = (showmorph) ? -1 : 0;  /* if showmorph == -1, write intermediate
                                   * images to /tmp/seq_output_1.pdf */
    pixb = pixConvertTo1(pixs, threshold);
    pixb2 = pixScale(pixb, 0.5, 0.5);
    pixseed = pixMorphSequence(pixb2, "o1.2 + c9.9 + o3.5", flag);
    pixsf = pixSeedfillBinary(NULL, pixseed, pixb2, 8);
    pixSetOrClearBorder(pixsf, 10, 10, 10, 10, PIX_SET);
    pixm = pixRemoveBorderConnComps(pixsf, 8);
    if (display) pixDisplay(pixm, 100, 100);

        /* Now, where is the main block of text?  We want to remove noise near
         * the edge of the image, but to do that, we have to be convinced that
         * (1) there is noise and (2) it is far enough from the text block
         * and close enough to the edge.  For each edge, if the block
         * is more than mindist from that edge, then clean 'erasedist'
         * pixels from the edge. */
    pix1 = pixMorphSequence(pixm, "c50.50", flag - 1);
    ba1 = pixConnComp(pix1, NULL, 8);
    ba2 = boxaSort(ba1, L_SORT_BY_AREA, L_SORT_DECREASING, NULL);
    pixGetDimensions(pix1, &w, &h, NULL);
    nbox = boxaGetCount(ba2);
    if (nbox > 1) {
        box = boxaGetBox(ba2, 0, L_CLONE);
        boxGetGeometry(box, &bx, &by, &bw, &bh);
        left = (bx > mindist) ? erasedist : 0;
        right = (w - bx - bw > mindist) ? erasedist : 0;
        top = (by > mindist) ? erasedist : 0;
        bottom = (h - by - bh > mindist) ? erasedist : 0;
        pixSetOrClearBorder(pixm, left, right, top, bottom, PIX_CLR);
        boxDestroy(&box);
    }
    pixDestroy(&pix1);
    boxaDestroy(&ba1);
    boxaDestroy(&ba2);

        /* Locate the foreground region; don't bother cropping */
    pixClipToForeground(pixm, NULL, &boxfg);

        /* Sanity check the fg region.  Make sure it's not confined
         * to a thin boundary on the left and right sides of the image,
         * in which case it is likely to be noise. */
    if (boxfg) {
        boxin = boxCreate(0.1 * w, 0, 0.8 * w, h);
        boxIntersects(boxfg, boxin, &intersects);
        if (!intersects) {
            L_INFO("found only noise on page %d\n", procName, pagenum);
            boxDestroy(&boxfg);
        }
        boxDestroy(&boxin);
    }

    boxd = NULL;
    if (!boxfg) {
        L_INFO("no fg region found for page %d\n", procName, pagenum);
    } else {
        boxAdjustSides(boxfg, boxfg, -2, 2, -2, 2);  /* tiny expansion */
        boxd = boxTransform(boxfg, 0, 0, 2.0, 2.0);

            /* Write image showing box for this page.  This is to be
             * bundled up into a pdf of all the pages, which can be
             * generated by convertFilesToPdf()  */
        if (pdfdir) {
            pixg2 = pixConvert1To4Cmap(pixb);
            pixRenderBoxArb(pixg2, boxd, 3, 255, 0, 0);
            snprintf(buf, sizeof(buf), "/tmp/%s/%05d.png", pdfdir, pagenum);
            if (display) pixDisplay(pixg2, 700, 100);
            pixWrite(buf, pixg2, IFF_PNG);
            pixDestroy(&pixg2);
        }
    }

    pixDestroy(&pixb);
    pixDestroy(&pixb2);
    pixDestroy(&pixseed);
    pixDestroy(&pixsf);
    pixDestroy(&pixm);
    boxDestroy(&boxfg);
    return boxd;
}



/*------------------------------------------------------------------*
 *         Extraction of characters from image with only text       *
 *------------------------------------------------------------------*/
/*!
 *  pixSplitIntoCharacters()
 *
 *      Input:  pixs (1 bpp, contains only deskewed text)
 *              minw (minimum component width for initial filtering; typ. 4)
 *              minh (minimum component height for initial filtering; typ. 4)
 *              &boxa (<optional return> character bounding boxes)
 *              &pixa (<optional return> character images)
 *              &pixdebug (<optional return> showing splittings)
 *
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is a simple function that attempts to find split points
 *          based on vertical pixel profiles.
 *      (2) It should be given an image that has an arbitrary number
 *          of text characters.
 *      (3) The returned pixa includes the boxes from which the
 *          (possibly split) components are extracted.
 */
l_int32
pixSplitIntoCharacters(PIX     *pixs,
                       l_int32  minw,
                       l_int32  minh,
                       BOXA   **pboxa,
                       PIXA   **ppixa,
                       PIX    **ppixdebug)
{
l_int32  ncomp, i, xoff, yoff;
BOXA   *boxa1, *boxa2, *boxat1, *boxat2, *boxad;
BOXAA  *baa;
PIX    *pix, *pix1, *pix2, *pixdb;
PIXA   *pixa1, *pixadb;

    PROCNAME("pixSplitIntoCharacters");

    if (pboxa) *pboxa = NULL;
    if (ppixa) *ppixa = NULL;
    if (ppixdebug) *ppixdebug = NULL;
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

        /* Remove the small stuff */
    pix1 = pixSelectBySize(pixs, minw, minh, 8, L_SELECT_IF_BOTH,
                           L_SELECT_IF_GT, NULL);

        /* Small vertical close for consolidation */
    pix2 = pixMorphSequence(pix1, "c1.10", 0);
    pixDestroy(&pix1);

        /* Get the 8-connected components */
    boxa1 = pixConnComp(pix2, &pixa1, 8);
    pixDestroy(&pix2);
    boxaDestroy(&boxa1);

        /* Split the components if obvious */
    ncomp = pixaGetCount(pixa1);
    boxa2 = boxaCreate(ncomp);
    pixadb = (ppixdebug) ? pixaCreate(ncomp) : NULL;
    for (i = 0; i < ncomp; i++) {
        pix = pixaGetPix(pixa1, i, L_CLONE);
        if (ppixdebug) {
            boxat1 = pixSplitComponentWithProfile(pix, 10, 7, &pixdb);
            if (pixdb)
                pixaAddPix(pixadb, pixdb, L_INSERT);
        } else {
            boxat1 = pixSplitComponentWithProfile(pix, 10, 7, NULL);
        }
        pixaGetBoxGeometry(pixa1, i, &xoff, &yoff, NULL, NULL);
        boxat2 = boxaTransform(boxat1, xoff, yoff, 1.0, 1.0);
        boxaJoin(boxa2, boxat2, 0, -1);
        pixDestroy(&pix);
        boxaDestroy(&boxat1);
        boxaDestroy(&boxat2);
    }
    pixaDestroy(&pixa1);

        /* Generate the debug image */
    if (ppixdebug) {
        if (pixaGetCount(pixadb) > 0) {
            *ppixdebug = pixaDisplayTiledInRows(pixadb, 32, 1500,
                                                1.0, 0, 20, 1);
        }
        pixaDestroy(&pixadb);
    }

        /* Do a 2D sort on the bounding boxes, and flatten the result to 1D */
    baa = boxaSort2d(boxa2, NULL, 0, 0, 5);
    boxad = boxaaFlattenToBoxa(baa, NULL, L_CLONE);
    boxaaDestroy(&baa);
    boxaDestroy(&boxa2);

        /* Optionally extract the pieces from the input image */
    if (ppixa)
        *ppixa = pixClipRectangles(pixs, boxad);
    if (pboxa)
        *pboxa = boxad;
    else
        boxaDestroy(&boxad);
    return 0;
}


/*!
 *  pixSplitComponentWithProfile()
 *
 *      Input:  pixs (1 bpp, exactly one connected component)
 *              delta (distance used in extrema finding in a numa; typ. 10)
 *              mindel (minimum required difference between profile minimum
 *                      and profile values +2 and -2 away; typ. 7)
 *              &pixdebug (<optional return> debug image of splitting)
 *      Return: boxa (of c.c. after splitting), or null on error
 *
 *  Notes:
 *      (1) This will split the most obvious cases of touching characters.
 *          The split points it is searching for are narrow and deep
 *          minimima in the vertical pixel projection profile, after a
 *          large vertical closing has been applied to the component.
 */
BOXA *
pixSplitComponentWithProfile(PIX     *pixs,
                             l_int32  delta,
                             l_int32  mindel,
                             PIX    **ppixdebug)
{
l_int32   w, h, n2, i, firstmin, xmin, xshift;
l_int32   nmin, nleft, nright, nsplit, isplit, ncomp;
l_int32  *array1, *array2;
BOX      *box;
BOXA     *boxad;
NUMA     *na1, *na2, *nasplit;
PIX      *pix1, *pixdb;

    PROCNAME("pixSplitComponentsWithProfile");

    if (ppixdebug) *ppixdebug = NULL;
    if (!pixs || pixGetDepth(pixs) != 1)
        return (BOXA *)ERROR_PTR("pixa undefined or not 1 bpp", procName, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);

        /* Closing to consolidate characters vertically */
    pix1 = pixCloseSafeBrick(NULL, pixs, 1, 100);

        /* Get extrema of column projections */
    boxad = boxaCreate(2);
    na1 = pixCountPixelsByColumn(pix1);  /* w elements */
    pixDestroy(&pix1);
    na2 = numaFindExtrema(na1, delta);
    n2 = numaGetCount(na2);
    if (n2 < 3) {  /* no split possible */
        box = boxCreate(0, 0, w, h);
        boxaAddBox(boxad, box, L_INSERT);
        numaDestroy(&na1);
        numaDestroy(&na2);
        return boxad;
    }

        /* Look for sufficiently deep and narrow minima.
         * All minima of of interest must be surrounded by max on each
         * side.  firstmin is the index of first possible minimum. */
    array1 = numaGetIArray(na1);
    array2 = numaGetIArray(na2);
    if (ppixdebug) numaWriteStream(stderr, na2);
    firstmin = (array1[array2[0]] > array1[array2[1]]) ? 1 : 2;
    nasplit = numaCreate(n2);  /* will hold split locations */
    for (i = firstmin; i < n2 - 1; i+= 2) {
        xmin = array2[i];
        nmin = array1[xmin];
        if (xmin + 2 >= w) break;  /* no more splits possible */
        nleft = array1[xmin - 2];
        nright = array1[xmin + 2];
        if (ppixdebug) {
            fprintf(stderr,
                "Splitting: xmin = %d, w = %d; nl = %d, nmin = %d, nr = %d\n",
                xmin, w, nleft, nmin, nright);
        }
        if (nleft - nmin >= mindel && nright - nmin >= mindel)  /* split */
            numaAddNumber(nasplit, xmin);
    }
    nsplit = numaGetCount(nasplit);

#if 0
    if (ppixdebug && nsplit > 0)
        gplotSimple1(na1, GPLOT_X11, "/tmp/splitroot", NULL);
#endif

    numaDestroy(&na1);
    numaDestroy(&na2);
    FREE(array1);
    FREE(array2);

    if (nsplit == 0) {  /* no splitting */
        box = boxCreate(0, 0, w, h);
        boxaAddBox(boxad, box, L_INSERT);
        return boxad;
    }

        /* Use split points to generate b.b. after splitting */
    for (i = 0, xshift = 0; i < nsplit; i++) {
        numaGetIValue(nasplit, i, &isplit);
        box = boxCreate(xshift, 0, isplit - xshift, h);
        boxaAddBox(boxad, box, L_INSERT);
        xshift = isplit + 1;
    }
    box = boxCreate(xshift, 0, w - xshift, h);
    boxaAddBox(boxad, box, L_INSERT);

    numaDestroy(&nasplit);

    if (ppixdebug) {
        pixdb = pixConvertTo32(pixs);
        ncomp = boxaGetCount(boxad);
        for (i = 0; i < ncomp; i++) {
            box = boxaGetBox(boxad, i, L_CLONE);
            pixRenderBoxBlend(pixdb, box, 1, 255, 0, 0, 0.5);
            boxDestroy(&box);
        }
        *ppixdebug = pixdb;
    }

    return boxad;
}
