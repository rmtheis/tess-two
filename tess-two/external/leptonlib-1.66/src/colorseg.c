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
 *  colorseg.c
 *
 *    Unsupervised color segmentation
 *
 *               PIX     *pixColorSegment()
 *               PIX     *pixColorSegmentCluster()
 *       static  l_int32  pixColorSegmentTryCluster()
 *               l_int32  pixAssignToNearestColor()
 *               l_int32  pixColorSegmentClean()
 *               l_int32  pixColorSegmentRemoveColors()
 *
 *    Selection and display of range of colors in HSV space
 *
 *               PIX     *pixMakeRangeMaskHS()
 *               PIX     *pixMakeRangeMaskHV()
 *               PIX     *pixMakeRangeMaskSV()
 *               PIX     *pixMakeHistoHS()
 *               PIX     *pixMakeHistoHV()
 *               PIX     *pixMakeHistoSV()
 *               PIX     *pixFindHistoPeaksHSV()
 *               PIX     *displayHSVColorRange()
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

    /* Maximum allowed iterations in Phase 1. */
static const l_int32  MAX_ALLOWED_ITERATIONS = 20;

    /* Factor by which max dist is increased on each iteration */
static const l_float32  DIST_EXPAND_FACT = 1.3;

    /* Octcube division level for computing nearest colormap color using LUT.
     * Using 4 should suffice for up to 50 - 100 colors, and it is
     * very fast.  Using 5 takes 8 times as long to set up the LUT
     * for little perceptual gain, even with 100 colors. */
static const l_int32  LEVEL_IN_OCTCUBE = 4;


static l_int32 pixColorSegmentTryCluster(PIX *pixd, PIX *pixs,
                                         l_int32 maxdist, l_int32 maxcolors);


#ifndef  NO_CONSOLE_IO
#define  DEBUG_HISTO       1
#endif  /* ~NO_CONSOLE_IO */


/*------------------------------------------------------------------*
 *                 Unsupervised color segmentation                  *
 *------------------------------------------------------------------*/
/*!
 *  pixColorSegment()
 *
 *      Input:  pixs  (32 bpp; 24-bit color)
 *              maxdist (max euclidean dist to existing cluster)
 *              maxcolors (max number of colors allowed in first pass)
 *              selsize (linear size of sel for closing to remove noise)
 *              finalcolors (max number of final colors allowed after 4th pass)
 *      Return: pixd (8 bit with colormap), or null on error
 *
 *  Color segmentation proceeds in four phases:
 *
 *  Phase 1:  pixColorSegmentCluster()
 *  The image is traversed in raster order.  Each pixel either
 *  becomes the representative for a new cluster or is assigned to an
 *  existing cluster.  Assignment is greedy.  The data is stored in
 *  a colormapped image.  Three auxiliary arrays are used to hold
 *  the colors of the representative pixels, for fast lookup.
 *  The average color in each cluster is computed.
 *
 *  Phase 2.  pixAssignToNearestColor()
 *  A second (non-greedy) clustering pass is performed, where each pixel
 *  is assigned to the nearest cluster (average).  We also keep track
 *  of how many pixels are assigned to each cluster.
 *
 *  Phase 3.  pixColorSegmentClean()
 *  For each cluster, starting with the largest, do a morphological
 *  closing to eliminate small components within larger ones.
 *
 *  Phase 4.  pixColorSegmentRemoveColors()
 *  Eliminate all colors except the most populated 'finalcolors'.
 *  Then remove unused colors from the colormap, and reassign those
 *  pixels to the nearest remaining cluster, using the original pixel values.
 *
 *  Notes:
 *      (1) The goal is to generate a small number of colors.
 *          Typically this would be specified by 'finalcolors',
 *          a number that would be somewhere between 3 and 6.
 *          The parameter 'maxcolors' specifies the maximum number of
 *          colors generated in the first phase.  This should be
 *          larger than finalcolors, perhaps twice as large.
 *          If more than 'maxcolors' are generated in the first phase
 *          using the input 'maxdist', the distance is repeatedly
 *          increased by a multiplicative factor until the condition
 *          is satisfied.  The implicit relation between 'maxdist'
 *          and 'maxcolors' is thus adjusted programmatically.
 *      (2) As a very rough guideline, given a target value of 'finalcolors',
 *          here are approximate values of 'maxdist' and 'maxcolors'
 *          to start with:
 *
 *               finalcolors    maxcolors    maxdist
 *               -----------    ---------    -------
 *                   3             6          100
 *                   4             8           90
 *                   5            10           75
 *                   6            12           60
 *
 *          For a given number of finalcolors, if you use too many
 *          maxcolors, the result will be noisy.  If you use too few,
 *          the result will be a relatively poor assignment of colors.
 */
PIX *
pixColorSegment(PIX     *pixs,
                l_int32  maxdist,
                l_int32  maxcolors,
                l_int32  selsize,
                l_int32  finalcolors)
{
l_int32   *countarray;
PIX       *pixd;

    PROCNAME("pixColorSegment");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("must be rgb color", procName, NULL);

        /* Phase 1; original segmentation */
    if ((pixd = pixColorSegmentCluster(pixs, maxdist, maxcolors)) == NULL)
        return (PIX *)ERROR_PTR("pixt1 not made", procName, NULL);
/*    pixWrite("junkpixd1", pixd, IFF_PNG);  */

        /* Phase 2; refinement in pixel assignment */
    if ((countarray = (l_int32 *)CALLOC(256, sizeof(l_int32))) == NULL)
        return (PIX *)ERROR_PTR("countarray not made", procName, NULL);
    pixAssignToNearestColor(pixd, pixs, NULL, LEVEL_IN_OCTCUBE, countarray);
/*    pixWrite("junkpixd2", pixd, IFF_PNG);  */

        /* Phase 3: noise removal by separately closing each color */
    pixColorSegmentClean(pixd, selsize, countarray);
/*    pixWrite("junkpixd3", pixd, IFF_PNG);  */
    FREE(countarray);

        /* Phase 4: removal of colors with small population and
         * reassignment of pixels to remaining colors */
    pixColorSegmentRemoveColors(pixd, pixs, finalcolors);
    return pixd;
}


