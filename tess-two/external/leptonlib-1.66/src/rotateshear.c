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
 *  rotateshear.c
 *
 *      Shear rotation about arbitrary point using 2 and 3 shears
 *
 *              PIX      *pixRotateShear()
 *              PIX      *pixRotate2Shear()
 *              PIX      *pixRotate3Shear()
 *
 *      Shear rotation in-place about arbitrary point using 3 shears
 *              l_int32   pixRotateShearIP()
 *
 *      Shear rotation around the image center
 *              PIX      *pixRotateShearCenter()    (2 or 3 shears)
 *              l_int32   pixRotateShearCenterIP()  (3 shears)
 *
 *  Rotation is measured in radians; clockwise rotations are positive.
 *
 *  Rotation by shear works on images of any depth, 
 *  including 8 bpp color paletted images and 32 bpp
 *  rgb images.  It works by translating each src pixel
 *  value to the appropriate pixel in the rotated dest.
 *  For 8 bpp grayscale images, it is about 10-15x faster
 *  than rotation by area-mapping.
 *
 *  This speed and flexibility comes at the following cost,
 *  relative to area-mapped rotation:
 *
 *    -  Jaggies are created on edges of straight lines
 *
 *    -  For large angles, where you must use 3 shears,
 *       there is some extra clipping from the shears.
 *
 *  For small angles, typically less than 0.05 radians,
 *  rotation can be done with 2 orthogonal shears.
 *  Two such continuous shears (as opposed to the discrete
 *  shears on a pixel lattice that we have here) give
 *  a rotated image that has a distortion in the lengths
 *  of the two rotated and still-perpendicular axes.  The
 *  length/width ratio changes by a fraction 
 *
 *       0.5 * (angle)**2
 *
 *  For an angle of 0.05 radians, this is about 1 part in
 *  a thousand.  This distortion is absent when you use
 *  3 continuous shears with the correct angles (see below).
 *
 *  Of course, the image is on a discrete pixel lattice.
 *  Rotation by shear gives an approximation to a continuous
 *  rotation, leaving pixel jaggies at sharp boundaries.
 *  For very small rotations, rotating from a corner gives
 *  better sensitivity than rotating from the image center.
 *  Here's why.  Define the shear "center" to be the line such
 *  that the image is sheared in opposite directions on
 *  each side of and parallel to the line.  For small
 *  rotations there is a "dead space" on each side of the
 *  shear center of width equal to half the shear angle,
 *  in radians.  Thus, when the image is sheared about the center,
 *  the dead space width equals the shear angle, but when
 *  the image is sheared from a corner, the dead space
 *  width is only half the shear angle.
 *
 *  All horizontal and vertical shears are implemented by
 *  rasterop.  The in-place rotation uses special in-place
 *  shears that copy rows sideways or columns vertically
 *  without buffering, and then rewrite old pixels that are
 *  no longer covered by sheared pixels.  For that rewriting,
 *  you have the choice of using white or black pixels.
 *  (Note that this may give undesirable results for colormapped
 *  images, where the white and black values are arbitrary
 *  indexes into the colormap, and may not even exist.)
 *
 *  Rotation by shear is fast and depth-independent.  However, it
 *  does not work well for large rotation angles.  In fact, for
 *  rotation angles greater than about 7 degrees, more pixels are
 *  lost at the edges than when using pixRotationBySampling(), which
 *  only loses pixels because they are rotated out of the image. 
 *  For large rotations, use pixRotationBySampling() or, for
 *  more accuracy when d > 1 bpp, pixRotateAM().
 *
 *  For small angles, when comparing the quality of rotation by
 *  sampling and by shear, you can see that rotation by sampling
 *  is slightly more accurate.  However, the difference in
 *  accuracy of rotation by sampling when compared to 3-shear and
 *  (for angles less than 2 degrees, when compared to 2-shear) is
 *  less than 1 pixel at any point.  For very small angles, rotation by
 *  sampling is slower than rotation by shear.  The speed difference
 *  depends on the pixel depth and the rotation angle.  Rotation
 *  by shear is very fast for small angles and for small depth (esp. 1 bpp).
 *  Rotation by sampling speed is independent of angle and relatively
 *  more efficient for 8 and 32 bpp images.  Here are some timings
 *  for the ratio of rotation times: (time for sampling)/ (time for shear)
  *
 *       depth (bpp)       ratio (2 deg)       ratio (10 deg)
 *       -----------------------------------------------------
 *          1                  25                  6
 *          8                   5                  2.6
 *          32                  1.6                1.0
 *
 *  Consequently, for small angles and low bit depth, use rotation by shear.
 *  For large angles or large bit depth, use rotation by sampling.
 *
 *  There has been some work on what is called a "quasishear
 *  rotation" ("The Quasi-Shear Rotation, Eric Andres,
 *  DGCI 1996, pp. 307-314).  I believe they use a 3-shear
 *  approximation to the continuous rotation, exactly as
 *  we do here.  The approximation is due to being on
 *  a square pixel lattice.  They also use integers to specify
 *  the rotation angle and center offset, but that makes
 *  little sense on a machine where you have a few GFLOPS
 *  and only a few hundred floating point operations to do (!) 
 *  They also allow subpixel specification of the center of
 *  rotation, which I haven't bothered with, and claim that
 *  better results are possible if each of the 4 quadrants is
 *  handled separately.
 * 
 *  But the bottom line is that for binary images, the quality
 *  of the simple 3-shear rotation is about as good as you can do,
 *  visually, without dithering the result.  The effect of dither
 *  is to break up the horizontal and vertical shear lines.
 *  It's a bit tricky to dither with block shears -- you have to
 *  dither the pixels on the block boundaries!
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "allheaders.h"

