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
 *
 *      Extraction of lines of text
 *          PIXA     *pixExtractTextlines()
 *
 *      Decision text vs photo
 *          l_int32   pixDecideIfText()
 *          l_int32   pixFindThreshFgExtent()
 */

#include "allheaders.h"

    /* These functions are not intended to work on very low-res images */
static const l_int32  MinWidth = 100;
static const l_int32  MinHeight = 100;

#ifndef  NO_CONSOLE_IO
#define  DEBUG_LINES     0
#endif  /* ~NO_CONSOLE_IO */

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
l_int32  w, h, htfound, tlfound;
PIX     *pixr, *pix1, *pix2;
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
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs undefined or not 1 bpp", procName, 1);
    pixGetDimensions(pixs, &w, &h, NULL);
    if (w < MinWidth || h < MinHeight) {
        L_ERROR("pix too small: w = %d, h = %d\n", procName, w, h);
        return 1;
    }

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
    pix1 = pixSeedfillBinary(NULL, pixhm, pixs, 8);
    pixOr(pixhm, pixhm, pix1);
    pixDestroy(&pix1);
    pixDisplayWriteFormat(pixhm, debug, IFF_PNG);

    pix1 = pixExpandReplicate(pixtm2, 2);
    pixtm = pixDilateBrick(NULL, pix1, 3, 3);
    pixDestroy(&pix1);
    pixDisplayWriteFormat(pixtm, debug, IFF_PNG);

    pix1 = pixExpandReplicate(pixtbf2, 2);
    pixtb = pixDilateBrick(NULL, pix1, 3, 3);
    pixDestroy(&pix1);
    pixDisplayWriteFormat(pixtb, debug, IFF_PNG);

    pixDestroy(&pixhm2);
    pixDestroy(&pixtm2);
    pixDestroy(&pixtbf2);

        /* Debug: identify objects that are neither text nor halftone image */
    if (debug) {
        pix1 = pixSubtract(NULL, pixs, pixtm);  /* remove text pixels */
        pix2 = pixSubtract(NULL, pix1, pixhm);  /* remove halftone pixels */
        pixDisplayWriteFormat(pix2, 1, IFF_PNG);
        pixDestroy(&pix1);
        pixDestroy(&pix2);
    }

        /* Debug: display textline components with random colors */
    if (debug) {
        l_int32  w, h;
        BOXA    *boxa;
        PIXA    *pixa;
        boxa = pixConnComp(pixtm, &pixa, 8);
        pixGetDimensions(pixtm, &w, &h, NULL);
        pix1 = pixaDisplayRandomCmap(pixa, w, h);
        pixcmapResetColor(pixGetColormap(pix1), 0, 255, 255, 255);
        pixDisplay(pix1, 100, 100);
        pixDisplayWriteFormat(pix1, 1, IFF_PNG);
        pixaDestroy(&pixa);
        boxaDestroy(&boxa);
        pixDestroy(&pix1);
    }

        /* Debug: identify the outlines of each textblock */
    if (debug) {
        PIXCMAP  *cmap;
        PTAA     *ptaa;
        ptaa = pixGetOuterBordersPtaa(pixtb);
        lept_mkdir("pageseg");
        ptaaWrite("/tmp/pageseg/tb_outlines.ptaa", ptaa, 1);
        pix1 = pixRenderRandomCmapPtaa(pixtb, ptaa, 1, 16, 1);
        cmap = pixGetColormap(pix1);
        pixcmapResetColor(cmap, 0, 130, 130, 130);
        pixDisplay(pix1, 500, 100);
        pixDisplayWriteFormat(pix1, 1, IFF_PNG);
        pixDestroy(&pix1);
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
 *
 *  Notes:
 *      (1) This is not intended to work on small thumbnails.  The
 *          dimensions of pixs must be at least MinWidth x MinHeight.
 */
PIX *
pixGenHalftoneMask(PIX      *pixs,
                   PIX     **ppixtext,
                   l_int32  *phtfound,
                   l_int32   debug)
{
l_int32  w, h, empty;
PIX     *pix1, *pix2, *pixhs, *pixhm, *pixd;

    PROCNAME("pixGenHalftoneMask");

    if (ppixtext) *ppixtext = NULL;
    if (phtfound) *phtfound = 0;
    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);
    if (w < MinWidth || h < MinHeight) {
        L_ERROR("pix too small: w = %d, h = %d\n", procName, w, h);
        return NULL;
    }

        /* Compute seed for halftone parts at 8x reduction */
    pix1 = pixReduceRankBinaryCascade(pixs, 4, 4, 3, 0);
    pix2 = pixOpenBrick(NULL, pix1, 5, 5);
    pixhs = pixExpandReplicate(pix2, 8);  /* back to 2x reduction */
    pixDestroy(&pix1);
    pixDestroy(&pix2);
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
    if (phtfound && !empty)
        *phtfound = 1;

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
 *      (3) This is not intended to work on small thumbnails.  The
 *          dimensions of pixs must be at least MinWidth x MinHeight.
 *      (4) Both the input image and the returned textline mask
 *          are at the same resolution.
 */
PIX *
pixGenTextlineMask(PIX      *pixs,
                   PIX     **ppixvws,
                   l_int32  *ptlfound,
                   l_int32   debug)
{
l_int32  w, h, empty;
PIX     *pix1, *pix2, *pixvws, *pixd;

    PROCNAME("pixGenTextlineMask");

    if (ptlfound) *ptlfound = 0;
    if (!ppixvws)
        return (PIX *)ERROR_PTR("&pixvws not defined", procName, NULL);
    *ppixvws = NULL;
    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);
    if (w < MinWidth || h < MinHeight) {
        L_ERROR("pix too small: w = %d, h = %d\n", procName, w, h);
        return NULL;
    }

        /* First we need a vertical whitespace mask.  Invert the image. */
    pix1 = pixInvert(NULL, pixs);

        /* The whitespace mask will break textlines where there
         * is a large amount of white space below or above.
         * This can be prevented by identifying regions of the
         * inverted image that have large horizontal extent (bigger than
         * the separation between columns) and significant
         * vertical extent (bigger than the separation between
         * textlines), and subtracting this from the bg. */
    pix2 = pixMorphCompSequence(pix1, "o80.60", 0);
    pixSubtract(pix1, pix1, pix2);
    pixDisplayWriteFormat(pix1, debug, IFF_PNG);
    pixDestroy(&pix2);

        /* Identify vertical whitespace by opening the remaining bg.
         * o5.1 removes thin vertical bg lines and o1.200 extracts
         * long vertical bg lines. */
    pixvws = pixMorphCompSequence(pix1, "o5.1 + o1.200", 0);
    *ppixvws = pixvws;
    pixDisplayWriteFormat(pixvws, debug, IFF_PNG);
    pixDestroy(&pix1);

        /* Three steps to getting text line mask:
         *   (1) close the characters and words in the textlines
         *   (2) open the vertical whitespace corridors back up
         *   (3) small opening to remove noise    */
    pix1 = pixCloseSafeBrick(NULL, pixs, 30, 1);
    pixDisplayWrite(pix1, debug);
    pixd = pixSubtract(NULL, pix1, pixvws);
    pixOpenBrick(pixd, pixd, 3, 3);
    pixDisplayWriteFormat(pixd, debug, IFF_PNG);
    pixDestroy(&pix1);

        /* Check if text line mask is empty */
    if (ptlfound) {
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
 *      (2) This is not intended to work on small thumbnails.  The
 *          dimensions of pixs must be at least MinWidth x MinHeight.
 *      (3) The result is somewhat noisy, in that small "blocks" of
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
l_int32  w, h;
PIX     *pix1, *pix2, *pix3, *pixd;

    PROCNAME("pixGenTextblockMask");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);
    if (w < MinWidth || h < MinHeight) {
        L_ERROR("pix too small: w = %d, h = %d\n", procName, w, h);
        return NULL;
    }
    if (!pixvws)
        return (PIX *)ERROR_PTR("pixvws not defined", procName, NULL);

        /* Join pixels vertically to make a textblock mask */
    pix1 = pixMorphSequence(pixs, "c1.10 + o4.1", 0);
    pixDisplayWriteFormat(pix1, debug, IFF_PNG);

        /* Solidify the textblock mask and remove noise:
         *   (1) For each cc, close the blocks and dilate slightly
         *       to form a solid mask.
         *   (2) Small horizontal closing between components.
         *   (3) Open the white space between columns, again.
         *   (4) Remove small components. */
    pix2 = pixMorphSequenceByComponent(pix1, "c30.30 + d3.3", 8, 0, 0, NULL);
    pixCloseSafeBrick(pix2, pix2, 10, 1);
    pixDisplayWriteFormat(pix2, debug, IFF_PNG);
    pix3 = pixSubtract(NULL, pix2, pixvws);
    pixDisplayWriteFormat(pix3, debug, IFF_PNG);
    pixd = pixSelectBySize(pix3, 25, 5, 8, L_SELECT_IF_BOTH,
                            L_SELECT_IF_GTE, NULL);
    pixDisplayWriteFormat(pixd, debug, IFF_PNG);

    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
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
 *      (3) This is not intended to work on small thumbnails.  The
 *          dimensions of pixs must be at least MinWidth x MinHeight.
 *      (4) If you want pdf output of results when called repeatedly,
 *          the pagenum arg labels the images written, which go into
 *          /tmp/lept/<pdfdir>/<pagenum>.png.  In that case,
 *          you would clean out the /tmp directory before calling this
 *          function on each page:
 *              lept_rmdir("/lept/<pdfdir>");
 *              lept_mkdir("/lept/<pdfdir>");
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
    pixGetDimensions(pixs, &w, &h, NULL);
    if (w < MinWidth || h < MinHeight) {
        L_ERROR("pix too small: w = %d, h = %d\n", procName, w, h);
        return NULL;
    }

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
            snprintf(buf, sizeof(buf), "lept/%s", pdfdir);
            lept_mkdir(buf);

            pixg2 = pixConvert1To4Cmap(pixb);
            pixRenderBoxArb(pixg2, boxd, 3, 255, 0, 0);
            snprintf(buf, sizeof(buf), "/tmp/lept/%s/%04d.png",
                     pdfdir, pagenum);
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
    LEPT_FREE(array1);
    LEPT_FREE(array2);

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


/*------------------------------------------------------------------*
 *                    Extraction of lines of text                   *
 *------------------------------------------------------------------*/
/*!
 *  pixExtractTextlines()
 *
 *      Input:  pixs (any depth, assumed to have nearly horizontal text)
 *              maxw, maxh (initial filtering: remove any components in pixs
 *                          with components larger than maxw or maxh)
 *              minw, minh (final filtering: remove extracted 'lines'
 *                          with sizes smaller than minw or minh)
 *      Return: pixa (of textline images, including bounding boxes), or
 *                    null on error
 *
 *  Notes:
 *      (1) This first removes components from pixs that are either
 *          wide (> @maxw) or tall (> @maxh).
 *      (2) This function assumes that textlines have sufficient
 *          vertical separation and small enough skew so that a
 *          horizontal dilation sufficient to join words will not join
 *          textlines.  Images with multiple columns of text may have
 *          the textlines join across the space between columns.
 *      (3) A final filtering operation removes small components, such
 *          that width < @minw or height < @minh.
 *      (4) For reasonable accuracy, the resolution of pixs should be
 *          at least 100 ppi.  For reasonable efficiency, the resolution
 *          should not exceed 600 ppi.
 *      (5) This can be used to determine if some region of a scanned
 *          image is horizontal text.
 *      (6) As an example, for a pix with resolution 300 ppi, a reasonable
 *          set of parameters is:
 *             pixExtractTextlines(pix, 150, 150, 10, 5);
 */
PIXA *
pixExtractTextlines(PIX     *pixs,
                    l_int32  maxw,
                    l_int32  maxh,
                    l_int32  minw,
                    l_int32  minh)
{
char     buf[64];
l_int32  i, n, res, csize, empty;
BOX     *box;
BOXA    *boxa1, *boxa2;
PIX     *pix1, *pix2, *pix3, *pix4, *pix5;
PIXA    *pixa1, *pixa2, *pixa3, *pixad;

    PROCNAME("pixExtractTextlines");

    if (!pixs)
        return (PIXA *)ERROR_PTR("pixs not defined", procName, NULL);

        /* Binarize carefully, if necessary */
    if (pixGetDepth(pixs) > 1) {
        pix2 = pixConvertTo8(pixs, FALSE);
        pix3 = pixCleanBackgroundToWhite(pix2, NULL, NULL, 1.0, 70, 190);
        pix1 = pixThresholdToBinary(pix3, 150);
        pixDestroy(&pix3);
        pixDestroy(&pix3);
    } else {
        pix1 = pixClone(pixs);
    }
    pixZero(pix1, &empty);
    if (empty) {
        pixDestroy(&pix1);
        L_INFO("no fg pixels in input image\n", procName);
        return NULL;
    }

        /* Remove any very tall or very wide connected components */
    pix2 = pixSelectBySize(pix1, maxw, maxh, 8, L_SELECT_IF_BOTH,
                           L_SELECT_IF_LT, NULL);
    pixDestroy(&pix1);

        /* Filter to solidify the text lines within the x-height region.
         * The closing (csize) bridges gaps between words.  The opening
         * removes isolated bridges between textlines. */
    if ((res = pixGetXRes(pixs)) == 0) {
        L_INFO("Resolution is not set: setting to 300 ppi\n", procName);
        res = 300;
    }
    csize = L_MIN(120., 60.0 * (res / 300));
    snprintf(buf, sizeof(buf), "c%d.1 + o20.1", csize);
    pix3 = pixMorphCompSequence(pix2, buf, 0);

        /* Extract the connected components.  These should be dilated lines */
    boxa1 = pixConnComp(pix3, &pixa1, 4);
    pixDestroy(&pix3);

        /* Remove line components that are too small */
    pixa2 = pixaSelectBySize(pixa1, minw, minh, L_SELECT_IF_BOTH,
                           L_SELECT_IF_GTE, NULL);

#if DEBUG_LINES
    pix1 = pixaDisplayRandomCmap(pixa2, 0, 0);
    pixcmapResetColor(pixGetColormap(pix1), 0, 255, 255, 255);
    pixWrite("/tmp/lept/junklines.png", pix1, IFF_PNG);
    pixDestroy(&pix1);
#endif

        /* Selectively AND with the version before dilation, and save */
    boxa2 = pixaGetBoxa(pixa2, L_CLONE);
    n = boxaGetCount(boxa2);
    pixa3 = pixClipRectangles(pix2, boxa2);
    pixad = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pix4 = pixaGetPix(pixa2, i, L_CLONE);
        pix5 = pixaGetPix(pixa3, i, L_COPY);
        pixAnd(pix5, pix5, pix4);
        pixaAddPix(pixad, pix5, L_INSERT);
        box = boxaGetBox(boxa2, i, L_COPY);
        pixaAddBox(pixad, box, L_INSERT);
        pixDestroy(&pix4);
    }

    pixDestroy(&pix2);
    pixaDestroy(&pixa1);
    pixaDestroy(&pixa2);
    pixaDestroy(&pixa3);
    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
    return pixad;
}


