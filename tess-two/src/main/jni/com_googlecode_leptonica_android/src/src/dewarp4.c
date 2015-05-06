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
 *  dewarp4.c
 *
 *    Single page dewarper
 *
 *    Reference model (book-level, dewarpa) operations and debugging output
 *
 *      Top-level single page dewarper
 *          l_int32            dewarpSinglePage()
 *
 *      Operations on dewarpa
 *          l_int32            dewarpaListPages()
 *          l_int32            dewarpaSetValidModels()
 *          l_int32            dewarpaInsertRefModels()
 *          l_int32            dewarpaStripRefModels()
 *          l_int32            dewarpaRestoreModels()
 *
 *      Dewarp debugging output
 *          l_int32            dewarpaInfo()
 *          l_int32            dewarpaModelStats()
 *          static l_int32     dewarpaTestForValidModel()
 *          l_int32            dewarpaShowArrays()
 *          l_int32            dewarpDebug()
 *          l_int32            dewarpShowResults()
 */

#include <math.h>
#include "allheaders.h"

static l_int32 dewarpaTestForValidModel(L_DEWARPA *dewa, L_DEWARP *dew,
                                        l_int32 notests);

    /* Special parameter values */
static const l_int32     GRAYIN_VALUE = 200;


/*----------------------------------------------------------------------*
 *                   Top-level single page dewarper                     *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpSinglePage()
 *
 *      Input:  pixs (with text, any depth)
 *              thresh (for global thresholding to 1 bpp; ignored otherwise)
 *              adaptive (1 for adaptive thresholding; 0 for global threshold)
 *              use_both (1 for horizontal and vertical; 0 for vertical only)
 *              &pixd (<return> dewarped result)
 *              &dewa (<optional return> dewa with single page; NULL to skip)
 *              debug (1 for debugging output, 0 otherwise)
 *      Return: 0 if OK, 1 on error (list of page numbers), or null on error
 *
 *  Notes:
 *      (1) Dewarps pixs and returns the result in &pixd.
 *      (2) This uses default values for all model parameters.
 *      (3) If pixs is 1 bpp, the parameters @adaptive and @thresh are ignored.
 *      (4) If it can't build a model, returns a copy of pixs in &pixd.
 */
l_int32
dewarpSinglePage(PIX         *pixs,
                 l_int32      thresh,
                 l_int32      adaptive,
                 l_int32      use_both,
                 PIX        **ppixd,
                 L_DEWARPA  **pdewa,
                 l_int32      debug)
{
const char  *debugfile;
l_int32      vsuccess, ret;
L_DEWARP    *dew;
L_DEWARPA   *dewa;
PIX         *pix1, *pixb;

    PROCNAME("dewarpSinglePage");

    if (!ppixd)
        return ERROR_INT("&pixd not defined", procName, 1);
    *ppixd = NULL;
    if (pdewa) *pdewa = NULL;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

    dewa = dewarpaCreate(1, 0, 1, 0, -1);
    dewarpaUseBothArrays(dewa, use_both);

         /* Generate a binary image, if necessary */
    if (pixGetDepth(pixs) > 1) {
        pix1 = pixConvertTo8(pixs, 0);
        if (adaptive)
            pixb = pixAdaptThresholdToBinary(pix1, NULL, 1.0);
        else
            pixb = pixThresholdToBinary(pix1, thresh);
        pixDestroy(&pix1);
    } else {
        pixb = pixClone(pixs);
    }

        /* Generate the page model */
    lept_mkdir("lept");
    dew = dewarpCreate(pixb, 0);
    dewarpaInsertDewarp(dewa, dew);
    debugfile = (debug) ? "/tmp/lept/singlepage_model.pdf" : NULL;
    dewarpBuildPageModel(dew, debugfile);
    dewarpaModelStatus(dewa, 0, &vsuccess, NULL);
    if (vsuccess == 0) {
        L_ERROR("failure to build model\n", procName);
        pixDestroy(&pixb);
        dewarpaDestroy(&dewa);
        *ppixd = pixCopy(NULL, pixs);
        return 0;
    }

        /* Apply the page model */
    debugfile = (debug) ? "/tmp/lept/singlepage_apply.pdf" : NULL;
    ret = dewarpaApplyDisparity(dewa, 0, pixs, 255, 0, 0, ppixd, debugfile);
    if (ret)
        L_ERROR("invalid model; failure to apply disparity\n", procName);
    if (pdewa)
        *pdewa = dewa;
    else
        dewarpaDestroy(&dewa);
    pixDestroy(&pixb);
    return 0;
}


