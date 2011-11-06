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
 *  scalelow.c
 *
 *         Color (interpolated) scaling: general case
 *                  void       scaleColorLILow()
 *
 *         Grayscale (interpolated) scaling: general case
 *                  void       scaleGrayLILow()
 *
 *         Color (interpolated) scaling: 2x upscaling
 *                  void       scaleColor2xLILow()
 *                  void       scaleColor2xLILineLow()
 *
 *         Grayscale (interpolated) scaling: 2x upscaling
 *                  void       scaleGray2xLILow()
 *                  void       scaleGray2xLILineLow()
 *
 *         Grayscale (interpolated) scaling: 4x upscaling
 *                  void       scaleGray4xLILow()
 *                  void       scaleGray4xLILineLow()
 *
 *         Grayscale and color scaling by closest pixel sampling
 *                  l_int32    scaleBySamplingLow()
 *
 *         Color and grayscale downsampling with (antialias) lowpass filter
 *                  l_int32    scaleSmoothLow()
 *                  void       scaleRGBToGray2Low()
 *
 *         Color and grayscale downsampling with (antialias) area mapping
 *                  l_int32    scaleColorAreaMapLow()
 *                  l_int32    scaleGrayAreaMapLow()
 *                  l_int32    scaleAreaMapLow2()
 *
 *         Binary scaling by closest pixel sampling
 *                  l_int32    scaleBinaryLow()
 *
 *         Scale-to-gray 2x
 *                  void       scaleToGray2Low()
 *                  l_uint32  *makeSumTabSG2()
 *                  l_uint8   *makeValTabSG2()
 *
 *         Scale-to-gray 3x
 *                  void       scaleToGray3Low()
 *                  l_uint32  *makeSumTabSG3()
 *                  l_uint8   *makeValTabSG3()
 *
 *         Scale-to-gray 4x
 *                  void       scaleToGray4Low()
 *                  l_uint32  *makeSumTabSG4()
 *                  l_uint8   *makeValTabSG4()
 *
 *         Scale-to-gray 6x
 *                  void       scaleToGray6Low()
 *                  l_uint8   *makeValTabSG6()
 *
 *         Scale-to-gray 8x
 *                  void       scaleToGray8Low()
 *                  l_uint8   *makeValTabSG8()
 *
 *         Scale-to-gray 16x
 *                  void       scaleToGray16Low()
 *
 *         Grayscale mipmap
 *                  l_int32    scaleMipmapLow()
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

#ifndef  NO_CONSOLE_IO
#define  DEBUG_OVERFLOW   0
#define  DEBUG_UNROLLING  0
#endif  /* ~NO_CONSOLE_IO */


/*------------------------------------------------------------------*
 *            General linear interpolated color scaling             *
 *------------------------------------------------------------------*/
/*!
 *  scaleColorLILow()
 *
 *  We choose to divide each pixel into 16 x 16 sub-pixels.
 *  Linear interpolation is equivalent to finding the 
 *  fractional area (i.e., number of sub-pixels divided
 *  by 256) associated with each of the four nearest src pixels,
 *  and weighting each pixel value by this fractional area.
 *
 *  P3 speed is about 7 x 10^6 dst pixels/sec/GHz
 */
void
scaleColorLILow(l_uint32  *datad,
               l_int32    wd,
               l_int32    hd,
               l_int32    wpld,
               l_uint32  *datas,
               l_int32    ws,
               l_int32    hs,
               l_int32    wpls)
{
l_int32    i, j, wm2, hm2;
l_int32    xpm, ypm;  /* location in src image, to 1/16 of a pixel */
l_int32    xp, yp, xf, yf;  /* src pixel and pixel fraction coordinates */
l_int32    v00r, v01r, v10r, v11r, v00g, v01g, v10g, v11g;
l_int32    v00b, v01b, v10b, v11b, area00, area01, area10, area11;
l_uint32   pixels1, pixels2, pixels3, pixels4, pixel;
l_uint32  *lines, *lined;
l_float32  scx, scy;

        /* (scx, scy) are scaling factors that are applied to the
         * dest coords to get the corresponding src coords.
         * We need them because we iterate over dest pixels
         * and must find the corresponding set of src pixels. */
    scx = 16. * (l_float32)ws / (l_float32)wd;
    scy = 16. * (l_float32)hs / (l_float32)hd;

    wm2 = ws - 2;
    hm2 = hs - 2;

        /* Iterate over the destination pixels */
    for (i = 0; i < hd; i++) {
        ypm = (l_int32)(scy * (l_float32)i);
        yp = ypm >> 4;
        yf = ypm & 0x0f;
        lined = datad + i * wpld;
        lines = datas + yp * wpls;
        for (j = 0; j < wd; j++) {
            xpm = (l_int32)(scx * (l_float32)j);
            xp = xpm >> 4;
            xf = xpm & 0x0f;

                /* Do bilinear interpolation.  This is a simple
                 * generalization of the calculation in scaleGrayLILow().
                 * Without this, we could simply subsample:
                 *     *(lined + j) = *(lines + xp);
                 * which is faster but gives lousy results!  */
            pixels1 = *(lines + xp);

            if (xp > wm2 || yp > hm2) {
                if (yp > hm2 && xp <= wm2) {  /* pixels near bottom */
                    pixels2 = *(lines + xp + 1);
                    pixels3 = pixels1;
                    pixels4 = pixels2;
                }
                else if (xp > wm2 && yp <= hm2) {  /* pixels near right side */
                    pixels2 = pixels1;
                    pixels3 = *(lines + wpls + xp);
                    pixels4 = pixels3;
                }
                else {  /* pixels at LR corner */
                    pixels4 = pixels3 = pixels2 = pixels1;
                }
            }
            else {
                pixels2 = *(lines + xp + 1);
                pixels3 = *(lines + wpls + xp);
                pixels4 = *(lines + wpls + xp + 1);
            }

            area00 = (16 - xf) * (16 - yf);
            area10 = xf * (16 - yf);
            area01 = (16 - xf) * yf;
            area11 = xf * yf;
            v00r = area00 * ((pixels1 >> L_RED_SHIFT) & 0xff);
            v00g = area00 * ((pixels1 >> L_GREEN_SHIFT) & 0xff);
            v00b = area00 * ((pixels1 >> L_BLUE_SHIFT) & 0xff);
            v10r = area10 * ((pixels2 >> L_RED_SHIFT) & 0xff);
            v10g = area10 * ((pixels2 >> L_GREEN_SHIFT) & 0xff);
            v10b = area10 * ((pixels2 >> L_BLUE_SHIFT) & 0xff);
            v01r = area01 * ((pixels3 >> L_RED_SHIFT) & 0xff);
            v01g = area01 * ((pixels3 >> L_GREEN_SHIFT) & 0xff);
            v01b = area01 * ((pixels3 >> L_BLUE_SHIFT) & 0xff);
            v11r = area11 * ((pixels4 >> L_RED_SHIFT) & 0xff);
            v11g = area11 * ((pixels4 >> L_GREEN_SHIFT) & 0xff);
            v11b = area11 * ((pixels4 >> L_BLUE_SHIFT) & 0xff);
            pixel = (((v00r + v10r + v01r + v11r + 128) << 16) & 0xff000000) |
                    (((v00g + v10g + v01g + v11g + 128) << 8) & 0x00ff0000) |
                    ((v00b + v10b + v01b + v11b + 128) & 0x0000ff00);
            *(lined + j) = pixel;
        }
    }

    return;
}


/*------------------------------------------------------------------*
 *            General linear interpolated gray scaling              *
 *------------------------------------------------------------------*/
/*!
 *  scaleGrayLILow()
 *
 *  We choose to divide each pixel into 16 x 16 sub-pixels.
 *  Linear interpolation is equivalent to finding the 
 *  fractional area (i.e., number of sub-pixels divided
 *  by 256) associated with each of the four nearest src pixels,
 *  and weighting each pixel value by this fractional area.
 */
void
scaleGrayLILow(l_uint32  *datad,
               l_int32    wd,
               l_int32    hd,
               l_int32    wpld,
               l_uint32  *datas,
               l_int32    ws,
               l_int32    hs,
               l_int32    wpls)
{
l_int32    i, j, wm2, hm2;
l_int32    xpm, ypm;  /* location in src image, to 1/16 of a pixel */
l_int32    xp, yp, xf, yf;  /* src pixel and pixel fraction coordinates */
l_int32    v00, v01, v10, v11, v00_val, v01_val, v10_val, v11_val;
l_uint8    val;
l_uint32  *lines, *lined;
l_float32  scx, scy;

        /* (scx, scy) are scaling factors that are applied to the
         * dest coords to get the corresponding src coords.
         * We need them because we iterate over dest pixels
         * and must find the corresponding set of src pixels. */
    scx = 16. * (l_float32)ws / (l_float32)wd;
    scy = 16. * (l_float32)hs / (l_float32)hd;

    wm2 = ws - 2;
    hm2 = hs - 2;

        /* Iterate over the destination pixels */
    for (i = 0; i < hd; i++) {
        ypm = (l_int32)(scy * (l_float32)i);
        yp = ypm >> 4;
        yf = ypm & 0x0f;
        lined = datad + i * wpld;
        lines = datas + yp * wpls;
        for (j = 0; j < wd; j++) {
            xpm = (l_int32)(scx * (l_float32)j);
            xp = xpm >> 4;
            xf = xpm & 0x0f;

                /* Do bilinear interpolation.  Without this, we could
                 * simply subsample:
                 *   SET_DATA_BYTE(lined, j, GET_DATA_BYTE(lines, xp));
                 * which is faster but gives lousy results!  */
            v00_val = GET_DATA_BYTE(lines, xp);
            if (xp > wm2 || yp > hm2) {
                if (yp > hm2 && xp <= wm2) {  /* pixels near bottom */
                    v01_val = v00_val;
                    v10_val = GET_DATA_BYTE(lines, xp + 1);
                    v11_val = v10_val;
                }
                else if (xp > wm2 && yp <= hm2) {  /* pixels near right side */
                    v01_val = GET_DATA_BYTE(lines + wpls, xp);
                    v10_val = v00_val;
                    v11_val = v01_val;
                }
                else {  /* pixels at LR corner */
                    v10_val = v01_val = v11_val = v00_val;
                }
            }
            else {
                v10_val = GET_DATA_BYTE(lines, xp + 1);
                v01_val = GET_DATA_BYTE(lines + wpls, xp);
                v11_val = GET_DATA_BYTE(lines + wpls, xp + 1);
            }

            v00 = (16 - xf) * (16 - yf) * v00_val;
            v10 = xf * (16 - yf) * v10_val;
            v01 = (16 - xf) * yf * v01_val;
            v11 = xf * yf * v11_val;

            val = (l_uint8)((v00 + v01 + v10 + v11 + 128) / 256);
            SET_DATA_BYTE(lined, j, val);
        }
    }

    return;
}


/*------------------------------------------------------------------*
 *                2x linear interpolated color scaling              *
 *------------------------------------------------------------------*/
/*!
 *  scaleColor2xLILow()
 *
 *  This is a special case of 2x expansion by linear
 *  interpolation.  Each src pixel contains 4 dest pixels.
 *  The 4 dest pixels in src pixel 1 are numbered at
 *  their UL corners.  The 4 dest pixels in src pixel 1
 *  are related to that src pixel and its 3 neighboring
 *  src pixels as follows:
 *
 *             1-----2-----|-----|-----|
 *             |     |     |     |     |
 *             |     |     |     |     |
 *  src 1 -->  3-----4-----|     |     |  <-- src 2
 *             |     |     |     |     |
 *             |     |     |     |     |
 *             |-----|-----|-----|-----|
 *             |     |     |     |     |
 *             |     |     |     |     |
 *  src 3 -->  |     |     |     |     |  <-- src 4
 *             |     |     |     |     |
 *             |     |     |     |     |
 *             |-----|-----|-----|-----|
 *
 *           dest      src
 *           ----      ---
 *           dp1    =  sp1
 *           dp2    =  (sp1 + sp2) / 2
 *           dp3    =  (sp1 + sp3) / 2
 *           dp4    =  (sp1 + sp2 + sp3 + sp4) / 4
 *
 *  We iterate over the src pixels, and unroll the calculation
 *  for each set of 4 dest pixels corresponding to that src
 *  pixel, caching pixels for the next src pixel whenever possible.
 *  The method is exactly analogous to the one we use for
 *  scaleGray2xLILow() and its line version.
 *
 *  P3 speed is about 5 x 10^7 dst pixels/sec/GHz
 */
void
scaleColor2xLILow(l_uint32  *datad,
                  l_int32    wpld,
                  l_uint32  *datas,
                  l_int32    ws,
                  l_int32    hs,
                  l_int32    wpls)
{
l_int32    i, hsm;
l_uint32  *lines, *lined;

    hsm = hs - 1;

        /* We're taking 2 src and 2 dest lines at a time,
         * and for each src line, we're computing 2 dest lines.
         * Call these 2 dest lines:  destline1 and destline2.
         * The first src line is used for destline 1.
         * On all but the last src line, both src lines are 
         * used in the linear interpolation for destline2.
         * On the last src line, both destline1 and destline2
         * are computed using only that src line (because there
         * isn't a lower src line). */

        /* iterate over all but the last src line */
    for (i = 0; i < hsm; i++) {
        lines = datas + i * wpls;
        lined = datad + 2 * i * wpld;
        scaleColor2xLILineLow(lined, wpld, lines, ws, wpls, 0);
    }
    
        /* last src line */
    lines = datas + hsm * wpls;
    lined = datad + 2 * hsm * wpld;
    scaleColor2xLILineLow(lined, wpld, lines, ws, wpls, 1);

    return;
}


