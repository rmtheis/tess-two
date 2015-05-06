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
 *  warper.c
 *
 *      High-level captcha interface
 *          PIX               *pixSimpleCaptcha()
 *
 *      Random sinusoidal warping
 *          PIX               *pixRandomHarmonicWarp()
 *
 *      Helper functions
 *          static l_float64  *generateRandomNumberArray()
 *          static l_int32     applyWarpTransform()
 *
 *      Version using a LUT for sin
 *          PIX               *pixRandomHarmonicWarpLUT()
 *          static l_int32     applyWarpTransformLUT()
 *          static l_int32     makeSinLUT()
 *          static l_float32   getSinFromLUT()
 *
 *      Stereoscopic warping
 *          PIX               *pixWarpStereoscopic()
 *
 *      Linear and quadratic horizontal stretching
 *          PIX               *pixStretchHorizontal()
 *          PIX               *pixStretchHorizontalSampled()
 *          PIX               *pixStretchHorizontalLI()
 *
 *      Quadratic vertical shear
 *          PIX               *pixQuadraticVShear()
 *          PIX               *pixQuadraticVShearSampled()
 *          PIX               *pixQuadraticVShearLI()
 *
 *      Stereo from a pair of images
 *          PIX               *pixStereoFromPair()
 */

#include <math.h>
#include "allheaders.h"

static l_float64 *generateRandomNumberArray(l_int32 size);
static l_int32 applyWarpTransform(l_float32 xmag, l_float32 ymag,
                                l_float32 xfreq, l_float32 yfreq,
                                l_float64 *randa, l_int32 nx, l_int32 ny,
                                l_int32 xp, l_int32 yp,
                                l_float32 *px, l_float32 *py);

#define  USE_SIN_TABLE    0

    /* Suggested input to pixStereoFromPair().  These are weighting
     * factors for input to the red channel from the left image. */
static const l_float32  L_DEFAULT_RED_WEIGHT   = 0.0;
static const l_float32  L_DEFAULT_GREEN_WEIGHT = 0.7;
static const l_float32  L_DEFAULT_BLUE_WEIGHT  = 0.3;


/*----------------------------------------------------------------------*
 *                High-level example captcha interface                  *
 *----------------------------------------------------------------------*/
/*!
 *  pixSimpleCaptcha()
 *
 *      Input:  pixs (8 bpp; no colormap)
 *              border (added white pixels on each side)
 *              nterms (number of x and y harmonic terms)
 *              seed (of random number generator)
 *              color (for colorizing; in 0xrrggbb00 format; use 0 for black)
 *              cmapflag (1 for colormap output; 0 for rgb)
 *      Return: pixd (8 bpp cmap or 32 bpp rgb), or null on error
 *
 *  Notes:
 *      (1) This uses typical default values for generating captchas.
 *          The magnitudes of the harmonic warp are typically to be
 *          smaller when more terms are used, even though the phases
 *          are random.  See, for example, prog/warptest.c.
 */
PIX *
pixSimpleCaptcha(PIX      *pixs,
                 l_int32   border,
                 l_int32   nterms,
                 l_uint32  seed,
                 l_uint32  color,
                 l_int32   cmapflag)
{
l_int32    k;
l_float32  xmag[] = {7.0, 5.0, 4.0, 3.0};
l_float32  ymag[] = {10.0, 8.0, 6.0, 5.0};
l_float32  xfreq[] = {0.12, 0.10, 0.10, 0.11};
l_float32  yfreq[] = {0.15, 0.13, 0.13, 0.11};
PIX       *pixg, *pixgb, *pixw, *pixd;

    PROCNAME("pixSimpleCaptcha");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (nterms < 1 || nterms > 4)
        return (PIX *)ERROR_PTR("nterms must be in {1,2,3,4}", procName, NULL);

    k = nterms - 1;
    pixg = pixConvertTo8(pixs, 0);
    pixgb = pixAddBorder(pixg, border, 255);
    pixw = pixRandomHarmonicWarp(pixgb, xmag[k], ymag[k], xfreq[k], yfreq[k],
                                 nterms, nterms, seed, 255);
    pixd = pixColorizeGray(pixw, color, cmapflag);

    pixDestroy(&pixg);
    pixDestroy(&pixgb);
    pixDestroy(&pixw);
    return pixd;
}


/*----------------------------------------------------------------------*
 *                     Random sinusoidal warping                        *
 *----------------------------------------------------------------------*/
/*!
 *  pixRandomHarmonicWarp()
 *
 *      Input:  pixs (8 bpp; no colormap)
 *              xmag, ymag (maximum magnitude of x and y distortion)
 *              xfreq, yfreq (maximum magnitude of x and y frequency)
 *              nx, ny (number of x and y harmonic terms)
 *              seed (of random number generator)
 *              grayval (color brought in from the outside;
 *                       0 for black, 255 for white)
 *      Return: pixd (8 bpp; no colormap), or null on error
 *
 *  Notes:
 *      (1) To generate the warped image p(x',y'), set up the transforms
 *          that are in getWarpTransform().  For each (x',y') in the
 *          dest, the warp function computes the originating location
 *          (x, y) in the src.  The differences (x - x') and (y - y')
 *          are given as a sum of products of sinusoidal terms.  Each
 *          term is multiplied by a maximum amplitude (in pixels), and the
 *          angle is determined by a frequency and phase, and depends
 *          on the (x', y') value of the dest.  Random numbers with
 *          a variable input seed are used to allow the warping to be
 *          unpredictable.  A linear interpolation is used to find
 *          the value for the source at (x, y); this value is written
 *          into the dest.
 *      (2) This can be used to generate 'captcha's, which are somewhat
 *          randomly distorted images of text.  A typical set of parameters
 *          for a captcha are:
 *                    xmag = 4.0     ymag = 6.0
 *                    xfreq = 0.10   yfreq = 0.13
 *                    nx = 3         ny = 3
 *          Other examples can be found in prog/warptest.c.
 */
