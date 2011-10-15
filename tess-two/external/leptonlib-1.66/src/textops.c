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
 *  textops.c
 *
 *    Font layout
 *       PIX             *pixAddSingleTextblock()
 *       l_int32          pixSetTextblock()
 *       l_int32          pixSetTextline()
 *
 *    Text size estimation and partitioning
 *       SARRAY          *bmfGetLineStrings()
 *       NUMA            *bmfGetWordWidths()
 *       l_int32          bmfGetStringWidth()
 *
 *    Text splitting
 *       SARRAY          *splitStringToParagraphs()
 *       static l_int32   stringAllWhitespace() 
 *       static l_int32   stringLeadingWhitespace() 
 *
 *    This is a simple utility to put text on images.  One font and style
 *    is provided, with a variety of pt sizes.  For example, to put a
 *    line of green 10 pt text on an image, with the beginning baseline
 *    at (50, 50):
 *        L_Bmf  *bmf = bmfCreate("./fonts", 10);
 *        const char *textstr = "This is a funny cat";
 *        pixSetTextline(pixs, bmf, textstr, 0x00ff0000, 50, 50, NULL, NULL);
 *
 *    A simpler interface for adding text to an image is pixAddTextblock().
 *    See prog/writetext_reg.c for examples of its use.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

static l_int32 stringAllWhitespace(char *textstr, l_int32 *pval);
static l_int32 stringLeadingWhitespace(char *textstr, l_int32 *pval);


/*---------------------------------------------------------------------*
 *                                 Font layout                         *
 *---------------------------------------------------------------------*/
/*!
 *  pixAddSingleTextblock()
 *
 *      Input:  pixs (input pix; colormap ok)
 *              bmf (bitmap font data)
 *              textstr (<optional> text string to be added)
 *              val (color to set the text)
 *              location (L_ADD_ABOVE, L_ADD_AT_TOP, L_ADD_AT_BOTTOM,
 *                        L_ADD_BELOW)
 *              &overflow (<optional return> 1 if text overflows
 *                         allocated region and is clipped; 0 otherwise)
 *      Return: pixd (new pix with rendered text), or null on error
 *
 *  Notes:
 *      (1) This function paints a set of lines of text over an image.
 *          If @location is L_ADD_ABOVE or L_ADD_BELOW, the pix size
 *          is expanded with a border and rendered over the border.
 *      (2) @val is the pixel value to be painted through the font mask.
 *          It should be chosen to agree with the depth of pixs.
 *          For RGB, use hex notation: 0xRRGGBB00,
 *      (3) If textstr == NULL, use the text field in the pix.
 *      (4) If there is a colormap, make sure @val is a valid cmap index
 *      (5) Typical usage is for labelling a pix with some text data.
 */