/*!
 *  scaleColor2xLILineLow()
 *
 *      Input:  lined   (ptr to top destline, to be made from current src line)
 *              wpld
 *              lines   (ptr to current src line)
 *              ws
 *              wpls
 *              lastlineflag  (1 if last src line; 0 otherwise)
 *      Return: void
 *
 *  *** Warning: implicit assumption about RGB component ordering ***
 */
void
scaleColor2xLILineLow(l_uint32  *lined,
                      l_int32    wpld,
                      l_uint32  *lines,
                      l_int32    ws,
                      l_int32    wpls,
                      l_int32    lastlineflag)
{
l_int32    j, jd, wsm;
l_int32    rval1, rval2, rval3, rval4, gval1, gval2, gval3, gval4;
l_int32    bval1, bval2, bval3, bval4;
l_uint32   pixels1, pixels2, pixels3, pixels4, pixel;
l_uint32  *linesp, *linedp;

    wsm = ws - 1;

    if (lastlineflag == 0) {
        linesp = lines + wpls;
        linedp = lined + wpld;
        pixels1 = *lines;
        pixels3 = *linesp;

            /* initialize with v(2) and v(4) */
        rval2 = pixels1 >> 24;
        gval2 = (pixels1 >> 16) & 0xff;
        bval2 = (pixels1 >> 8) & 0xff;
        rval4 = pixels3 >> 24;
        gval4 = (pixels3 >> 16) & 0xff;
        bval4 = (pixels3 >> 8) & 0xff;

        for (j = 0, jd = 0; j < wsm; j++, jd += 2) {
                /* shift in previous src values */
            rval1 = rval2;
            gval1 = gval2;
            bval1 = bval2;
            rval3 = rval4;
            gval3 = gval4;
            bval3 = bval4;
                /* get new src values */
            pixels2 = *(lines + j + 1);
            pixels4 = *(linesp + j + 1);
            rval2 = pixels2 >> 24;
            gval2 = (pixels2 >> 16) & 0xff;
            bval2 = (pixels2 >> 8) & 0xff;
            rval4 = pixels4 >> 24;
            gval4 = (pixels4 >> 16) & 0xff;
            bval4 = (pixels4 >> 8) & 0xff;
                /* save dest values */
            pixel = (rval1 << 24 | gval1 << 16 | bval1 << 8);
            *(lined + jd) = pixel;                               /* pix 1 */
            pixel = ((((rval1 + rval2) << 23) & 0xff000000) |
                     (((gval1 + gval2) << 15) & 0x00ff0000) |
                     (((bval1 + bval2) << 7) & 0x0000ff00));
            *(lined + jd + 1) = pixel;                           /* pix 2 */
            pixel = ((((rval1 + rval3) << 23) & 0xff000000) |
                     (((gval1 + gval3) << 15) & 0x00ff0000) |
                     (((bval1 + bval3) << 7) & 0x0000ff00));
            *(linedp + jd) = pixel;                              /* pix 3 */
            pixel = ((((rval1 + rval2 + rval3 + rval4) << 22) & 0xff000000) | 
                     (((gval1 + gval2 + gval3 + gval4) << 14) & 0x00ff0000) |
                     (((bval1 + bval2 + bval3 + bval4) << 6) & 0x0000ff00));
            *(linedp + jd + 1) = pixel;                          /* pix 4 */
        }  
            /* last src pixel on line */
        rval1 = rval2;
        gval1 = gval2;
        bval1 = bval2;
        rval3 = rval4;
        gval3 = gval4;
        bval3 = bval4;
        pixel = (rval1 << 24 | gval1 << 16 | bval1 << 8);
        *(lined + 2 * wsm) = pixel;                        /* pix 1 */
        *(lined + 2 * wsm + 1) = pixel;                    /* pix 2 */
        pixel = ((((rval1 + rval3) << 23) & 0xff000000) |
                 (((gval1 + gval3) << 15) & 0x00ff0000) |
                 (((bval1 + bval3) << 7) & 0x0000ff00));
        *(linedp + 2 * wsm) = pixel;                       /* pix 3 */
        *(linedp + 2 * wsm + 1) = pixel;                   /* pix 4 */
    }
    else {   /* last row of src pixels: lastlineflag == 1 */
        linedp = lined + wpld;
        pixels2 = *lines;
        rval2 = pixels2 >> 24;
        gval2 = (pixels2 >> 16) & 0xff;
        bval2 = (pixels2 >> 8) & 0xff;
        for (j = 0, jd = 0; j < wsm; j++, jd += 2) {
            rval1 = rval2;
            gval1 = gval2;
            bval1 = bval2;
            pixels2 = *(lines + j + 1);
            rval2 = pixels2 >> 24;
            gval2 = (pixels2 >> 16) & 0xff;
            bval2 = (pixels2 >> 8) & 0xff;
            pixel = (rval1 << 24 | gval1 << 16 | bval1 << 8);
            *(lined + jd) = pixel;                            /* pix 1 */
            *(linedp + jd) = pixel;                           /* pix 2 */
            pixel = ((((rval1 + rval2) << 23) & 0xff000000) |
                     (((gval1 + gval2) << 15) & 0x00ff0000) |
                     (((bval1 + bval2) << 7) & 0x0000ff00));
            *(lined + jd + 1) = pixel;                        /* pix 3 */
            *(linedp + jd + 1) = pixel;                       /* pix 4 */
        }  
        rval1 = rval2;
        gval1 = gval2;
        bval1 = bval2;
        pixel = (rval1 << 24 | gval1 << 16 | bval1 << 8);
        *(lined + 2 * wsm) = pixel;                           /* pix 1 */
        *(lined + 2 * wsm + 1) = pixel;                       /* pix 2 */
        *(linedp + 2 * wsm) = pixel;                          /* pix 3 */
        *(linedp + 2 * wsm + 1) = pixel;                      /* pix 4 */
    }
        
    return;
}


/*------------------------------------------------------------------*
 *                2x linear interpolated gray scaling               *
 *------------------------------------------------------------------*/
/*!
 *  scaleGray2xLILow()
 *
 *  This is a special case of 2x expansion by linear
 *  interpolation.  Each src pixel contains 4 dest pixels.
 *  The 4 dest pixels in src pixel 1 are numbered at
 *  their UL corners.  The 4 dest pixels in src pixel 1
 *  are related to that src pixel and its 3 neighboring
 *  src pixels as follows:
 *
 *             1-----2-----|-----|-----|
 *             |     |     |     |     |
 *             |     |     |     |     |
 *  src 1 -->  3-----4-----|     |     |  <-- src 2
 *             |     |     |     |     |
 *             |     |     |     |     |
 *             |-----|-----|-----|-----|
 *             |     |     |     |     |
 *             |     |     |     |     |
 *  src 3 -->  |     |     |     |     |  <-- src 4
 *             |     |     |     |     |
 *             |     |     |     |     |
 *             |-----|-----|-----|-----|
 *
 *           dest      src
 *           ----      ---
 *           dp1    =  sp1
 *           dp2    =  (sp1 + sp2) / 2
 *           dp3    =  (sp1 + sp3) / 2
 *           dp4    =  (sp1 + sp2 + sp3 + sp4) / 4
 *
 *  We iterate over the src pixels, and unroll the calculation
 *  for each set of 4 dest pixels corresponding to that src
 *  pixel, caching pixels for the next src pixel whenever possible.
 */
void
scaleGray2xLILow(l_uint32  *datad,
                 l_int32    wpld,
                 l_uint32  *datas,
                 l_int32    ws,
                 l_int32    hs,
                 l_int32    wpls)
{
l_int32    i, hsm;
l_uint32  *lines, *lined;

    hsm = hs - 1;

        /* We're taking 2 src and 2 dest lines at a time,
         * and for each src line, we're computing 2 dest lines.
         * Call these 2 dest lines:  destline1 and destline2.
         * The first src line is used for destline 1.
         * On all but the last src line, both src lines are 
         * used in the linear interpolation for destline2.
         * On the last src line, both destline1 and destline2
         * are computed using only that src line (because there
         * isn't a lower src line). */

        /* iterate over all but the last src line */
    for (i = 0; i < hsm; i++) {
        lines = datas + i * wpls;
        lined = datad + 2 * i * wpld;
        scaleGray2xLILineLow(lined, wpld, lines, ws, wpls, 0);
    }
    
        /* last src line */
    lines = datas + hsm * wpls;
    lined = datad + 2 * hsm * wpld;
    scaleGray2xLILineLow(lined, wpld, lines, ws, wpls, 1);

    return;
}


/*!
 *  scaleGray2xLILineLow()
 *
 *      Input:  lined   (ptr to top destline, to be made from current src line)
 *              wpld
 *              lines   (ptr to current src line)
 *              ws
 *              wpls
 *              lastlineflag  (1 if last src line; 0 otherwise)
 *      Return: void
 */
void
scaleGray2xLILineLow(l_uint32  *lined,
                     l_int32    wpld,
                     l_uint32  *lines,
                     l_int32    ws,
                     l_int32    wpls,
                     l_int32    lastlineflag)
{
l_int32    j, jd, wsm, w;
l_int32    sval1, sval2, sval3, sval4;
l_uint32  *linesp, *linedp;
l_uint32   words, wordsp, wordd, worddp;

    wsm = ws - 1;

    if (lastlineflag == 0) {
        linesp = lines + wpls;
        linedp = lined + wpld;

            /* Unroll the loop 4x and work on full words */
        words = lines[0];
        wordsp = linesp[0];
        sval2 = (words >> 24) & 0xff;
        sval4 = (wordsp >> 24) & 0xff;
        for (j = 0, jd = 0, w = 0; j + 3 < wsm; j += 4, jd += 8, w++) {
                /* At the top of the loop,
                 * words == lines[w], wordsp == linesp[w]
                 * and the top bytes of those have been loaded into
                 * sval2 and sval4. */
            sval1 = sval2;
            sval2 = (words >> 16) & 0xff;
            sval3 = sval4;
            sval4 = (wordsp >> 16) & 0xff;
            wordd = (sval1 << 24) | (((sval1 + sval2) >> 1) << 16);
            worddp = (((sval1 + sval3) >> 1) << 24) |
                (((sval1 + sval2 + sval3 + sval4) >> 2) << 16);

            sval1 = sval2;
            sval2 = (words >> 8) & 0xff;
            sval3 = sval4;
            sval4 = (wordsp >> 8) & 0xff;
            wordd |= (sval1 << 8) | ((sval1 + sval2) >> 1);
            worddp |= (((sval1 + sval3) >> 1) << 8) |
                ((sval1 + sval2 + sval3 + sval4) >> 2);
            lined[w * 2] = wordd;
            linedp[w * 2] = worddp;

            sval1 = sval2;
            sval2 = words & 0xff;
            sval3 = sval4;
            sval4 = wordsp & 0xff;
            wordd = (sval1 << 24) |                              /* pix 1 */
                (((sval1 + sval2) >> 1) << 16);                  /* pix 2 */
            worddp = (((sval1 + sval3) >> 1) << 24) |            /* pix 3 */
                (((sval1 + sval2 + sval3 + sval4) >> 2) << 16);  /* pix 4 */

                /* Load the next word as we need its first byte */
            words = lines[w + 1];
            wordsp = linesp[w + 1];
            sval1 = sval2;
            sval2 = (words >> 24) & 0xff;
            sval3 = sval4;
            sval4 = (wordsp >> 24) & 0xff;
            wordd |= (sval1 << 8) |                              /* pix 1 */
                ((sval1 + sval2) >> 1);                          /* pix 2 */
            worddp |= (((sval1 + sval3) >> 1) << 8) |            /* pix 3 */
                ((sval1 + sval2 + sval3 + sval4) >> 2);          /* pix 4 */
            lined[w * 2 + 1] = wordd;
            linedp[w * 2 + 1] = worddp;
        }

            /* Finish up the last word */
        for (; j < wsm; j++, jd += 2) {
            sval1 = sval2;
            sval3 = sval4;
            sval2 = GET_DATA_BYTE(lines, j + 1);
            sval4 = GET_DATA_BYTE(linesp, j + 1);
            SET_DATA_BYTE(lined, jd, sval1);                     /* pix 1 */
            SET_DATA_BYTE(lined, jd + 1, (sval1 + sval2) / 2);   /* pix 2 */
            SET_DATA_BYTE(linedp, jd, (sval1 + sval3) / 2);      /* pix 3 */
            SET_DATA_BYTE(linedp, jd + 1,
                          (sval1 + sval2 + sval3 + sval4) / 4);  /* pix 4 */
        }
        sval1 = sval2;
        sval3 = sval4;
        SET_DATA_BYTE(lined, 2 * wsm, sval1);                     /* pix 1 */
        SET_DATA_BYTE(lined, 2 * wsm + 1, sval1);                 /* pix 2 */
        SET_DATA_BYTE(linedp, 2 * wsm, (sval1 + sval3) / 2);      /* pix 3 */
        SET_DATA_BYTE(linedp, 2 * wsm + 1, (sval1 + sval3) / 2);  /* pix 4 */

#if DEBUG_UNROLLING
#define CHECK_BYTE(a, b, c) if (GET_DATA_BYTE(a, b) != c) {\
     fprintf(stderr, "Error: mismatch at %d, %d vs %d\n", \
             j, GET_DATA_BYTE(a, b), c); }

        sval2 = GET_DATA_BYTE(lines, 0);
        sval4 = GET_DATA_BYTE(linesp, 0);
        for (j = 0, jd = 0; j < wsm; j++, jd += 2) {
            sval1 = sval2;
            sval3 = sval4;
            sval2 = GET_DATA_BYTE(lines, j + 1);
            sval4 = GET_DATA_BYTE(linesp, j + 1);
            CHECK_BYTE(lined, jd, sval1);                     /* pix 1 */
            CHECK_BYTE(lined, jd + 1, (sval1 + sval2) / 2);   /* pix 2 */
            CHECK_BYTE(linedp, jd, (sval1 + sval3) / 2);      /* pix 3 */
            CHECK_BYTE(linedp, jd + 1,
                          (sval1 + sval2 + sval3 + sval4) / 4);  /* pix 4 */
        }
        sval1 = sval2;
        sval3 = sval4;
        CHECK_BYTE(lined, 2 * wsm, sval1);                     /* pix 1 */
        CHECK_BYTE(lined, 2 * wsm + 1, sval1);                 /* pix 2 */
        CHECK_BYTE(linedp, 2 * wsm, (sval1 + sval3) / 2);      /* pix 3 */
        CHECK_BYTE(linedp, 2 * wsm + 1, (sval1 + sval3) / 2);  /* pix 4 */
#undef CHECK_BYTE
#endif
    }
    else {   /* last row of src pixels: lastlineflag == 1 */
        linedp = lined + wpld;
        sval2 = GET_DATA_BYTE(lines, 0);
        for (j = 0, jd = 0; j < wsm; j++, jd += 2) {
            sval1 = sval2;
            sval2 = GET_DATA_BYTE(lines, j + 1);
            SET_DATA_BYTE(lined, jd, sval1);                       /* pix 1 */
            SET_DATA_BYTE(linedp, jd, sval1);                      /* pix 3 */
            SET_DATA_BYTE(lined, jd + 1, (sval1 + sval2) / 2);     /* pix 2 */
            SET_DATA_BYTE(linedp, jd + 1, (sval1 + sval2) / 2);    /* pix 4 */
        }  
        sval1 = sval2;
        SET_DATA_BYTE(lined, 2 * wsm, sval1);                     /* pix 1 */
        SET_DATA_BYTE(lined, 2 * wsm + 1, sval1);                 /* pix 2 */
        SET_DATA_BYTE(linedp, 2 * wsm, sval1);                    /* pix 3 */
        SET_DATA_BYTE(linedp, 2 * wsm + 1, sval1);                /* pix 4 */
    }
        
    return;
}


