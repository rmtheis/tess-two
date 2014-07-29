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
 *  alltests_reg.c
 *
 *    Tests all the reg tests:
 *
 *        alltests_reg command
 *
 *    where
 *        <command> == "generate" to make the golden files in /tmp/golden
 *        <command> == "compare" to make local files and compare with
 *                     the golden files
 *        <command> == "display" to make local files and display
 *
 *    You can also run each test individually with any one of these
 *    arguments.  Warning: if you run this with "display", a very
 *    large number of images will be displayed on the screen.
 */

#include <string.h>
#include "allheaders.h"

static const char *tests[] = {
                              "alphaops_reg",
                              "alphaxform_reg",
                              "bilateral2_reg",
                              "binarize_reg",
                              "blackwhite_reg",
                              "blend3_reg",
                              "blend4_reg",
                              "colorcontent_reg",
                              "coloring_reg",
                              "colormask_reg",
                              "colorquant_reg",
                              "colorspace_reg",
                              "compare_reg",
                              "convolve_reg",
                              "dewarp_reg",
                         /*   "distance_reg", */
                              "dna_reg",
                              "dwamorph1_reg",
                              "enhance_reg",
                              "files_reg",
                              "findcorners_reg",
                              "findpattern_reg",
                              "fpix1_reg",
                              "fpix2_reg",
                         /*   "gifio_reg",  */
                              "graymorph2_reg",
                              "hardlight_reg",
                              "insert_reg",
                              "ioformats_reg",
                              "jpegio_reg",
                              "kernel_reg",
                              "label_reg",
                              "maze_reg",
                              "multitype_reg",
                              "nearline_reg",
                              "newspaper_reg",
                              "overlap_reg",
                              "paint_reg",
                              "paintmask_reg",
                              "pdfseg_reg",
                              "pixa2_reg",
                              "pixserial_reg",
                              "pngio_reg",
                              "projection_reg",
                              "psio_reg",
                              "psioseg_reg",
                              "pta_reg",
                              "rankbin_reg",
                              "rankhisto_reg",
                              "rasteropip_reg",
                              "rotateorth_reg",
                              "rotate1_reg",
                              "rotate2_reg",
                              "scale_reg",
                              "seedspread_reg",
                              "selio_reg",
                              "shear1_reg",
                              "shear2_reg",
                              "skew_reg",
                              "splitcomp_reg",
                              "subpixel_reg",
                              "threshnorm_reg",
                              "translate_reg",
                              "warper_reg",
                              "writetext_reg",
                              "xformbox_reg",
                             };

static const char *header = {"\n=======================\n"
                             "Regression Test Results\n"
                             "======================="};

int main(int    argc,
         char **argv)
{
char        *str, *results_file;
char         command[256], buf[256];
l_int32      i, ntests, dotest, nfail, ret, start, stop;
SARRAY      *sa;
static char  mainName[] = "alltests_reg";

    if (argc != 2)
        return ERROR_INT(" Syntax alltests_reg [generate | compare | display]",
                         mainName, 1);

    l_getCurrentTime(&start, NULL);
    ntests = sizeof(tests) / sizeof(char *);
    fprintf(stderr, "Running alltests_reg:\n"
            "This currently tests %d of the 120 Regression Test\n"
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
#ifndef  _WIN32
        snprintf(command, sizeof(command) - 2, "./%s %s", tests[i], argv[1]);
#else  /* windows interprets '/' as a commandline flag */
        snprintf(command, sizeof(command) - 2, "%s %s", tests[i], argv[1]);
#endif  /* ! _WIN32 */
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