/*----------------------------------------------------------------------*
 *                        Operations on dewarpa                         *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpaListPages()
 *
 *      Input:  dewa (populated with dewarp structs for pages)
 *      Return: 0 if OK, 1 on error (list of page numbers), or null on error
 *
 *  Notes:
 *      (1) This generates two numas, stored in the dewarpa, that give:
 *          (a) the page number for each dew that has a page model.
 *          (b) the page number for each dew that has either a page
 *              model or a reference model.
 *          It can be called at any time.
 *      (2) It is called by the dewarpa serializer before writing.
 */
l_int32
dewarpaListPages(L_DEWARPA  *dewa)
{
l_int32    i;
L_DEWARP  *dew;
NUMA      *namodels, *napages;

    PROCNAME("dewarpaListPages");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    numaDestroy(&dewa->namodels);
    numaDestroy(&dewa->napages);
    namodels = numaCreate(dewa->maxpage + 1);
    napages = numaCreate(dewa->maxpage + 1);
    dewa->namodels = namodels;
    dewa->napages = napages;
    for (i = 0; i <= dewa->maxpage; i++) {
        if ((dew = dewarpaGetDewarp(dewa, i)) != NULL) {
            if (dew->hasref == 0)
                numaAddNumber(namodels, dew->pageno);
            numaAddNumber(napages, dew->pageno);
        }
    }
    return 0;
}


/*!
 *  dewarpaSetValidModels()
 *
 *      Input:  dewa
 *              notests
 *              debug (1 to output information on invalid page models)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) A valid model must meet the rendering requirements, which
 *          include whether or not a vertical disparity model exists
 *          and conditions on curvatures for vertical and horizontal
 *          disparity models.
 *      (2) If @notests == 1, this ignores the curvature constraints
 *          and assumes that all successfully built models are valid.
 *      (3) This function does not need to be called by the application.
 *          It is called by dewarpaInsertRefModels(), which
 *          will destroy all invalid dewarps.  Consequently, to inspect
 *          an invalid dewarp model, it must be done before calling
 *          dewarpaInsertRefModels().
 */
l_int32
dewarpaSetValidModels(L_DEWARPA  *dewa,
                      l_int32     notests,
                      l_int32     debug)
{
l_int32    i, n, maxcurv, diffcurv, diffedge;
L_DEWARP  *dew;

    PROCNAME("dewarpaSetValidModels");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    n = dewa->maxpage + 1;
    for (i = 0; i < n; i++) {
        if ((dew = dewarpaGetDewarp(dewa, i)) == NULL)
            continue;

        if (debug) {
            if (dew->hasref == 1) {
                L_INFO("page %d: has only a ref model\n", procName, i);
            } else if (dew->vsuccess == 0) {
                L_INFO("page %d: no model successfully built\n",
                       procName, i);
            } else if (!notests) {
                maxcurv = L_MAX(L_ABS(dew->mincurv), L_ABS(dew->maxcurv));
                diffcurv = dew->maxcurv - dew->mincurv;
                if (dewa->useboth && !dew->hsuccess)
                    L_INFO("page %d: useboth, but no horiz disparity\n",
                               procName, i);
                if (maxcurv > dewa->max_linecurv)
                    L_INFO("page %d: max curvature %d > max_linecurv\n",
                                procName, i, diffcurv);
                if (diffcurv < dewa->min_diff_linecurv)
                    L_INFO("page %d: diff curv %d < min_diff_linecurv\n",
                                procName, i, diffcurv);
                if (diffcurv > dewa->max_diff_linecurv)
                    L_INFO("page %d: abs diff curv %d > max_diff_linecurv\n",
                                procName, i, diffcurv);
                if (dew->hsuccess) {
                    if (L_ABS(dew->leftslope) > dewa->max_edgeslope)
                        L_INFO("page %d: abs left slope %d > max_edgeslope\n",
                                    procName, i, dew->leftslope);
                    if (L_ABS(dew->rightslope) > dewa->max_edgeslope)
                        L_INFO("page %d: abs right slope %d > max_edgeslope\n",
                                    procName, i, dew->rightslope);
                    diffedge = L_ABS(dew->leftcurv - dew->rightcurv);
                    if (L_ABS(dew->leftcurv) > dewa->max_edgecurv)
                        L_INFO("page %d: left curvature %d > max_edgecurv\n",
                                    procName, i, dew->leftcurv);
                    if (L_ABS(dew->rightcurv) > dewa->max_edgecurv)
                        L_INFO("page %d: right curvature %d > max_edgecurv\n",
                               procName, i, dew->rightcurv);
                    if (diffedge > dewa->max_diff_edgecurv)
                        L_INFO("page %d: abs diff left-right curv %d > "
                               "max_diff_edgecurv\n", procName, i, diffedge);
                }
            }
        }

        dewarpaTestForValidModel(dewa, dew, notests);
    }

    return 0;
}


