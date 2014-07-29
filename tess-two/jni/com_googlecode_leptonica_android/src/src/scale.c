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
 *  scale.c
 *
 *         Top-level scaling
 *               PIX      *pixScale()     ***
 *               PIX      *pixScaleToSize()     ***
 *               PIX      *pixScaleGeneral()     ***
 *
 *         Linearly interpreted (usually up-) scaling
 *               PIX      *pixScaleLI()     ***
 *               PIX      *pixScaleColorLI()
 *               PIX      *pixScaleColor2xLI()   ***
 *               PIX      *pixScaleColor4xLI()   ***
 *               PIX      *pixScaleGrayLI()
 *               PIX      *pixScaleGray2xLI()
 *               PIX      *pixScaleGray4xLI()
 *
 *         Scaling by closest pixel sampling
 *               PIX      *pixScaleBySampling()
 *               PIX      *pixScaleBySamplingToSize()
 *               PIX      *pixScaleByIntSampling()
 *
 *         Fast integer factor subsampling RGB to gray and to binary
 *               PIX      *pixScaleRGBToGrayFast()
 *               PIX      *pixScaleRGBToBinaryFast()
 *               PIX      *pixScaleGrayToBinaryFast()
 *
 *         Downscaling with (antialias) smoothing
 *               PIX      *pixScaleSmooth() ***
 *               PIX      *pixScaleRGBToGray2()   [special 2x reduction to gray]
 *
 *         Downscaling with (antialias) area mapping
 *               PIX      *pixScaleAreaMap()     ***
 *               PIX      *pixScaleAreaMap2()
 *
 *         Binary scaling by closest pixel sampling
 *               PIX      *pixScaleBinary()
 *
 *         Scale-to-gray (1 bpp --> 8 bpp; arbitrary downscaling)
 *               PIX      *pixScaleToGray()
 *               PIX      *pixScaleToGrayFast()
 *
 *         Scale-to-gray (1 bpp --> 8 bpp; integer downscaling)
 *               PIX      *pixScaleToGray2()
 *               PIX      *pixScaleToGray3()
 *               PIX      *pixScaleToGray4()
 *               PIX      *pixScaleToGray6()
 *               PIX      *pixScaleToGray8()
 *               PIX      *pixScaleToGray16()
 *
 *         Scale-to-gray by mipmap(1 bpp --> 8 bpp, arbitrary reduction)
 *               PIX      *pixScaleToGrayMipmap()
 *
 *         Grayscale scaling using mipmap
 *               PIX      *pixScaleMipmap()
 *
 *         Replicated (integer) expansion (all depths)
 *               PIX      *pixExpandReplicate()
 *
 *         Upscale 2x followed by binarization
 *               PIX      *pixScaleGray2xLIThresh()
 *               PIX      *pixScaleGray2xLIDither()
 *
 *         Upscale 4x followed by binarization
 *               PIX      *pixScaleGray4xLIThresh()
 *               PIX      *pixScaleGray4xLIDither()
 *
 *         Grayscale downscaling using min and max
 *               PIX      *pixScaleGrayMinMax()
 *               PIX      *pixScaleGrayMinMax2()
 *
 *         Grayscale downscaling using rank value
 *               PIX      *pixScaleGrayRankCascade()
 *               PIX      *pixScaleGrayRank2()
 *
 *         Helper function for transferring alpha with scaling
 *               l_int32   pixScaleAndTransferAlpha()
 *
 *         RGB scaling including alpha (blend) component
 *               PIX      *pixScaleWithAlpha()   ***
 *
 *  *** Note: these functions make an implicit assumption about RGB
 *            component ordering.
 */

#include <string.h>
#include "allheaders.h"

extern l_float32  AlphaMaskBorderVals[2];


/*------------------------------------------------------------------*
 *                    Top level scaling dispatcher                  *
 *------------------------------------------------------------------*/
/*!
 *  pixScale()
 *
 *      Input:  pixs (1, 2, 4, 8, 16 and 32 bpp)
 *              scalex, scaley
 *      Return: pixd, or null on error
 *
 *  This function scales 32 bpp RGB; 2, 4 or 8 bpp palette color;
 *  2, 4, 8 or 16 bpp gray; and binary images.
 *
 *  When the input has palette color, the colormap is removed and
 *  the result is either 8 bpp gray or 32 bpp RGB, depending on whether
 *  the colormap has color entries.  Images with 2, 4 or 16 bpp are
 *  converted to 8 bpp.
 *
 *  Because pixScale() is meant to be a very simple interface to a
 *  number of scaling functions, including the use of unsharp masking,
 *  the type of scaling and the sharpening parameters are chosen
 *  by default.  Grayscale and color images are scaled using one
 *  of four methods, depending on the scale factors:
 *   (1) antialiased subsampling (lowpass filtering followed by
 *       subsampling, implemented here by area mapping), for scale factors
 *       less than 0.2
 *   (2) antialiased subsampling with sharpening, for scale factors
 *       between 0.2 and 0.7
 *   (3) linear interpolation with sharpening, for scale factors between
 *       0.7 and 1.4
 *   (4) linear interpolation without sharpening, for scale factors >= 1.4.
 *
 *  One could use subsampling for scale factors very close to 1.0,
 *  because it preserves sharp edges.  Linear interpolation blurs
 *  edges because the dest pixels will typically straddle two src edge
 *  pixels.  Subsmpling removes entire columns and rows, so the edge is
 *  not blurred.  However, there are two reasons for not doing this.
 *  First, it moves edges, so that a straight line at a large angle to
 *  both horizontal and vertical will have noticable kinks where
 *  horizontal and vertical rasters are removed.  Second, although it
 *  is very fast, you get good results on sharp edges by applying
 *  a sharpening filter.
 *
 *  For images with sharp edges, sharpening substantially improves the
 *  image quality for scale factors between about 0.2 and about 2.0.
 *  pixScale() uses a small amount of sharpening by default because
 *  it strengthens edge pixels that are weak due to anti-aliasing.
 *  The default sharpening factors are:
 *      * for scaling factors < 0.7:   sharpfract = 0.2    sharpwidth = 1
 *      * for scaling factors >= 0.7:  sharpfract = 0.4    sharpwidth = 2
 *  The cases where the sharpening halfwidth is 1 or 2 have special
 *  implementations and are about twice as fast as the general case.
 *
 *  However, sharpening is computationally expensive, and one needs
 *  to consider the speed-quality tradeoff:
 *      * For upscaling of RGB images, linear interpolation plus default
 *        sharpening is about 5 times slower than upscaling alone.
 *      * For downscaling, area mapping plus default sharpening is
 *        about 10 times slower than downscaling alone.
 *  When the scale factor is larger than 1.4, the cost of sharpening,
 *  which is proportional to image area, is very large compared to the
 *  incremental quality improvement, so we cut off the default use of
 *  sharpening at 1.4.  Thus, for scale factors greater than 1.4,
 *  pixScale() only does linear interpolation.
 *
 *  In many situations you will get a satisfactory result by scaling
 *  without sharpening: call pixScaleGeneral() with @sharpfract = 0.0.
 *  Alternatively, if you wish to sharpen but not use the default
 *  value, first call pixScaleGeneral() with @sharpfract = 0.0, and
 *  then sharpen explicitly using pixUnsharpMasking().
 *
 *  Binary images are scaled to binary by sampling the closest pixel,
 *  without any low-pass filtering (averaging of neighboring pixels).
 *  This will introduce aliasing for reductions.  Aliasing can be
 *  prevented by using pixScaleToGray() instead.
 *
 *  *** Warning: implicit assumption about RGB component order
 *               for LI color scaling
 */
PIX *
pixScale(PIX       *pixs,
         l_float32  scalex,
         l_float32  scaley)
{
l_int32    sharpwidth;
l_float32  maxscale, sharpfract;

    PROCNAME("pixScale");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

        /* Reduce the default sharpening factors by 2 if maxscale < 0.7 */
    maxscale = L_MAX(scalex, scaley);
    sharpfract = (maxscale < 0.7) ? 0.2 : 0.4;
    sharpwidth = (maxscale < 0.7) ? 1 : 2;

    return pixScaleGeneral(pixs, scalex, scaley, sharpfract, sharpwidth);
}


/*!
 *  pixScaleToSize()
 *
 *      Input:  pixs (1, 2, 4, 8, 16 and 32 bpp)
 *              wd  (target width; use 0 if using height as target)
 *              hd  (target height; use 0 if using width as target)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This guarantees that the output scaled image has the
 *          dimension(s) you specify.
 *           - To specify the width with isotropic scaling, set @hd = 0.
 *           - To specify the height with isotropic scaling, set @wd = 0.
 *           - If both @wd and @hd are specified, the image is scaled
 *             (in general, anisotropically) to that size.
 *           - It is an error to set both @wd and @hd to 0.
 */
