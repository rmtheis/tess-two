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
 *  graphics.c
 *                     
 *      Pta generation for arbitrary shapes built with lines
 *
 *          PTA        *generatePtaLine()
 *          PTA        *generatePtaWideLine()
 *          PTA        *generatePtaBox()
 *          PTA        *generatePtaHashBox()
 *          PTA        *generatePtaBoxa()
 *          PTAA       *generatePtaaBoxa()
 *          PTAA       *generatePtaaHashBoxa()
 *          PTA        *generatePtaPolyline()
 *          PTA        *generatePtaFilledCircle()
 *          PTA        *generatePtaLineFromPt()
 *          l_int32     locatePtRadially()
 *
 *      Pta rendering
 *
 *          l_int32     pixRenderPta()
 *          l_int32     pixRenderPtaArb()
 *          l_int32     pixRenderPtaBlend()
 *
 *      Rendering of arbitrary shapes built with lines
 *
 *          l_int32     pixRenderLine()
 *          l_int32     pixRenderLineArb()
 *          l_int32     pixRenderLineBlend()
 *
 *          l_int32     pixRenderBox()
 *          l_int32     pixRenderBoxArb()
 *          l_int32     pixRenderBoxBlend()
 *
 *          l_int32     pixRenderHashBox()
 *          l_int32     pixRenderHashBoxArb()
 *          l_int32     pixRenderHashBoxBlend()
 *
 *          l_int32     pixRenderBoxa()
 *          l_int32     pixRenderBoxaArb()
 *          l_int32     pixRenderBoxaBlend()
 *
 *          l_int32     pixRenderPolyline()
 *          l_int32     pixRenderPolylineArb()
 *          l_int32     pixRenderPolylineBlend()
 *
 *          l_int32     pixRenderRandomCmapPtaa()
 *
 *      Contour rendering on grayscale images
 *
 *          PIX        *pixRenderContours()
 *          PIX        *fpixRenderContours()
 *
 *  The line rendering functions are relatively crude, but they
 *  get the job done for most simple situations.  We use the pta
 *  as an intermediate data structure.  A pta is generated
 *  for a line.  One of two rendering functions are used to
 *  render this onto a Pix.
 */

