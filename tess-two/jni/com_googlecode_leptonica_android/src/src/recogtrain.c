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
 *  recogtrain.c
 *
 *      Training on labelled data
 *         l_int32             recogTrainLabelled()
 *         l_int32             recogProcessMultLabelled()
 *         PIX                *recogProcessSingleLabelled()
 *         l_int32             recogAddSamples()
 *         PIX                *recogScaleCharacter()
 *         l_int32             recogAverageSamples()
 *         l_int32             pixaAccumulateSamples()
 *         l_int32             recogTrainingFinished()
 *         l_int32             recogRemoveOutliers()
 *
 *      Evaluate training status
 *         l_int32             recogaTrainingDone()
 *         l_int32             recogaFinishAveraging()
 *
 *      Training on unlabelled data
 *         l_int32             recogTrainUnlabelled()
 *
 *      Padding the training set
 *         l_int32             recogPadTrainingSet()
 *         l_int32            *recogMapIndexToIndex()
 *         static l_int32      recogAverageClassGeom()
 *         l_int32             recogaBestCorrelForPadding()
 *         l_int32             recogCorrelAverages()
 *         l_int32             recogSetPadParams()
 *         static l_int32      recogGetCharsetSize()
 *         static l_int32      recogCharsetAvailable()
 *
 *      Debugging
 *         l_int32             recogaShowContent()
 *         l_int32             recogShowContent()
 *         l_int32             recogDebugAverages()
 *         l_int32             recogShowAverageTemplates()
 *         PIX                *recogShowMatchesInRange()
 *         PIX                *recogShowMatch()
 *         l_int32             recogMakeBmf()
 *
 *      Static helpers
 *         static char        *l_charToString()
 *         static void         addDebugImage1()
 *         static void         addDebugImage2()
 */

#include <string.h>
#include "allheaders.h"


    /* Static functions */
static l_int32 *recogMapIndexToIndex(L_RECOG *recog1, L_RECOG *recog2);
static l_int32 recogAverageClassGeom(L_RECOG *recog, NUMA **pnaw, NUMA **pnah);
static l_int32 recogGetCharsetSize(l_int32 type);
static l_int32 recogCharsetAvailable(l_int32 type);
static char *l_charToString(char byte);
static void debugAddImage1(PIXA *pixa1, PIX *pix1, PIX *pix2, L_BMF *bmf,
                           l_float32 score);
static void debugAddImage2(PIXA **ppixadb, PIXA *pixa1, L_BMF *bmf,
                           l_int32 index);

    /* Defaults in pixRemoveOutliers() */
static const l_float32  DEFAULT_TARGET_SCORE = 0.75; /* keep everything above */
static const l_float32  DEFAULT_MIN_FRACTION = 0.5;  /* to be kept */

    /* Padding parameters for recognizer */
static const char *     DEFAULT_BOOT_DIR = "recog/digits";
static const char *     DEFAULT_BOOT_PATTERN = "digit_set";
static const char *     DEFAULT_BOOT_PATH = "recog/digits/bootnum1.pa";
static const l_int32    DEFAULT_CHARSET_TYPE = L_ARABIC_NUMERALS;
static const l_int32    DEFAULT_MIN_NOPAD = 3;
static const l_int32    DEFAULT_MAX_AFTERPAD = 15;
static const l_int32    MIN_TOTAL_SAMPLES = 10;  /* min char samples in recog */


/*------------------------------------------------------------------------*
 *                                Training                                *
 *------------------------------------------------------------------------*/
/*!
 *  recogTrainLabelled()
 *
 *      Input:  recog (in training mode)
 *              pixs (if depth > 1, will be thresholded to 1 bpp)
 *              box (<optional> cropping box)
 *              text (<optional> if null, use text field in pix)
 *              multflag (1 if one or more contiguous ascii characters;
 *                        0 for a single arbitrary character)
 *              debug (1 to display images of samples not captured)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Training is restricted to the addition of either:
 *          (a) multflag == 0: a single character in an arbitrary
 *              (e.g., UTF8) charset
 *          (b) multflag == 1: one or more ascii characters rendered
 *              contiguously in pixs
 *      (2) If box != null, it should represent the cropped location of
 *          the character image.
 *      (3) If multflag == 1, samples will be rejected if the number of
 *          connected components does not equal to the number of ascii
 *          characters in the textstring.  In that case, if debug == 1,
 *          the rejected samples will be displayed.
 */
l_int32
recogTrainLabelled(L_RECOG  *recog,
                   PIX      *pixs,
                   BOX      *box,
                   char     *text,
                   l_int32   multflag,
                   l_int32   debug)
{
l_int32  ret;
PIXA    *pixa;

    PROCNAME("recogTrainLabelled");

    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

    if (multflag == 0) {
        ret = recogProcessSingleLabelled(recog, pixs, box, text, &pixa);
    } else {
        ret = recogProcessMultLabelled(recog, pixs, box, text, &pixa, debug);
    }
    if (ret)
        return ERROR_INT("failure to add training data", procName, 1);
    recogAddSamples(recog, pixa, -1, debug);
    pixaDestroy(&pixa);
    return 0;
}


/*!
 *  recogProcessMultLabelled()
 *
 *      Input:  recog (in training mode)
 *              pixs (if depth > 1, will be thresholded to 1 bpp)
 *              box (<optional> cropping box)
 *              text (<optional> if null, use text field in pix)
 *              &pixa (<return> of split and thresholded characters)
 *              debug (1 to display images of samples not captured)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This crops and segments one or more labelled and contiguous
 *          ascii characters, for input in training.  It is a special case.
 *      (2) The character images are bundled into a pixa with the
 *          character text data embedded in each pix.
 *      (3) Where there is more than one character, this does some
 *          noise reduction and extracts the resulting character images
 *          from left to right.  No scaling is performed.
 */
l_int32
recogProcessMultLabelled(L_RECOG  *recog,
                         PIX      *pixs,
                         BOX      *box,
                         char     *text,
                         PIXA    **ppixa,
                         l_int32   debug)
{
char      *textdata, *textstr;
l_int32    textinpix, textin, nchars, ncomp, i;
BOX       *box2;
BOXA      *boxa1, *boxa2, *boxa3, *boxa4;
PIX       *pixc, *pixb, *pixt, *pix1, *pix2;

    PROCNAME("recogProcessMultLabelled");

    if (!ppixa)
        return ERROR_INT("&pixa not defined", procName, 1);
    *ppixa = NULL;
    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

        /* Find the text; this will be stored with the output images */
    textin = text && (text[0] != '\0');
    textinpix = pixs->text && (pixs->text[0] != '\0');
    if (!textin && !textinpix) {
        L_ERROR("no text: %d\n", procName, recog->samplenum);
        return 1;
    }
    textdata = (textin) ? text : pixs->text;  /* do not free */

        /* Crop and binarize if necessary */
    if (box)
        pixc = pixClipRectangle(pixs, box, NULL);
    else
        pixc = pixClone(pixs);
    if (pixGetDepth(pixc) > 1)
        pixb = pixConvertTo1(pixc, recog->threshold);
    else
        pixb = pixClone(pixc);
    pixDestroy(&pixc);

        /* We segment the set of characters as follows:
         * (1) A large vertical closing should consolidate most characters.
               Do not attempt to split touching characters using openings,
               because this is likely to break actual characters. */
    pix1 = pixMorphSequence(pixb, "c1.70", 0);

        /* (2) Include overlapping components and remove small ones */
    boxa1 = pixConnComp(pix1, NULL, 8);
    boxa2 = boxaCombineOverlaps(boxa1);
    boxa3 = boxaSelectBySize(boxa2, 2, 8, L_SELECT_IF_BOTH,
                             L_SELECT_IF_GT, NULL);
    pixDestroy(&pix1);
    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);

        /* (3) Make sure the components equal the number of text characters */
    ncomp = boxaGetCount(boxa3);
    nchars = strlen(textdata);
    if (ncomp != nchars) {
        L_ERROR("ncomp (%d) != nchars (%d); samplenum = %d\n",
                procName, ncomp, nchars, recog->samplenum);
        if (debug) {
            pixt = pixConvertTo32(pixb);
            pixRenderBoxaArb(pixt, boxa3, 1, 255, 0, 0);
            pixDisplay(pixt, 10 * recog->samplenum, 100);
            pixDestroy(&pixt);
        }
        pixDestroy(&pixb);
        boxaDestroy(&boxa3);
        return 1;
    }

        /* (4) Sort the components from left to right and extract them */
    boxa4 = boxaSort(boxa3, L_SORT_BY_X, L_SORT_INCREASING, NULL);
    boxaDestroy(&boxa3);

        /* Save the results, with one character in each pix */
    *ppixa = pixaCreate(ncomp);
    for (i = 0; i < ncomp; i++) {
        box2 = boxaGetBox(boxa4, i, L_CLONE);
        pix2 = pixClipRectangle(pixb, box2, NULL);
        textstr = l_charToString(textdata[i]);
        pixSetText(pix2, textstr);  /* inserts a copy */
        pixaAddPix(*ppixa, pix2, L_INSERT);
        boxDestroy(&box2);
        FREE(textstr);
    }

    pixDestroy(&pixb);
    boxaDestroy(&boxa4);
    return 0;
}


/*!
 *  recogProcessSingleLabelled()
 *
 *      Input:  recog (in training mode)
 *              pixs (if depth > 1, will be thresholded to 1 bpp)
 *              box (<optional> cropping box)
 *              text (<optional> if null, use text field in pix)
 *              &pixa (one pix, 1 bpp, labelled)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This crops and binarizes the input image, generating a pix
 *          of one character where the charval is inserted into the pix.
 */
l_int32
recogProcessSingleLabelled(L_RECOG  *recog,
                           PIX      *pixs,
                           BOX      *box,
                           char     *text,
                           PIXA    **ppixa)
{
char    *textdata;
l_int32  textinpix, textin;
PIX     *pixc, *pixb, *pixd;

    PROCNAME("recogProcessSingleLabelled");

    if (!ppixa)
        return ERROR_INT("&pixa not defined", procName, 1);
    *ppixa = NULL;
    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

        /* Find the text; this will be stored with the output images */
    textin = text && (text[0] != '\0');
    textinpix = (pixs->text && (pixs->text[0] != '\0'));
    if (!textin && !textinpix) {
        L_ERROR("no text: %d\n", procName, recog->samplenum);
        return 1;
    }
    textdata = (textin) ? text : pixs->text;  /* do not free */

        /* Crop and binarize if necessary */
    if (box)
        pixc = pixClipRectangle(pixs, box, NULL);
    else
        pixc = pixClone(pixs);
    if (pixGetDepth(pixc) > 1)
        pixb = pixConvertTo1(pixc, recog->threshold);
    else
        pixb = pixClone(pixc);
    pixDestroy(&pixc);

        /* Clip to foreground and save */
    pixClipToForeground(pixb, &pixd, NULL);
    pixDestroy(&pixb);
    if (!pixd)
        return ERROR_INT("pixd is empty", procName, 1);
    pixSetText(pixd, textdata);
    *ppixa = pixaCreate(1);
    pixaAddPix(*ppixa, pixd, L_INSERT);
    return 0;
}


