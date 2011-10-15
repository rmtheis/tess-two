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
 *   zlibmem.c
 *
 *      zlib operations in memory, using bbuffer
 *          l_uint8   *zlibCompress()
 *          l_uint8   *zlibUncompress()
 *
 *
 *    This provides an example use of the byte buffer utility
 *    (see bbuffer.c for details of how the bbuffer works internally).
 *    We use zlib to compress and decompress a byte array from
 *    one memory buffer to another.  The standard method uses streams,
 *    but here we use the bbuffer as an expandable queue of pixels
 *    for both the reading and writing sides of each operation.
 *
 *    With memory mapping, one should be able to compress between
 *    memory buffers by using the file system to buffer everything in
 *    the background, but the bbuffer implementation is more portable.
 */

#include <stdio.h>
#include <stdlib.h> 
#include "allheaders.h"

#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif  /* HAVE_CONFIG_H */

/* --------------------------------------------*/
#if  HAVE_LIBZ   /* defined in environ.h */
/* --------------------------------------------*/

#include "zlib.h"

static const l_int32  L_BUF_SIZE = 32768;
static const l_int32  ZLIB_COMPRESSION_LEVEL = 6;


/*!
 *  zlibCompress()
 *
 *      Input:  datain (byte buffer with input data)
 *              nin    (number of bytes of input data)
 *              &nout  (<return> number of bytes of output data)
 *      Return: dataout (compressed data), or null on error
 *
 *  Notes:
 *      (1) We repeatedly read in and fill up an input buffer,
 *          compress the data, and read it back out.  zlib
 *          uses two byte buffers internally in the z_stream
 *          data structure.  We use the bbuffers to feed data
 *          into the fixed bufferin, and feed it out of bufferout,
 *          in the same way that a pair of streams would normally
 *          be used if the data were being read from one file
 *          and written to another.  This is done iteratively,
 *          compressing L_BUF_SIZE bytes of input data at a time.
 */
l_uint8 *
zlibCompress(l_uint8  *datain,
             l_int32   nin,
             l_int32  *pnout)
{
l_uint8  *dataout;
l_int32   status, nbytes;
l_uint8  *bufferin, *bufferout;
BBUFFER  *bbin, *bbout;
z_stream  z;

    PROCNAME("zlibCompress");

    if (!datain)
        return (l_uint8 *)ERROR_PTR("datain not defined", procName, NULL);

        /* Set up fixed size buffers used in z_stream */
    if ((bufferin = (l_uint8 *)CALLOC(L_BUF_SIZE, sizeof(l_uint8))) == NULL)
        return (l_uint8 *)ERROR_PTR("bufferin not made", procName, NULL);
    if ((bufferout = (l_uint8 *)CALLOC(L_BUF_SIZE, sizeof(l_uint8))) == NULL)
        return (l_uint8 *)ERROR_PTR("bufferout not made", procName, NULL);

        /* Set up bbuffers and load bbin with the data */
    if ((bbin = bbufferCreate(datain, nin)) == NULL)
        return (l_uint8 *)ERROR_PTR("bbin not made", procName, NULL);
    if ((bbout = bbufferCreate(NULL, 0)) == NULL)
        return (l_uint8 *)ERROR_PTR("bbout not made", procName, NULL);

    z.zalloc = (alloc_func)0;
    z.zfree = (free_func)0;
    z.opaque = (voidpf)0;

    z.next_in = bufferin;
    z.avail_in = 0;
    z.next_out = bufferout;
    z.avail_out = L_BUF_SIZE;

    deflateInit(&z, ZLIB_COMPRESSION_LEVEL);

    for ( ; ; ) {
        if (z.avail_in == 0) {
            z.next_in = bufferin;
            bbufferWrite(bbin, bufferin, L_BUF_SIZE, &nbytes);
/*            fprintf(stderr, " wrote %d bytes to bufferin\n", nbytes); */
            z.avail_in = nbytes;
        }
        if (z.avail_in == 0)
            break;
        status = deflate(&z, Z_SYNC_FLUSH);
/*        fprintf(stderr, " status is %d, bytesleft = %d, totalout = %d\n",
                  status, z.avail_out, z.total_out); */
        nbytes = L_BUF_SIZE - z.avail_out;
        if (nbytes) {
            bbufferRead(bbout, bufferout, nbytes);
/*            fprintf(stderr, " read %d bytes from bufferout\n", nbytes); */
        }
        z.next_out = bufferout;
        z.avail_out = L_BUF_SIZE;
    }

    deflateEnd(&z);

    bbufferDestroy(&bbin);
    dataout = bbufferDestroyAndSaveData(&bbout, pnout);

    FREE(bufferin);
    FREE(bufferout);
    return dataout;
}
        