/*------------------------------------------------------------------*
 *               4x linear interpolated gray scaling                *
 *------------------------------------------------------------------*/
/*!
 *  scaleGray4xLILow()
 *
 *  This is a special case of 4x expansion by linear
 *  interpolation.  Each src pixel contains 16 dest pixels.
 *  The 16 dest pixels in src pixel 1 are numbered at
 *  their UL corners.  The 16 dest pixels in src pixel 1
 *  are related to that src pixel and its 3 neighboring
 *  src pixels as follows:
 *
 *             1---2---3---4---|---|---|---|---|
 *             |   |   |   |   |   |   |   |   |
 *             5---6---7---8---|---|---|---|---|
 *             |   |   |   |   |   |   |   |   |
 *  src 1 -->  9---a---b---c---|---|---|---|---|  <-- src 2
 *             |   |   |   |   |   |   |   |   |
 *             d---e---f---g---|---|---|---|---|
 *             |   |   |   |   |   |   |   |   |
 *             |===|===|===|===|===|===|===|===|
 *             |   |   |   |   |   |   |   |   |
 *             |---|---|---|---|---|---|---|---|
 *             |   |   |   |   |   |   |   |   |
 *  src 3 -->  |---|---|---|---|---|---|---|---|  <-- src 4
 *             |   |   |   |   |   |   |   |   |
 *             |---|---|---|---|---|---|---|---|
 *             |   |   |   |   |   |   |   |   |
 *             |---|---|---|---|---|---|---|---|
 *
 *           dest      src
 *           ----      ---
 *           dp1    =  sp1
 *           dp2    =  (3 * sp1 + sp2) / 4
 *           dp3    =  (sp1 + sp2) / 2
 *           dp4    =  (sp1 + 3 * sp2) / 4
 *           dp5    =  (3 * sp1 + sp3) / 4
 *           dp6    =  (9 * sp1 + 3 * sp2 + 3 * sp3 + sp4) / 16 
 *           dp7    =  (3 * sp1 + 3 * sp2 + sp3 + sp4) / 8
 *           dp8    =  (3 * sp1 + 9 * sp2 + 1 * sp3 + 3 * sp4) / 16 
 *           dp9    =  (sp1 + sp3) / 2
 *           dp10   =  (3 * sp1 + sp2 + 3 * sp3 + sp4) / 8
 *           dp11   =  (sp1 + sp2 + sp3 + sp4) / 4 
 *           dp12   =  (sp1 + 3 * sp2 + sp3 + 3 * sp4) / 8
 *           dp13   =  (sp1 + 3 * sp3) / 4
 *           dp14   =  (3 * sp1 + sp2 + 9 * sp3 + 3 * sp4) / 16 
 *           dp15   =  (sp1 + sp2 + 3 * sp3 + 3 * sp4) / 8
 *           dp16   =  (sp1 + 3 * sp2 + 3 * sp3 + 9 * sp4) / 16 
 *
 *  We iterate over the src pixels, and unroll the calculation
 *  for each set of 16 dest pixels corresponding to that src
 *  pixel, caching pixels for the next src pixel whenever possible.
 */
void
scaleGray4xLILow(l_uint32  *datad,
                 l_int32    wpld,
                 l_uint32  *datas,
                 l_int32    ws,
                 l_int32    hs,
                 l_int32    wpls)
{
l_int32    i, hsm;
l_uint32  *lines, *lined;

    hsm = hs - 1;

        /* We're taking 2 src and 4 dest lines at a time,
         * and for each src line, we're computing 4 dest lines.
         * Call these 4 dest lines:  destline1 - destline4.
         * The first src line is used for destline 1.
         * Two src lines are used for all other dest lines,
         * except for the last 4 dest lines, which are computed
         * using only the last src line. */

        /* iterate over all but the last src line */
    for (i = 0; i < hsm; i++) {
        lines = datas + i * wpls;
        lined = datad + 4 * i * wpld;
        scaleGray4xLILineLow(lined, wpld, lines, ws, wpls, 0);
    }
    
        /* last src line */
    lines = datas + hsm * wpls;
    lined = datad + 4 * hsm * wpld;
    scaleGray4xLILineLow(lined, wpld, lines, ws, wpls, 1);

    return;
}


/*!
 *  scaleGray4xLILineLow()
 *
 *      Input:  lined   (ptr to top destline, to be made from current src line)
 *              wpld
 *              lines   (ptr to current src line)
 *              ws
 *              wpls
 *              lastlineflag  (1 if last src line; 0 otherwise)
 *      Return: void
 */
void
scaleGray4xLILineLow(l_uint32  *lined,
                     l_int32    wpld,
                     l_uint32  *lines,
                     l_int32    ws,
                     l_int32    wpls,
                     l_int32    lastlineflag)
{
l_int32    j, jd, wsm, wsm4;
l_int32    s1, s2, s3, s4, s1t, s2t, s3t, s4t;
l_uint32  *linesp, *linedp1, *linedp2, *linedp3;

    wsm = ws - 1;
    wsm4 = 4 * wsm;

    if (lastlineflag == 0) {
        linesp = lines + wpls;
        linedp1 = lined + wpld;
        linedp2 = lined + 2 * wpld;
        linedp3 = lined + 3 * wpld;
        s2 = GET_DATA_BYTE(lines, 0);
        s4 = GET_DATA_BYTE(linesp, 0);
        for (j = 0, jd = 0; j < wsm; j++, jd += 4) {
            s1 = s2;
            s3 = s4;
            s2 = GET_DATA_BYTE(lines, j + 1);
            s4 = GET_DATA_BYTE(linesp, j + 1);
            s1t = 3 * s1;
            s2t = 3 * s2;
            s3t = 3 * s3;
            s4t = 3 * s4;
            SET_DATA_BYTE(lined, jd, s1);                             /* d1 */
            SET_DATA_BYTE(lined, jd + 1, (s1t + s2) / 4);             /* d2 */
            SET_DATA_BYTE(lined, jd + 2, (s1 + s2) / 2);              /* d3 */
            SET_DATA_BYTE(lined, jd + 3, (s1 + s2t) / 4);             /* d4 */
            SET_DATA_BYTE(linedp1, jd, (s1t + s3) / 4);                /* d5 */
            SET_DATA_BYTE(linedp1, jd + 1, (9*s1 + s2t + s3t + s4) / 16); /*d6*/
            SET_DATA_BYTE(linedp1, jd + 2, (s1t + s2t + s3 + s4) / 8); /* d7 */
            SET_DATA_BYTE(linedp1, jd + 3, (s1t + 9*s2 + s3 + s4t) / 16);/*d8*/
            SET_DATA_BYTE(linedp2, jd, (s1 + s3) / 2);                /* d9 */
            SET_DATA_BYTE(linedp2, jd + 1, (s1t + s2 + s3t + s4) / 8);/* d10 */
            SET_DATA_BYTE(linedp2, jd + 2, (s1 + s2 + s3 + s4) / 4);  /* d11 */
            SET_DATA_BYTE(linedp2, jd + 3, (s1 + s2t + s3 + s4t) / 8);/* d12 */
            SET_DATA_BYTE(linedp3, jd, (s1 + s3t) / 4);               /* d13 */
            SET_DATA_BYTE(linedp3, jd + 1, (s1t + s2 + 9*s3 + s4t) / 16);/*d14*/
            SET_DATA_BYTE(linedp3, jd + 2, (s1 + s2 + s3t + s4t) / 8); /* d15 */
            SET_DATA_BYTE(linedp3, jd + 3, (s1 + s2t + s3t + 9*s4) / 16);/*d16*/
        }  
        s1 = s2;
        s3 = s4;
        s1t = 3 * s1;
        s3t = 3 * s3;
        SET_DATA_BYTE(lined, wsm4, s1);                               /* d1 */
        SET_DATA_BYTE(lined, wsm4 + 1, s1);                           /* d2 */
        SET_DATA_BYTE(lined, wsm4 + 2, s1);                           /* d3 */
        SET_DATA_BYTE(lined, wsm4 + 3, s1);                           /* d4 */
        SET_DATA_BYTE(linedp1, wsm4, (s1t + s3) / 4);                 /* d5 */
        SET_DATA_BYTE(linedp1, wsm4 + 1, (s1t + s3) / 4);             /* d6 */
        SET_DATA_BYTE(linedp1, wsm4 + 2, (s1t + s3) / 4);             /* d7 */
        SET_DATA_BYTE(linedp1, wsm4 + 3, (s1t + s3) / 4);             /* d8 */
        SET_DATA_BYTE(linedp2, wsm4, (s1 + s3) / 2);                  /* d9 */
        SET_DATA_BYTE(linedp2, wsm4 + 1, (s1 + s3) / 2);              /* d10 */
        SET_DATA_BYTE(linedp2, wsm4 + 2, (s1 + s3) / 2);              /* d11 */
        SET_DATA_BYTE(linedp2, wsm4 + 3, (s1 + s3) / 2);              /* d12 */
        SET_DATA_BYTE(linedp3, wsm4, (s1 + s3t) / 4);                 /* d13 */
        SET_DATA_BYTE(linedp3, wsm4 + 1, (s1 + s3t) / 4);             /* d14 */
        SET_DATA_BYTE(linedp3, wsm4 + 2, (s1 + s3t) / 4);             /* d15 */
        SET_DATA_BYTE(linedp3, wsm4 + 3, (s1 + s3t) / 4);             /* d16 */
    }
    else {   /* last row of src pixels: lastlineflag == 1 */
        linedp1 = lined + wpld;
        linedp2 = lined + 2 * wpld;
        linedp3 = lined + 3 * wpld;
        s2 = GET_DATA_BYTE(lines, 0);
        for (j = 0, jd = 0; j < wsm; j++, jd += 4) {
            s1 = s2;
            s2 = GET_DATA_BYTE(lines, j + 1);
            s1t = 3 * s1;
            s2t = 3 * s2;
            SET_DATA_BYTE(lined, jd, s1);                            /* d1 */
            SET_DATA_BYTE(lined, jd + 1, (s1t + s2) / 4 );           /* d2 */
            SET_DATA_BYTE(lined, jd + 2, (s1 + s2) / 2 );            /* d3 */
            SET_DATA_BYTE(lined, jd + 3, (s1 + s2t) / 4 );           /* d4 */
            SET_DATA_BYTE(linedp1, jd, s1);                          /* d5 */
            SET_DATA_BYTE(linedp1, jd + 1, (s1t + s2) / 4 );         /* d6 */
            SET_DATA_BYTE(linedp1, jd + 2, (s1 + s2) / 2 );          /* d7 */
            SET_DATA_BYTE(linedp1, jd + 3, (s1 + s2t) / 4 );         /* d8 */
            SET_DATA_BYTE(linedp2, jd, s1);                          /* d9 */
            SET_DATA_BYTE(linedp2, jd + 1, (s1t + s2) / 4 );         /* d10 */
            SET_DATA_BYTE(linedp2, jd + 2, (s1 + s2) / 2 );          /* d11 */
            SET_DATA_BYTE(linedp2, jd + 3, (s1 + s2t) / 4 );         /* d12 */
            SET_DATA_BYTE(linedp3, jd, s1);                          /* d13 */
            SET_DATA_BYTE(linedp3, jd + 1, (s1t + s2) / 4 );         /* d14 */
            SET_DATA_BYTE(linedp3, jd + 2, (s1 + s2) / 2 );          /* d15 */
            SET_DATA_BYTE(linedp3, jd + 3, (s1 + s2t) / 4 );         /* d16 */
        }  
        s1 = s2;
        SET_DATA_BYTE(lined, wsm4, s1);                              /* d1 */
        SET_DATA_BYTE(lined, wsm4 + 1, s1);                          /* d2 */
        SET_DATA_BYTE(lined, wsm4 + 2, s1);                          /* d3 */
        SET_DATA_BYTE(lined, wsm4 + 3, s1);                          /* d4 */
        SET_DATA_BYTE(linedp1, wsm4, s1);                            /* d5 */
        SET_DATA_BYTE(linedp1, wsm4 + 1, s1);                        /* d6 */
        SET_DATA_BYTE(linedp1, wsm4 + 2, s1);                        /* d7 */
        SET_DATA_BYTE(linedp1, wsm4 + 3, s1);                        /* d8 */
        SET_DATA_BYTE(linedp2, wsm4, s1);                            /* d9 */
        SET_DATA_BYTE(linedp2, wsm4 + 1, s1);                        /* d10 */
        SET_DATA_BYTE(linedp2, wsm4 + 2, s1);                        /* d11 */
        SET_DATA_BYTE(linedp2, wsm4 + 3, s1);                        /* d12 */
        SET_DATA_BYTE(linedp3, wsm4, s1);                            /* d13 */
        SET_DATA_BYTE(linedp3, wsm4 + 1, s1);                        /* d14 */
        SET_DATA_BYTE(linedp3, wsm4 + 2, s1);                        /* d15 */
        SET_DATA_BYTE(linedp3, wsm4 + 3, s1);                        /* d16 */
    }
        
    return;
}


