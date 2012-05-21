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
 *  rop.c
 *
 *      General rasterop
 *           l_int32    pixRasterop()
 *
 *      In-place full band translation
 *           l_int32    pixRasteropVip()
 *           l_int32    pixRasteropHip()
 *
 *      Full image translation (general and in-place)
 *           l_int32    pixTranslate()
 *           l_int32    pixRasteropIP()
 *
 *      Full image rasterop with no translation
 *           l_int32    pixRasteropFullImage()
 */


#include <string.h>
#include "allheaders.h"

/*--------------------------------------------------------------------*
 *                General rasterop (basic pix interface)              *
 *--------------------------------------------------------------------*/
/*!
 *  pixRasterop()
 *
 *      Input:  pixd   (dest pix)
 *              dx     (x val of UL corner of dest rectangle)
 *              dy     (y val of UL corner of dest rectangle)
 *              dw     (width of dest rectangle)
 *              dh     (height of dest rectangle)
 *              op     (op code)
 *              pixs   (src pix)
 *              sx     (x val of UL corner of src rectangle)
 *              sy     (y val of UL corner of src rectangle)
 *      Return: 0 if OK; 1 on error.
 *
 *  Notes:
 *      (1) This has the standard set of 9 args for rasterop.
 *          This function is your friend; it is worth memorizing!
 *      (2) If the operation involves only dest, this calls
 *          rasteropUniLow().  Otherwise, checks depth of the
 *          src and dest, and if they match, calls rasteropLow().
 *      (3) For the two-image operation, where both pixs and pixd
 *          are defined, they are typically different images.  However
 *          there are cases, such as pixSetMirroredBorder(), where
 *          in-place operations can be done, blitting pixels from
 *          one part of pixd to another.  Consequently, we permit
 *          such operations.  If you use them, be sure that there
 *          is no overlap between the source and destination rectangles
 *          in pixd (!)
 *
 *  Background:
 *  -----------
 *
 *  There are 18 operations, described by the op codes in pix.h.
 *
 *  One, PIX_DST, is a no-op.
 *
 *  Three, PIX_CLR, PIX_SET, and PIX_NOT(PIX_DST) operate only on the dest.
 *  These are handled by the low-level rasteropUniLow().
 *
 *  The other 14 involve the both the src and the dest, and depend on
 *  the bit values of either just the src or the bit values of both
 *  src and dest.  They are handled by rasteropLow():
 *
 *          PIX_SRC                             s
 *          PIX_NOT(PIX_SRC)                   ~s
 *          PIX_SRC | PIX_DST                   s | d
 *          PIX_SRC & PIX_DST                   s & d
 *          PIX_SRC ^ PIX_DST                   s ^ d
 *          PIX_NOT(PIX_SRC) | PIX_DST         ~s | d
 *          PIX_NOT(PIX_SRC) & PIX_DST         ~s & d
 *          PIX_NOT(PIX_SRC) ^ PIX_DST         ~s ^ d
 *          PIX_SRC | PIX_NOT(PIX_DST)          s | ~d
 *          PIX_SRC & PIX_NOT(PIX_DST)          s & ~d
 *          PIX_SRC ^ PIX_NOT(PIX_DST)          s ^ ~d
 *          PIX_NOT(PIX_SRC | PIX_DST)         ~(s | d)
 *          PIX_NOT(PIX_SRC & PIX_DST)         ~(s & d)
 *          PIX_NOT(PIX_SRC ^ PIX_DST)         ~(s ^ d)
 *
 *  Each of these is implemented with one of three low-level
 *  functions, depending on the alignment of the left edge
 *  of the src and dest rectangles:
 *      * a fastest implementation if both left edges are
 *        (32-bit) word aligned
 *      * a very slightly slower implementation if both left
 *        edges have the same relative (32-bit) word alignment
 *      * the general routine that is invoked when
 *        both left edges have different word alignment
 *
 *  Of the 14 binary rasterops above, only 12 are unique
 *  logical combinations (out of a possible 16) of src
 *  and dst bits:
 *
 *        (sd)         (11)   (10)   (01)   (00)
 *   -----------------------------------------------
 *         s            1      1      0      0
 *        ~s            0      1      0      1
 *       s | d          1      1      1      0
 *       s & d          1      0      0      0
 *       s ^ d          0      1      1      0
 *      ~s | d          1      0      1      1
 *      ~s & d          0      0      1      0
 *      ~s ^ d          1      0      0      1
 *       s | ~d         1      1      0      1
 *       s & ~d         0      1      0      0
 *       s ^ ~d         1      0      0      1
 *      ~(s | d)        0      0      0      1
 *      ~(s & d)        0      1      1      1
 *      ~(s ^ d)        1      0      0      1
 *
 *  Note that the following three operations are equivalent:
 *      ~(s ^ d)
 *      ~s ^ d
 *      s ^ ~d
 *  and in the implementation, we call them out with the first form;
 *  namely, ~(s ^ d).
 *
 *  Of the 16 possible binary combinations of src and dest bits,
 *  the remaining 4 unique ones are independent of the src bit.
 *  They depend on either just the dest bit or on neither
 *  the src nor dest bits:
 *
 *         d            1      0      1      0    (indep. of s)
 *        ~d            0      1      0      1    (indep. of s)
 *        CLR           0      0      0      0    (indep. of both s & d)
 *        SET           1      1      1      1    (indep. of both s & d)
 *
 *  As mentioned above, three of these are implemented by
 *  rasteropUniLow(), and one is a no-op.
 *
 *  How can these operation codes be represented by bits
 *  in such a way that when the basic operations are performed
 *  on the bits the results are unique for unique
 *  operations, and mimic the logic table given above?
 *
 *  The answer is to choose a particular order of the pairings:
 *         (sd)         (11)   (10)   (01)   (00)
 *  (which happens to be the same as in the above table)
 *  and to translate the result into 4-bit representations
 *  of s and d.  For example, the Sun rasterop choice
 *  (omitting the extra bit for clipping) is
 *
 *      PIX_SRC      0xc
 *      PIX_DST      0xa
 *
 *  This corresponds to our pairing order given above:
 *         (sd)         (11)   (10)   (01)   (00)
 *  where for s = 1 we get the bit pattern
 *       PIX_SRC:        1      1      0      0     (0xc)
 *  and for d = 1 we get the pattern
 *       PIX_DST:         1      0      1      0    (0xa)
 *
 *  OK, that's the pairing order that Sun chose.  How many different
 *  ways can we assign bit patterns to PIX_SRC and PIX_DST to get
 *  the boolean ops to work out?  Any of the 4 pairs can be put
 *  in the first position, any of the remaining 3 pairs can go
 *  in the second; and one of the remaining 2 pairs can go the the third.
 *  There is a total of 4*3*2 = 24 ways these pairs can be permuted.
 */
