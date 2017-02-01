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

/*!
 * \file recogbasic.c
 * <pre>
 *
 *      Recoga creation, destruction and access
 *         L_RECOGA           *recogaCreateFromRecog()
 *         L_RECOGA           *recogaCreateFromPixaa()
 *         L_RECOGA           *recogaCreate()
 *         void                recogaDestroy()
 *         l_int32             recogaAddRecog()
 *         static l_int32      recogaExtendArray()
 *         l_int32             recogReplaceInRecoga()
 *         L_RECOG            *recogaGetRecog()
 *         l_int32             recogaGetCount()
 *         l_int32             recogGetCount()
 *         l_int32             recogGetIndex()
 *         l_int32             recogGetParent()
 *         l_int32             recogSetBootflag()
 *
 *      Recog initialization and destruction
 *         L_RECOG            *recogCreateFromRecog()
 *         L_RECOG            *recogCreateFromPixa()
 *         L_RECOG            *recogCreate()
 *         void                recogDestroy()
 *
 *      Appending (combining two recogs into one)
 *         l_int32             recogAppend()
 *
 *      Character/index lookup
 *         l_int32             recogGetClassIndex()
 *         l_int32             recogStringToIndex()
 *         l_int32             recogGetClassString()
 *         l_int32             l_convertCharstrToInt()
 *
 *      Serialization
 *         L_RECOGA           *recogaRead()
 *         L_RECOGA           *recogaReadStream()
 *         L_RECOGA           *recogaReadMem()
 *         l_int32             recogaWrite()
 *         l_int32             recogaWriteStream()
 *         l_int32             recogaWritePixaa()
 *         l_int32             recogaWriteMem()
 *         L_RECOG            *recogRead()
 *         L_RECOG            *recogReadStream()
 *         L_RECOG            *recogReadMem()
 *         l_int32             recogWrite()
 *         l_int32             recogWriteStream()
 *         l_int32             recogWriteMem()
 *         PIXA               *recogExtractPixa()
 *         static l_int32      recogAddCharstrLabels()
 *         static l_int32      recogAddAllSamples()
 *
 *  The recognizer functionality is split into four files:
 *    recogbasic.c: create, destroy, access, serialize
 *    recogtrain.c: training on labelled and unlabelled data
 *    recogident.c: running the recognizer(s) on input
 *    recogdid.c:   running the recognizer(s) on input using a
 *                  document image decoding (DID) hidden markov model
 *
 *  This is a content-adapted (or book-adapted) recognizer (BAR) application.
 *  The recognizers here are typically bootstrapped from data that has
 *  been labelled by a generic recognition system, such as Tesseract.
 *  The general procedure to create a recognizer (recog) from labelled data is
 *  to add the labelled character bitmaps, and call recogTrainingFinished()
 *  when done.
 *
 *  Typically, the recog is added to a recoga (an array of recognizers)
 *  before use.  However, for identifying single characters, it is possible
 *  to use a single recog.
 *
 *  If there is more than one recog, the usage options are:
 *  (1) To join the two together (e.g., if they're from the same source)
 *  (2) To put them separately into a recoga (recognizer array).
 *
 *  For training numeric input, an example set of calls that scales
 *  each training input to (w, h) and will use the averaged
 *  templates for identifying unknown characters is:
 *         L_Recog  *rec = recogCreate(w, h, L_USE_AVERAGE, 128, 1);
 *         for (i = 0; i < n; i++) {  // read in n training digits
 *             Pix *pix = ...
 *             recogTrainLabelled(rec, pix, NULL, text[i], 0, 0);
 *         }
 *         recogTrainingFinished(rec, 0);  // required
 *
 *  It is an error if any function that computes averages, removes
 *  outliers or requests identification of an unlabelled character,
 *  such as:
 *         (1) computing the sample averages: recogAverageSamples()
 *         (2) removing outliers: recogRemoveOutliers()
 *         (3) requesting identification of an unlabeled character:
 *                 recogIdentifyPix()
 *  is called before an explicit call to finish training.  Note that
 *  to do further training on a "finished" recognizer, just set
 *         recog->train_done = FALSE;
 *  add the new training samples, and again call
 *         recogTrainingFinished(rec, 0);  // required
 *
 *  If using all examples for identification, all scaled to (w, h),
 *  and with outliers removed, do something like this:
 *         L_Recog  *rec = recogCreate(w, h, L_USE_ALL, 128, 1);
 *         for (i = 0; i < n; i++) {  // read in n training characters
 *             Pix *pix = ...
 *             recogTrainLabelled(rec, pix, NULL, text[i], 0, 0);
 *         }
 *         recogTrainingFinished(rec, 0);
 *         // remove anything with correlation less than 0.7 with average
 *         recogRemoveOutliers(rec, 0.7, 0.5, 0);
 *
 *  You can train a recognizer from a pixa where the text field in each
 *  pix is the character string:
 *
 *         L_Recog  *recboot = recogCreateFromPixa(pixa, w, h, L_USE_AVERAGE,
 *                                                 128, 1);
 *
 *  This is useful as a "bootstrap" recognizer for training a new
 *  adapted recognizer (rec) on an unlabelled data set that has a different
 *  origin from recboot.  To do this, the new recognizer must be
 *  initialized to use the same (w,h) scaling as the bootstrap recognizer.
 *  If the new recognizer is to be used without scaling (e.g., on images
 *  from a single source, like a book), call recogSetScaling() to
 *  regenerate all the scaled samples and averages:
 *
 *         L_Recog  *rec = recogCreate(w, h, L_USE_ALL, 128, 1);
 *         for (i = 0; i < n; i++) {  // read in n training characters
 *             Pix *pix = ...
 *             recogTrainUnlabelled(rec, recboot, pix, NULL, 1, 0.75, 0);
 *         }
 *         recogTrainingFinished(rec, 0);
 *         recogSetScaling(rec, 0, 0, L_USE_ALL);  // use with no scaling
 *
 *  The adapted recognizer seems to work better if you use all the
 *  templates.
 *
 * </pre>
 */

#include <string.h>
#include "allheaders.h"

static const l_int32  INITIAL_PTR_ARRAYSIZE = 20;  /* n'import quoi */
static const l_int32  MAX_EXAMPLES_IN_CLASS = 256;

    /* Static functions */
static l_int32 recogaExtendArray(L_RECOGA *recoga);
static l_int32 recogAddCharstrLabels(L_RECOG *recog);
static l_int32 recogAddAllSamples(L_RECOG *recog, PIXAA *paa, l_int32 debug);

    /* Tolerance (+-) in asperity ratio between unknown and known */
static l_float32  DEFAULT_ASPERITY_FRACT = 0.25;


/*------------------------------------------------------------------------*
 *                Recoga: creation, destruction, access                   *
 *------------------------------------------------------------------------*/
