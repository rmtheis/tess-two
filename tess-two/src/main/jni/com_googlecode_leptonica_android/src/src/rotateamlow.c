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
 *  rotateamlow.c
 *
 *      Grayscale and color rotation (area mapped)
 *
 *          32 bpp grayscale rotation about image center
 *               void    rotateAMColorLow()
 *
 *          8 bpp grayscale rotation about image center
 *               void    rotateAMGrayLow()
 *
 *          32 bpp grayscale rotation about UL corner of image
 *               void    rotateAMColorCornerLow()
 *
 *          8 bpp grayscale rotation about UL corner of image
 *               void    rotateAMGrayCornerLow()
 *
 *          Fast RGB color rotation about center:
 *               void    rotateAMColorFastLow()
 *
 */

#include <string.h>
#include <math.h>   /* required for sin and tan */
#include "allheaders.h"


/*------------------------------------------------------------------*
 *             32 bpp grayscale rotation about the center           *
 *------------------------------------------------------------------*/
void
rotateAMColorLow(l_uint32  *datad,
                 l_int32    w,
                 l_int32    h,
                 l_int32    wpld,
                 l_uint32  *datas,
                 l_int32    wpls,
                 l_float32  angle,
                 l_uint32   colorval)
{
l_int32    i, j, xcen, ycen, wm2, hm2;
l_int32    xdif, ydif, xpm, ypm, xp, yp, xf, yf;
l_int32    rval, gval, bval;
l_uint32   word00, word01, word10, word11;
l_uint32  *lines, *lined;
l_float32  sina, cosa;

    xcen = w / 2;
    wm2 = w - 2;
    ycen = h / 2;
    hm2 = h - 2;
    sina = 16. * sin(angle);
    cosa = 16. * cos(angle);

    for (i = 0; i < h; i++) {
        ydif = ycen - i;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            xdif = xcen - j;
            xpm = (l_int32)(-xdif * cosa - ydif * sina);
            ypm = (l_int32)(-ydif * cosa + xdif * sina);
            xp = xcen + (xpm >> 4);
            yp = ycen + (ypm >> 4);
            xf = xpm & 0x0f;
            yf = ypm & 0x0f;

                /* if off the edge, write input colorval */
            if (xp < 0 || yp < 0 || xp > wm2 || yp > hm2) {
                *(lined + j) = colorval;
                continue;
            }

            lines = datas + yp * wpls;

                /* do area weighting.  Without this, we would
                 * simply do:
                 *   *(lined + j) = *(lines + xp);
                 * which is faster but gives lousy results!
                 */
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
            composeRGBPixel(rval, gval, bval, lined + j);
        }
    }

    return;
}


/*------------------------------------------------------------------*
 *             8 bpp grayscale rotation about the center            *
 *------------------------------------------------------------------*/
void
rotateAMGrayLow(l_uint32  *datad,
                l_int32    w,
                l_int32    h,
                l_int32    wpld,
                l_uint32  *datas,
                l_int32    wpls,
                l_float32  angle,
                l_uint8    grayval)
{
l_int32    i, j, xcen, ycen, wm2, hm2;
l_int32    xdif, ydif, xpm, ypm, xp, yp, xf, yf;
l_int32    v00, v01, v10, v11;
l_uint8    val;
l_uint32  *lines, *lined;
l_float32  sina, cosa;

    xcen = w / 2;
    wm2 = w - 2;
    ycen = h / 2;
    hm2 = h - 2;
    sina = 16. * sin(angle);
    cosa = 16. * cos(angle);

    for (i = 0; i < h; i++) {
        ydif = ycen - i;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            xdif = xcen - j;
            xpm = (l_int32)(-xdif * cosa - ydif * sina);
            ypm = (l_int32)(-ydif * cosa + xdif * sina);
            xp = xcen + (xpm >> 4);
            yp = ycen + (ypm >> 4);
            xf = xpm & 0x0f;
            yf = ypm & 0x0f;

                /* if off the edge, write input grayval */
            if (xp < 0 || yp < 0 || xp > wm2 || yp > hm2) {
                SET_DATA_BYTE(lined, j, grayval);
                continue;
            }

            lines = datas + yp * wpls;

                /* do area weighting.  Without this, we would
                 * simply do:
                 *   SET_DATA_BYTE(lined, j, GET_DATA_BYTE(lines, xp));
                 * which is faster but gives lousy results!
                 */
            v00 = (16 - xf) * (16 - yf) * GET_DATA_BYTE(lines, xp);
            v10 = xf * (16 - yf) * GET_DATA_BYTE(lines, xp + 1);
            v01 = (16 - xf) * yf * GET_DATA_BYTE(lines + wpls, xp);
            v11 = xf * yf * GET_DATA_BYTE(lines + wpls, xp + 1);
            val = (l_uint8)((v00 + v01 + v10 + v11 + 128) / 256);
            SET_DATA_BYTE(lined, j, val);
        }
    }

    return;
}


