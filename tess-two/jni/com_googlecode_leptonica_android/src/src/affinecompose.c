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
 *  affinecompose.c
 *
 *      Composable coordinate transforms
 *           l_float32   *createMatrix2dTranslate()
 *           l_float32   *createMatrix2dScale()
 *           l_float32   *createMatrix2dRotate()
 *
 *      Special coordinate transforms on pta
 *           PTA         *ptaTranslate()
 *           PTA         *ptaScale()
 *           PTA         *ptaRotate()
 *
 *      Special coordinate transforms on boxa
 *           BOXA        *boxaTranslate()
 *           BOXA        *boxaScale()
 *           BOXA        *boxaRotate()
 *
 *      General coordinate transform on pta and boxa
 *           PTA         *ptaAffineTransform()
 *           BOXA        *boxaAffineTransform()
 *
 *      Matrix operations
 *           l_int32      l_productMatVec()
 *           l_int32      l_productMat2()
 *           l_int32      l_productMat3()
 *           l_int32      l_productMat4()
 */

#include <math.h>
#include "allheaders.h"


/*-------------------------------------------------------------*
 *                Composable coordinate transforms             *
 *-------------------------------------------------------------*/
/*!
 *  createMatrix2dTranslate()
 *
 *      Input:  transx  (x component of translation wrt. the origin)
 *              transy  (y component of translation wrt. the origin)
 *      Return: 3x3 transform matrix, or null on error
 *
 *  Notes;
 *      (1) The translation is equivalent to:
 *             v' = Av
 *          where v and v' are 1x3 column vectors in the form
 *             v = [x, y, 1]^    (^ denotes transpose)
 *          and the affine tranlation matrix is
 *             A = [ 1   0   tx
 *                   0   1   ty
 *                   0   0    1  ]
 *
 *      (2) We consider translation as with respect to a fixed origin.
 *          In a clipping operation, the origin moves and the points
 *          are fixed, and you use (-tx, -ty) where (tx, ty) is the
 *          translation vector of the origin.
 */
l_float32 *
createMatrix2dTranslate(l_float32  transx,
                        l_float32  transy)
{
l_float32  *mat;

    PROCNAME("createMatrix2dTranslate");

    if ((mat = (l_float32 *)LEPT_CALLOC(9, sizeof(l_float32))) == NULL)
        return (l_float32 *)ERROR_PTR("mat not made", procName, NULL);

    mat[0] = mat[4] = mat[8] = 1;
    mat[2] = transx;
    mat[5] = transy;
    return mat;
}


/*!
 *  createMatrix2dScale()
 *
 *      Input:  scalex  (horizontal scale factor)
 *              scaley  (vertical scale factor)
 *      Return: 3x3 transform matrix, or null on error
 *
 *  Notes;
 *      (1) The scaling is equivalent to:
 *             v' = Av
 *          where v and v' are 1x3 column vectors in the form
 *             v = [x, y, 1]^    (^ denotes transpose)
 *          and the affine scaling matrix is
 *             A = [ sx  0    0
 *                   0   sy   0
 *                   0   0    1  ]
 *
 *      (2) We consider scaling as with respect to a fixed origin.
 *          In other words, the origin is the only point that doesn't
 *          move in the scaling transform.
 */
l_float32 *
createMatrix2dScale(l_float32  scalex,
                    l_float32  scaley)
{
l_float32  *mat;

    PROCNAME("createMatrix2dScale");

    if ((mat = (l_float32 *)LEPT_CALLOC(9, sizeof(l_float32))) == NULL)
        return (l_float32 *)ERROR_PTR("mat not made", procName, NULL);

    mat[0] = scalex;
    mat[4] = scaley;
    mat[8] = 1;
    return mat;
}


