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
 * heap_reg.c
 *
 *   Tests the heap utility.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

struct HeapElement {
    l_float32  distance;
    l_int32    x;
    l_int32    y;
};
typedef struct HeapElement  HEAPEL;

static const l_int32  NELEM = 50;


main(int    argc,
     char **argv)
{
l_int32      i;
l_float32    frand, fval;
HEAPEL      *item;
NUMA        *na;
L_HEAP      *lh;
static char  mainName[] = "heap_reg";

    if (argc != 1)
	exit(ERROR_INT(" Syntax: heap_reg", mainName, 1));

        /* make a numa of random numbers */
    na = numaCreate(5);
    for (i = 0; i < NELEM; i++) {
        frand = (l_float32)rand() / (l_float32)RAND_MAX;
        numaAddNumber(na, frand);
    }

        /* make an array of HEAPELs with the same numbers */
    lh = lheapCreate(5, L_SORT_INCREASING);
    for (i = 0; i < NELEM; i++) {
        numaGetFValue(na, i, &fval);
        item = (HEAPEL *)CALLOC(1, sizeof(HEAPEL));
        item->distance = fval;
        lheapAdd(lh, item);
    }
    lheapPrint(stderr, lh);

        /* switch the direction and resort into a heap */
    lh->direction = L_SORT_DECREASING;
    lheapSort(lh);
    lheapPrint(stderr, lh);

        /* resort for strict order */
    lheapSortStrictOrder(lh);
    lheapPrint(stderr, lh);

        /* switch the direction again and resort into a heap */
    lh->direction = L_SORT_INCREASING;
    lheapSort(lh);
    lheapPrint(stderr, lh);

        /* remove the elements, one at a time */
    for (i = 0; lheapGetCount(lh) > 0; i++) {
        item = (HEAPEL *)lheapRemove(lh);
	fprintf(stderr, "item %d: %f\n", i, item->distance);
	FREE(item);
    }

    lheapDestroy(&lh, 1);
    numaDestroy(&na);
    return 0;
}

