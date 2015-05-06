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
 *  morphapp.c
 *
 *      These are some useful and/or interesting composite
 *      image processing operations, of the type that are often
 *      useful in applications.  Most are morphological in
 *      nature.
 *
 *      Extraction of boundary pixels
 *            PIX       *pixExtractBoundary()
 *
 *      Selective morph sequence operation under mask
 *            PIX       *pixMorphSequenceMasked()
 *
 *      Selective morph sequence operation on each component
 *            PIX       *pixMorphSequenceByComponent()
 *            PIXA      *pixaMorphSequenceByComponent()
 *
 *      Selective morph sequence operation on each region
 *            PIX       *pixMorphSequenceByRegion()
 *            PIXA      *pixaMorphSequenceByRegion()
 *
 *      Union and intersection of parallel composite operations
 *            PIX       *pixUnionOfMorphOps()
 *            PIX       *pixIntersectionOfMorphOps()
 *
 *      Selective connected component filling
 *            PIX       *pixSelectiveConnCompFill()
 *
 *      Removal of matched patterns
 *            PIX       *pixRemoveMatchedPattern()
 *
 *      Display of matched patterns
 *            PIX       *pixDisplayMatchedPattern()
 *
 *      Iterative morphological seed filling (don't use for real work)
 *            PIX       *pixSeedfillMorph()
 *
 *      Granulometry on binary images
 *            NUMA      *pixRunHistogramMorph()
 *
 *      Composite operations on grayscale images
 *            PIX       *pixTophat()
 *            PIX       *pixHDome()
 *            PIX       *pixFastTophat()
 *            PIX       *pixMorphGradient()
 *
 *      Centroid of component
 *            PTA       *pixaCentroids()
 *            l_int32    pixCentroid()
 */

#include "allheaders.h"

#define   SWAP(x, y)   {temp = (x); (x) = (y); (y) = temp;}


/*-----------------------------------------------------------------*
 *                   Extraction of boundary pixels                 *
 *-----------------------------------------------------------------*/
/*!
 *  pixExtractBoundary()
 *
 *      Input:  pixs (1 bpp)
 *              type (0 for background pixels; 1 for foreground pixels)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Extracts the fg or bg boundary pixels for each component.
 *          Components are assumed to end at the boundary of pixs.
 */
PIX *
pixExtractBoundary(PIX     *pixs,
                   l_int32  type)
{
PIX  *pixd;

    PROCNAME("pixExtractBoundary");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

    if (type == 0)
        pixd = pixDilateBrick(NULL, pixs, 3, 3);
    else
        pixd = pixErodeBrick(NULL, pixs, 3, 3);
    pixXor(pixd, pixd, pixs);
    return pixd;
}


/*-----------------------------------------------------------------*
 *           Selective morph sequence operation under mask         *
 *-----------------------------------------------------------------*/
/*!
 *  pixMorphSequenceMasked()
 *
 *      Input:  pixs (1 bpp)
 *              pixm (<optional> 1 bpp mask)
 *              sequence (string specifying sequence of operations)
 *              dispsep (horizontal separation in pixels between
 *                       successive displays; use zero to suppress display)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This applies the morph sequence to the image, but only allows
 *          changes in pixs for pixels under the background of pixm.
 *      (5) If pixm is NULL, this is just pixMorphSequence().
 */
PIX *
pixMorphSequenceMasked(PIX         *pixs,
                       PIX         *pixm,
                       const char  *sequence,
                       l_int32      dispsep)
{
PIX  *pixd;

    PROCNAME("pixMorphSequenceMasked");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!sequence)
        return (PIX *)ERROR_PTR("sequence not defined", procName, NULL);

    pixd = pixMorphSequence(pixs, sequence, dispsep);
    pixCombineMasked(pixd, pixs, pixm);  /* restore src pixels under mask fg */
    return pixd;
}


/*-----------------------------------------------------------------*
 *             Morph sequence operation on each component          *
 *-----------------------------------------------------------------*/
/*!
 *  pixMorphSequenceByComponent()
 *
 *      Input:  pixs (1 bpp)
 *              sequence (string specifying sequence)
 *              connectivity (4 or 8)
 *              minw  (minimum width to consider; use 0 or 1 for any width)
 *              minh  (minimum height to consider; use 0 or 1 for any height)
 *              &boxa (<optional> return boxa of c.c. in pixs)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) See pixMorphSequence() for composing operation sequences.
 *      (2) This operates separately on each c.c. in the input pix.
 *      (3) The dilation does NOT increase the c.c. size; it is clipped
 *          to the size of the original c.c.   This is necessary to
 *          keep the c.c. independent after the operation.
 *      (4) You can specify that the width and/or height must equal
 *          or exceed a minimum size for the operation to take place.
 *      (5) Use NULL for boxa to avoid returning the boxa.
 */
PIX *
pixMorphSequenceByComponent(PIX         *pixs,
                            const char  *sequence,
                            l_int32      connectivity,
                            l_int32      minw,
                            l_int32      minh,
                            BOXA       **pboxa)
{
l_int32  n, i, x, y, w, h;
BOXA    *boxa;
PIX     *pix, *pixd;
PIXA    *pixas, *pixad;

    PROCNAME("pixMorphSequenceByComponent");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!sequence)
        return (PIX *)ERROR_PTR("sequence not defined", procName, NULL);

    if (minw <= 0) minw = 1;
    if (minh <= 0) minh = 1;

        /* Get the c.c. */
    if ((boxa = pixConnComp(pixs, &pixas, connectivity)) == NULL)
        return (PIX *)ERROR_PTR("boxa not made", procName, NULL);

        /* Operate on each c.c. independently */
    pixad = pixaMorphSequenceByComponent(pixas, sequence, minw, minh);
    pixaDestroy(&pixas);
    boxaDestroy(&boxa);
    if (!pixad)
        return (PIX *)ERROR_PTR("pixad not made", procName, NULL);

        /* Display the result out into pixd */
    pixd = pixCreateTemplate(pixs);
    n = pixaGetCount(pixad);
    for (i = 0; i < n; i++) {
        pixaGetBoxGeometry(pixad, i, &x, &y, &w, &h);
        pix = pixaGetPix(pixad, i, L_CLONE);
        pixRasterop(pixd, x, y, w, h, PIX_PAINT, pix, 0, 0);
        pixDestroy(&pix);
    }

    if (pboxa)
        *pboxa = pixaGetBoxa(pixad, L_CLONE);
    pixaDestroy(&pixad);
    return pixd;
}


