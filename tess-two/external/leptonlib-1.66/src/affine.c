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
 *  affine.c
 *
 *      Affine (3 pt) image transformation using a sampled
 *      (to nearest integer) transform on each dest point
 *           PIX        *pixAffineSampledPta()
 *           PIX        *pixAffineSampled()
 *
 *      Affine (3 pt) image transformation using interpolation 
 *      (or area mapping) for anti-aliasing images that are
 *      2, 4, or 8 bpp gray, or colormapped, or 32 bpp RGB
 *           PIX        *pixAffinePta()
 *           PIX        *pixAffine()
 *           PIX        *pixAffinePtaColor()
 *           PIX        *pixAffineColor()
 *           PIX        *pixAffinePtaGray()
 *           PIX        *pixAffineGray()
 *
 *      Affine transform including alpha (blend) component and gamma transform
 *           PIX        *pixAffinePtaWithAlpha()
 *           PIX        *pixAffinePtaGammaXform()
 *
 *      Affine coordinate transformation
 *           l_int32     getAffineXformCoeffs()
 *           l_int32     affineInvertXform()
 *           l_int32     affineXformSampledPt()
 *           l_int32     affineXformPt()
 *
 *      Interpolation helper functions
 *           l_int32     linearInterpolatePixelGray()
 *           l_int32     linearInterpolatePixelColor()
 *
 *      Gauss-jordan linear equation solver
 *           l_int32     gaussjordan()
 *
 *      Affine image transformation using a sequence of 
 *      shear/scale/translation operations
 *           PIX        *pixAffineSequential()
 *
 *      One can define a coordinate space by the location of the origin,
 *      the orientation of x and y axes, and the unit scaling along
 *      each axis.  An affine transform is a general linear
 *      transformation from one coordinate space to another.
 *
 *      For the general case, we can define the affine transform using
 *      two sets of three (noncollinear) points in a plane.  One set
 *      corresponds to the input (src) coordinate space; the other to the 
 *      transformed (dest) coordinate space.  Each point in the
 *      src corresponds to one of the points in the dest.  With two
 *      sets of three points, we get a set of 6 equations in 6 unknowns
 *      that specifies the mapping between the coordinate spaces.
 *      The interface here allows you to specify either the corresponding
 *      sets of 3 points, or the transform itself (as a vector of 6
 *      coefficients).
 *
 *      Given the transform as a vector of 6 coefficients, we can compute
 *      both a a pointwise affine coordinate transformation and an
 *      affine image transformation.
 *
 *      To compute the coordinate transform, we need the coordinate
 *      value (x',y') in the transformed space for any point (x,y)
 *      in the original space.  To derive this transform from the
 *      three corresponding points, it is convenient to express the affine
 *      coordinate transformation using an LU decomposition of
 *      a set of six linear equations that express the six coordinates
 *      of the three points in the transformed space as a function of
 *      the six coordinates in the original space.  Once we have
 *      this transform matrix , we can transform an image by
 *      finding, for each destination pixel, the pixel (or pixels)
 *      in the source that give rise to it.
 *
 *      This 'pointwise' transformation can be done either by sampling
 *      and picking a single pixel in the src to replicate into the dest,
 *      or by interpolating (or averaging) over four src pixels to
 *      determine the value of the dest pixel.  The first method is
 *      implemented by pixAffineSampled() and the second method by
 *      pixAffine().  The interpolated method can only be used for
 *      images with more than 1 bpp, but for these, the image quality
 *      is significantly better than the sampled method, due to
 *      the 'antialiasing' effect of weighting the src pixels.
 *
 *      Interpolation works well when there is relatively little scaling,
 *      or if there is image expansion in general.  However, if there
 *      is significant image reduction, one should apply a low-pass
 *      filter before subsampling to avoid aliasing the high frequencies.
 *
 *      A typical application might be to align two images, which
 *      may be scaled, rotated and translated versions of each other.
 *      Through some pre-processing, three corresponding points are
 *      located in each of the two images.  One of the images is
 *      then to be (affine) transformed to align with the other.
 *      As mentioned, the standard way to do this is to use three
 *      sets of points, compute the 6 transformation coefficients
 *      from these points that describe the linear transformation,
 *
 *          x' = ax + by + c
 *          y' = dx + ey + f
 *
 *      and use this in a pointwise manner to transform the image.
 *
 *      N.B.  Be sure to see the comment in getAffineXformCoeffs(),
 *      regarding using the inverse of the affine transform for points
 *      to transform images.
 *
 *      There is another way to do this transformation; namely,
 *      by doing a sequence of simple affine transforms, without
 *      computing directly the affine coordinate transformation.
 *      We have at our disposal (1) translations (using rasterop),
 *      (2) horizontal and vertical shear about any horizontal and vertical
 *      line, respectively, and (3) non-isotropic scaling by two
 *      arbitrary x and y scaling factors.  We also have rotation
 *      about an arbitrary point, but this is equivalent to a set 
 *      of three shears so we do not need to use it.
 *
 *      Why might we do this?  For binary images, it is usually
 *      more efficient to do such transformations by a sequence
 *      of word parallel operations.  Shear and translation can be
 *      done in-place and word parallel; arbitrary scaling is
 *      mostly pixel-wise.
 *
 *      Suppose that we are tranforming image 1 to correspond to image 2.
 *      We have a set of three points, describing the coordinate space
 *      embedded in image 1, and we need to transform image 1 until
 *      those three points exactly correspond to the new coordinate space
 *      defined by the second set of three points.  In our image
 *      matching application, the latter set of three points was
 *      found to be the corresponding points in image 2.
 *
 *      The most elegant way I can think of to do such a sequential
 *      implementation is to imagine that we're going to transform
 *      BOTH images until they're aligned.  (We don't really want
 *      to transform both, because in fact we may only have one image
 *      that is undergoing a general affine transformation.)
 *
 *      Choose the 3 corresponding points as follows:
 *         - The 1st point is an origin
 *         - The 2nd point gives the orientation and scaling of the
 *           "x" axis with respect to the origin
 *         - The 3rd point does likewise for the "y" axis.
 *      These "axes" must not be collinear; otherwise they are
 *      arbitrary (although some strange things will happen if
 *      the handedness sweeping through the minimum angle between
 *      the axes is opposite).
 *
 *      An important constraint is that we have shear operations
 *      about an arbitrary horizontal or vertical line, but always
 *      parallel to the x or y axis.  If we continue to pretend that
 *      we have an unprimed coordinate space embedded in image 1 and
 *      a primed coordinate space embedded in image 2, we imagine
 *      (a) transforming image 1 by horizontal and vertical shears about
 *      point 1 to align points 3 and 2 along the y and x axes,
 *      respectively, and (b) transforming image 2 by horizontal and
 *      vertical shears about point 1' to align points 3' and 2' along
 *      the y and x axes.  Then we scale image 1 so that the distances
 *      from 1 to 2 and from 1 to 3 are equal to the distances in
 *      image 2 from 1' to 2' and from 1' to 3'.  This scaling operation
 *      leaves the true image origin, at (0,0) invariant, and will in
 *      general translate point 1.  The original points 1 and 1' will
 *      typically not coincide in any event, so we must translate
 *      the origin of image 1, at its current point 1, to the origin
 *      of image 2 at 1'.  The images should now be aligned.  But
 *      because we never really transformed image 2 (and image 2 may
 *      not even exist), we now perform  on image 1 the reverse of
 *      the shear transforms that we imagined doing on image 2;
 *      namely, the negative vertical shear followed by the negative
 *      horizontal shear.  Image 1 should now have its transformed
 *      unprimed coordinates aligned with the original primed
 *      coordinates.  In all this, it is only necessary to keep track
 *      of the shear angles and translations of points during the shears.
 *      What has been accomplished is a general affine transformation
 *      on image 1.
 *
 *      Having described all this, if you are going to use an
 *      affine transformation in an application, this is what you
 *      need to know:
 *
 *          (1) You should NEVER use the sequential method, because
 *              the image quality for 1 bpp text is much poorer
 *              (even though it is about 2x faster than the pointwise sampled
 *              method), and for images with depth greater than 1, it is
 *              nearly 20x slower than the pointwise sampled method
 *              and over 10x slower than the pointwise interpolated method!
 *              The sequential method is given here for purely
 *              pedagogical reasons.
 *
 *          (2) For 1 bpp images, use the pointwise sampled function
 *              pixAffineSampled().  For all other images, the best
 *              quality results result from using the pointwise
 *              interpolated function pixAffinePta() or pixAffine();
 *              the cost is less than a doubling of the computation time
 *              with respect to the sampled function.  If you use 
 *              interpolation on colormapped images, the colormap will
 *              be removed, resulting in either a grayscale or color
 *              image, depending on the values in the colormap.
 *              If you want to retain the colormap, use pixAffineSampled().
 *
 *      Typical relative timing of pointwise transforms (sampled = 1.0):
 *      8 bpp:   sampled        1.0
 *               interpolated   1.6
 *      32 bpp:  sampled        1.0
 *               interpolated   1.8
 *      Additionally, the computation time/pixel is nearly the same
 *      for 8 bpp and 32 bpp, for both sampled and interpolated.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "allheaders.h"