/*!
 *  dewarpaInsertRefModels()
 *
 *      Input:  dewa
 *              notests (if 1, ignore curvature constraints on model)
 *              debug (1 to output information on invalid page models)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This destroys all dewarp models that are invalid, and then
 *          inserts reference models where possible.
 *      (2) If @notests == 1, this ignores the curvature constraints
 *          and assumes that all successfully built models are valid.
 *      (3) If useboth == 0, it uses the closest valid model within the
 *          distance and parity constraints.  If useboth == 1, it tries
 *          to use the closest allowed hvalid model; if it doesn't find
 *          an hvalid model, it uses the closest valid model.
 *      (4) For all pages without a model, this clears out any existing
 *          invalid and reference dewarps, finds the nearest valid model
 *          with the same parity, and inserts an empty dewarp with the
 *          reference page.
 *      (5) Then if it is requested to use both vertical and horizontal
 *          disparity arrays (useboth == 1), it tries to replace any
 *          hvalid == 0 model or reference with an hvalid == 1 reference.
 *      (6) The distance constraint is that any reference model must
 *          be within maxdist.  Note that with the parity constraint,
 *          no reference models will be used if maxdist < 2.
 *      (7) This function must be called, even if reference models will
 *          not be used.  It should be called after building models on all
 *          available pages, and after setting the rendering parameters.
 *      (8) If the dewa has been serialized, this function is called by
 *          dewarpaRead() when it is read back.  It is also called
 *          any time the rendering parameters are changed.
 *      (9) Note: if this has been called with useboth == 1, and useboth
 *          is reset to 0, you should first call dewarpRestoreModels()
 *          to bring real models from the cache back to the primary array.
 */
l_int32
dewarpaInsertRefModels(L_DEWARPA  *dewa,
                       l_int32     notests,
                       l_int32     debug)
{
l_int32    i, j, n, val, min, distdown, distup;
L_DEWARP  *dew;
NUMA      *na, *nah;

    PROCNAME("dewarpaInsertRefModels");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);
    if (dewa->maxdist < 2)
        L_INFO("maxdist < 2; no ref models can be used\n", procName);

        /* Make an indicator numa for pages with valid models. */
    dewarpaSetValidModels(dewa, notests, debug);
    n = dewa->maxpage + 1;
    na = numaMakeConstant(0, n);
    for (i = 0; i < n; i++) {
        dew = dewarpaGetDewarp(dewa, i);
        if (dew && dew->vvalid)
            numaReplaceNumber(na, i, 1);
    }

        /* Remove all existing ref models and restore models from cache */
    dewarpaRestoreModels(dewa);

        /* Move invalid models to the cache, and insert reference dewarps
         * for pages that need to borrow a model.
         * First, try to find a valid model for each page. */
    for (i = 0; i < n; i++) {
        numaGetIValue(na, i, &val);
        if (val == 1) continue;  /* already has a valid model */
        if ((dew = dewa->dewarp[i]) != NULL) {  /* exists but is not valid; */
            dewa->dewarpcache[i] = dew;  /* move it to the cache */
            dewa->dewarp[i] = NULL;
        }
        if (dewa->maxdist < 2) continue;  /* can't use a ref model */
            /* Look back for nearest model */
        distdown = distup = dewa->maxdist + 1;
        for (j = i - 2; j >= 0 && distdown > dewa->maxdist; j -= 2) {
            numaGetIValue(na, j, &val);
            if (val == 1) distdown = i - j;
        }
            /* Look ahead for nearest model */
        for (j = i + 2; j < n && distup > dewa->maxdist; j += 2) {
            numaGetIValue(na, j, &val);
            if (val == 1) distup = j - i;
        }
        min = L_MIN(distdown, distup);
        if (min > dewa->maxdist) continue;  /* no valid model in range */
        if (distdown <= distup)
            dewarpaInsertDewarp(dewa, dewarpCreateRef(i, i - distdown));
        else
            dewarpaInsertDewarp(dewa, dewarpCreateRef(i, i + distup));
    }
    numaDestroy(&na);

        /* If a valid model will do, we're finished. */
    if (dewa->useboth == 0) {
        dewa->modelsready = 1;  /* validated */
        return 0;
    }

        /* The request is useboth == 1.  Now try to find an hvalid model */
    nah = numaMakeConstant(0, n);
    for (i = 0; i < n; i++) {
        dew = dewarpaGetDewarp(dewa, i);
        if (dew && dew->hvalid)
            numaReplaceNumber(nah, i, 1);
    }
    for (i = 0; i < n; i++) {
        numaGetIValue(nah, i, &val);
        if (val == 1) continue;  /* already has a hvalid model */
        if (dewa->maxdist < 2) continue;  /* can't use a ref model */
        distdown = distup = 100000;
        for (j = i - 2; j >= 0; j -= 2) {  /* look back for nearest model */
            numaGetIValue(nah, j, &val);
            if (val == 1) {
                distdown = i - j;
                break;
            }
        }
        for (j = i + 2; j < n; j += 2) {  /* look ahead for nearest model */
            numaGetIValue(nah, j, &val);
            if (val == 1) {
                distup = j - i;
                break;
            }
        }
        min = L_MIN(distdown, distup);
        if (min > dewa->maxdist) continue;  /* no hvalid model within range */

            /* We can replace the existing valid model with an hvalid model.
             * If it's not a reference, save it in the cache. */
        if ((dew = dewarpaGetDewarp(dewa, i)) == NULL) {
            L_ERROR("dew is null for page %d!\n", procName, i);
        } else {
            if (dew->hasref == 0) {  /* not a ref model */
                dewa->dewarpcache[i] = dew;  /* move it to the cache */
                dewa->dewarp[i] = NULL;  /* must null the ptr */
            }
        }
        if (distdown <= distup)  /* insert the hvalid ref model */
            dewarpaInsertDewarp(dewa, dewarpCreateRef(i, i - distdown));
        else
            dewarpaInsertDewarp(dewa, dewarpCreateRef(i, i + distup));
    }
    numaDestroy(&nah);

    dewa->modelsready = 1;  /* validated */
    return 0;
}


