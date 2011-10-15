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
 *  arithlow.c
 *
 *      One image grayscale arithmetic (8, 16 or 32 bpp)
 *            void       addConstantGrayLow()
 *            void       multConstantGrayLow()
 *
 *      Two image grayscale arithmetic (8, 16 or 32 bpp)
 *            void       addGrayLow()
 *            void       subtractGrayLow()
 *
 *      Grayscale threshold operation (8, 16 or 32 bpp)
 *            void       thresholdToValueLow()
 *
 *      Image accumulator arithmetic operations (8, 16, 32 bpp)
 *            void       finalAccumulateLow()
 *            void       finalAccumulateThreshLow()
 *            void       accumulateLow()
 *            void       multConstAccumulateLow()
 *
 *      Absolute value of difference, component-wise.
 *            void       absDifferenceLow()
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "allheaders.h"


/*------------------------------------------------------------------*
 *        One image grayscale arithmetic (8, 16 or 32 bpp)          *
 *------------------------------------------------------------------*/
/*!
 *  addConstantGrayLow()
 */
void
addConstantGrayLow(l_uint32  *data,
                   l_int32    w,
                   l_int32    h,
                   l_int32    d,
                   l_int32    wpl,
                   l_int32    val)
{
l_int32    i, j, pval;
l_uint32  *line;

    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        if (d == 8) {
            if (val < 0) {
                for (j = 0; j < w; j++) {
                    pval = GET_DATA_BYTE(line, j);
                    pval = L_MAX(0, pval + val);
                    SET_DATA_BYTE(line, j, pval);
                }
            }
            else {  /* val >= 0 */
                for (j = 0; j < w; j++) {
                    pval = GET_DATA_BYTE(line, j);
                    pval = L_MIN(255, pval + val);
                    SET_DATA_BYTE(line, j, pval);
                }
            }
        }
        else if (d == 16) {
            if (val < 0) {
                for (j = 0; j < w; j++) {
                    pval = GET_DATA_TWO_BYTES(line, j);
                    pval = L_MAX(0, pval + val);
                    SET_DATA_TWO_BYTES(line, j, pval);
                }
            }
            else {  /* val >= 0 */
                for (j = 0; j < w; j++) {
                    pval = GET_DATA_TWO_BYTES(line, j);
                    pval = L_MIN(0xffff, pval + val);
                    SET_DATA_TWO_BYTES(line, j, pval);
                }
            }
        }
        else {  /* d == 32; no check for overflow (< 0 or > 0xffffffff) */
            for (j = 0; j < w; j++)
                *(line + j) += val;
        }
    }
    return;
}


/*!
 *  multConstantGrayLow()
 */
void
multConstantGrayLow(l_uint32  *data,
                    l_int32    w,
                    l_int32    h,
                    l_int32    d,
                    l_int32    wpl,
                    l_float32  val)
{
l_int32    i, j, pval;
l_uint32   upval;
l_uint32  *line;

    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        if (d == 8) {
            for (j = 0; j < w; j++) {
                pval = GET_DATA_BYTE(line, j);
                pval = (l_int32)(val * pval);
                pval = L_MIN(255, pval);
                SET_DATA_BYTE(line, j, pval);
            }
        }
        else if (d == 16) {
            for (j = 0; j < w; j++) {
                pval = GET_DATA_TWO_BYTES(line, j);
                pval = (l_int32)(val * pval);
                pval = L_MIN(0xffff, pval);
                SET_DATA_TWO_BYTES(line, j, pval);
            }
        }
        else {  /* d == 32; no clipping */
            for (j = 0; j < w; j++) {
                upval = *(line + j);
                upval = (l_uint32)(val * upval); 
                *(line + j) = upval;
            }
        }
    }
    return;
}


/*------------------------------------------------------------------*
 *        Two image grayscale arithmetic (8, 16 or 32 bpp)          *
 *------------------------------------------------------------------*/
/*!
 *  addGrayLow()
 */