PIX *
pixRandomHarmonicWarp(PIX       *pixs,
                      l_float32  xmag,
                      l_float32  ymag,
                      l_float32  xfreq,
                      l_float32  yfreq,
                      l_int32    nx,
                      l_int32    ny,
                      l_uint32   seed,
                      l_int32    grayval)
{
l_int32     w, h, d, i, j, wpls, wpld, val;
l_uint32   *datas, *datad, *lined;
l_float32   x, y;
l_float64  *randa;
PIX        *pixd;

    PROCNAME("pixRandomHarmonicWarp");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);

        /* Compute filter output at each location.  We iterate over
         * the destination pixels.  For each dest pixel, use the
         * warp function to compute the four source pixels that
         * contribute, at the location (x, y).  Each source pixel
         * is divided into 16 x 16 subpixels to get an approximate value. */
    srand(seed);
    randa = generateRandomNumberArray(5 * (nx + ny));
    pixd = pixCreateTemplate(pixs);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    for (i = 0; i < h; i++) {
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            applyWarpTransform(xmag, ymag, xfreq, yfreq, randa, nx, ny,
                               j, i, &x, &y);
            linearInterpolatePixelGray(datas, wpls, w, h, x, y, grayval, &val);
            SET_DATA_BYTE(lined, j, val);
        }
    }

    FREE(randa);
    return pixd;
}


/*----------------------------------------------------------------------*
 *                         Static helper functions                      *
 *----------------------------------------------------------------------*/
static l_float64 *
generateRandomNumberArray(l_int32  size)
{
l_int32     i;
l_float64  *randa;

    PROCNAME("generateRandomNumberArray");

    if ((randa = (l_float64 *)CALLOC(size, sizeof(l_float64))) == NULL)
        return (l_float64 *)ERROR_PTR("calloc fail for randa", procName, NULL);

        /* Return random values between 0.5 and 1.0 */
    for (i = 0; i < size; i++)
        randa[i] = 0.5 * (1.0 + (l_float64)rand() / (l_float64)RAND_MAX);
    return randa;
}


/*!
 *  applyWarpTransform()
 *
 *  Notes:
 *      (1) Uses the internal sin function.
 */
static l_int32
applyWarpTransform(l_float32   xmag,
                   l_float32   ymag,
                   l_float32   xfreq,
                   l_float32   yfreq,
                   l_float64  *randa,
                   l_int32     nx,
                   l_int32     ny,
                   l_int32     xp,
                   l_int32     yp,
                   l_float32  *px,
                   l_float32  *py)
{
l_int32    i;
l_float64  twopi, x, y, anglex, angley;

    twopi = 6.283185;
    for (i = 0, x = xp; i < nx; i++) {
        anglex = xfreq * randa[3 * i + 1] * xp + twopi * randa[3 * i + 2];
        angley = yfreq * randa[3 * i + 3] * yp + twopi * randa[3 * i + 4];
        x += xmag * randa[3 * i] * sin(anglex) * sin(angley);
    }
    for (i = nx, y = yp; i < nx + ny; i++) {
        angley = yfreq * randa[3 * i + 1] * yp + twopi * randa[3 * i + 2];
        anglex = xfreq * randa[3 * i + 3] * xp + twopi * randa[3 * i + 4];
        y += ymag * randa[3 * i] * sin(angley) * sin(anglex);
    }

    *px = (l_float32)x;
    *py = (l_float32)y;
    return 0;
}


#if  USE_SIN_TABLE
/*----------------------------------------------------------------------*
 *                       Version using a LUT for sin                    *
 *----------------------------------------------------------------------*/
static l_int32 applyWarpTransformLUT(l_float32 xmag, l_float32 ymag,
                                l_float32 xfreq, l_float32 yfreq,
                                l_float64 *randa, l_int32 nx, l_int32 ny,
                                l_int32 xp, l_int32 yp, l_float32 *lut,
                                l_int32 npts, l_float32 *px, l_float32 *py);
static l_int32 makeSinLUT(l_int32 npts, NUMA **pna);
static l_float32 getSinFromLUT(l_float32 *tab, l_int32 npts,
                               l_float32 radang);

/*!
 *  pixRandomHarmonicWarpLUT()
 *
 *      Input:  pixs (8 bpp; no colormap)
 *              xmag, ymag (maximum magnitude of x and y distortion)
 *              xfreq, yfreq (maximum magnitude of x and y frequency)
 *              nx, ny (number of x and y harmonic terms)
 *              seed (of random number generator)
 *              grayval (color brought in from the outside;
 *                       0 for black, 255 for white)
 *      Return: pixd (8 bpp; no colormap), or null on error
 *
 *  Notes:
 *      (1) See notes and inline comments in pixRandomHarmonicWarp().
 *          This version uses a LUT for the sin function.  It is not
 *          appreciably faster than using the built-in sin function,
 *          and is here for comparison only.
 */
PIX *
pixRandomHarmonicWarpLUT(PIX       *pixs,
                         l_float32  xmag,
                         l_float32  ymag,
                         l_float32  xfreq,
                         l_float32  yfreq,
                         l_int32    nx,
                         l_int32    ny,
                         l_uint32   seed,
                         l_int32    grayval)
{
l_int32     w, h, d, i, j, wpls, wpld, val, npts;
l_uint32   *datas, *datad, *lined;
l_float32   x, y;
l_float32  *lut;
l_float64  *randa;
NUMA       *na;
PIX        *pixd;

    PROCNAME("pixRandomHarmonicWarp");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8)
        return (PIX *)ERROR_PTR("pixs not 8 bpp", procName, NULL);

        /* Compute filter output at each location.  We iterate over
         * the destination pixels.  For each dest pixel, use the
         * warp function to compute the four source pixels that
         * contribute, at the location (x, y).  Each source pixel
         * is divided into 16 x 16 subpixels to get an approximate value. */
    srand(seed);
    randa = generateRandomNumberArray(5 * (nx + ny));
    pixd = pixCreateTemplate(pixs);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    npts = 100;
    makeSinLUT(npts, &na);
    lut = numaGetFArray(na, L_NOCOPY);
    for (i = 0; i < h; i++) {
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            applyWarpTransformLUT(xmag, ymag, xfreq, yfreq, randa, nx, ny,
                                  j, i, lut, npts, &x, &y);
            linearInterpolatePixelGray(datas, wpls, w, h, x, y, grayval, &val);
            SET_DATA_BYTE(lined, j, val);
        }
    }

    numaDestroy(&na);
    FREE(randa);
    return pixd;
}


/*!
 *  applyWarpTransformLUT()
 *
 *  Notes:
 *      (1) Uses an LUT for computing sin(theta).  There is little speed
 *          advantage to using the LUT.
 */