/*!
 *  pixColorSegmentCluster()
 *
 *      Input:  pixs  (32 bpp; 24-bit color)
 *              maxdist (max euclidean dist to existing cluster)
 *              maxcolors (max number of colors allowed in first pass)
 *      Return: pixd (8 bit with colormap), or null on error
 *
 *  Notes:
 *      (1) This is phase 1.  See description in pixColorSegment().
 *      (2) Greedy unsupervised classification.  If the limit 'maxcolors'
 *          is exceeded, the computation is repeated with a larger
 *          allowed cluster size.
 *      (3) On each successive iteration, 'maxdist' is increased by a
 *          constant factor.  See comments in pixColorSegment() for
 *          a guideline on parameter selection.
 *          Note that the diagonal of the 8-bit rgb color cube is about
 *          440, so for 'maxdist' = 440, you are guaranteed to get 1 color!
 */
PIX *
pixColorSegmentCluster(PIX       *pixs,
                       l_int32    maxdist,
                       l_int32    maxcolors)
{
l_int32    w, h, newmaxdist, ret, niters, ncolors, success;
PIX       *pixd;
PIXCMAP   *cmap;

    PROCNAME("pixColorSegmentCluster");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("must be rgb color", procName, NULL);

    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    cmap = pixcmapCreate(8);
    pixSetColormap(pixd, cmap);
    pixCopyResolution(pixd, pixs);

    newmaxdist = maxdist;
    niters = 0;
    success = TRUE;
    while (1) {
        ret = pixColorSegmentTryCluster(pixd, pixs, newmaxdist, maxcolors);
        niters++;
        if (!ret) {
            ncolors = pixcmapGetCount(cmap);
            L_INFO_INT2("Success with %d colors after %d iters", procName,
                       ncolors, niters);
            break;
        }
        if (niters == MAX_ALLOWED_ITERATIONS) {
            L_WARNING_INT("too many iters; newmaxdist = %d",
                          procName, newmaxdist);
            success = FALSE;
            break;
        }
        newmaxdist = (l_int32)(DIST_EXPAND_FACT * (l_float32)newmaxdist);
    }

    if (!success) {
        pixDestroy(&pixd);
        return (PIX *)ERROR_PTR("failure in phase 1", procName, NULL);
    }

    return pixd;
}


/*!
 *  pixColorSegmentTryCluster()
 *
 *      Input:  pixd
 *              pixs
 *              maxdist
 *              maxcolors
 *      Return: 0 if OK, 1 on error
 *
 *  Note: This function should only be called from pixColorSegCluster()
 */
static l_int32
pixColorSegmentTryCluster(PIX       *pixd,
                          PIX       *pixs,
                          l_int32    maxdist,
                          l_int32    maxcolors)
{
l_int32    rmap[256], gmap[256], bmap[256];
l_int32    w, h, wpls, wpld, i, j, k, found, ret, index, ncolors;
l_int32    rval, gval, bval, dist2, maxdist2;
l_int32    countarray[256];
l_int32    rsum[256], gsum[256], bsum[256];
l_uint32  *ppixel;
l_uint32  *datas, *datad, *lines, *lined;
PIXCMAP   *cmap;

    PROCNAME("pixColorSegmentTryCluster");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);

    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);
    maxdist2 = maxdist * maxdist;
    cmap = pixGetColormap(pixd);
    pixcmapClear(cmap);
    for (k = 0; k < 256; k++) {
        rsum[k] = gsum[k] = bsum[k] = 0;
        rmap[k] = gmap[k] = bmap[k] = 0;
    }

    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            ppixel = lines + j;
            rval = GET_DATA_BYTE(ppixel, COLOR_RED);
            gval = GET_DATA_BYTE(ppixel, COLOR_GREEN);
            bval = GET_DATA_BYTE(ppixel, COLOR_BLUE);
            ncolors = pixcmapGetCount(cmap);
            found = FALSE;
            for (k = 0; k < ncolors; k++) {
                dist2 = (rval - rmap[k]) * (rval - rmap[k]) +
                        (gval - gmap[k]) * (gval - gmap[k]) +
                        (bval - bmap[k]) * (bval - bmap[k]);
                if (dist2 <= maxdist2) {  /* take it; greedy */
                    found = TRUE;
                    SET_DATA_BYTE(lined, j, k);
                    countarray[k]++;
                    rsum[k] += rval;
                    gsum[k] += gval;
                    bsum[k] += bval;
                    break;
                }
            }
            if (!found) {  /* Add a new color */
                ret = pixcmapAddNewColor(cmap, rval, gval, bval, &index);
/*                fprintf(stderr,
                        "index = %d, (i,j) = (%d,%d), rgb = (%d, %d, %d)\n",
                        index, i, j, rval, gval, bval); */
                if (ret == 0 && index < maxcolors) {
                    countarray[index] = 1;
                    SET_DATA_BYTE(lined, j, index);
                    rmap[index] = rval;
                    gmap[index] = gval;
                    bmap[index] = bval;
                    rsum[index] = rval;
                    gsum[index] = gval;
                    bsum[index] = bval;
                }
                else  {
                    L_INFO_INT("maxcolors exceeded for maxdist = %d",
                               procName, maxdist);
                    return 1;
                }
            }
        }
    }

        /* Replace the colors in the colormap by the averages */
    for (k = 0; k < ncolors; k++) {
        rval = rsum[k] / countarray[k];
        gval = gsum[k] / countarray[k];
        bval = bsum[k] / countarray[k];
        pixcmapResetColor(cmap, k, rval, gval, bval);
    }

    return 0;
}


