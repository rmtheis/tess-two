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
 *  graphics.c
 *
 *      Pta generation for arbitrary shapes built with lines
 *          PTA        *generatePtaLine()
 *          PTA        *generatePtaWideLine()
 *          PTA        *generatePtaBox()
 *          PTA        *generatePtaBoxa()
 *          PTA        *generatePtaHashBox()
 *          PTA        *generatePtaHashBoxa()
 *          PTAA       *generatePtaaBoxa()
 *          PTAA       *generatePtaaHashBoxa()
 *          PTA        *generatePtaPolyline()
 *          PTA        *convertPtaLineTo4cc()
 *          PTA        *generatePtaFilledCircle()
 *          PTA        *generatePtaFilledSquare()
 *          PTA        *generatePtaLineFromPt()
 *          l_int32     locatePtRadially()
 *
 *      Rendering function plots directly on images
 *          l_int32     pixRenderPlotFromNuma()
 *          l_int32     pixRenderPlotFromNumaGen()
 *          PTA        *makePlotPtaFromNuma()
 *          PTA        *makePlotPtaFromNumaGen()
 *
 *      Pta rendering
 *          l_int32     pixRenderPta()
 *          l_int32     pixRenderPtaArb()
 *          l_int32     pixRenderPtaBlend()
 *
 *      Rendering of arbitrary shapes built with lines
 *          l_int32     pixRenderLine()
 *          l_int32     pixRenderLineArb()
 *          l_int32     pixRenderLineBlend()
 *
 *          l_int32     pixRenderBox()
 *          l_int32     pixRenderBoxArb()
 *          l_int32     pixRenderBoxBlend()
 *
 *          l_int32     pixRenderBoxa()
 *          l_int32     pixRenderBoxaArb()
 *          l_int32     pixRenderBoxaBlend()
 *
 *          l_int32     pixRenderHashBox()
 *          l_int32     pixRenderHashBoxArb()
 *          l_int32     pixRenderHashBoxBlend()
 *
 *          l_int32     pixRenderHashBoxa()
 *          l_int32     pixRenderHashBoxaArb()
 *          l_int32     pixRenderHashBoxaBlend()
 *
 *          l_int32     pixRenderPolyline()
 *          l_int32     pixRenderPolylineArb()
 *          l_int32     pixRenderPolylineBlend()
 *
 *          l_int32     pixRenderRandomCmapPtaa()
 *
 *      Rendering and filling of polygons
 *          PIX        *pixRenderPolygon()
 *          PIX        *pixFillPolygon()
 *
 *      Contour rendering on grayscale images
 *          PIX        *pixRenderContours()
 *          PIX        *fpixAutoRenderContours()
 *          PIX        *fpixRenderContours()
 *
 *  The line rendering functions are relatively crude, but they
 *  get the job done for most simple situations.  We use the pta
 *  (array of points) as an intermediate data structure.  For example,
 *  to render a line we first generate a pta.
 *
 *  Some rendering functions come in sets of three.  For example
 *       pixRenderLine() -- render on 1 bpp pix
 *       pixRenderLineArb() -- render on 32 bpp pix with arbitrary (r,g,b)
 *       pixRenderLineBlend() -- render on 32 bpp pix, blending the
 *               (r,g,b) graphic object with the underlying rgb pixels.
 *
 *  There are also procedures for plotting a function, computed
 *  from the row or column pixels, directly on the image.
 */

#include <string.h>
#include <math.h>
#include "allheaders.h"


/*------------------------------------------------------------------*
 *        Pta generation for arbitrary shapes built with lines      *
 *------------------------------------------------------------------*/
/*!
 *  generatePtaLine()
 *
 *      Input:  x1, y1  (end point 1)
 *              x2, y2  (end point 2)
 *      Return: pta, or null on error
 *
 *  Notes:
 *      (1) Uses Bresenham line drawing, which results in an 8-connected line.
 */
PTA  *
generatePtaLine(l_int32  x1,
                l_int32  y1,
                l_int32  x2,
                l_int32  y2)
{
l_int32    npts, diff, getyofx, sign, i, x, y;
l_float32  slope;
PTA       *pta;

    PROCNAME("generatePtaLine");

        /* Generate line parameters */
    if (x1 == x2 && y1 == y2) {  /* same point */
        npts = 1;
    } else if (L_ABS(x2 - x1) >= L_ABS(y2 - y1)) {
        getyofx = TRUE;
        npts = L_ABS(x2 - x1) + 1;
        diff = x2 - x1;
        sign = L_SIGN(x2 - x1);
        slope = (l_float32)(sign * (y2 - y1)) / (l_float32)diff;
    } else {
        getyofx = FALSE;
        npts = L_ABS(y2 - y1) + 1;
        diff = y2 - y1;
        sign = L_SIGN(y2 - y1);
        slope = (l_float32)(sign * (x2 - x1)) / (l_float32)diff;
    }

    if ((pta = ptaCreate(npts)) == NULL)
        return (PTA *)ERROR_PTR("pta not made", procName, NULL);

    if (npts == 1) {  /* degenerate case */
        ptaAddPt(pta, x1, y1);
        return pta;
    }

        /* Generate the set of points */
    if (getyofx) {  /* y = y(x) */
        for (i = 0; i < npts; i++) {
            x = x1 + sign * i;
            y = (l_int32)(y1 + (l_float32)i * slope + 0.5);
            ptaAddPt(pta, x, y);
        }
    } else {   /* x = x(y) */
        for (i = 0; i < npts; i++) {
            x = (l_int32)(x1 + (l_float32)i * slope + 0.5);
            y = y1 + sign * i;
            ptaAddPt(pta, x, y);
        }
    }

    return pta;
}


/*!
 *  generatePtaWideLine()
 *
 *      Input:  x1, y1  (end point 1)
 *              x2, y2  (end point 2)
 *              width
 *      Return: ptaj, or null on error
 */
PTA  *
generatePtaWideLine(l_int32  x1,
                    l_int32  y1,
                    l_int32  x2,
                    l_int32  y2,
                    l_int32  width)
{
l_int32  i, x1a, x2a, y1a, y2a;
PTA     *pta, *ptaj;

    PROCNAME("generatePtaWideLine");

    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }

    if ((ptaj = generatePtaLine(x1, y1, x2, y2)) == NULL)
        return (PTA *)ERROR_PTR("ptaj not made", procName, NULL);
    if (width == 1)
        return ptaj;

        /* width > 1; estimate line direction & join */
    if (L_ABS(x1 - x2) > L_ABS(y1 - y2)) {  /* "horizontal" line  */
        for (i = 1; i < width; i++) {
            if ((i & 1) == 1) {   /* place above */
                y1a = y1 - (i + 1) / 2;
                y2a = y2 - (i + 1) / 2;
            } else {  /* place below */
                y1a = y1 + (i + 1) / 2;
                y2a = y2 + (i + 1) / 2;
            }
            if ((pta = generatePtaLine(x1, y1a, x2, y2a)) == NULL)
                return (PTA *)ERROR_PTR("pta not made", procName, NULL);
            ptaJoin(ptaj, pta, 0, -1);
            ptaDestroy(&pta);
        }
    } else  {  /* "vertical" line  */
        for (i = 1; i < width; i++) {
            if ((i & 1) == 1) {   /* place to left */
                x1a = x1 - (i + 1) / 2;
                x2a = x2 - (i + 1) / 2;
            } else {  /* place to right */
                x1a = x1 + (i + 1) / 2;
                x2a = x2 + (i + 1) / 2;
            }
            if ((pta = generatePtaLine(x1a, y1, x2a, y2)) == NULL)
                return (PTA *)ERROR_PTR("pta not made", procName, NULL);
            ptaJoin(ptaj, pta, 0, -1);
            ptaDestroy(&pta);
        }
    }

    return ptaj;
}


/*!
 *  generatePtaBox()
 *
 *      Input:  box
 *              width (of line)
 *      Return: ptad, or null on error
 *
 *  Notes:
 *      (1) Because the box is constructed so that we don't have any
 *          overlapping lines, there is no need to remove duplicates.
 */
PTA  *
generatePtaBox(BOX     *box,
               l_int32  width)
{
l_int32  x, y, w, h;
PTA     *ptad, *pta;

    PROCNAME("generatePtaBox");

    if (!box)
        return (PTA *)ERROR_PTR("box not defined", procName, NULL);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }

        /* Generate line points and add them to the pta. */
    boxGetGeometry(box, &x, &y, &w, &h);
    if (w == 0 || h == 0)
        return (PTA *)ERROR_PTR("box has w = 0 or h = 0", procName, NULL);
    ptad = ptaCreate(0);
    if ((width & 1) == 1) {   /* odd width */
        pta = generatePtaWideLine(x - width / 2, y,
                                  x + w - 1 + width / 2, y, width);
        ptaJoin(ptad, pta, 0, -1);
        ptaDestroy(&pta);
        pta = generatePtaWideLine(x + w - 1, y + 1 + width / 2,
                                  x + w - 1, y + h - 2 - width / 2, width);
        ptaJoin(ptad, pta, 0, -1);
        ptaDestroy(&pta);
        pta = generatePtaWideLine(x + w - 1 + width / 2, y + h - 1,
                                  x - width / 2, y + h - 1, width);
        ptaJoin(ptad, pta, 0, -1);
        ptaDestroy(&pta);
        pta = generatePtaWideLine(x, y + h - 2 - width / 2,
                                  x, y + 1 + width / 2, width);
        ptaJoin(ptad, pta, 0, -1);
        ptaDestroy(&pta);
    } else {   /* even width */
        pta = generatePtaWideLine(x - width / 2, y,
                                  x + w - 2 + width / 2, y, width);
        ptaJoin(ptad, pta, 0, -1);
        ptaDestroy(&pta);
        pta = generatePtaWideLine(x + w - 1, y + 0 + width / 2,
                                  x + w - 1, y + h - 2 - width / 2, width);
        ptaJoin(ptad, pta, 0, -1);
        ptaDestroy(&pta);
        pta = generatePtaWideLine(x + w - 2 + width / 2, y + h - 1,
                                  x - width / 2, y + h - 1, width);
        ptaJoin(ptad, pta, 0, -1);
        ptaDestroy(&pta);
        pta = generatePtaWideLine(x, y + h - 2 - width / 2,
                                  x, y + 0 + width / 2, width);
        ptaJoin(ptad, pta, 0, -1);
        ptaDestroy(&pta);
    }

    return ptad;
}