static l_int32
applyWarpTransformLUT(l_float32   xmag,
                      l_float32   ymag,
                      l_float32   xfreq,
                      l_float32   yfreq,
                      l_float64  *randa,
                      l_int32     nx,
                      l_int32     ny,
                      l_int32     xp,
                      l_int32     yp,
                      l_float32  *lut,
                      l_int32     npts,
                      l_float32  *px,
                      l_float32  *py)
{
l_int32    i;
l_float64  twopi, x, y, anglex, angley, sanglex, sangley;

    twopi = 6.283185;
    for (i = 0, x = xp; i < nx; i++) {
        anglex = xfreq * randa[3 * i + 1] * xp + twopi * randa[3 * i + 2];
        angley = yfreq * randa[3 * i + 3] * yp + twopi * randa[3 * i + 4];
        sanglex = getSinFromLUT(lut, npts, anglex);
        sangley = getSinFromLUT(lut, npts, angley);
        x += xmag * randa[3 * i] * sanglex * sangley;
    }
    for (i = nx, y = yp; i < nx + ny; i++) {
        angley = yfreq * randa[3 * i + 1] * yp + twopi * randa[3 * i + 2];
        anglex = xfreq * randa[3 * i + 3] * xp + twopi * randa[3 * i + 4];
        sanglex = getSinFromLUT(lut, npts, anglex);
        sangley = getSinFromLUT(lut, npts, angley);
        y += ymag * randa[3 * i] * sangley * sanglex;
    }

    *px = (l_float32)x;
    *py = (l_float32)y;
    return 0;
}


static l_int32
makeSinLUT(l_int32  npts,
           NUMA   **pna)
{
l_int32    i, n;
l_float32  delx, fval;
NUMA      *na;

    PROCNAME("makeSinLUT");

    if (!pna)
        return ERROR_INT("&na not defined", procName, 1);
    *pna = NULL;
    if (npts < 2)
        return ERROR_INT("npts < 2", procName, 1);
    n = 2 * npts + 1;
    na = numaCreate(n);
    *pna = na;
    delx = 3.14159265 / (l_float32)npts;
    numaSetParameters(na, 0.0, delx);
    for (i = 0; i < n / 2; i++)
         numaAddNumber(na, (l_float32)sin((l_float64)i * delx));
    for (i = 0; i < n / 2; i++) {
         numaGetFValue(na, i, &fval);
         numaAddNumber(na, -fval);
    }
    numaAddNumber(na, 0);

    return 0;
}


static l_float32
getSinFromLUT(l_float32  *tab,
              l_int32     npts,
              l_float32   radang)
{
l_int32    index;
l_float32  twopi, invtwopi, findex, diff;

        /* Restrict radang to [0, 2pi] */
    twopi = 6.283185;
    invtwopi = 0.1591549;
    if (radang < 0.0)
        radang += twopi * (1.0 - (l_int32)(-radang * invtwopi));
    else if (radang > 0.0)
        radang -= twopi * (l_int32)(radang * invtwopi);

        /* Interpolate */
    findex = (2.0 * (l_float32)npts) * (radang * invtwopi);
    index = (l_int32)findex;
    if (index == 2 * npts)
        return tab[index];
    diff = findex - index;
    return (1.0 - diff) * tab[index] + diff * tab[index + 1];
}
#endif  /* USE_SIN_TABLE */



/*---------------------------------------------------------------------------*
 *                          Stereoscopic warping                             *
 *---------------------------------------------------------------------------*/
/*!
 *  pixWarpStereoscopic()
 *
 *      Input:  pixs (any depth, colormap ok)
 *              zbend (horizontal separation in pixels of red and cyan
 *                    at the left and right sides, that gives rise to
 *                    quadratic curvature out of the image plane)
 *              zshiftt (uniform pixel translation difference between
 *                      red and cyan, that pushes the top of the image
 *                      plane away from the viewer (zshiftt > 0) or
 *                      towards the viewer (zshiftt < 0))
 *              zshiftb (uniform pixel translation difference between
 *                      red and cyan, that pushes the bottom of the image
 *                      plane away from the viewer (zshiftb > 0) or
 *                      towards the viewer (zshiftb < 0))
 *              ybendt (multiplicative parameter for in-plane vertical
 *                      displacement at the left or right edge at the top:
 *                        y = ybendt * (2x/w - 1)^2 )
 *              ybendb (same as ybendt, except at the left or right edge
 *                      at the bottom)
 *              redleft (1 if the red filter is on the left; 0 otherwise)
 *      Return: pixd (32 bpp), or null on error
 *
 *  Notes:
 *      (1) This function splits out the red channel, mucks around with
 *          it, then recombines with the unmolested cyan channel.
 *      (2) By using a quadratically increasing shift of the red
 *          pixels horizontally and away from the vertical centerline,
 *          the image appears to bend quadratically out of the image
 *          plane, symmetrically with respect to the vertical center
 *          line.  A positive value of @zbend causes the plane to be
 *          curved away from the viewer.  We use linearly interpolated
 *          stretching to avoid the appearance of kinks in the curve.
 *      (3) The parameters @zshiftt and @zshiftb tilt the image plane
 *          about a horizontal line through the center, and at the
 *          same time move that line either in toward the viewer or away.
 *          This is implemented by a combination of horizontal shear
 *          about the center line (for the tilt) and horizontal
 *          translation (to move the entire plane in or out).
 *          A positive value of @zshiftt moves the top of the plane
 *          away from the viewer, and a positive value of @zshiftb
 *          moves the bottom of the plane away.  We use linear interpolated
 *          shear to avoid visible vertical steps in the tilted image.
 *      (4) The image can be bent in the plane and about the vertical
 *          centerline.  The centerline does not shift, and the
 *          parameter @ybend gives the relative shift at left and right
 *          edges, with a downward shift for positive values of @ybend.
 *      (6) When writing out a steroscopic (red/cyan) image in jpeg,
 *          first call pixSetChromaSampling(pix, 0) to get sufficient
 *          resolution in the red channel.
 *      (7) Typical values are:
 *             zbend = 20
 *             zshiftt = 15
 *             zshiftb = -15
 *             ybendt = 30
 *             ybendb = 0
 *          If the disparity z-values are too large, it is difficult for
 *          the brain to register the two images.
 *      (8) This function has been cleverly reimplemented by Jeff Breidenbach.
 *          The original implementation used two 32 bpp rgb images,
 *          and merged them at the end.  The result is somewhat faded,
 *          and has a parameter "thresh" that controls the amount of
 *          color in the result.  (The present implementation avoids these
 *          two problems, skipping both the colorization and the alpha
 *          blending at the end, and is about 3x faster)
 *          The basic operations with 32 bpp are as follows:
 *               // Immediate conversion to 32 bpp
 *            Pix *pixt1 = pixConvertTo32(pixs);
 *               // Do vertical shear
 *            Pix *pixr = pixQuadraticVerticalShear(pixt1, L_WARP_TO_RIGHT,
 *                                                  ybendt, ybendb,
 *                                                  L_BRING_IN_WHITE);
 *               // Colorize two versions, toward red and cyan
 *            Pix *pixc = pixCopy(NULL, pixr);
 *            l_int32 thresh = 150;  // if higher, get less original color
 *            pixColorGray(pixr, NULL, L_PAINT_DARK, thresh, 255, 0, 0);
 *            pixColorGray(pixc, NULL, L_PAINT_DARK, thresh, 0, 255, 255);
 *               // Shift the red pixels; e.g., by stretching
 *            Pix *pixrs = pixStretchHorizontal(pixr, L_WARP_TO_RIGHT,
 *                                              L_QUADRATIC_WARP, zbend,
 *                                              L_INTERPOLATED,
 *                                              L_BRING_IN_WHITE);
 *               // Blend the shifted red and unshifted cyan 50:50
 *            Pix *pixg = pixCreate(w, h, 8);
 *            pixSetAllArbitrary(pixg, 128);
 *            pixd = pixBlendWithGrayMask(pixrs, pixc, pixg, 0, 0);
 */
