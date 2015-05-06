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
 *  readbarcode.c
 *
 *      Basic operations to locate and identify the line widths
 *      in 1D barcodes.
 *
 *      Top level
 *          SARRAY          *pixProcessBarcodes()
 *
 *      Next levels
 *          PIXA            *pixExtractBarcodes()
 *          SARRAY          *pixReadBarcodes()
 *          l_int32          pixReadBarcodeWidths()
 *
 *      Location
 *          BOXA            *pixLocateBarcodes()
 *          static PIX      *pixGenerateBarcodeMask()
 *
 *      Extraction and deskew
 *          PIXA            *pixDeskewBarcodes()
 *
 *      Process to get line widths
 *          NUMA            *pixExtractBarcodeWidths1()
 *          NUMA            *pixExtractBarcodeWidths2()
 *          NUMA            *pixExtractBarcodeCrossings()
 *
 *      Average adjacent rasters
 *          static NUMA     *pixAverageRasterScans()
 *
 *      Signal processing for barcode widths
 *          NUMA            *numaQuantizeCrossingsByWidth()
 *          static l_int32   numaGetCrossingDistances()
 *          static NUMA     *numaLocatePeakRanges()
 *          static NUMA     *numaGetPeakCentroids()
 *          static NUMA     *numaGetPeakWidthLUT()
 *          NUMA            *numaQuantizeCrossingsByWindow()
 *          static l_int32   numaEvalBestWidthAndShift()
 *          static l_int32   numaEvalSyncError()
 *
 *
 *  NOTE CAREFULLY: This is "early beta" code.  It has not been tuned
 *  to work robustly on a large database of barcode images.  I'm putting
 *  it out so that people can play with it, find out how it breaks, and
 *  contribute decoders for other barcode formats.  Both the functional
 *  interfaces and ABI will almost certainly change in the coming
 *  few months.  The actual decoder, in bardecode.c, at present only
 *  works on the following codes: Code I2of5, Code 2of5, Code 39, Code 93
 *  Codabar and UPCA.  To add another barcode format, it is necessary
 *  to make changes in readbarcode.h and bardecode.c.
 *  The program prog/barcodetest shows how to run from the top level
 *  (image --> decoded data).
 */

#include <string.h>
#include "allheaders.h"
#include "readbarcode.h"

    /* Parameters for pixGenerateBarcodeMask() */
static const l_int32  MAX_SPACE_WIDTH = 19;  /* was 15 */
static const l_int32  MAX_NOISE_WIDTH = 50;  /* smaller than barcode width */
static const l_int32  MAX_NOISE_HEIGHT = 30;  /* smaller than barcode height */

    /* Static functions */
static PIX *pixGenerateBarcodeMask(PIX *pixs, l_int32 maxspace,
                                   l_int32 nwidth, l_int32 nheight);
static NUMA *pixAverageRasterScans(PIX *pixs, l_int32 nscans);
static l_int32 numaGetCrossingDistances(NUMA *nas, NUMA **pnaedist,
                                        NUMA **pnaodist, l_float32 *pmindist,
                                        l_float32 *pmaxdist);
static NUMA *numaLocatePeakRanges(NUMA *nas, l_float32 minfirst,
                                  l_float32 minsep, l_float32 maxmin);
static NUMA *numaGetPeakCentroids(NUMA *nahist, NUMA *narange);
static NUMA *numaGetPeakWidthLUT(NUMA *narange, NUMA *nacent);
static l_int32 numaEvalBestWidthAndShift(NUMA *nas, l_int32 nwidth,
                                         l_int32 nshift, l_float32 minwidth,
                                         l_float32 maxwidth,
                                         l_float32 *pbestwidth,
                                         l_float32 *pbestshift,
                                         l_float32 *pbestscore);
static l_int32 numaEvalSyncError(NUMA *nas, l_int32 ifirst, l_int32 ilast,
                                 l_float32 width, l_float32 shift,
                                 l_float32 *pscore, NUMA **pnad);


#ifndef  NO_CONSOLE_IO
#define  DEBUG_DESKEW     1
#define  DEBUG_WIDTHS     0
#endif  /* ~NO_CONSOLE_IO */


/*------------------------------------------------------------------------*
 *                               Top level                                *
 *------------------------------------------------------------------------*/
/*!
 *  pixProcessBarcodes()
 *
 *      Input:  pixs (any depth)
 *              format (L_BF_ANY, L_BF_CODEI2OF5, L_BF_CODE93, ...)
 *              method (L_USE_WIDTHS, L_USE_WINDOWS)
 *              &saw (<optional return> sarray of bar widths)
 *              debugflag (use 1 to generate debug output)
 *      Return: sarray (text of barcodes), or null if none found or on error
 */
SARRAY *
pixProcessBarcodes(PIX      *pixs,
                   l_int32   format,
                   l_int32   method,
                   SARRAY  **psaw,
                   l_int32   debugflag)
{
PIX     *pixg;
PIXA    *pixa;
SARRAY  *sad;

    PROCNAME("pixProcessBarcodes");

    if (psaw) *psaw = NULL;
    if (!pixs)
        return (SARRAY *)ERROR_PTR("pixs not defined", procName, NULL);
    if (format != L_BF_ANY && !barcodeFormatIsSupported(format))
        return (SARRAY *)ERROR_PTR("unsupported format", procName, NULL);
    if (method != L_USE_WIDTHS && method != L_USE_WINDOWS)
        return (SARRAY *)ERROR_PTR("invalid method", procName, NULL);

        /* Get an 8 bpp image, no cmap */
    if (pixGetDepth(pixs) == 8 && !pixGetColormap(pixs))
        pixg = pixClone(pixs);
    else
        pixg = pixConvertTo8(pixs, 0);

    if ((pixa = pixExtractBarcodes(pixg, debugflag)) == NULL) {
        pixDestroy(&pixg);
        return (SARRAY *)ERROR_PTR("no barcode(s) found", procName, NULL);
    }

    sad = pixReadBarcodes(pixa, format, method, psaw, debugflag);

    pixDestroy(&pixg);
    pixaDestroy(&pixa);
    return sad;
}


/*!
 *  pixExtractBarcodes()
 *
 *      Input:  pixs (8 bpp, no colormap)
 *              debugflag (use 1 to generate debug output)
 *      Return: pixa (deskewed and cropped barcodes), or null if
 *                    none found or on error
 */
