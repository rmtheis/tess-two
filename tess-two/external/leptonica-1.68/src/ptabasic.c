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
 *   ptabasic.c
 *
 *      Pta creation, destruction, copy, clone, empty
 *           PTA      *ptaCreate()
 *           PTA      *ptaCreateFromNuma()
 *           void      ptaDestroy()
 *           PTA      *ptaCopy()
 *           PTA      *ptaClone()
 *           l_int32   ptaEmpty()
 *
 *      Pta array extension
 *           l_int32   ptaAddPt()
 *           l_int32   ptaExtendArrays()
 *
 *      Pta Accessors
 *           l_int32   ptaGetRefcount()
 *           l_int32   ptaChangeRefcount()
 *           l_int32   ptaGetCount()
 *           l_int32   ptaGetPt()
 *           l_int32   ptaGetIPt()
 *           l_int32   ptaSetPt()
 *           l_int32   ptaGetArrays()
 *
 *      Pta serialized for I/O
 *           PTA      *ptaRead()
 *           PTA      *ptaReadStream()
 *           l_int32   ptaWrite()
 *           l_int32   ptaWriteStream()
 *
 *      Ptaa creation, destruction
 *           PTAA     *ptaaCreate()
 *           void      ptaaDestroy()
 *
 *      Ptaa array extension
 *           l_int32   ptaaAddPta()
 *           l_int32   ptaaExtendArray()
 *
 *      Ptaa Accessors
 *           l_int32   ptaaGetCount()
 *           l_int32   ptaaGetPta()
 *           l_int32   ptaaGetPt()
 *
 *      Ptaa serialized for I/O
 *           PTAA     *ptaaRead()
 *           PTAA     *ptaaReadStream()
 *           l_int32   ptaaWrite()
 *           l_int32   ptaaWriteStream()
 */

#include <string.h>
#include "allheaders.h"

static const l_int32  INITIAL_PTR_ARRAYSIZE = 20;   /* n'import quoi */


/*---------------------------------------------------------------------*
 *                Pta creation, destruction, copy, clone               *
 *---------------------------------------------------------------------*/
/*!
 *  ptaCreate()
 *
 *      Input:  n  (initial array sizes)
 *      Return: pta, or null on error.
 */
