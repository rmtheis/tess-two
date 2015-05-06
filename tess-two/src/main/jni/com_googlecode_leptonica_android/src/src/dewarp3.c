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
 *  dewarp3.c
 *
 *    Applying and stripping the page disparity model
 *
 *      Apply disparity array
 *          l_int32            dewarpaApplyDisparity()
 *          static l_int32     pixApplyVertDisparity()
 *          static l_int32     pixApplyHorizDisparity()
 *
 *      Stripping out data and populating full res disparity
 *          l_int32            dewarpMinimize()
 *          l_int32            dewarpPopulateFullRes()
 *
 *      Static functions not presently in use
 *          static FPIX       *fpixSampledDisparity()
 *          static FPIX       *fpixExtraHorizDisparity()
 *
 */

#include <math.h>
#include "allheaders.h"

static PIX *pixApplyVertDisparity(L_DEWARP *dew, PIX *pixs, l_int32 grayin);
static PIX * pixApplyHorizDisparity(L_DEWARP *dew, PIX *pixs, l_int32 grayin);


/*----------------------------------------------------------------------*
 *                     Apply warping disparity array                    *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpaApplyDisparity()
 *
 *      Input:  dewa
 *              pageno (of page model to be used; may be a ref model)
 *              pixs (image to be modified; can be 1, 8 or 32 bpp)
 *              grayin (gray value, from 0 to 255, for pixels brought in;
 *                      use -1 to use pixels on the boundary of pixs)
 *              x, y (origin for generation of disparity arrays)
 *              &pixd (<return> disparity corrected image)
 *              debugfile (use null to skip writing this)
 *      Return: 0 if OK, 1 on error (no models or ref models available)
 *
 *  Notes:
 *      (1) This applies the disparity arrays to the specified image.
 *      (2) Specify gray color for pixels brought in from the outside:
 *          0 is black, 255 is white.  Use -1 to select pixels from the
 *          boundary of the source image.
 *      (3) If the models and ref models have not been validated, this
 *          will do so by calling dewarpaInsertRefModels().
 *      (4) This works with both stripped and full resolution page models.
 *          If the full res disparity array(s) are missing, they are remade.
 *      (5) The caller must handle errors that are returned because there
 *          are no valid models or ref models for the page -- typically
 *          by using the input pixs.
 *      (6) If there is no model for @pageno, this will use the model for
 *          'refpage' and put the result in the dew for @pageno.
 *      (7) This populates the full resolution disparity arrays if
 *          necessary.  If x and/or y are positive, they are used,
 *          in conjunction with pixs, to determine the required
 *          slope-based extension of the full resolution disparity
 *          arrays in each direction.  When (x,y) == (0,0), all
 *          extension is to the right and down.  Nonzero values of (x,y)
 *          are useful for dewarping when pixs is deliberately undercropped.
 *      (8) Important: when applying disparity to a number of images,
 *          after calling this function and saving the resulting pixd,
 *          you should call dewarpMinimize(dew) on the dew for @pageno.
 *          This will remove pixs and pixd (or their clones) stored in dew,
 *          as well as the full resolution disparity arrays.  Together,
 *          these hold approximately 16 bytes for each pixel in pixs.
 */
