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
 * jbclass.c
 *
 *     These are functions for unsupervised classification of
 *     collections of connected components -- either characters or
 *     words -- in binary images.  They can be used as image
 *     processing steps in jbig2 compression.
 *
 *     Initialization
 *
 *         JBCLASSER         *jbRankHausInit()      [rank hausdorff encoder]
 *         JBCLASSER         *jbCorrelationInit()   [correlation encoder]
 *         JBCLASSER         *jbCorrelationInitWithoutComponents()  [ditto]
 *         static JBCLASSER  *jbCorrelationInitInternal()
 *
 *     Classify the pages
 *
 *         l_int32     jbAddPages()
 *         l_int32     jbAddPage()
 *         l_int32     jbAddPageComponents()
 *
 *     Rank hausdorff classifier
 *
 *         l_int32     jbClassifyRankHaus()
 *         l_int32     pixHaustest()
 *         l_int32     pixRankHaustest()
 *
 *     Binary correlation classifier
 *
 *         l_int32     jbClassifyCorrelation()
 *
 *     Determine the image components we start with
 *
 *         l_int32     jbGetComponents()
 *         l_int32     pixWordMaskByDilation()
 *         l_int32     pixWordBoxesByDilation()
 *
 *     Build grayscale composites (templates)
 *
 *         PIXA       *jbAccumulateComposites
 *         PIXA       *jbTemplatesFromComposites
 *
 *     Utility functions for Classer
 *
 *         JBCLASSER  *jbClasserCreate()
 *         void        jbClasserDestroy()
 *
 *     Utility functions for Data
 *
 *         JBDATA     *jbDataSave()
 *         void        jbDataDestroy()
 *         l_int32     jbDataWrite()
 *         JBDATA     *jbDataRead()
 *         PIXA       *jbDataRender()
 *         l_int32     jbGetULCorners()
 *         l_int32     jbGetLLCorners()
 *
 *     Static helpers
 *
 *         static JBFINDCTX *findSimilarSizedTemplatesInit()
 *         static l_int32    findSimilarSizedTemplatesNext()
 *         static void       findSimilarSizedTemplatesDestroy()
 *         static l_int32    finalPositioningForAlignment()
 *
 *     Note: this is NOT an implementation of the JPEG jbig2
 *     proposed standard encoder, the specifications for which
 *     can be found at http://www.jpeg.org/jbigpt2.html.
 *     (See below for a full implementation.)
 *     It is an implementation of the lower-level part of an encoder that:
 *
 *        (1) identifies connected components that are going to be used
 *        (2) puts them in similarity classes (this is an unsupervised
 *            classifier), and
 *        (3) stores the result in a simple file format (2 files,
 *            one for templates and one for page/coordinate/template-index
 *            quartets).
 *
 *     An actual implementation of the official jbig2 encoder could
 *     start with parts (1) and (2), and would then compress the quartets
 *     according to the standards requirements (e.g., Huffman or
 *     arithmetic coding of coordinate differences and image templates).
 *
 *     The low-level part of the encoder provided here has the
 *     following useful features:
 *
 *         - It is accurate in the identification of templates
 *           and classes because it uses a windowed hausdorff
 *           distance metric.
 *         - It is accurate in the placement of the connected
 *           components, doing a two step process of first aligning
 *           the the centroids of the template with those of each instance,
 *           and then making a further correction of up to +- 1 pixel
 *           in each direction to best align the templates.
 *         - It is fast because it uses a morphologically based
 *           matching algorithm to implement the hausdorff criterion,
 *           and it selects the patterns that are possible matches
 *           based on their size.
 *
 *     We provide two different matching functions, one using Hausdorff
 *     distance and one using a simple image correlation.
 *     The Hausdorff method sometimes produces better results for the
 *     same number of classes, because it gives a relatively small
 *     effective weight to foreground pixels near the boundary,
 *     and a relatively  large weight to foreground pixels that are
 *     not near the boundary.  By effectively ignoring these boundary
 *     pixels, Hausdorff weighting corresponds better to the expected
 *     probabilities of the pixel values in a scanned image, where the
 *     variations in instances of the same printed character are much
 *     more likely to be in pixels near the boundary.  By contrast,
 *     the correlation method gives equal weight to all foreground pixels.
 *
 *     For best results, use the correlation method.  Correlation takes
 *     the number of fg pixels in the AND of instance and template,
 *     divided by the product of the number of fg pixels in instance
 *     and template.  It compares this with a threshold that, in
 *     general, depends on the fractional coverage of the template.
 *     For heavy text, the threshold is raised above that for light
 *     text,  By using both these parameters (basic threshold and
 *     adjustment factor for text weight), one has more flexibility
 *     and can arrive at the fewest substitution errors, although
 *     this comes at the price of more templates.
 *
 *     The strict Hausdorff scoring is not a rank weighting, because a
 *     single pixel beyond the given distance will cause a match
 *     failure.  A rank Hausdorff is more robust to non-boundary noise,
 *     but it is also more susceptible to confusing components that
 *     should be in different classes.  For implementing a jbig2
 *     application for visually lossless binary image compression,
 *     you have two choices:
 *
 *        (1) use a 3x3 structuring element (size = 3) and a strict
 *            Hausdorff comparison (rank = 1.0 in the rank Hausdorff
 *            function).  This will result in a minimal number of classes,
 *            but confusion of small characters, such as italic and
 *            non-italic lower-case 'o', can still occur.
 *        (2) use the correlation method with a threshold of 0.85
 *            and a weighting factor of about 0.7.  This will result in
 *            a larger number of classes, but should not be confused
 *            either by similar small characters or by extremely
 *            thick sans serif characters, such as in prog/cootoots.png.
 *
 *     As mentioned above, if visual substitution errors must be
 *     avoided, you should use the correlation method.
 *
 *     We provide executables that show how to do the encoding:
 *         prog/jbrankhaus.c
 *         prog/jbcorrelation.c
 *
 *     The basic flow for correlation classification goes as follows,
 *     where specific choices have been made for parameters (Hausdorff
 *     is the same except for initialization):
 *
 *             // Initialize and save data in the classer
 *         JBCLASSER *classer =
 *             jbCorrelationInit(JB_CONN_COMPS, 0, 0, 0.8, 0.7);
 *         SARRAY *safiles = getSortedPathnamesInDirectory(directory,
 *                                                         NULL, 0, 0);
 *         jbAddPages(classer, safiles);
 *
 *             // Save the data in a data structure for serialization,
 *             // and write it into two files.
 *         JBDATA *data = jbDataSave(classer);
 *         jbDataWrite(rootname, data);
 *
 *             // Reconstruct (render) the pages from the encoded data.
 *         PIXA *pixa = jbDataRender(data, FALSE);
 *
 *     Adam Langley has built a jbig2 standards-compliant encoder, the
 *     first one to appear in open source.  You can get this encoder at:
 *          http://www.imperialviolet.org/jbig2.html
 *
 *     It uses arithmetic encoding throughout.  It encodes binary images
 *     losslessly with a single arithmetic coding over the full image.
 *     It also does both lossy and lossless encoding from connected
 *     components, using leptonica to generate the templates representing
 *     each cluster.
 */

#include <string.h>
#include <math.h>
#include "allheaders.h"

static const l_int32  L_BUF_SIZE = 512;

    /* For jbClassifyRankHaus(): size of border added around
     * pix of each c.c., to allow further processing.  This
     * should be at least the sum of the MAX_DIFF_HEIGHT
     * (or MAX_DIFF_WIDTH) and one-half the size of the Sel  */
static const l_int32  JB_ADDED_PIXELS = 6;

    /* For pixHaustest(), pixRankHaustest() and pixCorrelationScore():
     * choose these to be 2 or greater */
static const l_int32  MAX_DIFF_WIDTH = 2;  /* use at least 2 */
static const l_int32  MAX_DIFF_HEIGHT = 2;  /* use at least 2 */

    /* In initialization, you have the option to discard components
     * (cc, characters or words) that have either width or height larger
     * than a given size.  This is convenient for jbDataSave(), because
     * the components are placed onto a regular lattice with cell
     * dimension equal to the maximum component size.  The default
     * values are given here.  If you want to save all components,
     * use a sufficiently large set of dimensions. */
static const l_int32  MAX_CONN_COMP_WIDTH = 350;  /* default max cc width */
static const l_int32  MAX_CHAR_COMP_WIDTH = 350;  /* default max char width */
static const l_int32  MAX_WORD_COMP_WIDTH = 1000;  /* default max word width */
static const l_int32  MAX_COMP_HEIGHT = 120;  /* default max component height */

    /* Max allowed dilation to merge characters into words */
static const l_int32  MAX_ALLOWED_DILATION = 25;

    /* This stores the state of a state machine which fetches
     * similar sized templates */
struct JbFindTemplatesState
{
    JBCLASSER       *classer;    /* classer                               */
    l_int32          w;          /* desired width                         */
    l_int32          h;          /* desired height                        */
    l_int32          i;          /* index into two_by_two step array      */
    NUMA            *numa;       /* current number array                  */
    l_int32          n;          /* current element of numa               */
};
typedef struct JbFindTemplatesState JBFINDCTX;

    /* Static initialization function */
static JBCLASSER * jbCorrelationInitInternal(l_int32 components,
                       l_int32 maxwidth, l_int32 maxheight, l_float32 thresh,
                       l_float32 weightfactor, l_int32 keep_components);

    /* Static helper functions */
static JBFINDCTX * findSimilarSizedTemplatesInit(JBCLASSER *classer, PIX *pixs);
static l_int32 findSimilarSizedTemplatesNext(JBFINDCTX *context);
static void findSimilarSizedTemplatesDestroy(JBFINDCTX **pcontext);
static l_int32 finalPositioningForAlignment(PIX *pixs, l_int32 x, l_int32 y,
                             l_int32 idelx, l_int32 idely, PIX *pixt,
                             l_int32 *sumtab, l_int32 *pdx, l_int32 *pdy);

#ifndef NO_CONSOLE_IO
#define  DEBUG_PLOT_CC             0
#define  DEBUG_CORRELATION_SCORE   0
#endif  /* ~NO_CONSOLE_IO */


/*----------------------------------------------------------------------*
 *                            Initialization                            *
 *----------------------------------------------------------------------*/
/*!
 *  jbRankHausInit()
 *
 *      Input:  components (JB_CONN_COMPS, JB_CHARACTERS, JB_WORDS)
 *              maxwidth (of component; use 0 for default)
 *              maxheight (of component; use 0 for default)
 *              size  (of square structuring element; 2, representing
 *                     2x2 sel, is necessary for reasonable accuracy of
 *                     small components; combine this with rank ~ 0.97
 *                     to avoid undue class expansion)
 *              rank (rank val of match, each way; in [0.5 - 1.0];
 *                    when using size = 2, 0.97 is a reasonable value)
 *      Return: jbclasser if OK; NULL on error
 */
JBCLASSER *
jbRankHausInit(l_int32    components,
               l_int32    maxwidth,
               l_int32    maxheight,
               l_int32    size,
               l_float32  rank)
{
JBCLASSER  *classer;

    PROCNAME("jbRankHausInit");

    if (components != JB_CONN_COMPS && components != JB_CHARACTERS &&
        components != JB_WORDS)
        return (JBCLASSER *)ERROR_PTR("invalid components", procName, NULL);
    if (size < 1 || size > 10)
        return (JBCLASSER *)ERROR_PTR("size not reasonable", procName, NULL);
    if (rank < 0.5 || rank > 1.0)
        return (JBCLASSER *)ERROR_PTR("rank not in [0.5-1.0]", procName, NULL);
    if (maxwidth == 0) {
        if (components == JB_CONN_COMPS)
            maxwidth = MAX_CONN_COMP_WIDTH;
        else if (components == JB_CHARACTERS)
            maxwidth = MAX_CHAR_COMP_WIDTH;
        else  /* JB_WORDS */
            maxwidth = MAX_WORD_COMP_WIDTH;
    }
    if (maxheight == 0)
        maxheight = MAX_COMP_HEIGHT;

    if ((classer = jbClasserCreate(JB_RANKHAUS, components)) == NULL)
        return (JBCLASSER *)ERROR_PTR("classer not made", procName, NULL);
    classer->maxwidth = maxwidth;
    classer->maxheight = maxheight;
    classer->sizehaus = size;
    classer->rankhaus = rank;
    classer->nahash = numaHashCreate(5507, 4);  /* 5507 is prime */
    return classer;
}