extern l_float32  AlphaMaskBorderVals[2];

#ifndef  NO_CONSOLE_IO
#define  DEBUG     0
#endif  /* ~NO_CONSOLE_IO */


/*-------------------------------------------------------------*
 *               Sampled affine image transformation           *
 *-------------------------------------------------------------*/
/*!
 *  pixAffineSampledPta()
 *
 *      Input:  pixs (all depths)
 *              ptad  (3 pts of final coordinate space)
 *              ptas  (3 pts of initial coordinate space)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Brings in either black or white pixels from the boundary.
 *      (2) Retains colormap, which you can do for a sampled transform..
 *      (3) The 3 points must not be collinear.
 *      (4) The order of the 3 points is arbitrary; however, to compare
 *          with the sequential transform they must be in these locations
 *          and in this order: origin, x-axis, y-axis.
 *      (5) For 1 bpp images, this has much better quality results
 *          than pixAffineSequential(), particularly for text.
 *          It is about 3x slower, but does not require additional
 *          border pixels.  The poor quality of pixAffineSequential()
 *          is due to repeated quantized transforms.  It is strongly
 *          recommended that pixAffineSampled() be used for 1 bpp images.
 *      (6) For 8 or 32 bpp, much better quality is obtained by the
 *          somewhat slower pixAffinePta().  See that function
 *          for relative timings between sampled and interpolated.
 *      (7) To repeat, use of the sequential transform,
 *          pixAffineSequential(), for any images, is discouraged.
 */
PIX *
pixAffineSampledPta(PIX     *pixs,
                    PTA     *ptad,
                    PTA     *ptas,
                    l_int32  incolor)
{
l_float32  *vc;
PIX        *pixd;

    PROCNAME("pixAffineSampledPta");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!ptas)
        return (PIX *)ERROR_PTR("ptas not defined", procName, NULL);
    if (!ptad)
        return (PIX *)ERROR_PTR("ptad not defined", procName, NULL);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return (PIX *)ERROR_PTR("invalid incolor", procName, NULL);
    if (ptaGetCount(ptas) != 3)
        return (PIX *)ERROR_PTR("ptas count not 3", procName, NULL);
    if (ptaGetCount(ptad) != 3)
        return (PIX *)ERROR_PTR("ptad count not 3", procName, NULL);

        /* Get backwards transform from dest to src, and apply it */
    getAffineXformCoeffs(ptad, ptas, &vc);
    pixd = pixAffineSampled(pixs, vc, incolor);
    FREE(vc);

    return pixd;
}


/*!
 *  pixAffineSampled()
 *
 *      Input:  pixs (all depths)
 *              vc  (vector of 6 coefficients for affine transformation)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Brings in either black or white pixels from the boundary.
 *      (2) Retains colormap, which you can do for a sampled transform..
 *      (3) For 8 or 32 bpp, much better quality is obtained by the
 *          somewhat slower pixAffine().  See that function
 *          for relative timings between sampled and interpolated.
 */
PIX *
pixAffineSampled(PIX        *pixs,
                 l_float32  *vc,
                 l_int32     incolor)
{
l_int32     i, j, w, h, d, x, y, wpls, wpld, color, cmapindex;
l_uint32    val;
l_uint32   *datas, *datad, *lines, *lined;
PIX        *pixd;
PIXCMAP    *cmap;

    PROCNAME("pixAffineSampled");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!vc)
        return (PIX *)ERROR_PTR("vc not defined", procName, NULL);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return (PIX *)ERROR_PTR("invalid incolor", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 32)
        return (PIX *)ERROR_PTR("depth not 1, 2, 4, 8 or 16", procName, NULL);

        /* Init all dest pixels to color to be brought in from outside */
    pixd = pixCreateTemplate(pixs);
    if ((cmap = pixGetColormap(pixs)) != NULL) {
        if (incolor == L_BRING_IN_WHITE)
            color = 1;
        else
            color = 0;
        pixcmapAddBlackOrWhite(cmap, color, &cmapindex);
        pixSetAllArbitrary(pixd, cmapindex);
    }
    else {
        if ((d == 1 && incolor == L_BRING_IN_WHITE) ||
            (d > 1 && incolor == L_BRING_IN_BLACK))
            pixClearAll(pixd);
        else
            pixSetAll(pixd);
    }

        /* Scan over the dest pixels */
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < h; i++) {
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            affineXformSampledPt(vc, j, i, &x, &y);
            if (x < 0 || y < 0 || x >=w || y >= h)
                continue;
            lines = datas + y * wpls;
            if (d == 1) {
                val = GET_DATA_BIT(lines, x);
                SET_DATA_BIT_VAL(lined, j, val);
            }
            else if (d == 8) {
                val = GET_DATA_BYTE(lines, x);
                SET_DATA_BYTE(lined, j, val);
            }
            else if (d == 32) {
                lined[j] = lines[x];
            }
            else if (d == 2) {
                val = GET_DATA_DIBIT(lines, x);
                SET_DATA_DIBIT(lined, j, val);
            }
            else if (d == 4) {
                val = GET_DATA_QBIT(lines, x);
                SET_DATA_QBIT(lined, j, val);
            }
        }
    }

    return pixd;
}


