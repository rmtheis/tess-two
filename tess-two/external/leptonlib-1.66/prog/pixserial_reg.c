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
 * pixserial_reg.c
 *
 *    Tests the fast (uncompressed) serialization of pix to a string
 *    in memory and the deserialization back to a pix.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

    /* Use this set */
static l_int32  nfiles = 10;
static const char  *filename[] = {
                         "feyn.tif",         /* 1 bpp */
                         "dreyfus2.png",     /* 2 bpp cmapped */
                         "dreyfus4.png",     /* 4 bpp cmapped */
                         "weasel4.16c.png",  /* 4 bpp cmapped */
                         "dreyfus8.png",     /* 8 bpp cmapped */
                         "weasel8.240c.png", /* 8 bpp cmapped */
                         "karen8.jpg",       /* 8 bpp, not cmapped */
                         "test16.tif",       /* 8 bpp, not cmapped */
                         "marge.jpg",        /* rgb */
                         "test24.jpg"        /* rgb */
                            };

main(int    argc,
     char **argv)
{
char       buf[256];
size_t     nbytes;
l_int32    i, w, h, success, display;
l_int32    format, bps, spp, iscmap, format2, w2, h2, bps2, spp2, iscmap2;
l_uint8   *data;
l_uint32  *data32, *data32r;
BOX       *box;
FILE      *fp;
PIX       *pixs, *pixt, *pixt2, *pixd;

    if (regTestSetup(argc, argv, &fp, &display, &success, NULL))
        return 1;

            /* Test basic serialization/deserialization */
    for (i = 0; i < nfiles; i++) {
        pixs = pixRead(filename[i]);
            /* Serialize to memory */
        pixSerializeToMemory(pixs, &data32, &nbytes);
            /* Just for fun, write and read back from file */
        arrayWrite("/tmp/array", "w", data32, nbytes);
        data32r = (l_uint32 *)arrayRead("/tmp/array", (l_int32 *)(&nbytes)); 
            /* Deserialize */
        pixd = pixDeserializeFromMemory(data32r, nbytes);
        regTestComparePix(fp, argv, pixs, pixd, i, &success);
        pixDestroy(&pixd);
        pixDestroy(&pixs);
        FREE(data32);
        FREE(data32r);
    }

            /* Test read/write fileio interface */
    for (i = 0; i < nfiles; i++) {
        pixs = pixRead(filename[i]);
        pixGetDimensions(pixs, &w, &h, NULL);
        box = boxCreate(0, 0, L_MIN(150, w), L_MIN(150, h));
        pixt = pixClipRectangle(pixs, box, NULL);
        boxDestroy(&box);
        snprintf(buf, sizeof(buf), "/tmp/pixs.%d", i);
        pixWrite(buf, pixt, IFF_SPIX);
        regTestCheckFile(fp, argv, buf, i, &success);
        pixt2 = pixRead(buf);
        regTestComparePix(fp, argv, pixt, pixt2, nfiles + i, &success);
        pixDestroy(&pixs);
        pixDestroy(&pixt);
        pixDestroy(&pixt2);
    }
    
            /* Test read header.  Note that for rgb input, spp = 3,
             * but for 32 bpp spix, we set spp = 4. */
    for (i = 0; i < nfiles; i++) {
        pixs = pixRead(filename[i]);
        pixWriteMem(&data, &nbytes, pixs, IFF_SPIX);
        pixReadHeader(filename[i], &format, &w, &h, &bps, &spp, &iscmap);
        pixReadHeaderMem(data, nbytes, &format2, &w2, &h2, &bps2,
                         &spp2, &iscmap2);
        if (format2 != 16 || w != w2 || h != h2 || bps != bps2 ||
            iscmap != iscmap2) {
            if (fp)
                fprintf(fp, "Failure comparing data");
            else
                fprintf(stderr, "Failure comparing data");
            success = FALSE;
        }
        pixDestroy(&pixs);
        FREE(data);
    }

#if 0
        /* Do timing */
    for (i = 0; i < nfiles; i++) {
        pixs = pixRead(filename[i]);
        startTimer();
        pixSerializeToMemory(pixs, &data32, &nbytes);
        pixd = pixDeserializeFromMemory(data32, nbytes);
        fprintf(stderr, "Time for %s: %7.3f sec\n", filename[i], stopTimer());
        FREE(data32);
        pixDestroy(&pixs);
        pixDestroy(&pixd);
    }
#endif

    regTestCleanup(argc, argv, fp, success, NULL);
    return 0;
}