/*!
 *  recogAddSamples()
 *
 *      Input:  recog
 *              pixa (1 or more characters)
 *              classindex (use -1 if not forcing into a specified class)
 *              debug
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The pix in the pixa are all 1 bpp, and the character string
 *          labels are embedded in the pix.
 *      (2) Note: this function decides what class each pix belongs in.
 *          When input is from a multifont pixaa, with a valid value
 *          for @classindex, the character string label in each pix
 *          is ignored, and @classindex is used as the class index
 *          for all the pix in the pixa.  Thus, for that situation we
 *          use this class index to avoid making the decision through a
 *          lookup based on the character strings embedded in the pix.
 *      (3) When a recog is initially filled with samples, the pixaa_u
 *          array is initialized to accept up to 256 different classes.
 *          When training is finished, the arrays are truncated to the
 *          actual number of classes.  To pad an existing recog from
 *          the boot recognizers, training is started again; if samples
 *          from a new class are added, the pixaa_u array must be
 *          extended by adding a pixa to hold them.
 */
l_int32
recogAddSamples(L_RECOG  *recog,
                PIXA     *pixa,
                l_int32   classindex,
                l_int32   debug)
{
char    *text;
l_int32  i, n, npa, charint, index;
PIX     *pixb;
PIXA    *pixa1;
PIXAA   *paa;

    PROCNAME("recogAddSamples");

    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);
    if (!pixa) {
        L_ERROR("pixa not defined: %d\n", procName, recog->samplenum);
        return 1;
    }
    if (recog->train_done)
        return ERROR_INT("training has been completed", procName, 1);
    if ((n = pixaGetCount(pixa)) == 0)
        ERROR_INT("no pix in the pixa", procName, 1);
    paa = recog->pixaa_u;

    for (i = 0; i < n; i++) {
        pixb = pixaGetPix(pixa, i, L_CLONE);
        if (classindex < 0) {
                /* Determine the class array index.  Check if the class
                 * alreadly exists, and if not, add it. */
            text = pixGetText(pixb);
            if (l_convertCharstrToInt(text, &charint) == 1) {
                L_ERROR("invalid text: %s\n", procName, text);
                pixDestroy(&pixb);
                continue;
            }
            if (recogGetClassIndex(recog, charint, text, &index) == 1) {
                    /* New class must be added */
                npa = pixaaGetCount(paa, NULL);
                if (index > npa)
                    L_ERROR("index %d > npa %d!!\n", procName, index, npa);
                if (index == npa) {  /* paa needs to be extended */
                    L_INFO("Adding new class and pixa with index %d\n",
                           procName, index);
                    pixa1 = pixaCreate(10);
                    pixaaAddPixa(paa, pixa1, L_INSERT);
                }
            }
            if (debug) {
                L_INFO("Identified text label: %s\n", procName, text);
                L_INFO("Identified: charint = %d, index = %d\n",
                       procName, charint, index);
            }
        } else {
            index = classindex;
        }

            /* Insert the unscaled character image into the right pixa.
             * (Unscaled images are required to split touching characters.) */
        recog->samplenum++;
        pixaaAddPix(paa, index, pixb, NULL, L_INSERT);
    }

    return 0;
}


/*!
 *  recogScaleCharacter()
 *
 *      Input:  recog
 *              pixs (1 bpp, to be scaled)
 *      Return: pixd (scaled) if OK, null on error
 */
PIX *
recogScaleCharacter(L_RECOG  *recog,
                    PIX      *pixs)
{
l_int32  w, h;

    PROCNAME("recogScaleCharacter");

    if (!recog)
        return (PIX *)ERROR_PTR("pix not defined", procName, NULL);
    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    if ((recog->scalew == 0 || recog->scalew == w) &&
        (recog->scaleh == 0 || recog->scaleh == h))  /* no scaling */
        return pixClone(pixs);
    else
        return pixScaleToSize(pixs, recog->scalew, recog->scaleh);
}


/*!
 *  recogAverageSamples()
 *
 *      Input:  recog
 *              debug
 *      Return: 0 on success, 1 on failure
 *
 *  Notes:
 *      (1) This is called when training is finished, and after
 *          outliers have been removed.
 *          Both unscaled and scaled inputs are averaged.
 *          Averages must be computed before any identification is done.
 *      (2) Set debug = 1 to view the resulting templates
 *          and their centroids.
 */
l_int32
recogAverageSamples(L_RECOG  *recog,
                    l_int32   debug)
{
l_int32    i, nsamp, size, area;
l_float32  x, y;
PIXA      *pixat, *pixa_sel;
PIX       *pix1, *pix2;
PTA       *ptat;

    PROCNAME("recogAverageSamples");

    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);

    if (recog->ave_done) {
        if (debug)  /* always do this if requested */
            recogShowAverageTemplates(recog);
        return 0;
    }

        /* Remove any previous averaging data */
    size = recog->setsize;
    pixaDestroy(&recog->pixa_u);
    ptaDestroy(&recog->pta_u);
    numaDestroy(&recog->nasum_u);
    recog->pixa_u = pixaCreate(size);
    recog->pta_u = ptaCreate(size);
    recog->nasum_u = numaCreate(size);

    pixaDestroy(&recog->pixa);
    ptaDestroy(&recog->pta);
    numaDestroy(&recog->nasum);
    recog->pixa = pixaCreate(size);
    recog->pta = ptaCreate(size);
    recog->nasum = numaCreate(size);

        /* Unscaled bitmaps: compute averaged bitmap, centroid, and fg area */
    for (i = 0; i < size; i++) {
        pixat = pixaaGetPixa(recog->pixaa_u, i, L_CLONE);
        ptat = ptaaGetPta(recog->ptaa_u, i, L_CLONE);
        nsamp = pixaGetCount(pixat);
        nsamp = L_MIN(nsamp, 256);  /* we only use the first 256 */
        if (nsamp == 0) {  /* no information for this class */
            pix1 = pixCreate(1, 1, 1);
            pixaAddPix(recog->pixa_u, pix1, L_INSERT);
            ptaAddPt(recog->pta_u, 0, 0);
            numaAddNumber(recog->nasum_u, 0);
        } else {
            pixaAccumulateSamples(pixat, ptat, &pix1, &x, &y);
            nsamp = (nsamp == 1) ? 2 : nsamp;  /* special case thresh */
            pix2 = pixThresholdToBinary(pix1, nsamp / 2);
            pixInvert(pix2, pix2);
            pixaAddPix(recog->pixa_u, pix2, L_INSERT);
            ptaAddPt(recog->pta_u, x, y);
            pixCountPixels(pix2, &area, recog->sumtab);
            numaAddNumber(recog->nasum_u, area);  /* foreground */
            pixDestroy(&pix1);
        }
        pixaDestroy(&pixat);
        ptaDestroy(&ptat);
    }

        /* Any classes for which there are no samples will have a 1x1
         * pix as a placeholder.  This must not be included when
         * finding the size range of the averaged templates. */
    pixa_sel = pixaSelectBySize(recog->pixa_u, 5, 5, L_SELECT_IF_BOTH,
                                L_SELECT_IF_GTE, NULL);
    pixaSizeRange(pixa_sel, &recog->minwidth_u, &recog->minheight_u,
                  &recog->maxwidth_u, &recog->maxheight_u);
    pixaDestroy(&pixa_sel);

        /* Scaled bitmaps: compute averaged bitmap, centroid, and fg area */
    for (i = 0; i < size; i++) {
        pixat = pixaaGetPixa(recog->pixaa, i, L_CLONE);
        ptat = ptaaGetPta(recog->ptaa, i, L_CLONE);
        nsamp = pixaGetCount(pixat);
        nsamp = L_MIN(nsamp, 256);  /* we only use the first 256 */
        if (nsamp == 0) {  /* no information for this class */
            pix1 = pixCreate(1, 1, 1);
            pixaAddPix(recog->pixa, pix1, L_INSERT);
            ptaAddPt(recog->pta, 0, 0);
            numaAddNumber(recog->nasum, 0);
        } else {
            pixaAccumulateSamples(pixat, ptat, &pix1, &x, &y);
            nsamp = (nsamp == 1) ? 2 : nsamp;  /* special case thresh */
            pix2 = pixThresholdToBinary(pix1, nsamp / 2);
            pixInvert(pix2, pix2);
            pixaAddPix(recog->pixa, pix2, L_INSERT);
            ptaAddPt(recog->pta, x, y);
            pixCountPixels(pix2, &area, recog->sumtab);
            numaAddNumber(recog->nasum, area);  /* foreground */
            pixDestroy(&pix1);
        }
        pixaDestroy(&pixat);
        ptaDestroy(&ptat);
    }
    pixa_sel = pixaSelectBySize(recog->pixa, 5, 5, L_SELECT_IF_BOTH,
                                L_SELECT_IF_GTE, NULL);
    pixaSizeRange(pixa_sel, &recog->minwidth, NULL, &recog->maxwidth, NULL);
    pixaDestroy(&pixa_sel);

       /* Get min and max splitting dimensions */
    recog->min_splitw = L_MAX(5, recog->minwidth_u - 5);
    recog->min_splith = L_MAX(5, recog->minheight_u - 5);
    recog->max_splith = recog->maxheight_u + 12;  /* allow for skew */

    if (debug)
        recogShowAverageTemplates(recog);

    recog->ave_done = TRUE;
    return 0;
}


/*!
 *  pixaAccumulateSamples()
 *
 *      Input:  pixa (of samples from the same class, 1 bpp)
 *              pta (<optional> of centroids of the samples)
 *              &ppixd (<return> accumulated samples, 8 bpp)
 *              &px (<optional return> average x coordinate of centroids)
 *              &py (<optional return> average y coordinate of centroids)
 *      Return: 0 on success, 1 on failure
 *
 *  Notes:
 *      (1) This generates an aligned (by centroid) sum of the input pix.
 *      (2) We use only the first 256 samples; that's plenty.
 *      (3) If pta is not input, we generate two tables, and discard
 *          after use.  If this is called many times, it is better
 *          to precompute the pta.
 */
