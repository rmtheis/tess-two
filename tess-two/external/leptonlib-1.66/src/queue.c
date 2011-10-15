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
 *   queue.c
 *
 *      Create/Destroy L_Queue
 *          L_QUEUE    *lqueueCreate()
 *          void       *lqueueDestroy()
 *
 *      Operations to add/remove to/from a L_Queue
 *          l_int32     lqueueAdd()
 *          l_int32     lqueueExtendArray()
 *          void       *lqueueRemove()
 *
 *      Accessors
 *          l_int32     lqueueGetCount()
 *
 *      Debug output
 *          l_int32     lqueuePrint()
 *
 *    The lqueue is a fifo that implements a queue of void* pointers.
 *    It can be used to hold a queue of any type of struct.
 *    Internally, it maintains two counters: 
 *        nhead:  location of head (in ptrs) from the beginning
 *                of the buffer
 *        nelem:  number of ptr elements stored in the queue
 *    As items are added to the queue, nelem increases.
 *    As items are removed, nhead increases and nelem decreases.
 *    Any time the tail reaches the end of the allocated buffer,
 *      all the pointers are shifted to the left, so that the head
 *      is at the beginning of the array.
 *    If the buffer becomes more than 3/4 full, it doubles in size.
 *
 *    [A circular queue would allow us to skip the shifting and
 *    to resize only when the buffer is full.  For most applications,
 *    the extra work we do for a linear queue is not significant.]
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "allheaders.h"

static const l_int32  MIN_BUFFER_SIZE = 20;             /* n'importe quoi */
static const l_int32  INITIAL_BUFFER_ARRAYSIZE = 1024;  /* n'importe quoi */


/*--------------------------------------------------------------------------*
 *                         L_Queue create/destroy                           *
 *--------------------------------------------------------------------------*/
/*!
 *  lqueueCreate()
 *
 *      Input:  size of ptr array to be alloc'd (0 for default)
 *      Return: lqueue, or null on error
 *
 *  Notes:
 *      (1) Allocates a ptr array of given size, and initializes counters.
 */
L_QUEUE *
lqueueCreate(l_int32  nalloc)
{
L_QUEUE  *lq;

    PROCNAME("lqueueCreate");

    if (nalloc < MIN_BUFFER_SIZE)
        nalloc = INITIAL_BUFFER_ARRAYSIZE;

    if ((lq = (L_QUEUE *)CALLOC(1, sizeof(L_QUEUE))) == NULL)
        return (L_QUEUE *)ERROR_PTR("lq not made", procName, NULL);
    if ((lq->array = (void **)CALLOC(nalloc, sizeof(void *))) == NULL)
        return (L_QUEUE *)ERROR_PTR("ptr array not made", procName, NULL);
    lq->nalloc = nalloc;
    lq->nhead = lq->nelem = 0;
    return lq;
}


/*!
 *  lqueueDestroy()
 *
 *      Input:  &lqueue  (<to be nulled>)
 *              freeflag (TRUE to free each remaining struct in the array)
 *      Return: void
 *
 *  Notes:
 *      (1) If freeflag is TRUE, frees each struct in the array.
 *      (2) If freeflag is FALSE but there are elements on the array,
 *          gives a warning and destroys the array.  This will
 *          cause a memory leak of all the items that were on the queue.
 *          So if the items require their own destroy function, they
 *          must be destroyed before the queue.  The same applies to the
 *          auxiliary stack, if it is used.
 *      (3) To destroy the L_Queue, we destroy the ptr array, then
 *          the lqueue, and then null the contents of the input ptr.
 */
void
lqueueDestroy(L_QUEUE  **plq,
              l_int32    freeflag)
{
void     *item;
L_QUEUE  *lq;

    PROCNAME("lqueueDestroy");

    if (plq == NULL) {
        L_WARNING("ptr address is NULL", procName);
        return;
    }
    if ((lq = *plq) == NULL)
        return;

    if (freeflag) {
        while(lq->nelem > 0) {
            item = lqueueRemove(lq);
            FREE(item);
        }
    }
    else if (lq->nelem > 0)
        L_WARNING_INT("memory leak of %d items in lqueue!",
                      procName, lq->nelem);

    if (lq->array)
        FREE(lq->array);
    if (lq->stack)
        lstackDestroy(&lq->stack, freeflag);
    FREE(lq);
    *plq = NULL;

    return;
}


