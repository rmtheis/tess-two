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
 *  seedfilllow.c
 *
 *      Seedfill:
 *      Gray seedfill (source: Luc Vincent:fast-hybrid-grayscale-reconstruction)
 *               void   seedfillBinaryLow()
 *               void   seedfillGrayLow()
 *               void   seedfillGrayInvLow()
 *               void   seedfillGrayLowSimple()
 *               void   seedfillGrayInvLowSimple()
 *
 *      Distance function:
 *               void   distanceFunctionLow()
 *
 *      Seed spread:
 *               void   seedspreadLow()
 *
 */

#include <math.h>
#include "allheaders.h"

struct L_Pixel
{
    l_int32    x;
    l_int32    y;
};
typedef struct L_Pixel  L_PIXEL;


/*-----------------------------------------------------------------------*
 *                 Vincent's Iterative Binary Seedfill                   *
 *-----------------------------------------------------------------------*/
/*!
 *  seedfillBinaryLow()
 *
 *  Notes:
 *      (1) This is an in-place fill, where the seed image is
 *          filled, clipping to the filling mask, in one full
 *          cycle of UL -> LR and LR -> UL raster scans.
 *      (2) Assume the mask is a filling mask, not a blocking mask.
 *      (3) Assume that the RHS pad bits of the mask
 *          are properly set to 0.
 *      (4) Clip to the smallest dimensions to avoid invalid reads.
 */
void
seedfillBinaryLow(l_uint32  *datas,
                  l_int32    hs,
                  l_int32    wpls,
                  l_uint32  *datam,
                  l_int32    hm,
                  l_int32    wplm,
                  l_int32    connectivity)
{
l_int32    i, j, h, wpl;
l_uint32   word, mask;
l_uint32   wordabove, wordleft, wordbelow, wordright;
l_uint32   wordprev;  /* test against this in previous iteration */
l_uint32  *lines, *linem;

    PROCNAME("seedfillBinaryLow");

    h = L_MIN(hs, hm);
    wpl = L_MIN(wpls, wplm);

    switch (connectivity)
    {
    case 4:
            /* UL --> LR scan */
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = 0; j < wpl; j++) {
                word = *(lines + j);
                mask = *(linem + j);

                    /* OR from word above and from word to left; mask */
                if (i > 0) {
                    wordabove = *(lines - wpls + j);
                    word |= wordabove;
                }
                if (j > 0) {
                    wordleft = *(lines + j - 1);
                    word |= wordleft << 31;
                }
                word &= mask;

                    /* No need to fill horizontally? */
                if (!word || !(~word)) {
                    *(lines + j) = word;
                    continue;
                }

                while (1) {
                    wordprev = word;
                    word = (word | (word >> 1) | (word << 1)) & mask;
                    if ((word ^ wordprev) == 0) {
                        *(lines + j) = word;
                        break;
                    }
                }
            }
        }

            /* LR --> UL scan */
        for (i = h - 1; i >= 0; i--) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = wpl - 1; j >= 0; j--) {
                word = *(lines + j);
                mask = *(linem + j);

                    /* OR from word below and from word to right; mask */
                if (i < h - 1) {
                    wordbelow = *(lines + wpls + j);
                    word |= wordbelow;
                }
                if (j < wpl - 1) {
                    wordright = *(lines + j + 1);
                    word |= wordright >> 31;
                }
                word &= mask;

                    /* No need to fill horizontally? */
                if (!word || !(~word)) {
                    *(lines + j) = word;
                    continue;
                }

                while (1) {
                    wordprev = word;
                    word = (word | (word >> 1) | (word << 1)) & mask;
                    if ((word ^ wordprev) == 0) {
                        *(lines + j) = word;
                        break;
                    }
                }
            }
        }
        break;

    case 8:
            /* UL --> LR scan */
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = 0; j < wpl; j++) {
                word = *(lines + j);
                mask = *(linem + j);

                    /* OR from words above and from word to left; mask */
                if (i > 0) {
                    wordabove = *(lines - wpls + j);
                    word |= (wordabove | (wordabove << 1) | (wordabove >> 1));
                    if (j > 0)
                        word |= (*(lines - wpls + j - 1)) << 31;
                    if (j < wpl - 1)
                        word |= (*(lines - wpls + j + 1)) >> 31;
                }
                if (j > 0) {
                    wordleft = *(lines + j - 1);
                    word |= wordleft << 31;
                }
                word &= mask;

                    /* No need to fill horizontally? */
                if (!word || !(~word)) {
                    *(lines + j) = word;
                    continue;
                }

                while (1) {
                    wordprev = word;
                    word = (word | (word >> 1) | (word << 1)) & mask;
                    if ((word ^ wordprev) == 0) {
                        *(lines + j) = word;
                        break;
                    }
                }
            }
        }

            /* LR --> UL scan */
        for (i = h - 1; i >= 0; i--) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = wpl - 1; j >= 0; j--) {
                word = *(lines + j);
                mask = *(linem + j);

                    /* OR from words below and from word to right; mask */
                if (i < h - 1) {
                    wordbelow = *(lines + wpls + j);
                    word |= (wordbelow | (wordbelow << 1) | (wordbelow >> 1));
                    if (j > 0)
                        word |= (*(lines + wpls + j - 1)) << 31;
                    if (j < wpl - 1)
                        word |= (*(lines + wpls + j + 1)) >> 31;
                }
                if (j < wpl - 1) {
                    wordright = *(lines + j + 1);
                    word |= wordright >> 31;
                }
                word &= mask;

                    /* No need to fill horizontally? */
                if (!word || !(~word)) {
                    *(lines + j) = word;
                    continue;
                }

                while (1) {
                    wordprev = word;
                    word = (word | (word >> 1) | (word << 1)) & mask;
                    if ((word ^ wordprev) == 0) {
                        *(lines + j) = word;
                        break;
                    }
                }
            }
        }
        break;

    default:
        L_ERROR("connectivity must be 4 or 8\n", procName);
        return;
    }

    return;
}



/*-----------------------------------------------------------------------*
 *                 Vincent's Hybrid Grayscale Seedfill                *
 *-----------------------------------------------------------------------*/
/*!
 *  seedfillGrayLow()
 *
 *  Notes:
 *      (1) The pixels are numbered as follows:
 *              1  2  3
 *              4  x  5
 *              6  7  8
 *          This low-level filling operation consists of two scans,
 *          raster and anti-raster, covering the entire seed image.
 *          This is followed by a breadth-first propagation operation to
 *          complete the fill.
 *          During the anti-raster scan, every pixel p whose current value
 *          could still be propagated after the anti-raster scan is put into
 *          the FIFO queue.
 *          The propagation step is a breadth-first fill to completion.
 *          Unlike the simple grayscale seedfill pixSeedfillGraySimple(),
 *          where at least two full raster/anti-raster iterations are required
 *          for completion and verification, the hybrid method uses only a
 *          single raster/anti-raster set of scans.
 *      (2) The filling action can be visualized from the following example.
 *          Suppose the mask, which clips the fill, is a sombrero-shaped
 *          surface, where the highest point is 200 and the low pixels
 *          around the rim are 30.  Beyond the rim, the mask goes up a bit.
 *          Suppose the seed, which is filled, consists of a single point
 *          of height 150, located below the max of the mask, with
 *          the rest 0.  Then in the raster scan, nothing happens until
 *          the high seed point is encountered, and then this value is
 *          propagated right and down, until it hits the side of the
 *          sombrero.   The seed can never exceed the mask, so it fills
 *          to the rim, going lower along the mask surface.  When it
 *          passes the rim, the seed continues to fill at the rim
 *          height to the edge of the seed image.  Then on the
 *          anti-raster scan, the seed fills flat inside the
 *          sombrero to the upper and left, and then out from the
 *          rim as before.  The final result has a seed that is
 *          flat outside the rim, and inside it fills the sombrero
 *          but only up to 150.  If the rim height varies, the
 *          filled seed outside the rim will be at the highest
 *          point on the rim, which is a saddle point on the rim.
 *      (3) Reference paper :
 *            L. Vincent, Morphological grayscale reconstruction in image
 *            analysis: applications and efficient algorithms, IEEE Transactions
 *            on  Image Processing, vol. 2, no. 2, pp. 176-201, 1993.
 */