l_int32
dewarpaApplyDisparity(L_DEWARPA   *dewa,
                      l_int32      pageno,
                      PIX         *pixs,
                      l_int32      grayin,
                      l_int32      x,
                      l_int32      y,
                      PIX        **ppixd,
                      const char  *debugfile)
{
l_int32    debug;
L_DEWARP  *dew1, *dew2;
PIX       *pixv, *pixh;

    PROCNAME("dewarpaApplyDisparity");

        /* Initialize the output with the input, so we'll have that
         * in case we can't apply the page model. */
    if (!ppixd)
        return ERROR_INT("&pixd not defined", procName, 1);
    *ppixd = pixClone(pixs);

    if (!dewa)
        return ERROR_INT("dewa not defined", procName, 1);
    if (pageno < 0 || pageno > dewa->maxpage)
        return ERROR_INT("invalid pageno", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (grayin > 255)
        return ERROR_INT("invalid grayin", procName, 1);
    if (x < 0) x = 0;
    if (y < 0) y = 0;

        /* Make sure all models are valid and all refmodels have
         * been added to dewa */
    debug = (debugfile) ? 1 : 0;
    if (dewa->modelsready == FALSE)
        dewarpaInsertRefModels(dewa, 0, debug);

        /* Check for the existence of a valid model; we don't expect
         * all pages to have them. */
    if ((dew1 = dewarpaGetDewarp(dewa, pageno)) == NULL) {
        L_INFO("dew1 not defined; no valid model for page %d\n",
                   procName, pageno);
        return 1;
    }

        /* Get the page model that we will use and sanity-check that
         * it is valid.  The ultimate result will be put in dew1->pixd. */
    if (dew1->hasref)  /* point to another page with a model */
        dew2 = dewarpaGetDewarp(dewa, dew1->refpage);
    else
        dew2 = dew1;
    if (dew2->vvalid == FALSE)
        return ERROR_INT("no model; shouldn't happen", procName, 1);

        /* Generate the full res disparity arrays if they don't exist
         * (e.g., if they've been minimized or read from file), or if
         * they are too small for the current image.  */
    dewarpPopulateFullRes(dew2, pixs, x, y);

        /* Correct for vertical disparity and save the result */
    if ((pixv = pixApplyVertDisparity(dew2, pixs, grayin)) == NULL) {
        dewarpMinimize(dew2);
        return ERROR_INT("pixv not made", procName, 1);
    }
    pixDestroy(ppixd);
    *ppixd = pixv;
    if (debugfile) {
        pixDisplayWithTitle(pixv, 300, 0, "pixv", 1);
        lept_rmdir("dewapply");  /* remove previous images */
        lept_mkdir("dewapply");
        pixWrite("/tmp/dewapply/001.png", pixs, IFF_PNG);
        pixWrite("/tmp/dewapply/002.png", pixv, IFF_PNG);
    }

        /* Optionally, correct for horizontal disparity */
    if (dewa->useboth && dew2->hsuccess) {
        if (dew2->hvalid == FALSE) {
            L_INFO("invalid horiz model for page %d\n", procName, pageno);
        } else {
            if ((pixh = pixApplyHorizDisparity(dew2, pixv, grayin)) != NULL) {
                pixDestroy(ppixd);
                *ppixd = pixh;
                if (debugfile) {
                    pixDisplayWithTitle(pixh, 600, 0, "pixh", 1);
                    pixWrite("/tmp/dewapply/003.png", pixh, IFF_PNG);
                }
            } else {
                L_ERROR("horiz disparity failed on page %d\n",
                        procName, pageno);
            }
        }
    }

    if (debugfile) {
        dewarpDebug(dew1, "dewapply", 0);
        convertFilesToPdf("/tmp/dewapply", NULL, 135, 1.0, 0, 0,
                         "Dewarp Apply Disparity", debugfile);
        fprintf(stderr, "pdf file made: %s\n", debugfile);
    }

        /* Get rid of the large full res disparity arrays */
    dewarpMinimize(dew2);

    return 0;
}


/*!
 *  pixApplyVertDisparity()
 *
 *      Input:  dew
 *              pixs (1, 8 or 32 bpp)
 *              grayin (gray value, from 0 to 255, for pixels brought in;
 *                      use -1 to use pixels on the boundary of pixs)
 *      Return: pixd (modified to remove vertical disparity), or null on error
 *
 *  Notes:
 *      (1) This applies the vertical disparity array to the specified
 *          image.  For src pixels above the image, we use the pixels
 *          in the first raster line.
 *      (2) Specify gray color for pixels brought in from the outside:
 *          0 is black, 255 is white.  Use -1 to select pixels from the
 *          boundary of the source image.
 */
static PIX *
pixApplyVertDisparity(L_DEWARP  *dew,
                      PIX       *pixs,
                      l_int32    grayin)
{
l_int32     i, j, w, h, d, fw, fh, wpld, wplf, isrc, val8;
l_uint32   *datad, *lined;
l_float32  *dataf, *linef;
void      **lineptrs;
FPIX       *fpix;
PIX        *pixd;

    PROCNAME("pixApplyVertDisparity");

    if (!dew)
        return (PIX *)ERROR_PTR("dew not defined", procName, NULL);
    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pix not 1, 8 or 32 bpp", procName, NULL);
    if ((fpix = dew->fullvdispar) == NULL)
        return (PIX *)ERROR_PTR("fullvdispar not defined", procName, NULL);
    fpixGetDimensions(fpix, &fw, &fh);
    if (fw < w || fh < h) {
        fprintf(stderr, "fw = %d, w = %d, fh = %d, h = %d\n", fw, w, fh, h);
        return (PIX *)ERROR_PTR("invalid fpix size", procName, NULL);
    }

        /* Two choices for requested pixels outside pixs: (1) use pixels'
         * from the boundary of pixs; use white or light gray pixels. */
    pixd = pixCreateTemplate(pixs);
    if (grayin >= 0)
        pixSetAllGray(pixd, grayin);
    datad = pixGetData(pixd);
    dataf = fpixGetData(fpix);
    wpld = pixGetWpl(pixd);
    wplf = fpixGetWpl(fpix);
    if (d == 1) {
        lineptrs = pixGetLinePtrs(pixs, NULL);
        for (i = 0; i < h; i++) {
            lined = datad + i * wpld;
            linef = dataf + i * wplf;
            for (j = 0; j < w; j++) {
                isrc = (l_int32)(i - linef[j] + 0.5);
                if (grayin < 0)  /* use value at boundary if outside */
                    isrc = L_MIN(L_MAX(isrc, 0), h - 1);
                if (isrc >= 0 && isrc < h) {  /* remains gray if outside */
                    if (GET_DATA_BIT(lineptrs[isrc], j))
                        SET_DATA_BIT(lined, j);
                }
            }
        }
    } else if (d == 8) {
        lineptrs = pixGetLinePtrs(pixs, NULL);
        for (i = 0; i < h; i++) {
            lined = datad + i * wpld;
            linef = dataf + i * wplf;
            for (j = 0; j < w; j++) {
                isrc = (l_int32)(i - linef[j] + 0.5);
                if (grayin < 0)
                    isrc = L_MIN(L_MAX(isrc, 0), h - 1);
                if (isrc >= 0 && isrc < h) {
                    val8 = GET_DATA_BYTE(lineptrs[isrc], j);
                    SET_DATA_BYTE(lined, j, val8);
                }
            }
        }
    } else {  /* d == 32 */
        lineptrs = pixGetLinePtrs(pixs, NULL);
        for (i = 0; i < h; i++) {
            lined = datad + i * wpld;
            linef = dataf + i * wplf;
            for (j = 0; j < w; j++) {
                isrc = (l_int32)(i - linef[j] + 0.5);
                if (grayin < 0)
                    isrc = L_MIN(L_MAX(isrc, 0), h - 1);
                if (isrc >= 0 && isrc < h)
                    lined[j] = GET_DATA_FOUR_BYTES(lineptrs[isrc], j);
            }
        }
    }

    FREE(lineptrs);
    return pixd;
}


/*!
 *  pixApplyHorizDisparity()
 *
 *      Input:  dew
 *              pixs (1, 8 or 32 bpp)
 *              grayin (gray value, from 0 to 255, for pixels brought in;
 *                      use -1 to use pixels on the boundary of pixs)
 *      Return: pixd (modified to remove horizontal disparity if possible),
 *              or null on error.
 *
 *  Notes:
 *      (1) This applies the horizontal disparity array to the specified
 *          image.
 *      (2) Specify gray color for pixels brought in from the outside:
 *          0 is black, 255 is white.  Use -1 to select pixels from the
 *          boundary of the source image.
 *      (3) The input pixs has already been corrected for vertical disparity.
 *          If the horizontal disparity array doesn't exist, this returns
 *          a clone of @pixs.
 */
static PIX *
pixApplyHorizDisparity(L_DEWARP  *dew,
                       PIX       *pixs,
                       l_int32    grayin)
{
l_int32     i, j, w, h, d, fw, fh, wpls, wpld, wplf, jsrc, val8;
l_uint32   *datas, *lines, *datad, *lined;
l_float32  *dataf, *linef;
FPIX       *fpix;
PIX        *pixd;

    PROCNAME("pixApplyHorizDisparity");

    if (!dew)
        return (PIX *)ERROR_PTR("dew not defined", procName, pixs);
    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pix not 1, 8 or 32 bpp", procName, NULL);
    if ((fpix = dew->fullhdispar) == NULL)
        return (PIX *)ERROR_PTR("fullhdispar not defined", procName, NULL);
    fpixGetDimensions(fpix, &fw, &fh);
    if (fw < w || fh < h) {
        fprintf(stderr, "fw = %d, w = %d, fh = %d, h = %d\n", fw, w, fh, h);
        return (PIX *)ERROR_PTR("invalid fpix size", procName, NULL);
    }

        /* Two choices for requested pixels outside pixs: (1) use pixels'
         * from the boundary of pixs; use white or light gray pixels. */
    pixd = pixCreateTemplate(pixs);
    if (grayin >= 0)
        pixSetAllGray(pixd, grayin);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    dataf = fpixGetData(fpix);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    wplf = fpixGetWpl(fpix);
    if (d == 1) {
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            linef = dataf + i * wplf;
            for (j = 0; j < w; j++) {
                jsrc = (l_int32)(j - linef[j] + 0.5);
                if (grayin < 0)  /* use value at boundary if outside */
                    jsrc = L_MIN(L_MAX(jsrc, 0), w - 1);
                if (jsrc >= 0 && jsrc < w) {  /* remains gray if outside */
                    if (GET_DATA_BIT(lines, jsrc))
                        SET_DATA_BIT(lined, j);
                }
            }
        }
    } else if (d == 8) {
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            linef = dataf + i * wplf;
            for (j = 0; j < w; j++) {
                jsrc = (l_int32)(j - linef[j] + 0.5);
                if (grayin < 0)
                    jsrc = L_MIN(L_MAX(jsrc, 0), w - 1);
                if (jsrc >= 0 && jsrc < w) {
                    val8 = GET_DATA_BYTE(lines, jsrc);
                    SET_DATA_BYTE(lined, j, val8);
                }
            }
        }
    } else {  /* d == 32 */
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            linef = dataf + i * wplf;
            for (j = 0; j < w; j++) {
                jsrc = (l_int32)(j - linef[j] + 0.5);
                if (grayin < 0)
                    jsrc = L_MIN(L_MAX(jsrc, 0), w - 1);
                if (jsrc >= 0 && jsrc < w)
                    lined[j] = lines[jsrc];
            }
        }
    }

    return pixd;
}


