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
 *  bmf.c
 *
 *   Acquisition and generation of bitmap fonts.
 *
 *       L_BMF           *bmfCreate()
 *       L_BMF           *bmfDestroy()
 *
 *       PIX             *bmfGetPix()
 *       l_int32          bmfGetWidth()
 *       l_int32          bmfGetBaseline()
 *
 *       PIXA            *pixaGetFont()
 *       l_int32          pixaSaveFont()
 *       PIXA            *pixaGenerateFont()
 *       static l_int32   pixGetTextBaseline()
 *       static l_int32   bmfMakeAsciiTables()
 *
 *   This is not a very general utility, because it only uses bitmap
 *   representations of a single font, Palatino-Roman, with the
 *   normal style.  It uses bitmaps generated for nine sizes, from
 *   4 to 20 pts, rendered at 300 ppi.  Generalization to different
 *   fonts, styles and sizes is straightforward.
 *
 *   I chose Palatino-Roman is because I like it.
 *   The input font images were generated from a set of small
 *   PostScript files, such as chars-12.ps, which were rendered
 *   into the inputfont[] bitmap files using GhostScript.  See, for
 *   example, the bash script prog/ps2tiff, which will "rip" a
 *   PostScript file into a set of ccitt-g4 compressed tiff files.
 *
 *   The set of ascii characters from 32 through 126 are the 95
 *   printable ascii chars.  Palatino-Roman is missing char 92, '\'.
 *   I have substituted '/', char 47, for 92, so that there will be
 *   no missing printable chars in this set.  The space is char 32,
 *   and I have given it a width equal to twice the width of '!'.
 */

#include "allheaders.h"

#define  NFONTS  9
static const char  *inputfonts[] = {"chars-4.tif", "chars-6.tif",
                                    "chars-8.tif", "chars-10.tif",
                                    "chars-12.tif", "chars-14.tif",
                                    "chars-16.tif", "chars-18.tif",
                                    "chars-20.tif"};
static const char  *outputfonts[] = {"chars-4.pixa", "chars-6.pixa",
                                     "chars-8.pixa", "chars-10.pixa",
                                     "chars-12.pixa", "chars-14.pixa",
                                     "chars-16.pixa", "chars-18.pixa",
                                     "chars-20.pixa"};
static const l_int32 baselines[NFONTS][3] = {{11, 12, 12}, {18, 18, 18},
                                             {24, 24, 24}, {30, 30, 30},
                                             {36, 36, 36}, {42, 42, 42},
                                             {48, 48, 48}, {54, 54, 54},
                                             {60, 60, 60}};
static const l_float32  VERT_FRACT_SEP = 0.3;

#ifndef  NO_CONSOLE_IO
#define  DEBUG_BASELINE     0
#define  DEBUG_CHARS        0
#define  DEBUG_FONT_GEN     0
#endif  /* ~NO_CONSOLE_IO */

static l_int32 pixGetTextBaseline(PIX *pixs, l_int32 *tab8, l_int32 *py);
static l_int32 bmfMakeAsciiTables(L_BMF *bmf);


/*---------------------------------------------------------------------*/
/*                           Bmf create/destroy                        */
/*---------------------------------------------------------------------*/
/*!
 *  bmfCreate()
 *
 *      Input:  dir (directory holding pixa of character set)
 *              size (4, 6, 8, ... , 20)
 *      Return: bmf (holding the bitmap font and associated information)
 *
 *  Notes:
 *      (1) This first tries to read a pre-computed pixa file with the
 *          95 ascii chars in it.  If the file is not found, it
 *          creates the pixa from the raw image.  It then generates all 
 *          associated data required to use the bmf.
 */
