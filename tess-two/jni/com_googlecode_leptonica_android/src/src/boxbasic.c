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
 *   boxbasic.c
 *
 *   Basic 'class' functions for box, boxa and boxaa,
 *   including accessors and serialization.
 *
 *      Box creation, copy, clone, destruction
 *           BOX      *boxCreate()
 *           BOX      *boxCreateValid()
 *           BOX      *boxCopy()
 *           BOX      *boxClone()
 *           void      boxDestroy()
 *
 *      Box accessors
 *           l_int32   boxGetGeometry()
 *           l_int32   boxSetGeometry()
 *           l_int32   boxGetSideLocation()
 *           l_int32   boxGetRefcount()
 *           l_int32   boxChangeRefcount()
 *           l_int32   boxIsValid()
 *
 *      Boxa creation, copy, destruction
 *           BOXA     *boxaCreate()
 *           BOXA     *boxaCopy()
 *           void      boxaDestroy()
 *
 *      Boxa array extension
 *           l_int32   boxaAddBox()
 *           l_int32   boxaExtendArray()
 *           l_int32   boxaExtendArrayToSize()
 *
 *      Boxa accessors
 *           l_int32   boxaGetCount()
 *           l_int32   boxaGetValidCount()
 *           BOX      *boxaGetBox()
 *           BOX      *boxaGetValidBox()
 *           l_int32   boxaGetBoxGeometry()
 *           l_int32   boxaIsFull()
 *
 *      Boxa array modifiers
 *           l_int32   boxaReplaceBox()
 *           l_int32   boxaInsertBox()
 *           l_int32   boxaRemoveBox()
 *           l_int32   boxaRemoveBoxAndSave()
 *           BOXA     *boxaSaveValid()
 *           l_int32   boxaInitFull()
 *           l_int32   boxaClear()
 *
 *      Boxaa creation, copy, destruction
 *           BOXAA    *boxaaCreate()
 *           BOXAA    *boxaaCopy()
 *           void      boxaaDestroy()
 *
 *      Boxaa array extension
 *           l_int32   boxaaAddBoxa()
 *           l_int32   boxaaExtendArray()
 *           l_int32   boxaaExtendArrayToSize()
 *
 *      Boxaa accessors
 *           l_int32   boxaaGetCount()
 *           l_int32   boxaaGetBoxCount()
 *           BOXA     *boxaaGetBoxa()
 *           BOX      *boxaaGetBox()
 *
 *      Boxaa array modifiers
 *           l_int32   boxaaInitFull()
 *           l_int32   boxaaExtendWithInit()
 *           l_int32   boxaaReplaceBoxa()
 *           l_int32   boxaaInsertBoxa()
 *           l_int32   boxaaRemoveBoxa()
 *           l_int32   boxaaAddBox()
 *
 *      Boxaa serialized I/O
 *           BOXAA    *boxaaReadFromFiles()
 *           BOXAA    *boxaaRead()
 *           BOXAA    *boxaaReadStream()
 *           l_int32   boxaaWrite()
 *           l_int32   boxaaWriteStream()
 *
 *      Boxa serialized I/O
 *           BOXA     *boxaRead()
 *           BOXA     *boxaReadStream()
 *           BOXA     *boxaReadMem()
 *           l_int32   boxaWrite()
 *           l_int32   boxaWriteStream()
 *           l_int32   boxaWriteMem()
 *
 *      Box print (for debug)
 *           l_int32   boxPrintStreamInfo()
 *
 *   Most functions use only valid boxes, which are boxes that have both
 *   width and height > 0.  However, a few functions, such as
 *   boxaGetMedian() do not assume that all boxes are valid.  For any
 *   function that can use a boxa with invalid boxes, it is convenient
 *   to use these accessors:
 *       boxaGetValidCount()   :  count of valid boxes
 *       boxaGetValidBox()     :  returns NULL for invalid boxes
 */

#include <string.h>
#include "allheaders.h"

static const l_int32  INITIAL_PTR_ARRAYSIZE = 20;   /* n'import quoi */


/*---------------------------------------------------------------------*
 *                  Box creation, destruction and copy                 *
 *---------------------------------------------------------------------*/
/*!
 *  boxCreate()
 *
 *      Input:  x, y, w, h
 *      Return: box, or null on error
 *
 *  Notes:
 *      (1) This clips the box to the +quad.  If no part of the
 *          box is in the +quad, this returns NULL.
 *      (2) We allow you to make a box with w = 0 and/or h = 0.
 *          This does not represent a valid region, but it is useful
 *          as a placeholder in a boxa for which the index of the
 *          box in the boxa is important.  This is an atypical
 *          situation; usually you want to put only valid boxes with
 *          nonzero width and height in a boxa.  If you have a boxa
 *          with invalid boxes, the accessor boxaGetValidBox()
 *          will return NULL on each invalid box.
 *      (3) If you want to create only valid boxes, use boxCreateValid(),
 *          which returns NULL if either w or h is 0.
 */
BOX *
boxCreate(l_int32  x,
          l_int32  y,
          l_int32  w,
          l_int32  h)
{
BOX  *box;

    PROCNAME("boxCreate");

    if (w < 0 || h < 0)
        return (BOX *)ERROR_PTR("w and h not both >= 0", procName, NULL);
    if (x < 0) {  /* take part in +quad */
        w = w + x;
        x = 0;
        if (w <= 0)
            return (BOX *)ERROR_PTR("x < 0 and box off +quad", procName, NULL);
    }
    if (y < 0) {  /* take part in +quad */
        h = h + y;
        y = 0;
        if (h <= 0)
            return (BOX *)ERROR_PTR("y < 0 and box off +quad", procName, NULL);
    }

    if ((box = (BOX *)LEPT_CALLOC(1, sizeof(BOX))) == NULL)
        return (BOX *)ERROR_PTR("box not made", procName, NULL);
    boxSetGeometry(box, x, y, w, h);
    box->refcount = 1;

    return box;
}


/*!
 *  boxCreateValid()
 *
 *      Input:  x, y, w, h
 *      Return: box, or null on error
 *
 *  Notes:
 *      (1) This returns NULL if either w = 0 or h = 0.
 */
BOX *
boxCreateValid(l_int32  x,
               l_int32  y,
               l_int32  w,
               l_int32  h)
{
    PROCNAME("boxCreateValid");

    if (w <= 0 || h <= 0)
        return (BOX *)ERROR_PTR("w and h not both > 0", procName, NULL);
    return boxCreate(x, y, w, h);
}


/*!
 *  boxCopy()
 *
 *      Input:  box
 *      Return: copy of box, or null on error
 */
BOX *
boxCopy(BOX  *box)
{
BOX  *boxc;

    PROCNAME("boxCopy");

    if (!box)
        return (BOX *)ERROR_PTR("box not defined", procName, NULL);

    boxc = boxCreate(box->x, box->y, box->w, box->h);

    return boxc;
}