/*!
 * \brief   recogaCreateFromRecog()
 *
 * \param[in]    recog
 * \return  recoga, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This is a convenience function for making a recoga after
 *          you have a recog.  The recog is owned by the recoga.
 *      (2) For splitting connected components, the
 *          input recog must be from the material to be identified,
 *          and not a generic bootstrap recog.  Those can be added later.
 * </pre>
 */
L_RECOGA *
recogaCreateFromRecog(L_RECOG  *recog)
{
L_RECOGA  *recoga;

    PROCNAME("recogaCreateFromRecog");

    if (!recog)
        return (L_RECOGA *)ERROR_PTR("recog not defined", procName, NULL);

    recoga = recogaCreate(1);
    recogaAddRecog(recoga, recog);
    return recoga;
}


/*!
 * \brief   recogaCreateFromPixaa()
 *
 * \param[in]    paa of labelled, 1 bpp images
 * \param[in]    scalew  scale all widths to this; use 0 for no scaling
 * \param[in]    scaleh  scale all heights to this; use 0 for no scaling
 * \param[in]    templ_type L_USE_AVERAGE or L_USE_ALL
 * \param[in]    threshold for binarization; typically ~128
 * \param[in]    maxyshift from nominal centroid alignment; typically 0 or 1
 * \return  recoga, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This is a convenience function for training from labelled data.
 *      (2) Each pixa in the paa is a set of labelled data that is used
 *          to train a recognizer (e.g., for a set of characters in a font).
 *          Each image example in the pixa is put into a class in its
 *          recognizer, defined by its character label.  All examples in
 *          the same class should be similar.
 *      (3) The pixaa can be written by recogaWritePixaa(), and must contain
 *          the unscaled bitmaps used for training.
 * </pre>
 */
L_RECOGA *
recogaCreateFromPixaa(PIXAA       *paa,
                      l_int32      scalew,
                      l_int32      scaleh,
                      l_int32      templ_type,
                      l_int32      threshold,
                      l_int32      maxyshift)
{
l_int32    n, i, full;
L_RECOG   *recog;
L_RECOGA  *recoga;
PIXA      *pixa;

    PROCNAME("recogaCreateFromPixaa");

    if (!paa)
        return (L_RECOGA *)ERROR_PTR("paa not defined", procName, NULL);
    if (pixaaVerifyDepth(paa, NULL) != 1)
        return (L_RECOGA *)ERROR_PTR("all pix not 1 bpp", procName, NULL);
    pixaaIsFull(paa, &full);
    if (!full)
        return (L_RECOGA *)ERROR_PTR("all pix not present", procName, NULL);

    n = pixaaGetCount(paa, NULL);
    recoga = recogaCreate(n);
    for (i = 0; i < n; i++) {
        pixa = pixaaGetPixa(paa, i, L_CLONE);
        recog = recogCreateFromPixa(pixa, scalew, scaleh, templ_type,
                                    threshold, maxyshift);
        recogaAddRecog(recoga, recog);
        pixaDestroy(&pixa);
    }

    return recoga;
}


/*!
 * \brief   recogaCreate()
 *
 * \param[in]    n initial number of recog ptrs
 * \return  recoga, or NULL on error
 */
L_RECOGA *
recogaCreate(l_int32  n)
{
L_RECOGA  *recoga;

    PROCNAME("recogaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((recoga = (L_RECOGA *)LEPT_CALLOC(1, sizeof(L_RECOGA))) == NULL)
        return (L_RECOGA *)ERROR_PTR("recoga not made", procName, NULL);
    recoga->n = 0;
    recoga->nalloc = n;

    if ((recoga->recog = (L_RECOG **)LEPT_CALLOC(n, sizeof(L_RECOG *))) == NULL)
        return (L_RECOGA *)ERROR_PTR("recoga ptrs not made", procName, NULL);

    return recoga;
}


/*!
 * \brief   recogaDestroy()
 *
 * \param[in,out]   precoga will be set to null before returning
 * \return  void
 *
 * <pre>
 * Notes:
 *      (1) If a recog has a parent, the parent owns it.  To destroy
 *          a recog, it must first be "orphaned".
 * </pre>
 */
void
recogaDestroy(L_RECOGA  **precoga)
{
l_int32    i;
L_RECOG   *recog;
L_RECOGA  *recoga;

    PROCNAME("recogaDestroy");

    if (precoga == NULL) {
        L_WARNING("ptr address is null!\n", procName);
        return;
    }

    if ((recoga = *precoga) == NULL)
        return;

    rchaDestroy(&recoga->rcha);
    for (i = 0; i < recoga->n; i++) {
        if ((recog = recoga->recog[i]) == NULL) {
            L_ERROR("recog not found for index %d\n", procName, i);
            continue;
        }
        recog->parent = NULL;  /* orphan it */
        recogDestroy(&recog);
    }
    LEPT_FREE(recoga->recog);
    LEPT_FREE(recoga);
    *precoga = NULL;
    return;
}


/*!
 * \brief   recogaAddRecog()
 *
 * \param[in]    recoga
 * \param[in]    recog to be added and owned by the recoga; not a copy
 * \return  recoga, or NULL on error
 */
l_int32
recogaAddRecog(L_RECOGA  *recoga,
               L_RECOG   *recog)
{
l_int32  n;

    PROCNAME("recogaAddRecog");

    if (!recoga)
        return ERROR_INT("recoga not defined", procName, 1);
    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);

    n = recoga->n;
    if (n >= recoga->nalloc)
        recogaExtendArray(recoga);
    recoga->recog[n] = recog;
    recog->index = n;
    recog->parent = recoga;
    recoga->n++;
    return 0;
}


/*!
 * \brief   recogaExtendArray()
 *
 * \param[in]    recoga
 * \return  0 if OK, 1 on error
 */