/*----------------------------------------------------------------------*
 *          Stripping out data and populating full res disparity        *
 *----------------------------------------------------------------------*/
/*!
 *  dewarpMinimize()
 *
 *      Input:  dew
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This removes all data that is not needed for serialization.
 *          It keeps the subsampled disparity array(s), so the full
 *          resolution arrays can be reconstructed.
 */
l_int32
dewarpMinimize(L_DEWARP  *dew)
{
L_DEWARP  *dewt;

    PROCNAME("dewarpMinimize");

    if (!dew)
        return ERROR_INT("dew not defined", procName, 1);

        /* If dew is a ref, minimize the actual dewarp */
    if (dew->hasref)
        dewt = dewarpaGetDewarp(dew->dewa, dew->refpage);
    else
        dewt = dew;
    if (!dewt)
        return ERROR_INT("dewt not found", procName, 1);

    pixDestroy(&dewt->pixs);
    fpixDestroy(&dewt->fullvdispar);
    fpixDestroy(&dewt->fullhdispar);
    numaDestroy(&dewt->namidys);
    numaDestroy(&dewt->nacurves);
    return 0;
}


/*!
 *  dewarpPopulateFullRes()
 *
 *      Input:  dew
 *              pix (<optional>, to give size of actual image)
 *              x, y (origin for generation of disparity arrays)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If the full resolution vertical and horizontal disparity
 *          arrays do not exist, they are built from the subsampled ones.
 *      (2) If pixs is not given, the size of the arrays is determined
 *          by the original image from which the sampled version was
 *          generated.  Any values of (x,y) are ignored.
 *      (3) If pixs is given, the full resolution disparity arrays must
 *          be large enough to accommodate it.
 *          (a) If the arrays do not exist, the value of (x,y) determines
 *              the origin of the full resolution arrays without extension,
 *              relative to pixs.  Thus, (x,y) gives the amount of
 *              slope extension in (left, top).  The (right, bottom)
 *              extension is then determined by the size of pixs and
 *              (x,y); the values should never be < 0.
 *          (b) If the arrays exist and pixs is too large, the existing
 *              full res arrays are destroyed and new ones are made,
 *              again using (x,y) to determine the extension in the
 *              four directions.
 */