/*!
 *  jbCorrelationInit()
 *
 *      Input:  components (JB_CONN_COMPS, JB_CHARACTERS, JB_WORDS)
 *              maxwidth (of component; use 0 for default)
 *              maxheight (of component; use 0 for default)
 *              thresh (value for correlation score: in [0.4 - 0.98])
 *              weightfactor (corrects thresh for thick characters [0.0 - 1.0])
 *      Return: jbclasser if OK; NULL on error
 *
 *  Notes:
 *      (1) For scanned text, suggested input values are:
 *            thresh ~ [0.8 - 0.85]
 *            weightfactor ~ [0.5 - 0.6]
 *      (2) For electronically generated fonts (e.g., rasterized pdf),
 *          a very high thresh (e.g., 0.95) will not cause a significant
 *          increase in the number of classes.
 */
JBCLASSER *
jbCorrelationInit(l_int32    components,
                  l_int32    maxwidth,
                  l_int32    maxheight,
                  l_float32  thresh,
                  l_float32  weightfactor)
{
    return jbCorrelationInitInternal(components, maxwidth, maxheight, thresh,
                                     weightfactor, 1);
}

/*!
 *  jbCorrelationInitWithoutComponents()
 *
 *      Input:  same as jbCorrelationInit
 *      Output: same as jbCorrelationInit
 *
 *  Note: acts the same as jbCorrelationInit(), but the resulting
 *        object doesn't keep a list of all the components.
 */
JBCLASSER *
jbCorrelationInitWithoutComponents(l_int32    components,
                                   l_int32    maxwidth,
                                   l_int32    maxheight,
                                   l_float32  thresh,
                                   l_float32  weightfactor)
{
    return jbCorrelationInitInternal(components, maxwidth, maxheight, thresh,
                                     weightfactor, 0);
}


static JBCLASSER *
jbCorrelationInitInternal(l_int32    components,
                          l_int32    maxwidth,
                          l_int32    maxheight,
                          l_float32  thresh,
                          l_float32  weightfactor,
                          l_int32    keep_components)
{
JBCLASSER  *classer;

    PROCNAME("jbCorrelationInitInternal");

    if (components != JB_CONN_COMPS && components != JB_CHARACTERS &&
        components != JB_WORDS)
        return (JBCLASSER *)ERROR_PTR("invalid components", procName, NULL);
    if (thresh < 0.4 || thresh > 0.98)
        return (JBCLASSER *)ERROR_PTR("thresh not in range [0.4 - 0.98]",
                procName, NULL);
    if (weightfactor < 0.0 || weightfactor > 1.0)
        return (JBCLASSER *)ERROR_PTR("weightfactor not in range [0.0 - 1.0]",
                procName, NULL);
    if (maxwidth == 0) {
        if (components == JB_CONN_COMPS)
            maxwidth = MAX_CONN_COMP_WIDTH;
        else if (components == JB_CHARACTERS)
            maxwidth = MAX_CHAR_COMP_WIDTH;
        else  /* JB_WORDS */
            maxwidth = MAX_WORD_COMP_WIDTH;
    }
    if (maxheight == 0)
        maxheight = MAX_COMP_HEIGHT;


    if ((classer = jbClasserCreate(JB_CORRELATION, components)) == NULL)
        return (JBCLASSER *)ERROR_PTR("classer not made", procName, NULL);
    classer->maxwidth = maxwidth;
    classer->maxheight = maxheight;
    classer->thresh = thresh;
    classer->weightfactor = weightfactor;
    classer->nahash = numaHashCreate(5507, 4);  /* 5507 is prime */
    classer->keep_pixaa = keep_components;
    return classer;
}


/*----------------------------------------------------------------------*
 *                       Classify the pages                             *
 *----------------------------------------------------------------------*/
/*!
 *  jbAddPages()
 *
 *      Input:  jbclasser
 *              safiles (of page image file names)
 *      Return: 0 if OK; 1 on error
 *
 *  Note:
 *      (1) jbclasser makes a copy of the array of file names.
 *      (2) The caller is still responsible for destroying the input array.
 */
l_int32
jbAddPages(JBCLASSER  *classer,
           SARRAY     *safiles)
{
l_int32  i, nfiles;
char    *fname;
PIX     *pix;

    PROCNAME("jbAddPages");

    if (!classer)
        return ERROR_INT("classer not defined", procName, 1);
    if (!safiles)
        return ERROR_INT("safiles not defined", procName, 1);

    classer->safiles = sarrayCopy(safiles);
    nfiles = sarrayGetCount(safiles);
    for (i = 0; i < nfiles; i++) {
        fname = sarrayGetString(safiles, i, 0);
        if ((pix = pixRead(fname)) == NULL) {
            L_WARNING("image file %d not read\n", procName, i);
            continue;
        }
        if (pixGetDepth(pix) != 1) {
            L_WARNING("image file %d not 1 bpp\n", procName, i);
            continue;
        }
        jbAddPage(classer, pix);
        pixDestroy(&pix);
    }

    return 0;
}


/*!
 *  jbAddPage()
 *
 *      Input:  jbclasser
 *              pixs (of input page)
 *      Return: 0 if OK; 1 on error
 */
l_int32
jbAddPage(JBCLASSER  *classer,
          PIX        *pixs)
{
BOXA  *boxas;
PIXA  *pixas;

    PROCNAME("jbAddPage");

    if (!classer)
        return ERROR_INT("classer not defined", procName, 1);
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs not defined or not 1 bpp", procName, 1);

    classer->w = pixGetWidth(pixs);
    classer->h = pixGetHeight(pixs);

        /* Get the appropriate components and their bounding boxes */
    if (jbGetComponents(pixs, classer->components, classer->maxwidth,
                        classer->maxheight, &boxas, &pixas)) {
        return ERROR_INT("components not made", procName, 1);
    }

    jbAddPageComponents(classer, pixs, boxas, pixas);
    boxaDestroy(&boxas);
    pixaDestroy(&pixas);
    return 0;
}


/*!
 *  jbAddPageComponents()
 *
 *      Input:  jbclasser
 *              pixs (of input page)
 *              boxas (b.b. of components for this page)
 *              pixas (components for this page)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) If there are no components on the page, we don't require input
 *          of empty boxas or pixas, although that's the typical situation.
 */
l_int32
jbAddPageComponents(JBCLASSER  *classer,
                    PIX        *pixs,
                    BOXA       *boxas,
                    PIXA       *pixas)
{
l_int32  n;

    PROCNAME("jbAddPageComponents");

    if (!classer)
        return ERROR_INT("classer not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pix not defined", procName, 1);

        /* Test for no components on the current page.  Always update the
         * number of pages processed, even if nothing is on it. */
    if (!boxas || !pixas || (boxaGetCount(boxas) == 0)) {
        classer->npages++;
        return 0;
    }

        /* Get classes.  For hausdorff, it uses a specified size of
         * structuring element and specified rank.  For correlation,
         * it uses a specified threshold. */
    if (classer->method == JB_RANKHAUS) {
        if (jbClassifyRankHaus(classer, boxas, pixas))
            return ERROR_INT("rankhaus classification failed", procName, 1);
    } else {  /* classer->method == JB_CORRELATION */
        if (jbClassifyCorrelation(classer, boxas, pixas))
            return ERROR_INT("correlation classification failed", procName, 1);
    }

        /* Find the global UL corners, adjusted for each instance so
         * that the class template and instance will have their
         * centroids in the same place.  Then the template can be
         * used to replace the instance. */
    if (jbGetULCorners(classer, pixs, boxas))
        return ERROR_INT("UL corners not found", procName, 1);

        /* Update total component counts and number of pages processed. */
    n = boxaGetCount(boxas);
    classer->baseindex += n;
    numaAddNumber(classer->nacomps, n);
    classer->npages++;

    return 0;
}


/*----------------------------------------------------------------------*
 *         Classification using windowed rank hausdorff metric          *
 *----------------------------------------------------------------------*/
/*!
 *  jbClassifyRankHaus()
 *
 *      Input:  jbclasser
 *              boxa (of new components for classification)
 *              pixas (of new components for classification)
 *      Return: 0 if OK; 1 on error
 */