/*------------------------------------------------------------------*
 *           32 bpp grayscale rotation about the UL corner          *
 *------------------------------------------------------------------*/
void
rotateAMColorCornerLow(l_uint32  *datad,
                       l_int32    w,
                       l_int32    h,
                       l_int32    wpld,
                       l_uint32  *datas,
                       l_int32    wpls,
                       l_float32  angle,
                       l_uint32   colorval)
{
l_int32    i, j, wm2, hm2;
l_int32    xpm, ypm, xp, yp, xf, yf;
l_int32    rval, gval, bval;
l_uint32   word00, word01, word10, word11;
l_uint32  *lines, *lined;
l_float32  sina, cosa;

    wm2 = w - 2;
    hm2 = h - 2;
    sina = 16. * sin(angle);
    cosa = 16. * cos(angle);

    for (i = 0; i < h; i++) {
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            xpm = (l_int32)(j * cosa + i * sina);
            ypm = (l_int32)(i * cosa - j * sina);
            xp = xpm >> 4;
            yp = ypm >> 4;
            xf = xpm & 0x0f;
            yf = ypm & 0x0f;

                /* if off the edge, write input colorval */
            if (xp < 0 || yp < 0 || xp > wm2 || yp > hm2) {
                *(lined + j) = colorval;
                continue;
            }

            lines = datas + yp * wpls;

                /* do area weighting.  Without this, we would
                 * simply do:
                 *   *(lined + j) = *(lines + xp);
                 * which is faster but gives lousy results!
                 */
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
            composeRGBPixel(rval, gval, bval, lined + j);
        }
    }

    return;
}



/*------------------------------------------------------------------*
 *            8 bpp grayscale rotation about the UL corner          *
 *------------------------------------------------------------------*/
void
rotateAMGrayCornerLow(l_uint32  *datad,
                      l_int32    w,
                      l_int32    h,
                      l_int32    wpld,
                      l_uint32  *datas,
                      l_int32    wpls,
                      l_float32  angle,
                      l_uint8    grayval)
{
l_int32    i, j, wm2, hm2;
l_int32    xpm, ypm, xp, yp, xf, yf;
l_int32    v00, v01, v10, v11;
l_uint8    val;
l_uint32  *lines, *lined;
l_float32  sina, cosa;

    wm2 = w - 2;
    hm2 = h - 2;
    sina = 16. * sin(angle);
    cosa = 16. * cos(angle);

    for (i = 0; i < h; i++) {
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            xpm = (l_int32)(j * cosa + i * sina);
            ypm = (l_int32)(i * cosa - j * sina);
            xp = xpm >> 4;
            yp = ypm >> 4;
            xf = xpm & 0x0f;
            yf = ypm & 0x0f;

                /* if off the edge, write input grayval */
            if (xp < 0 || yp < 0 || xp > wm2 || yp > hm2) {
                SET_DATA_BYTE(lined, j, grayval);
                continue;
            }

            lines = datas + yp * wpls;

                /* do area weighting.  Without this, we would
                 * simply do:
                 *   SET_DATA_BYTE(lined, j, GET_DATA_BYTE(lines, xp));
                 * which is faster but gives lousy results!
                 */
            v00 = (16 - xf) * (16 - yf) * GET_DATA_BYTE(lines, xp);
            v10 = xf * (16 - yf) * GET_DATA_BYTE(lines, xp + 1);
            v01 = (16 - xf) * yf * GET_DATA_BYTE(lines + wpls, xp);
            v11 = xf * yf * GET_DATA_BYTE(lines + wpls, xp + 1);
            val = (l_uint8)((v00 + v01 + v10 + v11 + 128) / 256);
            SET_DATA_BYTE(lined, j, val);
        }
    }

    return;
}