l_int32
dewarpPopulateFullRes(L_DEWARP  *dew,
                      PIX       *pix,
                      l_int32    x,
                      l_int32    y)
{
l_int32     width, height, fw, fh, deltaw, deltah, redfactor;
FPIX       *fpixt1, *fpixt2;

    PROCNAME("dewarpPopulateFullRes");

    if (!dew)
        return ERROR_INT("dew not defined", procName, 1);
    if (!dew->sampvdispar)
        return ERROR_INT("no sampled vert disparity", procName, 1);
    if (x < 0) x = 0;
    if (y < 0) y = 0;

        /* Establish the target size for the full res arrays */
    if (pix)
        pixGetDimensions(pix, &width, &height, NULL);
    else {
        width = dew->w;
        height = dew->h;
    }

        /* Destroy the existing arrays if they are too small */
    if (dew->fullvdispar) {
        fpixGetDimensions(dew->fullvdispar, &fw, &fh);
        if (width > fw || height > fw)
            fpixDestroy(&dew->fullvdispar);
    }
    if (dew->fullhdispar) {
        fpixGetDimensions(dew->fullhdispar, &fw, &fh);
        if (width > fw || height > fw)
            fpixDestroy(&dew->fullhdispar);
    }

        /* Find the required width and height expansion deltas */
    deltaw = width - dew->sampling * (dew->nx - 1) + 2;
    deltah = height - dew->sampling * (dew->ny - 1) + 2;
    redfactor = dew->redfactor;
    deltaw = redfactor * L_MAX(0, deltaw);
    deltah = redfactor * L_MAX(0, deltah);

        /* Generate the full res vertical array if it doesn't exist,
         * extending it as required to make it big enough.  Use x,y
         * to determine the amounts on each side. */
    if (!dew->fullvdispar) {
        fpixt1 = fpixCopy(NULL, dew->sampvdispar);
        if (redfactor == 2)
            fpixAddMultConstant(fpixt1, 0.0, (l_float32)redfactor);
        fpixt2 = fpixScaleByInteger(fpixt1, dew->sampling * redfactor);
        fpixDestroy(&fpixt1);
        if (deltah == 0 && deltaw == 0) {
            dew->fullvdispar = fpixt2;
        }
        else {
            dew->fullvdispar = fpixAddSlopeBorder(fpixt2, x, deltaw - x,
                                                  y, deltah - y);
            fpixDestroy(&fpixt2);
        }
    }

        /* Similarly, generate the full res horizontal array if it
         * doesn't exist.  Do this even if useboth == 0. */
    if (!dew->fullhdispar && dew->samphdispar) {
        fpixt1 = fpixCopy(NULL, dew->samphdispar);
        if (redfactor == 2)
            fpixAddMultConstant(fpixt1, 0.0, (l_float32)redfactor);
        fpixt2 = fpixScaleByInteger(fpixt1, dew->sampling * redfactor);
        fpixDestroy(&fpixt1);
        if (deltah == 0 && deltaw == 0) {
            dew->fullhdispar = fpixt2;
        }
        else {
            dew->fullhdispar = fpixAddSlopeBorder(fpixt2, x, deltaw - x,
                                                  y, deltah - y);
            fpixDestroy(&fpixt2);
        }
    }

    return 0;
}