l_int32
jbClassifyRankHaus(JBCLASSER  *classer,
                   BOXA       *boxa,
                   PIXA       *pixas)
{
l_int32     n, nt, i, wt, ht, iclass, size, found, testval;
l_int32    *sumtab;
l_int32     npages, area1, area3;
l_int32    *tab8;
l_float32   rank, x1, y1, x2, y2;
BOX        *box;
NUMA       *naclass, *napage;
NUMA       *nafg;   /* fg area of all instances */
NUMA       *nafgt;  /* fg area of all templates */
JBFINDCTX  *findcontext;
NUMAHASH   *nahash;
PIX        *pix, *pix1, *pix2, *pix3, *pix4;
PIXA       *pixa, *pixa1, *pixa2, *pixat, *pixatd;
PIXAA      *pixaa;
PTA        *pta, *ptac, *ptact;
SEL        *sel;

    PROCNAME("jbClassifyRankHaus");

    if (!classer)
        return ERROR_INT("classer not found", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not found", procName, 1);
    if (!pixas)
        return ERROR_INT("pixas not found", procName, 1);

    npages = classer->npages;
    size = classer->sizehaus;
    sel = selCreateBrick(size, size, size / 2, size / 2, SEL_HIT);

        /* Generate the bordered pixa, with and without dilation.
         * pixa1 and pixa2 contain all the input components. */
    n = pixaGetCount(pixas);
    pixa1 = pixaCreate(n);
    pixa2 = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixas, i, L_CLONE);
        pix1 = pixAddBorderGeneral(pix, JB_ADDED_PIXELS, JB_ADDED_PIXELS,
                    JB_ADDED_PIXELS, JB_ADDED_PIXELS, 0);
        pix2 = pixDilate(NULL, pix1, sel);
        pixaAddPix(pixa1, pix1, L_INSERT);   /* un-dilated */
        pixaAddPix(pixa2, pix2, L_INSERT);   /* dilated */
        pixDestroy(&pix);
    }

        /* Get the centroids of all the bordered images.
         * These are relative to the UL corner of each (bordered) pix.  */
    pta = pixaCentroids(pixa1);  /* centroids for this page; use here */
    ptac = classer->ptac;  /* holds centroids of components up to this page */
    ptaJoin(ptac, pta, 0, -1);  /* save centroids of all components */
    ptact = classer->ptact;  /* holds centroids of templates */

        /* Use these to save the class and page of each component. */
    naclass = classer->naclass;
    napage = classer->napage;
    sumtab = makePixelSumTab8();

        /* Store the unbordered pix in a pixaa, in a hierarchical
         * set of arrays.  There is one pixa for each class,
         * and the pix in each pixa are all the instances found
         * of that class.  This is actually more than one would need
         * for a jbig2 encoder, but there are two reasons to keep
         * them around: (1) the set of instances for each class
         * can be used to make an improved binary (or, better,
         * a grayscale) template, rather than simply using the first
         * one in the set; (2) we can investigate the failures
         * of the classifier.  This pixaa grows as we process
         * successive pages. */
    pixaa = classer->pixaa;

        /* arrays to store class exemplars (templates) */
    pixat = classer->pixat;   /* un-dilated */
    pixatd = classer->pixatd;   /* dilated */

        /* Fill up the pixaa tree with the template exemplars as
         * the first pix in each pixa.  As we add each pix,
         * we also add the associated box to the pixa.
         * We also keep track of the centroid of each pix,
         * and use the difference between centroids (of the
         * pix with the exemplar we are checking it with)
         * to align the two when checking that the Hausdorff
         * distance does not exceed a threshold.
         * The threshold is set by the Sel used for dilating.
         * For example, a 3x3 brick, sel_3, corresponds to a
         * Hausdorff distance of 1.  In general, for an NxN brick,
         * with N odd, corresponds to a Hausdorff distance of (N - 1)/2.
         * It turns out that we actually need to use a sel of size 2x2
         * to avoid small bad components when there is a halftone image
         * from which components can be chosen.
         * The larger the Sel you use, the fewer the number of classes,
         * and the greater the likelihood of putting semantically
         * different objects in the same class.  For simplicity,
         * we do this separately for the case of rank == 1.0 (exact
         * match within the Hausdorff distance) and rank < 1.0.  */
    rank = classer->rankhaus;
    nahash = classer->nahash;
    if (rank == 1.0) {
        for (i = 0; i < n; i++) {
            pix1 = pixaGetPix(pixa1, i, L_CLONE);
            pix2 = pixaGetPix(pixa2, i, L_CLONE);
            ptaGetPt(pta, i, &x1, &y1);
            nt = pixaGetCount(pixat);  /* number of templates */
            found = FALSE;
            findcontext = findSimilarSizedTemplatesInit(classer, pix1);
            while ((iclass = findSimilarSizedTemplatesNext(findcontext)) > -1) {
                    /* Find score for this template */
                pix3 = pixaGetPix(pixat, iclass, L_CLONE);
                pix4 = pixaGetPix(pixatd, iclass, L_CLONE);
                ptaGetPt(ptact, iclass, &x2, &y2);
                testval = pixHaustest(pix1, pix2, pix3, pix4, x1 - x2, y1 - y2,
                                      MAX_DIFF_WIDTH, MAX_DIFF_HEIGHT);
                pixDestroy(&pix3);
                pixDestroy(&pix4);
                if (testval == 1) {
                    found = TRUE;
                    numaAddNumber(naclass, iclass);
                    numaAddNumber(napage, npages);
                    if (classer->keep_pixaa) {
                        pixa = pixaaGetPixa(pixaa, iclass, L_CLONE);
                        pix = pixaGetPix(pixas, i, L_CLONE);
                        pixaAddPix(pixa, pix, L_INSERT);
                        box = boxaGetBox(boxa, i, L_CLONE);
                        pixaAddBox(pixa, box, L_INSERT);
                        pixaDestroy(&pixa);
                    }
                    break;
                }
            }
            findSimilarSizedTemplatesDestroy(&findcontext);
            if (found == FALSE) {  /* new class */
                numaAddNumber(naclass, nt);
                numaAddNumber(napage, npages);
                pixa = pixaCreate(0);
                pix = pixaGetPix(pixas, i, L_CLONE);  /* unbordered instance */
                pixaAddPix(pixa, pix, L_INSERT);
                wt = pixGetWidth(pix);
                ht = pixGetHeight(pix);
                numaHashAdd(nahash, ht * wt, nt);
                box = boxaGetBox(boxa, i, L_CLONE);
                pixaAddBox(pixa, box, L_INSERT);
                pixaaAddPixa(pixaa, pixa, L_INSERT);  /* unbordered instance */
                ptaAddPt(ptact, x1, y1);
                pixaAddPix(pixat, pix1, L_INSERT);  /* bordered template */
                pixaAddPix(pixatd, pix2, L_INSERT);  /* bordered dil template */
            } else {  /* don't save them */
                pixDestroy(&pix1);
                pixDestroy(&pix2);
            }
        }
    } else {  /* rank < 1.0 */
        if ((nafg = pixaCountPixels(pixas)) == NULL)  /* areas for this page */
            return ERROR_INT("nafg not made", procName, 1);
        nafgt = classer->nafgt;
        tab8 = makePixelSumTab8();
        for (i = 0; i < n; i++) {   /* all instances on this page */
            pix1 = pixaGetPix(pixa1, i, L_CLONE);
            numaGetIValue(nafg, i, &area1);
            pix2 = pixaGetPix(pixa2, i, L_CLONE);
            ptaGetPt(pta, i, &x1, &y1);   /* use pta for this page */
            nt = pixaGetCount(pixat);  /* number of templates */
            found = FALSE;
            findcontext = findSimilarSizedTemplatesInit(classer, pix1);
            while ((iclass = findSimilarSizedTemplatesNext(findcontext)) > -1) {
                    /* Find score for this template */
                pix3 = pixaGetPix(pixat, iclass, L_CLONE);
                numaGetIValue(nafgt, iclass, &area3);
                pix4 = pixaGetPix(pixatd, iclass, L_CLONE);
                ptaGetPt(ptact, iclass, &x2, &y2);
                testval = pixRankHaustest(pix1, pix2, pix3, pix4,
                                          x1 - x2, y1 - y2,
                                          MAX_DIFF_WIDTH, MAX_DIFF_HEIGHT,
                                          area1, area3, rank, tab8);
                pixDestroy(&pix3);
                pixDestroy(&pix4);
                if (testval == 1) {  /* greedy match; take the first */
                    found = TRUE;
                    numaAddNumber(naclass, iclass);
                    numaAddNumber(napage, npages);
                    if (classer->keep_pixaa) {
                        pixa = pixaaGetPixa(pixaa, iclass, L_CLONE);
                        pix = pixaGetPix(pixas, i, L_CLONE);
                        pixaAddPix(pixa, pix, L_INSERT);
                        box = boxaGetBox(boxa, i, L_CLONE);
                        pixaAddBox(pixa, box, L_INSERT);
                        pixaDestroy(&pixa);
                    }
                    break;
                }
            }
            findSimilarSizedTemplatesDestroy(&findcontext);
            if (found == FALSE) {  /* new class */
                numaAddNumber(naclass, nt);
                numaAddNumber(napage, npages);
                pixa = pixaCreate(0);
                pix = pixaGetPix(pixas, i, L_CLONE);  /* unbordered instance */
                pixaAddPix(pixa, pix, L_INSERT);
                wt = pixGetWidth(pix);
                ht = pixGetHeight(pix);
                numaHashAdd(nahash, ht * wt, nt);
                box = boxaGetBox(boxa, i, L_CLONE);
                pixaAddBox(pixa, box, L_INSERT);
                pixaaAddPixa(pixaa, pixa, L_INSERT);  /* unbordered instance */
                ptaAddPt(ptact, x1, y1);
                pixaAddPix(pixat, pix1, L_INSERT);  /* bordered template */
                pixaAddPix(pixatd, pix2, L_INSERT);  /* ditto */
                numaAddNumber(nafgt, area1);
            } else {  /* don't save them */
                pixDestroy(&pix1);
                pixDestroy(&pix2);
            }
        }
        FREE(tab8);
        numaDestroy(&nafg);
    }
    classer->nclass = pixaGetCount(pixat);

    FREE(sumtab);
    ptaDestroy(&pta);
    pixaDestroy(&pixa1);
    pixaDestroy(&pixa2);
    selDestroy(&sel);
    return 0;
}


/*!
 *  pixHaustest()
 *
 *      Input:  pix1   (new pix, not dilated)
 *              pix2   (new pix, dilated)
 *              pix3   (exemplar pix, not dilated)
 *              pix4   (exemplar pix, dilated)
 *              delx   (x comp of centroid difference)
 *              dely   (y comp of centroid difference)
 *              maxdiffw (max width difference of pix1 and pix2)
 *              maxdiffh (max height difference of pix1 and pix2)
 *      Return: 0 (FALSE) if no match, 1 (TRUE) if the new
 *              pix is in the same class as the exemplar.
 *
 *  Note: we check first that the two pix are roughly
 *  the same size.  Only if they meet that criterion do
 *  we compare the bitmaps.  The Hausdorff is a 2-way
 *  check.  The centroid difference is used to align the two
 *  images to the nearest integer for each of the checks.
 *  These check that the dilated image of one contains
 *  ALL the pixels of the undilated image of the other.
 *  Checks are done in both direction.  A single pixel not
 *  contained in either direction results in failure of the test.
 */
l_int32
pixHaustest(PIX       *pix1,
            PIX       *pix2,
            PIX       *pix3,
            PIX       *pix4,
            l_float32  delx,   /* x(1) - x(3) */
            l_float32  dely,   /* y(1) - y(3) */
            l_int32    maxdiffw,
            l_int32    maxdiffh)
{
l_int32  wi, hi, wt, ht, delw, delh, idelx, idely, boolmatch;
PIX     *pixt;

        /* Eliminate possible matches based on size difference */
    wi = pixGetWidth(pix1);
    hi = pixGetHeight(pix1);
    wt = pixGetWidth(pix3);
    ht = pixGetHeight(pix3);
    delw = L_ABS(wi - wt);
    if (delw > maxdiffw)
        return FALSE;
    delh = L_ABS(hi - ht);
    if (delh > maxdiffh)
        return FALSE;

        /* Round difference in centroid location to nearest integer;
         * use this as a shift when doing the matching. */
    if (delx >= 0)
        idelx = (l_int32)(delx + 0.5);
    else
        idelx = (l_int32)(delx - 0.5);
    if (dely >= 0)
        idely = (l_int32)(dely + 0.5);
    else
        idely = (l_int32)(dely - 0.5);

        /*  Do 1-direction hausdorff, checking that every pixel in pix1
         *  is within a dilation distance of some pixel in pix3.  Namely,
         *  that pix4 entirely covers pix1:
         *       pixt = pixSubtract(NULL, pix1, pix4), including shift
         *  where pixt has no ON pixels.  */
    pixt = pixCreateTemplate(pix1);
    pixRasterop(pixt, 0, 0, wi, hi, PIX_SRC, pix1, 0, 0);
    pixRasterop(pixt, idelx, idely, wi, hi, PIX_DST & PIX_NOT(PIX_SRC),
                pix4, 0, 0);
    pixZero(pixt, &boolmatch);
    if (boolmatch == 0) {
        pixDestroy(&pixt);
        return FALSE;
    }

        /*  Do 1-direction hausdorff, checking that every pixel in pix3
         *  is within a dilation distance of some pixel in pix1.  Namely,
         *  that pix2 entirely covers pix3:
         *      pixSubtract(pixt, pix3, pix2), including shift
         *  where pixt has no ON pixels. */
    pixRasterop(pixt, idelx, idely, wt, ht, PIX_SRC, pix3, 0, 0);
    pixRasterop(pixt, 0, 0, wt, ht, PIX_DST & PIX_NOT(PIX_SRC), pix2, 0, 0);
    pixZero(pixt, &boolmatch);
    pixDestroy(&pixt);
    return boolmatch;
}