PIX *
pixWarpStereoscopic(PIX     *pixs,
                    l_int32  zbend,
                    l_int32  zshiftt,
                    l_int32  zshiftb,
                    l_int32  ybendt,
                    l_int32  ybendb,
                    l_int32  redleft)
{
l_int32    w, h, zshift;
l_float32  angle;
BOX       *boxleft, *boxright;
PIX       *pixt, *pixt2, *pixt3;
PIX       *pixr, *pixg, *pixb;
PIX       *pixv1, *pixv2, *pixv3, *pixv4;
PIX       *pixr1, *pixr2, *pixr3, *pixr4, *pixrs, *pixrss;
PIX       *pixd;

    PROCNAME("pixWarpStereoscopic");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

        /* Convert to the output depth, 32 bpp. */
    pixt = pixConvertTo32(pixs);

        /* If requested, do a quad vertical shearing, pushing pixels up
         * or down, depending on their distance from the centerline. */
    pixGetDimensions(pixs, &w, &h, NULL);
    boxleft = boxCreate(0, 0, w / 2, h);
    boxright = boxCreate(w / 2, 0, w - w / 2, h);
    if (ybendt != 0 || ybendb != 0) {
        pixv1 = pixClipRectangle(pixt, boxleft, NULL);
        pixv2 = pixClipRectangle(pixt, boxright, NULL);
        pixv3 = pixQuadraticVShear(pixv1, L_WARP_TO_LEFT, ybendt,
                                          ybendb, L_INTERPOLATED,
                                          L_BRING_IN_WHITE);
        pixv4 = pixQuadraticVShear(pixv2, L_WARP_TO_RIGHT, ybendt,
                                          ybendb, L_INTERPOLATED,
                                          L_BRING_IN_WHITE);
        pixt2 = pixCreate(w, h, 32);
        pixRasterop(pixt2, 0, 0, w / 2, h, PIX_SRC, pixv3, 0, 0);
        pixRasterop(pixt2, w / 2, 0, w - w / 2, h, PIX_SRC, pixv4, 0, 0);
        pixDestroy(&pixv1);
        pixDestroy(&pixv2);
        pixDestroy(&pixv3);
        pixDestroy(&pixv4);
    } else {
        pixt2 = pixClone(pixt);
    }

        /* Split out the 3 components */
    pixr = pixGetRGBComponent(pixt2, COLOR_RED);
    pixg = pixGetRGBComponent(pixt2, COLOR_GREEN);
    pixb = pixGetRGBComponent(pixt2, COLOR_BLUE);
    pixDestroy(&pixt);
    pixDestroy(&pixt2);

        /* The direction of the stereo disparity below is set
         * for the red filter to be over the left eye.  If the red
         * filter is over the right eye, invert the horizontal shifts. */
    if (redleft) {
        zbend = -zbend;
        zshiftt = -zshiftt;
        zshiftb = -zshiftb;
    }

        /* Shift the red pixels horizontally by an amount that
         * increases quadratically from the centerline. */
    if (zbend == 0) {
        pixrs = pixClone(pixr);
    } else {
        pixr1 = pixClipRectangle(pixr, boxleft, NULL);
        pixr2 = pixClipRectangle(pixr, boxright, NULL);
        pixr3 = pixStretchHorizontal(pixr1, L_WARP_TO_LEFT, L_QUADRATIC_WARP,
                                     zbend, L_INTERPOLATED, L_BRING_IN_WHITE);
        pixr4 = pixStretchHorizontal(pixr2, L_WARP_TO_RIGHT, L_QUADRATIC_WARP,
                                     zbend, L_INTERPOLATED, L_BRING_IN_WHITE);
        pixrs = pixCreate(w, h, 8);
        pixRasterop(pixrs, 0, 0, w / 2, h, PIX_SRC, pixr3, 0, 0);
        pixRasterop(pixrs, w / 2, 0, w - w / 2, h, PIX_SRC, pixr4, 0, 0);
        pixDestroy(&pixr1);
        pixDestroy(&pixr2);
        pixDestroy(&pixr3);
        pixDestroy(&pixr4);
    }

        /* Perform a combination of horizontal shift and shear of
         * red pixels.  The causes the plane of the image to tilt and
         * also move forward or backward. */
    if (zshiftt == 0 && zshiftb == 0) {
        pixrss = pixClone(pixrs);
    } else if (zshiftt == zshiftb) {
        pixrss = pixTranslate(NULL, pixrs, zshiftt, 0, L_BRING_IN_WHITE);
    } else {
        angle = (l_float32)(zshiftb - zshiftt) / (l_float32)pixGetHeight(pixrs);
        zshift = (zshiftt + zshiftb) / 2;
        pixt3 = pixTranslate(NULL, pixrs, zshift, 0, L_BRING_IN_WHITE);
        pixrss = pixHShearLI(pixt3, h / 2, angle, L_BRING_IN_WHITE);
        pixDestroy(&pixt3);
    }

        /* Combine the unchanged cyan (g,b) image with the shifted red */
    pixd = pixCreateRGBImage(pixrss, pixg, pixb);

    boxDestroy(&boxleft);
    boxDestroy(&boxright);
    pixDestroy(&pixrs);
    pixDestroy(&pixrss);
    pixDestroy(&pixr);
    pixDestroy(&pixg);
    pixDestroy(&pixb);
    return pixd;
}


/*----------------------------------------------------------------------*
 *              Linear and quadratic horizontal stretching              *
 *----------------------------------------------------------------------*/