PIX *
pixScaleToSize(PIX     *pixs,
               l_int32  wd,
               l_int32  hd)
{
l_int32    w, h;
l_float32  scalex, scaley;

    PROCNAME("pixScaleToSize");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (wd <= 0 && hd <= 0)
        return (PIX *)ERROR_PTR("neither wd nor hd > 0", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    if (wd <= 0) {
        scaley = (l_float32)hd / (l_float32)h;
        scalex = scaley;
    } else if (hd <= 0) {
        scalex = (l_float32)wd / (l_float32)w;
        scaley = scalex;
    } else {
        scalex = (l_float32)wd / (l_float32)w;
        scaley = (l_float32)hd / (l_float32)h;
    }

    return pixScale(pixs, scalex, scaley);
}


/*!
 *  pixScaleGeneral()
 *
 *      Input:  pixs (1, 2, 4, 8, 16 and 32 bpp)
 *              scalex, scaley (both > 0.0)
 *              sharpfract (use 0.0 to skip sharpening)
 *              sharpwidth (halfwidth of low-pass filter; typ. 1 or 2)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) See pixScale() for usage.
 *      (2) This interface may change in the future, as other special
 *          cases are added.
 *      (3) The actual sharpening factors used depend on the maximum
 *          of the two scale factors (maxscale):
 *            maxscale <= 0.2:        no sharpening
 *            0.2 < maxscale < 1.4:   uses the input parameters
 *            maxscale >= 1.4:        no sharpening
 *      (4) To avoid sharpening for grayscale and color images with
 *          scaling factors between 0.2 and 1.4, call this function
 *          with @sharpfract == 0.0.
 *      (5) To use arbitrary sharpening in conjunction with scaling,
 *          call this function with @sharpfract = 0.0, and follow this
 *          with a call to pixUnsharpMasking() with your chosen parameters.
 */
PIX *
pixScaleGeneral(PIX       *pixs,
                l_float32  scalex,
                l_float32  scaley,
                l_float32  sharpfract,
                l_int32    sharpwidth)
{
l_int32    d;
l_float32  maxscale;
PIX       *pixt, *pixt2, *pixd;

    PROCNAME("pixScaleGeneral");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("pixs not {1,2,4,8,16,32} bpp", procName, NULL);
    if (scalex <= 0.0 || scaley <= 0.0)
        return (PIX *)ERROR_PTR("scale factor <= 0", procName, NULL);
    if (scalex == 1.0 && scaley == 1.0)
        return pixCopy(NULL, pixs);

    if (d == 1)
        return pixScaleBinary(pixs, scalex, scaley);

        /* Remove colormap; clone if possible; result is either 8 or 32 bpp */
    if ((pixt = pixConvertTo8Or32(pixs, 0, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, NULL);

        /* Scale (up or down) */
    d = pixGetDepth(pixt);
    maxscale = L_MAX(scalex, scaley);
    if (maxscale < 0.7) {  /* area mapping for anti-aliasing */
        pixt2 = pixScaleAreaMap(pixt, scalex, scaley);
        if (maxscale > 0.2 && sharpfract > 0.0 && sharpwidth > 0)
            pixd = pixUnsharpMasking(pixt2, sharpwidth, sharpfract);
        else
            pixd = pixClone(pixt2);
    } else {  /* use linear interpolation */
        if (d == 8)
            pixt2 = pixScaleGrayLI(pixt, scalex, scaley);
        else  /* d == 32 */
            pixt2 = pixScaleColorLI(pixt, scalex, scaley);
        if (maxscale < 1.4 && sharpfract > 0.0 && sharpwidth > 0)
            pixd = pixUnsharpMasking(pixt2, sharpwidth, sharpfract);
        else
            pixd = pixClone(pixt2);
    }

    pixDestroy(&pixt);
    pixDestroy(&pixt2);
    return pixd;
}


/*------------------------------------------------------------------*
 *                  Scaling by linear interpolation                 *
 *------------------------------------------------------------------*/
/*!
 *  pixScaleLI()
 *
 *      Input:  pixs (2, 4, 8 or 32 bpp; with or without colormap)
 *              scalex, scaley (must both be >= 0.7)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This function should only be used when the scale factors are
 *          greater than or equal to 0.7, and typically greater than 1.
 *          If either scale factor is smaller than 0.7, we issue a warning
 *          and invoke pixScale().
 *      (2) This works on 2, 4, 8, 16 and 32 bpp images, as well as on
 *          2, 4 and 8 bpp images that have a colormap.  If there is a
 *          colormap, it is removed to either gray or RGB, depending
 *          on the colormap.
 *      (3) This does a linear interpolation on the src image.
 *      (4) It dispatches to much faster implementations for
 *          the special cases of 2x and 4x expansion.
 *
 *  *** Warning: implicit assumption about RGB component ordering ***
 */
PIX *
pixScaleLI(PIX       *pixs,
           l_float32  scalex,
           l_float32  scaley)
{
l_int32    d;
l_float32  maxscale;
PIX       *pixt, *pixd;

    PROCNAME("pixScaleLI");

    if (!pixs || (pixGetDepth(pixs) == 1))
        return (PIX *)ERROR_PTR("pixs not defined or 1 bpp", procName, NULL);
    maxscale = L_MAX(scalex, scaley);
    if (maxscale < 0.7) {
        L_WARNING("scaling factors < 0.7; do regular scaling\n", procName);
        return pixScale(pixs, scalex, scaley);
    }
    d = pixGetDepth(pixs);
    if (d != 2 && d != 4 && d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("pixs not {2,4,8,16,32} bpp", procName, NULL);

        /* Remove colormap; clone if possible; result is either 8 or 32 bpp */
    if ((pixt = pixConvertTo8Or32(pixs, 0, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, NULL);

    d = pixGetDepth(pixt);
    if (d == 8)
        pixd = pixScaleGrayLI(pixt, scalex, scaley);
    else if (d == 32)
        pixd = pixScaleColorLI(pixt, scalex, scaley);

    pixDestroy(&pixt);
    return pixd;
}


/*!
 *  pixScaleColorLI()
 *
 *      Input:  pixs  (32 bpp, representing rgb)
 *              scalex, scaley (must both be >= 0.7)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) If this is used for scale factors less than 0.7,
 *          it will suffer from antialiasing.  A warning is issued.
 *          Particularly for document images with sharp edges,
 *          use pixScaleSmooth() or pixScaleAreaMap() instead.
 *      (2) For the general case, it's about 4x faster to manipulate
 *          the color pixels directly, rather than to make images
 *          out of each of the 3 components, scale each component
 *          using the pixScaleGrayLI(), and combine the results back
 *          into an rgb image.
 *      (3) The speed on intel hardware for the general case (not 2x)
 *          is about 10 * 10^6 dest-pixels/sec/GHz.  (The special 2x
 *          case runs at about 80 * 10^6 dest-pixels/sec/GHz.)
 */
PIX *
pixScaleColorLI(PIX      *pixs,
               l_float32  scalex,
               l_float32  scaley)
{
l_int32    ws, hs, wpls, wd, hd, wpld;
l_uint32  *datas, *datad;
l_float32  maxscale;
PIX       *pixd;

    PROCNAME("pixScaleColorLI");

    if (!pixs || (pixGetDepth(pixs) != 32))
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);
    maxscale = L_MAX(scalex, scaley);
    if (maxscale < 0.7) {
        L_WARNING("scaling factors < 0.7; do regular scaling\n", procName);
        return pixScale(pixs, scalex, scaley);
    }

        /* Do fast special cases if possible */
    if (scalex == 1.0 && scaley == 1.0)
        return pixCopy(NULL, pixs);
    if (scalex == 2.0 && scaley == 2.0)
        return pixScaleColor2xLI(pixs);
    if (scalex == 4.0 && scaley == 4.0)
        return pixScaleColor4xLI(pixs);

        /* General case */
    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    wd = (l_int32)(scalex * (l_float32)ws + 0.5);
    hd = (l_int32)(scaley * (l_float32)hs + 0.5);
    if ((pixd = pixCreate(wd, hd, 32)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, scalex, scaley);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    scaleColorLILow(datad, wd, hd, wpld, datas, ws, hs, wpls);
    if (pixGetSpp(pixs) == 4)
        pixScaleAndTransferAlpha(pixd, pixs, scalex, scaley);
    return pixd;
}


/*!
 *  pixScaleColor2xLI()
 *
 *      Input:  pixs  (32 bpp, representing rgb)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This is a special case of linear interpolated scaling,
 *          for 2x upscaling.  It is about 8x faster than using
 *          the generic pixScaleColorLI(), and about 4x faster than
 *          using the special 2x scale function pixScaleGray2xLI()
 *          on each of the three components separately.
 *      (2) The speed on intel hardware is about
 *          80 * 10^6 dest-pixels/sec/GHz.
 *
 *  *** Warning: implicit assumption about RGB component ordering ***
 */
PIX *
pixScaleColor2xLI(PIX  *pixs)
{
l_int32    ws, hs, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleColor2xLI");

    if (!pixs || (pixGetDepth(pixs) != 32))
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if ((pixd = pixCreate(2 * ws, 2 * hs, 32)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 2.0, 2.0);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    scaleColor2xLILow(datad, wpld, datas, ws, hs, wpls);
    if (pixGetSpp(pixs) == 4)
        pixScaleAndTransferAlpha(pixd, pixs, 2.0, 2.0);
    return pixd;
}


/*!
 *  pixScaleColor4xLI()
 *
 *      Input:  pixs  (32 bpp, representing rgb)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This is a special case of color linear interpolated scaling,
 *          for 4x upscaling.  It is about 3x faster than using
 *          the generic pixScaleColorLI().
 *      (2) The speed on intel hardware is about
 *          30 * 10^6 dest-pixels/sec/GHz
 *      (3) This scales each component separately, using pixScaleGray4xLI().
 *          It would be about 4x faster to inline the color code properly,
 *          in analogy to scaleColor4xLILow(), and I leave this as
 *          an exercise for someone who really needs it.
 */
PIX *
pixScaleColor4xLI(PIX  *pixs)
{
PIX  *pixr, *pixg, *pixb;
PIX  *pixrs, *pixgs, *pixbs;
PIX  *pixd;

    PROCNAME("pixScaleColor4xLI");

    if (!pixs || (pixGetDepth(pixs) != 32))
        return (PIX *)ERROR_PTR("pixs undefined or not 32 bpp", procName, NULL);

    pixr = pixGetRGBComponent(pixs, COLOR_RED);
    pixrs = pixScaleGray4xLI(pixr);
    pixDestroy(&pixr);
    pixg = pixGetRGBComponent(pixs, COLOR_GREEN);
    pixgs = pixScaleGray4xLI(pixg);
    pixDestroy(&pixg);
    pixb = pixGetRGBComponent(pixs, COLOR_BLUE);
    pixbs = pixScaleGray4xLI(pixb);
    pixDestroy(&pixb);

    if ((pixd = pixCreateRGBImage(pixrs, pixgs, pixbs)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    if (pixGetSpp(pixs) == 4)
        pixScaleAndTransferAlpha(pixd, pixs, 4.0, 4.0);

    pixDestroy(&pixrs);
    pixDestroy(&pixgs);
    pixDestroy(&pixbs);
    return pixd;
}


/*!
 *  pixScaleGrayLI()
 *
 *      Input:  pixs (8 bpp grayscale, no cmap)
 *              scalex, scaley (must both be >= 0.7)
 *      Return: pixd, or null on error
 *
 *  This function is appropriate for upscaling
 *  (magnification: scale factors > 1), and for a
 *  small amount of downscaling (reduction: scale
 *  factors > 0.5).   For scale factors less than 0.5,
 *  the best result is obtained by area mapping,
 *  but this is very expensive.  So for such large
 *  reductions, it is more appropriate to do low pass
 *  filtering followed by subsampling, a combination
 *  which is effectively a cheap form of area mapping.
 *
 *  Some details follow.
 *
 *  For each pixel in the dest, this does a linear
 *  interpolation of 4 neighboring pixels in the src.
 *  Specifically, consider the UL corner of src and
 *  dest pixels.  The UL corner of the dest falls within
 *  a src pixel, whose four corners are the UL corners
 *  of 4 adjacent src pixels.  The value of the dest
 *  is taken by linear interpolation using the values of
 *  the four src pixels and the distance of the UL corner
 *  of the dest from each corner.
 *
 *  If the image is expanded so that the dest pixel is
 *  smaller than the src pixel, such interpolation
 *  is a reasonable approach.  This interpolation is
 *  also good for a small image reduction factor that
 *  is not more than a 2x reduction.
 *
 *  Note that the linear interpolation algorithm for scaling
 *  is identical in form to the area-mapping algorithm
 *  for grayscale rotation.  The latter corresponds to a
 *  translation of each pixel without scaling.
 *
 *  This function is NOT optimal if the scaling involves
 *  a large reduction.    If the image is significantly
 *  reduced, so that the dest pixel is much larger than
 *  the src pixels, this interpolation, which is over src
 *  pixels only near the UL corner of the dest pixel,
 *  is not going to give a good area-mapping average.
 *  Because area mapping for image scaling is considerably
 *  more computationally intensive than linear interpolation,
 *  we choose not to use it.   For large image reduction,
 *  linear interpolation over adjacent src pixels
 *  degenerates asymptotically to subsampling.  But
 *  subsampling without a low-pass pre-filter causes
 *  aliasing by the nyquist theorem.  To avoid aliasing,
 *  a low-pass filter (e.g., an averaging filter) of
 *  size roughly equal to the dest pixel (i.e., the
 *  reduction factor) should be applied to the src before
 *  subsampling.
 *
 *  As an alternative to low-pass filtering and subsampling
 *  for large reduction factors, linear interpolation can
 *  also be done between the (widely separated) src pixels in
 *  which the corners of the dest pixel lie.  This also is
 *  not optimal, as it samples src pixels only near the
 *  corners of the dest pixel, and it is not implemented.
 *
 *  Summary:
 *    (1) If this is used for scale factors less than 0.7,
 *        it will suffer from antialiasing.  A warning is issued.
 *        Particularly for document images with sharp edges,
 *        use pixScaleSmooth() or pixScaleAreaMap() instead.
 *    (2) The speed on intel hardware for the general case (not 2x)
 *        is about 13 * 10^6 dest-pixels/sec/GHz.  (The special 2x
 *        case runs at about 100 * 10^6 dest-pixels/sec/GHz.)
 */
PIX *
pixScaleGrayLI(PIX       *pixs,
               l_float32  scalex,
               l_float32  scaley)
{
l_int32    ws, hs, wpls, wd, hd, wpld;
l_uint32  *datas, *datad;
l_float32  maxscale;
PIX       *pixd;

    PROCNAME("pixScaleGrayLI");

    if (!pixs || pixGetDepth(pixs) != 8 || pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs undefined, cmapped or not 8 bpp",
                                procName, NULL);
    maxscale = L_MAX(scalex, scaley);
    if (maxscale < 0.7) {
        L_WARNING("scaling factors < 0.7; do regular scaling\n", procName);
        return pixScale(pixs, scalex, scaley);
    }

        /* Do fast special cases if possible */
    if (scalex == 1.0 && scaley == 1.0)
        return pixCopy(NULL, pixs);
    if (scalex == 2.0 && scaley == 2.0)
        return pixScaleGray2xLI(pixs);
    if (scalex == 4.0 && scaley == 4.0)
        return pixScaleGray4xLI(pixs);

        /* General case */
    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    wd = (l_int32)(scalex * (l_float32)ws + 0.5);
    hd = (l_int32)(scaley * (l_float32)hs + 0.5);
    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyText(pixd, pixs);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, scalex, scaley);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    scaleGrayLILow(datad, wd, hd, wpld, datas, ws, hs, wpls);
    return pixd;
}


/*!
 *  pixScaleGray2xLI()
 *
 *      Input:  pixs (8 bpp grayscale, not cmapped)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This is a special case of gray linear interpolated scaling,
 *          for 2x upscaling.  It is about 6x faster than using
 *          the generic pixScaleGrayLI().
 *      (2) The speed on intel hardware is about
 *          100 * 10^6 dest-pixels/sec/GHz
 */
PIX *
pixScaleGray2xLI(PIX  *pixs)
{
l_int32    ws, hs, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleGray2xLI");

    if (!pixs || pixGetDepth(pixs) != 8 || pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs undefined, cmapped or not 8 bpp",
                                procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if ((pixd = pixCreate(2 * ws, 2 * hs, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 2.0, 2.0);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    scaleGray2xLILow(datad, wpld, datas, ws, hs, wpls);
    return pixd;
}


/*!
 *  pixScaleGray4xLI()
 *
 *      Input:  pixs (8 bpp grayscale, not cmapped)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This is a special case of gray linear interpolated scaling,
 *          for 4x upscaling.  It is about 12x faster than using
 *          the generic pixScaleGrayLI().
 *      (2) The speed on intel hardware is about
 *          160 * 10^6 dest-pixels/sec/GHz.
 */
PIX *
pixScaleGray4xLI(PIX  *pixs)
{
l_int32    ws, hs, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleGray4xLI");

    if (!pixs || pixGetDepth(pixs) != 8 || pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs undefined, cmapped or not 8 bpp",
                                procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    if ((pixd = pixCreate(4 * ws, 4 * hs, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 4.0, 4.0);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    scaleGray4xLILow(datad, wpld, datas, ws, hs, wpls);
    return pixd;
}



/*------------------------------------------------------------------*
 *                  Scaling by closest pixel sampling               *
 *------------------------------------------------------------------*/
/*!
 *  pixScaleBySampling()
 *
 *      Input:  pixs (1, 2, 4, 8, 16, 32 bpp)
 *              scalex, scaley (both > 0.0)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This function samples from the source without
 *          filtering.  As a result, aliasing will occur for
 *          subsampling (@scalex and/or @scaley < 1.0).
 *      (2) If @scalex == 1.0 and @scaley == 1.0, returns a copy.
 */
PIX *
pixScaleBySampling(PIX       *pixs,
                   l_float32  scalex,
                   l_float32  scaley)
{
l_int32    ws, hs, d, wpls, wd, hd, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleBySampling");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (scalex <= 0.0 || scaley <= 0.0)
        return (PIX *)ERROR_PTR("scale factor <= 0", procName, NULL);
    if (scalex == 1.0 && scaley == 1.0)
        return pixCopy(NULL, pixs);
    if ((d = pixGetDepth(pixs)) == 1)
        return pixScaleBinary(pixs, scalex, scaley);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    wd = (l_int32)(scalex * (l_float32)ws + 0.5);
    hd = (l_int32)(scaley * (l_float32)hs + 0.5);
    if ((pixd = pixCreate(wd, hd, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, scalex, scaley);
    pixCopyColormap(pixd, pixs);
    pixCopyText(pixd, pixs);
    pixCopySpp(pixd, pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    scaleBySamplingLow(datad, wd, hd, wpld, datas, ws, hs, d, wpls);
    if (d == 32 && pixGetSpp(pixs) == 4)
        pixScaleAndTransferAlpha(pixd, pixs, scalex, scaley);

    return pixd;
}


/*!
 *  pixScaleBySamplingToSize()
 *
 *      Input:  pixs (1, 2, 4, 8, 16 and 32 bpp)
 *              wd  (target width; use 0 if using height as target)
 *              hd  (target height; use 0 if using width as target)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This guarantees that the output scaled image has the
 *          dimension(s) you specify.
 *           - To specify the width with isotropic scaling, set @hd = 0.
 *           - To specify the height with isotropic scaling, set @wd = 0.
 *           - If both @wd and @hd are specified, the image is scaled
 *             (in general, anisotropically) to that size.
 *           - It is an error to set both @wd and @hd to 0.
 */
PIX *
pixScaleBySamplingToSize(PIX     *pixs,
                         l_int32  wd,
                         l_int32  hd)
{
l_int32    w, h;
l_float32  scalex, scaley;

    PROCNAME("pixScaleBySamplingToSize");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (wd <= 0 && hd <= 0)
        return (PIX *)ERROR_PTR("neither wd nor hd > 0", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    if (wd <= 0) {
        scaley = (l_float32)hd / (l_float32)h;
        scalex = scaley;
    } else if (hd <= 0) {
        scalex = (l_float32)wd / (l_float32)w;
        scaley = scalex;
    } else {
        scalex = (l_float32)wd / (l_float32)w;
        scaley = (l_float32)hd / (l_float32)h;
    }

    return pixScaleBySampling(pixs, scalex, scaley);
}


/*!
 *  pixScaleByIntSampling()
 *
 *      Input:  pixs (1, 2, 4, 8, 16, 32 bpp)
 *              factor (integer subsampling)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) Simple interface to pixScaleBySampling(), for
 *          isotropic integer reduction.
 *      (2) If @factor == 1, returns a copy.
 */
PIX *
pixScaleByIntSampling(PIX     *pixs,
                      l_int32  factor)
{
l_float32  scale;

    PROCNAME("pixScaleByIntSampling");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (factor <= 1) {
        if (factor < 1)
            L_ERROR("factor must be >= 1; returning a copy\n", procName);
        return pixCopy(NULL, pixs);
    }

    scale = 1. / (l_float32)factor;
    return pixScaleBySampling(pixs, scale, scale);
}


/*------------------------------------------------------------------*
 *            Fast integer factor subsampling RGB to gray           *
 *------------------------------------------------------------------*/
/*!
 *  pixScaleRGBToGrayFast()
 *
 *      Input:  pixs (32 bpp rgb)
 *              factor (integer reduction factor >= 1)
 *              color (one of COLOR_RED, COLOR_GREEN, COLOR_BLUE)
 *      Return: pixd (8 bpp), or null on error
 *
 *  Notes:
 *      (1) This does simultaneous subsampling by an integer factor and
 *          extraction of the color from the RGB pix.
 *      (2) It is designed for maximum speed, and is used for quickly
 *          generating a downsized grayscale image from a higher resolution
 *          RGB image.  This would typically be used for image analysis.
 *      (3) The standard color byte order (RGBA) is assumed.
 */
PIX *
pixScaleRGBToGrayFast(PIX     *pixs,
                      l_int32  factor,
                      l_int32  color)
{
l_int32    byteval, shift;
l_int32    i, j, ws, hs, wd, hd, wpls, wpld;
l_uint32  *datas, *words, *datad, *lined;
l_float32  scale;
PIX       *pixd;

    PROCNAME("pixScaleRGBToGrayFast");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("depth not 32 bpp", procName, NULL);
    if (factor < 1)
        return (PIX *)ERROR_PTR("factor must be >= 1", procName, NULL);

    if (color == COLOR_RED)
        shift = L_RED_SHIFT;
    else if (color == COLOR_GREEN)
        shift = L_GREEN_SHIFT;
    else if (color == COLOR_BLUE)
        shift = L_BLUE_SHIFT;
    else
        return (PIX *)ERROR_PTR("invalid color", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);

    wd = ws / factor;
    hd = hs / factor;
    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    scale = 1. / (l_float32) factor;
    pixScaleResolution(pixd, scale, scale);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    for (i = 0; i < hd; i++) {
        words = datas + i * factor * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < wd; j++, words += factor) {
            byteval = ((*words) >> shift) & 0xff;
            SET_DATA_BYTE(lined, j, byteval);
        }
    }

    return pixd;
}


/*!
 *  pixScaleRGBToBinaryFast()
 *
 *      Input:  pixs (32 bpp RGB)
 *              factor (integer reduction factor >= 1)
 *              thresh (binarization threshold)
 *      Return: pixd (1 bpp), or null on error
 *
 *  Notes:
 *      (1) This does simultaneous subsampling by an integer factor and
 *          conversion from RGB to gray to binary.
 *      (2) It is designed for maximum speed, and is used for quickly
 *          generating a downsized binary image from a higher resolution
 *          RGB image.  This would typically be used for image analysis.
 *      (3) It uses the green channel to represent the RGB pixel intensity.
 */
PIX *
pixScaleRGBToBinaryFast(PIX     *pixs,
                        l_int32  factor,
                        l_int32  thresh)
{
l_int32    byteval;
l_int32    i, j, ws, hs, wd, hd, wpls, wpld;
l_uint32  *datas, *words, *datad, *lined;
l_float32  scale;
PIX       *pixd;

    PROCNAME("pixScaleRGBToBinaryFast");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (factor < 1)
        return (PIX *)ERROR_PTR("factor must be >= 1", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("depth not 32 bpp", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);

    wd = ws / factor;
    hd = hs / factor;
    if ((pixd = pixCreate(wd, hd, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    scale = 1. / (l_float32) factor;
    pixScaleResolution(pixd, scale, scale);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    for (i = 0; i < hd; i++) {
        words = datas + i * factor * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < wd; j++, words += factor) {
            byteval = ((*words) >> L_GREEN_SHIFT) & 0xff;
            if (byteval < thresh)
                SET_DATA_BIT(lined, j);
        }
    }

    return pixd;
}


/*!
 *  pixScaleGrayToBinaryFast()
 *
 *      Input:  pixs (8 bpp grayscale)
 *              factor (integer reduction factor >= 1)
 *              thresh (binarization threshold)
 *      Return: pixd (1 bpp), or null on error
 *
 *  Notes:
 *      (1) This does simultaneous subsampling by an integer factor and
 *          thresholding from gray to binary.
 *      (2) It is designed for maximum speed, and is used for quickly
 *          generating a downsized binary image from a higher resolution
 *          gray image.  This would typically be used for image analysis.
 */
PIX *
pixScaleGrayToBinaryFast(PIX     *pixs,
                         l_int32  factor,
                         l_int32  thresh)
{
l_int32    byteval;
l_int32    i, j, ws, hs, wd, hd, wpls, wpld, sj;
l_uint32  *datas, *datad, *lines, *lined;
l_float32  scale;
PIX       *pixd;

    PROCNAME("pixScaleGrayToBinaryFast");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (factor < 1)
        return (PIX *)ERROR_PTR("factor must be >= 1", procName, NULL);
    if (pixGetDepth(pixs) != 8)
        return (PIX *)ERROR_PTR("depth not 8 bpp", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);

    wd = ws / factor;
    hd = hs / factor;
    if ((pixd = pixCreate(wd, hd, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    scale = 1. / (l_float32) factor;
    pixScaleResolution(pixd, scale, scale);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    for (i = 0; i < hd; i++) {
        lines = datas + i * factor * wpls;
        lined = datad + i * wpld;
        for (j = 0, sj = 0; j < wd; j++, sj += factor) {
            byteval = GET_DATA_BYTE(lines, sj);
            if (byteval < thresh)
                SET_DATA_BIT(lined, j);
        }
    }

    return pixd;
}


/*------------------------------------------------------------------*
 *               Downscaling with (antialias) smoothing             *
 *------------------------------------------------------------------*/
/*!
 *  pixScaleSmooth()
 *
 *      Input:  pixs (2, 4, 8 or 32 bpp; and 2, 4, 8 bpp with colormap)
 *              scalex, scaley (must both be < 0.7)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This function should only be used when the scale factors are less
 *          than or equal to 0.7 (i.e., more than about 1.42x reduction).
 *          If either scale factor is larger than 0.7, we issue a warning
 *          and invoke pixScale().
 *      (2) This works only on 2, 4, 8 and 32 bpp images, and if there is
 *          a colormap, it is removed by converting to RGB.  In other
 *          cases, we issue a warning and invoke pixScale().
 *      (3) It does simple (flat filter) convolution, with a filter size
 *          commensurate with the amount of reduction, to avoid antialiasing.
 *      (4) It does simple subsampling after smoothing, which is appropriate
 *          for this range of scaling.  Linear interpolation gives essentially
 *          the same result with more computation for these scale factors,
 *          so we don't use it.
 *      (5) The result is the same as doing a full block convolution followed by
 *          subsampling, but this is faster because the results of the block
 *          convolution are only computed at the subsampling locations.
 *          In fact, the computation time is approximately independent of
 *          the scale factor, because the convolution kernel is adjusted
 *          so that each source pixel is summed approximately once.
 *
 *  *** Warning: implicit assumption about RGB component ordering ***
 */
PIX *
pixScaleSmooth(PIX       *pix,
               l_float32  scalex,
               l_float32  scaley)
{
l_int32    ws, hs, d, wd, hd, wpls, wpld, isize;
l_uint32  *datas, *datad;
l_float32  minscale, size;
PIX       *pixs, *pixd;

    PROCNAME("pixScaleSmooth");

    if (!pix)
        return (PIX *)ERROR_PTR("pix not defined", procName, NULL);
    if (scalex >= 0.7 || scaley >= 0.7) {
        L_WARNING("scaling factor not < 0.7; do regular scaling\n", procName);
        return pixScale(pix, scalex, scaley);
    }

        /* Remove colormap if necessary.
         * If 2 bpp or 4 bpp gray, convert to 8 bpp */
    d = pixGetDepth(pix);
    if ((d == 2 || d == 4 || d == 8) && pixGetColormap(pix)) {
        L_WARNING("pix has colormap; removing\n", procName);
        pixs = pixRemoveColormap(pix, REMOVE_CMAP_BASED_ON_SRC);
        d = pixGetDepth(pixs);
    } else if (d == 2 || d == 4) {
        pixs = pixConvertTo8(pix, FALSE);
        d = 8;
    } else {
        pixs = pixClone(pix);
    }

    if (d != 8 && d != 32) {   /* d == 1 or d == 16 */
        L_WARNING("depth not 8 or 32 bpp; do regular scaling\n", procName);
        pixDestroy(&pixs);
        return pixScale(pix, scalex, scaley);
    }

        /* If 1.42 < 1/minscale < 2.5, use isize = 2
         * If 2.5 =< 1/minscale < 3.5, use isize = 3, etc.
         * Under no conditions use isize < 2  */
    minscale = L_MIN(scalex, scaley);
    size = 1.0 / minscale;   /* ideal filter full width */
    isize = L_MAX(2, (l_int32)(size + 0.5));

    pixGetDimensions(pixs, &ws, &hs, NULL);
    if ((ws < isize) || (hs < isize)) {
        pixDestroy(&pixs);
        return (PIX *)ERROR_PTR("pixs too small", procName, NULL);
    }
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    wd = (l_int32)(scalex * (l_float32)ws + 0.5);
    hd = (l_int32)(scaley * (l_float32)hs + 0.5);
    if (wd < 1 || hd < 1) {
        pixDestroy(&pixs);
        return (PIX *)ERROR_PTR("pixd too small", procName, NULL);
    }
    if ((pixd = pixCreate(wd, hd, d)) == NULL) {
        pixDestroy(&pixs);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, scalex, scaley);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    scaleSmoothLow(datad, wd, hd, wpld, datas, ws, hs, d, wpls, isize);
    if (d == 32 && pixGetSpp(pixs) == 4)
        pixScaleAndTransferAlpha(pixd, pixs, scalex, scaley);

    pixDestroy(&pixs);
    return pixd;
}


/*!
 *  pixScaleRGBToGray2()
 *
 *      Input:  pixs (32 bpp rgb)
 *              rwt, gwt, bwt (must sum to 1.0)
 *      Return: pixd, (8 bpp, 2x reduced), or null on error
 */
PIX *
pixScaleRGBToGray2(PIX       *pixs,
                   l_float32  rwt,
                   l_float32  gwt,
                   l_float32  bwt)
{
l_int32    wd, hd, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleRGBToGray2");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 32)
        return (PIX *)ERROR_PTR("pixs not 32 bpp", procName, NULL);
    if (rwt + gwt + bwt < 0.98 || rwt + gwt + bwt > 1.02)
        return (PIX *)ERROR_PTR("sum of wts should be 1.0", procName, NULL);

    wd = pixGetWidth(pixs) / 2;
    hd = pixGetHeight(pixs) / 2;
    wpls = pixGetWpl(pixs);
    datas = pixGetData(pixs);
    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 0.5, 0.5);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);
    scaleRGBToGray2Low(datad, wd, hd, wpld, datas, wpls, rwt, gwt, bwt);
    return pixd;
}


/*------------------------------------------------------------------*
 *             Downscaling with (antialias) area mapping            *
 *------------------------------------------------------------------*/
/*!
 *  pixScaleAreaMap()
 *
 *      Input:  pixs (2, 4, 8 or 32 bpp; and 2, 4, 8 bpp with colormap)
 *              scalex, scaley (must both be <= 0.7)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This function should only be used when the scale factors are less
 *          than or equal to 0.7 (i.e., more than about 1.42x reduction).
 *          If either scale factor is larger than 0.7, we issue a warning
 *          and invoke pixScale().
 *      (2) This works only on 2, 4, 8 and 32 bpp images.  If there is
 *          a colormap, it is removed by converting to RGB.  In other
 *          cases, we issue a warning and invoke pixScale().
 *      (3) It does a relatively expensive area mapping computation, to
 *          avoid antialiasing.  It is about 2x slower than pixScaleSmooth(),
 *          but the results are much better on fine text.
 *      (4) This is typically about 20% faster for the special cases of
 *          2x, 4x, 8x and 16x reduction.
 *      (5) Surprisingly, there is no speedup (and a slight quality
 *          impairment) if you do as many successive 2x reductions as
 *          possible, ending with a reduction with a scale factor larger
 *          than 0.5.
 *
 *  *** Warning: implicit assumption about RGB component ordering ***
 */
PIX *
pixScaleAreaMap(PIX       *pix,
                l_float32  scalex,
                l_float32  scaley)
{
l_int32    ws, hs, d, wd, hd, wpls, wpld;
l_uint32  *datas, *datad;
l_float32  maxscale;
PIX       *pixs, *pixd, *pixt1, *pixt2, *pixt3;

    PROCNAME("pixScaleAreaMap");

    if (!pix)
        return (PIX *)ERROR_PTR("pix not defined", procName, NULL);
    d = pixGetDepth(pix);
    if (d != 2 && d != 4 && d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pix not 2, 4, 8 or 32 bpp", procName, NULL);
    maxscale = L_MAX(scalex, scaley);
    if (maxscale >= 0.7) {
        L_WARNING("scaling factors not < 0.7; do regular scaling\n", procName);
        return pixScale(pix, scalex, scaley);
    }

        /* Special cases: 2x, 4x, 8x, 16x reduction */
    if (scalex == 0.5 && scaley == 0.5)
        return pixScaleAreaMap2(pix);
    if (scalex == 0.25 && scaley == 0.25) {
        pixt1 = pixScaleAreaMap2(pix);
        pixd = pixScaleAreaMap2(pixt1);
        pixDestroy(&pixt1);
        return pixd;
    }
    if (scalex == 0.125 && scaley == 0.125) {
        pixt1 = pixScaleAreaMap2(pix);
        pixt2 = pixScaleAreaMap2(pixt1);
        pixd = pixScaleAreaMap2(pixt2);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        return pixd;
    }
    if (scalex == 0.0625 && scaley == 0.0625) {
        pixt1 = pixScaleAreaMap2(pix);
        pixt2 = pixScaleAreaMap2(pixt1);
        pixt3 = pixScaleAreaMap2(pixt2);
        pixd = pixScaleAreaMap2(pixt3);
        pixDestroy(&pixt1);
        pixDestroy(&pixt2);
        pixDestroy(&pixt3);
        return pixd;
    }

        /* Remove colormap if necessary.
         * If 2 bpp or 4 bpp gray, convert to 8 bpp */
    if ((d == 2 || d == 4 || d == 8) && pixGetColormap(pix)) {
        L_WARNING("pix has colormap; removing\n", procName);
        pixs = pixRemoveColormap(pix, REMOVE_CMAP_BASED_ON_SRC);
        d = pixGetDepth(pixs);
    } else if (d == 2 || d == 4) {
        pixs = pixConvertTo8(pix, FALSE);
        d = 8;
    } else {
        pixs = pixClone(pix);
    }

    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    wd = (l_int32)(scalex * (l_float32)ws + 0.5);
    hd = (l_int32)(scaley * (l_float32)hs + 0.5);
    if (wd < 1 || hd < 1) {
        pixDestroy(&pixs);
        return (PIX *)ERROR_PTR("pixd too small", procName, NULL);
    }
    if ((pixd = pixCreate(wd, hd, d)) == NULL) {
        pixDestroy(&pixs);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, scalex, scaley);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    if (d == 8) {
        scaleGrayAreaMapLow(datad, wd, hd, wpld, datas, ws, hs, wpls);
    } else {  /* RGB, d == 32 */
        scaleColorAreaMapLow(datad, wd, hd, wpld, datas, ws, hs, wpls);
        if (pixGetSpp(pixs) == 4)
            pixScaleAndTransferAlpha(pixd, pixs, scalex, scaley);
    }

    pixDestroy(&pixs);
    return pixd;
}


/*!
 *  pixScaleAreaMap2()
 *
 *      Input:  pixs (2, 4, 8 or 32 bpp; and 2, 4, 8 bpp with colormap)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This function does an area mapping (average) for 2x
 *          reduction.
 *      (2) This works only on 2, 4, 8 and 32 bpp images.  If there is
 *          a colormap, it is removed by converting to RGB.
 *      (3) Speed on 3 GHz processor:
 *             Color: 160 Mpix/sec
 *             Gray: 700 Mpix/sec
 *          This contrasts with the speed of the general pixScaleAreaMap():
 *             Color: 35 Mpix/sec
 *             Gray: 50 Mpix/sec
 *      (4) From (3), we see that this special function is about 4.5x
 *          faster for color and 14x faster for grayscale
 *      (5) Consequently, pixScaleAreaMap2() is incorporated into the
 *          general area map scaling function, for the special cases
 *          of 2x, 4x, 8x and 16x reduction.
 */
PIX *
pixScaleAreaMap2(PIX  *pix)
{
l_int32    wd, hd, d, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixs, *pixd;

    PROCNAME("pixScaleAreaMap2");

    if (!pix)
        return (PIX *)ERROR_PTR("pix not defined", procName, NULL);
    d = pixGetDepth(pix);
    if (d != 2 && d != 4 && d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pix not 2, 4, 8 or 32 bpp", procName, NULL);

        /* Remove colormap if necessary.
         * If 2 bpp or 4 bpp gray, convert to 8 bpp */
    if ((d == 2 || d == 4 || d == 8) && pixGetColormap(pix)) {
        L_WARNING("pix has colormap; removing\n", procName);
        pixs = pixRemoveColormap(pix, REMOVE_CMAP_BASED_ON_SRC);
        d = pixGetDepth(pixs);
    } else if (d == 2 || d == 4) {
        pixs = pixConvertTo8(pix, FALSE);
        d = 8;
    } else {
        pixs = pixClone(pix);
    }

    wd = pixGetWidth(pixs) / 2;
    hd = pixGetHeight(pixs) / 2;
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    pixd = pixCreate(wd, hd, d);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 0.5, 0.5);
    scaleAreaMapLow2(datad, wd, hd, wpld, datas, d, wpls);
    if (pixGetSpp(pixs) == 4)
        pixScaleAndTransferAlpha(pixd, pixs, 0.5, 0.5);
    pixDestroy(&pixs);
    return pixd;
}


/*------------------------------------------------------------------*
 *               Binary scaling by closest pixel sampling           *
 *------------------------------------------------------------------*/
/*!
 *  pixScaleBinary()
 *
 *      Input:  pixs (1 bpp)
 *              scalex, scaley (both > 0.0)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This function samples from the source without
 *          filtering.  As a result, aliasing will occur for
 *          subsampling (scalex and scaley < 1.0).
 */
PIX *
pixScaleBinary(PIX       *pixs,
               l_float32  scalex,
               l_float32  scaley)
{
l_int32    ws, hs, wpls, wd, hd, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleBinary");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs must be 1 bpp", procName, NULL);
    if (scalex <= 0.0 || scaley <= 0.0)
        return (PIX *)ERROR_PTR("scale factor <= 0", procName, NULL);
    if (scalex == 1.0 && scaley == 1.0)
        return pixCopy(NULL, pixs);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    wd = (l_int32)(scalex * (l_float32)ws + 0.5);
    hd = (l_int32)(scaley * (l_float32)hs + 0.5);
    if ((pixd = pixCreate(wd, hd, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyColormap(pixd, pixs);
    pixCopyText(pixd, pixs);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, scalex, scaley);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    scaleBinaryLow(datad, wd, hd, wpld, datas, ws, hs, wpls);
    return pixd;
}



/*------------------------------------------------------------------*
 *      Scale-to-gray (1 bpp --> 8 bpp; arbitrary downscaling)      *
 *------------------------------------------------------------------*/
/*!
 *  pixScaleToGray()
 *
 *      Input:  pixs (1 bpp)
 *              scalefactor (reduction: must be > 0.0 and < 1.0)
 *      Return: pixd (8 bpp), scaled down by scalefactor in each direction,
 *              or NULL on error.
 *
 *  Notes:
 *
 *  For faster scaling in the range of scalefactors from 0.0625 to 0.5,
 *  with very little difference in quality, use pixScaleToGrayFast().
 *
 *  Binary images have sharp edges, so they intrinsically have very
 *  high frequency content.  To avoid aliasing, they must be low-pass
 *  filtered, which tends to blur the edges.  How can we keep relatively
 *  crisp edges without aliasing?  The trick is to do binary upscaling
 *  followed by a power-of-2 scaleToGray.  For large reductions, where
 *  you don't end up with much detail, some corners can be cut.
 *
 *  The intent here is to get high quality reduced grayscale
 *  images with relatively little computation.  We do binary
 *  pre-scaling followed by scaleToGrayN() for best results,
 *  esp. to avoid excess blur when the scale factor is near
 *  an inverse power of 2.  Where a low-pass filter is required,
 *  we use simple convolution kernels: either the hat filter for
 *  linear interpolation or a flat filter for larger downscaling.
 *  Other choices, such as a perfect bandpass filter with infinite extent
 *  (the sinc) or various approximations to it (e.g., lanczos), are
 *  unnecessarily expensive.
 *
 *  The choices made are as follows:
 *      (1) Do binary upscaling before scaleToGrayN() for scalefactors > 1/8
 *      (2) Do binary downscaling before scaleToGray8() for scalefactors
 *          between 1/16 and 1/8.
 *      (3) Use scaleToGray16() before grayscale downscaling for
 *          scalefactors less than 1/16
 *  Another reasonable choice would be to start binary downscaling
 *  for scalefactors below 1/4, rather than below 1/8 as we do here.
 *
 *  The general scaling rules, not all of which are used here, go as follows:
 *      (1) For grayscale upscaling, use pixScaleGrayLI().  However,
 *          note that edges will be visibly blurred for scalefactors
 *          near (but above) 1.0.  Replication will avoid edge blur,
 *          and should be considered for factors very near 1.0.
 *      (2) For grayscale downscaling with a scale factor larger than
 *          about 0.7, use pixScaleGrayLI().  For scalefactors near
 *          (but below) 1.0, you tread between Scylla and Charybdis.
 *          pixScaleGrayLI() again gives edge blurring, but
 *          pixScaleBySampling() gives visible aliasing.
 *      (3) For grayscale downscaling with a scale factor smaller than
 *          about 0.7, use pixScaleSmooth()
 *      (4) For binary input images, do as much scale to gray as possible
 *          using the special integer functions (2, 3, 4, 8 and 16).
 *      (5) It is better to upscale in binary, followed by scaleToGrayN()
 *          than to do scaleToGrayN() followed by an upscale using either
 *          LI or oversampling.
 *      (6) It may be better to downscale in binary, followed by
 *          scaleToGrayN() than to first use scaleToGrayN() followed by
 *          downscaling.  For downscaling between 8x and 16x, this is
 *          a reasonable option.
 *      (7) For reductions greater than 16x, it's reasonable to use
 *          scaleToGray16() followed by further grayscale downscaling.
 */
PIX *
pixScaleToGray(PIX       *pixs,
               l_float32  scalefactor)
{
l_int32    w, h, minsrc, mindest;
l_float32  mag, red;
PIX       *pixt, *pixd;

    PROCNAME("pixScaleToGray");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);
    if (scalefactor <= 0.0)
        return (PIX *)ERROR_PTR("scalefactor <= 0.0", procName, NULL);
    if (scalefactor >= 1.0)
        return (PIX *)ERROR_PTR("scalefactor >= 1.0", procName, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);
    minsrc = L_MIN(w, h);
    mindest = (l_int32)((l_float32)minsrc * scalefactor);
    if (mindest < 2)
        return (PIX *)ERROR_PTR("scalefactor too small", procName, NULL);

    if (scalefactor > 0.5) {   /* see note (5) */
        mag = 2.0 * scalefactor;  /* will be < 2.0 */
/*        fprintf(stderr, "2x with mag %7.3f\n", mag);  */
        if ((pixt = pixScaleBinary(pixs, mag, mag)) == NULL)
            return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
        pixd = pixScaleToGray2(pixt);
    } else if (scalefactor == 0.5) {
        return pixd = pixScaleToGray2(pixs);
    } else if (scalefactor > 0.33333) {   /* see note (5) */
        mag = 3.0 * scalefactor;   /* will be < 1.5 */
/*        fprintf(stderr, "3x with mag %7.3f\n", mag);  */
        if ((pixt = pixScaleBinary(pixs, mag, mag)) == NULL)
            return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
        pixd = pixScaleToGray3(pixt);
    } else if (scalefactor > 0.25) {  /* see note (5) */
        mag = 4.0 * scalefactor;   /* will be < 1.3333 */
/*        fprintf(stderr, "4x with mag %7.3f\n", mag);  */
        if ((pixt = pixScaleBinary(pixs, mag, mag)) == NULL)
            return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
        pixd = pixScaleToGray4(pixt);
    } else if (scalefactor == 0.25) {
        return pixd = pixScaleToGray4(pixs);
    } else if (scalefactor > 0.16667) {  /* see note (5) */
        mag = 6.0 * scalefactor;   /* will be < 1.5 */
/*        fprintf(stderr, "6x with mag %7.3f\n", mag); */
        if ((pixt = pixScaleBinary(pixs, mag, mag)) == NULL)
            return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
        pixd = pixScaleToGray6(pixt);
    } else if (scalefactor == 0.16667) {
        return pixd = pixScaleToGray6(pixs);
    } else if (scalefactor > 0.125) {  /* see note (5) */
        mag = 8.0 * scalefactor;   /*  will be < 1.3333  */
/*        fprintf(stderr, "8x with mag %7.3f\n", mag);  */
        if ((pixt = pixScaleBinary(pixs, mag, mag)) == NULL)
            return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
        pixd = pixScaleToGray8(pixt);
    } else if (scalefactor == 0.125) {
        return pixd = pixScaleToGray8(pixs);
    } else if (scalefactor > 0.0625) {  /* see note (6) */
        red = 8.0 * scalefactor;   /* will be > 0.5 */
/*        fprintf(stderr, "8x with red %7.3f\n", red);  */
        if ((pixt = pixScaleBinary(pixs, red, red)) == NULL)
            return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
        pixd = pixScaleToGray8(pixt);
    } else if (scalefactor == 0.0625) {
        return pixd = pixScaleToGray16(pixs);
    } else {  /* see note (7) */
        red = 16.0 * scalefactor;  /* will be <= 1.0 */
/*        fprintf(stderr, "16x with red %7.3f\n", red);  */
        if ((pixt = pixScaleToGray16(pixs)) == NULL)
            return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
        if (red < 0.7)
            pixd = pixScaleSmooth(pixt, red, red);  /* see note (3) */
        else
            pixd = pixScaleGrayLI(pixt, red, red);  /* see note (2) */
    }

    pixDestroy(&pixt);
    if (!pixd)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    else
        return pixd;
}



/*!
 *  pixScaleToGrayFast()
 *
 *      Input:  pixs (1 bpp)
 *              scalefactor (reduction: must be > 0.0 and < 1.0)
 *      Return: pixd (8 bpp), scaled down by scalefactor in each direction,
 *              or NULL on error.
 *
 *  Notes:
 *      (1) See notes in pixScaleToGray() for the basic approach.
 *      (2) This function is considerably less expensive than pixScaleToGray()
 *          for scalefactor in the range (0.0625 ... 0.5), and the
 *          quality is nearly as good.
 *      (3) Unlike pixScaleToGray(), which does binary upscaling before
 *          downscaling for scale factors >= 0.0625, pixScaleToGrayFast()
 *          first downscales in binary for all scale factors < 0.5, and
 *          then does a 2x scale-to-gray as the final step.  For
 *          scale factors < 0.0625, both do a 16x scale-to-gray, followed
 *          by further grayscale reduction.
 */
PIX *
pixScaleToGrayFast(PIX       *pixs,
                   l_float32  scalefactor)
{
l_int32    w, h, minsrc, mindest;
l_float32  eps, factor;
PIX       *pixt, *pixd;

    PROCNAME("pixScaleToGrayFast");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);
    if (scalefactor <= 0.0)
        return (PIX *)ERROR_PTR("scalefactor <= 0.0", procName, NULL);
    if (scalefactor >= 1.0)
        return (PIX *)ERROR_PTR("scalefactor >= 1.0", procName, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);
    minsrc = L_MIN(w, h);
    mindest = (l_int32)((l_float32)minsrc * scalefactor);
    if (mindest < 2)
        return (PIX *)ERROR_PTR("scalefactor too small", procName, NULL);
    eps = 0.0001;

        /* Handle the special cases */
    if (scalefactor > 0.5 - eps && scalefactor < 0.5 + eps)
        return pixScaleToGray2(pixs);
    else if (scalefactor > 0.33333 - eps && scalefactor < 0.33333 + eps)
        return pixScaleToGray3(pixs);
    else if (scalefactor > 0.25 - eps && scalefactor < 0.25 + eps)
        return pixScaleToGray4(pixs);
    else if (scalefactor > 0.16666 - eps && scalefactor < 0.16666 + eps)
        return pixScaleToGray6(pixs);
    else if (scalefactor > 0.125 - eps && scalefactor < 0.125 + eps)
        return pixScaleToGray8(pixs);
    else if (scalefactor > 0.0625 - eps && scalefactor < 0.0625 + eps)
        return pixScaleToGray16(pixs);

    if (scalefactor > 0.0625) {  /* scale binary first */
        factor = 2.0 * scalefactor;
        if ((pixt = pixScaleBinary(pixs, factor, factor)) == NULL)
            return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
        pixd = pixScaleToGray2(pixt);
    } else {  /* scalefactor < 0.0625; scale-to-gray first */
        factor = 16.0 * scalefactor;  /* will be < 1.0 */
        if ((pixt = pixScaleToGray16(pixs)) == NULL)
            return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
        if (factor < 0.7)
            pixd = pixScaleSmooth(pixt, factor, factor);
        else
            pixd = pixScaleGrayLI(pixt, factor, factor);
    }
    pixDestroy(&pixt);
    if (!pixd)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    else
        return pixd;
}


/*-----------------------------------------------------------------------*
 *          Scale-to-gray (1 bpp --> 8 bpp; integer downscaling)         *
 *-----------------------------------------------------------------------*/
/*!
 *  pixScaleToGray2()
 *
 *      Input:  pixs (1 bpp)
 *      Return: pixd (8 bpp), scaled down by 2x in each direction,
 *              or null on error.
 */
PIX *
pixScaleToGray2(PIX  *pixs)
{
l_uint8   *valtab;
l_int32    ws, hs, wd, hd;
l_int32    wpld, wpls;
l_uint32  *sumtab;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleToGray2");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs must be 1 bpp", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = ws / 2;
    hd = hs / 2;
    if (wd == 0 || hd == 0)
        return (PIX *)ERROR_PTR("pixs too small", procName, NULL);

    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 0.5, 0.5);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);

    if ((sumtab = makeSumTabSG2()) == NULL)
        return (PIX *)ERROR_PTR("sumtab not made", procName, NULL);
    if ((valtab = makeValTabSG2()) == NULL)
        return (PIX *)ERROR_PTR("valtab not made", procName, NULL);

    scaleToGray2Low(datad, wd, hd, wpld, datas, wpls, sumtab, valtab);

    FREE(sumtab);
    FREE(valtab);
    return pixd;
}


/*!
 *  pixScaleToGray3()
 *
 *      Input:  pixs (1 bpp)
 *      Return: pixd (8 bpp), scaled down by 3x in each direction,
 *              or null on error.
 *
 *  Notes:
 *      (1) Speed is about 100 x 10^6 src-pixels/sec/GHz.
 *          Another way to express this is it processes 1 src pixel
 *          in about 10 cycles.
 *      (2) The width of pixd is truncated is truncated to a factor of 8.
 */
PIX *
pixScaleToGray3(PIX  *pixs)
{
l_uint8   *valtab;
l_int32    ws, hs, wd, hd;
l_int32    wpld, wpls;
l_uint32  *sumtab;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleToGray3");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = (ws / 3) & 0xfffffff8;    /* truncate to factor of 8 */
    hd = hs / 3;
    if (wd == 0 || hd == 0)
        return (PIX *)ERROR_PTR("pixs too small", procName, NULL);

    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 0.33333, 0.33333);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);

    if ((sumtab = makeSumTabSG3()) == NULL)
        return (PIX *)ERROR_PTR("sumtab not made", procName, NULL);
    if ((valtab = makeValTabSG3()) == NULL)
        return (PIX *)ERROR_PTR("valtab not made", procName, NULL);

    scaleToGray3Low(datad, wd, hd, wpld, datas, wpls, sumtab, valtab);

    FREE(sumtab);
    FREE(valtab);
    return pixd;
}


/*!
 *  pixScaleToGray4()
 *
 *      Input:  pixs (1 bpp)
 *      Return: pixd (8 bpp), scaled down by 4x in each direction,
 *              or null on error.
 *
 *  Notes:
 *      (1) The width of pixd is truncated is truncated to a factor of 2.
 */
PIX *
pixScaleToGray4(PIX  *pixs)
{
l_uint8   *valtab;
l_int32    ws, hs, wd, hd;
l_int32    wpld, wpls;
l_uint32  *sumtab;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleToGray4");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs must be 1 bpp", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = (ws / 4) & 0xfffffffe;    /* truncate to factor of 2 */
    hd = hs / 4;
    if (wd == 0 || hd == 0)
        return (PIX *)ERROR_PTR("pixs too small", procName, NULL);

    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 0.25, 0.25);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);

    if ((sumtab = makeSumTabSG4()) == NULL)
        return (PIX *)ERROR_PTR("sumtab not made", procName, NULL);
    if ((valtab = makeValTabSG4()) == NULL)
        return (PIX *)ERROR_PTR("valtab not made", procName, NULL);

    scaleToGray4Low(datad, wd, hd, wpld, datas, wpls, sumtab, valtab);

    FREE(sumtab);
    FREE(valtab);
    return pixd;
}



/*!
 *  pixScaleToGray6()
 *
 *      Input:  pixs (1 bpp)
 *      Return: pixd (8 bpp), scaled down by 6x in each direction,
 *              or null on error.
 *
 *  Notes:
 *      (1) The width of pixd is truncated is truncated to a factor of 8.
 */
PIX *
pixScaleToGray6(PIX  *pixs)
{
l_uint8   *valtab;
l_int32    ws, hs, wd, hd, wpld, wpls;
l_int32   *tab8;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleToGray6");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = (ws / 6) & 0xfffffff8;    /* truncate to factor of 8 */
    hd = hs / 6;
    if (wd == 0 || hd == 0)
        return (PIX *)ERROR_PTR("pixs too small", procName, NULL);

    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 0.16667, 0.16667);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);

    if ((tab8 = makePixelSumTab8()) == NULL)
        return (PIX *)ERROR_PTR("tab8 not made", procName, NULL);
    if ((valtab = makeValTabSG6()) == NULL)
        return (PIX *)ERROR_PTR("valtab not made", procName, NULL);

    scaleToGray6Low(datad, wd, hd, wpld, datas, wpls, tab8, valtab);

    FREE(tab8);
    FREE(valtab);
    return pixd;
}


/*!
 *  pixScaleToGray8()
 *
 *      Input:  pixs (1 bpp)
 *      Return: pixd (8 bpp), scaled down by 8x in each direction,
 *              or null on error
 */
PIX *
pixScaleToGray8(PIX  *pixs)
{
l_uint8   *valtab;
l_int32    ws, hs, wd, hd;
l_int32    wpld, wpls;
l_int32   *tab8;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleToGray8");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs must be 1 bpp", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = ws / 8;  /* truncate to nearest dest byte */
    hd = hs / 8;
    if (wd == 0 || hd == 0)
        return (PIX *)ERROR_PTR("pixs too small", procName, NULL);

    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 0.125, 0.125);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);

    if ((tab8 = makePixelSumTab8()) == NULL)
        return (PIX *)ERROR_PTR("tab8 not made", procName, NULL);
    if ((valtab = makeValTabSG8()) == NULL)
        return (PIX *)ERROR_PTR("valtab not made", procName, NULL);

    scaleToGray8Low(datad, wd, hd, wpld, datas, wpls, tab8, valtab);

    FREE(tab8);
    FREE(valtab);
    return pixd;
}


/*!
 *  pixScaleToGray16()
 *
 *      Input:  pixs (1 bpp)
 *      Return: pixd (8 bpp), scaled down by 16x in each direction,
 *              or null on error.
 */
PIX *
pixScaleToGray16(PIX  *pixs)
{
l_int32    ws, hs, wd, hd;
l_int32    wpld, wpls;
l_int32   *tab8;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixScaleToGray16");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs must be 1 bpp", procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = ws / 16;
    hd = hs / 16;
    if (wd == 0 || hd == 0)
        return (PIX *)ERROR_PTR("pixs too small", procName, NULL);

    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 0.0625, 0.0625);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);

    if ((tab8 = makePixelSumTab8()) == NULL)
        return (PIX *)ERROR_PTR("tab8 not made", procName, NULL);

    scaleToGray16Low(datad, wd, hd, wpld, datas, wpls, tab8);

    FREE(tab8);
    return pixd;
}


/*------------------------------------------------------------------*
 *    Scale-to-gray mipmap(1 bpp --> 8 bpp, arbitrary reduction)    *
 *------------------------------------------------------------------*/
/*!
 *  pixScaleToGrayMipmap()
 *
 *      Input:  pixs (1 bpp)
 *              scalefactor (reduction: must be > 0.0 and < 1.0)
 *      Return: pixd (8 bpp), scaled down by scalefactor in each direction,
 *              or NULL on error.
 *
 *  Notes:
 *
 *  This function is here mainly for pedagogical reasons.
 *  Mip-mapping is widely used in graphics for texture mapping, because
 *  the texture changes smoothly with scale.  This is accomplished by
 *  constructing a multiresolution pyramid and, for each pixel,
 *  doing a linear interpolation between corresponding pixels in
 *  the two planes of the pyramid that bracket the desired resolution.
 *  The computation is very efficient, and is implemented in hardware
 *  in high-end graphics cards.
 *
 *  We can use mip-mapping for scale-to-gray by using two scale-to-gray
 *  reduced images (we don't need the entire pyramid) selected from
 *  the set {2x, 4x, ... 16x}, and interpolating.  However, we get
 *  severe aliasing, probably because we are subsampling from the
 *  higher resolution image.  The method is very fast, but the result
 *  is very poor.  In fact, the results don't look any better than
 *  either subsampling off the higher-res grayscale image or oversampling
 *  on the lower-res image.  Consequently, this method should NOT be used
 *  for generating reduced images, scale-to-gray or otherwise.
 */
PIX *
pixScaleToGrayMipmap(PIX       *pixs,
                     l_float32  scalefactor)
{
l_int32    w, h, minsrc, mindest;
l_float32  red;
PIX       *pixs1, *pixs2, *pixt, *pixd;

    PROCNAME("pixScaleToGrayMipmap");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);
    if (scalefactor <= 0.0)
        return (PIX *)ERROR_PTR("scalefactor <= 0.0", procName, NULL);
    if (scalefactor >= 1.0)
        return (PIX *)ERROR_PTR("scalefactor >= 1.0", procName, NULL);
    pixGetDimensions(pixs, &w, &h, NULL);
    minsrc = L_MIN(w, h);
    mindest = (l_int32)((l_float32)minsrc * scalefactor);
    if (mindest < 2)
        return (PIX *)ERROR_PTR("scalefactor too small", procName, NULL);

    if (scalefactor > 0.5) {
        pixs1 = pixConvert1To8(NULL, pixs, 255, 0);
        pixs2 = pixScaleToGray2(pixs);
        red = scalefactor;
    } else if (scalefactor == 0.5) {
        return pixScaleToGray2(pixs);
    } else if (scalefactor > 0.25) {
        pixs1 = pixScaleToGray2(pixs);
        pixs2 = pixScaleToGray4(pixs);
        red = 2. * scalefactor;
    } else if (scalefactor == 0.25) {
        return pixScaleToGray4(pixs);
    } else if (scalefactor > 0.125) {
        pixs1 = pixScaleToGray4(pixs);
        pixs2 = pixScaleToGray8(pixs);
        red = 4. * scalefactor;
    } else if (scalefactor == 0.125) {
        return pixScaleToGray8(pixs);
    } else if (scalefactor > 0.0625) {
        pixs1 = pixScaleToGray8(pixs);
        pixs2 = pixScaleToGray16(pixs);
        red = 8. * scalefactor;
    } else if (scalefactor == 0.0625) {
        return pixScaleToGray16(pixs);
    } else {  /* end of the pyramid; just do it */
        red = 16.0 * scalefactor;  /* will be <= 1.0 */
        if ((pixt = pixScaleToGray16(pixs)) == NULL)
            return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
        if (red < 0.7)
            pixd = pixScaleSmooth(pixt, red, red);
        else
            pixd = pixScaleGrayLI(pixt, red, red);
        pixDestroy(&pixt);
        return pixd;
    }

    pixd = pixScaleMipmap(pixs1, pixs2, red);

    pixDestroy(&pixs1);
    pixDestroy(&pixs2);
    return pixd;
}


/*------------------------------------------------------------------*
 *                  Grayscale scaling using mipmap                  *
 *------------------------------------------------------------------*/
/*!
 *  pixScaleMipmap()
 *
 *      Input:  pixs1 (high res 8 bpp, no cmap)
 *              pixs2 (low res -- 2x reduced -- 8 bpp, no cmap)
 *              scale (reduction with respect to high res image, > 0.5)
 *      Return: 8 bpp pix, scaled down by reduction in each direction,
 *              or NULL on error.
 *
 *  Notes:
 *      (1) See notes in pixScaleToGrayMipmap().
 *      (2) This function suffers from aliasing effects that are
 *          easily seen in document images.
 */
PIX *
pixScaleMipmap(PIX       *pixs1,
               PIX       *pixs2,
               l_float32  scale)
{
l_int32    ws1, hs1, ws2, hs2, wd, hd, wpls1, wpls2, wpld;
l_uint32  *datas1, *datas2, *datad;
PIX       *pixd;

    PROCNAME("pixScaleMipmap");

    if (!pixs1 || pixGetDepth(pixs1) != 8 || pixGetColormap(pixs1))
        return (PIX *)ERROR_PTR("pixs1 underdefined, not 8 bpp, or cmapped",
                                procName, NULL);
    if (!pixs2 || pixGetDepth(pixs2) != 8 || pixGetColormap(pixs2))
        return (PIX *)ERROR_PTR("pixs2 underdefined, not 8 bpp, or cmapped",
                                procName, NULL);
    pixGetDimensions(pixs1, &ws1, &hs1, NULL);
    pixGetDimensions(pixs2, &ws2, &hs2, NULL);
    if (scale > 1.0 || scale < 0.5)
        return (PIX *)ERROR_PTR("scale not in [0.5, 1.0]", procName, NULL);
    if (ws1 < 2 * ws2)
        return (PIX *)ERROR_PTR("invalid width ratio", procName, NULL);
    if (hs1 < 2 * hs2)
        return (PIX *)ERROR_PTR("invalid height ratio", procName, NULL);

        /* Generate wd and hd from the lower resolution dimensions,
         * to guarantee staying within both src images */
    datas1 = pixGetData(pixs1);
    wpls1 = pixGetWpl(pixs1);
    datas2 = pixGetData(pixs2);
    wpls2 = pixGetWpl(pixs2);
    wd = (l_int32)(2. * scale * pixGetWidth(pixs2));
    hd = (l_int32)(2. * scale * pixGetHeight(pixs2));
    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs1);
    pixScaleResolution(pixd, scale, scale);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    scaleMipmapLow(datad, wd, hd, wpld, datas1, wpls1, datas2, wpls2, scale);
    return pixd;
}


/*------------------------------------------------------------------*
 *                  Replicated (integer) expansion                  *
 *------------------------------------------------------------------*/
/*!
 *  pixExpandReplicate()
 *
 *      Input:  pixs (1, 2, 4, 8, 16, 32 bpp)
 *              factor (integer scale factor for replicative expansion)
 *      Return: pixd (scaled up), or null on error.
 */
PIX *
pixExpandReplicate(PIX     *pixs,
                   l_int32  factor)
{
l_int32    w, h, d, wd, hd, wpls, wpld, start, i, j, k;
l_uint8    sval;
l_uint16   sval16;
l_uint32   sval32;
l_uint32  *lines, *datas, *lined, *datad;
PIX       *pixd;

    PROCNAME("pixExpandReplicate");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("depth not in {1,2,4,8,16,32}", procName, NULL);
    if (factor <= 0)
        return (PIX *)ERROR_PTR("factor <= 0; invalid", procName, NULL);
    if (factor == 1)
        return pixCopy(NULL, pixs);

    if (d == 1)
        return pixExpandBinaryReplicate(pixs, factor);

    wd = factor * w;
    hd = factor * h;
    if ((pixd = pixCreate(wd, hd, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyColormap(pixd, pixs);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, (l_float32)factor, (l_float32)factor);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    switch (d) {
    case 2:
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + factor * i * wpld;
            for (j = 0; j < w; j++) {
                sval = GET_DATA_DIBIT(lines, j);
                start = factor * j;
                for (k = 0; k < factor; k++)
                    SET_DATA_DIBIT(lined, start + k, sval);
            }
            for (k = 1; k < factor; k++)
                memcpy(lined + k * wpld, lined, 4 * wpld);
        }
        break;
    case 4:
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + factor * i * wpld;
            for (j = 0; j < w; j++) {
                sval = GET_DATA_QBIT(lines, j);
                start = factor * j;
                for (k = 0; k < factor; k++)
                    SET_DATA_QBIT(lined, start + k, sval);
            }
            for (k = 1; k < factor; k++)
                memcpy(lined + k * wpld, lined, 4 * wpld);
        }
        break;
    case 8:
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + factor * i * wpld;
            for (j = 0; j < w; j++) {
                sval = GET_DATA_BYTE(lines, j);
                start = factor * j;
                for (k = 0; k < factor; k++)
                    SET_DATA_BYTE(lined, start + k, sval);
            }
            for (k = 1; k < factor; k++)
                memcpy(lined + k * wpld, lined, 4 * wpld);
        }
        break;
    case 16:
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + factor * i * wpld;
            for (j = 0; j < w; j++) {
                sval16 = GET_DATA_TWO_BYTES(lines, j);
                start = factor * j;
                for (k = 0; k < factor; k++)
                    SET_DATA_TWO_BYTES(lined, start + k, sval16);
            }
            for (k = 1; k < factor; k++)
                memcpy(lined + k * wpld, lined, 4 * wpld);
        }
        break;
    case 32:
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + factor * i * wpld;
            for (j = 0; j < w; j++) {
                sval32 = *(lines + j);
                start = factor * j;
                for (k = 0; k < factor; k++)
                    *(lined + start + k) = sval32;
            }
            for (k = 1; k < factor; k++)
                memcpy(lined + k * wpld, lined, 4 * wpld);
        }
        break;
    default:
        fprintf(stderr, "invalid depth\n");
    }

    if (d == 32 && pixGetSpp(pixs) == 4)
        pixScaleAndTransferAlpha(pixd, pixs, (l_float32)factor,
                                 (l_float32)factor);
    return pixd;
}


/*------------------------------------------------------------------*
 *                Scale 2x followed by binarization                 *
 *------------------------------------------------------------------*/
/*!
 *  pixScaleGray2xLIThresh()
 *
 *      Input:  pixs (8 bpp, not cmapped)
 *              thresh  (between 0 and 256)
 *      Return: pixd (1 bpp), or null on error
 *
 *  Notes:
 *      (1) This does 2x upscale on pixs, using linear interpolation,
 *          followed by thresholding to binary.
 *      (2) Buffers are used to avoid making a large grayscale image.
 */
PIX *
pixScaleGray2xLIThresh(PIX     *pixs,
                       l_int32  thresh)
{
l_int32    i, ws, hs, hsm, wd, hd, wpls, wplb, wpld;
l_uint32  *datas, *datad, *lines, *lined, *lineb;
PIX       *pixd;

    PROCNAME("pixScaleGray2xLIThresh");

    if (!pixs || pixGetDepth(pixs) != 8 || pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs undefined, not 8 bpp, or cmapped",
                                procName, NULL);
    if (thresh < 0 || thresh > 256)
        return (PIX *)ERROR_PTR("thresh must be in [0, ... 256]",
            procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = 2 * ws;
    hd = 2 * hs;
    hsm = hs - 1;
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);

        /* Make line buffer for 2 lines of virtual intermediate image */
    wplb = (wd + 3) / 4;
    if ((lineb = (l_uint32 *)CALLOC(2 * wplb, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("lineb not made", procName, NULL);

        /* Make dest binary image */
    if ((pixd = pixCreate(wd, hd, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 2.0, 2.0);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);

        /* Do all but last src line */
    for (i = 0; i < hsm; i++) {
        lines = datas + i * wpls;
        lined = datad + 2 * i * wpld;  /* do 2 dest lines at a time */
        scaleGray2xLILineLow(lineb, wplb, lines, ws, wpls, 0);
        thresholdToBinaryLineLow(lined, wd, lineb, 8, thresh);
        thresholdToBinaryLineLow(lined + wpld, wd, lineb + wplb, 8, thresh);
    }

        /* Do last src line */
    lines = datas + hsm * wpls;
    lined = datad + 2 * hsm * wpld;
    scaleGray2xLILineLow(lineb, wplb, lines, ws, wpls, 1);
    thresholdToBinaryLineLow(lined, wd, lineb, 8, thresh);
    thresholdToBinaryLineLow(lined + wpld, wd, lineb + wplb, 8, thresh);

    FREE(lineb);
    return pixd;
}


/*!
 *  pixScaleGray2xLIDither()
 *
 *      Input:  pixs (8 bpp, not cmapped)
 *      Return: pixd (1 bpp), or null on error
 *
 *  Notes:
 *      (1) This does 2x upscale on pixs, using linear interpolation,
 *          followed by Floyd-Steinberg dithering to binary.
 *      (2) Buffers are used to avoid making a large grayscale image.
 *          - Two line buffers are used for the src, required for the 2x
 *            LI upscale.
 *          - Three line buffers are used for the intermediate image.
 *            Two are filled with each 2xLI row operation; the third is
 *            needed because the upscale and dithering ops are out of sync.
 */
PIX *
pixScaleGray2xLIDither(PIX  *pixs)
{
l_int32    i, ws, hs, hsm, wd, hd, wpls, wplb, wpld;
l_uint32  *datas, *datad;
l_uint32  *lined;
l_uint32  *lineb;   /* 2 intermediate buffer lines */
l_uint32  *linebp;  /* 1 intermediate buffer line */
l_uint32  *bufs;    /* 2 source buffer lines */
PIX       *pixd;

    PROCNAME("pixScaleGray2xLIDither");

    if (!pixs || pixGetDepth(pixs) != 8 || pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs undefined, not 8 bpp, or cmapped",
                                procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = 2 * ws;
    hd = 2 * hs;
    hsm = hs - 1;
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);

        /* Make line buffers for 2 lines of src image */
    if ((bufs = (l_uint32 *)CALLOC(2 * wpls, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("bufs not made", procName, NULL);

        /* Make line buffer for 2 lines of virtual intermediate image */
    wplb = (wd + 3) / 4;
    if ((lineb = (l_uint32 *)CALLOC(2 * wplb, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("lineb not made", procName, NULL);

        /* Make line buffer for 1 line of virtual intermediate image */
    if ((linebp = (l_uint32 *)CALLOC(wplb, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("linebp not made", procName, NULL);

        /* Make dest binary image */
    if ((pixd = pixCreate(wd, hd, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 2.0, 2.0);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);

        /* Start with the first src and the first dest line */
    memcpy(bufs, datas, 4 * wpls);   /* first src line */
    memcpy(bufs + wpls, datas + wpls, 4 * wpls);  /* 2nd src line */
    scaleGray2xLILineLow(lineb, wplb, bufs, ws, wpls, 0);  /* 2 i lines */
    lined = datad;
    ditherToBinaryLineLow(lined, wd, lineb, lineb + wplb,
                          DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
                                                    /* 1st d line */

        /* Do all but last src line */
    for (i = 1; i < hsm; i++) {
        memcpy(bufs, datas + i * wpls, 4 * wpls);  /* i-th src line */
        memcpy(bufs + wpls, datas + (i + 1) * wpls, 4 * wpls);
        memcpy(linebp, lineb + wplb, 4 * wplb);
        scaleGray2xLILineLow(lineb, wplb, bufs, ws, wpls, 0);  /* 2 i lines */
        lined = datad + 2 * i * wpld;
        ditherToBinaryLineLow(lined - wpld, wd, linebp, lineb,
                              DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
                                                   /* odd dest line */
        ditherToBinaryLineLow(lined, wd, lineb, lineb + wplb,
                              DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
                                                   /* even dest line */
    }

        /* Do the last src line and the last 3 dest lines */
    memcpy(bufs, datas + hsm * wpls, 4 * wpls);  /* hsm-th src line */
    memcpy(linebp, lineb + wplb, 4 * wplb);   /* 1 i line */
    scaleGray2xLILineLow(lineb, wplb, bufs, ws, wpls, 1);  /* 2 i lines */
    ditherToBinaryLineLow(lined + wpld, wd, linebp, lineb,
                          DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
                                                   /* odd dest line */
    ditherToBinaryLineLow(lined + 2 * wpld, wd, lineb, lineb + wplb,
                          DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
                                                   /* even dest line */
    ditherToBinaryLineLow(lined + 3 * wpld, wd, lineb + wplb, NULL,
                          DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 1);
                                                   /* last dest line */

    FREE(bufs);
    FREE(lineb);
    FREE(linebp);
    return pixd;
}


/*------------------------------------------------------------------*
 *                Scale 4x followed by binarization                 *
 *------------------------------------------------------------------*/
/*!
 *  pixScaleGray4xLIThresh()
 *
 *      Input:  pixs (8 bpp)
 *              thresh  (between 0 and 256)
 *      Return: pixd (1 bpp), or null on error
 *
 *  Notes:
 *      (1) This does 4x upscale on pixs, using linear interpolation,
 *          followed by thresholding to binary.
 *      (2) Buffers are used to avoid making a large grayscale image.
 *      (3) If a full 4x expanded grayscale image can be kept in memory,
 *          this function is only about 10% faster than separately doing
 *          a linear interpolation to a large grayscale image, followed
 *          by thresholding to binary.
 */
PIX *
pixScaleGray4xLIThresh(PIX     *pixs,
                       l_int32  thresh)
{
l_int32    i, j, ws, hs, hsm, wd, hd, wpls, wplb, wpld;
l_uint32  *datas, *datad, *lines, *lined, *lineb;
PIX       *pixd;

    PROCNAME("pixScaleGray4xLIThresh");

    if (!pixs || pixGetDepth(pixs) != 8 || pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs undefined, not 8 bpp, or cmapped",
                                procName, NULL);
    if (thresh < 0 || thresh > 256)
        return (PIX *)ERROR_PTR("thresh must be in [0, ... 256]",
            procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = 4 * ws;
    hd = 4 * hs;
    hsm = hs - 1;
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);

        /* Make line buffer for 4 lines of virtual intermediate image */
    wplb = (wd + 3) / 4;
    if ((lineb = (l_uint32 *)CALLOC(4 * wplb, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("lineb not made", procName, NULL);

        /* Make dest binary image */
    if ((pixd = pixCreate(wd, hd, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 4.0, 4.0);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);

        /* Do all but last src line */
    for (i = 0; i < hsm; i++) {
        lines = datas + i * wpls;
        lined = datad + 4 * i * wpld;  /* do 4 dest lines at a time */
        scaleGray4xLILineLow(lineb, wplb, lines, ws, wpls, 0);
        for (j = 0; j < 4; j++) {
            thresholdToBinaryLineLow(lined + j * wpld, wd,
                                     lineb + j * wplb, 8, thresh);
        }
    }

        /* Do last src line */
    lines = datas + hsm * wpls;
    lined = datad + 4 * hsm * wpld;
    scaleGray4xLILineLow(lineb, wplb, lines, ws, wpls, 1);
    for (j = 0; j < 4; j++) {
        thresholdToBinaryLineLow(lined + j * wpld, wd,
                                 lineb + j * wplb, 8, thresh);
    }

    FREE(lineb);
    return pixd;
}


/*!
 *  pixScaleGray4xLIDither()
 *
 *      Input:  pixs (8 bpp, not cmapped)
 *      Return: pixd (1 bpp), or null on error
 *
 *  Notes:
 *      (1) This does 4x upscale on pixs, using linear interpolation,
 *          followed by Floyd-Steinberg dithering to binary.
 *      (2) Buffers are used to avoid making a large grayscale image.
 *          - Two line buffers are used for the src, required for the
 *            4xLI upscale.
 *          - Five line buffers are used for the intermediate image.
 *            Four are filled with each 4xLI row operation; the fifth
 *            is needed because the upscale and dithering ops are
 *            out of sync.
 *      (3) If a full 4x expanded grayscale image can be kept in memory,
 *          this function is only about 5% faster than separately doing
 *          a linear interpolation to a large grayscale image, followed
 *          by error-diffusion dithering to binary.
 */
PIX *
pixScaleGray4xLIDither(PIX  *pixs)
{
l_int32    i, j, ws, hs, hsm, wd, hd, wpls, wplb, wpld;
l_uint32  *datas, *datad;
l_uint32  *lined;
l_uint32  *lineb;   /* 4 intermediate buffer lines */
l_uint32  *linebp;  /* 1 intermediate buffer line */
l_uint32  *bufs;    /* 2 source buffer lines */
PIX       *pixd;

    PROCNAME("pixScaleGray4xLIDither");

    if (!pixs || pixGetDepth(pixs) != 8 || pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs undefined, not 8 bpp, or cmapped",
                                procName, NULL);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = 4 * ws;
    hd = 4 * hs;
    hsm = hs - 1;
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);

        /* Make line buffers for 2 lines of src image */
    if ((bufs = (l_uint32 *)CALLOC(2 * wpls, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("bufs not made", procName, NULL);

        /* Make line buffer for 4 lines of virtual intermediate image */
    wplb = (wd + 3) / 4;
    if ((lineb = (l_uint32 *)CALLOC(4 * wplb, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("lineb not made", procName, NULL);

        /* Make line buffer for 1 line of virtual intermediate image */
    if ((linebp = (l_uint32 *)CALLOC(wplb, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("linebp not made", procName, NULL);

        /* Make dest binary image */
    if ((pixd = pixCreate(wd, hd, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, 4.0, 4.0);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);

        /* Start with the first src and the first 3 dest lines */
    memcpy(bufs, datas, 4 * wpls);   /* first src line */
    memcpy(bufs + wpls, datas + wpls, 4 * wpls);  /* 2nd src line */
    scaleGray4xLILineLow(lineb, wplb, bufs, ws, wpls, 0);  /* 4 b lines */
    lined = datad;
    for (j = 0; j < 3; j++) {  /* first 3 d lines of Q */
        ditherToBinaryLineLow(lined + j * wpld, wd, lineb + j * wplb,
                              lineb + (j + 1) * wplb,
                              DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
    }

        /* Do all but last src line */
    for (i = 1; i < hsm; i++) {
        memcpy(bufs, datas + i * wpls, 4 * wpls);  /* i-th src line */
        memcpy(bufs + wpls, datas + (i + 1) * wpls, 4 * wpls);
        memcpy(linebp, lineb + 3 * wplb, 4 * wplb);
        scaleGray4xLILineLow(lineb, wplb, bufs, ws, wpls, 0);  /* 4 b lines */
        lined = datad + 4 * i * wpld;
        ditherToBinaryLineLow(lined - wpld, wd, linebp, lineb,
                              DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
                                                     /* 4th dest line of Q */
        for (j = 0; j < 3; j++) {  /* next 3 d lines of Quad */
            ditherToBinaryLineLow(lined + j * wpld, wd, lineb + j * wplb,
                                  lineb + (j + 1) * wplb,
                                 DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
        }
    }

        /* Do the last src line and the last 5 dest lines */
    memcpy(bufs, datas + hsm * wpls, 4 * wpls);  /* hsm-th src line */
    memcpy(linebp, lineb + 3 * wplb, 4 * wplb);   /* 1 b line */
    scaleGray4xLILineLow(lineb, wplb, bufs, ws, wpls, 1);  /* 4 b lines */
    lined = datad + 4 * hsm * wpld;
    ditherToBinaryLineLow(lined - wpld, wd, linebp, lineb,
                          DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
                                                   /* 4th dest line of Q */
    for (j = 0; j < 3; j++) {  /* next 3 d lines of Quad */
        ditherToBinaryLineLow(lined + j * wpld, wd, lineb + j * wplb,
                              lineb + (j + 1) * wplb,
                              DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 0);
    }
        /* And finally, the last dest line */
    ditherToBinaryLineLow(lined + 3 * wpld, wd, lineb + 3 * wplb, NULL,
                              DEFAULT_CLIP_LOWER_1, DEFAULT_CLIP_UPPER_1, 1);

    FREE(bufs);
    FREE(lineb);
    FREE(linebp);
    return pixd;
}


/*-----------------------------------------------------------------------*
 *                    Downscaling using min or max                       *
 *-----------------------------------------------------------------------*/
/*!
 *  pixScaleGrayMinMax()
 *
 *      Input:  pixs (8 bpp, not cmapped)
 *              xfact (x downscaling factor; integer)
 *              yfact (y downscaling factor; integer)
 *              type (L_CHOOSE_MIN, L_CHOOSE_MAX, L_CHOOSE_MAX_MIN_DIFF)
 *      Return: pixd (8 bpp)
 *
 *  Notes:
 *      (1) The downscaled pixels in pixd are the min, max or (max - min)
 *          of the corresponding set of xfact * yfact pixels in pixs.
 *      (2) Using L_CHOOSE_MIN is equivalent to a grayscale erosion,
 *          using a brick Sel of size (xfact * yfact), followed by
 *          subsampling within each (xfact * yfact) cell.  Using
 *          L_CHOOSE_MAX is equivalent to the corresponding dilation.
 *      (3) Using L_CHOOSE_MAX_MIN_DIFF finds the difference between max
 *          and min values in each cell.
 *      (4) For the special case of downscaling by 2x in both directions,
 *          pixScaleGrayMinMax2() is about 2x more efficient.
 */
PIX *
pixScaleGrayMinMax(PIX     *pixs,
                   l_int32  xfact,
                   l_int32  yfact,
                   l_int32  type)
{
l_int32    ws, hs, wd, hd, wpls, wpld, i, j, k, m;
l_int32    minval, maxval, val;
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixScaleGrayMinMax");

    if (!pixs || pixGetDepth(pixs) != 8 || pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs undefined, not 8 bpp, or cmapped",
                                procName, NULL);
    pixGetDimensions(pixs, &ws, &hs, NULL);
    if (type != L_CHOOSE_MIN && type != L_CHOOSE_MAX &&
        type != L_CHOOSE_MAX_MIN_DIFF)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);
    if (xfact < 1 || yfact < 1)
        return (PIX *)ERROR_PTR("xfact and yfact must be >= 1", procName, NULL);

    if (xfact == 2 && yfact == 2)
        return pixScaleGrayMinMax2(pixs, type);

    wd = ws / xfact;
    if (wd == 0) {  /* single tile */
        wd = 1;
        xfact = ws;
    }
    hd = hs / yfact;
    if (hd == 0) {  /* single tile */
        hd = 1;
        yfact = hs;
    }
    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < hd; i++) {
        lined = datad + i * wpld;
        for (j = 0; j < wd; j++) {
            if (type == L_CHOOSE_MIN || type == L_CHOOSE_MAX_MIN_DIFF) {
                minval = 255;
                for (k = 0; k < yfact; k++) {
                    lines = datas + (yfact * i + k) * wpls;
                    for (m = 0; m < xfact; m++) {
                        val = GET_DATA_BYTE(lines, xfact * j + m);
                        if (val < minval)
                            minval = val;
                    }
                }
            }
            if (type == L_CHOOSE_MAX || type == L_CHOOSE_MAX_MIN_DIFF) {
                maxval = 0;
                for (k = 0; k < yfact; k++) {
                    lines = datas + (yfact * i + k) * wpls;
                    for (m = 0; m < xfact; m++) {
                        val = GET_DATA_BYTE(lines, xfact * j + m);
                        if (val > maxval)
                            maxval = val;
                    }
                }
            }
            if (type == L_CHOOSE_MIN)
                SET_DATA_BYTE(lined, j, minval);
            else if (type == L_CHOOSE_MAX)
                SET_DATA_BYTE(lined, j, maxval);
            else  /* type == L_CHOOSE_MAX_MIN_DIFF */
                SET_DATA_BYTE(lined, j, maxval - minval);
        }
    }

    return pixd;
}


/*!
 *  pixScaleGrayMinMax2()
 *
 *      Input:  pixs (8 bpp, not cmapped)
 *              type (L_CHOOSE_MIN, L_CHOOSE_MAX, L_CHOOSE_MAX_MIN_DIFF)
 *      Return: pixd (8 bpp downscaled by 2x)
 *
 *  Notes:
 *      (1) Special version for 2x reduction.  The downscaled pixels
 *          in pixd are the min, max or (max - min) of the corresponding
 *          set of 4 pixels in pixs.
 *      (2) The max and min operations are a special case (for levels 1
 *          and 4) of grayscale analog to the binary rank scaling operation
 *          pixReduceRankBinary2().  Note, however, that because of
 *          the photometric definition that higher gray values are
 *          lighter, the erosion-like L_CHOOSE_MIN will darken
 *          the resulting image, corresponding to a threshold level 1
 *          in the binary case.  Likewise, L_CHOOSE_MAX will lighten
 *          the pixd, corresponding to a threshold level of 4.
 *      (3) To choose any of the four rank levels in a 2x grayscale
 *          reduction, use pixScaleGrayRank2().
 *      (4) This runs at about 70 MPix/sec/GHz of source data for
 *          erosion and dilation.
 */
PIX *
pixScaleGrayMinMax2(PIX     *pixs,
                    l_int32  type)
{
l_int32    ws, hs, wd, hd, wpls, wpld, i, j, k;
l_int32    minval, maxval;
l_int32    val[4];
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixScaleGrayMinMax2");

    if (!pixs || pixGetDepth(pixs) != 8 || pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs undefined, not 8 bpp, or cmapped",
                                procName, NULL);
    pixGetDimensions(pixs, &ws, &hs, NULL);
    if (ws < 2 || hs < 2)
        return (PIX *)ERROR_PTR("too small: ws < 2 or hs < 2", procName, NULL);
    if (type != L_CHOOSE_MIN && type != L_CHOOSE_MAX &&
        type != L_CHOOSE_MAX_MIN_DIFF)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);

    wd = ws / 2;
    hd = hs / 2;
    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < hd; i++) {
        lines = datas + 2 * i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < wd; j++) {
            val[0] = GET_DATA_BYTE(lines, 2 * j);
            val[1] = GET_DATA_BYTE(lines, 2 * j + 1);
            val[2] = GET_DATA_BYTE(lines + wpls, 2 * j);
            val[3] = GET_DATA_BYTE(lines + wpls, 2 * j + 1);
            if (type == L_CHOOSE_MIN || type == L_CHOOSE_MAX_MIN_DIFF) {
                minval = 255;
                for (k = 0; k < 4; k++) {
                    if (val[k] < minval)
                        minval = val[k];
                }
            }
            if (type == L_CHOOSE_MAX || type == L_CHOOSE_MAX_MIN_DIFF) {
                maxval = 0;
                for (k = 0; k < 4; k++) {
                    if (val[k] > maxval)
                        maxval = val[k];
                }
            }
            if (type == L_CHOOSE_MIN)
                SET_DATA_BYTE(lined, j, minval);
            else if (type == L_CHOOSE_MAX)
                SET_DATA_BYTE(lined, j, maxval);
            else  /* type == L_CHOOSE_MAX_MIN_DIFF */
                SET_DATA_BYTE(lined, j, maxval - minval);
        }
    }

    return pixd;
}


/*-----------------------------------------------------------------------*
 *                  Grayscale downscaling using rank value               *
 *-----------------------------------------------------------------------*/
/*!
 *  pixScaleGrayRankCascade()
 *
 *      Input:  pixs (8 bpp, not cmapped)
 *              level1, ... level4 (rank thresholds, in set {0, 1, 2, 3, 4})
 *      Return: pixd (8 bpp, downscaled by up to 16x)
 *
 *  Notes:
 *      (1) This performs up to four cascaded 2x rank reductions.
 *      (2) Use level = 0 to truncate the cascade.
 */
PIX *
pixScaleGrayRankCascade(PIX     *pixs,
                        l_int32  level1,
                        l_int32  level2,
                        l_int32  level3,
                        l_int32  level4)
{
PIX  *pixt1, *pixt2, *pixt3, *pixt4;

    PROCNAME("pixScaleGrayRankCascade");

    if (!pixs || pixGetDepth(pixs) != 8 || pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs undefined, not 8 bpp, or cmapped",
                                procName, NULL);
    if (level1 > 4 || level2 > 4 || level3 > 4 || level4 > 4)
        return (PIX *)ERROR_PTR("levels must not exceed 4", procName, NULL);

    if (level1 <= 0) {
        L_WARNING("no reduction because level1 not > 0\n", procName);
        return pixCopy(NULL, pixs);
    }

    pixt1 = pixScaleGrayRank2(pixs, level1);
    if (level2 <= 0)
        return pixt1;

    pixt2 = pixScaleGrayRank2(pixt1, level2);
    pixDestroy(&pixt1);
    if (level3 <= 0)
        return pixt2;

    pixt3 = pixScaleGrayRank2(pixt2, level3);
    pixDestroy(&pixt2);
    if (level4 <= 0)
        return pixt3;

    pixt4 = pixScaleGrayRank2(pixt3, level4);
    pixDestroy(&pixt3);
    return pixt4;
}


/*!
 *  pixScaleGrayRank2()
 *
 *      Input:  pixs (8 bpp, no cmap)
 *              rank (1 (darkest), 2, 3, 4 (lightest))
 *      Return: pixd (8 bpp, downscaled by 2x)
 *
 *  Notes:
 *      (1) Rank 2x reduction.  If rank == 1(4), the downscaled pixels
 *          in pixd are the min(max) of the corresponding set of
 *          4 pixels in pixs.  Values 2 and 3 are intermediate.
 *      (2) This is the grayscale analog to the binary rank scaling operation
 *          pixReduceRankBinary2().  Here, because of the photometric
 *          definition that higher gray values are lighter, rank 1 gives
 *          the darkest pixel, whereas rank 4 gives the lightest pixel.
 *          This is opposite to the binary rank operation.
 *      (3) For rank = 1 and 4, this calls pixScaleGrayMinMax2(),
 *          which runs at about 70 MPix/sec/GHz of source data.
 *          For rank 2 and 3, this runs 3x slower, at about 25 MPix/sec/GHz.
 */
PIX *
pixScaleGrayRank2(PIX     *pixs,
                  l_int32  rank)
{
l_int32    ws, hs, wd, hd, wpls, wpld, i, j, k, m;
l_int32    minval, maxval, rankval, minindex, maxindex;
l_int32    val[4];
l_int32    midval[4];  /* should only use 2 of these */
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixScaleGrayRank2");

    if (!pixs || pixGetDepth(pixs) != 8 || pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs undefined, not 8 bpp, or cmapped",
                                procName, NULL);
    if (rank < 1 || rank > 4)
        return (PIX *)ERROR_PTR("invalid rank", procName, NULL);

    if (rank == 1)
        return pixScaleGrayMinMax2(pixs, L_CHOOSE_MIN);
    if (rank == 4)
        return pixScaleGrayMinMax2(pixs, L_CHOOSE_MAX);

    pixGetDimensions(pixs, &ws, &hs, NULL);
    wd = ws / 2;
    hd = hs / 2;
    if ((pixd = pixCreate(wd, hd, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    for (i = 0; i < hd; i++) {
        lines = datas + 2 * i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < wd; j++) {
            val[0] = GET_DATA_BYTE(lines, 2 * j);
            val[1] = GET_DATA_BYTE(lines, 2 * j + 1);
            val[2] = GET_DATA_BYTE(lines + wpls, 2 * j);
            val[3] = GET_DATA_BYTE(lines + wpls, 2 * j + 1);
            minval = maxval = val[0];
            minindex = maxindex = 0;
            for (k = 1; k < 4; k++) {
                if (val[k] < minval) {
                    minval = val[k];
                    minindex = k;
                    continue;
                }
                if (val[k] > maxval) {
                    maxval = val[k];
                    maxindex = k;
                }
            }
            for (k = 0, m = 0; k < 4; k++) {
                if (k == minindex || k == maxindex)
                    continue;
                midval[m++] = val[k];
            }
            if (m > 2)  /* minval == maxval; all val[k] are the same */
                rankval = minval;
            else if (rank == 2)
                rankval = L_MIN(midval[0], midval[1]);
            else  /* rank == 3 */
                rankval = L_MAX(midval[0], midval[1]);
            SET_DATA_BYTE(lined, j, rankval);
        }
    }

    return pixd;
}


/*------------------------------------------------------------------------*
 *           Helper function for transferring alpha with scaling          *
 *------------------------------------------------------------------------*/
/*!
 *  pixScaleAndTransferAlpha()
 *
 *      Input:  pixd  (32 bpp, scaled image)
 *              pixs  (32 bpp, original unscaled image)
 *              scalex, scaley (both > 0.0)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This scales the alpha component of pixs and inserts into pixd.
 */
l_int32
pixScaleAndTransferAlpha(PIX       *pixd,
                         PIX       *pixs,
                         l_float32  scalex,
                         l_float32  scaley)
{
PIX  *pix1, *pix2;

    PROCNAME("pixScaleAndTransferAlpha");

    if (!pixs || !pixd)
        return ERROR_INT("pixs and pixd not both defined", procName, 1);
    if (pixGetDepth(pixs) != 32 || pixGetSpp(pixs) != 4)
        return ERROR_INT("pixs not 32 bpp and 4 spp", procName, 1);
    if (pixGetDepth(pixd) != 32)
        return ERROR_INT("pixd not 32 bpp", procName, 1);

    if (scalex == 1.0 && scaley == 1.0) {
        pixCopyRGBComponent(pixd, pixs, L_ALPHA_CHANNEL);
        return 0;
    }

    pix1 = pixGetRGBComponent(pixs, L_ALPHA_CHANNEL);
    pix2 = pixScale(pix1, scalex, scaley);
    pixSetRGBComponent(pixd, pix2, L_ALPHA_CHANNEL);
    pixDestroy(&pix1);
    pixDestroy(&pix2);
    return 0;
}


/*------------------------------------------------------------------------*
 *    RGB scaling including alpha (blend) component and gamma transform   *
 *------------------------------------------------------------------------*/
/*!
 *  pixScaleWithAlpha()
 *
 *      Input:  pixs (32 bpp rgb or cmapped)
 *              scalex, scaley (must be > 0.0)
 *              pixg (<optional> 8 bpp, can be null)
 *              fract (between 0.0 and 1.0, with 0.0 fully transparent
 *                     and 1.0 fully opaque)
 *      Return: pixd (32 bpp rgba), or null on error
 *
 *  Notes:
 *      (1) The alpha channel is transformed separately from pixs,
 *          and aligns with it, being fully transparent outside the
 *          boundary of the transformed pixs.  For pixels that are fully
 *          transparent, a blending function like pixBlendWithGrayMask()
 *          will give zero weight to corresponding pixels in pixs.
 *      (2) Scaling is done with area mapping or linear interpolation,
 *          depending on the scale factors.  Default sharpening is done.
 *      (3) If pixg is NULL, it is generated as an alpha layer that is
 *          partially opaque, using @fract.  Otherwise, it is cropped
 *          to pixs if required, and @fract is ignored.  The alpha
 *          channel in pixs is never used.
 *      (4) Colormaps are removed to 32 bpp.
 *      (5) The default setting for the border values in the alpha channel
 *          is 0 (transparent) for the outermost ring of pixels and
 *          (0.5 * fract * 255) for the second ring.  When blended over
 *          a second image, this
 *          (a) shrinks the visible image to make a clean overlap edge
 *              with an image below, and
 *          (b) softens the edges by weakening the aliasing there.
 *          Use l_setAlphaMaskBorder() to change these values.
 *      (6) A subtle use of gamma correction is to remove gamma correction
 *          before scaling and restore it afterwards.  This is done
 *          by sandwiching this function between a gamma/inverse-gamma
 *          photometric transform:
 *              pixt = pixGammaTRCWithAlpha(NULL, pixs, 1.0 / gamma, 0, 255);
 *              pixd = pixScaleWithAlpha(pixt, scalex, scaley, NULL, fract);
 *              pixGammaTRCWithAlpha(pixd, pixd, gamma, 0, 255);
 *              pixDestroy(&pixt);
 *          This has the side-effect of producing artifacts in the very
 *          dark regions.
 *
 *  *** Warning: implicit assumption about RGB component ordering ***
 */
PIX *
pixScaleWithAlpha(PIX       *pixs,
                  l_float32  scalex,
                  l_float32  scaley,
                  PIX       *pixg,
                  l_float32  fract)
{
l_int32  ws, hs, d, spp;
PIX     *pixd, *pix32, *pixg2, *pixgs;

    PROCNAME("pixScaleWithAlpha");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &ws, &hs, &d);
    if (d != 32 && !pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs not cmapped or 32 bpp", procName, NULL);
    if (scalex <= 0.0 || scaley <= 0.0)
        return (PIX *)ERROR_PTR("scale factor <= 0.0", procName, NULL);
    if (pixg && pixGetDepth(pixg) != 8) {
        L_WARNING("pixg not 8 bpp; using @fract transparent alpha\n", procName);
        pixg = NULL;
    }
    if (!pixg && (fract < 0.0 || fract > 1.0)) {
        L_WARNING("invalid fract; using fully opaque\n", procName);
        fract = 1.0;
    }
    if (!pixg && fract == 0.0)
        L_WARNING("transparent alpha; image will not be blended\n", procName);

        /* Make sure input to scaling is 32 bpp rgb, and scale it */
    if (d != 32)
        pix32 = pixConvertTo32(pixs);
    else
        pix32 = pixClone(pixs);
    spp = pixGetSpp(pix32);
    pixSetSpp(pix32, 3);  /* ignore the alpha channel for scaling */
    pixd = pixScale(pix32, scalex, scaley);
    pixSetSpp(pix32, spp);  /* restore initial value in case it's a clone */
    pixDestroy(&pix32);

        /* Set up alpha layer with a fading border and scale it */
    if (!pixg) {
        pixg2 = pixCreate(ws, hs, 8);
        if (fract == 1.0)
            pixSetAll(pixg2);
        else if (fract > 0.0)
            pixSetAllArbitrary(pixg2, (l_int32)(255.0 * fract));
    } else {
        pixg2 = pixResizeToMatch(pixg, NULL, ws, hs);
    }
    if (ws > 10 && hs > 10) {  /* see note 4 */
        pixSetBorderRingVal(pixg2, 1,
                            (l_int32)(255.0 * fract * AlphaMaskBorderVals[0]));
        pixSetBorderRingVal(pixg2, 2,
                            (l_int32)(255.0 * fract * AlphaMaskBorderVals[1]));
    }
    pixgs = pixScaleGeneral(pixg2, scalex, scaley, 0.0, 0);

        /* Combine into a 4 spp result */
    pixSetRGBComponent(pixd, pixgs, L_ALPHA_CHANNEL);

    pixDestroy(&pixg2);
    pixDestroy(&pixgs);
    return pixd;
}

