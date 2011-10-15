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
 *  extrema_reg.c
 *
 *     Tests procedure for locating extrema (minima and maxima)
 *     of a sampled function.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allheaders.h"


main(int    argc,
char **argv)
{
l_int32      i, ival, n;
l_float32    f, val;
GPLOT       *gplot;
NUMA        *na1, *na2, *na3;
static char  mainName[] = "extrema_reg";

    if (argc != 1)
        return ERROR_INT("Syntax: extrema_reg", mainName, 1);

        /* Generate a 1D signal and plot it */
    na1 = numaCreate(500);
    for (i = 0; i < 500; i++) {
        f = 48.3 * sin(0.13 * (l_float32)i);
	f += 63.4 * cos(0.21 * (l_float32)i);
	numaAddNumber(na1, f);
    }
    gplot = gplotCreate("/tmp/junktest", GPLOT_X11, "Extrema test", "x", "y");
    gplotAddPlot(gplot, NULL, na1, GPLOT_LINES, "plot 1");

        /* Find the local min and max and plot them */
    na2 = numaFindExtrema(na1, 38.3);
    n = numaGetCount(na2);
    na3 = numaCreate(n);
    for (i = 0; i < n; i++) {
        numaGetIValue(na2, i, &ival);
        numaGetFValue(na1, ival, &val);
	numaAddNumber(na3, val);
    }
    gplotAddPlot(gplot, na2, na3, GPLOT_POINTS, "plot 2");
    gplotMakeOutput(gplot);
    gplotDestroy(&gplot);
    numaDestroy(&na1);
    numaDestroy(&na2);
    numaDestroy(&na3);

    return 0;
}
