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
 *  grayquantlow.c
 *
 *      Thresholding from 8 bpp to 1 bpp
 *
 *          Floyd-Steinberg dithering to binary
 *              void       ditherToBinaryLow()
 *              void       ditherToBinaryLineLow()
 *
 *          Simple (pixelwise) binarization
 *              void       thresholdToBinaryLow()
 *              void       thresholdToBinaryLineLow()
 *
 *          A slower version of Floyd-Steinberg dithering that uses LUTs
 *              void       ditherToBinaryLUTLow()
 *              void       ditherToBinaryLineLUTLow()
 *              l_int32    make8To1DitherTables()
 *
 *      Thresholding from 8 bpp to 2 bpp
 *
 *          Floyd-Steinberg-like dithering to 2 bpp
 *              void       ditherTo2bppLow()
 *              void       ditherTo2bppLineLow()
 *              l_int32    make8To2DitherTables()
 *
 *          Simple thresholding to 2 bpp
 *              void       thresholdTo2bppLow()
 *
 *      Thresholding from 8 bpp to 4 bpp
 *
 *          Simple thresholding to 4 bpp
 *              void       thresholdTo4bppLow()
 */

#include <string.h>
#include "allheaders.h"

#ifndef  NO_CONSOLE_IO
#define DEBUG_UNROLLING 0
#endif   /* ~NO_CONSOLE_IO */


/*------------------------------------------------------------------*
 *             Binarization by Floyd-Steinberg Dithering            *
 *------------------------------------------------------------------*/
/*
 *  ditherToBinaryLow()
 *
 *  See comments in pixDitherToBinary() in binarize.c
 */
void
ditherToBinaryLow(l_uint32  *datad,
                  l_int32    w,
                  l_int32    h,
                  l_int32    wpld,
                  l_uint32  *datas,
                  l_int32    wpls,
                  l_uint32  *bufs1,
                  l_uint32  *bufs2,
                  l_int32    lowerclip,
                  l_int32    upperclip)
{
l_int32      i;
l_uint32    *lined;

        /* do all lines except last line */
    memcpy(bufs2, datas, 4 * wpls);  /* prime the buffer */
    for (i = 0; i < h - 1; i++) {
        memcpy(bufs1, bufs2, 4 * wpls);
        memcpy(bufs2, datas + (i + 1) * wpls, 4 * wpls);
        lined = datad + i * wpld;
        ditherToBinaryLineLow(lined, w, bufs1, bufs2, lowerclip, upperclip, 0);
    }

        /* do last line */
    memcpy(bufs1, bufs2, 4 * wpls);
    lined = datad + (h - 1) * wpld;
    ditherToBinaryLineLow(lined, w, bufs1, bufs2, lowerclip, upperclip, 1);
    return;
}


/*
 *  ditherToBinaryLineLow()
 *
 *      Input:  lined  (ptr to beginning of dest line
 *              w   (width of image in pixels)
 *              bufs1 (buffer of current source line)
 *              bufs2 (buffer of next source line)
 *              lowerclip (lower clip distance to black)
 *              upperclip (upper clip distance to white)
 *              lastlineflag  (0 if not last dest line, 1 if last dest line)
 *      Return: void
 *
 *  Dispatches FS error diffusion dithering for
 *  a single line of the image.  If lastlineflag == 0,
 *  both source buffers are used; otherwise, only bufs1
 *  is used.  We use source buffers because the error
 *  is propagated into them, and we don't want to change
 *  the input src image.
 *
 *  We break dithering out line by line to make it
 *  easier to combine functions like interpolative
 *  scaling and error diffusion dithering, as such a
 *  combination of operations obviates the need to
 *  generate a 2x grayscale image as an intermediary.
 */