PIXA *
pixExtractBarcodes(PIX     *pixs,
                   l_int32  debugflag)
{
l_int32    i, n;
l_float32  angle, conf;
BOX       *box;
BOXA      *boxa;
PIX       *pixb, *pixm, *pixt;
PIXA      *pixa;

    PROCNAME("pixExtractBarcodes");

    if (!pixs || pixGetDepth(pixs) != 8 || pixGetColormap(pixs))
        return (PIXA *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);

        /* Locate them; use small threshold for edges. */
    boxa = pixLocateBarcodes(pixs, 20, &pixb, &pixm);
    n = boxaGetCount(boxa);
    L_INFO("%d possible barcode(s) found\n", procName, n);
    if (n == 0) {
        boxaDestroy(&boxa);
        pixDestroy(&pixb);
        pixDestroy(&pixm);
        return NULL;
    }

    if (debugflag) {
        boxaWriteStream(stderr, boxa);
        pixDisplay(pixb, 100, 100);
        pixDisplay(pixm, 800, 100);
    }

        /* Deskew each barcode individually */
    pixa = pixaCreate(n);
    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        pixt = pixDeskewBarcode(pixs, pixb, box, 15, 20, &angle, &conf);
        L_INFO("angle = %6.2f, conf = %6.2f\n", procName, angle, conf);
        if (conf > 5.0) {
            pixaAddPix(pixa, pixt, L_INSERT);
            pixaAddBox(pixa, box, L_INSERT);
        } else {
            pixDestroy(&pixt);
            boxDestroy(&box);
        }
    }

#if  DEBUG_DESKEW
    pixt = pixaDisplayTiledInRows(pixa, 8, 1000, 1.0, 0, 30, 2);
    pixWrite("junkpixt", pixt, IFF_PNG);
    pixDestroy(&pixt);
#endif  /* DEBUG_DESKEW */

    pixDestroy(&pixb);
    pixDestroy(&pixm);
    boxaDestroy(&boxa);
    return pixa;
}


/*!
 *  pixReadBarcodes()
 *
 *      Input:  pixa (of 8 bpp deskewed and cropped barcodes)
 *              format (L_BF_ANY, L_BF_CODEI2OF5, L_BF_CODE93, ...)
 *              method (L_USE_WIDTHS, L_USE_WINDOWS);
 *              &saw (<optional return> sarray of bar widths)
 *              debugflag (use 1 to generate debug output)
 *      Return: sa (sarray of widths, one string for each barcode found),
 *                  or null on error
 */
SARRAY *
pixReadBarcodes(PIXA     *pixa,
                l_int32   format,
                l_int32   method,
                SARRAY  **psaw,
                l_int32   debugflag)
{
char      *barstr, *data;
char       emptystring[] = "";
l_int32    i, j, n, nbars, ival;
NUMA      *na;
PIX       *pixt;
SARRAY    *saw, *sad;

    PROCNAME("pixReadBarcodes");

    if (psaw) *psaw = NULL;
    if (!pixa)
        return (SARRAY *)ERROR_PTR("pixa not defined", procName, NULL);
    if (format != L_BF_ANY && !barcodeFormatIsSupported(format))
        return (SARRAY *)ERROR_PTR("unsupported format", procName, NULL);
    if (method != L_USE_WIDTHS && method != L_USE_WINDOWS)
        return (SARRAY *)ERROR_PTR("invalid method", procName, NULL);

    n = pixaGetCount(pixa);
    saw = sarrayCreate(n);
    sad = sarrayCreate(n);
    for (i = 0; i < n; i++) {
            /* Extract the widths of the lines in each barcode */
        pixt = pixaGetPix(pixa, i, L_CLONE);
        na = pixReadBarcodeWidths(pixt, method, debugflag);
        pixDestroy(&pixt);
        if (!na) {
            ERROR_INT("valid barcode widths not returned", procName, 1);
            continue;
        }

            /* Save the widths as a string */
        nbars = numaGetCount(na);
        barstr = (char *)CALLOC(nbars + 1, sizeof(char));
        for (j = 0; j < nbars; j++) {
            numaGetIValue(na, j, &ival);
            barstr[j] = 0x30 + ival;
        }
        sarrayAddString(saw, barstr, L_INSERT);
        numaDestroy(&na);

            /* Decode the width strings */
        data = barcodeDispatchDecoder(barstr, format, debugflag);
        if (!data) {
            ERROR_INT("barcode not decoded", procName, 1);
            sarrayAddString(sad, emptystring, L_COPY);
            continue;
        }
        sarrayAddString(sad, data, L_INSERT);
    }

        /* If nothing found, clean up */
    if (sarrayGetCount(saw) == 0) {
        sarrayDestroy(&saw);
        sarrayDestroy(&sad);
        return (SARRAY *)ERROR_PTR("no valid barcode data", procName, NULL);
    }

    if (psaw)
        *psaw = saw;
    else
        sarrayDestroy(&saw);

    return sad;
}


/*!
 *  pixReadBarcodeWidths()
 *
 *      Input:  pixs (of 8 bpp deskewed and cropped barcode)
 *              method (L_USE_WIDTHS, L_USE_WINDOWS);
 *              debugflag (use 1 to generate debug output)
 *      Return: na (numa of widths (each in set {1,2,3,4}), or null on error
 */