void
seedfillGrayLow(l_uint32  *datas,
                l_int32    w,
                l_int32    h,
                l_int32    wpls,
                l_uint32  *datam,
                l_int32    wplm,
                l_int32    connectivity)
{
l_uint8    val1, val2, val3, val4, val5, val6, val7, val8;
l_uint8    val, maxval, maskval, boolval;
l_int32    i, j, imax, jmax, queue_size;
l_uint32  *lines, *linem;
L_PIXEL *pixel;
L_QUEUE  *lq_pixel;

    PROCNAME("seedfillGrayLow");

    if (connectivity != 4 && connectivity != 8) {
        L_ERROR("connectivity must be 4 or 8\n", procName);
        return;
    }

    imax = h - 1;
    jmax = w - 1;

        /* In the worst case, most of the pixels could be pushed
         * onto the FIFO queue during anti-raster scan.  However this
         * will rarely happen, and we initialize the queue ptr size to
         * the image perimeter. */
    lq_pixel = lqueueCreate(2 * (w + h));

    switch (connectivity)
    {
    case 4:
            /* UL --> LR scan  (Raster Order)
             * If I : mask image
             *    J : marker image
             * Let p be the currect pixel;
             * J(p) <- (max{J(p) union J(p) neighbors in raster order})
             *          intersection I(p) */
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = 0; j < w; j++) {
                if ((maskval = GET_DATA_BYTE(linem, j)) > 0) {
                    maxval = 0;
                    if (i > 0)
                        maxval = GET_DATA_BYTE(lines - wpls, j);
                    if (j > 0) {
                        val4 = GET_DATA_BYTE(lines, j - 1);
                        maxval = L_MAX(maxval, val4);
                    }
                    val = GET_DATA_BYTE(lines, j);
                    maxval = L_MAX(maxval, val);
                    val = L_MIN(maxval, maskval);
                    SET_DATA_BYTE(lines, j, val);
                }
            }
        }

            /* LR --> UL scan (anti-raster order)
             * Let p be the currect pixel;
             * J(p) <- (max{J(p) union J(p) neighbors in anti-raster order})
             *          intersection I(p) */
        for (i = imax; i >= 0; i--) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = jmax; j >= 0; j--) {
                boolval = FALSE;
                if ((maskval = GET_DATA_BYTE(linem, j)) > 0) {
                    maxval = 0;
                    if (i < imax)
                        maxval = GET_DATA_BYTE(lines + wpls, j);
                    if (j < jmax) {
                        val5 = GET_DATA_BYTE(lines, j + 1);
                        maxval = L_MAX(maxval, val5);
                    }
                    val = GET_DATA_BYTE(lines, j);
                    maxval = L_MAX(maxval, val);
                    val = L_MIN(maxval, maskval);
                    SET_DATA_BYTE(lines, j, val);

                        /*
                         * If there exists a point (q) which belongs to J(p)
                         * neighbors in anti-raster order such that J(q) < J(p)
                         * and J(q) < I(q) then
                         * fifo_add(p) */
                    if (i < imax) {
                        val7 = GET_DATA_BYTE(lines + wpls, j);
                        if ((val7 < val) &&
                            (val7 < GET_DATA_BYTE(linem + wplm, j))) {
                            boolval = TRUE;
                        }
                    }
                    if (j < jmax) {
                        val5 = GET_DATA_BYTE(lines, j + 1);
                        if (!boolval && (val5 < val) &&
                            (val5 < GET_DATA_BYTE(linem, j + 1))) {
                            boolval = TRUE;
                        }
                    }
                    if (boolval) {
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i;
                        pixel->y = j;
                        lqueueAdd(lq_pixel, pixel);
                    }
                }
            }
        }

            /* Propagation step:
             *        while fifo_empty = false
             *          p <- fifo_first()
             *          for every pixel (q) belong to neighbors of (p)
             *            if J(q) < J(p) and I(q) != J(q)
             *              J(q) <- min(J(p), I(q));
             *              fifo_add(q);
             *            end
             *          end
             *        end */
        queue_size = lqueueGetCount(lq_pixel);
        while (queue_size) {
            pixel = (L_PIXEL *)lqueueRemove(lq_pixel);
            i = pixel->x;
            j = pixel->y;
            LEPT_FREE(pixel);
            lines = datas + i * wpls;
            linem = datam + i * wplm;

            if ((val = GET_DATA_BYTE(lines, j)) > 0) {
                if (i > 0) {
                    val2 = GET_DATA_BYTE(lines - wpls, j);
                    maskval = GET_DATA_BYTE(linem - wplm, j);
                    if (val > val2 && val2 != maskval) {
                        SET_DATA_BYTE(lines - wpls, j, L_MIN(val, maskval));
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i - 1;
                        pixel->y = j;
                        lqueueAdd(lq_pixel, pixel);
                    }

                }
                if (j > 0) {
                    val4 = GET_DATA_BYTE(lines, j - 1);
                    maskval = GET_DATA_BYTE(linem, j - 1);
                    if (val > val4 && val4 != maskval) {
                        SET_DATA_BYTE(lines, j - 1, L_MIN(val, maskval));
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i;
                        pixel->y = j - 1;
                        lqueueAdd(lq_pixel, pixel);
                    }
                }
                if (i < imax) {
                    val7 = GET_DATA_BYTE(lines + wpls, j);
                    maskval = GET_DATA_BYTE(linem + wplm, j);
                    if (val > val7 && val7 != maskval) {
                        SET_DATA_BYTE(lines + wpls, j, L_MIN(val, maskval));
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i + 1;
                        pixel->y = j;
                        lqueueAdd(lq_pixel, pixel);
                    }
                }
                if (j < jmax) {
                    val5 = GET_DATA_BYTE(lines, j + 1);
                    maskval = GET_DATA_BYTE(linem, j + 1);
                    if (val > val5 && val5 != maskval) {
                        SET_DATA_BYTE(lines, j + 1, L_MIN(val, maskval));
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i;
                        pixel->y = j + 1;
                        lqueueAdd(lq_pixel, pixel);
                    }
                }
            }

            queue_size = lqueueGetCount(lq_pixel);
        }

        break;

    case 8:
            /* UL --> LR scan  (Raster Order)
             * If I : mask image
             *    J : marker image
             * Let p be the currect pixel;
             * J(p) <- (max{J(p) union J(p) neighbors in raster order})
             *          intersection I(p) */
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = 0; j < w; j++) {
                if ((maskval = GET_DATA_BYTE(linem, j)) > 0) {
                    maxval = 0;
                    if (i > 0) {
                        if (j > 0)
                            maxval = GET_DATA_BYTE(lines - wpls, j - 1);
                        if (j < jmax) {
                            val3 = GET_DATA_BYTE(lines - wpls, j + 1);
                            maxval = L_MAX(maxval, val3);
                        }
                        val2 = GET_DATA_BYTE(lines - wpls, j);
                        maxval = L_MAX(maxval, val2);
                    }
                    if (j > 0) {
                        val4 = GET_DATA_BYTE(lines, j - 1);
                        maxval = L_MAX(maxval, val4);
                    }
                    val = GET_DATA_BYTE(lines, j);
                    maxval = L_MAX(maxval, val);
                    val = L_MIN(maxval, maskval);
                    SET_DATA_BYTE(lines, j, val);
                }
            }
        }

            /* LR --> UL scan (anti-raster order)
             * Let p be the currect pixel;
             * J(p) <- (max{J(p) union J(p) neighbors in anti-raster order})
             *          intersection I(p) */
        for (i = imax; i >= 0; i--) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = jmax; j >= 0; j--) {
                boolval = FALSE;
                if ((maskval = GET_DATA_BYTE(linem, j)) > 0) {
                    maxval = 0;
                    if (i < imax) {
                        if (j > 0) {
                            maxval = GET_DATA_BYTE(lines + wpls, j - 1);
                        }
                        if (j < jmax) {
                            val8 = GET_DATA_BYTE(lines + wpls, j + 1);
                            maxval = L_MAX(maxval, val8);
                        }
                        val7 = GET_DATA_BYTE(lines + wpls, j);
                        maxval = L_MAX(maxval, val7);
                    }
                    if (j < jmax) {
                        val5 = GET_DATA_BYTE(lines, j + 1);
                        maxval = L_MAX(maxval, val5);
                    }
                    val = GET_DATA_BYTE(lines, j);
                    maxval = L_MAX(maxval, val);
                    val = L_MIN(maxval, maskval);
                    SET_DATA_BYTE(lines, j, val);

                        /* If there exists a point (q) which belongs to J(p)
                         * neighbors in anti-raster order such that J(q) < J(p)
                         * and J(q) < I(q) then
                         * fifo_add(p) */
                    if (i < imax) {
                        if (j > 0) {
                            val6 = GET_DATA_BYTE(lines + wpls, j - 1);
                            if ((val6 < val) &&
                                (val6 < GET_DATA_BYTE(linem + wplm, j - 1))) {
                                boolval = TRUE;
                            }
                        }
                        if (j < jmax) {
                            val8 = GET_DATA_BYTE(lines + wpls, j + 1);
                            if (!boolval && (val8 < val) &&
                                (val8 < GET_DATA_BYTE(linem + wplm, j + 1))) {
                                boolval = TRUE;
                            }
                        }
                        val7 = GET_DATA_BYTE(lines + wpls, j);
                        if (!boolval && (val7 < val) &&
                            (val7 < GET_DATA_BYTE(linem + wplm, j))) {
                            boolval = TRUE;
                        }
                    }
                    if (j < jmax) {
                        val5 = GET_DATA_BYTE(lines, j + 1);
                        if (!boolval && (val5 < val) &&
                            (val5 < GET_DATA_BYTE(linem, j + 1))) {
                            boolval = TRUE;
                        }
                    }
                    if (boolval) {
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i;
                        pixel->y = j;
                        lqueueAdd(lq_pixel, pixel);
                    }
                }
            }
        }

            /* Propagation step:
             *        while fifo_empty = false
             *          p <- fifo_first()
             *          for every pixel (q) belong to neighbors of (p)
             *            if J(q) < J(p) and I(q) != J(q)
             *              J(q) <- min(J(p), I(q));
             *              fifo_add(q);
             *            end
             *          end
             *        end */
        queue_size = lqueueGetCount(lq_pixel);
        while (queue_size) {
            pixel = (L_PIXEL *)lqueueRemove(lq_pixel);
            i = pixel->x;
            j = pixel->y;
            LEPT_FREE(pixel);
            lines = datas + i * wpls;
            linem = datam + i * wplm;

            if ((val = GET_DATA_BYTE(lines, j)) > 0) {
                if (i > 0) {
                    if (j > 0) {
                        val1 = GET_DATA_BYTE(lines - wpls, j - 1);
                        maskval = GET_DATA_BYTE(linem - wplm, j - 1);
                        if (val > val1 && val1 != maskval) {
                            SET_DATA_BYTE(lines - wpls, j - 1,
                                          L_MIN(val, maskval));
                            pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                            pixel->x = i - 1;
                            pixel->y = j - 1;
                            lqueueAdd(lq_pixel, pixel);
                        }
                    }
                    if (j < jmax) {
                        val3 = GET_DATA_BYTE(lines - wpls, j + 1);
                        maskval = GET_DATA_BYTE(linem - wplm, j + 1);
                        if (val > val3 && val3 != maskval) {
                            SET_DATA_BYTE(lines - wpls, j + 1,
                                          L_MIN(val, maskval));
                            pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                            pixel->x = i - 1;
                            pixel->y = j + 1;
                            lqueueAdd(lq_pixel, pixel);
                        }
                    }
                    val2 = GET_DATA_BYTE(lines - wpls, j);
                    maskval = GET_DATA_BYTE(linem - wplm, j);
                    if (val > val2 && val2 != maskval) {
                        SET_DATA_BYTE(lines - wpls, j, L_MIN(val, maskval));
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i - 1;
                        pixel->y = j;
                        lqueueAdd(lq_pixel, pixel);
                    }

                }
                if (j > 0) {
                    val4 = GET_DATA_BYTE(lines, j - 1);
                    maskval = GET_DATA_BYTE(linem, j - 1);
                    if (val > val4 && val4 != maskval) {
                        SET_DATA_BYTE(lines, j - 1, L_MIN(val, maskval));
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i;
                        pixel->y = j - 1;
                        lqueueAdd(lq_pixel, pixel);
                    }
                }
                if (i < imax) {
                    if (j > 0) {
                        val6 = GET_DATA_BYTE(lines + wpls, j - 1);
                        maskval = GET_DATA_BYTE(linem + wplm, j - 1);
                        if (val > val6 && val6 != maskval) {
                            SET_DATA_BYTE(lines + wpls, j - 1,
                                          L_MIN(val, maskval));
                            pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                            pixel->x = i + 1;
                            pixel->y = j - 1;
                            lqueueAdd(lq_pixel, pixel);
                        }
                    }
                    if (j < jmax) {
                        val8 = GET_DATA_BYTE(lines + wpls, j + 1);
                        maskval = GET_DATA_BYTE(linem + wplm, j + 1);
                        if (val > val8 && val8 != maskval) {
                            SET_DATA_BYTE(lines + wpls, j + 1,
                                          L_MIN(val, maskval));
                            pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                            pixel->x = i + 1;
                            pixel->y = j + 1;
                            lqueueAdd(lq_pixel, pixel);
                        }
                    }
                    val7 = GET_DATA_BYTE(lines + wpls, j);
                    maskval = GET_DATA_BYTE(linem + wplm, j);
                    if (val > val7 && val7 != maskval) {
                        SET_DATA_BYTE(lines + wpls, j, L_MIN(val, maskval));
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i + 1;
                        pixel->y = j;
                        lqueueAdd(lq_pixel, pixel);
                    }
                }
                if (j < jmax) {
                    val5 = GET_DATA_BYTE(lines, j + 1);
                    maskval = GET_DATA_BYTE(linem, j + 1);
                    if (val > val5 && val5 != maskval) {
                        SET_DATA_BYTE(lines, j + 1, L_MIN(val, maskval));
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i;
                        pixel->y = j + 1;
                        lqueueAdd(lq_pixel, pixel);
                    }
                }
            }

            queue_size = lqueueGetCount(lq_pixel);
        }
        break;

    default:
        L_ERROR("shouldn't get here!\n", procName);
        break;
    }

    lqueueDestroy(&lq_pixel, TRUE);
    return;
}