/*!
 *  createMatrix2dRotate()
 *
 *      Input:  xc, yc  (location of center of rotation)
 *              angle  (rotation in radians; clockwise is positive)
 *      Return: 3x3 transform matrix, or null on error
 *
 *  Notes;
 *      (1) The rotation is equivalent to:
 *             v' = Av
 *          where v and v' are 1x3 column vectors in the form
 *             v = [x, y, 1]^    (^ denotes transpose)
 *          and the affine rotation matrix is
 *             A = [ cosa   -sina    xc*(1-cosa) + yc*sina
 *                   sina    cosa    yc*(1-cosa) - xc*sina
 *                     0       0                 1         ]
 *
 *          If the rotation is about the origin, (xc, yc) = (0, 0) and
 *          this simplifies to
 *             A = [ cosa   -sina    0
 *                   sina    cosa    0
 *                     0       0     1 ]
 *
 *          These relations follow from the following equations, which
 *          you can convince yourself are correct as follows.  Draw a
 *          circle centered on (xc,yc) and passing through (x,y), with
 *          (x',y') on the arc at an angle 'a' clockwise from (x,y).
 *          [ Hint: cos(a + b) = cosa * cosb - sina * sinb
 *                  sin(a + b) = sina * cosb + cosa * sinb ]
 *
 *            x' - xc =  (x - xc) * cosa - (y - yc) * sina
 *            y' - yc =  (x - xc) * sina + (y - yc) * cosa
 */
l_float32 *
createMatrix2dRotate(l_float32  xc,
                     l_float32  yc,
                     l_float32  angle)
{
l_float32   sina, cosa;
l_float32  *mat;

    PROCNAME("createMatrix2dRotate");

    if ((mat = (l_float32 *)LEPT_CALLOC(9, sizeof(l_float32))) == NULL)
        return (l_float32 *)ERROR_PTR("mat not made", procName, NULL);

    sina = sin(angle);
    cosa = cos(angle);
    mat[0] = mat[4] = cosa;
    mat[1] = -sina;
    mat[2] = xc * (1.0 - cosa) + yc * sina;
    mat[3] = sina;
    mat[5] = yc * (1.0 - cosa) - xc * sina;
    mat[8] = 1;
    return mat;
}



/*-------------------------------------------------------------*
 *            Special coordinate transforms on pta             *
 *-------------------------------------------------------------*/
/*!
 *  ptaTranslate()
 *
 *      Input:  ptas (for initial points)
 *              transx  (x component of translation wrt. the origin)
 *              transy  (y component of translation wrt. the origin)
 *      Return: ptad  (translated points), or null on error
 *
 *  Notes;
 *      (1) See createMatrix2dTranslate() for details of transform.
 */
PTA *
ptaTranslate(PTA       *ptas,
             l_float32  transx,
             l_float32  transy)
{
l_int32    i, npts;
l_float32  x, y;
PTA       *ptad;

    PROCNAME("ptaTranslate");

    if (!ptas)
        return (PTA *)ERROR_PTR("ptas not defined", procName, NULL);

    npts = ptaGetCount(ptas);
    if ((ptad = ptaCreate(npts)) == NULL)
        return (PTA *)ERROR_PTR("ptad not made", procName, NULL);
    for (i = 0; i < npts; i++) {
        ptaGetPt(ptas, i, &x, &y);
        ptaAddPt(ptad, x + transx, y + transy);
    }

    return ptad;
}


/*!
 *  ptaScale()
 *
 *      Input:  ptas (for initial points)
 *              scalex  (horizontal scale factor)
 *              scaley  (vertical scale factor)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes;
 *      (1) See createMatrix2dScale() for details of transform.
 */
PTA *
ptaScale(PTA       *ptas,
         l_float32  scalex,
         l_float32  scaley)
{
l_int32    i, npts;
l_float32  x, y;
PTA       *ptad;

    PROCNAME("ptaScale");

    if (!ptas)
        return (PTA *)ERROR_PTR("ptas not defined", procName, NULL);

    npts = ptaGetCount(ptas);
    if ((ptad = ptaCreate(npts)) == NULL)
        return (PTA *)ERROR_PTR("ptad not made", procName, NULL);
    for (i = 0; i < npts; i++) {
        ptaGetPt(ptas, i, &x, &y);
        ptaAddPt(ptad, scalex * x, scaley * y);
    }

    return ptad;
}


