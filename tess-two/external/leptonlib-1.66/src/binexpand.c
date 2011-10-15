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
 *  binexpand.c
 *
 *      Replicated expansion (integer scaling)
 *         PIX     *pixExpandBinaryReplicate()
 *
 *      Special case: power of 2 replicated expansion
 *         PIX     *pixExpandBinaryPower2()
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "allheaders.h"


/*------------------------------------------------------------------*
 *              Replicated expansion (integer scaling)              *
 *------------------------------------------------------------------*/
/*!
 *  pixExpandBinaryReplicate()
 *
 *      Input:  pixs (1 bpp)
 *              factor (integer scale factor for replicative expansion)
 *      Return: pixd (scaled up), or null on error
 */
PIX *
pixExpandBinaryReplicate(PIX     *pixs,
                         l_int32  factor)
{
l_int32    w, h, d, wd, hd, wpls, wpld, i, j, k, start;
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixExpandBinaryReplicate");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1)
        return (PIX *)ERROR_PTR("pixs not binary", procName, NULL);
    if (factor <= 0)
        return (PIX *)ERROR_PTR("factor <= 0; invalid", procName, NULL);

    if (factor == 1)
        return pixCopy(NULL, pixs);
    if (factor == 2 || factor == 4 || factor == 8 || factor == 16)
        return pixExpandBinaryPower2(pixs, factor);

    wpls = pixGetWpl(pixs);
    datas = pixGetData(pixs);
    wd = factor * w;
    hd = factor * h;
    if ((pixd = pixCreate(wd, hd, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, (l_float32)factor, (l_float32)factor);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);

    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + factor * i * wpld;
        for (j = 0; j < w; j++) {
            if (GET_DATA_BIT(lines, j)) {
                start = factor * j;
                for (k = 0; k < factor; k++)
                    SET_DATA_BIT(lined, start + k);
            }
        }
        for (k = 1; k < factor; k++)
            memcpy(lined + k * wpld, lined, 4 * wpld);
    }

    return pixd;
}


/*------------------------------------------------------------------*
 *                      Power of 2 expansion                        *
 *------------------------------------------------------------------*/
/*!
 *  pixExpandBinaryPower2()
 *
 *      Input:  pixs (1 bpp)
 *              factor (expansion factor: 1, 2, 4, 8, 16)
 *      Return: pixd (expanded 1 bpp by replication), or null on error
 */
PIX *
pixExpandBinaryPower2(PIX     *pixs,
                      l_int32  factor)
{
l_int32    w, h, d, wd, hd, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixExpandBinaryPower2");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1)
        return (PIX *)ERROR_PTR("pixs not binary", procName, NULL);
    if (factor == 1)
        return pixCopy(NULL, pixs);
    if (factor != 2 && factor != 4 && factor != 8 && factor != 16)
        return (PIX *)ERROR_PTR("factor must be in {2,4,8,16}", procName, NULL);

    wpls = pixGetWpl(pixs);
    datas = pixGetData(pixs);
    wd = factor * w;
    hd = factor * h;
    if ((pixd = pixCreate(wd, hd, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyResolution(pixd, pixs);
    pixScaleResolution(pixd, (l_float32)factor, (l_float32)factor);
    wpld = pixGetWpl(pixd);
    datad = pixGetData(pixd);

    expandBinaryPower2Low(datad, wd, hd, wpld, datas, w, h, wpls, factor);

    return pixd;
}