/*!
 *  seedfillGrayInvLow()
 *
 *  Notes:
 *      (1) The pixels are numbered as follows:
 *              1  2  3
 *              4  x  5
 *              6  7  8
 *          This low-level filling operation consists of two scans,
 *          raster and anti-raster, covering the entire seed image.
 *          During the anti-raster scan, every pixel p such that its
 *          current value could still be propogated during the next
 *          raster scanning is put into the FIFO-queue.
 *          Next step is the propagation step where where we update
 *          and propagate the values using FIFO structure created in
 *          anti-raster scan.
 *      (2) The "Inv" signifies the fact that in this case, filling
 *          of the seed only takes place when the seed value is
 *          greater than the mask value.  The mask will act to stop
 *          the fill when it is higher than the seed level.  (This is
 *          in contrast to conventional grayscale filling where the
 *          seed always fills below the mask.)
 *      (3) An example of use is a basin, described by the mask (pixm),
 *          where within the basin, the seed pix (pixs) gets filled to the
 *          height of the highest seed pixel that is above its
 *          corresponding max pixel.  Filling occurs while the
 *          propagating seed pixels in pixs are larger than the
 *          corresponding mask values in pixm.
 *      (4) Reference paper :
 *            L. Vincent, Morphological grayscale reconstruction in image
 *            analysis: applications and efficient algorithms, IEEE Transactions
 *            on  Image Processing, vol. 2, no. 2, pp. 176-201, 1993.
 */
