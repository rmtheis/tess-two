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
 *  sel1.c
 *
 *      Basic ops on Sels and Selas
 *
 *         Create/destroy/copy:
 *            SELA      *selaCreate()
 *            void       selaDestroy()
 *            SEL       *selCreate()
 *            void       selDestroy()
 *            SEL       *selCopy()
 *            SEL       *selCreateBrick()
 *            SEL       *selCreateComb()
 *
 *         Helper proc:
 *            l_int32  **create2dIntArray()
 *
 *         Extension of sela:
 *            SELA      *selaAddSel()
 *            static l_int32  selaExtendArray()
 *
 *         Accessors:
 *            l_int32    selaGetCount()
 *            SEL       *selaGetSel()
 *            char      *selGetName()
 *            l_int32    selSetName()
 *            l_int32    selaFindSelByName()
 *            l_int32    selGetElement()
 *            l_int32    selSetElement()
 *            l_int32    selGetParameters()
 *            l_int32    selSetOrigin()
 *            l_int32    selGetTypeAtOrigin()
 *            char      *selaGetBrickName()
 *            char      *selaGetCombName()
 *     static char      *selaComputeCompositeParameters()
 *            l_int32    getCompositeParameters()
 *            SARRAY    *selaGetSelnames()
 *
 *         Max translations for erosion and hmt
 *            l_int32    selFindMaxTranslations()
 *
 *         Rotation by multiples of 90 degrees
 *            SEL       *selRotateOrth()
 *
 *         Sela and Sel serialized I/O
 *            SELA      *selaRead()
 *            SELA      *selaReadStream()
 *            SEL       *selRead()
 *            SEL       *selReadStream()
 *            l_int32    selaWrite()
 *            l_int32    selaWriteStream()
 *            l_int32    selWrite()
 *            l_int32    selWriteStream()
 *
 *         Building custom hit-miss sels from compiled strings
 *            SEL       *selCreateFromString()
 *            char      *selPrintToString()     [for debugging]
 *
 *         Building custom hit-miss sels from a simple file format
 *            SELA      *selaCreateFromFile()
 *            static SEL *selCreateFromSArray()
 *
 *         Making hit-only sels from Pta and Pix
 *            SEL       *selCreateFromPta()
 *            SEL       *selCreateFromPix()
 *
 *         Making hit-miss sels from Pix and image files
 *            SEL       *selReadFromColorImage()
 *            SEL       *selCreateFromColorPix()
 *
 *         Printable display of sel
 *            PIX       *selDisplayInPix()
 *            PIX       *selaDisplayInPix()
 *
 *     Usage notes:
 *        In this file we have seven functions that make sels:
 *          (1)  selCreate(), with input (h, w, [name])
 *               The generic function.  Roll your own, using selSetElement().
 *          (2)  selCreateBrick(), with input (h, w, cy, cx, val)
 *               The most popular function.  Makes a rectangular sel of
 *               all hits, misses or don't-cares.  We have many morphology
 *               operations that create a sel of all hits, use it, and
 *               destroy it.
 *          (3)  selCreateFromString() with input (text, h, w, [name])
 *               Adam Langley's clever function, allows you to make a hit-miss
 *               sel from a string in code that is geometrically laid out
 *               just like the actual sel.
 *          (4)  selaCreateFromFile() with input (filename)
 *               This parses a simple file format to create an array of
 *               hit-miss sels.  The sel data uses the same encoding
 *               as in (3), with geometrical layout enforced.
 *          (5)  selCreateFromPta() with input (pta, cy, cx, [name])
 *               Another way to make a sel with only hits.
 *          (6)  selCreateFromPix() with input (pix, cy, cx, [name])
 *               Yet another way to make a sel from hits.
 *          (7)  selCreateFromColorPix() with input (pix, name).
 *               Another way to make a general hit-miss sel, starting with
 *               an image editor.
 *        In addition, there are three functions in selgen.c that
 *        automatically generate a hit-miss sel from a pix and
 *        a number of parameters.  This is useful for problems like
 *        "find all patterns that look like this one."
 *
 *        Consistency, being the hobgoblin of small minds,
 *        is adhered to here in the dimensioning and accessing of sels.
 *        Everything is done in standard matrix (row, column) order.
 *        When we set specific elements in a sel, we likewise use
 *        (row, col) ordering:
 *             selSetElement(), with input (row, col, type)
 */

#include <string.h>
#include "allheaders.h"

static const l_int32  L_BUF_SIZE = 256;
static const l_int32  INITIAL_PTR_ARRAYSIZE = 50;  /* n'import quoi */
static const l_int32  MANY_SELS = 1000;

    /* Static functions */
static l_int32 selaExtendArray(SELA *sela);
static SEL *selCreateFromSArray(SARRAY *sa, l_int32 first, l_int32 last);

struct CompParameterMap
{
    l_int32  size;
    l_int32  size1;
    l_int32  size2;
    char     selnameh1[20];
    char     selnameh2[20];
    char     selnamev1[20];
    char     selnamev2[20];
};

static const struct CompParameterMap  comp_parameter_map[] =
    { { 2, 2, 1, "sel_2h", "", "sel_2v", "" },
      { 3, 3, 1, "sel_3h", "", "sel_3v", "" },
      { 4, 2, 2, "sel_2h", "sel_comb_4h", "sel_2v", "sel_comb_4v" },
      { 5, 5, 1, "sel_5h", "", "sel_5v", "" },
      { 6, 3, 2, "sel_3h", "sel_comb_6h", "sel_3v", "sel_comb_6v" },
      { 7, 7, 1, "sel_7h", "", "sel_7v", "" },
      { 8, 4, 2, "sel_4h", "sel_comb_8h", "sel_4v", "sel_comb_8v" },
      { 9, 3, 3, "sel_3h", "sel_comb_9h", "sel_3v", "sel_comb_9v" },
      { 10, 5, 2, "sel_5h", "sel_comb_10h", "sel_5v", "sel_comb_10v" },
      { 11, 4, 3, "sel_4h", "sel_comb_12h", "sel_4v", "sel_comb_12v" },
      { 12, 4, 3, "sel_4h", "sel_comb_12h", "sel_4v", "sel_comb_12v" },
      { 13, 4, 3, "sel_4h", "sel_comb_12h", "sel_4v", "sel_comb_12v" },
      { 14, 7, 2, "sel_7h", "sel_comb_14h", "sel_7v", "sel_comb_14v" },
      { 15, 5, 3, "sel_5h", "sel_comb_15h", "sel_5v", "sel_comb_15v" },
      { 16, 4, 4, "sel_4h", "sel_comb_16h", "sel_4v", "sel_comb_16v" },
      { 17, 4, 4, "sel_4h", "sel_comb_16h", "sel_4v", "sel_comb_16v" },
      { 18, 6, 3, "sel_6h", "sel_comb_18h", "sel_6v", "sel_comb_18v" },
      { 19, 5, 4, "sel_5h", "sel_comb_20h", "sel_5v", "sel_comb_20v" },
      { 20, 5, 4, "sel_5h", "sel_comb_20h", "sel_5v", "sel_comb_20v" },
      { 21, 7, 3, "sel_7h", "sel_comb_21h", "sel_7v", "sel_comb_21v" },
      { 22, 11, 2, "sel_11h", "sel_comb_22h", "sel_11v", "sel_comb_22v" },
      { 23, 6, 4, "sel_6h", "sel_comb_24h", "sel_6v", "sel_comb_24v" },
      { 24, 6, 4, "sel_6h", "sel_comb_24h", "sel_6v", "sel_comb_24v" },
      { 25, 5, 5, "sel_5h", "sel_comb_25h", "sel_5v", "sel_comb_25v" },
      { 26, 5, 5, "sel_5h", "sel_comb_25h", "sel_5v", "sel_comb_25v" },
      { 27, 9, 3, "sel_9h", "sel_comb_27h", "sel_9v", "sel_comb_27v" },
      { 28, 7, 4, "sel_7h", "sel_comb_28h", "sel_7v", "sel_comb_28v" },
      { 29, 6, 5, "sel_6h", "sel_comb_30h", "sel_6v", "sel_comb_30v" },
      { 30, 6, 5, "sel_6h", "sel_comb_30h", "sel_6v", "sel_comb_30v" },
      { 31, 6, 5, "sel_6h", "sel_comb_30h", "sel_6v", "sel_comb_30v" },
      { 32, 8, 4, "sel_8h", "sel_comb_32h", "sel_8v", "sel_comb_32v" },
      { 33, 11, 3, "sel_11h", "sel_comb_33h", "sel_11v", "sel_comb_33v" },
      { 34, 7, 5, "sel_7h", "sel_comb_35h", "sel_7v", "sel_comb_35v" },
      { 35, 7, 5, "sel_7h", "sel_comb_35h", "sel_7v", "sel_comb_35v" },
      { 36, 6, 6, "sel_6h", "sel_comb_36h", "sel_6v", "sel_comb_36v" },
      { 37, 6, 6, "sel_6h", "sel_comb_36h", "sel_6v", "sel_comb_36v" },
      { 38, 6, 6, "sel_6h", "sel_comb_36h", "sel_6v", "sel_comb_36v" },
      { 39, 13, 3, "sel_13h", "sel_comb_39h", "sel_13v", "sel_comb_39v" },
      { 40, 8, 5, "sel_8h", "sel_comb_40h", "sel_8v", "sel_comb_40v" },
      { 41, 7, 6, "sel_7h", "sel_comb_42h", "sel_7v", "sel_comb_42v" },
      { 42, 7, 6, "sel_7h", "sel_comb_42h", "sel_7v", "sel_comb_42v" },
      { 43, 7, 6, "sel_7h", "sel_comb_42h", "sel_7v", "sel_comb_42v" },
      { 44, 11, 4, "sel_11h", "sel_comb_44h", "sel_11v", "sel_comb_44v" },
      { 45, 9, 5, "sel_9h", "sel_comb_45h", "sel_9v", "sel_comb_45v" },
      { 46, 9, 5, "sel_9h", "sel_comb_45h", "sel_9v", "sel_comb_45v" },
      { 47, 8, 6, "sel_8h", "sel_comb_48h", "sel_8v", "sel_comb_48v" },
      { 48, 8, 6, "sel_8h", "sel_comb_48h", "sel_8v", "sel_comb_48v" },
      { 49, 7, 7, "sel_7h", "sel_comb_49h", "sel_7v", "sel_comb_49v" },
      { 50, 10, 5, "sel_10h", "sel_comb_50h", "sel_10v", "sel_comb_50v" },
      { 51, 10, 5, "sel_10h", "sel_comb_50h", "sel_10v", "sel_comb_50v" },
      { 52, 13, 4, "sel_13h", "sel_comb_52h", "sel_13v", "sel_comb_52v" },
      { 53, 9, 6, "sel_9h", "sel_comb_54h", "sel_9v", "sel_comb_54v" },
      { 54, 9, 6, "sel_9h", "sel_comb_54h", "sel_9v", "sel_comb_54v" },
      { 55, 11, 5, "sel_11h", "sel_comb_55h", "sel_11v", "sel_comb_55v" },
      { 56, 8, 7, "sel_8h", "sel_comb_56h", "sel_8v", "sel_comb_56v" },
      { 57, 8, 7, "sel_8h", "sel_comb_56h", "sel_8v", "sel_comb_56v" },
      { 58, 8, 7, "sel_8h", "sel_comb_56h", "sel_8v", "sel_comb_56v" },
      { 59, 10, 6, "sel_10h", "sel_comb_60h", "sel_10v", "sel_comb_60v" },
      { 60, 10, 6, "sel_10h", "sel_comb_60h", "sel_10v", "sel_comb_60v" },
      { 61, 10, 6, "sel_10h", "sel_comb_60h", "sel_10v", "sel_comb_60v" },
      { 62, 9, 7, "sel_9h", "sel_comb_63h", "sel_9v", "sel_comb_63v" },
      { 63, 9, 7, "sel_9h", "sel_comb_63h", "sel_9v", "sel_comb_63v" } };