l_int32
pixRasterop(PIX     *pixd,
            l_int32  dx,
            l_int32  dy,
            l_int32  dw,
            l_int32  dh,
            l_int32  op,
            PIX     *pixs,
            l_int32  sx,
            l_int32  sy)
{
l_int32  dd;

    PROCNAME("pixRasterop");

    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);

    if (op == PIX_DST)   /* no-op */
        return 0;

        /* Check if operation is only on dest */
    dd = pixGetDepth(pixd);
    if (op == PIX_CLR || op == PIX_SET || op == PIX_NOT(PIX_DST)) {
        rasteropUniLow(pixGetData(pixd),
                       pixGetWidth(pixd), pixGetHeight(pixd), dd,
                        pixGetWpl(pixd),
                       dx, dy, dw, dh,
                       op);
        return 0;
    }

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

        /* Check depth of src and dest; these must agree */
    if (dd != pixGetDepth(pixs))
        return ERROR_INT("depths of pixs and pixd differ", procName, 1);

    rasteropLow(pixGetData(pixd),
                pixGetWidth(pixd), pixGetHeight(pixd), dd,
                pixGetWpl(pixd),
                dx, dy, dw, dh,
                op,
                pixGetData(pixs),
                pixGetWidth(pixs), pixGetHeight(pixs),
                pixGetWpl(pixs),
                sx, sy);

    return 0;
}


/*--------------------------------------------------------------------*
 *                    In-place full band translation                  *
 *--------------------------------------------------------------------*/