/*------------------------------------------------------------------*
 *                      Decision text vs photo                      *
 *------------------------------------------------------------------*/
/*!
 *  pixDecideIfText()
 *
 *      Input:  pixs (any depth)
 *              box (<optional> if null, use entire pixs)
 *              &istext (<return> 1 if text; 0 if photo; -1 if not determined)
 *              pixadb (<optional> pre-allocated, for showing intermediate
 *                      computation; use null to skip)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) It is assumed that pixs has the correct resolution set.
 *          If the resolution is 0, we set to 300 and issue a warning.
 *      (2) If necessary, the image is scaled to 300 ppi; most of the
 *          processing is done at this resolution.
 *      (3) Text is assumed to be in horizontal lines.
 *      (4) Because thin vertical lines are removed before filtering for
 *          text lines, this should identify tables as text.
 *      (5) If @box is null and pixs contains both text lines and line art,
 *          this function might return @istext == true.
 *      (6) If the input pixs is empty, or for some other reason the
 *          result can not be determined, return -1.
 *      (7) For debug output, input a pre-allocated pixa.
 */
l_int32
pixDecideIfText(PIX      *pixs,
                BOX      *box,
                l_int32  *pistext,
                PIXA     *pixadb)
{
l_int32    i, empty, maxw, maxh, w, h, n1, n2, n3, minlines;
l_int32    res, big_comp;
l_float32  ratio1, ratio2, factor;
L_BMF     *bmf;
BOX       *box1;
BOXA      *boxa1, *boxa2, *boxa3, *boxa4, *boxa5;
PIX       *pix1, *pix2, *pix3, *pix4, *pix5, *pix5a;
PIX       *pix6, *pix7, *pix8, *pix9, *pix10;
PIXA      *pixa1, *pixa2;
SEL       *sel1;

    PROCNAME("pixDecideIfText");

    if (pistext) *pistext = -1;  /* init */
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

        /* Crop and convert to 1 bpp with adaptive background cleaning.
         * If no box is given, use most of the image.  Removing the
         * edges helps avoid false negatives from noise near the edges. */
    if (box) {
        pix1 = pixClipRectangle(pixs, box, NULL);
    } else {
        pixGetDimensions(pixs, &w, &h, NULL);
        box1 = boxCreate(w / 10, h / 10, 4 * w / 5, 4 * h / 5);
        pix1 = pixClipRectangle(pixs, box1, NULL);
        boxDestroy(&box1);
    }
    pix2 = pixConvertTo8(pix1, 0);
    pix3 = pixCleanBackgroundToWhite(pix2, NULL, NULL, 1.0, 70, 160);
    pixDestroy(&pix1);
    if (!pix3) {
        pixDestroy(&pix2);
        L_INFO("pix cleaning failed\n", procName);
        return 1;
    }
    pix4 = pixThresholdToBinary(pix3, 200);
    pixZero(pix4, &empty);
    if (empty) {
        pixDestroy(&pix2);
        pixDestroy(&pix3);
        pixDestroy(&pix4);
        L_INFO("pix is empty\n", procName);
        return 0;
    }

        /* Get the resolution, or guess, and scale the image to 300 ppi */
    if ((res = pixGetXRes(pixs)) == 0) {
        L_WARNING("Resolution is not set: using 300 ppi\n", procName);
        res = 300;
    }
    if (res != 300) {
        factor = 300. / res;
        pix5 = pixScale(pix4, factor, factor);
    } else {
        pix5 = pixClone(pix4);
    }
    w = pixGetWidth(pix5);

        /* Identify and remove tall, thin vertical lines (as found in tables)
         * that are up to 9 pixels wide.  Make a hit-miss sel with an
         * 81 pixel vertical set of hits and with 3 pairs of misses that
         * are 10 pixels apart horizontally.  It is necessary to use a
         * hit-miss transform; if we only opened with a vertical line of
         * hits, we would remove solid regions of pixels that are not
         * text or vertical lines. */
    pix5a = pixCreate(11, 81, 1);
    for (i = 0; i < 81; i++)
        pixSetPixel(pix5a, 5, i, 1);
    sel1 = selCreateFromPix(pix5a, 40, 5, NULL);
    selSetElement(sel1, 20, 0, SEL_MISS);
    selSetElement(sel1, 20, 10, SEL_MISS);
    selSetElement(sel1, 40, 0, SEL_MISS);
    selSetElement(sel1, 40, 10, SEL_MISS);
    selSetElement(sel1, 60, 0, SEL_MISS);
    selSetElement(sel1, 60, 10, SEL_MISS);
    pix6 = pixHMT(NULL, pix5, sel1);
    pix7 = pixSeedfillBinaryRestricted(NULL, pix6, pix5, 8, 5, 1000);
    pix8 = pixXor(NULL, pix5, pix7);
    pixDestroy(&pix5a);
    selDestroy(&sel1);

        /* Convert the text lines to separate long horizontal components */
    pix9 = pixMorphCompSequence(pix8, "c30.1 + o15.1 + c60.1 + o2.2", 0);

        /* Estimate the distance to the bottom of the significant region */
    if (box) {  /* use full height */
        pixGetDimensions(pix9, NULL, &h, NULL);
    } else {  /* use height of region that has text lines */
        pixFindThreshFgExtent(pix9, 400, NULL, &h);
    }

    if (pixadb) {
        bmf = bmfCreate(NULL, 8);
        pixaAddPixWithText(pixadb, pix2, 1, bmf, "initial 8 bpp",
                           0x0000ff00, L_ADD_BELOW);
        pixaAddPixWithText(pixadb, pix3, 1, bmf, "with background cleaning",
                           0x0000ff00, L_ADD_BELOW);
        pixaAddPixWithText(pixadb, pix4, 1, bmf, "threshold to binary",
                           0x0000ff00, L_ADD_BELOW);
        pixaAddPixWithText(pixadb, pix6, 2, bmf, "hit-miss for vertical line",
                           0x0000ff00, L_ADD_BELOW);
        pixaAddPixWithText(pixadb, pix7, 2, bmf, "restricted seed-fill",
                           0x0000ff00, L_ADD_BELOW);
        pixaAddPixWithText(pixadb, pix8, 2, bmf, "remove using xor",
                           0x0000ff00, L_ADD_BELOW);
        pixaAddPixWithText(pixadb, pix9, 2, bmf, "make long horiz components",
                           0x0000ff00, L_ADD_BELOW);
    }

        /* Extract the connected components */
    if (pixadb) {
        boxa1 = pixConnComp(pix9, &pixa1, 8);
        pix10 = pixaDisplayRandomCmap(pixa1, 0, 0);
        pixcmapResetColor(pixGetColormap(pix10), 0, 255, 255, 255);
        pixaAddPixWithText(pixadb, pix10, 2, bmf, "show connected components",
                           0x0000ff00, L_ADD_BELOW);
        pixDestroy(&pix10);
        pixaDestroy(&pixa1);
        bmfDestroy(&bmf);
    } else {
        boxa1 = pixConnComp(pix9, NULL, 8);
    }

        /* Analyze the connected components.  The following conditions
         * at 300 ppi must be satisfied if the image is text:
         * (1) There are no components that are wider than 400 pixels and
         *     taller than 175 pixels.
         * (2) The second longest component is at least 60% of the
         *     (possibly cropped) image width.  This catches images
         *     that don't have any significant content.
         * (3) Of the components that are at least 40% of the length
         *     of the longest (n2), at least 80% of them must not exceed
         *     60 pixels in height.
         * (4) The number of those long, thin components (n3) must
         *     equal or exceed a minimum that scales linearly with the
         *     image height.
         * Most images that are not text fail more than one of these
         * conditions. */
    boxa2 = boxaSort(boxa1, L_SORT_BY_WIDTH, L_SORT_DECREASING, NULL);
    boxaGetBoxGeometry(boxa2, 1, NULL, NULL, &maxw, NULL);  /* 2nd longest */
    boxa3 = boxaSelectBySize(boxa1, 0.4 * maxw, 0, L_SELECT_WIDTH,
                             L_SELECT_IF_GTE, NULL);
    boxa4 = boxaSelectBySize(boxa3, 0, 60, L_SELECT_HEIGHT,
                             L_SELECT_IF_LTE, NULL);
    boxa5 = boxaSelectBySize(boxa1, 400, 175, L_SELECT_IF_BOTH,
                             L_SELECT_IF_GT, NULL);
    big_comp = (boxaGetCount(boxa5) == 0) ? 0 : 1;
    n1 = boxaGetCount(boxa1);
    n2 = boxaGetCount(boxa3);
    n3 = boxaGetCount(boxa4);
    ratio1 = (l_float32)maxw / (l_float32)w;
    ratio2 = (l_float32)n3 / (l_float32)n2;
    minlines = L_MAX(2, h / 125);
    if (big_comp || ratio1 < 0.6 || ratio2 < 0.8 || n3 < minlines)
        *pistext = 0;
    else
        *pistext = 1;
    if (pixadb) {
        if (*pistext == 1) {
            L_INFO("This is text: \n  n1 = %d, n2 = %d, n3 = %d, "
                   "minlines = %d\n  maxw = %d, ratio1 = %4.2f, h = %d, "
                   "big_comp = %d\n", procName, n1, n2, n3, minlines,
                   maxw, ratio1, h, big_comp);
        } else {
            L_INFO("This is not text: \n  n1 = %d, n2 = %d, n3 = %d, "
                   "minlines = %d\n  maxw = %d, ratio1 = %4.2f, h = %d, "
                   "big_comp = %d\n", procName, n1, n2, n3, minlines,
                   maxw, ratio1, h, big_comp);
        }
    }

    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
    boxaDestroy(&boxa3);
    boxaDestroy(&boxa4);
    boxaDestroy(&boxa5);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    pixDestroy(&pix4);
    pixDestroy(&pix5);
    pixDestroy(&pix6);
    pixDestroy(&pix7);
    pixDestroy(&pix8);
    pixDestroy(&pix9);
    return 0;
}


/*!
 *  pixFindThreshFgExtent()
 *
 *      Input:  pixs (1 bpp)
 *              thresh (threshold number of pixels in row)
 *              &top (<optional return> location of top of region)
 *              &bot (<optional return> location of bottom of region)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixFindThreshFgExtent(PIX      *pixs,
                      l_int32   thresh,
                      l_int32  *ptop,
                      l_int32  *pbot)
{
l_int32    i, n, res;
l_int32   *array;
l_float32  factor;
NUMA      *na;

    PROCNAME("pixFindThreshFgExtent");

    if (ptop) *ptop = 0;
    if (pbot) *pbot = 0;
    if (!ptop && !pbot)
        return ERROR_INT("nothing to determine", procName, 1);
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

    na = pixCountPixelsByRow(pixs, NULL);
    n = numaGetCount(na);
    array = numaGetIArray(na);
    if (ptop) {
        for (i = 0; i < n; i++) {
            if (array[i] >= thresh) {
                *ptop = i;
                break;
            }
        }
    }
    if (pbot) {
        for (i = n - 1; i >= 0; i--) {
            if (array[i] >= thresh) {
                *pbot = i;
                break;
            }
        }
    }
    LEPT_FREE(array);
    numaDestroy(&na);
    return 0;
}

