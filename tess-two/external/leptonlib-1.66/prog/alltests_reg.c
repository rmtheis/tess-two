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
 *  alltests_reg.c
 *
 *    Tests all the reg tests:
 *
 *        alltests_reg  outfile
 *
 *    where
 *        <outfile> == "generate" to make the golden files in /tmp
 *        <outfile> otherwise gives the name of the output file for results
 *
 *    You can also run each test individually without an argument, but
 *    not here.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

static const char *tests[] = {
                              "alphaxform_reg",
                              "colorquant_reg",
                              "convolve_reg",
                              "dewarp_reg",
                              "dwamorph1_reg",
                              "enhance_reg",
                              "findpattern_reg",
                              "fpix_reg",
                              "hardlight_reg",
                              "ioformats_reg",
                              "kernel_reg",
                              "maze_reg",
                              "overlap_reg",
                              "pixa2_reg",
                              "pixserial_reg",
                              "psio_reg",
                              "psioseg_reg",
                              "rankbin_reg",
                              "rankhisto_reg",
                              "rotateorth_reg",
                              "rotate1_reg",
                              "scale_reg",
                              "selio_reg",
                              "shear_reg",
                              "shear2_reg",
                              "skew_reg",
                              "splitcomp_reg",
                              "subpixel_reg",
                              "threshnorm_reg",
                              "warper_reg",
                              "writetext_reg",
                             };

static const char *header = {"\n=======================\n"
                             "Regression Test Results\n"
                             "=======================\n"};

main(int    argc,
     char **argv)
{
char         command[256];
l_int32      i, ntests, dotest;
static char  mainName[] = "alltests_reg";

    if (argc != 2)
        exit(ERROR_INT(" Syntax alltests_reg outfile", mainName, 1));

    ntests = sizeof(tests) / sizeof(char *);

        /* Clear the output file if we're doing the set of reg tests */
    dotest = (strcmp(argv[1], "generate")) ? 1 : 0;
    if (dotest)
        arrayWrite(argv[1], "w", (char *)header, strlen(header));

    for (i = 0; i < ntests; i++) {
        snprintf(command, sizeof(command) - 2, "%s %s", tests[i], argv[1]);
        system(command);
    }

    if (dotest) {
        snprintf(command, sizeof(command) - 2, "cat %s", argv[1]);
        system(command);
    }
    return 0;
}

