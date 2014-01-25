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
 *   bbuffer.c
 *
 *      Create/Destroy BBuffer
 *          BBUFFER        *bbufferCreate()
 *          void           *bbufferDestroy()
 *          l_uint8        *bbufferDestroyAndSaveData()
 *
 *      Operations to read data TO a BBuffer
 *          l_int32         bbufferRead()
 *          l_int32         bbufferReadStream()
 *          static l_int32  bbufferExtendArray()
 *
 *      Operations to write data FROM a BBuffer
 *          l_int32         bbufferWrite()
 *          l_int32         bbufferWriteStream()
 *
 *      Accessors
 *          l_int32         bbufferBytesToWrite()
 *
 *      Read from stdin to memory
 *          l_int32         bbufferReadStdin()
 *
 *
 *    The bbuffer is an implementation of a byte queue.
 *    The bbuffer holds a byte array from which bytes are
 *    processed in a first-in/first-out fashion.  As with
 *    any queue, bbuffer maintains two "pointers," one to the
 *    tail of the queue (where you read new bytes onto it)
 *    and one to the head of the queue (where you start from
 *    when writing bytes out of it.
 *
 *    The queue can be visualized:
 *
 *
 *  byte 0                                           byte (nalloc - 1)
 *       |                                                |
 *       --------------------------------------------------
 *                 H                             T
 *       [   aw   ][  bytes currently on queue  ][  anr   ]
 *
 *       ---:  all allocated data in bbuffer
 *       H:    queue head (ptr to next byte to be written out)
 *       T:    queue tail (ptr to first byte to be written to)
 *       aw:   already written from queue
 *       anr:  allocated but not yet read to
 *
 *    The purpose of bbuffer is to allow you to safely read
 *    bytes in, and to sequentially write them out as well.
 *    In the process of writing bytes out, you don't actually
 *    remove the bytes in the array; you just move the pointer
 *    (nwritten) which points to the head of the queue.  In
 *    the process of reading bytes in, you sometimes need to
 *    expand the array size.  If a read is performed after a
 *    write, so that the head of the queue is not at the
 *    beginning of the array, the bytes already written are
 *    first removed by copying the others over them; then the
 *    new bytes are read onto the tail of the queue.
 *
 *    Note that the meaning of "read into" and "write from"
 *    the bbuffer is OPPOSITE to that for a stream, where
 *    you read "from" a stream and write "into" a stream.
 *    As a mnemonic for remembering the direction:
 *        - to read bytes from a stream into the bbuffer,
 *          you call fread on the stream
 *        - to write bytes from the bbuffer into a stream,
 *          you call fwrite on the stream
 *
 *    See zlibmem.c for an example use of bbuffer, where we
 *    compress and decompress an array of bytes in memory.
 *
 *    We can also use the bbuffer trivially to read from stdin
 *    into memory; e.g., to capture bytes piped from the stdout
 *    of another program.  This is equivalent to repeatedly
 *    calling bbufferReadStream() until the input queue is empty.
 */

#include <string.h>
#include "allheaders.h"

static const l_int32  INITIAL_BUFFER_ARRAYSIZE = 1024;   /* n'importe quoi */

    /* Static function */
static l_int32 bbufferExtendArray(BBUFFER *bb, l_int32 nbytes);


/*--------------------------------------------------------------------------*
 *                         BBuffer create/destroy                           *
 *--------------------------------------------------------------------------*/
/*!
 *  bbufferCreate()
 *
 *      Input:  buffer address in memory (<optional>)
 *              size of byte array to be alloc'd (0 for default)
 *      Return: bbuffer, or null on error
 *
 *  Notes:
 *      (1) If a buffer address is given, you should read all the data in.
 *      (2) Allocates a bbuffer with associated byte array of
 *          the given size.  If a buffer address is given,
 *          it then reads the number of bytes into the byte array.
 */
BBUFFER *
bbufferCreate(l_uint8  *indata,
              l_int32   nalloc)
{
BBUFFER  *bb;

    PROCNAME("bbufferCreate");

    if (nalloc <= 0)
        nalloc = INITIAL_BUFFER_ARRAYSIZE;

    if ((bb = (BBUFFER *)CALLOC(1, sizeof(BBUFFER))) == NULL)
        return (BBUFFER *)ERROR_PTR("bb not made", procName, NULL);
    if ((bb->array = (l_uint8 *)CALLOC(nalloc, sizeof(l_uint8))) == NULL)
        return (BBUFFER *)ERROR_PTR("byte array not made", procName, NULL);
    bb->nalloc = nalloc;
    bb->nwritten = 0;

    if (indata) {
        memcpy((l_uint8 *)bb->array, indata, nalloc);
        bb->n = nalloc;
    } else {
        bb->n = 0;
    }

    return bb;
}


/*!
 *  bbufferDestroy()
 *
 *      Input:  &bbuffer  (<to be nulled>)
 *      Return: void
 *
 *  Notes:
 *      (1) Destroys the byte array in the bbuffer and then the bbuffer;
 *          then nulls the contents of the input ptr.
 */
void
bbufferDestroy(BBUFFER  **pbb)
{
BBUFFER  *bb;

    PROCNAME("bbufferDestroy");

    if (pbb == NULL) {
        L_WARNING("ptr address is NULL\n", procName);
        return;
    }

    if ((bb = *pbb) == NULL)
        return;

    if (bb->array)
        FREE(bb->array);
    FREE(bb);
    *pbb = NULL;

    return;
}


/*!
 *  bbufferDestroyAndSaveData()
 *
 *      Input:  &bbuffer (<to be nulled>)
 *              &nbytes  (<return> number of bytes saved in array)
 *      Return: barray (newly allocated array of data)
 *
 *  Notes:
 *      (1) Copies data to newly allocated array; then destroys the bbuffer.
 */
l_uint8 *
bbufferDestroyAndSaveData(BBUFFER  **pbb,
                          size_t    *pnbytes)
{
l_uint8  *array;
size_t    nbytes;
BBUFFER  *bb;

    PROCNAME("bbufferDestroyAndSaveData");

    if (pbb == NULL) {
        L_WARNING("ptr address is NULL\n", procName);
        return NULL;
    }
    if (pnbytes == NULL) {
        L_WARNING("&nbytes is NULL\n", procName);
        bbufferDestroy(pbb);
        return NULL;
    }

    if ((bb = *pbb) == NULL)
        return NULL;

        /* write all unwritten bytes out to a new array */
    nbytes = bb->n - bb->nwritten;
    *pnbytes = nbytes;
    if ((array = (l_uint8 *)CALLOC(nbytes, sizeof(l_uint8))) == NULL) {
        L_WARNING("calloc failure for array\n", procName);
        return NULL;
    }
    memcpy((void *)array, (void *)(bb->array + bb->nwritten), nbytes);

    bbufferDestroy(pbb);
    return array;
}



/*--------------------------------------------------------------------------*
 *                   Operations to read data INTO a BBuffer                 *
 *--------------------------------------------------------------------------*/
/*!
 *  bbufferRead()
 *
 *      Input:  bbuffer
 *              src      (source memory buffer from which bytes are read)
 *              nbytes   (bytes to be read)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) For a read after write, first remove the written
 *          bytes by shifting the unwritten bytes in the array,
 *          then check if there is enough room to add the new bytes.
 *          If not, realloc with bbufferExpandArray(), resulting
 *          in a second writing of the unwritten bytes.  While less
 *          efficient, this is simpler than making a special case
 *          of reallocNew().
 */
l_int32
bbufferRead(BBUFFER  *bb,
            l_uint8  *src,
            l_int32   nbytes)
{
l_int32  navail, nadd, nwritten;

    PROCNAME("bbufferRead");

    if (!bb)
        return ERROR_INT("bb not defined", procName, 1);
    if (!src)
        return ERROR_INT("src not defined", procName, 1);
    if (nbytes == 0)
        return ERROR_INT("no bytes to read", procName, 1);

    if ((nwritten = bb->nwritten)) {  /* move the unwritten bytes over */
        memmove((l_uint8 *)bb->array, (l_uint8 *)(bb->array + nwritten),
                 bb->n - nwritten);
        bb->nwritten = 0;
        bb->n -= nwritten;
    }

        /* If necessary, expand the allocated array.  Do so by
         * by at least a factor of two. */
    navail = bb->nalloc - bb->n;
    if (nbytes > navail) {
        nadd = L_MAX(bb->nalloc, nbytes);
        bbufferExtendArray(bb, nadd);
    }

        /* Read in the new bytes */
    memcpy((l_uint8 *)(bb->array + bb->n), src, nbytes);
    bb->n += nbytes;

    return 0;
}


/*!
 *  bbufferReadStream()
 *
 *      Input:  bbuffer
 *              fp      (source stream from which bytes are read)
 *              nbytes   (bytes to be read)
 *      Return: 0 if OK, 1 on error
 */
l_int32
bbufferReadStream(BBUFFER  *bb,
                  FILE     *fp,
                  l_int32   nbytes)
{
l_int32  navail, nadd, nread, nwritten;

    PROCNAME("bbufferReadStream");

    if (!bb)
        return ERROR_INT("bb not defined", procName, 1);
    if (!fp)
        return ERROR_INT("fp not defined", procName, 1);
    if (nbytes == 0)
        return ERROR_INT("no bytes to read", procName, 1);

    if ((nwritten = bb->nwritten)) {  /* move any unwritten bytes over */
        memmove((l_uint8 *)bb->array, (l_uint8 *)(bb->array + nwritten),
                 bb->n - nwritten);
        bb->nwritten = 0;
        bb->n -= nwritten;
    }

        /* If necessary, expand the allocated array.  Do so by
         * by at least a factor of two. */
    navail = bb->nalloc - bb->n;
    if (nbytes > navail) {
        nadd = L_MAX(bb->nalloc, nbytes);
        bbufferExtendArray(bb, nadd);
    }

        /* Read in the new bytes */
    nread = fread((void *)(bb->array + bb->n), 1, nbytes, fp);
    bb->n += nread;

    return 0;
}


/*!
 *  bbufferExtendArray()
 *
 *      Input:  bbuffer
 *              nbytes  (number of bytes to extend array size)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) reallocNew() copies all bb->nalloc bytes, even though
 *          only bb->n are data.
 */
static l_int32
bbufferExtendArray(BBUFFER  *bb,
                   l_int32   nbytes)
{
    PROCNAME("bbufferExtendArray");

    if (!bb)
        return ERROR_INT("bb not defined", procName, 1);

    if ((bb->array = (l_uint8 *)reallocNew((void **)&bb->array,
                                bb->nalloc,
                                bb->nalloc + nbytes)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);

    bb->nalloc += nbytes;
    return 0;
}



/*--------------------------------------------------------------------------*
 *                  Operations to write data FROM a BBuffer                 *
 *--------------------------------------------------------------------------*/
/*!
 *  bbufferWrite()
 *
 *      Input:  bbuffer
 *              dest     (dest memory buffer to which bytes are written)
 *              nbytes   (bytes requested to be written)
 *              &nout    (<return> bytes actually written)
 *      Return: 0 if OK, 1 on error
 */
l_int32
bbufferWrite(BBUFFER  *bb,
             l_uint8  *dest,
             size_t    nbytes,
             size_t   *pnout)
{
l_int32  nleft, nout;

    PROCNAME("bbufferWrite");

    if (!bb)
        return ERROR_INT("bb not defined", procName, 1);
    if (!dest)
        return ERROR_INT("dest not defined", procName, 1);
    if (nbytes <= 0)
        return ERROR_INT("no bytes requested to write", procName, 1);
    if (!pnout)
        return ERROR_INT("&nout not defined", procName, 1);

    nleft = bb->n - bb->nwritten;
    nout = L_MIN(nleft, nbytes);
    *pnout = nout;

    if (nleft == 0) {   /* nothing to write; reinitialize the buffer */
        bb->n = 0;
        bb->nwritten = 0;
        return 0;
    }

        /* nout > 0; transfer the data out */
    memcpy(dest, (l_uint8 *)(bb->array + bb->nwritten), nout);
    bb->nwritten += nout;

        /* If all written; "empty" the buffer */
    if (nout == nleft) {
        bb->n = 0;
        bb->nwritten = 0;
    }

    return 0;
}


/*!
 *  bbufferWriteStream()
 *
 *      Input:  bbuffer
 *              fp       (dest stream to which bytes are written)
 *              nbytes   (bytes requested to be written)
 *              &nout    (<return> bytes actually written)
 *      Return: 0 if OK, 1 on error
 */
l_int32
bbufferWriteStream(BBUFFER  *bb,
                   FILE     *fp,
                   size_t    nbytes,
                   size_t   *pnout)
{
l_int32  nleft, nout;

    PROCNAME("bbufferWriteStream");

    if (!bb)
        return ERROR_INT("bb not defined", procName, 1);
    if (!fp)
        return ERROR_INT("output stream not defined", procName, 1);
    if (nbytes <= 0)
        return ERROR_INT("no bytes requested to write", procName, 1);
    if (!pnout)
        return ERROR_INT("&nout not defined", procName, 1);

    nleft = bb->n - bb->nwritten;
    nout = L_MIN(nleft, nbytes);
    *pnout = nout;

    if (nleft == 0) {   /* nothing to write; reinitialize the buffer */
        bb->n = 0;
        bb->nwritten = 0;
        return 0;
    }

        /* nout > 0; transfer the data out */
    fwrite((void *)(bb->array + bb->nwritten), 1, nout, fp);
    bb->nwritten += nout;

        /* If all written; "empty" the buffer */
    if (nout == nleft) {
        bb->n = 0;
        bb->nwritten = 0;
    }

    return 0;
}



/*--------------------------------------------------------------------------*
 *                                  Accessors                               *
 *--------------------------------------------------------------------------*/
/*!
 *  bbufferBytesToWrite()
 *
 *      Input:  bbuffer
 *              &nbytes (<return>)
 *      Return: 0 if OK; 1 on error
 */
l_int32
bbufferBytesToWrite(BBUFFER  *bb,
                    size_t   *pnbytes)
{
    PROCNAME("bbufferBytesToWrite");

    if (!bb)
        return ERROR_INT("bb not defined", procName, 1);
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);

    *pnbytes = bb->n - bb->nwritten;
    return 0;
}


/*--------------------------------------------------------------------------*
 *                           Read from stdin to memory                      *
 *--------------------------------------------------------------------------*/
/*!
 *  bbufferReadStdin()
 *
 *      Input:  &data (<return> binary data read in)
 *              &nbytes (<return>)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This can be used to capture data piped in from stdin.
 *          For example, you can read an image from stdin into memory
 *          using shell redirection, with one of these:
 *             cat <imagefile> | readprog
 *             readprog < <imagefile>
 *          where readprog is:
 *             bbufferReadStdin(&data, &nbytes);  // l_uint8*, size_t
 *             Pix *pix = pixReadMem(data, nbytes);
 */
l_int32
bbufferReadStdin(l_uint8  **pdata,
                 size_t    *pnbytes)
{
l_int32   navail, nadd, nread;
BBUFFER  *bb;

    PROCNAME("bbufferReadStdin");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    if (!pnbytes)
        return ERROR_INT("&nbytes not defined", procName, 1);

    bb = bbufferCreate(NULL, 4096);
    while (1) {
        navail = bb->nalloc - bb->n;
        if (navail < 4096) {
             nadd = L_MAX(bb->nalloc, 4096);
             bbufferExtendArray(bb, nadd);
        }
        nread = fread((void *)(bb->array + bb->n), 1, 4096, stdin);
        bb->n += nread;
        if (nread != 4096) break;
    }

    *pdata = bb->array;
    *pnbytes = bb->n;
    bb->array = NULL;
    bbufferDestroy(&bb);
    return 0;
}

