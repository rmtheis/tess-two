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
 *  rotateorthlow.c
 *
 *      90-degree rotation (cw)
 *            void      rotate90Low()
 *
 *      LR-flip
 *            void      flipLRLow()
 *
 *      TB-flip
 *            void      flipTBLow()
 *
 *      Byte reverse tables
 *            l_uint8  *makeReverseByteTab1()
 *            l_uint8  *makeReverseByteTab2()
 *            l_uint8  *makeReverseByteTab4()
 */

#include <string.h>
#include "allheaders.h"


/*------------------------------------------------------------------*
 *                           90 degree rotation                     *
 *------------------------------------------------------------------*/
/*!
 *  rotate90Low()
 *
 *      direction:  1 for cw rotation
 *                 -1 for ccw rotation
 *
 *  Notes:
 *      (1) The dest must be cleared in advance because not
 *          all source pixels are written to the destination.
 */
void
rotate90Low(l_uint32  *datad,
            l_int32    wd,
            l_int32    hd,
            l_int32    d,
            l_int32    wpld,
            l_uint32  *datas,
            l_int32    wpls,
            l_int32    direction)
{
l_int32    i, j, k, m, iend, nswords;
l_uint32   val, word;
l_uint32  *lines, *lined;

    PROCNAME("rotate90Low");

    if (direction == 1) {  /* clockwise */
        switch (d)
        {
            case 32:
                for (i = 0; i < hd; i++) {
                    lined = datad + i * wpld;
                    lines = datas + (wd - 1) * wpls;
                    for (j = 0; j < wd; j++) {
                        lined[j] = lines[i];
                        lines -= wpls;
                    }
                }
                break;
            case 16:
                for (i = 0; i < hd; i++) {
                    lined = datad + i * wpld;
                    lines = datas + (wd - 1) * wpls;
                    for (j = 0; j < wd; j++) {
                        if ((val = GET_DATA_TWO_BYTES(lines, i)))
                            SET_DATA_TWO_BYTES(lined, j, val);
                        lines -= wpls;
                    }
                }
                break;
            case 8:
                for (i = 0; i < hd; i++) {
                    lined = datad + i * wpld;
                    lines = datas + (wd - 1) * wpls;
                    for (j = 0; j < wd; j++) {
                        if ((val = GET_DATA_BYTE(lines, i)))
                            SET_DATA_BYTE(lined, j, val);
                        lines -= wpls;
                    }
                }
                break;
            case 4:
                for (i = 0; i < hd; i++) {
                    lined = datad + i * wpld;
                    lines = datas + (wd - 1) * wpls;
                    for (j = 0; j < wd; j++) {
                        if ((val = GET_DATA_QBIT(lines, i)))
                            SET_DATA_QBIT(lined, j, val);
                        lines -= wpls;
                    }
                }
                break;
            case 2:
                for (i = 0; i < hd; i++) {
                    lined = datad + i * wpld;
                    lines = datas + (wd - 1) * wpls;
                    for (j = 0; j < wd; j++) {
                        if ((val = GET_DATA_DIBIT(lines, i)))
                            SET_DATA_DIBIT(lined, j, val);
                        lines -= wpls;
                    }
                }
                break;
            case 1:
                nswords = hd / 32;
                for (j = 0; j < wd; j++) {
                    lined = datad;
                    lines = datas + (wd - 1 - j) * wpls;
                    for (k = 0; k < nswords; k++) {
                        word = lines[k];
                        if (!word) {
                            lined += 32 * wpld;
                            continue;
                        } else {
                            iend = 32 * (k + 1);
                            for (m = 0, i = 32 * k; i < iend; i++, m++) {
                                if ((word << m) & 0x80000000)
                                    SET_DATA_BIT(lined, j);
                                lined += wpld;
                            }
                        }
                    }
                    for (i = 32 * nswords; i < hd; i++) {
                        if (GET_DATA_BIT(lines, i))
                            SET_DATA_BIT(lined, j);
                        lined += wpld;
                    }
                }
                break;
            default:
                L_ERROR("illegal depth\n", procName);
                break;
        }
    } else  {     /* direction counter-clockwise */
        switch (d)
        {
            case 32:
                for (i = 0; i < hd; i++) {
                    lined = datad + i * wpld;
                    lines = datas;
                    for (j = 0; j < wd; j++) {
                        lined[j] = lines[hd - 1 - i];
                        lines += wpls;
                    }
                }
                break;
            case 16:
                for (i = 0; i < hd; i++) {
                    lined = datad + i * wpld;
                    lines = datas;
                    for (j = 0; j < wd; j++) {
                        if ((val = GET_DATA_TWO_BYTES(lines, hd - 1 - i)))
                            SET_DATA_TWO_BYTES(lined, j, val);
                        lines += wpls;
                    }
                }
                break;
            case 8:
                for (i = 0; i < hd; i++) {
                    lined = datad + i * wpld;
                    lines = datas;
                    for (j = 0; j < wd; j++) {
                        if ((val = GET_DATA_BYTE(lines, hd - 1 - i)))
                            SET_DATA_BYTE(lined, j, val);
                        lines += wpls;
                    }
                }
                break;
            case 4:
                for (i = 0; i < hd; i++) {
                    lined = datad + i * wpld;
                    lines = datas;
                    for (j = 0; j < wd; j++) {
                        if ((val = GET_DATA_QBIT(lines, hd - 1 - i)))
                            SET_DATA_QBIT(lined, j, val);
                        lines += wpls;
                    }
                }
                break;
            case 2:
                for (i = 0; i < hd; i++) {
                    lined = datad + i * wpld;
                    lines = datas;
                    for (j = 0; j < wd; j++) {
                        if ((val = GET_DATA_DIBIT(lines, hd - 1 - i)))
                            SET_DATA_DIBIT(lined, j, val);
                        lines += wpls;
                    }
                }
                break;
            case 1:
                nswords = hd / 32;
                for (j = 0; j < wd; j++) {
                    lined = datad + (hd - 1) * wpld;
                    lines = datas + (wd - 1 - j) * wpls;
                    for (k = 0; k < nswords; k++) {
                        word = lines[k];
                        if (!word) {
                            lined -= 32 * wpld;
                            continue;
                        } else {
                            iend = 32 * (k + 1);
                            for (m = 0, i = 32 * k; i < iend; i++, m++) {
                                if ((word << m) & 0x80000000)
                                    SET_DATA_BIT(lined, wd - 1 - j);
                                lined -= wpld;
                            }
                        }
                    }
                    for (i = 32 * nswords; i < hd; i++) {
                        if (GET_DATA_BIT(lines, i))
                            SET_DATA_BIT(lined, wd - 1 - j);
                        lined -= wpld;
                    }
                }
                break;
            default:
                L_ERROR("illegal depth\n", procName);
                break;
        }
    }

    return;
}