/*!
 *  pixAssignToNearestColor()
 *
 *      Input:  pixd  (8 bpp, colormapped)
 *              pixs  (32 bpp; 24-bit color)
 *              pixm  (<optional> 1 bpp)
 *              level (of octcube used for finding nearest color in cmap)
 *              countarray (<optional> ptr to array, in which we can store
 *                          the number of pixels found in each color in
 *                          the colormap in pixd)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is used in phase 2 of color segmentation, where pixs
 *          is the original input image to pixColorSegment(), and
 *          pixd is the colormapped image returned from
 *          pixColorSegmentCluster().  It is also used, with a mask,
 *          in phase 4.
 *      (2) This is an in-place operation.
 *      (3) The colormap in pixd is unchanged.
 *      (4) pixs and pixd must be the same size (w, h).
 *      (5) The selection mask pixm can be null.  If it exists, it must
 *          be the same size as pixs and pixd, and only pixels
 *          corresponding to fg in pixm are assigned.  Set to
 *          NULL if all pixels in pixd are to be assigned.
 *      (6) The countarray can be null.  If it exists, it is pre-allocated
 *          and of a size at least equal to the size of the colormap in pixd.
 *      (7) This does a best-fit (non-greedy) assignment of pixels to
 *          existing clusters.  Specifically, it assigns each pixel
 *          in pixd to the color index in the pixd colormap that has a
 *          color closest to the corresponding rgb pixel in pixs.
 *      (8) 'level' is the octcube level used to quickly find the nearest
 *          color in the colormap for each pixel.  For color segmentation,
 *          this parameter is set to LEVEL_IN_OCTCUBE.
 *      (9) We build a mapping table from octcube to colormap index so
 *          that this function can run in a time (otherwise) independent
 *          of the number of colors in the colormap.  This avoids a
 *          brute-force search for the closest colormap color to each
 *          pixel in the image.
 */
l_int32
pixAssignToNearestColor(PIX      *pixd,
                        PIX      *pixs,
                        PIX      *pixm,
                        l_int32   level,
                        l_int32  *countarray)
{
l_int32    w, h, wpls, wpld, wplm, i, j;
l_int32    rval, gval, bval, index;
l_int32   *cmaptab;
l_uint32   octindex;
l_uint32  *rtab, *gtab, *btab;
l_uint32  *ppixel;
l_uint32  *datas, *datad, *datam, *lines, *lined, *linem;
PIXCMAP   *cmap;

    PROCNAME("pixAssignToNearestColor");

    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if ((cmap = pixGetColormap(pixd)) == NULL)
        return ERROR_INT("cmap not found", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetDepth(pixs) != 32)
        return ERROR_INT("pixs not 32 bpp", procName, 1);

        /* Set up the tables to map rgb to the nearest colormap index */
    if (makeRGBToIndexTables(&rtab, &gtab, &btab, level))
        return ERROR_INT("index tables not made", procName, 1);
    if ((cmaptab = pixcmapToOctcubeLUT(cmap, level, L_MANHATTAN_DISTANCE))
            == NULL)
        return ERROR_INT("cmaptab not made", procName, 1);

    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    if (pixm) {
        datam = pixGetData(pixm);
        wplm = pixGetWpl(pixm);
    }
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        if (pixm)
            linem = datam + i * wplm;
        for (j = 0; j < w; j++) {
            if (pixm) {
                if (!GET_DATA_BIT(linem, j))
                    continue;
            }
            ppixel = lines + j;
            rval = GET_DATA_BYTE(ppixel, COLOR_RED);
            gval = GET_DATA_BYTE(ppixel, COLOR_GREEN);
            bval = GET_DATA_BYTE(ppixel, COLOR_BLUE);
                /* Map from rgb to octcube index */
            getOctcubeIndexFromRGB(rval, gval, bval, rtab, gtab, btab,
                                   &octindex);
                /* Map from octcube index to nearest colormap index */
            index = cmaptab[octindex];
            if (countarray)
                countarray[index]++;
            SET_DATA_BYTE(lined, j, index);
        }
    }

    FREE(cmaptab);
    FREE(rtab);
    FREE(gtab);
    FREE(btab);
    return 0;
}


/*!
 *  pixColorSegmentClean()
 *
 *      Input:  pixs  (8 bpp, colormapped)
 *              selsize (for closing)
 *              countarray (ptr to array containing the number of pixels
 *                          found in each color in the colormap)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This operation is in-place.
 *      (2) This is phase 3 of color segmentation.  It is the first
 *          part of a two-step noise removal process.  Colors with a
 *          large population are closed first; this operation absorbs
 *          small sets of intercolated pixels of a different color.
 */
l_int32
pixColorSegmentClean(PIX      *pixs,
                     l_int32   selsize,
                     l_int32  *countarray)
{
l_int32    i, ncolors, val;
l_uint32   val32;
NUMA      *na, *nasi;
PIX       *pixt1, *pixt2;
PIXCMAP   *cmap;

    PROCNAME("pixColorSegmentClean");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (pixGetDepth(pixs) != 8)
        return ERROR_INT("pixs not 8 bpp", procName, 1);
    if ((cmap = pixGetColormap(pixs)) == NULL)
        return ERROR_INT("cmap not found", procName, 1);
    if (!countarray)
        return ERROR_INT("countarray not defined", procName, 1);
    if (selsize <= 1)
        return 0;  /* nothing to do */

        /* Sort colormap indices in decreasing order of pixel population */
    ncolors = pixcmapGetCount(cmap);
    na = numaCreate(ncolors);
    for (i = 0; i < ncolors; i++)
        numaAddNumber(na, countarray[i]);
    if ((nasi = numaGetSortIndex(na, L_SORT_DECREASING)) == NULL)
        return ERROR_INT("nasi not made", procName, 1);

        /* For each color, in order of decreasing population,
         * do a closing and absorb the added pixels.  Note that
         * if the closing removes pixels at the border, they'll
         * still appear in the xor and will be properly (re)set. */
    for (i = 0; i < ncolors; i++) {
        numaGetIValue(nasi, i, &val);
        pixt1 = pixGenerateMaskByValue(pixs, val, 1);
        pixt2 = pixCloseSafeCompBrick(NULL, pixt1, selsize, selsize);
        pixXor(pixt2, pixt2, pixt1);  /* pixels to be added to type 'val' */
        pixcmapGetColor32(cmap, val, &val32);
        pixSetMasked(pixs, pixt2, val32);  /* add them */
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
    }
    numaDestroy(&na);
    numaDestroy(&nasi);
    return 0;
}