/*!
 *  dewarpaStripRefModels()
 *
 *      Input:  dewa (populated with dewarp structs for pages)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This examines each dew in a dewarpa, and removes
 *          all that don't have their own page model (i.e., all
 *          that have "references" to nearby pages with valid models).
 *          These references were generated by dewarpaInsertRefModels(dewa).
 */
l_int32
dewarpaStripRefModels(L_DEWARPA  *dewa)
{
l_int32    i;
L_DEWARP  *dew;

    PROCNAME("dewarpaStripRefModels");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    for (i = 0; i <= dewa->maxpage; i++) {
        if ((dew = dewarpaGetDewarp(dewa, i)) != NULL) {
            if (dew->hasref)
                dewarpDestroy(&dewa->dewarp[i]);
        }
    }
    dewa->modelsready = 0;

        /* Regenerate the page lists */
    dewarpaListPages(dewa);
    return 0;
}


/*!
 *  dewarpaRestoreModels()
 *
 *      Input:  dewa (populated with dewarp structs for pages)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This puts all real models (and only real models) in the
 *          primary dewarp array.  First remove all dewarps that are
 *          only references to other page models.  Then move all models
 *          that had been cached back into the primary dewarp array.
 *      (2) After this is done, we still need to recompute and insert
 *          the reference models before dewa->modelsready is true.
 */
l_int32
dewarpaRestoreModels(L_DEWARPA  *dewa)
{
l_int32    i;
L_DEWARP  *dew;

    PROCNAME("dewarpaRestoreModels");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

        /* Strip out ref models.  Then only real models will be in the
         * primary dewarp array. */
    dewarpaStripRefModels(dewa);

        /* The cache holds only real models, which are not necessarily valid. */
    for (i = 0; i <= dewa->maxpage; i++) {
        if ((dew = dewa->dewarpcache[i]) != NULL) {
            if (dewa->dewarp[i]) {
                L_ERROR("dew in both cache and main array!: page %d\n",
                        procName, i);
            } else {
                dewa->dewarp[i] = dew;
                dewa->dewarpcache[i] = NULL;
            }
        }
    }
    dewa->modelsready = 0;  /* new ref models not yet inserted */

        /* Regenerate the page lists */
    dewarpaListPages(dewa);
    return 0;
}


/*----------------------------------------------------------------------*
 *                      Dewarp debugging output                         *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpaInfo()
 *
 *      Input:  fp
 *              dewa
 *      Return: 0 if OK, 1 on error
 */