PIX *
pixAddSingleTextblock(PIX         *pixs,
                      L_BMF       *bmf,
                      const char  *textstr,
                      l_uint32     val,
                      l_int32      location,
                      l_int32     *poverflow)
{
char     *linestr;
l_int32   w, h, d, i, y, xstart, ystart, extra, spacer, rval, gval, bval;
l_int32   nlines, htext, ovf, overflow, offset, usable, index;
l_uint32  textcolor;
PIX      *pixd;
PIXCMAP  *cmap, *cmapd;
SARRAY   *salines;

    PROCNAME("pixAddSingleTextblock");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
    if (!bmf)
        return (PIX *)ERROR_PTR("bmf not defined", procName, NULL);
    if (val < 0)
        return (PIX *)ERROR_PTR("val must be >= 0", procName, NULL);
    if (location != L_ADD_ABOVE && location != L_ADD_AT_TOP &&
        location != L_ADD_AT_BOTTOM && location != L_ADD_BELOW)
        return (PIX *)ERROR_PTR("invalid location", procName, NULL);

    if (!textstr)
        textstr = pixGetText(pixs);
    if (!textstr) {
        L_ERROR("no textstring defined", procName);
        return pixCopy(NULL, pixs);
    }

    if ((cmap = pixGetColormap(pixs)) != NULL) {
        extractRGBValues(val, &rval, &gval, &bval);
        pixcmapUsableColor(cmap, rval, gval, bval, &usable);
        if (!usable)
            return (PIX *)ERROR_PTR("unable to use color", procName, NULL);
    }

    pixGetDimensions(pixs, &w, &h, &d);
    if (d == 1 && val > 1)
        return (PIX *)ERROR_PTR("1 bpp: use val <= 1", procName, NULL);
    else if (d == 2 && val > 3 && !cmap)
        return (PIX *)ERROR_PTR("2 bpp: use val <= 3", procName, NULL);
    else if (d == 4 && val > 15 && !cmap)
        return (PIX *)ERROR_PTR("4 bpp: use val <= 15", procName, NULL);
    else if (d == 8 && val > 0xff && !cmap)
        return (PIX *)ERROR_PTR("8 bpp: use val <= 255", procName, NULL);
    else if (d == 16 && val > 0xffff)
        return (PIX *)ERROR_PTR("16 bpp: use val <= 0xffff", procName, NULL);
    else if (d == 32 && val < 256)
        return (PIX *)ERROR_PTR("RGB: use val > 255", procName, NULL);

    xstart = (l_int32)(0.1 * w);
    salines = bmfGetLineStrings(bmf, textstr, w - 2 * xstart, 0, &htext);
    if (!salines)
        return (PIX *)ERROR_PTR("line string sa not made", procName, NULL);
    nlines = sarrayGetCount(salines);

        /* Add white border if required */
    spacer = 10;  /* pixels away from image boundary or added border */
    if (location == L_ADD_ABOVE || location == L_ADD_BELOW) {
        extra = htext + 2 * spacer;
        pixd = pixCreate(w, h + extra, d);
        if (cmap) {
            cmapd = pixcmapCopy(cmap);
            pixSetColormap(pixd, cmapd);
        }
        pixSetBlackOrWhite(pixd, L_BRING_IN_WHITE);
        if (location == L_ADD_ABOVE)
            pixRasterop(pixd, 0, extra, w, h, PIX_SRC, pixs, 0, 0);
        else  /* add below */
            pixRasterop(pixd, 0, 0, w, h, PIX_SRC, pixs, 0, 0);
    }
    else
        pixd = pixCopy(NULL, pixs);
    cmapd = pixGetColormap(pixd);

        /* bmf->baselinetab[93] is the approximate distance from
         * the top of the tallest character to the baseline.  93 was chosen
         * at random, as all the baselines are essentially equal for
         * each character in a font. */
    offset = bmf->baselinetab[93];
    if (location == L_ADD_ABOVE || location == L_ADD_AT_TOP)
        ystart = offset + spacer;
    else if (location == L_ADD_AT_BOTTOM)
        ystart = h - htext - spacer + offset;
    else   /* add below */
        ystart = h + offset + spacer;

        /* If colormapped, use the actual rgba color */
    if (cmapd) {
        pixcmapAddNewColor(cmapd, rval, gval, bval, &index);
        composeRGBPixel(rval, gval, bval, &textcolor);
    } else
        textcolor = val;

        /* Keep track of overflow condition on line width */
    overflow = 0;
    for (i = 0, y = ystart; i < nlines; i++) {
        linestr = sarrayGetString(salines, i, 0);
        pixSetTextline(pixd, bmf, linestr, textcolor,
                       xstart, y, NULL, &ovf);
        y += bmf->lineheight + bmf->vertlinesep;
        if (ovf)
            overflow = 1;
    }

       /* Also consider vertical overflow where there is too much text to
        * fit inside the image: the cases L_ADD_AT_TOP and L_ADD_AT_BOTTOM.
        *  The text requires a total of htext + 2 * spacer vertical pixels. */
    if (location == L_ADD_AT_TOP || location == L_ADD_AT_BOTTOM) {
        if (h < htext + 2 * spacer)
            overflow = 1;
    }
    if (poverflow)
        *poverflow = overflow;

    sarrayDestroy(&salines);
    return pixd;
}


