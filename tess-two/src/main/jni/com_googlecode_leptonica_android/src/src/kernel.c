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
 *  kernel.c
 *
 *      Basic operations on kernels for image convolution
 *
 *         Create/destroy/copy
 *            L_KERNEL   *kernelCreate()
 *            void        kernelDestroy()
 *            L_KERNEL   *kernelCopy()
 *
 *         Accessors:
 *            l_int32     kernelGetElement()
 *            l_int32     kernelSetElement()
 *            l_int32     kernelGetParameters()
 *            l_int32     kernelSetOrigin()
 *            l_int32     kernelGetSum()
 *            l_int32     kernelGetMinMax()
 *
 *         Normalize/invert
 *            L_KERNEL   *kernelNormalize()
 *            L_KERNEL   *kernelInvert()
 *
 *         Helper function
 *            l_float32 **create2dFloatArray()
 *
 *         Serialized I/O
 *            L_KERNEL   *kernelRead()
 *            L_KERNEL   *kernelReadStream()
 *            l_int32     kernelWrite()
 *            l_int32     kernelWriteStream()
 *
 *         Making a kernel from a compiled string
 *            L_KERNEL   *kernelCreateFromString()
 *
 *         Making a kernel from a simple file format
 *            L_KERNEL   *kernelCreateFromFile()
 *
 *         Making a kernel from a Pix
 *            L_KERNEL   *kernelCreateFromPix()
 *
 *         Display a kernel in a pix
 *            PIX        *kernelDisplayInPix()
 *
 *         Parse string to extract numbers
 *            NUMA       *parseStringForNumbers()
 *
 *      Simple parametric kernels
 *            L_KERNEL   *makeFlatKernel()
 *            L_KERNEL   *makeGaussianKernel()
 *            L_KERNEL   *makeGaussianKernelSep()
 *            L_KERNEL   *makeDoGKernel()
 */

#include <string.h>
#include <math.h>
#include "allheaders.h"


/*------------------------------------------------------------------------*
 *                           Create / Destroy                             *
 *------------------------------------------------------------------------*/
/*!
 *  kernelCreate()
 *
 *      Input:  height, width
 *      Return: kernel, or null on error
 *
 *  Notes:
 *      (1) kernelCreate() initializes all values to 0.
 *      (2) After this call, (cy,cx) and nonzero data values must be
 *          assigned.
 */
L_KERNEL *
kernelCreate(l_int32  height,
             l_int32  width)
{
L_KERNEL  *kel;

    PROCNAME("kernelCreate");

    if ((kel = (L_KERNEL *)CALLOC(1, sizeof(L_KERNEL))) == NULL)
        return (L_KERNEL *)ERROR_PTR("kel not made", procName, NULL);
    kel->sy = height;
    kel->sx = width;
    if ((kel->data = create2dFloatArray(height, width)) == NULL)
        return (L_KERNEL *)ERROR_PTR("data not allocated", procName, NULL);

    return kel;
}


/*!
 *  kernelDestroy()
 *
 *      Input:  &kel (<to be nulled>)
 *      Return: void
 */
void
kernelDestroy(L_KERNEL  **pkel)
{
l_int32    i;
L_KERNEL  *kel;

    PROCNAME("kernelDestroy");

    if (pkel == NULL)  {
        L_WARNING("ptr address is NULL!\n", procName);
        return;
    }
    if ((kel = *pkel) == NULL)
        return;

    for (i = 0; i < kel->sy; i++)
        FREE(kel->data[i]);
    FREE(kel->data);
    FREE(kel);

    *pkel = NULL;
    return;
}


/*!
 *  kernelCopy()
 *
 *      Input:  kels (source kernel)
 *      Return: keld (copy of kels), or null on error
 */
L_KERNEL *
kernelCopy(L_KERNEL  *kels)
{
l_int32    i, j, sx, sy, cx, cy;
L_KERNEL  *keld;

    PROCNAME("kernelCopy");

    if (!kels)
        return (L_KERNEL *)ERROR_PTR("kels not defined", procName, NULL);

    kernelGetParameters(kels, &sy, &sx, &cy, &cx);
    if ((keld = kernelCreate(sy, sx)) == NULL)
        return (L_KERNEL *)ERROR_PTR("keld not made", procName, NULL);
    keld->cy = cy;
    keld->cx = cx;
    for (i = 0; i < sy; i++)
        for (j = 0; j < sx; j++)
            keld->data[i][j] = kels->data[i][j];

    return keld;
}


/*----------------------------------------------------------------------*
 *                               Accessors                              *
 *----------------------------------------------------------------------*/
/*!
 *  kernelGetElement()
 *
 *      Input:  kel
 *              row
 *              col
 *              &val
 *      Return: 0 if OK; 1 on error
 */