void
ditherToBinaryLineLow(l_uint32  *lined,
                      l_int32    w,
                      l_uint32  *bufs1,
                      l_uint32  *bufs2,
                      l_int32    lowerclip,
                      l_int32    upperclip,
                      l_int32    lastlineflag)
{
l_int32   j;
l_int32   oval, eval;
l_uint8   fval1, fval2, rval, bval, dval;

    if (lastlineflag == 0) {
        for (j = 0; j < w - 1; j++) {
            oval = GET_DATA_BYTE(bufs1, j);
            if (oval > 127) {   /* binarize to OFF */
                if ((eval = 255 - oval) > upperclip) {
                        /* subtract from neighbors */
                    fval1 = (3 * eval) / 8;
                    fval2 = eval / 4;
                    rval = GET_DATA_BYTE(bufs1, j + 1);
                    rval = L_MAX(0, rval - fval1);
                    SET_DATA_BYTE(bufs1, j + 1, rval);
                    bval = GET_DATA_BYTE(bufs2, j);
                    bval = L_MAX(0, bval - fval1);
                    SET_DATA_BYTE(bufs2, j, bval);
                    dval = GET_DATA_BYTE(bufs2, j + 1);
                    dval = L_MAX(0, dval - fval2);
                    SET_DATA_BYTE(bufs2, j + 1, dval);
                }
            } else {   /* oval <= 127; binarize to ON  */
                SET_DATA_BIT(lined, j);   /* ON pixel */
                if (oval > lowerclip) {
                        /* add to neighbors */
                    fval1 = (3 * oval) / 8;
                    fval2 = oval / 4;
                    rval = GET_DATA_BYTE(bufs1, j + 1);
                    rval = L_MIN(255, rval + fval1);
                    SET_DATA_BYTE(bufs1, j + 1, rval);
                    bval = GET_DATA_BYTE(bufs2, j);
                    bval = L_MIN(255, bval + fval1);
                    SET_DATA_BYTE(bufs2, j, bval);
                    dval = GET_DATA_BYTE(bufs2, j + 1);
                    dval = L_MIN(255, dval + fval2);
                    SET_DATA_BYTE(bufs2, j + 1, dval);
                }
            }
        }

            /* do last column: j = w - 1 */
        oval = GET_DATA_BYTE(bufs1, j);
        if (oval > 127) {  /* binarize to OFF */
            if ((eval = 255 - oval) > upperclip) {
                    /* subtract from neighbors */
                fval1 = (3 * eval) / 8;
                bval = GET_DATA_BYTE(bufs2, j);
                bval = L_MAX(0, bval - fval1);
                SET_DATA_BYTE(bufs2, j, bval);
            }
        } else {  /*oval <= 127; binarize to ON */
            SET_DATA_BIT(lined, j);   /* ON pixel */
            if (oval > lowerclip) {
                    /* add to neighbors */
                fval1 = (3 * oval) / 8;
                bval = GET_DATA_BYTE(bufs2, j);
                bval = L_MIN(255, bval + fval1);
                SET_DATA_BYTE(bufs2, j, bval);
            }
        }
    } else {   /* lastlineflag == 1 */
        for (j = 0; j < w - 1; j++) {
            oval = GET_DATA_BYTE(bufs1, j);
            if (oval > 127) {   /* binarize to OFF */
                if ((eval = 255 - oval) > upperclip) {
                        /* subtract from neighbors */
                    fval1 = (3 * eval) / 8;
                    rval = GET_DATA_BYTE(bufs1, j + 1);
                    rval = L_MAX(0, rval - fval1);
                    SET_DATA_BYTE(bufs1, j + 1, rval);
                }
            } else {   /* oval <= 127; binarize to ON  */
                SET_DATA_BIT(lined, j);   /* ON pixel */
                if (oval > lowerclip) {
                        /* add to neighbors */
                    fval1 = (3 * oval) / 8;
                    rval = GET_DATA_BYTE(bufs1, j + 1);
                    rval = L_MIN(255, rval + fval1);
                    SET_DATA_BYTE(bufs1, j + 1, rval);
                }
            }
        }

            /* do last pixel: (i, j) = (h - 1, w - 1) */
        oval = GET_DATA_BYTE(bufs1, j);
        if (oval < 128)
            SET_DATA_BIT(lined, j);   /* ON pixel */
    }

    return;
}



/*------------------------------------------------------------------*
 *             Simple binarization with fixed threshold             *
 *------------------------------------------------------------------*/