L_BMF *
bmfCreate(const char  *dir,
          l_int32      size)
{
L_BMF   *bmf;
PIXA  *pixa;

    PROCNAME("bmfCreate");

    if ((bmf = (L_BMF *)CALLOC(1, sizeof(L_BMF))) == NULL)
        return (L_BMF *)ERROR_PTR("bmf not made", procName, NULL);

        /* Look for the pixa */
    pixa = pixaGetFont(dir, size, &bmf->baseline1, &bmf->baseline2,
                       &bmf->baseline3);

        /* If not found, make it */
    if (!pixa) {
        L_INFO("Generating pixa of bitmap fonts", procName);
        pixa = pixaGenerateFont(dir, size, &bmf->baseline1, &bmf->baseline2,
                                &bmf->baseline3);
        if (!pixa) {
            bmfDestroy(&bmf);
            return (L_BMF *)ERROR_PTR("font pixa not made", procName, NULL);
        }
    }

    bmf->pixa = pixa;
    bmf->size = size;
    bmf->directory = stringNew(dir);
    bmfMakeAsciiTables(bmf);
    return bmf;
}


/*!
 *  bmfDestroy()
 *
 *      Input:  &bmf (<set to null>)
 *      Return: void
 */
void
bmfDestroy(L_BMF  **pbmf)
{
L_BMF  *bmf;

    PROCNAME("bmfDestroy");

    if (pbmf == NULL) {
        L_WARNING("ptr address is null!", procName);
        return;
    }

    if ((bmf = *pbmf) == NULL)
        return;

    pixaDestroy(&bmf->pixa);
    FREE(bmf->directory);
    FREE(bmf->fonttab);
    FREE(bmf->baselinetab);
    FREE(bmf->widthtab);
    FREE(bmf);
    *pbmf = NULL;
    return;
}


/*---------------------------------------------------------------------*/
/*                             Bmf accessors                           */
/*---------------------------------------------------------------------*/
/*!
 *  bmfGetPix()
 *
 *      Input:  bmf
 *              chr (should be one of the 95 supported printable bitmaps)
 *      Return: pix (clone of pix in bmf), or null on error
 */
PIX *
bmfGetPix(L_BMF  *bmf,
          char    chr)
{
l_int32  i, index;
PIXA    *pixa;

    PROCNAME("bmfGetPix");

    if ((index = (l_int32)chr) == 10)  /* NL */
        return NULL;
    if (!bmf)
        return (PIX *)ERROR_PTR("bmf not defined", procName, NULL);

    i = bmf->fonttab[index];
    if (i == UNDEF) {
        L_ERROR_INT("no bitmap representation for %d", procName, index);
        return NULL;
    }

    if ((pixa = bmf->pixa) == NULL)
        return (PIX *)ERROR_PTR("pixa not found", procName, NULL);
    return pixaGetPix(pixa, i, L_CLONE);
}


/*!
 *  bmfGetWidth()
 *
 *      Input:  bmf
 *              chr (should be one of the 95 supported bitmaps)
 *              &w (<return> character width; -1 if not printable)
 *      Return: 0 if OK, 1 on error
 */
l_int32
bmfGetWidth(L_BMF    *bmf,
            char      chr,
            l_int32  *pw)
{
l_int32  i, index;
PIX     *pix;
PIXA    *pixa;

    PROCNAME("bmfGetWidth");

    if (!pw)
        return ERROR_INT("&w not defined", procName, 1);
    *pw = -1;
    if (!bmf)
        return ERROR_INT("bmf not defined", procName, 1);
    if ((index = (l_int32)chr) == 10)  /* NL */
        return 0;

    i = bmf->fonttab[index];
    if (i == UNDEF) {
        L_ERROR_INT("no bitmap representation for %d", procName, index);
        return 1;
    }

    if ((pixa = bmf->pixa) == NULL)
        return ERROR_INT("pixa not found", procName, 1);
    if ((pix = pixaGetPix(pixa, i, L_CLONE)) == NULL)
        return ERROR_INT("pix not found", procName, 1);
    *pw = pixGetWidth(pix);
    pixDestroy(&pix);

    return 0;
}


/*!
 *  bmfGetBaseline()
 *
 *      Input:  bmf
 *              chr (should be one of the 95 supported bitmaps)
 *              &baseline (<return>; distance below UL corner of bitmap char)
 *      Return: 0 if OK, 1 on error
 */
