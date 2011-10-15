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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
 *      Input:  argc (to regtest: either 1 or 2)
 *              argv (to regtest: if @argc == 2, @argv[1] is either
 *                    "generate" or a log file name)
 *              &fp (<return> stream for writing to a temporary file,
 *                   or null for the "generate" case)
 *              &display (<return> TRUE or FALSE for image display)
 *              &success (<return> initialize to TRUE)
 *              &rp (<optional return> all regression params for either
 *                   passthrough to subroutines or for  automated
 *                   "write and check")
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Call this function with the args to the reg test.
 *          There are exactly three cases:
 *          Case 1:
 *              The second arg is "generate".  This will cause
 *              generation of new golden files for the reg test.
 *              The results of the reg test are not recorded.  Also,
 *              the returned @display == FALSE (prevents image display).
 *          Case 2:
 *              The second arg is the name of the output log file
 *              into which all reg test failures are put, along with
 *              a SUCCESS or FAILURE summary for each test.  This is the
 *              mode in which you run a set of reg tests, looking for
 *              failures, and logging the results to a file.  For this
 *              case, as in (b), the returned @display == FALSE.
 *          Case 3:
 *              There is no second arg to the reg test.  The results
 *              (failures, summary) will go to stderr.  The returned
 *              @display == TRUE; this is used with pixDisplayWithTitle().
 *      (2) If you wish to use regTestWritePixAndCheck(), you must
 *          let regTestSetup() make and return an @rp struct.  It is also
 *          convenient to pass @rp into subroutines of the reg test.
 *      (3) See regutils.h for examples of usage.
 */
l_int32
regTestSetup(l_int32        argc,
             char         **argv,
             FILE         **pfp,
             l_int32       *pdisplay,
             l_int32       *psuccess,
             L_REGPARAMS  **prp)
{
char         *tempname;
char          errormsg[64];
L_REGPARAMS  *rp;

    PROCNAME("regTestSetup");

    if (argc != 1 && argc != 2) {
        snprintf(errormsg, sizeof(errormsg),
            "Syntax: %s [generate | <logfile>]", argv[0]);
        return ERROR_INT(errormsg, procName, 1);
    }
    if (prp) *prp = NULL;
    if (!psuccess)
        return ERROR_INT("&success not defined", procName, 1);
    *psuccess = TRUE;
    if (!pdisplay)
        return ERROR_INT("&display not defined", procName, 1);
    if (!pfp)
        return ERROR_INT("&fp defined", procName, 1);
    *pfp = NULL;   /* default for "generate case */

    *pdisplay = (argc == 1) ? TRUE : FALSE;
    if (argc == 1 || strcmp(argv[1], "generate")) {
        tempname = genTempFilename("/tmp", "regtest_output.txt", 1);
        *pfp = fopen(tempname, "wb");
        FREE(tempname);
        if (*pfp == NULL)
            return ERROR_INT("stream not opened", procName, 1);
    }

    if (prp) {
        if ((rp = (L_REGPARAMS *)CALLOC(1, sizeof(L_REGPARAMS))) == NULL)
            return ERROR_INT("rp not made", procName, 1);
        *prp = rp;
        rp->fp = *pfp;
        rp->argv = argv;
        rp->display = *pdisplay;
        rp->success = *psuccess;
    }

    return 0;
}


/*!
 *  regTestCleanup()
 *
 *      Input:  argc (to regtest: either 1 or 2)
 *              argv (to regtest: if @argc == 2, @argv[1] is either
 *                    "generate" or a log file name)
 *              fp (stream that was used writing to a temporary file;
 *                  null for the "generate" case)
 *              success (overall for this reg test)
 *              rp (regression test params; can be null)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This outputs anything written to the temporary file and
 *          closes the stream to that file.
 *      (2) If a rp struct is made in regTestSetup(), it must be
 *          passed in here for destruction.
 */
l_int32
regTestCleanup(l_int32       argc,
               char        **argv,
               FILE         *fp,
               l_int32       success,
               L_REGPARAMS  *rp)
{
char     result[128];
char    *tempname, *text, *message;
l_int32  nbytes;

    PROCNAME("regTestCleanup");

    if (!fp) {  /* for generating golden files; release rp if it exists */
        if (rp) FREE(rp);
        return 0;
    }
    fclose(fp);

        /* Read back data from temp file */
    tempname = genTempFilename("/tmp", "regtest_output.txt", 1);
    text = (char *)arrayRead(tempname, &nbytes);
    FREE(tempname);
    if (!text) {
        if (rp) FREE(rp);
        return ERROR_INT("text not returned", procName, 1);
    }

        /* Prepare result message */
    if (rp)  /* if either is 0, success == FALSE */
        success = rp->success && success;
    if (success)
        snprintf(result, sizeof(result), "SUCCESS: %s\n", argv[0]);
    else
        snprintf(result, sizeof(result), "FAILURE: %s\n", argv[0]);
    message = stringJoin(text, result);
    FREE(text);

    if (argc == 1)
        fprintf(stderr, "%s", message);
    else
        fileAppendString(argv[1], message);
    FREE(message);

    if (rp) FREE(rp);
    return 0;
}