/*
 *  thresholdToBinaryLow()
 *
 *  If the source pixel is less than thresh,
 *  the dest will be 1; otherwise, it will be 0
 */
void
thresholdToBinaryLow(l_uint32  *datad,
                     l_int32    w,
                     l_int32    h,
                     l_int32    wpld,
                     l_uint32  *datas,
                     l_int32    d,
                     l_int32    wpls,
                     l_int32    thresh)
{
l_int32    i;
l_uint32  *lines, *lined;

    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        thresholdToBinaryLineLow(lined, w, lines, d, thresh);
    }
    return;
}


/*
 *  thresholdToBinaryLineLow()
 *
 */
void
thresholdToBinaryLineLow(l_uint32  *lined,
                         l_int32    w,
                         l_uint32  *lines,
                         l_int32    d,
                         l_int32    thresh)
{
l_int32  j, k, gval, scount, dcount;
l_uint32 sword, dword;

    PROCNAME("thresholdToBinaryLineLow");

    switch (d)
    {
    case 4:
            /* Unrolled as 4 source words, 1 dest word */
        for (j = 0, scount = 0, dcount = 0; j + 31 < w; j += 32) {
            dword = 0;
            for (k = 0; k < 4; k++) {
                sword = lines[scount++];
                dword <<= 8;
                gval = (sword >> 28) & 0xf;
                    /* Trick used here and below: if gval < thresh then
                     * gval - thresh < 0, so its high-order bit is 1, and
                     * ((gval - thresh) >> 31) & 1 == 1; likewise, if
                     * gval >= thresh, then ((gval - thresh) >> 31) & 1 == 0
                     * Doing it this way avoids a random (and thus easily
                     * mispredicted) branch on each pixel. */
                dword |= ((gval - thresh) >> 24) & 128;
                gval = (sword >> 24) & 0xf;
                dword |= ((gval - thresh) >> 25) & 64;
                gval = (sword >> 20) & 0xf;
                dword |= ((gval - thresh) >> 26) & 32;
                gval = (sword >> 16) & 0xf;
                dword |= ((gval - thresh) >> 27) & 16;
                gval = (sword >> 12) & 0xf;
                dword |= ((gval - thresh) >> 28) & 8;
                gval = (sword >> 8) & 0xf;
                dword |= ((gval - thresh) >> 29) & 4;
                gval = (sword >> 4) & 0xf;
                dword |= ((gval - thresh) >> 30) & 2;
                gval = sword & 0xf;
                dword |= ((gval - thresh) >> 31) & 1;
            }
            lined[dcount++] = dword;
        }

        if (j < w) {
          dword = 0;
          for (; j < w; j++) {
              if ((j & 7) == 0) {
                  sword = lines[scount++];
              }
              gval = (sword >> 28) & 0xf;
              sword <<= 4;
              dword |= (((gval - thresh) >> 31) & 1) << (31 - (j & 31));
          }
          lined[dcount] = dword;
        }
#if DEBUG_UNROLLING
#define CHECK_BIT(a, b, c) if (GET_DATA_BIT(a, b) != c) { \
    fprintf(stderr, "Error: mismatch at %d/%d(%d), %d vs %d\n", \
            j, w, d, GET_DATA_BIT(a, b), c); }
        for (j = 0; j < w; j++) {
            gval = GET_DATA_QBIT(lines, j);
            CHECK_BIT(lined, j, gval < thresh ? 1 : 0);
        }
#endif
        break;
    case 8:
            /* Unrolled as 8 source words, 1 dest word */
        for (j = 0, scount = 0, dcount = 0; j + 31 < w; j += 32) {
            dword = 0;
            for (k = 0; k < 8; k++) {
                sword = lines[scount++];
                dword <<= 4;
                gval = (sword >> 24) & 0xff;
                dword |= ((gval - thresh) >> 28) & 8;
                gval = (sword >> 16) & 0xff;
                dword |= ((gval - thresh) >> 29) & 4;
                gval = (sword >> 8) & 0xff;
                dword |= ((gval - thresh) >> 30) & 2;
                gval = sword & 0xff;
                dword |= ((gval - thresh) >> 31) & 1;
            }
            lined[dcount++] = dword;
        }

        if (j < w) {
            dword = 0;
            for (; j < w; j++) {
                if ((j & 3) == 0) {
                    sword = lines[scount++];
                }
                gval = (sword >> 24) & 0xff;
                sword <<= 8;
                dword |= (((gval - thresh) >> 31) & 1) << (31 - (j & 31));
            }
            lined[dcount] = dword;
        }
#if DEBUG_UNROLLING
        for (j = 0; j < w; j++) {
            gval = GET_DATA_BYTE(lines, j);
            CHECK_BIT(lined, j, gval < thresh ? 1 : 0);
        }
#undef CHECK_BIT
#endif
        break;
    default:
        L_ERROR("src depth not 4 or 8 bpp\n", procName);
        break;
    }
    return;
}


