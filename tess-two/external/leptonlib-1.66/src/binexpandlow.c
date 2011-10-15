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
 *  binexpandlow.c
 *
 *      Low level power-of-2 binary expansion
 *              l_int32      expandBinaryPower2Low()
 *
 *      Expansion tables
 *              l_uint16    *makeExpandTab2x()
 *              l_uint32    *makeExpandTab4x()
 *              l_uint32    *makeExpandTab8x()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"


static  l_uint32 expandtab16[] = {
            0x00000000, 0x0000ffff, 0xffff0000, 0xffffffff};


/*-------------------------------------------------------------------*
 *              Low level power-of-2 binary expansion                *
 *-------------------------------------------------------------------*/
/*!
 *  expandBinaryPower2Low()
 */
l_int32
expandBinaryPower2Low(l_uint32  *datad,
                      l_int32    wd,
                      l_int32    hd,
                      l_int32    wpld,
                      l_uint32  *datas,
                      l_int32    ws,
                      l_int32    hs,
                      l_int32    wpls,
                      l_int32    factor)
{
l_int32    i, j, k, sdibits, sqbits, sbytes;
l_uint8    sval;
l_uint16  *tab2;
l_uint32  *tab4, *tab8;
l_uint32  *lines, *lined;

    PROCNAME("expandBinaryPower2Low");

    switch (factor)
    {
    case 2:
        if ((tab2 = makeExpandTab2x()) == NULL)
            return ERROR_INT("tab2 not made", procName, 1);
        sbytes = (ws + 7) / 8;
        for (i = 0; i < hs; i++) {
            lines = datas + i * wpls;
            lined = datad + 2 * i * wpld;
            for (j = 0; j < sbytes; j++) {
                sval = GET_DATA_BYTE(lines, j);
                SET_DATA_TWO_BYTES(lined, j, tab2[sval]);
            }
            memcpy((char *)(lined + wpld), (char *)lined, 4 * wpld);
        }
        FREE(tab2);
        break;
    case 4:
        if ((tab4 = makeExpandTab4x()) == NULL)
            return ERROR_INT("tab4 not made", procName, 1);
        sbytes = (ws + 7) / 8;
        for (i = 0; i < hs; i++) {
            lines = datas + i * wpls;
            lined = datad + 4 * i * wpld;
            for (j = 0; j < sbytes; j++) {
                sval = GET_DATA_BYTE(lines, j);
                lined[j] = tab4[sval];
            }
            for (k = 1; k < 4; k++) 
                memcpy((char *)(lined + k * wpld), (char *)lined, 4 * wpld);
        }
        FREE(tab4);
        break;
    case 8:
        if ((tab8 = makeExpandTab8x()) == NULL)
            return ERROR_INT("tab8 not made", procName, 1);
        sqbits = (ws + 3) / 4;
        for (i = 0; i < hs; i++) {
            lines = datas + i * wpls;
            lined = datad + 8 * i * wpld;
            for (j = 0; j < sqbits; j++) {
                sval = GET_DATA_QBIT(lines, j);
                if (sval > 15)
                    L_WARNING_INT("sval = %d; should be < 16", procName, sval);
                lined[j] = tab8[sval];
            }
            for (k = 1; k < 8; k++) 
                memcpy((char *)(lined + k * wpld), (char *)lined, 4 * wpld);
        }
        FREE(tab8);
        break;
    case 16:
        sdibits = (ws + 1) / 2;
        for (i = 0; i < hs; i++) {
            lines = datas + i * wpls;
            lined = datad + 16 * i * wpld;
            for (j = 0; j < sdibits; j++) {
                sval = GET_DATA_DIBIT(lines, j);
                lined[j] = expandtab16[sval];
            }
            for (k = 1; k < 16; k++) 
                memcpy((char *)(lined + k * wpld), (char *)lined, 4 * wpld);
        }
        break;
    default:
        return ERROR_INT("expansion factor not in {2,4,8,16}", procName, 1);
    }

    return 0;
}



/*-------------------------------------------------------------------*
 *             Expansion tables for 2x, 4x and 8x expansion          *
 *-------------------------------------------------------------------*/
l_uint16 *
makeExpandTab2x(void)
{
l_uint16  *tab;
l_int32    i;

    PROCNAME("makeExpandTab2x");

    if ((tab = (l_uint16 *) CALLOC(256, sizeof(l_uint16))) == NULL)
        return (l_uint16 *)ERROR_PTR("tab not made", procName, NULL);

    for (i = 0; i < 256; i++) {
        if (i & 0x01)
            tab[i] = 0x3;
        if (i & 0x02)
            tab[i] |= 0xc;
        if (i & 0x04)
            tab[i] |= 0x30;
        if (i & 0x08)
            tab[i] |= 0xc0;
        if (i & 0x10)
            tab[i] |= 0x300;
        if (i & 0x20)
            tab[i] |= 0xc00;
        if (i & 0x40)
            tab[i] |= 0x3000;
        if (i & 0x80)
            tab[i] |= 0xc000;
    }

    return tab;
}


l_uint32 *
makeExpandTab4x(void)
{
l_uint32  *tab;
l_int32    i;

    PROCNAME("makeExpandTab4x");

    if ((tab = (l_uint32 *) CALLOC(256, sizeof(l_uint32))) == NULL)
        return (l_uint32 *)ERROR_PTR("tab not made", procName, NULL);

    for (i = 0; i < 256; i++) {
        if (i & 0x01)
            tab[i] = 0xf;
        if (i & 0x02)
            tab[i] |= 0xf0;
        if (i & 0x04)
            tab[i] |= 0xf00;
        if (i & 0x08)
            tab[i] |= 0xf000;
        if (i & 0x10)
            tab[i] |= 0xf0000;
        if (i & 0x20)
            tab[i] |= 0xf00000;
        if (i & 0x40)
            tab[i] |= 0xf000000;
        if (i & 0x80)
            tab[i] |= 0xf0000000;
    }

    return tab;
}


l_uint32 *
makeExpandTab8x(void)
{
l_uint32  *tab;
l_int32    i;

    PROCNAME("makeExpandTab8x");

    if ((tab = (l_uint32 *) CALLOC(16, sizeof(l_uint32))) == NULL)
        return (l_uint32 *)ERROR_PTR("tab not made", procName, NULL);

    for (i = 0; i < 16; i++) {
        if (i & 0x01)
            tab[i] = 0xff;
        if (i & 0x02)
            tab[i] |= 0xff00;
        if (i & 0x04)
            tab[i] |= 0xff0000;
        if (i & 0x08)
            tab[i] |= 0xff000000;
    }

    return tab;
}