void
seedfillGrayInvLow(l_uint32  *datas,
                   l_int32    w,
                   l_int32    h,
                   l_int32    wpls,
                   l_uint32  *datam,
                   l_int32    wplm,
                   l_int32    connectivity)
{
l_uint8    val1, val2, val3, val4, val5, val6, val7, val8;
l_uint8    val, maxval, maskval, boolval;
l_int32    i, j, imax, jmax, queue_size;
l_uint32  *lines, *linem;
L_PIXEL *pixel;
L_QUEUE  *lq_pixel;

    PROCNAME("seedfillGrayInvLow");

    if (connectivity != 4 && connectivity != 8) {
        L_ERROR("connectivity must be 4 or 8\n", procName);
        return;
    }

    imax = h - 1;
    jmax = w - 1;

        /* In the worst case, most of the pixels could be pushed
         * onto the FIFO queue during anti-raster scan.  However this
         * will rarely happen, and we initialize the queue ptr size to
         * the image perimeter. */
    lq_pixel = lqueueCreate(2 * (w + h));

    switch (connectivity)
    {
    case 4:
            /* UL --> LR scan  (Raster Order)
             * If I : mask image
             *    J : marker image
             * Let p be the currect pixel;
             * tmp <- max{J(p) union J(p) neighbors in raster order}
             * if (tmp > I(p))
             *   J(p) <- tmp
             * end */
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = 0; j < w; j++) {
                if ((maskval = GET_DATA_BYTE(linem, j)) < 255) {
                    maxval = GET_DATA_BYTE(lines, j);
                    if (i > 0) {
                        val2 = GET_DATA_BYTE(lines - wpls, j);
                        maxval = L_MAX(maxval, val2);
                    }
                    if (j > 0) {
                        val4 = GET_DATA_BYTE(lines, j - 1);
                        maxval = L_MAX(maxval, val4);
                    }
                    if (maxval > maskval)
                        SET_DATA_BYTE(lines, j, maxval);
                }
            }
        }

            /* LR --> UL scan (anti-raster order)
             * If I : mask image
             *    J : marker image
             * Let p be the currect pixel;
             * tmp <- max{J(p) union J(p) neighbors in anti-raster order}
             * if (tmp > I(p))
             *   J(p) <- tmp
             * end */
        for (i = imax; i >= 0; i--) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = jmax; j >= 0; j--) {
                boolval = FALSE;
                if ((maskval = GET_DATA_BYTE(linem, j)) < 255) {
                    val = maxval = GET_DATA_BYTE(lines, j);
                    if (i < imax) {
                        val7 = GET_DATA_BYTE(lines + wpls, j);
                        maxval = L_MAX(maxval, val7);
                    }
                    if (j < jmax) {
                        val5 = GET_DATA_BYTE(lines, j + 1);
                        maxval = L_MAX(maxval, val5);
                    }
                    if (maxval > maskval)
                        SET_DATA_BYTE(lines, j, maxval);
                    val = GET_DATA_BYTE(lines, j);

                        /*
                         * If there exists a point (q) which belongs to J(p)
                         * neighbors in anti-raster order such that J(q) < J(p)
                         * and J(p) > I(q) then
                         * fifo_add(p) */
                    if (i < imax) {
                        val7 = GET_DATA_BYTE(lines + wpls, j);
                        if ((val7 < val) &&
                            (val > GET_DATA_BYTE(linem + wplm, j))) {
                            boolval = TRUE;
                        }
                    }
                    if (j < jmax) {
                        val5 = GET_DATA_BYTE(lines, j + 1);
                        if (!boolval && (val5 < val) &&
                            (val > GET_DATA_BYTE(linem, j + 1))) {
                            boolval = TRUE;
                        }
                    }
                    if (boolval) {
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i;
                        pixel->y = j;
                        lqueueAdd(lq_pixel, pixel);
                    }
                }
            }
        }

            /* Propagation step:
             *        while fifo_empty = false
             *          p <- fifo_first()
             *          for every pixel (q) belong to neighbors of (p)
             *            if J(q) < J(p) and J(p) > I(q)
             *              J(q) <- min(J(p), I(q));
             *              fifo_add(q);
             *            end
             *          end
             *        end */
        queue_size = lqueueGetCount(lq_pixel);
        while (queue_size) {
            pixel = (L_PIXEL *)lqueueRemove(lq_pixel);
            i = pixel->x;
            j = pixel->y;
            LEPT_FREE(pixel);
            lines = datas + i * wpls;
            linem = datam + i * wplm;

            if ((val = GET_DATA_BYTE(lines, j)) > 0) {
                if (i > 0) {
                    val2 = GET_DATA_BYTE(lines - wpls, j);
                    maskval = GET_DATA_BYTE(linem - wplm, j);
                    if (val > val2 && val > maskval) {
                        SET_DATA_BYTE(lines - wpls, j, val);
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i - 1;
                        pixel->y = j;
                        lqueueAdd(lq_pixel, pixel);
                    }

                }
                if (j > 0) {
                    val4 = GET_DATA_BYTE(lines, j - 1);
                    maskval = GET_DATA_BYTE(linem, j - 1);
                    if (val > val4 && val > maskval) {
                        SET_DATA_BYTE(lines, j - 1, val);
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i;
                        pixel->y = j - 1;
                        lqueueAdd(lq_pixel, pixel);
                    }
                }
                if (i < imax) {
                    val7 = GET_DATA_BYTE(lines + wpls, j);
                    maskval = GET_DATA_BYTE(linem + wplm, j);
                    if (val > val7 && val > maskval) {
                        SET_DATA_BYTE(lines + wpls, j, val);
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i + 1;
                        pixel->y = j;
                        lqueueAdd(lq_pixel, pixel);
                    }
                }
                if (j < jmax) {
                    val5 = GET_DATA_BYTE(lines, j + 1);
                    maskval = GET_DATA_BYTE(linem, j + 1);
                    if (val > val5 && val > maskval) {
                        SET_DATA_BYTE(lines, j + 1, val);
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i;
                        pixel->y = j + 1;
                        lqueueAdd(lq_pixel, pixel);
                    }
                }
            }

            queue_size = lqueueGetCount(lq_pixel);
        }

        break;

    case 8:
            /* UL --> LR scan  (Raster Order)
             * If I : mask image
             *    J : marker image
             * Let p be the currect pixel;
             * tmp <- max{J(p) union J(p) neighbors in raster order}
             * if (tmp > I(p))
             *   J(p) <- tmp
             * end */
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = 0; j < w; j++) {
                if ((maskval = GET_DATA_BYTE(linem, j)) < 255) {
                    maxval = GET_DATA_BYTE(lines, j);
                    if (i > 0) {
                        if (j > 0) {
                            val1 = GET_DATA_BYTE(lines - wpls, j - 1);
                            maxval = L_MAX(maxval, val1);
                        }
                        if (j < jmax) {
                            val3 = GET_DATA_BYTE(lines - wpls, j + 1);
                            maxval = L_MAX(maxval, val3);
                        }
                        val2 = GET_DATA_BYTE(lines - wpls, j);
                        maxval = L_MAX(maxval, val2);
                    }
                    if (j > 0) {
                        val4 = GET_DATA_BYTE(lines, j - 1);
                        maxval = L_MAX(maxval, val4);
                    }
                    if (maxval > maskval)
                        SET_DATA_BYTE(lines, j, maxval);
                }
            }
        }

            /* LR --> UL scan (anti-raster order)
             * If I : mask image
             *    J : marker image
             * Let p be the currect pixel;
             * tmp <- max{J(p) union J(p) neighbors in anti-raster order}
             * if (tmp > I(p))
             *   J(p) <- tmp
             * end */
        for (i = imax; i >= 0; i--) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = jmax; j >= 0; j--) {
                boolval = FALSE;
                if ((maskval = GET_DATA_BYTE(linem, j)) < 255) {
                    maxval = GET_DATA_BYTE(lines, j);
                    if (i < imax) {
                        if (j > 0) {
                            val6 = GET_DATA_BYTE(lines + wpls, j - 1);
                            maxval = L_MAX(maxval, val6);
                        }
                        if (j < jmax) {
                            val8 = GET_DATA_BYTE(lines + wpls, j + 1);
                            maxval = L_MAX(maxval, val8);
                        }
                        val7 = GET_DATA_BYTE(lines + wpls, j);
                        maxval = L_MAX(maxval, val7);
                    }
                    if (j < jmax) {
                        val5 = GET_DATA_BYTE(lines, j + 1);
                        maxval = L_MAX(maxval, val5);
                    }
                    if (maxval > maskval)
                        SET_DATA_BYTE(lines, j, maxval);
                    val = GET_DATA_BYTE(lines, j);

                        /*
                         * If there exists a point (q) which belongs to J(p)
                         * neighbors in anti-raster order such that J(q) < J(p)
                         * and J(p) > I(q) then
                         * fifo_add(p) */
                    if (i < imax) {
                        if (j > 0) {
                            val6 = GET_DATA_BYTE(lines + wpls, j - 1);
                            if ((val6 < val) &&
                                (val > GET_DATA_BYTE(linem + wplm, j - 1))) {
                                boolval = TRUE;
                            }
                        }
                        if (j < jmax) {
                            val8 = GET_DATA_BYTE(lines + wpls, j + 1);
                            if (!boolval && (val8 < val) &&
                                (val > GET_DATA_BYTE(linem + wplm, j + 1))) {
                                boolval = TRUE;
                            }
                        }
                        val7 = GET_DATA_BYTE(lines + wpls, j);
                        if (!boolval && (val7 < val) &&
                            (val > GET_DATA_BYTE(linem + wplm, j))) {
                            boolval = TRUE;
                        }
                    }
                    if (j < jmax) {
                        val5 = GET_DATA_BYTE(lines, j + 1);
                        if (!boolval && (val5 < val) &&
                            (val > GET_DATA_BYTE(linem, j + 1))) {
                            boolval = TRUE;
                        }
                    }
                    if (boolval) {
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i;
                        pixel->y = j;
                        lqueueAdd(lq_pixel, pixel);
                    }
                }
            }
        }

            /* Propagation step:
             *        while fifo_empty = false
             *          p <- fifo_first()
             *          for every pixel (q) belong to neighbors of (p)
             *            if J(q) < J(p) and J(p) > I(q)
             *              J(q) <- min(J(p), I(q));
             *              fifo_add(q);
             *            end
             *          end
             *        end */
        queue_size = lqueueGetCount(lq_pixel);
        while (queue_size) {
            pixel = (L_PIXEL *)lqueueRemove(lq_pixel);
            i = pixel->x;
            j = pixel->y;
            LEPT_FREE(pixel);
            lines = datas + i * wpls;
            linem = datam + i * wplm;

            if ((val = GET_DATA_BYTE(lines, j)) > 0) {
                if (i > 0) {
                    if (j > 0) {
                        val1 = GET_DATA_BYTE(lines - wpls, j - 1);
                        maskval = GET_DATA_BYTE(linem - wplm, j - 1);
                        if (val > val1 && val > maskval) {
                            SET_DATA_BYTE(lines - wpls, j - 1, val);
                            pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                            pixel->x = i - 1;
                            pixel->y = j - 1;
                            lqueueAdd(lq_pixel, pixel);
                        }
                    }
                    if (j < jmax) {
                        val3 = GET_DATA_BYTE(lines - wpls, j + 1);
                        maskval = GET_DATA_BYTE(linem - wplm, j + 1);
                        if (val > val3 && val > maskval) {
                            SET_DATA_BYTE(lines - wpls, j + 1, val);
                            pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                            pixel->x = i - 1;
                            pixel->y = j + 1;
                            lqueueAdd(lq_pixel, pixel);
                        }
                    }
                    val2 = GET_DATA_BYTE(lines - wpls, j);
                    maskval = GET_DATA_BYTE(linem - wplm, j);
                    if (val > val2 && val > maskval) {
                        SET_DATA_BYTE(lines - wpls, j, val);
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i - 1;
                        pixel->y = j;
                        lqueueAdd(lq_pixel, pixel);
                    }

                }
                if (j > 0) {
                    val4 = GET_DATA_BYTE(lines, j - 1);
                    maskval = GET_DATA_BYTE(linem, j - 1);
                    if (val > val4 && val > maskval) {
                        SET_DATA_BYTE(lines, j - 1, val);
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i;
                        pixel->y = j - 1;
                        lqueueAdd(lq_pixel, pixel);
                    }
                }
                if (i < imax) {
                    if (j > 0) {
                        val6 = GET_DATA_BYTE(lines + wpls, j - 1);
                        maskval = GET_DATA_BYTE(linem + wplm, j - 1);
                        if (val > val6 && val > maskval) {
                            SET_DATA_BYTE(lines + wpls, j - 1, val);
                            pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                            pixel->x = i + 1;
                            pixel->y = j - 1;
                            lqueueAdd(lq_pixel, pixel);
                        }
                    }
                    if (j < jmax) {
                        val8 = GET_DATA_BYTE(lines + wpls, j + 1);
                        maskval = GET_DATA_BYTE(linem + wplm, j + 1);
                        if (val > val8 && val > maskval) {
                            SET_DATA_BYTE(lines + wpls, j + 1, val);
                            pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                            pixel->x = i + 1;
                            pixel->y = j + 1;
                            lqueueAdd(lq_pixel, pixel);
                        }
                    }
                    val7 = GET_DATA_BYTE(lines + wpls, j);
                    maskval = GET_DATA_BYTE(linem + wplm, j);
                    if (val > val7 && val > maskval) {
                        SET_DATA_BYTE(lines + wpls, j, val);
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i + 1;
                        pixel->y = j;
                        lqueueAdd(lq_pixel, pixel);
                    }
                }
                if (j < jmax) {
                    val5 = GET_DATA_BYTE(lines, j + 1);
                    maskval = GET_DATA_BYTE(linem, j + 1);
                    if (val > val5 && val > maskval) {
                        SET_DATA_BYTE(lines, j + 1, val);
                        pixel = (L_PIXEL *)LEPT_CALLOC(1, sizeof(L_PIXEL));
                        pixel->x = i;
                        pixel->y = j + 1;
                        lqueueAdd(lq_pixel, pixel);
                    }
                }
            }

            queue_size = lqueueGetCount(lq_pixel);
        }
        break;

    default:
        L_ERROR("shouldn't get here!\n", procName);
        break;
    }

    lqueueDestroy(&lq_pixel, TRUE);
    return;
}

