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

/*!
 * \file ropiplow.c
 * <pre>
 *
 *      Low level in-place full height vertical block transfer
 *
 *           void     rasteropVipLow()
 *
 *      Low level in-place full width horizontal block transfer
 *
 *           void     rasteropHipLow()
 *           void     shiftDataHorizontalLow()
 * </pre>
 */


#include <string.h>
#include "allheaders.h"

#define COMBINE_PARTIAL(d, s, m)     ( ((d) & ~(m)) | ((s) & (m)) )

static const l_uint32 lmask32[] = {0x0,
    0x80000000, 0xc0000000, 0xe0000000, 0xf0000000,
    0xf8000000, 0xfc000000, 0xfe000000, 0xff000000,
    0xff800000, 0xffc00000, 0xffe00000, 0xfff00000,
    0xfff80000, 0xfffc0000, 0xfffe0000, 0xffff0000,
    0xffff8000, 0xffffc000, 0xffffe000, 0xfffff000,
    0xfffff800, 0xfffffc00, 0xfffffe00, 0xffffff00,
    0xffffff80, 0xffffffc0, 0xffffffe0, 0xfffffff0,
    0xfffffff8, 0xfffffffc, 0xfffffffe, 0xffffffff};

static const l_uint32 rmask32[] = {0x0,
    0x00000001, 0x00000003, 0x00000007, 0x0000000f,
    0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
    0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
    0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
    0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
    0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
    0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff};


/*--------------------------------------------------------------------*
 *                 Low-level Vertical In-place Rasterop               *
 *--------------------------------------------------------------------*/
/*!
 * \brief   rasteropVipLow()
 *
 * \param[in]    data   ptr to image data
 * \param[in]    pixw   width
 * \param[in]    pixh   height
 * \param[in]    depth  depth
 * \param[in]    wpl    wpl
 * \param[in]    x      x val of UL corner of rectangle
 * \param[in]    w      width of rectangle
 * \param[in]    shift  + shifts data downward in vertical column
 * \return  0 if OK; 1 on error.
 *
 * <pre>
 * Notes:
 *      (1) This clears the pixels that are left exposed after the
 *          translation.  You can consider them as pixels that are
 *          shifted in from outside the image.  This can be later
 *          overridden by the incolor parameter in higher-level functions
 *          that call this.  For example, for images with depth \> 1,
 *          these pixels are cleared to black; to be white they
 *          must later be SET to white.  See, e.g., pixRasteropVip().
 *      (2) This function scales the width to accommodate any depth,
 *          performs clipping, and then does the in-place rasterop.
 * </pre>
 */