/*!
 *  pixRasteropVip()
 *
 *      Input:  pixd (in-place)
 *              bx  (left edge of vertical band)
 *              bw  (width of vertical band)
 *              vshift (vertical shift of band; vshift > 0 is down)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This rasterop translates a vertical band of the
 *          image either up or down, bringing in either white
 *          or black pixels from outside the image.
 *      (2) The vertical band extends the full height of pixd.
 *      (3) If a colormap exists, the nearest color to white or black
 *          is brought in.
 */
l_int32
pixRasteropVip(PIX     *pixd,
               l_int32  bx,
               l_int32  bw,
               l_int32  vshift,
               l_int32  incolor)
{
l_int32   w, h, d, index, op;
PIX      *pixt;
PIXCMAP  *cmap;

    PROCNAME("pixRasteropVip");

    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return ERROR_INT("invalid value for incolor", procName, 1);
    if (bw <= 0)
        return ERROR_INT("width must be > 1", procName, 1);

    if (vshift == 0)
        return 0;

    pixGetDimensions(pixd, &w, &h, &d);
    rasteropVipLow(pixGetData(pixd), w, h, d, pixGetWpl(pixd), bx, bw, vshift);

    cmap = pixGetColormap(pixd);
    if (!cmap) {
        if ((d == 1 && incolor == L_BRING_IN_BLACK) ||
            (d > 1 && incolor == L_BRING_IN_WHITE))
            op = PIX_SET;
        else
            op = PIX_CLR;

            /* Set the pixels brought in at top or bottom */
        if (vshift > 0)
            pixRasterop(pixd, bx, 0, bw, vshift, op, NULL, 0, 0);
        else  /* vshift < 0 */
            pixRasterop(pixd, bx, h + vshift, bw, -vshift, op, NULL, 0, 0);
        return 0;
    }

        /* Get the nearest index and fill with that */
    if (incolor == L_BRING_IN_BLACK)
        pixcmapGetRankIntensity(cmap, 0.0, &index);
    else  /* white */
        pixcmapGetRankIntensity(cmap, 1.0, &index);
    pixt = pixCreate(bw, L_ABS(vshift), d);
    pixSetAllArbitrary(pixt, index);
    if (vshift > 0)
        pixRasterop(pixd, bx, 0, bw, vshift, PIX_SRC, pixt, 0, 0);
    else  /* vshift < 0 */
        pixRasterop(pixd, bx, h + vshift, bw, -vshift, PIX_SRC, pixt, 0, 0);
    pixDestroy(&pixt);
    return 0;
}


/*!
 *  pixRasteropHip()
 *
 *      Input:  pixd (in-place operation)
 *              by  (top of horizontal band)
 *              bh  (height of horizontal band)
 *              hshift (horizontal shift of band; hshift > 0 is to right)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) This rasterop translates a horizontal band of the
 *          image either left or right, bringing in either white
 *          or black pixels from outside the image.
 *      (2) The horizontal band extends the full width of pixd.
 *      (3) If a colormap exists, the nearest color to white or black
 *          is brought in.
 */
l_int32
pixRasteropHip(PIX     *pixd,
               l_int32  by,
               l_int32  bh,
               l_int32  hshift,
               l_int32  incolor)
{
l_int32   w, h, d, index, op;
PIX      *pixt;
PIXCMAP  *cmap;

    PROCNAME("pixRasteropHip");

    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (incolor != L_BRING_IN_WHITE && incolor != L_BRING_IN_BLACK)
        return ERROR_INT("invalid value for incolor", procName, 1);
    if (bh <= 0)
        return ERROR_INT("bh must be > 0", procName, 1);

    if (hshift == 0)
        return 0;

    pixGetDimensions(pixd, &w, &h, &d);
    rasteropHipLow(pixGetData(pixd), h, d, pixGetWpl(pixd), by, bh, hshift);

    cmap = pixGetColormap(pixd);
    if (!cmap) {
        if ((d == 1 && incolor == L_BRING_IN_BLACK) ||
            (d > 1 && incolor == L_BRING_IN_WHITE))
            op = PIX_SET;
        else
            op = PIX_CLR;

            /* Set the pixels brought in at left or right */
        if (hshift > 0)
            pixRasterop(pixd, 0, by, hshift, bh, op, NULL, 0, 0);
        else  /* hshift < 0 */
            pixRasterop(pixd, w + hshift, by, -hshift, bh, op, NULL, 0, 0);
        return 0;
    }

        /* Get the nearest index and fill with that */
    if (incolor == L_BRING_IN_BLACK)
        pixcmapGetRankIntensity(cmap, 0.0, &index);
    else  /* white */
        pixcmapGetRankIntensity(cmap, 1.0, &index);
    pixt = pixCreate(L_ABS(hshift), bh, d);
    pixSetAllArbitrary(pixt, index);
    if (hshift > 0)
        pixRasterop(pixd, 0, by, hshift, bh, PIX_SRC, pixt, 0, 0);
    else  /* hshift < 0 */
        pixRasterop(pixd, w + hshift, by, -hshift, bh, PIX_SRC, pixt, 0, 0);
    pixDestroy(&pixt);
    return 0;
}