static l_int32
recogaExtendArray(L_RECOGA  *recoga)
{
    PROCNAME("recogaExtendArray");

    if (!recoga)
        return ERROR_INT("recogaa not defined", procName, 1);

    if ((recoga->recog = (L_RECOG **)reallocNew((void **)&recoga->recog,
                              sizeof(L_RECOG *) * recoga->nalloc,
                              2 * sizeof(L_RECOG *) * recoga->nalloc)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);

    recoga->nalloc *= 2;
    return 0;
}


/*!
 * \brief   recogReplaceInRecoga()
 *
 * \param[in,out]  precog1 old recog, to be destroyed
 * \param[in]      recog2 new recog, to be inserted in place of %recog1
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This always destroys recog1.
 *      (2) If recog1 belongs to a recoga, this inserts recog2 into
 *          the slot that recog1 previously occupied.
 * </pre>
 */
l_int32
recogReplaceInRecoga(L_RECOG  **precog1,
                     L_RECOG   *recog2)
{
l_int32    n, index;
L_RECOG   *recog1;
L_RECOGA  *recoga;

    PROCNAME("recogReplaceInRecoga");

    if (!precog1)
        return ERROR_INT("&recog1 not defined", procName, 1);
    if (!recog2)
        return ERROR_INT("recog2 not defined", procName, 1);
    if ((recog1 = *precog1) == NULL)
        return ERROR_INT("recog1 not defined", procName, 1);

    if ((recoga = recogGetParent(recog1)) == NULL) {
        recogDestroy(precog1);
        return 0;
    }

    n = recogaGetCount(recoga);
    recogGetIndex(recog1, &index);
    if (index >= n) {
        L_ERROR("invalid index %d in recog1; no replacement\n", procName,
                recog1->index);
        recogDestroy(precog1);
        return 1;
    }

    recog1->parent = NULL;  /* necessary to destroy recog1 */
    recogDestroy(precog1);
    recoga->recog[index] = recog2;
    recog2->index = index;
    recog2->parent = recoga;
    return 0;
}


/*!
 * \brief   recogaGetRecog()
 *
 * \param[in]    recoga
 * \param[in]    index to the index-th recog
 * \return  recog, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This returns a ptr to the recog, which is still owned by
 *          the recoga.  Do not destroy it.
 * </pre>
 */
L_RECOG *
recogaGetRecog(L_RECOGA  *recoga,
               l_int32    index)
{
L_RECOG  *recog;

    PROCNAME("recogaAddRecog");

    if (!recoga)
        return (L_RECOG *)ERROR_PTR("recoga not defined", procName, NULL);
    if (index < 0 || index >= recoga->n)
        return (L_RECOG *)ERROR_PTR("index not valid", procName, NULL);

    recog = recoga->recog[index];
    return recog;
}


/*!
 * \brief   recogaGetCount()
 *
 * \param[in]    recoga
 * \return  count of recog in array; 0 if no recog or on error
 */
l_int32
recogaGetCount(L_RECOGA  *recoga)
{
    PROCNAME("recogaGetCount");

    if (!recoga)
        return ERROR_INT("recoga not defined", procName, 0);
    return recoga->n;
}


/*!
 * \brief   recogGetCount()
 *
 * \param[in]    recog
 * \return  count of classes in recog; 0 if no recog or on error
 */
l_int32
recogGetCount(L_RECOG  *recog)
{
    PROCNAME("recogGetCount");

    if (!recog)
        return ERROR_INT("recog not defined", procName, 0);
    return recog->setsize;
}


/*!
 * \brief   recogGetIndex()
 *
 * \param[in]    recog
 * \param[out]   pindex into the parent recoga; -1 if no parent
 * \return  0 if OK, 1 on error
 */
l_int32
recogGetIndex(L_RECOG  *recog,
              l_int32  *pindex)
{
    PROCNAME("recogGetIndex");

    if (!pindex)
        return ERROR_INT("&index not defined", procName, 1);
    *pindex = -1;
    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);
    *pindex = recog->index;
    return 0;
}

/*!
 * \brief   recogGetParent()
 *
 * \param[in]    recog
 * \return  recoga back-pointer to parent; can be null
 */
L_RECOGA *
recogGetParent(L_RECOG  *recog)
{
    PROCNAME("recogGetParent");

    if (!recog)
        return (L_RECOGA *)ERROR_PTR("recog not defined", procName, NULL);
    return recog->parent;
}


/*!
 * \brief   recogSetBootflag()
 *
 * \param[in]    recog
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This must be set for any bootstrap recog, where the samples
 *          are not from the media being identified.
 *      (2) It is used to enforce scaled bitmaps for identification,
 *          and to prevent the recog from being used to split touching
 *          characters (which requires unscaled samples from the
 *          material being identified).
 * </pre>
 */
l_int32
recogSetBootflag(L_RECOG  *recog)
{
    PROCNAME("recogSetBootflag");

    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);
    recog->bootrecog = 1;
    return 0;
}


/*------------------------------------------------------------------------*
 *                Recog: initialization and destruction                   *
 *------------------------------------------------------------------------*/
/*!
 * \brief   recogCreateFromRecog()
 *
 * \param[in]    recs source recog with arbitrary input parameters
 * \param[in]    scalew  scale all widths to this; use 0 for no scaling
 * \param[in]    scaleh  scale all heights to this; use 0 for no scaling
 * \param[in]    templ_type L_USE_AVERAGE or L_USE_ALL
 * \param[in]    threshold for binarization; typically ~128
 * \param[in]    maxyshift from nominal centroid alignment; typically 0 or 1
 * \return  recd, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This is a convenience function that generates a recog using
 *          the unscaled training data in an existing recog.
 * </pre>
 */
L_RECOG *
recogCreateFromRecog(L_RECOG     *recs,
                     l_int32      scalew,
                     l_int32      scaleh,
                     l_int32      templ_type,
                     l_int32      threshold,
                     l_int32      maxyshift)
{
L_RECOG  *recd;
PIXA     *pixa;

    PROCNAME("recogCreateFromRecog");

    if (!recs)
        return (L_RECOG *)ERROR_PTR("recs not defined", procName, NULL);

    pixa = pixaaFlattenToPixa(recs->pixaa_u, NULL, L_CLONE);
    recd = recogCreateFromPixa(pixa, scalew, scaleh, templ_type, threshold,
                               maxyshift);
    pixaDestroy(&pixa);
    return recd;
}


/*!
 * \brief   recogCreateFromPixa()
 *
 * \param[in]    pixa of labelled, 1 bpp images
 * \param[in]    scalew  scale all widths to this; use 0 for no scaling
 * \param[in]    scaleh  scale all heights to this; use 0 for no scaling
 * \param[in]    templ_type L_USE_AVERAGE or L_USE_ALL
 * \param[in]    threshold for binarization; typically ~128
 * \param[in]    maxyshift from nominal centroid alignment; typically 0 or 1
 * \return  recog, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This is a convenience function for training from labelled data.
 *          The pixa can be read from file.
 *      (2) The pixa should contain the unscaled bitmaps used for training.
 *      (3) The characters here should work as a single "font", because
 *          each image example is put into a class defined by its
 *          character label.  All examples in the same class should be
 *          similar.
 * </pre>
 */
