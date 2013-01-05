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
 *        alltests_reg  command
 *
 *    where
 *        <command> == "generate" to make the golden files in /tmp/golden
 *        <command> == "compare" to make local files and compare with
 *                     the golden files
 *        <command> == "display" to make local files and display
 *
 *    You can also run each test individually with any one of these
 *    arguments.
 */

#include <string.h>
#include "allheaders.h"

static const char *tests[] = {
                              "alphaops_reg",
                              "alphaxform_reg",
                              "binarize_reg",
                              "coloring_reg",
                              "colormask_reg",
                              "colorquant_reg",
                              "compare_reg",
                              "convolve_reg",
                              "dewarp_reg",
                         /*   "distance_reg", */
                              "dna_reg",
                              "dwamorph1_reg",
                              "enhance_reg",
                              "findpattern_reg",
                              "fpix_reg",
                              "gifio_reg",
                              "graymorph2_reg",
                              "hardlight_reg",
                              "ioformats_reg",
                              "kernel_reg",
                              "maze_reg",
                              "overlap_reg",
                              "pdfseg_reg",
                              "pixa2_reg",
                              "pixserial_reg",
                              "pngio_reg",
                              "projection_reg",
                              "psio_reg",
                              "psioseg_reg",
                              "rankbin_reg",
                              "rankhisto_reg",
                              "rasteropip_reg",
                              "rotateorth_reg",
                              "rotate1_reg",
                              "rotate2_reg",
                              "scale_reg",
                              "seedspread_reg",
                              "selio_reg",
                              "shear_reg",
                              "shear2_reg",
                              "skew_reg",
                              "splitcomp_reg",
                              "subpixel_reg",
                              "threshnorm_reg",
                              "translate_reg",
                              "warper_reg",
                              "writetext_reg",
                             };

static const char *header = {"\n=======================\n"
                             "Regression Test Results\n"
                             "======================="};

main(int    argc,
     char **argv)
{
char        *str, *results_file;
char         command[256], buf[256];
l_int32      i, ntests, dotest, nfail, ret, start, stop;
SARRAY      *sa;
L_TIMER      timer;
static char  mainName[] = "alltests_reg";


    if (argc != 2)
        return ERROR_INT(" Syntax alltests_reg [generate | compare | display]",
                         mainName, 1);

    l_getCurrentTime(&start, NULL);
    ntests = sizeof(tests) / sizeof(char *);
    fprintf(stderr, "Running alltests_reg:\n"
            "This currently tests %d of the 97 Regression Test\n"
            "programs in the /prog directory.\n", ntests);

        /* Clear the output file if we're doing the set of reg tests */
    dotest = strcmp(argv[1], "compare") ? 0 : 1;
    if (dotest) {
        results_file = genPathname("/tmp", "reg_results.txt");
        sa = sarrayCreate(3);
        sarrayAddString(sa, (char *)header, L_COPY);
        sarrayAddString(sa, getLeptonicaVersion(), L_INSERT);
        sarrayAddString(sa, getImagelibVersions(), L_INSERT);
        str = sarrayToString(sa, 1);
        sarrayDestroy(&sa);
        l_binaryWrite("/tmp/reg_results.txt", "w", str, strlen(str));
        lept_free(str);
    }

    nfail = 0;
    for (i = 0; i < ntests; i++) {
        snprintf(command, sizeof(command) - 2, "./%s %s", tests[i], argv[1]);
        ret = system(command);
        if (ret) {
            snprintf(buf, sizeof(buf), "Failed to complete %s\n", tests[i]);
            if (dotest) {
                l_binaryWrite("/tmp/reg_results.txt", "a", buf, strlen(buf));
                nfail++;
            }
            else
                fprintf(stderr, "%s", buf);
        }
    }

    if (dotest) {
#ifndef _WIN32
        snprintf(command, sizeof(command) - 2, "cat %s", results_file);
#else
        snprintf(command, sizeof(command) - 2, "type \"%s\"", results_file);
#endif  /* !_WIN32 */
        lept_free(results_file);
        ret = system(command);
        fprintf(stderr, "Success in %d of %d *_reg programs (output matches"
                " the \"golden\" files)\n", ntests - nfail, ntests);
    }

    l_getCurrentTime(&stop, NULL);
    fprintf(stderr, "Time for all regression tests: %d sec\n", stop - start);
    return 0;
}
