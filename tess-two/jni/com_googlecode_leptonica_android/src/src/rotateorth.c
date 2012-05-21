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
 *  rotateorth.c
 *
 *      Top-level rotation by multiples of 90 degrees
 *            PIX     *pixRotateOrth()
 *
 *      180-degree rotation
 *            PIX     *pixRotate180()
 *
 *      90-degree rotation (both directions)
 *            PIX     *pixRotate90()
 *
 *      Left-right flip
 *            PIX     *pixFlipLR()
 *
 *      Top-bottom flip
 *            PIX     *pixFlipTB()
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


/*!
 *  pixRotateOrth()
 *
 *      Input:  pixs (all depths)
 *              quads (0-3; number of 90 degree cw rotations)
 *      Return: pixd, or null on error
 */
PIX *
pixRotateOrth(PIX     *pixs,
              l_int32  quads)
{
    PROCNAME("pixRotateOrth");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (quads < 0 || quads > 4)
        return (PIX *)ERROR_PTR("quads not in {0,1,2,3,4}", procName, NULL);

    if (quads == 0 || quads == 4)
        return pixCopy(NULL, pixs);
    else if (quads == 1)
        return pixRotate90(pixs, 1);
    else if (quads == 2)
        return pixRotate180(NULL, pixs);
    else /* quads == 3 */
        return pixRotate90(pixs, -1);
}
    

/*!
 *  pixRotate180()
 *
 *      Input:  pixd  (<optional>; can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (all depths)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This does a 180 rotation of the image about the center,
 *          which is equivalent to a left-right flip about a vertical
 *          line through the image center, followed by a top-bottom
 *          flip about a horizontal line through the image center.
 *      (2) There are 3 cases for input:
 *          (a) pixd == null (creates a new pixd)
 *          (b) pixd == pixs (in-place operation)
 *          (c) pixd != pixs (existing pixd)
 *      (3) For clarity, use these three patterns, respectively:
 *          (a) pixd = pixRotate180(NULL, pixs);
 *          (b) pixRotate180(pixs, pixs);
 *          (c) pixRotate180(pixd, pixs);
 */
PIX *
pixRotate180(PIX  *pixd,
             PIX  *pixs)
{
l_int32  d;

    PROCNAME("pixRotate180");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("pixs not in {1,2,4,8,16,32} bpp",
                                procName, NULL);

        /* Prepare pixd for in-place operation */
    if ((pixd = pixCopy(pixd, pixs)) == NULL)
	return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

    pixFlipLR(pixd, pixd);
    pixFlipTB(pixd, pixd);
    return pixd;
}
    

/*!
 *  pixRotate90()
 *
 *      Input:  pixs (all depths)
 *              direction (1 = clockwise,  -1 = counter-clockwise)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This does a 90 degree rotation of the image about the center,
 *          either cw or ccw, returning a new pix.
 *      (2) The direction must be either 1 (cw) or -1 (ccw).
 */
PIX *
pixRotate90(PIX     *pixs,
            l_int32  direction)
{
l_int32    wd, hd, d, wpls, wpld;
l_uint32  *datas, *datad;
PIX       *pixd;

    PROCNAME("pixRotate90");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    d = pixGetDepth(pixs);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("pixs not in {1,2,4,8,16,32} bpp",
                                procName, NULL);
    if (direction != 1 && direction != -1)
        return (PIX *)ERROR_PTR("invalid direction", procName, NULL);

    hd = pixGetWidth(pixs);
    wd = pixGetHeight(pixs);
    if ((pixd = pixCreate(wd, hd, d)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    pixCopyColormap(pixd, pixs);
    pixCopyResolution(pixd, pixs);
    pixCopyInputFormat(pixd, pixs);

    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);

    rotate90Low(datad, wd, hd, d, wpld, datas, wpls, direction);

    return pixd;
}


/*!
 *  pixFlipLR()
 *
 *      Input:  pixd  (<optional>; can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (all depths)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This does a left-right flip of the image, which is
 *          equivalent to a rotation out of the plane about a
 *          vertical line through the image center.
 *      (2) There are 3 cases for input:
 *          (a) pixd == null (creates a new pixd)
 *          (b) pixd == pixs (in-place operation)
 *          (c) pixd != pixs (existing pixd)
 *      (3) For clarity, use these three patterns, respectively:
 *          (a) pixd = pixFlipLR(NULL, pixs);
 *          (b) pixFlipLR(pixs, pixs);
 *          (c) pixFlipLR(pixd, pixs);
 *      (4) If an existing pixd is not the same size as pixs, the
 *          image data will be reallocated.
 */
PIX *
pixFlipLR(PIX  *pixd,
          PIX  *pixs)
{
l_uint8   *tab;
l_int32    w, h, d, wpld;
l_uint32  *datad, *buffer;

    PROCNAME("pixFlipLR");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("pixs not in {1,2,4,8,16,32} bpp",
                                procName, NULL);

        /* Prepare pixd for in-place operation */
    if ((pixd = pixCopy(pixd, pixs)) == NULL)
	return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    switch (d)
    {
    case 1:
        tab = makeReverseByteTab1();
        break;
    case 2:
        tab = makeReverseByteTab2();
        break;
    case 4:
        tab = makeReverseByteTab4();
        break;
    default:
        tab = NULL;
        break;
    }

    if ((buffer = (l_uint32 *)CALLOC(wpld, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("buffer not made", procName, NULL);

    flipLRLow(datad, w, h, d, wpld, tab, buffer);

    FREE(buffer);
    if (tab) FREE(tab);
    return pixd;
}


/*!
 *  pixFlipTB()
 *
 *      Input:  pixd  (<optional>; can be null, equal to pixs,
 *                     or different from pixs)
 *              pixs (all depths)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) This does a top-bottom flip of the image, which is
 *          equivalent to a rotation out of the plane about a
 *          horizontal line through the image center.
 *      (2) There are 3 cases for input:
 *          (a) pixd == null (creates a new pixd)
 *          (b) pixd == pixs (in-place operation)
 *          (c) pixd != pixs (existing pixd)
 *      (3) For clarity, use these three patterns, respectively:
 *          (a) pixd = pixFlipTB(NULL, pixs);
 *          (b) pixFlipTB(pixs, pixs);
 *          (c) pixFlipTB(pixd, pixs);
 *      (4) If an existing pixd is not the same size as pixs, the
 *          image data will be reallocated.
 */
PIX *
pixFlipTB(PIX  *pixd,
          PIX  *pixs)
{
l_int32    h, d, wpld;
l_uint32  *datad, *buffer;

    PROCNAME("pixFlipTB");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    pixGetDimensions(pixs, NULL, &h, &d);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32)
        return (PIX *)ERROR_PTR("pixs not in {1,2,4,8,16,32} bpp",
                                procName, NULL);

        /* Prepare pixd for in-place operation */
    if ((pixd = pixCopy(pixd, pixs)) == NULL)
	return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    if ((buffer = (l_uint32 *)CALLOC(wpld, sizeof(l_uint32))) == NULL)
        return (PIX *)ERROR_PTR("buffer not made", procName, NULL);

    flipTBLow(datad, h, wpld, buffer);

    FREE(buffer);
    return pixd;
}