/*!
 *  pixaMorphSequenceByComponent()
 *
 *      Input:  pixas (of 1 bpp pix)
 *              sequence (string specifying sequence)
 *              minw  (minimum width to consider; use 0 or 1 for any width)
 *              minh  (minimum height to consider; use 0 or 1 for any height)
 *      Return: pixad, or null on error
 *
 *  Notes:
 *      (1) See pixMorphSequence() for composing operation sequences.
 *      (2) This operates separately on each c.c. in the input pixa.
 *      (3) You can specify that the width and/or height must equal
 *          or exceed a minimum size for the operation to take place.
 *      (4) The input pixa should have a boxa giving the locations
 *          of the pix components.
 */
PIXA *
pixaMorphSequenceByComponent(PIXA        *pixas,
                             const char  *sequence,
                             l_int32      minw,
                             l_int32      minh)
{
l_int32  n, i, w, h, d;
BOX     *box;
PIX     *pix1, *pix2;
PIXA    *pixad;

    PROCNAME("pixaMorphSequenceByComponent");

    if (!pixas)
        return (PIXA *)ERROR_PTR("pixas not defined", procName, NULL);
    if ((n = pixaGetCount(pixas)) == 0)
        return (PIXA *)ERROR_PTR("no pix in pixas", procName, NULL);
    if (n != pixaGetBoxaCount(pixas))
        L_WARNING("boxa size != n\n", procName);
    pixaGetPixDimensions(pixas, 0, NULL, NULL, &d);
    if (d != 1)
        return (PIXA *)ERROR_PTR("depth not 1 bpp", procName, NULL);

    if (!sequence)
        return (PIXA *)ERROR_PTR("sequence not defined", procName, NULL);
    if (minw <= 0) minw = 1;
    if (minh <= 0) minh = 1;

    if ((pixad = pixaCreate(n)) == NULL)
        return (PIXA *)ERROR_PTR("pixad not made", procName, NULL);
    for (i = 0; i < n; i++) {
        pixaGetPixDimensions(pixas, i, &w, &h, NULL);
        if (w >= minw && h >= minh) {
            if ((pix1 = pixaGetPix(pixas, i, L_CLONE)) == NULL)
                return (PIXA *)ERROR_PTR("pix1 not found", procName, NULL);
            if ((pix2 = pixMorphCompSequence(pix1, sequence, 0)) == NULL)
                return (PIXA *)ERROR_PTR("pix2 not made", procName, NULL);
            pixaAddPix(pixad, pix2, L_INSERT);
            box = pixaGetBox(pixas, i, L_COPY);
            pixaAddBox(pixad, box, L_INSERT);
            pixDestroy(&pix1);
        }
    }

    return pixad;
}


/*-----------------------------------------------------------------*
 *              Morph sequence operation on each region            *
 *-----------------------------------------------------------------*/
/*!
 *  pixMorphSequenceByRegion()
 *
 *      Input:  pixs (1 bpp)
 *              pixm (mask specifying regions)
 *              sequence (string specifying sequence)
 *              connectivity (4 or 8, used on mask)
 *              minw  (minimum width to consider; use 0 or 1 for any width)
 *              minh  (minimum height to consider; use 0 or 1 for any height)
 *              &boxa (<optional> return boxa of c.c. in pixm)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) See pixMorphCompSequence() for composing operation sequences.
 *      (2) This operates separately on the region in pixs corresponding
 *          to each c.c. in the mask pixm.  It differs from
 *          pixMorphSequenceByComponent() in that the latter does not have
 *          a pixm (mask), but instead operates independently on each
 *          component in pixs.
 *      (3) Dilation will NOT increase the region size; the result
 *          is clipped to the size of the mask region.  This is necessary
 *          to make regions independent after the operation.
 *      (4) You can specify that the width and/or height of a region must
 *          equal or exceed a minimum size for the operation to take place.
 *      (5) Use NULL for @pboxa to avoid returning the boxa.
 */
PIX *
pixMorphSequenceByRegion(PIX         *pixs,
                         PIX         *pixm,
                         const char  *sequence,
                         l_int32      connectivity,
                         l_int32      minw,
                         l_int32      minh,
                         BOXA       **pboxa)
{
l_int32  n, i, x, y, w, h;
BOXA    *boxa;
PIX     *pix, *pixd;
PIXA    *pixam, *pixad;

    PROCNAME("pixMorphSequenceByRegion");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!pixm)
        return (PIX *)ERROR_PTR("pixm not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1 || pixGetDepth(pixm) != 1)
        return (PIX *)ERROR_PTR("pixs and pixm not both 1 bpp", procName, NULL);
    if (!sequence)
        return (PIX *)ERROR_PTR("sequence not defined", procName, NULL);

    if (minw <= 0) minw = 1;
    if (minh <= 0) minh = 1;

        /* Get the c.c. of the mask */
    if ((boxa = pixConnComp(pixm, &pixam, connectivity)) == NULL)
        return (PIX *)ERROR_PTR("boxa not made", procName, NULL);

        /* Operate on each region in pixs independently */
    pixad = pixaMorphSequenceByRegion(pixs, pixam, sequence, minw, minh);
    pixaDestroy(&pixam);
    boxaDestroy(&boxa);
    if (!pixad)
        return (PIX *)ERROR_PTR("pixad not made", procName, NULL);

        /* Display the result out into pixd */
    pixd = pixCreateTemplate(pixs);
    n = pixaGetCount(pixad);
    for (i = 0; i < n; i++) {
        pixaGetBoxGeometry(pixad, i, &x, &y, &w, &h);
        pix = pixaGetPix(pixad, i, L_CLONE);
        pixRasterop(pixd, x, y, w, h, PIX_PAINT, pix, 0, 0);
        pixDestroy(&pix);
    }

    if (pboxa)
        *pboxa = pixaGetBoxa(pixad, L_CLONE);
    pixaDestroy(&pixad);
    return pixd;
}