/*!
 *  pixColorSegmentRemoveColors()
 *
 *      Input:  pixd  (8 bpp, colormapped)
 *              pixs  (32 bpp rgb, with initial pixel values)
 *              finalcolors (max number of colors to retain)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This operation is in-place.
 *      (2) This is phase 4 of color segmentation, and the second part
 *          of the 2-step noise removal.  Only 'finalcolors' different
 *          colors are retained, with colors with smaller populations
 *          being replaced by the nearest color of the remaining colors.
 *          For highest accuracy, for pixels that are being replaced,
 *          we find the nearest colormap color  to the original rgb color.
 */
l_int32
pixColorSegmentRemoveColors(PIX     *pixd,
                            PIX     *pixs,
                            l_int32  finalcolors)
{
l_int32    i, ncolors, index, tempindex;
l_int32   *tab;
l_uint32   tempcolor;
NUMA      *na, *nasi;
PIX       *pixm;
PIXCMAP   *cmap;

    PROCNAME("pixColorSegmentRemoveColors");

    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (pixGetDepth(pixd) != 8)
        return ERROR_INT("pixd not 8 bpp", procName, 1);
    if ((cmap = pixGetColormap(pixd)) == NULL)
        return ERROR_INT("cmap not found", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    ncolors = pixcmapGetCount(cmap);
    if (finalcolors >= ncolors)  /* few enough colors already; nothing to do */
        return 0;

        /* Generate a mask over all pixels that are not in the
         * 'finalcolors' most populated colors.  Save the colormap
         * index of any one of the retained colors in 'tempindex'.
         * The LUT has values 0 for the 'finalcolors' most populated colors,
         * which will be retained; and 1 for the rest, which are marked
         * by fg pixels in pixm and will be removed. */
    na = pixGetCmapHistogram(pixd, 1);
    if ((nasi = numaGetSortIndex(na, L_SORT_DECREASING)) == NULL) {
        numaDestroy(&na);
        return ERROR_INT("nasi not made", procName, 1);
    }
    numaGetIValue(nasi, finalcolors - 1, &tempindex);  /* retain down to this */
    pixcmapGetColor32(cmap, tempindex, &tempcolor);  /* use this color */
    tab = (l_int32 *)CALLOC(256, sizeof(l_int32));
    for (i = finalcolors; i < ncolors; i++) {
        numaGetIValue(nasi, i, &index);
        tab[index] = 1;
    }

    pixm = pixMakeMaskFromLUT(pixd, tab);
    FREE(tab);

        /* Reassign the masked pixels temporarily to the saved index
         * (tempindex).  This guarantees that no pixels are labeled by
         * a colormap index of any colors that will be removed.
         * The actual value doesn't matter, as long as it's one
         * of the retained colors, because these pixels will later
         * be reassigned based on the full set of colors retained
         * in the colormap. */
    pixSetMasked(pixd, pixm, tempcolor);

        /* Now remove unused colors from the colormap.  This reassigns
         * image pixels as required. */
    pixRemoveUnusedColors(pixd);

        /* Finally, reassign the pixels under the mask (those that were
         * given a 'tempindex' value) to the nearest color in the colormap.
         * This is the function used in phase 2 on all image pixels; here
         * it is only used on the masked pixels given by pixm. */
    pixAssignToNearestColor(pixd, pixs, pixm, LEVEL_IN_OCTCUBE, NULL);

    pixDestroy(&pixm);
    numaDestroy(&na);
    numaDestroy(&nasi);
    return 0;
}


/*------------------------------------------------------------------*
 *       Selection and display of range of colors in HSV space      *
 *------------------------------------------------------------------*/
/*!
 *  pixMakeRangeMaskHS()
 *
 *      Input:  pixs  (32 bpp rgb)
 *              huecenter (center value of hue range)
 *              huehw (half-width of hue range)
 *              satcenter (center value of saturation range)
 *              sathw (half-width of saturation range)
 *              regionflag (L_INCLUDE_REGION, L_EXCLUDE_REGION)
 *      Return: pixd (1 bpp mask over selected pixels), or null on error
 *
 *  Notes:
 *      (1) The pixels are selected based on the specified ranges of
 *          hue and saturation.  For selection or exclusion, the pixel
 *          HS component values must be within both ranges.  Care must
 *          be taken in finding the hue range because of wrap-around.
 *      (2) Use @regionflag == L_INCLUDE_REGION to take only those
 *          pixels within the rectangular region specified in HS space.
 *          Use @regionflag == L_EXCLUDE_REGION to take all pixels except
 *          those within the rectangular region specified in HS space.
 */
PIX *
pixMakeRangeMaskHS(PIX     *pixs,
                   l_int32  huecenter,
                   l_int32  huehw,
                   l_int32  satcenter,
                   l_int32  sathw,
                   l_int32  regionflag)
{
l_int32    i, j, w, h, wplt, wpld, hstart, hend, sstart, send, hval, sval;
l_int32   *hlut, *slut;
l_uint32   pixel;
l_uint32  *datat, *datad, *linet, *lined;
PIX       *pixt, *pixd;

    PROCNAME("pixMakeRangeMaskHS");

    if (!pixs || pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);
    if (regionflag != L_INCLUDE_REGION && regionflag != L_EXCLUDE_REGION)
        return (PIX *)ERROR_PTR("invalid regionflag", procName, NULL);

        /* Set up LUTs for hue and saturation.  These have the value 1
         * within the specified intervals of hue and saturation. */
    hlut = (l_int32 *)CALLOC(240, sizeof(l_int32));
    slut = (l_int32 *)CALLOC(256, sizeof(l_int32));
    sstart = L_MAX(0, satcenter - sathw);
    send = L_MIN(255, satcenter + sathw);
    for (i = sstart; i <= send; i++)
        slut[i] = 1;
    hstart = (huecenter - huehw + 240) % 240;
    hend = (huecenter + huehw + 240) % 240;
    if (hstart < hend) {
        for (i = hstart; i <= hend; i++)
            hlut[i] = 1;
    }
    else {  /* wrap */
        for (i = hstart; i < 240; i++)
            hlut[i] = 1;
        for (i = 0; i <= hend; i++)
            hlut[i] = 1;
    }

        /* Generate the mask */
    pixt = pixConvertRGBToHSV(NULL, pixs);
    pixGetDimensions(pixs, &w, &h, NULL);
    pixd = pixCreateNoInit(w, h, 1);
    if (regionflag == L_INCLUDE_REGION)
        pixClearAll(pixd);
    else  /* L_EXCLUDE_REGION */
        pixSetAll(pixd);
    datat = pixGetData(pixt);
    datad = pixGetData(pixd);
    wplt = pixGetWpl(pixt);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            pixel = linet[j];
            hval = (pixel >> L_RED_SHIFT) & 0xff;
            sval = (pixel >> L_GREEN_SHIFT) & 0xff;
            if (hlut[hval] == 1 && slut[sval] == 1) {
                if (regionflag == L_INCLUDE_REGION)
                    SET_DATA_BIT(lined, j);
                else  /* L_EXCLUDE_REGION */
                    CLEAR_DATA_BIT(lined, j);
            }
        }
    }

    FREE(hlut);
    FREE(slut);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixMakeRangeMaskHV()
 *
 *      Input:  pixs  (32 bpp rgb)
 *              huecenter (center value of hue range)
 *              huehw (half-width of hue range)
 *              valcenter (center value of max intensity range)
 *              valhw (half-width of max intensity range)
 *              regionflag (L_INCLUDE_REGION, L_EXCLUDE_REGION)
 *      Return: pixd (1 bpp mask over selected pixels), or null on error
 *
 *  Notes:
 *      (1) The pixels are selected based on the specified ranges of
 *          hue and max intensity values.  For selection or exclusion,
 *          the pixel HV component values must be within both ranges.
 *          Care must be taken in finding the hue range because of wrap-around.
 *      (2) Use @regionflag == L_INCLUDE_REGION to take only those
 *          pixels within the rectangular region specified in HV space.
 *          Use @regionflag == L_EXCLUDE_REGION to take all pixels except
 *          those within the rectangular region specified in HV space.
 */