/*!
 *  pixRankHaustest()
 *
 *      Input:  pix1   (new pix, not dilated)
 *              pix2   (new pix, dilated)
 *              pix3   (exemplar pix, not dilated)
 *              pix4   (exemplar pix, dilated)
 *              delx   (x comp of centroid difference)
 *              dely   (y comp of centroid difference)
 *              maxdiffw (max width difference of pix1 and pix2)
 *              maxdiffh (max height difference of pix1 and pix2)
 *              area1  (fg pixels in pix1)
 *              area3  (fg pixels in pix3)
 *              rank   (rank value of test, each way)
 *              tab8   (table of pixel sums for byte)
 *      Return: 0 (FALSE) if no match, 1 (TRUE) if the new
 *                 pix is in the same class as the exemplar.
 *
 *  Note: we check first that the two pix are roughly
 *  the same size.  Only if they meet that criterion do
 *  we compare the bitmaps.  We convert the rank value to
 *  a number of pixels by multiplying the rank fraction by the number
 *  of pixels in the undilated image.  The Hausdorff is a 2-way
 *  check.  The centroid difference is used to align the two
 *  images to the nearest integer for each of the checks.
 *  The rank hausdorff checks that the dilated image of one
 *  contains the rank fraction of the pixels of the undilated
 *  image of the other.   Checks are done in both direction.
 *  Failure of the test in either direction results in failure
 *  of the test.
 */
l_int32
pixRankHaustest(PIX       *pix1,
                PIX       *pix2,
                PIX       *pix3,
                PIX       *pix4,
                l_float32  delx,   /* x(1) - x(3) */
                l_float32  dely,   /* y(1) - y(3) */
                l_int32    maxdiffw,
                l_int32    maxdiffh,
                l_int32    area1,
                l_int32    area3,
                l_float32  rank,
                l_int32   *tab8)
{
l_int32  wi, hi, wt, ht, delw, delh, idelx, idely, boolmatch;
l_int32  thresh1, thresh3;
PIX     *pixt;

        /* Eliminate possible matches based on size difference */
    wi = pixGetWidth(pix1);
    hi = pixGetHeight(pix1);
    wt = pixGetWidth(pix3);
    ht = pixGetHeight(pix3);
    delw = L_ABS(wi - wt);
    if (delw > maxdiffw)
        return FALSE;
    delh = L_ABS(hi - ht);
    if (delh > maxdiffh)
        return FALSE;

        /* Upper bounds in remaining pixels for allowable match */
    thresh1 = (l_int32)(area1 * (1. - rank) + 0.5);
    thresh3 = (l_int32)(area3 * (1. - rank) + 0.5);

        /* Round difference in centroid location to nearest integer;
         * use this as a shift when doing the matching. */
    if (delx >= 0)
        idelx = (l_int32)(delx + 0.5);
    else
        idelx = (l_int32)(delx - 0.5);
    if (dely >= 0)
        idely = (l_int32)(dely + 0.5);
    else
        idely = (l_int32)(dely - 0.5);

        /*  Do 1-direction hausdorff, checking that every pixel in pix1
         *  is within a dilation distance of some pixel in pix3.  Namely,
         *  that pix4 entirely covers pix1:
         *       pixt = pixSubtract(NULL, pix1, pix4), including shift
         *  where pixt has no ON pixels.  */
    pixt = pixCreateTemplate(pix1);
    pixRasterop(pixt, 0, 0, wi, hi, PIX_SRC, pix1, 0, 0);
    pixRasterop(pixt, idelx, idely, wi, hi, PIX_DST & PIX_NOT(PIX_SRC),
                pix4, 0, 0);
    pixThresholdPixelSum(pixt, thresh1, &boolmatch, tab8);
    if (boolmatch == 1) { /* above thresh1 */
        pixDestroy(&pixt);
        return FALSE;
    }

        /*  Do 1-direction hausdorff, checking that every pixel in pix3
         *  is within a dilation distance of some pixel in pix1.  Namely,
         *  that pix2 entirely covers pix3:
         *      pixSubtract(pixt, pix3, pix2), including shift
         *  where pixt has no ON pixels. */
    pixRasterop(pixt, idelx, idely, wt, ht, PIX_SRC, pix3, 0, 0);
    pixRasterop(pixt, 0, 0, wt, ht, PIX_DST & PIX_NOT(PIX_SRC), pix2, 0, 0);
    pixThresholdPixelSum(pixt, thresh3, &boolmatch, tab8);
    pixDestroy(&pixt);
    if (boolmatch == 1)  /* above thresh3 */
        return FALSE;
    else
        return TRUE;
}


/*----------------------------------------------------------------------*
 *            Classification using windowed correlation score           *
 *----------------------------------------------------------------------*/
/*!
 *  jbClassifyCorrelation()
 *
 *      Input:  jbclasser
 *              boxa (of new components for classification)
 *              pixas (of new components for classification)
 *      Return: 0 if OK; 1 on error
 */
l_int32
jbClassifyCorrelation(JBCLASSER  *classer,
                      BOXA       *boxa,
                      PIXA       *pixas)
{
l_int32     n, nt, i, iclass, wt, ht, found, area, area1, area2, npages,
            overthreshold;
l_int32    *sumtab, *centtab;
l_uint32   *row, word;
l_float32   x1, y1, x2, y2, xsum, ysum;
l_float32   thresh, weight, threshold;
BOX        *box;
NUMA       *naclass, *napage;
NUMA       *nafgt;   /* fg area of all templates */
NUMA       *naarea;   /* w * h area of all templates */
JBFINDCTX  *findcontext;
NUMAHASH   *nahash;
PIX        *pix, *pix1, *pix2;
PIXA       *pixa, *pixa1, *pixat;
PIXAA      *pixaa;
PTA        *pta, *ptac, *ptact;
l_int32    *pixcts;  /* pixel counts of each pixa */
l_int32   **pixrowcts;  /* row-by-row pixel counts of each pixa */
l_int32     x, y, rowcount, downcount, wpl;
l_uint8     byte;

    PROCNAME("jbClassifyCorrelation");

    if (!classer)
        return ERROR_INT("classer not found", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not found", procName, 1);
    if (!pixas)
        return ERROR_INT("pixas not found", procName, 1);

    npages = classer->npages;

        /* Generate the bordered pixa, which contains all the the
         * input components.  This will not be saved.   */
    n = pixaGetCount(pixas);
    pixa1 = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixas, i, L_CLONE);
        pix1 = pixAddBorderGeneral(pix, JB_ADDED_PIXELS, JB_ADDED_PIXELS,
                    JB_ADDED_PIXELS, JB_ADDED_PIXELS, 0);
        pixaAddPix(pixa1, pix1, L_INSERT);
        pixDestroy(&pix);
    }

        /* Use these to save the class and page of each component. */
    naclass = classer->naclass;
    napage = classer->napage;

        /* Get the number of fg pixels in each component.  */
    nafgt = classer->nafgt;    /* holds fg areas of the templates */
    sumtab = makePixelSumTab8();

    pixcts = (l_int32 *)CALLOC(n, sizeof(*pixcts));
    pixrowcts = (l_int32 **)CALLOC(n, sizeof(*pixrowcts));
    centtab = makePixelCentroidTab8();
    if (!pixcts || !pixrowcts || !centtab)
        return ERROR_INT("calloc fail in pix*cts or centtab", procName, 1);

        /* Count the "1" pixels in each row of the pix in pixa1; this
         * allows pixCorrelationScoreThresholded to abort early if a match
         * is impossible.  This loop merges three calculations: the total
         * number of "1" pixels, the number of "1" pixels in each row, and
         * the centroid.  The centroids are relative to the UL corner of
         * each (bordered) pix.  The pixrowcts[i][y] are the total number
         * of fg pixels in pixa[i] below row y. */
    pta = ptaCreate(n);
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixa1, i, L_CLONE);
        pixrowcts[i] = (l_int32 *)CALLOC(pixGetHeight(pix),
                                         sizeof(**pixrowcts));
        xsum = 0;
        ysum = 0;
        wpl = pixGetWpl(pix);
        row = pixGetData(pix) + (pixGetHeight(pix) - 1) * wpl;
        downcount = 0;
        for (y = pixGetHeight(pix) - 1; y >= 0; y--, row -= wpl) {
            pixrowcts[i][y] = downcount;
            rowcount = 0;
            for (x = 0; x < wpl; x++) {
                word = row[x];
                byte = word & 0xff;
                rowcount += sumtab[byte];
                xsum += centtab[byte] + (x * 32 + 24) * sumtab[byte];
                byte = (word >> 8) & 0xff;
                rowcount += sumtab[byte];
                xsum += centtab[byte] + (x * 32 + 16) * sumtab[byte];
                byte = (word >> 16) & 0xff;
                rowcount += sumtab[byte];
                xsum += centtab[byte] + (x * 32 + 8) * sumtab[byte];
                byte = (word >> 24) & 0xff;
                rowcount += sumtab[byte];
                xsum += centtab[byte] + x * 32 * sumtab[byte];
            }
            downcount += rowcount;
            ysum += rowcount * y;
        }
        pixcts[i] = downcount;
        ptaAddPt(pta,
                 xsum / (l_float32)downcount, ysum / (l_float32)downcount);
        pixDestroy(&pix);
    }

    ptac = classer->ptac;  /* holds centroids of components up to this page */
    ptaJoin(ptac, pta, 0, -1);  /* save centroids of all components */
    ptact = classer->ptact;  /* holds centroids of templates */

    /* Store the unbordered pix in a pixaa, in a hierarchical
     * set of arrays.  There is one pixa for each class,
     * and the pix in each pixa are all the instances found
     * of that class.  This is actually more than one would need
     * for a jbig2 encoder, but there are two reasons to keep
     * them around: (1) the set of instances for each class
     * can be used to make an improved binary (or, better,
     * a grayscale) template, rather than simply using the first
     * one in the set; (2) we can investigate the failures
     * of the classifier.  This pixaa grows as we process
     * successive pages. */
    pixaa = classer->pixaa;

        /* Array to store class exemplars */
    pixat = classer->pixat;

        /* Fill up the pixaa tree with the template exemplars as
         * the first pix in each pixa.  As we add each pix,
         * we also add the associated box to the pixa.
         * We also keep track of the centroid of each pix,
         * and use the difference between centroids (of the
         * pix with the exemplar we are checking it with)
         * to align the two when checking that the correlation
         * score exceeds a threshold.  The correlation score
         * is given by the square of the area of the AND
         * between aligned instance and template, divided by
         * the product of areas of each image.  For identical
         * template and instance, the score is 1.0.
         * If the threshold is too small, non-equivalent instances
         * will be placed in the same class; if too large, there will
         * be an unnecessary division of classes representing the
         * same character.  The weightfactor adds in some of the
         * difference (1.0 - thresh), depending on the heaviness
         * of the template (measured as the fraction of fg pixels). */
    thresh = classer->thresh;
    weight = classer->weightfactor;
    naarea = classer->naarea;
    nahash = classer->nahash;
    for (i = 0; i < n; i++) {
        pix1 = pixaGetPix(pixa1, i, L_CLONE);
        area1 = pixcts[i];
        ptaGetPt(pta, i, &x1, &y1);  /* centroid for this instance */
        nt = pixaGetCount(pixat);
        found = FALSE;
        findcontext = findSimilarSizedTemplatesInit(classer, pix1);
        while ( (iclass = findSimilarSizedTemplatesNext(findcontext)) > -1) {
                /* Get the template */
            pix2 = pixaGetPix(pixat, iclass, L_CLONE);
            numaGetIValue(nafgt, iclass, &area2);
            ptaGetPt(ptact, iclass, &x2, &y2);  /* template centroid */

                /* Find threshold for this template */
            if (weight > 0.0) {
                numaGetIValue(naarea, iclass, &area);
                threshold = thresh + (1. - thresh) * weight * area2 / area;
            } else {
                threshold = thresh;
            }

                /* Find score for this template */
            overthreshold = pixCorrelationScoreThresholded(pix1, pix2,
                                         area1, area2, x1 - x2, y1 - y2,
                                         MAX_DIFF_WIDTH, MAX_DIFF_HEIGHT,
                                         sumtab, pixrowcts[i], threshold);
#if DEBUG_CORRELATION_SCORE
            {
                l_float32 score, testscore;
                l_int32 count, testcount;
                pixCorrelationScore(pix1, pix2, area1, area2, x1 - x2, y1 - y2,
                                    MAX_DIFF_WIDTH, MAX_DIFF_HEIGHT,
                                    sumtab, &score);

                pixCorrelationScoreSimple(pix1, pix2, area1, area2,
                                          x1 - x2, y1 - y2, MAX_DIFF_WIDTH,
                                          MAX_DIFF_HEIGHT, sumtab, &testscore);
                count = (l_int32)rint(sqrt(score * area1 * area2));
                testcount = (l_int32)rint(sqrt(testscore * area1 * area2));
                if ((score >= threshold) != (testscore >= threshold)) {
                    fprintf(stderr, "Correlation score mismatch: %d(%g,%d) vs %d(%g,%d) (%g)\n",
                            count, score, score >= threshold,
                            testcount, testscore, testscore >= threshold,
                            score - testscore);
                }

                if ((score >= threshold) != overthreshold) {
                    fprintf(stderr, "Mismatch between correlation/threshold comparison: %g(%g,%d) >= %g(%g) vs %s\n",
                            score, score*area1*area2, count, threshold, threshold*area1*area2, (overthreshold ? "true" : "false"));
                }
            }
#endif  /* DEBUG_CORRELATION_SCORE */
            pixDestroy(&pix2);

            if (overthreshold) {  /* greedy match */
                found = TRUE;
                numaAddNumber(naclass, iclass);
                numaAddNumber(napage, npages);
                if (classer->keep_pixaa) {
                        /* We are keeping a record of all components */
                    pixa = pixaaGetPixa(pixaa, iclass, L_CLONE);
                    pix = pixaGetPix(pixas, i, L_CLONE);
                    pixaAddPix(pixa, pix, L_INSERT);
                    box = boxaGetBox(boxa, i, L_CLONE);
                    pixaAddBox(pixa, box, L_INSERT);
                    pixaDestroy(&pixa);
                }
                break;
            }
        }
        findSimilarSizedTemplatesDestroy(&findcontext);
        if (found == FALSE) {  /* new class */
            numaAddNumber(naclass, nt);
            numaAddNumber(napage, npages);
            pixa = pixaCreate(0);
            pix = pixaGetPix(pixas, i, L_CLONE);  /* unbordered instance */
            pixaAddPix(pixa, pix, L_INSERT);
            wt = pixGetWidth(pix);
            ht = pixGetHeight(pix);
            numaHashAdd(nahash, ht * wt, nt);
            box = boxaGetBox(boxa, i, L_CLONE);
            pixaAddBox(pixa, box, L_INSERT);
            pixaaAddPixa(pixaa, pixa, L_INSERT);  /* unbordered instance */
            ptaAddPt(ptact, x1, y1);
            numaAddNumber(nafgt, area1);
            pixaAddPix(pixat, pix1, L_INSERT);   /* bordered template */
            area = (pixGetWidth(pix1) - 2 * JB_ADDED_PIXELS) *
                   (pixGetHeight(pix1) - 2 * JB_ADDED_PIXELS);
            numaAddNumber(naarea, area);
        } else {  /* don't save it */
            pixDestroy(&pix1);
        }
    }
    classer->nclass = pixaGetCount(pixat);

    FREE(pixcts);
    FREE(centtab);
    for (i = 0; i < n; i++) {
        FREE(pixrowcts[i]);
    }
    FREE(pixrowcts);

    FREE(sumtab);
    ptaDestroy(&pta);
    pixaDestroy(&pixa1);
    return 0;
}


