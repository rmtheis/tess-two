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
 *  regutils.c
 *
 *       Regression test utilities
 *           l_int32    regTestSetup()
 *           l_int32    regTestCleanup()
 *           l_int32    regTestComparePix()
 *           l_int32    regTestCompareSimilarPix()
 *           l_int32    regTestCheckFile()
 *           l_int32    regTestCompareFiles()
 *           l_int32    regTestWritePixAndCheck()
 *
 *       Static function
 *           char      *getRootNameFromArgv0()
 */

#include <string.h>
#include "allheaders.h"


extern l_int32 NumImageFileFormatExtensions;
extern const char *ImageFileFormatExtensions[];

static char *getRootNameFromArgv0(const char *argv0);


/*--------------------------------------------------------------------*
 *                      Regression test utilities                     *
 *--------------------------------------------------------------------*/
/*!
 *  regTestSetup()
 *
 *      Input:  argc (from invocation; can be either 1 or 2)
 *              argv (to regtest: @argv[1] is one of these:
 *                    "generate", "compare", "display")
 *              &rp (<return> all regression params)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Call this function with the args to the reg test.
 *          There are three cases:
 *          Case 1:
 *              The second arg is "generate".  This will cause
 *              generation of new golden files for the reg test.
 *              The results of the reg test are not recorded, and
 *              the display field is set to FALSE, preventing image display.
 *          Case 2:
 *              The second arg is "compare".  This is the mode in which
 *              you run a regression test (or a set of them), looking
 *              for failures and logging the results to a file.
 *              The output, which includes logging of all reg test
 *              failures plus a SUCCESS or FAILURE summary for each test,
 *              is appended to the file "/tmp/reg_results.txt.  For this
 *              case, as in Case 1, the display field in rp is set to FALSE.
 *          Case 3:
 *              There is either only arg, or the  second arg is "display".
 *              The test will run and files will be written.  Comparisons
 *              with golden files will not be carried out, so the only
 *              notion of success or failure is with tests that do not
 *              involve golden files.  The display field in rp is TRUE,
 *              and this is used by pixDisplayWithTitle().
 *      (2) See regutils.h for examples of usage.
 */
l_int32
regTestSetup(l_int32        argc,
             char         **argv,
             L_REGPARAMS  **prp)
{
char         *testname, *vers;
char          errormsg[64];
L_REGPARAMS  *rp;

    PROCNAME("regTestSetup");

    if (argc != 1 && argc != 2) {
        snprintf(errormsg, sizeof(errormsg),
            "Syntax: %s [generate | compare | [display]]", argv[0]);
        return ERROR_INT(errormsg, procName, 1);
    }

    if ((testname = getRootNameFromArgv0(argv[0])) == NULL)
        return ERROR_INT("invalid root", procName, 1);

    if ((rp = (L_REGPARAMS *)CALLOC(1, sizeof(L_REGPARAMS))) == NULL)
        return ERROR_INT("rp not made", procName, 1);
    *prp = rp;
    rp->testname = testname;
    rp->index = -1;  /* increment before each test */

        /* Initialize to true.  A failure in any test is registered
         * as a failure of the regression test. */
    rp->success = TRUE;

        /* Only open a stream to a temp file for the 'compare' case */
    if (argc == 1 || !strcmp(argv[1], "display")) {
        rp->mode = L_REG_DISPLAY;
        rp->display = TRUE;
    }
    else if (!strcmp(argv[1], "compare")) {
        rp->mode = L_REG_COMPARE;
        rp->tempfile = genTempFilename("/tmp", "regtest_output.txt", 0, 1);
        rp->fp = fopenWriteStream(rp->tempfile, "wb");
        if (rp->fp == NULL) {
            rp->success = FALSE;
            return ERROR_INT("stream not opened for tempfile", procName, 1);
        }
    }
    else if (!strcmp(argv[1], "generate")) {
        rp->mode = L_REG_GENERATE;
        lept_mkdir("golden");
    }
    else {
        FREE(rp);
        snprintf(errormsg, sizeof(errormsg),
            "Syntax: %s [generate | compare | [display]]", argv[0]);
        return ERROR_INT(errormsg, procName, 1);
    }

        /* Print out test name and both the leptonica and
         * image libarary versions */
    fprintf(stderr, "\n################   %s_reg   ###############\n",
            rp->testname);
    vers = getLeptonicaVersion();
    fprintf(stderr, "%s\n", vers);
    FREE(vers);
    vers = getImagelibVersions();
    fprintf(stderr, "%s\n", vers);
    FREE(vers);

    rp->tstart = startTimerNested();
    return 0;
}