PIX *
pixMakeRangeMaskHV(PIX     *pixs,
                   l_int32  huecenter,
                   l_int32  huehw,
                   l_int32  valcenter,
                   l_int32  valhw,
                   l_int32  regionflag)
{
l_int32    i, j, w, h, wplt, wpld, hstart, hend, vstart, vend, hval, vval;
l_int32   *hlut, *vlut;
l_uint32   pixel;
l_uint32  *datat, *datad, *linet, *lined;
PIX       *pixt, *pixd;

    PROCNAME("pixMakeRangeMaskHV");

    if (!pixs || pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);
    if (regionflag != L_INCLUDE_REGION && regionflag != L_EXCLUDE_REGION)
        return (PIX *)ERROR_PTR("invalid regionflag", procName, NULL);

        /* Set up LUTs for hue and maximum intensity (val).  These have
         * the value 1 within the specified intervals of hue and value. */
    hlut = (l_int32 *)CALLOC(240, sizeof(l_int32));
    vlut = (l_int32 *)CALLOC(256, sizeof(l_int32));
    vstart = L_MAX(0, valcenter - valhw);
    vend = L_MIN(255, valcenter + valhw);
    for (i = vstart; i <= vend; i++)
        vlut[i] = 1;
    hstart = (huecenter - huehw + 240) % 240;
    hend = (huecenter + huehw + 240) % 240;
    if (hstart < hend) {
        for (i = hstart; i <= hend; i++)
            hlut[i] = 1;
    }
    else {
        for (i = hstart; i < 240; i++)
            hlut[i] = 1;
        for (i = 0; i <= hend; i++)
            hlut[i] = 1;
    }

        /* Generate the mask */
    pixt = pixConvertRGBToHSV(NULL, pixs);
    pixGetDimensions(pixs, &w, &h, NULL);
    pixd = pixCreateNoInit(w, h, 1);
    if (regionflag == L_INCLUDE_REGION)
        pixClearAll(pixd);
    else  /* L_EXCLUDE_REGION */
        pixSetAll(pixd);
    datat = pixGetData(pixt);
    datad = pixGetData(pixd);
    wplt = pixGetWpl(pixt);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            pixel = linet[j];
            hval = (pixel >> L_RED_SHIFT) & 0xff;
            vval = (pixel >> L_BLUE_SHIFT) & 0xff;
            if (hlut[hval] == 1 && vlut[vval] == 1) {
                if (regionflag == L_INCLUDE_REGION)
                    SET_DATA_BIT(lined, j);
                else  /* L_EXCLUDE_REGION */
                    CLEAR_DATA_BIT(lined, j);
            }
        }
    }

    FREE(hlut);
    FREE(vlut);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixMakeRangeMaskSV()
 *
 *      Input:  pixs  (32 bpp rgb)
 *              satcenter (center value of saturation range)
 *              sathw (half-width of saturation range)
 *              valcenter (center value of max intensity range)
 *              valhw (half-width of max intensity range)
 *              regionflag (L_INCLUDE_REGION, L_EXCLUDE_REGION)
 *      Return: pixd (1 bpp mask over selected pixels), or null on error
 *
 *  Notes:
 *      (1) The pixels are selected based on the specified ranges of
 *          saturation and max intensity (val).  For selection or
 *          exclusion, the pixel SV component values must be within both ranges.
 *      (2) Use @regionflag == L_INCLUDE_REGION to take only those
 *          pixels within the rectangular region specified in SV space.
 *          Use @regionflag == L_EXCLUDE_REGION to take all pixels except
 *          those within the rectangular region specified in SV space.
 */
