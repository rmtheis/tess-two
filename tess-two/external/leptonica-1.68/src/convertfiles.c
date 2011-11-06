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
 *  convertfiles.c
 *
 *      Conversion to 1 bpp
 *          l_int32    convertFilesTo1bpp()
 *
 *  These are utility functions that will perform depth conversion
 *  on selected files, writing the results to a specified directory.
 *  We start with conversion to 1 bpp.
 */

#include <string.h>
#include "allheaders.h"


/*------------------------------------------------------------------*
 *                        Conversion to 1 bpp                       *
 *------------------------------------------------------------------*/
/*!
 *  convertFilesTo1bpp()
 *
 *      Input:  dirin
 *              substr (<optional> substring filter on filenames; can be NULL)
 *              upscaling (1, 2 or 4; only for input color or grayscale)
 *              thresh  (global threshold for binarization; use 0 for default)
 *              firstpage
 *              npages (use 0 to do all from @firstpage to the end)
 *              dirout
 *              outformat (IFF_PNG, IFF_TIFF_G4)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Images are sorted lexicographically, and the names in the
 *          output directory are retained except for the extension.
 */
l_int32
convertFilesTo1bpp(const char  *dirin,
                   const char  *substr,
                   l_int32      upscaling,
                   l_int32      thresh,
                   l_int32      firstpage,
                   l_int32      npages,
                   const char  *dirout,
                   l_int32      outformat)
{
l_int32  i, nfiles;
char     buf[512];
char    *fname, *tail, *basename;
PIX     *pixs, *pixg1, *pixg2, *pixb;
SARRAY  *safiles;

    PROCNAME("convertFilesTo1bpp");

    if (!dirin)
        return ERROR_INT("dirin", procName, 1);
    if (!dirout)
        return ERROR_INT("dirout", procName, 1);
    if (upscaling != 1 && upscaling != 2 && upscaling != 4)
        return ERROR_INT("invalid upscaling factor", procName, 1);
    if (thresh <= 0) thresh = 180;
    if (firstpage < 0) firstpage = 0;
    if (npages < 0) npages = 0;
    if (outformat != IFF_TIFF_G4)
        outformat = IFF_PNG;

    safiles = getSortedPathnamesInDirectory(dirin, substr, firstpage, npages);
    if (!safiles)
        return ERROR_INT("safiles not made", procName, 1);
    if ((nfiles = sarrayGetCount(safiles)) == 0) {
        sarrayDestroy(&safiles);
        return ERROR_INT("no matching files in the directory", procName, 1);
    }

    for (i = 0; i < nfiles; i++) {
        fname = sarrayGetString(safiles, i, L_NOCOPY);
        if ((pixs = pixRead(fname)) == NULL) {
            L_WARNING_STRING("Couldn't read file %s\n", procName, fname);
            continue;
        }
        if (pixGetDepth(pixs) == 32)
            pixg1 = pixConvertRGBToLuminance(pixs);
        else
            pixg1 = pixClone(pixs);
        pixg2 = pixRemoveColormap(pixg1, REMOVE_CMAP_TO_GRAYSCALE);
        if (pixGetDepth(pixg2) == 1)
            pixb = pixClone(pixg2);
        else {
            if (upscaling == 1)
                pixb = pixThresholdToBinary(pixg2, thresh);
            else if (upscaling == 2)
                pixb = pixScaleGray2xLIThresh(pixg2, thresh);
            else  /* upscaling == 4 */
                pixb = pixScaleGray4xLIThresh(pixg2, thresh);
        }
        pixDestroy(&pixs);
        pixDestroy(&pixg1);
        pixDestroy(&pixg2);

        splitPathAtDirectory(fname, NULL, &tail);
        splitPathAtExtension(tail, &basename, NULL);
        if (outformat == IFF_TIFF_G4) {
            snprintf(buf, sizeof(buf), "%s/%s.tif", dirout, basename);
            pixWrite(buf, pixb, IFF_TIFF_G4);
        }
        else {
            snprintf(buf, sizeof(buf), "%s/%s.png", dirout, basename);
            pixWrite(buf, pixb, IFF_PNG);
        }
        pixDestroy(&pixb);
        FREE(tail);
        FREE(basename);
    }

    sarrayDestroy(&safiles);
    return 0;
}

