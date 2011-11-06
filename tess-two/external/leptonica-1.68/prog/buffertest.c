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
 * buffertest.c
 *
 *   Tests the bbuffer operations
 */

#include "allheaders.h"

#define   NBLOCKS     11

main(int    argc,
     char **argv)
{
char        *filein, *fileout;
l_uint8     *array1, *array2, *dataout, *dataout2;
l_int32      i, blocksize;
size_t       nbytes, nout, nout2;
BBUFFER     *bb, *bb2;
FILE        *fp;
static char  mainName[] = "buffertest";

    if (argc != 3)
	exit(ERROR_INT(" Syntax:  buffertest filein fileout", mainName, 1));

    filein = argv[1];
    fileout = argv[2];

    if ((array1 = l_binaryRead(filein, &nbytes)) == NULL)
	exit(ERROR_INT("array not made", mainName, 1));
    fprintf(stderr, " Bytes read from file: %ld\n", nbytes);

        /* Application of byte buffer ops: compress/decompress in memory */
#if 1
    dataout = zlibCompress(array1, nbytes, &nout);
    l_binaryWrite(fileout, "w", dataout, nout);

    dataout2 = zlibUncompress(dataout, nout, &nout2);
    l_binaryWrite("/tmp/junktest", "w", dataout2, nout2);

    fprintf(stderr,
            "nbytes in = %ld, nbytes comp = %ld, nbytes uncomp = %ld\n",
            nbytes, nout, nout2);
    lept_free(dataout);
    lept_free(dataout2);
#endif

        /* Low-level byte buffer read/write test */
#if 0
    bb = bbufferCreate(array1, nbytes);
    bbufferRead(bb, array1, nbytes);

    array2 = (l_uint8 *)lept_calloc(2 * nbytes, sizeof(l_uint8));

    fprintf(stderr, " Bytes initially in buffer: %d\n", bb->n);

    blocksize = (2 * nbytes) / NBLOCKS;
    for (i = 0; i <= NBLOCKS; i++) {
	bbufferWrite(bb, array2, blocksize, &nout);
	fprintf(stderr, " block %d: wrote %d bytes\n", i + 1, nout); 
    }

    fprintf(stderr, " Bytes left in buffer: %d\n", bb->n);

    bb2 = bbufferCreate(NULL, 0);
    bbufferRead(bb2, array1, nbytes);
    fp = lept_fopen(fileout, "wb");
    bbufferWriteStream(bb2, fp, nbytes, &nout);
    fprintf(stderr, " bytes written out to fileout: %d\n", nout);
	    
    bbufferDestroy(&bb);
    bbufferDestroy(&bb2);
    lept_free(array2);
#endif

    lept_free(array1);
    return 0;
}