PIX *
pixMakeRangeMaskSV(PIX     *pixs,
                   l_int32  satcenter,
                   l_int32  sathw,
                   l_int32  valcenter,
                   l_int32  valhw,
                   l_int32  regionflag)
{
l_int32    i, j, w, h, wplt, wpld, sval, vval, sstart, send, vstart, vend;
l_int32   *slut, *vlut;
l_uint32   pixel;
l_uint32  *datat, *datad, *linet, *lined;
PIX       *pixt, *pixd;

    PROCNAME("pixMakeRangeMaskSV");

    if (!pixs || pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);
    if (regionflag != L_INCLUDE_REGION && regionflag != L_EXCLUDE_REGION)
        return (PIX *)ERROR_PTR("invalid regionflag", procName, NULL);

        /* Set up LUTs for saturation and max intensity (val).
         * These have the value 1 within the specified intervals of
         * saturation and max intensity. */
    slut = (l_int32 *)CALLOC(256, sizeof(l_int32));
    vlut = (l_int32 *)CALLOC(256, sizeof(l_int32));
    sstart = L_MAX(0, satcenter - sathw);
    send = L_MIN(255, satcenter + sathw);
    vstart = L_MAX(0, valcenter - valhw);
    vend = L_MIN(255, valcenter + valhw);
    for (i = sstart; i <= send; i++)
        slut[i] = 1;
    for (i = vstart; i <= vend; i++)
        vlut[i] = 1;

        /* Generate the mask */
    pixt = pixConvertRGBToHSV(NULL, pixs);
    pixGetDimensions(pixs, &w, &h, NULL);
    pixd = pixCreateNoInit(w, h, 1);
    if (regionflag == L_INCLUDE_REGION)
        pixClearAll(pixd);
    else  /* L_EXCLUDE_REGION */
        pixSetAll(pixd);
    datat = pixGetData(pixt);
    datad = pixGetData(pixd);
    wplt = pixGetWpl(pixt);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            pixel = linet[j];
            sval = (pixel >> L_GREEN_SHIFT) & 0xff;
            vval = (pixel >> L_BLUE_SHIFT) & 0xff;
            if (slut[sval] == 1 && vlut[vval] == 1) {
                if (regionflag == L_INCLUDE_REGION)
                    SET_DATA_BIT(lined, j);
                else  /* L_EXCLUDE_REGION */
                    CLEAR_DATA_BIT(lined, j);
            }
        }
    }

    FREE(slut);
    FREE(vlut);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixMakeHistoHS()
 *
 *      Input:  pixs  (HSV colorspace)
 *              factor (subsampling factor; integer)
 *              &nahue (<optional return> hue histogram)
 *              &nasat (<optional return> saturation histogram)
 *      Return: pixd (32 bpp histogram in hue and saturation), or null on error
 *
 *  Notes:
 *      (1) pixs is a 32 bpp image in HSV colorspace; hue is in the "red"
 *          byte, saturation is in the "green" byte.
 *      (2) In pixd, hue is displayed vertically; saturation horizontally.
 *          The dimensions of pixd are w = 256, h = 240, and the depth
 *          is 32 bpp.  The value at each point is simply the number
 *          of pixels found at that value of hue and saturation.
 */
PIX *
pixMakeHistoHS(PIX     *pixs,
               l_int32  factor,
               NUMA   **pnahue,
               NUMA   **pnasat)
{
l_int32    i, j, w, h, wplt, hval, sval, nd;
l_uint32   pixel;
l_uint32  *datat, *linet;
void     **lined32;
NUMA      *nahue, *nasat;
PIX       *pixt, *pixd;

    PROCNAME("pixMakeHistoHS");

    if (pnahue) *pnahue = NULL;
    if (pnasat) *pnasat = NULL;
    if (!pixs || pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);

    if (pnahue) {
        nahue = numaCreate(240);
        numaSetCount(nahue, 240);
        *pnahue = nahue;
    }
    if (pnasat) {
        nasat = numaCreate(256);
        numaSetCount(nasat, 256);
        *pnasat = nasat;
    }

    if (factor <= 1)
        pixt = pixClone(pixs);
    else
        pixt = pixScaleBySampling(pixs, 1.0 / (l_float32)factor,
                                  1.0 / (l_float32)factor);

        /* Create the hue-saturation histogram */
    pixd = pixCreate(256, 240, 32);
    lined32 = pixGetLinePtrs(pixd, NULL);
    pixGetDimensions(pixt, &w, &h, NULL);
    datat = pixGetData(pixt);
    wplt = pixGetWpl(pixt);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        for (j = 0; j < w; j++) {
            pixel = linet[j];
            hval = (pixel >> L_RED_SHIFT) & 0xff;

#if  DEBUG_HISTO
            if (hval > 239) {
                fprintf(stderr, "hval = %d for (%d,%d)\n", hval, i, j);
                continue;
            }
#endif  /* DEBUG_HISTO */

            sval = (pixel >> L_GREEN_SHIFT) & 0xff;
            if (pnahue)
                numaShiftValue(nahue, hval, 1.0);
            if (pnasat)
                numaShiftValue(nasat, sval, 1.0);
            nd = GET_DATA_FOUR_BYTES(lined32[hval], sval);
            SET_DATA_FOUR_BYTES(lined32[hval], sval, nd + 1);
        }
    }

    FREE(lined32);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixMakeHistoHV()
 *
 *      Input:  pixs  (HSV colorspace)
 *              factor (subsampling factor; integer)
 *              &nahue (<optional return> hue histogram)
 *              &naval (<optional return> max intensity (value) histogram)
 *      Return: pixd (32 bpp histogram in hue and value), or null on error
 *
 *  Notes:
 *      (1) pixs is a 32 bpp image in HSV colorspace; hue is in the "red"
 *          byte, max intensity ("value") is in the "blue" byte.
 *      (2) In pixd, hue is displayed vertically; intensity horizontally.
 *          The dimensions of pixd are w = 256, h = 240, and the depth
 *          is 32 bpp.  The value at each point is simply the number
 *          of pixels found at that value of hue and intensity.
 */