NUMA *
pixReadBarcodeWidths(PIX     *pixs,
                     l_int32  method,
                     l_int32  debugflag)
{
l_float32  winwidth;
NUMA      *na;

    PROCNAME("pixReadBarcodeWidths");

    if (!pixs)
        return (NUMA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (NUMA *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (method != L_USE_WIDTHS && method != L_USE_WINDOWS)
        return (NUMA *)ERROR_PTR("invalid method", procName, NULL);

        /* Extract the widths of the lines in each barcode */
    if (method == L_USE_WIDTHS)
        na = pixExtractBarcodeWidths1(pixs, 120, 0.25, NULL, NULL,
                                      debugflag);
    else  /* method == L_USE_WINDOWS */
        na = pixExtractBarcodeWidths2(pixs, 120, &winwidth,
                                      NULL, debugflag);
#if  DEBUG_WIDTHS
        if (method == L_USE_WINDOWS)
            fprintf(stderr, "Window width for barcode: %7.3f\n", winwidth);
        numaWriteStream(stderr, na);
#endif  /* DEBUG_WIDTHS */

    if (!na)
        return (NUMA *)ERROR_PTR("barcode widths invalid", procName, NULL);

    return na;
}


/*------------------------------------------------------------------------*
 *                        Locate barcode in image                         *
 *------------------------------------------------------------------------*/
/*!
 *  pixLocateBarcodes()
 *
 *      Input:  pixs (any depth)
 *              thresh (for binarization of edge filter output; typ. 20)
 *              &pixb (<optional return> binarized edge filtered input image)
 *              &pixm (<optional return> mask over barcodes)
 *      Return: boxa (location of barcodes), or null if none found or on error
 */
BOXA *
pixLocateBarcodes(PIX     *pixs,
                  l_int32  thresh,
                  PIX    **ppixb,
                  PIX    **ppixm)
{
BOXA  *boxa;
PIX   *pix8, *pixe, *pixb, *pixm;

    PROCNAME("pixLocateBarcodes");

    if (!pixs)
        return (BOXA *)ERROR_PTR("pixs not defined", procName, NULL);

        /* Get an 8 bpp image, no cmap */
    if (pixGetDepth(pixs) == 8 && !pixGetColormap(pixs))
        pix8 = pixClone(pixs);
    else
        pix8 = pixConvertTo8(pixs, 0);

        /* Get a 1 bpp image of the edges */
    pixe = pixSobelEdgeFilter(pix8, L_ALL_EDGES);
    pixb = pixThresholdToBinary(pixe, thresh);
    pixInvert(pixb, pixb);
    pixDestroy(&pix8);
    pixDestroy(&pixe);

    pixm = pixGenerateBarcodeMask(pixb, MAX_SPACE_WIDTH, MAX_NOISE_WIDTH,
                                  MAX_NOISE_HEIGHT);
    boxa = pixConnComp(pixm, NULL, 8);

    if (ppixb)
        *ppixb = pixb;
    else
        pixDestroy(&pixb);
    if (ppixm)
        *ppixm = pixm;
    else
        pixDestroy(&pixm);

    return boxa;
}


/*!
 *  pixGenerateBarcodeMask()
 *
 *      Input:  pixs (1 bpp)
 *              maxspace (largest space in the barcode, in pixels)
 *              nwidth (opening 'width' to remove noise)
 *              nheight (opening 'height' to remove noise)
 *      Return: pixm (mask over barcodes), or null if none found or on error
 *
 *  Notes:
 *      (1) For noise removal, 'width' and 'height' are referred to the
 *          barcode orientation.
 *      (2) If there is skew, the mask will not cover the barcode corners.
 */
static PIX *
pixGenerateBarcodeMask(PIX     *pixs,
                       l_int32  maxspace,
                       l_int32  nwidth,
                       l_int32  nheight)
{
PIX  *pixt1, *pixt2, *pixd;

    PROCNAME("pixGenerateBarcodeMask");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

        /* Identify horizontal barcodes */
    pixt1 = pixCloseBrick(NULL, pixs, maxspace + 1, 1);
    pixt2 = pixOpenBrick(NULL, pixs, maxspace + 1, 1);
    pixXor(pixt2, pixt2, pixt1);
    pixOpenBrick(pixt2, pixt2, nwidth, nheight);
    pixDestroy(&pixt1);

        /* Identify vertical barcodes */
    pixt1 = pixCloseBrick(NULL, pixs, 1, maxspace + 1);
    pixd = pixOpenBrick(NULL, pixs, 1, maxspace + 1);
    pixXor(pixd, pixd, pixt1);
    pixOpenBrick(pixd, pixd, nheight, nwidth);
    pixDestroy(&pixt1);

        /* Combine to get all barcodes */
    pixOr(pixd, pixd, pixt2);
    pixDestroy(&pixt2);

    return pixd;
}


/*------------------------------------------------------------------------*
 *                        Extract and deskew barcode                      *
 *------------------------------------------------------------------------*/
/*!
 *  pixDeskewBarcode()
 *
 *      Input:  pixs (input image; 8 bpp)
 *              pixb (binarized edge-filtered input image)
 *              box (identified region containing barcode)
 *              margin (of extra pixels around box to extract)
 *              threshold (for binarization; ~20)
 *              &angle (<optional return> in degrees, clockwise is positive)
 *              &conf (<optional return> confidence)
 *      Return: pixd (deskewed barcode), or null on error
 *
 *  Note:
 *     (1) The (optional) angle returned is the angle in degrees (cw positive)
 *         necessary to rotate the image so that it is deskewed.
 */
PIX *
pixDeskewBarcode(PIX        *pixs,
                 PIX        *pixb,
                 BOX        *box,
                 l_int32     margin,
                 l_int32     threshold,
                 l_float32  *pangle,
                 l_float32  *pconf)
{
l_int32    x, y, w, h, n;
l_float32  angle, angle1, angle2, conf, conf1, conf2, score1, score2, deg2rad;
BOX       *boxe, *boxt;
BOXA      *boxa, *boxat;
PIX       *pixt1, *pixt2, *pixt3, *pixt4, *pixt5, *pixt6, *pixd;

    PROCNAME("pixDeskewBarcode");

    if (!pixs || pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);
    if (!pixb || pixGetDepth(pixb) != 1)
        return (PIX *)ERROR_PTR("pixb undefined or not 1 bpp", procName, NULL);
    if (!box)
        return (PIX *)ERROR_PTR("box not defined or 1 bpp", procName, NULL);

        /* Clip out */
    deg2rad = 3.1415926535 / 180.;
    boxGetGeometry(box, &x, &y, &w, &h);
    boxe = boxCreate(x - 25, y - 25, w + 51, h + 51);
    pixt1 = pixClipRectangle(pixb, boxe, NULL);
    pixt2 = pixClipRectangle(pixs, boxe, NULL);
    boxDestroy(&boxe);

        /* Deskew, looking at all possible orientations over 180 degrees */
    pixt3 = pixRotateOrth(pixt1, 1);  /* look for vertical bar lines */
    pixt4 = pixClone(pixt1);   /* look for horizontal bar lines */
    pixFindSkewSweepAndSearchScore(pixt3, &angle1, &conf1, &score1,
                                   1, 1, 0.0, 45.0, 2.5, 0.01);
    pixFindSkewSweepAndSearchScore(pixt4, &angle2, &conf2, &score2,
                                   1, 1, 0.0, 45.0, 2.5, 0.01);

        /* Because we're using the boundary pixels of the barcodes,
         * the peak can be sharper (and the confidence ratio higher)
         * from the signal across the top and bottom of the barcode.
         * However, the max score, which is the magnitude of the signal
         * at the optimum skew angle, will be smaller, so we use the
         * max score as the primary indicator of orientation. */
    if (score1 >= score2) {
        conf = conf1;
        if (conf1 > 6.0 && L_ABS(angle1) > 0.1) {
            angle = angle1;
            pixt5 = pixRotate(pixt2, deg2rad * angle1, L_ROTATE_AREA_MAP,
                              L_BRING_IN_WHITE, 0, 0);
        } else {
            angle = 0.0;
            pixt5 = pixClone(pixt2);
        }
    } else {  /* score2 > score1 */
        conf = conf2;
        pixt6 = pixRotateOrth(pixt2, 1);
        if (conf2 > 6.0 && L_ABS(angle2) > 0.1) {
            angle = 90.0 + angle2;
            pixt5 = pixRotate(pixt6, deg2rad * angle2, L_ROTATE_AREA_MAP,
                              L_BRING_IN_WHITE, 0, 0);
        } else {
            angle = 90.0;
            pixt5 = pixClone(pixt6);
        }
        pixDestroy(&pixt6);
    }
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);

        /* Extract barcode plus a margin around it */
    boxa = pixLocateBarcodes(pixt5, threshold, 0, 0);
    if ((n = boxaGetCount(boxa)) != 1) {
        L_WARNING("barcode mask in %d components\n", procName, n);
        boxat = boxaSort(boxa, L_SORT_BY_AREA, L_SORT_DECREASING, NULL);
    } else {
        boxat = boxaCopy(boxa, L_CLONE);
    }
    boxt = boxaGetBox(boxat, 0, L_CLONE);
    boxGetGeometry(boxt, &x, &y, &w, &h);
    boxe = boxCreate(x - margin, y - margin, w + 2 * margin,
                     h + 2 * margin);
    pixd = pixClipRectangle(pixt5, boxe, NULL);
    boxDestroy(&boxt);
    boxDestroy(&boxe);
    boxaDestroy(&boxa);
    boxaDestroy(&boxat);

    if (pangle) *pangle = angle;
    if (pconf) *pconf = conf;

    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt5);
    return pixd;
}