/*------------------------------------------------------------------*
 *       Grayscale and color scaling by closest pixel sampling      *
 *------------------------------------------------------------------*/
/*!
 *  scaleBySamplingLow()
 *
 *  Notes:
 *      (1) The dest must be cleared prior to this operation,
 *          and we clear it here in the low-level code.
 *      (2) We reuse dest pixels and dest pixel rows whenever
 *          possible.  This speeds the upscaling; downscaling
 *          is done by strict subsampling and is unaffected.
 *      (3) Because we are sampling and not interpolating, this
 *          routine works directly, without conversion to full
 *          RGB color, for 2, 4 or 8 bpp palette color images.
 */
l_int32
scaleBySamplingLow(l_uint32  *datad,
                   l_int32    wd,
                   l_int32    hd,
                   l_int32    wpld,
                   l_uint32  *datas,
                   l_int32    ws,
                   l_int32    hs,
                   l_int32    d,
                   l_int32    wpls)
{
l_int32    i, j, bpld;
l_int32    xs, prevxs, sval;
l_int32   *srow, *scol;
l_uint32   csval;
l_uint32  *lines, *prevlines, *lined, *prevlined;
l_float32  wratio, hratio;

    PROCNAME("scaleBySamplingLow");

        /* clear dest */
    bpld = 4 * wpld;
    memset((char *)datad, 0, hd * bpld);
    
        /* the source row corresponding to dest row i ==> srow[i]
         * the source col corresponding to dest col j ==> scol[j]  */
    if ((srow = (l_int32 *)CALLOC(hd, sizeof(l_int32))) == NULL)
        return ERROR_INT("srow not made", procName, 1);
    if ((scol = (l_int32 *)CALLOC(wd, sizeof(l_int32))) == NULL)
        return ERROR_INT("scol not made", procName, 1);

    wratio = (l_float32)ws / (l_float32)wd;
    hratio = (l_float32)hs / (l_float32)hd;
    for (i = 0; i < hd; i++)
        srow[i] = L_MIN((l_int32)(hratio * i + 0.5), hs - 1);
    for (j = 0; j < wd; j++)
        scol[j] = L_MIN((l_int32)(wratio * j + 0.5), ws - 1);

    prevlines = NULL;
    for (i = 0; i < hd; i++) {
        lines = datas + srow[i] * wpls;
        lined = datad + i * wpld;
        if (lines != prevlines) {  /* make dest from new source row */
            prevxs = -1;
            sval = 0;
            csval = 0;
            switch (d)
            {
            case 2:
                for (j = 0; j < wd; j++) {
                    xs = scol[j];
                    if (xs != prevxs) {  /* get dest pix from source col */
                        sval = GET_DATA_DIBIT(lines, xs);
                        SET_DATA_DIBIT(lined, j, sval);
                        prevxs = xs;
                    }
                    else   /* copy prev dest pix */
                        SET_DATA_DIBIT(lined, j, sval);
                }
                break;
            case 4:
                for (j = 0; j < wd; j++) {
                    xs = scol[j];
                    if (xs != prevxs) {  /* get dest pix from source col */
                        sval = GET_DATA_QBIT(lines, xs);
                        SET_DATA_QBIT(lined, j, sval);
                        prevxs = xs;
                    }
                    else   /* copy prev dest pix */
                        SET_DATA_QBIT(lined, j, sval);
                }
                break;
            case 8:
                for (j = 0; j < wd; j++) {
                    xs = scol[j];
                    if (xs != prevxs) {  /* get dest pix from source col */
                        sval = GET_DATA_BYTE(lines, xs);
                        SET_DATA_BYTE(lined, j, sval);
                        prevxs = xs;
                    }
                    else   /* copy prev dest pix */
                        SET_DATA_BYTE(lined, j, sval);
                }
                break;
            case 16:
                for (j = 0; j < wd; j++) {
                    xs = scol[j];
                    if (xs != prevxs) {  /* get dest pix from source col */
                        sval = GET_DATA_TWO_BYTES(lines, xs);
                        SET_DATA_TWO_BYTES(lined, j, sval);
                        prevxs = xs;
                    }
                    else   /* copy prev dest pix */
                        SET_DATA_TWO_BYTES(lined, j, sval);
                }
                break;
            case 32:
                for (j = 0; j < wd; j++) {
                    xs = scol[j];
                    if (xs != prevxs) {  /* get dest pix from source col */
                        csval = lines[xs];
                        lined[j] = csval;
                        prevxs = xs;
                    }
                    else   /* copy prev dest pix */
                        lined[j] = csval;
                }
                break;
            default:
                return ERROR_INT("pixel depth not supported", procName, 1);
                break;
            }
        }
        else {  /* lines == prevlines; copy prev dest row */
            prevlined = lined - wpld;
            memcpy((char *)lined, (char *)prevlined, bpld);
        }
        prevlines = lines;
    }

    FREE(srow);
    FREE(scol);

    return 0;
}


/*------------------------------------------------------------------*
 *    Color and grayscale downsampling with (antialias) smoothing   *
 *------------------------------------------------------------------*/
/*!
 *  scaleSmoothLow()
 *
 *  Notes:
 *      (1) This function is called on 8 or 32 bpp src and dest images.
 *      (2) size is the full width of the lowpass smoothing filter.
 *          It is correlated with the reduction ratio, being the
 *          nearest integer such that size is approximately equal to hs / hd.
 */
l_int32
scaleSmoothLow(l_uint32  *datad,
               l_int32    wd,
               l_int32    hd,
               l_int32    wpld,
               l_uint32  *datas,
               l_int32    ws,
               l_int32    hs,
               l_int32    d,
               l_int32    wpls,
               l_int32    size)
{
l_int32    i, j, m, n, xstart;
l_int32    val, rval, gval, bval;
l_int32   *srow, *scol;
l_uint32  *lines, *lined, *line, *ppixel;
l_uint32   pixel;
l_float32  wratio, hratio, norm;

    PROCNAME("scaleSmoothLow");

        /* Clear dest */
    memset((char *)datad, 0, 4 * wpld * hd);
    
        /* Each dest pixel at (j,i) is computed as the average
           of size^2 corresponding src pixels.
           We store the UL corner location of the square of
           src pixels that correspond to dest pixel (j,i).
           The are labelled by the arrays srow[i] and scol[j]. */
    if ((srow = (l_int32 *)CALLOC(hd, sizeof(l_int32))) == NULL)
        return ERROR_INT("srow not made", procName, 1);
    if ((scol = (l_int32 *)CALLOC(wd, sizeof(l_int32))) == NULL)
        return ERROR_INT("scol not made", procName, 1);

    norm = 1. / (l_float32)(size * size);
    wratio = (l_float32)ws / (l_float32)wd;
    hratio = (l_float32)hs / (l_float32)hd;
    for (i = 0; i < hd; i++)
        srow[i] = L_MIN((l_int32)(hratio * i), hs - size);
    for (j = 0; j < wd; j++)
        scol[j] = L_MIN((l_int32)(wratio * j), ws - size);

        /* For each dest pixel, compute average */
    if (d == 8) {
        for (i = 0; i < hd; i++) {
            lines = datas + srow[i] * wpls;
            lined = datad + i * wpld;
            for (j = 0; j < wd; j++) {
                xstart = scol[j];
                val = 0;
                for (m = 0; m < size; m++) {
                    line = lines + m * wpls;
                    for (n = 0; n < size; n++) {
                        val += GET_DATA_BYTE(line, xstart + n);
                    }
                }
                val = (l_int32)((l_float32)val * norm);
                SET_DATA_BYTE(lined, j, val);
            }
        }
    }
    else {  /* d == 32 */
        for (i = 0; i < hd; i++) {
            lines = datas + srow[i] * wpls;
            lined = datad + i * wpld;
            for (j = 0; j < wd; j++) {
                xstart = scol[j];
                rval = gval = bval = 0;
                for (m = 0; m < size; m++) {
                    ppixel = lines + m * wpls + xstart;
                    for (n = 0; n < size; n++) {
                        pixel = *(ppixel + n);
                        rval += (pixel >> L_RED_SHIFT) & 0xff;
                        gval += (pixel >> L_GREEN_SHIFT) & 0xff;
                        bval += (pixel >> L_BLUE_SHIFT) & 0xff;
                    }
                }
                rval = (l_int32)((l_float32)rval * norm);
                gval = (l_int32)((l_float32)gval * norm);
                bval = (l_int32)((l_float32)bval * norm);
                *(lined + j) = rval << L_RED_SHIFT |
                               gval << L_GREEN_SHIFT |
                               bval << L_BLUE_SHIFT;
            }
        }
    }

    FREE(srow);
    FREE(scol);
    return 0;
}


/*!
 *  scaleRGBToGray2Low()
 *
 *  Note: This function is called with 32 bpp RGB src and 8 bpp,
 *        half-resolution dest.  The weights should add to 1.0.
 */
void
scaleRGBToGray2Low(l_uint32  *datad,
                   l_int32    wd,
                   l_int32    hd,
                   l_int32    wpld,
                   l_uint32  *datas,
                   l_int32    wpls,
                   l_float32  rwt,
                   l_float32  gwt,
                   l_float32  bwt)
{
l_int32    i, j, val, rval, gval, bval;
l_uint32  *lines, *lined;
l_uint32   pixel;

    rwt *= 0.25;
    gwt *= 0.25;
    bwt *= 0.25;
    for (i = 0; i < hd; i++) {
        lines = datas + 2 * i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < wd; j++) {
                /* Sum each of the color components from 4 src pixels */
            pixel = *(lines + 2 * j);
            rval = (pixel >> L_RED_SHIFT) & 0xff;
            gval = (pixel >> L_GREEN_SHIFT) & 0xff;
            bval = (pixel >> L_BLUE_SHIFT) & 0xff;
            pixel = *(lines + 2 * j + 1);
            rval += (pixel >> L_RED_SHIFT) & 0xff;
            gval += (pixel >> L_GREEN_SHIFT) & 0xff;
            bval += (pixel >> L_BLUE_SHIFT) & 0xff;
            pixel = *(lines + wpls + 2 * j);
            rval += (pixel >> L_RED_SHIFT) & 0xff;
            gval += (pixel >> L_GREEN_SHIFT) & 0xff;
            bval += (pixel >> L_BLUE_SHIFT) & 0xff;
            pixel = *(lines + wpls + 2 * j + 1);
            rval += (pixel >> L_RED_SHIFT) & 0xff;
            gval += (pixel >> L_GREEN_SHIFT) & 0xff;
            bval += (pixel >> L_BLUE_SHIFT) & 0xff;
                /* Generate the dest byte as a weighted sum of the averages */
            val = (l_int32)(rwt * rval + gwt * gval + bwt * bval);
            SET_DATA_BYTE(lined, j, val);
        }
    }
}


