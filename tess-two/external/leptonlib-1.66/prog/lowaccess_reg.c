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
 * lowaccess_reg.c
 *
 *    Test low-level accessors 
 *
 *    Note that the gnu C++ compiler:
 *      * allows a non-void* ptr to be passed to a function f(void *ptr)
 *      * forbids a void* ptr to be passed to a function f(non-void *ptr)
 *        ('forbids' may be too strong: it issues a warning)
 *    
 *    For this reason, the l_getData*() and l_setData*() accessors
 *    now take a (void *)lineptr, but internally cast to (l_uint32 *)
 *    so that the addressing arithmetic works properly.
 *
 *    By the same token, the GET_DATA_*() and SET_DATA_*() macro
 *    accessors now cast the input ptr to (l_uint32 *) for 1, 2 and 4 bpp.
 *    This allows them to take a (void *)lineptr.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static l_int32 compareResults(PIX *pixs, PIX *pixt1, PIX *pixt2,
                              l_int32 count1, l_int32 count2,
                              const char *descr);

main(int    argc,
     char **argv)
{
l_int32      x, y, i, j, k, w, h, w2, w4, w8, w16, w32, wpl, nerrors;
l_int32      count1, count2, count3, ret, val1, val2;
l_uint32     val32;
l_uint32    *data, *line, *line1, *line2, *data1, *data2;
void       **lines1, **linet1, **linet2;
PIX         *pixs, *pixt1, *pixt2;
static char  mainName[] = "lowaccess_reg";

    pixs = pixRead("feyn.tif");   /* width divisible by 16 */
    pixGetDimensions(pixs, &w, &h, NULL);
    data = pixGetData(pixs);
    wpl = pixGetWpl(pixs);
    lines1 = pixGetLinePtrs(pixs, NULL);

        /* Get timing for the 3 different methods */
    startTimer();
    for (k = 0; k < 10; k++) {
        count1 = 0;
        for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
                if (GET_DATA_BIT(lines1[i], j))
                    count1++;
            }
        }
    }
    fprintf(stderr, "Time with line ptrs     = %5.3f sec, count1 = %d\n",
            stopTimer(), count1);

    startTimer();
    for (k = 0; k < 10; k++) {
        count2 = 0;
        for (i = 0; i < h; i++) {
            line = data + i * wpl;
            for (j = 0; j < w; j++) {
               if (l_getDataBit(line, j))
                    count2++;
            }
        }
    }
    fprintf(stderr, "Time with l_get*        = %5.3f sec, count2 = %d\n",
            stopTimer(), count2);

    startTimer();
    for (k = 0; k < 10; k++) {
        count3 = 0;
        for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
                pixGetPixel(pixs, j, i, &val32);
                count3 += val32;
            }
        }
    }
    fprintf(stderr, "Time with pixGetPixel() = %5.3f sec, count3 = %d\n",
            stopTimer(), count3);

    pixt1 = pixCreateTemplate(pixs);
    data1 = pixGetData(pixt1);
    linet1 = pixGetLinePtrs(pixt1, NULL);
    pixt2 = pixCreateTemplate(pixs);
    data2 = pixGetData(pixt2);
    linet2 = pixGetLinePtrs(pixt2, NULL);

    nerrors = 0;

        /* Test different methods for 1 bpp */
    count1 = 0;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            val1 = GET_DATA_BIT(lines1[i], j);
            count1 += val1;
            if (val1) SET_DATA_BIT(linet1[i], j);
        }
    }
    count2 = 0;
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        line2 = data2 + i * wpl;
        for (j = 0; j < w; j++) {
            val2 = l_getDataBit(line, j);
            count2 += val2;
            if (val2) l_setDataBit(line2, j);
        }
    }
    ret = compareResults(pixs, pixt1, pixt2, count1, count2, "1 bpp");
    nerrors += ret;

        /* Test different methods for 2 bpp */
    count1 = 0;
    w2 = w / 2;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w2; j++) {
            val1 = GET_DATA_DIBIT(lines1[i], j);
            count1 += val1;
            SET_DATA_DIBIT(linet1[i], j, val1);
        }
    }
    count2 = 0;
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        line2 = data2 + i * wpl;
        for (j = 0; j < w2; j++) {
            val2 = l_getDataDibit(line, j);
            count2 += val2;
            l_setDataDibit(line2, j, val2);
        }
    }
    ret = compareResults(pixs, pixt1, pixt2, count1, count2, "2 bpp");
    nerrors += ret;

        /* Test different methods for 4 bpp */
    count1 = 0;
    w4 = w / 4;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w4; j++) {
            val1 = GET_DATA_QBIT(lines1[i], j);
            count1 += val1;
            SET_DATA_QBIT(linet1[i], j, val1);
        }
    }
    count2 = 0;
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        line2 = data2 + i * wpl;
        for (j = 0; j < w4; j++) {
            val2 = l_getDataQbit(line, j);
            count2 += val2;
            l_setDataQbit(line2, j, val2);
        }
    }
    ret = compareResults(pixs, pixt1, pixt2, count1, count2, "4 bpp");
    nerrors += ret;

        /* Test different methods for 8 bpp */
    count1 = 0;
    w8 = w / 8;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w8; j++) {
            val1 = GET_DATA_BYTE(lines1[i], j);
            count1 += val1;
            SET_DATA_BYTE(linet1[i], j, val1);
        }
    }
    count2 = 0;
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        line2 = data2 + i * wpl;
        for (j = 0; j < w8; j++) {
            val2 = l_getDataByte(line, j);
            count2 += val2;
            l_setDataByte(line2, j, val2);
        }
    }
    ret = compareResults(pixs, pixt1, pixt2, count1, count2, "8 bpp");
    nerrors += ret;

        /* Test different methods for 16 bpp */
    count1 = 0;
    w16 = w / 16;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w16; j++) {
            val1 = GET_DATA_TWO_BYTES(lines1[i], j);
            count1 += val1;
            SET_DATA_TWO_BYTES(linet1[i], j, val1);
        }
    }
    count2 = 0;
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        line2 = data2 + i * wpl;
        for (j = 0; j < w16; j++) {
            val2 = l_getDataTwoBytes(line, j);
            count2 += val2;
            l_setDataTwoBytes(line2, j, val2);
        }
    }
    ret = compareResults(pixs, pixt1, pixt2, count1, count2, "16 bpp");
    nerrors += ret;

        /* Test different methods for 32 bpp */
    count1 = 0;
    w32 = w / 32;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w32; j++) {
            val1 = GET_DATA_FOUR_BYTES(lines1[i], j);
            count1 += val1 & 0xfff;
            SET_DATA_FOUR_BYTES(linet1[i], j, val1);
        }
    }
    count2 = 0;
    for (i = 0; i < h; i++) {
        line = data + i * wpl;
        line2 = data2 + i * wpl;
        for (j = 0; j < w32; j++) {
            val2 = l_getDataFourBytes(line, j);
            count2 += val2 & 0xfff;
            l_setDataFourBytes(line2, j, val2);
        }
    }
    ret = compareResults(pixs, pixt1, pixt2, count1, count2, "32 bpp");
    nerrors += ret;

    if (!nerrors)
        fprintf(stderr, "****  No errors  ****\n");
    else
        fprintf(stderr, "****  %d errors found!  ****\n", nerrors);

    pixDestroy(&pixs);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    FREE(lines1);
    FREE(linet1);
    FREE(linet2);
}


static l_int32
compareResults(PIX         *pixs,
               PIX         *pixt1,
               PIX         *pixt2,
               l_int32      count1,
               l_int32      count2,
               const char  *descr)
{
l_int32  ret, same;

    ret = 0;
    pixEqual(pixs, pixt1, &same);
    if (!same) {
        fprintf(stderr, "pixt1 != pixs in %s\n", descr);
        ret = 1;
    }
    pixEqual(pixs, pixt2, &same);
    if (!same) {
        fprintf(stderr, "pixt2 != pixs in %s\n", descr);
        ret = 1;
    }
    if (count1 != count2) {
        fprintf(stderr, "Counts not same in %s\n", descr);
        ret = 1;
    } else
        fprintf(stderr, "Counts equal in %s: %d\n", descr, count1);
    pixClearAll(pixt1);
    pixClearAll(pixt2);
    if (!ret)
        fprintf(stderr, "All OK for %s\n", descr);
    return ret;
}