l_int32
bmfGetBaseline(L_BMF    *bmf,
               char      chr,
               l_int32  *pbaseline)
{
l_int32  bl, index;

    PROCNAME("bmfGetBaseline");

    if (!pbaseline)
        return ERROR_INT("&baseline not defined", procName, 1);
    *pbaseline = 0;
    if (!bmf)
        return ERROR_INT("bmf not defined", procName, 1);
    if ((index = (l_int32)chr) == 10)  /* NL */
        return 0;

    bl = bmf->baselinetab[index];
    if (bl == UNDEF) {
        L_ERROR_INT("no bitmap representation for %d", procName, index);
        return 1;
    }

    *pbaseline = bl;
    return 0;
}


/*---------------------------------------------------------------------*/
/*               Font bitmap acquisition and generation                */
/*---------------------------------------------------------------------*/
/*!
 *  pixaGetFont()
 *
 *      Input:  dir (directory holding pixa of character set)
 *              size (4, 6, 8, ... , 20)
 *              &bl1 (<return> baseline of row 1)
 *              &bl2 (<return> baseline of row 2)
 *              &bl3 (<return> baseline of row 3)
 *      Return: pixa of font bitmaps for 95 characters, or null on error
 *
 *  Notes:
 *      (1) This reads a pre-computed pixa file with the 95 ascii chars.
 */
PIXA *
pixaGetFont(const char  *dir,
            l_int32      size,
            l_int32     *pbl0,
            l_int32     *pbl1,
            l_int32     *pbl2)
{
char     *pathname;
l_int32   fileno;
PIXA     *pixa;

    PROCNAME("pixaGetFont");

    fileno = (size / 2) - 2;
    if (fileno < 0 || fileno > NFONTS)
        return (PIXA *)ERROR_PTR("font size invalid", procName, NULL);
    if (!pbl0 || !pbl1 || !pbl2)
        return (PIXA *)ERROR_PTR("&bl not all defined", procName, NULL);
    *pbl0 = baselines[fileno][0];
    *pbl1 = baselines[fileno][1];
    *pbl2 = baselines[fileno][2];

    pathname = genPathname(dir, outputfonts[fileno]);
    pixa = pixaRead(pathname);
    FREE(pathname);

    if (!pixa)
        L_WARNING("pixa of char bitmaps not found", procName);
    return pixa;
}


/*!
 *  pixaSaveFont()
 *
 *      Input:  indir (directory holding image of character set)
 *              outdir (directory into which the output pixa file
 *                      will be written)
 *              size (in pts, at 300 ppi)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This saves a font of a particular size.
 *      (2) prog/genfonts calls this function for each of the
 *          nine font sizes, to generate all the font pixa files.
 */
l_int32
pixaSaveFont(const char  *indir,
             const char  *outdir,
             l_int32      size)
{
char    *pathname;
l_int32  bl1, bl2, bl3;
PIXA    *pixa;

    PROCNAME("pixaSaveFont");

    if (size < 4 || size > 20 || (size % 2))
        return ERROR_INT("size must be in {4, 6, ..., 20}", procName, 1);

    if ((pixa = pixaGenerateFont(indir, size, &bl1, &bl2, &bl3)) == NULL)
        return ERROR_INT("pixa not made", procName, 1);
    pathname = genPathname(outdir, outputfonts[(size - 4) / 2]);
    pixaWrite(pathname, pixa);

#if  DEBUG_FONT_GEN
    fprintf(stderr, "Found %d chars in font size %d\n",
            pixaGetCount(pixa), size);
    fprintf(stderr, "Baselines are at: %d, %d, %d\n", bl1, bl2, bl3);
#endif  /* DEBUG_FONT_GEN */

    FREE(pathname);
    pixaDestroy(&pixa);
    return 0;
}


/*!
 *  pixaGenerateFont()
 *
 *      Input:  dir (directory holding image of character set)
 *              size (4, 6, 8, ... , 20, in pts at 300 ppi)
 *              &bl1 (<return> baseline of row 1)
 *              &bl2 (<return> baseline of row 2)
 *              &bl3 (<return> baseline of row 3)
 *      Return: pixa of font bitmaps for 95 characters, or null on error
 *
 *  These font generation functions use 9 sets, each with bitmaps
 *  of 94 ascii characters, all in Palatino-Roman font.
 *  Each input bitmap has 3 rows of characters.  The range of
 *  ascii values in each row is as follows:
 *    row 0:  32-57   (32 is a space)
 *    row 1:  58-91   (92, '\', is not represented in this font)
 *    row 2:  93-126 
 *  We LR flip the '/' char to generate a bitmap for the missing
 *  '\' character, so that we have representations of all 95
 *  printable chars.
 *
 *  Computation of the bitmaps and baselines for a single
 *  font takes from 40 to 200 msec on a 2 GHz processor,
 *  depending on the size.  Use pixaGetFont() to read the
 *  generated character set directly from files that were
 *  produced in prog/genfonts.c using this function.
 */