/*------------------------------------------------------------------*
 *                  General area mapped gray scaling                *
 *------------------------------------------------------------------*/
/*!
 *  scaleColorAreaMapLow()
 *
 *  This should only be used for downscaling.
 *  We choose to divide each pixel into 16 x 16 sub-pixels.
 *  This is much slower than scaleSmoothLow(), but it gives a
 *  better representation, esp. for downscaling factors between
 *  1.5 and 5.  All src pixels are subdivided into 256 sub-pixels,
 *  and are weighted by the number of sub-pixels covered by
 *  the dest pixel.  This is about 2x slower than scaleSmoothLow(),
 *  but the results are significantly better on small text.
 */
void
scaleColorAreaMapLow(l_uint32  *datad,
                    l_int32    wd,
                    l_int32    hd,
                    l_int32    wpld,
                    l_uint32  *datas,
                    l_int32    ws,
                    l_int32    hs,
                    l_int32    wpls)
{
l_int32    i, j, k, m, wm2, hm2;
l_int32    area00, area10, area01, area11, areal, arear, areat, areab;
l_int32    xu, yu;  /* UL corner in src image, to 1/16 of a pixel */
l_int32    xl, yl;  /* LR corner in src image, to 1/16 of a pixel */
l_int32    xup, yup, xuf, yuf;  /* UL src pixel: integer and fraction */
l_int32    xlp, ylp, xlf, ylf;  /* LR src pixel: integer and fraction */
l_int32    delx, dely, area;
l_int32    v00r, v00g, v00b;  /* contrib. from UL src pixel */
l_int32    v01r, v01g, v01b;  /* contrib. from LL src pixel */
l_int32    v10r, v10g, v10b;  /* contrib from UR src pixel */
l_int32    v11r, v11g, v11b;  /* contrib from LR src pixel */
l_int32    vinr, ving, vinb;  /* contrib from all full interior src pixels */
l_int32    vmidr, vmidg, vmidb;  /* contrib from side parts */
l_int32    rval, gval, bval;
l_uint32   pixel00, pixel10, pixel01, pixel11, pixel;
l_uint32  *lines, *lined;
l_float32  scx, scy;

        /* (scx, scy) are scaling factors that are applied to the
         * dest coords to get the corresponding src coords.
         * We need them because we iterate over dest pixels
         * and must find the corresponding set of src pixels. */
    scx = 16. * (l_float32)ws / (l_float32)wd;
    scy = 16. * (l_float32)hs / (l_float32)hd;

    wm2 = ws - 2;
    hm2 = hs - 2;

        /* Iterate over the destination pixels */
    for (i = 0; i < hd; i++) {
        yu = (l_int32)(scy * i);
        yl = (l_int32)(scy * (i + 1.0));
        yup = yu >> 4;
        yuf = yu & 0x0f;
        ylp = yl >> 4;
        ylf = yl & 0x0f;
        dely = ylp - yup;
        lined = datad + i * wpld;
        lines = datas + yup * wpls;
        for (j = 0; j < wd; j++) {
            xu = (l_int32)(scx * j);
            xl = (l_int32)(scx * (j + 1.0));
            xup = xu >> 4;
            xuf = xu & 0x0f;
            xlp = xl >> 4;
            xlf = xl & 0x0f;
            delx = xlp - xup;

                /* If near the edge, just use a src pixel value */
            if (xlp > wm2 || ylp > hm2) {
                *(lined + j) = *(lines + xup);
                continue;
            }

                /* Area summed over, in subpixels.  This varies
                 * due to the quantization, so we can't simply take
                 * the area to be a constant: area = scx * scy. */
            area = ((16 - xuf) + 16 * (delx - 1) + xlf) *
                   ((16 - yuf) + 16 * (dely - 1) + ylf);

                /* Do area map summation */
            pixel00 = *(lines + xup);
            pixel10 = *(lines + xlp);
            pixel01 = *(lines + dely * wpls +  xup);
            pixel11 = *(lines + dely * wpls +  xlp);
            area00 = (16 - xuf) * (16 - yuf);
            area10 = xlf * (16 - yuf);
            area01 = (16 - xuf) * ylf;
            area11 = xlf * ylf;
            v00r = area00 * ((pixel00 >> L_RED_SHIFT) & 0xff);
            v00g = area00 * ((pixel00 >> L_GREEN_SHIFT) & 0xff);
            v00b = area00 * ((pixel00 >> L_BLUE_SHIFT) & 0xff);
            v10r = area10 * ((pixel10 >> L_RED_SHIFT) & 0xff);
            v10g = area10 * ((pixel10 >> L_GREEN_SHIFT) & 0xff);
            v10b = area10 * ((pixel10 >> L_BLUE_SHIFT) & 0xff);
            v01r = area01 * ((pixel01 >> L_RED_SHIFT) & 0xff);
            v01g = area01 * ((pixel01 >> L_GREEN_SHIFT) & 0xff);
            v01b = area01 * ((pixel01 >> L_BLUE_SHIFT) & 0xff);
            v11r = area11 * ((pixel11 >> L_RED_SHIFT) & 0xff);
            v11g = area11 * ((pixel11 >> L_GREEN_SHIFT) & 0xff);
            v11b = area11 * ((pixel11 >> L_BLUE_SHIFT) & 0xff);
            vinr = ving = vinb = 0;
            for (k = 1; k < dely; k++) {  /* for full src pixels */
                for (m = 1; m < delx; m++) {
                    pixel = *(lines + k * wpls + xup + m);
                    vinr += 256 * ((pixel >> L_RED_SHIFT) & 0xff);
                    ving += 256 * ((pixel >> L_GREEN_SHIFT) & 0xff);
                    vinb += 256 * ((pixel >> L_BLUE_SHIFT) & 0xff);
                }
            }
            vmidr = vmidg = vmidb = 0;
            areal = (16 - xuf) * 16;
            arear = xlf * 16;
            areat = 16 * (16 - yuf);
            areab = 16 * ylf;
            for (k = 1; k < dely; k++) {  /* for left side */
                pixel = *(lines + k * wpls + xup);
                vmidr += areal * ((pixel >> L_RED_SHIFT) & 0xff);
                vmidg += areal * ((pixel >> L_GREEN_SHIFT) & 0xff);
                vmidb += areal * ((pixel >> L_BLUE_SHIFT) & 0xff);
            }
            for (k = 1; k < dely; k++) {  /* for right side */
                pixel = *(lines + k * wpls + xlp);
                vmidr += arear * ((pixel >> L_RED_SHIFT) & 0xff);
                vmidg += arear * ((pixel >> L_GREEN_SHIFT) & 0xff);
                vmidb += arear * ((pixel >> L_BLUE_SHIFT) & 0xff);
            }
            for (m = 1; m < delx; m++) {  /* for top side */
                pixel = *(lines + xup + m);
                vmidr += areat * ((pixel >> L_RED_SHIFT) & 0xff);
                vmidg += areat * ((pixel >> L_GREEN_SHIFT) & 0xff);
                vmidb += areat * ((pixel >> L_BLUE_SHIFT) & 0xff);
            }
            for (m = 1; m < delx; m++) {  /* for bottom side */
                pixel = *(lines + dely * wpls + xup + m);
                vmidr += areab * ((pixel >> L_RED_SHIFT) & 0xff);
                vmidg += areab * ((pixel >> L_GREEN_SHIFT) & 0xff);
                vmidb += areab * ((pixel >> L_BLUE_SHIFT) & 0xff);
            }

                /* Sum all the contributions */
            rval = (v00r + v01r + v10r + v11r + vinr + vmidr + 128) / area;
            gval = (v00g + v01g + v10g + v11g + ving + vmidg + 128) / area;
            bval = (v00b + v01b + v10b + v11b + vinb + vmidb + 128) / area;
#if  DEBUG_OVERFLOW
            if (rval > 255) fprintf(stderr, "rval ovfl: %d\n", rval);
            if (rval > 255) fprintf(stderr, "gval ovfl: %d\n", gval);
            if (rval > 255) fprintf(stderr, "bval ovfl: %d\n", bval);
#endif  /* DEBUG_OVERFLOW */
            composeRGBPixel(rval, gval, bval, lined + j);
        }
    }

    return;
}


/*!
 *  scaleGrayAreaMapLow()
 *
 *  This should only be used for downscaling.
 *  We choose to divide each pixel into 16 x 16 sub-pixels.
 *  This is about 2x slower than scaleSmoothLow(), but the results
 *  are significantly better on small text, esp. for downscaling
 *  factors between 1.5 and 5.  All src pixels are subdivided
 *  into 256 sub-pixels, and are weighted by the number of
 *  sub-pixels covered by the dest pixel.
 */
void
scaleGrayAreaMapLow(l_uint32  *datad,
                    l_int32    wd,
                    l_int32    hd,
                    l_int32    wpld,
                    l_uint32  *datas,
                    l_int32    ws,
                    l_int32    hs,
                    l_int32    wpls)
{
l_int32    i, j, k, m, wm2, hm2;
l_int32    xu, yu;  /* UL corner in src image, to 1/16 of a pixel */
l_int32    xl, yl;  /* LR corner in src image, to 1/16 of a pixel */
l_int32    xup, yup, xuf, yuf;  /* UL src pixel: integer and fraction */
l_int32    xlp, ylp, xlf, ylf;  /* LR src pixel: integer and fraction */
l_int32    delx, dely, area;
l_int32    v00;  /* contrib. from UL src pixel */
l_int32    v01;  /* contrib. from LL src pixel */
l_int32    v10;  /* contrib from UR src pixel */
l_int32    v11;  /* contrib from LR src pixel */
l_int32    vin;  /* contrib from all full interior src pixels */
l_int32    vmid;  /* contrib from side parts that are full in 1 direction */
l_int32    val;
l_uint32  *lines, *lined;
l_float32  scx, scy;

        /* (scx, scy) are scaling factors that are applied to the
         * dest coords to get the corresponding src coords.
         * We need them because we iterate over dest pixels
         * and must find the corresponding set of src pixels. */
    scx = 16. * (l_float32)ws / (l_float32)wd;
    scy = 16. * (l_float32)hs / (l_float32)hd;

    wm2 = ws - 2;
    hm2 = hs - 2;

        /* Iterate over the destination pixels */
    for (i = 0; i < hd; i++) {
        yu = (l_int32)(scy * i);
        yl = (l_int32)(scy * (i + 1.0));
        yup = yu >> 4;
        yuf = yu & 0x0f;
        ylp = yl >> 4;
        ylf = yl & 0x0f;
        dely = ylp - yup;
        lined = datad + i * wpld;
        lines = datas + yup * wpls;
        for (j = 0; j < wd; j++) {
            xu = (l_int32)(scx * j);
            xl = (l_int32)(scx * (j + 1.0));
            xup = xu >> 4;
            xuf = xu & 0x0f;
            xlp = xl >> 4;
            xlf = xl & 0x0f;
            delx = xlp - xup;

                /* If near the edge, just use a src pixel value */
            if (xlp > wm2 || ylp > hm2) {
                SET_DATA_BYTE(lined, j, GET_DATA_BYTE(lines, xup));
                continue;
            }

                /* Area summed over, in subpixels.  This varies
                 * due to the quantization, so we can't simply take
                 * the area to be a constant: area = scx * scy. */
            area = ((16 - xuf) + 16 * (delx - 1) + xlf) *
                   ((16 - yuf) + 16 * (dely - 1) + ylf);

                /* Do area map summation */
            v00 = (16 - xuf) * (16 - yuf) * GET_DATA_BYTE(lines, xup);
            v10 = xlf * (16 - yuf) * GET_DATA_BYTE(lines, xlp);
            v01 = (16 - xuf) * ylf * GET_DATA_BYTE(lines + dely * wpls, xup);
            v11 = xlf * ylf * GET_DATA_BYTE(lines + dely * wpls, xlp);
            for (vin = 0, k = 1; k < dely; k++) {  /* for full src pixels */
                 for (m = 1; m < delx; m++) {
                     vin += 256 * GET_DATA_BYTE(lines + k * wpls, xup + m);
                 }
            }
            for (vmid = 0, k = 1; k < dely; k++)  /* for left side */
                vmid += (16 - xuf) * 16 * GET_DATA_BYTE(lines + k * wpls, xup);
            for (k = 1; k < dely; k++)  /* for right side */
                vmid += xlf * 16 * GET_DATA_BYTE(lines + k * wpls, xlp);
            for (m = 1; m < delx; m++)  /* for top side */
                vmid += 16 * (16 - yuf) * GET_DATA_BYTE(lines, xup + m);
            for (m = 1; m < delx; m++)  /* for bottom side */
                vmid += 16 * ylf * GET_DATA_BYTE(lines + dely * wpls, xup + m);
            val = (v00 + v01 + v10 + v11 + vin + vmid + 128) / area;
#if  DEBUG_OVERFLOW
            if (val > 255) fprintf(stderr, "val overflow: %d\n", val);
#endif  /* DEBUG_OVERFLOW */
            SET_DATA_BYTE(lined, j, val);
        }
    }

    return;
}