/*!
 *  pixaMorphSequenceByRegion()
 *
 *      Input:  pixs (1 bpp)
 *              pixam (of 1 bpp mask elements)
 *              sequence (string specifying sequence)
 *              minw  (minimum width to consider; use 0 or 1 for any width)
 *              minh  (minimum height to consider; use 0 or 1 for any height)
 *      Return: pixad, or null on error
 *
 *  Notes:
 *      (1) See pixMorphSequence() for composing operation sequences.
 *      (2) This operates separately on each region in the input pixs
 *          defined by the components in pixam.
 *      (3) You can specify that the width and/or height of a mask
 *          component must equal or exceed a minimum size for the
 *          operation to take place.
 *      (4) The input pixam should have a boxa giving the locations
 *          of the regions in pixs.
 */
PIXA *
pixaMorphSequenceByRegion(PIX         *pixs,
                          PIXA        *pixam,
                          const char  *sequence,
                          l_int32      minw,
                          l_int32      minh)
{
l_int32  n, i, w, h, samedepth, maxdepth, fullpa, fullba;
BOX     *box;
PIX     *pix1, *pix2, *pix3;
PIXA    *pixad;

    PROCNAME("pixaMorphSequenceByRegion");

    if (!pixs)
        return (PIXA *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIXA *)ERROR_PTR("pixs not 1 bpp", procName, NULL);
    if (!sequence)
        return (PIXA *)ERROR_PTR("sequence not defined", procName, NULL);
    if (!pixam)
        return (PIXA *)ERROR_PTR("pixam not defined", procName, NULL);
    samedepth = pixaVerifyDepth(pixam, &maxdepth);
    if (samedepth != 1 && maxdepth != 1)
        return (PIXA *)ERROR_PTR("mask depth not 1 bpp", procName, NULL);
    pixaIsFull(pixam, &fullpa, &fullba);
    if (!fullpa || !fullba)
        return (PIXA *)ERROR_PTR("missing comps in pixam", procName, NULL);
    n = pixaGetCount(pixam);
    if (minw <= 0) minw = 1;
    if (minh <= 0) minh = 1;

    if ((pixad = pixaCreate(n)) == NULL)
        return (PIXA *)ERROR_PTR("pixad not made", procName, NULL);

        /* Use the rectangle to remove the appropriate part of pixs;
         * then AND with the mask component to get the actual fg
         * of pixs that is under the mask component. */
    for (i = 0; i < n; i++) {
        pixaGetPixDimensions(pixam, i, &w, &h, NULL);
        if (w >= minw && h >= minh) {
            pix1 = pixaGetPix(pixam, i, L_CLONE);
            box = pixaGetBox(pixam, i, L_COPY);
            pix2 = pixClipRectangle(pixs, box, NULL);
            pixAnd(pix2, pix2, pix1);
            pix3 = pixMorphCompSequence(pix2, sequence, 0);
            pixDestroy(&pix1);
            pixDestroy(&pix2);
            if (!pix3) {
                boxDestroy(&box);
                pixaDestroy(&pixad);
                L_ERROR("pix3 not made in iter %d; aborting\n", procName, i);
                break;
            }
            pixaAddPix(pixad, pix3, L_INSERT);
            pixaAddBox(pixad, box, L_INSERT);
        }
    }

    return pixad;
}


/*-----------------------------------------------------------------*
 *      Union and intersection of parallel composite operations    *
 *-----------------------------------------------------------------*/
/*!
 *  pixUnionOfMorphOps()
 *
 *      Input:  pixs (binary)
 *              sela
 *              type (L_MORPH_DILATE, etc.)
 *      Return: pixd (union of the specified morphological operation
 *                    on pixs for each Sel in the Sela), or null on error
 */
PIX *
pixUnionOfMorphOps(PIX     *pixs,
                   SELA    *sela,
                   l_int32  type)
{
l_int32  n, i;
PIX     *pixt, *pixd;
SEL     *sel;

    PROCNAME("pixUnionOfMorphOps");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (!sela)
        return (PIX *)ERROR_PTR("sela not defined", procName, NULL);
    n = selaGetCount(sela);
    if (n == 0)
        return (PIX *)ERROR_PTR("no sels in sela", procName, NULL);
    if (type != L_MORPH_DILATE && type != L_MORPH_ERODE &&
        type != L_MORPH_OPEN && type != L_MORPH_CLOSE &&
        type != L_MORPH_HMT)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);

    pixd = pixCreateTemplate(pixs);
    for (i = 0; i < n; i++) {
        sel = selaGetSel(sela, i);
        if (type == L_MORPH_DILATE)
            pixt = pixDilate(NULL, pixs, sel);
        else if (type == L_MORPH_ERODE)
            pixt = pixErode(NULL, pixs, sel);
        else if (type == L_MORPH_OPEN)
            pixt = pixOpen(NULL, pixs, sel);
        else if (type == L_MORPH_CLOSE)
            pixt = pixClose(NULL, pixs, sel);
        else  /* type == L_MORPH_HMT */
            pixt = pixHMT(NULL, pixs, sel);
        pixOr(pixd, pixd, pixt);
        pixDestroy(&pixt);
    }

    return pixd;
}


/*!
 *  pixIntersectionOfMorphOps()
 *
 *      Input:  pixs (binary)
 *              sela
 *              type (L_MORPH_DILATE, etc.)
 *      Return: pixd (intersection of the specified morphological operation
 *                    on pixs for each Sel in the Sela), or null on error
 */
PIX *
pixIntersectionOfMorphOps(PIX     *pixs,
                          SELA    *sela,
                          l_int32  type)
{
l_int32  n, i;
PIX     *pixt, *pixd;
SEL     *sel;

    PROCNAME("pixIntersectionOfMorphOps");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (!sela)
        return (PIX *)ERROR_PTR("sela not defined", procName, NULL);
    n = selaGetCount(sela);
    if (n == 0)
        return (PIX *)ERROR_PTR("no sels in sela", procName, NULL);
    if (type != L_MORPH_DILATE && type != L_MORPH_ERODE &&
        type != L_MORPH_OPEN && type != L_MORPH_CLOSE &&
        type != L_MORPH_HMT)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);

    pixd = pixCreateTemplate(pixs);
    pixSetAll(pixd);
    for (i = 0; i < n; i++) {
        sel = selaGetSel(sela, i);
        if (type == L_MORPH_DILATE)
            pixt = pixDilate(NULL, pixs, sel);
        else if (type == L_MORPH_ERODE)
            pixt = pixErode(NULL, pixs, sel);
        else if (type == L_MORPH_OPEN)
            pixt = pixOpen(NULL, pixs, sel);
        else if (type == L_MORPH_CLOSE)
            pixt = pixClose(NULL, pixs, sel);
        else  /* type == L_MORPH_HMT */
            pixt = pixHMT(NULL, pixs, sel);
        pixAnd(pixd, pixd, pixt);
        pixDestroy(&pixt);
    }

    return pixd;
}