/*!
 *  ptaRotate()
 *
 *      Input:  ptas (for initial points)
 *              (xc, yc)  (location of center of rotation)
 *              angle  (rotation in radians; clockwise is positive)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes;
 *      (1) See createMatrix2dScale() for details of transform.
 *      (2) This transform can be thought of as composed of the
 *          sum of two parts:
 *          (a) an (x,y)-dependent rotation about the origin:
 *              xr = x * cosa - y * sina
 *              yr = x * sina + y * cosa
 *          (b) an (x,y)-independent translation that depends on the
 *              rotation center and the angle:
 *              xt = xc - xc * cosa + yc * sina
 *              yt = yc - xc * sina - yc * cosa
 *          The translation part (xt,yt) is equal to the difference
 *          between the center (xc,yc) and the location of the
 *          center after it is rotated about the origin.
 */
PTA *
ptaRotate(PTA       *ptas,
          l_float32  xc,
          l_float32  yc,
          l_float32  angle)
{
l_int32    i, npts;
l_float32  x, y, xp, yp, sina, cosa;
PTA       *ptad;

    PROCNAME("ptaRotate");

    if (!ptas)
        return (PTA *)ERROR_PTR("ptas not defined", procName, NULL);

    npts = ptaGetCount(ptas);
    if ((ptad = ptaCreate(npts)) == NULL)
        return (PTA *)ERROR_PTR("ptad not made", procName, NULL);
    sina = sin(angle);
    cosa = cos(angle);
    for (i = 0; i < npts; i++) {
        ptaGetPt(ptas, i, &x, &y);
        xp = xc + (x - xc) * cosa - (y - yc) * sina;
        yp = yc + (x - xc) * sina + (y - yc) * cosa;
        ptaAddPt(ptad, xp, yp);
    }

    return ptad;
}


/*-------------------------------------------------------------*
 *            Special coordinate transforms on boxa            *
 *-------------------------------------------------------------*/
/*!
 *  boxaTranslate()
 *
 *      Input:  boxas
 *              transx  (x component of translation wrt. the origin)
 *              transy  (y component of translation wrt. the origin)
 *      Return: boxad  (translated boxas), or null on error
 *
 *  Notes;
 *      (1) See createMatrix2dTranslate() for details of transform.
 */
BOXA *
boxaTranslate(BOXA       *boxas,
              l_float32  transx,
              l_float32  transy)
{
PTA   *ptas, *ptad;
BOXA  *boxad;

    PROCNAME("boxaTranslate");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);

    ptas = boxaConvertToPta(boxas, 4);
    ptad = ptaTranslate(ptas, transx, transy);
    boxad = ptaConvertToBoxa(ptad, 4);
    ptaDestroy(&ptas);
    ptaDestroy(&ptad);
    return boxad;
}


/*!
 *  boxaScale()
 *
 *      Input:  boxas
 *              scalex  (horizontal scale factor)
 *              scaley  (vertical scale factor)
 *      Return: boxad  (scaled boxas), or null on error
 *
 *  Notes;
 *      (1) See createMatrix2dScale() for details of transform.
 */
BOXA *
boxaScale(BOXA      *boxas,
          l_float32  scalex,
          l_float32  scaley)
{
PTA   *ptas, *ptad;
BOXA  *boxad;

    PROCNAME("boxaScale");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);

    ptas = boxaConvertToPta(boxas, 4);
    ptad = ptaScale(ptas, scalex, scaley);
    boxad = ptaConvertToBoxa(ptad, 4);
    ptaDestroy(&ptas);
    ptaDestroy(&ptad);
    return boxad;
}