/*!
 *  boxClone()
 *
 *      Input:  box
 *      Return: ptr to same box, or null on error
 */
BOX *
boxClone(BOX  *box)
{

    PROCNAME("boxClone");

    if (!box)
        return (BOX *)ERROR_PTR("box not defined", procName, NULL);

    boxChangeRefcount(box, 1);
    return box;
}


/*!
 *  boxDestroy()
 *
 *      Input:  &box (<will be set to null before returning>)
 *      Return: void
 *
 *  Notes:
 *      (1) Decrements the ref count and, if 0, destroys the box.
 *      (2) Always nulls the input ptr.
 */
void
boxDestroy(BOX  **pbox)
{
BOX  *box;

    PROCNAME("boxDestroy");

    if (pbox == NULL) {
        L_WARNING("ptr address is null!\n", procName);
        return;
    }
    if ((box = *pbox) == NULL)
        return;

    boxChangeRefcount(box, -1);
    if (boxGetRefcount(box) <= 0)
        LEPT_FREE(box);
    *pbox = NULL;
    return;
}


/*---------------------------------------------------------------------*
 *                              Box accessors                          *
 *---------------------------------------------------------------------*/
/*!
 *  boxGetGeometry()
 *
 *      Input:  box
 *              &x, &y, &w, &h (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxGetGeometry(BOX      *box,
               l_int32  *px,
               l_int32  *py,
               l_int32  *pw,
               l_int32  *ph)
{
    PROCNAME("boxGetGeometry");

    if (px) *px = 0;
    if (py) *py = 0;
    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if (px) *px = box->x;
    if (py) *py = box->y;
    if (pw) *pw = box->w;
    if (ph) *ph = box->h;
    return 0;
}


/*!
 *  boxSetGeometry()
 *
 *      Input:  box
 *              x, y, w, h (use -1 to leave unchanged)
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxSetGeometry(BOX     *box,
               l_int32  x,
               l_int32  y,
               l_int32  w,
               l_int32  h)
{
    PROCNAME("boxSetGeometry");

    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if (x != -1) box->x = x;
    if (y != -1) box->y = y;
    if (w != -1) box->w = w;
    if (h != -1) box->h = h;
    return 0;
}


/*!
 *  boxGetSideLocation()
 *
 *      Input:  box
 *              side (L_GET_LEFT, L_GET_RIGHT, L_GET_TOP, L_GET_BOT)
 *              &loc (<return> location)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) All returned values are within the box.  In particular:
 *            right = left + width - 1
 *            bottom = top + height - 1
 */
l_int32
boxGetSideLocation(BOX      *box,
                   l_int32   side,
                   l_int32  *ploc)
{
l_int32  x, y, w, h;

    PROCNAME("boxGetSideLocation");

    if (!ploc)
        return ERROR_INT("&loc not defined", procName, 1);
    *ploc = 0;
    if (!box)
        return ERROR_INT("box not defined", procName, 1);

    boxGetGeometry(box, &x, &y, &w, &h);
    if (side == L_GET_LEFT)
        *ploc = x;
    else if (side == L_GET_RIGHT)
        *ploc = x + w - 1;
    else if (side == L_GET_TOP)
        *ploc = y;
    else if (side == L_GET_BOT)
        *ploc = y + h - 1;
    else
        return ERROR_INT("invalid side", procName, 1);
    return 0;
}


l_int32
boxGetRefcount(BOX  *box)
{
    PROCNAME("boxGetRefcount");

    if (!box)
        return ERROR_INT("box not defined", procName, UNDEF);

    return box->refcount;
}


l_int32
boxChangeRefcount(BOX     *box,
                  l_int32  delta)
{
    PROCNAME("boxChangeRefcount");

    if (!box)
        return ERROR_INT("box not defined", procName, 1);

    box->refcount += delta;
    return 0;
}


/*!
 *  boxIsValid()
 *
 *      Input:  box
 *              &valid (<return> 1 if valid; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxIsValid(BOX      *box,
           l_int32  *pvalid)
{
    PROCNAME("boxIsValid");

    if (!pvalid)
        return ERROR_INT("&valid not defined", procName, 1);
    *pvalid = 0;
    if (!box)
        return ERROR_INT("box not defined", procName, 1);

    if (box->w > 0 && box->h > 0)
        *pvalid = 1;
    return 0;
}


/*---------------------------------------------------------------------*
 *             Boxa creation, destruction, copy, extension             *
 *---------------------------------------------------------------------*/
/*!
 *  boxaCreate()
 *
 *      Input:  n  (initial number of ptrs)
 *      Return: boxa, or null on error
 */