PIX *
pixMakeHistoHV(PIX     *pixs,
               l_int32  factor,
               NUMA   **pnahue,
               NUMA   **pnaval)
{
l_int32    i, j, w, h, wplt, hval, vval, nd;
l_uint32   pixel;
l_uint32  *datat, *linet;
void     **lined32;
NUMA      *nahue, *naval;
PIX       *pixt, *pixd;

    PROCNAME("pixMakeHistoHV");

    if (pnahue) *pnahue = NULL;
    if (pnaval) *pnaval = NULL;
    if (!pixs || pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);

    if (pnahue) {
        nahue = numaCreate(240);
        numaSetCount(nahue, 240);
        *pnahue = nahue;
    }
    if (pnaval) {
        naval = numaCreate(256);
        numaSetCount(naval, 256);
        *pnaval = naval;
    }

    if (factor <= 1)
        pixt = pixClone(pixs);
    else
        pixt = pixScaleBySampling(pixs, 1.0 / (l_float32)factor,
                                  1.0 / (l_float32)factor);

        /* Create the hue-value histogram */
    pixd = pixCreate(256, 240, 32);
    lined32 = pixGetLinePtrs(pixd, NULL);
    pixGetDimensions(pixt, &w, &h, NULL);
    datat = pixGetData(pixt);
    wplt = pixGetWpl(pixt);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        for (j = 0; j < w; j++) {
            pixel = linet[j];
            hval = (pixel >> L_RED_SHIFT) & 0xff;
            vval = (pixel >> L_BLUE_SHIFT) & 0xff;
            if (pnahue)
                numaShiftValue(nahue, hval, 1.0);
            if (pnaval)
                numaShiftValue(naval, vval, 1.0);
            nd = GET_DATA_FOUR_BYTES(lined32[hval], vval);
            SET_DATA_FOUR_BYTES(lined32[hval], vval, nd + 1);
        }
    }

    FREE(lined32);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixMakeHistoSV()
 *
 *      Input:  pixs  (HSV colorspace)
 *              factor (subsampling factor; integer)
 *              &nasat (<optional return> sat histogram)
 *              &naval (<optional return> max intensity (value) histogram)
 *      Return: pixd (32 bpp histogram in sat and value), or null on error
 *
 *  Notes:
 *      (1) pixs is a 32 bpp image in HSV colorspace; sat is in the "green"
 *          byte, max intensity ("value") is in the "blue" byte.
 *      (2) In pixd, sat is displayed vertically; intensity horizontally.
 *          The dimensions of pixd are w = 256, h = 256, and the depth
 *          is 32 bpp.  The value at each point is simply the number
 *          of pixels found at that value of saturation and intensity.
 */
PIX *
pixMakeHistoSV(PIX     *pixs,
               l_int32  factor,
               NUMA   **pnasat,
               NUMA   **pnaval)
{
l_int32    i, j, w, h, wplt, sval, vval, nd;
l_uint32   pixel;
l_uint32  *datat, *linet;
void     **lined32;
NUMA      *nasat, *naval;
PIX       *pixt, *pixd;

    PROCNAME("pixMakeHistoSV");

    if (pnasat) *pnasat = NULL;
    if (pnaval) *pnaval = NULL;
    if (!pixs || pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);

    if (pnasat) {
        nasat = numaCreate(256);
        numaSetCount(nasat, 256);
        *pnasat = nasat;
    }
    if (pnaval) {
        naval = numaCreate(256);
        numaSetCount(naval, 256);
        *pnaval = naval;
    }

    if (factor <= 1)
        pixt = pixClone(pixs);
    else
        pixt = pixScaleBySampling(pixs, 1.0 / (l_float32)factor,
                                  1.0 / (l_float32)factor);

        /* Create the hue-value histogram */
    pixd = pixCreate(256, 256, 32);
    lined32 = pixGetLinePtrs(pixd, NULL);
    pixGetDimensions(pixt, &w, &h, NULL);
    datat = pixGetData(pixt);
    wplt = pixGetWpl(pixt);
    for (i = 0; i < h; i++) {
        linet = datat + i * wplt;
        for (j = 0; j < w; j++) {
            pixel = linet[j];
            sval = (pixel >> L_GREEN_SHIFT) & 0xff;
            vval = (pixel >> L_BLUE_SHIFT) & 0xff;
            if (pnasat)
                numaShiftValue(nasat, sval, 1.0);
            if (pnaval)
                numaShiftValue(naval, vval, 1.0);
            nd = GET_DATA_FOUR_BYTES(lined32[sval], vval);
            SET_DATA_FOUR_BYTES(lined32[sval], vval, nd + 1);
        }
    }

    FREE(lined32);
    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixFindHistoPeaksHSV()
 *
 *      Input:  pixs (32 bpp; HS, HV or SV histogram; not changed)
 *              type (L_HS_HISTO, L_HV_HISTO or L_SV_HISTO)
 *              width (half width of sliding window)
 *              height (half height of sliding window)
 *              npeaks (number of peaks to look for)
 *              erasefactor (ratio of erase window size to sliding window size)
 *              &pta (locations of maximum for each integrated peak area)
 *              &natot (integrated peak areas)
 *              &pixa (<optional return> pixa for debugging; NULL to skip)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) pixs is a 32 bpp histogram in a pair of HSV colorspace.  It
 *          should be thought of as a single sample with 32 bps (bits/sample).
 *      (2) After each peak is found, the peak is erased with a window
 *          that is centered on the peak and scaled from the sliding
 *          window by @erasefactor.  Typically, @erasefactor is chosen
 *          to be > 1.0.
 *      (3) Data for a maximum of @npeaks is returned in @pta and @natot.
 */
