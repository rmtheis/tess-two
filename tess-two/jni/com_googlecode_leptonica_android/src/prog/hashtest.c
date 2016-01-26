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
 *  hashtest.c
 *
 *  Tests hashing functions for strings and points, and the use of them with:
 *    *  sets (underlying rbtree implementation for sorting)
 *    *  hash maps (underlying dnaHash implementation for accessing)
 *
 *  For sets, it's important to use good 64-bit hashes to ensure that
 *  collisions are very rare.  With solid randomization, you expect
 *  that a collision is likely with 2^32 or more hashed entities.
 *  The probability of a collision goes as n^2, so with 10M entities,
 *  the collision probabililty is about 10^-5.
 *
 *  For hash maps, the 64-bit hash only needs to randomize the lower
 *  order bits corresponding to the prime number used for assigning
 *  to buckets.  To the extent that those bits are not randomized, the
 *  calculation will run slower but the result will be exact.
 *
 *  Also tests similar functions directly (without hashing the number)
 *  for numa.  (To be replaced by doing it with dna.)
 */

#include "allheaders.h"

static SARRAY *BuildShortStrings(l_int32 nchars, l_int32 add_dups);
static PTA *BuildPointSet(l_int32 w, l_int32 h, l_int32 add_dups);


l_int32 main(int    argc,
             char **argv)
{
L_ASET     *set;
L_DNA      *da1, *da2, *da3, *da4, *da5, *da6, *dav, *dac, *da7;
L_DNAHASH  *dahash;
NUMA       *na1, *na2, *na3, *na4, *na5, *na6, *na7, *na8, *nav, *nac;
PTA        *pta1, *pta2, *pta3;
SARRAY     *sa1, *sa2, *sa3, *sa4;

#if 1
        /* Test good string hashing with aset */
    fprintf(stderr, "Set results with string hashing:\n");
    sa1 = BuildShortStrings(3, 0);
    sa2 = BuildShortStrings(3, 1);
    fprintf(stderr, "  size with unique strings: %d\n", sarrayGetCount(sa1));
    fprintf(stderr, "  size with dups: %d\n", sarrayGetCount(sa2));
    startTimer();
    set = l_asetCreateFromSarray(sa2);
    fprintf(stderr, "  time to make set: %5.3f sec\n", stopTimer());
    fprintf(stderr, "  size of set without dups: %d\n", l_asetSize(set));
    l_asetDestroy(&set);
    startTimer();
    sa3 = sarrayRemoveDupsByAset(sa2);
    fprintf(stderr, "  time to remove dups: %5.3f sec\n", stopTimer());
    fprintf(stderr, "  size without dups = %d\n", sarrayGetCount(sa3));
    startTimer();
    sa4 = sarrayIntersectionByAset(sa1, sa2);
    fprintf(stderr, "  time to intersect: %5.3f sec\n", stopTimer());
    fprintf(stderr, "  intersection size = %d\n", sarrayGetCount(sa4));
    sarrayDestroy(&sa3);
    sarrayDestroy(&sa4);

        /* Test sarray set operations with hash map (dnaHash).
         * We use a weak hash; collisions are OK if they're not too
         * spatially localized. */
    fprintf(stderr, "\nHash map results for sarray:\n");
    fprintf(stderr, "  size with unique strings: %d\n", sarrayGetCount(sa1));
    fprintf(stderr, "  size with dups: %d\n", sarrayGetCount(sa2));
    startTimer();
    dahash = l_dnaHashCreateFromSarray(sa2);
    fprintf(stderr, "  time to make hashmap: %5.3f sec\n", stopTimer());
    fprintf(stderr, "  entries in hashmap with dups: %d\n",
            l_dnaHashGetTotalCount(dahash));
    l_dnaHashDestroy(&dahash);
    startTimer();
    sarrayRemoveDupsByHash(sa2, &sa3, NULL);
    fprintf(stderr, "  time to remove dups: %5.3f sec\n", stopTimer());
    fprintf(stderr, "  size without dups = %d\n", sarrayGetCount(sa3));
    startTimer();
    sa4 = sarrayIntersectionByHash(sa1, sa2);
    fprintf(stderr, "  time to intersect: %5.3f sec\n", stopTimer());
    fprintf(stderr, "  intersection size = %d\n", sarrayGetCount(sa4));
    sarrayDestroy(&sa3);
    sarrayDestroy(&sa4);
    sarrayDestroy(&sa1);
    sarrayDestroy(&sa2);
#endif

#if 1
        /* Test good pt hashing with aset.
         * Enter all points up to (1999, 1999), and include
         * 800,000 duplicates in pta1.  Note: there are no hash
         * collisions up to 100M points (up to 10000 x 10000). */
    pta1 = BuildPointSet(1000, 1000, 0);
    pta2 = BuildPointSet(1000, 1000, 1);
    fprintf(stderr, "\nSet results for pta:\n");
    fprintf(stderr, "  pta1 size with unique points: %d\n", ptaGetCount(pta1));
    fprintf(stderr, "  pta2 size with dups: %d\n", ptaGetCount(pta2));
    startTimer();
    pta3 = ptaRemoveDupsByAset(pta2);
    fprintf(stderr, "  Time to remove dups: %5.3f sec\n", stopTimer());
    fprintf(stderr, "  size without dups = %d\n", ptaGetCount(pta3));
    ptaDestroy(&pta3);

    startTimer();
    pta3 = ptaIntersectionByAset(pta1, pta2);
    fprintf(stderr, "  Time to intersect: %5.3f sec\n", stopTimer());
    fprintf(stderr, "  intersection size = %d\n", ptaGetCount(pta3));
    ptaDestroy(&pta1);
    ptaDestroy(&pta2);
    ptaDestroy(&pta3);
#endif

#if 1
        /* Test pta set operations with hash map (dnaHash).
         * Collisions are OK if they're not too spatially localized */
    pta1 = BuildPointSet(1000, 1000, 0);
    pta2 = BuildPointSet(1000, 1000, 1);
    fprintf(stderr, "\nHash map results for pta:\n");
    fprintf(stderr, "  pta1 size with unique points: %d\n", ptaGetCount(pta1));
    fprintf(stderr, "  pta2 size with dups: %d\n", ptaGetCount(pta2));
    startTimer();
    ptaRemoveDupsByHash(pta2, &pta3, NULL);
    fprintf(stderr, "  Time to remove dups: %5.3f sec\n", stopTimer());
    fprintf(stderr, "  size without dups = %d\n", ptaGetCount(pta3));
    ptaDestroy(&pta3);

    startTimer();
    pta3 = ptaIntersectionByHash(pta1, pta2);
    fprintf(stderr, "  Time to intersect: %5.3f sec\n", stopTimer());
    fprintf(stderr, "  intersection size = %d\n", ptaGetCount(pta3));
    ptaDestroy(&pta1);
    ptaDestroy(&pta2);
    ptaDestroy(&pta3);

#endif

        /* Test dna set and histo operations with hash map (dnaHash) */
#if 1
    fprintf(stderr, "\nHash map results for dna:\n");
    da1 = l_dnaMakeSequence(0.0, 0.125, 8000);
    da2 = l_dnaMakeSequence(300.0, 0.125, 8000);
    da3 = l_dnaMakeSequence(600.0, 0.125, 8000);
    da4 = l_dnaMakeSequence(900.0, 0.125, 8000);
    da5 = l_dnaMakeSequence(1200.0, 0.125, 8000);
    l_dnaJoin(da1, da2, 0, -1);
    l_dnaJoin(da1, da3, 0, -1);
    l_dnaJoin(da1, da4, 0, -1);
    l_dnaJoin(da1, da5, 0, -1);
    l_dnaRemoveDupsByHash(da1, &da6, &dahash);
    l_dnaHashDestroy(&dahash);
    fprintf(stderr, "  dna size with dups = %d\n", l_dnaGetCount(da1));
    fprintf(stderr, "  dna size of unique numbers = %d\n", l_dnaGetCount(da6));
    l_dnaMakeHistoByHash(da1, &dahash, &dav, &dac);
    nav = l_dnaConvertToNuma(dav);
    nac = l_dnaConvertToNuma(dac);
    fprintf(stderr, "  dna number of histo points = %d\n", l_dnaGetCount(dac));
    gplotSimpleXY1(nav, nac, GPLOT_IMPULSES, GPLOT_X11, "/tmp/histo", "Histo");
    da7 = l_dnaIntersectionByHash(da2, da3);
    fprintf(stderr, "  dna number of points: da2 = %d, da3 = %d\n",
            l_dnaGetCount(da2), l_dnaGetCount(da3));
    fprintf(stderr, "  dna number of da2/da3 intersection points = %d\n",
            l_dnaGetCount(da7));
    l_dnaDestroy(&da1);
    l_dnaDestroy(&da2);
    l_dnaDestroy(&da3);
    l_dnaDestroy(&da4);
    l_dnaDestroy(&da5);
    l_dnaDestroy(&da6);
    l_dnaDestroy(&da7);
    l_dnaDestroy(&dac);
    l_dnaDestroy(&dav);
    l_dnaHashDestroy(&dahash);
    numaDestroy(&nav);
    numaDestroy(&nac);
#endif

#if 0
    na1 = numaMakeSequence(0, 3, 10000);
    na2 = numaMakeSequence(0, 5, 10000);
    na3 = numaMakeSequence(0, 7, 10000);
    numaJoin(na1, na2, 0, -1);
    numaJoin(na1, na3, 0, -1);

    fprintf(stderr, "\nNuma results using set:\n");
    fprintf(stderr, "  na1 count: %d\n", numaGetCount(na1));
    set = l_asetCreateFromNuma(na1);
    fprintf(stderr, "  na1 set size: %d\n\n", l_asetSize(set));
    l_asetDestroy(&set);

    na4 = numaUnionByAset(na2, na3);
    fprintf(stderr, "  na4 count: %d\n", numaGetCount(na4));
    set = l_asetCreateFromNuma(na4);
    fprintf(stderr, "  na4 set size: %d\n\n", l_asetSize(set));
    l_asetDestroy(&set);

    na5 = numaIntersectionByAset(na1, na2);
    fprintf(stderr, "  na5 count: %d\n", numaGetCount(na5));
    set = l_asetCreateFromNuma(na5);
    fprintf(stderr, "  na5 set size: %d\n\n", l_asetSize(set));
    l_asetDestroy(&set);

    na6 = numaMakeSequence(100000, 11, 5000);
    numaJoin(na6, na1, 0, -1);
    fprintf(stderr, "  na6 count: %d\n", numaGetCount(na6));
    set = l_asetCreateFromNuma(na6);
    fprintf(stderr, "  na6 set size: %d\n\n", l_asetSize(set));
    l_asetDestroy(&set);

    na7 = numaIntersectionByAset(na6, na3);
    fprintf(stderr, "  na7 count: %d\n", numaGetCount(na7));
    set = l_asetCreateFromNuma(na7);
    fprintf(stderr, "  na7 set size: %d\n\n", l_asetSize(set));
    l_asetDestroy(&set);

    na8 = numaRemoveDups(na1);
    fprintf(stderr, "  na8 count: %d\n\n", numaGetCount(na8));

    numaDestroy(&na1);
    numaDestroy(&na2);
    numaDestroy(&na3);
    numaDestroy(&na4);
    numaDestroy(&na5);
    numaDestroy(&na6);
    numaDestroy(&na7);
    numaDestroy(&na8);
#endif

    return 0;
}


    /* Build all possible strings, up to a max of 5 roman alphabet characters */