/*---------------------------------------------------------------------*
 *               Interpolated affine image transformation              *
 *---------------------------------------------------------------------*/
/*!
 *  pixAffinePta()
 *
 *      Input:  pixs (all depths; colormap ok)
 *              ptad  (3 pts of final coordinate space)
 *              ptas  (3 pts of initial coordinate space)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Brings in either black or white pixels from the boundary
 *      (2) Removes any existing colormap, if necessary, before transforming
 */
PIX *
pixAffinePta(PIX     *pixs,
             PTA     *ptad,
             PTA     *ptas,
             l_int32  incolor)
{
l_int32   d;
l_uint32  colorval;
PIX      *pixt1, *pixt2, *pixd;

    PROCNAME("pixAffinePta");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!ptas)
        return (PIX *)ERROR_PTR("ptas not defined", procName, NULL);
    if (!ptad)
        return (PIX *)ERROR_PTR("ptad not defined", procName, NULL);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return (PIX *)ERROR_PTR("invalid incolor", procName, NULL);
    if (ptaGetCount(ptas) != 3)
        return (PIX *)ERROR_PTR("ptas count not 3", procName, NULL);
    if (ptaGetCount(ptad) != 3)
        return (PIX *)ERROR_PTR("ptad count not 3", procName, NULL);

    if (pixGetDepth(pixs) == 1)
        return pixAffineSampledPta(pixs, ptad, ptas, incolor);

        /* Remove cmap if it exists, and unpack to 8 bpp if necessary */
    pixt1 = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    d = pixGetDepth(pixt1);
    if (d < 8)
        pixt2 = pixConvertTo8(pixt1, FALSE);
    else
        pixt2 = pixClone(pixt1);
    d = pixGetDepth(pixt2);

        /* Compute actual color to bring in from edges */
    colorval = 0;
    if (incolor == L_BRING_IN_WHITE) {
        if (d == 8)
            colorval = 255;
        else  /* d == 32 */
            colorval = 0xffffff00;
    }
    
    if (d == 8)
        pixd = pixAffinePtaGray(pixt2, ptad, ptas, colorval);
    else  /* d == 32 */
        pixd = pixAffinePtaColor(pixt2, ptad, ptas, colorval);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    return pixd;
}


/*!
 *  pixAffine()
 *
 *      Input:  pixs (all depths; colormap ok)
 *              vc  (vector of 6 coefficients for affine transformation)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Brings in either black or white pixels from the boundary
 *      (2) Removes any existing colormap, if necessary, before transforming
 */
PIX *
pixAffine(PIX        *pixs,
          l_float32  *vc,
          l_int32     incolor)
{
l_int32   d;
l_uint32  colorval;
PIX      *pixt1, *pixt2, *pixd;

    PROCNAME("pixAffine");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!vc)
        return (PIX *)ERROR_PTR("vc not defined", procName, NULL);

    if (pixGetDepth(pixs) == 1)
        return pixAffineSampled(pixs, vc, incolor);

        /* Remove cmap if it exists, and unpack to 8 bpp if necessary */
    pixt1 = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    d = pixGetDepth(pixt1);
    if (d < 8)
        pixt2 = pixConvertTo8(pixt1, FALSE);
    else
        pixt2 = pixClone(pixt1);
    d = pixGetDepth(pixt2);

        /* Compute actual color to bring in from edges */
    colorval = 0;
    if (incolor == L_BRING_IN_WHITE) {
        if (d == 8)
            colorval = 255;
        else  /* d == 32 */
            colorval = 0xffffff00;
    }
    
    if (d == 8)
        pixd = pixAffineGray(pixt2, vc, colorval);
    else  /* d == 32 */
        pixd = pixAffineColor(pixt2, vc, colorval);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    return pixd;
}


/*!
 *  pixAffinePtaColor()
 *
 *      Input:  pixs (32 bpp)
 *              ptad  (3 pts of final coordinate space)
 *              ptas  (3 pts of initial coordinate space)
 *              colorval (e.g., 0 to bring in BLACK, 0xffffff00 for WHITE)
 *      Return: pixd, or null on error
 */
PIX *
pixAffinePtaColor(PIX      *pixs,
                  PTA      *ptad,
                  PTA      *ptas,
                  l_uint32  colorval)
{
l_float32  *vc;
PIX        *pixd;

    PROCNAME("pixAffinePtaColor");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!ptas)
        return (PIX *)ERROR_PTR("ptas not defined", procName, NULL);
    if (!ptad)
        return (PIX *)ERROR_PTR("ptad not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs must be 32 bpp", procName, NULL);
    if (ptaGetCount(ptas) != 3)
        return (PIX *)ERROR_PTR("ptas count not 3", procName, NULL);
    if (ptaGetCount(ptad) != 3)
        return (PIX *)ERROR_PTR("ptad count not 3", procName, NULL);

        /* Get backwards transform from dest to src, and apply it */
    getAffineXformCoeffs(ptad, ptas, &vc);
    pixd = pixAffineColor(pixs, vc, colorval);
    FREE(vc);

    return pixd;
}


/*!
 *  pixAffineColor()
 *
 *      Input:  pixs (32 bpp)
 *              vc  (vector of 6 coefficients for affine transformation)
 *              colorval (e.g., 0 to bring in BLACK, 0xffffff00 for WHITE)
 *      Return: pixd, or null on error
 */
PIX *
pixAffineColor(PIX        *pixs,
               l_float32  *vc,
               l_uint32    colorval)
{
l_int32    i, j, w, h, d, wpls, wpld;
l_uint32   val;
l_uint32  *datas, *datad, *lined;
l_float32  x, y;
PIX       *pixd;

    PROCNAME("pixAffineColor");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 32)
        return (PIX *)ERROR_PTR("pixs must be 32 bpp", procName, NULL);
    if (!vc)
        return (PIX *)ERROR_PTR("vc not defined", procName, NULL);

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    pixd = pixCreateTemplate(pixs);
    pixSetAllArbitrary(pixd, colorval);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

        /* Iterate over destination pixels */
    for (i = 0; i < h; i++) {
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
                /* Compute float src pixel location corresponding to (i,j) */
            affineXformPt(vc, j, i, &x, &y);
            linearInterpolatePixelColor(datas, wpls, w, h, x, y, colorval,
                                        &val);
            *(lined + j) = val;
        }
    }

    return pixd;
}


/*!
 *  pixAffinePtaGray()
 *
 *      Input:  pixs (8 bpp)
 *              ptad  (3 pts of final coordinate space)
 *              ptas  (3 pts of initial coordinate space)
 *              grayval (0 to bring in BLACK, 255 for WHITE)
 *      Return: pixd, or null on error
 */