/*-----------------------------------------------------------------------*
 *                 Vincent's Iterative Grayscale Seedfill                *
 *-----------------------------------------------------------------------*/
/*!
 *  seedfillGrayLowSimple()
 *
 *  Notes:
 *      (1) The pixels are numbered as follows:
 *              1  2  3
 *              4  x  5
 *              6  7  8
 *          This low-level filling operation consists of two scans,
 *          raster and anti-raster, covering the entire seed image.
 *          The caller typically iterates until the filling is
 *          complete.
 *      (2) The filling action can be visualized from the following example.
 *          Suppose the mask, which clips the fill, is a sombrero-shaped
 *          surface, where the highest point is 200 and the low pixels
 *          around the rim are 30.  Beyond the rim, the mask goes up a bit.
 *          Suppose the seed, which is filled, consists of a single point
 *          of height 150, located below the max of the mask, with
 *          the rest 0.  Then in the raster scan, nothing happens until
 *          the high seed point is encountered, and then this value is
 *          propagated right and down, until it hits the side of the
 *          sombrero.   The seed can never exceed the mask, so it fills
 *          to the rim, going lower along the mask surface.  When it
 *          passes the rim, the seed continues to fill at the rim
 *          height to the edge of the seed image.  Then on the
 *          anti-raster scan, the seed fills flat inside the
 *          sombrero to the upper and left, and then out from the
 *          rim as before.  The final result has a seed that is
 *          flat outside the rim, and inside it fills the sombrero
 *          but only up to 150.  If the rim height varies, the
 *          filled seed outside the rim will be at the highest
 *          point on the rim, which is a saddle point on the rim.
 */