/*--------------------------------------------------------------------------*
 *                                  Accessors                               *
 *--------------------------------------------------------------------------*/
/*!
 *  lqueueAdd()
 *
 *      Input:  lqueue
 *              item to be added to the tail of the queue
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) The algorithm is as follows.  If the queue is populated
 *          to the end of the allocated array, shift all ptrs toward
 *          the beginning of the array, so that the head of the queue
 *          is at the beginning of the array.  Then, if the array is
 *          more than 0.75 full, realloc with double the array size.
 *          Finally, add the item to the tail of the queue.
 */
l_int32
lqueueAdd(L_QUEUE  *lq,
          void     *item)
{
    PROCNAME("lqueueAdd");

    if (!lq)
        return ERROR_INT("lq not defined", procName, 1);
    if (!item)
        return ERROR_INT("item not defined", procName, 1);
    
        /* If filled to the end and the ptrs can be shifted to the left,
         * shift them. */
    if ((lq->nhead + lq->nelem >= lq->nalloc) && (lq->nhead != 0)) {
        memmove(lq->array, lq->array + lq->nhead, sizeof(void *) * lq->nelem);
        lq->nhead = 0;
    }

        /* If necessary, expand the allocated array by a factor of 2 */
    if (lq->nelem > 0.75 * lq->nalloc)
        lqueueExtendArray(lq);

        /* Now add the item */
    lq->array[lq->nhead + lq->nelem] = (void *)item;
    lq->nelem++;

    return 0;
}


/*!
 *  lqueueExtendArray()
 *
 *      Input:  lqueue
 *      Return: 0 if OK, 1 on error
 */
l_int32
lqueueExtendArray(L_QUEUE  *lq)
{
    PROCNAME("lqueueExtendArray");

    if (!lq)
        return ERROR_INT("lq not defined", procName, 1);

    if ((lq->array = (void **)reallocNew((void **)&lq->array,
                                sizeof(void *) * lq->nalloc,
                                2 * sizeof(void *) * lq->nalloc)) == NULL)
        return ERROR_INT("new ptr array not returned", procName, 1);

    lq->nalloc = 2 * lq->nalloc;
    return 0;
}


/*!
 *  lqueueRemove()
 *
 *      Input:  lqueue
 *      Return: ptr to item popped from the head of the queue,
 *              or null if the queue is empty or on error
 *
 *  Notes:
 *      (1) If this is the last item on the queue, so that the queue
 *          becomes empty, nhead is reset to the beginning of the array.
 */
void *
lqueueRemove(L_QUEUE  *lq)
{
void  *item;

    PROCNAME("lqueueRemove");

    if (!lq)
        return (void *)ERROR_PTR("lq not defined", procName, NULL);

    if (lq->nelem == 0)
        return NULL;
    item = lq->array[lq->nhead];
    lq->array[lq->nhead] = NULL;
    if (lq->nelem == 1) 
        lq->nhead = 0;  /* reset head ptr */
    else
        (lq->nhead)++;  /* can't go off end of array because nelem > 1 */
    lq->nelem--;
    return item;
}
       

/*!
 *  lqueueGetCount()
 *
 *      Input:  lqueue
 *      Return: count, or 0 on error
 */
l_int32
lqueueGetCount(L_QUEUE  *lq)
{
    PROCNAME("lqueueGetCount");

    if (!lq)
        return ERROR_INT("lq not defined", procName, 0);

    return lq->nelem;
}
        

/*---------------------------------------------------------------------*
 *                            Debug output                             *
 *---------------------------------------------------------------------*/
/*!
 *  lqueuePrint()
 *  
 *      Input:  stream
 *              lqueue 
 *      Return: 0 if OK; 1 on error
 */
l_int32
lqueuePrint(FILE     *fp,
            L_QUEUE  *lq)
{
l_int32  i;

    PROCNAME("lqueuePrint");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!lq)
        return ERROR_INT("lq not defined", procName, 1);

    fprintf(fp, "\n L_Queue: nalloc = %d, nhead = %d, nelem = %d, array = %p\n",
            lq->nalloc, lq->nhead, lq->nelem, lq->array);
    for (i = lq->nhead; i < lq->nhead + lq->nelem; i++)
    fprintf(fp,   "array[%d] = %p\n", i, lq->array[i]);

    return 0;
}

