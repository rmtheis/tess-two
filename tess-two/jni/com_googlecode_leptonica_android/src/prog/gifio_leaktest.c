/*====================================================================*
 -  Copyright (C) 2015 Leptonica.  All rights reserved.
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
 * gifio_leaktest.c
 *
 *     Contributed by Tom Powers.
 *
 *     If gifio_leaktest is supplied with any argument the write testing will
 *     be skipped.
 *
 *     Uses church.png to write and read church.gif 600 times 
 *     to test for file handle leakage (which was a problem on Windows and
 *     MinGW). The Microsoft C Runtime Library has a limit of
 *     512 open files as documented at "File Handling"
 *     https://msdn.microsoft.com/en-us/library/kdfaxaay(v=vs.90).aspx .
 */

#include "allheaders.h"
#define REPETITIONS 600

int main(int    argc,
         char **argv)
{
char        *vers;
PIX         *pixs;
l_int32      i;
l_int32      nWriteTries = REPETITIONS;
static char  mainName[] = "gifio_leaktest";

    vers = getLeptonicaVersion();
    fprintf(stderr, "%s\n", vers);
    lept_free(vers);
    vers = getImagelibVersions();
    fprintf(stderr, "%s\n", vers);
    lept_free(vers);

    if (argc > 2)
        return ERROR_INT(" Syntax:  gifio_leaktest [skipWriteTests]",
                         mainName, 1);
    if (argc == 2)
        nWriteTries = 0;

    if ((pixs = pixRead("church.png")) == NULL)
        return ERROR_INT("pix not found", mainName, 1);

    for (i = 0; i < nWriteTries; i++) {
        fprintf(stderr, "Writing attempt %d\n", i+1);
        if (pixWrite("/tmp/church.gif", pixs, IFF_GIF)) {
            fprintf(stderr, "Failed to write gif file on %dth try.\n", i+1);
            break;
        }
    }
    pixDestroy(&pixs);
    if (i == REPETITIONS)
        fprintf(stderr, "Successfully wrote gif file %d times.\n", i);

    for (i = 0; i < REPETITIONS; i++) {
        fprintf(stderr, "Reading attempt %d\n", i+1);
        pixs = pixRead("/tmp/church.gif");
        if (pixs == NULL) {
            fprintf(stderr, "Failed to read gif file on %dth try.\n", i+1);
            break;
        } else {
        pixDestroy(&pixs);
        }
    }
    if (i == REPETITIONS)
        fprintf(stderr, "Successfully read gif file %d times.\n", i);

    return 0;
}