l_int32
pixaAccumulateSamples(PIXA       *pixa,
                      PTA        *pta,
                      PIX       **ppixd,
                      l_float32  *px,
                      l_float32  *py)
{
l_int32    i, n, maxw, maxh, xdiff, ydiff;
l_int32   *centtab, *sumtab;
l_float32  x, y, xave, yave;
PIX       *pix1, *pix2, *pixsum;
PTA       *ptac;

    PROCNAME("pixaAccumulateSamples");

    if (!ppixd)
        return ERROR_INT("&pixd not defined", procName, 1);
    *ppixd = NULL;
    if (px) *px = 0;
    if (py) *py = 0;
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

    n = pixaGetCount(pixa);
    if (pta && ptaGetCount(pta) != n)
        return ERROR_INT("pta count differs from pixa count", procName, 1);
    n = L_MIN(n, 256);  /* take the first 256 only */
    if (n == 0)
        return ERROR_INT("pixa array empty", procName, 1);

    if (pta) {
        ptac = ptaClone(pta);
    } else {  /* generate them here */
        ptac = ptaCreate(n);
        centtab = makePixelCentroidTab8();
        sumtab = makePixelSumTab8();
        for (i = 0; i < n; i++) {
            pix1 = pixaGetPix(pixa, i, L_CLONE);
            pixCentroid(pix1, centtab, sumtab, &xave, &yave);
            ptaAddPt(ptac, xave, yave);
        }
        FREE(centtab);
        FREE(sumtab);
    }

        /* Find the average value of the centroids */
    xave = yave = 0;
    for (i = 0; i < n; i++) {
        ptaGetPt(pta, i, &x, &y);
        xave += x;
        yave += y;
    }
    xave = xave / (l_float32)n;
    yave = yave / (l_float32)n;
    if (px) *px = xave;
    if (py) *py = yave;

        /* Place all centroids at their average value and sum the results */
    pixaSizeRange(pixa, NULL, NULL, &maxw, &maxh);
    pixsum = pixInitAccumulate(maxw, maxh, 0);
    pix1 = pixCreate(maxw, maxh, 1);

    for (i = 0; i < n; i++) {
        pix2 = pixaGetPix(pixa, i, L_CLONE);
        ptaGetPt(ptac, i, &x, &y);
        xdiff = (l_int32)(x - xave);
        ydiff = (l_int32)(y - yave);
        pixClearAll(pix1);
        pixRasterop(pix1, xdiff, ydiff, maxw, maxh, PIX_SRC,
                    pix2, 0, 0);
        pixAccumulate(pixsum, pix1, L_ARITH_ADD);
        pixDestroy(&pix2);
    }
    *ppixd = pixFinalAccumulate(pixsum, 0, 8);

    pixDestroy(&pix1);
    pixDestroy(&pixsum);
    ptaDestroy(&ptac);
    return 0;
}


/*!
 *  recogTrainingFinished()
 *
 *      Input:  recog
 *              debug
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This must be called after all training samples have been added.
 *      (2) Set debug = 1 to view the resulting templates
 *          and their centroids.
 *      (3) The following things are done here:
 *          (a) Allocate (or reallocate) storage for (possibly) scaled
 *              bitmaps, centroids, and fg areas.
 *          (b) Generate the (possibly) scaled bitmaps.
 *          (c) Compute centroid and fg area data for both unscaled and
 *              scaled bitmaps.
 *          (d) Compute the averages for both scaled and unscaled bitmaps
 *          (e) Truncate the pixaa, ptaa and numaa arrays down from
 *              256 to the actual size.
 *      (4) Putting these operations here makes it simple to recompute
 *          the recog with different scaling on the bitmaps.
 *      (5) Removal of outliers must happen after this is called.
 */
l_int32
recogTrainingFinished(L_RECOG  *recog,
                      l_int32   debug)
{
l_int32    i, j, size, nc, ns, area;
l_float32  xave, yave;
PIX       *pix, *pixd;
PIXA      *pixa;
PIXAA     *paa;
PTA       *pta;
PTAA      *ptaa;

    PROCNAME("recogTrainingFinished");

    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);
    if (recog->train_done) return 0;

        /* Generate the storage for the possibly-scaled training bitmaps */
    size = recog->maxarraysize;
    paa = pixaaCreate(size);
    pixa = pixaCreate(1);
    pixaaInitFull(paa, pixa);
    pixaDestroy(&pixa);
    pixaaDestroy(&recog->pixaa);
    recog->pixaa = paa;

        /* Generate the storage for the unscaled centroid training data */
    ptaa = ptaaCreate(size);
    pta = ptaCreate(0);
    ptaaInitFull(ptaa, pta);
    ptaaDestroy(&recog->ptaa_u);
    recog->ptaa_u = ptaa;

        /* Generate the storage for the possibly-scaled centroid data */
    ptaa = ptaaCreate(size);
    ptaaInitFull(ptaa, pta);
    ptaDestroy(&pta);
    ptaaDestroy(&recog->ptaa);
    recog->ptaa = ptaa;

        /* Generate the storage for the fg area data */
    numaaDestroy(&recog->naasum_u);
    numaaDestroy(&recog->naasum);
    recog->naasum_u = numaaCreateFull(size, 0);
    recog->naasum = numaaCreateFull(size, 0);

    paa = recog->pixaa_u;
    nc = recog->setsize;
    for (i = 0; i < nc; i++) {
        pixa = pixaaGetPixa(paa, i, L_CLONE);
        ns = pixaGetCount(pixa);
        for (j = 0; j < ns; j++) {
                /* Save centroid and area data for the unscaled pix */
            pix = pixaGetPix(pixa, j, L_CLONE);
            pixCentroid(pix, recog->centtab, recog->sumtab, &xave, &yave);
            ptaaAddPt(recog->ptaa_u, i, xave, yave);
            pixCountPixels(pix, &area, recog->sumtab);
            numaaAddNumber(recog->naasum_u, i, area);  /* foreground */

                /* Insert the (optionally) scaled character image, and
                 * save centroid and area data for it */
            pixd = recogScaleCharacter(recog, pix);
            pixaaAddPix(recog->pixaa, i, pixd, NULL, L_INSERT);
            pixCentroid(pixd, recog->centtab, recog->sumtab, &xave, &yave);
            ptaaAddPt(recog->ptaa, i, xave, yave);
            pixCountPixels(pixd, &area, recog->sumtab);
            numaaAddNumber(recog->naasum, i, area);
            pixDestroy(&pix);
        }
        pixaDestroy(&pixa);
    }

        /* Get the template averages */
    recog->ave_done = FALSE;
    recogAverageSamples(recog, debug);

        /* Truncate the arrays to those with non-empty containers */
    pixaaTruncate(recog->pixaa_u);
    pixaaTruncate(recog->pixaa);
    ptaaTruncate(recog->ptaa_u);
    ptaaTruncate(recog->ptaa);
    numaaTruncate(recog->naasum_u);
    numaaTruncate(recog->naasum);

    recog->train_done = TRUE;
    return 0;
}


/*!
 *  recogRemoveOutliers()
 *
 *      Input:  recog (after training samples are entered)
 *              targetscore (keep everything with at least this score)
 *              minfract (minimum fraction to retain)
 *              debug (1 for debug output)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Removing outliers is particularly important when recognition
 *          goes against all the samples in the training set, as opposed
 *          to the averages for each class.  The reason is that we get
 *          an identification error if a mislabeled sample is a best
 *          match for an input bitmap.
 *      (2) However, the score values depend strongly on the quality
 *          of the character images.  To avoid losing too many samples,
 *          we supplement a target score for retention with a minimum
 *          fraction that we must keep.  With poor quality images, we
 *          may keep samples with a score less than the targetscore,
 *          in order to satisfy the @minfract requirement.
 *      (3) We always require that at least one sample will be retained.
 *      (4) Where the training set is from the same source (e.g., the
 *          same book), use a relatively large minscore; say, ~0.8.
 *      (5) Method: for each class, generate the averages and match each
 *          scaled sample against the average.  Decide which
 *          samples will be ejected, and throw out both the
 *          scaled and unscaled samples and associated data.
 *          Recompute the average without the poor matches.
 */
l_int32
recogRemoveOutliers(L_RECOG    *recog,
                    l_float32   targetscore,
                    l_float32   minfract,
                    l_int32     debug)
{
l_int32    i, j, nremoved, n, nkeep, ngood, ival, area1, area2;
l_float32  x1, y1, x2, y2, score, val;
NUMA      *nasum, *nasum_u, *nascore, *nainvert, *nasort;
PIX       *pix1, *pix2;
PIXA      *pixa, *pixa_u;
PTA       *pta, *pta_u;

    PROCNAME("recogRemoveOutliers");

    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);
    if (recog->train_done == FALSE)
        return ERROR_INT("recog training is not finished", procName, 1);
    targetscore = L_MIN(targetscore, 1.0);
    if (targetscore <= 0.0)
        targetscore = DEFAULT_TARGET_SCORE;
    minfract = L_MIN(minfract, 1.0);
    if (minfract <= 0.0)
        minfract = DEFAULT_MIN_FRACTION;

    nremoved = 0;
    for (i = 0; i < recog->setsize; i++) {
            /* Access the average template and values for scaled
             * images in this class */
        pix1 = pixaGetPix(recog->pixa, i, L_CLONE);
        ptaGetPt(recog->pta, i, &x1, &y1);
        numaGetIValue(recog->nasum, i, &area1);

            /* Get the sorted scores for each sample in the class */
        pixa = pixaaGetPixa(recog->pixaa, i, L_CLONE);
        pta = ptaaGetPta(recog->ptaa, i, L_CLONE);
        nasum = numaaGetNuma(recog->naasum, i, L_CLONE);
        n = pixaGetCount(pixa);
        nascore = numaCreate(n);
        for (j = 0; j < n; j++) {
            pix2 = pixaGetPix(pixa, j, L_CLONE);
            ptaGetPt(pta, j, &x2, &y2);
            numaGetIValue(nasum, j, &area2);
            pixCorrelationScoreSimple(pix1, pix2, area1, area2,
                                      x1 - x2, y1 - y2, 5, 5,
                                      recog->sumtab, &score);
            numaAddNumber(nascore, score);
            if (score == 0.0)  /* typ. large size difference */
                fprintf(stderr, "Got 0 score for i = %d, j = %d\n", i, j);
            pixDestroy(&pix2);
        }
        pixDestroy(&pix1);
            /* Symbolically, na[i] = nasort[nainvert[i]]  */
        numaSortGeneral(nascore, &nasort, NULL, &nainvert,
                        L_SORT_DECREASING, L_SHELL_SORT);

            /* Determine the cutoff in samples to keep */
        nkeep = (l_int32)(minfract * n + 0.5);
        ngood = n;
        for (j = 0; j < n; j++) {
            numaGetFValue(nasort, j, &val);
            if (val < targetscore) {
                ngood = j + 1;
                break;
            }
        }
        nkeep = L_MAX(1, L_MAX(nkeep, ngood));
        nremoved += (n - nkeep);
        if (debug && nkeep < n) {
            fprintf(stderr, "Removing %d of %d items from class %d\n",
                    n - nkeep, n, i);
        }

            /* Remove the samples with low scores.  Iterate backwards
             * in the original arrays, because we're compressing them
             * in place as elements are removed, and we must preserve
             * the indexing of elements not yet removed. */
        if (nkeep < n) {
            pixa_u = pixaaGetPixa(recog->pixaa_u, i, L_CLONE);
            pta_u = ptaaGetPta(recog->ptaa_u, i, L_CLONE);
            nasum_u = numaaGetNuma(recog->naasum_u, i, L_CLONE);
            for (j = n - 1; j >= 0; j--) {
                    /* ival is nainvert[j], which is the index into
                     * nasort that corresponds to the same element in
                     * na that is indexed by j (i.e., na[j]).  We retain
                     * the first nkeep elements in nasort. */
                numaGetIValue(nainvert, j, &ival);
                if (ival < nkeep) continue;
                pixaRemovePix(pixa, j);
                ptaRemovePt(pta, j);
                numaRemoveNumber(nasum, j);
                pixaRemovePix(pixa_u, j);
                ptaRemovePt(pta_u, j);
                numaRemoveNumber(nasum_u, j);
                if (debug) {
                    numaGetFValue(nascore, j, &val);
                    fprintf(stderr,
                            " removed item %d: score %7.3f\n", ival, val);
                }
            }
            pixaDestroy(&pixa_u);
            ptaDestroy(&pta_u);
            numaDestroy(&nasum_u);
        }

        pixaDestroy(&pixa);
        ptaDestroy(&pta);
        numaDestroy(&nasum);
        numaDestroy(&nascore);
        numaDestroy(&nainvert);
        numaDestroy(&nasort);
    }

        /* If anything was removed, recompute the average templates */
    if (nremoved > 0) {
        recog->samplenum -= nremoved;
        recog->ave_done = FALSE;  /* force recomputation */
        recogAverageSamples(recog, debug);
    }
    return 0;
}