/*!
 *  boxaRotate()
 *
 *      Input:  boxas
 *              (xc, yc)  (location of center of rotation)
 *              angle  (rotation in radians; clockwise is positive)
 *      Return: boxad  (scaled boxas), or null on error
 *
 *  Notes;
 *      (1) See createMatrix2dRotate() for details of transform.
 */
BOXA *
boxaRotate(BOXA      *boxas,
           l_float32  xc,
           l_float32  yc,
           l_float32  angle)
{
PTA   *ptas, *ptad;
BOXA  *boxad;

    PROCNAME("boxaRotate");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);

    ptas = boxaConvertToPta(boxas, 4);
    ptad = ptaRotate(ptas, xc, yc, angle);
    boxad = ptaConvertToBoxa(ptad, 4);
    ptaDestroy(&ptas);
    ptaDestroy(&ptad);
    return boxad;
}


/*-------------------------------------------------------------*
 *            General affine coordinate transform              *
 *-------------------------------------------------------------*/
/*!
 *  ptaAffineTransform()
 *
 *      Input:  ptas (for initial points)
 *              mat  (3x3 transform matrix; canonical form)
 *      Return: ptad  (transformed points), or null on error
 */
PTA *
ptaAffineTransform(PTA        *ptas,
                   l_float32  *mat)
{
l_int32    i, npts;
l_float32  vecs[3], vecd[3];
PTA       *ptad;

    PROCNAME("ptaAffineTransform");

    if (!ptas)
        return (PTA *)ERROR_PTR("ptas not defined", procName, NULL);
    if (!mat)
        return (PTA *)ERROR_PTR("transform not defined", procName, NULL);

    vecs[2] = 1;
    npts = ptaGetCount(ptas);
    if ((ptad = ptaCreate(npts)) == NULL)
        return (PTA *)ERROR_PTR("ptad not made", procName, NULL);
    for (i = 0; i < npts; i++) {
        ptaGetPt(ptas, i, &vecs[0], &vecs[1]);
        l_productMatVec(mat, vecs, vecd, 3);
        ptaAddPt(ptad, vecd[0], vecd[1]);
    }

    return ptad;
}


/*!
 *  boxaAffineTransform()
 *
 *      Input:  boxas
 *              mat  (3x3 transform matrix; canonical form)
 *      Return: boxad  (transformed boxas), or null on error
 */
BOXA *
boxaAffineTransform(BOXA       *boxas,
                    l_float32  *mat)
{
PTA   *ptas, *ptad;
BOXA  *boxad;

    PROCNAME("boxaAffineTransform");

    if (!boxas)
        return (BOXA *)ERROR_PTR("boxas not defined", procName, NULL);
    if (!mat)
        return (BOXA *)ERROR_PTR("transform not defined", procName, NULL);

    ptas = boxaConvertToPta(boxas, 4);
    ptad = ptaAffineTransform(ptas, mat);
    boxad = ptaConvertToBoxa(ptad, 4);
    ptaDestroy(&ptas);
    ptaDestroy(&ptad);
    return boxad;
}


/*-------------------------------------------------------------*
 *                      Matrix operations                      *
 *-------------------------------------------------------------*/
/*!
 *  l_productMatVec()
 *
 *      Input:  mat  (square matrix, as a 1-dimensional @size^2 array)
 *              vecs (input column vector of length @size)
 *              vecd (result column vector)
 *              size (matrix is @size x @size; vectors are length @size)
 *      Return: 0 if OK, 1 on error
 */
l_int32
l_productMatVec(l_float32  *mat,
                l_float32  *vecs,
                l_float32  *vecd,
                l_int32     size)
{
l_int32  i, j;

    PROCNAME("l_productMatVec");

    if (!mat)
        return ERROR_INT("matrix not defined", procName, 1);
    if (!vecs)
        return ERROR_INT("input vector not defined", procName, 1);
    if (!vecd)
        return ERROR_INT("result vector not defined", procName, 1);

    for (i = 0; i < size; i++) {
        vecd[i] = 0;
        for (j = 0; j < size; j++) {
            vecd[i] += mat[size * i + j] * vecs[j];
        }
    }
    return 0;
}