#include <stdio.h>
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
    if (L_ABS(x2 - x1) >= L_ABS(y2 - y1)) {
        getyofx = TRUE;
        npts = L_ABS(x2 - x1) + 1;
        diff = x2 - x1;
        sign = L_SIGN(x2 - x1);
        slope = (l_float32)(sign * (y2 - y1)) / (l_float32)diff;
    }
    else {
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
    }
    else {   /* x = x(y) */
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
        L_WARNING("width < 1; setting to 1", procName);
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
            }
            else {  /* place below */
                y1a = y1 + (i + 1) / 2;
                y2a = y2 + (i + 1) / 2;
            }
            if ((pta = generatePtaLine(x1, y1a, x2, y2a)) == NULL)
                return (PTA *)ERROR_PTR("pta not made", procName, NULL);
            ptaJoin(ptaj, pta, 0, 0);
            ptaDestroy(&pta);
        }
    }
    else  {  /* "vertical" line  */
        for (i = 1; i < width; i++) {
            if ((i & 1) == 1) {   /* place to left */
                x1a = x1 - (i + 1) / 2;
                x2a = x2 - (i + 1) / 2;
            }
            else {  /* place to right */
                x1a = x1 + (i + 1) / 2;
                x2a = x2 + (i + 1) / 2;
            }
            if ((pta = generatePtaLine(x1a, y1, x2a, y2)) == NULL)
                return (PTA *)ERROR_PTR("pta not made", procName, NULL);
            ptaJoin(ptaj, pta, 0, 0);
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

        /* Generate line points and add them to the pta. */
    boxGetGeometry(box, &x, &y, &w, &h);
    if (w == 0 || h == 0)
        return (PTA *)ERROR_PTR("box has w = 0 or h = 0", procName, NULL);
    ptad = ptaCreate(0);
    if ((width & 1) == 1) {   /* odd width */
        pta = generatePtaWideLine(x - width / 2, y,
                                  x + w - 1 + width / 2, y, width);
        ptaJoin(ptad, pta, 0, 0);
        ptaDestroy(&pta);
        pta = generatePtaWideLine(x + w - 1, y + 1 + width / 2,
                                  x + w - 1, y + h - 2 - width / 2, width);
        ptaJoin(ptad, pta, 0, 0);
        ptaDestroy(&pta);
        pta = generatePtaWideLine(x + w - 1 + width / 2, y + h - 1,
                                  x - width / 2, y + h - 1, width);
        ptaJoin(ptad, pta, 0, 0);
        ptaDestroy(&pta);
        pta = generatePtaWideLine(x, y + h - 2 - width / 2,
                                  x, y + 1 + width / 2, width);
        ptaJoin(ptad, pta, 0, 0);
        ptaDestroy(&pta);
    }
    else {   /* even width */
        pta = generatePtaWideLine(x - width / 2, y,
                                  x + w - 2 + width / 2, y, width);
        ptaJoin(ptad, pta, 0, 0);
        ptaDestroy(&pta);
        pta = generatePtaWideLine(x + w - 1, y + 0 + width / 2,
                                  x + w - 1, y + h - 2 - width / 2, width);
        ptaJoin(ptad, pta, 0, 0);
        ptaDestroy(&pta);
        pta = generatePtaWideLine(x + w - 2 + width / 2, y + h - 1,
                                  x - width / 2, y + h - 1, width);
        ptaJoin(ptad, pta, 0, 0);
        ptaDestroy(&pta);
        pta = generatePtaWideLine(x, y + h - 2 - width / 2,
                                  x, y + 0 + width / 2, width);
        ptaJoin(ptad, pta, 0, 0);
        ptaDestroy(&pta);
    }

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

        /* Generate line points and add them to the pta. */
    ptad = ptaCreate(0);
    if (outline) {
        pta = generatePtaBox(box, width);
        ptaJoin(ptad, pta, 0, 0);
        ptaDestroy(&pta);
    }
    if (orient == L_HORIZONTAL_LINE) {
        n = 1 + bh / spacing;
        for (i = 0; i < n; i++) {
            y = by + (i * (bh - 1)) / (n - 1);
            pta = generatePtaWideLine(bx, y, bx + bw - 1, y, width);
            ptaJoin(ptad, pta, 0, 0);
            ptaDestroy(&pta);
	}
    }
    else if (orient == L_VERTICAL_LINE) {
        n = 1 + bw / spacing;
        for (i = 0; i < n; i++) {
            x = bx + (i * (bw - 1)) / (n - 1);
            pta = generatePtaWideLine(x, by, x, by + bh - 1, width);
            ptaJoin(ptad, pta, 0, 0);
            ptaDestroy(&pta);
	}
    }
    else if (orient == L_POS_SLOPE_LINE) {
        n = 2 + (l_int32)((bw + bh) / (1.4 * spacing));
        for (i = 0; i < n; i++) {
            x = (l_int32)(bx + (i + 0.5) * 1.4 * spacing);
            boxIntersectByLine(box, x, by - 1, 1.0, &x1, &y1, &x2, &y2, &npts);
            if (npts == 2) {
                pta = generatePtaWideLine(x1, y1, x2, y2, width);
                ptaJoin(ptad, pta, 0, 0);
                ptaDestroy(&pta);
            }
        }
    }
    else {  /* orient == L_NEG_SLOPE_LINE */
        n = 2 + (l_int32)((bw + bh) / (1.4 * spacing));
        for (i = 0; i < n; i++) {
            x = (l_int32)(bx - bh + (i + 0.5) * 1.4 * spacing);
            boxIntersectByLine(box, x, by - 1, -1.0, &x1, &y1, &x2, &y2, &npts);
            if (npts == 2) {
                pta = generatePtaWideLine(x1, y1, x2, y2, width);
                ptaJoin(ptad, pta, 0, 0);
                ptaDestroy(&pta);
            }
        }
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

    n = boxaGetCount(boxa);
    ptat = ptaCreate(0);
    for (i = 0; i < n; i++) {
        box = boxaGetBox(boxa, i, L_CLONE);
        pta = generatePtaBox(box, width);
        ptaJoin(ptat, pta, 0, 0);
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
PTAA  *
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
 *
 *  Notes:
 *      (1) If the boxa has overlapping boxes, and if blending will
 *          be used to give a transparent effect, transparency
 *          artifacts at line intersections can be removed using
 *          removedups = 1.
 */
PTA  *
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

    n = ptaGetCount(ptas);
    ptat = ptaCreate(0);
    if (n < 2)  /* nothing to do */
        return ptat;

    ptaGetIPt(ptas, 0, &x1, &y1);
    for (i = 1; i < n; i++) {
        ptaGetIPt(ptas, i, &x2, &y2);
        pta = generatePtaWideLine(x1, y1, x2, y2, width);
        ptaJoin(ptat, pta, 0, 0);
        ptaDestroy(&pta);
        x1 = x2;
        y1 = y2;
    }

    if (closeflag) {
        ptaGetIPt(ptas, 0, &x2, &y2);
        pta = generatePtaWideLine(x1, y1, x2, y2, width);
        ptaJoin(ptat, pta, 0, 0);
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
PTA  *
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
 *      Input:  pix
 *              pta (arbitrary set of points)
 *              rval, gval, bval
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If pix is colormapped, render this color on each pixel.
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
        if (pixcmapAddNewColor(cmap, rval, gval, bval, &index))
            return ERROR_INT("colormap is full", procName, 1);
    }
    else {
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
        L_WARNING("fract must be in [0.0, 1.0]; setting to 0.5", procName);
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
        L_WARNING("width must be > 0; setting to 1", procName);
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
        L_WARNING("width must be > 0; setting to 1", procName);
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
        L_WARNING("width must be > 0; setting to 1", procName);
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
 *      Input:  pix
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

    if ((pta = generatePtaBox(box, width)) == NULL)
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
 *  pixRenderBoxArb()
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

    if ((pta = generatePtaBoxa(boxa, width, removedups)) == NULL)
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
        L_WARNING("invalid outdepth; setting to 1", procName);
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
        }
        else {  /* outdepth == d */
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
        }
        else {  /* outdepth == d */
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
 *  fpixRenderContours()
 *
 *      Input:  fpixs
 *              startval (value of lowest contour
 *              incr  (increment to next contour; must be > 0.0)
 *              proxim (required proximity to target value; typ. 0.1 * incr)
 *      Return: pixd (1 bpp), or null on error
 */
PIX *
fpixRenderContours(FPIX      *fpixs,
                   l_float32  startval,
                   l_float32  incr,
                   l_float32  proxim)
{
l_int32     i, j, w, h, wpls, wpld;
l_float32   val, invincr, finter, diff;
l_uint32   *datad, *lined;
l_float32  *datas, *lines;
PIX        *pixd;

    PROCNAME("fpixRenderContours");

    if (!fpixs)
        return (PIX *)ERROR_PTR("fpixs not defined", procName, NULL);
    if (incr <= 0.0)
        return (PIX *)ERROR_PTR("incr <= 0.0", procName, NULL);

    fpixGetDimensions(fpixs, &w, &h);
    if ((pixd = pixCreate(w, h, 1)) == NULL)
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);

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
            if (val < startval)
                continue;
            finter = L_ABS(invincr * (val - startval));
            diff = finter - floorf(finter);
            if (diff <= proxim)
                SET_DATA_BIT(lined, j); 
        }
    }

    return pixd;
}