void
seedfillGrayLowSimple(l_uint32  *datas,
                      l_int32    w,
                      l_int32    h,
                      l_int32    wpls,
                      l_uint32  *datam,
                      l_int32    wplm,
                      l_int32    connectivity)
{
l_uint8    val2, val3, val4, val5, val7, val8;
l_uint8    val, maxval, maskval;
l_int32    i, j, imax, jmax;
l_uint32  *lines, *linem;

    PROCNAME("seedfillGrayLowSimple");

    imax = h - 1;
    jmax = w - 1;

    switch (connectivity)
    {
    case 4:
            /* UL --> LR scan */
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = 0; j < w; j++) {
                if ((maskval = GET_DATA_BYTE(linem, j)) > 0) {
                    maxval = 0;
                    if (i > 0)
                        maxval = GET_DATA_BYTE(lines - wpls, j);
                    if (j > 0) {
                        val4 = GET_DATA_BYTE(lines, j - 1);
                        maxval = L_MAX(maxval, val4);
                    }
                    val = GET_DATA_BYTE(lines, j);
                    maxval = L_MAX(maxval, val);
                    val = L_MIN(maxval, maskval);
                    SET_DATA_BYTE(lines, j, val);
                }
            }
        }

            /* LR --> UL scan */
        for (i = imax; i >= 0; i--) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = jmax; j >= 0; j--) {
                if ((maskval = GET_DATA_BYTE(linem, j)) > 0) {
                    maxval = 0;
                    if (i < imax)
                        maxval = GET_DATA_BYTE(lines + wpls, j);
                    if (j < jmax) {
                        val5 = GET_DATA_BYTE(lines, j + 1);
                        maxval = L_MAX(maxval, val5);
                    }
                    val = GET_DATA_BYTE(lines, j);
                    maxval = L_MAX(maxval, val);
                    val = L_MIN(maxval, maskval);
                    SET_DATA_BYTE(lines, j, val);
                }
            }
        }
        break;

    case 8:
            /* UL --> LR scan */
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = 0; j < w; j++) {
                if ((maskval = GET_DATA_BYTE(linem, j)) > 0) {
                    maxval = 0;
                    if (i > 0) {
                        if (j > 0)
                            maxval = GET_DATA_BYTE(lines - wpls, j - 1);
                        if (j < jmax) {
                            val2 = GET_DATA_BYTE(lines - wpls, j + 1);
                            maxval = L_MAX(maxval, val2);
                        }
                        val3 = GET_DATA_BYTE(lines - wpls, j);
                        maxval = L_MAX(maxval, val3);
                    }
                    if (j > 0) {
                        val4 = GET_DATA_BYTE(lines, j - 1);
                        maxval = L_MAX(maxval, val4);
                    }
                    val = GET_DATA_BYTE(lines, j);
                    maxval = L_MAX(maxval, val);
                    val = L_MIN(maxval, maskval);
                    SET_DATA_BYTE(lines, j, val);
                }
            }
        }

            /* LR --> UL scan */
        for (i = imax; i >= 0; i--) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = jmax; j >= 0; j--) {
                if ((maskval = GET_DATA_BYTE(linem, j)) > 0) {
                    maxval = 0;
                    if (i < imax) {
                        if (j > 0)
                            maxval = GET_DATA_BYTE(lines + wpls, j - 1);
                        if (j < jmax) {
                            val8 = GET_DATA_BYTE(lines + wpls, j + 1);
                            maxval = L_MAX(maxval, val8);
                        }
                        val7 = GET_DATA_BYTE(lines + wpls, j);
                        maxval = L_MAX(maxval, val7);
                    }
                    if (j < jmax) {
                        val5 = GET_DATA_BYTE(lines, j + 1);
                        maxval = L_MAX(maxval, val5);
                    }
                    val = GET_DATA_BYTE(lines, j);
                    maxval = L_MAX(maxval, val);
                    val = L_MIN(maxval, maskval);
                    SET_DATA_BYTE(lines, j, val);
                }
            }
        }
        break;

    default:
        L_ERROR("connectivity must be 4 or 8\n", procName);
    }

    return;
}


/*!
 *  seedfillGrayInvLowSimple()
 *
 *  Notes:
 *      (1) The pixels are numbered as follows:
 *              1  2  3
 *              4  x  5
 *              6  7  8
 *          This low-level filling operation consists of two scans,
 *          raster and anti-raster, covering the entire seed image.
 *          The caller typically iterates until the filling is
 *          complete.
 *      (2) The "Inv" signifies the fact that in this case, filling
 *          of the seed only takes place when the seed value is
 *          greater than the mask value.  The mask will act to stop
 *          the fill when it is higher than the seed level.  (This is
 *          in contrast to conventional grayscale filling where the
 *          seed always fills below the mask.)
 *      (3) An example of use is a basin, described by the mask (pixm),
 *          where within the basin, the seed pix (pixs) gets filled to the
 *          height of the highest seed pixel that is above its
 *          corresponding max pixel.  Filling occurs while the
 *          propagating seed pixels in pixs are larger than the
 *          corresponding mask values in pixm.
 */
