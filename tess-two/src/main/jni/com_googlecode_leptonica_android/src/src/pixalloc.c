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
 *  pixalloc.c
 *
 *      Custom memory storage with allocator and deallocator
 *
 *          l_int32       pmsCreate()
 *          void          pmsDestroy()
 *          void         *pmsCustomAlloc()
 *          void          pmsCustomDealloc()
 *          void         *pmsGetAlloc()
 *          l_int32       pmsGetLevelForAlloc()
 *          l_int32       pmsGetLevelForDealloc()
 *          void          pmsLogInfo()
 */

#include "allheaders.h"

/*-------------------------------------------------------------------------*
 *                          Pix Memory Storage                             *
 *                                                                         *
 *  This is a simple utility for handling pix memory storage.  It is       *
 *  enabled by setting the PixMemoryManager allocators to the functions    *
 *  that are defined here                                                  *
 *        pmsCustomAlloc()                                                 *
 *        pmsCustomDealloc()                                               *
 *  Use pmsCreate() at the beginning to do the pre-allocation, and         *
 *  pmsDestroy() at the end to clean it up.                                *
 *-------------------------------------------------------------------------*/
/*
 *  In the following, the "memory" refers to the image data
 *  field that is used within the pix.  The memory store is a
 *  continuous block of memory, that is logically divided into
 *  smaller "chunks" starting with a set at a minimum size, and
 *  followed by sets of increasing size that are a power of 2 larger
 *  than the minimum size.  You must specify the number of chunks
 *  of each size.
 *
 *  A requested data chunk, if it exists, is borrowed from the memory
 *  storage, and returned after use.  If the chunk is too small, or
 *  too large, or if all chunks in the appropriate size range are
 *  in use, the memory is allocated dynamically and freed after use.
 *
 *  There are four parameters that determine the use of pre-allocated memory:
 *
 *    minsize: any requested chunk smaller than this is allocated
 *             dynamically and destroyed after use.  No preallocated
 *             memory is used.
 *    smallest: the size of the smallest pre-allocated memory chunk.
 *    nlevels:  the number of different sizes of data chunks, each a
 *              power of 2 larger than 'smallest'.
 *    numalloc: a Numa of size 'nlevels' containing the number of data
 *              chunks for each size that are in the memory store.
 *
 *  As an example, suppose:
 *    minsize = 0.5MB
 *    smallest = 1.0MB
 *    nlevels = 4
 *    numalloc = {10, 5, 5, 5}
 *  Then the total amount of allocated memory (in MB) is
 *    10 * 1 + 5 * 2 + 5 * 4 + 5 * 8 = 80 MB
 *  Any pix requiring less than 0.5 MB or more than 8 MB of memory will
 *  not come from the memory store.  Instead, it will be dynamically
 *  allocated and freed after use.
 *
 *  How is this implemented?
 *
 *  At setup, the full data block size is computed and allocated.
 *  The addresses of the individual chunks are found, and the pointers
 *  are stored in a set of Ptra (generic pointer arrays), using one Ptra
 *  for each of the sizes of the chunks.  When returning a chunk after
 *  use, it is necessary to determine from the address which size level
 *  (ptra) the chunk belongs to.  This is done by comparing the address
 *  of the associated chunk.
 *
 *  In the event that memory chunks need to be dynamically allocated,
 *  either (1) because they are too small or too large for the memory
 *  store or (2) because all the pix of that size (i.e., in the
 *  appropriate level) in the memory store are in use, the
 *  addresses generated will be outside the pre-allocated block.
 *  After use they won't be returned to a ptra; instead the deallocator
 *  will free them.
 */


struct PixMemoryStore
{
    struct L_Ptraa  *paa;          /* Holds ptrs to allocated memory        */
    size_t           minsize;      /* Pix smaller than this (in bytes)      */
                                   /* are allocated dynamically             */
    size_t           smallest;     /* Smallest mem (in bytes) alloc'd       */
    size_t           largest;      /* Larest mem (in bytes) alloc'd         */
    size_t           nbytes;       /* Size of allocated block w/ all chunks */
    l_int32          nlevels;      /* Num of power-of-2 sizes pre-alloc'd   */
    size_t          *sizes;        /* Mem sizes at each power-of-2 level    */
    l_int32         *allocarray;   /* Number of mem alloc'd at each size    */
    l_uint32        *baseptr;      /* ptr to allocated array                */
    l_uint32        *maxptr;       /* ptr just beyond allocated memory      */
    l_uint32       **firstptr;     /* array of ptrs to first chunk in size  */
    l_int32         *memused;      /* log: total # of pix used (by level)   */
    l_int32         *meminuse;     /* log: # of pix in use (by level)       */
    l_int32         *memmax;       /* log: max # of pix in use (by level)   */
    l_int32         *memempty;     /* log: # of pix alloc'd because         */
                                   /*      the store was empty (by level)   */
    char            *logfile;      /* log: set to null if no logging        */
};
typedef struct PixMemoryStore   L_PIX_MEM_STORE;

