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
 *  convolvelow.c
 *
 *      Grayscale block convolution
 *          void      blockconvLow()
 *          void      blockconvAccumLow()
 *
 *      Binary block sum and rank filter
 *          void      blocksumLow()
 */

#include <stdio.h>
#include "allheaders.h"


/*----------------------------------------------------------------------*
 *                     Grayscale Block Convolution                      *
 *----------------------------------------------------------------------*/
/*!
 *  blockconvLow()
 *
 *      Input:  data   (data of input image, to be convolved)
 *              w, h, wpl
 *              dataa    (data of 32 bpp accumulator)
 *              wpla     (accumulator)
 *              wc      (convolution "half-width")
 *              hc      (convolution "half-height")
 *      Return: void
 *
 *  Notes:
 *      (1) The full width and height of the convolution kernel
 *          are (2 * wc + 1) and (2 * hc + 1).
 *      (2) The lack of symmetry between the handling of the
 *          first (hc + 1) lines and the last (hc) lines,
 *          and similarly with the columns, is due to fact that
 *          for the pixel at (x,y), the accumulator values are
 *          taken at (x + wc, y + hc), (x - wc - 1, y + hc),
 *          (x + wc, y - hc - 1) and (x - wc - 1, y - hc - 1).
 *      (3) We compute sums, normalized as if there were no reduced
 *          area at the boundary.  This under-estimates the value
 *          of the boundary pixels, so we multiply them by another
 *          normalization factor that is greater than 1.
 *      (4) This second normalization is done first for the first
 *          hc + 1 lines; then for the last hc lines; and finally
 *          for the first wc + 1 and last wc columns in the intermediate
 *          lines.
 *      (5) The caller should verify that wc < w and hc < h.
 *          Under those conditions, illegal reads and writes can occur.
 *      (6) Implementation note: to get the same results in the interior
 *          between this function and pixConvolve(), it is necessary to
 *          add 0.5 for roundoff in the main loop that runs over all pixels.
 *          However, if we do that and have white (255) pixels near the
 *          image boundary, some overflow occurs for pixels very close
 *          to the boundary.  We can't fix this by subtracting from the
 *          normalized values for the boundary pixels, because this results
 *          in underflow if the boundary pixels are black (0).  Empirically,
 *          adding 0.25 (instead of 0.5) before truncating in the main
 *          loop will not cause overflow, but this gives some
 *          off-by-1-level errors in interior pixel values.  So we add
 *          0.5 for roundoff in the main loop, and for pixels within a
 *          half filter width of the boundary, use a L_MIN of the
 *          computed value and 255 to avoid overflow during normalization.
 */
