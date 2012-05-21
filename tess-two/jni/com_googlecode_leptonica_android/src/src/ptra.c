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
 *   ptra.c
 *
 *      Ptra creation and destruction
 *          L_PTRA      *ptraCreate()
 *          void        *ptraDestroy()
 *
 *      Add/insert/remove/replace generic ptr object
 *          l_int32      ptraAdd()
 *          l_int32      ptraExtendArray()
 *          l_int32      ptraInsert()
 *          void        *ptraGetHandle()
 *          void        *ptraRemove()
 *          void        *ptraRemoveLast()
 *          void        *ptraReplace()
 *          l_int32      ptraSwap()
 *          l_int32      ptraCompactArray()
 *
 *      Other array operations
 *          l_int32      ptraReverse()
 *          l_int32      ptraJoin()
 *
 *      Simple Ptra accessors
 *          l_int32      ptraGetMaxIndex()
 *          l_int32      ptraGetActualCount()
 *          void        *ptraGetPtrToItem()
 *
 *      Ptraa creation and destruction
 *          L_PTRAA     *ptraaCreate()
 *          void        *ptraaDestroy()
 *
 *      Ptraa accessors
 *          l_int32      ptraaGetSize()
 *          l_int32      ptraaInsertPtra()
 *          L_PTRA      *ptraaGetPtra()
 *
 *      Ptraa conversion
 *          L_PTRA      *ptraaFlattenToPtra()
 *
 *      Functions using L_PTRA
 *          NUMA        *numaGetBinSortIndex()
 *
 *    Notes on the Ptra:
 *
 *    (1) The Ptra is a struct, not an array.  Always use the accessors
 *        in this file, never the fields directly.
 *    (2) Items can be placed anywhere in the allocated ptr array,
 *        including one index beyond the last ptr (in which case the
 *        ptr array is realloc'd).
 *    (3) Thus, the items on the ptr array need not be compacted.  In
 *        general there will be null pointers in the ptr array.
 *    (4) A compacted array will remain compacted on removal if
 *        arbitrary items are removed with compaction, or if items
 *        are removed from the end of the array.
 *    (5) For addition to and removal from the end of the array, this
 *        functions exactly like a stack, and with the same O(1) cost.
 *    (6) This differs from the generic stack in that we allow
 *        random access for insertion, removal and replacement.
 *        Removal can be done without compacting the array.
 *        Insertion into a null ptr in the array has no effect on
 *        the other pointers, but insertion into a location already
 *        occupied by an item has a cost proportional to the
 *        distance to the next null ptr in the array.
 *    (7) Null ptrs are valid input args for both insertion and
 *        replacement; this allows arbitrary swapping.
 *    (8) The item in the array with the largest index is at pa->imax.
 *        This can be any value from -1 (initialized; all array ptrs
 *        are null) up to pa->nalloc - 1 (the last ptr in the array).
 *    (9) In referring to the array: the first ptr is the "top" or
 *        "beginning"; the last pointer is the "bottom" or "end";
 *        items are shifted "up" towards the top when compaction occurs;
 *        and items are shifted "down" towards the bottom when forced to
 *        move due to an insertion.
 *   (10) It should be emphasized that insertion, removal and replacement
 *        are general:
 *         * You can insert an item into any ptr location in the
 *           allocated ptr array, as well as into the next ptr address
 *           beyond the allocated array (in which case a realloc will occur).   
 *         * You can remove or replace an item from any ptr location
 *           in the allocated ptr array.
 *         * When inserting into an occupied location, you have
 *           three options for downshifting.
 *         * When removing, you can either leave the ptr null or
 *           compact the array.
 *
 *    Notes on the Ptraa:
 *
 *    (1) The Ptraa is a fixed size ptr array for holding Ptra.
 *        In that respect, it is different from other pointer arrays, which
 *        are extensible and grow using the *Add*() functions.
 *    (2) In general, the Ptra ptrs in the Ptraa can be randomly occupied.
 *        A typical usage is to allow an O(n) horizontal sort of Pix,
 *        where the size of the Ptra array is the width of the image,
 *        and each Ptra is an array of all the Pix at a specific x location.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const l_int32 INITIAL_PTR_ARRAYSIZE = 20;      /* n'importe quoi */


/*--------------------------------------------------------------------------*
 *                       Ptra creation and destruction                      *
 *--------------------------------------------------------------------------*/
/*!
 *  ptraCreate()
 *
 *      Input:  size of ptr array to be alloc'd (0 for default)
 *      Return: pa, or null on error
 */
L_PTRA *
ptraCreate(l_int32  n)
{
L_PTRA  *pa;

    PROCNAME("ptraCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((pa = (L_PTRA *)CALLOC(1, sizeof(L_PTRA))) == NULL)
        return (L_PTRA *)ERROR_PTR("pa not made", procName, NULL);
    if ((pa->array = (void **)CALLOC(n, sizeof(void *))) == NULL)
        return (L_PTRA *)ERROR_PTR("ptr array not made", procName, NULL);

    pa->nalloc = n;
    pa->imax = -1;
    pa->nactual = 0;

    return pa;
}


/*!
 *  ptraDestroy()
 *
 *      Input:  &ptra (<to be nulled>)
 *              freeflag (TRUE to free each remaining item in the array)
 *              warnflag (TRUE to warn if any remaining items are not destroyed)
 *      Return: void
 *
 *  Notes:
 *      (1) If @freeflag == TRUE, frees each item in the array.
 *      (2) If @freeflag == FALSE and warnflag == TRUE, and there are
 *          items on the array, this gives a warning and destroys the array.
 *          If these items are not owned elsewhere, this will cause
 *          a memory leak of all the items that were on the array.
 *          So if the items are not owned elsewhere and require their
 *          own destroy function, they must be destroyed before the ptra.
 *      (3) If warnflag == FALSE, no warnings will be issued.  This is
 *          useful if the items are owned elsewhere, such as a
 *          PixMemoryStore().
 *      (4) To destroy the ptra, we destroy the ptr array, then
 *          the ptra, and then null the contents of the input ptr.
 */
void
ptraDestroy(L_PTRA  **ppa,
            l_int32   freeflag,
            l_int32   warnflag)
{
l_int32  i, nactual;
void    *item;
L_PTRA  *pa;

    PROCNAME("ptraDestroy");

    if (ppa == NULL) {
        L_WARNING("ptr address is NULL", procName);
        return;
    }
    if ((pa = *ppa) == NULL)
        return;

    ptraGetActualCount(pa, &nactual);
    if (nactual > 0) {
        if (freeflag) {
            for (i = 0; i <= pa->imax; i++) {
                if ((item = ptraRemove(pa, i, L_NO_COMPACTION)) != NULL)
                    FREE(item);
            }
        }
        else if (warnflag)
            L_WARNING_INT("potential memory leak of %d items in ptra",
                          procName, nactual);
    }

    FREE(pa->array);
    FREE(pa);
    *ppa = NULL;
    return;
}


/*--------------------------------------------------------------------------*
 *               Add/insert/remove/replace generic ptr object               *
 *--------------------------------------------------------------------------*/
/*!
 *  ptraAdd()
 *
 *      Input:  ptra
 *              item  (generic ptr to a struct)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This adds the element to the next location beyond imax,
 *          which is the largest occupied ptr in the array.  This is
 *          what you expect from a stack, where all ptrs up to and
 *          including imax are occupied, but here the occuption of
 *          items in the array is entirely arbitrary.
 */
l_int32
ptraAdd(L_PTRA  *pa,
        void    *item)
{
l_int32  imax;

    PROCNAME("ptraAdd");

    if (!pa)
        return ERROR_INT("pa not defined", procName, 1);
    if (!item)
        return ERROR_INT("item not defined", procName, 1);
    
    ptraGetMaxIndex(pa, &imax);
    if (imax >= pa->nalloc - 1 && ptraExtendArray(pa))
        return ERROR_INT("extension failure", procName, 1);
    pa->array[imax + 1] = (void *)item;
    pa->imax++;
    pa->nactual++;
    return 0;
}


/*!
 *  ptraExtendArray()
 *
 *      Input:  ptra 
 *      Return: 0 if OK, 1 on error
 */
l_int32
ptraExtendArray(L_PTRA  *pa)
{
    PROCNAME("ptraExtendArray");

    if (!pa)
        return ERROR_INT("pa not defined", procName, 1);

    if ((pa->array = (void **)reallocNew((void **)&pa->array,
                                sizeof(void *) * pa->nalloc,
                                2 * sizeof(void *) * pa->nalloc)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);

    pa->nalloc *= 2;
    return 0;
}


/*!
 *  ptraInsert()
 *
 *      Input:  ptra 
 *              index (location in ptra to insert new value)
 *              item  (generic ptr to a struct; can be null)
 *              shiftflag (L_AUTO_DOWNSHIFT, L_MIN_DOWNSHIFT, L_FULL_DOWNSHIFT)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This checks first to see if the location is valid, and
 *          then if there is presently an item there.  If there is not,
 *          it is simply inserted into that location.
 *      (2) If there is an item at the insert location, items must be
 *          moved down to make room for the insert.  In the downward
 *          shift there are three options, given by @shiftflag.
 *            - If @shiftflag == L_AUTO_DOWNSHIFT, a decision is made
 *              whether, in a cascade of items, to downshift a minimum
 *              amount or for all items above @index.  The decision is
 *              based on the expectation of finding holes (null ptrs)
 *              between @index and the bottom of the array.
 *              Assuming the holes are distributed uniformly, if 2 or more
 *              holes are expected, we do a minimum shift.
 *            - If @shiftflag == L_MIN_DOWNSHIFT, the downward shifting
 *              cascade of items progresses a minimum amount, until
 *              the first empty slot is reached.  This mode requires
 *              some computation before the actual shifting is done.
 *            - If @shiftflag == L_FULL_DOWNSHIFT, a shifting cascade is
 *              performed where pa[i] --> pa[i + 1] for all i >= index.
 *              Then, the item is inserted at pa[index].
 *      (3) If you are not using L_AUTO_DOWNSHIFT, the rule of thumb is
 *          to use L_FULL_DOWNSHIFT if the array is compacted (each
 *          element points to an item), and to use L_MIN_DOWNSHIFT
 *          if there are a significant number of null pointers.
 *          There is no penalty to using L_MIN_DOWNSHIFT for a
 *          compacted array, however, because the full shift is required
 *          and we don't do the O(n) computation to look for holes.
 *      (4) This should not be used repeatedly on large arrays,
 *          because the function is generally O(n).
 *      (5) However, it can be used repeatedly if we start with an empty
 *          ptr array and insert only once at each location.  For example,
 *          you can support an array of Numa, where at each ptr location
 *          you store either 0 or 1 Numa, and the Numa can be added
 *          randomly to the ptr array.
 */
l_int32
ptraInsert(L_PTRA  *pa,
           l_int32  index,
           void    *item,
           l_int32  shiftflag)
{
l_int32    i, ihole, imax;
l_float32  nexpected;

    PROCNAME("ptraInsert");

    if (!pa)
        return ERROR_INT("pa not defined", procName, 1);
    if (index < 0 || index > pa->nalloc)
        return ERROR_INT("index not in [0 ... nalloc]", procName, 1);
    if (shiftflag != L_AUTO_DOWNSHIFT && shiftflag != L_MIN_DOWNSHIFT &&
        shiftflag != L_FULL_DOWNSHIFT)
        return ERROR_INT("invalid shiftflag", procName, 1);

    if (item) pa->nactual++;
    if (index == pa->nalloc) {  /* can happen when index == n */
        if (ptraExtendArray(pa))
            return ERROR_INT("extension failure", procName, 1);
    }

        /* We are inserting into a hole or adding to the end of the array.
         * No existing items are moved. */
    ptraGetMaxIndex(pa, &imax);
    if (pa->array[index] == NULL) {
        pa->array[index] = item;
        if (item && index > imax)  /* new item put beyond max so far */
            pa->imax = index;
        return 0;
    }

        /* We are inserting at the location of an existing item,
         * forcing the existing item and those below to shift down.
         * First, extend the array automatically if the last element
         * (nalloc - 1) is occupied (imax).  This may not be necessary
         * in every situation, but only an anomalous sequence of insertions
         * into the array would cause extra ptr allocation.  */
    if (imax >= pa->nalloc - 1 && ptraExtendArray(pa))
        return ERROR_INT("extension failure", procName, 1);

        /* If there are no holes, do a full downshift.
         * Otherwise, if L_AUTO_DOWNSHIFT, use the expected number
         * of holes between index and n to determine the shift mode */
    if (imax + 1 == pa->nactual)
        shiftflag = L_FULL_DOWNSHIFT;
    else if (shiftflag == L_AUTO_DOWNSHIFT) {
        if (imax < 10)
            shiftflag = L_FULL_DOWNSHIFT;  /* no big deal */
        else {
            nexpected = (l_float32)(imax - pa->nactual) *
                         (l_float32)((imax - index) / imax);
            shiftflag = (nexpected > 2.0) ? L_MIN_DOWNSHIFT : L_FULL_DOWNSHIFT;
        }
    }

    if (shiftflag == L_MIN_DOWNSHIFT) {  /* run down looking for a hole */
        for (ihole = index + 1; ihole <= imax; ihole++) {
             if (pa->array[ihole] == NULL)
                 break;
        }
    }
    else   /* L_FULL_DOWNSHIFT */
        ihole = imax + 1;

    for (i = ihole; i > index; i--)
        pa->array[i] = pa->array[i - 1];
    pa->array[index] = (void *)item;
    if (ihole == imax + 1)  /* the last item was shifted down */
        pa->imax++;

    return 0;
}


/*!
 *  ptraGetHandle()
 *
 *      Input:  ptra
 *              index (element to be retrieved)
 *      Return: item, or null on error
 *
 *  Notes:
 *      (1) This returns a ptr to the item.  You must cast it to
 *          the type of item.  Do not destroy it; the item belongs
 *          to the Ptra.
 *      (2) This can access all possible items on the ptr array.
 *          If an item doesn't exist, it returns null.
 */
void *
ptraGetHandle(L_PTRA  *pa,
              l_int32  index)
{
    PROCNAME("ptraGetHandle");

    if (!pa)
        return (void *)ERROR_PTR("pa not defined", procName, NULL);
    if (index < 0 || index >= pa->nalloc)
        return (void *)ERROR_PTR("index not in [0 ... nalloc-1]",
                                 procName, NULL);

    return pa->array[index];
}


/*!
 *  ptraRemove()
 *
 *      Input:  ptra
 *              index (element to be removed)
 *              flag (L_NO_COMPACTION, L_COMPACTION)
 *      Return: item, or null on error
 *
 *  Notes:
 *      (1) If flag == L_NO_COMPACTION, this removes the item and
 *          nulls the ptr on the array.  If it takes the last item
 *          in the array, pa->n is reduced to the next item.
 *      (2) If flag == L_COMPACTION, this compacts the array for
 *          for all i >= index.  It should not be used repeatedly on
 *          large arrays, because compaction is O(n).
 *      (3) The ability to remove without automatic compaction allows
 *          removal with cost O(1).
 */
void *
ptraRemove(L_PTRA  *pa,
           l_int32  index,
           l_int32  flag)
{
l_int32  i, imax, fromend, icurrent;
void    *item;

    PROCNAME("ptraRemove");

    if (!pa)
        return (void *)ERROR_PTR("pa not defined", procName, NULL);
    ptraGetMaxIndex(pa, &imax);
    if (index < 0 || index > imax)
        return (void *)ERROR_PTR("index not in [0 ... imax]", procName, NULL);

    item = pa->array[index];
    if (item)
        pa->nactual--;
    pa->array[index] = NULL;
  
        /* If we took the last item, need to reduce pa->n */
    fromend = (index == imax);
    if (fromend) {
        for (i = index - 1; i >= 0; i--) {
            if (pa->array[i])
                break;
        }
        pa->imax = i;
        imax = i + 1;
    }

        /* Compact from index to the end of the array */
    if (!fromend && flag == L_COMPACTION) {
        for (icurrent = index, i = index + 1; i <= imax; i++) {
            if (pa->array[i])
                pa->array[icurrent++] = pa->array[i];
        }
        pa->imax = icurrent - 1;
    }
    return item;
}


/*!
 *  ptraRemoveLast()
 *
 *      Input:  ptra
 *      Return: item, or null on error or if the array is empty
 */
void *
ptraRemoveLast(L_PTRA  *pa)
{
l_int32  imax;

    PROCNAME("ptraRemoveLast");

    if (!pa)
        return (void *)ERROR_PTR("pa not defined", procName, NULL);

        /* Remove the last item in the array.  No compaction is required. */
    ptraGetMaxIndex(pa, &imax);
    if (imax >= 0)
        return ptraRemove(pa, imax, L_NO_COMPACTION);
    else  /* empty */
        return NULL;
}


/*!
 *  ptraReplace()
 *
 *      Input:  ptra
 *              index (element to be replaced)
 *              item  (new generic ptr to a struct; can be null)
 *              freeflag (TRUE to free old item; FALSE to return it)
 *      Return: item  (old item, if it exists and is not freed),
 *                     or null on error
 */
void *
ptraReplace(L_PTRA  *pa,
            l_int32  index,
            void    *item,
            l_int32  freeflag)
{
l_int32  imax;
void    *olditem;

    PROCNAME("ptraReplace");

    if (!pa)
        return (void *)ERROR_PTR("pa not defined", procName, NULL);
    ptraGetMaxIndex(pa, &imax);
    if (index < 0 || index > imax)
        return (void *)ERROR_PTR("index not in [0 ... imax]", procName, NULL);

    olditem = pa->array[index];
    pa->array[index] = item;
    if (!item && olditem)
        pa->nactual--;
    else if (item && !olditem)
        pa->nactual++;

    if (freeflag == FALSE)
        return olditem;

    if (olditem)
        FREE(olditem);
    return NULL;
}


/*!
 *  ptraSwap()
 *
 *      Input:  ptra
 *              index1
 *              index2
 *      Return: 0 if OK, 1 on error
 */
l_int32
ptraSwap(L_PTRA  *pa,
         l_int32  index1,
         l_int32  index2)
{
l_int32  imax;
void    *item;

    PROCNAME("ptraSwap");

    if (!pa)
        return ERROR_INT("pa not defined", procName, 1);
    if (index1 == index2)
        return 0;
    ptraGetMaxIndex(pa, &imax);
    if (index1 < 0 || index1 > imax || index2 < 0 || index2 > imax)
        return ERROR_INT("invalid index: not in [0 ... imax]", procName, 1);

    item = ptraRemove(pa, index1, L_NO_COMPACTION);
    item = ptraReplace(pa, index2, item, FALSE);
    ptraInsert(pa, index1, item, L_MIN_DOWNSHIFT);
    return 0;
}


/*!
 *  ptraCompactArray()
 *
 *      Input:  ptra 
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This compacts the items on the array, filling any empty ptrs.
 *      (2) This does not change the size of the array of ptrs.
 */
l_int32
ptraCompactArray(L_PTRA  *pa)
{
l_int32  i, imax, nactual, index;

    PROCNAME("ptraCompactArray");

    if (!pa)
        return ERROR_INT("pa not defined", procName, 1);
    ptraGetMaxIndex(pa, &imax);
    ptraGetActualCount(pa, &nactual);
    if (imax + 1 == nactual) return 0;

        /* Compact the array */
    for (i = 0, index = 0; i <= imax; i++) {
        if (pa->array[i])
             pa->array[index++] = pa->array[i];
    }
    pa->imax = index - 1;
    if (nactual != index)
        L_ERROR_INT("index = %d; != nactual", procName, index);

    return 0;
}


/*----------------------------------------------------------------------*
 *                        Other array operations                        *
 *----------------------------------------------------------------------*/
/*!
 *  ptraReverse()
 *
 *      Input:  ptra
 *      Return: 0 if OK, 1 on error
 */
l_int32
ptraReverse(L_PTRA  *pa)
{
l_int32  i, imax;

    PROCNAME("ptraReverse");

    if (!pa)
        return ERROR_INT("pa not defined", procName, 1);
    ptraGetMaxIndex(pa, &imax);

    for (i = 0; i < (imax + 1) / 2; i++)
        ptraSwap(pa, i, imax - i);
    return 0;
}


/*!
 *  ptraJoin()
 *
 *      Input:  ptra1 (add to this one)
 *              ptra2 (appended to ptra1, and emptied of items; can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
ptraJoin(L_PTRA  *pa1,
         L_PTRA  *pa2)
{
l_int32  i, imax;
void    *item;

    PROCNAME("ptraJoin");

    if (!pa1)
        return ERROR_INT("pa1 not defined", procName, 1);
    if (!pa2)
        return 0;

    ptraGetMaxIndex(pa2, &imax);
    for (i = 0; i <= imax; i++) {
        item = ptraRemove(pa2, i, L_NO_COMPACTION);
        ptraAdd(pa1, item);
    }
    
    return 0;
}



/*----------------------------------------------------------------------*
 *                        Simple ptra accessors                         *
 *----------------------------------------------------------------------*/
/*!
 *  ptraGetMaxIndex()
 *
 *      Input:  ptra
 *              &maxindex (<return> index of last item in the array);
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The largest index to an item in the array is @maxindex.
 *          @maxindex is one less than the number of items that would be
 *          in the array if there were no null pointers between 0
 *          and @maxindex - 1.  However, because the internal ptr array
 *          need not be compacted, there may be null pointers at
 *          indices below @maxindex; for example, if items have
 *          been removed.
 *      (2) When an item is added to the end of the array, it goes
 *          into pa->array[maxindex + 1], and maxindex is then
 *          incremented by 1.
 *      (3) If there are no items in the array, this returns @maxindex = -1.
 */
l_int32
ptraGetMaxIndex(L_PTRA   *pa,
                l_int32  *pmaxindex)
{
    PROCNAME("ptraGetMaxIndex");

    if (!pa)
        return ERROR_INT("pa not defined", procName, 1);
    if (!pmaxindex)
        return ERROR_INT("&maxindex not defined", procName, 1);
    *pmaxindex = pa->imax;
    return 0;
}
        

/*!
 *  ptraGetActualCount()
 *
 *      Input:  ptra
 *              &count (<return> actual number of items on the ptr array)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The actual number of items on the ptr array, pa->nactual,
 *          will be smaller than pa->n if the array is not compacted.
 */
l_int32
ptraGetActualCount(L_PTRA   *pa,
                   l_int32  *pcount)
{
    PROCNAME("ptraGetActualCount");

    if (!pa)
        return ERROR_INT("pa not defined", procName, 1);
    if (!pcount)
        return ERROR_INT("&count not defined", procName, 1);
    *pcount = pa->nactual;

    return 0;
}


/*!
 *  ptraGetPtrToItem()
 *
 *      Input:  ptra
 *              index (element to fetch pointer to)
 *      Return: item (just a pointer to it)
 *
 *  Notes:
 *      (1) The item remains on the Ptra and is 'owned' by it, so
 *          the item must not be destroyed.
 */
void *
ptraGetPtrToItem(L_PTRA  *pa,
                 l_int32  index)
{
    PROCNAME("ptraGetPtrToItem");

    if (!pa)
        return (void *)ERROR_PTR("pa not defined", procName, NULL);
    if (index < 0 || index > pa->imax)
        return (void *)ERROR_PTR("index not in [0 ... imax]", procName, NULL);

    return pa->array[index];
}


/*--------------------------------------------------------------------------*
 *                      Ptraa creation and destruction                      *
 *--------------------------------------------------------------------------*/
/*!
 *  ptraaCreate()
 *
 *      Input:  size of ptr array to be alloc'd
 *      Return: paa, or null on error
 *
 *  Notes:
 *      (1) The ptraa is generated with a fixed size, that can not change.
 *          The ptra can be generated and inserted randomly into this array.
 */
L_PTRAA *
ptraaCreate(l_int32  n)
{
L_PTRAA  *paa;

    PROCNAME("ptraaCreate");

    if (n <= 0)
        return (L_PTRAA *)ERROR_PTR("n must be > 0", procName, NULL);

    if ((paa = (L_PTRAA *)CALLOC(1, sizeof(L_PTRAA))) == NULL)
        return (L_PTRAA *)ERROR_PTR("paa not made", procName, NULL);
    if ((paa->ptra = (L_PTRA **)CALLOC(n, sizeof(L_PTRA *))) == NULL)
        return (L_PTRAA *)ERROR_PTR("ptr array not made", procName, NULL);

    paa->nalloc = n;
    return paa;
}


/*!
 *  ptraaDestroy()
 *
 *      Input:  &paa (<to be nulled>)
 *              freeflag (TRUE to free each remaining item in each ptra)
 *              warnflag (TRUE to warn if any remaining items are not destroyed)
 *      Return: void
 *
 *  Notes:
 *      (1) See ptraDestroy() for use of @freeflag and @warnflag.
 *      (2) To destroy the ptraa, we destroy each ptra, then the ptr array,
 *          then the ptraa, and then null the contents of the input ptr.
 */
void
ptraaDestroy(L_PTRAA  **ppaa,
             l_int32    freeflag,
             l_int32    warnflag)
{
l_int32   i, n;
L_PTRA   *pa;
L_PTRAA  *paa;

    PROCNAME("ptraaDestroy");

    if (ppaa == NULL) {
        L_WARNING("ptr address is NULL", procName);
        return;
    }
    if ((paa = *ppaa) == NULL)
        return;

    ptraaGetSize(paa, &n);
    for (i = 0; i < n; i++) {
        pa = ptraaGetPtra(paa, i, L_REMOVE);
        ptraDestroy(&pa, freeflag, warnflag);
    }

    FREE(paa->ptra);
    FREE(paa);
    *ppaa = NULL;
    return;
}


/*--------------------------------------------------------------------------*
 *                             Ptraa accessors                              *
 *--------------------------------------------------------------------------*/
/*!
 *  ptraaGetSize()
 *
 *      Input:  ptraa
 *              &size (<return> size of ptr array)
 *      Return: 0 if OK; 1 on error
 */
l_int32
ptraaGetSize(L_PTRAA  *paa,
             l_int32  *psize)
{
    PROCNAME("ptraaGetSize");

    if (!paa)
        return ERROR_INT("paa not defined", procName, 1);
    if (!psize)
        return ERROR_INT("&size not defined", procName, 1);
    *psize = paa->nalloc;

    return 0;
}


/*!
 *  ptraaInsertPtra()
 *
 *      Input:  ptraa
 *              index (location in array for insertion)
 *              ptra (to be inserted)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Caller should check return value.  On success, the Ptra
 *          is inserted in the Ptraa and is owned by it.  However,
 *          on error, the Ptra remains owned by the caller.
 */
l_int32
ptraaInsertPtra(L_PTRAA  *paa,
                l_int32   index,
                L_PTRA   *pa)
{
l_int32  n;

    PROCNAME("ptraaInsertPtra");

    if (!paa)
        return ERROR_INT("paa not defined", procName, 1);
    if (!pa)
        return ERROR_INT("pa not defined", procName, 1);
    ptraaGetSize(paa, &n);
    if (index < 0 || index >= n)
        return ERROR_INT("invalid index", procName, 1);
    if (paa->ptra[index] != NULL)
        return ERROR_INT("ptra alread stored at index", procName, 1);

    paa->ptra[index] = pa;
    return 0;
}


/*!
 *  ptraaGetPtra()
 *
 *      Input:  ptraa
 *              index (location in array)
 *              accessflag (L_HANDLE_ONLY, L_REMOVE)
 *      Return: ptra (at index location), or NULL on error or if there
 *              is no ptra there.
 *
 *  Notes:
 *      (1) This returns the ptra ptr.  If @accessflag == L_HANDLE_ONLY,
 *          the ptra is left on the ptraa.  If @accessflag == L_REMOVE,
 *          the ptr in the ptraa is set to NULL, and the caller
 *          is responsible for disposing of the ptra (either putting it
 *          back on the ptraa, or destroying it).
 *      (2) This returns NULL if there is no Ptra at the index location.
 */
L_PTRA *
ptraaGetPtra(L_PTRAA  *paa,
             l_int32   index,
             l_int32   accessflag)
{
l_int32  n;
L_PTRA  *pa;

    PROCNAME("ptraaGetPtra");

    if (!paa)
        return (L_PTRA *)ERROR_PTR("paa not defined", procName, NULL);
    ptraaGetSize(paa, &n);
    if (index < 0 || index >= n)
        return (L_PTRA *)ERROR_PTR("invalid index", procName, NULL);
    if (accessflag != L_HANDLE_ONLY && accessflag != L_REMOVE)
        return (L_PTRA *)ERROR_PTR("invalid accessflag", procName, NULL);

    pa = paa->ptra[index];
    if (accessflag == L_REMOVE)
        paa->ptra[index] = NULL;
    return pa;
}


/*--------------------------------------------------------------------------*
 *                             Ptraa conversion                             *
 *--------------------------------------------------------------------------*/
/*!
 *  ptraaFlattenToPtra()
 *
 *      Input:  ptraa
 *      Return: ptra, or null on error
 *
 *  Notes:
 *      (1) This 'flattens' the ptraa to a ptra, taking the items in
 *          each ptra, in order, starting with the first ptra, etc.
 *      (2) As a side-effect, the ptra are all removed from the ptraa
 *          and destroyed, leaving an empty ptraa.
 */
L_PTRA *
ptraaFlattenToPtra(L_PTRAA  *paa)
{
l_int32  i, n;
L_PTRA    *pat, *pad;

    PROCNAME("ptraaFlattenToPtra");

    if (!paa)
        return (L_PTRA *)ERROR_PTR("paa not defined", procName, NULL);

    pad = ptraCreate(0);
    ptraaGetSize(paa, &n);
    for (i = 0; i < n; i++) {
        pat = ptraaGetPtra(paa, i, L_REMOVE);
        if (!pat) continue;
        ptraJoin(pad, pat);
        ptraDestroy(&pat, FALSE, FALSE);  /* they're all empty */
    }

    return pad;
}


/*--------------------------------------------------------------------------*
 *                          Functions using L_PTRA                          *
 *--------------------------------------------------------------------------*/
/*!
 *  numaGetBinSortIndex()
 *
 *      Input:  na (of non-negative integers with a max that is typically
 *                  less than 50,000)
 *              sortorder (L_SORT_INCREASING or L_SORT_DECREASING)
 *      Return: na (sorted), or null on error
 *
 *  Notes:
 *      (1) This creates an array (or lookup table) that gives the
 *          sorted position of the elements in the input Numa.
 *      (2) Because it uses a bin sort with buckets of size 1, it
 *          is not appropriate for sorting either small arrays or
 *          arrays containing very large integer values.  For such
 *          arrays, use a standard general sort function like
 *          numaGetSortIndex().
 */
NUMA *
numaGetBinSortIndex(NUMA    *nas,
                    l_int32  sortorder)
{
l_int32    i, n, isize, ival, imax;
l_float32  size;
NUMA      *na, *nai, *nad;
L_PTRA    *paindex;

    PROCNAME("numaGetBinSortIndex");

    if (!nas)
        return (NUMA *)ERROR_PTR("nas not defined", procName, NULL);
    if (sortorder != L_SORT_INCREASING && sortorder != L_SORT_DECREASING)
        return (NUMA *)ERROR_PTR("invalid sort order", procName, NULL);

        /* Set up a ptra holding numa at indices for which there
         * are values in nas.  This effectively sorts the input
         * numbers. */
    numaGetMax(nas, &size, NULL);
    isize = (l_int32)size;
    if (isize > 50000)
        L_WARNING_INT("large array: %d elements", procName, isize);
    paindex = ptraCreate(isize + 1);
    n = numaGetCount(nas);
    for (i = 0; i < n; i++) {
        numaGetIValue(nas, i, &ival);
        nai = (NUMA *)ptraGetHandle(paindex, ival);
        if (!nai) {  /* make it; no shifting will occur */
            nai = numaCreate(1);
            ptraInsert(paindex, ival, nai, L_MIN_DOWNSHIFT);
        }
        numaAddNumber(nai, i);
    }

        /* Sort by pulling the numbers out of the numas, taken
         * successively in requested order. */
    ptraGetMaxIndex(paindex, &imax);
    nad = numaCreate(0);
    if (sortorder == L_SORT_INCREASING) {
        for (i = 0; i <= imax; i++) {
            na = (NUMA *)ptraRemove(paindex, i, L_NO_COMPACTION);
            numaJoin(nad, na, 0, 0);
            numaDestroy(&na);
        }
    } else {  /* L_SORT_DECREASING */
        for (i = imax; i >= 0; i--) {
            na = (NUMA *)ptraRemove(paindex, i, L_NO_COMPACTION);
            numaJoin(nad, na, 0, 0);
            numaDestroy(&na);
        }
    }

    ptraDestroy(&paindex, FALSE, FALSE);
    return nad;
}