PTA *
ptaCreate(l_int32  n)
{
PTA  *pta;

    PROCNAME("ptaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((pta = (PTA *)CALLOC(1, sizeof(PTA))) == NULL)
        return (PTA *)ERROR_PTR("pta not made", procName, NULL);
    pta->n = 0;
    pta->nalloc = n;
    ptaChangeRefcount(pta, 1);  /* sets to 1 */

    if ((pta->x = (l_float32 *)CALLOC(n, sizeof(l_float32))) == NULL)
        return (PTA *)ERROR_PTR("x array not made", procName, NULL);
    if ((pta->y = (l_float32 *)CALLOC(n, sizeof(l_float32))) == NULL)
        return (PTA *)ERROR_PTR("y array not made", procName, NULL);

    return pta;
}


/*!
 *  ptaCreateFromNuma()
 *
 *      Input:  nax (<optional> can be null)
 *              nay
 *      Return: pta, or null on error.
 */
PTA *
ptaCreateFromNuma(NUMA  *nax,
                  NUMA  *nay)
{
l_int32    i, n;
l_float32  startx, delx, xval, yval;
PTA       *pta;

    PROCNAME("ptaCreateFromNuma");

    if (!nay)
        return (PTA *)ERROR_PTR("nay not defined", procName, NULL);
    n = numaGetCount(nay);
    if (nax && numaGetCount(nax) != n)
        return (PTA *)ERROR_PTR("nax and nay sizes differ", procName, NULL);

    pta = ptaCreate(n);
    numaGetXParameters(nay, &startx, &delx);
    for (i = 0; i < n; i++) {
        if (nax)
            numaGetFValue(nax, i, &xval);
        else  /* use implicit x values from nay */
            xval = startx + i * delx;
        numaGetFValue(nay, i, &yval);
        ptaAddPt(pta, xval, yval);
    }

    return pta;
}


/*!
 *  ptaDestroy()
 *
 *      Input:  &pta (<to be nulled>)
 *      Return: void
 *
 *  Note:
 *      - Decrements the ref count and, if 0, destroys the pta.
 *      - Always nulls the input ptr.
 */
void
ptaDestroy(PTA  **ppta)
{
PTA  *pta;

    PROCNAME("ptaDestroy");

    if (ppta == NULL) {
        L_WARNING("ptr address is NULL!", procName);
        return;
    }

    if ((pta = *ppta) == NULL)
        return;

    ptaChangeRefcount(pta, -1);
    if (ptaGetRefcount(pta) <= 0) {
        FREE(pta->x);
        FREE(pta->y);
        FREE(pta);
    }

    *ppta = NULL;
    return;
}


/*!
 *  ptaCopy()
 *
 *      Input:  pta
 *      Return: copy of pta, or null on error
 */
PTA *
ptaCopy(PTA  *pta)
{
l_int32    i;
l_float32  x, y;
PTA       *npta;

    PROCNAME("ptaCopy");

    if (!pta)
        return (PTA *)ERROR_PTR("pta not defined", procName, NULL);

    if ((npta = ptaCreate(pta->nalloc)) == NULL)
        return (PTA *)ERROR_PTR("npta not made", procName, NULL);

    for (i = 0; i < pta->n; i++) {
        ptaGetPt(pta, i, &x, &y);
        ptaAddPt(npta, x, y);
    }

    return npta;
}


/*!
 *  ptaClone()
 *
 *      Input:  pta
 *      Return: ptr to same pta, or null on error
 */
PTA *
ptaClone(PTA  *pta)
{
    PROCNAME("ptaClone");

    if (!pta)
        return (PTA *)ERROR_PTR("pta not defined", procName, NULL);

    ptaChangeRefcount(pta, 1);
    return pta;
}


/*!
 *  ptaEmpty()
 *
 *      Input:  pta
 *      Return: 0 if OK, 1 on error
 *
 *  Note: this only resets the "n" field, for reuse
 */
l_int32
ptaEmpty(PTA  *pta)
{
    PROCNAME("ptaEmpty");

    if (!pta)
        return ERROR_INT("ptad not defined", procName, 1);
    pta->n = 0;
    return 0;
}


/*---------------------------------------------------------------------*
 *                         Pta array extension                         *
 *---------------------------------------------------------------------*/
/*!
 *  ptaAddPt()
 *
 *      Input:  pta
 *              x, y
 *      Return: 0 if OK, 1 on error
 */
l_int32
ptaAddPt(PTA       *pta,
         l_float32  x,
         l_float32  y)
{
l_int32  n;

    PROCNAME("ptaAddPt");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

    n = pta->n;
    if (n >= pta->nalloc)
        ptaExtendArrays(pta);
    pta->x[n] = x;
    pta->y[n] = y;
    pta->n++;

    return 0;
}


/*!
 *  ptaExtendArrays()
 *
 *      Input:  pta
 *      Return: 0 if OK; 1 on error
 */
l_int32
ptaExtendArrays(PTA  *pta)
{
    PROCNAME("ptaExtendArrays");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

    if ((pta->x = (l_float32 *)reallocNew((void **)&pta->x,
                               sizeof(l_float32) * pta->nalloc,
                               2 * sizeof(l_float32) * pta->nalloc)) == NULL)
        return ERROR_INT("new x array not returned", procName, 1);
    if ((pta->y = (l_float32 *)reallocNew((void **)&pta->y,
                               sizeof(l_float32) * pta->nalloc,
                               2 * sizeof(l_float32) * pta->nalloc)) == NULL)
        return ERROR_INT("new y array not returned", procName, 1);

    pta->nalloc = 2 * pta->nalloc;
    return 0;
}


/*---------------------------------------------------------------------*
 *                           Pta accessors                             *
 *---------------------------------------------------------------------*/
l_int32
ptaGetRefcount(PTA  *pta)
{
    PROCNAME("ptaGetRefcount");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    return pta->refcount;
}


l_int32
ptaChangeRefcount(PTA     *pta,
                  l_int32  delta)
{
    PROCNAME("ptaChangeRefcount");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    pta->refcount += delta;
    return 0;
}


/*!
 *  ptaGetCount()
 *
 *      Input:  pta
 *      Return: count, or 0 if no pta
 */
l_int32
ptaGetCount(PTA  *pta)
{
    PROCNAME("ptaGetCount");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 0);

    return pta->n;
}


