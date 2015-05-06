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
 *   dnabasic.c
 *
 *      Dna creation, destruction, copy, clone, etc.
 *          L_DNA       *l_dnaCreate()
 *          L_DNA       *l_dnaCreateFromIArray()
 *          L_DNA       *l_dnaCreateFromDArray()
 *          L_DNA       *l_dnaMakeSequence()
 *          void        *l_dnaDestroy()
 *          L_DNA       *l_dnaCopy()
 *          L_DNA       *l_dnaClone()
 *          l_int32      l_dnaEmpty()
 *
 *      Dna: add/remove number and extend array
 *          l_int32      l_dnaAddNumber()
 *          static l_int32  l_dnaExtendArray()
 *          l_int32      l_dnaInsertNumber()
 *          l_int32      l_dnaRemoveNumber()
 *          l_int32      l_dnaReplaceNumber()
 *
 *      Dna accessors
 *          l_int32      l_dnaGetCount()
 *          l_int32      l_dnaSetCount()
 *          l_int32      l_dnaGetIValue()
 *          l_int32      l_dnaGetDValue()
 *          l_int32      l_dnaSetValue()
 *          l_int32      l_dnaShiftValue()
 *          l_int32     *l_dnaGetIArray()
 *          l_float64   *l_dnaGetDArray()
 *          l_int32      l_dnaGetRefcount()
 *          l_int32      l_dnaChangeRefcount()
 *          l_int32      l_dnaGetParameters()
 *          l_int32      l_dnaSetParameters()
 *          l_int32      l_dnaCopyParameters()
 *
 *      Serialize Dna for I/O
 *          L_DNA       *l_dnaRead()
 *          L_DNA       *l_dnaReadStream()
 *          l_int32      l_dnaWrite()
 *          l_int32      l_dnaWriteStream()
 *
 *      Dnaa creation, destruction
 *          L_DNAA      *l_dnaaCreate()
 *          void        *l_dnaaDestroy()
 *
 *      Add Dna to Dnaa
 *          l_int32      l_dnaaAddDna()
 *          l_int32      l_dnaaExtendArray()
 *
 *      Dnaa accessors
 *          l_int32      l_dnaaGetCount()
 *          l_int32      l_dnaaGetDnaCount()
 *          l_int32      l_dnaaGetNumberCount()
 *          L_DNA       *l_dnaaGetDna()
 *          L_DNA       *l_dnaaReplaceDna()
 *          l_int32      l_dnaaGetValue()
 *          l_int32      l_dnaaAddNumber()
 *
 *      Serialize Dnaa for I/O
 *          L_DNAA      *l_dnaaRead()
 *          L_DNAA      *l_dnaaReadStream()
 *          l_int32      l_dnaaWrite()
 *          l_int32      l_dnaaWriteStream()
 *
 *      Other Dna functions
 *          L_DNA       *l_dnaMakeDelta()
 *          NUMA        *l_dnaConvertToNuma()
 *          L_DNA       *numaConvertToDna()
 *          l_int32     *l_dnaJoin()
 *
 *    (1) The Dna is a struct holding an array of doubles.  It can also
 *        be used to store l_int32 values, up to the full precision
 *        of int32.  Use it whenever integers larger than a few million
 *        need to be stored.
 *
 *    (2) Always use the accessors in this file, never the fields directly.
 *
 *    (3) Storing and retrieving numbers:
 *
 *       * to append a new number to the array, use l_dnaAddNumber().  If
 *         the number is an int, it will will automatically be converted
 *         to l_float64 and stored.
 *
 *       * to reset a value stored in the array, use l_dnaSetValue().
 *
 *       * to increment or decrement a value stored in the array,
 *         use l_dnaShiftValue().
 *
 *       * to obtain a value from the array, use either l_dnaGetIValue()
 *         or l_dnaGetDValue(), depending on whether you are retrieving
 *         an integer or a float.  This avoids doing an explicit cast,
 *         such as
 *           (a) return a l_float64 and cast it to an l_int32
 *           (b) cast the return directly to (l_float64 *) to
 *               satisfy the function prototype, as in
 *                 l_dnaGetDValue(da, index, (l_float64 *)&ival);   [ugly!]
 *
 *    (4) int <--> double conversions:
 *
 *        Conversions go automatically from l_int32 --> l_float64,
 *        without loss of precision.  You must cast (l_int32)
 *        to go from l_float64 --> l_int32 because you're truncating
 *        to the integer value.
 *
 *    (5) As with other arrays in leptonica, the l_dna has both an allocated
 *        size and a count of the stored numbers.  When you add a number, it
 *        goes on the end of the array, and causes a realloc if the array
 *        is already filled.  However, in situations where you want to
 *        add numbers randomly into an array, such as when you build a
 *        histogram, you must set the count of stored numbers in advance.
 *        This is done with l_dnaSetCount().  If you set a count larger
 *        than the allocated array, it does a realloc to the size requested.
 *
 *    (6) In situations where the data in a l_dna correspond to a function
 *        y(x), the values can be either at equal spacings in x or at
 *        arbitrary spacings.  For the former, we can represent all x values
 *        by two parameters: startx (corresponding to y[0]) and delx
 *        for the change in x for adjacent values y[i] and y[i+1].
 *        startx and delx are initialized to 0.0 and 1.0, rsp.
 *        For arbitrary spacings, we use a second l_dna, and the two
 *        l_dnas are typically denoted dnay and dnax.
 */

#include <string.h>
#include <math.h>
#include "allheaders.h"

static const l_int32 INITIAL_PTR_ARRAYSIZE = 50;      /* n'importe quoi */

    /* Static functions */
static l_int32 l_dnaExtendArray(L_DNA *da);
static l_int32 l_dnaaExtendArray(L_DNAA *daa);