PIX *
pixAffinePtaGray(PIX     *pixs,
                 PTA     *ptad,
                 PTA     *ptas,
                 l_uint8  grayval)
{
l_float32  *vc;
PIX        *pixd;

    PROCNAME("pixAffinePtaGray");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!ptas)
        return (PIX *)ERROR_PTR("ptas not defined", procName, NULL);
    if (!ptad)
        return (PIX *)ERROR_PTR("ptad not defined", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs must be 8 bpp", procName, NULL);
    if (ptaGetCount(ptas) != 3)
        return (PIX *)ERROR_PTR("ptas count not 3", procName, NULL);
    if (ptaGetCount(ptad) != 3)
        return (PIX *)ERROR_PTR("ptad count not 3", procName, NULL);

        /* Get backwards transform from dest to src, and apply it */
    getAffineXformCoeffs(ptad, ptas, &vc);
    pixd = pixAffineGray(pixs, vc, grayval);
    FREE(vc);

    return pixd;
}



/*!
 *  pixAffineGray()
 *
 *      Input:  pixs (8 bpp)
 *              vc  (vector of 6 coefficients for affine transformation)
 *              grayval (0 to bring in BLACK, 255 for WHITE)
 *      Return: pixd, or null on error
 */
PIX *
pixAffineGray(PIX        *pixs,
              l_float32  *vc,
              l_uint8     grayval)
{
l_int32    i, j, w, h, wpls, wpld, val;
l_uint32  *datas, *datad, *lined;
l_float32  x, y;
PIX       *pixd;

    PROCNAME("pixAffineGray");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("pixs must be 8 bpp", procName, NULL);
    if (!vc)
        return (PIX *)ERROR_PTR("vc not defined", procName, NULL);

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    pixd = pixCreateTemplate(pixs);
    pixSetAllArbitrary(pixd, grayval);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

        /* Iterate over destination pixels */
    for (i = 0; i < h; i++) {
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
                /* Compute float src pixel location corresponding to (i,j) */
            affineXformPt(vc, j, i, &x, &y);
            linearInterpolatePixelGray(datas, wpls, w, h, x, y, grayval, &val);
            SET_DATA_BYTE(lined, j, val);
        }
    }

    return pixd;
}


/*---------------------------------------------------------------------------*
 *   Affine transform including alpha (blend) component and gamma transform  *
 *---------------------------------------------------------------------------*/
/*!
 *  pixAffinePtaWithAlpha()
 *
 *      Input:  pixs (32 bpp rgb)
 *              ptad  (3 pts of final coordinate space)
 *              ptas  (3 pts of initial coordinate space)
 *              pixg (<optional> 8 bpp, can be null)
 *              fract (between 0.0 and 1.0, with 0.0 fully transparent
 *                     and 1.0 fully opaque)
 *              border (of pixels added to capture transformed source pixels)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) The alpha channel is transformed separately from pixs,
 *          and aligns with it, being fully transparent outside the
 *          boundary of the transformed pixs.  For pixels that are fully
 *          transparent, a blending function like pixBlendWithGrayMask()
 *          will give zero weight to corresponding pixels in pixs.
 *      (2) If pixg is NULL, it is generated as an alpha layer that is
 *          partially opaque, using @fract.  Otherwise, it is cropped
 *          to pixs if required and @fract is ignored.  The alpha channel
 *          in pixs is never used.
 *      (3) Colormaps are removed.
 *      (4) When pixs is transformed, it doesn't matter what color is brought
 *          in because the alpha channel will be transparent (0) there.
 *      (5) To avoid losing source pixels in the destination, it may be
 *          necessary to add a border to the source pix before doing
 *          the affine transformation.  This can be any non-negative number.
 *      (6) The input @ptad and @ptas are in a coordinate space before
 *          the border is added.  Internally, we compensate for this
 *          before doing the affine transform on the image after the border
 *          is added.
 *      (7) The default setting for the border values in the alpha channel
 *          is 0 (transparent) for the outermost ring of pixels and
 *          (0.5 * fract * 255) for the second ring.  When blended over
 *          a second image, this
 *          (a) shrinks the visible image to make a clean overlap edge
 *              with an image below, and
 *          (b) softens the edges by weakening the aliasing there.
 *          Use l_setAlphaMaskBorder() to change these values.
 */
PIX *
pixAffinePtaWithAlpha(PIX       *pixs,
                      PTA       *ptad,
                      PTA       *ptas,
                      PIX       *pixg,
                      l_float32  fract,
                      l_int32    border)
{
l_int32  ws, hs, d;
PIX     *pixd, *pixb1, *pixb2, *pixg2, *pixga;
PTA     *ptad2, *ptas2;

    PROCNAME("pixAffinePtaWithAlpha");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &ws, &hs, &d);
    if (d != 32 && pixGetColormap(pixs) == NULL)
        return (PIX *)ERROR_PTR("pixs not cmapped or 32 bpp", procName, NULL);
    if (pixg && pixGetDepth(pixg) != 8) {
        L_WARNING("pixg not 8 bpp; using @fract transparent alpha", procName);
        pixg = NULL;
    }
    if (!pixg && (fract < 0.0 || fract > 1.0)) {
        L_WARNING("invalid fract; using 1.0 (fully transparent)", procName);
        fract = 1.0;
    }
    if (!pixg && fract == 0.0)
        L_WARNING("fully opaque alpha; image will not be blended", procName);
    if (!ptad)
        return (PIX *)ERROR_PTR("ptad not defined", procName, NULL);
    if (!ptas)
        return (PIX *)ERROR_PTR("ptas not defined", procName, NULL);

        /* Add border; the color doesn't matter */
    pixb1 = pixAddBorder(pixs, border, 0);

        /* Transform the ptr arrays to work on the bordered image */
    ptad2 = ptaTransform(ptad, border, border, 1.0, 1.0);
    ptas2 = ptaTransform(ptas, border, border, 1.0, 1.0);

        /* Do separate affine transform of rgb channels of pixs and of pixg */
    pixd = pixAffinePtaColor(pixb1, ptad2, ptas2, 0);
    if (!pixg) {
        pixg2 = pixCreate(ws, hs, 8);
        if (fract == 1.0)
            pixSetAll(pixg2);
        else
            pixSetAllArbitrary(pixg2, (l_int32)(255.0 * fract));
    }
    else
        pixg2 = pixResizeToMatch(pixg, NULL, ws, hs);
    if (ws > 10 && hs > 10) {  /* see note 7 */
        pixSetBorderRingVal(pixg2, 1,
                            (l_int32)(255.0 * fract * AlphaMaskBorderVals[0]));
        pixSetBorderRingVal(pixg2, 2,
                            (l_int32)(255.0 * fract * AlphaMaskBorderVals[1]));

    }
    pixb2 = pixAddBorder(pixg2, border, 0);  /* must be black border */
    pixga = pixAffinePtaGray(pixb2, ptad2, ptas2, 0);
    pixSetRGBComponent(pixd, pixga, L_ALPHA_CHANNEL);

    pixDestroy(&pixg2);
    pixDestroy(&pixb1);
    pixDestroy(&pixb2);
    pixDestroy(&pixga);
    ptaDestroy(&ptad2);
    ptaDestroy(&ptas2);
    return pixd;
}