/*!
 *  pixStretchHorizontal()
 *
 *      Input:  pixs (1, 8 or 32 bpp)
 *              dir (L_WARP_TO_LEFT or L_WARP_TO_RIGHT)
 *              type (L_LINEAR_WARP or L_QUADRATIC_WARP)
 *              hmax (horizontal displacement at edge)
 *              operation (L_SAMPLED or L_INTERPOLATED)
 *              incolor (L_BRING_IN_WHITE or L_BRING_IN_BLACK)
 *      Return: pixd (stretched/compressed), or null on error
 *
 *  Notes:
 *      (1) If @hmax > 0, this is an increase in the coordinate value of
 *          pixels in pixd, relative to the same pixel in pixs.
 *      (2) If @dir == L_WARP_TO_LEFT, the pixels on the right edge of
 *          the image are not moved. So, for example, if @hmax > 0
 *          and @dir == L_WARP_TO_LEFT, the pixels in pixd are
 *          contracted toward the right edge of the image, relative
 *          to those in pixs.
 *      (3) If @type == L_LINEAR_WARP, the pixel positions are moved
 *          to the left or right by an amount that varies linearly with
 *          the horizontal location.
 *      (4) If @operation == L_SAMPLED, the dest pixels are taken from
 *          the nearest src pixel.  Otherwise, we use linear interpolation
 *          between pairs of sampled pixels.
 */
PIX *
pixStretchHorizontal(PIX     *pixs,
                     l_int32  dir,
                     l_int32  type,
                     l_int32  hmax,
                     l_int32  operation,
                     l_int32  incolor)
{
l_int32  d;

    PROCNAME("pixStretchHorizontal");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    if (d != 1 && d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pixs not 1, 8 or 32 bpp", procName, NULL);
    if (dir != L_WARP_TO_LEFT && dir != L_WARP_TO_RIGHT)
        return (PIX *)ERROR_PTR("invalid direction", procName, NULL);
    if (type != L_LINEAR_WARP && type != L_QUADRATIC_WARP)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);
    if (operation != L_SAMPLED && operation != L_INTERPOLATED)
        return (PIX *)ERROR_PTR("invalid operation", procName, NULL);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return (PIX *)ERROR_PTR("invalid incolor", procName, NULL);
    if (d == 1 && operation == L_INTERPOLATED) {
        L_WARNING("Using sampling for 1 bpp\n", procName);
        operation = L_INTERPOLATED;
    }

    if (operation == L_SAMPLED)
        return pixStretchHorizontalSampled(pixs, dir, type, hmax, incolor);
    else
        return pixStretchHorizontalLI(pixs, dir, type, hmax, incolor);
}


/*!
 *  pixStretchHorizontalSampled()
 *
 *      Input:  pixs (1, 8 or 32 bpp)
 *              dir (L_WARP_TO_LEFT or L_WARP_TO_RIGHT)
 *              type (L_LINEAR_WARP or L_QUADRATIC_WARP)
 *              hmax (horizontal displacement at edge)
 *              incolor (L_BRING_IN_WHITE or L_BRING_IN_BLACK)
 *      Return: pixd (stretched/compressed), or null on error
 *
 *  Notes:
 *      (1) See pixStretchHorizontal() for details.
 */
PIX *
pixStretchHorizontalSampled(PIX     *pixs,
                            l_int32  dir,
                            l_int32  type,
                            l_int32  hmax,
                            l_int32  incolor)
{
l_int32    i, j, jd, w, wm, h, d, wpls, wpld, val;
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixStretchHorizontalSampled");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pixs not 1, 8 or 32 bpp", procName, NULL);
    if (dir != L_WARP_TO_LEFT && dir != L_WARP_TO_RIGHT)
        return (PIX *)ERROR_PTR("invalid direction", procName, NULL);
    if (type != L_LINEAR_WARP && type != L_QUADRATIC_WARP)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return (PIX *)ERROR_PTR("invalid incolor", procName, NULL);

    pixd = pixCreateTemplate(pixs);
    pixSetBlackOrWhite(pixd, L_BRING_IN_WHITE);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    wm = w - 1;
    for (jd = 0; jd < w; jd++) {
        if (dir == L_WARP_TO_LEFT) {
            if (type == L_LINEAR_WARP)
                j = jd - (hmax * (wm - jd)) / wm;
            else  /* L_QUADRATIC_WARP */
                j = jd - (hmax * (wm - jd) * (wm - jd)) / (wm * wm);
        } else if (dir == L_WARP_TO_RIGHT) {
            if (type == L_LINEAR_WARP)
                j = jd - (hmax * jd) / wm;
            else  /* L_QUADRATIC_WARP */
                j = jd - (hmax * jd * jd) / (wm * wm);
        }
        if (j < 0 || j > w - 1) continue;

        switch (d)
        {
        case 1:
            for (i = 0; i < h; i++) {
                lines = datas + i * wpls;
                lined = datad + i * wpld;
                val = GET_DATA_BIT(lines, j);
                if (val)
                    SET_DATA_BIT(lined, jd);
            }
            break;
        case 8:
            for (i = 0; i < h; i++) {
                lines = datas + i * wpls;
                lined = datad + i * wpld;
                val = GET_DATA_BYTE(lines, j);
                SET_DATA_BYTE(lined, jd, val);
            }
            break;
        case 32:
            for (i = 0; i < h; i++) {
                lines = datas + i * wpls;
                lined = datad + i * wpld;
                lined[jd] = lines[j];
            }
            break;
        default:
            L_ERROR("invalid depth: %d\n", procName, d);
            pixDestroy(&pixd);
            return NULL;
        }
    }

    return pixd;
}


/*!
 *  pixStretchHorizontalLI()
 *
 *      Input:  pixs (1, 8 or 32 bpp)
 *              dir (L_WARP_TO_LEFT or L_WARP_TO_RIGHT)
 *              type (L_LINEAR_WARP or L_QUADRATIC_WARP)
 *              hmax (horizontal displacement at edge)
 *              incolor (L_BRING_IN_WHITE or L_BRING_IN_BLACK)
 *      Return: pixd (stretched/compressed), or null on error
 *
 *  Notes:
 *      (1) See pixStretchHorizontal() for details.
 */