/*------------------------------------------------------------------------*
 *                        Evaluate training status                        *
 *------------------------------------------------------------------------*/
/*!
 *  recogaTrainingDone()
 *
 *      Input:  recoga
 *             &done  (1 if training finished on all recog; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 */
l_int32
recogaTrainingDone(L_RECOGA  *recoga,
                   l_int32   *pdone)
{
l_int32   i;
L_RECOG  *recog;

    PROCNAME("recogaTrainingDone");

    if (!pdone)
        return ERROR_INT("&done not defined", procName, 1);
    *pdone = 0;
    if (!recoga)
        return ERROR_INT("recoga not defined", procName, 1);

    for (i = 0; i < recoga->n; i++) {
        if ((recog = recogaGetRecog(recoga, i)) == NULL)
            return ERROR_INT("recog not found", procName, 1);
        if (!recog->train_done)
            return 0;
    }

    *pdone = 1;
    return 0;
}


/*!
 *  recogaFinishAveraging()
 *
 *      Input:  recoga
 *      Return: 0 if OK, 1 on error
 */
l_int32
recogaFinishAveraging(L_RECOGA  *recoga)
{
l_int32   i;
L_RECOG  *recog;

    PROCNAME("recogaFinishAveraging");

    if (!recoga)
        return ERROR_INT("recoga not defined", procName, 1);

    for (i = 0; i < recoga->n; i++) {
        if ((recog = recogaGetRecog(recoga, i)) == NULL)
            return ERROR_INT("recog not found", procName, 1);
        if (!recog->ave_done)
            recogAverageSamples(recog, 0);
    }
    return 0;
}


/*------------------------------------------------------------------------*
 *                       Training on unlabelled data                      *
 *------------------------------------------------------------------------*/
/*!
 *  recogTrainUnlabelled()
 *
 *      Input:  recog (in training mode: the input characters in pixs are
 *                     inserted after labelling)
 *              recogboot (labels the input)
 *              pixs (if depth > 1, will be thresholded to 1 bpp)
 *              box (<optional> cropping box)
 *              singlechar (1 if pixs is a single character; 0 otherwise)
 *              minscore (min score for accepting the example; e.g., 0.75)
 *              debug (1 for debug output saved to recog; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This trains on unlabelled data, using a bootstrap recognizer
 *          to apply the labels.  In this way, we can build a recognizer
 *          using a source of unlabelled data.
 *      (2) The input pix can have several (non-touching) characters.
 *          If box != NULL, we treat the region in the box as a single char
 *          If box == NULL, use all of pixs:
 *             if singlechar == 0, we identify each c.c. as a single character
 *             if singlechar == 1, we treat pixs as a single character
 *          Multiple chars are identified separately by recogboot and
 *          inserted into recog.
 *      (3) recogboot is a trained recognizer.  It would typically be
 *          constructed from a variety of sources, and use the average
 *          templates for scoring.
 *      (4) For debugging, if bmf is defined in the recog, the correlation
 *          scores are generated and saved (by adding to the pixadb_boot
 *          field) with the matching images.
 */
l_int32
recogTrainUnlabelled(L_RECOG   *recog,
                     L_RECOG   *recogboot,
                     PIX       *pixs,
                     BOX       *box,
                     l_int32    singlechar,
                     l_float32  minscore,
                     l_int32    debug)
{
char      *text;
l_float32  score;
NUMA      *nascore, *na;
PIX       *pixc, *pixb, *pixdb;
PIXA      *pixa, *pixaf;

    PROCNAME("recogTrainUnlabelled");

    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);
    if (!recogboot)
        return ERROR_INT("recogboot not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

        /* Optionally crop */
    if (box)
        pixc = pixClipRectangle(pixs, box, NULL);
    else
        pixc = pixClone(pixs);

        /* Binarize if necessary */
    if (pixGetDepth(pixc) > 1)
        pixb = pixConvertTo1(pixc, recog->threshold);
    else
        pixb = pixClone(pixc);
    pixDestroy(&pixc);

        /* Identify the components using recogboot */
    if (singlechar == 1) {
        if (!debug) {
            recogIdentifyPix(recogboot, pixb, NULL);
        } else {
            recogIdentifyPix(recogboot, pixb, &pixdb);
            pixaAddPix(recog->pixadb_boot, pixdb, L_INSERT);
        }
        rchExtract(recogboot->rch, NULL, &score, &text, NULL, NULL, NULL, NULL);

            /* Threshold based on the score, and insert in a pixa */
        pixaf = pixaCreate(1);
        if (score >= minscore) {
            pixSetText(pixb, text);
            pixaAddPix(pixaf, pixb, L_CLONE);
            FREE(text);
                /* In use pixs is "unlabelled", so we only find a text
                 * string in the input pixs when testing with labelled data. */
            if (debug && ((text = pixGetText(pixs)) != NULL))
                L_INFO("Testing: input pix has character label: %s\n",
                       procName, text);
        }
    } else {  /* possibly multiple characters */
            /* Split into characters */
        pixSplitIntoCharacters(pixb, 5, 5, NULL, &pixa, NULL);

        if (!debug) {
            recogIdentifyPixa(recogboot, pixa, NULL, NULL);
        } else {
            recogIdentifyPixa(recogboot, pixa, NULL, &pixdb);
            pixaAddPix(recog->pixadb_boot, pixdb, L_INSERT);
        }
            /* Threshold the pixa based on the score */
        rchaExtract(recogboot->rcha, NULL, &nascore, NULL, NULL, NULL,
                    NULL, NULL);
        na = numaMakeThresholdIndicator(nascore, minscore, L_SELECT_IF_GTE);
        pixaf = pixaSelectWithIndicator(pixa, na, NULL);
        pixaDestroy(&pixa);
        numaDestroy(&nascore);
        numaDestroy(&na);
    }
    pixDestroy(&pixb);

        /* Insert the labelled components */
    recogAddSamples(recog, pixaf, -1, debug);
    pixaDestroy(&pixaf);
    return 0;
}


/*------------------------------------------------------------------------*
 *                         Padding the training set                       *
 *------------------------------------------------------------------------*/
/*!
 *  recogPadTrainingSet()
 *
 *      Input:  &recog (to be replaced if padding or more drastic measures
 *                      are necessary; otherwise, it is unchanged.)
 *              debug (1 for debug output saved to recog; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Before calling this, call recogSetPadParams() if you want
 *          non-default values for the character set type, min_nopad
 *          and max_afterpad values, and paths for labelled bitmap
 *          character sets that can be used to augment an input recognizer.
 *      (2) If all classes in @recog have at least min_nopad samples,
 *          nothing is done.  If the total number of samples in @recog
 *          is very small, @recog is replaced by a boot recog from the
 *          specified bootpath.  Otherwise (the intermediate case),
 *          @recog is replaced by one with scaling to fixed height,
 *          where an array of recog are used to augment the input recog.
 *      (3) If padding or total replacement is done, this destroys
 *          the input recog and replaces it by a new one.  If the recog
 *          belongs to a recoga, the replacement is also done in the recoga.
 */