/*!
 *  regTestCleanup()
 *
 *      Input:  rp (regression test parameters)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This copies anything written to the temporary file to the
 *          output file /tmp/reg_results.txt.
 */
l_int32
regTestCleanup(L_REGPARAMS  *rp)
{
char    result[512];
char   *results_file;  /* success/failure output in 'compare' mode */
char   *text, *message;
size_t  nbytes;

    PROCNAME("regTestCleanup");

    if (!rp)
        return ERROR_INT("rp not defined", procName, 1);

    fprintf(stderr, "Time: %7.3f sec\n", stopTimerNested(rp->tstart));
    fprintf(stderr, "################################################\n");

        /* If generating golden files or running in display mode, release rp */
    if (!rp->fp) {
        FREE(rp->testname);
        FREE(rp->tempfile);
        FREE(rp);
        return 0;
    }

        /* Compare mode: read back data from temp file */
    fclose(rp->fp);
    text = (char *)l_binaryRead(rp->tempfile, &nbytes);
    FREE(rp->tempfile);
    if (!text) {
        rp->success = FALSE;
        FREE(rp);
        return ERROR_INT("text not returned", procName, 1);
    }

        /* Prepare result message */
    if (rp->success)
        snprintf(result, sizeof(result), "SUCCESS: %s_reg\n", rp->testname);
    else
        snprintf(result, sizeof(result), "FAILURE: %s_reg\n", rp->testname);
    message = stringJoin(text, result);
    FREE(text);
    results_file = genPathname("/tmp", "reg_results.txt");
    fileAppendString(results_file, message);
    FREE(results_file);
    FREE(message);

    FREE(rp->testname);
    FREE(rp);
    return 0;
}


/*!
 *  regTestComparePix()
 *
 *      Input:  rp (regtest parameters)
 *              pix1, pix2 (to be tested for equality)
 *      Return: 0 if OK, 1 on error (a failure in comparison is not an error)
 *
 *  Notes:
 *      (1) This function compares two pix for equality.  If not in compare
 *          mode, on failure it writes to stderr.
 */
l_int32
regTestComparePix(L_REGPARAMS  *rp,
                  PIX          *pix1,
                  PIX          *pix2)
{
l_int32  same;

    PROCNAME("regTestComparePix");

    if (!rp)
        return ERROR_INT("rp not defined", procName, 1);
    if (!pix1 || !pix2) {
        rp->success = FALSE;
        return ERROR_INT("pix1 and pix2 not both defined", procName, 1);
    }

    rp->index++;
    pixEqual(pix1, pix2, &same);

        /* Record on failure */
    if (!same) {
        if (rp->fp) {
            fprintf(rp->fp, "Failure in %s_reg: pix comparison for index %d\n",
                    rp->testname, rp->index);
        }
        fprintf(stderr, "Failure in %s_reg: pix comparison for index %d\n",
                rp->testname, rp->index);
        rp->success = FALSE;
    }
    return 0;
}


/*!
 *  regTestCompareSimilarPix()
 *
 *      Input:  rp (regtest parameters)
 *              pix1, pix2 (to be tested for equality)
 *              mindiff (minimum pixel difference to be counted; > 0)
 *              maxfract (maximum fraction of pixels allowed to have
 *                        diff greater than or equal to mindiff)
 *              printstats (use 1 to print normalized histogram to stderr)
 *      Return: 0 if OK, 1 on error (a failure in similarity comparison
 *              is not an error)
 *
 *  Notes:
 *      (1) This function compares two pix for equality.  If not in compare
 *          mode, on failure it writes to stderr.
 *      (2) To identify two images as 'similar', select @maxfract to be
 *          the upper bound for what you expect.  Typical values might
 *          be @mindiff = 15 and @maxfract = 0.01.
 *      (3) Normally, use @printstats = 0.  In debugging mode, to see
 *          the relation between @mindiff and the minimum value of
 *          @maxfract for success, set this to 1.
 */
l_int32
regTestCompareSimilarPix(L_REGPARAMS  *rp,
                         PIX          *pix1,
                         PIX          *pix2,
                         l_int32       mindiff,
                         l_float32     maxfract,
                         l_int32       printstats)
{
l_int32  w, h, factor, similar;

    PROCNAME("regTestCompareSimilarPix");

    if (!rp)
        return ERROR_INT("rp not defined", procName, 1);
    if (!pix1 || !pix2) {
        rp->success = FALSE;
        return ERROR_INT("pix1 and pix2 not both defined", procName, 1);
    }

    rp->index++;
    pixGetDimensions(pix1, &w, &h, NULL);
    factor = L_MAX(w, h) / 400;
    factor = L_MAX(1, L_MIN(factor, 4));   /* between 1 and 4 */
    pixTestForSimilarity(pix1, pix2, factor, mindiff, maxfract, 0.0,
                         &similar, printstats);

        /* Record on failure */
    if (!similar) {
        if (rp->fp) {
            fprintf(rp->fp,
                    "Failure in %s_reg: pix similarity comp for index %d\n",
                    rp->testname, rp->index);
        }
        fprintf(stderr, "Failure in %s_reg: pix similarity comp for index %d\n",
                rp->testname, rp->index);
        rp->success = FALSE;
    }
    return 0;
}