void
rasteropVipLow(l_uint32  *data,
               l_int32    pixw,
               l_int32    pixh,
               l_int32    depth,
               l_int32    wpl,
               l_int32    x,
               l_int32    w,
               l_int32    shift)
{
l_int32    fwpartb;    /* boolean (1, 0) if first word is partial */
l_int32    fwpart2b;   /* boolean (1, 0) if first word is doubly partial */
l_uint32   fwmask;     /* mask for first partial word */
l_int32    fwbits;     /* first word bits in ovrhang */
l_uint32  *pdfwpart;   /* ptr to first partial dest word */
l_uint32  *psfwpart;   /* ptr to first partial src word */
l_int32    fwfullb;    /* boolean (1, 0) if there exists a full word */
l_int32    nfullw;     /* number of full words */
l_uint32  *pdfwfull;   /* ptr to first full dest word */
l_uint32  *psfwfull;   /* ptr to first full src word */
l_int32    lwpartb;    /* boolean (1, 0) if last word is partial */
l_uint32   lwmask;     /* mask for last partial word */
l_int32    lwbits;     /* last word bits in ovrhang */
l_uint32  *pdlwpart;   /* ptr to last partial dest word */
l_uint32  *pslwpart;   /* ptr to last partial src word */
l_int32    dirwpl;     /* directed wpl (-wpl * sign(shift)) */
l_int32    absshift;   /* absolute value of shift; for use in iterator */
l_int32    vlimit;     /* vertical limit value for iterations */
l_int32    i, j;


   /*--------------------------------------------------------*
    *            Scale horizontal dimensions by depth        *
    *--------------------------------------------------------*/
    if (depth != 1) {
        pixw *= depth;
        x *= depth;
        w *= depth;
    }


   /*--------------------------------------------------------*
    *                   Clip horizontally                    *
    *--------------------------------------------------------*/
    if (x < 0) {
        w += x;    /* reduce w */
        x = 0;     /* clip to x = 0 */
    }
    if (x >= pixw || w <= 0)  /* no part of vertical slice is in the image */
        return;

    if (x + w > pixw)
        w = pixw - x;   /* clip to x + w = pixw */

    /*--------------------------------------------------------*
     *                Preliminary calculations                *
     *--------------------------------------------------------*/
        /* is the first word partial? */
    if ((x & 31) == 0) {  /* if not */
        fwpartb = 0;
        fwbits = 0;
    } else {  /* if so */
        fwpartb = 1;
        fwbits = 32 - (x & 31);
        fwmask = rmask32[fwbits];
        if (shift >= 0) { /* go up from bottom */
            pdfwpart = data + wpl * (pixh - 1) + (x >> 5);
            psfwpart = data + wpl * (pixh - 1 - shift) + (x >> 5);
        } else {  /* go down from top */
            pdfwpart = data + (x >> 5);
            psfwpart = data - wpl * shift + (x >> 5);
        }
    }

        /* is the first word doubly partial? */
    if (w >= fwbits) {  /* if not */
        fwpart2b = 0;
    } else {  /* if so */
        fwpart2b = 1;
        fwmask &= lmask32[32 - fwbits + w];
    }

        /* is there a full dest word? */
    if (fwpart2b == 1) {  /* not */
        fwfullb = 0;
        nfullw = 0;
    } else {
        nfullw = (w - fwbits) >> 5;
        if (nfullw == 0) {  /* if not */
            fwfullb = 0;
        } else {  /* if so */
            fwfullb = 1;
            if (fwpartb) {
                pdfwfull = pdfwpart + 1;
                psfwfull = psfwpart + 1;
            } else if (shift >= 0) { /* go up from bottom */
                pdfwfull = data + wpl * (pixh - 1) + (x >> 5);
                psfwfull = data + wpl * (pixh - 1 - shift) + (x >> 5);
            } else {  /* go down from top */
                pdfwfull = data + (x >> 5);
                psfwfull = data - wpl * shift + (x >> 5);
            }
        }
    }

        /* is the last word partial? */
    lwbits = (x + w) & 31;
    if (fwpart2b == 1 || lwbits == 0) {  /* if not */
        lwpartb = 0;
    } else {
        lwpartb = 1;
        lwmask = lmask32[lwbits];
        if (fwpartb) {
            pdlwpart = pdfwpart + 1 + nfullw;
            pslwpart = psfwpart + 1 + nfullw;
        } else if (shift >= 0) { /* go up from bottom */
            pdlwpart = data + wpl * (pixh - 1) + (x >> 5) + nfullw;
            pslwpart = data + wpl * (pixh - 1 - shift) + (x >> 5) + nfullw;
        } else {  /* go down from top */
            pdlwpart = data + (x >> 5) + nfullw;
            pslwpart = data - wpl * shift + (x >> 5) + nfullw;
        }
    }

        /* determine the direction of flow from the shift
         * If the shift >= 0, data flows downard from src
         * to dest, starting at the bottom and working up.
         * If shift < 0, data flows upward from src to
         * dest, starting at the top and working down. */
    dirwpl = (shift >= 0) ? -wpl : wpl;
    absshift = L_ABS(shift);
    vlimit = L_MAX(0, pixh - absshift);


/*--------------------------------------------------------*
 *            Now we're ready to do the ops               *
 *--------------------------------------------------------*/

        /* Do the first partial word */
    if (fwpartb) {
        for (i = 0; i < vlimit; i++) {
            *pdfwpart = COMBINE_PARTIAL(*pdfwpart, *psfwpart, fwmask);
            pdfwpart += dirwpl;
            psfwpart += dirwpl;
        }

            /* Clear the incoming pixels */
        for (i = vlimit; i < pixh; i++) {
            *pdfwpart = COMBINE_PARTIAL(*pdfwpart, 0x0, fwmask);
            pdfwpart += dirwpl;
        }
    }

        /* Do the full words */
    if (fwfullb) {
        for (i = 0; i < vlimit; i++) {
            for (j = 0; j < nfullw; j++)
                *(pdfwfull + j) = *(psfwfull + j);
            pdfwfull += dirwpl;
            psfwfull += dirwpl;
        }

            /* Clear the incoming pixels */
        for (i = vlimit; i < pixh; i++) {
            for (j = 0; j < nfullw; j++)
                *(pdfwfull + j) = 0x0;
            pdfwfull += dirwpl;
        }
    }

        /* Do the last partial word */
    if (lwpartb) {
        for (i = 0; i < vlimit; i++) {
            *pdlwpart = COMBINE_PARTIAL(*pdlwpart, *pslwpart, lwmask);
            pdlwpart += dirwpl;
            pslwpart += dirwpl;
        }

            /* Clear the incoming pixels */
        for (i = vlimit; i < pixh; i++) {
            *pdlwpart = COMBINE_PARTIAL(*pdlwpart, 0x0, lwmask);
            pdlwpart += dirwpl;
        }
    }

    return;
}