/*------------------------------------------------------------------------*
 *                        Process to get line widths                      *
 *------------------------------------------------------------------------*/
/*!
 *  pixExtractBarcodeWidths1()
 *
 *      Input:  pixs (input image; 8 bpp)
 *              thresh (estimated pixel threshold for crossing
 *                      white <--> black; typ. ~120)
 *              binfract (histo binsize as a fraction of minsize; e.g., 0.25)
 *              &naehist (<optional return> histogram of black widths; NULL ok)
 *              &naohist (<optional return> histogram of white widths; NULL ok)
 *              debugflag (use 1 to generate debug output)
 *      Return: nad (numa of barcode widths in encoded integer units),
 *                  or null on error
 *
 *  Note:
 *     (1) The widths are alternating black/white, starting with black
 *         and ending with black.
 *     (2) This method uses the widths of the bars directly, in terms
 *         of the (float) number of pixels between transitions.
 *         The histograms of these widths for black and white bars is
 *         generated and interpreted.
 */
NUMA *
pixExtractBarcodeWidths1(PIX      *pixs,
                        l_float32  thresh,
                        l_float32  binfract,
                        NUMA     **pnaehist,
                        NUMA     **pnaohist,
                        l_int32    debugflag)
{
NUMA  *nac, *nad;

    PROCNAME("pixExtractBarcodeWidths1");

    if (!pixs || pixGetDepth(pixs) != 8)
        return (NUMA *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);

        /* Get the best estimate of the crossings, in pixel units */
    nac = pixExtractBarcodeCrossings(pixs, thresh, debugflag);

        /* Get the array of bar widths, starting with a black bar  */
    nad = numaQuantizeCrossingsByWidth(nac, binfract, pnaehist,
                                       pnaohist, debugflag);

    numaDestroy(&nac);
    return nad;
}


/*!
 *  pixExtractBarcodeWidths2()
 *
 *      Input:  pixs (input image; 8 bpp)
 *              thresh (estimated pixel threshold for crossing
 *                      white <--> black; typ. ~120)
 *              &width (<optional return> best decoding window width, in pixels)
 *              &nac (<optional return> number of transitions in each window)
 *              debugflag (use 1 to generate debug output)
 *      Return: nad (numa of barcode widths in encoded integer units),
 *                  or null on error
 *
 *  Notes:
 *      (1) The widths are alternating black/white, starting with black
 *          and ending with black.
 *      (2) The optional best decoding window width is the width of the window
 *          that is used to make a decision about whether a transition occurs.
 *          It is approximately the average width in pixels of the narrowest
 *          white and black bars (i.e., those corresponding to unit width).
 *      (3) The optional return signal @nac is a sequence of 0s, 1s,
 *          and perhaps a few 2s, giving the number of crossings in each window.
 *          On the occasion where there is a '2', it is interpreted as
 *          as ending two runs: the previous one and another one that has length 1.
 */
NUMA *
pixExtractBarcodeWidths2(PIX        *pixs,
                         l_float32   thresh,
                         l_float32  *pwidth,
                         NUMA      **pnac,
                         l_int32     debugflag)
{
NUMA  *nacp, *nad;

    PROCNAME("pixExtractBarcodeWidths2");

    if (!pixs || pixGetDepth(pixs) != 8)
        return (NUMA *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);

        /* Get the best estimate of the crossings, in pixel units */
    nacp = pixExtractBarcodeCrossings(pixs, thresh, debugflag);

        /* Quantize the crossings to get actual windowed data */
    nad = numaQuantizeCrossingsByWindow(nacp, 2.0, pwidth, NULL, pnac, debugflag);

    numaDestroy(&nacp);
    return nad;
}


/*!
 *  pixExtractBarcodeCrossings()
 *
 *      Input:  pixs (input image; 8 bpp)
 *              thresh (estimated pixel threshold for crossing
 *                      white <--> black; typ. ~120)
 *              debugflag (use 1 to generate debug output)
 *      Return: numa (of crossings, in pixel units), or null on error
 */
NUMA *
pixExtractBarcodeCrossings(PIX       *pixs,
                           l_float32  thresh,
                           l_int32    debugflag)
{
l_int32    w;
l_float32  bestthresh;
NUMA      *nas, *nax, *nay, *nad;

    PROCNAME("pixExtractBarcodeCrossings");

    if (!pixs || pixGetDepth(pixs) != 8)
        return (NUMA *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);

        /* Scan pixels horizontally and average results */
    nas = pixAverageRasterScans(pixs, 51);

        /* Interpolate to get 4x the number of values */
    w = pixGetWidth(pixs);
    numaInterpolateEqxInterval(0.0, 1.0, nas, L_QUADRATIC_INTERP, 0.0,
                               (l_float32)(w - 1), 4 * w + 1, &nax, &nay);

    if (debugflag) {
        GPLOT *gplot = gplotCreate("junksignal", GPLOT_X11, "Pixel values",
                                   "dist in pixels", "value");
        gplotAddPlot(gplot, nax, nay, GPLOT_LINES, "plot 1");
        gplotMakeOutput(gplot);
        gplotDestroy(&gplot);
    }

        /* Locate the crossings.  Run multiple times with different
         * thresholds, and choose a threshold in the center of the
         * run of thresholds that all give the maximum number of crossings. */
    numaSelectCrossingThreshold(nax, nay, thresh, &bestthresh);

        /* Get the crossings with the best threshold. */
    nad = numaCrossingsByThreshold(nax, nay, bestthresh);

    numaDestroy(&nas);
    numaDestroy(&nax);
    numaDestroy(&nay);
    return nad;
}


/*------------------------------------------------------------------------*
 *                         Average adjacent rasters                       *
 *------------------------------------------------------------------------*/
