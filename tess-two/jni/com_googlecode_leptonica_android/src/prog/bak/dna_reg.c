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
 * dna_reg.c
 *
 *   Tests basic functioning of L_Dna (number array of doubles)
 */

#include <math.h>
#ifndef  _WIN32
#include <unistd.h>
#else
#include <windows.h>   /* for Sleep() */
#endif  /* _WIN32 */
#include "allheaders.h"


main(int    argc,
     char **argv)
{
l_int32       i, nbins, ival;
l_float64     pi, angle, val, sum;
L_DNA        *da1, *da2, *da3, *da4, *da5;
L_DNAA       *daa1, *daa2;
GPLOT        *gplot;
NUMA         *na, *nahisto, *nax;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

    pi = 3.1415926535;
    da1 = l_dnaCreate(50);
    for (i = 0; i < 5000; i++) {
        angle = 0.02293 * i * pi;
        val = 999. * sin(angle);
        l_dnaAddNumber(da1, val);
    }

        /* Conversion to Numa; I/O for Dna */
    na = l_dnaConvertToNuma(da1);
    da2 = numaConvertToDna(na);
    l_dnaWrite("/tmp/dna1.da", da1);
    l_dnaWrite("/tmp/dna2.da", da2);
    da3 = l_dnaRead("/tmp/dna2.da");
    l_dnaWrite("/tmp/dna3.da", da3);
    regTestCheckFile(rp, "/tmp/dna1.da");  /* 0 */
    regTestCheckFile(rp, "/tmp/dna2.da");  /* 1 */
    regTestCheckFile(rp, "/tmp/dna3.da");  /* 2 */
    regTestCompareFiles(rp, 1, 2);  /* 3 */

        /* I/O for Dnaa */
    daa1 = l_dnaaCreate(3);
    l_dnaaAddDna(daa1, da1, L_INSERT);
    l_dnaaAddDna(daa1, da2, L_INSERT);
    l_dnaaAddDna(daa1, da3, L_INSERT);
    l_dnaaWrite("/tmp/dnaa1.daa", daa1);
    daa2 = l_dnaaRead("/tmp/dnaa1.daa");
    l_dnaaWrite("/tmp/dnaa2.daa", daa2);
    regTestCheckFile(rp, "/tmp/dnaa1.daa");  /* 4 */
    regTestCheckFile(rp, "/tmp/dnaa2.daa");  /* 5 */
    regTestCompareFiles(rp, 4, 5);  /* 6 */
    l_dnaaDestroy(&daa1);
    l_dnaaDestroy(&daa2);

        /* Just for fun -- is the numa ok? */
    nahisto = numaMakeHistogramClipped(na, 12, 2000);
    nbins = numaGetCount(nahisto);
    nax = numaMakeSequence(0, 1, nbins);
    gplot = gplotCreate("/tmp/historoot", GPLOT_PNG, "Histo example",
                        "i", "histo[i]");
    gplotAddPlot(gplot, nax, nahisto, GPLOT_LINES, "sine");
    gplotMakeOutput(gplot);
#ifndef  _WIN32
    sleep(1);
#else
    Sleep(1000);
#endif  /* _WIN32 */
    regTestCheckFile(rp, "/tmp/historoot.png");  /* 7 */
    gplotDestroy(&gplot);
    numaDestroy(&na);
    numaDestroy(&nax);
    numaDestroy(&nahisto);

        /* Handling precision of int32 in double */
    da4 = l_dnaCreate(25);
    for (i = 0; i < 1000; i++)
        l_dnaAddNumber(da4, 1928374 * i);
    l_dnaWrite("/tmp/dna4.da", da4);
    da5 = l_dnaRead("/tmp/dna4.da");
    sum = 0;
    for (i = 0; i < 1000; i++) {
        l_dnaGetIValue(da5, i, &ival);
        sum += L_ABS(ival - i * 1928374);  /* we better be adding 0 each time */
    }
    regTestCompareValues(rp, sum, 0.0, 0.0);  /* 8 */
    l_dnaDestroy(&da4);
    l_dnaDestroy(&da5);

    return regTestCleanup(rp);
}
