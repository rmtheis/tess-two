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
 *   numabasic.c
 *
 *      Numa creation, destruction, copy, clone, etc.
 *          NUMA        *numaCreate()
 *          NUMA        *numaCreateFromIArray()
 *          NUMA        *numaCreateFromFArray()
 *          void        *numaDestroy()
 *          NUMA        *numaCopy()
 *          NUMA        *numaClone()
 *          l_int32      numaEmpty()
 *
 *      Add/remove number (float or integer)
 *          l_int32      numaAddNumber()
 *          l_int32      numaExtendArray()
 *          l_int32      numaInsertNumber()
 *          l_int32      numaRemoveNumber()
 *          l_int32      numaReplaceNumber()
 *
 *      Numa accessors
 *          l_int32      numaGetCount()
 *          l_int32      numaSetCount()
 *          l_int32      numaGetIValue()
 *          l_int32      numaGetFValue()
 *          l_int32      numaSetValue()
 *          l_int32      numaShiftValue()
 *          l_int32     *numaGetIArray()
 *          l_float32   *numaGetFArray()
 *          l_int32      numaGetRefcount()
 *          l_int32      numaChangeRefcount()
 *          l_int32      numaGetXParameters()
 *          l_int32      numaSetXParameters()
 *          l_int32      numaCopyXParameters()
 *
 *      Serialize numa for I/O
 *          l_int32      numaRead()
 *          l_int32      numaReadStream()
 *          l_int32      numaWrite()
 *          l_int32      numaWriteStream()
 *
 *      Numaa creation, destruction
 *          NUMAA       *numaaCreate()
 *          void        *numaaDestroy()
 *
 *      Add Numa to Numaa
 *          l_int32      numaaAddNuma()
 *          l_int32      numaaExtendArray()
 *
 *      Numaa accessors
 *          l_int32      numaaGetCount()
 *          l_int32      numaaGetNumberCount()
 *          NUMA       **numaaGetPtrArray()
 *          NUMA        *numaaGetNuma()
 *          NUMA        *numaaReplaceNuma()
 *          l_int32      numaaAddNumber()
 *
 *      Serialize numaa for I/O
 *          l_int32      numaaRead()
 *          l_int32      numaaReadStream()
 *          l_int32      numaaWrite()
 *          l_int32      numaaWriteStream()
 *
 *      Numa2d creation, destruction
 *          NUMA2D      *numa2dCreate()
 *          void        *numa2dDestroy()
 *
 *      Numa2d Accessors
 *          l_int32      numa2dAddNumber()
 *          l_int32      numa2dGetCount()
 *          NUMA        *numa2dGetNuma()
 *          l_int32      numa2dGetFValue()
 *          l_int32      numa2dGetIValue()
 *
 *      NumaHash creation, destruction
 *          NUMAHASH    *numaHashCreate()
 *          void        *numaHashDestroy()
 *
 *      NumaHash Accessors
 *          NUMA        *numaHashGetNuma()
 *          void        *numaHashAdd()
 *
 *    (1) The numa is a struct, not an array.  Always use the accessors
 *        in this file, never the fields directly.
 *
 *    (2) The number array holds l_float32 values.  It can also
 *        be used to store l_int32 values.
 *
 *    (3) Storing and retrieving numbers:
 *
 *       * to append a new number to the array, use numaAddNumber().  If
 *         the number is an int, it will will automatically be converted
 *         to l_float32 and stored.
 *
 *       * to reset a value stored in the array, use numaSetValue().
 *
 *       * to increment or decrement a value stored in the array,
 *         use numaShiftValue().
 *
 *       * to obtain a value from the array, use either numaGetIValue()
 *         or numaGetFValue(), depending on whether you are retrieving
 *         an integer or a float.  This avoids doing an explicit cast,
 *         such as
 *           (a) return a l_float32 and cast it to an l_int32
 *           (b) cast the return directly to (l_float32 *) to
 *               satisfy the function prototype, as in
 *                 numaGetFValue(na, index, (l_float32 *)&ival);   [ugly!]
 *
 *    (4) int <--> float conversions:
 *
 *        Tradition dictates that type conversions go automatically from
 *        l_int32 --> l_float32, even though it is possible to lose
 *        precision for large integers, whereas you must cast (l_int32)
 *        to go from l_float32 --> l_int32 because you're truncating
 *        to the integer value.
 *
 *    (5) As with other arrays in leptonica, the numa has both an allocated
 *        size and a count of the stored numbers.  When you add a number, it
 *        goes on the end of the array, and causes a realloc if the array
 *        is already filled.  However, in situations where you want to
 *        add numbers randomly into an array, such as when you build a
 *        histogram, you must set the count of stored numbers in advance.
 *        This is done with numaSetCount().  If you set a count larger
 *        than the allocated array, it does a realloc to the size requested.
 *
 *    (6) In situations where the data in a numa correspond to a function
 *        y(x), the values can be either at equal spacings in x or at
 *        arbitrary spacings.  For the former, we can represent all x values
 *        by two parameters: startx (corresponding to y[0]) and delx
 *        for the change in x for adjacent values y[i] and y[i+1].
 *        startx and delx are initialized to 0.0 and 1.0, rsp.
 *        For arbitrary spacings, we use a second numa, and the two
 *        numas are typically denoted nay and nax.
 *
 *    (7) The numa is also the basic struct used for histograms.  Every numa
 *        has startx and delx fields, initialized to 0.0 and 1.0, that can
 *        be used to represent the "x" value for the location of the
 *        first bin and the bin width, respectively.  Accessors are the
 *        numa*XParameters() functions.  All functions that make numa
 *        histograms must set these fields properly, and many functions
 *        that use numa histograms rely on the correctness of these values.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "allheaders.h"

static const l_int32 INITIAL_PTR_ARRAYSIZE = 50;      /* n'importe quoi */


/*--------------------------------------------------------------------------*
 *               Numa creation, destruction, copy, clone, etc.              *
 *--------------------------------------------------------------------------*/
/*!
 *  numaCreate()
 *
 *      Input:  size of number array to be alloc'd (0 for default)
 *      Return: na, or null on error
 */