/*------------------------------------------------------------------*
 *                     2x area mapped downscaling                   *
 *------------------------------------------------------------------*/
/*!
 *  scaleAreaMapLow2()
 *
 *  Note: This function is called with either 8 bpp gray or 32 bpp RGB.
 *        The result is a 2x reduced dest.
 */
void
scaleAreaMapLow2(l_uint32  *datad,
                 l_int32    wd,
                 l_int32    hd,
                 l_int32    wpld,
                 l_uint32  *datas,
                 l_int32    d,
                 l_int32    wpls)
{
l_int32    i, j, val, rval, gval, bval;
l_uint32  *lines, *lined;
l_uint32   pixel;

    if (d == 8) {
        for (i = 0; i < hd; i++) {
            lines = datas + 2 * i * wpls;
            lined = datad + i * wpld;
            for (j = 0; j < wd; j++) {
                    /* Average each dest pixel using 4 src pixels */
                val = GET_DATA_BYTE(lines, 2 * j);
                val += GET_DATA_BYTE(lines, 2 * j + 1);
                val += GET_DATA_BYTE(lines + wpls, 2 * j);
                val += GET_DATA_BYTE(lines + wpls, 2 * j + 1);
                val >>= 2;
                SET_DATA_BYTE(lined, j, val);
            }
        }
    }
    else {  /* d == 32 */
        for (i = 0; i < hd; i++) {
            lines = datas + 2 * i * wpls;
            lined = datad + i * wpld;
            for (j = 0; j < wd; j++) {
                    /* Average each of the color components from 4 src pixels */
                pixel = *(lines + 2 * j);
                rval = (pixel >> L_RED_SHIFT) & 0xff;
                gval = (pixel >> L_GREEN_SHIFT) & 0xff;
                bval = (pixel >> L_BLUE_SHIFT) & 0xff;
                pixel = *(lines + 2 * j + 1);
                rval += (pixel >> L_RED_SHIFT) & 0xff;
                gval += (pixel >> L_GREEN_SHIFT) & 0xff;
                bval += (pixel >> L_BLUE_SHIFT) & 0xff;
                pixel = *(lines + wpls + 2 * j);
                rval += (pixel >> L_RED_SHIFT) & 0xff;
                gval += (pixel >> L_GREEN_SHIFT) & 0xff;
                bval += (pixel >> L_BLUE_SHIFT) & 0xff;
                pixel = *(lines + wpls + 2 * j + 1);
                rval += (pixel >> L_RED_SHIFT) & 0xff;
                gval += (pixel >> L_GREEN_SHIFT) & 0xff;
                bval += (pixel >> L_BLUE_SHIFT) & 0xff;
                composeRGBPixel(rval >> 2, gval >> 2, bval >> 2, &pixel);
                *(lined + j) = pixel;
            }
        }
    }
    return;
}


/*------------------------------------------------------------------*
 *              Binary scaling by closest pixel sampling            *
 *------------------------------------------------------------------*/
/*
 *  scaleBinaryLow()
 *
 *  Notes:
 *      (1) The dest must be cleared prior to this operation,
 *          and we clear it here in the low-level code.
 *      (2) We reuse dest pixels and dest pixel rows whenever
 *          possible for upscaling; downscaling is done by
 *          strict subsampling.
 */
l_int32
scaleBinaryLow(l_uint32  *datad,
               l_int32    wd,
               l_int32    hd,
               l_int32    wpld,
               l_uint32  *datas,
               l_int32    ws,
               l_int32    hs,
               l_int32    wpls)
{
l_int32    i, j, bpld;
l_int32    xs, prevxs, sval;
l_int32   *srow, *scol;
l_uint32  *lines, *prevlines, *lined, *prevlined;
l_float32  wratio, hratio;

    PROCNAME("scaleBinaryLow");

        /* clear dest */
    bpld = 4 * wpld;
    memset((char *)datad, 0, hd * bpld);
    
        /* The source row corresponding to dest row i ==> srow[i]
         * The source col corresponding to dest col j ==> scol[j]  */
    if ((srow = (l_int32 *)CALLOC(hd, sizeof(l_int32))) == NULL)
        return ERROR_INT("srow not made", procName, 1);
    if ((scol = (l_int32 *)CALLOC(wd, sizeof(l_int32))) == NULL)
        return ERROR_INT("scol not made", procName, 1);

    wratio = (l_float32)ws / (l_float32)wd;
    hratio = (l_float32)hs / (l_float32)hd;
    for (i = 0; i < hd; i++)
        srow[i] = L_MIN((l_int32)(hratio * i + 0.5), hs - 1);
    for (j = 0; j < wd; j++)
        scol[j] = L_MIN((l_int32)(wratio * j + 0.5), ws - 1);

    prevlines = NULL;
    prevxs = -1;
    sval = 0;
    for (i = 0; i < hd; i++) {
        lines = datas + srow[i] * wpls;
        lined = datad + i * wpld;
        if (lines != prevlines) {  /* make dest from new source row */
            for (j = 0; j < wd; j++) {
                xs = scol[j];
                if (xs != prevxs) {  /* get dest pix from source col */
                    if ((sval = GET_DATA_BIT(lines, xs)))
                        SET_DATA_BIT(lined, j);
                    prevxs = xs;
                }
                else {  /* copy prev dest pix, if set */
                    if (sval)
                        SET_DATA_BIT(lined, j);
                }
            }
        }
        else {  /* lines == prevlines; copy prev dest row */
            prevlined = lined - wpld;
            memcpy((char *)lined, (char *)prevlined, bpld);
        }
        prevlines = lines;
    }

    FREE(srow);
    FREE(scol);

    return 0;
}


/*------------------------------------------------------------------*
 *                         Scale-to-gray 2x                         *
 *------------------------------------------------------------------*/
/*!
 *  scaleToGray2Low()
 *
 *      Input:  usual image variables
 *              sumtab  (made from makeSumTabSG2())
 *              valtab  (made from makeValTabSG2())
 *      Return: 0 if OK; 1 on error.
 *
 *  The output is processed in sets of 4 output bytes on a row,
 *  corresponding to 4 2x2 bit-blocks in the input image.
 *  Two lookup tables are used.  The first, sumtab, gets the
 *  sum of ON pixels in 4 sets of two adjacent bits,
 *  storing the result in 4 adjacent bytes.  After sums from
 *  two rows have been added, the second table, valtab,
 *  converts from the sum of ON pixels in the 2x2 block to
 *  an 8 bpp grayscale value between 0 (for 4 bits ON)
 *  and 255 (for 0 bits ON).
 */
void
scaleToGray2Low(l_uint32  *datad,
                l_int32    wd,
                l_int32    hd,
                l_int32    wpld,
                l_uint32  *datas,
                l_int32    wpls,
                l_uint32  *sumtab,
                l_uint8   *valtab)
{
l_int32    i, j, l, k, m, wd4, extra;
l_uint32   sbyte1, sbyte2, sum;
l_uint32  *lines, *lined;

        /* i indexes the dest lines
         * l indexes the source lines
         * j indexes the dest bytes
         * k indexes the source bytes
         * We take two bytes from the source (in 2 lines of 8 pixels
         * each) and convert them into four 8 bpp bytes of the dest. */
    wd4 = wd & 0xfffffffc;
    extra = wd - wd4;
    for (i = 0, l = 0; i < hd; i++, l += 2) {
        lines = datas + l * wpls;
        lined = datad + i * wpld; 
        for (j = 0, k = 0; j < wd4; j += 4, k++) {
            sbyte1 = GET_DATA_BYTE(lines, k);
            sbyte2 = GET_DATA_BYTE(lines + wpls, k);
            sum = sumtab[sbyte1] + sumtab[sbyte2];
            SET_DATA_BYTE(lined, j, valtab[sum >> 24]);
            SET_DATA_BYTE(lined, j + 1, valtab[(sum >> 16) & 0xff]);
            SET_DATA_BYTE(lined, j + 2, valtab[(sum >> 8) & 0xff]);
            SET_DATA_BYTE(lined, j + 3, valtab[sum & 0xff]);
        }
        if (extra > 0) {
            sbyte1 = GET_DATA_BYTE(lines, k);
            sbyte2 = GET_DATA_BYTE(lines + wpls, k);
            sum = sumtab[sbyte1] + sumtab[sbyte2];
            for (m = 0; m < extra; m++) {
                SET_DATA_BYTE(lined, j + m,
                              valtab[((sum >> (24 - 8 * m)) & 0xff)]);
            }
        }
        
    }

    return;
}


/*!
 *  makeSumTabSG2()
 *         
 *  Returns a table of 256 l_uint32s, giving the four output
 *  8-bit grayscale sums corresponding to 8 input bits of a binary
 *  image, for a 2x scale-to-gray op.  The sums from two
 *  adjacent scanlines are then added and transformed to
 *  output four 8 bpp pixel values, using makeValTabSG2().
 */
l_uint32  *
makeSumTabSG2(void)
{
l_int32    i;
l_int32    sum[] = {0, 1, 1, 2};
l_uint32  *tab;

    PROCNAME("makeSumTabSG2");

    if ((tab = (l_uint32 *)CALLOC(256, sizeof(l_uint32))) == NULL)
        return (l_uint32 *)ERROR_PTR("calloc fail for tab", procName, NULL);

        /* Pack the four sums separately in four bytes */
    for (i = 0; i < 256; i++) {
        tab[i] = (sum[i & 0x3] | sum[(i >> 2) & 0x3] << 8 |
                  sum[(i >> 4) & 0x3] << 16 | sum[(i >> 6) & 0x3] << 24);
    }

    return tab;
}


/*!
 *  makeValTabSG2()
 *         
 *  Returns an 8 bit value for the sum of ON pixels
 *  in a 2x2 square, according to
 *
 *         val = 255 - (255 * sum)/4
 *
 *  where sum is in set {0,1,2,3,4}
 */
l_uint8 *
makeValTabSG2(void)
{
l_int32   i;
l_uint8  *tab;

    PROCNAME("makeValTabSG2");

    if ((tab = (l_uint8 *)CALLOC(5, sizeof(l_uint8))) == NULL)
        return (l_uint8 *)ERROR_PTR("calloc fail for tab", procName, NULL);

    for (i = 0; i < 5; i++)
        tab[i] = 255 - (i * 255) / 4;

    return tab;
}


/*------------------------------------------------------------------*
 *                         Scale-to-gray 3x                         *
 *------------------------------------------------------------------*/
/*!
 *  scaleToGray3Low()
 *
 *      Input:  usual image variables
 *              sumtab  (made from makeSumTabSG3())
 *              valtab  (made from makeValTabSG3())
 *      Return: 0 if OK; 1 on error
 *
 *  Each set of 8 3x3 bit-blocks in the source image, which
 *  consist of 72 pixels arranged 24 pixels wide by 3 scanlines,
 *  is converted to a row of 8 8-bit pixels in the dest image.
 *  These 72 pixels of the input image are runs of 24 pixels
 *  in three adjacent scanlines.  Each run of 24 pixels is
 *  stored in the 24 LSbits of a 32-bit word.  We use 2 LUTs.
 *  The first, sumtab, takes 6 of these bits and stores
 *  sum, taken 3 bits at a time, in two bytes.  (See
 *  makeSumTabSG3).  This is done for each of the 3 scanlines,
 *  and the results are added.  We now have the sum of ON pixels
 *  in the first two 3x3 blocks in two bytes.  The valtab LUT
 *  then converts these values (which go from 0 to 9) to
 *  grayscale values between between 255 and 0.  (See makeValTabSG3).
 *  This process is repeated for each of the other 3 sets of
 *  6x3 input pixels, giving 8 output pixels in total.
 *
 *  Note: because the input image is processed in groups of
 *        24 x 3 pixels, the process clips the input height to
 *        (h - h % 3) and the input width to (w - w % 24).
 */