L_RECOG *
recogCreateFromPixa(PIXA        *pixa,
                    l_int32      scalew,
                    l_int32      scaleh,
                    l_int32      templ_type,
                    l_int32      threshold,
                    l_int32      maxyshift)
{
char     *text;
l_int32   full, n, i, ntext;
L_RECOG  *recog;
PIX      *pix;

    PROCNAME("recogCreateFromPixa");

    if (!pixa)
        return (L_RECOG *)ERROR_PTR("pixa not defined", procName, NULL);
    if (pixaVerifyDepth(pixa, NULL) != 1)
        return (L_RECOG *)ERROR_PTR("not all pix are 1 bpp", procName, NULL);

    pixaIsFull(pixa, &full, NULL);
    if (!full)
        return (L_RECOG *)ERROR_PTR("not all pix are present", procName, NULL);

    n = pixaGetCount(pixa);
    pixaCountText(pixa, &ntext);
    if (ntext == 0)
        return (L_RECOG *)ERROR_PTR("no pix have text strings", procName, NULL);
    if (ntext < n)
        L_ERROR("%d text strings < %d pix\n", procName, ntext, n);

    recog = recogCreate(scalew, scaleh, templ_type, threshold,
                        maxyshift);
    if (!recog)
        return (L_RECOG *)ERROR_PTR("recog not made", procName, NULL);
    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
        text = pixGetText(pix);
        if (!text || strlen(text) == 0) {
            L_ERROR("pix[%d] has no text\n", procName, i);
            pixDestroy(&pix);
            continue;
        }
        recogTrainLabelled(recog, pix, NULL, text, 0, 0);
        pixDestroy(&pix);
    }

    recogTrainingFinished(recog, 0);
    return recog;
}


/*!
 * \brief   recogCreate()
 *
 * \param[in]    scalew  scale all widths to this; use 0 for no scaling
 * \param[in]    scaleh  scale all heights to this; use 0 for no scaling
 * \param[in]    templ_type L_USE_AVERAGE or L_USE_ALL
 * \param[in]    threshold for binarization; typically ~128
 * \param[in]    maxyshift from nominal centroid alignment; typically 0 or 1
 * \return  recog, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) For a set trained on one font, such as numbers in a book,
 *          it is sensible to set scalew = scaleh = 0.
 *      (2) For a mixed training set, scaling to a fixed height,
 *          such as 32 pixels, but leaving the width unscaled, is effective.
 *      (3) The storage for most of the arrays is allocated when training
 *          is finished.
 * </pre>
 */
L_RECOG *
recogCreate(l_int32      scalew,
            l_int32      scaleh,
            l_int32      templ_type,
            l_int32      threshold,
            l_int32      maxyshift)
{
L_RECOG  *recog;
PIXA     *pixa;
PIXAA    *paa;

    PROCNAME("recogCreate");

    if (scalew < 0 || scaleh < 0)
        return (L_RECOG *)ERROR_PTR("invalid scalew or scaleh", procName, NULL);
    if (templ_type != L_USE_AVERAGE && templ_type != L_USE_ALL)
        return (L_RECOG *)ERROR_PTR("invalid templ_type flag", procName, NULL);
    if (threshold < 1 || threshold > 255)
        return (L_RECOG *)ERROR_PTR("invalid threshold", procName, NULL);

    if ((recog = (L_RECOG *)LEPT_CALLOC(1, sizeof(L_RECOG))) == NULL)
        return (L_RECOG *)ERROR_PTR("rec not made", procName, NULL);
    recog->templ_type = templ_type;
    recog->threshold = threshold;
    recog->scalew = scalew;
    recog->scaleh = scaleh;
    recog->maxyshift = maxyshift;
    recog->asperity_fr = DEFAULT_ASPERITY_FRACT;
    recogSetPadParams(recog, NULL, NULL, NULL, 0, -1, -1, -1, -1);
    recog->bmf = bmfCreate(NULL, 6);
    recog->bmf_size = 6;
    recog->maxarraysize = MAX_EXAMPLES_IN_CLASS;
    recog->index = -1;

        /* Generate the LUTs */
    recog->centtab = makePixelCentroidTab8();
    recog->sumtab = makePixelSumTab8();
    recog->sa_text = sarrayCreate(0);
    recog->dna_tochar = l_dnaCreate(0);

        /* Input default values for min component size for splitting.
         * These are overwritten when pixTrainingFinished() is called. */
    recog->min_splitw = 6;
    recog->min_splith = 6;
    recog->max_splith = 60;

        /* Generate the storage for the unscaled training bitmaps */
    paa = pixaaCreate(recog->maxarraysize);
    pixa = pixaCreate(1);
    pixaaInitFull(paa, pixa);
    pixaDestroy(&pixa);
    recog->pixaa_u = paa;

        /* Generate the storage for debugging */
    recog->pixadb_boot = pixaCreate(2);
    recog->pixadb_split = pixaCreate(2);
    return recog;
}


/*!
 * \brief   recogDestroy()
 *
 * \param[in,out]   precog will be set to null before returning
 * \return  void
 *
 * <pre>
 * Notes:
 *      (1) If a recog has a parent, the parent owns it.  A recogDestroy()
 *          will fail if there is a parent.
 * </pre>
 */
void
recogDestroy(L_RECOG  **precog)
{
L_RECOG  *recog;

    PROCNAME("recogDestroy");

    if (!precog) {
        L_WARNING("ptr address is null\n", procName);
        return;
    }

    if ((recog = *precog) == NULL) return;
    if (recogGetParent(recog) != NULL) {
        L_ERROR("recog has parent; can't be destroyed\n", procName);
        return;
    }

    LEPT_FREE(recog->bootdir);
    LEPT_FREE(recog->bootpattern);
    LEPT_FREE(recog->bootpath);
    LEPT_FREE(recog->centtab);
    LEPT_FREE(recog->sumtab);
    sarrayDestroy(&recog->sa_text);
    l_dnaDestroy(&recog->dna_tochar);
    pixaaDestroy(&recog->pixaa_u);
    pixaDestroy(&recog->pixa_u);
    ptaaDestroy(&recog->ptaa_u);
    ptaDestroy(&recog->pta_u);
    numaDestroy(&recog->nasum_u);
    numaaDestroy(&recog->naasum_u);
    pixaaDestroy(&recog->pixaa);
    pixaDestroy(&recog->pixa);
    ptaaDestroy(&recog->ptaa);
    ptaDestroy(&recog->pta);
    numaDestroy(&recog->nasum);
    numaaDestroy(&recog->naasum);
    pixaDestroy(&recog->pixa_tr);
    pixaDestroy(&recog->pixadb_ave);
    pixaDestroy(&recog->pixa_id);
    pixDestroy(&recog->pixdb_ave);
    pixDestroy(&recog->pixdb_range);
    pixaDestroy(&recog->pixadb_boot);
    pixaDestroy(&recog->pixadb_split);
    bmfDestroy(&recog->bmf);
    rchDestroy(&recog->rch);
    rchaDestroy(&recog->rcha);
    recogDestroyDid(recog);
    LEPT_FREE(recog);
    *precog = NULL;
    return;
}


/*------------------------------------------------------------------------*
 *                                Appending                               *
 *------------------------------------------------------------------------*/