NUMA *
numaCreate(l_int32  n)
{
NUMA  *na;

    PROCNAME("numaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((na = (NUMA *)CALLOC(1, sizeof(NUMA))) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);
    if ((na->array = (l_float32 *)CALLOC(n, sizeof(l_float32))) == NULL)
        return (NUMA *)ERROR_PTR("number array not made", procName, NULL);

    na->nalloc = n;
    na->n = 0;
    na->refcount = 1;
    na->startx = 0.0;
    na->delx = 1.0;

    return na;
}


/*!
 *  numaCreateFromIArray()
 *
 *      Input:  iarray (integer)
 *              size (of the array)
 *      Return: na, or null on error
 *
 *  Notes:
 *      (1) We can't insert this int array into the numa, because a numa
 *          takes a float array.  So this just copies the data from the
 *          input array into the numa.  The input array continues to be
 *          owned by the caller.
 */
NUMA *
numaCreateFromIArray(l_int32  *iarray,
                     l_int32   size)
{
l_int32  i;
NUMA    *na;

    PROCNAME("numaCreateFromIArray");

    if (!iarray)
        return (NUMA *)ERROR_PTR("iarray not defined", procName, NULL);
    if (size <= 0)
        return (NUMA *)ERROR_PTR("size must be > 0", procName, NULL);

    na = numaCreate(size);
    for (i = 0; i < size; i++)
        numaAddNumber(na, iarray[i]);

    return na;
}


/*!
 *  numaCreateFromFArray()
 *
 *      Input:  farray (float)
 *              size (of the array)
 *              copyflag (L_INSERT or L_COPY)
 *      Return: na, or null on error
 *
 *  Notes:
 *      (1) With L_INSERT, ownership of the input array is transferred
 *          to the returned numa.
 */
NUMA *
numaCreateFromFArray(l_float32  *farray,
                     l_int32     size,
                     l_int32     copyflag)
{
l_int32  i;
NUMA    *na;

    PROCNAME("numaCreateFromFArray");

    if (!farray)
        return (NUMA *)ERROR_PTR("farray not defined", procName, NULL);
    if (size <= 0)
        return (NUMA *)ERROR_PTR("size must be > 0", procName, NULL);
    if (copyflag != L_INSERT && copyflag != L_COPY)
        return (NUMA *)ERROR_PTR("invalid copyflag", procName, NULL);

    na = numaCreate(size);
    if (copyflag == L_INSERT) {
        if (na->array) FREE(na->array);
        na->array = farray;
    }
    else {  /* just copy the contents */
        for (i = 0; i < size; i++)
            numaAddNumber(na, farray[i]);
    }

    return na;
}


/*!
 *  numaDestroy()
 *
 *      Input:  &na (<to be nulled if it exists>)
 *      Return: void
 *
 *  Notes:
 *      (1) Decrements the ref count and, if 0, destroys the numa.
 *      (2) Always nulls the input ptr.
 */
void
numaDestroy(NUMA  **pna)
{
NUMA  *na;

    PROCNAME("numaDestroy");

    if (pna == NULL) {
        L_WARNING("ptr address is NULL", procName);
        return;
    }

    if ((na = *pna) == NULL)
        return;

        /* Decrement the ref count.  If it is 0, destroy the numa. */
    numaChangeRefcount(na, -1);
    if (numaGetRefcount(na) <= 0) {
        if (na->array)
            FREE(na->array);
        FREE(na);
    }

    *pna = NULL;
    return;
}


/*!
 *  numaCopy()
 *
 *      Input:  na
 *      Return: copy of numa, or null on error
 */
NUMA *
numaCopy(NUMA  *na)
{
l_int32  i;
NUMA    *cna;

    PROCNAME("numaCopy");

    if (!na)
        return (NUMA *)ERROR_PTR("na not defined", procName, NULL);

    if ((cna = numaCreate(na->nalloc)) == NULL)
        return (NUMA *)ERROR_PTR("cna not made", procName, NULL);
    cna->startx = na->startx;
    cna->delx = na->delx;

    for (i = 0; i < na->n; i++)
        numaAddNumber(cna, na->array[i]);

    return cna;
}


/*!
 *  numaClone()
 *
 *      Input:  na
 *      Return: ptr to same numa, or null on error
 */
NUMA *
numaClone(NUMA  *na)
{
    PROCNAME("numaClone");

    if (!na)
        return (NUMA *)ERROR_PTR("na not defined", procName, NULL);

    numaChangeRefcount(na, 1);
    return na;
}


/*!
 *  numaEmpty()
 *
 *      Input:  na
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This does not change the allocation of the array.
 *          It just clears the number of stored numbers, so that
 *          the array appears to be empty.
 */
l_int32
numaEmpty(NUMA  *na)
{
    PROCNAME("numaEmpty");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    na->n = 0;
    return 0;
}



/*--------------------------------------------------------------------------*
 *                 Number array: add number and extend array                *
 *--------------------------------------------------------------------------*/
/*!
 *  numaAddNumber()
 *
 *      Input:  na
 *              val  (float or int to be added; stored as a float)
 *      Return: 0 if OK, 1 on error
 */
l_int32
numaAddNumber(NUMA      *na,
              l_float32  val)
{
l_int32  n;

    PROCNAME("numaAddNumber");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    n = numaGetCount(na);
    if (n >= na->nalloc)
        numaExtendArray(na);
    na->array[n] = val;
    na->n++;
    return 0;
}


/*!
 *  numaExtendArray()
 *
 *      Input:  na
 *      Return: 0 if OK, 1 on error
 */
l_int32
numaExtendArray(NUMA  *na)
{
    PROCNAME("numaExtendArray");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    if ((na->array = (l_float32 *)reallocNew((void **)&na->array,
                                sizeof(l_float32) * na->nalloc,
                                2 * sizeof(l_float32) * na->nalloc)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);

    na->nalloc *= 2;
    return 0;
}


/*!
 *  numaInsertNumber()
 *
 *      Input:  na
 *              index (location in na to insert new value)
 *              val  (float32 or integer to be added)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This shifts na[i] --> na[i + 1] for all i >= index,
 *          and then inserts val as na[index].
 *      (2) It should not be used repeatedly on large arrays,
 *          because the function is O(n).
 *
 */
l_int32
numaInsertNumber(NUMA      *na,
                 l_int32    index,
                 l_float32  val)
{
l_int32  i, n;

    PROCNAME("numaInsertNumber");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);
    n = numaGetCount(na);
    if (index < 0 || index > n)
        return ERROR_INT("index not in {0...n}", procName, 1);

    if (n >= na->nalloc)
        numaExtendArray(na);
    for (i = n; i > index; i--)
        na->array[i] = na->array[i - 1];
    na->array[index] = val;
    na->n++;
    return 0;
}


/*!
 *  numaRemoveNumber()
 *
 *      Input:  na
 *              index (element to be removed)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This shifts na[i] --> na[i - 1] for all i > index.
 *      (2) It should not be used repeatedly on large arrays,
 *          because the function is O(n).
 */
l_int32
numaRemoveNumber(NUMA    *na,
                 l_int32  index)
{
l_int32  i, n;

    PROCNAME("numaRemoveNumber");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);
    n = numaGetCount(na);
    if (index < 0 || index >= n)
        return ERROR_INT("index not in {0...n - 1}", procName, 1);

    for (i = index + 1; i < n; i++)
        na->array[i - 1] = na->array[i];
    na->n--;
    return 0;
}