/*------------------------------------------------------------------*
 *               Fast RGB color rotation about center               *
 *------------------------------------------------------------------*/
/*!
 *  rotateAMColorFastLow()
 *
 *     This is a special simplification of area mapping with division
 *     of each pixel into 16 sub-pixels.  The exact coefficients that
 *     should be used are the same as for the 4x linear interpolation
 *     scaling case, and are given there.  I tried to approximate these
 *     as weighted coefficients with a maximum sum of 4, which
 *     allows us to do the arithmetic in parallel for the R, G and B
 *     components in a 32 bit pixel.  However, there are three reasons
 *     for not doing that:
 *        (1) the loss of accuracy in the parallel implementation
 *            is visually significant
 *        (2) the parallel implementation (described below) is slower
 *        (3) the parallel implementation requires allocation of
 *            a temporary color image
 *
 *     There are 16 cases for the choice of the subpixel, and
 *     for each, the mapping to the relevant source
 *     pixels is as follows:
 *
 *      subpixel      src pixel weights
 *      --------      -----------------
 *         0          sp1
 *         1          (3 * sp1 + sp2) / 4
 *         2          (sp1 + sp2) / 2
 *         3          (sp1 + 3 * sp2) / 4
 *         4          (3 * sp1 + sp3) / 4
 *         5          (9 * sp1 + 3 * sp2 + 3 * sp3 + sp4) / 16
 *         6          (3 * sp1 + 3 * sp2 + sp3 + sp4) / 8
 *         7          (3 * sp1 + 9 * sp2 + sp3 + 3 * sp4) / 16
 *         8          (sp1 + sp3) / 2
 *         9          (3 * sp1 + sp2 + 3 * sp3 + sp4) / 8
 *         10         (sp1 + sp2 + sp3 + sp4) / 4
 *         11         (sp1 + 3 * sp2 + sp3 + 3 * sp4) / 8
 *         12         (sp1 + 3 * sp3) / 4
 *         13         (3 * sp1 + sp2 + 9 * sp3 + 3 * sp4) / 16
 *         14         (sp1 + sp2 + 3 * sp3 + 3 * sp4) / 8
 *         15         (sp1 + 3 * sp2 + 3 * sp3 + 9 * sp4) / 16
 *
 *     Another way to visualize this is to consider the area mapping
 *     (or linear interpolation) coefficients  for the pixel sp1.
 *     Expressed in fourths, they can be written as asymmetric matrix:
 *
 *           4      3      2      1
 *           3      2.25   1.5    0.75
 *           2      1.5    1      0.5
 *           1      0.75   0.5    0.25
 *
 *     The coefficients for the three neighboring pixels can be
 *     similarly written.
 *
 *     This is implemented here, where, for each color component,
 *     we inline its extraction from each participating word,
 *     construct the linear combination, and combine the results
 *     into the destination 32 bit RGB pixel, using the appropriate shifts.
 *
 *     It is interesting to note that an alternative method, where
 *     we do the arithmetic on the 32 bit pixels directly (after
 *     shifting the components so they won't overflow into each other)
 *     is significantly inferior.  Because we have only 8 bits for
 *     internal overflows, which can be distributed as 2, 3, 3, it
 *     is impossible to add these with the correct linear
 *     interpolation coefficients, which require a sum of up to 16.
 *     Rounding off to a sum of 4 causes appreciable visual artifacts
 *     in the rotated image.  The code for the inferior method
 *     can be found in prog/rotatefastalt.c, for reference.
 *
 *     *** Warning: explicit assumption about RGB component ordering ***
 */