void
addGrayLow(l_uint32  *datad,
           l_int32    w,
           l_int32    h,
           l_int32    d,
           l_int32    wpld,
           l_uint32  *datas,
           l_int32    wpls)
{
l_int32    i, j, val, sum;
l_uint32  *lines, *lined;


    for (i = 0; i < h; i++) {
        lined = datad + i * wpld;
        lines = datas + i * wpls;
        if (d == 8) {
            for (j = 0; j < w; j++) {
                sum = GET_DATA_BYTE(lines, j) + GET_DATA_BYTE(lined, j);
                val = L_MIN(sum, 255);
                SET_DATA_BYTE(lined, j, val);
            }
        }
        else if (d == 16) {
            for (j = 0; j < w; j++) {
                sum = GET_DATA_TWO_BYTES(lines, j)
                      + GET_DATA_TWO_BYTES(lined, j);
                val = L_MIN(sum, 0xffff);
                SET_DATA_TWO_BYTES(lined, j, val);
            }
        }
        else {   /* d == 32; no clipping */
            for (j = 0; j < w; j++)
                *(lined + j) += *(lines + j);
        }
    }

    return;
}


/*!
 *  subtractGrayLow()
 */
void
subtractGrayLow(l_uint32  *datad,
                l_int32    w,
                l_int32    h,
                l_int32    d,
                l_int32    wpld,
                l_uint32  *datas,
                l_int32    wpls)
{
l_int32    i, j, val, diff;
l_uint32  *lines, *lined;

    for (i = 0; i < h; i++) {
        lined = datad + i * wpld;
        lines = datas + i * wpls;
        if (d == 8) {
            for (j = 0; j < w; j++) {
                diff = GET_DATA_BYTE(lined, j) - GET_DATA_BYTE(lines, j);
                val = L_MAX(diff, 0);
                SET_DATA_BYTE(lined, j, val);
            }
        }
        else if (d == 16) {
            for (j = 0; j < w; j++) {
                diff = GET_DATA_TWO_BYTES(lined, j)
                       - GET_DATA_TWO_BYTES(lines, j);
                val = L_MAX(diff, 0);
                SET_DATA_TWO_BYTES(lined, j, val);
            }
        }
        else {  /* d == 32; no clipping */
            for (j = 0; j < w; j++)
                *(lined + j) -= *(lines + j);
        }
    }

    return;
}


/*-------------------------------------------------------------*
 *                  Grayscale threshold operation              *
 *-------------------------------------------------------------*/
/*!
 *  thresholdToValueLow()
 */
void
thresholdToValueLow(l_uint32  *datad,
                    l_int32    w,
                    l_int32    h,
                    l_int32    d,
                    l_int32    wpld,
                    l_int32    threshval,
                    l_int32    setval)
{
l_int32    i, j, setabove;
l_uint32  *lined;

    if (setval > threshval)
        setabove = TRUE;
    else
        setabove = FALSE;

    for (i = 0; i < h; i++) {
        lined = datad + i * wpld;
        if (setabove == TRUE) {
            if (d == 8) {
                for (j = 0; j < w; j++) {
                    if (GET_DATA_BYTE(lined, j) - threshval >= 0)
                        SET_DATA_BYTE(lined, j, setval);
                }
            }
            else if (d == 16) {
                for (j = 0; j < w; j++) {
                    if (GET_DATA_TWO_BYTES(lined, j) - threshval >= 0)
                        SET_DATA_TWO_BYTES(lined, j, setval);
                }
            }
            else {  /* d == 32 */
                for (j = 0; j < w; j++) {
                    if (*(lined + j) >= threshval)
                        *(lined + j) = setval;
                }
            }
        }
        else  { /* set if below or at threshold */
            if (d == 8) {
                for (j = 0; j < w; j++) {
                    if (GET_DATA_BYTE(lined, j) - threshval <= 0)
                        SET_DATA_BYTE(lined, j, setval);
                }
            }
            else if (d == 16) {
                for (j = 0; j < w; j++) {
                    if (GET_DATA_TWO_BYTES(lined, j) - threshval <= 0)
                        SET_DATA_TWO_BYTES(lined, j, setval);
                }
            }
            else {  /* d == 32 */
                for (j = 0; j < w; j++) {
                    if (*(lined + j) <= threshval)
                        *(lined + j) = setval;
                }
            }
        }
    }
    return;
}