/*!
 *  regTestCheckFile()
 *
 *      Input:  rp (regtest parameters)
 *              localname (name of output file from reg test)
 *      Return: 0 if OK, 1 on error (a failure in comparison is not an error)
 *
 *  Notes:
 *      (1) This function does one of three things, depending on the mode:
 *           * "generate": makes a "golden" file as a copy @localname.
 *           * "compare": compares @localname contents with the golden file
 *           * "display": makes the @localname file but does no comparison
 *      (2) The canonical format of the golden filenames is:
 *            /tmp/golden/<root of main name>_golden.<index>.<ext of localname>
 *          e.g.,
 *             /tmp/golden/maze_golden.0.png
 *          It is important to add an extension to the local name, because
 *          the extension is added to the name of the golden file.
 */
l_int32
regTestCheckFile(L_REGPARAMS  *rp,
                 const char   *localname)
{
char    *ext;
char     namebuf[256];
l_int32  ret, same;

    PROCNAME("regTestCheckFile");

    if (!rp)
        return ERROR_INT("rp not defined", procName, 1);
    if (!localname) {
        rp->success = FALSE;
        return ERROR_INT("local name not defined", procName, 1);
    }
    if (rp->mode != L_REG_GENERATE && rp->mode != L_REG_COMPARE &&
        rp->mode != L_REG_DISPLAY) {
        rp->success = FALSE;
        return ERROR_INT("invalid mode", procName, 1);
    }
    rp->index++;

    if (rp->mode == L_REG_DISPLAY) return 0;

        /* Generate the golden file name; used in 'generate' and 'compare' */
    splitPathAtExtension(localname, NULL, &ext);
    snprintf(namebuf, sizeof(namebuf), "/tmp/golden/%s_golden.%d%s",
             rp->testname, rp->index, ext);
    FREE(ext);

    if (rp->mode == L_REG_GENERATE) {
            /* Save the file as a golden file */
/*        fprintf(stderr, "%d: %s\n", rp->index, namebuf);  */
        ret = fileCopy(localname, namebuf);
        if (!ret)
            fprintf(stderr, "Copy: %s to %s\n", localname, namebuf);
        return ret;
    }

        /* Compare mode: test and record on failure */
    filesAreIdentical(localname, namebuf, &same);
    if (!same) {
        fprintf(rp->fp, "Failure in %s_reg, index %d: comparing %s with %s\n",
                rp->testname, rp->index, localname, namebuf);
        fprintf(stderr, "Failure in %s_reg, index %d: comparing %s with %s\n",
                rp->testname, rp->index, localname, namebuf);
        rp->success = FALSE;
    }

    return 0;
}


/*!
 *  regTestCompareFiles()
 *
 *      Input:  rp (regtest parameters)
 *              index1 (of one output file from reg test)
 *              index2 (of another output file from reg test)
 *      Return: 0 if OK, 1 on error (a failure in comparison is not an error)
 *
 *  Notes:
 *      (1) This only does something in "compare" mode.
 *      (2) The canonical format of the golden filenames is:
 *            /tmp/golden/<root of main name>_golden.<index>.<ext of localname>
 *          e.g.,
 *            /tmp/golden/maze_golden.0.png
 */