/*!
 *  generatePtaBoxa()
 *
 *      Input:  boxa
 *              width
 *              removedups  (1 to remove, 0 to leave)
 *      Return: ptad, or null on error
 *
 *  Notes:
 *      (1) If the boxa has overlapping boxes, and if blending will
 *          be used to give a transparent effect, transparency
 *          artifacts at line intersections can be removed using
 *          removedups = 1.
 */
PTA  *
generatePtaBoxa(BOXA    *boxa,
                l_int32  width,
                l_int32  removedups)
{
l_int32  i, n;
BOX     *box;
PTA     *ptad, *ptat, *pta;

    PROCNAME("generatePtaBoxa");

    if (!boxa)
        return (PTA *)ERROR_PTR("boxa not defined", procName, NULL);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }

    n = boxaGetCount(boxa);
    ptat = ptaCreate(0);
    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        pta = generatePtaBox(box, width);
        ptaJoin(ptat, pta, 0, -1);
        ptaDestroy(&pta);
        boxDestroy(&box);
    }

    if (removedups)
        ptad = ptaRemoveDuplicates(ptat, 0);
    else
        ptad = ptaClone(ptat);

    ptaDestroy(&ptat);
    return ptad;
}


/*!
 *  generatePtaHashBox()
 *
 *      Input:  box
 *              spacing (spacing between lines; must be > 1)
 *              width  (of line)
 *              orient  (orientation of lines: L_HORIZONTAL_LINE, ...)
 *              outline  (0 to skip drawing box outline)
 *      Return: ptad, or null on error
 *
 *  Notes:
 *      (1) The orientation takes on one of 4 orientations (horiz, vertical,
 *          slope +1, slope -1).
 *      (2) The full outline is also drawn if @outline = 1.
 */
PTA  *
generatePtaHashBox(BOX     *box,
                   l_int32  spacing,
                   l_int32  width,
                   l_int32  orient,
                   l_int32  outline)
{
l_int32  bx, by, bh, bw, x, y, x1, y1, x2, y2, i, n, npts;
PTA     *ptad, *pta;

    PROCNAME("generatePtaHashBox");

    if (!box)
        return (PTA *)ERROR_PTR("box not defined", procName, NULL);
    if (spacing <= 1)
        return (PTA *)ERROR_PTR("spacing not > 1", procName, NULL);
    if (orient != L_HORIZONTAL_LINE && orient != L_POS_SLOPE_LINE &&
        orient != L_VERTICAL_LINE && orient != L_NEG_SLOPE_LINE)
        return (PTA *)ERROR_PTR("invalid line orientation", procName, NULL);
    boxGetGeometry(box, &bx, &by, &bw, &bh);
    if (bw == 0 || bh == 0)
        return (PTA *)ERROR_PTR("box has bw = 0 or bh = 0", procName, NULL);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }

        /* Generate line points and add them to the pta. */
    ptad = ptaCreate(0);
    if (outline) {
        pta = generatePtaBox(box, width);
        ptaJoin(ptad, pta, 0, -1);
        ptaDestroy(&pta);
    }
    if (orient == L_HORIZONTAL_LINE) {
        n = 1 + bh / spacing;
        for (i = 0; i < n; i++) {
            y = by + (i * (bh - 1)) / (n - 1);
            pta = generatePtaWideLine(bx, y, bx + bw - 1, y, width);
            ptaJoin(ptad, pta, 0, -1);
            ptaDestroy(&pta);
        }
    } else if (orient == L_VERTICAL_LINE) {
        n = 1 + bw / spacing;
        for (i = 0; i < n; i++) {
            x = bx + (i * (bw - 1)) / (n - 1);
            pta = generatePtaWideLine(x, by, x, by + bh - 1, width);
            ptaJoin(ptad, pta, 0, -1);
            ptaDestroy(&pta);
        }
    } else if (orient == L_POS_SLOPE_LINE) {
        n = 2 + (l_int32)((bw + bh) / (1.4 * spacing));
        for (i = 0; i < n; i++) {
            x = (l_int32)(bx + (i + 0.5) * 1.4 * spacing);
            boxIntersectByLine(box, x, by - 1, 1.0, &x1, &y1, &x2, &y2, &npts);
            if (npts == 2) {
                pta = generatePtaWideLine(x1, y1, x2, y2, width);
                ptaJoin(ptad, pta, 0, -1);
                ptaDestroy(&pta);
            }
        }
    } else {  /* orient == L_NEG_SLOPE_LINE */
        n = 2 + (l_int32)((bw + bh) / (1.4 * spacing));
        for (i = 0; i < n; i++) {
            x = (l_int32)(bx - bh + (i + 0.5) * 1.4 * spacing);
            boxIntersectByLine(box, x, by - 1, -1.0, &x1, &y1, &x2, &y2, &npts);
            if (npts == 2) {
                pta = generatePtaWideLine(x1, y1, x2, y2, width);
                ptaJoin(ptad, pta, 0, -1);
                ptaDestroy(&pta);
            }
        }
    }

    return ptad;
}


/*!
 *  generatePtaHashBoxa()
 *
 *      Input:  boxa
 *              spacing (spacing between lines; must be > 1)
 *              width  (of line)
 *              orient  (orientation of lines: L_HORIZONTAL_LINE, ...)
 *              outline  (0 to skip drawing box outline)
 *              removedups  (1 to remove, 0 to leave)
 *      Return: ptad, or null on error
 *
 *  Notes:
 *      (1) The orientation takes on one of 4 orientations (horiz, vertical,
 *          slope +1, slope -1).
 *      (2) The full outline is also drawn if @outline = 1.
 *      (3) If the boxa has overlapping boxes, and if blending will
 *          be used to give a transparent effect, transparency
 *          artifacts at line intersections can be removed using
 *          removedups = 1.
 */
PTA  *
generatePtaHashBoxa(BOXA    *boxa,
                    l_int32  spacing,
                    l_int32  width,
                    l_int32  orient,
                    l_int32  outline,
                    l_int32  removedups)
{
l_int32  i, n;
BOX     *box;
PTA     *ptad, *ptat, *pta;

    PROCNAME("generatePtaHashBoxa");

    if (!boxa)
        return (PTA *)ERROR_PTR("boxa not defined", procName, NULL);
    if (spacing <= 1)
        return (PTA *)ERROR_PTR("spacing not > 1", procName, NULL);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }
    if (orient != L_HORIZONTAL_LINE && orient != L_POS_SLOPE_LINE &&
        orient != L_VERTICAL_LINE && orient != L_NEG_SLOPE_LINE)
        return (PTA *)ERROR_PTR("invalid line orientation", procName, NULL);

    n = boxaGetCount(boxa);
    ptat = ptaCreate(0);
    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        pta = generatePtaHashBox(box, spacing, width, orient, outline);
        ptaJoin(ptat, pta, 0, -1);
        ptaDestroy(&pta);
        boxDestroy(&box);
    }

    if (removedups)
        ptad = ptaRemoveDuplicates(ptat, 0);
    else
        ptad = ptaClone(ptat);

    ptaDestroy(&ptat);
    return ptad;
}


/*!
 *  generatePtaaBoxa()
 *
 *      Input:  boxa
 *      Return: ptaa, or null on error
 *
 *  Notes:
 *      (1) This generates a pta of the four corners for each box in
 *          the boxa.
 *      (2) Each of these pta can be rendered onto a pix with random colors,
 *          by using pixRenderRandomCmapPtaa() with closeflag = 1.
 */
PTAA  *
generatePtaaBoxa(BOXA  *boxa)
{
l_int32  i, n, x, y, w, h;
BOX     *box;
PTA     *pta;
PTAA    *ptaa;

    PROCNAME("generatePtaaBoxa");

    if (!boxa)
        return (PTAA *)ERROR_PTR("boxa not defined", procName, NULL);

    n = boxaGetCount(boxa);
    ptaa = ptaaCreate(n);
    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        boxGetGeometry(box, &x, &y, &w, &h);
        pta = ptaCreate(4);
        ptaAddPt(pta, x, y);
        ptaAddPt(pta, x + w - 1, y);
        ptaAddPt(pta, x + w - 1, y + h - 1);
        ptaAddPt(pta, x, y + h - 1);
        ptaaAddPta(ptaa, pta, L_INSERT);
        boxDestroy(&box);
    }

    return ptaa;
}