/*!
 *  numaReplaceNumber()
 *
 *      Input:  na
 *              index (element to be replaced)
 *              val (new value to replace old one)
 *      Return: 0 if OK, 1 on error
 */
l_int32
numaReplaceNumber(NUMA      *na,
                  l_int32    index,
                  l_float32  val)
{
l_int32  n;

    PROCNAME("numaReplaceNumber");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);
    n = numaGetCount(na);
    if (index < 0 || index >= n)
        return ERROR_INT("index not in {0...n - 1}", procName, 1);

    na->array[index] = val;
    return 0;
}


/*----------------------------------------------------------------------*
 *                            Numa accessors                            *
 *----------------------------------------------------------------------*/
/*!
 *  numaGetCount()
 *
 *      Input:  na
 *      Return: count, or 0 if no numbers or on error
 */
l_int32
numaGetCount(NUMA  *na)
{
    PROCNAME("numaGetCount");

    if (!na)
        return ERROR_INT("na not defined", procName, 0);
    return na->n;
}


/*!
 *  numaSetCount()
 *
 *      Input:  na
 *              newcount
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If newcount <= na->nalloc, this resets na->n.
 *          Using newcount = 0 is equivalent to numaEmpty().
 *      (2) If newcount > na->nalloc, this causes a realloc
 *          to a size na->nalloc = newcount.
 *      (3) All the previously unused values in na are set to 0.0.
 */
l_int32
numaSetCount(NUMA    *na,
             l_int32  newcount)
{
    PROCNAME("numaSetCount");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);
    if (newcount > na->nalloc) {
        if ((na->array = (l_float32 *)reallocNew((void **)&na->array,
                         sizeof(l_float32) * na->nalloc,
                         sizeof(l_float32) * newcount)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);
        na->nalloc = newcount;
    }
    na->n = newcount;
    return 0;
}


/*!
 *  numaGetFValue()
 *
 *      Input:  na
 *              index (into numa)
 *              &val  (<return> float value; 0.0 on error)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Caller may need to check the function return value to
 *          decide if a 0.0 in the returned ival is valid.
 */
l_int32
numaGetFValue(NUMA       *na,
              l_int32     index,
              l_float32  *pval)
{
    PROCNAME("numaGetFValue");

    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = 0.0;
    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    if (index < 0 || index >= na->n)
        return ERROR_INT("index not valid", procName, 1);

    *pval = na->array[index];
    return 0;
}


/*!
 *  numaGetIValue()
 *
 *      Input:  na
 *              index (into numa)
 *              &ival  (<return> integer value; 0 on error)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Caller may need to check the function return value to
 *          decide if a 0 in the returned ival is valid.
 */
l_int32
numaGetIValue(NUMA     *na,
              l_int32   index,
              l_int32  *pival)
{
l_float32  val;

    PROCNAME("numaGetIValue");

    if (!pival)
        return ERROR_INT("&ival not defined", procName, 1);
    *pival = 0;
    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    if (index < 0 || index >= na->n)
        return ERROR_INT("index not valid", procName, 1);

    val = na->array[index];
    *pival = (l_int32)(val + L_SIGN(val) * 0.5);
    return 0;
}


/*!
 *  numaSetValue()
 *
 *      Input:  na
 *              index   (to element to be set)
 *              val  (to set element)
 *      Return: 0 if OK; 1 on error
 */
l_int32
numaSetValue(NUMA      *na,
             l_int32    index,
             l_float32  val)
{
    PROCNAME("numaSetValue");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);
    if (index < 0 || index >= na->n)
        return ERROR_INT("index not valid", procName, 1);

    na->array[index] = val;
    return 0;
}


/*!
 *  numaShiftValue()
 *
 *      Input:  na
 *              index (to element to change relative to the current value)
 *              diff  (increment if diff > 0 or decrement if diff < 0)
 *      Return: 0 if OK; 1 on error
 */
l_int32
numaShiftValue(NUMA      *na,
               l_int32    index,
               l_float32  diff)
{
    PROCNAME("numaShiftValue");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);
    if (index < 0 || index >= na->n)
        return ERROR_INT("index not valid", procName, 1);

    na->array[index] += diff;
    return 0;
}


/*!
 *  numaGetIArray()
 *
 *      Input:  na
 *      Return: a copy of the bare internal array, integerized
 *              by rounding, or null on error
 *  Notes:
 *      (1) A copy of the array is always made, because we need to
 *          generate an integer array from the bare float array.
 *          The caller is responsible for freeing the array.
 *      (2) The array size is determined by the number of stored numbers,
 *          not by the size of the allocated array in the Numa.
 *      (3) This function is provided to simplify calculations
 *          using the bare internal array, rather than continually
 *          calling accessors on the numa.  It is typically used
 *          on an array of size 256.
 */