/*---------------------------------------------------------------------*
 *    Alternate implementation of dithering that uses lookup tables.   *
 *    This is analogous to the method used in dithering to 2 bpp.      *
 *---------------------------------------------------------------------*/
/*!
 *  ditherToBinaryLUTLow()
 *
 *  Low-level function for doing Floyd-Steinberg error diffusion
 *  dithering from 8 bpp (datas) to 1 bpp (datad).  Two source
 *  line buffers, bufs1 and bufs2, are provided, along with three
 *  256-entry lookup tables: tabval gives the output pixel value,
 *  tab38 gives the extra (plus or minus) transferred to the pixels
 *  directly to the left and below, and tab14 gives the extra
 *  transferred to the diagonal below.  The choice of 3/8 and 1/4
 *  is traditional but arbitrary when you use a lookup table; the
 *  only constraint is that the sum is 1.  See other comments below.
 */
void
ditherToBinaryLUTLow(l_uint32  *datad,
                     l_int32    w,
                     l_int32    h,
                     l_int32    wpld,
                     l_uint32  *datas,
                     l_int32    wpls,
                     l_uint32  *bufs1,
                     l_uint32  *bufs2,
                     l_int32   *tabval,
                     l_int32   *tab38,
                     l_int32   *tab14)
{
l_int32      i;
l_uint32    *lined;

        /* do all lines except last line */
    memcpy(bufs2, datas, 4 * wpls);  /* prime the buffer */
    for (i = 0; i < h - 1; i++) {
        memcpy(bufs1, bufs2, 4 * wpls);
        memcpy(bufs2, datas + (i + 1) * wpls, 4 * wpls);
        lined = datad + i * wpld;
        ditherToBinaryLineLUTLow(lined, w, bufs1, bufs2,
                                 tabval, tab38, tab14, 0);
    }

        /* do last line */
    memcpy(bufs1, bufs2, 4 * wpls);
    lined = datad + (h - 1) * wpld;
    ditherToBinaryLineLUTLow(lined, w, bufs1, bufs2, tabval, tab38, tab14,  1);
    return;
}


/*!
 *  ditherToBinaryLineLUTLow()
 *
 *      Input:  lined  (ptr to beginning of dest line
 *              w   (width of image in pixels)
 *              bufs1 (buffer of current source line)
 *              bufs2 (buffer of next source line)
 *              tabval (value to assign for current pixel)
 *              tab38 (excess value to give to neighboring 3/8 pixels)
 *              tab14 (excess value to give to neighboring 1/4 pixel)
 *              lastlineflag  (0 if not last dest line, 1 if last dest line)
 *      Return: void
 */
