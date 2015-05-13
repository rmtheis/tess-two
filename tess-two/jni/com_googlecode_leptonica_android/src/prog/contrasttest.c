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
 * contrasttest.c
 *
 */

#include <math.h>
#include "allheaders.h"

#define   NPLOTS       5

int main(int    argc,
         char **argv)
{
char        *filein, *fileout;
char         bigbuf[512];
l_int32      iplot;
l_float32    factor;    /* scaled width of atan curve */
l_float32    fact[NPLOTS] = {.2, 0.4, 0.6, 0.8, 1.0};
GPLOT       *gplot;
NUMA        *na, *nax;
PIX         *pixs;
static char  mainName[] = "contrasttest";

    if (argc != 4)
        return ERROR_INT(" Syntax:  contrasttest filein factor fileout",
               mainName, 1);

    filein = argv[1];
    factor = atof(argv[2]);
    fileout = argv[3];

    if ((pixs = pixRead(filein)) == NULL)
        return ERROR_INT("pixs not made", mainName, 1);

    na = numaContrastTRC(factor);
    gplotSimple1(na, GPLOT_X11, "junkroot", "contrast trc");
    numaDestroy(&na);

         /* Plot contrast TRC maps */
    nax = numaMakeSequence(0.0, 1.0, 256);
    gplot = gplotCreate("junkmap", GPLOT_X11,
        "Atan mapping function for contrast enhancement",
        "value in", "value out");
    for (iplot = 0; iplot < NPLOTS; iplot++) {
        na = numaContrastTRC(fact[iplot]);
        sprintf(bigbuf, "factor = %3.1f", fact[iplot]);
        gplotAddPlot(gplot, nax, na, GPLOT_LINES, bigbuf);
        numaDestroy(&na);
    }
    gplotMakeOutput(gplot);

    gplotDestroy(&gplot);
    numaDestroy(&nax);
    return 0;
}