/*!
 *  generatePtaaHashBoxa()
 *
 *      Input:  boxa
 *              spacing (spacing between hash lines; must be > 1)
 *              width  (hash line width)
 *              orient  (orientation of lines: L_HORIZONTAL_LINE, ...)
 *              outline  (0 to skip drawing box outline)
 *      Return: ptaa, or null on error
 *
 *  Notes:
 *      (1) The orientation takes on one of 4 orientations (horiz, vertical,
 *          slope +1, slope -1).
 *      (2) The full outline is also drawn if @outline = 1.
 *      (3) Each of these pta can be rendered onto a pix with random colors,
 *          by using pixRenderRandomCmapPtaa() with closeflag = 1.
 *
 */
PTAA *
generatePtaaHashBoxa(BOXA    *boxa,
                     l_int32  spacing,
                     l_int32  width,
                     l_int32  orient,
                     l_int32  outline)
{
l_int32  i, n;
BOX     *box;
PTA     *pta;
PTAA    *ptaa;

    PROCNAME("generatePtaaHashBoxa");

    if (!boxa)
        return (PTAA *)ERROR_PTR("boxa not defined", procName, NULL);
    if (spacing <= 1)
        return (PTAA *)ERROR_PTR("spacing not > 1", procName, NULL);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }
    if (orient != L_HORIZONTAL_LINE && orient != L_POS_SLOPE_LINE &&
        orient != L_VERTICAL_LINE && orient != L_NEG_SLOPE_LINE)
        return (PTAA *)ERROR_PTR("invalid line orientation", procName, NULL);

    n = boxaGetCount(boxa);
    ptaa = ptaaCreate(n);
    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        pta = generatePtaHashBox(box, spacing, width, orient, outline);
        ptaaAddPta(ptaa, pta, L_INSERT);
        boxDestroy(&box);
    }

    return ptaa;
}


/*!
 *  generatePtaPolyline()
 *
 *      Input:  pta (vertices of polyline)
 *              width
 *              closeflag (1 to close the contour; 0 otherwise)
 *              removedups  (1 to remove, 0 to leave)
 *      Return: ptad, or null on error
 */
PTA *
generatePtaPolyline(PTA     *ptas,
                    l_int32  width,
                    l_int32  closeflag,
                    l_int32  removedups)
{
l_int32  i, n, x1, y1, x2, y2;
PTA     *ptad, *ptat, *pta;

    PROCNAME("generatePtaPolyline");

    if (!ptas)
        return (PTA *)ERROR_PTR("ptas not defined", procName, NULL);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }

    n = ptaGetCount(ptas);
    ptat = ptaCreate(0);
    if (n < 2)  /* nothing to do */
        return ptat;

    ptaGetIPt(ptas, 0, &x1, &y1);
    for (i = 1; i < n; i++) {
        ptaGetIPt(ptas, i, &x2, &y2);
        pta = generatePtaWideLine(x1, y1, x2, y2, width);
        ptaJoin(ptat, pta, 0, -1);
        ptaDestroy(&pta);
        x1 = x2;
        y1 = y2;
    }

    if (closeflag) {
        ptaGetIPt(ptas, 0, &x2, &y2);
        pta = generatePtaWideLine(x1, y1, x2, y2, width);
        ptaJoin(ptat, pta, 0, -1);
        ptaDestroy(&pta);
    }

    if (removedups)
        ptad = ptaRemoveDuplicates(ptat, 0);
    else
        ptad = ptaClone(ptat);

    ptaDestroy(&ptat);
    return ptad;
}


/*!
 *  convertPtaLineTo4cc()
 *
 *      Input:  ptas (8-connected line of points)
 *      Return: ptad (4-connected line), or null on error
 *
 *  Notes:
 *      (1) When a polyline is generated with width = 1, the resulting
 *          line is not 4-connected in general.  This function adds
 *          points as necessary to convert the line to 4-cconnected.
 *          It is useful when rendering 1 bpp on a pix.
 *      (2) Do not use this for lines generated with width > 1.
 */
PTA *
convertPtaLineTo4cc(PTA  *ptas)
{
l_int32  i, n, x, y, xp, yp;
PTA     *ptad;

    PROCNAME("convertPtaLineTo4cc");

    if (!ptas)
        return (PTA *)ERROR_PTR("ptas not defined", procName, NULL);

    n = ptaGetCount(ptas);
    ptad = ptaCreate(n);
    ptaGetIPt(ptas, 0, &xp, &yp);
    ptaAddPt(ptad, xp, yp);
    for (i = 1; i < n; i++) {
        ptaGetIPt(ptas, i, &x, &y);
        if (x != xp && y != yp)  /* diagonal */
            ptaAddPt(ptad, x, yp);
        ptaAddPt(ptad, x, y);
        xp = x;
        yp = y;
    }

    return ptad;
}


/*!
 *  generatePtaFilledCircle()
 *
 *      Input:  radius
 *      Return: pta, or null on error
 *
 *  Notes:
 *      (1) The circle is has diameter = 2 * radius + 1.
 *      (2) It is located with the center of the circle at the
 *          point (radius, radius).
 *      (3) Consequently, it typically must be translated if
 *          it is to represent a set of pixels in an image.
 */
PTA *
generatePtaFilledCircle(l_int32  radius)
{
l_int32    x, y;
l_float32  radthresh, sqdist;
PTA       *pta;

    PROCNAME("generatePtaFilledCircle");

    if (radius < 1)
        return (PTA *)ERROR_PTR("radius must be >= 1", procName, NULL);

    pta = ptaCreate(0);
    radthresh = (radius + 0.5) * (radius + 0.5);
    for (y = 0; y <= 2 * radius; y++) {
        for (x = 0; x <= 2 * radius; x++) {
            sqdist = (l_float32)((y - radius) * (y - radius) +
                                 (x - radius) * (x - radius));
            if (sqdist <= radthresh)
                ptaAddPt(pta, x, y);
        }
    }

    return pta;
}


/*!
 *  generatePtaFilledSquare()
 *
 *      Input:  side
 *      Return: pta, or null on error
 *
 *  Notes:
 *      (1) The center of the square can be chosen to be at
 *          (side / 2, side / 2).  It must be translated by this amount
 *          when used for replication.
 */
PTA *
generatePtaFilledSquare(l_int32  side)
{
l_int32  x, y;
PTA     *pta;

    PROCNAME("generatePtaFilledSquare");
    if (side < 1)
        return (PTA *)ERROR_PTR("side must be > 0", procName, NULL);

    pta = ptaCreate(0);
    for (y = 0; y < side; y++)
        for (x = 0; x < side; x++)
            ptaAddPt(pta, x, y);

    return pta;
}


/*!
 *  generatePtaLineFromPt()
 *
 *      Input:  x, y  (point of origination)
 *              length (of line, including starting point)
 *              radang (angle in radians, CW from horizontal)
 *      Return: pta, or null on error
 *
 *  Notes:
 *      (1) The @length of the line is 1 greater than the distance
 *          used in locatePtRadially().  Example: a distance of 1
 *          gives rise to a length of 2.
 */
PTA *
generatePtaLineFromPt(l_int32    x,
                      l_int32    y,
                      l_float64  length,
                      l_float64  radang)
{
l_int32  x2, y2;  /* the point at the other end of the line */

    x2 = x + (l_int32)((length - 1.0) * cos(radang));
    y2 = y + (l_int32)((length - 1.0) * sin(radang));
    return generatePtaLine(x, y, x2, y2);
}


/*!
 *  locatePtRadially()
 *
 *      Input:  xr, yr  (reference point)
 *              radang (angle in radians, CW from horizontal)
 *              dist (distance of point from reference point along line
 *                    given by the specified angle)
 *              &x, &y (<return> location of point)
 *      Return: 0 if OK, 1 on error
 */
l_int32
locatePtRadially(l_int32     xr,
                 l_int32     yr,
                 l_float64   dist,
                 l_float64   radang,
                 l_float64  *px,
                 l_float64  *py)
{
    PROCNAME("locatePtRadially");

    if (!px || !py)
        return ERROR_INT("&x and &y not both defined", procName, 1);

    *px = xr + dist * cos(radang);
    *py = yr + dist * sin(radang);
    return 0;
}


/*------------------------------------------------------------------*
 *            Rendering function plots directly on images           *
 *------------------------------------------------------------------*/
/*!
 *  pixRenderPlotFromNuma()
 *
 *      Input:  &pix (any type; replaced if not 32 bpp rgb)
 *              numa (to be plotted)
 *              plotloc (location of plot: L_PLOT_AT_TOP, etc)
 *              linewidth (width of "line" that is drawn; between 1 and 7)
 *              max (maximum excursion in pixels from baseline)
 *              color (plot color: 0xrrggbb00)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Simplified interface for plotting row or column aligned data
 *          on a pix.
 *      (2) This replaces @pix with a 32 bpp rgb version if it is not
 *          already 32 bpp.  It then draws the plot on the pix.
 *      (3) See makePlotPtaFromNumaGen() for more details.
 */
