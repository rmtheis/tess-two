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
 * string_reg.c
 *
 *    This tests several sarray functions.
 *
 *    N.B.  This requires 'diff' for testing.
 */

#include <string.h>
#include "allheaders.h"

int main(int    argc,
         char **argv)
{
l_int32      ignore;
size_t       nbytesin, nbytesout;
char        *infile, *instring, *outstring;
SARRAY      *sa1, *sa2, *sa3, *sa4, *sa5;
char         buf[256];
static char  mainName[] = "string_reg";

    if (argc != 2)
        return ERROR_INT(" Syntax:  string_reg infile", mainName, 1);

    infile = argv[1];
    instring = (char *)l_binaryRead(infile, &nbytesin);

    if (!instring)
        return ERROR_INT("file not read", mainName, 1);

    sa1 = sarrayCreateWordsFromString(instring);
    sa2 = sarrayCreateLinesFromString(instring, 0);
    sa3 = sarrayCreateLinesFromString(instring, 1);

    outstring = sarrayToString(sa1, 0);
    nbytesout = strlen(outstring);
    l_binaryWrite("/tmp/junk1.txt", "w", outstring, nbytesout);
    lept_free(outstring);

    outstring = sarrayToString(sa1, 1);
    nbytesout = strlen(outstring);
    l_binaryWrite("/tmp/junk2.txt", "w", outstring, nbytesout);
    lept_free(outstring);

    outstring = sarrayToString(sa2, 0);
    nbytesout = strlen(outstring);
    l_binaryWrite("/tmp/junk3.txt", "w", outstring, nbytesout);
    lept_free(outstring);

    outstring = sarrayToString(sa2, 1);
    nbytesout = strlen(outstring);
    l_binaryWrite("/tmp/junk4.txt", "w", outstring, nbytesout);
    lept_free(outstring);

    outstring = sarrayToString(sa3, 0);
    nbytesout = strlen(outstring);
    l_binaryWrite("/tmp/junk5.txt", "w", outstring, nbytesout);
    lept_free(outstring);

    outstring = sarrayToString(sa3, 1);
    nbytesout = strlen(outstring);
    l_binaryWrite("/tmp/junk6.txt", "w", outstring, nbytesout);
    lept_free(outstring);
    sprintf(buf, "diff -s /tmp/junk6.txt %s", infile);
    ignore = system(buf);  /* diff */

        /* write/read/write; compare /tmp/junkout5 with /tmp/junkout6 */
    sarrayWrite("/tmp/junk7.txt", sa2);
    sarrayWrite("/tmp/junk8.txt", sa3);
    sa4 = sarrayRead("/tmp/junk8.txt");
    sarrayWrite("/tmp/junk9.txt", sa4);
    sa5 = sarrayRead("/tmp/junk9.txt");
    ignore = system("diff -s /tmp/junk8.txt /tmp/junk9.txt");

    sarrayDestroy(&sa1);
    sarrayDestroy(&sa2);
    sarrayDestroy(&sa3);
    sarrayDestroy(&sa4);
    sarrayDestroy(&sa5);
    lept_free(instring);
    return 0;
}