void
ditherToBinaryLineLUTLow(l_uint32  *lined,
                         l_int32    w,
                         l_uint32  *bufs1,
                         l_uint32  *bufs2,
                         l_int32   *tabval,
                         l_int32   *tab38,
                         l_int32   *tab14,
                         l_int32    lastlineflag)
{
l_int32  j;
l_int32  oval, tab38val, tab14val;
l_uint8  rval, bval, dval;

    if (lastlineflag == 0) {
        for (j = 0; j < w - 1; j++) {
            oval = GET_DATA_BYTE(bufs1, j);
            if (tabval[oval])
                SET_DATA_BIT(lined, j);
            rval = GET_DATA_BYTE(bufs1, j + 1);
            bval = GET_DATA_BYTE(bufs2, j);
            dval = GET_DATA_BYTE(bufs2, j + 1);
            tab38val = tab38[oval];
            if (tab38val == 0)
                continue;
            tab14val = tab14[oval];
            if (tab38val < 0) {
                rval = L_MAX(0, rval + tab38val);
                bval = L_MAX(0, bval + tab38val);
                dval = L_MAX(0, dval + tab14val);
            } else {
                rval = L_MIN(255, rval + tab38val);
                bval = L_MIN(255, bval + tab38val);
                dval = L_MIN(255, dval + tab14val);
            }
            SET_DATA_BYTE(bufs1, j + 1, rval);
            SET_DATA_BYTE(bufs2, j, bval);
            SET_DATA_BYTE(bufs2, j + 1, dval);
        }

            /* do last column: j = w - 1 */
        oval = GET_DATA_BYTE(bufs1, j);
        if (tabval[oval])
            SET_DATA_BIT(lined, j);
        bval = GET_DATA_BYTE(bufs2, j);
        tab38val = tab38[oval];
        if (tab38val < 0) {
            bval = L_MAX(0, bval + tab38val);
            SET_DATA_BYTE(bufs2, j, bval);
        } else if (tab38val > 0 ) {
            bval = L_MIN(255, bval + tab38val);
            SET_DATA_BYTE(bufs2, j, bval);
        }
    } else {   /* lastlineflag == 1 */
        for (j = 0; j < w - 1; j++) {
            oval = GET_DATA_BYTE(bufs1, j);
            if (tabval[oval])
                SET_DATA_BIT(lined, j);
            rval = GET_DATA_BYTE(bufs1, j + 1);
            tab38val = tab38[oval];
            if (tab38val == 0)
                continue;
            if (tab38val < 0)
                rval = L_MAX(0, rval + tab38val);
            else
                rval = L_MIN(255, rval + tab38val);
            SET_DATA_BYTE(bufs1, j + 1, rval);
        }

            /* do last pixel: (i, j) = (h - 1, w - 1) */
        oval = GET_DATA_BYTE(bufs1, j);
        if (tabval[oval])
            SET_DATA_BIT(lined, j);
    }

    return;
}


/*!
 *  make8To1DitherTables()
 *
 *      Input: &tabval (value assigned to output pixel; 0 or 1)
 *             &tab38  (amount propagated to pixels left and below)
 *             &tab14  (amount propagated to pixel to left and down)
 *             lowerclip (values near 0 where the excess is not propagated)
 *             upperclip (values near 255 where the deficit is not propagated)
 *
 *      Return: 0 if OK, 1 on error
 */
l_int32
make8To1DitherTables(l_int32 **ptabval,
                     l_int32 **ptab38,
                     l_int32 **ptab14,
                     l_int32   lowerclip,
                     l_int32   upperclip)
{
l_int32   i;
l_int32  *tabval, *tab38, *tab14;

    PROCNAME("make8To1DitherTables");

    if (!ptabval || !ptab38 || !ptab14)
        return ERROR_INT("table ptrs not all defined", procName, 1);

        /* 3 lookup tables: 1-bit value, (3/8)excess, and (1/4)excess */
    if ((tabval = (l_int32 *)LEPT_CALLOC(256, sizeof(l_int32))) == NULL)
        return ERROR_INT("tabval not made", procName, 1);
    if ((tab38 = (l_int32 *)LEPT_CALLOC(256, sizeof(l_int32))) == NULL)
        return ERROR_INT("tab38 not made", procName, 1);
    if ((tab14 = (l_int32 *)LEPT_CALLOC(256, sizeof(l_int32))) == NULL)
        return ERROR_INT("tab14 not made", procName, 1);
    *ptabval = tabval;
    *ptab38 = tab38;
    *ptab14 = tab14;

    for (i = 0; i < 256; i++) {
        if (i <= lowerclip) {
            tabval[i] = 1;
            tab38[i] = 0;
            tab14[i] = 0;
        } else if (i < 128) {
            tabval[i] = 1;
            tab38[i] = (3 * i + 4) / 8;
            tab14[i] = (i + 2) / 4;
        } else if (i < 255 - upperclip) {
            tabval[i] = 0;
            tab38[i] = (3 * (i - 255) + 4) / 8;
            tab14[i] = ((i - 255) + 2) / 4;
        } else {  /* i >= 255 - upperclip */
            tabval[i] = 0;
            tab38[i] = 0;
            tab14[i] = 0;
        }
    }

    return 0;
}