/*!
 *  regTestComparePix()
 *
 *      Input:  stream (for output; use NULL to generate golden files)
 *              argv ([0] == name of reg test)
 *              pix1, pix2 (to be tested for equality)
 *              index (of the pix pair test; 0-based for the reg test)
 *              &success (<return> 0 on failure; input value on success)
 *      Return: 0 if OK, 1 on error (a failure in comparison is not an error)
 *
 *  Notes:
 *      (1) This function compares two pix for equality.  When called
 *          with @fp == NULL, it records success or failure to stderr.
 *          Otherwise, it writes on failure to @fp.
 *      (2) This function can be called repeatedly in a single reg test.
 *      (3) The value for @success is initialized to TRUE in the reg test
 *          setup before this function is called for the first time.
 *          A failure in any pix comparison is registered as a failure
 *          of the regression test.
 */
l_int32
regTestComparePix(FILE     *fp,
                  char    **argv,
                  PIX      *pix1,
                  PIX      *pix2,
                  l_int32   index,
                  l_int32  *psuccess)
{
l_int32  same;

    PROCNAME("regTestComparePix");

    if (!psuccess)
        return ERROR_INT("&success not defined", procName, 1);
    if (index < 0)
        return ERROR_INT("index is negative", procName, 1);
    if (!pix1 || !pix2)
        return ERROR_INT("pix1 and pix2 not both defined", procName, 1);

    pixEqual(pix1, pix2, &same);
    if (!fp) {  /* just output result to stderr */
        if (same)
            fprintf(stderr, "%s: Success in comparison %d\n", argv[0], index);
        else
            fprintf(stderr, "Failure in comparison %d\n", index);
        return 0;
    }

        /* Record on failure */
    if (!same) {
        fprintf(fp, "Failure in %s: pix comparison %d\n", argv[0], index);
        fprintf(stderr, "Failure in %s: pix comparison %d\n", argv[0], index);
        *psuccess = 0;
    }
    return 0;
}


/*!
 *  regTestCompareSimilarPix()
 *
 *      Input:  stream (for output; use NULL to generate golden files)
 *              argv ([0] == name of reg test)
 *              pix1, pix2 (to be tested for equality)
 *              mindiff (minimum pixel difference to be counted; > 0)
 *              maxfract (maximum fraction of pixels allowed to have
 *                        diff greater than or equal to mindiff)
 *              index (of the pix pair test; 0-based for the reg test)
 *              &success (<return> 0 on failure; input value on success)
 *              printstats (use 1 to print normalized histogram to stderr)
 *      Return: 0 if OK, 1 on error (a failure in similarity comparison
 *              is not an error)
 *
 *  Notes:
 *      (1) This function compares two pix for similarity, not equality.
 *          When called with @fp == NULL, it records success or failure
 *          to stderr.  Otherwise, it writes on failure to @fp.
 *      (2) To identify two images as 'similar', select @maxfract to be
 *          the upper bound for what you expect.  Typical values might
 *          be @mindiff = 15 and @maxfract = 0.01.
 *      (3) This function can be called repeatedly in a single reg test.
 *      (4) The value for @success is initialized to TRUE in the reg test
 *          setup before this function is called for the first time.
 *          A failure in any pix comparison is registered as a failure
 *          of the regression test.
 *      (5) Normally, use @printstats = 0.  In debugging mode, to see
 *          the relation between @mindiff and the minimum value of
 *          @maxfract for success, set this to 1.
 */
l_int32
regTestCompareSimilarPix(FILE      *fp,
                         char     **argv,
                         PIX       *pix1,
                         PIX       *pix2,
                         l_int32    mindiff,
                         l_float32  maxfract,
                         l_int32    index,
                         l_int32   *psuccess,
                         l_int32    printstats)
{
l_int32  w, h, factor, similar;

    PROCNAME("regTestCompareSimilarPix");

    if (!psuccess)
        return ERROR_INT("&success not defined", procName, 1);
    if (index < 0)
        return ERROR_INT("index is negative", procName, 1);
    if (!pix1 || !pix2)
        return ERROR_INT("pix1 and pix2 not both defined", procName, 1);

    pixGetDimensions(pix1, &w, &h, NULL);
    factor = L_MAX(w, h) / 400;
    factor = L_MAX(1, L_MIN(factor, 4));   /* between 1 and 4 */
    pixTestForSimilarity(pix1, pix2, factor, mindiff, maxfract, 0.0,
                         &similar, printstats);

    if (!fp) {  /* just output result to stderr */
        if (similar)
            fprintf(stderr, "%s: Success in similarity comparison %d\n",
                    argv[0], index);
        else
            fprintf(stderr, "Failure in similarity comparison %d\n", index);
        return 0;
    }

        /* Record on failure */
    if (!similar) {
        fprintf(fp, "Failure in %s: pix similarity comparison %d\n",
                argv[0], index);
        fprintf(stderr, "Failure in %s: pix similarity comparison %d\n",
                argv[0], index);
        *psuccess = 0;
    }
    return 0;
}