/*-------------------------------------------------------------*
 *          Image accumulator arithmetic operations            *
 *-------------------------------------------------------------*/
/*!
 *  finalAccumulateLow()
 */
void
finalAccumulateLow(l_uint32  *datad,
                   l_int32    w,
                   l_int32    h,
                   l_int32    d,
                   l_int32    wpld,
                   l_uint32  *datas,
                   l_int32    wpls,
                   l_uint32   offset)
{
l_int32    i, j;
l_int32    val;
l_uint32  *lines, *lined;

    switch (d)
    {
    case 8:
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            for (j = 0; j < w; j++) {
                val = lines[j] - offset;
                val = L_MAX(0, val);
                val = L_MIN(255, val);
                SET_DATA_BYTE(lined, j, (l_uint8)val);
            }
        }
        break;
    case 16:
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            for (j = 0; j < w; j++) {
                val = lines[j] - offset;
                val = L_MAX(0, val);
                val = L_MIN(0xffff, val);
                SET_DATA_TWO_BYTES(lined, j, (l_uint16)val);
            }
        }
        break;
    case 32:
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            for (j = 0; j < w; j++)
                lined[j] = lines[j] - offset;
        }
        break;
    }
    return;
}
   

void
finalAccumulateThreshLow(l_uint32 *datad,
                         l_int32 w,
                         l_int32 h,
                         l_int32 wpld,
                         l_uint32 *datas,
                         l_int32 wpls,
                         l_uint32 offset,
                         l_uint32 threshold)
{
l_int32    i, j;
l_int32    val;
l_uint32  *lines, *lined;

    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;

        for (j = 0; j < w; j++) {
            val = lines[j] - offset;
            if (val >= threshold) {
                SET_DATA_BIT(lined, j);
            }
        }
    }
}


/*!
 *  accumulateLow()
 */
void
accumulateLow(l_uint32  *datad,
              l_int32    w,
              l_int32    h,
              l_int32    wpld,
              l_uint32  *datas,
              l_int32    d,
              l_int32    wpls,
              l_int32    op)
{
l_int32    i, j;
l_uint32  *lines, *lined;

    switch (d)
    {
    case 1:
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            if (op == L_ARITH_ADD) {
                for (j = 0; j < w; j++)
                    lined[j] += GET_DATA_BIT(lines, j);
            }
            else {  /* op == L_ARITH_SUBTRACT */
                for (j = 0; j < w; j++)
                    lined[j] -= GET_DATA_BIT(lines, j);
            }
        }
        break;
    case 8:
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            if (op == L_ARITH_ADD) {
                for (j = 0; j < w; j++)
                    lined[j] += GET_DATA_BYTE(lines, j);
            }
            else {  /* op == L_ARITH_SUBTRACT */
                for (j = 0; j < w; j++)
                    lined[j] -= GET_DATA_BYTE(lines, j);
            }
        }
        break;
    case 16:
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            if (op == L_ARITH_ADD) {
                for (j = 0; j < w; j++)
                    lined[j] += GET_DATA_TWO_BYTES(lines, j);
            }
            else {  /* op == L_ARITH_SUBTRACT */
                for (j = 0; j < w; j++)
                    lined[j] -= GET_DATA_TWO_BYTES(lines, j);
            }
        }
        break;
    case 32:
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;
            if (op == L_ARITH_ADD) {
                for (j = 0; j < w; j++)
                    lined[j] += lines[j];
            }
            else {  /* op == L_ARITH_SUBTRACT */
                for (j = 0; j < w; j++)
                    lined[j] -= lines[j];
            }
        }
        break;
    }
    return;
}
   