PIXA *
pixaGenerateFont(const char  *dir,
                 l_int32      size,
                 l_int32     *pbl0,
                 l_int32     *pbl1,
                 l_int32     *pbl2)
{
char     *pathname;
l_int32   fileno;
l_int32   i, j, nrows, nrowchars, nchars, h, yval;
l_int32   width, height;
l_int32   baseline[3];
l_int32  *tab;
BOX      *box, *box1, *box2;
BOXA     *boxar, *boxac, *boxacs;
PIX      *pixs, *pixt1, *pixt2, *pixt3;
PIX      *pixr, *pixrc, *pixc;
PIXA     *pixa;

    PROCNAME("pixaGenerateFont");

    if (!pbl0 || !pbl1 || !pbl2)
        return (PIXA *)ERROR_PTR("&bl not all defined", procName, NULL);
    *pbl0 = *pbl1 = *pbl2 = 0;

    fileno = (size / 2) - 2;
    if (fileno < 0 || fileno > NFONTS)
        return (PIXA *)ERROR_PTR("font size invalid", procName, NULL);
    tab = makePixelSumTab8();
    pathname = genPathname(dir, inputfonts[fileno]);
    if ((pixs = pixRead(pathname)) == NULL)
        return (PIXA *)ERROR_PTR("pixs not all defined", procName, NULL);
    FREE(pathname);

    pixa = pixaCreate(95);
    pixt1 = pixMorphSequence(pixs, "c1.35 + c101.1", 0);
    boxar = pixConnComp(pixt1, NULL, 8);  /* one box for each row */
    pixDestroy(&pixt1);
    nrows = boxaGetCount(boxar);
#if  DEBUG_FONT_GEN
    fprintf(stderr, "For font %s, number of rows is %d\n",
            inputfonts[fileno], nrows);
#endif  /* DEBUG_FONT_GEN */
    if (nrows != 3) {
        L_INFO_INT2("nrows = %d; skipping font %d", procName, nrows, fileno);
        return (PIXA *)ERROR_PTR("3 rows not generated", procName, NULL);
    }
    for (i = 0; i < nrows; i++) {
        box = boxaGetBox(boxar, i, L_CLONE);
        pixr = pixClipRectangle(pixs, box, NULL);  /* row of chars */
        pixGetTextBaseline(pixr, tab, &yval);
        baseline[i] = yval;

#if DEBUG_BASELINE
      { PIX *pixbl;
        fprintf(stderr, "row %d, yval = %d, h = %d\n",
                i, yval, pixGetHeight(pixr));
        pixbl = pixCopy(NULL, pixr);
        pixRenderLine(pixbl, 0, yval, pixGetWidth(pixbl), yval, 1,
                      L_FLIP_PIXELS);
        if (i == 0 )
            pixWrite("junktl0", pixbl, IFF_PNG);
        else if (i == 1)
            pixWrite("junktl1", pixbl, IFF_PNG);
        else
            pixWrite("junktl2", pixbl, IFF_PNG);
        pixDestroy(&pixbl);
      }
#endif  /* DEBUG_BASELINE */

        boxDestroy(&box);
        pixrc = pixCloseSafeBrick(NULL, pixr, 1, 35);
        boxac = pixConnComp(pixrc, NULL, 8);
        boxacs = boxaSort(boxac, L_SORT_BY_X, L_SORT_INCREASING, NULL);
        if (i == 0) {  /* consolidate the two components of '"' */
            box1 = boxaGetBox(boxacs, 1, L_CLONE);
            box2 = boxaGetBox(boxacs, 2, L_CLONE);
            box1->w = box2->x + box2->w - box1->x;  /* increase width */
            boxDestroy(&box1);
            boxDestroy(&box2);
            boxaRemoveBox(boxacs, 2);
        }
        h = pixGetHeight(pixr);
        nrowchars = boxaGetCount(boxacs);
        for (j = 0; j < nrowchars; j++) {
            box = boxaGetBox(boxacs, j, L_COPY);
            if (box->w <= 2 && box->h == 1) {  /* skip 1x1, 2x1 components */
                boxDestroy(&box);
                continue;
            }
            box->y = 0;
            box->h = h - 1;
            pixc = pixClipRectangle(pixr, box, NULL);
            boxDestroy(&box);
            if (i == 0 && j == 0)  /* add a pix for the space; change later */
                pixaAddPix(pixa, pixc, L_COPY);
            if (i == 2 && j == 0)  /* add a pix for the '\'; change later */
                pixaAddPix(pixa, pixc, L_COPY);
            pixaAddPix(pixa, pixc, L_INSERT);
        }
        pixDestroy(&pixr);
        pixDestroy(&pixrc);
        boxaDestroy(&boxac);
        boxaDestroy(&boxacs);
    }

    nchars = pixaGetCount(pixa);
    if (nchars != 95)
        return (PIXA *)ERROR_PTR("95 chars not generated", procName, NULL);

    *pbl0 = baseline[0];
    *pbl1 = baseline[1];
    *pbl2 = baseline[2];
        
        /* Fix the space character up; it should have no ON pixels,
         * and be about twice as wide as the '!' character.    */
    pixt2 = pixaGetPix(pixa, 0, L_CLONE);
    width = 2 * pixGetWidth(pixt2);
    height = pixGetHeight(pixt2);
    pixDestroy(&pixt2);
    pixt2 = pixCreate(width, height, 1);
    pixaReplacePix(pixa, 0, pixt2, NULL);

        /* Fix up the '\' character; use a LR flip of the '/' char */
    pixt2 = pixaGetPix(pixa, 15, L_CLONE);
    pixt3 = pixFlipLR(NULL, pixt2);
    pixDestroy(&pixt2);
    pixaReplacePix(pixa, 60, pixt3, NULL);
    
#if DEBUG_CHARS
  { PIX *pixd;
    pixd = pixaDisplayTiled(pixa, 1500, 0, 10);
    pixDisplay(pixd, 100 * i, 200);
    pixDestroy(&pixd);
  }
#endif  /* DEBUG_CHARS */

    pixDestroy(&pixs);
    boxaDestroy(&boxar);
    FREE(tab);

    return pixa;
}