/*!
 *  l_productMat2()
 *
 *      Input:  mat1  (square matrix, as a 1-dimensional size^2 array)
 *              mat2  (square matrix, as a 1-dimensional size^2 array)
 *              matd  (square matrix; product stored here)
 *              size (of matrices)
 *      Return: 0 if OK, 1 on error
 */
l_int32
l_productMat2(l_float32  *mat1,
              l_float32  *mat2,
              l_float32  *matd,
              l_int32     size)
{
l_int32  i, j, k, index;

    PROCNAME("l_productMat2");

    if (!mat1)
        return ERROR_INT("matrix 1 not defined", procName, 1);
    if (!mat2)
        return ERROR_INT("matrix 2 not defined", procName, 1);
    if (!matd)
        return ERROR_INT("result matrix not defined", procName, 1);

    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            index = size * i + j;
            matd[index] = 0;
            for (k = 0; k < size; k++)
                 matd[index] += mat1[size * i + k] * mat2[size * k + j];
        }
    }
    return 0;
}


/*!
 *  l_productMat3()
 *
 *      Input:  mat1  (square matrix, as a 1-dimensional size^2 array)
 *              mat2  (square matrix, as a 1-dimensional size^2 array)
 *              mat3  (square matrix, as a 1-dimensional size^2 array)
 *              matd  (square matrix; product stored here)
 *              size  (of matrices)
 *      Return: 0 if OK, 1 on error
 */
l_int32
l_productMat3(l_float32  *mat1,
              l_float32  *mat2,
              l_float32  *mat3,
              l_float32  *matd,
              l_int32     size)
{
l_float32  *matt;

    PROCNAME("l_productMat3");

    if (!mat1)
        return ERROR_INT("matrix 1 not defined", procName, 1);
    if (!mat2)
        return ERROR_INT("matrix 2 not defined", procName, 1);
    if (!mat3)
        return ERROR_INT("matrix 3 not defined", procName, 1);
    if (!matd)
        return ERROR_INT("result matrix not defined", procName, 1);

    if ((matt = (l_float32 *)LEPT_CALLOC(size * size, sizeof(l_float32))) == NULL)
        return ERROR_INT("matt not made", procName, 1);
    l_productMat2(mat1, mat2, matt, size);
    l_productMat2(matt, mat3, matd, size);
    LEPT_FREE(matt);
    return 0;
}


/*!
 *  l_productMat4()
 *
 *      Input:  mat1  (square matrix, as a 1-dimensional size^2 array)
 *              mat2  (square matrix, as a 1-dimensional size^2 array)
 *              mat3  (square matrix, as a 1-dimensional size^2 array)
 *              mat4  (square matrix, as a 1-dimensional size^2 array)
 *              matd  (square matrix; product stored here)
 *              size  (of matrices)
 *      Return: 0 if OK, 1 on error
 */
l_int32
l_productMat4(l_float32  *mat1,
              l_float32  *mat2,
              l_float32  *mat3,
              l_float32  *mat4,
              l_float32  *matd,
              l_int32     size)
{
l_float32  *matt;

    PROCNAME("l_productMat4");

    if (!mat1)
        return ERROR_INT("matrix 1 not defined", procName, 1);
    if (!mat2)
        return ERROR_INT("matrix 2 not defined", procName, 1);
    if (!mat3)
        return ERROR_INT("matrix 3 not defined", procName, 1);
    if (!matd)
        return ERROR_INT("result matrix not defined", procName, 1);

    if ((matt = (l_float32 *)LEPT_CALLOC(size * size, sizeof(l_float32))) == NULL)
        return ERROR_INT("matt not made", procName, 1);
    l_productMat3(mat1, mat2, mat3, matt, size);
    l_productMat2(matt, mat4, matd, size);
    LEPT_FREE(matt);
    return 0;
}