l_int32
kernelGetElement(L_KERNEL   *kel,
                 l_int32     row,
                 l_int32     col,
                 l_float32  *pval)
{
    PROCNAME("kernelGetElement");

    if (!pval)
        return ERROR_INT("&val not defined", procName, 1);
    *pval = 0;
    if (!kel)
        return ERROR_INT("kernel not defined", procName, 1);
    if (row < 0 || row >= kel->sy)
        return ERROR_INT("kernel row out of bounds", procName, 1);
    if (col < 0 || col >= kel->sx)
        return ERROR_INT("kernel col out of bounds", procName, 1);

    *pval = kel->data[row][col];
    return 0;
}


/*!
 *  kernelSetElement()
 *
 *      Input:  kernel
 *              row
 *              col
 *              val
 *      Return: 0 if OK; 1 on error
 */
l_int32
kernelSetElement(L_KERNEL  *kel,
                 l_int32    row,
                 l_int32    col,
                 l_float32  val)
{
    PROCNAME("kernelSetElement");

    if (!kel)
        return ERROR_INT("kel not defined", procName, 1);
    if (row < 0 || row >= kel->sy)
        return ERROR_INT("kernel row out of bounds", procName, 1);
    if (col < 0 || col >= kel->sx)
        return ERROR_INT("kernel col out of bounds", procName, 1);

    kel->data[row][col] = val;
    return 0;
}


/*!
 *  kernelGetParameters()
 *
 *      Input:  kernel
 *              &sy, &sx, &cy, &cx (<optional return>; each can be null)
 *      Return: 0 if OK, 1 on error
 */
l_int32
kernelGetParameters(L_KERNEL  *kel,
                    l_int32   *psy,
                    l_int32   *psx,
                    l_int32   *pcy,
                    l_int32   *pcx)
{
    PROCNAME("kernelGetParameters");

    if (psy) *psy = 0;
    if (psx) *psx = 0;
    if (pcy) *pcy = 0;
    if (pcx) *pcx = 0;
    if (!kel)
        return ERROR_INT("kernel not defined", procName, 1);
    if (psy) *psy = kel->sy;
    if (psx) *psx = kel->sx;
    if (pcy) *pcy = kel->cy;
    if (pcx) *pcx = kel->cx;
    return 0;
}


/*!
 *  kernelSetOrigin()
 *
 *      Input:  kernel
 *              cy, cx
 *      Return: 0 if OK; 1 on error
 */
l_int32
kernelSetOrigin(L_KERNEL  *kel,
                l_int32    cy,
                l_int32    cx)
{
    PROCNAME("kernelSetOrigin");

    if (!kel)
        return ERROR_INT("kel not defined", procName, 1);
    kel->cy = cy;
    kel->cx = cx;
    return 0;
}


/*!
 *  kernelGetSum()
 *
 *      Input:  kernel
 *              &sum (<return> sum of all kernel values)
 *      Return: 0 if OK, 1 on error
 */
l_int32
kernelGetSum(L_KERNEL   *kel,
             l_float32  *psum)
{
l_int32    sx, sy, i, j;

    PROCNAME("kernelGetSum");

    if (!psum)
        return ERROR_INT("&sum not defined", procName, 1);
    *psum = 0.0;
    if (!kel)
        return ERROR_INT("kernel not defined", procName, 1);

    kernelGetParameters(kel, &sy, &sx, NULL, NULL);
    for (i = 0; i < sy; i++) {
        for (j = 0; j < sx; j++) {
            *psum += kel->data[i][j];
        }
    }
    return 0;
}


/*!
 *  kernelGetMinMax()
 *
 *      Input:  kernel
 *              &min (<optional return> minimum value)
 *              &max (<optional return> maximum value)
 *      Return: 0 if OK, 1 on error
 */
l_int32
kernelGetMinMax(L_KERNEL   *kel,
                l_float32  *pmin,
                l_float32  *pmax)
{
l_int32    sx, sy, i, j;
l_float32  val, minval, maxval;

    PROCNAME("kernelGetMinmax");

    if (!pmin && !pmax)
        return ERROR_INT("neither &min nor &max defined", procName, 1);
    if (pmin) *pmin = 0.0;
    if (pmax) *pmax = 0.0;
    if (!kel)
        return ERROR_INT("kernel not defined", procName, 1);

    kernelGetParameters(kel, &sy, &sx, NULL, NULL);
    minval = 10000000.0;
    maxval = -10000000.0;
    for (i = 0; i < sy; i++) {
        for (j = 0; j < sx; j++) {
            val = kel->data[i][j];
            if (val < minval)
                minval = val;
            if (val > maxval)
                maxval = val;
        }
    }
    if (pmin)
        *pmin = minval;
    if (pmax)
        *pmax = maxval;

    return 0;
}