static SARRAY *
BuildShortStrings(l_int32  nchars,  /* 3, 4 or 5 */
                  l_int32  add_dups)
{
char      buf[64];
l_int32   i, j, k, l, m;
l_uint64  hash;
SARRAY   *sa;

    sa = sarrayCreate(1000);
    for (i = 0; i < 26; i++) {
        sprintf(buf, "%c", i + 0x61);
        sarrayAddString(sa, buf, L_COPY);
        for (j = 0; j < 26; j++) {
            sprintf(buf, "%c%c", i + 0x61, j + 0x61);
            sarrayAddString(sa, buf, L_COPY);
            for (k = 0; k < 26; k++) {
                sprintf(buf, "%c%c%c", i + 0x61, j + 0x61, k + 0x61);
                sarrayAddString(sa, buf, L_COPY);
                if (add_dups && k < 4)  /* add redundant strings */
                    sarrayAddString(sa, buf, L_COPY);
                if (nchars > 3) {
                    for (l = 0; l < 26; l++) {
                        sprintf(buf, "%c%c%c%c", i + 0x61, j + 0x61,
                                k + 0x61, l + 0x61);
                        sarrayAddString(sa, buf, L_COPY);
                        if (add_dups && l < 4)  /* add redundant strings */
                            sarrayAddString(sa, buf, L_COPY);
                        if (nchars > 4) {
                            for (m = 0; m < 26; m++) {
                                sprintf(buf, "%c%c%c%c%c", i + 0x61, j + 0x61,
                                        k + 0x61, l + 0x61, m + 0x61);
                                sarrayAddString(sa, buf, L_COPY);
                                if (!add_dups && i == 17 && j == 12 &&
                                    k == 4 && l == 21) {
                                    l_hashStringToUint64(buf, &hash);
                                    fprintf(stderr, "  %llx\n", hash);
                                }
                                if (add_dups && m < 4)  /* add redundant */
                                    sarrayAddString(sa, buf, L_COPY);
                            }
                        }
                    }
                }
            }
        }
    }

    return sa;
}

static PTA *
BuildPointSet(l_int32  w, l_int32 h, l_int32 add_dups)
{
l_int32  i, j;
PTA     *pta;

    pta = ptaCreate(w * h);
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++)
            ptaAddPt(pta, j, i);
        if (add_dups) { /* extra (0.2 * w * h) points */
            for (j = 0.4 * w; j < 0.6 * w; j++)
                ptaAddPt(pta, j, i);
        }
    }

    return pta;
}