PIX *
pixStretchHorizontalLI(PIX     *pixs,
                       l_int32  dir,
                       l_int32  type,
                       l_int32  hmax,
                       l_int32  incolor)
{
l_int32    i, j, jd, jp, jf, w, wm, h, d, wpls, wpld, val, rval, gval, bval;
l_uint32   word0, word1;
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixStretchHorizontalLI");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pixs not 8 or 32 bpp", procName, NULL);
    if (dir != L_WARP_TO_LEFT && dir != L_WARP_TO_RIGHT)
        return (PIX *)ERROR_PTR("invalid direction", procName, NULL);
    if (type != L_LINEAR_WARP && type != L_QUADRATIC_WARP)
        return (PIX *)ERROR_PTR("invalid type", procName, NULL);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return (PIX *)ERROR_PTR("invalid incolor", procName, NULL);

        /* Standard linear interpolation, subdividing each pixel into 64 */
    pixd = pixCreateTemplate(pixs);
    pixSetBlackOrWhite(pixd, L_BRING_IN_WHITE);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    wm = w - 1;
    for (jd = 0; jd < w; jd++) {
        if (dir == L_WARP_TO_LEFT) {
            if (type == L_LINEAR_WARP)
                j = 64 * jd - 64 * (hmax * (wm - jd)) / wm;
            else  /* L_QUADRATIC_WARP */
                j = 64 * jd - 64 * (hmax * (wm - jd) * (wm - jd)) / (wm * wm);
        } else if (dir == L_WARP_TO_RIGHT) {
            if (type == L_LINEAR_WARP)
                j = 64 * jd - 64 * (hmax * jd) / wm;
            else  /* L_QUADRATIC_WARP */
                j = 64 * jd - 64 * (hmax * jd * jd) / (wm * wm);
        }
        jp = j / 64;
        jf = j & 0x3f;
        if (jp < 0 || jp > wm) continue;

        switch (d)
        {
        case 8:
            if (jp < wm) {
                for (i = 0; i < h; i++) {
                    lines = datas + i * wpls;
                    lined = datad + i * wpld;
                    val = ((63 - jf) * GET_DATA_BYTE(lines, jp) +
                           jf * GET_DATA_BYTE(lines, jp + 1) + 31) / 63;
                    SET_DATA_BYTE(lined, jd, val);
                }
            } else {  /* jp == wm */
                for (i = 0; i < h; i++) {
                    lines = datas + i * wpls;
                    lined = datad + i * wpld;
                    val = GET_DATA_BYTE(lines, jp);
                    SET_DATA_BYTE(lined, jd, val);
                }
            }
            break;
        case 32:
            if (jp < wm) {
                for (i = 0; i < h; i++) {
                    lines = datas + i * wpls;
                    lined = datad + i * wpld;
                    word0 = *(lines + jp);
                    word1 = *(lines + jp + 1);
                    rval = ((63 - jf) * ((word0 >> L_RED_SHIFT) & 0xff) +
                           jf * ((word1 >> L_RED_SHIFT) & 0xff) + 31) / 63;
                    gval = ((63 - jf) * ((word0 >> L_GREEN_SHIFT) & 0xff) +
                           jf * ((word1 >> L_GREEN_SHIFT) & 0xff) + 31) / 63;
                    bval = ((63 - jf) * ((word0 >> L_BLUE_SHIFT) & 0xff) +
                           jf * ((word1 >> L_BLUE_SHIFT) & 0xff) + 31) / 63;
                    composeRGBPixel(rval, gval, bval, lined + jd);
                }
            } else {  /* jp == wm */
                for (i = 0; i < h; i++) {
                    lines = datas + i * wpls;
                    lined = datad + i * wpld;
                    lined[jd] = lines[jp];
                }
            }
            break;
        default:
            L_ERROR("invalid depth: %d\n", procName, d);
            pixDestroy(&pixd);
            return NULL;
        }
    }

    return pixd;
}


/*----------------------------------------------------------------------*
 *                       Quadratic vertical shear                       *
 *----------------------------------------------------------------------*/
/*!
 *  pixQuadraticVShear()
 *
 *      Input:  pixs (1, 8 or 32 bpp)
 *              dir (L_WARP_TO_LEFT or L_WARP_TO_RIGHT)
 *              vmaxt (max vertical displacement at edge and at top)
 *              vmaxb (max vertical displacement at edge and at bottom)
 *              operation (L_SAMPLED or L_INTERPOLATED)
 *              incolor (L_BRING_IN_WHITE or L_BRING_IN_BLACK)
 *      Return: pixd (stretched), or null on error
 *
 *  Notes:
 *      (1) This gives a quadratic bending, upward or downward, as you
 *          move to the left or right.
 *      (2) If @dir == L_WARP_TO_LEFT, the right edge is unchanged, and
 *          the left edge pixels are moved maximally up or down.
 *      (3) Parameters @vmaxt and @vmaxb control the maximum amount of
 *          vertical pixel shear at the top and bottom, respectively.
 *          If @vmaxt > 0, the vertical displacement of pixels at the
 *          top is downward.  Likewise, if @vmaxb > 0, the vertical
 *          displacement of pixels at the bottom is downward.
 *      (4) If @operation == L_SAMPLED, the dest pixels are taken from
 *          the nearest src pixel.  Otherwise, we use linear interpolation
 *          between pairs of sampled pixels.
 *      (5) This is for quadratic shear.  For uniform (linear) shear,
 *          use the standard shear operators.
 */
PIX *
pixQuadraticVShear(PIX     *pixs,
                   l_int32  dir,
                   l_int32  vmaxt,
                   l_int32  vmaxb,
                   l_int32  operation,
                   l_int32  incolor)
{
l_int32    w, h, d;

    PROCNAME("pixQuadraticVShear");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pixs not 1, 8 or 32 bpp", procName, NULL);
    if (dir != L_WARP_TO_LEFT && dir != L_WARP_TO_RIGHT)
        return (PIX *)ERROR_PTR("invalid direction", procName, NULL);
    if (operation != L_SAMPLED && operation != L_INTERPOLATED)
        return (PIX *)ERROR_PTR("invalid operation", procName, NULL);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return (PIX *)ERROR_PTR("invalid incolor", procName, NULL);

    if (vmaxt == 0 && vmaxb == 0)
        return pixCopy(NULL, pixs);

    if (operation == L_INTERPOLATED && d == 1) {
        L_WARNING("no interpolation for 1 bpp; using sampling\n", procName);
        operation = L_SAMPLED;
    }

    if (operation == L_SAMPLED)
        return pixQuadraticVShearSampled(pixs, dir, vmaxt, vmaxb, incolor);
    else  /* operation == L_INTERPOLATED */
        return pixQuadraticVShearLI(pixs, dir, vmaxt, vmaxb, incolor);
}