/*!
 *  ptaGetPt()
 *
 *      Input:  pta
 *              index  (into arrays)
 *              &x (<optional return> float x value)
 *              &y (<optional return> float y value)
 *      Return: 0 if OK; 1 on error
 */
l_int32
ptaGetPt(PTA        *pta,
         l_int32     index,
         l_float32  *px,
         l_float32  *py)
{
    PROCNAME("ptaGetPt");

    if (px) *px = 0;
    if (py) *py = 0;
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    if (index < 0 || index >= pta->n)
        return ERROR_INT("invalid index", procName, 1);

    if (px) *px = pta->x[index];
    if (py) *py = pta->y[index];
    return 0;
}


/*!
 *  ptaGetIPt()
 *
 *      Input:  pta
 *              index  (into arrays)
 *              &x (<optional return> integer x value)
 *              &y (<optional return> integer y value)
 *      Return: 0 if OK; 1 on error
 */
l_int32
ptaGetIPt(PTA      *pta,
          l_int32   index,
          l_int32  *px,
          l_int32  *py)
{
    PROCNAME("ptaGetIPt");

    if (px) *px = 0;
    if (py) *py = 0;
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    if (index < 0 || index >= pta->n)
        return ERROR_INT("invalid index", procName, 1);

    if (px) *px = (l_int32)(pta->x[index] + 0.5);
    if (py) *py = (l_int32)(pta->y[index] + 0.5);
    return 0;
}


/*!
 *  ptaSetPt()
 *
 *      Input:  pta
 *              index  (into arrays)
 *              x, y
 *      Return: 0 if OK; 1 on error
 */
l_int32
ptaSetPt(PTA       *pta,
         l_int32    index,
         l_float32  x,
         l_float32  y)
{
    PROCNAME("ptaSetPt");

    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    if (index < 0 || index >= pta->n)
        return ERROR_INT("invalid index", procName, 1);

    pta->x[index] = x;
    pta->y[index] = y;
    return 0;
}


/*!
 *  ptaGetArrays()
 *
 *      Input:  pta
 *              &nax (<optional return> numa of x array)
 *              &nay (<optional return> numa of y array)
 *      Return: 0 if OK; 1 on error or if pta is empty
 *
 *  Notes:
 *      (1) This copies the internal arrays into new Numas.
 */
l_int32
ptaGetArrays(PTA    *pta,
             NUMA  **pnax,
             NUMA  **pnay)
{
l_int32  i, n;
NUMA    *nax, *nay;

    PROCNAME("ptaGetArrays");

    if (!pnax && !pnay)
        return ERROR_INT("no output requested", procName, 1);
    if (pnax) *pnax = NULL;
    if (pnay) *pnay = NULL;
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    if ((n = ptaGetCount(pta)) == 0)
        return ERROR_INT("pta is empty", procName, 1);

    if (pnax) {
        if ((nax = numaCreate(n)) == NULL)
            return ERROR_INT("nax not made", procName, 1);
        *pnax = nax;
        for (i = 0; i < n; i++)
            nax->array[i] = pta->x[i];
        nax->n = n;
    }
    if (pnay) {
        if ((nay = numaCreate(n)) == NULL)
            return ERROR_INT("nay not made", procName, 1);
        *pnay = nay;
        for (i = 0; i < n; i++)
            nay->array[i] = pta->y[i];
        nay->n = n;
    }
    return 0;
}


/*---------------------------------------------------------------------*
 *                       Pta serialized for I/O                        *
 *---------------------------------------------------------------------*/
/*!
 *  ptaRead()
 *
 *      Input:  filename
 *      Return: pta, or null on error
 */
PTA *
ptaRead(const char  *filename)
{
FILE  *fp;
PTA   *pta;

    PROCNAME("ptaRead");

    if (!filename)
        return (PTA *)ERROR_PTR("filename not defined", procName, NULL);
    if ((fp = fopenReadStream(filename)) == NULL)
        return (PTA *)ERROR_PTR("stream not opened", procName, NULL);

    if ((pta = ptaReadStream(fp)) == NULL) {
        fclose(fp);
        return (PTA *)ERROR_PTR("pta not read", procName, NULL);
    }

    fclose(fp);
    return pta;
}


/*!
 *  ptaReadStream()
 *
 *      Input:  stream
 *      Return: pta, or null on error
 */