/*!
 *  pixGetTextBaseline()
 *
 *      Input:  pixs (1 bpp, one textline character set)
 *              tab8 (<optional> pixel sum table)
 *              &y   (<return> baseline value)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Method: find the largest difference in pixel sums from one
 *          raster line to the next one below it.  The baseline is the
 *          upper raster line for the pair of raster lines that
 *          maximizes this function.
 */
static l_int32
pixGetTextBaseline(PIX      *pixs,
                   l_int32  *tab8,
                   l_int32  *py)
{
l_int32   i, h, val1, val2, diff, diffmax, ymax;
l_int32  *tab;
NUMA     *na;

    PROCNAME("pixGetTextBaseline");

    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);
    if (!py)
        return ERROR_INT("&y not defined", procName, 1);
    *py = 0;
    if (!tab8)
        tab = makePixelSumTab8();
    else
        tab = tab8;

    na = pixCountPixelsByRow(pixs, tab);
    h = numaGetCount(na);
    diffmax = 0;
    ymax = 0;
    for (i = 1; i < h; i++) {
        numaGetIValue(na, i - 1, &val1);
        numaGetIValue(na, i, &val2);
        diff = L_MAX(0, val1 - val2);
        if (diff > diffmax) {
            diffmax = diff;
            ymax = i - 1;  /* upper raster line */
        }
    }
    *py = ymax;

    if (!tab8)
        FREE(tab);
    numaDestroy(&na);
    return 0;
}


