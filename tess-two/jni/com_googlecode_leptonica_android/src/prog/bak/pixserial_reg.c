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
char          buf[256];
size_t        size;
l_int32       i, w, h;
l_int32       format, bps, spp, iscmap, format2, w2, h2, bps2, spp2, iscmap2;
l_uint8      *data;
l_uint32     *data32, *data32r;
BOX          *box;
PIX          *pixs, *pixt, *pixt2, *pixd;
L_REGPARAMS  *rp;

    if (regTestSetup(argc, argv, &rp))
        return 1;

            /* Test basic serialization/deserialization */
    data32 = NULL;
    for (i = 0; i < nfiles; i++) {
        pixs = pixRead(filename[i]);
            /* Serialize to memory */
        pixSerializeToMemory(pixs, &data32, &size);
            /* Just for fun, write and read back from file */
        l_binaryWrite("/tmp/array", "w", data32, size);
        data32r = (l_uint32 *)l_binaryRead("/tmp/array", &size); 
            /* Deserialize */
        pixd = pixDeserializeFromMemory(data32r, size);
        regTestComparePix(rp, pixs, pixd);  /* i */
        pixDestroy(&pixd);
        pixDestroy(&pixs);
        lept_free(data32);
        lept_free(data32r);
    }

            /* Test read/write fileio interface */
    for (i = 0; i < nfiles; i++) {
        pixs = pixRead(filename[i]);
        pixGetDimensions(pixs, &w, &h, NULL);
        box = boxCreate(0, 0, L_MIN(150, w), L_MIN(150, h));
        pixt = pixClipRectangle(pixs, box, NULL);
        boxDestroy(&box);
        snprintf(buf, sizeof(buf), "/tmp/pixs.%d.spix", rp->index + 1);
        pixWrite(buf, pixt, IFF_SPIX);
        regTestCheckFile(rp, buf);  /* nfiles + 2 * i */
        pixt2 = pixRead(buf);
        regTestComparePix(rp, pixt, pixt2);  /* nfiles + 2 * i + 1 */
        pixDestroy(&pixs);
        pixDestroy(&pixt);
        pixDestroy(&pixt2);
    }
    
            /* Test read header.  Note that for rgb input, spp = 3,
             * but for 32 bpp spix, we set spp = 4. */
    data = NULL;
    for (i = 0; i < nfiles; i++) {
        pixs = pixRead(filename[i]);
        pixWriteMem(&data, &size, pixs, IFF_SPIX);
        pixReadHeader(filename[i], &format, &w, &h, &bps, &spp, &iscmap);
        pixReadHeaderMem(data, size, &format2, &w2, &h2, &bps2,
                         &spp2, &iscmap2);
        if (format2 != IFF_SPIX || w != w2 || h != h2 || bps != bps2 ||
            iscmap != iscmap2) {
            if (rp->fp)
                fprintf(rp->fp, "Failure comparing data");
            else
                fprintf(stderr, "Failure comparing data");
        }
        pixDestroy(&pixs);
        lept_free(data);
    }

#if 0
        /* Do timing */
    for (i = 0; i < nfiles; i++) {
        pixs = pixRead(filename[i]);
        startTimer();
        pixSerializeToMemory(pixs, &data32, &size);
        pixd = pixDeserializeFromMemory(data32, size);
        fprintf(stderr, "Time for %s: %7.3f sec\n", filename[i], stopTimer());
        lept_free(data32);
        pixDestroy(&pixs);
        pixDestroy(&pixd);
    }
#endif

    return regTestCleanup(rp);
}