PTA *
ptaReadStream(FILE  *fp)
{
char       typestr[128];
l_int32    i, n, ix, iy, type, version;
l_float32  x, y;
PTA       *pta;

    PROCNAME("ptaReadStream");

    if (!fp)
        return (PTA *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\n Pta Version %d\n", &version) != 1)
        return (PTA *)ERROR_PTR("not a pta file", procName, NULL);
    if (version != PTA_VERSION_NUMBER)
        return (PTA *)ERROR_PTR("invalid pta version", procName, NULL);
    if (fscanf(fp, " Number of pts = %d; format = %s\n", &n, typestr) != 2)
        return (PTA *)ERROR_PTR("not a pta file", procName, NULL);
    if (!strcmp(typestr, "float"))
        type = 0;
    else  /* typestr is "integer" */
        type = 1;

    if ((pta = ptaCreate(n)) == NULL)
        return (PTA *)ERROR_PTR("pta not made", procName, NULL);
    for (i = 0; i < n; i++) {
        if (type == 0) {  /* data is float */
            if (fscanf(fp, "   (%f, %f)\n", &x, &y) != 2)
                return (PTA *)ERROR_PTR("error reading floats", procName, NULL);
            ptaAddPt(pta, x, y);
        }
        else {   /* data is integer */
            if (fscanf(fp, "   (%d, %d)\n", &ix, &iy) != 2)
                return (PTA *)ERROR_PTR("error reading ints", procName, NULL);
            ptaAddPt(pta, ix, iy);
        }
    }

    return pta;
}


/*!
 *  ptaWrite()
 *
 *      Input:  filename
 *              pta
 *              type  (0 for float values; 1 for integer values)
 *      Return: 0 if OK, 1 on error
 */
l_int32
ptaWrite(const char  *filename,
         PTA         *pta,
         l_int32      type)
{
FILE  *fp;

    PROCNAME("ptaWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "w")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (ptaWriteStream(fp, pta, type))
        return ERROR_INT("pta not written to stream", procName, 1);
    fclose(fp);

    return 0;
}


/*!
 *  ptaWriteStream()
 *
 *      Input:  stream
 *              pta
 *              type  (0 for float values; 1 for integer values)
 *      Return: 0 if OK; 1 on error
 */
l_int32
ptaWriteStream(FILE    *fp,
               PTA     *pta,
               l_int32  type)
{
l_int32    i, n, ix, iy;
l_float32  x, y;

    PROCNAME("ptaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

    n = ptaGetCount(pta);
    fprintf(fp, "\n Pta Version %d\n", PTA_VERSION_NUMBER);
    if (type == 0)
        fprintf(fp, " Number of pts = %d; format = float\n", n);
    else  /* type == 1 */
        fprintf(fp, " Number of pts = %d; format = integer\n", n);
    for (i = 0; i < n; i++) {
        if (type == 0) {  /* data is float */
            ptaGetPt(pta, i, &x, &y);
            fprintf(fp, "   (%f, %f)\n", x, y);
        }
        else {   /* data is integer */
            ptaGetIPt(pta, i, &ix, &iy);
            fprintf(fp, "   (%d, %d)\n", ix, iy);
        }
    }

    return 0;
}


/*---------------------------------------------------------------------*
 *                     PTAA creation, destruction                      *
 *---------------------------------------------------------------------*/
/*!
 *  ptaaCreate()
 *
 *      Input:  n  (initial number of ptrs)
 *      Return: ptaa, or null on error
 */
PTAA *
ptaaCreate(l_int32  n)
{
PTAA  *ptaa;

    PROCNAME("ptaaCreate");

    if (n <= 0)
        n = INITIAL_PTR_ARRAYSIZE;

    if ((ptaa = (PTAA *)CALLOC(1, sizeof(PTAA))) == NULL)
        return (PTAA *)ERROR_PTR("ptaa not made", procName, NULL);
    ptaa->n = 0;
    ptaa->nalloc = n;

    if ((ptaa->pta = (PTA **)CALLOC(n, sizeof(PTA *))) == NULL)
        return (PTAA *)ERROR_PTR("pta ptrs not made", procName, NULL);

    return ptaa;
}


/*!
 *  ptaaDestroy()
 *
 *      Input:  &ptaa <to be nulled>
 *      Return: void
 */
void
ptaaDestroy(PTAA  **pptaa)
{
l_int32  i;
PTAA    *ptaa;

    PROCNAME("ptaaDestroy");

    if (pptaa == NULL) {
        L_WARNING("ptr address is NULL!", procName);
        return;
    }

    if ((ptaa = *pptaa) == NULL)
        return;

    for (i = 0; i < ptaa->n; i++)
        ptaDestroy(&ptaa->pta[i]);
    FREE(ptaa->pta);

    FREE(ptaa);
    *pptaa = NULL;
    return;
}


/*---------------------------------------------------------------------*
 *                          PTAA array extension                       *
 *---------------------------------------------------------------------*/
/*!
 *  ptaaAddPta()
 *
 *      Input:  ptaa
 *              pta  (to be added)
 *              copyflag  (L_INSERT, L_COPY, L_CLONE)
 *      Return: 0 if OK, 1 on error
 */
l_int32
ptaaAddPta(PTAA    *ptaa,
           PTA     *pta,
           l_int32  copyflag)
{
l_int32  n;
PTA     *ptac;

    PROCNAME("ptaaAddPta");

    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);

    if (copyflag == L_INSERT)
        ptac = pta;
    else if (copyflag == L_COPY) {
        if ((ptac = ptaCopy(pta)) == NULL)
            return ERROR_INT("ptac not made", procName, 1);
    }
    else if (copyflag == L_CLONE) {
        if ((ptac = ptaClone(pta)) == NULL)
            return ERROR_INT("pta clone not made", procName, 1);
    }
    else
        return ERROR_INT("invalid copyflag", procName, 1);

    n = ptaaGetCount(ptaa);
    if (n >= ptaa->nalloc)
        ptaaExtendArray(ptaa);
    ptaa->pta[n] = ptac;
    ptaa->n++;

    return 0;
}


/*!
 *  ptaaExtendArray()
 *
 *      Input:  ptaa
 *      Return: 0 if OK, 1 on error
 */
l_int32
ptaaExtendArray(PTAA  *ptaa)
{
    PROCNAME("ptaaExtendArray");

    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);

    if ((ptaa->pta = (PTA **)reallocNew((void **)&ptaa->pta,
                             sizeof(PTA *) * ptaa->nalloc,
                             2 * sizeof(PTA *) * ptaa->nalloc)) == NULL)
        return ERROR_INT("new ptr array not returned", procName, 1);

    ptaa->nalloc = 2 * ptaa->nalloc;
    return 0;
}


/*---------------------------------------------------------------------*
 *                          Ptaa accessors                             *
 *---------------------------------------------------------------------*/
/*!
 *  ptaaGetCount()
 *
 *      Input:  ptaa
 *      Return: count, or 0 if no ptaa
 */
l_int32
ptaaGetCount(PTAA  *ptaa)
{
    PROCNAME("ptaaGetCount");

    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 0);

    return ptaa->n;
}