/*!
 * \brief   recogAppend()
 *
 * \param[in]    recog1
 * \param[in]    recog2 gets added to recog1
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This is used to make a training recognizer from more than
 *          one trained recognizer source.  It should only be used
 *          when the bitmaps for corresponding character classes are
 *          very similar.  That constraint does not arise when
 *          the character classes are disjoint; e.g., if recog1 is
 *          digits and recog2 is alphabetical.
 *      (2) This is done by appending recog2 to recog1.  Averages are
 *          computed for each recognizer, if necessary, before appending.
 *      (3) Non-array fields are combined using the appropriate min and max.
 * </pre>
 */
l_int32
recogAppend(L_RECOG  *recog1,
            L_RECOG  *recog2)
{
    PROCNAME("recogAppend");

    if (!recog1)
        return ERROR_INT("recog1 not defined", procName, 1);
    if (!recog2)
        return ERROR_INT("recog2 not defined", procName, 1);

        /* Make sure both are finalized with all arrays computed */
    recogAverageSamples(recog1, 0);
    recogAverageSamples(recog2, 0);

        /* Combine non-array field values */
    recog1->minwidth_u = L_MIN(recog1->minwidth_u, recog2->minwidth_u);
    recog1->maxwidth_u = L_MAX(recog1->maxwidth_u, recog2->maxwidth_u);
    recog1->minheight_u = L_MIN(recog1->minheight_u, recog2->minheight_u);
    recog1->maxheight_u = L_MAX(recog1->maxheight_u, recog2->maxheight_u);
    recog1->minwidth = L_MIN(recog1->minwidth, recog2->minwidth);
    recog1->maxwidth = L_MAX(recog1->maxwidth, recog2->maxwidth);
    recog1->min_splitw = L_MIN(recog1->min_splitw, recog2->min_splitw);
    recog1->min_splith = L_MIN(recog1->min_splith, recog2->min_splith);
    recog1->max_splith = L_MAX(recog1->max_splith, recog2->max_splith);

        /* Combine array field values */
    recog1->setsize += recog2->setsize;
    sarrayAppendRange(recog1->sa_text, recog2->sa_text, 0, -1);
    l_dnaJoin(recog1->dna_tochar, recog2->dna_tochar, 0, -1);
    pixaaJoin(recog1->pixaa_u, recog2->pixaa_u, 0, -1);
    pixaJoin(recog1->pixa_u, recog2->pixa_u, 0, -1);
    ptaaJoin(recog1->ptaa_u, recog2->ptaa_u, 0, -1);
    ptaJoin(recog1->pta_u, recog2->pta_u, 0, -1);
    numaaJoin(recog1->naasum_u, recog2->naasum_u, 0, -1);
    numaJoin(recog1->nasum_u, recog2->nasum_u, 0, -1);
    pixaaJoin(recog1->pixaa, recog2->pixaa, 0, -1);
    pixaJoin(recog1->pixa, recog2->pixa, 0, -1);
    ptaaJoin(recog1->ptaa, recog2->ptaa, 0, -1);
    ptaJoin(recog1->pta, recog2->pta, 0, -1);
    numaaJoin(recog1->naasum, recog2->naasum, 0, -1);
    numaJoin(recog1->nasum, recog2->nasum, 0, -1);
    return 0;
}


/*------------------------------------------------------------------------*
 *                         Character/index lookup                         *
 *------------------------------------------------------------------------*/
/*!
 * \brief   recogGetClassIndex()
 *
 * \param[in]    recog with LUT's pre-computed
 * \param[in]    val integer value; can be up to 3 bytes for UTF-8
 * \param[in]    text text from which %val was derived; used if not found
 * \param[out]   pindex index into dna_tochar
 * \return  0 if found; 1 if not found and added; 2 on error.
 *
 * <pre>
 * Notes:
 *      (1) This is used during training.  It searches the
 *          dna character array for %val.  If not found, it increments
 *          the setsize by 1, augmenting both the index and text arrays.
 *      (2) Returns the index in &index, except on error.
 *      (3) Caller must check the function return value.
 * </pre>
 */
l_int32
recogGetClassIndex(L_RECOG  *recog,
                   l_int32   val,
                   char     *text,
                   l_int32  *pindex)
{
l_int32  i, n, ival;

    PROCNAME("recogGetClassIndex");

    if (!pindex)
        return ERROR_INT("&index not defined", procName, 2);
    *pindex = 0;
    if (!recog)
        return ERROR_INT("recog not defined", procName, 2);
    if (!text)
        return ERROR_INT("text not defined", procName, 2);

        /* Search existing characters */
    n = l_dnaGetCount(recog->dna_tochar);
    for (i = 0; i < n; i++) {
        l_dnaGetIValue(recog->dna_tochar, i, &ival);
        if (val == ival) {  /* found */
            *pindex = i;
            return 0;
        }
    }

       /* If not found... */
    l_dnaAddNumber(recog->dna_tochar, val);
    sarrayAddString(recog->sa_text, text, L_COPY);
    recog->setsize++;
    *pindex = n;
    return 1;
}


/*!
 * \brief   recogStringToIndex()
 *
 * \param[in]    recog
 * \param[in]    text text string for some class
 * \param[out]   pindex index for that class; -1 if not found
 * \return  0 if OK, 1 on error not finding the string is an error
 */
l_int32
recogStringToIndex(L_RECOG  *recog,
                   char     *text,
                   l_int32  *pindex)
{
char    *charstr;
l_int32  i, n, diff;

    PROCNAME("recogStringtoIndex");

    if (!pindex)
        return ERROR_INT("&index not defined", procName, 1);
    *pindex = -1;
    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);
    if (!text)
        return ERROR_INT("text not defined", procName, 1);

        /* Search existing characters */
    n = recog->setsize;
    for (i = 0; i < n; i++) {
        recogGetClassString(recog, i, &charstr);
        if (!charstr) {
            L_ERROR("string not found for index %d\n", procName, i);
            continue;
        }
        diff = strcmp(text, charstr);
        LEPT_FREE(charstr);
        if (diff) continue;
        *pindex = i;
        return 0;
    }

    return 1;  /* not found */
}


/*!
 * \brief   recogGetClassString()
 *
 * \param[in]    recog
 * \param[in]    index into array of char types
 * \param[out]   pcharstr string representation;
 *                        returns an empty string on error
 * \return  0 if found, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Extracts a copy of the string from sa_text, which
 *          the caller must free.
 *      (2) Caller must check the function return value.
 * </pre>
 */
l_int32
recogGetClassString(L_RECOG  *recog,
                    l_int32   index,
                    char    **pcharstr)
{
    PROCNAME("recogGetClassString");

    if (!pcharstr)
        return ERROR_INT("&charstr not defined", procName, 1);
    *pcharstr = stringNew("");
    if (!recog)
        return ERROR_INT("recog not defined", procName, 2);

    if (index < 0 || index >= recog->setsize)
        return ERROR_INT("invalid index", procName, 1);
    LEPT_FREE(*pcharstr);
    *pcharstr = sarrayGetString(recog->sa_text, index, L_COPY);
    return 0;
}