/*----------------------------------------------------------------------*
 *                          Normalize/Invert                            *
 *----------------------------------------------------------------------*/
/*!
 *  kernelNormalize()
 *
 *      Input:  kels (source kel, to be normalized)
 *              normsum (desired sum of elements in keld)
 *      Return: keld (normalized version of kels), or null on error
 *                   or if sum of elements is very close to 0)
 *
 *  Notes:
 *      (1) If the sum of kernel elements is close to 0, do not
 *          try to calculate the normalized kernel.  Instead,
 *          return a copy of the input kernel, with a warning.
 */
L_KERNEL *
kernelNormalize(L_KERNEL  *kels,
                l_float32  normsum)
{
l_int32    i, j, sx, sy, cx, cy;
l_float32  sum, factor;
L_KERNEL  *keld;

    PROCNAME("kernelNormalize");

    if (!kels)
        return (L_KERNEL *)ERROR_PTR("kels not defined", procName, NULL);

    kernelGetSum(kels, &sum);
    if (L_ABS(sum) < 0.00001) {
        L_WARNING("null sum; not normalizing; returning a copy\n", procName);
        return kernelCopy(kels);
    }

    kernelGetParameters(kels, &sy, &sx, &cy, &cx);
    if ((keld = kernelCreate(sy, sx)) == NULL)
        return (L_KERNEL *)ERROR_PTR("keld not made", procName, NULL);
    keld->cy = cy;
    keld->cx = cx;

    factor = normsum / sum;
    for (i = 0; i < sy; i++)
        for (j = 0; j < sx; j++)
            keld->data[i][j] = factor * kels->data[i][j];

    return keld;
}


/*!
 *  kernelInvert()
 *
 *      Input:  kels (source kel, to be inverted)
 *      Return: keld (spatially inverted, about the origin), or null on error
 *
 *  Notes:
 *      (1) For convolution, the kernel is spatially inverted before
 *          a "correlation" operation is done between the kernel and the image.
 */
L_KERNEL *
kernelInvert(L_KERNEL  *kels)
{
l_int32    i, j, sx, sy, cx, cy;
L_KERNEL  *keld;

    PROCNAME("kernelInvert");

    if (!kels)
        return (L_KERNEL *)ERROR_PTR("kels not defined", procName, NULL);

    kernelGetParameters(kels, &sy, &sx, &cy, &cx);
    if ((keld = kernelCreate(sy, sx)) == NULL)
        return (L_KERNEL *)ERROR_PTR("keld not made", procName, NULL);
    keld->cy = sy - 1 - cy;
    keld->cx = sx - 1 - cx;

    for (i = 0; i < sy; i++)
        for (j = 0; j < sx; j++)
            keld->data[i][j] = kels->data[sy - 1 - i][sx - 1 - j];

    return keld;
}


/*----------------------------------------------------------------------*
 *                            Helper function                           *
 *----------------------------------------------------------------------*/
/*!
 *  create2dFloatArray()
 *
 *      Input:  sy (rows == height)
 *              sx (columns == width)
 *      Return: doubly indexed array (i.e., an array of sy row pointers,
 *              each of which points to an array of sx floats)
 *
 *  Notes:
 *      (1) The array[sy][sx] is indexed in standard "matrix notation",
 *          with the row index first.
 */
l_float32 **
create2dFloatArray(l_int32  sy,
                   l_int32  sx)
{
l_int32      i;
l_float32  **array;

    PROCNAME("create2dFloatArray");

    if ((array = (l_float32 **)CALLOC(sy, sizeof(l_float32 *))) == NULL)
        return (l_float32 **)ERROR_PTR("ptr array not made", procName, NULL);

    for (i = 0; i < sy; i++) {
        if ((array[i] = (l_float32 *)CALLOC(sx, sizeof(l_float32))) == NULL)
            return (l_float32 **)ERROR_PTR("array not made", procName, NULL);
    }

    return array;
}


/*----------------------------------------------------------------------*
 *                            Kernel serialized I/O                     *
 *----------------------------------------------------------------------*/
/*!
 *  kernelRead()
 *
 *      Input:  filename
 *      Return: kernel, or null on error
 */
L_KERNEL *
kernelRead(const char  *fname)
{
FILE      *fp;
L_KERNEL  *kel;

    PROCNAME("kernelRead");

    if (!fname)
        return (L_KERNEL *)ERROR_PTR("fname not defined", procName, NULL);

    if ((fp = fopenReadStream(fname)) == NULL)
        return (L_KERNEL *)ERROR_PTR("stream not opened", procName, NULL);
    if ((kel = kernelReadStream(fp)) == NULL)
        return (L_KERNEL *)ERROR_PTR("kel not returned", procName, NULL);
    fclose(fp);

    return kel;
}


