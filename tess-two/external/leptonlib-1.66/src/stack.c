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
 *  stack.c
 *
 *      Generic stack
 *
 *      The lstack is an array of void * ptrs, onto which
 *      objects can be stored.  At any time, the number of
 *      stored objects is lstack->n.  The object at the bottom
 *      of the lstack is at array[0]; the object at the top of
 *      the lstack is at array[n-1].  New objects are added
 *      to the top of the lstack; i.e., the first available 
 *      location, which is at array[n].  The lstack is expanded
 *      by doubling, when needed.  Objects are removed
 *      from the top of the lstack.  When an attempt is made
 *      to remove an object from an empty lstack, the result is null.
 *
 *      Create/Destroy
 *           L_STACK   *lstackCreate()
 *           void       lstackDestroy()
 *
 *      Accessors
 *           l_int32    lstackAdd()
 *           void      *lstackRemove()
 *           l_int32    lstackExtendArray()
 *           l_int32    lstackGetCount()
 *
 *      Text description
 *           l_int32    lstackPrint()
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const l_int32  INITIAL_PTR_ARRAYSIZE = 20;


/*---------------------------------------------------------------------*
 *                          Create/Destroy                             *
 *---------------------------------------------------------------------*/
/*!
 *  lstackCreate()
 *
 *      Input:  nalloc (initial ptr array size; use 0 for default)
 *      Return: lstack, or null on error
 */
L_STACK *
lstackCreate(l_int32  nalloc)
{
L_STACK  *lstack;

    PROCNAME("lstackCreate");

    if (nalloc <= 0)
        nalloc = INITIAL_PTR_ARRAYSIZE;

    if ((lstack = (L_STACK *)CALLOC(1, sizeof(L_STACK))) == NULL)
        return (L_STACK *)ERROR_PTR("lstack not made", procName, NULL);

    if ((lstack->array = (void **)CALLOC(nalloc, sizeof(void *))) == NULL)
        return (L_STACK *)ERROR_PTR("lstack array not made", procName, NULL);

    lstack->nalloc = nalloc;
    lstack->n = 0;
    
    return lstack;
}


/*!
 *  lstackDestroy()
 *
 *      Input:  &lstack (<to be nulled>)
 *              freeflag (TRUE to free each remaining struct in the array)
 *      Return: void
 *
 *  Notes:
 *      (1) If freeflag is TRUE, frees each struct in the array.
 *      (2) If freeflag is FALSE but there are elements on the array,
 *          gives a warning and destroys the array.  This will
 *          cause a memory leak of all the items that were on the lstack.
 *          So if the items require their own destroy function, they
 *          must be destroyed before the lstack.
 *      (3) To destroy the lstack, we destroy the ptr array, then
 *          the lstack, and then null the contents of the input ptr.
 */
void
lstackDestroy(L_STACK  **plstack,
              l_int32    freeflag)
{
void     *item;
L_STACK  *lstack;

    PROCNAME("lstackDestroy");

    if (plstack == NULL) {
        L_WARNING("ptr address is NULL", procName);
        return;
    }
    if ((lstack = *plstack) == NULL)
        return;

    if (freeflag) {
        while(lstack->n > 0) {
            item = lstackRemove(lstack);
            FREE(item);
        }
    }
    else if (lstack->n > 0)
        L_WARNING_INT("memory leak of %d items in lstack", procName, lstack->n);

    if (lstack->auxstack)
        lstackDestroy(&lstack->auxstack, freeflag);

    if (lstack->array)
        FREE(lstack->array);
    FREE(lstack);
    *plstack = NULL;
}



/*---------------------------------------------------------------------*
 *                               Accessors                             *
 *---------------------------------------------------------------------*/
/*!
 *  lstackAdd()
 *
 *      Input:  lstack
 *              item to be added to the lstack
 *      Return: 0 if OK; 1 on error.
 */
l_int32
lstackAdd(L_STACK  *lstack,
          void     *item)
{
    PROCNAME("lstackAdd");

    if (!lstack)
        return ERROR_INT("lstack not defined", procName, 1);
    if (!item)
        return ERROR_INT("item not defined", procName, 1);

        /* Do we need to extend the array? */
    if (lstack->n >= lstack->nalloc)
        lstackExtendArray(lstack);

        /* Store the new pointer */
    lstack->array[lstack->n] = (void *)item;
    lstack->n++;

    return 0;
}


/*!
 *  lstackRemove()
 *
 *      Input:  lstack 
 *      Return: ptr to item popped from the top of the lstack,
 *              or null if the lstack is empty or on error
 */
void *
lstackRemove(L_STACK  *lstack)
{
void  *item;

    PROCNAME("lstackRemove");

    if (!lstack)
        return ERROR_PTR("lstack not defined", procName, NULL);

    if (lstack->n == 0)
        return NULL;

    lstack->n--;
    item = lstack->array[lstack->n];
        
    return item;
}


/*!
 *  lstackExtendArray()
 *
 *      Input:  lstack
 *      Return: 0 if OK; 1 on error
 */
l_int32
lstackExtendArray(L_STACK  *lstack)
{
    PROCNAME("lstackExtendArray");

    if (!lstack)
        return ERROR_INT("lstack not defined", procName, 1);

    if ((lstack->array = (void **)reallocNew((void **)&lstack->array,
                              sizeof(void *) * lstack->nalloc,
                              2 * sizeof(void *) * lstack->nalloc)) == NULL)
        return ERROR_INT("new lstack array not defined", procName, 1);

    lstack->nalloc = 2 * lstack->nalloc;
    return 0;
}


/*!
 *  lstackGetCount()
 *
 *      Input:  lstack
 *      Return: count, or 0 on error
 */
l_int32
lstackGetCount(L_STACK  *lstack)
{
    PROCNAME("lstackGetCount");

    if (!lstack)
        return ERROR_INT("lstack not defined", procName, 1);

    return lstack->n;
}



/*---------------------------------------------------------------------*
 *                            Debug output                             *
 *---------------------------------------------------------------------*/
/*!
 *  lstackPrint()
 *
 *      Input:  stream
 *              lstack
 *      Return: 0 if OK; 1 on error
 */
l_int32
lstackPrint(FILE     *fp,
            L_STACK  *lstack)
{
l_int32  i;

    PROCNAME("lstackPrint");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!lstack)
        return ERROR_INT("lstack not defined", procName, 1);

    fprintf(fp, "\n Stack: nalloc = %d, n = %d, array = %p\n", 
            lstack->nalloc, lstack->n, lstack->array);
    for (i = 0; i < lstack->n; i++)
        fprintf(fp,   "array[%d] = %p\n", i, lstack->array[i]);
    
    return 0;
}