/*------------------------------------------------------------------*
 *                         Dithering to 2 bpp                       *
 *------------------------------------------------------------------*/
/*
 *  ditherTo2bppLow()
 *
 *  Low-level function for doing Floyd-Steinberg error diffusion
 *  dithering from 8 bpp (datas) to 2 bpp (datad).  Two source
 *  line buffers, bufs1 and bufs2, are provided, along with three
 *  256-entry lookup tables: tabval gives the output pixel value,
 *  tab38 gives the extra (plus or minus) transferred to the pixels
 *  directly to the left and below, and tab14 gives the extra
 *  transferred to the diagonal below.  The choice of 3/8 and 1/4
 *  is traditional but arbitrary when you use a lookup table; the
 *  only constraint is that the sum is 1.  See other comments
 *  below and in grayquant.c.
 */
void
ditherTo2bppLow(l_uint32  *datad,
                l_int32    w,
                l_int32    h,
                l_int32    wpld,
                l_uint32  *datas,
                l_int32    wpls,
                l_uint32  *bufs1,
                l_uint32  *bufs2,
                l_int32   *tabval,
                l_int32   *tab38,
                l_int32   *tab14)
{
l_int32      i;
l_uint32    *lined;

        /* do all lines except last line */
    memcpy(bufs2, datas, 4 * wpls);  /* prime the buffer */
    for (i = 0; i < h - 1; i++) {
        memcpy(bufs1, bufs2, 4 * wpls);
        memcpy(bufs2, datas + (i + 1) * wpls, 4 * wpls);
        lined = datad + i * wpld;
        ditherTo2bppLineLow(lined, w, bufs1, bufs2, tabval, tab38, tab14, 0);
    }

        /* do last line */
    memcpy(bufs1, bufs2, 4 * wpls);
    lined = datad + (h - 1) * wpld;
    ditherTo2bppLineLow(lined, w, bufs1, bufs2, tabval, tab38, tab14, 1);
    return;
}


/*
 *  ditherTo2bppLineLow()
 *
 *      Input:  lined  (ptr to beginning of dest line
 *              w   (width of image in pixels)
 *              bufs1 (buffer of current source line)
 *              bufs2 (buffer of next source line)
 *              tabval (value to assign for current pixel)
 *              tab38 (excess value to give to neighboring 3/8 pixels)
 *              tab14 (excess value to give to neighboring 1/4 pixel)
 *              lastlineflag  (0 if not last dest line, 1 if last dest line)
 *      Return: void
 *
 *  Dispatches error diffusion dithering for
 *  a single line of the image.  If lastlineflag == 0,
 *  both source buffers are used; otherwise, only bufs1
 *  is used.  We use source buffers because the error
 *  is propagated into them, and we don't want to change
 *  the input src image.
 *
 *  We break dithering out line by line to make it
 *  easier to combine functions like interpolative
 *  scaling and error diffusion dithering, as such a
 *  combination of operations obviates the need to
 *  generate a 2x grayscale image as an intermediary.
 */