/*!
 *  pixQuadraticVShearSampled()
 *
 *      Input:  pixs (1, 8 or 32 bpp)
 *              dir (L_WARP_TO_LEFT or L_WARP_TO_RIGHT)
 *              vmaxt (max vertical displacement at edge and at top)
 *              vmaxb (max vertical displacement at edge and at bottom)
 *              incolor (L_BRING_IN_WHITE or L_BRING_IN_BLACK)
 *      Return: pixd (stretched), or null on error
 *
 *  Notes:
 *      (1) See pixQuadraticVShear() for details.
 */
PIX *
pixQuadraticVShearSampled(PIX     *pixs,
                          l_int32  dir,
                          l_int32  vmaxt,
                          l_int32  vmaxb,
                          l_int32  incolor)
{
l_int32    i, j, id, w, h, d, wm, hm, wpls, wpld, val;
l_uint32  *datas, *datad, *lines, *lined;
l_float32  delrowt, delrowb, denom1, denom2, dely;
PIX       *pixd;

    PROCNAME("pixQuadraticVShearSampled");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 8 && d != 32)
        return (PIX *)ERROR_PTR("pixs not 1, 8 or 32 bpp", procName, NULL);
    if (dir != L_WARP_TO_LEFT && dir != L_WARP_TO_RIGHT)
        return (PIX *)ERROR_PTR("invalid direction", procName, NULL);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return (PIX *)ERROR_PTR("invalid incolor", procName, NULL);

    if (vmaxt == 0 && vmaxb == 0)
        return pixCopy(NULL, pixs);

    pixd = pixCreateTemplate(pixs);
    pixSetBlackOrWhite(pixd, L_BRING_IN_WHITE);
    datas = pixGetData(pixs);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);
    wm = w - 1;
    hm = h - 1;
    denom1 = 1. / (l_float32)h;
    denom2 = 1. / (l_float32)(wm * wm);
    for (j = 0; j < w; j++) {
        if (dir == L_WARP_TO_LEFT) {
            delrowt = (l_float32)(vmaxt * (wm - j) * (wm - j)) * denom2;
            delrowb = (l_float32)(vmaxb * (wm - j) * (wm - j)) * denom2;
        } else if (dir == L_WARP_TO_RIGHT) {
            delrowt = (l_float32)(vmaxt * j * j) * denom2;
            delrowb = (l_float32)(vmaxb * j * j) * denom2;
        }
        switch (d)
        {
        case 1:
            for (id = 0; id < h; id++) {
                dely = (delrowt * (hm - id) + delrowb * id) * denom1;
                i = id - (l_int32)(dely + 0.5);
                if (i < 0 || i > hm) continue;
                lines = datas + i * wpls;
                lined = datad + id * wpld;
                val = GET_DATA_BIT(lines, j);
                if (val)
                    SET_DATA_BIT(lined, j);
            }
            break;
        case 8:
            for (id = 0; id < h; id++) {
                dely = (delrowt * (hm - id) + delrowb * id) * denom1;
                i = id - (l_int32)(dely + 0.5);
                if (i < 0 || i > hm) continue;
                lines = datas + i * wpls;
                lined = datad + id * wpld;
                val = GET_DATA_BYTE(lines, j);
                SET_DATA_BYTE(lined, j, val);
            }
            break;
        case 32:
            for (id = 0; id < h; id++) {
                dely = (delrowt * (hm - id) + delrowb * id) * denom1;
                i = id - (l_int32)(dely + 0.5);
                if (i < 0 || i > hm) continue;
                lines = datas + i * wpls;
                lined = datad + id * wpld;
                lined[j] = lines[j];
            }
            break;
        default:
            L_ERROR("invalid depth: %d\n", procName, d);
            pixDestroy(&pixd);
            return NULL;
        }
    }

    return pixd;
}


/*!
 *  pixQuadraticVShearLI()
 *
 *      Input:  pixs (8 or 32 bpp, or colormapped)
 *              dir (L_WARP_TO_LEFT or L_WARP_TO_RIGHT)
 *              vmaxt (max vertical displacement at edge and at top)
 *              vmaxb (max vertical displacement at edge and at bottom)
 *              incolor (L_BRING_IN_WHITE or L_BRING_IN_BLACK)
 *      Return: pixd (stretched), or null on error
 *
 *  Notes:
 *      (1) See pixQuadraticVShear() for details.
 */
PIX *
pixQuadraticVShearLI(PIX     *pixs,
                     l_int32  dir,
                     l_int32  vmaxt,
                     l_int32  vmaxb,
                     l_int32  incolor)
{
l_int32    i, j, id, yp, yf, w, h, d, wm, hm, wpls, wpld;
l_int32    val, rval, gval, bval;
l_uint32   word0, word1;
l_uint32  *datas, *datad, *lines, *lined;
l_float32  delrowt, delrowb, denom1, denom2, dely;
PIX       *pix, *pixd;
PIXCMAP   *cmap;

    PROCNAME("pixQuadraticVShearLI");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d == 1)
        return (PIX *)ERROR_PTR("pixs is 1 bpp", procName, NULL);
    cmap = pixGetColormap(pixs);
    if (d != 8 && d != 32 && !cmap)
        return (PIX *)ERROR_PTR("pixs not 8, 32 bpp, or cmap", procName, NULL);
    if (dir != L_WARP_TO_LEFT && dir != L_WARP_TO_RIGHT)
        return (PIX *)ERROR_PTR("invalid direction", procName, NULL);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return (PIX *)ERROR_PTR("invalid incolor", procName, NULL);

    if (vmaxt == 0 && vmaxb == 0)
        return pixCopy(NULL, pixs);

        /* Remove any existing colormap */
    if (cmap)
        pix = pixRemoveColormap(pixs, REMOVE_CMAP_BASED_ON_SRC);
    else
        pix = pixClone(pixs);
    d = pixGetDepth(pix);
    if (d != 8 && d != 32) {
        pixDestroy(&pix);
        return (PIX *)ERROR_PTR("invalid depth", procName, NULL);
    }

        /* Standard linear interp: subdivide each pixel into 64 parts */
    pixd = pixCreateTemplate(pix);
    pixSetBlackOrWhite(pixd, L_BRING_IN_WHITE);
    datas = pixGetData(pix);
    datad = pixGetData(pixd);
    wpls = pixGetWpl(pix);
    wpld = pixGetWpl(pixd);
    wm = w - 1;
    hm = h - 1;
    denom1 = 1.0 / (l_float32)h;
    denom2 = 1.0 / (l_float32)(wm * wm);
    for (j = 0; j < w; j++) {
        if (dir == L_WARP_TO_LEFT) {
            delrowt = (l_float32)(vmaxt * (wm - j) * (wm - j)) * denom2;
            delrowb = (l_float32)(vmaxb * (wm - j) * (wm - j)) * denom2;
        } else if (dir == L_WARP_TO_RIGHT) {
            delrowt = (l_float32)(vmaxt * j * j) * denom2;
            delrowb = (l_float32)(vmaxb * j * j) * denom2;
        }
        switch (d)
        {
        case 8:
            for (id = 0; id < h; id++) {
                dely = (delrowt * (hm - id) + delrowb * id) * denom1;
                i = 64 * id - (l_int32)(64.0 * dely);
                yp = i / 64;
                yf = i & 63;
                if (yp < 0 || yp > hm) continue;
                lines = datas + yp * wpls;
                lined = datad + id * wpld;
                if (yp < hm) {
                    val = ((63 - yf) * GET_DATA_BYTE(lines, j) +
                           yf * GET_DATA_BYTE(lines + wpls, j) + 31) / 63;
                } else {  /* yp == hm */
                    val = GET_DATA_BYTE(lines, j);
                }
                SET_DATA_BYTE(lined, j, val);
            }
            break;
        case 32:
            for (id = 0; id < h; id++) {
                dely = (delrowt * (hm - id) + delrowb * id) * denom1;
                i = 64 * id - (l_int32)(64.0 * dely);
                yp = i / 64;
                yf = i & 63;
                if (yp < 0 || yp > hm) continue;
                lines = datas + yp * wpls;
                lined = datad + id * wpld;
                if (yp < hm) {
                    word0 = *(lines + j);
                    word1 = *(lines + wpls + j);
                    rval = ((63 - yf) * ((word0 >> L_RED_SHIFT) & 0xff) +
                           yf * ((word1 >> L_RED_SHIFT) & 0xff) + 31) / 63;
                    gval = ((63 - yf) * ((word0 >> L_GREEN_SHIFT) & 0xff) +
                           yf * ((word1 >> L_GREEN_SHIFT) & 0xff) + 31) / 63;
                    bval = ((63 - yf) * ((word0 >> L_BLUE_SHIFT) & 0xff) +
                           yf * ((word1 >> L_BLUE_SHIFT) & 0xff) + 31) / 63;
                    composeRGBPixel(rval, gval, bval, lined + j);
                } else {  /* yp == hm */
                    lined[j] = lines[j];
                }
            }
            break;
        default:
            L_ERROR("invalid depth: %d\n", procName, d);
            pixDestroy(&pix);
            pixDestroy(&pixd);
            return NULL;
        }
    }

    pixDestroy(&pix);
    return pixd;
}