l_int32
dewarpaInfo(FILE       *fp,
            L_DEWARPA  *dewa)
{
l_int32    i, n, pageno, nnone, nvsuccess, nvvalid, nhsuccess, nhvalid, nref;
L_DEWARP  *dew;

    PROCNAME("dewarpaInfo");

    if (!fp)
        return ERROR_INT("dewa not defined", procName, 1);
    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    fprintf(fp, "\nDewarpaInfo: %p\n", dewa);
    fprintf(fp, "nalloc = %d, maxpage = %d\n", dewa->nalloc, dewa->maxpage);
    fprintf(fp, "sampling = %d, redfactor = %d, minlines = %d\n",
            dewa->sampling, dewa->redfactor, dewa->minlines);
    fprintf(fp, "maxdist = %d, useboth = %d\n",
            dewa->maxdist, dewa->useboth);

    dewarpaModelStats(dewa, &nnone, &nvsuccess, &nvvalid,
                      &nhsuccess, &nhvalid, &nref);
    n = numaGetCount(dewa->napages);
    fprintf(stderr, "Total number of pages with a dew = %d\n", n);
    fprintf(stderr, "Number of pages without any models = %d\n", nnone);
    fprintf(stderr, "Number of pages with a vert model = %d\n", nvsuccess);
    fprintf(stderr, "Number of pages with a valid vert model = %d\n", nvvalid);
    fprintf(stderr, "Number of pages with both models = %d\n", nhsuccess);
    fprintf(stderr, "Number of pages with both models valid = %d\n", nhvalid);
    fprintf(stderr, "Number of pages with a ref model = %d\n", nref);

    for (i = 0; i < n; i++) {
        numaGetIValue(dewa->napages, i, &pageno);
        if ((dew = dewarpaGetDewarp(dewa, pageno)) == NULL)
            continue;
        fprintf(stderr, "Page: %d\n", dew->pageno);
        fprintf(stderr, "  hasref = %d, refpage = %d\n",
                dew->hasref, dew->refpage);
        fprintf(stderr, "  nlines = %d\n", dew->nlines);
        fprintf(stderr, "  w = %d, h = %d, nx = %d, ny = %d\n",
                dew->w, dew->h, dew->nx, dew->ny);
    }
    return 0;
}


/*!
 *  dewarpaModelStats()
 *
 *      Input:  dewa
 *              &nnone (<optional return> number without any model)
 *              &nvsuccess (<optional return> number with a vert model)
 *              &nvvalid (<optional return> number with a valid vert model)
 *              &nhsuccess (<optional return> number with both models)
 *              &nhvalid (<optional return> number with both models valid)
 *              &nref (<optional return> number with a reference model)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) A page without a model has no dew.  It most likely failed to
 *          generate a vertical model, and has not been assigned a ref
 *          model from a neighboring page with a valid vertical model.
 *      (2) A page has vsuccess == 1 if there is at least a model of the
 *          vertical disparity.  The model may be invalid, in which case
 *          dewarpaInsertRefModels() will stash it in the cache and
 *          attempt to replace it by a valid ref model.
 *      (3) A vvvalid model is a vertical disparity model whose parameters
 *          satisfy the constraints given in dewarpaSetValidModels().
 *      (4) A page has hsuccess == 1 if both the vertical and horizontal
 *          disparity arrays have been constructed.
 *      (5) An  hvalid model has vertical and horizontal disparity
 *          models whose parameters satisfy the constraints given
 *          in dewarpaSetValidModels().
 *      (6) A page has a ref model if it failed to generate a valid
 *          model but was assigned a vvalid or hvalid model on another
 *          page (within maxdist) by dewarpaInsertRefModel().
 *      (7) This calls dewarpaTestForValidModel(); it ignores the vvalid
 *          and hvalid fields.
 */
l_int32
dewarpaModelStats(L_DEWARPA  *dewa,
                  l_int32    *pnnone,
                  l_int32    *pnvsuccess,
                  l_int32    *pnvvalid,
                  l_int32    *pnhsuccess,
                  l_int32    *pnhvalid,
                  l_int32    *pnref)
{
l_int32    i, n, pageno, nnone, nvsuccess, nvvalid, nhsuccess, nhvalid, nref;
L_DEWARP  *dew;

    PROCNAME("dewarpaModelStats");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);

    dewarpaListPages(dewa);
    n = numaGetCount(dewa->napages);
    nnone = nref = nvsuccess = nvvalid = nhsuccess = nhvalid = 0;
    for (i = 0; i < n; i++) {
        numaGetIValue(dewa->napages, i, &pageno);
        dew = dewarpaGetDewarp(dewa, pageno);
        if (!dew) {
            nnone++;
            continue;
        }
        if (dew->hasref == 1)
            nref++;
        if (dew->vsuccess == 1)
            nvsuccess++;
        if (dew->hsuccess == 1)
            nhsuccess++;
        dewarpaTestForValidModel(dewa, dew, 0);
        if (dew->vvalid == 1)
            nvvalid++;
        if (dew->hvalid == 1)
            nhvalid++;
    }

    if (pnnone) *pnnone = nnone;
    if (pnref) *pnref = nref;
    if (pnvsuccess) *pnvsuccess = nvsuccess;
    if (pnvvalid) *pnvvalid = nvvalid;
    if (pnhsuccess) *pnhsuccess = nhsuccess;
    if (pnhvalid) *pnhvalid = nhvalid;
    return 0;
}