/*-----------------------------------------------------------------*
 *             Selective connected component filling               *
 *-----------------------------------------------------------------*/
/*!
 *  pixSelectiveConnCompFill()
 *
 *      Input:  pixs (binary)
 *              connectivity (4 or 8)
 *              minw  (minimum width to consider; use 0 or 1 for any width)
 *              minh  (minimum height to consider; use 0 or 1 for any height)
 *      Return: pix (with holes filled in selected c.c.), or null on error
 */
PIX *
pixSelectiveConnCompFill(PIX     *pixs,
                         l_int32  connectivity,
                         l_int32  minw,
                         l_int32  minh)
{
l_int32  n, i, x, y, w, h;
BOXA    *boxa;
PIX     *pix1, *pix2, *pixd;
PIXA    *pixa;

    PROCNAME("pixSelectiveConnCompFill");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);
    if (minw <= 0) minw = 1;
    if (minh <= 0) minh = 1;

    if ((boxa = pixConnComp(pixs, &pixa, connectivity)) == NULL)
        return (PIX *)ERROR_PTR("boxa not made", procName, NULL);
    n = boxaGetCount(boxa);
    pixd = pixCopy(NULL, pixs);
    for (i = 0; i < n; i++) {
        boxaGetBoxGeometry(boxa, i, &x, &y, &w, &h);
        if (w >= minw && h >= minh) {
            pix1 = pixaGetPix(pixa, i, L_CLONE);
            if ((pix2 = pixHolesByFilling(pix1, 12 - connectivity)) == NULL) {
                L_ERROR("pix2 not made in iter %d\n", procName, i);
                pixDestroy(&pix1);
                continue;
            }
            pixRasterop(pixd, x, y, w, h, PIX_PAINT, pix2, 0, 0);
            pixDestroy(&pix1);
            pixDestroy(&pix2);
        }
    }
    pixaDestroy(&pixa);
    boxaDestroy(&boxa);

    return pixd;
}


/*-----------------------------------------------------------------*
 *                    Removal of matched patterns                  *
 *-----------------------------------------------------------------*/
/*!
 *  pixRemoveMatchedPattern()
 *
 *      Input:  pixs (input image, 1 bpp)
 *              pixp (pattern to be removed from image, 1 bpp)
 *              pixe (image after erosion by Sel that approximates pixp, 1 bpp)
 *              x0, y0 (center of Sel)
 *              dsize (number of pixels on each side by which pixp is
 *                     dilated before being subtracted from pixs;
 *                     valid values are {0, 1, 2, 3, 4})
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *    (1) This is in-place.
 *    (2) You can use various functions in selgen to create a Sel
 *        that is used to generate pixe from pixs.
 *    (3) This function is applied after pixe has been computed.
 *        It finds the centroid of each c.c., and subtracts
 *        (the appropriately dilated version of) pixp, with the center
 *        of the Sel used to align pixp with pixs.
 */
l_int32
pixRemoveMatchedPattern(PIX     *pixs,
                        PIX     *pixp,
                        PIX     *pixe,
                        l_int32  x0,
                        l_int32  y0,
                        l_int32  dsize)
{
l_int32  i, nc, x, y, w, h, xb, yb;
BOXA    *boxa;
PIX     *pix1, *pix2;
PIXA    *pixa;
PTA     *pta;
SEL     *sel;

    PROCNAME("pixRemoveMatchedPattern");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixp)
        return ERROR_INT("pixp not defined", procName, 1);
    if (!pixe)
        return ERROR_INT("pixe not defined", procName, 1);
    if (pixGetDepth(pixs) != 1 || pixGetDepth(pixp) != 1 ||
        pixGetDepth(pixe) != 1)
        return ERROR_INT("all input pix not 1 bpp", procName, 1);
    if (dsize < 0 || dsize > 4)
        return ERROR_INT("dsize not in {0,1,2,3,4}", procName, 1);

        /* Find the connected components and their centroids */
    boxa = pixConnComp(pixe, &pixa, 8);
    if ((nc = boxaGetCount(boxa)) == 0) {
        L_WARNING("no matched patterns\n", procName);
        boxaDestroy(&boxa);
        pixaDestroy(&pixa);
        return 0;
    }
    pta = pixaCentroids(pixa);
    pixaDestroy(&pixa);

        /* Optionally dilate the pattern, first adding a border that
         * is large enough to accommodate the dilated pixels */
    sel = NULL;
    if (dsize > 0) {
        sel = selCreateBrick(2 * dsize + 1, 2 * dsize + 1, dsize, dsize,
                             SEL_HIT);
        pix1 = pixAddBorder(pixp, dsize, 0);
        pix2 = pixDilate(NULL, pix1, sel);
        selDestroy(&sel);
        pixDestroy(&pix1);
    } else {
        pix2 = pixClone(pixp);
    }

        /* Subtract out each dilated pattern.  The centroid of each
         * component is located at:
         *       (box->x + x, box->y + y)
         * and the 'center' of the pattern used in making pixe is located at
         *       (x0 + dsize, (y0 + dsize)
         * relative to the UL corner of the pattern.  The center of the
         * pattern is placed at the center of the component. */
    pixGetDimensions(pix2, &w, &h, NULL);
    for (i = 0; i < nc; i++) {
        ptaGetIPt(pta, i, &x, &y);
        boxaGetBoxGeometry(boxa, i, &xb, &yb, NULL, NULL);
        pixRasterop(pixs, xb + x - x0 - dsize, yb + y - y0 - dsize,
                    w, h, PIX_DST & PIX_NOT(PIX_SRC), pix2, 0, 0);
    }

    boxaDestroy(&boxa);
    ptaDestroy(&pta);
    pixDestroy(&pix2);
    return 0;
}


/*-----------------------------------------------------------------*
 *                    Display of matched patterns                  *
 *-----------------------------------------------------------------*/