BOXA *
boxaCreate(l_int32  n)
{
BOXA  *boxa;

    PROCNAME("boxaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((boxa = (BOXA *)LEPT_CALLOC(1, sizeof(BOXA))) == NULL)
        return (BOXA *)ERROR_PTR("boxa not made", procName, NULL);
    boxa->n = 0;
    boxa->nalloc = n;
    boxa->refcount = 1;

    if ((boxa->box = (BOX **)LEPT_CALLOC(n, sizeof(BOX *))) == NULL)
        return (BOXA *)ERROR_PTR("boxa ptrs not made", procName, NULL);

    return boxa;
}


/*!
 *  boxaCopy()
 *
 *      Input:  boxa
 *              copyflag (L_COPY, L_CLONE, L_COPY_CLONE)
 *      Return: new boxa, or null on error
 *
 *  Notes:
 *      (1) See pix.h for description of the copyflag.
 *      (2) The copy-clone makes a new boxa that holds clones of each box.
 */
BOXA *
boxaCopy(BOXA    *boxa,
         l_int32  copyflag)
{
l_int32  i;
BOX     *boxc;
BOXA    *boxac;

    PROCNAME("boxaCopy");

    if (!boxa)
        return (BOXA *)ERROR_PTR("boxa not defined", procName, NULL);

    if (copyflag == L_CLONE) {
        boxa->refcount++;
        return boxa;
    }

    if (copyflag != L_COPY && copyflag != L_COPY_CLONE)
        return (BOXA *)ERROR_PTR("invalid copyflag", procName, NULL);

    if ((boxac = boxaCreate(boxa->nalloc)) == NULL)
        return (BOXA *)ERROR_PTR("boxac not made", procName, NULL);
    for (i = 0; i < boxa->n; i++) {
        if (copyflag == L_COPY)
            boxc = boxaGetBox(boxa, i, L_COPY);
        else   /* copy-clone */
            boxc = boxaGetBox(boxa, i, L_CLONE);
        boxaAddBox(boxac, boxc, L_INSERT);
    }
    return boxac;
}


/*!
 *  boxaDestroy()
 *
 *      Input:  &boxa (<will be set to null before returning>)
 *      Return: void
 *
 *  Note:
 *      - Decrements the ref count and, if 0, destroys the boxa.
 *      - Always nulls the input ptr.
 */
void
boxaDestroy(BOXA  **pboxa)
{
l_int32  i;
BOXA    *boxa;

    PROCNAME("boxaDestroy");

    if (pboxa == NULL) {
        L_WARNING("ptr address is null!\n", procName);
        return;
    }

    if ((boxa = *pboxa) == NULL)
        return;

        /* Decrement the ref count.  If it is 0, destroy the boxa. */
    boxa->refcount--;
    if (boxa->refcount <= 0) {
        for (i = 0; i < boxa->n; i++)
            boxDestroy(&boxa->box[i]);
        LEPT_FREE(boxa->box);
        LEPT_FREE(boxa);
    }

    *pboxa = NULL;
    return;
}


/*!
 *  boxaAddBox()
 *
 *      Input:  boxa
 *              box  (to be added)
 *              copyflag (L_INSERT, L_COPY, L_CLONE)
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxaAddBox(BOXA    *boxa,
           BOX     *box,
           l_int32  copyflag)
{
l_int32  n;
BOX     *boxc;

    PROCNAME("boxaAddBox");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);

    if (copyflag == L_INSERT)
        boxc = box;
    else if (copyflag == L_COPY)
        boxc = boxCopy(box);
    else if (copyflag == L_CLONE)
        boxc = boxClone(box);
    else
        return ERROR_INT("invalid copyflag", procName, 1);
    if (!boxc)
        return ERROR_INT("boxc not made", procName, 1);

    n = boxaGetCount(boxa);
    if (n >= boxa->nalloc)
        boxaExtendArray(boxa);
    boxa->box[n] = boxc;
    boxa->n++;

    return 0;
}


/*!
 *  boxaExtendArray()
 *
 *      Input:  boxa
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Reallocs with doubled size of ptr array.
 */
l_int32
boxaExtendArray(BOXA  *boxa)
{
    PROCNAME("boxaExtendArray");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    return boxaExtendArrayToSize(boxa, 2 * boxa->nalloc);
}


/*!
 *  boxaExtendArrayToSize()
 *
 *      Input:  boxa
 *              size (new size of boxa array)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) If necessary, reallocs new boxa ptr array to @size.
 */
l_int32
boxaExtendArrayToSize(BOXA    *boxa,
                      l_int32  size)
{
    PROCNAME("boxaExtendArrayToSize");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    if (size > boxa->nalloc) {
        if ((boxa->box = (BOX **)reallocNew((void **)&boxa->box,
                                            sizeof(BOX *) * boxa->nalloc,
                                            size * sizeof(BOX *))) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);
        boxa->nalloc = size;
    }
    return 0;
}


/*---------------------------------------------------------------------*
 *                             Boxa accessors                          *
 *---------------------------------------------------------------------*/
/*!
 *  boxaGetCount()
 *
 *      Input:  boxa
 *      Return: count (of all boxes); 0 if no boxes or on error
 */
l_int32
boxaGetCount(BOXA  *boxa)
{
    PROCNAME("boxaGetCount");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 0);
    return boxa->n;
}


/*!
 *  boxaGetValidCount()
 *
 *      Input:  boxa
 *      Return: count (of valid boxes); 0 if no valid boxes or on error
 */
l_int32
boxaGetValidCount(BOXA  *boxa)
{
l_int32  n, i, w, h, count;

    PROCNAME("boxaGetValidCount");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 0);

    n = boxaGetCount(boxa);
    for (i = 0, count = 0; i < n; i++) {
        boxaGetBoxGeometry(boxa, i, NULL, NULL, &w, &h);
        if (w > 0 && h > 0)
            count++;
    }
    return count;
}


/*!
 *  boxaGetBox()
 *
 *      Input:  boxa
 *              index  (to the index-th box)
 *              accessflag  (L_COPY or L_CLONE)
 *      Return: box, or null on error
 */
BOX *
boxaGetBox(BOXA    *boxa,
           l_int32  index,
           l_int32  accessflag)
{
    PROCNAME("boxaGetBox");

    if (!boxa)
        return (BOX *)ERROR_PTR("boxa not defined", procName, NULL);
    if (index < 0 || index >= boxa->n)
        return (BOX *)ERROR_PTR("index not valid", procName, NULL);

    if (accessflag == L_COPY)
        return boxCopy(boxa->box[index]);
    else if (accessflag == L_CLONE)
        return boxClone(boxa->box[index]);
    else
        return (BOX *)ERROR_PTR("invalid accessflag", procName, NULL);
}


/*!
 *  boxaGetValidBox()
 *
 *      Input:  boxa
 *              index  (to the index-th box)
 *              accessflag  (L_COPY or L_CLONE)
 *      Return: box, or null if box is not valid or on error
 *
 *  Notes:
 *      (1) This returns NULL for an invalid box in a boxa.
 *          For a box to be valid, both the width and height must be > 0.
 *      (2) We allow invalid boxes, with w = 0 or h = 0, as placeholders
 *          in boxa for which the index of the box in the boxa is important.
 *          This is an atypical situation; usually you want to put only
 *          valid boxes in a boxa.
 */
BOX *
boxaGetValidBox(BOXA    *boxa,
                l_int32  index,
                l_int32  accessflag)
{
l_int32  w, h;
BOX     *box;

    PROCNAME("boxaGetValidBox");

    if (!boxa)
        return (BOX *)ERROR_PTR("boxa not defined", procName, NULL);

    if ((box = boxaGetBox(boxa, index, accessflag)) == NULL)
        return (BOX *)ERROR_PTR("box not returned", procName, NULL);
    boxGetGeometry(box, NULL, NULL, &w, &h);
    if (w <= 0 || h <= 0)  /* not valid, but not necessarily an error */
        boxDestroy(&box);
    return box;
}


/*!
 *  boxaGetBoxGeometry()
 *
 *      Input:  boxa
 *              index  (to the index-th box)
 *              &x, &y, &w, &h (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxaGetBoxGeometry(BOXA     *boxa,
                   l_int32   index,
                   l_int32  *px,
                   l_int32  *py,
                   l_int32  *pw,
                   l_int32  *ph)
{
BOX  *box;

    PROCNAME("boxaGetBoxGeometry");

    if (px) *px = 0;
    if (py) *py = 0;
    if (pw) *pw = 0;
    if (ph) *ph = 0;
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (index < 0 || index >= boxa->n)
        return ERROR_INT("index not valid", procName, 1);

    if ((box = boxaGetBox(boxa, index, L_CLONE)) == NULL)
        return ERROR_INT("box not found!", procName, 1);
    boxGetGeometry(box, px, py, pw, ph);
    boxDestroy(&box);
    return 0;
}


/*!
 *  boxaIsFull()
 *
 *      Input:  boxa
 *              &full (return> 1 if boxa is full)
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxaIsFull(BOXA     *boxa,
           l_int32  *pfull)
{
l_int32  i, n, full;
BOX     *box;

    PROCNAME("boxaIsFull");

    if (!pfull)
        return ERROR_INT("&full not defined", procName, 1);
    *pfull = 0;
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    n = boxaGetCount(boxa);
    full = 1;
    for (i = 0; i < n; i++) {
        if ((box = boxaGetBox(boxa, i, L_CLONE)) == NULL) {
            full = 0;
            break;
        }
        boxDestroy(&box);
    }
    *pfull = full;
    return 0;
}


/*---------------------------------------------------------------------*
 *                        Boxa array modifiers                         *
 *---------------------------------------------------------------------*/