/*!
 *  pixAffinePtaGammaXform()
 *
 *      Input:  pixs (32 bpp rgb)
 *              gamma (gamma correction; must be > 0.0)
 *              ptad  (3 pts of final coordinate space)
 *              ptas  (3 pts of initial coordinate space)
 *              fract (between 0.0 and 1.0, with 1.0 fully transparent)
 *              border (of pixels to capture transformed source pixels)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This wraps a gamma/inverse-gamma photometric transform around
 *          pixAffinePtaWithAlpha().
 *      (2) For usage, see notes in pixAffinePtaWithAlpha() and
 *          pixGammaTRCWithAlpha().
 *      (3) The basic idea of a gamma/inverse-gamma transform is to remove
 *          any gamma correction before the affine transform, and restore
 *          it afterward.  The effects can be subtle, but important for
 *          some applications.  For example, using gamma > 1.0 will
 *          cause the dark areas to become somewhat lighter and slightly
 *          reduce aliasing effects when blending using the alpha channel.
 */
PIX *
pixAffinePtaGammaXform(PIX       *pixs,
                       l_float32  gamma,
                       PTA       *ptad,
                       PTA       *ptas,
                       l_float32  fract,
                       l_int32    border)
{
PIX  *pixg, *pixd;

    PROCNAME("pixAffinePtaGammaXform");

    if (!pixs || (pixGetDepth(pixs) != 32))
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);
    if (fract == 0.0)
        L_WARNING("fully opaque alpha; image cannot be blended", procName);
    if (gamma <= 0.0)  {
        L_WARNING("gamma must be > 0.0; setting to 1.0", procName);
        gamma = 1.0;
    }

    pixg = pixGammaTRCWithAlpha(NULL, pixs, 1.0 / gamma, 0, 255);
    pixd = pixAffinePtaWithAlpha(pixg, ptad, ptas, NULL, fract, border);
    pixGammaTRCWithAlpha(pixd, pixd, gamma, 0, 255);
    pixDestroy(&pixg);
    return pixd;
}


/*-------------------------------------------------------------*
 *                 Affine coordinate transformation            *
 *-------------------------------------------------------------*/
/*!
 *  getAffineXformCoeffs()
 *
 *      Input:  ptas  (source 3 points; unprimed)
 *              ptad  (transformed 3 points; primed)
 *              &vc   (<return> vector of coefficients of transform)
 *      Return: 0 if OK; 1 on error
 *
 *  We have a set of six equations, describing the affine
 *  transformation that takes 3 points (ptas) into 3 other
 *  points (ptad).  These equations are:
 *
 *          x1' = c[0]*x1 + c[1]*y1 + c[2]
 *          y1' = c[3]*x1 + c[4]*y1 + c[5]
 *          x2' = c[0]*x2 + c[1]*y2 + c[2]
 *          y2' = c[3]*x2 + c[4]*y2 + c[5]
 *          x3' = c[0]*x3 + c[1]*y3 + c[2]
 *          y3' = c[3]*x3 + c[4]*y3 + c[5]
 *    
 *  This can be represented as
 *
 *          AC = B
 *
 *  where B and C are column vectors
 *    
 *          B = [ x1' y1' x2' y2' x3' y3' ]
 *          C = [ c[0] c[1] c[2] c[3] c[4] c[5] c[6] ]
 *    
 *  and A is the 6x6 matrix
 *
 *          x1   y1   1   0    0    0
 *           0    0   0   x1   y1   1
 *          x2   y2   1   0    0    0
 *           0    0   0   x2   y2   1
 *          x3   y3   1   0    0    0
 *           0    0   0   x3   y3   1
 *
 *  These six equations are solved here for the coefficients C.
 *
 *  These six coefficients can then be used to find the dest
 *  point (x',y') corresponding to any src point (x,y), according
 *  to the equations
 *
 *           x' = c[0]x + c[1]y + c[2]
 *           y' = c[3]x + c[4]y + c[5]
 *
 *  that are implemented in affineXformPt().
 *
 *  !!!!!!!!!!!!!!!!!!   Very important   !!!!!!!!!!!!!!!!!!!!!!
 *
 *  When the affine transform is composed from a set of simple
 *  operations such as translation, scaling and rotation,
 *  it is built in a form to convert from the un-transformed src
 *  point to the transformed dest point.  However, when an
 *  affine transform is used on images, it is used in an inverted
 *  way: it converts from the transformed dest point to the
 *  un-transformed src point.  So, for example, if you transform
 *  a boxa using transform A, to transform an image in the same
 *  way you must use the inverse of A.
 *
 *  For example, if you transform a boxa with a 3x3 affine matrix
 *  'mat', the analogous image transformation must use 'matinv':
 *
 *     boxad = boxaAffineTransform(boxas, mat);
 *     affineInvertXform(mat, &matinv);
 *     pixd = pixAffine(pixs, matinv, L_BRING_IN_WHITE);
 *
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */
l_int32
getAffineXformCoeffs(PTA         *ptas,
                     PTA         *ptad,
                     l_float32  **pvc)
{
l_int32     i;
l_float32   x1, y1, x2, y2, x3, y3;
l_float32  *b;   /* rhs vector of primed coords X'; coeffs returned in *pvc */
l_float32  *a[6];  /* 6x6 matrix A  */

    PROCNAME("getAffineXformCoeffs");

    if (!ptas)
        return ERROR_INT("ptas not defined", procName, 1);
    if (!ptad)
        return ERROR_INT("ptad not defined", procName, 1);
    if (!pvc)
        return ERROR_INT("&vc not defined", procName, 1);
        
    if ((b = (l_float32 *)CALLOC(6, sizeof(l_float32))) == NULL)
        return ERROR_INT("b not made", procName, 1);
    *pvc = b;

    ptaGetPt(ptas, 0, &x1, &y1);
    ptaGetPt(ptas, 1, &x2, &y2);
    ptaGetPt(ptas, 2, &x3, &y3);
    ptaGetPt(ptad, 0, &b[0], &b[1]);
    ptaGetPt(ptad, 1, &b[2], &b[3]);
    ptaGetPt(ptad, 2, &b[4], &b[5]);

    for (i = 0; i < 6; i++)
        if ((a[i] = (l_float32 *)CALLOC(6, sizeof(l_float32))) == NULL)
            return ERROR_INT("a[i] not made", procName, 1);

    a[0][0] = x1;
    a[0][1] = y1;
    a[0][2] = 1.;
    a[1][3] = x1;
    a[1][4] = y1;
    a[1][5] = 1.;
    a[2][0] = x2;
    a[2][1] = y2;
    a[2][2] = 1.;
    a[3][3] = x2;
    a[3][4] = y2;
    a[3][5] = 1.;
    a[4][0] = x3;
    a[4][1] = y3;
    a[4][2] = 1.;
    a[5][3] = x3;
    a[5][4] = y3;
    a[5][5] = 1.;

    gaussjordan(a, b, 6);

    for (i = 0; i < 6; i++)
        FREE(a[i]);

    return 0;
}


