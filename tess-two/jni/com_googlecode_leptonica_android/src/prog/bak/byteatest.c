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
 * byteatest.c
 *
 */

#include <string.h>
#include "allheaders.h"

main(int    argc,
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
        exit(ERROR_INT("syntax: whatever11", mainName, 1));

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
                 0, 0, 100, NULL, 0, NULL);
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