/*!
 *  pixDisplayMatchedPattern()
 *
 *      Input:  pixs (input image, 1 bpp)
 *              pixp (pattern to be removed from image, 1 bpp)
 *              pixe (image after erosion by Sel that approximates pixp, 1 bpp)
 *              x0, y0 (center of Sel)
 *              color (to paint the matched patterns; 0xrrggbb00)
 *              scale (reduction factor for output pixd)
 *              nlevels (if scale < 1.0, threshold to this number of levels)
 *      Return: pixd (8 bpp, colormapped), or null on error
 *
 *  Notes:
 *    (1) A 4 bpp colormapped image is generated.
 *    (2) If scale <= 1.0, do scale to gray for the output, and threshold
 *        to nlevels of gray.
 *    (3) You can use various functions in selgen to create a Sel
 *        that will generate pixe from pixs.
 *    (4) This function is applied after pixe has been computed.
 *        It finds the centroid of each c.c., and colors the output
 *        pixels using pixp (appropriately aligned) as a stencil.
 *        Alignment is done using the origin of the Sel and the
 *        centroid of the eroded image to place the stencil pixp.
 */
PIX *
pixDisplayMatchedPattern(PIX       *pixs,
                         PIX       *pixp,
                         PIX       *pixe,
                         l_int32    x0,
                         l_int32    y0,
                         l_uint32   color,
                         l_float32  scale,
                         l_int32    nlevels)
{
l_int32   i, nc, xb, yb, x, y, xi, yi, rval, gval, bval;
BOXA     *boxa;
PIX      *pixd, *pixt, *pixps;
PIXA     *pixa;
PTA      *pta;
PIXCMAP  *cmap;

    PROCNAME("pixDisplayMatchedPattern");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!pixp)
        return (PIX *)ERROR_PTR("pixp not defined", procName, NULL);
    if (!pixe)
        return (PIX *)ERROR_PTR("pixe not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1 || pixGetDepth(pixp) != 1 ||
        pixGetDepth(pixe) != 1)
        return (PIX *)ERROR_PTR("all input pix not 1 bpp", procName, NULL);
    if (scale > 1.0 || scale <= 0.0) {
        L_WARNING("scale > 1.0 or < 0.0; setting to 1.0\n", procName);
        scale = 1.0;
    }

        /* Find the connected components and their centroids */
    boxa = pixConnComp(pixe, &pixa, 8);
    if ((nc = boxaGetCount(boxa)) == 0) {
        L_WARNING("no matched patterns\n", procName);
        boxaDestroy(&boxa);
        pixaDestroy(&pixa);
        return 0;
    }
    pta = pixaCentroids(pixa);

    extractRGBValues(color, &rval, &gval, &bval);
    if (scale == 1.0) {  /* output 4 bpp at full resolution */
        pixd = pixConvert1To4(NULL, pixs, 0, 1);
        cmap = pixcmapCreate(4);
        pixcmapAddColor(cmap, 255, 255, 255);
        pixcmapAddColor(cmap, 0, 0, 0);
        pixSetColormap(pixd, cmap);

        /* Paint through pixp for each match location.  The centroid of each
         * component in pixe is located at:
         *       (box->x + x, box->y + y)
         * and the 'center' of the pattern used in making pixe is located at
         *       (x0, y0)
         * relative to the UL corner of the pattern.  The center of the
         * pattern is placed at the center of the component. */
        for (i = 0; i < nc; i++) {
            ptaGetIPt(pta, i, &x, &y);
            boxaGetBoxGeometry(boxa, i, &xb, &yb, NULL, NULL);
            pixSetMaskedCmap(pixd, pixp, xb + x - x0, yb + y - y0,
                             rval, gval, bval);
        }
    } else {  /* output 4 bpp downscaled */
        pixt = pixScaleToGray(pixs, scale);
        pixd = pixThresholdTo4bpp(pixt, nlevels, 1);
        pixps = pixScaleBySampling(pixp, scale, scale);

        for (i = 0; i < nc; i++) {
            ptaGetIPt(pta, i, &x, &y);
            boxaGetBoxGeometry(boxa, i, &xb, &yb, NULL, NULL);
            xi = (l_int32)(scale * (xb + x - x0));
            yi = (l_int32)(scale * (yb + y - y0));
            pixSetMaskedCmap(pixd, pixps, xi, yi, rval, gval, bval);
        }
        pixDestroy(&pixt);
        pixDestroy(&pixps);
    }

    boxaDestroy(&boxa);
    pixaDestroy(&pixa);
    ptaDestroy(&pta);
    return pixd;
}


/*-----------------------------------------------------------------*
 *             Iterative morphological seed filling                *
 *-----------------------------------------------------------------*/
/*!
 *  pixSeedfillMorph()
 *
 *      Input:  pixs (seed)
 *              pixm (mask)
 *              maxiters (use 0 to go to completion)
 *              connectivity (4 or 8)
 *      Return: pixd (after filling into the mask) or null on error
 *
 *  Notes:
 *    (1) This is in general a very inefficient method for filling
 *        from a seed into a mask.  Use it for a small number of iterations,
 *        but if you expect more than a few iterations, use
 *        pixSeedfillBinary().
 *    (2) We use a 3x3 brick SEL for 8-cc filling and a 3x3 plus SEL for 4-cc.
 */