l_int32
recogPadTrainingSet(L_RECOG  **precog,
                    l_int32    debug)
{
const char  *bootdir, *bootpattern, *bootpath;
char        *boottext, *fontdir;
l_int32      i, k, min_nopad, npix, nclass, nboot, nsamp, nextra, ntoadd;
l_int32      ave_height, targeth, setid, index, allclasses;
l_int32     *lut;
l_float32    minval, sum;
NUMA        *naclass, *naheight, *naset, *naindex, *nascore, *naave;
PIX         *pix1, *pix2;
PIXA        *pixaboot, *pixa1, *pixa2, *pixadb;
PIXAA       *paa, *paa1;
L_RECOG     *rec1, *recog, *recogboot;
L_RECOGA    *recoga;

    PROCNAME("recogPadTrainingSet");

    if (!precog)
        return ERROR_INT("&recog not defined", procName, 1);
    if ((recog = *precog) == NULL)
        return ERROR_INT("recog not defined", procName, 1);

    /* -------------------------------------------------
     * First, test if we need to use a boot recognizer.
     * ------------------------------------------------- */
        /* Are we asking for any padding at all? */
    min_nopad = recog->min_nopad;
    if (min_nopad <= 0)
        return 0;

        /* Do we have samples from all classes? */
    paa = recog->pixaa_u;  /* unscaled bitmaps */
    nclass = pixaaGetCount(paa, &naclass);
    allclasses = (nclass == recog->charset_size) ? 1 : 0;

        /* Are there enough samples in each class already? */
    numaGetMin(naclass, &minval, NULL);
    numaDestroy(&naclass);
    if (allclasses && minval >= min_nopad)
        return 0;

    /* ---------------------------------------------------------
     * We need a boot recognizer.  If there are too few characters
     * in the input recog, don't bother with padding.  Destroy the
     * input recog and return a generic boot recognizer that will
     * be run using scaling both width and height.
     * ----------------------------------------------------------*/
    fontdir = (recog->fontdir) ? stringNew(recog->fontdir) : NULL;
    if (recog->samplenum < MIN_TOTAL_SAMPLES) {
        L_WARNING("too few samples in recog; using bootrecog only\n", procName);
        bootpath = recog->bootpath;
        L_INFO("boot path = %s\n", procName, bootpath);
        if ((pixaboot = pixaRead(bootpath)) == NULL)
            return ERROR_INT("pixaboot not read", procName, 1);
        rec1 = recogCreateFromPixa(pixaboot, 20, 32, L_USE_AVERAGE, 100,
                                    1, fontdir);
        recogReplaceInRecoga(&recog, rec1);  /* destroys recog */
        *precog = rec1;
        pixaDestroy(&pixaboot);
        FREE(fontdir);
        return 0;
    }

    /* ---------------------------------------------------------
     * We need to pad the input recog.  Do this with an array of
     * boot recog with different 'fonts'.  For each class that must
     * be padded with samples, choose the boot recog from the recog
     * array that has the best correlation to samples of that class.
     * --------------------------------------------------------- */
         /* Do we have boot recog for this charset?
          * TODO: add some more of 'em */
    if (recogCharsetAvailable(recog->charset_type) == FALSE)
        return ERROR_INT("charset type not available", procName, 1);
    bootdir = recog->bootdir;
    bootpattern = recog->bootpattern;
    L_INFO("dir = %s; pattern = %s\n", procName, bootdir, bootpattern);
    L_INFO("min_nopad = %d; max_afterpad = %d\n", procName,
           min_nopad, recog->max_afterpad);

        /* To pad the input recognizer, use an array of recog, generated
         * from pixa with files specified by bootdir and bootpattern.
         * The samples are scaled to h = 32 to allow correlation with the
         * averages from a copy of the input recog, also scaled to h = 32. */
    if ((paa1 = pixaaReadFromFiles(bootdir, bootpattern, 0, 0)) == NULL)
        return ERROR_INT("boot recog files not found", procName, 1);
    recoga = recogaCreateFromPixaa(paa1, 0, 32, L_USE_AVERAGE, 100, 1,
                                   fontdir);
    pixaaDestroy(&paa1);
    if (!recoga)
        return ERROR_INT("recoga not made", procName, 1);

        /* The parameters of the input recog must match those of the
         * boot array.  Replace the input recog with a new one, that
         * uses the average templates for matching, scaled to h = 32. */
    rec1 = recogCreateFromRecog(recog, 0, 32, L_USE_AVERAGE, 100, 1,
                                fontdir);
    recogReplaceInRecoga(&recog, rec1);  /* destroys recog */
    *precog = rec1;
    recog = rec1;
    FREE(fontdir);

        /* Now for each class in the recog, decide which recog in recoga
         * should be used to select samples for padding the recog. */
    pixadb = (debug) ? pixaCreate(0) : NULL;
    recogBestCorrelForPadding(rec1, recoga, &naset, &naindex, &nascore,
                              &naave, &pixadb);

    if (pixadb) {
        lept_mkdir("recog");
        numaWriteStream(stderr, naset);
        numaWriteStream(stderr, naindex);
        numaWriteStream(stderr, nascore);
        numaWriteStream(stderr, naave);
        pix1 = pixaDisplayLinearly(pixadb, L_VERT, 1.0, 0, 20, 0, NULL);
        pixWrite("/tmp/recog/padmatch.png", pix1, IFF_PNG);
        pixDestroy(&pix1);
        pixaDestroy(&pixadb);
    }

        /* Allow more examples to be added to the input/returned recog */
    recog->train_done = FALSE;

    /* ---------------------------------------------------------
     * For the existing classes in recog, add samples from the boot
     * recognizer where needed.  For each sample, scale isotropically
     * to the average unscaled height for the given class.
     * ----------------------------------------------------------*/
    recogAverageClassGeom(recog, NULL, &naheight);
    numaGetSum(naheight, &sum);
    paa = recog->pixaa_u;
    pixaaGetCount(paa, &naclass);
    ave_height = (l_int32)(sum / nclass);
    for (i = 0; i < nclass; i++) {
        numaGetIValue(naclass, i, &npix);
        if (npix >= min_nopad) continue;
        numaGetIValue(naheight, i, &targeth);

            /* Locate the images to be added */
        numaGetIValue(naset, i, &setid);
        if ((rec1 = recogaGetRecog(recoga, setid)) == NULL) {
            L_ERROR("For class %d, didn't find recog %d\n", procName, i, setid);
            continue;
        }
        numaGetIValue(naindex, i, &index);  /* class index in rec */
        if ((pixa1 = pixaaGetPixa(rec1->pixaa_u, index, L_CLONE)) == NULL) {
            L_ERROR("For recog %d, didn't find class %d\n", procName,
                    setid, index);
            continue;
        }

            /* Decide how many of them to scale and add */
        nboot = pixaGetCount(pixa1);
        nextra = recog->max_afterpad - npix;
        if (nextra <= 0) continue;  /* this should not be triggered */
        ntoadd = L_MIN(nextra, nboot);
        L_INFO("For class %d, using %d samples from index %d in recog %d\n",
               procName, i, ntoadd, index, setid);

            /* Add them */
        pixa2 = pixaCreate(ntoadd);
        boottext = sarrayGetString(rec1->sa_text, index, L_NOCOPY);
        for (k = 0; k < ntoadd; k++) {
             pix1 = pixaGetPix(pixa1, k, L_CLONE);
             pix2 = pixScaleToSize(pix1, 0, targeth);
             pixSetText(pix2, boottext);
             pixaAddPix(pixa2, pix2, L_INSERT);
             pixDestroy(&pix1);
        }
        recogAddSamples(recog, pixa2, i, 0);
        pixaDestroy(&pixa1);
        pixaDestroy(&pixa2);
    }

    /* ---------------------------------------------------------------
     * Check if there are missing classes.  If there are, add these
     * classes and use samples from the bootrecog with the highest
     * overall correlation with the input recog.  Use the average
     * unscaled height of all the recog classes, which should be fine
     * for all-caps, where character heights are similar, but may give
     * bad estimates for old digit fonts where the digits "0", "1" and
     * "2" are often shorter.
     * ------------------------------------------------------------ */
    numaGetMax(naave, NULL, &index);  /* best of the recoga set */
    recogboot = recogaGetRecog(recoga, index);
    nboot = recogGetCount(recogboot);
    L_INFO("nboot = %d, nclass = %d, best index = %d\n",
           procName, nboot, nclass, index);
    if (nboot > nclass) {  /* missing some classes in recog */
        L_INFO("Adding %d classes to the recog\n", procName, nboot - nclass);
        targeth = ave_height;
        if ((lut = recogMapIndexToIndex(recogboot, recog)) == NULL)
            return ERROR_INT("index-to-index lut not made", procName, 1);
        for (i = 0; i < nboot; i++) {
            if (lut[i] >= 0)  /* already have this class */
                continue;
            pixaboot = pixaaGetPixa(recogboot->pixaa_u, i, L_CLONE);
            nsamp = pixaGetCount(pixaboot);
            ntoadd = L_MIN(recog->max_afterpad, nsamp);
            pixa1 = pixaCreate(ntoadd);
            boottext = sarrayGetString(recogboot->sa_text, i, L_NOCOPY);
            L_INFO("Adding %d chars of type '%s' from recog %d\n", procName,
                   ntoadd, boottext, index);
            for (k = 0; k < ntoadd; k++) {
                 pix1 = pixaGetPix(pixaboot, k, L_CLONE);
                 pix2 = pixScaleToSize(pix1, 0, targeth);
                 pixSetText(pix2, boottext);
                 pixaAddPix(pixa1, pix2, L_INSERT);
                 pixDestroy(&pix1);
            }
            recogAddSamples(recog, pixa1, -1, debug);
            pixaDestroy(&pixaboot);
            pixaDestroy(&pixa1);
        }
        FREE(lut);
    }
    recogTrainingFinished(recog, 0);

    if (debug) {
        recogShowContent(stderr, recog, 1);
        recogDebugAverages(recog, 1);
    }

    numaDestroy(&naclass);
    numaDestroy(&naheight);
    numaDestroy(&naset);
    numaDestroy(&naindex);
    numaDestroy(&nascore);
    numaDestroy(&naave);
    recogaDestroy(&recoga);
    return 0;
}


/*!
 *  recogMapIndexToIndex()
 *
 *      Input:  recog1
 *              recog2
 *      Return: lut (from recog1 --> recog2), or null on error
 *
 *  Notes:
 *      (1) This returns a map from each index in recog1 to the
 *          corresponding index in recog2.  Caller must free.
 *      (2) If the character string doesn't exist in any of the classes
 *          in recog2, the value -1 is inserted in the lut.
 */
static l_int32 *
recogMapIndexToIndex(L_RECOG  *recog1,
                     L_RECOG  *recog2)
{
char     *charstr;
l_int32   index1, index2, n1;
l_int32  *lut;

    PROCNAME("recogMapIndexToIndex");

    if (!recog1 || !recog2)
        return (l_int32 *)ERROR_PTR("recog1 and recog2 not both defined",
                                    procName, NULL);

    n1 = recog1->setsize;
    if ((lut = (l_int32 *)CALLOC(n1, sizeof(l_int32))) == NULL)
        return (l_int32 *)ERROR_PTR("lut not made", procName, NULL);
    for (index1 = 0; index1 < n1; index1++) {
        recogGetClassString(recog1, index1, &charstr);
        if (!charstr) {
            L_ERROR("string not found for index %d\n", procName, index1);
            lut[index1] = -1;
            continue;
        }
        recogStringToIndex(recog2, charstr, &index2);
        lut[index1] = index2;
        FREE(charstr);
    }

    return lut;
}


/*!
 *  recogAverageClassGeom()
 *
 *      Input:  recog
 *              &naw (<optional return> average widths for each class)
 *              &nah (<optional return> average heights for each class)
 *      Return: 0 if OK, 1 on error
 */