/*!
 *  pixSetTextblock()
 *
 *      Input:  pixs (input image)
 *              bmf (bitmap font data)
 *              textstr (block text string to be set)
 *              val (color to set the text)
 *              x0 (left edge for each line of text)
 *              y0 (baseline location for the first text line)
 *              wtext (max width of each line of generated text)
 *              firstindent (indentation of first line, in x-widths)
 *              &overflow (<optional return> 0 if text is contained in
 *                         input pix; 1 if it is clipped)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This function paints a set of lines of text over an image.
 *      (2) @val is the pixel value to be painted through the font mask.
 *          For RGB, it is easiest to use hex notation: 0xRRGGBB00,
 *          where RR is the hex representation of the red intensity, etc.
 *          The last two hex digits are 00 (byte value 0), assigned to
 *          the A component.  Note that, as usual, RGBA proceeds from 
 *          left to right in the order from MSB to LSB (see pix.h
 *          for details).
 *      (3) @val should be chosen to agree with the depth of pixs.
 *          For example, if pixs has 8 bpp, val should be some value
 *          between 0 (black) and 255 (white).
 */
l_int32
pixSetTextblock(PIX         *pixs,
                L_BMF       *bmf,
                const char  *textstr,
                l_uint32     val,
                l_int32      x0,
                l_int32      y0,
                l_int32      wtext,
                l_int32      firstindent,
                l_int32     *poverflow)
{
char     *linestr;
l_int32   d, h, i, w, x, y, nlines, htext, xwidth, wline, ovf, overflow;
SARRAY   *salines;
PIXCMAP  *cmap;

    PROCNAME("pixSetTextblock");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!bmf)
        return ERROR_INT("bmf not defined", procName, 1);
    if (!textstr)
        return ERROR_INT("textstr not defined", procName, 1);
    if (val < 0)
        return ERROR_INT("val must be >= 0", procName, 1);

    pixGetDimensions(pixs, &w, &h, &d);
    cmap = pixGetColormap(pixs);
    if (d == 2 && val > 0x03 && !cmap)
        return ERROR_INT("for 2 bpp, val must be < 4", procName, 1);
    if (d == 4 && val > 0x0f && !cmap)
        return ERROR_INT("for 4 bpp, val must be < 16", procName, 1);
    if (d == 8 && val > 0xff && !cmap)
        return ERROR_INT("for 8 bpp, val must be < 256", procName, 1);
    if (d == 16 && val > 0xffff)
        return ERROR_INT("for 16 bpp, val must be <= 0xffff", procName, 1);
    if (d == 32 && val < 256)
        return ERROR_INT("for RGB, val must be >= 256", procName, 1);

    if (w < x0 + wtext) {
        L_WARNING("reducing width of textblock", procName);
        wtext = w - x0 - w / 10;
        if (wtext <= 0)
            return ERROR_INT("wtext too small; no room for text", procName, 1);
    }

    salines = bmfGetLineStrings(bmf, textstr, wtext, firstindent, &htext);
    if (!salines)
        return ERROR_INT("line string sa not made", procName, 1);
    nlines = sarrayGetCount(salines);
    bmfGetWidth(bmf, 'x', &xwidth);

    y = y0;
    overflow = 0;
    for (i = 0; i < nlines; i++) {
        if (i == 0)
            x = x0 + firstindent * xwidth;
        else
            x = x0;
        linestr = sarrayGetString(salines, i, 0);
        pixSetTextline(pixs, bmf, linestr, val, x, y, &wline, &ovf);
        y += bmf->lineheight + bmf->vertlinesep;
        if (ovf)
            overflow = 1;
    }

       /* (y0 - baseline) is the top of the printed text.  Character
        * 93 was chosen at random, as all the baselines are essentially
        * equal for each character in a font. */
    if (h < y0 - bmf->baselinetab[93] + htext)
        overflow = 1;
    if (poverflow)
        *poverflow = overflow;

    sarrayDestroy(&salines);
    return 0;
}