/*----------------------------------------------------------------------*
 *             Determine the image components we start with             *
 *----------------------------------------------------------------------*/
/*!
 *  jbGetComponents()
 *
 *      Input:  pixs (1 bpp)
 *              components (JB_CONN_COMPS, JB_CHARACTERS, JB_WORDS)
 *              maxwidth, maxheight (of saved components; larger are discarded)
 *              &pboxa (<return> b.b. of component items)
 *              &ppixa (<return> component items)
 *      Return: 0 if OK, 1 on error
 */
l_int32
jbGetComponents(PIX     *pixs,
                l_int32  components,
                l_int32  maxwidth,
                l_int32  maxheight,
                BOXA   **pboxad,
                PIXA   **ppixad)
{
l_int32    empty, res, redfactor;
BOXA      *boxa;
PIX       *pixt1, *pixt2, *pixt3;
PIXA      *pixa, *pixat;

    PROCNAME("jbGetComponents");

    if (!pboxad)
        return ERROR_INT("&boxad not defined", procName, 1);
    *pboxad = NULL;
    if (!ppixad)
        return ERROR_INT("&pixad not defined", procName, 1);
    *ppixad = NULL;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (components != JB_CONN_COMPS && components != JB_CHARACTERS &&
        components != JB_WORDS)
        return ERROR_INT("invalid components", procName, 1);

    pixZero(pixs, &empty);
    if (empty) {
        *pboxad = boxaCreate(0);
        *ppixad = pixaCreate(0);
        return 0;
    }

        /* If required, preprocess input pixs.  The method for both
         * characters and words is to generate a connected component
         * mask over the units that we want to aggregrate, which are,
         * in general, sets of related connected components in pixs.
         * For characters, we want to include the dots with
         * 'i', 'j' and '!', so we do a small vertical closing to
         * generate the mask.  For words, we make a mask over all
         * characters in each word.  This is a bit more tricky, because
         * the spacing between words is difficult to predict a priori,
         * and words can be typeset with variable spacing that can
         * in some cases be barely larger than the space between
         * characters.  The first step is to generate the mask and
         * identify each of its connected components.  */
    if (components == JB_CONN_COMPS) {  /* no preprocessing */
        boxa = pixConnComp(pixs, &pixa, 8);
    } else if (components == JB_CHARACTERS) {
        pixt1 = pixMorphSequence(pixs, "c1.6", 0);
        boxa = pixConnComp(pixt1, &pixat, 8);
        pixa = pixaClipToPix(pixat, pixs);
        pixDestroy(&pixt1);
        pixaDestroy(&pixat);
    } else {  /* components == JB_WORDS */

            /* Do the operations at about 150 ppi resolution.
             * It is much faster at 75 ppi, but the results are
             * more accurate at 150 ppi.  This will segment the
             * words in body text.  It can be expected that relatively
             * infrequent words in a larger font will be split. */
        res = pixGetXRes(pixs);
        if (res <= 200) {
            redfactor = 1;
            pixt1 = pixClone(pixs);
        } else if (res <= 400) {
            redfactor = 2;
            pixt1 = pixReduceRankBinaryCascade(pixs, 1, 0, 0, 0);
        } else {
            redfactor = 4;
            pixt1 = pixReduceRankBinaryCascade(pixs, 1, 1, 0, 0);
        }

            /* Estimate the word mask, at aproximately 150 ppi.
             * This has both very large and very small components left in. */
        pixWordMaskByDilation(pixt1, 8, &pixt2, NULL);

            /* Expand the optimally dilated word mask to full res. */
        pixt3 = pixExpandReplicate(pixt2, redfactor);

            /* Pull out the pixels in pixs corresponding to the mask
             * components in pixt3.  Note that above we used threshold
             * levels in the reduction of 1 to insure that the resulting
             * mask fully covers the input pixs.  The downside of using
             * a threshold of 1 is that very close characters from adjacent
             * lines can be joined.  But with a level of 2 or greater,
             * it is necessary to use a seedfill, followed by a pixOr():
             *       pixt4 = pixSeedfillBinary(NULL, pixt3, pixs, 8);
             *       pixOr(pixt3, pixt3, pixt4);
             * to insure that the mask coverage is complete over pixs.  */
        boxa = pixConnComp(pixt3, &pixat, 4);
        pixa = pixaClipToPix(pixat, pixs);
        pixaDestroy(&pixat);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixt3);
    }

        /* Remove large components, and save the results.  */
    *ppixad = pixaSelectBySize(pixa, maxwidth, maxheight, L_SELECT_IF_BOTH,
                               L_SELECT_IF_LTE, NULL);
    *pboxad = boxaSelectBySize(boxa, maxwidth, maxheight, L_SELECT_IF_BOTH,
                               L_SELECT_IF_LTE, NULL);
    pixaDestroy(&pixa);
    boxaDestroy(&boxa);

    return 0;
}


/*!
 *  pixWordMaskByDilation()
 *
 *      Input:  pixs (1 bpp; typ. at 75 to 150 ppi)
 *              maxdil (maximum dilation; 0 for default; warning if > 20)
 *              &mask (<optional return> dilated word mask)
 *              &size (<optional return> size of optimal horiz Sel)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This gives a crude estimate of the word masks.  See
 *          pixWordBoxesByDilation() for further filtering of the word boxes.
 *      (2) For 75 to 150 ppi, the optimal dilation will be between 5 and 11.
 *          For 200 to 300 ppi, it is advisable to use a larger value
 *          for @maxdil, say between 10 and 20.  Setting maxdil <= 0
 *          results in a default dilation of 16.
 *      (3) The best size for dilating to get word masks is optionally returned.
 */
l_int32
pixWordMaskByDilation(PIX      *pixs,
                      l_int32   maxdil,
                      PIX     **ppixm,
                      l_int32  *psize)
{
l_int32  i, diffmin, ndiff, imin;
l_int32  ncc[MAX_ALLOWED_DILATION + 1];
BOXA    *boxa;
NUMA    *nacc, *nadiff;
PIX     *pix1, *pix2;

    PROCNAME("pixWordMaskByDilation");

    if (ppixm) *ppixm = NULL;
    if (psize) *psize = 0;
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs undefined or not 1 bpp", procName, 1);
    if (!ppixm && !psize)
        return ERROR_INT("no output requested", procName, 1);

        /* Find the optimal dilation to create the word mask.
         * Look for successively increasing dilations where the
         * number of connected components doesn't decrease.
         * This is the situation where the components in the
         * word mask should properly cover each word.  If the
         * input image had been 2x scaled, and you use 8 cc for
         * counting, every other differential count in the series
         * will be 0.  We avoid this possibility by using 4 cc. */
    diffmin = 1000000;
    pix1 = pixCopy(NULL, pixs);
    if (maxdil <= 0)
        maxdil = 16;  /* default for 200 to 300 ppi */
    maxdil = L_MIN(maxdil, MAX_ALLOWED_DILATION);
    if (maxdil > 20)
        L_WARNING("large dilation: exceeds 20\n", procName);
    nacc = numaCreate(maxdil + 1);
    nadiff = numaCreate(maxdil + 1);
    for (i = 0; i <= maxdil; i++) {
        if (i == 0)  /* first one not dilated */
            pix2 = pixCopy(NULL, pix1);
        else  /* successive dilation by sel_2h */
            pix2 = pixMorphSequence(pix1, "d2.1", 0);
        boxa = pixConnCompBB(pix2, 4);
        ncc[i] = boxaGetCount(boxa);
        numaAddNumber(nacc, ncc[i]);
        if (i > 0) {
            ndiff = ncc[i - 1] - ncc[i];
            numaAddNumber(nadiff, ndiff);
#if  DEBUG_PLOT_CC
            fprintf(stderr, "ndiff[%d] = %d\n", i - 1, ndiff);
#endif  /* DEBUG_PLOT_CC */
                /* Don't allow imin <= 2 with a 0 value of ndiff,
                 * which is unlikely to happen.  */
            if (ndiff < diffmin && (ndiff > 0 || i > 2)) {
                imin = i;
                diffmin = ndiff;
            }
        }
        pixDestroy(&pix1);
        pix1 = pix2;
        boxaDestroy(&boxa);
    }
    pixDestroy(&pix1);
    if (psize) *psize = imin + 1;

#if  DEBUG_PLOT_CC
    {GPLOT *gplot;
     NUMA  *naseq;
        L_INFO("Best dilation: %d\n", procName, imin);
        naseq = numaMakeSequence(1, 1, numaGetCount(nacc));
        gplot = gplotCreate("/tmp/numcc", GPLOT_PNG,
                            "Number of cc vs. horizontal dilation",
                            "Sel horiz", "Number of cc");
        gplotAddPlot(gplot, naseq, nacc, GPLOT_LINES, "");
        gplotMakeOutput(gplot);
        gplotDestroy(&gplot);
        numaDestroy(&naseq);
        naseq = numaMakeSequence(1, 1, numaGetCount(nadiff));
        gplot = gplotCreate("/tmp/diffcc", GPLOT_PNG,
                            "Diff count of cc vs. horizontal dilation",
                            "Sel horiz", "Diff in cc");
        gplotAddPlot(gplot, naseq, nadiff, GPLOT_LINES, "");
        gplotMakeOutput(gplot);
        gplotDestroy(&gplot);
        numaDestroy(&naseq);
    }
#endif  /* DEBUG_PLOT_CC */

        /* Optionally, save the result of the optimal closing */
    if (ppixm) {
        if (imin < 3)
            L_ERROR("imin = %d is too small\n", procName, imin);
        else
            *ppixm = pixCloseBrick(NULL, pixs, imin + 1, 1);
    }

    numaDestroy(&nacc);
    numaDestroy(&nadiff);
    return 0;
}