void
ditherTo2bppLineLow(l_uint32  *lined,
                    l_int32    w,
                    l_uint32  *bufs1,
                    l_uint32  *bufs2,
                    l_int32   *tabval,
                    l_int32   *tab38,
                    l_int32   *tab14,
                    l_int32    lastlineflag)
{
l_int32  j;
l_int32  oval, tab38val, tab14val;
l_uint8  rval, bval, dval;

    if (lastlineflag == 0) {
        for (j = 0; j < w - 1; j++) {
            oval = GET_DATA_BYTE(bufs1, j);
            SET_DATA_DIBIT(lined, j, tabval[oval]);
            rval = GET_DATA_BYTE(bufs1, j + 1);
            bval = GET_DATA_BYTE(bufs2, j);
            dval = GET_DATA_BYTE(bufs2, j + 1);
            tab38val = tab38[oval];
            tab14val = tab14[oval];
            if (tab38val < 0) {
                rval = L_MAX(0, rval + tab38val);
                bval = L_MAX(0, bval + tab38val);
                dval = L_MAX(0, dval + tab14val);
            } else {
                rval = L_MIN(255, rval + tab38val);
                bval = L_MIN(255, bval + tab38val);
                dval = L_MIN(255, dval + tab14val);
            }
            SET_DATA_BYTE(bufs1, j + 1, rval);
            SET_DATA_BYTE(bufs2, j, bval);
            SET_DATA_BYTE(bufs2, j + 1, dval);
        }

            /* do last column: j = w - 1 */
        oval = GET_DATA_BYTE(bufs1, j);
        SET_DATA_DIBIT(lined, j, tabval[oval]);
        bval = GET_DATA_BYTE(bufs2, j);
        tab38val = tab38[oval];
        if (tab38val < 0)
            bval = L_MAX(0, bval + tab38val);
        else
            bval = L_MIN(255, bval + tab38val);
        SET_DATA_BYTE(bufs2, j, bval);
    } else {   /* lastlineflag == 1 */
        for (j = 0; j < w - 1; j++) {
            oval = GET_DATA_BYTE(bufs1, j);
            SET_DATA_DIBIT(lined, j, tabval[oval]);
            rval = GET_DATA_BYTE(bufs1, j + 1);
            tab38val = tab38[oval];
            if (tab38val < 0)
                rval = L_MAX(0, rval + tab38val);
            else
                rval = L_MIN(255, rval + tab38val);
            SET_DATA_BYTE(bufs1, j + 1, rval);
        }

            /* do last pixel: (i, j) = (h - 1, w - 1) */
        oval = GET_DATA_BYTE(bufs1, j);
        SET_DATA_DIBIT(lined, j, tabval[oval]);
    }

    return;
}


/*!
 *  make8To2DitherTables()
 *
 *      Input: &tabval (value assigned to output pixel; 0, 1, 2 or 3)
 *             &tab38  (amount propagated to pixels left and below)
 *             &tab14  (amount propagated to pixel to left and down)
 *             cliptoblack (values near 0 where the excess is not propagated)
 *             cliptowhite (values near 255 where the deficit is not propagated)
 *
 *      Return: 0 if OK, 1 on error
 */
l_int32
make8To2DitherTables(l_int32 **ptabval,
                     l_int32 **ptab38,
                     l_int32 **ptab14,
                     l_int32   cliptoblack,
                     l_int32   cliptowhite)
{
l_int32   i;
l_int32  *tabval, *tab38, *tab14;

    PROCNAME("make8To2DitherTables");

        /* 3 lookup tables: 2-bit value, (3/8)excess, and (1/4)excess */
    if ((tabval = (l_int32 *)LEPT_CALLOC(256, sizeof(l_int32))) == NULL)
        return ERROR_INT("tabval not made", procName, 1);
    if ((tab38 = (l_int32 *)LEPT_CALLOC(256, sizeof(l_int32))) == NULL)
        return ERROR_INT("tab38 not made", procName, 1);
    if ((tab14 = (l_int32 *)LEPT_CALLOC(256, sizeof(l_int32))) == NULL)
        return ERROR_INT("tab14 not made", procName, 1);
    *ptabval = tabval;
    *ptab38 = tab38;
    *ptab14 = tab14;

    for (i = 0; i < 256; i++) {
        if (i <= cliptoblack) {
            tabval[i] = 0;
            tab38[i] = 0;
            tab14[i] = 0;
        } else if (i < 43) {
            tabval[i] = 0;
            tab38[i] = (3 * i + 4) / 8;
            tab14[i] = (i + 2) / 4;
        } else if (i < 85) {
            tabval[i] = 1;
            tab38[i] = (3 * (i - 85) - 4) / 8;
            tab14[i] = ((i - 85) - 2) / 4;
        } else if (i < 128) {
            tabval[i] = 1;
            tab38[i] = (3 * (i - 85) + 4) / 8;
            tab14[i] = ((i - 85) + 2) / 4;
        } else if (i < 170) {
            tabval[i] = 2;
            tab38[i] = (3 * (i - 170) - 4) / 8;
            tab14[i] = ((i - 170) - 2) / 4;
        } else if (i < 213) {
            tabval[i] = 2;
            tab38[i] = (3 * (i - 170) + 4) / 8;
            tab14[i] = ((i - 170) + 2) / 4;
        } else if (i < 255 - cliptowhite) {
            tabval[i] = 3;
            tab38[i] = (3 * (i - 255) - 4) / 8;
            tab14[i] = ((i - 255) - 2) / 4;
        } else {  /* i >= 255 - cliptowhite */
            tabval[i] = 3;
            tab38[i] = 0;
            tab14[i] = 0;
        }
    }

    return 0;
}