void
blockconvLow(l_uint32  *data,
             l_int32    w,
             l_int32    h,
             l_int32    wpl,
             l_uint32  *dataa,
             l_int32    wpla,
             l_int32    wc,
             l_int32    hc)
{
l_int32    i, j, imax, imin, jmax, jmin;
l_int32    wn, hn, fwc, fhc, wmwc, hmhc;
l_float32  norm, normh, normw;
l_uint32   val;
l_uint32  *linemina, *linemaxa, *line;

    PROCNAME("blockconvLow");

    wmwc = w - wc;
    hmhc = h - hc;
    if (wmwc <= 0 || hmhc <= 0) {
        L_ERROR("wc >= w || hc >=h", procName);
        return;
    }
    fwc = 2 * wc + 1;
    fhc = 2 * hc + 1;
    norm = 1. / (fwc * fhc);

        /*------------------------------------------------------------*
         *  compute, using b.c. only to set limits on the accum image *
         *------------------------------------------------------------*/
    for (i = 0; i < h; i++) {
        imin = L_MAX(i - 1 - hc, 0);
        imax = L_MIN(i + hc, h - 1);
        line = data + wpl * i;
        linemina = dataa + wpla * imin;
        linemaxa = dataa + wpla * imax;
        for (j = 0; j < w; j++) {
            jmin = L_MAX(j - 1 - wc, 0);
            jmax = L_MIN(j + wc, w - 1);
            val = linemaxa[jmax] - linemaxa[jmin]
                  + linemina[jmin] - linemina[jmax];
            val = (l_uint8)(norm * val + 0.5);  /* see comment above */
            SET_DATA_BYTE(line, j, val);
        }
    }

        /*------------------------------------------------------------*
         *          now fix normalization for boundary pixels         *
         *------------------------------------------------------------*/
    for (i = 0; i <= hc; i++) {    /* first hc + 1 lines */
        hn = hc + i;
        normh = (l_float32)fhc / (l_float32)hn;   /* > 1 */
        line = data + wpl * i;
        for (j = 0; j <= wc; j++) {
            wn = wc + j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(line, j);
            val = (l_uint8)L_MIN(val * normh * normw, 255);
            SET_DATA_BYTE(line, j, val);
        }
        for (j = wc + 1; j < wmwc; j++) {
            val = GET_DATA_BYTE(line, j);
            val = (l_uint8)L_MIN(val * normh, 255);
            SET_DATA_BYTE(line, j, val);
        }
        for (j = wmwc; j < w; j++) {
            wn = wc + w - j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(line, j);
            val = (l_uint8)L_MIN(val * normh * normw, 255);
            SET_DATA_BYTE(line, j, val);
        }
    }

    for (i = hmhc; i < h; i++) {  /* last hc lines */
        hn = hc + h - i;
        normh = (l_float32)fhc / (l_float32)hn;   /* > 1 */
        line = data + wpl * i;
        for (j = 0; j <= wc; j++) {
            wn = wc + j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(line, j);
            val = (l_uint8)L_MIN(val * normh * normw, 255);
            SET_DATA_BYTE(line, j, val);
        }
        for (j = wc + 1; j < wmwc; j++) {
            val = GET_DATA_BYTE(line, j);
            val = (l_uint8)L_MIN(val * normh, 255);
            SET_DATA_BYTE(line, j, val);
        }
        for (j = wmwc; j < w; j++) {
            wn = wc + w - j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(line, j);
            val = (l_uint8)L_MIN(val * normh * normw, 255);
            SET_DATA_BYTE(line, j, val);
        }
    }

    for (i = hc + 1; i < hmhc; i++) {    /* intermediate lines */
        line = data + wpl * i;
        for (j = 0; j <= wc; j++) {   /* first wc + 1 columns */
            wn = wc + j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(line, j);
            val = (l_uint8)L_MIN(val * normw, 255);
            SET_DATA_BYTE(line, j, val);
        }
        for (j = wmwc; j < w; j++) {   /* last wc columns */
            wn = wc + w - j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(line, j);
            val = (l_uint8)L_MIN(val * normw, 255);
            SET_DATA_BYTE(line, j, val);
        }
    }

    return;
}



/*
 *  blockconvAccumLow()
 *
 *      Input:  datad  (32 bpp dest)
 *              w, h, wpld (of 32 bpp dest)
 *              datas (1, 8 or 32 bpp src)
 *              d (bpp of src)
 *              wpls (of src)
 *      Return: void
 *
 *  Notes:
 *      (1) The general recursion relation is
 *             a(i,j) = v(i,j) + a(i-1, j) + a(i, j-1) - a(i-1, j-1)
 *          For the first line, this reduces to the special case
 *             a(i,j) = v(i,j) + a(i, j-1)
 *          For the first column, the special case is
 *             a(i,j) = v(i,j) + a(i-1, j)
 */