static L_PIX_MEM_STORE  *CustomPMS = NULL;


/*!
 *  pmsCreate()
 *
 *      Input:  minsize (of data chunk that can be supplied by pms)
 *              smallest (bytes of the smallest pre-allocated data chunk.
 *              numalloc (array with the number of data chunks for each
 *                        size that are in the memory store)
 *              logfile (use for debugging; null otherwise)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This computes the size of the block of memory required
 *          and allocates it.  Each chunk starts on a 32-bit word boundary.
 *          The chunk sizes are in powers of 2, starting at @smallest,
 *          and the number of levels and chunks at each level is
 *          specified by @numalloc.
 *      (2) This is intended to manage the image data for a small number
 *          of relatively large pix.  The system malloc is expected to
 *          handle very large numbers of small chunks efficiently.
 *      (3) Important: set the allocators and call this function
 *          before any pix have been allocated.  Destroy all the pix
 *          in the normal way before calling pmsDestroy().
 *      (4) The pms struct is stored in a static global, so this function
 *          is not thread-safe.  When used, there must be only one thread
 *          per process.
 */
l_int32
pmsCreate(size_t       minsize,
          size_t       smallest,
          NUMA        *numalloc,
          const char  *logfile)
{
l_int32           nlevels, i, j, nbytes;
l_int32          *alloca;
l_float32         nchunks;
l_uint32         *baseptr, *data;
l_uint32        **firstptr;
size_t           *sizes;
L_PIX_MEM_STORE  *pms;
L_PTRA           *pa;
L_PTRAA          *paa;

    PROCNAME("createPMS");

    if (!numalloc)
        return ERROR_INT("numalloc not defined", procName, 1);
    numaGetSum(numalloc, &nchunks);
    if (nchunks > 1000.0)
        L_WARNING("There are %.0f chunks\n", procName, nchunks);

    if ((pms = (L_PIX_MEM_STORE *)CALLOC(1, sizeof(L_PIX_MEM_STORE))) == NULL)
        return ERROR_INT("pms not made", procName, 1);
    CustomPMS = pms;

        /* Make sure that minsize and smallest are multiples of 32 bit words */
    if (minsize % 4 != 0)
        minsize -= minsize % 4;
    pms->minsize = minsize;
    nlevels = numaGetCount(numalloc);
    pms->nlevels = nlevels;

    if ((sizes = (size_t *)CALLOC(nlevels, sizeof(size_t))) == NULL)
        return ERROR_INT("sizes not made", procName, 1);
    pms->sizes = sizes;
    if (smallest % 4 != 0)
        smallest += 4 - (smallest % 4);
    pms->smallest = smallest;
    for (i = 0; i < nlevels; i++)
        sizes[i] = smallest * (1 << i);
    pms->largest = sizes[nlevels - 1];

    alloca = numaGetIArray(numalloc);
    pms->allocarray = alloca;
    if ((paa = ptraaCreate(nlevels)) == NULL)
        return ERROR_INT("paa not made", procName, 1);
    pms->paa = paa;

    for (i = 0, nbytes = 0; i < nlevels; i++)
        nbytes += alloca[i] * sizes[i];
    pms->nbytes = nbytes;

    if ((baseptr = (l_uint32 *)CALLOC(nbytes / 4, sizeof(l_uint32))) == NULL)
        return ERROR_INT("calloc fail for baseptr", procName, 1);
    pms->baseptr = baseptr;
    pms->maxptr = baseptr + nbytes / 4;  /* just beyond the memory store */
    if ((firstptr = (l_uint32 **)CALLOC(nlevels, sizeof(l_uint32 *))) == NULL)
        return ERROR_INT("calloc fail for firstptr", procName, 1);
    pms->firstptr = firstptr;

    data = baseptr;
    for (i = 0; i < nlevels; i++) {
        if ((pa = ptraCreate(alloca[i])) == NULL)
            return ERROR_INT("pa not made", procName, 1);
        ptraaInsertPtra(paa, i, pa);
        firstptr[i] = data;
        for (j = 0; j < alloca[i]; j++) {
            ptraAdd(pa, data);
            data += sizes[i] / 4;
        }
    }

    if (logfile) {
        pms->memused = (l_int32 *)CALLOC(nlevels, sizeof(l_int32));
        pms->meminuse = (l_int32 *)CALLOC(nlevels, sizeof(l_int32));
        pms->memmax = (l_int32 *)CALLOC(nlevels, sizeof(l_int32));
        pms->memempty = (l_int32 *)CALLOC(nlevels, sizeof(l_int32));
        pms->logfile = stringNew(logfile);
    }

    return 0;
}