/*!
 * \brief   l_convertCharstrToInt()
 *
 * \param[in]    str input string representing one UTF-8 character;
 *                   not more than 4 bytes
 * \param[out]   pval integer value for the input.  Think of it
 *                    as a 1-to-1 hash code.
 * \return  0 if OK, 1 on error
 */
l_int32
l_convertCharstrToInt(const char  *str,
                      l_int32     *pval)
{
l_int32  size, val;

    PROCNAME("l_convertCharstrToInt");

    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = 0;
    if (!str)
        return ERROR_INT("str not defined", procName, 1);
    size = strlen(str);
    if (size == 0)
        return ERROR_INT("empty string", procName, 1);
    if (size > 4)
        return ERROR_INT("invalid string: > 4 bytes", procName, 1);

    val = (l_int32)str[0];
    if (size > 1)
        val = (val << 8) + (l_int32)str[1];
    if (size > 2)
        val = (val << 8) + (l_int32)str[2];
    if (size > 3)
        val = (val << 8) + (l_int32)str[3];
    *pval = val;
    return 0;
}


/*------------------------------------------------------------------------*
 *                             Serialization                              *
 *------------------------------------------------------------------------*/
/*!
 * \brief   recogaRead()
 *
 * \param[in]    filename
 * \return  recoga, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This allows serialization of an array of recognizers, each of which
 *          can be used for different fonts, font styles, etc.
 * </pre>
 */
L_RECOGA *
recogaRead(const char  *filename)
{
FILE     *fp;
L_RECOGA  *recoga;

    PROCNAME("recogaRead");

    if (!filename)
        return (L_RECOGA *)ERROR_PTR("filename not defined", procName, NULL);
    if ((fp = fopenReadStream(filename)) == NULL)
        return (L_RECOGA *)ERROR_PTR("stream not opened", procName, NULL);

    if ((recoga = recogaReadStream(fp)) == NULL) {
        fclose(fp);
        return (L_RECOGA *)ERROR_PTR("recoga not read", procName, NULL);
    }

    fclose(fp);
    return recoga;
}


/*!
 * \brief   recogaReadStream()
 *
 * \param[in]    fp file stream
 * \return  recog, or NULL on error
 */