/*------------------------------------------------------------------*
 *                           Left/right flip                        *
 *------------------------------------------------------------------*/
/*!
 *  flipLRLow()
 *
 *  Notes:
 *      (1) The pixel access routines allow a trivial implementation.
 *          However, for d < 8, it is more efficient to right-justify
 *          each line to a 32-bit boundary and then extract bytes and
 *          do pixel reversing.   In those cases, as in the 180 degree
 *          rotation, we right-shift the data (if necessary) to
 *          right-justify on the 32 bit boundary, and then read the
 *          bytes off each raster line in reverse order, reversing
 *          the pixels in each byte using a table.  These functions
 *          for 1, 2 and 4 bpp were tested against the "trivial"
 *          version (shown here for 4 bpp):
 *              for (i = 0; i < h; i++) {
 *                  line = data + i * wpl;
 *                  memcpy(buffer, line, bpl);
 *                    for (j = 0; j < w; j++) {
 *                      val = GET_DATA_QBIT(buffer, w - 1 - j);
 *                        SET_DATA_QBIT(line, j, val);
 *                  }
 *              }
 *      (2) This operation is in-place.
 */
void
flipLRLow(l_uint32  *data,
          l_int32    w,
          l_int32    h,
          l_int32    d,
          l_int32    wpl,
          l_uint8   *tab,
          l_uint32  *buffer)
{
l_int32    extra, shift, databpl, bpl, i, j;
l_uint32   val;
l_uint32  *line;

    PROCNAME("flipLRLow");

    bpl = 4 * wpl;
    switch (d)
    {
        case 32:
            for (i = 0; i < h; i++) {
                line = data + i * wpl;
                memcpy(buffer, line, bpl);
                for (j = 0; j < w; j++)
                    line[j] = buffer[w - 1 - j];
            }
            break;
        case 16:
            for (i = 0; i < h; i++) {
                line = data + i * wpl;
                memcpy(buffer, line, bpl);
                for (j = 0; j < w; j++) {
                    val = GET_DATA_TWO_BYTES(buffer, w - 1 - j);
                    SET_DATA_TWO_BYTES(line, j, val);
                }
            }
            break;
        case 8:
            for (i = 0; i < h; i++) {
                line = data + i * wpl;
                memcpy(buffer, line, bpl);
                for (j = 0; j < w; j++) {
                    val = GET_DATA_BYTE(buffer, w - 1 - j);
                    SET_DATA_BYTE(line, j, val);
                }
            }
            break;
        case 4:
            extra = (w * d) & 31;
            if (extra)
                shift = 8 - extra / 4;
            else
                shift = 0;
            if (shift)
                rasteropHipLow(data, h, d, wpl, 0, h, shift);

            databpl = (w + 1) / 2;
            for (i = 0; i < h; i++) {
                line = data + i * wpl;
                memcpy(buffer, line, bpl);
                for (j = 0; j < databpl; j++) {
                    val = GET_DATA_BYTE(buffer, bpl - 1 - j);
                    SET_DATA_BYTE(line, j, tab[val]);
                }
            }
            break;
        case 2:
            extra = (w * d) & 31;
            if (extra)
                shift = 16 - extra / 2;
            else
                shift = 0;
            if (shift)
                rasteropHipLow(data, h, d, wpl, 0, h, shift);

            databpl = (w + 3) / 4;
            for (i = 0; i < h; i++) {
                line = data + i * wpl;
                memcpy(buffer, line, bpl);
                for (j = 0; j < databpl; j++) {
                    val = GET_DATA_BYTE(buffer, bpl - 1 - j);
                    SET_DATA_BYTE(line, j, tab[val]);
                }
            }
            break;
        case 1:
            extra = (w * d) & 31;
            if (extra)
                shift = 32 - extra;
            else
                shift = 0;
            if (shift)
                rasteropHipLow(data, h, d, wpl, 0, h, shift);

            databpl = (w + 7) / 8;
            for (i = 0; i < h; i++) {
                line = data + i * wpl;
                memcpy(buffer, line, bpl);
                for (j = 0; j < databpl; j++) {
                    val = GET_DATA_BYTE(buffer, bpl - 1 - j);
                    SET_DATA_BYTE(line, j, tab[val]);
                }
            }
            break;
        default:
            L_ERROR("depth not permitted for LR rot\n", procName);
            return;
    }

    return;
}


