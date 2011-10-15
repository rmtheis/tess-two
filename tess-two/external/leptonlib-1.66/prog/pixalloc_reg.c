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
 * pixalloc_reg.c
 *
 *   Tests custom pix allocator.
 *
 *   The custom allocator is intended for situations where a number of large
 *   pix will be repeatedly allocated and freed over the lifetime of a program.
 *   If those pix are large, relying on malloc and free can result in
 *   fragmentation, even if there are no small memory leaks in the program.
 *
 *   Here we test the allocator in two situations:
 *     * a small number of relatively large pix
 *     * a large number of very small pix
 *
 *   For the second case, timing shows that the custom allocator does
 *   about as well as (malloc, free), even for thousands of very small pix.
 *   (Turn off logging to get a fair comparison).
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allheaders.h"

static const l_int32 logging = FALSE;

static const l_int32 ncopies = 2;
static const l_int32 nlevels = 4;
static const l_int32 ntimes = 30;


PIXA *GenerateSetOfMargePix(void);
void CopyStoreClean(PIXA *pixas, l_int32 nlevels, l_int32 ncopies);


main(int    argc,
     char **argv)
{
l_int32      i;
BOXA        *boxa;
NUMA        *nas, *nab;
PIX         *pixs;
PIXA        *pixa, *pixas;
static char  mainName[] = "pixalloc_reg";


    /* ----------------- Custom with a few large pix -----------------*/
        /* Set up pms */
    nas = numaCreate(4);  /* small */
    numaAddNumber(nas, 5);   
    numaAddNumber(nas, 4);
    numaAddNumber(nas, 3);
    numaAddNumber(nas, 2);
    setPixMemoryManager(pmsCustomAlloc, pmsCustomDealloc);
    pmsCreate(200000, 400000, nas, "/tmp/junk1.log");

        /* Make the pix and do successive copies and removals of the copies */
    pixas = GenerateSetOfMargePix();
    startTimer();
    for (i = 0; i < ntimes; i++)
        CopyStoreClean(pixas, nlevels, ncopies);
    fprintf(stderr, "Time (big pix; custom) = %7.3f sec\n", stopTimer());

        /* Clean up */
    numaDestroy(&nas);
    pixaDestroy(&pixas);
    pmsDestroy();


    /* ----------------- Standard with a few large pix -----------------*/
    setPixMemoryManager(malloc, free);

        /* Make the pix and do successive copies and removals of the copies */
    startTimer();
    pixas = GenerateSetOfMargePix();
    for (i = 0; i < ntimes; i++)
        CopyStoreClean(pixas, nlevels, ncopies);
    fprintf(stderr, "Time (big pix; standard) = %7.3f sec\n", stopTimer());
    pixaDestroy(&pixas);


    /* ----------------- Custom with many small pix -----------------*/
        /* Set up pms */
    nab = numaCreate(10);
    numaAddNumber(nab, 2000);
    numaAddNumber(nab, 2000);
    numaAddNumber(nab, 2000);
    numaAddNumber(nab, 500);
    numaAddNumber(nab, 100);
    numaAddNumber(nab, 100);
    numaAddNumber(nab, 100);
    setPixMemoryManager(pmsCustomAlloc, pmsCustomDealloc);
    if (logging)   /* use logging == 0 for speed comparison */
        pmsCreate(20, 40, nab, "/tmp/junk2.log");
    else
        pmsCreate(20, 40, nab, NULL);
    pixs = pixRead("feyn.tif");

    startTimer();
    for (i = 0; i < 5; i++) {
        boxa = pixConnComp(pixs, &pixa, 8);
        boxaDestroy(&boxa);
        pixaDestroy(&pixa);
    }

    numaDestroy(&nab);
    pixDestroy(&pixs);
    pmsDestroy();
    fprintf(stderr, "Time (custom) = %7.3f sec\n", stopTimer());


    /* ----------------- Standard with many small pix -----------------*/
    setPixMemoryManager(malloc, free);
    pixs = pixRead("feyn.tif");

    startTimer();
    for (i = 0; i < 5; i++) {
        boxa = pixConnComp(pixs, &pixa, 8);
        boxaDestroy(&boxa);
        pixaDestroy(&pixa);
    }
    pixDestroy(&pixs);
    fprintf(stderr, "Time (standard) = %7.3f sec\n", stopTimer());
}


PIXA *
GenerateSetOfMargePix(void)
{
l_float32  factor;
BOX   *box;
PIX   *pixs, *pixt1, *pixt2, *pixt3, *pixt4;
PIXA  *pixa;

    pixs = pixRead("marge.jpg");
    box = boxCreate(130, 93, 263, 253);
    factor = sqrt(2.0);
    pixt1 = pixClipRectangle(pixs, box, NULL);  /* 266 KB */
    pixt2 = pixScale(pixt1, factor, factor);    /* 532 KB */
    pixt3 = pixScale(pixt2, factor, factor);    /* 1064 KB */
    pixt4 = pixScale(pixt3, factor, factor);    /* 2128 KB */
    pixa = pixaCreate(4);
    pixaAddPix(pixa, pixt1, L_INSERT);
    pixaAddPix(pixa, pixt2, L_INSERT);
    pixaAddPix(pixa, pixt3, L_INSERT);
    pixaAddPix(pixa, pixt4, L_INSERT);
    boxDestroy(&box);
    pixDestroy(&pixs);
    return pixa;
}


void
CopyStoreClean(PIXA    *pixas,
               l_int32  nlevels,
               l_int32  ncopies)
{
l_int32  i, j;
PIX     *pix, *pixt;
PIXA    *pixa;
PIXAA   *paa;

    paa = pixaaCreate(0);
    for (i = 0; i < nlevels ; i++) {
        pixa = pixaCreate(0);
        pixaaAddPixa(paa, pixa, L_INSERT);
        pix = pixaGetPix(pixas, i, L_CLONE);
        for (j = 0; j < ncopies; j++) {
            pixt = pixCopy(NULL, pix);
            pixaAddPix(pixa, pixt, L_INSERT);
        }
        pixDestroy(&pix);
    }
    pixaaDestroy(&paa);

    return;
}

