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
 * xtractprotos.c
 *
 *   This program accepts a list of C files on the command line
 *   and outputs the C prototypes to stdout.  It uses cpp to
 *   handle the preprocessor macros, and then parses the cpp output.
 *   In leptonica, it is used to make allheaders.h (and optionally
 *   leptprotos.h, which contains just the function prototypes.)
 *   In leptonica, only the file allheaders.h is included with
 *   source files.
 *
 *   An optional 'prestring' can be prepended to each declaration.
 *   And the function prototypes can either be sent to stdout, written
 *   to a named file, or placed in-line within allheaders.h.
 *
 *   The signature is:
 *
 *     xtractprotos [-prestring=<string>] [-protos=<where>] [list of C files]
 *
 *   Without -protos, the prototypes are written to stdout.
 *   With -protos, allheaders.h is rewritten:
 *      * if you use -protos=inline, the prototypes are placed within
 *        allheaders.h.
 *      * if you use -protos=leptprotos.h, the prototypes written to
 *        the file leptprotos.h, and alltypes.h has
 *           #include "leptprotos.h"
 *
 *   For constructing allheaders.h, two text files are provided:
 *      allheaders_top.txt
 *      allheaders_bot.txt
 *   The former contains the leptonica version number, so it must
 *   be updated when a new version is made.
 *
 *   For simple C prototype extraction, xtractprotos has essentially
 *   the same functionality as Adam Bryant's cextract, but the latter
 *   has not been officially supported for over 15 years, has been
 *   patched numerous times, and doesn't work with sys/sysmacros.h
 *   for 64 bit architecture.
 *
 *   This is used to extract all prototypes in liblept.
 *   The function that does all the work is parseForProtos(),
 *   which takes as input the output from cpp.
 *
 *   xtractprotos can run in leptonica to do an 'ab initio' generation
 *   of allheaders.h; that is, it can make allheaders.h without
 *   leptprotos.h and with an allheaders.h file of 0 length.
 *   Of course, the usual situation is to run it with a valid allheaders.h,
 *   which includes all the function prototypes.  To avoid including
 *   all the prototypes in the input for each file, cpp runs here
 *   with -DNO_PROTOS, so the prototypes are not included -- this is
 *   much faster.
 *
 *   The xtractprotos version number, defined below, is incremented
 *   whenever a new version is made.
 */

#include <string.h>
#include "allheaders.h"

    /* MS VC++ can't handle array initialization in C with static consts */
#define  L_BUF_SIZE   512

    /* Cygwin needs any extension, or it will append ".exe" to the filename! */
static const char *tempfile = "/tmp/temp_cpp_output.txt";
static const char *version = "1.5";