/*------------------------------------------------------------------*
 *                            Top/bottom flip                       *
 *------------------------------------------------------------------*/
/*!
 *  flipTBLow()
 *
 *  Notes:
 *      (1) This is simple and fast.  We use the memcpy function
 *          to do all the work on aligned data, regardless of pixel
 *          depth.
 *      (2) This operation is in-place.
 */
void
flipTBLow(l_uint32  *data,
          l_int32    h,
          l_int32    wpl,
          l_uint32  *buffer)
{
l_int32    i, k, h2, bpl;
l_uint32  *linet, *lineb;

    h2 = h / 2;
    bpl = 4 * wpl;
    for (i = 0, k = h - 1; i < h2; i++, k--) {
        linet = data + i * wpl;
        lineb = data + k * wpl;
        memcpy(buffer, linet, bpl);
        memcpy(linet, lineb, bpl);
        memcpy(lineb, buffer, bpl);
    }

    return;
}


/*------------------------------------------------------------------*
 *                          Byte reverse tables                     *
 *------------------------------------------------------------------*/
/*!
 *  makeReverseByteTab1()
 *
 *  Notes:
 *      (1) This generates an 8 bit lookup table for reversing
 *          the order of eight 1-bit pixels.
 */
l_uint8 *
makeReverseByteTab1(void)
{
l_int32   i;
l_uint8  *tab;

    PROCNAME("makeReverseByteTab1");

    if ((tab = (l_uint8 *)CALLOC(256, sizeof(l_uint8))) == NULL)
        return (l_uint8 *)ERROR_PTR("calloc fail for tab", procName, NULL);

    for (i = 0; i < 256; i++)
        tab[i] = ((0x80 & i) >> 7) |
                 ((0x40 & i) >> 5) |
                 ((0x20 & i) >> 3) |
                 ((0x10 & i) >> 1) |
                 ((0x08 & i) << 1) |
                 ((0x04 & i) << 3) |
                 ((0x02 & i) << 5) |
                 ((0x01 & i) << 7);

    return tab;
}


/*!
 *  makeReverseByteTab2()
 *
 *  Notes:
 *      (1) This generates an 8 bit lookup table for reversing
 *          the order of four 2-bit pixels.
 */
l_uint8 *
makeReverseByteTab2(void)
{
l_int32   i;
l_uint8  *tab;

    PROCNAME("makeReverseByteTab2");

    if ((tab = (l_uint8 *)CALLOC(256, sizeof(l_uint8))) == NULL)
        return (l_uint8 *)ERROR_PTR("calloc fail for tab", procName, NULL);

    for (i = 0; i < 256; i++)
        tab[i] = ((0xc0 & i) >> 6) |
                 ((0x30 & i) >> 2) |
                 ((0x0c & i) << 2) |
                 ((0x03 & i) << 6);
    return tab;
}


/*!
 *  makeReverseByteTab4()
 *
 *  Notes:
 *      (1) This generates an 8 bit lookup table for reversing
 *          the order of two 4-bit pixels.
 */
l_uint8 *
makeReverseByteTab4(void)
{
l_int32   i;
l_uint8  *tab;

    PROCNAME("makeReverseByteTab4");

    if ((tab = (l_uint8 *)CALLOC(256, sizeof(l_uint8))) == NULL)
        return (l_uint8 *)ERROR_PTR("calloc fail for tab", procName, NULL);

    for (i = 0; i < 256; i++)
        tab[i] = ((0xf0 & i) >> 4) | ((0x0f & i) << 4);
    return tab;
}