/*----------------------------------------------------------------------*
 *                     Stereo from a pair of images                     *
 *----------------------------------------------------------------------*/
/*!
 *  pixStereoFromPair()
 *
 *      Input:  pix1 (32 bpp rgb)
 *              pix2 (32 bpp rgb)
 *              rwt, gwt, bwt (weighting factors used for each component in
                               pix1 to determine the output red channel)
 *      Return: pixd (stereo enhanced), or null on error
 *
 *  Notes:
 *      (1) pix1 and pix2 are a pair of stereo images, ideally taken
 *          concurrently in the same plane, with some lateral translation.
 *      (2) The output red channel is determined from @pix1.
 *          The output green and blue channels are taken from the green
 *          and blue channels, respectively, of @pix2.
 *      (3) The weights determine how much of each component in @pix1
 *          goes into the output red channel.  The sum of weights
 *          must be 1.0.  If it's not, we scale the weights to
 *          satisfy this criterion.
 *      (4) The most general pixel mapping allowed here is:
 *            rval = rwt * r1 + gwt * g1 + bwt * b1  (from pix1)
 *            gval = g2   (from pix2)
 *            bval = b2   (from pix2)
 *      (5) The simplest method is to use rwt = 1.0, gwt = 0.0, bwt = 0.0,
 *          but this causes unpleasant visual artifacts with red in the image.
 *          Use of green and blue from @pix1 in the red channel,
 *          instead of red, tends to fix that problem.
 */
PIX *
pixStereoFromPair(PIX       *pix1,
                  PIX       *pix2,
                  l_float32  rwt,
                  l_float32  gwt,
                  l_float32  bwt)
{
l_int32    i, j, w, h, wpl1, wpl2, rval, gval, bval;
l_uint32   word1, word2;
l_uint32  *data1, *data2, *datad, *line1, *line2, *lined;
l_float32  sum;
PIX       *pixd;

    PROCNAME("pixStereoFromPair");

    if (!pix1 || !pix2)
        return (PIX *)ERROR_PTR("pix1, pix2 not both defined", procName, NULL);
    if (pixGetDepth(pix1) != 32 || pixGetDepth(pix2) != 32)
        return (PIX *)ERROR_PTR("pix1, pix2 not both 32 bpp", procName, NULL);

        /* Make sure the sum of weights is 1.0; otherwise, you can get
         * overflow in the gray value. */
    if (rwt == 0.0 && gwt == 0.0 && bwt == 0.0) {
        rwt = L_DEFAULT_RED_WEIGHT;
        gwt = L_DEFAULT_GREEN_WEIGHT;
        bwt = L_DEFAULT_BLUE_WEIGHT;
    }
    sum = rwt + gwt + bwt;
    if (L_ABS(sum - 1.0) > 0.0001) {  /* maintain ratios with sum == 1.0 */
        L_WARNING("weights don't sum to 1; maintaining ratios\n", procName);
        rwt = rwt / sum;
        gwt = gwt / sum;
        bwt = bwt / sum;
    }

    pixGetDimensions(pix1, &w, &h, NULL);
    pixd = pixCreateTemplate(pix1);
    data1 = pixGetData(pix1);
    data2 = pixGetData(pix2);
    datad = pixGetData(pixd);
    wpl1 = pixGetWpl(pix1);
    wpl2 = pixGetWpl(pix2);
    for (i = 0; i < h; i++) {
        line1 = data1 + i * wpl1;
        line2 = data2 + i * wpl2;
        lined = datad + i * wpl1;  /* wpl1 works for pixd */
        for (j = 0; j < w; j++) {
            word1 = *(line1 + j);
            word2 = *(line2 + j);
            rval = (l_int32)(rwt * ((word1 >> L_RED_SHIFT) & 0xff) +
                             gwt * ((word1 >> L_GREEN_SHIFT) & 0xff) +
                             bwt * ((word1 >> L_BLUE_SHIFT) & 0xff) + 0.5);
            gval = (word2 >> L_GREEN_SHIFT) & 0xff;
            bval = (word2 >> L_BLUE_SHIFT) & 0xff;
            composeRGBPixel(rval, gval, bval, lined + j);
        }
    }

    return pixd;
}
