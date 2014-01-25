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
 * byteatest.c
 *
 */

#include <string.h>
#include "allheaders.h"

int main(int    argc,
         char **argv)
{
char        *str;
l_uint8     *data1, *data2;
l_int32      i, n, start, end, same1, same2;
size_t       size1, size2;
FILE        *fp;
L_DNA       *da;
SARRAY      *sa;
L_BYTEA     *lba1, *lba2, *lba3, *lba4, *lba5;
static char  mainName[] = "byteatest";

    if (argc != 1)
        return ERROR_INT("syntax: byteatest", mainName, 1);

        /* Test basic init, join and split */
    lba1 = l_byteaInitFromFile("feyn.tif");
    lba2 = l_byteaInitFromFile("test24.jpg");
    size1 = l_byteaGetSize(lba1);
    size2 = l_byteaGetSize(lba2);
    l_byteaJoin(lba1, &lba2);
    lba3 = l_byteaInitFromMem(lba1->data, size1);
    lba4 = l_byteaInitFromMem(lba1->data + size1, size2);

        /* Split by hand */
    l_binaryWrite("junk1", "w", lba3->data, lba3->size);
    l_binaryWrite("junk2", "w", lba4->data, lba4->size);
    filesAreIdentical("feyn.tif", "junk1", &same1);
    filesAreIdentical("test24.jpg", "junk2", &same2);
    if (same1 && same2)
        fprintf(stderr, "OK for join file\n");
    else
        fprintf(stderr, "Error: files are different!\n");

        /* Split by function */
    l_byteaSplit(lba1, size1, &lba5);
    l_binaryWrite("junk3", "w", lba1->data, lba1->size);
    l_binaryWrite("junk4", "w", lba5->data, lba5->size);
    filesAreIdentical("feyn.tif", "junk3", &same1);
    filesAreIdentical("test24.jpg", "junk4", &same2);
    if (same1 && same2)
        fprintf(stderr, "OK for split file\n");
    else
        fprintf(stderr, "Error: files are different!\n");
    l_byteaDestroy(&lba1);
    l_byteaDestroy(&lba2);
    l_byteaDestroy(&lba3);
    l_byteaDestroy(&lba4);
    l_byteaDestroy(&lba5);

        /* Test appending */
    data1 = l_binaryRead("whatever10.c", &size1);
    sa = sarrayCreateLinesFromString((char *)data1, 1);
    lba1 = l_byteaCreate(0);
    n = sarrayGetCount(sa);
    for (i = 0; i < n; i++) {
        str = sarrayGetString(sa, i, L_NOCOPY);
        l_byteaAppendString(lba1, str);
        l_byteaAppendString(lba1, (char *)"\n");
    }
    data2 = l_byteaGetData(lba1, &size2);
    l_binaryWrite("junk1.txt", "w", data2, size2);
    filesAreIdentical("whatever10.c", "junk1.txt", &same1);
    if (same1)
        fprintf(stderr, "OK for appended file\n");
    else
        fprintf(stderr, "Error: appended file is different!\n");
    lept_free(data1);
    sarrayDestroy(&sa);
    l_byteaDestroy(&lba1);

        /* Test search */
    convertToPdf("test24.jpg", L_JPEG_ENCODE, 0, "junk3.pdf",
                 0, 0, 100, NULL, NULL, 0);
    lba1 = l_byteaInitFromFile("junk3.pdf");
    l_byteaFindEachSequence(lba1, (l_uint8 *)" 0 obj\n", 7, &da);
    l_dnaWriteStream(stderr, da);
    l_byteaDestroy(&lba1);
    l_dnaDestroy(&da);

        /* Test write to file */
    lba1 = l_byteaInitFromFile("feyn.tif");
    fp = lept_fopen("junk5", "wb");
    size1 = l_byteaGetSize(lba1);
    for (start = 0; start < size1; start += 1000) {
         end = L_MIN(start + 1000 - 1, size1 - 1);
         l_byteaWriteStream(fp, lba1, start, end);
    }
    lept_fclose(fp);
    l_byteaDestroy(&lba1);

    return 0;
}