void
blockconvAccumLow(l_uint32  *datad,
                  l_int32    w,
                  l_int32    h,
                  l_int32    wpld,
                  l_uint32  *datas,
                  l_int32    d,
                  l_int32    wpls)
{
l_uint8    val;
l_int32    i, j;
l_uint32   val32;
l_uint32  *lines, *lined, *linedp;

    PROCNAME("blockconvAccumLow");

    lines = datas;
    lined = datad;

    if (d == 1) {
            /* Do the first line */
        for (j = 0; j < w; j++) {
            val = GET_DATA_BIT(lines, j);
            if (j == 0)
                lined[0] = val;
            else
                lined[j] = lined[j - 1] + val;
        }

            /* Do the other lines */
        for (i = 1; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;  /* curr dest line */
            linedp = lined - wpld;   /* prev dest line */
            for (j = 0; j < w; j++) {
                val = GET_DATA_BIT(lines, j);
                if (j == 0)
                    lined[0] = val + linedp[0];
                else 
                    lined[j] = val + lined[j - 1] + linedp[j] - linedp[j - 1];
            }
        }
    }
    else if (d == 8) {
            /* Do the first line */
        for (j = 0; j < w; j++) {
            val = GET_DATA_BYTE(lines, j);
            if (j == 0)
                lined[0] = val;
            else
                lined[j] = lined[j - 1] + val;
        }

            /* Do the other lines */
        for (i = 1; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;  /* curr dest line */
            linedp = lined - wpld;   /* prev dest line */
            for (j = 0; j < w; j++) {
                val = GET_DATA_BYTE(lines, j);
                if (j == 0)
                    lined[0] = val + linedp[0];
                else 
                    lined[j] = val + lined[j - 1] + linedp[j] - linedp[j - 1];
            }
        }
    }
    else if (d == 32) {
            /* Do the first line */
        for (j = 0; j < w; j++) {
            val32 = lines[j];
            if (j == 0)
                lined[0] = val32;
            else
                lined[j] = lined[j - 1] + val32;
        }

            /* Do the other lines */
        for (i = 1; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;  /* curr dest line */
            linedp = lined - wpld;   /* prev dest line */
            for (j = 0; j < w; j++) {
                val32 = lines[j];
                if (j == 0)
                    lined[0] = val32 + linedp[0];
                else 
                    lined[j] = val32 + lined[j - 1] + linedp[j] - linedp[j - 1];
            }
        }
    }
    else
        L_ERROR("depth not 1, 8 or 32 bpp", procName);

    return;
}


/*----------------------------------------------------------------------*
 *                        Binary Block Sum/Rank                         *
 *----------------------------------------------------------------------*/
/*!
 *  blocksumLow()
 *
 *      Input:  datad  (of 8 bpp dest)
 *              w, h, wpl  (of 8 bpp dest)
 *              dataa (of 32 bpp accum)
 *              wpla  (of 32 bpp accum)
 *              wc, hc  (convolution "half-width" and "half-height")
 *      Return: void
 *
 *  Notes:
 *      (1) The full width and height of the convolution kernel
 *          are (2 * wc + 1) and (2 * hc + 1).
 *      (2) The lack of symmetry between the handling of the
 *          first (hc + 1) lines and the last (hc) lines,
 *          and similarly with the columns, is due to fact that
 *          for the pixel at (x,y), the accumulator values are
 *          taken at (x + wc, y + hc), (x - wc - 1, y + hc),
 *          (x + wc, y - hc - 1) and (x - wc - 1, y - hc - 1).
 *      (3) Compute sums of ON pixels within the block filter size,
 *          normalized between 0 and 255, as if there were no reduced
 *          area at the boundary.  This under-estimates the value
 *          of the boundary pixels, so we multiply them by another
 *          normalization factor that is greater than 1.
 *      (4) This second normalization is done first for the first
 *          hc + 1 lines; then for the last hc lines; and finally
 *          for the first wc + 1 and last wc columns in the intermediate
 *          lines.
 *      (5) The caller should verify that wc < w and hc < h.
 *          Under those conditions, illegal reads and writes can occur.
 */