/*!
 *  kernelReadStream()
 *
 *      Input:  stream
 *      Return: kernel, or null on error
 */
L_KERNEL *
kernelReadStream(FILE  *fp)
{
l_int32    sy, sx, cy, cx, i, j, ret, version, ignore;
L_KERNEL  *kel;

    PROCNAME("kernelReadStream");

    if (!fp)
        return (L_KERNEL *)ERROR_PTR("stream not defined", procName, NULL);

    ret = fscanf(fp, "  Kernel Version %d\n", &version);
    if (ret != 1)
        return (L_KERNEL *)ERROR_PTR("not a kernel file", procName, NULL);
    if (version != KERNEL_VERSION_NUMBER)
        return (L_KERNEL *)ERROR_PTR("invalid kernel version", procName, NULL);

    if (fscanf(fp, "  sy = %d, sx = %d, cy = %d, cx = %d\n",
            &sy, &sx, &cy, &cx) != 4)
        return (L_KERNEL *)ERROR_PTR("dimensions not read", procName, NULL);

    if ((kel = kernelCreate(sy, sx)) == NULL)
        return (L_KERNEL *)ERROR_PTR("kel not made", procName, NULL);
    kernelSetOrigin(kel, cy, cx);

    for (i = 0; i < sy; i++) {
        for (j = 0; j < sx; j++)
            ignore = fscanf(fp, "%15f", &kel->data[i][j]);
        ignore = fscanf(fp, "\n");
    }
    ignore = fscanf(fp, "\n");

    return kel;
}


/*!
 *  kernelWrite()
 *
 *      Input:  fname (output file)
 *              kernel
 *      Return: 0 if OK, 1 on error
 */
l_int32
kernelWrite(const char  *fname,
            L_KERNEL    *kel)
{
FILE  *fp;

    PROCNAME("kernelWrite");

    if (!fname)
        return ERROR_INT("fname not defined", procName, 1);
    if (!kel)
        return ERROR_INT("kel not defined", procName, 1);

    if ((fp = fopenWriteStream(fname, "wb")) == NULL)
        return ERROR_INT("stream not opened", procName, 1);
    kernelWriteStream(fp, kel);
    fclose(fp);

    return 0;
}


/*!
 *  kernelWriteStream()
 *
 *      Input:  stream
 *              kel
 *      Return: 0 if OK, 1 on error
 */
l_int32
kernelWriteStream(FILE      *fp,
                  L_KERNEL  *kel)
{
l_int32  sx, sy, cx, cy, i, j;

    PROCNAME("kernelWriteStream");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!kel)
        return ERROR_INT("kel not defined", procName, 1);
    kernelGetParameters(kel, &sy, &sx, &cy, &cx);

    fprintf(fp, "  Kernel Version %d\n", KERNEL_VERSION_NUMBER);
    fprintf(fp, "  sy = %d, sx = %d, cy = %d, cx = %d\n", sy, sx, cy, cx);
    for (i = 0; i < sy; i++) {
        for (j = 0; j < sx; j++)
            fprintf(fp, "%15.4f", kel->data[i][j]);
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");

    return 0;
}


/*----------------------------------------------------------------------*
 *                 Making a kernel from a compiled string               *
 *----------------------------------------------------------------------*/
/*!
 *  kernelCreateFromString()
 *
 *      Input:  height, width
 *              cy, cx   (origin)
 *              kdata
 *      Return: kernel of the given size, or null on error
 *
 *  Notes:
 *      (1) The data is an array of chars, in row-major order, giving
 *          space separated integers in the range [-255 ... 255].
 *      (2) The only other formatting limitation is that you must
 *          leave space between the last number in each row and
 *          the double-quote.  If possible, it's also nice to have each
 *          line in the string represent a line in the kernel; e.g.,
 *              static const char *kdata =
 *                  " 20   50   20 "
 *                  " 70  140   70 "
 *                  " 20   50   20 ";
 */
L_KERNEL *
kernelCreateFromString(l_int32      h,
                       l_int32      w,
                       l_int32      cy,
                       l_int32      cx,
                       const char  *kdata)
{
l_int32    n, i, j, index;
l_float32  val;
L_KERNEL  *kel;
NUMA      *na;

    PROCNAME("kernelCreateFromString");

    if (h < 1)
        return (L_KERNEL *)ERROR_PTR("height must be > 0", procName, NULL);
    if (w < 1)
        return (L_KERNEL *)ERROR_PTR("width must be > 0", procName, NULL);
    if (cy < 0 || cy >= h)
        return (L_KERNEL *)ERROR_PTR("cy invalid", procName, NULL);
    if (cx < 0 || cx >= w)
        return (L_KERNEL *)ERROR_PTR("cx invalid", procName, NULL);

    kel = kernelCreate(h, w);
    kernelSetOrigin(kel, cy, cx);
    na = parseStringForNumbers(kdata, " \t\n");
    n = numaGetCount(na);
    if (n != w * h) {
        numaDestroy(&na);
	fprintf(stderr, "w = %d, h = %d, num ints = %d\n", w, h, n);
        return (L_KERNEL *)ERROR_PTR("invalid integer data", procName, NULL);
    }

    index = 0;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            numaGetFValue(na, index, &val);
            kernelSetElement(kel, i, j, val);
	    index++;
        }
    }

    numaDestroy(&na);
    return kel;
}