/*!
 *  affineInvertXform()
 *
 *      Input:  vc (vector of 6 coefficients)
 *              *vci (<return> inverted transform)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The 6 affine transform coefficients are the first
 *          two rows of a 3x3 matrix where the last row has
 *          only a 1 in the third column.  We invert this
 *          using gaussjordan(), and select the first 2 rows
 *          as the coefficients of the inverse affine transform.
 *      (2) Alternatively, we can find the inverse transform
 *          coefficients by inverting the 2x2 submatrix,
 *          and treating the top 2 coefficients in the 3rd column as
 *          a RHS vector for that 2x2 submatrix.  Then the
 *          6 inverted transform coefficients are composed of
 *          the inverted 2x2 submatrix and the negative of the
 *          transformed RHS vector.  Why is this so?  We have
 *             Y = AX + R  (2 equations in 6 unknowns)
 *          Then
 *             X = A'Y - A'R
 *          Gauss-jordan solves
 *             AF = R
 *          and puts the solution for F, which is A'R,
 *          into the input R vector.
 *
 */
l_int32
affineInvertXform(l_float32   *vc,
                  l_float32  **pvci)
{
l_int32     i;
l_float32  *vci;
l_float32  *a[3];
l_float32   b[3] = {1.0, 1.0, 1.0};   /* anything; results ignored */

    PROCNAME("affineInvertXform");

    if (!pvci)
        return ERROR_INT("&vci not defined", procName, 1);
    *pvci = NULL;
    if (!vc)
        return ERROR_INT("vc not defined", procName, 1);

    for (i = 0; i < 3; i++)
        a[i] = (l_float32 *)CALLOC(3, sizeof(l_float32));
    a[0][0] = vc[0];
    a[0][1] = vc[1];
    a[0][2] = vc[2];
    a[1][0] = vc[3];
    a[1][1] = vc[4];
    a[1][2] = vc[5];
    a[2][2] = 1.0;
    gaussjordan(a, b, 3);  /* now matrix a contains the inverse */
    vci = (l_float32 *)CALLOC(6, sizeof(l_float32));
    *pvci = vci;
    vci[0] = a[0][0];
    vci[1] = a[0][1];
    vci[2] = a[0][2];
    vci[3] = a[1][0];
    vci[4] = a[1][1];
    vci[5] = a[1][2];

#if 0
        /* Alternative version, inverting a 2x2 matrix */
    for (i = 0; i < 2; i++)
        a[i] = (l_float32 *)CALLOC(2, sizeof(l_float32));
    a[0][0] = vc[0];
    a[0][1] = vc[1];
    a[1][0] = vc[3];
    a[1][1] = vc[4];
    b[0] = vc[2];
    b[1] = vc[5];
    gaussjordan(a, b, 2);  /* now matrix a contains the inverse */
    vci = (l_float32 *)CALLOC(6, sizeof(l_float32));
    *pvci = vci;
    vci[0] = a[0][0];
    vci[1] = a[0][1];
    vci[2] = -b[0];   /* note sign */
    vci[3] = a[1][0];
    vci[4] = a[1][1];
    vci[5] = -b[1];   /* note sign */
#endif

    return 0;
}


/*!
 *  affineXformSampledPt()
 *
 *      Input:  vc (vector of 6 coefficients)
 *              (x, y)  (initial point)
 *              (&xp, &yp)   (<return> transformed point)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This finds the nearest pixel coordinates of the transformed point.
 *      (2) It does not check ptrs for returned data!
 */
l_int32
affineXformSampledPt(l_float32  *vc,
                     l_int32     x,
                     l_int32     y,
                     l_int32    *pxp,
                     l_int32    *pyp)
{
    PROCNAME("affineXformSampledPt");

    if (!vc)
        return ERROR_INT("vc not defined", procName, 1);

    *pxp = (l_int32)(vc[0] * x + vc[1] * y + vc[2] + 0.5);
    *pyp = (l_int32)(vc[3] * x + vc[4] * y + vc[5] + 0.5);
    return 0;
}


/*!
 *  affineXformPt()
 *
 *      Input:  vc (vector of 6 coefficients)
 *              (x, y)  (initial point)
 *              (&xp, &yp)   (<return> transformed point)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This computes the floating point location of the transformed point.
 *      (2) It does not check ptrs for returned data!
 */
l_int32
affineXformPt(l_float32  *vc,
              l_int32     x,
              l_int32     y,
              l_float32  *pxp,
              l_float32  *pyp)
{
    PROCNAME("affineXformPt");

    if (!vc)
        return ERROR_INT("vc not defined", procName, 1);

    *pxp = vc[0] * x + vc[1] * y + vc[2];
    *pyp = vc[3] * x + vc[4] * y + vc[5];
    return 0;
}


/*-------------------------------------------------------------*
 *                 Interpolation helper functions              *
 *-------------------------------------------------------------*/
/*!
 *  linearInterpolatePixelColor()
 *
 *      Input:  datas (ptr to beginning of image data)
 *              wpls (32-bit word/line for this data array)
 *              w, h (of image)
 *              x, y (floating pt location for evaluation)
 *              colorval (color brought in from the outside when the
 *                        input x,y location is outside the image;
 *                        in 0xrrggbb00 format))
 *              &val (<return> interpolated color value)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is a standard linear interpolation function.  It is
 *          equivalent to area weighting on each component, and
 *          avoids "jaggies" when rendering sharp edges.
 */
l_int32
linearInterpolatePixelColor(l_uint32  *datas,
                            l_int32    wpls,
                            l_int32    w,
                            l_int32    h,
                            l_float32  x,
                            l_float32  y,
                            l_uint32   colorval,
                            l_uint32  *pval)
{
l_int32    xpm, ypm, xp, yp, xf, yf;
l_int32    rval, gval, bval;
l_uint32   word00, word01, word10, word11;
l_uint32  *lines;

    PROCNAME("linearInterpolatePixelColor");

    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = colorval;
    if (!datas)
        return ERROR_INT("datas not defined", procName, 1);

        /* Skip if off the edge */
    if (x < 0.0 || y < 0.0 || x > w - 2.0 || y > h - 2.0)
        return 0;

    xpm = (l_int32)(16.0 * x + 0.5);
    ypm = (l_int32)(16.0 * y + 0.5);
    xp = xpm >> 4;
    yp = ypm >> 4;
    xf = xpm & 0x0f;
    yf = ypm & 0x0f;

#if  DEBUG
    if (xf < 0 || yf < 0)
        fprintf(stderr, "xp = %d, yp = %d, xf = %d, yf = %d\n", xp, yp, xf, yf);
#endif  /* DEBUG */

        /* Do area weighting (eqiv. to linear interpolation) */
    lines = datas + yp * wpls;
    word00 = *(lines + xp);
    word10 = *(lines + xp + 1);
    word01 = *(lines + wpls + xp);
    word11 = *(lines + wpls + xp + 1);
    rval = ((16 - xf) * (16 - yf) * ((word00 >> L_RED_SHIFT) & 0xff) +
        xf * (16 - yf) * ((word10 >> L_RED_SHIFT) & 0xff) +
        (16 - xf) * yf * ((word01 >> L_RED_SHIFT) & 0xff) +
        xf * yf * ((word11 >> L_RED_SHIFT) & 0xff) + 128) / 256;
    gval = ((16 - xf) * (16 - yf) * ((word00 >> L_GREEN_SHIFT) & 0xff) +
        xf * (16 - yf) * ((word10 >> L_GREEN_SHIFT) & 0xff) +
        (16 - xf) * yf * ((word01 >> L_GREEN_SHIFT) & 0xff) +
        xf * yf * ((word11 >> L_GREEN_SHIFT) & 0xff) + 128) / 256;
    bval = ((16 - xf) * (16 - yf) * ((word00 >> L_BLUE_SHIFT) & 0xff) +
        xf * (16 - yf) * ((word10 >> L_BLUE_SHIFT) & 0xff) +
        (16 - xf) * yf * ((word01 >> L_BLUE_SHIFT) & 0xff) +
        xf * yf * ((word11 >> L_BLUE_SHIFT) & 0xff) + 128) / 256;
    *pval = (rval << L_RED_SHIFT) | (gval << L_GREEN_SHIFT) |
          (bval << L_BLUE_SHIFT);
    return 0;
}