static l_int32
recogAverageClassGeom(L_RECOG  *recog,
                      NUMA    **pnaw,
                      NUMA    **pnah)
{
l_int32  i, j, w, h, sumw, sumh, npix, nclass;
NUMA    *naw, *nah;
PIXA    *pixa;

    PROCNAME("recogAverageClassGeom");

    if (!pnaw && !pnah)
        return ERROR_INT("nothing to do", procName, 1);
    if (pnaw) *pnaw = NULL;
    if (pnah) *pnah = NULL;
    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);

    if ((nclass = pixaaGetCount(recog->pixaa_u, NULL)) == 0)
        return ERROR_INT("no classes", procName, 1);
    naw = numaCreate(nclass);
    nah = numaCreate(nclass);
    for (i = 0; i < nclass; i++) {
        if ((pixa = pixaaGetPixa(recog->pixaa_u, i, L_CLONE)) == NULL) {
            L_WARNING("pixa[%d] not found\n", procName, i);
            continue;
        }
        sumw = sumh = 0;
        npix = pixaGetCount(pixa);
        for (j = 0; j < npix; j++) {
            pixaGetPixDimensions(pixa, j, &w, &h, NULL);
            sumw += w;
            sumh += h;
        }
        numaAddNumber(naw, (l_int32)((l_float32)sumw / npix + 0.5));
        numaAddNumber(nah, (l_int32)((l_float32)sumh / npix + 0.5));
        pixaDestroy(&pixa);
    }

    if (pnaw)
        *pnaw = naw;
    else
        numaDestroy(&naw);
    if (pnah)
        *pnah = nah;
    else
        numaDestroy(&nah);
    return 0;
}


/*!
 *  recogBestCorrelForPadding()
 *
 *      Input:  recog (typically the recog to be padded)
 *              recoga (array of recogs for potentially providing the padding)
 *              &naset (<return> of indices into the sets to be matched)
 *              &naindex (<return> of matching indices into the best set)
 *              &nascore (<return> of best correlation scores)
 *              &naave (<return> average of correlation scores from each recog)
 *              &pixadb (<optional return> debug images; use NULL for no debug)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This finds, for each class in recog, the best matching template
 *          in the recoga.  For that best match, it returns:
 *            * the recog set index in the recoga,
 *            * the index in that recog for the class,
 *            * the score for the best match
 *      (2) It also returns in @naave, for each recog in recoga, the
 *          average overall correlation for all averaged templates to
 *          those in the input recog.  The recog with the largest average
 *          can supply templates in cases where the input recog has
 *          no examples.
 *      (3) For classes in recog1 for which no corresponding class
 *          is found in any recog in recoga, the index -1 is stored
 *          in both naset and naindex, and 0.0 is stored in nascore.
 *      (4) Both recog and all the recog in recoga should be generated
 *          with isotropic scaling to the same character height (e.g., 30).
 */
l_int32
recogBestCorrelForPadding(L_RECOG   *recog,
                          L_RECOGA  *recoga,
                          NUMA     **pnaset,
                          NUMA     **pnaindex,
                          NUMA     **pnascore,
                          NUMA     **pnasum,
                          PIXA     **ppixadb)
{
l_int32    i, j, n, nrec, index, maxindex, maxset;
l_float32  score, maxscore;
NUMA      *nain, *nasc, *naset, *naindex, *nascore, *nasum;
NUMAA     *naain, *naasc;
L_RECOG   *rec;

    PROCNAME("recogBestCorrelForPadding");

    if (!pnaset)
        return ERROR_INT("&naset not defined", procName, 1);
    *pnaset = NULL;
    if (!pnaindex)
        return ERROR_INT("&naindex not defined", procName, 1);
    *pnaindex = NULL;
    if (!pnascore)
        return ERROR_INT("&nascore not defined", procName, 1);
    *pnascore = NULL;
    if (!pnasum)
        return ERROR_INT("&nasum not defined", procName, 1);
    *pnasum = NULL;
    if (!recog)
        return ERROR_INT("recog is not defined", procName, 1);
    if (!recoga)
        return ERROR_INT("recoga is not defined", procName, 1);
    if (!recog->train_done)
        return ERROR_INT("recog training is not finished", procName, 1);

        /* Gather the correlation data */
    n = recog->setsize;
    nrec = recogaGetCount(recoga);
    if (n == 0 || nrec == 0)
        return ERROR_INT("recog or recoga is empty", procName, 1);
    naain = numaaCreate(nrec);
    naasc = numaaCreate(nrec);
    for (i = 0; i < nrec; i++) {
        rec = recogaGetRecog(recoga, i);
        recogCorrelAverages(recog, rec, &nain, &nasc, ppixadb);
        numaaAddNuma(naain, nain, L_INSERT);
        numaaAddNuma(naasc, nasc, L_INSERT);
    }

        /* Find the best matches */
    naset = numaCreate(n);
    naindex = numaCreate(n);
    nascore = numaCreate(n);
    nasum = numaMakeConstant(0.0, nrec);  /* accumulate sum over recognizers */
    for (i = 0; i < n; i++) {  /* over classes in recog */
        maxscore = 0.0;
        maxindex = -1;
        maxset = -1;
        for (j = 0; j < nrec; j++) {  /* over recognizers */
            numaaGetValue(naain, j, i, NULL, &index);  /* index in j to i */
            if (index == -1) continue;
            numaaGetValue(naasc, j, i, &score, NULL);  /* score in j to i */
            numaAddToNumber(nasum, j, score);
            if (score > maxscore) {
                maxscore = score;
                maxindex = index;
                maxset = j;
            }
        }
        numaAddNumber(naset, maxset);
        numaAddNumber(naindex, maxindex);
        numaAddNumber(nascore, maxscore);
    }

    *pnaset = naset;
    *pnaindex = naindex;
    *pnascore = nascore;
    *pnasum = numaTransform(nasum, 0.0, 1. / (l_float32)n);
    numaDestroy(&nasum);
    numaaDestroy(&naain);
    numaaDestroy(&naasc);
    return 0;
}


/*!
 *  recogCorrelAverages()
 *
 *      Input:  recog1 (typically the recog to be padded)
 *              recog2 (potentially providing the padding)
 *              &naindex (<return> of classes in 2 with respect to classes in 1)
 *              &nascore (<return> correlation scores of corresponding classes)
 *              &pixadb (<optional return> debug images)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Use this for potentially padding recog1 with instances in recog2.
 *          The recog have been generated with isotropic scaling to the
 *          same fixed height (e.g., 30).  The training has been "finished"
 *          in the sense that all arrays have been computed and they
 *          could potentially be used as they are.  This is necessary
 *          for doing the correlation between scaled images.
 *          However, this function is called when there is a request to
 *          augument some of the examples in classes in recog1.
 *      (2) Iterate over classes in recog1, finding the corresponding
 *          class in recog2 and computing the correlation score between
 *          the average templates of the two.  naindex is a LUT between
 *          the index of a class in recog1 and the corresponding one in recog2.
 *      (3) For classes in recog1 that do not exist in recog2, the index
 *          -1 is stored in naindex, and 0.0 is stored in the score.
 */
l_int32
recogCorrelAverages(L_RECOG  *recog1,
                    L_RECOG  *recog2,
                    NUMA    **pnaindex,
                    NUMA    **pnascore,
                    PIXA    **ppixadb)
{
l_int32    i1, i2, n1, area1, area2, wvar;
l_int32   *lut;
l_float32  x1, y1, x2, y2, score;
PIX       *pix1, *pix2;
PIXA      *pixa1;

    PROCNAME("recogCorrelAverages");

    if (!pnaindex)
        return ERROR_INT("&naindex not defined", procName, 1);
    *pnaindex = NULL;
    if (!pnascore)
        return ERROR_INT("&nascore not defined", procName, 1);
    *pnascore = NULL;
    if (!recog1 || !recog2)
        return ERROR_INT("recog1 and recog2 not both defined", procName, 1);
    if (!recog1->train_done || !recog2->train_done)
        return ERROR_INT("recog training is not finished", procName, 1);

    if ((lut = recogMapIndexToIndex(recog1, recog2)) == NULL)
        return ERROR_INT("index-to-index lut not made", procName, 1);
    n1 = recog1->setsize;
    *pnaindex = numaCreateFromIArray(lut, n1);
    *pnascore = numaMakeConstant(0.0, n1);

    pixa1 = (ppixadb) ? pixaCreate(n1) : NULL;
    for (i1 = 0; i1 < n1; i1++) {
            /* Access the average templates and values for this class */
        if ((i2 = lut[i1]) == -1) {
            L_INFO("no class in 2 corresponds to index %d in 1\n",
                   procName, i1);
            continue;
        }
        pix1 = pixaGetPix(recog1->pixa, i1, L_CLONE);
        ptaGetPt(recog1->pta, i1, &x1, &y1);
        numaGetIValue(recog1->nasum, i1, &area1);
        pix2 = pixaGetPix(recog2->pixa, i2, L_CLONE);
        ptaGetPt(recog2->pta, i2, &x2, &y2);
        numaGetIValue(recog2->nasum, i2, &area2);

            /* Find their correlation and save the results.
             * The heights should all be scaled to the same value (e.g., 30),
             * but the widths can vary, so we need a large tolerance (wvar)
             * to force correlation between all templates. */
        wvar = 0.6 * recog1->scaleh;
        pixCorrelationScoreSimple(pix1, pix2, area1, area2,
                                  x1 - x2, y1 - y2, wvar, 0,
                                  recog1->sumtab, &score);
        numaSetValue(*pnascore, i1, score);
        debugAddImage1(pixa1, pix1, pix2, recog1->bmf, score);
        pixDestroy(&pix1);
        pixDestroy(&pix2);
    }
    debugAddImage2(ppixadb, pixa1, recog1->bmf, recog2->index);

    pixaDestroy(&pixa1);
    FREE(lut);
    return 0;
}


/*!
 *  recogSetPadParams()
 *
 *      Input:  recog (to be padded, if necessary)
 *              bootdir (<optional> directory to bootstrap labelled pixa)
 *              bootpattern (<optional> pattern for bootstrap labelled pixa)
 *              bootpath (<optional> path to single bootstrap labelled pixa)
 *              type (character set type; -1 for default; see enum in recog.h)
 *              size (character set size; -1 for default)
 *              min_nopad (min number in a class without padding; -1 default)
 *              max_afterpad (max number of samples in padded classes;
 *                            -1 for default)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is used to augment or replace a book-adapted recognizer (BAR).
 *          It is called when the recognizer is created, and must be
 *          called again before recogPadTrainingSet() if non-default
 *          values are to be used.
 *      (2) Default values allow for some padding.  To disable padding,
 *          set @min_nopad = 0.
 *      (3) Constraint on @min_nopad and @max_afterpad guarantees that
 *          padding will be allowed if requested.
 *      (4) The file directory (@bootdir) and tail pattern (@bootpattern)
 *          are used to identify serialized pixa, from which we can
 *          generate an array of recog.  These can be used to augment
 *          an input but incomplete BAR (book adapted recognizer).
 *      (5) If the BAR is very sparse, we will ignore it and use a generic
 *          bootstrap recognizer at @bootpath.
 */