l_int32 *
numaGetIArray(NUMA  *na)
{
l_int32   i, n, ival;
l_int32  *array;

    PROCNAME("numaGetIArray");

    if (!na)
        return (l_int32 *)ERROR_PTR("na not defined", procName, NULL);

    n = numaGetCount(na);
    if ((array = (l_int32 *)CALLOC(n, sizeof(l_int32))) == NULL)
        return (l_int32 *)ERROR_PTR("array not made", procName, NULL);
    for (i = 0; i < n; i++) {
        numaGetIValue(na, i, &ival);
        array[i] = ival;
    }

    return array;
}


/*!
 *  numaGetFArray()
 *
 *      Input:  na
 *              copyflag (L_NOCOPY or L_COPY)
 *      Return: either the bare internal array or a copy of it,
 *              or null on error
 *
 *  Notes:
 *      (1) If copyflag == L_COPY, it makes a copy which the caller
 *          is responsible for freeing.  Otherwise, it operates
 *          directly on the bare array of the numa.
 *      (2) Very important: for L_NOCOPY, any writes to the array
 *          will be in the numa.  Do not write beyond the size of
 *          the count field, because it will not be accessable
 *          from the numa!  If necessary, be sure to set the count
 *          field to a larger number (such as the alloc size)
 *          BEFORE calling this function.  Creating with numaMakeConstant()
 *          is another way to insure full initialization.
 */
l_float32 *
numaGetFArray(NUMA    *na,
              l_int32  copyflag)
{
l_int32     i, n;
l_float32  *array;

    PROCNAME("numaGetFArray");

    if (!na)
        return (l_float32 *)ERROR_PTR("na not defined", procName, NULL);

    if (copyflag == L_NOCOPY)
        array = na->array;
    else {  /* copyflag == L_COPY */
        n = numaGetCount(na);
        if ((array = (l_float32 *)CALLOC(n, sizeof(l_float32))) == NULL)
            return (l_float32 *)ERROR_PTR("array not made", procName, NULL);
        for (i = 0; i < n; i++)
            array[i] = na->array[i];
    }

    return array;
}


/*!
 *  numaGetRefCount()
 *
 *      Input:  na
 *      Return: refcount, or UNDEF on error
 */
l_int32
numaGetRefcount(NUMA  *na)
{
    PROCNAME("numaGetRefcount");

    if (!na)
        return ERROR_INT("na not defined", procName, UNDEF);
    return na->refcount;
}


/*!
 *  numaChangeRefCount()
 *
 *      Input:  na
 *              delta (change to be applied)
 *      Return: 0 if OK, 1 on error
 */
l_int32
numaChangeRefcount(NUMA    *na,
                   l_int32  delta)
{
    PROCNAME("numaChangeRefcount");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);
    na->refcount += delta;
    return 0;
}


/*!
 *  numaGetXParameters()
 *
 *      Input:  na
 *              &startx (<optional return> startx)
 *              &delx (<optional return> delx)
 *      Return: 0 if OK, 1 on error
 */
l_int32
numaGetXParameters(NUMA       *na,
                   l_float32  *pstartx,
                   l_float32  *pdelx)
{
    PROCNAME("numaGetXParameters");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    if (pstartx) *pstartx = na->startx;
    if (pdelx) *pdelx = na->delx;
    return 0;
}


/*!
 *  numaSetXParameters()
 *
 *      Input:  na
 *              startx (x value corresponding to na[0])
 *              delx (difference in x values for the situation where the
 *                    elements of na correspond to the evaulation of a
 *                    function at equal intervals of size @delx)
 *      Return: 0 if OK, 1 on error
 */
l_int32
numaSetXParameters(NUMA      *na,
                   l_float32  startx,
                   l_float32  delx)
{
    PROCNAME("numaSetXParameters");

    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    na->startx = startx;
    na->delx = delx;
    return 0;
}


/*!
 *  numaCopyXParameters()
 *
 *      Input:  nad (destination Numa)
 *              nas (source Numa)
 *      Return: 0 if OK, 1 on error
 */
l_int32
numaCopyXParameters(NUMA  *nad,
                    NUMA  *nas)
{
l_float32  start, binsize;

    PROCNAME("numaCopyXParameters");

    if (!nas || !nad)
        return ERROR_INT("nas and nad not both defined", procName, 1);

    numaGetXParameters(nas, &start, &binsize);
    numaSetXParameters(nad, start, binsize);
    return 0;
}


/*----------------------------------------------------------------------*
 *                       Serialize numa for I/O                         *
 *----------------------------------------------------------------------*/
/*!
 *  numaRead()
 *
 *      Input:  filename
 *      Return: na, or null on error
 */
NUMA *
numaRead(const char  *filename)
{
FILE  *fp;
NUMA  *na;

    PROCNAME("numaRead");

    if (!filename)
        return (NUMA *)ERROR_PTR("filename not defined", procName, NULL);

    if ((fp = fopenReadStream(filename)) == NULL)
        return (NUMA *)ERROR_PTR("stream not opened", procName, NULL);

    if ((na = numaReadStream(fp)) == NULL) {
        fclose(fp);
        return (NUMA *)ERROR_PTR("na not read", procName, NULL);
    }

    fclose(fp);
    return na;
}


/*!
 *  numaReadStream()
 *
 *      Input:  stream
 *      Return: numa, or null on error
 */