/*!
 *  ptaaGetPta()
 *
 *      Input:  ptaa
 *              index  (to the i-th pta)
 *              accessflag  (L_COPY or L_CLONE)
 *      Return: pta, or null on error
 */
PTA *
ptaaGetPta(PTAA    *ptaa,
           l_int32  index,
           l_int32  accessflag)
{
    PROCNAME("ptaaGetPta");

    if (!ptaa)
        return (PTA *)ERROR_PTR("ptaa not defined", procName, NULL);
    if (index < 0 || index >= ptaa->n)
        return (PTA *)ERROR_PTR("index not valid", procName, NULL);

    if (accessflag == L_COPY)
        return ptaCopy(ptaa->pta[index]);
    else if (accessflag == L_CLONE)
        return ptaClone(ptaa->pta[index]);
    else
        return (PTA *)ERROR_PTR("invalid accessflag", procName, NULL);
}


/*!
 *  ptaaGetPt()
 *
 *      Input:  ptaa
 *              ipta  (to the i-th pta)
 *              jpt (index to the j-th pt in the pta)
 *              &x (<optional return> float x value)
 *              &y (<optional return> float y value)
 *      Return: 0 if OK; 1 on error
 */
l_int32
ptaaGetPt(PTAA       *ptaa,
           l_int32     ipta,
           l_int32     jpt,
           l_float32  *px,
           l_float32  *py)
{
PTA  *pta;

    PROCNAME("ptaaGetPt");

    if (px) *px = 0;
    if (py) *py = 0;
    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);
    if (ipta < 0 || ipta >= ptaa->n)
        return ERROR_INT("index ipta not valid", procName, 1);

    pta = ptaaGetPta(ptaa, ipta, L_CLONE);
    if (jpt < 0 || jpt >= pta->n) {
        ptaDestroy(&pta);
        return ERROR_INT("index jpt not valid", procName, 1);
    }

    ptaGetPt(pta, jpt, px, py);
    ptaDestroy(&pta);
    return 0;
}