l_int32
recogSetPadParams(L_RECOG     *recog,
                  const char  *bootdir,
                  const char  *bootpattern,
                  const char  *bootpath,
                  l_int32      type,
                  l_int32      min_nopad,
                  l_int32      max_afterpad)
{

    PROCNAME("recogSetPadParams");

    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);
    if (min_nopad >= 0 && max_afterpad >= 0 && min_nopad >= max_afterpad)
        return ERROR_INT("min_ must be less than max_", procName, 1);

    FREE(recog->bootdir);
    FREE(recog->bootpattern);
    FREE(recog->bootpath);
    recog->bootdir = (bootdir) ? stringNew(bootdir)
                               : stringNew(DEFAULT_BOOT_DIR);
    recog->bootpattern = (bootpattern) ? stringNew(bootpattern)
                                       : stringNew(DEFAULT_BOOT_PATTERN);
    recog->bootpath = (bootpath) ? stringNew(bootpath)
                                 : stringNew(DEFAULT_BOOT_PATH);
    recog->charset_type = (type >= 0) ? type : DEFAULT_CHARSET_TYPE;
    recog->charset_size = recogGetCharsetSize(recog->charset_type);
    recog->min_nopad = (min_nopad >= 0) ? min_nopad : DEFAULT_MIN_NOPAD;
    recog->max_afterpad =
        (max_afterpad >= 0) ? max_afterpad : DEFAULT_MAX_AFTERPAD;
    return 0;
}


/*!
 *  recogGetCharsetSize()
 *
 *      Input:  type (of charset)
 *      Return: size of charset, or 0 if unknown or on error
 */
static l_int32
recogGetCharsetSize(l_int32  type)
{
    PROCNAME("recogGetCharsetSize");

    switch (type) {
    case L_UNKNOWN:
        return 0;
    case L_ARABIC_NUMERALS:
        return 10;
    case L_LC_ROMAN_NUMERALS:
        return 7;
    case L_UC_ROMAN_NUMERALS:
        return 7;
    case L_LC_ALPHA:
        return 26;
    case L_UC_ALPHA:
        return 26;
    default:
        L_ERROR("invalid charset_type %d\n", procName, type);
        return 0;
    }
    return 0;  /* shouldn't happen */
}


/*!
 *  recogCharsetAvailable()
 *
 *      Input:  type (of charset for padding)
 *      Return: 1 if available; 0 if not.
 */
static l_int32
recogCharsetAvailable(l_int32  type)
{
l_int32  ret;

    PROCNAME("recogCharsetAvailable");

    switch (type)
    {
    case L_ARABIC_NUMERALS:
        ret = TRUE;
        break;
    case L_LC_ROMAN_NUMERALS:
    case L_UC_ROMAN_NUMERALS:
    case L_LC_ALPHA:
    case L_UC_ALPHA:
        L_INFO("charset type %d not available", procName, type);
        ret = FALSE;
        break;
    default:
        L_INFO("charset type %d is unknown", procName, type);
        ret = FALSE;
        break;
    }

    return ret;
}


/*------------------------------------------------------------------------*
 *                               Debugging                                *
 *------------------------------------------------------------------------*/
/*!
 *  recogaShowContent()
 *
 *      Input:  stream
 *              recoga
 *              display (1 for showing template images, 0 otherwise)
 *      Return: 0 if OK, 1 on error
 */
l_int32
recogaShowContent(FILE      *fp,
                  L_RECOGA  *recoga,
                  l_int32    display)
{
l_int32   i, n;
L_RECOG  *recog;

    PROCNAME("recogaShowContent");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!recoga)
        return ERROR_INT("recog not defined", procName, 1);
    if ((n = recogaGetCount(recoga)) == 0)
        return ERROR_INT("no recog found", procName, 1);

    fprintf(fp, "\nDebug print of recoga contents:\n");
    for (i = 0; i < n; i++) {
        if ((recog = recogaGetRecog(recoga, i)) == NULL) {
            L_ERROR("recog %d not found!\n", procName, i);
            continue;
        }
        fprintf(fp, "\nRecog %d:\n", i);
        if (recog->train_done == FALSE)
            L_WARNING("training for recog %d is not finished\n", procName, i);
        recogShowContent(fp, recog, display);
    }
    return 0;
}


/*!
 *  recogShowContent()
 *
 *      Input:  stream
 *              recog
 *              display (1 for showing template images, 0 otherwise)
 *      Return: 0 if OK, 1 on error
 */
l_int32
recogShowContent(FILE     *fp,
                 L_RECOG  *recog,
                 l_int32   display)
{
l_int32  i, val, count;
PIX     *pix;
NUMA    *na;

    PROCNAME("recogShowContent");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);

    fprintf(fp, "Debug print of recog contents\n");
    fprintf(fp, "  Setsize: %d\n", recog->setsize);
    fprintf(fp, "  Binarization threshold: %d\n", recog->threshold);
    fprintf(fp, "  Maximum matching y-jiggle: %d\n", recog->maxyshift);
    if (recog->templ_type == L_USE_ALL)
        fprintf(fp, "  Using all samples for matching\n");
    else
        fprintf(fp, "  Using averaged template for matching\n");
    if (recog->scalew == 0)
        fprintf(fp, "  No width scaling of templates\n");
    else
        fprintf(fp, "  Template width scaled to %d\n", recog->scalew);
    if (recog->scaleh == 0)
        fprintf(fp, "  No height scaling of templates\n");
    else
        fprintf(fp, "  Template height scaled to %d\n", recog->scaleh);
    fprintf(fp, "  Number of samples in each class:\n");
    pixaaGetCount(recog->pixaa_u, &na);
    for (i = 0; i < recog->setsize; i++) {
        l_dnaGetIValue(recog->dna_tochar, i, &val);
        numaGetIValue(na, i, &count);
        if (val < 128)
            fprintf(fp, "    class %d, char %c:   %d\n", i, val, count);
        else
            fprintf(fp, "    class %d, val %d:   %d\n", i, val, count);
    }
    numaDestroy(&na);

    if (display) {
        pix = pixaaDisplayByPixa(recog->pixaa_u, 20, 20, 1000);
        pixDisplay(pix, 0, 0);
        pixDestroy(&pix);
        if (recog->train_done) {
            pix = pixaaDisplayByPixa(recog->pixaa, 20, 20, 1000);
            pixDisplay(pix, 800, 0);
            pixDestroy(&pix);
        }
    }
    return 0;
}


/*!
 *  recogDebugAverages()
 *
 *      Input:  recog
 *              debug (0 no output; 1 for images; 2 for text; 3 for both)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Generates an image that pairs each of the input images used
 *          in training with the average template that it is best
 *          correlated to.  This is written into the recog.
 *      (2) It also generates pixa_tr of all the input training images,
 *          which can be used, e.g., in recogShowMatchesInRange().
 */
l_int32
recogDebugAverages(L_RECOG  *recog,
                   l_int32   debug)
{
l_int32    i, j, n, np, index;
l_float32  score;
PIX       *pix1, *pix2, *pix3;
PIXA      *pixa, *pixat;
PIXAA     *paa1, *paa2;

    PROCNAME("recogDebugAverages");

    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);

        /* Mark the training as finished if necessary, and make sure
         * that the average templates have been built. */
    recogAverageSamples(recog, 0);
    paa1 = recog->pixaa;

        /* Save a pixa of all the training examples */
    if (!recog->pixa_tr)
        recog->pixa_tr = pixaaFlattenToPixa(paa1, NULL, L_CLONE);

        /* Destroy any existing image and make a new one */
    if (recog->pixdb_ave)
        pixDestroy(&recog->pixdb_ave);
    n = pixaaGetCount(paa1, NULL);
    paa2 = pixaaCreate(n);
    for (i = 0; i < n; i++) {
        pixa = pixaCreate(0);
        pixat = pixaaGetPixa(paa1, i, L_CLONE);
        np = pixaGetCount(pixat);
        for (j = 0; j < np; j++) {
            pix1 = pixaaGetPix(paa1, i, j, L_CLONE);
            recogIdentifyPix(recog, pix1, &pix2);
            rchExtract(recog->rch, &index, &score, NULL, NULL, NULL,
                       NULL, NULL);
            if (debug >= 2)
                fprintf(stderr, "index = %d, score = %7.3f\n", index, score);
            pix3 = pixAddBorder(pix2, 2, 1);
            pixaAddPix(pixa, pix3, L_INSERT);
            pixDestroy(&pix1);
            pixDestroy(&pix2);
        }
        pixaaAddPixa(paa2, pixa, L_INSERT);
        pixaDestroy(&pixat);
    }
    recog->pixdb_ave = pixaaDisplayByPixa(paa2, 20, 20, 2500);
    if (debug % 2) pixDisplay(recog->pixdb_ave, 100, 100);

    pixaaDestroy(&paa2);
    return 0;
}


/*!
 *  recogShowAverageTemplates()
 *
 *      Input:  recog
 *      Return: 0 on success, 1 on failure
 *
 *  Notes:
 *      (1) This debug routine generates a display of the averaged templates,
 *          both scaled and unscaled, with the centroid visible in red.
 */
l_int32
recogShowAverageTemplates(L_RECOG  *recog)
{
l_int32    i, size;
l_float32  x, y;
PIX       *pix1, *pix2, *pixr;
PIXA      *pixat, *pixadb;

    PROCNAME("recogShowAverageTemplates");

    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);

    fprintf(stderr, "minwidth_u = %d, minheight_u = %d, maxheight_u = %d\n",
            recog->minwidth_u, recog->minheight_u, recog->maxheight_u);
    fprintf(stderr, "minw = %d, minh = %d, maxh = %d\n",
            recog->min_splitw, recog->min_splith, recog->max_splith);

    pixaDestroy(&recog->pixadb_ave);

    pixr = pixCreate(3, 3, 32);  /* 3x3 red square for centroid location */
    pixSetAllArbitrary(pixr, 0xff000000);
    pixadb = pixaCreate(2);

        /* Unscaled bitmaps */
    size = recog->setsize;
    pixat = pixaCreate(size);
    for (i = 0; i < size; i++) {
        if ((pix1 = pixaGetPix(recog->pixa_u, i, L_CLONE)) == NULL)
            continue;
        pix2 = pixConvertTo32(pix1);
        ptaGetPt(recog->pta_u, i, &x, &y);
        pixRasterop(pix2, (l_int32)(x - 0.5), (l_int32)(y - 0.5), 3, 3,
                    PIX_SRC, pixr, 0, 0);
        pixaAddPix(pixat, pix2, L_INSERT);
        pixDestroy(&pix1);
    }
    pix1 = pixaDisplayTiledInRows(pixat, 32, 3000, 1.0, 0, 20, 0);
    pixaAddPix(pixadb, pix1, L_INSERT);
    pixDisplay(pix1, 100, 100);
    pixaDestroy(&pixat);

        /* Scaled bitmaps */
    pixat = pixaCreate(size);
    for (i = 0; i < size; i++) {
        if ((pix1 = pixaGetPix(recog->pixa, i, L_CLONE)) == NULL)
            continue;
        pix2 = pixConvertTo32(pix1);
        ptaGetPt(recog->pta, i, &x, &y);
        pixRasterop(pix2, (l_int32)(x - 0.5), (l_int32)(y - 0.5), 3, 3,
                    PIX_SRC, pixr, 0, 0);
        pixaAddPix(pixat, pix2, L_INSERT);
        pixDestroy(&pix1);
    }
    pix1 = pixaDisplayTiledInRows(pixat, 32, 3000, 1.0, 0, 20, 0);
    pixaAddPix(pixadb, pix1, L_INSERT);
    pixDisplay(pix1, 100, 100);
    pixaDestroy(&pixat);
    pixDestroy(&pixr);
    recog->pixadb_ave = pixadb;
    return 0;
}