L_RECOGA *
recogaReadStream(FILE  *fp)
{
l_int32    version, i, nrec, ignore;
L_RECOG   *recog;
L_RECOGA  *recoga;

    PROCNAME("recogaReadStream");

    if (!fp)
        return (L_RECOGA *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nRecoga Version %d\n", &version) != 1)
        return (L_RECOGA *)ERROR_PTR("not a recog file", procName, NULL);
    if (version != RECOG_VERSION_NUMBER)
        return (L_RECOGA *)ERROR_PTR("invalid recog version", procName, NULL);
    if (fscanf(fp, "Number of recognizers = %d\n\n", &nrec) != 1)
        return (L_RECOGA *)ERROR_PTR("nrec not read", procName, NULL);

    recoga = recogaCreate(nrec);
    for (i = 0; i < nrec; i++) {
        ignore = fscanf(fp, "==============================\n");
        if (fscanf(fp, "Recognizer %d\n", &ignore) != 1)
            return (L_RECOGA *)ERROR_PTR("malformed file", procName, NULL);
        if ((recog = recogReadStream(fp)) == NULL) {
            recogaDestroy(&recoga);
            L_ERROR("recog read failed for recog %d\n", procName, i);
            return NULL;
        }
        ignore = fscanf(fp, "\n");
        recogaAddRecog(recoga, recog);
    }
    return recoga;
}


/*!
 * \brief   recogaReadMem()
 *
 * \param[in]    data  serialization of recoga (not ascii)
 * \param[in]    size  of data in bytes
 * \return  recoga, or NULL on error
 */
L_RECOGA *
recogaReadMem(const l_uint8  *data,
              size_t          size)
{
FILE      *fp;
L_RECOGA  *recoga;

    PROCNAME("recogaReadMem");

    if (!data)
        return (L_RECOGA *)ERROR_PTR("data not defined", procName, NULL);
    if ((fp = fopenReadFromMemory(data, size)) == NULL)
        return (L_RECOGA *)ERROR_PTR("stream not opened", procName, NULL);

    recoga = recogaReadStream(fp);
    fclose(fp);
    if (!recoga) L_ERROR("recoga not read\n", procName);
    return recoga;
}


/*!
 * \brief   recogaWrite()
 *
 * \param[in]    filename
 * \param[in]    recoga
 * \return  0 if OK, 1 on error
 */
l_int32
recogaWrite(const char  *filename,
            L_RECOGA    *recoga)
{
FILE  *fp;

    PROCNAME("recogaWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!recoga)
        return ERROR_INT("recoga not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "wb")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (recogaWriteStream(fp, recoga))
        return ERROR_INT("recoga not written to stream", procName, 1);
    fclose(fp);
    return 0;
}


/*!
 * \brief   recogaWriteStream()
 *
 * \param[in]    fp file stream opened for "wb"
 * \param[in]    recoga
 * \return  0 if OK, 1 on error
 */
l_int32
recogaWriteStream(FILE      *fp,
                  L_RECOGA  *recoga)
{
l_int32   i;
L_RECOG  *recog;

    PROCNAME("recogaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!recoga)
        return ERROR_INT("recoga not defined", procName, 1);

    fprintf(fp, "\nRecoga Version %d\n", RECOG_VERSION_NUMBER);
    fprintf(fp, "Number of recognizers = %d\n\n", recoga->n);

    for (i = 0; i < recoga->n; i++) {
        fprintf(fp, "==============================\n");
        fprintf(fp, "Recognizer %d\n", i);
        recog = recogaGetRecog(recoga, i);
        recogWriteStream(fp, recog);
        fprintf(fp, "\n");
    }

    return 0;
}


/*!
 * \brief   recogaWriteMem()
 *
 * \param[out]   pdata data of serialized recoga (not ascii)
 * \param[out]   psize size of returned data
 * \param[in]    recoga
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Serializes a recoga in memory and puts the result in a buffer.
 * </pre>
 */
l_int32
recogaWriteMem(l_uint8  **pdata,
               size_t    *psize,
               L_RECOGA  *recoga)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("recogaWriteMem");

    if (pdata) *pdata = NULL;
    if (psize) *psize = 0;
    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    if (!psize)
        return ERROR_INT("&size not defined", procName, 1);
    if (!recoga)
        return ERROR_INT("recoga not defined", procName, 1);

#if HAVE_FMEMOPEN
    if ((fp = open_memstream((char **)pdata, psize)) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    ret = recogaWriteStream(fp, recoga);
#else
    L_INFO("work-around: writing to a temp file\n", procName);
  #ifdef _WIN32
    if ((fp = fopenWriteWinTempfile()) == NULL)
        return ERROR_INT("tmpfile stream not opened", procName, 1);
  #else
    if ((fp = tmpfile()) == NULL)
        return ERROR_INT("tmpfile stream not opened", procName, 1);
  #endif  /* _WIN32 */
    ret = recogaWriteStream(fp, recoga);
    rewind(fp);
    *pdata = l_binaryReadStream(fp, psize);
#endif  /* HAVE_FMEMOPEN */
    fclose(fp);
    return ret;
}


/*!
 * \brief   recogaWritePixaa()
 *
 * \param[in]    filename
 * \param[in]    recoga
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) For each recognizer, this generates a pixa of all the
 *          unscaled images.  They are combined into a pixaa for
 *          the set of recognizers.  Each pix has has its character
 *          string in the pix text field.
 *      (2) As a side-effect, the character class label is written
 *          into each pix in recog.
 * </pre>
 */
l_int32
recogaWritePixaa(const char  *filename,
                 L_RECOGA    *recoga)
{
l_int32   i;
PIXA     *pixa;
PIXAA    *paa;
L_RECOG  *recog;

    PROCNAME("recogaWritePixaa");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!recoga)
        return ERROR_INT("recoga not defined", procName, 1);

    paa = pixaaCreate(recoga->n);
    for (i = 0; i < recoga->n; i++) {
        recog = recogaGetRecog(recoga, i);
        recogAddCharstrLabels(recog);
        pixa = pixaaFlattenToPixa(recog->pixaa_u, NULL, L_CLONE);
        pixaaAddPixa(paa, pixa, L_INSERT);
    }
    pixaaWrite(filename, paa);
    pixaaDestroy(&paa);
    return 0;
}


/*!
 * \brief   recogRead()
 *
 * \param[in]    filename
 * \return  recog, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Serialization can be applied to any recognizer, including
 *          one with more than one "font".  That is, it can have
 *          multiple character classes with the same character set
 *          description, where each of those classes contains characters
 *          that are very similar in size and shape.  Each pixa in
 *          the serialized pixaa contains images for a single character
 *          class.
 * </pre>
 */
L_RECOG *
recogRead(const char  *filename)
{
FILE     *fp;
L_RECOG  *recog;

    PROCNAME("recogRead");

    if (!filename)
        return (L_RECOG *)ERROR_PTR("filename not defined", procName, NULL);
    if ((fp = fopenReadStream(filename)) == NULL)
        return (L_RECOG *)ERROR_PTR("stream not opened", procName, NULL);

    if ((recog = recogReadStream(fp)) == NULL) {
        fclose(fp);
        return (L_RECOG *)ERROR_PTR("recog not read", procName, NULL);
    }

    fclose(fp);
    return recog;
}


/*!
 * \brief   recogReadStream()
 *
 * \param[in]    fp file stream
 * \return  recog, or NULL on error
 */
L_RECOG *
recogReadStream(FILE  *fp)
{
l_int32   version, setsize, templ_type, threshold, scalew, scaleh;
l_int32   maxyshift, nc;
L_DNA    *dna_tochar;
PIXAA    *paa;
L_RECOG  *recog;
SARRAY   *sa_text;

    PROCNAME("recogReadStream");

    if (!fp)
        return (L_RECOG *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nRecog Version %d\n", &version) != 1)
        return (L_RECOG *)ERROR_PTR("not a recog file", procName, NULL);
    if (version != RECOG_VERSION_NUMBER)
        return (L_RECOG *)ERROR_PTR("invalid recog version", procName, NULL);
    if (fscanf(fp, "Size of character set = %d\n", &setsize) != 1)
        return (L_RECOG *)ERROR_PTR("setsize not read", procName, NULL);
    if (fscanf(fp, "Template type = %d\n", &templ_type) != 1)
        return (L_RECOG *)ERROR_PTR("template type not read", procName, NULL);
    if (fscanf(fp, "Binarization threshold = %d\n", &threshold) != 1)
        return (L_RECOG *)ERROR_PTR("binary thresh not read", procName, NULL);
    if (fscanf(fp, "Maxyshift = %d\n", &maxyshift) != 1)
        return (L_RECOG *)ERROR_PTR("maxyshift not read", procName, NULL);
    if (fscanf(fp, "Scale to width = %d\n", &scalew) != 1)
        return (L_RECOG *)ERROR_PTR("width not read", procName, NULL);
    if (fscanf(fp, "Scale to height = %d\n", &scaleh) != 1)
        return (L_RECOG *)ERROR_PTR("height not read", procName, NULL);
    if ((recog = recogCreate(scalew, scaleh, templ_type, threshold,
                             maxyshift)) == NULL)
        return (L_RECOG *)ERROR_PTR("recog not made", procName, NULL);

    if (fscanf(fp, "\nLabels for character set:\n") != 0)
        return (L_RECOG *)ERROR_PTR("label intro not read", procName, NULL);
    l_dnaDestroy(&recog->dna_tochar);
    sarrayDestroy(&recog->sa_text);
    if ((dna_tochar = l_dnaReadStream(fp)) == NULL)
        return (L_RECOG *)ERROR_PTR("dna_tochar not read", procName, NULL);
    if ((sa_text = sarrayReadStream(fp)) == NULL)
        return (L_RECOG *)ERROR_PTR("sa_text not read", procName, NULL);
    recog->sa_text = sa_text;
    recog->dna_tochar = dna_tochar;

    if (fscanf(fp, "\nPixaa of all samples in the training set:\n") != 0)
        return (L_RECOG *)ERROR_PTR("pixaa intro not read", procName, NULL);
    if ((paa = pixaaReadStream(fp)) == NULL)
        return (L_RECOG *)ERROR_PTR("pixaa not read", procName, NULL);
    recog->setsize = setsize;
    nc = pixaaGetCount(paa, NULL);
    if (nc != setsize) {
        L_ERROR("(setsize = %d) != (paa count = %d)\n", procName,
                     setsize, nc);
        return NULL;
    }

    recogAddAllSamples(recog, paa, 0);  /* this finishes */
    pixaaDestroy(&paa);
    return recog;
}


/*!
 * \brief   recogReadMem()
 *
 * \param[in]    data  serialization of recog (not ascii)
 * \param[in]    size  of data in bytes
 * \return  recog, or NULL on error
 */
L_RECOG *
recogReadMem(const l_uint8  *data,
             size_t          size)
{
FILE     *fp;
L_RECOG  *recog;

    PROCNAME("recogReadMem");

    if (!data)
        return (L_RECOG *)ERROR_PTR("data not defined", procName, NULL);
    if ((fp = fopenReadFromMemory(data, size)) == NULL)
        return (L_RECOG *)ERROR_PTR("stream not opened", procName, NULL);

    recog = recogReadStream(fp);
    fclose(fp);
    if (!recog) L_ERROR("recog not read\n", procName);
    return recog;
}


/*!
 * \brief   recogWrite()
 *
 * \param[in]    filename
 * \param[in]    recog
 * \return  0 if OK, 1 on error
 */
l_int32
recogWrite(const char  *filename,
           L_RECOG     *recog)
{
FILE  *fp;

    PROCNAME("recogWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "wb")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (recogWriteStream(fp, recog))
        return ERROR_INT("recog not written to stream", procName, 1);
    fclose(fp);
    return 0;
}


/*!
 * \brief   recogWriteStream()
 *
 * \param[in]    fp file stream opened for "wb"
 * \param[in]    recog
 * \return  0 if OK, 1 on error
 */
l_int32
recogWriteStream(FILE     *fp,
                 L_RECOG  *recog)
{
    PROCNAME("recogWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);

    fprintf(fp, "\nRecog Version %d\n", RECOG_VERSION_NUMBER);
    fprintf(fp, "Size of character set = %d\n", recog->setsize);
    fprintf(fp, "Template type = %d\n", recog->templ_type);
    fprintf(fp, "Binarization threshold = %d\n", recog->threshold);
    fprintf(fp, "Maxyshift = %d\n", recog->maxyshift);
    fprintf(fp, "Scale to width = %d\n", recog->scalew);
    fprintf(fp, "Scale to height = %d\n", recog->scaleh);
    fprintf(fp, "\nLabels for character set:\n");
    l_dnaWriteStream(fp, recog->dna_tochar);
    sarrayWriteStream(fp, recog->sa_text);
    fprintf(fp, "\nPixaa of all samples in the training set:\n");
    pixaaWriteStream(fp, recog->pixaa);

    return 0;
}


/*!
 * \brief   recogWriteMem()
 *
 * \param[out]   pdata data of serialized recog (not ascii)
 * \param[out]   psize size of returned data
 * \param[in]    recog
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Serializes a recog in memory and puts the result in a buffer.
 * </pre>
 */
l_int32
recogWriteMem(l_uint8  **pdata,
              size_t    *psize,
              L_RECOG   *recog)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("recogWriteMem");

    if (pdata) *pdata = NULL;
    if (psize) *psize = 0;
    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    if (!psize)
        return ERROR_INT("&size not defined", procName, 1);
    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);

#if HAVE_FMEMOPEN
    if ((fp = open_memstream((char **)pdata, psize)) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    ret = recogWriteStream(fp, recog);
#else
    L_INFO("work-around: writing to a temp file\n", procName);
  #ifdef _WIN32
    if ((fp = fopenWriteWinTempfile()) == NULL)
        return ERROR_INT("tmpfile stream not opened", procName, 1);
  #else
    if ((fp = tmpfile()) == NULL)
        return ERROR_INT("tmpfile stream not opened", procName, 1);
  #endif  /* _WIN32 */
    ret = recogWriteStream(fp, recog);
    rewind(fp);
    *pdata = l_binaryReadStream(fp, psize);
#endif  /* HAVE_FMEMOPEN */
    fclose(fp);
    return ret;
}


/*!
 * \brief   recogExtractPixa()
 *
 * \param[in]   recog
 * \return  pixa if OK, NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This generates a pixa of all the unscaled images in the
 *          recognizer, where each one has its character string in
 *          the pix text field, by flattening pixaa_u to a pixa.
 *      (2) As a side-effect, the character class label is written
 *          into each pix in recog.
 * </pre>
 */
PIXA *
recogExtractPixa(L_RECOG  *recog)
{
    PROCNAME("recogExtractPixa");

    if (!recog)
        return (PIXA *)ERROR_PTR("recog not defined", procName, NULL);

    recogAddCharstrLabels(recog);
    return pixaaFlattenToPixa(recog->pixaa_u, NULL, L_CLONE);
}


/*!
 * \brief   recogAddCharstrLabels()
 *
 * \param[in]    recog
 * \return  0 if OK, 1 on error
 */
static l_int32
recogAddCharstrLabels(L_RECOG  *recog)
{
char    *text;
l_int32  i, j, n1, n2;
PIX     *pix;
PIXA    *pixa;
PIXAA   *paa;

    PROCNAME("recogAddCharstrLabels");

    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);

        /* Add the labels to each unscaled pix */
    paa = recog->pixaa_u;
    n1 = pixaaGetCount(paa, NULL);
    for (i = 0; i < n1; i++) {
        pixa = pixaaGetPixa(paa, i, L_CLONE);
        text = sarrayGetString(recog->sa_text, i, L_NOCOPY);
        n2 = pixaGetCount(pixa);
        for (j = 0; j < n2; j++) {
             pix = pixaGetPix(pixa, j, L_CLONE);
             pixSetText(pix, text);
             pixDestroy(&pix);
        }
        pixaDestroy(&pixa);
    }

    return 0;
}


/*!
 * \brief   recogAddAllSamples()
 *
 * \param[in]    recog
 * \param[in]    paa pixaa from previously trained recog
 * \param[in]    debug
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This is used with the serialization routine recogRead(),
 *          where each pixa in the pixaa represents a set of characters
 *          in a different class.  Two different pixa may represent
 *          characters with the same label.  Before calling this
 *          function, we verify that the number of character classes,
 *          given by the setsize field in recog, equals the number of
 *          pixa in the paa.  The character labels for each set are
 *          in the sa_text field.
 * </pre>
 */
static l_int32
recogAddAllSamples(L_RECOG  *recog,
                   PIXAA    *paa,
                   l_int32   debug)
{
char    *text;
l_int32  i, j, nc, ns;
PIX     *pix;
PIXA    *pixa;

    PROCNAME("recogAddAllSamples");

    if (!recog)
        return ERROR_INT("recog not defined", procName, 1);
    if (!paa)
        return ERROR_INT("paa not defined", procName, 1);

    nc = pixaaGetCount(paa, NULL);
    for (i = 0; i < nc; i++) {
        pixa = pixaaGetPixa(paa, i, L_CLONE);
        ns = pixaGetCount(pixa);
        text = sarrayGetString(recog->sa_text, i, L_NOCOPY);
        for (j = 0; j < ns; j++) {
            pix = pixaGetPix(pixa, j, L_CLONE);
            if (debug) {
                fprintf(stderr, "pix[%d,%d]: text = %s\n", i, j, text);
            }
            pixaaAddPix(recog->pixaa_u, i, pix, NULL, L_INSERT);
        }
        pixaDestroy(&pixa);
    }

    recogTrainingFinished(recog, debug);
    return 0;
}