/*!
 *  pixWordBoxesByDilation()
 *
 *      Input:  pixs (1 bpp; typ. at 75 to 150 ppi)
 *              maxdil (maximum dilation; 0 for default; warning if > 20)
 *              minwidth, minheight (of saved components; smaller are discarded)
 *              maxwidth, maxheight (of saved components; larger are discarded)
 *              &boxa (<return> dilated word mask)
 *              &size (<optional return> size of optimal horiz Sel)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Returns a pruned set of word boxes.
 *      (2) See pixWordMaskByDilation().
 */
l_int32
pixWordBoxesByDilation(PIX      *pixs,
                       l_int32   maxdil,
                       l_int32   minwidth,
                       l_int32   minheight,
                       l_int32   maxwidth,
                       l_int32   maxheight,
                       BOXA    **pboxa,
                       l_int32  *psize)
{
BOXA  *boxa1, *boxa2;
PIX   *pixm;

    PROCNAME("pixWordBoxesByDilation");

    if (psize) *psize = 0;
    if (!pixs || pixGetDepth(pixs) != 1)
        return ERROR_INT("pixs undefined or not 1 bpp", procName, 1);
    if (!pboxa)
        return ERROR_INT("&boxa not defined", procName, 1);
    *pboxa = NULL;

        /* Make a first estimate of the word masks */
    if (pixWordMaskByDilation(pixs, maxdil, &pixm, psize))
        return ERROR_INT("pixWordMaskByDilation() failed", procName, 1);

        /* Prune it.  Get the bounding boxes of the words.
         * Remove the small ones, which can be due to punctuation
         * that was not joined to a word.  Also remove the large ones,
         * which are not likely to be words. */
    boxa1 = pixConnComp(pixm, NULL, 8);
    boxa2 = boxaSelectBySize(boxa1, minwidth, minheight, L_SELECT_IF_BOTH,
                             L_SELECT_IF_GTE, NULL);
    *pboxa = boxaSelectBySize(boxa2, maxwidth, maxheight, L_SELECT_IF_BOTH,
                             L_SELECT_IF_LTE, NULL);
    boxaDestroy(&boxa1);
    boxaDestroy(&boxa2);
    pixDestroy(&pixm);
    return 0;
}


/*----------------------------------------------------------------------*
 *                 Build grayscale composites (templates)               *
 *----------------------------------------------------------------------*/
/*!
 *  jbAccumulateComposites()
 *
 *      Input:  pixaa (one pixa for each class)
 *              &pna (<return> number of samples used to build each composite)
 *              &ptat (<return> centroids of bordered composites)
 *      Return: pixad (accumulated sum of samples in each class),
 *                     or null on error
 *
 */
PIXA *
jbAccumulateComposites(PIXAA  *pixaa,
                       NUMA  **pna,
                       PTA   **pptat)
{
l_int32    n, nt, i, j, d, minw, maxw, minh, maxh, xdiff, ydiff;
l_float32  x, y, xave, yave;
NUMA      *na;
PIX       *pix, *pixt1, *pixt2, *pixsum;
PIXA      *pixa, *pixad;
PTA       *ptat, *pta;

    PROCNAME("jbAccumulateComposites");

    if (!pptat)
        return (PIXA *)ERROR_PTR("&ptat not defined", procName, NULL);
    *pptat = NULL;
    if (!pna)
        return (PIXA *)ERROR_PTR("&na not defined", procName, NULL);
    *pna = NULL;
    if (!pixaa)
        return (PIXA *)ERROR_PTR("pixaa not defined", procName, NULL);

    n = pixaaGetCount(pixaa, NULL);
    if ((ptat = ptaCreate(n)) == NULL)
        return (PIXA *)ERROR_PTR("ptat not made", procName, NULL);
    *pptat = ptat;
    pixad = pixaCreate(n);
    na = numaCreate(n);
    *pna = na;

    for (i = 0; i < n; i++) {
        pixa = pixaaGetPixa(pixaa, i, L_CLONE);
        nt = pixaGetCount(pixa);
        numaAddNumber(na, nt);
        if (nt == 0) {
            L_WARNING("empty pixa found!\n", procName);
            pixaDestroy(&pixa);
            continue;
        }
        pixaSizeRange(pixa, &minw, &minh, &maxw, &maxh);
        pix = pixaGetPix(pixa, 0, L_CLONE);
        d = pixGetDepth(pix);
        pixDestroy(&pix);
        pixt1 = pixCreate(maxw, maxh, d);
        pixsum = pixInitAccumulate(maxw, maxh, 0);
        pta = pixaCentroids(pixa);

            /* Find the average value of the centroids ... */
        xave = yave = 0;
        for (j = 0; j < nt; j++) {
            ptaGetPt(pta, j, &x, &y);
            xave += x;
            yave += y;
        }
        xave = xave / (l_float32)nt;
        yave = yave / (l_float32)nt;

            /* and place all centroids at their average value */
        for (j = 0; j < nt; j++) {
            pixt2 = pixaGetPix(pixa, j, L_CLONE);
            ptaGetPt(pta, j, &x, &y);
            xdiff = (l_int32)(x - xave);
            ydiff = (l_int32)(y - yave);
            pixClearAll(pixt1);
            pixRasterop(pixt1, xdiff, ydiff, maxw, maxh, PIX_SRC,
                        pixt2, 0, 0);
            pixAccumulate(pixsum, pixt1, L_ARITH_ADD);
            pixDestroy(&pixt2);
        }
        pixaAddPix(pixad, pixsum, L_INSERT);
        ptaAddPt(ptat, xave, yave);

        pixaDestroy(&pixa);
        pixDestroy(&pixt1);
        ptaDestroy(&pta);
    }

    return pixad;
}


/*!
 *  jbTemplatesFromComposites()
 *
 *      Input:  pixac (one pix of composites for each class)
 *              na (number of samples used for each class composite)
 *      Return: pixad (8 bpp templates for each class), or null on error
 *
 */
PIXA *
jbTemplatesFromComposites(PIXA  *pixac,
                          NUMA  *na)
{
l_int32    n, i;
l_float32  nt;  /* number of samples in the composite; always an integer */
l_float32  factor;
PIX       *pixsum;   /* accumulated composite */
PIX       *pixd;
PIXA      *pixad;

    PROCNAME("jbTemplatesFromComposites");

    if (!pixac)
        return (PIXA *)ERROR_PTR("pixac not defined", procName, NULL);
    if (!na)
        return (PIXA *)ERROR_PTR("na not defined", procName, NULL);

    n = pixaGetCount(pixac);
    pixad = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pixsum = pixaGetPix(pixac, i, L_COPY);  /* changed internally */
        numaGetFValue(na, i, &nt);
        factor = 255. / nt;
        pixMultConstAccumulate(pixsum, factor, 0);  /* changes pixsum */
        pixd = pixFinalAccumulate(pixsum, 0, 8);
        pixaAddPix(pixad, pixd, L_INSERT);
        pixDestroy(&pixsum);
    }

    return pixad;
}



/*----------------------------------------------------------------------*
 *                       jbig2 utility routines                         *
 *----------------------------------------------------------------------*/
/*!
 *  jbClasserCreate()
 *
 *      Input:  method (JB_RANKHAUS, JB_CORRELATION)
 *              components (JB_CONN_COMPS, JB_CHARACTERS, JB_WORDS)
 *      Return: jbclasser, or null on error
 */
JBCLASSER *
jbClasserCreate(l_int32  method,
                l_int32  components)
{
JBCLASSER  *classer;

    PROCNAME("jbClasserCreate");

    if ((classer = (JBCLASSER *)CALLOC(1, sizeof(JBCLASSER))) == NULL)
        return (JBCLASSER *)ERROR_PTR("classer not made", procName, NULL);
    if (method != JB_RANKHAUS && method != JB_CORRELATION)
        return (JBCLASSER *)ERROR_PTR("invalid type", procName, NULL);
    if (components != JB_CONN_COMPS && components != JB_CHARACTERS &&
        components != JB_WORDS)
        return (JBCLASSER *)ERROR_PTR("invalid type", procName, NULL);

    classer->method = method;
    classer->components = components;
    classer->nacomps = numaCreate(0);
    classer->pixaa = pixaaCreate(0);
    classer->pixat = pixaCreate(0);
    classer->pixatd = pixaCreate(0);
    classer->nafgt = numaCreate(0);
    classer->naarea = numaCreate(0);
    classer->ptac = ptaCreate(0);
    classer->ptact = ptaCreate(0);
    classer->naclass = numaCreate(0);
    classer->napage = numaCreate(0);
    classer->ptaul = ptaCreate(0);

    return classer;
}


/*
 *  jbClasserDestroy()
 *
 *      Input: &classer (<to be nulled>)
 *      Return: void
 */
void
jbClasserDestroy(JBCLASSER  **pclasser)
{
JBCLASSER  *classer;

    if (!pclasser)
        return;
    if ((classer = *pclasser) == NULL)
        return;

    sarrayDestroy(&classer->safiles);
    numaDestroy(&classer->nacomps);
    pixaaDestroy(&classer->pixaa);
    pixaDestroy(&classer->pixat);
    pixaDestroy(&classer->pixatd);
    numaHashDestroy(&classer->nahash);
    numaDestroy(&classer->nafgt);
    numaDestroy(&classer->naarea);
    ptaDestroy(&classer->ptac);
    ptaDestroy(&classer->ptact);
    numaDestroy(&classer->naclass);
    numaDestroy(&classer->napage);
    ptaDestroy(&classer->ptaul);
    ptaDestroy(&classer->ptall);
    FREE(classer);
    *pclasser = NULL;
    return;
}


/*!
 *  jbDataSave()
 *
 *      Input:  jbclasser
 *              latticew, latticeh (cell size used to store each
 *                  connected component in the composite)
 *      Return: jbdata, or null on error
 *
 *  Notes:
 *      (1) This routine stores the jbig2-type data required for
 *          generating a lossy jbig2 version of the image.
 *          It can be losslessly written to (and read from) two files.
 *      (2) It generates and stores the mosaic of templates.
 *      (3) It clones the Numa and Pta arrays, so these must all
 *          be destroyed by the caller.
 *      (4) Input 0 to use the default values for latticew and/or latticeh,
 */