/*--------------------------------------------------------------------------*
 *                 Dna creation, destruction, copy, clone, etc.             *
 *--------------------------------------------------------------------------*/
/*!
 *  l_dnaCreate()
 *
 *      Input:  size of number array to be alloc'd (0 for default)
 *      Return: da, or null on error
 */
L_DNA *
l_dnaCreate(l_int32  n)
{
L_DNA  *da;

    PROCNAME("l_dnaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((da = (L_DNA *)CALLOC(1, sizeof(L_DNA))) == NULL)
        return (L_DNA *)ERROR_PTR("da not made", procName, NULL);
    if ((da->array = (l_float64 *)CALLOC(n, sizeof(l_float64))) == NULL)
        return (L_DNA *)ERROR_PTR("double array not made", procName, NULL);

    da->nalloc = n;
    da->n = 0;
    da->refcount = 1;
    da->startx = 0.0;
    da->delx = 1.0;

    return da;
}


/*!
 *  l_dnaCreateFromIArray()
 *
 *      Input:  iarray (integer)
 *              size (of the array)
 *      Return: da, or null on error
 *
 *  Notes:
 *      (1) We can't insert this int array into the l_dna, because a l_dna
 *          takes a double array.  So this just copies the data from the
 *          input array into the l_dna.  The input array continues to be
 *          owned by the caller.
 */
L_DNA *
l_dnaCreateFromIArray(l_int32  *iarray,
                      l_int32   size)
{
l_int32  i;
L_DNA   *da;

    PROCNAME("l_dnaCreateFromIArray");

    if (!iarray)
        return (L_DNA *)ERROR_PTR("iarray not defined", procName, NULL);
    if (size <= 0)
        return (L_DNA *)ERROR_PTR("size must be > 0", procName, NULL);

    da = l_dnaCreate(size);
    for (i = 0; i < size; i++)
        l_dnaAddNumber(da, iarray[i]);

    return da;
}


/*!
 *  l_dnaCreateFromDArray()
 *
 *      Input:  da (float)
 *              size (of the array)
 *              copyflag (L_INSERT or L_COPY)
 *      Return: da, or null on error
 *
 *  Notes:
 *      (1) With L_INSERT, ownership of the input array is transferred
 *          to the returned l_dna, and all @size elements are considered
 *          to be valid.
 */
L_DNA *
l_dnaCreateFromDArray(l_float64  *darray,
                      l_int32     size,
                      l_int32     copyflag)
{
l_int32  i;
L_DNA   *da;

    PROCNAME("l_dnaCreateFromDArray");

    if (!darray)
        return (L_DNA *)ERROR_PTR("darray not defined", procName, NULL);
    if (size <= 0)
        return (L_DNA *)ERROR_PTR("size must be > 0", procName, NULL);
    if (copyflag != L_INSERT && copyflag != L_COPY)
        return (L_DNA *)ERROR_PTR("invalid copyflag", procName, NULL);

    da = l_dnaCreate(size);
    if (copyflag == L_INSERT) {
        if (da->array) FREE(da->array);
        da->array = darray;
        da->n = size;
    } else {  /* just copy the contents */
        for (i = 0; i < size; i++)
            l_dnaAddNumber(da, darray[i]);
    }

    return da;
}


/*!
 *  l_dnaMakeSequence()
 *
 *      Input:  startval
 *              increment
 *              size (of sequence)
 *      Return: l_dna of sequence of evenly spaced values, or null on error
 */
L_DNA *
l_dnaMakeSequence(l_float64  startval,
                  l_float64  increment,
                  l_int32    size)
{
l_int32    i;
l_float64  val;
L_DNA     *da;

    PROCNAME("l_dnaMakeSequence");

    if ((da = l_dnaCreate(size)) == NULL)
        return (L_DNA *)ERROR_PTR("da not made", procName, NULL);

    for (i = 0; i < size; i++) {
        val = startval + i * increment;
        l_dnaAddNumber(da, val);
    }

    return da;
}


/*!
 *  l_dnaDestroy()
 *
 *      Input:  &da (<to be nulled if it exists>)
 *      Return: void
 *
 *  Notes:
 *      (1) Decrements the ref count and, if 0, destroys the l_dna.
 *      (2) Always nulls the input ptr.
 */
void
l_dnaDestroy(L_DNA  **pda)
{
L_DNA  *da;

    PROCNAME("l_dnaDestroy");

    if (pda == NULL) {
        L_WARNING("ptr address is NULL\n", procName);
        return;
    }

    if ((da = *pda) == NULL)
        return;

        /* Decrement the ref count.  If it is 0, destroy the l_dna. */
    l_dnaChangeRefcount(da, -1);
    if (l_dnaGetRefcount(da) <= 0) {
        if (da->array)
            FREE(da->array);
        FREE(da);
    }

    *pda = NULL;
    return;
}


/*!
 *  l_dnaCopy()
 *
 *      Input:  da
 *      Return: copy of l_dna, or null on error
 */
L_DNA *
l_dnaCopy(L_DNA  *da)
{
l_int32  i;
L_DNA   *dac;

    PROCNAME("l_dnaCopy");

    if (!da)
        return (L_DNA *)ERROR_PTR("da not defined", procName, NULL);

    if ((dac = l_dnaCreate(da->nalloc)) == NULL)
        return (L_DNA *)ERROR_PTR("dac not made", procName, NULL);
    dac->startx = da->startx;
    dac->delx = da->delx;

    for (i = 0; i < da->n; i++)
        l_dnaAddNumber(dac, da->array[i]);

    return dac;
}


/*!
 *  l_dnaClone()
 *
 *      Input:  da
 *      Return: ptr to same l_dna, or null on error
 */
L_DNA *
l_dnaClone(L_DNA  *da)
{
    PROCNAME("l_dnaClone");

    if (!da)
        return (L_DNA *)ERROR_PTR("da not defined", procName, NULL);

    l_dnaChangeRefcount(da, 1);
    return da;
}


/*!
 *  l_dnaEmpty()
 *
 *      Input:  da
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This does not change the allocation of the array.
 *          It just clears the number of stored numbers, so that
 *          the array appears to be empty.
 */
l_int32
l_dnaEmpty(L_DNA  *da)
{
    PROCNAME("l_dnaEmpty");

    if (!da)
        return ERROR_INT("da not defined", procName, 1);

    da->n = 0;
    return 0;
}



/*--------------------------------------------------------------------------*
 *                  Dna: add/remove number and extend array                 *
 *--------------------------------------------------------------------------*/
/*!
 *  l_dnaAddNumber()
 *
 *      Input:  da
 *              val  (float or int to be added; stored as a float)
 *      Return: 0 if OK, 1 on error
 */
l_int32
l_dnaAddNumber(L_DNA     *da,
               l_float64  val)
{
l_int32  n;

    PROCNAME("l_dnaAddNumber");

    if (!da)
        return ERROR_INT("da not defined", procName, 1);

    n = l_dnaGetCount(da);
    if (n >= da->nalloc)
        l_dnaExtendArray(da);
    da->array[n] = val;
    da->n++;
    return 0;
}


/*!
 *  l_dnaExtendArray()
 *
 *      Input:  da
 *      Return: 0 if OK, 1 on error
 */
static l_int32
l_dnaExtendArray(L_DNA  *da)
{
    PROCNAME("l_dnaExtendArray");

    if (!da)
        return ERROR_INT("da not defined", procName, 1);

    if ((da->array = (l_float64 *)reallocNew((void **)&da->array,
                                sizeof(l_float64) * da->nalloc,
                                2 * sizeof(l_float64) * da->nalloc)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);

    da->nalloc *= 2;
    return 0;
}


/*!
 *  l_dnaInsertNumber()
 *
 *      Input:  da
 *              index (location in da to insert new value)
 *              val  (float64 or integer to be added)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This shifts da[i] --> da[i + 1] for all i >= index,
 *          and then inserts val as da[index].
 *      (2) It should not be used repeatedly on large arrays,
 *          because the function is O(n).
 *
 */
l_int32
l_dnaInsertNumber(L_DNA      *da,
                  l_int32    index,
                  l_float64  val)
{
l_int32  i, n;

    PROCNAME("l_dnaInsertNumber");

    if (!da)
        return ERROR_INT("da not defined", procName, 1);
    n = l_dnaGetCount(da);
    if (index < 0 || index > n)
        return ERROR_INT("index not in {0...n}", procName, 1);

    if (n >= da->nalloc)
        l_dnaExtendArray(da);
    for (i = n; i > index; i--)
        da->array[i] = da->array[i - 1];
    da->array[index] = val;
    da->n++;
    return 0;
}


/*!
 *  l_dnaRemoveNumber()
 *
 *      Input:  da
 *              index (element to be removed)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This shifts da[i] --> da[i - 1] for all i > index.
 *      (2) It should not be used repeatedly on large arrays,
 *          because the function is O(n).
 */
l_int32
l_dnaRemoveNumber(L_DNA   *da,
                  l_int32  index)
{
l_int32  i, n;

    PROCNAME("l_dnaRemoveNumber");

    if (!da)
        return ERROR_INT("da not defined", procName, 1);
    n = l_dnaGetCount(da);
    if (index < 0 || index >= n)
        return ERROR_INT("index not in {0...n - 1}", procName, 1);

    for (i = index + 1; i < n; i++)
        da->array[i - 1] = da->array[i];
    da->n--;
    return 0;
}


/*!
 *  l_dnaReplaceNumber()
 *
 *      Input:  da
 *              index (element to be replaced)
 *              val (new value to replace old one)
 *      Return: 0 if OK, 1 on error
 */
l_int32
l_dnaReplaceNumber(L_DNA     *da,
                   l_int32    index,
                   l_float64  val)
{
l_int32  n;

    PROCNAME("l_dnaReplaceNumber");

    if (!da)
        return ERROR_INT("da not defined", procName, 1);
    n = l_dnaGetCount(da);
    if (index < 0 || index >= n)
        return ERROR_INT("index not in {0...n - 1}", procName, 1);

    da->array[index] = val;
    return 0;
}


/*----------------------------------------------------------------------*
 *                             Dna accessors                            *
 *----------------------------------------------------------------------*/
/*!
 *  l_dnaGetCount()
 *
 *      Input:  da
 *      Return: count, or 0 if no numbers or on error
 */
l_int32
l_dnaGetCount(L_DNA  *da)
{
    PROCNAME("l_dnaGetCount");

    if (!da)
        return ERROR_INT("da not defined", procName, 0);
    return da->n;
}


/*!
 *  l_dnaSetCount()
 *
 *      Input:  da
 *              newcount
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If newcount <= da->nalloc, this resets da->n.
 *          Using newcount = 0 is equivalent to l_dnaEmpty().
 *      (2) If newcount > da->nalloc, this causes a realloc
 *          to a size da->nalloc = newcount.
 *      (3) All the previously unused values in da are set to 0.0.
 */
l_int32
l_dnaSetCount(L_DNA   *da,
              l_int32  newcount)
{
    PROCNAME("l_dnaSetCount");

    if (!da)
        return ERROR_INT("da not defined", procName, 1);
    if (newcount > da->nalloc) {
        if ((da->array = (l_float64 *)reallocNew((void **)&da->array,
                         sizeof(l_float64) * da->nalloc,
                         sizeof(l_float64) * newcount)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);
        da->nalloc = newcount;
    }
    da->n = newcount;
    return 0;
}


/*!
 *  l_dnaGetDValue()
 *
 *      Input:  da
 *              index (into l_dna)
 *              &val  (<return> double value; 0.0 on error)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Caller may need to check the function return value to
 *          decide if a 0.0 in the returned ival is valid.
 */
l_int32
l_dnaGetDValue(L_DNA      *da,
               l_int32     index,
               l_float64  *pval)
{
    PROCNAME("l_dnaGetDValue");

    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = 0.0;
    if (!da)
        return ERROR_INT("da not defined", procName, 1);

    if (index < 0 || index >= da->n)
        return ERROR_INT("index not valid", procName, 1);

    *pval = da->array[index];
    return 0;
}


/*!
 *  l_dnaGetIValue()
 *
 *      Input:  da
 *              index (into l_dna)
 *              &ival  (<return> integer value; 0 on error)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Caller may need to check the function return value to
 *          decide if a 0 in the returned ival is valid.
 */
l_int32
l_dnaGetIValue(L_DNA    *da,
               l_int32   index,
               l_int32  *pival)
{
l_float64  val;

    PROCNAME("l_dnaGetIValue");

    if (!pival)
        return ERROR_INT("&ival not defined", procName, 1);
    *pival = 0;
    if (!da)
        return ERROR_INT("da not defined", procName, 1);

    if (index < 0 || index >= da->n)
        return ERROR_INT("index not valid", procName, 1);

    val = da->array[index];
    *pival = (l_int32)(val + L_SIGN(val) * 0.5);
    return 0;
}


/*!
 *  l_dnaSetValue()
 *
 *      Input:  da
 *              index  (to element to be set)
 *              val  (to set element)
 *      Return: 0 if OK; 1 on error
 */
l_int32
l_dnaSetValue(L_DNA     *da,
              l_int32    index,
              l_float64  val)
{
    PROCNAME("l_dnaSetValue");

    if (!da)
        return ERROR_INT("da not defined", procName, 1);
    if (index < 0 || index >= da->n)
        return ERROR_INT("index not valid", procName, 1);

    da->array[index] = val;
    return 0;
}


/*!
 *  l_dnaShiftValue()
 *
 *      Input:  da
 *              index (to element to change relative to the current value)
 *              diff  (increment if diff > 0 or decrement if diff < 0)
 *      Return: 0 if OK; 1 on error
 */
l_int32
l_dnaShiftValue(L_DNA     *da,
                l_int32    index,
                l_float64  diff)
{
    PROCNAME("l_dnaShiftValue");

    if (!da)
        return ERROR_INT("da not defined", procName, 1);
    if (index < 0 || index >= da->n)
        return ERROR_INT("index not valid", procName, 1);

    da->array[index] += diff;
    return 0;
}


/*!
 *  l_dnaGetIArray()
 *
 *      Input:  da
 *      Return: a copy of the bare internal array, integerized
 *              by rounding, or null on error
 *  Notes:
 *      (1) A copy of the array is made, because we need to
 *          generate an integer array from the bare double array.
 *          The caller is responsible for freeing the array.
 *      (2) The array size is determined by the number of stored numbers,
 *          not by the size of the allocated array in the l_dna.
 *      (3) This function is provided to simplify calculations
 *          using the bare internal array, rather than continually
 *          calling accessors on the l_dna.  It is typically used
 *          on an array of size 256.
 */
l_int32 *
l_dnaGetIArray(L_DNA  *da)
{
l_int32   i, n, ival;
l_int32  *array;

    PROCNAME("l_dnaGetIArray");

    if (!da)
        return (l_int32 *)ERROR_PTR("da not defined", procName, NULL);

    n = l_dnaGetCount(da);
    if ((array = (l_int32 *)CALLOC(n, sizeof(l_int32))) == NULL)
        return (l_int32 *)ERROR_PTR("array not made", procName, NULL);
    for (i = 0; i < n; i++) {
        l_dnaGetIValue(da, i, &ival);
        array[i] = ival;
    }

    return array;
}


/*!
 *  l_dnaGetDArray()
 *
 *      Input:  da
 *              copyflag (L_NOCOPY or L_COPY)
 *      Return: either the bare internal array or a copy of it,
 *              or null on error
 *
 *  Notes:
 *      (1) If copyflag == L_COPY, it makes a copy which the caller
 *          is responsible for freeing.  Otherwise, it operates
 *          directly on the bare array of the l_dna.
 *      (2) Very important: for L_NOCOPY, any writes to the array
 *          will be in the l_dna.  Do not write beyond the size of
 *          the count field, because it will not be accessable
 *          from the l_dna!  If necessary, be sure to set the count
 *          field to a larger number (such as the alloc size)
 *          BEFORE calling this function.  Creating with l_dnaMakeConstant()
 *          is another way to insure full initialization.
 */
l_float64 *
l_dnaGetDArray(L_DNA   *da,
               l_int32  copyflag)
{
l_int32     i, n;
l_float64  *array;

    PROCNAME("l_dnaGetDArray");

    if (!da)
        return (l_float64 *)ERROR_PTR("da not defined", procName, NULL);

    if (copyflag == L_NOCOPY) {
        array = da->array;
    } else {  /* copyflag == L_COPY */
        n = l_dnaGetCount(da);
        if ((array = (l_float64 *)CALLOC(n, sizeof(l_float64))) == NULL)
            return (l_float64 *)ERROR_PTR("array not made", procName, NULL);
        for (i = 0; i < n; i++)
            array[i] = da->array[i];
    }

    return array;
}


/*!
 *  l_dnaGetRefCount()
 *
 *      Input:  da
 *      Return: refcount, or UNDEF on error
 */
l_int32
l_dnaGetRefcount(L_DNA  *da)
{
    PROCNAME("l_dnaGetRefcount");

    if (!da)
        return ERROR_INT("da not defined", procName, UNDEF);
    return da->refcount;
}


/*!
 *  l_dnaChangeRefCount()
 *
 *      Input:  da
 *              delta (change to be applied)
 *      Return: 0 if OK, 1 on error
 */
l_int32
l_dnaChangeRefcount(L_DNA   *da,
                    l_int32  delta)
{
    PROCNAME("l_dnaChangeRefcount");

    if (!da)
        return ERROR_INT("da not defined", procName, 1);
    da->refcount += delta;
    return 0;
}


/*!
 *  l_dnaGetParameters()
 *
 *      Input:  da
 *              &startx (<optional return> startx)
 *              &delx (<optional return> delx)
 *      Return: 0 if OK, 1 on error
 */
l_int32
l_dnaGetParameters(L_DNA     *da,
                   l_float64  *pstartx,
                   l_float64  *pdelx)
{
    PROCNAME("l_dnaGetParameters");

    if (!da)
        return ERROR_INT("da not defined", procName, 1);

    if (pstartx) *pstartx = da->startx;
    if (pdelx) *pdelx = da->delx;
    return 0;
}


/*!
 *  l_dnaSetParameters()
 *
 *      Input:  da
 *              startx (x value corresponding to da[0])
 *              delx (difference in x values for the situation where the
 *                    elements of da correspond to the evaulation of a
 *                    function at equal intervals of size @delx)
 *      Return: 0 if OK, 1 on error
 */
l_int32
l_dnaSetParameters(L_DNA     *da,
                   l_float64  startx,
                   l_float64  delx)
{
    PROCNAME("l_dnaSetParameters");

    if (!da)
        return ERROR_INT("da not defined", procName, 1);

    da->startx = startx;
    da->delx = delx;
    return 0;
}


/*!
 *  l_dnaCopyParameters()
 *
 *      Input:  dad (destination DNuma)
 *              das (source DNuma)
 *      Return: 0 if OK, 1 on error
 */
l_int32
l_dnaCopyParameters(L_DNA  *dad,
                    L_DNA  *das)
{
l_float64  start, binsize;

    PROCNAME("l_dnaCopyParameters");

    if (!das || !dad)
        return ERROR_INT("das and dad not both defined", procName, 1);

    l_dnaGetParameters(das, &start, &binsize);
    l_dnaSetParameters(dad, start, binsize);
    return 0;
}


/*----------------------------------------------------------------------*
 *                        Serialize Dna for I/O                         *
 *----------------------------------------------------------------------*/
/*!
 *  l_dnaRead()
 *
 *      Input:  filename
 *      Return: da, or null on error
 */
L_DNA *
l_dnaRead(const char  *filename)
{
FILE   *fp;
L_DNA  *da;

    PROCNAME("l_dnaRead");

    if (!filename)
        return (L_DNA *)ERROR_PTR("filename not defined", procName, NULL);

    if ((fp = fopenReadStream(filename)) == NULL)
        return (L_DNA *)ERROR_PTR("stream not opened", procName, NULL);

    if ((da = l_dnaReadStream(fp)) == NULL) {
        fclose(fp);
        return (L_DNA *)ERROR_PTR("da not read", procName, NULL);
    }

    fclose(fp);
    return da;
}


/*!
 *  l_dnaReadStream()
 *
 *      Input:  stream
 *      Return: da, or null on error
 */
L_DNA *
l_dnaReadStream(FILE  *fp)
{
l_int32    i, n, index, ret, version;
l_float64  val, startx, delx;
L_DNA     *da;

    PROCNAME("l_dnaReadStream");

    if (!fp)
        return (L_DNA *)ERROR_PTR("stream not defined", procName, NULL);

    ret = fscanf(fp, "\nL_Dna Version %d\n", &version);
    if (ret != 1)
        return (L_DNA *)ERROR_PTR("not a l_dna file", procName, NULL);
    if (version != DNA_VERSION_NUMBER)
        return (L_DNA *)ERROR_PTR("invalid l_dna version", procName, NULL);
    if (fscanf(fp, "Number of numbers = %d\n", &n) != 1)
        return (L_DNA *)ERROR_PTR("invalid number of numbers", procName, NULL);

    if ((da = l_dnaCreate(n)) == NULL)
        return (L_DNA *)ERROR_PTR("da not made", procName, NULL);

    for (i = 0; i < n; i++) {
        if (fscanf(fp, "  [%d] = %lf\n", &index, &val) != 2)
            return (L_DNA *)ERROR_PTR("bad input data", procName, NULL);
        l_dnaAddNumber(da, val);
    }

        /* Optional data */
    if (fscanf(fp, "startx = %lf, delx = %lf\n", &startx, &delx) == 2)
        l_dnaSetParameters(da, startx, delx);

    return da;
}


/*!
 *  l_dnaWrite()
 *
 *      Input:  filename, da
 *      Return: 0 if OK, 1 on error
 */
l_int32
l_dnaWrite(const char  *filename,
           L_DNA       *da)
{
FILE  *fp;

    PROCNAME("l_dnaWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!da)
        return ERROR_INT("da not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "w")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (l_dnaWriteStream(fp, da))
        return ERROR_INT("da not written to stream", procName, 1);
    fclose(fp);

    return 0;
}


/*!
 *  l_dnaWriteStream()
 *
 *      Input:  stream, da
 *      Return: 0 if OK, 1 on error
 */
l_int32
l_dnaWriteStream(FILE   *fp,
                 L_DNA  *da)
{
l_int32    i, n;
l_float64  startx, delx;

    PROCNAME("l_dnaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!da)
        return ERROR_INT("da not defined", procName, 1);

    n = l_dnaGetCount(da);
    fprintf(fp, "\nL_Dna Version %d\n", DNA_VERSION_NUMBER);
    fprintf(fp, "Number of numbers = %d\n", n);
    for (i = 0; i < n; i++)
        fprintf(fp, "  [%d] = %lf\n", i, da->array[i]);
    fprintf(fp, "\n");

        /* Optional data */
    l_dnaGetParameters(da, &startx, &delx);
    if (startx != 0.0 || delx != 1.0)
        fprintf(fp, "startx = %lf, delx = %lf\n", startx, delx);

    return 0;
}


/*--------------------------------------------------------------------------*
 *                       Dnaa creation, destruction                         *
 *--------------------------------------------------------------------------*/
/*!
 *  l_dnaaCreate()
 *
 *      Input:  size of l_dna ptr array to be alloc'd (0 for default)
 *      Return: daa, or null on error
 *
 */
L_DNAA *
l_dnaaCreate(l_int32  n)
{
L_DNAA  *daa;

    PROCNAME("l_dnaaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((daa = (L_DNAA *)CALLOC(1, sizeof(L_DNAA))) == NULL)
        return (L_DNAA *)ERROR_PTR("daa not made", procName, NULL);
    if ((daa->dna = (L_DNA **)CALLOC(n, sizeof(L_DNA *))) == NULL)
        return (L_DNAA *)ERROR_PTR("l_dna ptr array not made", procName, NULL);

    daa->nalloc = n;
    daa->n = 0;

    return daa;
}


/*!
 *  l_dnaaDestroy()
 *
 *      Input: &dnaa <to be nulled if it exists>
 *      Return: void
 */
void
l_dnaaDestroy(L_DNAA  **pdaa)
{
l_int32  i;
L_DNAA  *daa;

    PROCNAME("l_dnaaDestroy");

    if (pdaa == NULL) {
        L_WARNING("ptr address is NULL!\n", procName);
        return;
    }

    if ((daa = *pdaa) == NULL)
        return;

    for (i = 0; i < daa->n; i++)
        l_dnaDestroy(&daa->dna[i]);
    FREE(daa->dna);
    FREE(daa);
    *pdaa = NULL;

    return;
}


/*--------------------------------------------------------------------------*
 *                             Add Dna to Dnaa                              *
 *--------------------------------------------------------------------------*/
/*!
 *  l_dnaaAddDna()
 *
 *      Input:  daa
 *              da   (to be added)
 *              copyflag  (L_INSERT, L_COPY, L_CLONE)
 *      Return: 0 if OK, 1 on error
 */
l_int32
l_dnaaAddDna(L_DNAA  *daa,
               L_DNA   *da,
               l_int32  copyflag)
{
l_int32  n;
L_DNA   *dac;

    PROCNAME("l_dnaaAddDna");

    if (!daa)
        return ERROR_INT("daa not defined", procName, 1);
    if (!da)
        return ERROR_INT("da not defined", procName, 1);

    if (copyflag == L_INSERT) {
        dac = da;
    } else if (copyflag == L_COPY) {
        if ((dac = l_dnaCopy(da)) == NULL)
            return ERROR_INT("dac not made", procName, 1);
    } else if (copyflag == L_CLONE) {
        dac = l_dnaClone(da);
    } else {
        return ERROR_INT("invalid copyflag", procName, 1);
    }

    n = l_dnaaGetCount(daa);
    if (n >= daa->nalloc)
        l_dnaaExtendArray(daa);
    daa->dna[n] = dac;
    daa->n++;
    return 0;
}


/*!
 *  l_dnaaExtendArray()
 *
 *      Input:  daa
 *      Return: 0 if OK, 1 on error
 */
static l_int32
l_dnaaExtendArray(L_DNAA  *daa)
{
    PROCNAME("l_dnaaExtendArray");

    if (!daa)
        return ERROR_INT("daa not defined", procName, 1);

    if ((daa->dna = (L_DNA **)reallocNew((void **)&daa->dna,
                              sizeof(L_DNA *) * daa->nalloc,
                              2 * sizeof(L_DNA *) * daa->nalloc)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);

    daa->nalloc *= 2;
    return 0;
}


/*----------------------------------------------------------------------*
 *                           DNumaa accessors                           *
 *----------------------------------------------------------------------*/
/*!
 *  l_dnaaGetCount()
 *
 *      Input:  daa
 *      Return: count (number of l_dna), or 0 if no l_dna or on error
 */
l_int32
l_dnaaGetCount(L_DNAA  *daa)
{
    PROCNAME("l_dnaaGetCount");

    if (!daa)
        return ERROR_INT("daa not defined", procName, 0);
    return daa->n;
}


/*!
 *  l_dnaaGetDnaCount()
 *
 *      Input:  daa
 *              index (of l_dna in daa)
 *      Return: count of numbers in the referenced l_dna, or 0 on error.
 */
l_int32
l_dnaaGetDnaCount(L_DNAA   *daa,
                    l_int32  index)
{
    PROCNAME("l_dnaaGetDnaCount");

    if (!daa)
        return ERROR_INT("daa not defined", procName, 0);
    if (index < 0 || index >= daa->n)
        return ERROR_INT("invalid index into daa", procName, 0);
    return l_dnaGetCount(daa->dna[index]);
}


/*!
 *  l_dnaaGetNumberCount()
 *
 *      Input:  daa
 *      Return: count (total number of numbers in the l_dnaa),
 *                     or 0 if no numbers or on error
 */
l_int32
l_dnaaGetNumberCount(L_DNAA  *daa)
{
L_DNA   *da;
l_int32  n, sum, i;

    PROCNAME("l_dnaaGetNumberCount");

    if (!daa)
        return ERROR_INT("daa not defined", procName, 0);

    n = l_dnaaGetCount(daa);
    for (sum = 0, i = 0; i < n; i++) {
        da = l_dnaaGetDna(daa, i, L_CLONE);
        sum += l_dnaGetCount(da);
        l_dnaDestroy(&da);
    }

    return sum;
}


/*!
 *  l_dnaaGetDna()
 *
 *      Input:  daa
 *              index  (to the index-th l_dna)
 *              accessflag   (L_COPY or L_CLONE)
 *      Return: l_dna, or null on error
 */
L_DNA *
l_dnaaGetDna(L_DNAA  *daa,
             l_int32  index,
             l_int32  accessflag)
{
    PROCNAME("l_dnaaGetDna");

    if (!daa)
        return (L_DNA *)ERROR_PTR("daa not defined", procName, NULL);
    if (index < 0 || index >= daa->n)
        return (L_DNA *)ERROR_PTR("index not valid", procName, NULL);

    if (accessflag == L_COPY)
        return l_dnaCopy(daa->dna[index]);
    else if (accessflag == L_CLONE)
        return l_dnaClone(daa->dna[index]);
    else
        return (L_DNA *)ERROR_PTR("invalid accessflag", procName, NULL);
}


/*!
 *  l_dnaaReplaceDna()
 *
 *      Input:  daa
 *              index  (to the index-th l_dna)
 *              l_dna (insert and replace any existing one)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Any existing l_dna is destroyed, and the input one
 *          is inserted in its place.
 *      (2) If the index is invalid, return 1 (error)
 */
l_int32
l_dnaaReplaceDna(L_DNAA  *daa,
                 l_int32  index,
                 L_DNA   *da)
{
l_int32  n;

    PROCNAME("l_dnaaReplaceDna");

    if (!daa)
        return ERROR_INT("daa not defined", procName, 1);
    if (!da)
        return ERROR_INT("da not defined", procName, 1);
    n = l_dnaaGetCount(daa);
    if (index < 0 || index >= n)
        return ERROR_INT("index not valid", procName, 1);

    l_dnaDestroy(&daa->dna[index]);
    daa->dna[index] = da;
    return 0;
}


/*!
 *  l_dnaaGetValue()
 *
 *      Input:  daa
 *              i (index of l_dna within l_dnaa)
 *              j (index into l_dna)
 *              val (<return> double value)
 *      Return: 0 if OK, 1 on error
 */
l_int32
l_dnaaGetValue(L_DNAA     *daa,
               l_int32     i,
               l_int32     j,
               l_float64  *pval)
{
l_int32  n;
L_DNA   *da;

    PROCNAME("l_dnaaGetValue");

    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = 0.0;
    if (!daa)
        return ERROR_INT("daa not defined", procName, 1);
    n = l_dnaaGetCount(daa);
    if (i < 0 || i >= n)
        return ERROR_INT("invalid index into daa", procName, 1);
    da = daa->dna[i];
    if (j < 0 || j >= da->n)
        return ERROR_INT("invalid index into da", procName, 1);
    *pval = da->array[j];
    return 0;
}


/*!
 *  l_dnaaAddNumber()
 *
 *      Input:  daa
 *              index (of l_dna within l_dnaa)
 *              val  (number to be added; stored as a double)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Adds to an existing l_dna only.
 */
l_int32
l_dnaaAddNumber(L_DNAA    *daa,
                l_int32    index,
                l_float64  val)
{
l_int32  n;
L_DNA   *da;

    PROCNAME("l_dnaaAddNumber");

    if (!daa)
        return ERROR_INT("daa not defined", procName, 1);
    n = l_dnaaGetCount(daa);
    if (index < 0 || index >= n)
        return ERROR_INT("invalid index in daa", procName, 1);

    da = l_dnaaGetDna(daa, index, L_CLONE);
    l_dnaAddNumber(da, val);
    l_dnaDestroy(&da);
    return 0;
}


/*----------------------------------------------------------------------*
 *                        Serialize Dna for I/O                         *
 *----------------------------------------------------------------------*/
/*!
 *  l_dnaaRead()
 *
 *      Input:  filename
 *      Return: daa, or null on error
 */
L_DNAA *
l_dnaaRead(const char  *filename)
{
FILE    *fp;
L_DNAA  *daa;

    PROCNAME("l_dnaaRead");

    if (!filename)
        return (L_DNAA *)ERROR_PTR("filename not defined", procName, NULL);

    if ((fp = fopenReadStream(filename)) == NULL)
        return (L_DNAA *)ERROR_PTR("stream not opened", procName, NULL);

    if ((daa = l_dnaaReadStream(fp)) == NULL) {
        fclose(fp);
        return (L_DNAA *)ERROR_PTR("daa not read", procName, NULL);
    }

    fclose(fp);
    return daa;
}


/*!
 *  l_dnaaReadStream()
 *
 *      Input:  stream
 *      Return: daa, or null on error
 */
L_DNAA *
l_dnaaReadStream(FILE  *fp)
{
l_int32    i, n, index, ret, version;
L_DNA     *da;
L_DNAA    *daa;

    PROCNAME("l_dnaaReadStream");

    if (!fp)
        return (L_DNAA *)ERROR_PTR("stream not defined", procName, NULL);

    ret = fscanf(fp, "\nL_Dnaa Version %d\n", &version);
    if (ret != 1)
        return (L_DNAA *)ERROR_PTR("not a l_dna file", procName, NULL);
    if (version != DNA_VERSION_NUMBER)
        return (L_DNAA *)ERROR_PTR("invalid l_dnaa version", procName, NULL);
    if (fscanf(fp, "Number of L_Dna = %d\n\n", &n) != 1)
        return (L_DNAA *)ERROR_PTR("invalid number of l_dna", procName, NULL);
    if ((daa = l_dnaaCreate(n)) == NULL)
        return (L_DNAA *)ERROR_PTR("daa not made", procName, NULL);

    for (i = 0; i < n; i++) {
        if (fscanf(fp, "L_Dna[%d]:", &index) != 1)
            return (L_DNAA *)ERROR_PTR("invalid l_dna header", procName, NULL);
        if ((da = l_dnaReadStream(fp)) == NULL)
            return (L_DNAA *)ERROR_PTR("da not made", procName, NULL);
        l_dnaaAddDna(daa, da, L_INSERT);
    }

    return daa;
}


/*!
 *  l_dnaaWrite()
 *
 *      Input:  filename, daa
 *      Return: 0 if OK, 1 on error
 */
l_int32
l_dnaaWrite(const char  *filename,
            L_DNAA      *daa)
{
FILE  *fp;

    PROCNAME("l_dnaaWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!daa)
        return ERROR_INT("daa not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "w")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (l_dnaaWriteStream(fp, daa))
        return ERROR_INT("daa not written to stream", procName, 1);
    fclose(fp);

    return 0;
}


/*!
 *  l_dnaaWriteStream()
 *
 *      Input:  stream, daa
 *      Return: 0 if OK, 1 on error
 */
l_int32
l_dnaaWriteStream(FILE    *fp,
                  L_DNAA  *daa)
{
l_int32  i, n;
L_DNA   *da;

    PROCNAME("l_dnaaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!daa)
        return ERROR_INT("daa not defined", procName, 1);

    n = l_dnaaGetCount(daa);
    fprintf(fp, "\nL_Dnaa Version %d\n", DNA_VERSION_NUMBER);
    fprintf(fp, "Number of L_Dna = %d\n\n", n);
    for (i = 0; i < n; i++) {
        if ((da = l_dnaaGetDna(daa, i, L_CLONE)) == NULL)
            return ERROR_INT("da not found", procName, 1);
        fprintf(fp, "L_Dna[%d]:", i);
        l_dnaWriteStream(fp, da);
        l_dnaDestroy(&da);
    }

    return 0;
}


/*----------------------------------------------------------------------*
 *                         Other Dna functions                          *
 *----------------------------------------------------------------------*/
/*!
 *  l_dnaMakeDelta()
 *
 *      Input:  das (input l_dna)
 *      Return: dad (of difference values val[i+1] - val[i]),
 *                   or null on error
 */
L_DNA *
l_dnaMakeDelta(L_DNA  *das)
{
l_int32  i, n, prev, cur;
L_DNA   *dad;

    PROCNAME("l_dnaMakeDelta");

    if (!das)
        return (L_DNA *)ERROR_PTR("das not defined", procName, NULL);
    n = l_dnaGetCount(das);
    dad = l_dnaCreate(n - 1);
    prev = 0;
    for (i = 1; i < n; i++) {
        l_dnaGetIValue(das, i, &cur);
        l_dnaAddNumber(dad, cur - prev);
        prev = cur;
    }
    return dad;
}


/*!
 *  l_dnaConvertToNuma()
 *
 *      Input:  da
 *      Return: na, or null on error
 */
NUMA *
l_dnaConvertToNuma(L_DNA  *da)
{
l_int32    i, n;
l_float64  val;
NUMA      *na;

    PROCNAME("l_dnaConvertToNuma");

    if (!da)
        return (NUMA *)ERROR_PTR("da not defined", procName, NULL);

    n = l_dnaGetCount(da);
    na = numaCreate(n);
    for (i = 0; i < n; i++) {
        l_dnaGetDValue(da, i, &val);
        numaAddNumber(na, val);
    }
    return na;
}


/*!
 *  numaConvertToDna
 *
 *      Input:  na
 *      Return: da, or null on error
 */
L_DNA *
numaConvertToDna(NUMA  *na)
{
l_int32    i, n;
l_float32  val;
L_DNA     *da;

    PROCNAME("numaConvertToDna");

    if (!na)
        return (L_DNA *)ERROR_PTR("na not defined", procName, NULL);

    n = numaGetCount(na);
    da = l_dnaCreate(n);
    for (i = 0; i < n; i++) {
        numaGetFValue(na, i, &val);
        l_dnaAddNumber(da, val);
    }
    return da;
}


/*!
 *  l_dnaJoin()
 *
 *      Input:  dad  (dest dma; add to this one)
 *              das  (<optional> source dna; add from this one)
 *              istart  (starting index in das)
 *              iend  (ending index in das; use -1 to cat all)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) istart < 0 is taken to mean 'read from the start' (istart = 0)
 *      (2) iend < 0 means 'read to the end'
 *      (3) if das == NULL, this is a no-op
 */
l_int32
l_dnaJoin(L_DNA   *dad,
          L_DNA   *das,
          l_int32  istart,
          l_int32  iend)
{
l_int32    n, i;
l_float64  val;

    PROCNAME("l_dnaJoin");

    if (!dad)
        return ERROR_INT("dad not defined", procName, 1);
    if (!das)
        return 0;

    if (istart < 0)
        istart = 0;
    n = l_dnaGetCount(das);
    if (iend < 0 || iend >= n)
        iend = n - 1;
    if (istart > iend)
        return ERROR_INT("istart > iend; nothing to add", procName, 1);

    for (i = istart; i <= iend; i++) {
        l_dnaGetDValue(das, i, &val);
        l_dnaAddNumber(dad, val);
    }

    return 0;
}