/*!
 *  pmsDestroy()
 *
 *      Input:  (none)
 *      Return: void
 *
 *  Notes:
 *      (1) Important: call this function at the end of the program, after
 *          the last pix has been destroyed.
 */
void
pmsDestroy()
{
L_PIX_MEM_STORE  *pms;

    if ((pms = CustomPMS) == NULL)
        return;

    ptraaDestroy(&pms->paa, FALSE, FALSE);  /* don't touch the ptrs */
    FREE(pms->baseptr);  /* free the memory */

    if (pms->logfile) {
        pmsLogInfo();
        FREE(pms->logfile);
        FREE(pms->memused);
        FREE(pms->meminuse);
        FREE(pms->memmax);
        FREE(pms->memempty);
    }

    FREE(pms->sizes);
    FREE(pms->allocarray);
    FREE(pms->firstptr);
    FREE(pms);
    CustomPMS = NULL;
    return;
}


/*!
 *  pmsCustomAlloc()
 *
 *      Input: nbytes (min number of bytes in the chunk to be retrieved)
 *      Return: data (ptr to chunk)
 *
 *  Notes:
 *      (1) This attempts to find a suitable pre-allocated chunk.
 *          If not found, it dynamically allocates the chunk.
 *      (2) If logging is turned on, the allocations that are not taken
 *          from the memory store, and are at least as large as the
 *          minimum size the store can handle, are logged to file.
 */
void *
pmsCustomAlloc(size_t  nbytes)
{
l_int32           level;
void             *data;
L_PIX_MEM_STORE  *pms;
L_PTRA           *pa;

    PROCNAME("pmsCustomAlloc");

    if ((pms = CustomPMS) == NULL)
        return (void *)ERROR_PTR("pms not defined", procName, NULL);

    pmsGetLevelForAlloc(nbytes, &level);

    if (level < 0) {  /* size range invalid; must alloc */
        if ((data = pmsGetAlloc(nbytes)) == NULL)
            return (void *)ERROR_PTR("data not made", procName, NULL);
    } else {  /* get from store */
        pa = ptraaGetPtra(pms->paa, level, L_HANDLE_ONLY);
        data = ptraRemoveLast(pa);
        if (data && pms->logfile) {
            pms->memused[level]++;
            pms->meminuse[level]++;
            if (pms->meminuse[level] > pms->memmax[level])
                pms->memmax[level]++;
        }
        if (!data) {  /* none left at this level */
            data = pmsGetAlloc(nbytes);
            if (pms->logfile)
                pms->memempty[level]++;
        }
    }

    return data;
}


/*!
 *  pmsCustomDealloc()
 *
 *      Input: data (to be freed or returned to the storage)
 *      Return: void
 */
void
pmsCustomDealloc(void  *data)
{
l_int32           level;
L_PIX_MEM_STORE  *pms;
L_PTRA           *pa;

    PROCNAME("pmsCustomDealloc");

    if ((pms = CustomPMS) == NULL) {
        L_ERROR("pms not defined\n", procName);
        return;
    }

    if (pmsGetLevelForDealloc(data, &level) == 1) {
        L_ERROR("level not found\n", procName);
        return;
    }

    if (level < 0) {  /* no logging; just free the data */
        FREE(data);
    } else {  /* return the data to the store */
        pa = ptraaGetPtra(pms->paa, level, L_HANDLE_ONLY);
        ptraAdd(pa, data);
        if (pms->logfile)
            pms->meminuse[level]--;
    }

    return;
}