/*!
 *  multConstAccumulateLow()
 */
void
multConstAccumulateLow(l_uint32  *data,
                       l_int32    w,
                       l_int32    h,
                       l_int32    wpl,
                       l_float32  factor,
                       l_uint32   offset)
{
l_int32    i, j;
l_int32    val;
l_uint32  *line;

    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        for (j = 0; j < w; j++) {
            val = line[j] - offset;
            val = (l_int32)(val * factor);
            val += offset;
            line[j] = (l_uint32)val;
        }
    }
    return;
}
   

/*-----------------------------------------------------------------------*
 *              Absolute value of difference, component-wise             *
 *-----------------------------------------------------------------------*/
/*!
 *  absDifferenceLow()
 *
 *  Finds the absolute value of the difference of each pixel,
 *  for 8 and 16 bpp gray and for 32 bpp rgb.  For 32 bpp, the
 *  differences are found for each of the RGB components
 *  separately, and the LSB component is ignored.
 *  The results are written into datad.  
 */
void
absDifferenceLow(l_uint32  *datad,
                 l_int32    w,
                 l_int32    h,
                 l_int32    wpld,
                 l_uint32  *datas1,
                 l_uint32  *datas2,
                 l_int32    d,
                 l_int32    wpls)
{
l_int32    i, j, val1, val2, diff;
l_uint32   word1, word2;
l_uint32  *lines1, *lines2, *lined, *pdword;

    PROCNAME("absDifferenceLow");

    switch (d)
    {
    case 8:
        for (i = 0; i < h; i++) {
            lines1 = datas1 + i * wpls;
            lines2 = datas2 + i * wpls;
            lined = datad + i * wpld;
            for (j = 0; j < w; j++) {
                val1 = GET_DATA_BYTE(lines1, j);
                val2 = GET_DATA_BYTE(lines2, j);
                diff = L_ABS(val1 - val2);
                SET_DATA_BYTE(lined, j, diff);
            }
        }
        break;
    case 16:
        for (i = 0; i < h; i++) {
            lines1 = datas1 + i * wpls;
            lines2 = datas2 + i * wpls;
            lined = datad + i * wpld;
            for (j = 0; j < w; j++) {
                val1 = GET_DATA_TWO_BYTES(lines1, j);
                val2 = GET_DATA_TWO_BYTES(lines2, j);
                diff = L_ABS(val1 - val2);
                SET_DATA_TWO_BYTES(lined, j, diff);
            }
        }
        break;
    case 32:
        for (i = 0; i < h; i++) {
            lines1 = datas1 + i * wpls;
            lines2 = datas2 + i * wpls;
            lined = datad + i * wpld;
            for (j = 0; j < w; j++) {
                word1 = lines1[j];
                word2 = lines2[j];
                pdword = lined + j;
                val1 = GET_DATA_BYTE(&word1, COLOR_RED);
                val2 = GET_DATA_BYTE(&word2, COLOR_RED);
                diff = L_ABS(val1 - val2);
                SET_DATA_BYTE(pdword, COLOR_RED, diff);
                val1 = GET_DATA_BYTE(&word1, COLOR_GREEN);
                val2 = GET_DATA_BYTE(&word2, COLOR_GREEN);
                diff = L_ABS(val1 - val2);
                SET_DATA_BYTE(pdword, COLOR_GREEN, diff);
                val1 = GET_DATA_BYTE(&word1, COLOR_BLUE);
                val2 = GET_DATA_BYTE(&word2, COLOR_BLUE);
                diff = L_ABS(val1 - val2);
                SET_DATA_BYTE(pdword, COLOR_BLUE, diff);
            }
        }
        break;
    default:
        L_ERROR("source depth must be 8, 16 or 32 bpp", procName);
        break;
    }

    return;
}