/*----------------------------------------------------------------------*
 *                Making a kernel from a simple file format             *
 *----------------------------------------------------------------------*/
/*!
 *  kernelCreateFromFile()
 *
 *      Input:  filename
 *      Return: kernel, or null on error
 *
 *  Notes:
 *      (1) The file contains, in the following order:
 *           - Any number of comment lines starting with '#' are ignored
 *           - The height and width of the kernel
 *           - The y and x values of the kernel origin
 *           - The kernel data, formatted as lines of numbers (integers
 *             or floats) for the kernel values in row-major order,
 *             and with no other punctuation.
 *             (Note: this differs from kernelCreateFromString(),
 *             where each line must begin and end with a double-quote
 *             to tell the compiler it's part of a string.)
 *           - The kernel specification ends when a blank line,
 *             a comment line, or the end of file is reached.
 *      (2) All lines must be left-justified.
 *      (3) See kernelCreateFromString() for a description of the string
 *          format for the kernel data.  As an example, here are the lines
 *          of a valid kernel description file  In the file, all lines
 *          are left-justified:
 *                    # small 3x3 kernel
 *                    3 3
 *                    1 1
 *                    25.5   51    24.3
 *                    70.2  146.3  73.4
 *                    20     50.9  18.4
 */
L_KERNEL *
kernelCreateFromFile(const char  *filename)
{
char      *filestr, *line;
l_int32    nlines, i, j, first, index, w, h, cx, cy, n;
l_float32  val;
size_t     size;
NUMA      *na, *nat;
SARRAY    *sa;
L_KERNEL  *kel;

    PROCNAME("kernelCreateFromFile");

    if (!filename)
        return (L_KERNEL *)ERROR_PTR("filename not defined", procName, NULL);

    filestr = (char *)l_binaryRead(filename, &size);
    sa = sarrayCreateLinesFromString(filestr, 1);
    FREE(filestr);
    nlines = sarrayGetCount(sa);

        /* Find the first data line. */
    for (i = 0; i < nlines; i++) {
        line = sarrayGetString(sa, i, L_NOCOPY);
        if (line[0] != '#') {
            first = i;
            break;
        }
    }

        /* Find the kernel dimensions and origin location. */
    line = sarrayGetString(sa, first, L_NOCOPY);
    if (sscanf(line, "%d %d", &h, &w) != 2)
        return (L_KERNEL *)ERROR_PTR("error reading h,w", procName, NULL);
    line = sarrayGetString(sa, first + 1, L_NOCOPY);
    if (sscanf(line, "%d %d", &cy, &cx) != 2)
        return (L_KERNEL *)ERROR_PTR("error reading cy,cx", procName, NULL);

        /* Extract the data.  This ends when we reach eof, or when we
         * encounter a line of data that is either a null string or
         * contains just a newline. */
    na = numaCreate(0);
    for (i = first + 2; i < nlines; i++) {
        line = sarrayGetString(sa, i, L_NOCOPY);
        if (line[0] == '\0' || line[0] == '\n' || line[0] == '#')
            break;
        nat = parseStringForNumbers(line, " \t\n");
        numaJoin(na, nat, 0, -1);
        numaDestroy(&nat);
    }
    sarrayDestroy(&sa);

    n = numaGetCount(na);
    if (n != w * h) {
        numaDestroy(&na);
        fprintf(stderr, "w = %d, h = %d, num ints = %d\n", w, h, n);
        return (L_KERNEL *)ERROR_PTR("invalid integer data", procName, NULL);
    }

    kel = kernelCreate(h, w);
    kernelSetOrigin(kel, cy, cx);
    index = 0;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            numaGetFValue(na, index, &val);
            kernelSetElement(kel, i, j, val);
            index++;
        }
    }

    numaDestroy(&na);
    return kel;
}


/*----------------------------------------------------------------------*
 *                       Making a kernel from a Pix                     *
 *----------------------------------------------------------------------*/
