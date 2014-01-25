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
 *  graymorphlow.c
 *
 *      Low-level grayscale morphological operations
 *
 *            void     dilateGrayLow()
 *            void     erodeGrayLow()
 *
 *
 *      We use the van Herk/Gil-Werman (vHGW) algorithm, [van Herk,
 *      Patt. Recog. Let. 13, pp. 517-521, 1992; Gil and Werman,
 *      IEEE Trans PAMI 15(5), pp. 504-507, 1993.]
 *      This was the first grayscale morphology
 *      algorithm to compute dilation and erosion with
 *      complexity independent of the size of the structuring
 *      element.  It is simple and elegant, and surprising that
 *      it was discovered as recently as 1992.  It works for
 *      SEs composed of horizontal and/or vertical lines.  The
 *      general case requires finding the Min or Max over an
 *      arbitrary set of pixels, and this requires a number of
 *      pixel comparisons equal to the SE "size" at each pixel
 *      in the image.  The vHGW algorithm requires not
 *      more than 3 comparisons at each point.  The algorithm has been
 *      recently refined by Gil and Kimmel ("Efficient Dilation
 *      Erosion, Opening and Closing Algorithms", in "Mathematical
 *      Morphology and its Applications to Image and Signal Processing",
 *      the proceedings of the International Symposium on Mathematical
 *      Morphology, Palo Alto, CA, June 2000, Kluwer Academic
 *      Publishers, pp. 301-310).  They bring this number down below
 *      1.5 comparisons per output pixel but at a cost of significantly
 *      increased complexity, so I don't bother with that here.
 *
 *      In brief, the method is as follows.  We evaluate the dilation
 *      in groups of "size" pixels, equal to the size of the SE.
 *      For horizontal, we start at x = "size"/2 and go
 *      (w - 2 * ("size"/2))/"size" steps.  This means that
 *      we don't evaluate the first 0.5 * "size" pixels and, worst
 *      case, the last 1.5 * "size" pixels.  Thus we embed the
 *      image in a larger image with these augmented dimensions, where
 *      the new border pixels are appropriately initialized (0 for
 *      dilation; 255 for erosion), and remove the boundary at the end.
 *      (For vertical, use h instead of w.)   Then for each group
 *      of "size" pixels, we form an array of length 2 * "size" + 1,
 *      consisting of backward and forward partial maxima (for
 *      dilation) or minima (for erosion).  This represents a
 *      jumping window computed from the source image, over which
 *      the SE will slide.  The center of the array gets the source
 *      pixel at the center of the SE.  Call this the center pixel
 *      of the window.  Array values to left of center get
 *      the maxima(minima) of the pixels from the center
 *      one and going to the left an equal distance.  Array
 *      values to the right of center get the maxima(minima) to
 *      the pixels from the center one and going to the right
 *      an equal distance.  These are computed sequentially starting
 *      from the center one.  The SE (of length "size") can slide over this
 *      window (of length 2 * "size + 1) at "size" different places.
 *      At each place, the maxima(minima) of the values in the window
 *      that correspond to the end points of the SE give the extremal
 *      values over that interval, and these are stored at the dest
 *      pixel corresponding to the SE center.  A picture is worth
 *      at least this many words, so if this isn't clear, see the
 *      leptonica documentation on grayscale morphology.
 *
 */

#include "allheaders.h"


/*-----------------------------------------------------------------*
 *              Low-level gray morphological operations            *
 *-----------------------------------------------------------------*/
/*!
 *  dilateGrayLow()
 *
 *    Input:  datad, w, h, wpld (8 bpp image)
 *            datas, wpls  (8 bpp image, of same dimensions)
 *            size  (full length of SEL; restricted to odd numbers)
 *            direction  (L_HORIZ or L_VERT)
 *            buffer  (holds full line or column of src image pixels)
 *            maxarray  (array of dimension 2*size+1)
 *    Return: void
 *
 *    Notes:
 *        (1) To eliminate border effects on the actual image, these images
 *            are prepared with an additional border of dimensions:
 *               leftpix = 0.5 * size
 *               rightpix = 1.5 * size
 *               toppix = 0.5 * size
 *               bottompix = 1.5 * size
 *            and we initialize the src border pixels to 0.
 *            This allows full processing over the actual image; at
 *            the end the border is removed.
 *        (2) Uses algorithm of van Herk, Gil and Werman
 */
void
dilateGrayLow(l_uint32  *datad,
              l_int32    w,
              l_int32    h,
              l_int32    wpld,
              l_uint32  *datas,
              l_int32    wpls,
              l_int32    size,
              l_int32    direction,
              l_uint8   *buffer,
              l_uint8   *maxarray)
{
l_int32    i, j, k;
l_int32    hsize, nsteps, startmax, startx, starty;
l_uint8    maxval;
l_uint32  *lines, *lined;

    if (direction == L_HORIZ) {
        hsize = size / 2;
        nsteps = (w - 2 * hsize) / size;
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;

                /* fill buffer with pixels in byte order */
            for (j = 0; j < w; j++)
                buffer[j] = GET_DATA_BYTE(lines, j);

            for (j = 0; j < nsteps; j++) {
                    /* refill the minarray */
                startmax = (j + 1) * size - 1;
                maxarray[size - 1] = buffer[startmax];
                for (k = 1; k < size; k++) {
                    maxarray[size - 1 - k] =
                        L_MAX(maxarray[size - k], buffer[startmax - k]);
                    maxarray[size - 1 + k] =
                        L_MAX(maxarray[size + k - 2], buffer[startmax + k]);
                }

                    /* compute dilation values */
                startx = hsize + j * size;
                SET_DATA_BYTE(lined, startx, maxarray[0]);
                SET_DATA_BYTE(lined, startx + size - 1, maxarray[2 * size - 2]);
                for (k = 1; k < size - 1; k++) {
                    maxval = L_MAX(maxarray[k], maxarray[k + size - 1]);
                    SET_DATA_BYTE(lined, startx + k, maxval);
                }
            }
        }
    } else {  /* direction == L_VERT */
        hsize = size / 2;
        nsteps = (h - 2 * hsize) / size;
        for (j = 0; j < w; j++) {
                /* fill buffer with pixels in byte order */
            for (i = 0; i < h; i++) {
                lines = datas + i * wpls;
                buffer[i] = GET_DATA_BYTE(lines, j);
            }

            for (i = 0; i < nsteps; i++) {
                    /* refill the minarray */
                startmax = (i + 1) * size - 1;
                maxarray[size - 1] = buffer[startmax];
                for (k = 1; k < size; k++) {
                    maxarray[size - 1 - k] =
                        L_MAX(maxarray[size - k], buffer[startmax - k]);
                    maxarray[size - 1 + k] =
                        L_MAX(maxarray[size + k - 2], buffer[startmax + k]);
                }

                    /* compute dilation values */
                starty = hsize + i * size;
                lined = datad + starty * wpld;
                SET_DATA_BYTE(lined, j, maxarray[0]);
                SET_DATA_BYTE(lined + (size - 1) * wpld, j,
                        maxarray[2 * size - 2]);
                for (k = 1; k < size - 1; k++) {
                    maxval = L_MAX(maxarray[k], maxarray[k + size - 1]);
                    SET_DATA_BYTE(lined + wpld * k, j, maxval);
                }
            }
        }
    }

    return;
}