PIX *
pixSeedfillMorph(PIX     *pixs,
                 PIX     *pixm,
                 l_int32  maxiters,
                 l_int32  connectivity)
{
l_int32  same, i;
PIX     *pixt, *pixd, *temp;
SEL     *sel_3;

    PROCNAME("pixSeedfillMorph");

    if (!pixs || pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (!pixm)
        return (PIX *)ERROR_PTR("mask pix not defined", procName, NULL);
    if (connectivity != 4 && connectivity != 8)
        return (PIX *)ERROR_PTR("connectivity not in {4,8}", procName, NULL);
    if (maxiters <= 0) maxiters = 1000;
    if (pixSizesEqual(pixs, pixm) == 0)
        return (PIX *)ERROR_PTR("pix sizes unequal", procName, NULL);

    if ((sel_3 = selCreateBrick(3, 3, 1, 1, 1)) == NULL)
        return (PIX *)ERROR_PTR("sel_3 not made", procName, NULL);
    if (connectivity == 4) {  /* remove corner hits to make a '+' */
        selSetElement(sel_3, 0, 0, SEL_DONT_CARE);
        selSetElement(sel_3, 2, 2, SEL_DONT_CARE);
        selSetElement(sel_3, 2, 0, SEL_DONT_CARE);
        selSetElement(sel_3, 0, 2, SEL_DONT_CARE);
    }

    pixt = pixCopy(NULL, pixs);
    pixd = pixCreateTemplate(pixs);
    for (i = 1; i <= maxiters; i++) {
        pixDilate(pixd, pixt, sel_3);
        pixAnd(pixd, pixd, pixm);
        pixEqual(pixd, pixt, &same);
        if (same || i == maxiters)
            break;
        else
            SWAP(pixt, pixd);
    }
    fprintf(stderr, " Num iters in binary reconstruction = %d\n", i);

    pixDestroy(&pixt);
    selDestroy(&sel_3);
    return pixd;
}


/*-----------------------------------------------------------------*
 *                   Granulometry on binary images                 *
 *-----------------------------------------------------------------*/
/*!
 *  pixRunHistogramMorph()
 *
 *      Input:  pixs
 *              runtype (L_RUN_OFF, L_RUN_ON)
 *              direction (L_HORIZ, L_VERT)
 *              maxsize  (size of largest runlength counted)
 *      Return: numa of run-lengths
 */
NUMA *
pixRunHistogramMorph(PIX     *pixs,
                     l_int32  runtype,
                     l_int32  direction,
                     l_int32  maxsize)
{
l_int32    count, i, size;
l_float32  val;
NUMA      *na, *nah;
PIX       *pix1, *pix2, *pix3;
SEL       *sel_2a;

    PROCNAME("pixRunHistogramMorph");

    if (!pixs)
        return (NUMA *)ERROR_PTR("seed pix not defined", procName, NULL);
    if (runtype != L_RUN_OFF && runtype != L_RUN_ON)
        return (NUMA *)ERROR_PTR("invalid run type", procName, NULL);
    if (direction != L_HORIZ && direction != L_VERT)
        return (NUMA *)ERROR_PTR("direction not in {L_HORIZ, L_VERT}",
                                 procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (NUMA *)ERROR_PTR("pixs must be binary", procName, NULL);

    if (direction == L_HORIZ)
        sel_2a = selCreateBrick(1, 2, 0, 0, 1);
    else   /* direction == L_VERT */
        sel_2a = selCreateBrick(2, 1, 0, 0, 1);
    if (!sel_2a)
        return (NUMA *)ERROR_PTR("sel_2a not made", procName, NULL);

    if (runtype == L_RUN_OFF) {
        if ((pix1 = pixCopy(NULL, pixs)) == NULL)
            return (NUMA *)ERROR_PTR("pix1 not made", procName, NULL);
        pixInvert(pix1, pix1);
    } else {  /* runtype == L_RUN_ON */
        pix1 = pixClone(pixs);
    }

        /* Get pixel counts at different stages of erosion */
    na = numaCreate(0);
    pix2 = pixCreateTemplate(pixs);
    pix3 = pixCreateTemplate(pixs);
    pixCountPixels(pix1, &count, NULL);
    numaAddNumber(na, count);
    pixErode(pix2, pix1, sel_2a);
    pixCountPixels(pix2, &count, NULL);
    numaAddNumber(na, count);
    for (i = 0; i < maxsize / 2; i++) {
        pixErode(pix3, pix2, sel_2a);
        pixCountPixels(pix3, &count, NULL);
        numaAddNumber(na, count);
        pixErode(pix2, pix3, sel_2a);
        pixCountPixels(pix2, &count, NULL);
        numaAddNumber(na, count);
    }

        /* Compute length histogram */
    size = numaGetCount(na);
    nah = numaCreate(size);
    numaAddNumber(nah, 0); /* number at length 0 */
    for (i = 1; i < size - 1; i++) {
        val = na->array[i+1] - 2 * na->array[i] + na->array[i-1];
        numaAddNumber(nah, val);
    }

    pixDestroy(&pix1);
    pixDestroy(&pix2);
    pixDestroy(&pix3);
    selDestroy(&sel_2a);
    numaDestroy(&na);

    return nah;
}


/*-----------------------------------------------------------------*
 *            Composite operations on grayscale images             *
 *-----------------------------------------------------------------*/
/*!
 *  pixTophat()
 *
 *      Input:  pixs
 *              hsize (of Sel; must be odd; origin implicitly in center)
 *              vsize (ditto)
 *              type   (L_TOPHAT_WHITE: image - opening
 *                      L_TOPHAT_BLACK: closing - image)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Sel is a brick with all elements being hits
 *      (2) If hsize = vsize = 1, returns an image with all 0 data.
 *      (3) The L_TOPHAT_WHITE flag emphasizes small bright regions,
 *          whereas the L_TOPHAT_BLACK flag emphasizes small dark regions.
 *          The L_TOPHAT_WHITE tophat can be accomplished by doing a
 *          L_TOPHAT_BLACK tophat on the inverse, or v.v.
 */
PIX *
pixTophat(PIX     *pixs,
          l_int32  hsize,
          l_int32  vsize,
          l_int32  type)
{
PIX  *pixt, *pixd;

    PROCNAME("pixTophat");

    if (!pixs)
        return (PIX *)ERROR_PTR("seed pix not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize or vsize < 1", procName, NULL);
    if ((hsize & 1) == 0 ) {
        L_WARNING("horiz sel size must be odd; increasing by 1\n", procName);
        hsize++;
    }
    if ((vsize & 1) == 0 ) {
        L_WARNING("vert sel size must be odd; increasing by 1\n", procName);
        vsize++;
    }
    if (type != L_TOPHAT_WHITE && type != L_TOPHAT_BLACK)
        return (PIX *)ERROR_PTR("type must be L_TOPHAT_BLACK or L_TOPHAT_WHITE",
                                procName, NULL);

    if (hsize == 1 && vsize == 1)
        return pixCreateTemplate(pixs);

    switch (type)
    {
    case L_TOPHAT_WHITE:
        if ((pixt = pixOpenGray(pixs, hsize, vsize)) == NULL)
            return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
        pixd = pixSubtractGray(NULL, pixs, pixt);
        pixDestroy(&pixt);
        break;
    case L_TOPHAT_BLACK:
        if ((pixd = pixCloseGray(pixs, hsize, vsize)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
        pixSubtractGray(pixd, pixd, pixs);
        break;
    default:
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);
    }

    return pixd;
}


/*!
 *  pixHDome()
 *
 *      Input:  pixs (8 bpp, filling mask)
 *              height (of seed below the filling maskhdome; must be >= 0)
 *              connectivity (4 or 8)
 *      Return: pixd (8 bpp), or null on error
 *
 *  Notes:
 *      (1) It is more efficient to use a connectivity of 4 for the fill.
 *      (2) This fills bumps to some level, and extracts the unfilled
 *          part of the bump.  To extract the troughs of basins, first
 *          invert pixs and then apply pixHDome().
 *      (3) It is useful to compare the HDome operation with the TopHat.
 *          The latter extracts peaks or valleys that have a width
 *          not exceeding the size of the structuring element used
 *          in the opening or closing, rsp.  The height of the peak is
 *          irrelevant.  By contrast, for the HDome, the gray seedfill
 *          is used to extract all peaks that have a height not exceeding
 *          a given value, regardless of their width!
 *      (4) Slightly more precisely, suppose you set 'height' = 40.
 *          Then all bumps in pixs with a height greater than or equal
 *          to 40 become, in pixd, bumps with a max value of exactly 40.
 *          All shorter bumps have a max value in pixd equal to the height
 *          of the bump.
 *      (5) The method: the filling mask, pixs, is the image whose peaks
 *          are to be extracted.  The height of a peak is the distance
 *          between the top of the peak and the highest "leak" to the
 *          outside -- think of a sombrero, where the leak occurs
 *          at the highest point on the rim.
 *            (a) Generate a seed, pixd, by subtracting some value, p, from
 *                each pixel in the filling mask, pixs.  The value p is
 *                the 'height' input to this function.
 *            (b) Fill in pixd starting with this seed, clipping by pixs,
 *                in the way described in seedfillGrayLow().  The filling
 *                stops before the peaks in pixs are filled.
 *                For peaks that have a height > p, pixd is filled to
 *                the level equal to the (top-of-the-peak - p).
 *                For peaks of height < p, the peak is left unfilled
 *                from its highest saddle point (the leak to the outside).
 *            (c) Subtract the filled seed (pixd) from the filling mask (pixs).
 *          Note that in this procedure, everything is done starting
 *          with the filling mask, pixs.
 *      (6) For segmentation, the resulting image, pixd, can be thresholded
 *          and used as a seed for another filling operation.
 */
PIX *
pixHDome(PIX     *pixs,
         l_int32  height,
         l_int32  connectivity)
{
PIX  *pixsd, *pixd;

    PROCNAME("pixHDome");

    if (!pixs)
        return (PIX *)ERROR_PTR("src pix not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (height < 0)
        return (PIX *)ERROR_PTR("height not >= 0", procName, NULL);
    if (height == 0)
        return pixCreateTemplate(pixs);

    if ((pixsd = pixCopy(NULL, pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixsd not made", procName, NULL);
    pixAddConstantGray(pixsd, -height);
    pixSeedfillGray(pixsd, pixs, connectivity);
    pixd = pixSubtractGray(NULL, pixs, pixsd);
    pixDestroy(&pixsd);
    return pixd;
}


/*!
 *  pixFastTophat()
 *
 *      Input:  pixs
 *              xsize (width of max/min op, smoothing; any integer >= 1)
 *              ysize (height of max/min op, smoothing; any integer >= 1)
 *              type   (L_TOPHAT_WHITE: image - min
 *                      L_TOPHAT_BLACK: max - image)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Don't be fooled. This is NOT a tophat.  It is a tophat-like
 *          operation, where the result is similar to what you'd get
 *          if you used an erosion instead of an opening, or a dilation
 *          instead of a closing.
 *      (2) Instead of opening or closing at full resolution, it does
 *          a fast downscale/minmax operation, then a quick small smoothing
 *          at low res, a replicative expansion of the "background"
 *          to full res, and finally a removal of the background level
 *          from the input image.  The smoothing step may not be important.
 *      (3) It does not remove noise as well as a tophat, but it is
 *          5 to 10 times faster.
 *          If you need the preciseness of the tophat, don't use this.
 *      (4) The L_TOPHAT_WHITE flag emphasizes small bright regions,
 *          whereas the L_TOPHAT_BLACK flag emphasizes small dark regions.
 */
PIX *
pixFastTophat(PIX     *pixs,
              l_int32  xsize,
              l_int32  ysize,
              l_int32  type)
{
PIX  *pix1, *pix2, *pix3, *pixd;

    PROCNAME("pixFastTophat");

    if (!pixs)
        return (PIX *)ERROR_PTR("seed pix not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (xsize < 1 || ysize < 1)
        return (PIX *)ERROR_PTR("size < 1", procName, NULL);
    if (type != L_TOPHAT_WHITE && type != L_TOPHAT_BLACK)
        return (PIX *)ERROR_PTR("type must be L_TOPHAT_BLACK or L_TOPHAT_WHITE",
                                procName, NULL);

    if (xsize == 1 && ysize == 1)
        return pixCreateTemplate(pixs);

    switch (type)
    {
    case L_TOPHAT_WHITE:
        if ((pix1 = pixScaleGrayMinMax(pixs, xsize, ysize, L_CHOOSE_MIN))
               == NULL)
            return (PIX *)ERROR_PTR("pix1 not made", procName, NULL);
        pix2 = pixBlockconv(pix1, 1, 1);  /* small smoothing */
        pix3 = pixScaleBySampling(pix2, xsize, ysize);
        pixd = pixSubtractGray(NULL, pixs, pix3);
        pixDestroy(&pix3);
        break;
    case L_TOPHAT_BLACK:
        if ((pix1 = pixScaleGrayMinMax(pixs, xsize, ysize, L_CHOOSE_MAX))
               == NULL)
            return (PIX *)ERROR_PTR("pix1 not made", procName, NULL);
        pix2 = pixBlockconv(pix1, 1, 1);  /* small smoothing */
        pixd = pixScaleBySampling(pix2, xsize, ysize);
        pixSubtractGray(pixd, pixd, pixs);
        break;
    default:
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);
    }

    pixDestroy(&pix1);
    pixDestroy(&pix2);
    return pixd;
}


/*!
 *  pixMorphGradient()
 *
 *      Input:  pixs
 *              hsize (of Sel; must be odd; origin implicitly in center)
 *              vsize (ditto)
 *              smoothing  (half-width of convolution smoothing filter.
 *                          The width is (2 * smoothing + 1), so 0 is no-op.
 *      Return: pixd, or null on error
 */
PIX *
pixMorphGradient(PIX     *pixs,
                 l_int32  hsize,
                 l_int32  vsize,
                 l_int32  smoothing)
{
PIX  *pixg, *pixd;

    PROCNAME("pixMorphGradient");

    if (!pixs)
        return (PIX *)ERROR_PTR("seed pix not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);
    if (hsize < 1 || vsize < 1)
        return (PIX *)ERROR_PTR("hsize or vsize < 1", procName, NULL);
    if ((hsize & 1) == 0 ) {
        L_WARNING("horiz sel size must be odd; increasing by 1\n", procName);
        hsize++;
    }
    if ((vsize & 1) == 0 ) {
        L_WARNING("vert sel size must be odd; increasing by 1\n", procName);
        vsize++;
    }

        /* Optionally smooth first to remove noise.
         * If smoothing is 0, just get a copy */
    pixg = pixBlockconvGray(pixs, NULL, smoothing, smoothing);

        /* This gives approximately the gradient of a transition */
    pixd = pixDilateGray(pixg, hsize, vsize);
    pixSubtractGray(pixd, pixd, pixg);
    pixDestroy(&pixg);
    return pixd;
}


/*-----------------------------------------------------------------*
 *                       Centroid of component                     *
 *-----------------------------------------------------------------*/
/*!
 *  pixaCentroids()
 *
 *      Input:  pixa of components (1 or 8 bpp)
 *      Return: pta of centroids relative to the UL corner of
 *              each pix, or null on error
 *
 *  Notes:
 *      (1) An error message is returned if any pix has something other
 *          than 1 bpp or 8 bpp depth, and the centroid from that pix
 *          is saved as (0, 0).
 */
PTA *
pixaCentroids(PIXA  *pixa)
{
l_int32    i, n;
l_int32   *centtab = NULL;
l_int32   *sumtab = NULL;
l_float32  x, y;
PIX       *pix;
PTA       *pta;

    PROCNAME("pixaCentroids");

    if (!pixa)
        return (PTA *)ERROR_PTR("pixa not defined", procName, NULL);
    if ((n = pixaGetCount(pixa)) == 0)
        return (PTA *)ERROR_PTR("no pix in pixa", procName, NULL);

    if ((pta = ptaCreate(n)) == NULL)
        return (PTA *)ERROR_PTR("pta not defined", procName, NULL);
    centtab = makePixelCentroidTab8();
    sumtab = makePixelSumTab8();

    for (i = 0; i < n; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
        if (pixCentroid(pix, centtab, sumtab, &x, &y) == 1)
            L_ERROR("centroid failure for pix %d\n", procName, i);
        pixDestroy(&pix);
        ptaAddPt(pta, x, y);
    }

    FREE(centtab);
    FREE(sumtab);
    return pta;
}


/*!
 *  pixCentroid()
 *
 *      Input:  pix (1 or 8 bpp)
 *              centtab (<optional> table for finding centroids; can be null)
 *              sumtab (<optional> table for finding pixel sums; can be null)
 *              &xave, &yave (<return> coordinates of centroid, relative to
 *                            the UL corner of the pix)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Any table not passed in will be made internally and destroyed
 *          after use.
 */
l_int32
pixCentroid(PIX        *pix,
            l_int32    *centtab,
            l_int32    *sumtab,
            l_float32  *pxave,
            l_float32  *pyave)
{
l_int32    w, h, d, i, j, wpl, pixsum, rowsum, val;
l_float32  xsum, ysum;
l_uint32  *data, *line;
l_uint32   word;
l_uint8    byte;
l_int32   *ctab, *stab;

    PROCNAME("pixCentroid");

    if (!pxave || !pyave)
        return ERROR_INT("&pxave and &pyave not defined", procName, 1);
    *pxave = *pyave = 0.0;
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 1 && d != 8)
        return ERROR_INT("pix not 1 or 8 bpp", procName, 1);

    if (!centtab)
        ctab = makePixelCentroidTab8();
    else
        ctab = centtab;
    if (!sumtab)
        stab = makePixelSumTab8();
    else
        stab = sumtab;

    data = pixGetData(pix);
    wpl = pixGetWpl(pix);
    xsum = ysum = 0.0;
    pixsum = 0;
    if (d == 1) {
        for (i = 0; i < h; i++) {
                /* The body of this loop computes the sum of the set
                 * (1) bits on this row, weighted by their distance
                 * from the left edge of pix, and accumulates that into
                 * xsum; it accumulates their distance from the top
                 * edge of pix into ysum, and their total count into
                 * pixsum.  It's equivalent to
                 * for (j = 0; j < w; j++) {
                 *     if (GET_DATA_BIT(line, j)) {
                 *         xsum += j;
                 *         ysum += i;
                 *         pixsum++;
                 *     }
                 * }
                 */
            line = data + wpl * i;
            rowsum = 0;
            for (j = 0; j < wpl; j++) {
                word = line[j];
                if (word) {
                    byte = word & 0xff;
                    rowsum += stab[byte];
                    xsum += ctab[byte] + (j * 32 + 24) * stab[byte];
                    byte = (word >> 8) & 0xff;
                    rowsum += stab[byte];
                    xsum += ctab[byte] + (j * 32 + 16) * stab[byte];
                    byte = (word >> 16) & 0xff;
                    rowsum += stab[byte];
                    xsum += ctab[byte] + (j * 32 + 8) * stab[byte];
                    byte = (word >> 24) & 0xff;
                    rowsum += stab[byte];
                    xsum += ctab[byte] + j * 32 * stab[byte];
                }
            }
            pixsum += rowsum;
            ysum += rowsum * i;
        }
        if (pixsum == 0) {
            L_WARNING("no ON pixels in pix\n", procName);
        } else {
            *pxave = xsum / (l_float32)pixsum;
            *pyave = ysum / (l_float32)pixsum;
        }
    } else {  /* d == 8 */
        for (i = 0; i < h; i++) {
            line = data + wpl * i;
            for (j = 0; j < w; j++) {
                val = GET_DATA_BYTE(line, j);
                xsum += val * j;
                ysum += val * i;
                pixsum += val;
            }
        }
        if (pixsum == 0) {
            L_WARNING("all pixels are 0\n", procName);
        } else {
            *pxave = xsum / (l_float32)pixsum;
            *pyave = ysum / (l_float32)pixsum;
        }
    }

    if (!centtab) FREE(ctab);
    if (!sumtab) FREE(stab);
    return 0;
}