/*!
 *  dewarpaTestForValidModel()
 *
 *      Input:  dewa
 *              dew
 *              notests
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Computes validity of vertical (vvalid) model and both
 *          vertical and horizontal (hvalid) models.
 *      (2) If @notests == 1, this ignores the curvature constraints
 *          and assumes that all successfully built models are valid.
 *      (3) This is just about the models, not the rendering process,
 *          so the value of useboth is not considered here.
 */
static l_int32
dewarpaTestForValidModel(L_DEWARPA  *dewa,
                         L_DEWARP   *dew,
                         l_int32     notests)
{
l_int32  maxcurv, diffcurv, diffedge;

    PROCNAME("dewarpaTestForValidModel");

    if (!dewa || !dew)
        return ERROR_INT("dewa and dew not both defined", procName, 1);

    if (notests) {
       dew->vvalid = dew->vsuccess;
       dew->hvalid = dew->hsuccess;
       return 0;
    }

        /* No actual model was built */
    if (dew->vsuccess == 0) return 0;

        /* Was previously found not to have a valid model  */
    if (dew->hasref == 1) return 0;

        /* vsuccess == 1; a vertical (line) model exists.
         * First test that the vertical curvatures are within allowed
         * bounds.  Note that all curvatures are signed.*/
    maxcurv = L_MAX(L_ABS(dew->mincurv), L_ABS(dew->maxcurv));
    diffcurv = dew->maxcurv - dew->mincurv;
    if (maxcurv <= dewa->max_linecurv &&
        diffcurv >= dewa->min_diff_linecurv &&
        diffcurv <= dewa->max_diff_linecurv) {
        dew->vvalid = 1;
    } else {
        L_INFO("invalid vert model for page %d\n", procName, dew->pageno);
    }

        /* If a horizontal (edge) model exists, test for validity. */
    if (dew->hsuccess) {
        diffedge = L_ABS(dew->leftcurv - dew->rightcurv);
        if (L_ABS(dew->leftslope) <= dewa->max_edgeslope &&
            L_ABS(dew->rightslope) <= dewa->max_edgeslope &&
            L_ABS(dew->leftcurv) <= dewa->max_edgecurv &&
            L_ABS(dew->rightcurv) <= dewa->max_edgecurv &&
            diffedge <= dewa->max_diff_edgecurv) {
            dew->hvalid = 1;
        } else {
            L_INFO("invalid horiz model for page %d\n", procName, dew->pageno);
        }
    }

    return 0;
}


/*!
 *  dewarpaShowArrays()
 *
 *      Input:  dewa
 *              scalefact (on contour images; typ. 0.5)
 *              first (first page model to render)
 *              last (last page model to render; use 0 to go to end)
 *              fontdir (for text bitmap fonts)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Generates a pdf of contour plots of the disparity arrays.
 *      (2) This only shows actual models; not ref models
 */
