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
 *   In use, it is convenient to redirect stdout to a file.
 *
 *   An optional 'prestring' can be prepended to each declaration.
 *   Without this string, use:
 *      xtractprotos [list of C files]
 *   With this string, use:
 *      xtractprotos -prestring=[string] [list of C files]
 *
 *   Update the version number when making a new version.
 *
 *   For simple C prototype extraction, xtractprotos has essentially
 *   the same functionality as Adam Bryant's cextract, but the latter
 *   has not been officially supported for over 10 years, has been
 *   patched numerous times, and currently doesn't work with
 *   sys/sysmacros.h for 64 bit architecture.
 *
 *   This is used to extract all prototypes in leptonlib, in the
 *   file leptprotos.h.  The function that does all the work is
 *   parseForProtos(), which takes as input the output from cpp.
 *   To avoid including the very large leptprotos.h in the input
 *   from each file, cpp runs here with -DNO_PROTOS.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

    /* MS VC++ can't handle array initialization in C with static consts */
#define  L_BUF_SIZE   512

    /* Cygwin needs any extension, or it will append ".exe" to the filename! */
static const char *tempfile = "/tmp/temp_cpp_output.txt";
static const char *version = "1.4";


main(int    argc,
     char **argv)
{
char        *filein, *str, *prestring;
const char  *spacestr = " ";
char         buf[L_BUF_SIZE];
l_int32      i, firstfile, len, ret;
SARRAY      *sa;
static char  mainName[] = "xtractprotos";

        /* Output extern C head */
    sa = sarrayCreate(0);
    sarrayAddString(sa, (char *)"/*", 1);
    snprintf(buf, L_BUF_SIZE,
             " *  This file was autogen'd by xtractprotos, v. %s", version);
    sarrayAddString(sa, buf, 1);
    sarrayAddString(sa, (char *)" */", 1);
    sarrayAddString(sa, (char *)"#ifdef __cplusplus", 1);
    sarrayAddString(sa, (char *)"extern \"C\" {", 1);
    sarrayAddString(sa, (char *)"#endif  /* __cplusplus */\n", 1);
    str = sarrayToString(sa, 1);
    fprintf(stdout, "%s", str);
    sarrayDestroy(&sa);
    FREE(str);

        /* Prepend 'prestring' if requested */
    firstfile = 1;
    prestring = NULL;
    if (argv[1][0] == '-') {
        firstfile = 2;
        if (sscanf(argv[1], "-prestring=%s", buf) != 1)
            L_WARNING("Failure to parse prestring; omitting!", mainName);
        else {
            if ((len = strlen(buf)) > L_BUF_SIZE - 3)
                L_WARNING("prestring too large; omitting!", mainName);
            else {
                buf[len] = ' ';
                buf[len + 1] = '\0';
                prestring = stringNew(buf);
            }
        }
    }
        
    for (i = firstfile; i < argc; i++) {
        filein = argv[i];
	len = strlen(filein);
	if (filein[len - 1] == 'h')
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
	if (strlen(str) > 1)  /* strlen(str) == 1 is a file without protos */
            fprintf(stdout, "%s", str);
        FREE(str);
    }

        /* Output extern C tail */
    sa = sarrayCreate(0);
    sarrayAddString(sa, (char *)"\n#ifdef __cplusplus", 1);
    sarrayAddString(sa, (char *)"}", 1);
    sarrayAddString(sa, (char *)"#endif  /* __cplusplus */", 1);
    str = sarrayToString(sa, 1);
    fprintf(stdout, "%s", str);
    sarrayDestroy(&sa);
    FREE(str);
    if (prestring)
        FREE(prestring);

    remove(tempfile);
    return 0;
}