/*!
 *  boxaReplaceBox()
 *
 *      Input:  boxa
 *              index  (to the index-th box)
 *              box (insert to replace existing one)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) In-place replacement of one box.
 *      (2) The previous box at that location, if any, is destroyed.
 */
l_int32
boxaReplaceBox(BOXA    *boxa,
               l_int32  index,
               BOX     *box)
{
    PROCNAME("boxaReplaceBox");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (index < 0 || index >= boxa->n)
        return ERROR_INT("index not valid", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);

    boxDestroy(&(boxa->box[index]));
    boxa->box[index] = box;
    return 0;
}


/*!
 *  boxaInsertBox()
 *
 *      Input:  boxa
 *              index (location in boxa to insert new value)
 *              box (new box to be inserted)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This shifts box[i] --> box[i + 1] for all i >= index,
 *          and then inserts box as box[index].
 *      (2) To insert at the beginning of the array, set index = 0.
 *      (3) To append to the array, it's easier to use boxaAddBox().
 *      (4) This should not be used repeatedly to insert into large arrays,
 *          because the function is O(n).
 */
l_int32
boxaInsertBox(BOXA    *boxa,
              l_int32  index,
              BOX     *box)
{
l_int32  i, n;
BOX    **array;

    PROCNAME("boxaInsertBox");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    n = boxaGetCount(boxa);
    if (index < 0 || index > n)
        return ERROR_INT("index not in {0...n}", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);

    if (n >= boxa->nalloc)
        boxaExtendArray(boxa);
    array = boxa->box;
    boxa->n++;
    for (i = n; i > index; i--)
        array[i] = array[i - 1];
    array[index] = box;

    return 0;
}


/*!
 *  boxaRemoveBox()
 *
 *      Input:  boxa
 *              index (of box to be removed)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This removes box[index] and then shifts
 *          box[i] --> box[i - 1] for all i > index.
 *      (2) It should not be used repeatedly to remove boxes from
 *          large arrays, because the function is O(n).
 */
l_int32
boxaRemoveBox(BOXA    *boxa,
              l_int32  index)
{
l_int32  i, n;
BOX    **array;

    PROCNAME("boxaRemoveBox");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    n = boxaGetCount(boxa);
    if (index < 0 || index >= n)
        return ERROR_INT("index not in {0...n - 1}", procName, 1);

    array = boxa->box;
    boxDestroy(&array[index]);
    for (i = index + 1; i < n; i++)
        array[i - 1] = array[i];
    array[n - 1] = NULL;
    boxa->n--;

    return 0;
}


/*!
 *  boxaRemoveBoxAndSave()
 *
 *      Input:  boxa
 *              index (of box to be removed)
 *              &box (<optional return> removed box)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This removes box[index] and then shifts
 *          box[i] --> box[i - 1] for all i > index.
 *      (2) It should not be used repeatedly to remove boxes from
 *          large arrays, because the function is O(n).
 */
l_int32
boxaRemoveBoxAndSave(BOXA    *boxa,
                     l_int32  index,
                     BOX    **pbox)
{
l_int32  i, n;
BOX    **array;

    PROCNAME("boxaRemoveBoxAndSave");

    if (pbox) *pbox = NULL;
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    n = boxaGetCount(boxa);
    if (index < 0 || index >= n)
        return ERROR_INT("index not in {0...n - 1}", procName, 1);

    if (pbox)
        *pbox = boxaGetBox(boxa, index, L_CLONE);
    array = boxa->box;
    boxDestroy(&array[index]);
    for (i = index + 1; i < n; i++)
        array[i - 1] = array[i];
    array[n - 1] = NULL;
    boxa->n--;

    return 0;
}


/*!
 *  boxaSaveValid()
 *
 *      Input:  boxa
 *              copyflag (L_COPY or L_CLONE)
 *      Return: boxad if OK, null on error
 *
 *  Notes:
 *      (1) This makes a copy/clone of each valid box.
 */
BOXA *
boxaSaveValid(BOXA    *boxas,
              l_int32  copyflag)
{
l_int32  i, n;
BOX     *box;
BOXA    *boxad;

    PROCNAME("boxaSaveValid");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (copyflag != L_COPY && copyflag != L_CLONE)
        return (BOXA *)ERROR_PTR("invalid copyflag", procName, NULL);

    n = boxaGetCount(boxas);
    boxad = boxaCreate(n);
    for (i = 0; i < n; i++) {
        if ((box = boxaGetValidBox(boxas, i, copyflag)) != NULL)
            boxaAddBox(boxad, box, L_INSERT);
    }

    return boxad;
}


/*!
 *  boxaInitFull()
 *
 *      Input:  boxa (typically empty)
 *              box (<optional> to be replicated into the entire ptr array)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This initializes a boxa by filling up the entire box ptr array
 *          with copies of @box.  If @box == NULL, use a placeholder box
 *          of zero size.  Any existing boxes are destroyed.
 *          After this opepration, the number of boxes is equal to
 *          the number of allocated ptrs.
 *      (2) Note that we use boxaReplaceBox() instead of boxaInsertBox().
 *          They both have the same effect when inserting into a NULL ptr
 *          in the boxa ptr array:
 *      (3) Example usage.  This function is useful to prepare for a
 *          random insertion (or replacement) of boxes into a boxa.
 *          To randomly insert boxes into a boxa, up to some index "max":
 *             Boxa *boxa = boxaCreate(max);
 *             boxaInitFull(boxa, NULL);
 *          If you want placeholder boxes of non-zero size:
 *             Boxa *boxa = boxaCreate(max);
 *             Box *box = boxCreate(...);
 *             boxaInitFull(boxa, box);
 *             boxDestroy(&box);
 *          If we have an existing boxa with a smaller ptr array, it can
 *          be reused for up to max boxes:
 *             boxaExtendArrayToSize(boxa, max);
 *             boxaInitFull(boxa, NULL);
 *          The initialization allows the boxa to always be properly
 *          filled, even if all the boxes are not later replaced.
 *          If you want to know which boxes have been replaced,
 *          and you initialized with invalid zero-sized boxes,
 *          use boxaGetValidBox() to return NULL for the invalid boxes.
 */