JBDATA *
jbDataSave(JBCLASSER  *classer)
{
l_int32  maxw, maxh;
JBDATA  *data;
PIX     *pix;

    PROCNAME("jbDataSave");

    if (!classer)
        return (JBDATA *)ERROR_PTR("classer not defined", procName, NULL);

        /* Write the templates into an array. */
    pixaSizeRange(classer->pixat, NULL, NULL, &maxw, &maxh);
    pix = pixaDisplayOnLattice(classer->pixat, maxw + 1, maxh + 1,
                               NULL, NULL);
    if (!pix)
        return (JBDATA *)ERROR_PTR("data not made", procName, NULL);

    if ((data = (JBDATA *)CALLOC(1, sizeof(JBDATA))) == NULL)
        return (JBDATA *)ERROR_PTR("data not made", procName, NULL);
    data->pix = pix;
    data->npages = classer->npages;
    data->w = classer->w;
    data->h = classer->h;
    data->nclass = classer->nclass;
    data->latticew = maxw + 1;
    data->latticeh = maxh + 1;
    data->naclass = numaClone(classer->naclass);
    data->napage = numaClone(classer->napage);
    data->ptaul = ptaClone(classer->ptaul);

    return data;
}


/*
 *  jbDataDestroy()
 *
 *      Input: &data (<to be nulled>)
 *      Return: void
 */
void
jbDataDestroy(JBDATA  **pdata)
{
JBDATA  *data;

    if (!pdata)
        return;
    if ((data = *pdata) == NULL)
        return;

    pixDestroy(&data->pix);
    numaDestroy(&data->naclass);
    numaDestroy(&data->napage);
    ptaDestroy(&data->ptaul);
    FREE(data);
    *pdata = NULL;
    return;
}


/*!
 *  jbDataWrite()
 *
 *      Input:  rootname (for output files; everything but the extension)
 *              jbdata
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Serialization function that writes data in jbdata to file.
 */