/*!
 *  kernelCreateFromPix()
 *
 *      Input:  pix
 *              cy, cx (origin of kernel)
 *      Return: kernel, or null on error
 *
 *  Notes:
 *      (1) The origin must be positive and within the dimensions of the pix.
 */
L_KERNEL *
kernelCreateFromPix(PIX         *pix,
                    l_int32      cy,
                    l_int32      cx)
{
l_int32    i, j, w, h, d;
l_uint32   val;
L_KERNEL  *kel;

    PROCNAME("kernelCreateFromPix");

    if (!pix)
        return (L_KERNEL *)ERROR_PTR("pix not defined", procName, NULL);
    pixGetDimensions(pix, &w, &h, &d);
    if (d != 8)
        return (L_KERNEL *)ERROR_PTR("pix not 8 bpp", procName, NULL);
    if (cy < 0 || cx < 0 || cy >= h || cx >= w)
        return (L_KERNEL *)ERROR_PTR("(cy, cx) invalid", procName, NULL);

    kel = kernelCreate(h, w);
    kernelSetOrigin(kel, cy, cx);
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            pixGetPixel(pix, j, i, &val);
            kernelSetElement(kel, i, j, (l_float32)val);
        }
    }

    return kel;
}


/*----------------------------------------------------------------------*
 *                     Display a kernel in a pix                        *
 *----------------------------------------------------------------------*/
/*!
 *  kernelDisplayInPix()
 *
 *      Input:  kernel
 *              size (of grid interiors; odd; either 1 or a minimum size
 *                    of 17 is enforced)
 *              gthick (grid thickness; either 0 or a minimum size of 2
 *                      is enforced)
 *      Return: pix (display of kernel), or null on error
 *
 *  Notes:
 *      (1) This gives a visual representation of a kernel.
 *      (2) There are two modes of display:
 *          (a) Grid lines of minimum width 2, surrounding regions
 *              representing kernel elements of minimum size 17,
 *              with a "plus" mark at the kernel origin, or
 *          (b) A pix without grid lines and using 1 pixel per kernel element.
 *      (3) For both cases, the kernel absolute value is displayed,
 *          normalized such that the maximum absolute value is 255.
 *      (4) Large 2D separable kernels should be used for convolution
 *          with two 1D kernels.  However, for the bilateral filter,
 *          the computation time is independent of the size of the
 *          2D content kernel.
 */
PIX *
kernelDisplayInPix(L_KERNEL     *kel,
                   l_int32       size,
                   l_int32       gthick)
{
l_int32    i, j, w, h, sx, sy, cx, cy, width, x0, y0;
l_int32    normval;
l_float32  minval, maxval, max, val, norm;
PIX       *pixd, *pixt0, *pixt1;

    PROCNAME("kernelDisplayInPix");

    if (!kel)
        return (PIX *)ERROR_PTR("kernel not defined", procName, NULL);

        /* Normalize the max value to be 255 for display */
    kernelGetParameters(kel, &sy, &sx, &cy, &cx);
    kernelGetMinMax(kel, &minval, &maxval);
    max = L_MAX(maxval, -minval);
    if (max == 0.0)
        return (PIX *)ERROR_PTR("kernel elements all 0.0", procName, NULL);
    norm = 255. / (l_float32)max;

        /* Handle the 1 element/pixel case; typically with large kernels */
    if (size == 1 && gthick == 0) {
        pixd = pixCreate(sx, sy, 8);
        for (i = 0; i < sy; i++) {
            for (j = 0; j < sx; j++) {
                kernelGetElement(kel, i, j, &val);
                normval = (l_int32)(norm * L_ABS(val));
                pixSetPixel(pixd, j, i, normval);
            }
        }
        return pixd;
    }

        /* Enforce the constraints for the grid line version */
    if (size < 17) {
        L_WARNING("size < 17; setting to 17\n", procName);
        size = 17;
    }
    if (size % 2 == 0)
        size++;
    if (gthick < 2) {
        L_WARNING("grid thickness < 2; setting to 2\n", procName);
        gthick = 2;
    }

    w = size * sx + gthick * (sx + 1);
    h = size * sy + gthick * (sy + 1);
    pixd = pixCreate(w, h, 8);

        /* Generate grid lines */
    for (i = 0; i <= sy; i++)
        pixRenderLine(pixd, 0, gthick / 2 + i * (size + gthick),
                      w - 1, gthick / 2 + i * (size + gthick),
                      gthick, L_SET_PIXELS);
    for (j = 0; j <= sx; j++)
        pixRenderLine(pixd, gthick / 2 + j * (size + gthick), 0,
                      gthick / 2 + j * (size + gthick), h - 1,
                      gthick, L_SET_PIXELS);

        /* Generate mask for each element */
    pixt0 = pixCreate(size, size, 1);
    pixSetAll(pixt0);

        /* Generate crossed lines for origin pattern */
    pixt1 = pixCreate(size, size, 1);
    width = size / 8;
    pixRenderLine(pixt1, size / 2, (l_int32)(0.12 * size),
                           size / 2, (l_int32)(0.88 * size),
                           width, L_SET_PIXELS);
    pixRenderLine(pixt1, (l_int32)(0.15 * size), size / 2,
                           (l_int32)(0.85 * size), size / 2,
                           width, L_FLIP_PIXELS);
    pixRasterop(pixt1, size / 2 - width, size / 2 - width,
                2 * width, 2 * width, PIX_NOT(PIX_DST), NULL, 0, 0);

        /* Paste the patterns in */
    y0 = gthick;
    for (i = 0; i < sy; i++) {
        x0 = gthick;
        for (j = 0; j < sx; j++) {
            kernelGetElement(kel, i, j, &val);
            normval = (l_int32)(norm * L_ABS(val));
            pixSetMaskedGeneral(pixd, pixt0, normval, x0, y0);
	    if (i == cy && j == cx)
                pixPaintThroughMask(pixd, pixt1, x0, y0, 255 - normval);
            x0 += size + gthick;
        }
        y0 += size + gthick;
    }

    pixDestroy(&pixt0);
    pixDestroy(&pixt1);
    return pixd;
}