l_int32
boxaInitFull(BOXA  *boxa,
             BOX   *box)
{
l_int32  i, n;
BOX     *boxt;

    PROCNAME("boxaInitFull");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    n = boxa->nalloc;
    boxa->n = n;
    for (i = 0; i < n; i++) {
        if (box)
            boxt = boxCopy(box);
        else
            boxt = boxCreate(0, 0, 0, 0);
        boxaReplaceBox(boxa, i, boxt);
    }
    return 0;
}


/*!
 *  boxaClear()
 *
 *      Input:  boxa
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This destroys all boxes in the boxa, setting the ptrs
 *          to null.  The number of allocated boxes, n, is set to 0.
 */
l_int32
boxaClear(BOXA  *boxa)
{
l_int32  i, n;

    PROCNAME("boxaClear");

    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    n = boxaGetCount(boxa);
    for (i = 0; i < n; i++)
        boxDestroy(&boxa->box[i]);
    boxa->n = 0;
    return 0;
}


/*--------------------------------------------------------------------------*
 *                     Boxaa creation, destruction                          *
 *--------------------------------------------------------------------------*/
/*!
 *  boxaaCreate()
 *
 *      Input:  size of boxa ptr array to be alloc'd (0 for default)
 *      Return: baa, or null on error
 */
BOXAA *
boxaaCreate(l_int32  n)
{
BOXAA  *baa;

    PROCNAME("boxaaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((baa = (BOXAA *)LEPT_CALLOC(1, sizeof(BOXAA))) == NULL)
        return (BOXAA *)ERROR_PTR("baa not made", procName, NULL);
    if ((baa->boxa = (BOXA **)LEPT_CALLOC(n, sizeof(BOXA *))) == NULL)
        return (BOXAA *)ERROR_PTR("boxa ptr array not made", procName, NULL);

    baa->nalloc = n;
    baa->n = 0;

    return baa;
}


/*!
 *  boxaaCopy()
 *
 *      Input:  baas (input boxaa to be copied)
 *              copyflag (L_COPY, L_CLONE)
 *      Return: baad (new boxaa, composed of copies or clones of the boxa
 *                    in baas), or null on error
 *
 *  Notes:
 *      (1) L_COPY makes a copy of each boxa in baas.
 *          L_CLONE makes a clone of each boxa in baas.
 */
BOXAA *
boxaaCopy(BOXAA   *baas,
          l_int32  copyflag)
{
l_int32  i, n;
BOXA    *boxa;
BOXAA   *baad;

    PROCNAME("boxaaCopy");

    if (!baas)
        return (BOXAA *)ERROR_PTR("baas not defined", procName, NULL);
    if (copyflag != L_COPY && copyflag != L_CLONE)
        return (BOXAA *)ERROR_PTR("invalid copyflag", procName, NULL);

    n = boxaaGetCount(baas);
    baad = boxaaCreate(n);
    for (i = 0; i < n; i++) {
        boxa = boxaaGetBoxa(baas, i, copyflag);
        boxaaAddBoxa(baad, boxa, L_INSERT);
    }

    return baad;
}


/*!
 *  boxaaDestroy()
 *
 *      Input:  &boxaa (<will be set to null before returning>)
 *      Return: void
 */
void
boxaaDestroy(BOXAA  **pbaa)
{
l_int32  i;
BOXAA   *baa;

    PROCNAME("boxaaDestroy");

    if (pbaa == NULL) {
        L_WARNING("ptr address is NULL!\n", procName);
        return;
    }

    if ((baa = *pbaa) == NULL)
        return;

    for (i = 0; i < baa->n; i++)
        boxaDestroy(&baa->boxa[i]);
    LEPT_FREE(baa->boxa);
    LEPT_FREE(baa);
    *pbaa = NULL;

    return;
}



/*--------------------------------------------------------------------------*
 *                              Add Boxa to Boxaa                           *
 *--------------------------------------------------------------------------*/
/*!
 *  boxaaAddBoxa()
 *
 *      Input:  boxaa
 *              boxa     (to be added)
 *              copyflag  (L_INSERT, L_COPY, L_CLONE)
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxaaAddBoxa(BOXAA   *baa,
             BOXA    *ba,
             l_int32  copyflag)
{
l_int32  n;
BOXA    *bac;

    PROCNAME("boxaaAddBoxa");

    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);
    if (!ba)
        return ERROR_INT("ba not defined", procName, 1);
    if (copyflag != L_INSERT && copyflag != L_COPY && copyflag != L_CLONE)
        return ERROR_INT("invalid copyflag", procName, 1);

    if (copyflag == L_INSERT)
        bac = ba;
    else
        bac = boxaCopy(ba, copyflag);

    n = boxaaGetCount(baa);
    if (n >= baa->nalloc)
        boxaaExtendArray(baa);
    baa->boxa[n] = bac;
    baa->n++;
    return 0;
}


/*!
 *  boxaaExtendArray()
 *
 *      Input:  boxaa
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxaaExtendArray(BOXAA  *baa)
{

    PROCNAME("boxaaExtendArray");

    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);

    if ((baa->boxa = (BOXA **)reallocNew((void **)&baa->boxa,
                              sizeof(BOXA *) * baa->nalloc,
                              2 * sizeof(BOXA *) * baa->nalloc)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);

    baa->nalloc *= 2;
    return 0;
}


/*!
 *  boxaaExtendArrayToSize()
 *
 *      Input:  boxaa
 *              size (new size of boxa array)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) If necessary, reallocs the boxa ptr array to @size.
 */
l_int32
boxaaExtendArrayToSize(BOXAA   *baa,
                       l_int32  size)
{
    PROCNAME("boxaaExtendArrayToSize");

    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);

    if (size > baa->nalloc) {
        if ((baa->boxa = (BOXA **)reallocNew((void **)&baa->boxa,
                                             sizeof(BOXA *) * baa->nalloc,
                                             size * sizeof(BOXA *))) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);
        baa->nalloc = size;
    }
    return 0;
}


/*----------------------------------------------------------------------*
 *                           Boxaa accessors                            *
 *----------------------------------------------------------------------*/
/*!
 *  boxaaGetCount()
 *
 *      Input:  boxaa
 *      Return: count (number of boxa), or 0 if no boxa or on error
 */
l_int32
boxaaGetCount(BOXAA  *baa)
{
    PROCNAME("boxaaGetCount");

    if (!baa)
        return ERROR_INT("baa not defined", procName, 0);
    return baa->n;
}


/*!
 *  boxaaGetBoxCount()
 *
 *      Input:  boxaa
 *      Return: count (number of boxes), or 0 if no boxes or on error
 */