/*!
 *  erodeGrayLow()
 *
 *    Input:  datad, w, h, wpld (8 bpp image)
 *            datas, wpls  (8 bpp image, of same dimensions)
 *            size  (full length of SEL; restricted to odd numbers)
 *            direction  (L_HORIZ or L_VERT)
 *            buffer  (holds full line or column of src image pixels)
 *            minarray  (array of dimension 2*size+1)
 *    Return: void
 *
 *    Notes:
 *        (1) See notes in dilateGrayLow()
 */
void
erodeGrayLow(l_uint32  *datad,
             l_int32    w,
             l_int32    h,
             l_int32    wpld,
             l_uint32  *datas,
             l_int32    wpls,
             l_int32    size,
             l_int32    direction,
             l_uint8   *buffer,
             l_uint8   *minarray)
{
l_int32    i, j, k;
l_int32    hsize, nsteps, startmin, startx, starty;
l_uint8    minval;
l_uint32  *lines, *lined;

    if (direction == L_HORIZ) {
        hsize = size / 2;
        nsteps = (w - 2 * hsize) / size;
        for (i = 0; i < h; i++) {
            lines = datas + i * wpls;
            lined = datad + i * wpld;

                /* fill buffer with pixels in byte order */
            for (j = 0; j < w; j++)
                buffer[j] = GET_DATA_BYTE(lines, j);

            for (j = 0; j < nsteps; j++) {
                    /* refill the minarray */
                startmin = (j + 1) * size - 1;
                minarray[size - 1] = buffer[startmin];
                for (k = 1; k < size; k++) {
                    minarray[size - 1 - k] =
                        L_MIN(minarray[size - k], buffer[startmin - k]);
                    minarray[size - 1 + k] =
                        L_MIN(minarray[size + k - 2], buffer[startmin + k]);
                }

                    /* compute erosion values */
                startx = hsize + j * size;
                SET_DATA_BYTE(lined, startx, minarray[0]);
                SET_DATA_BYTE(lined, startx + size - 1, minarray[2 * size - 2]);
                for (k = 1; k < size - 1; k++) {
                    minval = L_MIN(minarray[k], minarray[k + size - 1]);
                    SET_DATA_BYTE(lined, startx + k, minval);
                }
            }
        }
    } else {  /* direction == L_VERT */
        hsize = size / 2;
        nsteps = (h - 2 * hsize) / size;
        for (j = 0; j < w; j++) {
                /* fill buffer with pixels in byte order */
            for (i = 0; i < h; i++) {
                lines = datas + i * wpls;
                buffer[i] = GET_DATA_BYTE(lines, j);
            }

            for (i = 0; i < nsteps; i++) {
                    /* refill the minarray */
                startmin = (i + 1) * size - 1;
                minarray[size - 1] = buffer[startmin];
                for (k = 1; k < size; k++) {
                    minarray[size - 1 - k] =
                        L_MIN(minarray[size - k], buffer[startmin - k]);
                    minarray[size - 1 + k] =
                        L_MIN(minarray[size + k - 2], buffer[startmin + k]);
                }

                    /* compute erosion values */
                starty = hsize + i * size;
                lined = datad + starty * wpld;
                SET_DATA_BYTE(lined, j, minarray[0]);
                SET_DATA_BYTE(lined + (size - 1) * wpld, j,
                        minarray[2 * size - 2]);
                for (k = 1; k < size - 1; k++) {
                    minval = L_MIN(minarray[k], minarray[k + size - 1]);
                    SET_DATA_BYTE(lined + wpld * k, j, minval);
                }
            }
        }
    }

    return;
}