/*!
 *  pixSetTextline()
 *
 *      Input:  pixs (input image)
 *              bmf (bitmap font data)
 *              textstr (text string to be set on the line)
 *              val (color to set the text)
 *              x0 (left edge for first char)
 *              y0 (baseline location for all text on line)
 *              &width (<optional return> width of generated text)
 *              &overflow (<optional return> 0 if text is contained in
 *                         input pix; 1 if it is clipped)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This function paints a line of text over an image.
 *      (2) @val is the pixel value to be painted through the font mask.
 *          For RGB, it is easiest to use hex notation: 0xRRGGBB00,
 *          where RR is the hex representation of the red intensity, etc.
 *          The last two hex digits are 00 (byte value 0), assigned to
 *          the A component.  Note that, as usual, RGBA proceeds from 
 *          left to right in the order from MSB to LSB (see pix.h
 *          for details).
 *      (3) @val should be chosen to agree with the depth of pixs.
 *          For example, if pixs has 8 bpp, val should be some value
 *          between 0 (black) and 255 (white).
 */
l_int32
pixSetTextline(PIX         *pixs,
               L_BMF       *bmf,
               const char  *textstr,
               l_uint32     val,
               l_int32      x0,
               l_int32      y0,
               l_int32     *pwidth,
               l_int32     *poverflow)
{
char      chr; 
l_int32   d, i, x, w, nchar, baseline;
PIX      *pix;
PIXCMAP  *cmap;

    PROCNAME("pixSetTextline");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!bmf)
        return ERROR_INT("bmf not defined", procName, 1);
    if (!textstr)
        return ERROR_INT("teststr not defined", procName, 1);
    if (val < 0) {
        L_WARNING("val must be non-negative; setting to 0", procName);
        val = 0;
    }

    d = pixGetDepth(pixs);
    cmap = pixGetColormap(pixs);
    if (d == 2 && val > 0x03 && !cmap)
        return ERROR_INT("for 2 bpp, val must be < 4", procName, 1);
    if (d == 4 && val > 0x0f && !cmap)
        return ERROR_INT("for 4 bpp, val must be < 16", procName, 1);
    if (d == 8 && val > 0xff && !cmap)
        return ERROR_INT("for 8 bpp, val must be < 256", procName, 1);
    if (d == 16 && val > 0xffff)
        return ERROR_INT("for 16 bpp, val must be <= 0xffff", procName, 1);
    if (d == 32 && val < 256)
        return ERROR_INT("for RGB, val must be >= 256", procName, 1);

    nchar = strlen(textstr);
    x = x0;
    for (i = 0; i < nchar; i++) {
        chr = textstr[i];
        pix = bmfGetPix(bmf, chr);
        bmfGetBaseline(bmf, chr, &baseline);
        pixPaintThroughMask(pixs, pix, x, y0 - baseline, val);
        w = pixGetWidth(pix);
        x += w + bmf->kernwidth;
        pixDestroy(&pix);
    }

    if (pwidth)
        *pwidth = x - bmf->kernwidth - x0;
    if (poverflow)
        *poverflow = (x > pixGetWidth(pixs) - 1) ? 1 : 0;
    return 0;
}


/*---------------------------------------------------------------------*
 *                   Text size estimation and partitioning             *
 *---------------------------------------------------------------------*/
/*!
 *  bmfGetLineStrings()
 *
 *      Input:  bmf
 *              textstr
 *              maxw (max width of a text line in pixels)
 *              firstindent (indentation of first line, in x-widths)
 *              &h (<return> height required to hold text bitmap)
 *      Return: sarray of text strings for each line, or null on error
 *
 *  Notes:
 *      (1) Divides the input text string into an array of text strings,
 *          each of which will fit within maxw bits of width.
 */