void
rotateAMColorFastLow(l_uint32  *datad,
                     l_int32    w,
                     l_int32    h,
                     l_int32    wpld,
                     l_uint32  *datas,
                     l_int32    wpls,
                     l_float32  angle,
                     l_uint32   colorval)
{
l_int32    i, j, xcen, ycen, wm2, hm2;
l_int32    xdif, ydif, xpm, ypm, xp, yp, xf, yf;
l_uint32   word1, word2, word3, word4, red, blue, green;
l_uint32  *pword, *lines, *lined;
l_float32  sina, cosa;

    xcen = w / 2;
    wm2 = w - 2;
    ycen = h / 2;
    hm2 = h - 2;
    sina = 4. * sin(angle);
    cosa = 4. * cos(angle);

    for (i = 0; i < h; i++) {
        ydif = ycen - i;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            xdif = xcen - j;
            xpm = (l_int32)(-xdif * cosa - ydif * sina);
            ypm = (l_int32)(-ydif * cosa + xdif * sina);
            xp = xcen + (xpm >> 2);
            yp = ycen + (ypm >> 2);
            xf = xpm & 0x03;
            yf = ypm & 0x03;

                /* if off the edge, write input grayval */
            if (xp < 0 || yp < 0 || xp > wm2 || yp > hm2) {
                *(lined + j) = colorval;
                continue;
            }

            lines = datas + yp * wpls;
            pword = lines + xp;

            switch (xf + 4 * yf)
            {
            case 0:
                *(lined + j) = *pword;
                break;
            case 1:
                word1 = *pword;
                word2 = *(pword + 1);
                red = 3 * (word1 >> 24) + (word2 >> 24);
                green = 3 * ((word1 >> 16) & 0xff) +
                            ((word2 >> 16) & 0xff);
                blue = 3 * ((word1 >> 8) & 0xff) +
                            ((word2 >> 8) & 0xff);
                *(lined + j) = ((red << 22) & 0xff000000) |
                               ((green << 14) & 0x00ff0000) |
                               ((blue << 6) & 0x0000ff00);
                break;
            case 2:
                word1 = *pword;
                word2 = *(pword + 1);
                red = (word1 >> 24) + (word2 >> 24);
                green = ((word1 >> 16) & 0xff) + ((word2 >> 16) & 0xff);
                blue = ((word1 >> 8) & 0xff) + ((word2 >> 8) & 0xff);
                *(lined + j) = ((red << 23) & 0xff000000) |
                               ((green << 15) & 0x00ff0000) |
                               ((blue << 7) & 0x0000ff00);
                break;
            case 3:
                word1 = *pword;
                word2 = *(pword + 1);
                red = (word1 >> 24) + 3 * (word2 >> 24);
                green = ((word1 >> 16) & 0xff) +
                          3 * ((word2 >> 16) & 0xff);
                blue = ((word1 >> 8) & 0xff) +
                          3 * ((word2 >> 8) & 0xff);
                *(lined + j) = ((red << 22) & 0xff000000) |
                               ((green << 14) & 0x00ff0000) |
                               ((blue << 6) & 0x0000ff00);
                break;
            case 4:
                word1 = *pword;
                word3 = *(pword + wpls);
                red = 3 * (word1 >> 24) + (word3 >> 24);
                green = 3 * ((word1 >> 16) & 0xff) +
                            ((word3 >> 16) & 0xff);
                blue = 3 * ((word1 >> 8) & 0xff) +
                            ((word3 >> 8) & 0xff);
                *(lined + j) = ((red << 22) & 0xff000000) |
                               ((green << 14) & 0x00ff0000) |
                               ((blue << 6) & 0x0000ff00);
                break;
            case 5:
                word1 = *pword;
                word2 = *(pword + 1);
                word3 = *(pword + wpls);
                word4 = *(pword + wpls + 1);
                red = 9 * (word1 >> 24) + 3 * (word2 >> 24) +
                      3 * (word3 >> 24) + (word4 >> 24);
                green = 9 * ((word1 >> 16) & 0xff) +
                        3 * ((word2 >> 16) & 0xff) +
                        3 * ((word3 >> 16) & 0xff) +
                        ((word4 >> 16) & 0xff);
                blue = 9 * ((word1 >> 8) & 0xff) +
                       3 * ((word2 >> 8) & 0xff) +
                       3 * ((word3 >> 8) & 0xff) +
                       ((word4 >> 8) & 0xff);
                *(lined + j) = ((red << 20) & 0xff000000) |
                               ((green << 12) & 0x00ff0000) |
                               ((blue << 4) & 0x0000ff00);
                break;
            case 6:
                word1 = *pword;
                word2 = *(pword + 1);
                word3 = *(pword + wpls);
                word4 = *(pword + wpls + 1);
                red = 3 * (word1 >> 24) +  3 * (word2 >> 24) +
                      (word3 >> 24) + (word4 >> 24);
                green = 3 * ((word1 >> 16) & 0xff) +
                        3 * ((word2 >> 16) & 0xff) +
                        ((word3 >> 16) & 0xff) +
                        ((word4 >> 16) & 0xff);
                blue = 3 * ((word1 >> 8) & 0xff) +
                       3 * ((word2 >> 8) & 0xff) +
                       ((word3 >> 8) & 0xff) +
                       ((word4 >> 8) & 0xff);
                *(lined + j) = ((red << 21) & 0xff000000) |
                               ((green << 13) & 0x00ff0000) |
                               ((blue << 5) & 0x0000ff00);
                break;
            case 7:
                word1 = *pword;
                word2 = *(pword + 1);
                word3 = *(pword + wpls);
                word4 = *(pword + wpls + 1);
                red = 3 * (word1 >> 24) + 9 * (word2 >> 24) +
                      (word3 >> 24) + 3 * (word4 >> 24);
                green = 3 * ((word1 >> 16) & 0xff) +
                        9 * ((word2 >> 16) & 0xff) +
                        ((word3 >> 16) & 0xff) +
                        3 * ((word4 >> 16) & 0xff);
                blue = 3 * ((word1 >> 8) & 0xff) +
                       9 * ((word2 >> 8) & 0xff) +
                         ((word3 >> 8) & 0xff) +
                         3 * ((word4 >> 8) & 0xff);
                *(lined + j) = ((red << 20) & 0xff000000) |
                               ((green << 12) & 0x00ff0000) |
                               ((blue << 4) & 0x0000ff00);
                break;
            case 8:
                word1 = *pword;
                word3 = *(pword + wpls);
                red = (word1 >> 24) + (word3 >> 24);
                green = ((word1 >> 16) & 0xff) + ((word3 >> 16) & 0xff);
                blue = ((word1 >> 8) & 0xff) + ((word3 >> 8) & 0xff);
                *(lined + j) = ((red << 23) & 0xff000000) |
                               ((green << 15) & 0x00ff0000) |
                               ((blue << 7) & 0x0000ff00);
                break;
            case 9:
                word1 = *pword;
                word2 = *(pword + 1);
                word3 = *(pword + wpls);
                word4 = *(pword + wpls + 1);
                red = 3 * (word1 >> 24) + (word2 >> 24) +
                      3 * (word3 >> 24) + (word4 >> 24);
                green = 3 * ((word1 >> 16) & 0xff) + ((word2 >> 16) & 0xff) +
                        3 * ((word3 >> 16) & 0xff) + ((word4 >> 16) & 0xff);
                blue = 3 * ((word1 >> 8) & 0xff) + ((word2 >> 8) & 0xff) +
                       3 * ((word3 >> 8) & 0xff) + ((word4 >> 8) & 0xff);
                *(lined + j) = ((red << 21) & 0xff000000) |
                               ((green << 13) & 0x00ff0000) |
                               ((blue << 5) & 0x0000ff00);
                break;
            case 10:
                word1 = *pword;
                word2 = *(pword + 1);
                word3 = *(pword + wpls);
                word4 = *(pword + wpls + 1);
                red = (word1 >> 24) + (word2 >> 24) +
                      (word3 >> 24) + (word4 >> 24);
                green = ((word1 >> 16) & 0xff) + ((word2 >> 16) & 0xff) +
                        ((word3 >> 16) & 0xff) + ((word4 >> 16) & 0xff);
                blue = ((word1 >> 8) & 0xff) + ((word2 >> 8) & 0xff) +
                       ((word3 >> 8) & 0xff) + ((word4 >> 8) & 0xff);
                *(lined + j) = ((red << 22) & 0xff000000) |
                               ((green << 14) & 0x00ff0000) |
                               ((blue << 6) & 0x0000ff00);
                break;
            case 11:
                word1 = *pword;
                word2 = *(pword + 1);
                word3 = *(pword + wpls);
                word4 = *(pword + wpls + 1);
                red = (word1 >> 24) + 3 * (word2 >> 24) +
                      (word3 >> 24) + 3 * (word4 >> 24);
                green = ((word1 >> 16) & 0xff) + 3 * ((word2 >> 16) & 0xff) +
                        ((word3 >> 16) & 0xff) + 3 * ((word4 >> 16) & 0xff);
                blue = ((word1 >> 8) & 0xff) + 3 * ((word2 >> 8) & 0xff) +
                       ((word3 >> 8) & 0xff) + 3 * ((word4 >> 8) & 0xff);
                *(lined + j) = ((red << 21) & 0xff000000) |
                               ((green << 13) & 0x00ff0000) |
                               ((blue << 5) & 0x0000ff00);
                break;
            case 12:
                word1 = *pword;
                word3 = *(pword + wpls);
                red = (word1 >> 24) + 3 * (word3 >> 24);
                green = ((word1 >> 16) & 0xff) +
                          3 * ((word3 >> 16) & 0xff);
                blue = ((word1 >> 8) & 0xff) +
                          3 * ((word3 >> 8) & 0xff);
                *(lined + j) = ((red << 22) & 0xff000000) |
                               ((green << 14) & 0x00ff0000) |
                               ((blue << 6) & 0x0000ff00);
                break;
            case 13:
                word1 = *pword;
                word2 = *(pword + 1);
                word3 = *(pword + wpls);
                word4 = *(pword + wpls + 1);
                red = 3 * (word1 >> 24) + (word2 >> 24) +
                      9 * (word3 >> 24) + 3 * (word4 >> 24);
                green = 3 * ((word1 >> 16) & 0xff) + ((word2 >> 16) & 0xff) +
                        9 * ((word3 >> 16) & 0xff) + 3 * ((word4 >> 16) & 0xff);
                blue = 3 *((word1 >> 8) & 0xff) + ((word2 >> 8) & 0xff) +
                       9 * ((word3 >> 8) & 0xff) + 3 * ((word4 >> 8) & 0xff);
                *(lined + j) = ((red << 20) & 0xff000000) |
                               ((green << 12) & 0x00ff0000) |
                               ((blue << 4) & 0x0000ff00);
                break;
            case 14:
                word1 = *pword;
                word2 = *(pword + 1);
                word3 = *(pword + wpls);
                word4 = *(pword + wpls + 1);
                red = (word1 >> 24) + (word2 >> 24) +
                      3 * (word3 >> 24) + 3 * (word4 >> 24);
                green = ((word1 >> 16) & 0xff) +((word2 >> 16) & 0xff) +
                        3 * ((word3 >> 16) & 0xff) + 3 * ((word4 >> 16) & 0xff);
                blue = ((word1 >> 8) & 0xff) + ((word2 >> 8) & 0xff) +
                       3 * ((word3 >> 8) & 0xff) + 3 * ((word4 >> 8) & 0xff);
                *(lined + j) = ((red << 21) & 0xff000000) |
                               ((green << 13) & 0x00ff0000) |
                               ((blue << 5) & 0x0000ff00);
                break;
            case 15:
                word1 = *pword;
                word2 = *(pword + 1);
                word3 = *(pword + wpls);
                word4 = *(pword + wpls + 1);
                red = (word1 >> 24) + 3 * (word2 >> 24) +
                      3 * (word3 >> 24) + 9 * (word4 >> 24);
                green = ((word1 >> 16) & 0xff) + 3 * ((word2 >> 16) & 0xff) +
                        3 * ((word3 >> 16) & 0xff) + 9 * ((word4 >> 16) & 0xff);
                blue = ((word1 >> 8) & 0xff) + 3 * ((word2 >> 8) & 0xff) +
                       3 * ((word3 >> 8) & 0xff) + 9 * ((word4 >> 8) & 0xff);
                *(lined + j) = ((red << 20) & 0xff000000) |
                               ((green << 12) & 0x00ff0000) |
                               ((blue << 4) & 0x0000ff00);
                break;
            default:
                fprintf(stderr, "shouldn't get here\n");
                break;
            }
        }
    }

    return;
}