/*---------------------------------------------------------------------*
 *                       Ptaa serialized for I/O                       *
 *---------------------------------------------------------------------*/
/*!
 *  ptaaRead()
 *
 *      Input:  filename
 *      Return: ptaa, or null on error
 */
PTAA *
ptaaRead(const char  *filename)
{
FILE  *fp;
PTAA  *ptaa;

    PROCNAME("ptaaRead");

    if (!filename)
        return (PTAA *)ERROR_PTR("filename not defined", procName, NULL);
    if ((fp = fopenReadStream(filename)) == NULL)
        return (PTAA *)ERROR_PTR("stream not opened", procName, NULL);

    if ((ptaa = ptaaReadStream(fp)) == NULL) {
        fclose(fp);
        return (PTAA *)ERROR_PTR("ptaa not read", procName, NULL);
    }

    fclose(fp);
    return ptaa;
}


/*!
 *  ptaaReadStream()
 *
 *      Input:  stream
 *      Return: ptaa, or null on error
 */
PTAA *
ptaaReadStream(FILE  *fp)
{
l_int32  i, n, version;
PTA     *pta;
PTAA    *ptaa;

    PROCNAME("ptaaReadStream");

    if (!fp)
        return (PTAA *)ERROR_PTR("stream not defined", procName, NULL);

    if (fscanf(fp, "\nPtaa Version %d\n", &version) != 1)
        return (PTAA *)ERROR_PTR("not a ptaa file", procName, NULL);
    if (version != PTA_VERSION_NUMBER)
        return (PTAA *)ERROR_PTR("invalid ptaa version", procName, NULL);
    if (fscanf(fp, "Number of Pta = %d\n", &n) != 1)
        return (PTAA *)ERROR_PTR("not a ptaa file", procName, NULL);

    if ((ptaa = ptaaCreate(n)) == NULL)
        return (PTAA *)ERROR_PTR("ptaa not made", procName, NULL);
    for (i = 0; i < n; i++) {
        if ((pta = ptaReadStream(fp)) == NULL)
            return (PTAA *)ERROR_PTR("error reading pta", procName, NULL);
        ptaaAddPta(ptaa, pta, L_INSERT);
    }

    return ptaa;
}


/*!
 *  ptaaWrite()
 *
 *      Input:  filename
 *              ptaa
 *              type  (0 for float values; 1 for integer values)
 *      Return: 0 if OK, 1 on error
 */
l_int32
ptaaWrite(const char  *filename,
          PTAA        *ptaa,
          l_int32      type)
{
FILE  *fp;

    PROCNAME("ptaaWrite");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);

    if ((fp = fopenWriteStream(filename, "w")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    if (ptaaWriteStream(fp, ptaa, type))
        return ERROR_INT("ptaa not written to stream", procName, 1);
    fclose(fp);

    return 0;
}


/*!
 *  ptaaWriteStream()
 *
 *      Input:  stream
 *              ptaa
 *              type  (0 for float values; 1 for integer values)
 *      Return: 0 if OK; 1 on error
 */
l_int32
ptaaWriteStream(FILE    *fp,
                PTAA    *ptaa,
                l_int32  type)
{
l_int32  i, n;
PTA     *pta;

    PROCNAME("ptaaWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!ptaa)
        return ERROR_INT("ptaa not defined", procName, 1);

    n = ptaaGetCount(ptaa);
    fprintf(fp, "\nPtaa Version %d\n", PTA_VERSION_NUMBER);
    fprintf(fp, "Number of Pta = %d\n", n);
    for (i = 0; i < n; i++) {
        pta = ptaaGetPta(ptaa, i, L_CLONE);
        ptaWriteStream(fp, pta, type);
        ptaDestroy(&pta);
    }

    return 0;
}