/*--------------------------------------------------------------------*
 *                 Low-level Horizontal In-place Rasterop             *
 *--------------------------------------------------------------------*/
/*!
 * \brief   rasteropHipLow()
 *
 * \param[in]    data   ptr to image data
 * \param[in]    pixh   height
 * \param[in]    depth  depth
 * \param[in]    wpl    wpl
 * \param[in]    y      y val of UL corner of rectangle
 * \param[in]    h      height of rectangle
 * \param[in]    shift  + shifts data to the left in a horizontal column
 * \return  0 if OK; 1 on error.
 *
 * <pre>
 * Notes:
 *      (1) This clears the pixels that are left exposed after the rasterop.
 *          Therefore, for Pix with depth \> 1, these pixels become black,
 *          and must be subsequently SET if they are to be white.
 *          For example, see pixRasteropHip().
 *      (2) This function performs clipping and calls shiftDataHorizontalLine()
 *          to do the in-place rasterop on each line.
 * </pre>
 */
void
rasteropHipLow(l_uint32  *data,
               l_int32    pixh,
               l_int32    depth,
               l_int32    wpl,
               l_int32    y,
               l_int32    h,
               l_int32    shift)
{
l_int32    i;
l_uint32  *line;

        /* clip band if necessary */
    if (y < 0) {
        h += y;  /* reduce h */
        y = 0;   /* clip to y = 0 */
    }
    if (h <= 0 || y > pixh)  /* no part of horizontal slice is in the image */
        return;

    if (y + h > pixh)
        h = pixh - y;   /* clip to y + h = pixh */

    for (i = y; i < y + h; i++) {
        line = data + i * wpl;
        shiftDataHorizontalLow(line, wpl, line, wpl, shift * depth);
    }
}


/*!
 * \brief   shiftDataHorizontalLow()
 *
 * \param[in]    datad  ptr to beginning of dest line
 * \param[in]    wpld   wpl of dest
 * \param[in]    datas  ptr to beginning of src line
 * \param[in]    wpls   wpl of src
 * \param[in]    shift  horizontal shift of block; >0 is to right
 * \return  void
 *
 * <pre>
 * Notes:
 *      (1) This can also be used for in-place operation; see, e.g.,
 *          rasteropHipLow().
 *      (2) We are clearing the pixels that are shifted in from
 *          outside the image.  This can be overridden by the
 *          incolor parameter in higher-level functions that call this.
 * </pre>
 */
void
shiftDataHorizontalLow(l_uint32  *datad,
                       l_int32    wpld,
                       l_uint32  *datas,
                       l_int32    wpls,
                       l_int32    shift)
{
l_int32    j, firstdw, wpl, rshift, lshift;
l_uint32  *lined, *lines;

    lined = datad;
    lines = datas;

    if (shift >= 0) {   /* src shift to right; data flows to
                         * right, starting at right edge and
                         * progressing leftward. */
        firstdw = shift / 32;
        wpl = L_MIN(wpls, wpld - firstdw);
        lined += firstdw + wpl - 1;
        lines += wpl - 1;
        rshift = shift & 31;
        if (rshift == 0) {
            for (j = 0; j < wpl; j++)
                *lined-- = *lines--;

                /* clear out the rest to the left edge */
            for (j = 0; j < firstdw; j++)
                *lined-- = 0;
        } else {
            lshift = 32 - rshift;
            for (j = 1; j < wpl; j++) {
                *lined-- = *(lines - 1) << lshift | *lines >> rshift;
                lines--;
            }
            *lined = *lines >> rshift;  /* partial first */

                /* clear out the rest to the left edge */
            *lined &= ~lmask32[rshift];
            lined--;
            for (j = 0; j < firstdw; j++)
                *lined-- = 0;
        }
    } else {  /* src shift to left; data flows to left, starting
             * at left edge and progressing rightward. */
        firstdw = (-shift) / 32;
        wpl = L_MIN(wpls - firstdw, wpld);
        lines += firstdw;
        lshift = (-shift) & 31;
        if (lshift == 0) {
            for (j = 0; j < wpl; j++)
                *lined++ = *lines++;

                /* clear out the rest to the right edge */
            for (j = 0; j < firstdw; j++)
                *lined++ = 0;
        } else {
            rshift = 32 - lshift;
            for (j = 1; j < wpl; j++) {
                *lined++ = *lines << lshift | *(lines + 1) >> rshift;
                lines++;
            }
            *lined = *lines << lshift;  /* partial last */

                /* clear out the rest to the right edge */
                /* first clear the lshift pixels of this partial word */
            *lined &= ~rmask32[lshift];
            lined++;
                /* then the remaining words to the right edge */
            for (j = 0; j < firstdw; j++)
                *lined++ = 0;
        }
    }

    return;
}