/*!
 *  bmfMakeAsciiTables
 *
 *      Input:  bmf
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This makes three tables, each of size 128, as follows:
 *          - fonttab is a table containing the index of the Pix
 *            that corresponds to each input ascii character;
 *            it maps (ascii-index) --> Pixa index
 *          - baselinetab is a table containing the baseline offset
 *            for the Pix that corresponds to each input ascii character;
 *            it maps (ascii-index) --> baseline offset
 *          - widthtab is a table containing the character width in
 *            pixels for the Pix that corresponds to that character;
 *            it maps (ascii-index) --> bitmap width
 *     (2) This also computes
 *          - lineheight (sum of maximum character extensions above and
 *                        below the baseline)
 *          - kernwidth (spacing between characters within a word)
 *          - spacewidth (space between words)
 *          - vertlinesep (extra vertical spacing between textlines)
 *     (3) The baselines apply as follows:
 *          baseline1   (ascii 32 - 57), ascii 92
 *          baseline2   (ascii 58 - 91)
 *          baseline3   (ascii 93 - 126)
 *     (4) The only array in bmf that is not ascii-based is the
 *         array of bitmaps in the pixa, which starts at ascii 32.
 */
static l_int32
bmfMakeAsciiTables(L_BMF  *bmf)
{
l_int32   i, maxh, height, charwidth, xwidth, kernwidth;
l_int32  *fonttab, *baselinetab, *widthtab;
PIX      *pix;

    PROCNAME("bmfMakeAsciiTables");

    if (!bmf)
        return ERROR_INT("bmf not defined", procName, 1);

        /* First get the fonttab; we use this later for the char widths */
    if ((fonttab = (l_int32 *)CALLOC(128, sizeof(l_int32))) == NULL)
        return ERROR_INT("fonttab not made", procName, 1);
    bmf->fonttab = fonttab;        
    for (i = 0; i < 128; i++)
        fonttab[i] = UNDEF;
    for (i = 32; i < 127; i++)
        fonttab[i] = i - 32;

    if ((baselinetab = (l_int32 *)CALLOC(128, sizeof(l_int32))) == NULL)
        return ERROR_INT("baselinetab not made", procName, 1);
    bmf->baselinetab = baselinetab;        
    for (i = 0; i < 128; i++)
        baselinetab[i] = UNDEF;
    for (i = 32; i <= 57; i++)
        baselinetab[i] = bmf->baseline1;
    for (i = 58; i <= 91; i++)
        baselinetab[i] = bmf->baseline2;
    baselinetab[92] = bmf->baseline1;  /* the '\' char */
    for (i = 93; i < 127; i++)
        baselinetab[i] = bmf->baseline3;

        /* Generate array of character widths; req's fonttab to exist */
    if ((widthtab = (l_int32 *)CALLOC(128, sizeof(l_int32))) == NULL)
        return ERROR_INT("widthtab not made", procName, 1);
    bmf->widthtab = widthtab;        
    for (i = 0; i < 128; i++)
        widthtab[i] = UNDEF;
    for (i = 32; i < 127; i++) {
        bmfGetWidth(bmf, i, &charwidth);
        widthtab[i] = charwidth;
    }

        /* Get the line height of text characters, from the highest
         * ascender to the lowest descender; req's fonttab to exist. */
    pix =  bmfGetPix(bmf, 32);
    maxh =  pixGetHeight(pix);
    pixDestroy(&pix);
    pix =  bmfGetPix(bmf, 58);
    height =  pixGetHeight(pix);
    pixDestroy(&pix);
    maxh = L_MAX(maxh, height);
    pix =  bmfGetPix(bmf, 93);
    height =  pixGetHeight(pix);
    pixDestroy(&pix);
    maxh = L_MAX(maxh, height);
    bmf->lineheight = maxh;

        /* Get the kern width (distance between characters).
         * We let it be the same for all characters in a given
         * font size, and scale it linearly with the size; 
         * req's fonttab to be built first. */
    bmfGetWidth(bmf, 120, &xwidth);
    kernwidth = (l_int32)(0.08 * (l_float32)xwidth + 0.5);
    bmf->kernwidth = L_MAX(1, kernwidth);

        /* Save the space width (between words) */
    bmfGetWidth(bmf, 32, &charwidth);
    bmf->spacewidth = charwidth;

        /* Save the extra vertical space between lines */
    bmf->vertlinesep = (l_int32)(VERT_FRACT_SEP * bmf->lineheight + 0.5);

    return 0;
}