#if 0
/*----------------------------------------------------------------------*
 *                Static functions not presently in use                 *
 *----------------------------------------------------------------------*/
/*!
 *  fpixSampledDisparity()
 *
 *      Input:  fpixs (full resolution disparity model)
 *              sampling (sampling factor)
 *      Return: fpixd (sampled disparity model), or null on error
 *
 *  Notes:
 *      (1) This converts full to sampled disparity.
 *      (2) The input array is sampled at the right and top edges, and
 *          at every @sampling pixels horizontally and vertically.
 *      (3) The sampled array may not extend to the right and bottom
 *          pixels in fpixs.  This will occur if fpixs was generated
 *          with slope extension because the image on that page was
 *          larger than normal.  This is fine, because in use the
 *          sampled array will be interpolated back to full resolution
 *          and then extended as required.  So the operations of
 *          sampling and interpolation will be idempotent.
 *      (4) There must be at least 3 sampled points horizontally and
 *          vertically.
 */
static FPIX *
fpixSampledDisparity(FPIX    *fpixs,
                     l_int32  sampling)
{
l_int32    w, h, wd, hd, i, j, is, js;
l_float32  val;
FPIX      *fpixd;

    PROCNAME("fpixSampledDisparity");

    if (!fpixs)
        return (FPIX *)ERROR_PTR("fpixs not defined", procName, NULL);
    if (sampling < 1)
        return (FPIX *)ERROR_PTR("sampling < 1", procName, NULL);

    fpixGetDimensions(fpixs, &w, &h);
    wd = 1 + (w + sampling - 2) / sampling;
    hd = 1 + (h + sampling - 2) / sampling;
    if (wd < 3 || hd < 3)
        return (FPIX *)ERROR_PTR("wd < 3 or hd < 3", procName, NULL);
    fpixd = fpixCreate(wd, hd);
    for (i = 0; i < hd; i++) {
        is = sampling * i;
        if (is >= h) continue;
        for (j = 0; j < wd; j++) {
            js = sampling * j;
            if (js >= w) continue;
            fpixGetPixel(fpixs, js, is, &val);
            fpixSetPixel(fpixd, j, i, val);
        }
    }

    return fpixd;
}