/*!
 *  pmsGetAlloc()
 *
 *      Input:  nbytes
 *      Return: data
 *
 *  Notes:
 *      (1) This is called when a request for pix data cannot be
 *          obtained from the preallocated memory store.  After use it
 *          is freed like normal memory.
 *      (2) If logging is on, only write out allocs that are as large as
 *          the minimum size handled by the memory store.
 *      (3) size_t is %lu on 64 bit platforms and %u on 32 bit platforms.
 *          The C99 platform-independent format specifier for size_t is %zu,
 *          but windows hasn't conformed, so we are forced to go back to
 *          C89, use %lu, and cast to get platform-independence.  Ugh.
 */
void *
pmsGetAlloc(size_t  nbytes)
{
void             *data;
FILE             *fp;
L_PIX_MEM_STORE  *pms;

    PROCNAME("pmsGetAlloc");

    if ((pms = CustomPMS) == NULL)
        return (void *)ERROR_PTR("pms not defined", procName, NULL);

    if ((data = (void *)CALLOC(nbytes, sizeof(char))) == NULL)
        return (void *)ERROR_PTR("data not made", procName, NULL);
    if (pms->logfile && nbytes >= pms->smallest) {
        fp = fopenWriteStream(pms->logfile, "a");
        fprintf(fp, "Alloc %lu bytes at %p\n", (unsigned long)nbytes, data);
        fclose(fp);
    }

    return data;
}


/*!
 *  pmsGetLevelForAlloc()
 *
 *      Input: nbytes (min number of bytes in the chunk to be retrieved)
 *             &level (<return>; -1 if either too small or too large)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pmsGetLevelForAlloc(size_t    nbytes,
                    l_int32  *plevel)
{
l_int32           i;
l_float64         ratio;
L_PIX_MEM_STORE  *pms;

    PROCNAME("pmsGetLevelForAlloc");

    if (!plevel)
        return ERROR_INT("&level not defined", procName, 1);
    *plevel = -1;
    if ((pms = CustomPMS) == NULL)
        return ERROR_INT("pms not defined", procName, 1);

    if (nbytes < pms->minsize || nbytes > pms->largest)
        return 0;   /*  -1  */

    ratio = (l_float64)nbytes / (l_float64)(pms->smallest);
    for (i = 0; i < pms->nlevels; i++) {
        if (ratio <= 1.0)
            break;
        ratio /= 2.0;
    }
    *plevel = i;

    return 0;
}


/*!
 *  pmsGetLevelForDealloc()
 *
 *      Input: data (ptr to memory chunk)
 *             &level (<return> level in memory store; -1 if allocated
 *                     outside the store)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pmsGetLevelForDealloc(void     *data,
                      l_int32  *plevel)
{
l_int32           i;
l_uint32         *first;
L_PIX_MEM_STORE  *pms;

    PROCNAME("pmsGetLevelForDealloc");

    if (!plevel)
        return ERROR_INT("&level not defined", procName, 1);
    *plevel = -1;
    if (!data)
        return ERROR_INT("data not defined", procName, 1);
    if ((pms = CustomPMS) == NULL)
        return ERROR_INT("pms not defined", procName, 1);

    if (data < (void *)pms->baseptr || data >= (void *)pms->maxptr)
        return 0;   /*  -1  */

    for (i = 1; i < pms->nlevels; i++) {
        first = pms->firstptr[i];
        if (data < (void *)first)
            break;
    }
    *plevel = i - 1;

    return 0;
}


/*!
 *  pmsLogInfo()
 *
 *      Input:  (none)
 *      Return: void
 */
void
pmsLogInfo()
{
l_int32           i;
L_PIX_MEM_STORE  *pms;

    if ((pms = CustomPMS) == NULL)
        return;

    fprintf(stderr, "Total number of pix used at each level\n");
    for (i = 0; i < pms->nlevels; i++)
         fprintf(stderr, " Level %d (%lu bytes): %d\n", i,
                 (unsigned long)pms->sizes[i], pms->memused[i]);

    fprintf(stderr, "Max number of pix in use at any time in each level\n");
    for (i = 0; i < pms->nlevels; i++)
         fprintf(stderr, " Level %d (%lu bytes): %d\n", i,
                 (unsigned long)pms->sizes[i], pms->memmax[i]);

    fprintf(stderr, "Number of pix alloc'd because none were available\n");
    for (i = 0; i < pms->nlevels; i++)
         fprintf(stderr, " Level %d (%lu bytes): %d\n", i,
                 (unsigned long)pms->sizes[i], pms->memempty[i]);

    return;
}