l_int32
boxaaGetBoxCount(BOXAA  *baa)
{
BOXA    *boxa;
l_int32  n, sum, i;

    PROCNAME("boxaaGetBoxCount");

    if (!baa)
        return ERROR_INT("baa not defined", procName, 0);

    n = boxaaGetCount(baa);
    for (sum = 0, i = 0; i < n; i++) {
        boxa = boxaaGetBoxa(baa, i, L_CLONE);
        sum += boxaGetCount(boxa);
        boxaDestroy(&boxa);
    }

    return sum;
}


/*!
 *  boxaaGetBoxa()
 *
 *      Input:  boxaa
 *              index  (to the index-th boxa)
 *              accessflag   (L_COPY or L_CLONE)
 *      Return: boxa, or null on error
 */
BOXA *
boxaaGetBoxa(BOXAA   *baa,
             l_int32  index,
             l_int32  accessflag)
{
l_int32  n;

    PROCNAME("boxaaGetBoxa");

    if (!baa)
        return (BOXA *)ERROR_PTR("baa not defined", procName, NULL);
    n = boxaaGetCount(baa);
    if (index < 0 || index >= n)
        return (BOXA *)ERROR_PTR("index not valid", procName, NULL);
    if (accessflag != L_COPY && accessflag != L_CLONE)
        return (BOXA *)ERROR_PTR("invalid accessflag", procName, NULL);

    return boxaCopy(baa->boxa[index], accessflag);
}


/*!
 *  boxaaGetBox()
 *
 *      Input:  baa
 *              iboxa  (index into the boxa array in the boxaa)
 *              ibox  (index into the box array in the boxa)
 *              accessflag   (L_COPY or L_CLONE)
 *      Return: box, or null on error
 */
BOX *
boxaaGetBox(BOXAA   *baa,
            l_int32  iboxa,
            l_int32  ibox,
            l_int32  accessflag)
{
BOX   *box;
BOXA  *boxa;

    PROCNAME("boxaaGetBox");

    if ((boxa = boxaaGetBoxa(baa, iboxa, L_CLONE)) == NULL)
        return (BOX *)ERROR_PTR("boxa not retrieved", procName, NULL);
    if ((box = boxaGetBox(boxa, ibox, accessflag)) == NULL)
        L_ERROR("box not retrieved\n", procName);
    boxaDestroy(&boxa);
    return box;
}


/*----------------------------------------------------------------------*
 *                           Boxaa array modifiers                      *
 *----------------------------------------------------------------------*/
/*!
 *  boxaaInitFull()
 *
 *      Input:  boxaa (typically empty)
 *              boxa (to be replicated into the entire ptr array)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This initializes a boxaa by filling up the entire boxa ptr array
 *          with copies of @boxa.  Any existing boxa are destroyed.
 *          After this operation, the number of boxa is equal to
 *          the number of allocated ptrs.
 *      (2) Note that we use boxaaReplaceBox() instead of boxaInsertBox().
 *          They both have the same effect when inserting into a NULL ptr
 *          in the boxa ptr array
 *      (3) Example usage.  This function is useful to prepare for a
 *          random insertion (or replacement) of boxa into a boxaa.
 *          To randomly insert boxa into a boxaa, up to some index "max":
 *             Boxaa *baa = boxaaCreate(max);
 *               // initialize the boxa
 *             Boxa *boxa = boxaCreate(...);
 *             ...  [optionally fix with boxes]
 *             boxaaInitFull(baa, boxa);
 *          A typical use is to initialize the array with empty boxa,
 *          and to replace only a subset that must be aligned with
 *          something else, such as a pixa.
 */
l_int32
boxaaInitFull(BOXAA  *baa,
              BOXA   *boxa)
{
l_int32  i, n;
BOXA    *boxat;

    PROCNAME("boxaaInitFull");

    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    n = baa->nalloc;
    baa->n = n;
    for (i = 0; i < n; i++) {
        boxat = boxaCopy(boxa, L_COPY);
        boxaaReplaceBoxa(baa, i, boxat);
    }
    return 0;
}


/*!
 *  boxaaExtendWithInit()
 *
 *      Input:  boxaa
 *              maxindex
 *              boxa (to be replicated into the extended ptr array)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This should be used on an existing boxaa that has been
 *          fully loaded with boxa.  It then extends the boxaa,
 *          loading all the additional ptrs with copies of boxa.
 *          Typically, boxa will be empty.
 */
l_int32
boxaaExtendWithInit(BOXAA   *baa,
                    l_int32  maxindex,
                    BOXA    *boxa)
{
l_int32  i, n;

    PROCNAME("boxaaExtendWithInit");

    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

        /* Extend the ptr array if necessary */
    n = boxaaGetCount(baa);
    if (maxindex < n) return 0;
    boxaaExtendArrayToSize(baa, maxindex + 1);

        /* Fill the new entries with copies of boxa */
    for (i = n; i <= maxindex; i++)
        boxaaAddBoxa(baa, boxa, L_COPY);
    return 0;
}


/*!
 *  boxaaReplaceBoxa()
 *
 *      Input:  boxaa
 *              index  (to the index-th boxa)
 *              boxa (insert and replace any existing one)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Any existing boxa is destroyed, and the input one
 *          is inserted in its place.
 *      (2) If the index is invalid, return 1 (error)
 */
l_int32
boxaaReplaceBoxa(BOXAA   *baa,
                 l_int32  index,
                 BOXA    *boxa)
{
l_int32  n;

    PROCNAME("boxaaReplaceBoxa");

    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    n = boxaaGetCount(baa);
    if (index < 0 || index >= n)
        return ERROR_INT("index not valid", procName, 1);

    boxaDestroy(&baa->boxa[index]);
    baa->boxa[index] = boxa;
    return 0;
}


/*!
 *  boxaaInsertBoxa()
 *
 *      Input:  boxaa
 *              index (location in boxaa to insert new boxa)
 *              boxa (new boxa to be inserted)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This shifts boxa[i] --> boxa[i + 1] for all i >= index,
 *          and then inserts boxa as boxa[index].
 *      (2) To insert at the beginning of the array, set index = 0.
 *      (3) To append to the array, it's easier to use boxaaAddBoxa().
 *      (4) This should not be used repeatedly to insert into large arrays,
 *          because the function is O(n).
 */
l_int32
boxaaInsertBoxa(BOXAA   *baa,
                l_int32  index,
                BOXA    *boxa)
{
l_int32  i, n;
BOXA   **array;

    PROCNAME("boxaaInsertBoxa");

    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);
    n = boxaaGetCount(baa);
    if (index < 0 || index > n)
        return ERROR_INT("index not in {0...n}", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    if (n >= baa->nalloc)
        boxaaExtendArray(baa);
    array = baa->boxa;
    baa->n++;
    for (i = n; i > index; i--)
        array[i] = array[i - 1];
    array[index] = boxa;

    return 0;
}


/*!
 *  boxaaRemoveBoxa()
 *
 *      Input:  boxaa
 *              index  (of the boxa to be removed)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This removes boxa[index] and then shifts
 *          boxa[i] --> boxa[i - 1] for all i > index.
 *      (2) The removed boxaa is destroyed.
 *      (2) This should not be used repeatedly on large arrays,
 *          because the function is O(n).
 */