void
seedfillGrayInvLowSimple(l_uint32  *datas,
                         l_int32    w,
                         l_int32    h,
                         l_int32    wpls,
                         l_uint32  *datam,
                         l_int32    wplm,
                         l_int32    connectivity)
{
l_uint8    val1, val2, val3, val4, val5, val6, val7, val8;
l_uint8    maxval, maskval;
l_int32    i, j, imax, jmax;
l_uint32  *lines, *linem;

    PROCNAME("seedfillGrayInvLowSimple");

    imax = h - 1;
    jmax = w - 1;

    switch (connectivity)
    {
    case 4:
            /* UL --> LR scan */
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = 0; j < w; j++) {
                if ((maskval = GET_DATA_BYTE(linem, j)) < 255) {
                    maxval = GET_DATA_BYTE(lines, j);
                    if (i > 0) {
                        val2 = GET_DATA_BYTE(lines - wpls, j);
                        maxval = L_MAX(maxval, val2);
                    }
                    if (j > 0) {
                        val4 = GET_DATA_BYTE(lines, j - 1);
                        maxval = L_MAX(maxval, val4);
                    }
                    if (maxval > maskval)
                        SET_DATA_BYTE(lines, j, maxval);
                }
            }
        }

            /* LR --> UL scan */
        for (i = imax; i >= 0; i--) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = jmax; j >= 0; j--) {
                if ((maskval = GET_DATA_BYTE(linem, j)) < 255) {
                    maxval = GET_DATA_BYTE(lines, j);
                    if (i < imax) {
                        val7 = GET_DATA_BYTE(lines + wpls, j);
                        maxval = L_MAX(maxval, val7);
                    }
                    if (j < jmax) {
                        val5 = GET_DATA_BYTE(lines, j + 1);
                        maxval = L_MAX(maxval, val5);
                    }
                    if (maxval > maskval)
                        SET_DATA_BYTE(lines, j, maxval);
                }
            }
        }
        break;

    case 8:
            /* UL --> LR scan */
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = 0; j < w; j++) {
                if ((maskval = GET_DATA_BYTE(linem, j)) < 255) {
                    maxval = GET_DATA_BYTE(lines, j);
                    if (i > 0) {
                        if (j > 0) {
                            val1 = GET_DATA_BYTE(lines - wpls, j - 1);
                            maxval = L_MAX(maxval, val1);
                        }
                        if (j < jmax) {
                            val2 = GET_DATA_BYTE(lines - wpls, j + 1);
                            maxval = L_MAX(maxval, val2);
                        }
                        val3 = GET_DATA_BYTE(lines - wpls, j);
                        maxval = L_MAX(maxval, val3);
                    }
                    if (j > 0) {
                        val4 = GET_DATA_BYTE(lines, j - 1);
                        maxval = L_MAX(maxval, val4);
                    }
                    if (maxval > maskval)
                        SET_DATA_BYTE(lines, j, maxval);
                }
            }
        }

            /* LR --> UL scan */
        for (i = imax; i >= 0; i--) {
            lines = datas + i * wpls;
            linem = datam + i * wplm;
            for (j = jmax; j >= 0; j--) {
                if ((maskval = GET_DATA_BYTE(linem, j)) < 255) {
                    maxval = GET_DATA_BYTE(lines, j);
                    if (i < imax) {
                        if (j > 0) {
                            val6 = GET_DATA_BYTE(lines + wpls, j - 1);
                            maxval = L_MAX(maxval, val6);
                        }
                        if (j < jmax) {
                            val8 = GET_DATA_BYTE(lines + wpls, j + 1);
                            maxval = L_MAX(maxval, val8);
                        }
                        val7 = GET_DATA_BYTE(lines + wpls, j);
                        maxval = L_MAX(maxval, val7);
                    }
                    if (j < jmax) {
                        val5 = GET_DATA_BYTE(lines, j + 1);
                        maxval = L_MAX(maxval, val5);
                    }
                    if (maxval > maskval)
                        SET_DATA_BYTE(lines, j, maxval);
                }
            }
        }
        break;

    default:
        L_ERROR("connectivity must be 4 or 8\n", procName);
    }

    return;
}


/*-----------------------------------------------------------------------*
 *                   Vincent's Distance Function method                  *
 *-----------------------------------------------------------------------*/
/*!
 *  distanceFunctionLow()
 */
void
distanceFunctionLow(l_uint32  *datad,
                    l_int32    w,
                    l_int32    h,
                    l_int32    d,
                    l_int32    wpld,
                    l_int32    connectivity)
{
l_int32    val1, val2, val3, val4, val5, val6, val7, val8, minval, val;
l_int32    i, j, imax, jmax;
l_uint32  *lined;

    PROCNAME("distanceFunctionLow");

        /* One raster scan followed by one anti-raster scan.
         * This does not re-set the 1-boundary of pixels that
         * were initialized to either 0 or maxval. */
    imax = h - 1;
    jmax = w - 1;
    switch (connectivity)
    {
    case 4:
        if (d == 8) {
                /* UL --> LR scan */
            for (i = 1; i < imax; i++) {
                lined = datad + i * wpld;
                for (j = 1; j < jmax; j++) {
                    if ((val = GET_DATA_BYTE(lined, j)) > 0) {
                        val2 = GET_DATA_BYTE(lined - wpld, j);
                        val4 = GET_DATA_BYTE(lined, j - 1);
                        minval = L_MIN(val2, val4);
                        minval = L_MIN(minval, 254);
                        SET_DATA_BYTE(lined, j, minval + 1);
                    }
                }
            }

                /* LR --> UL scan */
            for (i = imax - 1; i > 0; i--) {
                lined = datad + i * wpld;
                for (j = jmax - 1; j > 0; j--) {
                    if ((val = GET_DATA_BYTE(lined, j)) > 0) {
                        val7 = GET_DATA_BYTE(lined + wpld, j);
                        val5 = GET_DATA_BYTE(lined, j + 1);
                        minval = L_MIN(val5, val7);
                        minval = L_MIN(minval + 1, val);
                        SET_DATA_BYTE(lined, j, minval);
                    }
                }
            }
        } else {  /* d == 16 */
                /* UL --> LR scan */
            for (i = 1; i < imax; i++) {
                lined = datad + i * wpld;
                for (j = 1; j < jmax; j++) {
                    if ((val = GET_DATA_TWO_BYTES(lined, j)) > 0) {
                        val2 = GET_DATA_TWO_BYTES(lined - wpld, j);
                        val4 = GET_DATA_TWO_BYTES(lined, j - 1);
                        minval = L_MIN(val2, val4);
                        minval = L_MIN(minval, 0xfffe);
                        SET_DATA_TWO_BYTES(lined, j, minval + 1);
                    }
                }
            }

                /* LR --> UL scan */
            for (i = imax - 1; i > 0; i--) {
                lined = datad + i * wpld;
                for (j = jmax - 1; j > 0; j--) {
                    if ((val = GET_DATA_TWO_BYTES(lined, j)) > 0) {
                        val7 = GET_DATA_TWO_BYTES(lined + wpld, j);
                        val5 = GET_DATA_TWO_BYTES(lined, j + 1);
                        minval = L_MIN(val5, val7);
                        minval = L_MIN(minval + 1, val);
                        SET_DATA_TWO_BYTES(lined, j, minval);
                    }
                }
            }
        }
        break;

    case 8:
        if (d == 8) {
                /* UL --> LR scan */
            for (i = 1; i < imax; i++) {
                lined = datad + i * wpld;
                for (j = 1; j < jmax; j++) {
                    if ((val = GET_DATA_BYTE(lined, j)) > 0) {
                        val1 = GET_DATA_BYTE(lined - wpld, j - 1);
                        val2 = GET_DATA_BYTE(lined - wpld, j);
                        val3 = GET_DATA_BYTE(lined - wpld, j + 1);
                        val4 = GET_DATA_BYTE(lined, j - 1);
                        minval = L_MIN(val1, val2);
                        minval = L_MIN(minval, val3);
                        minval = L_MIN(minval, val4);
                        minval = L_MIN(minval, 254);
                        SET_DATA_BYTE(lined, j, minval + 1);
                    }
                }
            }

                /* LR --> UL scan */
            for (i = imax - 1; i > 0; i--) {
                lined = datad + i * wpld;
                for (j = jmax - 1; j > 0; j--) {
                    if ((val = GET_DATA_BYTE(lined, j)) > 0) {
                        val8 = GET_DATA_BYTE(lined + wpld, j + 1);
                        val7 = GET_DATA_BYTE(lined + wpld, j);
                        val6 = GET_DATA_BYTE(lined + wpld, j - 1);
                        val5 = GET_DATA_BYTE(lined, j + 1);
                        minval = L_MIN(val8, val7);
                        minval = L_MIN(minval, val6);
                        minval = L_MIN(minval, val5);
                        minval = L_MIN(minval + 1, val);
                        SET_DATA_BYTE(lined, j, minval);
                    }
                }
            }
        } else {  /* d == 16 */
                /* UL --> LR scan */
            for (i = 1; i < imax; i++) {
                lined = datad + i * wpld;
                for (j = 1; j < jmax; j++) {
                    if ((val = GET_DATA_TWO_BYTES(lined, j)) > 0) {
                        val1 = GET_DATA_TWO_BYTES(lined - wpld, j - 1);
                        val2 = GET_DATA_TWO_BYTES(lined - wpld, j);
                        val3 = GET_DATA_TWO_BYTES(lined - wpld, j + 1);
                        val4 = GET_DATA_TWO_BYTES(lined, j - 1);
                        minval = L_MIN(val1, val2);
                        minval = L_MIN(minval, val3);
                        minval = L_MIN(minval, val4);
                        minval = L_MIN(minval, 0xfffe);
                        SET_DATA_TWO_BYTES(lined, j, minval + 1);
                    }
                }
            }

                /* LR --> UL scan */
            for (i = imax - 1; i > 0; i--) {
                lined = datad + i * wpld;
                for (j = jmax - 1; j > 0; j--) {
                    if ((val = GET_DATA_TWO_BYTES(lined, j)) > 0) {
                        val8 = GET_DATA_TWO_BYTES(lined + wpld, j + 1);
                        val7 = GET_DATA_TWO_BYTES(lined + wpld, j);
                        val6 = GET_DATA_TWO_BYTES(lined + wpld, j - 1);
                        val5 = GET_DATA_TWO_BYTES(lined, j + 1);
                        minval = L_MIN(val8, val7);
                        minval = L_MIN(minval, val6);
                        minval = L_MIN(minval, val5);
                        minval = L_MIN(minval + 1, val);
                        SET_DATA_TWO_BYTES(lined, j, minval);
                    }
                }
            }
        }
        break;

    default:
        L_ERROR("connectivity must be 4 or 8\n", procName);
        break;
    }

    return;
}