l_int32
pixFindHistoPeaksHSV(PIX       *pixs,
                     l_int32    type,
                     l_int32    width,
                     l_int32    height,
                     l_int32    npeaks,
                     l_float32  erasefactor,
                     PTA      **ppta,
                     NUMA     **pnatot,
                     PIXA     **ppixa)
{
l_int32   i, xmax, ymax, ewidth, eheight;
l_uint32  maxval;
BOX      *box;
NUMA     *natot;
PIX      *pixh, *pixw, *pixt1, *pixt2;
PTA      *pta;

    PROCNAME("pixFindHistoPeaksHSV");

    if (!pixs || pixGetDepth(pixs) != 32)
        return ERROR_INT("pixs undefined or not 32 bpp", procName, 1);
    if (!ppta || !pnatot)
        return ERROR_INT("&pta and &natot not both defined", procName, 1);
    if (type != L_HS_HISTO && type != L_HV_HISTO && type != L_SV_HISTO)
        return ERROR_INT("invalid HSV histo type", procName, 1);

    if ((pta = ptaCreate(npeaks)) == NULL)
        return ERROR_INT("pta not made", procName, 1);
    *ppta = pta;
    if ((natot = numaCreate(npeaks)) == NULL)
        return ERROR_INT("natot not made", procName, 1);
    *pnatot = natot;

    *ppta = pta;
    if (type == L_SV_HISTO)
        pixh = pixAddMirroredBorder(pixs, width + 1, width + 1, height + 1,
                                    height + 1);
    else  /* type == L_HS_HISTO or type == L_HV_HISTO */
        pixh = pixAddMixedBorder(pixs, width + 1, width + 1, height + 1,
                                 height + 1);

        /* Get the total count in the sliding window.  If the window
         * fully covers the peak, this will be the integrated
         * volume under the peak. */
    pixw = pixWindowedMean(pixh, width, height, 0);
    pixDestroy(&pixh);

        /* Sequentially identify and erase peaks in the histogram.
         * If requested for debugging, save a pixa of the sequence of
         * false color histograms. */
    if (ppixa)
        *ppixa = pixaCreate(0);
    for (i = 0; i < npeaks; i++) {
        pixGetMaxValueInRect(pixw, NULL, &maxval, &xmax, &ymax);
        if (maxval == 0) break;
        numaAddNumber(natot, maxval);
        ptaAddPt(pta, xmax, ymax);
        ewidth = (l_int32)(width * erasefactor);
        eheight = (l_int32)(height * erasefactor);
        box = boxCreate(xmax - ewidth, ymax - eheight, 2 * ewidth + 1,
                        2 * eheight + 1);

        if (ppixa) {
            pixt1 = pixMaxDynamicRange(pixw, L_LINEAR_SCALE);
            pixt2 = pixConvertGrayToFalseColor(pixt1, 1.0);
            pixaAddPix(*ppixa, pixt2, L_INSERT);
            pixDestroy(&pixt1);
        }

        pixClearInRect(pixw, box);
        boxDestroy(&box);
        if (type == L_HS_HISTO || type == L_HV_HISTO) {
                /* clear wraps at bottom and top */
            if (ymax - eheight < 0) {  /* overlap to bottom */
                box = boxCreate(xmax - ewidth, 240 + ymax - eheight,
                                2 * ewidth + 1, eheight - ymax);
            }
            else if (ymax + eheight > 239) {  /* overlap to top */
                box = boxCreate(xmax - ewidth, 0, 2 * ewidth + 1,
                                ymax + eheight - 239);
            }
            else
                box = NULL;
            if (box) {
                pixClearInRect(pixw, box);
                boxDestroy(&box);
            }
        }
    }

    pixDestroy(&pixw);
    return 0;
}


/*!
 *  displayHSVColorRange()
 *
 *      Input:  hval (hue center value; in range [0 ... 240]
 *              sval (saturation center value; in range [0 ... 255]
 *              vval (max intensity value; in range [0 ... 255]
 *              huehw (half-width of hue range; > 0)
 *              sathw (half-width of saturation range; > 0)
 *              nsamp (number of samplings in each half-width in hue and sat)
 *              factor (linear size of each color square, in pixels; > 3)
 *      Return: pixd (32 bpp set of color squares over input range),
 *                     or null on error
 *
 *  Notes:
 *      (1) The total number of color samplings in each of the hue
 *          and saturation directions is 2 * nsamp + 1.
 */
PIX *
displayHSVColorRange(l_int32  hval,
                     l_int32  sval,
                     l_int32  vval,
                     l_int32  huehw,
                     l_int32  sathw,
                     l_int32  nsamp,
                     l_int32  factor)
{
l_int32  i, j, w, huedelta, satdelta, hue, sat, rval, gval, bval;
PIX     *pixt, *pixd;

    PROCNAME("displayHSVColorRange");

    if (hval < 0 || hval > 240)
        return (PIX *)ERROR_PTR("invalid hval", procName, NULL);
    if (huehw < 5 || huehw > 120)
        return (PIX *)ERROR_PTR("invalid huehw", procName, NULL);
    if (sval - sathw < 0 || sval + sathw > 255)
        return (PIX *)ERROR_PTR("invalid sval/sathw", procName, NULL);
    if (nsamp < 1 || factor < 3)
        return (PIX *)ERROR_PTR("invalid nsamp or rep. factor", procName, NULL);
    if (vval < 0 || vval > 255)
        return (PIX *)ERROR_PTR("invalid vval", procName, NULL);

    w = (2 * nsamp + 1);
    huedelta = (l_int32)((l_float32)huehw / (l_float32)nsamp);
    satdelta = (l_int32)((l_float32)sathw / (l_float32)nsamp);
    pixt = pixCreate(w, w, 32);
    for (i = 0; i < w; i++) {
        hue = hval + huedelta * (i - nsamp);
        if (hue < 0) hue += 240;
        if (hue >= 240) hue -= 240;
        for (j = 0; j < w; j++) {
            sat = sval + satdelta * (j - nsamp);
            convertHSVToRGB(hue, sat, vval, &rval, &gval, &bval);
            pixSetRGBPixel(pixt, j, i, rval, gval, bval);
        }
    }

    pixd = pixExpandReplicate(pixt, factor);
    pixDestroy(&pixt);
    return pixd;
}