l_int32
boxaaRemoveBoxa(BOXAA   *baa,
                l_int32  index)
{
l_int32  i, n;
BOXA   **array;

    PROCNAME("boxaaRemoveBox");

    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);
    n = boxaaGetCount(baa);
    if (index < 0 || index >= n)
        return ERROR_INT("index not valid", procName, 1);

    array = baa->boxa;
    boxaDestroy(&array[index]);
    for (i = index + 1; i < n; i++)
        array[i - 1] = array[i];
    array[n - 1] = NULL;
    baa->n--;

    return 0;
}


/*!
 *  boxaaAddBox()
 *
 *      Input:  boxaa
 *              index (of boxa with boxaa)
 *              box (to be added)
 *              accessflag (L_INSERT, L_COPY or L_CLONE)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Adds to an existing boxa only.
 */
l_int32
boxaaAddBox(BOXAA   *baa,
            l_int32  index,
            BOX     *box,
            l_int32  accessflag)
{
l_int32  n;
BOXA    *boxa;
    PROCNAME("boxaaAddBox");

    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);
    n = boxaaGetCount(baa);
    if (index < 0 || index >= n)
        return ERROR_INT("index not valid", procName, 1);
    if (accessflag != L_INSERT && accessflag != L_COPY && accessflag != L_CLONE)
        return ERROR_INT("invalid accessflag", procName, 1);

    boxa = boxaaGetBoxa(baa, index, L_CLONE);
    boxaAddBox(boxa, box, accessflag);
    boxaDestroy(&boxa);
    return 0;
}


/*---------------------------------------------------------------------*
 *                        Boxaa serialized I/O                         *
 *---------------------------------------------------------------------*/
/*!
 *  boxaaReadFromFiles()
 *
 *      Input:  dirname (directory)
 *              substr (<optional> substring filter on filenames; can be NULL)
 *              first (0-based)
 *              nfiles (use 0 for everything from @first to the end)
 *      Return: baa, or null on error or if no boxa files are found.
 *
 *  Notes:
 *      (1) The files must be serialized boxa files (e.g., *.ba).
 *          If some files cannot be read, warnings are issued.
 *      (2) Use @substr to filter filenames in the directory.  If
 *          @substr == NULL, this takes all files.
 *      (3) After filtering, use @first and @nfiles to select
 *          a contiguous set of files, that have been lexically
 *          sorted in increasing order.
 */
BOXAA *
boxaaReadFromFiles(const char  *dirname,
                   const char  *substr,
                   l_int32      first,
                   l_int32      nfiles)
{
char    *fname;
l_int32  i, n;
BOXA    *boxa;
BOXAA   *baa;
SARRAY  *sa;

  PROCNAME("boxaaReadFromFiles");

  if (!dirname)
      return (BOXAA *)ERROR_PTR("dirname not defined", procName, NULL);

  sa = getSortedPathnamesInDirectory(dirname, substr, first, nfiles);
  if (!sa || ((n = sarrayGetCount(sa)) == 0)) {
      sarrayDestroy(&sa);
      return (BOXAA *)ERROR_PTR("no pixa files found", procName, NULL);
  }

  baa = boxaaCreate(n);
  for (i = 0; i < n; i++) {
      fname = sarrayGetString(sa, i, L_NOCOPY);
      if ((boxa = boxaRead(fname)) == NULL) {
          L_ERROR("boxa not read for %d-th file", procName, i);
          continue;
      }
      boxaaAddBoxa(baa, boxa, L_INSERT);
  }

  sarrayDestroy(&sa);
  return baa;
}


/*!
 *  boxaaRead()
 *
 *      Input:  filename
 *      Return: boxaa, or null on error
 */
BOXAA *
boxaaRead(const char  *filename)
{
FILE   *fp;
BOXAA  *baa;

    PROCNAME("boxaaRead");

    if (!filename)
        return (BOXAA *)ERROR_PTR("filename not defined", procName, NULL);
    if ((fp = fopenReadStream(filename)) == NULL)
        return (BOXAA *)ERROR_PTR("stream not opened", procName, NULL);

    if ((baa = boxaaReadStream(fp)) == NULL) {
        fclose(fp);
        return (BOXAA *)ERROR_PTR("boxaa not read", procName, NULL);
    }

    fclose(fp);
    return baa;
}


/*!
 *  boxaaReadStream()
 *
 *      Input:  stream
 *      Return: boxaa, or null on error
 */