SARRAY *
bmfGetLineStrings(L_BMF       *bmf,
                  const char  *textstr,
                  l_int32      maxw,
                  l_int32      firstindent,
                  l_int32     *ph)
{
char    *linestr;
l_int32  i, ifirst, sumw, newsum, w, nwords, nlines, len, xwidth;
NUMA    *na;
SARRAY  *sa, *sawords;

    PROCNAME("bmfGetLineStrings");

    if (!bmf)
        return (SARRAY *)ERROR_PTR("bmf not defined", procName, NULL);
    if (!textstr)
        return (SARRAY *)ERROR_PTR("teststr not defined", procName, NULL);

    if ((sawords = sarrayCreateWordsFromString(textstr)) == NULL)
        return (SARRAY *)ERROR_PTR("sawords not made", procName, NULL);

    if ((na = bmfGetWordWidths(bmf, textstr, sawords)) == NULL)
        return (SARRAY *)ERROR_PTR("na not made", procName, NULL);
    nwords = numaGetCount(na);
    if (nwords == 0)
        return (SARRAY *)ERROR_PTR("no words in textstr", procName, NULL);
    bmfGetWidth(bmf, 'x', &xwidth);

    if ((sa = sarrayCreate(0)) == NULL)
        return (SARRAY *)ERROR_PTR("sa not made", procName, NULL);

    ifirst = 0;
    numaGetIValue(na, 0, &w);
    sumw = firstindent * xwidth + w;
    for (i = 1; i < nwords; i++) {
        numaGetIValue(na, i, &w);
        newsum = sumw + bmf->spacewidth + w;
        if (newsum > maxw) {
            linestr = sarrayToStringRange(sawords, ifirst, i - ifirst, 2);
            if (!linestr)
                continue;
            len = strlen(linestr);
            if (len > 0)  /* it should always be */
                linestr[len - 1] = '\0';  /* remove the last space */
            sarrayAddString(sa, linestr, 0);
            ifirst = i;
            sumw = w;
        }
        else
            sumw += bmf->spacewidth + w;
    }
    linestr = sarrayToStringRange(sawords, ifirst, nwords - ifirst, 2);
    if (linestr)
        sarrayAddString(sa, linestr, 0);
    nlines = sarrayGetCount(sa);
    *ph = nlines * bmf->lineheight + (nlines - 1) * bmf->vertlinesep;

    sarrayDestroy(&sawords);
    numaDestroy(&na);
    return sa;
}


/*!
 *  bmfGetWordWidths()
 *
 *      Input:  bmf
 *              textstr
 *              sa (of individual words)
 *      Return: numa (of word lengths in pixels for the font represented
 *                    by the bmf), or null on error
 */
NUMA *
bmfGetWordWidths(L_BMF       *bmf,
                 const char  *textstr,
                 SARRAY      *sa)
{
char    *wordstr;
l_int32  i, nwords, width;
NUMA    *na;

    PROCNAME("bmfGetWordWidths");

    if (!bmf)
        return (NUMA *)ERROR_PTR("bmf not defined", procName, NULL);
    if (!textstr)
        return (NUMA *)ERROR_PTR("teststr not defined", procName, NULL);
    if (!sa)
        return (NUMA *)ERROR_PTR("sa not defined", procName, NULL);

    nwords = sarrayGetCount(sa);
    if ((na = numaCreate(nwords)) == NULL)
        return (NUMA *)ERROR_PTR("na not made", procName, NULL);

    for (i = 0; i < nwords; i++) {
        wordstr = sarrayGetString(sa, i, 0);  /* not a copy */
        bmfGetStringWidth(bmf, wordstr, &width);
        numaAddNumber(na, width);
    }

    return na;
}


/*!
 *  bmfGetStringWidth()
 *
 *      Input:  bmf
 *              textstr
 *              &w (<return> width of text string, in pixels for the
 *                 font represented by the bmf)
 *      Return: 0 if OK, 1 on error
 */