l_int32
regTestCompareFiles(L_REGPARAMS  *rp,
                    l_int32       index1,
                    l_int32       index2)
{
char    *name1, *name2;
char     namebuf[256];
l_int32  same;
SARRAY  *sa;

    PROCNAME("regTestCompareFiles");

    if (!rp)
        return ERROR_INT("rp not defined", procName, 1);
    if (index1 < 0 || index2 < 0) {
        rp->success = FALSE;
        return ERROR_INT("index1 and/or index2 is negative", procName, 1);
    }
    if (index1 == index2) {
        rp->success = FALSE;
        return ERROR_INT("index1 must differ from index2", procName, 1);
    }

    rp->index++;
    if (rp->mode != L_REG_COMPARE) return 0;

        /* Generate the golden file names */
    snprintf(namebuf, sizeof(namebuf), "%s_golden.%d.", rp->testname, index1);
    sa = getSortedPathnamesInDirectory("/tmp/golden", namebuf, 0, 0);
    if (sarrayGetCount(sa) != 1) {
        sarrayDestroy(&sa);
        rp->success = FALSE;
        L_ERROR_STRING("golden file %s not found", procName, namebuf);
        return 1;
    }
    name1 = sarrayGetString(sa, 0, L_COPY);
    sarrayDestroy(&sa);

    snprintf(namebuf, sizeof(namebuf), "%s_golden.%d.", rp->testname, index2);
    sa = getSortedPathnamesInDirectory("/tmp/golden", namebuf, 0, 0);
    if (sarrayGetCount(sa) != 1) {
        sarrayDestroy(&sa);
        rp->success = FALSE;
        FREE(name1);
        L_ERROR_STRING("golden file %s not found", procName, namebuf);
        return 1;
    }
    name2 = sarrayGetString(sa, 0, L_COPY);
    sarrayDestroy(&sa);

        /* Test and record on failure */
    filesAreIdentical(name1, name2, &same);
    if (!same) {
        fprintf(rp->fp,
                "Failure in %s_reg, index %d: comparing %s with %s\n",
                rp->testname, rp->index, name1, name2);
        fprintf(stderr,
                "Failure in %s_reg, index %d: comparing %s with %s\n",
                rp->testname, rp->index, name1, name2);
        rp->success = FALSE;
    }

    FREE(name1);
    FREE(name2);
    return 0;
}


/*!
 *  regTestWritePixAndCheck()
 *
 *      Input:  rp (regtest parameters)
 *              pix (to be written)
 *              format (of output pix)
 *      Return: 0 if OK, 1 on error (a failure in comparison is not an error)
 *
 *  Notes:
 *      (1) This function makes it easy to write the pix in a numbered
 *          sequence of files, and either to:
 *             (a) write the golden file ("generate" arg to regression test)
 *             (b) make a local file and "compare" with the golden file
 *             (c) make a local file and "display" the results
 *      (3) The canonical format of the local filename is:
 *            /tmp/<root of main name>.<count>.<format extension string>
 *          e.g., for scale_reg,
 *            /tmp/scale.0.png
 */
l_int32
regTestWritePixAndCheck(L_REGPARAMS  *rp,
                        PIX          *pix,
                        l_int32       format)
{
char   namebuf[256];

    PROCNAME("regTestWritePixAndCheck");

    if (!rp)
        return ERROR_INT("rp not defined", procName, 1);
    if (!pix) {
        rp->success = FALSE;
        return ERROR_INT("pix not defined", procName, 1);
    }
    if (format < 0 || format >= NumImageFileFormatExtensions) {
        rp->success = FALSE;
        return ERROR_INT("invalid format", procName, 1);
    }

        /* Generate the local file name */
    snprintf(namebuf, sizeof(namebuf), "/tmp/%s.%d.%s", rp->testname,
             rp->index + 1, ImageFileFormatExtensions[format]);

        /* Write the local file */
    pixWrite(namebuf, pix, format);

        /* Either write the golden file ("generate") or check the
           local file against an existing golden file ("compare") */
    regTestCheckFile(rp, namebuf);

    return 0;
}


/*!
 *  getRootNameFromArgv0()
 *
 *      Input:  argv0
 *      Return: root name (without the '_reg'), or null on error
 *
 *  Notes:
 *      (1) For example, from psioseg_reg, we want to extract
 *          just 'psioseg' as the root.
 *      (2) In unix with autotools, the executable is not X,
 *          but ./.libs/lt-X.   So in addition to stripping out the
 *          last 4 characters of the tail, we have to check for
 *          the '-' and strip out the "lt-" prefix if we find it.
 */
static char *
getRootNameFromArgv0(const char  *argv0)
{
l_int32  len;
char    *root;

    PROCNAME("getRootNameFromArgv0");

    splitPathAtDirectory(argv0, NULL, &root);
    if ((len = strlen(root)) <= 4) {
        FREE(root);
        return (char *)ERROR_PTR("invalid argv0; too small", procName, NULL);
    }

#ifndef _WIN32
    {
        char    *newroot;
        l_int32  loc;
        if (stringFindSubstr(root, "-", &loc)) {
            newroot = stringNew(root + loc + 1);  /* strip out "lt-" */
            FREE(root);
            root = newroot;
            len = strlen(root);
        }
    }
#else
    if (strstr(root, ".exe") != NULL)
        len -= 4;
#endif  /* ! _WIN32 */

    root[len - 4] = '\0';  /* remove the suffix */
    return root;
}