void
scaleToGray3Low(l_uint32  *datad,
                l_int32    wd,
                l_int32    hd,
                l_int32    wpld,
                l_uint32  *datas,
                l_int32    wpls,
                l_uint32  *sumtab,
                l_uint8   *valtab)
{
l_int32    i, j, l, k;
l_uint32   threebytes1, threebytes2, threebytes3, sum;
l_uint32  *lines, *lined;

        /* i indexes the dest lines
         * l indexes the source lines
         * j indexes the dest bytes
         * k indexes the source bytes
         * We take 9 bytes from the source (72 binary pixels
         * in three lines of 24 pixels each) and convert it
         * into 8 bytes of the dest (8 8bpp pixels in one line)   */
    for (i = 0, l = 0; i < hd; i++, l += 3) {
        lines = datas + l * wpls;
        lined = datad + i * wpld; 
        for (j = 0, k = 0; j < wd; j += 8, k += 3) {
            threebytes1 = (GET_DATA_BYTE(lines, k) << 16) |
                          (GET_DATA_BYTE(lines, k + 1) << 8) |
                          GET_DATA_BYTE(lines, k + 2);
            threebytes2 = (GET_DATA_BYTE(lines + wpls, k) << 16) |
                          (GET_DATA_BYTE(lines + wpls, k + 1) << 8) |
                          GET_DATA_BYTE(lines + wpls, k + 2);
            threebytes3 = (GET_DATA_BYTE(lines + 2 * wpls, k) << 16) |
                          (GET_DATA_BYTE(lines + 2 * wpls, k + 1) << 8) |
                          GET_DATA_BYTE(lines + 2 * wpls, k + 2);

            sum = sumtab[(threebytes1 >> 18)] +
                  sumtab[(threebytes2 >> 18)] +
                  sumtab[(threebytes3 >> 18)];
            SET_DATA_BYTE(lined, j, valtab[GET_DATA_BYTE(&sum, 2)]);
            SET_DATA_BYTE(lined, j + 1, valtab[GET_DATA_BYTE(&sum, 3)]);

            sum = sumtab[((threebytes1 >> 12) & 0x3f)] +
                  sumtab[((threebytes2 >> 12) & 0x3f)] +
                  sumtab[((threebytes3 >> 12) & 0x3f)];
            SET_DATA_BYTE(lined, j + 2, valtab[GET_DATA_BYTE(&sum, 2)]);
            SET_DATA_BYTE(lined, j + 3, valtab[GET_DATA_BYTE(&sum, 3)]);

            sum = sumtab[((threebytes1 >> 6) & 0x3f)] +
                  sumtab[((threebytes2 >> 6) & 0x3f)] +
                  sumtab[((threebytes3 >> 6) & 0x3f)];
            SET_DATA_BYTE(lined, j + 4, valtab[GET_DATA_BYTE(&sum, 2)]);
            SET_DATA_BYTE(lined, j + 5, valtab[GET_DATA_BYTE(&sum, 3)]);

            sum = sumtab[(threebytes1 & 0x3f)] +
                  sumtab[(threebytes2 & 0x3f)] +
                  sumtab[(threebytes3 & 0x3f)];
            SET_DATA_BYTE(lined, j + 6, valtab[GET_DATA_BYTE(&sum, 2)]);
            SET_DATA_BYTE(lined, j + 7, valtab[GET_DATA_BYTE(&sum, 3)]);
        }
    }

    return;
}



/*!
 *  makeSumTabSG3()
 *         
 *  Returns a table of 64 l_uint32s, giving the two output
 *  8-bit grayscale sums corresponding to 6 input bits of a binary
 *  image, for a 3x scale-to-gray op.  In practice, this would
 *  be used three times (on adjacent scanlines), and the sums would
 *  be added and then transformed to output 8 bpp pixel values,
 *  using makeValTabSG3().
 */
l_uint32  *
makeSumTabSG3(void)
{
l_int32    i;
l_int32    sum[] = {0, 1, 1, 2, 1, 2, 2, 3};
l_uint32  *tab;

    PROCNAME("makeSumTabSG3");

    if ((tab = (l_uint32 *)CALLOC(64, sizeof(l_uint32))) == NULL)
        return (l_uint32 *)ERROR_PTR("calloc fail for tab", procName, NULL);

        /* Pack the two sums separately in two bytes */
    for (i = 0; i < 64; i++) {
        tab[i] = (sum[i & 0x07]) | (sum[(i >> 3) & 0x07] << 8);
    }

    return tab;
}


/*!
 *  makeValTabSG3()
 *         
 *  Returns an 8 bit value for the sum of ON pixels
 *  in a 3x3 square, according to
 *      val = 255 - (255 * sum)/9
 *  where sum is in set {0, ... ,9}
 */
l_uint8 *
makeValTabSG3(void)
{
l_int32   i;
l_uint8  *tab;

    PROCNAME("makeValTabSG3");

    if ((tab = (l_uint8 *)CALLOC(10, sizeof(l_uint8))) == NULL)
        return (l_uint8 *)ERROR_PTR("calloc fail for tab", procName, NULL);

    for (i = 0; i < 10; i++)
        tab[i] = 0xff - (i * 255) / 9;

    return tab;
}


/*------------------------------------------------------------------*
 *                         Scale-to-gray 4x                         *
 *------------------------------------------------------------------*/
/*!
 *  scaleToGray4Low()
 *
 *      Input:  usual image variables
 *              sumtab  (made from makeSumTabSG4())
 *              valtab  (made from makeValTabSG4())
 *      Return: 0 if OK; 1 on error.
 *
 *  The output is processed in sets of 2 output bytes on a row,
 *  corresponding to 2 4x4 bit-blocks in the input image.
 *  Two lookup tables are used.  The first, sumtab, gets the
 *  sum of ON pixels in two sets of four adjacent bits,
 *  storing the result in 2 adjacent bytes.  After sums from
 *  four rows have been added, the second table, valtab,
 *  converts from the sum of ON pixels in the 4x4 block to
 *  an 8 bpp grayscale value between 0 (for 16 bits ON)
 *  and 255 (for 0 bits ON).
 */
void
scaleToGray4Low(l_uint32  *datad,
                l_int32    wd,
                l_int32    hd,
                l_int32    wpld,
                l_uint32  *datas,
                l_int32    wpls,
                l_uint32  *sumtab,
                l_uint8   *valtab)
{
l_int32    i, j, l, k;
l_uint32   sbyte1, sbyte2, sbyte3, sbyte4, sum;
l_uint32  *lines, *lined;

        /* i indexes the dest lines
         * l indexes the source lines
         * j indexes the dest bytes
         * k indexes the source bytes
         * We take four bytes from the source (in 4 lines of 8 pixels
         * each) and convert it into two 8 bpp bytes of the dest. */
    for (i = 0, l = 0; i < hd; i++, l += 4) {
        lines = datas + l * wpls;
        lined = datad + i * wpld; 
        for (j = 0, k = 0; j < wd; j += 2, k++) {
            sbyte1 = GET_DATA_BYTE(lines, k);
            sbyte2 = GET_DATA_BYTE(lines + wpls, k);
            sbyte3 = GET_DATA_BYTE(lines + 2 * wpls, k);
            sbyte4 = GET_DATA_BYTE(lines + 3 * wpls, k);
            sum = sumtab[sbyte1] + sumtab[sbyte2] +
                  sumtab[sbyte3] + sumtab[sbyte4];
            SET_DATA_BYTE(lined, j, valtab[GET_DATA_BYTE(&sum, 2)]);
            SET_DATA_BYTE(lined, j + 1, valtab[GET_DATA_BYTE(&sum, 3)]);
        }
    }

    return;
}


/*!
 *  makeSumTabSG4()
 *         
 *  Returns a table of 256 l_uint32s, giving the two output
 *  8-bit grayscale sums corresponding to 8 input bits of a binary
 *  image, for a 4x scale-to-gray op.  The sums from four
 *  adjacent scanlines are then added and transformed to
 *  output 8 bpp pixel values, using makeValTabSG4().
 */
l_uint32  *
makeSumTabSG4(void)
{
l_int32    i;
l_int32    sum[] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
l_uint32  *tab;

    PROCNAME("makeSumTabSG4");

    if ((tab = (l_uint32 *)CALLOC(256, sizeof(l_uint32))) == NULL)
        return (l_uint32 *)ERROR_PTR("calloc fail for tab", procName, NULL);

        /* Pack the two sums separately in two bytes */
    for (i = 0; i < 256; i++) {
        tab[i] = (sum[i & 0xf]) | (sum[(i >> 4) & 0xf] << 8);
    }

    return tab;
}


/*!
 *  makeValTabSG4()
 *         
 *  Returns an 8 bit value for the sum of ON pixels
 *  in a 4x4 square, according to
 *
 *         val = 255 - (255 * sum)/16
 *
 *  where sum is in set {0, ... ,16}
 */
l_uint8 *
makeValTabSG4(void)
{
l_int32   i;
l_uint8  *tab;

    PROCNAME("makeValTabSG4");

    if ((tab = (l_uint8 *)CALLOC(17, sizeof(l_uint8))) == NULL)
        return (l_uint8 *)ERROR_PTR("calloc fail for tab", procName, NULL);

    for (i = 0; i < 17; i++)
        tab[i] = 0xff - (i * 255) / 16;

    return tab;
}


/*------------------------------------------------------------------*
 *                         Scale-to-gray 6x                         *
 *------------------------------------------------------------------*/
/*!
 *  scaleToGray6Low()
 *
 *      Input:  usual image variables
 *              tab8  (made from makePixelSumTab8())
 *              valtab  (made from makeValTabSG6())
 *      Return: 0 if OK; 1 on error
 *
 *  Each set of 4 6x6 bit-blocks in the source image, which
 *  consist of 144 pixels arranged 24 pixels wide by 6 scanlines,
 *  is converted to a row of 4 8-bit pixels in the dest image.
 *  These 144 pixels of the input image are runs of 24 pixels
 *  in six adjacent scanlines.  Each run of 24 pixels is
 *  stored in the 24 LSbits of a 32-bit word.  We use 2 LUTs.
 *  The first, tab8, takes 6 of these bits and stores
 *  sum in one byte.  This is done for each of the 6 scanlines,
 *  and the results are added.
 *  We now have the sum of ON pixels in the first 6x6 block.  The
 *  valtab LUT then converts these values (which go from 0 to 36) to
 *  grayscale values between between 255 and 0.  (See makeValTabSG6).
 *  This process is repeated for each of the other 3 sets of
 *  6x6 input pixels, giving 4 output pixels in total.
 *
 *  Note: because the input image is processed in groups of
 *        24 x 6 pixels, the process clips the input height to
 *        (h - h % 6) and the input width to (w - w % 24).
 *
 */
void
scaleToGray6Low(l_uint32  *datad,
                l_int32    wd,
                l_int32    hd,
                l_int32    wpld,
                l_uint32  *datas,
                l_int32    wpls,
                l_int32   *tab8,
                l_uint8   *valtab)
{
l_int32    i, j, l, k;
l_uint32   threebytes1, threebytes2, threebytes3;
l_uint32   threebytes4, threebytes5, threebytes6, sum;
l_uint32  *lines, *lined;

        /* i indexes the dest lines
         * l indexes the source lines
         * j indexes the dest bytes
         * k indexes the source bytes
         * We take 18 bytes from the source (144 binary pixels
         * in six lines of 24 pixels each) and convert it
         * into 4 bytes of the dest (four 8 bpp pixels in one line)   */
    for (i = 0, l = 0; i < hd; i++, l += 6) {
        lines = datas + l * wpls;
        lined = datad + i * wpld; 
        for (j = 0, k = 0; j < wd; j += 4, k += 3) {
                /* First grab the 18 bytes, 3 at a time, and put each set
                 * of 3 bytes into the LS bytes of a 32-bit word. */
            threebytes1 = (GET_DATA_BYTE(lines, k) << 16) |
                          (GET_DATA_BYTE(lines, k + 1) << 8) |
                          GET_DATA_BYTE(lines, k + 2);
            threebytes2 = (GET_DATA_BYTE(lines + wpls, k) << 16) |
                          (GET_DATA_BYTE(lines + wpls, k + 1) << 8) |
                          GET_DATA_BYTE(lines + wpls, k + 2);
            threebytes3 = (GET_DATA_BYTE(lines + 2 * wpls, k) << 16) |
                          (GET_DATA_BYTE(lines + 2 * wpls, k + 1) << 8) |
                          GET_DATA_BYTE(lines + 2 * wpls, k + 2);
            threebytes4 = (GET_DATA_BYTE(lines + 3 * wpls, k) << 16) |
                          (GET_DATA_BYTE(lines + 3 * wpls, k + 1) << 8) |
                          GET_DATA_BYTE(lines + 3 * wpls, k + 2);
            threebytes5 = (GET_DATA_BYTE(lines + 4 * wpls, k) << 16) |
                          (GET_DATA_BYTE(lines + 4 * wpls, k + 1) << 8) |
                          GET_DATA_BYTE(lines + 4 * wpls, k + 2);
            threebytes6 = (GET_DATA_BYTE(lines + 5 * wpls, k) << 16) |
                          (GET_DATA_BYTE(lines + 5 * wpls, k + 1) << 8) |
                          GET_DATA_BYTE(lines + 5 * wpls, k + 2);

                /* Sum first set of 36 bits and convert to 0-255 */
            sum = tab8[(threebytes1 >> 18)] +
                  tab8[(threebytes2 >> 18)] +
                  tab8[(threebytes3 >> 18)] +
                  tab8[(threebytes4 >> 18)] +
                  tab8[(threebytes5 >> 18)] +
                   tab8[(threebytes6 >> 18)];
            SET_DATA_BYTE(lined, j, valtab[GET_DATA_BYTE(&sum, 3)]);

                /* Ditto for second set */
            sum = tab8[((threebytes1 >> 12) & 0x3f)] +
                  tab8[((threebytes2 >> 12) & 0x3f)] +
                  tab8[((threebytes3 >> 12) & 0x3f)] +
                  tab8[((threebytes4 >> 12) & 0x3f)] +
                  tab8[((threebytes5 >> 12) & 0x3f)] +
                  tab8[((threebytes6 >> 12) & 0x3f)];
            SET_DATA_BYTE(lined, j + 1, valtab[GET_DATA_BYTE(&sum, 3)]);

            sum = tab8[((threebytes1 >> 6) & 0x3f)] +
                  tab8[((threebytes2 >> 6) & 0x3f)] +
                  tab8[((threebytes3 >> 6) & 0x3f)] +
                  tab8[((threebytes4 >> 6) & 0x3f)] +
                  tab8[((threebytes5 >> 6) & 0x3f)] +
                  tab8[((threebytes6 >> 6) & 0x3f)];
            SET_DATA_BYTE(lined, j + 2, valtab[GET_DATA_BYTE(&sum, 3)]);

            sum = tab8[(threebytes1 & 0x3f)] +
                  tab8[(threebytes2 & 0x3f)] +
                  tab8[(threebytes3 & 0x3f)] +
                  tab8[(threebytes4 & 0x3f)] +
                  tab8[(threebytes5 & 0x3f)] +
                  tab8[(threebytes6 & 0x3f)];
            SET_DATA_BYTE(lined, j + 3, valtab[GET_DATA_BYTE(&sum, 3)]);
        }
    }

    return;
}