l_int32
bmfGetStringWidth(L_BMF       *bmf,
                  const char  *textstr,
                  l_int32     *pw)
{
char     chr; 
l_int32  i, w, width, nchar;

    PROCNAME("bmfGetStringWidth");

    if (!bmf)
        return ERROR_INT("bmf not defined", procName, 1);
    if (!textstr)
        return ERROR_INT("teststr not defined", procName, 1);
    if (!pw)
        return ERROR_INT("&w not defined", procName, 1);

    nchar = strlen(textstr);
    w = 0;
    for (i = 0; i < nchar; i++) {
        chr = textstr[i];
        bmfGetWidth(bmf, chr, &width);
        if (width != UNDEF)
            w += width + bmf->kernwidth;
    }
    w -= bmf->kernwidth;  /* remove last one */

    *pw = w;
    return 0;
}



/*---------------------------------------------------------------------*
 *                             Text splitting                          *
 *---------------------------------------------------------------------*/
/*!
 *  splitStringToParagraphs()
 *
 *      Input:  textstring
 *              splitting flag (see enum in bmf.h; valid values in {1,2,3})
 *      Return: sarray (where each string is a paragraph of the input),
 *                      or null on error.
 */
SARRAY *
splitStringToParagraphs(char    *textstr,
                        l_int32  splitflag)
{
char    *linestr, *parastring;
l_int32  nlines, i, allwhite, leadwhite;
SARRAY  *salines, *satemp, *saout;

    PROCNAME("splitStringToParagraphs");

    if (!textstr)
        return (SARRAY *)ERROR_PTR("textstr not defined", procName, NULL);

    if ((salines = sarrayCreateLinesFromString(textstr, 1)) == NULL)
        return (SARRAY *)ERROR_PTR("salines not made", procName, NULL);
    nlines = sarrayGetCount(salines);
    saout = sarrayCreate(0);
    satemp = sarrayCreate(0);

    linestr = sarrayGetString(salines, 0, 0);
    sarrayAddString(satemp, linestr, 1);
    for (i = 1; i < nlines; i++) {
        linestr = sarrayGetString(salines, i, 0);
        stringAllWhitespace(linestr, &allwhite);
        stringLeadingWhitespace(linestr, &leadwhite);
        if ((splitflag == SPLIT_ON_LEADING_WHITE && leadwhite) ||
            (splitflag == SPLIT_ON_BLANK_LINE && allwhite) ||
            (splitflag == SPLIT_ON_BOTH && (allwhite || leadwhite))) {
            parastring = sarrayToString(satemp, 1);  /* add nl to each line */
            sarrayAddString(saout, parastring, 0);  /* insert */
            sarrayDestroy(&satemp);
            satemp = sarrayCreate(0);
        }
        sarrayAddString(satemp, linestr, 1);
    }
    parastring = sarrayToString(satemp, 1);  /* add nl to each line */
    sarrayAddString(saout, parastring, 0);  /* insert */
    sarrayDestroy(&satemp);

    return saout;
}


/*!
 *  stringAllWhitespace()
 *
 *      Input:  textstring
 *              &val (<return> 1 if all whitespace; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 */
static l_int32
stringAllWhitespace(char     *textstr,
                    l_int32  *pval)
{
l_int32  len, i;

    PROCNAME("stringAllWhitespace");

    if (!textstr)
        return ERROR_INT("textstr not defined", procName, 1);
    if (!pval)
        return ERROR_INT("&va not defined", procName, 1);

    len = strlen(textstr);
    *pval = 1;
    for (i = 0; i < len; i++) {
        if (textstr[i] != ' ' && textstr[i] != '\t' && textstr[i] != '\n') {
            *pval = 0;
            return 0;
        }
    }
    return 0;
}


/*!
 *  stringLeadingWhitespace()
 *
 *      Input:  textstring
 *              &val (<return> 1 if leading char is ' ' or '\t'; 0 otherwise)
 *      Return: 0 if OK, 1 on error
 */
static l_int32
stringLeadingWhitespace(char     *textstr,
                        l_int32  *pval)
{
    PROCNAME("stringLeadingWhitespace");

    if (!textstr)
        return ERROR_INT("textstr not defined", procName, 1);
    if (!pval)
        return ERROR_INT("&va not defined", procName, 1);

    *pval = 0;
    if (textstr[0] == ' ' || textstr[0] == '\t')
        *pval = 1;

    return 0;
}


