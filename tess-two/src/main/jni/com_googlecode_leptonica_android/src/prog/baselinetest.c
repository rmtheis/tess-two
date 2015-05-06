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
 * baselinetest.c
 *
 *   - e.g., use keystone.png as the input image
 *   - to get plots of baseline locations and other derived
 *     parameters, set DEBUG_PLOT to 1 in baseline.c
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
char        *filein, *fileout;
NUMA        *na;
PIX         *pixs, *pixd;
PTA         *pta;
static char  mainName[] = "baselinetest";

    if (argc != 3)
	return ERROR_INT(" Syntax:  baselinetest filein fileout", mainName, 1);

    filein = argv[1];
    fileout = argv[2];
    pixs = pixRead(filein);

#if 1
        /* Test function for deskewing using projective transform
	 * on linear approximation for local skew angle */
    pixd = pixDeskewLocal(pixs, 10, 0, 0, 0.0, 0.0, 0.0);
    pixWrite(fileout, pixd, IFF_TIFF_G4);

        /* test baseline finder */
    na = pixFindBaselines(pixd, &pta, 1);
/*    ptaWriteStream(stderr, pta, 1); */
    pixDestroy(&pixd);
    numaDestroy(&na);
    ptaDestroy(&pta);
#endif

#if 0
        /* Test function for finding local skew angles */
    na = pixGetLocalSkewAngles(pixs, 10, 0, 0, 0.0, 0.0, 0.0, NULL, NULL);
    numaDestroy(&na);
#endif

    pixDestroy(&pixs);
    return 0;
}