static const l_float32  VERY_SMALL_ANGLE = 0.001;  /* radians; ~0.06 degrees */
static const l_float32  MAX_2_SHEAR_ANGLE = 0.05;  /* radians; ~3 degrees    */


/*------------------------------------------------------------------*
 *                Rotations about an arbitrary point                *
 *------------------------------------------------------------------*/
/*!
 *  pixRotateShear()
 *
 *      Input:  pixs
 *              xcen (x value for which there is no horizontal shear)
 *              ycen (y value for which there is no vertical shear)
 *              angle (radians)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK);
 *      Return: pixd, or null on error.
 *
 *  Notes:
 *      (1) This rotates an image about the given point, using
 *          either 2 or 3 shears.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) This brings in 'incolor' pixels from outside the image.
 */
PIX *
pixRotateShear(PIX       *pixs,
               l_int32    xcen,
               l_int32    ycen,
               l_float32  angle,
               l_int32    incolor)
{
    PROCNAME("pixRotateShear");

    if (!pixs)
        return (PIX *)(PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return (PIX *)(PIX *)ERROR_PTR("invalid incolor value", procName, NULL);

    if (L_ABS(angle) < VERY_SMALL_ANGLE)
        return pixClone(pixs);

    if (L_ABS(angle) <= MAX_2_SHEAR_ANGLE)
        return pixRotate2Shear(pixs, xcen, ycen, angle, incolor);
    else
        return pixRotate3Shear(pixs, xcen, ycen, angle, incolor);

}


/*!
 *  pixRotate2Shear()
 *
 *      Input:  pixs
 *              xcen, ycen (center of rotation)
 *              angle (radians)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK);
 *      Return: pixd, or null on error.
 *
 *  Notes:
 *      (1) This rotates the image about the given point,
 *          using the 2-shear method.  It should only
 *          be used for angles smaller than MAX_2_SHEAR_ANGLE.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) 2-shear rotation by a specified angle is equivalent
 *          to the sequential transformations
 *             x' = x + tan(angle) * (y - ycen)     for x-shear
 *             y' = y + tan(angle) * (x - xcen)     for y-shear
 *      (4) Computation of tan(angle) is performed within the shear operation.
 *      (5) This brings in 'incolor' pixels from outside the image.
 */
PIX *
pixRotate2Shear(PIX       *pixs,
                l_int32    xcen,
                l_int32    ycen,
                l_float32  angle,
                l_int32    incolor)
{
PIX  *pixt, *pixd;

    PROCNAME("pixRotate2Shear");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return (PIX *)(PIX *)ERROR_PTR("invalid incolor value", procName, NULL);
    
    if (L_ABS(angle) < VERY_SMALL_ANGLE)
        return pixClone(pixs);

    if ((pixt = pixHShear(NULL, pixs, ycen, angle, incolor)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
    if ((pixd = pixVShear(NULL, pixt, xcen, angle, incolor)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixDestroy(&pixt);

    return pixd;
}


/*!
 *  pixRotate3Shear()
 *
 *      Input:  pixs
 *              xcen, ycen (center of rotation)
 *              angle (radians)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK);
 *      Return: pixd, or null on error.
 *
 *  Notes:
 *      (1) This rotates the image about the image center,
 *          using the 3-shear method.  It can be used for any angle, and
 *          should be used for angles larger than MAX_2_SHEAR_ANGLE.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) 3-shear rotation by a specified angle is equivalent
 *          to the sequential transformations
 *            y' = y + tan(angle/2) * (x - xcen)     for first y-shear
 *            x' = x + sin(angle) * (y - ycen)       for x-shear
 *            y' = y + tan(angle/2) * (x - xcen)     for second y-shear
 *      (4) Computation of tan(angle) is performed in the shear operations.
 *      (5) This brings in 'incolor' pixels from outside the image.
 *      (6) The algorithm was published by Alan Paeth: "A Fast Algorithm
 *          for General Raster Rotation," Graphics Interface '86,
 *          pp. 77-81, May 1986.  A description of the method, along with
 *          an implementation, can be found in Graphics Gems, p. 179,
 *          edited by Andrew Glassner, published by Academic Press, 1990.
 */
PIX *
pixRotate3Shear(PIX       *pixs,
                l_int32    xcen,
                l_int32    ycen,
                l_float32  angle,
                l_int32    incolor)
{
l_float32  hangle;
PIX              *pixt, *pixd;

    PROCNAME("pixRotate3Shear");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return (PIX *)(PIX *)ERROR_PTR("invalid incolor value", procName, NULL);
    
    if (L_ABS(angle) < VERY_SMALL_ANGLE)
        return pixClone(pixs);

    hangle = atan(sin(angle));
    if ((pixd = pixVShear(NULL, pixs, xcen, angle / 2., incolor)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    if ((pixt = pixHShear(NULL, pixd, ycen, hangle, incolor)) == NULL)
        return (PIX *)ERROR_PTR("pixt not made", procName, NULL);
    pixVShear(pixd, pixt, xcen, angle / 2., incolor);
    pixDestroy(&pixt);

    return pixd;
}


/*------------------------------------------------------------------*
 *             Rotations in-place about an arbitrary point          *
 *------------------------------------------------------------------*/
/*!
 *  pixRotateShearIP()
 *
 *      Input:  pixs (any depth; not colormapped)
 *              xcen, ycen (center of rotation)
 *              angle (radians)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This does an in-place rotation of the image
 *          about the image center, using the 3-shear method.
 *      (2) A positive angle gives a clockwise rotation.
 *      (3) 3-shear rotation by a specified angle is equivalent
 *          to the sequential transformations
 *            y' = y + tan(angle/2) * (x - xcen)      for first y-shear
 *            x' = x + sin(angle) * (y - ycen)        for x-shear
 *            y' = y + tan(angle/2) * (x - xcen)      for second y-shear
 *      (4) Computation of tan(angle) is performed in the shear operations.
 *      (5) This brings in 'incolor' pixels from outside the image.
 *      (6) The pix cannot be colormapped, because the in-place operation
 *          only blits in 0 or 1 bits, not an arbitrary colormap index.
 */
l_int32
pixRotateShearIP(PIX       *pixs,
                 l_int32    xcen,
                 l_int32    ycen,
                 l_float32  angle,
                 l_int32    incolor)
{
l_float32  hangle;

    PROCNAME("pixRotateShearIP");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return ERROR_INT("invalid value for incolor", procName, 1);
    if (pixGetColormap(pixs) != NULL)
        return ERROR_INT("pixs is colormapped", procName, 1);

    if (angle == 0.0)
        return 0;
    
    hangle = atan(sin(angle));
    pixHShearIP(pixs, ycen, angle / 2., incolor);
    pixVShearIP(pixs, xcen, hangle, incolor);
    pixHShearIP(pixs, ycen, angle / 2., incolor);

    return 0;
}


/*------------------------------------------------------------------*
 *                    Rotations about the image center              *
 *------------------------------------------------------------------*/
/*!
 *  pixRotateShearCenter()
 *
 *      Input:  pixs
 *              angle (radians)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK)
 *      Return: pixd, or null on error
 */
PIX *
pixRotateShearCenter(PIX       *pixs,
                     l_float32  angle,
                     l_int32    incolor)
{
    PROCNAME("pixRotateShearCenter");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    
    return pixRotateShear(pixs, pixGetWidth(pixs) / 2,
                          pixGetHeight(pixs) / 2, angle, incolor);
}


/*!
 *  pixRotateShearCenterIP()
 *
 *      Input:  pixs
 *              angle (radians)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixRotateShearCenterIP(PIX       *pixs,
                       l_float32  angle,
                       l_int32    incolor)
{
    PROCNAME("pixRotateShearCenterIP");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    
    return pixRotateShearIP(pixs, pixGetWidth(pixs) / 2,
                            pixGetHeight(pixs) / 2, angle, incolor);
}