/*!
 *  regTestCheckFile()
 *
 *      Input:  stream (for output; use NULL to generate golden files)
 *              argv ([0] == name of reg test)
 *              localname (name of output file from reg test)
 *              index (of the output file under test; 0-based for the reg test)
 *              &success (<return> 0 on failure; input value on success)
 *      Return: 0 if OK, 1 on error (a failure in comparison is not an error)
 *
 *  Notes:
 *      (1) This function either compares an input file with a "golden" file,
 *          or generates a "golden" file as a copy @localname.
 *          Call with @fp == NULL to generate a new golden file.
 *      (2) This function can be called repeatedly in a single reg test.
 *      (3) The value for @success is initialized to TRUE in the reg test
 *          setup before this function is called for the first time.
 *          A failure in any single file comparison is registered
 *          as a failure of the regression test.
 *      (4) The canonical format of the golden filenames is:
 *             /tmp/<root of main name>_golden.<index>.<ext of localname>
 *          e.g.,
 *             /tmp/maze_golden.0.png
 */
l_int32
regTestCheckFile(FILE        *fp,
                 char       **argv,
                 const char  *localname,
                 l_int32      index,
                 l_int32     *psuccess)
{
char    *root, *ext;
char     namebuf[64];
l_int32  ret, same;

    PROCNAME("regTestCheckFile");

    if (!psuccess)
        return ERROR_INT("&success not defined", procName, 1);
    if (!localname)
        return ERROR_INT("local name not defined", procName, 1);
    if (index < 0)
        return ERROR_INT("index is negative", procName, 1);

        /* Generate the golden file name */
    if ((root = getRootNameFromArgv0(argv[0])) == NULL)
        return ERROR_INT("invalid root", procName, 1);
    splitPathAtExtension(localname, NULL, &ext);
    snprintf(namebuf, sizeof(namebuf), "/tmp/%s_golden.%d%s", root, index, ext);
    FREE(root);
    FREE(ext);

        /* Save the file as a golden file */
    if (!fp) {
        ret = fileCopy(localname, namebuf);
        if (!ret)
            fprintf(stderr, "Copy: %s to %s\n", localname, namebuf);
        return ret;
    }

        /* Test and record on failure */
    filesAreIdentical(localname, namebuf, &same);
    if (!same) {
        fprintf(fp, "Failure in %s: comparing %s with %s\n", argv[0],
                localname, namebuf);
        fprintf(stderr, "Failure in %s: comparing %s with %s\n", argv[0],
                localname, namebuf);
        *psuccess = 0;
    }

    return 0;
}


/*!
 *  regTestCompareFiles()
 *
 *      Input:  stream (for output; use NULL to generate golden files)
 *              argv ([0] == name of reg test)
 *              index1 (of one output file from reg test)
 *              index2 (of another output file from reg test)
 *              &success (<return> 0 on if different; input value on success)
 *      Return: 0 if OK, 1 on error (a failure in comparison is not an error)
 *
 *  Notes:
 *      (1) If @fp != NULL, this function compares two golden files to
 *          determine if they are the same.  If @fp == NULL, this is a
 *          "generate" operation; don't do the comparison.
 *      (2) This function can be called repeatedly in a single reg test.
 *      (3) The value for @success is initialized to TRUE in the reg test
 *          setup before this function is called for the first time.
 *          A failure in any file comparison is registered as a failure
 *          of the regression test.
 *      (4) The canonical format of the golden filenames is:
 *             /tmp/<root of main name>_golden.<index>.<ext of localname>
 *          e.g.,
 *             /tmp/maze_golden.0.png
 */