/*!
 *  linearInterpolatePixelGray()
 *
 *      Input:  datas (ptr to beginning of image data)
 *              wpls (32-bit word/line for this data array)
 *              w, h (of image)
 *              x, y (floating pt location for evaluation)
 *              grayval (color brought in from the outside when the
 *                       input x,y location is outside the image)
 *              &val (<return> interpolated gray value)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This is a standard linear interpolation function.  It is
 *          equivalent to area weighting on each component, and
 *          avoids "jaggies" when rendering sharp edges.
 */
l_int32
linearInterpolatePixelGray(l_uint32  *datas,
                           l_int32    wpls,
                           l_int32    w,
                           l_int32    h,
                           l_float32  x,
                           l_float32  y,
                           l_int32    grayval,
                           l_int32   *pval)
{
l_int32    xpm, ypm, xp, yp, xf, yf, v00, v10, v01, v11;
l_uint32  *lines;

    PROCNAME("linearInterpolatePixelGray");

    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = grayval;
    if (!datas)
        return ERROR_INT("datas not defined", procName, 1);

        /* Skip if off the edge */
    if (x < 0.0 || y < 0.0 || x > w - 2.0 || y > h - 2.0)
        return 0;

    xpm = (l_int32)(16.0 * x + 0.5);
    ypm = (l_int32)(16.0 * y + 0.5);
    xp = xpm >> 4;
    yp = ypm >> 4;
    xf = xpm & 0x0f;
    yf = ypm & 0x0f;

#if  DEBUG
    if (xf < 0 || yf < 0)
        fprintf(stderr, "xp = %d, yp = %d, xf = %d, yf = %d\n", xp, yp, xf, yf);
#endif  /* DEBUG */

        /* Interpolate by area weighting. */
    lines = datas + yp * wpls;
    v00 = (16 - xf) * (16 - yf) * GET_DATA_BYTE(lines, xp);
    v10 = xf * (16 - yf) * GET_DATA_BYTE(lines, xp + 1);
    v01 = (16 - xf) * yf * GET_DATA_BYTE(lines + wpls, xp);
    v11 = xf * yf * GET_DATA_BYTE(lines + wpls, xp + 1);
    *pval = (v00 + v01 + v10 + v11 + 128) / 256;
    return 0;
}



/*-------------------------------------------------------------*
 *               Gauss-jordan linear equation solver           *
 *-------------------------------------------------------------*/
#define  SWAP(a,b)   {temp = (a); (a) = (b); (b) = temp;}

/*!
 *  gaussjordan()
 *
 *      Input:   a  (n x n matrix)
 *               b  (rhs column vector)
 *               n  (dimension)
 *      Return:  0 if ok, 1 on error
 *
 *      Note side effects:
 *            (1) the matrix a is transformed to its inverse
 *            (2) the vector b is transformed to the solution X to the
 *                linear equation AX = B
 *
 *      Adapted from "Numerical Recipes in C, Second Edition", 1992
 *      pp. 36-41 (gauss-jordan elimination)
 */
l_int32
gaussjordan(l_float32  **a,
            l_float32   *b,
            l_int32      n)
{
l_int32    i, icol, irow, j, k, l, ll;
l_int32   *indexc, *indexr, *ipiv;
l_float32  big, dum, pivinv, temp;

    PROCNAME("gaussjordan");

    if (!a)
        return ERROR_INT("a not defined", procName, 1);
    if (!b)
        return ERROR_INT("b not defined", procName, 1);

    if ((indexc = (l_int32 *)CALLOC(n, sizeof(l_int32))) == NULL)
        return ERROR_INT("indexc not made", procName, 1);
    if ((indexr = (l_int32 *)CALLOC(n, sizeof(l_int32))) == NULL)
        return ERROR_INT("indexr not made", procName, 1);
    if ((ipiv = (l_int32 *)CALLOC(n, sizeof(l_int32))) == NULL)
        return ERROR_INT("ipiv not made", procName, 1);

    for (i = 0; i < n; i++) {
        big = 0.0;
        for (j = 0; j < n; j++) {
            if (ipiv[j] != 1)
                for (k = 0; k < n; k++) {
                    if (ipiv[k] == 0) {
                        if (fabs(a[j][k]) >= big) {
                            big = fabs(a[j][k]);
                            irow = j;
                            icol = k;
                        }
                    }
                    else if (ipiv[k] > 1)
                        return ERROR_INT("singular matrix", procName, 1);
                }
        }
        ++(ipiv[icol]);
        
        if (irow != icol) {
            for (l = 0; l < n; l++)
                SWAP(a[irow][l], a[icol][l]);
            SWAP(b[irow], b[icol]);
        }

        indexr[i] = irow;
        indexc[i] = icol;
        if (a[icol][icol] == 0.0)
            return ERROR_INT("singular matrix", procName, 1);
        pivinv = 1.0 / a[icol][icol];
        a[icol][icol] = 1.0;
        for (l = 0; l < n; l++)
            a[icol][l] *= pivinv;
        b[icol] *= pivinv;

        for (ll = 0; ll < n; ll++) 
            if (ll != icol) {
                dum = a[ll][icol];
                a[ll][icol] = 0.0;
                for (l = 0; l < n; l++)
                    a[ll][l] -= a[icol][l] * dum;
                b[ll] -= b[icol] * dum;
            }
    }

    for (l = n - 1; l >= 0; l--) {
        if (indexr[l] != indexc[l])
            for (k = 0; k < n; k++)
                SWAP(a[k][indexr[l]], a[k][indexc[l]]);
    }

    FREE(indexr);
    FREE(indexc);
    FREE(ipiv);
    return 0;
}


/*-------------------------------------------------------------*
 *              Sequential affine image transformation         *
 *-------------------------------------------------------------*/
/*!
 *  pixAffineSequential()
 *
 *      Input:  pixs
 *              ptad  (3 pts of final coordinate space)
 *              ptas  (3 pts of initial coordinate space)
 *              bw    (pixels of additional border width during computation)
 *              bh    (pixels of additional border height during computation)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) The 3 pts must not be collinear.
 *      (2) The 3 pts must be given in this order:
 *           - origin
 *           - a location along the x-axis
 *           - a location along the y-axis.
 *      (3) You must guess how much border must be added so that no
 *          pixels are lost in the transformations from src to
 *          dest coordinate space.  (This can be calculated but it
 *          is a lot of work!)  For coordinate spaces that are nearly
 *          at right angles, on a 300 ppi scanned page, the addition
 *          of 1000 pixels on each side is usually sufficient.
 *      (4) This is here for pedagogical reasons.  It is about 3x faster
 *          on 1 bpp images than pixAffineSampled(), but the results
 *          on text are much inferior.
 */