/*!
 *  fpixExtraHorizDisparity()
 *
 *      Input:  fpixv (vertical disparity model)
 *              factor (conversion factor for vertical disparity slope;
 *                      use 0 for default)
 *              &xwid (<return> extra width to be added to dewarped pix)
 *      Return: fpixh, or null on error
 *
 *  Notes:
 *      (1) This takes the difference in vertical disparity at top
 *          and bottom of the image, and converts it to an assumed
 *          horizontal disparity.  In use, we add this to the
 *          horizontal disparity determined by the left and right
 *          ends of textlines.
 *      (2) Usage:
 *            l_int32 xwid = [extra width to be added to fpix and image]
 *            FPix *fpix = fpixExtraHorizDisparity(dew->fullvdispar, 0, &xwid);
 *            fpixLinearCombination(dew->fullhdispar, dew->fullhdispar,
 *                                  fpix, 1.0, 1.0);
 */
static FPIX *
fpixExtraHorizDisparity(FPIX      *fpixv,
                        l_float32  factor,
                        l_int32   *pxwid)
{
l_int32     w, h, i, j, fw, wpl, maxloc;
l_float32   val1, val2, vdisp, vdisp0, maxval;
l_float32  *data, *line, *fadiff;
NUMA       *nadiff;
FPIX       *fpixh;

    PROCNAME("fpixExtraHorizDisparity");

    if (!fpixv)
        return (FPIX *)ERROR_PTR("fpixv not defined", procName, NULL);
    if (!pxwid)
        return (FPIX *)ERROR_PTR("&xwid not defined", procName, NULL);
    if (factor == 0.0)
        factor = DEFAULT_SLOPE_FACTOR;

        /* Estimate horizontal disparity from the vertical disparity
         * difference between the top and bottom, normalized to the
         * image height.  Add the maximum value to the width of the
         * output image, so that all src pixels can be mapped
         * into the dest. */
    fpixGetDimensions(fpixv, &w, &h);
    nadiff = numaCreate(w);
    for (j = 0; j < w; j++) {
        fpixGetPixel(fpixv, j, 0, &val1);
        fpixGetPixel(fpixv, j, h - 1, &val2);
        vdisp = factor * (val2 - val1) / (l_float32)h;
        if (j == 0) vdisp0 = vdisp;
        vdisp = vdisp0 - vdisp;
        numaAddNumber(nadiff, vdisp);
    }
    numaGetMax(nadiff, &maxval, &maxloc);
    *pxwid = (l_int32)(maxval + 0.5);

    fw = w + *pxwid;
    fpixh = fpixCreate(fw, h);
    data = fpixGetData(fpixh);
    wpl = fpixGetWpl(fpixh);
    fadiff = numaGetFArray(nadiff, L_NOCOPY);
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < fw; j++) {
            if (j < maxloc)   /* this may not work for even pages */
                line[j] = fadiff[j];
            else  /* keep it at the max value the rest of the way across */
                line[j] = maxval;
        }
    }

    numaDestroy(&nadiff);
    return fpixh;
}
#endif