main(int    argc,
     char **argv)
{
char        *filein, *str, *prestring, *outprotos, *protostr;
const char  *spacestr = " ";
char         buf[L_BUF_SIZE];
l_uint8     *allheaders;
l_int32      i, maxindex, in_line, nflags, protos_added, firstfile, len, ret;
size_t       nbytes;
L_BYTEA     *ba, *ba2;
SARRAY      *sa, *safirst;
static char  mainName[] = "xtractprotos";

    if (argc == 1) {
        fprintf(stderr,
                "xtractprotos [-prestring=<string>] [-protos=<where>] "
                "[list of C files]\n"
                "where the prestring is prepended to each prototype, and \n"
                "protos can be either 'inline' or the name of an output "
                "prototype file\n");
        return 1;
    }

    /* ---------------------------------------------------------------- */
    /* Parse input flags and find prestring and outprotos, if requested */
    /* ---------------------------------------------------------------- */
    prestring = outprotos = NULL;
    in_line = FALSE;
    nflags = 0;
    maxindex = L_MIN(3, argc);
    for (i = 1; i < maxindex; i++) {
        if (argv[i][0] == '-') {
            if (!strncmp(argv[i], "-prestring", 10)) {
                nflags++;
                ret = sscanf(argv[i] + 1, "prestring=%s", buf);
                if (ret != 1) {
                    fprintf(stderr, "parse failure for prestring\n");
                    return 1;
                }
                if ((len = strlen(buf)) > L_BUF_SIZE - 3)
                    L_WARNING("prestring too large; omitting!", mainName);
                else {
                    buf[len] = ' ';
                    buf[len + 1] = '\0';
                    prestring = stringNew(buf);
                }
            }
            else if (!strncmp(argv[i], "-protos", 7)) {
                nflags++;
                ret = sscanf(argv[i] + 1, "protos=%s", buf);
                if (ret != 1) {
                    fprintf(stderr, "parse failure for protos\n");
                    return 1;
                }
                outprotos = stringNew(buf);
                if (!strncmp(outprotos, "inline", 7))
                    in_line = TRUE;
            }
        }
    }

    if (argc - nflags < 2) {
        fprintf(stderr, "no files specified!\n");
        return 1;
    }


    /* ---------------------------------------------------------------- */
    /*                   Generate the prototype string                  */
    /* ---------------------------------------------------------------- */
    ba = l_byteaCreate(500);

        /* First the extern C head */
    sa = sarrayCreate(0);
    sarrayAddString(sa, (char *)"/*", 1);
    snprintf(buf, L_BUF_SIZE,
             " *  These prototypes were autogen'd by xtractprotos, v. %s",
             version);
    sarrayAddString(sa, buf, 1);
    sarrayAddString(sa, (char *)" */", 1);
    sarrayAddString(sa, (char *)"#ifdef __cplusplus", 1);
    sarrayAddString(sa, (char *)"extern \"C\" {", 1);
    sarrayAddString(sa, (char *)"#endif  /* __cplusplus */\n", 1);
    str = sarrayToString(sa, 1);
    l_byteaAppendString(ba, str);
    lept_free(str);
    sarrayDestroy(&sa);

        /* Then the prototypes */
    firstfile = 1 + nflags;
    protos_added = FALSE;
    for (i = firstfile; i < argc; i++) {
        filein = argv[i];
	len = strlen(filein);
	if (filein[len - 1] == 'h')  /* skip .h files */
	    continue;
	snprintf(buf, L_BUF_SIZE, "cpp -ansi -DNO_PROTOS %s %s",
	         filein, tempfile);
	ret = system(buf);
	if (ret) {
            fprintf(stderr, "cpp failure for %s; continuing\n", filein);
	    continue;
	}

	if ((str = parseForProtos(tempfile, prestring)) == NULL) {
            fprintf(stderr, "parse failure for %s; continuing\n", filein);
	    continue;
	}
	if (strlen(str) > 1) {  /* strlen(str) == 1 is a file without protos */
            l_byteaAppendString(ba, str);
            protos_added = TRUE;
        }
        lept_free(str);
    }

        /* Lastly the extern C tail */
    sa = sarrayCreate(0);
    sarrayAddString(sa, (char *)"\n#ifdef __cplusplus", 1);
    sarrayAddString(sa, (char *)"}", 1);
    sarrayAddString(sa, (char *)"#endif  /* __cplusplus */", 1);
    str = sarrayToString(sa, 1);
    l_byteaAppendString(ba, str);
    lept_free(str);
    sarrayDestroy(&sa);

    protostr = (char *)l_byteaCopyData(ba, &nbytes);
    l_byteaDestroy(&ba);


    /* ---------------------------------------------------------------- */
    /*                       Generate the output                        */
    /* ---------------------------------------------------------------- */
    if (!outprotos) {  /* just write to stdout */
        fprintf(stderr, "%s\n", protostr);
        lept_free(protostr);
        return 0;
    }

        /* If no protos were found, do nothing further */
    if (!protos_added) {
        fprintf(stderr, "No protos found\n");
        lept_free(protostr);
        return 1;
    }

        /* Make the output files */
    ba = l_byteaInitFromFile("allheaders_top.txt");
    if (!in_line) {
        snprintf(buf, sizeof(buf), "#include \"%s\"\n", outprotos);
        l_byteaAppendString(ba, buf);
        l_binaryWrite(outprotos, "w", protostr, nbytes);
    }
    else
        l_byteaAppendString(ba, protostr);
    ba2 = l_byteaInitFromFile("allheaders_bot.txt");
    l_byteaJoin(ba, &ba2);
    l_byteaWrite("allheaders.h", ba, 0, 0);
    l_byteaDestroy(&ba);
    lept_free(protostr);
    return 0;
}