/*!
 *  makeValTabSG6()
 *         
 *  Returns an 8 bit value for the sum of ON pixels
 *  in a 6x6 square, according to
 *      val = 255 - (255 * sum)/36
 *  where sum is in set {0, ... ,36}
 */
l_uint8 *
makeValTabSG6(void)
{
l_int32   i;
l_uint8  *tab;

    PROCNAME("makeValTabSG6");

    if ((tab = (l_uint8 *)CALLOC(37, sizeof(l_uint8))) == NULL)
        return (l_uint8 *)ERROR_PTR("calloc fail for tab", procName, NULL);

    for (i = 0; i < 37; i++)
        tab[i] = 0xff - (i * 255) / 36;

    return tab;
}


/*------------------------------------------------------------------*
 *                         Scale-to-gray 8x                         *
 *------------------------------------------------------------------*/
/*!
 *  scaleToGray8Low()
 *
 *      Input:  usual image variables
 *              tab8  (made from makePixelSumTab8())
 *              valtab  (made from makeValTabSG8())
 *      Return: 0 if OK; 1 on error.
 *
 *  The output is processed one dest byte at a time,
 *  corresponding to 8 rows of src bytes in the input image.
 *  Two lookup tables are used.  The first, tab8, gets the
 *  sum of ON pixels in a byte.  After sums from 8 rows have
 *  been added, the second table, valtab, converts from this
 *  value (which is between 0 and 64) to an 8 bpp grayscale
 *  value between 0 (for all 64 bits ON) and 255 (for 0 bits ON).
 */
void
scaleToGray8Low(l_uint32  *datad,
                l_int32    wd,
                l_int32    hd,
                l_int32    wpld,
                l_uint32  *datas,
                l_int32    wpls,
                l_int32   *tab8,
                l_uint8   *valtab)
{
l_int32    i, j, k;
l_int32    sbyte0, sbyte1, sbyte2, sbyte3, sbyte4, sbyte5, sbyte6, sbyte7, sum;
l_uint32  *lines, *lined;

        /* i indexes the dest lines
         * k indexes the source lines
         * j indexes the src and dest bytes
         * We take 8 bytes from the source (in 8 lines of 8 pixels
         * each) and convert it into one 8 bpp byte of the dest. */
    for (i = 0, k = 0; i < hd; i++, k += 8) {
        lines = datas + k * wpls;
        lined = datad + i * wpld; 
        for (j = 0; j < wd; j++) {
            sbyte0 = GET_DATA_BYTE(lines, j);
            sbyte1 = GET_DATA_BYTE(lines + wpls, j);
            sbyte2 = GET_DATA_BYTE(lines + 2 * wpls, j);
            sbyte3 = GET_DATA_BYTE(lines + 3 * wpls, j);
            sbyte4 = GET_DATA_BYTE(lines + 4 * wpls, j);
            sbyte5 = GET_DATA_BYTE(lines + 5 * wpls, j);
            sbyte6 = GET_DATA_BYTE(lines + 6 * wpls, j);
            sbyte7 = GET_DATA_BYTE(lines + 7 * wpls, j);
            sum = tab8[sbyte0] + tab8[sbyte1] +
                  tab8[sbyte2] + tab8[sbyte3] +
                  tab8[sbyte4] + tab8[sbyte5] +
                  tab8[sbyte6] + tab8[sbyte7];
            SET_DATA_BYTE(lined, j, valtab[sum]);
        }
    }

    return;
}


/*!
 *  makeValTabSG8()
 *         
 *  Returns an 8 bit value for the sum of ON pixels
 *  in an 8x8 square, according to
 *      val = 255 - (255 * sum)/64
 *  where sum is in set {0, ... ,64}
 */
l_uint8 *
makeValTabSG8(void)
{
l_int32   i;
l_uint8  *tab;

    PROCNAME("makeValTabSG8");

    if ((tab = (l_uint8 *)CALLOC(65, sizeof(l_uint8))) == NULL)
        return (l_uint8 *)ERROR_PTR("calloc fail for tab", procName, NULL);

    for (i = 0; i < 65; i++)
        tab[i] = 0xff - (i * 255) / 64;

    return tab;
}


/*------------------------------------------------------------------*
 *                         Scale-to-gray 16x                        *
 *------------------------------------------------------------------*/
/*!
 *  scaleToGray16Low()
 *
 *      Input:  usual image variables
 *              tab8  (made from makePixelSumTab8())
 *      Return: 0 if OK; 1 on error.
 *
 *  The output is processed one dest byte at a time, corresponding
 *  to 16 rows consisting each of 2 src bytes in the input image.
 *  This uses one lookup table, tab8, which gives the sum of
 *  ON pixels in a byte.  After summing for all ON pixels in the
 *  32 src bytes, which is between 0 and 256, this is converted
 *  to an 8 bpp grayscale value between 0 (for 255 or 256 bits ON)
 *  and 255 (for 0 bits ON).
 */
void
scaleToGray16Low(l_uint32  *datad,
                  l_int32    wd,
                 l_int32    hd,
                 l_int32    wpld,
                 l_uint32  *datas,
                 l_int32    wpls,
                 l_int32   *tab8)
{
l_int32    i, j, k, m;
l_int32    sum;
l_uint32  *lines, *lined;

        /* i indexes the dest lines
         * k indexes the source lines
         * j indexes the dest bytes
         * m indexes the src bytes
         * We take 32 bytes from the source (in 16 lines of 16 pixels
         * each) and convert it into one 8 bpp byte of the dest. */
    for (i = 0, k = 0; i < hd; i++, k += 16) {
        lines = datas + k * wpls;
        lined = datad + i * wpld; 
        for (j = 0; j < wd; j++) {
            m = 2 * j;
            sum = tab8[GET_DATA_BYTE(lines, m)];
            sum += tab8[GET_DATA_BYTE(lines, m + 1)];
            sum += tab8[GET_DATA_BYTE(lines + wpls, m)];
            sum += tab8[GET_DATA_BYTE(lines + wpls, m + 1)];
            sum += tab8[GET_DATA_BYTE(lines + 2 * wpls, m)];
            sum += tab8[GET_DATA_BYTE(lines + 2 * wpls, m + 1)];
            sum += tab8[GET_DATA_BYTE(lines + 3 * wpls, m)];
            sum += tab8[GET_DATA_BYTE(lines + 3 * wpls, m + 1)];
            sum += tab8[GET_DATA_BYTE(lines + 4 * wpls, m)];
            sum += tab8[GET_DATA_BYTE(lines + 4 * wpls, m + 1)];
            sum += tab8[GET_DATA_BYTE(lines + 5 * wpls, m)];
            sum += tab8[GET_DATA_BYTE(lines + 5 * wpls, m + 1)];
            sum += tab8[GET_DATA_BYTE(lines + 6 * wpls, m)];
            sum += tab8[GET_DATA_BYTE(lines + 6 * wpls, m + 1)];
            sum += tab8[GET_DATA_BYTE(lines + 7 * wpls, m)];
            sum += tab8[GET_DATA_BYTE(lines + 7 * wpls, m + 1)];
            sum += tab8[GET_DATA_BYTE(lines + 8 * wpls, m)];
            sum += tab8[GET_DATA_BYTE(lines + 8 * wpls, m + 1)];
            sum += tab8[GET_DATA_BYTE(lines + 9 * wpls, m)];
            sum += tab8[GET_DATA_BYTE(lines + 9 * wpls, m + 1)];
            sum += tab8[GET_DATA_BYTE(lines + 10 * wpls, m)];
            sum += tab8[GET_DATA_BYTE(lines + 10 * wpls, m + 1)];
            sum += tab8[GET_DATA_BYTE(lines + 11 * wpls, m)];
            sum += tab8[GET_DATA_BYTE(lines + 11 * wpls, m + 1)];
            sum += tab8[GET_DATA_BYTE(lines + 12 * wpls, m)];
            sum += tab8[GET_DATA_BYTE(lines + 12 * wpls, m + 1)];
            sum += tab8[GET_DATA_BYTE(lines + 13 * wpls, m)];
            sum += tab8[GET_DATA_BYTE(lines + 13 * wpls, m + 1)];
            sum += tab8[GET_DATA_BYTE(lines + 14 * wpls, m)];
            sum += tab8[GET_DATA_BYTE(lines + 14 * wpls, m + 1)];
            sum += tab8[GET_DATA_BYTE(lines + 15 * wpls, m)];
            sum += tab8[GET_DATA_BYTE(lines + 15 * wpls, m + 1)];
            sum = L_MIN(sum, 255);
            SET_DATA_BYTE(lined, j, 255 - sum);
        }
    }

    return;
}



/*------------------------------------------------------------------*
 *                         Grayscale mipmap                         *
 *------------------------------------------------------------------*/
/*!
 *  scaleMipmapLow()
 *
 *  See notes in scale.c for pixScaleToGrayMipmap().  This function
 *  is here for pedagogical reasons.  It gives poor results on document
 *  images because of aliasing.
 */
l_int32
scaleMipmapLow(l_uint32  *datad,
               l_int32    wd,
               l_int32    hd,
               l_int32    wpld,
               l_uint32  *datas1,
               l_int32    wpls1,
               l_uint32  *datas2,
               l_int32    wpls2,
               l_float32  red)
{
l_int32    i, j, val1, val2, val, row2, col2;
l_int32   *srow, *scol;
l_uint32  *lines1, *lines2, *lined;
l_float32  ratio, w1, w2;

    PROCNAME("scaleMipmapLow");

        /* Clear dest */
    memset((char *)datad, 0, 4 * wpld * hd);
    
        /* Each dest pixel at (j,i) is computed by interpolating
           between the two src images at the corresponding location.
           We store the UL corner locations of the square of
           src pixels in thelower-resolution image that correspond
           to dest pixel (j,i).  The are labelled by the arrays
           srow[i], scol[j].  The UL corner locations of the higher
           resolution src pixels are obtained from these arrays
           by multiplying by 2. */
    if ((srow = (l_int32 *)CALLOC(hd, sizeof(l_int32))) == NULL)
        return ERROR_INT("srow not made", procName, 1);
    if ((scol = (l_int32 *)CALLOC(wd, sizeof(l_int32))) == NULL)
        return ERROR_INT("scol not made", procName, 1);
    ratio = 1. / (2. * red);  /* 0.5 for red = 1, 1 for red = 0.5 */
    for (i = 0; i < hd; i++)
        srow[i] = (l_int32)(ratio * i);
    for (j = 0; j < wd; j++)
        scol[j] = (l_int32)(ratio * j);

        /* Get weights for linear interpolation: these are the
         * 'distances' of the dest image plane from the two
         * src image planes. */
    w1 = 2. * red - 1.;   /* w1 --> 1 as red --> 1 */
    w2 = 1. - w1;

        /* For each dest pixel, compute linear interpolation */
    for (i = 0; i < hd; i++) {
        row2 = srow[i];
        lines1 = datas1 + 2 * row2 * wpls1;
        lines2 = datas2 + row2 * wpls2;
        lined = datad + i * wpld;
        for (j = 0; j < wd; j++) {
            col2 = scol[j];
            val1 = GET_DATA_BYTE(lines1, 2 * col2);
            val2 = GET_DATA_BYTE(lines2, col2);
            val = (l_int32)(w1 * val1 + w2 * val2);
            SET_DATA_BYTE(lined, j, val);
        }
    }

    FREE(srow);
    FREE(scol);
    return 0;
}