l_int32
jbDataWrite(const char  *rootout,
            JBDATA      *jbdata)
{
char     buf[L_BUF_SIZE];
l_int32  w, h, nclass, npages, cellw, cellh, ncomp, i, x, y, iclass, ipage;
NUMA    *naclass, *napage;
PTA     *ptaul;
PIX     *pixt;
FILE    *fp;

    PROCNAME("jbDataWrite");

    if (!rootout)
        return ERROR_INT("no rootout", procName, 1);
    if (!jbdata)
        return ERROR_INT("no jbdata", procName, 1);

    npages = jbdata->npages;
    w = jbdata->w;
    h = jbdata->h;
    pixt = jbdata->pix;
    nclass = jbdata->nclass;
    cellw = jbdata->latticew;
    cellh = jbdata->latticeh;
    naclass = jbdata->naclass;
    napage = jbdata->napage;
    ptaul = jbdata->ptaul;

    snprintf(buf, L_BUF_SIZE, "%s%s", rootout, JB_TEMPLATE_EXT);
    pixWrite(buf, pixt, IFF_PNG);

    snprintf(buf, L_BUF_SIZE, "%s%s", rootout, JB_DATA_EXT);
    if ((fp = fopenWriteStream(buf, "wb")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    ncomp = ptaGetCount(ptaul);
    fprintf(fp, "jb data file\n");
    fprintf(fp, "num pages = %d\n", npages);
    fprintf(fp, "page size: w = %d, h = %d\n", w, h);
    fprintf(fp, "num components = %d\n", ncomp);
    fprintf(fp, "num classes = %d\n", nclass);
    fprintf(fp, "template lattice size: w = %d, h = %d\n", cellw, cellh);
    for (i = 0; i < ncomp; i++) {
        numaGetIValue(napage, i, &ipage);
        numaGetIValue(naclass, i, &iclass);
        ptaGetIPt(ptaul, i, &x, &y);
        fprintf(fp, "%d %d %d %d\n", ipage, iclass, x, y);
    }
    fclose(fp);

    return 0;
}


/*!
 *  jbDataRead()
 *
 *      Input:  rootname (for template and data files)
 *      Return: jbdata, or NULL on error
 */
JBDATA *
jbDataRead(const char  *rootname)
{
char      fname[L_BUF_SIZE];
char     *linestr;
l_uint8  *data;
l_int32   nsa, i, w, h, cellw, cellh, x, y, iclass, ipage;
l_int32   npages, nclass, ncomp;
size_t    size;
JBDATA   *jbdata;
NUMA     *naclass, *napage;
PIX      *pixs;
PTA      *ptaul;
SARRAY   *sa;

    PROCNAME("jbDataRead");

    if (!rootname)
        return (JBDATA *)ERROR_PTR("rootname not defined", procName, NULL);

    snprintf(fname, L_BUF_SIZE, "%s%s", rootname, JB_TEMPLATE_EXT);
    if ((pixs = pixRead(fname)) == NULL)
        return (JBDATA *)ERROR_PTR("pix not read", procName, NULL);

    snprintf(fname, L_BUF_SIZE, "%s%s", rootname, JB_DATA_EXT);
    if ((data = l_binaryRead(fname, &size)) == NULL)
        return (JBDATA *)ERROR_PTR("data not read", procName, NULL);

    if ((sa = sarrayCreateLinesFromString((char *)data, 0)) == NULL)
        return (JBDATA *)ERROR_PTR("sa not made", procName, NULL);
    nsa = sarrayGetCount(sa);   /* number of cc + 6 */
    linestr = sarrayGetString(sa, 0, 0);
    if (strcmp(linestr, "jb data file"))
        return (JBDATA *)ERROR_PTR("invalid jb data file", procName, NULL);
    linestr = sarrayGetString(sa, 1, 0);
    sscanf(linestr, "num pages = %d", &npages);
    linestr = sarrayGetString(sa, 2, 0);
    sscanf(linestr, "page size: w = %d, h = %d", &w, &h);
    linestr = sarrayGetString(sa, 3, 0);
    sscanf(linestr, "num components = %d", &ncomp);
    linestr = sarrayGetString(sa, 4, 0);
    sscanf(linestr, "num classes = %d\n", &nclass);
    linestr = sarrayGetString(sa, 5, 0);
    sscanf(linestr, "template lattice size: w = %d, h = %d\n", &cellw, &cellh);

#if 1
    fprintf(stderr, "num pages = %d\n", npages);
    fprintf(stderr, "page size: w = %d, h = %d\n", w, h);
    fprintf(stderr, "num components = %d\n", ncomp);
    fprintf(stderr, "num classes = %d\n", nclass);
    fprintf(stderr, "template lattice size: w = %d, h = %d\n", cellw, cellh);
#endif

    if ((naclass = numaCreate(ncomp)) == NULL)
        return (JBDATA *)ERROR_PTR("naclass not made", procName, NULL);
    if ((napage = numaCreate(ncomp)) == NULL)
        return (JBDATA *)ERROR_PTR("napage not made", procName, NULL);
    if ((ptaul = ptaCreate(ncomp)) == NULL)
        return (JBDATA *)ERROR_PTR("pta not made", procName, NULL);
    for (i = 6; i < nsa; i++) {
        linestr = sarrayGetString(sa, i, 0);
        sscanf(linestr, "%d %d %d %d\n", &ipage, &iclass, &x, &y);
        numaAddNumber(napage, ipage);
        numaAddNumber(naclass, iclass);
        ptaAddPt(ptaul, x, y);
    }

    if ((jbdata = (JBDATA *)CALLOC(1, sizeof(JBDATA))) == NULL)
        return (JBDATA *)ERROR_PTR("data not made", procName, NULL);
    jbdata->pix = pixs;
    jbdata->npages = npages;
    jbdata->w = w;
    jbdata->h = h;
    jbdata->nclass = nclass;
    jbdata->latticew = cellw;
    jbdata->latticeh = cellh;
    jbdata->naclass = naclass;
    jbdata->napage = napage;
    jbdata->ptaul = ptaul;

    FREE(data);
    sarrayDestroy(&sa);
    return jbdata;
}


/*!
 *  jbDataRender()
 *
 *      Input:  jbdata
 *              debugflag (if TRUE, writes into 2 bpp pix and adds
 *                         component outlines in color)
 *      Return: pixa (reconstruction of original images, using templates) or
 *              null on error
 */
PIXA *
jbDataRender(JBDATA  *data,
             l_int32  debugflag)
{
l_int32   i, w, h, cellw, cellh, x, y, iclass, ipage;
l_int32   npages, nclass, ncomp, wp, hp;
BOX      *box;
NUMA     *naclass, *napage;
PIX      *pixt, *pixt2, *pix, *pixd;
PIXA     *pixat;   /* pixa of templates */
PIXA     *pixad;   /* pixa of output images */
PIXCMAP  *cmap;
PTA      *ptaul;

    PROCNAME("jbDataRender");

    if (!data)
        return (PIXA *)ERROR_PTR("data not defined", procName, NULL);

    npages = data->npages;
    w = data->w;
    h = data->h;
    pixt = data->pix;
    nclass = data->nclass;
    cellw = data->latticew;
    cellh = data->latticeh;
    naclass = data->naclass;
    napage = data->napage;
    ptaul = data->ptaul;
    ncomp = numaGetCount(naclass);

        /* Reconstruct the original set of images from the templates
         * and the data associated with each component.  First,
         * generate the output pixa as a set of empty pix. */
    if ((pixad = pixaCreate(npages)) == NULL)
        return (PIXA *)ERROR_PTR("pixad not made", procName, NULL);
    for (i = 0; i < npages; i++) {
        if (debugflag == FALSE) {
            pix = pixCreate(w, h, 1);
        } else {
            pix = pixCreate(w, h, 2);
            cmap = pixcmapCreate(2);
            pixcmapAddColor(cmap, 255, 255, 255);
            pixcmapAddColor(cmap, 0, 0, 0);
            pixcmapAddColor(cmap, 255, 0, 0);  /* for box outlines */
            pixSetColormap(pix, cmap);
        }
        pixaAddPix(pixad, pix, L_INSERT);
    }

        /* Put the class templates into a pixa. */
    if ((pixat = pixaCreateFromPix(pixt, nclass, cellw, cellh)) == NULL)
        return (PIXA *)ERROR_PTR("pixat not made", procName, NULL);

        /* Place each component in the right location on its page. */
    for (i = 0; i < ncomp; i++) {
        numaGetIValue(napage, i, &ipage);
        numaGetIValue(naclass, i, &iclass);
        pix = pixaGetPix(pixat, iclass, L_CLONE);  /* the template */
        wp = pixGetWidth(pix);
        hp = pixGetHeight(pix);
        ptaGetIPt(ptaul, i, &x, &y);
        pixd = pixaGetPix(pixad, ipage, L_CLONE);   /* the output page */
        if (debugflag == FALSE) {
            pixRasterop(pixd, x, y, wp, hp, PIX_SRC | PIX_DST, pix, 0, 0);
        } else {
            pixt2 = pixConvert1To2Cmap(pix);
            pixRasterop(pixd, x, y, wp, hp, PIX_SRC | PIX_DST, pixt2, 0, 0);
            box = boxCreate(x, y, wp, hp);
            pixRenderBoxArb(pixd, box, 1, 255, 0, 0);
            pixDestroy(&pixt2);
            boxDestroy(&box);
        }
        pixDestroy(&pix);   /* the clone only */
        pixDestroy(&pixd);  /* the clone only */
    }

    pixaDestroy(&pixat);
    return pixad;
}


/*!
 *  jbGetULCorners()
 *
 *      Input:  jbclasser
 *              pixs (full res image)
 *              boxa (of c.c. bounding rectangles for this page)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This computes the ptaul field, which has the global UL corners,
 *          adjusted for each specific component, so that each component
 *          can be replaced by the template for its class and have the
 *          centroid in the template in the same position as the
 *          centroid of the original connected component.  It is important
 *          that this be done properly to avoid a wavy baseline in the
 *          result.
 *      (2) The array fields ptac and ptact give the centroids of
 *          those components relative to the UL corner of each component.
 *          Here, we compute the difference in each component, round to
 *          nearest integer, and correct the box->x and box->y by
 *          the appropriate integral difference.
 *      (3) The templates and stored instances are all bordered.
 */
l_int32
jbGetULCorners(JBCLASSER  *classer,
               PIX        *pixs,
               BOXA       *boxa)
{
l_int32    i, baseindex, index, n, iclass, idelx, idely, x, y, dx, dy;
l_int32   *sumtab;
l_float32  x1, x2, y1, y2, delx, dely;
BOX       *box;
NUMA      *naclass;
PIX       *pixt;
PTA       *ptac, *ptact, *ptaul;

    PROCNAME("jbGetULCorners");

    if (!classer)
        return ERROR_INT("classer not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    n = boxaGetCount(boxa);
    ptaul = classer->ptaul;
    naclass = classer->naclass;
    ptac = classer->ptac;
    ptact = classer->ptact;
    baseindex = classer->baseindex;  /* num components before this page */
    sumtab = makePixelSumTab8();
    for (i = 0; i < n; i++) {
        index = baseindex + i;
        ptaGetPt(ptac, index, &x1, &y1);
        numaGetIValue(naclass, index, &iclass);
        ptaGetPt(ptact, iclass, &x2, &y2);
        delx = x2 - x1;
        dely = y2 - y1;
        if (delx >= 0)
            idelx = (l_int32)(delx + 0.5);
        else
            idelx = (l_int32)(delx - 0.5);
        if (dely >= 0)
            idely = (l_int32)(dely + 0.5);
        else
            idely = (l_int32)(dely - 0.5);
        if ((box = boxaGetBox(boxa, i, L_CLONE)) == NULL)
            return ERROR_INT("box not found", procName, 1);
        boxGetGeometry(box, &x, &y, NULL, NULL);

            /* Get final increments dx and dy for best alignment */
        pixt = pixaGetPix(classer->pixat, iclass, L_CLONE);
        finalPositioningForAlignment(pixs, x, y, idelx, idely,
                                     pixt, sumtab, &dx, &dy);
/*        if (i % 20 == 0)
            fprintf(stderr, "dx = %d, dy = %d\n", dx, dy); */
        ptaAddPt(ptaul, x - idelx + dx, y - idely + dy);
        boxDestroy(&box);
        pixDestroy(&pixt);
    }

    FREE(sumtab);
    return 0;
}


/*!
 *  jbGetLLCorners()
 *
 *      Input:  jbclasser
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This computes the ptall field, which has the global LL corners,
 *          adjusted for each specific component, so that each component
 *          can be replaced by the template for its class and have the
 *          centroid in the template in the same position as the
 *          centroid of the original connected component. It is important
 *          that this be done properly to avoid a wavy baseline in the result.
 *      (2) It is computed here from the corresponding UL corners, where
 *          the input templates and stored instances are all bordered.
 *          This should be done after all pages have been processed.
 *      (3) For proper substitution, the templates whose LL corners are
 *          placed in these locations must be UN-bordered.
 *          This is available for a realistic jbig2 encoder, which would
 *          (1) encode each template without a border, and (2) encode
 *          the position using the LL corner (rather than the UL
 *          corner) because the difference between y-values
 *          of successive instances is typically close to zero.
 */
l_int32
jbGetLLCorners(JBCLASSER  *classer)
{
l_int32    i, iclass, n, x1, y1, h;
NUMA      *naclass;
PIX       *pix;
PIXA      *pixat;
PTA       *ptaul, *ptall;

    PROCNAME("jbGetLLCorners");

    if (!classer)
        return ERROR_INT("classer not defined", procName, 1);

    ptaul = classer->ptaul;
    naclass = classer->naclass;
    pixat = classer->pixat;

    ptaDestroy(&classer->ptall);
    n = ptaGetCount(ptaul);
    ptall = ptaCreate(n);
    classer->ptall = ptall;

        /* If the templates were bordered, we would add h - 1 to the UL
         * corner y-value.  However, because the templates to be used
         * here have their borders removed, and the borders are
         * JB_ADDED_PIXELS on each side, we add h - 1 - 2 * JB_ADDED_PIXELS
         * to the UL corner y-value.  */
    for (i = 0; i < n; i++) {
        ptaGetIPt(ptaul, i, &x1, &y1);
        numaGetIValue(naclass, i, &iclass);
        pix = pixaGetPix(pixat, iclass, L_CLONE);
        h = pixGetHeight(pix);
        ptaAddPt(ptall, x1, y1 + h - 1 - 2 * JB_ADDED_PIXELS);
        pixDestroy(&pix);
    }

    return 0;
}


/*----------------------------------------------------------------------*
 *                              Static helpers                          *
 *----------------------------------------------------------------------*/
/* When looking for similar matches we check templates whose size is +/- 2 in
 * each direction. This involves 25 possible sizes. This array contains the
 * offsets for each of those positions in a spiral pattern. There are 25 pairs
 * of numbers in this array: even positions are x values. */
static int two_by_two_walk[50] = {
  0, 0,
  0, 1,
  -1, 0,
  0, -1,
  1, 0,
  -1, 1,
  1, 1,
  -1, -1,
  1, -1,
  0, -2,
  2, 0,
  0, 2,
  -2, 0,
  -1, -2,
  1, -2,
  2, -1,
  2, 1,
  1, 2,
  -1, 2,
  -2, 1,
  -2, -1,
  -2, -2,
  2, -2,
  2, 2,
  -2, 2};


/*!
 *  findSimilarSizedTemplatesInit()
 *
 *      Input:  classer
 *              pixs (instance to be matched)
 *      Return: Allocated context to be used with findSimilar*
 */
static JBFINDCTX *
findSimilarSizedTemplatesInit(JBCLASSER  *classer,
                              PIX        *pixs)
{
JBFINDCTX  *state;

    state = (JBFINDCTX *)CALLOC(1, sizeof(JBFINDCTX));
    state->w = pixGetWidth(pixs) - 2 * JB_ADDED_PIXELS;
    state->h = pixGetHeight(pixs) - 2 * JB_ADDED_PIXELS;
    state->classer = classer;

    return state;
}


static void
findSimilarSizedTemplatesDestroy(JBFINDCTX  **pstate)
{
JBFINDCTX  *state;

    PROCNAME("findSimilarSizedTemplatesDestroy");

    if (pstate == NULL) {
        L_WARNING("ptr address is null\n", procName);
        return;
    }
    if ((state = *pstate) == NULL)
        return;

    numaDestroy(&state->numa);
    FREE(state);
    *pstate = NULL;
    return;
}


/*!
 *  findSimilarSizedTemplatesNext()
 *
 *      Input:  state (from findSimilarSizedTemplatesInit)
 *      Return: Next template number, or -1 when finished
 *
 *  We have a hash table mapping template area to a list of template
 *  numbers with that area.  We wish to find similar sized templates,
 *  so we first look for templates with the same width and height, and
 *  then with width + 1, etc.  This walk is guided by the
 *  two_by_two_walk array, above.
 *
 *  We don't want to have to collect the whole list of templates first because
 *  (we hope) to find it quickly.  So we keep the context for this walk in an
 *  explictit state structure and this function acts like a generator.
 */
static l_int32
findSimilarSizedTemplatesNext(JBFINDCTX  *state)
{
l_int32  desiredh, desiredw, size, templ;
PIX     *pixt;

    while(1) {  /* Continue the walk over step 'i' */
        if (state->i >= 25) {  /* all done */
            return -1;
        }

        desiredw = state->w + two_by_two_walk[2 * state->i];
        desiredh = state->h + two_by_two_walk[2 * state->i + 1];
        if (desiredh < 1 || desiredw < 1) {  /* invalid size */
            state->i++;
            continue;
        }

        if (!state->numa) {
                /* We have yet to start walking the array for the step 'i' */
            state->numa = numaHashGetNuma(state->classer->nahash,
                                          desiredh * desiredw);
            if (!state->numa) {  /* nothing there */
                state->i++;
                continue;
            }

            state->n = 0;  /* OK, we got a numa. */
        }

            /* Continue working on this numa */
        size = numaGetCount(state->numa);
        for ( ; state->n < size; ) {
            templ = (l_int32)(state->numa->array[state->n++] + 0.5);
            pixt = pixaGetPix(state->classer->pixat, templ, L_CLONE);
            if (pixGetWidth(pixt) - 2 * JB_ADDED_PIXELS == desiredw &&
                pixGetHeight(pixt) - 2 * JB_ADDED_PIXELS == desiredh) {
                pixDestroy(&pixt);
                return templ;
            }
            pixDestroy(&pixt);
        }

            /* Exhausted the numa; take another step and try again */
        state->i++;
        numaDestroy(&state->numa);
        continue;
    }
}


/*!
 *  finalPositioningForAlignment()
 *
 *      Input:  pixs (input page image)
 *              x, y (location of UL corner of bb of component in pixs)
 *              idelx, idely (compensation to match centroids of component
 *                            and template)
 *              pixt (template, with JB_ADDED_PIXELS of padding on all sides)
 *              sumtab (for summing fg pixels in an image)
 *              &dx, &dy (return delta on position for best match; each
 *                        one is in the set {-1, 0, 1})
 *      Return: 0 if OK, 1 on error
 *
 */
static l_int32
finalPositioningForAlignment(PIX      *pixs,
                             l_int32   x,
                             l_int32   y,
                             l_int32   idelx,
                             l_int32   idely,
                             PIX      *pixt,
                             l_int32  *sumtab,
                             l_int32  *pdx,
                             l_int32  *pdy)
{
l_int32  w, h, i, j, minx, miny, count, mincount;
PIX     *pixi;  /* clipped from source pixs */
PIX     *pixr;  /* temporary storage */
BOX     *box;

    PROCNAME("finalPositioningForAlignment");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixt)
        return ERROR_INT("pixt not defined", procName, 1);
    if (!pdx || !pdy)
        return ERROR_INT("&dx and &dy not both defined", procName, 1);
    if (!sumtab)
        return ERROR_INT("sumtab not defined", procName, 1);
    *pdx = *pdy = 0;

        /* Use JB_ADDED_PIXELS pixels padding on each side */
    w = pixGetWidth(pixt);
    h = pixGetHeight(pixt);
    box = boxCreate(x - idelx - JB_ADDED_PIXELS,
                    y - idely - JB_ADDED_PIXELS, w, h);
    pixi = pixClipRectangle(pixs, box, NULL);
    boxDestroy(&box);
    if (!pixi)
        return ERROR_INT("pixi not made", procName, 1);

    pixr = pixCreate(pixGetWidth(pixi), pixGetHeight(pixi), 1);
    mincount = 0x7fffffff;
    for (i = -1; i <= 1; i++) {
        for (j = -1; j <= 1; j++) {
            pixCopy(pixr, pixi);
            pixRasterop(pixr, j, i, w, h, PIX_SRC ^ PIX_DST, pixt, 0, 0);
            pixCountPixels(pixr, &count, sumtab);
            if (count < mincount) {
                minx = j;
                miny = i;
                mincount = count;
            }
        }
    }
    pixDestroy(&pixi);
    pixDestroy(&pixr);

    *pdx = minx;
    *pdy = miny;
    return 0;
}