/*!
 *  zlibUncompress()
 *
 *      Input:  datain (byte buffer with compressed input data)
 *              nin    (number of bytes of input data)
 *              &nout  (<return> number of bytes of output data)
 *      Return: dataout (uncompressed data), or null on error
 *
 *  Notes:
 *      (1) See zlibCompress().
 */
l_uint8 *
zlibUncompress(l_uint8  *datain,
               l_int32   nin,
               l_int32  *pnout)
{
l_uint8  *dataout;
l_int32   status, nbytes;
l_uint8  *bufferin, *bufferout;
BBUFFER  *bbin, *bbout;
z_stream  z;

    PROCNAME("zlibUncompress");

    if (!datain)
        return (l_uint8 *)ERROR_PTR("datain not defined", procName, NULL);
    
    if ((bufferin = (l_uint8 *)CALLOC(L_BUF_SIZE, sizeof(l_uint8))) == NULL)
        return (l_uint8 *)ERROR_PTR("bufferin not made", procName, NULL);
    if ((bufferout = (l_uint8 *)CALLOC(L_BUF_SIZE, sizeof(l_uint8))) == NULL)
        return (l_uint8 *)ERROR_PTR("bufferout not made", procName, NULL);

    if ((bbin = bbufferCreate(datain, nin)) == NULL)
        return (l_uint8 *)ERROR_PTR("bbin not made", procName, NULL);
    if ((bbout = bbufferCreate(NULL, 0)) == NULL)
        return (l_uint8 *)ERROR_PTR("bbout not made", procName, NULL);

    z.zalloc = (alloc_func)0;
    z.zfree = (free_func)0;

    z.next_in = bufferin;
    z.avail_in = 0;
    z.next_out = bufferout;
    z.avail_out = L_BUF_SIZE;

    inflateInit(&z);

    for ( ; ; ) {
        if (z.avail_in == 0) {
            z.next_in = bufferin;
            bbufferWrite(bbin, bufferin, L_BUF_SIZE, &nbytes);
/*            fprintf(stderr, " wrote %d bytes to bufferin\n", nbytes); */
            z.avail_in = nbytes;
        }
        if (z.avail_in == 0)
            break;
        status = inflate(&z, Z_SYNC_FLUSH);
/*        fprintf(stderr, " status is %d, bytesleft = %d, totalout = %d\n",
                  status, z.avail_out, z.total_out); */
        nbytes = L_BUF_SIZE - z.avail_out;
        if (nbytes) {
            bbufferRead(bbout, bufferout, nbytes);
/*            fprintf(stderr, " read %d bytes from bufferout\n", nbytes); */
        }
        z.next_out = bufferout;
        z.avail_out = L_BUF_SIZE;
    }

    inflateEnd(&z);

    bbufferDestroy(&bbin);
    dataout = bbufferDestroyAndSaveData(&bbout, pnout);

    FREE(bufferin);
    FREE(bufferout);
    return dataout;
}

/* --------------------------------------------*/
#endif  /* HAVE_LIBZ */
/* --------------------------------------------*/