PIX *
pixAffineSequential(PIX     *pixs,
                    PTA     *ptad,
                    PTA     *ptas,
                    l_int32  bw,
                    l_int32  bh)
{
l_int32    x1, y1, x2, y2, x3, y3;    /* ptas */
l_int32    x1p, y1p, x2p, y2p, x3p, y3p;   /* ptad */
l_int32    x1sc, y1sc;  /* scaled origin */
l_float32  x2s, x2sp, scalex, scaley;
l_float32  th3, th3p, ph2, ph2p;
l_float32  rad2deg;
PIX       *pixt1, *pixt2, *pixd;

    PROCNAME("pixAffineSequential");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!ptas)
        return (PIX *)ERROR_PTR("ptas not defined", procName, NULL);
    if (!ptad)
        return (PIX *)ERROR_PTR("ptad not defined", procName, NULL);

    if (ptaGetCount(ptas) != 3)
        return (PIX *)ERROR_PTR("ptas count not 3", procName, NULL);
    if (ptaGetCount(ptad) != 3)
        return (PIX *)ERROR_PTR("ptad count not 3", procName, NULL);
    ptaGetIPt(ptas, 0, &x1, &y1);
    ptaGetIPt(ptas, 1, &x2, &y2);
    ptaGetIPt(ptas, 2, &x3, &y3);
    ptaGetIPt(ptad, 0, &x1p, &y1p);
    ptaGetIPt(ptad, 1, &x2p, &y2p);
    ptaGetIPt(ptad, 2, &x3p, &y3p);

    rad2deg = 180. / 3.1415926535;

    if (y1 == y3)
        return (PIX *)ERROR_PTR("y1 == y3!", procName, NULL);
    if (y1p == y3p)
        return (PIX *)ERROR_PTR("y1p == y3p!", procName, NULL);
        
    if (bw != 0 || bh != 0) {
            /* resize all points and add border to pixs */
        x1 = x1 + bw;
        y1 = y1 + bh;
        x2 = x2 + bw;
        y2 = y2 + bh;
        x3 = x3 + bw;
        y3 = y3 + bh;
        x1p = x1p + bw;
        y1p = y1p + bh;
        x2p = x2p + bw;
        y2p = y2p + bh;
        x3p = x3p + bw;
        y3p = y3p + bh;

        if ((pixt1 = pixAddBorderGeneral(pixs, bw, bw, bh, bh, 0)) == NULL)
            return (PIX *)ERROR_PTR("pixt1 not made", procName, NULL);
    }
    else
        pixt1 = pixCopy(NULL, pixs);

    /*-------------------------------------------------------------*
        The horizontal shear is done to move the 3rd point to the
        y axis.  This moves the 2nd point either towards or away
        from the y axis, depending on whether it is above or below
        the x axis.  That motion must be computed so that we know
        the angle of vertical shear to use to get the 2nd point
        on the x axis.  We must also know the x coordinate of the
        2nd point in order to compute how much scaling is required
        to match points on the axis.
     *-------------------------------------------------------------*/

        /* Shear angles required to put src points on x and y axes */
    th3 = atan2((l_float64)(x1 - x3), (l_float64)(y1 - y3));
    x2s = (l_float32)(x2 - ((l_float32)(y1 - y2) * (x3 - x1)) / (y1 - y3));
    if (x2s == (l_float32)x1)
        return (PIX *)ERROR_PTR("x2s == x1!", procName, NULL);
    ph2 = atan2((l_float64)(y1 - y2), (l_float64)(x2s - x1));

        /* Shear angles required to put dest points on x and y axes.
         * Use the negative of these values to instead move the
         * src points from the axes to the actual dest position.
         * These values are also needed to scale the image. */
    th3p = atan2((l_float64)(x1p - x3p), (l_float64)(y1p - y3p));
    x2sp = (l_float32)(x2p - ((l_float32)(y1p - y2p) * (x3p - x1p)) / (y1p - y3p));
    if (x2sp == (l_float32)x1p)
        return (PIX *)ERROR_PTR("x2sp == x1p!", procName, NULL);
    ph2p = atan2((l_float64)(y1p - y2p), (l_float64)(x2sp - x1p));

        /* Shear image to first put src point 3 on the y axis,
         * and then to put src point 2 on the x axis */
    pixHShearIP(pixt1, y1, th3, L_BRING_IN_WHITE);
    pixVShearIP(pixt1, x1, ph2, L_BRING_IN_WHITE);

        /* Scale image to match dest scale.  The dest scale
         * is calculated above from the angles th3p and ph2p
         * that would be required to move the dest points to
         * the x and y axes. */
    scalex = (l_float32)(x2sp - x1p) / (x2s - x1);
    scaley = (l_float32)(y3p - y1p) / (y3 - y1);
    if ((pixt2 = pixScale(pixt1, scalex, scaley)) == NULL)
        return (PIX *)ERROR_PTR("pixt2 not made", procName, NULL);

#if  DEBUG
    fprintf(stderr, "th3 = %5.1f deg, ph2 = %5.1f deg\n",
            rad2deg * th3, rad2deg * ph2);
    fprintf(stderr, "th3' = %5.1f deg, ph2' = %5.1f deg\n",
            rad2deg * th3p, rad2deg * ph2p);
    fprintf(stderr, "scalex = %6.3f, scaley = %6.3f\n", scalex, scaley);
#endif  /* DEBUG */

    /*-------------------------------------------------------------*
        Scaling moves the 1st src point, which is the origin. 
        It must now be moved again to coincide with the origin
        (1st point) of the dest.  After this is done, the 2nd
        and 3rd points must be sheared back to the original
        positions of the 2nd and 3rd dest points.  We use the
        negative of the angles that were previously computed
        for shearing those points in the dest image to x and y
        axes, and take the shears in reverse order as well.
     *-------------------------------------------------------------*/
        /* Shift image to match dest origin. */
    x1sc = (l_int32)(scalex * x1 + 0.5);   /* x comp of origin after scaling */
    y1sc = (l_int32)(scaley * y1 + 0.5);   /* y comp of origin after scaling */
    pixRasteropIP(pixt2, x1p - x1sc, y1p - y1sc, L_BRING_IN_WHITE);

        /* Shear image to take points 2 and 3 off the axis and
         * put them in the original dest position */
    pixVShearIP(pixt2, x1p, -ph2p, L_BRING_IN_WHITE);
    pixHShearIP(pixt2, y1p, -th3p, L_BRING_IN_WHITE);

    if (bw != 0 || bh != 0) {
        if ((pixd = pixRemoveBorderGeneral(pixt2, bw, bw, bh, bh)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    else
        pixd = pixClone(pixt2);

    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    return pixd;
}