/*------------------------------------------------------------------------*
 *                      Create / Destroy / Copy                           *
 *------------------------------------------------------------------------*/
/*!
 *  selaCreate()
 *
 *      Input:  n (initial number of sel ptrs; use 0 for default)
 *      Return: sela, or null on error
 */
SELA *
selaCreate(l_int32  n)
{
SELA  *sela;

    PROCNAME("selaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;
    if (n > MANY_SELS)
        L_WARNING("%d sels\n", procName, n);

    if ((sela = (SELA *)CALLOC(1, sizeof(SELA))) == NULL)
        return (SELA *)ERROR_PTR("sela not made", procName, NULL);

    sela->nalloc = n;
    sela->n = 0;

        /* make array of se ptrs */
    if ((sela->sel = (SEL **)CALLOC(n, sizeof(SEL *))) == NULL)
        return (SELA *)ERROR_PTR("sel ptrs not made", procName, NULL);

    return sela;
}


/*!
 *  selaDestroy()
 *
 *      Input:  &sela (<to be nulled>)
 *      Return: void
 */
void
selaDestroy(SELA  **psela)
{
SELA    *sela;
l_int32  i;

    if (!psela) return;
    if ((sela = *psela) == NULL)
        return;

    for (i = 0; i < sela->n; i++)
        selDestroy(&sela->sel[i]);
    FREE(sela->sel);
    FREE(sela);
    *psela = NULL;
    return;
}


/*!
 *  selCreate()
 *
 *      Input:  height, width
 *              name (<optional> sel name; can be null)
 *      Return: sel, or null on error
 *
 *  Notes:
 *      (1) selCreate() initializes all values to 0.
 *      (2) After this call, (cy,cx) and nonzero data values must be
 *          assigned.  If a text name is not assigned here, it will
 *          be needed later when the sel is put into a sela.
 */
SEL *
selCreate(l_int32      height,
          l_int32      width,
          const char  *name)
{
SEL  *sel;

    PROCNAME("selCreate");

    if ((sel = (SEL *)CALLOC(1, sizeof(SEL))) == NULL)
        return (SEL *)ERROR_PTR("sel not made", procName, NULL);
    if (name)
        sel->name = stringNew(name);
    sel->sy = height;
    sel->sx = width;
    if ((sel->data = create2dIntArray(height, width)) == NULL)
        return (SEL *)ERROR_PTR("data not allocated", procName, NULL);

    return sel;
}


/*!
 *  selDestroy()
 *
 *      Input:  &sel (<to be nulled>)
 *      Return: void
 */
void
selDestroy(SEL  **psel)
{
l_int32  i;
SEL     *sel;

    PROCNAME("selDestroy");

    if (psel == NULL)  {
        L_WARNING("ptr address is NULL!\n", procName);
        return;
    }
    if ((sel = *psel) == NULL)
        return;

    for (i = 0; i < sel->sy; i++)
        FREE(sel->data[i]);
    FREE(sel->data);
    if (sel->name)
        FREE(sel->name);
    FREE(sel);

    *psel = NULL;
    return;
}


/*!
 *  selCopy()
 *
 *      Input:  sel
 *      Return: a copy of the sel, or null on error
 */
SEL *
selCopy(SEL  *sel)
{
l_int32  sx, sy, cx, cy, i, j;
SEL     *csel;

    PROCNAME("selCopy");

    if (!sel)
        return (SEL *)ERROR_PTR("sel not defined", procName, NULL);

    if ((csel = (SEL *)CALLOC(1, sizeof(SEL))) == NULL)
        return (SEL *)ERROR_PTR("csel not made", procName, NULL);
    selGetParameters(sel, &sy, &sx, &cy, &cx);
    csel->sy = sy;
    csel->sx = sx;
    csel->cy = cy;
    csel->cx = cx;

    if ((csel->data = create2dIntArray(sy, sx)) == NULL)
        return (SEL *)ERROR_PTR("sel data not made", procName, NULL);

    for (i = 0; i < sy; i++)
        for (j = 0; j < sx; j++)
            csel->data[i][j] = sel->data[i][j];

    if (sel->name)
        csel->name = stringNew(sel->name);

    return csel;
}


/*!
 *  selCreateBrick()
 *
 *      Input:  height, width
 *              cy, cx  (origin, relative to UL corner at 0,0)
 *              type  (SEL_HIT, SEL_MISS, or SEL_DONT_CARE)
 *      Return: sel, or null on error
 *
 *  Notes:
 *      (1) This is a rectangular sel of all hits, misses or don't cares.
 */
SEL *
selCreateBrick(l_int32  h,
               l_int32  w,
               l_int32  cy,
               l_int32  cx,
               l_int32  type)
{
l_int32  i, j;
SEL     *sel;

    PROCNAME("selCreateBrick");

    if (h <= 0 || w <= 0)
        return (SEL *)ERROR_PTR("h and w must both be > 0", procName, NULL);
    if (type != SEL_HIT && type != SEL_MISS && type != SEL_DONT_CARE)
        return (SEL *)ERROR_PTR("invalid sel element type", procName, NULL);

    if ((sel = selCreate(h, w, NULL)) == NULL)
        return (SEL *)ERROR_PTR("sel not made", procName, NULL);
    selSetOrigin(sel, cy, cx);
    for (i = 0; i < h; i++)
        for (j = 0; j < w; j++)
            sel->data[i][j] = type;

    return sel;
}


/*!
 *  selCreateComb()
 *
 *      Input:  factor1 (contiguous space between comb tines)
 *              factor2 (number of comb tines)
 *              direction (L_HORIZ, L_VERT)
 *      Return: sel, or null on error
 *
 *  Notes:
 *      (1) This generates a comb Sel of hits with the origin as
 *          near the center as possible.
 */
SEL *
selCreateComb(l_int32  factor1,
              l_int32  factor2,
              l_int32  direction)
{
l_int32  i, size, z;
SEL     *sel;

    PROCNAME("selCreateComb");

    if (factor1 < 1 || factor2 < 1)
        return (SEL *)ERROR_PTR("factors must be >= 1", procName, NULL);
    if (direction != L_HORIZ && direction != L_VERT)
        return (SEL *)ERROR_PTR("invalid direction", procName, NULL);

    size = factor1 * factor2;
    if (direction == L_HORIZ) {
        sel = selCreate(1, size, NULL);
        selSetOrigin(sel, 0, size / 2);
    } else {
        sel = selCreate(size, 1, NULL);
        selSetOrigin(sel, size / 2, 0);
    }

    for (i = 0; i < factor2; i++) {
        if (factor2 & 1)  /* odd */
            z = factor1 / 2 + i * factor1;
        else
            z = factor1 / 2 + i * factor1;
/*        fprintf(stderr, "i = %d, factor1 = %d, factor2 = %d, z = %d\n",
                        i, factor1, factor2, z); */
        if (direction == L_HORIZ)
            selSetElement(sel, 0, z, SEL_HIT);
        else
            selSetElement(sel, z, 0, SEL_HIT);
    }

    return sel;
}


/*!
 *  create2dIntArray()
 *
 *      Input:  sy (rows == height)
 *              sx (columns == width)
 *      Return: doubly indexed array (i.e., an array of sy row pointers,
 *              each of which points to an array of sx ints)
 *
 *  Notes:
 *      (1) The array[sy][sx] is indexed in standard "matrix notation",
 *          with the row index first.
 */
l_int32 **
create2dIntArray(l_int32  sy,
                 l_int32  sx)
{
l_int32    i;
l_int32  **array;

    PROCNAME("create2dIntArray");

    if ((array = (l_int32 **)CALLOC(sy, sizeof(l_int32 *))) == NULL)
        return (l_int32 **)ERROR_PTR("ptr array not made", procName, NULL);

    for (i = 0; i < sy; i++) {
        if ((array[i] = (l_int32 *)CALLOC(sx, sizeof(l_int32))) == NULL)
            return (l_int32 **)ERROR_PTR("array not made", procName, NULL);
    }

    return array;
}



/*------------------------------------------------------------------------*
 *                           Extension of sela                            *
 *------------------------------------------------------------------------*/
/*!
 *  selaAddSel()
 *
 *      Input:  sela
 *              sel to be added
 *              selname (ignored if already defined in sel;
 *                       req'd in sel when added to a sela)
 *              copyflag (for sel: 0 inserts, 1 copies)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This adds a sel, either inserting or making a copy.
 *      (2) Because every sel in a sela must have a name, it copies
 *          the input name if necessary.  You can input NULL for
 *          selname if the sel already has a name.
 */
l_int32
selaAddSel(SELA        *sela,
           SEL         *sel,
           const char  *selname,
           l_int32      copyflag)
{
l_int32  n;
SEL     *csel;

    PROCNAME("selaAddSel");

    if (!sela)
        return ERROR_INT("sela not defined", procName, 1);
    if (!sel)
        return ERROR_INT("sel not defined", procName, 1);
    if (!sel->name && !selname)
        return ERROR_INT("added sel must have name", procName, 1);

    if (copyflag == TRUE) {
        if ((csel = selCopy(sel)) == NULL)
            return ERROR_INT("csel not made", procName, 1);
    } else {  /* copyflag is false; insert directly */
        csel = sel;
    }
    if (!csel->name)
        csel->name = stringNew(selname);

    n = selaGetCount(sela);
    if (n >= sela->nalloc)
        selaExtendArray(sela);
    sela->sel[n] = csel;
    sela->n++;

    return 0;
}


/*!
 *  selaExtendArray()
 *
 *      Input:  sela
 *      Return: 0 if OK; 1 on error
 */
static l_int32
selaExtendArray(SELA  *sela)
{
    PROCNAME("selaExtendArray");

    if (!sela)
        return ERROR_INT("sela not defined", procName, 1);

    if ((sela->sel = (SEL **)reallocNew((void **)&sela->sel,
                              sizeof(SEL *) * sela->nalloc,
                              2 * sizeof(SEL *) * sela->nalloc)) == NULL)
            return ERROR_INT("new ptr array not returned", procName, 1);

    sela->nalloc = 2 * sela->nalloc;
    return 0;
}



/*----------------------------------------------------------------------*
 *                               Accessors                              *
 *----------------------------------------------------------------------*/
/*!
 *  selaGetCount()
 *
 *      Input:  sela
 *      Return: count, or 0 on error
 */
l_int32
selaGetCount(SELA  *sela)
{
    PROCNAME("selaGetCount");

    if (!sela)
        return ERROR_INT("sela not defined", procName, 0);

    return sela->n;
}


/*!
 *  selaGetSel()
 *
 *      Input:  sela
 *              index of sel to be retrieved (not copied)
 *      Return: sel, or null on error
 *
 *  Notes:
 *      (1) This returns a ptr to the sel, not a copy, so the caller
 *          must not destroy it!
 */
SEL *
selaGetSel(SELA    *sela,
           l_int32  i)
{
    PROCNAME("selaGetSel");

    if (!sela)
        return (SEL *)ERROR_PTR("sela not defined", procName, NULL);

    if (i < 0 || i >= sela->n)
        return (SEL *)ERROR_PTR("invalid index", procName, NULL);
    return sela->sel[i];
}


/*!
 *  selGetName()
 *
 *      Input:  sel
 *      Return: sel name (not copied), or null if no name or on error
 */
char *
selGetName(SEL  *sel)
{
    PROCNAME("selGetName");

    if (!sel)
        return (char *)ERROR_PTR("sel not defined", procName, NULL);

    return sel->name;
}


/*!
 *  selSetName()
 *
 *      Input:  sel
 *              name (<optional>; can be null)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Always frees the existing sel name, if defined.
 *      (2) If name is not defined, just clears any existing sel name.
 */
l_int32
selSetName(SEL         *sel,
           const char  *name)
{
    PROCNAME("selSetName");

    if (!sel)
        return ERROR_INT("sel not defined", procName, 1);

    return stringReplace(&sel->name, name);
}


/*!
 *  selaFindSelByName()
 *
 *      Input:  sela
 *              sel name
 *              &index (<optional, return>)
 *              &sel  (<optional, return> sel (not a copy))
 *      Return: 0 if OK; 1 on error
 */
l_int32
selaFindSelByName(SELA        *sela,
                  const char  *name,
                  l_int32     *pindex,
                  SEL        **psel)
{
l_int32  i, n;
char    *sname;
SEL     *sel;

    PROCNAME("selaFindSelByName");

    if (pindex) *pindex = -1;
    if (psel) *psel = NULL;

    if (!sela)
        return ERROR_INT("sela not defined", procName, 1);

    n = selaGetCount(sela);
    for (i = 0; i < n; i++)
    {
        if ((sel = selaGetSel(sela, i)) == NULL) {
            L_WARNING("missing sel\n", procName);
            continue;
        }

        sname = selGetName(sel);
        if (sname && (!strcmp(name, sname))) {
            if (pindex)
                *pindex = i;
            if (psel)
                *psel = sel;
            return 0;
        }
    }

    return 1;
}


/*!
 *  selGetElement()
 *
 *      Input:  sel
 *              row
 *              col
 *              &type  (<return> SEL_HIT, SEL_MISS, SEL_DONT_CARE)
 *      Return: 0 if OK; 1 on error
 */
l_int32
selGetElement(SEL      *sel,
              l_int32   row,
              l_int32   col,
              l_int32  *ptype)
{
    PROCNAME("selGetElement");

    if (!ptype)
        return ERROR_INT("&type not defined", procName, 1);
    *ptype = SEL_DONT_CARE;
    if (!sel)
        return ERROR_INT("sel not defined", procName, 1);
    if (row < 0 || row >= sel->sy)
        return ERROR_INT("sel row out of bounds", procName, 1);
    if (col < 0 || col >= sel->sx)
        return ERROR_INT("sel col out of bounds", procName, 1);

    *ptype = sel->data[row][col];
    return 0;
}


/*!
 *  selSetElement()
 *
 *      Input:  sel
 *              row
 *              col
 *              type  (SEL_HIT, SEL_MISS, SEL_DONT_CARE)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Because we use row and column to index into an array,
 *          they are always non-negative.  The location of the origin
 *          (and the type of operation) determine the actual
 *          direction of the rasterop.
 */
l_int32
selSetElement(SEL     *sel,
              l_int32  row,
              l_int32  col,
              l_int32  type)
{
    PROCNAME("selSetElement");

    if (!sel)
        return ERROR_INT("sel not defined", procName, 1);
    if (type != SEL_HIT && type != SEL_MISS && type != SEL_DONT_CARE)
        return ERROR_INT("invalid sel element type", procName, 1);
    if (row < 0 || row >= sel->sy)
        return ERROR_INT("sel row out of bounds", procName, 1);
    if (col < 0 || col >= sel->sx)
        return ERROR_INT("sel col out of bounds", procName, 1);

    sel->data[row][col] = type;
    return 0;
}


/*!
 *  selGetParameters()
 *
 *      Input:  sel
 *              &sy, &sx, &cy, &cx (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
selGetParameters(SEL      *sel,
                 l_int32  *psy,
                 l_int32  *psx,
                 l_int32  *pcy,
                 l_int32  *pcx)
{
    PROCNAME("selGetParameters");

    if (psy) *psy = 0;
    if (psx) *psx = 0;
    if (pcy) *pcy = 0;
    if (pcx) *pcx = 0;
    if (!sel)
        return ERROR_INT("sel not defined", procName, 1);
    if (psy) *psy = sel->sy;
    if (psx) *psx = sel->sx;
    if (pcy) *pcy = sel->cy;
    if (pcx) *pcx = sel->cx;
    return 0;
}


/*!
 *  selSetOrigin()
 *
 *      Input:  sel
 *              cy, cx
 *      Return: 0 if OK; 1 on error
 */
l_int32
selSetOrigin(SEL     *sel,
             l_int32  cy,
             l_int32  cx)
{
    PROCNAME("selSetOrigin");

    if (!sel)
        return ERROR_INT("sel not defined", procName, 1);
    sel->cy = cy;
    sel->cx = cx;
    return 0;
}


/*!
 *  selGetTypeAtOrigin()
 *
 *      Input:  sel
 *              &type  (<return> SEL_HIT, SEL_MISS, SEL_DONT_CARE)
 *      Return: 0 if OK; 1 on error or if origin is not found
 */
l_int32
selGetTypeAtOrigin(SEL      *sel,
                   l_int32  *ptype)
{
l_int32  sx, sy, cx, cy, i, j;

    PROCNAME("selGetTypeAtOrigin");

    if (!ptype)
        return ERROR_INT("&type not defined", procName, 1);
    *ptype = SEL_DONT_CARE;  /* init */
    if (!sel)
        return ERROR_INT("sel not defined", procName, 1);

    selGetParameters(sel, &sy, &sx, &cy, &cx);
    for (i = 0; i < sy; i++) {
        for (j = 0; j < sx; j++) {
            if (i == cy && j == cx) {
                selGetElement(sel, i, j, ptype);
                return 0;
            }
        }
    }

    return ERROR_INT("sel origin not found", procName, 1);
}


/*!
 *  selaGetBrickName()
 *
 *      Input:  sela
 *              hsize, vsize (of brick sel)
 *      Return: sel name (new string), or null if no name or on error
 */
char *
selaGetBrickName(SELA    *sela,
                 l_int32  hsize,
                 l_int32  vsize)
{
l_int32  i, nsels, sx, sy;
SEL     *sel;

    PROCNAME("selaGetBrickName");

    if (!sela)
        return (char *)ERROR_PTR("sela not defined", procName, NULL);

    nsels = selaGetCount(sela);
    for (i = 0; i < nsels; i++) {
        sel = selaGetSel(sela, i);
        selGetParameters(sel, &sy, &sx, NULL, NULL);
        if (hsize == sx && vsize == sy)
            return stringNew(selGetName(sel));
    }

    return (char *)ERROR_PTR("sel not found", procName, NULL);
}


/*!
 *  selaGetCombName()
 *
 *      Input:  sela
 *              size (the product of sizes of the brick and comb parts)
 *              direction (L_HORIZ, L_VERT)
 *      Return: sel name (new string), or null if name not found or on error
 *
 *  Notes:
 *      (1) Combs are by definition 1-dimensional, either horiz or vert.
 *      (2) Use this with comb Sels; e.g., from selaAddDwaCombs().
 */
char *
selaGetCombName(SELA    *sela,
                l_int32  size,
		l_int32  direction)
{
char    *selname;
char     combname[L_BUF_SIZE];
l_int32  i, nsels, sx, sy, found;
SEL     *sel;

    PROCNAME("selaGetCombName");

    if (!sela)
        return (char *)ERROR_PTR("sela not defined", procName, NULL);
    if (direction != L_HORIZ && direction != L_VERT)
        return (char *)ERROR_PTR("invalid direction", procName, NULL);

        /* Derive the comb name we're looking for */
    if (direction == L_HORIZ)
        snprintf(combname, L_BUF_SIZE, "sel_comb_%dh", size);
    else  /* direction == L_VERT */
        snprintf(combname, L_BUF_SIZE, "sel_comb_%dv", size);

    found = FALSE;
    nsels = selaGetCount(sela);
    for (i = 0; i < nsels; i++) {
        sel = selaGetSel(sela, i);
        selGetParameters(sel, &sy, &sx, NULL, NULL);
	if (sy != 1 && sx != 1)  /* 2-D; not a comb */
            continue;
	selname = selGetName(sel);
        if (!strcmp(selname, combname)) {
            found = TRUE;
	    break;
	}
    }

    if (found)
        return stringNew(selname);
    else
        return (char *)ERROR_PTR("sel not found", procName, NULL);
}


/* --------- Function used to generate code in this file  ---------- */
#if 0
static void selaComputeCompositeParameters(const char *fileout);

/*!
 *  selaComputeCompParameters()
 *
 *      Input:  output filename
 *      Return: void
 *
 *  Notes:
 *      (1) This static function was used to construct the comp_parameter_map[]
 *          array at the top of this file.  It is static because it does
 *          not need to be called again.  It remains here to show how
 *          the composite parameter map was computed.
 *      (2) The output file was pasted directly into comp_parameter_map[].
 *          The composite parameter map is used to quickly determine
 *          the linear decomposition parameters and sel names.
 */
static void
selaComputeCompositeParameters(const char  *fileout)
{
char    *str, *nameh1, *nameh2, *namev1, *namev2;
char     buf[L_BUF_SIZE];
l_int32  size, size1, size2, len;
SARRAY  *sa;
SELA    *selabasic, *selacomb;

    selabasic = selaAddBasic(NULL);
    selacomb = selaAddDwaCombs(NULL);
    sa = sarrayCreate(64);
    for (size = 2; size < 64; size++) {
        selectComposableSizes(size, &size1, &size2);
        nameh1 = selaGetBrickName(selabasic, size1, 1);
        namev1 = selaGetBrickName(selabasic, 1, size1);
        if (size2 > 1) {
            nameh2 = selaGetCombName(selacomb, size1 * size2, L_HORIZ);
            namev2 = selaGetCombName(selacomb, size1 * size2, L_VERT);
        } else {
            nameh2 = stringNew("");
            namev2 = stringNew("");
        }
        snprintf(buf, L_BUF_SIZE,
                 "      { %d, %d, %d, \"%s\", \"%s\", \"%s\", \"%s\" },",
                 size, size1, size2, nameh1, nameh2, namev1, namev2);
	sarrayAddString(sa, buf, L_COPY);
        FREE(nameh1);
        FREE(nameh2);
        FREE(namev1);
        FREE(namev2);
    }
    str = sarrayToString(sa, 1);
    len = strlen(str);
    l_binaryWrite(fileout, "w", str, len + 1);
    FREE(str);
    sarrayDestroy(&sa);
    selaDestroy(&selabasic);
    selaDestroy(&selacomb);
    return;
}
#endif
/* -------------------------------------------------------------------- */


/*!
 *  getCompositeParameters()
 *
 *      Input:  size
 *              &size1 (<optional return> brick factor size)
 *              &size2 (<optional return> comb factor size)
 *              &nameh1 (<optional return> name of horiz brick)
 *              &nameh2 (<optional return> name of horiz comb)
 *              &namev1 (<optional return> name of vert brick)
 *              &namev2 (<optional return> name of vert comb)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This uses the big lookup table at the top of this file.
 *      (2) All returned strings are copies that must be freed.
 */
l_int32
getCompositeParameters(l_int32   size,
                       l_int32  *psize1,
                       l_int32  *psize2,
                       char    **pnameh1,
                       char    **pnameh2,
                       char    **pnamev1,
                       char    **pnamev2)
{
l_int32  index;

    PROCNAME("selaGetSelnames");

    if (psize1) *psize1 = 0;
    if (psize2) *psize2 = 0;
    if (pnameh1) *pnameh1 = NULL;
    if (pnameh2) *pnameh2 = NULL;
    if (pnamev1) *pnamev1 = NULL;
    if (pnamev2) *pnamev2 = NULL;
    if (size < 2 || size > 63)
        return ERROR_INT("valid size range is {2 ... 63}", procName, 1);
    index = size - 2;
    if (psize1)
        *psize1 = comp_parameter_map[index].size1;
    if (psize2)
        *psize2 = comp_parameter_map[index].size2;
    if (pnameh1)
        *pnameh1 = stringNew(comp_parameter_map[index].selnameh1);
    if (pnameh2)
        *pnameh2 = stringNew(comp_parameter_map[index].selnameh2);
    if (pnamev1)
        *pnamev1 = stringNew(comp_parameter_map[index].selnamev1);
    if (pnamev2)
        *pnamev2 = stringNew(comp_parameter_map[index].selnamev2);
    return 0;
}


/*!
 *  selaGetSelnames()
 *
 *      Input:  sela
 *      Return: sa (of all sel names), or null on error
 */
SARRAY *
selaGetSelnames(SELA  *sela)
{
char    *selname;
l_int32  i, n;
SEL     *sel;
SARRAY  *sa;

    PROCNAME("selaGetSelnames");

    if (!sela)
        return (SARRAY *)ERROR_PTR("sela not defined", procName, NULL);
    if ((n = selaGetCount(sela)) == 0)
        return (SARRAY *)ERROR_PTR("no sels in sela", procName, NULL);

    if ((sa = sarrayCreate(n)) == NULL)
        return (SARRAY *)ERROR_PTR("sa not made", procName, NULL);
    for (i = 0; i < n; i++) {
        sel = selaGetSel(sela, i);
        selname = selGetName(sel);
        sarrayAddString(sa, selname, 1);
    }

    return sa;
}



/*----------------------------------------------------------------------*
 *                Max translations for erosion and hmt                  *
 *----------------------------------------------------------------------*/
/*!
 *  selFindMaxTranslations()
 *
 *      Input:  sel
 *              &xp, &yp, &xn, &yn  (<return> max shifts)
 *      Return: 0 if OK; 1 on error
 *
 *  Note: these are the maximum shifts for the erosion operation.
 *        For example, when j < cx, the shift of the image
 *        is +x to the cx.  This is a positive xp shift.
 */
l_int32
selFindMaxTranslations(SEL      *sel,
                       l_int32  *pxp,
                       l_int32  *pyp,
                       l_int32  *pxn,
                       l_int32  *pyn)
{
l_int32  sx, sy, cx, cy, i, j;
l_int32  maxxp, maxyp, maxxn, maxyn;

    PROCNAME("selaFindMaxTranslations");

    if (!pxp || !pyp || !pxn || !pyn)
        return ERROR_INT("&xp (etc) defined", procName, 1);
    *pxp = *pyp = *pxn = *pyn = 0;
    if (!sel)
        return ERROR_INT("sel not defined", procName, 1);
    selGetParameters(sel, &sy, &sx, &cy, &cx);

    maxxp = maxyp = maxxn = maxyn = 0;
    for (i = 0; i < sy; i++) {
        for (j = 0; j < sx; j++) {
            if (sel->data[i][j] == 1) {
                maxxp = L_MAX(maxxp, cx - j);
                maxyp = L_MAX(maxyp, cy - i);
                maxxn = L_MAX(maxxn, j - cx);
                maxyn = L_MAX(maxyn, i - cy);
            }
        }
    }

    *pxp = maxxp;
    *pyp = maxyp;
    *pxn = maxxn;
    *pyn = maxyn;

    return 0;
}


/*----------------------------------------------------------------------*
 *                   Rotation by multiples of 90 degrees                *
 *----------------------------------------------------------------------*/
/*!
 *  selRotateOrth()
 *
 *      Input:  sel
 *              quads (0 - 4; number of 90 degree cw rotations)
 *      Return: seld, or null on error
 */
SEL  *
selRotateOrth(SEL     *sel,
              l_int32  quads)
{
l_int32  i, j, ni, nj, sx, sy, cx, cy, nsx, nsy, ncx, ncy, type;
SEL     *seld;

    PROCNAME("selRotateOrth");

    if (!sel)
        return (SEL *)ERROR_PTR("sel not defined", procName, NULL);
    if (quads < 0 || quads > 4)
        return (SEL *)ERROR_PTR("quads not in {0,1,2,3,4}", procName, NULL);
    if (quads == 0 || quads == 4)
        return selCopy(sel);

    selGetParameters(sel, &sy, &sx, &cy, &cx);
    if (quads == 1) {  /* 90 degrees cw */
        nsx = sy;
        nsy = sx;
        ncx = sy - cy - 1;
        ncy = cx;
    } else if (quads == 2) {  /* 180 degrees cw */
        nsx = sx;
        nsy = sy;
        ncx = sx - cx - 1;
        ncy = sy - cy - 1;
    } else {  /* 270 degrees cw */
        nsx = sy;
        nsy = sx;
        ncx = cy;
        ncy = sx - cx - 1;
    }
    seld = selCreateBrick(nsy, nsx, ncy, ncx, SEL_DONT_CARE);
    if (sel->name)
        seld->name = stringNew(sel->name);

    for (i = 0; i < sy; i++) {
        for (j = 0; j < sx; j++) {
            selGetElement(sel, i, j, &type);
            if (quads == 1) {
               ni = j;
               nj = sy - i - 1;
            } else if (quads == 2) {
               ni = sy - i - 1;
               nj = sx - j - 1;
            } else {  /* quads == 3 */
               ni = sx - j - 1;
               nj = i;
            }
            selSetElement(seld, ni, nj, type);
        }
    }

    return seld;
}


/*----------------------------------------------------------------------*
 *                       Sela and Sel serialized I/O                    *
 *----------------------------------------------------------------------*/
/*!
 *  selaRead()
 *
 *      Input:  filename
 *      Return: sela, or null on error
 */
SELA  *
selaRead(const char  *fname)
{
FILE  *fp;
SELA  *sela;

    PROCNAME("selaRead");

    if (!fname)
        return (SELA *)ERROR_PTR("fname not defined", procName, NULL);

    if ((fp = fopenReadStream(fname)) == NULL)
        return (SELA *)ERROR_PTR("stream not opened", procName, NULL);
    if ((sela = selaReadStream(fp)) == NULL)
        return (SELA *)ERROR_PTR("sela not returned", procName, NULL);
    fclose(fp);

    return sela;
}


/*!
 *  selaReadStream()
 *
 *      Input:  stream
 *      Return: sela, or null on error
 */
SELA  *
selaReadStream(FILE  *fp)
{
l_int32  i, n, version;
SEL     *sel;
SELA    *sela;

    PROCNAME("selaReadStream");

    if (!fp)
        return (SELA *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nSela Version %d\n", &version) != 1)
        return (SELA *)ERROR_PTR("not a sela file", procName, NULL);
    if (version != SEL_VERSION_NUMBER)
        return (SELA *)ERROR_PTR("invalid sel version", procName, NULL);
    if (fscanf(fp, "Number of Sels = %d\n\n", &n) != 1)
        return (SELA *)ERROR_PTR("not a sela file", procName, NULL);

    if ((sela = selaCreate(n)) == NULL)
        return (SELA *)ERROR_PTR("sela not made", procName, NULL);
    sela->nalloc = n;

    for (i = 0; i < n; i++)
    {
        if ((sel = selReadStream(fp)) == NULL)
            return (SELA *)ERROR_PTR("sel not made", procName, NULL);
        selaAddSel(sela, sel, NULL, 0);
    }

    return sela;
}


/*!
 *  selRead()
 *
 *      Input:  filename
 *      Return: sel, or null on error
 */
SEL  *
selRead(const char  *fname)
{
FILE  *fp;
SEL   *sel;

    PROCNAME("selRead");

    if (!fname)
        return (SEL *)ERROR_PTR("fname not defined", procName, NULL);

    if ((fp = fopenReadStream(fname)) == NULL)
        return (SEL *)ERROR_PTR("stream not opened", procName, NULL);
    if ((sel = selReadStream(fp)) == NULL)
        return (SEL *)ERROR_PTR("sela not returned", procName, NULL);
    fclose(fp);

    return sel;
}


/*!
 *  selReadStream()
 *
 *      Input:  stream
 *      Return: sel, or null on error
 */
SEL  *
selReadStream(FILE  *fp)
{
char    *selname;
char     linebuf[L_BUF_SIZE];
l_int32  sy, sx, cy, cx, i, j, version, ignore;
SEL     *sel;

    PROCNAME("selReadStream");

    if (!fp)
        return (SEL *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "  Sel Version %d\n", &version) != 1)
        return (SEL *)ERROR_PTR("not a sel file", procName, NULL);
    if (version != SEL_VERSION_NUMBER)
        return (SEL *)ERROR_PTR("invalid sel version", procName, NULL);

    if (fgets(linebuf, L_BUF_SIZE, fp) == NULL)
        return (SEL *)ERROR_PTR("error reading into linebuf", procName, NULL);
    selname = stringNew(linebuf);
    sscanf(linebuf, "  ------  %s  ------", selname);

    if (fscanf(fp, "  sy = %d, sx = %d, cy = %d, cx = %d\n",
            &sy, &sx, &cy, &cx) != 4)
        return (SEL *)ERROR_PTR("dimensions not read", procName, NULL);

    if ((sel = selCreate(sy, sx, selname)) == NULL)
        return (SEL *)ERROR_PTR("sel not made", procName, NULL);
    selSetOrigin(sel, cy, cx);

    for (i = 0; i < sy; i++) {
        ignore = fscanf(fp, "    ");
        for (j = 0; j < sx; j++)
            ignore = fscanf(fp, "%1d", &sel->data[i][j]);
        ignore = fscanf(fp, "\n");
    }
    ignore = fscanf(fp, "\n");

    FREE(selname);
    return sel;
}


/*!
 *  selaWrite()
 *
 *      Input:  filename
 *              sela
 *      Return: 0 if OK, 1 on error
 */
l_int32
selaWrite(const char  *fname,
          SELA        *sela)
{
FILE  *fp;

    PROCNAME("selaWrite");

    if (!fname)
        return ERROR_INT("fname not defined", procName, 1);
    if (!sela)
        return ERROR_INT("sela not defined", procName, 1);

    if ((fp = fopenWriteStream(fname, "wb")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    selaWriteStream(fp, sela);
    fclose(fp);

    return 0;
}


/*!
 *  selaWriteStream()
 *
 *      Input:  stream
 *              sela
 *      Return: 0 if OK, 1 on error
 */
l_int32
selaWriteStream(FILE  *fp,
                SELA  *sela)
{
l_int32  i, n;
SEL     *sel;

    PROCNAME("selaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!sela)
        return ERROR_INT("sela not defined", procName, 1);

    n = selaGetCount(sela);
    fprintf(fp, "\nSela Version %d\n", SEL_VERSION_NUMBER);
    fprintf(fp, "Number of Sels = %d\n\n", n);
    for (i = 0; i < n; i++) {
        if ((sel = selaGetSel(sela, i)) == NULL)
            continue;
        selWriteStream(fp, sel);
    }
    return 0;
}


/*!
 *  selWrite()
 *
 *      Input:  filename
 *              sel
 *      Return: 0 if OK, 1 on error
 */
l_int32
selWrite(const char  *fname,
         SEL         *sel)
{
FILE  *fp;

    PROCNAME("selWrite");

    if (!fname)
        return ERROR_INT("fname not defined", procName, 1);
    if (!sel)
        return ERROR_INT("sel not defined", procName, 1);

    if ((fp = fopenWriteStream(fname, "wb")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    selWriteStream(fp, sel);
    fclose(fp);

    return 0;
}


/*!
 *  selWriteStream()
 *
 *      Input:  stream
 *              sel
 *      Return: 0 if OK, 1 on error
 */
l_int32
selWriteStream(FILE  *fp,
               SEL   *sel)
{
l_int32  sx, sy, cx, cy, i, j;

    PROCNAME("selWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!sel)
        return ERROR_INT("sel not defined", procName, 1);
    selGetParameters(sel, &sy, &sx, &cy, &cx);

    fprintf(fp, "  Sel Version %d\n", SEL_VERSION_NUMBER);
    fprintf(fp, "  ------  %s  ------\n", selGetName(sel));
    fprintf(fp, "  sy = %d, sx = %d, cy = %d, cx = %d\n", sy, sx, cy, cx);
    for (i = 0; i < sy; i++) {
        fprintf(fp, "    ");
        for (j = 0; j < sx; j++)
            fprintf(fp, "%d", sel->data[i][j]);
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");

    return 0;
}


/*----------------------------------------------------------------------*
 *           Building custom hit-miss sels from compiled strings        *
 *----------------------------------------------------------------------*/
/*!
 *  selCreateFromString()
 *
 *      Input:  text
 *              height, width
 *              name (<optional> sel name; can be null)
 *      Return: sel of the given size, or null on error
 *
 *  Notes:
 *      (1) The text is an array of chars (in row-major order) where
 *          each char can be one of the following:
 *             'x': hit
 *             'o': miss
 *             ' ': don't-care
 *      (2) Use an upper case char to indicate the origin of the Sel.
 *          When the origin falls on a don't-care, use 'C' as the uppecase
 *          for ' '.
 *      (3) The text can be input in a format that shows the 2D layout; e.g.,
 *              static const char *seltext = "x    "
 *                                           "x Oo "
 *                                           "x    "
 *                                           "xxxxx";
 */
SEL *
selCreateFromString(const char  *text,
                    l_int32      h,
                    l_int32      w,
                    const char  *name)
{
SEL     *sel;
l_int32  y, x;
char     ch;

    PROCNAME("selCreateFromString");

    if (h < 1)
        return (SEL *)ERROR_PTR("height must be > 0", procName, NULL);
    if (w < 1)
        return (SEL *)ERROR_PTR("width must be > 0", procName, NULL);

    sel = selCreate(h, w, name);

    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            ch = *(text++);
            switch (ch)
            {
                case 'X':
                    selSetOrigin(sel, y, x);
                case 'x':
                    selSetElement(sel, y, x, SEL_HIT);
                    break;

                case 'O':
                    selSetOrigin(sel, y, x);
                case 'o':
                    selSetElement(sel, y, x, SEL_MISS);
                    break;

                case 'C':
                    selSetOrigin(sel, y, x);
                case ' ':
                    selSetElement(sel, y, x, SEL_DONT_CARE);
                    break;

                case '\n':
                    /* ignored */
                    continue;

                default:
                    selDestroy(&sel);
                    return (SEL *)ERROR_PTR("unknown char", procName, NULL);
            }
        }
    }

    return sel;
}


/*!
 *  selPrintToString()
 *
 *      Input:  sel
 *      Return: str (string; caller must free)
 *
 *  Notes:
 *      (1) This is an inverse function of selCreateFromString.
 *          It prints a textual representation of the SEL to a malloc'd
 *          string.  The format is the same as selCreateFromString
 *          except that newlines are inserted into the output
 *          between rows.
 *      (2) This is useful for debugging.  However, if you want to
 *          save some Sels in a file, put them in a Sela and write
 *          them out with selaWrite().  They can then be read in
 *          with selaRead().
 */
char *
selPrintToString(SEL  *sel)
{
char     is_center;
char    *str, *strptr;
l_int32  type;
l_int32  sx, sy, cx, cy, x, y;

    PROCNAME("selPrintToString");

    if (!sel)
        return (char *)ERROR_PTR("sel not defined", procName, NULL);

    selGetParameters(sel, &sy, &sx, &cy, &cx);
    if ((str = (char *)CALLOC(1, sy * (sx + 1) + 1)) == NULL)
        return (char *)ERROR_PTR("calloc fail for str", procName, NULL);
    strptr = str;

    for (y = 0; y < sy; ++y) {
        for (x = 0; x < sx; ++x) {
            selGetElement(sel, y, x, &type);
            is_center = (x == cx && y == cy);
            switch (type) {
                case SEL_HIT:
                    *(strptr++) = is_center ? 'X' : 'x';
                    break;
                case SEL_MISS:
                    *(strptr++) = is_center ? 'O' : 'o';
                    break;
                case SEL_DONT_CARE:
                    *(strptr++) = is_center ? 'C' : ' ';
                    break;
            }
        }
        *(strptr++) = '\n';
    }

    return str;
}


/*----------------------------------------------------------------------*
 *         Building custom hit-miss sels from a simple file format      *
 *----------------------------------------------------------------------*/
/*!
 *  selaCreateFromFile()
 *
 *      Input:  filename
 *      Return: sela, or null on error
 *
 *  Notes:
 *      (1) The file contains a sequence of Sel descriptions.
 *      (2) Each Sel is formatted as follows:
 *           - Any number of comment lines starting with '#' are ignored
 *           - The next line contains the selname
 *           - The next lines contain the Sel data.  They must be
 *             formatted similarly to the string format in
 *             selCreateFromString(), with each line beginning and
 *             ending with a double-quote, and showing the 2D layout.
 *           - Each Sel ends when a blank line, a comment line, or
 *             the end of file is reached.
 *      (3) See selCreateFromString() for a description of the string
 *          format for the Sel data.  As an example, here are the lines
 *          of is a valid file for a single Sel.  In the file, all lines
 *          are left-justified:
 *                    # diagonal sel
 *                    sel_5diag
 *                    "x    "
 *                    " x   "
 *                    "  X  "
 *                    "   x "
 *                    "    x"
 */
SELA *
selaCreateFromFile(const char  *filename)
{
char    *filestr, *line;
l_int32  i, n, first, last, nsel, insel;
size_t   nbytes;
NUMA    *nafirst, *nalast;
SARRAY  *sa;
SEL     *sel;
SELA    *sela;

    PROCNAME("selaCreateFromFile");

    if (!filename)
        return (SELA *)ERROR_PTR("filename not defined", procName, NULL);

    filestr = (char *)l_binaryRead(filename, &nbytes);
    sa = sarrayCreateLinesFromString(filestr, 1);
    FREE(filestr);
    n = sarrayGetCount(sa);
    sela = selaCreate(0);

        /* Find the start and end lines for each Sel.
         * We allow the "blank" lines to be null strings or
         * to have standard whitespace (' ','\t',\'n') or be '#'. */
    nafirst = numaCreate(0);
    nalast = numaCreate(0);
    insel = FALSE;
    for (i = 0; i < n; i++) {
        line = sarrayGetString(sa, i, L_NOCOPY);
        if (!insel &&
            (line[0] != '\0' && line[0] != ' ' &&
             line[0] != '\t' && line[0] != '\n' && line[0] != '#')) {
            numaAddNumber(nafirst, i);
            insel = TRUE;
	    continue;
        }
	if (insel &&
            (line[0] == '\0' || line[0] == ' ' ||
             line[0] == '\t' || line[0] == '\n' || line[0] == '#')) {
            numaAddNumber(nalast, i - 1);
            insel = FALSE;
            continue;
        }
    }
    if (insel)  /* fell off the end of the file */
        numaAddNumber(nalast, n - 1);

        /* Extract sels */
    nsel = numaGetCount(nafirst);
    for (i = 0; i < nsel; i++) {
        numaGetIValue(nafirst, i, &first);
        numaGetIValue(nalast, i, &last);
        if ((sel = selCreateFromSArray(sa, first, last)) == NULL) {
            fprintf(stderr, "Error reading sel from %d to %d\n", first, last);
            selaDestroy(&sela);
            sarrayDestroy(&sa);
            numaDestroy(&nafirst);
            numaDestroy(&nalast);
            return (SELA *)ERROR_PTR("bad sela file", procName, NULL);
        }
	selaAddSel(sela, sel, NULL, 0);
    }

    numaDestroy(&nafirst);
    numaDestroy(&nalast);
    sarrayDestroy(&sa);
    return sela;
}


/*!
 *  selCreateFromSArray()
 *
 *      Input:  sa
 *              first (line of sarray where Sel begins)
 *              last (line of sarray where Sel ends)
 *      Return: sela, or null on error
 *
 *  Notes:
 *      (1) The Sel contains the following lines:
 *          - The first line is the selname
 *          - The remaining lines contain the Sel data.  They must
 *            be formatted similarly to the string format in
 *            selCreateFromString(), with each line beginning and
 *            ending with a double-quote, and showing the 2D layout.
 *          - 'last' gives the last line in the Sel data.
 *      (2) See selCreateFromString() for a description of the string
 *          format for the Sel data.  As an example, here are the lines
 *          of is a valid file for a single Sel.  In the file, all lines
 *          are left-justified:
 *                    # diagonal sel
 *                    sel_5diag
 *                    "x    "
 *                    " x   "
 *                    "  X  "
 *                    "   x "
 *                    "    x"
 */
static SEL *
selCreateFromSArray(SARRAY  *sa,
		    l_int32  first,
		    l_int32  last)
{
char     ch;
char    *name, *line;
l_int32  n, len, i, w, h, y, x;
SEL     *sel;

    PROCNAME("selCreateFromSArray");

    if (!sa)
        return (SEL *)ERROR_PTR("sa not defined", procName, NULL);
    n = sarrayGetCount(sa);
    if (first < 0 || first >= n || last <= first || last >= n)
        return (SEL *)ERROR_PTR("invalid range", procName, NULL);

    name = sarrayGetString(sa, first, L_NOCOPY);
    h = last - first;
    line = sarrayGetString(sa, first + 1, L_NOCOPY);
    len = strlen(line);
    if (line[0] != '"' || line[len - 1] != '"')
        return (SEL *)ERROR_PTR("invalid format", procName, NULL);
    w = len - 2;
    if ((sel = selCreate(h, w, name)) == NULL)
        return (SEL *)ERROR_PTR("sel not made", procName, NULL);
    for (i = first + 1; i <= last; i++) {
        line = sarrayGetString(sa, i, L_NOCOPY);
        y = i - first - 1;
        for (x = 0; x < w; ++x) {
            ch = line[x + 1];  /* skip the leading double-quote */
            switch (ch)
            {
                case 'X':
                    selSetOrigin(sel, y, x);
                case 'x':
                    selSetElement(sel, y, x, SEL_HIT);
                    break;

                case 'O':
                    selSetOrigin(sel, y, x);
                case 'o':
                    selSetElement(sel, y, x, SEL_MISS);
                    break;

                case 'C':
                    selSetOrigin(sel, y, x);
                case ' ':
                    selSetElement(sel, y, x, SEL_DONT_CARE);
                    break;
                default:
                    selDestroy(&sel);
                    return (SEL *)ERROR_PTR("unknown char", procName, NULL);
            }
        }
    }

    return sel;
}


/*----------------------------------------------------------------------*
 *               Making hit-only SELs from Pta and Pix                  *
 *----------------------------------------------------------------------*/
/*!
 *  selCreateFromPta()
 *
 *      Input:  pta
 *              cy, cx (origin of sel)
 *              name (<optional> sel name; can be null)
 *      Return: sel (of minimum required size), or null on error
 *
 *  Notes:
 *      (1) The origin and all points in the pta must be positive.
 */
SEL *
selCreateFromPta(PTA         *pta,
                 l_int32      cy,
                 l_int32      cx,
                 const char  *name)
{
l_int32  i, n, x, y, w, h;
BOX     *box;
SEL     *sel;

    PROCNAME("selCreateFromPta");

    if (!pta)
        return (SEL *)ERROR_PTR("pta not defined", procName, NULL);
    if (cy < 0 || cx < 0)
        return (SEL *)ERROR_PTR("(cy, cx) not both >= 0", procName, NULL);
    n = ptaGetCount(pta);
    if (n == 0)
        return (SEL *)ERROR_PTR("no pts in pta", procName, NULL);

    box = ptaGetBoundingRegion(pta);
    boxGetGeometry(box, &x, &y, &w, &h);
    boxDestroy(&box);
    if (x < 0 || y < 0)
        return (SEL *)ERROR_PTR("not all x and y >= 0", procName, NULL);

    sel = selCreate(y + h, x + w, name);
    selSetOrigin(sel, cy, cx);
    for (i = 0; i < n; i++) {
        ptaGetIPt(pta, i, &x, &y);
	selSetElement(sel, y, x, SEL_HIT);
    }

    return sel;
}


/*!
 *  selCreateFromPix()
 *
 *      Input:  pix
 *              cy, cx (origin of sel)
 *              name (<optional> sel name; can be null)
 *      Return: sel, or null on error
 *
 *  Notes:
 *      (1) The origin must be positive.
 */
SEL *
selCreateFromPix(PIX         *pix,
                 l_int32      cy,
                 l_int32      cx,
                 const char  *name)
{
SEL      *sel;
l_int32   i, j, w, h, d;
l_uint32  val;

    PROCNAME("selCreateFromPix");

    if (!pix)
        return (SEL *)ERROR_PTR("pix not defined", procName, NULL);
    if (cy < 0 || cx < 0)
        return (SEL *)ERROR_PTR("(cy, cx) not both >= 0", procName, NULL);
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 1)
        return (SEL *)ERROR_PTR("pix not 1 bpp", procName, NULL);

    sel = selCreate(h, w, name);
    selSetOrigin(sel, cy, cx);
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            pixGetPixel(pix, j, i, &val);
            if (val)
                selSetElement(sel, i, j, SEL_HIT);
        }
    }

    return sel;
}


/*----------------------------------------------------------------------*
 *            Making hit-miss sels from color Pix and image files             *
 *----------------------------------------------------------------------*/
/*!
 *
 *  selReadFromColorImage()
 *
 *      Input:  pathname
 *      Return: sel if OK; null on error
 *
 *  Notes:
 *      (1) Loads an image from a file and creates a (hit-miss) sel.
 *      (2) The sel name is taken from the pathname without the directory
 *          and extension.
 */
SEL *
selReadFromColorImage(const char  *pathname)
{
PIX   *pix;
SEL   *sel;
char  *basename, *selname;

    PROCNAME("selReadFromColorImage");

    splitPathAtExtension (pathname, &basename, NULL);
    splitPathAtDirectory (basename, NULL, &selname);
    FREE(basename);

    if ((pix = pixRead(pathname)) == NULL)
        return (SEL *)ERROR_PTR("pix not returned", procName, NULL);
    if ((sel = selCreateFromColorPix(pix, selname)) == NULL)
        return (SEL *)ERROR_PTR("sel not made", procName, NULL);
    FREE(selname);
    pixDestroy(&pix);

    return sel;
}


/*!
 *
 *  selCreateFromColorPix()
 *
 *      Input:  pixs (cmapped or rgb)
 *              selname (<optional> sel name; can be null)
 *      Return: sel if OK, null on error
 *
 *  Notes:
 *      (1) The sel size is given by the size of pixs.
 *      (2) In pixs, hits are represented by green pixels, misses by red
 *          pixels, and don't-cares by white pixels.
 *      (3) In pixs, there may be no misses, but there must be at least 1 hit.
 *      (4) At most there can be only one origin pixel, which is optionally
 *          specified by using a lower-intensity pixel:
 *            if a hit:  dark green
 *            if a miss: dark red
 *            if a don't care: gray
 *          If there is no such pixel, the origin defaults to the approximate
 *          center of the sel.
 */
SEL *
selCreateFromColorPix(PIX   *pixs,
                      char  *selname)
{
PIXCMAP  *cmap;
SEL      *sel;
l_int32   hascolor, hasorigin, nohits;
l_int32   w, h, d, i, j, red, green, blue;
l_uint32  pixval;

    PROCNAME("selCreateFromColorPix");

    if (!pixs)
        return (SEL *)ERROR_PTR("pixs not defined", procName, NULL);

    hascolor = FALSE;
    cmap = pixGetColormap(pixs);
    if (cmap)
        pixcmapHasColor(cmap, &hascolor);
    pixGetDimensions(pixs, &w, &h, &d);
    if (hascolor == FALSE && d != 32)
        return (SEL *)ERROR_PTR("pixs has no color", procName, NULL);

    if ((sel = selCreate (h, w, NULL)) == NULL)
        return (SEL *)ERROR_PTR ("sel not made", procName, NULL);
    selSetOrigin (sel, h / 2, w / 2);
    selSetName(sel, selname);

    hasorigin = FALSE;
    nohits = TRUE;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            pixGetPixel (pixs, j, i, &pixval);

            if (cmap) {
                pixcmapGetColor (cmap, pixval, &red, &green, &blue);
            } else {
                red = GET_DATA_BYTE (&pixval, COLOR_RED);
                green = GET_DATA_BYTE (&pixval, COLOR_GREEN);
                blue = GET_DATA_BYTE (&pixval, COLOR_BLUE);
            }

            if (red < 255 && green < 255 && blue < 255) {
                if (hasorigin)
                    L_WARNING("multiple origins in sel image\n", procName);
                selSetOrigin (sel, i, j);
                hasorigin = TRUE;
            }
            if (!red && green && !blue) {
                nohits = FALSE;
                selSetElement (sel, i, j, SEL_HIT);
            } else if (red && !green && !blue) {
                selSetElement (sel, i, j, SEL_MISS);
            } else if (red && green && blue) {
                selSetElement (sel, i, j, SEL_DONT_CARE);
            } else {
                selDestroy(&sel);
                return (SEL *)ERROR_PTR("invalid color", procName, NULL);
            }
        }
    }

    if (nohits) {
        selDestroy(&sel);
        return (SEL *)ERROR_PTR("no hits in sel", procName, NULL);
    }
    return sel;
}


/*----------------------------------------------------------------------*
 *                     Printable display of sel                         *
 *----------------------------------------------------------------------*/
/*!
 *  selDisplayInPix()
 *
 *      Input:  sel
 *              size (of grid interiors; odd; minimum size of 13 is enforced)
 *              gthick (grid thickness; minimum size of 2 is enforced)
 *      Return: pix (display of sel), or null on error
 *
 *  Notes:
 *      (1) This gives a visual representation of a general (hit-miss) sel.
 *      (2) The empty sel is represented by a grid of intersecting lines.
 *      (3) Three different patterns are generated for the sel elements:
 *          - hit (solid black circle)
 *          - miss (black ring; inner radius is radius2)
 *          - origin (cross, XORed with whatever is there)
 */
PIX *
selDisplayInPix(SEL     *sel,
                l_int32  size,
                l_int32  gthick)
{
l_int32  i, j, w, h, sx, sy, cx, cy, type, width;
l_int32  radius1, radius2, shift1, shift2, x0, y0;
PIX     *pixd, *pix2, *pixh, *pixm, *pixorig;
PTA     *pta1, *pta2, *pta1t, *pta2t;

    PROCNAME("selDisplayInPix");

    if (!sel)
        return (PIX *)ERROR_PTR("sel not defined", procName, NULL);
    if (size < 13) {
        L_WARNING("size < 13; setting to 13\n", procName);
        size = 13;
    }
    if (size % 2 == 0)
        size++;
    if (gthick < 2) {
        L_WARNING("grid thickness < 2; setting to 2\n", procName);
        gthick = 2;
    }
    selGetParameters(sel, &sy, &sx, &cy, &cx);
    w = size * sx + gthick * (sx + 1);
    h = size * sy + gthick * (sy + 1);
    pixd = pixCreate(w, h, 1);

        /* Generate grid lines */
    for (i = 0; i <= sy; i++)
        pixRenderLine(pixd, 0, gthick / 2 + i * (size + gthick),
                      w - 1, gthick / 2 + i * (size + gthick),
                      gthick, L_SET_PIXELS);
    for (j = 0; j <= sx; j++)
        pixRenderLine(pixd, gthick / 2 + j * (size + gthick), 0,
                      gthick / 2 + j * (size + gthick), h - 1,
                      gthick, L_SET_PIXELS);

        /* Generate hit and miss patterns */
    radius1 = (l_int32)(0.85 * ((size - 1) / 2) + 0.5);  /* of hit */
    radius2 = (l_int32)(0.65 * ((size - 1) / 2) + 0.5);  /* inner miss radius */
    pta1 = generatePtaFilledCircle(radius1);
    pta2 = generatePtaFilledCircle(radius2);
    shift1 = (size - 1) / 2 - radius1;  /* center circle in square */
    shift2 = (size - 1) / 2 - radius2;
    pta1t = ptaTransform(pta1, shift1, shift1, 1.0, 1.0);
    pta2t = ptaTransform(pta2, shift2, shift2, 1.0, 1.0);
    pixh = pixGenerateFromPta(pta1t, size, size);  /* hits */
    pix2 = pixGenerateFromPta(pta2t, size, size);
    pixm = pixSubtract(NULL, pixh, pix2);

        /* Generate crossed lines for origin pattern */
    pixorig = pixCreate(size, size, 1);
    width = size / 8;
    pixRenderLine(pixorig, size / 2, (l_int32)(0.12 * size),
                           size / 2, (l_int32)(0.88 * size),
                           width, L_SET_PIXELS);
    pixRenderLine(pixorig, (l_int32)(0.15 * size), size / 2,
                           (l_int32)(0.85 * size), size / 2,
                           width, L_FLIP_PIXELS);
    pixRasterop(pixorig, size / 2 - width, size / 2 - width,
                2 * width, 2 * width, PIX_NOT(PIX_DST), NULL, 0, 0);

        /* Specialize origin pattern for this sel */
    selGetTypeAtOrigin(sel, &type);
    if (type == SEL_HIT)
        pixXor(pixorig, pixorig, pixh);
    else if (type == SEL_MISS)
        pixXor(pixorig, pixorig, pixm);

        /* Paste the patterns in */
    y0 = gthick;
    for (i = 0; i < sy; i++) {
        x0 = gthick;
        for (j = 0; j < sx; j++) {
            selGetElement(sel, i, j, &type);
	    if (i == cy && j == cx)  /* origin */
                pixRasterop(pixd, x0, y0, size, size, PIX_SRC, pixorig, 0, 0);
	    else if (type == SEL_HIT)
                pixRasterop(pixd, x0, y0, size, size, PIX_SRC, pixh, 0, 0);
	    else if (type == SEL_MISS)
                pixRasterop(pixd, x0, y0, size, size, PIX_SRC, pixm, 0, 0);
            x0 += size + gthick;
        }
        y0 += size + gthick;
    }

    pixDestroy(&pix2);
    pixDestroy(&pixh);
    pixDestroy(&pixm);
    pixDestroy(&pixorig);
    ptaDestroy(&pta1);
    ptaDestroy(&pta1t);
    ptaDestroy(&pta2);
    ptaDestroy(&pta2t);
    return pixd;
}


/*!
 *  selaDisplayInPix()
 *
 *      Input:  sela
 *              size (of grid interiors; odd; minimum size of 13 is enforced)
 *              gthick (grid thickness; minimum size of 2 is enforced)
 *              spacing (between sels, both horizontally and vertically)
 *              ncols (number of sels per "line")
 *      Return: pix (display of all sels in sela), or null on error
 *
 *  Notes:
 *      (1) This gives a visual representation of all the sels in a sela.
 *      (2) See notes in selDisplayInPix() for display params of each sel.
 *      (3) This gives the nicest results when all sels in the sela
 *          are the same size.
 */
PIX *
selaDisplayInPix(SELA    *sela,
                 l_int32  size,
                 l_int32  gthick,
                 l_int32  spacing,
                 l_int32  ncols)
{
l_int32  nsels, i, w, width;
PIX     *pixt, *pixd;
PIXA    *pixa;
SEL     *sel;

    PROCNAME("selaDisplayInPix");

    if (!sela)
        return (PIX *)ERROR_PTR("sela not defined", procName, NULL);
    if (size < 13) {
        L_WARNING("size < 13; setting to 13\n", procName);
        size = 13;
    }
    if (size % 2 == 0)
        size++;
    if (gthick < 2) {
        L_WARNING("grid thickness < 2; setting to 2\n", procName);
        gthick = 2;
    }
    if (spacing < 5) {
        L_WARNING("spacing < 5; setting to 5\n", procName);
        spacing = 5;
    }

        /* Accumulate the pix of each sel */
    nsels = selaGetCount(sela);
    pixa = pixaCreate(nsels);
    for (i = 0; i < nsels; i++) {
        sel = selaGetSel(sela, i);
        pixt = selDisplayInPix(sel, size, gthick);
        pixaAddPix(pixa, pixt, L_INSERT);
    }

        /* Find the tiled output width, using just the first
         * ncols pix in the pixa.   If all pix have the same width,
         * they will align properly in columns. */
    width = 0;
    ncols = L_MIN(nsels, ncols);
    for (i = 0; i < ncols; i++) {
        pixt = pixaGetPix(pixa, i, L_CLONE);
        pixGetDimensions(pixt, &w, NULL, NULL);
        width += w;
        pixDestroy(&pixt);
    }
    width += (ncols + 1) * spacing;  /* add spacing all around as well */

    pixd = pixaDisplayTiledInRows(pixa, 1, width, 1.0, 0, spacing, 0);
    pixaDestroy(&pixa);
    return pixd;
}