/*!
 *  pixAverageRasterScans()
 *
 *      Input:  pixs (input image; 8 bpp)
 *              nscans (number of adjacent scans, about the center vertically)
 *      Return: numa (of average pixel values across image), or null on error
 */
static NUMA *
pixAverageRasterScans(PIX     *pixs,
                      l_int32  nscans)
{
l_int32     w, h, first, last, i, j, wpl, val;
l_uint32   *line, *data;
l_float32  *array;
NUMA       *nad;

    PROCNAME("pixAverageRasterScans");

    if (!pixs || pixGetDepth(pixs) != 8)
        return (NUMA *)ERROR_PTR("pixs undefined or not 8 bpp", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    if (nscans <= h) {
        first = 0;
        last = h - 1;
        nscans = h;
    } else {
        first = (h - nscans) / 2;
        last = first + nscans - 1;
    }

    nad = numaCreate(w);
    numaSetCount(nad, w);
    array = numaGetFArray(nad, L_NOCOPY);
    wpl = pixGetWpl(pixs);
    data = pixGetData(pixs);
    for (j = 0; j < w; j++) {
        for (i = first; i <= last; i++) {
            line = data + i * wpl;
            val = GET_DATA_BYTE(line, j);
            array[j] += val;
        }
        array[j] = array[j] / (l_float32)nscans;
    }

    return nad;
}


/*------------------------------------------------------------------------*
 *                   Signal processing for barcode widths                 *
 *------------------------------------------------------------------------*/
/*!
 *  numaQuantizeCrossingsByWidth()
 *
 *      Input:  nas (numa of crossing locations, in pixel units)
 *              binfract (histo binsize as a fraction of minsize; e.g., 0.25)
 *              &naehist (<optional return> histo of even (black) bar widths)
 *              &naohist (<optional return> histo of odd (white) bar widths)
 *              debugflag (1 to generate plots of histograms of bar widths)
 *      Return: nad (sequence of widths, in unit sizes), or null on error
 *
 *  Notes:
 *      (1) This first computes the histogram of black and white bar widths,
 *          binned in appropriate units.  There should be well-defined
 *          peaks, each corresponding to a specific width.  The sequence
 *          of barcode widths (namely, the integers from the set {1,2,3,4})
 *          is returned.
 *      (2) The optional returned histograms are binned in width units
 *          that are inversely proportional to @binfract.  For example,
 *          if @binfract = 0.25, there are 4.0 bins in the distance of
 *          the width of the narrowest bar.
 */
NUMA *
numaQuantizeCrossingsByWidth(NUMA       *nas,
                             l_float32   binfract,
                             NUMA      **pnaehist,
                             NUMA      **pnaohist,
                             l_int32     debugflag)
{
l_int32    i, n, ned, nod, iw, width;
l_float32  val, minsize, maxsize, factor;
GPLOT     *gplot;
NUMA      *naedist, *naodist, *naehist, *naohist, *naecent, *naocent;
NUMA      *naerange, *naorange, *naelut, *naolut, *nad;

    PROCNAME("numaQuantizeCrossingsByWidth");

    if (!nas)
        return (NUMA *)ERROR_PTR("nas not defined", procName, NULL);
    n = numaGetCount(nas);
    if (n < 2)
        return (NUMA *)ERROR_PTR("n < 2", procName, NULL);
    if (binfract <= 0.0)
        return (NUMA *)ERROR_PTR("binfract <= 0.0", procName, NULL);

        /* Get even and odd crossing distances */
    numaGetCrossingDistances(nas, &naedist, &naodist, &minsize, &maxsize);

        /* Bin the spans in units of binfract * minsize.  These
         * units are convenient because they scale to make at least
         * 1/binfract bins in the smallest span (width).  We want this
         * number to be large enough to clearly separate the
         * widths, but small enough so that the histogram peaks
         * have very few if any holes (zeroes) within them.  */
    naehist = numaMakeHistogramClipped(naedist, binfract * minsize,
                                       (1.25 / binfract) * maxsize);
    naohist = numaMakeHistogramClipped(naodist, binfract * minsize,
                                       (1.25 / binfract) * maxsize);

    if (debugflag) {
        gplot = gplotCreate("junkhistw", GPLOT_X11,
                                   "Raw width histogram", "Width", "Number");
        gplotAddPlot(gplot, NULL, naehist, GPLOT_LINES, "plot black");
        gplotAddPlot(gplot, NULL, naohist, GPLOT_LINES, "plot white");
        gplotMakeOutput(gplot);
        gplotDestroy(&gplot);
    }

        /* Compute the peak ranges, still in units of binfract * minsize.  */
    naerange = numaLocatePeakRanges(naehist, 1.0 / binfract,
                                    1.0 / binfract, 0.0);
    naorange = numaLocatePeakRanges(naohist, 1.0 / binfract,
                                    1.0 / binfract, 0.0);

        /* Find the centroid values of each peak */
    naecent = numaGetPeakCentroids(naehist, naerange);
    naocent = numaGetPeakCentroids(naohist, naorange);

        /* Generate the lookup tables that map from the bar width, in
         * units of (binfract * minsize), to the integerized barcode
         * units (1, 2, 3, 4), which are the output integer widths
         * between transitions. */
    naelut = numaGetPeakWidthLUT(naerange, naecent);
    naolut = numaGetPeakWidthLUT(naorange, naocent);

        /* Get the widths.  Because the LUT accepts our funny units,
         * we first must convert the pixel widths to these units,
         * which is what 'factor' does. */
    nad = numaCreate(0);
    ned = numaGetCount(naedist);
    nod = numaGetCount(naodist);
    if (nod != ned - 1)
        L_WARNING("ned != nod + 1\n", procName);
    factor = 1.0 / (binfract * minsize);  /* for converting units */
    for (i = 0; i < ned - 1; i++) {
        numaGetFValue(naedist, i, &val);
        width = (l_int32)(factor * val);
        numaGetIValue(naelut, width, &iw);
        numaAddNumber(nad, iw);
/*        fprintf(stderr, "even: val = %7.3f, width = %d, iw = %d\n",
                val, width, iw); */
        numaGetFValue(naodist, i, &val);
        width = (l_int32)(factor * val);
        numaGetIValue(naolut, width, &iw);
        numaAddNumber(nad, iw);
/*        fprintf(stderr, "odd: val = %7.3f, width = %d, iw = %d\n",
                val, width, iw); */
    }
    numaGetFValue(naedist, ned - 1, &val);
    width = (l_int32)(factor * val);
    numaGetIValue(naelut, width, &iw);
    numaAddNumber(nad, iw);

    if (debugflag) {
        fprintf(stderr, " ---- Black bar widths (pixels) ------ \n");
        numaWriteStream(stderr, naedist);
    }
    if (debugflag) {
        fprintf(stderr, " ---- Histogram of black bar widths ------ \n");
        numaWriteStream(stderr, naehist);
    }
    if (debugflag) {
        fprintf(stderr, " ---- Peak ranges in black bar histogram bins ------ \n");
        numaWriteStream(stderr, naerange);
    }
    if (debugflag) {
        fprintf(stderr, " ---- Peak black bar centroid width values ------ \n");
        numaWriteStream(stderr, naecent);
    }
    if (debugflag) {
        fprintf(stderr, " ---- Black bar lookup table ------ \n");
        numaWriteStream(stderr, naelut);
    }
    if (debugflag) {
        fprintf(stderr, " ---- White bar widths (pixels) ------ \n");
        numaWriteStream(stderr, naodist);
    }
    if (debugflag) {
        fprintf(stderr, " ---- Histogram of white bar widths ------ \n");
        numaWriteStream(stderr, naohist);
    }
    if (debugflag) {
        fprintf(stderr, " ---- Peak ranges in white bar histogram bins ------ \n");
        numaWriteStream(stderr, naorange);
    }
    if (debugflag) {
        fprintf(stderr, " ---- Peak white bar centroid width values ------ \n");
        numaWriteStream(stderr, naocent);
    }
    if (debugflag) {
        fprintf(stderr, " ---- White bar lookup table ------ \n");
        numaWriteStream(stderr, naolut);
    }

    numaDestroy(&naedist);
    numaDestroy(&naodist);
    numaDestroy(&naerange);
    numaDestroy(&naorange);
    numaDestroy(&naecent);
    numaDestroy(&naocent);
    numaDestroy(&naelut);
    numaDestroy(&naolut);
    if (pnaehist)
        *pnaehist = naehist;
    else
        numaDestroy(&naehist);
    if (pnaohist)
        *pnaohist = naohist;
    else
        numaDestroy(&naohist);
    return nad;
}


/*!
 *  numaGetCrossingDistances()
 *
 *      Input:  nas (numa of crossing locations)
 *              &naedist (<optional return> even distances between crossings)
 *              &naodist (<optional return> odd distances between crossings)
 *              &mindist (<optional return> min distance between crossings)
 *              &maxdist (<optional return> max distance between crossings)
 *      Return: 0 if OK, 1 on error
 */
static l_int32
numaGetCrossingDistances(NUMA       *nas,
                         NUMA      **pnaedist,
                         NUMA      **pnaodist,
                         l_float32  *pmindist,
                         l_float32  *pmaxdist)
{
l_int32    i, n;
l_float32  val, newval, mindist, maxdist, dist;
NUMA      *naedist, *naodist;

    PROCNAME("numaGetCrossingDistances");

    if (pnaedist) *pnaedist = NULL;
    if (pnaodist) *pnaodist = NULL;
    if (pmindist) *pmindist = 0.0;
    if (pmaxdist) *pmaxdist = 0.0;
    if (!nas)
        return ERROR_INT("nas not defined", procName, 1);
    if ((n = numaGetCount(nas)) < 2)
        return ERROR_INT("n < 2", procName, 1);

        /* Get numas of distances between crossings.  Separate these
         * into even (e.g., black) and odd (e.g., white) spans.
         * For barcodes, the black spans are 0, 2, etc.  These
         * distances are in pixel units.  */
    naedist = numaCreate(n / 2 + 1);
    naodist = numaCreate(n / 2);
    numaGetFValue(nas, 0, &val);
    for (i = 1; i < n; i++) {
        numaGetFValue(nas, i, &newval);
        if (i % 2)
            numaAddNumber(naedist, newval - val);
        else
            numaAddNumber(naodist, newval - val);
        val = newval;
    }

        /* The mindist and maxdist of the spans are in pixel units. */
    numaGetMin(naedist, &mindist, NULL);
    numaGetMin(naodist, &dist, NULL);
    mindist = L_MIN(dist, mindist);
    numaGetMax(naedist, &maxdist, NULL);
    numaGetMax(naodist, &dist, NULL);
    maxdist = L_MAX(dist, maxdist);
    L_INFO("mindist = %7.3f, maxdist = %7.3f\n", procName, mindist, maxdist);

    if (pnaedist)
        *pnaedist = naedist;
    else
        numaDestroy(&naedist);
    if (pnaodist)
        *pnaodist = naodist;
    else
        numaDestroy(&naodist);
    if (pmindist) *pmindist = mindist;
    if (pmaxdist) *pmaxdist = maxdist;
    return 0;
}


/*!
 *  numaLocatePeakRanges()
 *
 *      Input:  nas (numa of histogram of crossing widths)
 *              minfirst (min location of center of first peak)
 *              minsep (min separation between peak range centers)
 *              maxmin (max allowed value for min histo value between peaks)
 *      Return: nad (ranges for each peak found, in pairs), or null on error
 *
 *  Notes:
 *      (1) Units of @minsep are the index into nas.
 *          This puts useful constraints on peak-finding.
 *      (2) If maxmin == 0.0, the value of nas[i] must go to 0.0 (or less)
 *          between peaks.
 *      (3) All calculations are done in units of the index into nas.
 *          The resulting ranges are therefore integers.
 *      (4) The output nad gives pairs of range values for successive peaks.
 *          Any location [i] for which maxmin = nas[i] = 0.0 will NOT be
 *          included in a peak range.  This works fine for histograms where
 *          if nas[i] == 0.0, it means that there are no samples at [i].
 *      (5) For barcodes, when this is used on a histogram of barcode
 *          widths, use maxmin = 0.0.  This requires that there is at
 *          least one histogram bin corresponding to a width value between
 *          adjacent peak ranges that is unpopulated, making the separation
 *          of the histogram peaks unambiguous.
 */
static NUMA *
numaLocatePeakRanges(NUMA      *nas,
                     l_float32  minfirst,
                     l_float32  minsep,
                     l_float32  maxmin)
{
l_int32    i, n, inpeak, left;
l_float32  center, prevcenter, val;
NUMA      *nad;

    PROCNAME("numaLocatePeakRanges");

    if (!nas)
        return (NUMA *)ERROR_PTR("nas not defined", procName, NULL);
    n = numaGetCount(nas);
    nad = numaCreate(0);

    inpeak = FALSE;
    prevcenter = minfirst - minsep - 1.0;
    for (i = 0; i < n; i++) {
        numaGetFValue(nas, i, &val);
        if (inpeak == FALSE && val > maxmin) {
            inpeak = TRUE;
            left = i;
        } else if (inpeak == TRUE && val <= maxmin) {  /* end peak */
            center = (left + i - 1.0) / 2.0;
            if (center - prevcenter >= minsep) {  /* save new peak */
                inpeak = FALSE;
                numaAddNumber(nad, left);
                numaAddNumber(nad, i - 1);
                prevcenter = center;
            } else {  /* attach to previous peak; revise the right edge */
                numaSetValue(nad, numaGetCount(nad) - 1, i - 1);
            }
        }
    }
    if (inpeak == TRUE) {  /* save the last peak */
        numaAddNumber(nad, left);
        numaAddNumber(nad, n - 1);
    }

    return nad;
}


/*!
 *  numaGetPeakCentroids()
 *
 *      Input:  nahist (numa of histogram of crossing widths)
 *              narange (numa of ranges of x-values for the peaks in @nahist)
 *      Return: nad (centroids for each peak found; max of 4, corresponding
 *                   to 4 different barcode line widths), or null on error
 */
static NUMA *
numaGetPeakCentroids(NUMA  *nahist,
                     NUMA  *narange)
{
l_int32    i, j, nh, nr, low, high;
l_float32  cent, sum, val;
NUMA      *nad;

    PROCNAME("numaGetPeakCentroids");

    if (!nahist)
        return (NUMA *)ERROR_PTR("nahist not defined", procName, NULL);
    if (!narange)
        return (NUMA *)ERROR_PTR("narange not defined", procName, NULL);
    nh = numaGetCount(nahist);
    nr = numaGetCount(narange) / 2;

    nad = numaCreate(4);
    for (i = 0; i < nr; i++) {
        numaGetIValue(narange, 2 * i, &low);
        numaGetIValue(narange, 2 * i + 1, &high);
        cent = 0.0;
        sum = 0.0;
        for (j = low; j <= high; j++) {
            numaGetFValue(nahist, j, &val);
            cent += j * val;
            sum += val;
        }
        numaAddNumber(nad, cent / sum);
    }

    return nad;
}


/*!
 *  numaGetPeakWidthLUT()
 *
 *      Input:  narange (numa of x-val ranges for the histogram width peaks)
 *              nacent (numa of centroids of each peak -- up to 4)
 *      Return: nalut (lookup table from the width of a bar to one of the four
 *                     integerized barcode units), or null on error
 *
 *  Notes:
 *      (1) This generates the lookup table that maps from a sequence of widths
 *          (in some units) to the integerized barcode units (1, 2, 3, 4),
 *          which are the output integer widths between transitions.
 *      (2) The smallest width can be lost in float roundoff.  To avoid
 *          losing it, we expand the peak range of the smallest width.
 */
static NUMA *
numaGetPeakWidthLUT(NUMA  *narange,
                    NUMA  *nacent)
{
l_int32     i, j, nc, low, high, imax;
l_int32     assign[4];
l_float32  *warray;
l_float32   max, rat21, rat32, rat42;
NUMA       *nalut;

    PROCNAME("numaGetPeakWidthLUT");

    if (!narange)
        return (NUMA *)ERROR_PTR("narange not defined", procName, NULL);
    if (!nacent)
        return (NUMA *)ERROR_PTR("nacent not defined", procName, NULL);
    nc = numaGetCount(nacent);  /* half the size of narange */
    if (nc < 1 || nc > 4)
        return (NUMA *)ERROR_PTR("nc must be 1, 2, 3, or 4", procName, NULL);

        /* Check the peak centroids for consistency with bar widths.
         * The third peak can correspond to a width of either 3 or 4.
         * Use ratios 3/2 and 4/2 instead of 3/1 and 4/1 because the
         * former are more stable and closer to the expected ratio.  */
    if (nc > 1) {
        warray = numaGetFArray(nacent, L_NOCOPY);
        if (warray[0] == 0)
            return (NUMA *)ERROR_PTR("first peak has width 0.0",
                                     procName, NULL);
        rat21 = warray[1] / warray[0];
        if (rat21 < 1.5 || rat21 > 2.6)
            L_WARNING("width ratio 2/1 = %f\n", procName, rat21);
        if (nc > 2) {
            rat32 = warray[2] / warray[1];
            if (rat32 < 1.3 || rat32 > 2.25)
                L_WARNING("width ratio 3/2 = %f\n", procName, rat32);
        }
        if (nc == 4) {
            rat42 = warray[3] / warray[1];
            if (rat42 < 1.7 || rat42 > 2.3)
                L_WARNING("width ratio 4/2 = %f\n", procName, rat42);
        }
    }

        /* Set width assignments.
         * The only possible ambiguity is with nc = 3 */
    for (i = 0; i < 4; i++)
        assign[i] = i + 1;
    if (nc == 3) {
        if (rat32 > 1.75)
            assign[2] = 4;
    }

        /* Put widths into the LUT */
    numaGetMax(narange, &max, NULL);
    imax = (l_int32)max;
    nalut = numaCreate(imax + 1);
    numaSetCount(nalut, imax + 1);  /* fill the array with zeroes */
    for (i = 0; i < nc; i++) {
        numaGetIValue(narange, 2 * i, &low);
        if (i == 0) low--;  /* catch smallest width */
        numaGetIValue(narange, 2 * i + 1, &high);
        for (j = low; j <= high; j++)
            numaSetValue(nalut, j, assign[i]);
    }

    return nalut;
}


/*!
 *  numaQuantizeCrossingsByWindow()
 *
 *      Input:  nas (numa of crossing locations)
 *              ratio (of max window size over min window size in search;
 *                     typ. 2.0)
 *              &width (<optional return> best window width)
 *              &firstloc (<optional return> center of window for first xing)
 *              &nac (<optional return> array of window crossings (0, 1, 2))
 *              debugflag (1 to generate various plots of intermediate results)
 *      Return: nad (sequence of widths, in unit sizes), or null on error
 *
 *  Notes:
 *      (1) The minimum size of the window is set by the minimum
 *          distance between zero crossings.
 *      (2) The optional return signal @nac is a sequence of 0s, 1s,
 *          and perhaps a few 2s, giving the number of crossings in each window.
 *          On the occasion where there is a '2', it is interpreted as
 *          ending two runs: the previous one and another one that has length 1.
 */
NUMA *
numaQuantizeCrossingsByWindow(NUMA       *nas,
                              l_float32   ratio,
                              l_float32  *pwidth,
                              l_float32  *pfirstloc,
                              NUMA      **pnac,
                              l_int32     debugflag)
{
l_int32    i, nw, started, count, trans;
l_float32  minsize, minwidth, minshift, xfirst;
NUMA      *nac, *nad;

    PROCNAME("numaQuantizeCrossingsByWindow");

    if (!nas)
        return (NUMA *)ERROR_PTR("nas not defined", procName, NULL);
    if (numaGetCount(nas) < 2)
        return (NUMA *)ERROR_PTR("nas size < 2", procName, NULL);

        /* Get the minsize, which is needed for the search for
         * the window width (ultimately found as 'minwidth') */
    numaGetCrossingDistances(nas, NULL, NULL, &minsize, NULL);

        /* Compute the width and shift increments; start at minsize
         * and go up to ratio * minsize  */
    numaEvalBestWidthAndShift(nas, 100, 10, minsize, ratio * minsize,
                              &minwidth, &minshift, NULL);

        /* Refine width and shift calculation */
    numaEvalBestWidthAndShift(nas, 100, 10, 0.98 * minwidth, 1.02 * minwidth,
                              &minwidth, &minshift, NULL);

    L_INFO("best width = %7.3f, best shift = %7.3f\n",
           procName, minwidth, minshift);

        /* Get the crossing array (0,1,2) for the best window width and shift */
    numaEvalSyncError(nas, 0, 0, minwidth, minshift, NULL, &nac);
    if (pwidth) *pwidth = minwidth;
    if (pfirstloc) {
        numaGetFValue(nas, 0, &xfirst);
        *pfirstloc = xfirst + minshift;
    }

        /* Get the array of bar widths, starting with a black bar  */
    nad = numaCreate(0);
    nw = numaGetCount(nac);  /* number of window measurements */
    started = FALSE;
    count = 0;  /* unnecessary init */
    for (i = 0; i < nw; i++) {
        numaGetIValue(nac, i, &trans);
        if (trans > 2)
            L_WARNING("trans = %d > 2 !!!\n", procName, trans);
        if (started) {
            if (trans > 1) {  /* i.e., when trans == 2 */
                numaAddNumber(nad, count);
                trans--;
                count = 1;
            }
            if (trans == 1) {
                numaAddNumber(nad, count);
                count = 1;
            } else {
                count++;
            }
        }
        if (!started && trans) {
            started = TRUE;
            if (trans == 2)  /* a whole bar in this window */
                numaAddNumber(nad, 1);
            count = 1;
        }
    }

    if (pnac)
        *pnac = nac;
    else
        numaDestroy(&nac);
    return nad;
}


/*!
 *  numaEvalBestWidthAndShift()
 *
 *      Input:  nas (numa of crossing locations)
 *              nwidth (number of widths to consider)
 *              nshift (number of shifts to consider for each width)
 *              minwidth (smallest width to consider)
 *              maxwidth (largest width to consider)
 *              &bestwidth (<return> best size of window)
 *              &bestshift (<return> best shift for the window)
 *              &bestscore (<optional return> average squared error of dist
 *                          of crossing signal from the center of the window)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This does a linear sweep of widths, evaluating at @nshift
 *          shifts for each width, finding the (width, shift) pair that
 *          gives the minimum score.
 */
static l_int32
numaEvalBestWidthAndShift(NUMA       *nas,
                          l_int32     nwidth,
                          l_int32     nshift,
                          l_float32   minwidth,
                          l_float32   maxwidth,
                          l_float32  *pbestwidth,
                          l_float32  *pbestshift,
                          l_float32  *pbestscore)
{
l_int32    i, j;
l_float32  delwidth, delshift, width, shift, score;
l_float32  bestwidth, bestshift, bestscore;

    PROCNAME("numaEvalBestWidthAndShift");

    if (!nas)
        return ERROR_INT("nas not defined", procName, 1);
    if (!pbestwidth || !pbestshift)
        return ERROR_INT("&bestwidth and &bestshift not defined", procName, 1);

    bestscore = 1.0;
    delwidth = (maxwidth - minwidth) / (nwidth - 1.0);
    for (i = 0; i < nwidth; i++) {
        width = minwidth + delwidth * i;
        delshift = width / (l_float32)(nshift);
        for (j = 0; j < nshift; j++) {
            shift = -0.5 * (width - delshift) + j * delshift;
            numaEvalSyncError(nas, 0, 0, width, shift, &score, NULL);
            if (score < bestscore) {
                bestscore = score;
                bestwidth = width;
                bestshift = shift;
#if  DEBUG_FREQUENCY
                fprintf(stderr, "width = %7.3f, shift = %7.3f, score = %7.3f\n",
                        width, shift, score);
#endif  /* DEBUG_FREQUENCY */
            }
        }
    }

    *pbestwidth = bestwidth;
    *pbestshift = bestshift;
    if (pbestscore)
        *pbestscore = bestscore;
    return 0;
}


/*!
 *  numaEvalSyncError()
 *
 *      Input:  nas (numa of crossing locations)
 *              ifirst (first crossing to use)
 *              ilast (last crossing to use; use 0 for all crossings)
 *              width (size of window)
 *              shift (of center of window w/rt first crossing)
 *              &score (<optional return> average squared error of dist
 *                      of crossing signal from the center of the window)
 *              &nad (<optional return> numa of 1s and 0s for crossings)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The score is computed only on the part of the signal from the
 *          @ifirst to @ilast crossings.  Use 0 for both of these to
 *          use all the crossings.  The score is normalized for
 *          the number of crossings and with half-width of the window.
 *      (2) The optional return @nad is a sequence of 0s and 1s, where a '1'
 *          indicates a crossing in the window.
 */
static l_int32
numaEvalSyncError(NUMA       *nas,
                  l_int32     ifirst,
                  l_int32     ilast,
                  l_float32   width,
                  l_float32   shift,
                  l_float32  *pscore,
                  NUMA      **pnad)
{
l_int32    i, n, nc, nw, ival;
l_int32    iw;  /* cell in which transition occurs */
l_float32  score, xfirst, xlast, xleft, xc, xwc;
NUMA      *nad;

    PROCNAME("numaEvalSyncError");

    if (!nas)
        return ERROR_INT("nas not defined", procName, 1);
    if ((n = numaGetCount(nas)) < 2)
        return ERROR_INT("nas size < 2", procName, 1);
    if (ifirst < 0) ifirst = 0;
    if (ilast <= 0) ilast = n - 1;
    if (ifirst >= ilast)
        return ERROR_INT("ifirst not < ilast", procName, 1);
    nc = ilast - ifirst + 1;

        /* Set up an array corresponding to the (shifted) windows,
         * and fill in the crossings. */
    score = 0.0;
    numaGetFValue(nas, ifirst, &xfirst);
    numaGetFValue(nas, ilast, &xlast);
    nw = (l_int32) ((xlast - xfirst + 2.0 * width) / width);
    nad = numaCreate(nw);
    numaSetCount(nad, nw);  /* init to all 0.0 */
    xleft = xfirst - width / 2.0 + shift;  /* left edge of first window */
    for (i = ifirst; i <= ilast; i++) {
        numaGetFValue(nas, i, &xc);
        iw = (l_int32)((xc - xleft) / width);
        xwc = xleft + (iw + 0.5) * width;  /* center of cell iw */
        score += (xwc - xc) * (xwc - xc);
        numaGetIValue(nad, iw, &ival);
        numaSetValue(nad, iw, ival + 1);
    }

    if (pscore)
        *pscore = 4.0 * score / (width * width * (l_float32)nc);
    if (pnad)
        *pnad = nad;
    else
        numaDestroy(&nad);

    return 0;
}
