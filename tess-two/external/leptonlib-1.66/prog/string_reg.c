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
 * string_reg.c
 *
 *    This tests several sarray functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
l_int32      nbytesin, nbytesout;
char        *infile, *outfile, *instring, *outstring;
SARRAY      *sa1, *sa2, *sa3, *sa4, *sa5;
char         buf[256];
static char  mainName[] = "string_reg";

    if (argc != 2)
	exit(ERROR_INT(" Syntax:  string_reg infile", mainName, 1));

    infile = argv[1];
    instring = (char *)arrayRead(infile, &nbytesin);

    sa1 = sarrayCreateWordsFromString(instring);
    sa2 = sarrayCreateLinesFromString(instring, 0);
    sa3 = sarrayCreateLinesFromString(instring, 1);

    outstring = sarrayToString(sa1, 0);
    nbytesout = strlen(outstring);
    arrayWrite("/tmp/junk1.txt", "w", outstring, nbytesout);
    FREE(outstring);

    outstring = sarrayToString(sa1, 1);
    nbytesout = strlen(outstring);
    arrayWrite("/tmp/junk2.txt", "w", outstring, nbytesout);
    FREE(outstring);

    outstring = sarrayToString(sa2, 0);
    nbytesout = strlen(outstring);
    arrayWrite("/tmp/junk3.txt", "w", outstring, nbytesout);
    FREE(outstring);

    outstring = sarrayToString(sa2, 1);
    nbytesout = strlen(outstring);
    arrayWrite("/tmp/junk4.txt", "w", outstring, nbytesout);
    FREE(outstring);

    outstring = sarrayToString(sa3, 0);
    nbytesout = strlen(outstring);
    arrayWrite("/tmp/junk5.txt", "w", outstring, nbytesout);
    FREE(outstring);

    outstring = sarrayToString(sa3, 1);
    nbytesout = strlen(outstring);
    arrayWrite("/tmp/junk6.txt", "w", outstring, nbytesout);
    FREE(outstring);
    sprintf(buf, "diff -s /tmp/junk6.txt %s", infile);
    system(buf);

	/* write/read/write; compare /tmp/junkout5 with /tmp/junkout6 */
    sarrayWrite("/tmp/junk7.txt", sa2);
    sarrayWrite("/tmp/junk8.txt", sa3);
    sa4 = sarrayRead("/tmp/junk8.txt");
    sarrayWrite("/tmp/junk9.txt", sa4);
    sa5 = sarrayRead("/tmp/junk9.txt");
    system("diff -s /tmp/junk8.txt /tmp/junk9.txt");

    sarrayDestroy(&sa1);
    sarrayDestroy(&sa2);
    sarrayDestroy(&sa3);
    sarrayDestroy(&sa4);
    sarrayDestroy(&sa5);
    FREE(instring);

    return 0;
}