/*------------------------------------------------------------------*
 *                   Simple thresholding to 2 bpp                   *
 *------------------------------------------------------------------*/
/*
 *  thresholdTo2bppLow()
 *
 *  Low-level function for thresholding from 8 bpp (datas) to
 *  2 bpp (datad), using thresholds implicitly defined through @tab,
 *  a 256-entry lookup table that gives a 2-bit output value
 *  for each possible input.
 *
 *  For each line, unroll the loop so that for each 32 bit src word,
 *  representing four consecutive 8-bit pixels, we compose one byte
 *  of output consisiting of four 2-bit pixels.
 */
void
thresholdTo2bppLow(l_uint32  *datad,
                   l_int32    h,
                   l_int32    wpld,
                   l_uint32  *datas,
                   l_int32    wpls,
                   l_int32   *tab)
{
l_uint8    sval1, sval2, sval3, sval4, dval;
l_int32    i, j, k;
l_uint32  *lines, *lined;

    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < wpls; j++) {
            k = 4 * j;
            sval1 = GET_DATA_BYTE(lines, k);
            sval2 = GET_DATA_BYTE(lines, k + 1);
            sval3 = GET_DATA_BYTE(lines, k + 2);
            sval4 = GET_DATA_BYTE(lines, k + 3);
            dval = (tab[sval1] << 6) | (tab[sval2] << 4) |
                   (tab[sval3] << 2) | tab[sval4];
            SET_DATA_BYTE(lined, j, dval);
        }
    }
    return;
}


/*------------------------------------------------------------------*
 *                   Simple thresholding to 4 bpp                   *
 *------------------------------------------------------------------*/
/*
 *  thresholdTo4bppLow()
 *
 *  Low-level function for thresholding from 8 bpp (datas) to
 *  4 bpp (datad), using thresholds implicitly defined through @tab,
 *  a 256-entry lookup table that gives a 4-bit output value
 *  for each possible input.
 *
 *  For each line, unroll the loop so that for each 32 bit src word,
 *  representing four consecutive 8-bit pixels, we compose two bytes
 *  of output consisiting of four 4-bit pixels.
 */
void
thresholdTo4bppLow(l_uint32  *datad,
                   l_int32    h,
                   l_int32    wpld,
                   l_uint32  *datas,
                   l_int32    wpls,
                   l_int32   *tab)
{
l_uint8    sval1, sval2, sval3, sval4;
l_uint16   dval;
l_int32    i, j, k;
l_uint32  *lines, *lined;

    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < wpls; j++) {
            k = 4 * j;
            sval1 = GET_DATA_BYTE(lines, k);
            sval2 = GET_DATA_BYTE(lines, k + 1);
            sval3 = GET_DATA_BYTE(lines, k + 2);
            sval4 = GET_DATA_BYTE(lines, k + 3);
            dval = (tab[sval1] << 12) | (tab[sval2] << 8) |
                   (tab[sval3] << 4) | tab[sval4];
            SET_DATA_TWO_BYTES(lined, j, dval);
        }
    }
    return;
}