l_int32
dewarpaShowArrays(L_DEWARPA   *dewa,
                  l_float32    scalefact,
                  l_int32      first,
                  l_int32      last,
                  const char  *fontdir)
{
char       buf[256];
char      *pathname;
l_int32    i, svd, shd;
L_BMF     *bmf;
L_DEWARP  *dew;
PIX       *pixv, *pixvs, *pixh, *pixhs, *pixt, *pixd;
PIXA      *pixa;

    PROCNAME("dewarpaShowArrays");

    if (!dewa)
        return ERROR_INT("dew not defined", procName, 1);
    if (first < 0 || first > dewa->maxpage)
        return ERROR_INT("first out of bounds", procName, 1);
    if (last <= 0 || last > dewa->maxpage) last = dewa->maxpage;
    if (last < first)
        return ERROR_INT("last < first", procName, 1);

    lept_rmdir("lept");
    lept_mkdir("lept");
    if ((bmf = bmfCreate(fontdir, 8)) == NULL)
              L_ERROR("bmf not made; page info not displayed", procName);

    fprintf(stderr, "Generating contour plots\n");
    for (i = first; i <= last; i++) {
        if (i && ((i % 10) == 0))
            fprintf(stderr, " .. %d", i);
        dew = dewarpaGetDewarp(dewa, i);
        if (!dew) continue;
        if (dew->hasref == 1) continue;
        svd = shd = 0;
        if (dew->sampvdispar) svd = 1;
        if (dew->samphdispar) shd = 1;
        if (!svd) {
            L_ERROR("sampvdispar not made for page %d!\n", procName, i);
            continue;
        }

            /* Generate contour plots at reduced resolution */
        dewarpPopulateFullRes(dew, NULL, 0, 0);
        pixv = fpixRenderContours(dew->fullvdispar, 3.0, 0.15);
        pixvs = pixScaleBySampling(pixv, scalefact, scalefact);
        pixDestroy(&pixv);
        if (shd) {
            pixh = fpixRenderContours(dew->fullhdispar, 3.0, 0.15);
            pixhs = pixScaleBySampling(pixh, scalefact, scalefact);
            pixDestroy(&pixh);
        }
        dewarpMinimize(dew);

            /* Save side-by-side */
        pixa = pixaCreate(2);
        pixaAddPix(pixa, pixvs, L_INSERT);
        if (shd)
            pixaAddPix(pixa, pixhs, L_INSERT);
        pixt = pixaDisplayTiledInRows(pixa, 32, 1500, 1.0, 0, 30, 2);
        snprintf(buf, sizeof(buf), "Page %d", i);
        pixd = pixAddSingleTextblock(pixt, bmf, buf, 0x0000ff00,
                                     L_ADD_BELOW, NULL);
        snprintf(buf, sizeof(buf), "arrays_%04d.png", i);
        pathname = genPathname("/tmp/lept", buf);
        pixWrite(pathname, pixd, IFF_PNG);
        pixaDestroy(&pixa);
        pixDestroy(&pixt);
        pixDestroy(&pixd);
        FREE(pathname);
    }
    bmfDestroy(&bmf);
    fprintf(stderr, "\n");

    fprintf(stderr, "Generating pdf of contour plots\n");
    convertFilesToPdf("/tmp/lept", "arrays_", 90, 1.0, L_FLATE_ENCODE,
                      0, "Disparity arrays", "/tmp/lept/disparity_arrays.pdf");
    fprintf(stderr, "Output written to: /tmp/lept/disparity_arrays.pdf\n");
    return 0;
}


/*!
 *  dewarpDebug()
 *
 *      Input:  dew
 *              subdir (a subdirectory of /tmp; e.g., "dew1")
 *              index (to help label output images; e.g., the page number)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Prints dewarp fields and generates disparity array contour images.
 *          The contour images are written to file:
 *                /tmp/[subdir]/pixv_[index].png
 */
l_int32
dewarpDebug(L_DEWARP    *dew,
            const char  *subdir,
            l_int32      index)
{
char     outdir[256], fname[64];
char    *pathname;
l_int32  svd, shd;
PIX     *pixv, *pixh;

    PROCNAME("dewarpDebug");

    if (!dew)
        return ERROR_INT("dew not defined", procName, 1);
    if (!subdir)
        return ERROR_INT("subdir not defined", procName, 1);

    fprintf(stderr, "pageno = %d, hasref = %d, refpage = %d\n",
            dew->pageno, dew->hasref, dew->refpage);
    fprintf(stderr, "sampling = %d, redfactor = %d, minlines = %d\n",
            dew->sampling, dew->redfactor, dew->minlines);
    svd = shd = 0;
    if (!dew->hasref) {
        if (dew->sampvdispar) svd = 1;
        if (dew->samphdispar) shd = 1;
        fprintf(stderr, "sampv = %d, samph = %d\n", svd, shd);
        fprintf(stderr, "w = %d, h = %d\n", dew->w, dew->h);
        fprintf(stderr, "nx = %d, ny = %d\n", dew->nx, dew->ny);
        fprintf(stderr, "nlines = %d\n", dew->nlines);
        if (svd) {
            fprintf(stderr, "(min,max,abs-diff) line curvature = (%d,%d,%d)\n",
                    dew->mincurv, dew->maxcurv, dew->maxcurv - dew->mincurv);
        }
        if (shd) {
            fprintf(stderr, "(left,right,abs-diff) edge curvature = "
                    "(%d,%d,%d)\n", dew->leftcurv, dew->rightcurv,
                    L_ABS(dew->leftcurv - dew->rightcurv));
        }
    }
    if (!svd && !shd) {
        fprintf(stderr, "No disparity arrays\n");
        return 0;
    }

    dewarpPopulateFullRes(dew, NULL, 0, 0);
    lept_mkdir(subdir);
    snprintf(outdir, sizeof(outdir), "/tmp/%s", subdir);
    if (svd) {
        pixv = fpixRenderContours(dew->fullvdispar, 3.0, 0.15);
        snprintf(fname, sizeof(fname), "pixv_%d.png", index);
        pathname = genPathname(outdir, fname);
        pixWrite(pathname, pixv, IFF_PNG);
        pixDestroy(&pixv);
        FREE(pathname);
    }
    if (shd) {
        pixh = fpixRenderContours(dew->fullhdispar, 3.0, 0.15);
        snprintf(fname, sizeof(fname), "pixh_%d.png", index);
        pathname = genPathname(outdir, fname);
        pixWrite(pathname, pixh, IFF_PNG);
        pixDestroy(&pixh);
        FREE(pathname);
    }
    return 0;
}