void
blocksumLow(l_uint32  *datad,
            l_int32    w,
            l_int32    h,
            l_int32    wpl,
            l_uint32  *dataa,
            l_int32    wpla,
            l_int32    wc,
            l_int32    hc)
{
l_int32    i, j, imax, imin, jmax, jmin;
l_int32    wn, hn, fwc, fhc, wmwc, hmhc;
l_float32  norm, normh, normw;
l_uint32   val;
l_uint32  *linemina, *linemaxa, *lined;

    PROCNAME("blocksumLow");

    wmwc = w - wc;
    hmhc = h - hc;
    if (wmwc <= 0 || hmhc <= 0) {
        L_ERROR("wc >= w || hc >=h", procName);
        return;
    }
    fwc = 2 * wc + 1;
    fhc = 2 * hc + 1;
    norm = 255. / (fwc * fhc);

        /*------------------------------------------------------------*
         *  compute, using b.c. only to set limits on the accum image *
         *------------------------------------------------------------*/
    for (i = 0; i < h; i++) {
        imin = L_MAX(i - 1 - hc, 0);
        imax = L_MIN(i + hc, h - 1);
        lined = datad + wpl * i;
        linemina = dataa + wpla * imin;
        linemaxa = dataa + wpla * imax;
        for (j = 0; j < w; j++) {
            jmin = L_MAX(j - 1 - wc, 0);
            jmax = L_MIN(j + wc, w - 1);
            val = linemaxa[jmax] - linemaxa[jmin]
                  - linemina[jmax] + linemina[jmin];
            val = (l_uint8)(norm * val);
            SET_DATA_BYTE(lined, j, val);
        }
    }

        /*------------------------------------------------------------*
         *          now fix normalization for boundary pixels         *
         *------------------------------------------------------------*/
    for (i = 0; i <= hc; i++) {    /* first hc + 1 lines */
        hn = hc + i;
        normh = (l_float32)fhc / (l_float32)hn;   /* > 1 */
        lined = datad + wpl * i;
        for (j = 0; j <= wc; j++) {
            wn = wc + j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(lined, j);
            val = (l_uint8)(val * normh * normw);
            SET_DATA_BYTE(lined, j, val);
        }
        for (j = wc + 1; j < wmwc; j++) {
            val = GET_DATA_BYTE(lined, j);
            val = (l_uint8)(val * normh);
            SET_DATA_BYTE(lined, j, val);
        }
        for (j = wmwc; j < w; j++) {
            wn = wc + w - j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(lined, j);
            val = (l_uint8)(val * normh * normw);
            SET_DATA_BYTE(lined, j, val);
        }
    }

    for (i = hmhc; i < h; i++) {  /* last hc lines */
        hn = hc + h - i;
        normh = (l_float32)fhc / (l_float32)hn;   /* > 1 */
        lined = datad + wpl * i;
        for (j = 0; j <= wc; j++) {
            wn = wc + j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(lined, j);
            val = (l_uint8)(val * normh * normw);
            SET_DATA_BYTE(lined, j, val);
        }
        for (j = wc + 1; j < wmwc; j++) {
            val = GET_DATA_BYTE(lined, j);
            val = (l_uint8)(val * normh);
            SET_DATA_BYTE(lined, j, val);
        }
        for (j = wmwc; j < w; j++) {
            wn = wc + w - j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(lined, j);
            val = (l_uint8)(val * normh * normw);
            SET_DATA_BYTE(lined, j, val);
        }
    }

    for (i = hc + 1; i < hmhc; i++) {    /* intermediate lines */
        lined = datad + wpl * i;
        for (j = 0; j <= wc; j++) {   /* first wc + 1 columns */
            wn = wc + j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(lined, j);
            val = (l_uint8)(val * normw);
            SET_DATA_BYTE(lined, j, val);
        }
        for (j = wmwc; j < w; j++) {   /* last wc columns */
            wn = wc + w - j;
            normw = (l_float32)fwc / (l_float32)wn;   /* > 1 */
            val = GET_DATA_BYTE(lined, j);
            val = (l_uint8)(val * normw);
            SET_DATA_BYTE(lined, j, val);
        }
    }

    return;
}