/*-----------------------------------------------------------------------*
 *                 Seed spread (based on distance function)              *
 *-----------------------------------------------------------------------*/
/*!
 *  seedspreadLow()
 *
 *    See pixSeedspread() for a brief description of the algorithm here.
 */
void
seedspreadLow(l_uint32  *datad,
              l_int32    w,
              l_int32    h,
              l_int32    wpld,
              l_uint32  *datat,
              l_int32    wplt,
              l_int32    connectivity)
{
l_int32    val1t, val2t, val3t, val4t, val5t, val6t, val7t, val8t;
l_int32    i, j, imax, jmax, minval, valt, vald;
l_uint32  *linet, *lined;

    PROCNAME("seedspreadLow");

        /* One raster scan followed by one anti-raster scan.
         * pixt is initialized to have 0 on pixels where the
         * input is specified in pixd, and to have 1 on all
         * other pixels.  We only change pixels in pixt and pixd
         * that are non-zero in pixt. */
    imax = h - 1;
    jmax = w - 1;
    switch (connectivity)
    {
    case 4:
            /* UL --> LR scan */
        for (i = 1; i < h; i++) {
            linet = datat + i * wplt;
            lined = datad + i * wpld;
            for (j = 1; j < jmax; j++) {
                if ((valt = GET_DATA_TWO_BYTES(linet, j)) > 0) {
                    val2t = GET_DATA_TWO_BYTES(linet - wplt, j);
                    val4t = GET_DATA_TWO_BYTES(linet, j - 1);
                    minval = L_MIN(val2t, val4t);
                    minval = L_MIN(minval, 0xfffe);
                    SET_DATA_TWO_BYTES(linet, j, minval + 1);
                    if (val2t < val4t)
                        vald = GET_DATA_BYTE(lined - wpld, j);
                    else
                        vald = GET_DATA_BYTE(lined, j - 1);
                    SET_DATA_BYTE(lined, j, vald);
                }
            }
        }

            /* LR --> UL scan */
        for (i = imax - 1; i > 0; i--) {
            linet = datat + i * wplt;
            lined = datad + i * wpld;
            for (j = jmax - 1; j > 0; j--) {
                if ((valt = GET_DATA_TWO_BYTES(linet, j)) > 0) {
                    val7t = GET_DATA_TWO_BYTES(linet + wplt, j);
                    val5t = GET_DATA_TWO_BYTES(linet, j + 1);
                    minval = L_MIN(val5t, val7t);
                    minval = L_MIN(minval + 1, valt);
                    if (valt > minval) {  /* replace */
                        SET_DATA_TWO_BYTES(linet, j, minval);
                        if (val5t < val7t)
                            vald = GET_DATA_BYTE(lined, j + 1);
                        else
                            vald = GET_DATA_BYTE(lined + wplt, j);
                        SET_DATA_BYTE(lined, j, vald);
                    }
                }
            }
        }
        break;
    case 8:
            /* UL --> LR scan */
        for (i = 1; i < h; i++) {
            linet = datat + i * wplt;
            lined = datad + i * wpld;
            for (j = 1; j < jmax; j++) {
                if ((valt = GET_DATA_TWO_BYTES(linet, j)) > 0) {
                    val1t = GET_DATA_TWO_BYTES(linet - wplt, j - 1);
                    val2t = GET_DATA_TWO_BYTES(linet - wplt, j);
                    val3t = GET_DATA_TWO_BYTES(linet - wplt, j + 1);
                    val4t = GET_DATA_TWO_BYTES(linet, j - 1);
                    minval = L_MIN(val1t, val2t);
                    minval = L_MIN(minval, val3t);
                    minval = L_MIN(minval, val4t);
                    minval = L_MIN(minval, 0xfffe);
                    SET_DATA_TWO_BYTES(linet, j, minval + 1);
                    if (minval == val1t)
                        vald = GET_DATA_BYTE(lined - wpld, j - 1);
                    else if (minval == val2t)
                        vald = GET_DATA_BYTE(lined - wpld, j);
                    else if (minval == val3t)
                        vald = GET_DATA_BYTE(lined - wpld, j + 1);
                    else  /* minval == val4t */
                        vald = GET_DATA_BYTE(lined, j - 1);
                    SET_DATA_BYTE(lined, j, vald);
                }
            }
        }

            /* LR --> UL scan */
        for (i = imax - 1; i > 0; i--) {
            linet = datat + i * wplt;
            lined = datad + i * wpld;
            for (j = jmax - 1; j > 0; j--) {
                if ((valt = GET_DATA_TWO_BYTES(linet, j)) > 0) {
                    val8t = GET_DATA_TWO_BYTES(linet + wplt, j + 1);
                    val7t = GET_DATA_TWO_BYTES(linet + wplt, j);
                    val6t = GET_DATA_TWO_BYTES(linet + wplt, j - 1);
                    val5t = GET_DATA_TWO_BYTES(linet, j + 1);
                    minval = L_MIN(val8t, val7t);
                    minval = L_MIN(minval, val6t);
                    minval = L_MIN(minval, val5t);
                    minval = L_MIN(minval + 1, valt);
                    if (valt > minval) {  /* replace */
                        SET_DATA_TWO_BYTES(linet, j, minval);
                        if (minval == val5t + 1)
                            vald = GET_DATA_BYTE(lined, j + 1);
                        else if (minval == val6t + 1)
                            vald = GET_DATA_BYTE(lined + wpld, j - 1);
                        else if (minval == val7t + 1)
                            vald = GET_DATA_BYTE(lined + wpld, j);
                        else  /* minval == val8t + 1 */
                            vald = GET_DATA_BYTE(lined + wpld, j + 1);
                        SET_DATA_BYTE(lined, j, vald);
                    }
                }
            }
        }
        break;
    default:
        L_ERROR("connectivity must be 4 or 8\n", procName);
        break;
    }

    return;
}