l_int32
regTestCompareFiles(FILE        *fp,
                    char       **argv,
                    l_int32      index1,
                    l_int32      index2,
                    l_int32     *psuccess)
{
char    *root, *name1, *name2;
char     namebuf[64];
l_int32  error,same;
SARRAY  *sa;

    PROCNAME("regTestCompareFiles");

    if (!psuccess)
        return ERROR_INT("&success not defined", procName, 1);
    if (index1 < 0 || index2 < 0)
        return ERROR_INT("index1 and/or index2 is negative", procName, 1);
    if (index1 == index2)
        return ERROR_INT("index1 must differ from index2", procName, 1);
    if (!fp)  /* no-op */
        return 0;

        /* Generate partial golden file names and find the actual
         * paths to them. */
    error = FALSE;
    name1 = name2 = NULL;
    if ((root = getRootNameFromArgv0(argv[0])) == NULL)
        return ERROR_INT("invalid root", procName, 1);
    snprintf(namebuf, sizeof(namebuf), "%s_golden.%d.", root, index1);
    sa = getSortedPathnamesInDirectory("/tmp", namebuf, 0, 0);
    if (sarrayGetCount(sa) != 1)
        error = TRUE;
    else
        name1 = sarrayGetString(sa, 0, L_COPY);
    sarrayDestroy(&sa);
    snprintf(namebuf, sizeof(namebuf), "%s_golden.%d.", root, index2);
    sa = getSortedPathnamesInDirectory("/tmp", namebuf, 0, 0);
    if (sarrayGetCount(sa) != 1)
        error = TRUE;
    else
        name2 = sarrayGetString(sa, 0, L_COPY);
    sarrayDestroy(&sa);
    FREE(root);
    if (error == TRUE) {
        if (name1) FREE(name1);
        if (name2) FREE(name2);
        L_ERROR("golden files not found", procName);
        return 1;
    }

        /* Test and record on failure */
    filesAreIdentical(name1, name2, &same);
    if (!same) {
        fprintf(fp, "Failure in %s: comparing %s with %s\n", argv[0],
                name1, name2);
        fprintf(stderr, "Failure in %s: comparing %s with %s\n", argv[0],
                name1, name2);
        *psuccess = 0;
    }

    return 0;
}


/*!
 *  regTestWritePixAndCheck()
 *
 *      Input:  pix (to be written)
 *              format (of output pix)
 *              &count (index to be written into filename)
 *              rp (regression test params; required here)
 *      Return: 0 if OK, 1 on error (a failure in comparison is not an error)
 *
 *  Notes:
 *      (1) This function makes it easy to write the pix in a numbered
 *          sequence of files, and either write the golden files (with
 *          the "generate" argument to the regression test) or compare
 *          the written file with an existing golden file.
 *      (2) This function can be called repeatedly in a single reg test.
 *          Each time it is called, the count is incremented.
 *      (3) The canonical format of the local filename is:
 *             /tmp/<root of main name>.<count>.<format extension string>
 *          e.g., for scale_reg,
 *             /tmp/scale.0.png
 */
l_int32
regTestWritePixAndCheck(PIX          *pix,
                        l_int32       format,
                        l_int32      *pcount,
                        L_REGPARAMS  *rp)
{
char    *root;
char     namebuf[256];

    PROCNAME("regTestWritePixAndCheck");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!pcount)
        return ERROR_INT("&count not defined", procName, 1);
    if (format < 0 || format >= NumImageFileFormatExtensions)
        return ERROR_INT("invalid format", procName, 1);
    if (!rp)
        return ERROR_INT("rp not defined", procName, 1);

        /* Generate the local file name */
    if ((root = getRootNameFromArgv0(rp->argv[0])) == NULL)
        return ERROR_INT("invalid root", procName, 1);
    snprintf(namebuf, sizeof(namebuf), "/tmp/%s.%d.%s", root, *pcount,
             ImageFileFormatExtensions[format]);
    FREE(root);

        /* Write the local file.  Then, either write the golden file
         * or check the local file against an existing golden file. */
    pixWrite(namebuf, pix, format);
    regTestCheckFile(rp->fp, rp->argv, namebuf, (*pcount)++, &rp->success);
    return 0;
}



/*!
 *  getRootNameFromArgv0()
 *
 *      Input:  argv0
 *      Return: root name (without the '_reg'), or null on error
 *
 *  Notes:
 *      (1) In windows, argv[0] is a long pathname with multiple
 *          subdirectories, and ending with '.exe'.  In unix it
 *          is just the tail name; e.g., psioseg_reg.
 *          From psioseg_reg, we want to extract just 'psioseg'
 *          as the root.
 */
static char *
getRootNameFromArgv0(const char  *argv0)
{
l_int32  len;
char    *root;

    PROCNAME("getRootNameFromArgv0");

#ifdef _WIN32
    if ((len = strlen(argv0)) < 9)
        return (char *)ERROR_PTR("invalid argv0; too small", procName, NULL);
    splitPathAtDirectory(argv0, NULL, &root);
    root[len - 8] = '\0';
#else
    if ((len = strlen(argv0)) < 5)
        return (char *)ERROR_PTR("invalid argv0", procName, NULL);
    root = stringNew(argv0);
    root[len - 4] = '\0';
#endif  /*  _WIN32 */
    return root;
}