NUMA *
numaReadStream(FILE  *fp)
{
l_int32    i, n, index, ret, version;
l_float32  val, startx, delx;
NUMA      *na;

    PROCNAME("numaReadStream");

    if (!fp)
        return (NUMA *)ERROR_PTR("stream not defined", procName, NULL);

    ret = fscanf(fp, "\nNuma Version %d\n", &version);
    if (ret != 1)
        return (NUMA *)ERROR_PTR("not a numa file", procName, NULL);
    if (version != NUMA_VERSION_NUMBER)
        return (NUMA *)ERROR_PTR("invalid numa version", procName, NULL);
    fscanf(fp, "Number of numbers = %d\n", &n);

    if ((na = numaCreate(n)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);

    for (i = 0; i < n; i++) {
        if ((fscanf(fp, "  [%d] = %f\n", &index, &val)) != 2)
            return (NUMA *)ERROR_PTR("bad input data", procName, NULL);
        numaAddNumber(na, val);
    }

        /* Optional data */
    if ((fscanf(fp, "startx = %f, delx = %f\n", &startx, &delx)) == 2)
        numaSetXParameters(na, startx, delx);

    return na;
}


/*!
 *  numaWrite()
 *
 *      Input:  filename, na
 *      Return: 0 if OK, 1 on error
 */
l_int32
numaWrite(const char  *filename,
          NUMA        *na)
{
FILE  *fp;

    PROCNAME("numaWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    if ((fp = fopen(filename, "w")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (numaWriteStream(fp, na))
        return ERROR_INT("na not written to stream", procName, 1);
    fclose(fp);

    return 0;
}


/*!
 *  numaWriteStream()
 *
 *      Input:  stream, na
 *      Return: 0 if OK, 1 on error
 */
l_int32
numaWriteStream(FILE  *fp,
                NUMA  *na)
{
l_int32    i, n;
l_float32  startx, delx;

    PROCNAME("numaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    n = numaGetCount(na);
    fprintf(fp, "\nNuma Version %d\n", NUMA_VERSION_NUMBER);
    fprintf(fp, "Number of numbers = %d\n", n);
    for (i = 0; i < n; i++)
        fprintf(fp, "  [%d] = %f\n", i, na->array[i]);
    fprintf(fp, "\n");

        /* Optional data */
    numaGetXParameters(na, &startx, &delx);
    if (startx != 0.0 || delx != 1.0)
        fprintf(fp, "startx = %f, delx = %f\n", startx, delx);

    return 0;
}



/*--------------------------------------------------------------------------*
 *                     Numaa creation, destruction                          *
 *--------------------------------------------------------------------------*/
/*!
 *  numaaCreate()
 *
 *      Input:  size of numa ptr array to be alloc'd (0 for default)
 *      Return: naa, or null on error
 *
 */
NUMAA *
numaaCreate(l_int32  n)
{
NUMAA  *naa;

    PROCNAME("numaaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((naa = (NUMAA *)CALLOC(1, sizeof(NUMAA))) == NULL)
        return (NUMAA *)ERROR_PTR("naa not made", procName, NULL);
    if ((naa->numa = (NUMA **)CALLOC(n, sizeof(NUMA *))) == NULL)
        return (NUMAA *)ERROR_PTR("numa ptr array not made", procName, NULL);

    naa->nalloc = n;
    naa->n = 0;

    return naa;
}


/*!
 *  numaaDestroy()
 *
 *      Input: &numaa <to be nulled if it exists>
 *      Return: void
 */
void
numaaDestroy(NUMAA  **pnaa)
{
l_int32  i;
NUMAA   *naa;

    PROCNAME("numaaDestroy");

    if (pnaa == NULL) {
        L_WARNING("ptr address is NULL!", procName);
        return;
    }

    if ((naa = *pnaa) == NULL)
        return;

    for (i = 0; i < naa->n; i++)
        numaDestroy(&naa->numa[i]);
    FREE(naa->numa);
    FREE(naa);
    *pnaa = NULL;

    return;
}



/*--------------------------------------------------------------------------*
 *                              Add Numa to Numaa                           *
 *--------------------------------------------------------------------------*/
/*!
 *  numaaAddNuma()
 *
 *      Input:  naa
 *              na   (to be added)
 *              copyflag  (L_INSERT, L_COPY, L_CLONE)
 *      Return: 0 if OK, 1 on error
 */
l_int32
numaaAddNuma(NUMAA   *naa,
             NUMA    *na,
             l_int32  copyflag)
{
l_int32  n;
NUMA    *nac;

    PROCNAME("numaaAddNuma");

    if (!naa)
        return ERROR_INT("naa not defined", procName, 1);
    if (!na)
        return ERROR_INT("na not defined", procName, 1);

    if (copyflag == L_INSERT)
        nac = na;
    else if (copyflag == L_COPY) {
        if ((nac = numaCopy(na)) == NULL)
            return ERROR_INT("nac not made", procName, 1);
    }
    else if (copyflag == L_CLONE)
        nac = numaClone(na);
    else
        return ERROR_INT("invalid copyflag", procName, 1);

    n = numaaGetCount(naa);
    if (n >= naa->nalloc)
        numaaExtendArray(naa);
    naa->numa[n] = nac;
    naa->n++;
    return 0;
}


/*!
 *  numaaExtendArray()
 *
 *      Input:  naa
 *      Return: 0 if OK, 1 on error
 */
l_int32
numaaExtendArray(NUMAA  *naa)
{
    PROCNAME("numaaExtendArray");

    if (!naa)
        return ERROR_INT("naa not defined", procName, 1);

    if ((naa->numa = (NUMA **)reallocNew((void **)&naa->numa,
                              sizeof(NUMA *) * naa->nalloc,
                              2 * sizeof(NUMA *) * naa->nalloc)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);

    naa->nalloc *= 2;
    return 0;
}


/*----------------------------------------------------------------------*
 *                           Numaa accessors                            *
 *----------------------------------------------------------------------*/
/*!
 *  numaaGetCount()
 *
 *      Input:  naa
 *      Return: count (number of numa), or 0 if no numa or on error
 */
l_int32
numaaGetCount(NUMAA  *naa)
{
    PROCNAME("numaaGetCount");

    if (!naa)
        return ERROR_INT("naa not defined", procName, 0);
    return naa->n;
}


/*!
 *  numaaGetNumberCount()
 *
 *      Input:  naa
 *      Return: count (number of numbers), or 0 if no numbers or on error
 */
l_int32
numaaGetNumberCount(NUMAA  *naa)
{
NUMA    *na;
l_int32  n, sum, i;

    PROCNAME("numaaGetNumberCount");

    if (!naa)
        return ERROR_INT("naa not defined", procName, 0);

    n = numaaGetCount(naa);
    for (sum = 0, i = 0; i < n; i++) {
        na = numaaGetNuma(naa, i, L_CLONE);
        sum += numaGetCount(na);
        numaDestroy(&na);
    }

    return sum;
}


/*!
 *  numaaGetPtrArray()
 *
 *      Input:  naa
 *      Return: the internal array of ptrs to Numa, or null on error
 *
 *  Notes:
 *      (1) This function is convenient for doing direct manipulation on
 *          a fixed size array of Numas.  To do this, it sets the count
 *          to the full size of the allocated array of Numa ptrs.
 *          The originating Numaa owns this array: DO NOT free it!
 *      (2) Intended usage:
 *            Numaa *naa = numaaCreate(n);
 *            Numa **array = numaaGetPtrArray(naa);
 *             ...  [manipulate Numas directly on the array]
 *            numaaDestroy(&naa);
 *      (3) Cautions:
 *           - Do not free this array; it is owned by tne Numaa.
 *           - Do not call any functions on the Numaa, other than
 *             numaaDestroy() when you're finished with the array.
 *             Adding a Numa will force a resize, destroying the ptr array.
 *           - Do not address the array outside its allocated size.
 *             With the bare array, there are no protections.  If the
 *             allocated size is n, array[n] is an error.
 */
NUMA **
numaaGetPtrArray(NUMAA  *naa)
{
    PROCNAME("numaaGetPtrArray");

    if (!naa)
        return (NUMA **)ERROR_PTR("naa not defined", procName, NULL);

    naa->n = naa->nalloc;
    return naa->numa;
}


/*!
 *  numaaGetNuma()
 *
 *      Input:  naa
 *              index  (to the index-th numa)
 *              accessflag   (L_COPY or L_CLONE)
 *      Return: numa, or null on error
 */
NUMA *
numaaGetNuma(NUMAA   *naa,
             l_int32  index,
             l_int32  accessflag)
{
    PROCNAME("numaaGetNuma");

    if (!naa)
        return (NUMA *)ERROR_PTR("naa not defined", procName, NULL);
    if (index < 0 || index >= naa->n)
        return (NUMA *)ERROR_PTR("index not valid", procName, NULL);

    if (accessflag == L_COPY)
        return numaCopy(naa->numa[index]);
    else if (accessflag == L_CLONE)
        return numaClone(naa->numa[index]);
    else
        return (NUMA *)ERROR_PTR("invalid accessflag", procName, NULL);
}


/*!
 *  numaaReplaceNuma()
 *
 *      Input:  naa
 *              index  (to the index-th numa)
 *              numa (insert and replace any existing one)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Any existing numa is destroyed, and the input one
 *          is inserted in its place.
 *      (2) If the index is invalid, return 1 (error)
 */
l_int32
numaaReplaceNuma(NUMAA   *naa,
                 l_int32  index,
                 NUMA    *na)
{
l_int32  n;

    PROCNAME("numaaReplaceNuma");

    if (!naa)
        return ERROR_INT("naa not defined", procName, 1);
    if (!na)
        return ERROR_INT("na not defined", procName, 1);
    n = numaaGetCount(naa);
    if (index < 0 || index >= n)
        return ERROR_INT("index not valid", procName, 1);

    numaDestroy(&naa->numa[index]);
    naa->numa[index] = na;
    return 0;
}


/*!
 *  numaaAddNumber()
 *
 *      Input:  naa
 *              index (of numa within numaa)
 *              val  (float or int to be added; stored as a float)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Adds to an existing numa only.
 */
l_int32
numaaAddNumber(NUMAA     *naa,
               l_int32    index,
               l_float32  val)
{
l_int32  n;
NUMA    *na;

    PROCNAME("numaaAddNumber");

    if (!naa)
        return ERROR_INT("naa not defined", procName, 1);
    n = numaaGetCount(naa);
    if (index < 0 || index >= n)
        return ERROR_INT("invalid index in naa", procName, 1);

    na = numaaGetNuma(naa, index, L_CLONE);
    numaAddNumber(na, val);
    numaDestroy(&na);
    return 0;
}


/*----------------------------------------------------------------------*
 *                      Serialize numaa for I/O                         *
 *----------------------------------------------------------------------*/
/*!
 *  numaaRead()
 *
 *      Input:  filename
 *      Return: naa, or null on error
 */
NUMAA *
numaaRead(const char  *filename)
{
FILE   *fp;
NUMAA  *naa;

    PROCNAME("numaaRead");

    if (!filename)
        return (NUMAA *)ERROR_PTR("filename not defined", procName, NULL);

    if ((fp = fopenReadStream(filename)) == NULL)
        return (NUMAA *)ERROR_PTR("stream not opened", procName, NULL);

    if ((naa = numaaReadStream(fp)) == NULL) {
        fclose(fp);
        return (NUMAA *)ERROR_PTR("naa not read", procName, NULL);
    }

    fclose(fp);
    return naa;
}


/*!
 *  numaaReadStream()
 *
 *      Input:  stream
 *      Return: naa, or null on error
 */
NUMAA *
numaaReadStream(FILE  *fp)
{
l_int32    i, n, index, ret, version;
NUMA      *na;
NUMAA     *naa;

    PROCNAME("numaaReadStream");

    if (!fp)
        return (NUMAA *)ERROR_PTR("stream not defined", procName, NULL);

    ret = fscanf(fp, "\nNumaa Version %d\n", &version);
    if (ret != 1)
        return (NUMAA *)ERROR_PTR("not a numa file", procName, NULL);
    if (version != NUMA_VERSION_NUMBER)
        return (NUMAA *)ERROR_PTR("invalid numaa version", procName, NULL);
    fscanf(fp, "Number of numa = %d\n\n", &n);
    if ((naa = numaaCreate(n)) == NULL)
        return (NUMAA *)ERROR_PTR("naa not made", procName, NULL);

    for (i = 0; i < n; i++) {
        fscanf(fp, "Numa[%d]:", &index);
        if ((na = numaReadStream(fp)) == NULL)
            return (NUMAA *)ERROR_PTR("na not made", procName, NULL);
        numaaAddNuma(naa, na, L_INSERT);
    }

    return naa;
}


/*!
 *  numaaWrite()
 *
 *      Input:  filename, naa
 *      Return: 0 if OK, 1 on error
 */
l_int32
numaaWrite(const char  *filename,
           NUMAA       *naa)
{
FILE  *fp;

    PROCNAME("numaaWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!naa)
        return ERROR_INT("naa not defined", procName, 1);

    if ((fp = fopen(filename, "w")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (numaaWriteStream(fp, naa))
        return ERROR_INT("naa not written to stream", procName, 1);
    fclose(fp);

    return 0;
}


/*!
 *  numaaWriteStream()
 *
 *      Input:  stream, naa
 *      Return: 0 if OK, 1 on error
 */
l_int32
numaaWriteStream(FILE   *fp,
                 NUMAA  *naa)
{
l_int32  i, n;
NUMA    *na;

    PROCNAME("numaaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!naa)
        return ERROR_INT("naa not defined", procName, 1);

    n = numaaGetCount(naa);
    fprintf(fp, "\nNumaa Version %d\n", NUMA_VERSION_NUMBER);
    fprintf(fp, "Number of numa = %d\n\n", n);
    for (i = 0; i < n; i++) {
        if ((na = numaaGetNuma(naa, i, L_CLONE)) == NULL)
            return ERROR_INT("na not found", procName, 1);
        fprintf(fp, "Numa[%d]:", i);
        numaWriteStream(fp, na);
        numaDestroy(&na);
    }

    return 0;
}


/*--------------------------------------------------------------------------*
 *                      Numa2d creation, destruction                        *
 *--------------------------------------------------------------------------*/
/*!
 *  numa2dCreate()
 *
 *      Input:  nrows (of 2d array)
 *              ncols (of 2d array)
 *              initsize (initial size of each allocated numa)
 *      Return: numa2d, or null on error
 *
 *  Notes:
 *      (1) The numa2d holds a doubly-indexed array of numa.
 *      (2) The numa ptr array is initialized with all ptrs set to NULL.
 *      (3) The numas are created only when a number is to be stored
 *          at an index (i,j) for which a numa has not yet been made.
 */
NUMA2D *
numa2dCreate(l_int32  nrows,
             l_int32  ncols,
             l_int32  initsize)
{
l_int32  i;
NUMA2D  *na2d;

    PROCNAME("numa2dCreate");

    if (nrows <= 1 || ncols <= 1)
        return (NUMA2D *)ERROR_PTR("rows, cols not both >= 1", procName, NULL);

    if ((na2d = (NUMA2D *)CALLOC(1, sizeof(NUMA2D))) == NULL)
        return (NUMA2D *)ERROR_PTR("na2d not made", procName, NULL);
    na2d->nrows = nrows;
    na2d->ncols = ncols;
    na2d->initsize = initsize;

        /* Set up the 2D array */
    if ((na2d->numa = (NUMA ***)CALLOC(nrows, sizeof(NUMA **))) == NULL)
        return (NUMA2D *)ERROR_PTR("numa row array not made", procName, NULL);
    for (i = 0; i < nrows; i++) {
        if ((na2d->numa[i] = (NUMA **)CALLOC(ncols, sizeof(NUMA *))) == NULL)
            return (NUMA2D *)ERROR_PTR("numa cols not made", procName, NULL);
    }

    return na2d;
}


/*!
 *  numa2dDestroy()
 *
 *      Input:  &numa2d (<to be nulled if it exists>)
 *      Return: void
 */
void
numa2dDestroy(NUMA2D  **pna2d)
{
l_int32  i, j;
NUMA2D  *na2d;

    PROCNAME("numa2dDestroy");

    if (pna2d == NULL) {
        L_WARNING("ptr address is NULL!", procName);
        return;
    }

    if ((na2d = *pna2d) == NULL)
        return;

    for (i = 0; i < na2d->nrows; i++) {
        for (j = 0; j < na2d->ncols; j++)
            numaDestroy(&na2d->numa[i][j]);
        FREE(na2d->numa[i]);
    }
    FREE(na2d->numa);
    FREE(na2d);
    *pna2d = NULL;

    return;
}



/*--------------------------------------------------------------------------*
 *                               Numa2d accessors                           *
 *--------------------------------------------------------------------------*/
/*!
 *  numa2dAddNumber()
 *
 *      Input:  na2d
 *              row of 2d array
 *              col of 2d array
 *              val  (float or int to be added; stored as a float)
 *      Return: 0 if OK, 1 on error
 */
l_int32
numa2dAddNumber(NUMA2D    *na2d,
                l_int32    row,
                l_int32    col,
                l_float32  val)
{
NUMA  *na;

    PROCNAME("numa2dAddNumber");

    if (!na2d)
        return ERROR_INT("na2d not defined", procName, 1);
    if (row < 0 || row >= na2d->nrows)
        return ERROR_INT("row out of bounds", procName, 1);
    if (col < 0 || col >= na2d->ncols)
        return ERROR_INT("col out of bounds", procName, 1);

    if ((na = na2d->numa[row][col]) == NULL) {
        na = numaCreate(na2d->initsize);
        na2d->numa[row][col] = na;
    }
    numaAddNumber(na, val);
    return 0;
}


/*!
 *  numa2dGetCount()
 *
 *      Input:  na2d
 *              row of 2d array
 *              col of 2d array
 *      Return: size of numa at [row][col], or 0 if the numa doesn't exist
 *              or on error
 */
l_int32
numa2dGetCount(NUMA2D  *na2d,
               l_int32  row,
               l_int32  col)
{
NUMA  *na;

    PROCNAME("numa2dGetCount");

    if (!na2d)
        return ERROR_INT("na2d not defined", procName, 0);
    if (row < 0 || row >= na2d->nrows)
        return ERROR_INT("row out of bounds", procName, 0);
    if (col < 0 || col >= na2d->ncols)
        return ERROR_INT("col out of bounds", procName, 0);
    if ((na = na2d->numa[row][col]) == NULL)
        return 0;
    else
        return na->n;
}


/*!
 *  numa2dGetNuma()
 *
 *      Input:  na2d
 *              row of 2d array
 *              col of 2d array
 *      Return: na (a clone of the numa if it exists) or null if it doesn't
 *
 *  Notes:
 *      (1) This does not give an error if the index is out of bounds.
 */
NUMA *
numa2dGetNuma(NUMA2D     *na2d,
              l_int32     row,
              l_int32     col)
{
NUMA  *na;

    PROCNAME("numa2dGetNuma");

    if (!na2d)
        return (NUMA *)ERROR_PTR("na2d not defined", procName, NULL);
    if (row < 0 || row >= na2d->nrows || col < 0 || col >= na2d->ncols)
        return NULL;
    if ((na = na2d->numa[row][col]) == NULL)
        return NULL;
    return numaClone(na);
}


/*!
 *  numa2dGetFValue()
 *
 *      Input:  na2d
 *              row of 2d array
 *              col of 2d array
 *              index (into numa)
 *              &val (<return> float value)
 *      Return: 0 if OK, 1 on error
 */
l_int32
numa2dGetFValue(NUMA2D     *na2d,
                l_int32     row,
                l_int32     col,
                l_int32     index,
                l_float32  *pval)
{
NUMA  *na;

    PROCNAME("numa2dGetFValue");

    if (!na2d)
        return ERROR_INT("na2d not defined", procName, 1);
    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = 0.0;

    if (row < 0 || row >= na2d->nrows)
        return ERROR_INT("row out of bounds", procName, 1);
    if (col < 0 || col >= na2d->ncols)
        return ERROR_INT("col out of bounds", procName, 1);
    if ((na = na2d->numa[row][col]) == NULL)
        return ERROR_INT("numa does not exist", procName, 1);

    return numaGetFValue(na, index, pval);
}


/*!
 *  numa2dGetIValue()
 *
 *      Input:  na2d
 *              row of 2d array
 *              col of 2d array
 *              index (into numa)
 *              &val (<return> integer value)
 *      Return: 0 if OK, 1 on error
 */
l_int32
numa2dGetIValue(NUMA2D   *na2d,
                l_int32   row,
                l_int32   col,
                l_int32   index,
                l_int32  *pval)
{
NUMA  *na;

    PROCNAME("numa2dGetIValue");

    if (!na2d)
        return ERROR_INT("na2d not defined", procName, 1);
    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = 0;

    if (row < 0 || row >= na2d->nrows)
        return ERROR_INT("row out of bounds", procName, 1);
    if (col < 0 || col >= na2d->ncols)
        return ERROR_INT("col out of bounds", procName, 1);
    if ((na = na2d->numa[row][col]) == NULL)
        return ERROR_INT("numa does not exist", procName, 1);

    return numaGetIValue(na, index, pval);
}


/*--------------------------------------------------------------------------*
 *               Number array hash: Creation and destruction                *
 *--------------------------------------------------------------------------*/
/*!
 *  numaHashCreate()
 *
 *      Input: nbuckets (the number of buckets in the hash table,
 *                       which should be prime.)
 *             initsize (initial size of each allocated numa; 0 for default)
 *      Return: ptr to new nahash, or null on error
 *
 *  Note: actual numa are created only as required by numaHashAdd()
 */
NUMAHASH *
numaHashCreate(l_int32  nbuckets,
               l_int32  initsize)
{
NUMAHASH  *nahash;

    PROCNAME("numaHashCreate");

    if (nbuckets <= 0)
        return (NUMAHASH *)ERROR_PTR("negative hash size", procName, NULL);
    if ((nahash = (NUMAHASH *)CALLOC(1, sizeof(NUMAHASH))) == NULL)
        return (NUMAHASH *)ERROR_PTR("nahash not made", procName, NULL);
    if ((nahash->numa = (NUMA **)CALLOC(nbuckets, sizeof(NUMA *))) == NULL) {
        FREE(nahash);
        return (NUMAHASH *)ERROR_PTR("numa ptr array not made", procName, NULL);
    }

    nahash->nbuckets = nbuckets;
    nahash->initsize = initsize;
    return nahash;
}


/*!
 *  numaHashDestroy()
 *
 *      Input:  &nahash (<to be nulled, if it exists>)
 *      Return: void
 */
void
numaHashDestroy(NUMAHASH **pnahash)
{
NUMAHASH  *nahash;
l_int32    i;

    PROCNAME("numaHashDestroy");

    if (pnahash == NULL) {
        L_WARNING("ptr address is NULL!", procName);
        return;
    }

    if ((nahash = *pnahash) == NULL)
        return;

    for (i = 0; i < nahash->nbuckets; i++)
        numaDestroy(&nahash->numa[i]);
    FREE(nahash->numa);
    FREE(nahash);
    *pnahash = NULL;
}


/*--------------------------------------------------------------------------*
 *               Number array hash: Add elements and return numas
 *--------------------------------------------------------------------------*/
/*!
 *  numaHashGetNuma()
 *
 *      Input:  nahash
 *              key  (key to be hashed into a bucket number)
 *      Return: ptr to numa
 */
NUMA *
numaHashGetNuma(NUMAHASH  *nahash,
                l_uint32   key)
{
l_int32  bucket;
NUMA    *na;

    PROCNAME("numaHashGetNuma");

    if (!nahash)
        return (NUMA *)ERROR_PTR("nahash not defined", procName, NULL);
    bucket = key % nahash->nbuckets;
    na = nahash->numa[bucket];
    if (na)
        return numaClone(na);
    else
        return NULL;
}

/*!
 *  numaHashAdd()
 *
 *      Input:  nahash
 *              key  (key to be hashed into a bucket number)
 *              value  (float value to be appended to the specific numa)
 *      Return: 0 if OK; 1 on error
 */
l_int32
numaHashAdd(NUMAHASH  *nahash,
            l_uint32   key,
            l_float32  value)
{
l_int32  bucket;
NUMA    *na;

    PROCNAME("numaHashAdd");

    if (!nahash)
        return ERROR_INT("nahash not defined", procName, 1);
    if (key < 0)
        return ERROR_INT("key < 0", procName, 1);
    bucket = key % nahash->nbuckets;
    na = nahash->numa[bucket];
    if (!na) {
        if ((na = numaCreate(nahash->initsize)) == NULL)
            return ERROR_INT("na not made", procName, 1);
        nahash->numa[bucket] = na;
    }
    numaAddNumber(na, value);
    return 0;
}