l_int32
pixRenderPlotFromNuma(PIX     **ppix,
                      NUMA     *na,
                      l_int32   plotloc,
                      l_int32   linewidth,
                      l_int32   max,
                      l_uint32  color)
{
l_int32  w, h, size, rval, gval, bval;
PIX     *pix1;
PTA     *pta;

    PROCNAME("pixRenderPlotFromNuma");

    if (!ppix)
        return ERROR_INT("&pix not defined", procName, 1);
    if (*ppix == NULL)
        return ERROR_INT("pix not defined", procName, 1);

    pixGetDimensions(*ppix, &w, &h, NULL);
    size = (plotloc == L_PLOT_AT_TOP || plotloc == L_PLOT_AT_MID_HORIZ ||
            plotloc == L_PLOT_AT_BOT) ? h : w;
    pta = makePlotPtaFromNuma(na, size, plotloc, linewidth, max);
    if (!pta)
        return ERROR_INT("pta not made", procName, 1);

    if (pixGetDepth(*ppix) != 32) {
        pix1 = pixConvertTo32(*ppix);
        pixDestroy(ppix);
        *ppix = pix1;
    }
    extractRGBValues(color, &rval, &gval, &bval);
    pixRenderPtaArb(*ppix, pta, rval, gval, bval);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  makePlotPtaFromNuma()
 *
 *      Input:  numa
 *              size (pix height for horizontal plot; width for vertical plot)
 *              plotloc (location of plot: L_PLOT_AT_TOP, etc)
 *              linewidth (width of "line" that is drawn; between 1 and 7)
 *              max (maximum excursion in pixels from baseline)
 *      Return: ptad, or null on error
 *
 *  Notes:
 *      (1) This generates points from @numa representing y(x) or x(y)
 *          with respect to a pix.  A horizontal plot y(x) is drawn for
 *          a function of column position, and a vertical plot is drawn
 *          for a function x(y) of row position.  The baseline is located
 *          so that all plot points will fit in the pix.
 *      (2) See makePlotPtaFromNumaGen() for more details.
 */
PTA *
makePlotPtaFromNuma(NUMA    *na,
                    l_int32  size,
                    l_int32  plotloc,
                    l_int32  linewidth,
                    l_int32  max)
{
l_int32  orient, refpos;

    PROCNAME("makePlotPtaFromNuma");

    if (!na)
        return (PTA *)ERROR_PTR("na not defined", procName, NULL);
    if (plotloc == L_PLOT_AT_TOP || plotloc == L_PLOT_AT_MID_HORIZ ||
        plotloc == L_PLOT_AT_BOT) {
        orient = L_HORIZONTAL_LINE;
    } else if (plotloc == L_PLOT_AT_LEFT || plotloc == L_PLOT_AT_MID_VERT ||
               plotloc == L_PLOT_AT_RIGHT) {
        orient = L_VERTICAL_LINE;
    } else {
        return (PTA *)ERROR_PTR("invalid plotloc", procName, NULL);
    }

    if (plotloc == L_PLOT_AT_LEFT || plotloc == L_PLOT_AT_TOP)
        refpos = max;
    else if (plotloc == L_PLOT_AT_MID_VERT || plotloc == L_PLOT_AT_MID_HORIZ)
        refpos = size / 2;
    else  /* L_PLOT_AT_RIGHT || L_PLOT_AT_BOT */
        refpos = size - max - 1;

    return makePlotPtaFromNumaGen(na, orient, linewidth, refpos, max, 1);
}


/*!
 *  pixRenderPlotFromNumaGen()
 *
 *      Input:  &pix (any type; replaced if not 32 bpp rgb)
 *              numa (to be plotted)
 *              orient (L_HORIZONTAL_LINE, L_VERTICAL_LINE)
 *              linewidth (width of "line" that is drawn; between 1 and 7)
 *              refpos (reference position: y for horizontal and x for vertical)
 *              max (maximum excursion in pixels from baseline)
 *              drawref (1 to draw the reference line and the normal to it)
 *              color (plot color: 0xrrggbb00)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) General interface for plotting row or column aligned data
 *          on a pix.
 *      (2) This replaces @pix with a 32 bpp rgb version if it is not
 *          already 32 bpp.  It then draws the plot on the pix.
 *      (3) See makePlotPtaFromNumaGen() for other input parameters.
 */
l_int32
pixRenderPlotFromNumaGen(PIX     **ppix,
                         NUMA     *na,
                         l_int32   orient,
                         l_int32   linewidth,
                         l_int32   refpos,
                         l_int32   max,
                         l_int32   drawref,
                         l_uint32  color)
{
l_int32  rval, gval, bval;
PIX     *pix1;
PTA     *pta;

    PROCNAME("pixRenderPlotFromNumaGen");

    if (!ppix)
        return ERROR_INT("&pix not defined", procName, 1);
    if (*ppix == NULL)
        return ERROR_INT("pix not defined", procName, 1);

    pta = makePlotPtaFromNumaGen(na, orient, linewidth, refpos, max, drawref);
    if (!pta)
        return ERROR_INT("pta not made", procName, 1);

    if (pixGetDepth(*ppix) != 32) {
        pix1 = pixConvertTo32(*ppix);
        pixDestroy(ppix);
        *ppix = pix1;
    }
    extractRGBValues(color, &rval, &gval, &bval);
    pixRenderPtaArb(*ppix, pta, rval, gval, bval);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  makePlotPtaFromNumaGen()
 *
 *      Input:  numa
 *              orient (L_HORIZONTAL_LINE, L_VERTICAL_LINE)
 *              linewidth (width of "line" that is drawn; between 1 and 7)
 *              refpos (reference position: y for horizontal and x for vertical)
 *              max (maximum excursion in pixels from baseline)
 *              drawref (1 to draw the reference line and the normal to it)
 *      Return: ptad, or null on error
 *
 *  Notes:
 *      (1) This generates points from @numa representing y(x) or x(y)
 *          with respect to a pix.  For y(x), we draw a horizontal line
 *          at the reference position and a vertical line at the edge; then
 *          we draw the values of @numa, scaled so that the maximum
 *          excursion from the reference position is @max pixels.
 *      (2) The start and delx parameters of @numa are used to refer
 *          its values to the raster lines (L_VERTICAL_LINE) or columns
 *          (L_HORIZONTAL_LINE).
 *      (3) The linewidth is chosen in the interval [1 ... 7].
 *      (4) @refpos should be chosen so the plot is entirely within the pix
 *          that it will be painted onto.
 *      (5) This would typically be used to plot, in place, a function
 *          computed along pixel rows or columns.
 */
PTA *
makePlotPtaFromNumaGen(NUMA    *na,
                       l_int32  orient,
                       l_int32  linewidth,
                       l_int32  refpos,
                       l_int32  max,
                       l_int32  drawref)
{
l_int32    i, n, maxw, maxh;
l_float32  minval, maxval, absval, val, scale, start, del;
PTA       *pta1, *pta2, *ptad;

    PROCNAME("makePlotPtaFromNumaGen");

    if (!na)
        return (PTA *)ERROR_PTR("na not defined", procName, NULL);
    if (orient != L_HORIZONTAL_LINE && orient != L_VERTICAL_LINE)
        return (PTA *)ERROR_PTR("invalid orient", procName, NULL);
    if (linewidth < 1) {
        L_WARNING("linewidth < 1; setting to 1\n", procName);
        linewidth = 1;
    }
    if (linewidth > 7) {
        L_WARNING("linewidth > 7; setting to 7\n", procName);
        linewidth = 7;
    }

    numaGetMin(na, &minval, NULL);
    numaGetMax(na, &maxval, NULL);
    absval = L_MAX(L_ABS(minval), L_ABS(maxval));
    scale = (l_float32)max / (l_float32)absval;
    n = numaGetCount(na);
    numaGetParameters(na, &start, &del);

        /* Generate the plot points */
    pta1 = ptaCreate(n);
    for (i = 0; i < n; i++) {
        numaGetFValue(na, i, &val);
        if (orient == L_HORIZONTAL_LINE) {
            ptaAddPt(pta1, start + i * del, refpos + scale * val);
            maxw = (del >= 0) ? start + n * del + linewidth
                              : start + linewidth;
            maxh = refpos + max + linewidth;
        } else {  /* vertical line */
            ptaAddPt(pta1, refpos + scale * val, start + i * del);
            maxw = refpos + max + linewidth;
            maxh = (del >= 0) ? start + n * del + linewidth
                              : start + linewidth;
        }
    }

        /* Optionally, widen the plot */
    if (linewidth > 1) {
        if (linewidth % 2 == 0)  /* even linewidth; use side of a square */
            pta2 = generatePtaFilledSquare(linewidth);
        else  /* odd linewidth; use radius of a circle */
            pta2 = generatePtaFilledCircle(linewidth / 2);
        ptad = ptaReplicatePattern(pta1, NULL, pta2, linewidth / 2,
                                   linewidth / 2, maxw, maxh);
        ptaDestroy(&pta2);
    } else {
        ptad = ptaClone(pta1);
    }
    ptaDestroy(&pta1);

        /* Optionally, add the reference lines */
    if (drawref) {
        if (orient == L_HORIZONTAL_LINE) {
            pta1 = generatePtaLine(start, refpos, start + n * del, refpos);
            ptaJoin(ptad, pta1, 0, -1);
            ptaDestroy(&pta1);
            pta1 = generatePtaLine(start, refpos - max,
                                   start, refpos + max);
            ptaJoin(ptad, pta1, 0, -1);
        } else {  /* vertical line */
            pta1 = generatePtaLine(refpos, start, refpos, start + n * del);
            ptaJoin(ptad, pta1, 0, -1);
            ptaDestroy(&pta1);
            pta1 = generatePtaLine(refpos - max, start,
                                   refpos + max, start);
            ptaJoin(ptad, pta1, 0, -1);
        }
        ptaDestroy(&pta1);
    }

    return ptad;
}


/*------------------------------------------------------------------*
 *        Pta generation for arbitrary shapes built with lines      *
 *------------------------------------------------------------------*/
/*!
 *  pixRenderPta()
 *
 *      Input:  pix
 *              pta (arbitrary set of points)
 *              op   (one of L_SET_PIXELS, L_CLEAR_PIXELS, L_FLIP_PIXELS)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) L_SET_PIXELS puts all image bits in each pixel to 1
 *          (black for 1 bpp; white for depth > 1)
 *      (2) L_CLEAR_PIXELS puts all image bits in each pixel to 0
 *          (white for 1 bpp; black for depth > 1)
 *      (3) L_FLIP_PIXELS reverses all image bits in each pixel
 *      (4) This function clips the rendering to the pix.  It performs
 *          clipping for functions such as pixRenderLine(),
 *          pixRenderBox() and pixRenderBoxa(), that call pixRenderPta().
 */
l_int32
pixRenderPta(PIX     *pix,
             PTA     *pta,
             l_int32  op)
{
l_int32  i, n, x, y, w, h, d, maxval;

    PROCNAME("pixRenderPta");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    if (op != L_SET_PIXELS && op != L_CLEAR_PIXELS && op != L_FLIP_PIXELS)
        return ERROR_INT("invalid op", procName, 1);

    pixGetDimensions(pix, &w, &h, &d);
    maxval = 1;
    if (op == L_SET_PIXELS) {
        switch (d)
        {
        case 2:
            maxval = 0x3;
            break;
        case 4:
            maxval = 0xf;
            break;
        case 8:
            maxval = 0xff;
            break;
        case 16:
            maxval = 0xffff;
            break;
        case 32:
            maxval = 0xffffffff;
            break;
        }
    }

    n = ptaGetCount(pta);
    for (i = 0; i < n; i++) {
        ptaGetIPt(pta, i, &x, &y);
        if (x < 0 || x >= w)
            continue;
        if (y < 0 || y >= h)
            continue;
        switch (op)
        {
        case L_SET_PIXELS:
            pixSetPixel(pix, x, y, maxval);
            break;
        case L_CLEAR_PIXELS:
            pixClearPixel(pix, x, y);
            break;
        case L_FLIP_PIXELS:
            pixFlipPixel(pix, x, y);
            break;
        default:
            break;
        }
    }

    return 0;
}


/*!
 *  pixRenderPtaArb()
 *
 *      Input:  pix (any depth, cmapped ok)
 *              pta (arbitrary set of points)
 *              rval, gval, bval
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If pix is colormapped, render this color (or the nearest
 *          color if the cmap is full) on each pixel.
 *      (2) If pix is not colormapped, do the best job you can using
 *          the input colors:
 *          - d = 1: set the pixels
 *          - d = 2, 4, 8: average the input rgb value
 *          - d = 32: use the input rgb value
 *      (3) This function clips the rendering to the pix.
 */
l_int32
pixRenderPtaArb(PIX     *pix,
                PTA     *pta,
                l_uint8  rval,
                l_uint8  gval,
                l_uint8  bval)
{
l_int32   i, n, x, y, w, h, d, index;
l_uint8   val;
l_uint32  val32;
PIXCMAP  *cmap;

    PROCNAME("pixRenderPtaArb");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    d = pixGetDepth(pix);
    if (d != 1 && d != 2 && d != 4 && d != 8 && d != 32)
        return ERROR_INT("depth not in {1,2,4,8,32}", procName, 1);

    if (d == 1) {
        pixRenderPta(pix, pta, L_SET_PIXELS);
        return 0;
    }

    cmap = pixGetColormap(pix);
    pixGetDimensions(pix, &w, &h, &d);
    if (cmap) {
        pixcmapAddNearestColor(cmap, rval, gval, bval, &index);
    } else {
        if (d == 2)
            val = (rval + gval + bval) / (3 * 64);
        else if (d == 4)
            val = (rval + gval + bval) / (3 * 16);
        else if (d == 8)
            val = (rval + gval + bval) / 3;
        else  /* d == 32 */
            composeRGBPixel(rval, gval, bval, &val32);
    }

    n = ptaGetCount(pta);
    for (i = 0; i < n; i++) {
        ptaGetIPt(pta, i, &x, &y);
        if (x < 0 || x >= w)
            continue;
        if (y < 0 || y >= h)
            continue;
        if (cmap)
            pixSetPixel(pix, x, y, index);
        else if (d == 32)
            pixSetPixel(pix, x, y, val32);
        else
            pixSetPixel(pix, x, y, val);
    }

    return 0;
}


/*!
 *  pixRenderPtaBlend()
 *
 *      Input:  pix (32 bpp rgb)
 *              pta  (arbitrary set of points)
 *              rval, gval, bval
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This function clips the rendering to the pix.
 */
l_int32
pixRenderPtaBlend(PIX     *pix,
                  PTA     *pta,
                  l_uint8  rval,
                  l_uint8  gval,
                  l_uint8  bval,
                  l_float32 fract)
{
l_int32    i, n, x, y, w, h;
l_uint8    nrval, ngval, nbval;
l_uint32   val32;
l_float32  frval, fgval, fbval;

    PROCNAME("pixRenderPtaBlend");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    if (pixGetDepth(pix) != 32)
        return ERROR_INT("depth not 32 bpp", procName, 1);
    if (fract < 0.0 || fract > 1.0) {
        L_WARNING("fract must be in [0.0, 1.0]; setting to 0.5\n", procName);
        fract = 0.5;
    }

    pixGetDimensions(pix, &w, &h, NULL);
    n = ptaGetCount(pta);
    frval = fract * rval;
    fgval = fract * gval;
    fbval = fract * bval;
    for (i = 0; i < n; i++) {
        ptaGetIPt(pta, i, &x, &y);
        if (x < 0 || x >= w)
            continue;
        if (y < 0 || y >= h)
            continue;
        pixGetPixel(pix, x, y, &val32);
        nrval = GET_DATA_BYTE(&val32, COLOR_RED);
        nrval = (l_uint8)((1. - fract) * nrval + frval);
        ngval = GET_DATA_BYTE(&val32, COLOR_GREEN);
        ngval = (l_uint8)((1. - fract) * ngval + fgval);
        nbval = GET_DATA_BYTE(&val32, COLOR_BLUE);
        nbval = (l_uint8)((1. - fract) * nbval + fbval);
        composeRGBPixel(nrval, ngval, nbval, &val32);
        pixSetPixel(pix, x, y, val32);
    }

    return 0;
}


/*------------------------------------------------------------------*
 *           Rendering of arbitrary shapes built with lines         *
 *------------------------------------------------------------------*/
/*!
 *  pixRenderLine()
 *
 *      Input:  pix
 *              x1, y1
 *              x2, y2
 *              width  (thickness of line)
 *              op  (one of L_SET_PIXELS, L_CLEAR_PIXELS, L_FLIP_PIXELS)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixRenderLine(PIX     *pix,
              l_int32  x1,
              l_int32  y1,
              l_int32  x2,
              l_int32  y2,
              l_int32  width,
              l_int32  op)
{
PTA  *pta;

    PROCNAME("pixRenderLine");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (width < 1) {
        L_WARNING("width must be > 0; setting to 1\n", procName);
        width = 1;
    }
    if (op != L_SET_PIXELS && op != L_CLEAR_PIXELS && op != L_FLIP_PIXELS)
        return ERROR_INT("invalid op", procName, 1);

    if ((pta = generatePtaWideLine(x1, y1, x2, y2, width)) == NULL)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPta(pix, pta, op);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderLineArb()
 *
 *      Input:  pix
 *              x1, y1
 *              x2, y2
 *              width  (thickness of line)
 *              rval, gval, bval
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixRenderLineArb(PIX     *pix,
                 l_int32  x1,
                 l_int32  y1,
                 l_int32  x2,
                 l_int32  y2,
                 l_int32  width,
                 l_uint8  rval,
                 l_uint8  gval,
                 l_uint8  bval)
{
PTA  *pta;

    PROCNAME("pixRenderLineArb");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (width < 1) {
        L_WARNING("width must be > 0; setting to 1\n", procName);
        width = 1;
    }

    if ((pta = generatePtaWideLine(x1, y1, x2, y2, width)) == NULL)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPtaArb(pix, pta, rval, gval, bval);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderLineBlend()
 *
 *      Input:  pix
 *              x1, y1
 *              x2, y2
 *              width  (thickness of line)
 *              rval, gval, bval
 *              fract
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixRenderLineBlend(PIX       *pix,
                   l_int32    x1,
                   l_int32    y1,
                   l_int32    x2,
                   l_int32    y2,
                   l_int32    width,
                   l_uint8    rval,
                   l_uint8    gval,
                   l_uint8    bval,
                   l_float32  fract)
{
PTA  *pta;

    PROCNAME("pixRenderLineBlend");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (width < 1) {
        L_WARNING("width must be > 0; setting to 1\n", procName);
        width = 1;
    }

    if ((pta = generatePtaWideLine(x1, y1, x2, y2, width)) == NULL)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPtaBlend(pix, pta, rval, gval, bval, fract);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderBox()
 *
 *      Input:  pix
 *              box
 *              width  (thickness of box lines)
 *              op  (one of L_SET_PIXELS, L_CLEAR_PIXELS, L_FLIP_PIXELS)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixRenderBox(PIX     *pix,
             BOX     *box,
             l_int32  width,
             l_int32  op)
{
PTA  *pta;

    PROCNAME("pixRenderBox");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }
    if (op != L_SET_PIXELS && op != L_CLEAR_PIXELS && op != L_FLIP_PIXELS)
        return ERROR_INT("invalid op", procName, 1);

    if ((pta = generatePtaBox(box, width)) == NULL)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPta(pix, pta, op);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderBoxArb()
 *
 *      Input:  pix (any depth, cmapped ok)
 *              box
 *              width  (thickness of box lines)
 *              rval, gval, bval
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixRenderBoxArb(PIX     *pix,
                BOX     *box,
                l_int32  width,
                l_uint8  rval,
                l_uint8  gval,
                l_uint8  bval)
{
PTA  *pta;

    PROCNAME("pixRenderBoxArb");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }

    if ((pta = generatePtaBox(box, width)) == NULL)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPtaArb(pix, pta, rval, gval, bval);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderBoxBlend()
 *
 *      Input:  pix
 *              box
 *              width  (thickness of box lines)
 *              rval, gval, bval
 *              fract (in [0.0 - 1.0]; complete transparency (no effect)
 *                     if 0.0; no transparency if 1.0)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixRenderBoxBlend(PIX       *pix,
                  BOX       *box,
                  l_int32    width,
                  l_uint8    rval,
                  l_uint8    gval,
                  l_uint8    bval,
                  l_float32  fract)
{
PTA  *pta;

    PROCNAME("pixRenderBoxBlend");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }

    if ((pta = generatePtaBox(box, width)) == NULL)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPtaBlend(pix, pta, rval, gval, bval, fract);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderBoxa()
 *
 *      Input:  pix
 *              boxa
 *              width  (thickness of line)
 *              op  (one of L_SET_PIXELS, L_CLEAR_PIXELS, L_FLIP_PIXELS)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixRenderBoxa(PIX     *pix,
              BOXA    *boxa,
              l_int32  width,
              l_int32  op)
{
PTA  *pta;

    PROCNAME("pixRenderBoxa");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }
    if (op != L_SET_PIXELS && op != L_CLEAR_PIXELS && op != L_FLIP_PIXELS)
        return ERROR_INT("invalid op", procName, 1);

    if ((pta = generatePtaBoxa(boxa, width, 0)) == NULL)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPta(pix, pta, op);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderBoxaArb()
 *
 *      Input:  pix
 *              boxa
 *              width  (thickness of line)
 *              rval, gval, bval
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixRenderBoxaArb(PIX     *pix,
                 BOXA    *boxa,
                 l_int32  width,
                 l_uint8  rval,
                 l_uint8  gval,
                 l_uint8  bval)
{
PTA  *pta;

    PROCNAME("pixRenderBoxaArb");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }

    if ((pta = generatePtaBoxa(boxa, width, 0)) == NULL)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPtaArb(pix, pta, rval, gval, bval);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderBoxaBlend()
 *
 *      Input:  pix
 *              boxa
 *              width  (thickness of line)
 *              rval, gval, bval
 *              fract (in [0.0 - 1.0]; complete transparency (no effect)
 *                     if 0.0; no transparency if 1.0)
 *              removedups  (1 to remove; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixRenderBoxaBlend(PIX       *pix,
                   BOXA      *boxa,
                   l_int32    width,
                   l_uint8    rval,
                   l_uint8    gval,
                   l_uint8    bval,
                   l_float32  fract,
                   l_int32    removedups)
{
PTA  *pta;

    PROCNAME("pixRenderBoxaBlend");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }

    if ((pta = generatePtaBoxa(boxa, width, removedups)) == NULL)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPtaBlend(pix, pta, rval, gval, bval, fract);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderHashBox()
 *
 *      Input:  pix
 *              box
 *              spacing (spacing between lines; must be > 1)
 *              width  (thickness of box and hash lines)
 *              orient  (orientation of lines: L_HORIZONTAL_LINE, ...)
 *              outline  (0 to skip drawing box outline)
 *              op  (one of L_SET_PIXELS, L_CLEAR_PIXELS, L_FLIP_PIXELS)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixRenderHashBox(PIX     *pix,
                 BOX     *box,
                 l_int32  spacing,
                 l_int32  width,
                 l_int32  orient,
                 l_int32  outline,
                 l_int32  op)
{
PTA  *pta;

    PROCNAME("pixRenderHashBox");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if (spacing <= 1)
        return ERROR_INT("spacing not > 1", procName, 1);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }
    if (orient != L_HORIZONTAL_LINE && orient != L_POS_SLOPE_LINE &&
        orient != L_VERTICAL_LINE && orient != L_NEG_SLOPE_LINE)
        return ERROR_INT("invalid line orientation", procName, 1);
    if (op != L_SET_PIXELS && op != L_CLEAR_PIXELS && op != L_FLIP_PIXELS)
        return ERROR_INT("invalid op", procName, 1);

    pta = generatePtaHashBox(box, spacing, width, orient, outline);
    if (!pta)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPta(pix, pta, op);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderHashBoxArb()
 *
 *      Input:  pix
 *              box
 *              spacing (spacing between lines; must be > 1)
 *              width  (thickness of box and hash lines)
 *              orient  (orientation of lines: L_HORIZONTAL_LINE, ...)
 *              outline  (0 to skip drawing box outline)
 *              rval, gval, bval
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixRenderHashBoxArb(PIX     *pix,
                    BOX     *box,
                    l_int32  spacing,
                    l_int32  width,
                    l_int32  orient,
                    l_int32  outline,
                    l_int32  rval,
                    l_int32  gval,
                    l_int32  bval)
{
PTA  *pta;

    PROCNAME("pixRenderHashBoxArb");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if (spacing <= 1)
        return ERROR_INT("spacing not > 1", procName, 1);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }
    if (orient != L_HORIZONTAL_LINE && orient != L_POS_SLOPE_LINE &&
        orient != L_VERTICAL_LINE && orient != L_NEG_SLOPE_LINE)
        return ERROR_INT("invalid line orientation", procName, 1);

    pta = generatePtaHashBox(box, spacing, width, orient, outline);
    if (!pta)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPtaArb(pix, pta, rval, gval, bval);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderHashBoxBlend()
 *
 *      Input:  pix
 *              box
 *              spacing (spacing between lines; must be > 1)
 *              width  (thickness of box and hash lines)
 *              orient  (orientation of lines: L_HORIZONTAL_LINE, ...)
 *              outline  (0 to skip drawing box outline)
 *              rval, gval, bval
 *              fract (in [0.0 - 1.0]; complete transparency (no effect)
 *                     if 0.0; no transparency if 1.0)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixRenderHashBoxBlend(PIX       *pix,
                      BOX       *box,
                      l_int32    spacing,
                      l_int32    width,
                      l_int32    orient,
                      l_int32    outline,
                      l_int32    rval,
                      l_int32    gval,
                      l_int32    bval,
                      l_float32  fract)
{
PTA  *pta;

    PROCNAME("pixRenderHashBoxBlend");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!box)
        return ERROR_INT("box not defined", procName, 1);
    if (spacing <= 1)
        return ERROR_INT("spacing not > 1", procName, 1);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }
    if (orient != L_HORIZONTAL_LINE && orient != L_POS_SLOPE_LINE &&
        orient != L_VERTICAL_LINE && orient != L_NEG_SLOPE_LINE)
        return ERROR_INT("invalid line orientation", procName, 1);

    pta = generatePtaHashBox(box, spacing, width, orient, outline);
    if (!pta)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPtaBlend(pix, pta, rval, gval, bval, fract);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderHashBoxa()
 *
 *      Input:  pix
 *              boxa
 *              spacing (spacing between lines; must be > 1)
 *              width  (thickness of box and hash lines)
 *              orient  (orientation of lines: L_HORIZONTAL_LINE, ...)
 *              outline  (0 to skip drawing box outline)
 *              op  (one of L_SET_PIXELS, L_CLEAR_PIXELS, L_FLIP_PIXELS)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixRenderHashBoxa(PIX     *pix,
                  BOXA    *boxa,
                  l_int32  spacing,
                  l_int32  width,
                  l_int32  orient,
                  l_int32  outline,
                  l_int32  op)
 {
PTA  *pta;

    PROCNAME("pixRenderHashBoxa");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (spacing <= 1)
        return ERROR_INT("spacing not > 1", procName, 1);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }
    if (orient != L_HORIZONTAL_LINE && orient != L_POS_SLOPE_LINE &&
        orient != L_VERTICAL_LINE && orient != L_NEG_SLOPE_LINE)
        return ERROR_INT("invalid line orientation", procName, 1);
    if (op != L_SET_PIXELS && op != L_CLEAR_PIXELS && op != L_FLIP_PIXELS)
        return ERROR_INT("invalid op", procName, 1);

    pta = generatePtaHashBoxa(boxa, spacing, width, orient, outline, 1);
    if (!pta)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPta(pix, pta, op);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderHashBoxaArb()
 *
 *      Input:  pix
 *              boxa
 *              spacing (spacing between lines; must be > 1)
 *              width  (thickness of box and hash lines)
 *              orient  (orientation of lines: L_HORIZONTAL_LINE, ...)
 *              outline  (0 to skip drawing box outline)
 *              rval, gval, bval
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixRenderHashBoxaArb(PIX     *pix,
                     BOXA    *boxa,
                     l_int32  spacing,
                     l_int32  width,
                     l_int32  orient,
                     l_int32  outline,
                     l_int32  rval,
                     l_int32  gval,
                     l_int32  bval)
{
PTA  *pta;

    PROCNAME("pixRenderHashBoxArb");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (spacing <= 1)
        return ERROR_INT("spacing not > 1", procName, 1);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }
    if (orient != L_HORIZONTAL_LINE && orient != L_POS_SLOPE_LINE &&
        orient != L_VERTICAL_LINE && orient != L_NEG_SLOPE_LINE)
        return ERROR_INT("invalid line orientation", procName, 1);

    pta = generatePtaHashBoxa(boxa, spacing, width, orient, outline, 1);
    if (!pta)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPtaArb(pix, pta, rval, gval, bval);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderHashBoxaBlend()
 *
 *      Input:  pix
 *              boxa
 *              spacing (spacing between lines; must be > 1)
 *              width  (thickness of box and hash lines)
 *              orient  (orientation of lines: L_HORIZONTAL_LINE, ...)
 *              outline  (0 to skip drawing box outline)
 *              rval, gval, bval
 *              fract (in [0.0 - 1.0]; complete transparency (no effect)
 *                     if 0.0; no transparency if 1.0)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixRenderHashBoxaBlend(PIX       *pix,
                       BOXA      *boxa,
                       l_int32    spacing,
                       l_int32    width,
                       l_int32    orient,
                       l_int32    outline,
                       l_int32    rval,
                       l_int32    gval,
                       l_int32    bval,
                       l_float32  fract)
{
PTA  *pta;

    PROCNAME("pixRenderHashBoxaBlend");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!boxa)
        return ERROR_INT("boxa not defined", procName, 1);
    if (spacing <= 1)
        return ERROR_INT("spacing not > 1", procName, 1);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }
    if (orient != L_HORIZONTAL_LINE && orient != L_POS_SLOPE_LINE &&
        orient != L_VERTICAL_LINE && orient != L_NEG_SLOPE_LINE)
        return ERROR_INT("invalid line orientation", procName, 1);

    pta = generatePtaHashBoxa(boxa, spacing, width, orient, outline, 1);
    if (!pta)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPtaBlend(pix, pta, rval, gval, bval, fract);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderPolyline()
 *
 *      Input:  pix
 *              ptas
 *              width  (thickness of line)
 *              op  (one of L_SET_PIXELS, L_CLEAR_PIXELS, L_FLIP_PIXELS)
 *              closeflag (1 to close the contour; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 *
 *  Note: this renders a closed contour.
 */
l_int32
pixRenderPolyline(PIX     *pix,
                  PTA     *ptas,
                  l_int32  width,
                  l_int32  op,
                  l_int32  closeflag)
{
PTA  *pta;

    PROCNAME("pixRenderPolyline");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!ptas)
        return ERROR_INT("ptas not defined", procName, 1);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }
    if (op != L_SET_PIXELS && op != L_CLEAR_PIXELS && op != L_FLIP_PIXELS)
        return ERROR_INT("invalid op", procName, 1);

    if ((pta = generatePtaPolyline(ptas, width, closeflag, 0)) == NULL)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPta(pix, pta, op);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderPolylineArb()
 *
 *      Input:  pix
 *              ptas
 *              width  (thickness of line)
 *              rval, gval, bval
 *              closeflag (1 to close the contour; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 *
 *  Note: this renders a closed contour.
 */
l_int32
pixRenderPolylineArb(PIX     *pix,
                     PTA     *ptas,
                     l_int32  width,
                     l_uint8  rval,
                     l_uint8  gval,
                     l_uint8  bval,
                     l_int32  closeflag)
{
PTA  *pta;

    PROCNAME("pixRenderPolylineArb");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!ptas)
        return ERROR_INT("ptas not defined", procName, 1);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }

    if ((pta = generatePtaPolyline(ptas, width, closeflag, 0)) == NULL)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPtaArb(pix, pta, rval, gval, bval);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderPolylineBlend()
 *
 *      Input:  pix
 *              ptas
 *              width  (thickness of line)
 *              rval, gval, bval
 *              fract (in [0.0 - 1.0]; complete transparency (no effect)
 *                     if 0.0; no transparency if 1.0)
 *              closeflag (1 to close the contour; 0 otherwise)
 *              removedups  (1 to remove; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixRenderPolylineBlend(PIX       *pix,
                       PTA       *ptas,
                       l_int32    width,
                       l_uint8    rval,
                       l_uint8    gval,
                       l_uint8    bval,
                       l_float32  fract,
                       l_int32    closeflag,
                       l_int32    removedups)
{
PTA  *pta;

    PROCNAME("pixRenderPolylineBlend");

    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);
    if (!ptas)
        return ERROR_INT("ptas not defined", procName, 1);
    if (width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }

    if ((pta = generatePtaPolyline(ptas, width, closeflag, removedups)) == NULL)
        return ERROR_INT("pta not made", procName, 1);
    pixRenderPtaBlend(pix, pta, rval, gval, bval, fract);
    ptaDestroy(&pta);
    return 0;
}


/*!
 *  pixRenderRandomCmapPtaa()
 *
 *      Input:  pix (1, 2, 4, 8, 16, 32 bpp)
 *              ptaa
 *              polyflag (1 to interpret each Pta as a polyline; 0 to simply
 *                        render the Pta as a set of pixels)
 *              width  (thickness of line; use only for polyline)
 *              closeflag (1 to close the contour; 0 otherwise;
 *                         use only for polyline mode)
 *      Return: pixd (cmapped, 8 bpp) or null on error
 *
 *  Notes:
 *      (1) This is a debugging routine, that displays a set of
 *          pixels, selected by the set of Ptas in a Ptaa,
 *          in a random color in a pix.
 *      (2) If @polyflag == 1, each Pta is considered to be a polyline,
 *          and is rendered using @width and @closeflag.  Each polyline
 *          is rendered in a random color.
 *      (3) If @polyflag == 0, all points in each Pta are rendered in a
 *          random color.  The @width and @closeflag parameters are ignored.
 *      (4) The output pix is 8 bpp and colormapped.  Up to 254
 *          different, randomly selected colors, can be used.
 *      (5) The rendered pixels replace the input pixels.  They will
 *          be clipped silently to the input pix.
 */
PIX  *
pixRenderRandomCmapPtaa(PIX     *pix,
                        PTAA    *ptaa,
                        l_int32  polyflag,
                        l_int32  width,
                        l_int32  closeflag)
{
l_int32   i, n, index, rval, gval, bval;
PIXCMAP  *cmap;
PTA      *pta, *ptat;
PIX      *pixd;

    PROCNAME("pixRenderRandomCmapPtaa");

    if (!pix)
        return (PIX *)ERROR_PTR("pix not defined", procName, NULL);
    if (!ptaa)
        return (PIX *)ERROR_PTR("ptaa not defined", procName, NULL);
    if (polyflag != 0 && width < 1) {
        L_WARNING("width < 1; setting to 1\n", procName);
        width = 1;
    }

    pixd = pixConvertTo8(pix, FALSE);
    cmap = pixcmapCreateRandom(8, 1, 1);
    pixSetColormap(pixd, cmap);

    if ((n = ptaaGetCount(ptaa)) == 0)
        return pixd;

    for (i = 0; i < n; i++) {
        index = 1 + (i % 254);
        pixcmapGetColor(cmap, index, &rval, &gval, &bval);
        pta = ptaaGetPta(ptaa, i, L_CLONE);
        if (polyflag)
            ptat = generatePtaPolyline(pta, width, closeflag, 0);
        else
            ptat = ptaClone(pta);
        pixRenderPtaArb(pixd, ptat, rval, gval, bval);
        ptaDestroy(&pta);
        ptaDestroy(&ptat);
    }

    return pixd;
}



/*------------------------------------------------------------------*
 *                Rendering and filling of polygons                 *
 *------------------------------------------------------------------*/
/*!
 *  pixRenderPolygon()
 *
 *      Input:  ptas (of vertices, none repeated)
 *              width (of polygon outline)
 *              &xmin (<optional return> min x value of input pts)
 *              &ymin (<optional return> min y value of input pts)
 *      Return: pix (1 bpp, with outline generated), or null on error
 *
 *  Notes:
 *      (1) The pix is the minimum size required to contain the origin
 *          and the polygon.  For example, the max x value of the input
 *          points is w - 1, where w is the pix width.
 *      (2) The rendered line is 4-connected, so that an interior or
 *          exterior 8-c.c. flood fill operation works properly.
 */
PIX *
pixRenderPolygon(PTA      *ptas,
                 l_int32   width,
                 l_int32  *pxmin,
                 l_int32  *pymin)
{
l_float32  fxmin, fxmax, fymin, fymax;
PIX       *pixd;
PTA       *pta1, *pta2;

    PROCNAME("pixRenderPolygon");

    if (pxmin) *pxmin = 0;
    if (pymin) *pymin = 0;
    if (!ptas)
        return (PIX *)ERROR_PTR("ptas not defined", procName, NULL);

        /* Generate a 4-connected polygon line */
    if ((pta1 = generatePtaPolyline(ptas, width, 1, 0)) == NULL)
        return (PIX *)ERROR_PTR("pta1 not made", procName, NULL);
    if (width < 2)
        pta2 = convertPtaLineTo4cc(pta1);
    else
        pta2 = ptaClone(pta1);

        /* Render onto a minimum-sized pix */
    ptaGetRange(pta2, &fxmin, &fxmax, &fymin, &fymax);
    if (pxmin) *pxmin = (l_int32)(fxmin + 0.5);
    if (pymin) *pymin = (l_int32)(fymin + 0.5);
    pixd = pixCreate((l_int32)(fxmax + 0.5) + 1, (l_int32)(fymax + 0.5) + 1, 1);
    pixRenderPolyline(pixd, pta2, width, L_SET_PIXELS, 1);
    ptaDestroy(&pta1);
    ptaDestroy(&pta2);
    return pixd;
}


/*!
 *  pixFillPolygon()
 *
 *      Input:  pixs (1 bpp, with 4-connected polygon outline)
 *              pta (vertices of the polygon)
 *              xmin, ymin (min values of vertices of polygon)
 *      Return: pixd (with outline filled), or null on error
 *
 *  Notes:
 *      (1) This fills the interior of the polygon, returning a
 *          new pix.  It works for both convex and non-convex polygons.
 *      (2) To generate a filled polygon from a pta:
 *            PIX *pixt = pixRenderPolygon(pta, 1, &xmin, &ymin);
 *            PIX *pixd = pixFillPolygon(pixt, pta, xmin, ymin);
 *            pixDestroy(&pixt);
 */
PIX *
pixFillPolygon(PIX     *pixs,
               PTA     *pta,
               l_int32  xmin,
               l_int32  ymin)
{
l_int32   w, h, i, n, inside, found;
l_int32  *xstart, *xend;
PIX      *pixi, *pixd;

    PROCNAME("pixFillPolygon");

    if (!pixs || (pixGetDepth(pixs) != 1))
        return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
    if (!pta)
        return (PIX *)ERROR_PTR("pta not defined", procName, NULL);

    pixGetDimensions(pixs, &w, &h, NULL);
    xstart = (l_int32 *)CALLOC(w / 2, sizeof(l_int32));
    xend = (l_int32 *)CALLOC(w / 2, sizeof(l_int32));

        /* Find a raster with 2 or more black runs.  The first background
         * pixel after the end of the first run is likely to be inside
         * the polygon, and can be used as a seed pixel. */
    found = FALSE;
    for (i = ymin + 1; i < h; i++) {
        pixFindHorizontalRuns(pixs, i, xstart, xend, &n);
        if (n > 1) {
            ptaPtInsidePolygon(pta, xend[0] + 1, i, &inside);
            if (inside) {
                found = TRUE;
                break;
            }
        }
    }
    if (!found) {
        L_WARNING("nothing found to fill\n", procName);
        FREE(xstart);
        FREE(xend);
        return 0;
    }

        /* Place the seed pixel in the output image */
    pixd = pixCreateTemplate(pixs);
    pixSetPixel(pixd, xend[0] + 1, i, 1);

        /* Invert pixs to make a filling mask, and fill from the seed */
    pixi = pixInvert(NULL, pixs);
    pixSeedfillBinary(pixd, pixd, pixi, 4);

        /* Add the pixels of the original polygon outline */
    pixOr(pixd, pixd, pixs);

    pixDestroy(&pixi);
    FREE(xstart);
    FREE(xend);
    return pixd;
}


/*------------------------------------------------------------------*
 *             Contour rendering on grayscale images                *
 *------------------------------------------------------------------*/
/*!
 *  pixRenderContours()
 *
 *      Input:  pixs (8 or 16 bpp; no colormap)
 *              startval (value of lowest contour; must be in [0 ... maxval])
 *              incr  (increment to next contour; must be > 0)
 *              outdepth (either 1 or depth of pixs)
 *      Return: pixd, or null on error
 *
 *  Notes:
 *      (1) The output can be either 1 bpp, showing just the contour
 *          lines, or a copy of the input pixs with the contour lines
 *          superposed.
 */
PIX *
pixRenderContours(PIX     *pixs,
                  l_int32  startval,
                  l_int32  incr,
                  l_int32  outdepth)
{
l_int32    w, h, d, maxval, wpls, wpld, i, j, val, test;
l_uint32  *datas, *datad, *lines, *lined;
PIX       *pixd;

    PROCNAME("pixRenderContours");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (pixGetColormap(pixs))
        return (PIX *)ERROR_PTR("pixs has colormap", procName, NULL);
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8 && d != 16)
        return (PIX *)ERROR_PTR("pixs not 8 or 16 bpp", procName, NULL);
    if (outdepth != 1 && outdepth != d) {
        L_WARNING("invalid outdepth; setting to 1\n", procName);
        outdepth = 1;
    }
    maxval = (1 << d) - 1;
    if (startval < 0 || startval > maxval)
        return (PIX *)ERROR_PTR("startval not in [0 ... maxval]",
               procName, NULL);
    if (incr < 1)
        return (PIX *)ERROR_PTR("incr < 1", procName, NULL);

    if (outdepth == d)
        pixd = pixCopy(NULL, pixs);
    else
        pixd = pixCreate(w, h, 1);

    pixCopyResolution(pixd, pixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    datas = pixGetData(pixs);
    wpls = pixGetWpl(pixs);

    switch (d)
    {
    case 8:
        if (outdepth == 1) {
            for (i = 0; i < h; i++) {
                lines = datas + i * wpls;
                lined = datad + i * wpld;
                for (j = 0; j < w; j++) {
                    val = GET_DATA_BYTE(lines, j);
                    if (val < startval)
                        continue;
                    test = (val - startval) % incr;
                    if (!test)
                        SET_DATA_BIT(lined, j);
                }
            }
        } else {  /* outdepth == d */
            for (i = 0; i < h; i++) {
                lines = datas + i * wpls;
                lined = datad + i * wpld;
                for (j = 0; j < w; j++) {
                    val = GET_DATA_BYTE(lines, j);
                    if (val < startval)
                        continue;
                    test = (val - startval) % incr;
                    if (!test)
                        SET_DATA_BYTE(lined, j, 0);
                }
            }
        }
        break;

    case 16:
        if (outdepth == 1) {
            for (i = 0; i < h; i++) {
                lines = datas + i * wpls;
                lined = datad + i * wpld;
                for (j = 0; j < w; j++) {
                    val = GET_DATA_TWO_BYTES(lines, j);
                    if (val < startval)
                        continue;
                    test = (val - startval) % incr;
                    if (!test)
                        SET_DATA_BIT(lined, j);
                }
            }
        } else {  /* outdepth == d */
            for (i = 0; i < h; i++) {
                lines = datas + i * wpls;
                lined = datad + i * wpld;
                for (j = 0; j < w; j++) {
                    val = GET_DATA_TWO_BYTES(lines, j);
                    if (val < startval)
                        continue;
                    test = (val - startval) % incr;
                    if (!test)
                        SET_DATA_TWO_BYTES(lined, j, 0);
                }
            }
        }
        break;

    default:
        return (PIX *)ERROR_PTR("pixs not 8 or 16 bpp", procName, NULL);
    }

    return pixd;
}


/*!
 *  fpixAutoRenderContours()
 *
 *      Input:  fpix
 *              ncontours (> 1, < 500, typ. about 50)
 *      Return: pixd (8 bpp), or null on error
 *
 *  Notes:
 *      (1) The increment is set to get approximately @ncontours.
 *      (2) The proximity to the target value for contour display
 *          is set to 0.15.
 *      (3) Negative values are rendered in red; positive values as black.
 */
PIX *
fpixAutoRenderContours(FPIX    *fpix,
                       l_int32  ncontours)
{
l_float32  minval, maxval, incr;

    PROCNAME("fpixAutoRenderContours");

    if (!fpix)
        return (PIX *)ERROR_PTR("fpix not defined", procName, NULL);
    if (ncontours < 2 || ncontours > 500)
        return (PIX *)ERROR_PTR("ncontours < 2 or > 500", procName, NULL);

    fpixGetMin(fpix, &minval, NULL, NULL);
    fpixGetMax(fpix, &maxval, NULL, NULL);
    if (minval == maxval)
        return (PIX *)ERROR_PTR("all values in fpix are equal", procName, NULL);
    incr = (maxval - minval) / ((l_float32)ncontours - 1);
    return fpixRenderContours(fpix, incr, 0.15);
}


/*!
 *  fpixRenderContours()
 *
 *      Input:  fpixs
 *              incr  (increment between contours; must be > 0.0)
 *              proxim (required proximity to target value; default 0.15)
 *      Return: pixd (8 bpp), or null on error
 *
 *  Notes:
 *      (1) Values are displayed when val/incr is within +-proxim
 *          to an integer.  The default value is 0.15; smaller values
 *          result in thinner contour lines.
 *      (2) Negative values are rendered in red; positive values as black.
 */
PIX *
fpixRenderContours(FPIX      *fpixs,
                   l_float32  incr,
                   l_float32  proxim)
{
l_int32     i, j, w, h, wpls, wpld;
l_float32   val, invincr, finter, above, below, diff;
l_uint32   *datad, *lined;
l_float32  *datas, *lines;
PIX        *pixd;
PIXCMAP    *cmap;

    PROCNAME("fpixRenderContours");

    if (!fpixs)
        return (PIX *)ERROR_PTR("fpixs not defined", procName, NULL);
    if (incr <= 0.0)
        return (PIX *)ERROR_PTR("incr <= 0.0", procName, NULL);
    if (proxim <= 0.0)
        proxim = 0.15;  /* default */

    fpixGetDimensions(fpixs, &w, &h);
    if ((pixd = pixCreate(w, h, 8)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    cmap = pixcmapCreate(8);
    pixSetColormap(pixd, cmap);
    pixcmapAddColor(cmap, 255, 255, 255);  /* white */
    pixcmapAddColor(cmap, 0, 0, 0);  /* black */
    pixcmapAddColor(cmap, 255, 0, 0);  /* red */

    datas = fpixGetData(fpixs);
    wpls = fpixGetWpl(fpixs);
    datad = pixGetData(pixd);
    wpld = pixGetWpl(pixd);
    invincr = 1.0 / incr;
    for (i = 0; i < h; i++) {
        lines = datas + i * wpls;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            val = lines[j];
            finter = invincr * val;
            above = finter - floorf(finter);
            below = ceilf(finter) - finter;
            diff = L_MIN(above, below);
            if (diff <= proxim) {
                if (val < 0.0)
                    SET_DATA_BYTE(lined, j, 2);
                else
                    SET_DATA_BYTE(lined, j, 1);
            }
        }
    }

    return pixd;
}