BOXAA *
boxaaReadStream(FILE  *fp)
{
l_int32  n, i, x, y, w, h, version;
l_int32  ignore;
BOXA    *boxa;
BOXAA   *baa;

    PROCNAME("boxaaReadStream");

    if (!fp)
        return (BOXAA *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nBoxaa Version %d\n", &version) != 1)
        return (BOXAA *)ERROR_PTR("not a boxaa file", procName, NULL);
    if (version != BOXAA_VERSION_NUMBER)
        return (BOXAA *)ERROR_PTR("invalid boxa version", procName, NULL);
    if (fscanf(fp, "Number of boxa = %d\n", &n) != 1)
        return (BOXAA *)ERROR_PTR("not a boxaa file", procName, NULL);

    if ((baa = boxaaCreate(n)) == NULL)
        return (BOXAA *)ERROR_PTR("boxaa not made", procName, NULL);

    for (i = 0; i < n; i++) {
        if (fscanf(fp, "\nBoxa[%d] extent: x = %d, y = %d, w = %d, h = %d",
                   &ignore, &x, &y, &w, &h) != 5)
            return (BOXAA *)ERROR_PTR("boxa descr not valid", procName, NULL);
        if ((boxa = boxaReadStream(fp)) == NULL)
            return (BOXAA *)ERROR_PTR("boxa not made", procName, NULL);
        boxaaAddBoxa(baa, boxa, L_INSERT);
    }

    return baa;
}

/*!
 *  boxaaWrite()
 *
 *      Input:  filename
 *              boxaa
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxaaWrite(const char  *filename,
           BOXAA       *baa)
{
FILE  *fp;

    PROCNAME("boxaaWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "w")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (boxaaWriteStream(fp, baa))
        return ERROR_INT("baa not written to stream", procName, 1);
    fclose(fp);

    return 0;
}


/*!
 *  boxaaWriteStream()
 *
 *      Input: stream
 *             boxaa
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxaaWriteStream(FILE   *fp,
                 BOXAA  *baa)
{
l_int32  n, i, x, y, w, h;
BOX     *box;
BOXA    *boxa;

    PROCNAME("boxaaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!baa)
        return ERROR_INT("baa not defined", procName, 1);

    n = boxaaGetCount(baa);
    fprintf(fp, "\nBoxaa Version %d\n", BOXAA_VERSION_NUMBER);
    fprintf(fp, "Number of boxa = %d\n", n);

    for (i = 0; i < n; i++) {
        if ((boxa = boxaaGetBoxa(baa, i, L_CLONE)) == NULL)
            return ERROR_INT("boxa not found", procName, 1);
        boxaGetExtent(boxa, NULL, NULL, &box);
        boxGetGeometry(box, &x, &y, &w, &h);
        fprintf(fp, "\nBoxa[%d] extent: x = %d, y = %d, w = %d, h = %d",
                i, x, y, w, h);
        boxaWriteStream(fp, boxa);
        boxDestroy(&box);
        boxaDestroy(&boxa);
    }
    return 0;
}


/*---------------------------------------------------------------------*
 *                         Boxa serialized I/O                         *
 *---------------------------------------------------------------------*/
/*!
 *  boxaRead()
 *
 *      Input:  filename
 *      Return: boxa, or null on error
 */
BOXA *
boxaRead(const char  *filename)
{
FILE  *fp;
BOXA  *boxa;

    PROCNAME("boxaRead");

    if (!filename)
        return (BOXA *)ERROR_PTR("filename not defined", procName, NULL);
    if ((fp = fopenReadStream(filename)) == NULL)
        return (BOXA *)ERROR_PTR("stream not opened", procName, NULL);

    if ((boxa = boxaReadStream(fp)) == NULL) {
        fclose(fp);
        return (BOXA *)ERROR_PTR("boxa not read", procName, NULL);
    }

    fclose(fp);
    return boxa;
}


/*!
 *  boxaReadStream()
 *
 *      Input:  stream
 *      Return: boxa, or null on error
 */
BOXA *
boxaReadStream(FILE  *fp)
{
l_int32  n, i, x, y, w, h, version;
l_int32  ignore;
BOX     *box;
BOXA    *boxa;

    PROCNAME("boxaReadStream");

    if (!fp)
        return (BOXA *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nBoxa Version %d\n", &version) != 1)
        return (BOXA *)ERROR_PTR("not a boxa file", procName, NULL);
    if (version != BOXA_VERSION_NUMBER)
        return (BOXA *)ERROR_PTR("invalid boxa version", procName, NULL);
    if (fscanf(fp, "Number of boxes = %d\n", &n) != 1)
        return (BOXA *)ERROR_PTR("not a boxa file", procName, NULL);

    if ((boxa = boxaCreate(n)) == NULL)
        return (BOXA *)ERROR_PTR("boxa not made", procName, NULL);

    for (i = 0; i < n; i++) {
        if (fscanf(fp, "  Box[%d]: x = %d, y = %d, w = %d, h = %d\n",
                &ignore, &x, &y, &w, &h) != 5)
            return (BOXA *)ERROR_PTR("box descr not valid", procName, NULL);
        if ((box = boxCreate(x, y, w, h)) == NULL)
            return (BOXA *)ERROR_PTR("box not made", procName, NULL);
        boxaAddBox(boxa, box, L_INSERT);
    }

    return boxa;
}


/*!
 *  boxaReadMem()
 *
 *      Input:  data (ascii)
 *              size (of data; can use strlen to get it)
 *      Return: boxa, or null on error
 */
BOXA *
boxaReadMem(const l_uint8  *data,
            size_t          size)
{
FILE  *fp;
BOXA  *boxa;

    PROCNAME("boxaReadMem");

    if (!data)
        return (BOXA *)ERROR_PTR("data not defined", procName, NULL);

        /* De-serialize: write serialized data to file and read back as boxa.
         * We are writing to file first, instead of reading from the memory
         * buffer, because the gnu extension fmemopen() is not available
         * with other runtimes. */
    if ((fp = tmpfile()) == NULL)
        return (BOXA *)ERROR_PTR("tmpfile stream not opened", procName, NULL);
    fwrite(data, 1, size, fp);
    rewind(fp);
    boxa = boxaReadStream(fp);
    fclose(fp);
    if (!boxa) L_ERROR("boxa not read\n", procName);
    return boxa;
}


/*!
 *  boxaWrite()
 *
 *      Input:  filename
 *              boxa
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxaWrite(const char  *filename,
          BOXA        *boxa)
{
FILE  *fp;

    PROCNAME("boxaWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "w")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (boxaWriteStream(fp, boxa))
        return ERROR_INT("boxa not written to stream", procName, 1);
    fclose(fp);

    return 0;
}


/*!
 *  boxaWriteStream()
 *
 *      Input: stream
 *             boxa
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxaWriteStream(FILE  *fp,
                BOXA  *boxa)
{
l_int32  n, i;
BOX     *box;

    PROCNAME("boxaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);

    n = boxaGetCount(boxa);
    fprintf(fp, "\nBoxa Version %d\n", BOXA_VERSION_NUMBER);
    fprintf(fp, "Number of boxes = %d\n", n);
    for (i = 0; i < n; i++) {
        if ((box = boxaGetBox(boxa, i, L_CLONE)) == NULL)
            return ERROR_INT("box not found", procName, 1);
        fprintf(fp, "  Box[%d]: x = %d, y = %d, w = %d, h = %d\n",
                i, box->x, box->y, box->w, box->h);
        boxDestroy(&box);
    }
    return 0;
}


/*!
 *  boxaWriteMem()
 *
 *      Input:  &data (<return> data of serialized boxa; ascii)
 *              &size (<return> size of returned data)
 *              boxa
 *      Return: 0 if OK, 1 on error
 */
l_int32
boxaWriteMem(l_uint8  **pdata,
             size_t    *psize,
             BOXA      *boxa)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("boxaWriteMem");

    if (!pdata)
        return ERROR_INT("&data not defined", procName, 1);
    *pdata = NULL;
    if (!psize)
        return ERROR_INT("&size not defined", procName, 1);
    *psize = 0;
    if (!boxa)
        return ERROR_INT("&boxa not defined", procName, 1);

        /* Serialize: write to file and read serialized data back into memory */
    if ((fp = tmpfile()) == NULL)
        return ERROR_INT("tmpfile stream not opened", procName, 1);
    ret = boxaWriteStream(fp, boxa);
    rewind(fp);
    *pdata = l_binaryReadStream(fp, psize);
    fclose(fp);
    return ret;
}


/*---------------------------------------------------------------------*
 *                            Debug printing                           *
 *---------------------------------------------------------------------*/
/*!
 *  boxPrintStreamInfo()
 *
 *      Input:  stream
 *              box
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This outputs debug info.  Use serialization functions to
 *          write to file if you want to read the data back.
 */
l_int32
boxPrintStreamInfo(FILE  *fp,
                   BOX   *box)
{
    PROCNAME("boxPrintStreamInfo");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);

    fprintf(fp, " Box: x = %d, y = %d, w = %d, h = %d\n",
            box->x, box->y, box->w, box->h);
    return 0;
}