/*------------------------------------------------------------------------*
 *                     Parse string to extract numbers                    *
 *------------------------------------------------------------------------*/
/*!
 *  parseStringForNumbers()
 *
 *      Input:  string (containing numbers; not changed)
 *              seps (string of characters that can be used between ints)
 *      Return: numa (of numbers found), or null on error
 *
 *  Note:
 *     (1) The numbers can be ints or floats.
 */
NUMA *
parseStringForNumbers(const char  *str,
                      const char  *seps)
{
char      *newstr, *head, *tail;
l_float32  val;
NUMA      *na;

    PROCNAME("parseStringForNumbers");

    if (!str)
        return (NUMA *)ERROR_PTR("str not defined", procName, NULL);

    newstr = stringNew(str);  /* to enforce const-ness of str */
    na = numaCreate(0);
    head = strtokSafe(newstr, seps, &tail);
    val = atof(head);
    numaAddNumber(na, val);
    FREE(head);
    while ((head = strtokSafe(NULL, seps, &tail)) != NULL) {
        val = atof(head);
        numaAddNumber(na, val);
        FREE(head);
    }

    FREE(newstr);
    return na;
}


/*------------------------------------------------------------------------*
 *                        Simple parametric kernels                       *
 *------------------------------------------------------------------------*/
/*!
 *  makeFlatKernel()
 *
 *      Input:  height, width
 *              cy, cx (origin of kernel)
 *      Return: kernel, or null on error
 *
 *  Notes:
 *      (1) This is the same low-pass filtering kernel that is used
 *          in the block convolution functions.
 *      (2) The kernel origin (@cy, @cx) is typically placed as near
 *          the center of the kernel as possible.  If height and
 *          width are odd, then using cy = height / 2 and
 *          cx = width / 2 places the origin at the exact center.
 *      (3) This returns a normalized kernel.
 */
L_KERNEL *
makeFlatKernel(l_int32  height,
               l_int32  width,
               l_int32  cy,
               l_int32  cx)
{
l_int32    i, j;
l_float32  normval;
L_KERNEL  *kel;

    PROCNAME("makeFlatKernel");

    if ((kel = kernelCreate(height, width)) == NULL)
        return (L_KERNEL *)ERROR_PTR("kel not made", procName, NULL);
    kernelSetOrigin(kel, cy, cx);
    normval = 1.0 / (l_float32)(height * width);
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            kernelSetElement(kel, i, j, normval);
        }
    }

    return kel;
}


/*!
 *  makeGaussianKernel()
 *
 *      Input:  halfheight, halfwidth (sx = 2 * halfwidth + 1, etc)
 *              stdev (standard deviation)
 *              max (value at (cx,cy))
 *      Return: kernel, or null on error
 *
 *  Notes:
 *      (1) The kernel size (sx, sy) = (2 * halfwidth + 1, 2 * halfheight + 1).
 *      (2) The kernel center (cx, cy) = (halfwidth, halfheight).
 *      (3) The halfwidth and halfheight are typically equal, and
 *          are typically several times larger than the standard deviation.
 *      (4) If pixConvolve() is invoked with normalization (the sum of
 *          kernel elements = 1.0), use 1.0 for max (or any number that's
 *          not too small or too large).
 */