/*--------------------------------------------------------------------*
 *             Full image translation (general and in-place)          *
 *--------------------------------------------------------------------*/
/*!
 *  pixTranslate()
 *
 *      Input:  pixd (<optional> destination: this can be null,
 *                    equal to pixs, or different from pixs)
 *              pixs
 *              hshift (horizontal shift; hshift > 0 is to right)
 *              vshift (vertical shift; vshift > 0 is down)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK)
 *      Return: pixd, or null on error.
 *
 *  Notes:
 *      (1) The general pattern is:
 *            pixd = pixTranslate(pixd, pixs, ...);
 *          For clarity, when you know the case, use one of these:
 *            pixd = pixTranslate(NULL, pixs, ...);  // new
 *            pixTranslate(pixs, pixs, ...);         // in-place
 *            pixTranslate(pixd, pixs, ...);         // to existing pixd
 *      (2) If an existing pixd is not the same size as pixs, the
 *          image data will be reallocated.
 */
PIX *
pixTranslate(PIX     *pixd,
             PIX     *pixs,
             l_int32  hshift,
             l_int32  vshift,
             l_int32  incolor)
{
    PROCNAME("pixTranslate");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

        /* Prepare pixd for in-place operation */
    if ((pixd = pixCopy(pixd, pixs)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

    pixRasteropIP(pixd, hshift, vshift, incolor);
    return pixd;
}


/*!
 *  pixRasteropIP()
 *
 *      Input:  pixd (in-place translation)
 *              hshift (horizontal shift; hshift > 0 is to right)
 *              vshift (vertical shift; vshift > 0 is down)
 *              incolor (L_BRING_IN_WHITE, L_BRING_IN_BLACK)
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixRasteropIP(PIX     *pixd,
              l_int32  hshift,
              l_int32  vshift,
              l_int32  incolor)
{
l_int32  w, h;

    PROCNAME("pixRasteropIP");

    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);

    pixGetDimensions(pixd, &w, &h, NULL);
    pixRasteropHip(pixd, 0, h, hshift, incolor);
    pixRasteropVip(pixd, 0, w, vshift, incolor);

    return 0;
}


/*--------------------------------------------------------------------*
 *                 Full image rasterop with no shifts                 *
 *--------------------------------------------------------------------*/
/*!
 *  pixRasteropFullImage()
 *
 *      Input:  pixd
 *              pixs
 *              op (any of the op-codes)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      - this is a wrapper for a common 2-image raster operation
 *      - both pixs and pixd must be defined
 *      - the operation is performed with aligned UL corners of pixs and pixd
 *      - the operation clips to the smallest pix; if the width or height
 *        of pixd is larger than pixs, some pixels in pixd will be unchanged
 */
l_int32
pixRasteropFullImage(PIX     *pixd,
                     PIX     *pixs,
                     l_int32  op)
{
    PROCNAME("pixRasteropFullImage");

    if (!pixd)
        return ERROR_INT("pixd not defined", procName, 1);
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

    pixRasterop(pixd, 0, 0, pixGetWidth(pixd), pixGetHeight(pixd), op,
                pixs, 0, 0);
    return 0;
}