/*!
 *  dewarpShowResults()
 *
 *      Input:  dewa
 *              sarray (of indexed input images)
 *              boxa (crop boxes for input images; can be null)
 *              firstpage, lastpage
 *              fontdir (for text bitmap fonts)
 *              pdfout (filename)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This generates a pdf of image pairs (before, after) for
 *          the designated set of input pages.
 *      (2) If the boxa exists, its elements are aligned with numbers
 *          in the filenames in @sa.  It is used to crop the input images.
 *          It is assumed that the dewa was generated from the cropped
 *          images.  No undercropping is applied before rendering.
 */
l_int32
dewarpShowResults(L_DEWARPA   *dewa,
                  SARRAY      *sa,
                  BOXA        *boxa,
                  l_int32      firstpage,
                  l_int32      lastpage,
                  const char  *fontdir,
                  const char  *pdfout)
{
char       bufstr[256];
char      *outpath;
l_int32    i, modelpage;
L_BMF     *bmf;
BOX       *box;
L_DEWARP  *dew;
PIX       *pixs, *pixc, *pixd, *pixt1, *pixt2;
PIXA      *pixa;

    PROCNAME("dewarpShowResults");

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);
    if (!sa)
        return ERROR_INT("sa not defined", procName, 1);
    if (!pdfout)
        return ERROR_INT("pdfout not defined", procName, 1);
    if (firstpage > lastpage)
        return ERROR_INT("invalid first/last page numbers", procName, 1);

    lept_rmdir("dewarp_pdfout");
    lept_mkdir("dewarp_pdfout");
    if ((bmf = bmfCreate(fontdir, 6)) == NULL)
        L_ERROR("bmf not made; page info not displayed", procName);

    fprintf(stderr, "Dewarping and generating s/by/s view\n");
    for (i = firstpage; i <= lastpage; i++) {
        if (i && (i % 10 == 0)) fprintf(stderr, ".. %d ", i);
        pixs = pixReadIndexed(sa, i);
        if (boxa) {
            box = boxaGetBox(boxa, i, L_CLONE);
            pixc = pixClipRectangle(pixs, box, NULL);
            boxDestroy(&box);
        }
        else
            pixc = pixClone(pixs);
        dew = dewarpaGetDewarp(dewa, i);
        pixd = NULL;
        if (dew) {
            dewarpaApplyDisparity(dewa, dew->pageno, pixc,
                                        GRAYIN_VALUE, 0, 0, &pixd, NULL);
            dewarpMinimize(dew);
        }
        pixa = pixaCreate(2);
        pixaAddPix(pixa, pixc, L_INSERT);
        if (pixd)
            pixaAddPix(pixa, pixd, L_INSERT);
        pixt1 = pixaDisplayTiledAndScaled(pixa, 32, 500, 2, 0, 35, 2);
        if (dew) {
            modelpage = (dew->hasref) ? dew->refpage : dew->pageno;
            snprintf(bufstr, sizeof(bufstr), "Page %d; using %d\n",
                     i, modelpage);
        }
        else
            snprintf(bufstr, sizeof(bufstr), "Page %d; no dewarp\n", i);
        pixt2 = pixAddSingleTextblock(pixt1, bmf, bufstr, 0x0000ff00,
                                      L_ADD_BELOW, 0);
        snprintf(bufstr, sizeof(bufstr), "/tmp/dewarp_pdfout/%05d", i);
        pixWrite(bufstr, pixt2, IFF_JFIF_JPEG);
        pixaDestroy(&pixa);
        pixDestroy(&pixs);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "Generating pdf of result\n");
    convertFilesToPdf("/tmp/dewarp_pdfout", NULL, 100, 1.0, L_JPEG_ENCODE,
                      0, "Dewarp sequence", pdfout);
    outpath = genPathname(pdfout, NULL);
    fprintf(stderr, "Output written to: %s\n", outpath);
    FREE(outpath);
    bmfDestroy(&bmf);
    return 0;
}