L_KERNEL *
makeGaussianKernel(l_int32    halfheight,
                   l_int32    halfwidth,
                   l_float32  stdev,
                   l_float32  max)
{
l_int32    sx, sy, i, j;
l_float32  val;
L_KERNEL  *kel;

    PROCNAME("makeGaussianKernel");

    sx = 2 * halfwidth + 1;
    sy = 2 * halfheight + 1;
    if ((kel = kernelCreate(sy, sx)) == NULL)
        return (L_KERNEL *)ERROR_PTR("kel not made", procName, NULL);
    kernelSetOrigin(kel, halfheight, halfwidth);
    for (i = 0; i < sy; i++) {
        for (j = 0; j < sx; j++) {
            val = expf(-(l_float32)((i - halfheight) * (i - halfheight) +
                                    (j - halfwidth) * (j - halfwidth)) /
                        (2. * stdev * stdev));
            kernelSetElement(kel, i, j, max * val);
        }
    }

    return kel;
}


/*!
 *  makeGaussianKernelSep()
 *
 *      Input:  halfheight, halfwidth (sx = 2 * halfwidth + 1, etc)
 *              stdev (standard deviation)
 *              max (value at (cx,cy))
 *              &kelx (<return> x part of kernel)
 *              &kely (<return> y part of kernel)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) See makeGaussianKernel() for description of input parameters.
 *      (2) These kernels are constructed so that the result of both
 *          normalized and un-normalized convolution will be the same
 *          as when convolving with pixConvolve() using the full kernel.
 *      (3) The trick for the un-normalized convolution is to have the
 *          product of the two kernel elemets at (cx,cy) be equal to max,
 *          not max**2.  That's why the max for kely is 1.0.  If instead
 *          we use sqrt(max) for both, the results are slightly less
 *          accurate, when compared to using the full kernel in
 *          makeGaussianKernel().
 */
l_int32
makeGaussianKernelSep(l_int32    halfheight,
                      l_int32    halfwidth,
                      l_float32  stdev,
                      l_float32  max,
                      L_KERNEL **pkelx,
                      L_KERNEL **pkely)
{
    PROCNAME("makeGaussianKernelSep");

    if (!pkelx || !pkely)
        return ERROR_INT("&kelx and &kely not defined", procName, 1);

    *pkelx = makeGaussianKernel(0, halfwidth, stdev, max);
    *pkely = makeGaussianKernel(halfheight, 0, stdev, 1.0);
    return 0;
}


/*!
 *  makeDoGKernel()
 *
 *      Input:  halfheight, halfwidth (sx = 2 * halfwidth + 1, etc)
 *              stdev (standard deviation of narrower gaussian)
 *              ratio (of stdev for wide filter to stdev for narrow one)
 *      Return: kernel, or null on error
 *
 *  Notes:
 *      (1) The DoG (difference of gaussians) is a wavelet mother
 *          function with null total sum.  By subtracting two blurred
 *          versions of the image, it acts as a bandpass filter for
 *          frequencies passed by the narrow gaussian but stopped
 *          by the wide one.See:
 *               http://en.wikipedia.org/wiki/Difference_of_Gaussians
 *      (2) The kernel size (sx, sy) = (2 * halfwidth + 1, 2 * halfheight + 1).
 *      (3) The kernel center (cx, cy) = (halfwidth, halfheight).
 *      (4) The halfwidth and halfheight are typically equal, and
 *          are typically several times larger than the standard deviation.
 *      (5) The ratio is the ratio of standard deviations of the wide
 *          to narrow gaussian.  It must be >= 1.0; 1.0 is a no-op.
 *      (6) Because the kernel is a null sum, it must be invoked without
 *          normalization in pixConvolve().
 */
L_KERNEL *
makeDoGKernel(l_int32    halfheight,
              l_int32    halfwidth,
              l_float32  stdev,
              l_float32  ratio)
{
l_int32    sx, sy, i, j;
l_float32  pi, squaredist, highnorm, lownorm, val;
L_KERNEL  *kel;

    PROCNAME("makeDoGKernel");

    sx = 2 * halfwidth + 1;
    sy = 2 * halfheight + 1;
    if ((kel = kernelCreate(sy, sx)) == NULL)
        return (L_KERNEL *)ERROR_PTR("kel not made", procName, NULL);
    kernelSetOrigin(kel, halfheight, halfwidth);

    pi = 3.1415926535;
    for (i = 0; i < sy; i++) {
        for (j = 0; j < sx; j++) {
            squaredist = (l_float32)((i - halfheight) * (i - halfheight) +
                                     (j - halfwidth) * (j - halfwidth));
            highnorm = 1. / (2 * stdev * stdev);
            lownorm = highnorm / (ratio * ratio);
            val = (highnorm / pi) * expf(-(highnorm * squaredist))
                  - (lownorm / pi) * expf(-(lownorm * squaredist));
            kernelSetElement(kel, i, j, val);
        }
    }

    return kel;
}