/*!
 *  recogShowMatchesInRange()
 *
 *      Input:  recog
 *              pixa (of 1 bpp images to match)
 *              minscore, maxscore (range to include output)
 *              display (to display the result)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This gives a visual output of the best matches for a given
 *          range of scores.  Each pair of images can optionally be
 *          labelled with the index of the best match and the correlation.
 *          If the bmf has been previously made, it will be used here.
 *      (2) To use this, save a set of 1 bpp images (labelled or
 *          unlabelled) that can be given to a recognizer in a pixa.
 *          Then call this function with the pixa and parameters
 *          to filter a range of score.
 */
l_int32
recogShowMatchesInRange(L_RECOG     *recog,
                        PIXA        *pixa,
                        l_float32    minscore,
                        l_float32    maxscore,
                        l_int32      display)
{
l_int32    i, n, index, depth;
l_float32  score;
NUMA      *nascore, *naindex;
PIX       *pix1, *pix2;
PIXA      *pixa1, *pixa2;

    PROCNAME("recogShowMatchesInRange");

    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);
    if (!pixa)
        return ERROR_INT("pixa not defined", procName, 1);

        /* Run the recognizer on the set of images */
    n = pixaGetCount(pixa);
    nascore = numaCreate(n);
    naindex = numaCreate(n);
    pixa1 = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pix1 = pixaGetPix(pixa, i, L_CLONE);
        recogIdentifyPix(recog, pix1, &pix2);
        rchExtract(recog->rch, &index, &score, NULL, NULL, NULL, NULL, NULL);
        numaAddNumber(nascore, score);
        numaAddNumber(naindex, index);
        pixaAddPix(pixa1, pix2, L_INSERT);
        pixDestroy(&pix1);
    }

        /* Filter the set and optionally add text to each */
    pixa2 = pixaCreate(n);
    depth = 1;
    for (i = 0; i < n; i++) {
        numaGetFValue(nascore, i, &score);
        if (score < minscore || score > maxscore) continue;
        pix1 = pixaGetPix(pixa1, i, L_CLONE);
        numaGetIValue(naindex, i, &index);
        pix2 = recogShowMatch(recog, pix1, NULL, NULL, index, score);
        if (i == 0) depth = pixGetDepth(pix2);
        pixaAddPix(pixa2, pix2, L_INSERT);
        pixDestroy(&pix1);
    }

        /* Package it up */
    pixDestroy(&recog->pixdb_range);
    if (pixaGetCount(pixa2) > 0) {
        recog->pixdb_range =
            pixaDisplayTiledInRows(pixa2, depth, 2500, 1.0, 0, 20, 1);
        if (display)
            pixDisplay(recog->pixdb_range, 300, 100);
    } else {
        L_INFO("no character matches in the range of scores\n", procName);
    }

    pixaDestroy(&pixa1);
    pixaDestroy(&pixa2);
    numaDestroy(&nascore);
    numaDestroy(&naindex);
    return 0;
}


/*!
 *  recogShowMatch()
 *
 *      Input:  recog
 *              pix1  (input pix; several possibilities)
 *              pix2  (<optional> matching template)
 *              box  (<optional> region in pix1 for which pix2 matches)
 *              index  (index of matching template; use -1 to disable printing)
 *              score  (score of match)
 *      Return: pixd (pair of images, showing input pix and best template),
 *                    or null on error.
 *
 *  Notes:
 *      (1) pix1 can be one of these:
 *          (a) The input pix alone, which can be either a single character
 *              (box == NULL) or several characters that need to be
 *              segmented.  If more than character is present, the box
 *              region is displayed with an outline.
 *          (b) Both the input pix and the matching template.  In this case,
 *              pix2 and box will both be null.
 *      (2) If the bmf has been made (by a call to recogMakeBmf())
 *          and the index >= 0, the index and score will be rendered;
 *          otherwise their values will be ignored.
 */
PIX *
recogShowMatch(L_RECOG   *recog,
               PIX       *pix1,
               PIX       *pix2,
               BOX       *box,
               l_int32    index,
               l_float32  score)
{
char    buf[32];
L_BMF  *bmf;
PIX    *pix3, *pix4, *pix5, *pixd;
PIXA   *pixa;

    PROCNAME("recogShowMatch");

    if (!recog)
        return (PIX *)ERROR_PTR("recog not defined", procName, NULL);
    if (!pix1)
        return (PIX *)ERROR_PTR("pix1 not defined", procName, NULL);

    bmf = (recog->bmf && index >= 0) ? recog->bmf : NULL;
    if (!pix2 && !box && !bmf)  /* nothing to do */
        return pixCopy(NULL, pix1);

    pix3 = pixConvertTo32(pix1);
    if (box)
        pixRenderBoxArb(pix3, box, 1, 255, 0, 0);

    if (pix2) {
        pixa = pixaCreate(2);
        pixaAddPix(pixa, pix3, L_CLONE);
        pixaAddPix(pixa, pix2, L_CLONE);
        pix4 = pixaDisplayTiledInRows(pixa, 1, 500, 1.0, 0, 15, 0);
        pixaDestroy(&pixa);
    } else {
        pix4 = pixCopy(NULL, pix3);
    }
    pixDestroy(&pix3);

    if (bmf) {
        pix5 = pixAddBorderGeneral(pix4, 55, 55, 0, 0, 0xffffff00);
        snprintf(buf, sizeof(buf), "I = %d, S = %4.3f", index, score);
        pixd = pixAddSingleTextblock(pix5, bmf, buf, 0xff000000,
                                     L_ADD_BELOW, NULL);
        pixDestroy(&pix5);
    } else {
        pixd = pixClone(pix4);
    }
    pixDestroy(&pix4);

    return pixd;
}


/*!
 *  recogMakeBmf()
 *
 *      Input:  recog
 *              fontdir (for bitmap fonts; typically "fonts")
 *              size  (of font; even integer between 4 and 20; default is 6)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This can be used to (re)set the size of the font used for
 *          debug labelling.
 */
l_int32
recogMakeBmf(L_RECOG     *recog,
             const char  *fontdir,
             l_int32      size)
{
   PROCNAME("recogMakeBmf");

   if (!recog || !fontdir)
        return ERROR_INT("recog and fontdir not both defined", procName, 1);
   if (size < 4 || size > 20 || (size % 2)) size = 6;
   if (size == recog->bmf_size) return 0;  /* no change */

   bmfDestroy(&recog->bmf);
   recog->bmf = bmfCreate(fontdir, size);
   recog->bmf_size = size;
   return 0;
}


/*------------------------------------------------------------------------*
 *                             Static helpers                             *
 *------------------------------------------------------------------------*/
static char *
l_charToString(char byte)
{
char  *str;

  str = (char *)CALLOC(2, sizeof(char));
  str[0] = byte;
  return str;
}


/*
 *  debugAddImage1()
 *
 *      Input:  pixa1 (<optional> for accumulating pairs of images
 *              pix1, pix2
 *              bmf
 *              score (rendered using the bmf)
 *      Return: void
 *
 *  Notes:
 *      (1) If pixa1 is NULL, do nothing.  Otherwise, we add a pair of
 *          images with a score.  This is accumulated for each
 *          corresponding class templates in the two recog.
 */
static void
debugAddImage1(PIXA      *pixa1,
               PIX       *pix1,
               PIX       *pix2,
               L_BMF     *bmf,
               l_float32  score)
{
char   buf[16];
PIX   *pix3, *pix4, *pix5;
PIXA  *pixa2;

    if (!pixa1) return;
    pixa2 = pixaCreate(2);
    pix3 = pixAddBorder(pix1, 5, 0);
    pixaAddPix(pixa2, pix3, L_INSERT);
    pix3 = pixAddBorder(pix2, 5, 0);
    pixaAddPix(pixa2, pix3, L_INSERT);
    pix4 = pixaDisplayTiledInRows(pixa2, 32, 1000, 1.0, 0, 20, 2);
    snprintf(buf, sizeof(buf), "%5.3f", score);
    pix5 = pixAddSingleTextline(pix4, bmf, buf, 0xff000000, L_ADD_BELOW);
    pixaAddPix(pixa1, pix5, L_INSERT);
    pixDestroy(&pix4);
    pixaDestroy(&pixa2);
    return;
}


/*
 *  debugAddImage2()
 *
 *      Input:  &pixadb (<optional; possible return>
 *              pixa1 (<optional> accumulated pairs of images)
 *              bmf
 *              index (of recog in recoga)
 *      Return: void
 *
 *  Notes:
 *      (1) If pixa1 is NULL, do nothing.
 *      (2) If this is the first time this function is called, then
 *          *ppixadb == NULL, so we create pixadb (storing the ptr at ppixadb).
 *      (3) Display pixa1 into a pix and add to pixadb.
 *      (4) Subsequent calls, for different recognizers that could be used
 *          for augmenting the instances, add to pixadb.
 */
static void
debugAddImage2(PIXA   **ppixadb,
               PIXA    *pixa1,
               L_BMF   *bmf,
               l_int32  index)
{
char   buf[16];
PIX   *pix1, *pix2, *pix3, *pix4;

    if (!pixa1) return;
    if (ppixadb == NULL) {
        L_ERROR("@pixadb is NULL; shouldn't happen!\n", "debugAddImage2");
        return;
    }
    if (*ppixadb == NULL)
        *ppixadb = pixaCreate(0);
    pix1 = pixaDisplayTiledInRows(pixa1, 32, 2000, 1.0, 0, 20, 0);
    snprintf(buf, sizeof(buf), "Recog %d", index);
    pix2 = pixAddSingleTextline(pix1, bmf, buf, 0xff000000, L_ADD_BELOW);
    pix3 = pixAddBorder(pix2, 5, 0);
    pix4 = pixAddBorder(pix3, 2, 1);
    pixaAddPix(*ppixadb, pix4, L_INSERT);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    return;
}


